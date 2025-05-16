// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/completion.h>
#include <linux/pci.h>
#include <linux/types.h>

#include "ossl_knl.h"
#include "hinic3_common.h"
#include "hinic3_hw.h"
#include "hinic3_hwdev.h"
#include "hinic3_csr.h"
#include "hinic3_hwif.h"
#include "hinic3_api_cmd.h"
#include "hinic3_mgmt.h"
#include "hinic3_mbox.h"
#include "hinic3_multi_host_mgmt.h"
#include "hinic3_hw_cfg.h"

#define HINIC3_SUPPORT_MAX_PF_NUM 32
#define HINIC3_MBOX_PF_BUSY_ACTIVE_FW	0x2

void hinic3_set_master_host_mbox_enable(struct hinic3_hwdev *hwdev, bool enable)
{
	u32 reg_val;

	if (!IS_MASTER_HOST(hwdev) || HINIC3_FUNC_TYPE(hwdev) != TYPE_PPF)
		return;

	reg_val = hinic3_hwif_read_reg(hwdev->hwif, HINIC3_MULT_HOST_MASTER_MBOX_STATUS_ADDR);
	reg_val = MULTI_HOST_REG_CLEAR(reg_val, MASTER_MBX_STS);
	reg_val |= MULTI_HOST_REG_SET((u8)enable, MASTER_MBX_STS);
	hinic3_hwif_write_reg(hwdev->hwif, HINIC3_MULT_HOST_MASTER_MBOX_STATUS_ADDR, reg_val);

	sdk_info(hwdev->dev_hdl, "Multi-host status: %d, reg value: 0x%x\n",
		 enable, reg_val);
}

static bool hinic3_get_master_host_mbox_enable(void *hwdev)
{
	u32 reg_val;
	struct hinic3_hwdev *dev = hwdev;

	if (!hwdev)
		return false;

	if (!IS_SLAVE_HOST(dev) || HINIC3_FUNC_TYPE(dev) == TYPE_VF)
		return true;

	reg_val = hinic3_hwif_read_reg(dev->hwif, HINIC3_MULT_HOST_MASTER_MBOX_STATUS_ADDR);

	return !!MULTI_HOST_REG_GET(reg_val, MASTER_MBX_STS);
}

bool hinic3_is_multi_bm(void *hwdev)
{
	struct hinic3_hwdev *hw_dev = hwdev;

	if (!hwdev)
		return false;

	return ((IS_BMGW_SLAVE_HOST(hw_dev)) || (IS_BMGW_MASTER_HOST(hw_dev))) ? true : false;
}
EXPORT_SYMBOL(hinic3_is_multi_bm);

bool hinic3_is_slave_host(void *hwdev)
{
	struct hinic3_hwdev *hw_dev = hwdev;

	if (!hwdev) {
		pr_err("hwdev is null\n");
		return false;
	}

	return ((IS_BMGW_SLAVE_HOST(hw_dev)) || (IS_VM_SLAVE_HOST(hw_dev))) ? true : false;
}
EXPORT_SYMBOL(hinic3_is_slave_host);

bool hinic3_is_vm_slave_host(void *hwdev)
{
	struct hinic3_hwdev *hw_dev = hwdev;

	if (!hwdev) {
		pr_err("hwdev is null\n");
		return false;
	}

	return (IS_VM_SLAVE_HOST(hw_dev)) ? true : false;
}
EXPORT_SYMBOL(hinic3_is_vm_slave_host);

bool hinic3_is_bm_slave_host(void *hwdev)
{
	struct hinic3_hwdev *hw_dev = hwdev;

	if (!hwdev) {
		pr_err("hwdev is null\n");
		return false;
	}

	return (IS_BMGW_SLAVE_HOST(hw_dev)) ? true : false;
}
EXPORT_SYMBOL(hinic3_is_bm_slave_host);

static int __send_mbox_to_host(struct hinic3_hwdev *mbox_hwdev,
			       struct hinic3_hwdev *hwdev,
			       enum hinic3_mod_type mod, u8 cmd,
			       void *buf_in, u16 in_size,
			       void *buf_out, u16 *out_size, u32 timeout,
			       enum hinic3_mbox_ack_type ack_type, u16 channel)
{
	u8 dst_host_func_idx;
	struct service_cap *cap = &hwdev->cfg_mgmt->svc_cap;

	if (!mbox_hwdev->chip_present_flag)
		return -EPERM;

	if (!hinic3_get_master_host_mbox_enable(hwdev)) {
		sdk_err(hwdev->dev_hdl, "Master host not initialized\n");
		return -EFAULT;
	}

	if (!mbox_hwdev->mhost_mgmt) {
		/* send to master host in default */
		dst_host_func_idx = hinic3_host_ppf_idx(hwdev, cap->master_host_id);
	} else {
		dst_host_func_idx = IS_MASTER_HOST(hwdev) ?
				mbox_hwdev->mhost_mgmt->shost_ppf_idx :
				mbox_hwdev->mhost_mgmt->mhost_ppf_idx;
	}

	if (ack_type == MBOX_ACK)
		return hinic3_mbox_to_host(mbox_hwdev, dst_host_func_idx,
					  mod, cmd, buf_in, in_size,
					  buf_out, out_size, timeout, channel);
	else
		return hinic3_mbox_to_func_no_ack(mbox_hwdev, dst_host_func_idx,
						 mod, cmd, buf_in, in_size, channel);
}

static int __mbox_to_host(struct hinic3_hwdev *hwdev, enum hinic3_mod_type mod,
			  u8 cmd, void *buf_in, u16 in_size, void *buf_out,
			  u16 *out_size, u32 timeout,
			  enum hinic3_mbox_ack_type ack_type, u16 channel)
{
	struct hinic3_hwdev *mbox_hwdev = hwdev;
	int err;

	if (!IS_MULTI_HOST(hwdev) || HINIC3_IS_VF(hwdev))
		return -EPERM;

	if (hinic3_func_type(hwdev) == TYPE_PF) {
		down(&hwdev->ppf_sem);
		mbox_hwdev = hwdev->ppf_hwdev;
		if (!mbox_hwdev) {
			err = -EINVAL;
			goto release_lock;
		}

		if (!test_bit(HINIC3_HWDEV_MBOX_INITED, &mbox_hwdev->func_state)) {
			err = -EPERM;
			goto release_lock;
		}
	}

	err = __send_mbox_to_host(mbox_hwdev, hwdev, mod, cmd, buf_in, in_size,
				  buf_out, out_size, timeout, ack_type, channel);

release_lock:
	if (hinic3_func_type(hwdev) == TYPE_PF)
		up(&hwdev->ppf_sem);

	return err;
}

int hinic3_mbox_to_host_sync(void *hwdev, enum hinic3_mod_type mod,
			     u8 cmd, void *buf_in, u16 in_size,
			     void *buf_out, u16 *out_size, u32 timeout, u16 channel)
{
	if (!hwdev)
		return -EINVAL;

	return __mbox_to_host((struct hinic3_hwdev *)hwdev, mod, cmd, buf_in,
			in_size, buf_out, out_size, timeout, MBOX_ACK, channel);
}
EXPORT_SYMBOL(hinic3_mbox_to_host_sync);

int hinic3_mbox_to_host_no_ack(struct hinic3_hwdev *hwdev,
			       enum hinic3_mod_type mod, u8 cmd,
			       void *buf_in, u16 in_size, u16 channel)
{
	return __mbox_to_host(hwdev, mod, cmd, buf_in, in_size, NULL, NULL,
			      0, MBOX_NO_ACK, channel);
}

static int __get_func_nic_state_from_pf(struct hinic3_hwdev *hwdev,
					u16 glb_func_idx, u8 *en);
static int __get_func_vroce_state_from_pf(struct hinic3_hwdev *hwdev,
					  u16 glb_func_idx, u8 *en);

int hinic3_sw_func_pf_mbox_handler(void *pri_handle, u16 vf_id, u16 cmd,
				   void *buf_in,
				   u16 in_size, void *buf_out, u16 *out_size)
{
	struct hinic3_hwdev *hwdev = pri_handle;
	struct hinic3_slave_func_nic_state *nic_state = NULL;
	struct hinic3_slave_func_nic_state *out_state = NULL;
	int err;

	switch (cmd) {
	case HINIC3_SW_CMD_GET_SLAVE_FUNC_NIC_STATE:
		nic_state = buf_in;
		out_state = buf_out;
		*out_size = sizeof(*nic_state);

		/* find nic state in PPF func_nic_en bitmap */
		err = __get_func_nic_state_from_pf(hwdev, nic_state->func_idx,
						   &out_state->enable);
		out_state->status = err ? 1 : 0;

		break;
	case HINIC3_SW_CMD_GET_SLAVE_FUNC_VROCE_STATE:
		nic_state = buf_in;
		out_state = buf_out;
		*out_size = sizeof(*nic_state);

		err = __get_func_vroce_state_from_pf(hwdev, nic_state->func_idx,
						     &out_state->enable);
		out_state->status = err ? 1 : 0;

		break;
	default:
		break;
	}

	return 0;
}

static int __master_host_sw_func_handler(struct hinic3_hwdev *hwdev, u16 pf_idx,
					 u8 cmd, void *buf_in, u16 in_size,
					 void *buf_out, u16 *out_size)
{
	struct hinic3_multi_host_mgmt *mhost_mgmt = hwdev->mhost_mgmt;
	struct register_slave_host *out_shost = NULL;
	struct register_slave_host *slave_host = NULL;
	u64 *vroce_en = NULL;

	int err = 0;

	if (!mhost_mgmt)
		return -ENXIO;
	switch (cmd) {
	case HINIC3_SW_CMD_SLAVE_HOST_PPF_REGISTER:
		slave_host = buf_in;
		out_shost = buf_out;
		*out_size = sizeof(*slave_host);
		vroce_en = out_shost->funcs_vroce_en;

		/* just get information about function nic enable */
		if (slave_host->get_nic_en) {
			bitmap_copy((ulong *)out_shost->funcs_nic_en,
				    mhost_mgmt->func_nic_en,
				    HINIC3_MAX_MGMT_FUNCTIONS);

			if (IS_MASTER_HOST(hwdev))
				bitmap_copy((ulong *)vroce_en,
					    mhost_mgmt->func_vroce_en,
					    HINIC3_MAX_MGMT_FUNCTIONS);
			out_shost->status = 0;
			break;
		}

		mhost_mgmt->shost_registered = true;
		mhost_mgmt->shost_host_idx = slave_host->host_id;
		mhost_mgmt->shost_ppf_idx = slave_host->ppf_idx;

		bitmap_copy((ulong *)out_shost->funcs_nic_en,
			    mhost_mgmt->func_nic_en, HINIC3_MAX_MGMT_FUNCTIONS);

		if (IS_MASTER_HOST(hwdev))
			bitmap_copy((ulong *)vroce_en,
				    mhost_mgmt->func_vroce_en,
				    HINIC3_MAX_MGMT_FUNCTIONS);

		sdk_info(hwdev->dev_hdl, "Slave host registers PPF, host_id: %u, ppf_idx: %u\n",
			 slave_host->host_id, slave_host->ppf_idx);

		out_shost->status = 0;
		break;
	case HINIC3_SW_CMD_SLAVE_HOST_PPF_UNREGISTER:
		slave_host = buf_in;
		mhost_mgmt->shost_registered = false;
		sdk_info(hwdev->dev_hdl, "Slave host unregisters PPF, host_id: %u, ppf_idx: %u\n",
			 slave_host->host_id, slave_host->ppf_idx);

		*out_size = sizeof(*slave_host);
		((struct register_slave_host *)buf_out)->status = 0;
		break;

	default:
		err = -EINVAL;
		break;
	}

	return err;
}

static int __event_func_service_state_handler(struct hinic3_hwdev *hwdev,
					      u8 sub_cmd, void *buf_in,
					      u16 in_size, void *buf_out,
					      u16 *out_size)
{
	struct hinic3_event_info event_info = {0};
	struct hinic3_mhost_nic_func_state state = {0};
	struct hinic3_slave_func_nic_state *out_state = NULL;
	struct hinic3_slave_func_nic_state *in_state = buf_in;

	if (!hwdev->event_callback)
		return 0;

	event_info.type = EVENT_COMM_MULTI_HOST_MGMT;
	((struct hinic3_multi_host_mgmt_event *)(void *)event_info.event_data)->sub_cmd = sub_cmd;
	((struct hinic3_multi_host_mgmt_event *)(void *)event_info.event_data)->data = &state;

	state.func_idx = in_state->func_idx;
	state.enable = in_state->enable;

	hwdev->event_callback(hwdev->event_pri_handle, &event_info);

	*out_size = sizeof(*out_state);
	out_state = buf_out;
	out_state->status = state.status;
	if (sub_cmd == HINIC3_MHOST_GET_VROCE_STATE)
		out_state->opened = state.enable;

	return state.status;
}

static int __event_set_func_nic_state(struct hinic3_hwdev *hwdev,
				      void *buf_in, u16 in_size,
				      void *buf_out, u16 *out_size)
{
	return __event_func_service_state_handler(hwdev,
			HINIC3_MHOST_NIC_STATE_CHANGE,
			buf_in, in_size,
			buf_out, out_size);
}

static int __event_set_func_vroce_state(struct hinic3_hwdev *hwdev,
					void *buf_in, u16 in_size,
					void *buf_out, u16 *out_size)
{
	return __event_func_service_state_handler(hwdev,
			HINIC3_MHOST_VROCE_STATE_CHANGE,
			buf_in, in_size,
			buf_out, out_size);
}

static int __event_get_func_vroce_state(struct hinic3_hwdev *hwdev,
					void *buf_in, u16 in_size,
					void *buf_out, u16 *out_size)
{
	return __event_func_service_state_handler(hwdev,
			HINIC3_MHOST_GET_VROCE_STATE,
			buf_in, in_size,
			buf_out, out_size);
}

int vf_sw_func_handler(void *hwdev, u8 cmd, void *buf_in, u16 in_size,
		       void *buf_out, u16 *out_size)
{
	int err = 0;

	switch (cmd) {
	case HINIC3_SW_CMD_SET_SLAVE_FUNC_VROCE_STATE:
		err = __event_set_func_vroce_state(hwdev, buf_in, in_size,
						   buf_out, out_size);
		break;
	case HINIC3_SW_CMD_GET_SLAVE_VROCE_DEVICE_STATE:
		err = __event_get_func_vroce_state(hwdev, buf_in, in_size,
						   buf_out, out_size);
		break;
	default:
		err = -EOPNOTSUPP;
		break;
	}

	return err;
}

static int multi_host_event_handler(struct hinic3_hwdev *hwdev,
				    u8 cmd, void *buf_in, u16 in_size,
				    void *buf_out, u16 *out_size)
{
	int err;

	switch (cmd) {
	case HINIC3_SW_CMD_SET_SLAVE_FUNC_VROCE_STATE:
		err = __event_set_func_vroce_state(hwdev, buf_in, in_size,
						   buf_out, out_size);
		break;
	case HINIC3_SW_CMD_SET_SLAVE_FUNC_NIC_STATE:
		err = __event_set_func_nic_state(hwdev, buf_in, in_size,
						 buf_out, out_size);
		break;
	case HINIC3_SW_CMD_GET_SLAVE_VROCE_DEVICE_STATE:
		err = __event_get_func_vroce_state(hwdev, buf_in, in_size,
						   buf_out, out_size);
		break;
	default:
		err = -EOPNOTSUPP;
		break;
	}

	return err;
}

static int sw_set_slave_func_nic_state(struct hinic3_hwdev *hwdev, u8 cmd,
				       void *buf_in, u16 in_size,
				       void *buf_out, u16 *out_size)
{
	struct hinic3_slave_func_nic_state *nic_state = buf_in;
	struct hinic3_slave_func_nic_state *nic_state_out = buf_out;
	struct hinic3_multi_host_mgmt *mhost_mgmt = hwdev->mhost_mgmt;
	*out_size = sizeof(*nic_state);
	nic_state_out->status = 0;
	sdk_info(hwdev->dev_hdl, "Slave func %u %s nic\n",
		 nic_state->func_idx,
		 nic_state->enable ? "register" : "unregister");

	if (nic_state->enable) {
		set_bit(nic_state->func_idx, mhost_mgmt->func_nic_en);
	} else {
		if ((test_bit(nic_state->func_idx, mhost_mgmt->func_nic_en)) &&
		    nic_state->func_idx >= HINIC3_SUPPORT_MAX_PF_NUM &&
		    (!test_bit(nic_state->func_idx, hwdev->func_probe_in_host))) {
			sdk_warn(hwdev->dev_hdl, "VF%u in vm, delete tap port failed\n",
				 nic_state->func_idx);
			nic_state_out->status = HINIC3_VF_IN_VM;
			return 0;
		}
		clear_bit(nic_state->func_idx, mhost_mgmt->func_nic_en);
	}

	return multi_host_event_handler(hwdev, cmd, buf_in, in_size, buf_out,
					out_size);
}

static int sw_set_slave_vroce_state(struct hinic3_hwdev *hwdev, u8 cmd,
				    void *buf_in, u16 in_size,
				    void *buf_out, u16 *out_size)
{
	struct hinic3_slave_func_nic_state *nic_state = buf_in;
	struct hinic3_slave_func_nic_state *nic_state_out = buf_out;
	struct hinic3_multi_host_mgmt *mhost_mgmt = hwdev->mhost_mgmt;
	int err;

	nic_state = buf_in;
	*out_size = sizeof(*nic_state);
	nic_state_out->status = 0;

	sdk_info(hwdev->dev_hdl, "Slave func %u %s vroce\n", nic_state->func_idx,
		 nic_state->enable ? "register" : "unregister");

	if (nic_state->enable)
		set_bit(nic_state->func_idx,
			mhost_mgmt->func_vroce_en);
	else
		clear_bit(nic_state->func_idx,
			  mhost_mgmt->func_vroce_en);

	err = multi_host_event_handler(hwdev, cmd, buf_in, in_size,
				       buf_out, out_size);

	return err;
}

static int sw_get_slave_vroce_device_state(struct hinic3_hwdev *hwdev, u8 cmd,
					   void *buf_in, u16 in_size,
					   void *buf_out, u16 *out_size)
{
	struct hinic3_slave_func_nic_state *nic_state_out = buf_out;
	int err;

	*out_size = sizeof(struct hinic3_slave_func_nic_state);
	nic_state_out->status = 0;
	err = multi_host_event_handler(hwdev, cmd, buf_in, in_size, buf_out, out_size);

	return err;
}

static void sw_get_slave_netdev_state(struct hinic3_hwdev *hwdev, u8 cmd,
				      void *buf_in, u16 in_size,
				      void *buf_out, u16 *out_size)
{
	struct hinic3_slave_func_nic_state *nic_state = buf_in;
	struct hinic3_slave_func_nic_state *nic_state_out = buf_out;

	*out_size = sizeof(*nic_state);
	nic_state_out->status = 0;
	nic_state_out->opened =
		test_bit(nic_state->func_idx,
			 hwdev->netdev_setup_state) ? 1 : 0;
}

static int __slave_host_sw_func_handler(struct hinic3_hwdev *hwdev, u16 pf_idx,
					u8 cmd, void *buf_in, u16 in_size,
					void *buf_out, u16 *out_size)
{
	struct hinic3_multi_host_mgmt *mhost_mgmt = hwdev->mhost_mgmt;
	int err = 0;

	if (!mhost_mgmt)
		return -ENXIO;
	switch (cmd) {
	case HINIC3_SW_CMD_SET_SLAVE_FUNC_NIC_STATE:
		err = sw_set_slave_func_nic_state(hwdev, cmd, buf_in, in_size,
						  buf_out, out_size);
		break;
	case HINIC3_SW_CMD_SET_SLAVE_FUNC_VROCE_STATE:
		err = sw_set_slave_vroce_state(hwdev, cmd, buf_in, in_size,
					       buf_out, out_size);
		break;
	case HINIC3_SW_CMD_GET_SLAVE_VROCE_DEVICE_STATE:
		err = sw_get_slave_vroce_device_state(hwdev, cmd,
						      buf_in, in_size,
						      buf_out, out_size);
		break;
	case HINIC3_SW_CMD_GET_SLAVE_NETDEV_STATE:
		sw_get_slave_netdev_state(hwdev, cmd, buf_in, in_size,
					  buf_out, out_size);
		break;
	default:
		err = -EINVAL;
		break;
	}

	return err;
}

static int sw_func_ppf_mbox_handler(void *handle, u16 pf_idx, u16 vf_id, u16 cmd,
				    void *buf_in, u16 in_size, void *buf_out,
				    u16 *out_size)
{
	struct hinic3_hwdev *hwdev = handle;
	int err;

	if (IS_MASTER_HOST(hwdev))
		err = __master_host_sw_func_handler(hwdev, pf_idx, (u8)cmd, buf_in,
						    in_size, buf_out, out_size);
	else if (IS_SLAVE_HOST(hwdev))
		err = __slave_host_sw_func_handler(hwdev, pf_idx, (u8)cmd, buf_in,
						   in_size, buf_out, out_size);
	else
		err = -EINVAL;

	if (err)
		sdk_err(hwdev->dev_hdl, "PPF process sw funcs cmd %u failed, err: %d\n",
			cmd, err);

	return err;
}

static int __ppf_process_mbox_msg(struct hinic3_hwdev *hwdev, u16 pf_idx, u16 vf_id,
				  enum hinic3_mod_type mod, u8 cmd, void *buf_in,
				  u16 in_size, void *buf_out, u16 *out_size)
{
	/* when not support return err */
	int err = -EFAULT;

	if (IS_SLAVE_HOST(hwdev)) {
		err = hinic3_mbox_to_host_sync(hwdev, mod, cmd, buf_in, in_size,
					       buf_out, out_size, 0, HINIC3_CHANNEL_COMM);
		if (err)
			sdk_err(hwdev->dev_hdl, "Send mailbox to mPF failed, err: %d\n",
				err);
	} else if (IS_MASTER_HOST(hwdev)) {
		if (mod == HINIC3_MOD_COMM && cmd == COMM_MGMT_CMD_START_FLR)
			err = hinic3_pf_to_mgmt_no_ack(hwdev, mod, cmd, buf_in,
						       in_size);
		else
			err = hinic3_pf_msg_to_mgmt_sync(hwdev, mod, cmd, buf_in,
							 in_size, buf_out,
							 out_size, 0U);
		if (err && err != HINIC3_MBOX_PF_BUSY_ACTIVE_FW)
			sdk_err(hwdev->dev_hdl, "PF mbox mod %d cmd %u callback handler err: %d\n",
				mod, cmd, err);
	}

	return err;
}

static int hinic3_ppf_process_mbox_msg(struct hinic3_hwdev *hwdev, u16 pf_idx, u16 vf_id,
				       enum hinic3_mod_type mod, u8 cmd, void *buf_in,
				       u16 in_size, void *buf_out, u16 *out_size)
{
	bool same_host = false;
	int err = -EFAULT;

	/* Currently, only the master ppf and slave ppf communicate with each
	 * other through ppf messages. If other PF/VFs need to communicate
	 * with the PPF, modify the same_host based on the
	 * hinic3_get_hw_pf_infos information.
	 */

	switch (hwdev->func_mode) {
	case FUNC_MOD_MULTI_VM_MASTER:
	case FUNC_MOD_MULTI_BM_MASTER:
		if (!same_host)
			err = __ppf_process_mbox_msg(hwdev, pf_idx, vf_id,
						     mod, cmd, buf_in, in_size,
						     buf_out, out_size);
		else
			sdk_warn(hwdev->dev_hdl, "Doesn't support PPF mbox message in BM master\n");

		break;
	case FUNC_MOD_MULTI_VM_SLAVE:
	case FUNC_MOD_MULTI_BM_SLAVE:
		same_host = true;
		if (same_host)
			err = __ppf_process_mbox_msg(hwdev, pf_idx, vf_id,
						     mod, cmd, buf_in, in_size,
						     buf_out, out_size);
		else
			sdk_warn(hwdev->dev_hdl, "Doesn't support receiving control messages from BM master\n");

		break;
	default:
		sdk_warn(hwdev->dev_hdl, "Doesn't support PPF mbox message\n");

		break;
	}

	return err;
}

static int comm_ppf_mbox_handler(void *handle, u16 pf_idx, u16 vf_id, u16 cmd,
				 void *buf_in, u16 in_size, void *buf_out,
				 u16 *out_size)
{
	return hinic3_ppf_process_mbox_msg(handle, pf_idx, vf_id, HINIC3_MOD_COMM,
					  (u8)cmd, buf_in, in_size, buf_out,
					  out_size);
}

static int hilink_ppf_mbox_handler(void *handle, u16 pf_idx, u16 vf_id, u16 cmd,
				   void *buf_in, u16 in_size,
				   void *buf_out, u16 *out_size)
{
	return hinic3_ppf_process_mbox_msg(handle, pf_idx, vf_id,
					  HINIC3_MOD_HILINK, (u8)cmd, buf_in,
					  in_size, buf_out, out_size);
}

static int hinic3_nic_ppf_mbox_handler(void *handle, u16 pf_idx, u16 vf_id, u16 cmd,
				       void *buf_in, u16 in_size,
				       void *buf_out, u16 *out_size)
{
	return hinic3_ppf_process_mbox_msg(handle, pf_idx, vf_id,
					  HINIC3_MOD_L2NIC, (u8)cmd, buf_in, in_size,
					  buf_out, out_size);
}

static int hinic3_register_slave_ppf(struct hinic3_hwdev *hwdev, bool registered)
{
	struct register_slave_host *host_info = NULL;
	u16 out_size = sizeof(struct register_slave_host);
	u8 cmd;
	int err;

	if (!IS_SLAVE_HOST(hwdev))
		return -EINVAL;

	if (UNSUPPORT_HOT_PLUG((struct hinic3_hwdev *)hwdev))
		return 0;

	host_info = kcalloc(1, sizeof(struct register_slave_host), GFP_KERNEL);
	if (!host_info)
		return -ENOMEM;

	cmd = registered ? HINIC3_SW_CMD_SLAVE_HOST_PPF_REGISTER :
		HINIC3_SW_CMD_SLAVE_HOST_PPF_UNREGISTER;

	host_info->host_id = hinic3_pcie_itf_id(hwdev);
	host_info->ppf_idx = hinic3_ppf_idx(hwdev);

	err = hinic3_mbox_to_host_sync(hwdev, HINIC3_MOD_SW_FUNC, cmd,
				       host_info, sizeof(struct register_slave_host), host_info,
				       &out_size, 0, HINIC3_CHANNEL_COMM);
	if (!!err || !out_size || host_info->status) {
		sdk_err(hwdev->dev_hdl, "Failed to %s slave host, err: %d, out_size: 0x%x, status: 0x%x\n",
			registered ? "register" : "unregister", err, out_size, host_info->status);

		kfree(host_info);
		return -EFAULT;
	}
	bitmap_copy(hwdev->mhost_mgmt->func_nic_en,
		    (ulong *)host_info->funcs_nic_en,
		    HINIC3_MAX_MGMT_FUNCTIONS);

	if (IS_SLAVE_HOST(hwdev))
		bitmap_copy(hwdev->mhost_mgmt->func_vroce_en,
			    (ulong *)host_info->funcs_vroce_en,
			    HINIC3_MAX_MGMT_FUNCTIONS);

	kfree(host_info);
	return 0;
}

static int get_host_id_by_func_id(struct hinic3_hwdev *hwdev, u16 func_idx,
				  u8 *host_id)
{
	struct hinic3_hw_pf_infos *pf_infos = NULL;
	u16 vf_id_start, vf_id_end;
	int i;

	if (!hwdev || !host_id || !hwdev->mhost_mgmt)
		return -EINVAL;

	pf_infos = &hwdev->mhost_mgmt->pf_infos;

	for (i = 0; i < pf_infos->num_pfs; i++) {
		if (func_idx == pf_infos->infos[i].glb_func_idx) {
			*host_id = pf_infos->infos[i].itf_idx;
			return 0;
		}

		vf_id_start = pf_infos->infos[i].glb_pf_vf_offset + 1;
		vf_id_end = pf_infos->infos[i].glb_pf_vf_offset +
			    pf_infos->infos[i].max_vfs;
		if (func_idx >= vf_id_start && func_idx <= vf_id_end) {
			*host_id = pf_infos->infos[i].itf_idx;
			return 0;
		}
	}

	return -EFAULT;
}

static int set_slave_func_nic_state(struct hinic3_hwdev *hwdev,
				    struct hinic3_func_nic_state *state)
{
	struct hinic3_slave_func_nic_state nic_state = {0};
	u16 out_size = sizeof(nic_state);
	u8 cmd = HINIC3_SW_CMD_SET_SLAVE_FUNC_NIC_STATE;
	int err;

	nic_state.func_idx = state->func_idx;
	nic_state.enable = state->state;
	nic_state.vroce_flag = state->vroce_flag;

	if (state->vroce_flag)
		cmd = HINIC3_SW_CMD_SET_SLAVE_FUNC_VROCE_STATE;

	err = hinic3_mbox_to_host_sync(hwdev, HINIC3_MOD_SW_FUNC,
				       cmd, &nic_state, sizeof(nic_state),
				       &nic_state, &out_size, 0, HINIC3_CHANNEL_COMM);
	if (err == MBOX_ERRCODE_UNKNOWN_DES_FUNC) {
		sdk_warn(hwdev->dev_hdl,
			 "Can not notify func %u %s state because slave host isn't initialized\n",
			 state->func_idx, state->vroce_flag ? "vroce" : "nic");
	} else if (err || !out_size || nic_state.status) {
		sdk_err(hwdev->dev_hdl,
			"Failed to set slave %s state, err: %d, out_size: 0x%x, status: 0x%x\n",
			state->vroce_flag ? "vroce" : "nic",
			err, out_size, nic_state.status);
		return -EFAULT;
	}

	return 0;
}

static int get_slave_func_netdev_state(struct hinic3_hwdev *hwdev, u16 func_idx, int *opened)
{
	struct hinic3_slave_func_nic_state nic_state = {0};
	u16 out_size = sizeof(nic_state);
	int err;

	nic_state.func_idx = func_idx;
	err = hinic3_mbox_to_host_sync(hwdev, HINIC3_MOD_SW_FUNC,
				       HINIC3_SW_CMD_GET_SLAVE_NETDEV_STATE,
				       &nic_state, sizeof(nic_state), &nic_state,
				       &out_size, 0, HINIC3_CHANNEL_COMM);
	if (err == MBOX_ERRCODE_UNKNOWN_DES_FUNC) {
		sdk_warn(hwdev->dev_hdl,
			 "Can not get func %u netdev state because slave host isn't initialized\n",
			 func_idx);
	} else if (err || !out_size || nic_state.status) {
		sdk_err(hwdev->dev_hdl,
			"Failed to get netdev state, err: %d, out_size: 0x%x, status: 0x%x\n",
			err, out_size, nic_state.status);
		return -EFAULT;
	}

	*opened = nic_state.opened;
	return 0;
}

static int set_nic_state_params_valid(void *hwdev,
				      struct hinic3_func_nic_state *state)
{
	struct hinic3_multi_host_mgmt *mhost_mgmt = NULL;
	struct hinic3_hwdev *ppf_hwdev = hwdev;

	if (!hwdev || !state)
		return -EINVAL;

	if (hinic3_func_type(hwdev) != TYPE_PPF)
		ppf_hwdev = ((struct hinic3_hwdev *)hwdev)->ppf_hwdev;

	if (!ppf_hwdev || !IS_MASTER_HOST(ppf_hwdev))
		return -EINVAL;

	mhost_mgmt = ppf_hwdev->mhost_mgmt;
	if (!mhost_mgmt || state->func_idx >= HINIC3_MAX_MGMT_FUNCTIONS)
		return -EINVAL;

	return 0;
}

static int get_func_current_state(struct hinic3_multi_host_mgmt *mhost_mgmt,
				  struct hinic3_func_nic_state *state,
				  int *old_state)
{
	ulong *func_bitmap = NULL;

	if (state->vroce_flag == 1)
		func_bitmap = mhost_mgmt->func_vroce_en;
	else
		func_bitmap = mhost_mgmt->func_nic_en;

	*old_state = test_bit(state->func_idx, func_bitmap) ? 1 : 0;
	if (state->state == HINIC3_FUNC_NIC_DEL)
		clear_bit(state->func_idx, func_bitmap);
	else if (state->state == HINIC3_FUNC_NIC_ADD)
		set_bit(state->func_idx, func_bitmap);
	else
		return -EINVAL;

	return 0;
}

static bool check_vroce_state(struct hinic3_multi_host_mgmt *mhost_mgmt,
			      struct hinic3_func_nic_state *state)
{
	bool is_ready = true;
	ulong *func_bitmap = mhost_mgmt->func_vroce_en;

	if (!state->vroce_flag && state->state == HINIC3_FUNC_NIC_DEL)
		is_ready = test_bit(state->func_idx, func_bitmap) ? false : true;

	return is_ready;
}

int hinic3_set_func_nic_state(void *hwdev, struct hinic3_func_nic_state *state)
{
	struct hinic3_multi_host_mgmt *mhost_mgmt = NULL;
	struct hinic3_hwdev *ppf_hwdev = hwdev;
	u8 host_enable;
	int err, old_state = 0;
	u8 host_id = 0;

	err = set_nic_state_params_valid(hwdev, state);
	if (err)
		return err;

	mhost_mgmt = ppf_hwdev->mhost_mgmt;

	if (IS_MASTER_HOST(ppf_hwdev) &&
	    !check_vroce_state(mhost_mgmt, state)) {
		sdk_warn(ppf_hwdev->dev_hdl,
			 "Should disable vroce before disable nic for function %u\n",
			 state->func_idx);
		return -EFAULT;
	}

	err = get_func_current_state(mhost_mgmt, state, &old_state);
	if (err) {
		sdk_err(ppf_hwdev->dev_hdl, "Failed to get function %u current state, err: %d\n",
			state->func_idx, err);
		return err;
	}

	err = get_host_id_by_func_id(ppf_hwdev, state->func_idx, &host_id);
	if (err) {
		sdk_err(ppf_hwdev->dev_hdl,
			"Failed to get function %u host id, err: %d\n", state->func_idx, err);
		if (state->vroce_flag)
			return -EFAULT;

		old_state ? set_bit(state->func_idx, mhost_mgmt->func_nic_en) :
			clear_bit(state->func_idx, mhost_mgmt->func_nic_en);
		return -EFAULT;
	}

	err = hinic3_get_slave_host_enable(hwdev, host_id, &host_enable);
	if (err != 0) {
		sdk_err(ppf_hwdev->dev_hdl,
			"Get slave host %u enable failed, ret %d\n", host_id, err);
		return err;
	}
	sdk_info(ppf_hwdev->dev_hdl, "Set slave host %u(status: %u) func %u %s %s\n",
		 host_id, host_enable, state->func_idx,
		 state->state ? "enable" : "disable", state->vroce_flag ? "vroce" : "nic");

	if (!host_enable)
		return 0;

	/* notify slave host */
	err = set_slave_func_nic_state(hwdev, state);
	if (err) {
		if (state->vroce_flag)
			return -EFAULT;

		old_state ? set_bit(state->func_idx, mhost_mgmt->func_nic_en) :
			clear_bit(state->func_idx, mhost_mgmt->func_nic_en);
		return err;
	}

	return 0;
}
EXPORT_SYMBOL(hinic3_set_func_nic_state);

int hinic3_get_netdev_state(void *hwdev, u16 func_idx, int *opened)
{
	struct hinic3_hwdev *ppf_hwdev = hwdev;
	int err;
	u8 host_enable;
	u8 host_id = 0;
	struct hinic3_func_nic_state state = {0};

	*opened = 0;
	state.func_idx = func_idx;
	err = set_nic_state_params_valid(hwdev, &state);
	if (err)
		return err;

	err = get_host_id_by_func_id(ppf_hwdev, func_idx, &host_id);
	if (err) {
		sdk_err(ppf_hwdev->dev_hdl, "Failed to get function %u host id, err: %d\n",
			func_idx, err);
		return -EFAULT;
	}

	err = hinic3_get_slave_host_enable(hwdev, host_id, &host_enable);
	if (err != 0) {
		sdk_err(ppf_hwdev->dev_hdl, "Get slave host %u enable failed, ret %d\n",
			host_id, err);
		return err;
	}
	if (!host_enable)
		return 0;

	return get_slave_func_netdev_state(hwdev, func_idx, opened);
}
EXPORT_SYMBOL(hinic3_get_netdev_state);

static int __get_func_nic_state_from_pf(struct hinic3_hwdev *hwdev,
					u16 glb_func_idx, u8 *en)
{
	struct hinic3_multi_host_mgmt *mhost_mgmt = NULL;
	struct hinic3_hwdev *ppf_hwdev = hwdev;

	down(&hwdev->ppf_sem);
	if (hinic3_func_type(hwdev) != TYPE_PPF)
		ppf_hwdev = ((struct hinic3_hwdev *)hwdev)->ppf_hwdev;

	if (!ppf_hwdev || !ppf_hwdev->mhost_mgmt) {
		up(&hwdev->ppf_sem);
		return -EFAULT;
	}

	mhost_mgmt = ppf_hwdev->mhost_mgmt;
	*en = !!test_bit(glb_func_idx, mhost_mgmt->func_nic_en);
	up(&hwdev->ppf_sem);

	return 0;
}

static int __get_func_vroce_state_from_pf(struct hinic3_hwdev *hwdev,
					  u16 glb_func_idx, u8 *en)
{
	struct hinic3_multi_host_mgmt *mhost_mgmt = NULL;
	struct hinic3_hwdev *ppf_hwdev = hwdev;

	down(&hwdev->ppf_sem);
	if (hinic3_func_type(hwdev) != TYPE_PPF)
		ppf_hwdev = ((struct hinic3_hwdev *)hwdev)->ppf_hwdev;

	if (!ppf_hwdev || !ppf_hwdev->mhost_mgmt) {
		up(&hwdev->ppf_sem);
		return -EFAULT;
	}

	mhost_mgmt = ppf_hwdev->mhost_mgmt;
	*en = !!test_bit(glb_func_idx, mhost_mgmt->func_vroce_en);
	up(&hwdev->ppf_sem);

	return 0;
}

static int __get_vf_func_nic_state(struct hinic3_hwdev *hwdev, u16 glb_func_idx,
				   bool *en)
{
	struct hinic3_slave_func_nic_state nic_state = {0};
	u16 out_size = sizeof(nic_state);
	int err;

	if (hinic3_func_type(hwdev) == TYPE_VF) {
		nic_state.func_idx = glb_func_idx;
		err = hinic3_mbox_to_pf(hwdev, HINIC3_MOD_SW_FUNC,
					HINIC3_SW_CMD_GET_SLAVE_FUNC_NIC_STATE,
					&nic_state, sizeof(nic_state),
					&nic_state, &out_size, 0, HINIC3_CHANNEL_COMM);
		if (err || !out_size || nic_state.status) {
			sdk_err(hwdev->dev_hdl,
				"Failed to get vf %u state, err: %d, out_size: %u, status: 0x%x\n",
				glb_func_idx, err, out_size, nic_state.status);
			return -EFAULT;
		}

		*en = !!nic_state.enable;

		return 0;
	}

	return -EFAULT;
}

static int __get_func_vroce_state(struct hinic3_hwdev *hwdev, u16 glb_func_idx,
				  u8 *en)
{
	struct hinic3_slave_func_nic_state vroce_state = {0};
	u16 out_size = sizeof(vroce_state);
	int err;

	if (hinic3_func_type(hwdev) == TYPE_VF) {
		vroce_state.func_idx = glb_func_idx;
		err = hinic3_mbox_to_pf(hwdev, HINIC3_MOD_SW_FUNC,
					HINIC3_SW_CMD_GET_SLAVE_FUNC_VROCE_STATE,
					&vroce_state, sizeof(vroce_state),
					&vroce_state, &out_size, 0, HINIC3_CHANNEL_COMM);
		if (err || !out_size || vroce_state.status) {
			sdk_err(hwdev->dev_hdl,
				"Failed to get vf %u state, err: %d, out_size: %u, status: 0x%x\n",
				glb_func_idx, err, out_size, vroce_state.status);
			return -EFAULT;
		}

		*en = !!vroce_state.enable;

		return 0;
	}

	return __get_func_vroce_state_from_pf(hwdev, glb_func_idx, en);
}

int hinic3_get_func_vroce_enable(void *hwdev, u16 glb_func_idx, u8 *en)
{
	if (!hwdev || !en)
		return -EINVAL;

	return __get_func_vroce_state(hwdev, glb_func_idx, en);
}
EXPORT_SYMBOL(hinic3_get_func_vroce_enable);

int hinic3_get_func_nic_enable(void *hwdev, u16 glb_func_idx, bool *en)
{
	u8 nic_en;
	int err;

	if (!hwdev || !en)
		return -EINVAL;

	/* if single host, return true. */
	if (!IS_MULTI_HOST((struct hinic3_hwdev *)hwdev) ||
	    UNSUPPORT_HOT_PLUG((struct hinic3_hwdev *)hwdev)) {
		*en = true;
		return 0;
	}

	if (!IS_SLAVE_HOST((struct hinic3_hwdev *)hwdev)) {
		/* if card mode is OVS, VFs don't need attach_uld, so return false. */
		if (hinic3_func_type(hwdev) == TYPE_VF &&
		    hinic3_support_ovs(hwdev, NULL))
			*en = false;
		else
			*en = true;

		return 0;
	}

	/* PF in slave host should be probe in CHIP_MODE_VMGW
	 * mode for pxe install.
	 * PF num need (0 ~31)
	 */
	if (hinic3_func_type(hwdev) != TYPE_VF &&
	    IS_VM_SLAVE_HOST((struct hinic3_hwdev *)hwdev) &&
	    glb_func_idx < HINIC3_SUPPORT_MAX_PF_NUM) {
		*en = true;
		return 0;
	}

	/* try to get function nic state in sdk directly */
	err = __get_func_nic_state_from_pf(hwdev, glb_func_idx, &nic_en);
	if (err) {
		if (glb_func_idx < HINIC3_SUPPORT_MAX_PF_NUM)
			return err;
	} else {
		*en = !!nic_en;
		return 0;
	}

	return __get_vf_func_nic_state(hwdev, glb_func_idx, en);
}

static int slave_host_init(struct hinic3_hwdev *hwdev)
{
	int err;

	if (IS_SLAVE_HOST(hwdev)) {
		/* PXE doesn't support to receive mbox from master host */
		hinic3_set_slave_host_enable(hwdev,
					     hinic3_pcie_itf_id(hwdev), true);
		if ((IS_VM_SLAVE_HOST(hwdev) &&
		     hinic3_get_master_host_mbox_enable(hwdev)) ||
		     IS_BMGW_SLAVE_HOST(hwdev)) {
			err = hinic3_register_slave_ppf(hwdev, true);
			if (err) {
				hinic3_set_slave_host_enable(hwdev, hinic3_pcie_itf_id(hwdev), false);
				return err;
			}
		}
	} else {
		/* slave host can send message to mgmt cpu
		 * after setup master mbox
		 */
		hinic3_set_master_host_mbox_enable(hwdev, true);
	}

	return 0;
}

int hinic3_multi_host_mgmt_init(struct hinic3_hwdev *hwdev)
{
	int err;
	struct service_cap *cap = &hwdev->cfg_mgmt->svc_cap;
	int is_use_vram, is_in_kexec;

	if (!IS_MULTI_HOST(hwdev) || !HINIC3_IS_PPF(hwdev))
		return 0;

	is_use_vram = get_use_vram_flag();
	if (is_use_vram != 0) {
		snprintf(hwdev->mhost_mgmt_name, VRAM_NAME_MAX_LEN, "%s",
			 VRAM_NIC_MHOST_MGMT);
		hwdev->mhost_mgmt = hi_vram_kalloc(hwdev->mhost_mgmt_name,
						   sizeof(*hwdev->mhost_mgmt));
	} else {
		hwdev->mhost_mgmt = kcalloc(1, sizeof(*hwdev->mhost_mgmt), GFP_KERNEL);
	}

	if (!hwdev->mhost_mgmt)
		return -ENOMEM;

	hwdev->mhost_mgmt->shost_ppf_idx = hinic3_host_ppf_idx(hwdev, HINIC3_MGMT_SHOST_HOST_ID);
	hwdev->mhost_mgmt->mhost_ppf_idx = hinic3_host_ppf_idx(hwdev, cap->master_host_id);

	err = hinic3_get_hw_pf_infos(hwdev, &hwdev->mhost_mgmt->pf_infos, HINIC3_CHANNEL_COMM);
	if (err)
		goto out_free_mhost_mgmt;

	hinic3_register_ppf_mbox_cb(hwdev, HINIC3_MOD_COMM, hwdev, comm_ppf_mbox_handler);
	hinic3_register_ppf_mbox_cb(hwdev, HINIC3_MOD_L2NIC, hwdev, hinic3_nic_ppf_mbox_handler);
	hinic3_register_ppf_mbox_cb(hwdev, HINIC3_MOD_HILINK, hwdev, hilink_ppf_mbox_handler);
	hinic3_register_ppf_mbox_cb(hwdev, HINIC3_MOD_SW_FUNC, hwdev, sw_func_ppf_mbox_handler);

	is_in_kexec = vram_get_kexec_flag();
	if (is_in_kexec == 0) {
		bitmap_zero(hwdev->mhost_mgmt->func_nic_en, HINIC3_MAX_MGMT_FUNCTIONS);
		bitmap_zero(hwdev->mhost_mgmt->func_vroce_en, HINIC3_MAX_MGMT_FUNCTIONS);
	}

	/* Slave host:
	 * register slave host ppf functions
	 * Get function's nic state
	 */
	err = slave_host_init(hwdev);
	if (err)
		goto out_free_mhost_mgmt;

	return 0;

out_free_mhost_mgmt:
	if (is_use_vram != 0)
		hi_vram_kfree((void *)hwdev->mhost_mgmt, hwdev->mhost_mgmt_name,
			      sizeof(*hwdev->mhost_mgmt));
	else
		kfree(hwdev->mhost_mgmt);
	hwdev->mhost_mgmt = NULL;

	return err;
}

int hinic3_multi_host_mgmt_free(struct hinic3_hwdev *hwdev)
{
	int is_use_vram;

	if (!IS_MULTI_HOST(hwdev) || !HINIC3_IS_PPF(hwdev))
		return 0;

	if (IS_SLAVE_HOST(hwdev)) {
		hinic3_register_slave_ppf(hwdev, false);

		hinic3_set_slave_host_enable(hwdev,
					     hinic3_pcie_itf_id(hwdev), false);
	} else {
		hinic3_set_master_host_mbox_enable(hwdev, false);
	}

	hinic3_unregister_ppf_mbox_cb(hwdev, HINIC3_MOD_COMM);
	hinic3_unregister_ppf_mbox_cb(hwdev, HINIC3_MOD_L2NIC);
	hinic3_unregister_ppf_mbox_cb(hwdev, HINIC3_MOD_HILINK);
	hinic3_unregister_ppf_mbox_cb(hwdev, HINIC3_MOD_SW_FUNC);

	is_use_vram = get_use_vram_flag();
	if (is_use_vram != 0)
		hi_vram_kfree((void *)hwdev->mhost_mgmt, hwdev->mhost_mgmt_name,
			      sizeof(*hwdev->mhost_mgmt));
	else
		kfree(hwdev->mhost_mgmt);

	hwdev->mhost_mgmt = NULL;

	return 0;
}

int hinic3_get_mhost_func_nic_enable(void *hwdev, u16 func_id, bool *en)
{
	struct hinic3_hwdev *dev = hwdev;
	u8 func_en;
	int ret;

	if (!hwdev || !en || func_id >=  HINIC3_MAX_MGMT_FUNCTIONS || !IS_MULTI_HOST(dev))
		return -EINVAL;

	ret = __get_func_nic_state_from_pf(hwdev, func_id, &func_en);
	if (ret)
		return ret;

	*en = !!func_en;

	return 0;
}
EXPORT_SYMBOL(hinic3_get_mhost_func_nic_enable);

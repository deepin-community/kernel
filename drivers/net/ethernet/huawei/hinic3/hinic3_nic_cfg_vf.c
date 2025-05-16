// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/etherdevice.h>
#include <linux/if_vlan.h>
#include <linux/ethtool.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/module.h>

#include "ossl_knl.h"
#include "hinic3_crm.h"
#include "hinic3_hw.h"
#include "hinic3_nic_io.h"
#include "hinic3_nic_cfg.h"
#include "hinic3_srv_nic.h"
#include "hinic3_nic.h"
#include "hinic3_nic_cmdq.h"
#include "nic_mpu_cmd.h"
#include "nic_npu_cmd.h"

/*lint -e806*/
static unsigned char set_vf_link_state;
module_param(set_vf_link_state, byte, 0444);
MODULE_PARM_DESC(set_vf_link_state, "Set vf link state, 0 represents link auto, 1 represents link always up, 2 represents link always down. - default is 0.");
/*lint +e806*/

/* In order to adapt different linux version */
enum {
	HINIC3_IFLA_VF_LINK_STATE_AUTO, /* link state of the uplink */
	HINIC3_IFLA_VF_LINK_STATE_ENABLE, /* link always up */
	HINIC3_IFLA_VF_LINK_STATE_DISABLE, /* link always down */
};

#define NIC_CVLAN_INSERT_ENABLE 0x1
#define NIC_QINQ_INSERT_ENABLE   0X3
static int hinic3_set_vlan_ctx(struct hinic3_nic_io *nic_io, u16 func_id,
			       u16 vlan_tag, u16 q_id, bool add)
{
	struct hinic3_cmd_buf *cmd_buf = NULL;
	u64 out_param = 0;
	int err;
	u8 cmd, vlan_mode;

	cmd_buf = hinic3_alloc_cmd_buf(nic_io->hwdev);
	if (cmd_buf == NULL) {
		nic_err(nic_io->dev_hdl, "Failed to allocate cmd buf\n");
		return -ENOMEM;
	}

	vlan_mode = add ?  NIC_QINQ_INSERT_ENABLE : NIC_CVLAN_INSERT_ENABLE;

	cmd = nic_io->cmdq_ops->prepare_cmd_buf_modify_svlan(cmd_buf, func_id,
		vlan_tag, q_id, vlan_mode);

	err = hinic3_cmdq_direct_resp(nic_io->hwdev, HINIC3_MOD_L2NIC,
				      cmd, cmd_buf, &out_param,
				      0, HINIC3_CHANNEL_NIC);

	hinic3_free_cmd_buf(nic_io->hwdev, cmd_buf);

	if ((err != 0) || out_param != 0) {
		nic_err(nic_io->dev_hdl, "Failed to set vlan context, err: %d, out_param: 0x%llx\n",
			err, out_param);
		return -EFAULT;
	}

	return err;
}

int hinic3_cfg_vf_vlan(struct hinic3_nic_io *nic_io, u8 opcode, u16 vid,
		       u8 qos, int vf_id)
{
	struct hinic3_cmd_vf_vlan_config vf_vlan;
	u16 out_size = sizeof(vf_vlan);
	u16 glb_func_id;
	int err;
	u16 vlan_tag;

	/* VLAN 0 is a special case, don't allow it to be removed */
	if (!vid && opcode == HINIC3_CMD_OP_DEL)
		return 0;

	memset(&vf_vlan, 0, sizeof(vf_vlan));

	vf_vlan.opcode = opcode;
	vf_vlan.func_id = hinic3_glb_pf_vf_offset(nic_io->hwdev) + (u16)vf_id;
	vf_vlan.vlan_id = vid;
	vf_vlan.qos = qos;

	err = l2nic_msg_to_mgmt_sync(nic_io->hwdev, HINIC3_NIC_CMD_CFG_VF_VLAN,
				     &vf_vlan, sizeof(vf_vlan),
				     &vf_vlan, &out_size);
	if (err || !out_size || vf_vlan.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to set VF %d vlan, err: %d, status: 0x%x,out size: 0x%x\n",
			HW_VF_ID_TO_OS(vf_id), err, vf_vlan.msg_head.status,
			out_size);
		return -EFAULT;
	}

	vlan_tag = vid + (u16)(qos << VLAN_PRIO_SHIFT);

	glb_func_id = hinic3_glb_pf_vf_offset(nic_io->hwdev) + (u16)vf_id;
	err = hinic3_set_vlan_ctx(nic_io, glb_func_id, vlan_tag,
				  NIC_CONFIG_ALL_QUEUE_VLAN_CTX,
				  opcode == HINIC3_CMD_OP_ADD);
	if (err != 0) {
		nic_err(nic_io->dev_hdl, "Failed to set VF %d vlan ctx, err: %d\n",
			HW_VF_ID_TO_OS(vf_id), err);

		/* rollback vlan config */
		if (opcode == HINIC3_CMD_OP_DEL)
			vf_vlan.opcode = HINIC3_CMD_OP_ADD;
		else
			vf_vlan.opcode = HINIC3_CMD_OP_DEL;
		l2nic_msg_to_mgmt_sync(nic_io->hwdev,
				       HINIC3_NIC_CMD_CFG_VF_VLAN, &vf_vlan,
				       sizeof(vf_vlan), &vf_vlan, &out_size);
		return err;
	}

	return 0;
}

/* this function just be called by hinic3_ndo_set_vf_mac,
 * others are not permitted.
 */
int hinic3_set_vf_mac(void *hwdev, int vf_id, const unsigned char *mac_addr)
{
	struct vf_data_storage *vf_info = NULL;
	struct hinic3_nic_io *nic_io = NULL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	vf_info = nic_io->vf_infos + HW_VF_ID_TO_OS(vf_id);
#ifndef __VMWARE__
	/* duplicate request, so just return success */
	if (ether_addr_equal(vf_info->user_mac_addr, mac_addr))
		return 0;

#else
	if (ether_addr_equal(vf_info->user_mac_addr, mac_addr))
		return 0;
#endif
	ether_addr_copy(vf_info->user_mac_addr, mac_addr);

	return 0;
}

int hinic3_add_vf_vlan(void *hwdev, int vf_id, u16 vlan, u8 qos)
{
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	err = hinic3_cfg_vf_vlan(nic_io, HINIC3_CMD_OP_ADD, vlan, qos, vf_id);
	if (err != 0)
		return err;

	nic_io->vf_infos[HW_VF_ID_TO_OS(vf_id)].pf_vlan = vlan;
	nic_io->vf_infos[HW_VF_ID_TO_OS(vf_id)].pf_qos = qos;

	nic_info(nic_io->dev_hdl, "Setting VLAN %u, QOS 0x%x on VF %d\n",
		 vlan, qos, HW_VF_ID_TO_OS(vf_id));

	return 0;
}

int hinic3_kill_vf_vlan(void *hwdev, int vf_id)
{
	struct vf_data_storage *vf_infos = NULL;
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	vf_infos = nic_io->vf_infos;

	err = hinic3_cfg_vf_vlan(nic_io, HINIC3_CMD_OP_DEL,
				 vf_infos[HW_VF_ID_TO_OS(vf_id)].pf_vlan,
				 vf_infos[HW_VF_ID_TO_OS(vf_id)].pf_qos, vf_id);
	if (err != 0)
		return err;

	nic_info(nic_io->dev_hdl, "Remove VLAN %u on VF %d\n",
		 vf_infos[HW_VF_ID_TO_OS(vf_id)].pf_vlan,
		 HW_VF_ID_TO_OS(vf_id));

	vf_infos[HW_VF_ID_TO_OS(vf_id)].pf_vlan = 0;
	vf_infos[HW_VF_ID_TO_OS(vf_id)].pf_qos = 0;

	return 0;
}

u16 hinic3_vf_info_vlanprio(void *hwdev, int vf_id)
{
	struct hinic3_nic_io *nic_io = NULL;
	u16 pf_vlan, vlanprio;
	u8 pf_qos;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return 0;

	pf_vlan = nic_io->vf_infos[HW_VF_ID_TO_OS(vf_id)].pf_vlan;
	pf_qos = nic_io->vf_infos[HW_VF_ID_TO_OS(vf_id)].pf_qos;
	vlanprio = (u16)(pf_vlan | (pf_qos << HINIC3_VLAN_PRIORITY_SHIFT));

	return vlanprio;
}

int hinic3_set_vf_link_state(void *hwdev, u16 vf_id, int link)
{
	struct hinic3_nic_io *nic_io =
		hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	struct vf_data_storage *vf_infos = NULL;
	u8 link_status = 0;

	if (!nic_io)
		return -EINVAL;

	vf_infos = nic_io->vf_infos;

	switch (link) {
	case HINIC3_IFLA_VF_LINK_STATE_AUTO:
		vf_infos[HW_VF_ID_TO_OS(vf_id)].link_forced = false;
		vf_infos[HW_VF_ID_TO_OS(vf_id)].link_up = nic_io->link_status ?
			true : false;
		link_status = nic_io->link_status;
		break;
	case HINIC3_IFLA_VF_LINK_STATE_ENABLE:
		vf_infos[HW_VF_ID_TO_OS(vf_id)].link_forced = true;
		vf_infos[HW_VF_ID_TO_OS(vf_id)].link_up = true;
		link_status = HINIC3_LINK_UP;
		break;
	case HINIC3_IFLA_VF_LINK_STATE_DISABLE:
		vf_infos[HW_VF_ID_TO_OS(vf_id)].link_forced = true;
		vf_infos[HW_VF_ID_TO_OS(vf_id)].link_up = false;
		link_status = HINIC3_LINK_DOWN;
		break;
	default:
		return -EINVAL;
	}

	/* Notify the VF of its new link state */
	hinic3_notify_vf_link_status(nic_io, vf_id, link_status);

	return 0;
}

int hinic3_set_vf_spoofchk(void *hwdev, u16 vf_id, bool spoofchk)
{
	struct hinic3_cmd_spoofchk_set spoofchk_cfg;
	struct vf_data_storage *vf_infos = NULL;
	u16 out_size = sizeof(spoofchk_cfg);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	vf_infos = nic_io->vf_infos;

	memset(&spoofchk_cfg, 0, sizeof(spoofchk_cfg));

	spoofchk_cfg.func_id = hinic3_glb_pf_vf_offset(hwdev) + vf_id;
	spoofchk_cfg.state = spoofchk ? 1 : 0;
	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_SET_SPOOPCHK_STATE,
				     &spoofchk_cfg,
				     sizeof(spoofchk_cfg), &spoofchk_cfg,
				     &out_size);
	if (err || !out_size || spoofchk_cfg.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to set VF(%d) spoofchk, err: %d, status: 0x%x, out size: 0x%x\n",
			HW_VF_ID_TO_OS(vf_id), err,
			spoofchk_cfg.msg_head.status, out_size);
		err = -EINVAL;
	}

	vf_infos[HW_VF_ID_TO_OS(vf_id)].spoofchk = spoofchk;

	return err;
}

bool hinic3_vf_info_spoofchk(void *hwdev, int vf_id)
{
	struct hinic3_nic_io *nic_io = NULL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return false;

	return nic_io->vf_infos[HW_VF_ID_TO_OS(vf_id)].spoofchk;
}

#ifdef HAVE_NDO_SET_VF_TRUST
int hinic3_set_vf_trust(void *hwdev, u16 vf_id, bool trust)
{
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io || vf_id > nic_io->max_vfs)
		return -EINVAL;

	nic_io->vf_infos[HW_VF_ID_TO_OS(vf_id)].trust = trust;

	return 0;
}

bool hinic3_get_vf_trust(void *hwdev, int vf_id)
{
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev)
		return false;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io || vf_id > nic_io->max_vfs)
		return false;

	return nic_io->vf_infos[HW_VF_ID_TO_OS(vf_id)].trust;
}
#endif

static int hinic3_set_vf_tx_rate_max_min(struct hinic3_nic_io *nic_io,
					 u16 vf_id, u32 max_rate, u32 min_rate)
{
	struct hinic3_cmd_tx_rate_cfg rate_cfg;
	u16 out_size = sizeof(rate_cfg);
	int err;

	memset(&rate_cfg, 0, sizeof(rate_cfg));

	rate_cfg.func_id = hinic3_glb_pf_vf_offset(nic_io->hwdev) + vf_id;
	rate_cfg.max_rate = max_rate;
	rate_cfg.min_rate = min_rate;
	rate_cfg.direct = HINIC3_NIC_TX;
	err = l2nic_msg_to_mgmt_sync(nic_io->hwdev,
				     HINIC3_NIC_CMD_SET_MAX_MIN_RATE,
				     &rate_cfg, sizeof(rate_cfg), &rate_cfg,
				     &out_size);
	if (rate_cfg.msg_head.status || err || !out_size) {
		nic_err(nic_io->dev_hdl, "Failed to set VF %d max rate %u, min rate %u, err: %d, status: 0x%x, out size: 0x%x\n",
			HW_VF_ID_TO_OS(vf_id), max_rate, min_rate, err,
			rate_cfg.msg_head.status, out_size);
		return -EIO;
	}

	return 0;
}

int hinic3_set_vf_tx_rate(void *hwdev, u16 vf_id, u32 max_rate, u32 min_rate)
{
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	if (!HINIC3_SUPPORT_RATE_LIMIT(hwdev)) {
		nic_err(nic_io->dev_hdl, "Current function doesn't support to set vf rate limit\n");
		return -EOPNOTSUPP;
	}

	err = hinic3_set_vf_tx_rate_max_min(nic_io, vf_id, max_rate, min_rate);
	if (err != 0)
		return err;

	nic_io->vf_infos[HW_VF_ID_TO_OS(vf_id)].max_rate = max_rate;
	nic_io->vf_infos[HW_VF_ID_TO_OS(vf_id)].min_rate = min_rate;

	return 0;
}

void hinic3_get_vf_config(void *hwdev, u16 vf_id, struct ifla_vf_info *ivi)
{
	struct vf_data_storage *vfinfo = NULL;
	struct hinic3_nic_io *nic_io = NULL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return;

	vfinfo = nic_io->vf_infos + HW_VF_ID_TO_OS(vf_id);
	if (!vfinfo)
		return;

	ivi->vf = HW_VF_ID_TO_OS(vf_id);
	ether_addr_copy(ivi->mac, vfinfo->user_mac_addr);
	ivi->vlan = vfinfo->pf_vlan;
	ivi->qos = vfinfo->pf_qos;

#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
	ivi->spoofchk = vfinfo->spoofchk;
#endif

#ifdef HAVE_NDO_SET_VF_TRUST
	ivi->trusted = vfinfo->trust;
#endif

#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	ivi->max_tx_rate = vfinfo->max_rate;
	ivi->min_tx_rate = vfinfo->min_rate;
#else
	ivi->tx_rate = vfinfo->max_rate;
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */

#ifdef HAVE_NDO_SET_VF_LINK_STATE
	if (!vfinfo->link_forced)
		ivi->linkstate = IFLA_VF_LINK_STATE_AUTO;
	else if (vfinfo->link_up)
		ivi->linkstate = IFLA_VF_LINK_STATE_ENABLE;
	else
		ivi->linkstate = IFLA_VF_LINK_STATE_DISABLE;
#endif
}

static int hinic3_init_vf_infos(struct hinic3_nic_io *nic_io, u16 vf_id)
{
	struct vf_data_storage *vf_infos = nic_io->vf_infos;
	u8 vf_link_state;

	if (set_vf_link_state > HINIC3_IFLA_VF_LINK_STATE_DISABLE) {
		nic_warn(nic_io->dev_hdl, "Module Parameter set_vf_link_state value %u is out of range, resetting to %d\n",
			 set_vf_link_state, HINIC3_IFLA_VF_LINK_STATE_AUTO);
		set_vf_link_state = HINIC3_IFLA_VF_LINK_STATE_AUTO;
	}

	vf_link_state = set_vf_link_state;

	switch (vf_link_state) {
	case HINIC3_IFLA_VF_LINK_STATE_AUTO:
		vf_infos[vf_id].link_forced = false;
		break;
	case HINIC3_IFLA_VF_LINK_STATE_ENABLE:
		vf_infos[vf_id].link_forced = true;
		vf_infos[vf_id].link_up = true;
		break;
	case HINIC3_IFLA_VF_LINK_STATE_DISABLE:
		vf_infos[vf_id].link_forced = true;
		vf_infos[vf_id].link_up = false;
		break;
	default:
		nic_err(nic_io->dev_hdl, "Input parameter set_vf_link_state error: %u\n",
			vf_link_state);
		return -EINVAL;
	}

	return 0;
}

static int vf_func_register(struct hinic3_nic_io *nic_io)
{
	struct hinic3_cmd_register_vf register_info;
	u16 out_size = sizeof(register_info);
	int err;

	err = hinic3_register_vf_mbox_cb(nic_io->hwdev, HINIC3_MOD_L2NIC,
					 nic_io->hwdev,
					 hinic3_vf_event_handler);
	if (err != 0)
		return err;

	err = hinic3_register_vf_mbox_cb(nic_io->hwdev, HINIC3_MOD_HILINK,
					 nic_io->hwdev,
					 hinic3_vf_mag_event_handler);
	if (err != 0)
		goto reg_hilink_err;

	memset(&register_info, 0, sizeof(register_info));
	register_info.op_register = 1;
	register_info.support_extra_feature = 0;
	err = hinic3_mbox_to_pf(nic_io->hwdev, HINIC3_MOD_L2NIC,
				HINIC3_NIC_CMD_VF_REGISTER,
				&register_info, sizeof(register_info),
				&register_info, &out_size, 0,
				HINIC3_CHANNEL_NIC);
	if (err || !out_size || register_info.msg_head.status) {
		if (hinic3_is_slave_host(nic_io->hwdev)) {
			nic_warn(nic_io->dev_hdl, "Failed to register VF, err: %d, status: 0x%x, out size: 0x%x\n",
				 err, register_info.msg_head.status, out_size);
			return 0;
		}
		nic_err(nic_io->dev_hdl, "Failed to register VF, err: %d, status: 0x%x, out size: 0x%x\n",
			err, register_info.msg_head.status, out_size);
		err = -EIO;
		goto register_err;
	}

	return 0;

register_err:
	hinic3_unregister_vf_mbox_cb(nic_io->hwdev, HINIC3_MOD_HILINK);

reg_hilink_err:
	hinic3_unregister_vf_mbox_cb(nic_io->hwdev, HINIC3_MOD_L2NIC);

	return err;
}

static int pf_init_vf_infos(struct hinic3_nic_io *nic_io)
{
	u32 size;
	int err;
	u16 i;

	nic_io->max_vfs = hinic3_func_max_vf(nic_io->hwdev);
	size = sizeof(*nic_io->vf_infos) * nic_io->max_vfs;
	if (!size)
		return 0;

	nic_io->vf_infos = kzalloc(size, GFP_KERNEL);
	if (!nic_io->vf_infos)
		return -ENOMEM;

	for (i = 0; i < nic_io->max_vfs; i++) {
		err = hinic3_init_vf_infos(nic_io, i);
		if (err != 0)
			goto init_vf_infos_err;
	}

	err = hinic3_register_pf_mbox_cb(nic_io->hwdev, HINIC3_MOD_L2NIC,
					 nic_io->hwdev,
					 hinic3_pf_mbox_handler);
	if (err != 0)
		goto register_pf_mbox_cb_err;

	err = hinic3_register_pf_mbox_cb(nic_io->hwdev, HINIC3_MOD_HILINK,
					 nic_io->hwdev,
					 hinic3_pf_mag_mbox_handler);
	if (err != 0)
		goto register_pf_mag_mbox_cb_err;

	return 0;

register_pf_mag_mbox_cb_err:
	hinic3_unregister_pf_mbox_cb(nic_io->hwdev, HINIC3_MOD_L2NIC);
register_pf_mbox_cb_err:
init_vf_infos_err:
	kfree(nic_io->vf_infos);

	return err;
}

int hinic3_vf_func_init(struct hinic3_nic_io *nic_io)
{
	int err;

	if (hinic3_func_type(nic_io->hwdev) == TYPE_VF)
		return vf_func_register(nic_io);

	err = hinic3_register_mgmt_msg_cb(nic_io->hwdev, HINIC3_MOD_L2NIC,
					  nic_io->hwdev,
					  hinic3_pf_event_handler);
	if (err != 0)
		return err;

	err = hinic3_register_mgmt_msg_cb(nic_io->hwdev, HINIC3_MOD_HILINK,
					  nic_io->hwdev,
					  hinic3_pf_mag_event_handler);
	if (err != 0)
		goto register_mgmt_msg_cb_err;

	err = pf_init_vf_infos(nic_io);
	if (err != 0)
		goto pf_init_vf_infos_err;

	return 0;

pf_init_vf_infos_err:
	hinic3_unregister_mgmt_msg_cb(nic_io->hwdev, HINIC3_MOD_HILINK);
register_mgmt_msg_cb_err:
	hinic3_unregister_mgmt_msg_cb(nic_io->hwdev, HINIC3_MOD_L2NIC);

	return err;
}

void hinic3_vf_func_free(struct hinic3_nic_io *nic_io)
{
	struct hinic3_cmd_register_vf unregister;
	u16 out_size = sizeof(unregister);
	int err;

	memset(&unregister, 0, sizeof(unregister));
	unregister.op_register = 0;
	if (hinic3_func_type(nic_io->hwdev) == TYPE_VF) {
		err = hinic3_mbox_to_pf(nic_io->hwdev, HINIC3_MOD_L2NIC,
					HINIC3_NIC_CMD_VF_REGISTER,
					&unregister, sizeof(unregister),
					&unregister, &out_size, 0,
					HINIC3_CHANNEL_NIC);
		if (err || !out_size || unregister.msg_head.status) {
			if (hinic3_is_slave_host(nic_io->hwdev)) {
				nic_info(nic_io->dev_hdl, "vRoCE VF notify PF unsuccessful is allowed");
			} else {
				nic_err(nic_io->dev_hdl, "Failed to unregister VF, err: %d, status: 0x%x, out_size: 0x%x\n",
					err, unregister.msg_head.status, out_size);
			}
		}
		hinic3_unregister_vf_mbox_cb(nic_io->hwdev, HINIC3_MOD_L2NIC);
	} else {
		if (nic_io->vf_infos) {
			hinic3_unregister_pf_mbox_cb(nic_io->hwdev,
						     HINIC3_MOD_HILINK);
			hinic3_unregister_pf_mbox_cb(nic_io->hwdev,
						     HINIC3_MOD_L2NIC);
			hinic3_clear_vfs_info(nic_io->hwdev);
			kfree(nic_io->vf_infos);
			nic_io->vf_infos = NULL;
		}
		hinic3_unregister_mgmt_msg_cb(nic_io->hwdev, HINIC3_MOD_HILINK);
		hinic3_unregister_mgmt_msg_cb(nic_io->hwdev, HINIC3_MOD_L2NIC);
	}
}

static void clear_vf_infos(void *hwdev, u16 vf_id)
{
	struct vf_data_storage *vf_infos = NULL;
	struct hinic3_nic_io *nic_io = NULL;
	u16 func_id;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is NULL\n");
		return;
	}

	func_id = hinic3_glb_pf_vf_offset(hwdev) + vf_id;
	vf_infos = nic_io->vf_infos + HW_VF_ID_TO_OS(vf_id);
	if (vf_infos->use_specified_mac)
		hinic3_del_mac(hwdev, vf_infos->drv_mac_addr,
			       vf_infos->pf_vlan, func_id, HINIC3_CHANNEL_NIC);

	if (hinic3_vf_info_vlanprio(hwdev, vf_id))
		hinic3_kill_vf_vlan(hwdev, vf_id);

	if (vf_infos->max_rate)
		hinic3_set_vf_tx_rate(hwdev, vf_id, 0, 0);

	if (vf_infos->spoofchk)
		hinic3_set_vf_spoofchk(hwdev, vf_id, false);

#ifdef HAVE_NDO_SET_VF_TRUST
	if (vf_infos->trust)
		hinic3_set_vf_trust(hwdev, vf_id, false);
#endif

	memset(vf_infos, 0, sizeof(*vf_infos));
	/* set vf_infos to default */
	hinic3_init_vf_infos(nic_io, HW_VF_ID_TO_OS(vf_id));
}

void hinic3_clear_vfs_info(void *hwdev)
{
	u16 i;
	struct hinic3_nic_io *nic_io =
			hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);

	if (!nic_io) {
		pr_err("nic_io is NULL\n");
		return;
	}

	for (i = 0; i < nic_io->max_vfs; i++)
		clear_vf_infos(hwdev, OS_VF_ID_TO_HW(i));
}

int hinic3_notify_vf_outband_cfg(void *hwdev, u16 func_id, u16 vlan_id)
{
	int err = 0;
	struct hinic3_outband_cfg_info outband_cfg_info;
	struct vf_data_storage *vf_infos = NULL;
	u16 out_size = sizeof(outband_cfg_info);
	u16 vf_id;
	struct hinic3_nic_io *nic_io =
			hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is NULL\n");
		return 0;
	}

	vf_id = func_id - hinic3_glb_pf_vf_offset(nic_io->hwdev);
	vf_infos = nic_io->vf_infos + HW_VF_ID_TO_OS(vf_id);

	memset(&outband_cfg_info, 0, sizeof(outband_cfg_info));
	if (vf_infos->registered) {
		outband_cfg_info.func_id = func_id;
		outband_cfg_info.outband_default_vid = vlan_id;
		err = hinic3_mbox_to_vf_no_ack(nic_io->hwdev, vf_id,
					       HINIC3_MOD_L2NIC,
					       HINIC3_NIC_CMD_OUTBAND_CFG_NOTICE,
					       &outband_cfg_info,
					       sizeof(outband_cfg_info),
					       &outband_cfg_info, &out_size,
					       HINIC3_CHANNEL_NIC);
		if (err == MBOX_ERRCODE_UNKNOWN_DES_FUNC) {
			nic_warn(nic_io->dev_hdl, "VF%d not initialized, disconnect it\n",
				 HW_VF_ID_TO_OS(vf_id));
			hinic3_unregister_vf(nic_io, vf_id);
			return 0;
		}
		if (err || !out_size || outband_cfg_info.msg_head.status)
			nic_err(nic_io->dev_hdl,
				"outband cfg event to VF %d failed, err: %d, status: 0x%x, out_size: 0x%x\n",
				HW_VF_ID_TO_OS(vf_id), err,
				outband_cfg_info.msg_head.status, out_size);
	}

	return err;
}

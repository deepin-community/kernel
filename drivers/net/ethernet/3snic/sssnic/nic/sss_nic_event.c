// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

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

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_io.h"
#include "sss_nic_cfg.h"
#include "sss_nic_vf_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_rss_cfg.h"
#include "sss_nic_io_define.h"
#include "sss_nic_cfg_define.h"
#include "sss_nic_event.h"

#define SSSNIC_VF_UNREGISTER	0

static void sss_nic_dcb_state_event_handler(struct sss_nic_io *nic_io,
					    void *in_buf, u16 in_size,
					    void *out_buf, u16 *out_size);
static void sss_nic_tx_pause_event_handler(struct sss_nic_io *nic_io,
					   void *in_buf, u16 in_size,
					   void *out_buf, u16 *out_size);
static void sss_nic_bond_active_event_handler(struct sss_nic_io *nic_io,
					      void *in_buf, u16 in_size,
					      void *out_buf, u16 *out_size);
static int sss_nic_register_vf_msg_handler(struct sss_nic_io *nic_io,
					   u16 vf_id, void *in_buf, u16 in_size,
					   void *out_buf, u16 *out_size);
static int sss_nic_get_vf_cos_msg_handler(struct sss_nic_io *nic_io,
					  u16 vf_id, void *in_buf, u16 in_size,
					  void *out_buf, u16 *out_size);
static int sss_nic_get_vf_mac_msg_handler(struct sss_nic_io *nic_io,
					  u16 vf_id, void *in_buf, u16 in_size,
					  void *out_buf, u16 *out_size);
static int sss_nic_set_vf_mac_msg_handler(struct sss_nic_io *nic_io,
					  u16 vf_id, void *in_buf, u16 in_size,
					  void *out_buf, u16 *out_size);
static int sss_nic_del_vf_mac_msg_handler(struct sss_nic_io *nic_io,
					  u16 vf_id, void *in_buf, u16 in_size,
					  void *out_buf, u16 *out_size);
static int sss_nic_update_vf_mac_msg_handler(struct sss_nic_io *nic_io,
					     u16 vf_id, void *in_buf, u16 in_size,
					     void *out_buf, u16 *out_size);

static const struct nic_event_handler g_event_proc[] = {
	{
		.opcode = SSSNIC_MBX_OPCODE_GET_VF_COS,
		.event_handler = sss_nic_dcb_state_event_handler,
	},

	{
		.opcode = SSSNIC_MBX_OPCODE_TX_PAUSE_EXCP_NOTICE,
		.event_handler = sss_nic_tx_pause_event_handler,
	},

	{
		.opcode = SSSNIC_MBX_OPCODE_BOND_ACTIVE_NOTICE,
		.event_handler = sss_nic_bond_active_event_handler,
	},
};

static const struct sss_nic_vf_msg_handler g_vf_cmd_proc[] = {
	{
		.opcode = SSSNIC_MBX_OPCODE_VF_REGISTER,
		.msg_handler = sss_nic_register_vf_msg_handler,
	},

	{
		.opcode = SSSNIC_MBX_OPCODE_GET_VF_COS,
		.msg_handler = sss_nic_get_vf_cos_msg_handler
	},

	{
		.opcode = SSSNIC_MBX_OPCODE_GET_MAC,
		.msg_handler = sss_nic_get_vf_mac_msg_handler,
	},

	{
		.opcode = SSSNIC_MBX_OPCODE_SET_MAC,
		.msg_handler = sss_nic_set_vf_mac_msg_handler,
	},

	{
		.opcode = SSSNIC_MBX_OPCODE_DEL_MAC,
		.msg_handler = sss_nic_del_vf_mac_msg_handler,
	},

	{
		.opcode = SSSNIC_MBX_OPCODE_UPDATE_MAC,
		.msg_handler = sss_nic_update_vf_mac_msg_handler,
	},
};

static const struct nic_event_handler *sss_nic_get_event_proc(u16 opcode)
{
	u16 i;
	u16 cmd_num = ARRAY_LEN(g_event_proc);

	for (i = 0; i < cmd_num; i++)
		if (g_event_proc[i].opcode == opcode)
			return &g_event_proc[i];

	return NULL;
}

static const struct sss_nic_vf_msg_handler *sss_nic_get_vf_cmd_proc(u16 opcode)
{
	u16 i;
	u16 cmd_num = ARRAY_LEN(g_vf_cmd_proc);

	for (i = 0; i < cmd_num; i++)
		if (g_vf_cmd_proc[i].opcode == opcode)
			return &g_vf_cmd_proc[i];

	return NULL;
}

static int sss_nic_init_vf_config(struct sss_nic_io *nic_io, u16 vf_id)
{
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	struct sss_nic_vf_info *vf_info = &nic_io->vf_info_group[id];
	u16 func_id;
	int ret;

	vf_info->specified_mac = false;
	ether_addr_copy(vf_info->drv_mac, vf_info->user_mac);

	if (!is_zero_ether_addr(vf_info->drv_mac)) {
		vf_info->specified_mac = true;
		func_id = sss_get_glb_pf_vf_offset(nic_io->hwdev) + vf_id;
		ret = sss_nic_set_mac(nic_io->nic_dev, vf_info->drv_mac,
				      vf_info->pf_vlan, func_id, SSS_CHANNEL_NIC);
		if (ret != 0) {
			nic_err(nic_io->dev_hdl, "Fail to set VF %d MAC, ret: %d\n", id, ret);
			return ret;
		}
	}

	if (SSSNIC_GET_VLAN_PRIO(vf_info->pf_vlan, vf_info->pf_qos) != 0) {
		ret = sss_nic_set_vf_vlan(nic_io, SSSNIC_MBX_OPCODE_ADD,
					  vf_info->pf_vlan, vf_info->pf_qos, vf_id);
		if (ret != 0) {
			nic_err(nic_io->dev_hdl, "Fail to add VF %d VLAN_QOS, ret: %d\n", id, ret);
			return ret;
		}
	}

	if (vf_info->max_rate != 0) {
		ret = sss_nic_set_vf_tx_rate_limit(nic_io, vf_id,
						   vf_info->min_rate, vf_info->max_rate);
		if (ret != 0) {
			nic_err(nic_io->dev_hdl,
				"Fail to set VF %d max rate %u, min rate %u, ret: %d\n",
				id, vf_info->max_rate, vf_info->min_rate, ret);
			return ret;
		}
	}

	return 0;
}

static int sss_nic_attach_vf(struct sss_nic_io *nic_io, u16 vf_id, u32 extra_feature)
{
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	struct sss_nic_vf_info *vf_info = &nic_io->vf_info_group[id];
	int ret;

	vf_info->extra_feature = extra_feature;

	if (vf_id > nic_io->max_vf_num) {
		nic_err(nic_io->dev_hdl, "Fail to register VF id %d out of range: [0-%d]\n",
			SSSNIC_HW_VF_ID_TO_OS(vf_id), SSSNIC_HW_VF_ID_TO_OS(nic_io->max_vf_num));
		return -EFAULT;
	}

	ret = sss_nic_init_vf_config(nic_io, vf_id);
	if (ret != 0)
		return ret;

	vf_info->attach = true;

	return 0;
}

int sss_nic_dettach_vf(struct sss_nic_io *nic_io, u16 vf_id)
{
	struct sss_nic_mbx_mac_addr cmd_set_mac = {0};
	struct sss_nic_vf_info *vf_info = &nic_io->vf_info_group[SSSNIC_HW_VF_ID_TO_OS(vf_id)];
	u16 out_len;
	int ret;

	vf_info->extra_feature = 0;

	if (vf_id > nic_io->max_vf_num) {
		nic_err(nic_io->dev_hdl, "Invalid vf_id %d, max_vf_num: %d\n",
			vf_id, nic_io->max_vf_num);
		return -EFAULT;
	}

	vf_info->attach = false;

	if (!vf_info->specified_mac && vf_info->pf_vlan == 0) {
		memset(vf_info->drv_mac, 0, ETH_ALEN);
		return 0;
	}

	out_len = sizeof(cmd_set_mac);
	ether_addr_copy(cmd_set_mac.mac, vf_info->drv_mac);
	cmd_set_mac.vlan_id = vf_info->pf_vlan;
	cmd_set_mac.func_id = sss_get_glb_pf_vf_offset(nic_io->hwdev) + (u16)vf_id;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev, SSSNIC_MBX_OPCODE_DEL_MAC,
					     &cmd_set_mac, sizeof(cmd_set_mac),
					     &cmd_set_mac, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_set_mac)) {
		nic_err(nic_io->dev_hdl,
			"Fail to delete the mac of VF %d, ret: %d, status: 0x%x, out_len: 0x%x\n",
			SSSNIC_HW_VF_ID_TO_OS(vf_id), ret,
			cmd_set_mac.head.state, out_len);
		return -EFAULT;
	}

	memset(vf_info->drv_mac, 0, ETH_ALEN);

	return 0;
}

static int sss_nic_register_vf_msg_handler(struct sss_nic_io *nic_io,
					   u16 vf_id, void *in_buf, u16 in_size,
					   void *out_buf, u16 *out_size)
{
	int ret;
	struct sss_nic_mbx_attach_vf *in_info = in_buf;
	struct sss_nic_mbx_attach_vf *out_info = out_buf;

	if (in_info->op_register == SSSNIC_VF_UNREGISTER)
		ret = sss_nic_dettach_vf(nic_io, vf_id);
	else
		ret = sss_nic_attach_vf(nic_io, vf_id, in_info->extra_feature);

	*out_size = sizeof(*out_info);
	if (ret != 0)
		out_info->head.state = EFAULT;

	return 0;
}

static int sss_nic_get_vf_cos_msg_handler(struct sss_nic_io *nic_io, u16 vf_id,
					  void *in_buf, u16 in_size, void *out_buf,
					  u16 *out_size)
{
	struct sss_nic_mbx_vf_dcb_cfg *out_state = out_buf;

	*out_size = sizeof(*out_state);
	out_state->head.state = SSS_MGMT_CMD_SUCCESS;
	memcpy(&out_state->dcb_info, &nic_io->dcb_info, sizeof(nic_io->dcb_info));

	return 0;
}

static int sss_nic_get_vf_mac_msg_handler(struct sss_nic_io *nic_io, u16 vf_id,
					  void *in_buf, u16 in_size, void *out_buf, u16 *out_size)
{
	struct sss_nic_vf_info *vf_info = &nic_io->vf_info_group[SSSNIC_HW_VF_ID_TO_OS(vf_id)];
	struct sss_nic_mbx_mac_addr *out_info = out_buf;
	int ret;

	if (SSSNIC_SUPPORT_VF_MAC(nic_io)) {
		ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev, SSSNIC_MBX_OPCODE_GET_MAC,
						     in_buf, in_size, out_buf, out_size);
		if (ret == 0) {
			if (is_zero_ether_addr(out_info->mac))
				ether_addr_copy(out_info->mac, vf_info->drv_mac);
		}
		return ret;
	}

	*out_size = sizeof(*out_info);
	ether_addr_copy(out_info->mac, vf_info->drv_mac);
	out_info->head.state = SSS_MGMT_CMD_SUCCESS;

	return 0;
}

static int sss_nic_cmd_vf_mac(struct sss_nic_io *nic_io, struct sss_nic_vf_info *vf_info,
			      u16 cmd, void *in_buf, u16 in_size, void *out_buf, u16 *out_size)
{
	struct sss_nic_mbx_mac_addr *in_mac = in_buf;
	struct sss_nic_mbx_mac_addr *out_mac = out_buf;
	int ret;

	if (!vf_info->trust && vf_info->specified_mac && is_valid_ether_addr(in_mac->mac)) {
		out_mac->head.state = SSSNIC_PF_SET_VF_ALREADY;
		*out_size = sizeof(*out_mac);
		nic_warn(nic_io->dev_hdl,
			 "PF has already set VF MAC address,and vf trust is off.\n");
		return 0;
	}
	if (is_valid_ether_addr(in_mac->mac))
		in_mac->vlan_id = vf_info->pf_vlan;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev, cmd, in_buf, in_size,
					     out_buf, out_size);
	if (ret != 0 || *out_size == 0) {
		nic_warn(nic_io->dev_hdl,
			 "Fail to send vf mac, ret: %d,status: 0x%x, out size: 0x%x\n",
			 ret, out_mac->head.state, *out_size);
		return -EFAULT;
	}

	return 0;
}

static int sss_nic_set_vf_mac_msg_handler(struct sss_nic_io *nic_io,
					  u16 vf_id, void *in_buf, u16 in_size,
					  void *out_buf, u16 *out_size)
{
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	struct sss_nic_vf_info *vf_info = &nic_io->vf_info_group[id];
	struct sss_nic_mbx_mac_addr *in_mac = in_buf;
	struct sss_nic_mbx_mac_addr *out_mac = out_buf;
	int ret;

	ret = sss_nic_cmd_vf_mac(nic_io, vf_info, SSSNIC_MBX_OPCODE_SET_MAC,
				 in_buf, in_size, out_buf, out_size);
	if (ret != 0)
		return ret;

	if (is_valid_ether_addr(in_mac->mac) &&
	    out_mac->head.state == SSS_MGMT_CMD_SUCCESS)
		ether_addr_copy(vf_info->drv_mac, in_mac->mac);

	return 0;
}

static int sss_nic_del_vf_mac_msg_handler(struct sss_nic_io *nic_io,
					  u16 vf_id, void *in_buf, u16 in_size,
					  void *out_buf, u16 *out_size)
{
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	struct sss_nic_vf_info *vf_info = &nic_io->vf_info_group[id];
	struct sss_nic_mbx_mac_addr *in_mac = in_buf;
	struct sss_nic_mbx_mac_addr *out_mac = out_buf;
	int ret;

	ret = sss_nic_cmd_vf_mac(nic_io, vf_info, SSSNIC_MBX_OPCODE_DEL_MAC,
				 in_buf, in_size, out_buf, out_size);
	if (ret != 0)
		return ret;

	if (is_valid_ether_addr(in_mac->mac) &&
	    out_mac->head.state == SSS_MGMT_CMD_SUCCESS)
		eth_zero_addr(vf_info->drv_mac);

	return 0;
}

static int sss_nic_update_vf_mac_msg_handler(struct sss_nic_io *nic_io,
					     u16 vf_id, void *in_buf, u16 in_size,
					     void *out_buf, u16 *out_size)
{
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	struct sss_nic_vf_info *vf_info = &nic_io->vf_info_group[id];
	struct sss_nic_mbx_mac_update *in_mac = in_buf;
	struct sss_nic_mbx_mac_update *out_mac = out_buf;
	int ret;

	if (!is_valid_ether_addr(in_mac->old_mac.mac)) {
		nic_err(nic_io->dev_hdl, "Fail to update mac, Invalid mac.\n");
		return -EINVAL;
	}

	if (!vf_info->trust && vf_info->specified_mac) {
		out_mac->old_mac.head.state = SSSNIC_PF_SET_VF_ALREADY;
		*out_size = sizeof(*out_mac);
		nic_warn(nic_io->dev_hdl,
			 "PF has already set VF MAC address,and vf trust is off.\n");
		return 0;
	}

	in_mac->old_mac.vlan_id = vf_info->pf_vlan;
	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev,
					     SSSNIC_MBX_OPCODE_UPDATE_MAC, in_buf, in_size,
					     out_buf, out_size);
	if (ret != 0 || *out_size == 0) {
		nic_warn(nic_io->dev_hdl,
			 "Fail to update vf mac, ret: %d,status: 0x%x, out size: 0x%x\n",
			 ret, out_mac->old_mac.head.state, *out_size);
		return -EFAULT;
	}

	if (out_mac->old_mac.head.state == SSS_MGMT_CMD_SUCCESS)
		ether_addr_copy(vf_info->drv_mac, in_mac->new_mac);

	return 0;
}

static int _sss_nic_l2nic_msg_to_mgmt_sync(void *hwdev, u16 cmd, void *in_buf,
					   u16 in_size, void *out_buf,
					   u16 *out_size, u16 channel)
{
	if (sss_get_func_type(hwdev) == SSS_FUNC_TYPE_VF)
		if (sss_nic_get_vf_cmd_proc(cmd))
			return sss_mbx_send_to_pf(hwdev, SSS_MOD_TYPE_L2NIC, cmd, in_buf,
						  in_size, out_buf, out_size, 0, channel);

	return sss_sync_mbx_send_msg(hwdev, SSS_MOD_TYPE_L2NIC, cmd, in_buf,
				     in_size, out_buf, out_size, 0, channel);
}

int sss_nic_l2nic_msg_to_mgmt_sync(void *hwdev, u16 cmd, void *in_buf, u16 in_size,
				   void *out_buf, u16 *out_size)
{
	return _sss_nic_l2nic_msg_to_mgmt_sync(hwdev, cmd, in_buf, in_size, out_buf,
					       out_size, SSS_CHANNEL_NIC);
}

int sss_nic_l2nic_msg_to_mgmt_sync_ch(void *hwdev, u16 cmd, void *in_buf, u16 in_size,
				      void *out_buf, u16 *out_size, u16 channel)
{
	return _sss_nic_l2nic_msg_to_mgmt_sync(hwdev, cmd, in_buf, in_size, out_buf,
					       out_size, channel);
}

/* pf/ppf handler mbx msg from vf */
int sss_nic_pf_mbx_handler(void *hwdev, u16 vf_id, u16 cmd, void *in_buf, u16 in_size,
			   void *out_buf, u16 *out_size)
{
	struct sss_nic_io *nic_io = NULL;
	const struct sss_nic_vf_msg_handler *handler = NULL;

	if (!hwdev)
		return -EFAULT;

	nic_io = sss_get_service_adapter(hwdev, SSS_SERVICE_TYPE_NIC);
	if (!nic_io)
		return -EINVAL;

	handler = sss_nic_get_vf_cmd_proc(cmd);
	if (handler)
		return handler->msg_handler(nic_io, vf_id, in_buf, in_size, out_buf, out_size);

	nic_warn(nic_io->dev_hdl, "NO handler for nic cmd(%u) received from vf id: %u\n",
		 cmd, vf_id);

	return -EINVAL;
}

void sss_nic_notify_dcb_state_event(void *hwdev,
				    struct sss_nic_dcb_info *dcb_info)
{
	struct sss_event_info event_info = {0};

	event_info.type = SSSNIC_EVENT_DCB_STATE_CHANGE;
	event_info.service = SSS_EVENT_SRV_NIC;
	memcpy((void *)event_info.event_data, dcb_info, sizeof(*dcb_info));

	sss_do_event_callback(hwdev, &event_info);
}

static void sss_nic_dcb_state_event_handler(struct sss_nic_io *nic_io,
					    void *in_buf, u16 in_size,
					    void *out_buf, u16 *out_size)
{
	struct sss_nic_mbx_vf_dcb_cfg *dcb_cfg = in_buf;

	if (!dcb_cfg)
		return;

	memcpy(&nic_io->dcb_info, &dcb_cfg->dcb_info, sizeof(dcb_cfg->dcb_info));
	sss_nic_notify_dcb_state_event(nic_io->hwdev, &dcb_cfg->dcb_info);
}

static void sss_nic_tx_pause_event_handler(struct sss_nic_io *nic_io,
					   void *in_buf, u16 in_size, void *out_buf, u16 *out_size)
{
	struct sss_nic_msg_tx_pause_info *in_pause = in_buf;

	if (in_size != sizeof(*in_pause)) {
		nic_err(nic_io->dev_hdl, "Invalid in buffer size value: %u,It should be %ld\n",
			in_size, sizeof(*in_pause));
		return;
	}

	nic_warn(nic_io->dev_hdl, "Receive tx pause exception event, excp: %u, level: %u\n",
		 in_pause->tx_pause_except, in_pause->except_level);
	sss_fault_event_report(nic_io->hwdev, SSS_FAULT_SRC_TX_PAUSE_EXCP,
			       (u16)in_pause->except_level);
}

static void sss_nic_bond_active_event_handler(struct sss_nic_io *nic_io,
					      void *in_buf, u16 in_size,
					      void *out_buf, u16 *out_size)
{
	struct sss_event_info in_info = {0};
	struct sss_nic_msg_bond_active_info *bond_info = in_buf;

	if (in_size != sizeof(*bond_info)) {
		nic_err(nic_io->dev_hdl, "Invalid in_size: %u, should be %ld\n",
			in_size, sizeof(*bond_info));
		return;
	}

	memcpy((void *)in_info.event_data, bond_info, sizeof(*bond_info));
	in_info.type = SSSNIC_MBX_OPCODE_BOND_ACTIVE_NOTICE;
	in_info.service = SSS_EVENT_SRV_NIC;
	sss_do_event_callback(nic_io->hwdev, &in_info);
}

static int _sss_nic_event_handler(void *hwdev, u16 cmd, void *in_buf, u16 in_size,
				  void *out_buf, u16 *out_size)
{
	struct sss_nic_io *nic_io = NULL;
	const struct nic_event_handler *handler = NULL;

	if (!hwdev)
		return -EINVAL;

	nic_io = sss_get_service_adapter(hwdev, SSS_SERVICE_TYPE_NIC);
	if (!nic_io)
		return -EINVAL;

	*out_size = 0;

	handler = sss_nic_get_event_proc(cmd);
	if (handler) {
		handler->event_handler(nic_io, in_buf, in_size, out_buf, out_size);
		return 0;
	}

	((struct sss_mgmt_msg_head *)out_buf)->state = SSS_MGMT_CMD_UNSUPPORTED;
	*out_size = sizeof(struct sss_mgmt_msg_head);
	nic_warn(nic_io->dev_hdl, "Unsupport nic event, cmd: %u\n", cmd);

	return 0;
}

int sss_nic_vf_event_handler(void *hwdev,
			     u16 cmd, void *in_buf, u16 in_size,
			     void *out_buf, u16 *out_size)
{
	return _sss_nic_event_handler(hwdev, cmd, in_buf, in_size, out_buf, out_size);
}

void sss_nic_pf_event_handler(void *hwdev, u16 cmd, void *in_buf, u16 in_size,
			      void *out_buf, u16 *out_size)
{
	_sss_nic_event_handler(hwdev, cmd, in_buf, in_size, out_buf, out_size);
}

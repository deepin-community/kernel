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
#include "nic_mpu_cmd.h"
#include "nic_npu_cmd.h"

static int hinic3_init_vf_config(struct hinic3_nic_io *nic_io, u16 vf_id)
{
	struct vf_data_storage *vf_info = NULL;
	u16 func_id;
	int err = 0;

	vf_info = nic_io->vf_infos + HW_VF_ID_TO_OS(vf_id);
	ether_addr_copy(vf_info->drv_mac_addr, vf_info->user_mac_addr);
	if (!is_zero_ether_addr(vf_info->drv_mac_addr)) {
		vf_info->use_specified_mac = true;
		func_id = hinic3_glb_pf_vf_offset(nic_io->hwdev) + vf_id;

		err = hinic3_set_mac(nic_io->hwdev, vf_info->drv_mac_addr,
				     vf_info->pf_vlan, func_id,
				     HINIC3_CHANNEL_NIC);
		if (err != 0) {
			nic_err(nic_io->dev_hdl, "Failed to set VF %d MAC\n",
				HW_VF_ID_TO_OS(vf_id));
			return err;
		}
	} else {
		vf_info->use_specified_mac = false;
	}

	if (hinic3_vf_info_vlanprio(nic_io->hwdev, vf_id)) {
		err = hinic3_cfg_vf_vlan(nic_io, HINIC3_CMD_OP_ADD,
					 vf_info->pf_vlan, vf_info->pf_qos,
					 vf_id);
		if (err != 0) {
			nic_err(nic_io->dev_hdl, "Failed to add VF %d VLAN_QOS\n",
				HW_VF_ID_TO_OS(vf_id));
			return err;
		}
	}

	if (vf_info->max_rate) {
		err = hinic3_set_vf_tx_rate(nic_io->hwdev, vf_id,
					    vf_info->max_rate,
					    vf_info->min_rate);
		if (err != 0) {
			nic_err(nic_io->dev_hdl, "Failed to set VF %d max rate %u, min rate %u\n",
				HW_VF_ID_TO_OS(vf_id), vf_info->max_rate,
				vf_info->min_rate);
			return err;
		}
	}

	return 0;
}

static int register_vf_msg_handler(struct hinic3_nic_io *nic_io, u16 vf_id)
{
	int err;

	if (vf_id > nic_io->max_vfs) {
		nic_err(nic_io->dev_hdl, "Register VF id %d exceed limit[0-%d]\n",
			HW_VF_ID_TO_OS(vf_id), HW_VF_ID_TO_OS(nic_io->max_vfs));
		return -EFAULT;
	}

	err = hinic3_init_vf_config(nic_io, vf_id);
	if (err != 0)
		return err;

	nic_io->vf_infos[HW_VF_ID_TO_OS(vf_id)].registered = true;

	return 0;
}

static int unregister_vf_msg_handler(struct hinic3_nic_io *nic_io, u16 vf_id)
{
	struct vf_data_storage *vf_info =
		nic_io->vf_infos + HW_VF_ID_TO_OS(vf_id);
	struct hinic3_port_mac_set mac_info;
	u16 out_size = sizeof(mac_info);
	int err;

	if (vf_id > nic_io->max_vfs)
		return -EFAULT;

	vf_info->registered = false;

	memset(&mac_info, 0, sizeof(mac_info));
	mac_info.func_id = hinic3_glb_pf_vf_offset(nic_io->hwdev) + (u16)vf_id;
	mac_info.vlan_id = vf_info->pf_vlan;
	ether_addr_copy(mac_info.mac, vf_info->drv_mac_addr);

	if (vf_info->use_specified_mac || vf_info->pf_vlan) {
		err = l2nic_msg_to_mgmt_sync(nic_io->hwdev,
					     HINIC3_NIC_CMD_DEL_MAC,
					     &mac_info, sizeof(mac_info),
					     &mac_info, &out_size);
		if (err || mac_info.msg_head.status || !out_size) {
			nic_err(nic_io->dev_hdl, "Failed to delete VF %d MAC, err: %d, status: 0x%x, out size: 0x%x\n",
				HW_VF_ID_TO_OS(vf_id), err,
				mac_info.msg_head.status, out_size);
			return -EFAULT;
		}
	}

	memset(vf_info->drv_mac_addr, 0, ETH_ALEN);

	return 0;
}

static int hinic3_register_vf_msg_handler(struct hinic3_nic_io *nic_io,
					  u16 vf_id, void *buf_in, u16 in_size,
					  void *buf_out, u16 *out_size)
{
	struct hinic3_cmd_register_vf *register_vf = buf_in;
	struct hinic3_cmd_register_vf *register_info = buf_out;
	struct vf_data_storage *vf_info = nic_io->vf_infos + HW_VF_ID_TO_OS(vf_id);
	int err;

	if (!vf_info)
		return -EINVAL;

	if (register_vf->op_register) {
		vf_info->support_extra_feature = register_vf->support_extra_feature;
		err = register_vf_msg_handler(nic_io, vf_id);
	} else {
		err = unregister_vf_msg_handler(nic_io, vf_id);
		vf_info->support_extra_feature = 0;
	}

	if (err != 0)
		register_info->msg_head.status = EFAULT;

	*out_size = sizeof(*register_info);

	return 0;
}

void hinic3_unregister_vf(struct hinic3_nic_io *nic_io, u16 vf_id)
{
	struct vf_data_storage *vf_info = nic_io->vf_infos + HW_VF_ID_TO_OS(vf_id);

	if (!vf_info)
		return;
	unregister_vf_msg_handler(nic_io, vf_id);
	vf_info->support_extra_feature = 0;
}

static int hinic3_get_vf_cos_msg_handler(struct hinic3_nic_io *nic_io,
					 u16 vf_id, void *buf_in,
					 u16 in_size, void *buf_out,
					 u16 *out_size)
{
	struct hinic3_cmd_vf_dcb_state *dcb_state = buf_out;

	memcpy(&dcb_state->state, &nic_io->dcb_state,
	       sizeof(nic_io->dcb_state));

	dcb_state->msg_head.status = 0;
	*out_size = sizeof(*dcb_state);
	return 0;
}

static int hinic3_get_vf_mac_msg_handler(struct hinic3_nic_io *nic_io, u16 vf,
					 void *buf_in, u16 in_size,
					 void *buf_out, u16 *out_size)
{
	struct vf_data_storage *vf_info = nic_io->vf_infos + HW_VF_ID_TO_OS(vf);
	struct hinic3_port_mac_set *mac_in =
		(struct hinic3_port_mac_set *)buf_in;
	struct hinic3_port_mac_set *mac_info = buf_out;

	int err;

	if (!mac_info || !vf_info)
		return -EINVAL;

	mac_in->func_id = hinic3_glb_pf_vf_offset(nic_io->hwdev) + vf;

	if (HINIC3_SUPPORT_VF_MAC(nic_io->hwdev)) {
		err = l2nic_msg_to_mgmt_sync(nic_io->hwdev, HINIC3_NIC_CMD_GET_MAC, buf_in,
					     in_size, buf_out, out_size);
		if (err == 0) {
			if (is_zero_ether_addr(mac_info->mac))
				ether_addr_copy(mac_info->mac, vf_info->drv_mac_addr);
		}
		return err;
	}

	ether_addr_copy(mac_info->mac, vf_info->drv_mac_addr);
	mac_info->msg_head.status = 0;
	*out_size = sizeof(*mac_info);

	return 0;
}

static int hinic3_set_vf_mac_msg_handler(struct hinic3_nic_io *nic_io, u16 vf,
					 void *buf_in, u16 in_size,
					 void *buf_out, u16 *out_size)
{
	struct vf_data_storage *vf_info = nic_io->vf_infos + HW_VF_ID_TO_OS(vf);
	struct hinic3_port_mac_set *mac_in = buf_in;
	struct hinic3_port_mac_set *mac_out = buf_out;
	int err;

	if (!vf_info)
		return -EINVAL;

	mac_in->func_id = hinic3_glb_pf_vf_offset(nic_io->hwdev) + vf;

	if (vf_info->use_specified_mac && !vf_info->trust &&
	    is_valid_ether_addr(mac_in->mac)) {
		nic_warn(nic_io->dev_hdl, "PF has already set VF %d MAC address, and vf trust is off.\n",
			 HW_VF_ID_TO_OS(vf));
		mac_out->msg_head.status = HINIC3_PF_SET_VF_ALREADY;
		*out_size = sizeof(*mac_out);
		return 0;
	}

	if (is_valid_ether_addr(mac_in->mac))
		mac_in->vlan_id = vf_info->pf_vlan;

	err = l2nic_msg_to_mgmt_sync(nic_io->hwdev, HINIC3_NIC_CMD_SET_MAC,
				     buf_in, in_size, buf_out, out_size);
	if (err || !(*out_size)) {
		nic_err(nic_io->dev_hdl, "Failed to set VF %d MAC address, err: %d,status: 0x%x, out size: 0x%x\n",
			HW_VF_ID_TO_OS(vf), err, mac_out->msg_head.status,
			*out_size);
		return -EFAULT;
	}

	if (is_valid_ether_addr(mac_in->mac) && !mac_out->msg_head.status)
		ether_addr_copy(vf_info->drv_mac_addr, mac_in->mac);

	return err;
}

static int hinic3_del_vf_mac_msg_handler(struct hinic3_nic_io *nic_io, u16 vf,
					 void *buf_in, u16 in_size,
					 void *buf_out, u16 *out_size)
{
	struct vf_data_storage *vf_info = nic_io->vf_infos + HW_VF_ID_TO_OS(vf);
	struct hinic3_port_mac_set *mac_in = buf_in;
	struct hinic3_port_mac_set *mac_out = buf_out;
	int err;

	if (!vf_info)
		return -EINVAL;
	mac_in->func_id = hinic3_glb_pf_vf_offset(nic_io->hwdev) + vf;

	if (vf_info->use_specified_mac && !vf_info->trust &&
	    is_valid_ether_addr(mac_in->mac)) {
		nic_warn(nic_io->dev_hdl, "PF has already set VF %d MAC address, and vf trust is off.\n",
			 HW_VF_ID_TO_OS(vf));
		mac_out->msg_head.status = HINIC3_PF_SET_VF_ALREADY;
		*out_size = sizeof(*mac_out);
		return 0;
	}

	if (is_valid_ether_addr(mac_in->mac))
		mac_in->vlan_id = vf_info->pf_vlan;

	err = l2nic_msg_to_mgmt_sync(nic_io->hwdev, HINIC3_NIC_CMD_DEL_MAC,
				     buf_in, in_size, buf_out, out_size);
	if (err || !(*out_size)) {
		nic_err(nic_io->dev_hdl, "Failed to delete VF %d MAC, err: %d, status: 0x%x, out size: 0x%x\n",
			HW_VF_ID_TO_OS(vf), err, mac_out->msg_head.status,
			*out_size);
		return -EFAULT;
	}

	if (is_valid_ether_addr(mac_in->mac) && !mac_out->msg_head.status)
		eth_zero_addr(vf_info->drv_mac_addr);

	return err;
}

static int hinic3_update_vf_mac_msg_handler(struct hinic3_nic_io *nic_io,
					    u16 vf, void *buf_in, u16 in_size,
					    void *buf_out, u16 *out_size)
{
	struct vf_data_storage *vf_info = nic_io->vf_infos + HW_VF_ID_TO_OS(vf);
	struct hinic3_port_mac_update *mac_in = buf_in;
	struct hinic3_port_mac_update *mac_out = buf_out;
	int err;

	if (!vf_info)
		return -EINVAL;
	if (!is_valid_ether_addr(mac_in->new_mac)) {
		nic_err(nic_io->dev_hdl, "Update VF MAC is invalid.\n");
		return -EINVAL;
	}
	mac_in->func_id = hinic3_glb_pf_vf_offset(nic_io->hwdev) + vf;

#ifndef __VMWARE__
	if (vf_info->use_specified_mac && !vf_info->trust) {
		nic_warn(nic_io->dev_hdl, "PF has already set VF %d MAC address, and vf trust is off.\n",
			 HW_VF_ID_TO_OS(vf));
		mac_out->msg_head.status = HINIC3_PF_SET_VF_ALREADY;
		*out_size = sizeof(*mac_out);
		return 0;
	}
#else
	err = hinic_config_vf_request(nic_io->hwdev->pcidev_hdl,
				      HW_VF_ID_TO_OS(vf),
				      HINIC_CFG_VF_MAC_CHANGED,
				      (void *)mac_in->new_mac);
	if (err != 0) {
		nic_err(nic_io->dev_hdl, "Failed to config VF %d MAC request, err: %d\n",
			HW_VF_ID_TO_OS(vf), err);
		return err;
	}
#endif
	mac_in->vlan_id = vf_info->pf_vlan;
	err = l2nic_msg_to_mgmt_sync(nic_io->hwdev, HINIC3_NIC_CMD_UPDATE_MAC,
				     buf_in, in_size, buf_out, out_size);
	if (err || !(*out_size)) {
		nic_warn(nic_io->dev_hdl, "Failed to update VF %d MAC, err: %d,status: 0x%x, out size: 0x%x\n",
			 HW_VF_ID_TO_OS(vf), err, mac_out->msg_head.status,
			 *out_size);
		return -EFAULT;
	}

	if (!mac_out->msg_head.status)
		ether_addr_copy(vf_info->drv_mac_addr, mac_in->new_mac);

	return err;
}

const struct vf_msg_handler vf_cmd_handler[] = {
	{
		.cmd = HINIC3_NIC_CMD_VF_REGISTER,
		.handler = hinic3_register_vf_msg_handler,
	},

	{
		.cmd = HINIC3_NIC_CMD_GET_MAC,
		.handler = hinic3_get_vf_mac_msg_handler,
	},

	{
		.cmd = HINIC3_NIC_CMD_SET_MAC,
		.handler = hinic3_set_vf_mac_msg_handler,
	},

	{
		.cmd = HINIC3_NIC_CMD_DEL_MAC,
		.handler = hinic3_del_vf_mac_msg_handler,
	},

	{
		.cmd = HINIC3_NIC_CMD_UPDATE_MAC,
		.handler = hinic3_update_vf_mac_msg_handler,
	},

	{
		.cmd = HINIC3_NIC_CMD_VF_COS,
		.handler = hinic3_get_vf_cos_msg_handler
	},
};

static int _l2nic_msg_to_mgmt_sync(void *hwdev, u16 cmd, void *buf_in,
				   u16 in_size, void *buf_out, u16 *out_size,
				   u16 channel)
{
	int i, cmd_cnt = ARRAY_LEN(vf_cmd_handler);
	bool cmd_to_pf = false;

	if (hinic3_func_type(hwdev) == TYPE_VF &&
	    !hinic3_is_slave_host(hwdev)) {
		for (i = 0; i < cmd_cnt; i++) {
			if (cmd == vf_cmd_handler[i].cmd)
				cmd_to_pf = true;
		}
	}

	if (cmd_to_pf)
		return hinic3_mbox_to_pf(hwdev, HINIC3_MOD_L2NIC, cmd, buf_in,
					 in_size, buf_out, out_size, 0,
					 channel);

	return hinic3_msg_to_mgmt_sync(hwdev, HINIC3_MOD_L2NIC, cmd, buf_in,
				       in_size, buf_out, out_size, 0, channel);
}

int l2nic_msg_to_mgmt_sync(void *hwdev, u16 cmd, void *buf_in, u16 in_size,
			   void *buf_out, u16 *out_size)
{
	return _l2nic_msg_to_mgmt_sync(hwdev, cmd, buf_in, in_size, buf_out,
				       out_size, HINIC3_CHANNEL_NIC);
}

int l2nic_msg_to_mgmt_sync_ch(void *hwdev, u16 cmd, void *buf_in, u16 in_size,
			      void *buf_out, u16 *out_size, u16 channel)
{
	return _l2nic_msg_to_mgmt_sync(hwdev, cmd, buf_in, in_size, buf_out,
				       out_size, channel);
}

/* pf/ppf handler mbox msg from vf */
int hinic3_pf_mbox_handler(void *hwdev,
			   u16 vf_id, u16 cmd, void *buf_in, u16 in_size,
			   void *buf_out, u16 *out_size)
{
	int index, cmd_size = ARRAY_LEN(vf_cmd_handler);
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev)
		return -EFAULT;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	for (index = 0; index < cmd_size; index++) {
		if (cmd == vf_cmd_handler[index].cmd)
			return vf_cmd_handler[index].handler(nic_io, vf_id,
							     buf_in, in_size,
							     buf_out, out_size);
	}

	nic_warn(nic_io->dev_hdl, "NO handler for nic cmd(%u) received from vf id: %u\n",
		 cmd, vf_id);

	return -EINVAL;
}

void hinic3_notify_dcb_state_event(struct hinic3_nic_io *nic_io,
				   struct hinic3_dcb_state *dcb_state)
{
	struct hinic3_event_info event_info = {0};
	int i;
/*lint -e679*/
	if (dcb_state->trust == HINIC3_DCB_PCP)
		/* This is 8 user priority to cos mapping relationships */
		sdk_info(nic_io->dev_hdl, "DCB %s, default cos %u, pcp2cos %u%u%u%u%u%u%u%u\n",
			 dcb_state->dcb_on ? "on" : "off", dcb_state->default_cos,
			 dcb_state->pcp2cos[ARRAY_INDEX_0], dcb_state->pcp2cos[ARRAY_INDEX_1],
			 dcb_state->pcp2cos[ARRAY_INDEX_2], dcb_state->pcp2cos[ARRAY_INDEX_3],
			 dcb_state->pcp2cos[ARRAY_INDEX_4], dcb_state->pcp2cos[ARRAY_INDEX_5],
			 dcb_state->pcp2cos[ARRAY_INDEX_6], dcb_state->pcp2cos[ARRAY_INDEX_7]);
	else
		for (i = 0; i < NIC_DCB_DSCP_NUM; i++) {
			sdk_info(nic_io->dev_hdl,
				 "DCB %s, default cos %u, dscp2cos %u%u%u%u%u%u%u%u\n",
				 dcb_state->dcb_on ? "on" : "off", dcb_state->default_cos,
				 dcb_state->dscp2cos[ARRAY_INDEX_0 + i * NIC_DCB_DSCP_NUM],
				 dcb_state->dscp2cos[ARRAY_INDEX_1 + i * NIC_DCB_DSCP_NUM],
				 dcb_state->dscp2cos[ARRAY_INDEX_2 + i * NIC_DCB_DSCP_NUM],
				 dcb_state->dscp2cos[ARRAY_INDEX_3 + i * NIC_DCB_DSCP_NUM],
				 dcb_state->dscp2cos[ARRAY_INDEX_4 + i * NIC_DCB_DSCP_NUM],
				 dcb_state->dscp2cos[ARRAY_INDEX_5 + i * NIC_DCB_DSCP_NUM],
				 dcb_state->dscp2cos[ARRAY_INDEX_6 + i * NIC_DCB_DSCP_NUM],
				 dcb_state->dscp2cos[ARRAY_INDEX_7 + i * NIC_DCB_DSCP_NUM]);
		}
/*lint +e679*/
	/* Saved in sdk for stateful module */
	hinic3_save_dcb_state(nic_io, dcb_state);

	event_info.service = EVENT_SRV_NIC;
	event_info.type = EVENT_NIC_DCB_STATE_CHANGE;
	memcpy((void *)event_info.event_data, dcb_state, sizeof(*dcb_state));

	hinic3_event_callback(nic_io->hwdev, &event_info);
}

static void dcb_state_event(void *hwdev, void *buf_in, u16 in_size,
			    void *buf_out, u16 *out_size)
{
	struct hinic3_cmd_vf_dcb_state *vf_dcb = NULL;
	struct hinic3_nic_io *nic_io = NULL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is NULL\n");
		return;
	}

	vf_dcb = buf_in;
	if (!vf_dcb)
		return;

	hinic3_notify_dcb_state_event(nic_io, &vf_dcb->state);
}

static void tx_pause_excp_event_handler(void *hwdev, void *buf_in, u16 in_size,
					void *buf_out, u16 *out_size)
{
	struct nic_cmd_tx_pause_notice *excp_info = buf_in;
	struct hinic3_nic_io *nic_io = NULL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is NULL\n");
		return;
	}

	if (in_size != sizeof(*excp_info)) {
		nic_err(nic_io->dev_hdl, "Invalid in_size: %u, should be %ld\n",
			in_size, sizeof(*excp_info));
		return;
	}

	nic_warn(nic_io->dev_hdl, "Receive tx pause exception event, excp: %u, level: %u\n",
		 excp_info->tx_pause_except, excp_info->except_level);

	hinic3_fault_event_report(hwdev, HINIC3_FAULT_SRC_TX_PAUSE_EXCP,
				  (u16)excp_info->except_level);
}

static void bond_active_event_handler(void *hwdev, void *buf_in, u16 in_size,
				      void *buf_out, u16 *out_size)
{
	struct hinic3_bond_active_report_info *active_info = buf_in;
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_event_info event_info = {0};

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is NULL\n");
		return;
	}

	if (in_size != sizeof(*active_info)) {
		nic_err(nic_io->dev_hdl, "Invalid in_size: %u, should be %ld\n",
			in_size, sizeof(*active_info));
		return;
	}

	event_info.service = EVENT_SRV_NIC;
	event_info.type = HINIC3_NIC_CMD_BOND_ACTIVE_NOTICE;
	memcpy((void *)event_info.event_data, active_info, sizeof(*active_info));

	hinic3_event_callback(nic_io->hwdev, &event_info);
}

static void outband_vlan_cfg_event_handler(void *hwdev, void *buf_in,
					   u16 in_size, void *buf_out,
					   u16 *out_size)
{
	struct hinic3_outband_cfg_info *outband_cfg_info = buf_in;
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_event_info event_info = {0};

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is NULL\n");
		return;
	}

	nic_info(nic_io->dev_hdl, "outband vlan cfg event received\n");

	if (in_size != sizeof(*outband_cfg_info)) {
		nic_err(nic_io->dev_hdl, "outband cfg info invalid in_size: %u, should be %lu\n",
			in_size, sizeof(*outband_cfg_info));
		return;
	}

	event_info.service = EVENT_SRV_NIC;
	event_info.type = EVENT_NIC_OUTBAND_CFG;
	memcpy((void *)event_info.event_data,
	       outband_cfg_info, sizeof(*outband_cfg_info));

	hinic3_event_callback(nic_io->hwdev, &event_info);
}

static const struct nic_event_handler nic_cmd_handler[] = {
	{
		.cmd = HINIC3_NIC_CMD_VF_COS,
		.handler = dcb_state_event,
	},
	{
		.cmd = HINIC3_NIC_CMD_TX_PAUSE_EXCP_NOTICE,
		.handler = tx_pause_excp_event_handler,
	},

	{
		.cmd = HINIC3_NIC_CMD_BOND_ACTIVE_NOTICE,
		.handler = bond_active_event_handler,
	},

	{
		.cmd = HINIC3_NIC_CMD_OUTBAND_CFG_NOTICE,
		.handler = outband_vlan_cfg_event_handler,
	},
};

static int _event_handler(void *hwdev, u16 cmd, void *buf_in, u16 in_size,
			  void *buf_out, u16 *out_size)
{
	struct hinic3_nic_io *nic_io = NULL;
	u32 size = sizeof(nic_cmd_handler) / sizeof(struct nic_event_handler);
	u32 i;

	if (!hwdev)
		return -EINVAL;

	*out_size = 0;
	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	for (i = 0; i < size; i++) {
		if (cmd == nic_cmd_handler[i].cmd) {
			nic_cmd_handler[i].handler(hwdev, buf_in, in_size,
						   buf_out, out_size);
			return 0;
		}
	}

	/* can't find this event cmd */
	sdk_warn(nic_io->dev_hdl, "Unsupported nic event, cmd: %u\n", cmd);
	*out_size = sizeof(struct mgmt_msg_head);
	((struct mgmt_msg_head *)buf_out)->status = HINIC3_MGMT_CMD_UNSUPPORTED;

	return 0;
}

/* vf handler mbox msg from ppf/pf */
/* vf link change event
 * vf fault report event, TBD
 */
int hinic3_vf_event_handler(void *hwdev,
			    u16 cmd, void *buf_in, u16 in_size,
			    void *buf_out, u16 *out_size)
{
	return _event_handler(hwdev, cmd, buf_in, in_size, buf_out, out_size);
}

/* pf/ppf handler mgmt cpu report nic event */
void hinic3_pf_event_handler(void *hwdev, u16 cmd,
			     void *buf_in, u16 in_size,
			     void *buf_out, u16 *out_size)
{
	_event_handler(hwdev, cmd, buf_in, in_size, buf_out, out_size);
}

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
#include "hinic3_srv_nic.h"
#include "hinic3_nic.h"
#include "hinic3_nic_cmdq.h"
#include "nic_mpu_cmd.h"
#include "nic_npu_cmd.h"
#include "hinic3_common.h"
#include "hinic3_nic_cfg.h"

#include "vram_common.h"

int hinic3_delete_bond(void *hwdev)
{
	struct hinic3_cmd_delete_bond cmd_delete_bond;
	u16 out_size = sizeof(cmd_delete_bond);
	struct hinic3_nic_io *nic_io = NULL;
	int err = 0;

	if (!hwdev) {
		pr_err("hwdev is null.\n");
		return -EINVAL;
	}

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is null.\n");
		return -EINVAL;
	}

	memset(&cmd_delete_bond, 0, sizeof(cmd_delete_bond));
	cmd_delete_bond.bond_id = HINIC3_INVALID_BOND_ID;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_BOND_DEV_DELETE,
				     &cmd_delete_bond, sizeof(cmd_delete_bond),
				     &cmd_delete_bond, &out_size);
	if (err || !out_size || cmd_delete_bond.head.status) {
		nic_err(nic_io->dev_hdl, "Failed to delete bond, err: %d, status: 0x%x, out_size: 0x%x\n",
			err, cmd_delete_bond.head.status, out_size);
		return -EFAULT;
	}

	if (cmd_delete_bond.bond_id != HINIC3_INVALID_BOND_ID)
		nic_info(nic_io->dev_hdl, "Delete bond success\n");

	return 0;
}

int hinic3_open_close_bond(void *hwdev, u32 bond_en)
{
	struct hinic3_cmd_open_close_bond cmd_open_close_bond;
	u16 out_size = sizeof(cmd_open_close_bond);
	struct hinic3_nic_io *nic_io = NULL;
	int err = 0;

	if (!hwdev) {
		pr_err("hwdev is null.\n");
		return -EINVAL;
	}

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is null.\n");
		return -EINVAL;
	}

	memset(&cmd_open_close_bond, 0, sizeof(cmd_open_close_bond));
	cmd_open_close_bond.open_close_bond_info.bond_id =
						HINIC3_INVALID_BOND_ID;
	cmd_open_close_bond.open_close_bond_info.open_close_flag = bond_en;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_BOND_DEV_OPEN_CLOSE,
				     &cmd_open_close_bond,
				     sizeof(cmd_open_close_bond),
				     &cmd_open_close_bond, &out_size);
	if (err || !out_size || cmd_open_close_bond.head.status) {
		nic_err(nic_io->dev_hdl, "Failed to %s bond, err: %d, status: 0x%x, out_size: 0x%x\n",
			bond_en ? "open" : "close", err,
			cmd_open_close_bond.head.status, out_size);
		return -EFAULT;
	}

	if (cmd_open_close_bond.open_close_bond_info.bond_id !=
	    HINIC3_INVALID_BOND_ID) {
		nic_info(nic_io->dev_hdl, "%s bond success\n",
			 bond_en ? "Open" : "Close");
	}

	return 0;
}

int hinic3_create_bond(void *hwdev, u32 *bond_id)
{
	struct hinic3_cmd_create_bond cmd_create_bond;
	u16 out_size = sizeof(cmd_create_bond);
	struct hinic3_nic_io *nic_io = NULL;
	int err = 0;

	if (!hwdev) {
		pr_err("hwdev is null.\n");
		return -EINVAL;
	}

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is null.\n");
		return -EINVAL;
	}

	memset(&cmd_create_bond, 0, sizeof(cmd_create_bond));
	cmd_create_bond.create_bond_info.default_param_flag = true;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_BOND_DEV_CREATE,
				     &cmd_create_bond, sizeof(cmd_create_bond),
				     &cmd_create_bond, &out_size);
	if (err || !out_size || cmd_create_bond.head.status) {
		nic_err(nic_io->dev_hdl, "Failed to create default bond, err: %d, status: 0x%x, out_size: 0x%x\n",
			err, cmd_create_bond.head.status, out_size);
		return -EFAULT;
	}

	if (cmd_create_bond.create_bond_info.bond_id !=
	    HINIC3_INVALID_BOND_ID) {
		*bond_id = cmd_create_bond.create_bond_info.bond_id;
		nic_info(nic_io->dev_hdl, "Create bond success\n");
	}

	return 0;
}

int hinic3_set_sq_ci_ctx(struct hinic3_nic_io *nic_io,
			 struct hinic3_sq_attr *attr)
{
	struct hinic3_cmd_cons_idx_attr cons_idx_attr;
	u16 out_size = sizeof(cons_idx_attr);
	int err;

	if (!nic_io || !attr)
		return -EINVAL;

	memset(&cons_idx_attr, 0, sizeof(cons_idx_attr));
	cons_idx_attr.func_idx = hinic3_global_func_id(nic_io->hwdev);
	cons_idx_attr.dma_attr_off  = attr->dma_attr_off;
	cons_idx_attr.pending_limit = attr->pending_limit;
	cons_idx_attr.coalescing_time  = attr->coalescing_time;

	if (attr->intr_en) {
		cons_idx_attr.intr_en = attr->intr_en;
		cons_idx_attr.intr_idx = attr->intr_idx;
	}

	cons_idx_attr.l2nic_sqn = attr->l2nic_sqn;
	cons_idx_attr.ci_addr = attr->ci_dma_base >> SQ_CI_ADDR_SHIFT;

	err = l2nic_msg_to_mgmt_sync(nic_io->hwdev,
				     HINIC3_NIC_CMD_SQ_CI_ATTR_SET,
				     &cons_idx_attr, sizeof(cons_idx_attr),
				     &cons_idx_attr, &out_size);
	if (err || !out_size || cons_idx_attr.msg_head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to set ci attribute table, err: %d, status: 0x%x, out_size: 0x%x\n",
			err, cons_idx_attr.msg_head.status, out_size);
		return -EFAULT;
	}

	return 0;
}

int hinic3_set_rq_ci_ctx(struct hinic3_nic_io *nic_io,
			 struct hinic3_rq_attr *attr)
{
	struct hinic3_rq_cqe_ctx cons_idx_ctx;
	u16 out_size = sizeof(cons_idx_ctx);
	int err;

	if (!nic_io || !attr)
		return -EINVAL;

	memset(&cons_idx_ctx, 0, sizeof(cons_idx_ctx));
	cons_idx_ctx.cqe_type = attr->cqe_type;
	cons_idx_ctx.rq_id = (u8)(attr->l2nic_rqn & 0xff);
	cons_idx_ctx.timer_loop = attr->coalescing_time;
	cons_idx_ctx.threshold_cqe_num = attr->pending_limit;
	cons_idx_ctx.msix_entry_idx = attr->intr_idx;
	cons_idx_ctx.ci_addr_hi = upper_32_bits(attr->ci_dma_base >>
							RQ_CI_ADDR_SHIFT);
	cons_idx_ctx.ci_addr_lo = lower_32_bits(attr->ci_dma_base >>
							RQ_CI_ADDR_SHIFT);

	err = l2nic_msg_to_mgmt_sync(nic_io->hwdev,
				     HINIC3_NIC_CMD_SET_RQ_CI_CTX,
				     &cons_idx_ctx, sizeof(cons_idx_ctx),
				     &cons_idx_ctx, &out_size);
	if (err || !out_size || cons_idx_ctx.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Set rq cqe ctx fail, qid: %d, err: %d, status: 0x%x, out_size: 0x%x",
			attr->l2nic_rqn, err,
			cons_idx_ctx.msg_head.status, out_size);
		return -EFAULT;
	}

	return 0;
}

#define PF_SET_VF_MAC(hwdev, status)	\
		(hinic3_func_type(hwdev) == TYPE_VF && \
		(status) == HINIC3_PF_SET_VF_ALREADY)

static int hinic3_check_mac_info(void *hwdev, u8 status, u16 vlan_id)
{
	if ((status && status != HINIC3_MGMT_STATUS_EXIST) ||
	    ((vlan_id & CHECK_IPSU_15BIT) &&
	     status == HINIC3_MGMT_STATUS_EXIST)) {
		if (PF_SET_VF_MAC(hwdev, status))
			return 0;

		return -EINVAL;
	}

	return 0;
}

#define HINIC_VLAN_ID_MASK	0x7FFF

int hinic3_set_mac(void *hwdev, const u8 *mac_addr, u16 vlan_id, u16 func_id,
		   u16 channel)
{
	struct hinic3_port_mac_set mac_info;
	u16 out_size = sizeof(mac_info);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !mac_addr)
		return -EINVAL;

	memset(&mac_info, 0, sizeof(mac_info));

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if ((vlan_id & HINIC_VLAN_ID_MASK) >= VLAN_N_VID) {
		nic_err(nic_io->dev_hdl, "Invalid VLAN number: %d\n",
			(vlan_id & HINIC_VLAN_ID_MASK));
		return -EINVAL;
	}

	mac_info.func_id = func_id;
	mac_info.vlan_id = vlan_id;
	ether_addr_copy(mac_info.mac, mac_addr);

	err = l2nic_msg_to_mgmt_sync_ch(hwdev, HINIC3_NIC_CMD_SET_MAC,
					&mac_info, sizeof(mac_info),
					&mac_info, &out_size, channel);
	if (err || !out_size ||
	    (hinic3_check_mac_info(hwdev, mac_info.msg_head.status,
				  mac_info.vlan_id) != 0)) {
		nic_err(nic_io->dev_hdl,
			"Failed to update MAC, err: %d, status: 0x%x, out size: 0x%x, channel: 0x%x\n",
			err, mac_info.msg_head.status, out_size, channel);
		return -EIO;
	}

	if (PF_SET_VF_MAC(hwdev, mac_info.msg_head.status)) {
		nic_warn(nic_io->dev_hdl, "PF has already set VF mac, Ignore set operation\n");
		return HINIC3_PF_SET_VF_ALREADY;
	}

	if (mac_info.msg_head.status == HINIC3_MGMT_STATUS_EXIST) {
		nic_warn(nic_io->dev_hdl, "MAC is repeated. Ignore update operation\n");
		return 0;
	}

	return 0;
}
EXPORT_SYMBOL(hinic3_set_mac);

int hinic3_del_mac(void *hwdev, const u8 *mac_addr, u16 vlan_id, u16 func_id,
		   u16 channel)
{
	struct hinic3_port_mac_set mac_info;
	u16 out_size = sizeof(mac_info);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !mac_addr)
		return -EINVAL;

	memset(&mac_info, 0, sizeof(mac_info));

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if ((vlan_id & HINIC_VLAN_ID_MASK) >= VLAN_N_VID) {
		nic_err(nic_io->dev_hdl, "Invalid VLAN number: %d\n",
			(vlan_id & HINIC_VLAN_ID_MASK));
		return -EINVAL;
	}

	mac_info.func_id = func_id;
	mac_info.vlan_id = vlan_id;
	ether_addr_copy(mac_info.mac, mac_addr);

	err = l2nic_msg_to_mgmt_sync_ch(hwdev, HINIC3_NIC_CMD_DEL_MAC,
					&mac_info, sizeof(mac_info), &mac_info,
					&out_size, channel);
	if (err || !out_size ||
	    (mac_info.msg_head.status &&
	    !PF_SET_VF_MAC(hwdev, mac_info.msg_head.status))) {
		nic_err(nic_io->dev_hdl,
			"Failed to delete MAC, err: %d, status: 0x%x, out size: 0x%x, channel: 0x%x\n",
			err, mac_info.msg_head.status, out_size, channel);
		return -EIO;
	}

	if (PF_SET_VF_MAC(hwdev, mac_info.msg_head.status)) {
		nic_warn(nic_io->dev_hdl, "PF has already set VF mac, Ignore delete operation.\n");
		return HINIC3_PF_SET_VF_ALREADY;
	}

	return 0;
}
EXPORT_SYMBOL(hinic3_del_mac);

int hinic3_update_mac(void *hwdev, const u8 *old_mac, u8 *new_mac, u16 vlan_id,
		      u16 func_id)
{
	struct hinic3_port_mac_update mac_info;
	u16 out_size = sizeof(mac_info);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !old_mac || !new_mac)
		return -EINVAL;

	memset(&mac_info, 0, sizeof(mac_info));

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if ((vlan_id & HINIC_VLAN_ID_MASK) >= VLAN_N_VID) {
		nic_err(nic_io->dev_hdl, "Invalid VLAN number: %d\n",
			(vlan_id & HINIC_VLAN_ID_MASK));
		return -EINVAL;
	}

	mac_info.func_id = func_id;
	mac_info.vlan_id = vlan_id;
	ether_addr_copy(mac_info.old_mac, old_mac);
	ether_addr_copy(mac_info.new_mac, new_mac);

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_UPDATE_MAC,
				     &mac_info, sizeof(mac_info),
				     &mac_info, &out_size);
	if (err || !out_size ||
	    (hinic3_check_mac_info(hwdev, mac_info.msg_head.status,
				  mac_info.vlan_id) != 0)) {
		nic_err(nic_io->dev_hdl,
			"Failed to update MAC, err: %d, status: 0x%x, out size: 0x%x\n",
			err, mac_info.msg_head.status, out_size);
		return -EIO;
	}

	if (PF_SET_VF_MAC(hwdev, mac_info.msg_head.status)) {
		nic_warn(nic_io->dev_hdl, "PF has already set VF MAC. Ignore update operation\n");
		return HINIC3_PF_SET_VF_ALREADY;
	}

	if (mac_info.msg_head.status == HINIC3_MGMT_STATUS_EXIST) {
		nic_warn(nic_io->dev_hdl, "MAC is repeated. Ignore update operation\n");
		return 0;
	}

	return 0;
}

int hinic3_get_default_mac(void *hwdev, u8 *mac_addr)
{
	struct hinic3_port_mac_set mac_info;
	u16 out_size = sizeof(mac_info);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !mac_addr)
		return -EINVAL;

	memset(&mac_info, 0, sizeof(mac_info));

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	mac_info.func_id = hinic3_global_func_id(hwdev);

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_GET_MAC,
				     &mac_info, sizeof(mac_info),
		&mac_info, &out_size);
	if (err || !out_size || mac_info.msg_head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to get mac, err: %d, status: 0x%x, out size: 0x%x\n",
			err, mac_info.msg_head.status, out_size);
		return -EINVAL;
	}

	ether_addr_copy(mac_addr, mac_info.mac);

	return 0;
}

static int hinic3_config_vlan(struct hinic3_nic_io *nic_io, u8 opcode,
			      u16 vlan_id, u16 func_id)
{
	struct hinic3_cmd_vlan_config vlan_info;
	u16 out_size = sizeof(vlan_info);
	int err;

	memset(&vlan_info, 0, sizeof(vlan_info));
	vlan_info.opcode = opcode;
	vlan_info.func_id = func_id;
	vlan_info.vlan_id = vlan_id;

	err = l2nic_msg_to_mgmt_sync(nic_io->hwdev,
				     HINIC3_NIC_CMD_CFG_FUNC_VLAN,
				     &vlan_info, sizeof(vlan_info),
				     &vlan_info, &out_size);
	if (err || !out_size || vlan_info.msg_head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to %s vlan, err: %d, status: 0x%x, out size: 0x%x\n",
			opcode == HINIC3_CMD_OP_ADD ? "add" : "delete",
			err, vlan_info.msg_head.status, out_size);
		return -EINVAL;
	}

	return 0;
}

#if defined(HAVE_NDO_UDP_TUNNEL_ADD) || defined(HAVE_UDP_TUNNEL_NIC_INFO)
int hinic3_vlxan_port_config(void *hwdev, u16 func_id, u16 port, u8 action)
{
	struct hinic3_cmd_vxlan_port_info vxlan_port_info;
	u16 out_size = sizeof(vxlan_port_info);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&vxlan_port_info, 0, sizeof(vxlan_port_info));
	vxlan_port_info.opcode = action;
	vxlan_port_info.cfg_mode = 0; // other tool set
	vxlan_port_info.func_id = func_id;
	vxlan_port_info.vxlan_port = port;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_CFG_VXLAN_PORT,
				     &vxlan_port_info, sizeof(vxlan_port_info),
				     &vxlan_port_info, &out_size);
	if (err || !out_size || vxlan_port_info.msg_head.status) {
		if (vxlan_port_info.msg_head.status == 0x2) // other tool set failed
			nic_warn(nic_io->dev_hdl,
				 "Failed to %s vxlan dst port because it has already been set by hinicadm\n",
				 action == HINIC3_CMD_OP_ADD
				 ? "add" : "delete");
		else
			nic_err(nic_io->dev_hdl,
				"Failed to %s vxlan dst port, err: %d, status: 0x%x, out size: 0x%x\n",
				action == HINIC3_CMD_OP_ADD ? "add" : "delete",
				err, vxlan_port_info.msg_head.status, out_size);
		return -EINVAL;
	}

	return 0;
}
#endif /* HAVE_NDO_UDP_TUNNEL_ADD || HAVE_UDP_TUNNEL_NIC_INFO */

int hinic3_add_vlan(void *hwdev, u16 vlan_id, u16 func_id)
{
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	return hinic3_config_vlan(nic_io, HINIC3_CMD_OP_ADD, vlan_id, func_id);
}

int hinic3_del_vlan(void *hwdev, u16 vlan_id, u16 func_id)
{
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	return hinic3_config_vlan(nic_io, HINIC3_CMD_OP_DEL, vlan_id, func_id);
}

int hinic3_set_vport_enable(void *hwdev, u16 func_id, bool enable, u16 channel)
{
	struct hinic3_vport_state en_state;
	u16 out_size = sizeof(en_state);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev)
		return -EINVAL;

	memset(&en_state, 0, sizeof(en_state));

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	en_state.func_id = func_id;
	en_state.state = enable ? 1 : 0;

	err = l2nic_msg_to_mgmt_sync_ch(hwdev, HINIC3_NIC_CMD_SET_VPORT_ENABLE,
					&en_state, sizeof(en_state),
					&en_state, &out_size, channel);
	if (err || !out_size || en_state.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to set vport state, err: %d, status: 0x%x, out size: 0x%x, channel: 0x%x\n",
			err, en_state.msg_head.status, out_size, channel);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(hinic3_set_vport_enable);

int hinic3_set_dcb_state(void *hwdev, struct hinic3_dcb_state *dcb_state)
{
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev || !dcb_state)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	if (!memcmp(&nic_io->dcb_state, dcb_state, sizeof(nic_io->dcb_state)))
		return 0;

	/* save in sdk, vf will get dcb state when probing */
	hinic3_save_dcb_state(nic_io, dcb_state);

	/* notify stateful in pf, than notify all vf */
	hinic3_notify_dcb_state_event(nic_io, dcb_state);

	return 0;
}

int hinic3_get_dcb_state(void *hwdev, struct hinic3_dcb_state *dcb_state)
{
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev || !dcb_state)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memcpy(dcb_state, &nic_io->dcb_state, sizeof(*dcb_state));

	return 0;
}
EXPORT_SYMBOL(hinic3_get_dcb_state);

int hinic3_get_cos_by_pri(void *hwdev, u8 pri, u8 *cos)
{
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev || !cos)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if (pri >= NIC_DCB_UP_MAX && nic_io->dcb_state.trust == HINIC3_DCB_PCP)
		return -EINVAL;

	if (pri >= NIC_DCB_IP_PRI_MAX &&
	    nic_io->dcb_state.trust == HINIC3_DCB_DSCP)
		return -EINVAL;

/*lint -e662*/
/*lint -e661*/
	if (nic_io->dcb_state.dcb_on) {
		if (nic_io->dcb_state.trust == HINIC3_DCB_PCP)
			*cos = nic_io->dcb_state.pcp2cos[pri];
		else
			*cos = nic_io->dcb_state.dscp2cos[pri];
	} else {
		*cos = nic_io->dcb_state.default_cos;
	}
/*lint +e662*/
/*lint +e661*/
	return 0;
}
EXPORT_SYMBOL(hinic3_get_cos_by_pri);

int hinic3_save_dcb_state(struct hinic3_nic_io *nic_io,
			  struct hinic3_dcb_state *dcb_state)
{
	memcpy(&nic_io->dcb_state, dcb_state, sizeof(*dcb_state));

	return 0;
}

int hinic3_get_pf_dcb_state(void *hwdev, struct hinic3_dcb_state *dcb_state)
{
	struct hinic3_cmd_vf_dcb_state vf_dcb;
	struct hinic3_nic_io *nic_io = NULL;
	u16 out_size = sizeof(vf_dcb);
	int err;

	if (!hwdev || !dcb_state)
		return -EINVAL;

	memset(&vf_dcb, 0, sizeof(vf_dcb));

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if (hinic3_func_type(hwdev) != TYPE_VF) {
		nic_err(nic_io->dev_hdl, "Only vf need to get pf dcb state\n");
		return -EINVAL;
	}

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_VF_COS, &vf_dcb,
				     sizeof(vf_dcb), &vf_dcb, &out_size);
	if (err || !out_size || vf_dcb.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to get vf default cos, err: %d, status: 0x%x, out size: 0x%x\n",
			err, vf_dcb.msg_head.status, out_size);
		return -EFAULT;
	}

	memcpy(dcb_state, &vf_dcb.state, sizeof(*dcb_state));
	/* Save dcb_state in hw for stateful module */
	hinic3_save_dcb_state(nic_io, dcb_state);

	return 0;
}
EXPORT_SYMBOL(hinic3_get_pf_dcb_state);

#define UNSUPPORT_SET_PAUSE 0x10
static int hinic3_cfg_hw_pause(struct hinic3_nic_io *nic_io, u8 opcode,
			       struct nic_pause_config *nic_pause)
{
	struct hinic3_cmd_pause_config pause_info;
	u16 out_size = sizeof(pause_info);
	int err;

	memset(&pause_info, 0, sizeof(pause_info));

	pause_info.port_id = hinic3_physical_port_id(nic_io->hwdev);
	pause_info.opcode = opcode;
	if (opcode == HINIC3_CMD_OP_SET) {
		pause_info.auto_neg = nic_pause->auto_neg;
		pause_info.rx_pause = nic_pause->rx_pause;
		pause_info.tx_pause = nic_pause->tx_pause;
	}

	err = l2nic_msg_to_mgmt_sync(nic_io->hwdev,
				     HINIC3_NIC_CMD_CFG_PAUSE_INFO,
				     &pause_info, sizeof(pause_info),
				     &pause_info, &out_size);
	if (err || !out_size || pause_info.msg_head.status) {
		if (pause_info.msg_head.status == UNSUPPORT_SET_PAUSE) {
			err = -EOPNOTSUPP;
			nic_err(nic_io->dev_hdl, "Can not set pause when pfc is enable\n");
		} else {
			err = -EFAULT;
			nic_err(nic_io->dev_hdl, "Failed to %s pause info, err: %d, status: 0x%x, out size: 0x%x\n",
				opcode == HINIC3_CMD_OP_SET ? "set" : "get",
				err, pause_info.msg_head.status, out_size);
		}
		return err;
	}

	if (opcode == HINIC3_CMD_OP_GET) {
		nic_pause->auto_neg = pause_info.auto_neg;
		nic_pause->rx_pause = pause_info.rx_pause;
		nic_pause->tx_pause = pause_info.tx_pause;
	}

	return 0;
}

int hinic3_set_pause_info(void *hwdev, struct nic_pause_config nic_pause)
{
	struct hinic3_nic_cfg *nic_cfg = NULL;
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	nic_cfg = &nic_io->nic_cfg;

	down(&nic_cfg->cfg_lock);

	err = hinic3_cfg_hw_pause(nic_io, HINIC3_CMD_OP_SET, &nic_pause);
	if (err != 0) {
		up(&nic_cfg->cfg_lock);
		return err;
	}

	nic_cfg->pfc_en = 0;
	nic_cfg->pfc_bitmap = 0;
	nic_cfg->pause_set = true;
	nic_cfg->nic_pause.auto_neg = nic_pause.auto_neg;
	nic_cfg->nic_pause.rx_pause = nic_pause.rx_pause;
	nic_cfg->nic_pause.tx_pause = nic_pause.tx_pause;

	up(&nic_cfg->cfg_lock);

	return 0;
}

int hinic3_get_pause_info(void *hwdev, struct nic_pause_config *nic_pause)
{
	struct hinic3_nic_io *nic_io = NULL;
	int err = 0;

	if (!hwdev || !nic_pause)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	err = hinic3_cfg_hw_pause(nic_io, HINIC3_CMD_OP_GET, nic_pause);
	if (err != 0)
		return err;

	return 0;
}

int hinic3_sync_dcb_state(void *hwdev, u8 op_code, u8 state)
{
	struct hinic3_cmd_set_dcb_state dcb_state;
	struct hinic3_nic_io *nic_io = NULL;
	u16 out_size = sizeof(dcb_state);
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&dcb_state, 0, sizeof(dcb_state));

	dcb_state.op_code = op_code;
	dcb_state.state = state;
	dcb_state.func_id = hinic3_global_func_id(hwdev);

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_QOS_DCB_STATE,
				     &dcb_state, sizeof(dcb_state),
				     &dcb_state, &out_size);
	if (err || dcb_state.head.status || !out_size) {
		nic_err(nic_io->dev_hdl,
			"Failed to set dcb state, err: %d, status: 0x%x, out size: 0x%x\n",
			err, dcb_state.head.status, out_size);
		return -EFAULT;
	}

	return 0;
}

int hinic3_dcb_set_rq_iq_mapping(void *hwdev, u32 num_rqs, u8 *map,
				 u32 max_map_num)
{
	return 0;
}
EXPORT_SYMBOL(hinic3_dcb_set_rq_iq_mapping);

int hinic3_flush_qps_res(void *hwdev)
{
	struct hinic3_cmd_clear_qp_resource sq_res;
	u16 out_size = sizeof(sq_res);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&sq_res, 0, sizeof(sq_res));

	sq_res.func_id = hinic3_global_func_id(hwdev);

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_CLEAR_QP_RESOURCE,
				     &sq_res, sizeof(sq_res), &sq_res,
				     &out_size);
	if (err || !out_size || sq_res.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to clear sq resources, err: %d, status: 0x%x, out size: 0x%x\n",
			err, sq_res.msg_head.status, out_size);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(hinic3_flush_qps_res);

int hinic3_cache_out_qps_res(void *hwdev)
{
	struct hinic3_cmd_cache_out_qp_resource qp_res;
	u16 out_size = sizeof(qp_res);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&qp_res, 0, sizeof(qp_res));

	qp_res.func_id = hinic3_global_func_id(hwdev);

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_CACHE_OUT_QP_RES,
				     &qp_res, sizeof(qp_res),
				     &qp_res, &out_size);
	if (err || !out_size || qp_res.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to cache out qp resources, err: %d, status: 0x%x, out size: 0x%x\n",
			err, qp_res.msg_head.status, out_size);
		return -EIO;
	}

	return 0;
}

int hinic3_get_vport_stats(void *hwdev, u16 func_id,
			   struct hinic3_vport_stats *stats)
{
	struct hinic3_port_stats_info stats_info;
	struct hinic3_cmd_vport_stats vport_stats;
	u16 out_size = sizeof(vport_stats);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !stats)
		return -EINVAL;

	memset(&stats_info, 0, sizeof(stats_info));
	memset(&vport_stats, 0, sizeof(vport_stats));

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	stats_info.func_id = func_id;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_GET_VPORT_STAT,
				     &stats_info, sizeof(stats_info),
				     &vport_stats, &out_size);
	if (err || !out_size || vport_stats.msg_head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to get function statistics, err: %d, status: 0x%x, out size: 0x%x\n",
			err, vport_stats.msg_head.status, out_size);
		return -EFAULT;
	}

	memcpy(stats, &vport_stats.stats, sizeof(*stats));

	return 0;
}

static int hinic3_set_function_table(struct hinic3_nic_io *nic_io,
				     u32 cfg_bitmap,
				     const struct hinic3_func_tbl_cfg *cfg)
{
	struct hinic3_cmd_set_func_tbl cmd_func_tbl;
	u16 out_size = sizeof(cmd_func_tbl);
	int err;

	memset(&cmd_func_tbl, 0, sizeof(cmd_func_tbl));
	cmd_func_tbl.func_id = hinic3_global_func_id(nic_io->hwdev);
	cmd_func_tbl.cfg_bitmap = cfg_bitmap;
	cmd_func_tbl.tbl_cfg = *cfg;

	err = l2nic_msg_to_mgmt_sync(nic_io->hwdev,
				     HINIC3_NIC_CMD_SET_FUNC_TBL,
				     &cmd_func_tbl, sizeof(cmd_func_tbl),
				     &cmd_func_tbl, &out_size);
	if (err || cmd_func_tbl.msg_head.status || !out_size) {
		nic_err(nic_io->dev_hdl,
			"Failed to set func table, bitmap: 0x%x, err: %d, status: 0x%x, out size: 0x%x\n",
			cfg_bitmap, err, cmd_func_tbl.msg_head.status,
			out_size);
		return -EFAULT;
	}

	return 0;
}

static int hinic3_init_function_table(struct hinic3_nic_io *nic_io)
{
	struct hinic3_func_tbl_cfg func_tbl_cfg = {0};
	u32 cfg_bitmap = BIT(FUNC_CFG_INIT) | BIT(FUNC_CFG_MTU) |
			BIT(FUNC_CFG_RX_BUF_SIZE);

	func_tbl_cfg.mtu = 0x3FFF; /* default, max mtu */
	func_tbl_cfg.rx_wqe_buf_size = nic_io->rx_buff_len;

	return hinic3_set_function_table(nic_io, cfg_bitmap, &func_tbl_cfg);
}

int hinic3_set_port_mtu(void *hwdev, u16 new_mtu)
{
	struct hinic3_func_tbl_cfg func_tbl_cfg = {0};
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if (new_mtu < HINIC3_MIN_MTU_SIZE) {
		nic_err(nic_io->dev_hdl,
			"Invalid mtu size: %ubytes, mtu size < %ubytes",
			new_mtu, HINIC3_MIN_MTU_SIZE);
		return -EINVAL;
	}

	if (new_mtu > HINIC3_MAX_JUMBO_FRAME_SIZE) {
		nic_err(nic_io->dev_hdl, "Invalid mtu size: %ubytes, mtu size > %ubytes",
			new_mtu, HINIC3_MAX_JUMBO_FRAME_SIZE);
		return -EINVAL;
	}

	func_tbl_cfg.mtu = new_mtu;
	return hinic3_set_function_table(nic_io, BIT(FUNC_CFG_MTU),
					 &func_tbl_cfg);
}

static int nic_feature_nego(void *hwdev, u8 opcode, u64 *s_feature, u16 size)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_cmd_feature_nego feature_nego;
	u16 out_size = sizeof(feature_nego);
	int err;

	if (!hwdev || !s_feature || size > NIC_MAX_FEATURE_QWORD)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	memset(&feature_nego, 0, sizeof(feature_nego));
	feature_nego.func_id = hinic3_global_func_id(hwdev);
	feature_nego.opcode = opcode;
	if (opcode == HINIC3_CMD_OP_SET)
		memcpy(feature_nego.s_feature, s_feature, size * sizeof(u64));

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_FEATURE_NEGO,
				     &feature_nego, sizeof(feature_nego),
				     &feature_nego, &out_size);
	if (err || !out_size || feature_nego.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to negotiate nic feature, err:%d, status: 0x%x, out_size: 0x%x\n",
			err, feature_nego.msg_head.status, out_size);
		return -EIO;
	}

	if (opcode == HINIC3_CMD_OP_GET)
		memcpy(s_feature, feature_nego.s_feature, size * sizeof(u64));

	return 0;
}

static int hinic3_get_bios_pf_bw_tx_limit(void *hwdev,
					  struct hinic3_nic_io *nic_io,
					  u16 func_id, u32 *pf_rate)
{
	int err = 0; // default success
	struct nic_cmd_bios_cfg cfg = {{0}};
	u16 out_size = sizeof(cfg);

	cfg.bios_cfg.func_id = (u8)func_id;
	cfg.bios_cfg.func_valid = 1;
	cfg.op_code = 0 | NIC_NVM_DATA_PF_TX_SPEED_LIMIT;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_BIOS_CFG,
				     &cfg, sizeof(cfg),
				     &cfg, &out_size);
	if (err || !out_size || cfg.head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to get bios pf bandwidth tx limit, err: %d, status: 0x%x, out size: 0x%x\n",
			err, cfg.head.status, out_size);
		return -EIO;
	}

	/* check data is valid or not */
	if (cfg.bios_cfg.signature != BIOS_CFG_SIGNATURE)
		nic_warn(nic_io->dev_hdl, "Invalid bios configuration data, signature: 0x%x\n",
			 cfg.bios_cfg.signature);

	if (cfg.bios_cfg.pf_tx_bw > MAX_LIMIT_BW) {
		nic_err(nic_io->dev_hdl, "Invalid bios cfg pf bandwidth limit: %u\n",
			cfg.bios_cfg.pf_tx_bw);
		return -EINVAL;
	}

	(*pf_rate) = cfg.bios_cfg.pf_tx_bw;
	return err;
}

static int hinic3_get_bios_pf_bw_rx_limit(void *hwdev,
					  struct hinic3_nic_io *nic_io,
					  u16 func_id, u32 *pf_rate)
{
	int err = 0; // default success
	struct nic_rx_rate_bios_cfg rx_bios_conf = {{0}};
	u16 out_size = sizeof(rx_bios_conf);

	rx_bios_conf.func_id = (u8)func_id;
	rx_bios_conf.op_code = 0; /* 1-save, 0-read */
	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_RX_RATE_CFG,
				     &rx_bios_conf, sizeof(rx_bios_conf),
				     &rx_bios_conf, &out_size);
	if (rx_bios_conf.msg_head.status == HINIC3_MGMT_CMD_UNSUPPORTED &&
	    err == 0) { // Compatible older firmware
		nic_warn(nic_io->dev_hdl, "Not support get bios pf bandwidth rx limit\n");
		return 0;
	} else if (err || !out_size || rx_bios_conf.msg_head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to get bios pf bandwidth rx limit, err: %d, status: 0x%x, out size: 0x%x\n",
			err, rx_bios_conf.msg_head.status, out_size);
		return -EIO;
	}
	if (rx_bios_conf.rx_rate_limit > MAX_LIMIT_BW) {
		nic_err(nic_io->dev_hdl, "Invalid bios cfg pf bandwidth limit: %u\n",
			rx_bios_conf.rx_rate_limit);
		return -EINVAL;
	}

	(*pf_rate) = rx_bios_conf.rx_rate_limit;
	return err;
}

static int hinic3_get_bios_pf_bw_limit(void *hwdev, u32 *pf_bw_limit, u8 direct)
{
	struct hinic3_nic_io *nic_io = NULL;
	u32 pf_rate = 0;
	int err = 0;
	u16 func_id;

	func_id = hinic3_global_func_id(hwdev);

	if (!hwdev || !pf_bw_limit)
		return -EINVAL;

	if (hinic3_func_type(hwdev) == TYPE_VF ||
	    !HINIC3_SUPPORT_RATE_LIMIT(hwdev))
		return 0;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if (direct == HINIC3_NIC_TX)
		err = hinic3_get_bios_pf_bw_tx_limit(hwdev, nic_io,
						     func_id, &pf_rate);
	else if (direct == HINIC3_NIC_RX)
		err = hinic3_get_bios_pf_bw_rx_limit(hwdev, nic_io,
						     func_id, &pf_rate);

	if (err != 0)
		return err;

	if (pf_rate > MAX_LIMIT_BW) {
		nic_err(nic_io->dev_hdl,
			"Invalid bios cfg pf bandwidth limit: %u\n", pf_rate);
		return -EINVAL;
	}

	*pf_bw_limit = pf_rate;

	return 0;
}

int hinic3_set_pf_rate(void *hwdev, u8 speed_level)
{
	struct hinic3_cmd_tx_rate_cfg rate_cfg = {{0}};
	struct hinic3_nic_io *nic_io = NULL;
	u32 rate_limit;
	u16 out_size = sizeof(rate_cfg);
	u32 pf_rate = 0;
	int err;
	u32 speed_convert[PORT_SPEED_UNKNOWN] = {
		0, 10, 100, 1000, 10000, 25000, 40000, 50000, 100000, 200000
	};

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	if (speed_level >= PORT_SPEED_UNKNOWN) {
		nic_err(nic_io->dev_hdl,
			"Invalid speed level: %u\n", speed_level);
		return -EINVAL;
	}

	rate_limit = (nic_io->direct == HINIC3_NIC_TX) ?
		nic_io->nic_cfg.pf_bw_tx_limit : nic_io->nic_cfg.pf_bw_rx_limit;
	if (rate_limit != MAX_LIMIT_BW) {
		/* divided by 100 to convert to percentage */
		pf_rate = (speed_convert[speed_level] / 100) * rate_limit;
		/* bandwidth limit is very small but not unlimit in this case */
		if (pf_rate == 0 && speed_level != PORT_SPEED_NOT_SET)
			pf_rate = 1;
	}

	rate_cfg.func_id = hinic3_global_func_id(hwdev);
	rate_cfg.min_rate = 0;
	rate_cfg.max_rate = pf_rate;
	rate_cfg.direct = nic_io->direct;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_SET_MAX_MIN_RATE,
				     &rate_cfg,
				     sizeof(rate_cfg), &rate_cfg, &out_size);
	if (err || !out_size || rate_cfg.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to set rate(%u), err: %d, status: 0x%x, out size: 0x%x\n",
			pf_rate, err, rate_cfg.msg_head.status, out_size);
		return rate_cfg.msg_head.status
					? rate_cfg.msg_head.status : -EIO;
	}

	return 0;
}

static int hinic3_get_nic_feature_from_hw(void *hwdev, u64 *s_feature, u16 size)
{
	return nic_feature_nego(hwdev, HINIC3_CMD_OP_GET, s_feature, size);
}

int hinic3_set_nic_feature_to_hw(void *hwdev)
{
	struct hinic3_nic_io *nic_io = NULL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);

	return nic_feature_nego(hwdev, HINIC3_CMD_OP_SET,
				&nic_io->feature_cap, 1);
}

u64 hinic3_get_feature_cap(void *hwdev)
{
	struct hinic3_nic_io *nic_io = NULL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return 0;

	return nic_io->feature_cap;
}

void hinic3_update_nic_feature(void *hwdev, u64 s_feature)
{
	struct hinic3_nic_io *nic_io = NULL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return;
	nic_io->feature_cap = s_feature;

	nic_info(nic_io->dev_hdl,
		 "Update nic feature to 0x%llx\n",
		 nic_io->feature_cap);
}

static inline int init_nic_hwdev_param_valid(const void *hwdev,
					     const void *pcidev_hdl,
					     const void *dev_hdl)
{
	if (!hwdev || !pcidev_hdl || !dev_hdl)
		return -EINVAL;

	return 0;
}

static int hinic3_init_nic_io(void *hwdev, void *pcidev_hdl, void *dev_hdl,
			      struct hinic3_nic_io **nic_io)
{
	if (init_nic_hwdev_param_valid(hwdev, pcidev_hdl, dev_hdl))
		return -EINVAL;

	*nic_io = kzalloc(sizeof(**nic_io), GFP_KERNEL);
	if (!(*nic_io))
		return -ENOMEM;

	(*nic_io)->dev_hdl = dev_hdl;
	(*nic_io)->pcidev_hdl = pcidev_hdl;
	(*nic_io)->hwdev = hwdev;

	sema_init(&((*nic_io)->nic_cfg.cfg_lock), 1);
	mutex_init(&((*nic_io)->nic_cfg.sfp_mutex));

	(*nic_io)->nic_cfg.rt_cmd.mpu_send_sfp_abs = false;
	(*nic_io)->nic_cfg.rt_cmd.mpu_send_sfp_info = false;
	(*nic_io)->nic_cfg.rt_cmd_ext.mpu_send_xsfp_tlv_info = false;

	return 0;
}

/* *
 * hinic3_init_nic_hwdev - init nic hwdev
 * @hwdev: pointer to hwdev
 * @pcidev_hdl: pointer to pcidev or handler
 * @dev_hdl: pointer to pcidev->dev or handler, for sdk_err() or dma_alloc()
 * @rx_buff_len: rx_buff_len is receive buffer length
 */
int hinic3_init_nic_hwdev(void *hwdev, void *pcidev_hdl, void *dev_hdl,
			  u16 rx_buff_len)
{
	struct hinic3_nic_io *nic_io = NULL;
	int err;
	int is_in_kexec = vram_get_kexec_flag();

	err = hinic3_init_nic_io(hwdev, pcidev_hdl, dev_hdl, &nic_io);
	if (err != 0)
		return err;

	nic_io->rx_buff_len = rx_buff_len;

	err = hinic3_register_service_adapter(hwdev, nic_io, SERVICE_T_NIC);
	if (err != 0) {
		nic_err(nic_io->dev_hdl, "Failed to register service adapter\n");
		goto register_sa_err;
	}

	err = hinic3_set_func_svc_used_state(hwdev, SVC_T_NIC,
					     1, HINIC3_CHANNEL_NIC);
	if (err != 0) {
		nic_err(nic_io->dev_hdl, "Failed to set function svc used state\n");
		goto set_used_state_err;
	}

	if (is_in_kexec == 0) {
		err = hinic3_init_function_table(nic_io);
		if (err != 0) {
			nic_err(nic_io->dev_hdl,
				"Failed to init function table\n");
			goto err_out;
		}
	}

	err = hinic3_get_nic_feature_from_hw(hwdev, &nic_io->feature_cap, 1);
	if (err) {
		nic_err(nic_io->dev_hdl, "Failed to get nic features\n");
		goto err_out;
	}

	sdk_info(dev_hdl, "nic features: 0x%llx\n", nic_io->feature_cap);
	hinic3_nic_cmdq_adapt_init(nic_io);

	err = hinic3_get_bios_pf_bw_limit(hwdev,
					  &nic_io->nic_cfg.pf_bw_tx_limit,
					  HINIC3_NIC_TX);
	if (err != 0) {
		nic_err(nic_io->dev_hdl, "Failed to get pf tx bandwidth limit\n");
		goto err_out;
	}

	err = hinic3_get_bios_pf_bw_limit(hwdev,
					  &nic_io->nic_cfg.pf_bw_rx_limit,
					  HINIC3_NIC_RX);
	if (err != 0) {
		nic_err(nic_io->dev_hdl, "Failed to get pf rx bandwidth limit\n");
		goto err_out;
	}

	err = hinic3_vf_func_init(nic_io);
	if (err != 0) {
		nic_err(nic_io->dev_hdl, "Failed to init vf info\n");
		goto err_out;
	}

	return 0;

err_out:
	if (hinic3_set_func_svc_used_state(hwdev, SVC_T_NIC,
					   0, HINIC3_CHANNEL_NIC) != 0) {
		nic_err(nic_io->dev_hdl, "Failed to set function svc used state\n");
	}

set_used_state_err:
	hinic3_unregister_service_adapter(hwdev, SERVICE_T_NIC);

register_sa_err:
	mutex_deinit(&nic_io->nic_cfg.sfp_mutex);
	sema_deinit(&nic_io->nic_cfg.cfg_lock);

	kfree(nic_io);

	return err;
}
EXPORT_SYMBOL(hinic3_init_nic_hwdev);

void hinic3_free_nic_hwdev(void *hwdev)
{
	struct hinic3_nic_io *nic_io = NULL;

	if (!hwdev)
		return;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return;

	hinic3_vf_func_free(nic_io);

	hinic3_set_func_svc_used_state(hwdev, SVC_T_NIC, 0, HINIC3_CHANNEL_NIC);

	hinic3_unregister_service_adapter(hwdev, SERVICE_T_NIC);

	mutex_deinit(&nic_io->nic_cfg.sfp_mutex);
	sema_deinit(&nic_io->nic_cfg.cfg_lock);

	kfree(nic_io);
}
EXPORT_SYMBOL(hinic3_free_nic_hwdev);

int hinic3_force_drop_tx_pkt(void *hwdev)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_force_pkt_drop pkt_drop;
	u16 out_size = sizeof(pkt_drop);
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&pkt_drop, 0, sizeof(pkt_drop));
	pkt_drop.port = hinic3_physical_port_id(hwdev);
	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_FORCE_PKT_DROP,
				     &pkt_drop, sizeof(pkt_drop),
				     &pkt_drop, &out_size);
	if ((pkt_drop.msg_head.status != HINIC3_MGMT_CMD_UNSUPPORTED &&
	     pkt_drop.msg_head.status) || err || !out_size) {
		nic_err(nic_io->dev_hdl,
			"Failed to set force tx packets drop, err: %d, status: 0x%x, out size: 0x%x\n",
			err, pkt_drop.msg_head.status, out_size);
		return -EFAULT;
	}

	return pkt_drop.msg_head.status;
}

int hinic3_set_rx_mode(void *hwdev, u32 enable)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_rx_mode_config rx_mode_cfg;
	u16 out_size = sizeof(rx_mode_cfg);
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&rx_mode_cfg, 0, sizeof(rx_mode_cfg));
	rx_mode_cfg.func_id = hinic3_global_func_id(hwdev);
	rx_mode_cfg.rx_mode = enable;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_SET_RX_MODE,
				     &rx_mode_cfg, sizeof(rx_mode_cfg),
				     &rx_mode_cfg, &out_size);
	if (err || !out_size || rx_mode_cfg.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to set rx mode, err: %d, status: 0x%x, out size: 0x%x\n",
			err, rx_mode_cfg.msg_head.status, out_size);
		return -EINVAL;
	}

	return 0;
}

int hinic3_set_rx_vlan_offload(void *hwdev, u8 en)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_cmd_vlan_offload vlan_cfg;
	u16 out_size = sizeof(vlan_cfg);
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&vlan_cfg, 0, sizeof(vlan_cfg));
	vlan_cfg.func_id = hinic3_global_func_id(hwdev);
	vlan_cfg.vlan_offload = en;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_SET_RX_VLAN_OFFLOAD,
				     &vlan_cfg, sizeof(vlan_cfg),
				     &vlan_cfg, &out_size);
	if (err || !out_size || vlan_cfg.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to set rx vlan offload, err: %d, status: 0x%x, out size: 0x%x\n",
			err, vlan_cfg.msg_head.status, out_size);
		return -EINVAL;
	}

	return 0;
}

int hinic3_update_mac_vlan(void *hwdev, const u16 old_vlan,
			   u16 new_vlan, int vf_id)
{
	struct vf_data_storage *vf_info = NULL;
	struct hinic3_nic_io *nic_io = NULL;
	u16 func_id;
	int err;

	if (!hwdev || old_vlan >= VLAN_N_VID || new_vlan >= VLAN_N_VID)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	vf_info = nic_io->vf_infos + HW_VF_ID_TO_OS(vf_id);
	if (!nic_io->vf_infos || is_zero_ether_addr(vf_info->drv_mac_addr))
		return 0;

	func_id = hinic3_glb_pf_vf_offset(nic_io->hwdev) + (u16)vf_id;

	err = hinic3_del_mac(nic_io->hwdev, vf_info->drv_mac_addr,
			     old_vlan, func_id, HINIC3_CHANNEL_NIC);
	if (err != 0) {
		nic_err(nic_io->dev_hdl, "Failed to delete VF %d MAC %pM vlan %u\n",
			HW_VF_ID_TO_OS(vf_id), vf_info->drv_mac_addr, old_vlan);
		return err;
	}

	err = hinic3_set_mac(nic_io->hwdev, vf_info->drv_mac_addr,
			     new_vlan, func_id, HINIC3_CHANNEL_NIC);
	if (err != 0) {
		nic_err(nic_io->dev_hdl, "Failed to add VF %d MAC %pM vlan %u\n",
			HW_VF_ID_TO_OS(vf_id), vf_info->drv_mac_addr, new_vlan);
		hinic3_set_mac(nic_io->hwdev, vf_info->drv_mac_addr,
			       old_vlan, func_id, HINIC3_CHANNEL_NIC);
		return err;
	}

	return 0;
}

static int hinic3_set_rx_lro(void *hwdev, u8 ipv4_en, u8 ipv6_en,
			     u8 lro_max_pkt_len)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_cmd_lro_config lro_cfg;
	u16 out_size = sizeof(lro_cfg);
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&lro_cfg, 0, sizeof(lro_cfg));
	lro_cfg.func_id = hinic3_global_func_id(hwdev);
	lro_cfg.opcode = HINIC3_CMD_OP_SET;
	lro_cfg.lro_ipv4_en = ipv4_en;
	lro_cfg.lro_ipv6_en = ipv6_en;
	lro_cfg.lro_max_pkt_len = lro_max_pkt_len;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_CFG_RX_LRO,
				     &lro_cfg, sizeof(lro_cfg),
				     &lro_cfg, &out_size);
	if (err || !out_size || lro_cfg.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to set lro offload, err: %d, status: 0x%x, out size: 0x%x\n",
			err, lro_cfg.msg_head.status, out_size);
		return -EINVAL;
	}

	return 0;
}

static int hinic3_set_rx_lro_timer(void *hwdev, u32 timer_value)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_cmd_lro_timer lro_timer;
	u16 out_size = sizeof(lro_timer);
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&lro_timer, 0, sizeof(lro_timer));
	lro_timer.opcode = HINIC3_CMD_OP_SET;
	lro_timer.timer = timer_value;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_CFG_LRO_TIMER,
				     &lro_timer, sizeof(lro_timer),
				     &lro_timer, &out_size);
	if (err || !out_size || lro_timer.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to set lro timer, err: %d, status: 0x%x, out size: 0x%x\n",
			err, lro_timer.msg_head.status, out_size);

		return -EINVAL;
	}

	return 0;
}

int hinic3_set_rx_lro_state(void *hwdev, u8 lro_en, u32 lro_timer,
			    u32 lro_max_pkt_len)
{
	struct hinic3_nic_io *nic_io = NULL;
	u8 ipv4_en = 0, ipv6_en = 0;
	int err;

	if (!hwdev)
		return -EINVAL;

	ipv4_en = lro_en ? 1 : 0;
	ipv6_en = lro_en ? 1 : 0;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	nic_info(nic_io->dev_hdl, "Set LRO max coalesce packet size to %uK\n",
		 lro_max_pkt_len);

	err = hinic3_set_rx_lro(hwdev, ipv4_en, ipv6_en, (u8)lro_max_pkt_len);
	if (err != 0)
		return err;

	/* we don't set LRO timer for VF */
	if (hinic3_func_type(hwdev) == TYPE_VF)
		return 0;

	nic_info(nic_io->dev_hdl, "Set LRO timer to %u\n", lro_timer);

	return hinic3_set_rx_lro_timer(hwdev, lro_timer);
}

int hinic3_set_vlan_fliter(void *hwdev, u32 vlan_filter_ctrl)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_cmd_set_vlan_filter vlan_filter;
	u16 out_size = sizeof(vlan_filter);
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	memset(&vlan_filter, 0, sizeof(vlan_filter));
	vlan_filter.func_id = hinic3_global_func_id(hwdev);
	vlan_filter.vlan_filter_ctrl = vlan_filter_ctrl;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_SET_VLAN_FILTER_EN,
				     &vlan_filter, sizeof(vlan_filter),
				     &vlan_filter, &out_size);
	if (err || !out_size || vlan_filter.msg_head.status) {
		nic_err(nic_io->dev_hdl, "Failed to set vlan filter, err: %d, status: 0x%x, out size: 0x%x\n",
			err, vlan_filter.msg_head.status, out_size);
		return -EINVAL;
	}

	return 0;
}

int hinic3_set_func_capture_en(void *hwdev, u16 func_id, bool cap_en)
{
	struct nic_cmd_capture_info cap_info = {{0}};
	u16 out_size = sizeof(cap_info);
	int err;

	if (!hwdev)
		return -EINVAL;

	/* 2 function capture types */
	cap_info.is_en_trx = cap_en;
	cap_info.func_port = func_id;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_SET_UCAPTURE_OPT,
				     &cap_info, sizeof(cap_info),
				     &cap_info, &out_size);
	if (err || !out_size || cap_info.msg_head.status)
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL(hinic3_set_func_capture_en);

int hinic3_add_tcam_rule(void *hwdev, struct nic_tcam_cfg_rule *tcam_rule)
{
	u16 out_size = sizeof(struct nic_cmd_fdir_add_rule);
	struct nic_cmd_fdir_add_rule tcam_cmd;
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !tcam_rule)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	if (tcam_rule->index >= HINIC3_MAX_TCAM_RULES_NUM) {
		nic_err(nic_io->dev_hdl, "Tcam rules num to add is invalid\n");
		return -EINVAL;
	}

	memset(&tcam_cmd, 0, sizeof(struct nic_cmd_fdir_add_rule));
	memcpy((void *)&tcam_cmd.rule, (void *)tcam_rule,
	       sizeof(struct nic_tcam_cfg_rule));
	tcam_cmd.func_id = hinic3_global_func_id(hwdev);
	tcam_cmd.type = TCAM_RULE_FDIR_TYPE;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_ADD_TC_FLOW,
				     &tcam_cmd, sizeof(tcam_cmd),
				     &tcam_cmd, &out_size);
	if (err || tcam_cmd.head.status || !out_size) {
		nic_err(nic_io->dev_hdl,
			"Add tcam rule failed, err: %d, status: 0x%x, out size: 0x%x\n",
			err, tcam_cmd.head.status, out_size);
		return -EIO;
	}

	return 0;
}

int hinic3_del_tcam_rule(void *hwdev, u32 index)
{
	u16 out_size = sizeof(struct nic_cmd_fdir_del_rules);
	struct nic_cmd_fdir_del_rules tcam_cmd;
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	if (index >= HINIC3_MAX_TCAM_RULES_NUM) {
		nic_err(nic_io->dev_hdl, "Tcam rules num to del is invalid\n");
		return -EINVAL;
	}

	memset(&tcam_cmd, 0, sizeof(struct nic_cmd_fdir_del_rules));
	tcam_cmd.index_start = index;
	tcam_cmd.index_num = 1;
	tcam_cmd.func_id = hinic3_global_func_id(hwdev);
	tcam_cmd.type = TCAM_RULE_FDIR_TYPE;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_DEL_TC_FLOW,
				     &tcam_cmd, sizeof(tcam_cmd),
				     &tcam_cmd, &out_size);
	if (err || tcam_cmd.head.status || !out_size) {
		nic_err(nic_io->dev_hdl,
			"Del tcam rule failed, err: %d, status: 0x%x, out size: 0x%x\n",
			err, tcam_cmd.head.status, out_size);
		return -EIO;
	}

	return 0;
}

/**
 * hinic3_mgmt_tcam_block - alloc or free tcam block for IO packet.
 *
 * @param hwdev
 *   The hardware interface of a nic device.
 * @param alloc_en
 *   1 alloc block.
 *   0 free block.
 * @param index
 *   block index from firmware.
 * @return
 *   0 on success,
 *   negative error value otherwise.
 */
static int hinic3_mgmt_tcam_block(void *hwdev, u8 alloc_en, u16 *index)
{
	struct nic_cmd_ctrl_tcam_block_out tcam_block_info;
	u16 out_size = sizeof(struct nic_cmd_ctrl_tcam_block_out);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !index)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	memset(&tcam_block_info, 0,
	       sizeof(struct nic_cmd_ctrl_tcam_block_out));

	tcam_block_info.func_id = hinic3_global_func_id(hwdev);
	tcam_block_info.alloc_en = alloc_en;
	tcam_block_info.tcam_type = NIC_TCAM_BLOCK_TYPE_LARGE;
	tcam_block_info.tcam_block_index = *index;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_CFG_TCAM_BLOCK,
				     &tcam_block_info, sizeof(tcam_block_info),
				     &tcam_block_info, &out_size);
	if (err || tcam_block_info.head.status || !out_size) {
		nic_err(nic_io->dev_hdl,
			"Set tcam block failed, err: %d, status: 0x%x, out size: 0x%x\n",
			err, tcam_block_info.head.status, out_size);
		return -EIO;
	}

	if (alloc_en)
		*index = tcam_block_info.tcam_block_index;

	return 0;
}

int hinic3_alloc_tcam_block(void *hwdev, u16 *index)
{
	return hinic3_mgmt_tcam_block(hwdev, HINIC3_TCAM_BLOCK_ENABLE, index);
}

int hinic3_free_tcam_block(void *hwdev, u16 *index)
{
	return hinic3_mgmt_tcam_block(hwdev, HINIC3_TCAM_BLOCK_DISABLE, index);
}

int hinic3_set_fdir_tcam_rule_filter(void *hwdev, bool enable)
{
	struct nic_cmd_set_tcam_enable port_tcam_cmd;
	u16 out_size = sizeof(port_tcam_cmd);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	memset(&port_tcam_cmd, 0, sizeof(port_tcam_cmd));
	port_tcam_cmd.func_id = hinic3_global_func_id(hwdev);
	port_tcam_cmd.tcam_enable = (u8)enable;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_ENABLE_TCAM,
				     &port_tcam_cmd, sizeof(port_tcam_cmd),
				     &port_tcam_cmd, &out_size);
	if (err || port_tcam_cmd.head.status || !out_size) {
		nic_err(nic_io->dev_hdl, "Set fdir tcam filter failed, err: %d, status: 0x%x, out size: 0x%x, enable: 0x%x\n",
			err, port_tcam_cmd.head.status, out_size,
			enable);
		return -EIO;
	}

	return 0;
}

int hinic3_flush_tcam_rule(void *hwdev)
{
	struct nic_cmd_flush_tcam_rules tcam_flush;
	u16 out_size = sizeof(struct nic_cmd_flush_tcam_rules);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	memset(&tcam_flush, 0, sizeof(struct nic_cmd_flush_tcam_rules));
	tcam_flush.func_id = hinic3_global_func_id(hwdev);

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_FLUSH_TCAM,
				     &tcam_flush,
				     sizeof(struct nic_cmd_flush_tcam_rules),
				     &tcam_flush, &out_size);
	if (err || tcam_flush.head.status || !out_size) {
		nic_err(nic_io->dev_hdl,
			"Flush tcam fdir rules failed, err: %d, status: 0x%x, out size: 0x%x\n",
			err, tcam_flush.head.status, out_size);
		return -EIO;
	}

	return 0;
}

int hinic3_get_rxq_hw_info(void *hwdev, struct rxq_check_info *rxq_info,
			   u16 num_qps, u16 wqe_type)
{
	struct hinic3_cmd_buf *cmd_buf = NULL;
	struct hinic3_nic_io *nic_io = NULL;
	struct hinic3_rxq_hw *rxq_hw = NULL;
	struct rxq_check_info *rxq_info_out = NULL;
	int err;
	u16 i;

	if (!hwdev || !rxq_info)
		return -EINVAL;

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;
	cmd_buf = hinic3_alloc_cmd_buf(hwdev);
	if (!cmd_buf) {
		nic_err(nic_io->dev_hdl, "Failed to allocate cmd_buf.\n");
		return -ENOMEM;
	}

	rxq_hw = cmd_buf->buf;
	rxq_hw->func_id = hinic3_global_func_id(hwdev);
	rxq_hw->num_queues = num_qps;

	hinic3_cpu_to_be32(rxq_hw, sizeof(struct hinic3_rxq_hw));

	cmd_buf->size = sizeof(struct hinic3_rxq_hw);

	err = hinic3_cmdq_detail_resp(hwdev, HINIC3_MOD_L2NIC,
				      HINIC3_UCODE_CMD_RXQ_INFO_GET,
				      cmd_buf, cmd_buf, NULL, 0,
				      HINIC3_CHANNEL_NIC);
	if (err != 0)
		goto get_rxq_info_failed;

	rxq_info_out = cmd_buf->buf;
	for (i = 0; i < num_qps; i++) {
		rxq_info[i].hw_pi = rxq_info_out[i].hw_pi >> wqe_type;
		rxq_info[i].hw_ci = rxq_info_out[i].hw_ci >> wqe_type;
	}

get_rxq_info_failed:
	hinic3_free_cmd_buf(hwdev, cmd_buf);

	return err;
}

int hinic3_pf_set_vf_link_state(void *hwdev, bool vf_link_forced,
				bool link_state)
{
	struct hinic3_nic_io *nic_io = NULL;
	struct vf_data_storage *vf_infos = NULL;
	int vf_id;

	if (!hwdev) {
		pr_err("hwdev is null.\n");
		return -EINVAL;
	}

	if (hinic3_func_type(hwdev) == TYPE_VF) {
		pr_err("VF are not supported to set link state.\n");
		return -EINVAL;
	}

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io) {
		pr_err("nic_io is null.\n");
		return -EINVAL;
	}

	vf_infos = nic_io->vf_infos;
	for (vf_id = 0; vf_id < nic_io->max_vfs; vf_id++) {
		vf_infos[vf_id].link_up = link_state;
		vf_infos[vf_id].link_forced = vf_link_forced;
	}

	return 0;
}
EXPORT_SYMBOL(hinic3_pf_set_vf_link_state);

int hinic3_get_outband_vlan_cfg(void *hwdev, u16 *outband_default_vid)
{
	struct hinic3_outband_cfg_info outband_cfg_info;
	u16 out_size = sizeof(outband_cfg_info);
	struct hinic3_nic_io *nic_io = NULL;
	int err;

	if (!hwdev || !outband_default_vid)
		return -EINVAL;

	memset(&outband_cfg_info, 0, sizeof(outband_cfg_info));

	nic_io = hinic3_get_service_adapter(hwdev, SERVICE_T_NIC);
	if (!nic_io)
		return -EINVAL;

	err = l2nic_msg_to_mgmt_sync(hwdev, HINIC3_NIC_CMD_GET_OUTBAND_CFG,
				     &outband_cfg_info,
				     sizeof(outband_cfg_info),
				     &outband_cfg_info, &out_size);
	if (err || !out_size || outband_cfg_info.msg_head.status) {
		nic_err(nic_io->dev_hdl,
			"Failed to get outband cfg, err: %d, status: 0x%x, out size: 0x%x\n",
			err, outband_cfg_info.msg_head.status, out_size);
		return -EINVAL;
	}

	*outband_default_vid = outband_cfg_info.outband_default_vid;

	return 0;
}

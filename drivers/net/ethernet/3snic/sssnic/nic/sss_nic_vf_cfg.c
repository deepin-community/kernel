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
#include "sss_nic_io_define.h"
#include "sss_nic_cfg_define.h"
#include "sss_nic_event.h"

static u8 vf_link_state;
module_param(vf_link_state, byte, 0444);
MODULE_PARM_DESC(vf_link_state,
		 "Set vf link state, 0 - link auto, 1 - always link up, 2 - always link down. - default is 0.");

/* In order to adapt different linux version */
enum {
	SSSNIC_IFLA_VF_LINK_STATE_AUTO,
	SSSNIC_IFLA_VF_LINK_STATE_ENABLE,
	SSSNIC_IFLA_VF_LINK_STATE_DISABLE,
	SSSNIC_IFLA_VF_LINK_STATE_MAX
};

#define SSSNIC_CVLAN_INSERT_ENABLE 0x1
#define SSSNIC_QINQ_INSERT_ENABLE  0X3

#define SSSNIC_GET_VLAN_TAG(vlan_id, qos) ((vlan_id) + (u16)((qos) << VLAN_PRIO_SHIFT))

typedef void (*sss_nic_link_vf_handler_t)(struct sss_nic_vf_info *);
typedef u8 (*sss_nic_link_state_handler_t)(struct sss_nic_io *nic_io, u16 vf_id);

static int sss_nic_set_vlan_mode(struct sss_nic_io *nic_io, u16 func_id,
				 u16 vlan_tag, u16 qid, u32 vlan_mode)
{
	int ret;
	u64 out_param = 0;
	struct sss_nic_vlan_ctx *vlan_ctx = NULL;
	struct sss_ctrl_msg_buf *msg_buf = NULL;

	msg_buf = sss_alloc_ctrlq_msg_buf(nic_io->hwdev);
	if (!msg_buf) {
		nic_err(nic_io->dev_hdl, "Fail to allocate send buf\n");
		return -ENOMEM;
	}

	msg_buf->size = sizeof(*vlan_ctx);
	vlan_ctx = (struct sss_nic_vlan_ctx *)msg_buf->buf;
	vlan_ctx->sel = 0; /* TPID0 in IPSU */
	vlan_ctx->func_id = func_id;
	vlan_ctx->mode = vlan_mode;
	vlan_ctx->qid = qid;
	vlan_ctx->tag = vlan_tag;

	sss_cpu_to_be32(vlan_ctx, sizeof(*vlan_ctx));

	ret = sss_ctrlq_direct_reply(nic_io->hwdev, SSS_MOD_TYPE_L2NIC,
				     SSSNIC_CTRLQ_OPCODE_MODIFY_VLAN_CTX, msg_buf,
				     &out_param, 0, SSS_CHANNEL_NIC);
	if (ret != 0 || out_param != 0) {
		nic_err(nic_io->dev_hdl, "Fail to set vlan ctx, ret: %d, out_param: 0x%llx\n",
			ret, out_param);
		sss_free_ctrlq_msg_buf(nic_io->hwdev, msg_buf);
		return -EFAULT;
	}

	sss_free_ctrlq_msg_buf(nic_io->hwdev, msg_buf);

	return 0;
}

int sss_nic_set_vf_vlan(struct sss_nic_io *nic_io, u8 opcode, u16 vlan_id, u8 qos, int vf_id)
{
	int ret;
	u32 vlan_mode;
	u16 os_id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	u16 vlan_tag = SSSNIC_GET_VLAN_TAG(vlan_id, qos);
	u16 func_id = sss_get_glb_pf_vf_offset(nic_io->hwdev) + (u16)vf_id;
	struct sss_nic_mbx_vf_vlan_cfg cmd_config_info = {0};
	u16 out_len = sizeof(cmd_config_info);

	if (vlan_id == 0 && opcode == SSSNIC_MBX_OPCODE_DEL)
		return 0;

	cmd_config_info.vlan_id = vlan_id;
	cmd_config_info.func_id = func_id;
	cmd_config_info.opcode = opcode;
	cmd_config_info.qos = qos;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev, SSSNIC_MBX_OPCODE_CFG_VF_VLAN,
					     &cmd_config_info, sizeof(cmd_config_info),
					     &cmd_config_info, &out_len);
	if (ret != 0 || out_len == 0 || cmd_config_info.head.state != SSS_MGMT_CMD_SUCCESS) {
		nic_err(nic_io->dev_hdl,
			"Fail to set VF %d vlan, ret: %d, status: 0x%x, out_len: 0x%x\n",
			os_id, ret, cmd_config_info.head.state, out_len);
		return -EFAULT;
	}

	vlan_mode = (opcode == SSSNIC_MBX_OPCODE_ADD) ?
		    SSSNIC_QINQ_INSERT_ENABLE : SSSNIC_CVLAN_INSERT_ENABLE;

	ret = sss_nic_set_vlan_mode(nic_io, func_id, vlan_tag,
				    SSSNIC_CONFIG_ALL_QUEUE_VLAN_CTX, vlan_mode);
	if (ret != 0) {
		cmd_config_info.opcode = (opcode == SSSNIC_MBX_OPCODE_DEL) ?
					 SSSNIC_MBX_OPCODE_ADD : SSSNIC_MBX_OPCODE_DEL;
		sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev, SSSNIC_MBX_OPCODE_CFG_VF_VLAN,
					       &cmd_config_info, sizeof(cmd_config_info),
					       &cmd_config_info, &out_len);
		nic_err(nic_io->dev_hdl,
			"Fail to set VF %d vlan context, ret: %d\n", os_id, ret);
	}

	return ret;
}

int sss_nic_create_vf_vlan(struct sss_nic_io *nic_io, int vf_id, u16 vlan, u8 qos)
{
	int ret;
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);

	ret = sss_nic_set_vf_vlan(nic_io, SSSNIC_MBX_OPCODE_ADD, vlan, qos, vf_id);
	if (ret != 0)
		return ret;

	nic_io->vf_info_group[id].pf_qos = qos;
	nic_io->vf_info_group[id].pf_vlan = vlan;

	nic_info(nic_io->dev_hdl, "Add vf vlan VLAN %u, QOS 0x%x on VF %d\n",
		 vlan, qos, id);

	return 0;
}

int sss_nic_destroy_vf_vlan(struct sss_nic_io *nic_io, int vf_id)
{
	int ret;
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	struct sss_nic_vf_info *vf_info_group;

	vf_info_group = nic_io->vf_info_group;

	ret = sss_nic_set_vf_vlan(nic_io, SSSNIC_MBX_OPCODE_DEL,
				  vf_info_group[id].pf_vlan,
				  vf_info_group[id].pf_qos, vf_id);
	if (ret != 0)
		return ret;

	nic_info(nic_io->dev_hdl, "Kill vf VLAN %u on VF %d\n",
		 vf_info_group[id].pf_vlan, id);

	vf_info_group[id].pf_qos = 0;
	vf_info_group[id].pf_vlan = 0;

	return 0;
}

u16 sss_nic_vf_info_vlan_prio(struct sss_nic_io *nic_io, int vf_id)
{
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	u16 vlan_prio;
	u16 pf_vlan;
	u8 pf_qos;

	pf_vlan = nic_io->vf_info_group[id].pf_vlan;
	pf_qos = nic_io->vf_info_group[id].pf_qos;

	vlan_prio = SSSNIC_GET_VLAN_PRIO(pf_vlan, pf_qos);

	return vlan_prio;
}

static u8 sss_nic_ifla_vf_link_state_auto(struct sss_nic_io *nic_io, u16 id)
{
	nic_io->vf_info_group[id].link_forced = false;
	nic_io->vf_info_group[id].link_up = !!nic_io->link_status;

	return nic_io->link_status;
}

static u8 sss_nic_ifla_vf_link_state_enable(struct sss_nic_io *nic_io, u16 id)
{
	nic_io->vf_info_group[id].link_forced = true;
	nic_io->vf_info_group[id].link_up = true;

	return SSSNIC_LINK_UP;
}

static u8 sss_nic_ifla_vf_link_state_disable(struct sss_nic_io *nic_io, u16 id)
{
	nic_io->vf_info_group[id].link_forced = true;
	nic_io->vf_info_group[id].link_up = false;

	return SSSNIC_LINK_DOWN;
}

int sss_nic_set_vf_link_state(struct sss_nic_io *nic_io, u16 vf_id, int link)
{
	u8 link_status = 0;
	struct sss_nic_vf_info *vf_info = NULL;

	sss_nic_link_state_handler_t handler[SSSNIC_IFLA_VF_LINK_STATE_MAX] = {
		sss_nic_ifla_vf_link_state_auto,
		sss_nic_ifla_vf_link_state_enable,
		sss_nic_ifla_vf_link_state_disable,
	};

	if (link >= SSSNIC_IFLA_VF_LINK_STATE_MAX)
		return -EINVAL;

	if (handler[link])
		link_status = handler[link](nic_io, SSSNIC_HW_VF_ID_TO_OS(vf_id));

	/* Notify the VF of its new link state */
	vf_info = &nic_io->vf_info_group[SSSNIC_HW_VF_ID_TO_OS(vf_id)];
	if (vf_info->attach)
		sss_nic_notify_vf_link_state(nic_io, vf_id, link_status);

	return 0;
}

int sss_nic_set_vf_spoofchk(struct sss_nic_io *nic_io, u16 vf_id, bool spoofchk)
{
	int ret;
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	struct sss_nic_vf_info *vf_info = NULL;
	struct sss_nic_mbx_set_spoofchk cmd_spoofchk_cfg = {0};
	u16 out_len = sizeof(cmd_spoofchk_cfg);

	cmd_spoofchk_cfg.func_id = sss_get_glb_pf_vf_offset(nic_io->hwdev) + vf_id;
	cmd_spoofchk_cfg.state = !!spoofchk;
	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev, SSSNIC_MBX_OPCODE_SET_SPOOPCHK_STATE,
					     &cmd_spoofchk_cfg,
					     sizeof(cmd_spoofchk_cfg), &cmd_spoofchk_cfg,
					     &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_spoofchk_cfg)) {
		nic_err(nic_io->dev_hdl, "Fail to set VF(%d) spoofchk, ret: %d, status: 0x%x, out_len: 0x%x\n",
			id, ret, cmd_spoofchk_cfg.head.state, out_len);
		ret = -EINVAL;
	}

	vf_info = nic_io->vf_info_group;
	vf_info[id].spoofchk = !!spoofchk;

	return ret;
}

#ifdef HAVE_NDO_SET_VF_TRUST
int sss_nic_set_vf_trust(struct sss_nic_io *nic_io, u16 vf_id, bool trust)
{
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);

	if (vf_id > nic_io->max_vf_num)
		return -EINVAL;

	nic_io->vf_info_group[id].trust = !!trust;

	return 0;
}

bool sss_nic_get_vf_trust(struct sss_nic_io *nic_io, int vf_id)
{
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);

	if (vf_id > nic_io->max_vf_num)
		return -EINVAL;

	return !!nic_io->vf_info_group[id].trust;
}
#endif

int sss_nic_set_vf_tx_rate_limit(struct sss_nic_io *nic_io, u16 vf_id, u32 min_rate, u32 max_rate)
{
	int ret;
	u16 id = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	struct sss_nic_mbx_tx_rate_cfg cmd_cfg = {0};
	u16 out_len = sizeof(cmd_cfg);

	cmd_cfg.min_rate = min_rate;
	cmd_cfg.max_rate = max_rate;
	cmd_cfg.func_id = sss_get_glb_pf_vf_offset(nic_io->hwdev) + vf_id;
	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev, SSSNIC_MBX_OPCODE_SET_MAX_MIN_RATE,
					     &cmd_cfg, sizeof(cmd_cfg), &cmd_cfg, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_cfg)) {
		nic_err(nic_io->dev_hdl,
			"Fail to set VF %d max_rate %u, min_rate %u, ret: %d, status: 0x%x, out_len: 0x%x\n",
			id, max_rate, min_rate, ret, cmd_cfg.head.state,
			out_len);
		return -EIO;
	}

	nic_io->vf_info_group[id].max_rate = max_rate;
	nic_io->vf_info_group[id].min_rate = min_rate;

	return 0;
}

void sss_nic_get_vf_attribute(struct sss_nic_io *nic_io, u16 vf_id,
			      struct ifla_vf_info *ifla_vf)
{
	struct sss_nic_vf_info *vf_info;

	vf_info = nic_io->vf_info_group + SSSNIC_HW_VF_ID_TO_OS(vf_id);

	ether_addr_copy(ifla_vf->mac, vf_info->user_mac);
	ifla_vf->vf = SSSNIC_HW_VF_ID_TO_OS(vf_id);
	ifla_vf->qos = vf_info->pf_qos;
	ifla_vf->vlan = vf_info->pf_vlan;

#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
	ifla_vf->spoofchk = vf_info->spoofchk;
#endif

#ifdef HAVE_NDO_SET_VF_TRUST
	ifla_vf->trusted = vf_info->trust;
#endif

#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
	ifla_vf->min_tx_rate = vf_info->min_rate;
	ifla_vf->max_tx_rate = vf_info->max_rate;
#else
	ifla_vf->tx_rate = vf_info->max_rate;
#endif /* HAVE_NDO_SET_VF_MIN_MAX_TX_RATE */

#ifdef HAVE_NDO_SET_VF_LINK_STATE
	if (!vf_info->link_forced)
		ifla_vf->linkstate = IFLA_VF_LINK_STATE_AUTO;
	else if (vf_info->link_up)
		ifla_vf->linkstate = IFLA_VF_LINK_STATE_ENABLE;
	else
		ifla_vf->linkstate = IFLA_VF_LINK_STATE_DISABLE;
#endif
}

static void sss_nic_init_link_disable_vf(struct sss_nic_vf_info *vf_info)
{
	vf_info->link_forced = true;
	vf_info->link_up = false;
}

static void sss_nic_init_link_enable_vf(struct sss_nic_vf_info *vf_info)
{
	vf_info->link_forced = true;
	vf_info->link_up = true;
}

static void sss_nic_init_link_auto_vf(struct sss_nic_vf_info *vf_info)
{
	vf_info->link_forced = false;
}

static int sss_nic_init_vf_info(struct sss_nic_io *nic_io, u16 vf_id)
{
	u8 link_state;
	struct sss_nic_vf_info *vf_info_group = nic_io->vf_info_group;
	sss_nic_link_vf_handler_t handler[SSSNIC_IFLA_VF_LINK_STATE_MAX] = {
		sss_nic_init_link_auto_vf,
		sss_nic_init_link_enable_vf,
		sss_nic_init_link_disable_vf
	};

	if (vf_link_state >= SSSNIC_IFLA_VF_LINK_STATE_MAX) {
		vf_link_state = SSSNIC_IFLA_VF_LINK_STATE_AUTO;
		nic_warn(nic_io->dev_hdl, "Invalid vf_link_state: %u out of range[%u - %u], adjust to %d\n",
			 vf_link_state, SSSNIC_IFLA_VF_LINK_STATE_AUTO,
			 SSSNIC_IFLA_VF_LINK_STATE_DISABLE, SSSNIC_IFLA_VF_LINK_STATE_AUTO);
	}

	link_state = vf_link_state;
	if (link_state < SSSNIC_IFLA_VF_LINK_STATE_MAX) {
		handler[link_state](&vf_info_group[vf_id]);
	} else {
		nic_err(nic_io->dev_hdl, "Fail to input vf_link_state: %u\n",
			link_state);
		return -EINVAL;
	}

	return 0;
}

static int sss_nic_register_vf_to_hw(struct sss_nic_io *nic_io)
{
	u16 out_len;
	int ret;
	struct sss_nic_mbx_attach_vf cmd_register_info = {0};

	cmd_register_info.op_register = 1;
	out_len = sizeof(cmd_register_info);
	ret = sss_mbx_send_to_pf(nic_io->hwdev, SSS_MOD_TYPE_L2NIC,
				 SSSNIC_MBX_OPCODE_VF_REGISTER,
				 &cmd_register_info, sizeof(cmd_register_info),
				 &cmd_register_info, &out_len, 0,
				 SSS_CHANNEL_NIC);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_register_info)) {
		nic_err(nic_io->dev_hdl, "Fail to register VF, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_register_info.head.state, out_len);
		return -EIO;
	}

	return 0;
}

static void sss_nic_unregister_vf_to_hw(struct sss_nic_io *nic_io)
{
	int ret;
	struct sss_nic_mbx_attach_vf cmd_register_info = {0};
	u16 out_len = sizeof(cmd_register_info);

	cmd_register_info.op_register = 0;

	ret = sss_mbx_send_to_pf(nic_io->hwdev, SSS_MOD_TYPE_L2NIC, SSSNIC_MBX_OPCODE_VF_REGISTER,
				 &cmd_register_info, sizeof(cmd_register_info), &cmd_register_info,
				 &out_len, 0, SSS_CHANNEL_NIC);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_register_info))
		nic_err(nic_io->dev_hdl,
			"Fail to unregister VF, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_register_info.head.state, out_len);
}

static void sss_nic_vf_unregister(struct sss_nic_io *nic_io)
{
	sss_nic_unregister_vf_to_hw(nic_io);
	sss_unregister_vf_mbx_handler(nic_io->hwdev, SSS_MOD_TYPE_SSSLINK);
	sss_unregister_vf_mbx_handler(nic_io->hwdev, SSS_MOD_TYPE_L2NIC);
}

static int sss_nic_vf_register(struct sss_nic_io *nic_io)
{
	int ret;

	ret = sss_register_vf_mbx_handler(nic_io->hwdev, SSS_MOD_TYPE_L2NIC,
					  nic_io->hwdev, sss_nic_vf_event_handler);
	if (ret != 0)
		return ret;

	ret = sss_register_vf_mbx_handler(nic_io->hwdev, SSS_MOD_TYPE_SSSLINK,
					  nic_io->hwdev, sss_nic_vf_mag_event_handler);
	if (ret != 0)
		goto reg_cb_error;

	ret = sss_nic_register_vf_to_hw(nic_io);
	if (ret != 0)
		goto register_vf_error;

	return 0;

register_vf_error:
	sss_unregister_vf_mbx_handler(nic_io->hwdev, SSS_MOD_TYPE_SSSLINK);

reg_cb_error:
	sss_unregister_vf_mbx_handler(nic_io->hwdev, SSS_MOD_TYPE_L2NIC);

	return ret;
}

void sss_nic_deinit_pf_vf_info(struct sss_nic_io *nic_io)
{
	if (sss_get_func_type(nic_io->hwdev) == SSS_FUNC_TYPE_VF)
		return;
	kfree(nic_io->vf_info_group);
	nic_io->vf_info_group = NULL;
}

int sss_nic_init_pf_vf_info(struct sss_nic_io *nic_io)
{
	u16 i;
	int ret;
	u32 len;

	if (sss_get_func_type(nic_io->hwdev) == SSS_FUNC_TYPE_VF)
		return 0;

	nic_io->max_vf_num = sss_get_max_vf_num(nic_io->hwdev);
	if (nic_io->max_vf_num == 0)
		return 0;

	len = sizeof(*nic_io->vf_info_group) * nic_io->max_vf_num;
	nic_io->vf_info_group = kzalloc(len, GFP_KERNEL);
	if (!nic_io->vf_info_group)
		return -ENOMEM;

	for (i = 0; i < nic_io->max_vf_num; i++) {
		ret = sss_nic_init_vf_info(nic_io, i);
		if (ret != 0)
			goto init_vf_info_error;
	}

	return 0;

init_vf_info_error:
	kfree(nic_io->vf_info_group);
	nic_io->vf_info_group = NULL;

	return ret;
}

int sss_nic_register_io_callback(struct sss_nic_io *nic_io)
{
	int ret;

	if (sss_get_func_type(nic_io->hwdev) == SSS_FUNC_TYPE_VF)
		return sss_nic_vf_register(nic_io);

	ret = sss_register_mgmt_msg_handler(nic_io->hwdev, SSS_MOD_TYPE_L2NIC,
					    nic_io->hwdev, sss_nic_pf_event_handler);
	if (ret != 0)
		return ret;

	ret = sss_register_mgmt_msg_handler(nic_io->hwdev, SSS_MOD_TYPE_SSSLINK,
					    nic_io->hwdev, sss_nic_pf_mag_event_handler);
	if (ret != 0)
		goto register_pf_mag_event_handler;

	ret = sss_register_pf_mbx_handler(nic_io->hwdev, SSS_MOD_TYPE_L2NIC,
					  nic_io->hwdev, sss_nic_pf_mbx_handler);
	if (ret != 0)
		goto register_pf_mbx_cb_error;

	ret = sss_register_pf_mbx_handler(nic_io->hwdev, SSS_MOD_TYPE_SSSLINK,
					  nic_io->hwdev, sss_nic_pf_mag_mbx_handler);
	if (ret != 0)
		goto register_pf_mag_mbx_cb_error;

	return 0;

register_pf_mag_mbx_cb_error:
	sss_unregister_pf_mbx_handler(nic_io->hwdev, SSS_MOD_TYPE_L2NIC);

register_pf_mbx_cb_error:
	sss_unregister_mgmt_msg_handler(nic_io->hwdev, SSS_MOD_TYPE_SSSLINK);

register_pf_mag_event_handler:
	sss_unregister_mgmt_msg_handler(nic_io->hwdev, SSS_MOD_TYPE_L2NIC);

	return ret;
}

void sss_nic_unregister_io_callback(struct sss_nic_io *nic_io)
{
	if (sss_get_func_type(nic_io->hwdev) == SSS_FUNC_TYPE_VF) {
		sss_nic_vf_unregister(nic_io);
	} else {
		if (nic_io->vf_info_group) {
			sss_unregister_pf_mbx_handler(nic_io->hwdev, SSS_MOD_TYPE_SSSLINK);
			sss_unregister_pf_mbx_handler(nic_io->hwdev, SSS_MOD_TYPE_L2NIC);
		}
		sss_unregister_mgmt_msg_handler(nic_io->hwdev, SSS_MOD_TYPE_SSSLINK);
		sss_unregister_mgmt_msg_handler(nic_io->hwdev, SSS_MOD_TYPE_L2NIC);
	}
}

static void sss_nic_clear_vf_info(struct sss_nic_io *nic_io, u16 vf_id)
{
	u16 func_id;
	struct sss_nic_vf_info *vf_info;

	func_id = sss_get_glb_pf_vf_offset(nic_io->hwdev) + vf_id;
	vf_info = nic_io->vf_info_group + SSSNIC_HW_VF_ID_TO_OS(vf_id);
	if (vf_info->specified_mac)
		sss_nic_del_mac(nic_io->nic_dev, vf_info->drv_mac,
				vf_info->pf_vlan, func_id, SSS_CHANNEL_NIC);

	if (sss_nic_vf_info_vlan_prio(nic_io, vf_id))
		sss_nic_destroy_vf_vlan(nic_io, vf_id);

	if (vf_info->max_rate && SSSNIC_SUPPORT_RATE_LIMIT(nic_io))
		sss_nic_set_vf_tx_rate_limit(nic_io, vf_id, 0, 0);

	if (vf_info->spoofchk)
		sss_nic_set_vf_spoofchk(nic_io, vf_id, false);

#ifdef HAVE_NDO_SET_VF_TRUST
	if (vf_info->trust)
		sss_nic_set_vf_trust(nic_io, vf_id, false);
#endif

	memset(vf_info, 0, sizeof(*vf_info));
	sss_nic_init_vf_info(nic_io, SSSNIC_HW_VF_ID_TO_OS(vf_id));
}

void sss_nic_clear_all_vf_info(struct sss_nic_io *nic_io)
{
	u16 i;

	for (i = 0; i < nic_io->max_vf_num; i++)
		sss_nic_clear_vf_info(nic_io, SSSNIC_OS_VF_ID_TO_HW(i));
}

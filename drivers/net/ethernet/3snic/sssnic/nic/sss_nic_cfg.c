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
#include "sss_nic_io_define.h"
#include "sss_nic_cfg_define.h"
#include "sss_nic_cfg.h"
#include "sss_nic_vf_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_rss_cfg.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_event.h"

#define SSSNIC_DEFAULT_MAX_MTU		0x3FFF
#define SSSNIC_VLAN_ID_MASK			0x7FFF

#define SSSNIC_INIT_FUNC_MASK		\
			(BIT(SSSNIC_FUNC_CFG_TYPE_INIT) | \
			BIT(SSSNIC_FUNC_CFG_TYPE_MTU) | \
			BIT(SSSNIC_FUNC_CFG_TYPE_RX_BUF_SIZE))

#define SSSNIC_MGMT_STATUS_EXIST	0x6

#define SSSNIC_CHECK_IPSU_15BIT		0x8000

#define SSSNIC_DCB_PCP				0
#define SSSNIC_DCB_DSCP				1

#define SSSNIC_F_ALL_MASK			0x3FFFF /* enable all feature */
#define SSSNIC_DRV_DEFAULT_FEATURE	SSSNIC_F_ALL_MASK

#define SSSNIC_UNSUPPORT_SET_PAUSE	0x10

#define SSSNIC_VF_SET_MAC_ALREADY(func_type, status)	\
		((func_type) == SSS_FUNC_TYPE_VF && (status) == SSSNIC_PF_SET_VF_ALREADY)

static int sss_nic_check_mac_set_status(u32 func_type, u8 status, u16 vlan_id)
{
	if (status != 0 && status != SSSNIC_MGMT_STATUS_EXIST) {
		if (!SSSNIC_VF_SET_MAC_ALREADY(func_type, status))
			return -EINVAL;
	}

	if ((vlan_id & SSSNIC_CHECK_IPSU_15BIT) != 0 && status == SSSNIC_MGMT_STATUS_EXIST) {
		if (!SSSNIC_VF_SET_MAC_ALREADY(func_type, status))
			return -EINVAL;
	}

	return 0;
}

int sss_nic_set_mac(struct sss_nic_dev *nic_dev, const u8 *mac_addr,
		    u16 vlan_id, u16 func_id, u16 channel)
{
	struct sss_nic_mbx_mac_addr cmd_mac = {0};
	u16 out_len = sizeof(cmd_mac);
	u32 func_type;
	int ret;

	if (!nic_dev || !mac_addr)
		return -EINVAL;

	if ((vlan_id & SSSNIC_VLAN_ID_MASK) >= VLAN_N_VID) {
		nic_err(nic_dev->dev_hdl, "Invalid VLAN ID: %d\n", (vlan_id & SSSNIC_VLAN_ID_MASK));
		return -EINVAL;
	}

	cmd_mac.vlan_id = vlan_id;
	cmd_mac.func_id = func_id;
	ether_addr_copy(cmd_mac.mac, mac_addr);

	ret = sss_nic_l2nic_msg_to_mgmt_sync_ch(nic_dev->hwdev, SSSNIC_MBX_OPCODE_SET_MAC,
						&cmd_mac, sizeof(cmd_mac),
						&cmd_mac, &out_len, channel);
	if (ret != 0 || out_len == 0) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set MAC, ret: %d, out_len: 0x%x, channel: 0x%x\n",
			ret, out_len, channel);
		return -EIO;
	}

	func_type = sss_get_func_type(nic_dev->hwdev);
	if (sss_nic_check_mac_set_status(func_type, cmd_mac.head.state, cmd_mac.vlan_id) != 0) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set MAC, state: 0x%x, channel: 0x%x\n",
			cmd_mac.head.state, channel);
		return -EIO;
	}

	if (SSSNIC_VF_SET_MAC_ALREADY(func_type, cmd_mac.head.state)) {
		nic_warn(nic_dev->dev_hdl,
			 "PF has already set VF mac, ignore it\n");
		return SSSNIC_PF_SET_VF_ALREADY;
	}

	if (cmd_mac.head.state == SSSNIC_MGMT_STATUS_EXIST) {
		nic_warn(nic_dev->dev_hdl, "Repeat mac, ignore it\n");
		return 0;
	}

	return 0;
}

int sss_nic_del_mac(struct sss_nic_dev *nic_dev, const u8 *mac_addr,
		    u16 vlan_id, u16 func_id, u16 channel)
{
	struct sss_nic_mbx_mac_addr cmd_mac = {0};
	u16 out_len = sizeof(cmd_mac);
	u32 func_type;
	int ret;

	if (!nic_dev || !mac_addr)
		return -EINVAL;

	if ((vlan_id & SSSNIC_VLAN_ID_MASK) >= VLAN_N_VID) {
		nic_err(nic_dev->dev_hdl, "Invalid VLAN number: %d\n",
			(vlan_id & SSSNIC_VLAN_ID_MASK));
		return -EINVAL;
	}

	cmd_mac.func_id = func_id;
	cmd_mac.vlan_id = vlan_id;
	ether_addr_copy(cmd_mac.mac, mac_addr);

	ret = sss_nic_l2nic_msg_to_mgmt_sync_ch(nic_dev->hwdev, SSSNIC_MBX_OPCODE_DEL_MAC,
						&cmd_mac, sizeof(cmd_mac), &cmd_mac,
						&out_len, channel);
	if (ret != 0 || out_len == 0) {
		nic_err(nic_dev->dev_hdl,
			"Fail to del MAC, ret: %d, out_len: 0x%x, channel: 0x%x\n",
			ret, out_len, channel);
		return -EIO;
	}

	func_type = sss_get_func_type(nic_dev->hwdev);
	if (SSSNIC_VF_SET_MAC_ALREADY(func_type, cmd_mac.head.state)) {
		nic_warn(nic_dev->dev_hdl, "PF has already set VF mac\n");
		return SSSNIC_PF_SET_VF_ALREADY;
	}

	if (cmd_mac.head.state != 0) {
		nic_err(nic_dev->dev_hdl,
			"Fail to delete MAC, ret: %d, state: 0x%x, channel: 0x%x\n",
			ret, cmd_mac.head.state, channel);
		return -EIO;
	}

	return 0;
}

int sss_nic_update_mac(struct sss_nic_dev *nic_dev, u8 *new_mac)
{
	int ret;
	u32 func_type;
	struct sss_nic_mbx_mac_update cmd_mac_update = {0};
	u16 out_len = sizeof(cmd_mac_update);

	ether_addr_copy(cmd_mac_update.new_mac, new_mac);
	ether_addr_copy(cmd_mac_update.old_mac.mac, nic_dev->netdev->dev_addr);
	cmd_mac_update.old_mac.func_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_UPDATE_MAC,
					     &cmd_mac_update, sizeof(cmd_mac_update),
					     &cmd_mac_update, &out_len);

	if (ret != 0 || out_len == 0) {
		nic_err(nic_dev->dev_hdl,
			"Fail to update MAC, ret: %d, out_len: 0x%x\n", ret, out_len);
		return -EIO;
	}

	func_type = sss_get_func_type(nic_dev->hwdev);
	if (sss_nic_check_mac_set_status(func_type, cmd_mac_update.old_mac.head.state,
					 cmd_mac_update.old_mac.vlan_id)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to update MAC, state: 0x%x", cmd_mac_update.old_mac.head.state);
		return -EIO;
	}

	if (SSSNIC_VF_SET_MAC_ALREADY(func_type, cmd_mac_update.old_mac.head.state)) {
		nic_warn(nic_dev->dev_hdl,
			 "PF has already set VF MAC. Ignore update\n");
		return SSSNIC_PF_SET_VF_ALREADY;
	}

	if (cmd_mac_update.old_mac.head.state == SSSNIC_MGMT_STATUS_EXIST)
		nic_warn(nic_dev->dev_hdl,
			 "MAC is existed. Ignore update\n");

	return 0;
}

int sss_nic_get_default_mac(struct sss_nic_dev *nic_dev, u8 *mac_addr)
{
	struct sss_nic_mbx_mac_addr cmd_mac = {0};
	u16 out_len = sizeof(cmd_mac);
	int ret;

	cmd_mac.func_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_GET_MAC,
					     &cmd_mac, sizeof(cmd_mac), &cmd_mac, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_mac)) {
		nic_err(nic_dev->hwdev,
			"Fail to get mac, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_mac.head.state, out_len);
		return -EINVAL;
	}

	ether_addr_copy(mac_addr, cmd_mac.mac);

	return 0;
}

int sss_nic_config_vlan(struct sss_nic_dev *nic_dev, u8 opcode, u16 vlan_id)
{
	struct sss_nic_mbx_vlan_cfg cmd_config_vlan = {0};
	u16 out_len = sizeof(cmd_config_vlan);
	int ret;

	cmd_config_vlan.func_id =
		sss_get_global_func_id(nic_dev->hwdev);
	cmd_config_vlan.opcode = opcode;
	cmd_config_vlan.vlan_id = vlan_id;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev,
					     SSSNIC_MBX_OPCODE_CFG_FUNC_VLAN,
					     &cmd_config_vlan, sizeof(cmd_config_vlan),
					     &cmd_config_vlan, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_config_vlan)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to %s vlan, ret: %d, state: 0x%x, out_len: 0x%x\n",
			opcode == SSSNIC_MBX_OPCODE_ADD ? "add" : "delete",
			ret, cmd_config_vlan.head.state, out_len);
		return -EINVAL;
	}

	return 0;
}

int sss_nic_set_hw_vport_state(struct sss_nic_dev *nic_dev,
			       u16 func_id, bool enable, u16 channel)
{
	struct sss_nic_mbx_vport_state cmd_set_vport_state = {0};
	u16 out_len = sizeof(cmd_set_vport_state);
	int ret;

	cmd_set_vport_state.func_id = func_id;
	cmd_set_vport_state.state = enable ? 1 : 0;

	ret = sss_nic_l2nic_msg_to_mgmt_sync_ch(nic_dev->hwdev, SSSNIC_MBX_OPCODE_SET_VPORT_ENABLE,
						&cmd_set_vport_state, sizeof(cmd_set_vport_state),
						&cmd_set_vport_state, &out_len, channel);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_set_vport_state)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set vport state, ret: %d, state: 0x%x, out_len: 0x%x, channel: 0x%x\n",
			ret, cmd_set_vport_state.head.state, out_len, channel);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(sss_nic_set_hw_vport_state);

int sss_nic_set_dcb_info(struct sss_nic_io *nic_io,
			 struct sss_nic_dcb_info *dcb_info)
{
	if (memcmp(&nic_io->dcb_info, dcb_info, sizeof(*dcb_info)) == 0)
		return 0;

	memcpy(&nic_io->dcb_info, dcb_info, sizeof(*dcb_info));

	/* notify stateful in pf, than notify all vf */
	sss_nic_notify_dcb_state_event(nic_io->hwdev, dcb_info);

	return 0;
}

static int sss_nic_cfg_hw_pause(struct sss_nic_dev *nic_dev,
				u8 opcode, struct sss_nic_pause_cfg *pause_cfg)
{
	struct sss_nic_mbx_pause_cfg cmd_pause_cfg = {0};
	u16 out_len = sizeof(cmd_pause_cfg);
	int ret;

	cmd_pause_cfg.port_id = sss_get_phy_port_id(nic_dev->hwdev);
	cmd_pause_cfg.opcode = opcode;
	if (opcode == SSSNIC_MBX_OPCODE_SET) {
		cmd_pause_cfg.auto_neg = pause_cfg->auto_neg;
		cmd_pause_cfg.rx_pause = pause_cfg->rx_pause;
		cmd_pause_cfg.tx_pause = pause_cfg->tx_pause;
	}

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev,
					     SSSNIC_MBX_OPCODE_CFG_PAUSE_INFO,
					     &cmd_pause_cfg, sizeof(cmd_pause_cfg),
					     &cmd_pause_cfg, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_pause_cfg)) {
		if (cmd_pause_cfg.head.state == SSSNIC_UNSUPPORT_SET_PAUSE) {
			ret = -EOPNOTSUPP;
			nic_err(nic_dev->dev_hdl, "Fail to set pause when pfc is enable\n");
		} else {
			ret = -EFAULT;
			nic_err(nic_dev->dev_hdl,
				"Fail to %s pause info, ret: %d, state: 0x%x, out_len: 0x%x\n",
				opcode == SSSNIC_MBX_OPCODE_SET ? "set" : "get",
				ret, cmd_pause_cfg.head.state, out_len);
		}
		return ret;
	}

	if (opcode == SSSNIC_MBX_OPCODE_GET) {
		pause_cfg->auto_neg = cmd_pause_cfg.auto_neg;
		pause_cfg->rx_pause = cmd_pause_cfg.rx_pause;
		pause_cfg->tx_pause = cmd_pause_cfg.tx_pause;
	}

	return 0;
}

int sss_nic_set_hw_pause_info(struct sss_nic_dev *nic_dev,
			      struct sss_nic_pause_cfg pause_cfg)
{
	struct sss_nic_mag_cfg *mag_cfg = NULL;
	int ret;

	mag_cfg = &nic_dev->nic_io->mag_cfg;

	down(&mag_cfg->cfg_lock);

	ret = sss_nic_cfg_hw_pause(nic_dev, SSSNIC_MBX_OPCODE_SET, &pause_cfg);
	if (ret != 0) {
		up(&mag_cfg->cfg_lock);
		return ret;
	}

	mag_cfg->pfc_en = 0;
	mag_cfg->pfc_bitmap = 0;
	mag_cfg->pause_set = true;
	mag_cfg->nic_pause.auto_neg = pause_cfg.auto_neg;
	mag_cfg->nic_pause.rx_pause = pause_cfg.rx_pause;
	mag_cfg->nic_pause.tx_pause = pause_cfg.tx_pause;

	up(&mag_cfg->cfg_lock);

	return 0;
}

int sss_nic_get_hw_pause_info(struct sss_nic_dev *nic_dev, struct sss_nic_pause_cfg *pause_cfg)
{
	struct sss_nic_mag_cfg *mag_cfg = NULL;
	int ret = 0;

	ret = sss_nic_cfg_hw_pause(nic_dev, SSSNIC_MBX_OPCODE_GET, pause_cfg);
	if (ret != 0)
		return ret;

	mag_cfg = &nic_dev->nic_io->mag_cfg;
	if (mag_cfg->pause_set || pause_cfg->auto_neg == SSSNIC_PORT_AN_NOT_SET) {
		pause_cfg->rx_pause = mag_cfg->nic_pause.rx_pause;
		pause_cfg->tx_pause = mag_cfg->nic_pause.tx_pause;
	}

	return 0;
}

int sss_nic_set_hw_dcb_state(struct sss_nic_dev *nic_dev, u8 op_code, u8 state)
{
	struct sss_nic_mbx_dcb_state cmd_dcb_state = {0};
	u16 out_len = sizeof(cmd_dcb_state);
	int ret;

	cmd_dcb_state.state = state;
	cmd_dcb_state.op_code = op_code;
	cmd_dcb_state.func_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_QOS_DCB_STATE,
					     &cmd_dcb_state, sizeof(cmd_dcb_state),
					     &cmd_dcb_state, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_dcb_state)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set dcb state, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_dcb_state.head.state, out_len);
		return -EFAULT;
	}

	return 0;
}

int sss_nic_clear_hw_qp_resource(struct sss_nic_dev *nic_dev)
{
	struct sss_nic_mbx_clear_qp_resource qp_res = {0};
	u16 out_len = sizeof(qp_res);
	int ret;

	if (!nic_dev)
		return -EINVAL;

	qp_res.func_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_CLEAR_QP_RESOURCE,
					     &qp_res, sizeof(qp_res), &qp_res, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &qp_res)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to clear qp resource, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, qp_res.head.state, out_len);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(sss_nic_clear_hw_qp_resource);

int sss_nic_cache_out_qp_resource(struct sss_nic_io *nic_io)
{
	struct sss_nic_mbx_invalid_qp_cache cmd_qp_res = {0};
	u16 out_len = sizeof(cmd_qp_res);
	int ret;

	cmd_qp_res.func_id = sss_get_global_func_id(nic_io->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev, SSSNIC_MBX_OPCODE_CACHE_OUT_QP_RES,
					     &cmd_qp_res, sizeof(cmd_qp_res),
					     &cmd_qp_res, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_qp_res)) {
		nic_err(nic_io->dev_hdl,
			"Fail to cache out qp resources, ret: %d, state: 0x%x, out len: 0x%x\n",
			ret, cmd_qp_res.head.state, out_len);
		return -EIO;
	}

	return 0;
}

int sss_nic_get_vport_stats(struct sss_nic_dev *nic_dev, u16 func_id,
			    struct sss_nic_port_stats *stats)
{
	struct sss_nic_mbx_port_stats_info cmd_port_stats = {0};
	struct sss_nic_mbx_port_stats vport_stats = {0};
	u16 out_len = sizeof(vport_stats);
	int ret;

	if (!nic_dev || !stats)
		return -EINVAL;

	cmd_port_stats.func_id = func_id;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_GET_VPORT_STAT,
					     &cmd_port_stats, sizeof(cmd_port_stats),
					     &vport_stats, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &vport_stats)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to get vport statistics, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, vport_stats.head.state, out_len);
		return -EFAULT;
	}

	memcpy(stats, &vport_stats.stats, sizeof(*stats));

	return 0;
}

static int sss_nic_set_func_table(struct sss_nic_io *nic_io,
				  u32 cfg_mask, const struct sss_nic_func_table_cfg *cfg)
{
	struct sss_nic_mbx_set_func_table cmd_func_tbl = {0};
	u16 out_len = sizeof(cmd_func_tbl);
	int ret;

	cmd_func_tbl.tbl_cfg = *cfg;
	cmd_func_tbl.cfg_bitmap = cfg_mask;
	cmd_func_tbl.func_id = sss_get_global_func_id(nic_io->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev,
					     SSSNIC_MBX_OPCODE_SET_FUNC_TBL,
					     &cmd_func_tbl, sizeof(cmd_func_tbl),
					     &cmd_func_tbl, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_func_tbl)) {
		nic_err(nic_io->dev_hdl,
			"Fail to set func table, bitmap: 0x%x, ret: %d, state: 0x%x, out_len: 0x%x\n",
			cfg_mask, ret, cmd_func_tbl.head.state, out_len);
		return -EFAULT;
	}

	return 0;
}

static int sss_nic_init_func_table(struct sss_nic_io *nic_io)
{
	struct sss_nic_func_table_cfg tbl_cfg = {0};

	tbl_cfg.mtu = SSSNIC_DEFAULT_MAX_MTU;
	tbl_cfg.rx_wqe_buf_size = nic_io->rx_buff_len;

	return sss_nic_set_func_table(nic_io, SSSNIC_INIT_FUNC_MASK, &tbl_cfg);
}

int sss_nic_set_dev_mtu(struct sss_nic_dev *nic_dev, u16 new_mtu)
{
	struct sss_nic_func_table_cfg func_tbl_cfg = {0};

	if (new_mtu < SSSNIC_MIN_MTU_SIZE || new_mtu > SSSNIC_MAX_JUMBO_FRAME_SIZE) {
		nic_err(nic_dev->dev_hdl,
			"Invalid mtu size: %ubytes, mtu range %ubytes - %ubytes.\n",
			new_mtu, SSSNIC_MIN_MTU_SIZE, SSSNIC_MAX_JUMBO_FRAME_SIZE);
		return -EINVAL;
	}

	func_tbl_cfg.mtu = new_mtu;

	return sss_nic_set_func_table(nic_dev->nic_io,
				      BIT(SSSNIC_FUNC_CFG_TYPE_MTU), &func_tbl_cfg);
}

static int sss_nic_feature_nego(struct sss_nic_io *nic_io, u8 opcode, u64 *feature)
{
	struct sss_nic_mbx_feature_nego cmd_feature_nego = {0};
	u16 out_len = sizeof(cmd_feature_nego);
	int ret;

	cmd_feature_nego.opcode = opcode;
	cmd_feature_nego.func_id = sss_get_global_func_id(nic_io->hwdev);
	if (opcode == SSSNIC_MBX_OPCODE_SET)
		memcpy(cmd_feature_nego.feature, feature, sizeof(u64));

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev, SSSNIC_MBX_OPCODE_FEATURE_NEGO,
					     &cmd_feature_nego, sizeof(cmd_feature_nego),
					     &cmd_feature_nego, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_feature_nego)) {
		nic_err(nic_io->dev_hdl,
			"Fail to negotiate nic feature, ret:%d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_feature_nego.head.state, out_len);
		return -EIO;
	}

	if (opcode == SSSNIC_MBX_OPCODE_GET)
		memcpy(feature, cmd_feature_nego.feature, sizeof(u64));

	return 0;
}

static int sss_nic_get_bios_pf_bandwidth(struct sss_nic_io *nic_io)
{
	struct sss_nic_mbx_bios_cfg cmd_bios_cfg = {0};
	u16 out_len = sizeof(cmd_bios_cfg);
	int ret;

	if (sss_get_func_type(nic_io->hwdev) == SSS_FUNC_TYPE_VF ||
	    !SSSNIC_SUPPORT_RATE_LIMIT(nic_io))
		return 0;

	cmd_bios_cfg.op_code = SSSNIC_NVM_PF_SPEED_LIMIT;
	cmd_bios_cfg.bios_cfg.func_valid = SSSNIC_BIOS_FUN_VALID;
	cmd_bios_cfg.bios_cfg.func_id = (u8)sss_get_global_func_id(nic_io->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_io->hwdev, SSSNIC_MBX_OPCODE_BIOS_CFG,
					     &cmd_bios_cfg, sizeof(cmd_bios_cfg),
					     &cmd_bios_cfg, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_bios_cfg)) {
		nic_err(nic_io->dev_hdl,
			"Fail to get bios pf bandwidth limit, ret: %d, status: 0x%x, out_len: 0x%x\n",
			ret, cmd_bios_cfg.head.state, out_len);
		return -EIO;
	}

	if (cmd_bios_cfg.bios_cfg.pf_bw > SSSNIC_MAX_LIMIT_BW) {
		nic_err(nic_io->dev_hdl, "Invalid bios cfg pf bandwidth limit: %u\n",
			cmd_bios_cfg.bios_cfg.pf_bw);
		return -EINVAL;
	}

	if (cmd_bios_cfg.bios_cfg.signature != SSSNIC_BIOS_SIGNATURE)
		nic_warn(nic_io->dev_hdl, "Invalid bios configuration data, signature: 0x%x\n",
			 cmd_bios_cfg.bios_cfg.signature);

	nic_io->mag_cfg.pf_bw_limit = cmd_bios_cfg.bios_cfg.pf_bw;

	return 0;
}

static int sss_nic_get_feature_from_hw(struct sss_nic_io *nic_io)
{
	return sss_nic_feature_nego(nic_io, SSSNIC_MBX_OPCODE_GET, &nic_io->feature_cap);
}

int sss_nic_set_feature_to_hw(struct sss_nic_io *nic_io)
{
	return sss_nic_feature_nego(nic_io, SSSNIC_MBX_OPCODE_SET, &nic_io->feature_cap);
}

void sss_nic_update_nic_feature(struct sss_nic_dev *nic_dev, u64 feature)
{
	struct sss_nic_io *nic_io = nic_dev->nic_io;

	nic_io->feature_cap = feature;

	nic_info(nic_io->dev_hdl, "Update nic feature to 0x%llx\n", nic_io->feature_cap);
}

int sss_nic_io_init(struct sss_nic_dev *nic_dev)
{
	struct pci_dev *pdev = nic_dev->pdev;
	struct sss_nic_io *nic_io = NULL;
	int ret;

	nic_io = kzalloc(sizeof(*nic_io), GFP_KERNEL);
	if (!nic_io)
		return -ENOMEM;

	nic_io->hwdev = nic_dev->hwdev;
	nic_io->pcidev_hdl = pdev;
	nic_io->dev_hdl = &pdev->dev;
	nic_io->nic_dev = nic_dev;
	mutex_init(&nic_io->mag_cfg.sfp_mutex);
	sema_init(&nic_io->mag_cfg.cfg_lock, 1);
	nic_io->rx_buff_len = nic_dev->rx_buff_len;
	nic_dev->nic_io = nic_io;

	ret = sss_register_service_adapter(nic_dev->hwdev, SSS_SERVICE_TYPE_NIC, nic_io);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to register service adapter\n");
		goto register_adapter_err;
	}

	ret = sss_chip_set_func_used_state(nic_dev->hwdev, SSS_SVC_TYPE_NIC,
					   true, SSS_CHANNEL_NIC);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to set function svc used state\n");
		goto set_state_err;
	}

	ret = sss_nic_init_func_table(nic_io);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to init function table\n");
		goto init_func_table_err;
	}

	ret = sss_nic_get_feature_from_hw(nic_io);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to get nic features\n");
		goto get_feature_from_hw_err;
	}

	ret = sss_nic_get_bios_pf_bandwidth(nic_io);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to get pf bandwidth limit\n");
		goto get_bios_pf_bandwidth_err;
	}

	ret = sss_nic_init_pf_vf_info(nic_io);
	if (ret != 0)
		goto init_pf_vf_info_err;

	ret = sss_nic_register_io_callback(nic_io);
	if (ret != 0) {
		nic_err(&pdev->dev, "Fail to init vf info\n");
		goto register_io_callback_err;
	}

	nic_io->feature_cap &= SSSNIC_DRV_DEFAULT_FEATURE;

	return 0;

register_io_callback_err:
	sss_nic_deinit_pf_vf_info(nic_io);

init_pf_vf_info_err:
get_bios_pf_bandwidth_err:
get_feature_from_hw_err:
init_func_table_err:
	sss_chip_set_func_used_state(nic_dev->hwdev, SSS_SVC_TYPE_NIC,
				     false, SSS_CHANNEL_NIC);

set_state_err:
	sss_unregister_service_adapter(nic_dev->hwdev, SSS_SERVICE_TYPE_NIC);

register_adapter_err:
	nic_dev->nic_io = NULL;
	kfree(nic_io);

	return ret;
}
EXPORT_SYMBOL(sss_nic_io_init);

void sss_nic_io_deinit(struct sss_nic_dev *nic_dev)
{
	struct sss_nic_io *nic_io = nic_dev->nic_io;

	sss_nic_unregister_io_callback(nic_io);

	if (nic_io->vf_info_group) {
		sss_nic_clear_all_vf_info(nic_io);
		sss_nic_deinit_pf_vf_info(nic_io);
	}

	sss_chip_set_func_used_state(nic_dev->hwdev, SSS_SVC_TYPE_NIC,
				     false, SSS_CHANNEL_NIC);

	sss_unregister_service_adapter(nic_dev->hwdev, SSS_SERVICE_TYPE_NIC);

	nic_dev->nic_io = NULL;
	kfree(nic_io);
}
EXPORT_SYMBOL(sss_nic_io_deinit);

int sss_nic_force_drop_tx_pkt(struct sss_nic_dev *nic_dev)
{
	struct sss_nic_mbx_force_drop_pkt cmd_force_drop_pkt = {0};
	u16 out_len = sizeof(cmd_force_drop_pkt);
	int ret;

	cmd_force_drop_pkt.port = sss_get_phy_port_id(nic_dev->hwdev);
	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_FORCE_PKT_DROP,
					     &cmd_force_drop_pkt, sizeof(cmd_force_drop_pkt),
					     &cmd_force_drop_pkt, &out_len);
	if ((cmd_force_drop_pkt.head.state != SSS_MGMT_CMD_UNSUPPORTED &&
	     cmd_force_drop_pkt.head.state) || ret || !out_len) {
		nic_err(nic_dev->dev_hdl,
			"Fail to force drop tx packet, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_force_drop_pkt.head.state, out_len);
		return -EFAULT;
	}

	return cmd_force_drop_pkt.head.state;
}

int sss_nic_set_rx_mode(struct sss_nic_dev *nic_dev, u32 rx_mode)
{
	struct sss_nic_mbx_set_rx_mode cmd_set_rx_mode = {0};
	u16 out_len = sizeof(cmd_set_rx_mode);
	int ret;

	cmd_set_rx_mode.func_id = sss_get_global_func_id(nic_dev->hwdev);
	cmd_set_rx_mode.rx_mode = rx_mode;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_SET_RX_MODE,
					     &cmd_set_rx_mode, sizeof(cmd_set_rx_mode),
					     &cmd_set_rx_mode, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_set_rx_mode)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set rx mode, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_set_rx_mode.head.state, out_len);
		return -EINVAL;
	}

	return 0;
}

int sss_nic_set_rx_vlan_offload(struct sss_nic_dev *nic_dev, bool en)
{
	struct sss_nic_mbx_offload_vlan cmd_vlan_offload = {0};
	u16 out_len = sizeof(cmd_vlan_offload);
	int ret;

	cmd_vlan_offload.vlan_offload = (u8)en;
	cmd_vlan_offload.func_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_SET_RX_VLAN_OFFLOAD,
					     &cmd_vlan_offload, sizeof(cmd_vlan_offload),
					     &cmd_vlan_offload, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_vlan_offload)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set rx vlan offload, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_vlan_offload.head.state, out_len);
		return -EINVAL;
	}

	return 0;
}

int sss_nic_update_mac_vlan(struct sss_nic_dev *nic_dev, u16 old_vlan, u16 new_vlan, int vf_id)
{
	struct sss_nic_vf_info *vf_info = NULL;
	struct sss_nic_io *nic_io = nic_dev->nic_io;
	u16 func_id;
	int ret;

	if (old_vlan >= VLAN_N_VID || new_vlan >= VLAN_N_VID)
		return -EINVAL;

	vf_info = nic_io->vf_info_group + SSSNIC_HW_VF_ID_TO_OS(vf_id);
	if (!nic_io->vf_info_group || is_zero_ether_addr(vf_info->drv_mac))
		return 0;

	func_id = sss_get_glb_pf_vf_offset(nic_dev->hwdev) + (u16)vf_id;

	ret = sss_nic_del_mac(nic_dev, vf_info->drv_mac,
			      func_id, old_vlan, SSS_CHANNEL_NIC);
	if (ret != 0) {
		nic_err(nic_dev->dev_hdl, "Fail to delete VF %d MAC %pM vlan %u\n",
			SSSNIC_HW_VF_ID_TO_OS(vf_id), vf_info->drv_mac, old_vlan);
		return ret;
	}

	ret = sss_nic_set_mac(nic_dev, vf_info->drv_mac,
			      func_id, new_vlan, SSS_CHANNEL_NIC);
	if (ret != 0) {
		nic_err(nic_dev->dev_hdl, "Fail to add VF %d MAC %pM vlan %u\n",
			SSSNIC_HW_VF_ID_TO_OS(vf_id), vf_info->drv_mac, new_vlan);
		sss_nic_set_mac(nic_dev, vf_info->drv_mac,
				func_id, old_vlan, SSS_CHANNEL_NIC);
		return ret;
	}

	return 0;
}

static int sss_nic_set_rx_lro(struct sss_nic_dev *nic_dev,
			      bool lro_en, u8 lro_max_pkt_len)
{
	struct sss_nic_mbx_lro_cfg cmd_lro_cfg = {0};
	u16 out_len = sizeof(cmd_lro_cfg);
	int ret;

	cmd_lro_cfg.lro_ipv4_en = (u8)lro_en;
	cmd_lro_cfg.lro_ipv6_en = (u8)lro_en;
	cmd_lro_cfg.lro_max_pkt_len = lro_max_pkt_len;
	cmd_lro_cfg.opcode = SSSNIC_MBX_OPCODE_SET;
	cmd_lro_cfg.func_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_CFG_RX_LRO,
					     &cmd_lro_cfg, sizeof(cmd_lro_cfg),
					     &cmd_lro_cfg, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_lro_cfg)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set lro offload, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_lro_cfg.head.state, out_len);
		return -EINVAL;
	}

	return 0;
}

static int sss_nic_set_rx_lro_timer(struct sss_nic_dev *nic_dev, u32 value)
{
	struct sss_nic_mbx_lro_timer cmd_lro_timer = {0};
	u16 out_len = sizeof(cmd_lro_timer);
	int ret;

	cmd_lro_timer.timer = value;
	cmd_lro_timer.opcode = SSSNIC_MBX_OPCODE_SET;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_CFG_LRO_TIMER,
					     &cmd_lro_timer, sizeof(cmd_lro_timer),
					     &cmd_lro_timer, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_lro_timer)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set lro timer, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_lro_timer.head.state, out_len);

		return -EINVAL;
	}

	return 0;
}

int sss_nic_set_rx_lro_state(struct sss_nic_dev *nic_dev, bool en, u32 timer, u32 max_pkt_len)
{
	int ret;

	nic_info(nic_dev->dev_hdl, "Set LRO max coalesce packet size to %uK\n",
		 max_pkt_len);
	ret = sss_nic_set_rx_lro(nic_dev, en, (u8)max_pkt_len);
	if (ret != 0)
		return ret;

	/* we don't set LRO timer for VF */
	if (sss_get_func_type(nic_dev->hwdev) == SSS_FUNC_TYPE_VF)
		return 0;

	nic_info(nic_dev->dev_hdl, "Success to set LRO timer to %u\n", timer);

	return sss_nic_set_rx_lro_timer(nic_dev, timer);
}

int sss_nic_set_vlan_fliter(struct sss_nic_dev *nic_dev, bool en)
{
	struct sss_nic_mbx_vlan_filter_cfg cmd_set_filter = {0};
	u16 out_len = sizeof(cmd_set_filter);
	int ret;

	cmd_set_filter.func_id = sss_get_global_func_id(nic_dev->hwdev);
	cmd_set_filter.vlan_filter_ctrl = (u32)en;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_SET_VLAN_FILTER_EN,
					     &cmd_set_filter, sizeof(cmd_set_filter),
					     &cmd_set_filter, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_set_filter)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set vlan filter, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_set_filter.head.state, out_len);
		return -EINVAL;
	}

	return 0;
}

int sss_nic_add_tcam_rule(struct sss_nic_dev *nic_dev, struct sss_nic_tcam_rule_cfg *tcam_rule)
{
	struct sss_nic_mbx_add_tcam_rule cmd_add_tcam_rule = {0};
	u16 out_len = sizeof(cmd_add_tcam_rule);
	int ret;

	if (!nic_dev || !tcam_rule)
		return -EINVAL;

	if (tcam_rule->index >= SSSNIC_TCAM_RULES_NUM_MAX) {
		nic_err(nic_dev->dev_hdl, "Invalid tcam rules num :%u to add\n",
			tcam_rule->index);
		return -EINVAL;
	}

	memcpy((void *)&cmd_add_tcam_rule.rule, (void *)tcam_rule,
	       sizeof(struct sss_nic_tcam_rule_cfg));
	cmd_add_tcam_rule.func_id = sss_get_global_func_id(nic_dev->hwdev);
	cmd_add_tcam_rule.type = SSSNIC_TCAM_RULE_FDIR_TYPE;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_ADD_TC_FLOW,
					     &cmd_add_tcam_rule, sizeof(cmd_add_tcam_rule),
					     &cmd_add_tcam_rule, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_add_tcam_rule)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to add tcam rule, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_add_tcam_rule.head.state, out_len);
		return -EIO;
	}

	return 0;
}

int sss_nic_del_tcam_rule(struct sss_nic_dev *nic_dev, u32 index)
{
	struct sss_nic_mbx_del_tcam_rule cmd_del_tcam_rule = {0};
	u16 out_len = sizeof(cmd_del_tcam_rule);
	int ret;

	if (!nic_dev)
		return -EINVAL;

	if (index >= SSSNIC_TCAM_RULES_NUM_MAX) {
		nic_err(nic_dev->dev_hdl, "Invalid tcam rule num :%u to del\n", index);
		return -EINVAL;
	}

	cmd_del_tcam_rule.index_start = index;
	cmd_del_tcam_rule.index_num = 1;
	cmd_del_tcam_rule.func_id = sss_get_global_func_id(nic_dev->hwdev);
	cmd_del_tcam_rule.type = SSSNIC_TCAM_RULE_FDIR_TYPE;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_DEL_TC_FLOW,
					     &cmd_del_tcam_rule, sizeof(cmd_del_tcam_rule),
					     &cmd_del_tcam_rule, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_del_tcam_rule)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to delete tcam rule, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_del_tcam_rule.head.state, out_len);
		return -EIO;
	}

	return 0;
}

static int sss_nic_mgmt_tcam_block(struct sss_nic_dev *nic_dev, u8 alloc_en, u16 *index)
{
	struct sss_nic_mbx_tcam_block_cfg cmd_mgmt_tcam_block = {0};
	u16 out_len = sizeof(cmd_mgmt_tcam_block);
	int ret;

	if (!nic_dev || !index)
		return -EINVAL;

	cmd_mgmt_tcam_block.func_id = sss_get_global_func_id(nic_dev->hwdev);
	cmd_mgmt_tcam_block.alloc_en = alloc_en;
	cmd_mgmt_tcam_block.tcam_type = SSSNIC_TCAM_BLOCK_TYPE_LARGE;
	cmd_mgmt_tcam_block.tcam_block_index = *index;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_CFG_TCAM_BLOCK,
					     &cmd_mgmt_tcam_block, sizeof(cmd_mgmt_tcam_block),
					     &cmd_mgmt_tcam_block, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_mgmt_tcam_block)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set tcam block, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_mgmt_tcam_block.head.state, out_len);
		return -EIO;
	}

	if (alloc_en)
		*index = cmd_mgmt_tcam_block.tcam_block_index;

	return 0;
}

int sss_nic_alloc_tcam_block(struct sss_nic_dev *nic_dev, u16 *index)
{
	return sss_nic_mgmt_tcam_block(nic_dev, SSSNIC_TCAM_BLOCK_ENABLE, index);
}

int sss_nic_free_tcam_block(struct sss_nic_dev *nic_dev, u16 *index)
{
	return sss_nic_mgmt_tcam_block(nic_dev, SSSNIC_TCAM_BLOCK_DISABLE, index);
}

int sss_nic_set_fdir_tcam_rule_filter(struct sss_nic_dev *nic_dev, bool enable)
{
	struct sss_nic_mbx_set_tcam_state cmd_set_tcam_enable = {0};
	u16 out_len = sizeof(cmd_set_tcam_enable);
	int ret;

	cmd_set_tcam_enable.func_id = sss_get_global_func_id(nic_dev->hwdev);
	cmd_set_tcam_enable.tcam_enable = (u8)enable;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_ENABLE_TCAM,
					     &cmd_set_tcam_enable, sizeof(cmd_set_tcam_enable),
					     &cmd_set_tcam_enable, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_set_tcam_enable)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to set fdir tcam filter, ret: %d, state: 0x%x, out_len: 0x%x, enable: 0x%x\n",
			ret, cmd_set_tcam_enable.head.state, out_len,
			enable);
		return -EIO;
	}

	return 0;
}

int sss_nic_flush_tcam_rule(struct sss_nic_dev *nic_dev)
{
	struct sss_nic_mbx_flush_tcam_rule cmd_flush_tcam_rule = {0};
	u16 out_len = sizeof(cmd_flush_tcam_rule);
	int ret;

	cmd_flush_tcam_rule.func_id = sss_get_global_func_id(nic_dev->hwdev);

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_FLUSH_TCAM,
					     &cmd_flush_tcam_rule,
					     sizeof(cmd_flush_tcam_rule),
					     &cmd_flush_tcam_rule, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &cmd_flush_tcam_rule)) {
		nic_err(nic_dev->dev_hdl,
			"Fail to flush tcam fdir rule, ret: %d, state: 0x%x, out_len: 0x%x\n",
			ret, cmd_flush_tcam_rule.head.state, out_len);
		return -EIO;
	}

	return 0;
}

int sss_nic_rq_hw_pc_info(struct sss_nic_dev *nic_dev,
			  struct sss_nic_rq_pc_info *out_info, u16 qp_num, u16 wqe_type)
{
	int ret;
	u16 i;
	struct sss_nic_rq_pc_info *rq_pc_info = NULL;
	struct sss_nic_rq_hw_info *rq_hw = NULL;
	struct sss_ctrl_msg_buf *msg_buf = NULL;

	msg_buf = sss_alloc_ctrlq_msg_buf(nic_dev->hwdev);
	if (!msg_buf) {
		nic_err(nic_dev->dev_hdl, "Fail to alloc cmd_buf\n");
		return -ENOMEM;
	}

	msg_buf->size = sizeof(*rq_hw);

	rq_hw = msg_buf->buf;
	rq_hw->num_queues = qp_num;
	rq_hw->func_id = sss_get_global_func_id(nic_dev->hwdev);
	sss_cpu_to_be32(rq_hw, sizeof(*rq_hw));

	ret = sss_ctrlq_detail_reply(nic_dev->hwdev, SSS_MOD_TYPE_L2NIC,
				     SSSNIC_CTRLQ_OPCODE_RXQ_INFO_GET,
				     msg_buf, msg_buf, NULL, 0, SSS_CHANNEL_NIC);
	if (ret)
		goto get_rq_info_error;

	rq_pc_info = msg_buf->buf;
	for (i = 0; i < qp_num; i++) {
		out_info[i].hw_ci = rq_pc_info[i].hw_ci >> wqe_type;
		out_info[i].hw_pi = rq_pc_info[i].hw_pi >> wqe_type;
	}

get_rq_info_error:
	sss_free_ctrlq_msg_buf(nic_dev->hwdev, msg_buf);

	return ret;
}

int sss_nic_set_pf_rate(struct sss_nic_dev *nic_dev, u8 speed)
{
	int ret;
	u32 pf_rate;
	u32 speed_convert[SSSNIC_PORT_SPEED_UNKNOWN] = {
		0, 10, 100, 1000, 10000, 25000, 40000, 50000, 100000, 200000
	};
	struct sss_nic_io *nic_io = nic_dev->nic_io;
	struct sss_nic_mbx_tx_rate_cfg rate_cfg = {0};
	u16 out_len = sizeof(rate_cfg);

	if (speed >= SSSNIC_PORT_SPEED_UNKNOWN) {
		nic_err(nic_io->dev_hdl, "Invalid speed level: %u\n", speed);
		return -EINVAL;
	}

	if (nic_io->mag_cfg.pf_bw_limit == SSSNIC_PF_LIMIT_BW_MAX) {
		pf_rate = 0;
	} else {
		pf_rate = (speed_convert[speed] / 100) * nic_io->mag_cfg.pf_bw_limit;
		if (pf_rate == 0 && speed != SSSNIC_PORT_SPEED_NOT_SET)
			pf_rate = 1;
	}

	rate_cfg.func_id = sss_get_global_func_id(nic_dev->hwdev);
	rate_cfg.max_rate = pf_rate;
	rate_cfg.min_rate = 0;

	ret = sss_nic_l2nic_msg_to_mgmt_sync(nic_dev->hwdev, SSSNIC_MBX_OPCODE_SET_MAX_MIN_RATE,
					     &rate_cfg, sizeof(rate_cfg), &rate_cfg, &out_len);
	if (SSS_ASSERT_SEND_MSG_RETURN(ret, out_len, &rate_cfg)) {
		nic_err(nic_dev->dev_hdl, "Fail to set rate:%u, ret: %d, state: 0x%x, out len: 0x%x\n",
			pf_rate, ret, rate_cfg.head.state, out_len);
		return rate_cfg.head.state ? rate_cfg.head.state : -EIO;
	}

	return 0;
}

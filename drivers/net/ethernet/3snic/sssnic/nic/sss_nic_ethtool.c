// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/if_vlan.h>
#include <linux/ethtool.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_tx.h"
#include "sss_nic_rx.h"
#include "sss_nic_rss.h"
#include "sss_nic_ethtool_api.h"
#include "sss_nic_ethtool_stats.h"
#include "sss_nic_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_cfg_define.h"
#include "sss_nic_netdev_ops_api.h"

#define SSSNIC_MGMT_VERSION_MAX_LEN			32

#define SSSNIC_AUTONEG_RESET_TIMEOUT		100
#define SSSNIC_AUTONEG_FINISH_TIMEOUT		200

static void sss_nic_get_drvinfo(struct net_device *netdev,
				struct ethtool_drvinfo *drvinfo)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	struct pci_dev *pdev = nic_dev->pdev;
	u8 mgmt_ver[SSSNIC_MGMT_VERSION_MAX_LEN] = {0};
	int ret;

	strscpy(drvinfo->driver, SSSNIC_DRV_NAME, sizeof(drvinfo->driver));
	strscpy(drvinfo->version, SSSNIC_DRV_VERSION, sizeof(drvinfo->version));
	strscpy(drvinfo->bus_info, pci_name(pdev), sizeof(drvinfo->bus_info));

	ret = sss_get_mgmt_version(nic_dev->hwdev, mgmt_ver,
				   SSSNIC_MGMT_VERSION_MAX_LEN,
				   SSS_CHANNEL_NIC);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to get fw version, ret: %d\n", ret);
		return;
	}

	ret = snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version), "%s", mgmt_ver);
	if (ret < 0)
		nicif_err(nic_dev, drv, netdev, "Fail to snprintf fw version\n");
}

static u32 sss_nic_get_msglevel(struct net_device *netdev)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	return nic_dev->msg_enable;
}

static void sss_nic_set_msglevel(struct net_device *netdev, u32 msg_enable)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	nicif_info(nic_dev, drv, netdev, "Success to change msg_enable from 0x%x to 0x%x\n",
		   nic_dev->msg_enable, msg_enable);

	nic_dev->msg_enable = msg_enable;
}

static int sss_nic_nway_reset(struct net_device *netdev)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	struct sss_nic_port_info port_info = {0};
	int ret;

	while (SSSNIC_TEST_SET_NIC_DEV_FLAG(nic_dev, SSSNIC_AUTONEG_RESET))
		msleep(SSSNIC_AUTONEG_RESET_TIMEOUT);

	ret = sss_nic_get_hw_port_info(nic_dev, &port_info, SSS_CHANNEL_NIC);
	if (ret) {
		nicif_err(nic_dev, drv, netdev, "Fail to get port info\n");
		ret = -EFAULT;
		goto reset_err;
	}

	if (port_info.autoneg_state != SSSNIC_PORT_CFG_AN_ON) {
		nicif_err(nic_dev, drv, netdev, "Autonegotiation is not on, don't support to restart it\n");
		ret = -EOPNOTSUPP;
		goto reset_err;
	}

	ret = sss_nic_set_autoneg(nic_dev, false);
	if (ret) {
		nicif_err(nic_dev, drv, netdev, "Fail to set autonegotiation off\n");
		ret = -EFAULT;
		goto reset_err;
	}

	msleep(SSSNIC_AUTONEG_FINISH_TIMEOUT);

	ret = sss_nic_set_autoneg(nic_dev, true);
	if (ret) {
		nicif_err(nic_dev, drv, netdev, "Fail to set autonegotiation on\n");
		ret = -EFAULT;
		goto reset_err;
	}

	msleep(SSSNIC_AUTONEG_FINISH_TIMEOUT);
	nicif_info(nic_dev, drv, netdev, "Success to restart autonegotiation\n");

reset_err:
	clear_bit(SSSNIC_AUTONEG_RESET, &nic_dev->flags);
	return ret;
}

static void sss_nic_get_ringparam(struct net_device *netdev,
				  struct ethtool_ringparam *ringparam,
				  struct kernel_ethtool_ringparam __maybe_unused *param,
				  struct netlink_ext_ack __maybe_unused *extack)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	ringparam->tx_pending = nic_dev->sq_desc_group[0].q_depth;
	ringparam->rx_pending = nic_dev->rq_desc_group[0].q_depth;
	ringparam->tx_max_pending = SSSNIC_MAX_TX_QUEUE_DEPTH;
	ringparam->rx_max_pending = SSSNIC_MAX_RX_QUEUE_DEPTH;
}

static int sss_nic_set_ringparam(struct net_device *netdev,
				 struct ethtool_ringparam *ringparam,
				 struct kernel_ethtool_ringparam __maybe_unused *param,
				 struct netlink_ext_ack __maybe_unused *extack)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	struct sss_nic_qp_resource qp_res = {0};
	u32 sq_depth;
	u32 rq_depth;
	int ret;

	ret = sss_nic_check_ringparam_valid(netdev, ringparam);
	if (ret != 0)
		return ret;

	sq_depth = (u32)(1U << (u16)ilog2(ringparam->tx_pending));
	rq_depth = (u32)(1U << (u16)ilog2(ringparam->rx_pending));
	if (sq_depth == nic_dev->qp_res.sq_depth &&
	    rq_depth == nic_dev->qp_res.rq_depth)
		return 0; /* nothing to do */

	nicif_info(nic_dev, drv, netdev,
		   "Change Tx/Rx ring depth from %u/%u to %u/%u\n",
		   nic_dev->qp_res.sq_depth, nic_dev->qp_res.rq_depth,
		   sq_depth, rq_depth);

	if (netif_running(netdev) == 0) {
		sss_nic_update_qp_depth(nic_dev, sq_depth, rq_depth);
		return 0;
	}

	qp_res = nic_dev->qp_res;
	qp_res.sq_depth = sq_depth;
	qp_res.rq_depth = rq_depth;
	qp_res.sq_res_group = NULL;
	qp_res.rq_res_group = NULL;
	qp_res.irq_cfg = NULL;

	nicif_info(nic_dev, drv, netdev, "Restarting channel\n");
	ret = sss_nic_update_channel_setting(nic_dev, &qp_res,
					     NULL, NULL);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to update channel settings\n");
		return -EFAULT;
	}

	return 0;
}

static int sss_nic_get_coalesce(struct net_device *netdev, struct ethtool_coalesce *coal,
				struct kernel_ethtool_coalesce __maybe_unused *kec,
				struct netlink_ext_ack __maybe_unused *extack)
{
	return sss_nic_ethtool_get_coalesce(netdev, coal, SSSNIC_COALESCE_ALL_QUEUE);
}

static int sss_nic_set_coalesce(struct net_device *netdev, struct ethtool_coalesce *coal,
				struct kernel_ethtool_coalesce __maybe_unused *kec,
				struct netlink_ext_ack __maybe_unused *extack)
{
	return sss_nic_ethtool_set_coalesce(netdev, coal, SSSNIC_COALESCE_ALL_QUEUE);
}

static int sss_nic_get_per_queue_coalesce(struct net_device *netdev, u32 queue,
					  struct ethtool_coalesce *coal)
{
	return sss_nic_ethtool_get_coalesce(netdev, coal, (u16)queue);
}

static int sss_nic_set_per_queue_coalesce(struct net_device *netdev, u32 queue,
					  struct ethtool_coalesce *coal)
{
	return sss_nic_ethtool_set_coalesce(netdev, coal, (u16)queue);
}

static int sss_nic_set_phys_id(struct net_device *netdev,
			       enum ethtool_phys_id_state state)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	enum sss_nic_mag_led_type led_type = SSSNIC_MAG_LED_TYPE_ALARM;
	enum sss_nic_mag_led_mode led_mode;
	int ret;

	if (state == ETHTOOL_ID_ACTIVE) {
		led_mode = SSSNIC_MAG_LED_FORCE_BLINK_2HZ;
	} else if (state == ETHTOOL_ID_INACTIVE) {
		led_mode = SSSNIC_MAG_LED_DEFAULT;
	} else {
		nicif_err(nic_dev, drv, netdev, "Not support to set phys id, state:%d\n", state);
		return -EOPNOTSUPP;
	}

	ret = sss_nic_set_hw_led_state(nic_dev, led_type, led_mode);
	if (ret != 0)
		nicif_err(nic_dev, drv, netdev, "Fail to set led status, ret:%d, type:%d, mode:%d\n",
			  ret, led_type, led_mode);

	return ret;
}

static void sss_nic_get_pauseparam(struct net_device *netdev,
				   struct ethtool_pauseparam *pauseparam)
{
	int ret;
	struct sss_nic_pause_cfg pause_config = {0};
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	ret = sss_nic_get_hw_pause_info(nic_dev, &pause_config);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to get pauseparam\n");
	} else {
		pauseparam->autoneg = pause_config.auto_neg == SSSNIC_PORT_CFG_AN_ON ?
				      AUTONEG_ENABLE : AUTONEG_DISABLE;
		pauseparam->rx_pause = pause_config.rx_pause;
		pauseparam->tx_pause = pause_config.tx_pause;
	}
}

static int sss_nic_set_pauseparam(struct net_device *netdev,
				  struct ethtool_pauseparam *pauseparam)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	struct sss_nic_port_info port_info = {0};
	struct sss_nic_pause_cfg pause_config = {0};
	u32 auto_neg;
	int ret;

	ret = sss_nic_get_hw_port_info(nic_dev, &port_info, SSS_CHANNEL_NIC);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev,
			  "Fail to get auto-negotiation state\n");
		return -EFAULT;
	}

	auto_neg = port_info.autoneg_state == SSSNIC_PORT_CFG_AN_ON ?
		   AUTONEG_ENABLE : AUTONEG_DISABLE;
	if (pauseparam->autoneg != auto_neg) {
		nicif_err(nic_dev, drv, netdev,
			  "Use: ethtool -s <dev> autoneg <on|off> to change autoneg\n");
		return -EOPNOTSUPP;
	}
	pause_config.auto_neg = pauseparam->autoneg == AUTONEG_ENABLE ?
				SSSNIC_PORT_CFG_AN_ON : SSSNIC_PORT_CFG_AN_OFF;
	pause_config.rx_pause = (u8)pauseparam->rx_pause;
	pause_config.tx_pause = (u8)pauseparam->tx_pause;

	ret = sss_nic_set_hw_pause_info(nic_dev, pause_config);
	if (ret != 0) {
		nicif_err(nic_dev, drv, netdev, "Fail to set pauseparam\n");
		return ret;
	}

	nicif_info(nic_dev, drv, netdev, "Success to set pauseparam option, rx: %s, tx: %s\n",
		   pauseparam->rx_pause ? "on" : "off", pauseparam->tx_pause ? "on" : "off");

	return 0;
}

static int sss_nic_get_module_info(struct net_device *netdev,
				   struct ethtool_modinfo *modinfo)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	sss_nic_get_module_info_t handler[SSSNIC_MODULE_TYPE_MAX] = {NULL};
	u8 sfp_type = 0;
	u8 sfp_type_ext = 0;
	int ret;

	handler[SSSNIC_MODULE_TYPE_SFP] = sss_nic_module_type_sfp;
	handler[SSSNIC_MODULE_TYPE_QSFP] = sss_nic_module_type_qsfp;
	handler[SSSNIC_MODULE_TYPE_QSFP_PLUS] = sss_nic_module_type_qsfp_plus;
	handler[SSSNIC_MODULE_TYPE_QSFP28] = sss_nic_module_type_qsfp28;

	ret = sss_nic_get_sfp_type(nic_dev, &sfp_type, &sfp_type_ext);
	if (ret != 0)
		return ret;

	if (sfp_type >= SSSNIC_MODULE_TYPE_MAX) {
		nicif_warn(nic_dev, drv, netdev,
			   "Unknown optical module type: 0x%x\n", sfp_type);
		return -EINVAL;
	}

	if (!handler[sfp_type]) {
		nicif_warn(nic_dev, drv, netdev,
			   "Unknown optical module type: 0x%x\n", sfp_type);
		return -EINVAL;
	}

	handler[sfp_type](modinfo, sfp_type_ext);

	return 0;
}

static int sss_nic_get_module_eeprom(struct net_device *netdev,
				     struct ethtool_eeprom *ee, u8 *data)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	u8 sfp_data[SSSNIC_STD_SFP_INFO_MAX_SIZE];
	u32 offset = ee->len + ee->offset;
	u32 len = ee->len;
	int ret;

	if (len == 0 || offset > SSSNIC_STD_SFP_INFO_MAX_SIZE)
		return -EINVAL;

	memset(data, 0, len);

	ret = sss_nic_get_sfp_eeprom(nic_dev, (u8 *)sfp_data, len);
	if (ret != 0)
		return ret;

	memcpy(data, sfp_data + ee->offset, len);

	return 0;
}

static u32 sss_nic_get_priv_flags(struct net_device *netdev)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	u32 ret_flag = 0;

	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_SAME_RXTX))
		ret_flag |= SSSNIC_PRIV_FLAG_SYMM_RSS;

	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_FORCE_LINK_UP))
		ret_flag |= SSSNIC_PRIV_FLAG_LINK_UP;

	if (SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_RXQ_RECOVERY))
		ret_flag |= SSSNIC_PRIV_FLAG_RQ_RECOVERY;

	return ret_flag;
}

static int sss_nic_set_priv_flags(struct net_device *netdev, u32 flags)
{
	int ret;

	ret = sss_nic_set_symm_rss_flag(netdev, flags);
	if (ret)
		return ret;

	ret = sss_nic_set_rq_recovery_flag(netdev, flags);
	if (ret)
		return ret;

	return sss_nic_set_force_link_flag(netdev, flags);
}

static void sss_nic_self_test(struct net_device *netdev,
			      struct ethtool_test *eth_test, u64 *data)
{
	memset(data, 0, SSSNIC_LP_TEST_TYPE_MAX * sizeof(u64));
	sss_nic_loop_test(netdev, eth_test, data);
}

static const struct ethtool_ops g_nic_ethtool_ops = {
	.supported_coalesce_params = SSSNIC_SUPPORTED_COALESCE_PARAMS,

	.get_link_ksettings = sss_nic_get_link_ksettings,
	.set_link_ksettings = sss_nic_set_link_ksettings,

	.get_drvinfo = sss_nic_get_drvinfo,
	.get_msglevel = sss_nic_get_msglevel,
	.set_msglevel = sss_nic_set_msglevel,
	.nway_reset = sss_nic_nway_reset,
	.get_link = ethtool_op_get_link,
	.get_ringparam = sss_nic_get_ringparam,
	.set_ringparam = sss_nic_set_ringparam,
	.get_pauseparam = sss_nic_get_pauseparam,
	.set_pauseparam = sss_nic_set_pauseparam,
	.get_sset_count = sss_nic_get_sset_count,
	.get_ethtool_stats = sss_nic_get_ethtool_stats,
	.get_strings = sss_nic_get_strings,

	.self_test = sss_nic_self_test,

	.set_phys_id = sss_nic_set_phys_id,

	.get_coalesce = sss_nic_get_coalesce,
	.set_coalesce = sss_nic_set_coalesce,

	.get_per_queue_coalesce = sss_nic_get_per_queue_coalesce,
	.set_per_queue_coalesce = sss_nic_set_per_queue_coalesce,

	.get_rxnfc = sss_nic_get_rxnfc,
	.set_rxnfc = sss_nic_set_rxnfc,
	.get_priv_flags = sss_nic_get_priv_flags,
	.set_priv_flags = sss_nic_set_priv_flags,

	.get_channels = sss_nic_get_channels,
	.set_channels = sss_nic_set_channels,

	.get_module_info = sss_nic_get_module_info,
	.get_module_eeprom = sss_nic_get_module_eeprom,

	.get_rxfh_indir_size = sss_nic_get_rxfh_indir_size,

	.get_rxfh_key_size = sss_nic_get_rxfh_key_size,
	.get_rxfh = sss_nic_get_rxfh,
	.set_rxfh = sss_nic_set_rxfh,
};

static const struct ethtool_ops g_nicvf_ethtool_ops = {
	.supported_coalesce_params = SSSNIC_SUPPORTED_COALESCE_PARAMS,

	.get_link_ksettings = sss_nic_get_link_ksettings,

	.get_drvinfo = sss_nic_get_drvinfo,
	.get_msglevel = sss_nic_get_msglevel,
	.set_msglevel = sss_nic_set_msglevel,
	.get_link = ethtool_op_get_link,
	.get_ringparam = sss_nic_get_ringparam,

	.set_ringparam = sss_nic_set_ringparam,
	.get_sset_count = sss_nic_get_sset_count,
	.get_ethtool_stats = sss_nic_get_ethtool_stats,
	.get_strings = sss_nic_get_strings,

	.get_coalesce = sss_nic_get_coalesce,
	.set_coalesce = sss_nic_set_coalesce,

	.get_per_queue_coalesce = sss_nic_get_per_queue_coalesce,
	.set_per_queue_coalesce = sss_nic_set_per_queue_coalesce,

	.get_rxnfc = sss_nic_get_rxnfc,
	.set_rxnfc = sss_nic_set_rxnfc,
	.get_priv_flags = sss_nic_get_priv_flags,
	.set_priv_flags = sss_nic_set_priv_flags,

	.get_channels = sss_nic_get_channels,
	.set_channels = sss_nic_set_channels,

	.get_rxfh_indir_size = sss_nic_get_rxfh_indir_size,

	.get_rxfh_key_size = sss_nic_get_rxfh_key_size,
	.get_rxfh = sss_nic_get_rxfh,
	.set_rxfh = sss_nic_set_rxfh,

};

void sss_nic_set_ethtool_ops(struct sss_nic_dev *adapter)
{
	struct net_device *netdev = adapter->netdev;

	if (!SSSNIC_FUNC_IS_VF(adapter->hwdev))
		netdev->ethtool_ops = &g_nic_ethtool_ops;
	else
		netdev->ethtool_ops = &g_nicvf_ethtool_ops;
}

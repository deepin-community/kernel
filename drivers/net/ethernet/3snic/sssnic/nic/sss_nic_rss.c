// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/device.h>
#include <linux/ethtool.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/dcbnl.h>

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_cfg.h"
#include "sss_nic_rss_cfg.h"
#include "sss_nic_dev_define.h"
#include "sss_nic_rss.h"
#include "sss_nic_ntuple.h"
#include "sss_nic_netdev_ops_api.h"
#include "sss_nic_dcb.h"

#define SSSNIC_INVALID_TC_ID 0xFF

#define SSSNIC_DEF_RSS_KEY_0 0x6d
#define SSSNIC_DEF_RSS_KEY_1 0x5a
#define SSSNIC_DEF_RSS_KEY_2 0x56
#define SSSNIC_DEF_RSS_KEY_3 0xda
#define SSSNIC_DEF_RSS_KEY_4 0x25
#define SSSNIC_DEF_RSS_KEY_5 0x5b
#define SSSNIC_DEF_RSS_KEY_6 0x0e
#define SSSNIC_DEF_RSS_KEY_7 0xc2
#define SSSNIC_DEF_RSS_KEY_8 0x41
#define SSSNIC_DEF_RSS_KEY_9 0x67
#define SSSNIC_DEF_RSS_KEY_10 0x25
#define SSSNIC_DEF_RSS_KEY_11 0x3d
#define SSSNIC_DEF_RSS_KEY_12 0x43
#define SSSNIC_DEF_RSS_KEY_13 0xa3
#define SSSNIC_DEF_RSS_KEY_14 0x8f
#define SSSNIC_DEF_RSS_KEY_15 0xb0
#define SSSNIC_DEF_RSS_KEY_16 0xd0
#define SSSNIC_DEF_RSS_KEY_17 0xca
#define SSSNIC_DEF_RSS_KEY_18 0x2b
#define SSSNIC_DEF_RSS_KEY_19 0xcb
#define SSSNIC_DEF_RSS_KEY_20 0xae
#define SSSNIC_DEF_RSS_KEY_21 0x7b
#define SSSNIC_DEF_RSS_KEY_22 0x30
#define SSSNIC_DEF_RSS_KEY_23 0xb4
#define SSSNIC_DEF_RSS_KEY_24 0x77
#define SSSNIC_DEF_RSS_KEY_25 0xcb
#define SSSNIC_DEF_RSS_KEY_26 0x2d
#define SSSNIC_DEF_RSS_KEY_27 0xa3
#define SSSNIC_DEF_RSS_KEY_28 0x80
#define SSSNIC_DEF_RSS_KEY_29 0x30
#define SSSNIC_DEF_RSS_KEY_30 0xf2
#define SSSNIC_DEF_RSS_KEY_31 0x0c
#define SSSNIC_DEF_RSS_KEY_32 0x6a
#define SSSNIC_DEF_RSS_KEY_33 0x42
#define SSSNIC_DEF_RSS_KEY_34 0xb7
#define SSSNIC_DEF_RSS_KEY_35 0x3b
#define SSSNIC_DEF_RSS_KEY_36 0xbe
#define SSSNIC_DEF_RSS_KEY_37 0xac
#define SSSNIC_DEF_RSS_KEY_38 0x01
#define SSSNIC_DEF_RSS_KEY_39 0xfa

#define SSSNIC_COS_CHANGE_OFFSET	4

#define SSSNIC_RXH_PORT			(RXH_L4_B_0_1 | RXH_L4_B_2_3)
#define SSSNIC_RXH_IP			(RXH_IP_DST | RXH_IP_SRC)
#define SSSNIC_SUPPORT_RXH		(SSSNIC_RXH_IP | SSSNIC_RXH_PORT)

static int sss_nic_set_hw_rss(struct net_device *netdev, u8 *cos_map, u8 cos_num);

static u16 max_qp_num;
module_param(max_qp_num, ushort, 0444);
MODULE_PARM_DESC(max_qp_num, "Number of Queue Pairs (default=0)");

static void sss_nic_fill_indir_tbl(struct sss_nic_dev *nic_dev, u8 cos_num, u32 *indir)
{
	int i = 0;
	u16 k;
	u16 group_size;
	u16 start_qid = 0;
	u16 qp_num = 0;
	u8 cur_cos = 0;
	u8 j;
	u8 default_cos;
	u8 cos_map = sss_nic_get_valid_cos_map(nic_dev);

	if (cos_num == 0) {
		for (i = 0; i < SSSNIC_RSS_INDIR_SIZE; i++)
			indir[i] = i % nic_dev->qp_res.qp_num;
		return;
	}

	group_size = SSSNIC_RSS_INDIR_SIZE / cos_num;
	for (j = 0; j < cos_num; j++) {
		while (cur_cos < SSSNIC_DCB_COS_MAX &&
		       nic_dev->hw_dcb_cfg.cos_qp_num[cur_cos] == 0)
			cur_cos++;

		if (cur_cos < SSSNIC_DCB_COS_MAX) {
			qp_num = nic_dev->hw_dcb_cfg.cos_qp_num[cur_cos];
			start_qid = nic_dev->hw_dcb_cfg.cos_qp_offset[cur_cos];
		} else {
			if (BIT(nic_dev->hw_dcb_cfg.default_cos) & cos_map)
				default_cos = nic_dev->hw_dcb_cfg.default_cos;
			else
				default_cos = (u8)fls(cos_map) - 1;
			qp_num = nic_dev->hw_dcb_cfg.cos_qp_num[default_cos];
			start_qid = nic_dev->hw_dcb_cfg.cos_qp_offset[default_cos];
		}

		for (k = 0; k < group_size; k++)
			indir[i++] = start_qid + k % qp_num;

		cur_cos++;
	}
}

static void sss_nic_get_dcb_cos_map(struct sss_nic_dev *nic_dev,
				    u8 *cos_map, u8 *cos_num)
{
	u8 i;
	u8 num;
	u8 cfg_map[SSSNIC_DCB_UP_MAX];
	bool dcb_en = !!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE);

	if (!dcb_en)
		return;

	if (nic_dev->hw_dcb_cfg.trust == 0) {
		memcpy(cfg_map, nic_dev->hw_dcb_cfg.pcp2cos, sizeof(cfg_map));
	} else if (nic_dev->hw_dcb_cfg.trust == 1) {
		for (i = 0; i < SSSNIC_DCB_UP_MAX; i++)
			cfg_map[i] = nic_dev->hw_dcb_cfg.dscp2cos[i * SSSNIC_DCB_DSCP_NUM];
	}

	for (i = 0; i < SSSNIC_COS_CHANGE_OFFSET; i++)
		cos_map[SSSNIC_COS_CHANGE_OFFSET + i] = cfg_map[i];

	for (i = 0; i < SSSNIC_COS_CHANGE_OFFSET; i++)
		cos_map[i] = cfg_map[SSSNIC_DCB_UP_MAX - (i + 1)];

	num = sss_nic_get_user_cos_num(nic_dev);
	while (num & (num - 1))
		num++;

	*cos_num = num;
}

int sss_nic_update_rss_cfg(struct sss_nic_dev *nic_dev)
{
	int ret;
	u8 cos_num = 0;
	u8 cos_map[SSSNIC_DCB_UP_MAX] = {0};
	struct net_device *netdev = nic_dev->netdev;

	sss_nic_get_dcb_cos_map(nic_dev, cos_map, &cos_num);

	ret = sss_nic_set_hw_rss(netdev, cos_map, cos_num);
	if (ret != 0)
		return ret;

	return ret;
}

void sss_nic_reset_rss_cfg(struct sss_nic_dev *nic_dev)
{
	u8 cos_map[SSSNIC_DCB_UP_MAX] = {0};

	sss_nic_config_rss_to_hw(nic_dev, 0, cos_map, 1, 0);
}

static void sss_nic_init_rss_type(struct sss_nic_dev *nic_dev)
{
	nic_dev->rss_type.ipv4 = 1;
	nic_dev->rss_type.ipv6 = 1;
	nic_dev->rss_type.ipv6_ext = 1;
	nic_dev->rss_type.tcp_ipv4 = 1;
	nic_dev->rss_type.tcp_ipv6 = 1;
	nic_dev->rss_type.tcp_ipv6_ext = 1;
	nic_dev->rss_type.udp_ipv4 = 1;
	nic_dev->rss_type.udp_ipv6 = 1;
	nic_dev->rss_hash_engine = SSSNIC_RSS_ENGINE_XOR;
}

void sss_nic_free_rss_key(struct sss_nic_dev *nic_dev)
{
	kfree(nic_dev->rss_key);
	nic_dev->rss_key = NULL;
	nic_dev->rss_key_big = NULL;

	kfree(nic_dev->rss_indir_tbl);
	nic_dev->rss_indir_tbl = NULL;
}

void sss_nic_set_default_rss_indir(struct net_device *netdev)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	set_bit(SSSNIC_RSS_DEFAULT_INDIR, &nic_dev->flags);
}

static void sss_nic_maybe_reset_rss_indir(struct net_device *netdev, bool dcb_en)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	int i;

	if (dcb_en) {
		nicif_info(nic_dev, drv, netdev, "DCB is enabled, set default rss indir\n");
		set_bit(SSSNIC_RSS_DEFAULT_INDIR, &nic_dev->flags);
		return;
	}

	for (i = 0; i < SSSNIC_RSS_INDIR_SIZE; i++) {
		if (nic_dev->rss_indir_tbl[i] >= nic_dev->qp_res.qp_num) {
			set_bit(SSSNIC_RSS_DEFAULT_INDIR, &nic_dev->flags);
			return;
		}
	}
}

static u16 sss_nic_get_online_cpu(struct pci_dev *pdev)
{
	int i;
	int node;
	u16 cpu_num = 0;

	for (i = 0; i < (int)num_online_cpus(); i++) {
		node = (int)cpu_to_node(i);
		if (node == dev_to_node(&pdev->dev))
			cpu_num++;
	}

	if (cpu_num == 0)
		cpu_num = (u16)num_online_cpus();

	return cpu_num;
}

static void sss_nic_init_qp_num(struct sss_nic_dev *nic_dev)
{
	u16 cpu_num = 0;
	u16 qp_num = nic_dev->max_qp_num;
	u16 default_qp_num = nic_dev->nic_svc_cap.def_queue_num;

	if (default_qp_num != 0 && default_qp_num < qp_num)
		qp_num = default_qp_num;

	if (max_qp_num > nic_dev->max_qp_num)
		qp_num = nic_dev->max_qp_num;
	else if (max_qp_num > 0)
		qp_num = max_qp_num;

	cpu_num = sss_nic_get_online_cpu(nic_dev->pdev);

	nic_dev->qp_res.qp_num = (u16)min_t(u16, qp_num, cpu_num);
}

static void sss_nic_set_rss_hkey(struct sss_nic_dev *nic_dev, const u8 *key)
{
	u32 i;
	u32 *rss_hkey = (u32 *)nic_dev->rss_key;

	memcpy(nic_dev->rss_key, key, SSSNIC_RSS_KEY_SIZE);

	/* make a copy of the key, and convert it to Big Endian */
	for (i = 0; i < SSSNIC_RSS_KEY_SIZE / sizeof(u32); i++)
		nic_dev->rss_key_big[i] = cpu_to_be32(rss_hkey[i]);
}

static void sss_nic_init_rss_default_key(struct sss_nic_dev *nic_dev)
{
	u8 default_key[SSSNIC_RSS_KEY_SIZE] = {
		SSSNIC_DEF_RSS_KEY_0, SSSNIC_DEF_RSS_KEY_1, SSSNIC_DEF_RSS_KEY_2,
		SSSNIC_DEF_RSS_KEY_3, SSSNIC_DEF_RSS_KEY_4, SSSNIC_DEF_RSS_KEY_5,
		SSSNIC_DEF_RSS_KEY_6, SSSNIC_DEF_RSS_KEY_7, SSSNIC_DEF_RSS_KEY_8,
		SSSNIC_DEF_RSS_KEY_9, SSSNIC_DEF_RSS_KEY_10, SSSNIC_DEF_RSS_KEY_11,
		SSSNIC_DEF_RSS_KEY_12, SSSNIC_DEF_RSS_KEY_13, SSSNIC_DEF_RSS_KEY_14,
		SSSNIC_DEF_RSS_KEY_15, SSSNIC_DEF_RSS_KEY_16, SSSNIC_DEF_RSS_KEY_17,
		SSSNIC_DEF_RSS_KEY_18, SSSNIC_DEF_RSS_KEY_19, SSSNIC_DEF_RSS_KEY_20,
		SSSNIC_DEF_RSS_KEY_21, SSSNIC_DEF_RSS_KEY_22, SSSNIC_DEF_RSS_KEY_23,
		SSSNIC_DEF_RSS_KEY_24, SSSNIC_DEF_RSS_KEY_25, SSSNIC_DEF_RSS_KEY_26,
		SSSNIC_DEF_RSS_KEY_27, SSSNIC_DEF_RSS_KEY_28, SSSNIC_DEF_RSS_KEY_29,
		SSSNIC_DEF_RSS_KEY_30, SSSNIC_DEF_RSS_KEY_31, SSSNIC_DEF_RSS_KEY_32,
		SSSNIC_DEF_RSS_KEY_33, SSSNIC_DEF_RSS_KEY_34, SSSNIC_DEF_RSS_KEY_35,
		SSSNIC_DEF_RSS_KEY_36, SSSNIC_DEF_RSS_KEY_37, SSSNIC_DEF_RSS_KEY_38,
		SSSNIC_DEF_RSS_KEY_39
	};

	sss_nic_set_rss_hkey(nic_dev, default_key);
}

static int sss_nic_alloc_rss_key(struct sss_nic_dev *nic_dev)
{
	/* We need double the space to store the RSS key,
	 *  with the second space used to store the RSS key in big-endian mode.
	 */
	nic_dev->rss_key =
		kzalloc(SSSNIC_RSS_KEY_SIZE * SSSNIC_RSS_KEY_RSV_NUM, GFP_KERNEL);
	if (!nic_dev->rss_key) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to alloc memory for rss_hkey\n");
		return -ENOMEM;
	}

	nic_dev->rss_indir_tbl = kzalloc(sizeof(u32) * SSSNIC_RSS_INDIR_SIZE, GFP_KERNEL);
	if (!nic_dev->rss_indir_tbl) {
		kfree(nic_dev->rss_key);
		nic_dev->rss_key = NULL;
		return -ENOMEM;
	}

	/* The second space is for big edian hash key */
	nic_dev->rss_key_big = (u32 *)(nic_dev->rss_key + SSSNIC_RSS_KEY_SIZE);

	return 0;
}

static int sss_nic_config_rss_hw_resource(struct sss_nic_dev *nic_dev, u32 *indir)
{
	int ret;
	u8 engine_type = nic_dev->rss_hash_engine;

	ret = sss_nic_set_rss_indir_tbl(nic_dev, indir);
	if (ret != 0)
		return ret;

	ret = sss_nic_set_rss_type(nic_dev, nic_dev->rss_type);
	if (ret != 0)
		return ret;

	return sss_nic_rss_hash_engine(nic_dev, SSSNIC_MBX_OPCODE_SET, &engine_type);
}

static int sss_nic_set_hw_rss(struct net_device *netdev, u8 *cos_map, u8 cos_num)
{
	int ret;
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	bool dcb_en = !!SSSNIC_TEST_NIC_DEV_FLAG(nic_dev, SSSNIC_DCB_ENABLE);

	ret = sss_nic_cfg_rss_hash_key(nic_dev, SSSNIC_MBX_OPCODE_SET, nic_dev->rss_key);
	if (ret != 0)
		return ret;

	sss_nic_maybe_reset_rss_indir(netdev, dcb_en);

	if (test_bit(SSSNIC_RSS_DEFAULT_INDIR, &nic_dev->flags))
		sss_nic_fill_indir_tbl(nic_dev, cos_num, nic_dev->rss_indir_tbl);

	ret = sss_nic_config_rss_hw_resource(nic_dev, nic_dev->rss_indir_tbl);
	if (ret != 0)
		return ret;

	ret = sss_nic_config_rss_to_hw(nic_dev, cos_num, cos_map,
				       nic_dev->qp_res.qp_num, 1);
	if (ret != 0)
		return ret;

	return 0;
}

static void sss_nic_init_rss_key(struct sss_nic_dev *nic_dev)
{
	sss_nic_init_rss_default_key(nic_dev);

	sss_nic_init_qp_num(nic_dev);

	sss_nic_init_rss_type(nic_dev);

	sss_nic_fill_indir_tbl(nic_dev, 0, nic_dev->rss_indir_tbl);
}

static int sss_nic_set_rss_key_to_hw(struct sss_nic_dev *nic_dev)
{
	int ret;
	u8 engine_type = nic_dev->rss_hash_engine;

	ret = sss_nic_cfg_rss_hash_key(nic_dev, SSSNIC_MBX_OPCODE_SET, nic_dev->rss_key);
	if (ret != 0)
		return ret;

	ret = sss_nic_set_rss_indir_tbl(nic_dev, nic_dev->rss_indir_tbl);
	if (ret != 0)
		return ret;

	ret = sss_nic_set_rss_type(nic_dev, nic_dev->rss_type);
	if (ret != 0)
		return ret;

	ret = sss_nic_rss_hash_engine(nic_dev, SSSNIC_MBX_OPCODE_SET, &engine_type);
	if (ret != 0)
		return ret;

	ret = sss_nic_init_hw_rss(nic_dev, nic_dev->qp_res.qp_num);
	if (ret != 0)
		return ret;

	return 0;
}

void sss_nic_try_to_enable_rss(struct sss_nic_dev *nic_dev)
{
	int ret = 0;

	if (!SSSNIC_SUPPORT_RSS(nic_dev->nic_io) || nic_dev->max_qp_num <= 1) {
		nic_dev->qp_res.qp_num = nic_dev->max_qp_num;
		return;
	}

	ret = sss_nic_alloc_rss_key(nic_dev);
	if (ret != 0)
		goto disable_rss;

	set_bit(SSSNIC_RSS_ENABLE, &nic_dev->flags);
	set_bit(SSSNIC_RSS_DEFAULT_INDIR, &nic_dev->flags);
	sss_nic_init_rss_key(nic_dev);

	ret = sss_nic_set_rss_key_to_hw(nic_dev);
	if (ret != 0) {
		sss_nic_free_rss_key(nic_dev);
		nic_err(nic_dev->dev_hdl, "Fail to set hardware rss parameters\n");
		goto disable_rss;
	}

	return;

disable_rss:
	clear_bit(SSSNIC_RSS_ENABLE, &nic_dev->flags);
	nic_dev->max_qp_num = 1;
	nic_dev->qp_res.qp_num = nic_dev->max_qp_num;
}

/* for ethtool */
static int sss_nic_set_l4_rss_hash_type(const struct ethtool_rxnfc *cmd,
					struct sss_nic_rss_type *rss_type)
{
	u8 rss_l4_en = 0;

	if ((cmd->data & SSSNIC_RXH_PORT) == 0)
		rss_l4_en = 0;
	else if ((cmd->data & SSSNIC_RXH_PORT) == SSSNIC_RXH_PORT)
		rss_l4_en = 1;
	else
		return -EINVAL;

	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
		rss_type->tcp_ipv4 = rss_l4_en;
		break;
	case TCP_V6_FLOW:
		rss_type->tcp_ipv6 = rss_l4_en;
		break;
	case UDP_V4_FLOW:
		rss_type->udp_ipv4 = rss_l4_en;
		break;
	case UDP_V6_FLOW:
		rss_type->udp_ipv6 = rss_l4_en;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sss_nic_update_rss_type(struct sss_nic_dev *nic_dev,
				   struct ethtool_rxnfc *cmd,
				   struct sss_nic_rss_type *rss_type)
{
	int ret;

	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
	case UDP_V4_FLOW:
	case TCP_V6_FLOW:
	case UDP_V6_FLOW:
		ret = sss_nic_set_l4_rss_hash_type(cmd, rss_type);
		if (ret != 0)
			return ret;

		break;
	case IPV4_FLOW:
		rss_type->ipv4 = 1;
		break;
	case IPV6_FLOW:
		rss_type->ipv6 = 1;
		break;
	default:
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unsupport flow type\n");
		return -EINVAL;
	}

	return 0;
}

static inline int sss_nic_check_cmd_data(struct ethtool_rxnfc *cmd)
{
	/* RSS only support hashing to queues based src and dst IP and port */
	if (cmd->data & ~SSSNIC_SUPPORT_RXH)
		return -EINVAL;

	/* We need at least the IP SRC and DEST fields for hashing */
	if (!(cmd->data & SSSNIC_RXH_IP))
		return -EINVAL;

	return 0;
}

static int sss_nic_set_rss_hash_type(struct sss_nic_dev *nic_dev, struct ethtool_rxnfc *cmd)
{
	struct sss_nic_rss_type *rss_type = &nic_dev->rss_type;
	int ret;

	if (test_bit(SSSNIC_RSS_ENABLE, &nic_dev->flags) == 0) {
		cmd->data = 0;
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "RSS disable, no support to set flow-hash\n");
		return -EOPNOTSUPP;
	}

	if (sss_nic_check_cmd_data(cmd) != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Invalid ethool rxnfc cmd data\n");
		return -EINVAL;
	}

	ret = sss_nic_get_rss_type(nic_dev, rss_type);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to get rss type\n");
		return -EFAULT;
	}

	ret = sss_nic_update_rss_type(nic_dev, cmd, rss_type);
	if (ret != 0)
		return ret;

	ret = sss_nic_set_rss_type(nic_dev, *rss_type);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to set rss type\n");
		return -EFAULT;
	}

	nicif_info(nic_dev, drv, nic_dev->netdev, "Success to set rss hash options\n");

	return 0;
}

static void translate_rss_type(u8 rss_opt, struct ethtool_rxnfc *cmd)
{
	if (rss_opt != 0)
		cmd->data |= SSSNIC_RXH_PORT;
}

static int sss_nic_translate_rss_type(struct sss_nic_dev *nic_dev,
				      struct sss_nic_rss_type *rss_type,
				      struct ethtool_rxnfc *cmd)
{
	cmd->data = SSSNIC_RXH_IP;
	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
		translate_rss_type(rss_type->tcp_ipv4, cmd);
		break;
	case UDP_V4_FLOW:
		translate_rss_type(rss_type->udp_ipv4, cmd);
		break;
	case TCP_V6_FLOW:
		translate_rss_type(rss_type->tcp_ipv6, cmd);
		break;
	case UDP_V6_FLOW:
		translate_rss_type(rss_type->udp_ipv6, cmd);
		break;
	case IPV4_FLOW:
	case IPV6_FLOW:
		break;
	default:
		nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupport flow type\n");
		cmd->data = 0;
		return -EINVAL;
	}

	return 0;
}

static int sss_nic_get_rss_hash_type(struct sss_nic_dev *nic_dev, struct ethtool_rxnfc *cmd)
{
	struct sss_nic_rss_type rss_type = {0};
	int ret;

	cmd->data = 0;

	if (test_bit(SSSNIC_RSS_ENABLE, &nic_dev->flags) == 0)
		return 0;

	ret = sss_nic_get_rss_type(nic_dev, &rss_type);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to get rss type\n");
		return ret;
	}

	return sss_nic_translate_rss_type(nic_dev, &rss_type, cmd);
}

int sss_nic_get_rxnfc(struct net_device *netdev,
		      struct ethtool_rxnfc *cmd, u32 *rule_locs)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	int ret = 0;

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		cmd->data = nic_dev->qp_res.qp_num;
		break;
	case ETHTOOL_GRXCLSRLCNT:
		cmd->rule_cnt = (u32)nic_dev->rx_rule.rule_cnt;
		break;
	case ETHTOOL_GRXCLSRULE:
		ret = sss_nic_ethtool_get_flow(nic_dev, cmd, cmd->fs.location);
		break;
	case ETHTOOL_GRXCLSRLALL:
		ret = sss_nic_ethtool_get_all_flows(nic_dev, cmd, rule_locs);
		break;
	case ETHTOOL_GRXFH:
		ret = sss_nic_get_rss_hash_type(nic_dev, cmd);
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	return ret;
}

int sss_nic_set_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *cmd)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	int ret = 0;

	switch (cmd->cmd) {
	case ETHTOOL_SRXFH:
		ret = sss_nic_set_rss_hash_type(nic_dev, cmd);
		break;
	case ETHTOOL_SRXCLSRLINS:
		ret = sss_nic_ethtool_update_flow(nic_dev, &cmd->fs);
		break;
	case ETHTOOL_SRXCLSRLDEL:
		ret = sss_nic_ethtool_delete_flow(nic_dev, cmd->fs.location);
		break;
	default:
		ret = -EOPNOTSUPP;
		break;
	}

	return ret;
}

static u16 sss_nic_channels_max(struct sss_nic_dev *nic_dev)
{
	u8 tcs = (u8)netdev_get_num_tc(nic_dev->netdev);

	return tcs ? nic_dev->max_qp_num / tcs : nic_dev->max_qp_num;
}

static u16 sss_nic_curr_channels(struct sss_nic_dev *nic_dev)
{
	if (netif_running(nic_dev->netdev))
		return nic_dev->qp_res.qp_num ?
		       nic_dev->qp_res.qp_num : 1;
	else
		return (u16)min_t(u16, sss_nic_channels_max(nic_dev),
				  nic_dev->qp_res.qp_num);
}

void sss_nic_get_channels(struct net_device *netdev,
			  struct ethtool_channels *channels)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);

	channels->tx_count = 0;
	channels->rx_count = 0;
	channels->other_count = 0;
	channels->max_tx = 0;
	channels->max_rx = 0;
	channels->max_other = 0;
	channels->max_combined = sss_nic_channels_max(nic_dev);
	/* report flow director queues as maximum channels */
	channels->combined_count = sss_nic_curr_channels(nic_dev);
}

static int sss_nic_check_channel_parameter(struct net_device *netdev,
					   const struct ethtool_channels *channels)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	unsigned int combined_count = channels->combined_count;
	u16 max_channel = sss_nic_channels_max(nic_dev);

	if (combined_count == 0) {
		nicif_err(nic_dev, drv, netdev,
			  "Unsupport combined_count=0\n");
		return -EINVAL;
	}

	if (channels->tx_count != 0 || channels->rx_count != 0 ||
	    channels->other_count != 0) {
		nicif_err(nic_dev, drv, netdev,
			  "Set rx/tx/other count no support\n");
		return -EINVAL;
	}

	if (combined_count > max_channel) {
		nicif_err(nic_dev, drv, netdev,
			  "Invalid combined_count %u out of range %u\n", combined_count,
			  max_channel);
		return -EINVAL;
	}

	return 0;
}

static void sss_nic_change_num_channel_reopen_handler(struct sss_nic_dev *nic_dev,
						      const void *priv_data)
{
	sss_nic_set_default_rss_indir(nic_dev->netdev);
}

int sss_nic_set_channels(struct net_device *netdev,
			 struct ethtool_channels *channels)
{
	struct sss_nic_qp_resource q_param = {0};
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	unsigned int combined_count = channels->combined_count;
	u8 user_cos_num = sss_nic_get_user_cos_num(nic_dev);
	int ret;

	if (sss_nic_check_channel_parameter(netdev, channels))
		return -EINVAL;

	if (!test_bit(SSSNIC_RSS_ENABLE, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, netdev,
			  "This function not support RSS, only support 1 queue pair\n");
		return -EOPNOTSUPP;
	}

	if (test_bit(SSSNIC_DCB_ENABLE, &nic_dev->flags)) {
		if (combined_count < user_cos_num) {
			nicif_err(nic_dev, drv, netdev,
				  "DCB is on, channel num should more than valid cos num:%u\n",
				  user_cos_num);
			return -EOPNOTSUPP;
		}
	}

	if (SSSNIC_SUPPORT_FDIR(nic_dev->nic_io) &&
	    !sss_nic_validate_channel_setting_in_ntuple(nic_dev, combined_count))
		return -EOPNOTSUPP;

	nicif_info(nic_dev, drv, netdev, "Set max combine queue number from %u to %u\n",
		   nic_dev->qp_res.qp_num, combined_count);

	if (netif_running(netdev)) {
		q_param = nic_dev->qp_res;
		q_param.irq_cfg = NULL;
		q_param.rq_res_group = NULL;
		q_param.sq_res_group = NULL;
		q_param.qp_num = (u16)combined_count;

		nicif_info(nic_dev, drv, netdev, "Restart channel\n");
		ret = sss_nic_update_channel_setting(nic_dev, &q_param,
						     sss_nic_change_num_channel_reopen_handler,
						     NULL);
		if (ret != 0) {
			nicif_err(nic_dev, drv, netdev, "Fail to change channel setting\n");
			return -EFAULT;
		}
	} else {
		/* Discard user configured rss */
		sss_nic_set_default_rss_indir(netdev);
		nic_dev->qp_res.qp_num = (u16)combined_count;
	}

	return 0;
}

#ifndef NOT_HAVE_GET_RXFH_INDIR_SIZE
u32 sss_nic_get_rxfh_indir_size(struct net_device *netdev)
{
	return SSSNIC_RSS_INDIR_SIZE;
}
#endif

static int sss_nic_set_rss_rxfh(struct net_device *netdev, const u32 *indir,
				const u8 *hash_key)
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	int ret;

	if (indir) {
		ret = sss_nic_set_rss_indir_tbl(nic_dev, indir);
		if (ret != 0) {
			nicif_err(nic_dev, drv, netdev,
				  "Fail to set rss indir table\n");
			return -EFAULT;
		}
		clear_bit(SSSNIC_RSS_DEFAULT_INDIR, &nic_dev->flags);

		memcpy(nic_dev->rss_indir_tbl, indir,
		       sizeof(u32) * SSSNIC_RSS_INDIR_SIZE);
		nicif_info(nic_dev, drv, netdev, "Success to set rss indir\n");
	}

	if (hash_key) {
		ret = sss_nic_set_rss_hash_key(nic_dev, hash_key);
		if (ret != 0) {
			nicif_err(nic_dev, drv, netdev, "Fail to set rss key\n");
			return -EFAULT;
		}

		sss_nic_set_rss_hkey(nic_dev, hash_key);
		nicif_info(nic_dev, drv, netdev, "Success to set rss key\n");
	}

	return 0;
}

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
u32 sss_nic_get_rxfh_key_size(struct net_device *netdev)
{
	return SSSNIC_RSS_KEY_SIZE;
}

#ifdef HAVE_RXFH_HASHFUNC
int sss_nic_get_rxfh(struct net_device *netdev, u32 *indir, u8 *hash_key, u8 *hfunc)
#else
int sss_nic_get_rxfh(struct net_device *netdev, u32 *indir, u8 *hash_key)
#endif
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	int ret = 0;

	if (!test_bit(SSSNIC_RSS_ENABLE, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Rss is disable\n");
		return -EOPNOTSUPP;
	}

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc)
		*hfunc = nic_dev->rss_hash_engine ?
			 ETH_RSS_HASH_TOP : ETH_RSS_HASH_XOR;
#endif

	if (indir) {
		ret = sss_nic_get_rss_indir_tbl(nic_dev, indir);
		if (ret != 0)
			return -EFAULT;
	}

	if (hash_key)
		memcpy(hash_key, nic_dev->rss_key, SSSNIC_RSS_KEY_SIZE);

	return ret;
}

#ifdef HAVE_RXFH_HASHFUNC
int sss_nic_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *hash_key,
		     const u8 hfunc)
#else
#ifdef HAVE_RXFH_NONCONST
int sss_nic_set_rxfh(struct net_device *netdev, u32 *indir, u8 *hash_key)
#else
int sss_nic_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *hash_key)
#endif
#endif /* HAVE_RXFH_HASHFUNC */
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	int ret = 0;

	if (!test_bit(SSSNIC_RSS_ENABLE, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "No support to set rss parameters when rss disable\n");
		return -EOPNOTSUPP;
	}

	if (test_bit(SSSNIC_DCB_ENABLE, &nic_dev->flags) && indir) {
		nicif_err(nic_dev, drv, netdev,
			  "No support to set indir when DCB enable\n");
		return -EOPNOTSUPP;
	}

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc != ETH_RSS_HASH_NO_CHANGE) {
		if (hfunc != ETH_RSS_HASH_TOP && hfunc != ETH_RSS_HASH_XOR) {
			nicif_err(nic_dev, drv, netdev,
				  "No support to set hfunc type except TOP and XOR\n");
			return -EOPNOTSUPP;
		}

		nic_dev->rss_hash_engine = (hfunc == ETH_RSS_HASH_XOR) ?
					   SSSNIC_RSS_ENGINE_XOR :
					   SSSNIC_RSS_ENGINE_TOEP;
		ret = sss_nic_set_rss_hash_engine(nic_dev,
						  nic_dev->rss_hash_engine);
		if (ret != 0)
			return -EFAULT;

		nicif_info(nic_dev, drv, netdev,
			   "Success to set hfunc to RSS_HASH_%s\n",
			   (hfunc == ETH_RSS_HASH_XOR) ? "XOR" : "TOP");
	}
#endif
	ret = sss_nic_set_rss_rxfh(netdev, indir, hash_key);

	return ret;
}

#else /* !(defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)) */

#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
int sss_nic_get_rxfh_indir(struct net_device *netdev,
			   struct ethtool_rxfh_indir *rxfh_indir)
#else
int sss_nic_get_rxfh_indir(struct net_device *netdev, u32 *indir)
#endif
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
	int ret = 0;
#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
	u32 *indir = NULL;

	/* In a low version kernel(eg:suse 11.2), call the interface twice.
	 * First call to get the size value,
	 * and second call to get the rxfh indir according to the size value.
	 */
	if (rxfh_indir->size == 0) {
		rxfh_indir->size = SSSNIC_RSS_INDIR_SIZE;
		return 0;
	}

	if (rxfh_indir->size < SSSNIC_RSS_INDIR_SIZE) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to get rss indir, rss size(%d) less than default rss size(%u).\n",
			  rxfh_indir->size, SSSNIC_RSS_INDIR_SIZE);
		return -EINVAL;
	}

	indir = rxfh_indir->ring_index;
#endif
	if (!test_bit(SSSNIC_RSS_ENABLE, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "No support to get rss when rss disable\n");
		return -EOPNOTSUPP;
	}

	if (indir)
		ret = sss_nic_get_rss_indir_tbl(nic_dev, indir);

	return ret;
}

#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
int sss_nic_set_rxfh_indir(struct net_device *netdev,
			   const struct ethtool_rxfh_indir *rxfh_indir)
#else
int sss_nic_set_rxfh_indir(struct net_device *netdev, const u32 *indir)
#endif
{
	struct sss_nic_dev *nic_dev = netdev_priv(netdev);
#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
	const u32 *indir = NULL;

	if (rxfh_indir->size != SSSNIC_RSS_INDIR_SIZE) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Fail to set rss indir, rss size(%d) is less than default rss size(%u).\n",
			  rxfh_indir->size, SSSNIC_RSS_INDIR_SIZE);
		return -EINVAL;
	}

	indir = rxfh_indir->ring_index;
#endif

	if (!test_bit(SSSNIC_RSS_ENABLE, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "No support to set rss indir when rss disable\n");
		return -EOPNOTSUPP;
	}

	if (test_bit(SSSNIC_DCB_ENABLE, &nic_dev->flags) && indir) {
		nicif_err(nic_dev, drv, netdev,
			  "No support to set indir when DCB enable\n");
		return -EOPNOTSUPP;
	}

	return sss_nic_set_rss_rxfh(netdev, indir, NULL);
}

#endif /* defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH) */

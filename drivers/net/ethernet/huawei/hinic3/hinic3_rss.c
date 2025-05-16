// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

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
#include <linux/init.h>

#include "ossl_knl.h"
#include "hinic3_crm.h"
#include "hinic3_nic_cfg.h"
#include "hinic3_nic_dev.h"
#include "hinic3_hw.h"
#include "hinic3_rss.h"

static u16 num_qps;
module_param(num_qps, ushort, 0444);
MODULE_PARM_DESC(num_qps, "Number of Queue Pairs (default=0)");

#define MOD_PARA_VALIDATE_NUM_QPS(nic_dev, num_qps, out_qps)	do {	\
	if ((num_qps) > (nic_dev)->max_qps)				\
		nic_warn(&(nic_dev)->pdev->dev,				\
		"Module Parameter %s value %u is out of range, "		\
		"Maximum value for the device: %u, using %u\n",		 \
		#num_qps, num_qps, (nic_dev)->max_qps,				\
		(nic_dev)->max_qps);						\
	if ((num_qps) > (nic_dev)->max_qps)				\
		(out_qps) = (nic_dev)->max_qps;				\
	else if ((num_qps) > 0)						\
		(out_qps) = (num_qps);					\
} while (0)

/* In rx, iq means cos */
static u8 hinic3_get_iqmap_by_tc(const u8 *prio_tc, u8 num_iq, u8 tc)
{
	u8 i, map = 0;

	for (i = 0; i < num_iq; i++) {
		if (prio_tc[i] == tc)
			map |= (u8)(1U << ((num_iq - 1) - i));
	}

	return map;
}

static u8 hinic3_get_tcid_by_rq(const u32 *indir_tbl, u8 num_tcs, u16 rq_id)
{
	u16 tc_group_size;
	int i;
	u8 temp_num_tcs = num_tcs;

	if (!num_tcs)
		temp_num_tcs = 1;

	tc_group_size = NIC_RSS_INDIR_SIZE / temp_num_tcs;
	for (i = 0; i < NIC_RSS_INDIR_SIZE; i++) {
		if (indir_tbl[i] == rq_id)
			return (u8)(i / tc_group_size);
		}

	return 0xFF; /* Invalid TC */
}

static int hinic3_get_rq2iq_map(struct hinic3_nic_dev *nic_dev,
				u16 num_rq, u8 num_tcs, u8 *prio_tc, u8 cos_num,
				u32 *indir_tbl, u8 *map, u32 map_size)
{
	u16 qid;
	u8 tc_id;
	u8 temp_num_tcs = num_tcs;

	if (!num_tcs)
		temp_num_tcs = 1;

	if (num_rq > map_size) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Rq number(%u) exceed max map qid(%u)\n",
			  num_rq, map_size);
		return -EINVAL;
	}

	if (cos_num < HINIC_NUM_IQ_PER_FUNC) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Cos number(%u) less then map qid(%d)\n",
			  cos_num, HINIC_NUM_IQ_PER_FUNC);
		return -EINVAL;
	}

	for (qid = 0; qid < num_rq; qid++) {
		tc_id = hinic3_get_tcid_by_rq(indir_tbl, temp_num_tcs, qid);
		map[qid] = hinic3_get_iqmap_by_tc(prio_tc,
						  HINIC_NUM_IQ_PER_FUNC, tc_id);
	}

	return 0;
}

static void hinic3_fillout_indir_tbl(struct hinic3_nic_dev *nic_dev,
				     u8 group_num, u32 *indir)
{
	struct hinic3_dcb *dcb = nic_dev->dcb;
	u16 k, group_size, start_qid = 0, cur_cos_qnum = 0;
	u32 i = 0;
	u8 j, cur_cos = 0, group = 0;
	u8 valid_cos_map = hinic3_get_dev_valid_cos_map(nic_dev);

	if (group_num == 0) {
		for (i = 0; i < NIC_RSS_INDIR_SIZE; i++)
			indir[i] = i % nic_dev->q_params.num_qps;
	} else {
		group_size = NIC_RSS_INDIR_SIZE / group_num;

		for (group = 0; group < group_num; group++) {
			cur_cos = dcb->hw_dcb_cfg.default_cos;
			for (j = 0; j < NIC_DCB_COS_MAX; j++) {
				if ((BIT(j) & valid_cos_map) != 0) {
					cur_cos = j;
					valid_cos_map -= (u8)BIT(j);
					break;
				}
			}

			cur_cos_qnum = dcb->hw_dcb_cfg.cos_qp_num[cur_cos];
			if (cur_cos_qnum > 0) {
				start_qid =
					dcb->hw_dcb_cfg.cos_qp_offset[cur_cos];
			} else {
				start_qid = cur_cos % nic_dev->q_params.num_qps;
				/* Ensure that the offset of start_id is 0. */
				cur_cos_qnum = 1;
			}

			for (k = 0; k < group_size; k++)
				indir[i++] = start_qid + k % cur_cos_qnum;
		}
	}
}

int hinic3_rss_init(struct hinic3_nic_dev *nic_dev, u8 *rq2iq_map, u32 map_size, u8 dcb_en)
{
	struct net_device *netdev = nic_dev->netdev;
	u8 i, group_num, cos_bitmap, group = 0;
	u8 cos_group[NIC_DCB_UP_MAX] = {0};
	int err;

	if (dcb_en != 0) {
		group_num = (u8)roundup_pow_of_two(hinic3_get_dev_user_cos_num(nic_dev));

		cos_bitmap = hinic3_get_dev_valid_cos_map(nic_dev);

		for (i = 0; i < NIC_DCB_UP_MAX; i++) {
			if ((BIT(i) & cos_bitmap) != 0)
				cos_group[NIC_DCB_UP_MAX - i - 1] = group++;
			else
				cos_group[NIC_DCB_UP_MAX - i - 1] =
								group_num - 1;
		}
	} else {
		group_num = 0;
	}

	err = hinic3_set_hw_rss_parameters(netdev, 1, group_num,
					   cos_group, dcb_en);
	if (err)
		return err;

	err = hinic3_get_rq2iq_map(nic_dev, nic_dev->q_params.num_qps,
				   group_num, cos_group, NIC_DCB_UP_MAX,
				   nic_dev->rss_indir, rq2iq_map, map_size);
	if (err)
		nicif_err(nic_dev, drv, netdev, "Failed to get rq map\n");
	return err;
}

void hinic3_rss_deinit(struct hinic3_nic_dev *nic_dev)
{
	u8 cos_map[NIC_DCB_UP_MAX] = {0};

	hinic3_rss_cfg(nic_dev->hwdev, 0, 0, cos_map, 1);
}

void hinic3_init_rss_parameters(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	nic_dev->rss_hash_engine = HINIC3_RSS_HASH_ENGINE_TYPE_XOR;
	nic_dev->rss_type.tcp_ipv6_ext = 1;
	nic_dev->rss_type.ipv6_ext = 1;
	nic_dev->rss_type.tcp_ipv6 = 1;
	nic_dev->rss_type.ipv6 = 1;
	nic_dev->rss_type.tcp_ipv4 = 1;
	nic_dev->rss_type.ipv4 = 1;
	nic_dev->rss_type.udp_ipv6 = 1;
	nic_dev->rss_type.udp_ipv4 = 1;
}

void hinic3_clear_rss_config(struct hinic3_nic_dev *nic_dev)
{
	kfree(nic_dev->rss_hkey);
	nic_dev->rss_hkey = NULL;

	kfree(nic_dev->rss_indir);
	nic_dev->rss_indir = NULL;
}

void hinic3_set_default_rss_indir(struct net_device *netdev)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	set_bit(HINIC3_RSS_DEFAULT_INDIR, &nic_dev->flags);
}

static void hinic3_maybe_reconfig_rss_indir(struct net_device *netdev, u8 dcb_en)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int i;

	/* if dcb is enabled, user can not config rss indir table */
	if (dcb_en) {
		nicif_info(nic_dev, drv, netdev, "DCB is enabled, set default rss indir\n");
		goto discard_user_rss_indir;
	}

	for (i = 0; i < NIC_RSS_INDIR_SIZE; i++) {
		if (nic_dev->rss_indir[i] >= nic_dev->q_params.num_qps)
			goto discard_user_rss_indir;
	}

	return;

discard_user_rss_indir:
	hinic3_set_default_rss_indir(netdev);
}

#ifdef HAVE_HOT_REPLACE_FUNC
bool partition_slave_doing_hotupgrade(void)
{
	return get_partition_role() && partition_doing_hotupgrade();
}
#endif

static void decide_num_qps(struct hinic3_nic_dev *nic_dev)
{
	u16 tmp_num_qps = nic_dev->max_qps;
	u16 num_cpus = 0;
	u16 max_num_cpus;
	int i, node;
	int is_in_kexec = vram_get_kexec_flag();

	if (is_in_kexec != 0) {
		nic_dev->q_params.num_qps = nic_dev->nic_vram->vram_num_qps;
		nicif_info(nic_dev, drv, nic_dev->netdev,
			   "Os hotreplace use vram to init num qps 1:%u 2:%u\n",
			   nic_dev->q_params.num_qps,
			   nic_dev->nic_vram->vram_num_qps);
		return;
	}

	if (nic_dev->nic_cap.default_num_queues != 0 &&
	    nic_dev->nic_cap.default_num_queues < nic_dev->max_qps)
		tmp_num_qps = nic_dev->nic_cap.default_num_queues;

	MOD_PARA_VALIDATE_NUM_QPS(nic_dev, num_qps, tmp_num_qps);

#ifdef HAVE_HOT_REPLACE_FUNC
	if (partition_slave_doing_hotupgrade())
		max_num_cpus = (u16)num_present_cpus();
	else
		max_num_cpus = (u16)num_online_cpus();
#else
	max_num_cpus = (u16)num_online_cpus();
#endif

	for (i = 0; i < max_num_cpus; i++) {
		node = (int)cpu_to_node(i);
		if (node == dev_to_node(&nic_dev->pdev->dev))
			num_cpus++;
	}

	if (!num_cpus)
		num_cpus = max_num_cpus;

	nic_dev->q_params.num_qps = (u16)min_t(u16, tmp_num_qps, num_cpus);
	nic_dev->nic_vram->vram_num_qps = nic_dev->q_params.num_qps;
	nicif_info(nic_dev, drv, nic_dev->netdev,
		   "init num qps 1:%u 2:%u\n",
		   nic_dev->q_params.num_qps, nic_dev->nic_vram->vram_num_qps);
}

static void copy_value_to_rss_hkey(struct hinic3_nic_dev *nic_dev,
				   const u8 *hkey)
{
	u32 i;
	u32 *rss_hkey = (u32 *)nic_dev->rss_hkey;

	memcpy(nic_dev->rss_hkey, hkey, NIC_RSS_KEY_SIZE);

	/* make a copy of the key, and convert it to Big Endian */
	for (i = 0; i < NIC_RSS_KEY_SIZE / sizeof(u32); i++)
		nic_dev->rss_hkey_be[i] = cpu_to_be32(rss_hkey[i]);
}

static int alloc_rss_resource(struct hinic3_nic_dev *nic_dev)
{
	u8 default_rss_key[NIC_RSS_KEY_SIZE] = {
		0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2,
		0x41, 0x67, 0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0,
		0xd0, 0xca, 0x2b, 0xcb, 0xae, 0x7b, 0x30, 0xb4,
		0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30, 0xf2, 0x0c,
		0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa};

	/* We request double spaces for the hash key,
	 * the second one holds the key of Big Edian
	 * format.
	 */
	nic_dev->rss_hkey =
		kzalloc(NIC_RSS_KEY_SIZE *
			HINIC3_RSS_KEY_RSV_NUM, GFP_KERNEL);
	if (!nic_dev->rss_hkey) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to alloc memory for rss_hkey\n");
		return -ENOMEM;
	}

	/* The second space is for big edian hash key */
	nic_dev->rss_hkey_be = (u32 *)(nic_dev->rss_hkey +
					   NIC_RSS_KEY_SIZE);
	copy_value_to_rss_hkey(nic_dev, (u8 *)default_rss_key);

	nic_dev->rss_indir = kzalloc(sizeof(u32) * NIC_RSS_INDIR_SIZE, GFP_KERNEL);
	if (!nic_dev->rss_indir) {
		kfree(nic_dev->rss_hkey);
		nic_dev->rss_hkey = NULL;
		return -ENOMEM;
	}

	set_bit(HINIC3_RSS_DEFAULT_INDIR, &nic_dev->flags);

	return 0;
}

void hinic3_try_to_enable_rss(struct hinic3_nic_dev *nic_dev)
{
	u8 cos_map[NIC_DCB_UP_MAX] = {0};
	int err = 0;

	if (!nic_dev)
		return;

	nic_dev->max_qps = hinic3_func_max_nic_qnum(nic_dev->hwdev);
	if (nic_dev->max_qps <= 1 || !HINIC3_SUPPORT_RSS(nic_dev->hwdev))
		goto set_q_params;

	err = alloc_rss_resource(nic_dev);
	if (err) {
		nic_dev->max_qps = 1;
		goto set_q_params;
	}

	set_bit(HINIC3_RSS_ENABLE, &nic_dev->flags);
	nic_dev->max_qps = hinic3_func_max_nic_qnum(nic_dev->hwdev);

	decide_num_qps(nic_dev);

	hinic3_init_rss_parameters(nic_dev->netdev);
	err = hinic3_set_hw_rss_parameters(nic_dev->netdev, 0, 0, cos_map,
					   test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags) ? 1 : 0);
	if (err) {
		nic_err(&nic_dev->pdev->dev, "Failed to set hardware rss parameters\n");

		hinic3_clear_rss_config(nic_dev);
		nic_dev->max_qps = 1;
		goto set_q_params;
	}
	return;

set_q_params:
	clear_bit(HINIC3_RSS_ENABLE, &nic_dev->flags);
	nic_dev->q_params.num_qps = nic_dev->max_qps;
	nic_dev->nic_vram->vram_num_qps = nic_dev->max_qps;
}

static int hinic3_config_rss_hw_resource(struct hinic3_nic_dev *nic_dev,
					 u32 *indir_tbl)
{
	int err;

	err = hinic3_rss_set_indir_tbl(nic_dev->hwdev, indir_tbl);
	if (err)
		return err;

	err = hinic3_set_rss_type(nic_dev->hwdev, nic_dev->rss_type);
	if (err)
		return err;

	return hinic3_rss_set_hash_engine(nic_dev->hwdev,
					  nic_dev->rss_hash_engine);
}

int hinic3_set_hw_rss_parameters(struct net_device *netdev, u8 rss_en,
				 u8 cos_num, u8 *cos_map, u8 dcb_en)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int err;

	/* RSS key */
	err = hinic3_rss_set_hash_key(nic_dev->hwdev, nic_dev->rss_hkey);
	if (err)
		return err;

	hinic3_maybe_reconfig_rss_indir(netdev, dcb_en);

	if (test_bit(HINIC3_RSS_DEFAULT_INDIR, &nic_dev->flags))
		hinic3_fillout_indir_tbl(nic_dev, cos_num, nic_dev->rss_indir);

	err = hinic3_config_rss_hw_resource(nic_dev, nic_dev->rss_indir);
	if (err)
		return err;

	err = hinic3_rss_cfg(nic_dev->hwdev, rss_en, cos_num, cos_map,
			     nic_dev->q_params.num_qps);
	if (err)
		return err;

	return 0;
}

/* for ethtool */
static int set_l4_rss_hash_ops(const struct ethtool_rxnfc *cmd,
			       struct nic_rss_type *rss_type)
{
	u8 rss_l4_en = 0;

	switch (cmd->data & (RXH_L4_B_0_1 | RXH_L4_B_2_3)) {
	case 0:
		rss_l4_en = 0;
		break;
	case (RXH_L4_B_0_1 | RXH_L4_B_2_3):
		rss_l4_en = 1;
		break;
	default:
		return -EINVAL;
	}

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

static int update_rss_hash_opts(struct hinic3_nic_dev *nic_dev,
				struct ethtool_rxnfc *cmd,
				struct nic_rss_type *rss_type)
{
	int err;

	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
	case TCP_V6_FLOW:
	case UDP_V4_FLOW:
	case UDP_V6_FLOW:
		err = set_l4_rss_hash_ops(cmd, rss_type);
		if (err)
			return err;

		break;
	case IPV4_FLOW:
		rss_type->ipv4 = 1;
		break;
	case IPV6_FLOW:
		rss_type->ipv6 = 1;
		break;
	default:
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Unsupported flow type\n");
		return -EINVAL;
	}

	return 0;
}

static int hinic3_set_rss_hash_opts(struct hinic3_nic_dev *nic_dev, struct ethtool_rxnfc *cmd)
{
	struct nic_rss_type *rss_type = &nic_dev->rss_type;
	int err;

	if (!test_bit(HINIC3_RSS_ENABLE, &nic_dev->flags)) {
		cmd->data = 0;
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "RSS is disable, not support to set flow-hash\n");
		return -EOPNOTSUPP;
	}

	/* RSS does not support anything other than hashing
	 * to queues on src and dst IPs and ports
	 */
	if (cmd->data & ~(RXH_IP_SRC | RXH_IP_DST | RXH_L4_B_0_1 |
		RXH_L4_B_2_3))
		return -EINVAL;

	/* We need at least the IP SRC and DEST fields for hashing */
	if (!(cmd->data & RXH_IP_SRC) || !(cmd->data & RXH_IP_DST))
		return -EINVAL;

	err = hinic3_get_rss_type(nic_dev->hwdev, rss_type);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Failed to get rss type\n");
		return -EFAULT;
	}

	err = update_rss_hash_opts(nic_dev, cmd, rss_type);
	if (err)
		return err;

	err = hinic3_set_rss_type(nic_dev->hwdev, *rss_type);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to set rss type\n");
		return -EFAULT;
	}

	nicif_info(nic_dev, drv, nic_dev->netdev, "Set rss hash options success\n");

	return 0;
}

static void convert_rss_type(u8 rss_opt, struct ethtool_rxnfc *cmd)
{
	if (rss_opt)
		cmd->data |= RXH_L4_B_0_1 | RXH_L4_B_2_3;
}

static int hinic3_convert_rss_type(struct hinic3_nic_dev *nic_dev,
				   struct nic_rss_type *rss_type,
				   struct ethtool_rxnfc *cmd)
{
	cmd->data = RXH_IP_SRC | RXH_IP_DST;
	switch (cmd->flow_type) {
	case TCP_V4_FLOW:
		convert_rss_type(rss_type->tcp_ipv4, cmd);
		break;
	case TCP_V6_FLOW:
		convert_rss_type(rss_type->tcp_ipv6, cmd);
		break;
	case UDP_V4_FLOW:
		convert_rss_type(rss_type->udp_ipv4, cmd);
		break;
	case UDP_V6_FLOW:
		convert_rss_type(rss_type->udp_ipv6, cmd);
		break;
	case IPV4_FLOW:
	case IPV6_FLOW:
		break;
	default:
		nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupported flow type\n");
		cmd->data = 0;
		return -EINVAL;
	}

	return 0;
}

static int hinic3_get_rss_hash_opts(struct hinic3_nic_dev *nic_dev, struct ethtool_rxnfc *cmd)
{
	struct nic_rss_type rss_type = {0};
	int err;

	cmd->data = 0;

	if (!test_bit(HINIC3_RSS_ENABLE, &nic_dev->flags))
		return 0;

	err = hinic3_get_rss_type(nic_dev->hwdev, &rss_type);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to get rss type\n");
		return err;
	}

	return hinic3_convert_rss_type(nic_dev, &rss_type, cmd);
}

int hinic3_get_rxnfc(struct net_device *netdev,
		     struct ethtool_rxnfc *cmd, u32 *rule_locs)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int err = 0;

	switch (cmd->cmd) {
	case ETHTOOL_GRXRINGS:
		cmd->data = nic_dev->q_params.num_qps;
		break;
	case ETHTOOL_GRXCLSRLCNT:
		cmd->rule_cnt = (u32)nic_dev->rx_flow_rule.tot_num_rules;
		break;
	case ETHTOOL_GRXCLSRULE:
		err = hinic3_ethtool_get_flow(nic_dev, cmd, cmd->fs.location);
		break;
	case ETHTOOL_GRXCLSRLALL:
		err = hinic3_ethtool_get_all_flows(nic_dev, cmd, rule_locs);
		break;
	case ETHTOOL_GRXFH:
		err = hinic3_get_rss_hash_opts(nic_dev, cmd);
		break;
	default:
		err = -EOPNOTSUPP;
		break;
	}

	return err;
}

int hinic3_set_rxnfc(struct net_device *netdev, struct ethtool_rxnfc *cmd)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int err = 0;

	switch (cmd->cmd) {
	case ETHTOOL_SRXFH:
		err = hinic3_set_rss_hash_opts(nic_dev, cmd);
		break;
	case ETHTOOL_SRXCLSRLINS:
		err = hinic3_ethtool_flow_replace(nic_dev, &cmd->fs);
		break;
	case ETHTOOL_SRXCLSRLDEL:
		err = hinic3_ethtool_flow_remove(nic_dev, cmd->fs.location);
		break;
	default:
		err = -EOPNOTSUPP;
		break;
	}

	return err;
}

static u16 hinic3_max_channels(struct hinic3_nic_dev *nic_dev)
{
	u8 tcs = (u8)netdev_get_num_tc(nic_dev->netdev);

	return tcs ? nic_dev->max_qps / tcs : nic_dev->max_qps;
}

static u16 hinic3_curr_channels(struct hinic3_nic_dev *nic_dev)
{
	if (netif_running(nic_dev->netdev))
		return nic_dev->q_params.num_qps ?
				nic_dev->q_params.num_qps : 1;
	else
		return (u16)min_t(u16, hinic3_max_channels(nic_dev),
				  nic_dev->q_params.num_qps);
}

void hinic3_get_channels(struct net_device *netdev,
			 struct ethtool_channels *channels)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);

	channels->max_rx = 0;
	channels->max_tx = 0;
	channels->max_other = 0;
	/* report maximum channels */
	channels->max_combined = hinic3_max_channels(nic_dev);
	channels->rx_count = 0;
	channels->tx_count = 0;
	channels->other_count = 0;
	/* report flow director queues as maximum channels */
	channels->combined_count = hinic3_curr_channels(nic_dev);
}

static int hinic3_validate_channel_parameter(struct net_device *netdev,
					     const struct ethtool_channels *channels)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	u16 max_channel = hinic3_max_channels(nic_dev);
	unsigned int count = channels->combined_count;

	if (!count) {
		nicif_err(nic_dev, drv, netdev,
			  "Unsupported combined_count=0\n");
		return -EINVAL;
	}

	if (channels->tx_count || channels->rx_count || channels->other_count) {
		nicif_err(nic_dev, drv, netdev,
			  "Setting rx/tx/other count not supported\n");
		return -EINVAL;
	}

	if (count > max_channel) {
		nicif_err(nic_dev, drv, netdev,
			  "Combined count %u exceed limit %u\n", count,
			  max_channel);
		return -EINVAL;
	}

	return 0;
}

static void change_num_channel_reopen_handler(struct hinic3_nic_dev *nic_dev,
					      const void *priv_data)
{
	hinic3_set_default_rss_indir(nic_dev->netdev);
}

int hinic3_set_channels(struct net_device *netdev,
			struct ethtool_channels *channels)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	struct hinic3_dyna_txrxq_params q_params = {0};
	unsigned int count = channels->combined_count;
	int err;
	u8 user_cos_num = hinic3_get_dev_user_cos_num(nic_dev);

	if (hinic3_validate_channel_parameter(netdev, channels))
		return -EINVAL;

	if (!test_bit(HINIC3_RSS_ENABLE, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, netdev,
			  "This function don't support RSS, only support 1 queue pair\n");
		return -EOPNOTSUPP;
	}

	if (test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags)) {
		if (count < user_cos_num) {
			nicif_err(nic_dev, drv, netdev,
				  "DCB is on, channels num should more than valid cos num:%u\n",
				  user_cos_num);

			return -EOPNOTSUPP;
		}
	}

	if (HINIC3_SUPPORT_FDIR(nic_dev->hwdev) &&
	    !hinic3_validate_channel_setting_in_ntuple(nic_dev, count))
		return -EOPNOTSUPP;

	nicif_info(nic_dev, drv, netdev, "Set max combined queue number from %u to %u\n",
		   nic_dev->q_params.num_qps, count);

	if (netif_running(netdev)) {
		q_params = nic_dev->q_params;
		q_params.num_qps = (u16)count;
		q_params.txqs_res = NULL;
		q_params.rxqs_res = NULL;
		q_params.irq_cfg = NULL;

		nicif_info(nic_dev, drv, netdev, "Restarting channel\n");
		err = hinic3_change_channel_settings(nic_dev, &q_params,
						     change_num_channel_reopen_handler, NULL);
		if (err) {
			nicif_err(nic_dev, drv, netdev, "Failed to change channel settings\n");
			return -EFAULT;
		}
	} else {
		/* Discard user configured rss */
		hinic3_set_default_rss_indir(netdev);
		nic_dev->q_params.num_qps = (u16)count;
	}

	nic_dev->nic_vram->vram_num_qps = nic_dev->q_params.num_qps;
	return 0;
}

#ifndef NOT_HAVE_GET_RXFH_INDIR_SIZE
u32 hinic3_get_rxfh_indir_size(struct net_device *netdev)
{
	return NIC_RSS_INDIR_SIZE;
}
#endif

static int set_rss_rxfh(struct net_device *netdev, const u32 *indir,
			const u8 *key)
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int err;

	if (indir) {
		err = hinic3_rss_set_indir_tbl(nic_dev->hwdev, indir);
		if (err) {
			nicif_err(nic_dev, drv, netdev,
				  "Failed to set rss indir table\n");
			return -EFAULT;
		}
		clear_bit(HINIC3_RSS_DEFAULT_INDIR, &nic_dev->flags);

		memcpy(nic_dev->rss_indir, indir,
		       sizeof(u32) * NIC_RSS_INDIR_SIZE);
		nicif_info(nic_dev, drv, netdev, "Change rss indir success\n");
	}

	if (key) {
		err = hinic3_rss_set_hash_key(nic_dev->hwdev, key);
		if (err) {
			nicif_err(nic_dev, drv, netdev, "Failed to set rss key\n");
			return -EFAULT;
		}

		copy_value_to_rss_hkey(nic_dev, key);
		nicif_info(nic_dev, drv, netdev, "Change rss key success\n");
	}

	return 0;
}

#if defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)
u32 hinic3_get_rxfh_key_size(struct net_device *netdev)
{
	return NIC_RSS_KEY_SIZE;
}

#ifdef HAVE_RXFH_HASHFUNC
int hinic3_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key, u8 *hfunc)
#else
int hinic3_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key)
#endif
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int err = 0;

	if (!test_bit(HINIC3_RSS_ENABLE, &nic_dev->flags)) {
		netdev_warn_once(nic_dev->netdev, "Rss is disable\n");
		return -EOPNOTSUPP;
	}

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc)
		*hfunc = nic_dev->rss_hash_engine ?
			ETH_RSS_HASH_TOP : ETH_RSS_HASH_XOR;
#endif

	if (indir) {
		err = hinic3_rss_get_indir_tbl(nic_dev->hwdev, indir);
		if (err)
			return -EFAULT;
	}

	if (key)
		memcpy(key, nic_dev->rss_hkey, NIC_RSS_KEY_SIZE);

	return err;
}

#ifdef HAVE_RXFH_HASHFUNC
int hinic3_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key,
		    const u8 hfunc)
#else
#ifdef HAVE_RXFH_NONCONST
int hinic3_set_rxfh(struct net_device *netdev, u32 *indir, u8 *key)
#else
int hinic3_set_rxfh(struct net_device *netdev, const u32 *indir, const u8 *key)
#endif
#endif /* HAVE_RXFH_HASHFUNC */
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int err = 0;

	if (!test_bit(HINIC3_RSS_ENABLE, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Not support to set rss parameters when rss is disable\n");
		return -EOPNOTSUPP;
	}

	if (test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags) && indir) {
		nicif_err(nic_dev, drv, netdev,
			  "Not support to set indir when DCB is enabled\n");
		return -EOPNOTSUPP;
	}

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc != ETH_RSS_HASH_NO_CHANGE) {
		if (hfunc != ETH_RSS_HASH_TOP && hfunc != ETH_RSS_HASH_XOR) {
			nicif_err(nic_dev, drv, netdev,
				  "Not support to set hfunc type except TOP and XOR\n");
			return -EOPNOTSUPP;
		}

		nic_dev->rss_hash_engine = (hfunc == ETH_RSS_HASH_XOR) ?
			HINIC3_RSS_HASH_ENGINE_TYPE_XOR :
			HINIC3_RSS_HASH_ENGINE_TYPE_TOEP;
		err = hinic3_rss_set_hash_engine(nic_dev->hwdev,
						 nic_dev->rss_hash_engine);
		if (err)
			return -EFAULT;

		nicif_info(nic_dev, drv, netdev,
			   "Change hfunc to RSS_HASH_%s success\n",
			   (hfunc == ETH_RSS_HASH_XOR) ? "XOR" : "TOP");
	}
#endif
	err = set_rss_rxfh(netdev, indir, key);

	return err;
}

#else /* !(defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH)) */

#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
int hinic3_get_rxfh_indir(struct net_device *netdev,
			  struct ethtool_rxfh_indir *indir1)
#else
int hinic3_get_rxfh_indir(struct net_device *netdev, u32 *indir)
#endif
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
	int err = 0;
#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
	u32 *indir = NULL;

	/* In a low version kernel(eg:suse 11.2), call the interface twice.
	 * First call to get the size value,
	 * and second call to get the rxfh indir according to the size value.
	 */
	if (indir1->size == 0) {
		indir1->size = NIC_RSS_INDIR_SIZE;
		return 0;
	}

	if (indir1->size < NIC_RSS_INDIR_SIZE) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to get rss indir, rss size(%d) is more than system rss size(%u).\n",
			  NIC_RSS_INDIR_SIZE, indir1->size);
		return -EINVAL;
	}

	indir = indir1->ring_index;
#endif
	if (!test_bit(HINIC3_RSS_ENABLE, &nic_dev->flags)) {
		netdev_warn_once(nic_dev->netdev, "Rss is disable\n");
		return -EOPNOTSUPP;
	}

	if (indir)
		err = hinic3_rss_get_indir_tbl(nic_dev->hwdev, indir);

	return err;
}

#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
int hinic3_set_rxfh_indir(struct net_device *netdev,
			  const struct ethtool_rxfh_indir *indir1)
#else
int hinic3_set_rxfh_indir(struct net_device *netdev, const u32 *indir)
#endif
{
	struct hinic3_nic_dev *nic_dev = netdev_priv(netdev);
#ifdef NOT_HAVE_GET_RXFH_INDIR_SIZE
	const u32 *indir = NULL;

	if (indir1->size != NIC_RSS_INDIR_SIZE) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Failed to set rss indir, rss size(%d) is more than system rss size(%u).\n",
			  NIC_RSS_INDIR_SIZE, indir1->size);
		return -EINVAL;
	}

	indir = indir1->ring_index;
#endif

	if (!test_bit(HINIC3_RSS_ENABLE, &nic_dev->flags)) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "Not support to set rss indir when rss is disable\n");
		return -EOPNOTSUPP;
	}

	if (test_bit(HINIC3_DCB_ENABLE, &nic_dev->flags) && indir) {
		nicif_err(nic_dev, drv, netdev,
			  "Not support to set indir when DCB is enabled\n");
		return -EOPNOTSUPP;
	}

	return set_rss_rxfh(netdev, indir, NULL);
}

#endif /* defined(ETHTOOL_GRSSH) && defined(ETHTOOL_SRSSH) */


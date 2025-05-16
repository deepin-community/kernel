// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/device.h>
#include <linux/ethtool.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/errno.h>

#include "ossl_knl.h"
#include "hinic3_crm.h"
#include "hinic3_nic_cfg.h"
#include "hinic3_nic_dev.h"

#define MAX_NUM_OF_ETHTOOL_NTUPLE_RULES BIT(9)
struct hinic3_ethtool_rx_flow_rule {
	struct list_head            list;
	struct ethtool_rx_flow_spec flow_spec;
};

static void tcam_translate_key_y(u8 *key_y, const u8 *src_input, const u8 *mask, u8 len)
{
	u8 idx;

	for (idx = 0; idx < len; idx++)
		key_y[idx] = src_input[idx] & mask[idx];
}

static void tcam_translate_key_x(u8 *key_x, const u8 *key_y, const u8 *mask, u8 len)
{
	u8 idx;

	for (idx = 0; idx < len; idx++)
		key_x[idx] = key_y[idx] ^ mask[idx];
}

static void tcam_key_calculate(struct tag_tcam_key *tcam_key,
			       struct nic_tcam_cfg_rule *fdir_tcam_rule)
{
	tcam_translate_key_y(fdir_tcam_rule->key.y,
			     (u8 *)(&tcam_key->key_info),
			     (u8 *)(&tcam_key->key_mask), TCAM_FLOW_KEY_SIZE);
	tcam_translate_key_x(fdir_tcam_rule->key.x, fdir_tcam_rule->key.y,
			     (u8 *)(&tcam_key->key_mask), TCAM_FLOW_KEY_SIZE);
}

#define TCAM_IPV4_TYPE 0
#define TCAM_IPV6_TYPE 1

static int hinic3_base_ipv4_parse(struct hinic3_nic_dev *nic_dev,
				  struct ethtool_rx_flow_spec *fs,
				  struct tag_tcam_key *tcam_key)
{
	struct ethtool_tcpip4_spec *mask = &fs->m_u.tcp_ip4_spec;
	struct ethtool_tcpip4_spec *val  = &fs->h_u.tcp_ip4_spec;
	u32 temp;

	switch (mask->ip4src) {
	case U32_MAX:
		temp = ntohl(val->ip4src);
		tcam_key->key_info.sipv4_h = high_16_bits(temp);
		tcam_key->key_info.sipv4_l = low_16_bits(temp);

		tcam_key->key_mask.sipv4_h = U16_MAX;
		tcam_key->key_mask.sipv4_l = U16_MAX;
		break;
	case 0:
		break;

	default:
		nicif_err(nic_dev, drv, nic_dev->netdev, "invalid src_ip mask\n");
		return -EINVAL;
	}

	switch (mask->ip4dst) {
	case U32_MAX:
		temp = ntohl(val->ip4dst);
		tcam_key->key_info.dipv4_h = high_16_bits(temp);
		tcam_key->key_info.dipv4_l = low_16_bits(temp);

		tcam_key->key_mask.dipv4_h = U16_MAX;
		tcam_key->key_mask.dipv4_l = U16_MAX;
		break;
	case 0:
		break;

	default:
		nicif_err(nic_dev, drv, nic_dev->netdev, "invalid src_ip mask\n");
		return -EINVAL;
	}

	tcam_key->key_info.ip_type = TCAM_IPV4_TYPE;
	tcam_key->key_mask.ip_type = TCAM_IP_TYPE_MASK;

	tcam_key->key_info.function_id = hinic3_global_func_id(nic_dev->hwdev);
	tcam_key->key_mask.function_id = TCAM_FUNC_ID_MASK;

	return 0;
}

static int hinic3_fdir_tcam_ipv4_l4_init(struct hinic3_nic_dev *nic_dev,
					 struct ethtool_rx_flow_spec *fs,
					 struct tag_tcam_key *tcam_key)
{
	struct ethtool_tcpip4_spec *l4_mask = &fs->m_u.tcp_ip4_spec;
	struct ethtool_tcpip4_spec *l4_val  = &fs->h_u.tcp_ip4_spec;
	int err;

	err = hinic3_base_ipv4_parse(nic_dev, fs, tcam_key);
	if (err)
		return err;

	tcam_key->key_info.dport = ntohs(l4_val->pdst);
	tcam_key->key_mask.dport = l4_mask->pdst;

	tcam_key->key_info.sport = ntohs(l4_val->psrc);
	tcam_key->key_mask.sport = l4_mask->psrc;

	if (fs->flow_type == TCP_V4_FLOW)
		tcam_key->key_info.ip_proto = IPPROTO_TCP;
	else
		tcam_key->key_info.ip_proto = IPPROTO_UDP;
	tcam_key->key_mask.ip_proto = U8_MAX;

	return 0;
}

static int hinic3_fdir_tcam_ipv4_init(struct hinic3_nic_dev *nic_dev,
				      struct ethtool_rx_flow_spec *fs,
				      struct tag_tcam_key *tcam_key)
{
	struct ethtool_usrip4_spec *l3_mask = &fs->m_u.usr_ip4_spec;
	struct ethtool_usrip4_spec *l3_val  = &fs->h_u.usr_ip4_spec;
	int err;

	err = hinic3_base_ipv4_parse(nic_dev, fs, tcam_key);
	if (err)
		return err;

	tcam_key->key_info.ip_proto = l3_val->proto;
	tcam_key->key_mask.ip_proto = l3_mask->proto;

	return 0;
}

#ifndef UNSUPPORT_NTUPLE_IPV6
enum ipv6_parse_res {
	IPV6_MASK_INVALID,
	IPV6_MASK_ALL_MASK,
	IPV6_MASK_ALL_ZERO,
};

enum ipv6_index {
	IPV6_IDX0,
	IPV6_IDX1,
	IPV6_IDX2,
	IPV6_IDX3,
};

static int ipv6_mask_parse(const u32 *ipv6_mask)
{
	if (ipv6_mask[IPV6_IDX0] == 0 && ipv6_mask[IPV6_IDX1] == 0 &&
	    ipv6_mask[IPV6_IDX2] == 0 && ipv6_mask[IPV6_IDX3] == 0)
		return IPV6_MASK_ALL_ZERO;

	if (ipv6_mask[IPV6_IDX0] == U32_MAX &&
	    ipv6_mask[IPV6_IDX1] == U32_MAX &&
	    ipv6_mask[IPV6_IDX2] == U32_MAX && ipv6_mask[IPV6_IDX3] == U32_MAX)
		return IPV6_MASK_ALL_MASK;

	return IPV6_MASK_INVALID;
}

static int hinic3_base_ipv6_parse(struct hinic3_nic_dev *nic_dev,
				  struct ethtool_rx_flow_spec *fs,
				  struct tag_tcam_key *tcam_key)
{
	struct ethtool_tcpip6_spec *mask = &fs->m_u.tcp_ip6_spec;
	struct ethtool_tcpip6_spec *val  = &fs->h_u.tcp_ip6_spec;
	int parse_res;
	u32 temp;

	parse_res = ipv6_mask_parse((u32 *)mask->ip6src);
	if (parse_res == IPV6_MASK_ALL_MASK) {
		temp = ntohl(val->ip6src[IPV6_IDX0]);
		tcam_key->key_info_ipv6.sipv6_key0 = high_16_bits(temp);
		tcam_key->key_info_ipv6.sipv6_key1 = low_16_bits(temp);
		temp = ntohl(val->ip6src[IPV6_IDX1]);
		tcam_key->key_info_ipv6.sipv6_key2 = high_16_bits(temp);
		tcam_key->key_info_ipv6.sipv6_key3 = low_16_bits(temp);
		temp = ntohl(val->ip6src[IPV6_IDX2]);
		tcam_key->key_info_ipv6.sipv6_key4 = high_16_bits(temp);
		tcam_key->key_info_ipv6.sipv6_key5 = low_16_bits(temp);
		temp = ntohl(val->ip6src[IPV6_IDX3]);
		tcam_key->key_info_ipv6.sipv6_key6 = high_16_bits(temp);
		tcam_key->key_info_ipv6.sipv6_key7 = low_16_bits(temp);

		tcam_key->key_mask_ipv6.sipv6_key0 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key1 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key2 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key3 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key4 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key5 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key6 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key7 = U16_MAX;
	} else if (parse_res == IPV6_MASK_INVALID) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "invalid src_ipv6 mask\n");
		return -EINVAL;
	}

	parse_res = ipv6_mask_parse((u32 *)mask->ip6dst);
	if (parse_res == IPV6_MASK_ALL_MASK) {
		temp = ntohl(val->ip6dst[IPV6_IDX0]);
		tcam_key->key_info_ipv6.dipv6_key0 = high_16_bits(temp);
		tcam_key->key_info_ipv6.dipv6_key1 = low_16_bits(temp);
		temp = ntohl(val->ip6dst[IPV6_IDX1]);
		tcam_key->key_info_ipv6.dipv6_key2 = high_16_bits(temp);
		tcam_key->key_info_ipv6.dipv6_key3 = low_16_bits(temp);
		temp = ntohl(val->ip6dst[IPV6_IDX2]);
		tcam_key->key_info_ipv6.dipv6_key4 = high_16_bits(temp);
		tcam_key->key_info_ipv6.dipv6_key5 = low_16_bits(temp);
		temp = ntohl(val->ip6dst[IPV6_IDX3]);
		tcam_key->key_info_ipv6.dipv6_key6 = high_16_bits(temp);
		tcam_key->key_info_ipv6.dipv6_key7 = low_16_bits(temp);

		tcam_key->key_mask_ipv6.dipv6_key0 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key1 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key2 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key3 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key4 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key5 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key6 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key7 = U16_MAX;
	} else if (parse_res == IPV6_MASK_INVALID) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "invalid dst_ipv6 mask\n");
		return -EINVAL;
	}

	tcam_key->key_info_ipv6.ip_type = TCAM_IPV6_TYPE;
	tcam_key->key_mask_ipv6.ip_type = TCAM_IP_TYPE_MASK;

	tcam_key->key_info_ipv6.function_id =
		hinic3_global_func_id(nic_dev->hwdev);
	tcam_key->key_mask_ipv6.function_id = TCAM_FUNC_ID_MASK;

	return 0;
}

static int hinic3_fdir_tcam_ipv6_l4_init(struct hinic3_nic_dev *nic_dev,
					 struct ethtool_rx_flow_spec *fs,
					 struct tag_tcam_key *tcam_key)
{
	struct ethtool_tcpip6_spec *l4_mask = &fs->m_u.tcp_ip6_spec;
	struct ethtool_tcpip6_spec *l4_val = &fs->h_u.tcp_ip6_spec;
	int err;

	err = hinic3_base_ipv6_parse(nic_dev, fs, tcam_key);
	if (err)
		return err;

	tcam_key->key_info_ipv6.dport = ntohs(l4_val->pdst);
	tcam_key->key_mask_ipv6.dport = l4_mask->pdst;

	tcam_key->key_info_ipv6.sport = ntohs(l4_val->psrc);
	tcam_key->key_mask_ipv6.sport = l4_mask->psrc;

	if (fs->flow_type == TCP_V6_FLOW)
		tcam_key->key_info_ipv6.ip_proto = NEXTHDR_TCP;
	else
		tcam_key->key_info_ipv6.ip_proto = NEXTHDR_UDP;
	tcam_key->key_mask_ipv6.ip_proto = U8_MAX;

	return 0;
}

static int hinic3_fdir_tcam_ipv6_init(struct hinic3_nic_dev *nic_dev,
				      struct ethtool_rx_flow_spec *fs,
				      struct tag_tcam_key *tcam_key)
{
	struct ethtool_usrip6_spec *l3_mask = &fs->m_u.usr_ip6_spec;
	struct ethtool_usrip6_spec *l3_val  = &fs->h_u.usr_ip6_spec;
	int err;

	err = hinic3_base_ipv6_parse(nic_dev, fs, tcam_key);
	if (err)
		return err;

	tcam_key->key_info_ipv6.ip_proto = l3_val->l4_proto;
	tcam_key->key_mask_ipv6.ip_proto = l3_mask->l4_proto;

	return 0;
}
#endif

static int hinic3_fdir_tcam_info_init(struct hinic3_nic_dev *nic_dev,
				      struct ethtool_rx_flow_spec *fs,
				      struct tag_tcam_key *tcam_key,
				      struct nic_tcam_cfg_rule *fdir_tcam_rule)
{
	int err;

	switch (fs->flow_type) {
	case TCP_V4_FLOW:
	case UDP_V4_FLOW:
		err = hinic3_fdir_tcam_ipv4_l4_init(nic_dev, fs, tcam_key);
		if (err)
			return err;
		break;
	case IP_USER_FLOW:
		err = hinic3_fdir_tcam_ipv4_init(nic_dev, fs, tcam_key);
		if (err)
			return err;
		break;
#ifndef UNSUPPORT_NTUPLE_IPV6
	case TCP_V6_FLOW:
	case UDP_V6_FLOW:
		err = hinic3_fdir_tcam_ipv6_l4_init(nic_dev, fs, tcam_key);
		if (err)
			return err;
		break;
	case IPV6_USER_FLOW:
		err = hinic3_fdir_tcam_ipv6_init(nic_dev, fs, tcam_key);
		if (err)
			return err;
		break;
#endif
	default:
		return -EOPNOTSUPP;
	}

	tcam_key->key_info.tunnel_type = 0;
	tcam_key->key_mask.tunnel_type = TCAM_TUNNEL_TYPE_MASK;

	fdir_tcam_rule->data.qid = (u32)fs->ring_cookie;
	tcam_key_calculate(tcam_key, fdir_tcam_rule);

	return 0;
}

void hinic3_flush_rx_flow_rule(struct hinic3_nic_dev *nic_dev)
{
	struct hinic3_tcam_info *tcam_info = &nic_dev->tcam;
	struct hinic3_ethtool_rx_flow_rule *eth_rule = NULL;
	struct hinic3_ethtool_rx_flow_rule *eth_rule_tmp = NULL;
	struct hinic3_tcam_filter *tcam_iter = NULL;
	struct hinic3_tcam_filter *tcam_iter_tmp = NULL;
	struct hinic3_tcam_dynamic_block *block = NULL;
	struct hinic3_tcam_dynamic_block *block_tmp = NULL;
	struct list_head *dynamic_list =
		&tcam_info->tcam_dynamic_info.tcam_dynamic_list;

	if (!list_empty(&tcam_info->tcam_list)) {
		list_for_each_entry_safe(tcam_iter, tcam_iter_tmp,
					 &tcam_info->tcam_list,
					 tcam_filter_list) {
			list_del(&tcam_iter->tcam_filter_list);
			kfree(tcam_iter);
		}
	}
	if (!list_empty(dynamic_list)) {
		list_for_each_entry_safe(block, block_tmp, dynamic_list,
					 block_list) {
			list_del(&block->block_list);
			kfree(block);
		}
	}

	if (!list_empty(&nic_dev->rx_flow_rule.rules)) {
		list_for_each_entry_safe(eth_rule, eth_rule_tmp,
					 &nic_dev->rx_flow_rule.rules, list) {
			list_del(&eth_rule->list);
			kfree(eth_rule);
		}
	}

	if (HINIC3_SUPPORT_FDIR(nic_dev->hwdev)) {
		hinic3_flush_tcam_rule(nic_dev->hwdev);
		hinic3_set_fdir_tcam_rule_filter(nic_dev->hwdev, false);
	}
}

static struct hinic3_tcam_dynamic_block *
hinic3_alloc_dynamic_block_resource(struct hinic3_nic_dev *nic_dev,
				    struct hinic3_tcam_info *tcam_info,
				    u16 dynamic_block_id)
{
	struct hinic3_tcam_dynamic_block *dynamic_block_ptr = NULL;

	dynamic_block_ptr = kzalloc(sizeof(*dynamic_block_ptr), GFP_KERNEL);
	if (!dynamic_block_ptr)
		return NULL;

	dynamic_block_ptr->dynamic_block_id = dynamic_block_id;
	list_add_tail(&dynamic_block_ptr->block_list,
		      &tcam_info->tcam_dynamic_info.tcam_dynamic_list);

	tcam_info->tcam_dynamic_info.dynamic_block_cnt++;

	return dynamic_block_ptr;
}

static void hinic3_free_dynamic_block_resource(struct hinic3_tcam_info *tcam_info,
					       struct hinic3_tcam_dynamic_block *block_ptr)
{
	if (!block_ptr)
		return;

	list_del(&block_ptr->block_list);
	kfree(block_ptr);

	tcam_info->tcam_dynamic_info.dynamic_block_cnt--;
}

static struct hinic3_tcam_dynamic_block *
hinic3_dynamic_lookup_tcam_filter(struct hinic3_nic_dev *nic_dev,
				  struct nic_tcam_cfg_rule *fdir_tcam_rule,
				  const struct hinic3_tcam_info *tcam_info,
				  struct hinic3_tcam_filter *tcam_filter,
				  u16 *tcam_index)
{
	struct hinic3_tcam_dynamic_block *tmp = NULL;
	u16 index;

	list_for_each_entry(tmp,
			    &tcam_info->tcam_dynamic_info.tcam_dynamic_list,
			     block_list)
		if (tmp->dynamic_index_cnt < HINIC3_TCAM_DYNAMIC_BLOCK_SIZE ||
		    !tmp)
			break;

	if (!tmp || tmp->dynamic_index_cnt >= HINIC3_TCAM_DYNAMIC_BLOCK_SIZE) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fdir filter dynamic lookup for index failed\n");
		return NULL;
	}

	for (index = 0; index < HINIC3_TCAM_DYNAMIC_BLOCK_SIZE; index++)
		if (tmp->dynamic_index_used[index] == 0)
			break;

	if (index == HINIC3_TCAM_DYNAMIC_BLOCK_SIZE) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "tcam block 0x%x supports filter rules is full\n",
			  tmp->dynamic_block_id);
		return NULL;
	}

	tcam_filter->dynamic_block_id = tmp->dynamic_block_id;
	tcam_filter->index = index;
	*tcam_index = index;

	fdir_tcam_rule->index = index +
		HINIC3_PKT_TCAM_DYNAMIC_INDEX_START(tmp->dynamic_block_id);

	return tmp;
}

static int hinic3_add_tcam_filter(struct hinic3_nic_dev *nic_dev,
				  struct hinic3_tcam_filter *tcam_filter,
				  struct nic_tcam_cfg_rule *fdir_tcam_rule)
{
	struct hinic3_tcam_info *tcam_info = &nic_dev->tcam;
	struct hinic3_tcam_dynamic_block *dynamic_block_ptr = NULL;
	struct hinic3_tcam_dynamic_block *tmp = NULL;
	u16 block_cnt = tcam_info->tcam_dynamic_info.dynamic_block_cnt;
	u16 tcam_block_index = 0;
	int block_alloc_flag = 0;
	u16 index = 0;
	int err;

	if (tcam_info->tcam_rule_nums >=
	    block_cnt * HINIC3_TCAM_DYNAMIC_BLOCK_SIZE) {
		if (block_cnt >= (HINIC3_MAX_TCAM_FILTERS /
				  HINIC3_TCAM_DYNAMIC_BLOCK_SIZE)) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Dynamic tcam block is full, alloc failed\n");
			goto failed;
		}

		err = hinic3_alloc_tcam_block(nic_dev->hwdev,
					      &tcam_block_index);
		if (err) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Fdir filter dynamic tcam alloc block failed\n");
			goto failed;
		}

		block_alloc_flag = 1;

		dynamic_block_ptr =
			hinic3_alloc_dynamic_block_resource(nic_dev, tcam_info,
							    tcam_block_index);
		if (!dynamic_block_ptr) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Fdir filter dynamic alloc block memory failed\n");
			goto block_alloc_failed;
		}
	}

	tmp = hinic3_dynamic_lookup_tcam_filter(nic_dev,
						fdir_tcam_rule, tcam_info,
						tcam_filter, &index);
	if (!tmp) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Dynamic lookup tcam filter failed\n");
		goto lookup_tcam_index_failed;
	}

	err = hinic3_add_tcam_rule(nic_dev->hwdev, fdir_tcam_rule);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fdir_tcam_rule add failed\n");
		goto add_tcam_rules_failed;
	}

	nicif_info(nic_dev, drv, nic_dev->netdev,
		   "Add fdir tcam rule, function_id: 0x%x, tcam_block_id: %u, local_index: %u, global_index: %u, queue: %u, tcam_rule_nums: %u succeed\n",
		   hinic3_global_func_id(nic_dev->hwdev),
		   tcam_filter->dynamic_block_id, index, fdir_tcam_rule->index,
		   fdir_tcam_rule->data.qid, tcam_info->tcam_rule_nums + 1);

	if (tcam_info->tcam_rule_nums == 0) {
		err = hinic3_set_fdir_tcam_rule_filter(nic_dev->hwdev, true);
		if (err)
			goto enable_failed;
	}

	list_add_tail(&tcam_filter->tcam_filter_list, &tcam_info->tcam_list);

	tmp->dynamic_index_used[index] = 1;
	tmp->dynamic_index_cnt++;

	tcam_info->tcam_rule_nums++;

	return 0;

enable_failed:
	hinic3_del_tcam_rule(nic_dev->hwdev, fdir_tcam_rule->index);

add_tcam_rules_failed:
lookup_tcam_index_failed:
	if (block_alloc_flag == 1)
		hinic3_free_dynamic_block_resource(tcam_info,
						   dynamic_block_ptr);

block_alloc_failed:
	if (block_alloc_flag == 1)
		hinic3_free_tcam_block(nic_dev->hwdev, &tcam_block_index);

failed:
	return -EFAULT;
}

static int hinic3_del_tcam_filter(struct hinic3_nic_dev *nic_dev,
				  struct hinic3_tcam_filter *tcam_filter)
{
	struct hinic3_tcam_info *tcam_info = &nic_dev->tcam;
	u16 dynamic_block_id = tcam_filter->dynamic_block_id;
	struct hinic3_tcam_dynamic_block *tmp = NULL;
	u32 index = 0;
	int err;

	list_for_each_entry(tmp,
			    &tcam_info->tcam_dynamic_info.tcam_dynamic_list,
			    block_list) {
		if (tmp->dynamic_block_id == dynamic_block_id)
			break;
	}
	if (!tmp || tmp->dynamic_block_id != dynamic_block_id) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fdir filter del dynamic lookup for block failed\n");
		return -EFAULT;
	}

	index = HINIC3_PKT_TCAM_DYNAMIC_INDEX_START(tmp->dynamic_block_id) +
			tcam_filter->index;

	err = hinic3_del_tcam_rule(nic_dev->hwdev, index);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "fdir_tcam_rule del failed\n");
		return -EFAULT;
	}

	nicif_info(nic_dev, drv, nic_dev->netdev,
		   "Del fdir_tcam_dynamic_rule function_id: 0x%x, tcam_block_id: %u, local_index: %u, global_index: %u, local_rules_nums: %u, global_rule_nums: %u succeed\n",
		   hinic3_global_func_id(nic_dev->hwdev), dynamic_block_id,
		   tcam_filter->index, index, tmp->dynamic_index_cnt - 1,
		   tcam_info->tcam_rule_nums - 1);

	tmp->dynamic_index_used[tcam_filter->index] = 0;
	tmp->dynamic_index_cnt--;
	tcam_info->tcam_rule_nums--;
	if (tmp->dynamic_index_cnt == 0) {
		hinic3_free_tcam_block(nic_dev->hwdev, &dynamic_block_id);
		hinic3_free_dynamic_block_resource(tcam_info, tmp);
	}

	if (tcam_info->tcam_rule_nums == 0)
		hinic3_set_fdir_tcam_rule_filter(nic_dev->hwdev, false);

	list_del(&tcam_filter->tcam_filter_list);
	kfree(tcam_filter);

	return 0;
}

static inline struct hinic3_tcam_filter *
hinic3_tcam_filter_lookup(const struct list_head *filter_list,
			  struct tag_tcam_key *key)
{
	struct hinic3_tcam_filter *iter = NULL;

	list_for_each_entry(iter, filter_list, tcam_filter_list) {
		if (memcmp(key, &iter->tcam_key,
			   sizeof(struct tag_tcam_key)) == 0) {
			return iter;
		}
	}

	return NULL;
}

static void del_ethtool_rule(struct hinic3_nic_dev *nic_dev,
			     struct hinic3_ethtool_rx_flow_rule *eth_rule)
{
	list_del(&eth_rule->list);
	nic_dev->rx_flow_rule.tot_num_rules--;

	kfree(eth_rule);
}

static int hinic3_remove_one_rule(struct hinic3_nic_dev *nic_dev,
				  struct hinic3_ethtool_rx_flow_rule *eth_rule)
{
	struct hinic3_tcam_info *tcam_info = &nic_dev->tcam;
	struct hinic3_tcam_filter *tcam_filter = NULL;
	struct nic_tcam_cfg_rule fdir_tcam_rule;
	struct tag_tcam_key tcam_key;
	int err;

	memset(&fdir_tcam_rule, 0, sizeof(fdir_tcam_rule));
	memset(&tcam_key, 0, sizeof(tcam_key));

	err = hinic3_fdir_tcam_info_init(nic_dev, &eth_rule->flow_spec,
					 &tcam_key, &fdir_tcam_rule);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Init fdir info failed\n");
		return err;
	}

	tcam_filter = hinic3_tcam_filter_lookup(&tcam_info->tcam_list,
						&tcam_key);
	if (!tcam_filter) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Filter does not exists\n");
		return -EEXIST;
	}

	err = hinic3_del_tcam_filter(nic_dev, tcam_filter);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Delete tcam filter failed\n");
		return err;
	}

	del_ethtool_rule(nic_dev, eth_rule);

	return 0;
}

static void add_rule_to_list(struct hinic3_nic_dev *nic_dev,
			     struct hinic3_ethtool_rx_flow_rule *rule)
{
	struct hinic3_ethtool_rx_flow_rule *iter = NULL;
	struct list_head *head = &nic_dev->rx_flow_rule.rules;

	list_for_each_entry(iter, &nic_dev->rx_flow_rule.rules, list) {
		if (iter->flow_spec.location > rule->flow_spec.location)
			break;
		head = &iter->list;
	}
	nic_dev->rx_flow_rule.tot_num_rules++;
	list_add(&rule->list, head);
}

static int hinic3_add_one_rule(struct hinic3_nic_dev *nic_dev,
			       struct ethtool_rx_flow_spec *fs)
{
	struct nic_tcam_cfg_rule fdir_tcam_rule;
	struct tag_tcam_key tcam_key;
	struct hinic3_ethtool_rx_flow_rule *eth_rule = NULL;
	struct hinic3_tcam_filter *tcam_filter = NULL;
	struct hinic3_tcam_info *tcam_info = &nic_dev->tcam;
	int err;

	memset(&fdir_tcam_rule, 0, sizeof(fdir_tcam_rule));
	memset(&tcam_key, 0, sizeof(tcam_key));
	err = hinic3_fdir_tcam_info_init(nic_dev, fs, &tcam_key,
					 &fdir_tcam_rule);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Init fdir info failed\n");
		return err;
	}

	tcam_filter = hinic3_tcam_filter_lookup(&tcam_info->tcam_list,
						&tcam_key);
	if (tcam_filter) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Filter exists\n");
		return -EEXIST;
	}

	tcam_filter = kzalloc(sizeof(*tcam_filter), GFP_KERNEL);
	if (!tcam_filter)
		return -ENOMEM;
	memcpy(&tcam_filter->tcam_key,
	       &tcam_key, sizeof(struct tag_tcam_key));
	tcam_filter->queue = (u16)fdir_tcam_rule.data.qid;

	err = hinic3_add_tcam_filter(nic_dev, tcam_filter, &fdir_tcam_rule);
	if (err)
		goto add_tcam_filter_fail;

	/* driver save new rule filter */
	eth_rule = kzalloc(sizeof(*eth_rule), GFP_KERNEL);
	if (!eth_rule) {
		err = -ENOMEM;
		goto alloc_eth_rule_fail;
	}

	eth_rule->flow_spec = *fs;
	add_rule_to_list(nic_dev, eth_rule);

	return 0;

alloc_eth_rule_fail:
	hinic3_del_tcam_filter(nic_dev, tcam_filter);
add_tcam_filter_fail:
	kfree(tcam_filter);
	return err;
}

static struct hinic3_ethtool_rx_flow_rule *
find_ethtool_rule(const struct hinic3_nic_dev *nic_dev, u32 location)
{
	struct hinic3_ethtool_rx_flow_rule *iter = NULL;

	list_for_each_entry(iter, &nic_dev->rx_flow_rule.rules, list) {
		if (iter->flow_spec.location == location)
			return iter;
	}
	return NULL;
}

static int validate_flow(struct hinic3_nic_dev *nic_dev,
			 const struct ethtool_rx_flow_spec *fs)
{
	if (fs->location >= MAX_NUM_OF_ETHTOOL_NTUPLE_RULES) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "loc exceed limit[0,%lu]\n",
			  MAX_NUM_OF_ETHTOOL_NTUPLE_RULES - 1);
		return -EINVAL;
	}

	if (fs->ring_cookie >= nic_dev->q_params.num_qps) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "action is larger than queue number %u\n",
			  nic_dev->q_params.num_qps);
		return -EINVAL;
	}

	switch (fs->flow_type) {
	case TCP_V4_FLOW:
	case UDP_V4_FLOW:
	case IP_USER_FLOW:
#ifndef UNSUPPORT_NTUPLE_IPV6
	case TCP_V6_FLOW:
	case UDP_V6_FLOW:
	case IPV6_USER_FLOW:
#endif
		break;
	default:
		nicif_err(nic_dev, drv, nic_dev->netdev, "flow type is not supported\n");
		return -EOPNOTSUPP;
	}

	return 0;
}

int hinic3_ethtool_flow_replace(struct hinic3_nic_dev *nic_dev,
				struct ethtool_rx_flow_spec *fs)
{
	struct hinic3_ethtool_rx_flow_rule *eth_rule = NULL;
	struct ethtool_rx_flow_spec flow_spec_temp;
	int loc_exit_flag = 0;
	int err;

	if (!HINIC3_SUPPORT_FDIR(nic_dev->hwdev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupported ntuple function\n");
		return -EOPNOTSUPP;
	}

	err = validate_flow(nic_dev, fs);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "flow is not valid %d\n", err);
		return err;
	}

	eth_rule = find_ethtool_rule(nic_dev, fs->location);
	/* when location is same, delete old location rule. */
	if (eth_rule) {
		memcpy(&flow_spec_temp, &eth_rule->flow_spec,
		       sizeof(struct ethtool_rx_flow_spec));
		err = hinic3_remove_one_rule(nic_dev, eth_rule);
		if (err)
			return err;

		loc_exit_flag = 1;
	}

	/* add new rule filter */
	err = hinic3_add_one_rule(nic_dev, fs);
	if (err) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Add new rule filter failed\n");
		if (loc_exit_flag)
			hinic3_add_one_rule(nic_dev, &flow_spec_temp);

		return -ENOENT;
	}

	return 0;
}

int hinic3_ethtool_flow_remove(struct hinic3_nic_dev *nic_dev, u32 location)
{
	struct hinic3_ethtool_rx_flow_rule *eth_rule = NULL;
	int err;

	if (!HINIC3_SUPPORT_FDIR(nic_dev->hwdev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupported ntuple function\n");
		return -EOPNOTSUPP;
	}

	if (location >= MAX_NUM_OF_ETHTOOL_NTUPLE_RULES)
		return -ENOSPC;

	eth_rule = find_ethtool_rule(nic_dev, location);
	if (!eth_rule)
		return -ENOENT;

	err = hinic3_remove_one_rule(nic_dev, eth_rule);

	return err;
}

int hinic3_ethtool_get_flow(const struct hinic3_nic_dev *nic_dev,
			    struct ethtool_rxnfc *info, u32 location)
{
	struct hinic3_ethtool_rx_flow_rule *eth_rule = NULL;

	if (!HINIC3_SUPPORT_FDIR(nic_dev->hwdev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupported ntuple function\n");
		return -EOPNOTSUPP;
	}

	if (location >= MAX_NUM_OF_ETHTOOL_NTUPLE_RULES)
		return -EINVAL;

	list_for_each_entry(eth_rule, &nic_dev->rx_flow_rule.rules, list) {
		if (eth_rule->flow_spec.location == location) {
			info->fs = eth_rule->flow_spec;
			return 0;
		}
	}

	return -ENOENT;
}

int hinic3_ethtool_get_all_flows(const struct hinic3_nic_dev *nic_dev,
				 struct ethtool_rxnfc *info, u32 *rule_locs)
{
	u32 idx = 0;
	struct hinic3_ethtool_rx_flow_rule *eth_rule = NULL;

	if (!HINIC3_SUPPORT_FDIR(nic_dev->hwdev)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupported ntuple function\n");
		return -EOPNOTSUPP;
	}

	info->data = MAX_NUM_OF_ETHTOOL_NTUPLE_RULES;
	list_for_each_entry(eth_rule, &nic_dev->rx_flow_rule.rules, list)
		rule_locs[idx++] = eth_rule->flow_spec.location;

	return info->rule_cnt == idx ? 0 : -ENOENT;
}

bool hinic3_validate_channel_setting_in_ntuple(const struct hinic3_nic_dev *nic_dev, u32 q_num)
{
	struct hinic3_ethtool_rx_flow_rule *iter = NULL;

	list_for_each_entry(iter, &nic_dev->rx_flow_rule.rules, list) {
		if (iter->flow_spec.ring_cookie >= q_num) {
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "User defined filter %u assigns flow to queue %llu. Queue number %u is invalid\n",
				  iter->flow_spec.location, iter->flow_spec.ring_cookie, q_num);
			return false;
		}
	}

	return true;
}

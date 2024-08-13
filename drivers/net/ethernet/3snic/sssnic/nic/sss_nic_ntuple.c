// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

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

#include "sss_kernel.h"
#include "sss_hw.h"
#include "sss_nic_cfg.h"
#include "sss_nic_vf_cfg.h"
#include "sss_nic_mag_cfg.h"
#include "sss_nic_rss_cfg.h"
#include "sss_nic_dev_define.h"

#define SSSNIC_MAX_ETHTOOL_NTUPLE_RULE				BIT(9)

#define SSSNIC_TCAM_IP_TYPE_MASK					0x1
#define SSSNIC_TCAM_TUNNEL_TYPE_MASK				0xF
#define SSSNIC_TCAM_FUNC_ID_MASK					0x7FFF

#define SSSNIC_TCAM_IPV4_TYPE 0
#define SSSNIC_TCAM_IPV6_TYPE 1

#ifndef UNSUPPORT_NTUPLE_IPV6
enum sss_nic_ipv6_parse_res {
	SSSNIC_IPV6_MASK_INVALID,
	SSSNIC_IPV6_MASK_ALL_MASK,
	SSSNIC_IPV6_MASK_ALL_ZERO,
};

enum sss_nic_ipv6_index {
	SSSNIC_IPV6_ID0,
	SSSNIC_IPV6_ID1,
	SSSNIC_IPV6_ID2,
	SSSNIC_IPV6_ID3,
};
#endif

struct sss_nic_ethtool_rx_flow_rule {
	struct list_head            list;
	struct ethtool_rx_flow_spec flow_spec;
};

static void sss_nic_calculate_tcam_key_y(u8 *key_y, const u8 *src_input, const u8 *mask, u8 len)
{
	u8 id;

	for (id = 0; id < len; id++)
		key_y[id] = src_input[id] & mask[id];
}

static void sss_nic_calculate_tcam_key_x(u8 *key_x, const u8 *key_y, const u8 *mask, u8 len)
{
	u8 id;

	for (id = 0; id < len; id++)
		key_x[id] = key_y[id] ^ mask[id];
}

static void sss_nic_calculate_tcam_key(struct sss_nic_tcam_key_tag *tcam_key,
				       struct sss_nic_tcam_rule_cfg *fdir_tcam_rule)
{
	sss_nic_calculate_tcam_key_y(fdir_tcam_rule->key.key_y,
				     (u8 *)(&tcam_key->key_info_ipv4),
				     (u8 *)(&tcam_key->key_mask_ipv4), SSSNIC_TCAM_FLOW_KEY_SIZE);
	sss_nic_calculate_tcam_key_x(fdir_tcam_rule->key.key_x, fdir_tcam_rule->key.key_y,
				     (u8 *)(&tcam_key->key_mask_ipv4), SSSNIC_TCAM_FLOW_KEY_SIZE);
}

static int sss_nic_parse_ipv4_base(struct sss_nic_dev *nic_dev,
				   struct ethtool_rx_flow_spec *flow_spec,
				   struct sss_nic_tcam_key_tag *tcam_key)
{
	u32 temp;
	struct ethtool_tcpip4_spec *val  = &flow_spec->h_u.tcp_ip4_spec;
	struct ethtool_tcpip4_spec *mask = &flow_spec->m_u.tcp_ip4_spec;

	if (mask->ip4src == U32_MAX) {
		temp = ntohl(val->ip4src);
		tcam_key->key_info_ipv4.sipv4_l = low_16_bits(temp);
		tcam_key->key_info_ipv4.sipv4_h = high_16_bits(temp);

		tcam_key->key_mask_ipv4.sipv4_l = U16_MAX;
		tcam_key->key_mask_ipv4.sipv4_h = U16_MAX;

	} else if (mask->ip4src != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Invalid source ip mask\n");
		return -EINVAL;
	}

	if (mask->ip4dst == U32_MAX) {
		temp = ntohl(val->ip4dst);
		tcam_key->key_info_ipv4.dipv4_l = low_16_bits(temp);
		tcam_key->key_info_ipv4.dipv4_h = high_16_bits(temp);

		tcam_key->key_mask_ipv4.dipv4_l = U16_MAX;
		tcam_key->key_mask_ipv4.dipv4_h = U16_MAX;

	} else if (mask->ip4dst != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Invalid destination ip mask\n");
		return -EINVAL;
	}

	tcam_key->key_mask_ipv4.ip_type = SSSNIC_TCAM_IP_TYPE_MASK;
	tcam_key->key_info_ipv4.ip_type = SSSNIC_TCAM_IPV4_TYPE;

	tcam_key->key_info_ipv4.func_id = sss_get_global_func_id(nic_dev->hwdev);
	tcam_key->key_mask_ipv4.func_id = SSSNIC_TCAM_FUNC_ID_MASK;

	return 0;
}

static int sss_nic_init_ipv4_l4_fdir_tcam(struct sss_nic_dev *nic_dev,
					  struct ethtool_rx_flow_spec *flow_spec,
		struct sss_nic_tcam_key_tag *tcam_key)
{
	struct ethtool_tcpip4_spec *l4_val  = &flow_spec->h_u.tcp_ip4_spec;
	struct ethtool_tcpip4_spec *l4_mask = &flow_spec->m_u.tcp_ip4_spec;
	int ret;

	ret = sss_nic_parse_ipv4_base(nic_dev, flow_spec, tcam_key);
	if (ret != 0)
		return ret;

	tcam_key->key_info_ipv4.dport = ntohs(l4_val->pdst);
	tcam_key->key_mask_ipv4.dport = l4_mask->pdst;

	tcam_key->key_info_ipv4.sport = ntohs(l4_val->psrc);
	tcam_key->key_mask_ipv4.sport = l4_mask->psrc;

	tcam_key->key_mask_ipv4.ip_proto = U8_MAX;
	if (flow_spec->flow_type == TCP_V4_FLOW)
		tcam_key->key_info_ipv4.ip_proto = IPPROTO_TCP;
	else
		tcam_key->key_info_ipv4.ip_proto = IPPROTO_UDP;

	return 0;
}

static int sss_nic_init_ipv4_fdir_tcam(struct sss_nic_dev *nic_dev,
				       struct ethtool_rx_flow_spec *flow_spec,
				       struct sss_nic_tcam_key_tag *tcam_key)
{
	int ret;
	struct ethtool_usrip4_spec *l3_val  = &flow_spec->h_u.usr_ip4_spec;
	struct ethtool_usrip4_spec *l3_mask = &flow_spec->m_u.usr_ip4_spec;

	ret = sss_nic_parse_ipv4_base(nic_dev, flow_spec, tcam_key);
	if (ret != 0)
		return ret;

	tcam_key->key_mask_ipv4.ip_proto = l3_mask->proto;
	tcam_key->key_info_ipv4.ip_proto = l3_val->proto;

	return 0;
}

#ifndef UNSUPPORT_NTUPLE_IPV6
static int sss_nic_parse_ipv6_mask(const u32 *ipv6_mask)
{
	if (ipv6_mask[SSSNIC_IPV6_ID0] == 0 && ipv6_mask[SSSNIC_IPV6_ID1] == 0 &&
	    ipv6_mask[SSSNIC_IPV6_ID2] == 0 && ipv6_mask[SSSNIC_IPV6_ID3] == 0)
		return SSSNIC_IPV6_MASK_ALL_ZERO;

	if (ipv6_mask[SSSNIC_IPV6_ID0] == U32_MAX &&
	    ipv6_mask[SSSNIC_IPV6_ID1] == U32_MAX &&
	    ipv6_mask[SSSNIC_IPV6_ID2] == U32_MAX && ipv6_mask[SSSNIC_IPV6_ID3] == U32_MAX)
		return SSSNIC_IPV6_MASK_ALL_MASK;

	return SSSNIC_IPV6_MASK_INVALID;
}

static int sss_nic_parse_ipv6_base(struct sss_nic_dev *nic_dev,
				   struct ethtool_rx_flow_spec *flow_spec,
				   struct sss_nic_tcam_key_tag *tcam_key)
{
	int parse_res;
	u32 temp;
	struct ethtool_tcpip6_spec *val  = &flow_spec->h_u.tcp_ip6_spec;
	struct ethtool_tcpip6_spec *mask = &flow_spec->m_u.tcp_ip6_spec;

	parse_res = sss_nic_parse_ipv6_mask((u32 *)mask->ip6src);
	if (parse_res == SSSNIC_IPV6_MASK_ALL_MASK) {
		tcam_key->key_mask_ipv6.sipv6_key0 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key1 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key2 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key3 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key4 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key5 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key6 = U16_MAX;
		tcam_key->key_mask_ipv6.sipv6_key7 = U16_MAX;

		temp = ntohl(val->ip6src[SSSNIC_IPV6_ID0]);
		tcam_key->key_info_ipv6.sipv6_key0 = high_16_bits(temp);
		tcam_key->key_info_ipv6.sipv6_key1 = low_16_bits(temp);
		temp = ntohl(val->ip6src[SSSNIC_IPV6_ID1]);
		tcam_key->key_info_ipv6.sipv6_key2 = high_16_bits(temp);
		tcam_key->key_info_ipv6.sipv6_key3 = low_16_bits(temp);
		temp = ntohl(val->ip6src[SSSNIC_IPV6_ID2]);
		tcam_key->key_info_ipv6.sipv6_key4 = high_16_bits(temp);
		tcam_key->key_info_ipv6.sipv6_key5 = low_16_bits(temp);
		temp = ntohl(val->ip6src[SSSNIC_IPV6_ID3]);
		tcam_key->key_info_ipv6.sipv6_key6 = high_16_bits(temp);
		tcam_key->key_info_ipv6.sipv6_key7 = low_16_bits(temp);

	} else if (parse_res == SSSNIC_IPV6_MASK_INVALID) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Invalid src_ipv6 mask\n");
		return -EINVAL;
	}

	parse_res = sss_nic_parse_ipv6_mask((u32 *)mask->ip6dst);
	if (parse_res == SSSNIC_IPV6_MASK_ALL_MASK) {
		tcam_key->key_mask_ipv6.dipv6_key0 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key1 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key2 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key3 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key4 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key5 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key6 = U16_MAX;
		tcam_key->key_mask_ipv6.dipv6_key7 = U16_MAX;

		temp = ntohl(val->ip6dst[SSSNIC_IPV6_ID0]);
		tcam_key->key_info_ipv6.dipv6_key0 = high_16_bits(temp);
		tcam_key->key_info_ipv6.dipv6_key1 = low_16_bits(temp);
		temp = ntohl(val->ip6dst[SSSNIC_IPV6_ID1]);
		tcam_key->key_info_ipv6.dipv6_key2 = high_16_bits(temp);
		tcam_key->key_info_ipv6.dipv6_key3 = low_16_bits(temp);
		temp = ntohl(val->ip6dst[SSSNIC_IPV6_ID2]);
		tcam_key->key_info_ipv6.dipv6_key4 = high_16_bits(temp);
		tcam_key->key_info_ipv6.dipv6_key5 = low_16_bits(temp);
		temp = ntohl(val->ip6dst[SSSNIC_IPV6_ID3]);
		tcam_key->key_info_ipv6.dipv6_key6 = high_16_bits(temp);
		tcam_key->key_info_ipv6.dipv6_key7 = low_16_bits(temp);

	} else if (parse_res == SSSNIC_IPV6_MASK_INVALID) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Invalid dst_ipv6 mask\n");
		return -EINVAL;
	}

	tcam_key->key_mask_ipv6.ip_type = SSSNIC_TCAM_IP_TYPE_MASK;
	tcam_key->key_info_ipv6.ip_type = SSSNIC_TCAM_IPV6_TYPE;

	tcam_key->key_info_ipv6.func_id =
		sss_get_global_func_id(nic_dev->hwdev);
	tcam_key->key_mask_ipv6.func_id = SSSNIC_TCAM_FUNC_ID_MASK;

	return 0;
}

static int sss_nic_init_ipv6_l4_fdir_tcam(struct sss_nic_dev *nic_dev,
					  struct ethtool_rx_flow_spec *flow_spec,
		struct sss_nic_tcam_key_tag *tcam_key)
{
	int ret;
	struct ethtool_tcpip6_spec *l4_val = &flow_spec->h_u.tcp_ip6_spec;
	struct ethtool_tcpip6_spec *l4_mask = &flow_spec->m_u.tcp_ip6_spec;

	ret = sss_nic_parse_ipv6_base(nic_dev, flow_spec, tcam_key);
	if (ret != 0)
		return ret;

	tcam_key->key_mask_ipv6.dport = l4_mask->pdst;
	tcam_key->key_info_ipv6.dport = ntohs(l4_val->pdst);

	tcam_key->key_mask_ipv6.sport = l4_mask->psrc;
	tcam_key->key_info_ipv6.sport = ntohs(l4_val->psrc);

	tcam_key->key_mask_ipv6.ip_proto = U8_MAX;
	if (flow_spec->flow_type == TCP_V6_FLOW)
		tcam_key->key_info_ipv6.ip_proto = NEXTHDR_TCP;
	else
		tcam_key->key_info_ipv6.ip_proto = NEXTHDR_UDP;

	return 0;
}

static int sss_nic_init_ipv6_fdir_tcam(struct sss_nic_dev *nic_dev,
				       struct ethtool_rx_flow_spec *flow_spec,
				       struct sss_nic_tcam_key_tag *tcam_key)
{
	int ret;
	struct ethtool_usrip6_spec *l3_mask = &flow_spec->m_u.usr_ip6_spec;
	struct ethtool_usrip6_spec *l3_val  = &flow_spec->h_u.usr_ip6_spec;

	ret = sss_nic_parse_ipv6_base(nic_dev, flow_spec, tcam_key);
	if (ret != 0)
		return ret;

	tcam_key->key_mask_ipv6.ip_proto = l3_mask->l4_proto;
	tcam_key->key_info_ipv6.ip_proto = l3_val->l4_proto;

	return 0;
}
#endif

static int sss_nic_init_fdir_tcam_info(struct sss_nic_dev *nic_dev,
				       struct ethtool_rx_flow_spec *flow_spec,
				       struct sss_nic_tcam_key_tag *tcam_key,
				       struct sss_nic_tcam_rule_cfg *fdir_tcam_rule)
{
	int ret;

	switch (flow_spec->flow_type) {
	case TCP_V4_FLOW:
	case UDP_V4_FLOW:
		ret = sss_nic_init_ipv4_l4_fdir_tcam(nic_dev, flow_spec, tcam_key);
		if (ret != 0)
			return ret;
		break;
	case IP_USER_FLOW:
		ret = sss_nic_init_ipv4_fdir_tcam(nic_dev, flow_spec, tcam_key);
		if (ret != 0)
			return ret;
		break;
#ifndef UNSUPPORT_NTUPLE_IPV6
	case TCP_V6_FLOW:
	case UDP_V6_FLOW:
		ret = sss_nic_init_ipv6_l4_fdir_tcam(nic_dev, flow_spec, tcam_key);
		if (ret != 0)
			return ret;
		break;
	case IPV6_USER_FLOW:
		ret = sss_nic_init_ipv6_fdir_tcam(nic_dev, flow_spec, tcam_key);
		if (ret != 0)
			return ret;
		break;
#endif
	default:
		return -EOPNOTSUPP;
	}

	tcam_key->key_mask_ipv4.tunnel_type = SSSNIC_TCAM_TUNNEL_TYPE_MASK;
	tcam_key->key_info_ipv4.tunnel_type = 0;

	fdir_tcam_rule->data.qid = (u32)flow_spec->ring_cookie;
	sss_nic_calculate_tcam_key(tcam_key, fdir_tcam_rule);

	return 0;
}

void sss_nic_flush_tcam_list(struct sss_nic_tcam_info *tcam_info)
{
	struct sss_nic_tcam_filter *filter_tmp = NULL;
	struct sss_nic_tcam_filter *filter = NULL;
	struct list_head *tcam_list = &tcam_info->tcam_list;

	if (list_empty(tcam_list))
		return;

	list_for_each_entry_safe(filter, filter_tmp,
				 tcam_list, tcam_filter_list) {
		list_del(&filter->tcam_filter_list);
		kfree(filter);
	}
}

void sss_nic_flush_tcam_node_list(struct sss_nic_tcam_info *tcam_info)
{
	struct sss_nic_tcam_node *block_tmp = NULL;
	struct sss_nic_tcam_node *block = NULL;
	struct list_head *dynamic_list =
			&tcam_info->tcam_node_info.tcam_node_list;

	if (list_empty(dynamic_list))
		return;

	list_for_each_entry_safe(block, block_tmp, dynamic_list, block_list) {
		list_del(&block->block_list);
		kfree(block);
	}
}

void sss_nic_flush_rx_flow_rule(struct sss_nic_rx_rule *rx_flow_rule)
{
	struct sss_nic_ethtool_rx_flow_rule *rule_tmp = NULL;
	struct sss_nic_ethtool_rx_flow_rule *rule = NULL;
	struct list_head *rule_list = &rx_flow_rule->rule_list;

	if (list_empty(rule_list))
		return;

	list_for_each_entry_safe(rule, rule_tmp, rule_list, list) {
		list_del(&rule->list);
		kfree(rule);
	}
}

void sss_nic_flush_tcam(struct sss_nic_dev *nic_dev)
{
	sss_nic_flush_tcam_list(&nic_dev->tcam_info);

	sss_nic_flush_tcam_node_list(&nic_dev->tcam_info);

	sss_nic_flush_rx_flow_rule(&nic_dev->rx_rule);

	if (SSSNIC_SUPPORT_FDIR(nic_dev->nic_io)) {
		sss_nic_flush_tcam_rule(nic_dev);
		sss_nic_set_fdir_tcam_rule_filter(nic_dev, false);
	}
}

static struct sss_nic_tcam_node *
sss_nic_alloc_tcam_block_resource(struct sss_nic_dev *nic_dev,
				  struct sss_nic_tcam_info *nic_tcam_info,
				  u16 block_id)
{
	struct sss_nic_tcam_node *dynamic_block_ptr = NULL;

	dynamic_block_ptr = kzalloc(sizeof(*dynamic_block_ptr), GFP_KERNEL);
	if (!dynamic_block_ptr)
		return NULL;

	dynamic_block_ptr->block_id = block_id;
	list_add_tail(&dynamic_block_ptr->block_list,
		      &nic_tcam_info->tcam_node_info.tcam_node_list);

	nic_tcam_info->tcam_node_info.block_cnt++;

	return dynamic_block_ptr;
}

static void sss_nic_free_tcam_block_resource(struct sss_nic_tcam_info *nic_tcam_info,
					     struct sss_nic_tcam_node *block_ptr)
{
	if (!block_ptr)
		return;

	list_del(&block_ptr->block_list);
	kfree(block_ptr);

	nic_tcam_info->tcam_node_info.block_cnt--;
}

static struct sss_nic_tcam_node *
sss_nic_dynamic_lookup_tcam_filter(struct sss_nic_dev *nic_dev,
				   struct sss_nic_tcam_rule_cfg *fdir_tcam_rule,
				   const struct sss_nic_tcam_info *tcam_info,
				   struct sss_nic_tcam_filter *tcam_filter,
				   u16 *tcam_index)
{
	u16 index;
	struct sss_nic_tcam_node *ptr = NULL;

	list_for_each_entry(ptr,
			    &tcam_info->tcam_node_info.tcam_node_list,
			    block_list)
		if (ptr->index_cnt < SSSNIC_TCAM_BLOCK_SIZE)
			break;

	if (!ptr || ptr->index_cnt >= SSSNIC_TCAM_BLOCK_SIZE) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to lookup index for fdir filter dynamic\n");
		return NULL;
	}

	for (index = 0; index < SSSNIC_TCAM_BLOCK_SIZE; index++)
		if (ptr->index_used[index] == 0)
			break;

	if (index == SSSNIC_TCAM_BLOCK_SIZE) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "tcam block 0x%x supports filter rules is full\n",
			  ptr->block_id);
		return NULL;
	}

	tcam_filter->block_id = ptr->block_id;
	tcam_filter->index = index;
	*tcam_index = index;

	fdir_tcam_rule->index = index +
				SSSNIC_PKT_TCAM_INDEX_START(ptr->block_id);

	return ptr;
}

static int sss_nic_add_tcam_filter(struct sss_nic_dev *nic_dev,
				   struct sss_nic_tcam_filter *tcam_filter,
				   struct sss_nic_tcam_rule_cfg *fdir_tcam_rule)
{
	int ret;
	struct sss_nic_tcam_info *tcam_info = &nic_dev->tcam_info;
	struct sss_nic_tcam_node *dynamic_block_ptr = NULL;
	struct sss_nic_tcam_node *tmp = NULL;
	u16 block_cnt = tcam_info->tcam_node_info.block_cnt;
	u16 tcam_block_index = 0;
	int block_alloc_flag = 0;
	u16 index = 0;

	if (tcam_info->tcam_rule_num >=
	    block_cnt * SSSNIC_TCAM_BLOCK_SIZE) {
		if (block_cnt >= (SSSNIC_TCAM_FILTERS_MAX /
				  SSSNIC_TCAM_BLOCK_SIZE)) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to alloc, dynamic tcam block is full\n");
			goto failed;
		}

		ret = sss_nic_alloc_tcam_block(nic_dev, &tcam_block_index);
		if (ret != 0) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to fdir filter dynamic tcam alloc block\n");
			goto failed;
		}

		block_alloc_flag = 1;

		dynamic_block_ptr =
			sss_nic_alloc_tcam_block_resource(nic_dev, tcam_info,
							  tcam_block_index);
		if (!dynamic_block_ptr) {
			nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to Fdir filter dynamic alloc block memory\n");
			goto block_alloc_failed;
		}
	}

	tmp = sss_nic_dynamic_lookup_tcam_filter(nic_dev,
						 fdir_tcam_rule, tcam_info,
			tcam_filter, &index);
	if (!tmp) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to dynamic lookup tcam filter\n");
		goto lookup_tcam_index_failed;
	}

	ret = sss_nic_add_tcam_rule(nic_dev, fdir_tcam_rule);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to add fdir_tcam_rule\n");
		goto add_tcam_rules_failed;
	}

	nicif_info(nic_dev, drv, nic_dev->netdev,
		   "Add fdir tcam rule, func_id: 0x%x, tcam_block_id: %d, local_index: %d, global_index: %d, queue: %d, tcam_rule_num: %d succeed\n",
		   sss_get_global_func_id(nic_dev->hwdev),
		   tcam_filter->block_id, index, fdir_tcam_rule->index,
		   fdir_tcam_rule->data.qid, tcam_info->tcam_rule_num + 1);

	if (tcam_info->tcam_rule_num == 0) {
		ret = sss_nic_set_fdir_tcam_rule_filter(nic_dev, true);
		if (ret != 0)
			goto enable_failed;
	}

	list_add_tail(&tcam_filter->tcam_filter_list, &tcam_info->tcam_list);

	tmp->index_used[index] = 1;
	tmp->index_cnt++;

	tcam_info->tcam_rule_num++;

	return 0;

enable_failed:
	sss_nic_del_tcam_rule(nic_dev, fdir_tcam_rule->index);

add_tcam_rules_failed:
lookup_tcam_index_failed:
	if (block_alloc_flag == 1)
		sss_nic_free_tcam_block_resource(tcam_info,
						 dynamic_block_ptr);

block_alloc_failed:
	if (block_alloc_flag == 1)
		sss_nic_free_tcam_block(nic_dev, &tcam_block_index);

failed:
	return -EFAULT;
}

static int sss_nic_del_tcam_filter(struct sss_nic_dev *nic_dev,
				   struct sss_nic_tcam_filter *tcam_filter)
{
	int ret;
	struct sss_nic_tcam_info *tcam_info = &nic_dev->tcam_info;
	u16 block_id = tcam_filter->block_id;
	struct sss_nic_tcam_node *ptr = NULL;
	u32 index = 0;

	list_for_each_entry(ptr,
			    &tcam_info->tcam_node_info.tcam_node_list,
			    block_list) {
		if (ptr->block_id == block_id)
			break;
	}
	if (!ptr || ptr->block_id != block_id) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to lookup block for fdir filter del dynamic\n");
		return -EFAULT;
	}

	index = SSSNIC_PKT_TCAM_INDEX_START(ptr->block_id) +
		tcam_filter->index;

	ret = sss_nic_del_tcam_rule(nic_dev, index);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to del fdir_tcam_rule\n");
		return -EFAULT;
	}

	nicif_info(nic_dev, drv, nic_dev->netdev,
		   "Del fdir_tcam_dynamic_rule func_id: 0x%x, tcam_block_id: %d, local_index: %d, global_index: %d, local_rules_nums: %d, global_rule_nums: %d succeed\n",
		   sss_get_global_func_id(nic_dev->hwdev), block_id,
		   tcam_filter->index, index, ptr->index_cnt - 1,
		   tcam_info->tcam_rule_num - 1);

	ptr->index_used[tcam_filter->index] = 0;
	ptr->index_cnt--;
	tcam_info->tcam_rule_num--;
	if (ptr->index_cnt == 0) {
		sss_nic_free_tcam_block(nic_dev, &block_id);
		sss_nic_free_tcam_block_resource(tcam_info, ptr);
	}

	if (tcam_info->tcam_rule_num == 0)
		sss_nic_set_fdir_tcam_rule_filter(nic_dev, false);

	list_del(&tcam_filter->tcam_filter_list);
	kfree(tcam_filter);

	return 0;
}

static inline struct sss_nic_tcam_filter *
sss_nic_lookup_tcam_filter(const struct list_head *filter_list,
			   struct sss_nic_tcam_key_tag *key)
{
	struct sss_nic_tcam_filter *ptr;

	list_for_each_entry(ptr, filter_list, tcam_filter_list) {
		if (memcmp(key, &ptr->tcam_key,
			   sizeof(*key)) == 0)
			return ptr;
	}

	return NULL;
}

static void sss_nic_del_ethtool_rule(struct sss_nic_dev *nic_dev,
				     struct sss_nic_ethtool_rx_flow_rule *eth_rule)
{
	list_del(&eth_rule->list);
	nic_dev->rx_rule.rule_cnt--;

	kfree(eth_rule);
}

static int sss_nic_del_one_rule(struct sss_nic_dev *nic_dev,
				struct sss_nic_ethtool_rx_flow_rule *eth_rule)
{
	int ret;
	struct sss_nic_tcam_info *tcam_info = &nic_dev->tcam_info;
	struct sss_nic_tcam_filter *tcam_filter;
	struct sss_nic_tcam_rule_cfg fdir_tcam_rule = {0};
	struct sss_nic_tcam_key_tag tcam_key = {0};

	ret = sss_nic_init_fdir_tcam_info(nic_dev, &eth_rule->flow_spec,
					  &tcam_key, &fdir_tcam_rule);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to init fdir info\n");
		return ret;
	}

	tcam_filter = sss_nic_lookup_tcam_filter(&tcam_info->tcam_list,
						 &tcam_key);
	if (!tcam_filter) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Filter does not exists\n");
		return -EEXIST;
	}

	ret = sss_nic_del_tcam_filter(nic_dev, tcam_filter);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to delete tcam filter\n");
		return ret;
	}

	sss_nic_del_ethtool_rule(nic_dev, eth_rule);

	return 0;
}

static void sss_nic_add_rule_to_list(struct sss_nic_dev *nic_dev,
				     struct sss_nic_ethtool_rx_flow_rule *rule)
{
	struct sss_nic_ethtool_rx_flow_rule *ptr = NULL;
	struct list_head *head = &nic_dev->rx_rule.rule_list;

	list_for_each_entry(ptr, &nic_dev->rx_rule.rule_list, list) {
		if (ptr->flow_spec.location > rule->flow_spec.location)
			break;
		head = &ptr->list;
	}
	nic_dev->rx_rule.rule_cnt++;
	list_add(&rule->list, head);
}

static int sss_nic_add_one_rule(struct sss_nic_dev *nic_dev,
				struct ethtool_rx_flow_spec *flow_spec)
{
	int ret;
	struct sss_nic_tcam_key_tag tcam_key = {0};
	struct sss_nic_tcam_rule_cfg fdir_tcam_rule = {0};
	struct sss_nic_tcam_filter *tcam_filter = NULL;
	struct sss_nic_ethtool_rx_flow_rule *eth_rule = NULL;
	struct sss_nic_tcam_info *tcam_info = &nic_dev->tcam_info;

	ret = sss_nic_init_fdir_tcam_info(nic_dev, flow_spec, &tcam_key,
					  &fdir_tcam_rule);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to init fdir info\n");
		return ret;
	}

	tcam_filter = sss_nic_lookup_tcam_filter(&tcam_info->tcam_list,
						 &tcam_key);
	if (tcam_filter) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Filter exists\n");
		return -EEXIST;
	}

	tcam_filter = kzalloc(sizeof(*tcam_filter), GFP_KERNEL);
	if (!tcam_filter)
		return -ENOMEM;
	memcpy(&tcam_filter->tcam_key,
	       &tcam_key, sizeof(tcam_key));
	tcam_filter->qid = (u16)fdir_tcam_rule.data.qid;

	ret = sss_nic_add_tcam_filter(nic_dev, tcam_filter, &fdir_tcam_rule);
	if (ret != 0)
		goto add_tcam_filter_fail;

	/* driver save new rule filter */
	eth_rule = kzalloc(sizeof(*eth_rule), GFP_KERNEL);
	if (!eth_rule) {
		ret = -ENOMEM;
		goto alloc_eth_rule_fail;
	}

	eth_rule->flow_spec = *flow_spec;
	sss_nic_add_rule_to_list(nic_dev, eth_rule);

	return 0;

alloc_eth_rule_fail:
	sss_nic_del_tcam_filter(nic_dev, tcam_filter);
add_tcam_filter_fail:
	kfree(tcam_filter);
	return ret;
}

static struct sss_nic_ethtool_rx_flow_rule *
sss_nic_ethtool_find_rule(const struct sss_nic_dev *nic_dev, u32 location)
{
	struct sss_nic_ethtool_rx_flow_rule *ptr = NULL;

	list_for_each_entry(ptr, &nic_dev->rx_rule.rule_list, list) {
		if (ptr->flow_spec.location == location)
			return ptr;
	}
	return NULL;
}

static int sss_nic_validate_flow(struct sss_nic_dev *nic_dev,
				 const struct ethtool_rx_flow_spec *flow_spec)
{
	int i;
	u32 flow_type[] = {
		TCP_V4_FLOW, UDP_V4_FLOW, IP_USER_FLOW,
#ifndef UNSUPPORT_NTUPLE_IPV6
		TCP_V6_FLOW, UDP_V6_FLOW, IPV6_USER_FLOW,
#endif
	};

	if (flow_spec->ring_cookie >= nic_dev->qp_res.qp_num) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Action larger than queue number %u\n",
			  nic_dev->qp_res.qp_num);
		return -EINVAL;
	}

	if (flow_spec->location >= SSSNIC_MAX_ETHTOOL_NTUPLE_RULE) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Invalid location out of range: [0,%lu]\n",
			  SSSNIC_MAX_ETHTOOL_NTUPLE_RULE);
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_LEN(flow_type); i++) {
		if (flow_spec->flow_type == flow_type[i])
			return 0;
	}

	nicif_err(nic_dev, drv, nic_dev->netdev, "flow type not supported\n");
	return -EOPNOTSUPP;
}

int sss_nic_ethtool_update_flow(struct sss_nic_dev *nic_dev,
				struct ethtool_rx_flow_spec *flow_spec)
{
	int ret;
	struct ethtool_rx_flow_spec flow_spec_temp;
	int loc_exit_flag = 0;
	struct sss_nic_ethtool_rx_flow_rule *eth_rule = NULL;

	if (!SSSNIC_SUPPORT_FDIR(nic_dev->nic_io)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupport ntuple function\n");
		return -EOPNOTSUPP;
	}

	ret = sss_nic_validate_flow(nic_dev, flow_spec);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev,
			  "flow is not valid %d\n", ret);
		return ret;
	}

	eth_rule = sss_nic_ethtool_find_rule(nic_dev, flow_spec->location);
	/* when location is same, delete old location rule. */
	if (eth_rule) {
		memcpy(&flow_spec_temp, &eth_rule->flow_spec,
		       sizeof(flow_spec_temp));
		ret = sss_nic_del_one_rule(nic_dev, eth_rule);
		if (ret != 0)
			return ret;

		loc_exit_flag = 1;
	}

	/* add new rule filter */
	ret = sss_nic_add_one_rule(nic_dev, flow_spec);
	if (ret != 0) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Fail to add new rule filter\n");
		if (loc_exit_flag)
			sss_nic_add_one_rule(nic_dev, &flow_spec_temp);

		return -ENOENT;
	}

	return 0;
}

int sss_nic_ethtool_delete_flow(struct sss_nic_dev *nic_dev, u32 location)
{
	int ret;
	struct sss_nic_ethtool_rx_flow_rule *eth_rule = NULL;

	if (!SSSNIC_SUPPORT_FDIR(nic_dev->nic_io)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupport ntuple function\n");
		return -EOPNOTSUPP;
	}

	if (location >= SSSNIC_MAX_ETHTOOL_NTUPLE_RULE)
		return -ENOSPC;

	eth_rule = sss_nic_ethtool_find_rule(nic_dev, location);
	if (!eth_rule)
		return -ENOENT;

	ret = sss_nic_del_one_rule(nic_dev, eth_rule);

	return ret;
}

int sss_nic_ethtool_get_flow(const struct sss_nic_dev *nic_dev,
			     struct ethtool_rxnfc *info, u32 location)
{
	struct sss_nic_ethtool_rx_flow_rule *nic_eth_rule = NULL;

	if (!SSSNIC_SUPPORT_FDIR(nic_dev->nic_io)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupported ntuple function\n");
		return -EOPNOTSUPP;
	}

	if (location >= SSSNIC_MAX_ETHTOOL_NTUPLE_RULE)
		return -EINVAL;

	list_for_each_entry(nic_eth_rule, &nic_dev->rx_rule.rule_list, list) {
		if (nic_eth_rule->flow_spec.location == location) {
			info->fs = nic_eth_rule->flow_spec;
			return 0;
		}
	}

	return -ENOENT;
}

int sss_nic_ethtool_get_all_flows(const struct sss_nic_dev *nic_dev,
				  struct ethtool_rxnfc *info, u32 *rule_locs)
{
	int id = 0;
	struct sss_nic_ethtool_rx_flow_rule *nic_eth_rule = NULL;

	if (!SSSNIC_SUPPORT_FDIR(nic_dev->nic_io)) {
		nicif_err(nic_dev, drv, nic_dev->netdev, "Unsupported ntuple function\n");
		return -EOPNOTSUPP;
	}

	info->data = SSSNIC_MAX_ETHTOOL_NTUPLE_RULE;
	list_for_each_entry(nic_eth_rule, &nic_dev->rx_rule.rule_list, list)
		rule_locs[id++] = nic_eth_rule->flow_spec.location;

	return info->rule_cnt == id ? 0 : -ENOENT;
}

bool sss_nic_validate_channel_setting_in_ntuple(const struct sss_nic_dev *nic_dev, u32 q_num)
{
	struct sss_nic_ethtool_rx_flow_rule *ptr = NULL;

	list_for_each_entry(ptr, &nic_dev->rx_rule.rule_list, list) {
		if (ptr->flow_spec.ring_cookie >= q_num) {
			nicif_err(nic_dev, drv, nic_dev->netdev,
				  "User defined filter %u assigns flow to queue %llu. Queue number %u is Invalid\n",
				  ptr->flow_spec.location, ptr->flow_spec.ring_cookie, q_num);
			return false;
		}
	}

	return true;
}

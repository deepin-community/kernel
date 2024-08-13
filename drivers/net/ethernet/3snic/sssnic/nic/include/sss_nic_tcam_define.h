/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_TCAM_DEFINE_H
#define SSS_NIC_TCAM_DEFINE_H

#include <linux/types.h>
#include <linux/list.h>

#include "sss_kernel.h"

#define SSSNIC_TCAM_BLOCK_SIZE	16
#define SSSNIC_TCAM_FILTERS_MAX			512

#define SSSNIC_PKT_TCAM_INDEX_START(block_index) \
		(SSSNIC_TCAM_BLOCK_SIZE * (block_index))

#define SSSNIC_TCAM_FLOW_KEY_SIZE (44)

#define SSSNIC_TCAM_RULE_FDIR_TYPE 0
#define SSSNIC_TCAM_RULE_PPA_TYPE  1

#define SSSNIC_TCAM_BLOCK_ENABLE      1
#define SSSNIC_TCAM_BLOCK_DISABLE     0
#define SSSNIC_TCAM_RULES_NUM_MAX   4096

/* tcam block type, according to tcam block size */
enum {
	SSSNIC_TCAM_BLOCK_TYPE_LARGE = 0, /* block_size: 16 */
	SSSNIC_TCAM_BLOCK_TYPE_SMALL,     /* block_size: 0 */
	SSSNIC_TCAM_BLOCK_TYPE_MAX
};

struct sss_nic_ipv4_tcam_key {
	u32 rsvd1 : 4;
	u32 tunnel_type : 4;
	u32 ip_proto : 8;
	u32 rsvd0 : 16;
	u32 sipv4_h : 16;
	u32 ip_type : 1;
	u32 func_id : 15;
	u32 dipv4_h : 16;
	u32 sipv4_l : 16;
	u32 rsvd2 : 16;
	u32 dipv4_l : 16;
	u32 rsvd3;
	u32 dport : 16;
	u32 rsvd4 : 16;
	u32 rsvd5 : 16;
	u32 sport : 16;
	u32 outer_sipv4_h : 16;
	u32 rsvd6 : 16;
	u32 outer_dipv4_h : 16;
	u32 outer_sipv4_l : 16;
	u32 vni_h : 16;
	u32 outer_dipv4_l : 16;
	u32 rsvd7 : 16;
	u32 vni_l : 16;
};

struct sss_nic_ipv6_tcam_key {
	u32 rsvd1 : 4;
	u32 tunnel_type : 4;
	u32 ip_proto : 8;
	u32 rsvd0 : 16;
	u32 sipv6_key0 : 16;
	u32 ip_type : 1;
	u32 func_id : 15;
	u32 sipv6_key2 : 16;
	u32 sipv6_key1 : 16;
	u32 sipv6_key4 : 16;
	u32 sipv6_key3 : 16;
	u32 sipv6_key6 : 16;
	u32 sipv6_key5 : 16;
	u32 dport : 16;
	u32 sipv6_key7 : 16;
	u32 dipv6_key0 : 16;
	u32 sport : 16;
	u32 dipv6_key2 : 16;
	u32 dipv6_key1 : 16;
	u32 dipv6_key4 : 16;
	u32 dipv6_key3 : 16;
	u32 dipv6_key6 : 16;
	u32 dipv6_key5 : 16;
	u32 rsvd2 : 16;
	u32 dipv6_key7 : 16;
};

struct sss_nic_vxlan_ipv6_tcam_key {
	u32 rsvd1 : 4;
	u32 tunnel_type : 4;
	u32 ip_proto : 8;
	u32 rsvd0 : 16;

	u32 dipv6_key0 : 16;
	u32 ip_type : 1;
	u32 func_id : 15;

	u32 dipv6_key2 : 16;
	u32 dipv6_key1 : 16;

	u32 dipv6_key4 : 16;
	u32 dipv6_key3 : 16;

	u32 dipv6_key6 : 16;
	u32 dipv6_key5 : 16;

	u32 dport : 16;
	u32 dipv6_key7 : 16;

	u32 rsvd2 : 16;
	u32 sport : 16;

	u32 outer_sipv4_h : 16;
	u32 rsvd3 : 16;

	u32 outer_dipv4_h : 16;
	u32 outer_sipv4_l : 16;

	u32 vni_h : 16;
	u32 outer_dipv4_l : 16;

	u32 rsvd4 : 16;
	u32 vni_l : 16;
};

struct sss_nic_tcam_key_tag {
	union {
		struct sss_nic_ipv4_tcam_key key_info_ipv4;
		struct sss_nic_ipv6_tcam_key key_info_ipv6;
		struct sss_nic_vxlan_ipv6_tcam_key key_info_vxlan_ipv6;
	};

	union {
		struct sss_nic_ipv4_tcam_key key_mask_ipv4;
		struct sss_nic_ipv6_tcam_key key_mask_ipv6;
		struct sss_nic_vxlan_ipv6_tcam_key key_mask_vxlan_ipv6;
	};
};

struct sss_nic_tcam_node {
	struct list_head block_list;
	u16 block_id;
	u16 index_cnt;
	u8 index_used[SSSNIC_TCAM_BLOCK_SIZE];
};

struct sss_nic_tcam_node_list {
	struct list_head tcam_node_list;
	u16 block_cnt;
};

struct sss_nic_tcam_filter {
	struct list_head tcam_filter_list;
	u16 block_id;
	u16 index;
	struct sss_nic_tcam_key_tag tcam_key;
	u16 qid;
};

/* function level struct info */
struct sss_nic_tcam_info {
	u16 tcam_rule_num;
	struct list_head tcam_list;
	struct sss_nic_tcam_node_list tcam_node_info;
};

struct sss_nic_tcam_result {
	u32 qid;
	u32 rsvd;
};

struct sss_nic_tcam_key {
	u8 key_x[SSSNIC_TCAM_FLOW_KEY_SIZE];
	u8 key_y[SSSNIC_TCAM_FLOW_KEY_SIZE];
};

struct sss_nic_tcam_rule_cfg {
	u32 index;
	struct sss_nic_tcam_result data;
	struct sss_nic_tcam_key key;
};

#endif

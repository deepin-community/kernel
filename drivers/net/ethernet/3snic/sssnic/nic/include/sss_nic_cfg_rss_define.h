/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_CFG_RSS_DEFINE_H
#define SSS_NIC_CFG_RSS_DEFINE_H

#include <linux/types.h>

/* rss */
#define SSSNIC_RSS_TYPE_VALID_SHIFT 23
#define SSSNIC_RSS_TYPE_TCP_IPV6_EXT_SHIFT 24
#define SSSNIC_RSS_TYPE_IPV6_EXT_SHIFT 25
#define SSSNIC_RSS_TYPE_TCP_IPV6_SHIFT 26
#define SSSNIC_RSS_TYPE_IPV6_SHIFT 27
#define SSSNIC_RSS_TYPE_TCP_IPV4_SHIFT 28
#define SSSNIC_RSS_TYPE_IPV4_SHIFT 29
#define SSSNIC_RSS_TYPE_UDP_IPV6_SHIFT 30
#define SSSNIC_RSS_TYPE_UDP_IPV4_SHIFT 31

#define SSSNIC_RSS_TYPE_SET(val, member) (((u32)(val) & 0x1) << SSSNIC_RSS_TYPE_##member##_SHIFT)
#define SSSNIC_RSS_TYPE_GET(val, member) (((u32)(val) >> SSSNIC_RSS_TYPE_##member##_SHIFT) & 0x1)

#define SSSNIC_RSS_KEY_RSV_NUM 2

#define SSSNIC_RSS_INDIR_SIZE      256
#define SSSNIC_RSS_KEY_SIZE        40

enum sss_nic_rss_hash_engine_type {
	SSSNIC_RSS_ENGINE_XOR = 0,
	SSSNIC_RSS_ENGINE_TOEP,
	SSSNIC_RSS_ENGINE_MAX,
};

struct sss_nic_rss_type {
	u8 tcp_ipv6_ext;
	u8 ipv6_ext;
	u8 tcp_ipv6;
	u8 ipv6;
	u8 tcp_ipv4;
	u8 ipv4;
	u8 udp_ipv6;
	u8 udp_ipv4;
};

/* rss */
struct sss_nic_rss_indirect_table {
	u32 rsvd[4]; /* Make sure that 16B beyond entry[] */
	u16 entry[SSSNIC_RSS_INDIR_SIZE];
};

struct sss_nic_rss_ctx_table {
	u32 rsvd[4];
	u32 ctx;
};

#endif

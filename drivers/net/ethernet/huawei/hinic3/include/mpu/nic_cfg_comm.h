/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2024 Huawei Technologies Co., Ltd */

#ifndef NIC_CFG_COMM_H
#define NIC_CFG_COMM_H

/* rss */
#define HINIC3_RSS_TYPE_VALID_SHIFT 23
#define HINIC3_RSS_TYPE_TCP_IPV6_EXT_SHIFT 24
#define HINIC3_RSS_TYPE_IPV6_EXT_SHIFT 25
#define HINIC3_RSS_TYPE_TCP_IPV6_SHIFT 26
#define HINIC3_RSS_TYPE_IPV6_SHIFT 27
#define HINIC3_RSS_TYPE_TCP_IPV4_SHIFT 28
#define HINIC3_RSS_TYPE_IPV4_SHIFT 29
#define HINIC3_RSS_TYPE_UDP_IPV6_SHIFT 30
#define HINIC3_RSS_TYPE_UDP_IPV4_SHIFT 31

#define HINIC3_RSS_TYPE_SET(val, member) (((u32)(val) & 0x1) << HINIC3_RSS_TYPE_##member##_SHIFT)
#define HINIC3_RSS_TYPE_GET(val, member) (((u32)(val) >> HINIC3_RSS_TYPE_##member##_SHIFT) & 0x1)

enum nic_rss_hash_type {
	NIC_RSS_HASH_TYPE_XOR = 0,
	NIC_RSS_HASH_TYPE_TOEP,

	NIC_RSS_HASH_TYPE_MAX	/* MUST BE THE LAST ONE */
};

#define NIC_RSS_INDIR_SIZE	256
#define NIC_RSS_KEY_SIZE		40

/* *
 * Definition of the NIC receiving mode
 */
#define NIC_RX_MODE_UC		0x01
#define NIC_RX_MODE_MC		0x02
#define NIC_RX_MODE_BC		0x04
#define NIC_RX_MODE_MC_ALL	0x08
#define NIC_RX_MODE_PROMISC	0x10
#define NIC_RX_DB_COS_MAX       0x4

/* IEEE 802.1Qaz std */
#define NIC_DCB_COS_MAX		0x8
#define NIC_DCB_UP_MAX		0x8
#define NIC_DCB_TC_MAX		0x8
#define NIC_DCB_PG_MAX		0x8
#define NIC_DCB_TSA_SP		0x0
#define NIC_DCB_TSA_CBS		0x1
#define NIC_DCB_TSA_ETS		0x2
#define NIC_DCB_DSCP_NUM	0x8
#define NIC_DCB_IP_PRI_MAX	0x40

#define NIC_DCB_PRIO_DWRR	0x0
#define NIC_DCB_PRIO_STRICT	0x1

#define NIC_DCB_MAX_PFC_NUM	0x4

#define NIC_ETS_PERCENT_WEIGHT    100

#endif

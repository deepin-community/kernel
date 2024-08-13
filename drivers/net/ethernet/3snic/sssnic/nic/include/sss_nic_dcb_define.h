/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_DCB_DEFINE_H
#define SSS_NIC_DCB_DEFINE_H

#include <linux/types.h>

#define SSSNIC_PCP_UP_MAX 8
#define SSSNIC_DSCP_MAC_UP 64

/* IEEE 802.1Qaz std */
#define SSSNIC_DCB_COS_MAX         0x8
#define SSSNIC_DCB_UP_MAX          0x8
#define SSSNIC_DCB_TC_MAX          0x8
#define SSSNIC_DCB_PG_MAX          0x8
#define SSSNIC_DCB_TSA_SP          0x0
#define SSSNIC_DCB_TSA_CBS         0x1
#define SSSNIC_DCB_TSA_ETS         0x2
#define SSSNIC_DCB_DSCP_NUM        0x8
#define SSSNIC_DCB_IP_PRI_MAX      0x40

#define SSSNIC_DCB_PRIO_DWRR       0x0
#define SSSNIC_DCB_PRIO_STRICT     0x1

#define SSSNIC_DCB_MAX_PFC_NUM     0x4

struct sss_nic_dcb_config {
	u8	trust; /* pcp, dscp */
	u8	default_cos;
	u8	pcp_user_cos_num;
	u8	pcp_valid_cos_map;
	u8	dscp_user_cos_num;
	u8	dscp_valid_cos_map;
	u8	pcp2cos[SSSNIC_PCP_UP_MAX];
	u8	dscp2cos[SSSNIC_DSCP_MAC_UP];

	u8	cos_qp_offset[SSSNIC_DCB_COS_MAX];
	u8	cos_qp_num[SSSNIC_DCB_COS_MAX];
};

struct sss_nic_dcb_info {
	u8 dcb_on;
	u8 default_cos;
	u8 trust;
	u8 rsvd1;
	u8 pcp2cos[SSSNIC_DCB_UP_MAX];
	u8 dscp2cos[64];
	u32 rsvd2[7];
};

#endif

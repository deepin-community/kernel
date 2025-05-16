/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_ENHANCED_CMDQ_H
#define HINIC3_ENHANCED_CMDQ_H

#include "hinic3_hw.h"

enum complete_format {
	INLINE_DATA = 0,
	SGE_RESPONSE = 1,
};

#define	HINIC3_CMDQ_MAX_INLINE_DATA_SIZE	160U
#define HINIC3_CMDQ_WQE_INLINE_DATA_PI_OFFSET	2

/* first part 16B */
#define ENHANCED_CMDQ_CTXT0_CI_WQE_ADDR_SHIFT		0
#define ENHANCED_CMDQ_CTXT0_RSV1_SHIFT			52
#define ENHANCED_CMDQ_CTXT0_EQ_SHIFT			53
#define ENHANCED_CMDQ_CTXT0_CEQ_ARM_SHIFT		61
#define ENHANCED_CMDQ_CTXT0_CEQ_EN_SHIFT		62
#define ENHANCED_CMDQ_CTXT0_HW_BUSY_BIT_SHIFT		63

#define ENHANCED_CMDQ_CTXT0_CI_WQE_ADDR_MASK		0xFFFFFFFFFFFFFU
#define ENHANCED_CMDQ_CTXT0_RSV1_MASK			0x1U
#define ENHANCED_CMDQ_CTXT0_EQ_MASK			0xFFU
#define ENHANCED_CMDQ_CTXT0_CEQ_ARM_MASK		0x1U
#define ENHANCED_CMDQ_CTXT0_CEQ_EN_MASK			0x1U
#define ENHANCED_CMDQ_CTXT0_HW_BUSY_BIT_MASK		0x1U

#define ENHANCED_CMDQ_CTXT1_Q_DIS_SHIFT			0
#define ENHANCED_CMDQ_CTXT1_ERR_CODE_SHIFT		1
#define ENHANCED_CMDQ_CTXT1_RSV1_SHIFT			3
#define ENHANCED_CMDQ_CTXT1_PI_SHIFT			32
#define ENHANCED_CMDQ_CTXT1_CI_SHIFT			48

#define ENHANCED_CMDQ_CTXT1_Q_DIS_MASK			0x1U
#define ENHANCED_CMDQ_CTXT1_ERR_CODE_MASK		0x3U
#define ENHANCED_CMDQ_CTXT1_RSV1_MASK			0x1FFFFFFFU
#define ENHANCED_CMDQ_CTXT1_PI_MASK			0xFFFFU
#define ENHANCED_CMDQ_CTXT1_CI_MASK			0xFFFFU

/* second PART 16B */
#define ENHANCED_CMDQ_CTXT2_PFT_CI_SHIFT		0
#define ENHANCED_CMDQ_CTXT2_O_BIT_SHIFT			4
#define ENHANCED_CMDQ_CTXT2_PFT_THD_SHIFT		32
#define ENHANCED_CMDQ_CTXT2_PFT_MAX_SHIFT		46
#define ENHANCED_CMDQ_CTXT2_PFT_MIN_SHIFT		57

#define ENHANCED_CMDQ_CTXT2_PFT_CI_MASK			0xFU
#define ENHANCED_CMDQ_CTXT2_O_BIT_MASK			0x1U
#define ENHANCED_CMDQ_CTXT2_PFT_THD_MASK		0x3FFFFU
#define ENHANCED_CMDQ_CTXT2_PFT_MAX_MASK		0x7FFFU
#define ENHANCED_CMDQ_CTXT2_PFT_MIN_MASK		0x7FU

#define ENHANCED_CMDQ_CTXT3_PFT_CI_ADDR_SHIFT		0
#define ENHANCED_CMDQ_CTXT3_PFT_CI_SHIFT		52

#define ENHANCED_CMDQ_CTXT3_PFT_CI_ADDR_MASK		0xFFFFFFFFFFFFFU
#define ENHANCED_CMDQ_CTXT3_PFT_CI_MASK			0xFFFFU

/* THIRD PART 16B */
#define ENHANCED_CMDQ_CTXT4_CI_CLA_ADDR_SHIFT		0

#define ENHANCED_CMDQ_CTXT4_CI_CLA_ADDR_MASK		0x7FFFFFFFFFFFFFU

#define ENHANCED_CMDQ_SET(val, member)		\
			(((u64)(val) & ENHANCED_CMDQ_##member##_MASK) << \
			 ENHANCED_CMDQ_##member##_SHIFT)

#define WQ_PREFETCH_MAX			4
#define WQ_PREFETCH_MIN			1
#define WQ_PREFETCH_THRESHOLD		256

#define CI_IDX_HIGH_SHIFH		12
#define CI_HIGN_IDX(val)		((val) >> CI_IDX_HIGH_SHIFH)

#define ENHANCE_CMDQ_WQE_HEADER_SEND_SGE_LEN_SHIFT		0
#define ENHANCE_CMDQ_WQE_HEADER_BDSL_SHIFT			19
#define ENHANCE_CMDQ_WQE_HEADER_DF_SHIFT			28
#define ENHANCE_CMDQ_WQE_HEADER_DN_SHIFT			29
#define ENHANCE_CMDQ_WQE_HEADER_EC_SHIFT			30
#define ENHANCE_CMDQ_WQE_HEADER_HW_BUSY_BIT_SHIFT		31

#define ENHANCE_CMDQ_WQE_HEADER_SEND_SGE_LEN_MASK		0x3FFFFU
#define ENHANCE_CMDQ_WQE_HEADER_BDSL_MASK			0xFFU
#define ENHANCE_CMDQ_WQE_HEADER_DF_MASK				0x1U
#define ENHANCE_CMDQ_WQE_HEADER_DN_MASK				0x1U
#define ENHANCE_CMDQ_WQE_HEADER_EC_MASK				0x1U
#define ENHANCE_CMDQ_WQE_HEADER_HW_BUSY_BIT_MASK		0x1U

#define ENHANCE_CMDQ_WQE_HEADER_SET(val, member)		\
			((((u32)(val)) & ENHANCE_CMDQ_WQE_HEADER_##member##_MASK) << \
			 ENHANCE_CMDQ_WQE_HEADER_##member##_SHIFT)

#define ENHANCE_CMDQ_WQE_HEADER_GET(val, member)		\
			(((val) >> ENHANCE_CMDQ_WQE_HEADER_##member##_SHIFT) & \
			 ENHANCE_CMDQ_WQE_HEADER_##member##_MASK)

#define ENHANCE_CMDQ_WQE_CS_ERR_CODE_SHIFT			0
#define ENHANCE_CMDQ_WQE_CS_CMD_SHIFT				4
#define ENHANCE_CMDQ_WQE_CS_ACK_TYPE_SHIFT			12
#define ENHANCE_CMDQ_WQE_CS_HW_BUSY_SHIFT			14
#define ENHANCE_CMDQ_WQE_CS_RN_SHIFT				15
#define ENHANCE_CMDQ_WQE_CS_MOD_SHIFT				16
#define ENHANCE_CMDQ_WQE_CS_CF_SHIFT				31

#define ENHANCE_CMDQ_WQE_CS_ERR_CODE_MASK			0xFU
#define ENHANCE_CMDQ_WQE_CS_CMD_MASK				0xFFU
#define ENHANCE_CMDQ_WQE_CS_ACK_TYPE_MASK			0x3U
#define ENHANCE_CMDQ_WQE_CS_HW_BUSY_MASK			0x1U
#define ENHANCE_CMDQ_WQE_CS_RN_MASK				0x1U
#define ENHANCE_CMDQ_WQE_CS_MOD_MASK				0x1FU
#define ENHANCE_CMDQ_WQE_CS_CF_MASK				0x1U

#define ENHANCE_CMDQ_WQE_CS_SET(val, member)		\
			((((u32)(val)) & ENHANCE_CMDQ_WQE_CS_##member##_MASK) << \
			 ENHANCE_CMDQ_WQE_CS_##member##_SHIFT)

#define ENHANCE_CMDQ_WQE_CS_GET(val, member)		\
			(((val) >> ENHANCE_CMDQ_WQE_CS_##member##_SHIFT) & \
			 ENHANCE_CMDQ_WQE_CS_##member##_MASK)

struct hinic3_cmdq_enhance_completion {
	u32 cs_format;
	u32 sge_resp_hi_addr;
	u32 sge_resp_lo_addr;
	u32 sge_resp_len; /* bit 14~31 rsvd, soft can't use. */
};

struct hinic3_cmdq_enhance_response {
	u32 cs_format;
	u32 resvd;
	u64 direct_data;
};

struct sge_send_info {
	u32 sge_hi_addr;
	u32 sge_li_addr;
	u32 seg_len;
	u32 rsvd;
};

#define NORMAL_WQE_TYPE  0
#define COMPACT_WQE_TYPE 1
struct hinic3_ctrl_section {
	u32 header;
	u32 rsv;
	u32 sge_send_hi_addr;
	u32 sge_send_lo_addr;
};

struct hinic3_enhanced_cmd_bufdesc {
	u32 len;
	u32 rsv;
	u32 sge_send_hi_addr;
	u32 sge_send_lo_addr;
};

struct hinic3_enhanced_cmdq_wqe {
	struct hinic3_ctrl_section		ctrl_sec; /* 16B */
	struct hinic3_cmdq_enhance_completion	completion; /* 16B */
	union {
		struct hinic3_enhanced_cmd_bufdesc	buf_desc[2]; /* 32B */
		u8 inline_data[HINIC3_CMDQ_MAX_INLINE_DATA_SIZE]; /* 160B max */
	};
};

#endif

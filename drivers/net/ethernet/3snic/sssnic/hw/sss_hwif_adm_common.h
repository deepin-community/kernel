/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HWIF_ADM_COMMON_H
#define SSS_HWIF_ADM_COMMON_H

#define SSS_ADM_MSG_AEQ_ID					2

#define SSS_WRITE_ADM_MSG_PRIV_DATA(id)			(((u8)(id)) << 16)
#define SSS_READ_ADM_MSG_PRIV_DATA(id, token)	((((u32)(id)) << 16) + (token))

#define SSS_MASK_ID(adm_msg, id)			\
		((id) & ((adm_msg)->elem_num - 1))

#define SSS_SIZE_TO_4B(size)				\
		(ALIGN((u32)(size), 4U) >> 2)
#define SSS_SIZE_TO_8B(size)				\
		(ALIGN((u32)(size), 8U) >> 3)

/* ADM_STATUS_0 CSR: 0x0030+adm msg id*0x080 */
#define SSS_ADM_MSG_STATE_CI_MASK			0xFFFFFFU
#define SSS_ADM_MSG_STATE_CI_SHIFT		0

#define SSS_ADM_MSG_STATE_FSM_MASK				0xFU
#define SSS_ADM_MSG_STATE_FSM_SHIFT				24

#define SSS_ADM_MSG_STATE_CHKSUM_ERR_MASK		0x3U
#define SSS_ADM_MSG_STATE_CHKSUM_ERR_SHIFT		28

#define SSS_ADM_MSG_STATE_CPLD_ERR_MASK			0x1U
#define SSS_ADM_MSG_STATE_CPLD_ERR_SHIFT		30

#define SSS_GET_ADM_MSG_STATE(val, member)			\
		(((val) >> SSS_ADM_MSG_STATE_##member##_SHIFT) & \
			SSS_ADM_MSG_STATE_##member##_MASK)

/* adm_msg_elem.desc structure */
#define SSS_ADM_MSG_DESC_SGL_TYPE_SHIFT				0
#define SSS_ADM_MSG_DESC_RD_WR_SHIFT				1
#define SSS_ADM_MSG_DESC_MGMT_BYPASS_SHIFT			2
#define SSS_ADM_MSG_DESC_REPLY_AEQE_EN_SHIFT		3
#define SSS_ADM_MSG_DESC_MSG_VALID_SHIFT			4
#define SSS_ADM_MSG_DESC_MSG_CHANNEL_SHIFT			6
#define SSS_ADM_MSG_DESC_PRIV_DATA_SHIFT			8
#define SSS_ADM_MSG_DESC_DEST_SHIFT					32
#define SSS_ADM_MSG_DESC_SIZE_SHIFT					40
#define SSS_ADM_MSG_DESC_XOR_CHKSUM_SHIFT			56

#define SSS_ADM_MSG_DESC_SGL_TYPE_MASK				0x1U
#define SSS_ADM_MSG_DESC_RD_WR_MASK					0x1U
#define SSS_ADM_MSG_DESC_MGMT_BYPASS_MASK			0x1U
#define SSS_ADM_MSG_DESC_REPLY_AEQE_EN_MASK			0x1U
#define SSS_ADM_MSG_DESC_MSG_VALID_MASK				0x3U
#define SSS_ADM_MSG_DESC_MSG_CHANNEL_MASK			0x3U
#define SSS_ADM_MSG_DESC_PRIV_DATA_MASK				0xFFFFFFU
#define SSS_ADM_MSG_DESC_DEST_MASK					0x1FU
#define SSS_ADM_MSG_DESC_SIZE_MASK					0x7FFU
#define SSS_ADM_MSG_DESC_XOR_CHKSUM_MASK				0xFFU

#define SSS_ADM_MSG_DESC_SET(val, member)			\
		((((u64)(val)) & SSS_ADM_MSG_DESC_##member##_MASK) << \
			SSS_ADM_MSG_DESC_##member##_SHIFT)

/* adm_msg_elem structure */
#define SSS_ADM_MSG_ELEM_CTRL_ELEM_LEN_SHIFT			0
#define SSS_ADM_MSG_ELEM_CTRL_RD_DMA_ATTR_OFF_SHIFT		16
#define SSS_ADM_MSG_ELEM_CTRL_WR_DMA_ATTR_OFF_SHIFT		24
#define SSS_ADM_MSG_ELEM_CTRL_XOR_CHKSUM_SHIFT			56

#define SSS_ADM_MSG_ELEM_CTRL_ELEM_LEN_MASK				0x3FU
#define SSS_ADM_MSG_ELEM_CTRL_RD_DMA_ATTR_OFF_MASK		0x3FU
#define SSS_ADM_MSG_ELEM_CTRL_WR_DMA_ATTR_OFF_MASK		0x3FU
#define SSS_ADM_MSG_ELEM_CTRL_XOR_CHKSUM_MASK			0xFFU

#define SSS_ADM_MSG_ELEM_CTRL_SET(val, member)		\
		((((u64)(val)) & SSS_ADM_MSG_ELEM_CTRL_##member##_MASK) << \
			SSS_ADM_MSG_ELEM_CTRL_##member##_SHIFT)

#endif

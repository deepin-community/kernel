/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_TOOL_NIC_H
#define SSS_TOOL_NIC_H

#define SSS_TOOL_DCB_OPCODE_WR   BIT(0)  /* 1 - write, 0 - read */

#define SSS_TOOL_MSG_QOS_DEV_TRUST     BIT(0)
#define SSS_TOOL_MSG_QOS_DEV_DFT_COS   BIT(1)
#define SSS_TOOL_MSG_QOS_DEV_PCP2COS   BIT(2)
#define SSS_TOOL_MSG_QOS_DEV_DSCP2COS  BIT(3)

struct sss_tool_loop_mode {
	u32 loop_mode;
	u32 loop_ctrl;
};

struct sss_tool_wqe_info {
	int q_id;
	void *slq_handle;
	unsigned int wqe_id;
};

struct sss_tool_hw_page {
	u64 phy_addr;
	u64 *map_addr;
};

struct sss_tool_sq_info {
	u16 q_id;
	u16 pi;
	u16 ci; /* sw_ci */
	u16 fi; /* hw_ci */
	u32 q_depth;
	u16 pi_reverse; /* TODO: what is this? */
	u16 wqebb_size;
	u8 priority;
	u16 *ci_addr;
	u64 cla_addr;
	void *slq_handle;
	/* TODO: NIC don't use direct wqe */
	struct sss_tool_hw_page direct_wqe;
	struct sss_tool_hw_page doorbell;
	u32 page_idx;
	u32 glb_sq_id;
};

struct sss_tool_rq_info {
	u16 q_id;
	u16 delta;
	u16 hw_pi;
	u16 ci; /* sw_ci */
	u16 sw_pi;
	u16 wqebb_size;
	u16 q_depth;
	u16 buf_len;

	void *slq_handle;
	u64 ci_wqe_page_addr;
	u64 ci_cla_tbl_addr;

	u8 coalesc_timer_cfg;
	u8 pending_limt;
	u16 msix_idx;
	u32 msix_vector;
};

struct sss_tool_msg_head {
	u8 status;
	u8 rsvd1[3];
};

struct sss_tool_dcb_state {
	struct sss_tool_msg_head head;

	u16 op_code; /* 0 - get dcb state, 1 - set dcb state */
	u8 state;    /* 0 - disable,       1 - enable dcb  */
	u8 rsvd;
};

struct sss_tool_qos_dev_cfg {
	struct sss_tool_msg_head head;

	u8 op_code;       /* 0：get 1: set */
	u8 rsvd0;
	u16 cfg_bitmap;   /* bit0 - trust, bit1 - dft_cos, bit2 - pcp2cos, bit3 - dscp2cos */

	u8 trust;         /* 0 - pcp, 1 - dscp */
	u8 dft_cos;
	u16 rsvd1;
	u8 pcp2cos[8];    /* 必须8个一起配置 */

	/* 配置dscp2cos时，若cos值设置为0xFF*/
	/*驱动则忽略此dscp优先级的配置*/
	/*允许一次性配置多个dscp跟cos的映射关系 */
	u8 dscp2cos[64];
	u32 rsvd2[4];
};

struct sss_tool_qos_cos_cfg {
	struct sss_tool_msg_head head;

	u8 port_id;
	u8 func_cos_bitmap;
	u8 port_cos_bitmap;
	u8 func_max_cos_num;
	u32 rsvd2[4];
};

#endif /* SSS_TOOL_NIC_H */

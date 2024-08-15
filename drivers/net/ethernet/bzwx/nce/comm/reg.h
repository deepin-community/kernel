/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2024, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_COMM_REG_H
#define _NE6X_COMM_REG_H

#include <asm/types.h>

#define NE6X_BAR2_VP_TDQ(__vp, __reg) \
	((((__vp) & 0x7f) << 12) | (0 << 11) | (((__reg) & 0xff) << 3))
#define NE6X_BAR2_VP_RDQ(__vp, __reg) \
	((((__vp) & 0x7f) << 12) | (1 << 11) | (((__reg) & 0xff) << 3))

/* CIU */
#define NE6X_VP_BASE_ADDR                   0x0
#define NE6X_VPINT_DYN_CTLN(_VPID, _OFFSET) \
	(((_VPID) << 12) + ((_OFFSET) << 4)) /* _i=0...64 * Reset: PFR */
#define NE6X_PF_BASE_ADDR                   0x138ULL
#define NE6X_PFINT_DYN_CTLN(_PFID, _OFFSET)  \
	(((NE6X_PF_BASE_ADDR + (_PFID)) << 12) + ((_OFFSET) << 4))
	/* _i=0...7 */ /* Reset: PFR */

#define NE6X_VP_INT                     0x00
#define NE6X_VP_INT_SET                 0x01
#define NE6X_VP_INT_MASK                0x02
#define NE6X_VP_CQ_INTSHIFT             16
#define NE6X_CQ_BASE_ADDR               0x03
#define NE6X_CQ_HD_POINTER              0x04
#define NE6X_CQ_CFG                     0x05
#define NE6X_RQ_BASE_ADDR               0x07
#define NE6X_RQ_CFG                     0x08
#define NE6X_RQ_TAIL_POINTER            0x09
#define NE6X_VP_RELOAD                  0x0a
#define NE6X_SQ_BASE_ADDR               0x0b
#define NE6X_SQ_CFG                     0x0c
#define NE6X_SQ_TAIL_POINTER            0x0d
#define NE6X_CQ_TAIL_POINTER            0x11
#define NE6X_RQ_BUFF_OFST               0x12
#define NE6X_RQ_HD_POINTER              0x13
#define NE6X_SQ_BUFF_OFST               0x14
#define NE6X_SQ_HD_POINTER              0x15
#define NE6X_RQ_OFST                    0x16
#define NE6X_SQ_OFST                    0x17
#define NE6X_RQ_BLOCK_CFG               0x1b
#define NE6X_SQ_METER_CFG0              0x1c
#define NE6X_SQ_METER_CFG1              0x1d
#define NE6X_SQ_METER_CFG2              0x1e
#define NE6X_SQ_METER_CFG3              0x1f
#define NE6X_INT_CFG                    0x21
#define NE6X_CIU_TIME_OUT_CFG           0x45
#define NE6X_ALL_CQ_CFG                 0x46
#define NE6X_ALL_SQ_CFG                 0x47
#define NE6X_ALL_RQ_CFG                 0x48
#define NE6X_MERGE_CFG                  0x49
#define NE6X_BFD_RECV_CNT               0x4a
#define NE6X_ETH_RECV_CNT               0x4b

#define NE6X_PF_CON_ADDR(_OFST) \
	(((NE6X_PF_BASE_ADDR) << 12) + ((_OFST) << 4))
#define NE6X_PF_MAILBOX_DATA      0x40
#define NE6X_VF_MAILBOX_DATA      0x80
#define NE6X_PF_MAILBOX_ADDR(_VP) \
	(((NE6X_PF_BASE_ADDR) << 12) + ((NE6X_PF_MAILBOX_DATA + (_VP)) << 4))
#define NE6X_VF_MAILBOX_ADDR(_VP) \
	(((NE6X_PF_BASE_ADDR) << 12) + ((NE6X_VF_MAILBOX_DATA + (_VP)) << 4))
#define NE6X_PF_DB_INT_REQ              0xC0
#define NE6X_PF_DB_INT_ACK              0xC1
#define NE6X_PF_DB_DREQ_INT             0xC2
#define NE6X_PF_DB_DREQ_INT_SET         0xC3
#define NE6X_PF_DB_DREQ_INT_MASK        0xC4
#define NE6X_PF_DB_DACK_INT             0xC5
#define NE6X_PF_DB_DACK_INT_SET         0xC6
#define NE6X_PF_DB_DACK_INT_MASK        0xC7

union ne6x_vp_int {
	struct vp_int {
		u64 csr_ciu_int_vp : 64;
	} reg;
	u64 val;
};

union ne6x_vp_int_mask {
	struct vp_int_mask {
		u64 csr_ciu_mask_vp : 64;
	} reg;
	u64 val;
};

union ne6x_cq_base_addr {
	struct cq_base_addr {
		u64 csr_cq_base_addr_vp : 64;
	} reg;
	u64 val;
};

union ne6x_cq_cfg {
	struct cq_cfg {
		u64 csr_cq_len_vp        : 16;
		u64 csr_cq_merge_time_vp : 16;
		u64 csr_cq_merge_size_vp : 4;
		u64 rsv0                 : 28;
	} reg;
	u64 val;
};

union ne6x_rq_base_addr {
	struct rq_base_addr {
		u64 csr_rq_base_addr_vp : 64;
	} reg;
	u64 val;
};

union ne6x_rq_cfg {
	struct rq_cfg {
		u64 csr_rq_len_vp           : 16;
		u64 csr_rdq_pull_en         : 1;
		u64 csr_rqevt_write_back_vp : 1;
		u64 csr_recv_pd_type_vp     : 2;
		u64 csr_recv_pd_revers_en   : 1;
		u64 rsv0                    : 11;
		u64 rsv1                    : 32;
	} reg;
	u64 val;
};

union ne6x_sq_base_addr {
	struct sq_base_addr {
		u64 csr_sq_base_addr_vp : 64;
	} reg;
	u64 val;
};

union ne6x_sq_cfg {
	struct sq_cfg {
		u64 csr_sq_len_vp           : 16;
		u64 csr_tdq_pull_en         : 1;
		u64 csr_sqevt_write_back_vp : 1;
		u64 csr_send_pd_revers_en   : 1;
		u64 rsv0                    : 13;
		u64 rsv1                    : 32;
	} reg;
	u64 val;
};

union ne6x_rq_block_cfg {
	struct rq_block_cfg {
		u64 csr_rdq_mop_len : 16;
		u64 csr_rdq_sop_len : 16;
		u64 rsv0            : 32;
	} reg;
	u64 val;
};

union ne6x_sq_meter_cfg0 {
	struct sq_meter_cfg0 {
		u64 csr_meter_pkt_token_num_vp : 16;
		u64 csr_meter_ipg_len_vp       : 8;
		u64 csr_meter_refresh_en_vp    : 1;
		u64 csr_meter_rate_limit_en_vp : 1;
		u64 csr_meter_packet_mode_vp   : 1;
		u64 reserved                   : 37;
	} reg;
	u64 val;
};

union ne6x_sq_meter_cfg1 {
	struct sq_meter_cfg1 {
		u64 csr_meter_refresh_count_vp    : 28;
		u64 reserved                      : 4;
		u64 csr_meter_refresh_interval_vp : 32;
	} reg;
	u64 val;
};

union ne6x_sq_meter_cfg2 {
	struct sq_meter_cfg2 {
		u64 csr_meter_resume_threshold_vp : 32;
		u64 reserved                      : 32;
	} reg;
	u64 val;
};

union ne6x_sq_meter_cfg3 {
	struct sq_meter_cfg3 {
		u64 csr_meter_pause_threshold_vp : 32;
		u64 reserved                     : 32;
	} reg;
	u64 val;
};

union ne6x_int_cfg {
	struct int_cfg {
		u64 csr_sq_hdle_half_int_cnt_vp : 16;
		u64 csr_rq_hdle_half_int_cnt_vp : 16;
		u64 csr_cq_hdle_half_int_cnt_vp : 16;
		u64 rsv0                        : 16;
	} reg;
	u64 val;
};

union ne6x_ciu_time_out_cfg {
	struct ciu_time_out_cfg {
		u64 csr_int_timer_out_cnt : 12;
		u64 rsv0                  : 52;
	} reg;
	u64 val;
};

union ne6x_all_cq_cfg {
	struct all_cq_cfg {
		u64 csr_allcq_merge_size : 4;
		u64 rsv0                 : 4;
		u64 csr_allcq_wt_rr_cnt  : 7;
		u64 csr_allcq_wt_rr_flag : 1;
		u64 rsv1                 : 48;
	} reg;
	u64 val;
};

union ne6x_all_sq_cfg {
	struct all_sq_cfg {
		u64 csr_allsq_wb_trigger_info  : 8;
		u64 csr_allsq_csum_zero_negate : 1;
		u64 csr_allsq_pull_merge_cfg   : 5;
		u64 rsv0                       : 50;
	} reg;
	u64 val;
};

union ne6x_all_rq_cfg {
	struct all_rq_cfg {
		u64 csr_allrq_wb_trigger_info : 8;
		u64 csr_allrq_pull_merge_cfg  : 5;
		u64 rsv0                      : 51;
	} reg;
	u64 val;
};

union ne6x_merge_cfg {
	struct merge_cfg {
		u64 csr_merge_clk_cnt : 16;
		u64 rsv0              : 48;
	} reg;
	u64 val;
};

union ne6x_eth_recv_cnt {
	struct eth_recv_cnt {
		u64 csr_eth_pkt_drop_cnt : 32;
		u64 csr_eth_rdq_drop_cnt : 32;
	} reg;
	u64 val;
};

#endif

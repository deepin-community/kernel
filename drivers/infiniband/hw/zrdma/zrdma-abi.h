/* SPDX-License-Identifier: (GPL-2.0 WITH Linux-syscall-note) OR Linux-OpenIB) */
/*
 * Copyright (c) 2023 - 2024 ZTE Corporation.  All rights reserved.
 * Copyright (c) 2005 Topspin Communications.  All rights reserved.
 * Copyright (c) 2005 Cisco Systems.  All rights reserved.
 * Copyright (c) 2005 Open Grid Computing, Inc. All rights reserved.
 */

#ifndef ZRDMA_ABI_H
#define ZRDMA_ABI_H

#include <linux/types.h>

/* user-space whose last ABI ver is 5 */
#define ZXDH_ABI_VER 5
#define ZXDH_CONTEXT_VER_V1 5
#define ZXDH_CONTEXT_VER_V2 6
#define ZXDH_CONTEXT_VER_V3 7

enum zxdh_rdma_tool_flags {
	ZXDH_QP_EXTEND_OP = 1 << 0,
	ZXDH_CAPTURE = 1 << 1,
	ZXDH_GET_HW_DATA = 1 << 2,
	ZXDH_GET_HW_OBJECT_DATA = 1 << 3,
	ZXDH_CHECK_HW_HEALTH = 1 << 4,
	ZXDH_RDMA_TOOL_CFG_DEV_PARAM = 1 << 5,
	ZXDH_RDMA_TOOL_SHOW_RES_MAP = 1 << 5,
	ZXDH_RDMA_TOOL_READ_RAM = 1 << 6,
	ZXDH_RDMA_TOOL_DEVX_MODIFY_CQ = 1 << 7,
	ZXDH_RDMA_SET_CREDIT_CAP = 1 << 8,
	ZXDH_RDMA_SWITCH = 1 << 9,
	ZXDH_RDMA_TOOL_DEVX_EXT_MEM = 1 << 10,
	ZXDH_RDMA_TOOL_PRIV_EXT = 1 << 11,
	ZXDH_RDMA_TOOL_IOVA_CAP = 1 << 12,
};

enum zxdh_memreg_type {
	ZXDH_MEMREG_TYPE_MEM = 0,
	ZXDH_MEMREG_TYPE_QP = 1,
	ZXDH_MEMREG_TYPE_CQ = 2,
	ZXDH_MEMREG_TYPE_SRQ = 3,
	ZXDH_MEMREG_TYPE_EXTERNEL_MEM = 4,
};

enum zxdh_db_addr_type {
	ZXDH_DB_ADDR_PHY = 0,
	ZXDH_DB_ADDR_BAR = 1,
};

struct zxdh_alloc_ucontext_req {
	__u32 rsvd32;
	__u8 userspace_ver;
	__u8 rsvd8[3];
};

struct zxdh_alloc_ucontext_resp {
	__u32 max_pds;
	__u32 max_qps;
	__u32 wq_size; /* size of the WQs (SQ+RQ) in the mmaped area */
	__u8 kernel_ver;
	__u8 db_addr_type;
	__u16 rdma_tool_flags;
	__aligned_u64 feature_flags;
	__aligned_u64 sq_db_mmap_key;
	__aligned_u64 cq_db_mmap_key;
	__aligned_u64 sq_db_pa;
	__aligned_u64 cq_db_pa;
	__u32 max_hw_wq_frags;
	__u32 max_hw_read_sges;
	__u32 max_hw_inline;
	__u32 max_hw_rq_quanta;
	__u32 max_hw_srq_quanta;
	__u32 max_hw_wq_quanta;
	__u32 max_hw_srq_wr;
	__u32 min_hw_cq_size;
	__u32 max_hw_cq_size;
	__u16 max_hw_sq_chunk;
	__u8 hw_rev;
	__u8 chip_rev;
    __aligned_u64 srq_db_mmap_key;
	__aligned_u64 sq_db_bar_off;
	__aligned_u64 cq_db_bar_off;
	__aligned_u64 srq_db_bar_off;
	__u32 srq_db_mmap_size;
	__u16 rsv[6];
	__aligned_u64 ext_sq_db_mmap_key;
	__aligned_u64 l2d_addr_mmap_key;
	__u32 max_hw_rq_sges;
    __u8 rsvd1[3];
    __u8 is_qp_ext_mem_support;
};

struct zxdh_alloc_pd_resp {
	__u32 pd_id;
	__u8 rsvd[4];
};

struct zxdh_resize_cq_req {
	__aligned_u64 user_cq_buffer;
};

struct zxdh_create_cq_req {
	__aligned_u64 user_cq_buf;
	__aligned_u64 user_shadow_area;
	__aligned_u64 dev_cq_buf;
    __aligned_u64 dev_shadow_area;
};

struct zxdh_create_qp_req {
	__aligned_u64 user_wqe_bufs;
	__aligned_u64 user_compl_ctx;
	__aligned_u64 dev_wqe_bufs;
    __u8 rsvd[7];
    __u8 support_ext_qp;
	__u32 rq_offset;
    __u32 shadow_offset;
};

struct zxdh_mem_reg_req {
	__u32 reg_type; /* enum zxdh_memreg_type */
	__u32 cq_pages;
	__u32 rq_pages;
	__u32 sq_pages;
	__u32 srq_pages;
	__u16 srq_list_pages;
	__u8 rsvd[2];
};

struct zxdh_reg_mr_resp {
	__u32 mr_pa_low;
	__u32 mr_pa_hig;
	__u16 host_page_size;
	__u16 leaf_pbl_size;
	__u8 rsvd[4];
};

struct zxdh_modify_qp_req {
	__u8 sq_flush;
	__u8 rq_flush;
	__u8 rsvd[6];
};

struct zxdh_create_cq_resp {
	__u32 cq_id;
	__u32 cq_size;
};

struct zxdh_create_qp_resp {
	__u32 qp_id;
	__u32 actual_sq_size;
	__u32 actual_rq_size;
	__u32 zxdh_drv_opt;
	__u16 push_idx;
	__u8 lsmm;
	__u8 wqe_rate_limit_flag : 1;
	__u8 wqe_rate_limit_valid : 1;
	__u8 rsvd : 6;
	__u32 qp_caps;
    __u8 rsvd2[7];
    __u8 is_ext_qp;
    __aligned_u64 sq_offset;
    __aligned_u64 rq_offset;
};

struct zxdh_modify_qp_resp {
	__aligned_u64 push_wqe_mmap_key;
	__aligned_u64 push_db_mmap_key;
	__u16 push_offset;
	__u8 push_valid;
	__u8 rd_fence_rate;
	__u8 rsvd[4];
};

struct zxdh_create_ah_resp {
	__u32 ah_id;
	__u8 rsvd[4];
};
#endif /* ZXDH_ABI_H */

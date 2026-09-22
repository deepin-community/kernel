/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef RDMA_SRQ_H
#define RDMA_SRQ_H

#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/workqueue.h>
#include <rdma/ib_smi.h>
#include <rdma/ib_verbs.h>
#include <rdma/ib_pack.h>
#include <rdma/rdma_cm.h>
#include <rdma/iw_cm.h>
#include <rdma/ib_user_verbs.h>
#include <rdma/ib_umem.h>
#include <rdma/ib_cache.h>

#include "main.h"
#include "verbs.h"

//SRQC_FIELD_MASK
#define RDMA_SRQC_MASK_GENERAL_CFG (0x01UL << 0)
#define RDMA_SRQC_MASK_LIMIT_WATER_CFG (0x01UL << 1)
#define RDMA_SRQC_MASK_DEBUG_SET_CFG (0x01UL << 2)

#define ZXDH_CQPSQ_SRQ_ID_S 0
#define ZXDH_CQPSQ_SRQ_ID GENMASK_ULL(19, 0)
#define ZXDH_CQPSQ_SRQ_FILED_VALID_S 0
#define ZXDH_CQPSQ_SRQ_FILEDVALID GENMASK_ULL(31, 0)

#define ZXDH_CQPSQ_SRQ_STATE_S 62
#define ZXDH_CQPSQ_SRQ_STATE GENMASK_ULL(63, 62)
#define ZXDH_CQPSQ_SRQ_LEAFPBLSIZE_S 62
#define ZXDH_CQPSQ_SRQ_LEAFPBLSIZE GENMASK_ULL(61, 60)
#define ZXDH_CQPSQ_SRQ_LOGSRQSIZE_S 56
#define ZXDH_CQPSQ_SRQ_LOGSRQSIZE GENMASK_ULL(59, 56)
#define ZXDH_CQPSQ_SRQ_PD_INDEX_S 32
#define ZXDH_CQPSQ_SRQ_PD_INDEX GENMASK_ULL(51, 32)
#define ZXDH_CQPSQ_SRQ_WQSIG_S 31
#define ZXDH_CQPSQ_SRQ_WQSIG BIT_ULL(31)
#define ZXDH_CQPSQ_SRQ_CONTSRQ_S 30
#define ZXDH_CQPSQ_SRQ_CONTSRQ BIT_ULL(30)
#define ZXDH_CQPSQ_SRQ_SRQVIRTMAP_S 29
#define ZXDH_CQPSQ_SRQ_SRQVIRTMAP BIT_ULL(29)
#define ZXDH_CQPSQ_SRQ_REVERSEDLKEY_S 28
#define ZXDH_CQPSQ_SRQ_REVERSEDLKEY BIT_ULL(28)
#define ZXDH_CQPSQ_SRQ_SRQAXIERRSIG_S 27
#define ZXDH_CQPSQ_SRQ_SRQAXIERRSIG BIT_ULL(27)
#define ZXDH_CQPSQ_SRQ_LOGSRQSTRIDE_S 24
#define ZXDH_CQPSQ_SRQ_LOGSRQSTRIDE GENMASK_ULL(26, 24)
#define ZXDH_CQPSQ_SRQ_LIST_LEAFPBLSIZE_S 22
#define ZXDH_CQPSQ_SRQ_LIST_LEAFPBLSIZE GENMASK_ULL(23, 22)
#define ZXDH_CQPSQ_SRQ_LISTVIRTMAP_S 21
#define ZXDH_CQPSQ_SRQ_LISTVIRTMAP BIT_ULL(21)
#define ZXDH_CQPSQ_SRQ_SWWQECNT_S 0
#define ZXDH_CQPSQ_SRQ_SWWQECNT GENMASK_ULL(15, 0)

#define ZXDH_CQPSQ_SRQ_VALIDWQEINDEXPOINT_S 42
#define ZXDH_CQPSQ_SRQ_VALIDWQEINDEXPOINT GENMASK_ULL(43, 42)
#define ZXDH_CQPSQ_SRQ_DEBUGSET_S 32
#define ZXDH_CQPSQ_SRQ_DEBUGSET GENMASK_ULL(41, 32)
#define ZXDH_CQPSQ_SRQ_LIMITWATERMARK_S 16
#define ZXDH_CQPSQ_SRQ_LIMITWATERMARK GENMASK_ULL(31, 16)
#define ZXDH_CQPSQ_SRQ_HWWQECNT_S 0
#define ZXDH_CQPSQ_SRQ_HWWQECNT GENMASK_ULL(15, 0)

#define ZXDH_SRQ_PARITY_SIGN_S 15
#define ZXDH_SRQ_PARITY_SIGN BIT_ULL(15)
#define ZXDH_SRQ_SW_SRQ_HEAD_S 0
#define ZXDH_SRQ_SW_SRQ_HEAD GENMASK_ULL(14, 0)

#define ZXDH_SRQ_DB_CACHE_ID_S 0
#define ZXDH_SRQ_DB_CACHE_ID GENMASK_ULL(1, 0)
#define ZXDH_SRQ_DB_INDICATE_ID_S 2
#define ZXDH_SRQ_DB_INDICATE_ID GENMASK_ULL(3, 2)
#define ZXDH_SRQ_DB_AXI_ID_S 4
#define ZXDH_SRQ_DB_AXI_ID GENMASK_ULL(6, 4)
#define ZXDH_SRQ_DB_WAY_PATION_S 7
#define ZXDH_SRQ_DB_WAY_PATION GENMASK_ULL(9, 7)

#define ZXDH_SRQ_SRQL_CACHE_ID_S 0
#define ZXDH_SRQ_DSRQL_CACHE_ID GENMASK_ULL(1, 0)
#define ZXDH_SRQ_SRQL_INDICATE_ID_S 2
#define ZXDH_SRQ_SRQL_INDICATE_ID GENMASK_ULL(3, 2)
#define ZXDH_SRQ_SRQL_AXI_ID_S 4
#define ZXDH_SRQ_SRQL_AXI_ID GENMASK_ULL(6, 4)
#define ZXDH_SRQ_SRQL_WAY_PATION_S 7
#define ZXDH_SRQ_SRQL_WAY_PATION GENMASK_ULL(9, 7)

#define ZXDH_SRQ_SW_MIN_WQSIZE 32u /* in WRs*/
#define ZXDH_SRQ_WQE_MIN_SIZE 16
#define ZXDH_SRQ_WQE_MAX_SIZE 512
#define ZXDH_SRQ_FRAG_BYTESIZE 16
#define ZXDH_SRQ_WQE_BYTESIZE 32

#define ZXDH_SRQE_SIZE 2

#define ZXDH_SRQ_STATE_ERROR 0
#define ZXDH_SRQ_STATE_GOOD 1

#define ZXDH_SRQ_WQE_NOT_CONT 0
#define ZXDH_SRQ_WQE_CONT 1

#define ZXDH_SRQ_WQE_MIN_LEN_32_BYTE 1
#define ZXDH_SRQ_WQE_MIN_LEN_64_BYTE 2

#define ZXDH_SRQ_INVALID_LKEY 0x100
#define ZXDH_SRQ_DB_INIT_VALUE 0x8000

struct zxdh_wqe_srq_next_sge {
	__le16 next_wqe_index;
	__le16 signature;
	u8 valid_sge_num;
	u8 rsvd[11];
};

struct zxdh_srq_sge {
	__le64 addr;
	__le32 length;
	__le32 lkey;
};

struct zxdh_srq_wqe {
	__le64 elem[ZXDH_SRQE_SIZE];
};

struct zxdh_srq_uk {
	struct zxdh_srq_wqe *srq_base;
	struct zxdh_uk_attrs *uk_attrs;
	__le16 *srq_list_base;
	__le64 *srq_db_base;
	u32 srq_id;
	u32 srq_size;
	u32 log2_srq_size;
	u32 srq_list_size;
	struct zxdh_ring srq_ring;
	struct zxdh_ring srq_list_ring;
	u8 srq_list_polarity;
	u64 *srq_wrid_array;
	u8 srq_wqe_size;
	u8 srq_wqe_size_multiplier;
	u32 srq_caps;
	u32 max_srq_frag_cnt;
	u32 srq_type;
	spinlock_t *lock;
	u8 srq_flush_complete : 1; /* Indicates flush was seen and SRQ was empty after the flush */
	u8 destroy_pending : 1; /* Indicates the SRQ is being destroyed */
	u8 srq_flush_seen;
};

struct zxdh_sc_srq {
	struct zxdh_srq_uk srq_uk;
	struct zxdh_sc_dev *dev;
	struct zxdh_sc_pd *pd;
	u64 srq_pa;
	u64 srq_list_pa;
	u64 srq_db_pa;
	u32 srq_limit;
	u64 srq_compl_ctx;
	void *back_srq;
	u8 srq_state;
	u8 hw_srq_size;
	u8 flush_srq;
	u8 virtual_map : 1;
	u8 list_virtual_map : 1;
	u8 pbl_chunk_size;
	u32 first_pm_pbl_idx;
	u8 list_pbl_chunk_size;
	u32 list_first_pm_pbl_idx;
};

struct zxdh_srq {
	struct ib_srq ibsrq;
	struct zxdh_sc_srq sc_srq;
	struct zxdh_device *iwdev;
	spinlock_t lock; /* serialize posting WRs to SQ/RQ */

	u32 max_wr;
	u32 max_sge;
	u32 srq_limit;
	refcount_t refcnt;
	struct ib_umem *umem;
	int wq_sig;
	u8 state;
	u8 user_mode;
	enum ib_srq_type srq_type;
	struct zxdh_dma_mem kmem;
	struct zxdh_dma_mem kmem_list;
	struct zxdh_dma_mem kmem_db;
	struct zxdh_pbl *iwpbl;
	struct completion free_srq;
	int limit;
	struct zxdh_srq_kmode ksrq;

	u64 srq_compl_ctx;
};

struct zxdh_srq_attr {
	u32 type;
	u32 flags;
	u32 log_size;
	u32 wqe_shift;
	u32 log_page_size;
	u32 wqe_cnt;
	u32 srqn;
	u32 page_offset;
	u32 user_index;
	struct ib_umem *umem;
};

struct zxdh_srq_uk_init_info {
	struct zxdh_srq_wqe *srq_base;
	struct zxdh_uk_attrs *uk_attrs;
	__le16 *srq_list_base;
	__le64 *srq_db_base;
	u64 *srq_wrid_array;
	u32 srq_id;
	u32 srq_caps;
	u32 srq_size;
	u32 log2_srq_size;
	u32 srq_list_size;
	u32 max_srq_frag_cnt;
	u32 srq_limit;
};

struct zxdh_srq_init_info {
	struct zxdh_srq_uk_init_info srq_uk_init_info;
	struct zxdh_sc_dev *dev;
	struct zxdh_sc_pd *pd;
	u8 virtual_map : 1;
	u8 list_virtual_map : 1;
	u8 db_virtual_map : 1;
	u64 srq_pa;
	u8 pbl_chunk_size;
	u32 first_pm_pbl_idx;
	u8 list_pbl_chunk_size;
	u32 list_first_pm_pbl_idx;
	u8 db_pbl_chunk_size;
	u32 db_first_pm_pbl_idx;
	u64 srq_list_pa;
	u64 srq_db_pa;
	u32 srq_limit;
};

struct zxdh_create_srq_req {
	__aligned_u64 user_wqe_bufs;
	__aligned_u64 user_compl_ctx;
	__aligned_u64 user_wqe_list;
	__aligned_u64 user_wqe_db;
};

struct zxdh_create_srq_resp {
	__u32 srq_id;
	__u32 srq_size;
	__u32 srq_list_size;
    __u32 base_srqn;
};

typedef enum {
    USER_DDR_KERNEL_DDR,
    USER_DDR_KERNEL_L2D,
    USER_L2D_KERNEL_L2D,
    USER_L2D_KERNEL_DDR,
    USER_L2D_KERNEL_L2D_V3,
} zxdh_rdma_srq_mem_type;

static inline struct zxdh_srq *to_iwsrq(struct ib_srq *ibsrq)
{
	return container_of(ibsrq, struct zxdh_srq, ibsrq);
}

void zxdh_free_srq_wqe(struct zxdh_srq_uk *srq, int wqe_index);
#ifdef ZRDMA_CREATE_SRQ_VER_2
int zxdh_create_srq(struct ib_srq *ibsrq, struct ib_srq_init_attr *init_attr,
		    struct ib_udata *udata);
#else
struct ib_srq *zxdh_create_srq(struct ib_pd *ibpd,
			       struct ib_srq_init_attr *init_attr,
			       struct ib_udata *udata);
#endif
void zxdh_srq_add_ref(struct ib_srq *ibsrq);
void zxdh_srq_rem_ref(struct ib_srq *ibsrq);
#ifdef ZRDMA_DESTROY_SRQ_VER_3
int zxdh_destroy_srq(struct ib_srq *ibsrq, struct ib_udata *udata);
#endif

#ifdef ZRDMA_DESTROY_SRQ_VER_2
void zxdh_destroy_srq(struct ib_srq *ibsrq, struct ib_udata *udata);
#endif

#ifdef ZRDMA_DESTROY_SRQ_VER_1
int zxdh_destroy_srq(struct ib_srq *ibsrq);
#endif
int zxdh_modify_srq(struct ib_srq *ibsrq, struct ib_srq_attr *attr,
		    enum ib_srq_attr_mask attr_mask, struct ib_udata *udata);
int zxdh_query_srq(struct ib_srq *ibsrq, struct ib_srq_attr *srq_attr);
int zxdh_post_srq_recv(struct ib_srq *ibsrq, const struct ib_recv_wr *ib_wr,
		       const struct ib_recv_wr **bad_wr);
int zxdh_sc_srq_create(struct zxdh_sc_srq *srq,
		       struct zxdh_create_srq_info *info, u64 scratch,
		       bool post_sq);
int zxdh_sc_srq_modify(struct zxdh_sc_srq *srq,
		       struct zxdh_modify_srq_info *info, u64 scratch,
		       bool post_sq);
int zxdh_sc_srq_destroy(struct zxdh_sc_srq *srq, u64 scratch, bool post_sq);

int zxdh_query_srqc(struct zxdh_sc_srq *srq, u32 *limit);
int zxdh_sc_query_srqc(struct zxdh_sc_dev *dev, u32 srqn, u64 srqc_buf_pa, u64 scratch,
		       bool post_sq);

#endif

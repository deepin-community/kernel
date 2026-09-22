/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */
#ifndef ZXDH_HMC_H
#define ZXDH_HMC_H

#include "defs.h"
#include "pble.h"
#include "./smmu/kernel/adk_mmu600.h"

#define ZXDH_HMC_MAX_BP_COUNT 512
#define ZXDH_MAX_SD_ENTRIES 11
#define ZXDH_HW_DBG_HMC_INVALID_BP_MARK 0xca
#define ZXDH_HMC_INFO_SIGNATURE 0x484d5347
#define ZXDH_HMC_PD_CNT_IN_SD 512
#define ZXDH_HMC_DIRECT_BP_SIZE 0x200000
#define ZXDH_HMC_MAX_SD_COUNT 8192
#define ZXDH_HMC_PAGED_BP_SIZE 4096
#define ZXDH_HMC_PD_BP_BUF_ALIGNMENT 4096
#define ZXDH_FIRST_VF_FPM_ID 8
#define FPM_MULTIPLIER 1024

#define ZXDH_MIN_GLOBAL_CQPN 2
#define ZXDH_MAX_GLOBAL_CQPN 1025
#define ZXDH_MIN_GLOBAL_QPN (1 + ZXDH_MAX_GLOBAL_CQPN)
#define ZXDH_MIN_GLOBAL_CQN 1
#define ZXDH_MIN_GLOBAL_SRQN 1
#define ZXDH_MIN_GLOBAL_CEQN 1
#define ZXDH_TABLE5_VF_EN   0x04

//修改为结构体
// #define ZXDH_HMC_CNT_DEBUG                        64
// #define ZXDH_HMC_1K                               1024
// #define ZXDH_HMC_1M                               (1024*ZXDH_HMC_1K/ZXDH_HMC_CNT_DEBUG)
// #define ZXDH_HMC_HOST_QPC_MAX_QUANTITY            (ZXDH_HMC_1M/ZXDH_HMC_CNT_DEBUG)
// #define ZXDH_HMC_HOST_CQC_MAX_QUANTITY            2*ZXDH_HMC_1M/ZXDH_HMC_CNT_DEBUG
// #define ZXDH_HMC_HOST_SRQC_MAX_QUANTITY           512*ZXDH_HMC_1K/ZXDH_HMC_CNT_DEBUG
// #define ZXDH_HMC_HOST_MRTE_MAX_QUANTITY           16*ZXDH_HMC_1M/ZXDH_HMC_CNT_DEBUG
// #define ZXDH_HMC_HOST_AH_MAX_QUANTITY             512*ZXDH_HMC_1K/ZXDH_HMC_CNT_DEBUG
// #define ZXDH_HMC_HOST_MGCPAYLOAD_MAX_QUANTITY     2   //There is no multicast scenario. The number can be set to a small value.
// #define ZXDH_HMC_HOST_MGC_MAX_QUANTITY            8*ZXDH_HMC_1K/ZXDH_HMC_CNT_DEBUG
// #define ZXDH_HMC_HOST_PBLEMR_MAX_QUANTITY         512*ZXDH_HMC_1M/ZXDH_HMC_CNT_DEBUG
// #define ZXDH_HMC_HOST_PBLEOTHER_MAX_QUANTITY      512*ZXDH_HMC_1M/ZXDH_HMC_CNT_DEBUG
// #define ZXDH_HMC_HOST_CEQC_MAX_QUANTITY		   4

// smmu使用参数
#define ZXDH_HMC_CNT_DEBUG 64
#define ZXDH_HMC_1K 1024
#define ZXDH_HMC_1M (1024 * ZXDH_HMC_1K / ZXDH_HMC_CNT_DEBUG)
#define ZXDH_HMC_HOST_QPC_MAX_QUANTITY (1024 * 8)
#define ZXDH_HMC_HOST_CQC_MAX_QUANTITY (1024 * 8)
#define ZXDH_HMC_HOST_SRQC_MAX_QUANTITY (1024 * 8)
#define ZXDH_HMC_HOST_MRTE_MAX_QUANTITY (1024 * 32)
#define ZXDH_HMC_HOST_AH_MAX_QUANTITY (1024 * 32)
// #define ZXDH_HMC_HOST_MGCPAYLOAD_MAX_QUANTITY     600
// #define ZXDH_HMC_HOST_MGC_MAX_QUANTITY            (8192)
#define ZXDH_HMC_HOST_PBLEMR_MAX_QUANTITY (1024 * 1024 * 4)
#define ZXDH_HMC_HOST_PBLEOTHER_MAX_QUANTITY (1024 * 1024 * 4)
#define ZXDH_HMC_HOST_CEQC_MAX_QUANTITY 4
#define ZXDH_AEQ_ERRORCODE_MAP_LEN 17
#define ZXDH_AEQ_ERRORINFO_MAX_LEN 128
#define ZXDH_EXT_MEM_EP_ID 0

enum zxdh_hmc_rsrc_type {
	ZXDH_HMC_IW_QP = 0,
	ZXDH_HMC_IW_CQ = 1,
	ZXDH_HMC_IW_SRQ = 2,
	ZXDH_HMC_IW_AH = 3,
	ZXDH_HMC_IW_MR = 4,
	ZXDH_HMC_IW_IRD = 5,
	ZXDH_HMC_IW_TXWINDOW = 6,
	ZXDH_HMC_IW_PBLE = 7,
	ZXDH_HMC_IW_PBLE_MR = 8,
	ZXDH_HMC_IW_MAX, /* Must be last entry */
};

enum zxdh_indicate_id {
	ZXDH_INDICATE_L2D = 0,
	ZXDH_INDICATE_DPU_DDR = ZXDH_INDICATE_L2D, // 外挂
	ZXDH_INDICATE_REGISTER = ZXDH_INDICATE_L2D,
	ZXDH_INDICATE_RESERVED = 1,
	ZXDH_INDICATE_HOST_NOSMMU = 2,
	ZXDH_INDICATE_HOST_SMMU = 3,
};

enum zxdh_axid_type {
	ZXDH_AXID_L2D = 0,
	ZXDH_AXID_DPUDDR = 1,
	ZXDH_AXID_HOST_EP0 = 2,
	ZXDH_AXID_HOST_EP1 = 3,
	ZXDH_AXID_HOST_EP2 = 4,
	ZXDH_AXID_HOST_EP3 = 5,
	ZXDH_AXID_HOST_EP4 = 6,
};

enum zxdh_interface_type {
	ZXDH_INTERFACE_CACHE = 0,
	ZXDH_INTERFACE_NOTCACHE = 1,
};

enum zxdh_object_id {
	ZXDH_PBLE_MR_OBJ_ID = 0,
	ZXDH_PBLE_QUEUE_OBJ_ID = 1,
	ZXDH_MR_OBJ_ID = 2,
	ZXDH_AH_OBJ_ID = 3,
	ZXDH_IRD_OBJ_ID = 4,
	ZXDH_TX_WINDOW_OBJ_ID = 5,
	ZXDH_SRQC_OBJ_ID = 6,
	ZXDH_CQC_OBJ_ID = 7,
	ZXDH_MG_PAYLOAD_OBJ_ID = 8,
	ZXDH_MG_OBJ_ID = 9,
	ZXDH_RW_PAYLOAD = 10,
	ZXDH_SQ = 11,
	ZXDH_SQ_SHADOW_AREA = 12,
	ZXDH_RQ = 13,
	ZXDH_RQ_SHADOW_AREA = 14,
	ZXDH_SRQP = 15,
	ZXDH_SRQ = 16,
	ZXDH_SRQ_SHADOW_AREA = 17,
	ZXDH_CQ = 18,
	ZXDH_CQ_SHADOW_AREA = 19,
	ZXDH_CEQ = 20,
	ZXDH_AEQ = 21,
	ZXDH_MG_QPN = 22,
	ZXDH_CPU_DDR = 24,
	ZXDH_QPC_OBJ_ID = 29,
	ZXDH_DMA_OBJ_ID = 30,
	ZXDH_L2D_OBJ_ID = 31,
	ZXDH_REG_OBJ_ID = ZXDH_L2D_OBJ_ID,
	//USER DEFINED start
	ZXDH_SQC_OBJ_ID = 200,
};

enum zxdh_sd_entry_type {
	ZXDH_SD_TYPE_INVALID = 0,
	ZXDH_SD_TYPE_PAGED = 1,
	ZXDH_SD_TYPE_DIRECT = 2,
};

enum zxdh_mb_opt_type {
	ZTE_ZXDH_VCHNL_OP_GET_HMC_FCN = 1,
	ZTE_ZXDH_OP_REQ_NP_CONFIG = 2,
	ZTE_ZXDH_OP_DEL_HMC_OBJ_RANGE = 3,
	ZTE_ZXDH_OP_REQ_NP_MAC_DEL = 4,
	ZTE_ZXDH_OP_REQ_NP_MAC_ADD = 5,
	ZTE_ZXDH_OP_GET_PBLE_HMC_BASEINFO = 6,
	ZTE_ZXDH_OP_REPLY_PBLE_HMC_BASEINFO = 7,
	ZTE_ZXDH_OP_ADD_QPBLE_HMC_RANGE = 8,
	ZTE_ZXDH_OP_ADD_MRPBLE_HMC_RANGE = 9,
	ZTE_ZXDH_OP_SET_SMMU_INVALID = 10,
	ZTE_ZXDH_OP_DEL_NP_CONFIG = 11,
	ZTE_ZXDH_OP_DEVICE_LOAD = 12,
	ZTE_ZXDH_OP_DEVICE_UNLOAD = 13,
	ZTE_ZXDH_OP_PF_UNLOAD = 14,
	ZTE_ZXDH_OP_CONFIG_TRUST_TYPE = 15,
	ZTE_ZXDH_OP_MAX_NUM,
};

enum function_type {
	FUNCTION_TYPE_PF = 0,
	FUNCTION_TYPE_VF = 1,
};

struct zxdh_hmc_obj_manage {
	u64 hmc_base;
	u64 hmc_size;
	u32 total_qp_cnt;
	u32 total_cq_cnt;
	u32 total_srq_cnt;
	u32 pf_pblemr_cnt;
	u32 pf_pblequeue_cnt;
	u32 vf_qp_cnt;
	u32 vf_mr_cnt;
	u32 vf_ah_cnt;
	u32 vf_pblemr_cnt;
	u32 vf_pblequeue_cnt;
};

struct zxdh_hmc_obj_info {
	u64 base;
	u32 max_cnt;
	u32 cnt;
	u64 size;
	u8 type;
};

struct zxdh_vf_hmc_obj_info {
	struct zxdh_hmc_obj_info hmc_objinfo[ZXDH_HMC_IW_MAX];
	u16 vf_id;
	u8 valid : 1;
};

struct zxdh_hmc_bp {
	enum zxdh_sd_entry_type entry_type;
	struct zxdh_dma_mem addr;
	struct zxdh_dma_mem addr_hardware; // for hardware
	u32 sd_pd_index;
	u32 use_cnt;
};

struct zxdh_hmc_pd_entry {
	struct zxdh_hmc_bp bp;
	u32 sd_index;
	u8 rsrc_pg : 1;
	u8 valid : 1;
};

struct zxdh_hmc_pd_table {
	struct zxdh_dma_mem pd_page_addr;
	struct zxdh_hmc_pd_entry *pd_entry;
	struct zxdh_virt_mem pd_entry_virt_mem;
	u32 use_cnt;
	u32 sd_index;
};

struct zxdh_hmc_sd_entry {
	enum zxdh_sd_entry_type entry_type;
	bool valid;
	union {
		struct zxdh_hmc_pd_table pd_table;
		struct zxdh_hmc_bp bp;
	} u;
};

struct zxdh_hmc_sd_table {
	struct zxdh_virt_mem addr;
	u32 sd_cnt;
	u32 use_cnt;
	struct zxdh_hmc_sd_entry *sd_entry;
};

struct zxdh_hmc_info {
	u32 signature;
	u8 hmc_fn_id;
	u16 first_sd_index;
	u32 pble_hmc_index;
	u32 pble_mr_hmc_index;
	u32 hmc_entry_total;
	u32 hmc_first_entry_pble;
	u32 hmc_first_entry_pble_mr;
	struct zxdh_hmc_obj_info *hmc_obj;
	struct zxdh_virt_mem hmc_obj_virt_mem;
	struct zxdh_hmc_sd_table sd_table;
	u16 sd_indexes[ZXDH_HMC_MAX_SD_COUNT];
	u8 pble_mr_cachid;
	u8 pble_ird_cachid;
};

struct zxdh_update_sd_entry {
	u64 cmd;
	u64 data;
};

struct zxdh_update_sds_info {
	u32 cnt;
	u8 hmc_fn_id;
	struct zxdh_update_sd_entry entry[ZXDH_MAX_SD_ENTRIES];
};

struct zxdh_ccq_cqe_info;
struct zxdh_hmc_fcn_info {
	u32 vf_id;
	u8 free_fcn;
};

struct zxdh_hmc_create_obj_info {
	struct zxdh_hmc_info *hmc_info;
	struct zxdh_virt_mem add_sd_virt_mem;
	u32 rsrc_type;
	u32 count;
	u32 add_sd_cnt;
	enum zxdh_sd_entry_type entry_type;
	bool privileged;
};

struct zxdh_hmc_del_obj_info {
	struct zxdh_hmc_info *hmc_info;
	struct zxdh_virt_mem del_sd_virt_mem;
	u32 rsrc_type;
	u32 count;
	u32 del_sd_cnt;
	bool privileged;
};

struct zxdh_aeq_cqp_errorcode_tbl {
	u32 error_code;
	unsigned char *error_info;
};

struct zxdh_errorcode_map_tbl {
    u32 error_code;
    const char *error_info;
    u32 used_flag;
    u32 used_list_flag;
    struct zxdh_errorcode_map_tbl *pnext;
};

int zxdh_sc_create_hmc_obj(struct zxdh_sc_dev *dev,
			   struct zxdh_hmc_create_obj_info *info);
int zxdh_sc_create_date_cap_obj(struct zxdh_sc_dev *dev);
int zxdh_add_pble_hmc_obj(struct zxdh_hmc_info *hmc_info,
			  struct zxdh_hmc_pble_rsrc *pble_rsrc, u32 pages);
int zxdh_vf_add_pble_hmc_obj(struct zxdh_sc_dev *dev,
			     struct zxdh_hmc_info *hmc_info,
			     struct zxdh_hmc_pble_rsrc *pble_rsrc, u32 pages);

int zxdh_prep_remove_sd_bp(struct zxdh_hmc_info *hmc_info, u32 idx);

int zxdh_recv_mb(struct zxdh_sc_dev *dev, struct zxdh_ccq_cqe_info *info);
int zxdh_recv_vf_mb(struct zxdh_sc_dev *dev, struct zxdh_ccq_cqe_info *info);

struct zxdh_vfdev *zxdh_pf_get_vf_hmc_res(struct zxdh_sc_dev *dev, u16 vf_id);
int zxdh_sc_write_hmc_register(struct zxdh_sc_dev *dev,
			       struct zxdh_hmc_obj_info *obj_info,
			       u32 rsrc_type, u16 vhca_id);
int zxdh_ext_sc_write_hmc_register(struct zxdh_sc_dev *ext_dev,
			       struct zxdh_hmc_obj_info *obj_info,
			       u32 rsrc_type, u16 vhca_id);
int zxdh_vfhmc_enter(struct zxdh_sc_dev *dev,
		     struct zxdh_hmc_info *vf_hmc_info);

int zxdh_create_vf_hmc_objs(struct zxdh_sc_dev *dev,
			    struct zxdh_hmc_info *hmc_info, u8 type,
			    struct zxdh_hmc_create_obj_info *obj_info);

char *zxdh_get_aeq_info(u32 error_code);
#endif /* ZXDH_HMC_H */

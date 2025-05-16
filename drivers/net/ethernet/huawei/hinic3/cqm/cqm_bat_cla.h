/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CQM_BAT_CLA_H
#define CQM_BAT_CLA_H

#include <linux/types.h>
#include <linux/mutex.h>

#include "cqm_bitmap_table.h"
#include "cqm_object.h"
#include "vram_common.h"

/* When the connection check is enabled, the maximum number of connections
 * supported by the chip is 1M - 63, which cannot reach 1M
 */
#define CQM_BAT_MAX_CONN_NUM       (0x100000 - 63)
#define CQM_BAT_MAX_CACHE_CONN_NUM (0x100000 - 63)

#define CLA_TABLE_PAGE_ORDER       0
#define CQM_4K_PAGE_ORDER          0
#define CQM_4K_PAGE_SIZE           4096
#define CQM_8K_PAGE_ORDER          1

#define CQM_BAT_ENTRY_MAX          16
#define CQM_BAT_ENTRY_SIZE         16
#define CQM_BAT_STORE_API_SIZE     16

#define CQM_BAT_SIZE_FT_RDMA_PF    240
#define CQM_BAT_SIZE_FT_RDMA_VF    160
#define CQM_BAT_SIZE_FT_PF         192
#define CQM_BAT_SIZE_FT_VF         112
#define CQM_BAT_SIZE_RDMA_PF       160
#define CQM_BAT_SIZE_RDMA_VF       80

#define CQM_BAT_INDEX0             0
#define CQM_BAT_INDEX1             1
#define CQM_BAT_INDEX2             2
#define CQM_BAT_INDEX3             3
#define CQM_BAT_INDEX4             4
#define CQM_BAT_INDEX5             5
#define CQM_BAT_INDEX6             6
#define CQM_BAT_INDEX7             7
#define CQM_BAT_INDEX8             8
#define CQM_BAT_INDEX9             9
#define CQM_BAT_INDEX10            10
#define CQM_BAT_INDEX11            11
#define CQM_BAT_INDEX12            12
#define CQM_BAT_INDEX13            13
#define CQM_BAT_INDEX14            14
#define CQM_BAT_INDEX15            15

enum cqm_bat_entry_type {
	CQM_BAT_ENTRY_T_CFG = 0,
	CQM_BAT_ENTRY_T_HASH = 1,
	CQM_BAT_ENTRY_T_QPC = 2,
	CQM_BAT_ENTRY_T_SCQC = 3,
	CQM_BAT_ENTRY_T_SRQC = 4,
	CQM_BAT_ENTRY_T_MPT = 5,
	CQM_BAT_ENTRY_T_GID = 6,
	CQM_BAT_ENTRY_T_LUN = 7,
	CQM_BAT_ENTRY_T_TASKMAP = 8,
	CQM_BAT_ENTRY_T_L3I = 9,
	CQM_BAT_ENTRY_T_CHILDC = 10,
	CQM_BAT_ENTRY_T_TIMER = 11,
	CQM_BAT_ENTRY_T_XID2CID = 12,
	CQM_BAT_ENTRY_T_REORDER = 13,
	CQM_BAT_ENTRY_T_INVALID = 14,
	CQM_BAT_ENTRY_T_MAX = 15,
};

/* CLA update mode */
#define CQM_CLA_RECORD_NEW_GPA                0
#define CQM_CLA_DEL_GPA_WITHOUT_CACHE_INVALID 1
#define CQM_CLA_DEL_GPA_WITH_CACHE_INVALID    2

#define CQM_CLA_LVL_0                         0
#define CQM_CLA_LVL_1                         1
#define CQM_CLA_LVL_2                         2

#define CQM_MAX_INDEX_BIT                     19

#define CQM_CHIP_CACHELINE                    256
#define CQM_CHIP_TIMER_CACHELINE              512
#define CQM_OBJECT_256                        256
#define CQM_OBJECT_512                        512
#define CQM_OBJECT_1024                       1024
#define CQM_CHIP_GPA_MASK                     0x1ffffffffffffff
#define CQM_CHIP_GPA_HIMASK                   0x1ffffff
#define CQM_CHIP_GPA_LOMASK                   0xffffffff
#define CQM_CHIP_GPA_HSHIFT                   32

/* Aligns with 64 buckets and shifts rightward by 6 bits */
#define CQM_HASH_NUMBER_UNIT                  6

struct tag_cqm_cla_table {
	u32 type;
	u32 max_buffer_size;
	u32 obj_num;
	bool alloc_static; /* Whether the buffer is statically allocated */
	u32 cla_lvl;
	u32 cacheline_x; /* x value calculated based on cacheline,
			  * used by the chip
			  */
	u32 cacheline_y; /* y value calculated based on cacheline,
			  * used by the chip
			  */
	u32 cacheline_z; /* z value calculated based on cacheline,
			  * used by the chip
			  */
	u32 x; /* x value calculated based on obj_size, used by software */
	u32 y; /* y value calculated based on obj_size, used by software */
	u32 z; /* z value calculated based on obj_size, used by software */
	struct tag_cqm_buf cla_x_buf;
	struct tag_cqm_buf cla_y_buf;
	struct tag_cqm_buf cla_z_buf;
	u32 trunk_order; /* A continuous physical page contains 2^order pages */
	u32 obj_size;
	struct mutex lock; /* Lock for cla buffer allocation and free */

	struct tag_cqm_bitmap bitmap;

	struct tag_cqm_object_table obj_table; /* Mapping table between
						* indexes and objects
						*/
	char name[VRAM_NAME_APPLY_LEN];
};

struct tag_cqm_bat_entry_cfg {
	u32 cur_conn_num_h_4 : 4;
	u32 rsv1 : 4;
	u32 max_conn_num : 20;
	u32 rsv2 : 4;

	u32 max_conn_cache : 10;
	u32 rsv3 : 6;
	u32 cur_conn_num_l_16 : 16;

	u32 bloom_filter_addr : 16;
	u32 cur_conn_cache : 10;
	u32 rsv4 : 6;

	u32 bucket_num : 16;
	u32 bloom_filter_len : 16;
};

#define CQM_BAT_NO_BYPASS_CACHE 0
#define CQM_BAT_BYPASS_CACHE    1

#define CQM_BAT_ENTRY_SIZE_256  0
#define CQM_BAT_ENTRY_SIZE_512  1
#define CQM_BAT_ENTRY_SIZE_1024 2

struct tag_cqm_bat_entry_standerd {
	u32 entry_size : 2;
	u32 rsv1 : 6;
	u32 max_number : 20;
	u32 rsv2 : 4;

	u32 cla_gpa_h : 32;

	u32 cla_gpa_l : 32;

	u32 rsv3 : 8;
	u32 z : 5;
	u32 y : 5;
	u32 x : 5;
	u32 rsv24 : 1;
	u32 bypass : 1;
	u32 cla_level : 2;
	u32 rsv5 : 5;
};

struct tag_cqm_bat_entry_vf2pf {
	u32 cla_gpa_h : 25;
	u32 pf_id : 5;
	u32 fake_vf_en : 1;
	u32 acs_spu_en : 1;
};

#define CQM_BAT_ENTRY_TASKMAP_NUM 4
struct tag_cqm_bat_entry_taskmap_addr {
	u32 gpa_h;
	u32 gpa_l;
};

struct tag_cqm_bat_entry_taskmap {
	struct tag_cqm_bat_entry_taskmap_addr addr[CQM_BAT_ENTRY_TASKMAP_NUM];
};

struct tag_cqm_bat_table {
	u32 bat_entry_type[CQM_BAT_ENTRY_MAX];
	u8 bat[CQM_BAT_ENTRY_MAX * CQM_BAT_ENTRY_SIZE];
	struct tag_cqm_cla_table entry[CQM_BAT_ENTRY_MAX];
	/* In LB mode 1, the timer needs to be configured in 4 SMFs,
	 * and the GPAs must be different and independent.
	 */
	struct tag_cqm_cla_table timer_entry[4];
	u32 bat_size;
};

s32 cqm_bat_init(struct tag_cqm_handle *cqm_handle);
void cqm_bat_uninit(struct tag_cqm_handle *cqm_handle);
s32 cqm_cla_init(struct tag_cqm_handle *cqm_handle);
void cqm_cla_uninit(struct tag_cqm_handle *cqm_handle, u32 entry_numb);
u8 *cqm_cla_get_unlock(struct tag_cqm_handle *cqm_handle, struct tag_cqm_cla_table *cla_table,
		       u32 index, u32 count, dma_addr_t *pa);
u8 *cqm_cla_get_lock(struct tag_cqm_handle *cqm_handle, struct tag_cqm_cla_table *cla_table,
		     u32 index, u32 count, dma_addr_t *pa);
void cqm_cla_put(struct tag_cqm_handle *cqm_handle, struct tag_cqm_cla_table *cla_table,
		 u32 index, u32 count);
struct tag_cqm_cla_table *cqm_cla_table_get(struct tag_cqm_bat_table *bat_table,
					    u32 entry_type);
u32 cqm_funcid2smfid(const struct tag_cqm_handle *cqm_handle);

#endif /* CQM_BAT_CLA_H */

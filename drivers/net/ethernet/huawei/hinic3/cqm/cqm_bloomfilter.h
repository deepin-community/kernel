/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CQM_BLOOMFILTER_H
#define CQM_BLOOMFILTER_H

#include <linux/types.h>
#include <linux/mutex.h>

/* Bloomfilter entry size is 32B(256bit), whitch index is the 48-32-bit of the
 * hash. |31~26|25~20|19~14|13~8| will be used to locate 4 bloom filter section
 * in one entry. k_en[3:0] used to specify the section of bloom filter.
 */
#define CQM_BF_ENTRY_SIZE         32
#define CQM_BF_ENTRY_SIZE_UNIT    8
#define CQM_BF_BITARRAY_MAX       BIT(17)

#define CQM_BF_SECTION_NUMBER     4
#define CQM_BF_SECTION_BASE       8
#define CQM_BF_SECTION_SIZE       6
#define CQM_BF_SECTION_MASK       0x3f
#define CQM_BF_SECTION_BIT_NUMBER 64

#define CQM_BF_ARRAY_INDEX_OFFSET 32
#define CQM_BF_BITARRAY_INDEX(id, mask) \
	(((id) >> CQM_BF_ARRAY_INDEX_OFFSET) & (mask))

/* The unit of bloomfilter_length is 64B(512bits). */
#define CQM_BF_LENGTH_UNIT        9

#define CQM_DW_MASK               0xffffffff
#define CQM_DW_OFFSET             32
#define CQM_DW_INDEX0             0
#define CQM_DW_INDEX1             1
#define CQM_DW_INDEX2             2
#define CQM_DW_INDEX3             3

struct tag_cqm_bloomfilter_table {
	u32 *table;
	u32 table_size; /* The unit is bit */
	u32 array_mask; /* The unit of array entry is 32B, used to address entry
			 */
	struct mutex lock;
};

/* only for test */
s32 cqm_bloomfilter_cmd(void *ex_handle, u16 func_id, u32 op, u32 k_flag, u64 id);
s32 cqm_bloomfilter_init(void *ex_handle);
void cqm_bloomfilter_uninit(void *ex_handle);
s32 cqm_bloomfilter_inc(void *ex_handle, u16 func_id, u64 id);
s32 cqm_bloomfilter_dec(void *ex_handle, u16 func_id, u64 id);

#endif /* CQM_BLOOMFILTER_H */

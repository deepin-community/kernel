/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CQM_BITMAP_TABLE_H
#define CQM_BITMAP_TABLE_H

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/spinlock.h>

#include "cqm_object.h"
#include "vram_common.h"

struct tag_cqm_bitmap {
	ulong *table;
	u32 max_num;
	u32 last;
	u32 reserved_top; /* reserved index */
	u32 reserved_back;
	spinlock_t lock; /* lock for cqm */
	struct vram_buf_info bitmap_info;
};

struct tag_cqm_object_table {
	/* Now is big array. Later will be optimized as a red-black tree. */
	struct tag_cqm_object **table;
	u32 max_num;
	rwlock_t lock;
};

struct tag_cqm_handle;

s32 cqm_bitmap_init(struct tag_cqm_handle *cqm_handle);
void cqm_bitmap_uninit(struct tag_cqm_handle *cqm_handle);
u32 cqm_bitmap_alloc(struct tag_cqm_bitmap *bitmap, u32 step, u32 count, bool update_last);
u32 cqm_bitmap_alloc_low2bit_align(struct tag_cqm_bitmap *bitmap, u32 low2bit, bool update_last);
u32 cqm_bitmap_alloc_reserved(struct tag_cqm_bitmap *bitmap, u32 count, u32 index);
void cqm_bitmap_free(struct tag_cqm_bitmap *bitmap, u32 index, u32 count);
s32 cqm_object_table_init(struct tag_cqm_handle *cqm_handle);
void cqm_object_table_uninit(struct tag_cqm_handle *cqm_handle);
s32 cqm_object_table_insert(struct tag_cqm_handle *cqm_handle,
			    struct tag_cqm_object_table *object_table,
			    u32 index, struct tag_cqm_object *obj, bool bh);
void cqm_object_table_remove(struct tag_cqm_handle *cqm_handle,
			     struct tag_cqm_object_table *object_table,
			     u32 index, const struct tag_cqm_object *obj, bool bh);
struct tag_cqm_object *cqm_object_table_get(struct tag_cqm_handle *cqm_handle,
					    struct tag_cqm_object_table *object_table,
					    u32 index, bool bh);
u32 cqm_bitmap_alloc_by_xid(struct tag_cqm_bitmap *bitmap, u32 count, u32 index);

void cqm_swab64(u8 *addr, u32 cnt);
void cqm_swab32(u8 *addr, u32 cnt);
bool cqm_check_align(u32 data);
s32 cqm_shift(u32 data);
s32 cqm_buf_alloc(struct tag_cqm_handle *cqm_handle, struct tag_cqm_buf *buf, bool direct);
s32 cqm_buf_alloc_direct(struct tag_cqm_handle *cqm_handle, struct tag_cqm_buf *buf, bool direct);
void cqm_buf_free(struct tag_cqm_buf *buf, struct tag_cqm_handle *cqm_handle);
void cqm_buf_free_cache_inv(struct tag_cqm_handle *cqm_handle, struct tag_cqm_buf *buf,
			    s32 *inv_flag);
s32 cqm_cla_cache_invalid(struct tag_cqm_handle *cqm_handle, dma_addr_t gpa,
			  u32 cache_size);
void *cqm_kmalloc_align(size_t size, gfp_t flags, u16 align_order);
void cqm_kfree_align(void *addr);
void cqm_byte_print(u32 *ptr, u32 len);

#endif /* CQM_BITMAP_TABLE_H */

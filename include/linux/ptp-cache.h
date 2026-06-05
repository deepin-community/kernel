/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PTP_CACHE_H
#define _LINUX_PTP_CACHE_H
#include <asm/haoc/haoc-bitmap.h>
#include <linux/types.h>
union freelist_aba_t {
	struct {
		void **freelist;
		unsigned long counter;
	};
	u128 full;
};

struct iee_cache {
	union {
		struct {
			void **freelist;
			unsigned long tid;
		};
		union freelist_aba_t freelist_tid;
	};
	unsigned long reserve_order;
	unsigned long reserve_start_addr;
	unsigned long reserve_end_addr;
	unsigned long object_order;
	int levels;
	enum HAOC_BITMAP_TYPE name;
	atomic_t count;
	atomic_t fail_count;
	bool init;
};

extern struct iee_cache pg_cache;
#ifdef CONFIG_PTP_S
extern struct iee_cache pg_user_cache;
extern bool ptp_is_user_pgtable(const void *ptp);
#endif
extern void iee_cache_init(struct iee_cache *cache, unsigned long object_order,
				int levels, enum HAOC_BITMAP_TYPE bitmap_type,
				unsigned long reserve_order);
extern void *iee_cache_alloc(struct iee_cache *cache, gfp_t gfp);
extern void iee_cache_free(struct iee_cache *cache, void *object);
#endif

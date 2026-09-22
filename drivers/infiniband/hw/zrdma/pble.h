/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_PBLE_H
#define ZXDH_PBLE_H

#define PBLE_SHIFT 6
#define PBLE_PER_PAGE 512
#define HMC_PAGED_BP_SHIFT 12
#define PBLE_512_SHIFT 9
#define PBLE_INVALID_IDX 0xffffffff

enum zxdh_pble_level {
	PBLE_LEVEL_0 = 0,
	PBLE_LEVEL_1 = 1,
	PBLE_LEVEL_2 = 2,
};

enum zxdh_alloc_type {
	PBLE_NO_ALLOC = 0,
	PBLE_SD_CONTIGOUS = 1,
	PBLE_SD_PAGED = 2,
};

struct zxdh_chunk;

struct zxdh_pble_chunkinfo {
	struct zxdh_chunk *pchunk;
	u64 bit_idx;
	u64 bits_used;
};

struct zxdh_pble_info {
	u64 *addr;
	dma_addr_t pa;
	u64 smmu_fpm_addr;
	u32 idx;
	u32 cnt;
	struct zxdh_pble_chunkinfo chunkinfo;
	bool pble_copy;
};

struct zxdh_pble_level2 {
	struct zxdh_pble_info root;
	struct zxdh_pble_info *leaf;
	struct zxdh_virt_mem leafmem;
	u32 leaf_cnt;
};

struct zxdh_pble_alloc {
	u32 total_cnt;
	enum zxdh_pble_level level;
	union {
		struct zxdh_pble_info level1;
		struct zxdh_pble_level2 level2;
	};
};

struct sd_pd_idx {
	u32 sd_idx;
	u32 pd_idx;
	u32 rel_pd_idx;
};

struct zxdh_add_page_info {
	struct zxdh_chunk *chunk;
	struct zxdh_hmc_sd_entry *sd_entry;
	struct zxdh_hmc_info *hmc_info;
	struct sd_pd_idx idx;
	u32 pages;
};

struct zxdh_chunk {
	struct list_head list;
	struct zxdh_dma_info dmainfo;
	void *bitmapbuf;

	u32 sizeofbitmap;
	u64 size;
	void *vaddr;
	dma_addr_t pa;
	u64 fpm_addr;
	u32 pg_cnt;
	u32 max_bits_available;
	enum zxdh_alloc_type type;
	struct zxdh_sc_dev *dev;
	struct zxdh_virt_mem bitmapmem;
	struct zxdh_virt_mem chunkmem;
};

struct zxdh_pble_prm {
	struct list_head clist;
	spinlock_t prm_lock; /* protect prm bitmap */
	u64 total_pble_alloc;
	u64 free_pble_cnt;
	u8 pble_shift;
};

struct zxdh_hmc_pble_rsrc {
	u32 unallocated_pble;
	struct mutex pble_mutex_lock; /* protect PBLE resource */
	struct zxdh_sc_dev *dev;
	u64 fpm_base_addr;
	u64 next_fpm_addr;
	struct zxdh_pble_prm pinfo;
	u64 allocdpbles;
	u64 free_pbles;
	u32 stats_direct_sds;
	u32 stats_paged_sds;
	u64 stats_alloc_ok;
	u64 stats_alloc_fail;
	u64 stats_alloc_freed;
	u64 stats_lvl1;
	u64 stats_lvl2;
	u32 pble_type;
	bool pble_copy;
};

void zxdh_destroy_pble_prm(struct zxdh_hmc_pble_rsrc *pble_rsrc);
int zxdh_hmc_init_pble(struct zxdh_sc_dev *dev,
		       struct zxdh_hmc_pble_rsrc *pble_rsrc, int mr);

void zxdh_free_pble(struct zxdh_hmc_pble_rsrc *pble_rsrc,
		    struct zxdh_pble_alloc *palloc);
int zxdh_get_pble(struct zxdh_hmc_pble_rsrc *pble_rsrc,
		  struct zxdh_pble_alloc *palloc, u32 pble_cnt,
		  bool level1_only);
int zxdh_prm_add_pble_mem(struct zxdh_pble_prm *pprm,
			  struct zxdh_chunk *pchunk);
int zxdh_prm_get_pbles(struct zxdh_pble_prm *pprm,
		       struct zxdh_pble_chunkinfo *chunkinfo, u64 mem_size,
		       u64 **vaddr, u64 *fpm_addr, dma_addr_t *paaddr);
void zxdh_prm_return_pbles(struct zxdh_pble_prm *pprm,
			   struct zxdh_pble_chunkinfo *chunkinfo);
void zxdh_pble_free_paged_mem(struct zxdh_chunk *chunk);
#endif /* ZXDH_PBLE_H */

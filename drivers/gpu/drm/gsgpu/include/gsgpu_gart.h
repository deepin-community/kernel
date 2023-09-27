#ifndef __GSGPU_GART_H__
#define __GSGPU_GART_H__

#include <linux/types.h>

/*
 * GART structures, functions & helpers
 */
struct gsgpu_device;
struct gsgpu_bo;

#define GSGPU_GPU_PAGE_SIZE (16 * 1024)
#define GSGPU_GPU_PAGE_MASK (GSGPU_GPU_PAGE_SIZE - 1)
#define GSGPU_GPU_PAGE_SHIFT 14
#define GSGPU_PAGE_PTE_SHIFT (GSGPU_GPU_PAGE_SHIFT - 3)
#define GSGPU_GPU_PAGE_ALIGN(a) (((a) + GSGPU_GPU_PAGE_MASK) & ~GSGPU_GPU_PAGE_MASK)

#define GSGPU_GPU_PAGES_IN_CPU_PAGE (PAGE_SIZE / GSGPU_GPU_PAGE_SIZE)

struct gsgpu_gart {
	u64				table_addr;
	struct gsgpu_bo			*robj;
	void				*ptr;
	unsigned			num_gpu_pages;
	unsigned			num_cpu_pages;
	unsigned			table_size;
#ifdef CONFIG_DRM_GSGPU_GART_DEBUGFS
	struct page			**pages;
#endif
	bool				ready;

	/* Asic default pte flags */
	uint64_t			gart_pte_flags;
};

int gsgpu_gart_table_vram_alloc(struct gsgpu_device *adev);
void gsgpu_gart_table_vram_free(struct gsgpu_device *adev);
int gsgpu_gart_table_vram_pin(struct gsgpu_device *adev);
void gsgpu_gart_table_vram_unpin(struct gsgpu_device *adev);
int gsgpu_gart_init(struct gsgpu_device *adev);
void gsgpu_gart_fini(struct gsgpu_device *adev);
int gsgpu_gart_unbind(struct gsgpu_device *adev, uint64_t offset,
		       int pages);
int gsgpu_gart_map(struct gsgpu_device *adev, uint64_t offset,
		    int pages, dma_addr_t *dma_addr, uint64_t flags,
		    void *dst);
int gsgpu_gart_bind(struct gsgpu_device *adev, uint64_t offset,
		     int pages, struct page **pagelist,
		     dma_addr_t *dma_addr, uint64_t flags);
void gsgpu_gart_invalidate_tlb(struct gsgpu_device *adev);

#endif /* __GSGPU_GART_H__ */

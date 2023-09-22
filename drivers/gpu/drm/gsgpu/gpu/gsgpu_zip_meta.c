#include <drm/drmP.h>
#include <drm/gsgpu_drm.h>
#include "gsgpu.h"

/*
 * TODO write some description
 */

/*
 * Common zip meta table functions.
 */

/**
 * gsgpu_zip_meta_vram_alloc - allocate vram for zip meta page table
 *
 * @adev: gsgpu_device pointer
 *
 * Allocate video memory for zip meta page table
 * Returns 0 for success, error for failure.
 */
int gsgpu_zip_meta_vram_alloc(struct gsgpu_device *adev)
{
	int r;
	struct gsgpu_bo_param bp;

	if (adev->zip_meta.robj)
		return 0;

	memset(&bp, 0, sizeof(bp));
	bp.size = adev->zip_meta.table_size;
	bp.byte_align = GSGPU_GEM_COMPRESSED_SIZE;
	bp.domain = GSGPU_GEM_DOMAIN_VRAM;
	bp.flags = GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED |
		GSGPU_GEM_CREATE_VRAM_CONTIGUOUS;
	bp.type = ttm_bo_type_kernel;
	bp.resv = NULL;
	r = gsgpu_bo_create(adev, &bp, &adev->zip_meta.robj);
	if (r) {
		return r;
	}

	return 0;
}

/**
 * gsgpu_zip_meta_vram_pin - pin zip meta page table in vram
 *
 * @adev: gsgpu_device pointer
 *
 * Pin the zip meta page table in vram so it will not be moved
 * Returns 0 for success, error for failure.
 */
int gsgpu_zip_meta_vram_pin(struct gsgpu_device *adev)
{
	int r;

	r = gsgpu_bo_reserve(adev->zip_meta.robj, false);
	if (unlikely(r != 0))
		return r;
	r = gsgpu_bo_pin(adev->zip_meta.robj, GSGPU_GEM_DOMAIN_VRAM);
	if (r) {
		gsgpu_bo_unreserve(adev->zip_meta.robj);
		return r;
	}
	r = gsgpu_bo_kmap(adev->zip_meta.robj, &adev->zip_meta.ptr);
	if (r)
		gsgpu_bo_unpin(adev->zip_meta.robj);
	gsgpu_bo_unreserve(adev->zip_meta.robj);

	adev->zip_meta.table_addr = gsgpu_bo_gpu_offset(adev->zip_meta.robj);
	return r;
}

/**
 * gsgpu_zip_meta_vram_unpin - unpin zip meta page table in vram
 *
 * @adev: gsgpu_device pointer
 *
 * Unpin the ZIP META page table in vram (pcie r4xx, r5xx+).
 * These asics require the zip meta table to be in video memory.
 */
void gsgpu_zip_meta_vram_unpin(struct gsgpu_device *adev)
{
	int r;

	if (adev->zip_meta.robj == NULL) {
		return;
	}
	r = gsgpu_bo_reserve(adev->zip_meta.robj, true);
	if (likely(r == 0)) {
		gsgpu_bo_kunmap(adev->zip_meta.robj);
		gsgpu_bo_unpin(adev->zip_meta.robj);
		gsgpu_bo_unreserve(adev->zip_meta.robj);
		adev->zip_meta.ptr = NULL;
	}
}

/**
 * gsgpu_zip_meta_vram_free - free zip meta page table vram
 *
 * @adev: gsgpu_device pointer
 *
 */
void gsgpu_zip_meta_vram_free(struct gsgpu_device *adev)
{
	if (adev->zip_meta.robj == NULL) {
		return;
	}
	gsgpu_bo_unref(&adev->zip_meta.robj);
}

/*
 * Common zip meta functions.
 */
/**
 * gsgpu_zip_meta_unbind - unbind pages from the zip meta page table
 *
 * @adev: gsgpu_device pointer
 * @offset: offset into the GPU's zip meta aperture
 * @pages: number of pages to unbind
 *
 * Unbinds the requested pages from the zip meta page table and
 * Returns 0 for success, -EINVAL for failure.
 */
int gsgpu_zip_meta_unbind(struct gsgpu_device *adev, uint64_t offset,
			int pages)
{
	return 0;
}

/**
 * gsgpu_zip_meta_map - map dma_addresses into zip meta entries
 *
 * @adev: gsgpu_device pointer
 * @start: start into the GPU's memery aperture
 *
 * Map the dma_addresses into zip meta entries (all asics).
 * Returns 0 for success, -EINVAL for failure.
 */
uint64_t gsgpu_zip_meta_map(struct gsgpu_device *adev, uint64_t start)
{
	return 0;
}

/**
 * gsgpu_zip_meta_bind - bind pages into the zip meta page table
 *
 * @adev: gsgpu_device pointer
 * @offset: offset into the GPU's zip meta aperture
 * @pages: number of pages to bind
 * @pagelist: pages to bind
 * @dma_addr: DMA addresses of pages
 *
 * Binds the requested pages to the zip meta page table
 * (all asics).
 * Returns 0 for success, -EINVAL for failure.
 */
int gsgpu_zip_meta_bind(struct gsgpu_device *adev, uint64_t offset,
		     int pages, struct page **pagelist, dma_addr_t *dma_addr,
		     uint64_t flags)
{
#ifdef CONFIG_DRM_GSGPU_ZIP_DEBUGFS
	unsigned i, t, p;
#endif

#ifdef CONFIG_DRM_GSGPU_ZIP_DEBUGFS
	t = offset / GSGPU_GPU_PAGE_SIZE;
	p = t / GSGPU_GPU_PAGES_IN_CPU_PAGE;
	for (i = 0; i < pages; i++, p++)
		adev->zip_meta.pages[p] = pagelist ? pagelist[i] : NULL;
#endif

	return 0;
}

/**
 * gsgpu_zip_meta_init - init the driver info for managing the zip meta
 *
 * @adev: gsgpu_device pointer
 *
 * Allocate the dummy page and init the zip meta driver info (all asics).
 * Returns 0 for success, error for failure.
 */
int gsgpu_zip_meta_init(struct gsgpu_device *adev)
{
	/* We need PAGE_SIZE >= GSGPU_GPU_PAGE_SIZE */
	if (PAGE_SIZE < GSGPU_GPU_PAGE_SIZE) {
		DRM_ERROR("Page size is smaller than GPU page size!\n");
		return -EINVAL;
	}

	/* Compute table size */
	adev->zip_meta.num_cpu_pages = (adev->gmc.real_vram_size >> 7) / PAGE_SIZE;
	adev->zip_meta.num_gpu_pages = (adev->gmc.real_vram_size >> 7) / GSGPU_GPU_PAGE_SIZE;
	DRM_INFO("ZIP META: num cpu pages %u, num gpu pages %u\n",
		 adev->zip_meta.num_cpu_pages, adev->zip_meta.num_gpu_pages);

#ifdef CONFIG_DRM_GSGPU_ZIP_DEBUGFS
	/* Allocate pages table */
	adev->zip_meta.pages = vzalloc(array_size(sizeof(void *),
					      adev->zip_meta.num_cpu_pages));
	if (adev->zip_meta.pages == NULL)
		return -ENOMEM;
#endif

	return 0;
}

/**
 * gsgpu_zip_meta_fini - tear down the driver info for managing the zip meta
 *
 * @adev: gsgpu_device pointer
 *
 */
void gsgpu_zip_meta_fini(struct gsgpu_device *adev)
{
#ifdef CONFIG_DRM_GSGPU_ZIP_DEBUGFS
	vfree(adev->zip_meta.pages);
	adev->zip_meta.pages = NULL;
#endif
}

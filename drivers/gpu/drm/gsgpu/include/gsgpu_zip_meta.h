#ifndef __GSGPU_ZIP_META_H__
#define __GSGPU_ZIP_META_H__

#include <linux/types.h>

/*
 * ZIP META structures, functions & helpers
 */
struct gsgpu_device;
struct gsgpu_bo;


#define GSGPU_GEM_META_ALIGN_SHIFT 7
#define GSGPU_GEM_COMPRESSED_SIZE (PAGE_SIZE<<GSGPU_GEM_META_ALIGN_SHIFT)

struct gsgpu_zip_meta {
	u64 table_addr;
	u64 mask;
	struct gsgpu_bo *robj;
	void *ptr;
	unsigned num_gpu_pages;
	unsigned num_cpu_pages;
	unsigned table_size;
#ifdef CONFIG_DRM_GSGPU_ZIP_DEBUGFS
	struct page **pages;
#endif
	bool ready;
	u64 pte_flags;
};

int gsgpu_zip_meta_vram_alloc(struct gsgpu_device *adev);
void gsgpu_zip_meta_vram_free(struct gsgpu_device *adev);
int gsgpu_zip_meta_vram_pin(struct gsgpu_device *adev);
void gsgpu_zip_meta_vram_unpin(struct gsgpu_device *adev);
int gsgpu_zip_meta_init(struct gsgpu_device *adev);
void gsgpu_zip_meta_fini(struct gsgpu_device *adev);
int gsgpu_zip_meta_unbind(struct gsgpu_device *adev, uint64_t offset,
		       int pages);
uint64_t gsgpu_zip_meta_map(struct gsgpu_device *adev, uint64_t start);
int gsgpu_zip_meta_bind(struct gsgpu_device *adev, uint64_t offset,
		     int pages, struct page **pagelist,
		     dma_addr_t *dma_addr, uint64_t flags);

#endif /* __GSGPU_ZIP_META_H__ */

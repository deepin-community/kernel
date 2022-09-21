/*
 * Copyright 2016 SiliconMotion Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License version 2. See the file COPYING in the main
 * directory of this archive for more details.
 *
 */

#include "smi_dbg.h"
#include "smi_drv.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#include <drm/drm_pci.h>
#else
#include <linux/pci.h>
#endif
#else
#include <drm/drmP.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
#include <drm/ttm/ttm_page_alloc.h>

static inline struct smi_device *smi_bdev(struct ttm_bo_device *bd)
{
	return container_of(bd, struct smi_device, ttm.bdev);
}

void smi_bo_ttm_destroy(struct ttm_buffer_object *tbo)
{
	struct smi_bo *bo;

	bo = container_of(tbo, struct smi_bo, bo);

	drm_gem_object_release(&bo->gem);
	kfree(bo);
}

bool smi_ttm_bo_is_smi_bo(struct ttm_buffer_object *bo)
{
	if (bo->destroy == &smi_bo_ttm_destroy)
		return true;
	return false;
}

static int smi_bo_init_mem_type(struct ttm_bo_device *bdev, uint32_t type,
				struct ttm_mem_type_manager *man)
{
	switch (type) {
	case TTM_PL_SYSTEM:
		man->flags = TTM_MEMTYPE_FLAG_MAPPABLE;
		man->available_caching = TTM_PL_MASK_CACHING;
		man->default_caching = TTM_PL_FLAG_CACHED;
		break;
	case TTM_PL_VRAM:
		man->func = &ttm_bo_manager_func;
		man->flags = TTM_MEMTYPE_FLAG_FIXED | TTM_MEMTYPE_FLAG_MAPPABLE;
#ifdef NO_WC
		man->available_caching = TTM_PL_FLAG_UNCACHED;
		man->default_caching = TTM_PL_FLAG_UNCACHED;
#else
		man->available_caching = TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_WC;
		man->default_caching = TTM_PL_FLAG_WC;
#endif
		break;
	default:
		DRM_ERROR("Unsupported memory type %u\n", (unsigned)type);
		return -EINVAL;
	}
	return 0;
}

static void smi_bo_evict_flags(struct ttm_buffer_object *bo, struct ttm_placement *pl)
{
	struct smi_bo *smibo = smi_bo(bo);

	if (!smi_ttm_bo_is_smi_bo(bo))
		return;

	smi_ttm_placement(smibo, TTM_PL_FLAG_SYSTEM);
	*pl = smibo->placement;
}

static int smi_ttm_io_mem_reserve(struct ttm_bo_device *bdev, struct ttm_mem_reg *mem)
{
	struct ttm_mem_type_manager *man = &bdev->man[mem->mem_type];
	struct smi_device *smi = smi_bdev(bdev);

	mem->bus.addr = NULL;
	mem->bus.offset = 0;
	mem->bus.size = mem->num_pages << PAGE_SHIFT;
	mem->bus.base = 0;
	mem->bus.is_iomem = false;
	if (!(man->flags & TTM_MEMTYPE_FLAG_MAPPABLE))
		return -EINVAL;
	switch (mem->mem_type) {
	case TTM_PL_SYSTEM:
		/* system memory */
		return 0;
	case TTM_PL_VRAM:
		mem->bus.offset = mem->start << PAGE_SHIFT;
		mem->bus.base = pci_resource_start(smi->dev->pdev, 0);
		mem->bus.is_iomem = true;
		break;
	default:
		return -EINVAL;
		break;
	}
	return 0;
}

static void smi_ttm_io_mem_free(struct ttm_bo_device *bdev, struct ttm_mem_reg *mem)
{
}

static void smi_ttm_backend_destroy(struct ttm_tt *tt)
{
	ttm_tt_fini(tt);
	kfree(tt);
}

static struct ttm_backend_func smi_tt_backend_func = {
	.destroy = &smi_ttm_backend_destroy,
};

static void smi_ttm_tt_unpopulate(struct ttm_tt *ttm)
{
	bool slave = !!(ttm->page_flags & TTM_PAGE_FLAG_SG);

	if (slave)
		return;

	ttm_pool_unpopulate(ttm);
}

static inline u64 smi_bo_gpu_offset(struct smi_bo *bo)
{
	return bo->bo.offset;
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
static int smi_bo_verify_access(struct ttm_buffer_object *bo, struct file *filp)
{
	struct smi_bo *smibo = smi_bo(bo);
	return drm_vma_node_verify_access(&smibo->gem.vma_node, filp->private_data);
}

struct ttm_tt *smi_ttm_tt_create(struct ttm_buffer_object *bo, uint32_t page_flags)
{
	struct ttm_tt *tt;

	tt = kzalloc(sizeof(struct ttm_tt), GFP_KERNEL);
	if (tt == NULL)
		return NULL;
	tt->func = &smi_tt_backend_func;
	if (ttm_tt_init(tt, bo, page_flags)) {
		kfree(tt);
		return NULL;
	}
	return tt;
}

static int smi_ttm_tt_populate(struct ttm_tt *ttm, struct ttm_operation_ctx *ctx)
{
	bool slave = !!(ttm->page_flags & TTM_PAGE_FLAG_SG);

	if (ttm->state != tt_unpopulated)
		return 0;

	if (slave && ttm->sg) {
		drm_prime_sg_to_page_addr_arrays(ttm->sg, ttm->pages, NULL, ttm->num_pages);
		ttm->state = tt_unbound;
		return 0;
	}

	return ttm_pool_populate(ttm, ctx);
}

struct ttm_bo_driver smi_bo_driver = {
	.ttm_tt_create = smi_ttm_tt_create,
	.ttm_tt_populate = smi_ttm_tt_populate,
	.ttm_tt_unpopulate = smi_ttm_tt_unpopulate,
	.init_mem_type = smi_bo_init_mem_type,
	.evict_flags = smi_bo_evict_flags,
	.eviction_valuable = ttm_bo_eviction_valuable,
	.move = NULL,
	.verify_access = smi_bo_verify_access,
	.io_mem_reserve = &smi_ttm_io_mem_reserve,
	.io_mem_free = &smi_ttm_io_mem_free,
};
#endif

int smi_mm_init(struct smi_device *smi)
{
	int ret;
	struct pci_dev *pdev;
	struct drm_vram_mm *vmm __attribute__((unused));
	struct drm_device *dev = smi->dev;
	unsigned long vram_size = smi->mc.vram_size;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	pdev = to_pci_dev(dev->dev);
#else
	pdev = dev->pdev;
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
	struct ttm_bo_device *bdev = &smi->ttm.bdev;

	ret = ttm_bo_device_init(&smi->ttm.bdev,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
				 &smi_bo_driver, dev->anon_inode->i_mapping, smi->need_dma32
#else
				 &smi_bo_driver, dev->anon_inode->i_mapping, DRM_FILE_PAGE_OFFSET,
				 smi->need_dma32
#endif
	);
	if (ret) {
		DRM_ERROR("Error initialising bo driver; %d\n", ret);
		return ret;
	}

	ret = ttm_bo_init_mm(bdev, TTM_PL_VRAM, vram_size >> PAGE_SHIFT);

	if (ret) {
		DRM_ERROR("Failed ttm VRAM init: %d\n", ret);
		return ret;
	}
#else

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)	
	vmm = drm_vram_helper_alloc_mm(dev, pci_resource_start(pdev, 0),
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
				       vram_size);
#else
				       vram_size, &drm_gem_vram_mm_funcs);
#endif
	if (IS_ERR(vmm)) {
		ret = PTR_ERR(vmm);

#else
		ret = drmm_vram_helper_init(dev, pci_resource_start(pdev, 0),
				    vram_size);
		if (ret) {
#endif
			DRM_ERROR("Error initializing VRAM MM; %d\n", ret);
			return ret;
	}
#endif

	arch_io_reserve_memtype_wc(pci_resource_start(pdev, 0),
				   pci_resource_len(pdev, 0));

	smi->fb_mtrr =
		arch_phys_wc_add(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));

	smi->mm_inited = true;
	return 0;
}

void smi_mm_fini(struct smi_device *smi)
{
	struct pci_dev *pdev;
	struct drm_device *dev = smi->dev;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	pdev = to_pci_dev(dev->dev);
#else
	pdev = dev->pdev;
#endif

	if (!smi->mm_inited)
		return;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
	ttm_bo_device_release(&smi->ttm.bdev);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	drm_vram_helper_release_mm(dev);
#endif

	arch_io_free_memtype_wc(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));

	arch_phys_wc_del(smi->fb_mtrr);

	smi->fb_mtrr = 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
void smi_ttm_placement(struct smi_bo *bo, int domain)
{
	u32 c = 0;
	unsigned i;
	bo->placement.placement = bo->placements;
	bo->placement.busy_placement = bo->placements;
	if (domain & TTM_PL_FLAG_VRAM)
#ifdef NO_WC
		bo->placements[c++].flags = TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_VRAM;
#else
		bo->placements[c++].flags =
			TTM_PL_FLAG_WC | TTM_PL_FLAG_UNCACHED | TTM_PL_FLAG_VRAM;
#endif
	if (domain & TTM_PL_FLAG_SYSTEM)
		bo->placements[c++].flags = TTM_PL_MASK_CACHING | TTM_PL_FLAG_SYSTEM;
	if (!c)
		bo->placements[c++].flags = TTM_PL_MASK_CACHING | TTM_PL_FLAG_SYSTEM;
	bo->placement.num_placement = c;
	bo->placement.num_busy_placement = c;
	for (i = 0; i < c; ++i) {
		bo->placements[i].fpfn = 0;
		bo->placements[i].lpfn = 0;
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
int smi_bo_create(struct drm_device *dev, int size, int align, uint32_t flags, struct sg_table *sg,
		  struct reservation_object *resv, struct smi_bo **psmibo)
#else
int smi_bo_create(struct drm_device *dev, int size, int align, uint32_t flags, struct sg_table *sg,
		  struct dma_resv *resv, struct smi_bo **psmibo)
#endif
{
	struct smi_device *smi = dev->dev_private;
	struct smi_bo *smibo;
	size_t acc_size;
	enum ttm_bo_type type;
	int ret;

	*psmibo = NULL;

	if (sg) {
		type = ttm_bo_type_sg;
	} else {
		type = ttm_bo_type_device;
	}

	smibo = kzalloc(sizeof(struct smi_bo), GFP_KERNEL);
	if (!smibo)
		return -ENOMEM;

	ret = drm_gem_object_init(dev, &smibo->gem, size);
	if (ret)
		goto error;

	smibo->bo.bdev = &smi->ttm.bdev;
	smi_ttm_placement(smibo, TTM_PL_FLAG_VRAM | TTM_PL_FLAG_SYSTEM);

	acc_size = ttm_bo_dma_acc_size(&smi->ttm.bdev, size, sizeof(struct smi_bo));

	ret = ttm_bo_init(&smi->ttm.bdev, &smibo->bo, size, type, &smibo->placement,
			  align >> PAGE_SHIFT, false, acc_size, sg, resv, smi_bo_ttm_destroy);
	if (ret)
		goto error;

	*psmibo = smibo;
	return 0;

error:
	kfree(smibo);
	return ret;
}

static int smi_bo_reserve(struct smi_bo *bo, bool no_wait)
{
	int ret;
	ret = ttm_bo_reserve(&bo->bo, true, no_wait, NULL);
	if (ret) {
		if (ret != -ERESTARTSYS && ret != -EBUSY)
			DRM_ERROR("reserve failed %p\n", bo);
		return ret;
	}
	return 0;
}

static void smi_bo_unreserve(struct smi_bo *bo)
{
	ttm_bo_unreserve(&bo->bo);
}

int smi_bo_pin(struct smi_bo *bo, u32 pl_flag, u64 *gpu_addr)
{
	int i, ret;
	struct ttm_operation_ctx ctx = { false, false };

	ret = smi_bo_reserve(bo, false);
	if (unlikely(ret < 0))
		return ret;

	if (bo->pin_count)
		goto out;

	if (pl_flag)
		smi_ttm_placement(bo, pl_flag);

	for (i = 0; i < bo->placement.num_placement; i++)
		bo->placements[i].flags |= TTM_PL_FLAG_NO_EVICT;

	ret = ttm_bo_validate(&bo->bo, &bo->placement, &ctx);
	if (ret < 0)
		goto err_smi_bo_unreserve;

out:
	++bo->pin_count;
	if (gpu_addr)
		*gpu_addr = smi_bo_gpu_offset(bo);
	smi_bo_unreserve(bo);
	return 0;

err_smi_bo_unreserve:
	smi_bo_unreserve(bo);
	return ret;
}

int smi_bo_unpin(struct smi_bo *bo)
{
	int i, ret;
	struct ttm_operation_ctx ctx = { false, false };

	ret = smi_bo_reserve(bo, false);
	if (unlikely(ret < 0))
		return ret;

	if (!bo->pin_count)
		goto out;

	--bo->pin_count;
	if (bo->pin_count)
		goto out;

	for (i = 0; i < bo->placement.num_placement; i++)
		bo->placements[i].flags &= ~TTM_PL_FLAG_NO_EVICT;

	ret = ttm_bo_validate(&bo->bo, &bo->placement, &ctx);
	if (ret < 0)
		goto err_smi_bo_unreserve;

out:
	smi_bo_unreserve(bo);
	return 0;

err_smi_bo_unreserve:
	smi_bo_unreserve(bo);
	return ret;
}

int smi_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct drm_file *file_priv;
	struct smi_device *smi;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
	if (unlikely(vma->vm_pgoff < DRM_FILE_PAGE_OFFSET))
		return -EINVAL;
#endif

	file_priv = filp->private_data;
	smi = file_priv->minor->dev->dev_private;

	return ttm_bo_mmap(filp, vma, &smi->ttm.bdev);
}

void *smi_bo_kmap(struct smi_bo *bo, bool map, bool *is_iomem)
{
	int ret;
	struct ttm_bo_kmap_obj *kmap = &bo->kmap;

	if (kmap->virtual || !map)
		goto out;

	ret = ttm_bo_kmap(&bo->bo, 0, bo->bo.num_pages, kmap);
	if (ret)
		return ERR_PTR(ret);

out:
	if (!is_iomem)
		return kmap->virtual;
	if (!kmap->virtual) {
		*is_iomem = false;
		return NULL;
	}
	return ttm_kmap_obj_virtual(kmap, is_iomem);
}

void smi_bo_kunmap(struct smi_bo *bo)
{
	struct ttm_bo_kmap_obj *kmap = &bo->kmap;

	if (!kmap->virtual)
		return;

	ttm_bo_kunmap(kmap);
	kmap->virtual = NULL;
}
#endif
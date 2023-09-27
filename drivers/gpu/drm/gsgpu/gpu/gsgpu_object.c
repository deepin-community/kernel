#include <linux/list.h>
#include <linux/slab.h>
#include <drm/gsgpu_drm.h>
#include <drm/drm_cache.h>
#include <asm/dma.h>
#include "gsgpu.h"
#include "gsgpu_trace.h"

/**
 * DOC: gsgpu_object
 *
 * This defines the interfaces to operate on a &gsgpu_bo buffer object which
 * represents memory used by the driver (VRAM, system memory, etc.). The driver
 * provides DRM/GEM APIs to userspace. DRM/GEM APIs then use these interfaces
 * to create/destroy/set buffer objects which are then managed by the kernel TTM
 * memory manager.
 *
 * The interfaces are also used internally by kernel clients, including gfx,
 * uvd, etc. for kernel managed allocations used by the GPU.
 */

static bool gsgpu_bo_need_backup(struct gsgpu_device *adev)
{
	if (adev->flags & GSGPU_IS_APU)
		return false;

	if (gsgpu_gpu_recovery == 0 || gsgpu_gpu_recovery == -1)
		return false;

	return true;
}

/**
 * gsgpu_bo_subtract_pin_size - Remove BO from pin_size accounting
 *
 * @bo: &gsgpu_bo buffer object
 *
 * This function is called when a BO stops being pinned, and updates the
 * &gsgpu_device pin_size values accordingly.
 */
static void gsgpu_bo_subtract_pin_size(struct gsgpu_bo *bo)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);

	if (bo->tbo.resource->mem_type == TTM_PL_VRAM) {
		atomic64_sub(gsgpu_bo_size(bo), &adev->vram_pin_size);
		atomic64_sub(gsgpu_vram_mgr_bo_visible_size(bo),
			     &adev->visible_pin_size);
	} else if (bo->tbo.resource->mem_type == TTM_PL_TT) {
		atomic64_sub(gsgpu_bo_size(bo), &adev->gart_pin_size);
	}
}

static void gsgpu_bo_destroy(struct ttm_buffer_object *tbo)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(tbo->bdev);
	struct gsgpu_bo *bo = ttm_to_gsgpu_bo(tbo);

	if (bo->tbo.pin_count > 0)
		gsgpu_bo_subtract_pin_size(bo);

	gsgpu_bo_kunmap(bo);

	if (bo->gem_base.import_attach)
		drm_prime_gem_destroy(&bo->gem_base, bo->tbo.sg);
	drm_gem_object_release(&bo->gem_base);
	gsgpu_bo_unref(&bo->parent);
	if (!list_empty(&bo->shadow_list)) {
		mutex_lock(&adev->shadow_list_lock);
		list_del_init(&bo->shadow_list);
		mutex_unlock(&adev->shadow_list_lock);
	}
	kfree(bo->metadata);
	kfree(bo);
}

/**
 * gsgpu_bo_is_gsgpu_bo - check if the buffer object is an &gsgpu_bo
 * @bo: buffer object to be checked
 *
 * Uses destroy function associated with the object to determine if this is
 * an &gsgpu_bo.
 *
 * Returns:
 * true if the object belongs to &gsgpu_bo, false if not.
 */
bool gsgpu_bo_is_gsgpu_bo(struct ttm_buffer_object *bo)
{
	if (bo->destroy == &gsgpu_bo_destroy)
		return true;
	return false;
}

/**
 * gsgpu_bo_placement_from_domain - set buffer's placement
 * @abo: &gsgpu_bo buffer object whose placement is to be set
 * @domain: requested domain
 *
 * Sets buffer's placement according to requested domain and the buffer's
 * flags.
 */
void gsgpu_bo_placement_from_domain(struct gsgpu_bo *abo, u32 domain)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(abo->tbo.bdev);
	struct ttm_placement *placement = &abo->placement;
	struct ttm_place *places = abo->placements;
	u64 flags = abo->flags;
	u32 c = 0;

	if (domain & GSGPU_GEM_DOMAIN_VRAM) {
		unsigned visible_pfn = adev->gmc.visible_vram_size >> PAGE_SHIFT;

		places[c].fpfn = 0;
		places[c].lpfn = 0;
		places[c].mem_type = TTM_PL_VRAM;
		places[c].flags = 0;

		if (flags & GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED)
			places[c].lpfn = visible_pfn;
		else
			places[c].flags |= TTM_PL_FLAG_TOPDOWN;

		if (flags & GSGPU_GEM_CREATE_VRAM_CONTIGUOUS)
			places[c].flags |= TTM_PL_FLAG_CONTIGUOUS;
		c++;
	}

	if (domain & GSGPU_GEM_DOMAIN_GTT) {
		places[c].fpfn = 0;
		if (flags & GSGPU_GEM_CREATE_SHADOW)
			places[c].lpfn = adev->gmc.gart_size >> PAGE_SHIFT;
		else
			places[c].lpfn = 0;
		places[c].mem_type = TTM_PL_TT;
		places[c].flags = 0;
		c++;
	}

	if (domain & GSGPU_GEM_DOMAIN_CPU) {
		places[c].fpfn = 0;
		places[c].lpfn = 0;
		places[c].mem_type = TTM_PL_SYSTEM;
		places[c].flags = 0;
		c++;
	}

	if (!c) {
		places[c].fpfn = 0;
		places[c].lpfn = 0;
		places[c].mem_type = TTM_PL_SYSTEM;
		places[c].flags = 0;
		c++;
	}

	BUG_ON(c >= GSGPU_BO_MAX_PLACEMENTS);

	placement->num_placement = c;
	placement->placement = places;

	placement->num_busy_placement = c;
	placement->busy_placement = places;
}

/**
 * gsgpu_bo_create_reserved - create reserved BO for kernel use
 *
 * @adev: gsgpu device object
 * @size: size for the new BO
 * @align: alignment for the new BO
 * @domain: where to place it
 * @bo_ptr: used to initialize BOs in structures
 * @gpu_addr: GPU addr of the pinned BO
 * @cpu_addr: optional CPU address mapping
 *
 * Allocates and pins a BO for kernel internal use, and returns it still
 * reserved.
 *
 * Note: For bo_ptr new BO is only created if bo_ptr points to NULL.
 *
 * Returns:
 * 0 on success, negative error code otherwise.
 */
int gsgpu_bo_create_reserved(struct gsgpu_device *adev,
			      unsigned long size, int align,
			      u32 domain, struct gsgpu_bo **bo_ptr,
			      u64 *gpu_addr, void **cpu_addr)
{
	struct gsgpu_bo_param bp;
	bool free = false;
	int r;

	memset(&bp, 0, sizeof(bp));
	bp.size = size;
	bp.byte_align = align;
	bp.domain = domain;
	bp.flags = GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED |
		GSGPU_GEM_CREATE_VRAM_CONTIGUOUS;
	bp.type = ttm_bo_type_kernel;
	bp.resv = NULL;

	if (!*bo_ptr) {
		r = gsgpu_bo_create(adev, &bp, bo_ptr);
		if (r) {
			dev_err(adev->dev, "(%d) failed to allocate kernel bo\n",
				r);
			return r;
		}
		free = true;
	}

	r = gsgpu_bo_reserve(*bo_ptr, false);
	if (r) {
		dev_err(adev->dev, "(%d) failed to reserve kernel bo\n", r);
		goto error_free;
	}

	r = gsgpu_bo_pin(*bo_ptr, domain);
	if (r) {
		dev_err(adev->dev, "(%d) kernel bo pin failed\n", r);
		goto error_unreserve;
	}

	r = gsgpu_ttm_alloc_gart(&(*bo_ptr)->tbo);
	if (r) {
		dev_err(adev->dev, "%p bind failed\n", *bo_ptr);
		goto error_unpin;
	}

	if (gpu_addr)
		*gpu_addr = gsgpu_bo_gpu_offset(*bo_ptr);

	if (cpu_addr) {
		r = gsgpu_bo_kmap(*bo_ptr, cpu_addr);
		if (r) {
			dev_err(adev->dev, "(%d) kernel bo map failed\n", r);
			goto error_unpin;
		}
	}

	return 0;

error_unpin:
	gsgpu_bo_unpin(*bo_ptr);
error_unreserve:
	gsgpu_bo_unreserve(*bo_ptr);

error_free:
	if (free)
		gsgpu_bo_unref(bo_ptr);

	return r;
}

/**
 * gsgpu_bo_create_kernel - create BO for kernel use
 *
 * @adev: gsgpu device object
 * @size: size for the new BO
 * @align: alignment for the new BO
 * @domain: where to place it
 * @bo_ptr:  used to initialize BOs in structures
 * @gpu_addr: GPU addr of the pinned BO
 * @cpu_addr: optional CPU address mapping
 *
 * Allocates and pins a BO for kernel internal use.
 *
 * Note: For bo_ptr new BO is only created if bo_ptr points to NULL.
 *
 * Returns:
 * 0 on success, negative error code otherwise.
 */
int gsgpu_bo_create_kernel(struct gsgpu_device *adev,
			    unsigned long size, int align,
			    u32 domain, struct gsgpu_bo **bo_ptr,
			    u64 *gpu_addr, void **cpu_addr)
{
	int r;

	r = gsgpu_bo_create_reserved(adev, size, align, domain, bo_ptr,
				      gpu_addr, cpu_addr);

	if (r)
		return r;

	gsgpu_bo_unreserve(*bo_ptr);

	return 0;
}

/**
 * gsgpu_bo_free_kernel - free BO for kernel use
 *
 * @bo: gsgpu BO to free
 * @gpu_addr: pointer to where the BO's GPU memory space address was stored
 * @cpu_addr: pointer to where the BO's CPU memory space address was stored
 *
 * unmaps and unpin a BO for kernel internal use.
 */
void gsgpu_bo_free_kernel(struct gsgpu_bo **bo, u64 *gpu_addr,
			   void **cpu_addr)
{
	if (*bo == NULL)
		return;

	if (likely(gsgpu_bo_reserve(*bo, true) == 0)) {
		if (cpu_addr)
			gsgpu_bo_kunmap(*bo);

		gsgpu_bo_unpin(*bo);
		gsgpu_bo_unreserve(*bo);
	}
	gsgpu_bo_unref(bo);

	if (gpu_addr)
		*gpu_addr = 0;

	if (cpu_addr)
		*cpu_addr = NULL;
}

/* Validate bo size is bit bigger then the request domain */
static bool gsgpu_bo_validate_size(struct gsgpu_device *adev,
					  unsigned long size, u32 domain)
{
	struct ttm_resource_manager *man = NULL;

	/*
	 * If GTT is part of requested domains the check must succeed to
	 * allow fall back to GTT
	 */
	if (domain & GSGPU_GEM_DOMAIN_GTT) {
		man = adev->mman.bdev.man_drv[TTM_PL_TT];

		if (size < (man->size << PAGE_SHIFT))
			return true;
		else
			goto fail;
	}

	if (domain & GSGPU_GEM_DOMAIN_VRAM) {
		man = adev->mman.bdev.man_drv[TTM_PL_VRAM];

		if (size < (man->size << PAGE_SHIFT))
			return true;
		else
			goto fail;
	}


	/* TODO add more domains checks, such as GSGPU_GEM_DOMAIN_CPU */
	return true;

fail:
	DRM_DEBUG("BO size %lu > total memory in domain: %llu\n", size,
		  man->size << PAGE_SHIFT);
	return false;
}

static int gsgpu_bo_do_create(struct gsgpu_device *adev,
			       struct gsgpu_bo_param *bp,
			       struct gsgpu_bo **bo_ptr)
{
	struct ttm_operation_ctx ctx = {
		.interruptible = (bp->type != ttm_bo_type_kernel),
		.no_wait_gpu = false,
		/* We opt to avoid OOM on system pages allocations */
		.gfp_retry_mayfail = true,
		.resv = bp->resv,
		.flags = TTM_OPT_FLAG_ALLOW_RES_EVICT
	};
	struct gsgpu_bo *bo;
	unsigned long page_align, size = bp->size;
	size_t acc_size;
	int r;

	page_align = roundup(bp->byte_align, PAGE_SIZE) >> PAGE_SHIFT;
	size = ALIGN(size, PAGE_SIZE);

	if (!gsgpu_bo_validate_size(adev, size, bp->domain))
		return -ENOMEM;

	*bo_ptr = NULL;

	acc_size = ttm_bo_dma_acc_size(&adev->mman.bdev, size,
				       sizeof(struct gsgpu_bo));

	bo = kzalloc(sizeof(struct gsgpu_bo), GFP_KERNEL);
	if (bo == NULL)
		return -ENOMEM;
	drm_gem_private_object_init(adev->ddev, &bo->gem_base, size);
	INIT_LIST_HEAD(&bo->shadow_list);
	INIT_LIST_HEAD(&bo->va);
	bo->preferred_domains = bp->preferred_domain ? bp->preferred_domain :
		bp->domain;
	bo->allowed_domains = bo->preferred_domains;
	if (bp->type != ttm_bo_type_kernel &&
	    bo->allowed_domains == GSGPU_GEM_DOMAIN_VRAM)
		bo->allowed_domains |= GSGPU_GEM_DOMAIN_GTT;

	bo->flags = bp->flags;

	if (!drm_arch_can_wc_memory())
		bo->flags &= ~GSGPU_GEM_CREATE_CPU_GTT_USWC;

	bo->tbo.bdev = &adev->mman.bdev;
	gsgpu_bo_placement_from_domain(bo, bp->domain);
	if (bp->type == ttm_bo_type_kernel)
		bo->tbo.priority = 1;

	r = ttm_bo_init_reserved(&adev->mman.bdev, &bo->tbo, size, bp->type,
				 &bo->placement, page_align, &ctx, acc_size,
				 NULL, bp->resv, &gsgpu_bo_destroy);
	if (unlikely(r != 0))
		return r;

	bo->gem_base.resv = bo->tbo.base.resv;

	if (!gsgpu_gmc_vram_full_visible(&adev->gmc) &&
	    bo->tbo.resource->mem_type == TTM_PL_VRAM &&
	    bo->tbo.resource->start < adev->gmc.visible_vram_size >> PAGE_SHIFT)
		gsgpu_cs_report_moved_bytes(adev, ctx.bytes_moved,
					     ctx.bytes_moved);
	else
		gsgpu_cs_report_moved_bytes(adev, ctx.bytes_moved, 0);

	if (bp->flags & GSGPU_GEM_CREATE_VRAM_CLEARED &&
	    bo->tbo.resource->placement & TTM_PL_FLAG_VRAM) {
		struct dma_fence *fence;

		r = gsgpu_fill_buffer(bo, 0, bo->tbo.base.resv, &fence);
		if (unlikely(r))
			goto fail_unreserve;

		gsgpu_bo_fence(bo, fence, false);
		dma_fence_put(bo->tbo.moving);
		bo->tbo.moving = dma_fence_get(fence);
		dma_fence_put(fence);
	}

	if (!bp->resv)
		gsgpu_bo_unreserve(bo);
	*bo_ptr = bo;

	trace_gsgpu_bo_create(bo);

	/* Treat CPU_ACCESS_REQUIRED only as a hint if given by UMD */
	if (bp->type == ttm_bo_type_device)
		bo->flags &= ~GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED;

	return 0;

fail_unreserve:
	if (!bp->resv)
		ww_mutex_unlock(&bo->tbo.base.resv->lock);
	gsgpu_bo_unref(&bo);
	return r;
}

static int gsgpu_bo_create_shadow(struct gsgpu_device *adev,
				   unsigned long size, int byte_align,
				   struct gsgpu_bo *bo)
{
	struct gsgpu_bo_param bp;
	int r;

	if (bo->shadow)
		return 0;

	memset(&bp, 0, sizeof(bp));
	bp.size = size;
	bp.byte_align = byte_align;
	bp.domain = GSGPU_GEM_DOMAIN_GTT;
	bp.flags = GSGPU_GEM_CREATE_CPU_GTT_USWC |
		GSGPU_GEM_CREATE_SHADOW;
	bp.type = ttm_bo_type_kernel;
	bp.resv = bo->tbo.base.resv;

	r = gsgpu_bo_do_create(adev, &bp, &bo->shadow);
	if (!r) {
		bo->shadow->parent = gsgpu_bo_ref(bo);
		mutex_lock(&adev->shadow_list_lock);
		list_add_tail(&bo->shadow_list, &adev->shadow_list);
		mutex_unlock(&adev->shadow_list_lock);
	}

	return r;
}

/**
 * gsgpu_bo_create - create an &gsgpu_bo buffer object
 * @adev: gsgpu device object
 * @bp: parameters to be used for the buffer object
 * @bo_ptr: pointer to the buffer object pointer
 *
 * Creates an &gsgpu_bo buffer object; and if requested, also creates a
 * shadow object.
 * Shadow object is used to backup the original buffer object, and is always
 * in GTT.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_create(struct gsgpu_device *adev,
		     struct gsgpu_bo_param *bp,
		     struct gsgpu_bo **bo_ptr)
{
	u64 flags = bp->flags;
	int r;

	bp->flags = bp->flags & ~GSGPU_GEM_CREATE_SHADOW;
	r = gsgpu_bo_do_create(adev, bp, bo_ptr);
	if (r)
		return r;

	if ((flags & GSGPU_GEM_CREATE_SHADOW) && gsgpu_bo_need_backup(adev)) {
		if (!bp->resv)
			WARN_ON(dma_resv_lock((*bo_ptr)->tbo.base.resv,
							NULL));

		r = gsgpu_bo_create_shadow(adev, bp->size, bp->byte_align, (*bo_ptr));

		if (!bp->resv)
			dma_resv_unlock((*bo_ptr)->tbo.base.resv);

		if (r)
			gsgpu_bo_unref(bo_ptr);
	}

	return r;
}

/**
 * gsgpu_bo_backup_to_shadow - Backs up an &gsgpu_bo buffer object
 * @adev: gsgpu device object
 * @ring: gsgpu_ring for the engine handling the buffer operations
 * @bo: &gsgpu_bo buffer to be backed up
 * @resv: reservation object with embedded fence
 * @fence: dma_fence associated with the operation
 * @direct: whether to submit the job directly
 *
 * Copies an &gsgpu_bo buffer object to its shadow object.
 * Not used for now.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_backup_to_shadow(struct gsgpu_device *adev,
			       struct gsgpu_ring *ring,
			       struct gsgpu_bo *bo,
			       struct dma_resv *resv,
			       struct dma_fence **fence,
			       bool direct)

{
	struct gsgpu_bo *shadow = bo->shadow;
	uint64_t bo_addr, shadow_addr;
	int r;

	if (!shadow)
		return -EINVAL;

	bo_addr = gsgpu_bo_gpu_offset(bo);
	shadow_addr = gsgpu_bo_gpu_offset(bo->shadow);

	r = dma_resv_reserve_shared(bo->tbo.base.resv);
	if (r)
		goto err;

	r = gsgpu_copy_buffer(ring, bo_addr, shadow_addr,
			       gsgpu_bo_size(bo), resv, fence,
			       direct, false);
	if (!r)
		gsgpu_bo_fence(bo, *fence, true);

err:
	return r;
}

/**
 * gsgpu_bo_validate - validate an &gsgpu_bo buffer object
 * @bo: pointer to the buffer object
 *
 * Sets placement according to domain; and changes placement and caching
 * policy of the buffer object according to the placement.
 * This is used for validating shadow bos.  It calls ttm_bo_validate() to
 * make sure the buffer is resident where it needs to be.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_validate(struct gsgpu_bo *bo)
{
	struct ttm_operation_ctx ctx = { false, false };
	uint32_t domain;
	int r;

	if (bo->tbo.pin_count)
		return 0;

	domain = bo->preferred_domains;

retry:
	gsgpu_bo_placement_from_domain(bo, domain);
	r = ttm_bo_validate(&bo->tbo, &bo->placement, &ctx);
	if (unlikely(r == -ENOMEM) && domain != bo->allowed_domains) {
		domain = bo->allowed_domains;
		goto retry;
	}

	return r;
}

/**
 * gsgpu_bo_restore_from_shadow - restore an &gsgpu_bo buffer object
 * @adev: gsgpu device object
 * @ring: gsgpu_ring for the engine handling the buffer operations
 * @bo: &gsgpu_bo buffer to be restored
 * @resv: reservation object with embedded fence
 * @fence: dma_fence associated with the operation
 * @direct: whether to submit the job directly
 *
 * Copies a buffer object's shadow content back to the object.
 * This is used for recovering a buffer from its shadow in case of a gpu
 * reset where vram context may be lost.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_restore_from_shadow(struct gsgpu_device *adev,
				  struct gsgpu_ring *ring,
				  struct gsgpu_bo *bo,
				  struct dma_resv *resv,
				  struct dma_fence **fence,
				  bool direct)

{
	struct gsgpu_bo *shadow = bo->shadow;
	uint64_t bo_addr, shadow_addr;
	int r;

	if (!shadow)
		return -EINVAL;

	bo_addr = gsgpu_bo_gpu_offset(bo);
	shadow_addr = gsgpu_bo_gpu_offset(bo->shadow);

	r = dma_resv_reserve_shared(bo->tbo.base.resv);
	if (r)
		goto err;

	r = gsgpu_copy_buffer(ring, shadow_addr, bo_addr,
			       gsgpu_bo_size(bo), resv, fence,
			       direct, false);
	if (!r)
		gsgpu_bo_fence(bo, *fence, true);

err:
	return r;
}

/**
 * gsgpu_bo_kmap - map an &gsgpu_bo buffer object
 * @bo: &gsgpu_bo buffer object to be mapped
 * @ptr: kernel virtual address to be returned
 *
 * Calls ttm_bo_kmap() to set up the kernel virtual mapping; calls
 * gsgpu_bo_kptr() to get the kernel virtual address.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_kmap(struct gsgpu_bo *bo, void **ptr)
{
	void *kptr;
	long r;

	if (bo->flags & GSGPU_GEM_CREATE_NO_CPU_ACCESS)
		return -EPERM;

	kptr = gsgpu_bo_kptr(bo);
	if (kptr) {
		if (ptr)
			*ptr = kptr;
		return 0;
	}

	r = dma_resv_wait_timeout_rcu(bo->tbo.base.resv, false, false,
						MAX_SCHEDULE_TIMEOUT);
	if (r < 0)
		return r;

	r = ttm_bo_kmap(&bo->tbo, 0, bo->tbo.ttm->num_pages, &bo->kmap);
	if (r)
		return r;

	if (ptr)
		*ptr = gsgpu_bo_kptr(bo);

	return 0;
}

/**
 * gsgpu_bo_kptr - returns a kernel virtual address of the buffer object
 * @bo: &gsgpu_bo buffer object
 *
 * Calls ttm_kmap_obj_virtual() to get the kernel virtual address
 *
 * Returns:
 * the virtual address of a buffer object area.
 */
void *gsgpu_bo_kptr(struct gsgpu_bo *bo)
{
	bool is_iomem;

	return ttm_kmap_obj_virtual(&bo->kmap, &is_iomem);
}

/**
 * gsgpu_bo_kunmap - unmap an &gsgpu_bo buffer object
 * @bo: &gsgpu_bo buffer object to be unmapped
 *
 * Unmaps a kernel map set up by gsgpu_bo_kmap().
 */
void gsgpu_bo_kunmap(struct gsgpu_bo *bo)
{
	if (bo->kmap.bo)
		ttm_bo_kunmap(&bo->kmap);
}

/**
 * gsgpu_bo_ref - reference an &gsgpu_bo buffer object
 * @bo: &gsgpu_bo buffer object
 *
 * References the contained &ttm_buffer_object.
 *
 * Returns:
 * a refcounted pointer to the &gsgpu_bo buffer object.
 */
struct gsgpu_bo *gsgpu_bo_ref(struct gsgpu_bo *bo)
{
	if (bo == NULL)
		return NULL;

	ttm_bo_get(&bo->tbo);
	return bo;
}

/**
 * gsgpu_bo_unref - unreference an &gsgpu_bo buffer object
 * @bo: &gsgpu_bo buffer object
 *
 * Unreferences the contained &ttm_buffer_object and clear the pointer
 */
void gsgpu_bo_unref(struct gsgpu_bo **bo)
{
	struct ttm_buffer_object *tbo;

	if ((*bo) == NULL)
		return;

	tbo = &((*bo)->tbo);
	ttm_bo_put(tbo);
	*bo = NULL;
}

/**
 * gsgpu_bo_pin_restricted - pin an &gsgpu_bo buffer object
 * @bo: &gsgpu_bo buffer object to be pinned
 * @domain: domain to be pinned to
 * @min_offset: the start of requested address range
 * @max_offset: the end of requested address range
 *
 * Pins the buffer object according to requested domain and address range. If
 * the memory is unbound gart memory, binds the pages into gart table. Adjusts
 * pin_count and pin_size accordingly.
 *
 * Pinning means to lock pages in memory along with keeping them at a fixed
 * offset. It is required when a buffer can not be moved, for example, when
 * a display buffer is being scanned out.
 *
 * Compared with gsgpu_bo_pin(), this function gives more flexibility on
 * where to pin a buffer if there are specific restrictions on where a buffer
 * must be located.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_pin_restricted(struct gsgpu_bo *bo, u32 domain,
			     u64 min_offset, u64 max_offset)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	struct ttm_operation_ctx ctx = { false, false };
	int r, i;

	if (gsgpu_ttm_tt_get_usermm(bo->tbo.ttm))
		return -EPERM;

	if (WARN_ON_ONCE(min_offset > max_offset))
		return -EINVAL;

	/* A shared bo cannot be migrated to VRAM */
	if (bo->prime_shared_count) {
		if (domain & GSGPU_GEM_DOMAIN_GTT)
			domain = GSGPU_GEM_DOMAIN_GTT;
		else
			return -EINVAL;
	}

	/* This assumes only APU display buffers are pinned with (VRAM|GTT).
	 * See function gsgpu_display_supported_domains()
	 */
	domain = gsgpu_bo_get_preferred_pin_domain(adev, domain);

	if (bo->tbo.pin_count) {
		uint32_t mem_type = bo->tbo.resource->mem_type;

		if (!(domain & gsgpu_mem_type_to_domain(mem_type)))
			return -EINVAL;

		ttm_bo_pin(&bo->tbo);

		if (max_offset != 0) {
			u64 domain_start = gsgpu_ttm_domain_start(adev, mem_type);
			WARN_ON_ONCE(max_offset < (gsgpu_bo_gpu_offset(bo) - domain_start));
		}

		return 0;
	}

	bo->flags |= GSGPU_GEM_CREATE_VRAM_CONTIGUOUS;
	/* force to pin into visible video ram */
	if (!(bo->flags & GSGPU_GEM_CREATE_NO_CPU_ACCESS))
		bo->flags |= GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED;
	gsgpu_bo_placement_from_domain(bo, domain);
	for (i = 0; i < bo->placement.num_placement; i++) {
		unsigned fpfn, lpfn;

		fpfn = min_offset >> PAGE_SHIFT;
		lpfn = max_offset >> PAGE_SHIFT;

		if (fpfn > bo->placements[i].fpfn)
			bo->placements[i].fpfn = fpfn;
		if (!bo->placements[i].lpfn ||
		    (lpfn && lpfn < bo->placements[i].lpfn))
			bo->placements[i].lpfn = lpfn;
	}

	r = ttm_bo_validate(&bo->tbo, &bo->placement, &ctx);
	if (unlikely(r)) {
		dev_err(adev->dev, "%p pin failed\n", bo);
		goto error;
	}

	ttm_bo_pin(&bo->tbo);

	domain = gsgpu_mem_type_to_domain(bo->tbo.resource->mem_type);
	if (domain == GSGPU_GEM_DOMAIN_VRAM) {
		atomic64_add(gsgpu_bo_size(bo), &adev->vram_pin_size);
		atomic64_add(gsgpu_vram_mgr_bo_visible_size(bo),
			     &adev->visible_pin_size);
	} else if (domain == GSGPU_GEM_DOMAIN_GTT) {
		atomic64_add(gsgpu_bo_size(bo), &adev->gart_pin_size);
	}

error:
	return r;
}

/**
 * gsgpu_bo_pin - pin an &gsgpu_bo buffer object
 * @bo: &gsgpu_bo buffer object to be pinned
 * @domain: domain to be pinned to
 *
 * A simple wrapper to gsgpu_bo_pin_restricted().
 * Provides a simpler API for buffers that do not have any strict restrictions
 * on where a buffer must be located.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_pin(struct gsgpu_bo *bo, u32 domain)
{
	return gsgpu_bo_pin_restricted(bo, domain, 0, 0);
}

/**
 * gsgpu_bo_unpin - unpin an &gsgpu_bo buffer object
 * @bo: &gsgpu_bo buffer object to be unpinned
 *
 * Decreases the pin_count, and clears the flags if pin_count reaches 0.
 * Changes placement and pin size accordingly.
 */
void gsgpu_bo_unpin(struct gsgpu_bo *bo)
{
	ttm_bo_unpin(&bo->tbo);
	if (bo->tbo.pin_count)
		return;

	gsgpu_bo_subtract_pin_size(bo);
}

/**
 * gsgpu_bo_evict_vram - evict VRAM buffers
 * @adev: gsgpu device object
 *
 * Evicts all VRAM buffers on the lru list of the memory type.
 * Mainly used for evicting vram at suspend time.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_evict_vram(struct gsgpu_device *adev)
{
	/* late 2.6.33 fix IGP hibernate - we need pm ops to do this correct */
	if (0 && (adev->flags & GSGPU_IS_APU)) {
		/* Useless to evict on IGP chips */
		return 0;
	}
	return ttm_bo_evict_mm(&adev->mman.bdev, TTM_PL_VRAM);
}

static const char *gsgpu_vram_names[] = {
	"UNKNOWN",
	"GDDR1",
	"DDR2",
	"GDDR3",
	"GDDR4",
	"GDDR5",
	"HBM",
	"DDR3",
	"DDR4",
};

/**
 * gsgpu_bo_init - initialize memory manager
 * @adev: gsgpu device object
 *
 * Calls gsgpu_ttm_init() to initialize gsgpu memory manager.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_init(struct gsgpu_device *adev)
{
	/* reserve PAT memory space to WC for VRAM */
	arch_io_reserve_memtype_wc(adev->gmc.aper_base,
				   adev->gmc.aper_size);

	/* Add an MTRR for the VRAM */
	adev->gmc.vram_mtrr = arch_phys_wc_add(adev->gmc.aper_base,
					      adev->gmc.aper_size);
	DRM_INFO("Detected VRAM RAM=%lluM, BAR=%lluM\n",
		 adev->gmc.mc_vram_size >> 20,
		 (unsigned long long)adev->gmc.aper_size >> 20);
	DRM_INFO("RAM width %dbits %s\n",
		 adev->gmc.vram_width, gsgpu_vram_names[adev->gmc.vram_type]);
	return gsgpu_ttm_init(adev);
}

/**
 * gsgpu_bo_late_init - late init
 * @adev: gsgpu device object
 *
 * Calls gsgpu_ttm_late_init() to free resources used earlier during
 * initialization.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_late_init(struct gsgpu_device *adev)
{
	gsgpu_ttm_late_init(adev);

	return 0;
}

/**
 * gsgpu_bo_fini - tear down memory manager
 * @adev: gsgpu device object
 *
 * Reverses gsgpu_bo_init() to tear down memory manager.
 */
void gsgpu_bo_fini(struct gsgpu_device *adev)
{
	gsgpu_ttm_fini(adev);
	arch_phys_wc_del(adev->gmc.vram_mtrr);
	arch_io_free_memtype_wc(adev->gmc.aper_base, adev->gmc.aper_size);
}

/**
 * gsgpu_bo_fbdev_mmap - mmap fbdev memory
 * @bo: &gsgpu_bo buffer object
 * @vma: vma as input from the fbdev mmap method
 *
 * Calls ttm_fbdev_mmap() to mmap fbdev memory if it is backed by a bo.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_fbdev_mmap(struct gsgpu_bo *bo,
			     struct vm_area_struct *vma)
{
	return ttm_fbdev_mmap(vma, &bo->tbo);
}

/**
 * gsgpu_bo_set_tiling_flags - set tiling flags
 * @bo: &gsgpu_bo buffer object
 * @tiling_flags: new flags
 *
 * Sets buffer object's tiling flags with the new one. Used by GEM ioctl or
 * kernel driver to set the tiling flags on a buffer.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_set_tiling_flags(struct gsgpu_bo *bo, u64 tiling_flags)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);

	if (adev->family <= GSGPU_FAMILY_CZ &&
	    GSGPU_TILING_GET(tiling_flags, TILE_SPLIT) > 6)
		return -EINVAL;

	bo->tiling_flags = tiling_flags;
	return 0;
}

/**
 * gsgpu_bo_get_tiling_flags - get tiling flags
 * @bo: &gsgpu_bo buffer object
 * @tiling_flags: returned flags
 *
 * Gets buffer object's tiling flags. Used by GEM ioctl or kernel driver to
 * set the tiling flags on a buffer.
 */
void gsgpu_bo_get_tiling_flags(struct gsgpu_bo *bo, u64 *tiling_flags)
{
	lockdep_assert_held(&bo->tbo.base.resv->lock.base);

	if (tiling_flags)
		*tiling_flags = bo->tiling_flags;
}

/**
 * gsgpu_bo_set_metadata - set metadata
 * @bo: &gsgpu_bo buffer object
 * @metadata: new metadata
 * @metadata_size: size of the new metadata
 * @flags: flags of the new metadata
 *
 * Sets buffer object's metadata, its size and flags.
 * Used via GEM ioctl.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_set_metadata (struct gsgpu_bo *bo, void *metadata,
			    uint32_t metadata_size, uint64_t flags)
{
	void *buffer;

	if (!metadata_size) {
		if (bo->metadata_size) {
			kfree(bo->metadata);
			bo->metadata = NULL;
			bo->metadata_size = 0;
		}
		return 0;
	}

	if (metadata == NULL)
		return -EINVAL;

	buffer = kmemdup(metadata, metadata_size, GFP_KERNEL);
	if (buffer == NULL)
		return -ENOMEM;

	kfree(bo->metadata);
	bo->metadata_flags = flags;
	bo->metadata = buffer;
	bo->metadata_size = metadata_size;

	return 0;
}

/**
 * gsgpu_bo_get_metadata - get metadata
 * @bo: &gsgpu_bo buffer object
 * @buffer: returned metadata
 * @buffer_size: size of the buffer
 * @metadata_size: size of the returned metadata
 * @flags: flags of the returned metadata
 *
 * Gets buffer object's metadata, its size and flags. buffer_size shall not be
 * less than metadata_size.
 * Used via GEM ioctl.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
int gsgpu_bo_get_metadata(struct gsgpu_bo *bo, void *buffer,
			   size_t buffer_size, uint32_t *metadata_size,
			   uint64_t *flags)
{
	if (!buffer && !metadata_size)
		return -EINVAL;

	if (buffer) {
		if (buffer_size < bo->metadata_size)
			return -EINVAL;

		if (bo->metadata_size)
			memcpy(buffer, bo->metadata, bo->metadata_size);
	}

	if (metadata_size)
		*metadata_size = bo->metadata_size;
	if (flags)
		*flags = bo->metadata_flags;

	return 0;
}

/**
 * gsgpu_bo_move_notify - notification about a memory move
 * @bo: pointer to a buffer object
 * @evict: if this move is evicting the buffer from the graphics address space
 * @new_mem: new information of the bufer object
 *
 * Marks the corresponding &gsgpu_bo buffer object as invalid, also performs
 * bookkeeping.
 * TTM driver callback which is called when ttm moves a buffer.
 */
void gsgpu_bo_move_notify(struct ttm_buffer_object *bo,
			   bool evict,
			   struct ttm_resource *new_mem)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->bdev);
	struct gsgpu_bo *abo;
	struct ttm_resource *old_mem = bo->resource;

	if (!gsgpu_bo_is_gsgpu_bo(bo))
		return;

	abo = ttm_to_gsgpu_bo(bo);
	gsgpu_vm_bo_invalidate(adev, abo, evict);

	gsgpu_bo_kunmap(abo);

	/* remember the eviction */
	if (evict)
		atomic64_inc(&adev->num_evictions);

	/* update statistics */
	if (!new_mem)
		return;

	/* move_notify is called before move happens */
	trace_gsgpu_bo_move(abo, new_mem->mem_type, old_mem->mem_type);
}

/**
 * gsgpu_bo_fault_reserve_notify - notification about a memory fault
 * @bo: pointer to a buffer object
 *
 * Notifies the driver we are taking a fault on this BO and have reserved it,
 * also performs bookkeeping.
 * TTM driver callback for dealing with vm faults.
 *
 * Returns:
 * 0 for success or a negative error code on failure.
 */
vm_fault_t gsgpu_bo_fault_reserve_notify(struct ttm_buffer_object *bo)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->bdev);
	struct ttm_operation_ctx ctx = { false, false };
	struct gsgpu_bo *abo = ttm_to_gsgpu_bo(bo);
	unsigned long offset, size;
	int r;

	/* Remember that this BO was accessed by the CPU */
	abo->flags |= GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED;

	if (bo->resource->mem_type != TTM_PL_VRAM)
		return 0;

	size = bo->ttm->num_pages << PAGE_SHIFT;
	offset = bo->resource->start << PAGE_SHIFT;
	if ((offset + size) <= adev->gmc.visible_vram_size)
		return 0;

	/* Can't move a pinned BO to visible VRAM */
	if (abo->pin_count > 0)
		return VM_FAULT_SIGBUS;

	/* hurrah the memory is not visible ! */
	atomic64_inc(&adev->num_vram_cpu_page_faults);
	gsgpu_bo_placement_from_domain(abo, GSGPU_GEM_DOMAIN_VRAM |
					GSGPU_GEM_DOMAIN_GTT);

	/* Avoid costly evictions; only set GTT as a busy placement */
	abo->placement.num_busy_placement = 1;
	abo->placement.busy_placement = &abo->placements[1];

	r = ttm_bo_validate(bo, &abo->placement, &ctx);
	if (unlikely(r == -EBUSY || r == -ERESTARTSYS))
		return VM_FAULT_NOPAGE;
	else if (unlikely(r))
		return VM_FAULT_SIGBUS;

	offset = bo->resource->start << PAGE_SHIFT;
	/* this should never happen */
	if (bo->resource->mem_type == TTM_PL_VRAM &&
	    (offset + size) > adev->gmc.visible_vram_size)
		return VM_FAULT_SIGBUS;

	ttm_bo_move_to_lru_tail_unlocked(bo);
	return 0;
}

/**
 * gsgpu_bo_fence - add fence to buffer object
 *
 * @bo: buffer object in question
 * @fence: fence to add
 * @shared: true if fence should be added shared
 *
 */
void gsgpu_bo_fence(struct gsgpu_bo *bo, struct dma_fence *fence,
		     bool shared)
{
	struct dma_resv *resv = bo->tbo.base.resv;

	if (shared)
		dma_resv_add_shared_fence(resv, fence);
	else
		dma_resv_add_excl_fence(resv, fence);
}

/**
 * gsgpu_bo_gpu_offset - return GPU offset of bo
 * @bo:	gsgpu object for which we query the offset
 *
 * Note: object should either be pinned or reserved when calling this
 * function, it might be useful to add check for this for debugging.
 *
 * Returns:
 * current GPU offset of the object.
 */
u64 gsgpu_bo_gpu_offset(struct gsgpu_bo *bo)
{
	WARN_ON_ONCE(bo->tbo.resource->mem_type == TTM_PL_SYSTEM);
	WARN_ON_ONCE(bo->tbo.resource->mem_type == TTM_PL_TT &&
		     !gsgpu_gtt_mgr_has_gart_addr(bo->tbo.resource));
	WARN_ON_ONCE(!ww_mutex_is_locked(&bo->tbo.base.resv->lock) &&
		     !bo->tbo.pin_count);
	WARN_ON_ONCE(bo->tbo.resource->start == GSGPU_BO_INVALID_OFFSET);
	WARN_ON_ONCE(bo->tbo.resource->mem_type == TTM_PL_VRAM &&
		     !(bo->flags & GSGPU_GEM_CREATE_VRAM_CONTIGUOUS));

	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	uint64_t offset = (bo->tbo.resource->start << PAGE_SHIFT) +
                gsgpu_ttm_domain_start(adev, bo->tbo.resource->mem_type);

	return offset;
}

/**
 * gsgpu_bo_get_preferred_pin_domain - get preferred domain for scanout
 * @adev: gsgpu device object
 * @domain: allowed :ref:`memory domains <gsgpu_memory_domains>`
 *
 * Returns:
 * Which of the allowed domains is preferred for pinning the BO for scanout.
 */
uint32_t gsgpu_bo_get_preferred_pin_domain(struct gsgpu_device *adev,
					    uint32_t domain)
{
	if (domain == (GSGPU_GEM_DOMAIN_VRAM | GSGPU_GEM_DOMAIN_GTT)) {
		domain = GSGPU_GEM_DOMAIN_VRAM;
		if (adev->gmc.real_vram_size <= GSGPU_SG_THRESHOLD)
			domain = GSGPU_GEM_DOMAIN_GTT;
	}
	return domain;
}

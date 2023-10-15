#include <drm/ttm/ttm_bo.h>
#include <drm/ttm/ttm_placement.h>
#include <drm/drm_cache.h>
#include <drm/gsgpu_drm.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/swiotlb.h>
#include <linux/swap.h>
#include <linux/pagemap.h>
#include <linux/debugfs.h>
#include <linux/iommu.h>
#include <linux/hmm.h>
#include "gsgpu.h"
#include "gsgpu_object.h"
#include "gsgpu_trace.h"

/**TODO
 *Need fixed this, mmap ops had access handler.
**/
#define mmMM_INDEX         0x0
#define mmMM_INDEX_HI      0x6
#define mmMM_DATA          0x1

static int gsgpu_ttm_backend_bind(struct ttm_device *bdev,
				  struct ttm_tt *ttm,
				  struct ttm_resource *bo_mem);
static int gsgpu_ttm_backend_unbind(struct ttm_device *bdev,
				    struct ttm_tt *ttm);
static int gsgpu_ttm_debugfs_init(struct gsgpu_device *adev);
static void gsgpu_ttm_debugfs_fini(struct gsgpu_device *adev);

#define ttm_to_gsgpu_ttm_tt(ptr)	container_of(ptr, struct gsgpu_ttm_tt, ttm)

/**
 * gsgpu_evict_flags - Compute placement flags
 *
 * @bo: The buffer object to evict
 * @placement: Possible destination(s) for evicted BO
 *
 * Fill in placement data when ttm_bo_evict() is called
 */
static void gsgpu_evict_flags(struct ttm_buffer_object *bo,
			      struct ttm_placement *placement)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->bdev);
	struct gsgpu_bo *abo;
	static const struct ttm_place placements = {
		.fpfn = 0,
		.lpfn = 0,
		.flags = 0
	};

	/* Don't handle scatter gather BOs */
	if (bo->type == ttm_bo_type_sg) {
		placement->num_placement = 0;
		placement->num_busy_placement = 0;
		return;
	}

	/* Object isn't an GSGPU object so ignore */
	if (!gsgpu_bo_is_gsgpu_bo(bo)) {
		placement->placement = &placements;
		placement->busy_placement = &placements;
		placement->num_placement = 1;
		placement->num_busy_placement = 1;
		return;
	}

	abo = ttm_to_gsgpu_bo(bo);
	switch (bo->resource->mem_type) {
	case TTM_PL_VRAM:
		if (!adev->mman.buffer_funcs_enabled) {
			/* Move to system memory */
			gsgpu_bo_placement_from_domain(abo, GSGPU_GEM_DOMAIN_CPU);
		} else if (!gsgpu_gmc_vram_full_visible(&adev->gmc) &&
			   !(abo->flags & GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED) &&
			   gsgpu_bo_in_cpu_visible_vram(abo)) {

			/* Try evicting to the CPU inaccessible part of VRAM
			 * first, but only set GTT as busy placement, so this
			 * BO will be evicted to GTT rather than causing other
			 * BOs to be evicted from VRAM
			 */
			gsgpu_bo_placement_from_domain(abo, GSGPU_GEM_DOMAIN_VRAM |
						       GSGPU_GEM_DOMAIN_GTT);
			abo->placements[0].fpfn = adev->gmc.visible_vram_size >> PAGE_SHIFT;
			abo->placements[0].lpfn = 0;
			abo->placement.busy_placement = &abo->placements[1];
			abo->placement.num_busy_placement = 1;
		} else {
			/* Move to GTT memory */
			gsgpu_bo_placement_from_domain(abo, GSGPU_GEM_DOMAIN_GTT);
		}
		break;
	case TTM_PL_TT:
	default:
		gsgpu_bo_placement_from_domain(abo, GSGPU_GEM_DOMAIN_CPU);
	}
	*placement = abo->placement;
}

/**
 * gsgpu_ttm_domain_start - Returns GPU start address
 * @adev: gsgpu device object
 * @type: type of the memory
 *
 * Returns:
 * GPU start address of a memory domain
 */
uint64_t gsgpu_ttm_domain_start(struct gsgpu_device *adev, uint32_t type)
{
	switch (type) {
	case TTM_PL_TT:
		return adev->gmc.gart_start;
	case TTM_PL_VRAM:
		return adev->gmc.vram_start;
	}

	return 0;
}

/**
 * gsgpu_ttm_map_buffer - Map memory into the GART windows
 * @bo: buffer object to map
 * @mem: memory object to map
 * @mm_cur: range to map
 * @num_pages: number of pages to map
 * @window: which GART window to use
 * @ring: DMA ring to use for the copy
 * @addr: resulting address inside the MC address space
 *
 * Setup one of the GART windows to access a specific piece of memory or return
 * the physical address for local memory.
 */
static int gsgpu_ttm_map_buffer(struct ttm_buffer_object *bo,
				struct ttm_resource *mem,
				struct gsgpu_res_cursor *mm_cur,
				unsigned num_pages, unsigned window,
				struct gsgpu_ring *ring,
				uint64_t *addr)
{
	struct gsgpu_device *adev = ring->adev;
	struct gsgpu_job *job;
	unsigned num_dw, num_bytes;
	struct dma_fence *fence;
	uint64_t src_addr, dst_addr;
	void *cpu_addr;
        uint64_t flags;
	unsigned int i;
	int r;

	BUG_ON(adev->mman.buffer_funcs->copy_max_bytes <
	       GSGPU_GTT_MAX_TRANSFER_SIZE * 8);

	/* Map only what can't be accessed directly */
	if (mem->start != GSGPU_BO_INVALID_OFFSET) {
		*addr = gsgpu_ttm_domain_start(adev, mem->mem_type) + mm_cur->start;
		return 0;
	}

	*addr = adev->gmc.gart_start;
	*addr += (u64)window * GSGPU_GTT_MAX_TRANSFER_SIZE *
		GSGPU_GPU_PAGE_SIZE;
	*addr += mm_cur->start & ~PAGE_MASK;

	num_dw = ALIGN(adev->mman.buffer_funcs->copy_num_dw, 8);
	num_bytes = num_pages * 8;

	r = gsgpu_job_alloc_with_ib(adev, num_dw * 4 + num_bytes, &job);
	if (r)
		return r;

	src_addr = num_dw * 4;
	src_addr += job->ibs[0].gpu_addr;

	dst_addr = adev->gart.table_addr;
	dst_addr += window * GSGPU_GTT_MAX_TRANSFER_SIZE * 8;
	gsgpu_emit_copy_buffer(adev, &job->ibs[0], src_addr,
			       dst_addr, num_bytes);

	gsgpu_ring_pad_ib(ring, &job->ibs[0]);
	WARN_ON(job->ibs[0].length_dw > num_dw);

	flags = gsgpu_ttm_tt_pte_flags(adev, bo->ttm, mem);

	cpu_addr = &job->ibs[0].ptr[num_dw];

	if (mem->mem_type == TTM_PL_TT) {
		dma_addr_t *dma_addr;
		dma_addr = &bo->ttm->dma_address[mm_cur->start >> PAGE_SHIFT];
		r = gsgpu_gart_map(adev, 0, num_pages, dma_addr, flags, cpu_addr);
		if (r)
			goto error_free;
	} else {
		dma_addr_t dma_address = mm_cur->start;
		dma_address += adev->vm_manager.vram_base_offset;

		for (i = 0; i < num_pages; ++i) {
			r = gsgpu_gart_map(adev, i << PAGE_SHIFT, 1,
					   &dma_address, flags, cpu_addr);
			if (r)
				goto error_free;

			dma_address += PAGE_SIZE;
		}
	}

	r = gsgpu_job_submit(job, &adev->mman.entity,
			     GSGPU_FENCE_OWNER_UNDEFINED, &fence);
	if (r)
		goto error_free;

	dma_fence_put(fence);

	return r;

error_free:
	gsgpu_job_free(job);
	return r;
}

/**
 * gsgpu_copy_ttm_mem_to_mem - Helper function for copy
 *
 * The function copies @size bytes from {src->mem + src->offset} to
 * {dst->mem + dst->offset}. src->bo and dst->bo could be same BO for a
 * move and different for a BO to BO copy.
 *
 * @f: Returns the last fence if multiple jobs are submitted.
 */
int gsgpu_ttm_copy_mem_to_mem(struct gsgpu_device *adev,
			      const struct gsgpu_copy_mem *src,
			      const struct gsgpu_copy_mem *dst,
			      uint64_t size,
			      struct dma_resv *resv,
			      struct dma_fence **f)
{
	const uint32_t GTT_MAX_BYTES = (GSGPU_GTT_MAX_TRANSFER_SIZE *
					GSGPU_GPU_PAGE_SIZE);

	struct gsgpu_ring *ring = adev->mman.buffer_funcs_ring;
	struct gsgpu_res_cursor src_mm, dst_mm;
	struct dma_fence *fence = NULL;
	int r = 0;

	if (!adev->mman.buffer_funcs_enabled) {
		DRM_ERROR("Trying to move memory with ring turned off.\n");
		return -EINVAL;
	}

	gsgpu_res_first(src->mem, src->offset, size, &src_mm);
	gsgpu_res_first(dst->mem, dst->offset, size, &dst_mm);

	mutex_lock(&adev->mman.gtt_window_lock);

	while (src_mm.remaining) {
		uint32_t src_page_offset = src_mm.start & ~PAGE_MASK;
		uint32_t dst_page_offset = dst_mm.start & ~PAGE_MASK;
		struct dma_fence *next;
		uint32_t cur_size;
		uint64_t from, to;

		/* Copy size cannot exceed GTT_MAX_BYTES. So if src or dst
		 * begins at an offset, then adjust the size accordingly
		 */
		cur_size = min3(src_mm.size, dst_mm.size, size);
		cur_size = min(GTT_MAX_BYTES - src_page_offset, cur_size);
		cur_size = min(GTT_MAX_BYTES - dst_page_offset, cur_size);

		/* Map src to window 0 and dst to window 1. */
		r = gsgpu_ttm_map_buffer(src->bo, src->mem, &src_mm,
					 PFN_UP(cur_size + src_page_offset),
					 0, ring, &from);
		if (r)
			goto error;

		r = gsgpu_ttm_map_buffer(dst->bo, dst->mem, &dst_mm,
					 PFN_UP(cur_size + dst_page_offset),
					 1, ring, &to);
		if (r)
			goto error;

		r = gsgpu_copy_buffer(ring, from, to, cur_size,
				      resv, &next, false, true);
		if (r)
			goto error;

		dma_fence_put(fence);
		fence = next;

		gsgpu_res_next(&src_mm, cur_size);
		gsgpu_res_next(&dst_mm, cur_size);
	}
error:
	mutex_unlock(&adev->mman.gtt_window_lock);
	if (f)
		*f = dma_fence_get(fence);
	dma_fence_put(fence);
	return r;
}

/**
 * gsgpu_move_blit - Copy an entire buffer to another buffer
 *
 * This is a helper called by gsgpu_bo_move() and gsgpu_move_vram_ram() to
 * help move buffers to and from VRAM.
 */
static int gsgpu_move_blit(struct ttm_buffer_object *bo,
			   bool evict, bool no_wait_gpu,
			   struct ttm_resource *new_mem,
			   struct ttm_resource *old_mem)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->bdev);
	struct gsgpu_copy_mem src, dst;
	struct dma_fence *fence = NULL;
	int r;

	src.bo = bo;
	dst.bo = bo;
	src.mem = old_mem;
	dst.mem = new_mem;
	src.offset = 0;
	dst.offset = 0;

	r = gsgpu_ttm_copy_mem_to_mem(adev, &src, &dst,
				      new_mem->size,
				      bo->base.resv, &fence);
	if (r)
		goto error;

	r = ttm_bo_move_accel_cleanup(bo, fence, evict, true, new_mem);
	dma_fence_put(fence);
	return r;

error:
	if (fence)
		dma_fence_wait(fence, false);
	dma_fence_put(fence);
	return r;
}

/**
 * gsgpu_bo_move - Move a buffer object to a new memory location
 *
 * Called by ttm_bo_handle_move_mem()
 */
static int gsgpu_bo_move(struct ttm_buffer_object *bo, bool evict,
			 struct ttm_operation_ctx *ctx,
			 struct ttm_resource *new_mem,
			 struct ttm_place *hop)
{
	struct gsgpu_device *adev;
	struct gsgpu_bo *abo;
	struct ttm_resource *old_mem = bo->resource;
	int r;

	if (new_mem->mem_type == TTM_PL_TT) {
		r = gsgpu_ttm_backend_bind(bo->bdev, bo->ttm, new_mem);
		if (r)
			return r;
	}

	abo = ttm_to_gsgpu_bo(bo);
	/* Can't move a pinned BO */
	if (WARN_ON_ONCE(abo->tbo.pin_count > 0))
		return -EINVAL;

	adev = gsgpu_ttm_adev(bo->bdev);

	if (!old_mem || (old_mem->mem_type == TTM_PL_SYSTEM && bo->ttm == NULL)) {
		ttm_bo_move_null(bo, new_mem);
		goto out;
	}
	if (old_mem->mem_type == TTM_PL_SYSTEM &&
	    new_mem->mem_type == TTM_PL_TT) {
		ttm_bo_move_null(bo, new_mem);
		goto out;
	}
	if (old_mem->mem_type == TTM_PL_TT &&
	    new_mem->mem_type == TTM_PL_SYSTEM) {
		r = ttm_bo_wait_ctx(bo, ctx);
		if (r)
			return r;

		gsgpu_ttm_backend_unbind(bo->bdev, bo->ttm);
		ttm_resource_free(bo, &bo->resource);
		ttm_bo_assign_mem(bo, new_mem);
		goto out;
	}

	if (!adev->mman.buffer_funcs_enabled)
		goto memcpy;

	if ((old_mem->mem_type == TTM_PL_SYSTEM &&
	     new_mem->mem_type == TTM_PL_VRAM) ||
	    (old_mem->mem_type == TTM_PL_VRAM &&
	     new_mem->mem_type == TTM_PL_SYSTEM)) {
		hop->fpfn = 0;
		hop->lpfn = 0;
		hop->mem_type = TTM_PL_TT;
		hop->flags = 0;
		return -EMULTIHOP;
	}

	r = gsgpu_move_blit(bo, evict, ctx->no_wait_gpu, new_mem, old_mem);
	if (r) {
	memcpy:
		r = ttm_bo_move_memcpy(bo, ctx, new_mem);
		if (r) {
			return r;
		}
	}

	if (bo->type == ttm_bo_type_device &&
	    new_mem->mem_type == TTM_PL_VRAM &&
	    old_mem->mem_type != TTM_PL_VRAM) {
		/* gsgpu_bo_fault_reserve_notify will re-set this if the CPU
		 * accesses the BO after it's moved.
		 */
		abo->flags &= ~GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED;
	}

out:
	/* update statistics */
	atomic64_add(bo->base.size, &adev->num_bytes_moved);
    	gsgpu_bo_move_notify(bo, evict, new_mem);
	return 0;
}

/**
 * gsgpu_ttm_io_mem_reserve - Reserve a block of memory during a fault
 *
 * Called by ttm_mem_io_reserve() ultimately via ttm_bo_vm_fault()
 */
static int gsgpu_ttm_io_mem_reserve(struct ttm_device *bdev, struct ttm_resource *mem)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bdev);
	size_t bus_size = (size_t)mem->size;

	switch (mem->mem_type) {
	case TTM_PL_SYSTEM:
		/* system memory */
		return 0;
	case TTM_PL_TT:
		break;
	case TTM_PL_VRAM:
		mem->bus.offset = mem->start << PAGE_SHIFT;
		/* check if it's visible */
		if ((mem->bus.offset + bus_size) > adev->gmc.visible_vram_size)
			return -EINVAL;

		if (adev->mman.aper_base_kaddr &&
		    mem->placement & TTM_PL_FLAG_CONTIGUOUS)
			mem->bus.addr = (u8 *)adev->mman.aper_base_kaddr +
				mem->bus.offset;

		mem->bus.offset += adev->gmc.aper_base;
		mem->bus.is_iomem = true;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void gsgpu_ttm_io_mem_free(struct ttm_device *bdev, struct ttm_resource *mem)
{
}

static unsigned long gsgpu_ttm_io_mem_pfn(struct ttm_buffer_object *bo,
					  unsigned long page_offset)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->bdev);
	struct gsgpu_res_cursor cursor;

	gsgpu_res_first(bo->resource, (u64)page_offset << PAGE_SHIFT, 0, &cursor);
	return (adev->gmc.aper_base + cursor.start) >> PAGE_SHIFT;
}

/*
 * TTM backend functions.
 */
struct gsgpu_ttm_tt {
	struct ttm_tt		ttm;
	u64			offset;
	uint64_t		userptr;
	struct task_struct	*usertask;
	uint32_t		userflags;
	struct hmm_range        *ranges;
	int                     nr_ranges;
};

/* Support Userptr pages cross max 16 vmas */
#define MAX_NR_VMAS    (16)

/**
 * gsgpu_ttm_tt_get_user_pages - get device accessible pages that back user
 * memory and start HMM tracking CPU page table update
 *
 * Calling function must call gsgpu_ttm_tt_userptr_range_done() once and only
 * once afterwards to stop HMM tracking
 */
int gsgpu_ttm_tt_get_user_pages(struct gsgpu_bo *bo, struct page **pages,
				struct hmm_range **range)
{
	struct ttm_tt *ttm = bo->tbo.ttm;
	struct gsgpu_ttm_tt *gtt = ttm_to_gsgpu_ttm_tt(ttm);
	unsigned long start = gtt->userptr;
	struct vm_area_struct *vma;
	struct mm_struct *mm;
	bool readonly;
	int r = 0;

	/* Make sure get_user_pages_done() can cleanup gracefully */
	*range = NULL;

	mm = bo->notifier.mm;
	if (unlikely(!mm)) {
		DRM_DEBUG_DRIVER("BO is not registered?\n");
		return -EFAULT;
	}

	if (!mmget_not_zero(mm)) /* Happens during process shutdown */
		return -ESRCH;

	mmap_read_lock(mm);
	vma = vma_lookup(mm, start);
	if (unlikely(!vma)) {
		r = -EFAULT;
		goto out_unlock;
	}
	if (unlikely((gtt->userflags & GSGPU_GEM_USERPTR_ANONONLY) &&
		     vma->vm_file)) {
		r = -EPERM;
		goto out_unlock;
	}

	readonly = gsgpu_ttm_tt_is_readonly(ttm);
	r = gsgpu_hmm_range_get_pages(&bo->notifier, start, ttm->num_pages,
				      readonly, NULL, pages, range);
out_unlock:
	mmap_read_unlock(mm);
	if (r)
		pr_debug("failed %d to get user pages 0x%lx\n", r, start);

	mmput(mm);

	return r;
}

/**
 * gsgpu_ttm_tt_userptr_range_done - stop HMM track the CPU page table change
 * Check if the pages backing this ttm range have been invalidated
 *
 * Returns: true if pages are still valid
 */
bool gsgpu_ttm_tt_get_user_pages_done(struct ttm_tt *ttm,
				      struct hmm_range *range)
{
	struct gsgpu_ttm_tt *gtt = ttm_to_gsgpu_ttm_tt(ttm);

	if (!gtt || !gtt->userptr || !range)
		return false;

	DRM_DEBUG_DRIVER("user_pages_done 0x%llx pages 0x%x\n",
			 gtt->userptr, ttm->num_pages);

	WARN_ONCE(!range->hmm_pfns, "No user pages to check\n");

	return !gsgpu_hmm_range_get_pages_done(range);
}

/**
 * gsgpu_ttm_tt_set_user_pages - Copy pages in, putting old pages as necessary.
 *
 * Called by gsgpu_cs_list_validate(). This creates the page list
 * that backs user memory and will ultimately be mapped into the device
 * address space.
 */
void gsgpu_ttm_tt_set_user_pages(struct ttm_tt *ttm, struct page **pages)
{
	unsigned i;

	for (i = 0; i < ttm->num_pages; ++i)
		ttm->pages[i] = pages ? pages[i] : NULL;
}

/**
 * gsgpu_ttm_tt_mark_user_page - Mark pages as dirty
 *
 * Called while unpinning userptr pages
 */
void gsgpu_ttm_tt_mark_user_pages(struct ttm_tt *ttm)
{
	struct gsgpu_ttm_tt *gtt = (void *)ttm;
	unsigned i;

	for (i = 0; i < ttm->num_pages; ++i) {
		struct page *page = ttm->pages[i];

		if (!page)
			continue;

		if (!(gtt->userflags & GSGPU_GEM_USERPTR_READONLY))
			set_page_dirty(page);

		mark_page_accessed(page);
	}
}

/**
 * gsgpu_ttm_tt_pin_userptr - 	prepare the sg table with the user pages
 *
 * Called by gsgpu_ttm_backend_bind()
 **/
static int gsgpu_ttm_tt_pin_userptr(struct ttm_device *bdev,
				    struct ttm_tt *ttm)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bdev);
	struct gsgpu_ttm_tt *gtt = ttm_to_gsgpu_ttm_tt(ttm);
	int write = !(gtt->userflags & GSGPU_GEM_USERPTR_READONLY);
	enum dma_data_direction direction = write ?
		DMA_BIDIRECTIONAL : DMA_TO_DEVICE;
	int r;

	/* Allocate an SG array and squash pages into it */
	r = sg_alloc_table_from_pages(ttm->sg, ttm->pages, ttm->num_pages, 0,
				      (u64)ttm->num_pages << PAGE_SHIFT,
				      GFP_KERNEL);
	if (r)
		goto release_sg;

	/* Map SG to device */
	r = dma_map_sgtable(adev->dev, ttm->sg, direction, 0);
	if (r)
		goto release_sg;

	/* convert SG to linear array of pages and dma addresses */
	drm_prime_sg_to_dma_addr_array(ttm->sg, gtt->ttm.dma_address,
				       ttm->num_pages);

	return 0;

release_sg:
	kfree(ttm->sg);
	ttm->sg = NULL;
	return r;
}

/**
 * gsgpu_ttm_tt_unpin_userptr - Unpin and unmap userptr pages
 */
static void gsgpu_ttm_tt_unpin_userptr(struct ttm_device *bdev,
				       struct ttm_tt *ttm)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bdev);
	struct gsgpu_ttm_tt *gtt = ttm_to_gsgpu_ttm_tt(ttm);
	int write = !(gtt->userflags & GSGPU_GEM_USERPTR_READONLY);
	enum dma_data_direction direction = write ?
		DMA_BIDIRECTIONAL : DMA_TO_DEVICE;

	/* double check that we don't free the table twice */
	if (!ttm->sg || !ttm->sg->sgl)
		return;

	/* unmap the pages mapped to the device */
	dma_unmap_sgtable(adev->dev, ttm->sg, direction, 0);
	sg_free_table(ttm->sg);
}

int gsgpu_ttm_gart_bind(struct gsgpu_device *adev,
			struct ttm_buffer_object *tbo,
			uint64_t flags)
{
	struct ttm_tt *ttm = tbo->ttm;
	struct gsgpu_ttm_tt *gtt = (void *)ttm;
	int r;

	r = gsgpu_gart_bind(adev, gtt->offset, ttm->num_pages,
			    ttm->pages, gtt->ttm.dma_address, flags);
	if (r)
		DRM_ERROR("failed to bind %u pages at 0x%08llX\n",
			  ttm->num_pages, gtt->offset);

	return r;
}

/**
 * gsgpu_ttm_backend_bind - Bind GTT memory
 *
 * Called by ttm_tt_bind() on behalf of ttm_bo_handle_move_mem().
 * This handles binding GTT memory to the device address space.
 */
static int gsgpu_ttm_backend_bind(struct ttm_device *bdev,
				  struct ttm_tt *ttm,
				  struct ttm_resource *bo_mem)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bdev);
	struct gsgpu_ttm_tt *gtt = (void *)ttm;
	uint64_t flags;
	int r = 0;

	if (gtt->userptr) {
		r = gsgpu_ttm_tt_pin_userptr(bdev, ttm);
		if (r) {
			DRM_ERROR("failed to pin userptr\n");
			return r;
		}
	}
	if (!ttm->num_pages) {
		WARN(1, "nothing to bind %u pages for mreg 0x%px back 0x%px!\n",
		     ttm->num_pages, bo_mem, ttm);
	}

	if (!gsgpu_gtt_mgr_has_gart_addr(bo_mem)) {
		gtt->offset = GSGPU_BO_INVALID_OFFSET;
		return 0;
	}

	/* compute PTE flags relevant to this BO memory */
	flags = gsgpu_ttm_tt_pte_flags(adev, ttm, bo_mem);

	/* bind pages into GART page tables */
	gtt->offset = (u64)bo_mem->start << PAGE_SHIFT;
	r = gsgpu_gart_bind(adev, gtt->offset, ttm->num_pages,
			    ttm->pages, gtt->ttm.dma_address, flags);

	if (r)
		DRM_ERROR("failed to bind %u pages at 0x%08llX\n",
			  ttm->num_pages, gtt->offset);
	return r;
}

/**
 * gsgpu_ttm_alloc_gart - Allocate GART memory for buffer object
 */
int gsgpu_ttm_alloc_gart(struct ttm_buffer_object *bo)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->bdev);
	struct ttm_operation_ctx ctx = { false, false };
	struct gsgpu_ttm_tt *gtt = (void *)bo->ttm;
	struct ttm_resource *tmp;
	struct ttm_placement placement;
	struct ttm_place placements;
	uint64_t flags;
	int r;

	if (bo->resource->start != GSGPU_BO_INVALID_OFFSET)
                return 0;

	/* allocate GART space */
	placement.num_placement = 1;
	placement.placement = &placements;
	placement.num_busy_placement = 1;
	placement.busy_placement = &placements;
	placements.fpfn = 0;
	placements.lpfn = adev->gmc.gart_size >> PAGE_SHIFT;
	placements.mem_type = TTM_PL_TT;
        placements.flags = bo->resource->placement;

	r = ttm_bo_mem_space(bo, &placement, &tmp, &ctx);
	if (unlikely(r))
		return r;

	/* compute PTE flags for this buffer object */
	flags = gsgpu_ttm_tt_pte_flags(adev, bo->ttm, tmp);

	/* Bind pages */
	gtt->offset = (u64)tmp->start << PAGE_SHIFT;
        r = gsgpu_ttm_gart_bind(adev, bo, flags);
	if (unlikely(r)) {
		ttm_resource_free(bo, &tmp);
		return r;
	}
        ttm_resource_free(bo, &bo->resource);
	ttm_bo_assign_mem(bo, tmp);

	return 0;
}

/**
 * gsgpu_ttm_recover_gart - Rebind GTT pages
 *
 * Called by gsgpu_gtt_mgr_recover() from gsgpu_device_reset() to
 * rebind GTT pages during a GPU reset.
 */
int gsgpu_ttm_recover_gart(struct ttm_buffer_object *tbo)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(tbo->bdev);
	uint64_t flags;
	int r;

	if (!tbo->ttm)
		return 0;

	flags = gsgpu_ttm_tt_pte_flags(adev, tbo->ttm, tbo->resource);
	r = gsgpu_ttm_gart_bind(adev, tbo, flags);

	return r;
}

/**
 * gsgpu_ttm_backend_unbind - Unbind GTT mapped pages
 *
 * Called by ttm_tt_unbind() on behalf of ttm_bo_move_ttm() and
 * ttm_tt_destroy().
 */
static int gsgpu_ttm_backend_unbind(struct ttm_device *bdev,
				    struct ttm_tt *ttm)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bdev);
	struct gsgpu_ttm_tt *gtt = (void *)ttm;
	int r;

	/* if the pages have userptr pinning then clear that first */
	if (gtt->userptr)
		gsgpu_ttm_tt_unpin_userptr(bdev, ttm);

	if (gtt->offset == GSGPU_BO_INVALID_OFFSET)
		return 0;

	/* unbind shouldn't be done for GDS/GWS/OA in ttm_bo_clean_mm */
	r = gsgpu_gart_unbind(adev, gtt->offset, ttm->num_pages);
	if (r)
		DRM_ERROR("failed to unbind %u pages at 0x%08llX\n",
			  gtt->ttm.num_pages, gtt->offset);
	return r;
}

static void gsgpu_ttm_backend_destroy(struct ttm_device *bdev,
				      struct ttm_tt *ttm)
{
	struct gsgpu_ttm_tt *gtt = (void *)ttm;

	if (gtt->usertask)
		put_task_struct(gtt->usertask);

	ttm_tt_fini(&gtt->ttm);
	kfree(gtt);
}

/**
 * gsgpu_ttm_tt_create - Create a ttm_tt object for a given BO
 *
 * @bo: The buffer object to create a GTT ttm_tt object around
 *
 * Called by ttm_tt_create().
 */
static struct ttm_tt *gsgpu_ttm_tt_create(struct ttm_buffer_object *bo,
					  uint32_t page_flags)
{
	struct gsgpu_device *adev;
	struct gsgpu_ttm_tt *gtt;
	enum ttm_caching caching;

	adev = gsgpu_ttm_adev(bo->bdev);

	gtt = kzalloc(sizeof(struct gsgpu_ttm_tt), GFP_KERNEL);
	if (gtt == NULL) {
		return NULL;
	}

	struct gsgpu_bo *abo = ttm_to_gsgpu_bo(bo);
	if (abo->flags & GSGPU_GEM_CREATE_CPU_GTT_USWC)
		caching = ttm_write_combined;
	else
		caching = ttm_cached;

	/* allocate space for the uninitialized page entries */
	if (ttm_sg_tt_init(&gtt->ttm, bo, page_flags, caching)) {
		kfree(gtt);
		return NULL;
	}
	return &gtt->ttm;
}

/**
 * gsgpu_ttm_tt_populate - Map GTT pages visible to the device
 *
 * Map the pages of a ttm_tt object to an address space visible
 * to the underlying device.
 */
static int gsgpu_ttm_tt_populate(struct ttm_device *bdev,
				 struct ttm_tt *ttm,
				 struct ttm_operation_ctx *ctx)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bdev);
	struct gsgpu_ttm_tt *gtt = ttm_to_gsgpu_ttm_tt(ttm);
	pgoff_t i;
	int ret;

	/* user pages are bound by gsgpu_ttm_tt_pin_userptr() */
	if (gtt->userptr) {
		ttm->sg = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
		if (!ttm->sg)
			return -ENOMEM;
		return 0;
	}

	if (ttm->page_flags & TTM_TT_FLAG_EXTERNAL)
		return 0;

	ret = ttm_pool_alloc(&adev->mman.bdev.pool, ttm, ctx);
	if (ret)
		return ret;

	for (i = 0; i < ttm->num_pages; ++i)
		ttm->pages[i]->mapping = bdev->dev_mapping;

	return 0;
}

/**
 * gsgpu_ttm_tt_unpopulate - unmap GTT pages and unpopulate page arrays
 *
 * Unmaps pages of a ttm_tt object from the device address space and
 * unpopulates the page array backing it.
 */
static void gsgpu_ttm_tt_unpopulate(struct ttm_device *bdev,
				    struct ttm_tt *ttm)
{
	struct gsgpu_ttm_tt *gtt = ttm_to_gsgpu_ttm_tt(ttm);
	struct gsgpu_device *adev;
	pgoff_t i;

	gsgpu_ttm_backend_unbind(bdev, ttm);

	if (gtt->userptr) {
		gsgpu_ttm_tt_set_user_pages(ttm, NULL);
		kfree(ttm->sg);
		ttm->sg = NULL;
		return;
	}

	if (ttm->page_flags & TTM_TT_FLAG_EXTERNAL)
		return;

	for (i = 0; i < ttm->num_pages; ++i)
		ttm->pages[i]->mapping = NULL;

	adev = gsgpu_ttm_adev(bdev);

	ttm_pool_free(&adev->mman.bdev.pool, ttm);
}

/**
 * gsgpu_ttm_tt_set_userptr - Initialize userptr GTT ttm_tt for the current
 * task
 *
 * @ttm: The ttm_tt object to bind this userptr object to
 * @addr:  The address in the current tasks VM space to use
 * @flags: Requirements of userptr object.
 *
 * Called by gsgpu_gem_userptr_ioctl() to bind userptr pages
 * to current task
 */
int gsgpu_ttm_tt_set_userptr(struct ttm_tt *ttm, uint64_t addr,
			     uint32_t flags)
{
	struct gsgpu_ttm_tt *gtt = (void *)ttm;

	if (gtt == NULL)
		return -EINVAL;

	gtt->userptr = addr;
	gtt->userflags = flags;

	if (gtt->usertask)
		put_task_struct(gtt->usertask);
	gtt->usertask = current->group_leader;
	get_task_struct(gtt->usertask);

	return 0;
}

/**
 * gsgpu_ttm_tt_get_usermm - Return memory manager for ttm_tt object
 */
struct mm_struct *gsgpu_ttm_tt_get_usermm(struct ttm_tt *ttm)
{
	struct gsgpu_ttm_tt *gtt = (void *)ttm;

	if (gtt == NULL)
		return NULL;

	if (gtt->usertask == NULL)
		return NULL;

	return gtt->usertask->mm;
}

/**
 * gsgpu_ttm_tt_affect_userptr - Determine if a ttm_tt object lays inside an
 * address range for the current task.
 *
 */
bool gsgpu_ttm_tt_affect_userptr(struct ttm_tt *ttm, unsigned long start,
				 unsigned long end)
{
	struct gsgpu_ttm_tt *gtt = (void *)ttm;
	unsigned long size;

	if (gtt == NULL || !gtt->userptr)
		return false;

	/* Return false if no part of the ttm_tt object lies within
	 * the range
	 */
	size = (unsigned long)gtt->ttm.num_pages * PAGE_SIZE;
	if (gtt->userptr > end || gtt->userptr + size <= start)
		return false;

	return true;
}

/**
 * gsgpu_ttm_tt_is_userptr - Have the pages backing by userptr?
 */
bool gsgpu_ttm_tt_is_userptr(struct ttm_tt *ttm)
{
	struct gsgpu_ttm_tt *gtt = (void *)ttm;

	if (gtt == NULL || !gtt->userptr)
		return false;

	return true;
}

/**
 * gsgpu_ttm_tt_is_readonly - Is the ttm_tt object read only?
 */
bool gsgpu_ttm_tt_is_readonly(struct ttm_tt *ttm)
{
	struct gsgpu_ttm_tt *gtt = (void *)ttm;

	if (gtt == NULL)
		return false;

	return !!(gtt->userflags & GSGPU_GEM_USERPTR_READONLY);
}

/**
 * gsgpu_ttm_tt_pte_flags - Compute PTE flags for ttm_tt object
 *
 * @ttm: The ttm_tt object to compute the flags for
 * @mem: The memory registry backing this ttm_tt object
 */
uint64_t gsgpu_ttm_tt_pte_flags(struct gsgpu_device *adev, struct ttm_tt *ttm,
				struct ttm_resource *mem)
{
	uint64_t flags = 0;

	if (mem && mem->mem_type != TTM_PL_SYSTEM)
		flags |= GSGPU_PTE_PRESENT;

	//if (mem && mem->mem_type == TTM_PL_TT) {
	//	flags |= GSGPU_PTE_SYSTEM;

	//	if (ttm->caching_state == tt_cached)
	//		flags |= GSGPU_PTE_SNOOPED;
	//}

	flags |= adev->gart.gart_pte_flags;
	//flags |= GSGPU_PTE_READABLE;

	if (!gsgpu_ttm_tt_is_readonly(ttm))
		flags |= GSGPU_PTE_WRITEABLE;

	return flags;
}

/**
 * gsgpu_ttm_bo_eviction_valuable - Check to see if we can evict a buffer
 * object.
 *
 * Return true if eviction is sensible. Called by ttm_mem_evict_first() on
 * behalf of ttm_bo_mem_force_space() which tries to evict buffer objects until
 * it can find space for a new object and by ttm_bo_force_list_clean() which is
 * used to clean out a memory space.
 */
static bool gsgpu_ttm_bo_eviction_valuable(struct ttm_buffer_object *bo,
					   const struct ttm_place *place)
{
	struct gsgpu_res_cursor cursor;

	switch (bo->resource->mem_type) {
	case TTM_PL_TT:
		return true;

	case TTM_PL_VRAM:
		/* Check each drm MM node individually */
		gsgpu_res_first(bo->resource, 0, bo->resource->size, &cursor);
		while (cursor.remaining) {
			if (place->fpfn < PFN_DOWN(cursor.start + cursor.size)
			    && !(place->lpfn &&
				 place->lpfn <= PFN_DOWN(cursor.start)))
				return true;

			gsgpu_res_next(&cursor, cursor.size);
		}
		return false;

	default:
		break;
	}

	return ttm_bo_eviction_valuable(bo, place);
}

/**
 * VRAM access helper functions.
 *
 * gsgpu_device_vram_access - read/write a buffer in vram
 *
 * @adev: gsgpu_device pointer
 * @pos: offset of the buffer in vram
 * @buf: virtual address of the buffer in system memory
 * @size: read/write size, sizeof(@buf) must > @size
 * @write: true - write to vram, otherwise - read from vram
 */
static void gsgpu_device_vram_access(struct gsgpu_device *adev, loff_t pos,
				     uint32_t *buf, size_t size, bool write)
{
       uint64_t last;
       unsigned long flags;

       last = size - 4;
       for (last += pos; pos <= last; pos += 4) {
               spin_lock_irqsave(&adev->mmio_idx_lock, flags);
               WREG32_NO_KIQ(mmMM_INDEX, ((uint32_t)pos) | 0x80000000);
               WREG32_NO_KIQ(mmMM_INDEX_HI, pos >> 31);
               if (write)
                       WREG32_NO_KIQ(mmMM_DATA, *buf++);
               else
                       *buf++ = RREG32_NO_KIQ(mmMM_DATA);
               spin_unlock_irqrestore(&adev->mmio_idx_lock, flags);
       }
}

/**
 * gsgpu_ttm_access_memory - Read or Write memory that backs a buffer object.
 *
 * @bo:  The buffer object to read/write
 * @offset:  Offset into buffer object
 * @buf:  Secondary buffer to write/read from
 * @len: Length in bytes of access
 * @write:  true if writing
 *
 * This is used to access VRAM that backs a buffer object via MMIO
 * access for debugging purposes.
 */
static int gsgpu_ttm_access_memory(struct ttm_buffer_object *bo,
                                   unsigned long offset, void *buf, int len,
                                   int write)
{
	struct gsgpu_bo *abo = ttm_to_gsgpu_bo(bo);
	struct gsgpu_device *adev = gsgpu_ttm_adev(abo->tbo.bdev);
	struct gsgpu_res_cursor cursor;
	unsigned long flags;
	uint32_t value = 0;
	int ret = 0;

	if (bo->resource->mem_type != TTM_PL_VRAM)
		return -EIO;

	gsgpu_res_first(bo->resource, offset, len, &cursor);
	while (cursor.remaining) {
		uint64_t aligned_pos = cursor.start & ~(uint64_t)3;
		uint64_t bytes = 4 - (cursor.start & 3);
		uint32_t shift = (cursor.start & 3) * 8;
		uint32_t mask = 0xffffffff << shift;

		if (cursor.size < bytes) {
			mask &= 0xffffffff >> (bytes - cursor.size) * 8;
			bytes = cursor.size;
		}

		if (mask != 0xffffffff) {
			spin_lock_irqsave(&adev->mmio_idx_lock, flags);
			WREG32_NO_KIQ(mmMM_INDEX, ((uint32_t)aligned_pos) | 0x80000000);
			WREG32_NO_KIQ(mmMM_INDEX_HI, aligned_pos >> 31);
			value = RREG32_NO_KIQ(mmMM_DATA);
			if (write) {
				value &= ~mask;
				value |= (*(uint32_t *)buf << shift) & mask;
				WREG32_NO_KIQ(mmMM_DATA, value);
			}
			spin_unlock_irqrestore(&adev->mmio_idx_lock, flags);
			if (!write) {
				value = (value & mask) >> shift;
				memcpy(buf, &value, bytes);
			}
		} else {
			bytes = cursor.size & ~0x3ULL;
			gsgpu_device_vram_access(adev, cursor.start,
						 (uint32_t *)buf, bytes,
						 write);
		}

		ret += bytes;
		buf = (uint8_t *)buf + bytes;
		gsgpu_res_next(&cursor, bytes);
	}

	return ret;
}

static void gsgpu_bo_delete_mem_notify(struct ttm_buffer_object *bo)
{
       gsgpu_bo_move_notify(bo, false, NULL);
}

static struct ttm_device_funcs gsgpu_device_funcs = {
	.ttm_tt_create = &gsgpu_ttm_tt_create,
	.ttm_tt_populate = &gsgpu_ttm_tt_populate,
	.ttm_tt_unpopulate = &gsgpu_ttm_tt_unpopulate,
	.ttm_tt_destroy = &gsgpu_ttm_backend_destroy,
	.eviction_valuable = gsgpu_ttm_bo_eviction_valuable,
	.evict_flags = &gsgpu_evict_flags,
	.move = &gsgpu_bo_move,
       .delete_mem_notify = &gsgpu_bo_delete_mem_notify,
	.io_mem_reserve = &gsgpu_ttm_io_mem_reserve,
	.io_mem_free = &gsgpu_ttm_io_mem_free,
	.io_mem_pfn = gsgpu_ttm_io_mem_pfn,
	.access_memory = &gsgpu_ttm_access_memory
};

/*
 * Firmware Reservation functions
 */
/**
 * gsgpu_ttm_fw_reserve_vram_fini - free fw reserved vram
 *
 * @adev: gsgpu_device pointer
 *
 * free fw reserved vram if it has been reserved.
 */
static void gsgpu_ttm_fw_reserve_vram_fini(struct gsgpu_device *adev)
{
	gsgpu_bo_free_kernel(&adev->fw_vram_usage.reserved_bo,
		NULL, &adev->fw_vram_usage.va);
}

/**
 * gsgpu_ttm_fw_reserve_vram_init - create bo vram reservation from fw
 *
 * @adev: gsgpu_device pointer
 *
 * create bo vram reservation from fw.
 */
static int gsgpu_ttm_fw_reserve_vram_init(struct gsgpu_device *adev)
{
	struct ttm_operation_ctx ctx = { false, false };
	struct gsgpu_bo_param bp;
	int r = 0;
	int i;
	u64 vram_size = adev->gmc.visible_vram_size;
	u64 offset = adev->fw_vram_usage.start_offset;
	u64 size = adev->fw_vram_usage.size;
	struct gsgpu_bo *bo;

	memset(&bp, 0, sizeof(bp));
	bp.size = adev->fw_vram_usage.size;
	bp.byte_align = PAGE_SIZE;
	bp.domain = GSGPU_GEM_DOMAIN_VRAM;
	bp.flags = GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED |
		GSGPU_GEM_CREATE_VRAM_CONTIGUOUS;
	bp.type = ttm_bo_type_kernel;
	bp.resv = NULL;
	adev->fw_vram_usage.va = NULL;
	adev->fw_vram_usage.reserved_bo = NULL;

	if (adev->fw_vram_usage.size > 0 &&
		adev->fw_vram_usage.size <= vram_size) {

		r = gsgpu_bo_create(adev, &bp,
				     &adev->fw_vram_usage.reserved_bo);
		if (r)
			goto error_create;

		r = gsgpu_bo_reserve(adev->fw_vram_usage.reserved_bo, false);
		if (r)
			goto error_reserve;

		/* remove the original mem node and create a new one at the
		 * request position
		 */
		bo = adev->fw_vram_usage.reserved_bo;
		offset = ALIGN(offset, PAGE_SIZE);
		for (i = 0; i < bo->placement.num_placement; ++i) {
			bo->placements[i].fpfn = offset >> PAGE_SHIFT;
			bo->placements[i].lpfn = (offset + size) >> PAGE_SHIFT;
		}

		ttm_resource_free(&bo->tbo, &bo->tbo.resource);
		r = ttm_bo_mem_space(&bo->tbo, &bo->placement,
				     &bo->tbo.resource, &ctx);
		if (r)
			goto error_pin;

		r = gsgpu_bo_pin_restricted(adev->fw_vram_usage.reserved_bo,
			GSGPU_GEM_DOMAIN_VRAM,
			adev->fw_vram_usage.start_offset,
			(adev->fw_vram_usage.start_offset +
			adev->fw_vram_usage.size));
		if (r)
			goto error_pin;
		r = gsgpu_bo_kmap(adev->fw_vram_usage.reserved_bo,
			&adev->fw_vram_usage.va);
		if (r)
			goto error_kmap;

		gsgpu_bo_unreserve(adev->fw_vram_usage.reserved_bo);
	}
	return r;

error_kmap:
	gsgpu_bo_unpin(adev->fw_vram_usage.reserved_bo);
error_pin:
	gsgpu_bo_unreserve(adev->fw_vram_usage.reserved_bo);
error_reserve:
	gsgpu_bo_unref(&adev->fw_vram_usage.reserved_bo);
error_create:
	adev->fw_vram_usage.va = NULL;
	adev->fw_vram_usage.reserved_bo = NULL;
	return r;
}
/**
 * gsgpu_ttm_init - Init the memory management (ttm) as well as various
 * gtt/vram related fields.
 *
 * This initializes all of the memory space pools that the TTM layer
 * will need such as the GTT space (system memory mapped to the device),
 * VRAM (on-board memory), and on-chip memories (GDS, GWS, OA) which
 * can be mapped per VMID.
 */
int gsgpu_ttm_init(struct gsgpu_device *adev)
{
	uint64_t gtt_size;
	u64 vis_vram_limit;

	/* No others user of address space so set it to 0 */
	int r = ttm_device_init(&adev->mman.bdev, &gsgpu_device_funcs, adev->dev,
				adev->ddev->anon_inode->i_mapping,
				adev->ddev->vma_offset_manager,
				adev->need_swiotlb,
				dma_addressing_limited(adev->dev));
	if (r) {
		DRM_ERROR("failed initializing buffer object driver(%d).\n", r);
		return r;
	}
	adev->mman.initialized = true;

	/* Initialize VRAM pool with all of VRAM divided into pages */
	r = gsgpu_vram_mgr_init(adev);
	if (r) {
		DRM_ERROR("Failed initializing VRAM heap.\n");
		return r;
	}

	/* Reduce size of CPU-visible VRAM if requested */
	vis_vram_limit = (u64)gsgpu_vis_vram_limit * 1024 * 1024;
	if (gsgpu_vis_vram_limit > 0 &&
	    vis_vram_limit <= adev->gmc.visible_vram_size)
		adev->gmc.visible_vram_size = vis_vram_limit;

	/* Change the size here instead of the init above so only lpfn is affected */
	gsgpu_ttm_set_buffer_funcs_status(adev, false);
#ifdef CONFIG_64BIT
	adev->mman.aper_base_kaddr = ioremap(adev->gmc.aper_base,
					     adev->gmc.visible_vram_size);
#endif

	/*
	 *The reserved vram for firmware must be pinned to the specified
	 *place on the VRAM, so reserve it early.
	 */
	r = gsgpu_ttm_fw_reserve_vram_init(adev);
	if (r) {
		return r;
	}

	/* allocate memory as required for VGA
	 * This is used for VGA emulation and pre-OS scanout buffers to
	 * avoid display artifacts while transitioning between pre-OS
	 * and driver.  */
	if (adev->gmc.stolen_size) {
		r = gsgpu_bo_create_kernel(adev, adev->gmc.stolen_size, PAGE_SIZE,
					    GSGPU_GEM_DOMAIN_VRAM,
					    &adev->stolen_vga_memory,
					    NULL, NULL);
		if (r)
			return r;
	}
	DRM_INFO("gsgpu: %uM of VRAM memory ready\n",
		 (unsigned) (adev->gmc.real_vram_size / (1024 * 1024)));

	/* Compute GTT size, either bsaed on 3/4th the size of RAM size
	 * or whatever the user passed on module init */
	if (gsgpu_gtt_size == -1) {
		struct sysinfo si;

		si_meminfo(&si);
		gtt_size = min(max((GSGPU_DEFAULT_GTT_SIZE_MB << 20),
			       adev->gmc.mc_vram_size),
			       ((uint64_t)si.totalram * si.mem_unit * 3/4));
	} else
		gtt_size = (uint64_t)gsgpu_gtt_size << 20;

	/* Initialize GTT memory pool */
	r = gsgpu_gtt_mgr_init(adev, gtt_size);
	if (r) {
		DRM_ERROR("Failed initializing GTT heap.\n");
		return r;
	}
	DRM_INFO("gsgpu: %uM of GTT memory ready.\n",
		 (unsigned)(gtt_size / (1024 * 1024)));

	/* Register debugfs entries for gsgpu_ttm */
	r = gsgpu_ttm_debugfs_init(adev);
	if (r) {
		DRM_ERROR("Failed to init debugfs\n");
		return r;
	}
	return 0;
}

/**
 * gsgpu_ttm_late_init - Handle any late initialization for gsgpu_ttm
 */
void gsgpu_ttm_late_init(struct gsgpu_device *adev)
{
	/* return the VGA stolen memory (if any) back to VRAM */
	gsgpu_bo_free_kernel(&adev->stolen_vga_memory, NULL, NULL);
}

/**
 * gsgpu_ttm_fini - De-initialize the TTM memory pools
 */
void gsgpu_ttm_fini(struct gsgpu_device *adev)
{
	if (!adev->mman.initialized)
		return;

	gsgpu_ttm_debugfs_fini(adev);
	gsgpu_ttm_fw_reserve_vram_fini(adev);
	if (adev->mman.aper_base_kaddr)
		iounmap(adev->mman.aper_base_kaddr);
	adev->mman.aper_base_kaddr = NULL;

	gsgpu_vram_mgr_fini(adev);
	gsgpu_gtt_mgr_fini(adev);
	ttm_device_fini(&adev->mman.bdev);
	adev->mman.initialized = false;
	DRM_INFO("gsgpu: ttm finalized\n");
}

/**
 * gsgpu_ttm_set_buffer_funcs_status - enable/disable use of buffer functions
 *
 * @adev: gsgpu_device pointer
 * @enable: true when we can use buffer functions.
 *
 * Enable/disable use of buffer functions during suspend/resume. This should
 * only be called at bootup or when userspace isn't running.
 */
void gsgpu_ttm_set_buffer_funcs_status(struct gsgpu_device *adev, bool enable)
{
	struct ttm_resource_manager *man = adev->mman.bdev.man_drv[TTM_PL_VRAM];
	uint64_t size;
	int r;

	if (!adev->mman.initialized || adev->in_gpu_reset ||
	    adev->mman.buffer_funcs_enabled == enable)
		return;

	if (enable) {
		struct gsgpu_ring *ring = adev->mman.buffer_funcs_ring;
		struct drm_gpu_scheduler *sched = &ring->sched;
		r = drm_sched_entity_init(&adev->mman.entity,
					  DRM_SCHED_PRIORITY_KERNEL, &sched, 1, NULL);
		if (r) {
			DRM_ERROR("Failed setting up TTM BO move entity (%d)\n",
				  r);
			return;
		}
	} else {
		drm_sched_entity_destroy(&adev->mman.entity);
		dma_fence_put(man->move);
		man->move = NULL;
	}

	/* this just adjusts TTM size idea, which sets lpfn to the correct value */
	if (enable)
		size = adev->gmc.real_vram_size;
	else
		size = adev->gmc.visible_vram_size;
	man->size = size >> PAGE_SHIFT;
	adev->mman.buffer_funcs_enabled = enable;
}

int gsgpu_copy_buffer(struct gsgpu_ring *ring, uint64_t src_offset,
		       uint64_t dst_offset, uint32_t byte_count,
		       struct dma_resv *resv,
		       struct dma_fence **fence, bool direct_submit,
		       bool vm_needs_flush)
{
	struct gsgpu_device *adev = ring->adev;
	struct gsgpu_job *job;

	uint32_t max_bytes;
	unsigned num_loops, num_dw;
	unsigned i;
	int r;

	if (direct_submit && !ring->ready) {
		DRM_ERROR("Trying to move memory with ring turned off.\n");
		return -EINVAL;
	}

	max_bytes = adev->mman.buffer_funcs->copy_max_bytes;
	num_loops = DIV_ROUND_UP(byte_count, max_bytes);
	num_dw = num_loops * adev->mman.buffer_funcs->copy_num_dw;

	/* for IB padding */
	while (num_dw & 0x7)
		num_dw++;

	r = gsgpu_job_alloc_with_ib(adev, num_dw * 4, &job);
	if (r)
		return r;

	job->vm_needs_flush = vm_needs_flush;
	if (resv) {
		r = gsgpu_sync_resv(adev, &job->sync, resv,
				    GSGPU_FENCE_OWNER_UNDEFINED,
				    false);
		if (r) {
			DRM_ERROR("sync failed (%d).\n", r);
			goto error_free;
		}
	}

	for (i = 0; i < num_loops; i++) {
		uint32_t cur_size_in_bytes = min(byte_count, max_bytes);

		gsgpu_emit_copy_buffer(adev, &job->ibs[0], src_offset,
					dst_offset, cur_size_in_bytes);

		src_offset += cur_size_in_bytes;
		dst_offset += cur_size_in_bytes;
		byte_count -= cur_size_in_bytes;
	}

	gsgpu_ring_pad_ib(ring, &job->ibs[0]);
	WARN_ON(job->ibs[0].length_dw > num_dw);
	if (direct_submit)
		r = gsgpu_job_submit_direct(job, ring, fence);
	else
		r = gsgpu_job_submit(job, &adev->mman.entity,
				      GSGPU_FENCE_OWNER_UNDEFINED, fence);
	if (r)
		goto error_free;

	return r;

error_free:
	gsgpu_job_free(job);
	DRM_ERROR("Error scheduling IBs (%d)\n", r);
	return r;
}

int gsgpu_fill_buffer(struct gsgpu_bo *bo,
		      uint32_t src_data,
		      struct dma_resv *resv,
		      struct dma_fence **fence)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	uint32_t max_bytes = adev->mman.buffer_funcs->fill_max_bytes;
	struct gsgpu_ring *ring = adev->mman.buffer_funcs_ring;

	int r;

	if (!adev->mman.buffer_funcs_enabled) {
		DRM_ERROR("Trying to clear memory with ring turned off.\n");
		return -EINVAL;
	}

	if (bo->tbo.resource->mem_type == TTM_PL_TT) {
		r = gsgpu_ttm_alloc_gart(&bo->tbo);
		if (r)
			return r;
	}

	struct gsgpu_res_cursor cursor;
	gsgpu_res_first(bo->tbo.resource, 0, gsgpu_bo_size(bo), &cursor);
	unsigned int num_loops = 0;
	while (cursor.remaining) {
		num_loops += DIV_ROUND_UP_ULL(cursor.size, max_bytes);
		gsgpu_res_next(&cursor, cursor.size);
	}
	unsigned int num_dw = num_loops * adev->mman.buffer_funcs->fill_num_dw;

	/* for IB padding */
	num_dw += 64;

	struct gsgpu_job *job;
	r = gsgpu_job_alloc_with_ib(adev, num_dw * 4, &job);
	if (r)
		return r;

	if (resv) {
		r = gsgpu_sync_resv(adev, &job->sync, resv,
				     GSGPU_FENCE_OWNER_UNDEFINED, false);
		if (r) {
			DRM_ERROR("sync failed (%d).\n", r);
			goto error_free;
		}
	}

	gsgpu_res_first(bo->tbo.resource, 0, gsgpu_bo_size(bo), &cursor);
	while (cursor.remaining) {
		uint32_t cur_size = min_t(uint64_t, cursor.size, max_bytes);
		uint64_t dst_addr = cursor.start;

		dst_addr += gsgpu_ttm_domain_start(adev, bo->tbo.resource->mem_type);
		gsgpu_emit_fill_buffer(adev, &job->ibs[0], src_data, dst_addr,
					cur_size);
		gsgpu_res_next(&cursor, cur_size);
	}

	gsgpu_ring_pad_ib(ring, &job->ibs[0]);
	WARN_ON(job->ibs[0].length_dw > num_dw);
	r = gsgpu_job_submit(job, &adev->mman.entity,
			      GSGPU_FENCE_OWNER_UNDEFINED, fence);
	if (r)
		goto error_free;

	return 0;

error_free:
	gsgpu_job_free(job);
	return r;
}

#if defined(CONFIG_DEBUG_FS)

static int gsgpu_mm_dump_table(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *)m->private;
	unsigned ttm_pl = *(int *)node->info_ent->data;
	struct drm_device *dev = node->minor->dev;
	struct gsgpu_device *adev = drm_to_adev(dev);
	struct ttm_resource_manager *man = adev->mman.bdev.man_drv[ttm_pl];
	struct drm_printer p = drm_seq_file_printer(m);

	man->func->debug(man, &p);
	return 0;
}

static int gsgpu_ttm_pool_debugfs(struct seq_file *m, void *data)
{
       struct drm_info_node *node = (struct drm_info_node *)m->private;
       struct drm_device *dev = node->minor->dev;
       struct gsgpu_device *adev = drm_to_adev(dev);

       return ttm_pool_debugfs(&adev->mman.bdev.pool, m);
}

static int ttm_pl_vram = TTM_PL_VRAM;
static int ttm_pl_tt = TTM_PL_TT;

static const struct drm_info_list gsgpu_ttm_debugfs_list[] = {
	{"gsgpu_vram_mm", gsgpu_mm_dump_table, 0, &ttm_pl_vram},
	{"gsgpu_gtt_mm", gsgpu_mm_dump_table, 0, &ttm_pl_tt},
	{"ttm_page_pool", gsgpu_ttm_pool_debugfs, 0, NULL},
};

/**
 * gsgpu_ttm_vram_read - Linear read access to VRAM
 *
 * Accesses VRAM via MMIO for debugging purposes.
 */
static ssize_t gsgpu_ttm_vram_read(struct file *f, char __user *buf,
				    size_t size, loff_t *pos)
{
	struct gsgpu_device *adev = file_inode(f)->i_private;
	ssize_t result = 0;
	int r;

	if (size & 0x3 || *pos & 0x3)
		return -EINVAL;

	if (*pos >= adev->gmc.mc_vram_size)
		return -ENXIO;

	while (size) {
		unsigned long flags;
		uint32_t value;

		if (*pos >= adev->gmc.mc_vram_size)
			return result;

		spin_lock_irqsave(&adev->mmio_idx_lock, flags);
		WREG32_NO_KIQ(mmMM_INDEX, ((uint32_t)*pos) | 0x80000000);
		WREG32_NO_KIQ(mmMM_INDEX_HI, *pos >> 31);
		value = RREG32_NO_KIQ(mmMM_DATA);
		spin_unlock_irqrestore(&adev->mmio_idx_lock, flags);

		r = put_user(value, (uint32_t *)buf);
		if (r)
			return r;

		result += 4;
		buf += 4;
		*pos += 4;
		size -= 4;
	}

	return result;
}

/**
 * gsgpu_ttm_vram_write - Linear write access to VRAM
 *
 * Accesses VRAM via MMIO for debugging purposes.
 */
static ssize_t gsgpu_ttm_vram_write(struct file *f, const char __user *buf,
				    size_t size, loff_t *pos)
{
	struct gsgpu_device *adev = file_inode(f)->i_private;
	ssize_t result = 0;
	int r;

	if (size & 0x3 || *pos & 0x3)
		return -EINVAL;

	if (*pos >= adev->gmc.mc_vram_size)
		return -ENXIO;

	while (size) {
		unsigned long flags;
		uint32_t value;

		if (*pos >= adev->gmc.mc_vram_size)
			return result;

		r = get_user(value, (uint32_t *)buf);
		if (r)
			return r;

		spin_lock_irqsave(&adev->mmio_idx_lock, flags);
		WREG32_NO_KIQ(mmMM_INDEX, ((uint32_t)*pos) | 0x80000000);
		WREG32_NO_KIQ(mmMM_INDEX_HI, *pos >> 31);
		WREG32_NO_KIQ(mmMM_DATA, value);
		spin_unlock_irqrestore(&adev->mmio_idx_lock, flags);

		result += 4;
		buf += 4;
		*pos += 4;
		size -= 4;
	}

	return result;
}

static const struct file_operations gsgpu_ttm_vram_fops = {
	.owner = THIS_MODULE,
	.read = gsgpu_ttm_vram_read,
	.write = gsgpu_ttm_vram_write,
	.llseek = default_llseek,
};

#ifdef CONFIG_DRM_GSGPU_GART_DEBUGFS

/**
 * gsgpu_ttm_gtt_read - Linear read access to GTT memory
 */
static ssize_t gsgpu_ttm_gtt_read(struct file *f, char __user *buf,
				   size_t size, loff_t *pos)
{
	struct gsgpu_device *adev = file_inode(f)->i_private;
	ssize_t result = 0;
	int r;

	while (size) {
		loff_t p = *pos / PAGE_SIZE;
		unsigned off = *pos & ~PAGE_MASK;
		size_t cur_size = min_t(size_t, size, PAGE_SIZE - off);
		struct page *page;
		void *ptr;

		if (p >= adev->gart.num_cpu_pages)
			return result;

		page = adev->gart.pages[p];
		if (page) {
			ptr = kmap(page);
			ptr += off;

			r = copy_to_user(buf, ptr, cur_size);
			kunmap(adev->gart.pages[p]);
		} else
			r = clear_user(buf, cur_size);

		if (r)
			return -EFAULT;

		result += cur_size;
		buf += cur_size;
		*pos += cur_size;
		size -= cur_size;
	}

	return result;
}

static const struct file_operations gsgpu_ttm_gtt_fops = {
	.owner = THIS_MODULE,
	.read = gsgpu_ttm_gtt_read,
	.llseek = default_llseek
};

#endif

/**
 * gsgpu_iomem_read - Virtual read access to GPU mapped memory
 *
 * This function is used to read memory that has been mapped to the
 * GPU and the known addresses are not physical addresses but instead
 * bus addresses (e.g., what you'd put in an IB or ring buffer).
 */
static ssize_t gsgpu_iomem_read(struct file *f, char __user *buf,
				 size_t size, loff_t *pos)
{
	struct gsgpu_device *adev = file_inode(f)->i_private;
	struct iommu_domain *dom;
	ssize_t result = 0;
	int r;

	/* retrieve the IOMMU domain if any for this device */
	dom = iommu_get_domain_for_dev(adev->dev);

	while (size) {
		phys_addr_t addr = *pos & PAGE_MASK;
		loff_t off = *pos & ~PAGE_MASK;
		size_t bytes = PAGE_SIZE - off;
		unsigned long pfn;
		struct page *p;
		void *ptr;

		bytes = bytes < size ? bytes : size;

		/* Translate the bus address to a physical address.  If
		 * the domain is NULL it means there is no IOMMU active
		 * and the address translation is the identity
		 */
		addr = dom ? iommu_iova_to_phys(dom, addr) : addr;

		pfn = addr >> PAGE_SHIFT;
		if (!pfn_valid(pfn))
			return -EPERM;

		p = pfn_to_page(pfn);
		if (p->mapping != adev->mman.bdev.dev_mapping)
			return -EPERM;

		ptr = kmap(p);
		r = copy_to_user(buf, ptr + off, bytes);
		kunmap(p);
		if (r)
			return -EFAULT;

		size -= bytes;
		*pos += bytes;
		result += bytes;
	}

	return result;
}

/**
 * gsgpu_iomem_write - Virtual write access to GPU mapped memory
 *
 * This function is used to write memory that has been mapped to the
 * GPU and the known addresses are not physical addresses but instead
 * bus addresses (e.g., what you'd put in an IB or ring buffer).
 */
static ssize_t gsgpu_iomem_write(struct file *f, const char __user *buf,
				 size_t size, loff_t *pos)
{
	struct gsgpu_device *adev = file_inode(f)->i_private;
	struct iommu_domain *dom;
	ssize_t result = 0;
	int r;

	dom = iommu_get_domain_for_dev(adev->dev);

	while (size) {
		phys_addr_t addr = *pos & PAGE_MASK;
		loff_t off = *pos & ~PAGE_MASK;
		size_t bytes = PAGE_SIZE - off;
		unsigned long pfn;
		struct page *p;
		void *ptr;

		bytes = bytes < size ? bytes : size;

		addr = dom ? iommu_iova_to_phys(dom, addr) : addr;

		pfn = addr >> PAGE_SHIFT;
		if (!pfn_valid(pfn))
			return -EPERM;

		p = pfn_to_page(pfn);
		if (p->mapping != adev->mman.bdev.dev_mapping)
			return -EPERM;

		ptr = kmap(p);
		r = copy_from_user(ptr + off, buf, bytes);
		kunmap(p);
		if (r)
			return -EFAULT;

		size -= bytes;
		*pos += bytes;
		result += bytes;
	}

	return result;
}

static const struct file_operations gsgpu_ttm_iomem_fops = {
	.owner = THIS_MODULE,
	.read = gsgpu_iomem_read,
	.write = gsgpu_iomem_write,
	.llseek = default_llseek
};

static const struct {
	char *name;
	const struct file_operations *fops;
	int domain;
} ttm_debugfs_entries[] = {
	{ "gsgpu_vram", &gsgpu_ttm_vram_fops, TTM_PL_VRAM },
#ifdef CONFIG_DRM_GSGPU_GART_DEBUGFS
	{ "gsgpu_gtt", &gsgpu_ttm_gtt_fops, TTM_PL_TT },
#endif
	{ "gsgpu_iomem", &gsgpu_ttm_iomem_fops, TTM_PL_SYSTEM },
};

#endif

static int gsgpu_ttm_debugfs_init(struct gsgpu_device *adev)
{
#if defined(CONFIG_DEBUG_FS)
	unsigned count;

	struct drm_minor *minor = adev->ddev->primary;
	struct dentry *ent, *root = minor->debugfs_root;

	for (count = 0; count < ARRAY_SIZE(ttm_debugfs_entries); count++) {
		ent = debugfs_create_file(
				ttm_debugfs_entries[count].name,
				S_IFREG | S_IRUGO, root,
				adev,
				ttm_debugfs_entries[count].fops);
		if (IS_ERR(ent))
			return PTR_ERR(ent);
		if (ttm_debugfs_entries[count].domain == TTM_PL_VRAM)
			i_size_write(ent->d_inode, adev->gmc.mc_vram_size);
		else if (ttm_debugfs_entries[count].domain == TTM_PL_TT)
			i_size_write(ent->d_inode, adev->gmc.gart_size);
		adev->mman.debugfs_entries[count] = ent;
	}

	count = ARRAY_SIZE(gsgpu_ttm_debugfs_list);
	return gsgpu_debugfs_add_files(adev, gsgpu_ttm_debugfs_list, count);
#else
	return 0;
#endif
}

static void gsgpu_ttm_debugfs_fini(struct gsgpu_device *adev)
{
#if defined(CONFIG_DEBUG_FS)
	unsigned i;

	for (i = 0; i < ARRAY_SIZE(ttm_debugfs_entries); i++)
		debugfs_remove(adev->mman.debugfs_entries[i]);
#endif
}

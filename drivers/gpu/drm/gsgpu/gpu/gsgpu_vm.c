#include <linux/dma-fence-array.h>
#include <linux/interval_tree_generic.h>
#include <linux/idr.h>
#include <drm/gsgpu_drm.h>
#include "gsgpu.h"
#include "gsgpu_trace.h"
#include "gsgpu_vm_it.h"
#include "gsgpu_gmc.h"

/**
 * DOC: GPUVM
 *
 * GPUVM is similar to the legacy gart on older asics, however
 * rather than there being a single global gart table
 * for the entire GPU, there are multiple VM page tables active
 * at any given time.  The VM page tables can contain a mix
 * vram pages and system memory pages and system memory pages
 * can be mapped as snooped (cached system pages) or unsnooped
 * (uncached system pages).
 * Each VM has an ID associated with it and there is a page table
 * associated with each VMID.  When execting a command buffer,
 * the kernel tells the the ring what VMID to use for that command
 * buffer.  VMIDs are allocated dynamically as commands are submitted.
 * The userspace drivers maintain their own address space and the kernel
 * sets up their pages tables accordingly when they submit their
 * command buffers and a VMID is assigned.
 * Cayman/Trinity support up to 8 active VMs at any given time;
 * SI supports 16.
 */

/**
 * struct gsgpu_pte_update_params - Local structure
 *
 * Encapsulate some VM table update parameters to reduce
 * the number of function parameters
 *
 */
struct gsgpu_pte_update_params {

	/**
	 * @ldev: gsgpu device we do this update for
	 */
	struct gsgpu_device *ldev;

	/**
	 * @vm: optional gsgpu_vm we do this update for
	 */
	struct gsgpu_vm *vm;

	/**
	 * @src: address where to copy page table entries from
	 */
	u64 src;

	/**
	 * @ib: indirect buffer to fill with commands
	 */
	struct gsgpu_ib *ib;

	/**
	 * @func: Function which actually does the update
	 */
	void (*func)(struct gsgpu_pte_update_params *params,
		     struct gsgpu_bo *bo, u64 pe,
		     u64 addr, unsigned count, u32 incr,
		     u64 flags);
	/**
	 * @pages_addr:
	 *
	 * DMA addresses to use for mapping, used during VM update by CPU
	 */
	dma_addr_t *pages_addr;

	/**
	 * @kptr:
	 *
	 * Kernel pointer of PD/PT BO that needs to be updated,
	 * used during VM update by CPU
	 */
	void *kptr;
};

/**
 * struct gsgpu_prt_cb - Helper to disable partial resident texture feature from a fence callback
 */
struct gsgpu_prt_cb {

	/**
	 * @ldev: gsgpu device
	 */
	struct gsgpu_device *ldev;

	/**
	 * @cb: callback
	 */
	struct dma_fence_cb cb;
};

/**
 * gsgpu_vm_bo_base_init - Adds bo to the list of bos associated with the vm
 *
 * @base: base structure for tracking BO usage in a VM
 * @vm: vm to which bo is to be added
 * @bo: gsgpu buffer object
 *
 * Initialize a bo_va_base structure and add it to the appropriate lists
 *
 */
static void gsgpu_vm_bo_base_init(struct gsgpu_vm_bo_base *base,
				   struct gsgpu_vm *vm,
				   struct gsgpu_bo *bo)
{
	base->vm = vm;
	base->bo = bo;
	INIT_LIST_HEAD(&base->bo_list);
	INIT_LIST_HEAD(&base->vm_status);

	if (!bo)
		return;

	list_add_tail(&base->bo_list, &bo->va);

	if (bo->tbo.type == ttm_bo_type_kernel)
		list_move(&base->vm_status, &vm->relocated);

	if (bo->tbo.base.resv != vm->root.base.bo->tbo.base.resv)
		return;

	if (bo->preferred_domains &
	    gsgpu_mem_type_to_domain(bo->tbo.mem.mem_type))
		return;

	/*
	 * we checked all the prerequisites, but it looks like this per vm bo
	 * is currently evicted. add the bo to the evicted list to make sure it
	 * is validated on next vm use to avoid fault.
	 * */
	list_move_tail(&base->vm_status, &vm->evicted);
	base->moved = true;
}

static u32 gsgpu_get_pde_pte_size (struct gsgpu_device *ldev)
{
	return ldev->vm_manager.pde_pte_bytes;
}

/**
 * gsgpu_vm_level_shift - return the addr shift for each level
 *
 * @ldev: gsgpu_device pointer
 * @level: VMPT level
 *
 * Returns:
 * The number of bits the pfn needs to be right shifted for a level.
 */
static unsigned gsgpu_vm_level_shift(struct gsgpu_device *ldev,
				      unsigned level)
{
	unsigned shift = 0xff;

	switch (level) {
	case GSGPU_VM_DIR0:
		shift = ldev->vm_manager.dir0_shift - ldev->vm_manager.dir2_shift;
		break;
	case GSGPU_VM_DIR1:
		shift = ldev->vm_manager.dir1_shift - ldev->vm_manager.dir2_shift;
		break;
	case GSGPU_VM_DIR2:
		shift = 0;
		break;
	default:
		dev_err(ldev->dev, "the level%d isn't supported.\n", level);
	}

	return shift;
}

/**
 * gsgpu_vm_num_entries - return the number of entries in a PD/PT
 *
 * @ldev: gsgpu_device pointer
 * @level: VMPT level
 *
 * Returns:
 * The number of entries in a page directory or page table.
 */
static unsigned gsgpu_vm_num_entries(struct gsgpu_device *ldev,
				      unsigned level)
{
	unsigned width = 0;

	switch (level) {
	case GSGPU_VM_DIR0:
		width = ldev->vm_manager.dir0_width;
		break;
	case GSGPU_VM_DIR1:
		width = ldev->vm_manager.dir1_width;
		break;
	case GSGPU_VM_DIR2:
		width = ldev->vm_manager.dir2_width;
		break;
	default:
		dev_err(ldev->dev, "the level%d isn't supported.\n", level);
	}

	return 1 << width;
}

/**
 * gsgpu_vm_bo_size - returns the size of the BOs in bytes
 *
 * @ldev: gsgpu_device pointer
 * @level: VMPT level
 *
 * Returns:
 * The size of the BO for a page directory or page table in bytes.
 */
static unsigned gsgpu_vm_bo_size(struct gsgpu_device *ldev, unsigned level)
{
	return GSGPU_GPU_PAGE_ALIGN(gsgpu_vm_num_entries(ldev, level) *
				gsgpu_get_pde_pte_size(ldev));
}

/**
 * gsgpu_vm_get_pd_bo - add the VM PD to a validation list
 *
 * @vm: vm providing the BOs
 * @validated: head of validation list
 * @entry: entry to add
 *
 * Add the page directory to the list of BOs to
 * validate for command submission.
 */
void gsgpu_vm_get_pd_bo(struct gsgpu_vm *vm,
			 struct list_head *validated,
			 struct gsgpu_bo_list_entry *entry)
{
	entry->robj = vm->root.base.bo;
	entry->priority = 0;
	entry->tv.bo = &entry->robj->tbo;
	entry->tv.shared = true;
	entry->user_pages = NULL;
	list_add(&entry->tv.head, validated);
}

/**
 * gsgpu_vm_validate_pt_bos - validate the page table BOs
 *
 * @ldev: gsgpu device pointer
 * @vm: vm providing the BOs
 * @validate: callback to do the validation
 * @param: parameter for the validation callback
 *
 * Validate the page table BOs on command submission if neccessary.
 *
 * Returns:
 * Validation result.
 */
int gsgpu_vm_validate_pt_bos(struct gsgpu_device *ldev, struct gsgpu_vm *vm,
			      int (*validate)(void *p, struct gsgpu_bo *bo),
			      void *param)
{
	struct ttm_bo_global *glob = ldev->mman.bdev.glob;
	struct gsgpu_vm_bo_base *bo_base, *tmp;
	int r = 0;

	list_for_each_entry_safe(bo_base, tmp, &vm->evicted, vm_status) {
		struct gsgpu_bo *bo = bo_base->bo;

		if (bo->parent) {
			r = validate(param, bo);
			if (r)
				break;

			spin_lock(&glob->lru_lock);
			ttm_bo_move_to_lru_tail(&bo->tbo);
			if (bo->shadow)
				ttm_bo_move_to_lru_tail(&bo->shadow->tbo);
			spin_unlock(&glob->lru_lock);
		}

		if (bo->tbo.type != ttm_bo_type_kernel) {
			spin_lock(&vm->moved_lock);
			list_move(&bo_base->vm_status, &vm->moved);
			spin_unlock(&vm->moved_lock);
		} else {
			list_move(&bo_base->vm_status, &vm->relocated);
		}
	}

	spin_lock(&glob->lru_lock);
	list_for_each_entry(bo_base, &vm->idle, vm_status) {
		struct gsgpu_bo *bo = bo_base->bo;

		if (!bo->parent)
			continue;

		ttm_bo_move_to_lru_tail(&bo->tbo);
		if (bo->shadow)
			ttm_bo_move_to_lru_tail(&bo->shadow->tbo);
	}
	spin_unlock(&glob->lru_lock);

	return r;
}

/**
 * gsgpu_vm_ready - check VM is ready for updates
 *
 * @vm: VM to check
 *
 * Check if all VM PDs/PTs are ready for updates
 *
 * Returns:
 * True if eviction list is empty.
 */
bool gsgpu_vm_ready(struct gsgpu_vm *vm)
{
	return list_empty(&vm->evicted);
}

/**
 * gsgpu_vm_clear_bo - initially clear the PDs/PTs
 *
 * @ldev: gsgpu_device pointer
 * @vm: VM to clear BO from
 * @bo: BO to clear
 * @level: level this BO is at
 *
 * Root PD needs to be reserved when calling this.
 *
 * Returns:
 * 0 on success, errno otherwise.
 */
static int gsgpu_vm_clear_bo(struct gsgpu_device *ldev,
			      struct gsgpu_vm *vm, struct gsgpu_bo *bo,
			      unsigned level)
{
	struct ttm_operation_ctx ctx = { true, false };
	struct dma_fence *fence = NULL;
	unsigned entries;
	struct gsgpu_ring *ring;
	struct gsgpu_job *job;
	u64 addr;
	int r;

	entries = gsgpu_bo_size(bo) / gsgpu_get_pde_pte_size(ldev);

	ring = container_of(vm->entity.rq->sched, struct gsgpu_ring, sched);

	r = dma_resv_reserve_shared(bo->tbo.base.resv);
	if (r)
		return r;

	r = ttm_bo_validate(&bo->tbo, &bo->placement, &ctx);
	if (r)
		goto error;

	r = gsgpu_job_alloc_with_ib(ldev, 64, &job);
	if (r)
		goto error;

	addr = gsgpu_bo_gpu_offset(bo);

	if (entries)
		gsgpu_vm_set_pte_pde(ldev, &job->ibs[0], addr, 0,
				      entries, GSGPU_GPU_PAGE_SIZE, 0);

	gsgpu_ring_pad_ib(ring, &job->ibs[0]);

	WARN_ON(job->ibs[0].length_dw > 64);
	r = gsgpu_sync_resv(ldev, &job->sync, bo->tbo.base.resv,
			     GSGPU_FENCE_OWNER_UNDEFINED, false);
	if (r)
		goto error_free;

	r = gsgpu_job_submit(job, &vm->entity, GSGPU_FENCE_OWNER_UNDEFINED, &fence);
	if (r)
		goto error_free;

	gsgpu_bo_fence(bo, fence, true);
	dma_fence_put(fence);

	if (bo->shadow)
		return gsgpu_vm_clear_bo(ldev, vm, bo->shadow, level);

	return 0;

error_free:
	gsgpu_job_free(job);

error:
	return r;
}

/**
 * gsgpu_vm_alloc_levels - allocate the PD/PT levels
 *
 * @ldev: gsgpu_device pointer
 * @vm: requested vm
 * @parent: parent PT
 * @saddr: start of the address range
 * @eaddr: end of the address range
 * @level: VMPT level
 *
 * Make sure the page directories and page tables are allocated
 *
 * Returns:
 * 0 on success, errno otherwise.
 */
static int gsgpu_vm_alloc_levels(struct gsgpu_device *ldev,
				  struct gsgpu_vm *vm,
				  struct gsgpu_vm_pt *parent,
				  u64 saddr, u64 eaddr,
				  unsigned level)
{
	unsigned shift = gsgpu_vm_level_shift(ldev, level);
	unsigned pt_idx, from, to;
	u64 flags;
	int r;

	if (!parent->entries) {
		unsigned num_entries = gsgpu_vm_num_entries(ldev, level);

		parent->entries = kvmalloc_array(num_entries,
						   sizeof(struct gsgpu_vm_pt),
						   GFP_KERNEL | __GFP_ZERO);
		if (!parent->entries)
			return -ENOMEM;
	}

	from = saddr >> shift;
	to = eaddr >> shift;
	if (from >= gsgpu_vm_num_entries(ldev, level) ||
	    to >= gsgpu_vm_num_entries(ldev, level))
		return -EINVAL;

	++level;
	saddr = saddr & ((1 << shift) - 1);
	eaddr = eaddr & ((1 << shift) - 1);

	flags = GSGPU_GEM_CREATE_VRAM_CONTIGUOUS;
	if (vm->root.base.bo->shadow)
		flags |= GSGPU_GEM_CREATE_SHADOW;
	if (vm->use_cpu_for_update)
		flags |= GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED;
	else
		flags |= GSGPU_GEM_CREATE_NO_CPU_ACCESS;

	/**
	 * walk over the address space and allocate the page tables
	 */
	for (pt_idx = from; pt_idx <= to; ++pt_idx) {
		struct dma_resv *resv = vm->root.base.bo->tbo.base.resv;
		struct gsgpu_vm_pt *entry = &parent->entries[pt_idx];
		struct gsgpu_bo *pt;

		if (!entry->base.bo) {
			struct gsgpu_bo_param bp;

			memset(&bp, 0, sizeof(bp));
			bp.size = gsgpu_vm_bo_size(ldev, level);
			bp.byte_align = GSGPU_GPU_PAGE_SIZE;
			bp.domain = GSGPU_GEM_DOMAIN_VRAM;
			bp.flags = flags;
			bp.type = ttm_bo_type_kernel;
			bp.resv = resv;
			r = gsgpu_bo_create(ldev, &bp, &pt);
			if (r)
				return r;

			r = gsgpu_vm_clear_bo(ldev, vm, pt, level);
			if (r) {
				gsgpu_bo_unref(&pt->shadow);
				gsgpu_bo_unref(&pt);
				return r;
			}

			if (vm->use_cpu_for_update) {
				r = gsgpu_bo_kmap(pt, NULL);
				if (r) {
					gsgpu_bo_unref(&pt->shadow);
					gsgpu_bo_unref(&pt);
					return r;
				}
			}

			/**
			 * Keep a reference to the root directory to avoid
			 * freeing them up in the wrong order.
			 */
			pt->parent = gsgpu_bo_ref(parent->base.bo);

			gsgpu_vm_bo_base_init(&entry->base, vm, pt);
		}

		if (level < GSGPU_VM_DIR2) {
			u64 sub_saddr = (pt_idx == from) ? saddr : 0;
			u64 sub_eaddr = (pt_idx == to) ? eaddr :
				((1 << shift) - 1);
			r = gsgpu_vm_alloc_levels(ldev, vm, entry, sub_saddr,
						   sub_eaddr, level);
			if (r)
				return r;
		}
	}

	return 0;
}

/**
 * gsgpu_vm_alloc_pts - Allocate page tables.
 *
 * @ldev: gsgpu_device pointer
 * @vm: VM to allocate page tables for
 * @saddr: Start address which needs to be allocated
 * @size: Size from start address we need.
 *
 * Make sure the page tables are allocated.
 *
 * Returns:
 * 0 on success, errno otherwise.
 */
int gsgpu_vm_alloc_pts(struct gsgpu_device *ldev,
			struct gsgpu_vm *vm,
			u64 saddr, u64 size)
{
	u64 eaddr;

	/**
	 * validate the parameters
	 */
	if (saddr & GSGPU_GPU_PAGE_MASK || size & GSGPU_GPU_PAGE_MASK)
		return -EINVAL;

	eaddr = saddr + size - 1;

	saddr /= GSGPU_GPU_PAGE_SIZE;
	eaddr /= GSGPU_GPU_PAGE_SIZE;

	if (eaddr >= ldev->vm_manager.max_pfn) {
		dev_err(ldev->dev, "va above limit (0x%08llX >= 0x%08llX)\n",
			eaddr, ldev->vm_manager.max_pfn);
		return -EINVAL;
	}

	return gsgpu_vm_alloc_levels(ldev, vm, &vm->root, saddr, eaddr,
				      ldev->vm_manager.root_level);
}

/**
 * gsgpu_vm_need_pipeline_sync - Check if pipe sync is needed for job.
 *
 * @ring: ring on which the job will be submitted
 * @job: job to submit
 *
 * Returns:
 * True if sync is needed.
 */
bool gsgpu_vm_need_pipeline_sync(struct gsgpu_ring *ring,
				  struct gsgpu_job *job)
{
	struct gsgpu_device *ldev = ring->adev;
	struct gsgpu_vmid_mgr *id_mgr = &ldev->vm_manager.id_mgr;
	struct gsgpu_vmid *id;
	bool vm_flush_needed = job->vm_needs_flush;

	if (job->vmid == 0)
		return false;

	id = &id_mgr->ids[job->vmid];

	if (gsgpu_vmid_had_gpu_reset(ldev, id))
		return true;

	return vm_flush_needed;
}

/**
 * gsgpu_vm_flush - hardware flush the vm
 *
 * @ring: ring to use for flush
 * @job:  related job
 * @need_pipe_sync: is pipe sync needed
 *
 * Emit a VM flush when it is necessary.
 *
 * Returns:
 * 0 on success, errno otherwise.
 */
int gsgpu_vm_flush(struct gsgpu_ring *ring, struct gsgpu_job *job, bool need_pipe_sync)
{
	struct gsgpu_device *ldev = ring->adev;
	struct gsgpu_vmid_mgr *id_mgr = &ldev->vm_manager.id_mgr;
	struct gsgpu_vmid *id = &id_mgr->ids[job->vmid];
	bool vm_flush_needed = job->vm_needs_flush;
	struct dma_fence *fence = NULL;
	int r;
	bool pasid_mapping_needed = false;

	if (gsgpu_vmid_had_gpu_reset(ldev, id)) {
		vm_flush_needed = true;
		pasid_mapping_needed = true;
	}

	mutex_lock(&id_mgr->lock);
	if (id->pasid != job->pasid || !id->pasid_mapping ||
	    !dma_fence_is_signaled(id->pasid_mapping))
		pasid_mapping_needed = true;
	mutex_unlock(&id_mgr->lock);

	vm_flush_needed &= !!ring->funcs->emit_vm_flush  &&
			job->vm_pd_addr != GSGPU_BO_INVALID_OFFSET;

	pasid_mapping_needed &= ldev->gmc.gmc_funcs->emit_pasid_mapping &&
		ring->funcs->emit_wreg;

	if (!vm_flush_needed && !need_pipe_sync)
		return 0;

	// if (need_pipe_sync)
	// 	gsgpu_ring_emit_pipeline_sync(ring);

	if (pasid_mapping_needed)
		gsgpu_gmc_emit_pasid_mapping(ring, job->vmid, job->pasid);

	if (vm_flush_needed || pasid_mapping_needed) {
		trace_gsgpu_vm_flush(ring, job->vmid, job->vm_pd_addr);
		gsgpu_ring_emit_vm_flush(ring, job->vmid, job->vm_pd_addr);
	}

	if (vm_flush_needed) {
		r = gsgpu_fence_emit(ring, &fence, 0);
		if (r)
			return r;
	}

	if (vm_flush_needed) {
		mutex_lock(&id_mgr->lock);
		dma_fence_put(id->last_flush);
		id->last_flush = dma_fence_get(fence);
		id->current_gpu_reset_count =
			atomic_read(&ldev->gpu_reset_counter);
		mutex_unlock(&id_mgr->lock);
	}


	if (pasid_mapping_needed) {
		mutex_lock(&id_mgr->lock);
		id->pasid = job->pasid;
		dma_fence_put(id->pasid_mapping);
		id->pasid_mapping = dma_fence_get(fence);
		mutex_unlock(&id_mgr->lock);
	}

	dma_fence_put(fence);

	return 0;
}

/**
 * gsgpu_vm_bo_find - find the bo_va for a specific vm & bo
 *
 * @vm: requested vm
 * @bo: requested buffer object
 *
 * Find @bo inside the requested vm.
 * Search inside the @bos vm list for the requested vm
 * Returns the found bo_va or NULL if none is found
 *
 * Object has to be reserved!
 *
 * Returns:
 * Found bo_va or NULL.
 */
struct gsgpu_bo_va *gsgpu_vm_bo_find(struct gsgpu_vm *vm,
				       struct gsgpu_bo *bo)
{
	struct gsgpu_bo_va *bo_va;

	list_for_each_entry(bo_va, &bo->va, base.bo_list) {
		if (bo_va->base.vm == vm) {
			return bo_va;
		}
	}
	return NULL;
}

/**
 * gsgpu_vm_do_set_ptes - helper to call the right asic function
 *
 * @params: see gsgpu_pte_update_params definition
 * @bo: PD/PT to update
 * @pe: addr of the page entry
 * @addr: dst addr to write into pe
 * @count: number of page entries to update
 * @incr: increase next addr by incr bytes
 * @flags: hw access flags
 *
 * Traces the parameters and calls the right asic functions
 * to setup the page table using the DMA.
 */
static void gsgpu_vm_do_set_ptes(struct gsgpu_pte_update_params *params,
				  struct gsgpu_bo *bo,
				  u64 pe, u64 addr,
				  unsigned count, u32 incr,
				  u64 flags)
{
	pe += gsgpu_bo_gpu_offset(bo);
	trace_gsgpu_vm_set_ptes(pe, addr, count, incr, flags);

	gsgpu_vm_set_pte_pde(params->ldev, params->ib, pe, addr,
			      count, incr, flags);
}

/**
 * gsgpu_vm_do_copy_ptes - copy the PTEs from the GART
 *
 * @params: see gsgpu_pte_update_params definition
 * @bo: PD/PT to update
 * @pe: addr of the page entry
 * @addr: dst addr to write into pe
 * @count: number of page entries to update
 * @incr: increase next addr by incr bytes
 * @flags: hw access flags
 *
 * Traces the parameters and calls the DMA function to copy the PTEs.
 */
static void gsgpu_vm_do_copy_ptes(struct gsgpu_pte_update_params *params,
				   struct gsgpu_bo *bo,
				   u64 pe, u64 addr,
				   unsigned count, u32 incr,
				   u64 flags)
{
	u64 src = (params->src +
				(addr >> GSGPU_GPU_PAGE_SHIFT) *
				gsgpu_get_pde_pte_size(params->ldev));

	pe += gsgpu_bo_gpu_offset(bo);
	trace_gsgpu_vm_copy_ptes(pe, src, count);

	gsgpu_vm_copy_pte(params->ldev, params->ib, pe, src, count);
}

/**
 * gsgpu_vm_map_gart - Resolve gart mapping of addr
 *
 * @pages_addr: optional DMA address to use for lookup
 * @addr: the unmapped addr
 *
 * Look up the physical address of the page that the pte resolves
 * to.
 *
 * Returns:
 * The pointer for the page table entry.
 */
static u64 gsgpu_vm_map_gart(const dma_addr_t *pages_addr, u64 addr)
{
	u64 result;

	/* page table offset */
	result = pages_addr[addr >> PAGE_SHIFT];

	/* in case cpu page size != gpu page size*/
	result |= addr & (~PAGE_MASK);

	result &= ~((1ULL << GSGPU_GPU_PAGE_SHIFT) - 1);

	return result;
}

/**
 * gsgpu_vm_cpu_set_ptes - helper to update page tables via CPU
 *
 * @params: see gsgpu_pte_update_params definition
 * @bo: PD/PT to update
 * @pe: kmap addr of the page entry
 * @addr: dst addr to write into pe
 * @count: number of page entries to update
 * @incr: increase next addr by incr bytes
 * @flags: hw access flags
 *
 * Write count number of PT/PD entries directly.
 */
static void gsgpu_vm_cpu_set_ptes(struct gsgpu_pte_update_params *params,
				   struct gsgpu_bo *bo,
				   u64 pe, u64 addr,
				   unsigned count, u32 incr,
				   u64 flags)
{
	unsigned int i, r;
	void *kptr;
	u64 value;

	r = gsgpu_bo_kmap(bo, &kptr);
	if (r)
		return;
	pe += (u64)kptr;

	trace_gsgpu_vm_set_ptes(pe, addr, count, incr, flags);

	for (i = 0; i < count; i++) {
		value = params->pages_addr ?
			gsgpu_vm_map_gart(params->pages_addr, addr) : addr;
		gsgpu_gmc_set_pte_pde(params->ldev, (void *)(uintptr_t)pe,
				       i, value, flags);
		addr += incr;
	}
}


/**
 * gsgpu_vm_wait_pd - Wait for PT BOs to be free.
 *
 * @ldev: gsgpu_device pointer
 * @vm: related vm
 * @owner: fence owner
 *
 * Returns:
 * 0 on success, errno otherwise.
 */
static int gsgpu_vm_wait_pd(struct gsgpu_device *ldev, struct gsgpu_vm *vm,
			     void *owner)
{
	struct gsgpu_sync sync;
	int r;

	gsgpu_sync_create(&sync);
	gsgpu_sync_resv(ldev, &sync, vm->root.base.bo->tbo.base.resv, owner, false);
	r = gsgpu_sync_wait(&sync, true);
	gsgpu_sync_free(&sync);

	return r;
}

/*
 * gsgpu_vm_update_pde - update a single level in the hierarchy
 *
 * @param: parameters for the update
 * @vm: requested vm
 * @parent: parent directory
 * @entry: entry to update
 *
 * Makes sure the requested entry in parent is up to date.
 */
static void gsgpu_vm_update_pde(struct gsgpu_pte_update_params *params,
				 struct gsgpu_vm *vm,
				 struct gsgpu_vm_pt *parent,
				 struct gsgpu_vm_pt *entry)
{
	struct gsgpu_bo *bo = parent->base.bo, *pbo;
	u64 pde, pt, flags;
	unsigned level;

	/* Don't update huge pages here */
	if (entry->huge)
		return;

	for (level = 0, pbo = bo->parent; pbo; ++level)
		pbo = pbo->parent;

	level += params->ldev->vm_manager.root_level;
	pt = gsgpu_bo_gpu_offset(entry->base.bo);
	flags = GSGPU_PTE_PRESENT;
	gsgpu_gmc_get_vm_pde(params->ldev, level, &pt, &flags);
	pde = (entry - parent->entries) * gsgpu_get_pde_pte_size(params->ldev);
	if (bo->shadow)
		params->func(params, bo->shadow, pde, pt, 1, 0, flags);
	params->func(params, bo, pde, pt, 1, 0, flags);
}

/*
 * gsgpu_vm_invalidate_level - mark all PD levels as invalid
 *
 * @ldev: gsgpu_device pointer
 * @vm: related vm
 * @parent: parent PD
 * @level: VMPT level
 *
 * Mark all PD level as invalid after an error.
 */
static void gsgpu_vm_invalidate_level(struct gsgpu_device *ldev,
				       struct gsgpu_vm *vm,
				       struct gsgpu_vm_pt *parent,
				       unsigned level)
{
	unsigned pt_idx, num_entries;

	/*
	 * Recurse into the subdirectories. This recursion is harmless because
	 * we only have a maximum of 5 layers.
	 */
	num_entries = gsgpu_vm_num_entries(ldev, level);
	for (pt_idx = 0; pt_idx < num_entries; ++pt_idx) {
		struct gsgpu_vm_pt *entry = &parent->entries[pt_idx];

		if (!entry->base.bo)
			continue;

		if (!entry->base.moved)
			list_move(&entry->base.vm_status, &vm->relocated);
		gsgpu_vm_invalidate_level(ldev, vm, entry, level + 1);
	}
}

/*
 * gsgpu_vm_update_directories - make sure that all directories are valid
 *
 * @ldev: gsgpu_device pointer
 * @vm: requested vm
 *
 * Makes sure all directories are up to date.
 *
 * Returns:
 * 0 for success, error for failure.
 */
int gsgpu_vm_update_directories(struct gsgpu_device *ldev,
				 struct gsgpu_vm *vm)
{
	struct gsgpu_pte_update_params params;
	struct gsgpu_job *job;
	unsigned ndw = 0;
	int r = 0;

	if (list_empty(&vm->relocated))
		return 0;

restart:
	memset(&params, 0, sizeof(params));
	params.ldev = ldev;

	if (vm->use_cpu_for_update) {
		struct gsgpu_vm_bo_base *bo_base;

		list_for_each_entry(bo_base, &vm->relocated, vm_status) {
			r = gsgpu_bo_kmap(bo_base->bo, NULL);
			if (unlikely(r))
				return r;
		}

		r = gsgpu_vm_wait_pd(ldev, vm, GSGPU_FENCE_OWNER_VM);
		if (unlikely(r))
			return r;

		params.func = gsgpu_vm_cpu_set_ptes;
	} else {
		ndw = 512 * 8; //TODO
		r = gsgpu_job_alloc_with_ib(ldev, ndw * GSGPU_BYTES_PER_DW, &job);
		if (r)
			return r;

		params.ib = &job->ibs[0];
		params.func = gsgpu_vm_do_set_ptes;
	}

	while (!list_empty(&vm->relocated)) {
		struct gsgpu_vm_bo_base *bo_base, *parent;
		struct gsgpu_vm_pt *pt, *entry;
		struct gsgpu_bo *bo;

		bo_base = list_first_entry(&vm->relocated,
					   struct gsgpu_vm_bo_base,
					   vm_status);
		bo_base->moved = false;
		list_del_init(&bo_base->vm_status);

		bo = bo_base->bo->parent;
		if (!bo)
			continue;

		parent = list_first_entry(&bo->va, struct gsgpu_vm_bo_base,
					  bo_list);
		pt = container_of(parent, struct gsgpu_vm_pt, base);
		entry = container_of(bo_base, struct gsgpu_vm_pt, base);

		gsgpu_vm_update_pde(&params, vm, pt, entry);

		if (!vm->use_cpu_for_update &&
		    (ndw - params.ib->length_dw) < 32)
			break;
	}

	if (vm->use_cpu_for_update) {
		/* Flush HDP */
		mb();
		//TODO make sure cpu write pte is coherent
	} else if (params.ib->length_dw == 0) {
		gsgpu_job_free(job);
	} else {
		struct gsgpu_bo *root = vm->root.base.bo;
		struct gsgpu_ring *ring;
		struct dma_fence *fence;

		ring = container_of(vm->entity.rq->sched, struct gsgpu_ring,
				    sched);

		gsgpu_ring_pad_ib(ring, params.ib);
		gsgpu_sync_resv(ldev, &job->sync, root->tbo.base.resv,
				 GSGPU_FENCE_OWNER_VM, false);
		WARN_ON(params.ib->length_dw > ndw);
		r = gsgpu_job_submit(job, &vm->entity, GSGPU_FENCE_OWNER_VM, &fence);
		if (r)
			goto error;

		gsgpu_bo_fence(root, fence, true);
		dma_fence_put(vm->last_update);
		vm->last_update = fence;
	}

	if (!list_empty(&vm->relocated))
		goto restart;

	return 0;

error:
	gsgpu_vm_invalidate_level(ldev, vm, &vm->root,
				   ldev->vm_manager.root_level);
	gsgpu_job_free(job);
	return r;
}

/**
 * gsgpu_vm_find_entry - find the entry for an address
 *
 * @p: see gsgpu_pte_update_params definition
 * @addr: virtual address in question
 * @entry: resulting entry or NULL
 * @parent: parent entry
 *
 * Find the vm_pt entry and it's parent for the given address.
 */
void gsgpu_vm_get_entry(struct gsgpu_pte_update_params *p, u64 addr,
			 struct gsgpu_vm_pt **entry,
			 struct gsgpu_vm_pt **parent)
{
	unsigned level = p->ldev->vm_manager.root_level;

	*parent = NULL;
	*entry = &p->vm->root;

	while ((*entry)->entries) {
		unsigned shift = gsgpu_vm_level_shift(p->ldev, level++);

		*parent = *entry;
		*entry = &(*entry)->entries[addr >> shift];
		addr &= (1ULL << shift) - 1;
	}

	if (level != GSGPU_VM_DIR2)
		*entry = NULL;
}

/**
 * gsgpu_vm_handle_huge_pages - handle updating the PD with huge pages
 *
 * @p: see gsgpu_pte_update_params definition
 * @entry: vm_pt entry to check
 * @parent: parent entry
 * @nptes: number of PTEs updated with this operation
 * @dst: destination address where the PTEs should point to
 * @flags: access flags fro the PTEs
 *
 * Check if we can update the PD with a huge page.
 */
static void gsgpu_vm_handle_huge_pages(struct gsgpu_pte_update_params *p,
					struct gsgpu_vm_pt *entry,
					struct gsgpu_vm_pt *parent,
					unsigned nptes, u64 dst,
					u64 flags)
{
	u64 pde;

	/**
	 * In the case of a mixed PT the PDE must point to it
	 */
	if (!p->src && nptes == GSGPU_VM_PTE_COUNT(p->ldev)) {
		/**
		 * Set the huge page flag to stop scanning at this PDE
		 */
		flags |= GSGPU_PTE_HUGEPAGE;
	}

	if (!(flags & GSGPU_PTE_HUGEPAGE)) {
		if (entry->huge) {
			/**
			 * Add the entry to the relocated list to update it.
			 */
			entry->huge = false;
			list_move(&entry->base.vm_status, &p->vm->relocated);
		}
		return;
	}

	entry->huge = true;
	gsgpu_gmc_get_vm_pde(p->ldev, GSGPU_VM_DIR2, &dst, &flags);

	pde = (entry - parent->entries) * gsgpu_get_pde_pte_size(p->ldev);
	if (parent->base.bo->shadow)
		p->func(p, parent->base.bo->shadow, pde, dst, 1, 0, flags);
	p->func(p, parent->base.bo, pde, dst, 1, 0, flags);
}

/**
 * gsgpu_vm_update_ptes - make sure that page tables are valid
 *
 * @params: see gsgpu_pte_update_params definition
 * @start: start of GPU address range
 * @end: end of GPU address range
 * @dst: destination address to map to, the next dst inside the function
 * @flags: mapping flags
 *
 * Update the page tables in the range @start - @end.
 *
 * Returns:
 * 0 for success, -EINVAL for failure.
 */
static int gsgpu_vm_update_ptes(struct gsgpu_pte_update_params *params,
				  u64 start, u64 end,
				  u64 dst, u64 flags)
{
	struct gsgpu_device *ldev = params->ldev;
	const u64 mask = GSGPU_VM_PTE_COUNT(ldev) - 1;

	u64 addr, pe_start;
	struct gsgpu_bo *pt;
	unsigned nptes;

	/**
	 * walk over the address space and update the page tables
	 */
	for (addr = start; addr < end; addr += nptes,
	     dst += nptes * GSGPU_GPU_PAGE_SIZE) {
		struct gsgpu_vm_pt *entry, *parent;

		gsgpu_vm_get_entry(params, addr, &entry, &parent);
		if (!entry)
			return -ENOENT;

		if ((addr & ~mask) == (end & ~mask))
			nptes = end - addr;
		else
			nptes = GSGPU_VM_PTE_COUNT(ldev) - (addr & mask);

		gsgpu_vm_handle_huge_pages(params, entry, parent,
					    nptes, dst, flags);
		if (entry->huge)
			continue;

		pt = entry->base.bo;
		pe_start = (addr & mask) * gsgpu_get_pde_pte_size(params->ldev);
		if (pt->shadow)
			params->func(params, pt->shadow, pe_start, dst, nptes,
				     GSGPU_GPU_PAGE_SIZE, flags);
		params->func(params, pt, pe_start, dst, nptes,
			     GSGPU_GPU_PAGE_SIZE, flags);
	}

	return 0;
}

/**
 * gsgpu_vm_bo_update_mapping - update a mapping in the vm page table
 *
 * @ldev: gsgpu_device pointer
 * @exclusive: fence we need to sync to
 * @pages_addr: DMA addresses to use for mapping
 * @vm: requested vm
 * @start: start of mapped range
 * @last: last mapped entry
 * @flags: flags for the entries
 * @addr: addr to set the area to
 * @fence: optional resulting fence
 *
 * Fill in the page table entries between @start and @last.
 *
 * Returns:
 * 0 for success, -EINVAL for failure.
 */
static int gsgpu_vm_bo_update_mapping(struct gsgpu_device *ldev,
				       struct dma_fence *exclusive,
				       dma_addr_t *pages_addr,
				       struct gsgpu_vm *vm,
				       u64 start, u64 last,
				       u64 flags, u64 addr,
				       struct dma_fence **fence)
{
	struct gsgpu_ring *ring;
	void *owner = GSGPU_FENCE_OWNER_VM;
	unsigned nptes, ncmds, ndw;
	struct gsgpu_job *job;
	struct gsgpu_pte_update_params params;
	struct dma_fence *f = NULL;
	int r;

	memset(&params, 0, sizeof(params));
	params.ldev = ldev;
	params.vm = vm;

	/**
	 * sync to everything on unmapping
	 */
	if (!(flags & GSGPU_PTE_PRESENT))
		owner = GSGPU_FENCE_OWNER_UNDEFINED;

	if (vm->use_cpu_for_update) {
		/**
		 * params.src is used as flag to indicate system Memory
		 */
		if (pages_addr)
			params.src = ~0;

		/**
		 * Wait for PT BOs to be free. PTs share the same resv. object
		 * as the root PD BO
		 */
		r = gsgpu_vm_wait_pd(ldev, vm, owner);
		if (unlikely(r))
			return r;

		params.func = gsgpu_vm_cpu_set_ptes;
		params.pages_addr = pages_addr;
		return gsgpu_vm_update_ptes(&params, start, last + 1,
					   addr, flags);
	}

	ring = container_of(vm->entity.rq->sched, struct gsgpu_ring, sched);

	nptes = last - start + 1;

	/**
	 * reserve space for two commands every (1 << BLOCK_SIZE)
	 * entries or 2k dwords (whatever is smaller)
     *
     * The second command is for the shadow pagetables.
	 *
	 * formula - gsgpu_vm_update_ptes
	 */
	if (vm->root.base.bo->shadow)
		ncmds = ((nptes >> min(ldev->vm_manager.block_size, 11u)) + 2) * 2;
	else
		ncmds = ((nptes >> min(ldev->vm_manager.block_size, 11u)) + 2);

	/* ib padding, default is 8 bytes, but reserved 64 bytes. */
	ndw = 64;

	if (pages_addr) {
		/**
		 * copy commands needed
		 */
		ndw += ncmds * ldev->vm_manager.vm_pte_funcs->copy_pte_num_dw;

		/**
		 * and also PTEs
		 */
		ndw += nptes * (gsgpu_get_pde_pte_size(ldev) / GSGPU_BYTES_PER_DW);

		params.func = gsgpu_vm_do_copy_ptes;

	} else {
		/**
		 * set page commands needed
		 */
		ndw += ncmds * ldev->vm_manager.vm_pte_funcs->set_pte_pde_num_dw;

		params.func = gsgpu_vm_do_set_ptes;
	}

	r = gsgpu_job_alloc_with_ib(ldev, ndw * GSGPU_BYTES_PER_DW, &job);
	if (r)
		return r;

	params.ib = &job->ibs[0];

	if (pages_addr) {
		u64 *pte;
		unsigned i;

		/*
		 * Put the PTEs at the end of the IB
		 */
		i = ndw - nptes * (gsgpu_get_pde_pte_size(ldev) / GSGPU_BYTES_PER_DW);
		pte = (u64 *)&(job->ibs->ptr[i]);
		params.src = job->ibs->gpu_addr + i * GSGPU_BYTES_PER_DW;

		for (i = 0; i < nptes; ++i) {
			pte[i] = gsgpu_vm_map_gart(pages_addr, addr + i *
						    GSGPU_GPU_PAGE_SIZE);
			pte[i] |= flags;
		}
		addr = 0;
	}

	r = gsgpu_sync_fence(ldev, &job->sync, exclusive, false);
	if (r)
		goto error_free;

	r = gsgpu_sync_resv(ldev, &job->sync, vm->root.base.bo->tbo.base.resv,
			     owner, false);
	if (r)
		goto error_free;

	r = dma_resv_reserve_shared(vm->root.base.bo->tbo.base.resv);
	if (r)
		goto error_free;

	r = gsgpu_vm_update_ptes(&params, start, last + 1, addr, flags);
	if (r)
		goto error_free;

	gsgpu_ring_pad_ib(ring, params.ib);
	WARN_ON(params.ib->length_dw > ndw);
	r = gsgpu_job_submit(job, &vm->entity, GSGPU_FENCE_OWNER_VM, &f);
	if (r)
		goto error_free;

	gsgpu_bo_fence(vm->root.base.bo, f, true);
	dma_fence_put(*fence);
	*fence = f;

	return 0;

error_free:
	gsgpu_job_free(job);
	return r;
}

/**
 * gsgpu_vm_bo_split_mapping - split a mapping into smaller chunks
 *
 * @ldev: gsgpu_device pointer
 * @exclusive: fence we need to sync to
 * @pages_addr: DMA addresses to use for mapping
 * @vm: requested vm
 * @mapping: mapped range and flags to use for the update
 * @flags: HW flags for the mapping
 * @nodes: array of drm_mm_nodes with the MC addresses
 * @fence: optional resulting fence
 *
 * Split the mapping into smaller chunks so that each update fits
 * into a SDMA IB.
 *
 * Returns:
 * 0 for success, -EINVAL for failure.
 */
static int gsgpu_vm_bo_split_mapping(struct gsgpu_device *ldev,
				      struct dma_fence *exclusive,
				      dma_addr_t *pages_addr,
				      struct gsgpu_vm *vm,
				      struct gsgpu_bo_va_mapping *mapping,
				      u64 flags,
				      struct drm_mm_node *nodes,
				      struct dma_fence **fence)
{
	u64 pfn, start = mapping->start;
	int r;

	if (!(mapping->flags & GSGPU_PTE_WRITEABLE))
		flags &= ~GSGPU_PTE_WRITEABLE;

	trace_gsgpu_vm_bo_update(mapping);
	pfn = mapping->offset >> PAGE_SHIFT;

	/* vram - find the node and pfn with mapping->offset */
	if ((!pages_addr) && nodes) {
		while (pfn >= nodes->size) {
			pfn -= nodes->size;
			++nodes;
		}
	}

	do {
		dma_addr_t *dma_addr;
		u64 max_entries;
		u64 addr, last;

		if (pages_addr) {
			addr = pfn << PAGE_SHIFT;
			max_entries = GSGPU_VM_MAX_UPDATE_SIZE;
			dma_addr = pages_addr;
		} else if (nodes) {
			addr = nodes->start << PAGE_SHIFT;
			addr += pfn << PAGE_SHIFT;
			addr += ldev->vm_manager.vram_base_offset;
			max_entries = (nodes->size - pfn) * GSGPU_GPU_PAGES_IN_CPU_PAGE;
			dma_addr = NULL;
		} else {
			addr = 0;
			dma_addr = NULL;
		}

		last = min(mapping->last, start + max_entries - 1);
		r = gsgpu_vm_bo_update_mapping(ldev, exclusive, dma_addr, vm, start, last, flags, addr, fence);
		if (r)
			return r;

		pfn += (last - start + 1) / GSGPU_GPU_PAGES_IN_CPU_PAGE;

		if ((!pages_addr) && nodes && nodes->size == pfn) {
			pfn = 0;
			++nodes;
		}

		start = last + 1;

	} while (unlikely(start != mapping->last + 1));

	return 0;
}

/**
 * gsgpu_vm_bo_update - update all BO mappings in the vm page table
 *
 * @ldev: gsgpu_device pointer
 * @bo_va: requested BO and VM object
 * @clear: if true clear the entries
 *
 * Fill in the page table entries for @bo_va.
 *
 * Returns:
 * 0 for success, -EINVAL for failure.
 */
int gsgpu_vm_bo_update(struct gsgpu_device *ldev,
			struct gsgpu_bo_va *bo_va,
			bool clear)
{
	struct gsgpu_bo *bo = bo_va->base.bo;
	struct gsgpu_vm *vm = bo_va->base.vm;
	struct gsgpu_bo_va_mapping *mapping;
	dma_addr_t *pages_addr = NULL;
	struct ttm_resource *mem;
	struct drm_mm_node *nodes;
	struct dma_fence *exclusive, **last_update;
	u64 flags;
	int r;

	if (clear || !bo) {
		mem = NULL;
		nodes = NULL;
		exclusive = NULL;
	} else {
		mem = &bo->tbo.mem;
		nodes = mem->mm_node;
		if (mem->mem_type == TTM_PL_TT) {
			pages_addr = bo->tbo.ttm->dma_address;
		}

		exclusive = dma_resv_get_excl(bo->tbo.base.resv);
	}

	if (bo)
		flags = gsgpu_ttm_tt_pte_flags(ldev, bo->tbo.ttm, mem);
	else
		flags = 0x0;

	if (bo && bo->flags & GSGPU_GEM_CREATE_COMPRESSED_MASK)
		flags |= (bo->flags & GSGPU_GEM_CREATE_COMPRESSED_MASK)
				  >> GSGPU_PTE_COMPRESSED_SHIFT;

	if (clear || (bo && bo->tbo.base.resv == vm->root.base.bo->tbo.base.resv))
		last_update = &vm->last_update;
	else
		last_update = &bo_va->last_pt_update;

	if (!clear && bo_va->base.moved) {
		bo_va->base.moved = false;
		list_splice_init(&bo_va->valids, &bo_va->invalids);

	} else if (bo_va->cleared != clear) {
		list_splice_init(&bo_va->valids, &bo_va->invalids);
	}

	list_for_each_entry(mapping, &bo_va->invalids, list) {
		r = gsgpu_vm_bo_split_mapping(ldev, exclusive, pages_addr, vm,
					       mapping, flags, nodes,
					       last_update);
		if (r)
			return r;
	}


	spin_lock(&vm->moved_lock);
	list_del_init(&bo_va->base.vm_status);
	spin_unlock(&vm->moved_lock);

	/*
	 * If the BO is not in its preferred location add it back to
	 * the evicted list so that it gets validated again on the
	 * next command submission.
	 */
	if (bo && bo->tbo.base.resv == vm->root.base.bo->tbo.base.resv) {
		u32 mem_type = bo->tbo.mem.mem_type;

		if (!(bo->preferred_domains & gsgpu_mem_type_to_domain(mem_type)))
			list_add_tail(&bo_va->base.vm_status, &vm->evicted);
		else
			list_add(&bo_va->base.vm_status, &vm->idle);
	}

	list_splice_init(&bo_va->invalids, &bo_va->valids);
	bo_va->cleared = clear;

	if (trace_gsgpu_vm_bo_mapping_enabled()) {
		list_for_each_entry(mapping, &bo_va->valids, list)
			trace_gsgpu_vm_bo_mapping(mapping);
	}

	return 0;
}

/**
 * gsgpu_vm_free_mapping - free a mapping
 *
 * @ldev: gsgpu_device pointer
 * @vm: requested vm
 * @mapping: mapping to be freed
 * @fence: fence of the unmap operation
 *
 * Free a mapping and make sure we decrease the PRT usage count if applicable.
 */
static void gsgpu_vm_free_mapping(struct gsgpu_device *ldev,
				   struct gsgpu_vm *vm,
				   struct gsgpu_bo_va_mapping *mapping,
				   struct dma_fence *fence)
{
	kfree(mapping);
}

/**
 * gsgpu_vm_clear_freed - clear freed BOs in the PT
 *
 * @ldev: gsgpu_device pointer
 * @vm: requested vm
 * @fence: optional resulting fence (unchanged if no work needed to be done
 * or if an error occurred)
 *
 * Make sure all freed BOs are cleared in the PT.
 * PTs have to be reserved and mutex must be locked!
 *
 * Returns:
 * 0 for success.
 *
 */
int gsgpu_vm_clear_freed(struct gsgpu_device *ldev,
			  struct gsgpu_vm *vm,
			  struct dma_fence **fence)
{
	struct gsgpu_bo_va_mapping *mapping;
	u64 init_pte_value = 0;
	struct dma_fence *f = NULL;
	int r;

	while (!list_empty(&vm->freed)) {
		mapping = list_first_entry(&vm->freed,
			struct gsgpu_bo_va_mapping, list);
		list_del(&mapping->list);

		r = gsgpu_vm_bo_update_mapping(ldev, NULL, NULL, vm,
						mapping->start, mapping->last,
						init_pte_value, 0, &f);
		gsgpu_vm_free_mapping(ldev, vm, mapping, f);
		if (r) {
			dma_fence_put(f);
			return r;
		}
	}

	if (fence && f) {
		dma_fence_put(*fence);
		*fence = f;
	} else {
		dma_fence_put(f);
	}

	return 0;

}

/**
 * gsgpu_vm_handle_moved - handle moved BOs in the PT
 *
 * @ldev: gsgpu_device pointer
 * @vm: requested vm
 *
 * Make sure all BOs which are moved are updated in the PTs.
 *
 * Returns:
 * 0 for success.
 *
 * PTs have to be reserved!
 */
int gsgpu_vm_handle_moved(struct gsgpu_device *ldev,
			   struct gsgpu_vm *vm)
{
	struct gsgpu_bo_va *bo_va, *tmp;
	struct list_head moved;
	bool clear;
	int r;

	INIT_LIST_HEAD(&moved);
	spin_lock(&vm->moved_lock);
	list_splice_init(&vm->moved, &moved);
	spin_unlock(&vm->moved_lock);

	list_for_each_entry_safe(bo_va, tmp, &moved, base.vm_status) {
		struct dma_resv *resv = bo_va->base.bo->tbo.base.resv;

		/* Per VM BOs never need to bo cleared in the page tables */
		if (resv == vm->root.base.bo->tbo.base.resv)
			clear = false;
		/* Try to reserve the BO to avoid clearing its ptes */
		else if (!gsgpu_vm_debug && dma_resv_trylock(resv))
			clear = false;
		/* Somebody else is using the BO right now */
		else
			clear = true;

		r = gsgpu_vm_bo_update(ldev, bo_va, clear);
		if (r) {
			spin_lock(&vm->moved_lock);
			list_splice(&moved, &vm->moved);
			spin_unlock(&vm->moved_lock);
			return r;
		}

		if (!clear && resv != vm->root.base.bo->tbo.base.resv)
			dma_resv_unlock(resv);

	}

	return 0;
}

/**
 * gsgpu_vm_bo_add - add a bo to a specific vm
 *
 * @ldev: gsgpu_device pointer
 * @vm: requested vm
 * @bo: gsgpu buffer object
 *
 * Add @bo into the requested vm.
 * Add @bo to the list of bos associated with the vm
 *
 * Returns:
 * Newly added bo_va or NULL for failure
 *
 * Object has to be reserved!
 */
struct gsgpu_bo_va *gsgpu_vm_bo_add(struct gsgpu_device *ldev,
				      struct gsgpu_vm *vm,
				      struct gsgpu_bo *bo)
{
	struct gsgpu_bo_va *bo_va;

	bo_va = kzalloc(sizeof(struct gsgpu_bo_va), GFP_KERNEL);
	if (bo_va == NULL) {
		return NULL;
	}
	gsgpu_vm_bo_base_init(&bo_va->base, vm, bo);

	bo_va->ref_count = 1;
	INIT_LIST_HEAD(&bo_va->valids);
	INIT_LIST_HEAD(&bo_va->invalids);

	return bo_va;
}


/**
 * gsgpu_vm_bo_insert_mapping - insert a new mapping
 *
 * @ldev: gsgpu_device pointer
 * @bo_va: bo_va to store the address
 * @mapping: the mapping to insert
 *
 * Insert a new mapping into all structures.
 */
static void gsgpu_vm_bo_insert_map(struct gsgpu_device *ldev,
				    struct gsgpu_bo_va *bo_va,
				    struct gsgpu_bo_va_mapping *mapping)
{
	struct gsgpu_vm *vm = bo_va->base.vm;
	struct gsgpu_bo *bo = bo_va->base.bo;

	mapping->bo_va = bo_va;
	list_add(&mapping->list, &bo_va->invalids);
	gsgpu_vm_it_insert(mapping, &vm->va);

	if (bo && bo->tbo.base.resv == vm->root.base.bo->tbo.base.resv &&
	    !bo_va->base.moved) {
		spin_lock(&vm->moved_lock);
		list_move(&bo_va->base.vm_status, &vm->moved);
		spin_unlock(&vm->moved_lock);
	}
	trace_gsgpu_vm_bo_map(bo_va, mapping);
}

/**
 * gsgpu_vm_bo_map - map bo inside a vm
 *
 * @ldev: gsgpu_device pointer
 * @bo_va: bo_va to store the address
 * @saddr: where to map the BO
 * @offset: requested offset in the BO
 * @size: BO size in bytes
 * @flags: attributes of pages (read/write/valid/etc.)
 *
 * Add a mapping of the BO at the specefied addr into the VM.
 *
 * Returns:
 * 0 for success, error for failure.
 *
 * Object has to be reserved and unreserved outside!
 */
int gsgpu_vm_bo_map(struct gsgpu_device *ldev,
		     struct gsgpu_bo_va *bo_va,
		     u64 saddr, u64 offset,
		     u64 size, u64 flags)
{
	struct gsgpu_bo_va_mapping *mapping, *tmp;
	struct gsgpu_bo *bo = bo_va->base.bo;
	struct gsgpu_vm *vm = bo_va->base.vm;
	u64 eaddr;

	/* validate the parameters */
	if (saddr & GSGPU_GPU_PAGE_MASK || offset & GSGPU_GPU_PAGE_MASK ||
	    size == 0 || size & GSGPU_GPU_PAGE_MASK)
		return -EINVAL;

	/* make sure object fit at this offset */
	eaddr = saddr + size - 1;
	if (saddr >= eaddr ||
	    (bo && offset + size > gsgpu_bo_size(bo)))
		return -EINVAL;

	saddr /= GSGPU_GPU_PAGE_SIZE;
	eaddr /= GSGPU_GPU_PAGE_SIZE;

	tmp = gsgpu_vm_it_iter_first(&vm->va, saddr, eaddr);
	if (tmp) {
		/* bo and tmp overlap, invalid addr */
		dev_err(ldev->dev, "bo %p va 0x%010Lx-0x%010Lx conflict with "
			"0x%010Lx-0x%010Lx\n", bo, saddr, eaddr,
			tmp->start, tmp->last + 1);
		return -EINVAL;
	}

	mapping = kmalloc(sizeof(*mapping), GFP_KERNEL);
	if (!mapping)
		return -ENOMEM;

	mapping->start = saddr;
	mapping->last = eaddr;
	mapping->offset = offset;
	mapping->flags = flags;

	gsgpu_vm_bo_insert_map(ldev, bo_va, mapping);

	return 0;
}

/**
 * gsgpu_vm_bo_replace_map - map bo inside a vm, replacing existing mappings
 *
 * @ldev: gsgpu_device pointer
 * @bo_va: bo_va to store the address
 * @saddr: where to map the BO
 * @offset: requested offset in the BO
 * @size: BO size in bytes
 * @flags: attributes of pages (read/write/valid/etc.)
 *
 * Add a mapping of the BO at the specefied addr into the VM. Replace existing
 * mappings as we do so.
 *
 * Returns:
 * 0 for success, error for failure.
 *
 * Object has to be reserved and unreserved outside!
 */
int gsgpu_vm_bo_replace_map(struct gsgpu_device *ldev,
			     struct gsgpu_bo_va *bo_va,
			     u64 saddr, u64 offset,
			     u64 size, u64 flags)
{
	struct gsgpu_bo_va_mapping *mapping;
	struct gsgpu_bo *bo = bo_va->base.bo;
	u64 eaddr;
	int r;

	/* validate the parameters */
	if (saddr & GSGPU_GPU_PAGE_MASK || offset & GSGPU_GPU_PAGE_MASK ||
	    size == 0 || size & GSGPU_GPU_PAGE_MASK)
		return -EINVAL;

	/* make sure object fit at this offset */
	eaddr = saddr + size - 1;
	if (saddr >= eaddr ||
	    (bo && offset + size > gsgpu_bo_size(bo)))
		return -EINVAL;

	/* Allocate all the needed memory */
	mapping = kmalloc(sizeof(*mapping), GFP_KERNEL);
	if (!mapping)
		return -ENOMEM;

	r = gsgpu_vm_bo_clear_mappings(ldev, bo_va->base.vm, saddr, size);
	if (r) {
		kfree(mapping);
		return r;
	}

	saddr /= GSGPU_GPU_PAGE_SIZE;
	eaddr /= GSGPU_GPU_PAGE_SIZE;

	mapping->start = saddr;
	mapping->last = eaddr;
	mapping->offset = offset;
	mapping->flags = flags;

	gsgpu_vm_bo_insert_map(ldev, bo_va, mapping);

	return 0;
}

/**
 * gsgpu_vm_bo_unmap - remove bo mapping from vm
 *
 * @ldev: gsgpu_device pointer
 * @bo_va: bo_va to remove the address from
 * @saddr: where to the BO is mapped
 *
 * Remove a mapping of the BO at the specefied addr from the VM.
 *
 * Returns:
 * 0 for success, error for failure.
 *
 * Object has to be reserved and unreserved outside!
 */
int gsgpu_vm_bo_unmap(struct gsgpu_device *ldev,
		       struct gsgpu_bo_va *bo_va,
		       u64 saddr)
{
	struct gsgpu_bo_va_mapping *mapping;
	struct gsgpu_vm *vm = bo_va->base.vm;
	bool valid = true;

	saddr /= GSGPU_GPU_PAGE_SIZE;

	list_for_each_entry(mapping, &bo_va->valids, list) {
		if (mapping->start == saddr)
			break;
	}

	if (&mapping->list == &bo_va->valids) {
		valid = false;

		list_for_each_entry(mapping, &bo_va->invalids, list) {
			if (mapping->start == saddr)
				break;
		}

		if (&mapping->list == &bo_va->invalids)
			return -ENOENT;
	}

	list_del(&mapping->list);
	gsgpu_vm_it_remove(mapping, &vm->va);
	mapping->bo_va = NULL;
	trace_gsgpu_vm_bo_unmap(bo_va, mapping);

	if (valid)
		list_add(&mapping->list, &vm->freed);
	else
		gsgpu_vm_free_mapping(ldev, vm, mapping,
				       bo_va->last_pt_update);

	return 0;
}

/**
 * gsgpu_vm_bo_clear_mappings - remove all mappings in a specific range
 *
 * @ldev: gsgpu_device pointer
 * @vm: VM structure to use
 * @saddr: start of the range
 * @size: size of the range
 *
 * Remove all mappings in a range, split them as appropriate.
 *
 * Returns:
 * 0 for success, error for failure.
 */
int gsgpu_vm_bo_clear_mappings(struct gsgpu_device *ldev,
				struct gsgpu_vm *vm,
				u64 saddr, u64 size)
{
	struct gsgpu_bo_va_mapping *before, *after, *tmp, *next;
	LIST_HEAD(removed);
	u64 eaddr;

	eaddr = saddr + size - 1;
	saddr /= GSGPU_GPU_PAGE_SIZE;
	eaddr /= GSGPU_GPU_PAGE_SIZE;

	/* Allocate all the needed memory */
	before = kzalloc(sizeof(*before), GFP_KERNEL);
	if (!before)
		return -ENOMEM;
	INIT_LIST_HEAD(&before->list);

	after = kzalloc(sizeof(*after), GFP_KERNEL);
	if (!after) {
		kfree(before);
		return -ENOMEM;
	}
	INIT_LIST_HEAD(&after->list);

	/* Now gather all removed mappings */
	tmp = gsgpu_vm_it_iter_first(&vm->va, saddr, eaddr);
	while (tmp) {
		/* Remember mapping split at the start */
		if (tmp->start < saddr) {
			before->start = tmp->start;
			before->last = saddr - 1;
			before->offset = tmp->offset;
			before->flags = tmp->flags;
			before->bo_va = tmp->bo_va;
			list_add(&before->list, &tmp->bo_va->invalids);
		}

		/* Remember mapping split at the end */
		if (tmp->last > eaddr) {
			after->start = eaddr + 1;
			after->last = tmp->last;
			after->offset = tmp->offset;
			after->offset += after->start - tmp->start;
			after->flags = tmp->flags;
			after->bo_va = tmp->bo_va;
			list_add(&after->list, &tmp->bo_va->invalids);
		}

		list_del(&tmp->list);
		list_add(&tmp->list, &removed);

		tmp = gsgpu_vm_it_iter_next(tmp, saddr, eaddr);
	}

	/* And free them up */
	list_for_each_entry_safe(tmp, next, &removed, list) {
		gsgpu_vm_it_remove(tmp, &vm->va);
		list_del(&tmp->list);

		if (tmp->start < saddr)
		    tmp->start = saddr;
		if (tmp->last > eaddr)
		    tmp->last = eaddr;

		tmp->bo_va = NULL;
		list_add(&tmp->list, &vm->freed);
		trace_gsgpu_vm_bo_unmap(NULL, tmp);
	}

	/* Insert partial mapping before the range */
	if (!list_empty(&before->list)) {
		gsgpu_vm_it_insert(before, &vm->va);
	} else {
		kfree(before);
	}

	/* Insert partial mapping after the range */
	if (!list_empty(&after->list)) {
		gsgpu_vm_it_insert(after, &vm->va);
	} else {
		kfree(after);
	}

	return 0;
}

/**
 * gsgpu_vm_bo_lookup_mapping - find mapping by address
 *
 * @vm: the requested VM
 * @addr: the address
 *
 * Find a mapping by it's address.
 *
 * Returns:
 * The gsgpu_bo_va_mapping matching for addr or NULL
 *
 */
struct gsgpu_bo_va_mapping *gsgpu_vm_bo_lookup_mapping(struct gsgpu_vm *vm,
							 u64 addr)
{
	return gsgpu_vm_it_iter_first(&vm->va, addr, addr);
}

/**
 * gsgpu_vm_bo_trace_cs - trace all reserved mappings
 *
 * @vm: the requested vm
 * @ticket: CS ticket
 *
 * Trace all mappings of BOs reserved during a command submission.
 */
void gsgpu_vm_bo_trace_cs(struct gsgpu_vm *vm, struct ww_acquire_ctx *ticket)
{
	struct gsgpu_bo_va_mapping *mapping;

	if (!trace_gsgpu_vm_bo_cs_enabled())
		return;

	for (mapping = gsgpu_vm_it_iter_first(&vm->va, 0, U64_MAX); mapping;
	     mapping = gsgpu_vm_it_iter_next(mapping, 0, U64_MAX)) {
		if (mapping->bo_va && mapping->bo_va->base.bo) {
			struct gsgpu_bo *bo;

			bo = mapping->bo_va->base.bo;
			if (READ_ONCE(bo->tbo.base.resv->lock.ctx) != ticket)
				continue;
		}

		trace_gsgpu_vm_bo_cs(mapping);
	}
}

/**
 * gsgpu_vm_bo_rmv - remove a bo to a specific vm
 *
 * @ldev: gsgpu_device pointer
 * @bo_va: requested bo_va
 *
 * Remove @bo_va->bo from the requested vm.
 *
 * Object have to be reserved!
 */
void gsgpu_vm_bo_rmv(struct gsgpu_device *ldev,
		      struct gsgpu_bo_va *bo_va)
{
	struct gsgpu_bo_va_mapping *mapping, *next;
	struct gsgpu_vm *vm = bo_va->base.vm;

	list_del(&bo_va->base.bo_list);

	spin_lock(&vm->moved_lock);
	list_del(&bo_va->base.vm_status);
	spin_unlock(&vm->moved_lock);

	list_for_each_entry_safe(mapping, next, &bo_va->valids, list) {
		list_del(&mapping->list);
		gsgpu_vm_it_remove(mapping, &vm->va);
		mapping->bo_va = NULL;
		trace_gsgpu_vm_bo_unmap(bo_va, mapping);
		list_add(&mapping->list, &vm->freed);
	}
	list_for_each_entry_safe(mapping, next, &bo_va->invalids, list) {
		list_del(&mapping->list);
		gsgpu_vm_it_remove(mapping, &vm->va);
		gsgpu_vm_free_mapping(ldev, vm, mapping,
				       bo_va->last_pt_update);
	}

	dma_fence_put(bo_va->last_pt_update);
	kfree(bo_va);
}

/**
 * gsgpu_vm_bo_invalidate - mark the bo as invalid
 *
 * @ldev: gsgpu_device pointer
 * @bo: gsgpu buffer object
 * @evicted: is the BO evicted
 *
 * Mark @bo as invalid.
 */
void gsgpu_vm_bo_invalidate(struct gsgpu_device *ldev,
			     struct gsgpu_bo *bo, bool evicted)
{
	struct gsgpu_vm_bo_base *bo_base;

	/* shadow bo doesn't have bo base, its validation needs its parent */
	if (bo->parent && bo->parent->shadow == bo)
		bo = bo->parent;

	list_for_each_entry(bo_base, &bo->va, bo_list) {
		struct gsgpu_vm *vm = bo_base->vm;
		bool was_moved = bo_base->moved;

		bo_base->moved = true;
		if (evicted && bo->tbo.base.resv == vm->root.base.bo->tbo.base.resv) {
			if (bo->tbo.type == ttm_bo_type_kernel)
				list_move(&bo_base->vm_status, &vm->evicted);
			else
				list_move_tail(&bo_base->vm_status,
					       &vm->evicted);
			continue;
		}

		if (was_moved)
			continue;

		if (bo->tbo.type == ttm_bo_type_kernel) {
			list_move(&bo_base->vm_status, &vm->relocated);
		} else {
			spin_lock(&bo_base->vm->moved_lock);
			list_move(&bo_base->vm_status, &vm->moved);
			spin_unlock(&bo_base->vm->moved_lock);
		}
	}
}

/**
 * gsgpu_vm_get_block_size - calculate the shift of PTEs a block contains
 *
 * @vm_size: VM size in GB
 *
 * Returns:
 * The shift of PTEs a block contains
 */
static u32 gsgpu_vm_get_block_size(u64 vm_size)
{
	(void)(vm_size);
	return GSGPU_PAGE_PTE_SHIFT;
}

/**
 * gsgpu_vm_adjust_size - adjust vm size, block size
 *
 * @ldev: gsgpu_device pointer
 * @min_vm_size: the minimum vm size in GB if it's set auto
 * @max_level: max page table levels
 * @max_bits: max address space size in bits
 *
 */
void gsgpu_vm_adjust_size(struct gsgpu_device *ldev, u32 min_vm_size,
			  unsigned max_level, unsigned max_bits)
{
	unsigned int vm_size;
	u64 tmp;

#if 0
	unsigned int max_size = 1 << (max_bits - 30);

	struct sysinfo si;
	unsigned int phys_ram_gb;

	/* Optimal VM size depends on the amount of physical
	 * RAM available. Underlying requirements and
	 * assumptions:
	 *
	 *  - Need to map system memory and VRAM from all GPUs
	 *     - VRAM from other GPUs not known here
	 *     - Assume VRAM <= system memory
	 *  - On GFX8 and older, VM space can be segmented for
	 *    different MTYPEs
	 *  - Need to allow room for fragmentation, guard pages etc.
	 *
	 * This adds up to a rough guess of system memory x3.
	 * Round up to power of two to maximize the available
	 * VM size with the given page table size.
	 */
	si_meminfo(&si);
	phys_ram_gb = ((uint64_t)si.totalram * si.mem_unit +
		       (1 << 30) - 1) >> 30;
	vm_size = roundup_pow_of_two(min(max(phys_ram_gb * 3, min_vm_size), max_size));

#else
	vm_size = 1 << (max_bits - GSGPU_GB_SHIFT_BITS);

#endif
	ldev->vm_manager.pde_pte_bytes = GSGPU_VM_PDE_PTE_BYTES;
	ldev->vm_manager.max_pfn = (u64)vm_size <<
				(GSGPU_GB_SHIFT_BITS - GSGPU_GPU_PAGE_SHIFT);

	tmp = roundup_pow_of_two(ldev->vm_manager.max_pfn);
	if (gsgpu_vm_block_size != -1)
		tmp >>= gsgpu_vm_block_size - GSGPU_PAGE_PTE_SHIFT;

	tmp = DIV_ROUND_UP(fls64(tmp), GSGPU_PAGE_PTE_SHIFT);
	ldev->vm_manager.num_level = min(max_level, (unsigned)tmp);
	ldev->vm_manager.root_level = GSGPU_VM_DIR0;
	ldev->vm_manager.dir2_width = GSGPU_PAGE_PTE_SHIFT;
	ldev->vm_manager.dir2_shift = GSGPU_GPU_PAGE_SHIFT;
	ldev->vm_manager.dir1_shift = ldev->vm_manager.dir2_shift + ldev->vm_manager.dir2_width;
	ldev->vm_manager.dir1_width = GSGPU_PAGE_PTE_SHIFT;
	ldev->vm_manager.dir0_shift = ldev->vm_manager.dir1_shift + ldev->vm_manager.dir1_width;
	ldev->vm_manager.dir0_width = max_bits - ldev->vm_manager.dir0_shift;

	/* block size depends on vm size and hw setup*/
	if (gsgpu_vm_block_size != -1)
		ldev->vm_manager.block_size =
			min((unsigned)gsgpu_vm_block_size, (unsigned)GSGPU_PAGE_PTE_SHIFT);
	else if (ldev->vm_manager.num_level > 1)
		ldev->vm_manager.block_size = GSGPU_PAGE_PTE_SHIFT;
	else
		ldev->vm_manager.block_size = gsgpu_vm_get_block_size(vm_size);

	DRM_INFO("vm size is %u GB, %u levels, block size is %u-bit\n",
		 vm_size, ldev->vm_manager.num_level + 1,
		 ldev->vm_manager.block_size);
}

/**
 * gsgpu_vm_init - initialize a vm instance
 *
 * @ldev: gsgpu_device pointer
 * @vm: requested vm
 * @vm_context: Indicates if it GFX or Compute context
 * @pasid: Process address space identifier
 *
 * Init @vm fields.
 *
 * Returns:
 * 0 for success, error for failure.
 */
int gsgpu_vm_init(struct gsgpu_device *ldev, struct gsgpu_vm *vm,
		   int vm_context, unsigned int pasid)
{
	struct gsgpu_bo_param bp;
	struct gsgpu_bo *root;
	const unsigned align = min((unsigned)GSGPU_VM_PTB_ALIGN_SIZE,
		GSGPU_VM_PTE_COUNT(ldev) * gsgpu_get_pde_pte_size(ldev));
	unsigned ring_instance;
	struct gsgpu_ring *ring;
	struct drm_sched_rq *rq;
	unsigned long size;
	u64 flags;
	int r;

	vm->va = RB_ROOT_CACHED;
	vm->reserved_vmid = NULL;
	INIT_LIST_HEAD(&vm->evicted);
	INIT_LIST_HEAD(&vm->relocated);
	spin_lock_init(&vm->moved_lock);
	INIT_LIST_HEAD(&vm->moved);
	INIT_LIST_HEAD(&vm->idle);
	INIT_LIST_HEAD(&vm->freed);

	/* create scheduler entity for page table updates */

	ring_instance = atomic_inc_return(&ldev->vm_manager.vm_pte_next_ring);
	ring_instance %= ldev->vm_manager.vm_pte_num_rings;
	ring = ldev->vm_manager.vm_pte_rings[ring_instance];
	rq = &ring->sched.sched_rq[DRM_SCHED_PRIORITY_KERNEL];
	r = drm_sched_entity_init(&vm->entity, &rq, 1, NULL);
	if (r)
		return r;

	vm->pte_support_ats = false;

	vm->use_cpu_for_update = ldev->vm_manager.vm_update_mode;

	DRM_DEBUG_DRIVER("VM update mode is %s\n",
			 vm->use_cpu_for_update ? "CPU" : "XDMA");
	WARN_ONCE((vm->use_cpu_for_update & !gsgpu_gmc_vram_full_visible(&ldev->gmc)),
		  "CPU update of VM recommended only for large BAR system\n");
	vm->last_update = NULL;

	flags = GSGPU_GEM_CREATE_VRAM_CONTIGUOUS;
	if (vm->use_cpu_for_update)
		flags |= GSGPU_GEM_CREATE_CPU_ACCESS_REQUIRED;
	else if (vm_context != GSGPU_VM_CONTEXT_COMPUTE)
		flags |= GSGPU_GEM_CREATE_SHADOW;

	size = gsgpu_vm_bo_size(ldev, ldev->vm_manager.root_level);
	memset(&bp, 0, sizeof(bp));
	bp.size = size;
	bp.byte_align = align;
	bp.domain = GSGPU_GEM_DOMAIN_VRAM;
	bp.flags = flags;
	bp.type = ttm_bo_type_kernel;
	bp.resv = NULL;
	r = gsgpu_bo_create(ldev, &bp, &root);
	if (r)
		goto error_free_sched_entity;

	r = gsgpu_bo_reserve(root, true);
	if (r)
		goto error_free_root;

	r = gsgpu_vm_clear_bo(ldev, vm, root,
			       ldev->vm_manager.root_level);
	if (r)
		goto error_unreserve;

	gsgpu_vm_bo_base_init(&vm->root.base, vm, root);
	gsgpu_bo_unreserve(vm->root.base.bo);

	if (pasid) {
		unsigned long flags;

		spin_lock_irqsave(&ldev->vm_manager.pasid_lock, flags);
		r = idr_alloc(&ldev->vm_manager.pasid_idr, vm, pasid, pasid + 1,
			      GFP_ATOMIC);
		spin_unlock_irqrestore(&ldev->vm_manager.pasid_lock, flags);
		if (r < 0)
			goto error_free_root;

		vm->pasid = pasid;
	}

	INIT_KFIFO(vm->faults);
	vm->fault_credit = 16;

	return 0;

error_unreserve:
	gsgpu_bo_unreserve(vm->root.base.bo);

error_free_root:
	gsgpu_bo_unref(&vm->root.base.bo->shadow);
	gsgpu_bo_unref(&vm->root.base.bo);
	vm->root.base.bo = NULL;

error_free_sched_entity:
	drm_sched_entity_destroy(&vm->entity);

	return r;
}

/**
 * gsgpu_vm_free_levels - free PD/PT levels
 *
 * @ldev: gsgpu device structure
 * @parent: PD/PT starting level to free
 * @level: level of parent structure
 *
 * Free the page directory or page table level and all sub levels.
 */
static void gsgpu_vm_free_levels(struct gsgpu_device *ldev,
				  struct gsgpu_vm_pt *parent,
				  unsigned level)
{
	unsigned i, num_entries = gsgpu_vm_num_entries(ldev, level);

	if (parent->base.bo) {
		list_del(&parent->base.bo_list);
		list_del(&parent->base.vm_status);
		gsgpu_bo_unref(&parent->base.bo->shadow);
		gsgpu_bo_unref(&parent->base.bo);
	}

	if (parent->entries)
		for (i = 0; i < num_entries; i++)
			gsgpu_vm_free_levels(ldev, &parent->entries[i],
					      level + 1);

	kvfree(parent->entries);
}

/**
 * gsgpu_vm_fini - tear down a vm instance
 *
 * @ldev: gsgpu_device pointer
 * @vm: requested vm
 *
 * Tear down @vm.
 * Unbind the VM and remove all bos from the vm bo list
 */
void gsgpu_vm_fini(struct gsgpu_device *ldev, struct gsgpu_vm *vm)
{
	struct gsgpu_bo_va_mapping *mapping, *tmp;
	struct gsgpu_bo *root;
	int r;

	if (vm->pasid) {
		unsigned long flags;

		spin_lock_irqsave(&ldev->vm_manager.pasid_lock, flags);
		idr_remove(&ldev->vm_manager.pasid_idr, vm->pasid);
		spin_unlock_irqrestore(&ldev->vm_manager.pasid_lock, flags);
	}

	drm_sched_entity_destroy(&vm->entity);

	if (!RB_EMPTY_ROOT(&vm->va.rb_root)) {
		dev_err(ldev->dev, "still active bo inside vm\n");
	}
	rbtree_postorder_for_each_entry_safe(mapping, tmp,
					     &vm->va.rb_root, rb) {
		list_del(&mapping->list);
		gsgpu_vm_it_remove(mapping, &vm->va);
		kfree(mapping);
	}
	list_for_each_entry_safe(mapping, tmp, &vm->freed, list) {
		list_del(&mapping->list);
		gsgpu_vm_free_mapping(ldev, vm, mapping, NULL);
	}

	root = gsgpu_bo_ref(vm->root.base.bo);
	r = gsgpu_bo_reserve(root, true);
	if (r) {
		dev_err(ldev->dev, "Leaking page tables because BO reservation failed\n");
	} else {
		gsgpu_vm_free_levels(ldev, &vm->root,
				      ldev->vm_manager.root_level);
		gsgpu_bo_unreserve(root);
	}
	gsgpu_bo_unref(&root);
	dma_fence_put(vm->last_update);
	gsgpu_vmid_free_reserved(ldev, vm);

	gsgpu_sema_free(ldev, vm);
}

/**
 * gsgpu_vm_pasid_fault_credit - Check fault credit for given PASID
 *
 * @ldev: gsgpu_device pointer
 * @pasid: PASID do identify the VM
 *
 * This function is expected to be called in interrupt context.
 *
 * Returns:
 * True if there was fault credit, false otherwise
 */
bool gsgpu_vm_pasid_fault_credit(struct gsgpu_device *ldev,
				  unsigned int pasid)
{
	struct gsgpu_vm *vm;

	spin_lock(&ldev->vm_manager.pasid_lock);
	vm = idr_find(&ldev->vm_manager.pasid_idr, pasid);
	if (!vm) {
		/* VM not found, can't track fault credit */
		spin_unlock(&ldev->vm_manager.pasid_lock);
		return true;
	}

	/* No lock needed. only accessed by IRQ handler */
	if (!vm->fault_credit) {
		/* Too many faults in this VM */
		spin_unlock(&ldev->vm_manager.pasid_lock);
		return false;
	}

	vm->fault_credit--;
	spin_unlock(&ldev->vm_manager.pasid_lock);
	return true;
}

/**
 * gsgpu_vm_manager_init - init the VM manager
 *
 * @ldev: gsgpu_device pointer
 *
 * Initialize the VM manager structures
 */
void gsgpu_vm_manager_init(struct gsgpu_device *ldev)
{
	unsigned i;

	gsgpu_vmid_mgr_init(ldev);

	ldev->vm_manager.fence_context =
		dma_fence_context_alloc(GSGPU_MAX_RINGS);

	for (i = 0; i < GSGPU_MAX_RINGS; ++i)
		ldev->vm_manager.seqno[i] = 0;

	atomic_set(&ldev->vm_manager.vm_pte_next_ring, 0);
	spin_lock_init(&ldev->vm_manager.prt_lock);
	atomic_set(&ldev->vm_manager.num_prt_users, 0);

	if (gsgpu_vm_update_mode == -1)
		if (gsgpu_gmc_vram_full_visible(&ldev->gmc))
			ldev->vm_manager.vm_update_mode = 1;
		else
			ldev->vm_manager.vm_update_mode = 0;
	else
		ldev->vm_manager.vm_update_mode = gsgpu_vm_update_mode;

	idr_init(&ldev->vm_manager.pasid_idr);
	spin_lock_init(&ldev->vm_manager.pasid_lock);
}

/**
 * gsgpu_vm_manager_fini - cleanup VM manager
 *
 * @ldev: gsgpu_device pointer
 *
 * Cleanup the VM manager and free resources.
 */
void gsgpu_vm_manager_fini(struct gsgpu_device *ldev)
{
	WARN_ON(!idr_is_empty(&ldev->vm_manager.pasid_idr));
	idr_destroy(&ldev->vm_manager.pasid_idr);

	gsgpu_vmid_mgr_fini(ldev);
}

/**
 * gsgpu_vm_ioctl - Manages VMID reservation.
 *
 * @dev: drm device pointer
 * @data: drm_gsgpu_vm
 * @filp: drm file pointer
 *
 * Returns:
 * 0 for success, -errno for errors.
 */
int gsgpu_vm_ioctl(struct drm_device *dev, void *data, struct drm_file *filp)
{
	union drm_gsgpu_vm *args = data;
	struct gsgpu_device *ldev = dev->dev_private;
	struct gsgpu_fpriv *fpriv = filp->driver_priv;
	int r;

	switch (args->in.op) {
	case GSGPU_VM_OP_RESERVE_VMID:
		/* current, we only have requirement to reserve vmid */
		r = gsgpu_vmid_alloc_reserved(ldev, &fpriv->vm);
		if (r)
			return r;
		break;
	case GSGPU_VM_OP_UNRESERVE_VMID:
		gsgpu_vmid_free_reserved(ldev, &fpriv->vm);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/**
 * gsgpu_vm_get_task_info - Extracts task info for a PASID.
 *
 * @dev: drm device pointer
 * @pasid: PASID identifier for VM
 * @task_info: task_info to fill.
 */
void gsgpu_vm_get_task_info(struct gsgpu_device *ldev, unsigned int pasid,
			 struct gsgpu_task_info *task_info)
{
	struct gsgpu_vm *vm;
	unsigned long flags;

	spin_lock_irqsave(&ldev->vm_manager.pasid_lock, flags);

	vm = idr_find(&ldev->vm_manager.pasid_idr, pasid);
	if (vm)
		*task_info = vm->task_info;

	spin_unlock_irqrestore(&ldev->vm_manager.pasid_lock, flags);
}

/**
 * gsgpu_vm_set_task_info - Sets VMs task info.
 *
 * @vm: vm for which to set the info
 */
void gsgpu_vm_set_task_info(struct gsgpu_vm *vm)
{
	if (!vm->task_info.pid) {
		vm->task_info.pid = current->pid;
		get_task_comm(vm->task_info.task_name, current);

		if (current->group_leader->mm == current->mm) {
			vm->task_info.tgid = current->group_leader->pid;
			get_task_comm(vm->task_info.process_name, current->group_leader);
		}
	}
}

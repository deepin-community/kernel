#include "gsgpu_ids.h"

#include <linux/idr.h>
#include <linux/dma-fence-array.h>
#include <drm/drmP.h>

#include "gsgpu.h"
#include "gsgpu_trace.h"

/*
 * PASID manager
 *
 * PASIDs are global address space identifiers that can be shared
 * between the GPU, an IOMMU and the driver. VMs on different devices
 * may use the same PASID if they share the same address
 * space. Therefore PASIDs are allocated using a global IDA. VMs are
 * looked up from the PASID per gsgpu_device.
 */
static DEFINE_IDA(gsgpu_pasid_ida);

/* Helper to free pasid from a fence callback */
struct gsgpu_pasid_cb {
	struct dma_fence_cb cb;
	unsigned int pasid;
};

/**
 * gsgpu_pasid_alloc - Allocate a PASID
 * @bits: Maximum width of the PASID in bits, must be at least 1
 *
 * Allocates a PASID of the given width while keeping smaller PASIDs
 * available if possible.
 *
 * Returns a positive integer on success. Returns %-EINVAL if bits==0.
 * Returns %-ENOSPC if no PASID was available. Returns %-ENOMEM on
 * memory allocation failure.
 */
int gsgpu_pasid_alloc(unsigned int bits)
{
	int pasid = -EINVAL;

	for (bits = min(bits, 31U); bits > 0; bits--) {
		pasid = ida_simple_get(&gsgpu_pasid_ida,
				       1U << (bits - 1), 1U << bits,
				       GFP_KERNEL);
		if (pasid != -ENOSPC)
			break;
	}

	if (pasid >= 0)
		trace_gsgpu_pasid_allocated(pasid);

	return pasid;
}

/**
 * gsgpu_pasid_free - Free a PASID
 * @pasid: PASID to free
 */
void gsgpu_pasid_free(unsigned int pasid)
{
	trace_gsgpu_pasid_freed(pasid);
	ida_simple_remove(&gsgpu_pasid_ida, pasid);
}

static void gsgpu_pasid_free_cb(struct dma_fence *fence,
				 struct dma_fence_cb *_cb)
{
	struct gsgpu_pasid_cb *cb =
		container_of(_cb, struct gsgpu_pasid_cb, cb);

	gsgpu_pasid_free(cb->pasid);
	dma_fence_put(fence);
	kfree(cb);
}

/**
 * gsgpu_pasid_free_delayed - free pasid when fences signal
 *
 * @resv: reservation object with the fences to wait for
 * @pasid: pasid to free
 *
 * Free the pasid only after all the fences in resv are signaled.
 */
void gsgpu_pasid_free_delayed(struct reservation_object *resv,
			       unsigned int pasid)
{
	struct dma_fence *fence, **fences;
	struct gsgpu_pasid_cb *cb;
	unsigned count;
	int r;

	r = reservation_object_get_fences_rcu(resv, NULL, &count, &fences);
	if (r)
		goto fallback;

	if (count == 0) {
		gsgpu_pasid_free(pasid);
		return;
	}

	if (count == 1) {
		fence = fences[0];
		kfree(fences);
	} else {
		uint64_t context = dma_fence_context_alloc(1);
		struct dma_fence_array *array;

		array = dma_fence_array_create(count, fences, context,
					       1, false);
		if (!array) {
			kfree(fences);
			goto fallback;
		}
		fence = &array->base;
	}

	cb = kmalloc(sizeof(*cb), GFP_KERNEL);
	if (!cb) {
		/* Last resort when we are OOM */
		dma_fence_wait(fence, false);
		dma_fence_put(fence);
		gsgpu_pasid_free(pasid);
	} else {
		cb->pasid = pasid;
		if (dma_fence_add_callback(fence, &cb->cb,
					   gsgpu_pasid_free_cb))
			gsgpu_pasid_free_cb(fence, &cb->cb);
	}

	return;

fallback:
	/* Not enough memory for the delayed delete, as last resort
	 * block for all the fences to complete.
	 */
	reservation_object_wait_timeout_rcu(resv, true, false,
					    MAX_SCHEDULE_TIMEOUT);
	gsgpu_pasid_free(pasid);
}

/*
 * VMID manager
 *
 * VMIDs are an identifier for page tables handling.
 */

/**
 * gsgpu_vmid_had_gpu_reset - check if reset occured since last use
 *
 * @adev: gsgpu_device pointer
 * @id: VMID structure
 *
 * Check if GPU reset occured since last use of the VMID.
 */
bool gsgpu_vmid_had_gpu_reset(struct gsgpu_device *adev,
			       struct gsgpu_vmid *id)
{
	return id->current_gpu_reset_count !=
		atomic_read(&adev->gpu_reset_counter);
}

/**
 * gsgpu_vm_grab_idle - grab idle VMID
 *
 * @vm: vm to allocate id for
 * @ring: ring we want to submit job to
 * @sync: sync object where we add dependencies
 * @idle: resulting idle VMID
 *
 * Try to find an idle VMID, if none is idle add a fence to wait to the sync
 * object. Returns -ENOMEM when we are out of memory.
 */
static int gsgpu_vmid_grab_idle(struct gsgpu_vm *vm,
				 struct gsgpu_ring *ring,
				 struct gsgpu_sync *sync,
				 struct gsgpu_vmid **idle)
{
	struct gsgpu_device *adev = ring->adev;
	struct gsgpu_vmid_mgr *id_mgr = &adev->vm_manager.id_mgr;
	struct dma_fence **fences;
	unsigned i;
	int r;

	if (ring->vmid_wait && !dma_fence_is_signaled(ring->vmid_wait))
		return gsgpu_sync_fence(adev, sync, ring->vmid_wait, false);

	fences = kmalloc_array(sizeof(void *), id_mgr->num_ids, GFP_KERNEL);
	if (!fences)
		return -ENOMEM;

	/* Check if we have an idle VMID */
	i = 0;
	list_for_each_entry((*idle), &id_mgr->ids_lru, list) {
		fences[i] = gsgpu_sync_peek_fence(&(*idle)->active, ring);
		if (!fences[i])
			break;
		++i;
	}

	/* If we can't find a idle VMID to use, wait till one becomes available */
	if (&(*idle)->list == &id_mgr->ids_lru) {
		u64 fence_context = adev->vm_manager.fence_context + ring->idx;
		unsigned seqno = ++adev->vm_manager.seqno[ring->idx];
		struct dma_fence_array *array;
		unsigned j;

		*idle = NULL;
		for (j = 0; j < i; ++j)
			dma_fence_get(fences[j]);

		array = dma_fence_array_create(i, fences, fence_context,
					       seqno, true);
		if (!array) {
			for (j = 0; j < i; ++j)
				dma_fence_put(fences[j]);
			kfree(fences);
			return -ENOMEM;
		}

		r = gsgpu_sync_fence(adev, sync, &array->base, false);
		dma_fence_put(ring->vmid_wait);
		ring->vmid_wait = &array->base;
		return r;
	}
	kfree(fences);

	return 0;
}

/**
 * gsgpu_vm_grab_reserved - try to assign reserved VMID
 *
 * @vm: vm to allocate id for
 * @ring: ring we want to submit job to
 * @sync: sync object where we add dependencies
 * @fence: fence protecting ID from reuse
 * @job: job who wants to use the VMID
 *
 * Try to assign a reserved VMID.
 */
static int gsgpu_vmid_grab_reserved(struct gsgpu_vm *vm,
				     struct gsgpu_ring *ring,
				     struct gsgpu_sync *sync,
				     struct dma_fence *fence,
				     struct gsgpu_job *job,
				     struct gsgpu_vmid **id)
{
	struct gsgpu_device *adev = ring->adev;
	uint64_t fence_context = adev->fence_context + ring->idx;
	struct dma_fence *updates = sync->last_vm_update;
	bool needs_flush = vm->use_cpu_for_update;
	int r = 0;

	*id = vm->reserved_vmid;
	if (updates && (*id)->flushed_updates &&
	    updates->context == (*id)->flushed_updates->context &&
	    !dma_fence_is_later(updates, (*id)->flushed_updates))
	    updates = NULL;

	if ((*id)->owner != vm->entity.fence_context ||
	    job->vm_pd_addr != (*id)->pd_gpu_addr ||
	    updates || !(*id)->last_flush ||
	    ((*id)->last_flush->context != fence_context &&
	     !dma_fence_is_signaled((*id)->last_flush))) {
		struct dma_fence *tmp;

		/* to prevent one context starved by another context */
		(*id)->pd_gpu_addr = 0;
		tmp = gsgpu_sync_peek_fence(&(*id)->active, ring);
		if (tmp) {
			*id = NULL;
			r = gsgpu_sync_fence(adev, sync, tmp, false);
			return r;
		}
		needs_flush = true;
	}

	/* Good we can use this VMID. Remember this submission as
	* user of the VMID.
	*/
	r = gsgpu_sync_fence(ring->adev, &(*id)->active, fence, false);
	if (r)
		return r;

	if (updates) {
		dma_fence_put((*id)->flushed_updates);
		(*id)->flushed_updates = dma_fence_get(updates);
	}
	job->vm_needs_flush = needs_flush;
	return 0;
}

/**
 * gsgpu_vm_grab_used - try to reuse a VMID
 *
 * @vm: vm to allocate id for
 * @ring: ring we want to submit job to
 * @sync: sync object where we add dependencies
 * @fence: fence protecting ID from reuse
 * @job: job who wants to use the VMID
 * @id: resulting VMID
 *
 * Try to reuse a VMID for this submission.
 */
static int gsgpu_vmid_grab_used(struct gsgpu_vm *vm,
				 struct gsgpu_ring *ring,
				 struct gsgpu_sync *sync,
				 struct dma_fence *fence,
				 struct gsgpu_job *job,
				 struct gsgpu_vmid **id)
{
	struct gsgpu_device *adev = ring->adev;
	struct gsgpu_vmid_mgr *id_mgr = &adev->vm_manager.id_mgr;
	uint64_t fence_context = adev->fence_context + ring->idx;
	struct dma_fence *updates = sync->last_vm_update;
	int r;

	job->vm_needs_flush = vm->use_cpu_for_update;

	/* Check if we can use a VMID already assigned to this VM */
	list_for_each_entry_reverse((*id), &id_mgr->ids_lru, list) {
		bool needs_flush = vm->use_cpu_for_update;
		struct dma_fence *flushed;

		/* Check all the prerequisites to using this VMID */
		if ((*id)->owner != vm->entity.fence_context)
			continue;

		if ((*id)->pd_gpu_addr != job->vm_pd_addr)
			continue;

		if (!(*id)->last_flush ||
		    ((*id)->last_flush->context != fence_context &&
		     !dma_fence_is_signaled((*id)->last_flush)))
			needs_flush = true;

		flushed  = (*id)->flushed_updates;
		if (updates && (!flushed || dma_fence_is_later(updates, flushed)))
			needs_flush = true;

		/* Concurrent flushes are only possible starting with Vega10 */
		// if (needs_flush)
		// 	continue;

		/* Good, we can use this VMID. Remember this submission as
		 * user of the VMID.
		 */
		r = gsgpu_sync_fence(ring->adev, &(*id)->active, fence, false);
		if (r)
			return r;

		if (updates && (!flushed || dma_fence_is_later(updates, flushed))) {
			dma_fence_put((*id)->flushed_updates);
			(*id)->flushed_updates = dma_fence_get(updates);
		}

		job->vm_needs_flush |= needs_flush;
		return 0;
	}

	*id = NULL;
	return 0;
}

/**
 * gsgpu_vm_grab_id - allocate the next free VMID
 *
 * @vm: vm to allocate id for
 * @ring: ring we want to submit job to
 * @sync: sync object where we add dependencies
 * @fence: fence protecting ID from reuse
 * @job: job who wants to use the VMID
 *
 * Allocate an id for the vm, adding fences to the sync obj as necessary.
 */
int gsgpu_vmid_grab(struct gsgpu_vm *vm, struct gsgpu_ring *ring,
		     struct gsgpu_sync *sync, struct dma_fence *fence,
		     struct gsgpu_job *job)
{
	struct gsgpu_device *adev = ring->adev;
	struct gsgpu_vmid_mgr *id_mgr = &adev->vm_manager.id_mgr;
	struct gsgpu_vmid *idle = NULL;
	struct gsgpu_vmid *id = NULL;
	int r = 0;

	mutex_lock(&id_mgr->lock);
	r = gsgpu_vmid_grab_idle(vm, ring, sync, &idle);
	if (r || !idle)
		goto error;

	if (vm->reserved_vmid) {
		r = gsgpu_vmid_grab_reserved(vm, ring, sync, fence, job, &id);
		if (r || !id)
			goto error;
	} else {
		r = gsgpu_vmid_grab_used(vm, ring, sync, fence, job, &id);
		if (r)
			goto error;

		if (!id) {
			struct dma_fence *updates = sync->last_vm_update;

			/* Still no ID to use? Then use the idle one found earlier */
			id = idle;

			/* Remember this submission as user of the VMID */
			r = gsgpu_sync_fence(ring->adev, &id->active,
					      fence, false);
			if (r)
				goto error;

			dma_fence_put(id->flushed_updates);
			id->flushed_updates = dma_fence_get(updates);
			job->vm_needs_flush = true;
		}

		list_move_tail(&id->list, &id_mgr->ids_lru);
	}

	id->pd_gpu_addr = job->vm_pd_addr;
	id->owner = vm->entity.fence_context;

	if (job->vm_needs_flush) {
		dma_fence_put(id->last_flush);
		id->last_flush = NULL;
	}
	job->vmid = id - id_mgr->ids;
	job->pasid = vm->pasid;
	trace_gsgpu_vm_grab_id(vm, ring, job);

error:
	mutex_unlock(&id_mgr->lock);
	return r;
}

int gsgpu_vmid_alloc_reserved(struct gsgpu_device *adev, struct gsgpu_vm *vm)
{
	struct gsgpu_vmid_mgr *id_mgr;
	struct gsgpu_vmid *idle;
	int r = 0;

	id_mgr = &adev->vm_manager.id_mgr;
	mutex_lock(&id_mgr->lock);
	if (vm->reserved_vmid)
		goto unlock;
	if (atomic_inc_return(&id_mgr->reserved_vmid_num) >
	    GSGPU_VM_MAX_RESERVED_VMID) {
		DRM_ERROR("Over limitation of reserved vmid\n");
		atomic_dec(&id_mgr->reserved_vmid_num);
		r = -EINVAL;
		goto unlock;
	}
	/* Select the first entry VMID */
	idle = list_first_entry(&id_mgr->ids_lru, struct gsgpu_vmid, list);
	list_del_init(&idle->list);
	vm->reserved_vmid = idle;
	mutex_unlock(&id_mgr->lock);

	return 0;
unlock:
	mutex_unlock(&id_mgr->lock);
	return r;
}

void gsgpu_vmid_free_reserved(struct gsgpu_device *adev, struct gsgpu_vm *vm)
{
	struct gsgpu_vmid_mgr *id_mgr = &adev->vm_manager.id_mgr;

	mutex_lock(&id_mgr->lock);
	if (vm->reserved_vmid) {
		list_add(&vm->reserved_vmid->list,
			&id_mgr->ids_lru);
		vm->reserved_vmid = NULL;
		atomic_dec(&id_mgr->reserved_vmid_num);
	}
	mutex_unlock(&id_mgr->lock);
}

/**
 * gsgpu_vmid_reset - reset VMID to zero
 *
 * @adev: gsgpu device structure
 * @vmid: vmid number to use
 *
 * Reset saved GDW, GWS and OA to force switch on next flush.
 */
void gsgpu_vmid_reset(struct gsgpu_device *adev, unsigned vmid)
{
	struct gsgpu_vmid_mgr *id_mgr = &adev->vm_manager.id_mgr;
	struct gsgpu_vmid *id = &id_mgr->ids[vmid];

	mutex_lock(&id_mgr->lock);
	id->owner = 0;
	mutex_unlock(&id_mgr->lock);
}

/**
 * gsgpu_vmid_reset_all - reset VMID to zero
 *
 * @adev: gsgpu device structure
 *
 * Reset VMID to force flush on next use
 */
void gsgpu_vmid_reset_all(struct gsgpu_device *adev)
{
	unsigned i;

	struct gsgpu_vmid_mgr *id_mgr = &adev->vm_manager.id_mgr;

	for (i = 1; i < id_mgr->num_ids; ++i)
		gsgpu_vmid_reset(adev, i);
}

/**
 * gsgpu_vmid_mgr_init - init the VMID manager
 *
 * @adev: gsgpu_device pointer
 *
 * Initialize the VM manager structures
 */
void gsgpu_vmid_mgr_init(struct gsgpu_device *adev)
{
	unsigned i;

	struct gsgpu_vmid_mgr *id_mgr = &adev->vm_manager.id_mgr;

	mutex_init(&id_mgr->lock);
	INIT_LIST_HEAD(&id_mgr->ids_lru);
	atomic_set(&id_mgr->reserved_vmid_num, 0);

	/* skip over VMID 0, since it is the system VM */
	for (i = 1; i < id_mgr->num_ids; ++i) {
		gsgpu_vmid_reset(adev, i);
		gsgpu_sync_create(&id_mgr->ids[i].active);
		list_add_tail(&id_mgr->ids[i].list, &id_mgr->ids_lru);
	}
}

/**
 * gsgpu_vmid_mgr_fini - cleanup VM manager
 *
 * @adev: gsgpu_device pointer
 *
 * Cleanup the VM manager and free resources.
 */
void gsgpu_vmid_mgr_fini(struct gsgpu_device *adev)
{
	unsigned i;

	struct gsgpu_vmid_mgr *id_mgr = &adev->vm_manager.id_mgr;

	mutex_destroy(&id_mgr->lock);
	for (i = 0; i < GSGPU_NUM_VMID; ++i) {
		struct gsgpu_vmid *id = &id_mgr->ids[i];

		gsgpu_sync_free(&id->active);
		dma_fence_put(id->flushed_updates);
		dma_fence_put(id->last_flush);
		dma_fence_put(id->pasid_mapping);
	}
}

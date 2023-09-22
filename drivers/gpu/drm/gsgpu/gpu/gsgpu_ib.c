#include <linux/seq_file.h>
#include <linux/slab.h>
#include <drm/drmP.h>
#include <drm/gsgpu_drm.h>
#include "gsgpu.h"

#define GSGPU_IB_TEST_TIMEOUT	msecs_to_jiffies(1000)

/*
 * IB
 * IBs (Indirect Buffers) and areas of GPU accessible memory where
 * commands are stored.  You can put a pointer to the IB in the
 * command ring and the hw will fetch the commands from the IB
 * and execute them.  Generally userspace acceleration drivers
 * produce command buffers which are send to the kernel and
 * put in IBs for execution by the requested ring.
 */
static int gsgpu_debugfs_sa_init(struct gsgpu_device *adev);

/**
 * gsgpu_ib_get - request an IB (Indirect Buffer)
 *
 * @ring: ring index the IB is associated with
 * @size: requested IB size
 * @ib: IB object returned
 *
 * Request an IB (all asics).  IBs are allocated using the
 * suballocator.
 * Returns 0 on success, error on failure.
 */
int gsgpu_ib_get(struct gsgpu_device *adev, struct gsgpu_vm *vm,
		  unsigned size, struct gsgpu_ib *ib)
{
	int r;

	if (size) {
		r = gsgpu_sa_bo_new(&adev->ring_tmp_bo,
				      &ib->sa_bo, size, 256);
		if (r) {
			dev_err(adev->dev, "failed to get a new IB (%d)\n", r);
			return r;
		}

		ib->ptr = gsgpu_sa_bo_cpu_addr(ib->sa_bo);

		if (!vm)
			ib->gpu_addr = gsgpu_sa_bo_gpu_addr(ib->sa_bo);
	}

	return 0;
}

/**
 * gsgpu_ib_free - free an IB (Indirect Buffer)
 *
 * @adev: gsgpu_device pointer
 * @ib: IB object to free
 * @f: the fence SA bo need wait on for the ib alloation
 *
 * Free an IB (all asics).
 */
void gsgpu_ib_free(struct gsgpu_device *adev, struct gsgpu_ib *ib,
		    struct dma_fence *f)
{
	gsgpu_sa_bo_free(adev, &ib->sa_bo, f);
}

/**
 * gsgpu_ib_schedule - schedule an IB (Indirect Buffer) on the ring
 *
 * @adev: gsgpu_device pointer
 * @num_ibs: number of IBs to schedule
 * @ibs: IB objects to schedule
 * @f: fence created during this submission
 *
 * Schedule an IB on the associated ring (all asics).
 * Returns 0 on success, error on failure.
 *
 * On SI, there are two parallel engines fed from the primary ring,
 * the CE (Constant Engine) and the DE (Drawing Engine).  Since
 * resource descriptors have moved to memory, the CE allows you to
 * prime the caches while the DE is updating register state so that
 * the resource descriptors will be already in cache when the draw is
 * processed.  To accomplish this, the userspace driver submits two
 * IBs, one for the CE and one for the DE.  If there is a CE IB (called
 * a CONST_IB), it will be put on the ring prior to the DE IB.  Prior
 * to SI there was just a DE IB.
 */
int gsgpu_ib_schedule(struct gsgpu_ring *ring, unsigned num_ibs,
		       struct gsgpu_ib *ibs, struct gsgpu_job *job,
		       struct dma_fence **f)
{
	struct gsgpu_device *adev = ring->adev;
	struct gsgpu_ib *ib = &ibs[0];
	struct dma_fence *tmp = NULL;
	bool skip_preamble, need_ctx_switch;
	struct gsgpu_vm *vm;
	uint64_t fence_ctx;
	uint32_t status = 0, alloc_size;
	unsigned fence_flags = 0;

	unsigned i;
	int r = 0;
	bool need_pipe_sync = false;

	if (num_ibs == 0)
		return -EINVAL;

	/* ring tests don't use a job */
	if (job) {
		vm = job->vm;
		fence_ctx = job->base.s_fence->scheduled.context;
	} else {
		vm = NULL;
		fence_ctx = 0;
	}

	if (!ring->ready) {
		dev_err(adev->dev, "couldn't schedule ib on ring <%s>\n", ring->name);
		return -EINVAL;
	}

	if (vm && !job->vmid) {
		dev_err(adev->dev, "VM IB without ID\n");
		return -EINVAL;
	}

	alloc_size = ring->funcs->emit_frame_size + num_ibs *
		ring->funcs->emit_ib_size;

	r = gsgpu_ring_alloc(ring, alloc_size);
	if (r) {
		dev_err(adev->dev, "scheduling IB failed (%d).\n", r);
		return r;
	}

	need_ctx_switch = ring->current_ctx != fence_ctx;
	if (ring->funcs->emit_pipeline_sync && job &&
	    ((tmp = gsgpu_sync_get_fence(&job->sched_sync, NULL)) ||
	    gsgpu_vm_need_pipeline_sync(ring, job))) {
		need_pipe_sync = true;
		dma_fence_put(tmp);
	}

	if (job) {
		r = gsgpu_vm_flush(ring, job, need_pipe_sync);
		if (r) {
			gsgpu_ring_undo(ring);
			return r;
		}
	}

	skip_preamble = ring->current_ctx == fence_ctx;
	if (job && ring->funcs->emit_cntxcntl) {
		if (need_ctx_switch)
			status |= GSGPU_HAVE_CTX_SWITCH;
		status |= job->preamble_status;

		gsgpu_ring_emit_cntxcntl(ring, status);
	}

	for (i = 0; i < num_ibs; ++i) {
		ib = &ibs[i];

		gsgpu_ring_emit_ib(ring, ib, job ? job->vmid : 0,
				    need_ctx_switch);
		need_ctx_switch = false;
	}

	if (ib->flags & GSGPU_IB_FLAG_TC_WB_NOT_INVALIDATE)
		fence_flags |= GSGPU_FENCE_FLAG_TC_WB_ONLY;

	/* wrap the last IB with fence */
	if (job && job->uf_addr) {
		gsgpu_ring_emit_fence(ring, job->uf_addr, job->uf_sequence,
				       fence_flags | GSGPU_FENCE_FLAG_64BIT);
	}

	r = gsgpu_fence_emit(ring, f, fence_flags);
	if (r) {
		dev_err(adev->dev, "failed to emit fence (%d)\n", r);
		if (job && job->vmid)
			gsgpu_vmid_reset(adev, job->vmid);
		gsgpu_ring_undo(ring);
		return r;
	}

	if (ring->funcs->insert_end)
		ring->funcs->insert_end(ring);

	ring->current_ctx = fence_ctx;
	//if (vm && ring->funcs->emit_switch_buffer)
	//	gsgpu_ring_emit_switch_buffer(ring);
	gsgpu_ring_commit(ring);
	return 0;
}

/**
 * gsgpu_ib_pool_init - Init the IB (Indirect Buffer) pool
 *
 * @adev: gsgpu_device pointer
 *
 * Initialize the suballocator to manage a pool of memory
 * for use as IBs (all asics).
 * Returns 0 on success, error on failure.
 */
int gsgpu_ib_pool_init(struct gsgpu_device *adev)
{
	int r;

	if (adev->ib_pool_ready) {
		return 0;
	}
	r = gsgpu_sa_bo_manager_init(adev, &adev->ring_tmp_bo,
				      GSGPU_IB_POOL_SIZE*64*1024,
				      GSGPU_GPU_PAGE_SIZE,
				      GSGPU_GEM_DOMAIN_GTT);
	if (r) {
		return r;
	}

	adev->ib_pool_ready = true;
	if (gsgpu_debugfs_sa_init(adev)) {
		dev_err(adev->dev, "failed to register debugfs file for SA\n");
	}
	return 0;
}

/**
 * gsgpu_ib_pool_fini - Free the IB (Indirect Buffer) pool
 *
 * @adev: gsgpu_device pointer
 *
 * Tear down the suballocator managing the pool of memory
 * for use as IBs (all asics).
 */
void gsgpu_ib_pool_fini(struct gsgpu_device *adev)
{
	if (adev->ib_pool_ready) {
		gsgpu_sa_bo_manager_fini(adev, &adev->ring_tmp_bo);
		adev->ib_pool_ready = false;
	}
}

/**
 * gsgpu_ib_ring_tests - test IBs on the rings
 *
 * @adev: gsgpu_device pointer
 *
 * Test an IB (Indirect Buffer) on each ring.
 * If the test fails, disable the ring.
 * Returns 0 on success, error if the primary GFX ring
 * IB test fails.
 */
int gsgpu_ib_ring_tests(struct gsgpu_device *adev)
{
	unsigned i;
	int r, ret = 0;
	long tmo_gfx, tmo_mm;

	tmo_mm = tmo_gfx = GSGPU_IB_TEST_TIMEOUT;

	for (i = 0; i < GSGPU_MAX_RINGS; ++i) {
		struct gsgpu_ring *ring = adev->rings[i];
		long tmo;

		if (!ring || !ring->ready)
			continue;

		tmo = tmo_gfx;

		r = gsgpu_ring_test_ib(ring, tmo);
		if (r) {
			ring->ready = false;

			if (ring == &adev->gfx.gfx_ring[0]) {
				/* oh, oh, that's really bad */
				DRM_ERROR("gsgpu: failed testing IB on GFX ring (%d).\n", r);
				adev->accel_working = false;
				return r;

			} else {
				/* still not good, but we can live with it */
				DRM_ERROR("gsgpu: failed testing IB on ring %d (%d).\n", i, r);
				ret = r;
			}
		}
	}
	return ret;
}

/*
 * Debugfs info
 */
#if defined(CONFIG_DEBUG_FS)

static int gsgpu_debugfs_sa_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct gsgpu_device *adev = dev->dev_private;

	gsgpu_sa_bo_dump_debug_info(&adev->ring_tmp_bo, m);

	return 0;

}

static const struct drm_info_list gsgpu_debugfs_sa_list[] = {
	{"gsgpu_sa_info", &gsgpu_debugfs_sa_info, 0, NULL},
};

#endif

static int gsgpu_debugfs_sa_init(struct gsgpu_device *adev)
{
#if defined(CONFIG_DEBUG_FS)
	return gsgpu_debugfs_add_files(adev, gsgpu_debugfs_sa_list, 1);
#else
	return 0;
#endif
}

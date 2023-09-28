#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include "gsgpu.h"
#include "gsgpu_trace.h"

static void gsgpu_job_timedout(struct drm_sched_job *s_job)
{
	struct gsgpu_ring *ring = to_gsgpu_ring(s_job->sched);
	struct gsgpu_job *job = to_gsgpu_job(s_job);

	DRM_ERROR("ring %s timeout, signaled seq=%u, emitted seq=%u\n",
		  job->base.sched->name, atomic_read(&ring->fence_drv.last_seq),
		  ring->fence_drv.sync_seq);

	gsgpu_device_gpu_recover(ring->adev, job, false);
}

int gsgpu_job_alloc(struct gsgpu_device *adev, unsigned num_ibs,
		     struct gsgpu_job **job, struct gsgpu_vm *vm)
{
	size_t size = sizeof(struct gsgpu_job);

	if (num_ibs == 0)
		return -EINVAL;

	size += sizeof(struct gsgpu_ib) * num_ibs;

	*job = kzalloc(size, GFP_KERNEL);
	if (!*job)
		return -ENOMEM;

	/*
	 * Initialize the scheduler to at least some ring so that we always
	 * have a pointer to adev.
	 */
	(*job)->base.sched = &adev->rings[0]->sched;
	(*job)->vm = vm;
	(*job)->ibs = (void *)&(*job)[1];
	(*job)->num_ibs = num_ibs;

	gsgpu_sync_create(&(*job)->sync);
	gsgpu_sync_create(&(*job)->sched_sync);
	(*job)->vram_lost_counter = atomic_read(&adev->vram_lost_counter);
	(*job)->vm_pd_addr = GSGPU_BO_INVALID_OFFSET;

	return 0;
}

int gsgpu_job_alloc_with_ib(struct gsgpu_device *adev, unsigned size,
			     struct gsgpu_job **job)
{
	int r;

	r = gsgpu_job_alloc(adev, 1, job, NULL);
	if (r)
		return r;

	r = gsgpu_ib_get(adev, NULL, size, &(*job)->ibs[0]);
	if (r)
		kfree(*job);
	else
		(*job)->vm_pd_addr = adev->gart.table_addr;

	return r;
}

void gsgpu_job_free_resources(struct gsgpu_job *job)
{
	struct gsgpu_ring *ring = to_gsgpu_ring(job->base.sched);
	struct dma_fence *f;
	unsigned i;

	/* use sched fence if available */
	f = job->base.s_fence ? &job->base.s_fence->finished : job->fence;

	for (i = 0; i < job->num_ibs; ++i)
		gsgpu_ib_free(ring->adev, &job->ibs[i], f);
}

static void gsgpu_job_free_cb(struct drm_sched_job *s_job)
{
	struct gsgpu_ring *ring = to_gsgpu_ring(s_job->sched);
	struct gsgpu_job *job = to_gsgpu_job(s_job);

	gsgpu_ring_priority_put(ring, s_job->s_priority);
	dma_fence_put(job->fence);
	gsgpu_sync_free(&job->sync);
	gsgpu_sync_free(&job->sched_sync);
	kfree(job);
}

void gsgpu_job_free(struct gsgpu_job *job)
{
	gsgpu_job_free_resources(job);

	dma_fence_put(job->fence);
	gsgpu_sync_free(&job->sync);
	gsgpu_sync_free(&job->sched_sync);
	kfree(job);
}

int gsgpu_job_submit(struct gsgpu_job *job, struct drm_sched_entity *entity,
		      void *owner, struct dma_fence **f)
{
	enum drm_sched_priority priority;
	struct gsgpu_ring *ring;
	int r;

	if (!f)
		return -EINVAL;

	r = drm_sched_job_init(&job->base, entity, owner);
	if (r)
		return r;

	job->owner = owner;
	*f = dma_fence_get(&job->base.s_fence->finished);
	gsgpu_job_free_resources(job);
	priority = job->base.s_priority;
	drm_sched_entity_push_job(&job->base);

	ring = to_gsgpu_ring(entity->rq->sched);
	gsgpu_ring_priority_get(ring, priority);

	return 0;
}

int gsgpu_job_submit_direct(struct gsgpu_job *job, struct gsgpu_ring *ring,
			     struct dma_fence **fence)
{
	int r;

	job->base.sched = &ring->sched;
	r = gsgpu_ib_schedule(ring, job->num_ibs, job->ibs, NULL, fence);
	job->fence = dma_fence_get(*fence);
	if (r)
		return r;

	gsgpu_job_free(job);
	return 0;
}

static struct dma_fence *gsgpu_job_dependency(struct drm_sched_job *sched_job,
					       struct drm_sched_entity *s_entity)
{
	struct gsgpu_ring *ring = to_gsgpu_ring(s_entity->rq->sched);
	struct gsgpu_job *job = to_gsgpu_job(sched_job);
	struct gsgpu_vm *vm = job->vm;
	struct dma_fence *fence;
	bool explicit = false;
	int r;

	fence = gsgpu_sync_get_fence(&job->sync, &explicit);
	if (fence && explicit) {
		if (drm_sched_dependency_optimized(fence, s_entity)) {
			r = gsgpu_sync_fence(ring->adev, &job->sched_sync,
					      fence, false);
			if (r)
				DRM_ERROR("Error adding fence (%d)\n", r);
		}
	}

	while (fence == NULL && vm && !job->vmid) {
		r = gsgpu_vmid_grab(vm, ring, &job->sync,
				     &job->base.s_fence->finished,
				     job);
		if (r)
			DRM_ERROR("Error getting VM ID (%d)\n", r);

		fence = gsgpu_sync_get_fence(&job->sync, NULL);
	}

	return fence;
}

static struct dma_fence *gsgpu_job_run(struct drm_sched_job *sched_job)
{
	struct gsgpu_ring *ring = to_gsgpu_ring(sched_job->sched);
	struct dma_fence *fence = NULL, *finished;
	struct gsgpu_job *job;
	int r;

	job = to_gsgpu_job(sched_job);
	finished = &job->base.s_fence->finished;

	BUG_ON(gsgpu_sync_peek_fence(&job->sync, NULL));

	trace_gsgpu_sched_run_job(job);

	if (job->vram_lost_counter != atomic_read(&ring->adev->vram_lost_counter))
		dma_fence_set_error(finished, -ECANCELED);/* skip IB as well if VRAM lost */

	if (finished->error < 0) {
		DRM_INFO("Skip scheduling IBs!\n");
	} else {
		r = gsgpu_ib_schedule(ring, job->num_ibs, job->ibs, job,
				       &fence);
		if (r)
			DRM_ERROR("Error scheduling IBs (%d)\n", r);
	}
	/* if gpu reset, hw fence will be replaced here */
	dma_fence_put(job->fence);
	job->fence = dma_fence_get(fence);

	gsgpu_job_free_resources(job);
	return fence;
}

const struct drm_sched_backend_ops gsgpu_sched_ops = {
	.dependency = gsgpu_job_dependency,
	.run_job = gsgpu_job_run,
	.timedout_job = gsgpu_job_timedout,
	.free_job = gsgpu_job_free_cb
};

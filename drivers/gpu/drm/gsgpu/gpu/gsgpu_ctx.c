#include <drm/drm_auth.h>
#include "gsgpu.h"
#include "gsgpu_sched.h"

static int gsgpu_ctx_priority_permit(struct drm_file *filp,
				      enum drm_sched_priority priority)
{
	/* NORMAL and below are accessible by everyone */
	if (priority <= DRM_SCHED_PRIORITY_NORMAL)
		return 0;

	if (capable(CAP_SYS_NICE))
		return 0;

	if (drm_is_current_master(filp))
		return 0;

	return -EACCES;
}

static int gsgpu_ctx_init(struct gsgpu_device *adev,
			   enum drm_sched_priority priority,
			   struct drm_file *filp,
			   struct gsgpu_ctx *ctx)
{
	unsigned i, j;
	int r;

	if (priority < 0 || priority >= DRM_SCHED_PRIORITY_COUNT)
		return -EINVAL;

	r = gsgpu_ctx_priority_permit(filp, priority);
	if (r)
		return r;

	memset(ctx, 0, sizeof(*ctx));
	ctx->adev = adev;
	kref_init(&ctx->refcount);
	spin_lock_init(&ctx->ring_lock);
	ctx->fences = kcalloc(gsgpu_sched_jobs * GSGPU_MAX_RINGS,
			      sizeof(struct dma_fence *), GFP_KERNEL);
	if (!ctx->fences)
		return -ENOMEM;

	mutex_init(&ctx->lock);

	for (i = 0; i < GSGPU_MAX_RINGS; ++i) {
		ctx->rings[i].sequence = 1;
		ctx->rings[i].fences = &ctx->fences[gsgpu_sched_jobs * i];
	}

	ctx->reset_counter = atomic_read(&adev->gpu_reset_counter);
	ctx->reset_counter_query = ctx->reset_counter;
	ctx->vram_lost_counter = atomic_read(&adev->vram_lost_counter);
	ctx->init_priority = priority;
	ctx->override_priority = GSGPU_CTX_PRIORITY_UNSET;

	/* create context entity for each ring */
	for (i = 0; i < adev->num_rings; i++) {
		struct gsgpu_ring *ring = adev->rings[i];
		struct drm_gpu_scheduler *sched = &ring->sched;
		r = drm_sched_entity_init(&ctx->rings[i].entity,
					  priority, &sched, 1, &ctx->guilty);
		if (r)
			goto failed;
	}

	r = gsgpu_queue_mgr_init(adev, &ctx->queue_mgr);
	if (r)
		goto failed;

	return 0;

failed:
	for (j = 0; j < i; j++)
		drm_sched_entity_destroy(&ctx->rings[j].entity);
	kfree(ctx->fences);
	ctx->fences = NULL;
	return r;
}

static void gsgpu_ctx_fini(struct kref *ref)
{
	struct gsgpu_ctx *ctx = container_of(ref, struct gsgpu_ctx, refcount);
	struct gsgpu_device *adev = ctx->adev;
	unsigned i, j;

	if (!adev)
		return;

	for (i = 0; i < GSGPU_MAX_RINGS; ++i)
		for (j = 0; j < gsgpu_sched_jobs; ++j)
			dma_fence_put(ctx->rings[i].fences[j]);
	kfree(ctx->fences);
	ctx->fences = NULL;

	gsgpu_queue_mgr_fini(adev, &ctx->queue_mgr);

	mutex_destroy(&ctx->lock);

	kfree(ctx);
}

static int gsgpu_ctx_alloc(struct gsgpu_device *adev,
			    struct gsgpu_fpriv *fpriv,
			    struct drm_file *filp,
			    enum drm_sched_priority priority,
			    uint32_t *id)
{
	struct gsgpu_ctx_mgr *mgr = &fpriv->ctx_mgr;
	struct gsgpu_ctx *ctx;
	int r;

	ctx = kmalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mutex_lock(&mgr->lock);
	r = idr_alloc(&mgr->ctx_handles, ctx, 1, 0, GFP_KERNEL);
	if (r < 0) {
		mutex_unlock(&mgr->lock);
		kfree(ctx);
		return r;
	}

	*id = (uint32_t)r;
	r = gsgpu_ctx_init(adev, priority, filp, ctx);
	if (r) {
		idr_remove(&mgr->ctx_handles, *id);
		*id = 0;
		kfree(ctx);
	}
	mutex_unlock(&mgr->lock);
	return r;
}

static void gsgpu_ctx_do_release(struct kref *ref)
{
	struct gsgpu_ctx *ctx;
	u32 i;

	ctx = container_of(ref, struct gsgpu_ctx, refcount);

	for (i = 0; i < ctx->adev->num_rings; i++) {
		drm_sched_entity_destroy(&ctx->rings[i].entity);
	}

	gsgpu_ctx_fini(ref);
}

static int gsgpu_ctx_free(struct gsgpu_fpriv *fpriv, uint32_t id)
{
	struct gsgpu_ctx_mgr *mgr = &fpriv->ctx_mgr;
	struct gsgpu_ctx *ctx;

	mutex_lock(&mgr->lock);
	ctx = idr_remove(&mgr->ctx_handles, id);
	if (ctx)
		kref_put(&ctx->refcount, gsgpu_ctx_do_release);
	mutex_unlock(&mgr->lock);
	return ctx ? 0 : -EINVAL;
}

static int gsgpu_ctx_query(struct gsgpu_device *adev,
			    struct gsgpu_fpriv *fpriv, uint32_t id,
			    union drm_gsgpu_ctx_out *out)
{
	struct gsgpu_ctx *ctx;
	struct gsgpu_ctx_mgr *mgr;
	unsigned reset_counter;

	if (!fpriv)
		return -EINVAL;

	mgr = &fpriv->ctx_mgr;
	mutex_lock(&mgr->lock);
	ctx = idr_find(&mgr->ctx_handles, id);
	if (!ctx) {
		mutex_unlock(&mgr->lock);
		return -EINVAL;
	}

	/* TODO: these two are always zero */
	out->state.flags = 0x0;
	out->state.hangs = 0x0;

	/* determine if a GPU reset has occured since the last call */
	reset_counter = atomic_read(&adev->gpu_reset_counter);
	/* TODO: this should ideally return NO, GUILTY, or INNOCENT. */
	if (ctx->reset_counter_query == reset_counter)
		out->state.reset_status = GSGPU_CTX_NO_RESET;
	else
		out->state.reset_status = GSGPU_CTX_UNKNOWN_RESET;
	ctx->reset_counter_query = reset_counter;

	mutex_unlock(&mgr->lock);
	return 0;
}

static int gsgpu_ctx_query2(struct gsgpu_device *adev,
	struct gsgpu_fpriv *fpriv, uint32_t id,
	union drm_gsgpu_ctx_out *out)
{
	struct gsgpu_ctx *ctx;
	struct gsgpu_ctx_mgr *mgr;

	if (!fpriv)
		return -EINVAL;

	mgr = &fpriv->ctx_mgr;
	mutex_lock(&mgr->lock);
	ctx = idr_find(&mgr->ctx_handles, id);
	if (!ctx) {
		mutex_unlock(&mgr->lock);
		return -EINVAL;
	}

	out->state.flags = 0x0;
	out->state.hangs = 0x0;

	if (ctx->reset_counter != atomic_read(&adev->gpu_reset_counter))
		out->state.flags |= GSGPU_CTX_QUERY2_FLAGS_RESET;

	if (ctx->vram_lost_counter != atomic_read(&adev->vram_lost_counter))
		out->state.flags |= GSGPU_CTX_QUERY2_FLAGS_VRAMLOST;

	if (atomic_read(&ctx->guilty))
		out->state.flags |= GSGPU_CTX_QUERY2_FLAGS_GUILTY;

	mutex_unlock(&mgr->lock);
	return 0;
}

int gsgpu_ctx_ioctl(struct drm_device *dev, void *data,
		     struct drm_file *filp)
{
	union drm_gsgpu_ctx *args = data;
	struct gsgpu_device *adev = dev->dev_private;
	struct gsgpu_fpriv *fpriv = filp->driver_priv;

	int r = 0;
	uint32_t id = args->in.ctx_id;
	enum drm_sched_priority priority = gsgpu_to_sched_priority(args->in.priority);

	/* For backwards compatibility reasons, we need to accept
	 * ioctls with garbage in the priority field */
	if (priority == -EINVAL)
		priority = DRM_SCHED_PRIORITY_NORMAL;

	switch (args->in.op) {
	case GSGPU_CTX_OP_ALLOC_CTX:
		r = gsgpu_ctx_alloc(adev, fpriv, filp, priority, &id);
		args->out.alloc.ctx_id = id;
		break;
	case GSGPU_CTX_OP_FREE_CTX:
		r = gsgpu_ctx_free(fpriv, id);
		break;
	case GSGPU_CTX_OP_QUERY_STATE:
		r = gsgpu_ctx_query(adev, fpriv, id, &args->out);
		break;
	case GSGPU_CTX_OP_QUERY_STATE2:
		r = gsgpu_ctx_query2(adev, fpriv, id, &args->out);
		break;
	default:
		return -EINVAL;
	}

	return r;
}

struct gsgpu_ctx *gsgpu_ctx_get(struct gsgpu_fpriv *fpriv, uint32_t id)
{
	struct gsgpu_ctx *ctx;
	struct gsgpu_ctx_mgr *mgr;

	if (!fpriv)
		return NULL;

	mgr = &fpriv->ctx_mgr;

	mutex_lock(&mgr->lock);
	ctx = idr_find(&mgr->ctx_handles, id);
	if (ctx)
		kref_get(&ctx->refcount);
	mutex_unlock(&mgr->lock);
	return ctx;
}

int gsgpu_ctx_put(struct gsgpu_ctx *ctx)
{
	if (ctx == NULL)
		return -EINVAL;

	kref_put(&ctx->refcount, gsgpu_ctx_do_release);
	return 0;
}

int gsgpu_ctx_add_fence(struct gsgpu_ctx *ctx, struct gsgpu_ring *ring,
			      struct dma_fence *fence, uint64_t *handler)
{
	struct gsgpu_ctx_ring *cring = &ctx->rings[ring->idx];
	uint64_t seq = cring->sequence;
	unsigned idx = 0;
	struct dma_fence *other = NULL;

	idx = seq & (gsgpu_sched_jobs - 1);
	other = cring->fences[idx];
	if (other)
		BUG_ON(!dma_fence_is_signaled(other));

	dma_fence_get(fence);

	spin_lock(&ctx->ring_lock);
	cring->fences[idx] = fence;
	cring->sequence++;
	spin_unlock(&ctx->ring_lock);

	dma_fence_put(other);
	if (handler)
		*handler = seq;

	return 0;
}

struct dma_fence *gsgpu_ctx_get_fence(struct gsgpu_ctx *ctx,
				       struct gsgpu_ring *ring, uint64_t seq)
{
	struct gsgpu_ctx_ring *cring = &ctx->rings[ring->idx];
	struct dma_fence *fence;

	spin_lock(&ctx->ring_lock);

	if (seq == ~0ull)
		seq = ctx->rings[ring->idx].sequence - 1;

	if (seq >= cring->sequence) {
		spin_unlock(&ctx->ring_lock);
		return ERR_PTR(-EINVAL);
	}


	if (seq + gsgpu_sched_jobs < cring->sequence) {
		spin_unlock(&ctx->ring_lock);
		return NULL;
	}

	fence = dma_fence_get(cring->fences[seq & (gsgpu_sched_jobs - 1)]);
	spin_unlock(&ctx->ring_lock);

	return fence;
}

void gsgpu_ctx_priority_override(struct gsgpu_ctx *ctx,
				  enum drm_sched_priority priority)
{
	int i;
	struct gsgpu_device *adev = ctx->adev;
	struct drm_sched_entity *entity;
	struct gsgpu_ring *ring;
	enum drm_sched_priority ctx_prio;

	ctx->override_priority = priority;

	ctx_prio = (ctx->override_priority == GSGPU_CTX_PRIORITY_UNSET) ?
			ctx->init_priority : ctx->override_priority;

	for (i = 0; i < adev->num_rings; i++) {
		ring = adev->rings[i];
		entity = &ctx->rings[i].entity;

		drm_sched_entity_set_priority(entity, ctx_prio);
	}
}

int gsgpu_ctx_wait_prev_fence(struct gsgpu_ctx *ctx, unsigned ring_id)
{
	struct gsgpu_ctx_ring *cring = &ctx->rings[ring_id];
	unsigned idx = cring->sequence & (gsgpu_sched_jobs - 1);
	struct dma_fence *other = cring->fences[idx];

	if (other) {
		signed long r;
		r = dma_fence_wait(other, true);
		if (r < 0) {
			if (r != -ERESTARTSYS)
				DRM_ERROR("Error (%ld) waiting for fence!\n", r);

			return r;
		}
	}

	return 0;
}

void gsgpu_ctx_mgr_init(struct gsgpu_ctx_mgr *mgr)
{
	mutex_init(&mgr->lock);
	idr_init(&mgr->ctx_handles);
}

void gsgpu_ctx_mgr_entity_flush(struct gsgpu_ctx_mgr *mgr)
{
	struct gsgpu_ctx *ctx;
	struct idr *idp;
	uint32_t id, i;
	long max_wait = MAX_WAIT_SCHED_ENTITY_Q_EMPTY;

	idp = &mgr->ctx_handles;

	mutex_lock(&mgr->lock);
	idr_for_each_entry(idp, ctx, id) {

		if (!ctx->adev) {
			mutex_unlock(&mgr->lock);
			return;
		}

		for (i = 0; i < ctx->adev->num_rings; i++) {
			max_wait = drm_sched_entity_flush(&ctx->rings[i].entity,
							  max_wait);
		}
	}
	mutex_unlock(&mgr->lock);
}

void gsgpu_ctx_mgr_entity_fini(struct gsgpu_ctx_mgr *mgr)
{
	struct gsgpu_ctx *ctx;
	struct idr *idp;
	uint32_t id, i;

	idp = &mgr->ctx_handles;

	idr_for_each_entry(idp, ctx, id) {

		if (!ctx->adev)
			return;

		for (i = 0; i < ctx->adev->num_rings; i++) {
			if (kref_read(&ctx->refcount) == 1)
				drm_sched_entity_fini(&ctx->rings[i].entity);
			else
				DRM_ERROR("ctx 0x%px is still alive\n", ctx);
		}
	}
}

void gsgpu_ctx_mgr_fini(struct gsgpu_ctx_mgr *mgr)
{
	struct gsgpu_ctx *ctx;
	struct idr *idp;
	uint32_t id;

	gsgpu_ctx_mgr_entity_fini(mgr);

	idp = &mgr->ctx_handles;

	idr_for_each_entry(idp, ctx, id) {
		if (kref_put(&ctx->refcount, gsgpu_ctx_fini) != 1)
			DRM_ERROR("ctx 0x%px is still alive\n", ctx);
	}

	idr_destroy(&mgr->ctx_handles);
	mutex_destroy(&mgr->lock);
}

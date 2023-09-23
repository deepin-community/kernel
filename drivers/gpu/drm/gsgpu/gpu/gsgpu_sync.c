#include <drm/drmP.h>
#include "gsgpu.h"
#include "gsgpu_trace.h"

struct gsgpu_sync_entry {
	struct hlist_node	node;
	struct dma_fence	*fence;
	bool	explicit;
};

static struct kmem_cache *gsgpu_sync_slab;

/**
 * gsgpu_sync_create - zero init sync object
 *
 * @sync: sync object to initialize
 *
 * Just clear the sync object for now.
 */
void gsgpu_sync_create(struct gsgpu_sync *sync)
{
	hash_init(sync->fences);
	sync->last_vm_update = NULL;
}

/**
 * gsgpu_sync_same_dev - test if fence belong to us
 *
 * @adev: gsgpu device to use for the test
 * @f: fence to test
 *
 * Test if the fence was issued by us.
 */
static bool gsgpu_sync_same_dev(struct gsgpu_device *adev,
				 struct dma_fence *f)
{
	struct drm_sched_fence *s_fence = to_drm_sched_fence(f);

	if (s_fence) {
		struct gsgpu_ring *ring;

		ring = container_of(s_fence->sched, struct gsgpu_ring, sched);
		return ring->adev == adev;
	}

	return false;
}

/**
 * gsgpu_sync_get_owner - extract the owner of a fence
 *
 * @fence: fence get the owner from
 *
 * Extract who originally created the fence.
 */
static void *gsgpu_sync_get_owner(struct dma_fence *f)
{
	struct drm_sched_fence *s_fence;

	if (!f)
		return GSGPU_FENCE_OWNER_UNDEFINED;

	s_fence = to_drm_sched_fence(f);
	if (s_fence)
		return s_fence->owner;

	return GSGPU_FENCE_OWNER_UNDEFINED;
}

/**
 * gsgpu_sync_keep_later - Keep the later fence
 *
 * @keep: existing fence to test
 * @fence: new fence
 *
 * Either keep the existing fence or the new one, depending which one is later.
 */
static void gsgpu_sync_keep_later(struct dma_fence **keep,
				   struct dma_fence *fence)
{
	if (*keep && dma_fence_is_later(*keep, fence))
		return;

	dma_fence_put(*keep);
	*keep = dma_fence_get(fence);
}

/**
 * gsgpu_sync_add_later - add the fence to the hash
 *
 * @sync: sync object to add the fence to
 * @f: fence to add
 *
 * Tries to add the fence to an existing hash entry. Returns true when an entry
 * was found, false otherwise.
 */
static bool gsgpu_sync_add_later(struct gsgpu_sync *sync, struct dma_fence *f, bool explicit)
{
	struct gsgpu_sync_entry *e;

	hash_for_each_possible(sync->fences, e, node, f->context) {
		if (unlikely(e->fence->context != f->context))
			continue;

		gsgpu_sync_keep_later(&e->fence, f);

		/* Preserve eplicit flag to not loose pipe line sync */
		e->explicit |= explicit;

		return true;
	}
	return false;
}

/**
 * gsgpu_sync_fence - remember to sync to this fence
 *
 * @sync: sync object to add fence to
 * @fence: fence to sync to
 *
 */
int gsgpu_sync_fence(struct gsgpu_device *adev, struct gsgpu_sync *sync,
		      struct dma_fence *f, bool explicit)
{
	struct gsgpu_sync_entry *e;

	if (!f)
		return 0;
	if (gsgpu_sync_same_dev(adev, f) &&
	    gsgpu_sync_get_owner(f) == GSGPU_FENCE_OWNER_VM)
		gsgpu_sync_keep_later(&sync->last_vm_update, f);

	if (gsgpu_sync_add_later(sync, f, explicit))
		return 0;

	e = kmem_cache_alloc(gsgpu_sync_slab, GFP_KERNEL);
	if (!e)
		return -ENOMEM;

	e->explicit = explicit;

	hash_add(sync->fences, &e->node, f->context);
	e->fence = dma_fence_get(f);
	return 0;
}

/**
 * gsgpu_sync_resv - sync to a reservation object
 *
 * @sync: sync object to add fences from reservation object to
 * @resv: reservation object with embedded fence
 * @explicit_sync: true if we should only sync to the exclusive fence
 *
 * Sync to the fence
 */
int gsgpu_sync_resv(struct gsgpu_device *adev,
		     struct gsgpu_sync *sync,
		     struct dma_resv *resv,
		     void *owner, bool explicit_sync)
{
	struct dma_resv_list *flist;
	struct dma_fence *f;
	void *fence_owner;
	unsigned i;
	int r = 0;

	if (resv == NULL)
		return -EINVAL;

	/* always sync to the exclusive fence */
	f = dma_resv_get_excl(resv);
	r = gsgpu_sync_fence(adev, sync, f, false);

	flist = dma_resv_get_list(resv);
	if (!flist || r)
		return r;

	for (i = 0; i < flist->shared_count; ++i) {
		f = rcu_dereference_protected(flist->shared[i],
					      dma_resv_held(resv));
		/* We only want to trigger KFD eviction fences on
		 * evict or move jobs. Skip KFD fences otherwise.
		 */
		fence_owner = gsgpu_sync_get_owner(f);
		if (fence_owner == GSGPU_FENCE_OWNER_KFD &&
		    owner != GSGPU_FENCE_OWNER_UNDEFINED)
			continue;

		if (gsgpu_sync_same_dev(adev, f)) {
			/* VM updates are only interesting
			 * for other VM updates and moves.
			 */
			if ((owner != GSGPU_FENCE_OWNER_UNDEFINED) &&
			    (fence_owner != GSGPU_FENCE_OWNER_UNDEFINED) &&
			    ((owner == GSGPU_FENCE_OWNER_VM) !=
			     (fence_owner == GSGPU_FENCE_OWNER_VM)))
				continue;

			/* Ignore fence from the same owner and explicit one as
			 * long as it isn't undefined.
			 */
			if (owner != GSGPU_FENCE_OWNER_UNDEFINED &&
			    (fence_owner == owner || explicit_sync))
				continue;
		}

		r = gsgpu_sync_fence(adev, sync, f, false);
		if (r)
			break;
	}
	return r;
}

/**
 * gsgpu_sync_peek_fence - get the next fence not signaled yet
 *
 * @sync: the sync object
 * @ring: optional ring to use for test
 *
 * Returns the next fence not signaled yet without removing it from the sync
 * object.
 */
struct dma_fence *gsgpu_sync_peek_fence(struct gsgpu_sync *sync,
					 struct gsgpu_ring *ring)
{
	struct gsgpu_sync_entry *e;
	struct hlist_node *tmp;
	int i;

	hash_for_each_safe(sync->fences, i, tmp, e, node) {
		struct dma_fence *f = e->fence;
		struct drm_sched_fence *s_fence = to_drm_sched_fence(f);

		if (dma_fence_is_signaled(f)) {
			hash_del(&e->node);
			dma_fence_put(f);
			kmem_cache_free(gsgpu_sync_slab, e);
			continue;
		}
		if (ring && s_fence) {
			/* For fences from the same ring it is sufficient
			 * when they are scheduled.
			 */
			if (s_fence->sched == &ring->sched) {
				if (dma_fence_is_signaled(&s_fence->scheduled))
					continue;

				return &s_fence->scheduled;
			}
		}

		return f;
	}

	return NULL;
}

/**
 * gsgpu_sync_get_fence - get the next fence from the sync object
 *
 * @sync: sync object to use
 * @explicit: true if the next fence is explicit
 *
 * Get and removes the next fence from the sync object not signaled yet.
 */
struct dma_fence *gsgpu_sync_get_fence(struct gsgpu_sync *sync, bool *explicit)
{
	struct gsgpu_sync_entry *e;
	struct hlist_node *tmp;
	struct dma_fence *f;
	int i;
	hash_for_each_safe(sync->fences, i, tmp, e, node) {

		f = e->fence;
		if (explicit)
			*explicit = e->explicit;

		hash_del(&e->node);
		kmem_cache_free(gsgpu_sync_slab, e);

		if (!dma_fence_is_signaled(f))
			return f;

		dma_fence_put(f);
	}
	return NULL;
}

/**
 * gsgpu_sync_clone - clone a sync object
 *
 * @source: sync object to clone
 * @clone: pointer to destination sync object
 *
 * Adds references to all unsignaled fences in @source to @clone. Also
 * removes signaled fences from @source while at it.
 */
int gsgpu_sync_clone(struct gsgpu_sync *source, struct gsgpu_sync *clone)
{
	struct gsgpu_sync_entry *e;
	struct hlist_node *tmp;
	struct dma_fence *f;
	int i, r;

	hash_for_each_safe(source->fences, i, tmp, e, node) {
		f = e->fence;
		if (!dma_fence_is_signaled(f)) {
			r = gsgpu_sync_fence(NULL, clone, f, e->explicit);
			if (r)
				return r;
		} else {
			hash_del(&e->node);
			dma_fence_put(f);
			kmem_cache_free(gsgpu_sync_slab, e);
		}
	}

	dma_fence_put(clone->last_vm_update);
	clone->last_vm_update = dma_fence_get(source->last_vm_update);

	return 0;
}

int gsgpu_sync_wait(struct gsgpu_sync *sync, bool intr)
{
	struct gsgpu_sync_entry *e;
	struct hlist_node *tmp;
	int i, r;

	hash_for_each_safe(sync->fences, i, tmp, e, node) {
		r = dma_fence_wait(e->fence, intr);
		if (r)
			return r;

		hash_del(&e->node);
		dma_fence_put(e->fence);
		kmem_cache_free(gsgpu_sync_slab, e);
	}

	return 0;
}

/**
 * gsgpu_sync_free - free the sync object
 *
 * @sync: sync object to use
 *
 * Free the sync object.
 */
void gsgpu_sync_free(struct gsgpu_sync *sync)
{
	struct gsgpu_sync_entry *e;
	struct hlist_node *tmp;
	unsigned i;

	hash_for_each_safe(sync->fences, i, tmp, e, node) {
		hash_del(&e->node);
		dma_fence_put(e->fence);
		kmem_cache_free(gsgpu_sync_slab, e);
	}

	dma_fence_put(sync->last_vm_update);
}

/**
 * gsgpu_sync_init - init sync object subsystem
 *
 * Allocate the slab allocator.
 */
int gsgpu_sync_init(void)
{
	gsgpu_sync_slab = kmem_cache_create(
		"gsgpu_sync", sizeof(struct gsgpu_sync_entry), 0,
		SLAB_HWCACHE_ALIGN, NULL);
	if (!gsgpu_sync_slab)
		return -ENOMEM;

	return 0;
}

/**
 * gsgpu_sync_fini - fini sync object subsystem
 *
 * Free the slab allocator.
 */
void gsgpu_sync_fini(void)
{
	kmem_cache_destroy(gsgpu_sync_slab);
}

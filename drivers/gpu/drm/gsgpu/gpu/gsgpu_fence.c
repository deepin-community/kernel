#include <linux/seq_file.h>
#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/kref.h>
#include <linux/slab.h>
#include <linux/firmware.h>
#include <linux/delay.h>
#include "gsgpu.h"
#include "gsgpu_trace.h"

/*
 * Fences
 * Fences mark an event in the GPUs pipeline and are used
 * for GPU/CPU synchronization.  When the fence is written,
 * it is expected that all buffers associated with that fence
 * are no longer in use by the associated ring on the GPU and
 * that the the relevant GPU caches have been flushed.
 */

struct gsgpu_fence {
	struct dma_fence base;

	/* RB, DMA, etc. */
	struct gsgpu_ring		*ring;
};

static struct kmem_cache *gsgpu_fence_slab;

int gsgpu_fence_slab_init(void)
{
	gsgpu_fence_slab = kmem_cache_create(
		"gsgpu_fence", sizeof(struct gsgpu_fence), 0,
		SLAB_HWCACHE_ALIGN, NULL);
	if (!gsgpu_fence_slab)
		return -ENOMEM;
	return 0;
}

void gsgpu_fence_slab_fini(void)
{
	rcu_barrier();
	kmem_cache_destroy(gsgpu_fence_slab);
}
/*
 * Cast helper
 */
static const struct dma_fence_ops gsgpu_fence_ops;
static inline struct gsgpu_fence *to_gsgpu_fence(struct dma_fence *f)
{
	struct gsgpu_fence *__f = container_of(f, struct gsgpu_fence, base);

	if (__f->base.ops == &gsgpu_fence_ops)
		return __f;

	return NULL;
}

/**
 * gsgpu_fence_write - write a fence value
 *
 * @ring: ring the fence is associated with
 * @seq: sequence number to write
 *
 * Writes a fence value to memory (all asics).
 */
static void gsgpu_fence_write(struct gsgpu_ring *ring, u32 seq)
{
	struct gsgpu_fence_driver *drv = &ring->fence_drv;

	if (drv->cpu_addr)
		*drv->cpu_addr = cpu_to_le32(seq);
}

/**
 * gsgpu_fence_read - read a fence value
 *
 * @ring: ring the fence is associated with
 *
 * Reads a fence value from memory (all asics).
 * Returns the value of the fence read from memory.
 */
static u32 gsgpu_fence_read(struct gsgpu_ring *ring)
{
	struct gsgpu_fence_driver *drv = &ring->fence_drv;
	u32 seq = 0;

	if (drv->cpu_addr)
		seq = le32_to_cpu(*drv->cpu_addr);
	else
		seq = atomic_read(&drv->last_seq);

	return seq;
}

/**
 * gsgpu_fence_emit - emit a fence on the requested ring
 *
 * @ring: ring the fence is associated with
 * @f: resulting fence object
 *
 * Emits a fence command on the requested ring (all asics).
 * Returns 0 on success, -ENOMEM on failure.
 */
int gsgpu_fence_emit(struct gsgpu_ring *ring, struct dma_fence **f,
		      unsigned flags)
{
	struct gsgpu_device *adev = ring->adev;
	struct gsgpu_fence *fence;
	struct dma_fence __rcu **ptr;
	uint32_t seq;
	int r;

	fence = kmem_cache_alloc(gsgpu_fence_slab, GFP_KERNEL);
	if (fence == NULL)
		return -ENOMEM;

	seq = ++ring->fence_drv.sync_seq;
	fence->ring = ring;
	dma_fence_init(&fence->base, &gsgpu_fence_ops,
		       &ring->fence_drv.lock,
		       adev->fence_context + ring->idx,
		       seq);
	gsgpu_ring_emit_fence(ring, ring->fence_drv.gpu_addr,
			       seq, flags | GSGPU_FENCE_FLAG_INT);

	ptr = &ring->fence_drv.fences[seq & ring->fence_drv.num_fences_mask];
	if (unlikely(rcu_dereference_protected(*ptr, 1))) {
		struct dma_fence *old;

		rcu_read_lock();
		old = dma_fence_get_rcu_safe(ptr);
		rcu_read_unlock();

		if (old) {
			r = dma_fence_wait(old, false);
			dma_fence_put(old);
			if (r)
				return r;
		}
	}

	/* This function can't be called concurrently anyway, otherwise
	 * emitting the fence would mess up the hardware ring buffer.
	 */
	rcu_assign_pointer(*ptr, dma_fence_get(&fence->base));

	*f = &fence->base;

	return 0;
}

/**
 * gsgpu_fence_emit_polling - emit a fence on the requeste ring
 *
 * @ring: ring the fence is associated with
 * @s: resulting sequence number
 *
 * Emits a fence command on the requested ring (all asics).
 * Used For polling fence.
 * Returns 0 on success, -ENOMEM on failure.
 */
int gsgpu_fence_emit_polling(struct gsgpu_ring *ring, uint32_t *s)
{
	uint32_t seq;

	if (!s)
		return -EINVAL;

	seq = ++ring->fence_drv.sync_seq;
	gsgpu_ring_emit_fence(ring, ring->fence_drv.gpu_addr,
			       seq, 0);

	*s = seq;

	return 0;
}

/**
 * gsgpu_fence_schedule_fallback - schedule fallback check
 *
 * @ring: pointer to struct gsgpu_ring
 *
 * Start a timer as fallback to our interrupts.
 */
static void gsgpu_fence_schedule_fallback(struct gsgpu_ring *ring)
{
	mod_timer(&ring->fence_drv.fallback_timer,
		  jiffies + GSGPU_FENCE_JIFFIES_TIMEOUT);
}

/**
 * gsgpu_fence_process - check for fence activity
 *
 * @ring: pointer to struct gsgpu_ring
 *
 * Checks the current fence value and calculates the last
 * signalled fence value. Wakes the fence queue if the
 * sequence number has increased.
 */
void gsgpu_fence_process(struct gsgpu_ring *ring)
{
	struct gsgpu_fence_driver *drv = &ring->fence_drv;
	uint32_t seq, last_seq;
	int r;

	do {
		last_seq = atomic_read(&ring->fence_drv.last_seq);
		seq = gsgpu_fence_read(ring);

	} while (atomic_cmpxchg(&drv->last_seq, last_seq, seq) != last_seq);

	if (seq != ring->fence_drv.sync_seq)
		gsgpu_fence_schedule_fallback(ring);

	if (unlikely(seq == last_seq))
		return;

	last_seq &= drv->num_fences_mask;
	seq &= drv->num_fences_mask;

	do {
		struct dma_fence *fence, **ptr;

		++last_seq;
		last_seq &= drv->num_fences_mask;
		ptr = &drv->fences[last_seq];

		/* There is always exactly one thread signaling this fence slot */
		fence = rcu_dereference_protected(*ptr, 1);
		RCU_INIT_POINTER(*ptr, NULL);

		if (!fence)
			continue;

		r = dma_fence_signal(fence);
		if (r)
			BUG();

		dma_fence_put(fence);
	} while (last_seq != seq);
}

/**
 * gsgpu_fence_fallback - fallback for hardware interrupts
 *
 * @work: delayed work item
 *
 * Checks for fence activity.
 */
static void gsgpu_fence_fallback(struct timer_list *t)
{
	struct gsgpu_ring *ring = from_timer(ring, t,
					      fence_drv.fallback_timer);

	gsgpu_fence_process(ring);
}

/**
 * gsgpu_fence_wait_empty - wait for all fences to signal
 *
 * @adev: gsgpu device pointer
 * @ring: ring index the fence is associated with
 *
 * Wait for all fences on the requested ring to signal (all asics).
 * Returns 0 if the fences have passed, error for all other cases.
 */
int gsgpu_fence_wait_empty(struct gsgpu_ring *ring)
{
	uint64_t seq = READ_ONCE(ring->fence_drv.sync_seq);
	struct dma_fence *fence, **ptr;
	int r;

	if (!seq)
		return 0;

	ptr = &ring->fence_drv.fences[seq & ring->fence_drv.num_fences_mask];
	rcu_read_lock();
	fence = rcu_dereference(*ptr);
	if (!fence || !dma_fence_get_rcu(fence)) {
		rcu_read_unlock();
		return 0;
	}
	rcu_read_unlock();

	r = dma_fence_wait(fence, false);
	dma_fence_put(fence);
	return r;
}

/**
 * gsgpu_fence_wait_polling - busy wait for givn sequence number
 *
 * @ring: ring index the fence is associated with
 * @wait_seq: sequence number to wait
 * @timeout: the timeout for waiting in usecs
 *
 * Wait for all fences on the requested ring to signal (all asics).
 * Returns left time if no timeout, 0 or minus if timeout.
 */
signed long gsgpu_fence_wait_polling(struct gsgpu_ring *ring,
				      uint32_t wait_seq,
				      signed long timeout)
{
	uint32_t seq;

	do {
		seq = gsgpu_fence_read(ring);
		udelay(5);
		timeout -= 5;
	} while ((int32_t)(wait_seq - seq) > 0 && timeout > 0);

	return timeout > 0 ? timeout : 0;
}
/**
 * gsgpu_fence_count_emitted - get the count of emitted fences
 *
 * @ring: ring the fence is associated with
 *
 * Get the number of fences emitted on the requested ring (all asics).
 * Returns the number of emitted fences on the ring.  Used by the
 * dynpm code to ring track activity.
 */
unsigned gsgpu_fence_count_emitted(struct gsgpu_ring *ring)
{
	uint64_t emitted;

	/* We are not protected by ring lock when reading the last sequence
	 * but it's ok to report slightly wrong fence count here.
	 */
	gsgpu_fence_process(ring);
	emitted = 0x100000000ull;
	emitted -= atomic_read(&ring->fence_drv.last_seq);
	emitted += READ_ONCE(ring->fence_drv.sync_seq);
	return lower_32_bits(emitted);
}

/**
 * gsgpu_fence_driver_start_ring - make the fence driver
 * ready for use on the requested ring.
 *
 * @ring: ring to start the fence driver on
 * @irq_src: interrupt source to use for this ring
 * @irq_type: interrupt type to use for this ring
 *
 * Make the fence driver ready for processing (all asics).
 * Not all asics have all rings, so each asic will only
 * start the fence driver on the rings it has.
 * Returns 0 for success, errors for failure.
 */
int gsgpu_fence_driver_start_ring(struct gsgpu_ring *ring,
				   struct gsgpu_irq_src *irq_src,
				   unsigned irq_type)
{
	struct gsgpu_device *adev = ring->adev;

	ring->fence_drv.cpu_addr = &adev->wb.wb[ring->fence_offs];
	ring->fence_drv.gpu_addr = adev->wb.gpu_addr + (ring->fence_offs * 4);
	gsgpu_fence_write(ring, atomic_read(&ring->fence_drv.last_seq));
	gsgpu_irq_get(adev, irq_src, irq_type);

	ring->fence_drv.irq_src = irq_src;
	ring->fence_drv.irq_type = irq_type;
	ring->fence_drv.initialized = true;

	dev_dbg(adev->dev, "fence driver on ring %d use gpu addr 0x%016llx, "
		"cpu addr 0x0x%px\n", ring->idx,
		ring->fence_drv.gpu_addr, ring->fence_drv.cpu_addr);
	return 0;
}

/**
 * gsgpu_fence_driver_init_ring - init the fence driver
 * for the requested ring.
 *
 * @ring: ring to init the fence driver on
 * @num_hw_submission: number of entries on the hardware queue
 *
 * Init the fence driver for the requested ring (all asics).
 * Helper function for gsgpu_fence_driver_init().
 */
int gsgpu_fence_driver_init_ring(struct gsgpu_ring *ring,
				  unsigned num_hw_submission)
{
	long timeout;
	int r;

	/* Check that num_hw_submission is a power of two */
	if ((num_hw_submission & (num_hw_submission - 1)) != 0)
		return -EINVAL;

	ring->fence_drv.cpu_addr = NULL;
	ring->fence_drv.gpu_addr = 0;
	ring->fence_drv.sync_seq = 0;
	atomic_set(&ring->fence_drv.last_seq, 0);
	ring->fence_drv.initialized = false;

	timer_setup(&ring->fence_drv.fallback_timer, gsgpu_fence_fallback, 0);

	ring->fence_drv.num_fences_mask = num_hw_submission * 2 - 1;
	spin_lock_init(&ring->fence_drv.lock);
	ring->fence_drv.fences = kcalloc(num_hw_submission * 2, sizeof(void *),
					 GFP_KERNEL);
	if (!ring->fence_drv.fences)
		return -ENOMEM;

	timeout = msecs_to_jiffies(gsgpu_lockup_timeout);

	r = drm_sched_init(&ring->sched, &gsgpu_sched_ops,
			   num_hw_submission, gsgpu_job_hang_limit,
			   timeout, NULL, NULL, ring->name, ring->adev->dev);
	if (r) {
		DRM_ERROR("Failed to create scheduler on ring %s.\n",
			  ring->name);
		return r;
	}

	return 0;
}

/**
 * gsgpu_fence_driver_init - init the fence driver
 * for all possible rings.
 *
 * @adev: gsgpu device pointer
 *
 * Init the fence driver for all possible rings (all asics).
 * Not all asics have all rings, so each asic will only
 * start the fence driver on the rings it has using
 * gsgpu_fence_driver_start_ring().
 * Returns 0 for success.
 */
int gsgpu_fence_driver_init(struct gsgpu_device *adev)
{
	if (gsgpu_debugfs_fence_init(adev))
		dev_err(adev->dev, "fence debugfs file creation failed\n");

	return 0;
}

/**
 * gsgpu_fence_driver_fini - tear down the fence driver
 * for all possible rings.
 *
 * @adev: gsgpu device pointer
 *
 * Tear down the fence driver for all possible rings (all asics).
 */
void gsgpu_fence_driver_fini(struct gsgpu_device *adev)
{
	unsigned i, j;
	int r;

	for (i = 0; i < GSGPU_MAX_RINGS; i++) {
		struct gsgpu_ring *ring = adev->rings[i];

		if (!ring || !ring->fence_drv.initialized)
			continue;
		r = gsgpu_fence_wait_empty(ring);
		if (r) {
			/* no need to trigger GPU reset as we are unloading */
			gsgpu_fence_driver_force_completion(ring);
		}
		gsgpu_irq_put(adev, ring->fence_drv.irq_src,
			       ring->fence_drv.irq_type);
		drm_sched_fini(&ring->sched);
		del_timer_sync(&ring->fence_drv.fallback_timer);
		for (j = 0; j <= ring->fence_drv.num_fences_mask; ++j)
			dma_fence_put(ring->fence_drv.fences[j]);
		kfree(ring->fence_drv.fences);
		ring->fence_drv.fences = NULL;
		ring->fence_drv.initialized = false;
	}
}

/**
 * gsgpu_fence_driver_suspend - suspend the fence driver
 * for all possible rings.
 *
 * @adev: gsgpu device pointer
 *
 * Suspend the fence driver for all possible rings (all asics).
 */
void gsgpu_fence_driver_suspend(struct gsgpu_device *adev)
{
	int i, r;

	for (i = 0; i < GSGPU_MAX_RINGS; i++) {
		struct gsgpu_ring *ring = adev->rings[i];
		if (!ring || !ring->fence_drv.initialized)
			continue;

		/* wait for gpu to finish processing current batch */
		r = gsgpu_fence_wait_empty(ring);
		if (r) {
			/* delay GPU reset to resume */
			gsgpu_fence_driver_force_completion(ring);
		}

		/* disable the interrupt */
		gsgpu_irq_put(adev, ring->fence_drv.irq_src,
			       ring->fence_drv.irq_type);
	}
}

/**
 * gsgpu_fence_driver_resume - resume the fence driver
 * for all possible rings.
 *
 * @adev: gsgpu device pointer
 *
 * Resume the fence driver for all possible rings (all asics).
 * Not all asics have all rings, so each asic will only
 * start the fence driver on the rings it has using
 * gsgpu_fence_driver_start_ring().
 * Returns 0 for success.
 */
void gsgpu_fence_driver_resume(struct gsgpu_device *adev)
{
	int i;

	for (i = 0; i < GSGPU_MAX_RINGS; i++) {
		struct gsgpu_ring *ring = adev->rings[i];
		if (!ring || !ring->fence_drv.initialized)
			continue;

		/* enable the interrupt */
		gsgpu_irq_get(adev, ring->fence_drv.irq_src,
			       ring->fence_drv.irq_type);
	}
}

/**
 * gsgpu_fence_driver_force_completion - force signal latest fence of ring
 *
 * @ring: fence of the ring to signal
 *
 */
void gsgpu_fence_driver_force_completion(struct gsgpu_ring *ring)
{
	gsgpu_fence_write(ring, ring->fence_drv.sync_seq);
	gsgpu_fence_process(ring);
}

/*
 * Common fence implementation
 */

static const char *gsgpu_fence_get_driver_name(struct dma_fence *fence)
{
	return "gsgpu";
}

static const char *gsgpu_fence_get_timeline_name(struct dma_fence *f)
{
	struct gsgpu_fence *fence = to_gsgpu_fence(f);
	return (const char *)fence->ring->name;
}

/**
 * gsgpu_fence_enable_signaling - enable signalling on fence
 * @fence: fence
 *
 * This function is called with fence_queue lock held, and adds a callback
 * to fence_queue that checks if this fence is signaled, and if so it
 * signals the fence and removes itself.
 */
static bool gsgpu_fence_enable_signaling(struct dma_fence *f)
{
	struct gsgpu_fence *fence = to_gsgpu_fence(f);
	struct gsgpu_ring *ring = fence->ring;

	if (!timer_pending(&ring->fence_drv.fallback_timer))
		gsgpu_fence_schedule_fallback(ring);

	return true;
}

/**
 * gsgpu_fence_free - free up the fence memory
 *
 * @rcu: RCU callback head
 *
 * Free up the fence memory after the RCU grace period.
 */
static void gsgpu_fence_free(struct rcu_head *rcu)
{
	struct dma_fence *f = container_of(rcu, struct dma_fence, rcu);
	struct gsgpu_fence *fence = to_gsgpu_fence(f);
	kmem_cache_free(gsgpu_fence_slab, fence);
}

/**
 * gsgpu_fence_release - callback that fence can be freed
 *
 * @fence: fence
 *
 * This function is called when the reference count becomes zero.
 * It just RCU schedules freeing up the fence.
 */
static void gsgpu_fence_release(struct dma_fence *f)
{
	call_rcu(&f->rcu, gsgpu_fence_free);
}

static const struct dma_fence_ops gsgpu_fence_ops = {
	.get_driver_name = gsgpu_fence_get_driver_name,
	.get_timeline_name = gsgpu_fence_get_timeline_name,
	.enable_signaling = gsgpu_fence_enable_signaling,
	.release = gsgpu_fence_release,
};

/*
 * Fence debugfs
 */
#if defined(CONFIG_DEBUG_FS)
static int gsgpu_debugfs_fence_info(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *)m->private;
	struct drm_device *dev = node->minor->dev;
	struct gsgpu_device *adev = dev->dev_private;
	int i;

	for (i = 0; i < GSGPU_MAX_RINGS; ++i) {
		struct gsgpu_ring *ring = adev->rings[i];
		if (!ring || !ring->fence_drv.initialized)
			continue;

		gsgpu_fence_process(ring);

		seq_printf(m, "--- ring %d (%s) ---\n", i, ring->name);
		seq_printf(m, "Last signaled fence 0x%08x\n",
			   atomic_read(&ring->fence_drv.last_seq));
		seq_printf(m, "Last emitted        0x%08x\n",
			   ring->fence_drv.sync_seq);

		if (ring->funcs->type != GSGPU_RING_TYPE_GFX)
			continue;

		/* set in CP_VMID_PREEMPT and preemption occurred */
		seq_printf(m, "Last preempted      0x%08x\n",
			   le32_to_cpu(*(ring->fence_drv.cpu_addr + 2)));
		/* set in CP_VMID_RESET and reset occurred */
		seq_printf(m, "Last reset          0x%08x\n",
			   le32_to_cpu(*(ring->fence_drv.cpu_addr + 4)));
		/* Both preemption and reset occurred */
		seq_printf(m, "Last both           0x%08x\n",
			   le32_to_cpu(*(ring->fence_drv.cpu_addr + 6)));
	}
	return 0;
}

/**
 * gsgpu_debugfs_gpu_recover - manually trigger a gpu reset & recover
 *
 * Manually trigger a gpu reset at the next fence wait.
 */
static int gsgpu_debugfs_gpu_recover(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct gsgpu_device *adev = dev->dev_private;

	seq_printf(m, "gpu recover\n");
	gsgpu_device_gpu_recover(adev, NULL, true);

	return 0;
}

static const struct drm_info_list gsgpu_debugfs_fence_list[] = {
	{"gsgpu_fence_info", &gsgpu_debugfs_fence_info, 0, NULL},
	{"gsgpu_gpu_recover", &gsgpu_debugfs_gpu_recover, 0, NULL}
};

#endif

int gsgpu_debugfs_fence_init(struct gsgpu_device *adev)
{
#if defined(CONFIG_DEBUG_FS)
	return gsgpu_debugfs_add_files(adev, gsgpu_debugfs_fence_list, 2);
#else
	return 0;
#endif
}


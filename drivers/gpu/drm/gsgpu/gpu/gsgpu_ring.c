#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <drm/gsgpu_drm.h>
#include "gsgpu.h"

/*
 * Rings
 * Most engines on the GPU are fed via ring buffers.  Ring
 * buffers are areas of GPU accessible memory that the host
 * writes commands into and the GPU reads commands out of.
 * There is a rptr (read pointer) that determines where the
 * GPU is currently reading, and a wptr (write pointer)
 * which determines where the host has written.  When the
 * pointers are equal, the ring is idle.  When the host
 * writes commands to the ring buffer, it increments the
 * wptr.  The GPU then starts fetching commands and executes
 * them until the pointers are equal again.
 */
static int gsgpu_debugfs_ring_init(struct gsgpu_device *adev,
				    struct gsgpu_ring *ring);
static void gsgpu_debugfs_ring_fini(struct gsgpu_ring *ring);

/**
 * gsgpu_ring_alloc - allocate space on the ring buffer
 *
 * @adev: gsgpu_device pointer
 * @ring: gsgpu_ring structure holding ring information
 * @ndw: number of dwords to allocate in the ring buffer
 *
 * Allocate @ndw dwords in the ring buffer (all asics).
 * Returns 0 on success, error on failure.
 */
int gsgpu_ring_alloc(struct gsgpu_ring *ring, unsigned ndw)
{
	/* Align requested size with padding so unlock_commit can
	 * pad safely */
	ndw = (ndw + ring->funcs->align_mask) & ~ring->funcs->align_mask;

	/* Make sure we aren't trying to allocate more space
	 * than the maximum for one submission
	 */
	if (WARN_ON_ONCE(ndw > ring->max_dw))
		return -ENOMEM;

	ring->count_dw = ndw;
	ring->wptr_old = ring->wptr;

	if (ring->funcs->begin_use)
		ring->funcs->begin_use(ring);

	return 0;
}

/** gsgpu_ring_insert_nop - insert NOP packets
 *
 * @ring: gsgpu_ring structure holding ring information
 * @count: the number of NOP packets to insert
 *
 * This is the generic insert_nop function for rings except XDMA
 */
void gsgpu_ring_insert_nop(struct gsgpu_ring *ring, uint32_t count)
{
	int i;

	for (i = 0; i < count; i++)
		gsgpu_ring_write(ring, ring->funcs->nop);
}

/** gsgpu_ring_generic_pad_ib - pad IB with NOP packets
 *
 * @ring: gsgpu_ring structure holding ring information
 * @ib: IB to add NOP packets to
 *
 * This is the generic pad_ib function for rings except XDMA
 */
void gsgpu_ring_generic_pad_ib(struct gsgpu_ring *ring, struct gsgpu_ib *ib)
{
	while (ib->length_dw & ring->funcs->align_mask)
		ib->ptr[ib->length_dw++] = ring->funcs->nop;
}

/**
 * gsgpu_ring_commit - tell the GPU to execute the new
 * commands on the ring buffer
 *
 * @adev: gsgpu_device pointer
 * @ring: gsgpu_ring structure holding ring information
 *
 * Update the wptr (write pointer) to tell the GPU to
 * execute new commands on the ring buffer (all asics).
 */
void gsgpu_ring_commit(struct gsgpu_ring *ring)
{
	uint32_t count;

	/* We pad to match fetch size */
	count = ring->funcs->align_mask + 1 -
		(ring->wptr & ring->funcs->align_mask);
	count %= ring->funcs->align_mask + 1;
	ring->funcs->insert_nop(ring, count);

	mb();
	gsgpu_ring_set_wptr(ring);

	if (ring->funcs->end_use)
		ring->funcs->end_use(ring);

	gsgpu_ring_lru_touch(ring->adev, ring);
}

/**
 * gsgpu_ring_undo - reset the wptr
 *
 * @ring: gsgpu_ring structure holding ring information
 *
 * Reset the driver's copy of the wptr (all asics).
 */
void gsgpu_ring_undo(struct gsgpu_ring *ring)
{
	ring->wptr = ring->wptr_old;

	if (ring->funcs->end_use)
		ring->funcs->end_use(ring);
}

/**
 * gsgpu_ring_priority_put - restore a ring's priority
 *
 * @ring: gsgpu_ring structure holding the information
 * @priority: target priority
 *
 * Release a request for executing at @priority
 */
void gsgpu_ring_priority_put(struct gsgpu_ring *ring,
			      enum drm_sched_priority priority)
{
	int i;

	if (!ring->funcs->set_priority)
		return;

	if (atomic_dec_return(&ring->num_jobs[priority]) > 0)
		return;

	/* no need to restore if the job is already at the lowest priority */
	if (priority == DRM_SCHED_PRIORITY_NORMAL)
		return;

	mutex_lock(&ring->priority_mutex);
	/* something higher prio is executing, no need to decay */
	if (ring->priority > priority)
		goto out_unlock;

	/* decay priority to the next level with a job available */
	for (i = priority; i >= DRM_SCHED_PRIORITY_MIN; i--) {
		if (i == DRM_SCHED_PRIORITY_NORMAL
				|| atomic_read(&ring->num_jobs[i])) {
			ring->priority = i;
			ring->funcs->set_priority(ring, i);
			break;
		}
	}

out_unlock:
	mutex_unlock(&ring->priority_mutex);
}

/**
 * gsgpu_ring_priority_get - change the ring's priority
 *
 * @ring: gsgpu_ring structure holding the information
 * @priority: target priority
 *
 * Request a ring's priority to be raised to @priority (refcounted).
 */
void gsgpu_ring_priority_get(struct gsgpu_ring *ring,
			      enum drm_sched_priority priority)
{
	if (!ring->funcs->set_priority)
		return;

	if (atomic_inc_return(&ring->num_jobs[priority]) <= 0)
		return;

	mutex_lock(&ring->priority_mutex);
	if (priority <= ring->priority)
		goto out_unlock;

	ring->priority = priority;
	ring->funcs->set_priority(ring, priority);

out_unlock:
	mutex_unlock(&ring->priority_mutex);
}

/**
 * gsgpu_ring_init - init driver ring struct.
 *
 * @adev: gsgpu_device pointer
 * @ring: gsgpu_ring structure holding ring information
 * @max_ndw: maximum number of dw for ring alloc
 * @nop: nop packet for this ring
 *
 * Initialize the driver information for the selected ring (all asics).
 * Returns 0 on success, error on failure.
 */
int gsgpu_ring_init(struct gsgpu_device *adev, struct gsgpu_ring *ring,
		     unsigned max_dw, struct gsgpu_irq_src *irq_src,
		     unsigned irq_type)
{
	int r, i;
	int sched_hw_submission = gsgpu_sched_hw_submission;

	if (ring->adev == NULL) {
		if (adev->num_rings >= GSGPU_MAX_RINGS)
			return -EINVAL;

		ring->adev = adev;
		ring->idx = adev->num_rings++;
		adev->rings[ring->idx] = ring;
		r = gsgpu_fence_driver_init_ring(ring, sched_hw_submission);
		if (r)
			return r;
	}

	r = gsgpu_device_wb_get(adev, &ring->rptr_offs);
	if (r) {
		dev_err(adev->dev, "(%d) ring rptr_offs wb alloc failed\n", r);
		return r;
	}

	r = gsgpu_device_wb_get(adev, &ring->wptr_offs);
	if (r) {
		dev_err(adev->dev, "(%d) ring wptr_offs wb alloc failed\n", r);
		return r;
	}

	r = gsgpu_device_wb_get(adev, &ring->fence_offs);
	if (r) {
		dev_err(adev->dev, "(%d) ring fence_offs wb alloc failed\n", r);
		return r;
	}

	r = gsgpu_device_wb_get(adev, &ring->cond_exe_offs);
	if (r) {
		dev_err(adev->dev, "(%d) ring cond_exec_polling wb alloc failed\n", r);
		return r;
	}
	ring->cond_exe_gpu_addr = adev->wb.gpu_addr + (ring->cond_exe_offs * 4);
	ring->cond_exe_cpu_addr = &adev->wb.wb[ring->cond_exe_offs];
	/* always set cond_exec_polling to CONTINUE */
	*ring->cond_exe_cpu_addr = 1;

	r = gsgpu_fence_driver_start_ring(ring, irq_src, irq_type);
	if (r) {
		dev_err(adev->dev, "failed initializing fences (%d).\n", r);
		return r;
	}

	ring->ring_size = roundup_pow_of_two(max_dw * 4 * sched_hw_submission);

	ring->buf_mask = ring->ring_size/4 - 1;
	ring->ptr_mask = ring->funcs->support_64bit_ptrs ?
		0xffffffffffffffff : ring->buf_mask;
	/* Allocate ring buffer */
	if (ring->ring_obj == NULL) {
		r = gsgpu_bo_create_kernel(adev, ring->ring_size + ring->funcs->extra_dw, PAGE_SIZE,
					    GSGPU_GEM_DOMAIN_GTT,
					    &ring->ring_obj,
					    &ring->gpu_addr,
					    (void **)&ring->ring);
		if (r) {
			dev_err(adev->dev, "(%d) ring create failed\n", r);
			return r;
		}
		gsgpu_ring_clear_ring(ring);
	}

	ring->max_dw = max_dw;
	ring->priority = DRM_SCHED_PRIORITY_NORMAL;
	mutex_init(&ring->priority_mutex);
	INIT_LIST_HEAD(&ring->lru_list);
	gsgpu_ring_lru_touch(adev, ring);

	for (i = 0; i < DRM_SCHED_PRIORITY_COUNT; ++i)
		atomic_set(&ring->num_jobs[i], 0);

	if (gsgpu_debugfs_ring_init(adev, ring)) {
		DRM_ERROR("Failed to register debugfs file for rings !\n");
	}

	return 0;
}

/**
 * gsgpu_ring_fini - tear down the driver ring struct.
 *
 * @adev: gsgpu_device pointer
 * @ring: gsgpu_ring structure holding ring information
 *
 * Tear down the driver information for the selected ring (all asics).
 */
void gsgpu_ring_fini(struct gsgpu_ring *ring)
{
	ring->ready = false;

	/* Not to finish a ring which is not initialized */
	if (!(ring->adev) || !(ring->adev->rings[ring->idx]))
		return;

	gsgpu_device_wb_free(ring->adev, ring->rptr_offs);
	gsgpu_device_wb_free(ring->adev, ring->wptr_offs);

	gsgpu_device_wb_free(ring->adev, ring->cond_exe_offs);
	gsgpu_device_wb_free(ring->adev, ring->fence_offs);

	gsgpu_bo_free_kernel(&ring->ring_obj,
			      &ring->gpu_addr,
			      (void **)&ring->ring);

	gsgpu_debugfs_ring_fini(ring);

	dma_fence_put(ring->vmid_wait);
	ring->vmid_wait = NULL;
	ring->me = 0;

	ring->adev->rings[ring->idx] = NULL;
}

static void gsgpu_ring_lru_touch_locked(struct gsgpu_device *adev,
					 struct gsgpu_ring *ring)
{
	/* list_move_tail handles the case where ring isn't part of the list */
	list_move_tail(&ring->lru_list, &adev->ring_lru_list);
}

static bool gsgpu_ring_is_blacklisted(struct gsgpu_ring *ring,
				       int *blacklist, int num_blacklist)
{
	int i;

	for (i = 0; i < num_blacklist; i++) {
		if (ring->idx == blacklist[i])
			return true;
	}

	return false;
}

/**
 * gsgpu_ring_lru_get - get the least recently used ring for a HW IP block
 *
 * @adev: gsgpu_device pointer
 * @type: gsgpu_ring_type enum
 * @blacklist: blacklisted ring ids array
 * @num_blacklist: number of entries in @blacklist
 * @lru_pipe_order: find a ring from the least recently used pipe
 * @ring: output ring
 *
 * Retrieve the gsgpu_ring structure for the least recently used ring of
 * a specific IP block (all asics).
 * Returns 0 on success, error on failure.
 */
int gsgpu_ring_lru_get(struct gsgpu_device *adev, int type,
			int *blacklist,	int num_blacklist,
			bool lru_pipe_order, struct gsgpu_ring **ring)
{
	struct gsgpu_ring *entry;

	/* List is sorted in LRU order, find first entry corresponding
	 * to the desired HW IP */
	*ring = NULL;
	spin_lock(&adev->ring_lru_list_lock);
	list_for_each_entry(entry, &adev->ring_lru_list, lru_list) {
		if (entry->funcs->type != type)
			continue;

		if (gsgpu_ring_is_blacklisted(entry, blacklist, num_blacklist))
			continue;

		if (!*ring) {
			*ring = entry;

			/* We are done for ring LRU */
			if (!lru_pipe_order)
				break;
		}

		/* Move all rings on the same pipe to the end of the list */
		if (entry->pipe == (*ring)->pipe)
			gsgpu_ring_lru_touch_locked(adev, entry);
	}

	/* Move the ring we found to the end of the list */
	if (*ring)
		gsgpu_ring_lru_touch_locked(adev, *ring);

	spin_unlock(&adev->ring_lru_list_lock);

	if (!*ring) {
		DRM_ERROR("Ring LRU contains no entries for ring type:%d\n", type);
		return -EINVAL;
	}

	return 0;
}

/**
 * gsgpu_ring_lru_touch - mark a ring as recently being used
 *
 * @adev: gsgpu_device pointer
 * @ring: ring to touch
 *
 * Move @ring to the tail of the lru list
 */
void gsgpu_ring_lru_touch(struct gsgpu_device *adev, struct gsgpu_ring *ring)
{
	spin_lock(&adev->ring_lru_list_lock);
	gsgpu_ring_lru_touch_locked(adev, ring);
	spin_unlock(&adev->ring_lru_list_lock);
}

/**
 * gsgpu_ring_emit_reg_write_reg_wait_helper - ring helper
 *
 * @adev: gsgpu_device pointer
 * @reg0: register to write
 * @reg1: register to wait on
 * @ref: reference value to write/wait on
 * @mask: mask to wait on
 *
 * Helper for rings that don't support write and wait in a
 * single oneshot packet.
 */
void gsgpu_ring_emit_reg_write_reg_wait_helper(struct gsgpu_ring *ring,
						uint32_t reg0, uint32_t reg1,
						uint32_t ref, uint32_t mask)
{
	gsgpu_ring_emit_wreg(ring, reg0, ref);
	gsgpu_ring_emit_reg_wait(ring, reg1, mask, mask);
}

/*
 * Debugfs info
 */
#if defined(CONFIG_DEBUG_FS)

/* Layout of file is 12 bytes consisting of
 * - rptr
 * - wptr
 * - driver's copy of wptr
 *
 * followed by n-words of ring data
 */
static ssize_t gsgpu_debugfs_ring_read(struct file *f, char __user *buf,
					size_t size, loff_t *pos)
{
	struct gsgpu_ring *ring = file_inode(f)->i_private;
	int r, i;
	uint32_t value, result, early[3];

	if (*pos & 3 || size & 3)
		return -EINVAL;

	result = 0;

	if (*pos < 12) {
		early[0] = gsgpu_ring_get_rptr(ring) & ring->buf_mask;
		early[1] = gsgpu_ring_get_wptr(ring) & ring->buf_mask;
		early[2] = ring->wptr & ring->buf_mask;
		for (i = *pos / 4; i < 3 && size; i++) {
			r = put_user(early[i], (uint32_t *)buf);
			if (r)
				return r;
			buf += 4;
			result += 4;
			size -= 4;
			*pos += 4;
		}
	}

	while (size) {
		if (*pos >= (ring->ring_size + 12))
			return result;

		value = ring->ring[(*pos - 12)/4];
		r = put_user(value, (uint32_t *)buf);
		if (r)
			return r;
		buf += 4;
		result += 4;
		size -= 4;
		*pos += 4;
	}

	return result;
}

static const struct file_operations gsgpu_debugfs_ring_fops = {
	.owner = THIS_MODULE,
	.read = gsgpu_debugfs_ring_read,
	.llseek = default_llseek
};

#endif

static int gsgpu_debugfs_ring_init(struct gsgpu_device *adev,
				    struct gsgpu_ring *ring)
{
#if defined(CONFIG_DEBUG_FS)
	struct drm_minor *minor = adev->ddev->primary;
	struct dentry *ent, *root = minor->debugfs_root;
	char name[32];

	sprintf(name, "gsgpu_ring_%s", ring->name);

	ent = debugfs_create_file(name,
				  S_IFREG | S_IRUGO, root,
				  ring, &gsgpu_debugfs_ring_fops);
	if (!ent)
		return -ENOMEM;

	i_size_write(ent->d_inode, ring->ring_size + 12);
	ring->ent = ent;
#endif
	return 0;
}

static void gsgpu_debugfs_ring_fini(struct gsgpu_ring *ring)
{
#if defined(CONFIG_DEBUG_FS)
	debugfs_remove(ring->ent);
#endif
}

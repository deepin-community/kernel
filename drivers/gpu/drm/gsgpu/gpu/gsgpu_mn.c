#include <linux/firmware.h>
#include <linux/module.h>
#include <linux/mmu_notifier.h>
#include <linux/interval_tree.h>
#include <drm/drmP.h>
#include <drm/drm.h>

#include "gsgpu.h"

/**
 * struct gsgpu_mn
 *
 * @adev: gsgpu device pointer
 * @mm: process address space
 * @mn: MMU notifier structure
 * @type: type of MMU notifier
 * @work: destruction work item
 * @node: hash table node to find structure by adev and mn
 * @lock: rw semaphore protecting the notifier nodes
 * @objects: interval tree containing gsgpu_mn_nodes
 * @read_lock: mutex for recursive locking of @lock
 * @recursion: depth of recursion
 *
 * Data for each gsgpu device and process address space.
 */
struct gsgpu_mn {
	/* constant after initialisation */
	struct gsgpu_device	*adev;
	struct mm_struct	*mm;
	struct mmu_notifier	mn;
	enum gsgpu_mn_type	type;

	/* only used on destruction */
	struct work_struct	work;

	/* protected by adev->mn_lock */
	struct hlist_node	node;

	/* objects protected by lock */
	struct rw_semaphore	lock;
	struct rb_root_cached	objects;
	struct mutex		read_lock;
	atomic_t		recursion;
};

/**
 * struct gsgpu_mn_node
 *
 * @it: interval node defining start-last of the affected address range
 * @bos: list of all BOs in the affected address range
 *
 * Manages all BOs which are affected of a certain range of address space.
 */
struct gsgpu_mn_node {
	struct interval_tree_node	it;
	struct list_head		bos;
};

/**
 * gsgpu_mn_destroy - destroy the MMU notifier
 *
 * @work: previously sheduled work item
 *
 * Lazy destroys the notifier from a work item
 */
static void gsgpu_mn_destroy(struct work_struct *work)
{
	struct gsgpu_mn *amn = container_of(work, struct gsgpu_mn, work);
	struct gsgpu_device *adev = amn->adev;
	struct gsgpu_mn_node *node, *next_node;
	struct gsgpu_bo *bo, *next_bo;

	mutex_lock(&adev->mn_lock);
	down_write(&amn->lock);
	hash_del(&amn->node);
	rbtree_postorder_for_each_entry_safe(node, next_node,
					     &amn->objects.rb_root, it.rb) {
		list_for_each_entry_safe(bo, next_bo, &node->bos, mn_list) {
			bo->mn = NULL;
			list_del_init(&bo->mn_list);
		}
		kfree(node);
	}
	up_write(&amn->lock);
	mutex_unlock(&adev->mn_lock);
	mmu_notifier_unregister_no_release(&amn->mn, amn->mm);
	kfree(amn);
}

/**
 * gsgpu_mn_release - callback to notify about mm destruction
 *
 * @mn: our notifier
 * @mm: the mm this callback is about
 *
 * Shedule a work item to lazy destroy our notifier.
 */
static void gsgpu_mn_release(struct mmu_notifier *mn,
			      struct mm_struct *mm)
{
	struct gsgpu_mn *amn = container_of(mn, struct gsgpu_mn, mn);

	INIT_WORK(&amn->work, gsgpu_mn_destroy);
	schedule_work(&amn->work);
}


/**
 * gsgpu_mn_lock - take the write side lock for this notifier
 *
 * @mn: our notifier
 */
void gsgpu_mn_lock(struct gsgpu_mn *mn)
{
	if (mn)
		down_write(&mn->lock);
}

/**
 * gsgpu_mn_unlock - drop the write side lock for this notifier
 *
 * @mn: our notifier
 */
void gsgpu_mn_unlock(struct gsgpu_mn *mn)
{
	if (mn)
		up_write(&mn->lock);
}

/**
 * gsgpu_mn_read_lock - take the read side lock for this notifier
 *
 * @amn: our notifier
 */
static int gsgpu_mn_read_lock(struct gsgpu_mn *amn, bool blockable)
{
	if (blockable)
		mutex_lock(&amn->read_lock);
	else if (!mutex_trylock(&amn->read_lock))
		return -EAGAIN;

	if (atomic_inc_return(&amn->recursion) == 1)
		down_read_non_owner(&amn->lock);
	mutex_unlock(&amn->read_lock);

	return 0;
}

/**
 * gsgpu_mn_read_unlock - drop the read side lock for this notifier
 *
 * @amn: our notifier
 */
static void gsgpu_mn_read_unlock(struct gsgpu_mn *amn)
{
	if (atomic_dec_return(&amn->recursion) == 0)
		up_read_non_owner(&amn->lock);
}

/**
 * gsgpu_mn_invalidate_node - unmap all BOs of a node
 *
 * @node: the node with the BOs to unmap
 * @start: start of address range affected
 * @end: end of address range affected
 *
 * Block for operations on BOs to finish and mark pages as accessed and
 * potentially dirty.
 */
static void gsgpu_mn_invalidate_node(struct gsgpu_mn_node *node,
				      unsigned long start,
				      unsigned long end)
{
	struct gsgpu_bo *bo;
	long r;

	list_for_each_entry(bo, &node->bos, mn_list) {

		if (!gsgpu_ttm_tt_affect_userptr(bo->tbo.ttm, start, end))
			continue;

		r = dma_resv_wait_timeout_rcu(bo->tbo.base.resv,
			true, false, MAX_SCHEDULE_TIMEOUT);
		if (r <= 0)
			DRM_ERROR("(%ld) failed to wait for user bo\n", r);

		gsgpu_ttm_tt_mark_user_pages(bo->tbo.ttm);
	}
}

/**
 * gsgpu_mn_invalidate_range_start_gfx - callback to notify about mm change
 *
 * @mn: our notifier
 * @mm: the mm this callback is about
 * @start: start of updated range
 * @end: end of updated range
 *
 * Block for operations on BOs to finish and mark pages as accessed and
 * potentially dirty.
 */
static int gsgpu_mn_invalidate_range_start_gfx(struct mmu_notifier *mn,
						 struct mm_struct *mm,
						 unsigned long start,
						 unsigned long end,
						 bool blockable)
{
	struct gsgpu_mn *amn = container_of(mn, struct gsgpu_mn, mn);
	struct interval_tree_node *it;

	/* notification is exclusive, but interval is inclusive */
	end -= 1;

	/* TODO we should be able to split locking for interval tree and
	 * gsgpu_mn_invalidate_node
	 */
	if (gsgpu_mn_read_lock(amn, blockable))
		return -EAGAIN;

	it = interval_tree_iter_first(&amn->objects, start, end);
	while (it) {
		struct gsgpu_mn_node *node;

		if (!blockable) {
			gsgpu_mn_read_unlock(amn);
			return -EAGAIN;
		}

		node = container_of(it, struct gsgpu_mn_node, it);
		it = interval_tree_iter_next(it, start, end);

		gsgpu_mn_invalidate_node(node, start, end);
	}

	return 0;
}

/**
 * gsgpu_mn_invalidate_range_end - callback to notify about mm change
 *
 * @mn: our notifier
 * @mm: the mm this callback is about
 * @start: start of updated range
 * @end: end of updated range
 *
 * Release the lock again to allow new command submissions.
 */
static void gsgpu_mn_invalidate_range_end(struct mmu_notifier *mn,
					   struct mm_struct *mm,
					   unsigned long start,
					   unsigned long end)
{
	struct gsgpu_mn *amn = container_of(mn, struct gsgpu_mn, mn);

	gsgpu_mn_read_unlock(amn);
}

static const struct mmu_notifier_ops gsgpu_mn_ops[] = {
	[GSGPU_MN_TYPE_GFX] = {
		.release = gsgpu_mn_release,
		.invalidate_range_start = gsgpu_mn_invalidate_range_start_gfx,
		.invalidate_range_end = gsgpu_mn_invalidate_range_end,
	},
};

/* Low bits of any reasonable mm pointer will be unused due to struct
 * alignment. Use these bits to make a unique key from the mm pointer
 * and notifier type.
 */
#define GSGPU_MN_KEY(mm, type) ((unsigned long)(mm) + (type))

/**
 * gsgpu_mn_get - create notifier context
 *
 * @adev: gsgpu device pointer
 * @type: type of MMU notifier context
 *
 * Creates a notifier context for current->mm.
 */
struct gsgpu_mn *gsgpu_mn_get(struct gsgpu_device *adev,
				enum gsgpu_mn_type type)
{
	struct mm_struct *mm = current->mm;
	struct gsgpu_mn *amn;
	unsigned long key = GSGPU_MN_KEY(mm, type);
	int r;

	mutex_lock(&adev->mn_lock);
	if (down_write_killable(&mm->mmap_sem)) {
		mutex_unlock(&adev->mn_lock);
		return ERR_PTR(-EINTR);
	}

	hash_for_each_possible(adev->mn_hash, amn, node, key)
		if (GSGPU_MN_KEY(amn->mm, amn->type) == key)
			goto release_locks;

	amn = kzalloc(sizeof(*amn), GFP_KERNEL);
	if (!amn) {
		amn = ERR_PTR(-ENOMEM);
		goto release_locks;
	}

	amn->adev = adev;
	amn->mm = mm;
	init_rwsem(&amn->lock);
	amn->type = type;
	amn->mn.ops = &gsgpu_mn_ops[type];
	amn->objects = RB_ROOT_CACHED;
	mutex_init(&amn->read_lock);
	atomic_set(&amn->recursion, 0);

	r = __mmu_notifier_register(&amn->mn, mm);
	if (r)
		goto free_amn;

	hash_add(adev->mn_hash, &amn->node, GSGPU_MN_KEY(mm, type));

release_locks:
	up_write(&mm->mmap_sem);
	mutex_unlock(&adev->mn_lock);

	return amn;

free_amn:
	up_write(&mm->mmap_sem);
	mutex_unlock(&adev->mn_lock);
	kfree(amn);

	return ERR_PTR(r);
}

/**
 * gsgpu_mn_register - register a BO for notifier updates
 *
 * @bo: gsgpu buffer object
 * @addr: userptr addr we should monitor
 *
 * Registers an MMU notifier for the given BO at the specified address.
 * Returns 0 on success, -ERRNO if anything goes wrong.
 */
int gsgpu_mn_register(struct gsgpu_bo *bo, unsigned long addr)
{
	unsigned long end = addr + gsgpu_bo_size(bo) - 1;
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	enum gsgpu_mn_type type = GSGPU_MN_TYPE_GFX;
	struct gsgpu_mn *amn;
	struct gsgpu_mn_node *node = NULL, *new_node;
	struct list_head bos;
	struct interval_tree_node *it;

	amn = gsgpu_mn_get(adev, type);
	if (IS_ERR(amn))
		return PTR_ERR(amn);

	new_node = kmalloc(sizeof(*new_node), GFP_KERNEL);
	if (!new_node)
		return -ENOMEM;

	INIT_LIST_HEAD(&bos);

	down_write(&amn->lock);

	while ((it = interval_tree_iter_first(&amn->objects, addr, end))) {
		kfree(node);
		node = container_of(it, struct gsgpu_mn_node, it);
		interval_tree_remove(&node->it, &amn->objects);
		addr = min(it->start, addr);
		end = max(it->last, end);
		list_splice(&node->bos, &bos);
	}

	if (!node)
		node = new_node;
	else
		kfree(new_node);

	bo->mn = amn;

	node->it.start = addr;
	node->it.last = end;
	INIT_LIST_HEAD(&node->bos);
	list_splice(&bos, &node->bos);
	list_add(&bo->mn_list, &node->bos);

	interval_tree_insert(&node->it, &amn->objects);

	up_write(&amn->lock);

	return 0;
}

/**
 * gsgpu_mn_unregister - unregister a BO for notifier updates
 *
 * @bo: gsgpu buffer object
 *
 * Remove any registration of MMU notifier updates from the buffer object.
 */
void gsgpu_mn_unregister(struct gsgpu_bo *bo)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	struct gsgpu_mn *amn;
	struct list_head *head;

	mutex_lock(&adev->mn_lock);

	amn = bo->mn;
	if (amn == NULL) {
		mutex_unlock(&adev->mn_lock);
		return;
	}

	down_write(&amn->lock);

	/* save the next list entry for later */
	head = bo->mn_list.next;

	bo->mn = NULL;
	list_del_init(&bo->mn_list);

	if (list_empty(head)) {
		struct gsgpu_mn_node *node;

		node = container_of(head, struct gsgpu_mn_node, bos);
		interval_tree_remove(&node->it, &amn->objects);
		kfree(node);
	}

	up_write(&amn->lock);
	mutex_unlock(&adev->mn_lock);
}


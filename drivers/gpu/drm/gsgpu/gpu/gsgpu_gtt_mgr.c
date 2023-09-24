#include <drm/drmP.h>
#include "gsgpu.h"

struct gsgpu_gtt_mgr {
	struct drm_mm mm;
	spinlock_t lock;
	atomic64_t available;
};

struct gsgpu_gtt_node {
	struct drm_mm_node node;
	struct ttm_buffer_object *tbo;
};

/**
 * gsgpu_gtt_mgr_init - init GTT manager and DRM MM
 *
 * @man: TTM memory type manager
 * @p_size: maximum size of GTT
 *
 * Allocate and initialize the GTT manager.
 */
static int gsgpu_gtt_mgr_init(struct ttm_resource_manager *man,
			       unsigned long p_size)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(man->bdev);
	struct gsgpu_gtt_mgr *mgr;
	uint64_t start, size;

	mgr = kzalloc(sizeof(*mgr), GFP_KERNEL);
	if (!mgr)
		return -ENOMEM;

	start = GSGPU_GTT_MAX_TRANSFER_SIZE * GSGPU_GTT_NUM_TRANSFER_WINDOWS;
	size = (adev->gmc.gart_size >> PAGE_SHIFT) - start;
	drm_mm_init(&mgr->mm, start, size);
	spin_lock_init(&mgr->lock);
	atomic64_set(&mgr->available, p_size);
	man->priv = mgr;
	return 0;
}

/**
 * gsgpu_gtt_mgr_fini - free and destroy GTT manager
 *
 * @man: TTM memory type manager
 *
 * Destroy and free the GTT manager, returns -EBUSY if ranges are still
 * allocated inside it.
 */
static int gsgpu_gtt_mgr_fini(struct ttm_resource_manager *man)
{
	struct gsgpu_gtt_mgr *mgr = man->priv;
	spin_lock(&mgr->lock);
	drm_mm_takedown(&mgr->mm);
	spin_unlock(&mgr->lock);
	kfree(mgr);
	man->priv = NULL;
	return 0;
}

/**
 * gsgpu_gtt_mgr_has_gart_addr - Check if mem has address space
 *
 * @mem: the mem object to check
 *
 * Check if a mem object has already address space allocated.
 */
bool gsgpu_gtt_mgr_has_gart_addr(struct ttm_resource *mem)
{
	struct gsgpu_gtt_node *node = mem->mm_node;

	return (node->node.start != GSGPU_BO_INVALID_OFFSET);
}

/**
 * gsgpu_gtt_mgr_alloc - allocate new ranges
 *
 * @man: TTM memory type manager
 * @tbo: TTM BO we need this range for
 * @place: placement flags and restrictions
 * @mem: the resulting mem object
 *
 * Allocate the address space for a node.
 */
static int gsgpu_gtt_mgr_alloc(struct ttm_resource_manager *man,
				struct ttm_buffer_object *tbo,
				const struct ttm_place *place,
				struct ttm_resource *mem)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(man->bdev);
	struct gsgpu_gtt_mgr *mgr = man->priv;
	struct gsgpu_gtt_node *node = mem->mm_node;
	enum drm_mm_insert_mode mode;
	unsigned long fpfn, lpfn;
	int r;

	if (gsgpu_gtt_mgr_has_gart_addr(mem))
		return 0;

	if (place)
		fpfn = place->fpfn;
	else
		fpfn = 0;

	if (place && place->lpfn)
		lpfn = place->lpfn;
	else
		lpfn = adev->gart.num_cpu_pages;

	mode = DRM_MM_INSERT_BEST;
	if (place && place->flags & TTM_PL_FLAG_TOPDOWN)
		mode = DRM_MM_INSERT_HIGH;

	spin_lock(&mgr->lock);
	r = drm_mm_insert_node_in_range(&mgr->mm, &node->node, mem->num_pages,
					mem->page_alignment, 0, fpfn, lpfn,
					mode);
	spin_unlock(&mgr->lock);

	if (!r)
		mem->start = node->node.start;

	return r;
}

/**
 * gsgpu_gtt_mgr_new - allocate a new node
 *
 * @man: TTM memory type manager
 * @tbo: TTM BO we need this range for
 * @place: placement flags and restrictions
 * @mem: the resulting mem object
 *
 * Dummy, allocate the node but no space for it yet.
 */
static int gsgpu_gtt_mgr_new(struct ttm_resource_manager *man,
			      struct ttm_buffer_object *tbo,
			      const struct ttm_place *place,
			      struct ttm_resource *mem)
{
	struct gsgpu_gtt_mgr *mgr = man->priv;
	struct gsgpu_gtt_node *node;
	int r;

	spin_lock(&mgr->lock);
	if ((&tbo->mem == mem || tbo->mem.mem_type != TTM_PL_TT) &&
	    atomic64_read(&mgr->available) < mem->num_pages) {
		spin_unlock(&mgr->lock);
		return 0;
	}
	atomic64_sub(mem->num_pages, &mgr->available);
	spin_unlock(&mgr->lock);

	node = kzalloc(sizeof(*node), GFP_KERNEL);
	if (!node) {
		r = -ENOMEM;
		goto err_out;
	}

	node->node.start = GSGPU_BO_INVALID_OFFSET;
	node->node.size = mem->num_pages;
	node->tbo = tbo;
	mem->mm_node = node;

	if (place->fpfn || place->lpfn || place->flags & TTM_PL_FLAG_TOPDOWN) {
		r = gsgpu_gtt_mgr_alloc(man, tbo, place, mem);
		if (unlikely(r)) {
			kfree(node);
			mem->mm_node = NULL;
			r = 0;
			goto err_out;
		}
	} else {
		mem->start = node->node.start;
	}

	return 0;
err_out:
	atomic64_add(mem->num_pages, &mgr->available);

	return r;
}

/**
 * gsgpu_gtt_mgr_del - free ranges
 *
 * @man: TTM memory type manager
 * @tbo: TTM BO we need this range for
 * @place: placement flags and restrictions
 * @mem: TTM memory object
 *
 * Free the allocated GTT again.
 */
static void gsgpu_gtt_mgr_del(struct ttm_resource_manager *man,
			       struct ttm_resource *mem)
{
	struct gsgpu_gtt_mgr *mgr = man->priv;
	struct gsgpu_gtt_node *node = mem->mm_node;

	if (!node)
		return;

	spin_lock(&mgr->lock);
	if (node->node.start != GSGPU_BO_INVALID_OFFSET)
		drm_mm_remove_node(&node->node);
	spin_unlock(&mgr->lock);
	atomic64_add(mem->num_pages, &mgr->available);

	kfree(node);
	mem->mm_node = NULL;
}

/**
 * gsgpu_gtt_mgr_usage - return usage of GTT domain
 *
 * @man: TTM memory type manager
 *
 * Return how many bytes are used in the GTT domain
 */
uint64_t gsgpu_gtt_mgr_usage(struct ttm_resource_manager *man)
{
	struct gsgpu_gtt_mgr *mgr = man->priv;
	s64 result = man->size - atomic64_read(&mgr->available);

	return (result > 0 ? result : 0) * PAGE_SIZE;
}

int gsgpu_gtt_mgr_recover(struct ttm_resource_manager *man)
{
	struct gsgpu_gtt_mgr *mgr = man->priv;
	struct gsgpu_gtt_node *node;
	struct drm_mm_node *mm_node;
	int r = 0;

	spin_lock(&mgr->lock);
	drm_mm_for_each_node(mm_node, &mgr->mm) {
		node = container_of(mm_node, struct gsgpu_gtt_node, node);
		r = gsgpu_ttm_recover_gart(node->tbo);
		if (r)
			break;
	}
	spin_unlock(&mgr->lock);

	return r;
}

/**
 * gsgpu_gtt_mgr_debug - dump VRAM table
 *
 * @man: TTM memory type manager
 * @printer: DRM printer to use
 *
 * Dump the table content using printk.
 */
static void gsgpu_gtt_mgr_debug(struct ttm_resource_manager *man,
				 struct drm_printer *printer)
{
	struct gsgpu_gtt_mgr *mgr = man->priv;

	spin_lock(&mgr->lock);
	drm_mm_print(&mgr->mm, printer);
	spin_unlock(&mgr->lock);

	drm_printf(printer, "man size:%llu pages, gtt available:%lld pages, usage:%lluMB\n",
		   man->size, (u64)atomic64_read(&mgr->available),
		   gsgpu_gtt_mgr_usage(man) >> 20);
}

const struct ttm_resource_manager_func gsgpu_gtt_mgr_func = {
	.init = gsgpu_gtt_mgr_init,
	.takedown = gsgpu_gtt_mgr_fini,
	.get_node = gsgpu_gtt_mgr_new,
	.put_node = gsgpu_gtt_mgr_del,
	.debug = gsgpu_gtt_mgr_debug
};

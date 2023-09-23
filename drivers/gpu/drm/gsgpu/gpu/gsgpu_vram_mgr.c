#include <drm/drmP.h>
#include "gsgpu.h"

struct gsgpu_vram_mgr {
	struct drm_mm mm;
	spinlock_t lock;
	atomic64_t usage;
	atomic64_t vis_usage;
};

/**
 * gsgpu_vram_mgr_init - init VRAM manager and DRM MM
 *
 * @man: TTM memory type manager
 * @p_size: maximum size of VRAM
 *
 * Allocate and initialize the VRAM manager.
 */
static int gsgpu_vram_mgr_init(struct ttm_resource_manager *man,
				unsigned long p_size)
{
	struct gsgpu_vram_mgr *mgr;

	mgr = kzalloc(sizeof(*mgr), GFP_KERNEL);
	if (!mgr)
		return -ENOMEM;

	drm_mm_init(&mgr->mm, 0, p_size);
	spin_lock_init(&mgr->lock);
	man->priv = mgr;
	return 0;
}

/**
 * gsgpu_vram_mgr_fini - free and destroy VRAM manager
 *
 * @man: TTM memory type manager
 *
 * Destroy and free the VRAM manager, returns -EBUSY if ranges are still
 * allocated inside it.
 */
static int gsgpu_vram_mgr_fini(struct ttm_resource_manager *man)
{
	struct gsgpu_vram_mgr *mgr = man->priv;

	spin_lock(&mgr->lock);
	drm_mm_takedown(&mgr->mm);
	spin_unlock(&mgr->lock);
	kfree(mgr);
	man->priv = NULL;
	return 0;
}

/**
 * gsgpu_vram_mgr_vis_size - Calculate visible node size
 *
 * @adev: gsgpu device structure
 * @node: MM node structure
 *
 * Calculate how many bytes of the MM node are inside visible VRAM
 */
static u64 gsgpu_vram_mgr_vis_size(struct gsgpu_device *adev,
				    struct drm_mm_node *node)
{
	uint64_t start = node->start << PAGE_SHIFT;
	uint64_t end = (node->size + node->start) << PAGE_SHIFT;

	if (start >= adev->gmc.visible_vram_size)
		return 0;

	return (end > adev->gmc.visible_vram_size ?
		adev->gmc.visible_vram_size : end) - start;
}

/**
 * gsgpu_vram_mgr_bo_visible_size - CPU visible BO size
 *
 * @bo: &gsgpu_bo buffer object (must be in VRAM)
 *
 * Returns:
 * How much of the given &gsgpu_bo buffer object lies in CPU visible VRAM.
 */
u64 gsgpu_vram_mgr_bo_visible_size(struct gsgpu_bo *bo)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	struct ttm_mem_reg *mem = &bo->tbo.mem;
	struct drm_mm_node *nodes = mem->mm_node;
	unsigned pages = mem->num_pages;
	u64 usage;

	if (gsgpu_gmc_vram_full_visible(&adev->gmc))
		return gsgpu_bo_size(bo);

	if (mem->start >= adev->gmc.visible_vram_size >> PAGE_SHIFT)
		return 0;

	for (usage = 0; nodes && pages; pages -= nodes->size, nodes++)
		usage += gsgpu_vram_mgr_vis_size(adev, nodes);

	return usage;
}

/**
 * gsgpu_vram_mgr_new - allocate new ranges
 *
 * @man: TTM memory type manager
 * @tbo: TTM BO we need this range for
 * @place: placement flags and restrictions
 * @mem: the resulting mem object
 *
 * Allocate VRAM for the given BO.
 */
static int gsgpu_vram_mgr_new(struct ttm_resource_manager *man,
			       struct ttm_buffer_object *tbo,
			       const struct ttm_place *place,
			       struct ttm_mem_reg *mem)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(man->bdev);
	struct gsgpu_vram_mgr *mgr = man->priv;
	struct drm_mm *mm = &mgr->mm;
	struct drm_mm_node *nodes;
	enum drm_mm_insert_mode mode;
	unsigned long lpfn, num_nodes, pages_per_node, pages_left;
	uint64_t usage = 0, vis_usage = 0;
	unsigned i;
	int r;

	lpfn = place->lpfn;
	if (!lpfn)
		lpfn = man->size;

	if (place->flags & TTM_PL_FLAG_CONTIGUOUS ||
	    gsgpu_vram_page_split == -1) {
		pages_per_node = ~0ul;
		num_nodes = 1;
	} else {
		pages_per_node = max((uint32_t)gsgpu_vram_page_split,
				     mem->page_alignment);
		num_nodes = DIV_ROUND_UP(mem->num_pages, pages_per_node);
	}

	nodes = kvmalloc_array(num_nodes, sizeof(*nodes),
			       GFP_KERNEL | __GFP_ZERO);
	if (!nodes)
		return -ENOMEM;

	mode = DRM_MM_INSERT_BEST;
	if (place->flags & TTM_PL_FLAG_TOPDOWN)
		mode = DRM_MM_INSERT_HIGH;

	mem->start = 0;
	pages_left = mem->num_pages;

	spin_lock(&mgr->lock);
	for (i = 0; i < num_nodes; ++i) {
		unsigned long pages = min(pages_left, pages_per_node);
		uint32_t alignment = mem->page_alignment;
		unsigned long start;

		if (pages == pages_per_node)
			alignment = pages_per_node;

		r = drm_mm_insert_node_in_range(mm, &nodes[i],
						pages, alignment, 0,
						place->fpfn, lpfn,
						mode);
		if (unlikely(r))
			goto error;

		usage += nodes[i].size << PAGE_SHIFT;
		vis_usage += gsgpu_vram_mgr_vis_size(adev, &nodes[i]);

		/* Calculate a virtual BO start address to easily check if
		 * everything is CPU accessible.
		 */
		start = nodes[i].start + nodes[i].size;
		if (start > mem->num_pages)
			start -= mem->num_pages;
		else
			start = 0;
		mem->start = max(mem->start, start);
		pages_left -= pages;
	}
	spin_unlock(&mgr->lock);

	atomic64_add(usage, &mgr->usage);
	atomic64_add(vis_usage, &mgr->vis_usage);

	mem->mm_node = nodes;

	return 0;

error:
	while (i--)
		drm_mm_remove_node(&nodes[i]);
	spin_unlock(&mgr->lock);

	kvfree(nodes);
	return r == -ENOSPC ? 0 : r;
}

/**
 * gsgpu_vram_mgr_del - free ranges
 *
 * @man: TTM memory type manager
 * @tbo: TTM BO we need this range for
 * @place: placement flags and restrictions
 * @mem: TTM memory object
 *
 * Free the allocated VRAM again.
 */
static void gsgpu_vram_mgr_del(struct ttm_resource_manager *man,
				struct ttm_mem_reg *mem)
{
	struct gsgpu_device *adev = gsgpu_ttm_adev(man->bdev);
	struct gsgpu_vram_mgr *mgr = man->priv;
	struct drm_mm_node *nodes = mem->mm_node;
	uint64_t usage = 0, vis_usage = 0;
	unsigned pages = mem->num_pages;

	if (!mem->mm_node)
		return;

	spin_lock(&mgr->lock);
	while (pages) {
		pages -= nodes->size;
		drm_mm_remove_node(nodes);
		usage += nodes->size << PAGE_SHIFT;
		vis_usage += gsgpu_vram_mgr_vis_size(adev, nodes);
		++nodes;
	}
	spin_unlock(&mgr->lock);

	atomic64_sub(usage, &mgr->usage);
	atomic64_sub(vis_usage, &mgr->vis_usage);

	kvfree(mem->mm_node);
	mem->mm_node = NULL;
}

/**
 * gsgpu_vram_mgr_usage - how many bytes are used in this domain
 *
 * @man: TTM memory type manager
 *
 * Returns how many bytes are used in this domain.
 */
uint64_t gsgpu_vram_mgr_usage(struct ttm_resource_manager *man)
{
	struct gsgpu_vram_mgr *mgr = man->priv;

	return atomic64_read(&mgr->usage);
}

/**
 * gsgpu_vram_mgr_vis_usage - how many bytes are used in the visible part
 *
 * @man: TTM memory type manager
 *
 * Returns how many bytes are used in the visible part of VRAM
 */
uint64_t gsgpu_vram_mgr_vis_usage(struct ttm_resource_manager *man)
{
	struct gsgpu_vram_mgr *mgr = man->priv;

	return atomic64_read(&mgr->vis_usage);
}

/**
 * gsgpu_vram_mgr_debug - dump VRAM table
 *
 * @man: TTM memory type manager
 * @printer: DRM printer to use
 *
 * Dump the table content using printk.
 */
static void gsgpu_vram_mgr_debug(struct ttm_resource_manager *man,
				  struct drm_printer *printer)
{
	struct gsgpu_vram_mgr *mgr = man->priv;

	spin_lock(&mgr->lock);
	drm_mm_print(&mgr->mm, printer);
	spin_unlock(&mgr->lock);

	drm_printf(printer, "man size:%llu pages, ram usage:%lluMB, vis usage:%lluMB\n",
		   man->size, gsgpu_vram_mgr_usage(man) >> 20,
		   gsgpu_vram_mgr_vis_usage(man) >> 20);
}

const struct ttm_resource_manager_func gsgpu_vram_mgr_func = {
	.init		= gsgpu_vram_mgr_init,
	.takedown	= gsgpu_vram_mgr_fini,
	.get_node	= gsgpu_vram_mgr_new,
	.put_node	= gsgpu_vram_mgr_del,
	.debug		= gsgpu_vram_mgr_debug
};

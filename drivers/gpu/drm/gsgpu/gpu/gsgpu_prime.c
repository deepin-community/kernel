#include "gsgpu.h"
#include "gsgpu_display.h"
#include <drm/gsgpu_drm.h>
#include <linux/dma-buf.h>
#include <linux/dma-fence-array.h>

static const struct dma_buf_ops gsgpu_dmabuf_ops;

/**
 * gsgpu_gem_prime_get_sg_table - &drm_driver.gem_prime_get_sg_table
 * implementation
 * @obj: GEM buffer object
 *
 * Returns:
 * A scatter/gather table for the pinned pages of the buffer object's memory.
 */
struct sg_table *gsgpu_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	struct gsgpu_bo *bo = gem_to_gsgpu_bo(obj);
	int npages = bo->tbo.ttm->num_pages;

	return drm_prime_pages_to_sg(obj->dev, bo->tbo.ttm->pages, npages);
}

/**
 * gsgpu_gem_prime_import_sg_table - &drm_driver.gem_prime_import_sg_table
 * implementation
 * @dev: DRM device
 * @attach: DMA-buf attachment
 * @sg: Scatter/gather table
 *
 * Import shared DMA buffer memory exported by another device.
 *
 * Returns:
 * A new GEM buffer object of the given DRM device, representing the memory
 * described by the given DMA-buf attachment and scatter/gather table.
 */
struct drm_gem_object *
gsgpu_gem_prime_import_sg_table(struct drm_device *dev,
				 struct dma_buf_attachment *attach,
				 struct sg_table *sg)
{
	struct dma_resv *resv = attach->dmabuf->resv;
	struct gsgpu_device *adev = dev->dev_private;
	struct gsgpu_bo *bo;
	struct gsgpu_bo_param bp;
	int ret;

	memset(&bp, 0, sizeof(bp));
	bp.size = attach->dmabuf->size;
	bp.byte_align = PAGE_SIZE;
	bp.domain = GSGPU_GEM_DOMAIN_CPU;
	bp.flags = 0;
	bp.type = ttm_bo_type_sg;
	bp.resv = resv;
	ww_mutex_lock(&resv->lock, NULL);
	ret = gsgpu_bo_create(adev, &bp, &bo);
	if (ret)
		goto error;

	bo->tbo.sg = sg;
	bo->tbo.ttm->sg = sg;
	bo->allowed_domains = GSGPU_GEM_DOMAIN_GTT;
	bo->preferred_domains = GSGPU_GEM_DOMAIN_GTT;
	if (attach->dmabuf->ops != &gsgpu_dmabuf_ops)
		bo->prime_shared_count = 1;

	ww_mutex_unlock(&resv->lock);
	return &bo->gem_base;

error:
	ww_mutex_unlock(&resv->lock);
	return ERR_PTR(ret);
}

static int __dma_resv_make_exclusive(struct dma_resv *obj)
{
	struct dma_fence **fences;
	unsigned int count;
	int r;

	if (!dma_resv_get_list(obj)) /* no shared fences to convert */
		return 0;

	r = dma_resv_get_fences(obj, DMA_RESV_USAGE_READ, &count, &fences);
	if (r)
		return r;

	if (count == 0) {
		/* Now that was unexpected. */
	} else if (count == 1) {
		dma_resv_add_fence(obj, fences[0], DMA_RESV_USAGE_WRITE);
		dma_fence_put(fences[0]);
		kfree(fences);
	} else {
		struct dma_fence_array *array;

		array = dma_fence_array_create(count, fences,
					       dma_fence_context_alloc(1), 0,
					       false);
		if (!array)
			goto err_fences_put;

		dma_resv_add_fence(obj, &array->base, DMA_RESV_USAGE_WRITE);
		dma_fence_put(&array->base);
	}

	return 0;

err_fences_put:
	while (count--)
		dma_fence_put(fences[count]);
	kfree(fences);
	return -ENOMEM;
}

/**
 * gsgpu_gem_map_attach - &dma_buf_ops.attach implementation
 * @dma_buf: shared DMA buffer
 * @attach: DMA-buf attachment
 *
 * Makes sure that the shared DMA buffer can be accessed by the target device.
 * For now, simply pins it to the GTT domain, where it should be accessible by
 * all DMA devices.
 *
 * Returns:
 * 0 on success or negative error code.
 */
static int gsgpu_gem_map_attach(struct dma_buf *dma_buf,
				 struct dma_buf_attachment *attach)
{
	struct drm_gem_object *obj = dma_buf->priv;
	struct gsgpu_bo *bo = gem_to_gsgpu_bo(obj);
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	long r;

	r = drm_gem_map_attach(dma_buf, attach);
	if (r)
		return r;

	r = gsgpu_bo_reserve(bo, false);
	if (unlikely(r != 0))
		goto error_detach;


	if (attach->dev->driver != adev->dev->driver) {
		/*
		 * We only create shared fences for internal use, but importers
		 * of the dmabuf rely on exclusive fences for implicitly
		 * tracking write hazards. As any of the current fences may
		 * correspond to a write, we need to convert all existing
		 * fences on the reservation object into a single exclusive
		 * fence.
		 */
		r = __dma_resv_make_exclusive(bo->tbo.base.resv);
		if (r)
			goto error_unreserve;
	}

	/* pin buffer into GTT */
	r = gsgpu_bo_pin(bo, GSGPU_GEM_DOMAIN_GTT);
	if (r)
		goto error_unreserve;

	if (attach->dev->driver != adev->dev->driver)
		bo->prime_shared_count++;

error_unreserve:
	gsgpu_bo_unreserve(bo);

error_detach:
	if (r)
		drm_gem_map_detach(dma_buf, attach);
	return r;
}

/**
 * gsgpu_gem_map_detach - &dma_buf_ops.detach implementation
 * @dma_buf: shared DMA buffer
 * @attach: DMA-buf attachment
 *
 * This is called when a shared DMA buffer no longer needs to be accessible by
 * the other device. For now, simply unpins the buffer from GTT.
 */
static void gsgpu_gem_map_detach(struct dma_buf *dma_buf,
				  struct dma_buf_attachment *attach)
{
	struct drm_gem_object *obj = dma_buf->priv;
	struct gsgpu_bo *bo = gem_to_gsgpu_bo(obj);
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	int ret = 0;

	ret = gsgpu_bo_reserve(bo, true);
	if (unlikely(ret != 0))
		goto error;

	gsgpu_bo_unpin(bo);
	if (attach->dev->driver != adev->dev->driver && bo->prime_shared_count)
		bo->prime_shared_count--;
	gsgpu_bo_unreserve(bo);

error:
	drm_gem_map_detach(dma_buf, attach);
}

/**
 * gsgpu_gem_begin_cpu_access - &dma_buf_ops.begin_cpu_access implementation
 * @dma_buf: shared DMA buffer
 * @direction: direction of DMA transfer
 *
 * This is called before CPU access to the shared DMA buffer's memory. If it's
 * a read access, the buffer is moved to the GTT domain if possible, for optimal
 * CPU read performance.
 *
 * Returns:
 * 0 on success or negative error code.
 */
static int gsgpu_gem_begin_cpu_access(struct dma_buf *dma_buf,
				       enum dma_data_direction direction)
{
	struct gsgpu_bo *bo = gem_to_gsgpu_bo(dma_buf->priv);
	struct gsgpu_device *adev = gsgpu_ttm_adev(bo->tbo.bdev);
	struct ttm_operation_ctx ctx = { true, false };
	u32 domain = gsgpu_display_supported_domains(adev);
	int ret;
	bool reads = (direction == DMA_BIDIRECTIONAL ||
		      direction == DMA_FROM_DEVICE);

	if (!reads || !(domain & GSGPU_GEM_DOMAIN_GTT))
		return 0;

	/* move to gtt */
	ret = gsgpu_bo_reserve(bo, false);
	if (unlikely(ret != 0))
		return ret;

	if (!bo->pin_count && (bo->allowed_domains & GSGPU_GEM_DOMAIN_GTT)) {
		gsgpu_bo_placement_from_domain(bo, GSGPU_GEM_DOMAIN_GTT);
		ret = ttm_bo_validate(&bo->tbo, &bo->placement, &ctx);
	}

	gsgpu_bo_unreserve(bo);
	return ret;
}

static const struct dma_buf_ops gsgpu_dmabuf_ops = {
	.attach = gsgpu_gem_map_attach,
	.detach = gsgpu_gem_map_detach,
	.map_dma_buf = drm_gem_map_dma_buf,
	.unmap_dma_buf = drm_gem_unmap_dma_buf,
	.release = drm_gem_dmabuf_release,
	.begin_cpu_access = gsgpu_gem_begin_cpu_access,
	.mmap = drm_gem_dmabuf_mmap,
	.vmap = drm_gem_dmabuf_vmap,
	.vunmap = drm_gem_dmabuf_vunmap,
};

/**
 * gsgpu_gem_prime_export - &drm_gem_object_funcs.gem_prime_export implementation
 * @gobj: GEM buffer object
 * @flags: flags like DRM_CLOEXEC and DRM_RDWR
 *
 * The main work is done by the &drm_gem_prime_export helper, which in turn
 * uses &gsgpu_gem_prime_res_obj.
 *
 * Returns:
 * Shared DMA buffer representing the GEM buffer object from the given device.
 */
struct dma_buf *gsgpu_gem_prime_export(struct drm_gem_object *gobj,
				       int flags)
{
	struct gsgpu_bo *bo = gem_to_gsgpu_bo(gobj);
	struct dma_buf *buf;

	if (gsgpu_ttm_tt_get_usermm(bo->tbo.ttm) ||
	    bo->flags & GSGPU_GEM_CREATE_VM_ALWAYS_VALID)
		return ERR_PTR(-EPERM);

	buf = drm_gem_prime_export(gobj, flags);
	if (!IS_ERR(buf)) {
		buf->file->f_mapping = gobj->dev->anon_inode->i_mapping;
		buf->ops = &gsgpu_dmabuf_ops;
	}

	return buf;
}

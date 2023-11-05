#include "gsgpu.h"
#include <linux/dma-buf.h>

/*
 * DMA-BUF
 *
 * The Loongson 7A2000 bridge chip disables the builtin GSGPU display
 * output if it detects a discrete graphics card. Therefore it is not
 * physically possible to render a buffer from the discrete graphics
 * card into a display connected to the integrated GSGPU DC (although
 * the integrated GPU might very well be able to see the foreign buffer,
 * its DC would be disabled, so nothing will show up on-screen). We
 * therefore provide an empty implementation here, where we return ENOSYS
 * for all import/export requests for foreign buffer objects. Self-
 * importing is handled transparently by the DRM subsystem and works
 * automatically (self-importing is needed for newer userspace).
 */

/**
 * gsgpu_dma_buf_pin - &dma_buf_ops.pin implementation
 *
 * @attach: attachment to pin down
 *
 * Pin the BO which is backing the DMA-buf so that it can't move any more.
 */
static int gsgpu_dma_buf_pin(struct dma_buf_attachment *attach)
{
	struct drm_gem_object *obj = attach->dmabuf->priv;
	struct gsgpu_bo *bo = gem_to_gsgpu_bo(obj);

	/* pin buffer into GTT */
	return gsgpu_bo_pin(bo, GSGPU_GEM_DOMAIN_GTT);
}

/**
 * gsgpu_dma_buf_unpin - &dma_buf_ops.unpin implementation
 *
 * @attach: attachment to unpin
 *
 * Unpin a previously pinned BO to make it movable again.
 */
static void gsgpu_dma_buf_unpin(struct dma_buf_attachment *attach)
{
	struct drm_gem_object *obj = attach->dmabuf->priv;
	struct gsgpu_bo *bo = gem_to_gsgpu_bo(obj);

	gsgpu_bo_unpin(bo);
}

struct sg_table *gsgpu_gem_prime_get_sg_table(struct drm_gem_object *obj)
{
	return ERR_PTR(-ENOSYS);
}

struct drm_gem_object *gsgpu_gem_prime_import_sg_table(struct drm_device *dev,
						       struct dma_buf_attachment *attach,
						       struct sg_table *table)
{
	return ERR_PTR(-ENOSYS);
}

const struct dma_buf_ops gsgpu_dmabuf_ops = {
	.pin = gsgpu_dma_buf_pin,
	.unpin = gsgpu_dma_buf_unpin,
	.release = drm_gem_dmabuf_release,
	.mmap = drm_gem_dmabuf_mmap,
	.vmap = drm_gem_dmabuf_vmap,
	.vunmap = drm_gem_dmabuf_vunmap,
};

/*
 * Copyright 2016 SiliconMotion Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License version 2. See the file COPYING in the main
 * directory of this archive for more details.
 *
 */
#include "smi_drv.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#include <drm/drm_pci.h>
#else
#include <linux/pci.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
#include <drm/drm_irq.h>
#endif

#include <drm/drm_vblank.h>
#else
#include <drm/drmP.h>
#endif
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fourcc.h>
#include <linux/dma-buf.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
#include <drm/drm_probe_helper.h>
#endif

#include "hw750.h"
#include "hw768.h"
#include "smi_dbg.h"

#define MB(x) (x << 20) /* Macro for Mega Bytes */


static int smi_user_framebuffer_dirty(struct drm_framebuffer *fb, struct drm_file *file,
				      unsigned flags, unsigned color, struct drm_clip_rect *clips,
				      unsigned num_clips);


static struct drm_framebuffer *smi_user_framebuffer_create(struct drm_device *dev,
							   struct drm_file *filp,
							   const struct drm_mode_fb_cmd2 *mode_cmd);

static const struct drm_framebuffer_funcs smi_fb_funcs = {
	.create_handle = drm_gem_fb_create_handle,
	.destroy = drm_gem_fb_destroy,
	.dirty = smi_user_framebuffer_dirty,

};

static const struct drm_mode_config_helper_funcs smi_mode_config_helper_funcs = {
	.atomic_commit_tail = drm_atomic_helper_commit_tail,
};

static const struct drm_mode_config_funcs smi_mode_config_funcs = {
	.fb_create = smi_user_framebuffer_create,
	.output_poll_changed = drm_fb_helper_output_poll_changed,
	.atomic_check = drm_atomic_helper_check,
	.atomic_commit = drm_atomic_helper_commit,
};

static bool smi_merge_clips(struct drm_clip_rect *dst, struct drm_clip_rect *src,
			    unsigned int num_clips, unsigned int flags, u32 max_width,
			    u32 max_height)
{
	unsigned int i;

	if (!src || !num_clips) {
		dst->x1 = 0;
		dst->x2 = max_width;
		dst->y1 = 0;
		dst->y2 = max_height;
		return true;
	}

	dst->x1 = ~0;
	dst->y1 = ~0;
	dst->x2 = 0;
	dst->y2 = 0;

	for (i = 0; i < num_clips; i++) {
		if (flags & DRM_MODE_FB_DIRTY_ANNOTATE_COPY)
			i++;
		dst->x1 = min(dst->x1, src[i].x1);
		dst->x2 = max(dst->x2, src[i].x2);
		dst->y1 = min(dst->y1, src[i].y1);
		dst->y2 = max(dst->y2, src[i].y2);
	}

	if (dst->x2 > max_width || dst->y2 > max_height || dst->x1 >= dst->x2 ||
	    dst->y1 >= dst->y2) {
		DRM_DEBUG_KMS("Illegal clip: x1 = %u, x2 = %u, y1 = %u, y2 = %u\n", dst->x1,
			      dst->x2, dst->y1, dst->y2);
		dst->x1 = 0;
		dst->x2 = max_width;
		dst->y1 = 0;
		dst->y2 = max_height;
	}

	return (dst->x2 - dst->x1) == max_width && (dst->y2 - dst->y1) == max_height;
}

static int smi_handle_damage(struct drm_framebuffer *fb, struct drm_clip_rect clip)
{
	bool kmap = false;
	int i, ret = 0;
	unsigned long offset = 0;
	void *dst = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	struct drm_gem_vram_object *gbo;
#else
	struct smi_bo *dst_bo = NULL;
#endif
	struct drm_gem_object *obj = fb->obj[0];
	void *src = NULL;
	unsigned bytesPerPixel = fb->format->cpp[0];

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
	struct dma_buf_map src_map,dst_map;
#endif

	if (!obj->import_attach) {
		return (-EINVAL);
	}

	if (!src) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
		ret = dma_buf_vmap(obj->import_attach->dmabuf, &src_map);
		if (ret) {
				DRM_ERROR("Failed to vmap src buffer\n");
				return (0);
		}
		src = src_map.vaddr;
#else
		src = dma_buf_vmap(obj->import_attach->dmabuf);
		if (!src)
			return (0);
#endif

	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	gbo = drm_gem_vram_of_gem(obj);
	ret = drm_gem_vram_pin(gbo, DRM_GEM_VRAM_PL_FLAG_VRAM);
	if (ret)
		return (0);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
	ret = drm_gem_vram_vmap(gbo, &dst_map);
				
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
	dst = drm_gem_vram_vmap(gbo);
#else
	dst = drm_gem_vram_kmap(gbo, false, NULL);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
	if (ret) {
		DRM_ERROR("failed to map vram\n");
		goto error;
	}
	dst = dst_map.vaddr;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	if (IS_ERR(dst)) {

		DRM_ERROR("failed to map vram\n");
		goto error;
	} else if (!dst) {
		dst = drm_gem_vram_vmap(gbo);
		if (IS_ERR(dst)) {
			DRM_ERROR("failed to kmap vram\n");
			goto error;
		}
	}        
#else
	if (IS_ERR(dst)) {

		DRM_ERROR("failed to map vram\n");
		goto error;
	} else if (!dst) {
		dst = drm_gem_vram_kmap(gbo, true, NULL);
		if (IS_ERR(dst)) {
			DRM_ERROR("failed to kmap vram\n");
			goto error;
		}
		kmap = true;
	}
#endif



#else
	dst_bo = gem_to_smi_bo(obj);
	if (!dst_bo->pin_count) {
		ret = smi_bo_pin(dst_bo, TTM_PL_FLAG_VRAM, NULL);
		if (ret)
			return (0);
	}

	dst = smi_bo_kmap(dst_bo, false, NULL);
	if (IS_ERR(dst)) {
		DRM_ERROR("failed to kmap vram\n");
		goto error;
	} else if (!dst) {
		dst = smi_bo_kmap(dst_bo, true, NULL);
		if (IS_ERR(dst)) {
			DRM_ERROR("failed to kmap vram\n");
			goto error;
		}
		kmap = true;
	}
#endif

	dbg_msg("src: %p, dst: %p, x=%d, y=%d, fbwidth=%d, fbheight=%d, "
		"width=%d, height=%d, bpp = %u, pitch=%d\n",
		src, dst, clip.x1, clip.y1, fb->width, fb->height, clip.x2 - clip.x1,
		clip.y2 - clip.y1, (bytesPerPixel << 3), fb->pitches[0]);

	for (i = clip.y1; i < clip.y2; i++) {
		offset = i * fb->pitches[0] + (clip.x1 * bytesPerPixel);
		memcpy_toio(dst + offset, src + offset, (clip.x2 - clip.x1) * bytesPerPixel);
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	if (kmap)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
		drm_gem_vram_vunmap(gbo, dst);
#else	
		drm_gem_vram_kunmap(gbo);
#endif
#else
	if (kmap)
		smi_bo_kunmap(dst_bo);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)	
	dma_buf_vunmap(obj->import_attach->dmabuf, &src_map);
#else
    dma_buf_vunmap(obj->import_attach->dmabuf, src);
#endif

error:
	return (0);
}

static int smi_user_framebuffer_dirty(struct drm_framebuffer *fb, struct drm_file *file,
				      unsigned flags, unsigned color, struct drm_clip_rect *clips,
				      unsigned num_clips)
{
	struct drm_gem_object *obj = fb->obj[0];
	struct drm_clip_rect clip;

	drm_modeset_lock_all(fb->dev);

	if (obj->import_attach) {
		int ret = dma_buf_begin_cpu_access(obj->import_attach->dmabuf, DMA_FROM_DEVICE);
		if (ret)
			goto unlock;
	}

	smi_merge_clips(&clip, clips, num_clips, flags, fb->width, fb->height);
	smi_handle_damage(fb, clip);

	if (obj->import_attach) {
		dma_buf_end_cpu_access(obj->import_attach->dmabuf, DMA_FROM_DEVICE);
	}

unlock:
	drm_modeset_unlock_all(fb->dev);

	return 0;
}


static struct drm_framebuffer *smi_user_framebuffer_create(struct drm_device *dev,
							   struct drm_file *filp,
							   const struct drm_mode_fb_cmd2 *mode_cmd)
{
	return (drm_gem_fb_create_with_funcs(dev, filp, mode_cmd, &smi_fb_funcs));
}

/*
 * Functions here will be called by the core once it's bound the driver to
 * a PCI device
 */
int smi_driver_load(struct drm_device *dev, unsigned long flags)
{
	struct smi_device *cdev;
	struct pci_dev *pdev; 
	int r;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	pdev = to_pci_dev(dev->dev);
#else
	pdev = dev->pdev;
#endif

	cdev = kzalloc(sizeof(struct smi_device), GFP_KERNEL);
	if (cdev == NULL)
		return -ENOMEM;
	dev->dev_private = (void *)cdev;

	switch (pdev->device) {
	case PCI_DEVID_LYNX_EXP:
		g_specId = SPC_SM750;
		break;
	case PCI_DEVID_SM768:
		g_specId = SPC_SM768;
		break;
	default:
		return -ENODEV;
	}

	r = pci_enable_device(pdev);

	r = smi_device_init(cdev, dev, pdev, flags);
	if (r) {
		dev_err(&pdev->dev, "Fatal error during GPU init: %d\n", r);
		goto out;
	}
	if(g_specId == SPC_SM750)
	{
	    if (pdev->resource[PCI_ROM_RESOURCE].flags & IORESOURCE_ROM_SHADOW) {
			cdev->is_boot_gpu = true;
		}
		ddk750_initChip();
		ddk750_deInit();
		
#ifdef USE_HDMICHIP
		if((r = sii9022xInitChip()) < 0)
		{	
			printk("Init HDMI-Tx chip failed!");
			r = 0;	
		}
#endif
#ifdef USE_EP952
		EP_HDMI_Init(0);
		EP_HDMI_Set_Video_Timing(1,0);
#endif

	}
	else
	{
		ddk768_initChip();
		ddk768_deInit();
		hw768_init_hdmi();
#ifdef USE_EP952
		EP_HDMI_Init(1);
		EP_HDMI_Set_Video_Timing(1,1);
#endif
		
		if(audio_en)
			smi_audio_init(dev);

	}	
	


	r = smi_mm_init(cdev);
	if (r) {
		dev_err(&pdev->dev, "fatal err on mm init\n");
		goto out;
	}

	drm_vblank_init(dev, dev->mode_config.num_crtc);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	r = drm_irq_install(dev, pdev->irq);
#else
	r = request_irq(pdev->irq, smi_drm_interrupt, IRQF_SHARED,
				  KBUILD_MODNAME, dev);
#endif
	if (r)
		DRM_ERROR("install irq failed , ret = %d\n", r);

	dev->mode_config.funcs = (void *)&smi_mode_config_funcs;
	r = smi_modeset_init(cdev);
	if (r) {
		DRM_ERROR("Fatal error during modeset init: %d\n", r);
		goto out;
	}

	cdev->regsave = vmalloc(1024);
	if (!cdev->regsave) {
		DRM_ERROR("cannot allocate regsave\n");
		//return -ENOMEM;
	}

	drm_kms_helper_poll_init(dev);

	return 0;
out:
	if (r)
		smi_driver_unload(dev);
	return r;
}

void smi_driver_unload(struct drm_device *dev)
{
	struct smi_device *cdev = dev->dev_private;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	struct pci_dev *pdev = to_pci_dev(dev->dev);
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	if (dev->irq_enabled)
		drm_irq_uninstall(dev);
#else
	free_irq(pdev->irq, dev);
	/* Disable *all* interrupts */
	if (g_specId == SPC_SM750) {
		ddk750_disable_IntMask();
	} else if (g_specId == SPC_SM768) {
		ddk768_disable_IntMask();
	}
#endif

	if (cdev == NULL)
		return;

	smi_modeset_fini(cdev);
	smi_mm_fini(cdev);
	smi_device_fini(cdev);

	if(g_specId == SPC_SM768)
	{
		if(audio_en)
			smi_audio_remove(dev);

    }


	vfree(cdev->regsave);
	kfree(cdev);
	dev->dev_private = NULL;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
int smi_gem_create(struct drm_device *dev, u32 size, bool iskernel, struct drm_gem_object **obj)
{
	struct smi_bo *smibo;
	int ret;

	*obj = NULL;

	size = roundup(size, PAGE_SIZE);
	if (size == 0)
		return (-EINVAL);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
	ret = smi_bo_create(dev, size, 0, 0, NULL, NULL, &smibo);
#endif

	if (ret) {
		if (ret != -ERESTARTSYS)
			DRM_ERROR("failed to allocate GEM object\n");
		return (ret);
	}

	*obj = &smibo->gem;
	return (0);
}

int smi_dumb_create(struct drm_file *file, struct drm_device *dev,
		    struct drm_mode_create_dumb *args)
{
	int ret;
	struct drm_gem_object *gobj;
	u32 handle;

	args->pitch = ALIGN(DIV_ROUND_UP(args->width * args->bpp, 8), 16);

	args->size = roundup(args->pitch * args->height, PAGE_SIZE);

	ret = smi_gem_create(dev, args->size, false, &gobj);
	if (ret)
		return (ret);

	ret = drm_gem_handle_create(file, gobj, &handle);
	drm_gem_object_put_unlocked(gobj);
	if (ret)
		return (ret);

	args->handle = handle;
	return (0);
}

static inline u64 smi_bo_mmap_offset(struct smi_bo *bo)
{
	return drm_vma_node_offset_addr(&bo->bo.vma_node);
}

int smi_dumb_mmap_offset(struct drm_file *file, struct drm_device *dev, uint32_t handle,
			 uint64_t *offset)
{
	struct drm_gem_object *obj;
	struct smi_bo *bo;
	obj = drm_gem_object_lookup(file, handle);
	if (obj == NULL)
		return -ENOENT;

	bo = gem_to_smi_bo(obj);
	*offset = smi_bo_mmap_offset(bo);

	drm_gem_object_put_unlocked(obj);
	return 0;
}

void smi_bo_unref(struct smi_bo **bo)
{
	struct ttm_buffer_object *tbo;

	if ((*bo) == NULL)
		return;

	tbo = &((*bo)->bo);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
	ttm_bo_put(tbo);
#else
	ttm_bo_unref(&tbo);
	if (tbo == NULL)
#endif
	{
		*bo = NULL;
	}
}

void smi_gem_free_object(struct drm_gem_object *obj)
{
	struct smi_bo *smi_bo = gem_to_smi_bo(obj);

	if (smi_bo) {
		if (smi_bo->gem.import_attach)
			drm_prime_gem_destroy(&smi_bo->gem, smi_bo->bo.sg);
		smi_bo_unref(&smi_bo);
	}
}
#endif

/* Unmap the framebuffer from the core and release the memory */
static void smi_vram_fini(struct smi_device *cdev)
{
	iounmap(cdev->rmmio);
	cdev->rmmio = NULL;

}

/* Map the framebuffer from the card and configure the core */
static int smi_vram_init(struct smi_device *cdev)
{

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	struct pci_dev *pdev = to_pci_dev(cdev->dev->dev);
	/* BAR 0 is VRAM */
	cdev->mc.vram_base = pci_resource_start(pdev, 0);
#else	
	/* BAR 0 is VRAM */
	cdev->mc.vram_base = pci_resource_start(cdev->dev->pdev, 0);
#endif

	/* VRAM Size */
	if (g_specId == SPC_SM750)
		cdev->mc.vram_size = ddk750_getFrameBufSize();
	else
		cdev->mc.vram_size = ddk768_getFrameBufSize();

	return 0;
}

/*
 * SMI Graphics has two sets of memory. One is video RAM and can
 * simply be used as a linear framebuffer - the other provides mmio access
 * to the display registers. The latter can also be accessed via IO port
 * access, but we map the range and use mmio to program them instead
 */

int smi_device_init(struct smi_device *cdev, struct drm_device *ddev, struct pci_dev *pdev,
		    uint32_t flags)
{
	int ret, dma_bits;

	cdev->dev = ddev;
	cdev->flags = flags;

	/* Hardcode the number of CRTCs to 2 */
	cdev->num_crtc = 2;

	dma_bits = 40;
	cdev->need_dma32 = false;
	ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(dma_bits));
	if (ret) {
		cdev->need_dma32 = true;
		dma_bits = 32;
		printk(KERN_WARNING "smifb: No suitable DMA available.\n");
	}

#if 0
	ret = pci_set_consistent_dma_mask(cdev->dev->pdev, DMA_BIT_MASK(dma_bits));
	if (ret) {
		pci_set_consistent_dma_mask(cdev->dev->pdev, DMA_BIT_MASK(32));
		printk(KERN_WARNING "smifb: No coherent DMA available.\n");
	}
#endif

	/* BAR 0 is the framebuffer, BAR 1 contains registers */
	cdev->rmmio_base = pci_resource_start(pdev, 1);
	cdev->rmmio_size = pci_resource_len(pdev, 1);
	cdev->rmmio = ioremap(cdev->rmmio_base, cdev->rmmio_size);

	if (cdev->rmmio == NULL)
		return -ENOMEM;

	if (g_specId == SPC_SM750)
		ddk750_set_mmio(cdev->rmmio, pdev->device, pdev->revision);
	else
		ddk768_set_mmio(cdev->rmmio, pdev->device, pdev->revision);

	ret = smi_vram_init(cdev);
	if (ret) {
		return ret;
	}

	return 0;
}

void smi_device_fini(struct smi_device *cdev)
{
	smi_vram_fini(cdev);
}

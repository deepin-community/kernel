// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.

#include "smi_drv.h"
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#include <drm/drm_pci.h>
#else
#include <linux/pci.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
#include <drm/drm_irq.h>
#endif

#include <drm/drm_vblank.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_fourcc.h>
#include <linux/dma-buf.h>
#include <drm/drm_probe_helper.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
#include <drm/drm_framebuffer.h>
#endif

#include "hw750.h"
#include "hw768.h"
#include "hw770.h"
#include "smi_dbg.h"

static const struct drm_framebuffer_funcs smi_fb_funcs = {
	.create_handle = drm_gem_fb_create_handle,
	.destroy = drm_gem_fb_destroy,
};

static const struct drm_mode_config_helper_funcs smi_mode_config_helper_funcs = {
	.atomic_commit_tail = drm_atomic_helper_commit_tail,
};

static const struct drm_mode_config_funcs smi_mode_config_funcs = {
    .fb_create = drm_gem_fb_create_with_dirty,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 12, 0)
    .output_poll_changed = drm_fb_helper_output_poll_changed,
#endif
    .atomic_check = drm_atomic_helper_check,
    .atomic_commit = drm_atomic_helper_commit,
};


/*
 * Functions here will be called by the core once it's bound the driver to
 * a PCI device
 */
int smi_driver_load(struct drm_device *dev, unsigned long flags)
{
	struct smi_device *cdev;
	struct pci_dev *pdev; 
	int r;
	
	pdev = to_pci_dev(dev->dev);
	cdev = kzalloc(sizeof(struct smi_device), GFP_KERNEL);
	if (cdev == NULL)
		return -ENOMEM;
	dev->dev_private = (void *)cdev;

	switch (pdev->device) {
	case PCI_DEVID_LYNX_EXP:
		cdev->specId = SPC_SM750;
		break;
	case PCI_DEVID_SM768:
		cdev->specId = SPC_SM768;
		break;
	case PCI_DEVID_SM770:
		cdev->specId = SPC_SM770;
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
	if(cdev->specId == SPC_SM750)
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
		setDisplayControl(CHANNEL0_CTRL, DISP_OFF);
		setDisplayControl(CHANNEL1_CTRL, DISP_OFF);
	}
	else if(cdev->specId == SPC_SM768)
	{
		ddk768_initChip();
		ddk768_deInit();
		HDMI_Init();
#ifdef USE_EP952
		EP_HDMI_Init(1);
		EP_HDMI_Set_Video_Timing(1,1);
#endif

	}
	else if(cdev->specId == SPC_SM770){
		ddk770_initChip();
		ddk770_iis_Init();
		
		hw770_init_hdmi();

		hw770_init_dp();

	}

	if((cdev->specId == SPC_SM768 || cdev->specId == SPC_SM770) && audio_en)
			smi_audio_init(dev);

	if (use_vblank) {
		drm_vblank_init(dev, dev->mode_config.num_crtc);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
		r = drm_irq_install(dev, pdev->irq);
#else
		r = request_irq(pdev->irq, smi_drm_interrupt, IRQF_SHARED,
					KBUILD_MODNAME, dev);
#endif
		if (r)
			DRM_ERROR("install irq failed , ret = %d\n", r);
	}

	dev->mode_config.funcs = (void *)&smi_mode_config_funcs;
	r = smi_modeset_init(cdev);
	if (r) {
		DRM_ERROR("Fatal error during modeset init: %d\n", r);
		goto out;
	}
#ifdef ENABLE_HDMI_IRQ
	if (cdev->specId == SPC_SM770)
	{

		r = devm_request_threaded_irq(cdev->dev->dev, pdev->irq, smi_hdmi0_hardirq,
									  smi_hdmi0_pnp_handler, IRQF_SHARED,
									  dev_name(cdev->dev->dev), cdev->dev);
		if (r)
			DRM_ERROR("install irq failed , ret = %d\n", r);

		r = devm_request_threaded_irq(cdev->dev->dev, pdev->irq, smi_hdmi1_hardirq,
									  smi_hdmi1_pnp_handler, IRQF_SHARED,
									  dev_name(cdev->dev->dev), cdev->dev);
		if (r)
			DRM_ERROR("install irq failed , ret = %d\n", r);

		r = devm_request_threaded_irq(cdev->dev->dev, pdev->irq, smi_hdmi2_hardirq,
									  smi_hdmi2_pnp_handler, IRQF_SHARED,
									  dev_name(cdev->dev->dev), cdev->dev);
		if (r)
			DRM_ERROR("install irq failed , ret = %d\n", r);
	}
#endif
	cdev->regsave = kvmalloc(1024,GFP_KERNEL);
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

	if (use_vblank){
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	if (dev->irq_enabled)
		drm_irq_uninstall(dev);
#else
	free_irq(pdev->irq, dev);
#endif
	}

	/* Disable *all* interrupts */
	if (cdev->specId == SPC_SM750) {
		ddk750_disable_IntMask();
	} else if (cdev->specId == SPC_SM768) {
		ddk768_disable_IntMask();
	} else if(cdev->specId == SPC_SM770) {
		ddk770_disable_IntMask();
	}

	if (cdev == NULL)
		return;

	smi_modeset_fini(cdev);
	smi_device_fini(cdev);


#ifndef NO_AUDIO
	if(cdev->specId == SPC_SM768 || cdev->specId == SPC_SM770)
	{
		if(audio_en)
			smi_audio_remove(dev);
    }
#endif

	kvfree(cdev->regsave);
	kfree(cdev);
	dev->dev_private = NULL;
}

/* Unmap the framebuffer from the core and release the memory */
static void smi_vram_fini(struct smi_device *cdev)
{

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0)

	if (!cdev->mm_inited)
		return;
	arch_io_free_memtype_wc(cdev->vram_base, cdev->vram_size);
	arch_phys_wc_del(cdev->fb_mtrr);
	cdev->fb_mtrr = 0;
	
#endif


}

/* Map the framebuffer from the card and configure the core */
static int smi_vram_init(struct smi_device *cdev)
{

	struct drm_device *dev = cdev->dev;
	struct pci_dev *pdev = to_pci_dev(dev->dev);
	
	cdev->vram_base = pci_resource_start(pdev, 0);

	/* VRAM Size */
	if (cdev->specId == SPC_SM750)
		cdev->vram_size = ddk750_getFrameBufSize();
	else if(cdev->specId == SPC_SM768)
		cdev->vram_size = ddk768_getFrameBufSize();
	else if (cdev->specId == SPC_SM770)
	{
		cdev->vram_size = ddk770_getFrameBufSize();
		if (cdev->vram_size > (256 << 20))
			sm770_max_mode_size *= 2;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
	/* Don't fail on errors, but performance might be reduced. */
	devm_arch_io_reserve_memtype_wc(dev->dev, cdev->vram_base, cdev->vram_size);
	devm_arch_phys_wc_add(dev->dev, cdev->vram_base, cdev->vram_size);

#else
	arch_io_reserve_memtype_wc(cdev->vram_base, cdev->vram_size);
	cdev->fb_mtrr = arch_phys_wc_add(cdev->vram_base, cdev->vram_size);
	cdev->mm_inited = true;
#endif

	cdev->vram = devm_ioremap_wc(dev->dev, cdev->vram_base, cdev->vram_size);

	if (cdev->vram == NULL)
		return -ENOMEM;
	
#ifdef CONFIG_X86
	if (cdev->fb_mtrr == 0)
	 	set_memory_wc((unsigned long)cdev->vram, cdev->vram_size >> PAGE_SHIFT);
#endif

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
	cdev->num_crtc = MAX_CRTC(cdev->specId);

	dma_bits = 40;
	cdev->need_dma32 = false;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
	ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(dma_bits));
#else
	ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(dma_bits));
#endif
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
	cdev->rmmio = devm_ioremap(cdev->dev->dev, cdev->rmmio_base, cdev->rmmio_size);

	if (cdev->rmmio == NULL)
		return -ENOMEM;

	if (cdev->specId == SPC_SM750)
		ddk750_set_mmio(cdev->rmmio, pdev->device, pdev->revision);
	else if(cdev->specId == SPC_SM768)
		ddk768_set_mmio(cdev->rmmio, pdev->device, pdev->revision);
	else if(cdev->specId == SPC_SM770)
		ddk770_set_mmio(cdev->rmmio, pdev->device, pdev->revision);
	
	ret = smi_vram_init(cdev);
	if (ret) {
		return ret;
	}

	cdev->m_connector = 0;

	return 0;
}

void smi_device_fini(struct smi_device *cdev)
{
	smi_vram_fini(cdev);
}

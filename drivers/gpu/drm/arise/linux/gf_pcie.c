/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
#include "gf.h"
#include "gf_driver.h"
#include "os_interface.h"
#include "gf_ioctl.h"
#include "gf_debugfs.h"
#include "gf_version.h"
#include "gf_gem.h"
#include "gf_gem_priv.h"
#include "gf_fence.h"
#include "gf_irq.h"
#include "gf_fbdev.h"
#include "gf_params.h"
#include "gf_pm.h"

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,5,0)
#if DRM_VERSION_CODE < KERNEL_VERSION(5,8,0)
#include <drm/drm_pci.h>
#else
#if DRM_VERSION_CODE < KERNEL_VERSION(6,8,0)
#include <drm/drm_legacy.h>
#endif
#endif
#endif
#include <linux/irq.h>

#if DRM_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
#if DRM_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
#include <drm/drm_fbdev_generic.h>
#else
#if DRM_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
#if DRM_VERSION_CODE < KERNEL_VERSION(6, 14, 0)
#include <drm/drm_client_setup.h>
#else
#include <drm/clients/drm_client_setup.h>
#endif
#endif
#include <drm/drm_fbdev_ttm.h>
#endif
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
#if DRM_VERSION_CODE < KERNEL_VERSION(6, 13, 0)
#include <drm/drm_aperture.h>
#else
#include <linux/aperture.h>
#endif
#endif

#if defined(__loongarch__)  && defined(KYLIN) && DRM_VERSION_CODE == KERNEL_VERSION(6, 6, 0)
#include <linux/sysfb.h>
#endif

#define DRIVER_DESC         "Glenfly DRM Pro"

#define ROM_IMAGE_HEADER     0xaa55

struct gf_device_info {
    u32 display_mmio_offset;
    u32 intrupte_enable_regs;
};

static struct drm_gf_driver gf_drm_driver;

/*
 * A pci_device_id struct {
 *    __u32 vendor, device;
 *      __u32 subvendor, subdevice;
 *    __u32 class, class_mask;
 *    kernel_ulong_t driver_data;
 * };
 * Don't use C99 here because "class" is reserved and we want to
 * give userspace flexibility.
 */
static const struct gf_device_info gf_e3k_info = {
    0,
    0,
};


static struct pci_device_id pciidlist[] =
{
    {0x1d17, 0x3D00, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (kernel_ulong_t)&gf_e3k_info}, //e3k
    {0x6766, 0x3D00, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (kernel_ulong_t)&gf_e3k_info}, //arise10c0
    {0x6766, 0x3D02, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (kernel_ulong_t)&gf_e3k_info}, //arise1020
    {0x6766, 0x3D03, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (kernel_ulong_t)&gf_e3k_info}, //arise1040
    {0x6766, 0x3D04, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (kernel_ulong_t)&gf_e3k_info}, //arise1010
    {0x6766, 0x3D06, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (kernel_ulong_t)&gf_e3k_info}, //arise10c0t
    {0x6766, 0x3D07, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (kernel_ulong_t)&gf_e3k_info},
    {0x6766, 0x3D08, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (kernel_ulong_t)&gf_e3k_info},
    {0x6766, 0x3D0E, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (kernel_ulong_t)&gf_e3k_info}, //arise10D0
    {0, 0, 0}
};

MODULE_DEVICE_TABLE(pci, pciidlist);

static struct pci_driver gf_driver;

#if 0
static int gf_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    gf_card_t    *gf = &gf_cards[num_probed_card++];
    int           ret  = 0;

    gf_memset(gf, 0, sizeof(gf_card_t));

    pci_set_drvdata(pdev, gf);

    gf_card_pre_init(gf, pdev);

    ret = pci_request_regions(pdev, gf_driver.name);
    if(ret)
    {
        gf_error("pci_request_regions() failed. ret:%x.\n", ret);
    }

    ret = pci_enable_device(pdev);
    if(ret)
    {
        gf_error("pci_enable_device() failed. ret:%x.\n", ret);
    }

    pci_set_master(pdev);

    /*don't use the vga arbiter*/
#if defined(CONFIG_VGA_ARB)
    vga_set_legacy_decoding(pdev, VGA_RSRC_NONE);
#endif

    ret = gf_card_init(gf, pdev);

    return ret;
}

static void gf_pcie_shutdown(struct pci_dev *pdev)
{
    gf_card_t *gf = pci_get_drvdata(pdev);

    if(gf->gfb_enable)
    {
#ifdef CONFIG_FB
        gf_fb_shutdown(gf);
#endif
    }

}
#endif

static int __gf_drm_suspend(struct drm_device *dev, pm_message_t state, bool fbcon)
{
    gf_card_t* gf = dev->dev_private;
    int ret;

    gf_info("gfx driver suspending, pm event = %d.\n", state.event);
    disp_suspend(dev);
    gf_info("drm suspend: save display status finished.\n");

    if (fbcon)
    {
    #if DRM_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
        gf_fbdev_set_suspend(gf, 1);
    #else
        drm_fb_helper_set_suspend_unlocked(dev->fb_helper, true);
    #endif
        gf_info("drm suspend: save drmfb status finished.\n");
    }

    ret = gf_core_interface->save_state(gf->adapter, state.event == PM_EVENT_FREEZE);

    if (ret)
    {
        gf_error("drm suspend: save state failed.\n");
        return ret;
    }

    /* disable IRQ */
#ifdef _DEBUG_
    gf_info("Start Disable irq.\n");
#endif
    gf_disable_interrupt(gf->pdev);

#ifdef _DEBUG_
    gf_info("Start Disable tasklet.\n");
#endif
    tasklet_disable(&gf->fence_notify);

    synchronize_irq(to_pci_dev(dev->dev)->irq);
    gf_info("drm suspend: disable irq finished.\n");

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    disp_vblank_save(dev);
#endif

    pci_save_state(to_pci_dev(dev->dev));

    if (state.event == PM_EVENT_SUSPEND)
    {
        /*patch for CHX001 A0 and B0, pci_disable_device will clear master bit PCI04[2], and this bit can't be cleared for chx001 hw, so don't call pci_disable_device here*/
        pci_disable_device(to_pci_dev(dev->dev));
        pci_set_power_state(to_pci_dev(dev->dev), PCI_D3hot);
    }

    return 0;
}

static int gf_drm_suspend(struct drm_device *dev, pm_message_t state)
{
    return __gf_drm_suspend(dev, state, true);
}

static int gf_runtime_suspend(struct drm_device *dev, pm_message_t state)
{
    //TODO
    return 0;
    //return __gf_drm_suspend(dev, state, false);
}

static int __gf_drm_resume(struct drm_device *dev, bool fbcon)
{
    gf_card_t* gf = dev->dev_private;
    int         ret = 0;

    gf_info("gfx driver resume back.\n");

    pci_set_power_state(to_pci_dev(dev->dev), PCI_D0);

    pci_restore_state(to_pci_dev(dev->dev));

    if (pci_enable_device(to_pci_dev(dev->dev)))
    {
        return -1;
    }
    pci_set_master(to_pci_dev(dev->dev));

    disp_pre_resume(dev);
    gf_info("drm resume: enable and post chip finished.\n");

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    disp_vblank_restore(dev);
#endif

    gf_core_interface->reset_dvfs_power_flag(gf->adapter);

    tasklet_enable(&gf->fence_notify);
    gf_enable_interrupt(gf->pdev);
    gf_info("drm resume: re-enable irq finished.\n");

    ret = gf_core_interface->restore_state(gf->adapter);

    if(ret != 0)
    {
        return -1;
    }

    if (fbcon)
    {
    #if DRM_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
        gf_fbdev_set_suspend(gf, 0);
    #else
        drm_fb_helper_set_suspend_unlocked(dev->fb_helper, false);
    #endif
        gf_info("drm resume: restore drmfb status finished.\n");
    }

    disp_post_resume(dev);
    gf_info("drm resume: restore display status finished.\n");

    return 0;
}

static int gf_drm_resume(struct drm_device *dev)
{
    return __gf_drm_resume(dev, true);
}

static int gf_runtime_resume(struct drm_device *dev)
{
    //TODO
    return 0;
    //return __gf_drm_resume(dev, false);
}

static __inline__ int gf_backdoor_available(void)
{
#if defined(__i386__) || defined(__x86_64__)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
    return (boot_cpu_data.x86_vendor == X86_VENDOR_CENTAUR || boot_cpu_data.x86_vendor == X86_VENDOR_ZHAOXIN) &&
           boot_cpu_data.x86_model == 59;
#else
    // old kernel may be patched with zhaoxin cpu driver (such as UOS 4.19 and kylin V10 4.4)
    return (boot_cpu_data.x86_vendor == X86_VENDOR_CENTAUR || boot_cpu_data.x86_vendor == 10) &&
           boot_cpu_data.x86_model == 59;
#endif
#else
    return TRUE;
#endif
}

static __inline__ void gf_init_adapter_info_by_params(struct krnl_adapter_init_info_s *info, struct gf_params *p)
{
    info->gf_pwm_mode                   = p->gf_pwm_mode;
    info->gf_dfs_mode                   = p->gf_dfs_mode;
    info->gf_worker_thread_enable       = p->gf_worker_thread_enable;
    info->gf_recovery_enable            = p->gf_recovery_enable;
    info->gf_hang_dump                  = p->gf_hang_dump;
    info->gf_run_on_qt                  = p->gf_run_on_qt;
    info->gf_flag_buffer_verify         = p->gf_flag_buffer_verify;
    info->gf_vesa_tempbuffer_enable     = p->gf_vesa_tempbuffer_enable;

    info->miu_channel_size              = p->miu_channel_size;
    info->gf_backdoor_enable            = p->gf_backdoor_enable && gf_backdoor_available();

    info->chip_slice_mask               = p->chip_slice_mask;
    info->gf_local_size_g               = p->gf_local_size_g;
    info->gf_local_size_m               = p->gf_local_size_m;
    info->gf_pcie_size_g                = p->gf_pcie_size_g;
    info->gf_pcie_size_m                = p->gf_pcie_size_m;
    info->debugfs_mask                  = p->debugfs_mask;
}

#if  DRM_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
static int gf_kick_out_firmware_fb(struct pci_dev *pdev)
{
    struct apertures_struct *ap;
    bool primary = false;
#if defined(__loongarch__)
    int i = 0;
    struct fb_info *pfb_info = NULL;
    unsigned long long fix_fb_base;
    struct platform_device *pd = NULL;
#endif

    ap = alloc_apertures(1);
    if (!ap)
        return -ENOMEM;

    ap->ranges[0].base = pci_resource_start(pdev, 1);
    ap->ranges[0].size = pci_resource_len(pdev, 1);

#ifdef CONFIG_X86
    primary = pdev->resource[PCI_ROM_RESOURCE].flags & IORESOURCE_ROM_SHADOW;
#endif

#if defined(__loongarch__)
    if ((ap->ranges[0].base >> 32) != 0)
    {
        fix_fb_base =  ap->ranges[0].base & 0xFFFFFFFF;
        for (i = 0; i < num_registered_fb; i++)
        {
            pfb_info = registered_fb[i];
            if (!pfb_info)
            {
                continue;
            }

            pd = to_platform_device(pfb_info->device);
            if (!pd)
            {
                continue;
            }

            if (gf_strcmp(pd->name, "efi-framebuffer") == 0 &&
                pfb_info->fix.smem_start == fix_fb_base)
            {
                ap->ranges[0].base = fix_fb_base;
                gf_info("reassign aperture base address to remove firmware framebuffer.");
                break;
            }
        }
    }
#endif

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 9, 0)
    drm_fb_helper_remove_conflicting_framebuffers(ap, "arisedrmfb", primary);
#else
    remove_conflicting_framebuffers(ap, "arisedrmfb", primary);
#endif

    gf_free(ap);

    return 0;
}
#endif

#define PCI_EN_IO_SPACE     1
static int gf_drm_load_kms(struct drm_device *dev, unsigned long flags)
{
    struct pci_dev *pdev = to_pci_dev(dev->dev);
    struct device* device = &pdev->dev;
    gf_card_t  *gf = NULL;
    int         ret = 0;

    gf = (gf_card_t *)gf_calloc(sizeof(gf_card_t));
    if (!gf) {
        gf_error("allocate failed for gfx card!\n");
        return -ENOMEM;
    }

    dev->dev_private = (void*)gf;

    gf->drm_dev = dev;
    //TODO..FIXME. need remove gf->index.
    gf->index  = dev->primary->index;

    pci_set_drvdata(pdev, dev);

    dma_set_mask_and_coherent(device, DMA_BIT_MASK(40));

    gf->pdev = pdev;

#if  DRM_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
#if  DRM_VERSION_CODE < KERNEL_VERSION(6, 13, 0)

#if defined(__loongarch__)  && defined(KYLIN) && DRM_VERSION_CODE == KERNEL_VERSION(6, 6, 0)
    //NOTE: add patch here to resolve firmware fb remove failure for old firmware on loongarch64 platform.
    //  gop fb address issue has been fixed from firmware 03000014 version.
    //  now only patch for KylinV11 which use kernel verion 6.6.0
    if (vga_default_device() == pdev)
    {
        sysfb_disable(NULL);
    }
#endif

    ret = drm_aperture_remove_conflicting_pci_framebuffers(pdev, (const struct drm_driver *)&gf_drm_driver.base);
#else
    ret = aperture_remove_conflicting_pci_devices(pdev, gf_drm_driver.base.name);
#endif
#elif DRM_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
    ret = drm_aperture_remove_conflicting_pci_framebuffers(pdev, "arisedrmfb");
#else
    ret = gf_kick_out_firmware_fb(pdev);
#endif

    if (ret)
    {
        gf_error("remove conflicting framebuffer failed. ret:%x.\n", ret);
    }

    pci_set_master(pdev);

   //fpga only
#if 0
    unsigned short command = 0;
    gf_get_command_status16(pdev,&command);
    gf_info("command:%x\n",command);
    if(!(command & PCI_EN_IO_SPACE))
    {
        gf_write_command_status16(pdev,(command|PCI_EN_IO_SPACE));
        gf_info("set primary command:%x\n",gf_get_command_status16(pdev,&command));
    }
#endif

    /*don't use the vga arbiter*/
#if defined(CONFIG_VGA_ARB)
    vga_set_legacy_decoding(pdev, VGA_RSRC_NONE);
#endif

    gf_init_adapter_info_by_params(&gf->a_info, &gf_modparams);
    gf->a_info.minor_index = gf->index;
    ret = gf_card_init(gf, pdev);
    if(ret)
    {
        gf_error("%s_card_init() failed. ret:0x%x\n", STR(DRIVER_NAME), ret);
    }

    gf->fence_drv = gf_calloc(sizeof(struct gf_dma_fence_driver));
    gf->fence_drv->gf_card = gf;

    gf_dma_fence_driver_init(gf->adapter, gf->fence_drv);

    gf_info("%s = %p, %s->pdev = %p, dev = %p, dev->primary = %p\n", STR(DRIVER_NAME), gf, STR(DRIVER_NAME), gf->pdev,  dev, dev ? dev->primary : NULL);

    disp_irq_install(gf->disp_info);

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
    gf_debugfs_init(gf);
    gf_fbdev_init(gf);
#endif

    if (gf->runtime_pm)
    {
        gf_rpm_set_driver_flags(dev->dev);
        gf_rpm_use_autosuspend(dev->dev);
        gf_rpm_set_autosuspend_delay(dev->dev, 5000); //5s
        gf_rpm_set_active(dev->dev);
        gf_rpm_allow(dev->dev);             //-1
        gf_rpm_mark_last_busy(dev->dev);
        gf_rpm_put_autosuspend(dev->dev);
    }

    return ret;
}

static int gf_drm_open(struct drm_device *dev, struct drm_file *file)
{
    gf_card_t *gf = dev->dev_private;
    gf_file_t *priv;
    int err = -ENODEV;

    if (!gf->adapter)
    {
        return err;
    }

    err = gf_rpm_get_sync(dev->dev);
    if (err < 0)
    {
        goto err_pm_put;
    }

    priv = gf_calloc(sizeof(gf_file_t));
    if (!priv)
    {
        err = -ENOMEM;
        goto err_suspend_out;
    }

    file->driver_priv = priv;
    priv->parent_file = file;
    priv->card  = gf;

    priv->lock  = gf_create_mutex();

    err = gf_core_interface->create_device(gf->adapter, file, &priv->gpu_device);
    gf_assert(err == 0, GF_FUNC_NAME(__func__));
    if(gf->debugfs_dev)
    {
        priv->debug = gf_debugfs_add_device_node(gf->debugfs_dev, gf_get_current_pid(), priv->gpu_device);
    }

err_suspend_out:
    gf_rpm_mark_last_busy(dev->dev);
err_pm_put:
    gf_rpm_put_autosuspend(dev->dev);

    return err;
}

static void gf_drm_postclose(struct drm_device *dev, struct drm_file *file)
{
    gf_file_t *priv = file->driver_priv;
    gf_card_t *gf  = priv->card;

    if(priv->hold_lock)
    {
        gf_mutex_unlock(gf->lock);
    }

    gf_rpm_get_sync(dev->dev);

    if(priv->gpu_device)
    {
        if(gf->debugfs_dev)
        {
            gf_debugfs_remove_device_node(gf->debugfs_dev, priv->debug);
            priv->debug = NULL;
        }

        gf_core_interface->final_cleanup(gf->adapter, priv->gpu_device);
        priv->gpu_device = 0;
    }

    gf_destroy_mutex(priv->lock);
    gf_free(priv);

    file->driver_priv = NULL;

    gf_rpm_mark_last_busy(dev->dev);
    gf_rpm_put_autosuspend(dev->dev);
}

static void  gf_drm_last_close(struct drm_device* dev)
{
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
    gf_card_t *gf = dev->dev_private;
    gf_fbdev_restore_mode(gf);
#else
    drm_fb_helper_lastclose(dev);
#endif
}

#if DRM_VERSION_CODE < KERNEL_VERSION(4,11,0)
static int gf_drm_device_is_agp(struct drm_device * dev)
{
    return 0;
}
#endif

int gf_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int ret = 0;
#ifndef __frv__
    struct drm_file *file = filp->private_data;
    gf_file_t     *priv = file->driver_priv;
    gf_card_t     *card = priv->card;
    gf_map_argu_t *map  = priv->map;
    gf_map_argu_t map_argu;

    if(map == NULL)
    {
        bus_config_t bus_config = {0};

        unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

        gf_get_bus_config(card->pdev, &bus_config);

        map_argu.flags.mem_type   = GF_SYSTEM_IO;

        if((offset >= bus_config.mem_start_addr[0]) &&
           (offset <  bus_config.mem_end_addr[0]))
        {
            /* map fb as wc */
            map_argu.flags.cache_type = GF_MEM_WRITE_COMBINED;
        }
        else
        {
            /* map mmio reg as uc*/
            map_argu.flags.cache_type = GF_MEM_UNCACHED;
        }

        map = &map_argu;
    }

    switch(map->flags.mem_type)
    {
        case GF_SYSTEM_IO:
            ret = gf_map_system_io(vma, map);
            break;

        case GF_SYSTEM_RAM:
        //case GF_SYSTEM_RAM_DYNAMIC:
            ret = gf_map_system_ram(vma, map);
            break;

        default:
            gf_error("%s, unknown memory map type", GF_FUNC_NAME(__func__));
            ret = -1;
            break;
    }
#endif

    return ret;
}

static long gf_drm_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct drm_file *file_priv = filp->private_data;
    struct drm_device *dev = file_priv->minor->dev ;
    long ret;

    ret = gf_rpm_get_sync(dev->dev);
    if (ret < 0)
    {
        goto out;
    }

    ret = drm_ioctl(filp, cmd, arg);

    gf_rpm_mark_last_busy(dev->dev);
out:
    gf_rpm_put_autosuspend(dev->dev);
    return ret;
}


#if defined(CONFIG_COMPAT)
static long gf_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret;
    unsigned int nr = GF_IOCTL_NR(cmd);

    if (nr < DRM_COMMAND_BASE)
    {
        ret = drm_compat_ioctl(filp, cmd, arg);
    }
    else
    {
        ret = gf_drm_ioctl(filp, cmd, arg);
    }

    return ret;
}
#endif


static void gf_drm_unload_kms(struct drm_device *dev)
{
    gf_card_t *gf = dev->dev_private;
    gf_info("drm unload.\n");

    if(gf == NULL) return;

    disp_irq_uninstall(gf->disp_info);

    if (gf->fence_drv) {
        gf_dma_fence_driver_fini(gf->fence_drv);
        gf_free(gf->fence_drv);
        gf->fence_drv = NULL;
    }
    gf_card_deinit(gf);
    gf_free(gf);
    dev->dev_private = NULL;
}
static struct file_operations gf_drm_fops = {
    .owner      = THIS_MODULE,
    .open       = drm_open,
    .release    = drm_release,
    .unlocked_ioctl = gf_drm_ioctl,
#if defined(__x86_64__) && defined(CONFIG_COMPAT)
    .compat_ioctl = gf_compat_ioctl,
#endif
    .mmap       = gf_drm_gem_mmap,
    .read       = drm_read,
    .poll       = drm_poll,
    .llseek     = noop_llseek,
#if DRM_VERSION_CODE >= KERNEL_VERSION(6, 12, 0)
    .fop_flags  = FOP_UNSIGNED_OFFSET,
#endif
};

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
#define  GF_DRM_FEATURE \
    ( DRIVER_MODESET | DRIVER_RENDER | DRIVER_GEM)
#elif DRM_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
#define  GF_DRM_FEATURE \
    ( DRIVER_MODESET | DRIVER_RENDER | DRIVER_PRIME | DRIVER_GEM)
#else
#define  GF_DRM_FEATURE \
    (DRIVER_HAVE_IRQ | DRIVER_IRQ_SHARED | DRIVER_MODESET | DRIVER_RENDER | DRIVER_PRIME | DRIVER_GEM)
#endif

static struct drm_gf_driver gf_drm_driver = {
    .file_priv = NULL,
//    .lock = __MUTEX_INITIALIZER(gf_drm_driver_lock),
    .base = {
        .driver_features    = GF_DRM_FEATURE,
    #if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
        .driver_features = GF_DRM_FEATURE | DRIVER_ATOMIC,
    #endif

    #if DRM_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
        .load               = gf_drm_load_kms,
    #endif
        .open               = gf_drm_open,

    #if DRM_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
        .unload             = gf_drm_unload_kms,
    #endif

        .postclose          = gf_drm_postclose,
    #if DRM_VERSION_CODE < KERNEL_VERSION(6,12,0)
        .lastclose          = gf_drm_last_close,
    #endif
        .ioctls             = gf_ioctls,
        .num_ioctls         = ioctl_nr_total_num,
    #if DRM_VERSION_CODE < KERNEL_VERSION(4,14,0) && DRM_VERSION_CODE >= KERNEL_VERSION(3,18,0)
        .set_busid          = drm_pci_set_busid,
    #endif

    #if DRM_VERSION_CODE < KERNEL_VERSION(4,0,0)
    #ifndef __ARM_ARCH
        .suspend            = gf_drm_suspend,
        .resume             = gf_drm_resume,
    #endif
    #endif
    #if DRM_VERSION_CODE < KERNEL_VERSION(4,11,0)
        .device_is_agp      = gf_drm_device_is_agp,
    #endif
        .fops               = &gf_drm_fops,

    #if DRM_VERSION_CODE < KERNEL_VERSION(5,11,0)
        .gem_vm_ops         = &gf_gem_vm_ops,
    #endif

    #if DRM_VERSION_CODE >=  KERNEL_VERSION(6, 13, 0)
        DRM_FBDEV_TTM_DRIVER_OPS,
    #endif

    #if DRM_VERSION_CODE <  KERNEL_VERSION(5, 11, 0)
        .gem_prime_vmap = gf_gem_prime_vmap,
        .gem_prime_vunmap = gf_gem_prime_vunmap,
    #endif


    #if DRM_VERSION_CODE < KERNEL_VERSION(5,9,0)
        .gem_free_object    = gf_gem_free_object,
    #elif DRM_VERSION_CODE < KERNEL_VERSION(5,11,0)
        .gem_free_object_unlocked = gf_gem_free_object,
    #else
    #endif
    #if DRM_VERSION_CODE < KERNEL_VERSION(6,6,0)
        .prime_handle_to_fd = drm_gem_prime_handle_to_fd,
        .prime_fd_to_handle = gf_gem_prime_fd_to_handle,
    #endif

    #if DRM_VERSION_CODE < KERNEL_VERSION(5,11,0)
        .gem_open_object    = gf_gem_open,
        .gem_prime_export   = gf_gem_prime_export,
    #endif
        .gem_prime_import   = gf_gem_prime_import,

        .dumb_create        = gf_gem_dumb_create,
        .dumb_map_offset    = gf_gem_mmap_gtt,

     #if DRM_VERSION_CODE < KERNEL_VERSION(5,14,0)
        .irq_preinstall  = gf_irq_preinstall,
        .irq_postinstall = gf_irq_postinstall,
        .irq_handler     = gf_irq_handle,
        .irq_uninstall   = gf_irq_uninstall,
    #endif

    #if DRM_VERSION_CODE < KERNEL_VERSION(5,12,0)
        .dumb_destroy       = drm_gem_dumb_destroy,
    #endif
        .name               = STR(DRIVER_NAME),
        .desc               = DRIVER_DESC,
    #if DRM_VERSION_CODE < KERNEL_VERSION(6, 14, 0)
        .date               = DRIVER_DATE,
    #endif
        .major              = DRIVER_MAJOR,
        .minor              = DRIVER_MINOR,
        .patchlevel         = DRIVER_PATCHLEVEL,
    }
};

static int gf_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 19 ,0)
    struct drm_device *dev;
    unsigned long flags = ent->driver_data;
    gf_card_t *gf = NULL;
    int ret;

    dev = drm_dev_alloc(&gf_drm_driver.base, &pdev->dev);
    if (IS_ERR(dev))
    {
        return PTR_ERR(dev);
    }

    ret = pci_enable_device(pdev);
    if (ret)
    {
        goto err_free;
    }

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
    dev->pdev = pdev;
#endif

    pci_set_drvdata(pdev, dev);

    ret = gf_drm_load_kms(dev, flags);
    if (ret)
    {
        goto err_pci;
    }

    ret = drm_dev_register(dev, flags);
    if (ret)
    {
        goto err_pci;
    }

    gf = (gf_card_t *)dev->dev_private;

    gf_debugfs_init(gf);

#if DRM_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
    drm_fbdev_generic_setup(dev, 32);
#elif DRM_VERSION_CODE < KERNEL_VERSION(6, 13, 0)
    drm_fbdev_ttm_setup(dev, 32);
#else
    drm_client_setup(dev, NULL);
#endif

#if DRM_VERSION_CODE < KERNEL_VERSION(5, 2, 0)
    //skip_vt_switch is not set in drm_fb_helper_alloc_fbi function under kernel version of 5.2.0, patch it here
    if (dev->fb_helper && dev->fb_helper->fbdev &&
        dev->fb_helper->fbdev->dev)
    {
        pm_vt_switch_required(dev->fb_helper->fbdev->dev, false);
    }
#endif

    return 0;

err_pci:
    pci_disable_device(pdev);
err_free:
    drm_dev_put(dev);
    return ret;

#elif DRM_VERSION_CODE > KERNEL_VERSION(3,10,52)
    return drm_get_pci_dev(pdev, ent, &gf_drm_driver.base);
#else
    return 0;
#endif
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(3,10,52)
#if DRM_VERSION_CODE >= KERNEL_VERSION(6,11,0)
static void gf_pcie_cleanup(struct pci_dev *pdev)
#else
static void __exit gf_pcie_cleanup(struct pci_dev *pdev)
#endif
{
    struct drm_device *dev = pci_get_drvdata(pdev);
    gf_card_t *gf_card = (gf_card_t *)dev->dev_private;

    gf_info("pcie_cleanup.\n");

    if (gf_card->runtime_pm)
    {
        gf_rpm_get_sync(dev->dev);
        gf_rpm_forbid(dev->dev);
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
    drm_dev_unplug(dev);

#if DRM_VERSION_CODE >= KERNEL_VERSION(5, 2, 0)
    drm_dev_put(dev);
#endif

#else
    gf_drm_unload_kms(dev);

    drm_put_dev(dev);
#endif

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 19 ,0)
    pci_disable_device(pdev);
#endif

    pci_set_drvdata(pdev, NULL);
}
#endif

static int gf_pmops_suspend(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);

    gf_info("pci device(vendor:0x%X, device:0x%X) pm suspend\n", pdev->vendor, pdev->device);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4,0,0)
    gf_drm_suspend(pci_get_drvdata(pdev), PMSG_SUSPEND);
#endif

    return 0;
}
static int gf_pmops_resume(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct drm_device *drm_dev = pci_get_drvdata(pdev);
    gf_card_t *gf_card = (gf_card_t *)drm_dev->dev_private;

    gf_info("pci device(vendor:0x%X, device:0x%X) pm resume\n", pdev->vendor, pdev->device);

    if (gf_card->runtime_pm)
    {
        gf_rpm_disable(dev);
        gf_rpm_set_active(dev);
        gf_rpm_enable(dev);
    }

#if DRM_VERSION_CODE >= KERNEL_VERSION(4,0,0)
    gf_drm_resume(pci_get_drvdata(pdev));
#endif

    return 0;
}

static int gf_pmops_freeze(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);

    gf_info("pci device(vendor:0x%X, device:0x%X) pm freeze\n", pdev->vendor, pdev->device);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4,0,0)
    gf_drm_suspend(pci_get_drvdata(pdev), PMSG_FREEZE);
#endif

    return 0;
}

static int gf_pmops_thaw(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct drm_device *drm_dev = NULL;
    gf_card_t *gf_card = NULL;

    gf_info("pci device(vendor:0x%X, device:0x%X) pm thaw\n", pdev->vendor, pdev->device);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4,0,0)
    drm_dev = (struct drm_device *)pci_get_drvdata(pdev);
    gf_card = (gf_card_t *)drm_dev->dev_private;
    gf_card->flags |= GF_S4_RESUME;

    gf_drm_resume(pci_get_drvdata(pdev));
#endif

    return 0;
}

static int gf_pmops_poweroff(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);

    gf_info("pci device(vendor:0x%X, device:0x%X) pm poweroff\n", pdev->vendor, pdev->device);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4,0,0)
    gf_drm_suspend(pci_get_drvdata(pdev), PMSG_SUSPEND);
#endif

    return 0;
}

static int gf_pmops_restore(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);

    gf_info("pci device(vendor:0x%X, device:0x%X) pm restore\n", pdev->vendor, pdev->device);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4,0,0)
    gf_drm_resume(pci_get_drvdata(pdev));
#endif

    return 0;
}

static int gf_pmops_runtime_suspend(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct drm_device *drm_dev = pci_get_drvdata(pdev);
    gf_card_t *gf_card = (gf_card_t *)drm_dev->dev_private;

    if (!gf_card->runtime_pm)
    {
        gf_rpm_forbid(dev);
        return -EBUSY;
    }

    gf_info("pci device(vendor:0x%X, device:0x%X) pm runtime_suspend\n", pdev->vendor, pdev->device);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4,0,0)
    gf_runtime_suspend(drm_dev, PMSG_AUTO_SUSPEND);
#endif

    return 0;
}

static int gf_pmops_runtime_resume(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct drm_device *drm_dev = dev_get_drvdata(dev);
    gf_card_t *gf_card = (gf_card_t *)drm_dev->dev_private;

    if (!gf_card->runtime_pm)
    {
        return -EINVAL;
    }

    gf_info("pci device(vendor:0x%X, device:0x%X) pm runtime_resume\n", pdev->vendor, pdev->device);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4,0,0)
    gf_runtime_resume(drm_dev);
#endif

    return 0;
}

static int gf_pmops_runtime_idle(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct drm_device *drm_dev = dev_get_drvdata(dev);
    gf_card_t *gf_card = (gf_card_t *)drm_dev->dev_private;
    struct drm_crtc *crtc;

    if (!gf_card->runtime_pm)
    {
        gf_rpm_forbid(dev);
        return -EBUSY;
    }

    gf_info("pci device(vendor:0x%X, device:0x%X) pm runtime_idle\n", pdev->vendor, pdev->device);

    list_for_each_entry(crtc, &drm_dev->mode_config.crtc_list, head)
    {
        if (crtc->enabled)
        {
            DRM_DEBUG_DRIVER("fail to power off - crtc active\n");
            return -EBUSY;
        }
    }

    gf_rpm_mark_last_busy(dev);
    gf_rpm_autosuspend(dev);

    /* use autosuspend instead of calling rpm_suspend directly in main rpm_idle */
    return 1;
}

static void gf_shutdown(struct pci_dev *pdev)
{
    struct drm_device *dev = pci_get_drvdata(pdev);
    gf_card_t *gf = dev->dev_private;

    gf_info("pci device(vendor:0x%X, device:0x%X) shutting down.\n", pdev->vendor, pdev->device);

    gf->flags |= GF_SHUT_DOWN;
    gf_core_interface->wait_chip_idle(gf->adapter);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
    drm_atomic_helper_suspend(dev);
#else
    gf_disp_suspend_helper(dev);
#endif
    gf_info("pci device(vendor:0x%X, device:0x%X) already shut down.\n", pdev->vendor, pdev->device);
}

static const struct dev_pm_ops gf_pm_ops = {
    .suspend = gf_pmops_suspend,
    .resume = gf_pmops_resume,
    .freeze = gf_pmops_freeze,
    .thaw = gf_pmops_thaw,
    .poweroff = gf_pmops_poweroff,
    .restore = gf_pmops_restore,
    .runtime_suspend = gf_pmops_runtime_suspend,
    .runtime_resume = gf_pmops_runtime_resume,
    .runtime_idle = gf_pmops_runtime_idle,
};

static struct pci_driver gf_driver =
{
    .name = STR(DRIVER_NAME),
    .id_table = pciidlist,
    .probe = gf_pcie_probe,
#if DRM_VERSION_CODE < KERNEL_VERSION(3,10,52)
   //.remove   = __devexit_p(gf_pcie_cleanup),
   .remove   = NULL,
#else
    .remove   = __exit_p(gf_pcie_cleanup),
#endif
    .driver.pm = &gf_pm_ops,
    .shutdown  = gf_shutdown,
};


/*************************************** PCI functions ********************************************/

int gf_get_command_status16(void *dev, unsigned short *command)
{
    return pci_read_config_word((struct pci_dev*)dev, 0x4, command);
}

int gf_get_command_status32(void *dev, unsigned int *command)
{
    return pci_read_config_dword((struct pci_dev*)dev, 0x4, command);
}

int gf_write_command_status16(void *dev, unsigned short command)
{
    return pci_write_config_word((struct pci_dev*)dev, 0x4, command);
}

int gf_write_command_status32(void *dev, unsigned int command)
{
    return pci_write_config_dword((struct pci_dev*)dev, 0x4, command);
}

int gf_get_bar1(void *dev, unsigned int *bar1)
{
    return pci_read_config_dword((struct pci_dev*)dev, 0x14, bar1);
}

int gf_get_rom_save_addr(void *dev, unsigned int *romsave)
{
    return pci_read_config_dword((struct pci_dev*)dev, 0x30, romsave);
}

int gf_write_rom_save_addr(void *dev, unsigned int romsave)
{
    return pci_write_config_dword((struct pci_dev*)dev, 0x30, romsave);
}

int gf_pci_get_rom(void *dev, void *buf, unsigned int buf_size)
{
    size_t size;
    int ret = -1;
    struct pci_dev *pdev = (struct pci_dev*)dev;
#ifdef __aarch64__
    struct resource *res = &pdev->resource[PCI_ROM_RESOURCE];
    void            *virt_addr = NULL;

    if ((res == NULL) || (res->start == 0) || (res->start == ~0))
    {
        gf_info("invalid pci resource!\n");
        return -1;
    }

    if (pci_enable_rom(pdev))
    {
        gf_info("enable pci rom failed!\n");
        return -1;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
    virt_addr = ioremap((phys_addr_t)res->start, 0x10000);
#else
    virt_addr = ioremap_nocache((phys_addr_t)res->start, 0x10000);
#endif

    if (virt_addr)
    {
        if (*(unsigned short*)virt_addr == ROM_IMAGE_HEADER)
        {
            gf_memcpy(buf, virt_addr, buf_size);
            ret = 0;
        }

        iounmap(virt_addr);

        if (!(res->flags & IORESOURCE_ROM_ENABLE))
        {
            pci_disable_rom(pdev);
        }
    }
#else
    unsigned int  rom_base = 0;
    gf_map_argu_t map  = {0};
    gf_vm_area_t  *vma = NULL;
    void          *src_image = NULL;

    gf_get_rom_save_addr(pdev, &rom_base);
    rom_base |= 0x01;   // enable expansion rom
    gf_write_rom_save_addr(pdev, rom_base);
    rom_base &= 0xFFFFFFFE;
    map.flags.cache_type = GF_MEM_UNCACHED;
    map.flags.mem_space  = GF_MEM_KERNEL;
    map.flags.mem_type   = GF_SYSTEM_IO;
    map.size             = 0x10000;
    map.phys_addr        = rom_base;

    vma = gf_map_io_memory(NULL, &map);
    src_image = vma->virt_addr;

    if ((src_image) && (*(unsigned short*)src_image == ROM_IMAGE_HEADER))
    {
        gf_memcpy(buf, src_image, buf_size);
        gf_unmap_io_memory(vma);
        ret = 0;
    }
    else
    {
        if (src_image)
        {
            gf_unmap_io_memory(vma);
            gf_write_rom_save_addr(pdev, rom_base);
        }

        src_image = pci_map_rom(pdev, &size);
        if (src_image)
        {
            if (*(unsigned short*)src_image == ROM_IMAGE_HEADER)
            {
                gf_memcpy(buf, src_image, buf_size);
                ret = 0;
            }

            pci_unmap_rom(pdev, src_image);
        }
    }
#endif

    return ret;
}

unsigned long gf_get_rom_start_addr(void *dev)
{
    return pci_resource_start((struct pci_dev*)dev, 6);
}

int gf_get_platform_config(void *dev, const char* config_name, int *buffer, int length)
{
    return 0;
}

void gf_get_bus_config(void *dev, bus_config_t *bus)
{
    struct pci_dev *pdev = dev;

    pci_read_config_word(pdev, 0x2,  &bus->device_id);
    pci_read_config_word(pdev, 0x0,  &bus->vendor_id);
    pci_read_config_word(pdev, 0x4,  &bus->command);
    pci_read_config_word(pdev, 0x6,  &bus->status);
    pci_read_config_byte(pdev, 0x8,  &bus->revision_id);
    pci_read_config_byte(pdev, 0x9,  &bus->prog_if);

    pci_read_config_byte(pdev, 0xa,  &bus->sub_class);
    pci_read_config_byte(pdev, 0xb,  &bus->base_class);
    pci_read_config_byte(pdev, 0xc,  &bus->cache_line_size);
    pci_read_config_byte(pdev, 0xd,  &bus->latency_timer);
    pci_read_config_byte(pdev, 0xe,  &bus->header_type);
    pci_read_config_byte(pdev, 0xf,  &bus->bist);
    pci_read_config_word(pdev, 0x2c, &bus->sub_sys_vendor_id);
    pci_read_config_word(pdev, 0x2e, &bus->sub_sys_id);
    //pci_read_config_word(pdev, 0x52, &bus->link_status);
    pcie_capability_read_word(pdev,0x12,  &bus->link_status); //PCI_EXP_LNKST

    //pci_write_config_word(pdev, 0x4,  7);

    bus->reg_start_addr[0] = pci_resource_start(pdev, 0);

    bus->mem_start_addr[0] = pci_resource_start(pdev, 1);

    bus->reg_start_addr[2] = 0;
    bus->reg_start_addr[3] = 0;
    bus->reg_start_addr[4] = 0;

    bus->reg_end_addr[0]   = pci_resource_end(pdev, 0);
    bus->mem_end_addr[0]   = pci_resource_end(pdev, 1);

    bus->reg_end_addr[2]   = 0;
    bus->reg_end_addr[3]   = 0;
    bus->reg_end_addr[4]   = 0;
}

void gf_init_bus_id(gf_card_t *gf)
{
    struct pci_dev *pdev = gf->pdev;

    int pci_domain = 0;
    int pci_bus    = pdev->bus->number;
    int pci_slot   = PCI_SLOT(pdev->devfn);
    int pci_func   = PCI_FUNC(pdev->devfn);

    gf->len = snprintf(gf->busId, 40, "pci:%04x:%02x:%02x.%d", pci_domain, pci_bus, pci_slot, pci_func);
}

int gf_register_driver(void)
{
    int ret = 0;

    mutex_init(&gf_drm_driver.lock);

    ret = pci_register_driver(&gf_driver);//register driver first,  register drm device during pci probe
#if DRM_VERSION_CODE <= KERNEL_VERSION(3,10,52)
    ret = drm_pci_init(&gf_drm_driver.base, &gf_driver);
#endif

    return ret;
}

void gf_unregister_driver(void)
{
#if DRM_VERSION_CODE <= KERNEL_VERSION(3,10,52)
    drm_pci_exit(&gf_drm_driver.base, &gf_driver);
#endif

    pci_unregister_driver(&gf_driver);
}

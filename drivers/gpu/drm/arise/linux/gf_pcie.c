//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************

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

#if DRM_VERSION_CODE >= KERNEL_VERSION(5,5,0)
#if DRM_VERSION_CODE < KERNEL_VERSION(5,8,0)
#include <drm/drm_pci.h>
#else
#include <drm/drm_legacy.h>
#endif
#endif
#include <linux/irq.h>

#define DRIVER_DESC         "Glenfly DRM Pro"

struct gf_device_info {
    u32 display_mmio_offset;
    u32 intrupte_enable_regs;
};

/*
 * A pci_device_id struct {
 *	__u32 vendor, device;
 *      __u32 subvendor, subdevice;
 *	__u32 class, class_mask;
 *	kernel_ulong_t driver_data;
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
    {0x6766, 0x3D00, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (kernel_ulong_t)&gf_e3k_info}, //arise
    {0x6766, 0x3D02, PCI_ANY_ID, PCI_ANY_ID, 0, 0, (kernel_ulong_t)&gf_e3k_info}, //arise1020
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

static int gf_drm_suspend(struct drm_device *dev, pm_message_t state)
{
    gf_card_t* gf = dev->dev_private;
    int ret;

    gf_info("gfx driver suspending, pm event = %d.\n", state.event);
    disp_suspend(dev);
    gf_info("drm suspend: save display status finished.\n");
    
//#ifndef __mips__
    if(gf->gfb_enable)
    {
        gf_fbdev_set_suspend(gf, 1);
        gf_info("drm suspend: save drmfb status finished.\n");
    }
    
    ret = gf_core_interface->save_state(gf->adapter, state.event == PM_EVENT_FREEZE);

    if (ret)
    {
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

#ifdef _DEBUG_
    {
       struct irq_desc *desc = irq_to_desc(dev->irq);
       gf_info("current irq inprogress state %d\n", irqd_irq_inprogress(&desc->irq_data));

    }
#endif

    synchronize_irq(dev->irq);
    gf_info("drm suspend: disable irq finished.\n");

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    disp_vblank_save(dev);
#endif

    pci_save_state(dev->pdev);

    if (state.event == PM_EVENT_SUSPEND)
    {
        /*patch for CHX001 A0 and B0, pci_disable_device will clear master bit PCI04[2], and this bit can't be cleared for chx001 hw, so don't call pci_disable_device here*/
        pci_disable_device(dev->pdev);
        pci_set_power_state(dev->pdev, PCI_D3hot);
    }
//#endif

    return 0;
}

static int gf_drm_resume(struct drm_device *dev)
{
    gf_card_t* gf = dev->dev_private;
    int         ret = 0;

    gf_info("gfx driver resume back.\n");

//#ifndef __mips__
    pci_set_power_state(dev->pdev, PCI_D0);

    pci_restore_state(dev->pdev);

    if (pci_enable_device(dev->pdev))
    {
        return -1;
    }
    pci_set_master(dev->pdev);

    disp_pre_resume(dev);
    gf_info("drm resume: enable and post chip finished.\n");

#if DRM_VERSION_CODE < KERNEL_VERSION(4, 8, 0)
    disp_vblank_restore(dev);
#endif
    
    tasklet_enable(&gf->fence_notify);
    gf_enable_interrupt(gf->pdev);
    gf_info("drm resume: re-enable irq finished.\n");

    ret = gf_core_interface->restore_state(gf->adapter);

    if(ret != 0)
    {
        return -1;
    }

    if(gf->gfb_enable)
    {
        gf_fbdev_set_suspend(gf, 0);
        gf_info("drm resume: restore drmfb status finished.\n");
    }

    disp_post_resume(dev);
    gf_info("drm resume: restore display status finished.\n");
//#endif

    return 0;
}
static __inline__ void gf_init_adapter_info_by_params(struct krnl_adapter_init_info_s *info, struct gf_params *p)
{
    info->gf_pwm_mode                   = p->gf_pwm_mode;
    info->gf_dfs_mode                   = p->gf_dfs_mode;
    info->gf_worker_thread_enable       = p->gf_worker_thread_enable;
    info->gf_recovery_enable            = p->gf_recovery_enable;
    info->gf_hang_dump                  = p->gf_hang_dump;
    info->gf_force_3dblt                = p->gf_force_3dblt;
    info->gf_run_on_qt                  = p->gf_run_on_qt;
    info->gf_flag_buffer_verify         = p->gf_flag_buffer_verify;
    info->gf_debug_secure               = p->gf_debug_secure;
    info->gf_one_shot_enable            = p->gf_one_shot_enable;
    info->gf_hotplug_polling_enable     = p->gf_hotplug_polling_enable;
    info->gf_reboot_patch               = p->gf_reboot_patch;
    info->gf_vesa_tempbuffer_enable     = p->gf_vesa_tempbuffer_enable;

    info->miu_channel_size              = p->miu_channel_size;
    info->chip_slice_mask               = p->chip_slice_mask;
    info->gf_local_size_g               = p->gf_local_size_g;
    info->gf_local_size_m               = p->gf_local_size_m;
    info->gf_pcie_size_g                = p->gf_pcie_size_g;
    info->gf_pcie_size_m                = p->gf_pcie_size_m;
    info->debugfs_mask                  = p->debugfs_mask;
}

#define PCI_EN_IO_SPACE     1
static int gf_drm_load(struct drm_device *dev, unsigned long flags)
{
    struct pci_dev *pdev = dev->pdev;
    struct device* device = &pdev->dev;
    gf_card_t  *gf = NULL;
    int         ret = 0;
    unsigned short command = 0;
  //TODO. add some KMS related reg info to device info
  //  struct gf_device_info *match_info = (struct gf_device_info *)flags;

#if DRM_VERSION_CODE > KERNEL_VERSION(3,10,52)
    struct dev_pm_ops  *new_pm_ops;

//    drm_debug = 1;

    /* since if drm driver not support DRIVER_MODESET,   it won't support resume from S4, 
        *  because drm_class_dev_pm_ops not set .restore field and won't call into our gf_drm_resume callback;
        *
        *  so, we should hook the drm_class_dev_pm_ops structure, set  it's .restore field ;
        *  anyway, it's a stupid way to do this
        */
    if (dev && dev->primary && 
        dev->primary->kdev && 
        dev->primary->kdev->class && 
        dev->primary->kdev->class->pm && 
        !dev->primary->kdev->class->pm->restore)
    {
        new_pm_ops = gf_calloc(sizeof(*new_pm_ops));

        gf_memcpy(new_pm_ops, dev->primary->kdev->class->pm, sizeof(*new_pm_ops));

        new_pm_ops->restore = new_pm_ops->resume;

        new_pm_ops->thaw = new_pm_ops->resume;

        dev->primary->kdev->class->pm = new_pm_ops;
    }
#endif
    gf = (gf_card_t *)gf_calloc(sizeof(gf_card_t));
    if (!gf) {
        gf_error("allocate failed for gfx card!\n");
        return -ENOMEM;
    }
    gf_memset(gf, 0, sizeof(gf_card_t));

    dev->dev_private = (void*)gf;

    gf->drm_dev = dev;
    //TODO..FIXME. need remove gf->index. 
    gf->index  = dev->primary->index;

    pci_set_drvdata(pdev, dev);

    dma_set_mask_and_coherent(device, DMA_BIT_MASK(40));

    gf_card_pre_init(gf, pdev);

    ret = pci_request_regions(pdev, gf_driver.name);
    if(ret)
    {
        gf_error("pci_request_regions() failed. ret:%x.\n", ret);
    }

    pci_set_master(pdev);

   //fpga only
#if 0
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

    gf_dma_fence_driver_init(gf->adapter, gf->fence_drv);
    
    gf_info("%s = %p, %s->pdev = %p, dev = %p, dev->primary = %p\n", STR(DRIVER_NAME), gf, STR(DRIVER_NAME), gf->pdev,  dev, dev ? dev->primary : NULL);

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

    priv = gf_calloc(sizeof(gf_file_t));
    if (!priv)
        return -ENOMEM;

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

    return 0;
}

static void gf_drm_preclose(struct drm_device *dev, struct drm_file *file)
{
}

static void gf_drm_postclose(struct drm_device *dev, struct drm_file *file)
{
    gf_file_t *priv = file->driver_priv;
    gf_card_t *gf  = priv->card;

    if(priv->hold_lock)
    {
        gf_mutex_unlock(gf->lock);
    }

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
}

void  gf_drm_last_close(struct drm_device* dev)
{
    gf_card_t *gf = dev->dev_private;
    struct gf_fbdev *fbdev = gf->fbdev;

    if(!fbdev)
    {
        return;
    }

    drm_fb_helper_restore_fbdev_mode_unlocked(&fbdev->helper);
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

static long gf_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret;
    unsigned int nr = GF_IOCTL_NR(cmd);
    unsigned int type = GF_IOCTL_TYPE(cmd);
    struct drm_file *file_priv = filp->private_data;

    if (type == GF_IOCTL_BASE)
    {
        ret = (long)(gf_ioctls[nr](file_priv->driver_priv, cmd, arg));

        return ret;
    }
    else
    {
        return drm_ioctl(filp, cmd, arg);
    }
}

//#if (defined(__x86_64__) || defined(__mips64__)  || defined(__loongarch__)) && defined(HAVE_COMPAT_IOCTL)
#if defined(CONFIG_COMPAT)
static long gf_compat_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    long ret;
    unsigned int nr = GF_IOCTL_NR(cmd);
    unsigned int type = GF_IOCTL_TYPE(cmd);
    struct drm_file *file_priv = filp->private_data;

    if (type == GF_IOCTL_BASE)
    {
        ret = (long)(gf_ioctls[nr](file_priv->driver_priv, cmd, arg));
    }
    else
    {
        if (nr < DRM_COMMAND_BASE) 
        {
            ret = drm_compat_ioctl(filp, cmd, arg);
        } 
        else 
        {
            ret = drm_ioctl(filp, cmd, arg);
        }
    }

    return ret;
}
#endif


static void gf_drm_unload(struct drm_device *dev)
{
    gf_card_t *gf = dev->dev_private;
    gf_info("drm unload.\n");

    if(gf == NULL) return;
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
    .unlocked_ioctl = gf_unlocked_ioctl,
#if defined(__x86_64__) && defined(HAVE_COMPAT_IOCTL)
    .compat_ioctl = gf_compat_ioctl,
#endif
    .mmap       = gf_drm_gem_mmap,
    .read       = drm_read,
    .poll       = drm_poll,
    .llseek     = noop_llseek,
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
        .load               = gf_drm_load,
        .open               = gf_drm_open,
    #if DRM_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)
        .unload             = gf_drm_unload,
    #endif
#if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
    #ifdef CONFIG_DRM_LEGACY
        .preclose           = gf_drm_preclose,
    #endif
#else
	.preclose           = gf_drm_preclose,
#endif

        .postclose          = gf_drm_postclose,
        .lastclose          = gf_drm_last_close,

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

    #if DRM_VERSION_CODE < KERNEL_VERSION(5,9,0)
        .gem_free_object    = gf_gem_free_object,
    #elif DRM_VERSION_CODE < KERNEL_VERSION(5,11,0)
        .gem_free_object_unlocked = gf_gem_free_object,
    #else
    #endif
        .prime_handle_to_fd = drm_gem_prime_handle_to_fd,
        .prime_fd_to_handle = gf_gem_prime_fd_to_handle,

    #if DRM_VERSION_CODE < KERNEL_VERSION(5,11,0)
        .gem_prime_export   = gf_gem_prime_export,
    #endif
        .gem_prime_import   = gf_gem_prime_import,

        .dumb_create        = gf_gem_dumb_create,
        .dumb_map_offset    = gf_gem_mmap_gtt,

        .irq_preinstall  = gf_irq_preinstall,
        .irq_postinstall = gf_irq_postinstall,
        .irq_handler     = gf_irq_handle,
        .irq_uninstall   = gf_irq_uninstall,


    #if DRM_VERSION_CODE < KERNEL_VERSION(5,12,0)
        .dumb_destroy       = drm_gem_dumb_destroy,
    #endif
        .name               = STR(DRIVER_NAME),
        .desc               = DRIVER_DESC,
        .date               = DRIVER_DATE,
        .major              = DRIVER_MAJOR,
        .minor              = DRIVER_MINOR,
        .patchlevel         = DRIVER_PATCHLEVEL,
    }
};

static int gf_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
#if DRM_VERSION_CODE >= KERNEL_VERSION(5,7,0)
    struct drm_device *dev;
    int ret;

    #if DRM_VERSION_CODE >= KERNEL_VERSION(5,11,0)
        #ifdef CONFIG_DRM_LEGACY
            INIT_LIST_HEAD(&gf_drm_driver.base.legacy_dev_list);
        #endif
    #else
            INIT_LIST_HEAD(&gf_drm_driver.base.legacy_dev_list);
    #endif

    dev = drm_dev_alloc(&gf_drm_driver.base, &pdev->dev);
    if (IS_ERR(dev))
            return PTR_ERR(dev);

    ret = pci_enable_device(pdev);
    if (ret)
            goto err_free;

    dev->pdev = pdev;

    pci_set_drvdata(pdev, dev);

    ret = drm_dev_register(dev, ent->driver_data);
    if (ret)
            goto err_pci;

    return 0;

err_pci:
    pci_disable_device(pdev);
err_free:
    drm_dev_put(dev);
    return ret;

#elif DRM_VERSION_CODE > KERNEL_VERSION(3,10,52)

    INIT_LIST_HEAD(&gf_drm_driver.base.legacy_dev_list);//init list manually, drm_get_pci_dev will use this list
    return drm_get_pci_dev(pdev, ent, &gf_drm_driver.base);
#else
    return 0;
#endif
}

#if DRM_VERSION_CODE >= KERNEL_VERSION(3,10,52)
static void __exit gf_pcie_cleanup(struct pci_dev *pdev)
{
    struct drm_device *dev = pci_get_drvdata(pdev);
    gf_info("pcie_cleanup.\n");
#if DRM_VERSION_CODE >= KERNEL_VERSION(4,14,0)
    drm_dev_unplug(dev);
#else
    gf_drm_unload(dev);
#endif
    drm_put_dev(dev);

    pci_set_drvdata(pdev, NULL);

    pci_release_regions(pdev);
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

    gf_info("pci device(vendor:0x%X, device:0x%X) pm resume\n", pdev->vendor, pdev->device);

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

    gf_info("pci device(vendor:0x%X, device:0x%X) pm thaw\n", pdev->vendor, pdev->device);

#if DRM_VERSION_CODE >= KERNEL_VERSION(4,0,0)
    gf_drm_resume(pci_get_drvdata(pdev));
#endif

    
    return 0;
}

static int gf_pmops_poweroff(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);

    gf_info("pci device(vendor:0x%X, device:0x%X) pm poweroff\n", pdev->vendor, pdev->device);
    
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

    gf_info("pci device(vendor:0x%X, device:0x%X) pm runtime_suspend\n", pdev->vendor, pdev->device);
    
    return 0;
}

static int gf_pmops_runtime_resume(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);

    gf_info("pci device(vendor:0x%X, device:0x%X) pm runtime_resume\n", pdev->vendor, pdev->device);
    
    return 0;
}
static int gf_pmops_runtime_idle(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);

    gf_info("pci device(vendor:0x%X, device:0x%X) pm runtime_idle\n", pdev->vendor, pdev->device);
    
    return 0;
}

static void gf_shutdown(struct pci_dev *pdev)
{
    struct drm_device *dev = pci_get_drvdata(pdev);
    gf_card_t *gf = dev->dev_private;

    gf_info("pci device(vendor:0x%X, device:0x%X) shutting down.\n", pdev->vendor, pdev->device);

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

void* gf_pci_map_rom(void *dev)
{
    size_t size;
    struct pci_dev  *pdev = (struct pci_dev*)dev;
#ifdef __aarch64__
    struct resource *res = &pdev->resource[PCI_ROM_RESOURCE];
    void            *virt_addr = NULL;

    if ((res == NULL) || (res->start == 0) || (res->start == ~0))
    {
        gf_info("invalid pci resource!\n");
        return NULL;
    }

    if (pci_enable_rom(pdev))
    {
        gf_info("enable pci rom failed!\n");
        return NULL;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
    virt_addr = ioremap((phys_addr_t)res->start, 0x10000);
#else
    virt_addr = ioremap_nocache((phys_addr_t)res->start, 0x10000);
#endif
    return virt_addr;

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
    if((src_image) && (*(unsigned short*)src_image == 0xaa55))
    {
        return src_image;
    }
    else
    {
        if (src_image)
        {
            gf_unmap_io_memory(vma);
            gf_write_rom_save_addr(pdev, rom_base);
        }
        
        return pci_map_rom(pdev, &size);
    }
#endif
}

void gf_pci_unmap_rom(void *dev, void* rom)
{
#ifdef __aarch64__
    struct pci_dev  *pdev = (struct pci_dev*)dev;
    struct resource *res = &pdev->resource[PCI_ROM_RESOURCE];
    if(rom)
    {
        iounmap(rom);
    }
    if ((res != NULL) && (!(res->flags & IORESOURCE_ROM_ENABLE)))
    {
        pci_disable_rom(pdev);
    }
#else
    if(rom)
    {
        pci_unmap_rom((struct pci_dev*)dev, rom);
    }
#endif

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

    gf_info("bus_command: 0x%x.\n", bus->command);

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

    gf_info("device id:%x\n",bus->device_id);



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

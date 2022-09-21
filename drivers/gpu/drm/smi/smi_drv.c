/*
 * Copyright 2016 SiliconMotion Inc.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License version 2. See the file COPYING in the main
 * directory of this archive for more details.
 *
 */
#include "smi_drv.h"
#include <linux/console.h>
#include <linux/module.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
#include <drm/drmP.h>
#else
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_ioctl.h>
#include <drm/drm_print.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
#include <drm/drm_pci.h>
#else
#include <linux/pci.h>
#endif
#include <drm/drm_vblank.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
#include <drm/drm_aperture.h>
#endif


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0)
#include <drm/drm_probe_helper.h>
#endif
#include <drm/drm_crtc_helper.h>

#include "smi_dbg.h"

#include "hw750.h"
#include "hw768.h"

#include "smi_sysfs.h"

int smi_modeset = -1;
int smi_indent = 0;
int smi_bpp = 32;
int force_connect = 0;
int g_specId = 0;
int smi_pat = 0xff;
int lvds_channel = 0;
int usb_host = 0;
int audio_en = 0;
int fixed_width = 0;
int fixed_height = 0;
int hwi2c_en = 0;
int swcur_en = 0;
int edid_mode = 1;
int smi_debug = 0;
int lcd_scale = 0;
int pwm_ctrl = 0;
int clk_phase = -1;

module_param(smi_pat, int, S_IWUSR | S_IRUSR);


MODULE_PARM_DESC(modeset, "Disable/Enable modesetting");
module_param_named(modeset, smi_modeset, int, 0400);
MODULE_PARM_DESC(bpp, "Max bits-per-pixel (default:32)");
module_param_named(bpp, smi_bpp, int, 0400);
MODULE_PARM_DESC(nopnp, "Force conncet to the monitor without monitor EDID (default:0) bit0:DVI,bit1:VGA,bit2:HDMI ");
module_param_named(nopnp, force_connect, int, 0400);
MODULE_PARM_DESC(lvds, "LVDS Channel, 0=disable 1=single channel, 2=dual channel (default:0)");
module_param_named(lvds, lvds_channel, int, 0400);
MODULE_PARM_DESC(width, "Fixed mode width for LVDS or nopnp (default:0)");
module_param_named(width, fixed_width, int, 0400);
MODULE_PARM_DESC(height, "Fixed mode height for LVDS or nopnp (default:0)");
module_param_named(height, fixed_height, int, 0400);
MODULE_PARM_DESC(audio, "SM768 Audio, 0=diable 1=use UDA1345 Codec, 2=use WM8978 Codec(default:0)");
module_param_named(audio, audio_en, int, 0400);
MODULE_PARM_DESC(hwi2c, "HW I2C for EDID reading, 0=SW I2C 1=HW I2C(default:0)");
module_param_named(hwi2c, hwi2c_en, int, 0400);
MODULE_PARM_DESC(swcur, "Use Software cursor, 0=HW Cursor 1=SW Cursor(default:0)");
module_param_named(swcur, swcur_en, int, 0400);
MODULE_PARM_DESC(edidmode, "Use Monitor EDID mode timing, 0 = Driver build-in mode timing 1 = Monitor EDID mode timing(default:1)");
module_param_named(edidmode, edid_mode, int, 0400);
MODULE_PARM_DESC(debug, "Driver debug log enable, 0 = disable 1 = enable (default:0)");
module_param_named(debug, smi_debug, int, 0400);
MODULE_PARM_DESC(lcdscale, "LCD(LVDS) scale  enable, 0 = disable 1 = enable (default:0)");
module_param_named(lcdscale, lcd_scale, int, 0400);
MODULE_PARM_DESC(pwm, "PWM Value, 0 = disable (default:0) bit 0-3: PWM 0/1/2 bit4-7: PWM Divider bit8-19: PWM Low Counter bit20-31: PWM High Counter");
module_param_named(pwm, pwm_ctrl, int, 0400);
MODULE_PARM_DESC(clkphase, "Panel Mode Clock phase, -1 = Use Mode table (Default)  0 = Negative 1 = Postive");
module_param_named(clkphase, clk_phase, int, 0400);



/*
 * This is the generic driver code. This binds the driver to the drm core,
 * which then performs further device association and calls our graphics init
 * functions
 */
#define PCI_VENDOR_ID_SMI 0x126f
#define PCI_DEVID_SM750 0x0750
#define PCI_DEVID_SM768 0x0768

static struct drm_driver driver;

/* only bind to the smi chip in qemu */
static const struct pci_device_id pciidlist[] = {
	{ PCI_VENDOR_ID_SMI, PCI_DEVID_SM750, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ PCI_VENDOR_ID_SMI, PCI_DEVID_SM768, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{
		0,
	}
};

static void claim(void)
{
	printk("+-------------SMI Driver Information------------+\n");
	printk("Release type: " RELEASE_TYPE "\n");
	printk("Driver version: v" _version_ "\n");
	printk("Support products: " SUPPORT_CHIP "\n");
	printk("+-----------------------------------------------+\n");
}


static int smi_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
	struct drm_device *dev;
#endif
	int ret __attribute__((unused)) = 0;

	claim();
	if (ent->vendor != PCI_VENDOR_ID_SMI &&
	    !(ent->device == PCI_DEVID_LYNX_EXP || ent->device == PCI_DEVID_SM768)) {
		return -ENODEV;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	drm_aperture_remove_conflicting_pci_framebuffers(pdev, &driver);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	drm_aperture_remove_conflicting_pci_framebuffers(pdev, "smidrmfb");
#else
	drm_fb_helper_remove_conflicting_pci_framebuffers(pdev, "smidrmfb");
#endif
#else
	drm_fb_helper_remove_conflicting_pci_framebuffers(pdev, 0, "smidrmfb");
#endif

	if (ent->vendor == PCI_VENDOR_ID_SMI && ent->device == PCI_DEVID_SM768) {
		
		if (lvds_channel != 0) {
			dbg_msg("LVDS channel set to %d\n", lvds_channel);
		}
	}

	smi_sysfs_init(&THIS_MODULE->mkobj.kobj);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
	ret = pci_enable_device(pdev);
	if (ret)
		return ret;

	dev = drm_dev_alloc(&driver, &pdev->dev);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		goto err_pci_disable_device;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 14, 0)
	dev->pdev = pdev;
#endif

	pci_set_drvdata(pdev, dev);

	ret = smi_driver_load(dev, ent->driver_data);
	if (ret)
		goto err_drm_dev_put;

	ret = drm_dev_register(dev, ent->driver_data);
	if (ret)
		goto err_smi_driver_unload;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
	if ((g_specId == SPC_SM750 && (pdev->resource[PCI_ROM_RESOURCE].flags & IORESOURCE_ROM_SHADOW)) || g_specId == SPC_SM768)
		drm_fbdev_generic_setup(dev, dev->mode_config.preferred_depth);
#endif

	return 0;

err_smi_driver_unload:
	smi_driver_unload(dev);
err_drm_dev_put:
	drm_dev_put(dev);
err_pci_disable_device:
	pci_disable_device(pdev);
	return ret;
#else
	return drm_get_pci_dev(pdev, ent, &driver);
#endif
}

static void smi_pci_remove(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);

	smi_sysfs_deinit(&THIS_MODULE->mkobj.kobj);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
	drm_dev_unregister(dev);
	smi_driver_unload(dev);
	drm_dev_put(dev);
#else
	drm_put_dev(dev);
#endif
}

static int smi_drm_freeze(struct drm_device *dev)
{
	int ret;
	struct smi_device *sdev = dev->dev_private;
	ENTER();
	
	if (g_specId == SPC_SM750)
		hw750_suspend(sdev->regsave);
    else if(g_specId == SPC_SM768){
		 if(audio_en)
			 smi_audio_suspend();
		hw768_suspend(sdev->regsave_768);

    }
	ret = drm_mode_config_helper_suspend(dev);
	if (ret)
		return ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	pci_save_state(to_pci_dev(dev->dev));
#else
    pci_save_state(dev->pdev);
#endif

	LEAVE(0);
}

static int smi_drm_thaw(struct drm_device *dev)
{
	struct smi_device *sdev = dev->dev_private;

	ENTER();
	
	
	if(g_specId == SPC_SM750)
			hw750_resume(sdev->regsave);
	else if(g_specId == SPC_SM768){
			hw768_resume(sdev->regsave_768);
		if(audio_en)
			smi_audio_resume();
		if (lvds_channel == 1)
			hw768_enable_lvds(1);
		else if (lvds_channel == 2) {
			hw768_enable_lvds(2);
			EnableDoublePixel(0);
		}
	}

	LEAVE(0);
	
	

}

static int smi_drm_resume(struct drm_device *dev)
{
	struct smi_device *UNUSED(sdev) = dev->dev_private;
	int ret;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	if (pci_enable_device(to_pci_dev(dev->dev)))
#else
    if (pci_enable_device(dev->pdev))
#endif	
		return -EIO;

	ret = smi_drm_thaw(dev);
	if (ret)
		return ret;

	return drm_mode_config_helper_resume(dev);
}

static int smi_pm_suspend(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *ddev = pci_get_drvdata(pdev);
	int error;

	error = smi_drm_freeze(ddev);
	if (error)
		return error;

	pci_disable_device(pdev);
	pci_set_power_state(pdev, PCI_D3hot);
	
	return 0;
}

static int smi_pm_resume(struct device *dev)
{
	struct drm_device *ddev = dev_get_drvdata(dev);

	return smi_drm_resume(ddev);
}

static int smi_pm_freeze(struct device *dev)
{
	struct drm_device *ddev = dev_get_drvdata(dev);

	return smi_drm_freeze(ddev);
}

static int smi_pm_thaw(struct device *dev)
{
	struct drm_device *ddev = dev_get_drvdata(dev);
	return smi_drm_thaw(ddev);
}

static int smi_pm_poweroff(struct device *dev)
{
	struct drm_device *ddev = dev_get_drvdata(dev);

	return smi_drm_freeze(ddev);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
static int smi_enable_vblank(struct drm_device *dev, unsigned int pipe)
{
	if (g_specId == SPC_SM750) {
		hw750_en_dis_interrupt(1, pipe);
	} else if (g_specId == SPC_SM768) {
		hw768_en_dis_interrupt(1, pipe);
	}
	return 0;
}

static void smi_disable_vblank(struct drm_device *dev, unsigned int pipe)
{
	if (g_specId == SPC_SM750) {
		hw750_en_dis_interrupt(0, pipe);
	} else if (g_specId == SPC_SM768) {
		hw768_en_dis_interrupt(0, pipe);
	}
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
static void smi_irq_preinstall(struct drm_device *dev)
{
	// To Do....
	/* Disable *all* interrupts */

	/* Clear bits if they're already high */
}

static int smi_irq_postinstall(struct drm_device *dev)
{
	return 0;
}

static void smi_irq_uninstall(struct drm_device *dev)
{
	/* Disable *all* interrupts */
	if (g_specId == SPC_SM750) {
		ddk750_disable_IntMask();
	} else if (g_specId == SPC_SM768) {
		ddk768_disable_IntMask();
	}
}
#endif

irqreturn_t smi_drm_interrupt(DRM_IRQ_ARGS)
{
	struct drm_device *dev = (struct drm_device *)arg;

	int handled = 0;

	if (g_specId == SPC_SM750) {
		if (hw750_check_vsync_interrupt(0)) {
			/* Clear the panel VSync Interrupt */
			drm_handle_vblank(dev, 0);
			handled = 1;
			hw750_clear_vsync_interrupt(0);
		}
		if (hw750_check_vsync_interrupt(1)) {
			drm_handle_vblank(dev, 1);
			handled = 1;
			hw750_clear_vsync_interrupt(1);
		}
	} else if (g_specId == SPC_SM768) {
		if (hw768_check_vsync_interrupt(0)) {
			/* Clear the panel VSync Interrupt */
			drm_handle_vblank(dev, 0);
			handled = 1;
			hw768_clear_vsync_interrupt(0);
		}
		if (hw768_check_vsync_interrupt(1)) {
			drm_handle_vblank(dev, 1);
			handled = 1;
			hw768_clear_vsync_interrupt(1);
		}
	}

	if (handled)
		return IRQ_HANDLED;
	return IRQ_NONE;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
static int smi_dumb_create_align(struct drm_file *file, struct drm_device *dev,
			     struct drm_mode_create_dumb *args)
{
	return drm_gem_vram_fill_create_dumb(file, dev, 0, 16, args);
}
#endif


static const struct dev_pm_ops smi_pm_ops = {
	.suspend = smi_pm_suspend,
	.resume = smi_pm_resume,
	.freeze = smi_pm_freeze,
	.thaw = smi_pm_thaw,
	.poweroff = smi_pm_poweroff,
	.restore = smi_pm_resume,
};

static struct pci_driver smi_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = pciidlist,
	.probe = smi_pci_probe,
	.remove = smi_pci_remove,
	.driver.pm = &smi_pm_ops,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
DEFINE_DRM_GEM_FOPS(smi_driver_fops);
#else
static const struct file_operations smi_driver_fops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	DRM_VRAM_MM_FILE_OPERATIONS,
#else
	.open = drm_open,
	.release = drm_release,
	.unlocked_ioctl = drm_ioctl,
	.mmap = smi_mmap,
	.poll = drm_poll,
#ifdef CONFIG_COMPAT
	.compat_ioctl = drm_compat_ioctl,
#endif
	.read = drm_read,
	.llseek = no_llseek,
#endif
	.owner = THIS_MODULE,
};
#endif

static struct drm_driver driver = {
	.driver_features = DRIVER_HAVE_IRQ | DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC
#if defined(PRIME) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
			   | DRIVER_PRIME
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)
			   | DRIVER_IRQ_SHARED
#endif
	,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 6, 0)
	.load = smi_driver_load,
	.unload = smi_driver_unload,
#endif
	.fops = &smi_driver_fops,
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
	.debugfs_init             = drm_vram_mm_debugfs_init,
	.dumb_create		  = smi_dumb_create_align,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	.dumb_map_offset		  = drm_gem_ttm_dumb_map_offset,
#else
	.dumb_map_offset		  = drm_gem_vram_driver_dumb_mmap_offset,
#endif
	.gem_prime_mmap		  = drm_gem_prime_mmap,
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 3, 0)
	 DRM_GEM_VRAM_DRIVER,
#else
	.gem_free_object_unlocked = smi_gem_free_object,
	.dumb_create = smi_dumb_create,
	.dumb_map_offset = smi_dumb_mmap_offset,
	.dumb_destroy = drm_gem_dumb_destroy,
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 5, 0)
	.enable_vblank = smi_enable_vblank,
	.disable_vblank = smi_disable_vblank,
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
	.irq_preinstall = smi_irq_preinstall,
	.irq_postinstall = smi_irq_postinstall,
	.irq_uninstall = smi_irq_uninstall,
	.irq_handler = smi_drm_interrupt,
#endif
	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 3, 0)
	.gem_prime_import = drm_gem_prime_import,
	.gem_prime_export = drm_gem_prime_export,
	.gem_prime_vmap = smi_gem_prime_vmap,
	.gem_prime_vunmap = smi_gem_prime_vunmap,
	.gem_prime_pin = smi_gem_prime_pin,
	.gem_prime_unpin = smi_gem_prime_unpin,
	.gem_prime_get_sg_table = smi_gem_prime_get_sg_table,
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0)
	DRM_GEM_VRAM_DRIVER_PRIME,
#endif
	.gem_prime_import_sg_table = smi_gem_prime_import_sg_table,
};

static int __init smi_init(void)
{
	if (vgacon_text_force() && smi_modeset == -1)
		return -EINVAL;

	if (smi_modeset == 0)
		return -EINVAL;
	return pci_register_driver(&smi_pci_driver);
}

static void __exit smi_exit(void)
{
	pci_unregister_driver(&smi_pci_driver);
}

module_init(smi_init);
module_exit(smi_exit);

MODULE_DEVICE_TABLE(pci, pciidlist);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
MODULE_IMPORT_NS(DMA_BUF);
#endif


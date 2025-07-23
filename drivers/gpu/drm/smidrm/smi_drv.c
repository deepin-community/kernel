// SPDX-License-Identifier: GPL-2.0+
// Copyright (c) 2023, SiliconMotion Inc.

#include "smi_drv.h"
#include <linux/console.h>
#include <linux/module.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
#include <drm/drm_fbdev_ttm.h>
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
#include <drm/drm_fbdev_generic.h>
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 3, 0)
#include <drm/drm_modeset_helper.h>
#endif

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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
#include <drm/drm_fbdev_shmem.h>
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
#include <linux/aperture.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 14, 0)
#include <drm/clients/drm_client_setup.h>
#else
#include <drm/drm_client_setup.h>
#endif
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
#include <drm/drm_aperture.h>
#endif


#include <drm/drm_gem_shmem_helper.h>

#include <drm/drm_probe_helper.h>
#include <drm/drm_crtc_helper.h>

#include "smi_dbg.h"

#include "hw750.h"
#include "hw768.h"
#include "hw770.h"

#include "smi_debugfs.h"

int smi_modeset = -1;
int smi_indent = 0;
int smi_bpp = 32;
int force_connect = 0;
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
int use_vblank = 0;
int use_doublebuffer = 0;

module_param(smi_pat, int, S_IWUSR | S_IRUSR);


MODULE_PARM_DESC(modeset, "Disable/Enable modesetting");
module_param_named(modeset, smi_modeset, int, 0400);
MODULE_PARM_DESC(bpp, "Max bits-per-pixel (default:32)");
module_param_named(bpp, smi_bpp, int, 0400);
MODULE_PARM_DESC(nopnp, "Force conncet to the monitor without monitor EDID (default:0) bit0:DVI,bit1:VGA,bit2:HDMI SM770: bit3-5 HDMI0-2, bit6-7 DP0-1");
module_param_named(nopnp, force_connect, int, 0400);
MODULE_PARM_DESC(lvds, "LVDS Channel, 0=disable 1=single channel, 2=dual channel (default:0)");
module_param_named(lvds, lvds_channel, int, 0400);
MODULE_PARM_DESC(width, "Fixed mode width for LVDS or nopnp (default:0)");
module_param_named(width, fixed_width, int, 0400);
MODULE_PARM_DESC(height, "Fixed mode height for LVDS or nopnp (default:0)");
module_param_named(height, fixed_height, int, 0400);
MODULE_PARM_DESC(audio, "SM768/SM770 Audio, 0=diable 1=use UDA1345 Codec, 2=use WM8978 Codec(default:0), SM770 only support HDMI/DP Audio");
module_param_named(audio, audio_en, int, 0400);
MODULE_PARM_DESC(hwi2c, "HW I2C for EDID reading for SM750/SM768, 0=SW I2C 1=HW I2C(default:0)");
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
MODULE_PARM_DESC(vblank, "Disable/Enable hw vblank support");
module_param_named(vblank, use_vblank, int, 0400);
MODULE_PARM_DESC(doublebuffer, "Disable/Enable double buffer");
module_param_named(doublebuffer, use_doublebuffer, int, 0400);

/*
 * This is the generic driver code. This binds the driver to the drm core,
 * which then performs further device association and calls our graphics init
 * functions
 */
#define PCI_VENDOR_ID_SMI 0x126f
#define PCI_DEVID_SM750 0x0750
#define PCI_DEVID_SM768 0x0768
#define PCI_DEVID_SM770 0x0770

static struct drm_driver driver;
unsigned int sm770_max_mode_size = (80<<20);  //if sram size is 256M, then max mode size is 80M. otherwise 160M

/* only bind to the smi chip in qemu */
static const struct pci_device_id pciidlist[] = {
	{ PCI_VENDOR_ID_SMI, PCI_DEVID_SM750, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ PCI_VENDOR_ID_SMI, PCI_DEVID_SM768, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ PCI_VENDOR_ID_SMI, PCI_DEVID_SM770, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
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
	struct drm_device *dev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
	struct smi_device *sdev;
#endif
	int ret __attribute__((unused)) = 0;

	claim();
	if (ent->vendor != PCI_VENDOR_ID_SMI &&
	    !(ent->device == PCI_DEVID_LYNX_EXP || ent->device == PCI_DEVID_SM768 || ent->device == PCI_DEVID_SM770)) {
		return -ENODEV;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
	aperture_remove_conflicting_pci_devices(pdev, driver.name);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	drm_aperture_remove_conflicting_pci_framebuffers(pdev, &driver);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	drm_aperture_remove_conflicting_pci_framebuffers(pdev, "smidrmfb");
#else
	drm_fb_helper_remove_conflicting_pci_framebuffers(pdev, "smidrmfb");
#endif
#else
	drm_fb_helper_remove_conflicting_pci_framebuffers(pdev, 0, "smidrmfb");
#endif

	if (ent->vendor == PCI_VENDOR_ID_SMI && (ent->device == PCI_DEVID_SM768 || ent->device == PCI_DEVID_SM770)) {
		
		if (lvds_channel != 0) {
			dbg_msg("LVDS channel set to %d\n", lvds_channel);
		}
	}


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
	sdev = dev->dev_private;
	if ((sdev->specId == SPC_SM750 && (pdev->resource[PCI_ROM_RESOURCE].flags & IORESOURCE_ROM_SHADOW)) || sdev->specId == SPC_SM768 || sdev->specId == SPC_SM770)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
		drm_client_setup(dev, NULL);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6, 11, 0)
		drm_fbdev_shmem_setup(dev, dev->mode_config.preferred_depth);
#else
		drm_fbdev_generic_setup(dev, dev->mode_config.preferred_depth);
#endif
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
	ret =  drm_get_pci_dev(pdev, ent, &driver);
	dev = pci_get_drvdata(pdev);
	smi_debugfs_init(dev->primary);
	return ret;
#endif
}

static void smi_pci_remove(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);



#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
	drm_dev_unregister(dev);
	smi_driver_unload(dev);
	drm_dev_put(dev);
#else
	drm_put_dev(dev);
#endif
}



static int smi_vram_suspend(struct smi_device *sdev,int vram_size)
{

	sdev->vram_save = kvmalloc(vram_size << 20,GFP_KERNEL);
	if (!sdev->vram_save)			
		goto malloc_failed;
	
	memcpy_fromio(sdev->vram_save, sdev->vram, vram_size << 20);

	return 0;
	
malloc_failed:
	if (sdev->vram_save) {			
		kvfree(sdev->vram_save); 		
		sdev->vram_save = NULL; 	
	}	
	return -ENOMEM;
}

static void smi_vram_resume(struct smi_device *sdev,int vram_size)
{
	if (sdev->vram_save) {		
		memcpy_toio(sdev->vram, sdev->vram_save, vram_size << 20);
		kvfree(sdev->vram_save); 		
		sdev->vram_save = NULL; 
	}
}


static int smi_drm_freeze(struct drm_device *dev)
{
	int ret;
	struct smi_device *sdev = dev->dev_private;
	ENTER();
	
	if (sdev->specId == SPC_SM750){
		smi_vram_suspend(sdev,16);
		hw750_suspend(sdev->regsave);
	}else if(sdev->specId == SPC_SM768){
#ifndef NO_AUDIO
		if(audio_en)
			 smi_audio_suspend(sdev);
#endif
		smi_vram_suspend(sdev,32);
		hw768_suspend(sdev->regsave_768);
    }else if(sdev->specId == SPC_SM770){
#ifndef NO_AUDIO
		if(audio_en)
			 smi_audio_suspend(sdev);
#endif

		hw770_suspend(sdev->regsave_770);
		hw770_HDMI_Disable_Output(0);
		hw770_HDMI_Disable_Output(1);
		hw770_HDMI_Disable_Output(2);

    }
	ret = drm_mode_config_helper_suspend(dev);
	if (ret)
		return ret;


	pci_save_state(to_pci_dev(dev->dev));

	LEAVE(0);
}

static int smi_drm_thaw(struct drm_device *dev)
{
	struct smi_device *sdev = dev->dev_private;

	ENTER();
	
	
	
	if(sdev->specId == SPC_SM750){
		smi_vram_resume(sdev,16);
		hw750_resume(sdev->regsave);
	}else if(sdev->specId == SPC_SM768){
		smi_vram_resume(sdev,32);
		hw768_resume(sdev->regsave_768);
#ifndef NO_AUDIO
		if(audio_en)
			smi_audio_resume(sdev);
#endif
		if(sdev->m_connector & USE_DVI){
			if (lvds_channel == 1){
				hw768_enable_lvds(1);
				DisableDoublePixel(0);
			}
			else if (lvds_channel == 2) {
				hw768_enable_lvds(2);
				EnableDoublePixel(0);
			}
		}
	}else if(sdev->specId == SPC_SM770){
				hw770_resume(sdev->regsave_770);
#ifndef NO_AUDIO
				if(audio_en)
					smi_audio_resume(sdev);
#endif

	}
	

	LEAVE(0);
	
	

}

static int smi_drm_resume(struct drm_device *dev)
{
	int ret;
	struct pci_dev *pdev = to_pci_dev(dev->dev);

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
    if (pci_enable_device(pdev))
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
	struct smi_device *sdev = dev->dev_private;
	
	if (sdev->specId == SPC_SM750) {
		hw750_en_dis_interrupt(1, pipe);
	} else if (sdev->specId == SPC_SM768) {
		hw768_en_dis_interrupt(1, pipe);
	}else if (sdev->specId == SPC_SM770) {
		hw770_en_dis_interrupt(1, pipe);
	}
	return 0;
}

static void smi_disable_vblank(struct drm_device *dev, unsigned int pipe)
{
	struct smi_device *sdev = dev->dev_private;
	
	if (sdev->specId == SPC_SM750) {
		hw750_en_dis_interrupt(0, pipe);
	} else if (sdev->specId == SPC_SM768) {
		hw768_en_dis_interrupt(0, pipe);
	} else if (sdev->specId == SPC_SM770) {
		hw770_en_dis_interrupt(0, pipe);
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
	struct smi_device *sdev = dev->dev_private;

	/* Disable *all* interrupts */
	if (sdev->specId == SPC_SM750) {
		ddk750_disable_IntMask();
	} else if (sdev->specId == SPC_SM768) {
		ddk768_disable_IntMask();
	} else if (sdev->specId == SPC_SM770) {
		ddk770_disable_IntMask();
	}
}
#endif

irqreturn_t smi_drm_interrupt(DRM_IRQ_ARGS)
{
	struct drm_device *dev = (struct drm_device *)arg;
	int handled = 0;
	struct smi_device *sdev = dev->dev_private;

	if (sdev->specId == SPC_SM750) {
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
	} else if (sdev->specId == SPC_SM768) {
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
	} else if(sdev->specId == SPC_SM770){
		if (hw770_check_vsync_interrupt(0)) {
			/* Clear the panel VSync Interrupt */
			drm_handle_vblank(dev, 0);
			handled = 1;
			hw770_clear_vsync_interrupt(0);
		}
		if (hw770_check_vsync_interrupt(1)) {
			drm_handle_vblank(dev, 1);
			handled = 1;
			hw770_clear_vsync_interrupt(1);
		}
	}

	if (handled)
		return IRQ_HANDLED;
	return IRQ_NONE;
}


irqreturn_t smi_hdmi0_hardirq(int irq, void *dev_id)
{
	int ret;

	ret = hw770_check_pnp_interrupt(0);
	if(!ret)
		return IRQ_NONE;
	else if (ret == HDMI_INT_HPD)
	{
		return IRQ_WAKE_THREAD;
	}

	return IRQ_HANDLED;
}

irqreturn_t smi_hdmi1_hardirq(int irq, void *dev_id)
{
	int ret;

	ret = hw770_check_pnp_interrupt(1);
	if(!ret)
		return IRQ_NONE;
	else if (ret == HDMI_INT_HPD)
	{	
		return IRQ_WAKE_THREAD;
	}

	return IRQ_HANDLED;
}

irqreturn_t smi_hdmi2_hardirq(int irq, void *dev_id)
{
	int ret;

	ret = hw770_check_pnp_interrupt(2);
	if(!ret)
		return IRQ_NONE;
	else if (ret == HDMI_INT_HPD)
	{	
		return IRQ_WAKE_THREAD;
	}

	return IRQ_HANDLED;
}


irqreturn_t smi_hdmi0_pnp_handler(int irq, void *dev_id)
{
	struct drm_display_mode *mode;
	struct smi_device *sdev;
	struct drm_device *dev;
	logicalMode_t logicalMode;
	unsigned long refresh_rate;
	struct drm_crtc *crtc0;
	struct smi_770_fb_info fb_info = {0};
	int monitor_status = 0;
	int ret = 0;

	dev = dev_id;
	sdev = dev->dev_private;
	monitor_status = hw770_hdmi_detect(0);
	if(!monitor_status)
	{
		drm_kms_helper_hotplug_event(dev);
		return IRQ_HANDLED;
	}
	crtc0 = sdev->smi_enc_tab[2]->crtc;
	if (crtc0) {
		dbg_msg("CRTC0 found: %p\n", crtc0);
	} else {
		dbg_msg("CRTC0 not found\n");
		goto error;
	}

	mode = &crtc0->state->adjusted_mode;

	if(!mode)
		goto error;

	refresh_rate = drm_mode_vrefresh(mode);
	hw770_get_current_fb_info(0,&fb_info);

	logicalMode.valid_edid = false;
	if (sdev->hdmi0_edid && drm_edid_header_is_valid((u8 *)sdev->hdmi0_edid) == 8)
		logicalMode.valid_edid = true;

	logicalMode.x = mode->hdisplay;
	logicalMode.y = mode->vdisplay;
	logicalMode.bpp = smi_bpp;
	logicalMode.hz = refresh_rate;
	logicalMode.pitch = 0;
	logicalMode.dispCtrl = 0;

	if (monitor_status)
	{
		dbg_msg("HDMI0: Monitor status is $%d\n", monitor_status);
		hw770_setMode(&logicalMode, *mode);
		ret = hw770_set_hdmi_mode(&logicalMode, *mode, sdev->is_hdmi[0], INDEX_HDMI0);
		if (ret != 0)
		{
			dbg_msg("HDMI Mode not supported!\n");
			goto error;
		}
		hw770_set_current_pitch((disp_control_t)INDEX_HDMI0,&fb_info);
	}
	return IRQ_HANDLED;
error:
	return IRQ_HANDLED;

}

irqreturn_t smi_hdmi1_pnp_handler(int irq, void *dev_id)
{
	struct drm_display_mode *mode;
	struct smi_device *sdev;
	struct drm_device *dev;
	logicalMode_t logicalMode;
	unsigned long refresh_rate;
	struct drm_crtc *crtc1;
	struct smi_770_fb_info fb_info = {0};

	int monitor_status = 0;
	int ret = 0;

	dev = dev_id;
	sdev = dev->dev_private;
	monitor_status = hw770_hdmi_detect(1);
	if(!monitor_status)
	{
		drm_kms_helper_hotplug_event(dev);
		return IRQ_HANDLED;
	}
	crtc1 = sdev->smi_enc_tab[3]->crtc;
	if (crtc1) {
		dbg_msg("CRTC1 found: %p\n", crtc1);
	} else {
		dbg_msg("CRTC1 not found\n");
		goto error;
	}


	mode = &crtc1->state->adjusted_mode;

	if(!mode)
		goto error;

	refresh_rate = drm_mode_vrefresh(mode);
	hw770_get_current_fb_info(1,&fb_info);
	logicalMode.valid_edid = false;
	if (sdev->hdmi1_edid && drm_edid_header_is_valid((u8 *)sdev->hdmi1_edid) == 8)
		logicalMode.valid_edid = true;

	logicalMode.x = mode->hdisplay;
	logicalMode.y = mode->vdisplay;
	logicalMode.bpp = smi_bpp;
	logicalMode.hz = refresh_rate;
	logicalMode.pitch = 0;
	logicalMode.dispCtrl = 1;

	if (monitor_status)
	{
		dbg_msg("HDMI1: Monitor status is $%d\n", monitor_status);
		hw770_setMode(&logicalMode, *mode);
		ret = hw770_set_hdmi_mode(&logicalMode, *mode, sdev->is_hdmi[1], INDEX_HDMI1);
		if (ret != 0)
		{
			dbg_msg("HDMI Mode not supported!\n");
			goto error;
		}
		hw770_set_current_pitch((disp_control_t)INDEX_HDMI1,&fb_info);
	}
	return IRQ_HANDLED;
error:
	return IRQ_HANDLED;

}

irqreturn_t smi_hdmi2_pnp_handler(int irq, void *dev_id)
{
	struct drm_display_mode *mode;
	struct smi_device *sdev;
	struct drm_device *dev;
	logicalMode_t logicalMode;
	unsigned long refresh_rate;
	struct drm_crtc *crtc2;
	struct smi_770_fb_info fb_info = {0};
	int monitor_status = 0;
	int ret = 0;

	dev = dev_id;
	sdev = dev->dev_private;
	monitor_status = hw770_hdmi_detect(2);
	if(!monitor_status)
	{
		drm_kms_helper_hotplug_event(dev);
		return IRQ_HANDLED;
	}
	crtc2 = sdev->smi_enc_tab[4]->crtc;
	if (crtc2) {
		dbg_msg("CRTC2 found: %p\n", crtc2);
	} else {
		dbg_msg("CRTC2 not found\n");
		goto error;
	}


	mode = &crtc2->state->adjusted_mode;

	if(!mode)
		goto error;

	refresh_rate = drm_mode_vrefresh(mode);
	hw770_get_current_fb_info(2,&fb_info);
	logicalMode.valid_edid = false;
	if (sdev->hdmi2_edid && drm_edid_header_is_valid((u8 *)sdev->hdmi2_edid) == 8)
		logicalMode.valid_edid = true;

	logicalMode.x = mode->hdisplay;
	logicalMode.y = mode->vdisplay;
	logicalMode.bpp = smi_bpp;
	logicalMode.hz = refresh_rate;
	logicalMode.pitch = 0;
	logicalMode.dispCtrl = 2;


	if (monitor_status)
	{
		dbg_msg("HDMI2: Monitor status is $%d\n", monitor_status);
		hw770_setMode(&logicalMode, *mode);
		ret = hw770_set_hdmi_mode(&logicalMode, *mode, sdev->is_hdmi[2], INDEX_HDMI2);
		if (ret != 0)
		{
			dbg_msg("HDMI Mode not supported!\n");
			goto error;
		}
		hw770_set_current_pitch((disp_control_t)INDEX_HDMI2,&fb_info);
	}
	return IRQ_HANDLED;
error:
	return IRQ_HANDLED;

}



static int smi_dumb_create_align(struct drm_file *file, struct drm_device *dev,
	struct drm_mode_create_dumb *args)
{
struct smi_device *sdev = dev->dev_private;
if(sdev->specId == SPC_SM770){
args->width = alignLineOffset(args->width * args->bpp / 8) / (args->bpp / 8);
}
else
args->width = ALIGN (args->width , 8);	
//printk("smi_dumb_create_align args->width:%d args->height:%d\n",args->width,args->height);
return drm_gem_shmem_dumb_create(file, dev,  args);
}



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
DEFINE_DRM_GEM_SHMEM_FOPS(smi_driver_fops);
#endif



static struct drm_driver driver = {
	.driver_features = DRIVER_HAVE_IRQ | DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
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
#if LINUX_VERSION_CODE > KERNEL_VERSION(5, 7, 0) && LINUX_VERSION_CODE < KERNEL_VERSION(5, 11, 0)
	.gem_create_object = drm_gem_shmem_create_object_cached,
#endif
	.dumb_create		  = smi_dumb_create_align,
#if 0
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 14, 0)
	.dumb_map_offset		  = drm_gem_ttm_dumb_map_offset,
#else
	.dumb_map_offset		  = drm_gem_vram_driver_dumb_mmap_offset,
#endif
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	.gem_prime_mmap		  = drm_gem_prime_mmap,
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
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,
#endif
	.gem_prime_import_sg_table = drm_gem_shmem_prime_import_sg_table,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
	.debugfs_init = smi_debugfs_init,
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
	DRM_FBDEV_TTM_DRIVER_OPS,
	DRM_FBDEV_SHMEM_DRIVER_OPS,
#endif

};

static int __init smi_init(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 17, 0)
        if (drm_firmware_drivers_only() || !smi_modeset)
#else
        if (vgacon_text_force() || !smi_modeset)
#endif
		return -ENODEV;

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
MODULE_VERSION(_version_);

#ifdef RHEL_MAJOR
#if RHEL_MAJOR == 9
MODULE_IMPORT_NS(DMA_BUF);
#endif
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 16, 0)
#ifndef RHEL_MAJOR
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 13, 0)
MODULE_IMPORT_NS("DMA_BUF");
#else
MODULE_IMPORT_NS(DMA_BUF);
#endif
#endif
#endif

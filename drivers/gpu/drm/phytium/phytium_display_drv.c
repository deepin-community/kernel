// SPDX-License-Identifier: GPL-2.0
/* Phytium X100 display drm driver
 *
 * Copyright (c) 2021 Phytium Limited.
 *
 * Author:
 *	Yang Xun <yangxun@phytium.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <drm/drm_atomic_helper.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_crtc_helper.h>
#include <drm/drm_gem.h>
#include <linux/atomic.h>
#include <drm/drm_atomic.h>
#include <linux/workqueue.h>
#include "phytium_display_drv.h"
#include "phytium_reg.h"
#include "phytium_plane.h"
#include "phytium_crtc.h"
#include "phytium_dp.h"
#include "phytium_gem.h"
#include "phytium_fb.h"
#include "phytium_fbdev.h"

static int dc_msi_enable;
module_param(dc_msi_enable, int, 0644);
MODULE_PARM_DESC(dc_msi_enable, "Enable DC msi interrupt (0-disabled; 1-enabled; default-0)");

int dc_fake_mode_enable;
module_param(dc_fake_mode_enable, int, 0644);
MODULE_PARM_DESC(dc_fake_mode_enable, "Enable DC fake mode (0-disabled; 1-enabled; default-0)");

int dc_fast_training_check = 1;
module_param(dc_fast_training_check, int, 0644);
MODULE_PARM_DESC(dc_fast_training_check, "Check dp fast training (0-disabled; 1-enabled; default-1)");

int num_source_rates = 4;
module_param(num_source_rates, int, 0644);
MODULE_PARM_DESC(num_source_rates, "set the source max rates (1-1.62Gbps; 2-2.7Gbps; 3-5.4Gbps; 4-8.1Gbps; default-4)");

int source_max_lane_count = 4;
module_param(source_max_lane_count, int, 0644);
MODULE_PARM_DESC(source_max_lane_count, "set the source lane count (1-1lane; 2-2lane; 4-4lane; default-4)");

int link_dynamic_adjust = 1;
module_param(link_dynamic_adjust, int, 0644);
MODULE_PARM_DESC(link_dynamic_adjust, "dynamic select the train pamameter according to the display mode (0-disabled; 1-enabled; default-1)");

void phytium_irq_preinstall(struct drm_device *dev)
{
	struct phytium_display_drm_private *priv = dev->dev_private;
	int i, status;

	for_each_pipe_masked(priv, i) {
		status = phytium_readl_reg(priv, PHYTIUM_DC_INT_STATUS(i));
		phytium_writel_reg(priv, INT_DISABLE, PHYTIUM_DC_INT_ENABLE(i));
	}
}

static void phytium_irq_uninstall(struct drm_device *dev)
{
	struct phytium_display_drm_private *priv = dev->dev_private;
	int i, status;

	for_each_pipe_masked(priv, i) {
		status = phytium_readl_reg(priv, PHYTIUM_DC_INT_STATUS(i));
		phytium_writel_reg(priv, INT_DISABLE, PHYTIUM_DC_INT_ENABLE(i));
	}
}

static irqreturn_t phytium_display_irq_handler(int irq, void *data)
{
	struct drm_device *dev = data;
	struct phytium_display_drm_private *priv = dev->dev_private;
	int status = 0;
	int i = 0, virt_pipe = 0;
	irqreturn_t ret = IRQ_NONE, ret1 = IRQ_NONE;

	for_each_pipe_masked(priv, i) {
		status = phytium_readl_reg(priv, PHYTIUM_DC_INT_STATUS(i));

		if (status & INT_STATUS) {
			virt_pipe = phytium_get_virt_pipe(priv, i);
			if (virt_pipe < 0)
				return IRQ_NONE;
			drm_handle_vblank(dev, virt_pipe);
			ret = IRQ_HANDLED;
			phytium_writel_reg(priv, MSI_CLEAR, PHYTIUM_DCREQ_MSI_CLEAR(i));
		}
	}

	ret1 = phytium_dp_hpd_irq_handler(priv);
	if (ret == IRQ_HANDLED || ret1 == IRQ_HANDLED)
		return IRQ_HANDLED;

	return IRQ_NONE;
}

static int phytium_enable_vblank(struct drm_device *dev, unsigned int virt_pipe)
{
	struct phytium_display_drm_private *priv = dev->dev_private;
	int phys_pipe;

	phys_pipe = phytium_get_phys_pipe(priv, virt_pipe);
	if (phys_pipe < 0)
		return phys_pipe;
	phytium_writel_reg(priv, INT_ENABLE, PHYTIUM_DC_INT_ENABLE(phys_pipe));

	return 0;
}

static void phytium_disable_vblank(struct drm_device *dev, unsigned int virt_pipe)
{
	struct phytium_display_drm_private *priv = dev->dev_private;
	int phys_pipe;

	phys_pipe = phytium_get_phys_pipe(priv, virt_pipe);
	if (phys_pipe >= 0)
		phytium_writel_reg(priv, INT_DISABLE, PHYTIUM_DC_INT_ENABLE(phys_pipe));
}

static const struct vm_operations_struct phytium_vm_ops = {
	.open	= drm_gem_vm_open,
	.close	= drm_gem_vm_close,
};

static const struct drm_ioctl_desc phytium_ioctls[] = {
	/* for test, none so far */
};

static const struct file_operations phytium_drm_driver_fops = {
	.owner		= THIS_MODULE,
	.open		= drm_open,
	.release	= drm_release,
	.unlocked_ioctl	= drm_ioctl,
	.compat_ioctl	= drm_compat_ioctl,
	.poll		= drm_poll,
	.read		= drm_read,
	.llseek		= no_llseek,
	.mmap		= phytium_gem_mmap,
};

static struct drm_driver phytium_display_drm_driver = {
	.driver_features	= DRIVER_HAVE_IRQ   |
				  DRIVER_IRQ_SHARED |
				  DRIVER_PRIME      |
				  DRIVER_MODESET    |
				  DRIVER_ATOMIC     |
				  DRIVER_GEM,
	.lastclose		= drm_fb_helper_lastclose,
	.irq_handler		= phytium_display_irq_handler,
	.irq_preinstall		= phytium_irq_preinstall,
	.irq_uninstall		= phytium_irq_uninstall,
	.enable_vblank		= phytium_enable_vblank,
	.disable_vblank		= phytium_disable_vblank,
	.gem_free_object	= phytium_gem_free_object,
	.gem_vm_ops		= &phytium_vm_ops,
	.prime_handle_to_fd	= drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle	= drm_gem_prime_fd_to_handle,
	.gem_prime_export	= drm_gem_prime_export,
	.gem_prime_import	= drm_gem_prime_import,
	.gem_prime_get_sg_table	= phytium_gem_prime_get_sg_table,
	.gem_prime_import_sg_table = phytium_gem_prime_import_sg_table,
	.gem_prime_vmap		= phytium_gem_prime_vmap,
	.gem_prime_vunmap	= phytium_gem_prime_vunmap,
	.gem_prime_mmap		= phytium_gem_prime_mmap,
	.dumb_create		= phytium_gem_dumb_create,
	.dumb_destroy		= phytium_gem_dumb_destroy,
	.ioctls			= phytium_ioctls,
	.num_ioctls		= ARRAY_SIZE(phytium_ioctls),
	.fops			= &phytium_drm_driver_fops,
	.name			= DRV_NAME,
	.desc			= DRV_DESC,
	.date			= DRV_DATE,
	.major			= DRV_MAJOR,
	.minor			= DRV_MINOR,
};

static const struct drm_mode_config_funcs phytium_mode_funcs = {
	.fb_create		= phytium_fb_create,
	.output_poll_changed	= drm_fb_helper_output_poll_changed,
	.atomic_check		= drm_atomic_helper_check,
	.atomic_commit		= drm_atomic_helper_commit,
};

static void phytium_atomic_commit_tail(struct drm_atomic_state *state)
{
	struct drm_device *dev = state->dev;

	drm_atomic_helper_commit_modeset_disables(dev, state);
	drm_atomic_helper_commit_planes(dev, state, false);
	drm_atomic_helper_commit_modeset_enables(dev, state);
	drm_atomic_helper_commit_hw_done(state);
	drm_atomic_helper_wait_for_flip_done(dev, state);
	drm_atomic_helper_cleanup_planes(dev, state);
}

static struct drm_mode_config_helper_funcs phytium_mode_config_helpers = {
	.atomic_commit_tail = phytium_atomic_commit_tail,
};

static int phytium_modeset_init(struct drm_device *dev)
{
	struct phytium_display_drm_private *priv = dev->dev_private;
	int i = 0, ret;

	drm_mode_config_init(dev);
	dev->mode_config.min_width = 0;
	dev->mode_config.min_height = 0;
	dev->mode_config.max_width = 16384;
	dev->mode_config.max_height = 16384;
	dev->mode_config.cursor_width = 32;
	dev->mode_config.cursor_height = 32;

	dev->mode_config.preferred_depth = 24;
	dev->mode_config.prefer_shadow = 1;
	dev->mode_config.allow_fb_modifiers = true;

	dev->mode_config.funcs	= &phytium_mode_funcs;
	dev->mode_config.helper_private = &phytium_mode_config_helpers;

	for_each_pipe_masked(priv, i) {
		ret = phytium_crtc_init(dev, i);
		if (ret) {
			DRM_ERROR("phytium_crtc_init(pipe %d) return failed\n", i);
			goto failed_crtc_init;
		}
	}

	for_each_pipe_masked(priv, i) {
		ret = phytium_dp_init(dev, i);
		if (ret) {
			DRM_ERROR("phytium_dp_init(pipe %d) return failed\n", i);
			goto failed_dp_init;
		}
	}

	drm_mode_config_reset(dev);

	return 0;
failed_dp_init:
failed_crtc_init:
	drm_mode_config_cleanup(dev);
	return ret;
}

int phytium_get_virt_pipe(struct phytium_display_drm_private *priv, int phys_pipe)
{
	int i = 0;
	int virt_pipe = 0;

	for_each_pipe_masked(priv, i) {
		if (i != phys_pipe)
			virt_pipe++;
		else
			return virt_pipe;
	}

	DRM_ERROR("%s %d failed\n", __func__, phys_pipe);
	return -EINVAL;
}

int phytium_get_phys_pipe(struct phytium_display_drm_private *priv, int virt_pipe)
{
	int i = 0;
	int tmp = 0;

	for_each_pipe_masked(priv, i) {
		if (tmp != virt_pipe)
			tmp++;
		else
			return i;
	}

	DRM_ERROR("%s %d failed\n", __func__, virt_pipe);
	return -EINVAL;
}

static int phytium_display_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	struct phytium_display_drm_private *priv = NULL;
	struct drm_device *dev = NULL;
	struct phytium_device_info *phytium_info = (struct phytium_device_info *)ent->driver_data;
	int ret = 0, i = 0, config, timeout = 100;
	resource_size_t io_addr, io_size, vram_addr, vram_size;

	dev = drm_dev_alloc(&phytium_display_drm_driver, &pdev->dev);
	if (IS_ERR(dev)) {
		DRM_ERROR("failed to allocate drm_device\n");
		return PTR_ERR(dev);
	}

	dev->pdev = pdev;
	pci_set_drvdata(pdev, dev);
	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (IS_ERR(priv)) {
		DRM_ERROR("no memory to allocate for drm_display_private\n");
		goto failed_malloc_priv;
	}
	memset(priv, 0, sizeof(*priv));
	INIT_LIST_HEAD(&priv->gem_list_head);
	spin_lock_init(&priv->hotplug_irq_lock);
	dev->dev_private = priv;
	priv->dev = dev;
	memcpy(&(priv->info), phytium_info, sizeof(struct phytium_device_info));
	DRM_DEBUG_KMS("priv->info.num_pipes :%d\n", priv->info.num_pipes);
	priv->info.pipe_mask = ((pdev->subsystem_device >> PIPE_MASK_SHIFT) & PIPE_MASK_MASK);
	priv->info.edp_mask = ((pdev->subsystem_device >> EDP_MASK_SHIFT) & EDP_MASK_MASK);
	priv->info.num_pipes = 0;
	for_each_pipe_masked(priv, i)
		priv->info.num_pipes++;
	if (priv->info.num_pipes == 0) {
		DRM_ERROR("num_pipes is zero, so exit init\n");
		goto failed_init_numpipe;
	}

	pci_set_master(pdev);
	ret = pci_enable_device(pdev);
	if (ret) {
		DRM_ERROR("pci enbale device fail\n");
		goto failed_enable_device;
	}

	if (dc_msi_enable) {
		ret = pci_enable_msi(pdev);
		if (ret)
			DRM_ERROR("pci enbale msi fail\n");
	}
	io_addr = pci_resource_start(pdev, 0);
	io_size = pci_resource_len(pdev, 0);
	priv->regs = ioremap(io_addr, io_size);
	if (priv->regs == NULL) {
		DRM_ERROR("pci bar0 ioremap fail, addr:0x%llx, size:0x%llx\n", io_addr, io_size);
		goto failed_ioremap;
	}

	vram_addr = pci_resource_start(pdev, 2);
	vram_size = pci_resource_len(pdev, 2);

	/* reset dc/dp */
	for_each_pipe_masked(priv, i) {
		timeout = 100;
		phytium_writel_reg(priv, PHYTIUM_DC_DP_RESET_STATUS, 0);
		phytium_writel_reg(priv, CMD_DC_DP_RESET | FLAG_REQUEST,
				   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(i));
		do {
			mdelay(10);
			timeout--;
			config = phytium_readl_reg(priv, PHYTIUM_DC_DP_RESET_STATUS);
			if (config & DC_DP_RESET_STATUS(i))
				break;
		} while (timeout);
		if (timeout == 0) {
			DRM_ERROR("reset dc/dp pipe(%d) failed\n", i);
			goto failed_reset;
		}
	}

	if ((vram_addr != 0) && (vram_size != 0)) {
		DRM_DEBUG_KMS("vram_addr:0x%llx vram_size: 0x%llx\n", vram_addr, vram_size);
		ret = dma_declare_coherent_memory(dev->dev, vram_addr, vram_addr,
						  vram_size, DMA_MEMORY_EXCLUSIVE);
		if (ret) {
			DRM_ERROR("pci bar2 vram declare fail\n");
			goto failed_declare_memory;
		}
		phytium_writel_reg(priv, (vram_addr & SRC_ADDR_MASK) >> SRC_ADDR_OFFSET,
				   PHYTIUM_DC_ADDRESS_TRANSFORM_SRC_ADDR);
		phytium_writel_reg(priv, (vram_size >> SIZE_OFFSET) | ADDRESS_TRANSFORM_ENABLE,
				   PHYTIUM_DC_ADDRESS_TRANSFORM_SIZE);
		config = phytium_readl_reg(priv, PHYTIUM_DC_ADDRESS_TRANSFORM_DST_ADDR);
		phytium_writel_reg(priv, config, PHYTIUM_DC_ADDRESS_TRANSFORM_DST_ADDR);
		priv->vram_support = true;
	} else {
		DRM_DEBUG_KMS("not support vram\n");
		priv->vram_support = false;
	}

	ret = drm_vblank_init(dev, priv->info.num_pipes);
	if (ret) {
		DRM_ERROR("vblank init failed\n");
		goto failed_vblank_init;
	}

	ret = phytium_modeset_init(dev);
	if (ret) {
		DRM_ERROR("phytium_modeset_init failed\n");
		goto failed_modeset_init;
	}

	INIT_WORK(&priv->hotplug_work, phytium_dp_hpd_work_func);
	ret = drm_irq_install(dev, dev->pdev->irq);
	if (ret) {
		DRM_ERROR("install irq failed\n");
		goto failed_irq_install;
	}

	ret = drm_dev_register(dev, 0);
	if (ret) {
		DRM_ERROR("failed to register drm dev\n");
		goto failed_register_drm;
	}

	ret = phytium_drm_fbdev_init(dev);
	if (ret)
		DRM_ERROR("failed to init dev\n");

	phytium_dp_hpd_irq_setup(dev, true);

	return 0;

failed_register_drm:
	drm_irq_uninstall(dev);
failed_irq_install:
	drm_mode_config_cleanup(dev);
failed_modeset_init:
failed_vblank_init:
	dma_release_declared_memory(dev->dev);
failed_declare_memory:
failed_reset:
	iounmap(priv->regs);
failed_ioremap:
	if (pdev->msi_enabled)
		pci_disable_msi(pdev);
	pci_disable_device(pdev);
failed_enable_device:
failed_init_numpipe:
	devm_kfree(&pdev->dev, priv);
failed_malloc_priv:
	pci_set_drvdata(pdev, NULL);
	drm_dev_unref(dev);
	return -1;
}

static void phytium_display_remove(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct phytium_display_drm_private *priv = dev->dev_private;

	phytium_dp_hpd_irq_setup(dev, false);
	cancel_work_sync(&priv->hotplug_work);
	phytium_drm_fbdev_fini(dev);
	drm_dev_unregister(dev);
	drm_irq_uninstall(dev);
	drm_mode_config_cleanup(dev);
	dma_release_declared_memory(dev->dev);
	iounmap(priv->regs);
	if (pdev->msi_enabled)
		pci_disable_msi(pdev);
	pci_disable_device(pdev);
	devm_kfree(&pdev->dev, priv);
	pci_set_drvdata(pdev, NULL);
	drm_dev_unref(dev);
}

static void phytium_display_shutdown(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);
	struct phytium_display_drm_private *priv = dev->dev_private;

	drm_atomic_helper_shutdown(dev);
	phytium_dp_hpd_irq_setup(dev, false);
	cancel_work_sync(&priv->hotplug_work);
	phytium_drm_fbdev_fini(dev);
	drm_dev_unregister(dev);
	drm_irq_uninstall(dev);
	drm_mode_config_cleanup(dev);
	dma_release_declared_memory(dev->dev);
	iounmap(priv->regs);
	if (pdev->msi_enabled)
		pci_disable_msi(pdev);
	pci_disable_device(pdev);
	devm_kfree(&pdev->dev, priv);
	pci_set_drvdata(pdev, NULL);
	drm_dev_unref(dev);
}

static int phytium_display_pm_suspend(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);
	struct drm_atomic_state *state;
	struct phytium_display_drm_private *priv = drm_dev->dev_private;
	int ret, ret1;

	priv->save_reg[0] = phytium_readl_reg(priv, PHYTIUM_DC_ADDRESS_TRANSFORM_SRC_ADDR);
	priv->save_reg[1] = phytium_readl_reg(priv, PHYTIUM_DC_ADDRESS_TRANSFORM_SIZE);
	priv->save_reg[2] = phytium_readl_reg(priv, PHYTIUM_DC_ADDRESS_TRANSFORM_DST_ADDR);

	phytium_dp_hpd_irq_setup(drm_dev, false);
	cancel_work_sync(&priv->hotplug_work);
	drm_fb_helper_set_suspend_unlocked(drm_dev->fb_helper, 1);
	state = drm_atomic_helper_suspend(drm_dev);
	if (IS_ERR(state)) {
		DRM_ERROR("drm_atomic_helper_suspend failed: %ld\n", PTR_ERR(state));
		ret = PTR_ERR(state);
		goto suspend_failed;
	}
	drm_dev->mode_config.suspend_state = state;
	ret = phytium_gem_suspend(drm_dev);
	if (ret) {
		DRM_ERROR("phytium_gem_suspend failed: %d\n", ret);
		goto gem_suspend_failed;
	}

	return 0;

gem_suspend_failed:
	ret1 = drm_atomic_helper_resume(drm_dev, drm_dev->mode_config.suspend_state);
	if (ret1)
		DRM_ERROR("Failed to resume (%d)\n", ret1);
	drm_dev->mode_config.suspend_state = NULL;
suspend_failed:
	drm_fb_helper_set_suspend_unlocked(drm_dev->fb_helper, 0);
	phytium_dp_hpd_irq_setup(drm_dev, true);

	return ret;
}

static int phytium_display_pm_resume(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *drm_dev = pci_get_drvdata(pdev);
	struct phytium_display_drm_private *priv = drm_dev->dev_private;
	int ret = 0, i = 0, config, timeout = 100;
	struct phytium_dp_device *phytium_dp;
	struct drm_encoder *encoder;

	if (WARN_ON(!drm_dev->mode_config.suspend_state))
		return -EINVAL;

	/* reset dc/dp */
	for_each_pipe_masked(priv, i) {
		timeout = 100;
		phytium_writel_reg(priv, PHYTIUM_DC_DP_RESET_STATUS, 0);
		phytium_writel_reg(priv, CMD_DC_DP_RESET | FLAG_REQUEST,
				   PHYTIUM_DCREQ_PIX_CLOCK_CONFIG(i));
		do {
			mdelay(10);
			timeout--;
			config = phytium_readl_reg(priv, PHYTIUM_DC_DP_RESET_STATUS);
			if (config & DC_DP_RESET_STATUS(i))
				break;
		} while (timeout);
		if (timeout == 0) {
			DRM_ERROR("reset dc/dp pipe(%d) failed\n", i);
			return -EIO;
		}
	}

	phytium_gem_resume(drm_dev);
	phytium_writel_reg(priv, priv->save_reg[0], PHYTIUM_DC_ADDRESS_TRANSFORM_SRC_ADDR);
	phytium_writel_reg(priv, priv->save_reg[1], PHYTIUM_DC_ADDRESS_TRANSFORM_SIZE);
	phytium_writel_reg(priv, priv->save_reg[2], PHYTIUM_DC_ADDRESS_TRANSFORM_DST_ADDR);

	drm_for_each_encoder(encoder, drm_dev) {
		phytium_dp = encoder_to_dp_device(encoder);
		phytium_dp->funcs->dp_hw_init(phytium_dp);
	}

	ret = drm_atomic_helper_resume(drm_dev, drm_dev->mode_config.suspend_state);
	if (ret) {
		DRM_ERROR("Failed to resume (%d)\n", ret);
		return ret;
	}

	drm_dev->mode_config.suspend_state = NULL;
	drm_fb_helper_set_suspend_unlocked(drm_dev->fb_helper, 0);
	phytium_dp_hpd_irq_setup(drm_dev, true);

	return 0;
}

static const struct dev_pm_ops phytium_display_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(phytium_display_pm_suspend, phytium_display_pm_resume)
};

static const struct phytium_device_info phytium_pipe_info = {
	.total_pipes = 3,
};

static const struct pci_device_id phytium_display_ids[] = {
	{ PCI_VDEVICE(PHYTIUM, 0xdc22), (kernel_ulong_t)&phytium_pipe_info },
	{ /* End: all zeroes */ }
};
MODULE_DEVICE_TABLE(pci, phytium_display_ids);

static struct pci_driver phytium_display_driver = {
	.name = "phytium_display",
	.id_table = phytium_display_ids,
	.probe = phytium_display_probe,
	.remove = phytium_display_remove,
	.shutdown = phytium_display_shutdown,
	.driver.pm = &phytium_display_pm_ops,
};

static int __init phytium_display_init(void)
{
	return pci_register_driver(&phytium_display_driver);
}

static void __exit phytium_display_exit(void)
{
	return pci_unregister_driver(&phytium_display_driver);
}

module_init(phytium_display_init);
module_exit(phytium_display_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yang Xun <yangxun@phytium.com.cn>");
MODULE_DESCRIPTION("Phytium Display Controller");

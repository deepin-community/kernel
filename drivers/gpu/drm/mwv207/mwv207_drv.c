// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
 * All rights reserved.
 *
 * Author:
 *      shanjinkui <shanjinkui@jingjiamicro.com>
 *
 * The software and information contained herein is proprietary and
 * confidential to JingJiaMicro Electronics. This software can only be
 * used by JingJiaMicro Electronics Corporation. Any use, reproduction,
 * or disclosure without the written permission of JingJiaMicro
 * Electronics Corporation is strictly prohibited.
 */
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fbdev_generic.h>
#include <drm/drm_aperture.h>
#include <drm/drm_gem_ttm_helper.h>
#include <drm/drm_device.h>
#include <drm/drm_drv.h>
#include <drm/drm_file.h>
#include <drm/drm_print.h>
#include <drm/drm_gem.h>
#include <drm/drm_ioctl.h>
#include "mwv207_drm.h"
#include "mwv207_gem.h"
#include "mwv207_bo.h"
#include "dc/mwv207_kms.h"
#include "mwv207_ctx.h"
#include "mwv207_submit.h"
#include "mwv207_sched.h"
#include "mwv207_irq.h"
#include "mwv207_db.h"
#include "mwv207_vbios.h"

#define DRIVER_NAME	"mwv207"
#define DRIVER_DESC	"mwv207 gpu driver"
#define DRIVER_DATE	"20220407"
#define DRIVER_MAJOR	0
#define DRIVER_MINOR	1

#define WIN_SIZE  0x10000

#define IATU_BAR_LOW_OFFSET            0x108
#define IATU_BAR_HIGH_OFFSET           0x10c
#define IATU_BAR_LIMIT_LOW_OFFSET      0x110

#define IATU_REGION_MODE_ADDR    0

static int selftest;
module_param(selftest, int, 0644);
MODULE_PARM_DESC(selftest, "run selftest when startup");


static int mwv207_driver_open(struct drm_device *dev, struct drm_file *file_priv)
{
	struct mwv207_fpriv *fpriv;

	fpriv = kzalloc(sizeof(struct mwv207_fpriv), GFP_KERNEL);
	if (fpriv == NULL)
		return -ENOMEM;

	mwv207_ctx_mgr_init(dev, &fpriv->ctx_mgr);

	file_priv->driver_priv = fpriv;

	return 0;
}

static void mwv207_driver_postclose(struct drm_device *dev,
				 struct drm_file *file_priv)
{
	struct mwv207_fpriv *fpriv = to_fpriv(file_priv);

	mwv207_ctx_mgr_fini(dev, &fpriv->ctx_mgr);
	kfree(fpriv);
}

DEFINE_DRM_GEM_FOPS(mwv207_drm_driver_fops);

static const struct drm_ioctl_desc mwv207_ioctls_drm[] = {
	DRM_IOCTL_DEF_DRV(MWV207_INFO, mwv207_db_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207_GEM_CREATE, mwv207_gem_create_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207_GEM_MMAP, mwv207_gem_mmap_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207_GEM_WAIT, mwv207_gem_wait_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207_CTX, mwv207_ctx_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
	DRM_IOCTL_DEF_DRV(MWV207_SUBMIT, mwv207_submit_ioctl, DRM_AUTH|DRM_RENDER_ALLOW),
};

static struct drm_driver mwv207_drm_driver = {
	.driver_features	= DRIVER_MODESET | DRIVER_ATOMIC | DRIVER_GEM | DRIVER_RENDER,
	.fops                   = &mwv207_drm_driver_fops,
	.open                   = mwv207_driver_open,
	.postclose              = mwv207_driver_postclose,

	.dumb_create		= mwv207_gem_dumb_create,
	.dumb_map_offset        = drm_gem_ttm_dumb_map_offset,

	.gem_prime_import       = drm_gem_prime_import,

	.ioctls                 = mwv207_ioctls_drm,
	.num_ioctls             = ARRAY_SIZE(mwv207_ioctls_drm),

	.name			= DRIVER_NAME,
	.desc			= DRIVER_DESC,
	.date			= DRIVER_DATE,
	.major			= DRIVER_MAJOR,
	.minor			= DRIVER_MINOR,
};

static inline void iatu_write(struct mwv207_device *jdev, int index, u32 offset, u32 value)
{
	u32 region_base = 0x10000 + index * 0x200;

	writel(value, jdev->iatu + region_base + offset);
}

static inline u32 iatu_read(struct mwv207_device *jdev, int index, u32 offset)
{
	u32 region_base = 0x10000 + index * 0x200;

	return readl(jdev->iatu + region_base + offset);
}

static void mwv207_win_slide(struct mwv207_device *jdev, int region,
		u64 pci_win_base, u64 axi_win_base)
{
	iatu_write(jdev, region, IATU_BAR_LOW_OFFSET, pci_win_base & 0xffffffff);
	iatu_write(jdev, region, IATU_BAR_HIGH_OFFSET, pci_win_base >> 32);
	iatu_write(jdev, region, 0x114, axi_win_base & 0xffffffff);
	iatu_write(jdev, region, 0x118, axi_win_base >> 32);

	iatu_write(jdev, region, IATU_BAR_LIMIT_LOW_OFFSET,
			(pci_win_base + WIN_SIZE - 1) & 0xffffffff);
	iatu_write(jdev, region, 0x100, 0x00);
	iatu_write(jdev, region, 0x104, 1 << 31 | IATU_REGION_MODE_ADDR << 30 | 6 << 8);

	mb();
	if (iatu_read(jdev, region, 0x118) != (axi_win_base >> 32))
		pr_warn("mwv207: sliding window failed");
}

static int mwv207_iatu_map_bar(struct mwv207_device *jdev, int bar, u64 axi_addr)
{
	iatu_write(jdev, bar + 9, 0x114, axi_addr & 0xffffffff);
	iatu_write(jdev, bar + 9, 0x118, axi_addr >> 32);
	iatu_write(jdev, bar + 9, 0x100, 0x00);
	iatu_write(jdev, bar + 9, 0x104, 1 << 31 | 1 << 30 | bar << 8);

	mb();
	if (iatu_read(jdev, bar + 9, 0x114) != (axi_addr & 0xffffffff))
		return -ENODEV;

	return 0;
}

static int mwv207_iatu_init(struct mwv207_device *jdev)
{
	int ret;

	ret = mwv207_iatu_map_bar(jdev, 0,  0x10000000);
	if (ret)
		return ret;

	ret = mwv207_iatu_map_bar(jdev, 1,  0x00000000);
	if (ret)
		return ret;

	ret = mwv207_iatu_map_bar(jdev, 2,  0x10000000);
	if (ret)
		return ret;

	spin_lock_init(&jdev->win_lock);

	jdev->pci_win_base  = pci_bus_address(jdev->pdev, 2)
			+ pci_resource_len(jdev->pdev, 2) - WIN_SIZE;

	mwv207_win_slide(jdev, 0, jdev->pci_win_base, 0x10000000);

	jdev->vram_last_win = 0;

	return 0;
}

static void *mwv207_remap_region(struct mwv207_device *jdev, u64 vram_addr, u32 *len)
{
	u64 vram_win_base  = (vram_addr & (~(WIN_SIZE - 1)));
	u64 winlimit       = vram_win_base + WIN_SIZE;

	if (vram_win_base != jdev->vram_last_win) {
		mwv207_win_slide(jdev, 0, jdev->pci_win_base, vram_win_base + 0x10000000);
		jdev->vram_last_win = vram_win_base;
	}

	if (*len > winlimit - vram_addr)
		*len = winlimit - vram_addr;

	return jdev->win + vram_addr - vram_win_base;
}

void jdev_write_vram(struct mwv207_device *jdev, u64 vram_addr, void *buf, int size)
{
	int remain, len;
	void *va;

	for (remain = size; remain > 0; remain -= len, buf += len, vram_addr += len) {
		len = remain;
		spin_lock(&jdev->win_lock);
		va = mwv207_remap_region(jdev, vram_addr, &len);
		if (!va || len <= 0) {
			spin_unlock(&jdev->win_lock);
			pr_warn("mwv207: write vram failed");
			return;
		}
		memcpy_toio(va, buf, len);
		spin_unlock(&jdev->win_lock);
	}
}

static int mwv207_remove_conflicting_framebuffers(struct pci_dev *pdev)
{
	return drm_aperture_remove_conflicting_pci_framebuffers(pdev, &mwv207_drm_driver);
}

static u64 mwv207_vram_size(struct mwv207_device *jdev)
{
	int nr = 0;
	u64 size;

	switch (jdev_read(jdev, 0x9b0190) & 0x7) {
	case 0:
		size = 2048;
		break;
	case 1:
		size = 1024;
		break;
	case 2:
		size = 4096;
		break;
	case 3:
		size = 8192;
		break;
	default:
		pr_warn("mwv207: ddr size default to 256MB");
		return 256UL * 1024 * 1024;
	}

	if (jdev->lite) {
		nr += jdev_read(jdev, 0xd101bc) & 0x1;
		nr += jdev_read(jdev, 0xd11cbc) & 0x1;
	} else {
		nr += (jdev_read(jdev, 0x9702f4) & 0x02000000) ? 1 : 0;
		nr += (jdev_read(jdev, 0x9802f4) & 0x02000000) ? 1 : 0;
	}
	if (!nr) {
		nr = 1;
		pr_warn("mwv207: ddr cnt default to 1");
	}

	return size * 1024 * 1024 * nr;
}

static int mwv207_pci_probe(struct pci_dev *pdev,
			    const struct pci_device_id *ent)
{
	struct mwv207_device *jdev;
	int ret;

	ret = mwv207_remove_conflicting_framebuffers(pdev);
	if (ret)
		return ret;

	pci_set_master(pdev);
	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	jdev = devm_drm_dev_alloc(&pdev->dev, &mwv207_drm_driver,
			struct mwv207_device, base);
	if (IS_ERR(jdev))
		return PTR_ERR(jdev);

	jdev->dev = &pdev->dev;
	jdev->pdev = pdev;
	jdev->base.dev_private = jdev;

	jdev->mmio = devm_ioremap(jdev->dev,
			pci_resource_start(pdev, 1), pci_resource_len(pdev, 1));
	if (!jdev->mmio)
		return -ENOMEM;

	jdev->iatu = devm_ioremap(jdev->dev,
			pci_resource_start(pdev, 4), pci_resource_len(pdev, 4));
	if (!jdev->iatu)
		return -ENOMEM;

	jdev->win  = devm_ioremap(jdev->dev,
			pci_resource_end(pdev, 2) - WIN_SIZE + 1, WIN_SIZE);
	if (!jdev->win)
		return -ENOMEM;

	ret = mwv207_iatu_init(jdev);
	if (ret)
		return ret;

	jdev->lite = jdev_read(jdev, 0x12c0000) == 0x21 ? false : true;

	pci_set_drvdata(pdev, jdev);

	ret = mwv207_db_init(jdev);
	if (ret)
		return ret;

	mwv207_vbios_init(jdev);

	ret = mwv207_irq_init(jdev);
	if (ret)
		goto err_vbios;

	jdev->visible_vram_size = pci_resource_len(pdev, 2) - WIN_SIZE;
	jdev->vram_size         = mwv207_vram_size(jdev);
	jdev->vram_bar_base     = pci_resource_start(pdev, 2);

	ret = mwv207_ttm_init(jdev);
	if (ret)
		goto err_irq;

	ret = mwv207_sched_init(jdev);
	if (ret)
		goto err_ttm;

	ret = mwv207_kctx_init(jdev);
	if (ret)
		goto err_sched;

	ret = mwv207_kms_init(jdev);
	if (ret)
		goto err_kctx;

	if (selftest) {
		ret = mwv207_test(jdev);
		if (ret)
			goto err_kms;
	}

	mwv207_db_sort(jdev);
	ret = drm_dev_register(&jdev->base, 0);
	if (ret)
		goto err_kms;
	drm_fbdev_generic_setup(&jdev->base, 32);

	return 0;

err_kms:
	mwv207_kms_fini(jdev);
err_kctx:
	mwv207_kctx_fini(jdev);
err_sched:
	mwv207_sched_fini(jdev);
err_ttm:
	mwv207_ttm_fini(jdev);
err_irq:
	mwv207_irq_fini(jdev);
err_vbios:
	mwv207_vbios_fini(jdev);
	mwv207_db_fini(jdev);
	return ret;
}

static void mwv207_pci_remove(struct pci_dev *pdev)
{
	struct mwv207_device *jdev = pci_get_drvdata(pdev);

	drm_dev_unregister(&jdev->base);
	mwv207_kms_fini(jdev);
	mwv207_kctx_fini(jdev);
	mwv207_sched_fini(jdev);
	mwv207_ttm_fini(jdev);
	mwv207_irq_fini(jdev);
	mwv207_vbios_fini(jdev);
	mwv207_db_fini(jdev);
	pci_clear_master(pdev);
	pci_set_drvdata(pdev, NULL);
}

static int mwv207_pmops_suspend(struct device *dev)
{
	struct mwv207_device *jdev = dev_get_drvdata(dev);
	struct pci_dev *pdev = to_pci_dev(dev);
	int ret;
	struct ttm_resource_manager *man;

	ret = mwv207_kms_suspend(jdev);
	if (ret)
		return ret;

	man = ttm_manager_type(&jdev->bdev, TTM_PL_VRAM);
	ttm_resource_manager_evict_all(&jdev->bdev, man);

	ret = mwv207_sched_suspend(jdev);
	if (ret)
		goto fail;

	mwv207_irq_suspend(jdev);

	pci_save_state(pdev);
	pci_disable_device(pdev);
	pci_set_power_state(pdev, PCI_D3hot);
	udelay(200);
	return 0;
fail:
	mwv207_kms_resume(jdev);
	return ret;
}

static int mwv207_pmops_resume(struct device *dev)
{
	struct mwv207_device *jdev = dev_get_drvdata(dev);
	struct pci_dev *pdev = to_pci_dev(dev);
	int ret;

	msleep(100);

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	ret = pci_enable_device(pdev);
	if (ret) {
		pr_err("mwv207: failed to enable pci device");
		return ret;
	}
	pci_set_master(pdev);

	ret = mwv207_iatu_init(jdev);
	if (ret)
		return ret;

	mwv207_irq_resume(jdev);

	ret = mwv207_sched_resume(jdev);
	if (ret)
		return ret;

	return mwv207_kms_resume(jdev);
}

static SIMPLE_DEV_PM_OPS(mwv207_pm_ops, mwv207_pmops_suspend, mwv207_pmops_resume);

#define MWV207_PCI_DEVICE_DATA(vend, dev, data) \
	.vendor = vend, .device = dev, .driver_data = (kernel_ulong_t)(data), \
	.subvendor = PCI_ANY_ID, .subdevice = PCI_ANY_ID, \
	.class = 0, .class_mask = 0

static const struct pci_device_id pciidlist[] = {
	{MWV207_PCI_DEVICE_DATA(0x0731, 0x9100, NULL)},
	{0},
};

MODULE_DEVICE_TABLE(pci, pciidlist);
static struct pci_driver mwv207_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = pciidlist,
	.probe = mwv207_pci_probe,
	.remove = mwv207_pci_remove,
	.driver.pm = &mwv207_pm_ops,
};

static int __init mwv207_init(void)
{
	return pci_register_driver(&mwv207_pci_driver);
}

static void __exit mwv207_exit(void)
{
	pci_unregister_driver(&mwv207_pci_driver);
}

module_init(mwv207_init);
module_exit(mwv207_exit);
MODULE_DESCRIPTION("MWV207 Graphics Driver");
MODULE_AUTHOR("shanjinkui");
MODULE_LICENSE("GPL");

// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pci.h>
#include <linux/pci-ats.h>
#include <linux/sysfs.h>

#include "tsse_dev_drv.h"
#include "tsse_vuart.h"
#include "tsse_ipc.h"
#include "tsse_fw_service.h"

#define CLUSTER_SLOT_CONFIG_OFFSET 0x5780000
#define QPAIR_SETTING_OFFSET 0x50000
#define BAR_START 2
#define BAR_END 4

static DEFINE_IDA(tsse_ida);

static inline void tsse_qpair_enable_pf(struct tsse_dev *tdev, bool enable)
{
	writel(enable ? 1 : 0,
	       TSSE_DEV_BARS(tdev)[2].virt_addr +
		   CLUSTER_SLOT_CONFIG_OFFSET + QPAIR_SETTING_OFFSET);
}
static int tsse_sriov_disable(struct tsse_dev *tdev)
{
	pci_disable_sriov(tdev->tsse_pci_dev.pci_dev);
	tsse_qpair_enable_pf(tdev, true);

	return 0;
}

static int tsse_sriov_configure(struct pci_dev *pdev, int num_vfs_param)
{
	int totalvfs = pci_sriov_get_totalvfs(pdev);
	struct tsse_dev *tdev = pci_to_tsse_dev(pdev);
	int ret = 0;

	if ((!tdev) || (num_vfs_param < 0) || (totalvfs <= 0)) {
		dev_err(&pdev->dev,
			"%s %d: failed to config sriov, tdev=%p totalvfs=%d num_vfs_param=%d\n",
			__func__, __LINE__, tdev, totalvfs, num_vfs_param);
		return -EBADE;
	}

	if (num_vfs_param > totalvfs)
		num_vfs_param = totalvfs;

	dev_info(&pdev->dev, "%s %d: has total %d vfs, and enable %d vfs\n",
		 __func__, __LINE__, totalvfs, num_vfs_param);

	if ((num_vfs_param > TSSE_PF_MAX_IRQ_NUM) ||
	    (num_vfs_param > TSSE_PF_MAX_QPAIR_NUM)) {
		tsse_dev_err(
			tdev,
			"vfs number is greater than pf's \"max_irq_num=%d or max_qpairs_num=%d\"\n",
			TSSE_PF_MAX_IRQ_NUM, TSSE_PF_MAX_QPAIR_NUM);
		return -EBADE;
	}

	if (!tsse_dev_started(tdev)) {
		dev_err(&pdev->dev, "%s %d: device is not started\n", __func__,
			__LINE__);
		return -EBADE;
	}

	if (tsse_dev_in_use(tdev)) {
		dev_err(&pdev->dev, "%s %d: device is busy\n", __func__,
			__LINE__);
		return -EBUSY;
	}

	tsse_sriov_disable(tdev);

	tsse_prepare_restart_dev(tdev);

	tdev->num_vfs = num_vfs_param;

	if (tdev->num_vfs > 0) {
		tdev->num_irqs = TSSE_SRIOV_PF_MAX_IRQ_NUM;
		tdev->qpairs_bank.num_qparis = TSSE_SRIOV_PF_MAX_QPAIR_NUM;
	} else {
		tdev->num_irqs = TSSE_PF_MAX_IRQ_NUM;
		tdev->qpairs_bank.num_qparis = TSSE_PF_MAX_QPAIR_NUM;
	}

	tsse_dev_info(
		tdev,
		"num_irqs:%u num_qparis:%u qpairs' start irq vector index:%u qpairs' reg base:0x%lx\n",
		tdev->num_irqs, tdev->qpairs_bank.num_qparis,
		tdev->qpairs_bank.irq_vec, (ulong)tdev->qpairs_bank.reg_base);

	ret = tsse_start_dev(tdev);
	if (ret) {
		dev_err(&pdev->dev, "%s %d: failed to start the device\n",
			__func__, __LINE__);
		return ret;
	}

	if (num_vfs_param > 0) {
		tsse_qpair_enable_pf(tdev, false);
		pci_enable_sriov(pdev, num_vfs_param);
	}

	return num_vfs_param;
}

/**
 * tsse_image_load_store() - This function will be called when user
 * writes string to /sys/bus/pci/devices/.../tsse_image_load.
 * Driver will always loads /lib/firmware/tsse_firmware.bin.
 * @dev: device
 * @attr: device attribute
 * @buf: string that user writes
 * @count: string length that user writes
 * Return: the number of bytes used from the buffer, here it is just the count argument.
*/
static ssize_t tsse_image_load_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct pci_dev *pdev = NULL;
	struct tsse_dev *tdev = NULL;

	pdev = container_of(dev, struct pci_dev, dev);
	if (pdev)
		tdev = pci_to_tsse_dev(pdev);
	if (buf && count && tdev) {
		tsse_dev_info(tdev, "receive command to load firmware %s\n", TSSE_FIRMWARE);
		if (!tsse_fw_load(pdev, TSSE_FIRMWARE, &tdev->fw)) {
			if (!get_firmware_version(tdev->fw, tdev->fw_version))
				tdev->fw_version_exist = true;
			if (tsse_fw_manual_load_ipc(pdev))
				dev_err(&pdev->dev, "%s %d: firmware update failed\n",
					__func__, __LINE__);
		}
	}
	return count;
}

DEVICE_ATTR_WO(tsse_image_load);

static int device_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	int status = 0;
	int bar;
	u32 tmp_val;
	struct tsse_dev *tdev;

	if (!pdev->is_physfn) {
		dev_err(&pdev->dev, "%s %d: this is not Physical fn\n",
			__func__, __LINE__);
		return -EPERM;
	}

	if (num_possible_nodes() > 1 && dev_to_node(&pdev->dev) < 0) {
		dev_err(&pdev->dev,
			"%s %d: invalid numa configuration for tsse\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	tdev = kzalloc_node(sizeof(*tdev), GFP_KERNEL, dev_to_node(&pdev->dev));

	if (!tdev)
		return -ENOMEM;

	status = pcim_enable_device(pdev);

	if (status) {
		dev_err(&pdev->dev, "pcim_enable_device failed\n");
		goto out_err;
	}

	pci_set_master(pdev);

	if (dma_set_mask(&pdev->dev, DMA_BIT_MASK(48))) {
		if ((dma_set_mask(&pdev->dev, DMA_BIT_MASK(32)))) {
			dev_err(&pdev->dev,
				"failed to set tsse dma address width\n");
			status = -EFAULT;
			goto out_err;
		} else {
			dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
		}

	} else {
		dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(48));
	}

	dma_set_max_seg_size(&pdev->dev, UINT_MAX);

	status = pcim_iomap_regions(pdev, BIT(0) | BIT(2), TSSE_DEV_NAME);
	if (status) {
		dev_err(&pdev->dev, "I/O memory remapping failed\n");
		goto out_err;
	}

	for (bar = BAR_START; bar < BAR_END;) {
		TSSE_DEV_BARS(tdev)[bar].addr = pci_resource_start(pdev, bar);
		TSSE_DEV_BARS(tdev)[bar].size = pci_resource_len(pdev, bar);
		TSSE_DEV_BARS(tdev)
		[bar].virt_addr = pcim_iomap_table(pdev)[bar];

		dev_info(&pdev->dev,
			 "bar[%d]: addr=0x%llx, size=0x%llx, virt_addr=0x%lx\n",
			 bar, TSSE_DEV_BARS(tdev)[bar].addr,
			 TSSE_DEV_BARS(tdev)[bar].size,
			 (ulong)TSSE_DEV_BARS(tdev)[bar].virt_addr);

		bar += 2;
	}

	tdev->owner = THIS_MODULE;
	tdev->is_vf = false;
	tdev->tsse_pci_dev.pci_dev = pdev;
	tdev->id = ida_alloc(&tsse_ida, GFP_KERNEL);
	if (tdev->id < 0) {
		dev_err(&pdev->dev, "Unable to get id\n");
		status = tdev->id;
		goto out_err;
	}

	pci_set_drvdata(pdev, tdev);

	tdev->num_irqs = TSSE_PF_MAX_IRQ_NUM;
	tdev->qpairs_bank.num_qparis = TSSE_PF_MAX_QPAIR_NUM;
	tdev->qpairs_bank.irq_vec = TSSE_PF_QPAIR_START_IRQ_VECTOR;
	tdev->qpairs_bank.reg_base =
		TSSE_DEV_BARS(tdev)[2].virt_addr + TSSE_PF_QPAIR_REG_BASE;

	tsse_qpair_enable_pf(tdev, true);

	tsse_dev_info(
		tdev,
		"num_irqs:%u num_qparis:%u qpairs' start irq vector index:%u qpairs' reg base:0x%lx\n",
		tdev->num_irqs, tdev->qpairs_bank.num_qparis,
		tdev->qpairs_bank.irq_vec, (ulong)tdev->qpairs_bank.reg_base);

	if (tsse_devmgr_add_dev(tdev)) {
		dev_err(&pdev->dev,
			"%s %d: tsse_devmgr failed to add new device\n",
			__func__, __LINE__);
		status = -EFAULT;
		goto out_err_ida_free;
	}

	if (vuart_init_port(pdev)) {
		dev_err(&pdev->dev,
			"%s %d: vuart_init_port failed to init vuart.\n",
			__func__, __LINE__);
		status = -EFAULT;
		goto out_err_port_init;
	}

	tdev->fw_version_exist = false;
	/* Its result not break driver init process */
	if (!tsse_fw_load(pdev, TSSE_FIRMWARE, &tdev->fw)) {
		if (!get_firmware_version(tdev->fw, tdev->fw_version))
			tdev->fw_version_exist = true;
	}

	if (tsse_ipc_init(pdev)) {
		dev_err(&pdev->dev,
			"%s %d: tsse_ipc_init failed to tsse_ipc.\n", __func__,
			__LINE__);
		status = -EFAULT;
		goto out_err_ipc;
	}

	if (sysfs_create_file(&pdev->dev.kobj, &dev_attr_tsse_image_load.attr)) {
		dev_err(&pdev->dev,
			"%s %d: sysfs_create_file failed for tsse image load.\n",
			__func__, __LINE__);
		status = -EFAULT;
		goto out_err_image_load;
	}

	tsse_dev_info(tdev, "successful\n");

	pci_read_config_dword(pdev, 0x720, &tmp_val);
	tsse_dev_dbg(tdev, "the value of FILTER_MASK_2_REG is 0x%x\n", tmp_val);

	return 0;
out_err_image_load:
	tsse_ipc_deinit(tdev);
out_err_ipc:
	vuart_uninit_port(pdev);
out_err_port_init:
	tsse_devmgr_rm_dev(tdev);
out_err_ida_free:
	ida_free(&tsse_ida, tdev->id);
out_err:
	kfree(tdev);
	return status;
}

static void device_remove(struct pci_dev *pdev)
{
	struct tsse_dev *tdev = pci_to_tsse_dev(pdev);

	pr_info("%s %d: pci_dev 0x%lx tsse_dev 0x%lx\n", __func__, __LINE__,
		(ulong)pdev, (ulong)tdev);

	tsse_sriov_disable(tdev);
	if (tdev->fw) {
		release_firmware(tdev->fw);
		tdev->fw = NULL;
	}
	sysfs_remove_file(&pdev->dev.kobj, &dev_attr_tsse_image_load.attr);
	tsse_ipc_deinit(tdev);
	vuart_uninit_port(pdev);
	tsse_devmgr_rm_dev(tdev);
	ida_free(&tsse_ida, tdev->id);
	kfree(tdev);
	dev_info(&pdev->dev, "%s %d: successful\n", __func__, __LINE__);
}

static const struct pci_device_id pci_ids[] = {
	{
		PCI_DEVICE(0x1b00, 0xc011),
	},
	{
		PCI_DEVICE(0x1b00, 0xd011),
	},
	{ 0 }
};

static struct pci_driver pci_driver = {
	.name = TSSE_DEV_NAME,
	.id_table = pci_ids,
	.probe = device_probe,
	.remove = device_remove,
	.sriov_configure = tsse_sriov_configure,
};

MODULE_DEVICE_TABLE(pci, pci_ids);

static int __init tsse_init(void)
{
	int status;

	status = vuart_register();
	if (status) {
		pr_err("vuart_register failed[%d].\n", status);
		return status;
	}

	status = pci_register_driver(&pci_driver);
	if (status) {
		vuart_unregister();
		return status;
	}

	pr_info(KBUILD_MODNAME ": loaded.\n");

	return 0;
}

static void __exit tsse_exit(void)
{
	pci_unregister_driver(&pci_driver);
	vuart_unregister();

	pr_info(KBUILD_MODNAME ": unloaded.\n");
}

module_init(tsse_init);
module_exit(tsse_exit);

MODULE_AUTHOR("montage-tech.com");
MODULE_DESCRIPTION("TSSE device driver");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
MODULE_FIRMWARE(TSSE_FIRMWARE);

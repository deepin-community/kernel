// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium USB DRD Driver.
 *
 * Copyright (C) 2023 - 2024 Phytium.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

#include "core.h"

#define BAR_HOST_OFFSET		0x8000
#define BAR_DEVICE_OFFSET	0x4000
#define BAR_DEVICE_SIZE		0x4000
#define BAR_OTG_SIZE		0x4000

static int phytium_pci_probe(struct pci_dev *pdev,
		const struct pci_device_id *id)
{
	int ret;

	struct phytium_usb *phytium_usb = NULL;

	ret = pcim_enable_device(pdev);
	if (ret) {
		dev_err(&pdev->dev, "pcim_enable_device failed\n");
		goto put_pci;
	}

	pci_set_master(pdev);
	if (pci_is_enabled(pdev)) {
		phytium_usb = pci_get_drvdata(pdev);
	}
	
	if (!phytium_usb) {
		phytium_usb = kzalloc(sizeof(*phytium_usb), GFP_KERNEL);
		if (!phytium_usb) {
			ret = -ENOMEM;
			goto disabled_pci;
		}
	}

	//otg resource init
	phytium_usb->otg_res.start = pci_resource_start(pdev, 0);
	phytium_usb->otg_res.end = phytium_usb->otg_res.start + BAR_OTG_SIZE;
	phytium_usb->otg_res.name = "otg";
	phytium_usb->otg_res.flags = IORESOURCE_MEM;
	phytium_usb->otg_irq = pdev->irq;

	//gadget resource init
	phytium_usb->device_irq = pdev->irq;
	phytium_usb->device_regs = devm_ioremap(&pdev->dev, pci_resource_start(pdev, 0)
			+ BAR_DEVICE_OFFSET, BAR_DEVICE_SIZE);
	if (IS_ERR(phytium_usb->device_regs)) {
		dev_warn(&pdev->dev, "gadget ioremap failed\n");
		return PTR_ERR(phytium_usb->device_regs);
	}

	//host resource init
	dev_warn(&pdev->dev, "host resource init\n");
	phytium_usb->host_res[0].start = pdev->irq;
	phytium_usb->host_res[0].name = "host";
	phytium_usb->host_res[0].flags = IORESOURCE_IRQ;

	phytium_usb->host_res[1].start = pci_resource_start(pdev, 0) + BAR_HOST_OFFSET;
	phytium_usb->host_res[1].end = pci_resource_end(pdev, 0) - BAR_HOST_OFFSET;
	phytium_usb->host_res[1].name = "host";
	phytium_usb->host_res[1].flags = IORESOURCE_MEM;

	if (pci_is_enabled(pdev)) {
		phytium_usb->dev = &pdev->dev;
		pci_set_drvdata(pdev, phytium_usb);

		ret = phytium_usb_init(phytium_usb);
		if (ret) {
			dev_err(&pdev->dev, "phytium_usb_init failed\n");
			goto free_phytium_usb;
		}
	}

	return 0;
free_phytium_usb:
	kfree(phytium_usb);

disabled_pci:
	pci_disable_device(pdev);

put_pci:
	pci_dev_put(pdev);

	return 0;
}

static void phytium_pci_remove(struct pci_dev *pdev)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)pci_get_drvdata(pdev);

	if (!pci_is_enabled(pdev)) {
		kfree(phytium_usb);
		goto pci_put;
	}

	phytium_usb_uninit(phytium_usb);

pci_put:
	pci_dev_put(pdev);
	pci_set_drvdata(pdev, NULL);
}

static int __maybe_unused phytium_pci_suspend(struct device *dev)
{
	struct phytium_usb *phytium_usb = dev_get_drvdata(dev);

	return phytium_usb_suspend(phytium_usb);
}

static int __maybe_unused phytium_pci_resume(struct device *dev)
{
	struct phytium_usb *phytium_usb = dev_get_drvdata(dev);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&phytium_usb->lock, flags);
	ret = phytium_usb_resume(phytium_usb, 1);
	spin_unlock_irqrestore(&phytium_usb->lock, flags);

	return ret;
}

static const struct dev_pm_ops phytium_pci_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(phytium_pci_suspend, phytium_pci_resume)
};

static const struct pci_device_id phytium_pci_ids[] = {
	{ 0x10ee, 0x8012, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ 0, }
};

static struct pci_driver phytium_pci_driver = {
	.name = "phytium-pci",
	.id_table = &phytium_pci_ids[0],
	.probe = phytium_pci_probe,
	.remove = phytium_pci_remove,
	.driver = {
		.pm = &phytium_pci_pm_ops,
	}
};

module_pci_driver(phytium_pci_driver);
MODULE_DEVICE_TABLE(pci, phytium_pci_ids);

MODULE_ALIAS("pci:phytium");
MODULE_AUTHOR("Chen Zhenhua <chenzhenhua@phytium.com.cn>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Phytium USB PCI driver");
MODULE_VERSION(PHYTIUM_USB_DRIVER_V2_VERSION);

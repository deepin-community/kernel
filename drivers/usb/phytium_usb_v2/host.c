// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium USB DRD Driver.
 *
 * Copyright (C) 2023 - 2024 Phytium.
 */

#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/module.h>

#include "host.h"
#include "core.h"
#include "otg.h"
#include "../host/xhci.h"
#include "../host/xhci-plat.h"

#define HOST_RESOURCE_NUM 2
#define XECP_AUX_CTRL_REG1	0x8120
#define XECP_PORT_CAP_REG	0x8000
#define CFG_RXDET_P3_EN		BIT(15)
#define LPM_2_STB_SWITCH_EN	BIT(25)
#define PM_RUNTIME_ALLOW	BIT(0)

static void plat_phytium_start(struct usb_hcd *hcd)
{
	struct xhci_hcd *host = hcd_to_xhci(hcd);
	u32 value;

	if (host) {
		value = readl(&host->op_regs->command);
		value |= CMD_PM_INDEX;
		writel(value, &host->op_regs->command);
	}
}

static int plat_phytium_resume_quirk(struct usb_hcd *hcd)
{
	plat_phytium_start(hcd);

	return 0;
}

static const struct xhci_plat_priv plat_phytium_usb = {
	.plat_start = plat_phytium_start,
	.resume_quirk = plat_phytium_resume_quirk,
};

static int host_start(void *data)
{
	struct phytium_usb *phytium_usb;
	struct platform_device *host;
	struct usb_hcd *hcd;
	int ret;

	phytium_usb = (struct phytium_usb *)data;
	if (!phytium_usb)
		return 0;

	phytium_usb_otg_host_on((void *)phytium_usb);

	host = platform_device_alloc("xhci-hcd", PLATFORM_DEVID_AUTO);
	if (!host) {
		dev_err(phytium_usb->dev, "platform_device_alloc failed\n");
		return -ENOMEM;
	}

	host->dev.parent = phytium_usb->dev;
	phytium_usb->host_dev = host;

	ret = platform_device_add_resources(host, phytium_usb->host_res,
			HOST_RESOURCE_NUM);
	if (ret) {
		dev_err(phytium_usb->dev, "platform_device_add_resources failed\n");
		goto err;
	}

	phytium_usb->host_plat_data = kmemdup(&plat_phytium_usb,
			sizeof(struct xhci_plat_priv), GFP_KERNEL);
	if (!phytium_usb->host_plat_data) {
		ret = -ENOMEM;
		goto err;
	}


	phytium_usb->host_plat_data->quirks |= XHCI_RESET_ON_RESUME;
	ret = platform_device_add_data(host, phytium_usb->host_plat_data,
			sizeof(struct xhci_plat_priv));
	if (ret)
		goto free_memory;

	ret = platform_device_add(host);
	if (ret) {
		dev_err(phytium_usb->dev, "register host failed\n");
		goto free_memory;
	}

	hcd = platform_get_drvdata(host);
	if (hcd)
		phytium_usb->host_regs = hcd->regs;

	return 0;

free_memory:
	kfree(phytium_usb->host_plat_data);

err:
	platform_device_put(host);

	return ret;
}

static void host_stop(void *data)
{
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		phytium_usb_otg_host_off((void *)phytium_usb);
		kfree(phytium_usb->host_plat_data);
		platform_device_unregister(phytium_usb->host_dev);
		phytium_usb->host_dev = NULL;
	}
}

int phytium_usb_host_init(void *data)
{
	struct phytium_usb_role_driver *role_driver;
	struct phytium_usb *phytium_usb = (struct phytium_usb *)data;

	if (phytium_usb) {
		role_driver = devm_kzalloc(phytium_usb->dev, sizeof(*phytium_usb), GFP_KERNEL);
		if (!role_driver)
			return -ENOMEM;

		role_driver->start = host_start;
		role_driver->stop = host_stop;
		role_driver->state = ROLE_STATE_INACTIVE;
		role_driver->name = "host";
		phytium_usb->roles[USB_ROLE_HOST] = role_driver;
	}
	return 0;
}

MODULE_AUTHOR("Zhenhua Chen <chenzhenhua@phytium.com.cn>");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Phytium USB2 DRD Controller Driver");

// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium SWMAC specific glue layer
 *
 * Copyright (C) 2022, Phytium Technology Co., Ltd.
 *
 * Chen Baozi <chenbaozi@phytium.com.cn>
 */

#include <linux/acpi.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>

#include "stmmac.h"
#include "stmmac_platform.h"

int phytium_dwmac_probe(struct platform_device *pdev, bool quirk);
int phytium_dwmac_remove(struct platform_device *pdev);

static int phytium_dwmac_probe_ft2000(struct platform_device *pdev)
{
    return phytium_dwmac_probe(pdev, true);
}

#ifdef CONFIG_ACPI
static const struct acpi_device_id phytium_dwmac_acpi_ids[] = {
	{ .id = "FTGM0001" }, // compat FT2000/4 id
	{ }
};
MODULE_DEVICE_TABLE(acpi, phytium_dwmac_acpi_ids);
#endif

static struct platform_driver phytium_dwmac_driver = {
	.probe = phytium_dwmac_probe_ft2000,
	.remove = phytium_dwmac_remove,
	.driver = {
		.name		= "phytium-dwmac",
		.acpi_match_table = ACPI_PTR(phytium_dwmac_acpi_ids),
	},
};
module_platform_driver(phytium_dwmac_driver);

MODULE_AUTHOR("Chen Baozi <chenbaozi@phytium.com.cn>");
MODULE_DESCRIPTION("Phytium DWMAC specific glue layer");
MODULE_LICENSE("GPL");

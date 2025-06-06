// SPDX-License-Identifier: GPL-2.0
/* UltraRisc DWMAC platform driver
 *
 * Copyright(C) 2025 UltraRisc Technology Co., Ltd.
 * 
 *  Author:  wangjiahao <wangjiahao@ultrarisc.com>
 */

#include <linux/of_device.h>
#include "stmmac_platform.h"

static const struct of_device_id ultrarisc_eth_plat_match[] = {
	{ .compatible = "ultrarisc,dp1000-gmac", },
	{ }
};
MODULE_DEVICE_TABLE(of, ultrarisc_eth_plat_match);

static int ultrarisc_eth_plat_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	int err;

	err = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (err)
		return err;

	plat_dat = stmmac_probe_config_dt(pdev, stmmac_res.mac);
	if (IS_ERR(plat_dat)) {
		dev_err(&pdev->dev, "dt configuration failed\n");
		return PTR_ERR(plat_dat);
	}

	if (!is_of_node(dev->fwnode))
		goto err;

	err = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (err)
		return err;

	return 0;
err:
	stmmac_remove_config_dt(pdev, plat_dat);
	return err;
}

static int ultrarisc_eth_plat_remove(struct platform_device *pdev)
{
	stmmac_dvr_remove(&pdev->dev);
	return 0;
}

static struct platform_driver ultrarisc_eth_plat_driver = {
	.probe  = ultrarisc_eth_plat_probe,
	.remove = ultrarisc_eth_plat_remove,
	.driver = {
		.name = "ultrarisc-eth-plat",
		.pm = &stmmac_pltfr_pm_ops,
		.of_match_table = ultrarisc_eth_plat_match,
	},
};

module_platform_driver(ultrarisc_eth_plat_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("UltraRisc DWMAC platform driver");

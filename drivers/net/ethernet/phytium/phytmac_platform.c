// SPDX-License-Identifier: GPL-2.0-only
/*
 * Phytium GMAC Platform wrapper.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_net.h>
#include <linux/acpi.h>
#include "phytmac.h"
#include "phytmac_v1.h"
#include "phytmac_v2.h"

static const struct phytmac_config phytium_1p0_config = {
	.hw_if = &phytmac_1p0_hw,
	.caps = PHYTMAC_CAPS_TAILPTR
			| PHYTMAC_CAPS_START
			| PHYTMAC_CAPS_JUMBO
			| PHYTMAC_CAPS_LSO,
	.queue_num = 4,
	.tsu_rate = 300000000,
};

static const struct phytmac_config phytium_2p0_config = {
	.hw_if = &phytmac_2p0_hw,
	.caps = PHYTMAC_CAPS_TAILPTR
			| PHYTMAC_CAPS_LPI
			| PHYTMAC_CAPS_LSO
			| PHYTMAC_CAPS_MSG
			| PHYTMAC_CAPS_JUMBO,
	.queue_num = 2,
	.tsu_rate = 300000000,
};

#if defined(CONFIG_OF)
static const struct of_device_id phytmac_dt_ids[] = {
	{ .compatible = "phytium,gmac-1.0", .data = &phytium_1p0_config },
	{ .compatible = "phytium,gmac-2.0", .data = &phytium_2p0_config },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, phytmac_dt_ids);
#endif /* CONFIG_OF */

#ifdef CONFIG_ACPI
static const struct acpi_device_id phytmac_acpi_ids[] = {
	{ .id = "PHYT0046", .driver_data = (kernel_ulong_t)&phytium_1p0_config },
	{ }
};

MODULE_DEVICE_TABLE(acpi, phytmac_acpi_ids);
#else
#define phytmac_acpi_ids NULL
#endif

static int phytmac_get_phy_mode(struct platform_device *pdev)
{
	const char *pm;
	int err, i;

	err = device_property_read_string(&pdev->dev, "phy-mode", &pm);
	if (err < 0)
		return err;

	for (i = 0; i < PHY_INTERFACE_MODE_MAX; i++) {
		if (!strcasecmp(pm, phy_modes(i)))
			return i;
	}

	return -ENODEV;
}

static int phytmac_plat_probe(struct platform_device *pdev)
{
	const struct phytmac_config *phytmac_config = &phytium_1p0_config;
	struct device_node *np = pdev->dev.of_node;
	struct resource *regs;
	struct phytmac *pdata;
	int ret, i;
	u32 queue_num;

	pdata = phytmac_alloc_pdata(&pdev->dev);
	if (IS_ERR(pdata)) {
		ret = PTR_ERR(pdata);
		goto err_alloc;
	}

	platform_set_drvdata(pdev, pdata);

	pdata->platdev = pdev;

	if (pdev->dev.of_node) {
		const struct of_device_id *match;

		match = of_match_node(phytmac_dt_ids, np);
		if (match && match->data) {
			phytmac_config = match->data;
			pdata->hw_if = phytmac_config->hw_if;
			pdata->capacities = phytmac_config->caps;
			pdata->queues_max_num = phytmac_config->queue_num;
		}
	} else if (has_acpi_companion(&pdev->dev)) {
		const struct acpi_device_id *match;

		match = acpi_match_device(phytmac_acpi_ids, &pdev->dev);
		if (match && match->driver_data) {
			phytmac_config = (void *)match->driver_data;
			pdata->hw_if = phytmac_config->hw_if;
			pdata->capacities = phytmac_config->caps;
			pdata->queues_max_num = phytmac_config->queue_num;
		}
	}

	i = 0;
	pdata->mac_regs = devm_platform_get_and_ioremap_resource(pdev, i, &regs);
	if (IS_ERR(pdata->mac_regs)) {
		dev_err(&pdev->dev, "mac_regs ioremap failed\n");
		ret = PTR_ERR(pdata->mac_regs);
		goto err_mem;
	}
	pdata->ndev->base_addr = regs->start;

	if (pdata->capacities & PHYTMAC_CAPS_MSG) {
		++i;
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (regs) {
			pdata->msg_regs = ioremap_wt(regs->start, MEMORY_SIZE);
			if (!pdata->msg_regs) {
				dev_err(&pdev->dev, "msg_regs ioremap failed, i=%d\n", i);
				goto err_mem;
			}
		}
	}

	if (device_property_read_bool(&pdev->dev, "lpi"))
		pdata->capacities |= PHYTMAC_CAPS_LPI;

	if (pdata->capacities & PHYTMAC_CAPS_LPI) {
		/* lpi resource */
		++i;
		regs = platform_get_resource(pdev, IORESOURCE_MEM, i);
		if (regs) {
			pdata->mhu_regs = ioremap(regs->start, MHU_SIZE);
			if (!pdata->mhu_regs)
				dev_err(&pdev->dev, "mhu_regs ioremap failed, i=%d\n", i);
		}
	}

	if (device_property_read_u32(&pdev->dev, "dma-burst-length", &pdata->dma_burst_length))
		pdata->dma_burst_length = DEFAULT_DMA_BURST_LENGTH;

	if (device_property_read_u32(&pdev->dev, "jumbo-max-length", &pdata->jumbo_len))
		pdata->jumbo_len = DEFAULT_JUMBO_MAX_LENGTH;

	if (device_property_read_u32(&pdev->dev, "queue-number", &queue_num))
		pdata->queues_num = pdata->queues_max_num;
	else
		pdata->queues_num = queue_num;

	pdata->wol = 0;
	if (device_property_read_bool(&pdev->dev, "magic-packet"))
		pdata->wol |= PHYTMAC_WAKE_MAGIC;

	pdata->use_ncsi = device_property_read_bool(&pdev->dev, "use-ncsi");
	pdata->use_mii = device_property_read_bool(&pdev->dev, "use-mii");

	pdata->power_state = PHYTMAC_POWEROFF;

	device_set_wakeup_capable(&pdev->dev, pdata->wol & PHYTMAC_WOL_MAGIC_PACKET);

	for (i = 0; i < pdata->queues_num; i++) {
		pdata->irq_type = IRQ_TYPE_INT;
		pdata->queue_irq[i] = platform_get_irq(pdev, i);
	}

	ret = phytmac_get_phy_mode(pdev);
	if (ret < 0)
		pdata->phy_interface = PHY_INTERFACE_MODE_MII;
	else
		pdata->phy_interface = ret;

	ret = phytmac_drv_probe(pdata);
	if (ret)
		goto err_mem;

	if (netif_msg_probe(pdata)) {
		dev_notice(&pdev->dev, "phytium net device enabled\n");
		dev_dbg(pdata->dev, "use_ncsi:%d, use_mii:%d, wol:%d, queues_num:%d\n",
			pdata->use_ncsi, pdata->use_mii, pdata->wol, pdata->queues_num);
	}

	return 0;

err_mem:
	phytmac_free_pdata(pdata);

err_alloc:
	dev_err(&pdev->dev, "phytium net device not enabled\n");

	return ret;
}

static int phytmac_plat_remove(struct platform_device *pdev)
{
	struct phytmac *pdata = platform_get_drvdata(pdev);

	phytmac_drv_remove(pdata);
	phytmac_free_pdata(pdata);

	return 0;
}

static int __maybe_unused phytmac_plat_suspend(struct device *dev)
{
	struct phytmac *pdata = dev_get_drvdata(dev);
	int ret;

	ret = phytmac_drv_suspend(pdata);

	return ret;
}

static int __maybe_unused phytmac_plat_resume(struct device *dev)
{
	struct phytmac *pdata = dev_get_drvdata(dev);
	int ret;

	ret = phytmac_drv_resume(pdata);

	return ret;
}

static const struct dev_pm_ops phytmac_plat_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(phytmac_plat_suspend, phytmac_plat_resume)
};

static struct platform_driver phytmac_driver = {
	.probe = phytmac_plat_probe,
	.remove = phytmac_plat_remove,
	.driver = {
		.name = PHYTMAC_DRV_NAME,
		.of_match_table = of_match_ptr(phytmac_dt_ids),
		.acpi_match_table = phytmac_acpi_ids,
		.pm = &phytmac_plat_pm_ops,
	},
};

module_platform_driver(phytmac_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Phytium Ethernet driver");
MODULE_AUTHOR("Wenting Song");
MODULE_ALIAS("platform:phytmac");

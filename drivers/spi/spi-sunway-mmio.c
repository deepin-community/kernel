// SPDX-License-Identifier: GPL-2.0
/*
 * Memory-mapped interface driver for SUNWAY CHIP SPI Core
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/property.h>
#include <linux/regmap.h>
#include <linux/acpi.h>

#include "spi-sunway.h"


#define DRIVER_NAME "sunway_chip_spi"

struct chip_spi_mmio {
	struct spi_chip  spi_chip;
	struct clk     *clk;
	void           *priv;
};

static int chip_spi_mmio_probe(struct platform_device *pdev)
{
	int (*init_func)(struct platform_device *pdev,
			 struct chip_spi_mmio *spimmio);
	struct chip_spi_mmio *spimmio;
	struct spi_chip *spi_chip;
	struct resource *mem;
	int ret;
	int num_cs;

	spimmio = devm_kzalloc(&pdev->dev, sizeof(struct chip_spi_mmio),
			GFP_KERNEL);
	if (!spimmio)
		return -ENOMEM;

	spi_chip = &spimmio->spi_chip;

	/* Get basic io resource and map it */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	spi_chip->regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(spi_chip->regs)) {
		dev_err(&pdev->dev, "SPI region map failed\n");
		return PTR_ERR(spi_chip->regs);
	}

	spimmio->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(spimmio->clk))
		return PTR_ERR(spimmio->clk);
	ret = clk_prepare_enable(spimmio->clk);
	if (ret)
		return ret;

	spi_chip->bus_num = pdev->id;
	spi_chip->max_freq = clk_get_rate(spimmio->clk);

	device_property_read_u32(&pdev->dev, "reg-io-width",
				&spi_chip->reg_io_width);

	num_cs = 4;
	device_property_read_u32(&pdev->dev, "num-cs", &num_cs);
	spi_chip->num_cs = num_cs;

	if (pdev->dev.of_node) {
		int i;

		for (i = 0; i < spi_chip->num_cs; i++) {
			int cs_gpio = of_get_named_gpio(pdev->dev.of_node,
					"cs-gpios", i);

			if (cs_gpio == -EPROBE_DEFER) {
				ret = cs_gpio;
				goto out;
			}

			if (gpio_is_valid(cs_gpio)) {
				ret = devm_gpio_request(&pdev->dev, cs_gpio,
						dev_name(&pdev->dev));
				if (ret)
					goto out;
			}
		}
	}

	init_func = device_get_match_data(&pdev->dev);
	if (init_func) {
		ret = init_func(pdev, spimmio);
		if (ret)
			goto out;
	}

	spi_chip->flags = SPI_PLAT;

	ret = spi_chip_add_host(&pdev->dev, spi_chip);
	if (ret)
		goto out;

	platform_set_drvdata(pdev, spimmio);

	dev_info(&pdev->dev, "SPI(MMIO) probe succeed\n");

	return 0;
out:
	clk_disable_unprepare(spimmio->clk);
	return ret;
}

static int chip_spi_mmio_remove(struct platform_device *pdev)
{
	struct chip_spi_mmio *spimmio = platform_get_drvdata(pdev);

	spi_chip_remove_host(&spimmio->spi_chip);
	clk_disable_unprepare(spimmio->clk);

	return 0;
}

static const struct of_device_id chip_spi_mmio_of_match[] = {
	{ .compatible = "sunway,chip-spi",},
	{ /* end of table */}
};
MODULE_DEVICE_TABLE(of, chip_spi_mmio_of_match);

#ifdef CONFIG_ACPI
static const struct acpi_device_id chip_spi_mmio_acpi_match[] = {
	{ "SUNW0008", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, chip_spi_mmio_acpi_match);
#endif

static struct platform_driver chip_spi_mmio_driver = {
	.probe		= chip_spi_mmio_probe,
	.remove		= chip_spi_mmio_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.of_match_table = chip_spi_mmio_of_match,
		.acpi_match_table = ACPI_PTR(chip_spi_mmio_acpi_match),
	},
};
module_platform_driver(chip_spi_mmio_driver);

MODULE_AUTHOR("Platform@wiat.com");
MODULE_DESCRIPTION("Memory-mapped I/O interface driver for Sunway CHIP");
MODULE_LICENSE("GPL v2");

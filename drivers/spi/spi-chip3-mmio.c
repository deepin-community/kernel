// SPDX-License-Identifier: GPL-2.0
/*
 * Memory-mapped interface driver for SUNWAY CHIP3 SPI Core
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

#include "spi-chip3.h"

#define DRIVER_NAME "sunway_chip3_spi"

struct chip3_spi_mmio {
	struct chip3_spi  dws;
	struct clk     *clk;
	void           *priv;
};

static int chip3_spi_mmio_probe(struct platform_device *pdev)
{
	int (*init_func)(struct platform_device *pdev,
			 struct chip3_spi_mmio *dwsmmio);
	struct chip3_spi_mmio *dwsmmio;
	struct chip3_spi *dws;
	struct resource *mem;
	int ret;
	int num_cs;

	dwsmmio = devm_kzalloc(&pdev->dev, sizeof(struct chip3_spi_mmio),
			GFP_KERNEL);
	if (!dwsmmio)
		return -ENOMEM;

	dws = &dwsmmio->dws;

	/* Get basic io resource and map it */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dws->regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(dws->regs)) {
		dev_err(&pdev->dev, "SPI region map failed\n");
		return PTR_ERR(dws->regs);
	}

	dwsmmio->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(dwsmmio->clk))
		return PTR_ERR(dwsmmio->clk);
	ret = clk_prepare_enable(dwsmmio->clk);
	if (ret)
		return ret;

	dws->bus_num = pdev->id;
	dws->max_freq = clk_get_rate(dwsmmio->clk);

	device_property_read_u32(&pdev->dev, "reg-io-width",
				&dws->reg_io_width);

	num_cs = 4;
	device_property_read_u32(&pdev->dev, "num-cs", &num_cs);
	dws->num_cs = num_cs;

	if (pdev->dev.of_node) {
		int i;

		for (i = 0; i < dws->num_cs; i++) {
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
		ret = init_func(pdev, dwsmmio);
		if (ret)
			goto out;
	}

	ret = chip3_spi_add_host(&pdev->dev, dws);
	if (ret)
		goto out;

	platform_set_drvdata(pdev, dwsmmio);

	return 0;
out:
	clk_disable_unprepare(dwsmmio->clk);
	return ret;
}

static int chip3_spi_mmio_remove(struct platform_device *pdev)
{
	struct chip3_spi_mmio *dwsmmio = platform_get_drvdata(pdev);

	chip3_spi_remove_host(&dwsmmio->dws);
	clk_disable_unprepare(dwsmmio->clk);

	return 0;
}

static const struct of_device_id chip3_spi_mmio_of_match[] = {
	{ .compatible = "sunway,chip3-spi", },
	{ /* end of table */}
};
MODULE_DEVICE_TABLE(of, chip3_spi_mmio_of_match);

static struct platform_driver chip3_spi_mmio_driver = {
	.probe		= chip3_spi_mmio_probe,
	.remove		= chip3_spi_mmio_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.of_match_table = chip3_spi_mmio_of_match,
	},
};
module_platform_driver(chip3_spi_mmio_driver);

MODULE_AUTHOR("Platform@wxiat.com");
MODULE_DESCRIPTION("Memory-mapped I/O interface driver for Sunway CHIP3");
MODULE_LICENSE("GPL");

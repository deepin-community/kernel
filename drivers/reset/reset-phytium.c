// SPDX-License-Identifier: GPL-2.0-only
/*
 * Phytium Reset Driver.
 *
 * Copyright (c) 2024, Phytium Technology Co., Ltd.
 */

#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/spinlock.h>

#define RESET_PHYTIUM_DRV_VERSION "1.0.0"

#define PHYTIUM_RESET_MAX_CNTS (19)
#define PHYTIUM_I2C_RESET_ID0  (3)
#define PHYTIUM_RESET_OFFSET_0 (0)
#define PHYTIUM_RESET_OFFSET_1 (4)

struct reset_phytium_dev {
	void __iomem			*base;
	struct device			*dev;
	struct reset_controller_dev     rcdev;
};

struct reset_reg_info {
	u32 reg_offset;
	u32 data;
};
static struct reset_reg_info reset_mng[PHYTIUM_RESET_MAX_CNTS] = {
				{PHYTIUM_RESET_OFFSET_0, (u32)BIT(28)},
				{PHYTIUM_RESET_OFFSET_0, (u32)BIT(29)},
				{PHYTIUM_RESET_OFFSET_0, (u32)BIT(30)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(16)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(17)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(18)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(19)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(20)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(21)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(22)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(23)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(24)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(25)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(26)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(27)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(28)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(29)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(30)},
				{PHYTIUM_RESET_OFFSET_1, (u32)BIT(31)}
				};

static inline struct reset_phytium_dev *
to_reset_phytium_data(struct reset_controller_dev *rcdev)
{
	return container_of(rcdev, struct reset_phytium_dev, rcdev);
}

static int reset_phytium_reset(struct reset_controller_dev *rcdev,
			      unsigned long id)
{
	struct reset_phytium_dev *rdev = to_reset_phytium_data(rcdev);
	u32 reg_val, reset_reg_val;

	if (id >= PHYTIUM_RESET_MAX_CNTS) {
		dev_err(rdev->dev, "The reset id is out of range %ld\n", id);
		return -EINVAL;
	}

	reg_val = readl_relaxed(rdev->base + reset_mng[id].reg_offset);
	reset_reg_val = reg_val;
	reg_val &= ~reset_mng[id].data;
	writel_relaxed(reg_val, rdev->base + reset_mng[id].reg_offset);
	writel_relaxed(reset_reg_val, rdev->base + reset_mng[id].reg_offset);

	return 0;
}

const struct reset_control_ops reset_phytium_ops = {
	.reset		= reset_phytium_reset,
};
EXPORT_SYMBOL_GPL(reset_phytium_ops);

static const struct of_device_id reset_phytium_dt_ids[] = {
	{ .compatible = "phytium,reset", },
	{ /* sentinel */ },
};

static int reset_phytium_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct reset_phytium_dev *rdev;
	struct resource *res;

	rdev = devm_kzalloc(dev, sizeof(*rdev), GFP_KERNEL);
	if (!rdev)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	rdev->base = devm_ioremap_resource(dev, res);
	if (IS_ERR(rdev->base))
		return PTR_ERR(rdev->base);

	rdev->rcdev.owner = THIS_MODULE;
	rdev->rcdev.nr_resets = PHYTIUM_RESET_MAX_CNTS;
	rdev->rcdev.ops = &reset_phytium_ops;
	rdev->rcdev.of_node = dev->of_node;
	rdev->dev = &pdev->dev;

	return devm_reset_controller_register(dev, &rdev->rcdev);
}

static struct platform_driver reset_phytium_driver = {
	.probe	= reset_phytium_probe,
	.driver = {
		.name		= "phytium-reset",
		.of_match_table	= reset_phytium_dt_ids,
	},
};
builtin_platform_driver(reset_phytium_driver);

MODULE_AUTHOR("Wu Jinyong <wujinyong1788@phytium.com.cn>");
MODULE_DESCRIPTION("Phytium reset");
MODULE_LICENSE("GPL");
MODULE_VERSION(RESET_PHYTIUM_DRV_VERSION);

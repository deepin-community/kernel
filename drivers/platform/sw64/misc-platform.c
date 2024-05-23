// SPDX-License-Identifier: GPL-2.0

#define pr_fmt(fmt) "sunway-misc-platform: " fmt

#include <linux/of.h>
#include <linux/acpi.h>
#include <linux/platform_device.h>

#include <asm/sw64io.h>

struct misc_platform {
	void __iomem *spbu_base;
	void __iomem *intpu_base;
	void __iomem *gpio_base;
};

static struct misc_platform misc_platform_devices[MAX_NUMNODES];

#ifdef CONFIG_OF
static const struct of_device_id misc_platform_of_match[] = {
	{ .compatible = "sunway,misc-platform" },
	{},
};
#endif

#ifdef CONFIG_ACPI
static const struct acpi_device_id misc_platform_acpi_match[] = {
	{ "SUNW0200", 0 },
	{},
};
#endif

static int misc_platform_get_node(struct device *dev)
{
	int nid = NUMA_NO_NODE;
	unsigned long long pxm;
	struct device_node *np;
	acpi_status status;

	/* Try to directly get the physical node ID */
	if (acpi_disabled) {
		/**
		 * We don't use the function of_node_to_nid()
		 * in case CONFIG_NUMA=n.
		 */
		np = of_node_get(dev->of_node);
		of_property_read_u32(np, "numa-node-id", &nid);
		of_node_put(np);
	} else {
		/**
		 * We don't use the function acpi_get_node() here
		 * beacuse we want physical node ID instead of the
		 * logical one.
		 */
		status = acpi_evaluate_integer(ACPI_HANDLE(dev),
				"_PXM", NULL, &pxm);
		if (ACPI_SUCCESS(status))
			nid = (int)pxm;
	}

	return nid;
}

static int misc_platform_probe(struct platform_device *pdev)
{
	int ret, node;
	u64 base_address;
	void __iomem *spbu_base = NULL;
	void __iomem *intpu_base = NULL;
	void __iomem *gpio_base = NULL;
	struct device *dev = &pdev->dev;

	node = misc_platform_get_node(dev);
	if (node == NUMA_NO_NODE) {
		pr_err("unable to get node ID\n");
		return ret;
	}

	if (!device_property_read_u64(dev, "sunway,spbu_base",
				&base_address))
		spbu_base = __va(base_address);

	if (!device_property_read_u64(dev, "sunway,intpu_base",
				&base_address))
		intpu_base = __va(base_address);

	if (!device_property_read_u64(dev, "sunway,gpio_base",
				&base_address))
		gpio_base = __va(base_address);

	misc_platform_devices[node].spbu_base = spbu_base;
	misc_platform_devices[node].intpu_base = intpu_base;
	misc_platform_devices[node].gpio_base = gpio_base;

	pr_info("misc-platform on node %d found\n", node);

	return 0;
}

void __iomem *misc_platform_get_spbu_base(unsigned long node)
{
	void __iomem *spbu_base;

	if (node >= MAX_NUMNODES)
		return NULL;

	spbu_base = misc_platform_devices[node].spbu_base;

	/* Fallback to legacy address */
	if (!spbu_base)
		return __va(SW64_IO_BASE(node) | SPBU_BASE);

	return spbu_base;
}

void __iomem *misc_platform_get_intpu_base(unsigned long node)
{
	void __iomem *intpu_base;

	if (node >= MAX_NUMNODES)
		return NULL;

	intpu_base = misc_platform_devices[node].intpu_base;

	/* Fallback to legacy address */
	if (!intpu_base)
		return __va(SW64_IO_BASE(node) | INTPU_BASE);

	return intpu_base;
}

void __iomem *misc_platform_get_gpio_base(unsigned long node)
{
	void __iomem *gpio_base;

	if (node >= MAX_NUMNODES)
		return NULL;

	gpio_base = misc_platform_devices[node].gpio_base;

	/* Fallback to legacy address */
	if (!gpio_base)
		return __va(SW64_IO_BASE(node) | GPIO_BASE);

	return gpio_base;
}

#ifdef CONFIG_SUBARCH_C3B
void __iomem *misc_platform_get_cab0_base(unsigned long node)
{
	return __va(SW64_IO_BASE(node) | CAB0_BASE);
}
#endif

static struct platform_driver misc_platform_driver = {
	.probe = misc_platform_probe,
	.driver = {
		.name = "sunway-misc-platform",
		.of_match_table = of_match_ptr(misc_platform_of_match),
		.acpi_match_table = ACPI_PTR(misc_platform_acpi_match),
	},
};

static int __init misc_platform_driver_init(void)
{
	return platform_driver_register(&misc_platform_driver);
}
arch_initcall(misc_platform_driver_init);


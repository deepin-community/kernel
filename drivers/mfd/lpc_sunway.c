// SPDX-License-Identifier: GPL-2.0
/*
 *  lpc_sunway_chip3.c - LPC interface for SUNWAY CHIP3
 *
 *  LPC bridge function contains many other functional units,
 *  such as Interrupt controllers, Timers, Power Management,
 *  System Management, GPIO, RTC, and LPC Configuration
 *  Registers.
 *
 *  Copyright (c) 2014 JN
 *  Author: Weiqiang Su <paper_purple@163.com>
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/acpi.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/mfd/core.h>
#include <linux/slab.h>
#include <linux/sizes.h>
#include <linux/mtd/physmap.h>

enum features {
	LPC_USE_MSI = (1 << 0),
	LPC_USE_INTX = (1 << 1),
};

enum {
	LPC_HST_BAR = 0,
	LPC_MEM_BAR = 2,
	LPC_FWH_BAR = 4,
};

enum {
	LPC_CTL = 0x0,
	LPC_IRQ = 0x4,
	LPC_IRQ_MASK = 0x8,
	LPC_STAT = 0xc,
	LPC_ERR_INF = 0x10,
	LPC_MEM_HADDR = 0x14,
	LPC_FWH_IDSEL_R1 = 0x18,
	LPC_FWH_IDSEL_R2 = 0x1c,
	LPC_FWH_IDSEL_R3 = 0x20,
	LPC_FWH_IDSEL_R4 = 0x24,
	LPC_FWH_IDSEL_R5 = 0x28,
	LPC_FWH_DEC_EN1 = 0x2c,
	LPC_FWH_DEC_EN2 = 0x30,
	LPC_DMA_CTL = 0x34,
	LPC_CH_STAT = 0x38,
	LPC_CH0_ADDR = 0x3c,
	LPC_CH1_ADDR = 0x40,
	LPC_CH2_ADDR = 0x44,
	LPC_CH3_ADDR = 0x48,
	LPC_CH0_LENG = 0x4c,
	LPC_CH1_LENG = 0x50,
	LPC_CH2_LENG = 0x54,
	LPC_CH3_LENG = 0x58,
	LPC_CH0_MODE = 0x5c,
	LPC_CH1_MODE = 0x60,
	LPC_CH2_MODE = 0x64,
	LPC_CH3_MODE = 0x68,
	LPC_CH_MASK = 0x6c,
	LPC_DMA_SWRST = 0x70,
};

struct lpc_chip3_adapter {
	void __iomem *hst_regs;
	struct device *dev;
	int irq;
	unsigned int features;
};

static struct resource superio_chip3_resources[] = {
	{
		.flags = IORESOURCE_IO,
#ifdef CONFIG_SUBARCH_C4
		.start = LPC_MEM_IO,
		.end = LPC_MEM_IO + 0x10,
#endif
	}
};

static struct resource mem_flash_resource = {
	.flags = IORESOURCE_MEM,
};

static struct resource fw_flash_resource = {
	.flags = IORESOURCE_MEM,
};

static struct physmap_flash_data mem_flash_data = {
	.width = 1,
};

static struct physmap_flash_data fw_flash_data = {
	.width = 1,
};

static struct mfd_cell lpc_chip3_cells[] = {
	{
	 .name = "sunway_superio_ast2400",
	 .id = 0,
	 .num_resources = ARRAY_SIZE(superio_chip3_resources),
	 .resources = superio_chip3_resources,
	},
	{
	 .name = "chip3-flash",
	 .id = 0,
	 .num_resources = 1,
	 .resources = &mem_flash_resource,
	 .platform_data = &mem_flash_data,
	 .pdata_size = sizeof(mem_flash_data),
	},
	{
	 .name = "chip3_fwh-flash",
	 .id = 0,
	 .num_resources = 1,
	 .resources = &fw_flash_resource,
	 .platform_data = &fw_flash_data,
	 .pdata_size = sizeof(fw_flash_data),
	}
};

static inline void lpc_writel(void *address, int reg_base, int value)
{
	unsigned long addr = (unsigned long)address + reg_base;

	writel(value, (void *)addr);
}

static inline int lpc_readl(void *address, int reg_base)
{
	unsigned long addr = (unsigned long)address + reg_base;
	int value = readl((void *)addr);

	return value;
}

static void lpc_enable(struct lpc_chip3_adapter *lpc_adapter)
{
	unsigned int value;

	value = lpc_readl(lpc_adapter->hst_regs, LPC_CTL);
	value |= 0x1600;

	/* LPC host enable */
	lpc_writel(lpc_adapter->hst_regs, LPC_CTL, value);
}

static void lpc_mem_flash_init(struct platform_device *pdev,
			       struct lpc_chip3_adapter *lpc_adapter)
{
	mem_flash_resource.start =
	    (((unsigned long)(lpc_adapter->hst_regs) & (~(0xfUL << 28))) | (0x2UL << 28));
	mem_flash_resource.end = mem_flash_resource.start + SZ_256M - 1;

	writel(0x1f, lpc_adapter->hst_regs + LPC_MEM_HADDR);
}

static void lpc_fw_flash_init(struct platform_device *pdev,
			      struct lpc_chip3_adapter *lpc_adapter)
{
	fw_flash_resource.start =
	    (((unsigned long)(lpc_adapter->hst_regs) & (~(0xfUL << 28))) | (0x3UL << 28));
	fw_flash_resource.end = fw_flash_resource.start + SZ_256M - 1;

	writel(0xff0f, lpc_adapter->hst_regs + LPC_FWH_DEC_EN1);
	writel(0xffff11ff, lpc_adapter->hst_regs + LPC_FWH_IDSEL_R5);
	writel(0xffffffff, lpc_adapter->hst_regs + LPC_FWH_IDSEL_R4);
	writel(0xffffffff, lpc_adapter->hst_regs + LPC_FWH_IDSEL_R3);
	writel(0xffffffff, lpc_adapter->hst_regs + LPC_FWH_IDSEL_R2);
	writel(0xffffffff, lpc_adapter->hst_regs + LPC_FWH_IDSEL_R1);

}

static int lpc_chip3_probe(struct platform_device *pdev)
{
	int ret;
	struct lpc_chip3_adapter *lpc_adapter;
	struct resource *mem;

	lpc_adapter = kzalloc(sizeof(*lpc_adapter), GFP_KERNEL);
	if (lpc_adapter == NULL) {
		dev_err(&pdev->dev, "%s kzalloc failed !\n", __func__);
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, lpc_adapter);
	/* Get basic io resource and map it */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -EINVAL;
	}

	lpc_adapter->hst_regs = devm_ioremap_resource(&pdev->dev, mem);
	if (IS_ERR(lpc_adapter->hst_regs)) {
		dev_err(&pdev->dev, "lpc region map failed\n");
		return PTR_ERR(lpc_adapter->hst_regs);
	}

	lpc_adapter->dev = &pdev->dev;
	lpc_adapter->features = 0;

	lpc_enable(lpc_adapter);

	lpc_mem_flash_init(pdev, lpc_adapter);
	lpc_fw_flash_init(pdev, lpc_adapter);

	ret = mfd_add_devices(&pdev->dev, 0,
			      lpc_chip3_cells, ARRAY_SIZE(lpc_chip3_cells),
			      NULL, 0, NULL);
	if (ret)
		goto out_dev;

	dev_info(lpc_adapter->dev, "probe succeed !\n");

	return ret;

out_dev:
	dev_info(lpc_adapter->dev, "probe failed !\n");

	mfd_remove_devices(&pdev->dev);
	kfree(lpc_adapter);

	return ret;
}

static int lpc_chip3_remove(struct platform_device *pdev)
{
	struct lpc_chip3_adapter *lpc_adapter = platform_get_drvdata(pdev);

	mfd_remove_devices(&pdev->dev);
	iounmap(lpc_adapter->hst_regs);
	kfree(lpc_adapter);

	return 0;
}

static const struct of_device_id chip3_lpc_of_match[] = {
	{.compatible = "sunway,chip3_lpc",},
	{ /* end of table */ }
};
MODULE_DEVICE_TABLE(of, chip3_lpc_of_match);

#ifdef CONFIG_ACPI
static const struct acpi_device_id chip3_lpc_acpi_match[] = {
	{ "SUNW0006", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, chip3_lpc_acpi_match);
#endif

#ifdef CONFIG_PM_SLEEP
unsigned int lpc_irq_ctrl_value;
unsigned int lpc_irq_irq_value;
unsigned int lpc_irq_mask_value;

/**
 * chip3_lpc_platform_suspend - Suspend an chip3_lpc-platform device
 * @dev: the platform device to suspend
 *
 * This function stores the lpc controller register values and
 * restores them when the machine wakes up.
 */
int chip3_lpc_platform_suspend(struct device *dev)
{
	struct lpc_chip3_adapter *lpc_adapter = dev_get_drvdata(dev);

	lpc_irq_ctrl_value = lpc_readl(lpc_adapter->hst_regs, LPC_CTL);
	lpc_irq_irq_value = lpc_readl(lpc_adapter->hst_regs, LPC_IRQ);
	lpc_irq_mask_value = lpc_readl(lpc_adapter->hst_regs, LPC_IRQ_MASK);

	return 0;
}

/**
 * chip3_lpc_platform_resume - Resume an chip3_lpc-platform device
 * @dev: the platform device to resume
 *
 * This function restores the register value before the suspend.
 */
int chip3_lpc_platform_resume(struct device *dev)
{
	struct lpc_chip3_adapter *lpc_adapter = dev_get_drvdata(dev);

	lpc_writel(lpc_adapter->hst_regs, LPC_CTL, lpc_irq_ctrl_value);
	lpc_writel(lpc_adapter->hst_regs, LPC_IRQ, lpc_irq_irq_value);
	lpc_writel(lpc_adapter->hst_regs, LPC_IRQ_MASK, lpc_irq_mask_value);

	return 0;
}
static SIMPLE_DEV_PM_OPS(chip3_lpc_pm_ops, chip3_lpc_platform_suspend,
			 chip3_lpc_platform_resume);
#endif


static struct platform_driver chip3_lpc_platform_driver = {
	.driver = {
		   .name = "chip3_lpc",
		   .of_match_table = chip3_lpc_of_match,
		   .acpi_match_table = ACPI_PTR(chip3_lpc_acpi_match),
#ifdef CONFIG_PM_SLEEP
		   .pm = &chip3_lpc_pm_ops,
#endif
		   },
	.remove = lpc_chip3_remove,
};

static int __init chip3_lpc_drvinit(void)
{
	return platform_driver_probe(&chip3_lpc_platform_driver,
				     lpc_chip3_probe);
}

/*
 * lpc controller init configure before serial drivers;
 * The lpc & ast2400 should be initialized much before
 * the serial initialized functions are called.
 */
subsys_initcall_sync(chip3_lpc_drvinit);

static void __exit chip3_lpc_drvexit(void)
{
	platform_driver_unregister(&chip3_lpc_platform_driver);
}

module_exit(chip3_lpc_drvexit);

MODULE_AUTHOR("Weiqiang Su <paper_purple@163.com>");
MODULE_DESCRIPTION("LPC Interface for CHIP3");
MODULE_LICENSE("GPL");

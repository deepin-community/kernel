// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium Localbus Controller driver.
 *
 * Copyright (c) 2023-2024, Phytium Technology Co., Ltd.
 */
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/io.h>
#include <asm/byteorder.h>

#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/reboot.h>
#include <linux/bitmap.h>
#include <linux/mtd/map.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/cfi.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <linux/property.h>


#define LBC_CSAR(n)			(0x018+8*n)
#define LBC_CSASR(n)			(0x01c+8*n)
#define LBC_CSPR(n)			(0x058+4*n)
#define LBC_CFG_SEQ_ADDR		0x130
#define LBC_CFG_SEQ_DATA		0x134
#define LBC_CFG_SEQ_STAT		0x138
#define LBC_FTIM0_CS_NOR(n)		(0x13c+0xc*n)
#define LBC_FTIM1_CS_NOR(n)		(0x140+0xc*n)	//read seq
#define LBC_FTIM2_CS_NOR(n)		(0x144+0xc*n)	//write seq
#define LBC_FTIM0_CS_GPCM(n)		(0x19c+0xc*n)
#define LBC_FTIM1_CS_GPCM(n)		(0x1a0+0xc*n)
#define LBC_FTIM2_CS_GPCM(n)		(0x1a4+0xc*n)

static uint64_t LBC_DEVICE_ADDR;
#define PHYTIUM_MAX_SRAM_BLOCK		8

#define PHYTIUM_LOCALBUS_DRIVER_VERSION	"1.0.0"

struct phytium_lbc_dev {
	struct resource *res;
	struct mtd_info *mtd;
	struct map_info *map;
	u32 device_type;	// nor flash, sync-ram
};

struct phytium_lbc {
	struct phytium_lbc_dev dev[PHYTIUM_MAX_SRAM_BLOCK];
	u8  dev_num;
	void __iomem *io_base;
	void __iomem *mm_base;
	resource_size_t mm_size;
	u32 total_device_size;
};

//functions for read, write, copy to, copy from, etc.
static map_word lbc_read(struct map_info *map, unsigned long adr)
{
	map_word temp;
	unsigned long i = adr;

	temp.x[0] = readw_relaxed(map->virt + i);
	return temp;
}


static void lbc_write(struct map_info *map, map_word d, unsigned long adr)
{
	u8 tmp[2] = {0};
	unsigned long i = adr;

	if ((adr & 0x1) == 0)
		writew_relaxed(d.x[0], map->virt+i);
	else {
		tmp[1] = d.x[0];
		writew_relaxed(*tmp, map->virt + i-1);
	}
}

static void lbc_copy_from(struct map_info *map, void *to,
	unsigned long from, ssize_t len)
{
	unsigned char *f = (unsigned char *)map->virt + from;
	unsigned char *t = (unsigned char *)to;

	memcpy_fromio(t, f, len);
}

static void lbc_copy_to(struct map_info *map, unsigned long to,
	const void *from, ssize_t len)
{
	unsigned char *f = (unsigned char *)from;
	unsigned char *t = (unsigned char *)map->virt + to;

	memcpy_toio(t, f, len);
}

static int phytium_lbc_calc_device_size(int device_size)
{
	u32 value;

	switch (device_size) {
	case SZ_16K:
		value = 0x0;
		break;
	case SZ_32K:
		value = 0x1;
		break;
	case SZ_64K:
		value = 0x2;
		break;
	case SZ_128K:
		value = 0x3;
		break;
	case SZ_256K:
		value = 0x4;
		break;
	case SZ_512K:
		value = 0x5;
		break;
	case SZ_1M:
		value = 0x6;
		break;
	case SZ_2M:
		value = 0x7;
		break;
	case SZ_4M:
		value = 0x8;
		break;
	case SZ_8M:
		value = 0x9;
		break;
	case SZ_16M:
		value = 0xa;
		break;
	case SZ_32M:
		value = 0xb;
		break;
	case SZ_64M:
		value = 0xc;
		break;
	case SZ_128M:
		value = 0xd;
		break;
	case SZ_256M:
		value = 0xe;
		break;
	default:
		value = -EINVAL;
		pr_err("lbc: device size is invalid or not supported.\n");
		break;
	}

	return value;
}

static int phytium_lbc_time_seq_init(struct phytium_lbc *lbc, u8 dev_num, u32 device_time_seq[])
{
	switch (lbc->dev[dev_num].device_type) {
	case 1:
		writel_relaxed(device_time_seq[0], lbc->io_base + LBC_CSPR(dev_num));
		writel_relaxed(device_time_seq[1], lbc->io_base + LBC_FTIM0_CS_NOR(dev_num));
		writel_relaxed(device_time_seq[2], lbc->io_base + LBC_FTIM1_CS_NOR(dev_num));
		writel_relaxed(device_time_seq[3], lbc->io_base + LBC_FTIM2_CS_NOR(dev_num));
		break;
	case 2:
		writel_relaxed(device_time_seq[0], lbc->io_base + LBC_CSPR(dev_num));
		writel_relaxed(device_time_seq[1], lbc->io_base + LBC_FTIM0_CS_GPCM(dev_num));
		writel_relaxed(device_time_seq[2], lbc->io_base + LBC_FTIM1_CS_GPCM(dev_num));
		writel_relaxed(device_time_seq[3], lbc->io_base + LBC_FTIM2_CS_GPCM(dev_num));
		break;
	case 3:
		writel_relaxed(device_time_seq[0], lbc->io_base + LBC_CSPR(dev_num));
		writel_relaxed(device_time_seq[1], lbc->io_base + LBC_FTIM0_CS_NOR(dev_num));
		writel_relaxed(device_time_seq[2], lbc->io_base + LBC_FTIM1_CS_NOR(dev_num));
		writel_relaxed(device_time_seq[3], lbc->io_base + LBC_FTIM2_CS_NOR(dev_num));
		break;
	case 4:
		writel_relaxed(device_time_seq[0], lbc->io_base + LBC_CSPR(dev_num));
		writel_relaxed(device_time_seq[1], lbc->io_base + LBC_FTIM0_CS_GPCM(dev_num));
		writel_relaxed(device_time_seq[2], lbc->io_base + LBC_FTIM1_CS_GPCM(dev_num));
		writel_relaxed(device_time_seq[3], lbc->io_base + LBC_FTIM2_CS_GPCM(dev_num));
		break;
	default:
		return -EINVAL;
	}
	return 0;
}
static int phytium_lbc_dev_init(struct phytium_lbc *lbc, u8 dev_num, u32 device_time_seq[])
{
	int ret = 0;
	int err;
	u32 tmp = lbc->dev[dev_num].map->phys >> 12;

	writel_relaxed(tmp, lbc->io_base+LBC_CSAR(dev_num));
	ret = phytium_lbc_calc_device_size(lbc->dev[dev_num].map->size);
	if (ret >= 0)
		writel_relaxed(ret, lbc->io_base+LBC_CSASR(dev_num));
	else
		return ret;
	//sequence
	err = phytium_lbc_time_seq_init(lbc, dev_num, device_time_seq);
	if (err) {
		pr_err("lbc: time sequence init failed");
		return err;
	}
	return ret;
}

static int phytium_lbc_of_setup(struct phytium_lbc *lbc,
			struct platform_device *pdev, struct device_node *np, u8 dev_num)
{
	int err = 0;
	int bankwidth;
	int device_size;
	int device_type;
	u32 device_time_seq[4];
	struct device *dev = &pdev->dev;
	int ret;

	of_property_read_u32(np, "bank-width", &bankwidth);
	if (!bankwidth)
		return -EINVAL;

	of_property_read_u32(np, "device-size", &device_size);
	if (!device_size)
		return -EINVAL;
	if (device_size > SZ_256M) {
		dev_err(dev, "device size is too large\n");
		return -EINVAL;
	}
	if (device_size < SZ_16K) {
		dev_err(dev, "device size is too small\n");
		return -EINVAL;
	}

	of_property_read_u32(np, "device-type", &device_type);
	if (!device_type)
		return -EINVAL;
	lbc->dev[dev_num].device_type = device_type;
	dev_dbg(dev, "device_type=%d", lbc->dev[dev_num].device_type);

	if (!lbc->dev[dev_num].device_type) {
		dev_err(dev, "device type invalid or not find");
		return -EINVAL;
	}

	ret = of_property_read_u32_array(np, "device-time-seq", device_time_seq, 4);
	if (ret) {
		dev_err(dev, "device time sequence invalid or not found\n");
		return -EINVAL;
	}

	lbc->total_device_size = lbc->total_device_size + device_size;
	if (lbc->total_device_size > lbc->mm_size) {
		dev_err(dev, "total device size is too large\n");
		return -EINVAL;
	}

	lbc->dev[dev_num].map = devm_kzalloc(&pdev->dev, sizeof(struct map_info), GFP_KERNEL);
	if (!lbc->dev[dev_num].map)
		return -ENOMEM;

	lbc->dev[dev_num].map->size = device_size;
	if (dev_num == 0) {
		lbc->dev[dev_num].map->phys = LBC_DEVICE_ADDR;
		lbc->dev[dev_num].map->virt = lbc->mm_base;
	} else {
		lbc->dev[dev_num].map->phys = lbc->dev[dev_num-1].map->phys + device_size;
		lbc->dev[dev_num].map->virt = lbc->dev[dev_num-1].map->virt + device_size;
	}

	err = phytium_lbc_dev_init(lbc, dev_num, device_time_seq);
	if (err < 0) {
		dev_err(dev, "device size is invalid\n");
		return -EINVAL;
	}

	lbc->dev[dev_num].map->name = np->name;
	lbc->dev[dev_num].map->bankwidth = bankwidth;
	lbc->dev[dev_num].map->read = lbc_read;
	lbc->dev[dev_num].map->write = lbc_write;
	lbc->dev[dev_num].map->copy_from = lbc_copy_from;
	lbc->dev[dev_num].map->copy_to = lbc_copy_to;

	lbc->dev[dev_num].map->device_node = np;

	switch (lbc->dev[dev_num].device_type) {
	case 1:
		lbc->dev[dev_num].mtd = do_map_probe("cfi_probe", lbc->dev[dev_num].map);
		break;
	case 2:
		lbc->dev[dev_num].mtd = do_map_probe("map_ram", lbc->dev[dev_num].map);
		break;
	case 3:
		lbc->dev[dev_num].mtd = do_map_probe("cfi_probe", lbc->dev[dev_num].map);
		break;
	case 4:
		lbc->dev[dev_num].mtd = do_map_probe("map_ram", lbc->dev[dev_num].map);
		break;
	default:
		dev_err(dev, "failed to alloc probe func");
		return -EINVAL;
	}

	if (!lbc->dev[dev_num].mtd) {
		dev_err(&pdev->dev, "device tree probing failed\n");
		return -ENXIO;
	}

	lbc->dev[dev_num].mtd->dev.parent = &pdev->dev;
	mtd_set_of_node(lbc->dev[dev_num].mtd, pdev->dev.of_node);

	err = mtd_device_register(lbc->dev[dev_num].mtd, NULL, 0);
	if (err)
		dev_err(&pdev->dev, "failed to add partitions\n");
	return err;
}

static int phytium_lbc_acpi_setup(struct phytium_lbc *lbc,
			struct platform_device *pdev, struct fwnode_handle *fh,
			u8 dev_num)
{
	int err = 0;
	int bankwidth;
	int device_size;
	int device_type;
	u32 device_time_seq[4];
	struct device *dev = &pdev->dev;
	int ret;

	fwnode_property_read_u32(fh, "bank-width", &bankwidth);
	if (!bankwidth)
		return -EINVAL;

	fwnode_property_read_u32(fh, "device-size", &device_size);
	if (!device_size)
		return -EINVAL;
	if (device_size > SZ_256M) {
		dev_err(dev, "device size is too large\n");
		return -EINVAL;
	}
	if (device_size < SZ_16K) {
		dev_err(dev, "device size is too small\n");
		return -EINVAL;
	}

	fwnode_property_read_u32(fh, "device-type", &device_type);
	if (!device_type)
		return -EINVAL;
	lbc->dev[dev_num].device_type = device_type;
	dev_dbg(dev, "device_type = %d", lbc->dev[dev_num].device_type);

	if (!lbc->dev[dev_num].device_type) {
		dev_err(dev, "device type invalid or not find");
		return -EINVAL;
	}

	ret = fwnode_property_read_u32_array(fh, "device-time-seq", device_time_seq, 4);
	if (ret) {
		dev_err(dev, "device time sequence invalid or not found\n");
		return -EINVAL;
	}

	lbc->total_device_size = lbc->total_device_size + device_size;
	if (lbc->total_device_size > lbc->mm_size) {
		dev_err(dev, "total device size is too large\n");
		return -EINVAL;
	}

	lbc->dev[dev_num].map = devm_kzalloc(&pdev->dev, sizeof(struct map_info), GFP_KERNEL);
	if (!lbc->dev[dev_num].map)
		return -ENOMEM;

	lbc->dev[dev_num].map->size = device_size;
	if (dev_num == 0) {
		lbc->dev[dev_num].map->phys = LBC_DEVICE_ADDR;
		lbc->dev[dev_num].map->virt = lbc->mm_base;
	} else {
		lbc->dev[dev_num].map->phys = lbc->dev[dev_num-1].map->phys + device_size;
		lbc->dev[dev_num].map->virt = lbc->dev[dev_num-1].map->virt + device_size;
	}

	err = phytium_lbc_dev_init(lbc, dev_num, device_time_seq);
	if (err < 0) {
		dev_err(dev, "device size is invalid\n");
		return -EINVAL;
	}

	//there is something wrong, because struct fwnode_handle has no variable
	//called "name", need fixing
	lbc->dev[dev_num].map->bankwidth = bankwidth;
	lbc->dev[dev_num].map->read = lbc_read;
	lbc->dev[dev_num].map->write = lbc_write;
	lbc->dev[dev_num].map->copy_from = lbc_copy_from;
	lbc->dev[dev_num].map->copy_to = lbc_copy_to;

	lbc->dev[dev_num].map->fwnode_handle = fh;

	switch (lbc->dev[dev_num].device_type) {
	case 1:
		lbc->dev[dev_num].mtd = do_map_probe("cfi_probe", lbc->dev[dev_num].map);
		break;
	case 2:
		lbc->dev[dev_num].mtd = do_map_probe("map_ram", lbc->dev[dev_num].map);
		break;
	case 3:
		lbc->dev[dev_num].mtd = do_map_probe("cfi_probe", lbc->dev[dev_num].map);
		break;
	case 4:
		lbc->dev[dev_num].mtd = do_map_probe("map_ram", lbc->dev[dev_num].map);
		break;
	default:
		dev_err(dev, "failed to alloc probe func");
		return -EINVAL;
	}

	if (!lbc->dev[dev_num].mtd) {
		dev_err(&pdev->dev, "acpi probing failed\n");
		return -ENXIO;
	}

	lbc->dev[dev_num].mtd->dev.parent = &pdev->dev;
	lbc->dev[dev_num].mtd->dev.fwnode = pdev->dev.fwnode;

	err = mtd_device_register(lbc->dev[dev_num].mtd, NULL, 0);
	if (err)
		dev_err(&pdev->dev, "failed to add partitions\n");
	return err;
}

static int phytium_lbc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct phytium_lbc *lbc;
	struct device_node *np;
	struct fwnode_handle *child_handle;
	u8 dev_num = 0;
	int ret;
	int i;
	struct resource *res;
	const char **reg_names_array;

	lbc = devm_kzalloc(dev, sizeof(struct phytium_lbc), GFP_KERNEL);
	if (!lbc)
		return -ENOMEM;

	device_for_each_child_node(dev, child_handle)
		dev_num++;
	lbc->dev_num = dev_num;

	if (!lbc->dev_num || lbc->dev_num > PHYTIUM_MAX_SRAM_BLOCK) {
		dev_err(dev, "no device deceted, or device number is too large\n");
		kfree(lbc);
		return -ENODEV;
	}
	dev_num = 0;
	reg_names_array = kcalloc(4, sizeof(*reg_names_array), GFP_KERNEL);
	if (dev->of_node) {
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "localbus");
		lbc->io_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(lbc->io_base)) {
			ret = PTR_ERR(lbc->io_base);
			kfree(lbc);
			return ret;
		}
		res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "lbc_mm");
		lbc->mm_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(lbc->mm_base)) {
			ret = PTR_ERR(lbc->mm_base);
			kfree(lbc);
			return ret;
		}
	} else if (has_acpi_companion(dev)) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		fwnode_property_read_string_array(dev->fwnode, "reg-names", reg_names_array, 2);
		res->name = reg_names_array[0];
		lbc->io_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(lbc->io_base)) {
			ret = PTR_ERR(lbc->io_base);
			kfree(lbc);
			return ret;
		}

		res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
		res->name = reg_names_array[1];
		lbc->mm_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(lbc->mm_base)) {
			ret = PTR_ERR(lbc->mm_base);
			kfree(lbc);
			return ret;
		}
	}
	LBC_DEVICE_ADDR = res->start;
	lbc->mm_size = resource_size(res);
	lbc->total_device_size = 0;

	platform_set_drvdata(pdev, lbc);

	if (dev->of_node) {
		for_each_available_child_of_node(dev->of_node, np) {
			ret = phytium_lbc_of_setup(lbc, pdev, np, dev_num);
			if (ret) {
				dev_err(dev, "unable to setup new device\n");
				goto err_destroy;
			}
			dev_num++;
			lbc->dev_num = dev_num;
		}
	} else if (has_acpi_companion(dev)) {
		device_for_each_child_node(dev, child_handle) {
			ret = phytium_lbc_acpi_setup(lbc, pdev, child_handle, dev_num);
			if (ret) {
				dev_err(dev, "unable to setup new device\n");
				goto err_destroy;
			}
			dev_num++;
			lbc->dev_num = dev_num;
		}
	}

	return 0;

err_destroy:
	for (i = 0; i < dev_num; i++)
		map_destroy(lbc->dev[i].mtd);
	return ret;
}

static int phytium_lbc_remove(struct platform_device *pdev)
{

	struct phytium_lbc *lbc = platform_get_drvdata(pdev);
	int i = 0;

	for (i = 0; i < lbc->dev_num; i++) {
		mtd_device_unregister(lbc->dev[i].mtd);
		map_destroy(lbc->dev[i].mtd);
	}

	return 0;
}

static int __maybe_unused phytium_lbc_suspend(struct device *dev)
{
	return pm_runtime_force_suspend(dev);
}

static int __maybe_unused phytium_lbc_resume(struct device *dev)
{
	return pm_runtime_force_resume(dev);
}

static const struct dev_pm_ops phytium_lbc_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(phytium_lbc_suspend, phytium_lbc_resume)
};

static const struct of_device_id phytium_lbc_of_match[] = {
	{.compatible = "phytium,localbus"},
	{ }
};
MODULE_DEVICE_TABLE(of, phytium_lbc_of_match);
static const struct acpi_device_id phytium_lbc_acpi_match[] = {
	{"PHYT0053", 0},
	{ }
};
MODULE_DEVICE_TABLE(acpi, phytium_lbc_acpi_match);

static struct platform_driver phytium_lbc_driver = {
	.probe		= phytium_lbc_probe,
	.remove		= phytium_lbc_remove,
	.driver		= {
		.name	= "phytium_lbc",
		.of_match_table = phytium_lbc_of_match,
		.acpi_match_table = phytium_lbc_acpi_match,
	},
};

static int __init phytium_lbc_init(void)
{
	int err;

	err = platform_driver_register(&phytium_lbc_driver);
	return err;
}

static void __exit phytium_lbc_exit(void)
{
	platform_driver_unregister(&phytium_lbc_driver);
}

module_init(phytium_lbc_init);
module_exit(phytium_lbc_exit);

MODULE_AUTHOR("Hanmo Wang <Wanghanmo2242@phytium.com.cn>");
MODULE_DESCRIPTION("Phytium LBC driver for devices");
MODULE_LICENSE("GPL");
MODULE_VERSION(PHYTIUM_LOCALBUS_DRIVER_VERSION);

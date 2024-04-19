// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * PVT device driver.
 *
 * Part of lm_sensors, Linux kernel modules
 * for hardware monitoring in sunway.
 */
#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/io.h>
#include <linux/module.h>

#define PVT_VSYS                      0
#define PVT0_CTRL                     0x7c00
#define PVT02SPBU_DATA_OUT            (0x1 << 26)
#define PVT_READ                      0xc000
#define PVT_WADDR                     0xc800
#define PVT_WDATA                     0xcc00

/* The PVT registers */
#define PVT_SAFECTRL                  0x0
#define CLK_SEL                       0x1
#define PVT_RUN                       0x2
#define PVT_CONFIG                    0x3
#define PVT_WAIT_TIME                 0x4
#define TS_ALARM_HVALUE_L             0x5
#define TS_ALARM_HVALUE_H             0x6
#define TS_ALARM_LVALUE_L             0x7
#define TS_ALARM_LVALUE_H             0x8
#define TS_ALARM_TIMES                0x9
#define TRIMG                         0xa
#define TRIMO                         0xb
#define VS_ALARM_HVALUE_L             0xc
#define VS_ALARM_HVALUE_H             0xd
#define VS_ALARM_LVALUE_L             0xe
#define VS_ALARM_LVALUE_H             0xf
#define VS_ALARM_TIMES                0x10
#define PVT_ALARM_CLEAR               0x11
#define PVT_ALARM_MASK                0x12
#define PVT_DATA_OUT_L                0x13
#define PVT_DATA_OUT_H                0x14
#define PVT_STATE_INFO                0x15
#define PVT_ALARM_INFO                0x16
#define COFFICIENT                    71
#define FIXEDVAL                      45598

#define vol_algorithm(m, n) (((((m >> 16) & 0x3) * 0x100) +\
			((n >> 16) & 0xff)) * COFFICIENT + FIXEDVAL)


struct pvt_hwmon {
	struct  pvt             *pvt;
	void __iomem            *base;
};

static const char * const input_names[] = {
	[PVT_VSYS]       = "voltage",
};

static inline void pvt_write_reg(struct pvt_hwmon *pvtvol, u64 a,
				u64 b, unsigned int offset)
{
	writel(a | b, pvtvol->base + offset);
}

static inline u64 pvt_read_reg(struct pvt_hwmon *pvtvol, unsigned int offset)
{
	u64 value;

	value = readl(pvtvol->base + offset);
	return value;
}

void pvt_configure(struct pvt_hwmon *pvtvol, u64 value, u64 reg)
{
	pvt_write_reg(pvtvol, PVT_WDATA, value, PVT0_CTRL);
	pvt_write_reg(pvtvol, PVT_WADDR, reg, PVT0_CTRL);
}

static inline u64 pvt_read_vol(struct pvt_hwmon *pvtvol, u64 data,
			u64 reg, unsigned int offset)
{
	unsigned int value;

	pvt_write_reg(pvtvol, data, reg, offset);
	msleep(100);
	value = pvt_read_reg(pvtvol, offset);
	return value;
}

static int pvt_get_vol(struct pvt_hwmon *pvtvol)
{
	unsigned long long data_h, data_l;

	pvt_configure(pvtvol, 0x1, PVT_SAFECTRL);

	/* configure PVT mode */
	pvt_configure(pvtvol, 0x3, PVT_CONFIG);

	/* PVT monitor enable */
	pvt_configure(pvtvol, 0x1, PVT_RUN);

	/* get the upper 2 bits of the PVT voltage */
	data_h = pvt_read_vol(pvtvol, PVT_READ, PVT_DATA_OUT_H, PVT0_CTRL);
	if ((data_h & PVT02SPBU_DATA_OUT) == 0) {
		pr_err("error: the Voltage_h is error\n");
		return false;
	}

	/* get the lower 8 bits of the PVT voltage */
	data_l = pvt_read_vol(pvtvol, PVT_READ, PVT_DATA_OUT_L, PVT0_CTRL);
	if ((data_l & PVT02SPBU_DATA_OUT) == 0) {
		pr_err("error: the Voltage_l is error\n");
		return false;
	}

	return vol_algorithm(data_h, data_l);
}

static ssize_t pvt_read(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	struct pvt_hwmon *pvtvol = dev_get_drvdata(dev);
	unsigned long long pvt_vol;

	pvt_vol = pvt_get_vol(pvtvol);
	return sprintf(buf, "%lld\n", (pvt_vol / 100));
}

static ssize_t show_label(struct device *dev,
		struct device_attribute *devattr, char *buf)
{
	return sprintf(buf, "%s\n",
			input_names[to_sensor_dev_attr(devattr)->index]);
}

static SENSOR_DEVICE_ATTR(in0_input, 0444, pvt_read, NULL,
		PVT_VSYS);
static SENSOR_DEVICE_ATTR(in0_label, 0444, show_label, NULL,
		PVT_VSYS);

static struct attribute *pvt_attrs[] = {
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in0_label.dev_attr.attr,
	NULL
};

ATTRIBUTE_GROUPS(pvt);

static int pvt_vol_plat_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct pvt_hwmon *pvtvol;
	struct device *hwmon_dev;
	unsigned long long value;
	struct device *dev = &pdev->dev;

	pvtvol = devm_kzalloc(&pdev->dev, sizeof(*pvtvol), GFP_KERNEL);
	if (!pvtvol)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		goto err;

	pvtvol->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(pvtvol->base))
		return PTR_ERR(pvtvol->base);

	platform_set_drvdata(pdev, pvtvol);
	hwmon_dev = devm_hwmon_device_register_with_groups(dev, "pvt",
			pvtvol, pvt_groups);

	if (IS_ERR(hwmon_dev))
		return PTR_ERR(hwmon_dev);

	value = pvt_get_vol(pvtvol);
	if (!value) {
		dev_info(&pdev->dev, "pvt_vol get failed\n");
		return false;
	}

	dev_info(&pdev->dev, "PVT probe succeed\n");

	return 0;

err:
	dev_err(&pdev->dev, "no PVT resource\n");
	return -ENXIO;
}

#ifdef CONFIG_OF
static const struct of_device_id pvt_vol_of_match[] = {
	{ .compatible = "sw64,pvt-vol", },
	{},
};
MODULE_DEVICE_TABLE(of, pvt_vol_of_match);
#endif

#ifdef CONFIG_ACPI
static const struct acpi_device_id pvt_vol_acpi_match[] = {
	{ "SUNW0007", 0 },
	{ },
};
MODULE_DEVICE_TABLE(acpi, pvt_vol_acpi_match);
#endif

static struct platform_driver pvt_vol_driver = {
	.probe = pvt_vol_plat_probe,
	.driver         = {
		.name   = "pvt-sw64",
		.of_match_table = of_match_ptr(pvt_vol_of_match),
		.acpi_match_table = ACPI_PTR(pvt_vol_acpi_match),
	},
};

static int __init pvt_vol_init_driver(void)
{
	return platform_driver_register(&pvt_vol_driver);
}
subsys_initcall(pvt_vol_init_driver);

static void __exit pvt_vol_exit_driver(void)
{
	platform_driver_unregister(&pvt_vol_driver);
}
module_exit(pvt_vol_exit_driver);

MODULE_AUTHOR("Wang Yingying <wangyingying@wxiat.com>");
MODULE_DESCRIPTION("pvt controller");
MODULE_LICENSE("GPL");

// SPDX-License-Identifier: GPL-2.0
/* rtc-sw64-virt.c: Hypervisor based RTC for SW64 systems.
 *
 * Copyright (C) 2021 Lu Feifei <luff@gmail.com>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>

#define RTC_IO_ADDR (0x804910000000ULL)
unsigned long vtime_old, vtime_new;

static int sw64_virt_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long *ioaddr;
	unsigned long vtime_now;
	long vtime_offset;

	ioaddr = ioremap(RTC_IO_ADDR, sizeof(long));
	if (!vtime_new) {
		rtc_time64_to_tm(*ioaddr, tm);
	} else {
		vtime_now = *ioaddr;
		vtime_offset = vtime_new - vtime_old;
		vtime_now += vtime_offset;
		rtc_time64_to_tm(vtime_now, tm);
	}
	return 0;
}

static int sw64_virt_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long *ioaddr;

	ioaddr = ioremap(RTC_IO_ADDR, sizeof(long));
	vtime_old = *ioaddr;

	vtime_new = rtc_tm_to_time64(tm);
	return 0;
}

static const struct rtc_class_ops rtc_sw64_virt_ops = {
	.read_time	= sw64_virt_read_time,
	.set_time	= sw64_virt_set_time,
};

static int __init rtc_sw64_virt_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;

	rtc = devm_rtc_device_register(&pdev->dev, "sw64_virt",
				&rtc_sw64_virt_ops, THIS_MODULE);
	if (IS_ERR(rtc))
		return PTR_ERR(rtc);

	platform_set_drvdata(pdev, rtc);
	return 0;
}

static struct platform_driver rtc_sw64_virt_driver = {
	.driver		= {
		.name	= "rtc_sw64_virt",
	},
};

module_platform_driver_probe(rtc_sw64_virt_driver, rtc_sw64_virt_probe);

MODULE_AUTHOR("Lu Feifei <luff@gmail.com>");
MODULE_DESCRIPTION("Sunway virtual RTC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:rtc_sw64_virt");

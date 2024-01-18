// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Lu Feifei <lufeifei@wxiat.com>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>

#define RTC_IO_ADDR (0x804910000000ULL)

static int sw_guest_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long *ioaddr;

	ioaddr = ioremap(RTC_IO_ADDR, sizeof(long));
	rtc_time64_to_tm(*ioaddr, tm);
	return 0;
}

static const struct rtc_class_ops rtc_sw_guest_ops = {
	.read_time	= sw_guest_read_time,
};

static int __init rtc_sw_guest_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;

	rtc = devm_rtc_device_register(&pdev->dev, "sw_guest",
				&rtc_sw_guest_ops, THIS_MODULE);
	if (IS_ERR(rtc))
		return PTR_ERR(rtc);

	platform_set_drvdata(pdev, rtc);
	return 0;
}

static struct platform_driver rtc_sw_guest_driver = {
	.driver		= {
		.name	= "rtc_sw_guest",
	},
};

module_platform_driver_probe(rtc_sw_guest_driver, rtc_sw_guest_probe);

MODULE_AUTHOR("Lu Feifei <lufeifei@wxiat.com>");
MODULE_DESCRIPTION("SW GUEST RTC driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:rtc_sw_guest");

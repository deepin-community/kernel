// SPDX-License-Identifier: GPL-2.0
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/efi.h>
#include <linux/platform_device.h>

static struct platform_device rtc_sw64_virt_device = {
	.name		= "rtc_sw64_virt",
	.id		= -1,
};

static int __init rtc_sw64_virt_init(void)
{
	if (is_in_host())
		return 0;

	if (platform_device_register(&rtc_sw64_virt_device) < 0)
		pr_err("unable to register rtc device...\n");
		/* not necessarily an error */
	return 0;
}
module_init(rtc_sw64_virt_init);

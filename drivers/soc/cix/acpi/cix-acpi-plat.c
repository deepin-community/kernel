// SPDX-License-Identifier: GPL-2.0+
/*
 *
 * Add CIX SKY1 SoC Version driver
 *
 */
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/acpi.h>

static const struct acpi_device_id device_deny_ids[] = {
	{"PNP0D10", 0},
	{"", 0},
};

static int acpi_device_deny(struct acpi_device *adev,
			     const struct acpi_device_id *not_used)
{
	acpi_set_device_status(adev, 0);

	dev_dbg(&adev->dev, "acpi disable dev[%s]\n", dev_name(&adev->dev));

	return -1; /* return -1 to block platform device enumeration */
}

static struct acpi_scan_handler device_deny_handler = {
	.ids = device_deny_ids,
	.attach = acpi_device_deny,
};

int cix_acpi_plat_arch_init(void)
{
	acpi_scan_add_handler(&device_deny_handler);

	return 0;
}
arch_initcall(cix_acpi_plat_arch_init);

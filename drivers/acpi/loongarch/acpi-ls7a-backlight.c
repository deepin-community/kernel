// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2025 Xi Ruoyao <xry111@aosc.io>
 *
 * ACPI support for devices using LG110 <sarcasm>discrete</sarcasm> graphics
 * with LS7A PWM controlling the backlight.
 */

#include <linux/acpi.h>
#include <linux/pwm.h>

#include "../internal.h"

static struct pwm_lookup ls7a_pwm_lookup[] = {
	PWM_LOOKUP_WITH_MODULE("LOON0006:03", 0, NULL, "gsgpu_backlight", 0,
			       PWM_POLARITY_NORMAL, "pwm-loongson"),
};

static int acpi_ls7a_pwm_attach(struct acpi_device *adev,
				 const struct acpi_device_id *id)
{
	if (acpi_dev_uid_match(adev, 3))
		pwm_add_table(ls7a_pwm_lookup, ARRAY_SIZE(ls7a_pwm_lookup));

	return 0;
}

static const struct acpi_device_id acpi_ls7a_pwm_ids[] = {
	{ "LOON0006", 0 },
	{ },
};

static struct acpi_scan_handler ls7a_pwm_handler = {
	.ids = acpi_ls7a_pwm_ids,
	.attach = acpi_ls7a_pwm_attach,
};


void __init acpi_ls7a_pwm_init(void)
{
	acpi_scan_add_handler(&ls7a_pwm_handler);
}

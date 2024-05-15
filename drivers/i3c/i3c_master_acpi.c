// SPDX-License-Identifier: GPL-2.0
/*
 * Driver for I3C ACPI Interface
 *
 * Copyright (C) 2021-2023 Phytium Technology Co., Ltd.
 */

#include <linux/acpi.h>
#include <acpi/acpi_bus.h>
#include <linux/device.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include "i3c_master_acpi.h"

static int i3c_master_acpi_parse_val(union acpi_object *store_elements, char *method,
						 int count, u32 *value)
{
	int i, package_count, ret = -1;
	union acpi_object *acpi_elements;

	for (i = 0; i < count; i++) {
		acpi_elements = store_elements;
		if (acpi_elements[i].type == ACPI_TYPE_PACKAGE) {
			package_count = acpi_elements[i].package.count;

			if (package_count == 2) {
				acpi_elements = acpi_elements[i].package.elements;
				if (acpi_elements[0].type == ACPI_TYPE_STRING) {
					if (!strcmp(acpi_elements[0].string.pointer, method)) {
						*value = acpi_elements[1].integer.value;
						ret = 0;
						break;
					}
				}
			}
		}
	}
	return ret;
}

int i3c_master_acpi_get_params(acpi_handle handle, char *method, u32 *value)
{
	struct acpi_buffer buf = { ACPI_ALLOCATE_BUFFER };
	acpi_status status = 0;
	union acpi_object *obj;
	int count, ret = -1;

	status = acpi_evaluate_object(handle, "_DSD", NULL, &buf);
	if (ACPI_FAILURE(status)) {
		kfree(buf.pointer);
		return -2;
	}

	obj = (union acpi_object *)buf.pointer;

	if (obj->type == ACPI_TYPE_PACKAGE) {
		union acpi_object *acpi_elements;
		union acpi_object *store_elements;

		acpi_elements = obj->package.elements;
		if ((obj->package.count >= 2) && (acpi_elements[1].type == ACPI_TYPE_PACKAGE)) {
			count = acpi_elements[1].package.count;
			acpi_elements = acpi_elements[1].package.elements;

			store_elements = acpi_elements;
			ret = i3c_master_acpi_parse_val(store_elements, method, count, value);
		}
	}

	kfree(buf.pointer);
	return ret;
}
EXPORT_SYMBOL_GPL(i3c_master_acpi_get_params);

void i3c_master_acpi_clk_params(acpi_handle handle, char *method,
				    u32 *pres_ctrl0, u32 *pres_ctrl1, u32 *thd_delay)
{
	struct acpi_buffer buf = { ACPI_ALLOCATE_BUFFER };
	union acpi_object *obj;

	if (ACPI_FAILURE(acpi_evaluate_object(handle, method, NULL, &buf)))
		return;

	obj = (union acpi_object *)buf.pointer;
	if (obj->type == ACPI_TYPE_PACKAGE && obj->package.count == 3) {
		const union acpi_object *objs = obj->package.elements;

	*pres_ctrl0 = (u32)objs[0].integer.value;
	*pres_ctrl1 = (u32)objs[1].integer.value;
	*thd_delay = (u32)objs[2].integer.value;
	}

	kfree(buf.pointer);
}
EXPORT_SYMBOL_GPL(i3c_master_acpi_clk_params);

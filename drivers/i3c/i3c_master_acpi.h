/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Driver for I3C ACPI Interface
 *
 * Copyright (C) 2021-2023 Phytium Technology Co., Ltd.
 */

#ifndef __I3C_MASTER_ACPI_H
#define __I3C_MASTER_ACPI_H

#include <linux/acpi.h>
#include <acpi/acpi_bus.h>

int i3c_master_acpi_get_params(acpi_handle handle, char *method, u32 *value);
void i3c_master_acpi_clk_params(acpi_handle handle, char *method,
	   u32 *pres_ctrl0, u32 *pres_ctrl1, u32 *thd_delay);

#endif

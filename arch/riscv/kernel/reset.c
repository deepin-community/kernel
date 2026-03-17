// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <linux/reboot.h>
#include <linux/pm.h>
#include <linux/smp.h>

static void default_power_off(void)
{
	while (1)
		wait_for_interrupt();
}

void (*pm_power_off)(void) = NULL;
EXPORT_SYMBOL(pm_power_off);

void machine_restart(char *cmd)
{
	local_irq_disable();
	smp_send_stop();

	do_kernel_restart(cmd);
	while (1);
}

void machine_halt(void)
{
	local_irq_disable();
	smp_send_stop();

	do_kernel_power_off();
	default_power_off();
}

void machine_power_off(void)
{
	local_irq_disable();
	smp_send_stop();

	do_kernel_power_off();
	default_power_off();
}

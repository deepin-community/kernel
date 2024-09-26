// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020-2024 Sunway Technology Corporation Limited
 */

#include <linux/acpi.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/efi.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/pm.h>
#include <linux/reboot.h>
#include <linux/types.h>

#include <acpi/reboot.h>
#include <asm/idle.h>
#include <asm/efi.h>

void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

void machine_halt(void)
{
	preempt_disable();
	local_irq_disable();
	smp_send_stop();

	pr_notice("\n\n** You can safely turn off the power now **\n\n");

	while (true)
		arch_cpu_idle();
}

void machine_power_off(void)
{
	preempt_disable();
	local_irq_disable();
	smp_send_stop();

	do_kernel_power_off();

	/* VM cannot reach here */
	WARN_ON(!is_in_host());

	/**
	 * Compatibility with old firmware, can be removed
	 * when no longer support SW3231.
	 */
	if (!sunway_bios_version)
		cpld_write(0x64, 0x00, 0xf0);

	while (true)
		arch_cpu_idle();
}

void machine_restart(char *command)
{
	preempt_disable();
	local_irq_disable();
	smp_send_stop();

	do_kernel_restart(command);

	/* VM cannot reach here */
	WARN_ON(!is_in_host());

	acpi_reboot();

	/**
	 * Compatibility with old firmware, can be removed
	 * when no longer support SW3231.
	 */
	if (!sunway_bios_version)
		cpld_write(0x64, 0x00, 0xc3);
	else if (efi_enabled(EFI_RUNTIME_SERVICES))
		efi_reboot(reboot_mode, NULL);

	while (true)
		arch_cpu_idle();
}

static int vm_restart(struct sys_off_data *data)
{
	hcall(HCALL_SET_CLOCKEVENT, 0, 0, 0);
	hcall(HCALL_RESTART, 0, 0, 0);
	mb();

	return NOTIFY_DONE;
}

static int vm_power_off(struct sys_off_data *data)
{
	hcall(HCALL_SET_CLOCKEVENT, 0, 0, 0);
	hcall(HCALL_SHUTDOWN, 0, 0, 0);
	mb();

	return NOTIFY_DONE;
}

static int __init vm_power_init(void)
{
	struct sys_off_handler *handler;

	if (is_in_host())
		return 0;

	handler = register_sys_off_handler(SYS_OFF_MODE_RESTART,
			SYS_OFF_PRIO_DEFAULT, vm_restart, NULL);
	if (WARN_ON(IS_ERR(handler)))
		return PTR_ERR(handler);

	handler = register_sys_off_handler(SYS_OFF_MODE_POWER_OFF,
			SYS_OFF_PRIO_DEFAULT, vm_power_off, NULL);
	if (WARN_ON(IS_ERR(handler)))
		return PTR_ERR(handler);

	return 0;
}
arch_initcall(vm_power_init);

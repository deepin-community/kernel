// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020-2022 Sunway Technology Corporation Limited
 */
#include <linux/acpi.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/efi.h>
#include <linux/export.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/pm.h>
#include <linux/reboot.h>
#include <linux/types.h>

#include <acpi/reboot.h>
#include <asm/idle.h>
#include <asm/sw64io.h>

void fix_jm585_reset(void)
{
	struct pci_dev *pdev;
	struct pci_controller *hose;
	int val;

	pdev = pci_get_device(PCI_VENDOR_ID_JMICRON,
				0x0585, NULL);
	if (pdev) {
		hose = pci_bus_to_pci_controller(pdev->bus);
		val = read_rc_conf(hose->node, hose->index,
				RC_PORT_LINK_CTL);
		write_rc_conf(hose->node, hose->index,
				RC_PORT_LINK_CTL, val | 0x8);
		write_rc_conf(hose->node, hose->index,
		RC_PORT_LINK_CTL, val);
	}

}
static void default_halt(void)
{
	local_irq_disable();

	pr_notice("\n\n** You can safely turn off the power now **\n\n");

	while (true)
		arch_cpu_idle();
}

static void default_poweroff(void)
{
	/* No point in taking interrupts anymore. */
	local_irq_disable();
#ifdef CONFIG_EFI
	efi.reset_system(EFI_RESET_SHUTDOWN, EFI_SUCCESS, 0, NULL);
#endif
	while (true)
		arch_cpu_idle();
}

static void default_restart(void)
{
	/* No point in taking interrupts anymore. */
	local_irq_disable();

	fix_jm585_reset();
#ifdef CONFIG_EFI
	if (efi_capsule_pending(NULL))
		efi_reboot(REBOOT_WARM, NULL);
	else
		efi_reboot(REBOOT_COLD, NULL);
#endif

	while (true)
		arch_cpu_idle();
}

void (*pm_restart)(void);

void (*pm_power_off)(void);
EXPORT_SYMBOL(pm_power_off);

void (*pm_halt)(void);

void machine_halt(void)
{
#ifdef CONFIG_SMP
	preempt_disable();
	smp_send_stop();
#endif
	pm_halt();
}

void machine_power_off(void)
{
#ifdef CONFIG_SMP
	preempt_disable();
	smp_send_stop();
#endif
	pm_power_off();
}

void machine_restart(char *command)
{
#ifdef CONFIG_SMP
	preempt_disable();
	smp_send_stop();
#endif
	do_kernel_restart(command);
	pm_restart();
}

static int __init sw64_reboot_setup(void)
{
	pm_restart = default_restart;
	pm_power_off = default_poweroff;
	pm_halt = default_halt;

	return 0;
}
arch_initcall(sw64_reboot_setup);

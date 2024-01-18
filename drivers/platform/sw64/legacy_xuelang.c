// SPDX-License-Identifier: GPL-2.0
#include <asm/platform.h>
#include <asm/sw64_init.h>

#include <linux/pm.h>
#include <linux/reboot.h>

static void vt_mode_kill_arch(int mode)
{
	hcall(HCALL_SET_CLOCKEVENT, 0, 0, 0);

	switch (mode) {
	case LINUX_REBOOT_CMD_RESTART:
		hcall(HCALL_RESTART, 0, 0, 0);
		mb();
		break;
	case LINUX_REBOOT_CMD_HALT:
	case LINUX_REBOOT_CMD_POWER_OFF:
		hcall(HCALL_SHUTDOWN, 0, 0, 0);
		mb();
		break;
	default:
		break;
	}
}

void sw64_halt(void)
{
	if (is_in_host())
		cpld_write(0x64, 0x00, 0xf0);
	else
		vt_mode_kill_arch(LINUX_REBOOT_CMD_HALT);
}

void sw64_poweroff(void)
{
	if (is_in_host())
		cpld_write(0x64, 0x00, 0xf0);
	else
		vt_mode_kill_arch(LINUX_REBOOT_CMD_POWER_OFF);
}

void sw64_restart(void)
{
	if (is_in_host()) {
		fix_jm585_reset();
		cpld_write(0x64, 0x00, 0xc3);
	} else
		vt_mode_kill_arch(LINUX_REBOOT_CMD_RESTART);
}

static int sw64_reset_init(void)
{
#ifdef CONFIG_EFI
	if (BIOS_SUPPORT_RESET_CLALLBACK((void *)bios_version))
		return 0;
#endif
	pm_restart = sw64_restart;
	pm_power_off = sw64_poweroff;
	pm_halt = sw64_halt;
	return 0;
}
subsys_initcall(sw64_reset_init);

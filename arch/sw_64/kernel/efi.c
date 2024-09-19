// SPDX-License-Identifier: GPL-2.0

#include <linux/acpi.h>
#include <linux/efi.h>

#include <asm/hw_init.h>

bool efi_poweroff_required(void)
{
	/* VM has its own poweroff interface */
	if (!is_in_host())
		return false;

	/* Prefer ACPI S5 */
	if (acpi_sleep_state_supported(ACPI_STATE_S5))
		return false;

	return efi_enabled(EFI_RUNTIME_SERVICES);
}

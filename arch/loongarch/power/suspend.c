// SPDX-License-Identifier: GPL-2.0
/*
 * loongson-specific suspend support
 *
 * Author: Huacai Chen <chenhuacai@loongson.cn>
 * Copyright (C) 2020-2022 Loongson Technology Corporation Limited
 */
#include <linux/acpi.h>
#include <linux/pm.h>
#include <linux/suspend.h>

#include <asm/loongarch.h>
#include <asm/loongson.h>
#include <asm/setup.h>
#include <asm/time.h>
#include <asm/tlbflush.h>

#include "../drivers/acpi/acpica/aclocal.h"

u64 loongarch_suspend_addr;
static u32 gpe_en_init;
struct acpi_gpe_event_info *gpe_info;

struct saved_registers {
	u32 ecfg;
	u32 euen;
	u64 pgd;
	u64 kpgd;
	u32 pwctl0;
	u32 pwctl1;
};
static struct saved_registers saved_regs;

void loongarch_common_suspend(void)
{
	save_counter();
	saved_regs.pgd = csr_read64(LOONGARCH_CSR_PGDL);
	saved_regs.kpgd = csr_read64(LOONGARCH_CSR_PGDH);
	saved_regs.pwctl0 = csr_read32(LOONGARCH_CSR_PWCTL0);
	saved_regs.pwctl1 = csr_read32(LOONGARCH_CSR_PWCTL1);
	saved_regs.ecfg = csr_read32(LOONGARCH_CSR_ECFG);
	saved_regs.euen = csr_read32(LOONGARCH_CSR_EUEN);

	loongarch_suspend_addr = loongson_sysconf.suspend_addr;
}

void loongarch_common_resume(void)
{
	sync_counter();
	local_flush_tlb_all();
	csr_write64(per_cpu_offset(0), PERCPU_BASE_KS);
	csr_write64(eentry, LOONGARCH_CSR_EENTRY);
	csr_write64(eentry, LOONGARCH_CSR_MERRENTRY);
	csr_write64(tlbrentry, LOONGARCH_CSR_TLBRENTRY);

	csr_write64(saved_regs.pgd, LOONGARCH_CSR_PGDL);
	csr_write64(saved_regs.kpgd, LOONGARCH_CSR_PGDH);
	csr_write32(saved_regs.pwctl0, LOONGARCH_CSR_PWCTL0);
	csr_write32(saved_regs.pwctl1, LOONGARCH_CSR_PWCTL1);
	csr_write32(saved_regs.ecfg, LOONGARCH_CSR_ECFG);
	csr_write32(saved_regs.euen, LOONGARCH_CSR_EUEN);
}

int loongarch_acpi_suspend(void)
{
	enable_gpe_wakeup();
	enable_pci_wakeup();

	loongarch_common_suspend();

	/* processor specific suspend */
	loongarch_suspend_enter();

	loongarch_common_resume();

	return 0;
}

extern struct acpi_gpe_event_info *acpi_ev_get_gpe_event_info(acpi_handle gpe_device,
							      u32 gpe_number);

static int plat_pm_callback(struct notifier_block *nb, unsigned long action, void *ptr)
{
	int ret = 0;
	u32 data, gpe_en;
	acpi_event_status gpe_status = 0;

	switch (action) {
	case PM_POST_SUSPEND:
		if (!gpe_info)
			break;

		enable_gpe_wakeup();
		data = readl((void *)gpe_info->register_info->enable_address.address);
		gpe_en = gpe_en_init;
		while (gpe_en) {
			int bit = __ffs(gpe_en);

			gpe_en &= ~BIT(bit);

			if (acpi_get_gpe_status(NULL, bit, &gpe_status) != AE_OK)
				continue;

			if (gpe_status & ACPI_EVENT_FLAG_ENABLED)
				data |= BIT(bit);
		}

		writel(data, (void *)gpe_info->register_info->enable_address.address);

		break;
	default:
		break;
	}

	return notifier_from_errno(ret);
}

static int __init plat_pm_post_init(void)
{

	if (acpi_disabled || acpi_gbl_reduced_hardware)
		return 0;

	gpe_info = acpi_ev_get_gpe_event_info(NULL, 0);
	if (!gpe_info)
		return 0;
	gpe_en_init = readl((void *)gpe_info->register_info->enable_address.address);

	enable_gpe_wakeup();
	pm_notifier(plat_pm_callback, -INT_MAX);

	return 0;
}
late_initcall_sync(plat_pm_post_init);

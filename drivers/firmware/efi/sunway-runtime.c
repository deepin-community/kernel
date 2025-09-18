// SPDX-License-Identifier: GPL-2.0
/*
 * Extensible Firmware Interface
 *
 * Based on Extensible Firmware Interface Specification version 2.4
 *
 * Copyright (C) 2013, 2014 Linaro Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/dmi.h>
#include <linux/efi.h>
#include <linux/io.h>
#include <linux/memblock.h>
#include <linux/mm_types.h>
#include <linux/preempt.h>
#include <linux/rbtree.h>
#include <linux/rwsem.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/cacheflush.h>
#include <linux/pgtable.h>

#include <asm/efi.h>
#include <asm/mmu.h>
#include <asm/pgalloc.h>
#include <asm/mmu_context.h>

#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE

DEFINE_PER_CPU(unsigned long, atc_state);

static bool __init efi_virtmap_init(void)
{
	efi_memory_desc_t *md;

	efi_mm.pgd = pgd_alloc(&efi_mm);
	memcpy(efi_mm.pgd + USER_PTRS_PER_PGD,
	       init_mm.pgd + USER_PTRS_PER_PGD,
	       (PTRS_PER_PGD - USER_PTRS_PER_PGD) * sizeof(pgd_t));
	mm_init_cpumask(&efi_mm);
	init_new_context(NULL, &efi_mm);

	for_each_efi_memory_desc(md) {
		phys_addr_t phys = md->phys_addr;
		int ret;

		if (!(md->attribute & EFI_MEMORY_RUNTIME))
			continue;

		ret = efi_create_mapping(&efi_mm, md);
		if (ret) {
			pr_warn("EFI remap %pa: failed to create mapping (%d)\n",
				&phys, ret);
			return false;
		}
	}

	return true;
}

void efi_virtmap_load(void)
{
	preempt_disable();
	update_ptbr_sys(virt_to_phys(efi_mm.pgd));
	/* switch CSR_ATC for bios compatibility */
	this_cpu_write(atc_state, get_atc());
	set_atc(ATC_KSEG);
}

void efi_virtmap_unload(void)
{
	set_atc(this_cpu_read(atc_state));
	update_ptbr_sys(virt_to_phys(init_mm.pgd));
	preempt_enable();
}
#endif /* CONFIG_SW64_KERNEL_PAGE_TABLE */

/*
 * Enable the UEFI Runtime Services if all prerequisites are in place, i.e.,
 * non-early mapping of the UEFI system table and virtual mappings for all
 * EFI_MEMORY_RUNTIME regions.
 */
static int __init sunway_enable_runtime_services(void)
{
	u64 mapsize;

	if (!efi_enabled(EFI_BOOT)) {
		pr_info("EFI services will not be available.\n");
		return 0;
	}

	efi_memmap_unmap();

	mapsize = efi.memmap.desc_size * efi.memmap.nr_map;

	if (efi_memmap_init_late(efi.memmap.phys_map, mapsize)) {
		pr_err("Failed to remap EFI memory map\n");
		return 0;
	}

	if (efi_runtime_disabled()) {
		pr_info("EFI runtime services will be disabled.\n");
		return 0;
	}

	if (efi_enabled(EFI_RUNTIME_SERVICES)) {
		pr_info("EFI runtime services access via paravirt.\n");
		return 0;
	}

#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE
	if (!efi_virtmap_init()) {
		pr_err("UEFI virtual mapping missing or invalid -- runtime services will not be available\n");
		return -ENOMEM;
	}
#endif

	/* Set up runtime services function pointers */
	efi_native_runtime_setup();
	set_bit(EFI_RUNTIME_SERVICES, &efi.flags);

	return 0;
}
early_initcall(sunway_enable_runtime_services);

static int __init sunway_dmi_init(void)
{
	/*
	 * On SW64, DMI depends on UEFI, and dmi_scan_machine() needs to
	 * be called early because dmi_id_init(), which is an arch_initcall
	 * itself, depends on dmi_scan_machine() having been called already.
	 */
	dmi_setup();
	return 0;
}
core_initcall(sunway_dmi_init);

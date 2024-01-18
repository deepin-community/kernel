// SPDX-License-Identifier: GPL-2.0
/*
 * Extensible Firmware Interface
 *
 * Based on Extensible Firmware Interface Specification version 2.4
 *
 * Copyright (C) 2013 - 2015 Linaro Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define pr_fmt(fmt)	"efi: " fmt

#include <linux/efi.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/mm_types.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/platform_device.h>
#include <linux/screen_info.h>

#include <asm/efi.h>
#include <asm/platform.h>

unsigned long entSuspend;
unsigned long bios_version;

static int __init is_memory(efi_memory_desc_t *md)
{
	if (md->attribute & (EFI_MEMORY_WB|EFI_MEMORY_WT|EFI_MEMORY_WC))
		return 1;
	return 0;
}
static efi_config_table_type_t arch_tables[] __initdata = {
	{SMBIOS3_TABLE_GUID, NULL, ""},
	{SLEEP_ENTRY_GUID, &entSuspend, "SLEEP ENTRY"},
	{BIOS_VERSION_GUID,  &bios_version, "BIOS VERSION"},
	{},
};

static int __init uefi_init(u64 efi_system_table)
{
	efi_char16_t *c16;
	efi_config_table_t *config_tables;
	efi_system_table_t *systab;
	size_t table_size;
	char vendor[100] = "unknown";
	int i, retval;

	systab = early_memremap(efi_system_table,
			sizeof(efi_system_table_t));
	if (systab == NULL) {
		pr_warn("Unable to map EFI system table.\n");
		return -ENOMEM;
	}

	set_bit(EFI_BOOT, &efi.flags);
	if (IS_ENABLED(CONFIG_64BIT))
		set_bit(EFI_64BIT, &efi.flags);

	/*
	 * Verify the EFI Table
	 */
	if (systab->hdr.signature != EFI_SYSTEM_TABLE_SIGNATURE) {
		pr_err("System table signature incorrect\n");
		retval = -EINVAL;
		goto out;
	}
	if ((systab->hdr.revision >> 16) < 2)
		pr_warn("Warning: EFI system table version %d.%02d, expected 2.00 or greater\n",
				systab->hdr.revision >> 16,
				systab->hdr.revision & 0xffff);

	efi.runtime = systab->runtime;
	efi.runtime_version = systab->hdr.revision;

	/* Show what we know for posterity */
	c16 = early_memremap(systab->fw_vendor,
			sizeof(vendor) * sizeof(efi_char16_t));
	if (c16) {
		for (i = 0; i < (int) sizeof(vendor) - 1 && *c16; ++i)
			vendor[i] = c16[i];
		vendor[i] = '\0';
		early_memunmap(c16, sizeof(vendor) * sizeof(efi_char16_t));
	}

	pr_info("EFI v%u.%.02u by %s\n",
			systab->hdr.revision >> 16,
			systab->hdr.revision & 0xffff, vendor);

	table_size = sizeof(efi_config_table_64_t) * systab->nr_tables;
	config_tables = early_memremap(systab->tables, table_size);
	if (config_tables == NULL) {
		pr_warn("Unable to map EFI config table array.\n");
		retval = -ENOMEM;
		goto out;
	}

	retval = efi_config_parse_tables(config_tables, systab->nr_tables,
			arch_tables);

	early_memunmap(config_tables, table_size);
out:
	early_memunmap(systab,  sizeof(efi_system_table_t));

	if (!bios_version)
		retval = -EINVAL;

	return retval;
}

/*
 * Return true for regions that can be used as System RAM.
 */
static __init int is_usable_memory(efi_memory_desc_t *md)
{
	switch (md->type) {
	case EFI_LOADER_CODE:
	case EFI_LOADER_DATA:
	case EFI_ACPI_RECLAIM_MEMORY:
	case EFI_BOOT_SERVICES_CODE:
	case EFI_BOOT_SERVICES_DATA:
	case EFI_CONVENTIONAL_MEMORY:
	case EFI_PERSISTENT_MEMORY:
		/*
		 * According to the spec, these regions are no longer reserved
		 * after calling ExitBootServices(). However, we can only use
		 * them as System RAM if they can be mapped writeback cacheable.
		 */
		return (md->attribute & EFI_MEMORY_WB);
	default:
		break;
	}
	return false;
}

static __init void reserve_regions(void)
{
	efi_memory_desc_t *md;
	u64 paddr, npages, size;

	if (efi_enabled(EFI_DBG))
		pr_info("Processing EFI memory map:\n");

	for_each_efi_memory_desc(md) {
		paddr = md->phys_addr;
		npages = md->num_pages;

		if (efi_enabled(EFI_DBG)) {
			char buf[64];

			pr_info("  0x%012llx-0x%012llx %s\n",
				paddr, paddr + (npages << EFI_PAGE_SHIFT) - 1,
				efi_md_typeattr_format(buf, sizeof(buf), md));
		}

		memrange_efi_to_native(&paddr, &npages);
		size = npages << PAGE_SHIFT;

		if (is_memory(md)) {
			early_init_dt_add_memory_arch(paddr, size);

			if (!is_usable_memory(md))
				memblock_mark_nomap(paddr, size);

			/* keep ACPI reclaim memory intact for kexec etc. */
			if (md->type == EFI_ACPI_RECLAIM_MEMORY)
				memblock_reserve(paddr, size);
		}
	}
}

void __init efi_init(void)
{
	struct efi_memory_map_data data;
	u64 efi_system_table;

	if (sunway_boot_params->efi_systab == 0) {
		pr_info("System Table is not exist, disabling EFI.\n");
		return;
	}

	/* Grab UEFI information placed in struct boot_params by stub */
	efi_system_table = sunway_boot_params->efi_systab;
	if (!efi_system_table)
		return;

	data.desc_version = sunway_boot_params->efi_memdesc_version;
	data.desc_size = sunway_boot_params->efi_memdesc_size;
	data.size = sunway_boot_params->efi_memmap_size;
	data.phys_map = sunway_boot_params->efi_memmap;

	if (efi_memmap_init_early(&data) < 0) {
		/*
		 * If we are booting via UEFI, the UEFI memory map is the only
		 * description of memory we have, so there is little point in
		 * proceeding if we cannot access it.
		 */
		panic("Unable to map EFI memory map.\n");
	}

	WARN(efi.memmap.desc_version != 1,
	     "Unexpected EFI_MEMORY_DESCRIPTOR version %ld",
	      efi.memmap.desc_version);

	if (uefi_init(efi_system_table) < 0) {
		efi_memmap_unmap();
		return;
	}

	reserve_regions();

	memblock_reserve(sunway_boot_params->efi_memmap & PAGE_MASK,
			 PAGE_ALIGN(sunway_boot_params->efi_memmap_size +
				    (sunway_boot_params->efi_memmap & ~PAGE_MASK)));

}

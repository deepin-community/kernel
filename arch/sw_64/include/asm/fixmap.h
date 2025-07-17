/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_FIXMAP_H
#define _ASM_SW64_FIXMAP_H

#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE

#include <linux/kernel.h>
#include <linux/sizes.h>
#include <linux/pgtable.h>
#include <asm/acpi.h>
#include <asm/page.h>

enum fixed_addresses {
	FIX_HOLE,
	FIX_PTE,
	FIX_PMD,
	FIX_PUD,
	FIX_TEXT_POKE0,
	FIX_EARLYCON_MEM_BASE,

	__end_of_permanent_fixed_addresses,
#define FIX_BTMAPS_SLOTS	7
#define TOTAL_FIX_BTMAPS	(NR_FIX_BTMAPS * FIX_BTMAPS_SLOTS)

	FIX_BTMAP_END = __end_of_permanent_fixed_addresses,
	FIX_BTMAP_BEGIN = FIX_BTMAP_END + TOTAL_FIX_BTMAPS - 1,

	__end_of_fixed_addresses
};

#define FIXMAP_PAGE_IO			PAGE_KERNEL

#define __early_set_fixmap		__set_fixmap

#define __late_set_fixmap		__set_fixmap
#define __late_clear_fixmap(idx)	__set_fixmap((idx), 0, FIXMAP_PAGE_CLEAR)
extern void __set_fixmap(enum fixed_addresses idx, phys_addr_t phys, pgprot_t prot);

#include <asm-generic/fixmap.h>

#endif /* CONFIG_SW64_KERNEL_PAGE_TABLE */
#endif /* _ASM_SW64_FIXMAP_H */

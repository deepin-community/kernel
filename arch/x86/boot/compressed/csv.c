// SPDX-License-Identifier: GPL-2.0-only
/*
 * Hygon CSV Support
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 */

#include "misc.h"

#undef __init
#undef __initdata
#undef __pa
#define __init
#define __initdata
#define __pa(x)	((unsigned long)(x))

#include <asm/csv.h>
#include <asm/cpuid.h>

/* Include code for early secure calls */
#include "../../kernel/csv-shared.c"

static unsigned int csv3_enabled __section(".data");
static unsigned int csv3_secure_call_init;

void csv_update_page_attr(unsigned long address, pteval_t set, pteval_t clr)
{
	if (!csv3_enabled)
		return;

	if ((set | clr) & _PAGE_ENC) {
		if (set & _PAGE_ENC)
			csv3_early_secure_call_ident_map(__pa(address), 1,
							 CSV3_SECURE_CMD_ENC);

		if (clr & _PAGE_ENC)
			csv3_early_secure_call_ident_map(__pa(address), 1,
							 CSV3_SECURE_CMD_DEC);
	}
}

/* Invoke it before jump to real kernel in case secure call pages are not mapped
 * in the identity page table.
 *
 * If no #VC happens, there is no identity mapping in page table for secure call
 * pages. And page fault is not supported in the early stage when real kernel is
 * running. As a result, CSV3 guest will shutdown when access secure call pages
 * by then.
 */
void csv_init_secure_call_pages(void *boot_params)
{
	if (!csv3_enabled || csv3_secure_call_init)
		return;

	/*
	 * boot_params may be not sanitized, but it's OK to access e820_table
	 * field.
	 */
	csv3_scan_secure_call_pages(boot_params);
	csv3_early_secure_call_ident_map(0, 0, CSV3_SECURE_CMD_RESET);
	csv3_secure_call_init = 1;
}

void csv_set_status(void)
{
	unsigned int eax;
	unsigned int ebx;
	unsigned int ecx;
	unsigned int edx;

	eax = 0;
	native_cpuid(&eax, &ebx, &ecx, &edx);

	/* HygonGenuine */
	if (ebx == CPUID_VENDOR_HygonGenuine_ebx &&
	    ecx == CPUID_VENDOR_HygonGenuine_ecx &&
	    edx == CPUID_VENDOR_HygonGenuine_edx &&
	    sme_me_mask) {
		unsigned long low, high;

		asm volatile("rdmsr\n" : "=a" (low), "=d" (high) :
			"c" (MSR_AMD64_SEV));

		if (low & MSR_CSV3_ENABLED)
			csv3_enabled = 1;
	}
}

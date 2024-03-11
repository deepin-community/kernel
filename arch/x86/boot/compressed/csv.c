// SPDX-License-Identifier: GPL-2.0-only
/*
 * Hygon CSV Support
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 */

#include "misc.h"

#include <asm/csv.h>
#include <asm/cpuid.h>

static unsigned int csv3_enabled __section(".data");

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

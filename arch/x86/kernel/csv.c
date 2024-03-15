// SPDX-License-Identifier: GPL-2.0-only
/*
 * HYGON CSV support
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 */

#include <linux/preempt.h>
#include <linux/smp.h>
#include <linux/memblock.h>
#include <asm/mem_encrypt.h>
#include <asm/csv.h>

#include "../mm/mm_internal.h"
#include "csv-shared.c"

struct secure_call_pages {
	struct csv3_secure_call_cmd page_a;
	struct csv3_secure_call_cmd page_b;
};

void __init csv_early_reset_memory(struct boot_params *bp)
{
	if (!csv3_active())
		return;

	csv3_scan_secure_call_pages(bp);
	csv3_early_secure_call_ident_map(0, 0, CSV3_SECURE_CMD_RESET);
}

void __init csv_early_update_memory_dec(u64 vaddr, u64 pages)
{
	if (!csv3_active())
		return;

	if (pages)
		csv3_early_secure_call_ident_map(__pa(vaddr), pages,
						 CSV3_SECURE_CMD_DEC);
}

void __init csv_early_update_memory_enc(u64 vaddr, u64 pages)
{
	if (!csv3_active())
		return;

	if (pages)
		csv3_early_secure_call_ident_map(__pa(vaddr), pages,
						 CSV3_SECURE_CMD_ENC);
}

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

static u32 csv3_percpu_secure_call_init __initdata;
static u32 early_secure_call_page_idx __initdata;

static DEFINE_PER_CPU(struct secure_call_pages*, secure_call_data);
static DEFINE_PER_CPU(int, secure_call_page_idx);

typedef void (*csv3_secure_call_func)(u64 base_address, u64 num_pages,
				      enum csv3_secure_command_type cmd_type);

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

static void __init csv3_alloc_secure_call_data(int cpu)
{
	struct secure_call_pages *data;

	data = memblock_alloc(sizeof(*data), PAGE_SIZE);
	if (!data)
		panic("Can't allocate CSV3 secure all data");

	per_cpu(secure_call_data, cpu) = data;
}

static void __init csv3_secure_call_update_table(void)
{
	int cpu;
	struct secure_call_pages *data;
	struct csv3_secure_call_cmd *page_rd;
	struct csv3_secure_call_cmd *page_wr;
	u32 cmd_ack;

	if (!csv3_active())
		return;

	page_rd = (void *)early_memremap_encrypted(csv3_boot_sc_page_a, PAGE_SIZE);
	page_wr = (void *)early_memremap_encrypted(csv3_boot_sc_page_b, PAGE_SIZE);

	while (1) {
		page_wr->cmd_type = CSV3_SECURE_CMD_UPDATE_SECURE_CALL_TABLE;
		page_wr->nums = 0;

		/* initialize per-cpu secure call pages */
		for_each_possible_cpu(cpu) {
			if (cpu >= SECURE_CALL_ENTRY_MAX)
				panic("csv does not support cpus > %d\n",
				      SECURE_CALL_ENTRY_MAX);
			csv3_alloc_secure_call_data(cpu);
			data = per_cpu(secure_call_data, cpu);
			per_cpu(secure_call_page_idx, cpu) = 0;
			page_wr->entry[cpu].base_address = __pa(data);
			page_wr->entry[cpu].size = PAGE_SIZE * 2;
			page_wr->nums++;
		}

		/*
		 * Write command in page_wr must be done before retrieve cmd
		 * ack from page_rd, and it is ensured by the mb below.
		 */
		mb();

		cmd_ack = page_rd->cmd_type;
		if (cmd_ack != CSV3_SECURE_CMD_UPDATE_SECURE_CALL_TABLE)
			break;
	}

	early_memunmap(page_rd, PAGE_SIZE);
	early_memunmap(page_wr, PAGE_SIZE);
}

/**
 * __csv3_early_secure_call - issue secure call command at the stage where new
 *			kernel page table is created and early identity page
 *			table is deprecated .
 * @base_address:	Start address of the specified memory range.
 * @num_pages:		number of the specific pages.
 * @cmd_type:		Secure call cmd type.
 */
static void __init __csv3_early_secure_call(u64 base_address, u64 num_pages,
					    enum csv3_secure_command_type cmd_type)
{
	struct csv3_secure_call_cmd *page_rd;
	struct csv3_secure_call_cmd *page_wr;
	u32 cmd_ack;

	if (csv3_boot_sc_page_a == -1ul || csv3_boot_sc_page_b == -1ul)
		return;

	if (!csv3_percpu_secure_call_init) {
		csv3_secure_call_update_table();
		csv3_percpu_secure_call_init = 1;
	}

	if (early_secure_call_page_idx == 0) {
		page_rd = (void *)early_memremap_encrypted(csv3_boot_sc_page_a,
							   PAGE_SIZE);
		page_wr = (void *)early_memremap_encrypted(csv3_boot_sc_page_b,
							   PAGE_SIZE);
	} else {
		page_wr = (void *)early_memremap_encrypted(csv3_boot_sc_page_a,
							   PAGE_SIZE);
		page_rd = (void *)early_memremap_encrypted(csv3_boot_sc_page_b,
							   PAGE_SIZE);
	}

	while (1) {
		page_wr->cmd_type = (u32)cmd_type;
		page_wr->nums = 1;
		page_wr->entry[0].base_address = base_address;
		page_wr->entry[0].size = num_pages << PAGE_SHIFT;

		/*
		 * Write command in page_wr must be done before retrieve cmd
		 * ack from page_rd, and it is ensured by the mb below.
		 */
		mb();

		cmd_ack = page_rd->cmd_type;
		if (cmd_ack != cmd_type)
			break;
	}

	early_memunmap(page_rd, PAGE_SIZE);
	early_memunmap(page_wr, PAGE_SIZE);

	early_secure_call_page_idx ^= 1;
}


static void __csv3_memory_enc_dec(csv3_secure_call_func secure_call, u64 vaddr,
				  u64 pages, bool enc)
{
	u64 vaddr_end, vaddr_next;
	u64 psize, pmask;
	u64 last_paddr, paddr;
	u64 last_psize = 0;
	pte_t *kpte;
	int level;
	enum csv3_secure_command_type cmd_type;

	cmd_type = enc ? CSV3_SECURE_CMD_ENC : CSV3_SECURE_CMD_DEC;
	vaddr_next = vaddr;
	vaddr_end = vaddr + (pages << PAGE_SHIFT);
	for (; vaddr < vaddr_end; vaddr = vaddr_next) {
		kpte = lookup_address(vaddr, &level);
		if (!kpte || pte_none(*kpte)) {
			panic("invalid pte, vaddr 0x%llx\n", vaddr);
			goto out;
		}

		psize = page_level_size(level);
		pmask = page_level_mask(level);

		vaddr_next = (vaddr & pmask) + psize;
		paddr = ((pte_pfn(*kpte) << PAGE_SHIFT) & pmask) +
			(vaddr & ~pmask);
		psize -= (vaddr & ~pmask);

		if (vaddr_end - vaddr < psize)
			psize = vaddr_end - vaddr;
		if (last_psize == 0 || (last_paddr + last_psize) == paddr) {
			last_paddr = (last_psize == 0 ? paddr : last_paddr);
			last_psize += psize;
		} else {
			secure_call(last_paddr, last_psize >> PAGE_SHIFT,
				    cmd_type);
			last_paddr = paddr;
			last_psize = psize;
		}
	}

	if (last_psize)
		secure_call(last_paddr, last_psize >> PAGE_SHIFT, cmd_type);

out:
	return;
}

void __init csv_early_memory_enc_dec(u64 vaddr, u64 size, bool enc)
{
	u64 npages;

	if (!csv3_active())
		return;

	npages = (size + (vaddr & ~PAGE_MASK) + PAGE_SIZE - 1) >> PAGE_SHIFT;
	__csv3_memory_enc_dec(__csv3_early_secure_call, vaddr & PAGE_MASK,
			      npages, enc);
}

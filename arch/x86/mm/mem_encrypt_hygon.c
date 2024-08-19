// SPDX-License-Identifier: GPL-2.0-only
/*
 * HYGON Memory Encryption Support
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DISABLE_BRANCH_PROFILING

#include <linux/cc_platform.h>
#include <linux/mem_encrypt.h>
#include <linux/printk.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/cma.h>
#include <linux/minmax.h>
#include <asm/io.h>
#include <asm/cacheflush.h>
#include <asm/set_memory.h>
#include <asm/csv.h>
#include <asm/processor-hygon.h>

void print_hygon_cc_feature_info(void)
{
	/* Secure Memory Encryption */
	if (cc_platform_has(CC_ATTR_HOST_MEM_ENCRYPT)) {
		/*
		 * HYGON SME is mutually exclusive with any of the
		 * HYGON CSV features below.
		 */
		pr_info(" HYGON SME");
		return;
	}

	/* Secure Encrypted Virtualization */
	if (cc_platform_has(CC_ATTR_GUEST_MEM_ENCRYPT))
		pr_info(" HYGON CSV");

	/* Encrypted Register State */
	if (cc_platform_has(CC_ATTR_GUEST_STATE_ENCRYPT))
		pr_info(" HYGON CSV2");
}

/*
 * Check whether host supports CSV3 in hygon platform.
 * Called in the guest, it always returns false.
 */
static bool __init __maybe_unused csv3_check_cpu_support(void)
{
	unsigned int eax, ebx, ecx, edx;
	unsigned long me_mask;
	u64 msr;
	bool csv3_enabled;

	if (!is_x86_vendor_hygon())
		return false;

	if (sev_status)
		return false;

	/* Check for the SME/CSV support leaf */
	eax = 0x80000000;
	ecx = 0;
	native_cpuid(&eax, &ebx, &ecx, &edx);
	if (eax < 0x8000001f)
		return false;

#define HYGON_SME_BIT	BIT(0)
#define HYGON_CSV3_BIT	BIT(30)
	/*
	 * Check for the CSV feature:
	 * CPUID Fn8000_001F[EAX]
	 * - Bit 0  - SME support
	 * - Bit 1  - CSV support
	 * - Bit 3  - CSV2 support
	 * - Bit 30 - CSV3 support
	 */
	eax = 0x8000001f;
	ecx = 0;
	native_cpuid(&eax, &ebx, &ecx, &edx);
	if (!(eax & HYGON_SME_BIT))
		return false;

	csv3_enabled = !!(eax & HYGON_CSV3_BIT);

	me_mask = 1UL << (ebx & 0x3f);

	/* No SME if Hypervisor bit is set */
	eax = 1;
	ecx = 0;
	native_cpuid(&eax, &ebx, &ecx, &edx);
	if (ecx & BIT(31))
		return false;

	/* For SME, check the SYSCFG MSR */
	msr = __rdmsr(MSR_AMD64_SYSCFG);
	if (!(msr & MSR_AMD64_SYSCFG_MEM_ENCRYPT))
		return false;

	return !!me_mask && csv3_enabled;
}

/******************************************************************************/
/**************************** CSV3 CMA interfaces *****************************/
/******************************************************************************/

/* 0 percent of total memory by default*/
static unsigned char csv_mem_percentage;
static unsigned long csv_mem_size;

static int __init cmdline_parse_csv_mem_size(char *str)
{
	unsigned long size;
	char *endp;

	if (str) {
		size  = memparse(str, &endp);
		csv_mem_size = size;
		if (!csv_mem_size)
			csv_mem_percentage = 0;
	}

	return 0;
}
early_param("csv_mem_size", cmdline_parse_csv_mem_size);

static int __init cmdline_parse_csv_mem_percentage(char *str)
{
	unsigned char percentage;
	int ret;

	if (!str)
		return 0;

	ret  = kstrtou8(str, 10, &percentage);
	if (!ret) {
		csv_mem_percentage = min_t(unsigned char, percentage, 80);
		if (csv_mem_percentage != percentage)
			pr_warn("csv_mem_percentage is limited to 80.\n");
	} else {
		/* Disable CSV CMA. */
		csv_mem_percentage = 0;
		pr_err("csv_mem_percentage is invalid. (0 - 80) is expected.\n");
	}

	return ret;
}
early_param("csv_mem_percentage", cmdline_parse_csv_mem_percentage);

#define NUM_SMR_ENTRIES			(8 * 1024)
#define CSV_CMA_SHIFT			PUD_SHIFT
#define CSV_CMA_SIZE			(1 << CSV_CMA_SHIFT)
#define MIN_SMR_ENTRY_SHIFT		23
#define CSV_SMR_INFO_SIZE		(nr_node_ids * sizeof(struct csv_mem))

struct csv_mem *csv_smr;
EXPORT_SYMBOL_GPL(csv_smr);

unsigned int csv_smr_num;
EXPORT_SYMBOL_GPL(csv_smr_num);

struct csv_cma {
	int fast;
	struct cma *cma;
};

struct cma_array {
	unsigned long count;
	struct csv_cma csv_cma[];
};

static unsigned int smr_entry_shift;
static struct cma_array *csv_contiguous_pernuma_area[MAX_NUMNODES];

static void csv_set_smr_entry_shift(unsigned int shift)
{
	smr_entry_shift = max_t(unsigned int, shift, MIN_SMR_ENTRY_SHIFT);
	pr_info("CSV-CMA: SMR entry size is 0x%x\n", 1 << smr_entry_shift);
}

unsigned int csv_get_smr_entry_shift(void)
{
	return smr_entry_shift;
}
EXPORT_SYMBOL_GPL(csv_get_smr_entry_shift);

static unsigned long __init present_pages_in_node(int nid)
{
	unsigned long range_start_pfn, range_end_pfn;
	unsigned long nr_present = 0;
	int i;

	for_each_mem_pfn_range(i, nid, &range_start_pfn, &range_end_pfn, NULL)
		nr_present += range_end_pfn - range_start_pfn;

	return nr_present;
}

static phys_addr_t __init csv_early_percent_memory_on_node(int nid)
{
	return (present_pages_in_node(nid) * csv_mem_percentage / 100) << PAGE_SHIFT;
}

static void __init csv_cma_reserve_mem(void)
{
	int node, i;
	unsigned long size;
	int idx = 0;
	int count;
	int cma_array_size;
	unsigned long max_spanned_size = 0;

	csv_smr = memblock_alloc_node(CSV_SMR_INFO_SIZE, SMP_CACHE_BYTES, NUMA_NO_NODE);
	if (!csv_smr) {
		pr_err("CSV-CMA: Fail to allocate csv_smr\n");
		return;
	}

	for_each_node_state(node, N_ONLINE) {
		int ret;
		char name[CMA_MAX_NAME];
		struct cma_array *array;
		unsigned long spanned_size;
		unsigned long start = 0, end = 0;
		struct csv_cma *csv_cma;

		size = csv_early_percent_memory_on_node(node);
		count = DIV_ROUND_UP(size, 1 << CSV_CMA_SHIFT);
		if (!count)
			continue;

		cma_array_size = count * sizeof(*csv_cma) + sizeof(*array);
		array = memblock_alloc_node(cma_array_size, SMP_CACHE_BYTES, NUMA_NO_NODE);
		if (!array) {
			pr_err("CSV-CMA: Fail to allocate cma_array\n");
			continue;
		}

		array->count = 0;
		csv_contiguous_pernuma_area[node] = array;

		for (i = 0; i < count; i++) {
			csv_cma = &array->csv_cma[i];
			csv_cma->fast = 1;
			snprintf(name, sizeof(name), "csv-n%dc%d", node, i);
			ret = cma_declare_contiguous_nid(0, CSV_CMA_SIZE, 0,
					1 << CSV_MR_ALIGN_BITS, PMD_SHIFT - PAGE_SHIFT,
					false, name, &(csv_cma->cma), node);
			if (ret) {
				pr_warn("CSV-CMA: Fail to reserve memory size 0x%x node %d\n",
					1 << CSV_CMA_SHIFT, node);
				break;
			}

			if (start > cma_get_base(csv_cma->cma) || !start)
				start = cma_get_base(csv_cma->cma);

			if (end < cma_get_base(csv_cma->cma) + cma_get_size(csv_cma->cma))
				end = cma_get_base(csv_cma->cma) + cma_get_size(csv_cma->cma);
		}

		if (!i)
			continue;

		array->count = i;
		spanned_size = end - start;
		if (spanned_size > max_spanned_size)
			max_spanned_size = spanned_size;

		csv_smr[idx].start = start;
		csv_smr[idx].size  = end - start;
		idx++;

		pr_info("CSV-CMA: Node %d - reserve size 0x%016lx, (expected size 0x%016lx)\n",
			node, (unsigned long)i * CSV_CMA_SIZE, size);
	}

	csv_smr_num = idx;
	WARN_ON((max_spanned_size / NUM_SMR_ENTRIES) < 1);
	if (likely((max_spanned_size / NUM_SMR_ENTRIES) >= 1))
		csv_set_smr_entry_shift(ilog2(max_spanned_size / NUM_SMR_ENTRIES - 1) + 1);
}

#define CSV_CMA_AREAS		2458

void __init early_csv_reserve_mem(void)
{
	unsigned long total_pages;

	/* Only reserve memory on the host that enabled CSV3 feature */
	if (!csv3_check_cpu_support())
		return;

	if (cma_alloc_areas(CSV_CMA_AREAS))
		return;

	total_pages = PHYS_PFN(memblock_phys_mem_size());
	if (csv_mem_size) {
		if (csv_mem_size < (total_pages << PAGE_SHIFT)) {
			csv_mem_percentage = csv_mem_size * 100 / (total_pages << PAGE_SHIFT);
			if (csv_mem_percentage > 80)
				csv_mem_percentage = 80; /* Maximum percentage */
		} else
			csv_mem_percentage = 80; /* Maximum percentage */
	}

	if (!csv_mem_percentage) {
		pr_warn("CSV-CMA: Don't reserve any memory\n");
		return;
	}

	csv_cma_reserve_mem();
}

phys_addr_t csv_alloc_from_contiguous(size_t size, nodemask_t *nodes_allowed,
				unsigned int align)
{
	int nid;
	int nr_nodes;
	struct page *page = NULL;
	phys_addr_t phys_addr;
	int count;
	struct csv_cma *csv_cma;
	int fast = 1;

	if (!nodes_allowed || size > CSV_CMA_SIZE) {
		pr_err("CSV-CMA: Invalid params, size = 0x%lx, nodes_allowed = %p\n",
			size, nodes_allowed);
		return 0;
	}

	align = min_t(unsigned int, align, get_order(CSV_CMA_SIZE));
retry:
	nr_nodes = nodes_weight(*nodes_allowed);

	/* Traverse from current node */
	nid = numa_node_id();
	if (!node_isset(nid, *nodes_allowed))
		nid = next_node_in(nid, *nodes_allowed);

	for (; nr_nodes > 0; nid = next_node_in(nid, *nodes_allowed), nr_nodes--) {
		struct cma_array *array = csv_contiguous_pernuma_area[nid];

		if (!array)
			continue;

		count = array->count;
		while (count) {
			csv_cma = &array->csv_cma[count - 1];

			/*
			 * The value check of csv_cma->fast is lockless, but
			 * that's ok as this don't affect functional correntness
			 * whatever the value of csv_cma->fast.
			 */
			if (fast && !csv_cma->fast) {
				count--;
				continue;
			}
			page = cma_alloc(csv_cma->cma, PAGE_ALIGN(size) >> PAGE_SHIFT,
							align, true);
			if (page) {
				page->private = (unsigned long)csv_cma;
				if (!csv_cma->fast)
					csv_cma->fast = 1;
				goto success;
			} else
				csv_cma->fast = 0;

			count--;
		}
	}

	if (fast) {
		fast = 0;
		goto retry;
	} else {
		pr_err("CSV-CMA: Fail to alloc secure memory(size = 0x%lx)\n", size);
		return 0;
	}

success:
	phys_addr = page_to_phys(page);
	clflush_cache_range(__va(phys_addr), size);

	return phys_addr;
}
EXPORT_SYMBOL_GPL(csv_alloc_from_contiguous);

void csv_release_to_contiguous(phys_addr_t pa, size_t size)
{
	struct csv_cma *csv_cma;
	struct page *page = pfn_to_page(pa >> PAGE_SHIFT);

	WARN_ON(!page);
	if (likely(page)) {
		csv_cma = (struct csv_cma *)page->private;
		WARN_ON(!csv_cma);
		if (likely(csv_cma)) {
			page->private = 0;
			csv_cma->fast = 1;
			cma_release(csv_cma->cma, page, PAGE_ALIGN(size) >> PAGE_SHIFT);
		}
	}
}
EXPORT_SYMBOL_GPL(csv_release_to_contiguous);

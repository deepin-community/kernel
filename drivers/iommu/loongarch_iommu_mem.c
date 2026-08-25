// SPDX-License-Identifier: GPL-2.0
/*
 * Loongson IOMMU Driver
 *
 * Copyright (C) 2026 Loongson Technology Ltd.
 * Author:	Xianglai Li <lixianglai@loongson.cn>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/printk.h>
#include <linux/acpi.h>
#include <linux/memblock.h>
#include "loongarch_iommu.h"

#define ALLOC_MEM SZ_64M
#define ALLOC_PAGE_SIZE IOMMU_PAGE_SIZE
#define ALLOC_PAGES (ALLOC_MEM / ALLOC_PAGE_SIZE)

static struct loongarch_iommu_mem {
	ulong vaddr;
	void *mem_bitmap;
	unsigned long bitmap_sz;
	spinlock_t bitmap_lock;
	bool init_failed;
} iommu_mem;

static int __init loongarch_iommu_mem_init(void)
{
	struct acpi_table_header *ivrs_base;
	acpi_status status;
	phys_addr_t phys;
	struct page *pages;

	status = acpi_get_table("IVRS", 0, &ivrs_base);
	if (ACPI_FAILURE(status)) {
		iommu_mem.init_failed = true;
		pr_info("%s get ivrs table failed\n", __func__);
		return 0;
	}
	acpi_put_table(ivrs_base);

	pages = alloc_contig_pages(ALLOC_PAGES, GFP_KERNEL, numa_mem_id(), &node_online_map);
	if (!pages) {
		iommu_mem.init_failed = true;
		pr_info("%s Unable to alloc memory for iommu page table\n",
				__func__);
		return 0;
	}

	phys = page_to_phys(pages);
	iommu_mem.vaddr = TO_UNCACHE(phys);
	iommu_mem.bitmap_sz = ALLOC_PAGES;
	iommu_mem.mem_bitmap = bitmap_zalloc(iommu_mem.bitmap_sz, GFP_KERNEL);

	spin_lock_init(&iommu_mem.bitmap_lock);
	pr_info("%s alloc iommu page table mem %lx-%lx bitmap %lx map_size %lu\n",
			__func__,
			iommu_mem.vaddr,
			iommu_mem.vaddr + ALLOC_MEM,
			(ulong)iommu_mem.mem_bitmap,
			iommu_mem.bitmap_sz);

	if (!iommu_mem.mem_bitmap) {
		iommu_mem.init_failed = true;
		pr_info("%s Failed to allocate bitmap for iommu\n",
			__func__);
	}
	return 0;
}
arch_initcall(loongarch_iommu_mem_init);

void *loongarch_iommu_alloc_page(void)
{
	void *page;
	unsigned long index;

	if (iommu_mem.init_failed) {
		pr_info("%s iommu mem init failed!!\n", __func__);
		return NULL;
	}

	spin_lock(&iommu_mem.bitmap_lock);
	index = find_first_zero_bit(iommu_mem.mem_bitmap,
				iommu_mem.bitmap_sz);
	if (index < iommu_mem.bitmap_sz)
		__set_bit(index, iommu_mem.mem_bitmap);
	spin_unlock(&iommu_mem.bitmap_lock);

	if (index >= iommu_mem.bitmap_sz) {
		pr_info("%s Insufficient memory index %lu bitmap_sz %lu\n",
				__func__, index, iommu_mem.bitmap_sz);
		return NULL;
	}

	page = (void *)(iommu_mem.vaddr + index * ALLOC_PAGE_SIZE);
	memset(page, 0, ALLOC_PAGE_SIZE);
	pr_debug("%s Using iommu api to alloc memory %lx %lx\n",
			__func__, (ulong)page, iommu_mem.vaddr);


	return page;
}
EXPORT_SYMBOL(loongarch_iommu_alloc_page);

void loongarch_iommu_free_page(void *page)
{
	unsigned long index, offset;

	if (iommu_mem.init_failed) {
		pr_info("%s iommu mem init failed!!\n", __func__);
		return;
	}

	offset = (ulong)page - iommu_mem.vaddr;
	index = offset / ALLOC_PAGE_SIZE;
	if (index >= iommu_mem.bitmap_sz) {
		pr_info("%s Using the wrong api to free memory %lx %lx\n",
				__func__, (ulong)page, iommu_mem.vaddr);
		return;
	}

	spin_lock(&iommu_mem.bitmap_lock);
	__clear_bit(index, iommu_mem.mem_bitmap);
	spin_unlock(&iommu_mem.bitmap_lock);
}
EXPORT_SYMBOL(loongarch_iommu_free_page);

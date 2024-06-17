// SPDX-License-Identifier: GPL-2.0
/*
 * Contiguous Memory Allocator for KVM
 *
 * This program is modified on the basis of CMA, to achieve cross-node
 * memory reservation, as well as reserved memory information statistics.
 */

#define pr_fmt(fmt) "kvm_cma: " fmt

#include <linux/memblock.h>
#include <linux/err.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/cma.h>
#include <linux/page-isolation.h>

#include "../../../mm/cma.h"
#include "../../../mm/internal.h"

struct cma kvm_cma_areas[MAX_CMA_AREAS];
unsigned int kvm_cma_area_count;

static void __init init_kvm_cma_reserved_pageblock(struct page *page)
{
	unsigned int i = pageblock_nr_pages;
	struct page *p = page;

	do {
		__ClearPageReserved(p);
		set_page_count(p, 0);
	} while (++p, --i);

	set_pageblock_migratetype(page, MIGRATE_ISOLATE);

	if (pageblock_order >= MAX_ORDER) {
		i = pageblock_nr_pages;
		p = page;
		do {
			set_page_refcounted(p);
			__free_pages(p, MAX_ORDER - 1);
			p += MAX_ORDER_NR_PAGES;
		} while (i -= MAX_ORDER_NR_PAGES);
	} else {
		set_page_refcounted(page);
		__free_pages(page, pageblock_order);
	}

	adjust_managed_page_count(page, pageblock_nr_pages);
}

static int __init kvm_cma_activate_area(struct cma *cma)
{
	int bitmap_size = BITS_TO_LONGS(cma_bitmap_maxno(cma)) * sizeof(long);
	unsigned long base_pfn = cma->base_pfn, pfn = base_pfn;
	unsigned int i = cma->count >> pageblock_order;

	cma->bitmap = kzalloc(bitmap_size, GFP_KERNEL);

	if (!cma->bitmap) {
		cma->count = 0;
		return -ENOMEM;
	}

	WARN_ON_ONCE(!pfn_valid(pfn));

	do {
		unsigned int j;

		base_pfn = pfn;

		for (j = pageblock_nr_pages; j; --j, pfn++)
			WARN_ON_ONCE(!pfn_valid(pfn));

		init_kvm_cma_reserved_pageblock(pfn_to_page(base_pfn));
	} while (--i);

	spin_lock_init(&cma->lock);

	return 0;
}

static int __init kvm_cma_init_reserved_areas(void)
{
	int i;

	for (i = 0; i < kvm_cma_area_count; i++) {
		int ret = kvm_cma_activate_area(&kvm_cma_areas[i]);

		if (ret)
			return ret;
	}

	return 0;
}
core_initcall(kvm_cma_init_reserved_areas);

/**
 * kvm_cma_init_reserved_mem() - create custom contiguous area
 * from reserved memory
 * @base: Base address of the reserved area
 * @size: Size of the reserved area (in bytes),
 * @order_per_bit: Order of pages represented by one bit on bitmap.
 * @name: The name of the area. If this parameter is NULL, the name of
 *        the area will be set to "cmaN", where N is a running counter of
 *        used areas.
 * @res_cma: Pointer to store the created cma region.
 *
 * This function creates custom contiguous area from already reserved memory.
 */
int __init kvm_cma_init_reserved_mem(phys_addr_t base, phys_addr_t size,
				 unsigned int order_per_bit, const char *name,
				 struct cma **res_cma)
{
	struct cma *cma;
	phys_addr_t alignment;

	/* Sanity checks */
	if (kvm_cma_area_count == ARRAY_SIZE(kvm_cma_areas)) {
		pr_err("Not enough slots for CMA reserved regions!\n");
		return -ENOSPC;
	}

	if (!size || !memblock_is_region_reserved(base, size))
		return -EINVAL;

	/* ensure minimal alignment required by mm core */
	alignment = PAGE_SIZE <<
			max_t(unsigned long, MAX_ORDER - 1, pageblock_order);

	/* alignment should be aligned with order_per_bit */
	if (!IS_ALIGNED(alignment >> PAGE_SHIFT, 1 << order_per_bit))
		return -EINVAL;

	if (ALIGN(base, alignment) != base || ALIGN(size, alignment) != size)
		return -EINVAL;

	/*
	 * Each reserved area must be initialised later, when more kernel
	 * subsystems (like slab allocator) are available.
	 */
	cma = &kvm_cma_areas[kvm_cma_area_count];

	if (name)
		snprintf(cma->name, CMA_MAX_NAME, name);
	else
		snprintf(cma->name, CMA_MAX_NAME,  "cma%d\n", cma_area_count);

	cma->base_pfn = PFN_DOWN(base);
	cma->count = size >> PAGE_SHIFT;
	cma->order_per_bit = order_per_bit;
	*res_cma = cma;
	kvm_cma_area_count++;
	totalcma_pages += (size / PAGE_SIZE);

	return 0;
}

/**
 * kvm_cma_declare_contiguous() - reserve contiguous area for VM
 * @base: Base address of the reserved area optional,
 * @size: Size of the reserved area (in bytes),
 * @limit: End address of the reserved memory (optional, 0 for any).
 * @alignment: Alignment for the CMA area, should be power of 2 or zero
 * @order_per_bit: Order of pages represented by one bit on bitmap.
 * @name: The name of the area. See function cma_init_reserved_mem()
 * @res_cma: Pointer to store the created cma region.
 *
 * This function reserves memory from early allocator. It should be
 * called by arch specific code once the early allocator (memblock or bootmem)
 * has been activated and all other subsystems have already allocated/reserved
 * memory. This function allows to create custom reserved areas.
 */
int __init kvm_cma_declare_contiguous(phys_addr_t base,
			phys_addr_t size, phys_addr_t limit,
			phys_addr_t alignment, unsigned int order_per_bit,
			const char *name, struct cma **res_cma)
{
	phys_addr_t memblock_end = memblock_end_of_DRAM();
	phys_addr_t highmem_start;
	int ret = 0;

	/*
	 * We can't use __pa(high_memory) directly, since high_memory
	 * isn't a valid direct map VA, and DEBUG_VIRTUAL will (validly)
	 * complain. Find the boundary by adding one to the last valid
	 * address.
	 */
	highmem_start = __pa(high_memory - 1) + 1;

	if (!size)
		return -EINVAL;

	if (alignment && !is_power_of_2(alignment))
		return -EINVAL;

	/*
	 * Sanitise input arguments.
	 * Pages both ends in CMA area could be merged into adjacent unmovable
	 * migratetype page by page allocator's buddy algorithm. In the case,
	 * you couldn't get a contiguous memory, which is not what we want.
	 */
	alignment = max(alignment,  (phys_addr_t)PAGE_SIZE <<
			  max_t(unsigned long, MAX_ORDER - 1, pageblock_order));
	if (base & (alignment - 1)) {
		ret = -EINVAL;
		pr_err("Region at %pa must be aligned to %pa bytes\n",
			&base, &alignment);
		goto err;
	}
	base = ALIGN(base, alignment);
	size = ALIGN(size, alignment);
	limit &= ~(alignment - 1);

	if (!base) {
		pr_err("Base address of region must be needed!\n");
		goto err;
	}

	/* size should be aligned with order_per_bit */
	if (!IS_ALIGNED(size >> PAGE_SHIFT, 1 << order_per_bit))
		return -EINVAL;

	/*
	 * The request region must not cross the low/high memory boundary.
	 */
	if (base < highmem_start && base + size > highmem_start) {
		ret = -EINVAL;
		pr_err("Region at %pa defined on low/high memory boundary (%pa)\n",
			&base, &highmem_start);
		goto err;
	}

	/*
	 * If the limit is unspecified or above the memblock end, its effective
	 * value will be the memblock end. Set it explicitly to simplify further
	 * checks.
	 */
	if (limit == 0 || limit > memblock_end)
		limit = memblock_end;

	if (base + size > limit) {
		ret = -EINVAL;
		pr_err("Size (%pa) of region at %pa exceeds limit (%pa)\n",
			&size, &base, &limit);
		goto err;
	}

	/* Reserve memory */
	if (memblock_is_region_reserved(base, size) ||
			memblock_reserve(base, size) < 0) {
		ret = -EBUSY;
		goto err;
	}
	ret = kvm_cma_init_reserved_mem(base, size, order_per_bit,
			name, res_cma);
	if (ret)
		goto free_mem;

	pr_info("Reserved %ld MiB at %pa\n", (unsigned long)size / SZ_1M,
		&base);
	return 0;

free_mem:
	memblock_free((void *)base, size);
err:
	pr_err("Failed to reserve %ld MiB\n", (unsigned long)size / SZ_1M);
	return ret;
}

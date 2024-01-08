// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 1995  Linus Torvalds
 */

/* 2.3.x zone allocator, 1999 Andrea Arcangeli <andrea@suse.de> */

#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/memblock.h>
#include <linux/swiotlb.h>
#include <linux/acpi.h>
#include <linux/memory.h>

#include <asm/pgalloc.h>
#include <asm/mmu_context.h>

struct mem_desc_t mem_desc;
#ifndef CONFIG_NUMA
struct numa_node_desc_t numa_nodes_desc[1];
#endif /* CONFIG_NUMA */

/*
 * empty_zero_page is a special page that is used for
 * zero-initialized data and COW.
 */
unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)] __page_aligned_bss;
EXPORT_SYMBOL(empty_zero_page);
pg_data_t *node_data[MAX_NUMNODES] __read_mostly;
EXPORT_SYMBOL(node_data);

pgd_t swapper_pg_dir[1024]	__aligned(PAGE_SIZE);
static pud_t vmalloc_pud[1024]	__aligned(PAGE_SIZE);

static phys_addr_t mem_start;
static phys_addr_t mem_size_limit;

#ifdef CONFIG_MEMORY_HOTPLUG_SPARSE
unsigned long memory_block_size_bytes(void)
{
	if (is_in_guest())
		return MIN_MEMORY_BLOCK_SIZE_VM_MEMHP;
	else
		return MIN_MEMORY_BLOCK_SIZE;
}
#endif /* CONFIG_MEMORY_HOTPLUG_SPARSE */

static int __init setup_mem_size(char *p)
{
	char *oldp;
	unsigned long start, size;

	start = 0;
	oldp = p;
	size = memparse(p, &p);
	if (p == oldp)
		return -EINVAL;

	if (*p == '@')
		start = memparse(p + 1, &p);

	mem_start = start;
	mem_size_limit = size;
	return 0;
}
early_param("mem", setup_mem_size);

#if defined(CONFIG_SUBARCH_C3B)
pgd_t *
pgd_alloc(struct mm_struct *mm)
{
	pgd_t *ret, *init;

	ret = (pgd_t *)__get_free_page(GFP_KERNEL | __GFP_ZERO);
	init = pgd_offset(&init_mm, 0UL);
	if (ret)
		pgd_val(ret[PTRS_PER_PGD-2]) = pgd_val(init[PTRS_PER_PGD-2]);

	return ret;
}
#elif defined(CONFIG_SUBARCH_C4)
pgd_t *
pgd_alloc(struct mm_struct *mm)
{
	pgd_t *ret;

	ret = (pgd_t *)__get_free_page(GFP_KERNEL | __GFP_ZERO);

	return ret;
}
#endif

/* Set up initial PCB, VPTB, and other such nicities.  */

static inline void
switch_to_system_map(void)
{
	memset(swapper_pg_dir, 0, PAGE_SIZE);
	update_ptbr_sys(virt_to_phys(swapper_pg_dir));
	tbiv();
}

void __init callback_init(void)
{
	pgd_t *pgd;
	p4d_t *p4d;

	switch_to_system_map();

	/* Allocate one PGD and one PUD. */
	pgd = pgd_offset_k(VMALLOC_START);
	p4d = p4d_offset(pgd, VMALLOC_START);
	p4d_populate(&init_mm, p4d, (pud_t *)vmalloc_pud);
}

void __init zone_sizes_init(void)
{
	unsigned long max_zone_pfns[MAX_NR_ZONES];

	memset(max_zone_pfns, 0, sizeof(max_zone_pfns));

#ifdef CONFIG_ZONE_DMA32
	max_zone_pfns[ZONE_DMA32] = min(MAX_DMA32_PFN, max_low_pfn);
#endif
	max_zone_pfns[ZONE_NORMAL] = max_low_pfn;

	free_area_init(max_zone_pfns);
}

/*
 * paging_init() sets up the memory map.
 */
void __init paging_init(void)
{
}

void __init mem_detect(void)
{
	int i;

	mem_desc.phys_base = 0;
	for (i = 0; i < MAX_NUMSOCKETS; i++) {
		if (socket_desc[i].is_online)
			mem_desc.phys_size += socket_desc[i].socket_mem;
	}

	if (mem_start >= NODE0_START) {
		mem_desc.base = mem_start;
	} else {
		mem_desc.base = NODE0_START;
		mem_size_limit -= NODE0_START - mem_start;
	}

	if (mem_size_limit && mem_size_limit < mem_desc.phys_size - NODE0_START)
		mem_desc.size = mem_size_limit;
	else
		mem_desc.size = mem_desc.phys_size - NODE0_START;
}

void __init sw64_memblock_init(void)
{
	memblock_add(mem_desc.base, mem_desc.size);

	memblock_remove(1ULL << MAX_PHYSMEM_BITS, PHYS_ADDR_MAX);

	max_pfn = max_low_pfn = PFN_DOWN(memblock_end_of_DRAM());

	memblock_allow_resize();
	memblock_initialized = true;
	process_memmap();

	/* Make sure kernel text is in memory range. */
	memblock_add(__pa_symbol(_text), _end - _text);
	memblock_reserve(__pa_symbol(_text), _end - _text);

	/* Make sure initrd is in memory range. */
	if (sunway_boot_params->initrd_start) {
		phys_addr_t base = __boot_pa(sunway_boot_params->initrd_start);
		phys_addr_t size = sunway_boot_params->initrd_size;

		memblock_add(base, size);
		memblock_reserve(base, size);
	}

	/* end of DRAM range may have been changed */
	max_pfn = max_low_pfn = PFN_DOWN(memblock_end_of_DRAM());
}

#ifndef CONFIG_NUMA
void __init sw64_numa_init(void)
{
	const size_t nd_size = roundup(sizeof(pg_data_t), SMP_CACHE_BYTES);
	u64 nd_pa;
	void *nd;
	int tnid;

	memblock_set_node(mem_desc.base, mem_desc.size, &memblock.memory, 0);
	nd_pa = memblock_phys_alloc(nd_size, SMP_CACHE_BYTES);
	nd = __va(nd_pa);

	/* report and initialize */
	pr_info("NODE_DATA [mem %#018llx-%#018llx]\n",
		nd_pa, nd_pa + nd_size - 1);
	tnid = early_pfn_to_nid(nd_pa >> PAGE_SHIFT);
	if (tnid != 0)
		pr_info("NODE_DATA(%d) on node %d\n", 0, tnid);

	node_data[0] = nd;
	memset(NODE_DATA(0), 0, sizeof(pg_data_t));
	NODE_DATA(0)->node_id = 0;
	NODE_DATA(0)->node_start_pfn = mem_desc.base >> PAGE_SHIFT;
	NODE_DATA(0)->node_spanned_pages = mem_desc.size >> PAGE_SHIFT;
	node_set_online(0);
}
#endif /* CONFIG_NUMA */

void __init
mem_init(void)
{
	set_max_mapnr(max_low_pfn);
	high_memory = (void *) __va(max_low_pfn * PAGE_SIZE);
#ifdef CONFIG_SWIOTLB
	swiotlb_init(true, SWIOTLB_VERBOSE);
#endif
	memblock_free_all();
}

#ifdef CONFIG_SPARSEMEM_VMEMMAP
int __meminit vmemmap_populate(unsigned long start, unsigned long end, int node,
		struct vmem_altmap *altmap)
{
	return vmemmap_populate_basepages(start, end, node, altmap);
}

void vmemmap_free(unsigned long start, unsigned long end,
		struct vmem_altmap *altmap)
{
}
#endif

#ifdef CONFIG_HAVE_MEMBLOCK
#ifndef MIN_MEMBLOCK_ADDR
#define MIN_MEMBLOCK_ADDR       __pa(PAGE_OFFSET)
#endif
#ifndef MAX_MEMBLOCK_ADDR
#define MAX_MEMBLOCK_ADDR       ((phys_addr_t)~0)
#endif
void __init early_init_dt_add_memory_arch(u64 base, u64 size)
{
	const u64 phys_offset = MIN_MEMBLOCK_ADDR;

	if (acpi_disabled) {
		if (!PAGE_ALIGNED(base)) {
			if (size < PAGE_SIZE - (base & ~PAGE_MASK)) {
				pr_warn("Ignoring memory block 0x%llx - 0x%llx\n",
					base, base + size);
				return;
			}
			size -= PAGE_SIZE - (base & ~PAGE_MASK);
			base = PAGE_ALIGN(base);
		}
		size &= PAGE_MASK;

		if (base > MAX_MEMBLOCK_ADDR) {
			pr_warn("Ignoring memory block 0x%llx - 0x%llx\n",
				base, base + size);
			return;
		}

		if (base + size - 1 > MAX_MEMBLOCK_ADDR) {
			pr_warn("Ignoring memory range 0x%llx - 0x%llx\n",
				((u64)MAX_MEMBLOCK_ADDR) + 1, base + size);
					size = MAX_MEMBLOCK_ADDR - base + 1;
		}

		if (base + size < phys_offset) {
			pr_warn("Ignoring memory block 0x%llx - 0x%llx\n",
				base, base + size);
		return;
		}

		if (base < phys_offset) {
			pr_warn("Ignoring memory range 0x%llx - 0x%llx\n",
				base, phys_offset);
			size -= phys_offset - base;
			base = phys_offset;
		}
		memblock_add(base, size);
	} else
		return;
}
#endif

#ifdef CONFIG_MEMORY_HOTPLUG
int arch_add_memory(int nid, u64 start, u64 size, struct mhp_params *params)
{
	unsigned long start_pfn = start >> PAGE_SHIFT;
	unsigned long nr_pages = size >> PAGE_SHIFT;
	int ret;

	ret = __add_pages(nid, start_pfn, nr_pages, params);
	if (ret)
		pr_warn("%s: Problem encountered in __add_pages() as ret=%d\n",
		       __func__,  ret);

	return ret;
}

void arch_remove_memory(int nid, u64 start, u64 size,
			struct vmem_altmap *altmap)
{
	unsigned long start_pfn = start >> PAGE_SHIFT;
	unsigned long nr_pages = size >> PAGE_SHIFT;

	__remove_pages(start_pfn, nr_pages, altmap);
}
#endif

static const pgprot_t protection_map[16] = {
	[VM_NONE]					= _PAGE_P(_PAGE_FOE | _PAGE_FOW |
								  _PAGE_FOR),
	[VM_READ]					= _PAGE_P(_PAGE_FOE | _PAGE_FOW),
	[VM_WRITE]					= _PAGE_P(_PAGE_FOE),
	[VM_WRITE | VM_READ]				= _PAGE_P(_PAGE_FOE),
	[VM_EXEC]					= _PAGE_P(_PAGE_FOW | _PAGE_FOR),
	[VM_EXEC | VM_READ]				= _PAGE_P(_PAGE_FOW),
	[VM_EXEC | VM_WRITE]				= _PAGE_P(0),
	[VM_EXEC | VM_WRITE | VM_READ]			= _PAGE_P(0),
	[VM_SHARED]					= _PAGE_S(_PAGE_FOE | _PAGE_FOW |
								  _PAGE_FOR),
	[VM_SHARED | VM_READ]				= _PAGE_S(_PAGE_FOE | _PAGE_FOW),
	[VM_SHARED | VM_WRITE]				= _PAGE_S(_PAGE_FOE),
	[VM_SHARED | VM_WRITE | VM_READ]		= _PAGE_S(_PAGE_FOE),
	[VM_SHARED | VM_EXEC]				= _PAGE_S(_PAGE_FOW | _PAGE_FOR),
	[VM_SHARED | VM_EXEC | VM_READ]			= _PAGE_S(_PAGE_FOW),
	[VM_SHARED | VM_EXEC | VM_WRITE]		= _PAGE_S(0),
	[VM_SHARED | VM_EXEC | VM_WRITE | VM_READ]	= _PAGE_S(0)
};
DECLARE_VM_GET_PAGE_PROT

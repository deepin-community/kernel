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
#include <linux/of_fdt.h>
#include <linux/libfdt.h>
#include <linux/initrd.h>
#include <linux/genalloc.h>

#include <asm/pgalloc.h>
#include <asm/mmu_context.h>
#include <asm/platform.h>
#include <asm/kexec.h>
#include <asm/sw64_init.h>
#include <asm/kvm_cma.h>

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

	if (mem_start < NODE0_START) {
		mem_size_limit -= min(mem_size_limit,
				NODE0_START - mem_start);
		mem_start = NODE0_START;
	}

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
#ifdef CONFIG_SUBARCH_C4
	update_ptbr_usr(__pa_symbol(empty_zero_page));
#endif
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

static void __init setup_socket_info(void)
{
	int i;
	int numsockets = sw64_chip->get_cpu_num();

	memset(socket_desc, 0, MAX_NUMSOCKETS * sizeof(struct socket_desc_t));

	for (i = 0; i < numsockets; i++) {
		socket_desc[i].is_online = 1;
		if (sw64_chip_init->early_init.get_node_mem)
			socket_desc[i].socket_mem = sw64_chip_init->early_init.get_node_mem(i);
	}
}

static void __init show_socket_mem_layout(void)
{
	int i;
	phys_addr_t base, size, end;

	base = 0;

	pr_info("Socket memory layout:\n");
	for (i = 0; i < MAX_NUMSOCKETS; i++) {
		if (socket_desc[i].is_online) {
			size = socket_desc[i].socket_mem;
			end = base + size - 1;
			pr_info("Socket %d: [mem %#018llx-%#018llx], size %llu\n",
					i, base, end, size);
			base = end + 1;
		}
	}
	pr_info("Reserved memory size for Socket 0: %#lx\n", NODE0_START);
}

static void __init mem_detect(void)
{
	int i;

	mem_desc.phys_base = 0;
	for (i = 0; i < MAX_NUMSOCKETS; i++) {
		if (socket_desc[i].is_online)
			mem_desc.phys_size += socket_desc[i].socket_mem;
	}

	mem_desc.base = NODE0_START;
	mem_desc.size = mem_desc.phys_size - NODE0_START;
}

#ifdef CONFIG_BLK_DEV_INITRD
static void __init reserve_mem_for_initrd(void)
{
	phys_addr_t phys_initrd_start, initrd_size;

	/**
	 * Get initrd params from boot_params for backward
	 * compatibility. These code can be removed when
	 * no longer support C3B(xuelang).
	 */
	if (sunway_boot_magic != 0xDEED2024UL) {
		initrd_start = sunway_boot_params->initrd_start;
		if (initrd_start) {
			/**
			 * It works regardless of whether the firmware
			 * passes a virtual address or a physical address.
			 *
			 * __boot_pa here is used for compatibility with
			 * old firmware. We can use __pa instead when no
			 * longer support C3B(xuelang).
			 */
			initrd_start = (unsigned long)__va(__boot_pa(initrd_start));
			initrd_end = initrd_start + sunway_boot_params->initrd_size;
		}
	}

	phys_initrd_start = __boot_pa(initrd_start);
	initrd_size = initrd_end - initrd_start;

	if (!initrd_start || !initrd_size) {
		pr_info("No initrd found\n");
		return;
	}

	pr_info("Initial ramdisk at: 0x%lx(va)/0x%llx(pa) (%llu bytes)\n",
			initrd_start, phys_initrd_start, initrd_size);

	/**
	 * Usually, it means that there is an error in the
	 * initrd params. We should check the firmware.
	 */
	if ((phys_initrd_start + initrd_size) > memblock_end_of_DRAM()) {
		/* Disable initrd */
		initrd_start = 0;
		initrd_end = 0;
		pr_err("Initial ramdisk exceed DRAM limitation\n");
		return;
	}

	/* Reserve initrd */
	memblock_add(phys_initrd_start, initrd_size);
	memblock_reserve(phys_initrd_start, initrd_size);
}
#endif /* CONFIG_BLK_DEV_INITRD */

#ifdef CONFIG_SUBARCH_C3B
#if defined(CONFIG_KVM) || defined(CONFIG_KVM_MODULE)
struct cma *sw64_kvm_cma;
EXPORT_SYMBOL(sw64_kvm_cma);

static phys_addr_t kvm_mem_size;
static phys_addr_t kvm_mem_base;

struct gen_pool *sw64_kvm_pool;
EXPORT_SYMBOL(sw64_kvm_pool);

static int __init early_kvm_reserved_mem(char *p)
{
	if (!p) {
		pr_err("Config string not provided\n");
		return -EINVAL;
	}

	kvm_mem_size = memparse(p, &p);
	if (*p != '@')
		return -EINVAL;
	kvm_mem_base = memparse(p + 1, &p);
	return 0;
}
early_param("kvm_mem", early_kvm_reserved_mem);

void __init sw64_kvm_reserve(void)
{
	kvm_cma_declare_contiguous(kvm_mem_base, kvm_mem_size, 0,
			PAGE_SIZE, 0, "sw64_kvm_cma", &sw64_kvm_cma);
}

static int __init sw64_kvm_pool_init(void)
{
	int status = 0;
	unsigned long kvm_pool_virt;
	struct page *base_page, *end_page, *p;

	if (!sw64_kvm_cma)
		goto out;

	kvm_pool_virt = (unsigned long)kvm_mem_base;

	sw64_kvm_pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!sw64_kvm_pool)
		goto out;

	status = gen_pool_add_virt(sw64_kvm_pool, kvm_pool_virt, kvm_mem_base,
			kvm_mem_size, -1);
	if (status < 0) {
		pr_err("failed to add memory chunks to sw64 kvm pool\n");
		gen_pool_destroy(sw64_kvm_pool);
		sw64_kvm_pool = NULL;
		goto out;
	}
	gen_pool_set_algo(sw64_kvm_pool, gen_pool_best_fit, NULL);

	base_page = pfn_to_page(kvm_mem_base >> PAGE_SHIFT);
	end_page  = pfn_to_page((kvm_mem_base + kvm_mem_size - 1) >> PAGE_SHIFT);

	p = base_page;
	while (p <= end_page && page_ref_count(p) == 0) {
		set_page_count(p, 1);
		page_mapcount_reset(p);
		SetPageReserved(p);
		p++;
	}

	return status;

out:
	return -ENOMEM;
}
core_initcall_sync(sw64_kvm_pool_init);
#endif
#endif

void __init sw64_memblock_init(void)
{
	if (sunway_boot_magic != 0xDEED2024UL) {
		/**
		 * Detect all memory on all nodes, used in the following
		 * cases:
		 * 1. Legacy memory detect
		 * 2. Legacy NUMA initialization
		 */
		setup_socket_info();
		show_socket_mem_layout();

		/* Find our usable memory */
		mem_detect();

		/* Add usable memory */
		memblock_add(mem_desc.base, mem_desc.size);
	}

	memblock_remove(1ULL << MAX_PHYSMEM_BITS, PHYS_ADDR_MAX);

	max_pfn = max_low_pfn = PFN_DOWN(memblock_end_of_DRAM());

#ifdef CONFIG_PCI
	reserve_mem_for_pci();
#endif

	memblock_allow_resize();
	memblock_initialized = true;
	process_memmap();

	/* Make sure kernel text is in memory range. */
	memblock_add(__pa_symbol(_text), _end - _text);
	memblock_reserve(__pa_symbol(_text), _end - _text);

#ifdef CONFIG_BLK_DEV_INITRD
	/* Make sure initrd is in memory range. */
	reserve_mem_for_initrd();
#endif

#ifdef CONFIG_SUBARCH_C3B
#if defined(CONFIG_KVM) || defined(CONFIG_KVM_MODULE)
	/* Reserve large chunks of memory for use by CMA for KVM. */
	sw64_kvm_reserve();
#endif
#endif

	reserve_crashkernel();

	/* All memory has been added, it's time to handle memory limitation */
	if (mem_size_limit) {
		memblock_remove(0, mem_start);
		memblock_remove(mem_start + mem_size_limit, PHYS_ADDR_MAX);
		if (sunway_boot_magic != 0xDEED2024UL) {
			mem_desc.base = mem_start;
			mem_desc.size = memblock_phys_mem_size();
		}
	}

	early_init_fdt_scan_reserved_mem();

	/* end of DRAM range may have been changed */
	max_pfn = max_low_pfn = PFN_DOWN(memblock_end_of_DRAM());
}

#ifndef CONFIG_NUMA
void __init sw64_numa_init(void)
{
	phys_addr_t mem_base = memblock_start_of_DRAM();
	phys_addr_t mem_size = memblock_phys_mem_size();
	const size_t nd_size = roundup(sizeof(pg_data_t), SMP_CACHE_BYTES);
	u64 nd_pa;
	void *nd;
	int tnid;

	memblock_set_node(mem_base, mem_size, &memblock.memory, 0);
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
	NODE_DATA(0)->node_start_pfn = mem_base >> PAGE_SHIFT;
	NODE_DATA(0)->node_spanned_pages = mem_size >> PAGE_SHIFT;
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

void arch_remove_memory(u64 start, u64 size, struct vmem_altmap *altmap)
{
	unsigned long start_pfn = start >> PAGE_SHIFT;
	unsigned long nr_pages = size >> PAGE_SHIFT;

	__remove_pages(start_pfn, nr_pages, altmap);
}
#endif

static const pgprot_t protection_map[16] = {
	[VM_NONE]					= PAGE_NONE,
	[VM_READ]                                       = PAGE_READONLY_NOEXEC,
	[VM_WRITE]                                      = PAGE_COPY_NOEXEC,
	[VM_WRITE | VM_READ]                            = PAGE_COPY_NOEXEC,
	[VM_EXEC]                                       = PAGE_EXEC,
	[VM_EXEC | VM_READ]                             = PAGE_READONLY_EXEC,
	[VM_EXEC | VM_WRITE]                            = PAGE_COPY_EXEC,
	[VM_EXEC | VM_WRITE | VM_READ]                  = PAGE_COPY_EXEC,
	[VM_SHARED]                                     = PAGE_NONE,
	[VM_SHARED | VM_READ]                           = PAGE_READONLY_NOEXEC,
	[VM_SHARED | VM_WRITE]                          = PAGE_SHARED_NOEXEC,
	[VM_SHARED | VM_WRITE | VM_READ]                = PAGE_SHARED_NOEXEC,
	[VM_SHARED | VM_EXEC]                           = PAGE_EXEC,
	[VM_SHARED | VM_EXEC | VM_READ]                 = PAGE_READONLY_EXEC,
	[VM_SHARED | VM_EXEC | VM_WRITE]                = PAGE_SHARED_EXEC,
	[VM_SHARED | VM_EXEC | VM_WRITE | VM_READ]      = PAGE_SHARED_EXEC
};
DECLARE_VM_GET_PAGE_PROT

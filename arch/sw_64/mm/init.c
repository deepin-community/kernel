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
#include <linux/set_memory.h>

#include <asm/pgalloc.h>
#include <asm/mmu_context.h>
#include <asm/platform.h>
#include <asm/kexec.h>
#include <asm/sw64_init.h>
#include <asm/kvm_cma.h>
#include <asm/fixmap.h>
#include <asm/tlbflush.h>

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

#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE
pgd_t early_pg_dir[1024] __initdata __attribute__((__aligned__(PAGE_SIZE)));

pmd_t early_pmd[PTRS_PER_PMD] __initdata __aligned(PAGE_SIZE);
pud_t early_pud[PTRS_PER_PUD] __initdata __aligned(PAGE_SIZE);
pmd_t early_dtb_pmd[PTRS_PER_PMD] __initdata __aligned(PAGE_SIZE);
pud_t early_dtb_pud[PTRS_PER_PUD] __initdata __aligned(PAGE_SIZE);
pmd_t early_printk_pmd[PTRS_PER_PMD] __initdata __aligned(PAGE_SIZE);
pud_t early_printk_pud[PTRS_PER_PUD] __initdata __aligned(PAGE_SIZE);

pte_t fixmap_pte[PTRS_PER_PTE] __page_aligned_bss;
pmd_t fixmap_pmd[PTRS_PER_PMD] __page_aligned_bss;
pud_t fixmap_pud[PTRS_PER_PUD] __page_aligned_bss;
#endif /* CONFIG_SW64_KERNEL_PAGE_TABLE */

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
#ifndef CONFIG_SW64_KERNEL_PAGE_TABLE
	memset(swapper_pg_dir, 0, PAGE_SIZE);
#endif
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

#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE
void __set_fixmap(enum fixed_addresses idx, phys_addr_t phys, pgprot_t prot)
{
	unsigned long addr = __fix_to_virt(idx);
	pte_t *ptep;

	BUG_ON(idx <= FIX_HOLE || idx >= __end_of_fixed_addresses);

	ptep = &fixmap_pte[pte_index(addr)];

	if (pgprot_val(prot))
		set_pte(ptep, pfn_pte(PHYS_PFN(phys), prot));
	else
		pte_clear(&init_mm, addr, ptep);

	local_flush_tlb_all();
}

static pte_t *__init get_pte_virt_fixmap(phys_addr_t phys)
{
	clear_fixmap(FIX_PTE);
	return (pte_t *)set_fixmap_offset(FIX_PTE, phys);
}

static pmd_t *__init get_pmd_virt_fixmap(phys_addr_t phys)
{
	clear_fixmap(FIX_PMD);
	return (pmd_t *)set_fixmap_offset(FIX_PMD, phys);
}

static pud_t *__init get_pud_virt_fixmap(phys_addr_t phys)
{
	clear_fixmap(FIX_PUD);
	return (pud_t *)set_fixmap_offset(FIX_PUD, phys);
}

void * __init pgtable_alloc_fixmap(void)
{
	return (void *)__va(memblock_phys_alloc(PAGE_SIZE, PAGE_SIZE));
}

static void __init
create_pte_mapping(pte_t *pte_first, unsigned long virt, unsigned long phys,
		   unsigned long size, pgprot_t prot)
{
	pte_t *pte;
	unsigned long addr, next, end, pfn;

	addr = virt;
	end = virt + size;
	pte_first = get_pte_virt_fixmap(__pa(pte_first));
	for (; addr < end; addr = next) {
		next = (addr + PAGE_SIZE) &  PAGE_MASK;
		pte = pte_first + pte_index(addr);
		pfn = PHYS_PFN(phys);
		set_pte(pte, pfn_pte(pfn, prot));
		phys += next - addr;
	}
}

static void __init
create_pmd_mapping(pmd_t *pmd_first, unsigned long virt, unsigned long phys,
		   unsigned long size, pgprot_t prot,
		   void *(*pgtable_alloc)(void))
{
	pmd_t *pmd;
	pte_t *pte;
	unsigned long addr, next, end, pfn;

	addr = virt;
	end = virt + size;
	for (; addr < end; addr = next) {
		next = pmd_addr_end(addr, end);
		pmd = pmd_first + pmd_index(addr);

		if (next - addr == PMD_SIZE) {
			pfn = PHYS_PFN(phys);
			set_pmd(pmd, pfn_pmd(pfn, prot));
		} else {
			if (!pmd_none(*pmd))
				pte = pte_offset_kernel(pmd, 0);
			else {
				pte = (pte_t *)pgtable_alloc();
				memset(get_pte_virt_fixmap(__pa(pte)), 0,
				       PAGE_SIZE);
				pmd_populate(NULL, pmd,
					     virt_to_page((unsigned long)pte));
			}
			create_pte_mapping(pte, addr, phys, next - addr, prot);
		}
		phys += next - addr;
	}
}

static void __init
create_cont_pmd_mapping(pmd_t *pmd_first, unsigned long virt,
			unsigned long phys, unsigned long size, pgprot_t prot,
			void *(*pgtable_alloc)(void))
{
	pmd_t *pmd;
	unsigned long addr, next, end, pfn, i;

	addr = virt;
	end = virt + size;
	pmd_first = get_pmd_virt_fixmap(__pa(pmd_first));
	for (; addr < end; addr = next) {
		next = cont_pmd_addr_end(addr, end);
		pmd = pmd_first + pmd_index(addr);

		if ((next - addr == CONT_PMD_SIZE) &&
		    (PTRS_PER_PMD - pmd_index(addr) >= CONT_PMDS)) {
			pfn = PHYS_PFN(phys);
			for (i = 0; i < CONT_PMDS; i++)
				set_pmd(pmd + i, pfn_pmd(pfn,
					__pgprot(pgprot_val(prot) |
					_PAGE_CONT)));
		} else
			create_pmd_mapping(pmd_first, addr, phys, next - addr,
					   prot, pgtable_alloc);
		phys += next - addr;
	}
}

static void __init
create_pud_mapping(pud_t *pud_first, unsigned long virt, unsigned long phys,
		   unsigned long size, pgprot_t prot,
		   void *(*pgtable_alloc)(void))
{
	pud_t *pud;
	pmd_t *pmd;
	unsigned long addr, next, end, pfn;

	addr = virt;
	end = virt + size;
	pud_first = get_pud_virt_fixmap(__pa(pud_first));
	for (; addr < end; addr = next) {
		next = pud_addr_end(addr, end);
		pud = pud_first + pud_index(addr);
		if (next - addr == PUD_SIZE) {
			pfn = PHYS_PFN(phys);
			set_pud(pud, pfn_pud(pfn, prot));
		} else {
			if (!pud_none(*pud))
				pmd = pmd_offset(pud, 0);
			else {
				pmd = (pmd_t *)pgtable_alloc();
				memset(get_pmd_virt_fixmap(__pa(pmd)), 0,
				       PAGE_SIZE);
				pud_populate(NULL, pud, pmd);
			}
			create_cont_pmd_mapping(pmd, addr, phys, next - addr,
						prot, pgtable_alloc);
		}
		phys += next - addr;
	}
}

void __init
create_pgd_mapping(pgd_t *pgdir, unsigned long virt, unsigned long phys,
		   unsigned long size, pgprot_t prot,
		   void *(*pgtable_alloc)(void))
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	unsigned long addr, next, end;

	addr = virt & PAGE_MASK;
	phys &= PAGE_MASK;
	end = PAGE_ALIGN(virt + size);
	for (; addr < end; addr = next) {
		next = pgd_addr_end(addr, end);
		pgd = pgd_offset_pgd(pgdir, addr);
		p4d = p4d_offset(pgd, addr);

		if (!p4d_none(*p4d))
			pud = pud_offset(p4d, 0);
		else {
			pud = (pud_t *)pgtable_alloc();
			memset(get_pud_virt_fixmap(__pa(pud)), 0, PAGE_SIZE);
			p4d_populate(NULL, p4d, pud);
		}
		create_pud_mapping(pud, addr, phys, next - addr, prot,
				   pgtable_alloc);
		phys += next - addr;
	}
	clear_fixmap(FIX_PTE);
	clear_fixmap(FIX_PMD);
	clear_fixmap(FIX_PUD);
}

static void __init early_create_pmd(pgd_t *pgdir, pud_t *pud, pmd_t *pmd,
		unsigned long start_va, unsigned long size, unsigned long pa)
{
	pgd_t *pgdp;
	p4d_t *p4dp;
	pud_t *pudp;
	pmd_t *pmdp;
	unsigned long addr, end_va;
	int pmd_num, i;

	addr = start_va & PMD_MASK;
	end_va = start_va + size;
	pmd_num = (end_va - addr) / PMD_SIZE;
	if (end_va % PMD_SIZE)
		pmd_num += 1;

	pgdp = pgd_offset_pgd(pgdir, addr);
	p4dp = p4d_offset(pgdp, addr);
	if (p4d_none(*p4dp)) {
		BUG_ON(!pud);
		p4d_populate(NULL, p4dp, pud);
	}
	pudp = pud_offset(p4dp, addr);
	if (pud_none(*pudp)) {
		BUG_ON(!pmd);
		pud_populate(NULL, pudp, pmd);
	}

	for (i = 0; i < pmd_num; i++) {
		pmdp = pmd_offset(pudp, addr);
		set_pmd(pmdp, pfn_pmd(PHYS_PFN(pa), PAGE_KERNEL));
		addr += PMD_SIZE;
		pa += PMD_SIZE;
	}
}

static void __init fixmap_init(pgd_t *pgdir)
{
	pgd_t *pgdp;
	p4d_t *p4dp;
	pud_t *pudp;
	pmd_t *pmdp;
	unsigned long addr = FIXADDR_START & PMD_MASK;

	pgdp = pgd_offset_pgd(pgdir, addr);
	p4dp = p4d_offset(pgdp, addr);
	if (p4d_none(*p4dp))
		p4d_populate(NULL, p4dp, (pud_t *)fixmap_pud);
	pudp = pud_offset(p4dp, addr);
	if (pud_none(*pudp))
		pud_populate(NULL, pudp, (pmd_t *)fixmap_pmd);
	pmdp = pmd_offset(pudp, addr);
	if (pmd_none(*pmdp))
		pmd_populate(NULL, pmdp, virt_to_page(fixmap_pte));
}

/*
 * Map legacy io to K segmemt in advance.
 */
extern unsigned long legacy_io_base;
extern unsigned long legacy_io_shift;
static void __init map_legacy_io(pgd_t *pgdir)
{
	unsigned long pci_io_start;
	unsigned long lpc_legacy_io_start = LPC_LEGACY_IO;
	unsigned long legacy_io_start = legacy_io_base;
	unsigned long size = 0x10000;
	unsigned long i, j;
	pgprot_t prot_none;

	prot_none = __pgprot(pgprot_val(PAGE_KERNEL_READONLY) | _PAGE_FOW);

	for (i = 0; i < 2; i++) {
		for (j = 0; j < 6; j++) {
			pci_io_start = SW64_PCI_IO_BASE(i, j) | PCI_LEGACY_IO;
			create_pgd_mapping(pgdir, (unsigned long)__va(pci_io_start), pci_io_start,
					   size, prot_none, pgtable_alloc_fixmap);
		}
	}
	create_pgd_mapping(pgdir, (unsigned long)__va(legacy_io_start), legacy_io_start,
			   size << legacy_io_shift, PAGE_KERNEL_NOEXEC, pgtable_alloc_fixmap);
	create_pgd_mapping(pgdir, (unsigned long)__va(lpc_legacy_io_start), lpc_legacy_io_start,
			   size, PAGE_KERNEL_NOEXEC, pgtable_alloc_fixmap);
}
#endif /* CONFIG_SW64_KERNEL_PAGE_TABLE */

/*
 * early_paging_init sets up a temporary memory map.
 */
void __init early_paging_init(void)
{
#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE
	unsigned long img_start, img_size;
	unsigned long dtb_start, dtb_size = 0;

	img_start = (unsigned long)(KERNEL_START_PHYS + __START_KERNEL_map);
	img_size = (unsigned long)_end - img_start;
	dtb_start = sunway_dtb_address;

	fixmap_init(early_pg_dir);

#ifdef CONFIG_SW64_RRK
	early_create_pmd(early_pg_dir, (pud_t *)early_printk_pud,
			(pmd_t *)early_printk_pmd, KERNEL_PRINTK_BUFF_BASE,
			PRINTK_SIZE, __pa(KERNEL_PRINTK_BUFF_BASE));
#endif
	early_create_pmd(early_pg_dir, (pud_t *)early_pud, (pmd_t *)early_pmd,
			img_start, img_size, __pa(img_start));
	if (dtb_start) {
		dtb_size = (unsigned long)fdt_totalsize((void *)dtb_start);
		early_create_pmd(early_pg_dir, (pud_t *)early_dtb_pud,
				(pmd_t *)early_dtb_pmd, dtb_start, dtb_size,
				__pa(dtb_start));
	}
	update_ptbr_sys(virt_to_phys(early_pg_dir));

	/* switch to paging mode */
	if (sunway_support_kpt) {
		pr_info("SW64 kernel page table enabled\n");
		set_atc(ATC_PAGE);
	}

	tbiv();
#endif /* CONFIG_SW64_KERNEL_PAGE_TABLE */
}

/*
 * paging_init() sets up the final memory map.
 */
void __init paging_init(void)
{
#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE
	unsigned long sw64_vcpucb_start = PAGE_OFFSET + 0x20000;
	unsigned long sw64_vcpucb_size = 0x60000;
	unsigned long sw64_reserve_start = CONFIG_PHYSICAL_START + PAGE_OFFSET;
	unsigned long sw64_reserve_size = (unsigned long)_stext - sw64_reserve_start;
	unsigned long text_start = (unsigned long)_stext;
	unsigned long text_size = (unsigned long)_etext - text_start;
	unsigned long ro_start = (unsigned long)__start_rodata;
	unsigned long ro_size = (unsigned long)__init_begin - ro_start;
	unsigned long init_start = (unsigned long)__init_begin;
	unsigned long init_size = (unsigned long)__init_end - init_start;
	unsigned long data_start = (unsigned long)_sdata;
	unsigned long data_size = (unsigned long)_end - data_start;
	pgd_t *pgdir = (&init_mm)->pgd;
	phys_addr_t start, end;
	u64 i;

	fixmap_init(pgdir);

	map_legacy_io(pgdir);

	create_pgd_mapping(pgdir, sw64_vcpucb_start, __pa(sw64_vcpucb_start),
			   sw64_vcpucb_size, PAGE_KERNEL_NOEXEC, pgtable_alloc_fixmap);
#ifdef CONFIG_SW64_RRU
	create_pgd_mapping(pgdir, USER_PRINT_BUFF_BASE, __pa(USER_PRINT_BUFF_BASE),
			   USER_PRINT_BUFF_LEN, PAGE_KERNEL_NOEXEC,
			   pgtable_alloc_fixmap);
#endif
#ifdef CONFIG_SW64_RRK
	create_pgd_mapping(pgdir, KERNEL_PRINTK_BUFF_BASE, __pa(KERNEL_PRINTK_BUFF_BASE),
			   PRINTK_SIZE, PAGE_KERNEL_NOEXEC, pgtable_alloc_fixmap);
#endif
	create_pgd_mapping(pgdir, sw64_reserve_start, __pa(sw64_reserve_start),
			   sw64_reserve_size, PAGE_KERNEL_NOEXEC,
			   pgtable_alloc_fixmap);
	create_pgd_mapping(pgdir, text_start, __pa(text_start), text_size,
			   PAGE_KERNEL_READONLY_EXEC, pgtable_alloc_fixmap);
	create_pgd_mapping(pgdir, ro_start, __pa(ro_start), ro_size,
			   PAGE_KERNEL_NOEXEC, pgtable_alloc_fixmap);
	create_pgd_mapping(pgdir, init_start, __pa(init_start), init_size,
			   PAGE_KERNEL, pgtable_alloc_fixmap);
	create_pgd_mapping(pgdir, data_start, __pa(data_start), data_size,
			   PAGE_KERNEL_NOEXEC, pgtable_alloc_fixmap);

	memblock_mark_nomap(__pa(sw64_reserve_start),
			    __pa((unsigned long)_end - sw64_reserve_start));
	for_each_mem_range(i, &start, &end) {
		if (start >= end)
			break;
		create_pgd_mapping(pgdir, (unsigned long)__va(start),
				   (unsigned long)start,
				   (unsigned long)(end - start),
				   PAGE_KERNEL_NOEXEC, pgtable_alloc_fixmap);
	}
	memblock_clear_nomap(__pa(sw64_reserve_start),
			     __pa((unsigned long)_end - sw64_reserve_start));
#endif /* CONFIG_SW64_KERNEL_PAGE_TABLE */
}

#ifdef CONFIG_STRICT_KERNEL_RWX
void mark_rodata_ro(void)
{
	unsigned long ro_start = (unsigned long)__start_rodata;
	unsigned long ro_size = (unsigned long)__init_begin - ro_start;

	if (sunway_support_kpt)
		set_memory_ro(ro_start, PAGE_ALIGN(ro_size) >> PAGE_SHIFT);
}
#endif

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

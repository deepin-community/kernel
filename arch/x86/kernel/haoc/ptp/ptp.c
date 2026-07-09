// SPDX-License-Identifier: GPL-2.0
#include <asm/set_memory.h>
#include <asm/haoc/iee.h>
#include <asm/haoc/ptp.h>
#include <asm-generic/sections.h>
#include <asm/pgalloc.h>
#include <linux/memblock.h>
#include <asm/haoc/iee-func.h>
#include <linux/types.h>
#include <linux/init.h>

#define __pte_to_phys(pte)		(pte_pfn(pte) << PAGE_SHIFT)
#define __pmd_to_phys(pmd)		(__pte_to_phys(__pte(pmd_val(pmd))))
#define __pud_to_phys(pud)		(__pte_to_phys(__pte(pud_val(pud))))
#define __p4d_to_phys(p4d)		(__pte_to_phys(__pte(p4d_val(p4d))))
#define __pgd_to_phys(pgd)		(__pte_to_phys(__pte(pgd_val(pgd))))

static void __init _ptp_set_pte_table_ro(pmd_t *pmdp, unsigned long addr, unsigned long end)
{
	pmd_t pmd = READ_ONCE(*pmdp);
	unsigned long logical_addr = (unsigned long)__va(__pmd_to_phys(pmd));

	set_iee_page(logical_addr, 1, IEE_FIXED_PGTABLE);
}

static void __init _ptp_set_pmd_table_ro(pud_t *pudp, unsigned long addr, unsigned long end)
{
	unsigned long next;
	pud_t pud = READ_ONCE(*pudp);
	pmd_t *pmdp;
	pmd_t pmd;
	unsigned long logical_addr = (unsigned long)__va(__pud_to_phys(pud));

	set_iee_page(logical_addr, 1, IEE_FIXED_PGTABLE);
	pmdp = pmd_offset(pudp, addr);
	do {
		next = pmd_addr_end(addr, end);
		pmd = READ_ONCE(*pmdp);
		if (pmd_val(pmd) & _PSE)
			continue;
		else
			_ptp_set_pte_table_ro(pmdp, addr, next);
	} while (pmdp++, addr = next, addr != end);
}

static void __init _ptp_set_pud_table_ro(p4d_t *p4dp, unsigned long addr, unsigned long end)
{
	unsigned long next;
	p4d_t p4d = READ_ONCE(*p4dp);
	pud_t *pudp;
	pud_t pud;
	unsigned long logical_addr = (unsigned long)__va(__p4d_to_phys(p4d));

	set_iee_page(logical_addr, 1, IEE_FIXED_PGTABLE);
	pudp = pud_offset(p4dp, addr);
	do {
		next = pud_addr_end(addr, end);
		pud = READ_ONCE(*pudp);
		if (pud_val(pud) & _PSE) {
			// _PSE = 1 means a page, not a table
			continue;
		} else {
			_ptp_set_pmd_table_ro(pudp, addr, next);
		}
	} while (pudp++, addr = next, addr != end);
}

static void __init _ptp_set_p4d_table_ro(pgd_t *pgdp, unsigned long addr, unsigned long end)
{
	unsigned long next;
	pgd_t pgd = READ_ONCE(*pgdp);
	p4d_t *p4dp;
	p4d_t p4d;
	unsigned long logical_addr = (unsigned long)__va(__pgd_to_phys(pgd));

	set_iee_page(logical_addr, 1, IEE_FIXED_PGTABLE);
	p4dp = p4d_offset(pgdp, addr);
	do {
		next = p4d_addr_end(addr, end);
		p4d = READ_ONCE(*p4dp);
		/* No 512 GiB huge pages yet */
		_ptp_set_pud_table_ro(p4dp, addr, next);
	} while (p4dp++, addr = next, addr != end);
}

static void __init _ptp_mark_iee_pgtable_for_one_region_ro(pgd_t *pgdir,
					unsigned long va_start, unsigned long va_end)
{
	unsigned long addr, end, next;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, va_start);

	addr = va_start & PAGE_MASK;
	end = PAGE_ALIGN(va_end);

	do {
		next = pgd_addr_end(addr, end);
		_ptp_set_p4d_table_ro(pgdp, addr, next);
	} while (pgdp++, addr = next, addr != end);
}

static void __init ptp_mark_all_pgtable_ro(void)
{
	unsigned long logical_addr;
	phys_addr_t start, end;
	u64 i;
	pgd_t *pgdp;

	// handing 1-level page table swapper_pg_dir
	pgdp = swapper_pg_dir;
	set_iee_page((unsigned long)swapper_pg_dir, 1, IEE_PGTABLE);
	logical_addr = (unsigned long)__va(__pa_symbol(swapper_pg_dir));
	set_iee_page(logical_addr, 1, IEE_PGTABLE);

	// handling 2/3/4/5-level page table for kernel
	_ptp_mark_iee_pgtable_for_one_region_ro(pgdp,
	(unsigned long)_text, (unsigned long)_etext);
	_ptp_mark_iee_pgtable_for_one_region_ro(pgdp,
	(unsigned long)__start_rodata, (unsigned long)__end_rodata);
	_ptp_mark_iee_pgtable_for_one_region_ro(pgdp,
	(unsigned long)_sdata, (unsigned long)_edata);
	_ptp_mark_iee_pgtable_for_one_region_ro(pgdp,
	(unsigned long)__bss_start, (unsigned long)__bss_stop);

	// handling 2/3/4/5-level statically allocated page table
	#ifdef CONFIG_X86_5LEVEL
	set_iee_page((unsigned long)level4_kernel_pgt, 1, IEE_PGTABLE);
	logical_addr = (unsigned long)__va(__pa_symbol(level4_kernel_pgt));
	set_iee_page(logical_addr, 1, IEE_PGTABLE);
	#endif

	set_iee_page((unsigned long)level3_kernel_pgt, 1, IEE_PGTABLE);
	logical_addr = (unsigned long)__va(__pa_symbol(level3_kernel_pgt));
	set_iee_page(logical_addr, 1, IEE_PGTABLE);

	set_iee_page((unsigned long)level2_kernel_pgt, 1, IEE_PGTABLE);
	logical_addr = (unsigned long)__va(__pa_symbol(level2_kernel_pgt));
	set_iee_page(logical_addr, 1, IEE_PGTABLE);

	set_iee_page((unsigned long)level2_fixmap_pgt, FIXMAP_PMD_NUM, IEE_PGTABLE);
	logical_addr = (unsigned long)__va(__pa_symbol(level2_fixmap_pgt));
	set_iee_page(logical_addr, FIXMAP_PMD_NUM, IEE_PGTABLE);

	set_iee_page((unsigned long)level1_fixmap_pgt, FIXMAP_PMD_NUM, IEE_PGTABLE);
	logical_addr = (unsigned long)__va(__pa_symbol(level1_fixmap_pgt));
	set_iee_page(logical_addr, FIXMAP_PMD_NUM, IEE_PGTABLE);

	// handling 2/3/4-level page table for logical mem and iee
	for_each_mem_range(i, &start, &end) {
		if (start >= end)
			break;
		/*
		 * The linear map must allow allocation tags reading/writing
		 * if MTE is present. Otherwise, it has the same attributes as
		 * PAGE_KERNEL.
		 */
		_ptp_mark_iee_pgtable_for_one_region_ro(pgdp,
		(unsigned long)__va(start), (unsigned long)__va(end));
		_ptp_mark_iee_pgtable_for_one_region_ro(pgdp,
		(unsigned long)__phys_to_iee(start), (unsigned long)__phys_to_iee(end));
	}
	pr_info("PTP: mark all kernel page tables ro done");
}

static inline void check_addr_range_ro(unsigned long addr_start, unsigned long addr_end,
				unsigned long start, unsigned long end, pte_t pte, const char *msg)
{
	if (unlikely(!haoc_init_done))
		return;

	if (unlikely((addr_start >= start && addr_start < end)
				|| (addr_end > start && addr_end < end)))
		if (unlikely(pte_write(pte)))
			panic("IEE Error: Are you trying to write %s: [0x%lx, 0x%lx]?",
					msg, addr_start, addr_end);
}

static inline void check_dep_and_ro(unsigned long image_addr, unsigned long size, pte_t pte)
{
	if (unlikely(!haoc_init_done))
		return;

	if (unlikely(!kernel_set_to_readonly))
		return;

	check_addr_range_ro(image_addr, image_addr + size,
(unsigned long)_stext, (unsigned long)_etext, pte, "text");
	check_addr_range_ro(image_addr, image_addr + size,
(unsigned long)__start_rodata, (unsigned long)__end_rodata, pte, "rodata");
#ifdef CONFIG_IEE_SIP
	check_addr_range_ro(image_addr, image_addr + size,
(unsigned long)__iee_si_text_start, (unsigned long)__iee_si_text_end, pte, "iee_si_text");
	check_addr_range_ro(image_addr, image_addr + size,
(unsigned long)__iee_si_data_start, (unsigned long)__iee_si_data_end, pte, "iee_si_data");
#endif
#ifdef CONFIG_IEE_SELINUX_P
	check_addr_range_ro(image_addr, image_addr + size,
(unsigned long)__iee_selinux_data_start, (unsigned long)__iee_selinux_data_end, pte, "iee_si_data");
#endif
}

static inline bool preserves_existing_writable_mapping(pte_t *ptep, pte_t pte)
{
	pte_t old_pte = READ_ONCE(*ptep);

	return pte_present(old_pte) && pte_write(old_pte) &&
		pte_pfn(old_pte) == pte_pfn(pte);
}

static inline bool
pmd_preserves_existing_writable_mapping(pmd_t *pmdp, pmd_t pmd)
{
	pmd_t old_pmd = READ_ONCE(*pmdp);

	return pmd_present(old_pmd) && pmd_leaf(old_pmd) &&
		pmd_write(old_pmd) && pmd_pfn(old_pmd) == pmd_pfn(pmd);
}

static inline bool
pud_preserves_existing_writable_mapping(pud_t *pudp, pud_t pud)
{
	pud_t old_pud = READ_ONCE(*pudp);

	return pud_present(old_pud) && pud_leaf(old_pud) &&
		pud_write(old_pud) && pud_pfn(old_pud) == pud_pfn(pud);
}

static inline void check_alias_mapping(phys_addr_t start, phys_addr_t end,
								bool writable,
								bool existing_writable_mapping)
{
	uint8_t type;
	phys_addr_t addr;

	if (unlikely(!haoc_init_done))
		return;

	if (unlikely(PHYS_PFN(end) >= max_pfn))
		return;

	addr = start;
	while (addr < end) {
		type = iee_get_bitmap_type(addr);
		if (unlikely(type != IEE_NORMAL && writable &&
			     !existing_writable_mapping))
			panic("Detected Alias mapping of IEE data");
		addr += PAGE_SIZE;
	}
}

static inline int pte_smode(pte_t pte)
{
	return !(pte_flags(pte) & _PAGE_USER);
}

static inline void check_sx(pte_t *ptep, pte_t pte)
{
	pte_t old_pte;
	pteval_t diff;

	if (unlikely(!haoc_init_done))
		return;
	old_pte = READ_ONCE(*ptep);
	if (unlikely(!pte_present(old_pte)))
		return;

	diff = pte_flags(old_pte) ^ pte_flags(pte);
	if (pte_exec(pte) && pte_smode(pte) &&
		((diff & _PAGE_USER) || (diff & _PAGE_NX))) {
		pr_err("PTP: old_pte=%lx, pte=%lx\n", pte_flags(old_pte), pte_flags(pte));
		pr_err("PTP: diff=%lx\n", diff);
		panic("Detected new S-page with execute permission");
	}
	if (pte_exec(pte) && pte_smode(pte) && pte_write(pte))
		panic("Detected SX-page with write permission");
}

static inline void check_pt_integrity(pte_t *ptep, pte_t pte)
{
	pte_t old_pte;

	if (unlikely(!haoc_init_done))
		return;
	if (iee_get_bitmap_type(__pa(ptep)) != IEE_FIXED_PGTABLE)
		return;
	old_pte = READ_ONCE(*ptep);
	if (pte_pfn(old_pte) != pte_pfn(pte))
		panic("PTP: Are you trying to break pt integrity? old_pte: 0x%lx, new_pte: 0x%lx",
				old_pte.pte, pte.pte);
}

void _iee_set_pte(unsigned long __unused, pte_t *ptep, pte_t pte)
{
	if (!(pte_val(pte) & _PAGE_PRESENT)) {
		WRITE_ONCE(*ptep, pte);
		return;
	}

	phys_addr_t phys_addr = __pte_to_phys(pte);
	unsigned long image_addr = phys_addr + __START_KERNEL_map - phys_base;

	check_dep_and_ro(image_addr, PAGE_SIZE, pte);
	check_alias_mapping(phys_addr, phys_addr + PAGE_SIZE, pte_write(pte),
			    preserves_existing_writable_mapping(ptep, pte));
	check_sx(ptep, pte);
	check_pt_integrity(ptep, pte);
	WRITE_ONCE(*ptep, pte);
}

void _iee_set_sx_pte(unsigned long __unused, pte_t *ptep, pte_t pte)
{
	if (!(pte_val(pte) & _PAGE_PRESENT)) {
		WRITE_ONCE(*ptep, pte);
		return;
	}

	phys_addr_t phys_addr = __pte_to_phys(pte);
	unsigned long image_addr = phys_addr + __START_KERNEL_map - phys_base;

	check_dep_and_ro(image_addr, PAGE_SIZE, pte);

	// FIXME: Writing the PTE during page split may create temporary alias mappings,
	// so we cannot perform the check here.
	// A different approach is needed in the future.
	// check_alias_mapping(phys_addr, phys_addr + PAGE_SIZE, ptep, pte);
	WRITE_ONCE(*ptep, pte);
}

void _iee_set_pmd(unsigned long __unused, pmd_t *pmdp, pmd_t pmd)
{
	phys_addr_t phys_addr;
	unsigned long image_addr;

	if (!(pmd_val(pmd) & _PAGE_PRESENT)) {
		WRITE_ONCE(*pmdp, pmd);
		return;
	}
	if (pmd_leaf(pmd)) {
		phys_addr = __pmd_to_phys(pmd);
		image_addr = phys_addr + __START_KERNEL_map - phys_base;
		check_dep_and_ro(image_addr, PMD_SIZE, __pte(pmd_val(pmd)));
		check_alias_mapping(phys_addr, phys_addr + PMD_SIZE,
				    pmd_write(pmd),
				    pmd_preserves_existing_writable_mapping(pmdp, pmd));
		check_sx((pte_t *)pmdp, __pte(pmd_val(pmd)));
	}

	WRITE_ONCE(*pmdp, pmd);
}

void _iee_set_pud(unsigned long __unused, pud_t *pudp, pud_t pud)
{
	phys_addr_t phys_addr;
	unsigned long image_addr;

	if (!(pud_val(pud) & _PAGE_PRESENT)) {
		WRITE_ONCE(*pudp, pud);
		return;
	}
	if (pud_leaf(pud)) {
		phys_addr = __pud_to_phys(pud);
		image_addr = phys_addr + __START_KERNEL_map - phys_base;
		check_dep_and_ro(image_addr, PUD_SIZE, __pte(pud_val(pud)));
		check_alias_mapping(phys_addr, phys_addr + PUD_SIZE,
				    pud_write(pud),
				    pud_preserves_existing_writable_mapping(pudp, pud));
		check_sx((pte_t *)pudp, __pte(pud_val(pud)));
	}

	WRITE_ONCE(*pudp, pud);
}

void _iee_set_p4d(unsigned long __unused, p4d_t *p4dp, p4d_t p4d)
{
	WRITE_ONCE(*p4dp, p4d);
}

void _iee_set_pgd(unsigned long __unused, pgd_t *pgdp, pgd_t pgd)
{
	WRITE_ONCE(*pgdp, pgd);
}

static inline void check_text_poke_ro(unsigned long image_addr, unsigned long size, pte_t pte)
{
	if (unlikely(!kernel_set_to_readonly))
		return;

	check_addr_range_ro(image_addr, image_addr + size,
(unsigned long)__start_rodata, (unsigned long)__end_rodata, pte, "rodata");
#ifdef CONFIG_IEE_SIP
	check_addr_range_ro(image_addr, image_addr + size,
(unsigned long)__iee_si_data_start, (unsigned long)__iee_si_data_end, pte, "iee_si_data");
#endif
#ifdef CONFIG_IEE_SELINUX_P
	check_addr_range_ro(image_addr, image_addr + size,
(unsigned long)__iee_selinux_data_start, (unsigned long)__iee_selinux_data_end, pte, "iee_si_data");
#endif
}

void _iee_set_pte_text_poke(unsigned long __unused, pte_t *ptep, pte_t pte)
{
	if (!(pte_val(pte) & _PAGE_PRESENT)) {
		WRITE_ONCE(*ptep, pte);
		return;
	}

	phys_addr_t phys_addr = __pte_to_phys(pte);
	unsigned long image_addr = phys_addr + __START_KERNEL_map - phys_base;

	check_text_poke_ro(image_addr, PAGE_SIZE, pte);
	check_alias_mapping(phys_addr, phys_addr + PAGE_SIZE, pte_write(pte),
			    preserves_existing_writable_mapping(ptep, pte));
	WRITE_ONCE(*ptep, pte);
}

void _iee_set_pmd_ident(unsigned long __unused, pmd_t *pmdp, pmd_t pmd)
{
	phys_addr_t phys_addr;
	unsigned long image_addr;

	if (!(pmd_val(pmd) & _PAGE_PRESENT)) {
		WRITE_ONCE(*pmdp, pmd);
		return;
	}
	if (pmd_leaf(pmd)) {
		phys_addr = __pmd_to_phys(pmd);
		image_addr = phys_addr + __START_KERNEL_map - phys_base;
		check_dep_and_ro(image_addr, PMD_SIZE, __pte(pmd_val(pmd)));
	}

	WRITE_ONCE(*pmdp, pmd);
}

void _iee_set_pud_ident(unsigned long __unused, pud_t *pudp, pud_t pud)
{
	phys_addr_t phys_addr;
	unsigned long image_addr;

	if (!(pud_val(pud) & _PAGE_PRESENT)) {
		WRITE_ONCE(*pudp, pud);
		return;
	}
	if (pud_leaf(pud)) {
		phys_addr = __pud_to_phys(pud);
		image_addr = phys_addr + __START_KERNEL_map - phys_base;
		check_dep_and_ro(image_addr, PUD_SIZE, __pte(pud_val(pud)));
	}

	WRITE_ONCE(*pudp, pud);
}

DEFINE_PER_CPU(struct iee_cr0, iee_cr0s);

void __init haoc_ptp_init(void)
{
	int cpu;
	struct iee_cr0 *iee_cr0;

	for_each_possible_cpu(cpu) {
		iee_cr0 = per_cpu_ptr(&iee_cr0s, cpu);
		iee_cr0->wp_disabled_cnt = 0;
	}

	ptp_mark_all_pgtable_ro();
}

void ptp_disable_wp(unsigned long *cr0)
{
	unsigned long irq_flags;
	int cpu;
	struct iee_cr0 *iee_cr0;

	if (unlikely(!haoc_init_done))
		return;

	local_irq_save(irq_flags);
	cpu = get_cpu();
	iee_cr0 = per_cpu_ptr(&iee_cr0s, cpu);
	if (iee_cr0->wp_disabled_cnt == 0) {
		*cr0 = read_cr0();
		asm volatile("mov %0, %%cr0" : : "r"(*cr0 & ~X86_CR0_WP) : "memory");
	}
	iee_cr0->wp_disabled_cnt++;
	put_cpu();
	local_irq_restore(irq_flags);
}

void ptp_restore_wp(unsigned long cr0)
{
	unsigned long irq_flags;
	int cpu;
	struct iee_cr0 *iee_cr0;

	if (unlikely(!haoc_init_done))
		return;

	local_irq_save(irq_flags);
	cpu = get_cpu();
	iee_cr0 = per_cpu_ptr(&iee_cr0s, cpu);
	iee_cr0->wp_disabled_cnt--;
	if (iee_cr0->wp_disabled_cnt == 0)
		asm volatile("mov %0, %%cr0" : : "r"(cr0 | X86_CR0_WP) : "memory");
	put_cpu();
	local_irq_restore(irq_flags);
}

void ptp_context_enable_wp(int *wp_disabled_cnt, unsigned long *cr0)
{
	unsigned long irq_flags;
	int cpu;
	struct iee_cr0 *iee_cr0;

	if (unlikely(!haoc_init_done))
		return;

	local_irq_save(irq_flags);
	cpu = get_cpu();
	iee_cr0 = per_cpu_ptr(&iee_cr0s, cpu);
	*wp_disabled_cnt = iee_cr0->wp_disabled_cnt;
	if (*wp_disabled_cnt > 0) {
		*cr0 = read_cr0();
		iee_cr0->wp_disabled_cnt = 0;
		asm volatile("mov %0, %%cr0" : : "r"(*cr0 | X86_CR0_WP) : "memory");
	}
	put_cpu();
	local_irq_restore(irq_flags);
}

void ptp_context_restore_wp(int wp_disabled_cnt, unsigned long cr0)
{
	unsigned long irq_flags;
	int cpu;
	struct iee_cr0 *iee_cr0;

	if (unlikely(!haoc_init_done))
		return;

	local_irq_save(irq_flags);
	if (wp_disabled_cnt > 0) {
		cpu = get_cpu();
		iee_cr0 = per_cpu_ptr(&iee_cr0s, cpu);
		asm volatile("mov %0, %%cr0" : : "r"(cr0 & ~X86_CR0_WP) : "memory");
		iee_cr0->wp_disabled_cnt = wp_disabled_cnt;
		put_cpu();
	}
	local_irq_restore(irq_flags);
}

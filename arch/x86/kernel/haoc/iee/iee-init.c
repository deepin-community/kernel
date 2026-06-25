// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#include <linux/gfp.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/types.h>
#include <asm/cpu.h>
#include <asm/haoc/haoc-bitmap.h>
#include <asm/haoc/iee-func.h>
#include <asm/haoc/iee.h>
#include <asm/pgalloc.h>
#include <asm/processor-flags.h>
#include <asm/set_memory.h>
#ifdef CONFIG_IEE_SIP
#include <asm/haoc/iee-si.h>
#endif
#ifdef CONFIG_CREDP
extern void credp_init(void);
#endif

/* IEE_OFFSET = pgtable_l5_enabled() ? 0x40000000000000 : 0x200000000000; */
unsigned long IEE_OFFSET = 0x200000000000;

#ifdef CONFIG_IEE_SIP
bool iee_init_done __iee_si_data;
#else
bool iee_init_done;
#endif
DEFINE_PER_CPU(struct iee_stack, iee_stacks);

bool __ro_after_init haoc_enabled;
EXPORT_SYMBOL(haoc_enabled);

bool __ro_after_init haoc_init_done;

/*
 * Deepin keeps HAOC disabled by default. The boot parameter enables the
 * three-stage IEE initialization when set to a true value, for example haoc=on.
 */
static bool cmd_haoc_enabled;
static int __init parse_cmd_haoc_enabled(char *str)
{
	return kstrtobool(str, &cmd_haoc_enabled);
}
early_param("haoc", parse_cmd_haoc_enabled);

static void __init _iee_mapping_populate_pud(pud_t *pud, unsigned long addr,
					     unsigned long end)
{
	void *p;
	pmd_t *pmd;
	unsigned long pmd_next;
	phys_addr_t phys;
	pgprot_t pgprot_shadow_pmd;

	addr = ALIGN_DOWN(addr, PMD_SIZE);
	phys = __iee_pa(addr);
	pgprot_shadow_pmd = __pgprot(pgprot_val(PAGE_KERNEL_LARGE) & ~__RW & ~___D);

	if (pud_none(*pud)) {
		p = ptdesc_to_virt(pagetable_alloc(GFP_KERNEL | __GFP_ZERO, 0));
		pud_populate(&init_mm, pud, p);
	}

	pmd = pmd_offset(pud, addr);
	do {
		pmd_next = pmd_addr_end(addr, end);
		set_pmd(pmd, __pmd(phys | pgprot_val(pgprot_shadow_pmd)));
		phys += pmd_next - addr;
	} while (pmd++, addr = pmd_next, addr != end);
}

static void __init _iee_mapping_populate_p4d(p4d_t *p4d, unsigned long addr,
					     unsigned long end)
{
	void *p;
	pud_t *pud;
	unsigned long pud_next;

	if (p4d_none(*p4d)) {
		p = ptdesc_to_virt(pagetable_alloc(GFP_KERNEL | __GFP_ZERO, 0));
		p4d_populate(&init_mm, p4d, p);
	}

	pud = pud_offset(p4d, addr);
	do {
		pud_next = pud_addr_end(addr, end);
		pr_info("IEE: iee_populate_pud(%#010lx, %#010lx)\n",
			addr, pud_next);
		_iee_mapping_populate_pud(pud, addr, pud_next);
	} while (pud++, addr = pud_next, addr != end);
}

static void __init _iee_mapping_populate_pgd(pgd_t *pgd, unsigned long addr,
					     unsigned long end)
{
	void *p;
	p4d_t *p4d;
	unsigned long p4d_next;

	if (pgd_none(*pgd)) {
		p = ptdesc_to_virt(pagetable_alloc(GFP_KERNEL | __GFP_ZERO, 0));
		pgd_populate(&init_mm, pgd, p);
	}

	p4d = p4d_offset(pgd, addr);
	do {
		p4d_next = p4d_addr_end(addr, end);
		pr_info("IEE: iee_populate_p4d(%#010lx, %#010lx)\n",
			addr, p4d_next);
		_iee_mapping_populate_p4d(p4d, addr, p4d_next);
	} while (p4d++, addr = p4d_next, addr != end);
}

static void __init _iee_init_mapping(phys_addr_t start_paddr, phys_addr_t end_paddr)
{
	unsigned long addr = (unsigned long)__phys_to_iee(start_paddr);
	unsigned long end = (unsigned long)__phys_to_iee(end_paddr);
	unsigned long pgd_next;
	pgd_t *pgd = pgd_offset_k(addr);

	spin_lock(&pgd_lock);
	do {
		pgd_next = pgd_addr_end(addr, end);
		pr_info("IEE: iee_populate_pgd(%#010lx, %#010lx)\n",
			addr, pgd_next);
		_iee_mapping_populate_pgd(pgd, addr, pgd_next);
	} while (pgd++, addr = pgd_next, addr != end);
	spin_unlock(&pgd_lock);
}

static void __init iee_init_offset(void)
{
	if (pgtable_l5_enabled())
		IEE_OFFSET = 0x40000000000000;
}

static void __init iee_init_mapping(void)
{
	struct memblock_region *r;
	unsigned long start_pfn, end_pfn;
	phys_addr_t start_paddr, end_paddr;

	for_each_mem_region(r) {
		start_pfn = memblock_region_memory_base_pfn(r);
		end_pfn = memblock_region_memory_end_pfn(r);

		start_paddr = PFN_PHYS(start_pfn);
		end_paddr = PFN_PHYS(end_pfn);

		pr_info("IEE: mapping iee mapping [mem %#010lx-%#010lx]\n",
			(unsigned long)start_paddr, (unsigned long)end_paddr);

		_iee_init_mapping(start_paddr, end_paddr);
	}
	pr_info("IEE: IEE shadow mapping init done");
}

static void __init iee_init_stack(void)
{
	int cpu;
	struct iee_stack *iee_stack;
	unsigned long stack_base;
	struct page *page;

	for_each_possible_cpu(cpu) {
		iee_stack = per_cpu_ptr(&iee_stacks, cpu);
		page = alloc_pages(GFP_KERNEL, IEE_STACK_ORDER);
		if (!page)
			panic("IEE: no memory for iee stack");

		stack_base = (unsigned long)page_address(page);
		iee_stack->stack = (void *)stack_base + PAGE_SIZE * (1 << IEE_STACK_ORDER);
		pr_info("IEE: cpu %d, iee_stack 0x%lx", cpu,
			(unsigned long)iee_stack->stack);
	}
}

static void __init iee_stack_set_ro(void)
{
	int cpu;
	struct iee_stack *iee_stack;
	unsigned long stack_base;

	for_each_possible_cpu(cpu) {
		iee_stack = per_cpu_ptr(&iee_stacks, cpu);
		stack_base = (unsigned long)iee_stack->stack - PAGE_SIZE * (1 << IEE_STACK_ORDER);
		set_iee_pages(stack_base, 1 << IEE_STACK_ORDER, IEE_SIP_DATA);
	}
}

static bool __init check_haoc_hardware_support(void)
{
	return cpu_feature_enabled(X86_FEATURE_SMAP) &&
	       cpu_feature_enabled(X86_FEATURE_SMEP);
}

#ifdef CONFIG_IEE_SIP
static void __init iee_setup_sip_cr4_pinning(void)
{
	cr4_pinned_mask &= ~(X86_CR4_SMEP | X86_CR4_SMAP);
}
#endif

void __init iee_early_init(void)
{
	if (!cmd_haoc_enabled)
		return;

	if (!check_haoc_hardware_support())
		pr_info("HAOC disabled because SMAP or SMEP is not available");
}

void __init iee_init(void)
{
	if (!cmd_haoc_enabled)
		return;

	if (!check_haoc_hardware_support())
		return;

	iee_init_offset();
	iee_init_mapping();
	iee_init_bitmap();
	iee_init_stack();

	/*
	 * HAOC temporarily clears CR0.WP in the write gate. Disable CET while
	 * HAOC is active to avoid the CR4.CET and CR0.WP architectural conflict.
	 */
	cet_disable();

	haoc_enabled = true;
#ifdef CONFIG_IEE_SIP
	iee_setup_sip_cr4_pinning();
#endif
	pr_info("HAOC initialized");
}

void __init iee_post_init(void)
{
	if (!cmd_haoc_enabled || !haoc_enabled)
		return;

	iee_stack_set_ro();
#ifdef CONFIG_IEE_SIP
	iee_sip_init();
#endif
#ifdef CONFIG_CREDP
	credp_init();
#endif
	haoc_init_done = true;
}

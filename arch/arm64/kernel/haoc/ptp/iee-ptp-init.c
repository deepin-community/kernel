// SPDX-License-Identifier: GPL-2.0
#include <linux/mm.h>
#include <linux/efi.h>
#include <linux/io.h>
#include <asm/fixmap.h>
#include <asm/pgtable.h>
#include <asm/tlbflush.h>
#include <asm/haoc/iee-init.h>
#include <asm/haoc/iee-ptp-init.h>

#define PTP_CHECK(condition) do {	\
	if (unlikely(condition))	\
		panic("PTP check failed on %s.", __func__);	\
} while (0)

#if CONFIG_PGTABLE_LEVELS > 3
void __iee_p4d_populate_pre_init(p4d_t *p4dp, phys_addr_t pudp, p4dval_t prot)
{
	iee_set_p4d_pre_init(p4dp, __p4d(__phys_to_p4d_val(pudp) | prot));
}
#else
void __iee_p4d_populate_pre_init(p4d_t *p4dp, phys_addr_t pudp, p4dval_t prot)
{
	BUILD_BUG();
}
#endif

void __iee_pud_populate_pre_init(pud_t *pudp, phys_addr_t pmdp, pudval_t prot)
{
	iee_set_pud_pre_init(pudp, __pud(__phys_to_pud_val(pmdp) | prot));
}

void __iee_pmd_populate_pre_init(pmd_t *pmdp, phys_addr_t ptep,
				  pmdval_t prot)
{
	iee_set_pmd_pre_init(pmdp, __pmd(__phys_to_pmd_val(ptep) | prot));
}

int iee_pud_set_huge_pre_init(pud_t *pudp, phys_addr_t phys, pgprot_t prot)
{
	pud_t new_pud = pfn_pud(__phys_to_pfn(phys), mk_pud_sect_prot(prot));

	/* Only allow permission changes for now */
	if (!pgattr_change_is_safe(READ_ONCE(pud_val(*pudp)),
				   pud_val(new_pud)))
		return 0;

	WARN_ON_ONCE(phys & ~PUD_MASK);
	iee_set_pud_pre_init(pudp, new_pud);
	return 1;
}

int iee_pmd_set_huge_pre_init(pmd_t *pmdp, phys_addr_t phys, pgprot_t prot)
{
	pmd_t new_pmd = pfn_pmd(__phys_to_pfn(phys), mk_pmd_sect_prot(prot));

	/* Only allow permission changes for now */
	if (!pgattr_change_is_safe(READ_ONCE(pmd_val(*pmdp)),
				   pmd_val(new_pmd)))
		return 0;

	WARN_ON_ONCE(phys & ~PMD_MASK);
	iee_set_pmd_pre_init(pmdp, new_pmd);
	return 1;
}

static inline pte_t *fixmap_pte(unsigned long addr)
{
	return &bm_pte[BM_PTE_TABLE_IDX(addr)][pte_index(addr)];
}

void __iee_set_fixmap_pre_init(enum fixed_addresses idx,
			       phys_addr_t phys, pgprot_t flags)
{
	unsigned long addr = __fix_to_virt(idx);
	pte_t *ptep;

	PTP_CHECK(idx <= FIX_HOLE || idx >= __end_of_fixed_addresses);

	ptep = fixmap_pte(addr);

	if (pgprot_val(flags)) {
		iee_set_pte_pre_init(ptep, pfn_pte(phys >> PAGE_SHIFT, flags));
	} else {
		iee_set_pte_pre_init(ptep, __pte(0));
		flush_tlb_kernel_range(addr, addr+PAGE_SIZE);
	}
}

void iee_set_pgtable_pre_init(unsigned long *addr, unsigned long content)
{
	WRITE_ONCE(*addr, content);
}

void set_iee_address_pre_init(unsigned long addr, bool valid)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);

	p4d_t *p4dp = p4d_offset(pgdp, addr);

	pud_t *pudp = pud_offset(p4dp, addr);

	pmd_t *pmdp = pmd_offset(pudp, addr);

	pte_t *ptep = pte_offset_kernel(pmdp, addr);
	pte_t pte = READ_ONCE(*ptep);

	if (valid)
		pte = __pte(pte_val(pte) | PTE_VALID);
	else
		pte = __pte(pte_val(pte) & ~PTE_VALID);
	iee_set_pgtable_pre_init((unsigned long *)ptep, (unsigned long)pte.pte);
	dsb(ishst);
	isb();
}

static void iee_init_pte_pre_init(pmd_t *pmdp, unsigned long addr, unsigned long end,
		     phys_addr_t phys, pgprot_t prot)
{
	pte_t *ptep;

	ptep = pte_set_fixmap_offset_pre_init(pmdp, addr);
	do {
		pte_t old_pte = __ptep_get(ptep);

		iee_set_pgtable_pre_init((unsigned long *)ptep,
				(unsigned long)(pfn_pte(__phys_to_pfn(phys), prot).pte));

		/*
		 * After the PTE entry has been populated once, we
		 * only allow updates to the permission attributes.
		 */
		PTP_CHECK(!pgattr_change_is_safe(pte_val(old_pte),
					      pte_val(__ptep_get(ptep))));

		phys += PAGE_SIZE;
	} while (ptep++, addr += PAGE_SIZE, addr != end);

	pte_clear_fixmap_pre_init();
}

static void iee_alloc_init_cont_pte_pre_init(pmd_t *pmdp, unsigned long addr,
				unsigned long end, phys_addr_t phys,
				pgprot_t prot,
				phys_addr_t (*pgtable_alloc)(int),
				int flags)
{
	unsigned long next;
	pmd_t pmd = READ_ONCE(*pmdp);

	PTP_CHECK(pmd_sect(pmd));
	if (pmd_none(pmd)) {
		pmdval_t pmdval = PMD_TYPE_TABLE | PMD_TABLE_UXN | PMD_TABLE_AF;
		phys_addr_t pte_phys;

		if (flags & NO_EXEC_MAPPINGS)
			pmdval |= PMD_TABLE_PXN;
		PTP_CHECK(!pgtable_alloc);
		pte_phys = pgtable_alloc(PAGE_SHIFT);
		iee_set_pgtable_pre_init((unsigned long *)pmdp,
					(unsigned long)(__phys_to_pmd_val(pte_phys) | pmdval));
		pmd = READ_ONCE(*pmdp);
	}
	PTP_CHECK(pmd_bad(pmd));

	do {
		pgprot_t __prot = prot;

		next = pte_cont_addr_end(addr, end);

		/* use a contiguous mapping if the range is suitably aligned */
		if ((((addr | next | phys) & ~CONT_PTE_MASK) == 0) &&
		    (flags & NO_CONT_MAPPINGS) == 0)
			__prot = __pgprot(pgprot_val(prot) | PTE_CONT);

		iee_init_pte_pre_init(pmdp, addr, next, phys, __prot);

		phys += next - addr;
	} while (addr = next, addr != end);
}

static void iee_init_pmd_pre_init(pud_t *pudp, unsigned long addr, unsigned long end,
		     phys_addr_t phys, pgprot_t prot,
		     phys_addr_t (*pgtable_alloc)(int), int flags)
{
	unsigned long next;
	pmd_t *pmdp;

	pmdp = pmd_set_fixmap_offset_pre_init(pudp, addr);
	do {
		pmd_t old_pmd = READ_ONCE(*pmdp);

		next = pmd_addr_end(addr, end);

		/* try section mapping first */
		if (((addr | next | phys) & ~PMD_MASK) == 0 &&
		    (flags & NO_BLOCK_MAPPINGS) == 0) {
			iee_set_pgtable_pre_init((unsigned long *)pmdp,
				(unsigned long)(pfn_pmd(__phys_to_pfn(phys),
				mk_pmd_sect_prot(prot)).pmd));

			/*
			 * After the PMD entry has been populated once, we
			 * only allow updates to the permission attributes.
			 */
			PTP_CHECK(!pgattr_change_is_safe(pmd_val(old_pmd),
						      READ_ONCE(pmd_val(*pmdp))));
		} else {
			iee_alloc_init_cont_pte_pre_init(pmdp, addr, next, phys, prot,
					    pgtable_alloc, flags);

			PTP_CHECK(pmd_val(old_pmd) != 0 &&
			       pmd_val(old_pmd) != READ_ONCE(pmd_val(*pmdp)));
		}
		phys += next - addr;
	} while (pmdp++, addr = next, addr != end);

	pmd_clear_fixmap_pre_init();
}

static void iee_alloc_init_cont_pmd_pre_init(pud_t *pudp, unsigned long addr,
				unsigned long end, phys_addr_t phys,
				pgprot_t prot,
				phys_addr_t (*pgtable_alloc)(int), int flags)
{
	unsigned long next;
	pud_t pud = READ_ONCE(*pudp);

	/*
	 * Check for initial section mappings in the pgd/pud.
	 */
	PTP_CHECK(pud_sect(pud));
	if (pud_none(pud)) {
		pudval_t pudval = PUD_TYPE_TABLE | PUD_TABLE_UXN | PUD_TABLE_AF;
		phys_addr_t pmd_phys;

		if (flags & NO_EXEC_MAPPINGS)
			pudval |= PUD_TABLE_PXN;
		PTP_CHECK(!pgtable_alloc);
		pmd_phys = pgtable_alloc(PMD_SHIFT);
		iee_set_pgtable_pre_init((unsigned long *)pudp,
					(unsigned long)(__phys_to_pud_val(pmd_phys) | pudval));
		pud = READ_ONCE(*pudp);
	}
	PTP_CHECK(pud_bad(pud));

	do {
		pgprot_t __prot = prot;

		next = pmd_cont_addr_end(addr, end);

		/* use a contiguous mapping if the range is suitably aligned */
		if ((((addr | next | phys) & ~CONT_PMD_MASK) == 0) &&
		    (flags & NO_CONT_MAPPINGS) == 0)
			__prot = __pgprot(pgprot_val(prot) | PTE_CONT);

		iee_init_pmd_pre_init(pudp, addr, next, phys, __prot, pgtable_alloc, flags);

		phys += next - addr;
	} while (addr = next, addr != end);
}

void iee_alloc_init_pud_pre_init(pgd_t *pgdp, unsigned long addr, unsigned long end,
			   phys_addr_t phys, pgprot_t prot,
			   phys_addr_t (*pgtable_alloc)(int),
			   int flags)
{
	unsigned long next;
	pud_t *pudp;
	p4d_t *p4dp = p4d_offset(pgdp, addr);
	p4d_t p4d = READ_ONCE(*p4dp);

	if (p4d_none(p4d)) {
		p4dval_t p4dval = P4D_TYPE_TABLE | P4D_TABLE_UXN | P4D_TABLE_AF;
		phys_addr_t pud_phys;

		if (flags & NO_EXEC_MAPPINGS)
			p4dval |= P4D_TABLE_PXN;
		PTP_CHECK(!pgtable_alloc);
		pud_phys = pgtable_alloc(PUD_SHIFT);
		iee_set_pgtable_pre_init((unsigned long *)p4dp,
					(unsigned long)(__phys_to_p4d_val(pud_phys) | p4dval));
		p4d = READ_ONCE(*p4dp);
	}
	PTP_CHECK(p4d_bad(p4d));

	pudp = pud_set_fixmap_offset_pre_init(p4dp, addr);
	do {
		pud_t old_pud = READ_ONCE(*pudp);

		next = pud_addr_end(addr, end);

		iee_alloc_init_cont_pmd_pre_init(pudp, addr, next, phys, prot,
					    pgtable_alloc, flags);

		PTP_CHECK(pud_val(old_pud) != 0 &&
			       pud_val(old_pud) != READ_ONCE(pud_val(*pudp)));
		phys += next - addr;
	} while (pudp++, addr = next, addr != end);

	pud_clear_fixmap_pre_init();
}

static void __create_pgd_mapping_locked_pre_init(pgd_t *pgdir, phys_addr_t phys,
					unsigned long virt, phys_addr_t size,
					pgprot_t prot,
					phys_addr_t (*pgtable_alloc)(int),
					int flags)
{
	unsigned long addr, end, next;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, virt);

	/*
	 * If the virtual and physical address don't have the same offset
	 * within a page, we cannot map the region as the caller expects.
	 */
	if (WARN_ON((phys ^ virt) & ~PAGE_MASK))
		return;

	phys &= PAGE_MASK;
	addr = virt & PAGE_MASK;
	end = PAGE_ALIGN(virt + size);

	do {
		next = pgd_addr_end(addr, end);
		iee_alloc_init_pud_pre_init(pgdp, addr, next, phys, prot, pgtable_alloc,
			       flags);
		phys += next - addr;
	} while (pgdp++, addr = next, addr != end);
}

void __create_pgd_mapping_pre_init(pgd_t *pgdir, phys_addr_t phys,
				 unsigned long virt, phys_addr_t size,
				 pgprot_t prot,
				 phys_addr_t (*pgtable_alloc)(int),
				 int flags)
{
	mutex_lock(&fixmap_lock);
	__create_pgd_mapping_locked_pre_init(pgdir, phys, virt, size, prot,
				    pgtable_alloc, flags);
	mutex_unlock(&fixmap_lock);
}

static void __init early_iounmap_after_init(void __iomem *addr, unsigned long size)
{
	unsigned long virt_addr;
	unsigned long offset;
	unsigned int nrpages;
	enum fixed_addresses idx;
	int i, slot;

	slot = -1;
	for (i = 0; i < FIX_BTMAPS_SLOTS; i++) {
		if (prev_map[i] == addr) {
			slot = i;
			break;
		}
	}

	if (WARN(slot < 0, "early_iounmap(%p, %08lx) not found slot\n",
		 addr, size))
		return;

	if (WARN(prev_size[slot] != size,
		 "early_iounmap(%p, %08lx) [%d] size not consistent %08lx\n",
		 addr, size, slot, prev_size[slot]))
		return;

	WARN(early_ioremap_debug, "early_iounmap(%p, %08lx) [%d]\n",
	     addr, size, slot);

	virt_addr = (unsigned long)addr;
	if (WARN_ON(virt_addr < fix_to_virt(FIX_BTMAP_BEGIN)))
		return;

	offset = offset_in_page(virt_addr);
	nrpages = PAGE_ALIGN(offset + size) >> PAGE_SHIFT;

	idx = FIX_BTMAP_BEGIN - NR_FIX_BTMAPS*slot;
	while (nrpages > 0) {
		if (after_paging_init)
			__late_clear_fixmap(idx);
		else
			__early_set_fixmap(idx, 0, FIXMAP_PAGE_CLEAR);
		--idx;
		--nrpages;
	}
	prev_map[slot] = NULL;
}

void __init efi_memmap_unmap_after_init(void)
{
	if (!efi_enabled(EFI_MEMMAP))
		return;

	if (!(efi.memmap.flags & EFI_MEMMAP_LATE)) {
		unsigned long size;

		size = efi.memmap.desc_size * efi.memmap.nr_map;
		early_iounmap_after_init((__force void __iomem *)efi.memmap.map, size);
	} else {
		memunmap(efi.memmap.map);
	}

	efi.memmap.map = NULL;
	clear_bit(EFI_MEMMAP, &efi.flags);
}

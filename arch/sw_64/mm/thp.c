// SPDX-License-Identifier: GPL-2.0
#include <asm/tlbflush.h>
#include <asm/pgalloc.h>

int pmdp_set_access_flags(struct vm_area_struct *vma,
			  unsigned long address, pmd_t *pmdp,
			  pmd_t entry, int dirty)
{
	int changed = !pmd_same(*pmdp, entry);

	VM_BUG_ON(address & ~HPAGE_PMD_MASK);

	if (changed && dirty) {
		*pmdp = entry;
		flush_tlb_range(vma, address, address + HPAGE_PMD_SIZE);
	}

	return changed;
}
int pmdp_test_and_clear_young(struct vm_area_struct *vma,
			      unsigned long addr, pmd_t *pmdp)
{
	int ret = 0;

	if (pmd_young(*pmdp))
		ret = test_and_clear_bit(_PAGE_BIT_ACCESSED,
				(unsigned long *)pmdp);
	return ret;
}

int pmdp_clear_flush_young(struct vm_area_struct *vma,
			   unsigned long address, pmd_t *pmdp)
{
	int young;

	VM_BUG_ON(address & ~HPAGE_PMD_MASK);

	young = pmdp_test_and_clear_young(vma, address, pmdp);
	if (young)
		flush_tlb_range(vma, address, address + HPAGE_PMD_SIZE);

	return young;
}
void pmdp_splitting_flush(struct vm_area_struct *vma,
			  unsigned long address, pmd_t *pmdp)
{
	int set;

	VM_BUG_ON(address & ~HPAGE_PMD_MASK);
	set = !test_and_set_bit(_PAGE_BIT_SPLITTING, (unsigned long *)pmdp);
	if (set) {
		/* need tlb flush only to serialize against gup-fast */
		flush_tlb_range(vma, address, address + HPAGE_PMD_SIZE);
	}
}

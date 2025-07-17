/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_MMU_H
#define _ASM_SW64_MMU_H

/* The sw64 MMU context is one "unsigned long" bitmap per CPU*/
typedef struct {
	unsigned long asid[NR_CPUS];
	void *vdso;
} mm_context_t;

#ifdef CONFIG_SW64_KERNEL_PAGE_TABLE
void create_pgd_mapping(pgd_t *pgdir, unsigned long virt, unsigned long phys,
			unsigned long size, pgprot_t prot,
			void *(*pgtable_alloc)(void));
#endif

#endif /* _ASM_SW64_MMU_H */

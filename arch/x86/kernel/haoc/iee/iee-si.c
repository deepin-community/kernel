// SPDX-License-Identifier: GPL-2.0
#include <linux/cache.h>
#include <linux/init.h>

#include <asm/haoc/iee-si.h>
#include <asm/haoc/haoc-def.h>
#include <asm/set_memory.h>
#include <asm/haoc/iee.h>

/*
 * SMEP/SMAP bits the boot CPU actually enabled. The rwx gate exit
 * re-enables these after running the handler; it must never set bits
 * the CPU does not support (writing them to CR4 would #GP).
 * Initialized in identify_cpu() before setup_smep()/setup_smap().
 */
unsigned long iee_cr4_set_mask __read_mostly;

unsigned long __iee_si_code notrace _iee_si_handler(int flag, ...)
{
	va_list pArgs;
	unsigned long val;

	va_start(pArgs, flag);
	switch (flag) {
		case IEE_SIP_TEST:
			break;
		case IEE_WRITE_CR0: {
			val = va_arg(pArgs, u64);
			unsigned long bits_missing = 0;
		set_register_cr0:
			asm volatile("mov %0,%%cr0" : "+r"(val) : : "memory");
			if (static_branch_likely(&cr_pinning)) {
				if (unlikely((val & X86_CR0_WP) != X86_CR0_WP)) {
					bits_missing = X86_CR0_WP;
					val |= bits_missing;
					goto set_register_cr0;
				}
				/* Warn after we've set the missing bits. */
				WARN_ONCE(bits_missing, "CR0 WP bit went missing!?\n");
			}
			break;
		}
		case IEE_WRITE_CR3: {
			val = va_arg(pArgs, u64);
			asm volatile("mov %0,%%cr3" : : "r"(val) : "memory");
			break;
		}
		case IEE_WRITE_CR4: {
			unsigned long bits_changed = 0;
			/*
			 * The handler runs from user-mapped .iee.si_text with SMEP
			 * already cleared by the gate: writing CR4 with SMEP=1
			 * here would fault on the next instruction fetch. SMEP is
			 * restored by the gate exit per iee_cr4_set_mask, so it
			 * takes no part in the pinning enforcement below.
			 */
			const unsigned long check_mask = cr4_pinned_mask & ~X86_CR4_SMEP;
			const unsigned long check_bits = cr4_pinned_bits & ~X86_CR4_SMEP;

			val = va_arg(pArgs, u64);
			val &= ~X86_CR4_SMEP;
			if (static_branch_likely(&cr_pinning)) {
				if (unlikely((val & check_mask) != check_bits)) {
					bits_changed = (val & check_mask) ^ check_bits;
					val = (val & ~check_mask) | check_bits;
				}
			}
			asm volatile("mov %0,%%cr4" : "+r" (val) : : "memory");
			/* Warn after we've corrected the changed bits. */
			if (static_branch_likely(&cr_pinning))
				WARN_ONCE(bits_changed, "pinned CR4 bits changed: 0x%lx!?\n",
					  bits_changed);
			break;
		}
		case IEE_LOAD_IDT: {
			const struct desc_ptr *new_val;

			new_val = va_arg(pArgs, const struct desc_ptr*);
			asm volatile("lidt %0"::"m" (*new_val));
			break;
		}
	}
	va_end(pArgs);
	return 0;
}

static void __init _iee_set_kernel_upage(unsigned long addr)
{
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
	pgd_t pgd = READ_ONCE(*pgdp);

	pgd = __pgd((pgd_val(pgd) | _USR) & ~___G);
	set_pgd(pgdp, pgd);

	p4d_t *p4dp = p4d_offset(pgdp, addr);
	p4d_t p4d = READ_ONCE(*p4dp);

	p4d = __p4d((p4d_val(p4d) | _USR) & ~___G);
	set_p4d(p4dp, p4d);

	pud_t *pudp = pud_offset(p4dp, addr);
	pud_t pud = READ_ONCE(*pudp);

	pud = __pud((pud_val(pud) | _USR) & ~___G);
	set_pud(pudp, pud);

	pmd_t *pmdp = pmd_offset(pudp, addr);
	pmd_t pmd = READ_ONCE(*pmdp);

	pmd = __pmd((pmd_val(pmd) | _USR) & ~___G);
	set_pmd(pmdp, pmd);

	pte_t *ptep = pte_offset_kernel(pmdp, addr);
	pte_t pte = READ_ONCE(*ptep);

	pte = __pte((pte_val(pte) | _USR) & ~___G);
	set_pte(ptep, pte);
	flush_tlb_kernel_range(addr, addr + PAGE_SIZE);
}

void __init iee_sip_init(void)
{
	unsigned long addr, start, end;
	int num_pages;
	/* Map .iee.text as U RWX pages */
	start = (unsigned long)__iee_si_text_start;
	end = (unsigned long)__iee_si_text_end;
	pr_info("IEE: mapping .iee.text:[0x%lx, 0x%lx] as U pages...", start, end);
	addr = start;
	for ( ; addr < end; addr += PAGE_SIZE) {
		set_memory_4k(addr, 1);
		_iee_set_kernel_upage(addr);
		set_memory_4k((unsigned long)__va(__pa(addr)), 1);
		_iee_set_kernel_upage((unsigned long)__va(__pa(addr)));
	}
	iee_init_done = true;
	/* Map .iee.data as RO pages */
	start = (unsigned long)__iee_si_data_start;
	end = (unsigned long)__iee_si_data_end;
	num_pages = (end - start) / PAGE_SIZE;
	set_memory_ro(start, num_pages);
	/* All initialization is done. Do some simple tests. */
	pr_info("IEE: testing iee_exec_entry si_test...");
	iee_sip_test();
}
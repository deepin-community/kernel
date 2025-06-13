// SPDX-License-Identifier: GPL-2.0
#include <linux/stdarg.h>
#include <asm/sysreg.h>
#include <asm/haoc/iee.h>
#include <asm/haoc/iee-asm.h>
#include <asm/haoc/iee-si.h>
#ifdef CONFIG_IEE_PTRP
#include <asm/haoc/iee-token.h>
#endif

u64 __ro_after_init iee_si_reserved_pg_dir;

static inline unsigned long iee_si_mask(unsigned long mask,
					unsigned long new_val, unsigned long old_val)
{
	return (new_val & mask) | (old_val & ~mask);
}

#ifdef CONFIG_IEE_PTRP
static inline void iee_si_check_ttbr0(void)
{
	u64 old_ttbr0 = read_sysreg(ttbr0_el1);
	u64 old_phys = (old_ttbr0 & PAGE_MASK) & ~TTBR_ASID_MASK;
	struct task_token *token = (struct task_token *)__addr_to_iee(current);
	/* Phys in TTBR0 shall be the same with current->mm->pgd. */
	if (!(current == &init_task) && token->pgd
		&& old_phys != iee_si_reserved_pg_dir) {
		u64 token_phys = __pa(token->pgd);

		if (old_phys != token_phys)
			pr_err("IEE: Pgd set error. old ttbr0:%llx, token ttbr0:%llx",
				old_phys, token_phys);
	}
}
#else
static inline void iee_si_check_ttbr0(void) {}
#endif

unsigned long __iee_si_code iee_si_handler(int flag, ...)
{
	va_list pArgs;
	unsigned long old_val, new_val, ttbr1, ttbr0;

	va_start(pArgs, flag);
	switch (flag) {
	case IEE_SI_TEST:
		break;
	case IEE_SI_SET_SCTLR_EL1:
		old_val = read_sysreg(sctlr_el1);
		new_val = va_arg(pArgs, u64);
		new_val = iee_si_mask(IEE_SI_SCTLR_MASK, new_val, old_val);
		write_sysreg(new_val, sctlr_el1);
		break;
	case IEE_SI_SET_TTBR0:
		ttbr0 = va_arg(pArgs, u64) & ~TTBR_ASID_MASK;
		/* Skip checking before init IEE. */
		if (iee_init_done)
			iee_si_check_ttbr0();
		/* Load the reserved IEE ASID into TTBR0.*/
		ttbr0 |= FIELD_PREP(TTBR_ASID_MASK, IEE_ASID);
		write_sysreg(ttbr0, ttbr0_el1);
		break;
	case IEE_SI_CONTEXT_SWITCH:
		ttbr1 = va_arg(pArgs, u64);
		ttbr0 = va_arg(pArgs, u64) & ~TTBR_ASID_MASK;
		ttbr0 |= FIELD_PREP(TTBR_ASID_MASK, IEE_ASID);
		/* Skip checking before init IEE. */
		if (iee_init_done) {
			u64 new_asid = ttbr1 >> 48;
			/* ASID shall not be the one reserved for IEE. */
			if (new_asid == IEE_ASID)
				panic("IEE SI: Using reserved IEE ASID in TTRB0.");

			iee_si_check_ttbr0();
		}
		write_sysreg(ttbr1, ttbr1_el1);
		write_sysreg(ttbr0, ttbr0_el1);
		break;
	case IEE_SI_CONTEXT_SWITCH_PRE_INIT:
		ttbr1 = va_arg(pArgs, u64);
		ttbr0 = va_arg(pArgs, u64);

		if (iee_init_done)
			panic("IEE: Using legacy context switch after IEE init.");
		write_sysreg(ttbr1, ttbr1_el1);
		write_sysreg(ttbr0, ttbr0_el1);
		break;
	case IEE_SI_SET_VBAR:
		new_val = va_arg(pArgs, u64);
		write_sysreg(new_val, vbar_el1);
		break;
	case IEE_SI_SET_TCR_EL1:
		new_val = va_arg(pArgs, u64);
		old_val = read_sysreg(tcr_el1);
		new_val = iee_si_mask(IEE_SI_TCR_MASK, new_val, old_val);
		write_sysreg(new_val, tcr_el1);
		break;
	default:
		break;
	}
	va_end(pArgs);
	return 0;
}

static inline void iee_si_setup_data(void)
{
	iee_si_reserved_pg_dir = phys_to_ttbr(__pa_symbol(reserved_pg_dir));
}

/* Map iee si code PXNTable=1 on pmd page tables. */
static int __init iee_si_init_code(void)
{
	u64 addr = (u64)__iee_si_text_start;
	pgd_t *pgdir = swapper_pg_dir;
	pgd_t *pgdp = pgd_offset_pgd(pgdir, addr);
	p4d_t *p4dp = p4d_offset(pgdp, addr);
	pud_t *pudp = pud_offset(p4dp, addr);
	pmd_t *pmdp = pmd_offset(pudp, addr);
	pmd_t pmd = READ_ONCE(*pmdp);

	if ((pmd_val(pmd) & PMD_TYPE_MASK) == PMD_TYPE_SECT) {
		pr_err("IEE SI: addr 0x%llx is pmd block. SI init failed.", addr);
		return 0;
	}

	pmd = __pmd(pmd_val(pmd) | PGD_PXNTABLE);
	set_pmd(pmdp, pmd);
	flush_tlb_all();
	return 1;
}

void __init iee_si_init(void)
{
	iee_si_setup_data();
	
	if (iee_si_init_code())
		pr_info("IEE: Sensitive instruction protection is ready.");
}

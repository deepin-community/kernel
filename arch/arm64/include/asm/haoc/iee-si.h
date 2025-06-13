/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_SI_H
#define _LINUX_IEE_SI_H

#include <asm/sysreg.h>

#define __iee_si_code		__section(".iee.si_text")

enum {
	IEE_SI_TEST,
	IEE_SI_SET_SCTLR_EL1,
	IEE_SI_SET_TCR_EL1,
	IEE_SI_SET_VBAR,
	IEE_SI_SET_TTBR0,
	IEE_SI_CONTEXT_SWITCH,
	IEE_SI_CONTEXT_SWITCH_PRE_INIT,
	IEE_SI_FLAGS
};

/* Handler function for sensitive instruction. */
extern unsigned long iee_rwx_gate(int flag, ...);

/* Replace sysreg_clear_set in sysreg.h. */
#define iee_si_sysreg_clear_set(sysreg, clear, set) do {			\
	u64 __scs_val = read_sysreg(sysreg);				\
	u64 __scs_new = (__scs_val & ~(u64)(clear)) | (set);		\
	if (__scs_new != __scs_val)					\
		iee_rwx_gate(IEE_SI_SET_##sysreg, __scs_new);			\
} while (0)

#define IEE_SI_SCTLR_MASK  (SCTLR_EL1_CP15BEN | SCTLR_EL1_SED | SCTLR_EL1_UCT | \
			SCTLR_EL1_UCI | SCTLR_EL1_BT0 | SCTLR_EL1_BT1 | SCTLR_EL1_TCF0_MASK | \
			SCTLR_ELx_DSSBS | SCTLR_ELx_ENIA | SCTLR_ELx_ENIB | SCTLR_ELx_ENDA |\
			SCTLR_ELx_ENDB | SCTLR_EL1_SPINTMASK | SCTLR_EL1_NMI | SCTLR_EL1_TIDCP |\
			SCTLR_EL1_MSCEn | SCTLR_ELx_ENTP2 | SCTLR_EL1_TCF_MASK)

extern u64 __iee_si_text_start[];
extern u64 __iee_si_text_end[];
extern u64 iee_si_reserved_pg_dir;

#endif

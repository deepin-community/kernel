/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PTP_H
#define _LINUX_PTP_H

#include <asm/haoc/haoc-def.h>
#include <asm/pgtable_types.h>

extern unsigned long long ptp_rw_gate(int flag, ...);

static inline void ptp_set_pte(pte_t *ptep, pte_t pte)
{
	ptp_rw_gate(IEE_OP_SET_PTE, ptep, pte);
}

static inline void ptp_set_pmd(pmd_t *pmdp, pmd_t pmd)
{
	ptp_rw_gate(IEE_OP_SET_PMD, pmdp, pmd);
}

static inline void ptp_set_pud(pud_t *pudp, pud_t pud)
{
	ptp_rw_gate(IEE_OP_SET_PUD, pudp, pud);
}

static inline void ptp_set_p4d(p4d_t *p4dp, p4d_t p4d)
{
	ptp_rw_gate(IEE_OP_SET_P4D, p4dp, p4d);
}

static inline void ptp_set_pgd(pgd_t *pgdp, pgd_t pgd)
{
	ptp_rw_gate(IEE_OP_SET_PGD, pgdp, pgd);
}

static inline void ptp_set_pte_text_poke(pte_t *ptep, pte_t pte)
{
	ptp_rw_gate(IEE_OP_SET_PTE_TEXT_POKE, ptep, pte);
}

static inline void ptp_set_pmd_ident(pmd_t *pmdp, pmd_t pmd)
{
	ptp_rw_gate(IEE_OP_SET_PMD_IDENT, pmdp, pmd);
}

static inline void ptp_set_pud_ident(pud_t *pudp, pud_t pud)
{
	ptp_rw_gate(IEE_OP_SET_PUD_IDENT, pudp, pud);
}

static inline void ptp_set_sx_pte(pte_t *ptep, pte_t pte)
{
	ptp_rw_gate(IEE_OP_SET_SX_PTE, ptep, pte);
}

extern pgprotval_t ptp_xchg(pgprotval_t *pgprotp, pgprotval_t pgprotval);
extern pgprotval_t ptp_try_cmpxchg(pgprotval_t *pgprotp,
			pgprotval_t old_pgprot, pgprotval_t new_pgprotval);
extern void haoc_ptp_init(void);
extern struct iee_cache pgd_cache;

#include <linux/percpu.h>
struct iee_cr0 {
	/* Writable but considered safe to expose */
	unsigned long wp_disabled_cnt;
};

DECLARE_PER_CPU(struct iee_cr0, iee_cr0s);
#endif

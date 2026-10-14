/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_PTP_H
#define _LINUX_PTP_H

#include <asm/haoc/haoc-def.h>
#include <asm/page.h>
#include <asm/pgtable_types.h>
#ifdef CONFIG_PTP_S
#include <asm/haoc/haoc-bitmap.h>
#endif

extern unsigned long long ptp_rw_gate(int flag, ...);

#ifdef CONFIG_PTP_S
extern bool ptp_is_user_pgtable(const void *ptp);
extern void ptp_user_check_pte_update(pte_t *ptep, pte_t pte);
extern void ptp_user_check_pmd_update(pmd_t *pmdp, pmd_t pmd);
extern void ptp_user_check_pud_update(pud_t *pudp, pud_t pud);
#endif

static inline void ptp_set_pte(pte_t *ptep, pte_t pte)
{
#ifdef CONFIG_PTP_S
	if (ptp_is_user_pgtable(ptep)) {
		ptp_user_check_pte_update(ptep, pte);
		WRITE_ONCE(*ptep, pte);
		return;
	}
#endif
	ptp_rw_gate(IEE_OP_SET_PTE, ptep, pte);
}

static inline void ptp_set_pmd(pmd_t *pmdp, pmd_t pmd)
{
#ifdef CONFIG_PTP_S
	if (ptp_is_user_pgtable(pmdp)) {
		ptp_user_check_pmd_update(pmdp, pmd);
		WRITE_ONCE(*pmdp, pmd);
		return;
	}
#endif
	ptp_rw_gate(IEE_OP_SET_PMD, pmdp, pmd);
}

static inline void ptp_set_pud(pud_t *pudp, pud_t pud)
{
#ifdef CONFIG_PTP_S
	if (ptp_is_user_pgtable(pudp)) {
		ptp_user_check_pud_update(pudp, pud);
		WRITE_ONCE(*pudp, pud);
		return;
	}
#endif
	ptp_rw_gate(IEE_OP_SET_PUD, pudp, pud);
}

static inline void ptp_set_p4d(p4d_t *p4dp, p4d_t p4d)
{
#ifdef CONFIG_PTP_S
	if (ptp_is_user_pgtable(p4dp)) {
		WRITE_ONCE(*p4dp, p4d);
		return;
	}
#endif
	ptp_rw_gate(IEE_OP_SET_P4D, p4dp, p4d);
}

static inline void ptp_set_pgd(pgd_t *pgdp, pgd_t pgd)
{
#ifdef CONFIG_PTP_S
	if (ptp_is_user_pgtable(pgdp)) {
		WRITE_ONCE(*pgdp, pgd);
		return;
	}
#endif
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

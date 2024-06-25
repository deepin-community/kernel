// SPDX-License-Identifier: GPL-2.0
/*
 * arch/sw_64/kernel/hmcall.c
 *
 * Copyright (C) 2022 WXIAT
 * Author: He Sheng
 */

#include <asm/hmcall.h>
#include <asm/page.h>

#define A0(func)	(((HMC_##func & 0xFF) >> 6) & 0x1)
#define A1(func)	((((HMC_##func & 0xFF)>>6) & 0x2) >> 1)
#define A2(func)	((HMC_##func & 0x3F) << 7)

#define T(func)		((A0(func) ^ A1(func)) & 0x1)
#define B0(func)	((T(func) | A0(func)) << 13)
#define B1(func)	(((~T(func) & 1) | A1(func)) << 14)

#define PRI_BASE	0x10000UL

#define HMCALL_ENTRY(func)	(PRI_BASE | B1(func) | B0(func) | A2(func))


static inline void fixup_rdtp(void)
{
	unsigned int *entry = __va(HMCALL_ENTRY(rdtp));

	entry[0] = 0x181ffec7;	/* pri_rcsr $0, CSR__TID */
	entry[1] = 0x1ee00000;	/* pri_ret $23 */
}

static inline void fixup_wrtp(void)
{
	unsigned int *entry = __va(HMCALL_ENTRY(wrtp));

	entry[0] = 0x1a1fffc7;	/* pri_wcsr $16, CSR__TID */
	entry[1] = 0x1ee00000;	/* pri_ret $23 */
}

static inline void fixup_tbiasid(void)
{
	unsigned int *entry = __va(HMCALL_ENTRY(tbisasid));

	entry[0] = 0x18fffe47;	/* pri_rcsr p7, CSR__DTB_PCR*/
	entry[1] = 0x4a05c905;	/* sll r16, CSR__DTB_PCR__UPN__S, p5 */
	entry[2] = 0xf89f03ff;  /* ldi p4, CSR__DTB_PCR__UPN__M */
	entry[3] = 0x4885c904;	/* sll p4, CSR__DTB_PCR__UPN__S, p4 */
	entry[4] = 0x40e40724;	/* bic p7, p4, p4 */
	entry[5] = 0x40850745;	/* bis p4, p5, p5 */
	entry[6] = 0x18bfff47;	/* pri_wcsr p5, CSR__DTB_PCR */
	entry[7] = 0x1a3fff46;	/* pri_wcsr r17, CSR__DTB_IS */
	entry[8] = 0x18ffff47;	/* pri_wcsr p7, CSR__DTB_PCR */
	entry[9] = 0x4a04e906;	/* sll r16, CSR__UPCR_UPN__UPN__S, p6 */
	entry[10] = 0x189ffe22;	/* pri_rcsr p4, CSR__UPCR_UPN */
	entry[11] = 0x18dfff22; /* pri_wcsr p6, CSR__UPCR_UPN */
	entry[12] = 0x1a3fff06; /* pri_wcsr r17, CSR__ITB_IS */
	entry[13] = 0x1bffff15; /* pri_wcsr r31, CSR__IC_FLUSH */
	entry[14] = 0x189fff22; /* pri_wcsr p4, CSR__UPCR_UPN */
	entry[15] = 0x1ef00000; /* pri_ret/b p23 */
}

static inline void fixup_wrasid(void)
{
	unsigned int *entry = __va(HMCALL_ENTRY(wrasid));

	entry[0] = 0x18fffe47;	/* pri_rcsr p7, CSR__DTB_PCR*/
	entry[1] = 0x4a05c905;	/* sll r16, CSR__DTB_PCR__UPN__S, p5 */
	entry[2] = 0xf89f03ff;  /* ldi p4, CSR__DTB_PCR__UPN__M */
	entry[3] = 0x4885c904;	/* sll p4, CSR__DTB_PCR__UPN__S, p4 */
	entry[4] = 0x40e40724;	/* bic p7, p4, p4 */
	entry[5] = 0x40850745;	/* bis p4, p5, p5 */
	entry[6] = 0x18bfff47;	/* pri_wcsr p5, CSR__DTB_PCR */
	entry[7] = 0x4a04e906;	/* sll r16, CSR__UPCR_UPN__UPN__S, p6 */
	entry[8] = 0x18dfff22;	/* pri_wcsr p4, CSR__UPCR_UPN */
	entry[9] = 0x1ef00000;	/* pri_ret/b p23 */
}

static inline void fixup_rdktp(void)
{
	unsigned int *entry = __va(HMCALL_ENTRY(rdktp));

	entry[0] = 0x95161000;	/* pri_ldl/p $8, VC__KTP(vcpucb) */
	entry[1] = 0x1ee00000;	/* pri_ret $23 */
}

static inline void fixup_wrktp(void)
{
	unsigned int *entry = __va(HMCALL_ENTRY(wrktp));

	entry[0] = 0xb5161000;	/* pri_stl/p $8, VC__KTP(vcpucb) */
	entry[1] = 0x1ee00000;	/* pri_ret $23 */
}

static inline void fixup_rdusp(void)
{
	unsigned int *entry = __va(HMCALL_ENTRY(rdusp));

	entry[0] = 0x94161018;	/* pri_ldl/p $0, VC__USP(vcpucb) */
	entry[1] = 0x1ee00000;	/* pri_ret $23 */
}

static inline void fixup_wrusp(void)
{
	unsigned int *entry = __va(HMCALL_ENTRY(wrusp));

	entry[0] = 0xb6161018;	/* pri_stl/p $16, VC__USP(vcpucb) */
	entry[1] = 0x1ee00000;	/* pri_ret $23 */
}

static inline void fixup_uwhami(void)
{
	unsigned int *entry = __va(HMCALL_ENTRY(uwhami));

	entry[0] = 0x94161078;	/* pri_ldl/p       r0, VC__WHAMI(vcpucb) */
	entry[1] = 0x1ef00000;	/* pri_ret/b $23 */
}

void __init fixup_hmcall(void)
{
#if defined(CONFIG_SUBARCH_C3B)
	fixup_rdtp();
	fixup_wrtp();
	fixup_tbiasid();
	fixup_wrasid();
	fixup_rdktp();
	fixup_wrktp();
	fixup_rdusp();
	fixup_wrusp();
	fixup_uwhami();
	imemb();
#endif
}

#undef A0
#undef A1
#undef A2
#undef T
#undef B0
#undef B1

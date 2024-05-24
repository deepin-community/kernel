/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_CORE_H
#define _ASM_SW64_CORE_H

#include <asm/csr.h>

#define II_II0			0
#define II_II1			1
#define II_SLEEP		2
#define II_WAKE			3
#define II_NMII			6

#define II_RESET		II_NMII

#if defined(CONFIG_SUBARCH_C3B)

#define DOMAIN_ID_BITS		2
#define DOMAIN_ID_SHIFT		5

#define THREAD_ID_BITS		1
#define THREAD_ID_SHIFT		31

#define CORE_ID_BITS		5
#define CORE_ID_SHIFT		0

static inline bool core_is_ht(void)
{
	return 0;
}

#elif defined(CONFIG_SUBARCH_C4)

#define DOMAIN_ID_BITS		2
#define DOMAIN_ID_SHIFT		12

#define THREAD_ID_BITS		1
#define THREAD_ID_SHIFT		8

#define CORE_ID_BITS		6
#define CORE_ID_SHIFT		0

static inline bool core_is_ht(void)
{
	return rdhtctl() == 0x3;
}

#endif

#define DOMAIN_ID_MASK		(GENMASK(DOMAIN_ID_BITS - 1, 0) << DOMAIN_ID_SHIFT)
#define THREAD_ID_MASK		(GENMASK(THREAD_ID_BITS - 1, 0) << THREAD_ID_SHIFT)
#define CORE_ID_MASK		(GENMASK(CORE_ID_BITS - 1, 0) << CORE_ID_SHIFT)
#define MAX_CORES_PER_CPU	(1 << CORE_ID_BITS)

/*
 * 0x00 ~ 0xff for hardware mm fault
 */

#define MMCSR__TNV		0x0
#define MMCSR__IACV		0x1
#define MMCSR__FOR		0x2
#define MMCSR__FOE		0x3
#define MMCSR__FOW		0x4

#define MMCSR__BAD_DVA		0x6
#define MMCSR__ACV1		0x7
#define MMCSR__ACV0		0xc
#define MMCSR__BAD_IVA		0xf

/* 0x100 ~ 0x1ff for match debug */
#define MMCSR__DA_MATCH		0x100
#define MMCSR__DV_MATCH		0x101
#define MMCSR__DAV_MATCH	0x102
#define MMCSR__IA_MATCH		0x103
#define MMCSR__IDA_MATCH	0x104
#define MMCSR__IV_MATCH		0x105

 /* entry.S */
extern void entArith(void);
extern void entIF(void);
extern void entInt(void);
extern void entNMI(void);
extern void entMM(void);
extern void entSys(void);
extern void entUna(void);
/* head.S */
extern void __smp_callin(unsigned long args);
#endif /* _ASM_SW64_CORE_H */

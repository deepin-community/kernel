/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#ifndef _LINUX_IEE_ASM_H
#define _LINUX_IEE_ASM_H

#include <asm/pgtable-hwdef.h>

#define BAD_ELR_EL1	0
#define BAD_TCR_EL1 1

#define ASID_BIT		(UL(1) << 48)
/* 
 * We reserves the bigest ASID for IEE and always stores it in TTBR0.
 */
#define IEE_ASID			0xffff
#define IEE_ASM_ASID		(UL(IEE_ASID) << 48)

#define TCR_HPD1		(UL(1) << 42)
#define TCR_A1			(UL(1) << 22)
#define IEE_TCR_MASK		(~(TCR_HD | TCR_E0PD1 | TCR_T0SZ_MASK))

#ifdef CONFIG_IEE_SIP
/* IEE exit code Remember that ARM instructions are aligned with 8 byte. */
#define IEE_SI_EXIT_OFFSET	(4*4)
#define IEE_SI_TCR_MASK		(~IEE_TCR_MASK)
#endif

#endif

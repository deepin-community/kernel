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
 * We reserves the bigest ASID for IEE and always stores it in TTBR1. As KPTI also reserves
 * odd ASIDs for user-viewed TTBR1, we should use even number for IEE ASID to allow KPTI to
 * switch between them at kernel entry/exit.
 */
#ifdef CONFIG_UNMAP_KERNEL_AT_EL0
#define IEE_ASID			0xfffe
#else
#define IEE_ASID			0xffff
#endif
#define IEE_ASM_ASID		(UL(IEE_ASID) << 48)

#define TCR_HPD1		(UL(1) << 42)
#define TCR_A1			(UL(1) << 22)
#define IEE_TCR_MASK		(~(TCR_HD | TCR_E0PD1 | TCR_T0SZ_MASK))

#endif

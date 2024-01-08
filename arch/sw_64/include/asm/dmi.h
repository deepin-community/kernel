/* SPDX-License-Identifier: GPL-2.0 */
/*
 * arch/sw_64/include/asm/dmi.h
 *
 * Copyright (C) 2019 Deepin Limited.
 * Porting by: Deepin Kernel Team (kernel@deepin.com)
 *
 * based on arch/x864/include/asm/dmi.h
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#ifndef _ASM_SW64_DMI_H
#define _ASM_SW64_DMI_H

#include <linux/io.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <asm/early_ioremap.h>

/* Use early IO mappings for DMI because it's initialized early */
#define dmi_early_remap(x, l)		early_ioremap(x, l)
#define dmi_early_unmap(x, l)		early_iounmap(x, l)
#define dmi_remap(x, l)			early_ioremap(x, l)
#define dmi_unmap(x)			early_iounmap(x, 0)
#define dmi_alloc(l)			kzalloc(l, GFP_KERNEL)

#endif /* _ASM_SW64_DMI_H */

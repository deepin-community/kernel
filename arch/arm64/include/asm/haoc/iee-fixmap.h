/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IEE_FIXMAP_H
#define _LINUX_IEE_FIXMAP_H

#include <asm/fixmap.h>

#ifndef clear_fixmap_pre_init
#define clear_fixmap_pre_init(idx)			\
	__iee_set_fixmap_pre_init(idx, 0, FIXMAP_PAGE_CLEAR)
#endif

#define __iee_set_fixmap_offset_pre_init(idx, phys, flags)				\
({									\
	unsigned long ________addr;					\
	__iee_set_fixmap_pre_init(idx, phys, flags);					\
	________addr = fix_to_virt(idx) + ((phys) & (PAGE_SIZE - 1));	\
	________addr;							\
})

#define iee_set_fixmap_offset_pre_init(idx, phys) \
	__iee_set_fixmap_offset_pre_init(idx, phys, FIXMAP_PAGE_NORMAL)

#endif

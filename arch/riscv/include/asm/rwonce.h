/* SPDX-License-Identifier: GPL-2.0 */

#ifndef __ASM_RWONCE_H
#define __ASM_RWONCE_H

#include <linux/compiler_types.h>
#include <asm/alternative-macros.h>
#include <asm/vendorid_list.h>
#include <asm/errata_list_vendors.h>

#if defined(CONFIG_ERRATA_THEAD_WRITE_ONCE) && !defined(NO_ALTERNATIVE)

#define write_once_fence()				\
do {							\
	asm volatile(ALTERNATIVE(			\
		"nop",					\
		"fence w, o",				\
		THEAD_VENDOR_ID,			\
		ERRATA_THEAD_WRITE_ONCE,		\
		CONFIG_ERRATA_THEAD_WRITE_ONCE)		\
		: : : "memory");			\
} while (0)

#define __WRITE_ONCE(x, val)				\
do {							\
	*(volatile typeof(x) *)&(x) = (val);		\
	write_once_fence();				\
} while (0)

#endif /* defined(CONFIG_ERRATA_THEAD_WRITE_ONCE) && !defined(NO_ALTERNATIVE) */

#include <asm-generic/rwonce.h>

#endif	/* __ASM_RWONCE_H */

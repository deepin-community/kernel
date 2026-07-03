/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_BUG_H
#define __ASM_BUG_H

#include <asm/break.h>
#include <linux/stringify.h>
#include <linux/objtool.h>

#ifndef CONFIG_DEBUG_BUGVERBOSE
#define _BUGVERBOSE_LOCATION(file, line)
#else
#define __BUGVERBOSE_LOCATION(file, line)			\
		.pushsection .rodata.str, "aMS", @progbits, 1;	\
	10002:	.ascii file "\0";				\
		.popsection;					\
								\
		.long 10002b - .;				\
		.short line;
#define _BUGVERBOSE_LOCATION(file, line) __BUGVERBOSE_LOCATION(file, line)
#endif

#ifndef CONFIG_GENERIC_BUG
#define __BUG_ENTRY(cond_str, flags)
#else

/*
 * __bug_table entries are writable, so use a raw ANNOTYPE_DATA_SPECIAL
 * annotation instead of SHF_MERGE to let klp-build discover individual
 * entries.  This macro is stringified for C inline asm, so keep the
 * annotation in assembler syntax rather than using ANNOTATE_DATA_SPECIAL,
 * which is already a C string literal there.
 */
#define __BUG_ENTRY(cond_str, flags)				\
		.pushsection __bug_table, "aw";			\
		.align 2;					\
	10000:	.long 10001f - .;				\
		.pushsection .discard.annotate_data, "M", @progbits, 8; \
		.long 10000b - ., ANNOTYPE_DATA_SPECIAL;		\
		.popsection;					\
		_BUGVERBOSE_LOCATION(WARN_CONDITION_STR(cond_str) __FILE__, __LINE__) \
		.short flags;					\
		.popsection;					\
	10001:
#endif

#define ASM_BUG_FLAGS(cond_str, flags)				\
	__BUG_ENTRY(cond_str, flags)				\
	break		BRK_BUG;

#define ASM_BUG()	ASM_BUG_FLAGS("", 0)

#define __BUG_FLAGS(cond_str, flags, extra)			\
	asm_inline volatile (__stringify(ASM_BUG_FLAGS(cond_str, flags)) extra);

#define __WARN_FLAGS(cond_str, flags)				\
do {								\
	instrumentation_begin();				\
	__BUG_FLAGS(cond_str, BUGFLAG_WARNING|(flags), ANNOTATE_REACHABLE(10001b));\
	instrumentation_end();					\
} while (0)

#define BUG()							\
do {								\
	instrumentation_begin();				\
	__BUG_FLAGS("", 0, "");					\
	unreachable();						\
} while (0)

#define HAVE_ARCH_BUG

#include <asm-generic/bug.h>

#endif /* __ASM_BUG_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_LINKAGE_H
#define _ASM_SW64_LINKAGE_H

#define cond_syscall(x)	asm(".weak\t" #x "\n" #x " = sys_ni_syscall")
#define SYSCALL_ALIAS(alias, name)                                      \
	asm (#alias " = " #name "\n\t.globl " #alias)

#define SYM_END(name, sym_type)				\
       .type name sym_type ASM_NL			\
       .size name, .-name

#endif /* _ASM_SW64_LINKAGE_H */

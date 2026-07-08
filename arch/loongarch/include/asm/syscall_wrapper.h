/* SPDX-License-Identifier: GPL-2.0 */
/*
 * syscall_wrapper.h - LoongArch specific wrappers to syscall definitions
 *
 * Based on arch/arm64/include/asm/syscall_wrapper.h
 */

#ifndef __ASM_SYSCALL_WRAPPER_H
#define __ASM_SYSCALL_WRAPPER_H

#include <asm/ptrace.h>

/*
 * Instead of syscall_fn(regs->orig_a0, regs->regs[5], ...), call
 * __loongarch_sys_*(const struct pt_regs *regs) and let the wrapper
 * extract arguments.  This isolates the syscall table from the
 * calling convention.
 *
 * Use orig_a0 for the first argument (not regs[4]) to protect
 * against concurrent modification by ptrace during a syscall stop,
 * matching the rationale used by riscv.
 *
 * __MAP(x, __SC_ARGS, ,,val1,,val2, ...) deliberately omits the type
 * field of __SC_ARGS(t, a) via leading empty commas, extracting only
 * the argument values needed by __se_sys_*().
 */
#define SC_LOONGARCH_REGS_TO_ARGS(x, ...)			\
	__MAP(x,__SC_ARGS					\
	      ,,regs->orig_a0,,regs->regs[5],,regs->regs[6]	\
	      ,,regs->regs[7],,regs->regs[8],,regs->regs[9])

/*
 * For future CONFIG_COMPAT support.
 * LoongArch does not yet implement a compat layer; the macros below are
 * prepared for when it does, following the arm64 pattern.
 */
#if 0 /* IS_ENABLED(CONFIG_COMPAT) -- placeholder for future compat ABI */

#define COMPAT_SYSCALL_DEFINEx(x, name, ...)					\
	asmlinkage long __loongarch_compat_sys##name(const struct pt_regs *regs);\
	ALLOW_ERROR_INJECTION(__loongarch_compat_sys##name, ERRNO);		\
	static long __se_compat_sys##name(__MAP(x,__SC_LONG,__VA_ARGS__));	\
	static inline long __do_compat_sys##name(__MAP(x,__SC_DECL,__VA_ARGS__));\
	asmlinkage long __loongarch_compat_sys##name(const struct pt_regs *regs)\
	{									\
		return __se_compat_sys##name(SC_LOONGARCH_REGS_TO_ARGS(x,__VA_ARGS__));\
	}									\
	static long __se_compat_sys##name(__MAP(x,__SC_LONG,__VA_ARGS__))	\
	{									\
		return __do_compat_sys##name(__MAP(x,__SC_DELOUSE,__VA_ARGS__));\
	}									\
	static inline long __do_compat_sys##name(__MAP(x,__SC_DECL,__VA_ARGS__))

#define COMPAT_SYSCALL_DEFINE0(sname)						\
	asmlinkage long __loongarch_compat_sys_##sname(const struct pt_regs *__unused);\
	ALLOW_ERROR_INJECTION(__loongarch_compat_sys_##sname, ERRNO);		\
	asmlinkage long __loongarch_compat_sys_##sname(const struct pt_regs *__unused)

#define COND_SYSCALL_COMPAT(name)						\
	asmlinkage long __weak __loongarch_compat_sys_##name(const struct pt_regs *regs);\
	asmlinkage long __weak __loongarch_compat_sys_##name(const struct pt_regs *regs)\
	{									\
		return sys_ni_syscall();					\
	}

#endif /* CONFIG_COMPAT placeholder */

/*
 * Arch-specific syscall stub macros, following the x86 __X64_SYS_STUBx
 * pattern.  These generate the pt_regs-based entry points that the
 * syscall table references.  Both the arch's own __SYSCALL_DEFINEx
 * and the livepatch KLP helpers reuse these macros.
 */
#define __LOONGARCH_SYS_STUBx(x, name, ...)					\
	asmlinkage long __loongarch_sys##name(const struct pt_regs *regs);	\
	ALLOW_ERROR_INJECTION(__loongarch_sys##name, ERRNO);			\
	asmlinkage long __loongarch_sys##name(const struct pt_regs *regs)	\
	{									\
		return __se_sys##name(SC_LOONGARCH_REGS_TO_ARGS(x, __VA_ARGS__));\
	}

#define __LOONGARCH_SYS_STUB0(sname)						\
	asmlinkage long __loongarch_sys_##sname(const struct pt_regs *__unused);\
	ALLOW_ERROR_INJECTION(__loongarch_sys_##sname, ERRNO);

/*
 * The syscall table entry point.  The real implementation lives in
 * __do_sys_*(), reached through the sign-extension sanitizer
 * __se_sys_*().
 */
#define __SYSCALL_DEFINEx(x, name, ...)					\
	static long __se_sys##name(__MAP(x,__SC_LONG,__VA_ARGS__));		\
	static inline long __do_sys##name(__MAP(x,__SC_DECL,__VA_ARGS__));	\
	__LOONGARCH_SYS_STUBx(x, name, __VA_ARGS__)				\
	static long __se_sys##name(__MAP(x,__SC_LONG,__VA_ARGS__))		\
	{									\
		long ret = __do_sys##name(__MAP(x,__SC_CAST,__VA_ARGS__));	\
		__MAP(x,__SC_TEST,__VA_ARGS__);					\
		__PROTECT(x, ret,__MAP(x,__SC_ARGS,__VA_ARGS__));		\
		return ret;							\
	}									\
	static inline long __do_sys##name(__MAP(x,__SC_DECL,__VA_ARGS__))

/*
 * __loongarch_sys_ni_syscall() is the table fallback for unimplemented
 * syscall numbers; it delegates to the generic sys_ni_syscall().
 */
asmlinkage long __loongarch_sys_ni_syscall(const struct pt_regs *__unused);

#define SYSCALL_DEFINE0(sname)							\
	SYSCALL_METADATA(_##sname, 0);						\
	__LOONGARCH_SYS_STUB0(sname)						\
	asmlinkage long __loongarch_sys_##sname(const struct pt_regs *__unused)

#define COND_SYSCALL(name)							\
	asmlinkage long __loongarch_sys_##name(const struct pt_regs *regs);	\
	asmlinkage long __weak __loongarch_sys_##name(const struct pt_regs *regs)\
	{									\
		return sys_ni_syscall();					\
	}

#endif /* __ASM_SYSCALL_WRAPPER_H */

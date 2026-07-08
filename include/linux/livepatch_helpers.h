/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_LIVEPATCH_HELPERS_H
#define _LINUX_LIVEPATCH_HELPERS_H

/*
 * Interfaces for use by livepatch patches
 */

#include <linux/syscalls.h>
#include <linux/livepatch.h>

#ifdef MODULE
#define KLP_OBJNAME __KBUILD_MODNAME
#else
#define KLP_OBJNAME vmlinux
#endif

/* Livepatch callback registration */

#define KLP_CALLBACK_PTRS ".discard.klp_callback_ptrs"

#define KLP_PRE_PATCH_CALLBACK(func)						\
	klp_pre_patch_t __used __section(KLP_CALLBACK_PTRS)			\
		__PASTE(__KLP_PRE_PATCH_PREFIX, KLP_OBJNAME) = func

#define KLP_POST_PATCH_CALLBACK(func)						\
	klp_post_patch_t __used __section(KLP_CALLBACK_PTRS)			\
		__PASTE(__KLP_POST_PATCH_PREFIX, KLP_OBJNAME) = func

#define KLP_PRE_UNPATCH_CALLBACK(func)						\
	klp_pre_unpatch_t __used __section(KLP_CALLBACK_PTRS)			\
		__PASTE(__KLP_PRE_UNPATCH_PREFIX, KLP_OBJNAME) = func

#define KLP_POST_UNPATCH_CALLBACK(func)						\
	klp_post_unpatch_t __used __section(KLP_CALLBACK_PTRS)			\
		__PASTE(__KLP_POST_UNPATCH_PREFIX, KLP_OBJNAME) = func

/*
 * Replace static_call() usage with this macro when create-diff-object
 * recommends it due to the original static call key living in a module.
 *
 * This converts the static call to a regular indirect call.
 */
#define KLP_STATIC_CALL(name) \
	((typeof(STATIC_CALL_TRAMP(name))*)(STATIC_CALL_KEY(name).func))

/* Syscall patching */

#define KLP_SYSCALL_DEFINE1(name, ...) KLP_SYSCALL_DEFINEx(1, _##name, __VA_ARGS__)
#define KLP_SYSCALL_DEFINE2(name, ...) KLP_SYSCALL_DEFINEx(2, _##name, __VA_ARGS__)
#define KLP_SYSCALL_DEFINE3(name, ...) KLP_SYSCALL_DEFINEx(3, _##name, __VA_ARGS__)
#define KLP_SYSCALL_DEFINE4(name, ...) KLP_SYSCALL_DEFINEx(4, _##name, __VA_ARGS__)
#define KLP_SYSCALL_DEFINE5(name, ...) KLP_SYSCALL_DEFINEx(5, _##name, __VA_ARGS__)
#define KLP_SYSCALL_DEFINE6(name, ...) KLP_SYSCALL_DEFINEx(6, _##name, __VA_ARGS__)

#ifdef CONFIG_LOONGARCH
/*
 * LoongArch uses pt_regs-based syscall wrappers (ARCH_HAS_SYSCALL_WRAPPER).
 * The syscall table entry is __loongarch_sys_*(), which extracts arguments
 * from pt_regs before calling the sign-extension sanitizer __se_sys_*().
 * KLP replacements must provide the same __loongarch_sys_*() entry point,
 * delegating the real work to __klp_do_sys_*() to avoid colliding with
 * the original __do_sys_*().
 */
#define KLP_SYSCALL_DEFINE0(sname)						\
	__LOONGARCH_SYS_STUB0(sname)						\
	static inline long __klp_do_sys##sname(void);				\
	asmlinkage long __loongarch_sys_##sname(const struct pt_regs *__unused)\
	{									\
		return __klp_do_sys##sname();					\
	}									\
	static inline long __klp_do_sys##sname(void)
#else
#define KLP_SYSCALL_DEFINE0(sname)					\
	asmlinkage long sys_##sname(void)				\
		__attribute__((alias(__stringify(__se_sys##sname))));	\
	ALLOW_ERROR_INJECTION(sys_##sname, ERRNO);			\
	static inline long __klp_do_sys##sname(void);			\
	asmlinkage long __se_sys##sname(void)				\
	{								\
		return __klp_do_sys##sname();				\
	}								\
	static inline long __klp_do_sys##sname(void)
#endif

#define KLP_SYSCALL_DEFINEx(x, sname, ...)				\
	__KLP_SYSCALL_DEFINEx(x, sname, __VA_ARGS__)

#ifdef CONFIG_X86_64
// TODO move this to arch/x86/include/asm/syscall_wrapper.h and share code
#define __KLP_SYSCALL_DEFINEx(x, name, ...)			\
	static long __se_sys##name(__MAP(x,__SC_LONG,__VA_ARGS__));	\
	static inline long __klp_do_sys##name(__MAP(x,__SC_DECL,__VA_ARGS__));\
	__X64_SYS_STUBx(x, name, __VA_ARGS__)				\
	__IA32_SYS_STUBx(x, name, __VA_ARGS__)				\
	static long __se_sys##name(__MAP(x,__SC_LONG,__VA_ARGS__))	\
	{								\
		long ret = __klp_do_sys##name(__MAP(x,__SC_CAST,__VA_ARGS__));\
		__MAP(x,__SC_TEST,__VA_ARGS__);				\
		__PROTECT(x, ret,__MAP(x,__SC_ARGS,__VA_ARGS__));	\
		return ret;						\
	}								\
	static inline long __klp_do_sys##name(__MAP(x,__SC_DECL,__VA_ARGS__))

#endif

#ifdef CONFIG_LOONGARCH
/*
 * Generate pt_regs-based KLP syscall stubs that match the LoongArch
 * ARCH_HAS_SYSCALL_WRAPPER convention: __loongarch_sys_*(regs) extracts
 * arguments from regs, passes them through the sanitizer __se_sys_*(),
 * and calls the KLP override body __klp_do_sys_*().
 */
#define __KLP_SYSCALL_DEFINEx(x, name, ...)					\
	static long __se_sys##name(__MAP(x,__SC_LONG,__VA_ARGS__));		\
	static inline long __klp_do_sys##name(__MAP(x,__SC_DECL,__VA_ARGS__));\
	__LOONGARCH_SYS_STUBx(x, name, __VA_ARGS__)				\
	static long __se_sys##name(__MAP(x,__SC_LONG,__VA_ARGS__))		\
	{									\
		long ret = __klp_do_sys##name(__MAP(x,__SC_CAST,__VA_ARGS__));\
		__MAP(x,__SC_TEST,__VA_ARGS__);					\
		__PROTECT(x, ret,__MAP(x,__SC_ARGS,__VA_ARGS__));		\
		return ret;							\
	}									\
	static inline long __klp_do_sys##name(__MAP(x,__SC_DECL,__VA_ARGS__))
#endif /* CONFIG_LOONGARCH */

#endif /* _LINUX_LIVEPATCH_HELPERS_H */

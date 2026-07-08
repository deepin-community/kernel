// SPDX-License-Identifier: GPL-2.0+
/*
 * Author: Hanlu Li <lihanlu@loongson.cn>
 *         Huacai Chen <chenhuacai@loongson.cn>
 *
 * Copyright (C) 2020-2022 Loongson Technology Corporation Limited
 */
#include <linux/capability.h>
#include <linux/entry-common.h>
#include <linux/errno.h>
#include <linux/linkage.h>
#include <linux/nospec.h>
#include <linux/objtool.h>
#include <linux/randomize_kstack.h>
#include <linux/syscalls.h>
#include <linux/unistd.h>

#include <asm/asm.h>
#include <asm/exception.h>
#include <asm/loongarch.h>
#include <asm/signal.h>
#include <asm/switch_to.h>
#include <asm/syscall.h>
#include <asm-generic/syscalls.h>

SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len, unsigned long,
		prot, unsigned long, flags, unsigned long, fd, unsigned long, offset)
{
	if (offset & ~PAGE_MASK)
		return -EINVAL;

	return ksys_mmap_pgoff(addr, len, prot, flags, fd, offset >> PAGE_SHIFT);
}

SYSCALL_DEFINE6(mmap2, unsigned long, addr, unsigned long, len, unsigned long,
		 prot, unsigned long, flags, unsigned long, fd, unsigned long, offset)
{
	if (offset & (~PAGE_MASK >> 12))
		return -EINVAL;

	return ksys_mmap_pgoff(addr, len, prot, flags, fd, offset >> (PAGE_SHIFT - 12));
}

/*
 * Forward-declare every syscall table entry so the table initializer
 * can reference them.  Arch overrides (mmap, mmap2) are declared
 * separately above through SYSCALL_DEFINEx.
 */
#undef __SYSCALL
#define __SYSCALL(nr, sym)	asmlinkage long __loongarch_##sym(const struct pt_regs *);
#define __SYSCALL_WITH_COMPAT(nr, native, compat) __SYSCALL(nr, native)
#include <asm/syscall_table_64.h>

#undef __SYSCALL
#define __SYSCALL(nr, sym)	[nr] = __loongarch_##sym,
#define __SYSCALL_WITH_COMPAT(nr, native, compat) __SYSCALL(nr, native)

/*
 * sys_ni_syscall() is declared inside #ifndef CONFIG_ARCH_HAS_SYSCALL_WRAPPER
 * in include/linux/syscalls.h, so it is not visible with the wrapper enabled.
 * Provide a local forward declaration, matching what arm64 does.
 */
asmlinkage long sys_ni_syscall(void);

const syscall_fn_t sys_call_table[__NR_syscalls] = {
	[0 ... __NR_syscalls - 1] = __loongarch_sys_ni_syscall,
#ifdef CONFIG_32BIT
#include <asm/syscall_table_32.h>
#else
#include <asm/syscall_table_64.h>
#endif
};

asmlinkage long __loongarch_sys_ni_syscall(const struct pt_regs *__unused)
{
	return sys_ni_syscall();
}

void noinstr __no_stack_protector do_syscall(struct pt_regs *regs)
{
	unsigned long nr;
	syscall_fn_t syscall_fn;

	nr = regs->regs[11];
	/* Set for syscall restarting */
	if (nr < NR_syscalls)
		regs->regs[0] = nr + 1;

	regs->csr_era += 4;
	regs->orig_a0 = regs->regs[4];
	regs->regs[4] = -ENOSYS;

	nr = syscall_enter_from_user_mode(regs, nr);

	add_random_kstack_offset();

	if (nr < NR_syscalls) {
		syscall_fn = sys_call_table[array_index_nospec(nr, NR_syscalls)];
		regs->regs[4] = syscall_fn(regs);
	}

	syscall_exit_to_user_mode(regs);
}

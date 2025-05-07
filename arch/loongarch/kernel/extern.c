// SPDX-License-Identifier: GPL-2.0
#include <linux/signal.h>

void loongarch_set_current_blocked(sigset_t *newset);
int loongarch_copy_siginfo_to_user(siginfo_t __user *to, const kernel_siginfo_t *from);
int loongarch___save_altstack(stack_t __user *uss, unsigned long sp);
int loongarch_restore_altstack(const stack_t __user *uss);

void loongarch_set_current_blocked(sigset_t *newset)
{
	return set_current_blocked(newset);
}
EXPORT_SYMBOL_GPL(loongarch_set_current_blocked);

int loongarch_copy_siginfo_to_user(siginfo_t __user *to, const kernel_siginfo_t *from)
{
	return copy_siginfo_to_user(to, from);
}
EXPORT_SYMBOL_GPL(loongarch_copy_siginfo_to_user);

int loongarch___save_altstack(stack_t __user *uss, unsigned long sp)
{
	return __save_altstack(uss, sp);
}
EXPORT_SYMBOL_GPL(loongarch___save_altstack);

int loongarch_restore_altstack(const stack_t __user *uss)
{
	return restore_altstack(uss);
}
EXPORT_SYMBOL_GPL(loongarch_restore_altstack);

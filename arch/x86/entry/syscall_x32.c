// SPDX-License-Identifier: GPL-2.0
/* System call table for x32 ABI. */

#include <linux/linkage.h>
#include <linux/sys.h>
#include <linux/cache.h>
#include <linux/moduleparam.h>
#undef MODULE_PARAM_PREFIX
#define MODULE_PARAM_PREFIX "syscall."
#include <linux/syscalls.h>
#include <asm/syscall.h>

#define __SYSCALL(nr, sym) extern long __x64_##sym(const struct pt_regs *);
#include <asm/syscalls_x32.h>
#undef __SYSCALL

#define __SYSCALL(nr, sym) case nr: return __x64_##sym(regs);

long x32_sys_call(const struct pt_regs *regs, unsigned int nr)
{
	switch (nr) {
	#include <asm/syscalls_x32.h>
	default: return __x64_sys_ni_syscall(regs);
	}
};

/* Maybe enable x32 syscalls */

#if defined(CONFIG_X86_X32_DISABLED)
DEFINE_STATIC_KEY_FALSE(x32_enabled_skey);
#else
DEFINE_STATIC_KEY_TRUE(x32_enabled_skey);
#endif

static int __init x32_param_set(const char *val, const struct kernel_param *p)
{
	bool enabled;
	int ret;

	ret = kstrtobool(val, &enabled);
	if (ret)
		return ret;
	if (IS_ENABLED(CONFIG_X86_X32_DISABLED)) {
		if (enabled) {
			static_key_enable(&x32_enabled_skey.key);
			pr_info("Enabled x32 syscalls\n");
		}
	} else {
		if (!enabled) {
			static_key_disable(&x32_enabled_skey.key);
			pr_info("Disabled x32 syscalls\n");
		}
	}
	return 0;
}

static int x32_param_get(char *buffer, const struct kernel_param *p)
{
	return sprintf(buffer, "%c\n",
		       static_key_enabled(&x32_enabled_skey) ? 'Y' : 'N');
}

static const struct kernel_param_ops x32_param_ops = {
	.set = x32_param_set,
	.get = x32_param_get,
};

arch_param_cb(x32, &x32_param_ops, NULL, 0444);

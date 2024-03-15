// SPDX-License-Identifier: GPL-2.0

#include <asm/hw_init.h>

#include <linux/export.h>

extern long ____copy_user_hw_una(void *to, const void *from, long len);
extern long ____copy_user_sw_una(void *to, const void *from, long len);

long __copy_user(void *to, const void *from, long len)
{
	if (static_branch_likely(&core_hw_una_enabled))
		return ____copy_user_hw_una(to, from, len);
	else
		return ____copy_user_sw_una(to, from, len);
}
EXPORT_SYMBOL(__copy_user);

extern long ____clear_user_hw_una(void __user *to, long len);
extern long ____clear_user_sw_una(void __user *to, long len);

long __clear_user(void __user *to, long len)
{
	if (static_branch_likely(&core_hw_una_enabled))
		return ____clear_user_hw_una(to, len);
	else
		return ____clear_user_sw_una(to, len);
}
EXPORT_SYMBOL(__clear_user);

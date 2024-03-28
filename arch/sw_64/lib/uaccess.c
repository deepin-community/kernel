// SPDX-License-Identifier: GPL-2.0

#include <asm/hw_init.h>

#include <linux/export.h>

extern long ____copy_user_sisd(void *to, const void *from, long len);
extern long ____copy_user_simd(void *to, const void *from, long len);
extern long ____copy_user_simd_align(void *to, const void *from, long len);

long __copy_user(void *to, const void *from, long len)
{
	if (!IS_ENABLED(CONFIG_DEEP_COPY_USER))
		return ____copy_user_sisd(to, from, len);

	if (static_branch_likely(&hw_una_enabled))
		return ____copy_user_simd(to, from, len);
	else
		return ____copy_user_simd_align(to, from, len);
}
EXPORT_SYMBOL(__copy_user);

extern long ____clear_user_sisd(void __user *to, long len);
extern long ____clear_user_simd(void __user *to, long len);
extern long ____clear_user_simd_align(void __user *to, long len);

long __clear_user(void __user *to, long len)
{
	if (!IS_ENABLED(CONFIG_DEEP_CLEAR_USER))
		return ____clear_user_sisd(to, len);

	if (static_branch_likely(&hw_una_enabled))
		return ____clear_user_simd(to, len);
	else
		return ____clear_user_simd_align(to, len);
}
EXPORT_SYMBOL(__clear_user);

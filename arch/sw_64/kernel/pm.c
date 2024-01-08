// SPDX-License-Identifier: GPL-2.0
#include <linux/suspend.h>
#include <linux/syscore_ops.h>

#include <asm/suspend.h>

struct syscore_ops io_syscore_ops;

static int __init sw64_pm_init(void)
{
#ifdef CONFIG_SUSPEND
	suspend_set_ops(&native_suspend_ops);
#endif
	register_syscore_ops(&io_syscore_ops);

	return 0;
}
device_initcall(sw64_pm_init);

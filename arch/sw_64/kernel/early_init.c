// SPDX-License-Identifier: GPL-2.0
#include <linux/start_kernel.h>

#include <asm/sw64_init.h>
#include <asm/cpu.h>

bool sunway_support_kpt;

asmlinkage __visible void __init sw64_start_kernel(void)
{
	fixup_hmcall();
	save_ktp();
	start_kernel();
}

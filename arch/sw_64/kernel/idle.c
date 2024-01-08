// SPDX-License-Identifier: GPL-2.0
/*
 * sw64 idle loop support.
 *
 */
#include <linux/cpu.h>
#include <linux/irqflags.h>
#include <asm/cpu.h>
#include <asm/idle.h>
#include <asm/asm-offsets.h>

void arch_cpu_idle(void)
{
	local_irq_enable();
	cpu_relax();

	if (is_in_guest()) {
		if (!need_resched())
			hcall(HCALL_HALT, 0, 0, 0);
	} else {
		asm(
		".globl __idle_start\n"
		"__idle_start = .\n"
		"ldw	$1, %0($8)\n"
		"srl	$1, %1, $1\n"
		"blbs	$1, $need_resched\n"
		"halt\n"
		".globl __idle_end\n"
		"__idle_end = .\n"
		"$need_resched:"
		:: "i"(TI_FLAGS), "i"(TIF_NEED_RESCHED)
		: "$1");
	}
	local_irq_disable();
}

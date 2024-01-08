// SPDX-License-Identifier: GPL-2.0
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/clk-provider.h>

#include <asm/debug.h>
#include <asm/timer.h>

#include "proto.h"

DEFINE_SPINLOCK(rtc_lock);
EXPORT_SYMBOL(rtc_lock);

#define TICK_SIZE (tick_nsec / 1000)

/*
 * Shift amount by which scaled_ticks_per_cycle is scaled.  Shifting
 * by 48 gives us 16 bits for HZ while keeping the accuracy good even
 * for large CPU clock rates.
 */
#define FIX_SHIFT	48

unsigned long est_cycle_freq;

#ifdef CONFIG_IRQ_WORK

DEFINE_PER_CPU(u8, irq_work_pending);

#define set_irq_work_pending_flag()  __this_cpu_write(irq_work_pending, 1)
#define test_irq_work_pending()      __this_cpu_read(irq_work_pending)
#define clear_irq_work_pending()     __this_cpu_write(irq_work_pending, 0)

void arch_irq_work_raise(void)
{
	set_irq_work_pending_flag();
}

#else /* CONFIG_IRQ_WORK */

#define test_irq_work_pending()      0
#define clear_irq_work_pending()

#endif /* CONFIG_IRQ_WORK */

void __init
time_init(void)
{
	unsigned long cycle_freq;

	cycle_freq = get_cpu_freq();

	pr_info("CPU Cycle frequency = %ld Hz\n", cycle_freq);

	/* Register clocksource */
	sw64_setup_clocksource();
	of_clk_init(NULL);
	/* Startup the timer source. */
	sw64_setup_timer();
	/* Calibrate the delay loop directly */
	lpj_fine = cycle_freq / HZ;
}

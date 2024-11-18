// SPDX-License-Identifier: GPL-2.0
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/clk-provider.h>

#include <asm/cpu.h>
#include <asm/debug.h>
#include <asm/timer.h>
#include <linux/clocksource.h>

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

void clocksource_arch_init(struct clocksource *cs)
{
	cs->vdso_clock_mode = VDSO_CLOCKMODE_ARCHTIMER;
}

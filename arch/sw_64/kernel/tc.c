// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019, serveros, linyue
 */


#include <linux/topology.h>
#include <asm/tc.h>

/*
 * Entry/exit counters that make sure that both CPUs
 * run the measurement code at once:
 */
unsigned long time_sync;

DEFINE_PER_CPU(u64, tc_offset);

void tc_sync_clear(void)
{
	time_sync = 0;
}

void tc_sync_ready(void *ignored)
{
	/* make sure we can see time_sync been set to 0 */
	smp_mb();
	while (!time_sync)
		cpu_relax();

	__this_cpu_write(tc_offset, time_sync - rdtc());
}

void tc_sync_set(void)
{
	time_sync = rdtc() + __this_cpu_read(tc_offset);
}

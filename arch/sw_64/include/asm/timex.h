/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_TIMEX_H
#define _ASM_SW64_TIMEX_H

#include <asm/tc.h>

/* With only one or two oddballs, we use the RTC as the ticker, selecting
 * the 32.768kHz reference clock, which nicely divides down to our HZ.
 */
#define CLOCK_TICK_RATE	32768

/*
 * Standard way to access the cycle counter.
 */

typedef unsigned long cycles_t;

static inline cycles_t get_cycles(void)
{
	return rdtc();
}

#endif /* _ASM_SW64_TIMEX_H */

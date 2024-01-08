/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_TIMER_H
#define _ASM_SW64_TIMER_H

extern void sw64_setup_clocksource(void);

extern void sw64_setup_timer(void);

extern void __init setup_sched_clock(void);

#endif /* _ASM_SW64_TIMER_H */

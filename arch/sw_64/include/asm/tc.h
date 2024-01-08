/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_TC_H
#define _ASM_SW64_TC_H

static inline unsigned long rdtc(void)
{
	unsigned long ret;

	__asm__ __volatile__ ("rtc %0" : "=r"(ret));
	return ret;
}

extern void tc_sync_clear(void);
extern void tc_sync_ready(void *ignored);
extern void tc_sync_set(void);
#endif /* _ASM_SW64_TC_H */

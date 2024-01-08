// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 1993, 2000 Linus Torvalds
 *
 * Delay routines, using a pre-computed "loops_per_jiffy" value.
 */

#include <linux/module.h>

/*
 * Use only for very small delays (< 1 msec).
 *
 * The active part of our cycle counter is only 32-bits wide, and
 * we're treating the difference between two marks as signed.  On
 * a 1GHz box, that's about 2 seconds.
 */
void __delay(unsigned long loops)
{
	unsigned long tmp;

	__asm__ __volatile__(
		"	rtc %0\n"
		"	addl %1,%0,%1\n"
		"1:	rtc %0\n"
		"	subl %1,%0,%0\n"
		"	bgt %0,1b"
		: "=&r" (tmp), "=r" (loops) : "1"(loops));
}
EXPORT_SYMBOL(__delay);

void udelay(unsigned long usecs)
{
	unsigned long loops = usecs * get_cpu_freq() / 1000000;
	unsigned long tmp;

	__asm__ __volatile__(
		"	rtc %0\n"
		"	addl %1,%0,%1\n"
		"1:	rtc %0\n"
		"	subl %1,%0,%0\n"
		"	bgt %0,1b"
		: "=&r" (tmp), "=r" (loops) : "1"(loops));
}
EXPORT_SYMBOL(udelay);

void ndelay(unsigned long nsecs)
{
	unsigned long loops = nsecs * get_cpu_freq() / 1000000000;
	unsigned long tmp;

	__asm__ __volatile__(
		"	rtc %0\n"
		"	addl %1,%0,%1\n"
		"1:	rtc %0\n"
		"	subl %1,%0,%0\n"
		"	bgt %0,1b"
		: "=&r" (tmp), "=r" (loops) : "1"(loops));
}
EXPORT_SYMBOL(ndelay);

// SPDX-License-Identifier: GPL-2.0
/*
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

#include <linux/time.h>
#include <linux/types.h>

extern
int __vdso_clock_gettime(clockid_t clock, struct __kernel_timespec *ts);
int __vdso_clock_gettime(clockid_t clock, struct __kernel_timespec *ts)
{
	return __cvdso_clock_gettime(clock, ts);
}

extern
int __vdso_gettimeofday(struct __kernel_old_timeval *tv, struct timezone *tz);
int __vdso_gettimeofday(struct __kernel_old_timeval *tv, struct timezone *tz)
{
	return __cvdso_gettimeofday(tv, tz);
}

extern
int __vdso_clock_getres(clockid_t clock_id, struct __kernel_timespec *res);
int __vdso_clock_getres(clockid_t clock_id, struct __kernel_timespec *res)
{
	return __cvdso_clock_getres(clock_id, res);
}


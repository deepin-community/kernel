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

#include <asm/unistd.h>
#include <asm/vdso.h>
#include <asm/csr.h>
#include <asm/hmcall.h>

static __always_inline int syscall_fallback(clockid_t clkid, struct timespec64 *ts)
{
	register int r0 asm("$0");
	register unsigned long r19 asm("$19");
	asm volatile(
	"	mov		%0, $16\n"
	"	mov		%1, $17\n"
	"	ldi		$0, %2\n"
	"	sys_call	%3\n"
	:: "r"(clkid), "r"(ts), "i"(__NR_clock_gettime), "i"(HMC_callsys)
	: "$0", "$16", "$17", "$19");
	if (unlikely(r19))
		return -r0;
	else
		return r0;
}

static __always_inline int do_realtime_coarse(struct timespec64 *ts,
		const struct vdso_data *data)
{
	u32 start_seq;

	do {
		start_seq = vdso_data_read_begin(data);

		ts->tv_sec = data->xtime_sec;
		ts->tv_nsec = data->xtime_nsec >> data->cs_shift;
	} while (vdso_data_read_retry(data, start_seq));

	return 0;
}


static __always_inline int do_monotonic_coarse(struct timespec64 *ts,
		const struct vdso_data *data)
{
	u32 start_seq;
	u64 to_mono_sec;
	u64 to_mono_nsec;

	do {
		start_seq = vdso_data_read_begin(data);

		ts->tv_sec = data->xtime_sec;
		ts->tv_nsec = data->xtime_nsec >> data->cs_shift;

		to_mono_sec = data->wall_to_mono_sec;
		to_mono_nsec = data->wall_to_mono_nsec;
	} while (vdso_data_read_retry(data, start_seq));

	ts->tv_sec += to_mono_sec;
	timespec64_add_ns(ts, to_mono_nsec);

	return 0;
}

#if defined(CONFIG_SUBARCH_C3B)
static __always_inline u64 read_longtime(void)
{
	register unsigned long __r0 __asm__("$0");

	__asm__ __volatile__(
		"sys_call %1" : "=r"(__r0) : "i" (HMC_longtime));

	return __r0;
}
#elif defined(CONFIG_SUBARCH_C4)
static __always_inline u64 read_longtime(void)
{
	return read_csr(CSR_SHTCLOCK);
}
#endif

static __always_inline u64 get_ns(const struct vdso_data *data)
{
	u64 cycle_now, delta, nsec;

	cycle_now = read_longtime();
	delta = (cycle_now - data->cs_cycle_last) & data->cs_mask;

	nsec = (delta * data->cs_mult) + data->xtime_nsec;
	nsec >>= data->cs_shift;

	return nsec;
}


static __always_inline int do_realtime(struct timespec64 *ts,
		const struct vdso_data *data)
{
	u32 start_seq;
	u64 ns;

	do {
		start_seq = vdso_data_read_begin(data);

		ts->tv_sec = data->xtime_sec;
		ns = get_ns(data);
	} while (vdso_data_read_retry(data, start_seq));

	ts->tv_nsec = 0;
	timespec64_add_ns(ts, ns);

	return 0;
}

static __always_inline int do_monotonic(struct timespec64 *ts,
		const struct vdso_data *data)
{
	u32 start_seq;
	u64 ns;
	u64 to_mono_sec;
	u64 to_mono_nsec;

	do {
		start_seq = vdso_data_read_begin(data);

		ts->tv_sec = data->xtime_sec;
		ns = get_ns(data);

		to_mono_sec = data->wall_to_mono_sec;
		to_mono_nsec = data->wall_to_mono_nsec;
	} while (vdso_data_read_retry(data, start_seq));

	ts->tv_sec += to_mono_sec;
	ts->tv_nsec = 0;
	timespec64_add_ns(ts, ns + to_mono_nsec);

	return 0;
}


int __vdso_gettimeofday(struct __kernel_old_timeval *tv, struct timezone *tz)
{
	const struct vdso_data *data = get_vdso_data();
	struct timespec64 ts;
	int ret;

	ret = do_realtime(&ts, data);
	if (ret)
		return ret;

	if (tv) {
		tv->tv_sec = ts.tv_sec;
		tv->tv_usec = ts.tv_nsec / 1000;
	}

	if (tz) {
		tz->tz_minuteswest = data->tz_minuteswest;
		tz->tz_dsttime = data->tz_dsttime;
	}

	return 0;
}

int __vdso_clock_gettime(clockid_t clkid, struct timespec64 *ts)
{
	const struct vdso_data *data = get_vdso_data();
	int ret;

	switch (clkid) {
	case CLOCK_REALTIME_COARSE:
		ret = do_realtime_coarse(ts, data);
		break;
	case CLOCK_MONOTONIC_COARSE:
		ret = do_monotonic_coarse(ts, data);
		break;
	case CLOCK_REALTIME:
		ret = do_realtime(ts, data);
		break;
	case CLOCK_MONOTONIC:
		ret = do_monotonic(ts, data);
		break;
	default:
		/* fall back to a syscall */
		ret = syscall_fallback(clkid, ts);
	}

	return ret;
}

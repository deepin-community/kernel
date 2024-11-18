/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_VDSO_GETTIMEOFDAY_H
#define __ASM_VDSO_GETTIMEOFDAY_H

#ifndef __ASSEMBLY__

#include <asm/barrier.h>
#include <asm/unistd.h>
#include <asm/csr.h>
#include <uapi/linux/time.h>
#include <asm/hmcall.h>
#include <linux/kernel.h>
#define VDSO_HAS_CLOCK_GETRES	1

static __always_inline
int gettimeofday_fallback(struct __kernel_old_timeval *_tv,
			  struct timezone *_tz)
{
	long retval;
	long error;
	asm volatile(
	"	mov		%2, $16\n"
	"	mov		%3, $17\n"
	"	ldi		$0, %4\n"
	"	sys_call	%5\n"
	"	mov		$0, %0\n"
	"	mov		$19, %1"
	: "=r"(retval), "=r"(error)
	: "r"(_tv), "r"(_tz), "i"(__NR_gettimeofday), "i"(HMC_callsys)
	: "$0", "$16", "$17", "$19");
	if (unlikely(error))
		return -retval;
	else
		return retval;
}

static __always_inline
long clock_gettime_fallback(clockid_t _clkid, struct __kernel_timespec *_ts)
{
	long retval;
	long error;
	asm volatile(
	"	mov		%2, $16\n"
	"	mov		%3, $17\n"
	"	ldi		$0, %4\n"
	"	sys_call	%5\n"
	"	mov		$0, %0\n"
	"	mov		$19, %1"
	: "=r"(retval), "=r"(error)
	: "r"(_clkid), "r"(_ts), "i"(__NR_clock_gettime), "i"(HMC_callsys)
	: "$0", "$16", "$17", "$19");
	if (unlikely(error))
		return -retval;
	else
		return retval;
}

static __always_inline
int clock_getres_fallback(clockid_t _clkid, struct __kernel_timespec *_ts)
{
	long retval;
	long error;
	asm volatile(
	"	mov		%2, $16\n"
	"	mov		%3, $17\n"
	"	ldi		$0, %4\n"
	"	sys_call	%5\n"
	"	mov		$0, %0\n"
	"	mov		$19, %1"
	: "=r"(retval), "=r"(error)
	: "r"(_clkid), "r"(_ts), "i"(__NR_clock_getres), "i"(HMC_callsys)
	: "$0", "$16", "$17", "$19");
	if (unlikely(error))
		return -retval;
	else
		return retval;
}

#if defined(CONFIG_SUBARCH_C3B)
static __always_inline u64 __arch_get_hw_counter(s32 clock_mode,
						 const struct vdso_data *vd)
{
	register unsigned long __r0 __asm__("$0");

	__asm__ __volatile__(
		"sys_call %1" : "=r"(__r0) : "i" (HMC_longtime));

	return __r0;
}
#elif defined(CONFIG_SUBARCH_C4)
static __always_inline u64 __arch_get_hw_counter(s32 clock_mode,
						 const struct vdso_data *vd)
{
	return sw64_read_csr(CSR_SHTCLOCK);
}
#endif

static __always_inline const struct vdso_data *__arch_get_vdso_data(void)
{
	return _vdso_data;
}

#endif /* !__ASSEMBLY__ */

#endif /* __ASM_VDSO_GETTIMEOFDAY_H */

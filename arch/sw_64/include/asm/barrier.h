/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_BARRIER_H
#define _ASM_SW64_BARRIER_H

#include <asm/compiler.h>

#define mb()	__asm__ __volatile__("memb" : : : "memory")

#define rmb()	__asm__ __volatile__("memb" : : : "memory")

#if defined(CONFIG_SUBARCH_C3B)
#define wmb()	__asm__ __volatile__("memb" : : : "memory")
#elif defined(CONFIG_SUBARCH_C4)
#define wmb()	__asm__ __volatile__("wmemb" : : : "memory")
#endif

#define imemb()	__asm__ __volatile__("imemb" : : : "memory")

#ifdef CONFIG_SMP
#define __ASM_SMP_MB	"\tmemb\n"
#else
#define __ASM_SMP_MB
#endif

#define __smp_mb__before_atomic()	barrier()
#define __smp_mb__after_atomic()	barrier()

#include <asm-generic/barrier.h>

#endif /* _ASM_SW64_BARRIER_H */

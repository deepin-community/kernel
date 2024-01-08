/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_SW64_COMPILER_H
#define _UAPI_ASM_SW64_COMPILER_H

/*
 * Herein are macros we use when describing various patterns we want to GCC.
 * In all cases we can get better schedules out of the compiler if we hide
 * as little as possible inside inline assembly.  However, we want to be
 * able to know what we'll get out before giving up inline assembly.  Thus
 * these tests and macros.
 */

#define __kernel_inslb(val, shift)					\
({									\
	unsigned long __kir;						\
	__asm__("inslb %2, %1, %0" : "=r"(__kir) : "rI"(shift), "r"(val));\
	__kir;								\
})

#define __kernel_inslh(val, shift)					\
({									\
	unsigned long __kir;						\
	__asm__("inslh %2, %1, %0" : "=r"(__kir) : "rI"(shift), "r"(val));\
	__kir;								\
})

#define __kernel_insll(val, shift)					\
({									\
	unsigned long __kir;						\
	__asm__("insll %2, %1, %0" : "=r"(__kir) : "rI"(shift), "r"(val));\
	__kir;								\
})

#define __kernel_inshw(val, shift)					\
({									\
	unsigned long __kir;						\
	__asm__("inshw %2, %1, %0" : "=r"(__kir) : "rI"(shift), "r"(val));\
	__kir;								\
})

#define __kernel_extlb(val, shift)					\
({									\
	unsigned long __kir;						\
	__asm__("extlb %2, %1, %0" : "=r"(__kir) : "rI"(shift), "r"(val));\
	__kir;								\
})

#define __kernel_extlh(val, shift)					\
({									\
	unsigned long __kir;						\
	__asm__("extlh %2, %1, %0" : "=r"(__kir) : "rI"(shift), "r"(val));\
	__kir;								\
})

#define __kernel_cmpgeb(a, b)						\
({									\
	unsigned long __kir;						\
	__asm__("cmpgeb %r2, %1, %0" : "=r"(__kir) : "rI"(b), "rJ"(a));	\
	__kir;								\
})

#define __kernel_cttz(x)						\
({									\
	unsigned long __kir;						\
	__asm__("cttz %1, %0" : "=r"(__kir) : "r"(x));			\
	 __kir;								\
})

#define __kernel_ctlz(x)						\
({									\
	unsigned long __kir;						\
	__asm__("ctlz %1, %0" : "=r"(__kir) : "r"(x));			\
	__kir;								\
})

#define __kernel_ctpop(x)						\
({									\
	unsigned long __kir;						\
	__asm__("ctpop %1, %0" : "=r"(__kir) : "r"(x));			\
	__kir;								\
})

#endif /* _UAPI_ASM_SW64_COMPILER_H */

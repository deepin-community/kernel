/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_CMPXCHG_H
#define _ASM_SW64_CMPXCHG_H

/*
 * Atomic exchange routines.
 */

#define __ASM__MB
#define ____xchg(type, args...)		__arch_xchg ## type ## _local(args)
#define ____cmpxchg(type, args...)	__cmpxchg ## type ## _local(args)
#include <asm/xchg.h>

#define arch_xchg_local(ptr, x)						\
({									\
	__typeof__(*(ptr)) _x_ = (x);					\
	(__typeof__(*(ptr))) __arch_xchg_local((ptr), (unsigned long)_x_,	\
				       sizeof(*(ptr)));			\
})

#define arch_cmpxchg_local(ptr, o, n)					\
({									\
	__typeof__(*(ptr)) _o_ = (o);					\
	__typeof__(*(ptr)) _n_ = (n);					\
	(__typeof__(*(ptr))) __cmpxchg_local((ptr), (unsigned long)_o_,	\
					  (unsigned long)_n_,		\
					  sizeof(*(ptr)));		\
})

#define arch_cmpxchg64_local(ptr, o, n)					\
({									\
	BUILD_BUG_ON(sizeof(*(ptr)) != 8);				\
	arch_cmpxchg_local((ptr), (o), (n));					\
})

#ifdef CONFIG_SMP
#undef __ASM__MB
#define __ASM__MB	"\tmemb\n"
#endif
#undef ____xchg
#undef ____cmpxchg
#undef _ASM_SW64_XCHG_H
#define ____xchg(type, args...)		__arch_xchg ##type(args)
#define ____cmpxchg(type, args...)	__cmpxchg ##type(args)
#include <asm/xchg.h>

#define arch_xchg(ptr, x)							\
({									\
	__typeof__(*(ptr)) _x_ = (x);					\
	(__typeof__(*(ptr))) __arch_xchg((ptr), (unsigned long)_x_,		\
				 sizeof(*(ptr)));			\
})

#define arch_cmpxchg(ptr, o, n)						\
({									\
	__typeof__(*(ptr)) _o_ = (o);					\
	__typeof__(*(ptr)) _n_ = (n);					\
	(__typeof__(*(ptr))) __cmpxchg((ptr), (unsigned long)_o_,	\
				    (unsigned long)_n_,	sizeof(*(ptr)));\
})

#define arch_cmpxchg64(ptr, o, n)						\
({									\
	BUILD_BUG_ON(sizeof(*(ptr)) != 8);				\
	arch_cmpxchg((ptr), (o), (n));					\
})

#undef __ASM__MB
#undef ____cmpxchg

#define __HAVE_ARCH_CMPXCHG 1

#endif /* _ASM_SW64_CMPXCHG_H */

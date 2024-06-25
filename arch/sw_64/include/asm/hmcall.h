/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_HMCALL_H
#define _ASM_SW64_HMCALL_H

/*
 * Common HMC-code
 */
/* 0x0  - 0x3F : Kernel Level HMC routine */
#define HMC_halt		0x00
#define HMC_rdio64		0x01
#define HMC_rdio32		0x02
#define HMC_cpuid		0x03
#define HMC_sleepen		0x05
#define HMC_rdksp		0x06
#define HMC_wrasid		0x08
#define HMC_rdktp		0x09
#define HMC_wrktp		0x0A
#define HMC_rdptbr		0x0B
#define HMC_wrptbr		0x0C
#define HMC_rdhtctl		0x0D
#define HMC_wrksp		0x0E
#define HMC_mtinten		0x0F
#define HMC_wrap_asid		0x10
#define HMC_load_mm		0x11
#define HMC_tbisasid		0x14
#define HMC_tbivpn		0x19
#define HMC_ret			0x1A
#define HMC_wrvpcr		0x29
#define HMC_wrfen		0x2B
#define HMC_sflush		0x2F
#define HMC_entervm		0x31
#define HMC_hcall		0x32
#define HMC_tbi			0x33
#define HMC_wrent		0x34
#define HMC_swpipl		0x35
#define HMC_rdps		0x36
#define HMC_wrkgp		0x37
#define HMC_wrusp		0x38
#define HMC_rvpcr		0x39
#define HMC_rdusp		0x3A
#define HMC_wrtimer		0x3B
#define HMC_whami		0x3C
#define HMC_retsys		0x3D
#define HMC_sendii		0x3E
#define HMC_rti			0x3F

/* 0x40  - 0x7F : Hypervisor Level HMC routine */
#define HMC_rti_nmi             0x40
#define HMC_setup_nmi           0x41

/* 0x80  - 0xBF : User Level HMC routine */
#include <uapi/asm/hmcall.h>

/* Following will be deprecated from user level invocation */
#define HMC_rwreg		0x87
#define HMC_sz_uflush		0xA8
#define HMC_uwhami		0xA0
#define HMC_longtime		0xB1

#ifdef __KERNEL__
#ifndef __ASSEMBLY__

#include <linux/init.h>
extern void __init fixup_hmcall(void);

extern void halt(void) __noreturn;

#define __CALL_HMC_VOID(NAME)					\
static inline void NAME(void)					\
{								\
	__asm__ __volatile__(					\
		"sys_call %0 " : : "i" (HMC_ ## NAME));		\
}

#define __CALL_HMC_R0(NAME, TYPE)				\
static inline TYPE NAME(void)					\
{								\
	register TYPE __r0 __asm__("$0");			\
	__asm__ __volatile__(					\
		"sys_call %1 # " #NAME				\
		: "=r" (__r0)					\
		: "i" (HMC_ ## NAME)				\
		: "$1", "$16", "$22", "$23", "$24", "$25");	\
	return __r0;						\
}

#define __CALL_HMC_W1(NAME, TYPE0)				\
static inline void NAME(TYPE0 arg0)				\
{								\
	register TYPE0 __r16 __asm__("$16") = arg0;		\
	__asm__ __volatile__(					\
		"sys_call %1 # "#NAME				\
		: "=r"(__r16)					\
		: "i"(HMC_ ## NAME), "0"(__r16)			\
		: "$1", "$22", "$23", "$24", "$25");		\
}

#define __CALL_HMC_W2(NAME, TYPE0, TYPE1)			\
static inline void NAME(TYPE0 arg0, TYPE1 arg1)			\
{								\
	register TYPE0 __r16 __asm__("$16") = arg0;		\
	register TYPE1 __r17 __asm__("$17") = arg1;		\
	__asm__ __volatile__(					\
		"sys_call %2 # "#NAME				\
		: "=r"(__r16), "=r"(__r17)			\
		: "i"(HMC_ ## NAME), "0"(__r16), "1"(__r17)	\
		: "$1", "$22", "$23", "$24", "$25");		\
}

#define __CALL_HMC_RW1(NAME, RTYPE, TYPE0)			\
static inline RTYPE NAME(TYPE0 arg0)				\
{								\
	register RTYPE __r0 __asm__("$0");			\
	register TYPE0 __r16 __asm__("$16") = arg0;		\
	__asm__ __volatile__(					\
		"sys_call %2 # "#NAME				\
		: "=r"(__r16), "=r"(__r0)			\
		: "i"(HMC_ ## NAME), "0"(__r16)			\
		: "$1", "$22", "$23", "$24", "$25");		\
	return __r0;						\
}

#define __CALL_HMC_RW2(NAME, RTYPE, TYPE0, TYPE1)		\
static inline RTYPE NAME(TYPE0 arg0, TYPE1 arg1)		\
{								\
	register RTYPE __r0 __asm__("$0");			\
	register TYPE0 __r16 __asm__("$16") = arg0;		\
	register TYPE1 __r17 __asm__("$17") = arg1;		\
	__asm__ __volatile__(					\
		"sys_call %3 # "#NAME				\
		: "=r"(__r16), "=r"(__r17), "=r"(__r0)		\
		: "i"(HMC_ ## NAME), "0"(__r16), "1"(__r17)	\
		: "$1", "$22", "$23", "$24", "$25");		\
	return __r0;						\
}

#define __CALL_HMC_RW3(NAME, RTYPE, TYPE0, TYPE1, TYPE2)		\
static inline RTYPE NAME(TYPE0 arg0, TYPE1 arg1, TYPE2 arg2)		\
{									\
	register RTYPE __r0 __asm__("$0");				\
	register TYPE0 __r16 __asm__("$16") = arg0;			\
	register TYPE1 __r17 __asm__("$17") = arg1;			\
	register TYPE2 __r18 __asm__("$18") = arg2;			\
	__asm__ __volatile__(						\
		"sys_call %4 # "#NAME					\
		: "=r"(__r16), "=r"(__r17), "=r"(__r18), "=r"(__r0)	\
		: "i"(HMC_ ## NAME), "0"(__r16), "1"(__r17), "2"(__r18)	\
		: "$1", "$22", "$23", "$24", "$25");			\
	return __r0;							\
}


__CALL_HMC_VOID(imb);
__CALL_HMC_VOID(sflush);
__CALL_HMC_VOID(wrfen);
#define fpu_enable()	wrfen()

__CALL_HMC_VOID(sleepen);
__CALL_HMC_VOID(mtinten);

__CALL_HMC_VOID(rdktp);
__CALL_HMC_VOID(wrktp);

__CALL_HMC_R0(rdps, unsigned long);
__CALL_HMC_R0(rvpcr, unsigned long);
__CALL_HMC_R0(uwhami, unsigned long);

__CALL_HMC_R0(rdusp, unsigned long);
__CALL_HMC_W1(wrusp, unsigned long);

__CALL_HMC_R0(rdksp, unsigned long);
__CALL_HMC_W1(wrksp, unsigned long);
__CALL_HMC_R0(rdhtctl, unsigned long);

/*
 * Load a mm context. This is needed when we change the page
 * table pointer(CSR:PTBR) or when we update the ASID.
 * load_mm(asid, ptbr)
 *
 */
__CALL_HMC_W2(load_mm, unsigned long, unsigned long);

__CALL_HMC_W1(wrasid, unsigned long);
__CALL_HMC_R0(rdptbr, unsigned long);
__CALL_HMC_W1(wrptbr, unsigned long);

__CALL_HMC_RW1(swpipl, unsigned long, unsigned long);
__CALL_HMC_R0(whami, unsigned long);
__CALL_HMC_RW1(rdio64, unsigned long, unsigned long);
__CALL_HMC_RW1(rdio32, unsigned int, unsigned long);
__CALL_HMC_W2(wrent, void*, unsigned long);
__CALL_HMC_W2(tbisasid, unsigned long, unsigned long);
__CALL_HMC_W2(setup_nmi, unsigned long, unsigned long);
__CALL_HMC_W1(wrkgp, unsigned long);
__CALL_HMC_RW2(wrperfmon, unsigned long, unsigned long, unsigned long);
__CALL_HMC_RW3(sendii, unsigned long, unsigned long, unsigned long, unsigned long);
__CALL_HMC_W1(wrtimer, unsigned long);
__CALL_HMC_RW3(tbivpn, unsigned long, unsigned long, unsigned long, unsigned long);
__CALL_HMC_RW2(cpuid, unsigned long, unsigned long, unsigned long);

__CALL_HMC_W1(wrtp, unsigned long);
/*
 * TB routines..
 */
#define __tbi(nr, arg, arg1...)					\
({								\
	register unsigned long __r16 __asm__("$16") = (nr);	\
	register unsigned long __r17 __asm__("$17"); arg;	\
	__asm__ __volatile__(					\
		"sys_call %3 #__tbi"				\
		: "=r" (__r16), "=r" (__r17)			\
		: "0" (__r16), "i" (HMC_tbi), ##arg1		\
		: "$0", "$1", "$22", "$23", "$24", "$25");	\
})

#define tbi(x, y)	__tbi(x, __r17 = (y), "1" (__r17))

/* Invalidate all TLB, only used by hypervisor */
#define tbia()		__tbi(-2, /* no second argument */)

/* Invalidate TLB for all processes with currnet VPN */
#define tbivp()		__tbi(-1, /* no second argument */)

/* Invalidate all TLB with current VPN */
#define tbiv()		__tbi(0, /* no second argument */)

/* Invalidate ITLB of addr with current UPN and VPN */
#define tbisi(addr)	__tbi(1, __r17 = (addr), "1" (__r17))

/* Invalidate DTLB of addr with current UPN and VPN */
#define tbisd(addr)	__tbi(2, __r17 = (addr), "1" (__r17))

/* Invalidate TLB of addr with current UPN and VPN */
#define tbis(addr)	__tbi(3, __r17 = (addr), "1" (__r17))

/* Invalidate all user TLB with current UPN and VPN */
#define tbiu()		__tbi(4, /* no second argument */)

#if defined(CONFIG_SUBARCH_C4)
__CALL_HMC_W2(wrap_asid, unsigned long, unsigned long);
static inline void save_ktp(void)
{
	__asm__ __volatile__("csrw $8, 0xef");
}
#elif defined(CONFIG_SUBARCH_C3B)
static inline void wrap_asid(unsigned long asid, unsigned long ptbr)
{
	tbivp();
}
#define save_ktp()     wrktp()
#endif

#define set_nmi(irq)	setup_nmi(1, (irq))
#define clear_nmi(irq)	setup_nmi(0, (irq))

#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */

#endif /* _ASM_SW64_HMCALL_H */

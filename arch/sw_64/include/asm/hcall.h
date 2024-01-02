/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_HCALL_H
#define _ASM_SW64_HCALL_H

#define HMC_hcall	0x32
/* HCALL must > 0 */
enum HCALL_TYPE {
	HCALL_HALT		= 10,
	HCALL_NOTIFY		= 11,
	HCALL_SHUTDOWN		= 12,
	HCALL_SET_CLOCKEVENT	= 13,
	HCALL_IVI		= 14,   /* interrupt between virtual cpu */
	HCALL_TBI		= 15,   /* tlb flush for virtual cpu */
	HCALL_STOP		= 16,   /* indicate virtual cpu stopped */
	HCALL_RESTART		= 17,	/* indicate virtual cpu restarted */
	HCALL_MSI		= 18,   /* guest request msi intr */
	HCALL_MSIX		= 19,	/* guest request msix intr */
	HCALL_SWNET		= 20,   /* guest request swnet service */
	HCALL_SWNET_IRQ		= 21,   /* guest request swnet intr */
	HCALL_FATAL_ERROR	= 22,   /* guest fatal error, issued by hmcode */
	NR_HCALL
};

static inline unsigned long hcall(unsigned long hcall, unsigned long arg0,
				  unsigned long arg1, unsigned long arg2)
{
	register unsigned long __r0 __asm__("$0");
	register unsigned long __r16 __asm__("$16") = hcall;
	register unsigned long __r17 __asm__("$17") = arg0;
	register unsigned long __r18 __asm__("$18") = arg1;
	register unsigned long __r19 __asm__("$19") = arg2;
	__asm__ __volatile__(
		"sys_call %5 "
		: "=r"(__r16), "=r"(__r17), "=r"(__r18), "=r"(__r19), "=r"(__r0)
		: "i"(HMC_hcall), "0"(__r16), "1"(__r17), "2"(__r18), "3"(__r19)
		: "$1", "$22", "$23", "$24", "$25");
	return __r0;
}

#endif /* _ASM_SW64_HCALL_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_KVM_PARA_H
#define _ASM_SW64_KVM_PARA_H

#include <uapi/asm/kvm_para.h>

#define HMC_hcall 0x32

static inline unsigned long kvm_hypercall3(unsigned long num,
					   unsigned long arg0,
					   unsigned long arg1,
					   unsigned long arg2)
{
	register unsigned long __r0 __asm__("$0");
	register unsigned long __r16 __asm__("$16") = num;
	register unsigned long __r17 __asm__("$17") = arg0;
	register unsigned long __r18 __asm__("$18") = arg1;
	register unsigned long __r19 __asm__("$19") = arg2;
	__asm__ __volatile__(
		"sys_call %5"
		: "=r"(__r16), "=r"(__r17), "=r"(__r18), "=r"(__r19), "=r"(__r0)
		: "i"(HMC_hcall), "0"(__r16), "1"(__r17), "2"(__r18), "3"(__r19)
		: "$1", "$22", "$23", "$24", "$25");
	return __r0;
}
#endif /* _ASM_SW64_KVM_PARA_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_SUSPEND_H
#define _ASM_SW64_SUSPEND_H

#include <asm/hmcall.h>
#include <asm/ptrace.h>
#include <asm/processor.h>
#define SOFTINF_SLEEP_MAGIC 0x0123456789ABCDEFUL

#ifdef CONFIG_HIBERNATION
#include <asm/vcpu.h>
#include <asm/thread_info.h>
#endif

struct callee_saved_regs {
	unsigned long r9;
	unsigned long r10;
	unsigned long r11;
	unsigned long r12;
	unsigned long r13;
	unsigned long r14;
	unsigned long r15;
	unsigned long ra;
};

struct callee_saved_fpregs {
	unsigned long f2[4];
	unsigned long f3[4];
	unsigned long f4[4];
	unsigned long f5[4];
	unsigned long f6[4];
	unsigned long f7[4];
	unsigned long f8[4];
	unsigned long f9[4];
} __aligned(32);	/* 256 bits aligned for simd */

struct processor_state {
	struct callee_saved_regs regs;
	struct callee_saved_fpregs fpregs;
	unsigned long fpcr;
	unsigned long ktp;
#ifdef CONFIG_HIBERNATION
	unsigned long sp;
	struct vcpucb vcb;
#endif
};

extern void sw64_suspend_deep_sleep(struct processor_state *state);
extern const struct platform_suspend_ops native_suspend_ops;
#endif /* _ASM_SW64_SUSPEND_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_PTRACE_H
#define _ASM_SW64_PTRACE_H

#include <uapi/asm/ptrace.h>
#include <asm/hmcall.h>
#include <asm/page.h>

#define NO_SYSCALL	_AC(-1, UL)

#ifdef __KERNEL__
#ifndef __ASSEMBLY__

/*
 * This struct defines the way the registers are stored on the
 * kernel stack during a system call or other kernel entry
 */

struct pt_regs {
	union {
		struct user_pt_regs user_regs;
		struct {
			unsigned long regs[31];
			unsigned long pc;
			unsigned long ps;
		};
	};
	unsigned long orig_r0;
	unsigned long orig_r19;
	unsigned long cause;
	/* These are saved by HMcode: */
	unsigned long hm_ps;
	unsigned long hm_pc;
	unsigned long hm_gp;
	unsigned long earg0;
	unsigned long earg1;
	unsigned long earg2;
} __aligned(16);

#define arch_has_single_step()		(1)
#define user_mode(regs) (((regs)->ps & 8) != 0)
#define instruction_pointer(regs) ((regs)->pc)
#define profile_pc(regs) instruction_pointer(regs)
#define user_stack_pointer(pt_regs) ((pt_regs)->regs[30])
#define kernel_stack_pointer(regs) ((unsigned long)((regs) + 1))
#define instruction_pointer_set(regs, val) ((regs)->pc = val)

#define force_successful_syscall_return() (current_pt_regs()->orig_r0 = NO_SYSCALL)

#define MAX_REG_OFFSET (offsetof(struct pt_regs, orig_r0))

extern short regoffsets[];

extern unsigned long syscall_trace_enter(void);
extern void syscall_trace_leave(void);

/**
 * regs_get_register() - get register value from its offset
 * @regs:       pt_regs from which register value is gotten
 * @offset:     offset of the register.
 *
 * regs_get_register returns the value of a register whose offset from @regs.
 * The @offset is the offset of the register in struct pt_regs.
 * If @offset is bigger than MAX_REG_OFFSET, this returns 0.
 */
static inline u64 regs_get_register(struct pt_regs *regs, unsigned int offset)
{
	if (unlikely(offset > MAX_REG_OFFSET))
		return 0;

	return *(unsigned long *)((unsigned long)regs + offset);
}
extern int regs_query_register_offset(const char *name);
extern unsigned long regs_get_kernel_stack_nth(struct pt_regs *regs,
					       unsigned int n);

static inline int is_syscall_success(struct pt_regs *regs)
{
	return !regs->regs[19];
}

static inline long regs_return_value(struct pt_regs *regs)
{
	if ((regs->orig_r0 == NO_SYSCALL) || is_syscall_success(regs))
		return regs->regs[0];
	else
		return -regs->regs[0];
}

#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */

#endif /* _ASM_SW64_PTRACE_H */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/ibt.h>
#include <asm/percpu.h>
#include <asm/processor-flags.h>
#include <asm/asm.h>

#define X86_CR4_SMEP_SMAP (X86_CR4_SMEP | X86_CR4_SMAP)

/*
 * scratch_reg would be changed,
 * caller should dertimine if scratch_reg should be saved and restored.
 */
.macro DISABLE_WP scratch_reg:req
	/* Disable write protection*/
	movq %cr0, %\scratch_reg
	andq $(~X86_CR0_WP), %\scratch_reg
	movq %\scratch_reg, %cr0
.endm

.macro ENABLE_WP scratch_reg:req
	/* Enable write protection */
	movq %cr0, %\scratch_reg
1:
	orq $X86_CR0_WP, %\scratch_reg
	movq %\scratch_reg, %cr0
	testq $X86_CR0_WP, %\scratch_reg
	je 1b
.endm

/*
 * IEE memory access gate.
 * Kernel calls the gate to modify IEE-protected memory.
 *
 * The gates switch to the per-cpu IEE stack in the middle of the
 * function. objtool cannot model that: %rsp is untraceable after the
 * per-cpu stack load, so no provable frame pointer state exists at the
 * call sites (the only modelled stack-switch idiom yields CFI_SP_INDIRECT,
 * which also fails has_valid_stack_frame()). Declare the gates as code
 * (not function) symbols to exempt them from stack validation, the same
 * way x86 entry code handles mid-function stack switches, and maintain a
 * real %rbp frame chain manually so the frame pointer unwinder still
 * works at runtime.
 */

SYM_CODE_START(iee_rw_gate)
	ENDBR

	/* save Interrupt flag */
	pushfq
	/* close irq*/
	cli

	pushq %r12

	DISABLE_WP r12

	/* set up a real frame for the frame pointer unwinder */
	pushq %rbp
	movq %rsp, %rbp

	/* switch to iee stack */
	movq %rsp, %r12
	movq PER_CPU_VAR(iee_stacks) + IEE_STACK, %rsp

	/* direct dispatch: no indirect call, no writable func table */
	call iee_dispatch

	/* switch to kernel stack */
	movq %r12, %rsp

	popq %rbp

	ENABLE_WP r12

	popq %r12

	/* restore irq*/
	popfq

	jmp __x86_return_thunk  /* ret */
SYM_CODE_END(iee_rw_gate)
EXPORT_SYMBOL(iee_rw_gate)

#ifdef CONFIG_IEE_SIP
SYM_CODE_START(iee_rwx_gate)
	ENDBR

	pushq %r12
 
	/* save Interrupt flag*/
	pushfq
	/* close irq */
	cli
 
	/* set SMEP=0 to enable supervisor-mode exec user-mode insn */
	movq %cr4, %rax  /* rax -> cr4 */
	andq $(~X86_CR4_SMEP), %rax
	movq %rax, %cr4
 
	DISABLE_WP r12

	/* set up a real frame for the frame pointer unwinder */
	pushq %rbp
	movq %rsp, %rbp

	movq %rsp, %r12
	/* If iee hasn't been initialized, skip stack switch. */
	cmpb $0, iee_init_done(%rip)
	jz 2f

	/* switch to iee stack */
	movq PER_CPU_VAR(iee_stacks) + IEE_STACK, %rsp

2:  call _iee_si_handler

	/* switch to kernel stack. If iee hasn't been initialized, skip switch*/
	movq %r12, %rsp

	popq %rbp

	ENABLE_WP r12
 
	/* set SMEP=1 to disable supervisor-mode exec user-mode insn */
	movq %cr4, %rax  /* rax -> cr4 */
1:  orq $X86_CR4_SMEP_SMAP, %rax
	movq %rax, %cr4
	andq $X86_CR4_SMEP_SMAP, %rax
	cmpq $X86_CR4_SMEP_SMAP, %rax
	jnz 1
 
	/* restore irq*/
	popfq
 
	popq %r12
	jmp __x86_return_thunk  /* ret */
SYM_CODE_END(iee_rwx_gate)
#endif
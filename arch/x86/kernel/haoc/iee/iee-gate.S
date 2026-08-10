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
 */

SYM_FUNC_START(iee_rw_gate)
	/* save Interrupt flag */
	pushfq
	/* close irq*/
	cli

	pushq %r12

	DISABLE_WP r12

	/* switch to iee stack */
	movq %rsp, %r12
	movq PER_CPU_VAR(iee_stacks) + IEE_STACK, %rsp

	/* direct dispatch: no indirect call, no writable func table */
	call iee_dispatch

	/* switch to kernel stack */
	movq %r12, %rsp

	ENABLE_WP r12

	popq %r12

	/* restore irq*/
	popfq

	jmp __x86_return_thunk  /* ret */
SYM_FUNC_END(iee_rw_gate)
EXPORT_SYMBOL(iee_rw_gate)

#ifdef CONFIG_IEE_SIP
SYM_FUNC_START(iee_rwx_gate)
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
 
	movq %rsp, %r12
	/* If iee hasn't been initialized, skip stack switch. */
	cmpb $0, iee_init_done(%rip)
	jz 2f
 
	/* switch to iee stack */
	movq PER_CPU_VAR(iee_stacks) + IEE_STACK, %rsp
 
2:  call _iee_si_handler
	/* switch to kernel stack. If iee hasn't been initialized, skip switch*/
	movq %r12, %rsp
 
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
SYM_FUNC_END(iee_rwx_gate)
#endif
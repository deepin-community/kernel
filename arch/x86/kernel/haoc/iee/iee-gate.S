/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/percpu.h>
#include <asm/processor-flags.h>
#include <asm/asm.h>

#define X86_CR4_SMEP_SMAP (X86_CR4_SMEP | X86_CR4_SMAP)

/*
 * scratch_reg is clobbered, so callers must decide whether it should be
 * saved and restored around these macros.
 */
.macro DISABLE_WP scratch_reg:req
	/* Disable write protection. */
	movq %cr0, %\scratch_reg
	andq $(~X86_CR0_WP), %\scratch_reg
	movq %\scratch_reg, %cr0
.endm

.macro ENABLE_WP scratch_reg:req
	/* Enable write protection. */
	movq %cr0, %\scratch_reg
1:
	orq $X86_CR0_WP, %\scratch_reg
	movq %\scratch_reg, %cr0
	testq $X86_CR0_WP, %\scratch_reg
	je 1b
.endm

SYM_FUNC_START(iee_rw_gate)
	cmpb    $0, haoc_enabled(%rip)
	jnz     .L_haoc_enabled_rw

.L_haoc_disabled_rw:
	leaq iee_funcs(%rip), %rax
	call *(%rax, %rdi, 8)
	jmp __x86_return_thunk

.L_haoc_enabled_rw:
	/* save Interrupt flag */
	pushfq
	/* close irq */
	cli

	pushq %r12

	DISABLE_WP r12

	/* call iee func */
	leaq iee_funcs(%rip), %rax
	call *(%rax, %rdi, 8)

	ENABLE_WP r12

	popq %r12

	/* restore irq */
	popfq

	jmp __x86_return_thunk
SYM_FUNC_END(iee_rw_gate)
EXPORT_SYMBOL(iee_rw_gate)

#ifdef CONFIG_IEE_SIP
SYM_FUNC_START(iee_rwx_gate)
	cmpb    $0, haoc_enabled(%rip)
	jnz     .L_haoc_enabled_rwx

.L_haoc_disabled_rwx:
	call _iee_si_handler
	jmp __x86_return_thunk

.L_haoc_enabled_rwx:
	pushq %r12

	/* save Interrupt flag */
	pushfq
	/* close irq */
	cli

	/* set SMEP=0 to enable supervisor-mode exec user-mode insn */
	movq %cr4, %rax
	andq $(~X86_CR4_SMEP), %rax
	movq %rax, %cr4

	DISABLE_WP r12

	call _iee_si_handler

	ENABLE_WP r12

	/* set SMEP=1 to disable supervisor-mode exec user-mode insn */
	movq %cr4, %rax
1:	orq $X86_CR4_SMEP_SMAP, %rax
	movq %rax, %cr4
	andq $X86_CR4_SMEP_SMAP, %rax
	cmpq $X86_CR4_SMEP_SMAP, %rax
	jnz 1b

	/* restore irq */
	popfq

	popq %r12
	jmp __x86_return_thunk
SYM_FUNC_END(iee_rwx_gate)
#endif

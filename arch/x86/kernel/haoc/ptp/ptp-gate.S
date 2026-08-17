/* SPDX-License-Identifier: GPL-2.0 */
#include "../gate.h"

SYM_FUNC_START(ptp_xchg)
	cmpb    $0, haoc_enabled(%rip)
	jnz     .L_haoc_enabled_xchg

.L_haoc_disabled_xchg:
	xchg	%rsi, (%rdi)
	movq	%rsi, %rax
	jmp __x86_return_thunk

.L_haoc_enabled_xchg:
	/* save RFLAGS, close irq */
	pushfq
	cli

	pushq %r12

	movq PER_CPU_VAR(iee_cr0s), %r12
	testq %r12, %r12
	jz xchg_wp_enabled

	xchg	%rsi, (%rdi)
	movq	%rsi, %rax

	popq %r12
	popfq
	jmp __x86_return_thunk

xchg_wp_enabled:
	DISABLE_WP r12

	xchg	%rsi, (%rdi)
	movq	%rsi, %rax

	ENABLE_WP r12

	popq %r12

	/* restore RFLAGS*/
	popfq
	jmp __x86_return_thunk  /* ret */
SYM_FUNC_END(ptp_xchg)

SYM_FUNC_START(ptp_try_cmpxchg)
	cmpb    $0, haoc_enabled(%rip)
	jnz     .L_haoc_enabled_cmpxchg

.L_haoc_disabled_cmpxchg:
	movq %rsi, %rax
	lock cmpxchgq %rdx, (%rdi)
	jmp __x86_return_thunk

.L_haoc_enabled_cmpxchg:
	/* save RFLAGS, close irq */
	pushfq
	cli

	pushq %r12

	movq PER_CPU_VAR(iee_cr0s), %r12
	testq %r12, %r12
	jz cmpxchg_wp_enabled

	movq %rsi, %rax
	lock cmpxchgq %rdx, (%rdi)

	popq %r12
	popfq
	jmp __x86_return_thunk

cmpxchg_wp_enabled:
	DISABLE_WP r12

	movq %rsi, %rax
	lock cmpxchgq %rdx, (%rdi)

	ENABLE_WP r12

	popq %r12
	/* restore RFLAGS*/
	popfq
	jmp __x86_return_thunk  /* ret */
SYM_FUNC_END(ptp_try_cmpxchg)

SYM_FUNC_START(ptp_rw_gate)
	pushq %rbp
	movq %rsp, %rbp

	cmpb    $0, haoc_enabled(%rip)
	jnz     .L_haoc_enabled_rw

.L_haoc_disabled_rw:
	leaq iee_funcs(%rip), %rax
	call *(%rax, %rdi, 8)
	popq %rbp
	jmp __x86_return_thunk

.L_haoc_enabled_rw:
	/* save Interrupt flag */
	pushfq
	/* close irq*/
	cli

	pushq %r12

	movq PER_CPU_VAR(iee_cr0s), %r12
	testq %r12, %r12
	jz ptprw_wp_enabled

	leaq iee_funcs(%rip), %rax
	call *(%rax, %rdi, 8)

	popq %r12
	popfq
	popq %rbp
	jmp __x86_return_thunk

ptprw_wp_enabled:
	DISABLE_WP r12

	/* call iee func */
	leaq iee_funcs(%rip), %rax
	call *(%rax, %rdi, 8)

	ENABLE_WP r12

	popq %r12

	/* restore irq*/
	popfq

	popq %rbp
	jmp __x86_return_thunk  /* ret */
SYM_FUNC_END(ptp_rw_gate)
EXPORT_SYMBOL(ptp_rw_gate)

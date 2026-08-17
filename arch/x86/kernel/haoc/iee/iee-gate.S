/* SPDX-License-Identifier: GPL-2.0 */
#include "../gate.h"

SYM_FUNC_START(iee_rw_gate)
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

#ifdef CONFIG_PTP
	movq PER_CPU_VAR(iee_cr0s), %r12
	testq %r12, %r12
	jz rw_wp_enabled

	leaq iee_funcs(%rip), %rax
	call *(%rax, %rdi, 8)

	popq %r12
	popfq
	popq %rbp
	jmp __x86_return_thunk

rw_wp_enabled:
#endif
	DISABLE_WP r12

	/* call iee func */
	leaq iee_funcs(%rip), %rax
	call *(%rax, %rdi, 8)

	ENABLE_WP r12

	popq %r12

	/* restore irq*/
	popfq

	popq %rbp
	jmp __x86_return_thunk
SYM_FUNC_END(iee_rw_gate)
#if defined(CONFIG_CREDP) || defined(CONFIG_KEYP)
EXPORT_SYMBOL(iee_rw_gate)
#endif

#ifdef CONFIG_IEE_SIP
SYM_FUNC_START(iee_rwx_gate)
	pushq %rbp
	movq %rsp, %rbp

	cmpb    $0, haoc_enabled(%rip)
	jnz     .L_haoc_enabled_rwx

.L_haoc_disabled_rwx:
	call _iee_si_handler
	popq %rbp
	jmp __x86_return_thunk

.L_haoc_enabled_rwx:
	pushq %r12

	/* save Interrupt flag*/
	pushfq
	/* close irq */
	cli

#ifdef CONFIG_PTP
	movq PER_CPU_VAR(iee_cr0s), %r12
	testq %r12, %r12
	jz rwx_wp_enabled

	movq %cr4, %rax  /* rax -> cr4 */
	andq $(~X86_CR4_SMEP), %rax
	movq %rax, %cr4

	call _iee_si_handler

	movq %cr4, %rax  /* rax -> cr4 */
3:  orq $X86_CR4_SMEP_SMAP, %rax
	movq %rax, %cr4
	andq $X86_CR4_SMEP_SMAP, %rax
	cmpq $X86_CR4_SMEP_SMAP, %rax
	jnz 3

	popfq
	popq %r12
	popq %rbp
	jmp __x86_return_thunk  /* ret */

rwx_wp_enabled:
#endif
	/* set SMEP=0 to enable supervisor-mode exec user-mode insn */
	movq %cr4, %rax  /* rax -> cr4 */
	andq $(~X86_CR4_SMEP), %rax
	movq %rax, %cr4

	DISABLE_WP r12

2:  call _iee_si_handler

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
	popq %rbp
	jmp __x86_return_thunk  /* ret */
SYM_FUNC_END(iee_rwx_gate)
#endif

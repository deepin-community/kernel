/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/page.h>
#include <asm/regdef.h>
#include <asm/csr.h>

	.text
	.set	noat
ENTRY(sw64_suspend_deep_sleep)
	/* a0 $16 will be the address of suspend_state */
	ldi	$1, PSTATE_REGS($16)
	stl	$9, CALLEE_R9($1)
	stl	$10, CALLEE_R10($1)
	stl	$11, CALLEE_R11($1)
	stl	$12, CALLEE_R12($1)
	stl	$13, CALLEE_R13($1)
	stl	$14, CALLEE_R14($1)
	stl	$15, CALLEE_R15($1)
	stl	$26, CALLEE_RA($1)
	/* SIMD-FP */
	ldi	$1, PSTATE_FPREGS($16)
	vstd	$f2, CALLEE_F2($1)
	vstd	$f3, CALLEE_F3($1)
	vstd	$f4, CALLEE_F4($1)
	vstd	$f5, CALLEE_F5($1)
	vstd	$f6, CALLEE_F6($1)
	vstd	$f7, CALLEE_F7($1)
	vstd	$f8, CALLEE_F8($1)
	vstd	$f9, CALLEE_F9($1)
	rfpcr	$f0
	fstd	$f0, PSTATE_FPCR($16)
	stl	$8, PSTATE_KTP($16)

	/* save the address of suspend_state to $18 */
	mov	$16, $18

	/*
	 * Now will Go to Deep Sleep
	 * HMcode should save  pc, gp, ps, r16, r17, r18
	 */

	sys_call HMC_sleepen
	sys_call HMC_whami
	bis	$0, $0, $16
	ldi	$17, 0x2($31)
	sys_call HMC_sendii

	/* wait for a while to receive interrupt */
	ldi	$16, 0x1($31)
	sll	$16, 24, $16
$subloop:
	subl	$16, 1, $16
	bis	$16, $16, $16
	bis	$16, $16, $16
	bne	$16, $subloop


	ldl	$8, PSTATE_KTP($18)
#ifdef CONFIG_SUBARCH_C4
	csrw	$8, CSR_KTP
#endif
	ldi	$1, PSTATE_REGS($18)
	ldl	$9, CALLEE_R9($1)
	ldl	$10, CALLEE_R10($1)
	ldl	$11, CALLEE_R11($1)
	ldl	$12, CALLEE_R12($1)
	ldl	$13, CALLEE_R13($1)
	ldl	$14, CALLEE_R14($1)
	ldl	$15, CALLEE_R15($1)
	ldl	$26, CALLEE_RA($1)
	/* SIMD-FP */
	fldd	$f0, PSTATE_FPCR($18)
	wfpcr	$f0
	fimovd	$f0, $2
	and	$2, 0x3, $2
	beq	$2, $suspend_setfpec_0
	subl	$2, 0x1, $2
	beq	$2, $suspend_setfpec_1
	subl	$2, 0x1, $2
	beq	$2, $suspend_setfpec_2
	setfpec3
	br	$suspend_setfpec_over
$suspend_setfpec_0:
	setfpec0
	br	$suspend_setfpec_over
$suspend_setfpec_1:
	setfpec1
	br	$suspend_setfpec_over
$suspend_setfpec_2:
	setfpec2
$suspend_setfpec_over:
	ldi	$1, PSTATE_FPREGS($18)
	vldd	$f2, CALLEE_F2($1)
	vldd	$f3, CALLEE_F3($1)
	vldd	$f4, CALLEE_F4($1)
	vldd	$f5, CALLEE_F5($1)
	vldd	$f6, CALLEE_F6($1)
	vldd	$f7, CALLEE_F7($1)
	vldd	$f8, CALLEE_F8($1)
	vldd	$f9, CALLEE_F9($1)
	ret
END(sw64_suspend_deep_sleep)

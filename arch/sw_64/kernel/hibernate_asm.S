/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/page.h>
#include <asm/regdef.h>
#include <asm/csr.h>

	.macro SAVE_KTP
#ifdef CONFIG_SUBARCH_C3B
	sys_call HMC_wrktp
#else
	csrw	$8, CSR_KTP
#endif
	.endm

	.text
	.set noat
ENTRY(swsusp_arch_suspend)
	ldi	$16, hibernate_state
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
	stl	sp, PSTATE_SP($16)
	call	swsusp_save
	ldi	$16, hibernate_state
	ldi	$1, PSTATE_REGS($16)
	ldl	$26, CALLEE_RA($1)

	/* save current_thread_info()->pcbb */
	ret
END(swsusp_arch_suspend)

ENTRY(restore_image)
	/* prepare to copy image data to their original locations */
	ldi	t0, restore_pblist
	ldl	t0, 0(t0)
$loop:
	beq	t0, $done

	/* get addresses from the pbe and copy the page */
	ldl	t1, PBE_ADDR(t0)  /* source */
	ldl	t2, PBE_ORIG_ADDR(t0) /* destination */
	ldi	t3, PAGE_SIZE
	addl	t1, t3, t3
$cpyloop:
	ldl	t8, 0(t1)
	stl	t8, 0(t2)
	addl	t1, 8, t1
	addl	t2, 8, t2
	cmpeq	t1, t3, t4
	beq	t4, $cpyloop

	/* progress to the next pbe */
	ldl	t0, PBE_NEXT(t0)
	bne	t0, $loop
$done:

	/* tell the hibernation core that we've just restored the memory */
	ldi	$0, in_suspend
	stl	$31, 0($0)

	ldi	$16, hibernate_state
	ldi	$1, PSTATE_REGS($16)

	ldl	$9, CALLEE_R9($1)
	ldl	$10, CALLEE_R10($1)
	ldl	$11, CALLEE_R11($1)
	ldl	$12, CALLEE_R12($1)
	ldl	$13, CALLEE_R13($1)
	ldl	$14, CALLEE_R14($1)
	ldl	$15, CALLEE_R15($1)
	ldl	$26, CALLEE_RA($1)
	/* SIMD-FP */
	fldd	$f0, PSTATE_FPCR($16)
	wfpcr	$f0
	fimovd	$f0, $2
	and	$2, 0x3, $2
	beq	$2, $hibernate_setfpec_0
	subl	$2, 0x1, $2
	beq	$2, $hibernate_setfpec_1
	subl	$2, 0x1, $2
	beq	$2, $hibernate_setfpec_2
	setfpec3
	br	$hibernate_setfpec_over
$hibernate_setfpec_0:
	setfpec0
	br	$hibernate_setfpec_over
$hibernate_setfpec_1:
	setfpec1
	br	$hibernate_setfpec_over
$hibernate_setfpec_2:
	setfpec2
$hibernate_setfpec_over:
	ldi	$1, PSTATE_FPREGS($16)
	vldd	$f2, CALLEE_F2($1)
	vldd	$f3, CALLEE_F3($1)
	vldd	$f4, CALLEE_F4($1)
	vldd	$f5, CALLEE_F5($1)
	vldd	$f6, CALLEE_F6($1)
	vldd	$f7, CALLEE_F7($1)
	vldd	$f8, CALLEE_F8($1)
	vldd	$f9, CALLEE_F9($1)

	ldl	sp, PSTATE_SP($16)
	ldl	$8, PSTATE_KTP($16)
	SAVE_KTP
	ldi	$0, 0($31)

	ret
END(restore_image)

/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/thread_info.h>
#include <asm/regdef.h>
#include <asm/csr.h>

	.text
	.set noat
ENTRY(__fpstate_save)
	/* a0: prev task */
#ifdef CONFIG_SUBARCH_C4
	csrr	$1, CSR_WR_FREGS
	beq	$1, out
#endif
	vstd	$f0, TASK_THREAD_F0(a0)
	vstd	$f1, TASK_THREAD_F1(a0)
	vstd	$f2, TASK_THREAD_F2(a0)
	vstd	$f3, TASK_THREAD_F3(a0)
	vstd	$f4, TASK_THREAD_F4(a0)
	vstd	$f5, TASK_THREAD_F5(a0)
	vstd	$f6, TASK_THREAD_F6(a0)
	vstd	$f7, TASK_THREAD_F7(a0)
	vstd	$f8, TASK_THREAD_F8(a0)
	vstd	$f9, TASK_THREAD_F9(a0)
	vstd	$f10, TASK_THREAD_F10(a0)
	vstd	$f11, TASK_THREAD_F11(a0)
	vstd	$f12, TASK_THREAD_F12(a0)
	vstd	$f13, TASK_THREAD_F13(a0)
	vstd	$f14, TASK_THREAD_F14(a0)
	vstd	$f15, TASK_THREAD_F15(a0)
	vstd	$f16, TASK_THREAD_F16(a0)
	vstd	$f17, TASK_THREAD_F17(a0)
	vstd	$f18, TASK_THREAD_F18(a0)
	vstd	$f19, TASK_THREAD_F19(a0)
	vstd	$f20, TASK_THREAD_F20(a0)
	vstd	$f21, TASK_THREAD_F21(a0)
	vstd	$f22, TASK_THREAD_F22(a0)
	vstd	$f23, TASK_THREAD_F23(a0)
	vstd	$f24, TASK_THREAD_F24(a0)
	vstd	$f25, TASK_THREAD_F25(a0)
	vstd	$f26, TASK_THREAD_F26(a0)
	vstd	$f27, TASK_THREAD_F27(a0)
	rfpcr	$f0
	vstd	$f28, TASK_THREAD_F28(a0)
	vstd	$f29, TASK_THREAD_F29(a0)
	vstd	$f30, TASK_THREAD_F30(a0)
	fstd	$f0, TASK_THREAD_FPCR(a0)
	vldd	$f0, TASK_THREAD_F0(a0)
out:
	ret
END(__fpstate_save)

ENTRY(__fpstate_restore)
	/* a0: next task */
	fldd	$f0, TASK_THREAD_FPCR(a0)
	wfpcr	$f0
	fimovd	$f0, t1
	and	t1, 0x3, t1
	beq	t1, $setfpec_0
	subl	t1, 0x1, t1
	beq	t1, $setfpec_1
	subl	t1, 0x1, t1
	beq	t1, $setfpec_2
	setfpec3
	br	$setfpec_over
$setfpec_0:
	setfpec0
	br	$setfpec_over
$setfpec_1:
	setfpec1
	br	$setfpec_over
$setfpec_2:
	setfpec2
$setfpec_over:
	vldd	$f0, TASK_THREAD_F0(a0)
	vldd	$f1, TASK_THREAD_F1(a0)
	vldd	$f2, TASK_THREAD_F2(a0)
	vldd	$f3, TASK_THREAD_F3(a0)
	vldd	$f4, TASK_THREAD_F4(a0)
	vldd	$f5, TASK_THREAD_F5(a0)
	vldd	$f6, TASK_THREAD_F6(a0)
	vldd	$f7, TASK_THREAD_F7(a0)
	vldd	$f8, TASK_THREAD_F8(a0)
	vldd	$f9, TASK_THREAD_F9(a0)
	vldd	$f10, TASK_THREAD_F10(a0)
	vldd	$f11, TASK_THREAD_F11(a0)
	vldd	$f12, TASK_THREAD_F12(a0)
	vldd	$f13, TASK_THREAD_F13(a0)
	vldd	$f14, TASK_THREAD_F14(a0)
	vldd	$f15, TASK_THREAD_F15(a0)
	vldd	$f16, TASK_THREAD_F16(a0)
	vldd	$f17, TASK_THREAD_F17(a0)
	vldd	$f18, TASK_THREAD_F18(a0)
	vldd	$f19, TASK_THREAD_F19(a0)
	vldd	$f20, TASK_THREAD_F20(a0)
	vldd	$f21, TASK_THREAD_F21(a0)
	vldd	$f22, TASK_THREAD_F22(a0)
	vldd	$f23, TASK_THREAD_F23(a0)
	vldd	$f24, TASK_THREAD_F24(a0)
	vldd	$f25, TASK_THREAD_F25(a0)
	vldd	$f26, TASK_THREAD_F26(a0)
	vldd	$f27, TASK_THREAD_F27(a0)
	vldd	$f28, TASK_THREAD_F28(a0)
	vldd	$f29, TASK_THREAD_F29(a0)
	vldd	$f30, TASK_THREAD_F30(a0)
#ifdef CONFIG_SUBARCH_C4
	csrw	$31, CSR_WR_FREGS
#endif
	ret
END(__fpstate_restore)

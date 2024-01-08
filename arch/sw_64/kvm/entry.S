/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2018 - os kernal
 * Author: fire3 <fire3@example.com>
 */
	.text
#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/page.h>
#include <asm/regdef.h>

	.set noat

/*
 * r16: physical address of guest kvm_vcpu.arch.vcb
 * r17: pointer to guest kvm_vcpu.arch.kvm_regs
 * r18: pointer to hcall args
 */
ENTRY(__sw64_vcpu_run)
	/* save host fpregs */
	rfpcr	$f0
	fstd	$f0, TASK_THREAD_FPCR($8)
	vstd	$f2, TASK_THREAD_F2($8)
	vstd	$f3, TASK_THREAD_F3($8)
	vstd	$f4, TASK_THREAD_F4($8)
	vstd	$f5, TASK_THREAD_F5($8)
	vstd	$f6, TASK_THREAD_F6($8)
	vstd	$f7, TASK_THREAD_F7($8)
	vstd	$f8, TASK_THREAD_F8($8)
	vstd	$f9, TASK_THREAD_F9($8)

	ldi	sp, -VCPU_RET_SIZE(sp)
	/* save host  pt_regs to current kernel  stack */
	ldi	sp, -PT_REGS_SIZE(sp)
	stl	$9, PT_REGS_R9(sp)
	stl	$8, PT_REGS_R8(sp)
	stl	$10, PT_REGS_R10(sp)
	stl	$11, PT_REGS_R11(sp)
	stl	$12, PT_REGS_R12(sp)
	stl	$13, PT_REGS_R13(sp)
	stl	$14, PT_REGS_R14(sp)
	stl	$15, PT_REGS_R15(sp)
	stl	$26, PT_REGS_R26(sp)

	/* restore guest switch stack from guest kvm_regs struct */
	ldl	$0, KVM_REGS_R0($17)
	ldl	$1, KVM_REGS_R1($17)
	/* restore $2 later */
	ldl	$3, KVM_REGS_R3($17)
	ldl	$4, KVM_REGS_R4($17)
	ldl	$5, KVM_REGS_R5($17)
	ldl	$6, KVM_REGS_R6($17)
	ldl	$7, KVM_REGS_R7($17)
	ldl	$8, KVM_REGS_R8($17)
	ldl	$9, KVM_REGS_R9($17)
	ldl	$10, KVM_REGS_R10($17)
	ldl	$11, KVM_REGS_R11($17)
	ldl	$12, KVM_REGS_R12($17)
	ldl	$13, KVM_REGS_R13($17)
	ldl	$14, KVM_REGS_R14($17)
	ldl	$15, KVM_REGS_R15($17)
	ldl	$19, KVM_REGS_R19($17)
	ldl	$20, KVM_REGS_R20($17)
	ldl	$21, KVM_REGS_R21($17)
	ldl	$22, KVM_REGS_R22($17)
	ldl	$23, KVM_REGS_R23($17)
	ldl	$24, KVM_REGS_R24($17)
	ldl	$25, KVM_REGS_R25($17)
	ldl	$26, KVM_REGS_R26($17)
	ldl	$27, KVM_REGS_R27($17)
	ldl	$28, KVM_REGS_R28($17)

	fldd	$f0, KVM_REGS_FPCR($17)
	wfpcr	$f0
	fimovd  $f0, $2
	and	$2, 0x3, $2
	beq	$2, $g_setfpec_0
	subl	$2, 0x1, $2
	beq	$2, $g_setfpec_1
	subl	$2, 0x1, $2
	beq	$2, $g_setfpec_2
	setfpec3
	br	$g_setfpec_over
$g_setfpec_0:
	setfpec0
	br	$g_setfpec_over
$g_setfpec_1:
	setfpec1
	br	$g_setfpec_over
$g_setfpec_2:
	setfpec2
$g_setfpec_over:
	ldl	$2, KVM_REGS_R2($17)
	vldd	$f0, KVM_REGS_F0($17)
	vldd	$f1, KVM_REGS_F1($17)
	vldd	$f2, KVM_REGS_F2($17)
	vldd	$f3, KVM_REGS_F3($17)
	vldd	$f4, KVM_REGS_F4($17)
	vldd	$f5, KVM_REGS_F5($17)
	vldd	$f6, KVM_REGS_F6($17)
	vldd	$f7, KVM_REGS_F7($17)
	vldd	$f8, KVM_REGS_F8($17)
	vldd	$f9, KVM_REGS_F9($17)
	vldd	$f10, KVM_REGS_F10($17)
	vldd	$f11, KVM_REGS_F11($17)
	vldd	$f12, KVM_REGS_F12($17)
	vldd	$f13, KVM_REGS_F13($17)
	vldd	$f14, KVM_REGS_F14($17)
	vldd	$f15, KVM_REGS_F15($17)
	vldd	$f16, KVM_REGS_F16($17)
	vldd	$f17, KVM_REGS_F17($17)
	vldd	$f18, KVM_REGS_F18($17)
	vldd	$f19, KVM_REGS_F19($17)
	vldd	$f20, KVM_REGS_F20($17)
	vldd	$f21, KVM_REGS_F21($17)
	vldd	$f22, KVM_REGS_F22($17)
	vldd	$f23, KVM_REGS_F23($17)
	vldd	$f24, KVM_REGS_F24($17)
	vldd	$f25, KVM_REGS_F25($17)
	vldd	$f26, KVM_REGS_F26($17)
	vldd	$f27, KVM_REGS_F27($17)
	vldd	$f28, KVM_REGS_F28($17)
	vldd	$f29, KVM_REGS_F29($17)
	vldd	$f30, KVM_REGS_F30($17)

	ldi	$17, KVM_REGS_PS($17)

	/* enter guest */
	/* r16 = guest vcpucb pointer */
	/* r17 = base of guest kvm_regs.ps, saved/restored by hmcode */

	/* enter guest now */
	sys_call 0x31
	/* exit guest now */

	ldi	$17, -KVM_REGS_PS($17) /* r17: base of kvm_regs */

	vstd	$f0, KVM_REGS_F0($17)
	vstd	$f1, KVM_REGS_F1($17)
	vstd	$f2, KVM_REGS_F2($17)
	vstd	$f3, KVM_REGS_F3($17)
	vstd	$f4, KVM_REGS_F4($17)
	vstd	$f5, KVM_REGS_F5($17)
	vstd	$f6, KVM_REGS_F6($17)
	vstd	$f7, KVM_REGS_F7($17)
	vstd	$f8, KVM_REGS_F8($17)
	vstd	$f9, KVM_REGS_F9($17)
	vstd	$f10, KVM_REGS_F10($17)
	vstd	$f11, KVM_REGS_F11($17)
	vstd	$f12, KVM_REGS_F12($17)
	vstd	$f13, KVM_REGS_F13($17)
	vstd	$f14, KVM_REGS_F14($17)
	vstd	$f15, KVM_REGS_F15($17)
	vstd	$f16, KVM_REGS_F16($17)
	vstd	$f17, KVM_REGS_F17($17)
	vstd	$f18, KVM_REGS_F18($17)
	vstd	$f19, KVM_REGS_F19($17)
	vstd	$f20, KVM_REGS_F20($17)
	vstd	$f21, KVM_REGS_F21($17)
	vstd	$f22, KVM_REGS_F22($17)
	vstd	$f23, KVM_REGS_F23($17)
	vstd	$f24, KVM_REGS_F24($17)
	vstd	$f25, KVM_REGS_F25($17)
	vstd	$f26, KVM_REGS_F26($17)
	vstd	$f27, KVM_REGS_F27($17)
	vstd	$f28, KVM_REGS_F28($17)
	vstd	$f29, KVM_REGS_F29($17)
	vstd	$f30, KVM_REGS_F30($17)

	rfpcr	$f0
	fstd	$f0, KVM_REGS_FPCR($17)

	/* don't save r0 Hmcode have saved r0 for us */
	stl	$1, KVM_REGS_R1($17)
	stl	$2, KVM_REGS_R2($17)
	stl	$3, KVM_REGS_R3($17)
	stl	$4, KVM_REGS_R4($17)
	stl	$5, KVM_REGS_R5($17)
	stl	$6, KVM_REGS_R6($17)
	stl	$7, KVM_REGS_R7($17)
	stl	$8, KVM_REGS_R8($17)
	stl	$9, KVM_REGS_R9($17)
	stl	$10, KVM_REGS_R10($17)
	stl	$11, KVM_REGS_R11($17)
	stl	$12, KVM_REGS_R12($17)
	stl	$13, KVM_REGS_R13($17)
	stl	$14, KVM_REGS_R14($17)
	stl	$15, KVM_REGS_R15($17)
	stl	$19, KVM_REGS_R19($17)
	stl	$20, KVM_REGS_R20($17)
	stl	$21, KVM_REGS_R21($17)
	stl	$22, KVM_REGS_R22($17)
	stl	$23, KVM_REGS_R23($17)
	stl	$24, KVM_REGS_R24($17)
	stl	$25, KVM_REGS_R25($17)
	stl	$26, KVM_REGS_R26($17)
	stl	$27, KVM_REGS_R27($17)
	stl	$28, KVM_REGS_R28($17)

	/* restore host regs from host sp */
	ldl	$8, PT_REGS_R8(sp)
	ldl	$9, PT_REGS_R9(sp)
	ldl	$10, PT_REGS_R10(sp)
	ldl	$11, PT_REGS_R11(sp)
	ldl	$12, PT_REGS_R12(sp)
	ldl	$13, PT_REGS_R13(sp)
	ldl	$14, PT_REGS_R14(sp)
	ldl	$15, PT_REGS_R15(sp)
	ldl	$26, PT_REGS_R26(sp)
	ldi	sp, PT_REGS_SIZE(sp)

	/* restore host fpregs */
	fldd	$f0, TASK_THREAD_FPCR($8)
	wfpcr	$f0
	fimovd	$f0, $2
	and	$2, 0x3, $2
	beq	$2, $setfpec_0
	subl	$2, 0x1, $2
	beq	$2, $setfpec_1
	subl	$2, 0x1, $2
	beq	$2, $setfpec_2
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
	vldd	$f2, TASK_THREAD_F2($8)
	vldd	$f3, TASK_THREAD_F3($8)
	vldd	$f4, TASK_THREAD_F4($8)
	vldd	$f5, TASK_THREAD_F5($8)
	vldd	$f6, TASK_THREAD_F6($8)
	vldd	$f7, TASK_THREAD_F7($8)
	vldd	$f8, TASK_THREAD_F8($8)
	vldd	$f9, TASK_THREAD_F9($8)

	/* if $0 > 0, handle hcall */
	bgt	$0, $ret_to

	stl	$26, VCPU_RET_RA(sp)
	stl	$0, VCPU_RET_R0(sp)

	/* Hmcode will setup in  */
	/* restore $16 $17 $18, do interrupt trick */
	ldi	sp, -(HOST_INT_SIZE + PT_REGS_SIZE)(sp)
	ldl	$16, HOST_INT_R16(sp)
	ldl	$17, HOST_INT_R17(sp)
	ldl	$18, HOST_INT_R18(sp)
	ldi	sp, (HOST_INT_SIZE + PT_REGS_SIZE)(sp)

	ldi	$19, -PT_REGS_SIZE(sp)
	call	$26, do_entInt
	ldl	$26, VCPU_RET_RA(sp)
	ldl	$0, VCPU_RET_R0(sp)
$ret_to:
	/* ret($0) indicate hcall number */
	ldi	sp, VCPU_RET_SIZE(sp)	/* pop stack */
	ret

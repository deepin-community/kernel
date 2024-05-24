/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Kernel entry-points.
 */

#include <asm/asm-offsets.h>
#include <asm/thread_info.h>
#include <asm/hmcall.h>
#include <asm/errno.h>
#include <asm/unistd.h>
#include <asm/ptrace.h>
#include <asm/csr.h>

	.text
	.set noat

	.macro SAVE_ALL_NMI
	csrw	$sp, CSR_NMI_SCRATCH
	csrr	$sp, CSR_NMI_STACK

	ldi	$sp, -PT_REGS_SIZE($sp)
	stl	$0, PT_REGS_R0($sp)
	stl	$1, PT_REGS_R1($sp)
	stl	$2, PT_REGS_R2($sp)
	stl	$3, PT_REGS_R3($sp)
	stl	$4, PT_REGS_R4($sp)
	stl	$5, PT_REGS_R5($sp)
	stl	$6, PT_REGS_R6($sp)
	stl	$7, PT_REGS_R7($sp)
	stl	$8, PT_REGS_R8($sp)
	stl	$9, PT_REGS_R9($sp)
	stl	$10, PT_REGS_R10($sp)
	stl	$11, PT_REGS_R11($sp)
	stl	$12, PT_REGS_R12($sp)
	stl	$13, PT_REGS_R13($sp)
	stl	$14, PT_REGS_R14($sp)
	stl	$15, PT_REGS_R15($sp)
	stl	$16, PT_REGS_R16($sp)
	stl	$17, PT_REGS_R17($sp)
	stl	$18, PT_REGS_R18($sp)
	stl	$19, PT_REGS_R19($sp)
	stl	$20, PT_REGS_R20($sp)
	stl	$21, PT_REGS_R21($sp)
	stl	$22, PT_REGS_R22($sp)
	stl	$23, PT_REGS_R23($sp)
	stl	$24, PT_REGS_R24($sp)
	stl	$25, PT_REGS_R25($sp)
	stl	$26, PT_REGS_R26($sp)
	stl	$27, PT_REGS_R27($sp)
	stl	$28, PT_REGS_R28($sp)
	stl	$29, PT_REGS_GP($sp)

	/* Due to a flaw in the CSR design, in some cases the
	 * read CSR instruction is sent before the write instruction,
	 * so it needs to be read twice to ensure that the correct
	 * CSR value is read. Don`t delete it!
	 */
	csrr	$1, CSR_NMI_SCRATCH
	csrr	$1, CSR_NMI_SCRATCH
	csrr	$2, CSR_PS
	csrr	$3, CSR_PC
	csrr    $4, CSR_CAUSE
	csrr	$16, CSR_EARG0
	csrr	$17, CSR_EARG1
	csrr	$18, CSR_EARG2
	stl	$16, PT_REGS_EARG0($sp)
	stl	$17, PT_REGS_EARG1($sp)
	stl	$18, PT_REGS_EARG2($sp)

	stl     $1, PT_REGS_SP($sp)
	stl	$2, PT_REGS_PS($sp)
	stl	$3, PT_REGS_PC($sp)
	stl     $4, PT_REGS_CAUSE($sp)
	ldi	$1, NO_SYSCALL
	stl	$1, PT_REGS_ORIG_R0($sp)
	csrr	$8, CSR_KTP
#ifdef CONFIG_FRAME_POINTER
	ldi	$sp, -STACKFRAME_SIZE($sp)
	stl	$3, STACKFRAME_PC($sp)
	stl	$15, STACKFRAME_FP($sp)
	mov	$sp, $15
#endif
	br      $27, 1f
1:      ldgp    $29, 0($27)
	call	$26, save_nmi_ctx
	ldl	$16, (PT_REGS_EARG0 + STACKFRAME_SIZE)($sp)
	ldl	$17, (PT_REGS_EARG1 + STACKFRAME_SIZE)($sp)
	ldl	$18, (PT_REGS_EARG2 + STACKFRAME_SIZE)($sp)
	.endm

	.macro RESTORE_ALL_NMI
	call	$26, restore_nmi_ctx

	ldi	$sp, STACKFRAME_SIZE($sp)

	ldl	$1, PT_REGS_PS($sp)
	ldl	$2, PT_REGS_PC($sp)
	csrw	$1, CSR_PS
	csrw	$2, CSR_PC

	ldl	$0, PT_REGS_R0($sp)
	ldl	$1, PT_REGS_R1($sp)
	ldl	$2, PT_REGS_R2($sp)
	ldl	$3, PT_REGS_R3($sp)
	ldl	$4, PT_REGS_R4($sp)
	ldl	$5, PT_REGS_R5($sp)
	ldl	$6, PT_REGS_R6($sp)
	ldl	$7, PT_REGS_R7($sp)
	ldl	$8, PT_REGS_R8($sp)
	ldl	$9, PT_REGS_R9($sp)
	ldl	$10, PT_REGS_R10($sp)
	ldl	$11, PT_REGS_R11($sp)
	ldl	$12, PT_REGS_R12($sp)
	ldl	$13, PT_REGS_R13($sp)
	ldl	$14, PT_REGS_R14($sp)
	ldl	$15, PT_REGS_R15($sp)
	ldl	$16, PT_REGS_R16($sp)
	ldl	$17, PT_REGS_R17($sp)
	ldl	$18, PT_REGS_R18($sp)
	ldl	$19, PT_REGS_R19($sp)
	ldl	$20, PT_REGS_R20($sp)
	ldl	$21, PT_REGS_R21($sp)
	ldl	$22, PT_REGS_R22($sp)
	ldl	$23, PT_REGS_R23($sp)
	ldl	$24, PT_REGS_R24($sp)
	ldl	$25, PT_REGS_R25($sp)
	ldl	$26, PT_REGS_R26($sp)
	ldl	$27, PT_REGS_R27($sp)
	ldl	$28, PT_REGS_R28($sp)
	ldl     $29, PT_REGS_GP($sp)
	ldl	$sp, PT_REGS_SP($sp)
	.endm

	.macro SAVE_ALL
	csrw	$sp, CSR_SP
	csrw	$1, CSR_SCRATCH
	csrr	$1, CSR_PS
	and	$1, 8, $1
	beq	$1, 1f
	csrr	$1, CSR_KTP
	ldl	$sp, TI_PCB_KSP($1)
1:
	/* Due to a flaw in the CSR design, in some cases the
	 * read CSR instruction is sent before the write instruction,
	 * so it needs to be read twice to ensure that the correct
	 * CSR value is read. Don`t delete it!
	 */
	csrr	$1, CSR_SCRATCH
	csrr	$1, CSR_SCRATCH
	ldi	$sp, -PT_REGS_SIZE($sp)
	stl	$0, PT_REGS_R0($sp)
	stl	$1, PT_REGS_R1($sp)
	stl	$2, PT_REGS_R2($sp)
	stl	$3, PT_REGS_R3($sp)
	stl	$4, PT_REGS_R4($sp)
	stl	$5, PT_REGS_R5($sp)
	stl	$6, PT_REGS_R6($sp)
	stl	$7, PT_REGS_R7($sp)
	stl	$8, PT_REGS_R8($sp)
	stl	$9, PT_REGS_R9($sp)
	stl	$10, PT_REGS_R10($sp)
	stl	$11, PT_REGS_R11($sp)
	stl	$12, PT_REGS_R12($sp)
	stl	$13, PT_REGS_R13($sp)
	stl	$14, PT_REGS_R14($sp)
	stl	$15, PT_REGS_R15($sp)
	stl	$16, PT_REGS_R16($sp)
	stl	$17, PT_REGS_R17($sp)
	stl	$18, PT_REGS_R18($sp)
	stl	$19, PT_REGS_R19($sp)
	stl	$20, PT_REGS_R20($sp)
	stl	$21, PT_REGS_R21($sp)
	stl	$22, PT_REGS_R22($sp)
	stl	$23, PT_REGS_R23($sp)
	stl	$24, PT_REGS_R24($sp)
	stl	$25, PT_REGS_R25($sp)
	stl	$26, PT_REGS_R26($sp)
	stl	$27, PT_REGS_R27($sp)
	stl	$28, PT_REGS_R28($sp)
	stl	$29, PT_REGS_GP($sp)

	csrr	$1, CSR_SP
	csrr	$1, CSR_SP
	csrr	$2, CSR_PS
	csrr	$3, CSR_PC
	csrr	$4, CSR_CAUSE
	csrr	$16, CSR_EARG0
	csrr	$17, CSR_EARG1
	csrr	$18, CSR_EARG2
	stl	$16, PT_REGS_EARG0($sp)
	stl	$17, PT_REGS_EARG1($sp)
	stl	$18, PT_REGS_EARG2($sp)

	stl     $1, PT_REGS_SP($sp)
	stl	$2, PT_REGS_PS($sp)
	stl	$3, PT_REGS_PC($sp)
	stl	$4, PT_REGS_CAUSE($sp)
	ldi	$1, NO_SYSCALL
	stl	$1, PT_REGS_ORIG_R0($sp)
	csrr	$8, CSR_KTP
#ifdef CONFIG_FRAME_POINTER
	ldi	$sp, -STACKFRAME_SIZE($sp)
	stl	$3, STACKFRAME_PC($sp)
	stl	$15, STACKFRAME_FP($sp)
	mov	$sp, $15
#endif
	.endm

	.macro RESTORE_ALL
	ldi     $16, 7
	sys_call HMC_swpipl

	ldi	$sp, STACKFRAME_SIZE($sp)

	ldl	$1, PT_REGS_PS($sp)
	ldl	$2, PT_REGS_PC($sp)
	csrw	$1, CSR_PS
	csrw	$2, CSR_PC
	and     $1, 0x8, $1
	beq	$1, 1f
	ldi	$16,PT_REGS_SIZE($sp)
	stl	$16, TI_PCB_KSP($8)
1:
	ldl	$0, PT_REGS_R0($sp)
	ldl	$1, PT_REGS_R1($sp)
	ldl	$2, PT_REGS_R2($sp)
	ldl	$3, PT_REGS_R3($sp)
	ldl	$4, PT_REGS_R4($sp)
	ldl	$5, PT_REGS_R5($sp)
	ldl	$6, PT_REGS_R6($sp)
	ldl	$7, PT_REGS_R7($sp)
	ldl	$8, PT_REGS_R8($sp)
	ldl	$9, PT_REGS_R9($sp)
	ldl	$10, PT_REGS_R10($sp)
	ldl	$11, PT_REGS_R11($sp)
	ldl	$12, PT_REGS_R12($sp)
	ldl	$13, PT_REGS_R13($sp)
	ldl	$14, PT_REGS_R14($sp)
	ldl	$15, PT_REGS_R15($sp)
	ldl	$16, PT_REGS_R16($sp)
	ldl	$17, PT_REGS_R17($sp)
	ldl	$18, PT_REGS_R18($sp)
	ldl	$19, PT_REGS_R19($sp)
	ldl	$20, PT_REGS_R20($sp)
	ldl	$21, PT_REGS_R21($sp)
	ldl	$22, PT_REGS_R22($sp)
	ldl	$23, PT_REGS_R23($sp)
	ldl	$24, PT_REGS_R24($sp)
	ldl	$25, PT_REGS_R25($sp)
	ldl	$26, PT_REGS_R26($sp)
	ldl	$27, PT_REGS_R27($sp)
	ldl	$28, PT_REGS_R28($sp)
	ldl     $29, PT_REGS_GP($sp)
	ldl	$sp, PT_REGS_SP($sp)
	.endm

	.macro RESTORE_IRQ
	csrr	$16, CSR_PS
	sys_call HMC_swpipl
	ldl	$16, (PT_REGS_EARG0 + STACKFRAME_SIZE)($sp)
	.endm
/*
 * Non-syscall kernel entry points.
 */

	.align 4
	.globl entNMI
	.ent entNMI
entNMI:
	SAVE_ALL_NMI
	br      $27, 1f
1:      ldgp    $29, 0($27)
	ldi	$19, STACKFRAME_SIZE($sp)
	call	$26, do_entInt
	br	ret_from_nmi
	.end entNMI

	.align 4
	.globl entInt
	.ent entInt
entInt:
	SAVE_ALL
	br      $27, 1f
1:      ldgp    $29, 0($27)
	ldi	$19, STACKFRAME_SIZE($sp)
	call	$26, do_entInt
	br	ret_from_sys_call
	.end entInt

	.align 4
	.globl entArith
	.ent entArith
entArith:
	SAVE_ALL
	ldl     $1, (PT_REGS_PC + STACKFRAME_SIZE)($sp)
	ldi	$1, 4($1)
	stl     $1, (PT_REGS_PC + STACKFRAME_SIZE)($sp)
	br      $27, 1f
1:      ldgp    $29, 0($27)
	RESTORE_IRQ
	ldi	$18, STACKFRAME_SIZE($sp)
	call	$26, do_entArith
	br	ret_from_sys_call
	.end entArith

	.align 4
	.globl entMM
	.ent entMM
entMM:
	SAVE_ALL
	br      $27, 1f
1:      ldgp    $29, 0($27)
	RESTORE_IRQ
	ldi	$19, STACKFRAME_SIZE($sp)
	call	$26, do_page_fault
	br	ret_from_sys_call
	.end entMM

	.align 4
	.globl entIF
	.ent entIF
entIF:
	SAVE_ALL
	ldl     $1, (PT_REGS_PC + STACKFRAME_SIZE)($sp)
	ldi	$1, 4($1)
	stl     $1, (PT_REGS_PC + STACKFRAME_SIZE)($sp)
	br      $27, 1f
1:      ldgp    $29, 0($27)
	RESTORE_IRQ
	ldi	$18, STACKFRAME_SIZE($sp)
	call	$26, do_entIF
	br	ret_from_sys_call
	.end entIF

/*
 * Handle unalignment exception.
 * We don't handle the "gp" register correctly, but if we fault on a
 * gp-register unaligned load/store, something is _very_ wrong in the
 * kernel anyway.
 */
	.align 4
	.globl entUna
	.ent entUna
entUna:
	SAVE_ALL
	ldl     $1, (PT_REGS_PC + STACKFRAME_SIZE)($sp)
	ldi	$1, 4($1)
	stl     $1, (PT_REGS_PC + STACKFRAME_SIZE)($sp)
	br      $27, 1f
1:      ldgp    $29, 0($27)
	RESTORE_IRQ
	ldi	$19, STACKFRAME_SIZE($sp)
	ldl	$0, (PT_REGS_PS + STACKFRAME_SIZE)($sp)
	and	$0, 8, $0		/* user mode ? */
	beq	$0, 1f
	call	$26, do_entUnaUser	/* return to ret_from_syscall */
	br	ret_from_sys_call
1:	ldl	$9, (PT_REGS_GP + STACKFRAME_SIZE)($sp)
	call	$26, do_entUna
	stl	$9, (PT_REGS_GP + STACKFRAME_SIZE)($sp)
	RESTORE_ALL
	sys_call HMC_rti
	.end entUna

/*
 * The system call entry point is special.  Most importantly, it looks
 * like a function call to userspace as far as clobbered registers.  We
 * do preserve the argument registers (for syscall restarts) and $26
 * (for leaf syscall functions).
 *
 * So much for theory.  We don't take advantage of this yet.
 *
 * Note that a0-a2 are not saved by HMcode as with the other entry points.
 */

	.align 4
	.globl entSys
	.ent entSys
entSys:
	SAVE_ALL
	ldl     $1, (PT_REGS_PC + STACKFRAME_SIZE)($sp)
	ldi	$1, 4($1)
	stl     $1, (PT_REGS_PC + STACKFRAME_SIZE)($sp)
	br      $27, 1f
1:      ldgp    $29, 0($27)
	RESTORE_IRQ
	ldi	$16, STACKFRAME_SIZE($sp)
	call	$26, do_entSys
	br	ret_from_sys_call
	.end entSys

	.align 4
	.globl ret_from_nmi
	.ent ret_from_nmi
ret_from_nmi:
	br	$27, 1f
1:	ldgp	$29, 0($27)
	RESTORE_ALL_NMI
	sys_call HMC_rti_nmi
	.end ret_from_nmi

	.align 4
	.globl ret_from_sys_call
	.ent ret_from_sys_call
ret_from_sys_call:
	br	$27, 1f
1:	ldgp	$29, 0($27)
	/* Make sure need_resched and sigpending don't change between
		sampling and the rti.  */
	ldi	$16, 7
	sys_call HMC_swpipl
	ldl	$0, (PT_REGS_PS + STACKFRAME_SIZE)($sp)
	and	$0, 8, $0
	beq	$0, restore_all
ret_to_user:
	ldw	$17, TI_FLAGS($8)
	and	$17, _TIF_WORK_MASK, $2
	beq	$2, restore_all
	ldi	$16, STACKFRAME_SIZE($sp)
	call	$26, do_notify_resume
restore_all:
	RESTORE_ALL
	sys_call HMC_rti
	.end ret_from_sys_call

/*
 * Integer register context switch
 * The callee-saved registers must be saved and restored.
 *
 *   a0: previous task_struct (must be preserved across the switch)
 *   a1: next task_struct
 *
 * The value of a0  must be preserved by this function, as that's how
 * arguments are passed to schedule_tail.
 */
	.align 4
	.globl __switch_to
	.ent __switch_to
__switch_to:
	.prologue 0
	/* Save context into prev->thread */
	stl	$26, TASK_THREAD_RA($16)
	stl	$30, TASK_THREAD_SP($16)
	stl	$9, TASK_THREAD_S0($16)
	stl	$10, TASK_THREAD_S1($16)
	stl	$11, TASK_THREAD_S2($16)
	stl	$12, TASK_THREAD_S3($16)
	stl	$13, TASK_THREAD_S4($16)
	stl	$14, TASK_THREAD_S5($16)
	stl	$15, TASK_THREAD_S6($16)
	/* Restore context from next->thread */
	ldl	$26, TASK_THREAD_RA($17)
	ldl	$30, TASK_THREAD_SP($17)
	ldl	$9, TASK_THREAD_S0($17)
	ldl	$10, TASK_THREAD_S1($17)
	ldl	$11, TASK_THREAD_S2($17)
	ldl	$12, TASK_THREAD_S3($17)
	ldl	$13, TASK_THREAD_S4($17)
	ldl	$14, TASK_THREAD_S5($17)
	ldl	$15, TASK_THREAD_S6($17)
	mov	$17, $8
	csrw	$8, CSR_KTP
	mov	$16, $0
	ret
	.end __switch_to

/*
 * New processes begin life here.
 */

	.globl ret_from_fork
	.align 4
	.ent ret_from_fork
ret_from_fork:
	call	$26, schedule_tail
	br	ret_from_sys_call
	.end ret_from_fork

/*
 * ... and new kernel threads - here
 */
	.align 4
	.globl ret_from_kernel_thread
	.ent ret_from_kernel_thread
ret_from_kernel_thread:
	call	$26, schedule_tail
	mov	$9, $27
	mov	$10, $16
	call	$26, ($9)
	br	ret_to_user
	.end ret_from_kernel_thread

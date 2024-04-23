/* SPDX-License-Identifier: GPL-2.0 */
/*
 * initial boot stuff.. At this point, the bootloader has already
 * switched into HMcode, and loaded us at the correct address
 * (START_ADDR).  So there isn't much left for us to do: just set up
 * the kernel global pointer and jump to the kernel entry-point.
 */

#include <linux/init.h>
#include <asm/asm-offsets.h>
#include <asm/hmcall.h>
#include <asm/setup.h>
#include <asm/csr.h>

__HEAD
	.globl _stext
	.set noreorder
	.globl __start
	.ent __start
_stext:
__start:
	.prologue 0
	br	$27, 1f
1:	ldgp	$29, 0($27)
	/* We need to get current_task_info loaded up...  */
	ldi	$8, init_task
	ldl	$30, TASK_STACK($8)
	/* ... and find our stack ... */
	ldi	$30, ASM_THREAD_SIZE($30)

	/**
	 * Starting from Core4, Grub uses $16 to pass magic num
	 * and use $17 to pass DTB base address.
	 *
	 * So, we need to save $16 and $17 before clearing bss.
	 */
	ldi     $30, -16($30)
	stl     $16, 8($30)
	stl     $17, 0($30)

	/* ... and then we can clear bss data.  */
	ldi	$16, __bss_start
	ldi	$18, __bss_stop
	subl	$18, $16, $18
	mov	$31, $17
	call	$26, __constant_c_memset
	ldgp	$29, 0($26)

	/* Get magic num and DTB base address. */
	ldl     $16, 8($30)
	ldl     $17, 0($30)
	ldi     $30, 16($30)
	/* sunway_boot_magic = $16 */
	ldi     $7, sunway_boot_magic
	stl     $16, 0($7)
	/* sunway_dtb_address = $17 */
	ldi     $7, sunway_dtb_address
	stl     $17, 0($7)

#ifdef CONFIG_RELOCATABLE
	ldi	$30, -8($30)
	stl	$29, 0($30)
	/* Copy kernel and apply the relocations */
	call	$26, relocate_kernel
	ldl	$29, 0($30)
	addl	$29, $0, $29
	addl	$8, $0, $8
	ldi	$30, 8($30)
	/* Repoint the sp into the new kernel image */
	addl	$30, $0, $30
#endif

	/* ... and then we can start the kernel.  */
	call	$26, sw64_start_kernel
	sys_call HMC_halt
	.end __start

#ifdef CONFIG_SMP
	.align 3
	.globl __smp_callin
	.ent __smp_callin
	/* On entry here the PCB of the idle task for this processor
	 * has been loaded.  We've arranged for the tilde_pcb[x] for
	 * this process to contain the PCBB of the target idle task.
	 */
__smp_callin:
	.prologue 1
	br	$27, 2f		# we copy this from above "br $27 1f"
2:	ldgp	$29, 0($27)	# First order of business, load the GP.

	bis	$31, $31, $16	# invalidate all TLB with current VPN
	sys_call HMC_tbi

#if defined(CONFIG_SUBARCH_C3B)
	sys_call HMC_whami	# Get hard cid
	ldi	$1, __cpu_to_rcid
	ldi	$2, 0($31)
	ldi	$4, CONFIG_NR_CPUS
3:	ldw	$3, 0($1)
	cmpeq	$3, $0, $3
	bne	$3, 4f
	addl	$1, 4, $1
	addl	$2, 1, $2
	cmpeq	$2, $4, $5
	bne	$5, 5f
	br	$31, 3b
4:	ldi	$0, 0($2)
#else
	rcid	$0
#endif

	ldi	$2, idle_task_pointer
	s8addl	$0, $2, $2
	ldl	$8, 0($2)	# Get ksp of idle thread

	ldl	$30, TASK_STACK($8)
	ldi	$30, ASM_THREAD_SIZE($30)

	call	$26, smp_callin
5:
	sys_call HMC_halt
	.end __smp_callin
#endif /* CONFIG_SMP */
	#
	# It is handy, on occasion, to make halt actually just loop.
	# Putting it here means we dont have to recompile the whole
	# kernel.
	#

	.align 3
	.globl halt
	.ent halt
halt:
	.prologue 0
	sys_call HMC_halt
	.end halt

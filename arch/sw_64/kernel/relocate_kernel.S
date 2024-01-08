/* SPDX-License-Identifier: GPL-2.0 */
/*
 * relocate_kernel.S for kexec
 * Created by <hesheng05@gmail.com> Jul 2 2019
 *
 * This source code is licensed under the GNU General Public License,
 * Version 2.  See the file COPYING for more details.
 */

#include <asm/regdef.h>
#include <asm/page.h>

	.align 3
	.globl relocate_new_kernel
	.ent relocate_new_kernel

relocate_new_kernel:
	.prologue 0
	ldl	a0, arg0
	ldl	a1, arg1
	ldl	a2, arg2
	ldl	a3, arg3

	ldl	s0, kexec_indirection_page
	ldl	s1, kexec_start_address

process_entry:
	ldl	s2, 0(s0)
	addl	s0, 8, s0

	/*
	 * In case of a kdump/crash kernel, the indirection page is not
	 * populated as the kernel is directly copied to a reserved location
	 */
	beq	s2, done

	/* destination page */
	and	s2, 0x1, s3
	beq	s3, 1f
	bic	s2, 0x1, s4/* store destination addr in s4 */
	br	$31, process_entry

1:
	/* indirection page, update s0*/
	and	s2, 0x2, s3
	beq	s3, 1f
	bic	s2, 0x2, s0
	br	$31, process_entry

1:
	/* done page */
	and	s2, 0x4, s3
	beq	s3, 1f
	br	$31, done
1:
	/* source page */
	and	s2, 0x8, s3
	beq	s3, process_entry
	bic	s2, 0x8, s2
	ldi	s6, 0x1
	sll	s6, (PAGE_SHIFT - 3), s6

copy_word:
	/* copy page word by word */
	ldl	s5, 0(s2)
	stl	s5, 0(s4)
	addl	s4, 8, s4
	addl	s2, 8, s2
	subl	s6, 1, s6
	beq	s6, process_entry
	br	$31, copy_word
	br	$31, process_entry

done:
#ifdef CONFIG_CRASH_SMP /* unsupported now!!!! */
	/* kexec_flag reset is signal to other CPUs what kernel
	 * was moved to it's location. Note - we need relocated address
	 * of kexec_flag.
	 */

	br	ra, 1f
1:	mov	ra, t1
	ldi	t2, 1b
	ldi	t0, kexec_flag
	subl	t0, t2, t0
	addl	t1, t0, t0
	stl	zero, 0(t0)
#endif
	memb
	jmp	ra, (s1)
	.end relocate_new_kernel
	.size relocate_new_kernel, .-relocate_new_kernel

#ifdef CONFIG_CRASH_SMP
	/*
	 * Other CPUs should wait until code is relocated and
	 * then start at entry (?) point.
	 */
	.align 3
	.globl kexec_smp_wait
	.ent kexec_smp_wait
kexec_smp_wait:
	ldl	a0, s_arg0
	ldl	a1, s_arg1
	ldl	a2, s_arg2
	ldl	a3, s_arg3
	ldl	s1, kexec_start_address

	/* Non-relocated address works for args and kexec_start_address (old
	 * kernel is not overwritten). But we need relocated address of
	 * kexec_flag.
	 */

	bsr	ra, 1f
1:	mov	ra, t1
	ldi	t2, 1b
	ldi	t0, kexec_flag
	subl	t0, t2, t0
	addl	t1, t0, t0

1:	stl	s0, 0(t0)
	bne	s0, 1b
	memb
	jmp	ra, (s1)
	.end kexec_smp_wait
	.size kexec_smp_wait, .-kexec_smp_wait
#endif

	.align 3

	/* All parameters to new kernel are passed in registers a0-a3.
	 * kexec_args[0..3] are uses to prepare register values.
	 */

kexec_args:
	.globl kexec_args
arg0:	.quad 0x0
arg1:	.quad 0x0
arg2:	.quad 0x0
arg3:	.quad 0x0
	.size kexec_args, 8*4

#ifdef CONFIG_CRASH_SMP
	/*
	 * Secondary CPUs may have different kernel parameters in
	 * their registers a0-a3. secondary_kexec_args[0..3] are used
	 * to prepare register values.
	 */
secondary_kexec_args:
	.globl secondary_kexec_args
s_arg0:	.quad 0x0
s_arg1:	.quad 0x0
s_arg2:	.quad 0x0
s_arg3:	.quad 0x0
	.size secondary_kexec_args, 8*4

kexec_flag:
	.quad 0x1
#endif

kexec_start_address:
	.globl kexec_start_address
	.quad 0x0
	.size kexec_start_address, 8

kexec_indirection_page:
	.globl kexec_indirection_page
	.quad 0
	.size kexec_indirection_page, 8

relocate_new_kernel_end:

relocate_new_kernel_size:
	.global relocate_new_kernel_size
	.quad relocate_new_kernel_end - relocate_new_kernel
	.size relocate_new_kernel_size, 8

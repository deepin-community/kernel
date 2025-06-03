/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright © 2011 Siarhei Siamashka <siarhei.siamashka@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <linux/linkage.h>
#include <asm/current.h>
#include <asm/asm-offsets.h>
#include <asm/thread_info.h>
#include <asm/cpufeatures.h>
#include <asm/alternative.h>
#include <asm/asm.h>
#include <asm/smap.h>
#include <asm/export.h>
#include <asm/trapnr.h>

#define PREFETCH_DISTANCE 256

.macro ALIGN_DESTINATION_16
    /* check for bad alignment of destination, there is 16Bytes, for we will use movdqa */
    /* if len<16Bytes, jb .Lcopy_user_string */
    cmpq $16,%rdx
    jb .Lcopy_user_string

    /*
     * Adjust unaligned destination addresses, for 16-bit aligned ones,
     * only the lower 32 bits need to be checked.
     */
    movl %edi,%ecx
    andl $15,%ecx
    jz .Lcopy_user_string           /* already aligned */

    subl $16,%ecx
    negl %ecx
    subl %ecx,%edx

200:
    movb (%rsi),%al
201:
    movb %al,(%rdi)
    incq %rsi
    incq %rdi
    decl %ecx
    jnz 200b
    jmp .Lcopy_user_string

203:
    addl %ecx,%edx/* ecx is zerorest also */
    jmp Lsse2_copy_user_handle_tail

    _ASM_EXTABLE_UA(200b, 203b)
    _ASM_EXTABLE_UA(201b, 203b)

.Lcopy_user_string:
.endm
/*****************************************************************************/
SYM_FUNC_START(copy_user_sse2_opt_string)
    ASM_STAC
    ALIGN_DESTINATION_16

    cmpq $64,%rdx
    jb 70f /* less then 64 bytes, avoid the costly 'rep' */

    /*
     * Check if src is aligned, for 16-bit aligned ones,
     * only the lower 32 bits need to be checked.
     */
    movl %esi,%ecx
    andl $15,%ecx
    jnz 20f

10:
    prefetchnta PREFETCH_DISTANCE(%rsi)
11:
    prefetchnta (PREFETCH_DISTANCE + 32)(%rsi)
12:
    movdqa      (%rsi),%xmm0
13:
    movdqa      16(%rsi),%xmm1
14:
    movdqa      32(%rsi),%xmm2
15:
    movdqa      48(%rsi),%xmm3
16:
    movntdq     %xmm0,0(%rdi)
17:
    movntdq     %xmm1,16(%rdi)
18:
    movntdq     %xmm2,32(%rdi)
19:
    movntdq     %xmm3,48(%rdi)
    add         $64,%rsi
    add         $64,%rdi
    subq        $64,%rdx
    cmpq        $64,%rdx
    jg          10b
    sfence
    jmp         70f

20:
    prefetchnta PREFETCH_DISTANCE(%rsi)
21:
    prefetchnta (PREFETCH_DISTANCE + 32)(%rsi)
22:
    movdqu      (%rsi),%xmm0
23:
    movdqu      16(%rsi),%xmm1
24:
    movdqu      32(%rsi),%xmm2
25:
    movdqu      48(%rsi),%xmm3
26:
    movntdq     %xmm0,0(%rdi)
27:
    movntdq     %xmm1,16(%rdi)
28:
    movntdq     %xmm2,32(%rdi)
29:
    movntdq     %xmm3,48(%rdi)
    add         $64,%rsi
    add         $64,%rdi
    subq        $64,%rdx
    cmpq        $64,%rdx
    jg          20b
    sfence

70:
    movl %edx,%ecx
80:
    rep
    movsb

    xorl %eax,%eax
    ASM_CLAC
    RET//ret

99:
    movl %ecx,%edx      /* ecx is zerorest also */
100:
    sfence
    jmp Lsse2_copy_user_handle_tail

  _ASM_EXTABLE_UA(10b, 100b)
  _ASM_EXTABLE_UA(11b, 100b)
  _ASM_EXTABLE_UA(12b, 100b)
  _ASM_EXTABLE_UA(13b, 100b)
  _ASM_EXTABLE_UA(14b, 100b)
  _ASM_EXTABLE_UA(15b, 100b)
  _ASM_EXTABLE_UA(16b, 100b)
  _ASM_EXTABLE_UA(17b, 100b)
  _ASM_EXTABLE_UA(18b, 100b)
  _ASM_EXTABLE_UA(19b, 100b)

  _ASM_EXTABLE_UA(20b, 100b)
  _ASM_EXTABLE_UA(21b, 100b)
  _ASM_EXTABLE_UA(22b, 100b)
  _ASM_EXTABLE_UA(23b, 100b)
  _ASM_EXTABLE_UA(24b, 100b)
  _ASM_EXTABLE_UA(25b, 100b)
  _ASM_EXTABLE_UA(26b, 100b)
  _ASM_EXTABLE_UA(27b, 100b)
  _ASM_EXTABLE_UA(28b, 100b)
  _ASM_EXTABLE_UA(29b, 100b)

  _ASM_EXTABLE_UA(80b, 99b)
SYM_FUNC_END(copy_user_sse2_opt_string)
EXPORT_SYMBOL(copy_user_sse2_opt_string)

SYM_FUNC_START(fpu_restore_xmm0_3)
    ASM_STAC
    movdqu      (%rsi),%xmm0
    movdqu      16(%rsi),%xmm1
    movdqu      32(%rsi),%xmm2
    movdqu      48(%rsi),%xmm3

    xorl %eax,%eax
    ASM_CLAC
    RET//ret
SYM_FUNC_END(fpu_restore_xmm0_3)
EXPORT_SYMBOL(fpu_restore_xmm0_3)

SYM_FUNC_START(fpu_save_xmm0_3)
    ASM_STAC

    movdqu      %xmm0,(%rdi)
    movdqu      %xmm1,16(%rdi)
    movdqu      %xmm2,32(%rdi)
    movdqu      %xmm3,48(%rdi)

    xorl %eax,%eax
    ASM_CLAC
    RET//ret
SYM_FUNC_END(fpu_save_xmm0_3)
EXPORT_SYMBOL(fpu_save_xmm0_3)

/*
 * Try to copy last bytes and clear the rest if needed.
 * Since protection fault in copy_from/to_user is not a normal situation,
 * it is not necessary to optimize tail handling.
 * Don't try to copy the tail if machine check happened
 *
 * Input:
 * rdi destination
 * rsi source
 * rdx count
 *
 * Output:
 * eax uncopied bytes or 0 if successful.
 */
SYM_CODE_START_LOCAL(Lsse2_copy_user_handle_tail)
    movq %rdx,%rcx
    /*
     * The trap number and error code are both 32 bits.
     */
    cmp $X86_TRAP_MC,%eax       /* check if X86_TRAP_MC */
    je 3f
1:  rep movsb
2:  mov %rcx,%rax
    ASM_CLAC
    RET

    /*
     * Return zero to pretend that this copy succeeded. This
     * is counter-intuitive, but needed to prevent the code
     * in lib/iov_iter.c from retrying and running back into
     * the poison cache line again. The machine check handler
     * will ensure that a SIGBUS is sent to the task.
     */
3:  xorl %eax,%eax
    ASM_CLAC
    RET

    _ASM_EXTABLE_UA(1b, 2b)
SYM_CODE_END(Lsse2_copy_user_handle_tail)

/*****************************************************************************/

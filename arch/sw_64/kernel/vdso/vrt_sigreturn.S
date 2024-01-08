/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Sigreturn trampoline for returning from a signal when the SA_RESTORER
 * flag is not set.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/linkage.h>
#include <asm/unistd.h>
#include <uapi/asm/hmcall.h>
#include <asm/asm-offsets.h>

	.text

	.macro SIGCONTEXT_REGS_I base, from = 0
	.cfi_offset \from, \base + (4 + \from) * 8
	.if	30 - \from
	SIGCONTEXT_REGS_I \base, "(\from + 1)"
	.endif
	.endm

	.macro SIGCONTEXT_REGS_F base, from = 32
	.cfi_offset \from, \base + (4 + 32 + 1) * 8 + (\from - 32) * 32
	.if	62 - \from
	SIGCONTEXT_REGS_F \base, "(\from + 1)"
	.endif
	.endm

	.macro SIGCONTEXT_REGS_V base, from = 67
	.cfi_offset \from, \base + (4 + 32 + 1) * 8 + ((\from - 67) & 0x1f) * 32 + (((\from - 67) >> 5) + 1) * 8
	.if	161 - \from
	SIGCONTEXT_REGS_V \base, "(\from + 1)"
	.endif
	.endm

	.macro SIGCONTEXT_REGS base
	SIGCONTEXT_REGS_I \base
	SIGCONTEXT_REGS_F \base
	SIGCONTEXT_REGS_V \base
	.cfi_offset 63, \base + (4 + 32 + 1) * 8 + 32 * 32
	.cfi_offset 64, \base + 2 * 8
	.endm

	.cfi_startproc
	.cfi_return_column 64
	.cfi_signal_frame
	SIGCONTEXT_REGS (-RT_SIGFRAME_SIZE + RT_SIGFRAME_MCTX)
	.cfi_def_cfa_offset RT_SIGFRAME_SIZE

	nop
ENTRY(__vdso_rt_sigreturn)
	mov		$sp, $16
	ldi		$0, __NR_rt_sigreturn
	sys_call	HMC_callsys
ENDPROC(__vdso_rt_sigreturn)
	.cfi_endproc

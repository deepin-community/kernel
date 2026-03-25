/* SPDX-License-Identifier: GPL-2.0 */
#include <linux/linkage.h>
#include <asm/asm-offsets.h>
#include <asm/percpu.h>
#include <asm/processor-flags.h>
#include <asm/asm.h>

#define X86_CR4_SMEP_SMAP (X86_CR4_SMEP | X86_CR4_SMAP)

/*
 * scratch_reg would be changed, caller should dertimine if scratch_reg
 * should be saved and restored.
 */
.macro DISABLE_WP scratch_reg:req
	/* Disable write protection */
	movq %cr0, %\scratch_reg
	andq $(~X86_CR0_WP), %\scratch_reg
	movq %\scratch_reg, %cr0
.endm

.macro ENABLE_WP scratch_reg:req
	/* Enable write protection */
	movq %cr0, %\scratch_reg
1:
	orq $X86_CR0_WP, %\scratch_reg
	movq %\scratch_reg, %cr0
	testq $X86_CR0_WP, %\scratch_reg
	je 1b
.endm

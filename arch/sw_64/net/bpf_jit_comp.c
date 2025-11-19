// SPDX-License-Identifier: GPL-2.0
/*
 * BPF JIT compiler for SW64
 *
 * Copyright (C) Mao Minkai
 * Author: Mao Minkai
 *
 * This file is taken from arch/arm64/net/bpf_jit_comp.c
 *	Copyright (C) 2014-2016 Zi Shen Lim <zlim.lnx@gmail.com>
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
 */

#include <linux/bpf.h>
#include <linux/filter.h>
#include <linux/printk.h>
#include <linux/memory.h>

#include <asm/cacheflush.h>
#include <asm/insn.h>
#include <asm/ftrace.h>

#include "bpf_jit.h"

#define TCALL_CNT (MAX_BPF_JIT_REG + 0)
#define SW64_FENTRY_NINSNS 5
#define SW64_MAX_REG_ARGS  6
#define STACK_ALIGN 16
static const int bpf2sw64[] = {
	/* return value from in-kernel function, and exit value from eBPF */
	[BPF_REG_0] = SW64_BPF_REG_V0,
	/* arguments from eBPF program to in-kernel function */
	[BPF_REG_1] = SW64_BPF_REG_A0,
	[BPF_REG_2] = SW64_BPF_REG_A1,
	[BPF_REG_3] = SW64_BPF_REG_A2,
	[BPF_REG_4] = SW64_BPF_REG_A3,
	[BPF_REG_5] = SW64_BPF_REG_A4,
	/* callee saved registers that in-kernel function will preserve */
	[BPF_REG_6] = SW64_BPF_REG_S0,
	[BPF_REG_7] = SW64_BPF_REG_S1,
	[BPF_REG_8] = SW64_BPF_REG_S2,
	[BPF_REG_9] = SW64_BPF_REG_S3,
	/* read-only frame pointer to access stack */
	[BPF_REG_FP] = SW64_BPF_REG_FP,
	/* tail_call_cnt */
	[TCALL_CNT] = SW64_BPF_REG_S4,
	/* temporary register for blinding constants */
	[BPF_REG_AX] = SW64_BPF_REG_T11,
};

struct jit_ctx {
	const struct bpf_prog *prog;
	int idx;		// JITed instruction index
	int current_tmp_reg;
	int epilogue_offset;
	int *insn_offset;	// [bpf_insn_idx] = jited_insn_idx
	int exentry_idx;
	u32 *image;		// JITed instruction
	u32 *ro_image;
	u32 stack_size;
};

struct sw64_jit_data {
	struct bpf_binary_header *header;
	u8 *image;	// bpf instruction
	struct jit_ctx ctx;
};

static inline u32 sw64_bpf_gen_format_br(int opcode, enum sw64_bpf_registers ra, u32 disp)
{
	opcode = opcode << SW64_BPF_OPCODE_OFFSET;
	ra = ra << SW64_BPF_RA_OFFSET;
	return opcode | ra | (disp & 0x1fffff);
}

static inline u32 sw64_bpf_gen_format_ls(int opcode, enum sw64_bpf_registers ra,
		enum sw64_bpf_registers rb, u16 disp)
{
	opcode = opcode << SW64_BPF_OPCODE_OFFSET;
	ra = ra << SW64_BPF_RA_OFFSET;
	rb = rb << SW64_BPF_RB_OFFSET;
	return opcode | ra | rb | (disp & 0xffff);
}

static inline u32 sw64_bpf_gen_format_ls_func(int opcode, enum sw64_bpf_registers ra,
		enum sw64_bpf_registers rb, u16 disp, int function)
{
	opcode = opcode << SW64_BPF_OPCODE_OFFSET;
	ra = ra << SW64_BPF_RA_OFFSET;
	rb = rb << SW64_BPF_RB_OFFSET;
	function = function << SW64_BPF_LS_FUNC_OFFSET;
	return opcode | ra | rb | function | (disp & 0xfff);
}

static inline u32 sw64_bpf_gen_format_simple_alu_reg(int opcode, enum sw64_bpf_registers ra,
		enum sw64_bpf_registers rb, enum sw64_bpf_registers rc, int function)
{
	opcode = opcode << SW64_BPF_OPCODE_OFFSET;
	ra = ra << SW64_BPF_RA_OFFSET;
	rb = rb << SW64_BPF_RB_OFFSET;
	rc = rc << SW64_BPF_SIMPLE_ALU_RC_OFFSET;
	function = function << SW64_BPF_SIMPLE_ALU_FUNC_OFFSET;
	return opcode | ra | rb | function | rc;
}

static inline u32 sw64_bpf_gen_format_simple_alu_imm(int opcode, enum sw64_bpf_registers ra,
		u32 imm, enum sw64_bpf_registers rc, int function)
{
	opcode = opcode << SW64_BPF_OPCODE_OFFSET;
	ra = ra << SW64_BPF_RA_OFFSET;
	imm = (imm & 0xff) << SW64_BPF_SIMPLE_ALU_IMM_OFFSET;
	rc = rc << SW64_BPF_SIMPLE_ALU_RC_OFFSET;
	function = function << SW64_BPF_SIMPLE_ALU_FUNC_OFFSET;
	return opcode | ra | imm | function | rc;
}

static inline void emit(const u32 insn, struct jit_ctx *ctx)
{
	if (ctx->image != NULL)
		ctx->image[ctx->idx] = insn;

	ctx->idx++;
}

static inline int get_tmp_reg(struct jit_ctx *ctx)
{
	ctx->current_tmp_reg++;
	/* Do not use 22-25. Should be more than enough. */
	if (unlikely(ctx->current_tmp_reg == 8)) {
		pr_err("eBPF JIT %s[%d]: not enough temporary registers!\n",
				current->comm, current->pid);
		return -1;
	}
	return ctx->current_tmp_reg;
}

static inline void put_tmp_reg(struct jit_ctx *ctx)
{
	ctx->current_tmp_reg--;
	if (ctx->current_tmp_reg == 21)
		ctx->current_tmp_reg = 7;
}

static void emit_sw64_lds32(const int dst, const s32 imm, struct jit_ctx *ctx)
{
	s16 hi = imm >> 16;
	s16 lo = imm & 0xffff;

	if (!imm)
		return emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, dst), ctx);

	if (imm >= S16_MIN && imm <= S16_MAX)
		return emit(SW64_BPF_LDI(dst, SW64_BPF_REG_ZR, imm), ctx);

	if (lo < 0) {
		if (hi == S16_MAX) {
			emit(SW64_BPF_LDIH(dst, SW64_BPF_REG_ZR, (hi >> 1) + 1), ctx);
			emit(SW64_BPF_ADDL_REG(dst, dst, dst), ctx);
		} else {
			emit(SW64_BPF_LDIH(dst, SW64_BPF_REG_ZR, hi + 1), ctx);
		}
	} else {
		emit(SW64_BPF_LDIH(dst, SW64_BPF_REG_ZR, hi), ctx);
	}
	if (lo)
		emit(SW64_BPF_LDI(dst, dst, lo), ctx);
}

static void emit_sw64_ldu32(const int dst, const u32 imm, struct jit_ctx *ctx)
{
	emit_sw64_lds32(dst, imm, ctx);
	if ((s32)imm < 0)
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
}

/*
 * Load high 32 bits for emit_sw64_ldu64().
 *
 * Same as emit_sw64_lds32() except this one does not worry about sign extension
 * and generates a 32 bits left shift at the end.
 */
static void emit_sw64_ld32_hi(const int dst, const s32 imm, struct jit_ctx *ctx)
{
	s16 hi = imm >> 16;
	s16 lo = imm & 0xffff;

	if (!imm)
		return emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, dst), ctx);

	if (imm >= S16_MIN && imm <= S16_MAX) {
		emit(SW64_BPF_LDI(dst, SW64_BPF_REG_ZR, imm), ctx);
		goto out;
	}

	if (lo < 0)
		hi++;
	emit(SW64_BPF_LDIH(dst, SW64_BPF_REG_ZR, hi), ctx);
	if (lo)
		emit(SW64_BPF_LDI(dst, dst, lo), ctx);
out:
	emit(SW64_BPF_SLL_IMM(dst, 32, dst), ctx);
}

static void emit_sw64_ldu64(const int dst, const u64 imm, struct jit_ctx *ctx)
{
	u32 hi = imm >> 32;
	u32 lo = imm & 0xffffffff;
	u8 reg_tmp;

	if (!hi)
		return emit_sw64_ldu32(dst, lo, ctx);

	if (!lo)
		return emit_sw64_ld32_hi(dst, hi, ctx);

	reg_tmp = get_tmp_reg(ctx);

	emit_sw64_ld32_hi(dst, hi, ctx);
	emit_sw64_ldu32(reg_tmp, lo, ctx);
	emit(SW64_BPF_BIS_REG(dst, reg_tmp, dst), ctx);

	put_tmp_reg(ctx);
}

/* constant insn count */
static void emit_sw64_load_call_addr(u8 dst, u64 addr, struct jit_ctx *ctx)
{
	u16 imm_tmp;
	u8 reg_tmp = get_tmp_reg(ctx);

	imm_tmp = (addr >> 60) & 0xf;
	emit(SW64_BPF_LDI(dst, SW64_BPF_REG_ZR, imm_tmp), ctx);
	emit(SW64_BPF_SLL_IMM(dst, 60, dst), ctx);

	imm_tmp = (addr >> 45) & 0x7fff;
	emit(SW64_BPF_LDI(reg_tmp, SW64_BPF_REG_ZR, imm_tmp), ctx);
	emit(SW64_BPF_SLL_IMM(reg_tmp, 45, reg_tmp), ctx);
	emit(SW64_BPF_ADDL_REG(dst, reg_tmp, dst), ctx);

	imm_tmp = (addr >> 30) & 0x7fff;
	emit(SW64_BPF_LDI(reg_tmp, SW64_BPF_REG_ZR, imm_tmp), ctx);
	emit(SW64_BPF_SLL_IMM(reg_tmp, 30, reg_tmp), ctx);
	emit(SW64_BPF_ADDL_REG(dst, reg_tmp, dst), ctx);

	imm_tmp = (addr >> 15) & 0x7fff;
	emit(SW64_BPF_LDI(reg_tmp, SW64_BPF_REG_ZR, imm_tmp), ctx);
	emit(SW64_BPF_SLL_IMM(reg_tmp, 15, reg_tmp), ctx);
	emit(SW64_BPF_ADDL_REG(dst, reg_tmp, dst), ctx);

	imm_tmp = addr & 0x7fff;
	emit(SW64_BPF_LDI(dst, dst, imm_tmp), ctx);

	put_tmp_reg(ctx);
}

#if defined(CONFIG_SUBARCH_C3B)
/* Do not change!!! See arch/sw_64/lib/divide.S for more detail */
#define REG(x)		"$"str(x)
#define str(x)		#x
#define DIV_RET_ADDR	23
#define DIVIDEND	24
#define DIVISOR		25
#define RESULT		27

#include <asm/asm-prototypes.h>
static void emit_sw64_divmod(const int dst, const int src, struct jit_ctx *ctx, u8 code)
{
	emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, dst, DIVIDEND), ctx);
	emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, src, DIVISOR), ctx);
	switch (BPF_CLASS(code)) {
	case BPF_ALU:
		switch (BPF_OP(code)) {
		case BPF_DIV:
			emit_sw64_ldu64(SW64_BPF_REG_PV, (u64)__divwu, ctx);
			break;
		case BPF_MOD:
			emit_sw64_ldu64(SW64_BPF_REG_PV, (u64)__remwu, ctx);
			break;
		}
		emit(SW64_BPF_CALL(DIV_RET_ADDR, SW64_BPF_REG_PV), ctx);
		emit(SW64_BPF_ZAP_IMM(RESULT, 0xf0, dst), ctx);
		break;
	case BPF_ALU64:
		switch (BPF_OP(code)) {
		case BPF_DIV:
			emit_sw64_ldu64(SW64_BPF_REG_PV, (u64)__divlu, ctx);
			break;
		case BPF_MOD:
			emit_sw64_ldu64(SW64_BPF_REG_PV, (u64)__remlu, ctx);
			break;
		}
		emit(SW64_BPF_CALL(DIV_RET_ADDR, SW64_BPF_REG_PV), ctx);
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, RESULT, dst), ctx);
		break;
	}
}

#undef REG
#undef str
#undef DIVIDEND
#undef DIVISOR
#undef RESULT
#endif

/* STX XADD: lock *(u32 *)(dst + off) += src */
static void emit_sw64_xadd32(const int src, int dst, s16 off, struct jit_ctx *ctx)
{
	int atomic_start;
	int atomic_end;
	u8 tmp1 = get_tmp_reg(ctx);
	u8 tmp2 = get_tmp_reg(ctx);
	u8 __maybe_unused tmp3 = get_tmp_reg(ctx);

	if (off < -0x800 || off > 0x7ff) {
		emit(SW64_BPF_LDI(tmp1, dst, off), ctx);
		dst = tmp1;
		off = 0;
	}

	atomic_start = ctx->idx;
	emit(SW64_BPF_LLDW(tmp2, dst, off), ctx);
#if defined(CONFIG_SUBARCH_C3B)
	emit(SW64_BPF_LDI(tmp3, SW64_BPF_REG_ZR, 1), ctx);
	emit(SW64_BPF_WR_F(tmp3), ctx);
#endif
	emit(SW64_BPF_ADDW_REG(tmp2, src, tmp2), ctx);
	if (ctx->idx & 1)
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, SW64_BPF_REG_ZR), ctx);
	emit(SW64_BPF_LSTW(tmp2, dst, off), ctx);
#if defined(CONFIG_SUBARCH_C3B)
	emit(SW64_BPF_RD_F(tmp2), ctx);
#endif
	atomic_end = ctx->idx;
	emit(SW64_BPF_BEQ(tmp2, atomic_start - atomic_end - 1), ctx);

	put_tmp_reg(ctx);
	put_tmp_reg(ctx);
	put_tmp_reg(ctx);
}

/* STX XADD: lock *(u64 *)(dst + off) += src */
static void emit_sw64_xadd64(const int src, int dst, s16 off, struct jit_ctx *ctx)
{
	int atomic_start;
	int atomic_end;
	u8 tmp1 = get_tmp_reg(ctx);
	u8 tmp2 = get_tmp_reg(ctx);
	u8 __maybe_unused tmp3 = get_tmp_reg(ctx);

	if (off < -0x800 || off > 0x7ff) {
		emit(SW64_BPF_LDI(tmp1, dst, off), ctx);
		dst = tmp1;
		off = 0;
	}

	atomic_start = ctx->idx;
	emit(SW64_BPF_LLDL(tmp2, dst, off), ctx);
#if defined(CONFIG_SUBARCH_C3B)
	emit(SW64_BPF_LDI(tmp3, SW64_BPF_REG_ZR, 1), ctx);
	emit(SW64_BPF_WR_F(tmp3), ctx);
#endif
	emit(SW64_BPF_ADDL_REG(tmp2, src, tmp2), ctx);
	if (ctx->idx & 1)
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, SW64_BPF_REG_ZR), ctx);
	emit(SW64_BPF_LSTL(tmp2, dst, off), ctx);
#if defined(CONFIG_SUBARCH_C3B)
	emit(SW64_BPF_RD_F(tmp2), ctx);
#endif
	atomic_end = ctx->idx;
	emit(SW64_BPF_BEQ(tmp2, atomic_start - atomic_end - 1), ctx);

	put_tmp_reg(ctx);
	put_tmp_reg(ctx);
	put_tmp_reg(ctx);
}

#if defined(CONFIG_SUBARCH_C3B)
static void emit_sw64_htobe16(const int dst, struct jit_ctx *ctx)
{
	u8 tmp = get_tmp_reg(ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x2, tmp), ctx);
	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x1, dst), ctx);
	emit(SW64_BPF_SRL_IMM(tmp, 8, tmp), ctx);
	emit(SW64_BPF_SLL_IMM(dst, 8, dst), ctx);
	emit(SW64_BPF_BIS_REG(dst, tmp, dst), ctx);

	put_tmp_reg(ctx);
}

static void emit_sw64_htobe32(const int dst, struct jit_ctx *ctx)
{
	u8 tmp1 = get_tmp_reg(ctx);
	u8 tmp2 = get_tmp_reg(ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x8, tmp1), ctx);
	emit(SW64_BPF_SRL_IMM(tmp1, 24, tmp2), ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x4, tmp1), ctx);
	emit(SW64_BPF_SRL_IMM(tmp1, 8, tmp1), ctx);
	emit(SW64_BPF_BIS_REG(tmp2, tmp1, tmp2), ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x2, tmp1), ctx);
	emit(SW64_BPF_SLL_IMM(tmp1, 8, tmp1), ctx);
	emit(SW64_BPF_BIS_REG(tmp2, tmp1, tmp2), ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x1, dst), ctx);
	emit(SW64_BPF_SLL_IMM(dst, 24, dst), ctx);
	emit(SW64_BPF_BIS_REG(dst, tmp2, dst), ctx);

	put_tmp_reg(ctx);
	put_tmp_reg(ctx);
}

static void emit_sw64_htobe64(const int dst, struct jit_ctx *ctx)
{
	u8 tmp1 = get_tmp_reg(ctx);
	u8 tmp2 = get_tmp_reg(ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x80, tmp1), ctx);
	emit(SW64_BPF_SRL_IMM(tmp1, 56, tmp2), ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x40, tmp1), ctx);
	emit(SW64_BPF_SRL_IMM(tmp1, 40, tmp1), ctx);
	emit(SW64_BPF_BIS_REG(tmp2, tmp1, tmp2), ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x20, tmp1), ctx);
	emit(SW64_BPF_SRL_IMM(tmp1, 24, tmp1), ctx);
	emit(SW64_BPF_BIS_REG(tmp2, tmp1, tmp2), ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x10, tmp1), ctx);
	emit(SW64_BPF_SRL_IMM(tmp1, 8, tmp1), ctx);
	emit(SW64_BPF_BIS_REG(tmp2, tmp1, tmp2), ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x08, tmp1), ctx);
	emit(SW64_BPF_SLL_IMM(tmp1, 8, tmp1), ctx);
	emit(SW64_BPF_BIS_REG(tmp2, tmp1, tmp2), ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x04, tmp1), ctx);
	emit(SW64_BPF_SLL_IMM(tmp1, 24, tmp1), ctx);
	emit(SW64_BPF_BIS_REG(tmp2, tmp1, tmp2), ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x02, tmp1), ctx);
	emit(SW64_BPF_SLL_IMM(tmp1, 40, tmp1), ctx);
	emit(SW64_BPF_BIS_REG(tmp2, tmp1, tmp2), ctx);

	emit(SW64_BPF_ZAPNOT_IMM(dst, 0x01, dst), ctx);
	emit(SW64_BPF_SLL_IMM(dst, 56, dst), ctx);
	emit(SW64_BPF_BIS_REG(dst, tmp2, dst), ctx);

	put_tmp_reg(ctx);
	put_tmp_reg(ctx);
}
#endif

static void jit_fill_hole(void *area, unsigned int size)
{
	unsigned long c = SW64_BPF_ILLEGAL_INSN;

	c |= c << 32;
	__constant_c_memset(area, c, size);
}

static int offset_to_epilogue(const struct jit_ctx *ctx);
static int bpf2sw64_offset(int bpf_idx, s32 off, const struct jit_ctx *ctx)
{
	int from = ctx->insn_offset[bpf_idx + 1];
	int to = ctx->insn_offset[bpf_idx + 1 + off];

	if (ctx->image == NULL)
		return 0;

	return to - from;
}

static int offset_to_epilogue(const struct jit_ctx *ctx)
{
	if (ctx->image == NULL)
		return 0;

	return ctx->epilogue_offset - ctx->idx;
}

/* For tail call, jump to set up function call stack */
#define PROLOGUE_OFFSET	(11 + SW64_FENTRY_NINSNS)

static void build_prologue(struct jit_ctx *ctx, bool was_classic)
{
	int i;
	const u8 r6 = bpf2sw64[BPF_REG_6];
	const u8 r7 = bpf2sw64[BPF_REG_7];
	const u8 r8 = bpf2sw64[BPF_REG_8];
	const u8 r9 = bpf2sw64[BPF_REG_9];
	const u8 fp = bpf2sw64[BPF_REG_FP];
	const u8 tcc = bpf2sw64[TCALL_CNT];

	/* nops reserved for fentry call */
	for (i = 0; i < SW64_FENTRY_NINSNS; i++)
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, SW64_BPF_REG_ZR), ctx);

	/* Save callee-saved registers */
	emit(SW64_BPF_LDI(SW64_BPF_REG_SP, SW64_BPF_REG_SP, -64), ctx);
	emit(SW64_BPF_STL(SW64_BPF_REG_RA, SW64_BPF_REG_SP, 0), ctx);
	emit(SW64_BPF_STL(fp, SW64_BPF_REG_SP, 8), ctx);
	emit(SW64_BPF_STL(r6, SW64_BPF_REG_SP, 16), ctx);
	emit(SW64_BPF_STL(r7, SW64_BPF_REG_SP, 24), ctx);
	emit(SW64_BPF_STL(r8, SW64_BPF_REG_SP, 32), ctx);
	emit(SW64_BPF_STL(r9, SW64_BPF_REG_SP, 40), ctx);
	emit(SW64_BPF_STL(tcc, SW64_BPF_REG_SP, 48), ctx);
	emit(SW64_BPF_STL(SW64_BPF_REG_GP, SW64_BPF_REG_SP, 56), ctx);

	/* Set up BPF prog stack base register */
	emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_SP, fp), ctx);
	if (!was_classic)
		/* Initialize tail_call_cnt */
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, tcc), ctx);

	/* Set up function call stack */
	ctx->stack_size = (ctx->prog->aux->stack_depth + 15) & (~15);
	emit(SW64_BPF_LDI(SW64_BPF_REG_SP, SW64_BPF_REG_SP, -ctx->stack_size), ctx);
}

static void build_epilogue(struct jit_ctx *ctx)
{
	const u8 r6 = bpf2sw64[BPF_REG_6];
	const u8 r7 = bpf2sw64[BPF_REG_7];
	const u8 r8 = bpf2sw64[BPF_REG_8];
	const u8 r9 = bpf2sw64[BPF_REG_9];
	const u8 fp = bpf2sw64[BPF_REG_FP];
	const u8 tcc = bpf2sw64[TCALL_CNT];

	/* Destroy function call stack */
	emit(SW64_BPF_LDI(SW64_BPF_REG_SP, SW64_BPF_REG_SP, ctx->stack_size), ctx);

	/* Restore callee-saved registers */
	emit(SW64_BPF_LDL(SW64_BPF_REG_RA, SW64_BPF_REG_SP, 0), ctx);
	emit(SW64_BPF_LDL(fp, SW64_BPF_REG_SP, 8), ctx);
	emit(SW64_BPF_LDL(r6, SW64_BPF_REG_SP, 16), ctx);
	emit(SW64_BPF_LDL(r7, SW64_BPF_REG_SP, 24), ctx);
	emit(SW64_BPF_LDL(r8, SW64_BPF_REG_SP, 32), ctx);
	emit(SW64_BPF_LDL(r9, SW64_BPF_REG_SP, 40), ctx);
	emit(SW64_BPF_LDL(tcc, SW64_BPF_REG_SP, 48), ctx);
	emit(SW64_BPF_LDL(SW64_BPF_REG_GP, SW64_BPF_REG_SP, 56), ctx);
	emit(SW64_BPF_LDI(SW64_BPF_REG_SP, SW64_BPF_REG_SP, 64), ctx);

	/* Return */
	emit(SW64_BPF_RET(SW64_BPF_REG_RA), ctx);
}

static int emit_bpf_tail_call(struct jit_ctx *ctx)
{
	/* bpf_tail_call(void *ctx, struct bpf_map *prog_array_map, u32 index) */
	const u8 r2 = bpf2sw64[BPF_REG_2];	/* struct bpf_array *array */
	const u8 r3 = bpf2sw64[BPF_REG_3];	/* u32 index */

	const u8 tmp = get_tmp_reg(ctx);
	const u8 prg = get_tmp_reg(ctx);
	const u8 tcc = bpf2sw64[TCALL_CNT];
	u64 offset;
	static int out_idx;
#define out_offset	(ctx->image ? (out_idx - ctx->idx - 1) : 0)

	/* if (index >= array->map.max_entries)
	 *     goto out;
	 */
	offset = offsetof(struct bpf_array, map.max_entries);
	emit_sw64_ldu64(tmp, offset, ctx);
	emit(SW64_BPF_ADDL_REG(r2, tmp, tmp), ctx);	/* tmp = r2 + tmp = &map.max_entries */
	emit(SW64_BPF_LDW(tmp, tmp, 0), ctx);		/* tmp = *tmp = map.max_entries */
	emit(SW64_BPF_ZAP_IMM(tmp, 0xf0, tmp), ctx);	/* map.max_entries is u32 */
	emit(SW64_BPF_ZAP_IMM(r3, 0xf0, r3), ctx);	/* index is u32 */
	emit(SW64_BPF_CMPULE_REG(tmp, r3, tmp), ctx);
	emit(SW64_BPF_BNE(tmp, out_offset), ctx);

	/* if (tail_call_cnt > MAX_TAIL_CALL_CNT)
	 *     goto out;
	 * tail_call_cnt++;
	 */
	emit_sw64_ldu64(tmp, MAX_TAIL_CALL_CNT, ctx);
	emit(SW64_BPF_CMPULT_REG(tmp, tcc, tmp), ctx);
	emit(SW64_BPF_BNE(tmp, out_offset), ctx);
	emit(SW64_BPF_ADDL_IMM(tcc, 1, tcc), ctx);

	/* prog = array->ptrs[index];
	 * if (prog == NULL)
	 *     goto out;
	 */
	offset = offsetof(struct bpf_array, ptrs);
	emit_sw64_ldu64(tmp, offset, ctx);
	emit(SW64_BPF_ADDL_REG(r2, tmp, tmp), ctx);	/* tmp = r2 + tmp = &ptrs[0] */
	emit(SW64_BPF_SLL_IMM(r3, 3, prg), ctx);	/* prg = r3 * 8, each entry is a pointer */
	emit(SW64_BPF_ADDL_REG(tmp, prg, prg), ctx);	/* prg = tmp + prg = &ptrs[index] */
	emit(SW64_BPF_LDL(prg, prg, 0), ctx);		/* prg = *prg = ptrs[index] = prog */
	emit(SW64_BPF_BEQ(prg, out_offset), ctx);

	/* goto *(prog->bpf_func + prologue_offset); */
	offset = offsetof(struct bpf_prog, bpf_func);
	emit_sw64_ldu64(tmp, offset, ctx);
	emit(SW64_BPF_ADDL_REG(prg, tmp, tmp), ctx);	/* tmp = prg + tmp = &bpf_func */
	emit(SW64_BPF_LDL(tmp, tmp, 0), ctx);		/* tmp = *tmp = bpf_func */
	emit(SW64_BPF_BEQ(tmp, out_offset), ctx);
	emit(SW64_BPF_LDI(tmp, tmp, sizeof(u32) * PROLOGUE_OFFSET), ctx);
	emit(SW64_BPF_LDI(SW64_BPF_REG_SP, SW64_BPF_REG_SP, ctx->stack_size), ctx);
	emit(SW64_BPF_JMP(SW64_BPF_REG_ZR, tmp), ctx);

	put_tmp_reg(ctx);
	put_tmp_reg(ctx);

	/* out */
	if (ctx->image == NULL)
		out_idx = ctx->idx;
	if (ctx->image != NULL && out_idx <= 0)
		return -1;
#undef out_offset
	return 0;
}

/* For accesses to BTF pointers, add an entry to the exception table */
static int add_exception_handler(const struct bpf_insn *insn,
				 struct jit_ctx *ctx,
				 int dst_reg)
{
	off_t offset;
	unsigned long pc;
	struct exception_table_entry *ex;

	if (!ctx->image)
		/* First pass */
		return 0;

	if (!ctx->prog->aux->extable || BPF_MODE(insn->code) != BPF_PROBE_MEM)
		return 0;

	if (WARN_ON_ONCE(ctx->exentry_idx >= ctx->prog->aux->num_exentries))
		return -EINVAL;

	ex = &ctx->prog->aux->extable[ctx->exentry_idx];
	pc = (unsigned long)&ctx->image[ctx->idx - 1];

	offset = pc - (long)&ex->insn;
	ex->insn = offset;

	ex->fixup.bits.nextinsn = sizeof(u32);
	ex->fixup.bits.valreg = dst_reg;
	ex->fixup.bits.errreg = SW64_BPF_REG_ZR;

	ctx->exentry_idx++;
	return 0;
}

/* JITs an eBPF instruction.
 * Returns:
 * 0  - successfully JITed an 8-byte eBPF instruction.
 * >0 - successfully JITed a 16-byte eBPF instruction.
 * <0 - failed to JIT.
 */
static int build_insn(const struct bpf_insn *insn, struct jit_ctx *ctx,
		bool extra_pass)
{
	const u8 code = insn->code;
	u8 dst = bpf2sw64[insn->dst_reg];
	u8 src = bpf2sw64[insn->src_reg];
	const u8 tmp1 __maybe_unused = get_tmp_reg(ctx);
	const u8 tmp2 __maybe_unused = get_tmp_reg(ctx);
	const s16 off = insn->off;
	const s32 imm = insn->imm;
	const int bpf_idx = insn - ctx->prog->insnsi;
	s32 jmp_offset;
	struct bpf_insn insn1;
	u64 imm64;
	int ret;

	switch (code) {
	case BPF_ALU | BPF_MOV | BPF_X:
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, src, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_MOV | BPF_X:
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, src, dst), ctx);
		break;
	case BPF_ALU | BPF_ADD | BPF_X:
		emit(SW64_BPF_ADDW_REG(dst, src, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_ADD | BPF_X:
		emit(SW64_BPF_ADDL_REG(dst, src, dst), ctx);
		break;
	case BPF_ALU | BPF_SUB | BPF_X:
		emit(SW64_BPF_SUBW_REG(dst, src, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_SUB | BPF_X:
		emit(SW64_BPF_SUBL_REG(dst, src, dst), ctx);
		break;
	case BPF_ALU | BPF_MUL | BPF_X:
		emit(SW64_BPF_MULW_REG(dst, src, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_MUL | BPF_X:
		emit(SW64_BPF_MULL_REG(dst, src, dst), ctx);
		break;
	case BPF_ALU | BPF_DIV | BPF_X:
#if defined(CONFIG_SUBARCH_C3B)
		emit_sw64_divmod(dst, src, ctx, code);
#else
		emit(SW64_BPF_UDIVW_REG(dst, src, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
#endif
		break;
	case BPF_ALU64 | BPF_DIV | BPF_X:
#if defined(CONFIG_SUBARCH_C3B)
		emit_sw64_divmod(dst, src, ctx, code);
#else
		emit(SW64_BPF_UDIVL_REG(dst, src, dst), ctx);
#endif
		break;
	case BPF_ALU | BPF_MOD | BPF_X:
#if defined(CONFIG_SUBARCH_C3B)
		emit_sw64_divmod(dst, src, ctx, code);
#else
		emit(SW64_BPF_UREMW_REG(dst, src, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
#endif
		break;
	case BPF_ALU64 | BPF_MOD | BPF_X:
#if defined(CONFIG_SUBARCH_C3B)
		emit_sw64_divmod(dst, src, ctx, code);
#else
		emit(SW64_BPF_UREML_REG(dst, src, dst), ctx);
#endif
		break;
	case BPF_ALU | BPF_LSH | BPF_X:
		emit(SW64_BPF_SLL_REG(dst, src, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_LSH | BPF_X:
		emit(SW64_BPF_SLL_REG(dst, src, dst), ctx);
		break;
	case BPF_ALU | BPF_RSH | BPF_X:
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		fallthrough;
	case BPF_ALU64 | BPF_RSH | BPF_X:
		emit(SW64_BPF_SRL_REG(dst, src, dst), ctx);
		break;
	case BPF_ALU | BPF_ARSH | BPF_X:
		emit(SW64_BPF_ADDW_REG(SW64_BPF_REG_ZR, dst, dst), ctx);
		emit(SW64_BPF_SRA_REG(dst, src, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_ARSH | BPF_X:
		emit(SW64_BPF_SRA_REG(dst, src, dst), ctx);
		break;
	case BPF_ALU | BPF_AND | BPF_X:
		emit(SW64_BPF_AND_REG(dst, src, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_AND | BPF_X:
		emit(SW64_BPF_AND_REG(dst, src, dst), ctx);
		break;
	case BPF_ALU | BPF_OR | BPF_X:
		emit(SW64_BPF_BIS_REG(dst, src, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_OR | BPF_X:
		emit(SW64_BPF_BIS_REG(dst, src, dst), ctx);
		break;
	case BPF_ALU | BPF_XOR | BPF_X:
		emit(SW64_BPF_XOR_REG(dst, src, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_XOR | BPF_X:
		emit(SW64_BPF_XOR_REG(dst, src, dst), ctx);
		break;
	case BPF_ALU | BPF_NEG:
		emit(SW64_BPF_SUBW_REG(SW64_BPF_REG_ZR, dst, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_NEG:
		emit(SW64_BPF_SUBL_REG(SW64_BPF_REG_ZR, dst, dst), ctx);
		break;
	case BPF_ALU | BPF_END | BPF_TO_LE:
		switch (imm) {
		case 16:
			emit(SW64_BPF_ZAPNOT_IMM(dst, 0x3, dst), ctx);
			break;
		case 32:
			emit(SW64_BPF_ZAPNOT_IMM(dst, 0xf, dst), ctx);
			break;
		case 64:
			break;
		default:
			pr_err("eBPF JIT %s[%d]: BPF_TO_LE unknown size\n",
					current->comm, current->pid);
			return -EINVAL;
		}
		break;
	case BPF_ALU | BPF_END | BPF_TO_BE:
		switch (imm) {
		case 16:
#if defined(CONFIG_SUBARCH_C3B)
			emit_sw64_htobe16(dst, ctx);
#else
			emit(SW64_BPF_REVBH_REG(dst, dst), ctx);
#endif
			break;
		case 32:
#if defined(CONFIG_SUBARCH_C3B)
			emit_sw64_htobe32(dst, ctx);
#else
			emit(SW64_BPF_REVBW_REG(dst, dst), ctx);
			emit(SW64_BPF_ZAPNOT_IMM(dst, 0xf, dst), ctx);
#endif
			break;
		case 64:
#if defined(CONFIG_SUBARCH_C3B)
			emit_sw64_htobe64(dst, ctx);
#else
			emit(SW64_BPF_REVBL_REG(dst, dst), ctx);
#endif
			break;
		default:
			pr_err("eBPF JIT %s[%d]: BPF_TO_BE unknown size\n",
					current->comm, current->pid);
			return -EINVAL;
		}
		break;

	case BPF_ALU | BPF_MOV | BPF_K:
		if (imm >= S16_MIN && imm <= S16_MAX)
			emit(SW64_BPF_LDI(dst, SW64_BPF_REG_ZR, imm), ctx);
		else
			emit_sw64_ldu32(dst, imm, ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_MOV | BPF_K:
		if (imm >= S16_MIN && imm <= S16_MAX)
			emit(SW64_BPF_LDI(dst, SW64_BPF_REG_ZR, imm), ctx);
		else
			emit_sw64_lds32(dst, imm, ctx);
		break;
	case BPF_ALU | BPF_ADD | BPF_K:
		if (imm >= S16_MIN && imm <= S16_MAX) {
			emit(SW64_BPF_LDI(dst, dst, imm), ctx);
		} else {
			emit_sw64_ldu32(tmp1, imm, ctx);
			emit(SW64_BPF_ADDW_REG(dst, tmp1, dst), ctx);
		}
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_ADD | BPF_K:
		if (imm >= S16_MIN && imm <= S16_MAX) {
			emit(SW64_BPF_LDI(dst, dst, imm), ctx);
		} else {
			emit_sw64_lds32(tmp1, imm, ctx);
			emit(SW64_BPF_ADDL_REG(dst, tmp1, dst), ctx);
		}
		break;
	case BPF_ALU | BPF_SUB | BPF_K:
		if (imm >= -S16_MAX && imm <= -S16_MIN) {
			emit(SW64_BPF_LDI(dst, dst, -imm), ctx);
		} else {
			emit_sw64_ldu32(tmp1, imm, ctx);
			emit(SW64_BPF_SUBL_REG(dst, tmp1, dst), ctx);
		}
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_SUB | BPF_K:
		if (imm >= -S16_MAX && imm <= -S16_MIN) {
			emit(SW64_BPF_LDI(dst, dst, -imm), ctx);
		} else {
			emit_sw64_lds32(tmp1, imm, ctx);
			emit(SW64_BPF_SUBL_REG(dst, tmp1, dst), ctx);
		}
		break;
	case BPF_ALU | BPF_MUL | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_MULL_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_ldu32(tmp1, imm, ctx);
			emit(SW64_BPF_MULL_REG(dst, tmp1, dst), ctx);
		}
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_MUL | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_MULL_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_lds32(tmp1, imm, ctx);
			emit(SW64_BPF_MULL_REG(dst, tmp1, dst), ctx);
		}
		break;
	case BPF_ALU | BPF_DIV | BPF_K:
		emit_sw64_ldu32(tmp1, imm, ctx);
#if defined(CONFIG_SUBARCH_C3B)
		emit_sw64_divmod(dst, tmp1, ctx, code);
#else
		emit(SW64_BPF_UDIVW_REG(dst, tmp1, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
#endif
		break;
	case BPF_ALU64 | BPF_DIV | BPF_K:
		emit_sw64_lds32(tmp1, imm, ctx);
#if defined(CONFIG_SUBARCH_C3B)
		emit_sw64_divmod(dst, tmp1, ctx, code);
#else
		emit(SW64_BPF_UDIVL_REG(dst, tmp1, dst), ctx);
#endif
		break;
	case BPF_ALU | BPF_MOD | BPF_K:
		emit_sw64_ldu32(tmp1, imm, ctx);
#if defined(CONFIG_SUBARCH_C3B)
		emit_sw64_divmod(dst, tmp1, ctx, code);
#else
		emit(SW64_BPF_UREMW_REG(dst, tmp1, dst), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
#endif
		break;
	case BPF_ALU64 | BPF_MOD | BPF_K:
		emit_sw64_lds32(tmp1, imm, ctx);
#if defined(CONFIG_SUBARCH_C3B)
		emit_sw64_divmod(dst, tmp1, ctx, code);
#else
		emit(SW64_BPF_UREML_REG(dst, tmp1, dst), ctx);
#endif
		break;
	case BPF_ALU | BPF_LSH | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_SLL_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_ldu32(tmp1, imm, ctx);
			emit(SW64_BPF_SLL_REG(dst, tmp1, dst), ctx);
		}
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_LSH | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_SLL_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_lds32(tmp1, imm, ctx);
			emit(SW64_BPF_SLL_REG(dst, tmp1, dst), ctx);
		}
		break;
	case BPF_ALU | BPF_RSH | BPF_K:
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_SRL_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_ldu32(tmp1, imm, ctx);
			emit(SW64_BPF_SRL_REG(dst, tmp1, dst), ctx);
		}
		break;
	case BPF_ALU64 | BPF_RSH | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_SRL_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_lds32(tmp1, imm, ctx);
			emit(SW64_BPF_SRL_REG(dst, tmp1, dst), ctx);
		}
		break;
	case BPF_ALU | BPF_ARSH | BPF_K:
		emit(SW64_BPF_ADDW_REG(SW64_BPF_REG_ZR, dst, dst), ctx);
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_SRA_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_ldu32(tmp1, imm, ctx);
			emit(SW64_BPF_SRA_REG(dst, tmp1, dst), ctx);
		}
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_ARSH | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_SRA_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_lds32(tmp1, imm, ctx);
			emit(SW64_BPF_SRA_REG(dst, tmp1, dst), ctx);
		}
		break;
	case BPF_ALU | BPF_AND | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_AND_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_ldu32(tmp1, imm, ctx);
			emit(SW64_BPF_AND_REG(dst, tmp1, dst), ctx);
		}
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_AND | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_AND_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_lds32(tmp1, imm, ctx);
			emit(SW64_BPF_AND_REG(dst, tmp1, dst), ctx);
		}
		break;
	case BPF_ALU | BPF_OR | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_BIS_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_ldu32(tmp1, imm, ctx);
			emit(SW64_BPF_BIS_REG(dst, tmp1, dst), ctx);
		}
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_OR | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_BIS_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_lds32(tmp1, imm, ctx);
			emit(SW64_BPF_BIS_REG(dst, tmp1, dst), ctx);
		}
		break;
	case BPF_ALU | BPF_XOR | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_XOR_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_ldu32(tmp1, imm, ctx);
			emit(SW64_BPF_XOR_REG(dst, tmp1, dst), ctx);
		}
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		break;
	case BPF_ALU64 | BPF_XOR | BPF_K:
		if (imm >= 0 && imm <= U8_MAX) {
			emit(SW64_BPF_XOR_IMM(dst, imm, dst), ctx);
		} else {
			emit_sw64_lds32(tmp1, imm, ctx);
			emit(SW64_BPF_XOR_REG(dst, tmp1, dst), ctx);
		}
		break;

	case BPF_JMP | BPF_JA:
		jmp_offset = bpf2sw64_offset(bpf_idx, off, ctx);
		if (jmp_offset >= -0x100000 && jmp_offset <= 0xfffff) {
			emit(SW64_BPF_BR(SW64_BPF_REG_ZR, jmp_offset), ctx);
		} else {
			pr_err("eBPF JIT %s[%d]: BPF_JMP out of range, %d instructions\n",
					current->comm, current->pid, jmp_offset);
			return -EINVAL;
		}
		break;

	case BPF_JMP32 | BPF_JEQ | BPF_X:
	case BPF_JMP32 | BPF_JGT | BPF_X:
	case BPF_JMP32 | BPF_JLT | BPF_X:
	case BPF_JMP32 | BPF_JGE | BPF_X:
	case BPF_JMP32 | BPF_JLE | BPF_X:
	case BPF_JMP32 | BPF_JNE | BPF_X:
	case BPF_JMP32 | BPF_JSGT | BPF_X:
	case BPF_JMP32 | BPF_JSLT | BPF_X:
	case BPF_JMP32 | BPF_JSGE | BPF_X:
	case BPF_JMP32 | BPF_JSLE | BPF_X:
	case BPF_JMP32 | BPF_JSET | BPF_X:
		emit(SW64_BPF_ADDW_REG(SW64_BPF_REG_ZR, src, tmp1), ctx);
		src = tmp1;
		emit(SW64_BPF_ADDW_REG(SW64_BPF_REG_ZR, dst, tmp2), ctx);
		dst = tmp2;
		fallthrough;
	case BPF_JMP | BPF_JEQ | BPF_X:
	case BPF_JMP | BPF_JGT | BPF_X:
	case BPF_JMP | BPF_JLT | BPF_X:
	case BPF_JMP | BPF_JGE | BPF_X:
	case BPF_JMP | BPF_JLE | BPF_X:
	case BPF_JMP | BPF_JNE | BPF_X:
	case BPF_JMP | BPF_JSGT | BPF_X:
	case BPF_JMP | BPF_JSLT | BPF_X:
	case BPF_JMP | BPF_JSGE | BPF_X:
	case BPF_JMP | BPF_JSLE | BPF_X:
	case BPF_JMP | BPF_JSET | BPF_X:
		switch (BPF_OP(code)) {
		case BPF_JEQ:
			emit(SW64_BPF_CMPEQ_REG(dst, src, tmp1), ctx);
			break;
		case BPF_JGT:
			emit(SW64_BPF_CMPULT_REG(src, dst, tmp1), ctx);
			break;
		case BPF_JLT:
			emit(SW64_BPF_CMPULT_REG(dst, src, tmp1), ctx);
			break;
		case BPF_JGE:
			emit(SW64_BPF_CMPULE_REG(src, dst, tmp1), ctx);
			break;
		case BPF_JLE:
			emit(SW64_BPF_CMPULE_REG(dst, src, tmp1), ctx);
			break;
		case BPF_JNE:
			emit(SW64_BPF_CMPEQ_REG(dst, src, tmp1), ctx);
			emit(SW64_BPF_XOR_IMM(tmp1, 1, tmp1), ctx);
			break;
		case BPF_JSGT:
			emit(SW64_BPF_CMPLT_REG(src, dst, tmp1), ctx);
			break;
		case BPF_JSLT:
			emit(SW64_BPF_CMPLT_REG(dst, src, tmp1), ctx);
			break;
		case BPF_JSGE:
			emit(SW64_BPF_CMPLE_REG(src, dst, tmp1), ctx);
			break;
		case BPF_JSLE:
			emit(SW64_BPF_CMPLE_REG(dst, src, tmp1), ctx);
			break;
		case BPF_JSET:
			emit(SW64_BPF_AND_REG(dst, src, tmp1), ctx);
			break;
		}
		jmp_offset = bpf2sw64_offset(bpf_idx, off, ctx);
		if (jmp_offset >= -0x100000 && jmp_offset <= 0xfffff) {
			emit(SW64_BPF_BNE(tmp1, jmp_offset), ctx);
		} else {
			pr_err("eBPF JIT %s[%d]: BPF_JMP out of range, %d instructions\n",
					current->comm, current->pid, jmp_offset);
			return -EINVAL;
		}
		break;

	case BPF_JMP32 | BPF_JEQ | BPF_K:
	case BPF_JMP32 | BPF_JGT | BPF_K:
	case BPF_JMP32 | BPF_JLT | BPF_K:
	case BPF_JMP32 | BPF_JGE | BPF_K:
	case BPF_JMP32 | BPF_JLE | BPF_K:
	case BPF_JMP32 | BPF_JNE | BPF_K:
	case BPF_JMP32 | BPF_JSGT | BPF_K:
	case BPF_JMP32 | BPF_JSLT | BPF_K:
	case BPF_JMP32 | BPF_JSGE | BPF_K:
	case BPF_JMP32 | BPF_JSLE | BPF_K:
	case BPF_JMP32 | BPF_JSET | BPF_K:
		emit(SW64_BPF_ADDW_REG(SW64_BPF_REG_ZR, dst, tmp2), ctx);
		dst = tmp2;
		fallthrough;
	case BPF_JMP | BPF_JEQ | BPF_K:
	case BPF_JMP | BPF_JGT | BPF_K:
	case BPF_JMP | BPF_JLT | BPF_K:
	case BPF_JMP | BPF_JGE | BPF_K:
	case BPF_JMP | BPF_JLE | BPF_K:
	case BPF_JMP | BPF_JNE | BPF_K:
	case BPF_JMP | BPF_JSGT | BPF_K:
	case BPF_JMP | BPF_JSLT | BPF_K:
	case BPF_JMP | BPF_JSGE | BPF_K:
	case BPF_JMP | BPF_JSLE | BPF_K:
	case BPF_JMP | BPF_JSET | BPF_K:
		emit_sw64_lds32(tmp1, imm, ctx);
		switch (BPF_OP(code)) {
		case BPF_JEQ:
			emit(SW64_BPF_CMPEQ_REG(dst, tmp1, tmp2), ctx);
			break;
		case BPF_JGT:
			emit(SW64_BPF_CMPULT_REG(tmp1, dst, tmp2), ctx);
			break;
		case BPF_JLT:
			emit(SW64_BPF_CMPULT_REG(dst, tmp1, tmp2), ctx);
			break;
		case BPF_JGE:
			emit(SW64_BPF_CMPULE_REG(tmp1, dst, tmp2), ctx);
			break;
		case BPF_JLE:
			emit(SW64_BPF_CMPULE_REG(dst, tmp1, tmp2), ctx);
			break;
		case BPF_JNE:
			emit(SW64_BPF_CMPEQ_REG(dst, tmp1, tmp2), ctx);
			emit(SW64_BPF_XOR_IMM(tmp2, 1, tmp2), ctx);
			break;
		case BPF_JSGT:
			emit(SW64_BPF_CMPLT_REG(tmp1, dst, tmp2), ctx);
			break;
		case BPF_JSLT:
			emit(SW64_BPF_CMPLT_REG(dst, tmp1, tmp2), ctx);
			break;
		case BPF_JSGE:
			emit(SW64_BPF_CMPLE_REG(tmp1, dst, tmp2), ctx);
			break;
		case BPF_JSLE:
			emit(SW64_BPF_CMPLE_REG(dst, tmp1, tmp2), ctx);
			break;
		case BPF_JSET:
			emit(SW64_BPF_AND_REG(dst, tmp1, tmp2), ctx);
			break;
		}
		jmp_offset = bpf2sw64_offset(bpf_idx, off, ctx);
		if (jmp_offset >= -0x100000 && jmp_offset <= 0xfffff) {
			emit(SW64_BPF_BNE(tmp2, jmp_offset), ctx);
		} else {
			pr_err("eBPF JIT %s[%d]: BPF_JMP out of range, %d instructions\n",
					current->comm, current->pid, jmp_offset);
			return -EINVAL;
		}
		break;

	case BPF_JMP | BPF_CALL:
	{
		bool fixed;
		u64 func;

		ret = bpf_jit_get_func_addr(ctx->prog, insn, extra_pass, &func, &fixed);
		if (ret < 0)
			return ret;

		emit_sw64_load_call_addr(SW64_BPF_REG_PV, func, ctx);
		emit(SW64_BPF_CALL(SW64_BPF_REG_RA, SW64_BPF_REG_PV), ctx);
		break;
	}

	case BPF_JMP | BPF_TAIL_CALL:
		if (emit_bpf_tail_call(ctx))
			return -EFAULT;
		break;

	case BPF_JMP | BPF_EXIT:
		// if this is the last bpf instruction, skip to epilogue
		if (bpf_idx == ctx->prog->len - 1)
			break;
		jmp_offset = offset_to_epilogue(ctx) - 1;
		// epilogue is always at the end, must jump forward
		if (jmp_offset >= -1 && jmp_offset <= 0xfffff) {
			if (ctx->image && !jmp_offset)
				// if this is the last jited instruction, generate nop
				emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, SW64_BPF_REG_ZR), ctx);
			else
				emit(SW64_BPF_BR(SW64_BPF_REG_ZR, jmp_offset), ctx);
		} else {
			pr_err("eBPF JIT %s[%d]: BPF_EXIT out of range, %d instructions\n",
					current->comm, current->pid, jmp_offset);
			return -EINVAL;
		}
		break;

	case BPF_LD | BPF_IMM | BPF_DW:
		insn1 = insn[1];
		imm64 = ((u64)insn1.imm << 32) | (u32)imm;
		if (bpf_pseudo_func(insn))
			emit_sw64_load_call_addr(SW64_BPF_REG_A1, imm64, ctx);
		else
			emit_sw64_ldu64(dst, imm64, ctx);
		put_tmp_reg(ctx);
		put_tmp_reg(ctx);
		return 1;

	/* LDX: dst = *(size *)(src + off) */
	case BPF_LDX | BPF_MEM | BPF_W:
	case BPF_LDX | BPF_MEM | BPF_H:
	case BPF_LDX | BPF_MEM | BPF_B:
	case BPF_LDX | BPF_MEM | BPF_DW:
	case BPF_LDX | BPF_PROBE_MEM | BPF_DW:
	case BPF_LDX | BPF_PROBE_MEM | BPF_W:
	case BPF_LDX | BPF_PROBE_MEM | BPF_H:
	case BPF_LDX | BPF_PROBE_MEM | BPF_B:
		switch (BPF_SIZE(code)) {
		case BPF_W:
			emit(SW64_BPF_LDW(dst, src, off), ctx);
			emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
			break;
		case BPF_H:
			emit(SW64_BPF_LDHU(dst, src, off), ctx);
			break;
		case BPF_B:
			emit(SW64_BPF_LDBU(dst, src, off), ctx);
			break;
		case BPF_DW:
			emit(SW64_BPF_LDL(dst, src, off), ctx);
			break;
		}

		ret = add_exception_handler(insn, ctx, dst);
		if (ret)
			return ret;
		break;

	/* speculation barrier */
	case BPF_ST | BPF_NOSPEC:
		/*
		 * Nothing required here.
		 */
		break;

	/* ST: *(size *)(dst + off) = imm */
	case BPF_ST | BPF_MEM | BPF_W:
	case BPF_ST | BPF_MEM | BPF_H:
	case BPF_ST | BPF_MEM | BPF_B:
	case BPF_ST | BPF_MEM | BPF_DW:
		/* Load imm to a register then store it */
		emit_sw64_lds32(tmp1, imm, ctx);
		switch (BPF_SIZE(code)) {
		case BPF_W:
			emit(SW64_BPF_STW(tmp1, dst, off), ctx);
			break;
		case BPF_H:
			emit(SW64_BPF_STH(tmp1, dst, off), ctx);
			break;
		case BPF_B:
			emit(SW64_BPF_STB(tmp1, dst, off), ctx);
			break;
		case BPF_DW:
			emit(SW64_BPF_STL(tmp1, dst, off), ctx);
			break;
		}
		break;

	/* STX: *(size *)(dst + off) = src */
	case BPF_STX | BPF_MEM | BPF_W:
		emit(SW64_BPF_STW(src, dst, off), ctx);
		break;
	case BPF_STX | BPF_MEM | BPF_H:
		emit(SW64_BPF_STH(src, dst, off), ctx);
		break;
	case BPF_STX | BPF_MEM | BPF_B:
		emit(SW64_BPF_STB(src, dst, off), ctx);
		break;
	case BPF_STX | BPF_MEM | BPF_DW:
		emit(SW64_BPF_STL(src, dst, off), ctx);
		break;

	/* STX XADD: lock *(u32 *)(dst + off) += src */
	case BPF_STX | BPF_XADD | BPF_W:
		emit_sw64_xadd32(src, dst, off, ctx);
		break;
	/* STX XADD: lock *(u64 *)(dst + off) += src */
	case BPF_STX | BPF_XADD | BPF_DW:
		emit_sw64_xadd64(src, dst, off, ctx);
		break;

	default:
		pr_err("eBPF JIT %s[%d]: unknown opcode 0x%02x\n",
				current->comm, current->pid, code);
		return -EINVAL;
	}

	put_tmp_reg(ctx);
	put_tmp_reg(ctx);
	return 0;
}

static int build_body(struct jit_ctx *ctx, bool extra_pass)
{
	const struct bpf_prog *prog = ctx->prog;
	int i;

	for (i = 0; i < prog->len; i++) {
		const struct bpf_insn *insn = &prog->insnsi[i];
		int ret;

		if (ctx->image == NULL)
			ctx->insn_offset[i] = ctx->idx;
		ret = build_insn(insn, ctx, extra_pass);
		if (ret < 0)
			return ret;
		while (ret > 0) {
			i++;
			if (ctx->image == NULL)
				ctx->insn_offset[i] = ctx->insn_offset[i - 1];
			ret--;
		}
	}

	return 0;
}

static int validate_code(struct jit_ctx *ctx)
{
	int i;

	for (i = 0; i < ctx->idx; i++) {
		if (ctx->image[i] == SW64_BPF_ILLEGAL_INSN)
			return -1;
	}

	if (WARN_ON_ONCE(ctx->exentry_idx != ctx->prog->aux->num_exentries))
		return -1;

	return 0;
}

static inline void bpf_flush_icache(void *start, void *end)
{
	flush_icache_range((unsigned long)start, (unsigned long)end);
}

static int __patch_insn_write(void *addr, const void *insn, size_t len)
{
	return copy_to_kernel_nofault(addr, insn, len);
}

int patch_insn_write(void *addr, const void *insn, size_t len)
{
	size_t size;
	int ret;

	while (len) {
		size = min(len, PAGE_SIZE - offset_in_page(addr));

		ret = __patch_insn_write(addr, insn, size);
		if (ret)
			return ret;

		addr += size;
		insn += size;
		len -= size;
	}

	return 0;
}

int sw64_insn_copy(void *addr, const void *insns, size_t len)
{
	int ret;

	ret = patch_insn_write(addr, insns, len);
	if (!ret) {
		flush_icache_range((unsigned long)addr, (unsigned long)addr + len);
		mb();
	}

	return ret;
}

static int gen_call_or_nops(void *target, void *ip, u32 *insns, bool is_call)
{
	int i;
	s64 offset;
	s32 jmp_offset;
	struct jit_ctx ctx = {
		.image = insns,
		.idx = 0,
	};

	if (!target) {
		for (i = 0; i < SW64_FENTRY_NINSNS; i++)
			emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR,
					SW64_BPF_REG_ZR), &ctx);
		return 0;
	}
	offset = (s64)((unsigned long)target - (unsigned long)ip);
	if (offset >= -0x100000 && offset <= 0xfffff) {
		jmp_offset = (s32)offset;
		/* we must remember br in sw is 4 * disp， and -1 is for pc will add 1 when exec */
		jmp_offset = jmp_offset/4 - 1;
		emit(SW64_BPF_BR(is_call ? SW64_BPF_REG_AT : SW64_BPF_REG_ZR, jmp_offset), &ctx);
	} else {
		pr_err("bpf-jit: target offset 0x%llx is out of range\n", offset);
		return -ERANGE;
	}
	return 0;
}

static void set_sw_nops(u32 *insns, int num)
{
	int i;
	struct jit_ctx ctx = {
		.image = insns,
		.idx = 0,
	};

	for (i = 0; i < num; i++)
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, SW64_BPF_REG_ZR), &ctx);

	return;

}

int bpf_arch_text_poke(void *ip, enum bpf_text_poke_type poke_type,
		       void *old_addr, void *new_addr)
{

	u32 old_insns[SW64_FENTRY_NINSNS], new_insns[SW64_FENTRY_NINSNS];
	bool is_call = poke_type == BPF_MOD_CALL;
	int ret;

	if (!is_kernel_text((unsigned long)ip) &&
	    !is_bpf_text_address((unsigned long)ip))
		return -EOPNOTSUPP;

	set_sw_nops(old_insns, SW64_FENTRY_NINSNS);
	set_sw_nops(new_insns, SW64_FENTRY_NINSNS);

	ret = gen_call_or_nops(old_addr, ip, old_insns, is_call);
	if (ret)
		return ret;
	/* if not same, old addr is wrong, maybe change illegal */
	if (memcmp(ip, old_insns, SW64_FENTRY_NINSNS * 4))
		return -EFAULT;

	ret = gen_call_or_nops(new_addr, ip, new_insns, is_call);
	if (ret)
		return ret;

	cpus_read_lock();
	mutex_lock(&text_mutex);
	if (memcmp(ip, new_insns, SW64_FENTRY_NINSNS * 4))
		ret = sw64_insn_copy(ip, new_insns, SW64_FENTRY_NINSNS * 4);
	mutex_unlock(&text_mutex);
	cpus_read_unlock();

	return ret;
}

static int btf_func_model_nregs(const struct btf_func_model *m)
{
	int nregs = m->nr_args;
	int i;

	/* extra registers needed for struct argument */
	for (i = 0; i < MAX_BPF_FUNC_ARGS; i++) {
		/* The arg_size is at most 16 bytes, enforced by the verifier. */
		if (m->arg_flags[i] & BTF_FMODEL_STRUCT_ARG)
			nregs += (m->arg_size[i] + 7) / 8 - 1;
	}

	return nregs;
}

static void emit_sw64_call(u64 target, struct jit_ctx *ctx)
{
	unsigned long ip = (unsigned long)(ctx->ro_image + ctx->idx);
	s64 offset = (s64)((unsigned long)target - (unsigned long)ip);

	if (offset >= -0x100000 && offset <= 0xfffff) {
		s32 jmp_offset = (s32)offset;
		/* we must remember br in sw is 4 * disp， and -1 is for pc will add 1 when exec */
		jmp_offset = jmp_offset/4 - 1;
		emit(SW64_BPF_BR(SW64_BPF_REG_RA, jmp_offset), ctx);
	} else {
		emit_sw64_load_call_addr(SW64_BPF_REG_PV, target, ctx);
		emit(SW64_BPF_CALL(SW64_BPF_REG_RA, SW64_BPF_REG_PV), ctx);
	}

}

static void save_args(struct jit_ctx *ctx, int args_off, int nregs)
{
	int i;

	for (i = 0; i < nregs; i++) {
		if (i < SW64_MAX_REG_ARGS) {
			emit(SW64_BPF_STL(i + SW64_BPF_REG_A0, SW64_BPF_REG_FP, -args_off), ctx);
		} else {
			emit(SW64_BPF_LDL(SW64_BPF_REG_T0,
					SW64_BPF_REG_FP, 16 + (i - SW64_MAX_REG_ARGS) * 8), ctx);
			emit(SW64_BPF_STL(SW64_BPF_REG_T0, SW64_BPF_REG_FP, -args_off), ctx);
		}
		args_off -= 8;
	}
}

static void restore_args(struct jit_ctx *ctx, int args_off, int nr_reg_args)
{
	int i;

	for (i = 0; i < nr_reg_args; i++) {
		emit(SW64_BPF_LDL(i + SW64_BPF_REG_A0, SW64_BPF_REG_FP, -args_off), ctx);
		args_off -= 8;
	}
}

static void restore_stack_args(int nr_stack_args, int args_off, int stk_arg_off,
			       struct jit_ctx *ctx)
{
	int i;

	for (i = 0; i < nr_stack_args; i++) {
		emit(SW64_BPF_LDL(SW64_BPF_REG_T0,
				SW64_BPF_REG_FP, -(args_off - SW64_MAX_REG_ARGS * 8)), ctx);
		emit(SW64_BPF_STL(SW64_BPF_REG_T0, SW64_BPF_REG_FP, -stk_arg_off), ctx);
		args_off -= 8;
		stk_arg_off -= 8;
	}
}

void *bpf_arch_text_copy(void *dst, void *src, size_t len)
{
	int ret;

	mutex_lock(&text_mutex);
	ret = sw64_insn_copy(dst, src, len);
	mutex_unlock(&text_mutex);

	if (ret)
		return ERR_PTR(-EINVAL);

	return dst;
}

int bpf_arch_text_invalidate(void *dst, size_t len)
{
	int ret;
	void *image = kzalloc(len, GFP_KERNEL);

	mutex_lock(&text_mutex);
	ret = sw64_insn_copy(dst, image, len);
	mutex_unlock(&text_mutex);

	kfree(image);
	return ret;
}

static void sw64_invoke_bpf_prog(struct jit_ctx *ctx, struct bpf_tramp_link *l,
	int args_off, int retval_off, int run_ctx_off, bool save_ret)
{
	u32 *branch;
	u64 enter_prog;
	u64 exit_prog;
	struct bpf_prog *p = l->link.prog;
	int cookie_off = offsetof(struct bpf_tramp_run_ctx, bpf_cookie);

	enter_prog = (u64)bpf_trampoline_enter(p);
	exit_prog = (u64)bpf_trampoline_exit(p);

	if (l->cookie == 0) {
		/* if cookie is zero, one instruction is enough to store it */
		emit(SW64_BPF_STL(SW64_BPF_REG_ZR,
				SW64_BPF_REG_FP, -run_ctx_off + cookie_off), ctx);
	} else {
		emit_sw64_ldu64(SW64_BPF_REG_T0, l->cookie, ctx);
		emit(SW64_BPF_STL(SW64_BPF_REG_T0,
				SW64_BPF_REG_FP, -run_ctx_off + cookie_off), ctx);
	}

	/* arg1: prog */
	emit_sw64_ldu64(SW64_BPF_REG_A0, (const u64)p, ctx);
	/* arg2: &run_ctx */
	emit(SW64_BPF_LDI(SW64_BPF_REG_A1, SW64_BPF_REG_FP, -run_ctx_off), ctx);
	emit_sw64_call(enter_prog, ctx);

	/* save return value to callee saved register S0 , V0 is return value for sw64 */
	emit(SW64_BPF_LDI(SW64_BPF_REG_S0, SW64_BPF_REG_V0, 0), ctx);

	/* if (__bpf_prog_enter(prog) == 0)
	 *         goto skip_exec_of_prog;
	 */
	branch = ctx->image + ctx->idx;
	/* nop reserved for conditional jump */
	emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, SW64_BPF_REG_ZR), ctx);

	/*  must use BPF_REG_1(SW64_BPF_REG_A0), this is defined in clang */
	emit(SW64_BPF_LDI(SW64_BPF_REG_A0, SW64_BPF_REG_FP, -args_off), ctx);
	if (!p->jited)
		emit_sw64_ldu64(SW64_BPF_REG_A1, (const u64)p->insnsi, ctx);
	emit_sw64_call((const u64)p->bpf_func, ctx);

	if (save_ret)
		emit(SW64_BPF_STL(SW64_BPF_REG_V0, SW64_BPF_REG_FP, -retval_off), ctx);

	if (ctx->image) {
		/* we must remember pc will add 1 when exec in sw */
		int offset = ctx->image + ctx->idx - branch - 1;
		*branch = SW64_BPF_BEQ(SW64_BPF_REG_V0, offset);
	}

	/* arg1: prog */
	emit_sw64_ldu64(SW64_BPF_REG_A0, (const u64)p, ctx);
	/* arg2: start time */
	emit(SW64_BPF_LDI(SW64_BPF_REG_A1, SW64_BPF_REG_S0, 0), ctx);
	/* arg3: &run_ctx */
	emit(SW64_BPF_LDI(SW64_BPF_REG_A2, SW64_BPF_REG_FP, -run_ctx_off), ctx);
	emit_sw64_call(exit_prog, ctx);
}

static void sw64_invoke_bpf_mod_ret(struct jit_ctx *ctx, struct bpf_tramp_links *tl,
	int args_off, int retval_off, int run_ctx_off, u32 **branches)
{
	int i;

	/*
	 * The first fmod_ret program will receive a garbage return value.
	 * Set this to 0 to avoid confusing the program.
	 */
	emit(SW64_BPF_STL(SW64_BPF_REG_ZR, SW64_BPF_REG_FP, -retval_off), ctx);
	for (i = 0; i < tl->nr_links; i++) {
		sw64_invoke_bpf_prog(ctx, tl->links[i], args_off, retval_off,
				run_ctx_off, true);
		/* if (*(u64 *)(sp + retval_off) !=  0)
		 *	goto do_fexit;
		 */
		emit(SW64_BPF_LDL(SW64_BPF_REG_T0, SW64_BPF_REG_FP, -retval_off), ctx);
		/*
		 * Save the location of branch, and generate a nop.
		 * This nop will be replaced with a BNE later.
		 */
		branches[i] = ctx->image + ctx->idx;
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, SW64_BPF_REG_ZR), ctx);
	}
}

static int __arch_prepare_bpf_trampoline(struct jit_ctx *ctx, struct bpf_tramp_image *im,
	struct bpf_tramp_links *tlinks, void *func_addr,
	int nregs, u32 flags)
{
	int i, offset;
	u32 **branches = NULL;
	int stack_size = 0;
	int retval_off, args_off, nregs_off, ip_off, run_ctx_off, sreg_off, stk_arg_off;
	struct bpf_tramp_links *fentry = &tlinks[BPF_TRAMP_FENTRY];
	struct bpf_tramp_links *fexit = &tlinks[BPF_TRAMP_FEXIT];
	struct bpf_tramp_links *fmod_ret = &tlinks[BPF_TRAMP_MODIFY_RETURN];
	bool save_ret;
	void *orig_call = func_addr;

	/* Two types of generated trampoline stack layout:
	 *
	 * 1. trampoline called from function entry
	 * --------------------------------------
	 * FP + 8	    [ RA to parent func	] return address to parent
	 *					  function
	 * FP + 0	    [ FP of parent func ] frame pointer of parent
	 *					  function
	 * FP - 8       [ R28 (BPF_AT) to traced func ] return address of traced
	 *					  function
	 * FP - 16	    [ FP of traced func ] frame pointer of traced
	 *					  function
	 * FP - 24	    [ GP of traced func ] global pointer of traced
	 *					  function
	 * --------------------------------------
	 *
	 * 2. trampoline called directly
	 * --------------------------------------
	 * FP - 8	    [ RA to caller func ] return address to caller
	 *					  function
	 * FP - 16	    [ FP of caller func	] frame pointer of caller
	 *					  function
	 * FP - 24	    [ GP of caller func	] global pointer of caller
	 *					  function
	 * --------------------------------------
	 *
	 * FP - retval_off  [ return value      ] BPF_TRAMP_F_CALL_ORIG or
	 *					  BPF_TRAMP_F_RET_FENTRY_RET
	 *                  [ argN              ]
	 *                  [ ...               ]
	 * FP - args_off    [ arg1              ]
	 *
	 * FP - nregs_off   [ regs count        ]
	 *
	 * FP - ip_off      [ traced func	] BPF_TRAMP_F_IP_ARG
	 *
	 * FP - run_ctx_off [ bpf_tramp_run_ctx ]
	 *
	 * FP - sreg_off    [ callee saved reg	]
	 *
	 *		    [ pads              ] pads for 16 bytes alignment
	 *
	 *		    [ stack_argN        ]
	 *		    [ ...               ]
	 * FP - stk_arg_off [ stack_arg1        ] BPF_TRAMP_F_CALL_ORIG
	 */

	if (flags & (BPF_TRAMP_F_ORIG_STACK | BPF_TRAMP_F_SHARE_IPMODIFY))
		return -EOPNOTSUPP;

	/* room of trampoline frame to store return address, frame pointer and GP */
	stack_size += 24;

	save_ret = flags & (BPF_TRAMP_F_CALL_ORIG | BPF_TRAMP_F_RET_FENTRY_RET);
	if (save_ret) {
		stack_size += 8; /* Save (BPF R0) or SW A0, in sw64, they are the same */
		retval_off = stack_size;
	}

	stack_size += nregs * 8;
	args_off = stack_size;

	stack_size += 8;
	nregs_off = stack_size;

	if (flags & BPF_TRAMP_F_IP_ARG) {
		stack_size += 8;
		ip_off = stack_size;
	}

	stack_size += round_up(sizeof(struct bpf_tramp_run_ctx), 8);
	run_ctx_off = stack_size;

	stack_size += 8;
	sreg_off = stack_size;

	if ((flags & BPF_TRAMP_F_CALL_ORIG) && (nregs - SW64_MAX_REG_ARGS > 0))
		stack_size += (nregs - SW64_MAX_REG_ARGS) * 8;

	stack_size = round_up(stack_size, STACK_ALIGN);

	/* room for args on stack must be at the top of stack */
	stk_arg_off = stack_size;

	if (func_addr) {
		/* For the trampoline called from function entry,
		 * the frame of traced function and the frame of
		 * trampoline need to be considered.
		 */
		emit(SW64_BPF_LDI(SW64_BPF_REG_SP, SW64_BPF_REG_SP, -16), ctx);
		emit(SW64_BPF_STL(SW64_BPF_REG_RA, SW64_BPF_REG_SP, 8), ctx);
		emit(SW64_BPF_STL(SW64_BPF_REG_FP, SW64_BPF_REG_SP, 0), ctx);
		emit(SW64_BPF_LDI(SW64_BPF_REG_FP, SW64_BPF_REG_SP, 16), ctx);

		emit(SW64_BPF_LDI(SW64_BPF_REG_SP, SW64_BPF_REG_SP, -stack_size), ctx);
		emit(SW64_BPF_STL(SW64_BPF_REG_AT, SW64_BPF_REG_SP, stack_size - 8), ctx);
		emit(SW64_BPF_STL(SW64_BPF_REG_FP, SW64_BPF_REG_SP, stack_size - 16), ctx);
		emit(SW64_BPF_STL(SW64_BPF_REG_GP, SW64_BPF_REG_SP, stack_size - 24), ctx);
		emit(SW64_BPF_LDI(SW64_BPF_REG_FP, SW64_BPF_REG_SP, stack_size), ctx);
	} else {
		/* For the trampoline called directly, just handle
		 * the frame of trampoline.
		 */
		emit(SW64_BPF_LDI(SW64_BPF_REG_SP, SW64_BPF_REG_SP, -stack_size), ctx);
		emit(SW64_BPF_STL(SW64_BPF_REG_RA, SW64_BPF_REG_SP, stack_size - 8), ctx);
		emit(SW64_BPF_STL(SW64_BPF_REG_FP, SW64_BPF_REG_SP, stack_size - 16), ctx);
		emit(SW64_BPF_STL(SW64_BPF_REG_GP, SW64_BPF_REG_SP, stack_size - 24), ctx);
		emit(SW64_BPF_LDI(SW64_BPF_REG_FP, SW64_BPF_REG_SP, stack_size), ctx);
	}

	/*
	 * callee saved register S0 to pass start time,
	 * we need to remember it in invoke_bpf_prog
	 */
	emit(SW64_BPF_STL(SW64_BPF_REG_S0, SW64_BPF_REG_FP, -sreg_off), ctx);

	/* store ip address of the traced function */
	if (flags & BPF_TRAMP_F_IP_ARG) {
		emit_sw64_ldu64(SW64_BPF_REG_T0, (const u64)func_addr, ctx);
		emit(SW64_BPF_STL(SW64_BPF_REG_T0, SW64_BPF_REG_FP, -ip_off), ctx);
	}

	emit(SW64_BPF_LDI(SW64_BPF_REG_T0, SW64_BPF_REG_ZR, nregs), ctx);
	emit(SW64_BPF_STL(SW64_BPF_REG_T0, SW64_BPF_REG_FP, -nregs_off), ctx);

	save_args(ctx, args_off, nregs);

	if (flags & BPF_TRAMP_F_SKIP_FRAME)
		orig_call += MCOUNT_INSN_SIZE;

	if (flags & BPF_TRAMP_F_CALL_ORIG) {
		emit_sw64_ldu64(SW64_BPF_REG_A0, (const u64)im, ctx);
		emit_sw64_call((const u64)__bpf_tramp_enter, ctx);
	}

	for (i = 0; i < fentry->nr_links; i++)
		sw64_invoke_bpf_prog(ctx, fentry->links[i], args_off, retval_off,
				run_ctx_off, flags & BPF_TRAMP_F_RET_FENTRY_RET);

	if (fmod_ret->nr_links) {
		branches = kcalloc(fmod_ret->nr_links, sizeof(u32 *), GFP_KERNEL);
		if (!branches)
			return -ENOMEM;

		sw64_invoke_bpf_mod_ret(ctx, fmod_ret, args_off, retval_off, run_ctx_off, branches);
	}

	if (flags & BPF_TRAMP_F_CALL_ORIG) {
		restore_args(ctx, args_off, min_t(int, nregs, SW64_MAX_REG_ARGS));
		restore_stack_args(nregs - SW64_MAX_REG_ARGS, args_off, stk_arg_off, ctx);
		/* call original func */
		emit_sw64_call((const u64)orig_call, ctx);
		/* store return value */
		emit(SW64_BPF_STL(SW64_BPF_REG_V0, SW64_BPF_REG_FP, -retval_off), ctx);
		/* reserve a nop for bpf_tramp_image_put */
		im->ip_after_call = ctx->image + ctx->idx;
		/* reserved 16 nop for long jmp, that is enough */
		for (i = 0; i < 16; i++)
			emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR,
					SW64_BPF_REG_ZR, SW64_BPF_REG_ZR), ctx);
	}

	for (i = 0; i < fmod_ret->nr_links && ctx->image != NULL; i++) {
		/* we must remember pc will add 1 when exec in sw */
		offset = ctx->image + ctx->idx - branches[i] - 1;
		*branches[i] = SW64_BPF_BNE(SW64_BPF_REG_T0, offset);
	}

	for (i = 0; i < fexit->nr_links; i++)
		sw64_invoke_bpf_prog(ctx, fexit->links[i], args_off,
				retval_off, run_ctx_off, false);

	if (flags & BPF_TRAMP_F_CALL_ORIG) {
		im->ip_epilogue = ctx->image + ctx->idx;
		/* for the first pass, assume the worst case */
		emit_sw64_ldu64(SW64_BPF_REG_A0, (const u64)im, ctx);
		emit_sw64_call((const u64)__bpf_tramp_exit, ctx);
	}

	if (flags & BPF_TRAMP_F_RESTORE_REGS)
		restore_args(ctx, args_off, min_t(int, nregs, SW64_MAX_REG_ARGS));

	if (save_ret)
		emit(SW64_BPF_LDL(SW64_BPF_REG_V0, SW64_BPF_REG_FP, -retval_off), ctx);

	/* callee saved register S0 to transmit start time, so use this reg, now we restore it  */
	emit(SW64_BPF_LDL(SW64_BPF_REG_S0, SW64_BPF_REG_FP, -sreg_off), ctx);

	if (func_addr) {
		/* trampoline called from function entry */
		emit(SW64_BPF_LDL(SW64_BPF_REG_AT, SW64_BPF_REG_SP, stack_size - 8), ctx);
		emit(SW64_BPF_LDL(SW64_BPF_REG_FP, SW64_BPF_REG_SP, stack_size - 16), ctx);
		emit(SW64_BPF_LDL(SW64_BPF_REG_GP, SW64_BPF_REG_SP, stack_size - 24), ctx);
		emit(SW64_BPF_LDI(SW64_BPF_REG_SP, SW64_BPF_REG_SP, stack_size), ctx);

		emit(SW64_BPF_LDL(SW64_BPF_REG_RA, SW64_BPF_REG_SP, 8), ctx);
		emit(SW64_BPF_LDL(SW64_BPF_REG_FP, SW64_BPF_REG_SP, 0), ctx);
		emit(SW64_BPF_LDI(SW64_BPF_REG_SP, SW64_BPF_REG_SP, 16), ctx);

		if (flags & BPF_TRAMP_F_SKIP_FRAME)
			/* return to parent function */
			emit(SW64_BPF_RET(SW64_BPF_REG_RA), ctx);
		else
			/* return to traced function */
			emit(SW64_BPF_RET(SW64_BPF_REG_AT), ctx);
	} else {
		/* trampoline called directly */
		emit(SW64_BPF_LDL(SW64_BPF_REG_RA, SW64_BPF_REG_SP, stack_size - 8), ctx);
		emit(SW64_BPF_LDL(SW64_BPF_REG_FP, SW64_BPF_REG_SP, stack_size - 16), ctx);
		emit(SW64_BPF_LDL(SW64_BPF_REG_GP, SW64_BPF_REG_SP, stack_size - 24), ctx);
		emit(SW64_BPF_LDI(SW64_BPF_REG_SP, SW64_BPF_REG_SP, stack_size), ctx);

		emit(SW64_BPF_RET(SW64_BPF_REG_RA), ctx);
	}

	kfree(branches);

	return ctx->idx;
}

int arch_prepare_bpf_trampoline(struct bpf_tramp_image *im, void *image, void *image_end,
	const struct btf_func_model *m, u32 flags,
	struct bpf_tramp_links *tlinks,
	void *func_addr)
{
	int ret;
	int nregs;
	struct jit_ctx ctx = {
		.image = NULL,
		.ro_image = image,
		.idx = 0,
	};

	nregs = btf_func_model_nregs(m);

	ret = __arch_prepare_bpf_trampoline(&ctx, im, tlinks, func_addr, nregs, flags);
	if (ret < 0)
		return ret;

	if (ret * SW64_INSN_SIZE > (long)image_end - (long)image)
		return -EFBIG;

	ctx.image = image;
	ctx.idx = 0;

	ret = __arch_prepare_bpf_trampoline(&ctx, im, tlinks, func_addr, nregs, flags);
	if (ret < 0)
		goto out;

out:
	return ret < 0 ? ret : ret * SW64_INSN_SIZE;
}

struct bpf_prog *bpf_int_jit_compile(struct bpf_prog *prog)
{
	struct bpf_prog *tmp, *orig_prog = prog;
	struct bpf_binary_header *header;
	struct sw64_jit_data *jit_data;
	bool was_classic = bpf_prog_was_classic(prog);
	bool tmp_blinded = false;
	bool extra_pass = false;
	struct jit_ctx ctx;
	int image_size, prog_size, extable_size;
	u8 *image_ptr;

	if (!prog->jit_requested)
		return orig_prog;

	tmp = bpf_jit_blind_constants(prog);
	/* If blinding was requested and we failed during blinding,
	 * we must fall back to the interpreter.
	 */
	if (IS_ERR(tmp))
		return orig_prog;
	if (tmp != prog) {
		tmp_blinded = true;
		prog = tmp;
	}

	jit_data = prog->aux->jit_data;
	if (!jit_data) {
		jit_data = kzalloc(sizeof(*jit_data), GFP_KERNEL);
		if (!jit_data) {
			prog = orig_prog;
			goto out;
		}
		prog->aux->jit_data = jit_data;
	}
	if (jit_data->ctx.insn_offset) {
		ctx = jit_data->ctx;
		image_ptr = jit_data->image;
		header = jit_data->header;
		extra_pass = true;
		prog_size = sizeof(u32) * ctx.idx;
		goto skip_init_ctx;
	}
	memset(&ctx, 0, sizeof(ctx));
	ctx.prog = prog;

	ctx.insn_offset = kcalloc(prog->len + 1, sizeof(int), GFP_KERNEL);
	if (ctx.insn_offset == NULL) {
		prog = orig_prog;
		goto out_off;
	}

	/* 1. Initial fake pass to compute ctx->idx. */

	/* Fake pass to fill in ctx->offset. */
	build_prologue(&ctx, was_classic);

	if (build_body(&ctx, extra_pass)) {
		prog = orig_prog;
		goto out_off;
	}

	ctx.insn_offset[prog->len] = ctx.epilogue_offset = ctx.idx;
	build_epilogue(&ctx);

	extable_size = prog->aux->num_exentries *
		sizeof(struct exception_table_entry);

	/* Now we know the actual image size. */
	/* And we need extra 8 bytes for lock instructions alignment */
	prog_size = sizeof(u32) * ctx.idx + 8;
	image_size = prog_size + extable_size;
	header = bpf_jit_binary_alloc(image_size, &image_ptr,
				      sizeof(u32), jit_fill_hole);
	if (header == NULL) {
		prog = orig_prog;
		goto out_off;
	}

	/* 2. Now, the actual pass. */

	/* lock instructions need 8-byte alignment */
	ctx.image = (u32 *)(((unsigned long)image_ptr + 7) & (~7));
	if (extable_size)
		prog->aux->extable = (void *)image_ptr + prog_size;
skip_init_ctx:
	ctx.idx = 0;
	ctx.exentry_idx = 0;

	build_prologue(&ctx, was_classic);

	if (build_body(&ctx, extra_pass)) {
		bpf_jit_binary_free(header);
		prog = orig_prog;
		goto out_off;
	}

	build_epilogue(&ctx);

	/* 3. Extra pass to validate JITed code. */
	if (validate_code(&ctx)) {
		bpf_jit_binary_free(header);
		prog = orig_prog;
		goto out_off;
	}

	/* And we're done. */
	if (bpf_jit_enable > 1)
		bpf_jit_dump(prog->len, prog_size, 2, ctx.image);

	bpf_flush_icache(header, ctx.image + ctx.idx);

	if (!prog->is_func || extra_pass) {
		bpf_jit_binary_lock_ro(header);
	} else {
		jit_data->ctx = ctx;
		jit_data->image = image_ptr;
		jit_data->header = header;
	}
	prog->bpf_func = (void *)ctx.image;
	prog->jited = 1;
	prog->jited_len = prog_size;
	if (ctx.current_tmp_reg) {
		pr_err("eBPF JIT %s[%d]: unreleased temporary regsters %d\n",
				current->comm, current->pid, ctx.current_tmp_reg);
	}

	if (!prog->is_func || extra_pass) {
out_off:
		kfree(ctx.insn_offset);
		kfree(jit_data);
		prog->aux->jit_data = NULL;
	}
out:
	if (tmp_blinded)
		bpf_jit_prog_release_other(prog, prog == orig_prog ?
					   tmp : orig_prog);
	return prog;
}

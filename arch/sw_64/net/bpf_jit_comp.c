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

#include <asm/cacheflush.h>

#include "bpf_jit.h"

#define TCALL_CNT (MAX_BPF_JIT_REG + 0)

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

static void emit_sw64_ldu32(const int dst, const u32 imm, struct jit_ctx *ctx)
{
	u16 imm_tmp;
	u8 reg_tmp = get_tmp_reg(ctx);

	if (!imm) {
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, dst), ctx);
		put_tmp_reg(ctx);
		return;
	}

	if (imm <= S16_MAX) {
		emit(SW64_BPF_LDI(dst, SW64_BPF_REG_ZR, imm), ctx);
		put_tmp_reg(ctx);
		return;
	}

	if (imm >= U32_MAX - S16_MAX) {
		emit(SW64_BPF_LDI(dst, SW64_BPF_REG_ZR, imm), ctx);
		emit(SW64_BPF_ZAP_IMM(dst, 0xf0, dst), ctx);
		put_tmp_reg(ctx);
		return;
	}

	imm_tmp = (imm >> 30) & 3;
	emit(SW64_BPF_LDI(dst, SW64_BPF_REG_ZR, imm_tmp), ctx);
	if (imm_tmp)
		emit(SW64_BPF_SLL_IMM(dst, 30, dst), ctx);

	imm_tmp = (imm >> 15) & 0x7fff;
	if (imm_tmp) {
		emit(SW64_BPF_LDI(reg_tmp, SW64_BPF_REG_ZR, imm_tmp), ctx);
		emit(SW64_BPF_SLL_IMM(reg_tmp, 15, reg_tmp), ctx);
		emit(SW64_BPF_ADDL_REG(dst, reg_tmp, dst), ctx);
	}

	imm_tmp = imm & 0x7fff;
	if (imm_tmp)
		emit(SW64_BPF_LDI(dst, dst, imm_tmp), ctx);

	put_tmp_reg(ctx);
}

static void emit_sw64_lds32(const int dst, const s32 imm, struct jit_ctx *ctx)
{
	s16 hi = imm >> 16;
	s16 lo = imm & 0xffff;
	u8 reg_tmp = get_tmp_reg(ctx);

	if (!imm) {
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, dst), ctx);
		put_tmp_reg(ctx);
		return;
	}

	if (imm >= S16_MIN && imm <= S16_MAX) {
		emit(SW64_BPF_LDI(dst, SW64_BPF_REG_ZR, imm), ctx);
		put_tmp_reg(ctx);
		return;
	}

	emit(SW64_BPF_LDIH(dst, SW64_BPF_REG_ZR, hi), ctx);
	if (lo & 0x8000) {	// sign bit is 1
		lo = lo & 0x7fff;
		emit(SW64_BPF_LDI(reg_tmp, SW64_BPF_REG_ZR, 1), ctx);
		emit(SW64_BPF_SLL_IMM(reg_tmp, 15, reg_tmp), ctx);
		emit(SW64_BPF_ADDL_REG(dst, reg_tmp, dst), ctx);
		if (lo)
			emit(SW64_BPF_LDI(dst, dst, lo), ctx);
	} else {	// sign bit is 0
		if (lo)
			emit(SW64_BPF_LDI(dst, dst, lo), ctx);
	}

	put_tmp_reg(ctx);
}

static void emit_sw64_ldu64(const int dst, const u64 imm, struct jit_ctx *ctx)
{
	u16 imm_tmp;
	u8 reg_tmp = get_tmp_reg(ctx);

	if (!imm) {
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, dst), ctx);
		put_tmp_reg(ctx);
		return;
	}

	if (imm <= U32_MAX) {
		put_tmp_reg(ctx);
		return emit_sw64_ldu32(dst, (u32)imm, ctx);
	}

	if (imm >= (U64_MAX - S16_MAX) || imm <= S16_MAX) {
		emit(SW64_BPF_LDI(dst, SW64_BPF_REG_ZR, imm), ctx);
		put_tmp_reg(ctx);
		return;
	}

	imm_tmp = (imm >> 60) & 0xf;
	emit(SW64_BPF_LDI(dst, SW64_BPF_REG_ZR, imm_tmp), ctx);
	if (imm_tmp)
		emit(SW64_BPF_SLL_IMM(dst, 60, dst), ctx);

	imm_tmp = (imm >> 45) & 0x7fff;
	if (imm_tmp) {
		emit(SW64_BPF_LDI(reg_tmp, SW64_BPF_REG_ZR, imm_tmp), ctx);
		emit(SW64_BPF_SLL_IMM(reg_tmp, 45, reg_tmp), ctx);
		emit(SW64_BPF_ADDL_REG(dst, reg_tmp, dst), ctx);
	}

	imm_tmp = (imm >> 30) & 0x7fff;
	if (imm_tmp) {
		emit(SW64_BPF_LDI(reg_tmp, SW64_BPF_REG_ZR, imm_tmp), ctx);
		emit(SW64_BPF_SLL_IMM(reg_tmp, 30, reg_tmp), ctx);
		emit(SW64_BPF_ADDL_REG(dst, reg_tmp, dst), ctx);
	}

	imm_tmp = (imm >> 15) & 0x7fff;
	if (imm_tmp) {
		emit(SW64_BPF_LDI(reg_tmp, SW64_BPF_REG_ZR, imm_tmp), ctx);
		emit(SW64_BPF_SLL_IMM(reg_tmp, 15, reg_tmp), ctx);
		emit(SW64_BPF_ADDL_REG(dst, reg_tmp, dst), ctx);
	}

	imm_tmp = imm & 0x7fff;
	if (imm_tmp)
		emit(SW64_BPF_LDI(dst, dst, imm_tmp), ctx);

	put_tmp_reg(ctx);
}

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

/* STX XADD: lock *(u32 *)(dst + off) += src */
static void emit_sw64_xadd32(const int src, int dst, s16 off, struct jit_ctx *ctx)
{
	int atomic_start;
	int atomic_end;
	u8 tmp1 = get_tmp_reg(ctx);
	u8 tmp2 = get_tmp_reg(ctx);
	u8 tmp3 = get_tmp_reg(ctx);

	if (off < -0x800 || off > 0x7ff) {
		emit(SW64_BPF_LDI(tmp1, dst, off), ctx);
		dst = tmp1;
		off = 0;
	}

	atomic_start = ctx->idx;
	emit(SW64_BPF_LLDW(tmp2, dst, off), ctx);
	emit(SW64_BPF_LDI(tmp3, SW64_BPF_REG_ZR, 1), ctx);
	emit(SW64_BPF_WR_F(tmp3), ctx);
	emit(SW64_BPF_ADDW_REG(tmp2, src, tmp2), ctx);
	if (ctx->idx & 1)
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, SW64_BPF_REG_ZR), ctx);
	emit(SW64_BPF_LSTW(tmp2, dst, off), ctx);
	emit(SW64_BPF_RD_F(tmp3), ctx);
	atomic_end = ctx->idx;
	emit(SW64_BPF_BEQ(tmp3, atomic_start - atomic_end - 1), ctx);

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
	u8 tmp3 = get_tmp_reg(ctx);

	if (off < -0x800 || off > 0x7ff) {
		emit(SW64_BPF_LDI(tmp1, dst, off), ctx);
		dst = tmp1;
		off = 0;
	}

	atomic_start = ctx->idx;
	emit(SW64_BPF_LLDL(tmp2, dst, off), ctx);
	emit(SW64_BPF_LDI(tmp3, SW64_BPF_REG_ZR, 1), ctx);
	emit(SW64_BPF_WR_F(tmp3), ctx);
	emit(SW64_BPF_ADDL_REG(tmp2, src, tmp2), ctx);
	if (ctx->idx & 1)
		emit(SW64_BPF_BIS_REG(SW64_BPF_REG_ZR, SW64_BPF_REG_ZR, SW64_BPF_REG_ZR), ctx);
	emit(SW64_BPF_LSTL(tmp2, dst, off), ctx);
	emit(SW64_BPF_RD_F(tmp3), ctx);
	atomic_end = ctx->idx;
	emit(SW64_BPF_BEQ(tmp3, atomic_start - atomic_end - 1), ctx);

	put_tmp_reg(ctx);
	put_tmp_reg(ctx);
	put_tmp_reg(ctx);
}

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
#define PROLOGUE_OFFSET	11

static void build_prologue(struct jit_ctx *ctx, bool was_classic)
{
	const u8 r6 = bpf2sw64[BPF_REG_6];
	const u8 r7 = bpf2sw64[BPF_REG_7];
	const u8 r8 = bpf2sw64[BPF_REG_8];
	const u8 r9 = bpf2sw64[BPF_REG_9];
	const u8 fp = bpf2sw64[BPF_REG_FP];
	const u8 tcc = bpf2sw64[TCALL_CNT];

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

	offset = (long)&ex->insn - pc;
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
static int build_insn(const struct bpf_insn *insn, struct jit_ctx *ctx)
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
	u64 func;
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
		emit_sw64_divmod(dst, src, ctx, code);
		break;
	case BPF_ALU64 | BPF_DIV | BPF_X:
		emit_sw64_divmod(dst, src, ctx, code);
		break;
	case BPF_ALU | BPF_MOD | BPF_X:
		emit_sw64_divmod(dst, src, ctx, code);
		break;
	case BPF_ALU64 | BPF_MOD | BPF_X:
		emit_sw64_divmod(dst, src, ctx, code);
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
			emit_sw64_htobe16(dst, ctx);
			break;
		case 32:
			emit_sw64_htobe32(dst, ctx);
			break;
		case 64:
			emit_sw64_htobe64(dst, ctx);
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
		emit_sw64_divmod(dst, tmp1, ctx, code);
		break;
	case BPF_ALU64 | BPF_DIV | BPF_K:
		emit_sw64_lds32(tmp1, imm, ctx);
		emit_sw64_divmod(dst, tmp1, ctx, code);
		break;
	case BPF_ALU | BPF_MOD | BPF_K:
		emit_sw64_ldu32(tmp1, imm, ctx);
		emit_sw64_divmod(dst, tmp1, ctx, code);
		break;
	case BPF_ALU64 | BPF_MOD | BPF_K:
		emit_sw64_lds32(tmp1, imm, ctx);
		emit_sw64_divmod(dst, tmp1, ctx, code);
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
		func = (u64)__bpf_call_base + imm;
		if ((func & ~(KERNEL_IMAGE_SIZE - 1)) != __START_KERNEL_map)
			/* calling bpf program, switch to vmalloc addr */
			func = (func & U32_MAX) | VMALLOC_START;
		emit_sw64_ldu64(SW64_BPF_REG_PV, func, ctx);
		emit(SW64_BPF_CALL(SW64_BPF_REG_RA, SW64_BPF_REG_PV), ctx);
		break;

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

static int build_body(struct jit_ctx *ctx)
{
	const struct bpf_prog *prog = ctx->prog;
	int i;

	for (i = 0; i < prog->len; i++) {
		const struct bpf_insn *insn = &prog->insnsi[i];
		int ret;

		if (ctx->image == NULL)
			ctx->insn_offset[i] = ctx->idx;
		ret = build_insn(insn, ctx);
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

	if (build_body(&ctx)) {
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

	if (build_body(&ctx)) {
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

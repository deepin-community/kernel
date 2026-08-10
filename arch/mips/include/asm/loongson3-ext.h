/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Encodings for the Loongson-3 (GS464) EXT 128-bit quad load/store
 * instructions, gslq/gssq.
 *
 * Some assemblers (e.g. Clang's integrated assembler) support neither
 * the gslq/gssq mnemonics nor '.set arch=loongson3a'.  Emitting the
 * instruction words directly yields the exact same machine code GNU as
 * produces for the mnemonics, and matches what the kernel generates at
 * runtime via uasm (arch/mips/mm/page.c, arch/mips/mm/uasm-mips.c).
 *
 * Instruction format ("Loongson-3 overridden lwc2/swc2 Load/Store
 * format", see struct loongson3_lswc2_format in
 * arch/mips/include/uapi/asm/inst.h):
 *
 *   [31:26] opcode: lwc2_op (0x32) = gslq / swc2_op (0x3a) = gssq
 *   [25:21] base register
 *   [20:16] rt: register holding the lower 8 bytes
 *   [15]    fr = 0 (integer register pair)
 *   [14:6]  offset: 9-bit signed offset in 16-byte units, i.e. the
 *           byte offset >> 4 (as build_rc() in arch/mips/mm/uasm-mips.c
 *           does for uasm, and GNU as does for the mnemonics)
 *   [5]     ls = 1
 *   [4:0]   rq: register holding the upper 8 bytes
 *
 * LQS_GSLQ_ENC()/LQS_GSSQ_ENC() take register *numbers*.  To convert a
 * symbolic register name to its number, paste it with __LQS_REGNO_ in
 * the outermost preprocessor macro that receives the name, i.e.
 * __LQS_REGNO_##name; the paste must happen before the name is
 * macro-expanded (e.g. before a0 becomes $4 via asm/regdef.h).
 * t0-t7 below use the o32 numbering ($8-$15), matching the register
 * convention of the Loongson-3 memcpy/memset code.
 */
#ifndef __ASM_LOONGSON3_EXT_H
#define __ASM_LOONGSON3_EXT_H

#define __LQS_REGNO_zero	0
#define __LQS_REGNO_AT		1
#define __LQS_REGNO_v0		2
#define __LQS_REGNO_v1		3
#define __LQS_REGNO_a0		4
#define __LQS_REGNO_a1		5
#define __LQS_REGNO_a2		6
#define __LQS_REGNO_a3		7
#define __LQS_REGNO_t0		8
#define __LQS_REGNO_t1		9
#define __LQS_REGNO_t2		10
#define __LQS_REGNO_t3		11
#define __LQS_REGNO_t4		12
#define __LQS_REGNO_t5		13
#define __LQS_REGNO_t6		14
#define __LQS_REGNO_t7		15
#define __LQS_REGNO_s0		16
#define __LQS_REGNO_s1		17
#define __LQS_REGNO_s2		18
#define __LQS_REGNO_s3		19
#define __LQS_REGNO_s4		20
#define __LQS_REGNO_s5		21
#define __LQS_REGNO_s6		22
#define __LQS_REGNO_s7		23
#define __LQS_REGNO_t8		24
#define __LQS_REGNO_t9		25
#define __LQS_REGNO_k0		26
#define __LQS_REGNO_k1		27
#define __LQS_REGNO_gp		28
#define __LQS_REGNO_sp		29
#define __LQS_REGNO_fp		30
#define __LQS_REGNO_s8		30
#define __LQS_REGNO_ra		31

/*
 * The arguments follow the operand order of the GNU as mnemonics:
 *
 *   gslq q, v, offset(base):  q = [base + offset + 8], v = [base + offset]
 *   gssq q, v, offset(base):  [base + offset + 8] = q, [base + offset] = v
 *
 * i.e. the first mnemonic operand lands in the rq field [4:0] and the
 * second in the rt field [20:16] (cf. struct loongson3_lswc2_format in
 * arch/mips/include/uapi/asm/inst.h, and build_rz()/build_rc() in
 * arch/mips/mm/uasm-mips.c).
 *
 * q/v/b are register numbers; off is the byte offset.
 */
#define LQS_GSLQ_ENC(q, v, off, b)					\
	((0x32 << 26) | ((b) << 21) | ((v) << 16) |			\
	 ((((off) >> 4) & 0x1ff) << 6) | 0x20 | (q))

#define LQS_GSSQ_ENC(q, v, off, b)					\
	((0x3a << 26) | ((b) << 21) | ((v) << 16) |			\
	 ((((off) >> 4) & 0x1ff) << 6) | 0x20 | (q))

#endif /* __ASM_LOONGSON3_EXT_H */

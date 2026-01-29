/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HYGON SM3 Cipher Algorithm, using cis instructions.
 *
 * Copyright (C) 2026 Hygon Information Technology Co., Ltd.
 *
 */

#include <linux/linkage.h>
#include <asm/frame.h>
#include <linux/cfi_types.h>

#define rRIP              (%rip)

#define W_A_T0            %xmm0
#define W_A_T1            %xmm1
#define W_A_T2            %xmm2
#define W_A_T3            %xmm3
#define W_E_T0            %xmm4
#define W_E_T1            %xmm5
#define D_B_T             %xmm6
#define D_A_T             %xmm7
#define W_A_T4            %xmm8
#define D_B               %xmm9
#define D_A               %xmm10
#define D_S_T             %xmm11

#define digest_transpose_pre()                \
	vpshufd $0xd8, D_B, D_B_T;            \
	vpshufd $0xd8, D_A, D_A_T;            \
	vpunpcklqdq D_A_T, D_B_T, D_A;        \
	vpunpckhqdq D_A_T, D_B_T, D_B

#define digest_transpose_post()               \
	vpunpcklqdq D_B, D_A, D_B_T;          \
	vpunpckhqdq D_B, D_A, D_A_T;          \
	vpshufd $0xd8, D_A_T, D_A;            \
	vpshufd $0xd8, D_B_T, D_B

#define MSG_EXT_W_L()                         \
	vpxor      W_A_T1, W_A_T0, W_E_T0;    \
	vpunpckldq W_E_T0, W_A_T0, W_E_T1;    \
	vpunpckhdq W_E_T0, W_A_T0, W_E_T0;

#define MSG_EXT_W()                           \
	/* vsm3msga xmm0 xmm0 xmm1 xmm2 */    \
	.byte 0xc4,0xe3,0xf9,0x89,0xc1,0x20;  \
	/* vsm3msgb xmm0, xmm0,xmm3 */        \
	.byte 0xc4,0x62,0xf9,0xf9,0xc3;       \
	vmovdqa W_A_T1, W_A_T0;               \
	vmovdqa W_A_T2, W_A_T1;               \
	vmovdqa W_A_T3, W_A_T2;               \
	vmovdqa W_A_T4, W_A_T3;

#define MSG_EXT_W_56()                        \
	vmovdqa W_A_T1, W_A_T0;               \
	vmovdqa W_A_T2, W_A_T1;               \
	vmovdqa W_A_T3, W_A_T2;

#define MSG_EXT_W_60()                        \
	vmovdqa W_A_T1, W_A_T0;               \
	vmovdqa W_A_T2, W_A_T1;

#define MSG_HASH_ROUND_INST_FOUR(IDX0, IDX1, IDX2, IDX3)    \
	/* vsm3rnda xmm6 xmm6 xmm7 IDX0*/                   \
	.byte 0xc4,0xe3,0xc9,0x88,0xf7,IDX0;                \
	/* vsm3rndb xmm6 xmm6 xmm5 */                       \
	.byte 0xc4,0xe2,0xc9,0xf8,0xf5;                     \
	vpshufd $0x0e, W_E_T1, W_E_T1;                      \
	/* vsm3rnda xmm7 xmm7 xmm6 IDX1*/                   \
	.byte 0xc4,0xe3,0xc1,0x88,0xfe,IDX1;                \
	/* vsm3rndb xmm7 xmm7 xmm5 */                       \
	.byte 0xc4,0xe2,0xc1,0xf8,0xfd;                     \
	/* vsm3rnda xmm6 xmm6 xmm7 IDX2*/                   \
	.byte 0xc4,0xe3,0xc9,0x88,0xf7,IDX2;                \
	/* vsm3rndb xmm6 xmm6 xmm4 */                       \
	.byte 0xc4,0xe2,0xc9,0xf8,0xf4;                     \
	vpshufd $0x0e, W_E_T0, W_E_T0;                      \
	/* vsm3rnda xmm7 xmm7 xmm6 IDX3*/                   \
	.byte 0xc4,0xe3,0xc1,0x88,0xfe,IDX3;                \
	/* vsm3rndb xmm7 xmm7 xmm4 */                       \
	.byte 0xc4,0xe2,0xc1,0xf8,0xfc;

#define MSG_HASH_ROUND_FOUR_0(IDX0, IDX1, IDX2, IDX3)       \
	MSG_EXT_W_L()                                       \
	MSG_HASH_ROUND_INST_FOUR(IDX0, IDX1, IDX2, IDX3)

#define MSG_HASH_ROUND_FOUR(IDX0, IDX1, IDX2, IDX3)         \
	MSG_EXT_W()                                         \
	MSG_EXT_W_L()                                       \
	MSG_HASH_ROUND_INST_FOUR(IDX0, IDX1, IDX2, IDX3)

#define MSG_HASH_ROUND_FOUR_56(IDX0, IDX1, IDX2, IDX3)      \
	MSG_EXT_W_56()                                      \
	MSG_EXT_W_L()                                       \
	MSG_HASH_ROUND_INST_FOUR(IDX0, IDX1, IDX2, IDX3)

#define MSG_HASH_ROUND_FOUR_60(IDX0, IDX1, IDX2, IDX3)      \
	MSG_EXT_W_60()                                      \
	MSG_EXT_W_L()                                       \
	MSG_HASH_ROUND_INST_FOUR(IDX0, IDX1, IDX2, IDX3)

#define MSG_HASH_ROUND_LOOP()                               \
	MSG_HASH_ROUND_FOUR_0(0, 1, 2, 3)                   \
	MSG_HASH_ROUND_FOUR(4, 5, 6, 7)                     \
	MSG_HASH_ROUND_FOUR(8, 9, 10, 11)                   \
	MSG_HASH_ROUND_FOUR(12, 13, 14, 15)                 \
	MSG_HASH_ROUND_FOUR(16, 17, 18, 19)                 \
	MSG_HASH_ROUND_FOUR(20, 21, 22, 23)                 \
	MSG_HASH_ROUND_FOUR(24, 25, 26, 27)                 \
	MSG_HASH_ROUND_FOUR(28, 29, 30, 31)                 \
	MSG_HASH_ROUND_FOUR(32, 33, 34, 35)                 \
	MSG_HASH_ROUND_FOUR(36, 37, 38, 39)                 \
	MSG_HASH_ROUND_FOUR(40, 41, 42, 43)                 \
	MSG_HASH_ROUND_FOUR(44, 45, 46, 47)                 \
	MSG_HASH_ROUND_FOUR(48, 49, 50, 51)                 \
	MSG_HASH_ROUND_FOUR(52, 53, 54, 55)                 \
	MSG_HASH_ROUND_FOUR_56(56, 57, 58, 59)              \
	MSG_HASH_ROUND_FOUR_60(60, 61, 62, 63)


.section .data
.align 16

.Lbswap32_mask:
	.byte 3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12

.section .text
.align 16

/**
 * void cis_sm3_transform(u32 *digest,
                        const u8 *data,
                        int nblocks);
 */
SYM_TYPED_FUNC_START(cis_sm3_transform)
	/* input:
	 *     %rdi: digest context, digest
	 *     %rsi: input data, data
	 *     %rdx: block count, nblocks
	 */
    FRAME_BEGIN

	test %rdx, %rdx
	jle Over
	vmovdqu (%rdi),   D_B
	vmovdqu 16(%rdi), D_A
	digest_transpose_pre()

Loops:
	vmovdqa D_B, D_B_T
	vmovdqa D_A, D_A_T

	vmovdqu .Lbswap32_mask rRIP, D_S_T
	vmovdqu 0*16(%rsi), W_A_T0
	vmovdqu 1*16(%rsi), W_A_T1
	vmovdqu 2*16(%rsi), W_A_T2
	vmovdqu 3*16(%rsi), W_A_T3

	vpshufb D_S_T, W_A_T0, W_A_T0
	vpshufb D_S_T, W_A_T1, W_A_T1
	vpshufb D_S_T, W_A_T2, W_A_T2
	vpshufb D_S_T, W_A_T3, W_A_T3

	MSG_HASH_ROUND_LOOP()

	vpxor D_B, D_B_T, D_B
	vpxor D_A, D_A_T, D_A

	addq $64, %rsi
	dec %rdx
	jnz Loops

	digest_transpose_post()
	vmovdqu D_B, (%rdi)
	vmovdqu D_A, 16(%rdi)

Over:
    FRAME_END
	RET
SYM_FUNC_END(cis_sm3_transform)

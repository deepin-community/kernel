/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HYGON SM4 Cipher Algorithm, using cis instructions.
 *
 * Copyright (C) 2026 Hygon Information Technology Co., Ltd.
 *
 */

#include <linux/linkage.h>
#include <asm/frame.h>
#include <linux/cfi_types.h>

#define rRIP         (%rip)

#define RX0          %xmm0
#define RX1          %xmm1
#define RX2          %xmm2
#define RX3          %xmm3
#define RX4          %xmm4
#define RX5          %xmm5
#define RX6          %xmm6
#define RX7          %xmm7

#define XMM0         1111b    /* xmm0 */
#define XMM1         1110b    /* xmm1 */
#define XMM2         1101b    /* xmm2 */

/**
 * SM4RK   xmm0,  xmm2,  0x0
 * SM4RK   xmm0,  xmm2,  0x1
 * SM4RK   xmm0,  xmm2,  0x2
 * SM4RK   xmm0,  xmm2,  0x3
 */
#define SM4_ROUND_INST_SM4RK(XMM0, XMM2)        \
	.byte 0x66,0x0f,0x3a,0x8b,0xc2,0x00;    \
	.byte 0x66,0x0f,0x3a,0x8b,0xc2,0x01;    \
	.byte 0x66,0x0f,0x3a,0x8b,0xc2,0x02;    \
	.byte 0x66,0x0f,0x3a,0x8b,0xc2,0x03

/**
 * SM4RND xmm0,  xmm1,  0x0
 * SM4RND xmm0,  xmm1,  0x1
 * SM4RND xmm0,  xmm1,  0x2
 * SM4RND xmm0,  xmm1,  0x3
 */
#define SM4_ROUND_INST_SM4RND(XMM0, XMM1)       \
	.byte 0x66,0x0f,0x3a,0x8a,0xc1,0x00;    \
	.byte 0x66,0x0f,0x3a,0x8a,0xc1,0x01;    \
	.byte 0x66,0x0f,0x3a,0x8a,0xc1,0x02;    \
	.byte 0x66,0x0f,0x3a,0x8a,0xc1,0x03

/**
 * Multiply in GF(2^128) for XTS IVs (tweak)
 */
#define _gf128_primitive_mul(tw, xm0, xm1)      \
	movdqa .Gf128mul_shuf_data rRIP, xm1;   \
	vpshufb xm1, tw, xm0;                   \
	movdqa .Gf128mul_comp_data rRIP, xm1;   \
	vpsrlq $1, tw, tw;                      \
	pand xm1, xm0;                          \
	pcmpeqd xm1, xm0;                       \
	movdqa .Gf128mul_mask rRIP, xm1;        \
	pand xm1, xm0;                          \
	pxor xm0, tw

.section .data
.align 16

/**
 * SM4 key schedule constants (FK and CK)
 * Standard: GB/T 32907-2016
 * FK: 4x32-bit system parameters
 * CK: 32x32-bit round constants
 */
.L_sm4_fk:
	.long 0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc

.L_sm4_ck:
	.long 0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269
	.long 0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9
	.long 0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249
	.long 0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9
	.long 0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229
	.long 0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299
	.long 0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209
	.long 0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279

/* For byteswap */
.Lbswap128_mask:
	.byte 0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08
	.byte 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00

/* For input word byte-swap */
.Lbswap32_mask:
	.byte 0x03, 0x02, 0x01, 0x00, 0x07, 0x06, 0x05, 0x04
	.byte 0x0b, 0x0a, 0x09, 0x08, 0x0f, 0x0e, 0x0d, 0x0c

/* Process the carry of tweak */
.Gf128mul_mask:
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe1

/* Retrieve the middle and highest bytes of tweak */
.Gf128mul_shuf_data:
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

/* Check if there is any carry in tweak */
.Gf128mul_comp_data:
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
	.byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01

.text

/**
 * void gf128_mul_by_2(unsigned char *twk)
 */
SYM_TYPED_FUNC_START(gf128_mul_by_2)
	/* input:
	 *      %rdi: tweak value, twk
	 */
	FRAME_BEGIN

	movdqa .Lbswap128_mask rRIP, RX3;
	movdqu (%rdi), RX2;
	pshufb RX3, RX2;
	_gf128_primitive_mul(RX2, RX5, RX7);
	pshufb RX3, RX2;
	movdqu RX2, (%rdi);

	FRAME_END
	RET
SYM_FUNC_END(gf128_mul_by_2)

/**
 * void cis_sm4_set_key(unsigned char *key,
 *                           unsigned int *rk)
 */
SYM_TYPED_FUNC_START(cis_sm4_set_key)
	/* input:
	 *     %rdi: key data, key
	 *     %rsi: round key array, rk
	 */
	FRAME_BEGIN

	movdqu .Lbswap32_mask rRIP, RX3;
	movdqu (%rdi), RX0;
	movdqu .L_sm4_fk rRIP, RX1;
	pshufb RX3, RX0;
	pxor RX1, RX0;
	movq $0x08, %r8;
	lea .L_sm4_ck rRIP, %rdx

key_loop:
	movdqu (%rdx), RX2;
	SM4_ROUND_INST_SM4RK(XMM0, XMM2);
	movdqu RX0, (%rsi);
	add $0x10, %rsi;
	add $0x10, %rdx;
	dec %r8;
	jne key_loop;

	FRAME_END
	RET
SYM_FUNC_END(cis_sm4_set_key)

/**
 * void cis_sm4_blk(unsigned int *sk,
 *                         unsigned char *in,
 *                         unsigned char *out)
 */
SYM_TYPED_FUNC_START(cis_sm4_blk)
	/* input:
	 *      %rdi: round key array, sk
	 *      %rsi: input data, in
	 *      %rdx: output data, out
	 */
	FRAME_BEGIN

	leaq (16*8)(%rdi), %rax;
	movdqa .Lbswap32_mask rRIP, RX3;
	movdqu (%rsi), RX0;
	pshufb RX3, RX0;

asm_loop:
	movdqu (%rdi), RX1;
	SM4_ROUND_INST_SM4RND(XMM0, XMM1);
	add $0x10, %rdi;
	cmpq %rax, %rdi;
	jne asm_loop;

	pshufd $0x1b, RX0, RX0;
	pshufb RX3, RX0;
	movdqu RX0, (%rdx);

	FRAME_END
	RET
SYM_FUNC_END(cis_sm4_blk)

/**
 * void cis_sm4_ecb_crypt_blk8(unsigned int *sk,
 *                               unsigned char *in,
 *                               unsigned char *out,
 *                               unsigned int len)
 */
SYM_TYPED_FUNC_START(cis_sm4_ecb_crypt_blk8)
	/* input:
	 *      %rdi: round key array, sk
	 *      %rsi: input data, in
	 *      %rdx: output data, out
	 *      %rcx: data length, len
	 */
	FRAME_BEGIN

	test %rcx, %rcx;
	jle ecb_over;
	movq %rdi, %r9;
	movdqa .Lbswap32_mask rRIP, RX3;

ecb_loop:
	movq %r9, %rdi;
	leaq (16*8)(%rdi), %rax;
	movdqu (%rsi), RX0;
	pshufb RX3, RX0;

ecb_rnd_loop:
	movdqu (%rdi), RX1;
	SM4_ROUND_INST_SM4RND(XMM0, XMM1);
	add $0x10, %rdi;
	cmpq %rax, %rdi;
	jne ecb_rnd_loop;

	pshufd $0x1b, RX0, RX0;
	pshufb RX3, RX0;
	movdqu RX0, (%rdx);

	add $0x10, %rsi;
	add $0x10, %rdx;
	sub $0x10, %rcx;
	jnz ecb_loop;

ecb_over:
	FRAME_END
	RET
SYM_FUNC_END(cis_sm4_ecb_crypt_blk8)

/**
 * void cis_sm4_cbc_encrypt_blk8(unsigned int *sk,
 *                                 unsigned char *in,
 *                                 unsigned char *out,
 *                                 unsigned char *iv,
 *                                 unsigned int len)
 */
SYM_TYPED_FUNC_START(cis_sm4_cbc_encrypt_blk8)
	/* input:
	 *      %rdi: round key array, sk
	 *      %rsi: input data, in
	 *      %rdx: output data, out
	 *      %rcx: iv data, iv
	 *      %r8: data length, len
	 */
	FRAME_BEGIN

	test %r8, %r8
	jle cbc_enc_over
	movdqa .Lbswap32_mask rRIP, RX3
	movq %rdi, %r9
	movdqu (%rcx), RX0

cbc_enc_loop:
	movq %r9, %rdi
	leaq (16*8)(%rdi), %rax
	/* pxor must be 16-byte aligned */
	movdqu (%rsi), RX2
	pxor RX2, RX0
	pshufb RX3, RX0

cbc_enc_rnd_loop:
	movdqu (%rdi), RX1
	SM4_ROUND_INST_SM4RND(XMM0, XMM1)
	add $0x10, %rdi
	cmpq %rax, %rdi
	jne cbc_enc_rnd_loop

	pshufd $0x1b, RX0, RX0
	pshufb RX3, RX0
	movdqu RX0, (%rdx)

	add $0x10, %rsi
	add $0x10, %rdx
	sub $0x10, %r8
	jnz cbc_enc_loop
	movdqu RX0, (%rcx)

cbc_enc_over:
	FRAME_END
	RET
SYM_FUNC_END(cis_sm4_cbc_encrypt_blk8)

/**
 * void cis_sm4_cbc_decrypt_blk8(unsigned int *sk,
 *                                 unsigned char *in,
 *                                 unsigned char *out,
 *                                 unsigned char *iv,
 *                                 unsigned int len)
 */
SYM_TYPED_FUNC_START(cis_sm4_cbc_decrypt_blk8)
	/* input:
	 *      %rdi: round key array, sk
	 *      %rsi: input data, in
	 *      %rdx: output data, out
	 *      %rcx: iv data, iv
	 *      %r8: data length, len
	 */
	FRAME_BEGIN

	test %r8, %r8
	jle cbc_dec_over
	movq %rdi, %r9
	movdqa .Lbswap32_mask rRIP, RX3
	movdqu (%rcx), RX4

cbc_dec_loop:
	movq %r9, %rdi
	leaq (16*8)(%rdi), %rax
	movdqu (%rsi), RX0
	movdqa RX0, RX5
	pshufb RX3, RX0

cbc_dec_rnd_loop:
	movdqu (%rdi), RX1
	SM4_ROUND_INST_SM4RND(XMM0, XMM1)
	add $0x10, %rdi
	cmpq %rax, %rdi
	jne cbc_dec_rnd_loop

	pshufd $0x1b, RX0, RX0
	pshufb RX3, RX0
	pxor RX4, RX0
	movdqu RX0, (%rdx)
	movdqa RX5, RX4

	add $0x10, %rsi
	add $0x10, %rdx
	sub $0x10, %r8
	jnz cbc_dec_loop
	movdqu RX5, (%rcx)

cbc_dec_over:
	FRAME_END
	RET
SYM_FUNC_END(cis_sm4_cbc_decrypt_blk8)

/**
 * void cis_sm4_ctr_crypt_blk8(unsigned int *sk,
 *                               unsigned char *in,
 *                               unsigned char *out,
 *                               unsigned char *iv,
 *                               unsigned int len)
 */
SYM_TYPED_FUNC_START(cis_sm4_ctr_crypt_blk8)
	/* input:
	 *      %rdi: round key array, sk
	 *      %rsi: input data, in
	 *      %rdx: output data, out
	 *      %rcx: iv data, iv
	 *      %r8: data length, len
	 */
	FRAME_BEGIN

	test %r8, %r8
	jle ctr_over
	movq %rdi, %r9
	movdqa .Lbswap32_mask rRIP, RX3
	movdqa .Lbswap128_mask rRIP, RX6;
	movdqu (%rcx), RX4
	pcmpeqd RX5, RX5
	psrldq $8, RX5

#define inc_le128(x, minus_one, tmp)  \
	movdqa x, tmp;                \
	pcmpeqq minus_one, tmp;       \
	psubq minus_one, x;           \
	pslldq $8, tmp;               \
	psubq tmp, x

ctr_loop:
	movq %r9, %rdi
	leaq (16*8)(%rdi), %rax
	movdqa RX4, RX0
	pshufb RX3, RX0

ctr_rnd_loop:
	movdqu (%rdi), RX1
	SM4_ROUND_INST_SM4RND(XMM0, XMM1)
	add $0x10, %rdi
	cmpq %rax, %rdi
	jne ctr_rnd_loop

	pshufb RX6, RX4
	inc_le128(RX4, RX5, RX7)
	pshufb RX6, RX4

	pshufd $0x1b, RX0, RX0
	pshufb RX3, RX0
	movdqu (%rsi), RX2
	pxor RX2, RX0
	movdqu RX0, (%rdx)

	add $0x10, %rsi
	add $0x10, %rdx
	sub $0x10, %r8
	jnz ctr_loop
	movdqu RX4, (%rcx)

ctr_over:
	FRAME_END
	RET
SYM_FUNC_END(cis_sm4_ctr_crypt_blk8)


/**
 * void cis_sm4_cfb_encrypt_blk8(unsigned int *sk,
 *                                 unsigned char *in,
 *                                 unsigned char *out,
 *                                 unsigned char *iv,
 *                                 unsigned int len)
 */
SYM_TYPED_FUNC_START(cis_sm4_cfb_encrypt_blk8)
	/* input:
	 *      %rdi: round key array, sk
	 *      %rsi: input data, in
	 *      %rdx: output data, out
	 *      %rcx: iv data, iv
	 *      %r8: data length, len
	 */
	FRAME_BEGIN

	test %r8, %r8
	jle cfb_enc_over
	movdqa .Lbswap32_mask rRIP, RX3
	movq %rdi, %r9
	movdqu (%rcx), RX0

cfb_enc_loop:
	movq %r9, %rdi
	leaq (16*8)(%rdi), %rax
	pshufb RX3, RX0

cfb_enc_rnd_loop:
	movdqu (%rdi), RX1
	SM4_ROUND_INST_SM4RND(XMM0, XMM1)
	add $0x10, %rdi
	cmpq %rax, %rdi
	jne cfb_enc_rnd_loop

	pshufd $0x1b, RX0, RX0
	pshufb RX3, RX0
	movdqu (%rsi), RX2
	pxor RX2, RX0
	movdqu RX0, (%rdx)

	add $0x10, %rsi
	add $0x10, %rdx
	sub $0x10, %r8
	jnz cfb_enc_loop
	movdqu RX0, (%rcx)

cfb_enc_over:
	FRAME_END
	RET
SYM_FUNC_END(cis_sm4_cfb_encrypt_blk8)

/**
 * void cis_sm4_cfb_decrypt_blk8(unsigned int *sk,
 *                                 unsigned char *in,
 *                                 unsigned char *out,
 *                                 unsigned char *iv,
 *                                 unsigned int len)
 */
SYM_TYPED_FUNC_START(cis_sm4_cfb_decrypt_blk8)
	/* input:
	 *      %rdi: round key array, sk
	 *      %rsi: input data, in
	 *      %rdx: output data, out
	 *      %rcx: iv data, iv
	 *      %r8: data length, len
	 */
	FRAME_BEGIN

	test %r8, %r8
	jle cfb_dec_over
	movq %rdi, %r9
	movdqa .Lbswap32_mask rRIP, RX3
	movdqu (%rcx), RX4

cfb_dec_loop:
	movq %r9, %rdi
	leaq (16*8)(%rdi), %rax
	movdqa RX4, RX0
	pshufb RX3, RX0

cfb_dec_rnd_loop:
	movdqu (%rdi), RX1
	SM4_ROUND_INST_SM4RND(XMM0, XMM1)
	add $0x10, %rdi
	cmpq %rax, %rdi
	jne cfb_dec_rnd_loop

	pshufd $0x1b, RX0, RX0
	pshufb RX3, RX0
	movdqu (%rsi), RX4
	pxor RX4, RX0
	movdqu RX0, (%rdx)

	add $0x10, %rsi
	add $0x10, %rdx
	sub $0x10, %r8
	jnz cfb_dec_loop
	movdqu RX4, (%rcx)

cfb_dec_over:
	FRAME_END
	RET
SYM_FUNC_END(cis_sm4_cfb_decrypt_blk8)

/**
 * void cis_sm4_ofb_crypt_blk8(unsigned int *sk,
 *                               unsigned char *in,
 *                               unsigned char *out,
 *                               unsigned char *iv,
 *                               unsigned int len)
 */
SYM_TYPED_FUNC_START(cis_sm4_ofb_crypt_blk8)
	/* input:
	 *      %rdi: round key array, sk
	 *      %rsi: input data, in
	 *      %rdx: output data, out
	 *      %rcx: iv data, iv
	 *      %r8: data length, len
	 */
	FRAME_BEGIN

	test %r8, %r8
	jle ofb_over
	movq %rdi, %r9
	movdqa .Lbswap32_mask rRIP, RX3
	movdqu (%rcx), RX0

ofb_loop:
	movq %r9, %rdi
	leaq (16*8)(%rdi), %rax
	pshufb RX3, RX0

ofb_rnd_loop:
	movdqu (%rdi), RX1
	SM4_ROUND_INST_SM4RND(XMM0, XMM1)
	add $0x10, %rdi
	cmpq %rax, %rdi
	jne ofb_rnd_loop

	pshufd $0x1b, RX0, RX0
	pshufb RX3, RX0
	movdqa RX0, RX4
	movdqu (%rsi), RX2
	pxor RX2, RX4
	movdqu RX4, (%rdx)

	add $0x10, %rsi
	add $0x10, %rdx
	sub $0x10, %r8
	jnz ofb_loop
	movdqu RX0, (%rcx)

ofb_over:
	FRAME_END
	RET
SYM_FUNC_END(cis_sm4_ofb_crypt_blk8)

/**
 * void cis_sm4_xts_crypt_blk8(unsigned int *sk,
 *                             unsigned char *in,
 *                             unsigned char *out,
 *                             unsigned char *twk,
 *                             unsigned int len)
 */
SYM_TYPED_FUNC_START(cis_sm4_xts_crypt_blk8)
	/* input:
	 *      %rdi: round key array, sk
	 *      %rsi: inout data, in
	 *      %rdx: output data, out
	 *      %rcx: tweak value, twk
	 *      %r8:  data length, len
	 */
	FRAME_BEGIN
	test %r8, %r8;
	jle xts_over;
	movdqa .Lbswap32_mask rRIP, RX3;
	movdqa .Lbswap128_mask rRIP, RX6;
	movq %rdi, %r9;
	movdqu (%rcx), RX2;

xts_loop:
	movq %r9, %rdi;
	leaq (16*8)(%rdi), %rax;
	movdqu (%rsi), RX0;
	pxor RX2, RX0;
	pshufb RX3, RX0;

xts_rnd_loop:
	movdqu (%rdi), RX1;
	SM4_ROUND_INST_SM4RND(XMM0, XMM1);
	add $0x10, %rdi;
	cmpq %rax, %rdi;
	jne xts_rnd_loop;

	pshufd $0x1b, RX0, RX0;
	pshufb RX3, RX0;
	pxor RX2, RX0;
	movdqu RX0, (%rdx);

	pshufb RX6, RX2
	_gf128_primitive_mul(RX2, RX5, RX7);
	pshufb RX6, RX2;
	movdqu RX2, (%rcx);

	add $0x10, %rsi;
	add $0x10, %rdx;
	sub $0x10, %r8;
	jnz xts_loop;

xts_over:
	FRAME_END
	RET
SYM_FUNC_END(cis_sm4_xts_crypt_blk8)

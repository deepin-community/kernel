// SPDX-License-Identifier: GPL-2.0-only
/*
 * Hygon Cryptographic Coprocessor (CCP) SM2 crypto API support
 *
 * Copyright (C) 2022 Hygon Info Technologies Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/scatterlist.h>
#include <linux/crypto.h>
#include <crypto/algapi.h>
#include <crypto/internal/akcipher.h>
#include <crypto/akcipher.h>
#include <crypto/scatterwalk.h>

#include "ccp-crypto.h"
#include "ccp_sm2_sign.asn1.h"

static const u8 sm2_ecc_p[CCP_SM2_OPERAND_LEN] = {
	0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

static const u8 sm2_ecc_a[CCP_SM2_OPERAND_LEN] = {
	0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC,
};

static const u8 sm2_ecc_b[CCP_SM2_OPERAND_LEN] = {
	0x28, 0xE9, 0xFA, 0x9E, 0x9D, 0x9F, 0x5E, 0x34,
	0x4D, 0x5A, 0x9E, 0x4B, 0xCF, 0x65, 0x09, 0xA7,
	0xF3, 0x97, 0x89, 0xF5, 0x15, 0xAB, 0x8F, 0x92,
	0xDD, 0xBC, 0xBD, 0x41, 0x4D, 0x94, 0x0E, 0x93,
};

static const u8 sm2_ecc_n_sub_1[CCP_SM2_OPERAND_LEN] = {
	0xFF, 0xFF, 0xFF, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x72, 0x03, 0xDF, 0x6B, 0x21, 0xC6, 0x05, 0x2B,
	0x53, 0xBB, 0xF4, 0x09, 0x39, 0xD5, 0x41, 0x22,
};

static const u8 sm2_ecc_gx[CCP_SM2_OPERAND_LEN] = {
	0x32, 0xC4, 0xAE, 0x2C, 0x1F, 0x19, 0x81, 0x19,
	0x5F, 0x99, 0x04, 0x46, 0x6A, 0x39, 0xC9, 0x94,
	0x8F, 0xE3, 0x0B, 0xBF, 0xF2, 0x66, 0x0B, 0xE1,
	0x71, 0x5A, 0x45, 0x89, 0x33, 0x4C, 0x74, 0xC7,
};

static const u8 sm2_ecc_gy[CCP_SM2_OPERAND_LEN] = {
	0xBC, 0x37, 0x36, 0xA2, 0xF4, 0xF6, 0x77, 0x9C,
	0x59, 0xBD, 0xCE, 0xE3, 0x6B, 0x69, 0x21, 0x53,
	0xD0, 0xA9, 0x87, 0x7C, 0xC6, 0x2A, 0x47, 0x40,
	0x02, 0xDF, 0x32, 0xE5, 0x21, 0x39, 0xF0, 0xA0,
};

struct ccp_sm2_verify_src {
	u8 operand_e[CCP_SM2_OPERAND_LEN];	/* compressed message */
	u8 operand_d[CCP_SM2_OPERAND_LEN];	/* input data r */
	u8 operand_k[CCP_SM2_OPERAND_LEN];	/* input data s */
	u8 operand_px[CCP_SM2_OPERAND_LEN];	/* x of public key */
	u8 operand_py[CCP_SM2_OPERAND_LEN];	/* y of public key */
};

struct ccp_sm2_lp_src {
	u8 operand_k[CCP_SM2_OPERAND_LEN];	/* random number */
	u8 operand_px[CCP_SM2_OPERAND_LEN];	/* x of public key */
	u8 operand_py[CCP_SM2_OPERAND_LEN];	/* y of public key */
};

struct ccp_sm2_kg_src {
	u8 operand_k[CCP_SM2_OPERAND_LEN];	/* random number */
};

struct ccp_sm2_sign_src {
	u8 operand_e[CCP_SM2_OPERAND_LEN];	/* compressed message */
	u8 operand_d[CCP_SM2_OPERAND_LEN];	/* private key */
	u8 operand_k[CCP_SM2_OPERAND_LEN];	/* random number */
};

struct ccp_sm2_mmul_src {
	u8 operand_e[CCP_SM2_OPERAND_LEN];	/* mulplicand */
	u8 operand_d[CCP_SM2_OPERAND_LEN];	/* mulplicator */
};

struct ccp_sm2_dst {
	union {
		u8 result[CCP_SM2_OPERAND_LEN];
		u32 status;
	} u;
	u8 result_r[CCP_SM2_OPERAND_LEN];
	u8 result_s[CCP_SM2_OPERAND_LEN];
	u8 result_t[CCP_SM2_OPERAND_LEN];
};

struct sm2_signature_ctx {
	const u8 *sig_r;
	const u8 *sig_s;
	size_t r_len;
	size_t s_len;
};

int ccp_sm2_get_signature_r(void *context, size_t hdrlen, unsigned char tag,
				const void *value, size_t vlen)
{
	struct sm2_signature_ctx *sig = context;

	if (!value || !vlen)
		return -EINVAL;

	sig->sig_r = value;
	sig->r_len = vlen;

	if (!sig->sig_r)
		return -ENOMEM;

	return 0;
}

int ccp_sm2_get_signature_s(void *context, size_t hdrlen, unsigned char tag,
				const void *value, size_t vlen)
{
	struct sm2_signature_ctx *sig = context;

	if (!value || !vlen)
		return -EINVAL;

	sig->sig_s = value;
	sig->s_len = vlen;

	if (!sig->sig_s)
		return -ENOMEM;

	return 0;
}

static bool ccp_sm2_is_zero(const u64 *data, u32 count)
{
	u32 i;

	for (i = 0; i < count; i++) {
		if (data[i])
			return false;
	}

	return true;
}

/* Return:
 *  1: a > b
 * -1: a < b
 *  0: a = b
 */
static int ccp_sm2_fp_cmp(const u64 *a, const u64 *b, u32 count)
{
	u64 a_cpu, b_cpu;
	u32 i;

	for (i = 0; i < count; i++) {
		a_cpu = be64_to_cpu(a[i]);
		b_cpu = be64_to_cpu(b[i]);
		if (a_cpu > b_cpu)
			return 1;
		else if (a_cpu < b_cpu)
			return -1;
	}

	return 0;
}

/* a = a + b */
static void ccp_sm2_fp_add(u64 *a, const u64 *b, u32 count)
{
	u64 a_cpu, b_cpu, c_cpu, d_cpu;
	u32 carry = 0;
	s32 i;

	for (i = count - 1; i >= 0; i--) {
		a_cpu = be64_to_cpu(a[i]);
		b_cpu = be64_to_cpu(b[i]);
		c_cpu = a_cpu + b_cpu;
		d_cpu = c_cpu + carry;
		a[i] = cpu_to_be64(d_cpu);

		if (c_cpu < a_cpu)
			carry = 1;
		else if (carry && !d_cpu)
			carry = 1;
		else
			carry = 0;
	}
}

/* a = -a */
static void ccp_sm2_fp_neg(u64 *a, u32 count)
{
	u64 a_cpu, c_cpu;
	s32 i;

	for (i = 0; i <= count - 1; i++)
		a[i] = ~a[i];

	for (i = count - 1; i >= 0; i--) {
		a_cpu = be64_to_cpu(a[i]);
		c_cpu = a_cpu + 1;
		a[i] = cpu_to_be64(c_cpu);

		if (a_cpu < c_cpu)
			break;
	}
}

/* a = a - b */
static void ccp_sm2_fp_sub(u64 *a, u64 *b, u32 count)
{
	ccp_sm2_fp_neg(b, count);
	ccp_sm2_fp_add(a, b, count);
}

/* a and tmp must be 64B, b and c must be 32B
 * a = b * c
 */
static void ccp_sm2_fp_mmul32(u8 *a, const u32 *b, const u32 *c, u8 *tmp)
{
	u64 b_cpu, c_cpu, m_cpu;
	u32 rem_cpu;
	u32 *base, *m_cur;
	int i, j, iter;

	memset(a, 0, CCP_SM2_MMUL_LEN);

	iter = 7;
	base = (u32 *)(tmp + CCP_SM2_MMUL_LEN - sizeof(u32));
	for (i = iter; i >= 0; i--) {
		b_cpu = be32_to_cpu(b[i]);
		memset(tmp, 0, CCP_SM2_MMUL_LEN);

		rem_cpu = 0;
		m_cur = base;
		for (j = iter; j >= 0; j--) {
			c_cpu = be32_to_cpu(c[j]);

			m_cpu = b_cpu * c_cpu + rem_cpu;
			rem_cpu = (u32)(m_cpu >> 32);
			*m_cur = cpu_to_be32((u32)(m_cpu));
			m_cur--;
		}
		*m_cur = cpu_to_be32(rem_cpu);
		ccp_sm2_fp_add((u64 *)a, (u64 *)tmp,
				CCP_SM2_MMUL_LEN / sizeof(u64));

		base--;
	}
}

/* mmul, dst, tmp must be 64B, remainder in mmul[32-63]
 * high:low mod p
 * = high*2^256+low mod p
 * = high*(p+h)+low mod p
 * = high*h+low mod p
 * = high*(2^224+2^96-2^64+1)+low mod p
 * iterating 8 times
 */
static void ccp_sm2_fast_mod_p(u8 *mmul, u8 *dst, u8 *tmp)
{
	u8 *mmul_high, *mmul_low;
	u32 count;
	int i, iter, ret;

	mmul_high = mmul;
	mmul_low = mmul + CCP_SM2_OPERAND_LEN;
	count = CCP_SM2_MMUL_LEN / sizeof(u64);

	iter = 8;
	for (i = 0; i < iter; i++) {
		/* dst = high * 2^224 */
		memset(dst, 0, CCP_SM2_MMUL_LEN);
		memcpy(dst + 4, mmul_high, CCP_SM2_OPERAND_LEN);

		/* dst += high * 2^96 */
		memset(tmp, 0, CCP_SM2_MMUL_LEN);
		memcpy(tmp + 20, mmul_high, CCP_SM2_OPERAND_LEN);
		ccp_sm2_fp_add((u64 *)dst, (u64 *)tmp, count);

		/* dst += high * 2^64 */
		memset(tmp, 0, CCP_SM2_MMUL_LEN);
		memcpy(tmp + 24, mmul_high, CCP_SM2_OPERAND_LEN);
		ccp_sm2_fp_sub((u64 *)dst, (u64 *)tmp, count);

		/* dst += high * 1 */
		memset(tmp, 0, CCP_SM2_MMUL_LEN);
		memcpy(tmp + 32, mmul_high, CCP_SM2_OPERAND_LEN);
		ccp_sm2_fp_add((u64 *)dst, (u64 *)tmp, count);

		/* dst += low */
		memset(tmp, 0, CCP_SM2_MMUL_LEN);
		memcpy(tmp + 32, mmul_low, CCP_SM2_OPERAND_LEN);
		ccp_sm2_fp_add((u64 *)dst, (u64 *)tmp, count);

		/* copy dst to mmul */
		memcpy(mmul, dst, CCP_SM2_MMUL_LEN);
	}

	do {
		memset(tmp, 0, CCP_SM2_MMUL_LEN);
		memcpy(tmp + 32, sm2_ecc_p, CCP_SM2_OPERAND_LEN);
		ret = ccp_sm2_fp_cmp(
			(u64 *)mmul, (u64 *)tmp,
			CCP_SM2_MMUL_LEN / sizeof(u64));
		if (ret < 0)
			break;

		ccp_sm2_fp_sub((u64 *)mmul, (u64 *)tmp, count);
	} while (1);
}

static int ccp_sm2_is_privkey_valid(const u8 *priv_key)
{
	u64 last, last_cpu;
	bool zero;
	int ret;

	/* private key is satisfied with(1, n-1) */
	zero = ccp_sm2_is_zero((const u64 *)priv_key,
			CCP_SM2_PRIVATE_KEY_LEN / sizeof(u64) - 1);
	if (zero) {
		last = *(const u64 *)
			(priv_key + CCP_SM2_PRIVATE_KEY_LEN - sizeof(u64));
		last_cpu = be64_to_cpu(last);
		if (last_cpu <= 1)
			return -EINVAL;
	}

	ret = ccp_sm2_fp_cmp((const u64 *)priv_key,
			     (const u64 *)sm2_ecc_n_sub_1,
			     CCP_SM2_PRIVATE_KEY_LEN / sizeof(u64));
	if (ret >= 0)
		return -EINVAL;

	return 0;
}

static int ccp_sm2_setprivkey(struct crypto_akcipher *tfm,
		const void *key, unsigned int keylen)
{
	struct ccp_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct ccp_sm2_ctx *sm2 = &ctx->u.sm2;
	int ret;

	if (!key || keylen != CCP_SM2_PRIVATE_KEY_LEN)
		return -EINVAL;

	ret = ccp_sm2_is_privkey_valid(key);
	if (ret < 0)
		return ret;

	memcpy(sm2->pri_key, key, CCP_SM2_PRIVATE_KEY_LEN);
	sm2->pri_key_len = CCP_SM2_PRIVATE_KEY_LEN;

	return 0;
}

static int ccp_sm2_post_cmd(struct ccp_sm2_req_ctx *rctx,
	u32 src_size, enum ccp_sm2_mode mode, u32 rand)
{
	struct akcipher_request *req = rctx->req;
	struct ccp_sm2_engine *sm2 = NULL;
	int ret;

	sg_init_one(&rctx->src_sg, rctx->src, src_size);
	memset(rctx->dst, 0, CCP_SM2_DST_SIZE);
	sg_init_one(&rctx->dst_sg, rctx->dst, CCP_SM2_DST_SIZE);

	memset(&rctx->cmd, 0, sizeof(rctx->cmd));
	INIT_LIST_HEAD(&rctx->cmd.entry);
	rctx->cmd.engine = CCP_ENGINE_SM2;

	sm2 = &rctx->cmd.u.sm2;
	sm2->mode = mode;
	sm2->rand = rand;	/* whether read operand_k from trng */
	sm2->src = &rctx->src_sg;
	sm2->src_len = src_size;
	sm2->dst = &rctx->dst_sg;
	sm2->dst_len = CCP_SM2_DST_SIZE;

	ret = ccp_crypto_enqueue_request(&req->base, &rctx->cmd);

	return ret;
}

static int ccp_sm2_pubkey_strict_valid(const u8 *px, const u8 *py)
{
	u64 buf[CCP_SM2_OPERAND_LEN / sizeof(u64)];
	int ret1, ret2;

	/* private key is 1, corresponding public key is invalid */
	ret1 = memcmp(px, sm2_ecc_gx, CCP_SM2_OPERAND_LEN);
	ret2 = memcmp(py, sm2_ecc_gy, CCP_SM2_OPERAND_LEN);
	if (!ret1 && !ret2)
		return -EINVAL;

	/* private key is n - 1, corresponding public key is invalid */
	memcpy(buf, py, CCP_SM2_OPERAND_LEN);
	ccp_sm2_fp_add(buf, (const u64 *)sm2_ecc_gy,
			CCP_SM2_OPERAND_LEN / sizeof(u64));
	ret2 = memcmp(buf, sm2_ecc_p, CCP_SM2_OPERAND_LEN);
	if (!ret1 && !ret2)
		return -EINVAL;

	return 0;
}

static int ccp_sm2_is_pubkey_valid(struct ccp_sm2_req_ctx *rctx, bool strict)
{
	const u8 *px, *py;
	u8 *tmp;
	bool zero;
	int ret;

	px = rctx->src + CCP_SM2_LP_SRC_SIZE;
	py = px + CCP_SM2_OPERAND_LEN;

	zero = ccp_sm2_is_zero((u64 *)px, CCP_SM2_PUBLIC_KEY_LEN / sizeof(u64));
	if (zero)
		return -EINVAL;

	/* x < p */
	ret = ccp_sm2_fp_cmp((u64 *)px, (const u64 *)sm2_ecc_p,
				CCP_SM2_OPERAND_LEN / sizeof(u64));
	if (ret >= 0)
		return -EINVAL;

	/* y < p */
	ret = ccp_sm2_fp_cmp((u64 *)py, (const u64 *)sm2_ecc_p,
				CCP_SM2_OPERAND_LEN / sizeof(u64));
	if (ret >= 0)
		return -EINVAL;

	if (strict) {
		ret = ccp_sm2_pubkey_strict_valid(px, py);
		if (ret < 0)
			return ret;
	}

	/* check whether y^2 = x^3 + ax + b */
	tmp = rctx->dst + CCP_SM2_MMUL_LEN;
	/* y * y */
	ccp_sm2_fp_mmul32(rctx->dst, (u32 *)py, (u32 *)py, tmp);
	ccp_sm2_fast_mod_p(rctx->dst, rctx->src, tmp);
	memcpy(rctx->src + CCP_SM2_MMUL_LEN,
		rctx->dst + CCP_SM2_OPERAND_LEN, CCP_SM2_OPERAND_LEN);
	/* x * x + a */
	ccp_sm2_fp_mmul32(rctx->dst, (u32 *)px, (u32 *)px, tmp);
	memset(rctx->src, 0, CCP_SM2_MMUL_LEN);
	memcpy(rctx->src + CCP_SM2_OPERAND_LEN, sm2_ecc_a, CCP_SM2_OPERAND_LEN);
	ccp_sm2_fp_add((u64 *)rctx->dst, (u64 *)rctx->src,
				CCP_SM2_MMUL_LEN / sizeof(u64));
	ccp_sm2_fast_mod_p(rctx->dst, rctx->src, tmp);
	memcpy(rctx->src, rctx->dst + CCP_SM2_OPERAND_LEN, CCP_SM2_OPERAND_LEN);
	/* (x * x + a) * x + b */
	ccp_sm2_fp_mmul32(rctx->dst, (u32 *)px, (u32 *)rctx->src, tmp);
	memset(rctx->src, 0, CCP_SM2_MMUL_LEN);
	memcpy(rctx->src + CCP_SM2_OPERAND_LEN, sm2_ecc_b, CCP_SM2_OPERAND_LEN);
	ccp_sm2_fp_add((u64 *)rctx->dst, (u64 *)rctx->src,
				CCP_SM2_MMUL_LEN / sizeof(u64));
	ccp_sm2_fast_mod_p(rctx->dst, rctx->src, tmp);

	ret = memcmp(rctx->src + CCP_SM2_MMUL_LEN,
		rctx->dst + CCP_SM2_OPERAND_LEN, CCP_SM2_OPERAND_LEN);
	if (ret)
		return -EINVAL;

	/* Because the cofactor of the ECC group is 1,
	 * the checking that [n]P=O is not required.
	 */

	return 0;
}

static int ccp_sm2_setpubkey(struct crypto_akcipher *tfm,
		const void *key, unsigned int keylen)
{
	struct ccp_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct ccp_sm2_ctx *sm2 = &ctx->u.sm2;
	struct ccp_sm2_req_ctx *rctx = NULL;
	const unsigned char *cflag = (const unsigned char *)key;
	int ret;

	if (!key || keylen < CCP_SM2_PUBLIC_KEY_LEN)
		return -EINVAL;

	/*  When the length of sm2 public key is 65,
	 *  content of key should be 04 || X || Y, from GM/T0009-2012.
	 */
	if (keylen > CCP_SM2_PUBLIC_KEY_LEN) {
		if (*cflag != 0x04)
			return -EINVAL;
		key = key + 1;
	}

	/* check whether public key is valid */
	rctx = kmalloc(sizeof(*rctx), GFP_KERNEL);
	if (!rctx)
		return -ENOMEM;

	memcpy(rctx->src + CCP_SM2_LP_SRC_SIZE, key, CCP_SM2_PUBLIC_KEY_LEN);
	ret = ccp_sm2_is_pubkey_valid(rctx, true);
	kfree(rctx);
	if (ret < 0)
		return ret;

	/* public key is valid */
	memcpy(sm2->pub_key, key, CCP_SM2_PUBLIC_KEY_LEN);
	sm2->pub_key_len = CCP_SM2_PUBLIC_KEY_LEN;

	return 0;
}

static unsigned int ccp_sm2_maxsize(struct crypto_akcipher *tfm)
{
	return CCP_SM2_DST_SIZE;
}

static int ccp_sm2_compute_c3(struct crypto_shash *shash,
		struct scatterlist *sg, u32 mlen,
		u8 *c3, const u8 *x2, const u8 *y2)
{
	unsigned int len, remain;
	int ret;

	SHASH_DESC_ON_STACK(sdesc, shash);

	sdesc->tfm = shash;
	ret = crypto_shash_init(sdesc);
	if (ret < 0)
		return ret;

	/* update X2 */
	ret = crypto_shash_update(sdesc, x2, CCP_SM2_OPERAND_LEN);
	if (ret < 0)
		return ret;

	/* update M */
	remain = mlen;
	while (sg) {
		len = sg->length;
		if (len > remain)
			len = remain;
		ret = crypto_shash_update(sdesc, (u8 *)sg_virt(sg), len);
		if (ret < 0)
			return ret;

		remain -= len;
		if (!remain)
			break;

		sg = sg_next(sg);
	}

	/* ccp_sm2_encrypt should have checked length */
	if (unlikely(!sg))
		return -EINVAL;

	/* update Y2 */
	ret = crypto_shash_finup(sdesc, y2, CCP_SM2_OPERAND_LEN, c3);

	return ret;
}

static bool ccp_sm2_msg_xor_t(u8 *msg, const u8 *t, u32 len)
{
	u64 *msg_cur, *msg_last, *t_cur;
	u32 zero_cnt = 0;
	u32 rem;
	int i;

	msg_cur = (u64 *)msg;
	t_cur = (u64 *)t;
	msg_last = msg_cur + (len / sizeof(u64));
	while (msg_cur != msg_last) {
		if (likely(*t_cur))
			*msg_cur = *msg_cur ^ *t_cur;
		else
			zero_cnt += sizeof(u64);

		msg_cur++;
		t_cur++;
	}

	msg = (u8 *)msg_cur;
	t = (const u8 *)t_cur;
	rem = len % sizeof(u64);
	for (i = 0; i < rem; i++) {
		if (likely(t[i]))
			msg[i] = msg[i] ^ t[i];
		else
			zero_cnt++;
	}

	return zero_cnt == len;
}

static int ccp_sm2_kdf_xor(struct crypto_shash *shash,
	struct scatterlist *src, u32 src_offset, u32 src_len,
	struct scatterlist *dst, u32 dst_offset,
	u8 *x2_y2_ct, bool *all_zero, struct ccp_sm2_req_ctx *rctx)
{
	u32 *be_ct = NULL;
	u32 ct, len, remain;
	bool zero;
	int ret = 0;

	SHASH_DESC_ON_STACK(sdesc, shash);

	sdesc->tfm = shash;

	*all_zero = true;
	ct = 1;
	be_ct = (u32 *)(x2_y2_ct + CCP_SM2_PUBLIC_KEY_LEN);
	remain = src_len;
	while (remain) {
		len = SM3_DIGEST_SIZE;
		if (len > remain)
			len = remain;
		*be_ct = cpu_to_be32(ct);
		ret = crypto_shash_digest(sdesc, x2_y2_ct,
			CCP_SM2_PUBLIC_KEY_LEN + sizeof(*be_ct), rctx->src);
		if (ret < 0)
			break;

		scatterwalk_map_and_copy(rctx->src + SM3_DIGEST_SIZE, src,
					src_offset, len, 0);
		zero = ccp_sm2_msg_xor_t(rctx->src + SM3_DIGEST_SIZE,
					rctx->src, len);
		if (zero == false)
			*all_zero = false;
		scatterwalk_map_and_copy(rctx->src + SM3_DIGEST_SIZE, dst,
					dst_offset, len, 1);

		remain -= len;
		src_offset += len;
		dst_offset += len;
		ct++;
	}

	return ret;
}

static void ccp_sm2_enc_compute(struct work_struct *work)
{
	struct ccp_sm2_req_ctx *rctx =
			container_of(work, struct ccp_sm2_req_ctx, work);
	struct akcipher_request *req = rctx->req;
	struct ccp_sm2_dst *dst = (struct ccp_sm2_dst *)rctx->dst;
	struct crypto_shash *shash = NULL;
	bool all_zero = true;
	int ret;

	shash = crypto_alloc_shash("sm3", 0, 0);
	if (IS_ERR(shash)) {
		ret = PTR_ERR(shash);
		goto e_complete;
	}

	/* C2 = M ^ t */
	ret = ccp_sm2_kdf_xor(shash, req->src, 0, req->src_len,
			req->dst, CCP_SM2_ENCRYPT_EXT_LEN,
			dst->result_r, &all_zero, rctx);
	if (ret < 0)
		goto e_hash;
	if (unlikely(all_zero)) {
		ret = -EAGAIN;
		goto e_hash;
	}

	/* C3 */
	ret = ccp_sm2_compute_c3(shash, req->src, req->src_len, rctx->src,
			dst->result_r, dst->result_s);
	if (ret < 0)
		goto e_hash;

	/* save C3 */
	scatterwalk_map_and_copy(rctx->src, req->dst,
		CCP_SM2_PUBLIC_KEY_LEN, SM3_DIGEST_SIZE, 1);

e_hash:
	crypto_free_shash(shash);

e_complete:
	req->base.complete(req->base.data, ret);
}

static void ccp_sm2_enc_lp(struct work_struct *work)
{
	struct ccp_sm2_req_ctx *rctx =
			container_of(work, struct ccp_sm2_req_ctx, work);
	struct akcipher_request *req = rctx->req;
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	struct ccp_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct ccp_sm2_dst *dst = (struct ccp_sm2_dst *)rctx->dst;
	struct ccp_sm2_lp_src *src = (struct ccp_sm2_lp_src *)rctx->src;
	int ret;

	/* save C1 */
	scatterwalk_map_and_copy(dst->result_r, req->dst, 0,
					CCP_SM2_PUBLIC_KEY_LEN, 1);
	/* operand_k used by kg is placed in dst->result_t */
	memcpy(src->operand_k, dst->result_t, CCP_SM2_OPERAND_LEN);
	memcpy(src->operand_px, ctx->u.sm2.pub_key, CCP_SM2_OPERAND_LEN);
	memcpy(src->operand_py, ctx->u.sm2.pub_key + CCP_SM2_OPERAND_LEN,
						CCP_SM2_OPERAND_LEN);
	rctx->phase = CCP_SM2_ENC_PH_LP;

	ret = ccp_sm2_post_cmd(rctx, CCP_SM2_LP_SRC_SIZE, CCP_SM2_MODE_LP, 0);
	if (ret != -EBUSY && ret != -EINPROGRESS)
		req->base.complete(req->base.data, ret);
}

static int ccp_sm2_encrypt(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	struct ccp_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct ccp_sm2_req_ctx *rctx = akcipher_request_ctx(req);
	int nents;
	int ret;

	if (!ctx->u.sm2.pub_key_len)
		return -ENOKEY;

	if (!req->src_len ||
		req->dst_len < CCP_SM2_ENCRYPT_EXT_LEN + req->src_len)
		return -EINVAL;

	nents = sg_nents_for_len(req->src, req->src_len);
	if (nents < 0)
		return -EINVAL;

	rctx->req = req;
	rctx->phase = CCP_SM2_ENC_PH_KG;
	ret = ccp_sm2_post_cmd(rctx, CCP_SM2_KG_SRC_SIZE, CCP_SM2_MODE_KG, 1);

	return ret;
}

static void ccp_sm2_dec_compute(struct work_struct *work)
{
	struct ccp_sm2_req_ctx *rctx =
			container_of(work, struct ccp_sm2_req_ctx, work);
	struct akcipher_request *req = rctx->req;
	struct ccp_sm2_dst *dst = (struct ccp_sm2_dst *)rctx->dst;
	struct crypto_shash *shash = NULL;
	bool all_zero = true;
	int ret;

	shash = crypto_alloc_shash("sm3", 0, 0);
	if (IS_ERR(shash)) {
		ret = PTR_ERR(shash);
		goto e_complete;
	}

	/* M' = C2 ^ t */
	ret = ccp_sm2_kdf_xor(shash, req->src, CCP_SM2_ENCRYPT_EXT_LEN,
		req->src_len - CCP_SM2_ENCRYPT_EXT_LEN, req->dst, 0,
		dst->result_r, &all_zero, rctx);
	if (ret < 0)
		goto e_hash;
	if (all_zero) {
		ret = -EBADMSG;
		goto e_hash;
	}

	/* u */
	ret = ccp_sm2_compute_c3(shash, req->dst,
		req->src_len - CCP_SM2_ENCRYPT_EXT_LEN,
		rctx->src, dst->result_r, dst->result_s);
	if (ret < 0)
		goto e_hash;

	/* load and compare C3 */
	scatterwalk_map_and_copy(rctx->src + SM3_DIGEST_SIZE, req->src,
		CCP_SM2_PUBLIC_KEY_LEN, SM3_DIGEST_SIZE, 0);
	ret = memcmp(rctx->src, rctx->src + SM3_DIGEST_SIZE, SM3_DIGEST_SIZE);
	if (ret)
		ret = -EBADMSG;

e_hash:
	crypto_free_shash(shash);

e_complete:
	/* clear private key, plain, and dC1 */
	memset(rctx->src, 0, CCP_SM2_OPERAND_LEN * 2);
	memset(dst, 0, CCP_SM2_DST_SIZE);
	req->base.complete(req->base.data, ret);
}

static int ccp_sm2_decrypt(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	struct ccp_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct ccp_sm2_req_ctx *rctx = akcipher_request_ctx(req);
	struct ccp_sm2_lp_src *src = (struct ccp_sm2_lp_src *)rctx->src;
	int nents;
	int ret;

	if (!ctx->u.sm2.pri_key_len)
		return -ENOKEY;

	if (req->src_len <= (CCP_SM2_PUBLIC_KEY_LEN + SM3_DIGEST_SIZE))
		return -EINVAL;

	if (req->dst_len < req->src_len - CCP_SM2_ENCRYPT_EXT_LEN)
		return -EINVAL;

	nents = sg_nents_for_len(req->src, req->src_len);
	if (nents < 0)
		return -EINVAL;

	/* load C1 */
	scatterwalk_map_and_copy(rctx->src + CCP_SM2_LP_SRC_SIZE,
				req->src, 0, CCP_SM2_PUBLIC_KEY_LEN, 0);
	ret = ccp_sm2_is_pubkey_valid(rctx, false);
	if (ret < 0)
		return -EBADMSG;

	/* do kP */
	memcpy(src->operand_k, ctx->u.sm2.pri_key, CCP_SM2_PRIVATE_KEY_LEN);
	memcpy(src->operand_px, rctx->src + CCP_SM2_LP_SRC_SIZE,
						CCP_SM2_OPERAND_LEN);
	memcpy(src->operand_py, rctx->src + CCP_SM2_LP_SRC_SIZE
						+ CCP_SM2_OPERAND_LEN, CCP_SM2_OPERAND_LEN);
	rctx->req = req;
	rctx->phase = CCP_SM2_DEC_PH_LP;
	ret = ccp_sm2_post_cmd(rctx, CCP_SM2_LP_SRC_SIZE, CCP_SM2_MODE_LP, 0);

	return ret;
}

static int ccp_sm2_sign(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	struct ccp_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct ccp_sm2_req_ctx *rctx = akcipher_request_ctx(req);
	struct ccp_sm2_sign_src *src = (struct ccp_sm2_sign_src *)rctx->src;
	int nents;
	int ret;

	if (!ctx->u.sm2.pri_key_len)
		return -ENOKEY;

	if (req->src_len != CCP_SM2_OPERAND_LEN)
		return -EINVAL;

	nents = sg_nents_for_len(req->src, CCP_SM2_OPERAND_LEN);
	if (nents < 0)
		return -EINVAL;

	scatterwalk_map_and_copy(src->operand_e, req->src, 0,
					CCP_SM2_OPERAND_LEN, 0);
	memcpy(src->operand_d, ctx->u.sm2.pri_key, CCP_SM2_PRIVATE_KEY_LEN);

	rctx->req = req;
	rctx->phase = CCP_SM2_SIGN_PH_SIGN;
	ret = ccp_sm2_post_cmd(rctx, CCP_SM2_SIGN_SRC_SIZE,
					CCP_SM2_MODE_SIGN, 1);

	return ret;
}

static int ccp_sm2_verify(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	struct ccp_ctx *ctx = akcipher_tfm_ctx(tfm);
	struct ccp_sm2_req_ctx *rctx = akcipher_request_ctx(req);
	struct ccp_sm2_verify_src *src = (struct ccp_sm2_verify_src *)rctx->src;
	int siglen;
	int nents;
	int ret;
	struct sm2_signature_ctx sig;
	unsigned char *buffer;

	if (!ctx->u.sm2.pub_key_len)
		return -ENOKEY;

	if (req->src_len == CCP_SM2_OPERAND_LEN * 3) {
		/* Compatible with non-encoded signature from user space */
		nents = sg_nents_for_len(req->src, CCP_SM2_OPERAND_LEN * 3);
		if (nents < 0)
			return -EINVAL;

		scatterwalk_map_and_copy(src->operand_e, req->src, 0,
						CCP_SM2_OPERAND_LEN * 3, 0);
		memcpy(src->operand_px, ctx->u.sm2.pub_key, CCP_SM2_OPERAND_LEN);
		memcpy(src->operand_py, ctx->u.sm2.pub_key + CCP_SM2_OPERAND_LEN,
							CCP_SM2_OPERAND_LEN);

		rctx->req = req;
		rctx->phase = CCP_SM2_VERIFY_PH_VERIFY;
		ret = ccp_sm2_post_cmd(rctx, CCP_SM2_VERIFY_SRC_SIZE,
						CCP_SM2_MODE_VERIFY, 0);

		return ret;
	} else if (req->src_len < CCP_SM2_OPERAND_LEN * 3) {
		/* Compatible with usage like sm2 test of testmgr */
		siglen = req->src_len;
		if (req->dst_len != CCP_SM2_OPERAND_LEN)
			return -EINVAL;
	} else {
		/* deal with der encoding signature from user space */
		siglen = req->src_len - CCP_SM2_OPERAND_LEN;
	}

	buffer = kmalloc(siglen + CCP_SM2_OPERAND_LEN, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	sg_pcopy_to_buffer(req->src,
		sg_nents_for_len(req->src, siglen + CCP_SM2_OPERAND_LEN),
		buffer, siglen + CCP_SM2_OPERAND_LEN, 0);

	sig.sig_r = NULL;
	sig.sig_s = NULL;
	ret = asn1_ber_decoder(&ccp_sm2_sign_decoder, &sig,
				buffer, siglen);

	if (ret)
		goto error;

	memcpy(src->operand_e, buffer + siglen, CCP_SM2_OPERAND_LEN);

	if (sig.r_len > CCP_SM2_OPERAND_LEN)
		memcpy(src->operand_d, sig.sig_r + 1, CCP_SM2_OPERAND_LEN);
	else
		memcpy(src->operand_d, sig.sig_r, CCP_SM2_OPERAND_LEN);

	if (sig.s_len > CCP_SM2_OPERAND_LEN)
		memcpy(src->operand_k, sig.sig_s + 1, CCP_SM2_OPERAND_LEN);
	else
		memcpy(src->operand_k, sig.sig_s, CCP_SM2_OPERAND_LEN);

	memcpy(src->operand_px, ctx->u.sm2.pub_key, CCP_SM2_OPERAND_LEN);
	memcpy(src->operand_py, ctx->u.sm2.pub_key + CCP_SM2_OPERAND_LEN,
						CCP_SM2_OPERAND_LEN);

	rctx->req = req;
	rctx->phase = CCP_SM2_VERIFY_PH_VERIFY;
	ret = ccp_sm2_post_cmd(rctx, CCP_SM2_VERIFY_SRC_SIZE,
					CCP_SM2_MODE_VERIFY, 0);

error:
	kfree(buffer);
	return ret;
}

static int ccp_sm2_verify_handle(struct ccp_sm2_req_ctx *rctx)
{
	struct ccp_sm2_dst *dst = (struct ccp_sm2_dst *)rctx->dst;

	if (dst->u.status)
		return -EBADMSG;

	return 0;
}

static int ccp_sm2_sign_handle(struct ccp_sm2_req_ctx *rctx)
{
	struct ccp_sm2_dst *dst = (struct ccp_sm2_dst *)rctx->dst;
	struct ccp_sm2_sign_src *src = (struct ccp_sm2_sign_src *)rctx->src;
	struct akcipher_request *req = rctx->req;

	if (unlikely(dst->u.status))
		return -EAGAIN;

	/* save signature */
	scatterwalk_map_and_copy(dst->result_r, req->dst, 0,
					CCP_SM2_OPERAND_LEN * 2, 1);
	/* clear private key */
	memset(src->operand_d, 0, CCP_SM2_PRIVATE_KEY_LEN);

	return 0;
}

static int ccp_sm2_enc_kg_handle(struct ccp_sm2_req_ctx *rctx)
{
	struct ccp_sm2_dst *dst = (struct ccp_sm2_dst *)rctx->dst;

	/* random operand_k is not satisfied with[1, n-1], try again */
	if (unlikely(dst->u.status))
		return -EAGAIN;

	INIT_WORK(&rctx->work, ccp_sm2_enc_lp);
	schedule_work(&rctx->work);

	return -EINPROGRESS;
}

static int ccp_sm2_enc_lp_handle(struct ccp_sm2_req_ctx *rctx)
{
	struct ccp_sm2_dst *dst = (struct ccp_sm2_dst *)rctx->dst;

	if (unlikely(dst->u.status))
		return -EIO;

	INIT_WORK(&rctx->work, ccp_sm2_enc_compute);
	schedule_work(&rctx->work);

	return -EINPROGRESS;
}

static int ccp_sm2_dec_lp_handle(struct ccp_sm2_req_ctx *rctx)
{
	struct ccp_sm2_dst *dst = (struct ccp_sm2_dst *)rctx->dst;

	if (unlikely(dst->u.status))
		return -EIO;

	INIT_WORK(&rctx->work, ccp_sm2_dec_compute);
	schedule_work(&rctx->work);

	return -EINPROGRESS;
}

static int ccp_sm2_complete(struct crypto_async_request *async_req, int ret)
{
	struct akcipher_request *req =
		container_of(async_req, struct akcipher_request, base);
	struct ccp_sm2_req_ctx *rctx = akcipher_request_ctx(req);

	if (ret)
		return ret;

	switch (rctx->phase) {
	case CCP_SM2_SIGN_PH_SIGN:
		ret = ccp_sm2_sign_handle(rctx);
		break;
	case CCP_SM2_VERIFY_PH_VERIFY:
		ret = ccp_sm2_verify_handle(rctx);
		break;
	case CCP_SM2_ENC_PH_KG:
		ret = ccp_sm2_enc_kg_handle(rctx);
		break;
	case CCP_SM2_ENC_PH_LP:
		ret = ccp_sm2_enc_lp_handle(rctx);
		break;
	case CCP_SM2_DEC_PH_LP:
		ret = ccp_sm2_dec_lp_handle(rctx);
		break;
	}

	return ret;
}

static int ccp_sm2_init_tfm(struct crypto_akcipher *tfm)
{
	struct ccp_ctx *ctx = akcipher_tfm_ctx(tfm);

	akcipher_set_reqsize(tfm, sizeof(struct ccp_sm2_req_ctx));
	ctx->complete = ccp_sm2_complete;

	return 0;
}

static void ccp_sm2_exit_tfm(struct crypto_akcipher *tfm)
{
}

static struct akcipher_alg ccp_sm2_defaults = {
	.sign		= ccp_sm2_sign,
	.verify		= ccp_sm2_verify,
	.encrypt	= ccp_sm2_encrypt,
	.decrypt	= ccp_sm2_decrypt,
	.set_pub_key	= ccp_sm2_setpubkey,
	.set_priv_key	= ccp_sm2_setprivkey,
	.max_size	= ccp_sm2_maxsize,
	.init		= ccp_sm2_init_tfm,
	.exit		= ccp_sm2_exit_tfm,
	.base		= {
		.cra_flags	= CRYPTO_ALG_ASYNC |
				  CRYPTO_ALG_KERN_DRIVER_ONLY,
		.cra_ctxsize	= sizeof(struct ccp_ctx),
		.cra_priority	= CCP_CRA_PRIORITY,
		.cra_module	= THIS_MODULE,
	},
};

struct ccp_sm2_def {
	unsigned int version;
	const char *name;
	const char *driver_name;
	struct akcipher_alg *alg_defaults;
};

static struct ccp_sm2_def sm2_algs[] = {
	{
		.version	= CCP_VERSION(5, 0),
		.name		= "sm2",
		.driver_name	= "sm2-ccp",
		.alg_defaults	= &ccp_sm2_defaults,
	}
};

static int ccp_register_sm2_hygon_alg(struct list_head *head,
				const struct ccp_sm2_def *def)
{
	struct ccp_crypto_akcipher_alg *ccp_alg;
	struct akcipher_alg *alg;
	int ret;

	ccp_alg = kzalloc(sizeof(*ccp_alg), GFP_KERNEL);
	if (!ccp_alg)
		return -ENOMEM;

	INIT_LIST_HEAD(&ccp_alg->entry);

	alg = &ccp_alg->alg;
	*alg = *def->alg_defaults;
	snprintf(alg->base.cra_name, CRYPTO_MAX_ALG_NAME, "%s", def->name);
	snprintf(alg->base.cra_driver_name, CRYPTO_MAX_ALG_NAME, "%s",
		 def->driver_name);

	ret = crypto_register_akcipher(alg);
	if (ret) {
		pr_err("%s akcipher algorithm registration error (%d)\n",
			alg->base.cra_name, ret);
		kfree(ccp_alg);
		return ret;
	}

	list_add(&ccp_alg->entry, head);

	return 0;
}

int ccp_register_sm2_hygon_algs(struct list_head *head)
{
	int i, ret;
	unsigned int ccpversion = ccp_version();

	for (i = 0; i < ARRAY_SIZE(sm2_algs); i++) {
		if (sm2_algs[i].version > ccpversion)
			continue;
		ret = ccp_register_sm2_hygon_alg(head, &sm2_algs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

// SPDX-License-Identifier: GPL-2.0-only
/*
 * HYGON SM4 Cipher Algorithm, using cis instructions.
 *
 * Copyright (C) 2026 Hygon Information Technology Co., Ltd.
 *
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/align.h>
#include <linux/crypto.h>
#include <asm/cpu.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <crypto/b128ops.h>
#include <crypto/hash.h>
#include <crypto/ghash.h>
#include <crypto/gcm.h>
#include <crypto/sm4.h>
#include <crypto/scatterwalk.h>
#include <crypto/internal/skcipher.h>
#include <crypto/internal/aead.h>
#include <crypto/internal/simd.h>

asmlinkage void gf128_mul_by_2(unsigned char *twk);
asmlinkage void cis_sm4_set_key(const unsigned char *key,
				unsigned int *rk);
asmlinkage void cis_sm4_blk(unsigned int *sk,
				unsigned char *in,
				unsigned char *out);
asmlinkage void cis_sm4_ecb_crypt_blk8(unsigned int *sk,
				unsigned char *in,
				unsigned char *out,
				unsigned int len);
asmlinkage void cis_sm4_cbc_encrypt_blk8(unsigned int *sk,
				unsigned char *in,
				unsigned char *out,
				unsigned char *iv,
				unsigned int len);
asmlinkage void cis_sm4_cbc_decrypt_blk8(unsigned int *sk,
				unsigned char *in,
				unsigned char *out,
				unsigned char *iv,
				unsigned int len);
asmlinkage void cis_sm4_ctr_crypt_blk8(unsigned int *sk,
				unsigned char *in,
				unsigned char *out,
				unsigned char *iv,
				unsigned int len);
asmlinkage void cis_sm4_cfb_encrypt_blk8(unsigned int *sk,
				unsigned char *in,
				unsigned char *out,
				unsigned char *iv,
				unsigned int len);
asmlinkage void cis_sm4_cfb_decrypt_blk8(unsigned int *sk,
				unsigned char *in,
				unsigned char *out,
				unsigned char *iv,
				unsigned int len);
asmlinkage void cis_sm4_ofb_crypt_blk8(unsigned int *sk,
				unsigned char *in,
				unsigned char *out,
				unsigned char *iv,
				unsigned int len);
asmlinkage void cis_sm4_xts_crypt_blk8(unsigned int *sk,
				unsigned char *in,
				unsigned char *out,
				unsigned char *twk,
				unsigned int len);

struct cis_sm4_ctx {
	struct sm4_ctx ctx;
	struct sm4_ctx tweak_ctx;
	struct crypto_shash *ghash_tfm;
	struct shash_desc *ghash_desc;
};

#define CRYPTO_SM4_CTX_SIZE		 sizeof(struct cis_sm4_ctx)

static int sm4_set_key(struct crypto_skcipher *tfm,
				const u8 *key, unsigned int key_len)
{
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;
	int i = 0;

	if (key_len != SM4_KEY_SIZE)
		return -EINVAL;

	kernel_fpu_begin();
	cis_sm4_set_key(key, ctx->rkey_enc);
	kernel_fpu_end();

	for (i = 0; i < SM4_RKEY_WORDS; i++)
		ctx->rkey_dec[i] = ctx->rkey_enc[SM4_RKEY_WORDS - 1 - i];

	return 0;
}

static int sm4_ecb_do_crypt(struct skcipher_request *req, u32 *rkey)
{
	struct skcipher_walk walk;
	unsigned int nbytes = 0;
	int err = 0;

	if (req->cryptlen & (SM4_BLOCK_SIZE - 1))
		return -EINVAL;

	err = skcipher_walk_virt(&walk, req, false);
	if (err)
		return err;

	while ((nbytes = walk.nbytes) > 0) {
		nbytes &= ~(SM4_BLOCK_SIZE - 1);
		kernel_fpu_begin();
		cis_sm4_ecb_crypt_blk8(rkey, walk.src.virt.addr, walk.dst.virt.addr, nbytes);
		kernel_fpu_end();
		err = skcipher_walk_done(&walk, walk.nbytes - nbytes);
		if (err)
			break;
	}

	return err;
}

static int sm4_ecb_encrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;

	return sm4_ecb_do_crypt(req, ctx->rkey_enc);
}

static int sm4_ecb_decrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;

	return sm4_ecb_do_crypt(req, ctx->rkey_dec);
}

static int sm4_cbc_do_crypt(struct skcipher_request *req,
			 struct cis_sm4_ctx *cis_ctx, bool encrypt)
{
	struct skcipher_walk walk;
	struct sm4_ctx *ctx = &cis_ctx->ctx;
	unsigned int nbytes;
	int err;

	if (req->cryptlen & (SM4_BLOCK_SIZE - 1))
		return -EINVAL;

	err = skcipher_walk_virt(&walk, req, false);
	if (err)
		return err;

	while ((nbytes = walk.nbytes) > 0) {
		nbytes &= ~(SM4_BLOCK_SIZE - 1);
		kernel_fpu_begin();
		if (encrypt) {
			cis_sm4_cbc_encrypt_blk8(ctx->rkey_enc,
				walk.src.virt.addr, walk.dst.virt.addr, walk.iv, nbytes);
		} else {
			cis_sm4_cbc_decrypt_blk8(ctx->rkey_dec,
				walk.src.virt.addr, walk.dst.virt.addr, walk.iv, nbytes);
		}
		kernel_fpu_end();
		err = skcipher_walk_done(&walk, walk.nbytes - nbytes);
		if (err)
			break;
	}

	return err;
}

static int sm4_cbc_encrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);

	return sm4_cbc_do_crypt(req, cis_ctx, true);
}

static int sm4_cbc_decrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);

	return sm4_cbc_do_crypt(req, cis_ctx, false);
}

static int sm4_ctr_do_crypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;
	struct skcipher_walk walk;
	u8 keystream[SM4_BLOCK_SIZE];
	unsigned int nbytes, remain;
	int err;

	err = skcipher_walk_virt(&walk, req, false);
	if (err)
		return err;

	while ((nbytes = walk.nbytes) > 0) {
		nbytes &= ~(SM4_BLOCK_SIZE - 1);
		remain = walk.nbytes - nbytes;
		kernel_fpu_begin();
		if (nbytes) {
			cis_sm4_ctr_crypt_blk8(ctx->rkey_enc,
				walk.src.virt.addr, walk.dst.virt.addr, walk.iv, nbytes);
		}

		if (walk.nbytes == walk.total && remain > 0) {
			cis_sm4_blk(ctx->rkey_enc, walk.iv, keystream);
			crypto_xor_cpy(walk.dst.virt.addr + nbytes,
					   walk.src.virt.addr + nbytes,
					   keystream, remain);
			crypto_inc(walk.iv, SM4_BLOCK_SIZE);
			remain = 0;
		}
		kernel_fpu_end();
		err = skcipher_walk_done(&walk, remain);
		if (err)
			break;
	}
	return err;
}

static int sm4_cfb_encrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;
	struct skcipher_walk walk;
	u8 keystream[SM4_BLOCK_SIZE];
	unsigned int nbytes, remain;
	int err;

	err = skcipher_walk_virt(&walk, req, false);
	if (err)
		return err;
	while ((nbytes = walk.nbytes) > 0) {
		nbytes &= ~(SM4_BLOCK_SIZE - 1);
		remain = walk.nbytes - nbytes;
		kernel_fpu_begin();
		if (nbytes) {
			cis_sm4_cfb_encrypt_blk8(ctx->rkey_enc,
				walk.src.virt.addr, walk.dst.virt.addr, walk.iv, nbytes);
		}

		if (walk.nbytes == walk.total && remain > 0) {
			cis_sm4_blk(ctx->rkey_enc, walk.iv, keystream);
			crypto_xor_cpy(walk.dst.virt.addr + nbytes,
					   walk.src.virt.addr + nbytes,
					   keystream, remain);
			remain = 0;
		}

		kernel_fpu_end();
		err = skcipher_walk_done(&walk, remain);
		if (err)
			break;
	}
	return err;
}

static int sm4_cfb_decrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;
	struct skcipher_walk walk;
	u8 keystream[SM4_BLOCK_SIZE];
	unsigned int nbytes, remain;
	int err;

	err = skcipher_walk_virt(&walk, req, false);
	if (err)
		return err;
	while ((nbytes = walk.nbytes) > 0) {
		nbytes &= ~(SM4_BLOCK_SIZE - 1);
		remain = walk.nbytes - nbytes;
		kernel_fpu_begin();
		if (nbytes) {
			cis_sm4_cfb_decrypt_blk8(ctx->rkey_enc,
				walk.src.virt.addr, walk.dst.virt.addr, walk.iv, nbytes);
		}

		if (walk.nbytes == walk.total && remain > 0) {
			cis_sm4_blk(ctx->rkey_enc, walk.iv, keystream);
			crypto_xor_cpy(walk.dst.virt.addr + nbytes,
					   walk.src.virt.addr + nbytes,
					   keystream, remain);
			remain = 0;
		}

		kernel_fpu_end();
		err = skcipher_walk_done(&walk, remain);
		if (err)
			break;
	}
	return err;
}

static int sm4_ofb_do_crypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;
	struct skcipher_walk walk;
	u8 keystream[SM4_BLOCK_SIZE];
	unsigned int nbytes, remain;
	int err;

	err = skcipher_walk_virt(&walk, req, false);
	if (err)
		return err;
	while ((nbytes = walk.nbytes) > 0) {
		nbytes &= ~(SM4_BLOCK_SIZE - 1);
		remain = walk.nbytes - nbytes;
		kernel_fpu_begin();
		if (nbytes) {
			cis_sm4_ofb_crypt_blk8(ctx->rkey_enc,
				walk.src.virt.addr, walk.dst.virt.addr, walk.iv, nbytes);
		}

		if (walk.nbytes == walk.total && remain > 0) {
			cis_sm4_blk(ctx->rkey_enc, walk.iv, keystream);
			crypto_xor_cpy(walk.dst.virt.addr + nbytes,
					   walk.src.virt.addr + nbytes,
					   keystream, remain);
			remain = 0;
		}
		kernel_fpu_end();
		err = skcipher_walk_done(&walk, remain);
		if (err)
			break;
	}
	return err;
}

static int sm4_xts_set_key(struct crypto_skcipher *tfm,
				const u8 *key, unsigned int key_len)
{
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;
	struct sm4_ctx *tweak_ctx = &cis_ctx->tweak_ctx;
	int i = 0;

	if (key_len != 2 * SM4_KEY_SIZE)
		return -EINVAL;

	kernel_fpu_begin();
	cis_sm4_set_key(key, ctx->rkey_enc);
	cis_sm4_set_key(key + SM4_KEY_SIZE, tweak_ctx->rkey_enc);
	kernel_fpu_end();

	for (i = 0; i < SM4_RKEY_WORDS; i++)
		ctx->rkey_dec[i] = ctx->rkey_enc[SM4_RKEY_WORDS - 1 - i];

	for (i = 0; i < SM4_RKEY_WORDS; i++)
		tweak_ctx->rkey_dec[i] = tweak_ctx->rkey_enc[SM4_RKEY_WORDS - 1 - i];

	return 0;
}

static int sm4_xts_encrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;
	struct sm4_ctx *tweak_ctx = &cis_ctx->tweak_ctx;
	struct skcipher_walk walk;
	u8 keystream[SM4_BLOCK_SIZE * 2] = { 0 };
	u8 tweak[SM4_BLOCK_SIZE] = { 0 };
	unsigned int nbytes, remain;
	int err = 0;

	if (req->cryptlen < SM4_BLOCK_SIZE)
		return -EINVAL;

	kernel_fpu_begin();
	cis_sm4_blk(tweak_ctx->rkey_enc, req->iv, tweak);
	kernel_fpu_end();
	err = skcipher_walk_virt(&walk, req, false);
	if (err)
		return err;
	while ((nbytes = walk.nbytes) > 0) {
		nbytes &= ~(SM4_BLOCK_SIZE - 1);
		remain = walk.nbytes - nbytes;
		kernel_fpu_begin();
		if (nbytes) {
			cis_sm4_xts_crypt_blk8(ctx->rkey_enc,
				walk.src.virt.addr, walk.dst.virt.addr, tweak, nbytes);
		}

		if (walk.nbytes == walk.total && remain > 0) {
			if (walk.nbytes >= SM4_BLOCK_SIZE + remain) {
				memcpy(keystream, walk.dst.virt.addr + nbytes - SM4_BLOCK_SIZE,
					SM4_BLOCK_SIZE);
			} else {
				/**
				 * skcipher_walk ensures all previous processing is multiple of
				 * SM4_BLOCK_SIZE, so 'walk.nbytes < SM4_BLOCK_SIZE + remain'
				 * means 'walk.nbytes == reamin', and last encrypted block is
				 * from last walk.
				 */
				scatterwalk_map_and_copy(keystream,
					req->dst, req->cryptlen - remain - SM4_BLOCK_SIZE,
					SM4_BLOCK_SIZE, 0);
			}

			memcpy(keystream + SM4_BLOCK_SIZE, keystream, remain);
			memcpy(keystream, walk.src.virt.addr + nbytes, remain);

			crypto_xor_cpy(keystream, tweak, keystream, SM4_BLOCK_SIZE);
			cis_sm4_blk(ctx->rkey_enc, keystream, keystream);
			crypto_xor_cpy(keystream, tweak, keystream, SM4_BLOCK_SIZE);

			/* copy last SM4_BLOCK_SIZE + remain to dst */
			if (walk.nbytes >= SM4_BLOCK_SIZE + remain) {
				memcpy(walk.dst.virt.addr + walk.nbytes - SM4_BLOCK_SIZE - remain,
					keystream, SM4_BLOCK_SIZE + remain);
			} else {
				scatterwalk_map_and_copy(keystream,
					req->dst, req->cryptlen - remain - SM4_BLOCK_SIZE,
					SM4_BLOCK_SIZE, 1);
				memcpy(walk.dst.virt.addr, keystream + SM4_BLOCK_SIZE,
					walk.nbytes);
			}

			remain = 0;
		}

		kernel_fpu_end();
		err = skcipher_walk_done(&walk, remain);
		if (err)
			break;
	}

	return err;
}

static int sm4_xts_decrypt(struct skcipher_request *req)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct cis_sm4_ctx *cis_ctx = crypto_skcipher_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;
	struct sm4_ctx *tweak_ctx = &cis_ctx->tweak_ctx;
	struct skcipher_walk walk;
	u8 keystream[SM4_BLOCK_SIZE * 2] = { 0 };
	u8 tweak[SM4_BLOCK_SIZE] = { 0 };
	u8 tweakn[SM4_BLOCK_SIZE] = { 0 };
	unsigned int nbytes, remain, ntail, nfinish = 0;
	int err = 0;

	if (req->cryptlen < SM4_BLOCK_SIZE)
		return -EINVAL;

	ntail = req->cryptlen & (SM4_BLOCK_SIZE - 1);

	kernel_fpu_begin();
	cis_sm4_blk(tweak_ctx->rkey_enc, req->iv, tweak);
	kernel_fpu_end();
	err = skcipher_walk_virt(&walk, req, false);
	if (err)
		return err;
	while ((nbytes = walk.nbytes) > 0) {
		nbytes &= ~(SM4_BLOCK_SIZE - 1);
		remain = walk.nbytes - nbytes;

		if (ntail && nbytes && (nfinish + nbytes == req->cryptlen - ntail)) {
			memcpy(keystream, walk.src.virt.addr + nbytes - SM4_BLOCK_SIZE,
				 SM4_BLOCK_SIZE);
			nbytes -= SM4_BLOCK_SIZE;
			nfinish += SM4_BLOCK_SIZE;
		}

		kernel_fpu_begin();
		if (nbytes) {
			cis_sm4_xts_crypt_blk8(ctx->rkey_dec,
				walk.src.virt.addr, walk.dst.virt.addr, tweak, nbytes);
			nfinish += nbytes;
		}

		if (walk.nbytes == walk.total && remain > 0) {
			memcpy(tweakn, tweak, SM4_BLOCK_SIZE);
			gf128_mul_by_2(tweakn);
			crypto_xor_cpy(keystream, tweakn, keystream, SM4_BLOCK_SIZE);
			cis_sm4_blk(ctx->rkey_dec, keystream, keystream);
			crypto_xor_cpy(keystream, tweakn, keystream, SM4_BLOCK_SIZE);

			memcpy(keystream + SM4_BLOCK_SIZE, keystream, remain);

			memcpy(keystream, walk.src.virt.addr + walk.nbytes - remain, remain);

			crypto_xor_cpy(keystream, tweak, keystream, SM4_BLOCK_SIZE);
			cis_sm4_blk(ctx->rkey_dec, keystream, keystream);
			crypto_xor_cpy(keystream, tweak, keystream, SM4_BLOCK_SIZE);

			/* copy last SM4_BLOCK_SIZE + remain to dst */
			if (walk.nbytes >= SM4_BLOCK_SIZE + remain) {
				memcpy(walk.dst.virt.addr + walk.nbytes - SM4_BLOCK_SIZE - remain,
					keystream, SM4_BLOCK_SIZE + remain);
			} else {
				scatterwalk_map_and_copy(keystream, req->dst,
					req->cryptlen - remain - SM4_BLOCK_SIZE,
					SM4_BLOCK_SIZE, 1);
				memcpy(walk.dst.virt.addr, keystream + SM4_BLOCK_SIZE,
					walk.nbytes);
			}

			remain = 0;
		}

		kernel_fpu_end();
		err = skcipher_walk_done(&walk, remain);
		if (err)
			break;
	}

	return err;
}

static struct skcipher_alg cis_skciphers[] = {
	{
		.base = {
			.cra_name		= "__ecb(sm4)",
			.cra_driver_name	= "__ecb-sm4-cis",
			.cra_priority		= 400,
			.cra_flags		= CRYPTO_ALG_INTERNAL,
			.cra_blocksize		= SM4_BLOCK_SIZE,
			.cra_ctxsize		= CRYPTO_SM4_CTX_SIZE,
			.cra_module		= THIS_MODULE,
		},
		.min_keysize	= SM4_KEY_SIZE,
		.max_keysize	= SM4_KEY_SIZE,
		.setkey		= sm4_set_key,
		.encrypt	= sm4_ecb_encrypt,
		.decrypt	= sm4_ecb_decrypt,
	}, {
		.base = {
			.cra_name		= "__cbc(sm4)",
			.cra_driver_name	= "__cbc-sm4-cis",
			.cra_priority		= 400,
			.cra_flags		= CRYPTO_ALG_INTERNAL,
			.cra_blocksize		= SM4_BLOCK_SIZE,
			.cra_ctxsize		= CRYPTO_SM4_CTX_SIZE,
			.cra_module		= THIS_MODULE,
		},
		.min_keysize	= SM4_KEY_SIZE,
		.max_keysize	= SM4_KEY_SIZE,
		.ivsize		= SM4_BLOCK_SIZE,
		.setkey		= sm4_set_key,
		.encrypt	= sm4_cbc_encrypt,
		.decrypt	= sm4_cbc_decrypt,
	}, {
		.base = {
			.cra_name		= "__ctr(sm4)",
			.cra_driver_name	= "__ctr-sm4-cis",
			.cra_priority		= 400,
			.cra_flags		= CRYPTO_ALG_INTERNAL,
			.cra_blocksize		= 1,
			.cra_ctxsize		= CRYPTO_SM4_CTX_SIZE,
			.cra_module		= THIS_MODULE,
		},
		.min_keysize	= SM4_KEY_SIZE,
		.max_keysize	= SM4_KEY_SIZE,
		.ivsize		= SM4_BLOCK_SIZE,
		.chunksize	= SM4_BLOCK_SIZE,
		.setkey		= sm4_set_key,
		.encrypt	= sm4_ctr_do_crypt,
		.decrypt	= sm4_ctr_do_crypt,
	}, {
		.base = {
			.cra_name		= "__cfb(sm4)",
			.cra_driver_name	= "__cfb-sm4-cis",
			.cra_priority		= 400,
			.cra_flags		= CRYPTO_ALG_INTERNAL,
			.cra_blocksize		= 1,
			.cra_ctxsize		= CRYPTO_SM4_CTX_SIZE,
			.cra_module		= THIS_MODULE,
		},
		.min_keysize	= SM4_KEY_SIZE,
		.max_keysize	= SM4_KEY_SIZE,
		.ivsize		= SM4_BLOCK_SIZE,
		.chunksize	= SM4_BLOCK_SIZE,
		.setkey		= sm4_set_key,
		.encrypt	= sm4_cfb_encrypt,
		.decrypt	= sm4_cfb_decrypt,
	}, {
		.base = {
			.cra_name		= "__ofb(sm4)",
			.cra_driver_name	= "__ofb-sm4-cis",
			.cra_priority		= 400,
			.cra_flags		= CRYPTO_ALG_INTERNAL,
			.cra_blocksize		= 1,
			.cra_ctxsize		= CRYPTO_SM4_CTX_SIZE,
			.cra_module		= THIS_MODULE,
		},
		.min_keysize	= SM4_KEY_SIZE,
		.max_keysize	= SM4_KEY_SIZE,
		.ivsize		= SM4_BLOCK_SIZE,
		.chunksize	= SM4_BLOCK_SIZE,
		.setkey		= sm4_set_key,
		.encrypt	= sm4_ofb_do_crypt,
		.decrypt	= sm4_ofb_do_crypt,
	}, {
		.base = {
			.cra_name		= "__gbt17964(xts(sm4))",
			.cra_driver_name	= "__xts-sm4-cis",
			.cra_priority		= 400,
			.cra_flags		= CRYPTO_ALG_INTERNAL,
			.cra_blocksize		= 1,
			.cra_ctxsize		= CRYPTO_SM4_CTX_SIZE,
			.cra_module		= THIS_MODULE,
		},
		.min_keysize	= 2 * SM4_KEY_SIZE,
		.max_keysize	= 2 * SM4_KEY_SIZE,
		.ivsize		= SM4_BLOCK_SIZE,
		.chunksize	= SM4_BLOCK_SIZE,
		.setkey		= sm4_xts_set_key,
		.encrypt	= sm4_xts_encrypt,
		.decrypt	= sm4_xts_decrypt,
	}
};

static struct simd_skcipher_alg *cis_simd_skciphers[ARRAY_SIZE(cis_skciphers)];

static int sm4_gcm_init(struct crypto_aead *tfm)
{
	struct cis_sm4_ctx *cis_ctx = crypto_aead_ctx(tfm);
	struct crypto_shash *ghash = NULL;
	struct shash_desc *sdesc = NULL;

	cis_ctx->ghash_tfm = NULL;
	cis_ctx->ghash_desc = NULL;

	ghash = crypto_alloc_shash("__ghash-pclmulqdqni",
		CRYPTO_ALG_INTERNAL, 0);
	if (IS_ERR(ghash)) {
		pr_warn("load ghash-pclmulqdqni failed\n");
		return PTR_ERR(ghash);
	}
	sdesc = kzalloc(sizeof(struct shash_desc) +
				   crypto_shash_descsize(ghash), GFP_KERNEL);
	if (unlikely(!sdesc)) {
		crypto_free_shash(ghash);
		return -ENOMEM;
	}

	sdesc->tfm = ghash;
	cis_ctx->ghash_tfm = ghash;
	cis_ctx->ghash_desc = sdesc;

	return 0;
}

static void sm4_gcm_exit(struct crypto_aead *tfm)
{
	struct cis_sm4_ctx *cis_ctx = crypto_aead_ctx(tfm);

	if (cis_ctx->ghash_tfm)
		crypto_free_shash(cis_ctx->ghash_tfm);

	kfree(cis_ctx->ghash_desc);
}

static int sm4_gcm_set_key(struct crypto_aead *tfm,
				const u8 *key, unsigned int key_len)
{
	struct cis_sm4_ctx *cis_ctx = crypto_aead_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;
	int i = 0;

	if (key_len != SM4_KEY_SIZE)
		return -EINVAL;

	kernel_fpu_begin();
	cis_sm4_set_key(key, ctx->rkey_enc);
	kernel_fpu_end();
	for (i = 0; i < SM4_RKEY_WORDS; i++)
		ctx->rkey_dec[i] = ctx->rkey_enc[SM4_RKEY_WORDS - 1 - i];

	return 0;
}

static int sm4_gcm_set_authsize(struct crypto_aead *tfm,
					   unsigned int authsize)
{
	switch (authsize) {
	case 4:
	case 8:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sm4_gcm_do_crypt(struct aead_request *req, bool encrypt)
{
	struct crypto_aead *tfm = crypto_aead_reqtfm(req);
	struct cis_sm4_ctx *cis_ctx = crypto_aead_ctx(tfm);
	struct sm4_ctx *ctx = &cis_ctx->ctx;
	struct scatter_walk assoc_sg_walk;
	struct skcipher_walk walk;
	unsigned long auth_tag_len = crypto_aead_authsize(tfm);
	be128 tail = {cpu_to_be64(req->assoclen * 8), 0};
	u8 iv[SM4_BLOCK_SIZE] = {0};
	u8 ghash[GHASH_BLOCK_SIZE] = {0};
	u8 H[GHASH_BLOCK_SIZE] = {0};
	u8 keystream[SM4_BLOCK_SIZE] = {0};
	u8 auth_tag[SM4_BLOCK_SIZE] = {0};
	u8 auth_tag_msg[SM4_BLOCK_SIZE];
	u8 pad[GHASH_BLOCK_SIZE] = {0};
	unsigned int nbytes, remain, zfilled;
	u8 *assoc = NULL, *assocmem = NULL;
	gfp_t flags;
	int err = 0;

	if (!cis_ctx->ghash_tfm || !cis_ctx->ghash_desc)
		return -EPERM;

	tail.b = encrypt ? cpu_to_be64(req->cryptlen * 8) :
			cpu_to_be64((req->cryptlen - auth_tag_len) * 8);

	memcpy(iv, req->iv, 12);
	iv[SM4_BLOCK_SIZE - 1] = 1;
	memcpy(auth_tag, iv, SM4_BLOCK_SIZE);

	kernel_fpu_begin();
	cis_sm4_blk(ctx->rkey_enc, auth_tag, auth_tag);
	cis_sm4_blk(ctx->rkey_enc, H, H);
	kernel_fpu_end();

	crypto_shash_setkey(cis_ctx->ghash_tfm, H, sizeof(H));
	crypto_shash_init(cis_ctx->ghash_desc);

	if (req->assoclen > 0) {
		if (req->src->length >= req->assoclen) {
			scatterwalk_start(&assoc_sg_walk, req->src);
			assoc = scatterwalk_map(&assoc_sg_walk);
		} else {
			flags = (req->base.flags & CRYPTO_TFM_REQ_MAY_SLEEP) ?
				GFP_KERNEL : GFP_ATOMIC;

			/* assoc can be any length, so must be on heap */
			assocmem = kmalloc(req->assoclen, flags);
			if (unlikely(!assocmem))
				return -ENOMEM;
			assoc = assocmem;

			scatterwalk_map_and_copy(assoc, req->src, 0, req->assoclen, 0);
		}

		zfilled = ALIGN(req->assoclen, GHASH_BLOCK_SIZE) - req->assoclen;
		crypto_shash_update(cis_ctx->ghash_desc, assoc, req->assoclen);
		if (zfilled)
			crypto_shash_update(cis_ctx->ghash_desc, pad, zfilled);

	}

	if (!assocmem)
		scatterwalk_unmap(assoc);
	else
		kfree(assocmem);

	err = encrypt ? skcipher_walk_aead_encrypt(&walk, req, false)
		  : skcipher_walk_aead_decrypt(&walk, req, false);
	if (err)
		return err;

	crypto_inc(iv, SM4_BLOCK_SIZE);
	while ((nbytes = walk.nbytes) > 0) {
		nbytes &= ~(SM4_BLOCK_SIZE - 1);
		remain = walk.nbytes - nbytes;
		if (nbytes) {
			if (encrypt) {
				kernel_fpu_begin();
				cis_sm4_ctr_crypt_blk8(ctx->rkey_enc,
					walk.src.virt.addr, walk.dst.virt.addr, iv, nbytes);
				kernel_fpu_end();

				crypto_shash_update(cis_ctx->ghash_desc,
					walk.dst.virt.addr, nbytes);
			} else {
				crypto_shash_update(cis_ctx->ghash_desc,
					walk.src.virt.addr, nbytes);

				kernel_fpu_begin();
				cis_sm4_ctr_crypt_blk8(ctx->rkey_enc,
					walk.src.virt.addr, walk.dst.virt.addr, iv, nbytes);
				kernel_fpu_end();
			}
		}

		if (walk.nbytes == walk.total && remain > 0) {
			kernel_fpu_begin();
			cis_sm4_blk(ctx->rkey_enc, iv, keystream);
			kernel_fpu_end();
			if (encrypt) {
				crypto_xor_cpy(walk.dst.virt.addr + nbytes,
					   walk.src.virt.addr + nbytes,
					   keystream, remain);

				memcpy(pad, walk.dst.virt.addr + nbytes, remain);
				crypto_shash_update(cis_ctx->ghash_desc,
					pad, GHASH_BLOCK_SIZE);
			} else {
				memcpy(pad, walk.src.virt.addr + nbytes, remain);
				crypto_shash_update(cis_ctx->ghash_desc,
					pad, GHASH_BLOCK_SIZE);

				crypto_xor_cpy(walk.dst.virt.addr + nbytes,
					   walk.src.virt.addr + nbytes,
					   keystream, remain);
			}

			crypto_inc(iv, SM4_BLOCK_SIZE);
			remain = 0;
		}


		err = skcipher_walk_done(&walk, remain);
		if (err)
			break;
	}
	crypto_shash_update(cis_ctx->ghash_desc, (const u8 *)&tail, sizeof(tail));
	crypto_shash_final(cis_ctx->ghash_desc, ghash);

	crypto_xor(auth_tag, ghash, SM4_BLOCK_SIZE);
	if (encrypt) {
		scatterwalk_map_and_copy(auth_tag, req->dst,
			req->assoclen + req->cryptlen,
			auth_tag_len, 1);
	} else {
		scatterwalk_map_and_copy(auth_tag_msg, req->src,
			req->assoclen + req->cryptlen - auth_tag_len,
			auth_tag_len, 0);
		if (crypto_memneq(auth_tag_msg, auth_tag, auth_tag_len)) {
			memzero_explicit(auth_tag, sizeof(auth_tag));
			err = -EBADMSG;
		}
	}

	return err;
}

static int sm4_gcm_encrypt(struct aead_request *req)
{
	return sm4_gcm_do_crypt(req, 1);
}


static int sm4_gcm_decrypt(struct aead_request *req)
{
	return sm4_gcm_do_crypt(req, 0);
}

static struct aead_alg cis_aeads[] = {
	{
		.init			= sm4_gcm_init,
		.exit			= sm4_gcm_exit,
		.setkey			= sm4_gcm_set_key,
		.setauthsize	= sm4_gcm_set_authsize,
		.encrypt		= sm4_gcm_encrypt,
		.decrypt		= sm4_gcm_decrypt,
		.ivsize			= GCM_SM4_IV_SIZE,
		.chunksize		= SM4_BLOCK_SIZE,
		.maxauthsize		= 16,
		.base = {
			.cra_name		= "__gcm(sm4)",
			.cra_driver_name	= "__gcm-sm4-cis",
			.cra_priority		= 400,
			.cra_flags		= CRYPTO_ALG_INTERNAL,
			.cra_blocksize		= 1,
			.cra_ctxsize		= CRYPTO_SM4_CTX_SIZE,
			.cra_alignmask		= 0,
			.cra_module		= THIS_MODULE,
		},
	}
};

static struct simd_aead_alg *cis_simd_aeads[ARRAY_SIZE(cis_aeads)];

static int __init cis_sm4_init(void)
{
	int err = 0;
#ifdef CONFIG_X86_64
	if (!boot_cpu_has(X86_FEATURE_HYGON_SM4)) {
		pr_err("CIS SM4 Not Support");
		return -ENODEV;
	}
#endif /* CONFIG_X86_64 */

	err = simd_register_skciphers_compat(cis_skciphers,
						 ARRAY_SIZE(cis_skciphers),
						 cis_simd_skciphers);
	if (err)
		return err;

	err = simd_register_aeads_compat(cis_aeads, ARRAY_SIZE(cis_aeads),
					 cis_simd_aeads);
	if (err)
		goto unregister_skciphers;

	return 0;

unregister_skciphers:
	simd_unregister_skciphers(cis_skciphers, ARRAY_SIZE(cis_skciphers),
				  cis_simd_skciphers);

	return err;
}

static void __exit cis_sm4_exit(void)
{
	simd_unregister_aeads(cis_aeads, ARRAY_SIZE(cis_aeads),
				  cis_simd_aeads);
	simd_unregister_skciphers(cis_skciphers, ARRAY_SIZE(cis_skciphers),
				  cis_simd_skciphers);
}

late_initcall(cis_sm4_init);
module_exit(cis_sm4_exit);

MODULE_DESCRIPTION("SM4 Cipher Algorithm, Hygon CIS optimized");
MODULE_LICENSE("GPL");
MODULE_ALIAS_CRYPTO("sm4");

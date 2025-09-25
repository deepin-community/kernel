// SPDX-License-Identifier: GPL-2.0
/*
 * zhaoxin-gmi-sm4.c - wrapper code for Zhaoxin GMI.
 *
 * Copyright (C) 2023 Shanghai Zhaoxin Semiconductor LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/module.h>
#include <linux/err.h>
#include <crypto/cryptd.h>
#include <crypto/scatterwalk.h>
#include <crypto/algapi.h>
#include <crypto/internal/simd.h>
#include <crypto/internal/skcipher.h>
#include <linux/workqueue.h>
#include <crypto/sm4.h>
#include <asm/unaligned.h>
#include <linux/processor.h>
#include <linux/cpufeature.h>
#include <asm/cpu_device_id.h>

#define SM4_ECB (1 << 6)
#define SM4_CBC (1 << 7)
#define SM4_CFB (1 << 8)
#define SM4_OFB (1 << 9)
#define SM4_CTR (1 << 10)

#define ZX_GMI_ALIGNMENT 16

#define GETU16(p) ((u16)(p)[0] << 8 | (u16)(p)[1])

#define GMI_SM4_CRA_PRIORITY 600

/* Control word. */
struct sm4_cipher_data {
	u8 iv[SM4_BLOCK_SIZE]; /* Initialization vector */
	union {
		u32 pad;
		struct {
			u32 encdec:1;
			u32 func:5;
			u32 mode:5;
			u32 digest:1;
		} b;
	} cword; /* Control word */
	struct sm4_ctx keys; /* Encryption key */
};

static u8 *rep_xcrypt(const u8 *input, u8 *output, void *key, u8 *iv,
		      struct sm4_cipher_data *sm4_data, size_t count)
{
	size_t pad = sm4_data->cword.pad;

	/* Set the flag for encryption or decryption */
	if (sm4_data->cword.b.encdec == 1)
		pad &= ~0x01;
	else
		pad |= 0x01;

	__asm__ __volatile__(".byte 0xf3, 0x0f, 0xa7, 0xf0\n"
			     : "+S"(input), "+D"(output), "+c"(count)
			     : "a"(pad), "b"(key), "d"(iv)
			     : "memory");

	return iv;
}

static u8 *rep_xcrypt_ctr(const u8 *input, u8 *output, void *key, u8 *iv,
			  struct sm4_cipher_data *sm4_data, size_t count)
{
	u8 oiv[SM4_BLOCK_SIZE] = { 0 };
	u16 cnt_tmp;
	u32 i;
	u8 *in_tmp = (u8 *)input, *out_tmp = output;

	/* Backup the original IV if it is not NULL. */
	if (iv)
		memcpy(oiv, iv, SM4_BLOCK_SIZE);

	/* Get the current counter. */
	cnt_tmp = GETU16(&iv[14]);

	/* Get the available counter space before overflow. */
	cnt_tmp = 0x10000 - cnt_tmp;

	/* Check there is enough counter space for the required blocks. */
	if (cnt_tmp < count) {
		/* Process the first part of data blocks. */
		rep_xcrypt(in_tmp, out_tmp, key, iv, sm4_data, cnt_tmp);
		/* Only increase the counter by SW when overflow occurs. */
		memcpy(iv, oiv, SM4_BLOCK_SIZE);

		for (i = 0; i < cnt_tmp; i++)
			crypto_inc(iv, SM4_BLOCK_SIZE);

		out_tmp = output + cnt_tmp * SM4_BLOCK_SIZE;
		in_tmp = (u8 *)(input + cnt_tmp * SM4_BLOCK_SIZE);

		/* Get the number of data blocks that have not been encrypted. */
		cnt_tmp = count - cnt_tmp;
		/* Process the remaining part of data blocks. */
		rep_xcrypt(in_tmp, out_tmp, key, iv, sm4_data, cnt_tmp);
	} else {
		/* Counter space is big enough, the counter will not overflow. */
		rep_xcrypt(in_tmp, out_tmp, key, iv, sm4_data, count);
	}

	/* Restore the iv if not null */
	if (iv)
		memcpy(iv, oiv, SM4_BLOCK_SIZE);

	return iv;
}

static u8 *rep_xcrypt_ecb_one(const u8 *input, u8 *output, void *key, u8 *iv)
{
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 1;
	cw.cword.pad |= 0x20 | SM4_ECB;

	return rep_xcrypt(input, output, key, iv, &cw, 1);
}

/*
 * gmi_sm4_set_key - Set the sm4 key.
 * @tfm:  The %crypto_skcipher that is used in the context.
 * @in_key: The input key.
 * @key_len:The size of the key.
 */
static int gmi_sm4_set_key(struct crypto_skcipher *tfm, const u8 *in_key, unsigned int key_len)
{
	struct sm4_ctx *ctx = crypto_skcipher_ctx(tfm);

	if (key_len != SM4_KEY_SIZE) {
		pr_warn("The key_len must be 16 bytes. please check\n");
		return -EINVAL;
	}

	memcpy(ctx->rkey_enc, in_key, key_len);
	memcpy(ctx->rkey_dec, in_key, key_len);

	return 0;
}

static int sm4_cipher_common(struct skcipher_request *req, struct sm4_cipher_data *cw)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct sm4_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct skcipher_walk walk;
	unsigned int blocks;
	int err;

	err = skcipher_walk_virt(&walk, req, true);

	while ((blocks = (walk.nbytes / SM4_BLOCK_SIZE))) {
		rep_xcrypt(walk.src.virt.addr, walk.dst.virt.addr, ctx->rkey_enc, walk.iv, cw,
			   blocks);

		err = skcipher_walk_done(&walk, walk.nbytes % SM4_BLOCK_SIZE);
	}

	return err;
}

static int ecb_encrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 1;
	cw.cword.pad |= 0x20 | SM4_ECB;

	err = sm4_cipher_common(req, &cw);

	return err;
}

static int ecb_decrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.pad |= 0x20 | SM4_ECB;

	err = sm4_cipher_common(req, &cw);

	return err;
}

static int cbc_encrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 1;
	cw.cword.pad |= 0x20 | SM4_CBC;

	err = sm4_cipher_common(req, &cw);

	return err;
}

static int cbc_decrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.pad |= 0x20 | SM4_CBC;

	err = sm4_cipher_common(req, &cw);

	return err;
}

/*
 * sm4_cipher_ctr is used for ZX-E and newer
 */
static int sm4_cipher_ctr(struct skcipher_request *req, struct sm4_cipher_data *cw)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct sm4_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct skcipher_walk walk;
	unsigned int blocks, nbytes;
	int err;
	u8 *dst, *src;
	u8 keystream[SM4_BLOCK_SIZE];
	u32 i;

	err = skcipher_walk_virt(&walk, req, true);

	while ((nbytes = walk.nbytes) > 0) {
		src = walk.src.virt.addr;
		dst = walk.dst.virt.addr;

		while (nbytes >= SM4_BLOCK_SIZE) {
			blocks = nbytes / SM4_BLOCK_SIZE;
			rep_xcrypt_ctr(walk.src.virt.addr, walk.dst.virt.addr, ctx->rkey_enc,
				       walk.iv, cw, blocks);

			for (i = 0; i < blocks; i++)
				crypto_inc(walk.iv, SM4_BLOCK_SIZE);

			dst += blocks * SM4_BLOCK_SIZE;
			src += blocks * SM4_BLOCK_SIZE;
			nbytes -= blocks * SM4_BLOCK_SIZE;
		}

		if (walk.nbytes == walk.total && nbytes > 0) {
			rep_xcrypt_ecb_one(walk.iv, keystream, ctx->rkey_enc, walk.iv);
			crypto_inc(walk.iv, SM4_BLOCK_SIZE);
			crypto_xor_cpy(dst, keystream, src, nbytes);
			dst += nbytes;
			src += nbytes;
			nbytes = 0;
		}

		err = skcipher_walk_done(&walk, nbytes);
	}

	return err;
}

/*
 * ctr_encrypt is used for ZX-E and newer
 */
static int ctr_encrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 1;
	cw.cword.pad |= 0x20 | SM4_CTR;

	err = sm4_cipher_ctr(req, &cw);

	return err;
}

/*
 * ctr_decrypt is used for ZX-E and newer
 */
static int ctr_decrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.pad |= 0x20 | SM4_CTR;

	err = sm4_cipher_ctr(req, &cw);

	return err;
}

/*
 * sm4_cipher_ofb is used for ZX-E and newer
 */
static int sm4_cipher_ofb(struct skcipher_request *req, struct sm4_cipher_data *cw)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct sm4_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct skcipher_walk walk;
	unsigned int blocks, nbytes;
	int err;
	u8 *dst, *src;

	err = skcipher_walk_virt(&walk, req, true);

	while ((nbytes = walk.nbytes) > 0) {
		src = walk.src.virt.addr;
		dst = walk.dst.virt.addr;

		while (nbytes >= SM4_BLOCK_SIZE) {
			blocks = nbytes / SM4_BLOCK_SIZE;
			rep_xcrypt(walk.src.virt.addr, walk.dst.virt.addr, ctx->rkey_enc, walk.iv,
				   cw, blocks);

			dst += blocks * SM4_BLOCK_SIZE;
			src += blocks * SM4_BLOCK_SIZE;
			nbytes -= blocks * SM4_BLOCK_SIZE;
		}

		if (walk.nbytes == walk.total && nbytes > 0) {
			rep_xcrypt_ecb_one(walk.iv, walk.iv, ctx->rkey_enc, walk.iv);
			crypto_xor_cpy(dst, src, walk.iv, nbytes);
			dst += nbytes;
			src += nbytes;
			nbytes = 0;
		}

		err = skcipher_walk_done(&walk, nbytes);
	}

	return err;
}

/*
 * ofb_encrypt is used for ZX-E and newer
 */
static int ofb_encrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 1;
	cw.cword.pad |= 0x20 | SM4_OFB;

	err = sm4_cipher_ofb(req, &cw);

	return err;
}

/*
 * ofb_decrypt is used for ZX-E and newer
 */
static int ofb_decrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.pad |= 0x20 | SM4_OFB;

	err = sm4_cipher_ofb(req, &cw);

	return err;
}

/*
 * sm4_cipher_cfb is used for ZX-E and newer
 */
static int sm4_cipher_cfb(struct skcipher_request *req, struct sm4_cipher_data *cw)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct sm4_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct skcipher_walk walk;
	unsigned int blocks, nbytes;
	int err;
	u8 *dst, *src;

	err = skcipher_walk_virt(&walk, req, true);

	while ((nbytes = walk.nbytes) > 0) {
		src = walk.src.virt.addr;
		dst = walk.dst.virt.addr;

		while (nbytes >= SM4_BLOCK_SIZE) {
			blocks = nbytes / SM4_BLOCK_SIZE;
			rep_xcrypt(walk.src.virt.addr, walk.dst.virt.addr, ctx->rkey_enc, walk.iv,
				   cw, blocks);

			dst += blocks * SM4_BLOCK_SIZE;
			src += blocks * SM4_BLOCK_SIZE;
			nbytes -= blocks * SM4_BLOCK_SIZE;
		}

		if (walk.nbytes == walk.total && nbytes > 0) {
			u8 keystream[SM4_BLOCK_SIZE];

			if (cw->cword.b.encdec) {
				rep_xcrypt_ecb_one(walk.iv, walk.iv, ctx->rkey_enc, walk.iv);
				crypto_xor_cpy(keystream, walk.iv, src, nbytes);
				memcpy(dst, keystream, nbytes);
			} else {
				rep_xcrypt_ecb_one(walk.iv, walk.iv, ctx->rkey_enc, walk.iv);
				crypto_xor_cpy(dst, src, walk.iv, nbytes);
				memcpy(walk.iv, src, nbytes);
			}

			dst += nbytes;
			src += nbytes;
			nbytes = 0;
		}

		err = skcipher_walk_done(&walk, nbytes);
	}

	return err;
}

/*
* cfb_encrypt is used for ZX-E and newer.
*/
static int cfb_encrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 1;
	cw.cword.pad |= 0x20 | SM4_CFB;

	err = sm4_cipher_cfb(req, &cw);

	return err;
}

/*
* cfb_decrypt is used for ZX-E and newer.
*/

static int cfb_decrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.pad |= 0x20 | SM4_CFB;

	err = sm4_cipher_cfb(req, &cw);

	return err;
}

static struct skcipher_alg sm4_algs[] = {
	{
		.base = {
			.cra_name = "__ecb(sm4)",
			.cra_driver_name = "__ecb-sm4-gmi",
			.cra_priority = GMI_SM4_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_INTERNAL,
			.cra_blocksize = SM4_BLOCK_SIZE,
			.cra_ctxsize = sizeof(struct sm4_ctx),
			.cra_module = THIS_MODULE,
		},
		.min_keysize = SM4_KEY_SIZE,
		.max_keysize = SM4_KEY_SIZE,
		.walksize = 8 * SM4_BLOCK_SIZE,
		.setkey = gmi_sm4_set_key,
		.encrypt = ecb_encrypt,
		.decrypt = ecb_decrypt,
	},

	{
		.base = {
			.cra_name = "__cbc(sm4)",
			.cra_driver_name = "__cbc-sm4-gmi",
			.cra_priority = GMI_SM4_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_INTERNAL,
			.cra_blocksize = SM4_BLOCK_SIZE,
			.cra_ctxsize = sizeof(struct sm4_ctx),
			.cra_module = THIS_MODULE,
		},
		.min_keysize = SM4_KEY_SIZE,
		.max_keysize = SM4_KEY_SIZE,
		.ivsize = SM4_BLOCK_SIZE,
		.walksize = 8 * SM4_BLOCK_SIZE,
		.setkey = gmi_sm4_set_key,
		.encrypt = cbc_encrypt,
		.decrypt = cbc_decrypt,
	},

	{
		.base = {
			.cra_name = "__ctr(sm4)",
			.cra_driver_name = "__ctr-sm4-gmi",
			.cra_priority = GMI_SM4_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_INTERNAL,
			.cra_blocksize = 1, //SM4_BLOCK_SIZE,
			.cra_ctxsize = sizeof(struct sm4_ctx),
			.cra_module = THIS_MODULE,
		},
		.min_keysize = SM4_KEY_SIZE,
		.max_keysize = SM4_KEY_SIZE,
		.ivsize = SM4_BLOCK_SIZE,
		.chunksize = SM4_BLOCK_SIZE,
		.walksize = 8 * SM4_BLOCK_SIZE,
		.setkey = gmi_sm4_set_key,
		.encrypt = ctr_encrypt,
		.decrypt = ctr_decrypt,
	},

	{
		.base = {
			.cra_name = "__ofb(sm4)",
			.cra_driver_name = "__ofb-sm4-gmi",
			.cra_priority = GMI_SM4_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_INTERNAL,
			.cra_blocksize = 1,
			.cra_ctxsize = sizeof(struct sm4_ctx),
			.cra_module = THIS_MODULE,
		},
		.min_keysize = SM4_KEY_SIZE,
		.max_keysize = SM4_KEY_SIZE,
		.ivsize = SM4_BLOCK_SIZE,
		.chunksize = SM4_BLOCK_SIZE,
		.walksize = 8 * SM4_BLOCK_SIZE,
		.setkey = gmi_sm4_set_key,
		.encrypt = ofb_encrypt,
		.decrypt = ofb_decrypt,
	},

	{
		.base = {
			.cra_name = "__cfb(sm4)",
			.cra_driver_name = "__cfb-sm4-gmi",
			.cra_priority = GMI_SM4_CRA_PRIORITY,
			.cra_flags = CRYPTO_ALG_INTERNAL,
			.cra_blocksize = 1,
			.cra_ctxsize = sizeof(struct sm4_ctx),
			.cra_module = THIS_MODULE,
		},
		.min_keysize = SM4_KEY_SIZE,
		.max_keysize = SM4_KEY_SIZE,
		.ivsize = SM4_BLOCK_SIZE,
		.chunksize = SM4_BLOCK_SIZE,
		.walksize = 8 * SM4_BLOCK_SIZE,
		.setkey = gmi_sm4_set_key,
		.encrypt = cfb_encrypt,
		.decrypt = cfb_decrypt,
	}
};

static struct simd_skcipher_alg *sm4_simd_algs[ARRAY_SIZE(sm4_algs)];

static void gmi_sm4_exit(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sm4_simd_algs) && sm4_simd_algs[i]; i++)
		simd_skcipher_free(sm4_simd_algs[i]);

	crypto_unregister_skciphers(sm4_algs, ARRAY_SIZE(sm4_algs));
}

static const struct x86_cpu_id zhaoxin_ccs_cpu_ids[] = {
	X86_MATCH_VENDOR_FAM_FEATURE(ZHAOXIN, 7, X86_FEATURE_CCS, NULL),
	X86_MATCH_VENDOR_FAM_FEATURE(CENTAUR, 7, X86_FEATURE_CCS, NULL),
	{}
};
MODULE_DEVICE_TABLE(x86cpu, zhaoxin_ccs_cpu_ids);

static int __init gmi_sm4_init(void)
{
	struct simd_skcipher_alg *simd;
	const char *basename;
	const char *algname;
	const char *drvname;
	int err;
	int i;

	if (!x86_match_cpu(zhaoxin_ccs_cpu_ids) || !boot_cpu_has(X86_FEATURE_CCS_EN))
		return -ENODEV;

	err = crypto_register_skciphers(sm4_algs, ARRAY_SIZE(sm4_algs));
	if (err)
		return err;

	for (i = 0; i < ARRAY_SIZE(sm4_algs); i++) {
		algname = sm4_algs[i].base.cra_name + 2;
		drvname = sm4_algs[i].base.cra_driver_name + 2;
		basename = sm4_algs[i].base.cra_driver_name;
		simd = simd_skcipher_create_compat(sm4_algs + i, algname, drvname, basename);
		err = PTR_ERR(simd);
		if (IS_ERR(simd))
			goto unregister_simds;

		sm4_simd_algs[i] = simd;
	}

	return 0;

unregister_simds:
	gmi_sm4_exit();
	return err;
}

late_initcall(gmi_sm4_init);
module_exit(gmi_sm4_exit);

MODULE_DESCRIPTION("SM4-ECB/CBC/CTR/CFB/OFB using Zhaoxin GMI");
MODULE_AUTHOR("GRX");
MODULE_LICENSE("GPL");
MODULE_VERSION("2.0.1");

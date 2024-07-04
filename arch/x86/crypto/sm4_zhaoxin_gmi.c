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

#define SM4_ECB  (1<<6)
#define SM4_CBC  (1<<7)
#define SM4_CFB  (1<<8)
#define SM4_OFB  (1<<9)
#define SM4_CTR  (1<<10)

#define ZX_GMI_ALIGNMENT 16

#define GETU16(p)  ((u16)(p)[0]<<8 | (u16)(p)[1])

/* Control word. */
struct sm4_cipher_data {
	u8 iv[SM4_BLOCK_SIZE];	/* Initialization vector */
	union {
		u32 pad;
		struct {
			u32 encdec:1;
			u32 func:5;
			u32 mode:5;
			u32 digest:1;
		} b;
	} cword;		/* Control word */
	struct sm4_ctx keys;	/* Encryption key */
};

static u8 *rep_xcrypt(const u8 *input, u8 *output, void *key, u8 *iv,
		      struct sm4_cipher_data *sm4_data, u64 count)
{
	unsigned long rax = sm4_data->cword.pad;

	/* Set the flag for encryption or decryption */
	if (sm4_data->cword.b.encdec == 1)
		rax &= ~0x01;
	else
		rax |= 0x01;

	__asm__ __volatile__(
		".byte 0xf3,0x0f,0xa7,0xf0\n"
		:
		: "S"(input), "D"(output), "a"(rax), "b"(key), "c"((unsigned long)count), "d"(iv));
	return iv;
}

static u8 *rep_xcrypt_ctr(const u8 *input, u8 *output, void *key, u8 *iv,
			  struct sm4_cipher_data *sm4_data, u64 count)
{
	u8 oiv[SM4_BLOCK_SIZE] = {0};
	u16 cnt_tmp;
	u32 i;
	u8 *in_tmp = (u8 *)input, *out_tmp = output;

	/* Backup the original IV if it is not NULL. */
	if (iv)
		memcpy(oiv,  iv, SM4_BLOCK_SIZE);

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

static u8 *rep_xcrypt_ecb_ONE(const u8 *input, u8 *output, void *key, u8 *iv,
			      struct sm4_cipher_data *sm4_data, u64 count)
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
static int gmi_sm4_set_key(struct crypto_skcipher  *tfm, const u8 *in_key, unsigned int key_len)
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
			blocks = nbytes/SM4_BLOCK_SIZE;
			rep_xcrypt_ctr(walk.src.virt.addr, walk.dst.virt.addr, ctx->rkey_enc, walk.iv, cw, blocks);

			for (i = 0; i < blocks; i++)
				crypto_inc(walk.iv, SM4_BLOCK_SIZE);

			dst += blocks * SM4_BLOCK_SIZE;
			src += blocks * SM4_BLOCK_SIZE;
			nbytes -= blocks * SM4_BLOCK_SIZE;
		}

		if (walk.nbytes == walk.total && nbytes > 0) {
			rep_xcrypt_ecb_ONE(walk.iv, keystream, ctx->rkey_enc, walk.iv, cw, 1);
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
 *  sm4_ctr_zxc is used for ZXC+
 */
static int sm4_ctr_zxc(struct skcipher_request *req, struct sm4_cipher_data *cw)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct sm4_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct skcipher_walk walk;
	unsigned int nbytes;
	int err;
	u8 *dst, *src;
	u8 en_iv[SM4_BLOCK_SIZE] = {0};

	err = skcipher_walk_virt(&walk, req, true);

	while ((nbytes = walk.nbytes) > 0) {
		src = walk.src.virt.addr;
		dst = walk.dst.virt.addr;

		while (nbytes >= SM4_BLOCK_SIZE) {
			rep_xcrypt_ecb_ONE(walk.iv, en_iv, ctx->rkey_enc, walk.iv, cw, 1);
			crypto_inc(walk.iv, SM4_BLOCK_SIZE);

			crypto_xor_cpy(dst, en_iv, src, SM4_BLOCK_SIZE);

			dst += SM4_BLOCK_SIZE;
			src += SM4_BLOCK_SIZE;
			nbytes -= SM4_BLOCK_SIZE;
		}

		if (walk.nbytes == walk.total && nbytes > 0) {

			rep_xcrypt_ecb_ONE(walk.iv, en_iv, ctx->rkey_enc, walk.iv, cw, 1);
			crypto_xor_cpy(dst, en_iv, src, nbytes);

			dst += nbytes;
			src += nbytes;
			nbytes = 0;
		}

		err = skcipher_walk_done(&walk, nbytes);
	}

	return err;
}

/*
 * ctr_encrypt_zxc is used for ZX-C+
 */
static int ctr_encrypt_zxc(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 1;
	cw.cword.pad |= 0x20 | SM4_CTR;

	err = sm4_ctr_zxc(req, &cw);

	return err;
}

/*
 * ctr_decrypt_zxc is used for ZX-C+
 */
static int ctr_decrypt_zxc(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 0;
	cw.cword.pad |= 0x20 | SM4_CTR;

	err = sm4_ctr_zxc(req, &cw);

	return err;
}

/*
 *  ofb_encrypt is used for ZX-E and newer
 */
static int ofb_encrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 1;
	cw.cword.pad |= 0x20 | SM4_OFB;

	err = sm4_cipher_common(req, &cw);

	return err;
}

/*
 *  ofb_decrypt is used for ZX-E and newer
 */
static int ofb_decrypt(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.pad |= 0x20 | SM4_OFB;

	err = sm4_cipher_common(req, &cw);

	return err;
}

/*
 * sm4_ofb_zxc is used for ZX-C+
 */
static int sm4_ofb_zxc(struct skcipher_request *req, struct sm4_cipher_data *cw)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct sm4_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct skcipher_walk walk;
	unsigned int blocks;
	int err = skcipher_walk_virt(&walk, req, true);
	u32 n;

	while ((blocks = (walk.nbytes / SM4_BLOCK_SIZE))) {
		while (blocks--) {
			rep_xcrypt_ecb_ONE(walk.iv, walk.iv, ctx->rkey_enc, NULL, cw, 1);

			for (n = 0; n < SM4_BLOCK_SIZE; n += sizeof(size_t))
				*(size_t *)(walk.dst.virt.addr + n) =
					*(size_t *)(walk.iv + n) ^
					*(size_t *)(walk.src.virt.addr + n);

			walk.src.virt.addr += SM4_BLOCK_SIZE;
			walk.dst.virt.addr += SM4_BLOCK_SIZE;
		}
		err = skcipher_walk_done(&walk, walk.nbytes % SM4_BLOCK_SIZE);
	}
	return err;
}

/*
 *  ofb_encrypt_zxc is used for ZX-C+
 */
static int ofb_encrypt_zxc(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 1;
	cw.cword.pad |= 0x20 | SM4_OFB;

	err = sm4_ofb_zxc(req, &cw);

	return err;
}

/*
 * ofb_decrypt_zxc is used for ZX-C+
 */
static int ofb_decrypt_zxc(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 0;
	cw.cword.pad |= 0x20 | SM4_OFB;

	err = sm4_ofb_zxc(req, &cw);

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

	err = sm4_cipher_common(req, &cw);

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

	err = sm4_cipher_common(req, &cw);

	return err;
}

/*
 * sm4_cfb_zxc is used for ZX-C+
 */
static int sm4_cfb_zxc(struct skcipher_request *req, struct sm4_cipher_data *cw)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct sm4_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct skcipher_walk walk;
	unsigned int blocks;
	int err;
	u32 n;
	size_t t;

	err = skcipher_walk_virt(&walk, req, true);

	while ((blocks = (walk.nbytes / SM4_BLOCK_SIZE))) {
		while (blocks--) {
			rep_xcrypt_ecb_ONE(walk.iv, walk.iv, ctx->rkey_enc, NULL, cw, 1);

			if (cw->cword.b.encdec)
				for (n = 0; n < SM4_BLOCK_SIZE; n += sizeof(size_t))
					*(size_t *)(walk.dst.virt.addr + n) =
						*(size_t *)(walk.iv + n) ^=
						*(size_t *)(walk.src.virt.addr + n);

			else
				for (n = 0; n < SM4_BLOCK_SIZE; n += sizeof(size_t)) {
					t = *(size_t *)(walk.src.virt.addr + n);
					*(size_t *)(walk.dst.virt.addr + n) =
						*(size_t *)(walk.iv + n) ^ t;
					*(size_t *)(walk.iv + n) = t;
				}

			walk.src.virt.addr += SM4_BLOCK_SIZE;
			walk.dst.virt.addr += SM4_BLOCK_SIZE;
		}

		err = skcipher_walk_done(&walk, walk.nbytes % SM4_BLOCK_SIZE);
	}

	return err;
}

/*
 * cfb_encrypt_zxc is used for ZX-C+
 */
static int cfb_encrypt_zxc(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 1;
	cw.cword.pad |= 0x20 | SM4_CFB;

	err = sm4_cfb_zxc(req, &cw);

	return err;
}

/*
 * cfb_decrypt_zxc is used for ZX-C+
 */
static int cfb_decrypt_zxc(struct skcipher_request *req)
{
	int err;
	struct sm4_cipher_data cw;

	cw.cword.pad = 0;
	cw.cword.b.encdec = 0;
	cw.cword.pad |= 0x20 | SM4_CFB;

	err = sm4_cfb_zxc(req, &cw);

	return err;
}

static struct skcipher_alg sm4_algs[] = {
	{
		.base = {
			.cra_name = "__ecb(sm4)",
			.cra_driver_name = "__ecb-sm4-gmi",
			.cra_priority = 300,
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
		.encrypt = ecb_encrypt,
		.decrypt = ecb_decrypt,
	},

	{
		.base = {
			.cra_name = "__cbc(sm4)",
			.cra_driver_name = "__cbc-sm4-gmi",
			.cra_priority = 300,
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
			.cra_priority = 300,
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
			.cra_priority = 300,
			.cra_flags = CRYPTO_ALG_INTERNAL,
			.cra_blocksize = SM4_BLOCK_SIZE,
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
			.cra_priority = 300,
			.cra_flags = CRYPTO_ALG_INTERNAL,
			.cra_blocksize = SM4_BLOCK_SIZE,
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

static int gmi_zxc_check(void)
{
	int f_zxc = 0;

	struct cpuinfo_x86 *c = &cpu_data(0);

	if ((c->x86 > 6)) {
		f_zxc = 0;
	} else if (((c->x86 == 6) && (c->x86_model >= 0x0f)) ||
		   ((c->x86 == 6) && (c->x86_model == 0x09))) {
		f_zxc = 1;
	}

	return f_zxc;
}

/*
 * Load supported features of the CPU to see if the SM4 is available.
 */
static int gmi_ccs_available(void)
{
	struct cpuinfo_x86 *c = &cpu_data(0);
	u32 eax, edx;

	if (((c->x86 == 6) && (c->x86_model >= 0x0f)) ||
	    ((c->x86 == 6) && (c->x86_model == 0x09)) ||
	    (c->x86 > 6)) {
		if (!boot_cpu_has(X86_FEATURE_CCS) || !boot_cpu_has(X86_FEATURE_CCS_EN)) {
			eax = 0xC0000001;
			__asm__ __volatile__ ("cpuid":"=d"(edx):"a"(eax) : );

			if ((edx & 0x0030) != 0x0030)
				return -ENODEV;

			pr_debug("GMI SM4 is detected by CPUID\n");
			return 0;
		}
		pr_debug("GMI SM4 is available\n");
		return 0;
	}
	return -ENODEV;
}

static void gmi_sm4_exit(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sm4_simd_algs) && sm4_simd_algs[i]; i++)
		simd_skcipher_free(sm4_simd_algs[i]);

	crypto_unregister_skciphers(sm4_algs, ARRAY_SIZE(sm4_algs));
}

static int __init gmi_sm4_init(void)
{
	struct simd_skcipher_alg *simd;
	const char *basename;
	const char *algname;
	const char *drvname;
	int err;
	int i;

	if (gmi_ccs_available())
		return -ENODEV;

	if (gmi_zxc_check()) {
		for (i = 0; i < ARRAY_SIZE(sm4_algs); i++) {
			if (!strcmp(sm4_algs[i].base.cra_name, "__ctr(sm4)")) {
				sm4_algs[i].encrypt = ctr_encrypt_zxc;
				sm4_algs[i].decrypt = ctr_decrypt_zxc;
			} else if (!strcmp(sm4_algs[i].base.cra_name, "__cfb(sm4)")) {
				sm4_algs[i].encrypt = cfb_encrypt_zxc;
				sm4_algs[i].decrypt = cfb_decrypt_zxc;
			} else if (!strcmp(sm4_algs[i].base.cra_name, "__ofb(sm4)")) {
				sm4_algs[i].encrypt = ofb_encrypt_zxc;
				sm4_algs[i].decrypt = ofb_decrypt_zxc;
			}
		}
	}

	err = crypto_register_skciphers(sm4_algs, ARRAY_SIZE(sm4_algs));
	if (err)
		return err;

	for (i = 0; i < ARRAY_SIZE(sm4_algs); i++) {
		algname = sm4_algs[i].base.cra_name + 2;
		drvname = sm4_algs[i].base.cra_driver_name + 2;
		basename = sm4_algs[i].base.cra_driver_name;
		simd = simd_skcipher_create_compat(algname, drvname, basename);
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
MODULE_LICENSE("GPL v2");

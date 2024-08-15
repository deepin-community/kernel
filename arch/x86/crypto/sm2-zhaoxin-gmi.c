// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * SM2 asymmetric public-key algorithm
 * as specified by OSCCA GM/T 0003.1-2012 -- 0003.5-2012 SM2 and
 * described at https://tools.ietf.org/html/draft-shen-sm2-ecdsa-02
 *
 * Copyright (c) 2023 Shanghai Zhaoxin Semiconductor LTD.
 * Authors: YunShen <yunshen@zhaoxin.com>
 */

#include <linux/module.h>
#include <linux/mpi.h>
#include <crypto/internal/akcipher.h>
#include <crypto/akcipher.h>
#include <crypto/sm2.h>
#include <asm/cpufeature.h>
#include <asm/processor.h>
#include <asm/cpu_device_id.h>

#define SCRATCH_SIZE (4 * 2048)

#define SM2_CWORD_VERIFY 0x8
#define SM2_VERIFY_PASS 1

struct sm2_cipher_data {
	u8 pub_key[65]; /* public key */
};

/* Load supported features of the CPU to see if the SM2 is available. */
static int zhaoxin_gmi_available(void)
{
	if (!boot_cpu_has(X86_FEATURE_SM2_EN)) {
		pr_err("can't enable hardware SM2 if Zhaoxin GMI SM2 is not enabled\n");
		return -ENODEV;
	}
	return 0;
}

/* Zhaoxin sm2 verify function */
static inline size_t zhaoxin_gmi_sm2_verify(unsigned char *key, unsigned char *hash,
				unsigned char *sig, unsigned char *scratch)
{
	size_t result;

	asm volatile(
		".byte 0xf2, 0x0f, 0xa6, 0xc0"
		:"=c"(result)
		:"a"(hash), "b"(key), "d"(SM2_CWORD_VERIFY), "S"(scratch), "D"(sig)
		:"memory");

	return result;
}

/* Zhaoxin sm2 verify function */
static int _zhaoxin_sm2_verify(struct sm2_cipher_data *ec, unsigned char *hash, unsigned char *sig)
{
	unsigned char *scratch = kzalloc(SCRATCH_SIZE, GFP_KERNEL);
	int ret = -EKEYREJECTED;
	size_t result;

	result = zhaoxin_gmi_sm2_verify(ec->pub_key, hash, sig, scratch);
	if (result == SM2_VERIFY_PASS)
		ret = 0;

	kfree(scratch);

	return ret;
}

static int zhaoxin_sm2_verify(struct akcipher_request *req)
{
	struct crypto_akcipher *tfm = crypto_akcipher_reqtfm(req);
	struct sm2_cipher_data *ec = akcipher_tfm_ctx(tfm);
	unsigned char *buffer;
	int ret, buf_len;

	buf_len = req->src_len + req->dst_len;
	buffer = kmalloc(buf_len, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;

	sg_pcopy_to_buffer(req->src, sg_nents_for_len(req->src, buf_len), buffer, buf_len, 0);
	ret = _zhaoxin_sm2_verify(ec, buffer + req->src_len, buffer);

	kfree(buffer);

	return ret;
}

static int zhaoxin_sm2_set_pub_key(struct crypto_akcipher *tfm, const void *key,
				unsigned int keylen)
{
	struct sm2_cipher_data *ec = akcipher_tfm_ctx(tfm);

	memcpy(ec->pub_key, key, keylen);

	return 0;
}

static unsigned int zhaoxin_sm2_max_size(struct crypto_akcipher *tfm)
{
	/* Unlimited max size */
	return PAGE_SIZE;
}

static int zhaoxin_sm2_init_tfm(struct crypto_akcipher *tfm)
{
	return zhaoxin_gmi_available();
}

static void zhaoxin_sm2_exit_tfm(struct crypto_akcipher *tfm)
{
	struct sm2_cipher_data *ec = akcipher_tfm_ctx(tfm);

	memset(ec, 0, sizeof(*ec));
}

static struct akcipher_alg zhaoxin_sm2 = {
	.verify = zhaoxin_sm2_verify,
	.set_pub_key = zhaoxin_sm2_set_pub_key,
	.max_size = zhaoxin_sm2_max_size,
	.init = zhaoxin_sm2_init_tfm,
	.exit = zhaoxin_sm2_exit_tfm,
	.base = {
		.cra_name = "sm2",
		.cra_driver_name = "zhaoxin-gmi-sm2",
		.cra_priority = 150,
		.cra_module = THIS_MODULE,
		.cra_ctxsize = sizeof(struct sm2_cipher_data),
	},
};

static const struct x86_cpu_id zhaoxin_sm2_cpu_ids[] = {
	X86_MATCH_FEATURE(X86_FEATURE_SM2, NULL),
	{}
};
MODULE_DEVICE_TABLE(x86cpu, zhaoxin_sm2_cpu_ids);

static int __init zhaoxin_sm2_init(void)
{
	if (!x86_match_cpu(zhaoxin_sm2_cpu_ids))
		return -ENODEV;

	return crypto_register_akcipher(&zhaoxin_sm2);
}

static void __exit zhaoxin_sm2_exit(void)
{
	crypto_unregister_akcipher(&zhaoxin_sm2);
}

module_init(zhaoxin_sm2_init);
module_exit(zhaoxin_sm2_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("YunShen <yunshen@zhaoxin.com>");
MODULE_DESCRIPTION("SM2 Zhaoxin GMI Algorithm");
MODULE_ALIAS_CRYPTO("zhaoxin-gmi-sm2");

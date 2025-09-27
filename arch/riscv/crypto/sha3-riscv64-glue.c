// SPDX-License-Identifier: GPL-2.0-only
/*
 * SHA3 using the RISC-V vector crypto extensions
 *
 * Copyright (C) 2025 YiLin Zhang <Gas6352@outlook.com>
 */

#include <asm/simd.h>
#include <asm/vector.h>
#include <crypto/internal/hash.h>
#include <crypto/internal/simd.h>
#include <crypto/sha3.h>
#include <linux/cpufeature.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/unaligned.h>

MODULE_DESCRIPTION("SHA3 using RISC-V Vector Extensions");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS_CRYPTO("sha3-224");
MODULE_ALIAS_CRYPTO("sha3-256");
MODULE_ALIAS_CRYPTO("sha3-384");
MODULE_ALIAS_CRYPTO("sha3-512");

asmlinkage int sha3_riscv64_transform_zvkb(u64 *st, const u8 *data, int blocks,
					   int md_len);

static int sha3_update(struct shash_desc *desc, const u8 *data,
		       unsigned int len)
{
	struct sha3_state *sctx = shash_desc_ctx(desc);
	unsigned int digest_size = crypto_shash_digestsize(desc->tfm);

	if (!crypto_simd_usable())
		return crypto_sha3_update(desc, data, len);

	if ((sctx->partial + len) >= sctx->rsiz) {
		int blocks;

		if (sctx->partial) {
			int p = sctx->rsiz - sctx->partial;
			memcpy(sctx->buf + sctx->partial, data, p);
			kernel_vector_begin();
			sha3_riscv64_transform_zvkb(sctx->st, sctx->buf, 1,
						    digest_size);
			kernel_vector_end();

			data += p;
			len -= p;
			sctx->partial = 0;
		}

		blocks = len / sctx->rsiz;
		len %= sctx->rsiz;

		while (blocks) {
			int rem;

			kernel_vector_begin();
			rem = sha3_riscv64_transform_zvkb(sctx->st, data,
							  blocks, digest_size);
			kernel_vector_end();
			data += (blocks - rem) * sctx->rsiz;
			blocks = rem;
		}
	}

	if (len) {
		memcpy(sctx->buf + sctx->partial, data, len);
		sctx->partial += len;
	}
	return 0;
}

static int sha3_final(struct shash_desc *desc, u8 *out)
{
	struct sha3_state *sctx = shash_desc_ctx(desc);
	unsigned int digest_size = crypto_shash_digestsize(desc->tfm);
	__le64 *digest = (__le64 *)out;
	int i;

	if (!crypto_simd_usable())
		return crypto_sha3_final(desc, out);

	sctx->buf[sctx->partial++] = 0x06;
	memset(sctx->buf + sctx->partial, 0, sctx->rsiz - sctx->partial);
	sctx->buf[sctx->rsiz - 1] |= 0x80;

	kernel_vector_begin();
	sha3_riscv64_transform_zvkb(sctx->st, sctx->buf, 1, digest_size);
	kernel_vector_end();

	for (i = 0; i < digest_size / 8; i++)
		put_unaligned_le64(sctx->st[i], digest++);

	if (digest_size & 4)
		put_unaligned_le32(sctx->st[i], (__le32 *)digest);

	memzero_explicit(sctx, sizeof(*sctx));
	return 0;
}

static struct shash_alg riscv64_sha3_algs[] = {
	{
		.digestsize = SHA3_224_DIGEST_SIZE,
		.init = crypto_sha3_init,
		.update = sha3_update,
		.final = sha3_final,
		.descsize = sizeof(struct sha3_state),
		.base.cra_name = "sha3-224",
		.base.cra_driver_name = "sha3-224-riscv64-zvkb",
		.base.cra_blocksize = SHA3_224_BLOCK_SIZE,
		.base.cra_module = THIS_MODULE,
		.base.cra_priority = 200,
	},
	{
		.digestsize = SHA3_256_DIGEST_SIZE,
		.init = crypto_sha3_init,
		.update = sha3_update,
		.final = sha3_final,
		.descsize = sizeof(struct sha3_state),
		.base.cra_name = "sha3-256",
		.base.cra_driver_name = "sha3-256-riscv64-zvkb",
		.base.cra_blocksize = SHA3_256_BLOCK_SIZE,
		.base.cra_module = THIS_MODULE,
		.base.cra_priority = 200,
	},
	{
		.digestsize = SHA3_384_DIGEST_SIZE,
		.init = crypto_sha3_init,
		.update = sha3_update,
		.final = sha3_final,
		.descsize = sizeof(struct sha3_state),
		.base.cra_name = "sha3-384",
		.base.cra_driver_name = "sha3-384-riscv64-zvkb",
		.base.cra_blocksize = SHA3_384_BLOCK_SIZE,
		.base.cra_module = THIS_MODULE,
		.base.cra_priority = 200,
	},
	{
		.digestsize = SHA3_512_DIGEST_SIZE,
		.init = crypto_sha3_init,
		.update = sha3_update,
		.final = sha3_final,
		.descsize = sizeof(struct sha3_state),
		.base.cra_name = "sha3-512",
		.base.cra_driver_name = "sha3-512-riscv64-zvkb",
		.base.cra_blocksize = SHA3_512_BLOCK_SIZE,
		.base.cra_module = THIS_MODULE,
		.base.cra_priority = 200,
	}
};

static int __init sha3_riscv_mod_init(void)
{
	if (riscv_isa_extension_available(NULL, ZVKB)) {
		return crypto_register_shashes(riscv64_sha3_algs,
					       ARRAY_SIZE(riscv64_sha3_algs));
	}
	return -ENODEV;
}

static void __exit sha3_riscv_mod_fini(void)
{
	crypto_unregister_shashes(riscv64_sha3_algs,
				  ARRAY_SIZE(riscv64_sha3_algs));
}

module_init(sha3_riscv_mod_init);
module_exit(sha3_riscv_mod_fini);

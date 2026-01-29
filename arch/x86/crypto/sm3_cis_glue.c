// SPDX-License-Identifier: GPL-2.0-only
/*
 * HYGON SM3 Cipher Algorithm, using cis instructions.
 *
 * Copyright (C) 2026 Hygon Information Technology Co., Ltd.
 *
 */

#include <crypto/internal/hash.h>
#include <crypto/internal/simd.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <crypto/sm3.h>
#include <crypto/sm3_base.h>
#include <asm/simd.h>

asmlinkage void cis_sm3_transform(u32 *digest,
				const u8 *data,
				int nblocks);

static inline void sm3_transform_cis(struct sm3_state *sctx,
			const u8 *data, int nblocks)
{
	cis_sm3_transform(sctx->state, data, nblocks);
}

static int sm3_cis_update(struct shash_desc *desc, const u8 *data,
			 unsigned int len)
{
	struct sm3_state *sctx = shash_desc_ctx(desc);

	if (!crypto_simd_usable() ||
			(sctx->count % SM3_BLOCK_SIZE) + len < SM3_BLOCK_SIZE) {
		sm3_update(sctx, data, len);
		return 0;
	}

	kernel_fpu_begin();
	sm3_base_do_update(desc, data, len, sm3_transform_cis);
	kernel_fpu_end();

	return 0;
}

static int sm3_cis_finup(struct shash_desc *desc, const u8 *data,
			  unsigned int len, u8 *out)
{
	if (!crypto_simd_usable()) {
		struct sm3_state *sctx = shash_desc_ctx(desc);

		if (len)
			sm3_update(sctx, data, len);

		sm3_final(sctx, out);
		return 0;
	}

	kernel_fpu_begin();
	if (len)
		sm3_base_do_update(desc, data, len, sm3_transform_cis);
	sm3_base_do_finalize(desc, sm3_transform_cis);
	kernel_fpu_end();

	return sm3_base_finish(desc, out);
}

static int sm3_cis_final(struct shash_desc *desc, u8 *out)
{
	if (!crypto_simd_usable()) {
		sm3_final(shash_desc_ctx(desc), out);
		return 0;
	}

	kernel_fpu_begin();
	sm3_base_do_finalize(desc, sm3_transform_cis);
	kernel_fpu_end();

	return sm3_base_finish(desc, out);
}

static struct shash_alg sm3_cis_alg = {
	.digestsize	=	SM3_DIGEST_SIZE,
	.init		=	sm3_base_init,
	.update		=	sm3_cis_update,
	.final		=	sm3_cis_final,
	.finup		=	sm3_cis_finup,
	.descsize	=	sizeof(struct sm3_state),
	.base		=	{
		.cra_name	=	"sm3",
		.cra_driver_name =	"sm3-cis",
		.cra_priority	=	400,
		.cra_blocksize	=	SM3_BLOCK_SIZE,
		.cra_module	=	THIS_MODULE,
	}
};

static int __init sm3_cis_init(void)
{
#ifdef CONFIG_X86_64
	if (!boot_cpu_has(X86_FEATURE_HYGON_SM3)) {
		pr_err("CIS SM3 Not Support");
		return -ENODEV;
	}
#endif /* CONFIG_X86_64 */

	return crypto_register_shash(&sm3_cis_alg);
}

static void __exit sm3_cis_exit(void)
{
	crypto_unregister_shash(&sm3_cis_alg);
}

module_init(sm3_cis_init);
module_exit(sm3_cis_exit);

MODULE_DESCRIPTION("SM3 Cipher Algorithm, Hygon CIS optimized");
MODULE_LICENSE("GPL");
MODULE_ALIAS_CRYPTO("sm3");

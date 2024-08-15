// SPDX-License-Identifier: GPL-2.0
/*
 * sm3_zhaoxin_gmi.c - wrapper code for Zhaoxin GMI.
 *
 * Copyright (C) 2023 Shanghai Zhaoxin Semiconductor LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <crypto/internal/hash.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <crypto/sm3.h>
#include <crypto/sm3_base.h>
#include <linux/bitops.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>
#include <linux/cpufeature.h>
#include <linux/processor.h>

/*
 * Load supported features of the CPU to see if the SM3/SM4 is available.
 */
static int gmi_available(void)
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

			pr_debug("GMI SM3 detected by CPUID\n");
			return 0;
		}
		pr_debug("GMI SM3 is available\n");
		return 0;
	}
	return -ENODEV;
}

static void sm3_generic_block_fn(struct sm3_state *sst, const u8 *inp, int blockcnt)
{
	unsigned long in, out, cnt, blksz, ctrl;

	if (!blockcnt)
		return;

	in  = (unsigned long)inp;
	out = (unsigned long)(sst->state);
	cnt = (unsigned long)blockcnt;
	blksz = 0x20;
	ctrl = -1;

	__asm__ __volatile__(
		".byte 0xf3,0x0f,0xa6,0xe8\n"
		: "+S"(in)
		: "S"(in), "D"(out), "c"(cnt), "b"(blksz), "a"(ctrl)
		: "memory"
	);
}

static inline int zx_sm3_init(struct shash_desc *desc)
{
	struct sm3_state *sctx;

	if (!desc)
		return -EINVAL;

	sctx = shash_desc_ctx(desc);

	sctx->state[0] = 0x6f168073UL;
	sctx->state[1] = 0xb9b21449UL;
	sctx->state[2] = 0xd7422417UL;
	sctx->state[3] = 0x00068adaUL;
	sctx->state[4] = 0xbc306fa9UL;
	sctx->state[5] = 0xaa383116UL;
	sctx->state[6] = 0x4dee8de3UL;
	sctx->state[7] = 0x4e0efbb0UL;

	sctx->count = 0;

	return 0;
}

static inline int zx_sm3_base_finish(struct shash_desc *desc, u8 *out)
{
	struct sm3_state *sctx = shash_desc_ctx(desc);
	__be32 *digest = (__be32 *)out;

	memcpy(digest, sctx->state, SM3_DIGEST_SIZE);

	*sctx = (struct sm3_state){};
	return 0;
}

static int zx_sm3_update(struct shash_desc *desc, const u8 *data, unsigned int len)
{
	return sm3_base_do_update(desc, data, len, sm3_generic_block_fn);
}

static int zx_sm3_final(struct shash_desc *desc, u8 *out)
{
	sm3_base_do_finalize(desc, sm3_generic_block_fn);

	return zx_sm3_base_finish(desc, out);
}

static int zx_sm3_finup(struct shash_desc *desc, const u8 *data, unsigned int len, u8 *hash)
{
	sm3_base_do_update(desc, data, len, sm3_generic_block_fn);

	return zx_sm3_final(desc, hash);
}

static struct shash_alg zx_sm3_alg = {
	.digestsize = SM3_DIGEST_SIZE,
	.init = zx_sm3_init,
	.update = zx_sm3_update,
	.final = zx_sm3_final,
	.finup = zx_sm3_finup,
	.descsize = sizeof(struct sm3_state),
	.base = {
		.cra_name = "sm3",
		.cra_driver_name = "sm3-zhaoxin-gmi",
		.cra_priority = 300,
		.cra_blocksize = SM3_BLOCK_SIZE,
		.cra_module = THIS_MODULE,
	}
};

static int __init zx_sm3_generic_mod_init(void)
{
	if (!!gmi_available())
		return -ENODEV;

	pr_info("GMI is available on this platform.");
	return crypto_register_shash(&zx_sm3_alg);
}

static void __exit zx_sm3_generic_mod_fini(void)
{
	crypto_unregister_shash(&zx_sm3_alg);
}

module_init(zx_sm3_generic_mod_init);
module_exit(zx_sm3_generic_mod_fini);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("SM3 Secure Hash Algorithm");

MODULE_ALIAS_CRYPTO("sm3-zhaoxin");
MODULE_ALIAS_CRYPTO("sm3-zhaoxin-gmi");

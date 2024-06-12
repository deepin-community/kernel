// SPDX-License-Identifier: GPL-2.0
/*
 * RNG driver for Zhaoxin RNGs
 *
 * Copyright 2023 (c) Zhaoxin Semiconductor Co., Ltd
 */

#include <crypto/padlock.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hw_random.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/cpufeature.h>
#include <asm/cpu_device_id.h>
#include <asm/fpu/api.h>

enum {
	ZHAOXIN_RNG_CHUNK_8		= 0x00, /* 64 rand bits, 64 stored bits */
	ZHAOXIN_RNG_CHUNK_4		= 0x01, /* 32 rand bits, 32 stored bits */
	ZHAOXIN_RNG_CHUNK_2		= 0x02, /* 16 rand bits, 32 stored bits */
	ZHAOXIN_RNG_CHUNK_1		= 0x03, /*  8 rand bits, 32 stored bits */
	ZHAOXIN_RNG_MAX_SIZE	= (128 * 1024),
};

static int zhaoxin_rng_init(struct hwrng *rng)
{
	if (!boot_cpu_has(X86_FEATURE_XSTORE_EN)) {
		pr_err(PFX "can't enable hardware RNG if XSTORE is not enabled\n");
		return -ENODEV;
	}

	return 0;
}

static inline int rep_xstore(size_t size, size_t factor, void *result)
{
	asm(".byte 0xf3, 0x0f, 0xa7, 0xc0"
		: "=m"(*(size_t *)result), "+c"(size), "+d"(factor), "+D"(result));

	return 0;
}

static int zhaoxin_rng_read(struct hwrng *rng, void *data, size_t max, bool wait)
{
	if (max > ZHAOXIN_RNG_MAX_SIZE)
		max = ZHAOXIN_RNG_MAX_SIZE;

	rep_xstore(max, ZHAOXIN_RNG_CHUNK_1, data);

	return max;
}

static struct hwrng zhaoxin_rng = {
	.name = "zhaoxin",
	.init = zhaoxin_rng_init,
	.read = zhaoxin_rng_read,
};

static const struct x86_cpu_id zhaoxin_rng_cpu_ids[] = {
	X86_MATCH_VENDOR_FAM_FEATURE(ZHAOXIN, 6, X86_FEATURE_XSTORE, NULL),
	X86_MATCH_VENDOR_FAM_FEATURE(ZHAOXIN, 7, X86_FEATURE_XSTORE, NULL),
	X86_MATCH_VENDOR_FAM_FEATURE(CENTAUR, 7, X86_FEATURE_XSTORE, NULL),
	{}
};
MODULE_DEVICE_TABLE(x86cpu, zhaoxin_rng_cpu_ids);

static int __init zhaoxin_rng_mod_init(void)
{
	int err;

	if (!x86_match_cpu(zhaoxin_rng_cpu_ids)) {
		pr_err(PFX "The CPU isn't support XSTORE.\n");
		return -ENODEV;
	}

	pr_info("Zhaoxin RNG detected\n");

	err = hwrng_register(&zhaoxin_rng);
	if (err)
		pr_err(PFX "RNG registering failed (%d)\n", err);

	return err;
}
module_init(zhaoxin_rng_mod_init);

static void __exit zhaoxin_rng_mod_exit(void)
{
	hwrng_unregister(&zhaoxin_rng);
}
module_exit(zhaoxin_rng_mod_exit);

MODULE_DESCRIPTION("H/W RNG driver for Zhaoxin CPUs");
MODULE_AUTHOR("YunShen@zhaoxin.com");
MODULE_LICENSE("GPL");

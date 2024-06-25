// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Support for ACE hardware crypto engine.
 */

#include <crypto/internal/hash.h>
#include <crypto/padlock.h>
#include <crypto/sha1.h>
#include <crypto/sha2.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/scatterlist.h>
#include <asm/cpu_device_id.h>
#include <asm/fpu/api.h>

#define DRIVER_VERSION "1.0.0"

static inline void padlock_output_block(uint32_t *src, uint32_t *dst, size_t count)
{
	while (count--)
		*dst++ = swab32(*src++);
}

/*
 * Add two shash_alg instance for hardware-implemented multiple-parts hash
 * supported by Zhaoxin Processor.
 */
static int padlock_sha1_init_zhaoxin(struct shash_desc *desc)
{
	struct sha1_state *sctx = shash_desc_ctx(desc);

	*sctx = (struct sha1_state){
		.state = { SHA1_H0, SHA1_H1, SHA1_H2, SHA1_H3, SHA1_H4 },
	};

	return 0;
}

static int padlock_sha1_update_zhaoxin(struct shash_desc *desc, const u8 *data,	unsigned int len)
{
	struct sha1_state *sctx = shash_desc_ctx(desc);
	unsigned int partial, done;
	const u8 *src;

	/* The PHE require the out buffer must 128 bytes and 16-bytes aligned */
	u8 buf[128 + PADLOCK_ALIGNMENT - STACK_ALIGN] __aligned(STACK_ALIGN);
	u8 *dst = PTR_ALIGN(&buf[0], PADLOCK_ALIGNMENT);

	partial = sctx->count & 0x3f;
	sctx->count += len;
	done = 0;
	src = data;
	memcpy(dst, (u8 *)(sctx->state), SHA1_DIGEST_SIZE);

	if ((partial + len) >= SHA1_BLOCK_SIZE) {
		/* Append the bytes in state's buffer to a block to handle */
		if (partial) {
			done = -partial;
			memcpy(sctx->buffer + partial, data, done + SHA1_BLOCK_SIZE);
			src = sctx->buffer;
			asm volatile (".byte 0xf3,0x0f,0xa6,0xc8"
			: "+S"(src), "+D"(dst)
			: "a"((long)-1), "c"((unsigned long)1));
			done += SHA1_BLOCK_SIZE;
			src = data + done;
		}

		/* Process the left bytes from the input data */
		if (len - done >= SHA1_BLOCK_SIZE) {
			asm volatile (".byte 0xf3,0x0f,0xa6,0xc8"
			: "+S"(src), "+D"(dst)
			: "a"((long)-1), "c"((unsigned long)((len - done) / SHA1_BLOCK_SIZE)));
			done += ((len - done) - (len - done) % SHA1_BLOCK_SIZE);
			src = data + done;
		}
		partial = 0;
	}
	memcpy((u8 *)(sctx->state), dst, SHA1_DIGEST_SIZE);
	memcpy(sctx->buffer + partial, src, len - done);

	return 0;
}

static int padlock_sha1_final_zhaoxin(struct shash_desc *desc, u8 *out)
{
	struct sha1_state *state = (struct sha1_state *)shash_desc_ctx(desc);
	unsigned int partial, padlen;
	__be64 bits;
	static const u8 padding[64] = { 0x80, };

	bits = cpu_to_be64(state->count << 3);

	/* Pad out to 56 mod 64 */
	partial = state->count & 0x3f;
	padlen = (partial < 56) ? (56 - partial) : ((64+56) - partial);
	padlock_sha1_update_zhaoxin(desc, padding, padlen);

	/* Append length field bytes */
	padlock_sha1_update_zhaoxin(desc, (const u8 *)&bits, sizeof(bits));

	/* Swap to output */
	padlock_output_block((uint32_t *)(state->state), (uint32_t *)out, 5);

	return 0;
}

static int padlock_sha256_init_zhaoxin(struct shash_desc *desc)
{
	struct sha256_state *sctx = shash_desc_ctx(desc);

	*sctx = (struct sha256_state) {
		.state = {
			SHA256_H0, SHA256_H1, SHA256_H2, SHA256_H3,
			SHA256_H4, SHA256_H5, SHA256_H6, SHA256_H7
		},
	};

	return 0;
}

static int padlock_sha256_update_zhaoxin(struct shash_desc *desc, const u8 *data, unsigned int len)
{
	struct sha256_state *sctx = shash_desc_ctx(desc);
	unsigned int partial, done;
	const u8 *src;

	/* The PHE require the out buffer must 128 bytes and 16-bytes aligned */
	u8 buf[128 + PADLOCK_ALIGNMENT - STACK_ALIGN] __aligned(STACK_ALIGN);
	u8 *dst = PTR_ALIGN(&buf[0], PADLOCK_ALIGNMENT);

	partial = sctx->count & 0x3f;
	sctx->count += len;
	done = 0;
	src = data;
	memcpy(dst, (u8 *)(sctx->state), SHA256_DIGEST_SIZE);

	if ((partial + len) >= SHA256_BLOCK_SIZE) {

		/* Append the bytes in state's buffer to a block to handle */
		if (partial) {
			done = -partial;
			memcpy(sctx->buf + partial, data, done + SHA256_BLOCK_SIZE);
			src = sctx->buf;
			asm volatile (".byte 0xf3,0x0f,0xa6,0xd0"
			: "+S"(src), "+D"(dst)
			: "a"((long)-1), "c"((unsigned long)1));
			done += SHA256_BLOCK_SIZE;
			src = data + done;
		}

		/* Process the left bytes from input data */
		if (len - done >= SHA256_BLOCK_SIZE) {
			asm volatile (".byte 0xf3,0x0f,0xa6,0xd0"
			: "+S"(src), "+D"(dst)
			: "a"((long)-1), "c"((unsigned long)((len - done) / 64)));
			done += ((len - done) - (len - done) % 64);
			src = data + done;
		}
		partial = 0;
	}
	memcpy((u8 *)(sctx->state), dst, SHA256_DIGEST_SIZE);
	memcpy(sctx->buf + partial, src, len - done);

	return 0;
}

static int padlock_sha256_final_zhaoxin(struct shash_desc *desc, u8 *out)
{
	struct sha256_state *state = (struct sha256_state *)shash_desc_ctx(desc);
	unsigned int partial, padlen;
	__be64 bits;
	static const u8 padding[64] = { 0x80, };

	bits = cpu_to_be64(state->count << 3);

	/* Pad out to 56 mod 64 */
	partial = state->count & 0x3f;
	padlen = (partial < 56) ? (56 - partial) : ((64+56) - partial);
	padlock_sha256_update_zhaoxin(desc, padding, padlen);

	/* Append length field bytes */
	padlock_sha256_update_zhaoxin(desc, (const u8 *)&bits, sizeof(bits));

	/* Swap to output */
	padlock_output_block((uint32_t *)(state->state), (uint32_t *)out, 8);

	return 0;
}

static int padlock_sha_export_zhaoxin(struct shash_desc *desc, void *out)
{
	int statesize = crypto_shash_statesize(desc->tfm);
	void *sctx = shash_desc_ctx(desc);

	memcpy(out, sctx, statesize);
	return 0;
}

static int padlock_sha_import_zhaoxin(struct shash_desc *desc, const void *in)
{
	int statesize = crypto_shash_statesize(desc->tfm);
	void *sctx = shash_desc_ctx(desc);

	memcpy(sctx, in, statesize);
	return 0;
}

static struct shash_alg sha1_alg_zhaoxin = {
	.digestsize	=	SHA1_DIGEST_SIZE,
	.init		=	padlock_sha1_init_zhaoxin,
	.update		=	padlock_sha1_update_zhaoxin,
	.final		=	padlock_sha1_final_zhaoxin,
	.export		=	padlock_sha_export_zhaoxin,
	.import		=	padlock_sha_import_zhaoxin,
	.descsize	=	sizeof(struct sha1_state),
	.statesize	=	sizeof(struct sha1_state),
	.base = {
		.cra_name			=	"sha1",
		.cra_driver_name	=	"sha1-padlock-zhaoxin",
		.cra_priority		=	PADLOCK_CRA_PRIORITY,
		.cra_blocksize		=	SHA1_BLOCK_SIZE,
		.cra_module			=	THIS_MODULE,
	}
};

static struct shash_alg sha256_alg_zhaoxin = {
	.digestsize	=	SHA256_DIGEST_SIZE,
	.init		=	padlock_sha256_init_zhaoxin,
	.update		=	padlock_sha256_update_zhaoxin,
	.final		=	padlock_sha256_final_zhaoxin,
	.export		=	padlock_sha_export_zhaoxin,
	.import		=	padlock_sha_import_zhaoxin,
	.descsize	=	sizeof(struct sha256_state),
	.statesize	=	sizeof(struct sha256_state),
	.base = {
		.cra_name			=	"sha256",
		.cra_driver_name	=	"sha256-padlock-zhaoxin",
		.cra_priority		=	PADLOCK_CRA_PRIORITY,
		.cra_blocksize		=	SHA256_BLOCK_SIZE,
		.cra_module			=	THIS_MODULE,
	}
};

static const struct x86_cpu_id zhaoxin_sha_ids[] = {
	{ X86_VENDOR_CENTAUR, 7, X86_MODEL_ANY, X86_STEPPING_ANY, X86_FEATURE_PHE },
	{ X86_VENDOR_ZHAOXIN, 7, X86_MODEL_ANY, X86_STEPPING_ANY, X86_FEATURE_PHE },
	{}
};
MODULE_DEVICE_TABLE(x86cpu, zhaoxin_sha_ids);

static int __init padlock_init(void)
{
	int rc = -ENODEV;
	struct shash_alg *sha1;
	struct shash_alg *sha256;

	if (!x86_match_cpu(zhaoxin_sha_ids) || !boot_cpu_has(X86_FEATURE_PHE_EN))
		return -ENODEV;

	sha1 = &sha1_alg_zhaoxin;
	sha256 = &sha256_alg_zhaoxin;

	rc = crypto_register_shash(sha1);
	if (rc)
		goto out;

	rc = crypto_register_shash(sha256);
	if (rc)
		goto out_unreg1;

	pr_notice("Using ACE for SHA1/SHA256 algorithms.\n");

	return 0;

out_unreg1:
	crypto_unregister_shash(sha1);

out:
	pr_err("ACE SHA1/SHA256 initialization failed.\n");
	return rc;
}

static void __exit padlock_fini(void)
{
	crypto_unregister_shash(&sha1_alg_zhaoxin);
	crypto_unregister_shash(&sha256_alg_zhaoxin);
}

module_init(padlock_init);
module_exit(padlock_fini);

MODULE_DESCRIPTION("ACE SHA1/SHA256 algorithms support.");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Michal Ludvig");
MODULE_VERSION(DRIVER_VERSION);

MODULE_ALIAS_CRYPTO("sha1-all");
MODULE_ALIAS_CRYPTO("sha256-all");
MODULE_ALIAS_CRYPTO("sha1-padlock");
MODULE_ALIAS_CRYPTO("sha256-padlock");

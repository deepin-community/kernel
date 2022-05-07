// SPDX-License-Identifier: GPL-2.0-only
/*
 * Hygon Cryptographic Coprocessor (CCP) SM3 crypto API support
 *
 * Copyright (C) 2022 Hygon Info Technologies Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/scatterlist.h>
#include <linux/crypto.h>
#include <crypto/algapi.h>
#include <crypto/hash.h>
#include <crypto/internal/hash.h>
#include <crypto/scatterwalk.h>
#include <crypto/hmac.h>

#include "ccp-crypto.h"

static int ccp_sm3_complete(struct crypto_async_request *async_req, int ret)
{
	struct ahash_request *req = ahash_request_cast(async_req);
	struct ccp_sm3_req_ctx *rctx = ahash_request_ctx(req);

	if (ret)
		goto e_free;

	rctx->msg_bits += (rctx->hash_cnt << 3);
	if (rctx->hash_rem) {
		/* save remaining data to buffer */
		unsigned int offset = rctx->nbytes - rctx->hash_rem;

		scatterwalk_map_and_copy(rctx->buf, rctx->src,
					offset, rctx->hash_rem, 0);
		rctx->buf_count = rctx->hash_rem;
	} else {
		rctx->buf_count = 0;
	}

	if (rctx->final) {
		if (req->result)
			memcpy(req->result, rctx->ctx, SM3_DIGEST_SIZE);

		memset(rctx->ctx, 0, SM3_DIGEST_SIZE);
	}

e_free:
	sg_free_table(&rctx->data_sg);

	return ret;
}

static int ccp_do_sm3_update(struct ahash_request *req, unsigned int nbytes,
			     unsigned int final)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ccp_ctx *ctx = crypto_ahash_ctx(tfm);
	struct ccp_sm3_req_ctx *rctx = ahash_request_ctx(req);
	struct scatterlist *sg = req->src;
	struct ccp_sm3_engine *sm3 = NULL;
	unsigned int sg_count;
	gfp_t gfp;
	u64 len, msg_bits = 0;
	int nents;
	int ret;

	/* must check length of src,
	 * otherwise will result in NullPointer exception in ccp_sm3_complete
	 */
	if (nbytes) {
		nents = sg_nents_for_len(req->src, nbytes);
		if (nents < 0)
			return -EINVAL;
	}

	len = (u64)rctx->buf_count + (u64)nbytes;
	if (len <= SM3_BLOCK_SIZE) {
		scatterwalk_map_and_copy(rctx->buf + rctx->buf_count, req->src,
					 0, nbytes, 0);
		rctx->buf_count += nbytes;
		if (!final)
			return 0;

		sg_init_one(&rctx->buf_sg, rctx->buf, rctx->buf_count);
		sg = &rctx->buf_sg;
	} else {
		gfp = req->base.flags & CRYPTO_TFM_REQ_MAY_SLEEP ?
			GFP_KERNEL : GFP_ATOMIC;

		if (rctx->buf_count) {
			/* build the scatterlist table: (buffer and input data) */
			sg_count = sg_nents(req->src) + 1;
			ret = sg_alloc_table(&rctx->data_sg, sg_count, gfp);
			if (ret)
				return ret;

			sg_init_one(&rctx->buf_sg, rctx->buf, rctx->buf_count);
			sg = ccp_crypto_sg_table_add(
				&rctx->data_sg, &rctx->buf_sg);
			if (!sg) {
				ret = -EINVAL;
				goto e_free;
			}
			sg = ccp_crypto_sg_table_add(&rctx->data_sg,
						req->src);
			if (!sg) {
				ret = -EINVAL;
				goto e_free;
			}
			sg_mark_end(sg);

			sg = rctx->data_sg.sgl;
		} else {
			sg = req->src;
		}
	}

	rctx->final = final;
	if (final) {
		rctx->hash_rem = 0;
		rctx->hash_cnt = len;
		msg_bits = rctx->msg_bits + (len << 3);
	} else {
		rctx->hash_rem = len & (SM3_BLOCK_SIZE - 1);
		rctx->hash_cnt = len - rctx->hash_rem;
		rctx->src = req->src;
		rctx->nbytes = nbytes;
	}

	memset(&rctx->cmd, 0, sizeof(rctx->cmd));
	INIT_LIST_HEAD(&rctx->cmd.entry);
	rctx->cmd.engine = CCP_ENGINE_SM3;

	sm3 = &rctx->cmd.u.sm3;
	sm3->type = CCP_SM3_TYPE_256;
	sm3->ctx = &rctx->ctx_sg;
	sm3->ctx_len = SM3_DIGEST_SIZE;
	sm3->src = sg;
	sm3->src_len = rctx->hash_cnt;
	sm3->first = rctx->msg_bits ? 0 : 1;
	sm3->final = final;
	sm3->msg_bits = msg_bits;
	if (final && ctx->u.sm3.key_len) {
		sm3->opad = &ctx->u.sm3.opad_sg;
		sm3->opad_len = SM3_BLOCK_SIZE;
	}

	ret = ccp_crypto_enqueue_request(&req->base, &rctx->cmd);

	return ret;

e_free:
	sg_free_table(&rctx->data_sg);

	return ret;
}

static int ccp_sm3_init(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ccp_ctx *ctx = crypto_ahash_ctx(tfm);
	struct ccp_sm3_req_ctx *rctx = ahash_request_ctx(req);

	if ((crypto_ahash_get_flags(tfm) & CRYPTO_TFM_NEED_KEY) &&
		(!ctx->u.sm3.key_len))
		return -ENOKEY;

	memset(rctx, 0, sizeof(*rctx));
	if (ctx->u.sm3.key_len) {
		/* buffer the HMAC key for first update */
		memcpy(rctx->buf, ctx->u.sm3.ipad, SM3_BLOCK_SIZE);
		rctx->buf_count = SM3_BLOCK_SIZE;
	}

	sg_init_one(&rctx->ctx_sg, rctx->ctx, SM3_DIGEST_SIZE);

	return 0;
}

static int ccp_sm3_update(struct ahash_request *req)
{
	return ccp_do_sm3_update(req, req->nbytes, 0);
}

static int ccp_sm3_final(struct ahash_request *req)
{
	return ccp_do_sm3_update(req, 0, 1);
}

static int ccp_sm3_finup(struct ahash_request *req)
{
	return ccp_do_sm3_update(req, req->nbytes, 1);
}

static int ccp_sm3_digest(struct ahash_request *req)
{
	int ret;

	ret = ccp_sm3_init(req);
	if (unlikely(ret))
		return ret;

	return ccp_sm3_finup(req);
}

static int ccp_sm3_export(struct ahash_request *req, void *out)
{
	struct ccp_sm3_req_ctx *rctx = ahash_request_ctx(req);
	struct ccp_sm3_exp_ctx state;

	if (!out)
		return -EINVAL;

	/* don't let anything leak to 'out' */
	memset(&state, 0, sizeof(state));

	state.msg_bits = rctx->msg_bits;
	memcpy(state.ctx, rctx->ctx, SM3_DIGEST_SIZE);
	state.buf_count = rctx->buf_count;
	memcpy(state.buf, rctx->buf, SM3_BLOCK_SIZE);

	/* 'out' may not be aligned so memcpy from local variable */
	memcpy(out, &state, sizeof(state));
	memset(&state, 0, sizeof(state));

	return 0;
}

static int ccp_sm3_import(struct ahash_request *req, const void *in)
{
	struct ccp_sm3_req_ctx *rctx = ahash_request_ctx(req);
	struct ccp_sm3_exp_ctx state;

	if (!in)
		return -EINVAL;

	/* 'in' may not be aligned so memcpy to local variable */
	memcpy(&state, in, sizeof(state));

	memset(rctx, 0, sizeof(*rctx));
	rctx->msg_bits = state.msg_bits;
	memcpy(rctx->ctx, state.ctx, SM3_DIGEST_SIZE);
	sg_init_one(&rctx->ctx_sg, rctx->ctx, SM3_DIGEST_SIZE);
	rctx->buf_count = state.buf_count;
	memcpy(rctx->buf, state.buf, SM3_BLOCK_SIZE);

	memset(&state, 0, sizeof(state));

	return 0;
}

static int ccp_sm3_setkey(struct crypto_ahash *tfm, const u8 *key,
			  unsigned int key_len)
{
	struct ccp_ctx *ctx = crypto_tfm_ctx(crypto_ahash_tfm(tfm));
	struct crypto_shash *shash = ctx->u.sm3.hmac_tfm;

	SHASH_DESC_ON_STACK(sdesc, shash);

	int i, ret;

	/* set to zero until complete */
	ctx->u.sm3.key_len = 0;
	if (!key)
		return -EINVAL;

	if (!key_len) {
		crypto_ahash_set_flags(tfm, CRYPTO_TFM_NEED_KEY);
		return -EINVAL;
	}

	/* clear key area to provide zero padding for keys smaller
	 * than the block size
	 */
	memset(ctx->u.sm3.key, 0, SM3_BLOCK_SIZE);

	if (key_len > SM3_BLOCK_SIZE) {
		/* must hash the input key */
		sdesc->tfm = shash;
		ret = crypto_shash_digest(sdesc, key, key_len,
					  ctx->u.sm3.key);
		if (ret) {
			crypto_ahash_set_flags(
				tfm, CRYPTO_TFM_NEED_KEY);
			return -EINVAL;
		}

		key_len = SM3_DIGEST_SIZE;
	} else {
		memcpy(ctx->u.sm3.key, key, key_len);
	}

	for (i = 0; i < SM3_BLOCK_SIZE; i++) {
		ctx->u.sm3.ipad[i] = ctx->u.sm3.key[i] ^ HMAC_IPAD_VALUE;
		ctx->u.sm3.opad[i] = ctx->u.sm3.key[i] ^ HMAC_OPAD_VALUE;
	}

	sg_init_one(&ctx->u.sm3.opad_sg, ctx->u.sm3.opad, SM3_BLOCK_SIZE);

	ctx->u.sm3.key_len = key_len;

	return 0;
}

static int ccp_sm3_cra_init(struct crypto_tfm *tfm)
{
	struct ccp_ctx *ctx = crypto_tfm_ctx(tfm);
	struct crypto_ahash *ahash = __crypto_ahash_cast(tfm);

	ctx->complete = ccp_sm3_complete;
	crypto_ahash_set_reqsize(ahash, sizeof(struct ccp_sm3_req_ctx));

	return 0;
}

static void ccp_sm3_cra_exit(struct crypto_tfm *tfm)
{
}

static int ccp_hmac_sm3_cra_init(struct crypto_tfm *tfm)
{
	struct ccp_ctx *ctx = crypto_tfm_ctx(tfm);
	struct ccp_crypto_ahash_alg *alg = ccp_crypto_ahash_alg(tfm);
	struct crypto_shash *hmac_tfm;

	hmac_tfm = crypto_alloc_shash(alg->child_alg, 0, 0);
	if (IS_ERR(hmac_tfm)) {
		pr_warn("could not load driver %s need for HMAC support\n",
			alg->child_alg);
		return PTR_ERR(hmac_tfm);
	}

	ctx->u.sm3.hmac_tfm = hmac_tfm;

	return ccp_sm3_cra_init(tfm);
}

static void ccp_hmac_sm3_cra_exit(struct crypto_tfm *tfm)
{
	struct ccp_ctx *ctx = crypto_tfm_ctx(tfm);

	if (ctx->u.sm3.hmac_tfm)
		crypto_free_shash(ctx->u.sm3.hmac_tfm);

	ccp_sm3_cra_exit(tfm);
}

struct ccp_sm3_def {
	unsigned int version;
	const char *name;
	const char *drv_name;
	enum ccp_sm3_type type;
	u32 digest_size;
	u32 block_size;
};

static struct ccp_sm3_def sm3_algs[] = {
	{
		.version	= CCP_VERSION(5, 0),
		.name		= "sm3",
		.drv_name	= "sm3-ccp",
		.type		= CCP_SM3_TYPE_256,
		.digest_size	= SM3_DIGEST_SIZE,
		.block_size	= SM3_BLOCK_SIZE,
	},
};

static int ccp_register_hmac_sm3_hygon_alg(struct list_head *head,
				const struct ccp_sm3_def *def,
				const struct ccp_crypto_ahash_alg *base_alg)
{
	struct ccp_crypto_ahash_alg *ccp_alg;
	struct ahash_alg *alg;
	struct crypto_alg *base;
	int ret;

	ccp_alg = kzalloc(sizeof(*ccp_alg), GFP_KERNEL);
	if (!ccp_alg)
		return -ENOMEM;

	/* copy the base algorithm and only change what's necessary */
	*ccp_alg = *base_alg;
	INIT_LIST_HEAD(&ccp_alg->entry);

	strscpy(ccp_alg->child_alg, def->name, CRYPTO_MAX_ALG_NAME);

	alg = &ccp_alg->alg;
	alg->setkey = ccp_sm3_setkey;

	base = &alg->halg.base;
	snprintf(base->cra_name, CRYPTO_MAX_ALG_NAME, "hmac(%s)", def->name);
	snprintf(base->cra_driver_name, CRYPTO_MAX_ALG_NAME, "hmac-%s",
		 def->drv_name);
	base->cra_flags |= CRYPTO_ALG_NEED_FALLBACK;
	base->cra_init = ccp_hmac_sm3_cra_init;
	base->cra_exit = ccp_hmac_sm3_cra_exit;

	ret = crypto_register_ahash(alg);
	if (ret) {
		pr_err("%s ahash algorithm registration error (%d)\n",
		       base->cra_name, ret);
		kfree(ccp_alg);
		return ret;
	}

	list_add(&ccp_alg->entry, head);

	return ret;
}

static int ccp_register_sm3_hygon_alg(struct list_head *head,
				const struct ccp_sm3_def *def)
{
	struct ccp_crypto_ahash_alg *ccp_alg;
	struct ahash_alg *alg;
	struct hash_alg_common *halg;
	struct crypto_alg *base;
	int ret;

	ccp_alg = kzalloc(sizeof(*ccp_alg), GFP_KERNEL);
	if (!ccp_alg)
		return -ENOMEM;

	INIT_LIST_HEAD(&ccp_alg->entry);

	ccp_alg->type = def->type;

	alg = &ccp_alg->alg;
	alg->init = ccp_sm3_init;
	alg->update = ccp_sm3_update;
	alg->final = ccp_sm3_final;
	alg->finup = ccp_sm3_finup;
	alg->digest = ccp_sm3_digest;
	alg->export = ccp_sm3_export;
	alg->import = ccp_sm3_import;

	halg = &alg->halg;
	halg->digestsize = def->digest_size;
	halg->statesize = sizeof(struct ccp_sm3_exp_ctx);

	base = &halg->base;
	snprintf(base->cra_name, CRYPTO_MAX_ALG_NAME, "%s", def->name);
	snprintf(base->cra_driver_name, CRYPTO_MAX_ALG_NAME, "%s",
		 def->drv_name);
	base->cra_flags = CRYPTO_ALG_ASYNC |
			  CRYPTO_ALG_KERN_DRIVER_ONLY |
			  CRYPTO_ALG_NEED_FALLBACK;
	base->cra_blocksize = def->block_size;
	base->cra_ctxsize = sizeof(struct ccp_ctx);
	base->cra_priority = CCP_CRA_PRIORITY;
	base->cra_init = ccp_sm3_cra_init;
	base->cra_exit = ccp_sm3_cra_exit;
	base->cra_module = THIS_MODULE;

	ret = crypto_register_ahash(alg);
	if (ret) {
		pr_err("%s ahash algorithm registration error (%d)\n",
		       base->cra_name, ret);
		kfree(ccp_alg);
		return ret;
	}

	list_add(&ccp_alg->entry, head);

	ret = ccp_register_hmac_sm3_hygon_alg(head, def, ccp_alg);

	return ret;
}

int ccp_register_sm3_hygon_algs(struct list_head *head)
{
	int i, ret;
	unsigned int ccpversion = ccp_version();

	for (i = 0; i < ARRAY_SIZE(sm3_algs); i++) {
		if (sm3_algs[i].version > ccpversion)
			continue;
		ret = ccp_register_sm3_hygon_alg(head, &sm3_algs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

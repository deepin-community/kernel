// SPDX-License-Identifier: GPL-2.0-only
/*
 * Hygon Cryptographic Coprocessor (CCP) SM4 crypto API support
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
#include <crypto/scatterwalk.h>

#include "ccp-crypto.h"

enum ccp_sm4_alg_mode {
	CCP_SM4_ALG_MODE_ECB = CCP_SM4_MODE_ECB,
	CCP_SM4_ALG_MODE_CBC = CCP_SM4_MODE_CBC,
	CCP_SM4_ALG_MODE_OFB = CCP_SM4_MODE_OFB,
	CCP_SM4_ALG_MODE_CFB = CCP_SM4_MODE_CFB,
	CCP_SM4_ALG_MODE_CTR = CCP_SM4_MODE_CTR,
	CCP_SM4_ALG_MODE_ECB_HS = CCP_SM4_MODE_HS_SEL | CCP_SM4_MODE_ECB,
	CCP_SM4_ALG_MODE_CBC_HS = CCP_SM4_MODE_HS_SEL | CCP_SM4_MODE_CBC,
	CCP_SM4_ALG_MODE__LAST,
};

static int ccp_sm4_complete(struct crypto_async_request *async_req, int ret)
{
	struct skcipher_request *req = skcipher_request_cast(async_req);
	struct ccp_ctx *ctx = crypto_tfm_ctx(req->base.tfm);
	struct ccp_sm4_req_ctx *rctx = skcipher_request_ctx(req);

	if (ret)
		return ret;

	if ((ctx->u.sm4.mode & CCP_SM4_MODE_MASK) != CCP_SM4_ALG_MODE_ECB) {
		memcpy(req->iv, rctx->iv, SM4_BLOCK_SIZE);
		memset(rctx->iv, 0, SM4_BLOCK_SIZE);
	}

	return 0;
}

static int ccp_sm4_setkey(struct crypto_skcipher *tfm, const u8 *key,
			  unsigned int key_len)
{
	struct ccp_ctx *ctx = crypto_skcipher_ctx(tfm);

	/* key_len is checked by crypto_ablkcipher_type,
	 * but key isn't checked
	 */
	if (!key)
		return -EINVAL;

	memcpy(ctx->u.sm4.key, key, SM4_KEY_SIZE);
	sg_init_one(&ctx->u.sm4.key_sg, ctx->u.sm4.key, SM4_KEY_SIZE);

	ctx->u.sm4.key_len = SM4_KEY_SIZE;

	return 0;
}

static int ccp_sm4_crypt(struct skcipher_request *req, bool encrypt)
{
	struct crypto_skcipher *tfm = crypto_skcipher_reqtfm(req);
	struct ccp_ctx *ctx = crypto_skcipher_ctx(tfm);
	struct ccp_sm4_req_ctx *rctx = skcipher_request_ctx(req);
	struct scatterlist *iv_sg = NULL;
	struct ccp_cmd *cmd = NULL;
	enum ccp_sm4_alg_mode mode;
	enum ccp_sm4_action action;
	int ret;

	if (!ctx->u.sm4.key_len)
		return -ENOKEY;

	mode = ctx->u.sm4.mode;
	if ((mode != CCP_SM4_ALG_MODE_CTR) &&
			(mode != CCP_SM4_ALG_MODE_OFB) &&
			(mode != CCP_SM4_ALG_MODE_CFB) &&
			(req->cryptlen & (SM4_BLOCK_SIZE - 1)))
		return -EINVAL;

	if ((mode & CCP_SM4_MODE_MASK) != CCP_SM4_ALG_MODE_ECB) {
		if (!req->iv)
			return -EINVAL;

		memcpy(rctx->iv, req->iv, SM4_BLOCK_SIZE);
		iv_sg = &rctx->iv_sg;
		sg_init_one(iv_sg, rctx->iv, SM4_BLOCK_SIZE);
	}

	cmd = &rctx->cmd;
	memset(cmd, 0, sizeof(*cmd));
	INIT_LIST_HEAD(&cmd->entry);
	action = encrypt ? CCP_SM4_ACTION_ENCRYPT : CCP_SM4_ACTION_DECRYPT;
	if (mode == CCP_SM4_ALG_MODE_CTR) {
		cmd->engine = CCP_ENGINE_SM4_CTR;
		cmd->u.sm4_ctr.action = action;
		cmd->u.sm4_ctr.size = 63;
		cmd->u.sm4_ctr.step = 1;

		cmd->u.sm4_ctr.key = &ctx->u.sm4.key_sg;
		cmd->u.sm4_ctr.key_len = SM4_KEY_SIZE;
		cmd->u.sm4_ctr.iv = iv_sg;
		cmd->u.sm4_ctr.iv_len = SM4_BLOCK_SIZE;

		cmd->u.sm4_ctr.src = req->src;
		cmd->u.sm4_ctr.dst = req->dst;
		cmd->u.sm4_ctr.src_len = req->cryptlen;

	} else {
		cmd->engine = CCP_ENGINE_SM4;
		cmd->u.sm4.mode = mode & CCP_SM4_MODE_MASK;
		cmd->u.sm4.action = action;
		if (mode & CCP_SM4_MODE_HS_SEL)
			cmd->u.sm4.select = 1;

		cmd->u.sm4.key = &ctx->u.sm4.key_sg;
		cmd->u.sm4.key_len = SM4_KEY_SIZE;
		cmd->u.sm4.iv = iv_sg;
		cmd->u.sm4.iv_len = iv_sg ? SM4_BLOCK_SIZE : 0;

		cmd->u.sm4.src = req->src;
		cmd->u.sm4.dst = req->dst;
		cmd->u.sm4.src_len = req->cryptlen;
	}

	ret = ccp_crypto_enqueue_request(&req->base, &rctx->cmd);

	return ret;
}

static int ccp_sm4_encrypt(struct skcipher_request *req)
{
	return ccp_sm4_crypt(req, true);
}

static int ccp_sm4_decrypt(struct skcipher_request *req)
{
	return ccp_sm4_crypt(req, false);
}

static int ccp_sm4_init_tfm(struct crypto_skcipher *tfm)
{
	struct ccp_crypto_skcipher_alg *alg = ccp_crypto_skcipher_alg(tfm);
	struct ccp_ctx *ctx = crypto_skcipher_ctx(tfm);

	ctx->complete = ccp_sm4_complete;
	ctx->u.sm4.mode = alg->mode;

	crypto_skcipher_set_reqsize(tfm, sizeof(struct ccp_sm4_req_ctx));

	return 0;
}

static const struct skcipher_alg ccp_sm4_defaults = {
	.setkey			= ccp_sm4_setkey,
	.encrypt		= ccp_sm4_encrypt,
	.decrypt		= ccp_sm4_decrypt,
	.min_keysize		= SM4_KEY_SIZE,
	.max_keysize		= SM4_KEY_SIZE,
	.init			= ccp_sm4_init_tfm,

	.base.cra_flags		= CRYPTO_ALG_ASYNC |
				  CRYPTO_ALG_KERN_DRIVER_ONLY |
				  CRYPTO_ALG_NEED_FALLBACK,
	.base.cra_blocksize	= SM4_BLOCK_SIZE,
	.base.cra_ctxsize	= sizeof(struct ccp_ctx),
	.base.cra_priority	= CCP_CRA_PRIORITY,
	.base.cra_module	= THIS_MODULE,
};

struct ccp_sm4_def {
	enum ccp_sm4_alg_mode mode;
	unsigned int version;
	const char *name;
	const char *driver_name;
	unsigned int blocksize;
	unsigned int ivsize;
	const struct skcipher_alg *alg_defaults;
};

static struct ccp_sm4_def sm4_algs[] = {
	{
		.mode		= CCP_SM4_ALG_MODE_ECB,
		.version	= CCP_VERSION(5, 0),
		.name		= "ecb(sm4)",
		.driver_name	= "ecb-sm4-ccp",
		.blocksize	= SM4_BLOCK_SIZE,
		.ivsize		= 0,
		.alg_defaults	= &ccp_sm4_defaults,
	},
	{
		.mode		= CCP_SM4_ALG_MODE_ECB_HS,
		.version	= CCP_VERSION(5, 0),
		.name		= "ecb(sm4)",
		.driver_name	= "ecb-sm4-hs-ccp",
		.blocksize	= SM4_BLOCK_SIZE,
		.ivsize		= 0,
		.alg_defaults	= &ccp_sm4_defaults,
	},
	{
		.mode		= CCP_SM4_ALG_MODE_CBC,
		.version	= CCP_VERSION(5, 0),
		.name		= "cbc(sm4)",
		.driver_name	= "cbc-sm4-ccp",
		.blocksize	= SM4_BLOCK_SIZE,
		.ivsize		= SM4_BLOCK_SIZE,
		.alg_defaults	= &ccp_sm4_defaults,
	},
	{
		.mode		= CCP_SM4_ALG_MODE_CBC_HS,
		.version	= CCP_VERSION(5, 0),
		.name		= "cbc(sm4)",
		.driver_name	= "cbc-sm4-hs-ccp",
		.blocksize	= SM4_BLOCK_SIZE,
		.ivsize		= SM4_BLOCK_SIZE,
		.alg_defaults	= &ccp_sm4_defaults,
	},
	{
		.mode		= CCP_SM4_ALG_MODE_OFB,
		.version	= CCP_VERSION(5, 0),
		.name		= "ofb(sm4)",
		.driver_name	= "ofb-sm4-ccp",
		.blocksize	= SM4_BLOCK_SIZE,
		.ivsize		= SM4_BLOCK_SIZE,
		.alg_defaults	= &ccp_sm4_defaults,
	},
	{
		.mode		= CCP_SM4_ALG_MODE_CFB,
		.version	= CCP_VERSION(5, 0),
		.name		= "cfb(sm4)",
		.driver_name	= "cfb-sm4-ccp",
		.blocksize	= SM4_BLOCK_SIZE,
		.ivsize		= SM4_BLOCK_SIZE,
		.alg_defaults	= &ccp_sm4_defaults,
	},
	{
		.mode		= CCP_SM4_ALG_MODE_CTR,
		.version	= CCP_VERSION(5, 0),
		.name		= "ctr(sm4)",
		.driver_name	= "ctr-sm4-ccp",
		.blocksize	= 1,
		.ivsize		= SM4_BLOCK_SIZE,
		.alg_defaults	= &ccp_sm4_defaults,
	},
};

static int ccp_register_sm4_hygon_alg(struct list_head *head,
				const struct ccp_sm4_def *def)
{
	struct ccp_crypto_skcipher_alg *ccp_alg;
	struct skcipher_alg *alg;
	int ret;

	ccp_alg = kzalloc(sizeof(*ccp_alg), GFP_KERNEL);
	if (!ccp_alg)
		return -ENOMEM;

	INIT_LIST_HEAD(&ccp_alg->entry);

	ccp_alg->mode = def->mode;

	/* copy the defaults and override as necessary */
	alg = &ccp_alg->alg;
	*alg = *def->alg_defaults;
	snprintf(alg->base.cra_name, CRYPTO_MAX_ALG_NAME, "%s", def->name);
	snprintf(alg->base.cra_driver_name, CRYPTO_MAX_ALG_NAME, "%s",
			def->driver_name);
	alg->base.cra_blocksize = def->blocksize;
	alg->ivsize = def->ivsize;

	ret = crypto_register_skcipher(alg);
	if (ret) {
		pr_err("%s skcipher algorithm registration error (%d)\n",
		       alg->base.cra_name, ret);
		kfree(ccp_alg);
		return ret;
	}

	list_add(&ccp_alg->entry, head);

	return 0;
}

int ccp_register_sm4_hygon_algs(struct list_head *head)
{
	int i, ret;
	unsigned int ccpversion = ccp_version();

	for (i = 0; i < ARRAY_SIZE(sm4_algs); i++) {
		if (sm4_algs[i].version > ccpversion)
			continue;
		ret = ccp_register_sm4_hygon_alg(head, &sm4_algs[i]);
		if (ret)
			return ret;
	}

	return 0;
}

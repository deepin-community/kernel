// SPDX-License-Identifier: GPL-2.0
#include "loongson_se_crypto.h"

extern struct device *se_dev;

static int se_send_sm3_cmd(struct lsse_crypto *se, u32 cmd,
		struct se_alg_engine *sae, int retry)
{
	struct se_alg_msg *alg_msg = (struct se_alg_msg *)sae->se_ch->smsg;
	dma_addr_t data_base = sae->se_ch->se->mem_addr;
	unsigned long flag;
	int err = 0;

	if (!alg_msg)
		return -EINVAL;

	spin_lock_irqsave(&sae->se_ch->ch_lock, flag);

	if (cmd == SE_CMD_SM3_HMAC_DIGEST)
		alg_msg->u.req.len = sae->buffer_cnt - SM3_BLOCK_SIZE;
	else
		alg_msg->u.req.len = sae->buffer_cnt;

	alg_msg->cmd = cmd;
	alg_msg->u.req.in_off = sae->in_addr - data_base;
	alg_msg->u.req.out_off = sae->out_addr - data_base;

	sae->cmd = alg_msg->cmd;

try_again:
	pr_debug("SM3 CMD data offset is 0x%x, data length is %d\n",
			alg_msg->u.req.in_off, alg_msg->u.req.len);

	if (!retry--)
		goto out;

	err = se_send_ch_request(sae->se_ch);
	if (err)
		goto try_again;

out:
	spin_unlock_irqrestore(&sae->se_ch->ch_lock, flag);
	return err;
}

static int loongson_sm3_update(struct ahash_request *req)
{
	struct loongson_hash_request_ctx *rctx = ahash_request_ctx(req);

	/*
	 * NOTE:
	 * return -EINPROGRESS upper layer will wait
	 * return other ecodes to inform upper layer request failed
	 * return 0 upper layer will go
	 */
	if (!req->nbytes)
		return 0;

	rctx->sg = req->src;
	rctx->nents = sg_nents_for_len(req->src, req->nbytes);
	rctx->rest_bytes = req->nbytes;

	return loongson_sm3_enqueue(req, SE_HASH_OP_UPDATE);
}

static int loongson_sm3_finup(struct ahash_request *req)
{
	struct loongson_hash_request_ctx *rctx = ahash_request_ctx(req);
	int err;

	/* The engine only allows one request to be pushed in */
	rctx->flags |= SE_HASH_FLAGS_FINUP;

	err = loongson_sm3_update(req);
	if (err)
		return err;

	err = loongson_sm3_final(req);

	return err;
}

static int loongson_sm3_digest(struct ahash_request *req)
{
	return loongson_sm3_init(req) ? : loongson_sm3_finup(req);
}

static int loongson_sm3_final_req(struct ahash_request *req)
{
	struct lsse_crypto *se = (struct lsse_crypto *)se_dev->driver_data;
	struct loongson_hash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));
	struct se_alg_engine *sae = ctx->sae;
	u32 cmd;
	int err = 0;

	if (ctx->keylen)
		cmd = SE_CMD_SM3_HMAC_DIGEST;
	else
		cmd = SE_CMD_SM3_DIGEST;

	err = se_send_sm3_cmd(se, cmd, sae, 5);
	if (err)
		return err;

	return -EINPROGRESS;
}

static int loongson_sm3_update_req(struct ahash_request *req)
{
	struct loongson_hash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));
	struct loongson_hash_request_ctx *rctx = ahash_request_ctx(req);
	struct se_alg_engine *sae = ctx->sae;
	size_t bytes;
	int err = 0;

	if (rctx->rest_bytes + sae->buffer_cnt > sae->buffer_size) {
		pr_err("SM3 data overflow! Max size is 0x%x Bytes\n", sae->buffer_size);
		err = -ENOMEM;
		rctx->flags |= SE_HASH_FLAGS_FINUP;
		goto out;
	}

	if (!sae->buffer_cnt && ctx->keylen) {
		memcpy(sae->in_buffer, ctx->key, SM3_BLOCK_SIZE);
		sae->buffer_cnt = SM3_BLOCK_SIZE;
	}

	if (rctx->sg && rctx->nents) {
		bytes = sg_pcopy_to_buffer(rctx->sg, rctx->nents,
			sae->in_buffer + sae->buffer_cnt, rctx->rest_bytes, 0);
		if (bytes != rctx->rest_bytes) {
			err = -EFAULT;
			rctx->flags |= SE_HASH_FLAGS_FINUP;
		}
		sae->buffer_cnt += rctx->rest_bytes;
		rctx->sg = NULL;
		rctx->nents = 0;
		rctx->rest_bytes = 0;
	}

out:
	return err;
}

static void loongson_sm3_finish_task(struct se_alg_engine *sae)
{
	struct loongson_hash_request_ctx *rctx =
		(struct loongson_hash_request_ctx *)sae->rctx;

	pr_debug("%s op %lu ret %d\n", __func__, rctx->op, sae->cmd_ret);

	if (!rctx || !rctx->req) {
		sae->rctx = NULL;
		goto no_req;
	}

	/* The buffer is cleared regardless of whether the final cmd is successful*/
	if (rctx->op == SE_HASH_OP_FINAL || rctx->flags & SE_HASH_FLAGS_FINUP) {
		if (!sae->cmd_ret)
			memcpy(rctx->req->result, sae->out_buffer, SM3_DIGEST_SIZE);

		memset(sae->in_buffer, 0, sae->buffer_cnt);
		sae->buffer_cnt = 0;
		rctx->flags &= ~SE_HASH_FLAGS_FINUP;
		sae->rctx = NULL;
	}

	crypto_finalize_hash_request(sae->engine, rctx->req, sae->cmd_ret);
no_req:
	list_del(&sae->finish_list);
}

static int loongson_sm3_one_request(struct crypto_engine *engine, void *areq)
{
	struct lsse_crypto *se = (struct lsse_crypto *)se_dev->driver_data;
	struct ahash_request *req = container_of(areq, struct ahash_request, base);
	struct loongson_hash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));
	struct loongson_hash_request_ctx *rctx = ahash_request_ctx(req);
	struct se_alg_engine *sae = ctx->sae;
	int err = 0;

	if (!sae)
		return -ENODEV;

	sae->rctx = rctx;
	rctx->req = req;

	if (rctx->op == SE_HASH_OP_UPDATE)
		err = loongson_sm3_update_req(req);

	if (rctx->op == SE_HASH_OP_FINAL ||
			(rctx->flags & SE_HASH_FLAGS_FINUP && !err))
		err = loongson_sm3_final_req(req);

	pr_debug("%s op %lu flags 0x%lx err %d\n",
			__func__, rctx->op, rctx->flags, err);

	if (err != -EINPROGRESS) {
		sae->cmd_ret = err;
		list_add_tail(&sae->finish_list, &se->finish_engine);
		loongson_se_finish_req(se);
	}

	return 0;
}

static int loongson_se_ahash_init(struct crypto_tfm *tfm)
{
	struct loongson_hash_ctx *tfm_ctx = crypto_tfm_ctx(tfm);
	struct lsse_crypto *se = dev_get_drvdata(se_dev);

	crypto_ahash_set_reqsize(__crypto_ahash_cast(tfm),
				 sizeof(struct loongson_hash_request_ctx));
	memset(tfm_ctx, 0, sizeof(*tfm_ctx));

	tfm_ctx->sae = se_find_engine(se, SE_ALG_TYPE_SM3);
	if (unlikely(!tfm_ctx->sae))
		return -ENODEV;

	return 0;
}

static int loongson_sm4_cipher_once(struct se_alg_engine *sae,
		struct skcipher_request *req, u32 op)
{
	struct lsse_crypto *se = (struct lsse_crypto *)se_dev->driver_data;
	struct loongson_skcipher_request_ctx *rctx = skcipher_request_ctx(req);
	size_t nbytes;

	if (sae->buffer_cnt)
		return -EINPROGRESS;

	nbytes = min(rctx->rest_bytes, sae->buffer_size);
	if (!nbytes)
		return 0;

	/* TMP workaround for no hardware ctr */
	if (rctx->op == SE_CMD_SM4_CTR) {
		int i;

		for (i = 0; i < nbytes; i += SM4_BLOCK_SIZE) {
			memcpy(sae->in_buffer + i,
				rctx->update_iv, SM4_BLOCK_SIZE);
			crypto_inc((u8 *)rctx->update_iv, SM4_BLOCK_SIZE);
		}
	} else if (sg_pcopy_to_buffer(rctx->src, rctx->in_nents, sae->in_buffer,
				nbytes, rctx->copyed_bytes) != nbytes) {
		pr_err("SM4 copy sg to buffer failed!\n");
		return -EFAULT;
	}

	sae->buffer_cnt = nbytes;

	return se_send_sm4_cmd(se, sae, op, 5);
}

static void loongson_sm4_finish_task(struct se_alg_engine *sae)
{
	struct loongson_skcipher_request_ctx *rctx =
		(struct loongson_skcipher_request_ctx *)sae->rctx;
	int err = 0;

	if (!rctx || !rctx->req)
		goto no_req;

	pr_debug("%s op %lu ret %d\n", __func__, rctx->op, sae->cmd_ret);

	if (sae->cmd_ret || !sae->buffer_cnt) {
		err = sae->cmd_ret;
		sae->buffer_cnt = 0;
		goto out;
	}

	/* TMP workaround for no hardware ctr */
	if (rctx->op == SE_CMD_SM4_CTR) {
		char *tmp = kmalloc(sae->buffer_cnt, GFP_KERNEL);

		if (sg_pcopy_to_buffer(rctx->src, sg_nents(rctx->src), tmp,
			sae->buffer_cnt, rctx->copyed_bytes) != sae->buffer_cnt)
			pr_info("copy != sae->buffer_cnt\n");

		crypto_xor_cpy(sae->out_buffer, sae->out_buffer, tmp, sae->buffer_cnt);

		if (sg_pcopy_from_buffer(rctx->dst, sg_nents(rctx->dst), sae->out_buffer,
			sae->buffer_cnt, rctx->copyed_bytes) != sae->buffer_cnt)
			pr_info("copy2 != sae->buffer_cnt\n");
		memcpy(rctx->req->iv, rctx->update_iv, 16);

		kfree(tmp);

	} else if (sg_pcopy_from_buffer(rctx->dst, rctx->out_nents, sae->out_buffer,
			sae->buffer_cnt, rctx->copyed_bytes) != sae->buffer_cnt) {
		pr_err("SM4 finish copy failed!\n");
		err = -EFAULT;
	}

	rctx->rest_bytes -= sae->buffer_cnt;
	rctx->copyed_bytes += sae->buffer_cnt;
	sae->buffer_cnt = 0;

	if (rctx->rest_bytes && !err) {
		err = loongson_sm4_cipher_once(sae, rctx->req, rctx->op);
		if (!err) {
			list_del(&sae->finish_list);
			return;
		}
	}

out:
	crypto_finalize_skcipher_request(sae->engine, rctx->req, err);
no_req:
	sae->rctx = NULL;
	list_del(&sae->finish_list);

	err = 0;
	while (!list_empty(&sae->wait_list) && !err) {
		rctx = list_first_entry(&sae->wait_list,
			struct loongson_skcipher_request_ctx, rctx_list);
		list_del(&sae->wait_list);
		sae->rctx = rctx;
		err = loongson_sm4_cipher_once(sae, rctx->req, rctx->op);
		if (err)
			crypto_finalize_skcipher_request(sae->engine, rctx->req, err);
	}
}

static int loongson_sm4_one_request(struct crypto_engine *engine, void *areq)
{
	struct skcipher_request *req = container_of(areq,
					struct skcipher_request, base);
	struct loongson_skcipher_ctx *ctx = crypto_skcipher_ctx(
					crypto_skcipher_reqtfm(req));
	struct crypto_skcipher *cipher = crypto_skcipher_reqtfm(req);
	struct loongson_skcipher_request_ctx *rctx = skcipher_request_ctx(req);
	size_t ivlen = crypto_skcipher_ivsize(cipher);
	struct se_alg_engine *sae = ctx->sae;

	if (!sae)
		return -ENODEV;

	if (sae->rctx) {
		int err;
		/* Should wait last request done */
		crypto_finalize_skcipher_request(sae->engine, req, -EINPROGRESS);
		err = loongson_sm4_enqueue(req, rctx->op);
		return err == -EINPROGRESS ? 0 : err;
	}

	rctx->req = req;
	rctx->src = req->src;
	rctx->dst = req->dst;
	rctx->in_nents = sg_nents_for_len(req->src, req->cryptlen);
	rctx->out_nents = sg_nents_for_len(req->dst, req->cryptlen);
	rctx->copyed_bytes = 0;
	rctx->rest_bytes = req->cryptlen;
	rctx->keylen = ctx->keylen;
	rctx->ivlen = ivlen;

	/* TMP workaround for no hardware ctr */
	if (rctx->op == SE_CMD_SM4_CTR) {
		memcpy(rctx->update_iv, req->iv, ivlen);
		rctx->rest_bytes = round_up(rctx->rest_bytes, SM4_BLOCK_SIZE);
	}

	memcpy(sae->key_buffer, ctx->key, ctx->keylen);
	if (ivlen)
		memcpy(rctx->iv, req->iv, ivlen);
	memcpy(sae->info_buffer, rctx->iv, rctx->ivlen);

	if (sae->rctx) {
		list_add_tail(&rctx->rctx_list, &sae->wait_list);
		return 0;
	}
	sae->rctx = rctx;

	return loongson_sm4_cipher_once(sae, req, rctx->op);
}

static int loongson_se_sm4_init(struct crypto_tfm *tfm)
{
	struct loongson_skcipher_ctx *tfm_ctx = crypto_tfm_ctx(tfm);
	struct lsse_crypto *se = dev_get_drvdata(se_dev);

	crypto_skcipher_set_reqsize(__crypto_skcipher_cast(tfm),
				 sizeof(struct loongson_skcipher_request_ctx));
	memset(tfm_ctx, 0, sizeof(*tfm_ctx));

	tfm_ctx->sae = se_find_engine(se, SE_ALG_TYPE_SM4);
	if (unlikely(!tfm_ctx->sae))
		return -ENODEV;

	return 0;
}

void loongson_se_task_routine_1_1(unsigned long data)
{
	struct lsse_crypto *se = (struct lsse_crypto *)data;
	struct se_alg_engine *sae;
	struct se_alg_engine *next;

	list_for_each_entry_safe(sae, next, &se->finish_engine, finish_list) {
		switch (sae->type) {
		case SE_ALG_TYPE_SM3:
			loongson_sm3_finish_task(sae);
			break;

		case SE_ALG_TYPE_SM4:
			loongson_sm4_finish_task(sae);
			break;

		default:
			pr_debug("%s Unrecognized ALG %d\n", __func__, sae->type);
			break;
		}
	}
}

static int loongson_sm3_export(struct ahash_request *req, void *out)
{
	struct loongson_hash_request_ctx *rctx = ahash_request_ctx(req);

	rctx->flags = 0;
	return 0;
}

static int loongson_sm3_import(struct ahash_request *req, const void *in)
{
	struct loongson_hash_request_ctx *rctx = ahash_request_ctx(req);

	rctx->flags = 0;
	return 0;
}


/*------------------- Algorithm definitions ----------------------------------*/
struct loongson_alg_common se_alg_1_1[5] = {
	{
		.type = SE_ALG_TYPE_SM3,
		.data_size = SE_SM3_DATA_SIZE_1,
		.need_engine = true,
		.u.ahash.base = {
			.init = loongson_sm3_init,
			.update = loongson_sm3_update,
			.export = loongson_sm3_export,
			.import = loongson_sm3_import,
			.final = loongson_sm3_final,
			.digest = loongson_sm3_digest,
			.halg.digestsize = SM3_DIGEST_SIZE,
			.halg.statesize = sizeof(struct loongson_hash_request_ctx),
			.halg.base = {
				.cra_name = "sm3",
				.cra_driver_name = "loongson-se-sm3",
				.cra_priority = 300,
				.cra_flags = CRYPTO_ALG_ASYNC,
				.cra_blocksize = SM3_BLOCK_SIZE,
				.cra_ctxsize = sizeof(struct loongson_hash_ctx),
				.cra_alignmask = 0,
				.cra_module = THIS_MODULE,
				.cra_init = loongson_se_ahash_init,
				.cra_exit = loongson_se_ahash_exit,
			},
		},
		.u.ahash.op = {
			.do_one_request = loongson_sm3_one_request,
		}
	},
	{
		.type = SE_ALG_TYPE_SM3,
		.data_size = SE_SM3_DATA_SIZE_1,
		.need_engine = true,
		.u.ahash.base = {
			.init = loongson_sm3_init,
			.update = loongson_sm3_update,
			.export = loongson_sm3_export,
			.import = loongson_sm3_import,
			.final = loongson_sm3_final,
			.digest = loongson_sm3_digest,
			.setkey = loongson_sm3_setkey,
			.halg.digestsize = SM3_DIGEST_SIZE,
			.halg.statesize = sizeof(struct loongson_hash_request_ctx),
			.halg.base = {
				.cra_name = "hmac(sm3)",
				.cra_driver_name = "loongson-se-sm3-hmac",
				.cra_priority = 300,
				.cra_flags = CRYPTO_ALG_ASYNC,
				.cra_blocksize = SM3_BLOCK_SIZE,
				.cra_ctxsize = sizeof(struct loongson_hash_ctx),
				.cra_alignmask = 0,
				.cra_module = THIS_MODULE,
				.cra_init = loongson_se_ahash_init,
				.cra_exit = loongson_se_ahash_exit,
			},
		},
		.u.ahash.op = {
			.do_one_request = loongson_sm3_one_request,
		}
	},
	{
		.type = SE_ALG_TYPE_SM4,
		.data_size = SE_SM4_DATA_SIZE,
		.need_engine = true,
		.u.skcipher.base = {
			.setkey = loongson_sm4_setkey,
			.encrypt = loongson_ecb_sm4_encrypt,
			.decrypt = loongson_ecb_sm4_decrypt,
			.min_keysize = SM4_KEY_SIZE,
			.max_keysize = SM4_KEY_SIZE,
			.base = {
				.cra_name = "ecb(sm4)",
				.cra_driver_name = "loongson-se-ecb-sm4",
				.cra_priority = 300,
				.cra_flags = CRYPTO_ALG_ASYNC,
				.cra_blocksize = SM4_BLOCK_SIZE,
				.cra_ctxsize = sizeof(struct loongson_skcipher_ctx),
				.cra_alignmask = 0,
				.cra_module = THIS_MODULE,
				.cra_init = loongson_se_sm4_init,
				.cra_exit = loongson_se_sm4_exit,
			},
		},
		.u.skcipher.op = {
			.do_one_request = loongson_sm4_one_request,
		}
	},
	{
		.type = SE_ALG_TYPE_SM4,
		.data_size = SE_SM4_DATA_SIZE,
		.need_engine = true,
		.u.skcipher.base = {
			.setkey = loongson_sm4_setkey,
			.encrypt = loongson_ctr_sm4,
			.decrypt = loongson_ctr_sm4,
			.min_keysize = SM4_KEY_SIZE,
			.max_keysize = SM4_KEY_SIZE,
			.ivsize = SM4_BLOCK_SIZE,
			.base = {
				.cra_name = "ctr(sm4)",
				.cra_driver_name = "loongson-se-ctr-sm4",
				.cra_priority = 300,
				.cra_flags = CRYPTO_ALG_ASYNC,
				.cra_blocksize = SM4_BLOCK_SIZE,
				.cra_ctxsize = sizeof(struct loongson_skcipher_ctx),
				.cra_alignmask = 0,
				.cra_module = THIS_MODULE,
				.cra_init = loongson_se_sm4_init,
				.cra_exit = loongson_se_sm4_exit,
			},
		},
		.u.skcipher.op = {
			.do_one_request = loongson_sm4_one_request,
		}
	},
	{
		.type = SE_ALG_TYPE_RNG,
		.data_size = SE_RNG_DATA_SIZE,
		.need_engine = false,
		.u.rng = {
			.generate	= loongson_rng_generate,
			.seed		= loongson_rng_seed,
			.seedsize	= 0,
			.base		= {
				.cra_name		= "stdrng",
				.cra_driver_name	= "loongson-se-rng",
				.cra_flags		= CRYPTO_ALG_TYPE_RNG,
				.cra_priority		= 300,
				.cra_ctxsize		= sizeof(struct loongson_rng_ctx),
				.cra_module		= THIS_MODULE,
				.cra_init		= loongson_se_rng_init,
			}
		}
	}
};


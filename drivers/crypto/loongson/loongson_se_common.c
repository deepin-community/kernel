// SPDX-License-Identifier: GPL-2.0
#include "loongson_se_crypto.h"

struct device *se_dev;

struct se_alg_engine *se_find_engine(struct lsse_crypto *se, u32 type)
{
	struct se_alg_engine *sae;
	struct se_alg_engine *next;

	list_for_each_entry_safe(sae, next, &se->alg_engine, engine_list) {
		if (sae->type == type)
			return sae;
	}

	return NULL;
}

/*-------------------- SM3 functions -----------------------------------------*/
static int keyhash(const char *alg_name, const u8 *key, u32 keylen, u8 *hash)
{
	struct crypto_ahash *tfm;
	struct scatterlist sg[1];
	struct ahash_request *req;
	struct crypto_wait wait;
	int ret;

	crypto_init_wait(&wait);

	tfm = crypto_alloc_ahash(alg_name, 0, 0);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);

	req = ahash_request_alloc(tfm, GFP_KERNEL);
	if (!req)
		return -ENOMEM;

	/* Keep tfm keylen == 0 during hash of the long key */
	ahash_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
				   crypto_req_done, &wait);

	sg_init_one(&sg[0], key, keylen);

	ahash_request_set_crypt(req, sg, hash, keylen);
	ret = crypto_wait_req(crypto_ahash_digest(req), &wait);

	ahash_request_free(req);

	return ret;
}

int loongson_sm3_enqueue(struct ahash_request *req, unsigned int op)
{
	struct loongson_hash_request_ctx *rctx = ahash_request_ctx(req);
	struct loongson_hash_ctx *ctx = crypto_tfm_ctx(req->base.tfm);

	rctx->op = op;

	return crypto_transfer_hash_request_to_engine(ctx->sae->engine, req);
}

int loongson_sm3_init(struct ahash_request *req)
{
	struct loongson_hash_request_ctx *rctx = ahash_request_ctx(req);

	memset(rctx, 0, sizeof(*rctx));

	return 0;
}

int loongson_sm3_hmac_init(struct ahash_request *req)
{
	struct loongson_hash_request_ctx *rctx = ahash_request_ctx(req);

	memset(rctx, 0, sizeof(*rctx));

	return loongson_sm3_enqueue(req, SE_CMD_SM3_HMAC_INIT);
}

int loongson_sm3_final(struct ahash_request *req)
{
	return loongson_sm3_enqueue(req, SE_HASH_OP_FINAL);
}

int loongson_sm3_hmac_final(struct ahash_request *req)
{
	return loongson_sm3_enqueue(req, SE_CMD_SM3_HMAC_FINISH);
}

int loongson_sm3_setkey(struct crypto_ahash *tfm,
			     const u8 *key, unsigned int keylen)
{
	struct loongson_hash_ctx *ctx = crypto_ahash_ctx(tfm);
	unsigned int blocksize =
			crypto_tfm_alg_blocksize(crypto_ahash_tfm(tfm));
	unsigned int digestsize = crypto_ahash_digestsize(tfm);
	unsigned int keysize = keylen;
	const char *alg_name = "loongson-se-sm3";
	u8 hash[SM3_DIGEST_SIZE];
	int ret;

	if (keylen <= blocksize) {
		memcpy(ctx->key, key, keysize);
		memset(((char *)ctx->key) + keysize, 0, blocksize - keysize);
	} else {
		/* Must get the hash of the long key */
		ret = keyhash(alg_name, key, keylen, hash);

		if (ret)
			return -EINVAL;

		keysize = digestsize;
		memcpy(ctx->key, hash, digestsize);
	}

	ctx->keylen = keysize;

	return 0;
}

void loongson_se_ahash_exit(struct crypto_tfm *tfm)
{
	struct loongson_hash_ctx *tfm_ctx = crypto_tfm_ctx(tfm);
	struct loongson_hash_request_ctx *rctx;
	struct se_alg_engine *sae = tfm_ctx->sae;

	if (sae && sae->rctx)
		rctx = (struct loongson_hash_request_ctx *)sae->rctx;
	else
		return;

	if (rctx->req && rctx->req->base.tfm == tfm) {
		/* Last request blocked, remove it */
		crypto_finalize_hash_request(sae->engine, rctx->req, -EFAULT);
		sae->rctx = NULL;
		sae->cmd = 0;
		sae->cmd_ret = 0;
		sae->buffer_cnt = 0;
	}
}

/*-------------------- SM4 functions -----------------------------------------*/
int loongson_sm4_enqueue(struct skcipher_request *req, unsigned int op)
{
	struct loongson_skcipher_ctx *ctx = crypto_skcipher_ctx(
					crypto_skcipher_reqtfm(req));
	struct loongson_skcipher_request_ctx *rctx = skcipher_request_ctx(req);
	struct se_alg_engine *sae = ctx->sae;

	rctx->op = op;

	return crypto_transfer_skcipher_request_to_engine(sae->engine, req);
}

int loongson_sm4_setkey(struct crypto_skcipher *cipher,
		const u8 *key, unsigned int len)
{
	struct crypto_tfm *tfm = crypto_skcipher_tfm(cipher);
	struct loongson_skcipher_ctx *ctx = crypto_tfm_ctx(tfm);

	if (unlikely(len > SM4_KEY_SIZE)) {
		pr_err("SM4 key length overflow!\n");
		return -EFAULT;
	}

	memcpy(ctx->key, key, len);
	ctx->keylen = len;

	return 0;
}

int loongson_ecb_sm4_encrypt(struct skcipher_request *req)
{
	return loongson_sm4_enqueue(req, SE_CMD_SM4_ECB_ENCRY);
}

int loongson_ecb_sm4_decrypt(struct skcipher_request *req)
{
	return loongson_sm4_enqueue(req, SE_CMD_SM4_ECB_DECRY);
}

int loongson_cbc_sm4_encrypt(struct skcipher_request *req)
{
	return loongson_sm4_enqueue(req, SE_CMD_SM4_CBC_ENCRY);
}

int loongson_cbc_sm4_decrypt(struct skcipher_request *req)
{
	return loongson_sm4_enqueue(req, SE_CMD_SM4_CBC_DECRY);
}

int loongson_ctr_sm4(struct skcipher_request *req)
{
	return loongson_sm4_enqueue(req, SE_CMD_SM4_CTR);
}

void loongson_se_sm4_exit(struct crypto_tfm *tfm)
{
	struct loongson_skcipher_ctx *tfm_ctx = crypto_tfm_ctx(tfm);
	struct loongson_skcipher_request_ctx *rctx;
	struct se_alg_engine *sae = tfm_ctx->sae;

	if (sae && sae->rctx)
		rctx = (struct loongson_skcipher_request_ctx *)sae->rctx;
	else
		return;

	if (rctx->req && rctx->req->base.tfm == tfm) {
		/* Last request blocked, remove it */
		crypto_finalize_skcipher_request(sae->engine, rctx->req, -EFAULT);
		sae->rctx = NULL;
		sae->cmd = 0;
		sae->cmd_ret = 0;
		sae->buffer_cnt = 0;
	}
}

int se_send_sm4_cmd(struct lsse_crypto *se,
		struct se_alg_engine *sae, u32 op, int retry)
{
	struct se_alg_msg *alg_msg = (struct se_alg_msg *)sae->se_ch->smsg;
	dma_addr_t data_base = sae->se_ch->se->mem_addr;
	unsigned long flag;
	int err = 0;

	if (!sae->buffer_cnt)
		return 0;

	if (!alg_msg)
		return -EINVAL;

	spin_lock_irqsave(&sae->se_ch->ch_lock, flag);

	/* TMP workaround for no hardware ctr */
	if (op == SE_CMD_SM4_CTR)
		alg_msg->cmd = SE_CMD_SM4_ECB_ENCRY;
	else
		alg_msg->cmd = op;

	sae->cmd = alg_msg->cmd;

	/*
	 * Buffer_cnt should not greater than buffer_size,
	 * so it is safe to round up.
	 */
	alg_msg->u.req.len = sae->buffer_cnt;
	alg_msg->u.req.in_off = sae->in_addr - data_base;
	alg_msg->u.req.out_off = sae->out_addr - data_base;
	alg_msg->u.req.key_off = sae->key_addr - data_base;
	alg_msg->u.req.info_off = sae->info_addr - data_base;

try_again:
	pr_debug("SM4 CMD %u key 0x%x, data 0x%x, data length is %d\n",
			alg_msg->cmd, alg_msg->u.req.key_off,
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

/*-------------------- RNG functions -----------------------------------------*/
static int se_send_rng_cmd(struct lsse_crypto *se,
		struct se_alg_engine *sae, int retry)
{
	struct se_alg_msg *alg_msg = (struct se_alg_msg *)sae->se_ch->smsg;
	dma_addr_t data_base = sae->se_ch->se->mem_addr;
	unsigned long flag;
	int err = 0;

	if (!sae->buffer_cnt)
		return 0;

	if (!alg_msg)
		return -EINVAL;

	spin_lock_irqsave(&sae->se_ch->ch_lock, flag);

	alg_msg->cmd = SE_CMD_RNG;
	alg_msg->u.req.len = round_up(sae->buffer_cnt, SE_RNG_DATA_ALIGN);
	alg_msg->u.req.out_off = sae->out_addr - data_base;

	sae->cmd = SE_CMD_RNG;

try_again:
	pr_debug("RNG CMD data offset is 0x%x, data length is %d\n",
			alg_msg->u.req.out_off, alg_msg->u.req.len);

	if (!retry--)
		goto out;

	err = se_send_ch_request(sae->se_ch);
	if (err)
		goto try_again;

out:
	spin_unlock_irqrestore(&sae->se_ch->ch_lock, flag);
	return err;
}

int loongson_rng_seed(struct crypto_rng *tfm, const u8 *seed,
			 unsigned int slen)
{
	return 0;
}

int loongson_rng_generate(struct crypto_rng *tfm,
			     const u8 *src, unsigned int slen,
			     u8 *dstn, unsigned int dlen)
{
	struct lsse_crypto *se = (struct lsse_crypto *)se_dev->driver_data;
	struct loongson_rng_ctx *ctx = crypto_rng_ctx(tfm);
	struct se_alg_engine *sae = ctx->sae;
	int err;

	if (sae->buffer_cnt)
		return -EBUSY;

	sae->rctx = ctx;

	do {
		sae->buffer_cnt = min(dlen, sae->buffer_size);
		err = se_send_rng_cmd(se, sae, 5);
		if (err)
			return err;

		wait_for_completion_interruptible(&se->rng_completion);

		if (sae->cmd_ret)
			return -EFAULT;

		memcpy(dstn, sae->out_buffer, sae->buffer_cnt);
		dlen -= sae->buffer_cnt;
		dstn += sae->buffer_cnt;
	} while (dlen > 0);

	sae->buffer_cnt = 0;
	sae->rctx = NULL;

	return 0;
}

int loongson_se_rng_init(struct crypto_tfm *tfm)
{
	struct lsse_crypto *se = (struct lsse_crypto *)se_dev->driver_data;
	struct loongson_rng_ctx *ctx = crypto_tfm_ctx(tfm);

	ctx->sae = se_find_engine(se, SE_ALG_TYPE_RNG);
	if (!ctx->sae) {
		pr_err("%s Can not find TYPE %d engine\n",
				__func__, SE_ALG_TYPE_RNG);
		return -EFAULT;
	}

	return 0;
}

/*-------------------- ZUC functions -----------------------------------------*/
int loongson_zuc_enqueue(struct skcipher_request *req, unsigned int op)
{
	struct loongson_skcipher_ctx *ctx = crypto_skcipher_ctx(
					crypto_skcipher_reqtfm(req));
	struct loongson_skcipher_request_ctx *rctx = skcipher_request_ctx(req);
	struct se_alg_engine *sae = ctx->sae;

	rctx->op = op;

	return crypto_transfer_skcipher_request_to_engine(sae->engine, req);
}

static int se_send_zuc_cmd(struct lsse_crypto *se,
		struct se_alg_engine *sae, u32 op, int retry)
{
	struct se_alg_msg *alg_msg = (struct se_alg_msg *)sae->se_ch->smsg;
	dma_addr_t data_base = sae->se_ch->se->mem_addr;
	unsigned long flag;
	int err = 0;

	if (!sae->buffer_cnt)
		return 0;

	if (!alg_msg)
		return -EINVAL;

	spin_lock_irqsave(&sae->se_ch->ch_lock, flag);

	alg_msg->cmd = op;
	sae->cmd = alg_msg->cmd;

	/*
	 * Buffer_cnt should not greater than buffer_size,
	 * so it is safe to round up.
	 */
	alg_msg->u.req.len = round_up(sae->buffer_cnt, SE_ZUC_DATA_ALIGN);
	alg_msg->u.req.in_off = sae->in_addr - data_base;
	alg_msg->u.req.out_off = sae->out_addr - data_base;
	alg_msg->u.req.key_off = sae->key_addr - data_base;
	alg_msg->u.req.info_off = sae->info_addr - data_base;

try_again:
	pr_debug("ZUC CMD 0x%x info is 0x%x, key is 0x%x, data is 0x%x, data length is %d\n",
			alg_msg->cmd, alg_msg->u.req.info_off,
			alg_msg->u.req.key_off, alg_msg->u.req.in_off,
			alg_msg->u.req.len);

	if (!retry--)
		goto out;

	err = se_send_ch_request(sae->se_ch);
	if (err)
		goto try_again;

out:
	spin_unlock_irqrestore(&sae->se_ch->ch_lock, flag);
	return err;
}

int loongson_zuc_cipher_once(struct se_alg_engine *sae,
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

	sae->buffer_cnt = nbytes;

	return se_send_zuc_cmd(se, sae, op, 5);
}

int loongson_zuc_one_request(struct crypto_engine *engine, void *areq)
{
	struct skcipher_request *req = container_of(areq,
					struct skcipher_request, base);
	struct crypto_skcipher *cipher = crypto_skcipher_reqtfm(req);
	struct loongson_skcipher_ctx *ctx = crypto_skcipher_ctx(cipher);
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

	sae->rctx = rctx;
	rctx->req = req;
	rctx->src = req->src;
	rctx->dst = req->dst;
	rctx->in_nents = sg_nents_for_len(req->src, req->cryptlen);
	rctx->out_nents = sg_nents_for_len(req->dst, req->cryptlen);
	rctx->copyed_bytes = 0;
	rctx->rest_bytes = req->cryptlen;
	rctx->ivlen = ivlen;

	memcpy(sae->key_buffer, ctx->key, ctx->keylen);
	memcpy(sae->info_buffer, req->iv, ivlen);

	return loongson_zuc_cipher_once(sae, req, rctx->op);
}

int loongson_zuc_setkey(struct crypto_skcipher *cipher,
		const u8 *key, unsigned int len)
{
	struct crypto_tfm *tfm = crypto_skcipher_tfm(cipher);
	struct loongson_skcipher_ctx *ctx = crypto_tfm_ctx(tfm);

	if (unlikely(len > SE_ZUC_KEY_SIZE)) {
		pr_err("ZUC key length overflow!\n");
		return -EFAULT;
	}

	memcpy(ctx->key, key, len);
	ctx->keylen = len;

	return 0;
}

int loongson_ecb_zuc(struct skcipher_request *req)
{
	return loongson_zuc_enqueue(req, SE_CMD_ZUC_INIT_READ);
}

int loongson_se_zuc_init(struct crypto_tfm *tfm)
{
	struct loongson_skcipher_ctx *tfm_ctx = crypto_tfm_ctx(tfm);
	struct lsse_crypto *se = dev_get_drvdata(se_dev);

	crypto_skcipher_set_reqsize(__crypto_skcipher_cast(tfm),
				 sizeof(struct loongson_skcipher_request_ctx));
	memset(tfm_ctx, 0, sizeof(*tfm_ctx));

	tfm_ctx->sae = se_find_engine(se, SE_ALG_TYPE_ZUC);
	if (unlikely(!tfm_ctx->sae))
		return -ENODEV;

	return 0;
}

void loongson_se_zuc_exit(struct crypto_tfm *tfm)
{
	struct loongson_skcipher_ctx *tfm_ctx = crypto_tfm_ctx(tfm);
	struct loongson_skcipher_request_ctx *rctx;
	struct se_alg_engine *sae = tfm_ctx->sae;

	if (sae && sae->rctx)
		rctx = (struct loongson_skcipher_request_ctx *)sae->rctx;
	else
		return;

	if (rctx->req && rctx->req->base.tfm == tfm) {
		/* Last request blocked, remove it */
		crypto_finalize_skcipher_request(sae->engine, rctx->req, -EFAULT);
		sae->rctx = NULL;
		sae->cmd = 0;
		sae->cmd_ret = 0;
		sae->buffer_cnt = 0;
	}
}

static void loongson_finish_req_task(struct se_alg_engine *sae)
{
	struct lsse_crypto *se = (struct lsse_crypto *)se_dev->driver_data;

	tasklet_schedule(&se->task);
}

void loongson_se_finish_req(struct lsse_crypto *se)
{
	struct se_alg_engine *sae;
	struct se_alg_engine *next;

	list_for_each_entry_safe(sae, next, &se->finish_engine, finish_list) {
		switch (sae->type) {
		case SE_ALG_TYPE_SM3:
		case SE_ALG_TYPE_SM4:
		case SE_ALG_TYPE_ZUC:
			loongson_finish_req_task(sae);
			break;

		case SE_ALG_TYPE_RNG:
			complete(&se->rng_completion);
			list_del(&sae->finish_list);
			break;

		default:
			pr_err("%s Unrecognized ALG %d\n", __func__, sae->type);
			break;
		}
	}
}

// SPDX-License-Identifier: GPL-2.0
#include <crypto/engine.h>
#include <crypto/hmac.h>
#include <crypto/internal/skcipher.h>
#include <crypto/scatterwalk.h>
#include <crypto/sm4.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mfd/loongson-se.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>

#define LOONGSON_SM4_CTX_SIZE		64
#define LOONGSON_SM4_ALIGN_SIZE		64

#define LOONGSON_SKCIPHER_ENCRYPT	0
#define LOONGSON_SKCIPHER_DECRYPT	1

#define LOONGSON_SKCIPHER_CBC		0x2

struct loongson_skcipher_dev_list {
	struct mutex lock;
	struct list_head list;
	int registered;
};

struct loongson_skcipher_dev {
	struct loongson_se_engine *loongson_engine;
	struct crypto_engine *crypto_engine;
	struct list_head list;
	u32 used;
};

struct loongson_skcipher_ctx {
	struct loongson_skcipher_dev *sdev;
	u8 sm4_ctx[LOONGSON_SM4_CTX_SIZE];
};

struct loongson_skcipher_reqctx {
	int op;
};

struct loongson_skcipher_cmd {
	u32 cmd_id;
	union {
		u32 len;
		u32 ret;
	} u;
	u32 in_off;
	u32 out_off;
	u32 key_off;
	u32 iv_off;
	u32 pad[2];
};

static struct loongson_skcipher_dev_list skcipher_devices = {
	.lock = __MUTEX_INITIALIZER(skcipher_devices.lock),
	.list = LIST_HEAD_INIT(skcipher_devices.list),
};

static int loongson_sm4_setkey(struct crypto_skcipher *tfm, const u8 *key,
			       unsigned int keylen)
{
	struct loongson_skcipher_ctx *ctx = crypto_skcipher_ctx(tfm);

	if (keylen != SM4_KEY_SIZE)
		return -EINVAL;

	memcpy(ctx->sm4_ctx, key, keylen);

	return 0;
}

static int loongson_sm4_enqueue(struct skcipher_request *req, int op)
{
	struct loongson_skcipher_ctx *ctx = crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));
	struct loongson_skcipher_reqctx *rctx = skcipher_request_ctx(req);

	if (req->cryptlen % SM4_BLOCK_SIZE)
		return -EINVAL;

	rctx->op = op;

	return crypto_transfer_skcipher_request_to_engine(ctx->sdev->crypto_engine, req);
}

static int loongson_sm4_encrypt(struct skcipher_request *req)
{
	return loongson_sm4_enqueue(req, LOONGSON_SKCIPHER_ENCRYPT);
}

static int loongson_sm4_decrypt(struct skcipher_request *req)
{
	return loongson_sm4_enqueue(req, LOONGSON_SKCIPHER_DECRYPT);
}

static int loongson_sm4_encrypt_cbc(struct skcipher_request *req)
{
	return loongson_sm4_enqueue(req, LOONGSON_SKCIPHER_ENCRYPT | LOONGSON_SKCIPHER_CBC);
}

static int loongson_sm4_decrypt_cbc(struct skcipher_request *req)
{
	return loongson_sm4_enqueue(req, LOONGSON_SKCIPHER_DECRYPT | LOONGSON_SKCIPHER_CBC);
}

static int loongson_sm4_do_one_request(struct crypto_engine *engine, void *areq)
{
	struct skcipher_request *req = container_of(areq, struct skcipher_request, base);
	struct loongson_skcipher_ctx *ctx = crypto_skcipher_ctx(crypto_skcipher_reqtfm(req));
	struct loongson_skcipher_reqctx *rctx = skcipher_request_ctx(req);
	void *dma_buff = ctx->sdev->loongson_engine->data_buffer + LOONGSON_SM4_CTX_SIZE;
	u32 dma_buff_size = ctx->sdev->loongson_engine->buffer_size - LOONGSON_SM4_CTX_SIZE;
	struct loongson_skcipher_cmd *cmd;
	int err = 0, skip = 0, copyed = 0;

	if (rctx->op & LOONGSON_SKCIPHER_CBC)
		memcpy(ctx->sm4_ctx + SM4_KEY_SIZE, req->iv, SM4_BLOCK_SIZE);

	if (rctx->op == (LOONGSON_SKCIPHER_CBC | LOONGSON_SKCIPHER_DECRYPT))
		sg_pcopy_to_buffer(req->src, sg_nents(req->src), req->iv,
				   SM4_BLOCK_SIZE, req->cryptlen - SM4_BLOCK_SIZE);

	memcpy(ctx->sdev->loongson_engine->data_buffer, ctx->sm4_ctx, LOONGSON_SM4_CTX_SIZE);

	while (skip < req->cryptlen) {
		copyed = sg_pcopy_to_buffer(req->src, sg_nents(req->src),
					    dma_buff, min(dma_buff_size, req->cryptlen), skip);

		cmd = ctx->sdev->loongson_engine->command;
		cmd->cmd_id = SE_CMD_SKCIPHER | rctx->op;
		cmd->u.len = ALIGN(copyed, LOONGSON_SM4_ALIGN_SIZE);
		err = loongson_se_send_engine_cmd(ctx->sdev->loongson_engine);
		if (err)
			break;

		cmd = ctx->sdev->loongson_engine->command_ret;
		if (cmd->u.ret) {
			err = -EIO;
			break;
		}

		sg_pcopy_from_buffer(req->dst, sg_nents(req->dst), dma_buff, copyed, skip);

		skip += copyed;
	}

	if (rctx->op == (LOONGSON_SKCIPHER_CBC | LOONGSON_SKCIPHER_ENCRYPT))
		memcpy(req->iv, dma_buff + copyed - SM4_BLOCK_SIZE, SM4_BLOCK_SIZE);

	crypto_finalize_skcipher_request(ctx->sdev->crypto_engine, req, err);

	return err;
}

static int loongson_skcipher_init(struct crypto_tfm *tfm)
{
	struct loongson_skcipher_ctx *ctx = crypto_tfm_ctx(tfm);
	struct loongson_skcipher_dev *sdev;
	u32 min_used = U32_MAX;

	mutex_lock(&skcipher_devices.lock);
	list_for_each_entry(sdev, &skcipher_devices.list, list) {
		if (sdev->used < min_used) {
			ctx->sdev = sdev;
			min_used = sdev->used;
		}
	}
	ctx->sdev->used++;
	mutex_unlock(&skcipher_devices.lock);

	crypto_skcipher_set_reqsize(__crypto_skcipher_cast(tfm),
				    sizeof(struct loongson_skcipher_reqctx));

	return 0;
}

static void loongson_skcipher_exit(struct crypto_tfm *tfm)
{
	struct loongson_skcipher_ctx *ctx = crypto_tfm_ctx(tfm);

	mutex_lock(&skcipher_devices.lock);
	ctx->sdev->used--;
	mutex_unlock(&skcipher_devices.lock);
}

static struct skcipher_engine_alg loongson_sm4[] = {
	{
		.base = {
			.min_keysize	= SM4_KEY_SIZE,
			.max_keysize	= SM4_KEY_SIZE,
			.setkey		= loongson_sm4_setkey,
			.encrypt	= loongson_sm4_encrypt,
			.decrypt	= loongson_sm4_decrypt,
			.base = {
				.cra_name = "ecb(sm4)",
				.cra_driver_name = "loongson-ecb(sm4)",
				.cra_priority = 300,
				.cra_flags = CRYPTO_ALG_ASYNC,
				.cra_blocksize = SM4_BLOCK_SIZE,
				.cra_ctxsize = sizeof(struct loongson_skcipher_ctx),
				.cra_module = THIS_MODULE,
				.cra_init = loongson_skcipher_init,
				.cra_exit = loongson_skcipher_exit,
			},
		},
		.op.do_one_request = loongson_sm4_do_one_request,
	},
	{
		.base = {
			.min_keysize	= SM4_KEY_SIZE,
			.max_keysize	= SM4_KEY_SIZE,
			.ivsize		= SM4_BLOCK_SIZE,
			.setkey		= loongson_sm4_setkey,
			.encrypt	= loongson_sm4_encrypt_cbc,
			.decrypt	= loongson_sm4_decrypt_cbc,
			.base = {
				.cra_name = "cbc(sm4)",
				.cra_driver_name = "loongson-cbc(sm4)",
				.cra_priority = 300,
				.cra_flags = CRYPTO_ALG_ASYNC,
				.cra_blocksize = SM4_BLOCK_SIZE,
				.cra_ctxsize = sizeof(struct loongson_skcipher_ctx),
				.cra_module = THIS_MODULE,
				.cra_init = loongson_skcipher_init,
				.cra_exit = loongson_skcipher_exit,
			},
		},
		.op.do_one_request = loongson_sm4_do_one_request,
	},
};

static int loongson_skcipher_probe(struct platform_device *pdev)
{
	struct loongson_skcipher_cmd *cmd;
	struct loongson_skcipher_dev *sdev;
	int ret = 0;

	sdev = devm_kzalloc(&pdev->dev, sizeof(*sdev), GFP_KERNEL);
	if (!sdev)
		return -ENOMEM;

	sdev->loongson_engine = loongson_se_init_engine(pdev->dev.parent, SE_ENGINE_SKCIPHER);
	if (!sdev->loongson_engine)
		return -ENODEV;

	cmd = sdev->loongson_engine->command;
	cmd->key_off = sdev->loongson_engine->buffer_off;
	cmd->iv_off = sdev->loongson_engine->buffer_off + SM4_KEY_SIZE;
	cmd->in_off = sdev->loongson_engine->buffer_off + LOONGSON_SM4_CTX_SIZE;
	cmd->out_off = cmd->in_off;

	sdev->crypto_engine = crypto_engine_alloc_init(&pdev->dev, 1);
	crypto_engine_start(sdev->crypto_engine);

	mutex_lock(&skcipher_devices.lock);
	if (!skcipher_devices.registered) {
		skcipher_devices.registered = 1;
		list_add_tail(&sdev->list, &skcipher_devices.list);
		mutex_unlock(&skcipher_devices.lock);

		ret = crypto_engine_register_skciphers(loongson_sm4, ARRAY_SIZE(loongson_sm4));
		if (ret)
			dev_err(&pdev->dev, "failed to register crypto(%d)\n", ret);

		return ret;
	}

	list_add_tail(&sdev->list, &skcipher_devices.list);
	mutex_unlock(&skcipher_devices.lock);

	return ret;
}

static struct platform_driver loongson_skcipher_driver = {
	.probe		= loongson_skcipher_probe,
	.driver		= {
		.name   = "loongson-skcipher",
	},
};
module_platform_driver(loongson_skcipher_driver);

MODULE_ALIAS("platform:loongson-skcipher");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yinggang Gu <guyinggang@loongson.cn>");
MODULE_AUTHOR("Qunqin Zhao <zhaoqunqin@loongson.cn>");
MODULE_DESCRIPTION("Loongson skcipher acceleration engine driver");

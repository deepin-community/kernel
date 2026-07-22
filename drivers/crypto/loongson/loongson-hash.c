// SPDX-License-Identifier: GPL-2.0
#include <crypto/engine.h>
#include <crypto/hmac.h>
#include <crypto/internal/hash.h>
#include <crypto/scatterwalk.h>
#include <crypto/sm3.h>
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

#define LOONGSON_SM3_CTX_SIZE	64

#define LOONGSON_HASH_UPDATE	1
#define LOONGSON_HASH_FINAL	2

struct loongson_hash_dev_list {
	struct mutex lock;
	struct list_head list;
	int registered;
};

struct loongson_hash_dev {
	struct loongson_se_engine *loongson_engine;
	struct crypto_engine *crypto_engine;
	struct list_head list;
	u32 used;
};

struct loongson_hash_ctx {
	struct loongson_hash_dev *hdev;
	u8 sm3_ctx[LOONGSON_SM3_CTX_SIZE];
};

struct loongson_hash_reqctx {
	int op;
};

struct loongson_hash_cmd {
	u32 cmd_id;
	union {
		u32 len;
		u32 ret;
	} u;
	u32 block_off;
	u32 digest_off;
	u32 pad[4];
};

static struct loongson_hash_dev_list hash_devices = {
	.lock = __MUTEX_INITIALIZER(hash_devices.lock),
	.list = LIST_HEAD_INIT(hash_devices.list),
};

static int loongson_sm3_init(struct ahash_request *req)
{
	struct loongson_hash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));

	memset(ctx->sm3_ctx, 0, LOONGSON_SM3_CTX_SIZE);

	return 0;
}

static int loongson_sm3_update(struct ahash_request *req)
{
	struct loongson_hash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));
	struct loongson_hash_reqctx *rctx = ahash_request_ctx(req);

	rctx->op = LOONGSON_HASH_UPDATE;

	return crypto_transfer_hash_request_to_engine(ctx->hdev->crypto_engine, req);
}

static int loongson_sm3_final(struct ahash_request *req)
{
	struct loongson_hash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));
	struct loongson_hash_reqctx *rctx = ahash_request_ctx(req);

	rctx->op = LOONGSON_HASH_FINAL;

	return crypto_transfer_hash_request_to_engine(ctx->hdev->crypto_engine, req);
}

static int loongson_sm3_finup(struct ahash_request *req)
{
	struct loongson_hash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));
	struct loongson_hash_reqctx *rctx = ahash_request_ctx(req);

	rctx->op = LOONGSON_HASH_UPDATE | LOONGSON_HASH_FINAL;

	return crypto_transfer_hash_request_to_engine(ctx->hdev->crypto_engine, req);
}

static int loongson_sm3_do_update(struct ahash_request *req,
				  struct loongson_hash_ctx *ctx,
				  struct loongson_hash_reqctx *rctx)
{
	struct loongson_hash_cmd *cmd;
	void *dma_buff = ctx->hdev->loongson_engine->data_buffer + LOONGSON_SM3_CTX_SIZE;
	u32 dma_buff_size = ctx->hdev->loongson_engine->buffer_size - LOONGSON_SM3_CTX_SIZE;
	int err = 0, skip = 0, copyed;

	/* Import */
	memcpy(ctx->hdev->loongson_engine->data_buffer, ctx->sm3_ctx, LOONGSON_SM3_CTX_SIZE);

	while (skip < req->nbytes) {
		copyed = sg_pcopy_to_buffer(req->src, sg_nents(req->src),
					    dma_buff, min(dma_buff_size, req->nbytes), skip);

		cmd = ctx->hdev->loongson_engine->command;
		cmd->cmd_id = SE_CMD_HASH | LOONGSON_HASH_UPDATE;
		cmd->u.len = copyed;
		err = loongson_se_send_engine_cmd(ctx->hdev->loongson_engine);
		if (err)
			break;

		cmd = ctx->hdev->loongson_engine->command_ret;
		if (cmd->u.ret) {
			err = -EIO;
			break;
		}

		skip += copyed;
	}

	/* Export */
	memcpy(ctx->sm3_ctx, ctx->hdev->loongson_engine->data_buffer, LOONGSON_SM3_CTX_SIZE);

	return err;
}

static int loongson_sm3_do_final(struct ahash_request *req,
				 struct loongson_hash_ctx *ctx,
				  struct loongson_hash_reqctx *rctx)
{
	struct loongson_hash_cmd *cmd = ctx->hdev->loongson_engine->command;
	int err;

	cmd->cmd_id = SE_CMD_HASH | LOONGSON_HASH_FINAL;
	cmd->u.len = SM3_DIGEST_SIZE;
	err = loongson_se_send_engine_cmd(ctx->hdev->loongson_engine);
	if (err)
		goto out;

	cmd = ctx->hdev->loongson_engine->command_ret;
	if (cmd->u.ret)
		err = -EIO;

	memcpy(req->result, ctx->hdev->loongson_engine->data_buffer, SM3_DIGEST_SIZE);
	/* Init */
	memset(ctx->sm3_ctx, 0, LOONGSON_SM3_CTX_SIZE);
out:
	return err;
}

static int loongson_sm3_do_one_request(struct crypto_engine *engine, void *areq)
{
	struct ahash_request *req = container_of(areq, struct ahash_request, base);
	struct loongson_hash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));
	struct loongson_hash_reqctx *rctx = ahash_request_ctx(req);
	int err;

	if (rctx->op & LOONGSON_HASH_UPDATE) {
		err = loongson_sm3_do_update(req, ctx, rctx);
		if (err)
			goto out;
	}

	if (rctx->op & LOONGSON_HASH_FINAL)
		err = loongson_sm3_do_final(req, ctx, rctx);

out:
	crypto_finalize_hash_request(ctx->hdev->crypto_engine, req, err);

	return err;
}

static int loongson_sm3_export(struct ahash_request *req, void *out)
{
	struct loongson_hash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));

	memcpy(out, ctx->sm3_ctx, LOONGSON_SM3_CTX_SIZE);

	return 0;
}

static int loongson_sm3_import(struct ahash_request *req, const void *in)
{
	struct loongson_hash_ctx *ctx = crypto_ahash_ctx(crypto_ahash_reqtfm(req));

	memcpy(ctx->sm3_ctx, in, LOONGSON_SM3_CTX_SIZE);

	return 0;
}

static int loongson_ahash_init(struct crypto_tfm *tfm)
{
	struct loongson_hash_ctx *ctx = crypto_tfm_ctx(tfm);
	struct loongson_hash_dev *hdev;
	u32 min_used = U32_MAX;

	mutex_lock(&hash_devices.lock);
	list_for_each_entry(hdev, &hash_devices.list, list) {
		if (hdev->used < min_used) {
			ctx->hdev = hdev;
			min_used = hdev->used;
		}
	}
	ctx->hdev->used++;
	mutex_unlock(&hash_devices.lock);

	crypto_ahash_set_reqsize(__crypto_ahash_cast(tfm),
				 sizeof(struct loongson_hash_reqctx));

	return 0;
}

static void loongson_ahash_exit(struct crypto_tfm *tfm)
{
	struct loongson_hash_ctx *ctx = crypto_tfm_ctx(tfm);

	mutex_lock(&hash_devices.lock);
	ctx->hdev->used--;
	mutex_unlock(&hash_devices.lock);
}

static struct ahash_engine_alg loongson_sm3 = {
	.base = {
		.init = loongson_sm3_init,
		.update = loongson_sm3_update,
		.final = loongson_sm3_final,
		.finup = loongson_sm3_finup,
		.digest = loongson_sm3_finup,
		.export = loongson_sm3_export,
		.import = loongson_sm3_import,
		.halg.digestsize = SM3_DIGEST_SIZE,
		.halg.statesize = LOONGSON_SM3_CTX_SIZE,
		.halg.base = {
			.cra_name = "sm3",
			.cra_driver_name = "loongson-sm3",
			.cra_priority = 300,
			.cra_flags = CRYPTO_ALG_ASYNC,
			.cra_blocksize = SM3_BLOCK_SIZE,
			.cra_ctxsize = sizeof(struct loongson_hash_ctx),
			.cra_module = THIS_MODULE,
			.cra_init = loongson_ahash_init,
			.cra_exit = loongson_ahash_exit,
		},
	},
	.op.do_one_request = loongson_sm3_do_one_request,
};

static int loongson_hash_probe(struct platform_device *pdev)
{
	struct loongson_hash_cmd *cmd;
	struct loongson_hash_dev *hdev;
	int ret = 0;

	hdev = devm_kzalloc(&pdev->dev, sizeof(*hdev), GFP_KERNEL);
	if (!hdev)
		return -ENOMEM;

	hdev->loongson_engine = loongson_se_init_engine(pdev->dev.parent, SE_ENGINE_HASH);
	if (!hdev->loongson_engine)
		return -ENODEV;

	cmd = hdev->loongson_engine->command;
	cmd->digest_off = hdev->loongson_engine->buffer_off;
	cmd->block_off = hdev->loongson_engine->buffer_off + LOONGSON_SM3_CTX_SIZE;

	hdev->crypto_engine = crypto_engine_alloc_init(&pdev->dev, 1);
	crypto_engine_start(hdev->crypto_engine);

	mutex_lock(&hash_devices.lock);
	if (!hash_devices.registered) {
		hash_devices.registered = 1;
		list_add_tail(&hdev->list, &hash_devices.list);
		mutex_unlock(&hash_devices.lock);

		ret = crypto_engine_register_ahash(&loongson_sm3);
		if (ret)
			dev_err(&pdev->dev, "failed to register crypto(%d)\n", ret);

		return ret;
	}

	list_add_tail(&hdev->list, &hash_devices.list);
	mutex_unlock(&hash_devices.lock);

	return ret;
}

static struct platform_driver loongson_hash_driver = {
	.probe		= loongson_hash_probe,
	.driver		= {
		.name   = "loongson-hash",
	},
};
module_platform_driver(loongson_hash_driver);

MODULE_ALIAS("platform:loongson-hash");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yinggang Gu <guyinggang@loongson.cn>");
MODULE_AUTHOR("Qunqin Zhao <zhaoqunqin@loongson.cn>");
MODULE_DESCRIPTION("Loongson hash acceleration engine driver");

// SPDX-License-Identifier: GPL-2.0
/*
 *   Driver for loongson SE module using the kernel asynchronous crypto api.
 *
 *   Copyright 2023 Loongson Technology, Inc.
 */
#define pr_fmt(fmt)     KBUILD_MODNAME ": " fmt

#include "loongson_se_crypto.h"

/* The crypto framework makes it hard to avoid this global. */

static void loongson_se_update_irq(struct lsse_crypto *se,
		struct se_alg_engine *sae)
{
	struct se_alg_msg *alg_msg;

	if (!sae) {
		pr_info("Can not find Alg %d engine!\n", sae->type);
		return;
	}

	alg_msg = (struct se_alg_msg *)sae->se_ch->rmsg;
	if (!alg_msg) {
		pr_info("Can not find channel %d Message buffer!\n", sae->se_ch->id);
		return;
	}

	if (alg_msg->cmd == sae->cmd) {
		sae->cmd_ret = alg_msg->u.res.cmd_ret;
		list_add_tail(&sae->finish_list, &se->finish_engine);
	} else {
		pr_err("Channel %d response CMD is 0x%x, expect 0x%x!\n",
				sae->se_ch->id, alg_msg->cmd, sae->cmd);
	}

	memset(alg_msg, 0, sizeof(struct se_alg_msg));
}

static void lsse_crypto_complete(struct lsse_ch *ch)
{
	struct lsse_crypto *se = (struct lsse_crypto *)se_dev->driver_data;
	struct se_alg_engine *sae = ch->priv;

	if (!sae) {
		pr_err("Can not find sae in %s\n", __func__);
		return;
	}

	loongson_se_update_irq(se, sae);
	loongson_se_finish_req(se);
}

static struct se_alg_engine *loongson_se_init_sae(struct lsse_crypto *se,
		u32 type, bool need_engine)
{
	struct device *dev = se_dev;
	struct se_alg_engine *sae;
	int err;

	sae = se_find_engine(se, type);
	if (sae)
		return sae;

	sae = devm_kzalloc(dev, sizeof(struct se_alg_engine), GFP_KERNEL);
	if (!sae)
		return NULL;

	if (!need_engine)
		goto add_sae;

	sae->engine = crypto_engine_alloc_init(dev, 1);
	if (!sae->engine)
		return NULL;

	err = crypto_engine_start(sae->engine);
	if (err) {
		crypto_engine_exit(sae->engine);
		return NULL;
	}

add_sae:
	INIT_LIST_HEAD(&sae->wait_list);
	list_add_tail(&sae->engine_list, &se->alg_engine);
	sae->type = type;

	return sae;
}

static void loongson_se_free_sae(struct lsse_crypto *se)
{
	struct device *dev = se_dev;
	struct se_alg_engine *sae, *next;

	list_for_each_entry_safe(sae, next, &se->alg_engine, engine_list) {
		se_deinit_ch(sae->se_ch);
		if (sae->engine)
			crypto_engine_exit(sae->engine);
		list_del(&sae->engine_list);
		devm_kfree(dev, sae);
	}
}

static int loongson_se_register_alg(struct device *dev, struct lsse_crypto *se,
		struct loongson_alg_common *se_alg, unsigned int se_alg_num)
{
	struct se_alg_engine *sae;
	int err = 0;
	int msg_size, offset, i;

	msg_size = 2 * sizeof(struct se_alg_msg);

	for (i = 0; i < se_alg_num; i++) {
		sae = loongson_se_init_sae(se, se_alg[i].type,
				se_alg[i].need_engine);
		if (!sae)
			goto exit_engine;

		offset = 0;

		switch (se_alg[i].type) {
		case SE_ALG_TYPE_SM3:
			if (sae->se_ch) {
				err = crypto_engine_register_ahash(&se_alg[i].u.ahash);
				break;
			}

			sae->se_ch = se_init_ch(dev, SE_CH_SM3, se_alg[i].data_size, msg_size,
				sae, lsse_crypto_complete);
			if (!sae->se_ch) {
				crypto_engine_unregister_ahash(&se_alg[i].u.ahash);
				err = -ENODEV;
				break;
			}

			sae->in_buffer = sae->se_ch->data_buffer;
			sae->in_addr = sae->se_ch->data_addr;
			offset += se_alg[i].data_size - SM3_DIGEST_SIZE;

			sae->out_buffer = sae->in_buffer + offset;
			sae->out_addr = sae->in_addr + offset;
			sae->buffer_size = offset;

			err = crypto_engine_register_ahash(&se_alg[i].u.ahash);
			if (err)
				se_deinit_ch(sae->se_ch);

			break;

		case SE_ALG_TYPE_SM4:
			if (sae->se_ch) {
				err = crypto_engine_register_skcipher(&se_alg[i].u.skcipher);
				break;
			}

			sae->se_ch = se_init_ch(dev, SE_CH_SM4, se_alg[i].data_size, msg_size,
				sae, lsse_crypto_complete);
			if (!sae->se_ch) {
				err = -ENODEV;
				crypto_engine_unregister_skcipher(&se_alg[i].u.skcipher);
				break;
			}

			/* Inplace encrypt and decrypt */
			sae->in_buffer = sae->se_ch->data_buffer;
			sae->in_addr = sae->se_ch->data_addr;
			sae->out_buffer = sae->in_buffer;
			sae->out_addr = sae->in_addr;
			sae->buffer_size = se_alg[i].data_size - SM4_KEY_SIZE
								- SM4_BLOCK_SIZE - msg_size;
			sae->buffer_size &= ~(SE_SM4_DATA_ALIGN_MASK);
			offset += sae->buffer_size;

			sae->key_buffer = sae->in_buffer + offset;
			sae->key_addr = sae->in_addr + offset;
			offset += SM4_BLOCK_SIZE;
			sae->info_buffer = sae->in_buffer + offset;
			sae->info_addr = sae->in_addr + offset;

			err = crypto_engine_register_skcipher(&se_alg[i].u.skcipher);
			if (err)
				se_deinit_ch(sae->se_ch);

			break;

		case SE_ALG_TYPE_ZUC:
			if (sae->se_ch) {
				err = crypto_engine_register_skcipher(&se_alg[i].u.skcipher);
				break;
			}

			sae->se_ch = se_init_ch(dev, SE_CH_ZUC, se_alg[i].data_size, msg_size,
				sae, lsse_crypto_complete);
			if (!sae->se_ch) {
				err = -ENODEV;
				crypto_engine_unregister_skcipher(&se_alg[i].u.skcipher);
				break;
			}

			/* Inplace encrypt and decrypt */
			sae->in_buffer = sae->se_ch->data_buffer;
			sae->in_addr = sae->se_ch->data_addr;
			sae->out_buffer = sae->in_buffer;
			sae->out_addr = sae->in_addr;
			sae->buffer_size = se_alg[i].data_size - SE_ZUC_KEY_SIZE
								- SE_ZUC_IV_SIZE - msg_size;
			sae->buffer_size &= ~(SE_ZUC_DATA_ALIGN_MASK);
			offset += sae->buffer_size;

			sae->key_buffer = sae->in_buffer + offset;
			sae->key_addr = sae->in_addr + offset;
			offset += SE_ZUC_KEY_SIZE;

			sae->info_buffer = sae->in_buffer + offset;
			sae->info_addr = sae->in_addr + offset;

			err = crypto_engine_register_skcipher(&se_alg[i].u.skcipher);
			if (err)
				se_deinit_ch(sae->se_ch);

			break;

		case SE_ALG_TYPE_RNG:
			if (sae->se_ch) {
				err = crypto_register_rng(&se_alg[i].u.rng);
				break;
			}

			sae->se_ch = se_init_ch(dev, SE_CH_RNG, se_alg[i].data_size, msg_size,
				sae, lsse_crypto_complete);
			if (!sae->se_ch) {
				err = -ENODEV;
				break;
			}

			sae->out_buffer = sae->se_ch->data_buffer;
			sae->out_addr = sae->se_ch->data_addr;
			sae->buffer_size = se_alg[i].data_size;

			err = crypto_register_rng(&se_alg[i].u.rng);
			if (err)
				se_deinit_ch(sae->se_ch);

			break;

		default:
			pr_err("%s Unrecognized ALG %d\n", __func__, se_alg[i].type);
			err = -EINVAL;
			break;
		}

		if (err) {
			pr_err("Register type %d alg failed!\n",
				se_alg[i].type);
			goto alg_err;
		}

		continue;

alg_err:
exit_engine:
		if (err == -ENOMEM)
			break;
	}

	return err;
}

static void loongson_unregister_alg(struct lsse_crypto *se,
		struct loongson_alg_common *se_alg, unsigned int se_alg_num)
{
	int i;

	for (i = 0; i < se_alg_num; i++) {
		switch (se_alg[i].type) {
		case SE_ALG_TYPE_SM3:
			crypto_engine_unregister_ahash(&se_alg[i].u.ahash);
			break;

		case SE_ALG_TYPE_ZUC:
		case SE_ALG_TYPE_SM4:
			crypto_engine_unregister_skcipher(&se_alg[i].u.skcipher);
			break;

		case SE_ALG_TYPE_RNG:
			crypto_unregister_rng(&se_alg[i].u.rng);
			break;

		default:
			pr_err("%s Unrecognized ALG %d\n", __func__, se_alg[i].type);
			break;
		}
	}
}

static int lsse_crypto_probe(struct platform_device *pdev)
{
	struct lsse_crypto *se;
	struct device *dev = &pdev->dev;
	unsigned int version;
	int err;

	if (se_dev)
		return -ENODEV;

	se = devm_kzalloc(dev, sizeof(struct lsse_crypto),
			  GFP_KERNEL);
	if (!se)
		return -ENOMEM;

	se_dev = &pdev->dev;
	platform_set_drvdata(pdev, se);

	INIT_LIST_HEAD(&se->alg_engine);
	INIT_LIST_HEAD(&se->finish_engine);
	init_completion(&se->rng_completion);
	spin_lock_init(&se->cmd_lock);

	version = se_get_version(pdev->dev.parent);
	if (version == 0x10001) {
		tasklet_init(&se->task, loongson_se_task_routine_1_1, (unsigned long)se);
		err = loongson_se_register_alg(pdev->dev.parent, se, se_alg_1_1,
				ARRAY_SIZE(se_alg_1_1));
	/*
	 *} else if (version == 0x10002) {
	 *	tasklet_init(&se->task, loongson_se_task_routine_1_2, (unsigned long)se);
	 *	err = loongson_se_register_alg(pdev->dev.parent, se, se_alg_1_2,
	 *			ARRAY_SIZE(se_alg_1_2));
	 */
	} else {
		pr_info("Unknown version 0x%x\n", version);
		return -ENODEV;
	}
	return err;
}

static int lsse_crypto_remove(struct platform_device *pdev)
{
	unsigned int version;
	struct lsse_crypto *se = platform_get_drvdata(pdev);

	tasklet_kill(&se->task);
	version = se_get_version(pdev->dev.parent);
	if (version == 0x10001) {
		loongson_unregister_alg(se, se_alg_1_1, ARRAY_SIZE(se_alg_1_1));
	/*
	 *} else if (version == 0x10002) {
	 *	loongson_unregister_alg(se, se_alg_1_2, ARRAY_SIZE(se_alg_1_2));
	 */
	} else {
		pr_info("Unknown version 0x%x\n", version);
		return -ENODEV;
	}
	loongson_se_free_sae(se);

	return 0;
}

static struct platform_device_id lsse_crypto_ids[] = {
	{ .name = "loongson-se-crypto", },
	{}
};
MODULE_DEVICE_TABLE(platform, lsse_crypto_ids);

static struct platform_driver lsse_crypto_driver = {
	.probe   = lsse_crypto_probe,
	.remove  = lsse_crypto_remove,
	.id_table = lsse_crypto_ids,
	.driver  = {
		.name  = "loongson-se-crypto",
		.owner = THIS_MODULE,
	},
};
module_platform_driver(lsse_crypto_driver);

MODULE_AUTHOR("Yinggang Gu");
MODULE_DESCRIPTION("Loongson SE driver");
MODULE_LICENSE("GPL");

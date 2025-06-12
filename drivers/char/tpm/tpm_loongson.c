// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2025 Loongson Technology Corporation Limited. */

#include <linux/device.h>
#include <soc/loongson/se.h>
#include <linux/platform_device.h>
#include <linux/wait.h>

#include "tpm.h"

struct tpm_loongson_msg {
	u32 cmd;
	u32 data_off;
	u32 data_len;
	u32 info[5];
};

struct tpm_loongson_dev {
	struct lsse_ch *se_ch;
	struct completion tpm_loongson_completion;
};

static void tpm_loongson_complete(struct lsse_ch *ch)
{
	struct tpm_loongson_dev *td = ch->priv;

	complete(&td->tpm_loongson_completion);
}

static int tpm_loongson_recv(struct tpm_chip *chip, u8 *buf, size_t count)
{
	struct tpm_loongson_dev *td = dev_get_drvdata(&chip->dev);
	struct tpm_loongson_msg *rmsg;
	int sig;

	sig = wait_for_completion_interruptible(&td->tpm_loongson_completion);
	if (sig)
		return sig;

	rmsg = td->se_ch->rmsg;
	memcpy(buf, td->se_ch->data_buffer, rmsg->data_len);

	return rmsg->data_len;
}

static int tpm_loongson_send(struct tpm_chip *chip, u8 *buf, size_t count)
{
	struct tpm_loongson_dev *td = dev_get_drvdata(&chip->dev);
	struct tpm_loongson_msg *smsg = td->se_ch->smsg;

	memcpy(td->se_ch->data_buffer, buf, count);
	smsg->data_len = count;

	return se_send_ch_request(td->se_ch);
}

static const struct tpm_class_ops tpm_loongson_ops = {
	.flags = TPM_OPS_AUTO_STARTUP,
	.recv = tpm_loongson_recv,
	.send = tpm_loongson_send,
};

static int tpm_loongson_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct tpm_loongson_msg *smsg;
	struct tpm_loongson_dev *td;
	struct tpm_chip *chip;

	td = devm_kzalloc(dev, sizeof(struct tpm_loongson_dev), GFP_KERNEL);
	if (!td)
		return -ENOMEM;

	init_completion(&td->tpm_loongson_completion);
	td->se_ch = se_init_ch(dev->parent, SE_CH_TPM, PAGE_SIZE,
			       2 * sizeof(struct tpm_loongson_msg), td,
			       tpm_loongson_complete);
	if (!td->se_ch)
		return -ENODEV;
	smsg = td->se_ch->smsg;
	smsg->cmd = SE_CMD_TPM;
	smsg->data_off = td->se_ch->data_buffer - td->se_ch->se->mem_base;

	chip = tpmm_chip_alloc(dev, &tpm_loongson_ops);
	if (IS_ERR(chip))
		return PTR_ERR(chip);
	chip->flags = TPM_CHIP_FLAG_TPM2 | TPM_CHIP_FLAG_IRQ;
	dev_set_drvdata(&chip->dev, td);

	return tpm_chip_register(chip);
}

static struct platform_driver tpm_loongson_driver = {
	.probe   = tpm_loongson_probe,
	.driver  = {
		.name  = "loongson-tpm",
	},
};
module_platform_driver(tpm_loongson_driver);

MODULE_ALIAS("platform:loongson-tpm");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Loongson TPM driver");

// SPDX-License-Identifier: GPL-2.0
/*
 * The Hygon TPM2.0 device driver.
 *
 * Copyright (C) 2020 Hygon Info Technologies Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/err.h>
#include <linux/psp-hygon.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/tpm.h>
#include "tpm.h"

#define TPM2PSP_CMD(id)			(0x100 | (id))
#define MAX_TPM_BUF_LEN			4096
#define MAX_CMD_BUF_LEN			(MAX_TPM_BUF_LEN + sizeof(u32) + sizeof(u32))

struct tpm_hygon_priv {
	u8 priv_buf[MAX_CMD_BUF_LEN];
};

/*
 * tpm header struct name is different in different kernel versions.
 * so redefine it for driver porting.
 */
struct tpm_header_t {
	__be16	tag;
	__be32	length;
	union {
		__be32 ordinal;
		__be32 return_code;
	};
} __packed;

static int tpm_c_recv(struct tpm_chip *chip, u8 *buf, size_t count)
{
	int ret = 0;
	struct tpm_hygon_priv *priv = dev_get_drvdata(&chip->dev);
	struct tpm_header_t *header = (void *)(priv->priv_buf + sizeof(u32) + sizeof(u32));
	u32 len = be32_to_cpu(header->length);

	if (len > count) {
		ret = -E2BIG;
		goto out;
	}

	if (len > 0)
		memmove(buf, (u8 *)header, len);

	ret = len;

out:
	return ret;
}

static int tpm_c_send(struct tpm_chip *chip, u8 *buf, size_t count)
{
	int ret, error;
	struct tpm_hygon_priv *priv = dev_get_drvdata(&chip->dev);
	u32 buf_size = cpu_to_be32(sizeof(priv->priv_buf));
	u32 cmd_size = cpu_to_be32((u32)count);
	u8 *p = priv->priv_buf;

	*(u32 *)p = buf_size;
	p += sizeof(buf_size);
	*(u32 *)p = cmd_size;
	p += sizeof(cmd_size);
	memmove(p, buf, count);

	ret = psp_do_cmd(TPM2PSP_CMD(0), priv->priv_buf, &error);
	if (ret) {
		pr_err("%s: sev do cmd error, %d\n", __func__, error);
		ret = -EIO;
	}

	return ret;
}

static const struct tpm_class_ops tpm_c_ops = {
	.flags = TPM_OPS_AUTO_STARTUP,
	.recv = tpm_c_recv,
	.send = tpm_c_send,
};

static int hygon_tpm2_acpi_add(struct acpi_device *device)
{
	int ret;
	struct tpm_chip *chip;
	struct tpm_hygon_priv *priv;
	struct device *dev = &device->dev;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		goto err;
	}

	chip = tpmm_chip_alloc(dev, &tpm_c_ops);
	if (IS_ERR(chip)) {
		pr_err("tpmm_chip_alloc fail\n");
		ret = PTR_ERR(chip);
		goto err;
	}

	dev_set_drvdata(&chip->dev, priv);

	chip->flags |= TPM_CHIP_FLAG_TPM2;
	chip->flags |= TPM_CHIP_FLAG_IRQ;

	ret = tpm_chip_register(chip);
	if (ret) {
		pr_err("tpm_chip_register fail\n");
		goto err;
	}

	pr_info("Hygon TPM2 detected\n");

	return 0;

err:
	return ret;
}

static void hygon_tpm2_acpi_remove(struct acpi_device *device)
{
	struct device *dev = &device->dev;
	struct tpm_chip *chip = dev_get_drvdata(dev);

	tpm_chip_unregister(chip);

	pr_info("Hygon TPM2 removed\n");
}

static SIMPLE_DEV_PM_OPS(tpm_hygon_pm, tpm_pm_suspend, tpm_pm_resume);

static const struct acpi_device_id hygon_tpm2_device_ids[] = {
	{"HYGT0101", 0},
	{"", 0},
};

MODULE_DEVICE_TABLE(acpi, hygon_tpm2_device_ids);

static struct acpi_driver hygon_tpm2_acpi_driver = {
	.name = "tpm_hygon",
	.ids = hygon_tpm2_device_ids,
	.ops = {
		.add = hygon_tpm2_acpi_add,
		.remove = hygon_tpm2_acpi_remove,
	},
	.drv = {
		.pm = &tpm_hygon_pm,
	},
};

static int __init hygon_tpm2_init(void)
{
	return acpi_bus_register_driver(&hygon_tpm2_acpi_driver);
}

static void __exit hygon_tpm2_exit(void)
{
	acpi_bus_unregister_driver(&hygon_tpm2_acpi_driver);
}

/*
 * hygon_tpm2_init must be done after ccp module init, but before
 * ima module init. That's why we use a device_initcall_sync which is
 * called after all the device_initcall(includes ccp) but before the
 * late_initcall(includes ima).
 */
device_initcall_sync(hygon_tpm2_init);
module_exit(hygon_tpm2_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mayuanchen (mayuanchen@hygon.cn)");
MODULE_DESCRIPTION("TPM2 device driver for Hygon PSP");

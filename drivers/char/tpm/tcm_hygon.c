// SPDX-License-Identifier: GPL-2.0
/*
 * The Hygon TCM2.0 device driver.
 *
 * Copyright (C) 2023 Hygon Info Technologies Ltd.
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
#include <linux/security.h>
#include "tpm.h"

#define TCM2PSP_CMD(id)			(0x100 | (id))
#define MAX_TCM_BUF_LEN			4096

struct tcm_hygon_priv {
	u8 priv_buf[MAX_TCM_BUF_LEN];
};

struct tcm_header_t {
	__be16	tag;
	__be32	length;
	union {
		__be32 ordinal;
		__be32 return_code;
	};
} __packed;

static int tcm_c_recv(struct tpm_chip *chip, u8 *buf, size_t count)
{
	int ret = 0;
	struct tcm_hygon_priv *priv = dev_get_drvdata(&chip->dev);
	struct tcm_header_t *header = (void *)(priv->priv_buf + sizeof(u32) + sizeof(u32));
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

static int tcm_c_send(struct tpm_chip *chip, u8 *buf, size_t count)
{
	int ret, error;
	struct tcm_hygon_priv *priv = dev_get_drvdata(&chip->dev);
	u32 buf_size = sizeof(priv->priv_buf);
	u32 cmd_size = (u32)count;
	u8 *p = priv->priv_buf;

	if (buf_size - sizeof(u32) - sizeof(u32) < count) {
		ret = -E2BIG;
		goto out;
	}

	*(u32 *)p = cpu_to_be32(buf_size);
	p += sizeof(buf_size);
	*(u32 *)p = cpu_to_be32(cmd_size);
	p += sizeof(cmd_size);
	memmove(p, buf, count);

	ret = psp_do_cmd(TCM2PSP_CMD(0), priv->priv_buf, &error);
	if (ret) {
		pr_err("%s: psp do cmd error, %d\n", __func__, error);
		ret = -EIO;
	}

out:
	return ret;
}

static const struct tpm_class_ops tcm_c_ops = {
	.flags = TPM_OPS_AUTO_STARTUP,
	.recv = tcm_c_recv,
	.send = tcm_c_send,
};

static void tcm_bios_log_teardown(struct tpm_chip *chip)
{
	int i;
	struct inode *inode;

	/* securityfs_remove currently doesn't take care of handling sync
	 * between removal and opening of pseudo files. To handle this, a
	 * workaround is added by making i_private = NULL here during removal
	 * and to check it during open(), both within inode_lock()/unlock().
	 * This design ensures that open() either safely gets kref or fails.
	 */
	for (i = (TPM_NUM_EVENT_LOG_FILES - 1); i >= 0; i--) {
		if (chip->bios_dir[i]) {
			inode = d_inode(chip->bios_dir[i]);
			inode_lock(inode);
			inode->i_private = NULL;
			inode_unlock(inode);
			securityfs_remove(chip->bios_dir[i]);
		}
	}
}

static void tcm_chip_unregister(struct tpm_chip *chip)
{
	if (IS_ENABLED(CONFIG_HW_RANDOM_TPM))
		hwrng_unregister(&chip->hwrng);
	tcm_bios_log_teardown(chip);
	cdev_del(&chip->cdevs);
	put_device(&chip->devs);
	cdev_device_del(&chip->cdev, &chip->dev);
}

static int hygon_tcm2_acpi_add(struct acpi_device *device)
{
	int ret;
	struct tpm_chip *chip;
	struct tcm_hygon_priv *priv;
	struct device *dev = &device->dev;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		ret = -ENOMEM;
		goto err;
	}

	chip = tpmm_chip_alloc(dev, &tcm_c_ops);
	if (IS_ERR(chip)) {
		pr_err("tcmm_chip_alloc fail\n");
		ret = PTR_ERR(chip);
		goto err;
	}

	ret = dev_set_name(&chip->dev, "tcm%d", chip->dev_num);
	if (ret) {
		pr_err("tcm device set name fail\n");
		goto err;
	}

	dev_set_drvdata(&chip->dev, priv);

	chip->flags |= TPM_CHIP_FLAG_TPM2;
	chip->flags |= TPM_CHIP_FLAG_IRQ;

	ret = tpm_chip_register(chip);
	if (ret) {
		pr_err("tcm chip_register fail\n");
		goto err;
	}

	if (chip->flags & TPM_CHIP_FLAG_TPM2) {
		device_del(&chip->devs);
		ret = dev_set_name(&chip->devs, "tcmrm%d", chip->dev_num);
		if (ret) {
			pr_err("tcmrm device set name fail\n");
			goto err_dev;
		}
		ret = device_add(&chip->devs);
		if (ret) {
			pr_err("devs add fail\n");
			goto err_dev;
		}
	}

	pr_info("Hygon TCM2 detected\n");

	return 0;

err_dev:
	tcm_chip_unregister(chip);

err:
	return ret;
}

static void hygon_tcm2_acpi_remove(struct acpi_device *device)
{
	struct device *dev = &device->dev;
	struct tpm_chip *chip = dev_get_drvdata(dev);

	tpm_chip_unregister(chip);

	pr_info("Hygon TCM2 removed\n");
}

static SIMPLE_DEV_PM_OPS(tcm_hygon_pm, tpm_pm_suspend, tpm_pm_resume);

static const struct acpi_device_id hygon_tcm2_device_ids[] = {
	{"HYGT0201", 0},
	{"", 0},
};

MODULE_DEVICE_TABLE(acpi, hygon_tcm2_device_ids);

static struct acpi_driver hygon_tcm2_acpi_driver = {
	.name = "tcm_hygon",
	.ids = hygon_tcm2_device_ids,
	.ops = {
		.add = hygon_tcm2_acpi_add,
		.remove = hygon_tcm2_acpi_remove,
	},
	.drv = {
		.pm = &tcm_hygon_pm,
	},
};

static int __init hygon_tcm2_init(void)
{
	return acpi_bus_register_driver(&hygon_tcm2_acpi_driver);
}

static void __exit hygon_tcm2_exit(void)
{
	acpi_bus_unregister_driver(&hygon_tcm2_acpi_driver);
}

/*
 * hygon_tcm2_init must be done after ccp module init, but before
 * ima module init. That's why we use a device_initcall_sync which is
 * called after all the device_initcall(includes ccp) but before the
 * late_initcall(includes ima).
 */
device_initcall_sync(hygon_tcm2_init);
module_exit(hygon_tcm2_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("mayuanchen (mayuanchen@hygon.cn)");
MODULE_DESCRIPTION("TCM2 device driver for Hygon PSP");

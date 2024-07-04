// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Zhaoxin SMBus controller driver
 *
 * Copyright(c) 2023 Shanghai Zhaoxin Semiconductor Corporation.
 * All rights reserved.
 */

#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define DRIVER_VERSION "3.1.0"

#define ZXSMB_NAME "smbus_zhaoxin"

/*
 * registers
 */
/* SMBus MMIO address offsets */
#define ZXSMB_STS		0x00
#define   ZXSMB_BUSY		BIT(0)
#define   ZXSMB_CMD_CMPLET	BIT(1)
#define   ZXSMB_DEV_ERR		BIT(2)
#define   ZXSMB_BUS_CLSI	BIT(3)
#define   ZXSMB_FAIL_TRANS	BIT(4)
#define   ZXSMB_STS_MASK 		GENMASK(4, 0)
#define   ZXSMB_NSMBSRST	BIT(5)
#define ZXSMB_CTL		0x02
#define   ZXSMB_CMPLT_EN	BIT(0)
#define   ZXSMB_KILL_PRG	BIT(1)
#define   ZXSMB_START		BIT(6)
#define   ZXSMB_PEC_EN		BIT(7)
#define ZXSMB_CMD		0x03
#define ZXSMB_ADD		0x04
#define ZXSMB_DAT0		0x05
#define ZXSMB_DAT1		0x06
#define ZXSMB_BLKDAT	0x07

/*
 * platform related informations
 */
 /* protocol cmd constants */
#define ZXSMB_QUICK				0x00
#define ZXSMB_BYTE				0x04
#define ZXSMB_BYTE_DATA			0x08
#define ZXSMB_WORD_DATA			0x0C
#define ZXSMB_PROC_CALL			0x10
#define ZXSMB_BLOCK_DATA		0x14
#define ZXSMB_I2C_10_BIT_ADDR	0x18
#define ZXSMB_I2C_PROC_CALL		0x30
#define ZXSMB_I2C_BLOCK_DATA	0x34
#define ZXSMB_I2C_7_BIT_ADDR	0x38
#define ZXSMB_UNIVERSAL			0x3C

#define ZXSMB_TIMEOUT 500

struct zxsmb {
	struct device *dev;
	struct i2c_adapter adap;
	struct completion complete;
	u16 base;
	int irq;
	u8 status;
	int size;
	u8 pec;
};

static irqreturn_t zxsmb_irq_handle(int irq, void *dev_id)
{
	struct zxsmb *smb = (struct zxsmb *)dev_id;

	smb->status = inb(smb->base + ZXSMB_STS);
	if ((smb->status & ZXSMB_STS_MASK) == 0)
		return IRQ_NONE;

	/* clear status */
	outb(smb->status, smb->base + ZXSMB_STS);
	complete(&smb->complete);

	return IRQ_HANDLED;
}

static int zxsmb_status_check(struct zxsmb *smb)
{
	if (smb->status & ZXSMB_CMD_CMPLET)
		return 0;

	if (smb->status & ZXSMB_BUS_CLSI) {
		dev_err(smb->dev, "Lost arbitration\n");
		outb(ZXSMB_KILL_PRG, smb->base + ZXSMB_CTL);
		return -EAGAIN;
	}

	dev_dbg(smb->dev, "Trans failed, status = 0x%X\n", smb->status);

	return -EIO;
}

static int zxsmb_wait_interrput_finish(struct zxsmb *smb)
{
	int time_left;

	time_left = wait_for_completion_timeout(&smb->complete, msecs_to_jiffies(ZXSMB_TIMEOUT));
	if (time_left == 0) {
		u8 status = inb(smb->base + ZXSMB_STS);

		/* some host's irq config not work well */
		if (status & ZXSMB_STS_MASK) {
			outb(status, smb->base + ZXSMB_STS);
			outb(ZXSMB_KILL_PRG, smb->base + ZXSMB_CTL);
			devm_free_irq(smb->dev, smb->irq, smb);
			smb->irq = 0;
			dev_warn(smb->dev, "change to polling mode\n");

			return -EAGAIN;
		}
		dev_dbg(smb->dev, "interrput timeout\n");
		return -EIO;
	}

	return zxsmb_status_check(smb);
}

static int zxsmb_wait_polling_finish(struct zxsmb *smb)
{
	int status;
	int time_left = ZXSMB_TIMEOUT * 10;

	do {
		usleep_range(100, 200);
		status = inb(smb->base + ZXSMB_STS);
	} while ((status & ZXSMB_BUSY) && (--time_left));

	if (time_left == 0) {
		dev_dbg(smb->dev, "polling timeout\n");
		return -EIO;
	}

	/* clear status */
	outb(status, smb->base + ZXSMB_STS);
	smb->status = status;

	return zxsmb_status_check(smb);
}

static int zxsmb_trans_start(struct zxsmb *smb)
{
	u16 base = smb->base;
	int tmp;

	/* Make sure the SMBus host is ready to start transmitting */
	if ((tmp = inb(base + ZXSMB_STS)) & ZXSMB_BUSY) {
		outb(tmp, base + ZXSMB_STS);
		usleep_range(1000, 5000);
		if ((tmp = inb(base + ZXSMB_STS)) & ZXSMB_BUSY) {
			dev_err(smb->dev, "SMBus reset failed! (0x%02x)\n", tmp);
			return -EIO;
		}
	}

	tmp = ZXSMB_START | smb->size;

	if (smb->pec)
		tmp |= ZXSMB_PEC_EN;
	else
		tmp &= (~ZXSMB_PEC_EN);

	if (smb->irq)
		tmp |= ZXSMB_CMPLT_EN;

	reinit_completion(&smb->complete);
	smb->status = 0;
	outb(tmp, base + ZXSMB_CTL);
	return 0;
}

static int zxsmb_transaction(struct zxsmb *smb)
{
	int err;

	err = zxsmb_trans_start(smb);
	if (err)
		return err;

	if (smb->irq)
		err = zxsmb_wait_interrput_finish(smb);
	else
		err = zxsmb_wait_polling_finish(smb);

	outb(0, smb->base + ZXSMB_CTL);
	return err;
}

static int zxsmb_smbus_xfer(struct i2c_adapter *adap, u16 addr, u16 flags, char read, u8 command,
				int size, union i2c_smbus_data *data)
{
	int i;
	int err;
	u8 len;
	struct zxsmb *smb = (struct zxsmb *)i2c_get_adapdata(adap);
	u16 base = smb->base;

	switch (size) {
	case I2C_SMBUS_QUICK:
		size = ZXSMB_QUICK;
		break;
	case I2C_SMBUS_BYTE:
		size = ZXSMB_BYTE;
		if (!read)
			outb(command, base + ZXSMB_CMD);
		break;
	case I2C_SMBUS_BYTE_DATA:
		outb(command, base + ZXSMB_CMD);
		if (!read)
			outb(data->byte, base + ZXSMB_DAT0);
		size = ZXSMB_BYTE_DATA;
		break;
	case I2C_SMBUS_PROC_CALL:
	case I2C_SMBUS_WORD_DATA:
		if (read && size == I2C_SMBUS_PROC_CALL)
			goto exit_unsupported;
		outb(command, base + ZXSMB_CMD);
		if (!read) {
			outb(data->word & 0xff, base + ZXSMB_DAT0);
			outb((data->word & 0xff00) >> 8, base + ZXSMB_DAT1);
		}
		size = (size == I2C_SMBUS_PROC_CALL) ?
			ZXSMB_PROC_CALL : ZXSMB_WORD_DATA;
		break;
	case I2C_SMBUS_I2C_BLOCK_DATA:
	case I2C_SMBUS_BLOCK_DATA:
		len = data->block[0];
		if (read && size == I2C_SMBUS_I2C_BLOCK_DATA)
			outb(len, base + ZXSMB_DAT1);
		outb(command, base + ZXSMB_CMD);
		/* Reset ZXSMB_BLKDAT */
		inb(base + ZXSMB_CTL);
		if (!read) {
			outb(len, base + ZXSMB_DAT0);
			outb(0, base + ZXSMB_DAT1);
			for (i = 1; i <= len; i++)
				outb(data->block[i], base + ZXSMB_BLKDAT);
		}
		size = (size == I2C_SMBUS_I2C_BLOCK_DATA) ?
			ZXSMB_I2C_BLOCK_DATA : ZXSMB_BLOCK_DATA;
		break;
	default:
		goto exit_unsupported;
	}

	outb(((addr & 0x7f) << 1) | read, base + ZXSMB_ADD);
	smb->size = size;
	smb->pec = flags & I2C_CLIENT_PEC;
	err = zxsmb_transaction(smb);
	if (err)
		return err;

	if ((read == I2C_SMBUS_WRITE) || (size == ZXSMB_QUICK)) {
		if (unlikely(size == ZXSMB_PROC_CALL))
			goto prepare_read;
		return 0;
	}

prepare_read:
	switch (size) {
	case ZXSMB_BYTE:
	case ZXSMB_BYTE_DATA:
		data->byte = inb(base + ZXSMB_DAT0);
		break;
	case ZXSMB_PROC_CALL:
	case ZXSMB_WORD_DATA:
		data->word = inb(base + ZXSMB_DAT0) + (inb(base + ZXSMB_DAT1) << 8);
		break;
	case ZXSMB_I2C_BLOCK_DATA:
	case ZXSMB_BLOCK_DATA:
		data->block[0] = inb(base + ZXSMB_DAT0);
		if (data->block[0] > I2C_SMBUS_BLOCK_MAX)
			data->block[0] = I2C_SMBUS_BLOCK_MAX;
		/* Reset ZXSMB_BLKDAT */
		inb(base + ZXSMB_CTL);
		for (i = 1; i <= data->block[0]; i++)
			data->block[i] = inb(base + ZXSMB_BLKDAT);
		break;
	}

	return 0;

exit_unsupported:
	dev_err(smb->dev, "unsupported access, size:%x, dir:%s", size, read ? "read" : "write");
	return -EOPNOTSUPP;
}

static u32 zxsmb_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm smbus_algorithm = {
	.smbus_xfer = zxsmb_smbus_xfer,
	.functionality  = zxsmb_func,
};

static int zxsmb_probe(struct platform_device *pdev)
{
	struct zxsmb *smb;
	struct resource *res;
	struct i2c_adapter *adap;

	smb = devm_kzalloc(&pdev->dev, sizeof(*smb), GFP_KERNEL);
	if (!smb)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (IS_ERR(res))
		return -ENODEV;
	smb->base = res->start;
	if (!devm_request_region(&pdev->dev, res->start, resource_size(res), pdev->name)) {
		dev_err(&pdev->dev, "Can't get I/O resource\n");
		return -EBUSY;
	}

	smb->irq = platform_get_irq(pdev, 0);
	if (smb->irq < 0 || devm_request_irq(&pdev->dev, smb->irq, zxsmb_irq_handle, IRQF_SHARED,
			pdev->name, smb)) {
		dev_warn(&pdev->dev, "failed to request irq %d\n", smb->irq);
		smb->irq = 0;
	} else
		init_completion(&smb->complete);

	smb->dev = &pdev->dev;
	platform_set_drvdata(pdev, (void *)smb);

	adap = &smb->adap;
	adap->algo = &smbus_algorithm;
	adap->retries = 2;
	adap->owner = THIS_MODULE;
	adap->dev.parent = &pdev->dev;
	ACPI_COMPANION_SET(&adap->dev, ACPI_COMPANION(&pdev->dev));
	snprintf(adap->name, sizeof(adap->name), "zhaoxin-%s-%s", dev_name(pdev->dev.parent),
		dev_name(smb->dev));
	i2c_set_adapdata(&smb->adap, smb);

	return i2c_add_adapter(&smb->adap);
}

static int zxsmb_remove(struct platform_device *pdev)
{
	struct zxsmb *smb = platform_get_drvdata(pdev);

	i2c_del_adapter(&(smb->adap));
	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, smb);

	return 0;
}

static const struct acpi_device_id zxsmb_acpi_match[] = {
	{"SMB3324", 0 },
	{ }
};
MODULE_DEVICE_TABLE(acpi, zxsmb_acpi_match);

static struct platform_driver zxsmb_driver = {
	.probe  = zxsmb_probe,
	.remove = zxsmb_remove,
	.driver = {
		.name = ZXSMB_NAME,
		.acpi_match_table = ACPI_PTR(zxsmb_acpi_match),
	},
};

module_platform_driver(zxsmb_driver);

MODULE_AUTHOR("hanshu@zhaoxin.com");
MODULE_DESCRIPTION("Zhaoxin SMBus driver");
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPL");

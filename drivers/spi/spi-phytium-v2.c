// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium SPI core controller driver.
 *
 * Copyright (c) 2023-2024, Phytium Technology Co., Ltd..
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/highmem.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/property.h>
#include <linux/acpi.h>
#include <linux/mtd/spi-nor.h>
#include "spi-phytium.h"

static inline void spi_phyt_enable_chip(struct phytium_spi *fts, u8 enable)
{
	u8 val = enable ? 1 : 2;

	spi_phytium_set_cmd8(fts, PHYTSPI_MSG_CMD_SET_MODULE_EN, val);
}

static inline void spi_phyt_set_clk(struct phytium_spi *fts, u16 div)
{
	u32 new_div = div;

	spi_phytium_set_cmd32(fts, PHYTSPI_MSG_CMD_SET_BAUDR, new_div);
}

static inline void spi_phyt_mask_intr(struct phytium_spi *fts, u8 enable)
{
	spi_phytium_set_cmd8(fts, PHYTSPI_MSG_CMD_SET_INTERRUPT, enable);
}

static inline void spi_phyt_global_cs(struct phytium_spi *fts)
{
	u32 global_cs_en;
	u16 cs;

	global_cs_en = GENMASK(fts->num_cs-1, 0) << fts->num_cs;

	cs = (u16)((0x1 << 8) | global_cs_en);
	spi_phytium_set_cmd16(fts, PHYTSPI_MSG_CMD_SET_CS, cs);
}

static inline void spi_phyt_reset_chip(struct phytium_spi *fts)
{
	spi_phyt_enable_chip(fts, 0);
	if (fts->global_cs)
		spi_phyt_global_cs(fts);
	spi_phyt_mask_intr(fts, 0);
	spi_phyt_enable_chip(fts, 1);
}

static inline void spi_phyt_shutdown_chip(struct phytium_spi *fts)
{
	spi_phyt_enable_chip(fts, 0);
	spi_phyt_set_clk(fts, 0);
}

struct phytium_spi_chip {
	u8 poll_mode;
	u8 type;
	void (*cs_control)(u32 command);
};

struct chip_data {
	u8 cs;
	u8 tmode;
	u8 type;

	u8 poll_mode;

	u16 clk_div;
	u32 speed_hz;
	void (*cs_control)(u32 command);
};

static void spi_phyt_set_cs(struct spi_device *spi, bool enable)
{
	struct phytium_spi *fts = spi_master_get_devdata(spi->master);
	struct chip_data *chip = spi_get_ctldata(spi);
	u32 origin;
	u16 cs;

	if (fts->tx || fts->rx)
		return;

	if (fts->msg->cmd_id == PHYTSPI_MSG_CMD_DATA &&
			fts->msg->cmd_subid == PHYTSPI_MSG_CMD_DATA_TX)
		return;

	if (chip && chip->cs_control)
		chip->cs_control(!enable);

	if (!enable) {
		cs = BIT(spi->chip_select);
		spi_phytium_set_cmd16(fts, PHYTSPI_MSG_CMD_SET_CS, cs);
		if (fts->global_cs) {
			origin = (GENMASK(fts->num_cs-1, 0) << fts->num_cs)
				| (1 << spi->chip_select);
			cs = (0x1 << 8) | origin;
			spi_phytium_set_cmd16(fts, PHYTSPI_MSG_CMD_SET_CS, cs);
		}
	} else {
		if (fts->global_cs) {
			origin = (GENMASK(fts->num_cs-1, 0) << fts->num_cs)
				& ~(1 << spi->chip_select);
			cs = (0x1 << 8) | origin;
			spi_phytium_set_cmd16(fts, PHYTSPI_MSG_CMD_SET_CS, cs);
		}
	}
}

static irqreturn_t spi_phyt_irq(int irq, void *dev_id)
{
	struct spi_master *master = dev_id;
	struct phytium_spi *fts = spi_master_get_devdata(master);

	complete(&fts->cmd_completion);
	writel_relaxed(0, fts->regfile + SPI_REGFILE_RV2AP_INTR_STATE);
	writel_relaxed(0x10, fts->regfile + SPI_REGFILE_RV2AP_INT_CLEAN);

	return IRQ_HANDLED;
}

static int spi_phyt_transfer_one(struct spi_master *master,
		struct spi_device *spi, struct spi_transfer *transfer)
{
	struct phytium_spi *fts = spi_master_get_devdata(master);
	struct chip_data *chip = spi_get_ctldata(spi);
	int ret;

	fts->tx = (void *)transfer->tx_buf;
	fts->tx_end = fts->tx + transfer->len;
	fts->rx = transfer->rx_buf;
	fts->rx_end = fts->rx + transfer->len;
	fts->len = transfer->len;

	if (chip->cs_control) {
		if (fts->rx && fts->tx)
			chip->tmode = TMOD_TR;
		else if (fts->rx)
			chip->tmode = TMOD_RO;
		else
			chip->tmode = TMOD_TO;
	}

	if (fts->tx) {
		if ((*(u8 *)fts->tx == SPINOR_OP_WREN) && fts->spi_write_flag == 0) {
			spi_phytium_write_pre(fts, spi->chip_select,
					transfer->bits_per_word, spi->mode,
					chip->tmode, 3, fts->spi_write_flag);
			fts->spi_write_flag++;
			return 0;
		}

		if ((*(u8 *)fts->tx == SPINOR_OP_BE_4K) && (fts->spi_write_flag == 1) &&
				fts->flash_read == 0 && fts->flash_erase == 0) {
			fts->spi_write_flag++;
			fts->flash_erase = 1;
			return 0;
		}

		if ((*(u8 *)fts->tx == SPINOR_OP_CHIP_ERASE) && (fts->spi_write_flag == 1) &&
				fts->flash_read == 0 && fts->flash_erase == 0) {
			ret = spi_phytium_flash_erase(fts, spi->chip_select,
					transfer->bits_per_word,
					spi->mode, chip->tmode, 3, SPINOR_OP_CHIP_ERASE);
			fts->spi_write_flag = 0;
			fts->flash_erase = 2;
		}

		if ((*(u8 *)fts->tx == SPINOR_OP_READ || *(u8 *)fts->tx == SPINOR_OP_READ_FAST ||
				*(u8 *)fts->tx == SPINOR_OP_READ_4B ||
				*(u8 *)fts->tx == SPINOR_OP_READ_FAST_4B) &&
				fts->spi_write_flag == 0 && fts->flash_read == 0 &&
				fts->flash_erase == 0) {
			fts->flash_cmd = *(u8 *)fts->tx;
			fts->spi_write_flag++;
			fts->flash_read = 1;
			return 0;
		}

		if ((fts->spi_write_flag == 1) && fts->flash_read == 0 &&
				fts->flash_write == 0 && ((*(u8 *)fts->tx == SPINOR_OP_PP) ||
				(*(u8 *)fts->tx == SPINOR_OP_PP_4B))) {
			fts->flash_cmd = *(u8 *)fts->tx;
			fts->spi_write_flag++;
			fts->flash_write = 1;
			return 0;
		}

		if ((*(u8 *)fts->tx == SPINOR_OP_RDSR) && fts->flash_write == 3) {
			fts->read_sr = 1;
			fts->flash_write = 0;
			return 0;
		}

		if ((*(u8 *)fts->tx == SPINOR_OP_RDSR) && fts->flash_erase == 2) {
			fts->read_sr = 1;
			return 0;
		}
	}

	if (fts->read_sr) {
		*(u8 *)(fts->rx) = 0;
		fts->read_sr = 0;
		return 0;
	}

	if (fts->tx) {
		if (fts->flash_erase == 1) {
			ret = spi_phytium_flash_erase(fts, spi->chip_select,
					transfer->bits_per_word,
					spi->mode, chip->tmode, 3, SPINOR_OP_BE_4K);
			if (ret) {
				dev_err(&master->dev, "flash erase failed\n");
				return ret;
			}
			fts->spi_write_flag = 0;
			fts->flash_erase++;
		} else if (fts->flash_read) {
			ret = spi_phytium_flash_erase(fts, spi->chip_select,
					transfer->bits_per_word,
					spi->mode, chip->tmode, 1, fts->flash_cmd);
			if (ret) {
				dev_err(&master->dev, "transfer read-command failed\n");
				return ret;
			}
			fts->spi_write_flag = 0;
			fts->flash_read = 0;
		} else if (fts->flash_write == 1) {
			fts->flash_write++;
			ret = spi_phytium_flash_write(fts, fts->flash_cmd);
			if (ret) {
				dev_err(&master->dev, "flash write failed\n");
				return ret;
			}
		} else if (fts->flash_erase == 2 && (*(u8 *)fts->tx == SPINOR_OP_WRDI)) {
			ret = spi_phytium_write(fts, spi->chip_select, transfer->bits_per_word,
					spi->mode, chip->tmode, 3, fts->spi_write_flag);
			if (ret) {
				dev_err(&master->dev, "transfer disable-command failed\n");
				return ret;
			}
			fts->flash_erase = 0;
		} else {
			ret = spi_phytium_write(fts, spi->chip_select, transfer->bits_per_word,
					spi->mode, chip->tmode, 1, fts->spi_write_flag);
			if (ret) {
				dev_err(&master->dev, "write command failed\n");
				return ret;
			}
			if (fts->flash_write == 2)
				fts->flash_write++;
			fts->spi_write_flag = 0;
		}
	}

	if (fts->rx) {
		ret = spi_phytium_read(fts, spi->chip_select, transfer->bits_per_word,
				spi->mode, chip->tmode, 2);
		if (ret) {
			dev_err(&master->dev, "read data failed\n");
			return ret;
		}
	}

	return ret;
}

static void spi_phyt_handle_err(struct spi_master *master,
		struct spi_message *msg)
{
	struct phytium_spi *fts = spi_master_get_devdata(master);

	spi_phyt_reset_chip(fts);
}

static int spi_phyt_setup(struct spi_device *spi)
{
	struct phytium_spi_chip *chip_info = NULL;
	struct chip_data *chip;
	struct spi_master *master = spi->master;
	struct phytium_spi *fts = spi_master_get_devdata(master);
	u8 data_width, scph, scpol, tmode;
	u16 mode;
	u16 clk_div;

	spi_phyt_enable_chip(fts, 0);

	clk_div = (fts->max_freq / spi->max_speed_hz + 1) & 0xfffe;
	spi_phyt_set_clk(fts, clk_div);
	fts->clk_div = clk_div;

	chip = spi_get_ctldata(spi);
	if (!chip) {
		chip = kzalloc(sizeof(struct chip_data), GFP_KERNEL);
		if (!chip)
			return -ENOMEM;
		spi_set_ctldata(spi, chip);
	}

	chip_info = spi->controller_data;

	if (chip_info) {
		if (chip_info->cs_control)
			chip->cs_control = chip_info->cs_control;

		chip->poll_mode = chip_info->poll_mode;
		chip->type = chip_info->type;
	}

	chip->tmode = 0;

	data_width = spi->bits_per_word;
	spi_phytium_set_cmd8(fts, PHYTSPI_MSG_CMD_SET_DATA_WIDTH, data_width);

	scph = spi->mode & (0x1);
	scpol = spi->mode >> 1;
	mode = (scph << 8) | scpol;
	spi_phytium_set_cmd16(fts, PHYTSPI_MSG_CMD_SET_MODE, mode);

	tmode = chip->tmode;
	spi_phytium_set_cmd8(fts, PHYTSPI_MSG_CMD_SET_TMOD, tmode);

	spi_phyt_enable_chip(fts, 1);

	return 0;
}

static void spi_phyt_cleanup(struct spi_device *spi)
{
	struct chip_data *chip = spi_get_ctldata(spi);

	kfree(chip);
	spi_set_ctldata(spi, NULL);
}

void spi_phyt_enable_debug(struct phytium_spi *fts)
{
	u32 reg;

	reg = phytium_read_regfile(fts, SPI_REGFILE_DEBUG);

	phytium_write_regfile(fts, SPI_REGFILE_DEBUG,
			reg | SPI_REGFILE_DEBUG_VAL);
}

void spi_phyt_disable_debug(struct phytium_spi *fts)
{
	u32 reg;

	reg = phytium_read_regfile(fts, SPI_REGFILE_DEBUG);
	reg &= ~SPI_REGFILE_DEBUG_VAL;

	phytium_write_regfile(fts, SPI_REGFILE_DEBUG, reg);
}

void spi_phyt_disable_alive(struct phytium_spi *fts)
{
	u32 reg;

	reg = phytium_read_regfile(fts, SPI_REGFILE_DEBUG);
	reg &= ~SPI_REGFILE_ALIVE_VAL;

	phytium_write_regfile(fts, SPI_REGFILE_DEBUG, reg);
}

void spi_watchdog(struct phytium_spi *fts)
{
	u32 reg;

	reg = phytium_read_regfile(fts, SPI_REGFILE_DEBUG);
	phytium_write_regfile(fts, SPI_REGFILE_DEBUG,
			reg | SPI_REGFILE_HEARTBIT_VAL);
}

static void spi_phyt_timer_handle(struct timer_list *t)
{
	struct phytium_spi *fts = from_timer(fts, t, timer);

	if (fts->alive_enabled && fts->watchdog) {
		if (fts->runtimes < 20)
			fts->watchdog(fts);

		fts->runtimes++;
	}

	mod_timer(&fts->timer, jiffies + msecs_to_jiffies(10));
}

static void spi_phyt_hw_init(struct device *dev, struct phytium_spi *fts)
{
	spi_phytium_default(fts);
}

int spi_phyt_add_host(struct device *dev, struct phytium_spi *fts)
{
	struct spi_master *master;
	int ret;

	WARN_ON(fts == NULL);

	master = spi_alloc_master(dev, 0);
	if (!master)
		return -ENOMEM;

	fts->master = master;
	snprintf(fts->name, sizeof(fts->name), "phytium_spi%d", fts->bus_num);

	init_completion(&fts->cmd_completion);
	ret = devm_request_irq(dev, fts->irq, spi_phyt_irq, IRQF_SHARED, fts->name, master);
	if (ret < 0) {
		dev_err(dev, "can not get IRQ\n");
		goto err_free_master;
	}

	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_LOOP;
	master->bits_per_word_mask = SPI_BPW_MASK(8) | SPI_BPW_MASK(16);
	master->bus_num = fts->bus_num;
	master->num_chipselect = fts->num_cs;
	master->setup = spi_phyt_setup;
	master->cleanup = spi_phyt_cleanup;
	master->set_cs = spi_phyt_set_cs;
	master->transfer_one = spi_phyt_transfer_one;
	master->handle_err = spi_phyt_handle_err;
	master->max_speed_hz = fts->max_freq;
	master->dev.of_node = dev->of_node;
	master->dev.fwnode = dev->fwnode;
	master->flags = SPI_CONTROLLER_GPIO_SS;

	spi_master_set_devdata(master, fts);

	spi_phyt_disable_debug(fts);
	spi_phyt_disable_alive(fts);
	fts->runtimes = 0;
	fts->debug_enabled = false;
	fts->alive_enabled = false;

	fts->watchdog = spi_watchdog;

	fts->timer.expires = jiffies + msecs_to_jiffies(50);
	timer_setup(&fts->timer, spi_phyt_timer_handle, 0);
	add_timer(&fts->timer);

	spi_phyt_hw_init(dev, fts);

	ret = devm_spi_register_master(dev, master);
	if (ret) {
		dev_err(&master->dev, "problem registering spi master\n");
		goto err_exit;
	}

	return 0;

err_exit:
	spi_phyt_enable_chip(fts, 0);
err_free_master:
	spi_master_put(master);
	return ret;
}
EXPORT_SYMBOL_GPL(spi_phyt_add_host);

void spi_phyt_remove_host(struct phytium_spi *fts)
{
	del_timer(&fts->timer);
	spi_phyt_shutdown_chip(fts);
}
EXPORT_SYMBOL_GPL(spi_phyt_remove_host);

int spi_phyt_suspend_host(struct phytium_spi *fts)
{
	int ret;

	ret = spi_controller_suspend(fts->master);
	if (ret)
		return ret;

	spi_phyt_shutdown_chip(fts);
	return 0;
}
EXPORT_SYMBOL_GPL(spi_phyt_suspend_host);

int spi_phyt_resume_host(struct phytium_spi *fts)
{
	int ret;

	spi_phyt_hw_init(&fts->master->dev, fts);

	spi_phyt_enable_chip(fts, 0);
	spi_phyt_set_clk(fts, fts->clk_div);
	spi_phyt_enable_chip(fts, 1);

	ret = spi_controller_resume(fts->master);
	if (ret)
		dev_err(&fts->master->dev, "fail to start queue (%d)\n", ret);
	return ret;
}
EXPORT_SYMBOL_GPL(spi_phyt_resume_host);

MODULE_AUTHOR("Peng Min <pengmin1540@phytium.com.cn>");
MODULE_DESCRIPTION("Driver for Phytium SPI controller core");
MODULE_LICENSE("GPL");

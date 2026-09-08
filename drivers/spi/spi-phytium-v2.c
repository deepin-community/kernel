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
#include <linux/timer.h>
#include <linux/spi/spi.h>
#include <linux/scatterlist.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/property.h>
#include <linux/acpi.h>
#include <linux/mtd/spi-nor.h>
#include "spi-phytium.h"

#define MCP251x_READ		0x03
#define MCP251x_READ_RXB0	0x90
#define MCP251x_READ_RXB1	0x94

static inline int spi_phyt_enable_chip(struct phytium_spi *fts, u8 enable)
{
	u8 val = enable ? 1 : 2;

	return spi_phytium_set_cmd8(fts, PHYTSPI_MSG_CMD_SET_MODULE_EN, val);
}

static inline int spi_phyt_set_clk(struct phytium_spi *fts, u16 div)
{
	u32 new_div = div;

	return spi_phytium_set_cmd32(fts, PHYTSPI_MSG_CMD_SET_BAUDR, new_div);
}

static inline int spi_phyt_dma_reset(struct phytium_spi *fts, u8 enable)
{
	return spi_phytium_set_cmd8(fts, PHYTSPI_MSG_CMD_SET_DMA_RESET, enable);
}

static inline int spi_phyt_global_cs(struct phytium_spi *fts)
{
	u32 global_cs_en;
	u16 cs;

	global_cs_en = GENMASK(fts->num_cs-1, 0) << fts->num_cs;

	cs = (u16)((0x1 << 8) | global_cs_en);
	return spi_phytium_set_cmd16(fts, PHYTSPI_MSG_CMD_SET_CS, cs);
}

static inline void spi_phyt_reset_chip(struct phytium_spi *fts)
{
	spi_phyt_dma_reset(fts, 1);
	spi_phyt_enable_chip(fts, 0);
	if (fts->global_cs)
		spi_phyt_global_cs(fts);
	spi_phyt_enable_chip(fts, 1);
}

static inline int spi_phyt_shutdown_chip(struct phytium_spi *fts)
{
	int ret;

	ret = spi_phyt_enable_chip(fts, 0);
	if (ret)
		return ret;

	return spi_phyt_set_clk(fts, 0);
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
	struct phytium_spi *fts = spi_controller_get_devdata(spi->controller);
	struct chip_data *chip = spi_get_ctldata(spi);
	u32 origin;
	u16 cs;

	if (fts->tx || fts->rx)
		return;

	if (fts->msg_buf.cmd_id == PHYTSPI_MSG_CMD_DATA &&
			fts->msg_buf.cmd_subid == PHYTSPI_MSG_CMD_DATA_TX)
		return;

	if (chip && chip->cs_control)
		chip->cs_control(!enable);

	if (!enable) {
		cs = BIT(spi_get_chipselect(spi, 0));
		spi_phytium_set_cmd16(fts, PHYTSPI_MSG_CMD_SET_CS, cs);
		if (fts->global_cs) {
			origin = (GENMASK(fts->num_cs-1, 0) << fts->num_cs)
				| (1 << spi_get_chipselect(spi, 0));
			cs = (0x1 << 8) | origin;
			spi_phytium_set_cmd16(fts, PHYTSPI_MSG_CMD_SET_CS, cs);
		}
	} else {
		if (fts->global_cs) {
			origin = (GENMASK(fts->num_cs-1, 0) << fts->num_cs)
				& ~(1 << spi_get_chipselect(spi, 0));
			cs = (0x1 << 8) | origin;
			spi_phytium_set_cmd16(fts, PHYTSPI_MSG_CMD_SET_CS, cs);
		}
	}
}

static irqreturn_t spi_phyt_irq(int irq, void *dev_id)
{
	struct spi_controller *master = dev_id;
	struct phytium_spi *fts = spi_controller_get_devdata(master);
	u32 state;

	/* Check whether this controller actually raised the interrupt;
	 * the handler is registered with IRQF_SHARED.
	 */
	state = readl_relaxed(fts->regfile + SPI_REGFILE_RV2AP_INTR_STATE);
	if (!state)
		return IRQ_NONE;

	/* Acknowledge before completing so a waiter on another CPU cannot
	 * submit a new command before the old status is cleared.
	 */
	writel_relaxed(0, fts->regfile + SPI_REGFILE_RV2AP_INTR_STATE);
	writel_relaxed(0x10, fts->regfile + SPI_REGFILE_RV2AP_INT_CLEAN);
	complete(&fts->cmd_completion);

	return IRQ_HANDLED;
}

static int spi_phyt_transfer_one(struct spi_controller *master,
		struct spi_device *spi, struct spi_transfer *transfer)
{
	struct phytium_spi *fts = spi_controller_get_devdata(master);
	struct chip_data *chip = spi_get_ctldata(spi);
	struct spi_mem *mem = NULL;
	struct spi_nor *nor = NULL;
	int ret;

	/*
	 * spi_get_drvdata() returns whatever the client driver stored:
	 * spi_mem for spi-mem drivers, but e.g. mcp251x_priv for the
	 * MCP251x CAN controller also supported here.  Only interpret it
	 * as a struct spi_mem when the client is actually the spi-nor
	 * driver, otherwise the dereference below crashes on ordinary
	 * SPI clients.
	 */
	if (spi->dev.driver && !strcmp(spi->dev.driver->name, "spi-nor")) {
		mem = spi_get_drvdata(spi);
		if (mem)
			nor = spi_mem_get_drvdata(mem);
	}

	if (!transfer->tx_buf && !transfer->rx_buf) {
		/* clock-only transfers without buffers are not supported */
		dev_err(&master->dev, "transfer with no tx/rx buffer\n");
		return -EOPNOTSUPP;
	}

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

	if (fts->tx && fts->rx) {
		if (fts->half_duplex) {
			dev_err(&master->dev, "SPI-V2 not support full duplex\n");
			return -EPERM;
		}
		ret = spi_phytium_xfer(fts, spi_get_chipselect(spi, 0), transfer->bits_per_word,
				spi->mode, chip->tmode, 0);
		return ret;
	}

	if (mem != NULL && nor != NULL && mem == nor->spimem && fts->tx && fts->len == 1) {
		if ((*(u8 *)fts->tx == SPINOR_OP_WREN) && fts->spi_write_flag == 0) {
			spi_phytium_write_pre(fts, spi_get_chipselect(spi, 0),
					transfer->bits_per_word, spi->mode,
					chip->tmode, 3, fts->spi_write_flag);
			fts->spi_write_flag++;
			return 0;
		}

		if ((*(u8 *)fts->tx == SPINOR_OP_BE_4K) && (fts->spi_write_flag == 1) &&
				fts->flash_read == 0 && fts->flash_erase != 1) {
			fts->spi_write_flag++;
			fts->flash_erase = 1;
			return 0;
		}

		if ((*(u8 *)fts->tx == SPINOR_OP_CHIP_ERASE) && (fts->spi_write_flag == 1) &&
				fts->flash_read == 0 && fts->flash_erase == 0) {
			ret = spi_phytium_flash_erase(fts, spi_get_chipselect(spi, 0),
					transfer->bits_per_word,
					spi->mode, chip->tmode, 3, SPINOR_OP_CHIP_ERASE);
			if (ret) {
				dev_err(&spi->dev, "chip erase failed: %d\n", ret);
				return ret;
			}
			fts->spi_write_flag = 0;
			fts->flash_erase = 2;
			/* the erase was issued to the firmware; do not fall
			 * through to the generic TX path below, which would
			 * send the opcode a second time */
			return ret;
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
			ret = spi_phytium_flash_erase(fts, spi_get_chipselect(spi, 0),
					transfer->bits_per_word,
					spi->mode, chip->tmode, 3, SPINOR_OP_BE_4K);
			if (ret) {
				dev_err(&master->dev, "flash erase failed\n");
				return ret;
			}
			fts->spi_write_flag = 0;
			fts->flash_erase++;
		} else if (fts->flash_read) {
			ret = spi_phytium_flash_erase(fts, spi_get_chipselect(spi, 0),
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
			ret = spi_phytium_write(fts, spi_get_chipselect(spi, 0), transfer->bits_per_word,
					spi->mode, chip->tmode, 3, fts->spi_write_flag);
			if (ret) {
				dev_err(&master->dev, "transfer disable-command failed\n");
				return ret;
			}
			fts->flash_erase = 0;
		} else {
			fts->flags = 1;
			if (fts->spi_write_flag == 0 && *(u8 *)(fts->tx) != MCP251x_READ
					&& *(u8 *)(fts->tx) != MCP251x_READ_RXB0
					&& *(u8 *)(fts->tx) != MCP251x_READ_RXB1)
				fts->flags = 3;

			ret = spi_phytium_write(fts, spi_get_chipselect(spi, 0), transfer->bits_per_word,
					spi->mode, chip->tmode, fts->flags, fts->spi_write_flag);
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
		ret = spi_phytium_read(fts, spi_get_chipselect(spi, 0), transfer->bits_per_word,
				spi->mode, chip->tmode, 2);
		if (ret) {
			dev_err(&master->dev, "read data failed\n");
			return ret;
		}
	}

	return ret;
}

static void spi_phyt_handle_err(struct spi_controller *master,
		struct spi_message *msg)
{
	struct phytium_spi *fts = spi_controller_get_devdata(master);

	spi_phyt_reset_chip(fts);
}

static int spi_phyt_setup(struct spi_device *spi)
{
	struct phytium_spi_chip *chip_info = NULL;
	struct chip_data *chip;
	struct spi_controller *master = spi->controller;
	struct phytium_spi *fts = spi_controller_get_devdata(master);
	u8 data_width, scph, scpol, tmode;
	u16 mode;
	u16 clk_div;
	int ret;

	ret = spi_phyt_enable_chip(fts, 0);
	if (ret)
		return ret;

	if (!spi->max_speed_hz) {
		dev_err(&spi->dev, "max_speed_hz is zero\n");
		return -EINVAL;
	}

	clk_div = (fts->max_freq / spi->max_speed_hz + 1) & 0xfffe;
	ret = spi_phyt_set_clk(fts, clk_div);
	if (ret)
		return ret;
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
	ret = spi_phytium_set_cmd8(fts, PHYTSPI_MSG_CMD_SET_DATA_WIDTH, data_width);
	if (ret)
		return ret;

	scph = spi->mode & (0x1);
	scpol = spi->mode >> 1;
	mode = (scph << 8) | scpol;
	ret = spi_phytium_set_cmd16(fts, PHYTSPI_MSG_CMD_SET_MODE, mode);
	if (ret)
		return ret;

	tmode = chip->tmode;
	ret = spi_phytium_set_cmd8(fts, PHYTSPI_MSG_CMD_SET_TMOD, tmode);
	if (ret)
		return ret;

	ret = spi_phyt_enable_chip(fts, 1);
	if (ret)
		return ret;

	return 0;
}

static void spi_phyt_cleanup(struct spi_device *spi)
{
	struct chip_data *chip = spi_get_ctldata(spi);

	kfree(chip);
	spi_set_ctldata(spi, NULL);
}

static void spi_phyt_disable_debug(struct phytium_spi *fts)
{
	u32 reg;

	reg = phytium_read_regfile(fts, SPI_REGFILE_DEBUG);
	reg &= ~SPI_REGFILE_DEBUG_VAL;

	phytium_write_regfile(fts, SPI_REGFILE_DEBUG, reg);
}

static void spi_phyt_disable_alive(struct phytium_spi *fts)
{
	u32 reg;

	reg = phytium_read_regfile(fts, SPI_REGFILE_DEBUG);
	reg &= ~SPI_REGFILE_ALIVE_VAL;

	phytium_write_regfile(fts, SPI_REGFILE_DEBUG, reg);
}

static void spi_watchdog(struct phytium_spi *fts)
{
	u32 reg;

	reg = phytium_read_regfile(fts, SPI_REGFILE_DEBUG);
	phytium_write_regfile(fts, SPI_REGFILE_DEBUG,
			reg | SPI_REGFILE_HEARTBIT_VAL);
}

static void spi_phyt_timer_handle(struct timer_list *t)
{
	struct phytium_spi *fts = timer_container_of(fts, t, timer);

	if (fts->alive_enabled && fts->watchdog) {
		if (fts->runtimes < 20)
			fts->watchdog(fts);

		fts->runtimes++;

		/* rearm only while alive monitoring is enabled */
		mod_timer(&fts->timer, jiffies + msecs_to_jiffies(10));
	}
}

static void spi_handle_debug_err(struct phytium_spi *fts)
{
	struct device *dev = &fts->master->dev;
	u32 reg, len, i;

	reg = phytium_read_regfile(fts, SPI_REGFILE_DEBUG);

	if (reg & SPI_REGFILE_HAVE_LOG) {
		len = strnlen(fts->log, fts->log_size);
		dev_info(dev, "log len :%d,addr: 0x%llx,size:%d\n",
				len, (u64)fts->log, fts->log_size);
		if (len > SPI_LOG_LINE_MAX_LEN) {
			for (i = 0; i + SPI_LOG_LINE_MAX_LEN < len; i += SPI_LOG_LINE_MAX_LEN)
				dev_info(dev, "(log)%.*s\n", SPI_LOG_LINE_MAX_LEN, &fts->log[i]);
		} else {
			dev_info(dev, "(log)%.*s\n", SPI_LOG_LINE_MAX_LEN, &fts->log[0]);
		}

		for (i = 0; i < fts->log_size; i++)
			fts->log[i] = 0;
	}

	reg &= ~SPI_REGFILE_HAVE_LOG;
	phytium_write_regfile(fts, SPI_REGFILE_DEBUG, reg);
}

static int spi_phyt_hw_init(struct device *dev, struct phytium_spi *fts)
{
	u32 reg, i, reg_ddr_high;
	int ret;

	ret = spi_phytium_default(fts);
	if (ret) {
		dev_err(dev, "firmware is not responsive: %d\n", ret);
		return ret;
	}

	reg = phytium_read_regfile(fts, SPI_REGFILE_DEBUG);

	if (fts->regfile_version & SPI_REGFILE_VERSION_DDR) {
		fts->ddr_paddr = ((reg & SPI_REGFILE_ADDR_MASK) >> 8);
		reg_ddr_high = phytium_read_regfile(fts, SPI_REGFILE_DDR_HIGH_REG);
		fts->ddr_paddr |= ((u64)reg_ddr_high << 20);
	} else {
		fts->ddr_paddr = ((reg & SPI_REGFILE_ADDR_MASK) >> 8) << SPI_DDR_ADDR_HIGH;
	}

	fts->log_size = ((reg & SPI_REGFILE_SIZE_MASK) >> 4) * SPI_DEBUG_LOG_SIZE;

	/*
	 * Map the firmware log buffer only once: hw_init also runs from
	 * the resume path and a devm_ioremap() there would leak a mapping
	 * and a devres entry on every suspend/resume cycle.
	 */
	if (!fts->log) {
		fts->log = devm_ioremap(dev, fts->ddr_paddr, fts->log_size);
		if (!fts->log) {
			dev_err(dev, "log_addr is err\n");
			return -ENOMEM;
		}

		for (i = 0; i < fts->log_size; i++)
			fts->log[i] = 0;
	}

	return 0;
}

int spi_phyt_add_host(struct device *dev, struct phytium_spi *fts)
{
	struct spi_controller *master;
	int ret;

	WARN_ON(fts == NULL);

	master = devm_spi_alloc_host(dev, 0);
	if (!master)
		return -ENOMEM;

	fts->master = master;
	snprintf(fts->name, sizeof(fts->name), "phytium_spi%d", fts->bus_num);

	init_completion(&fts->cmd_completion);
	ret = devm_request_irq(dev, fts->irq, spi_phyt_irq, IRQF_SHARED, fts->name, master);
	if (ret < 0) {
		dev_err(dev, "can not get IRQ\n");
		return ret;
	}

	master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_LOOP;
	master->bits_per_word_mask = SPI_BPW_MASK(8) | SPI_BPW_MASK(16);
	master->bus_num = fts->bus_num;
	master->num_chipselect = fts->num_cs;
	master->use_gpio_descriptors = true;
	master->setup = spi_phyt_setup;
	master->cleanup = spi_phyt_cleanup;
	master->set_cs = spi_phyt_set_cs;
	master->transfer_one = spi_phyt_transfer_one;
	master->handle_err = spi_phyt_handle_err;
	master->max_speed_hz = fts->max_freq;
	master->dev.of_node = dev->of_node;
	master->dev.fwnode = dev->fwnode;
	master->flags = SPI_CONTROLLER_GPIO_SS;

	fts->half_duplex = false;
	if (!(phytium_read_regfile(fts, SPI_REGFILE_SOFTWARE2)
				& SPI_REGFILE_FULL_DUPLEX)) {
		dev_warn(dev, "SPI-V2 only support half duplex\n");
		fts->half_duplex = true;
		master->flags |= SPI_CONTROLLER_HALF_DUPLEX;
	}

	spi_controller_set_devdata(master, fts);

	spi_phyt_disable_debug(fts);
	spi_phyt_disable_alive(fts);
	fts->runtimes = 0;
	fts->debug_enabled = false;
	fts->alive_enabled = false;

	fts->watchdog = spi_watchdog;
	fts->handle_debug_err = spi_handle_debug_err;

	timer_setup(&fts->timer, spi_phyt_timer_handle, 0);

	ret = spi_phyt_hw_init(dev, fts);
	if (ret) {
		dev_err(dev, "hardware init failed: %d\n", ret);
		goto err_exit;
	}

	/*
	 * Register explicitly rather than devm: the devres unregister
	 * action would only run after the platform .remove callback has
	 * already shut the chip down, leaving child devices and the
	 * transfer queue registered against disabled hardware. With an
	 * explicit registration, spi_phyt_remove_host() unregisters the
	 * controller first and only then stops the hardware.
	 */
	ret = spi_register_controller(master);
	if (ret) {
		dev_err(&master->dev, "problem registering spi master\n");
		goto err_exit;
	}

	return 0;

err_exit:
	timer_delete_sync(&fts->timer);
	spi_phyt_enable_chip(fts, 0);
	return ret;
}
EXPORT_SYMBOL_GPL(spi_phyt_add_host);

void spi_phyt_remove_host(struct phytium_spi *fts)
{
	/* unregister children and the transfer queue before the
	 * hardware is turned off */
	spi_unregister_controller(fts->master);

	timer_delete_sync(&fts->timer);
	spi_phyt_shutdown_chip(fts);
}
EXPORT_SYMBOL_GPL(spi_phyt_remove_host);

int spi_phyt_suspend_host(struct phytium_spi *fts)
{
	int ret;

	ret = spi_controller_suspend(fts->master);
	if (ret)
		return ret;

	/* stop the watchdog timer before shutting down the chip */
	timer_delete_sync(&fts->timer);
	ret = spi_phyt_shutdown_chip(fts);
	if (ret) {
		dev_err(&fts->master->dev, "firmware shutdown failed: %d\n", ret);
		/*
		 * Let system suspend abort: the controller may still be
		 * enabled or keep its active clock divider. The PM core
		 * resumes the device during rollback, which restarts the
		 * transfer queue via spi_phyt_resume_host().
		 */
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(spi_phyt_suspend_host);

int spi_phyt_resume_host(struct phytium_spi *fts)
{
	int ret;

	ret = spi_phyt_hw_init(&fts->master->dev, fts);
	if (ret) {
		dev_err(&fts->master->dev, "hardware init failed on resume: %d\n", ret);
		return ret;
	}

	ret = spi_phyt_enable_chip(fts, 0);
	if (ret)
		return ret;
	ret = spi_phyt_set_clk(fts, fts->clk_div);
	if (ret)
		return ret;
	ret = spi_phyt_enable_chip(fts, 1);
	if (ret)
		return ret;

	ret = spi_controller_resume(fts->master);
	if (ret) {
		dev_err(&fts->master->dev, "fail to start queue (%d)\n", ret);
		return ret;
	}

	/* restart the watchdog timer after a successful resume,
	 * but only while alive monitoring is enabled */
	if (fts->alive_enabled)
		mod_timer(&fts->timer, jiffies + msecs_to_jiffies(10));
	return 0;
}
EXPORT_SYMBOL_GPL(spi_phyt_resume_host);

MODULE_AUTHOR("Peng Min <pengmin1540@phytium.com.cn>");
MODULE_DESCRIPTION("Driver for Phytium SPI controller core");
MODULE_LICENSE("GPL");

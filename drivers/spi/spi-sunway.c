// SPDX-License-Identifier: GPL-2.0
/*
 * SPI core controller driver
 */

#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/highmem.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/mtd/spi-nor.h>
#include <linux/kernel.h>

#include "spi-sunway.h"

/* Slave spi_dev related */
struct chip_data {
	u8 tmode;		/* TR/TO/RO/EEPROM */
	u8 type;		/* SPI/SSP/MicroWire */

	u8 poll_mode;		/* 1 means use poll mode */

	u16 clk_div;		/* baud rate divider */
	u32 speed_hz;		/* baud rate */
	void (*cs_control)(u32 command);
};

static void spi_chip_handle_err(struct spi_controller *master,
		struct spi_message *msg)
{
	struct spi_chip *spi_chip = spi_controller_get_devdata(master);

	spi_reset_chip(spi_chip);
}

static size_t spi_chip_max_length(struct spi_device *spi)
{
	struct spi_chip *spi_chip = spi_controller_get_devdata(spi->master);

	return spi_chip->fifo_len;
}

static int spi_chip_transfer_one_message(struct spi_controller *master,
		struct spi_message *m)
{
	struct spi_chip *spi_chip = spi_controller_get_devdata(master);
	struct spi_transfer *t = NULL;
	u16 clk_div;
	u32 freq;
	u32 speed_hz;
	u32 status;
	u32 len = 0;
	int ret = 0;
	int i = 0;

	spi_enable_chip(spi_chip, 0);

	/* Handle per transfer options for bpw and speed. */
	freq = clamp(m->spi->max_speed_hz, 0U, spi_chip->max_freq);
	clk_div = (DIV_ROUND_UP(spi_chip->max_freq, freq) + 1) & 0xfffe;
	speed_hz = spi_chip->max_freq / clk_div;

	if (spi_chip->current_freq != speed_hz) {
		spi_set_clk(spi_chip, clk_div);
		spi_chip->current_freq = speed_hz;
	}

	spi_chip->n_bytes = 1;

	/* For poll mode just disable all interrupts */
	spi_mask_intr(spi_chip, 0xff);

	spi_writel(spi_chip, SPI_CHIP_CTRL0, SPI_TRANSMIT_RECEIVE);

	spi_enable_chip(spi_chip, 1);

	list_for_each_entry(t, &m->transfers, transfer_list) {
		len += t->len;
		/* Judge if data is overflow */
		if (len > spi_chip->fifo_len) {
			pr_err("SPI transfer overflow.\n");
			m->actual_length = 0;
			m->status = -EIO;
			ret = -EIO;
			goto way_out;
		}

		if (t->tx_buf)
			memcpy(&spi_chip->buf[len], t->tx_buf, t->len);
		else
			memset(&spi_chip->buf[len], 0, t->len);
	}

	spi_writel(spi_chip, SPI_CHIP_SER, 0x0);
	for (i = 0; i < len; i++)
		spi_writel(spi_chip, SPI_CHIP_DR, spi_chip->buf[i]);
	spi_writel(spi_chip, SPI_CHIP_SER, BIT(m->spi->chip_select));

	do {
		status = spi_readl(spi_chip, SPI_CHIP_SR);
	} while (status & SR_BUSY);

	list_for_each_entry(t, &m->transfers, transfer_list) {
		if (t->rx_buf) {
			for (i = 0; i < t->len; i++, t->rx_buf += 1)
				*(u8 *)t->rx_buf = spi_readl(spi_chip,
							     SPI_CHIP_DR);
		} else {
			for (i = 0; i < t->len; i++)
				spi_readl(spi_chip, SPI_CHIP_DR);
		}
	}

	m->actual_length = len;
	m->status = 0;
	spi_finalize_current_message(master);

way_out:
	return ret;
}

static int spi_chip_adjust_mem_op_size(struct spi_mem *mem,
				       struct spi_mem_op *op)
{
	struct spi_chip *spi_chip = spi_controller_get_devdata(
					   mem->spi->controller);
	size_t len;

	len = sizeof(op->cmd.opcode) + op->addr.nbytes + op->dummy.nbytes;

	op->data.nbytes = min((size_t)op->data.nbytes,
			     (spi_chip->fifo_len - len));
	if (!op->data.nbytes)
		return -EINVAL;

	return 0;
}

static int spi_chip_init_mem_buf(struct spi_chip *spi_chip,
		const struct spi_mem_op *op)
{
	int ret = 0;
	int i, j, len;

	/* Calculate the total length of the transfer. */
	len = op->cmd.nbytes + op->addr.nbytes + op->dummy.nbytes;

	/* Judge if data is overflow */
	if (len + op->data.nbytes > spi_chip->fifo_len) {
		ret = -EIO;
		goto way_out;
	}

	/*
	 * Collect the operation code, address and dummy bytes into the single
	 * buffer. If it's a transfer with data to be sent, also copy it into
	 * the single buffer.
	 */
	for (i = 0; i < op->cmd.nbytes; i++)
		spi_chip->buf[i] = op->cmd.opcode;
	for (j = 0; j < op->addr.nbytes; i++, j++)
		spi_chip->buf[i] = op->addr.val >> (8 * (op->addr.nbytes - i));
	for (j = 0; j < op->dummy.nbytes; i++, j++)
		spi_chip->buf[i] = 0xff;

	if (op->data.dir == SPI_MEM_DATA_OUT) {
		memcpy(&spi_chip->buf[i], op->data.buf.out, op->data.nbytes);
		len += op->data.nbytes;
	}

	spi_chip->tx_len = len;

	if (op->data.dir == SPI_MEM_DATA_IN) {
		spi_chip->rx = op->data.buf.in;
		spi_chip->rx_len = op->data.nbytes;
	} else {
		spi_chip->rx = NULL;
		spi_chip->rx_len = 0;
	}

way_out:
	return ret;
}

static int spi_chip_exec_mem_op(struct spi_mem *mem,
				const struct spi_mem_op *op)
{
	struct spi_chip *spi_chip = spi_controller_get_devdata(
					   mem->spi->controller);
	u16 clk_div;
	int ret = 0;
	int i;
	unsigned short value;
	u32 freq;
	u32 speed_hz;

	ret = spi_chip_init_mem_buf(spi_chip, op);
	if (ret)
		return ret;

	spi_enable_chip(spi_chip, 0);

	/* Handle per transfer options for bpw and speed. */
	freq = clamp(mem->spi->max_speed_hz, 0U, spi_chip->max_freq);
	clk_div = (DIV_ROUND_UP(spi_chip->max_freq, freq) + 1) & 0xfffe;
	speed_hz = spi_chip->max_freq / clk_div;

	if (spi_chip->current_freq != speed_hz) {
		spi_set_clk(spi_chip, clk_div);
		spi_chip->current_freq = speed_hz;
	}

	spi_chip->n_bytes = 1;

	/* For poll mode just disable all interrupts */
	spi_mask_intr(spi_chip, 0xff);

	if ((spi_chip->tx_len != 0) && (spi_chip->rx_len != 0)) {
		spi_writel(spi_chip, SPI_CHIP_CTRL0, SPI_EEPROM_READ);
		spi_writel(spi_chip, SPI_CHIP_CTRL1, (spi_chip->rx_len - 1));
	} else {
		spi_writel(spi_chip, SPI_CHIP_CTRL0, SPI_TRANSMIT_ONLY);
	}

	spi_enable_chip(spi_chip, 1);

	spi_writel(spi_chip, SPI_CHIP_SER, 0x0);
	for (i = 0; i < spi_chip->tx_len; i++)
		spi_writel(spi_chip, SPI_CHIP_DR, spi_chip->buf[i]);
	spi_writel(spi_chip, SPI_CHIP_SER, BIT(mem->spi->chip_select));

	value = spi_readl(spi_chip, SPI_CHIP_SR);
	while (value & SR_BUSY)
		value = spi_readl(spi_chip, SPI_CHIP_SR);

	for (i = 0; i < spi_chip->rx_len; spi_chip->rx += spi_chip->n_bytes, i++)
		*(u8 *)spi_chip->rx = spi_readl(spi_chip, SPI_CHIP_DR);

	return ret;
}

/* This may be called twice for each spi dev */
static int spi_chip_setup(struct spi_device *spi)
{
	struct spi_chip_info *chip_info = NULL;
	struct chip_data *chip;
	u32 poll_mode = 0;
	struct device_node *np = spi->dev.of_node;

	/* Only alloc on first setup */
	chip = spi_get_ctldata(spi);
	if (!chip) {
		chip = kzalloc(sizeof(struct chip_data), GFP_KERNEL);
		if (!chip)
			return -ENOMEM;
		spi_set_ctldata(spi, chip);
	}

	/*
	 * Protocol drivers may change the chip settings, so...
	 * if chip_info exists, use it
	 */
	chip_info = spi->controller_data;

	/* chip_info doesn't always exist */
	if (chip_info) {
		if (chip_info->cs_control)
			chip->cs_control = chip_info->cs_control;

		chip->poll_mode = chip_info->poll_mode;
		chip->type = chip_info->type;
	} else {
		if (np) {
			of_property_read_u32(np, "poll_mode", &poll_mode);
			chip->poll_mode = poll_mode;
		}

	}

	chip->tmode = SPI_TMOD_TR;
	return 0;
}

static void spi_chip_cleanup(struct spi_device *spi)
{
	struct chip_data *chip = spi_get_ctldata(spi);

	kfree(chip);
	spi_set_ctldata(spi, NULL);
}

/* Restart the controller, disable all interrupts, clean rx fifo */
static void spi_hw_init(struct device *dev, struct spi_chip *spi_chip)
{
	spi_reset_chip(spi_chip);

	/*
	 * Try to detect the FIFO depth if not set by interface driver,
	 * the depth could be from 2 to 256 from HW spec
	 */
	if (!spi_chip->fifo_len) {
		u32 fifo;

		for (fifo = 1; fifo < 256; fifo++) {
			spi_writel(spi_chip, SPI_CHIP_TXFLTR, fifo);
			if (fifo != spi_readl(spi_chip, SPI_CHIP_TXFLTR))
				break;
		}
		spi_writel(spi_chip, SPI_CHIP_TXFLTR, 0);

		spi_chip->fifo_len = (fifo == 1) ? 0 : fifo;
		dev_info(dev, "Detected FIFO size: %u bytes\n",
			 spi_chip->fifo_len);
	}
}

static const struct spi_controller_mem_ops spi_mem_ops = {
	.adjust_op_size = spi_chip_adjust_mem_op_size,
	.exec_op = spi_chip_exec_mem_op,
};

int spi_chip_add_host(struct device *dev, struct spi_chip *spi_chip)
{
	struct spi_controller *master;
	int ret;

	WARN_ON(spi_chip == NULL);

	master = spi_alloc_master(dev, 0);
	if (!master)
		return -ENOMEM;

	spi_chip->master = master;
	spi_chip->type = SSI_MOTO_SPI;

	spi_controller_set_devdata(master, spi_chip);

	master->mode_bits = SPI_CPOL | SPI_CPHA;
	master->bits_per_word_mask = SPI_BPW_MASK(8) | SPI_BPW_MASK(16);
	master->bus_num = spi_chip->bus_num;
	master->num_chipselect = spi_chip->num_cs;
	master->setup = spi_chip_setup;
	master->cleanup = spi_chip_cleanup;
	master->transfer_one_message = spi_chip_transfer_one_message;
	master->handle_err = spi_chip_handle_err;
	master->max_speed_hz = spi_chip->max_freq;
	master->dev.of_node = dev->of_node;
	master->flags = SPI_CONTROLLER_GPIO_SS;
	master->max_transfer_size = spi_chip_max_length;
	master->max_message_size = spi_chip_max_length;

	master->mem_ops = &spi_mem_ops;

	/* Basic HW init */
	spi_hw_init(dev, spi_chip);

	ret = devm_spi_register_controller(dev, master);
	if (ret) {
		dev_err(&master->dev, "problem registering spi master\n");
		spi_enable_chip(spi_chip, 0);
		free_irq(spi_chip->irq, master);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(spi_chip_add_host);

void spi_chip_remove_host(struct spi_chip *spi_chip)
{
	spi_shutdown_chip(spi_chip);

	free_irq(spi_chip->irq, spi_chip->master);
}
EXPORT_SYMBOL_GPL(spi_chip_remove_host);

int spi_chip_suspend_host(struct spi_chip *spi_chip)
{
	int ret;

	ret = spi_controller_suspend(spi_chip->master);
	if (ret)
		return ret;

	spi_shutdown_chip(spi_chip);
	return 0;
}
EXPORT_SYMBOL_GPL(spi_chip_suspend_host);

int spi_chip_resume_host(struct spi_chip *spi_chip)
{
	int ret;

	spi_hw_init(&spi_chip->master->dev, spi_chip);
	ret = spi_controller_resume(spi_chip->master);
	if (ret)
		dev_err(&spi_chip->master->dev, "fail to start queue (%d)\n",
			ret);
	return ret;
}
EXPORT_SYMBOL_GPL(spi_chip_resume_host);

MODULE_AUTHOR("Platform@wiat.com");
MODULE_DESCRIPTION("Driver for SPI controller core");
MODULE_LICENSE("GPL v2");

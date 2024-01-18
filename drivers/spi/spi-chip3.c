// SPDX-License-Identifier: GPL-2.0
/*
 * SUNWAY CHIP3 SPI core controller driver
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

#include "spi-chip3.h"

/* Slave spi_dev related */
struct chip_data {
	u8 tmode;		/* TR/TO/RO/EEPROM */
	u8 type;		/* SPI/SSP/MicroWire */

	u8 poll_mode;		/* 1 means use poll mode */

	u16 clk_div;		/* baud rate divider */
	u32 speed_hz;		/* baud rate */
	void (*cs_control)(u32 command);
};

static void chip3_spi_handle_err(struct spi_controller *master,
		struct spi_message *msg)
{
	struct chip3_spi *dws = spi_controller_get_devdata(master);

	spi_reset_chip(dws);
}

static size_t chip3_spi_max_length(struct spi_device *spi)
{
	struct chip3_spi *dws = spi_controller_get_devdata(spi->master);

	return dws->fifo_len;
}

static int chip3_spi_transfer_one_message(struct spi_controller *master,
		struct spi_message *m)
{
	struct chip3_spi *dws = spi_controller_get_devdata(master);
	struct spi_transfer *t = NULL;
	u16 clk_div;
	u32 freq;
	u32 speed_hz;
	u32 status;
	u32 len = 0;
	int ret = 0;
	int i = 0;

	spi_enable_chip(dws, 0);

	/* Handle per transfer options for bpw and speed. */
	freq = clamp(m->spi->max_speed_hz, 0U, dws->max_freq);
	clk_div = (DIV_ROUND_UP(dws->max_freq, freq) + 1) & 0xfffe;
	speed_hz = dws->max_freq / clk_div;

	if (dws->current_freq != speed_hz) {
		spi_set_clk(dws, clk_div);
		dws->current_freq = speed_hz;
	}

	dws->n_bytes = 1;

	/* For poll mode just disable all interrupts */
	spi_mask_intr(dws, 0xff);

	chip3_writel(dws, CHIP3_SPI_CTRL0, SPI_TRANSMIT_RECEIVE);

	spi_enable_chip(dws, 1);

	list_for_each_entry(t, &m->transfers, transfer_list) {
		len += t->len;
		/* Judge if data is overflow */
		if (len > dws->fifo_len) {
			pr_err("SPI transfer overflow.\n");
			m->actual_length = 0;
			m->status = -EIO;
			ret = -EIO;
			goto way_out;
		}

		if (t->tx_buf)
			memcpy(&dws->buf[len], t->tx_buf, t->len);
		else
			memset(&dws->buf[len], 0, t->len);
	}

	chip3_writel(dws, CHIP3_SPI_SER, 0x0);
	for (i = 0; i < len; i++)
		chip3_writel(dws, CHIP3_SPI_DR, dws->buf[i]);
	chip3_writel(dws, CHIP3_SPI_SER, BIT(m->spi->chip_select));

	do {
		status = chip3_readl(dws, CHIP3_SPI_SR);
	} while (status & SR_BUSY);

	list_for_each_entry(t, &m->transfers, transfer_list) {
		if (t->rx_buf) {
			for (i = 0; i < t->len; i++, t->rx_buf += 1)
				*(u8 *)t->rx_buf = chip3_readl(dws, CHIP3_SPI_DR);
		} else {
			for (i = 0; i < t->len; i++)
				chip3_readl(dws, CHIP3_SPI_DR);
		}
	}

	m->actual_length = len;
	m->status = 0;
	spi_finalize_current_message(master);

way_out:
	return ret;
}

static int chip3_spi_adjust_mem_op_size(struct spi_mem *mem,
		struct spi_mem_op *op)
{
	struct chip3_spi *dws = spi_controller_get_devdata(mem->spi->controller);
	size_t len;

	len = sizeof(op->cmd.opcode) + op->addr.nbytes + op->dummy.nbytes;

	op->data.nbytes = min((size_t)op->data.nbytes, (dws->fifo_len - len));
	if (!op->data.nbytes)
		return -EINVAL;

	return 0;
}

static int chip3_spi_init_mem_buf(struct chip3_spi *dws,
		const struct spi_mem_op *op)
{
	int ret = 0;
	int i, j, len;

	/* Calculate the total length of the transfer. */
	len = sizeof(op->cmd.opcode) + op->addr.nbytes + op->dummy.nbytes;

	/* Judge if data is overflow */
	if (len + op->data.nbytes > dws->fifo_len) {
		ret = -EIO;
		goto way_out;
	}

	/*
	 * Collect the operation code, address and dummy bytes into the single
	 * buffer. If it's a transfer with data to be sent, also copy it into
	 * the single buffer.
	 */
	for (i = 0; i < sizeof(op->cmd.opcode); i++)
		dws->buf[i] = op->cmd.opcode;
	for (j = 0; j < op->addr.nbytes; i++, j++)
		dws->buf[i] = op->addr.val >> (8 * (op->addr.nbytes - i));
	for (j = 0; j < op->dummy.nbytes; i++, j++)
		dws->buf[i] = 0xff;

	if (op->data.dir == SPI_MEM_DATA_OUT) {
		memcpy(&dws->buf[i], op->data.buf.out, op->data.nbytes);
		len += op->data.nbytes;
	}

	dws->tx_len = len;

	if (op->data.dir == SPI_MEM_DATA_IN) {
		dws->rx = op->data.buf.in;
		dws->rx_len = op->data.nbytes;
	} else {
		dws->rx = NULL;
		dws->rx_len = 0;
	}

way_out:
	return ret;
}

static int chip3_spi_exec_mem_op(struct spi_mem *mem,
		const struct spi_mem_op *op)
{
	struct chip3_spi *dws = spi_controller_get_devdata(mem->spi->controller);
	u16 clk_div;
	int ret = 0;
	int i;
	unsigned short value;
	u32 freq;
	u32 speed_hz;

	ret = chip3_spi_init_mem_buf(dws, op);
	if (ret)
		return ret;

	spi_enable_chip(dws, 0);

	/* Handle per transfer options for bpw and speed. */
	freq = clamp(mem->spi->max_speed_hz, 0U, dws->max_freq);
	clk_div = (DIV_ROUND_UP(dws->max_freq, freq) + 1) & 0xfffe;
	speed_hz = dws->max_freq / clk_div;

	if (dws->current_freq != speed_hz) {
		spi_set_clk(dws, clk_div);
		dws->current_freq = speed_hz;
	}

	dws->n_bytes = 1;

	/* For poll mode just disable all interrupts */
	spi_mask_intr(dws, 0xff);

	if ((dws->tx_len != 0) && (dws->rx_len != 0)) {
		chip3_writel(dws, CHIP3_SPI_CTRL0, SPI_EEPROM_READ);
		chip3_writel(dws, CHIP3_SPI_CTRL1, (dws->rx_len - 1));
	} else {
		chip3_writel(dws, CHIP3_SPI_CTRL0, SPI_TRANSMIT_ONLY);
	}

	spi_enable_chip(dws, 1);

	chip3_writel(dws, CHIP3_SPI_SER, 0x0);
	for (i = 0; i < dws->tx_len; i++)
		chip3_writel(dws, CHIP3_SPI_DR, dws->buf[i]);
	chip3_writel(dws, CHIP3_SPI_SER, BIT(mem->spi->chip_select));

	value = chip3_readl(dws, CHIP3_SPI_SR);
	while (value & SR_BUSY)
		value = chip3_readl(dws, CHIP3_SPI_SR);

	for (i = 0; i < dws->rx_len; dws->rx += dws->n_bytes, i++)
		*(u8 *)dws->rx = chip3_readl(dws, CHIP3_SPI_DR);

	return ret;
}

/* This may be called twice for each spi dev */
static int chip3_spi_setup(struct spi_device *spi)
{
	struct chip3_spi_chip *chip_info = NULL;
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

static void chip3_spi_cleanup(struct spi_device *spi)
{
	struct chip_data *chip = spi_get_ctldata(spi);

	kfree(chip);
	spi_set_ctldata(spi, NULL);
}

/* Restart the controller, disable all interrupts, clean rx fifo */
static void spi_hw_init(struct device *dev, struct chip3_spi *dws)
{
	spi_reset_chip(dws);

	/*
	 * Try to detect the FIFO depth if not set by interface driver,
	 * the depth could be from 2 to 256 from HW spec
	 */
	if (!dws->fifo_len) {
		u32 fifo;

		for (fifo = 1; fifo < 256; fifo++) {
			chip3_writel(dws, CHIP3_SPI_TXFLTR, fifo);
			if (fifo != chip3_readl(dws, CHIP3_SPI_TXFLTR))
				break;
		}
		chip3_writel(dws, CHIP3_SPI_TXFLTR, 0);

		dws->fifo_len = (fifo == 1) ? 0 : fifo;
		dev_info(dev, "Detected FIFO size: %u bytes\n", dws->fifo_len);
	}
}

static const struct spi_controller_mem_ops chip3_mem_ops = {
	.adjust_op_size = chip3_spi_adjust_mem_op_size,
	.exec_op = chip3_spi_exec_mem_op,
};


int chip3_spi_add_host(struct device *dev, struct chip3_spi *dws)
{
	struct spi_controller *master;
	int ret;

	BUG_ON(dws == NULL);

	master = spi_alloc_master(dev, 0);
	if (!master)
		return -ENOMEM;

	dws->master = master;
	dws->type = SSI_MOTO_SPI;

	spi_controller_set_devdata(master, dws);

	master->mode_bits = SPI_CPOL | SPI_CPHA;
	master->bits_per_word_mask = SPI_BPW_MASK(8) | SPI_BPW_MASK(16);
	master->bus_num = dws->bus_num;
	master->num_chipselect = dws->num_cs;
	master->setup = chip3_spi_setup;
	master->cleanup = chip3_spi_cleanup;
	master->transfer_one_message = chip3_spi_transfer_one_message;
	master->handle_err = chip3_spi_handle_err;
	master->max_speed_hz = dws->max_freq;
	master->dev.of_node = dev->of_node;
	master->flags = SPI_CONTROLLER_GPIO_SS;
	master->max_transfer_size = chip3_spi_max_length;
	master->max_message_size = chip3_spi_max_length;

	master->mem_ops = &chip3_mem_ops;

	/* Basic HW init */
	spi_hw_init(dev, dws);

	ret = devm_spi_register_controller(dev, master);
	if (ret) {
		dev_err(&master->dev, "problem registering spi master\n");
		spi_enable_chip(dws, 0);
		free_irq(dws->irq, master);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(chip3_spi_add_host);

void chip3_spi_remove_host(struct chip3_spi *dws)
{
	spi_shutdown_chip(dws);

	free_irq(dws->irq, dws->master);
}
EXPORT_SYMBOL_GPL(chip3_spi_remove_host);

int chip3_spi_suspend_host(struct chip3_spi *dws)
{
	int ret;

	ret = spi_controller_suspend(dws->master);
	if (ret)
		return ret;

	spi_shutdown_chip(dws);
	return 0;
}
EXPORT_SYMBOL_GPL(chip3_spi_suspend_host);

int chip3_spi_resume_host(struct chip3_spi *dws)
{
	int ret;

	spi_hw_init(&dws->master->dev, dws);
	ret = spi_controller_resume(dws->master);
	if (ret)
		dev_err(&dws->master->dev, "fail to start queue (%d)\n", ret);
	return ret;
}
EXPORT_SYMBOL_GPL(chip3_spi_resume_host);

MODULE_AUTHOR("Platform@wxiat.com");
MODULE_DESCRIPTION("Driver for Sunway CHIP3 SPI controller core");
MODULE_LICENSE("GPL");

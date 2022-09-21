/* SPDX-License-Identifier: GPL-2.0 */
#ifndef PHYTIUM_SPI_HEADER_H
#define PHYTIUM_SPI_HEADER_H

#include <linux/io.h>
#include <linux/scatterlist.h>
#include <linux/gpio.h>

#define CTRL0			0x00
#define SSIENR			0x08
#define SER			0x10
#define BAUDR			0x14
#define TXFLTR			0x18
#define TXFLR			0x20
#define RXFLR			0x24
#define IMR			0x2c
#define ISR			0x30
#define ICR			0x48
#define DR			0x60
#define GCSR			0x100

#define FRF_OFFSET		4
#define MODE_OFFSET		6
#define TMOD_OFFSET		8

#define TMOD_MASK		(0x3 << TMOD_OFFSET)
#define	TMOD_TR			0x0
#define TMOD_TO			0x1
#define TMOD_RO			0x2

#define INT_TXEI		(1 << 0)
#define INT_TXOI		(1 << 1)
#define INT_RXUI		(1 << 2)
#define INT_RXOI		(1 << 3)

struct phytium_spi {
	struct spi_master	*master;
	char			name[16];

	void __iomem		*regs;
	bool			global_cs;
	unsigned long		paddr;
	int			irq;
	u32			fifo_len;
	u32			max_freq;

	u32			reg_io_width;
	u16			bus_num;
	u16			num_cs;
	int			*cs;

	size_t			len;
	void			*tx;
	void			*tx_end;
	void			*rx;
	void			*rx_end;
	u8			n_bytes;
	irqreturn_t		(*transfer_handler)(struct phytium_spi *fts);
};

extern int phytium_spi_add_host(struct device *dev, struct phytium_spi *fts);
extern void phytium_spi_remove_host(struct phytium_spi *fts);
extern int phytium_spi_suspend_host(struct phytium_spi *fts);
extern int phytium_spi_resume_host(struct phytium_spi *fts);

#endif /* PHYTIUM_SPI_HEADER_H */

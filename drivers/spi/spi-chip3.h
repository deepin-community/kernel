/* SPDX-License-Identifier: GPL-2.0 */
#ifndef CHIP3_SPI_HEADER_H
#define CHIP3_SPI_HEADER_H

#include <linux/io.h>
#include <linux/scatterlist.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>

/* Register offsets */
#define CHIP3_SPI_CTRL0			(0x00<<7)
#define CHIP3_SPI_CTRL1			(0x04<<7)
#define CHIP3_SPI_SSIENR		(0x08<<7)
#define CHIP3_SPI_MWCR			(0x0c<<7)
#define CHIP3_SPI_SER			(0x10<<7)
#define CHIP3_SPI_BAUDR			(0x14<<7)
#define CHIP3_SPI_TXFLTR		(0x18<<7)
#define CHIP3_SPI_RXFLTR		(0x1c<<7)
#define CHIP3_SPI_TXFLR			(0x20<<7)
#define CHIP3_SPI_RXFLR			(0x24<<7)
#define CHIP3_SPI_SR			(0x28<<7)
#define CHIP3_SPI_IMR			(0x2c<<7)
#define CHIP3_SPI_ISR			(0x30<<7)
#define CHIP3_SPI_RISR			(0x34<<7)
#define CHIP3_SPI_TXOICR		(0x38<<7)
#define CHIP3_SPI_RXOICR		(0x3c<<7)
#define CHIP3_SPI_RXUICR		(0x40<<7)
#define CHIP3_SPI_MSTICR		(0x44<<7)
#define CHIP3_SPI_ICR			(0x48<<7)
#define CHIP3_SPI_DMACR			(0x4c<<7)
#define CHIP3_SPI_DMATDLR		(0x50<<7)
#define CHIP3_SPI_DMARDLR		(0x54<<7)
#define CHIP3_SPI_IDR			(0x58<<7)
#define CHIP3_SPI_VERSION		(0x5c<<7)
#define CHIP3_SPI_DR			(0x60<<7)

/* Bit fields in CTRLR0 */
#define SPI_DFS_OFFSET			0

#define SPI_FRF_OFFSET			4
#define SPI_FRF_SPI			0x0
#define SPI_FRF_SSP			0x1
#define SPI_FRF_MICROWIRE		0x2
#define SPI_FRF_RESV			0x3

#define SPI_MODE_OFFSET			6
#define SPI_SCPH_OFFSET			6
#define SPI_SCOL_OFFSET			7

#define SPI_TMOD_OFFSET			8
#define SPI_TMOD_MASK			(0x3 << SPI_TMOD_OFFSET)
#define	SPI_TMOD_TR			0x0		/* xmit & recv */
#define SPI_TMOD_TO			0x1		/* xmit only */
#define SPI_TMOD_RO			0x2		/* recv only */
#define SPI_TMOD_EPROMREAD		0x3		/* eeprom read mode */

#define SPI_SLVOE_OFFSET		10
#define SPI_SRL_OFFSET			11
#define SPI_CFS_OFFSET			12

/* Bit fields in SR, 7 bits */
#define SR_MASK				0x7f		/* cover 7 bits */
#define SR_BUSY				(1 << 0)
#define SR_TF_NOT_FULL			(1 << 1)
#define SR_TF_EMPT			(1 << 2)
#define SR_RF_NOT_EMPT			(1 << 3)
#define SR_RF_FULL			(1 << 4)
#define SR_TX_ERR			(1 << 5)
#define SR_DCOL				(1 << 6)

/* Bit fields in ISR, IMR, RISR, 7 bits */
#define SPI_INT_TXEI			(1 << 0)
#define SPI_INT_TXOI			(1 << 1)
#define SPI_INT_RXUI			(1 << 2)
#define SPI_INT_RXOI			(1 << 3)
#define SPI_INT_RXFI			(1 << 4)
#define SPI_INT_MSTI			(1 << 5)

/* Bit fields in DMACR */
#define SPI_DMA_RDMAE			(1 << 0)
#define SPI_DMA_TDMAE			(1 << 1)

/* TX RX interrupt level threshold, max can be 256 */
#define SPI_INT_THRESHOLD		32

/* The depth of the FIFO buffer is 256, so the max transfer length is 256. */
#define MAX_LEN				256

/* The mode of spi controller. */
#define SPI_TRANSMIT_RECEIVE		0x0c7
#define SPI_EEPROM_READ			0x3c7
#define SPI_TRANSMIT_ONLY		0x1c7

enum chip3_ssi_type {
	SSI_MOTO_SPI = 0,
	SSI_TI_SSP,
	SSI_NS_MICROWIRE,
};

struct chip3_spi;

struct chip3_spi {
	struct spi_controller	*master;
	enum chip3_ssi_type	type;

	void __iomem		*regs;
	unsigned long		paddr;
	int			irq;
	u32			fifo_len;	/* depth of the FIFO buffer */
	u32			max_freq;	/* max bus freq supported */

	u32			reg_io_width;	/* DR I/O width in bytes */
	u16			bus_num;
	u16			num_cs;		/* supported slave numbers */
	void (*set_cs)(struct spi_device *spi, bool enable);

	/* Current message transfer state info */
	size_t			len;
	void			*tx;
	unsigned int		tx_len;
	void			*rx;
	unsigned int		rx_len;
	u8			n_bytes;	/* current is a 1/2 bytes op */
	u32			current_freq;	/* frequency in hz */

	u8			buf[MAX_LEN];

	/* Bus interface info */
	void			*priv;
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs;
#endif
};

static inline u32 chip3_readl(struct chip3_spi *dws, u32 offset)
{
	return __raw_readl(dws->regs + offset);
}

static inline u16 chip3_readw(struct chip3_spi *dws, u32 offset)
{
	return __raw_readw(dws->regs + offset);
}

static inline void chip3_writel(struct chip3_spi *dws, u32 offset, u32 val)
{
	__raw_writel(val, dws->regs + offset);
}

static inline void chip3_writew(struct chip3_spi *dws, u32 offset, u16 val)
{
	__raw_writew(val, dws->regs + offset);
}

static inline u32 chip3_read_io_reg(struct chip3_spi *dws, u32 offset)
{
	switch (dws->reg_io_width) {
	case 2:
		return chip3_readw(dws, offset);
	case 4:
	default:
		return chip3_readl(dws, offset);
	}
}

static inline void chip3_write_io_reg(struct chip3_spi *dws, u32 offset, u32 val)
{
	switch (dws->reg_io_width) {
	case 2:
		chip3_writew(dws, offset, val);
		break;
	case 4:
	default:
		chip3_writel(dws, offset, val);
		break;
	}
}

static inline void spi_enable_chip(struct chip3_spi *dws, int enable)
{
	chip3_writel(dws, CHIP3_SPI_SSIENR, (enable ? 1 : 0));
}

static inline void spi_set_clk(struct chip3_spi *dws, u16 div)
{
	chip3_writel(dws, CHIP3_SPI_BAUDR, div);
}

/* Disable IRQ bits */
static inline void spi_mask_intr(struct chip3_spi *dws, u32 mask)
{
	u32 new_mask;

	new_mask = chip3_readl(dws, CHIP3_SPI_IMR) & ~mask;
	chip3_writel(dws, CHIP3_SPI_IMR, new_mask);
}

/* Enable IRQ bits */
static inline void spi_umask_intr(struct chip3_spi *dws, u32 mask)
{
	u32 new_mask;

	new_mask = chip3_readl(dws, CHIP3_SPI_IMR) | mask;
	chip3_writel(dws, CHIP3_SPI_IMR, new_mask);
}

/*
 * This does disable the SPI controller, interrupts, and re-enable the
 * controller back. Transmit and receive FIFO buffers are cleared when the
 * device is disabled.
 */
static inline void spi_reset_chip(struct chip3_spi *dws)
{
	spi_enable_chip(dws, 0);
	spi_mask_intr(dws, 0xff);
	spi_enable_chip(dws, 1);
}

static inline void spi_shutdown_chip(struct chip3_spi *dws)
{
	spi_enable_chip(dws, 0);
	spi_set_clk(dws, 0);
}

/*
 * Each SPI slave device to work with chip3_api controller should
 * has such a structure claiming its working mode (poll or PIO/DMA),
 * which can be save in the "controller_data" member of the
 * struct spi_device.
 */
struct chip3_spi_chip {
	u8 poll_mode;	/* 1 for controller polling mode */
	u8 type;	/* SPI/SSP/MicroWire */
	u8 chip_select;
	void (*cs_control)(u32 command);
};

extern int chip3_spi_add_host(struct device *dev, struct chip3_spi *dws);
extern void chip3_spi_remove_host(struct chip3_spi *dws);
extern int chip3_spi_suspend_host(struct chip3_spi *dws);
extern int chip3_spi_resume_host(struct chip3_spi *dws);

/* platform related setup */
extern int chip3_spi_mid_init(struct chip3_spi *dws); /* Intel MID platforms */
#endif /* CHIP3_SPI_HEADER_H */

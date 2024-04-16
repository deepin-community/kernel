/* SPDX-License-Identifier: GPL-2.0 */
#ifndef SPI_CHIP_HEADER_H
#define SPI_CHIP_HEADER_H

#include <linux/io.h>
#include <linux/scatterlist.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>

/* Register offsets */
#define SPI_CHIP_CTRL0			0x00
#define SPI_CHIP_CTRL1			0x04
#define SPI_CHIP_SSIENR			0x08
#define SPI_CHIP_MWCR			0x0c
#define SPI_CHIP_SER			0x10
#define SPI_CHIP_BAUDR			0x14
#define SPI_CHIP_TXFLTR			0x18
#define SPI_CHIP_RXFLTR			0x1c
#define SPI_CHIP_TXFLR			0x20
#define SPI_CHIP_RXFLR			0x24
#define SPI_CHIP_SR			0x28
#define SPI_CHIP_IMR			0x2c
#define SPI_CHIP_ISR			0x30
#define SPI_CHIP_RISR			0x34
#define SPI_CHIP_TXOICR			0x38
#define SPI_CHIP_RXOICR			0x3c
#define SPI_CHIP_RXUICR			0x40
#define SPI_CHIP_MSTICR			0x44
#define SPI_CHIP_ICR			0x48
#define SPI_CHIP_DMACR			0x4c
#define SPI_CHIP_DMATDLR		0x50
#define SPI_CHIP_DMARDLR		0x54
#define SPI_CHIP_IDR			0x58
#define SPI_CHIP_VERSION		0x5c
#define SPI_CHIP_DR			0x60

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

#define SPI_PLAT			0x1
#define SPI_PCI				0x2

enum spi_ssi_type {
	SSI_MOTO_SPI = 0,
	SSI_TI_SSP,
	SSI_NS_MICROWIRE,
};


struct spi_chip;

struct spi_chip {
	struct spi_controller	*master;
	enum spi_ssi_type	type;

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
	int			flags;

	u8			buf[MAX_LEN];

	/* Bus interface info */
	void			*priv;
#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs;
#endif
};

static inline u32 spi_readl(struct spi_chip *spi_chip, u32 offset)
{
	if (spi_chip->flags & SPI_PLAT)
		offset <<= 7;

	return __raw_readl(spi_chip->regs + offset);
}

static inline u16 spi_readw(struct spi_chip *spi_chip, u32 offset)
{
	if (spi_chip->flags & SPI_PLAT)
		offset <<= 7;

	return __raw_readw(spi_chip->regs + offset);
}

static inline void spi_writel(struct spi_chip *spi_chip, u32 offset, u32 val)
{
	if (spi_chip->flags & SPI_PLAT)
		offset <<= 7;

	__raw_writel(val, spi_chip->regs + offset);
}

static inline void spi_writew(struct spi_chip *spi_chip, u32 offset, u16 val)
{
	if (spi_chip->flags & SPI_PLAT)
		offset <<= 7;

	__raw_writew(val, spi_chip->regs + offset);
}

static inline u32 spi_read_io_reg(struct spi_chip *spi_chip, u32 offset)
{
	switch (spi_chip->reg_io_width) {
	case 2:
		return spi_readw(spi_chip, offset);
	case 4:
	default:
		return spi_readl(spi_chip, offset);
	}
}

static inline void spi_write_io_reg(struct spi_chip *spi_chip, u32 offset,
				    u32 val)
{
	switch (spi_chip->reg_io_width) {
	case 2:
		spi_writew(spi_chip, offset, val);
		break;
	case 4:
	default:
		spi_writel(spi_chip, offset, val);
		break;
	}
}

static inline void spi_enable_chip(struct spi_chip *spi_chip, int enable)
{
	spi_writel(spi_chip, SPI_CHIP_SSIENR, (enable ? 1 : 0));
}

static inline void spi_set_clk(struct spi_chip *spi_chip, u16 div)
{
	spi_writel(spi_chip, SPI_CHIP_BAUDR, div);
}

/* Disable IRQ bits */
static inline void spi_mask_intr(struct spi_chip *spi_chip, u32 mask)
{
	u32 new_mask;

	new_mask = spi_readl(spi_chip, SPI_CHIP_IMR) & ~mask;
	spi_writel(spi_chip, SPI_CHIP_IMR, new_mask);
}

/* Enable IRQ bits */
static inline void spi_umask_intr(struct spi_chip *spi_chip, u32 mask)
{
	u32 new_mask;

	new_mask = spi_readl(spi_chip, SPI_CHIP_IMR) | mask;
	spi_writel(spi_chip, SPI_CHIP_IMR, new_mask);
}

/*
 * This does disable the SPI controller, interrupts, and re-enable the
 * controller back. Transmit and receive FIFO buffers are cleared when the
 * device is disabled.
 */
static inline void spi_reset_chip(struct spi_chip *spi_chip)
{
	spi_enable_chip(spi_chip, 0);
	spi_mask_intr(spi_chip, 0xff);
	spi_enable_chip(spi_chip, 1);
}

static inline void spi_shutdown_chip(struct spi_chip *spi_chip)
{
	spi_enable_chip(spi_chip, 0);
	spi_set_clk(spi_chip, 0);
}

/*
 * Each SPI slave device to work with spi_api controller should
 * has such a structure claiming its working mode (poll or PIO/DMA),
 * which can be save in the "controller_data" member of the
 * struct spi_device.
 */
struct spi_chip_info {
	u8 poll_mode;	/* 1 for controller polling mode */
	u8 type;	/* SPI/SSP/MicroWire */
	u8 chip_select;
	void (*cs_control)(u32 command);
};

extern int spi_chip_add_host(struct device *dev, struct spi_chip *spi_chip);
extern void spi_chip_remove_host(struct spi_chip *spi_chip);
extern int spi_chip_suspend_host(struct spi_chip *spi_chip);
extern int spi_chip_resume_host(struct spi_chip *spi_chip);

/* platform related setup */
extern int spi_ich_init(struct spi_chip *spi_chip);
#endif /* SPI_CHIP_HEADER_H */

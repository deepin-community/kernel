// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium SPI core controller driver.
 *
 * Copyright (c) 2023-2024, Phytium Technology Co., Ltd..
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
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
#include <linux/of_platform.h>
#include <linux/property.h>
#include <linux/acpi.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/mtd/spi-nor.h>
#include "spi-phytium.h"

#define SPI_SHOW_MSG_DEBUG 0

static void spi_phytium_show_msg(struct msg *info)
{
	if (SPI_SHOW_MSG_DEBUG) {
		pr_err("module:0x%4x, cmd:0x%04x, sub:0x%04x\n",
				info->seq, info->cmd_id, info->cmd_subid);

		pr_err("0x%02x 0x%02x 0x%02x 0x%02x", info->data[0],
				info->data[1], info->data[2], info->data[3]);

		pr_err("0x%02x 0x%02x 0x%02x 0x%02x", info->data[4],
				info->data[5], info->data[6], info->data[7]);

		pr_err("0x%02x 0x%02x 0x%02x 0x%02x", info->data[8],
				info->data[9], info->data[10], info->data[11]);

		pr_err("0x%02x 0x%02x 0x%02x 0x%02x", info->data[12],
				info->data[13], info->data[14], info->data[15]);
	}
}

static void *memcpy_byte(void *_dest, const void *_src, size_t sz)
{
	while (sz >= 8) {
		*(u64 *)_dest = *(u64 *)_src;
		_dest += 8;
		_src += 8;
		sz -= 8;
	}

	while (sz) {
		*(u8 *)_dest = *(u8 *)_src;
		_dest++;
		_src++;
		sz--;
	}

	return _dest;
}

static int spi_phytium_print_status(struct phytium_spi *fts, u8 status0,
		u8 status1)
{
	if (status1 == 0)
		return 0;

	switch (status1) {
	case 1:
		pr_err("SPI Bus is busy.\n");
		break;
	case 2:
		pr_err("DMA queue transfer error.\n");
		break;
	case 3:
		pr_err("DMA transfer timeout.\n");
		break;
	case 4:
		pr_err("Operating flash timeout.\n");
		break;
	case 5:
		pr_err("spi_tx channel:DMA initialization failure.\n");
		break;
	case 6:
		pr_err("spi_rx channel:DMA initialization failure.\n");
		break;
	case 7:
		pr_err("DMA queue initialization failure.\n");
		break;
	default:
		pr_err("status=0x%x,Unknown error\n", status1);
		break;
	}

	if (fts->debug_enabled)
		fts->handle_debug_err(fts);

	return -1;
}

static int spi_phytium_check_result(struct phytium_spi *fts)
{
	unsigned long ms;
	struct msg *msg = &fts->msg_buf;

	ms = wait_for_completion_timeout(&fts->cmd_completion,
					 msecs_to_jiffies(SPI_MASTER_TIMEOUT));

	if (ms == 0) {
		dev_err(&fts->master->dev, "SPI controller timed out\n");
		return -1;
	}

	/* read the response status only after the firmware completed */
	memcpy_fromio(&fts->msg_buf, fts->msg, sizeof(struct msg));

	return spi_phytium_print_status(fts, msg->status0, msg->status1);
}

static int spi_phytium_set(struct phytium_spi *fts)
{
	int ret;

	spi_phytium_show_msg(&fts->msg_buf);
	memcpy_toio(fts->msg, &fts->msg_buf, sizeof(struct msg));
	reinit_completion(&fts->cmd_completion);
	phytium_write_regfile(fts, SPI_REGFILE_AP2RV_INTR_STATE, 0x10);
	ret = spi_phytium_check_result(fts);

	return ret;
}

int spi_phytium_default(struct phytium_spi *fts)
{
	int ret;

	memset(&fts->msg_buf, 0, sizeof(struct msg));

	fts->msg_buf.cmd_id = PHYTSPI_MSG_CMD_DEFAULT;

	spi_phytium_show_msg(&fts->msg_buf);
	memcpy_toio(fts->msg, &fts->msg_buf, sizeof(struct msg));
	reinit_completion(&fts->cmd_completion);
	phytium_write_regfile(fts, SPI_REGFILE_AP2RV_INTR_STATE, 0x10);
	ret = spi_phytium_check_result(fts);

	return ret;
}
EXPORT_SYMBOL_GPL(spi_phytium_default);

static void spi_phytium_set_subid(struct phytium_spi *fts, u16 sub_cmd)
{
	fts->msg_buf.cmd_id = PHYTSPI_MSG_CMD_SET;
	fts->msg_buf.cmd_subid = sub_cmd;
}

void spi_phytium_set_cmd8(struct phytium_spi *fts, u16 sub_cmd,
		u8 data)
{
	memset(&fts->msg_buf, 0, sizeof(struct msg));
	spi_phytium_set_subid(fts, sub_cmd);
	fts->msg_buf.data[0] = data;
	spi_phytium_show_msg(&fts->msg_buf);
	memcpy_toio(fts->msg, &fts->msg_buf, sizeof(struct msg));
	reinit_completion(&fts->cmd_completion);
	phytium_write_regfile(fts, SPI_REGFILE_AP2RV_INTR_STATE, 0x10);
	spi_phytium_check_result(fts);
}
EXPORT_SYMBOL_GPL(spi_phytium_set_cmd8);

void spi_phytium_set_cmd16(struct phytium_spi *fts, u16 sub_cmd,
		u16 data)
{
	u16 *cp_data = (u16 *)&fts->msg_buf.data[0];

	memset(&fts->msg_buf, 0, sizeof(struct msg));
	spi_phytium_set_subid(fts, sub_cmd);
	*cp_data = data;
	spi_phytium_show_msg(&fts->msg_buf);
	memcpy_toio(fts->msg, &fts->msg_buf, sizeof(struct msg));
	reinit_completion(&fts->cmd_completion);
	phytium_write_regfile(fts, SPI_REGFILE_AP2RV_INTR_STATE, 0x10);
	spi_phytium_check_result(fts);
}
EXPORT_SYMBOL_GPL(spi_phytium_set_cmd16);

void spi_phytium_set_cmd32(struct phytium_spi *fts, u16 sub_cmd,
		u32 data)
{
	u32 *cp_data = (u32 *)&fts->msg_buf.data[0];

	memset(&fts->msg_buf, 0, sizeof(struct msg));
	spi_phytium_set_subid(fts, sub_cmd);
	*cp_data = data;
	spi_phytium_show_msg(&fts->msg_buf);
	memcpy_toio(fts->msg, &fts->msg_buf, sizeof(struct msg));
	reinit_completion(&fts->cmd_completion);
	phytium_write_regfile(fts, SPI_REGFILE_AP2RV_INTR_STATE, 0x10);
	spi_phytium_check_result(fts);
}
EXPORT_SYMBOL_GPL(spi_phytium_set_cmd32);

void spi_phytium_write_pre(struct phytium_spi *fts, u8 cs, u8 dfs, u8 mode, u8 tmode,
		u8 flags, u8 spi_write_flag)
{
	u32 len;
	void __iomem *smem_tx;
	u8 first = 1;
	u64 tx_addr;
	void __iomem *msg_io;
	struct msg msg_shadow;

	len = min((u32)(fts->tx_end - fts->tx), (u32)SPI_TRANS_DATA_SIZE);
	msg_io = fts->msg + (sizeof(struct msg) + FLASH_PAGE_SIZE) * spi_write_flag;
	memset(&msg_shadow, 0, sizeof(struct msg));

	msg_shadow.cmd_id = PHYTSPI_MSG_CMD_DATA;
	if (spi_write_flag) {
		if (len > 16 && fts->dma_get_ddrdata)
			msg_shadow.cmd_subid = PHYTSPI_MSG_CMD_DATA_FLASH_DMA_TX;
		else
			msg_shadow.cmd_subid = PHYTSPI_MSG_CMD_DATA_FLASH_TX;
	} else {
		if (len > 16 && fts->dma_get_ddrdata)
			msg_shadow.cmd_subid = PHYTSPI_MSG_CMD_DATA_DMA_TX;
		else
			msg_shadow.cmd_subid = PHYTSPI_MSG_CMD_DATA_TX;
	}

	if (len > 16 && fts->dma_get_ddrdata) {
		tx_addr = dma_map_single(&fts->master->dev, fts->tx, len,
					 DMA_TO_DEVICE);
		if (dma_mapping_error(&fts->master->dev, tx_addr)) {
			dev_err(&fts->master->dev, "tx address translation failed\n");
			return;
		}
		*(u64 *)&msg_shadow.data[0] = tx_addr;
	} else {
		smem_tx = msg_io + sizeof(struct msg);
		memcpy_toio(smem_tx, fts->tx, len);
		*(u64 *)&msg_shadow.data[0] = sizeof(struct msg);
	}

	*(u32 *)&msg_shadow.data[8] = len;
	fts->tx += len;
	msg_shadow.data[12] = cs;
	msg_shadow.data[13] = dfs;
	msg_shadow.data[14] = mode;
	msg_shadow.data[15] = tmode;
	msg_shadow.data[16] = flags;
	msg_shadow.data[17] = first;
	memcpy_toio(msg_io, &msg_shadow, sizeof(struct msg));
	first = 0;
}
EXPORT_SYMBOL_GPL(spi_phytium_write_pre);

int spi_phytium_flash_erase(struct phytium_spi *fts, u8 cs, u8 dfs, u8 mode,
		u8 tmode, u8 flags, u8 cmd)
{
	u32 len;
	void __iomem *smem_tx;
	u8 first = 1;
	u8 cmd_addr[8];
	int ret;

	len = (u32)(fts->tx_end - fts->tx);

	memset(&fts->msg_buf, 0, sizeof(struct msg));

	fts->msg_buf.cmd_id = PHYTSPI_MSG_CMD_DATA;

	if (cmd == SPINOR_OP_BE_4K || cmd == SPINOR_OP_CHIP_ERASE)
		fts->msg_buf.cmd_subid = PHYTSPI_MSG_CMD_FLASH_ERASE;
	else if (cmd == SPINOR_OP_READ || cmd == SPINOR_OP_READ_FAST ||
			cmd == SPINOR_OP_READ_4B || cmd == SPINOR_OP_READ_FAST_4B)
		fts->msg_buf.cmd_subid = PHYTSPI_MSG_CMD_DATA_TX;

	smem_tx = fts->msg + sizeof(struct msg);

	cmd_addr[0] = cmd;
	if (cmd == SPINOR_OP_BE_4K || cmd == SPINOR_OP_READ ||
			cmd == SPINOR_OP_READ_FAST || cmd == SPINOR_OP_READ_4B ||
			cmd == SPINOR_OP_READ_FAST_4B) {
		memcpy_byte((void *)&cmd_addr[1], fts->tx, len);
		memcpy_toio(smem_tx, (void *)&cmd_addr[0], len + 1);
		*(u32 *)&fts->msg_buf.data[8] = len + 1;
	} else if (cmd == SPINOR_OP_CHIP_ERASE) {
		memcpy_toio(smem_tx, (void *)&cmd_addr[0], 1);
		*(u32 *)&fts->msg_buf.data[8] = len;
	}

	*(u32 *)&fts->msg_buf.data[0] = sizeof(struct msg);
	fts->tx += len;
	fts->msg_buf.data[12] = cs;
	fts->msg_buf.data[13] = dfs;
	fts->msg_buf.data[14] = mode;
	fts->msg_buf.data[15] = tmode;
	fts->msg_buf.data[16] = flags;
	fts->msg_buf.data[17] = first;

	ret = spi_phytium_set(fts);
	if (ret) {
		dev_err(&fts->master->dev, "AP <-> RV interaction failed\n");
		return ret;
	}
	return ret;
}
EXPORT_SYMBOL_GPL(spi_phytium_flash_erase);

int spi_phytium_flash_write(struct phytium_spi *fts, u8 cmd)
{
	u8 cmd_addr[8] = {0};

	cmd_addr[0] = fts->len + 1;
	cmd_addr[1] = cmd;
	memcpy_byte((void *)&cmd_addr[2], fts->tx, fts->len);

	fts->msg_buf.data[18] = cmd_addr[0];
	fts->msg_buf.data[19] = cmd_addr[1];
	fts->msg_buf.data[20] = cmd_addr[2];
	fts->msg_buf.data[21] = cmd_addr[3];
	fts->msg_buf.data[22] = cmd_addr[4];
	fts->msg_buf.data[23] = cmd_addr[5];

	return 0;
}
EXPORT_SYMBOL_GPL(spi_phytium_flash_write);

int spi_phytium_write(struct phytium_spi *fts, u8 cs, u8 dfs, u8 mode,
		u8 tmode, u8 flags, u8 spi_write_flag)
{
	int ret = 0;
	u32 len;
	void __iomem *smem_tx;
	u8 first = 1;
	u64 tx_addr;

	if (spi_write_flag == 1) {
		ret = spi_phytium_set(fts);
		if (ret) {
			dev_err(&fts->master->dev, "AP <-> RV interaction failed\n");
			return ret;
		}
	}

	do {
		len = min_t(u32, (u32)(fts->tx_end - fts->tx),
				(u32)SPI_TRANS_DATA_SIZE);

		fts->msg_buf.cmd_id = PHYTSPI_MSG_CMD_DATA;

		if (spi_write_flag == 2) {
			if (len > 16 && fts->dma_get_ddrdata)
				fts->msg_buf.cmd_subid = PHYTSPI_MSG_CMD_DATA_FLASH_DMA_TX;
			else
				fts->msg_buf.cmd_subid = PHYTSPI_MSG_CMD_DATA_FLASH_TX;
		} else {
			if (len > 16 && fts->dma_get_ddrdata)
				fts->msg_buf.cmd_subid = PHYTSPI_MSG_CMD_DATA_DMA_TX;
			else
				fts->msg_buf.cmd_subid = PHYTSPI_MSG_CMD_DATA_TX;
		}

		if (len > 16 && fts->dma_get_ddrdata) {
			tx_addr = dma_map_single(&fts->master->dev, fts->tx, len,
						 DMA_TO_DEVICE);
			if (dma_mapping_error(&fts->master->dev, tx_addr)) {
				dev_err(&fts->master->dev, "tx address translation failed\n");
				return -1;
			}
			*(u64 *)&fts->msg_buf.data[0] = tx_addr;
		} else {
			smem_tx = fts->msg + sizeof(struct msg);
			memcpy_toio(smem_tx, fts->tx, len);
			*(u64 *)&fts->msg_buf.data[0] = sizeof(struct msg);
		}

		*(u32 *)&fts->msg_buf.data[8] = len;
		fts->tx += len;
		fts->msg_buf.data[12] = cs;
		fts->msg_buf.data[13] = dfs;
		fts->msg_buf.data[14] = mode;
		fts->msg_buf.data[15] = tmode;
		fts->msg_buf.data[16] = flags;
		fts->msg_buf.data[17] = first;
		ret = spi_phytium_set(fts);
		if (len > 16 && fts->dma_get_ddrdata)
			dma_unmap_single(&fts->master->dev, tx_addr, len,
					 DMA_TO_DEVICE);
		if (ret) {
			dev_err(&fts->master->dev, "AP <-> RV interaction failed\n");
			return ret;
		}

		first = 0;
	} while (fts->tx_end > fts->tx);

	return ret;
}
EXPORT_SYMBOL_GPL(spi_phytium_write);

int spi_phytium_read(struct phytium_spi *fts, u8 cs, u8 dfs, u8 mode,
		u8 tmode, u8 flags)
{
	int ret;
	u32 len;
	void __iomem *smem_rx;
	u8 first = 1;
	u64 rx_addr;

	do {
		if (fts->dma_get_ddrdata)
			len = min_t(u32, (u32)(fts->rx_end - fts->rx),
					(u32)SPI_TRANS_DATA_SIZE);
		else
			len = min_t(u32, (u32)(fts->rx_end - fts->rx), 128);

		fts->msg_buf.cmd_id = PHYTSPI_MSG_CMD_DATA;

		smem_rx = fts->msg + sizeof(struct msg);

		if (len > 16 && fts->dma_get_ddrdata) {
			fts->msg_buf.cmd_subid = PHYTSPI_MSG_CMD_DATA_DMA_RX;
			rx_addr = dma_map_single(&fts->master->dev, fts->rx, len,
						 DMA_FROM_DEVICE);
			if (dma_mapping_error(&fts->master->dev, rx_addr)) {
				dev_err(&fts->master->dev, "rx address translation failed\n");
				return -1;
			}
			*(u64 *)&fts->msg_buf.data[0] = rx_addr;
		} else {
			fts->msg_buf.cmd_subid = PHYTSPI_MSG_CMD_DATA_RX;
			*(u64 *)&fts->msg_buf.data[0] = sizeof(struct msg);
		}

		*(u32 *)&fts->msg_buf.data[8] = len;
		fts->msg_buf.data[12] = cs;
		fts->msg_buf.data[13] = dfs;
		fts->msg_buf.data[14] = mode;
		fts->msg_buf.data[15] = tmode;
		if (fts->rx_end <= fts->rx + len)
			fts->msg_buf.data[16] = flags;
		else if (first == 1)
			fts->msg_buf.data[16] = 1;
		else
			fts->msg_buf.data[16] = 0;
		fts->msg_buf.data[17] = first;
		ret = spi_phytium_set(fts);
		if (len > 16 && fts->dma_get_ddrdata)
			dma_unmap_single(&fts->master->dev, rx_addr, len,
					 DMA_FROM_DEVICE);
		if (ret) {
			dev_err(&fts->master->dev, "AP <-> RV interaction failed\n");
			return ret;
		}
		if (len <= 16 || !fts->dma_get_ddrdata)
			memcpy_fromio(fts->rx, smem_rx, len);

		fts->rx += len;
		first = 0;
	} while (fts->rx_end > fts->rx);

	return ret;
}
EXPORT_SYMBOL_GPL(spi_phytium_read);

int spi_phytium_xfer(struct phytium_spi *fts, u8 cs, u8 dfs, u8 mode,
		u8 tmode, u8 flags)
{
	int ret;
	u32 len;
	void __iomem *smem_tx, *smem_rx;
	u8 first = 1;
	u64 tx_addr, rx_addr;

	do {
		if (fts->dma_get_ddrdata)
			len = min_t(u32, (u32)(fts->rx_end - fts->rx),
					(u32)SPI_TRANS_DATA_SIZE);
		else
			len = min_t(u32, (u32)(fts->rx_end - fts->rx), 128);

		fts->msg_buf.cmd_id = PHYTSPI_MSG_CMD_DATA;

		smem_tx = fts->msg + sizeof(struct msg);
		smem_rx = fts->msg + sizeof(struct msg) + 128;

		if (len > 16 && fts->dma_get_ddrdata) {
			fts->msg_buf.cmd_subid = PHYTSPI_MSG_CMD_DATA_DMA_XFER;
			tx_addr = dma_map_single(&fts->master->dev, fts->tx, len,
						 DMA_TO_DEVICE);
			if (dma_mapping_error(&fts->master->dev, tx_addr)) {
				dev_err(&fts->master->dev, "tx address translation failed\n");
				return -1;
			}
			rx_addr = dma_map_single(&fts->master->dev, fts->rx, len,
						 DMA_FROM_DEVICE);
			if (dma_mapping_error(&fts->master->dev, rx_addr)) {
				dma_unmap_single(&fts->master->dev, tx_addr, len,
						 DMA_TO_DEVICE);
				dev_err(&fts->master->dev, "rx address translation failed\n");
				return -1;
			}

			*(u64 *)&fts->msg_buf.data[0] = tx_addr;
			*(u64 *)&fts->msg_buf.data[8] = rx_addr;
		} else {
			fts->msg_buf.cmd_subid = PHYTSPI_MSG_CMD_DATA_XFER;
			memcpy_toio(smem_tx, fts->tx, len);
			*(u64 *)&fts->msg_buf.data[0] = sizeof(struct msg);
			*(u64 *)&fts->msg_buf.data[8] = sizeof(struct msg) + 128;
		}

		*(u32 *)&fts->msg_buf.data[16] = len;
		fts->msg_buf.data[20] = cs;
		fts->msg_buf.data[21] = dfs;
		fts->msg_buf.data[22] = mode;
		fts->msg_buf.data[23] = tmode;
		if (first == 1)
			fts->msg_buf.data[24] = 1;
		else
			fts->msg_buf.data[24] = flags;
		ret = spi_phytium_set(fts);
		if (len > 16 && fts->dma_get_ddrdata) {
			dma_unmap_single(&fts->master->dev, tx_addr, len,
					 DMA_TO_DEVICE);
			dma_unmap_single(&fts->master->dev, rx_addr, len,
					 DMA_FROM_DEVICE);
		}
		if (ret) {
			dev_err(&fts->master->dev, "AP <-> RV interaction failed\n");
			return ret;
		}
		if (len <= 16 || !fts->dma_get_ddrdata)
			memcpy_fromio(fts->rx, smem_rx, len);

		fts->tx += len;
		fts->rx += len;
		first = 0;
	} while (fts->rx_end > fts->rx);

	return ret;
}
EXPORT_SYMBOL_GPL(spi_phytium_xfer);

MODULE_AUTHOR("Peng Min <pengmin1540@phytium.com.cn>");
MODULE_DESCRIPTION("Phytium SPI adapter core");
MODULE_LICENSE("GPL");

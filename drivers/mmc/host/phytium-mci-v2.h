/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Driver for Phytium Multimedia Card Interface
 *
 * Copyright (C) 2024 Phytium Technology Co., Ltd.
 */

#ifndef __PHYTIUM_MCI_H
#define __PHYTIUM_MCI_H

#include <linux/scatterlist.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/interrupt.h>
#include <linux/mmc/host.h>

/*------------------------------------------------------*/
/* Common Definition					*/
/*------------------------------------------------------*/
#define SD_BLOCK_SIZE		512
#define MAX_BD_NUM		128
#define MCI_CLK			1200000000
#define MCI_REALEASE_MEM	0x1

#define MCI_PREPARE_FLAG	(0x1 << 0)
#define MCI_ASYNC_FLAG		(0x1 << 1)
#define MCI_MMAP_FLAG		(0x1 << 2)

#define MCI_F_MIN		400000
#define MCI_F_MAX		50000000

/* MCI_RAW_INTS mask */
#define MCI_RAW_INTS_CD			(0x1 << 0) /* W1C */
#define MCI_RAW_INTS_RE			(0x1 << 1) /* W1C */
#define MCI_RAW_INTS_CMD		(0x1 << 2) /* W1C */
#define MCI_RAW_INTS_DTO		(0x1 << 3) /* W1C */
#define MCI_RAW_INTS_TXDR		(0x1 << 4) /* W1C */
#define MCI_RAW_INTS_RXDR		(0x1 << 5) /* W1C */
#define MCI_RAW_INTS_RCRC		(0x1 << 6) /* W1C */
#define MCI_RAW_INTS_DCRC		(0x1 << 7) /* W1C */
#define MCI_RAW_INTS_RTO		(0x1 << 8) /* W1C */
#define MCI_RAW_INTS_DRTO		(0x1 << 9) /* W1C */
#define MCI_RAW_INTS_HTO		(0x1 << 10) /* W1C */
#define MCI_RAW_INTS_FRUN		(0x1 << 11) /* W1C */
#define MCI_RAW_INTS_HLE		(0x1 << 12) /* W1C */
#define MCI_RAW_INTS_SBE_BCI		(0x1 << 13) /* W1C */
#define MCI_RAW_INTS_ACD		(0x1 << 14) /* W1C */
#define MCI_RAW_INTS_EBE		(0x1 << 15) /* W1C */
#define MCI_RAW_INTS_SDIO		(0x1 << 16)  /* W1C */

/* MCI_MASKED_INTS mask */
#define MCI_MASKED_INTS_CD		(0x1 << 0) /* RO */
#define MCI_MASKED_INTS_RE		(0x1 << 1) /* RO */
#define MCI_MASKED_INTS_CMD		(0x1 << 2) /* RO */
#define MCI_MASKED_INTS_DTO		(0x1 << 3) /* RO */
#define MCI_MASKED_INTS_TXDR		(0x1 << 4) /* RO */
#define MCI_MASKED_INTS_RXDR		(0x1 << 5) /* RO */
#define MCI_MASKED_INTS_RCRC		(0x1 << 6) /* RO */
#define MCI_MASKED_INTS_DCRC		(0x1 << 7) /* RO */
#define MCI_MASKED_INTS_RTO		(0x1 << 8) /* RO */
#define MCI_MASKED_INTS_DRTO		(0x1 << 9) /* RO */
#define MCI_MASKED_INTS_HTO		(0x1 << 10) /* RO */
#define MCI_MASKED_INTS_FRUN		(0x1 << 11) /* RO */
#define MCI_MASKED_INTS_HLE		(0x1 << 12) /* RO */
#define MCI_MASKED_INTS_SBE_BCI		(0x1 << 13) /* RO */
#define MCI_MASKED_INTS_ACD		(0x1 << 14) /* RO */
#define MCI_MASKED_INTS_EBE		(0x1 << 15) /* RO */
#define MCI_MASKED_INTS_SDIO		(0x1 << 16) /* RO */

/* MCI_INT_MASK mask */
#define MCI_INT_MASK_CD			(0x1 << 0) /* RW */
#define MCI_INT_MASK_RE			(0x1 << 1) /* RW */
#define MCI_INT_MASK_CMD		(0x1 << 2) /* RW */
#define MCI_INT_MASK_DTO		(0x1 << 3) /* RW */
#define MCI_INT_MASK_TXDR		(0x1 << 4) /* RW */
#define MCI_INT_MASK_RXDR		(0x1 << 5) /* RW */
#define MCI_INT_MASK_RCRC		(0x1 << 6) /* RW */
#define MCI_INT_MASK_DCRC		(0x1 << 7) /* RW */
#define MCI_INT_MASK_RTO		(0x1 << 8) /* RW */
#define MCI_INT_MASK_DRTO		(0x1 << 9) /* RW */
#define MCI_INT_MASK_HTO		(0x1 << 10) /* RW */
#define MCI_INT_MASK_FRUN		(0x1 << 11) /* RW */
#define MCI_INT_MASK_HLE		(0x1 << 12) /* RW */
#define MCI_INT_MASK_SBE_BCI		(0x1 << 13) /* RW */
#define MCI_INT_MASK_ACD		(0x1 << 14) /* RW */
#define MCI_INT_MASK_EBE		(0x1 << 15) /* RW */
#define MCI_INT_MASK_SDIO		(0x1 << 16) /* RW */

#define MCI_CARD_DETECT			0x30 /* the card detect reg */
#define MCI_DATA			0x34 /* the data FIFO access */

#define MCI_DEBUG			0x58 /* debug function */

/* MCI_DEBUG mask */
#define MCI_DEBUG_ENABLE		(0x1 << 0) /* RW */
#define MCI_ALIVE_ENABLE		(0x1 << 1) /* RW */
#define MCI_ALIVE			(0x1 << 2) /* RW */
#define MCI_HAVE_LOG			(0x1 << 3) /* RW */
#define MCI_SIZE			(0xf << 4) /* RW */
#define MCI_ADDR			(0x3fffff << 8) /* RW */

#define AP_TO_RV_MAX_DATA		64
#define MCI_HW_RESET			0x00
#define MCI_ADMA_RESET			0x01
#define MCI_INIT			0x00
#define MCI_INIT_HW			0x01
#define MCI_DEINIT_HW			0x02
#define MCI_START_CMD			0x03
#define MCI_START_DATA			0x04
#define MCI_OPS_SET_IOS			0x05
#define MCI_SDIO_IRQ_EN			0x06
#define MCI_OPS_SWITCH_VOLT		0x07
#define MCI_GET_CD			0x00
#define MCI_GET_CARD_BUSY		0x01
#define MCI_GET_STATUS			0x02
#define MCI_GET_RO			0x03
#define MCI_CD_IRQ			0x00
#define MCI_ERR_IRQ			0x01
#define MCI_CMD_NEXT			0x02
#define MCI_DATA_NEXT			0x03

#define RING_MAX			20
#define RING_MAX_RV			10

#define COMPLETE_TIMEOUT		200

#define MMC_TX_HEAD			0x0
#define MMC_TX_TAIL			0x4
#define MMC_RX_HEAD			0x8
#define MMC_RX_TAIL			0xc
#define MMC_TX_TAIL_INT			(0x1 << 16) /* RW */

#define MMC_RV2AP_INT_MASK		0x028
#define MMC_TXRING_HEAD_INT_MASK	0x1 /* RW */
#define MMC_RXRING_TAIL_INT_MASK	(0x1 << 1) /* RW */

#define MMC_RV2AP_INT			0x02c
#define MMC_TXRING_HEAD_INT		0x1 /* RW */
#define MMC_RXRING_TAIL_INT		(0x1 << 1) /* RW */
/*--------------------------------------*/
/*		Structure Type		*/
/*--------------------------------------*/
/* Maximum segments assuming a 512KiB maximum requisition */
/* size and a minimum4KiB page size. */
#define MCI_MAX_SEGS			128
/* ADMA2 64-bit DMA descriptor size */
#define ADMA2_64_DESC_SZ		32

/* Each descriptor can transfer up to 4KB of data in chained mode */
/*ADMA2 64-bit descriptor.*/
struct phytium_adma2_64_desc {
	u32 attribute;
#define IDMAC_DES0_DIC	BIT(1)
#define IDMAC_DES0_LD	BIT(2)
#define IDMAC_DES0_FD	BIT(3)
#define IDMAC_DES0_CH	BIT(4)
#define IDMAC_DES0_ER	BIT(5)
#define IDMAC_DES0_CES	BIT(30)
#define IDMAC_DES0_OWN	BIT(31)
	u32	NON1;
	u32	len;
	u32	NON2;
	u32	addr_lo; /* Lower 32-bits of Buffer Address Pointer 1*/
	u32	addr_hi; /* Upper 32-bits of Buffer Address Pointer 1*/
	u32	desc_lo; /* Lower 32-bits of Next Descriptor Address */
	u32	desc_hi; /* Upper 32-bits of Next Descriptor Address */
} __packed __aligned(4);

struct phytium_mci_dma {
	struct scatterlist *sg;	/* I/O scatter list */
	/* ADMA descriptor table, pointer to adma_table array */
	struct phytium_adma2_64_desc *adma_table;
	/* Mapped ADMA descr. table, the physical address of adma_table array */
	dma_addr_t adma_addr;
	unsigned int desc_sz;	/* ADMA descriptor size */
};

enum adtc_t {
	COMMOM_ADTC	= 0,
	BLOCK_RW_ADTC	= 1
};

enum phytmmc_msg_cmd_id {
	PHYTMMC_MSG_CMD_DEFAULT = 0,
	PHYTMMC_MSG_CMD_SET,
	PHYTMMC_MSG_CMD_GET,
	PHYTMMC_MSG_CMD_DATA,
	PHYTMMC_MSG_CMD_REPORT
};

struct phyt_msg_info {
	u8 reserved;
	u8 seq;
	u8 cmd_type;
	u8 cmd_subid;
	u16 len;
	u8 status1;
	u8 status0;
#define NOT_READY		0x0
#define SUCCESS			0x1
#define GOING			0x2
#define GENERIC_ERROR		0x10
#define TYPE_NOT_SUPPORTED	0x11
#define CMD_NOT_SUPPORTED	0x12
#define INVALID_PARAMETERS	0x13
	u8 data[56];
};

struct phytium_mci_host {
	struct device *dev;
	struct mmc_host *mmc;
	u32 caps;
	u32 caps2;
	spinlock_t lock;
	struct mmc_request *mrq;
	struct mmc_command *cmd;
	struct mmc_data *data;
	int error;
	void __iomem *base;		    /* host base address */
	void __iomem *regf_base;		    /* regfile base address */
	void *adma_table1;
	dma_addr_t adma_addr1;
	struct phytium_mci_dma dma_rx;	/* dma channel */
	struct phytium_mci_dma dma_tx;	/* dma channel */
	struct phytium_mci_dma dma;	/* dma channel */
	u64 dma_mask;
	bool vqmmc_enabled;
	u32 *sg_virt_addr;
	enum adtc_t adtc_type;      /* 0:common adtc cmd; 1:block r/w adtc cmd;*/
	struct timer_list hotplug_timer;
	struct delayed_work req_timeout;
	int irq;		    /* host interrupt */
	u32 current_rca;    /*the current rca value*/
	u32 current_ios_clk;
	u32 is_use_dma;
	u32 is_device_x100;
	struct clk *src_clk;	    /* phytium_mci source clock */
	unsigned long clk_rate;
	unsigned long clk_div;
	unsigned long irq_flags;
	unsigned long flags;
#define MCI_CARD_NEED_INIT	1
	bool cmd_cs;	/*volt switch cmd cs status*/
	bool use_hold;	/*use hold*/
	bool clk_set;	/*clock set*/
	s32 clk_smpl_drv_25m;
	s32 clk_smpl_drv_50m;
	s32 clk_smpl_drv_66m;
	s32 clk_smpl_drv_100m;
	struct phyt_msg_info msg;
	struct phyt_msg_info rxmsg;
	bool debug_enable;
	bool alive_enable;
	struct timer_list alive_timer;
};

int phyt_mci_common_probe(struct phytium_mci_host *host);
void phyt_mci_deinit_hw(struct phytium_mci_host *host);
int phyt_mci_runtime_suspend(struct device *dev);
int phyt_mci_runtime_resume(struct device *dev);
int phyt_mci_resume(struct device *dev);
int phyt_mci_suspend(struct device *dev);
int phytium_mci_set_debug_enable(struct phytium_mci_host *host, bool enable);
int phytium_mci_set_alive_enable(struct phytium_mci_host *host, bool enable);

struct init_data_t {
	uint32_t caps;
	uint32_t clk_rate;
};

struct start_command_data_t {
	uint32_t cmd_arg;
	uint32_t cmd_flags;
	uint32_t cmd_opcode;
	uint32_t rawcmd;
};

struct start_data_data_t {
	uint32_t data_flags;
	uint32_t adtc_type;
	uint64_t adma_addr;
	uint32_t mrq_data_blksz;
	uint32_t mrq_data_blocks;
	uint32_t cmd_arg;
	uint32_t rawcmd;
};

struct set_ios_data_t {
	uint32_t ios_clock;
	uint8_t ios_timing;
	uint8_t ios_bus_width;
	uint8_t ios_power_mode;
};

struct cmd_next_data_t {
	uint32_t events;
	uint32_t response0;
	uint32_t response1;
	uint32_t response2;
	uint32_t response3;
};

struct err_irq_data_t {
	uint32_t raw_ints;
	uint32_t ints_mask;
	uint32_t dmac_status;
	uint32_t dmac_mask;
	uint32_t opcode;
};
#endif /* __PHYTIUM_MCI_HW_H */

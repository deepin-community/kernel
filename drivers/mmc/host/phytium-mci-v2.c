// SPDX-License-Identifier: GPL-2.0+
/*
 * Driver for Phytium Multimedia Card Interface
 *
 * Copyright (C) 2024 Phytium Technology Co., Ltd.
 */

#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/acpi.h>
#include <linux/timer.h>
#include <linux/swab.h>
#include <linux/pci.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/delay.h>
#include "phytium-mci-v2.h"

struct init_data_t *init_data;
struct start_command_data_t *start_command_data;
struct start_data_data_t *start_data_data;
struct set_ios_data_t *set_ios_data;
struct cmd_next_data_t *cmd_next_data;
struct err_irq_data_t *err_irq_data;

static struct phytium_mci_host *shost;

static const u32 rv2ap_int_mask = MMC_RXRING_TAIL_INT_MASK;

static const u32 cmd_err_ints_mask = MCI_INT_MASK_RTO | MCI_INT_MASK_RCRC | MCI_INT_MASK_RE |
				     MCI_INT_MASK_DCRC | MCI_INT_MASK_DRTO |
				     MCI_MASKED_INTS_SBE_BCI;

static int ap_phytium_mci_cmd_next(struct phyt_msg_info *rxmsg);
static int ap_phytium_mci_data_xfer_next(struct phyt_msg_info *rxmsg);
static void phytium_mci_adma_reset(struct phytium_mci_host *host);
static void phytium_mci_init(struct phytium_mci_host *host);
static void phytium_mci_init_adma_table(struct phytium_mci_host *host,
					 struct phytium_mci_dma *dma);
static void phytium_mci_init_hw(struct phytium_mci_host *host);
static int phytium_mci_get_cd(struct mmc_host *mmc);
static int phytium_mci_get_ro(struct mmc_host *mmc);
static void phytium_mci_start_command(struct phytium_mci_host *host,
		struct mmc_request *mrq,
		struct mmc_command *cmd);
static void
phytium_mci_start_data(struct phytium_mci_host *host, struct mmc_request *mrq,
		struct mmc_command *cmd, struct mmc_data *data);
static void __phytium_mci_enable_sdio_irq(struct phytium_mci_host *host, int enable);
static int phytium_check_msg(void);
static int phytium_mci_card_busy(struct mmc_host *mmc);
struct phyt_msg_info *shmem[RING_MAX];
struct phyt_msg_info *rvshmem[RING_MAX_RV];

static void phytium_mci_set_int_mask(struct phytium_mci_host *host)
{
	u32 int_mask;

	int_mask = ~rv2ap_int_mask;

	writel(int_mask, host->regf_base + MMC_RV2AP_INT_MASK);
	dev_info(host->dev, "set int mask %x\n", int_mask);
}

static void phytium_mci_prepare_data(struct phytium_mci_host *host,
				      struct mmc_request *mrq)
{
	struct mmc_data *data = mrq->data;

	if (!(data->host_cookie & MCI_PREPARE_FLAG)) {
		data->host_cookie |= MCI_PREPARE_FLAG;
		data->sg_count = dma_map_sg(host->dev, data->sg, data->sg_len,
					    mmc_get_dma_dir(data));
	}
}

static void phytium_mci_unprepare_data(struct phytium_mci_host *host,
					struct mmc_request *mrq)
{
	struct mmc_data *data = mrq->data;

	if (data->host_cookie & MCI_ASYNC_FLAG)
		return;

	if (data->host_cookie & MCI_PREPARE_FLAG) {
		dma_unmap_sg(host->dev, data->sg, data->sg_len, mmc_get_dma_dir(data));
		data->host_cookie &= ~MCI_PREPARE_FLAG;
	}
}

static inline u32
phytium_mci_cmd_find_resp(struct phytium_mci_host *host,
			  struct mmc_request *mrq,
			  struct mmc_command *cmd)
{
	u32 resp;

	switch (mmc_resp_type(cmd)) {
	case MMC_RSP_R1:
	case MMC_RSP_R1B:
		resp = 0x5;
		break;

	case MMC_RSP_R2:
		resp = 0x7;
		break;

	case MMC_RSP_R3:
		resp = 0x1;
		break;

	case MMC_RSP_NONE:
	default:
		resp = 0x0;
		break;
	}

	return resp;
}

static inline
u32 phytium_mci_cmd_prepare_raw_cmd(struct phytium_mci_host *host,
				     struct mmc_request *mrq,
				     struct mmc_command *cmd)
{
	u32 opcode = cmd->opcode;
	u32 resp = phytium_mci_cmd_find_resp(host, mrq, cmd);
	u32 rawcmd = ((opcode & 0x3f) | ((resp & 0x7) << 6));

	if (opcode == MMC_GO_INACTIVE_STATE ||
	    (opcode == SD_IO_RW_DIRECT && ((cmd->arg >> 9) & 0x1FFFF) == SDIO_CCCR_ABORT))
		rawcmd |= (0x1 << 14);
	else if (opcode == SD_SWITCH_VOLTAGE)
		rawcmd |= (0x1 << 28);

	if (test_and_clear_bit(MCI_CARD_NEED_INIT, &host->flags))
		rawcmd |= (0x1 << 15);

	if (cmd->data) {
		struct mmc_data *data = cmd->data;

		rawcmd |= (0x1 << 9);

		if (data->flags & MMC_DATA_WRITE)
			rawcmd |= (0x1 << 10);
	}

	if (host->use_hold)
		rawcmd |= (0x1 << 29);

	return (rawcmd | (0x1 << 31));
}

static inline void
phytium_mci_adma_write_desc(struct phytium_mci_host *host,
			     struct phytium_adma2_64_desc *desc,
			     dma_addr_t addr, u32 len, u32 attribute)
{
	desc->attribute = attribute;
	desc->len = len;
	desc->addr_lo = lower_32_bits(addr);
	desc->addr_hi = upper_32_bits(addr);
	dev_dbg(host->dev, "%s %d:addr_lo:0x%x ddr_hi:0x%x\n", __func__,
		__LINE__,  desc->addr_lo, desc->addr_hi);

	if ((attribute == 0x80000004) || (attribute == 0x8000000c)) {
		desc->desc_lo = 0;
		desc->desc_hi = 0;
	}
}

static void
phytium_mci_data_sg_write_2_admc_table(struct phytium_mci_host *host, struct mmc_data *data)
{
	struct phytium_adma2_64_desc *desc;
	u32 dma_len, i;
	dma_addr_t dma_address;
	struct scatterlist *sg;

	phytium_mci_init_adma_table(host, &host->dma);

	desc = host->dma.adma_table;
	for_each_sg(data->sg, sg, data->sg_count, i) {
		dma_address = sg_dma_address(sg);
		dma_len = sg_dma_len(sg);

		if (i == 0) {
			if (sg_is_last(sg) || (data->sg_count == 1 && dma_len == SD_BLOCK_SIZE))
				phytium_mci_adma_write_desc(host, desc, dma_address,
							     dma_len, 0x8000000c);
			else
				phytium_mci_adma_write_desc(host, desc, dma_address,
							     dma_len, 0x8000001a);
		} else if (sg_is_last(sg)) {
			phytium_mci_adma_write_desc(host, desc, dma_address,
						     dma_len, 0x80000004);
		} else {
			phytium_mci_adma_write_desc(host, desc, dma_address,
						     dma_len, 0x80000012);
		}

		desc++;
	}
}

static void phytium_mci_track_cmd_data(struct phytium_mci_host *host,
					struct mmc_command *cmd,
					struct mmc_data *data)
{
	if (host->error)
		dev_dbg(host->dev, "%s: cmd=%d arg=%08X; host->error=0x%08X\n",
			__func__, cmd->opcode, cmd->arg, host->error);
}

static void phytium_mci_request_done(struct phytium_mci_host *host, struct mmc_request *mrq)
{
	phytium_mci_track_cmd_data(host, mrq->cmd, mrq->data);

	if (mrq->data)
		phytium_mci_unprepare_data(host, mrq);

	mmc_request_done(host->mmc, mrq);
}

static void phytium_mci_ops_request(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct phytium_mci_host *host = mmc_priv(mmc);

	host->error = 0;
	host->mrq = NULL;
	WARN_ON(host->mrq);
	host->mrq = mrq;

	dev_dbg(host->dev, "%s %d: cmd:%d arg:0x%x\n", __func__, __LINE__,
		mrq->cmd->opcode, mrq->cmd->arg);

	if (mrq->sbc) {
		phytium_mci_start_command(host, mrq, mrq->sbc);
		return;
	}
	if (mrq->data) {
		phytium_mci_prepare_data(host, mrq);

		if ((mrq->data->sg->length >= 512) && host->is_use_dma &&
			((mrq->cmd->opcode == MMC_READ_MULTIPLE_BLOCK) ||
			(mrq->cmd->opcode == MMC_READ_SINGLE_BLOCK) ||
			(mrq->cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK) ||
			(mrq->cmd->opcode == MMC_WRITE_BLOCK) ||
			(mrq->cmd->opcode == SD_IO_RW_EXTENDED)))

			host->adtc_type = BLOCK_RW_ADTC;
		else
			host->adtc_type = COMMOM_ADTC;

		phytium_mci_start_data(host, mrq, mrq->cmd, mrq->data);
		return;
	}
	phytium_mci_start_command(host, mrq, mrq->cmd);
}

static void phytium_mci_pre_req(struct mmc_host *mmc, struct mmc_request *mrq)
{
	struct phytium_mci_host *host = mmc_priv(mmc);
	struct mmc_data *data = mrq->data;

	if (!data)
		return;

	phytium_mci_prepare_data(host, mrq);
	data->host_cookie |= MCI_ASYNC_FLAG;
}

static void phytium_mci_post_req(struct mmc_host *mmc, struct mmc_request *mrq,
				  int err)
{
	struct phytium_mci_host *host = mmc_priv(mmc);
	struct mmc_data *data = mrq->data;

	if (!data)
		return;

	if (data->host_cookie & MCI_ASYNC_FLAG) {
		data->host_cookie &= ~MCI_ASYNC_FLAG;
		phytium_mci_unprepare_data(host, mrq);
	}
}

static void phytium_mci_data_read_without_dma(struct phytium_mci_host *host,
					      struct mmc_data *data)
{
	u32 length, i, data_val, dma_len, tmp = 0;
	u32 *virt_addr;
	unsigned long flags;
	struct scatterlist *sg;

	length = data->blocks * data->blksz;

	if (mmc_get_dma_dir(data) == DMA_FROM_DEVICE) {
		spin_lock_irqsave(&host->lock, flags);
		if (data->host_cookie & MCI_ASYNC_FLAG) {
			tmp = MCI_ASYNC_FLAG;
			phytium_mci_post_req(host->mmc, data->mrq, 0);
		} else {
			phytium_mci_unprepare_data(host, data->mrq);
		}

		for_each_sg(data->sg, sg, data->sg_count, i) {
			dma_len = sg_dma_len(sg);
			virt_addr = sg_virt(data->sg);

			for (i = 0; i < (dma_len / 4); i++) {
				data_val = readl(host->regf_base + MCI_DATA);
				memcpy(virt_addr, &data_val, 4);
				++virt_addr;
			}
		}

		if (tmp & MCI_ASYNC_FLAG)
			phytium_mci_pre_req(host->mmc, data->mrq);
		else
			phytium_mci_prepare_data(host, data->mrq);

		spin_unlock_irqrestore(&host->lock, flags);
	}
	data->bytes_xfered = length;
}

static irqreturn_t phytium_mci_irq(int irq, void *dev_id)
{
	struct phytium_mci_host *host = (struct phytium_mci_host *) dev_id;
	u32 rv2ap_event;

	rv2ap_event = readl(host->regf_base + MMC_RV2AP_INT);
	if (rv2ap_event & MMC_TXRING_HEAD_INT) {
		rv2ap_event &= ~MMC_TXRING_HEAD_INT;
		pr_debug("MCIAP %s MMC_TXRING_HEAD_INT %x\n", __func__, rv2ap_event);
		writel(rv2ap_event, host->regf_base + MMC_RV2AP_INT);
	}
	if (rv2ap_event & MMC_RXRING_TAIL_INT) {
		rv2ap_event &= ~MMC_RXRING_TAIL_INT;
		pr_debug("MCIAP %s MMC_RXRING_TAIL_INT %x\n", __func__, rv2ap_event);
		writel(rv2ap_event, host->regf_base + MMC_RV2AP_INT);
		phytium_check_msg();
	}
	return IRQ_HANDLED;
}

static void phytium_mci_init_adma_table(struct phytium_mci_host *host,
					 struct phytium_mci_dma *dma)
{
	struct phytium_adma2_64_desc *adma_table = dma->adma_table;
	dma_addr_t dma_addr;
	int i;

	memset(adma_table, 0, sizeof(struct phytium_adma2_64_desc) * MAX_BD_NUM);

	for (i = 0; i < (MAX_BD_NUM - 1); i++) {
		dma_addr = dma->adma_addr + sizeof(*adma_table) * (i + 1);
		adma_table[i].desc_lo = lower_32_bits(dma_addr);
		adma_table[i].desc_hi = upper_32_bits(dma_addr);
		adma_table[i].attribute = 0;
		adma_table[i].NON1 = 0;
		adma_table[i].len = 0;
		adma_table[i].NON2 = 0;
	}

	phytium_mci_adma_reset(host);
}

static void phytium_mci_ack_sdio_irq(struct mmc_host *mmc)
{
	unsigned long flags;
	struct phytium_mci_host *host = mmc_priv(mmc);

	spin_lock_irqsave(&host->lock, flags);
	__phytium_mci_enable_sdio_irq(host, 1);
	spin_unlock_irqrestore(&host->lock, flags);
}

#ifdef CONFIG_PM_SLEEP
int phyt_mci_suspend(struct device *dev)
{
	struct mmc_host *mmc = dev_get_drvdata(dev);
	struct phytium_mci_host *host = mmc_priv(mmc);

	phyt_mci_deinit_hw(host);
	return 0;
}
EXPORT_SYMBOL(phyt_mci_suspend);

int phyt_mci_resume(struct device *dev)
{
	struct mmc_host *mmc = dev_get_drvdata(dev);
	struct phytium_mci_host *host = mmc_priv(mmc);

	phytium_mci_set_int_mask(host);
	phytium_mci_init(host);
	phytium_mci_init_hw(host);
	return 0;
}
EXPORT_SYMBOL(phyt_mci_resume);

#endif

#ifdef CONFIG_PM
int phyt_mci_runtime_suspend(struct device *dev)
{
	struct mmc_host *mmc = dev_get_drvdata(dev);
	struct phytium_mci_host *host = mmc_priv(mmc);

	phyt_mci_deinit_hw(host);
	return 0;
}
EXPORT_SYMBOL(phyt_mci_runtime_suspend);

int phyt_mci_runtime_resume(struct device *dev)
{
	struct mmc_host *mmc = dev_get_drvdata(dev);
	struct phytium_mci_host *host = mmc_priv(mmc);

	phytium_mci_set_int_mask(host);
	phytium_mci_init(host);
	phytium_mci_init_hw(host);
	return 0;
}
EXPORT_SYMBOL(phyt_mci_runtime_resume);

#endif

static int phytium_ap_to_rv(struct phyt_msg_info *msg)
{
	int i;
	int ret = 0;
	int tx_t, tx_h, p;

	tx_t = readl(shost->regf_base + MMC_TX_TAIL) & 0xffff;
	tx_h = readl(shost->regf_base + MMC_TX_HEAD) & 0xffff;

	if ((tx_t + 1) % RING_MAX == tx_h) {
		pr_err("mci ring buff full\n");
		return -1;
	}

	p = tx_t;

	tx_t = (tx_t + 1) % RING_MAX;

	shmem[p] = (struct phyt_msg_info *)(*shmem + p);
	pr_debug("MCIAP shmem[%d] %x\n", p, (unsigned int)(long)shmem[p]);
	pr_debug("MCIAP MSG CMD:%d SCMD:%d\n", msg->cmd_type, msg->cmd_subid);

	/*write msg to shmem*/
	memcpy(shmem[p], msg, sizeof(struct phyt_msg_info));

	/*update tx tail pointer*/
	writel(tx_t | MMC_TX_TAIL_INT, shost->regf_base + MMC_TX_TAIL);

	/*wait and check status*/
	for (i = 0; i < COMPLETE_TIMEOUT; i++) {
		dsb(sy);
		if (shmem[p]->status0 == GOING || shmem[p]->status0 == NOT_READY) {
			pr_debug("MCIAP [%d] not complete %x\r\n", p, shmem[p]->status0);
		} else if (shmem[p]->status0 == SUCCESS) {
			ret = shmem[p]->status1;
			pr_debug("MCIAP [%d] complete and return %d\r\n", p, ret);
			/*clear status*/
			shmem[p]->status0 = 0;
			shmem[p]->status1 = 0;
			break;
		} else if (shmem[p]->status0 >= GENERIC_ERROR) {
			pr_debug("MCIAP [%d] status error %x\r\n", p, shmem[p]->status0);
			ret = -1;
			/*clear status*/
			shmem[p]->status0 = 0;
			shmem[p]->status1 = 0;
			break;
		}
		udelay(40);
	}
	pr_debug("MCIAP %s end [%d] times:%d\n", __func__, p, i);
	return ret;
}

static void phytium_mci_init(struct phytium_mci_host *host)
{
	host->msg.cmd_type = PHYTMMC_MSG_CMD_SET;
	host->msg.cmd_subid = MCI_INIT;

	pr_debug("MCIAP host->mmc->caps %x host->clk_rate %ld\n",
		host->mmc->caps, host->clk_rate);

	init_data = (struct init_data_t *)host->msg.data;
	init_data->caps = host->mmc->caps;
	init_data->clk_rate = host->clk_rate;

	phytium_ap_to_rv(&host->msg);
}

static void phytium_mci_init_hw(struct phytium_mci_host *host)
{
	host->msg.cmd_type = PHYTMMC_MSG_CMD_SET;
	host->msg.cmd_subid = MCI_INIT_HW;

	phytium_ap_to_rv(&host->msg);
	dev_info(host->dev, "init hardware done!");
}

void phyt_mci_deinit_hw(struct phytium_mci_host *host)
{
	host->msg.cmd_type = PHYTMMC_MSG_CMD_SET;
	host->msg.cmd_subid = MCI_DEINIT_HW;

	phytium_ap_to_rv(&host->msg);
}
EXPORT_SYMBOL_GPL(phyt_mci_deinit_hw);

static void phytium_mci_start_command(struct phytium_mci_host *host,
		struct mmc_request *mrq,
		struct mmc_command *cmd)
{
	u32 rawcmd;

	host->cmd = cmd;
	host->mrq = mrq;
	rawcmd = phytium_mci_cmd_prepare_raw_cmd(host, mrq, cmd);

	pr_debug("MCIAP arg%x flags%x opcode%d rawcmd%x\n",
		host->cmd->arg, host->cmd->flags, host->cmd->opcode, rawcmd);
	host->msg.cmd_type = PHYTMMC_MSG_CMD_SET;
	host->msg.cmd_subid = MCI_START_CMD;

	start_command_data = (struct start_command_data_t *)host->msg.data;
	start_command_data->cmd_arg = cmd->arg;
	start_command_data->cmd_flags = cmd->flags;
	start_command_data->cmd_opcode = cmd->opcode;
	start_command_data->rawcmd = rawcmd;

	phytium_ap_to_rv(&host->msg);
}

static void
phytium_mci_start_data(struct phytium_mci_host *host, struct mmc_request *mrq,
		struct mmc_command *cmd, struct mmc_data *data)
{
	u32 rawcmd;

	host->cmd = cmd;
	cmd->error = 0;
	host->mrq = mrq;
	host->data = data;
	rawcmd = phytium_mci_cmd_prepare_raw_cmd(host, mrq, cmd);
	phytium_mci_data_sg_write_2_admc_table(host, data);

	host->msg.cmd_type = PHYTMMC_MSG_CMD_SET;
	host->msg.cmd_subid = MCI_START_DATA;

	start_data_data = (struct start_data_data_t *)host->msg.data;
	start_data_data->data_flags = data->flags;
	start_data_data->adtc_type = host->adtc_type;
	start_data_data->adma_addr = host->dma.adma_addr;
	start_data_data->mrq_data_blksz = mrq->data->blksz;
	start_data_data->mrq_data_blocks = mrq->data->blocks;
	start_data_data->cmd_arg = cmd->arg;
	start_data_data->rawcmd = rawcmd;

	phytium_ap_to_rv(&host->msg);
}

static void phytium_mci_ops_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct phytium_mci_host *host = mmc_priv(mmc);

	host->msg.cmd_type = PHYTMMC_MSG_CMD_SET;
	host->msg.cmd_subid = MCI_OPS_SET_IOS;

	set_ios_data = (struct set_ios_data_t *)host->msg.data;
	set_ios_data->ios_clock = ios->clock;
	set_ios_data->ios_timing = ios->timing;
	set_ios_data->ios_bus_width = ios->bus_width;
	set_ios_data->ios_power_mode = ios->power_mode;

	phytium_ap_to_rv(&host->msg);

	if (ios->power_mode == MMC_POWER_UP)
		set_bit(MCI_CARD_NEED_INIT, &host->flags);
}

static void __phytium_mci_enable_sdio_irq(struct phytium_mci_host *host, int enable)
{
	host->msg.cmd_type = PHYTMMC_MSG_CMD_SET;
	host->msg.cmd_subid = MCI_SDIO_IRQ_EN;

	host->msg.data[0] = enable & 0xFF;

	phytium_ap_to_rv(&host->msg);
}

static void phytium_mci_enable_sdio_irq(struct mmc_host *mmc, int enable)
{
	struct phytium_mci_host *host  = mmc_priv(mmc);

	__phytium_mci_enable_sdio_irq(host, enable);
}

static int phytium_mci_ops_switch_volt(struct mmc_host *mmc, struct mmc_ios *ios)
{
	struct phytium_mci_host *host = mmc_priv(mmc);
	int ret = 0;

	host->msg.cmd_type = PHYTMMC_MSG_CMD_SET;
	host->msg.cmd_subid = MCI_OPS_SWITCH_VOLT;

	host->msg.data[0] = ios->signal_voltage & 0xFF;

	ret = phytium_ap_to_rv(&host->msg);

	pr_debug("MCIAP %s %d\n", __func__, ret);

	return ret;
}

static void phytium_mci_hw_reset(struct mmc_host *mmc)
{
	struct phytium_mci_host *host = mmc_priv(mmc);

	host->msg.cmd_type = PHYTMMC_MSG_CMD_DEFAULT;
	host->msg.cmd_subid = MCI_HW_RESET;

	phytium_ap_to_rv(&host->msg);
}

static void phytium_mci_adma_reset(struct phytium_mci_host *host)
{
	host->msg.cmd_type = PHYTMMC_MSG_CMD_DEFAULT;
	host->msg.cmd_subid = MCI_ADMA_RESET;

	phytium_ap_to_rv(&host->msg);
}

static int phytium_mci_get_cd(struct mmc_host *mmc)
{
	struct phytium_mci_host *host = mmc_priv(mmc);
	u32 status;

	if (mmc->caps & MMC_CAP_NONREMOVABLE)
		return 1;

	status = readl(host->regf_base + MCI_CARD_DETECT);

	pr_debug("MCIAP get cd %d\n", status);

	if ((status & 0x1) == 0x1)
		return 0;

	return 1;
}

static int phytium_mci_card_busy(struct mmc_host *mmc)
{
	struct phytium_mci_host *host = mmc_priv(mmc);
	int ret;

	host->msg.cmd_type = PHYTMMC_MSG_CMD_GET;
	host->msg.cmd_subid = MCI_GET_CARD_BUSY;

	ret = phytium_ap_to_rv(&host->msg);

	pr_debug("MCIAP card busy %d\n", ret);
	return ret;
}

static int phytium_mci_get_ro(struct mmc_host *mmc)
{
	struct phytium_mci_host *host = mmc_priv(mmc);
	int ret;

	host->msg.cmd_type = PHYTMMC_MSG_CMD_GET;
	host->msg.cmd_subid = MCI_GET_RO;

	ret = phytium_ap_to_rv(&host->msg);

	dev_dbg(host->dev, "MCIAP ro %d\n", ret);

	return ret;
}

static int ap_phytium_mci_cmd_next(struct phyt_msg_info *rxmsg)
{
	struct phytium_mci_host *host;
	u32 events;

	host = shost;

	if (!host->cmd)
		return 1;

	cmd_next_data = (struct cmd_next_data_t *)rxmsg->data;
	events = cmd_next_data->events;
	host->cmd->resp[0] = cmd_next_data->response0;
	host->cmd->resp[1] = cmd_next_data->response1;
	host->cmd->resp[2] = cmd_next_data->response2;
	host->cmd->resp[3] = cmd_next_data->response3;

	if (!(events & (MCI_RAW_INTS_CMD | MCI_INT_MASK_HTO))) {
		if (!(host->mmc->caps & MMC_CAP_NONREMOVABLE) && (events & MCI_RAW_INTS_RTO)
		   && readl(host->regf_base + MCI_CARD_DETECT)) {
			host->cmd->error = -ENOMEDIUM;
			host->cmd->resp[0] = 0;
		} else if (events & MCI_RAW_INTS_RTO ||
		    (host->cmd->opcode != MMC_SEND_TUNING_BLOCK &&
		     host->cmd->opcode != MMC_SEND_TUNING_BLOCK_HS200)) {
			host->cmd->error = -ETIMEDOUT;
		} else if (events & MCI_RAW_INTS_RCRC) {
			host->cmd->error = -EILSEQ;
		} else {
			host->cmd->error = -ETIMEDOUT;
		}
	}

	if ((host->cmd->error && !(host->cmd->opcode == MMC_SEND_TUNING_BLOCK ||
		host->cmd->opcode == MMC_SEND_TUNING_BLOCK_HS200)) ||
		(host->mrq->sbc && host->mrq->sbc->error)) {
		phytium_mci_request_done(host, host->mrq);
	} else if (host->cmd == host->mrq->sbc) {
		if ((host->mrq->cmd->opcode == MMC_READ_MULTIPLE_BLOCK) ||
		    (host->mrq->cmd->opcode == MMC_WRITE_MULTIPLE_BLOCK) ||
		    (host->mrq->cmd->opcode == MMC_READ_SINGLE_BLOCK) ||
		    (host->mrq->cmd->opcode == MMC_WRITE_BLOCK)) {
			dev_dbg(host->dev, "%s %d:sbc done and next cmd :%d length:%d\n",
				__func__, __LINE__, host->mrq->cmd->opcode,
				host->mrq->data->sg->length);
			phytium_mci_prepare_data(host, host->mrq);
			if (host->is_use_dma)
				host->adtc_type = BLOCK_RW_ADTC;
			else
				host->adtc_type = COMMOM_ADTC;
			phytium_mci_start_data(host, host->mrq, host->mrq->cmd, host->mrq->data);
		} else {
			dev_err(host->dev, "%s %d:ERROR: cmd %d followers the SBC\n",
				__func__, __LINE__, host->cmd->opcode);
		}
	} else if (!host->cmd->data) {
		phytium_mci_request_done(host, host->mrq);
	}
	return 1;
}

static int ap_phytium_mci_data_xfer_next(struct phyt_msg_info *rxmsg)
{
	struct phytium_mci_host *host;
	u32 events;
	struct mmc_request *mrq;
	struct mmc_data *data;

	host = shost;
	mrq = host->mrq;
	data = host->data;

	events = ((uint32_t *)(rxmsg->data))[0];

	if (events & MCI_RAW_INTS_DTO) {
		if (host->adtc_type == COMMOM_ADTC &&
				(mrq->cmd->flags & MMC_CMD_MASK) == MMC_CMD_ADTC)
			phytium_mci_data_read_without_dma(host, data);
		else
			data->bytes_xfered = data->blocks * data->blksz;
	} else {
		data->bytes_xfered = 0;
		if (!(host->mmc->caps & MMC_CAP_NONREMOVABLE)
				&& readl(host->regf_base + MCI_CARD_DETECT)
				&& (events & cmd_err_ints_mask)) {
			data->error = -ENOMEDIUM;
			data->mrq->cmd->error =  -ENOMEDIUM;
		} else if (events & (MCI_RAW_INTS_DCRC | MCI_RAW_INTS_EBE |
					MCI_RAW_INTS_SBE_BCI)) {
			data->error = -EILSEQ;
		} else {
			data->error = -ETIMEDOUT;
		}
	}

	if (mmc_op_multi(mrq->cmd->opcode) && mrq->stop &&
	    (data->error || !mrq->sbc)) {
		phytium_mci_start_command(host, mrq, mrq->stop);
	} else {
		phytium_mci_request_done(host, mrq);
	}
	return 1;
}

static int ap_phytium_mci_card_detect_irq(struct phyt_msg_info *rxmsg)
{
	u8 cd;
	struct phytium_mci_host *host;

	host = shost;
	cd = rxmsg->data[0];

	pr_debug("MCIAP card_detect_irq %d\n", cd);
	if (cd) {
		if (host->mmc->card) {
			cancel_delayed_work(&host->mmc->detect);
			mmc_detect_change(host->mmc, msecs_to_jiffies(0));
		}
	} else {
		cancel_delayed_work(&host->mmc->detect);
		mmc_detect_change(host->mmc, msecs_to_jiffies(200));
	}
	return 1;
}

static int ap_phytium_mci_err_irq(struct phyt_msg_info *rxmsg)
{
	u32 raw_ints;
	u32 dmac_status;
	u32 ints_mask;
	u32 dmac_mask;
	u32 opcode;

	err_irq_data = (struct err_irq_data_t *)rxmsg->data;
	raw_ints = err_irq_data->raw_ints;
	ints_mask = err_irq_data->ints_mask;
	dmac_status = err_irq_data->dmac_status;
	dmac_mask = err_irq_data->dmac_mask;
	opcode = err_irq_data->opcode;

	pr_err("MCIAP ERR raw_ints:%x ints_mask:%x dmac_status:%x dmac_mask:%x cmd:%d\n",
		raw_ints, ints_mask, dmac_status, dmac_mask, opcode);

	return 1;
}

static int phytium_check_msg(void)
{
	int rx_t, rx_h;
	int complete;
	struct phyt_msg_info rxmsg;
	int p = 0;

	rx_t = readl(shost->regf_base + MMC_RX_TAIL) & 0xffff;
	rx_h = readl(shost->regf_base + MMC_RX_HEAD) & 0xffff;

	while (rx_t != rx_h) {
		pr_debug("MCIAP %s rx_t:%d rx_h:%d\n", __func__, rx_t, rx_h);

		p = rx_h;
		rx_h = (rx_h + 1) % RING_MAX_RV;

		writel(rx_h, shost->regf_base + MMC_RX_HEAD);
		rvshmem[p] = (struct phyt_msg_info *)(*rvshmem + p);
		pr_debug("MCIAP %s %d %x\n", __func__, p, (unsigned int)(long)rvshmem[p]);

		/*read msg from shmem*/
		memcpy(&rxmsg, rvshmem[p], sizeof(struct phyt_msg_info));
		/*read msg from shmem*/

		/*execute cmd*/
		switch (rxmsg.cmd_type) {
		case PHYTMMC_MSG_CMD_REPORT:
			switch (rxmsg.cmd_subid) {
			case MCI_CMD_NEXT:
				complete = ap_phytium_mci_cmd_next(&rxmsg);
				break;
			case MCI_DATA_NEXT:
				complete = ap_phytium_mci_data_xfer_next(&rxmsg);
				break;
			case MCI_CD_IRQ:
				complete = ap_phytium_mci_card_detect_irq(&rxmsg);
				break;
			case MCI_ERR_IRQ:
				complete = ap_phytium_mci_err_irq(&rxmsg);
				break;
			default:
				pr_debug("MCIAP invalid sub cmd\r\n");
				break;
			}
			break;
		default:
			pr_debug("MCIAP invalid cmd\r\n");
			break;
		}

		/*write complete*/
		rvshmem[p]->status0 = complete;
	}
	return 1;
}

int phytium_mci_set_debug_enable(struct phytium_mci_host *host, bool enable)
{
	static bool debug_enable;
	u32 debug_reg;

	if (enable == debug_enable)
		return 0;

	debug_enable = enable;

	debug_reg = readl(host->regf_base + MCI_DEBUG);
	pr_debug("MCIAP %s debug_reg %x\n", __func__, debug_reg);

	if (!debug_enable && (debug_reg & MCI_DEBUG_ENABLE)) {
		debug_reg &= ~MCI_DEBUG_ENABLE;
		writel(debug_reg, host->regf_base + MCI_DEBUG);
	} else if (debug_enable && !(debug_reg & MCI_DEBUG_ENABLE)) {
		debug_reg |= MCI_DEBUG_ENABLE;
		writel(debug_reg, host->regf_base + MCI_DEBUG);
	}

	return 1;
}
EXPORT_SYMBOL(phytium_mci_set_debug_enable);

int phytium_mci_set_alive_enable(struct phytium_mci_host *host, bool enable)
{
	static bool alive_enable;
	u32 debug_reg;

	if (enable == alive_enable)
		return 0;

	alive_enable = enable;

	debug_reg = readl(host->regf_base + MCI_DEBUG);
	pr_debug("MCIAP %s debug_reg %x\n", __func__, debug_reg);

	if (!alive_enable && (debug_reg & MCI_ALIVE_ENABLE)) {
		debug_reg &= ~MCI_ALIVE_ENABLE;
		writel(debug_reg, host->regf_base + MCI_DEBUG);
		del_timer(&host->alive_timer);
	} else if (alive_enable && !(debug_reg & MCI_ALIVE_ENABLE)) {
		debug_reg |= MCI_ALIVE_ENABLE | MCI_ALIVE;
		writel(debug_reg, host->regf_base + MCI_DEBUG);
		add_timer(&host->alive_timer);
	}

	return 1;
}
EXPORT_SYMBOL(phytium_mci_set_alive_enable);

static void alive_timer_func(struct timer_list *t)
{
	struct phytium_mci_host *host;
	u32 debug_reg;

	host = from_timer(host, t, alive_timer);
	if (!host)
		return;

	debug_reg = readl(host->regf_base + MCI_DEBUG);
	pr_debug("MCIAP %s debug_reg %x\n", __func__, debug_reg);
	debug_reg |= MCI_ALIVE;
	writel(debug_reg, host->regf_base + MCI_DEBUG);
	mod_timer(&host->alive_timer, jiffies + msecs_to_jiffies(5000));
}

static struct mmc_host_ops phytium_mci_ops = {
	.post_req = phytium_mci_post_req,
	.pre_req = phytium_mci_pre_req,
	.request = phytium_mci_ops_request,
	.set_ios = phytium_mci_ops_set_ios,
	.get_cd = phytium_mci_get_cd,
	.get_ro = phytium_mci_get_ro,
	.enable_sdio_irq = phytium_mci_enable_sdio_irq,
	.ack_sdio_irq = phytium_mci_ack_sdio_irq,
	.card_busy = phytium_mci_card_busy,
	.start_signal_voltage_switch = phytium_mci_ops_switch_volt,
	.card_hw_reset = phytium_mci_hw_reset,
};

int phyt_mci_common_probe(struct phytium_mci_host *host)
{
	struct mmc_host *mmc = host->mmc;
	struct device *dev = host->dev;
	int ret;

	shost = host;

	dev_info(host->dev, "phytium_mci_common_probe start\n");

	*shmem = host->base;
	*rvshmem = (struct phyt_msg_info *)(*shmem + RING_MAX);

	dma_set_mask(dev, DMA_BIT_MASK(64));
	dma_set_coherent_mask(dev, DMA_BIT_MASK(64));

	host->alive_timer.expires = jiffies + msecs_to_jiffies(5000);
	timer_setup(&host->alive_timer, alive_timer_func, 0);

	mmc->f_min = MCI_F_MIN;
	if (!mmc->f_max)
		mmc->f_max = MCI_F_MAX;

	mmc->ops = &phytium_mci_ops;
	mmc->ocr_avail_sdio = MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->ocr_avail_sd = MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->ocr_avail_mmc = MMC_VDD_165_195;
	mmc->ocr_avail = MMC_VDD_165_195 | MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->caps |= host->caps;

	if (mmc->caps & MMC_CAP_SDIO_IRQ) {
		mmc->caps2 |= MMC_CAP2_SDIO_IRQ_NOTHREAD;
		dev_dbg(host->dev, "%s %d: MMC_CAP_SDIO_IRQ\n", __func__, __LINE__);
	}
	mmc->caps2 |= host->caps2;

	phytium_mci_set_int_mask(host);

	phytium_mci_init(host);

	phytium_mci_init_hw(host);

	if (host->is_use_dma) {
		/* MMC core transfer sizes tunable parameters */
		mmc->max_segs = MAX_BD_NUM;
		mmc->max_seg_size = 4 * 1024;
		mmc->max_blk_size = 512;
		mmc->max_req_size = 512 * 1024;
		mmc->max_blk_count = mmc->max_req_size / 512;
		host->dma.adma_table = dma_alloc_coherent(host->dev,
							   MAX_BD_NUM *
							   sizeof(struct phytium_adma2_64_desc),
							   &host->dma.adma_addr, GFP_KERNEL);
		if (!host->dma.adma_table)
			return MCI_REALEASE_MEM;

		host->dma.desc_sz = ADMA2_64_DESC_SZ;
		phytium_mci_init_adma_table(host, &host->dma);
	} else {
		mmc->max_segs = MAX_BD_NUM;
		mmc->max_seg_size = 4 * 1024;
		mmc->max_blk_size = 512;
		mmc->max_req_size = 4 * 512;
		mmc->max_blk_count = mmc->max_req_size / 512;
	}

	spin_lock_init(&host->lock);

	ret = devm_request_irq(host->dev, host->irq, phytium_mci_irq,
			       host->irq_flags, "phytium-mci", host);
	if (ret)
		return ret;

	ret = mmc_add_host(mmc);
	if (ret) {
		dev_err(host->dev, "%s %d: mmc add host!\n", __func__, __LINE__);
		return ret;
	}
	return 0;
}
EXPORT_SYMBOL(phyt_mci_common_probe);

MODULE_DESCRIPTION("Phytium Multimedia Card Interface driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lai Xueyu <laixueyu1280@phytium.com.cn>");

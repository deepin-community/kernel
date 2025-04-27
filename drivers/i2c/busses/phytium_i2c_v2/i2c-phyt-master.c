// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium I2C adapter driver.
 *
 * Copyright (C) 2021-2023, Phytium Technology Co., Ltd.
 */
#include <linux/acpi.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/reset.h>
#include <linux/spinlock.h>
#include <linux/kernel.h>
#include <linux/time64.h>
#include "i2c-phyt-core.h"

#define FT_DEFAULT_TIMEOUT		(FT_DEFAULT_CLOCK_FREQUENCY / 1000 * 35)

#define FT_I2C_MSG_MNG_SIZE		8
#define FT_I2C_SHMEM_STORE_OFFSET	(FT_I2C_MSG_MNG_SIZE + sizeof(struct i2c_ft_trans_msg_info))

static int i2c_phyt_master_xfer(struct i2c_adapter *adapter,
					struct i2c_msg msgs[], int num);
static void i2c_phyt_master_parse_data(struct i2c_phyt_dev *dev,
						   struct phyt_msg_info *shmem_msg,
						   int index);
static int i2c_phyt_master_default_init(struct i2c_phyt_dev *dev);
static int i2c_phyt_master_set_timings_master(struct i2c_phyt_dev *dev);

static const struct i2c_algorithm i2c_phyt_master_algo = {
	.master_xfer = i2c_phyt_master_xfer,
	.functionality = i2c_phyt_func,
};


static int i2c_phyt_master_set_timings_master(struct i2c_phyt_dev *dev)
{
	const char *mode_str, *fp_str = "";
	u32 sda_falling_time, scl_falling_time;
	struct i2c_timings *t = &dev->timings;
	u32 ic_clk;
	int ret = 0;

	/* Set standard and fast speed dividers for high/low periods */
	sda_falling_time = t->sda_fall_ns ?: 300; /* ns */
	scl_falling_time = t->scl_fall_ns ?: 300; /* ns */

	/* Calculate SCL timing parameters for standard mode if not set */
	if (!dev->ss_hcnt || !dev->ss_lcnt) {
		ic_clk = i2c_phyt_clk_rate(dev);
		dev->ss_hcnt =
			i2c_phyt_scl_hcnt(ic_clk, 4000,/* tHD;STA = tHIGH = 4.0 us */
						sda_falling_time, 0,/* 0: DW default, 1: Ideal */
						0);                  /* No offset */
		dev->ss_lcnt = i2c_phyt_scl_lcnt(ic_clk, 4700, /* tLOW = 4.7 us */
							scl_falling_time, 0); /* No offset */
	}
	dev_dbg(dev->dev, "Standard Mode HCNT:LCNT = %d:%d\n", dev->ss_hcnt, dev->ss_lcnt);
	/*
	 * Set SCL timing parameters for fast mode or fast mode plus. Only
	 * difference is the timing parameter values since the registers are
	 * the same.
	 */
	if (t->bus_freq_hz == 1000000) {
		/*
		 * Check are fast mode plus parameters available and use
		 * fast mode if not.
		 */
		if (dev->fp_hcnt && dev->fp_lcnt) {
			dev->fs_hcnt = dev->fp_hcnt;
			dev->fs_lcnt = dev->fp_lcnt;
			fp_str = " Plus";
		}
	}
	/*
	 * Calculate SCL timing parameters for fast mode if not set. They are
	 * needed also in high speed mode.
	 */
	if (!dev->fs_hcnt || !dev->fs_lcnt) {
		ic_clk = i2c_phyt_clk_rate(dev);
		dev->fs_hcnt =
			i2c_phyt_scl_hcnt(ic_clk, 600,         /* tHD;STA = tHIGH = 0.6 us */
				sda_falling_time, 0, /* 0: DW default, 1: Ideal */
				0);                  /* No offset */
		dev->fs_lcnt = i2c_phyt_scl_lcnt(ic_clk, 1300, /* tLOW = 1.3 us */
			scl_falling_time, 0); /* No offset */
	}
	dev_dbg(dev->dev, "Fast Mode%s HCNT:LCNT = %d:%d\n", fp_str, dev->fs_hcnt, dev->fs_lcnt);

	if (dev->hs_hcnt && dev->hs_lcnt)
		dev_dbg(dev->dev, "High Speed Mode HCNT:LCNT = %d:%d\n", dev->hs_hcnt,
									dev->hs_lcnt);

	switch (dev->master_cfg & FT_IC_CON_SPEED_MASK) {
	case FT_IC_CON_SPEED_STD:
		mode_str = "Standard Mode";
		break;
	case FT_IC_CON_SPEED_HIGH:
		mode_str = "High Speed Mode";
		break;
	default:
		mode_str = "Fast Mode";
	}
	dev_dbg(dev->dev, "Bus speed: %s%s\n", mode_str, fp_str);

	return ret;
}

int i2c_phyt_master_smbus_alert_process(struct i2c_phyt_dev *dev)
{
	if (dev->ara)
		i2c_handle_smbus_alert(dev->ara);
	else
		dev_dbg(dev->dev, "alert do nothing\n");

	return 0;
}

static int i2c_phyt_master_update_new_msg(struct i2c_phyt_dev *dev)
{
	struct phyt_msg_info shmem_msg;
	struct phyt_msg_info *tx_shmem = (struct phyt_msg_info *)dev->tx_shmem_addr;
	struct i2c_ft_trans_msg_info *i2c_msg_info;
	struct i2c_msg *msgs = dev->msgs;
	int remain_len;

	/*Check the current index and the len are valid */
	if ((dev->mng.cur_index >= dev->msgs_num) ||
	    (msgs[dev->mng.cur_index].len < dev->mng.opt_finish_len))
		return -EINVAL;

	memset(&shmem_msg, 0, sizeof(struct phyt_msg_info));
	/*Get the remain len*/
	remain_len = msgs[dev->mng.cur_index].len - dev->mng.opt_finish_len;
	if (!remain_len) {
		/*Use the next index*/
		if (dev->mng.cur_index + 1 >= dev->msgs_num)
			return FT_I2C_RUNNING;

		dev->mng.cur_index++;

		if ((msgs[dev->mng.cur_index - 1].flags & I2C_M_RD) !=
		    (msgs[dev->mng.cur_index].flags & I2C_M_RD))
			shmem_msg.head.seq = FT_I2C_TRANS_FRAME_RESTART;

		dev->mng.opt_finish_len = 0;
		remain_len = msgs[dev->mng.cur_index].len;
	}

	/*Set the Head.len and Head.seq*/
	shmem_msg.head.len = remain_len;
	if (remain_len > FT_I2C_SINGLE_BUF_LEN) {
		shmem_msg.head.len = FT_I2C_SINGLE_BUF_LEN;
	} else {
		if (dev->mng.cur_index + 1 >= dev->msgs_num) {
			shmem_msg.head.seq |= FT_I2C_TRANS_FRAME_END;
			dev->mng.is_last_frame = true;
		}
	}
	/*Set other info of the Head*/
	shmem_msg.head.cmd_type = PHYTI2C_MSG_CMD_DATA;
	shmem_msg.head.cmd_subid = PHYTI2C_MSG_CMD_DATA_XFER;
	shmem_msg.head.status0 = FT_I2C_MSG_COMPLETE_UNKNOW;

	/*store addr flags of struct i2c_msg*/
	i2c_msg_info = (struct i2c_ft_trans_msg_info *)&shmem_msg.data[0];
	i2c_msg_info->addr = msgs[dev->mng.cur_index].addr;
	i2c_msg_info->flags = msgs[dev->mng.cur_index].flags;

	dev->total_shmem_len = FT_I2C_SHMEM_STORE_OFFSET;
	/*store data*/
	if (!(msgs[dev->mng.cur_index].flags & I2C_M_RD)) {
		memcpy(&shmem_msg.data[sizeof(struct i2c_ft_trans_msg_info)],
		       &msgs[dev->mng.cur_index].buf[dev->mng.opt_finish_len],
		       shmem_msg.head.len);
		dev->total_shmem_len += shmem_msg.head.len;
	}

	dev->mng.opt_finish_len += shmem_msg.head.len;
	/*update new data to shmem*/
	memcpy(&tx_shmem[dev->mng.tx_cmd_cnt % FT_I2C_SINGLE_FRAME_CNT],
	       &shmem_msg, dev->total_shmem_len);

	dev->real_index[dev->mng.tx_cmd_cnt % FT_I2C_SINGLE_FRAME_CNT] =
		dev->mng.cur_index;
	/*Update the Tx_Tail*/
	dev->mng.tx_cmd_cnt++;
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_TX_TAIL,
				   dev->mng.tx_cmd_cnt % FT_I2C_SINGLE_FRAME_CNT);
	return FT_I2C_RUNNING;
}

static int i2c_phyt_master_handle(struct i2c_phyt_dev *dev)
{
	struct phyt_msg_info *tx_shmem = (struct phyt_msg_info *)dev->tx_shmem_addr;
	struct phyt_msg_info shmem_msg;
	u32 tx_head;
	int ret;

	if (dev->mng.cur_cmd_cnt >= dev->mng.tx_cmd_cnt)
		return 0;

	/*Get the index of general msg*/
	tx_head = dev->mng.cur_cmd_cnt % FT_I2C_SINGLE_FRAME_CNT;
	dev->mng.cur_cmd_cnt++;

	/*Read the general_msg*/
	memcpy(&shmem_msg, &tx_shmem[tx_head], sizeof(struct phyt_msg_info));
	ret = i2c_phyt_check_status(dev, &shmem_msg);

	/*Error*/
	if (ret)
		return -EINVAL;

	/*no check cmd*/
	if (!dev->mng.is_need_check)
		return 0;

	i2c_phyt_master_parse_data(dev, &shmem_msg, tx_head);
	/*No need to update new data*/
	if (dev->mng.is_last_frame) {
		if (dev->mng.cur_cmd_cnt == dev->mng.tx_cmd_cnt)
			return 0;/*This is last frame */
		return FT_I2C_RUNNING;/*There are some msgs needed to process*/
	}
	return i2c_phyt_master_update_new_msg(dev);
}

static void i2c_phyt_master_xfer_single_frame(struct i2c_phyt_dev *dev)
{
	struct phyt_msg_info i2c_mng_msg;
	struct i2c_ft_trans_msg_info *i2c_msg_info;
	struct phyt_msg_info *shmem_msg =
		(struct phyt_msg_info *)dev->tx_shmem_addr;
	struct i2c_msg *msgs = dev->msgs;
	int i, len = 0, remain_len;

	/*orign info*/
	for (i = 0; i < dev->msgs_num; i++) {
		remain_len = msgs[i].len;
		dev->mng.opt_finish_len = 0;

		while (dev->mng.tx_cmd_cnt < FT_I2C_SINGLE_FRAME_CNT - 1) {
			dev->total_shmem_len = 0;
			memset(&i2c_mng_msg, 0, sizeof(i2c_mng_msg));

			/*Set the Head.len*/
			len = remain_len;
			if (remain_len >= FT_I2C_SINGLE_BUF_LEN)
				len = FT_I2C_SINGLE_BUF_LEN;
			i2c_mng_msg.head.len = len;

			/*Set the Head.seq (Start)*/
			if (!dev->mng.tx_cmd_cnt)
				i2c_mng_msg.head.seq = FT_I2C_TRANS_FRAME_START;

			/*Set the Head.seq (Restart)*/
			if ((i) && ((msgs[i - 1].flags & I2C_M_RD) !=
				    (msgs[i].flags & I2C_M_RD))) {
				if (remain_len == msgs[i].len)
					i2c_mng_msg.head.seq = FT_I2C_TRANS_FRAME_RESTART;
			}
			/*Set the Head.seq (End)*/
			if (((i + 1) >= dev->msgs_num) &&  (len == remain_len)) {
				dev->mng.is_last_frame = true;
				i2c_mng_msg.head.seq |= FT_I2C_TRANS_FRAME_END;
			}

			/*Set other info of the Head*/
			i2c_mng_msg.head.cmd_type = PHYTI2C_MSG_CMD_DATA;
			i2c_mng_msg.head.cmd_subid = PHYTI2C_MSG_CMD_DATA_XFER;
			i2c_mng_msg.head.status0 = FT_I2C_MSG_COMPLETE_UNKNOW;

			/*Store addr flags of struct i2c_msg*/
			i2c_msg_info =
				(struct i2c_ft_trans_msg_info *)&i2c_mng_msg.data[0];
			i2c_msg_info->addr = (u16)msgs[i].addr;
			i2c_msg_info->flags = (u16)msgs[i].flags;

			dev->total_shmem_len = FT_I2C_SHMEM_STORE_OFFSET;
			/*store data*/
			if (!(msgs[i].flags & I2C_M_RD)) {
				memcpy(&i2c_mng_msg.data[sizeof(
					       struct i2c_ft_trans_msg_info)],
				       &msgs[i].buf[dev->mng.opt_finish_len], len);
				dev->total_shmem_len += len;
			}
			/*Update to share memory*/
			memcpy(&shmem_msg[dev->mng.tx_cmd_cnt], &i2c_mng_msg,
			       dev->total_shmem_len);

			/*Store the index of the i2c_msgs\the current index and the finished len*/
			dev->real_index[dev->mng.tx_cmd_cnt] = i;
			dev->mng.cur_index = i;
			dev->mng.opt_finish_len += len;
			/*Record the count of AP2RV's msg*/
			dev->mng.tx_cmd_cnt++;

			remain_len -= len;
			if (!remain_len)
				break;
		}
	}
	/*Update the Tx_Tail*/
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_TX_TAIL, dev->mng.tx_cmd_cnt);
	i2c_phyt_notify_rv(dev, true);
}

static int i2c_phyt_master_xfer(struct i2c_adapter *adapter,
					struct i2c_msg msgs[], int num)
{
	struct i2c_phyt_dev *dev = i2c_get_adapdata(adapter);
	int ret;

	pm_runtime_get_sync(dev->dev);

	dev->msgs = msgs;
	dev->msgs_num = num;
	dev->msg_err = 0;
	dev->abort_source = 0;
	dev->rx_buf_len = 0;
	dev->mng.tx_cmd_cnt = 0;
	dev->mng.cur_cmd_cnt = 0;
	dev->mng.is_last_frame = false;

	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_TX_HEAD, 0);
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_TX_TAIL, 0);

	i2c_phyt_master_xfer_single_frame(dev);

	ret = i2c_phyt_check_result(dev);
	if (!ret) {
		ret = num;
	} else {
		if (dev->abort_source != FT_I2C_SUCCESS) {
			i2c_phyt_handle_tx_abort(dev);
			if (ret == FT_I2C_TIMEOUT) {
				i2c_phyt_set_module_en(
					dev, FT_I2C_ADAPTER_MODULE_RESET);
				i2c_phyt_master_default_init(dev);
				ret = -ETIMEDOUT;
				goto done;
			}
			ret = -EINVAL;
		}
	}

done:
	pm_runtime_mark_last_busy(dev->dev);
	pm_runtime_put_autosuspend(dev->dev);

	return ret;
}

static void
i2c_phyt_master_parse_data(struct i2c_phyt_dev *dev,
				       struct phyt_msg_info *shmem_msg,
				       int index)
{
	int real_index;
	struct i2c_ft_trans_msg_info *i2c_msg_info;
	struct i2c_msg *msgs = dev->msgs;

	i2c_msg_info = (struct i2c_ft_trans_msg_info *)&shmem_msg->data[0];
	/*Find the real index of i2c_msgs*/
	real_index = dev->real_index[index];
	if (real_index >= dev->msgs_num)
		return;

	if (i2c_msg_info->flags & I2C_M_RD) {
		memcpy(&msgs[real_index].buf[dev->rx_buf_len],
		       &shmem_msg->data[sizeof(struct i2c_ft_trans_msg_info)],
		       shmem_msg->head.len);
		dev->rx_buf_len += shmem_msg->head.len;
	}
}

static int i2c_phyt_master_default_init(struct i2c_phyt_dev *dev)
{
	int ret;
	struct i2c_ft_default_cfg_msg cfg_info;

	cfg_info.ss_hcnt = dev->ss_hcnt;
	cfg_info.ss_lcnt = dev->ss_lcnt;
	cfg_info.fs_hcnt = dev->fs_hcnt;
	cfg_info.fs_lcnt = dev->fs_lcnt;
	cfg_info.hs_hcnt = dev->hs_hcnt;
	cfg_info.hs_lcnt = dev->hs_lcnt;
	cfg_info.sda_hold = dev->sda_hold_time;
	cfg_info.tx_fifo_thr = 3;
	cfg_info.rx_fifo_thr = 0;
	cfg_info.smbclk_mext = FT_DEFAULT_TIMEOUT;
	cfg_info.smbclk_timeout = FT_DEFAULT_TIMEOUT;
	cfg_info.smbdat_timeout = FT_DEFAULT_TIMEOUT;
	cfg_info.cfg = dev->master_cfg;
	cfg_info.intr_poll = FT_I2C_IN_INTERRUPT_MODE;
	cfg_info.intr_mask = dev->intr_mask;

	i2c_phyt_default_cfg(dev, &cfg_info);
	ret = i2c_phyt_check_result(dev);
	if (ret) {
		dev_err(dev->dev, "fail to set module open: %d\n", ret);
		return ret;
	}

	return 0;
}

static irqreturn_t i2c_phyt_master_regfile_isr(int this_irq, void *dev_id)
{
	struct i2c_phyt_dev *dev = (struct i2c_phyt_dev *)dev_id;
	struct phyt_msg_info *rx_msg = (struct phyt_msg_info *)dev->rx_shmem_addr;
	u32 stat;
	int ret;

	stat = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_RV2AP_INTR_STAT);

	if (!(stat & FT_I2C_RV2AP_INTR_BIT4)) {
		dev_warn(dev->dev, "unexpect i2c_phyt regfile intr, stat:%#x\n", stat);
		return IRQ_NONE;
	}

	i2c_phyt_common_regfile_clear_rv2ap_int(dev, stat);

	if (dev->complete_flag) {
		if (rx_msg->head.cmd_type == PHYTI2C_MSG_CMD_REPORT)
			goto done;

		ret = i2c_phyt_master_handle(dev);
		if (ret == FT_I2C_RUNNING)
			return IRQ_HANDLED;

		dev->complete_flag = false;
		dev->mng.cur_cmd_cnt = 0;
		i2c_phyt_write_reg(dev, FT_I2C_REGFILE_TX_HEAD, 0);
		i2c_phyt_write_reg(dev, FT_I2C_REGFILE_TX_TAIL, 0);
		complete(&dev->cmd_complete);
		return IRQ_HANDLED;
	}
done:
	i2c_phyt_master_isr_handle(dev);

	return IRQ_HANDLED;
}

static int i2c_phyt_enable_smbus_alert(struct i2c_phyt_dev *i2c_dev)
{
	struct i2c_adapter *adap = &i2c_dev->adapter;
	struct i2c_smbus_alert_setup setup;

	setup.irq = 0;
	i2c_dev->ara = i2c_new_smbus_alert_device(adap, &setup);

	if (IS_ERR(i2c_dev->ara))
		return PTR_ERR(i2c_dev->ara);

	return 0;
}

int i2c_phyt_master_probe(struct i2c_phyt_dev *dev)
{
	struct i2c_adapter *adapter = &dev->adapter;
	unsigned long irq_flags;
	int ret;
	u32 alert = 0;

	init_completion(&dev->cmd_complete);

	i2c_phyt_common_regfile_disable_int(dev);
	i2c_phyt_common_regfile_clear_rv2ap_int(dev, 0);

	irq_flags = IRQF_SHARED | IRQF_COND_SUSPEND;
	ret = devm_request_irq(dev->dev, dev->irq, i2c_phyt_master_regfile_isr, irq_flags,
			dev_name(dev->dev), dev);
	if (ret) {
		dev_err(dev->dev, "failure requesting irq %i: %d\n",
			dev->irq, ret);
		return ret;
	}

	i2c_phyt_common_regfile_enable_int(dev);

	ret = i2c_phyt_set_rx_addr(dev);
	if (ret) {
		dev_err(dev->dev, "failed set rx addr\n");
		return ret;
	}

	ret = i2c_phyt_master_set_timings_master(dev);
	if (ret) {
		dev_err(dev->dev, "master set times\n");
		return ret;
	}

	dev->intr_mask = FT_IC_INTR_SMBUS_TIME_MASK;
	ret = i2c_phyt_master_default_init(dev);
	if (ret) {
		dev_err(dev->dev, "master default init\n");
		return ret;
	}
	dev->init = i2c_phyt_master_default_init;
	dev->disable = i2c_phyt_set_suspend;
	dev->disable_int = i2c_phyt_disable_int;

	/* XXX: should be initialized in firmware, remove it in future */
	snprintf(adapter->name, sizeof(adapter->name), "Phytium I2C_2.0 Master adapter");
	adapter->retries = 3;
	adapter->algo = &i2c_phyt_master_algo;
	adapter->dev.parent = dev->dev;
	i2c_set_adapdata(adapter, dev);
	/*
	 * Increment PM usage count during adapter registration in order to
	 * avoid possible spurious runtime suspend when adapter device is
	 * registered to the device core and immediate resume in case bus has
	 * registered I2C slaves that do I2C transfers in their probe.
	 */
	pm_runtime_get_noresume(dev->dev);

	ret = i2c_add_numbered_adapter(adapter);
	if (ret) {
		dev_err(dev->dev, "fail to add adapter: %d\n", ret);
		goto exit_probe;
	}

	if (of_property_read_bool(dev->dev->of_node, "smbus-alert"))
		alert = 1;
	else if (has_acpi_companion(dev->dev))
		fwnode_property_read_u32_array(dev->dev->fwnode, "smbus_alert", &alert, 1);

	if (alert) {
		ret = i2c_phyt_enable_smbus_alert(dev);
		if (ret) {
			dev_err(dev->dev,
			"failed to enable SMBus alert protocol (%d)\n", ret);
		}
		dev->intr_mask |= FT_IC_INTR_SMBALERT_IN_N;
		i2c_phyt_set_cmd32(
			dev, PHYTI2C_MSG_CMD_SET_INTERRUPT, dev->intr_mask);
		ret = i2c_phyt_check_result(dev);
		if (ret)
			dev_warn(dev->dev, "fail to set interrupt: %d\n", ret);
	}

exit_probe:
	pm_runtime_put_noidle(dev->dev);
	dev_info(dev->dev, "i2c_2.0 master probe ok\n");

	return ret;
}
EXPORT_SYMBOL_GPL(i2c_phyt_master_probe);

MODULE_DESCRIPTION("Phytium I2C bus master adapter");
MODULE_LICENSE("GPL");

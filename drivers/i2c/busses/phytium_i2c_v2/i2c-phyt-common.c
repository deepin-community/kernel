// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Phytium I2C adapter driver.
 *
 * Copyright (C) 2023-2024, Phytium Technology Co., Ltd.
 */
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/export.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/i2c-smbus.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/swab.h>

#include "i2c-phyt-core.h"

#define FT_LOG_LINE_MAX_LEN		400
#define FT_LOG_MAX_SIZE			8192

static void i2c_phyt_send_msg(struct i2c_phyt_dev *dev,
				struct phyt_msg_info *set_msg, bool complete_flag);

static char *i2c_ft_abort_sources[] = {
	[FT_I2C_TIMEOUT] = "I2C cmd not acknowledged",
	[FT_I2C_CNT_ERR] = "I2C timings count error",
	[FT_I2C_TX_ABRT] = "Tx abort",
	[FT_I2C_INT_ERR] = "Interrupt error",
	[FT_I2C_BLOCK_SIZE] = "smbus block error",
	[FT_I2C_INVALID_ADDR] = "slave address invalid",
	[FT_I2C_CHECK_STATUS_ERR] = "Uncomplete status",
};

u32 i2c_phyt_scl_hcnt(u32 ic_clk, u32 tSYMBOL, u32 tf, int cond, int offset)
{
	if (cond)
		return (ic_clk * tSYMBOL + 500000) / 1000000 - 8 + offset;
	else
		return (ic_clk * (tSYMBOL + tf) + 500000) / 1000000 - 3 + offset;
}

u32 i2c_phyt_scl_lcnt(u32 ic_clk, u32 tLOW, u32 tf, int offset)
{
	return ((ic_clk * (tLOW + tf) + 500000) / 1000000) - 1 + offset;
}

unsigned long i2c_phyt_clk_rate(struct i2c_phyt_dev *dev)
{
	if (WARN_ON_ONCE(!dev->get_clk_rate_khz))
		return 0;

	return dev->get_clk_rate_khz(dev);
}

int i2c_phyt_prepare_clk(struct i2c_phyt_dev *dev, bool prepare)
{
	if (IS_ERR(dev->clk))
		return PTR_ERR(dev->clk);

	if (prepare)
		return clk_prepare_enable(dev->clk);

	clk_disable_unprepare(dev->clk);

	return 0;
}

void i2c_phyt_handle_tx_abort(struct i2c_phyt_dev *dev)
{
	unsigned long abort_source = dev->abort_source;
	int i;

	for_each_set_bit(i, &abort_source, ARRAY_SIZE(i2c_ft_abort_sources))
		dev_info(dev->dev, "%s: %s\n", __func__, i2c_ft_abort_sources[i]);
}

u32 i2c_phyt_func(struct i2c_adapter *adapter)
{
	struct i2c_phyt_dev *dev = i2c_get_adapdata(adapter);

	return dev->functionality;
}

void i2c_phyt_common_watchdog(struct i2c_phyt_dev *dev)
{
	u32 reg;

	reg = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_DEBUG);

	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_DEBUG,
			reg | FT_I2C_REGFILE_HEARTBIT_VAL);
}

void i2c_phyt_enable_alive(struct i2c_phyt_dev *dev)
{
	u32 reg;

	reg = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_DEBUG);

	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_DEBUG,
			reg | FT_I2C_REGFILE_HEARTBIT_VAL);

	reg = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_DEBUG);
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_DEBUG,
			reg | FT_I2C_LOG_ADDR_LOG_ALIVE);
}

void i2c_phyt_disable_alive(struct i2c_phyt_dev *dev)
{
	u32 reg;

	reg = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_DEBUG);
	reg &= ~FT_I2C_LOG_ADDR_LOG_ALIVE;

	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_DEBUG, reg);
}

void i2c_phyt_enable_debug(struct i2c_phyt_dev *dev)
{
	u32 reg;

	reg = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_DEBUG);

	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_DEBUG,
			reg | FT_I2C_LOG_ADDR_LOG_DEBUG);
}

void i2c_phyt_disable_debug(struct i2c_phyt_dev *dev)
{
	u32 reg;

	reg = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_DEBUG);
	reg &= ~FT_I2C_LOG_ADDR_LOG_DEBUG;

	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_DEBUG, reg);
}


void i2c_phyt_show_log(struct i2c_phyt_dev *dev)
{
	u32 i, reg, len;
	u8 *plog;

	if (!dev->log_addr)
		return;

	/*set lock*/
	reg = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_DEBUG);
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_DEBUG,
			reg | FT_I2C_LOG_ADDR_LOCK_VAL);

	plog = dev->log_addr;
	if (reg & FT_I2C_LOG_ADDR_LOG_FLAG) {
		len = strnlen((char *)dev->log_addr, FT_LOG_MAX_SIZE);
		dev_info(dev->dev, "log len :%d,addr: 0x%llx,size:%d\n", len, (u64)dev->log_addr,
					dev->log_size);
		if (len > FT_LOG_LINE_MAX_LEN) {
			for (i = 0; i < len; i += FT_LOG_LINE_MAX_LEN)
				dev_info(dev->dev, "(log)%.*s\n", FT_LOG_LINE_MAX_LEN, &plog[i]);
		} else {
			dev_info(dev->dev, "(log)%.*s\n", FT_LOG_LINE_MAX_LEN, &plog[0]);
		}

		for (i = 0; i < dev->log_size; i++)
			plog[i] = 0;
	}
	/*clear log flag*/
	reg &= ~FT_I2C_LOG_ADDR_LOG_FLAG;
	/*unset lock*/
	reg &= ~FT_I2C_LOG_ADDR_LOCK_VAL;
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_DEBUG, reg);
}

int  i2c_phyt_malloc_log_mem(struct i2c_phyt_dev *dev)
{
	u32 reg;
	u64 phy_addr;

	reg = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_DEBUG);
	phy_addr = ((reg & FT_I2C_LOG_ADDR_MASK) >> FT_I2C_LOG_ADDR_LOW_SHIFT) <<
							FT_I2C_LOG_ADDR_SHIFT;
	dev->log_size = ((reg & FT_I2C_LOG_SIZE_MASK) >> FT_I2C_LOG_SIZE_LOW_SHIFT) * 1024;

	dev->log_addr = devm_ioremap(dev->dev, phy_addr, dev->log_size);

	if (IS_ERR(dev->log_addr)) {
		dev_err(dev->dev, "log_addr is err\n");
		return -ENOMEM;
	}

	return 0;
}

void i2c_phyt_common_regfile_disable_int(struct i2c_phyt_dev *dev)
{
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_RV2AP_INTR_MASK,
			FT_I2C_REGFILE_DISABLE_INTR_VAL);
}

void i2c_phyt_common_regfile_enable_int(struct i2c_phyt_dev *dev)
{
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_RV2AP_INTR_MASK,
					FT_I2C_REGFILE_ENABLE_INTR_VAL);
}

void i2c_phyt_common_regfile_clear_rv2ap_int(struct i2c_phyt_dev *dev,
						u32 stat)
{
	/*For Desktop type,this opt is useful*/
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_RV2AP_INTR_STAT,
					stat & ~FT_I2C_RV2AP_INTR_BIT4);
	/*For Service type,this opt is useful*/
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_RV2AP_INTR_CLEAR,
					stat | FT_I2C_RV2AP_INTR_BIT4);
}

void i2c_phyt_common_set_cmd(struct i2c_phyt_dev *dev,
				struct phyt_msg_info *i2c_mng_msg, u8 cmd, u8 sub_cmd)
{
	i2c_mng_msg->head.cmd_type = cmd;
	i2c_mng_msg->head.cmd_subid = sub_cmd;
	i2c_mng_msg->head.status0 = FT_I2C_MSG_COMPLETE_UNKNOW;
}

void i2c_phyt_set_cmd8(struct i2c_phyt_dev *dev,
				u8 sub_cmd, u8 data)
{
	struct phyt_msg_info i2c_mng_msg;

	memset(&i2c_mng_msg, 0, sizeof(i2c_mng_msg));

	i2c_phyt_common_set_cmd(dev, &i2c_mng_msg, PHYTI2C_MSG_CMD_SET, sub_cmd);
	i2c_mng_msg.data[0] = data;
	dev->total_shmem_len = FT_I2C_MSG_CMDDATA_SIZE;

	i2c_phyt_send_msg(dev, &i2c_mng_msg, true);
}

void i2c_phyt_set_cmd16(struct i2c_phyt_dev *dev,
				u8 sub_cmd, u16 data)
{
	struct phyt_msg_info i2c_mng_msg;
	u16 *ctrl = (u16 *)&i2c_mng_msg.data[0];

	memset(&i2c_mng_msg, 0, sizeof(i2c_mng_msg));

	i2c_phyt_common_set_cmd(dev, &i2c_mng_msg, PHYTI2C_MSG_CMD_SET, sub_cmd);
	*ctrl = data;
	dev->total_shmem_len = FT_I2C_MSG_CMDDATA_SIZE;

	i2c_phyt_send_msg(dev, &i2c_mng_msg, true);
}

void i2c_phyt_set_cmd32(struct i2c_phyt_dev *dev,
				u8 sub_cmd, u32 data)
{
	struct phyt_msg_info i2c_mng_msg;
	u32 *cmd_data = (u32 *)&i2c_mng_msg.data[0];

	memset(&i2c_mng_msg, 0, sizeof(i2c_mng_msg));

	i2c_phyt_common_set_cmd(dev, &i2c_mng_msg, PHYTI2C_MSG_CMD_SET, sub_cmd);
	*cmd_data = data;
	dev->total_shmem_len = FT_I2C_MSG_CMDDATA_SIZE;

	i2c_phyt_send_msg(dev, &i2c_mng_msg, true);
}

void i2c_phyt_data_cmd8_array(struct i2c_phyt_dev *dev,
					u8 sub_cmd, u8 *data, int len)
{
	struct phyt_msg_info i2c_mng_msg;

	memset(&i2c_mng_msg, 0, sizeof(i2c_mng_msg));

	/*set cmd*/
	i2c_phyt_common_set_cmd(dev, &i2c_mng_msg, PHYTI2C_MSG_CMD_DATA, sub_cmd);
	/*set data*/
	memcpy(&i2c_mng_msg.data[0], &data[0], len);
	/*set len*/
	dev->total_shmem_len = FT_I2C_MSG_CMDDATA_SIZE;

	i2c_phyt_send_msg(dev, &i2c_mng_msg, false);
}

void i2c_phyt_default_cfg(struct i2c_phyt_dev *dev,
					struct i2c_ft_default_cfg_msg *buf)
{
	struct phyt_msg_info i2c_mng_msg;

	memset(&i2c_mng_msg, 0, sizeof(i2c_mng_msg));

	i2c_phyt_common_set_cmd(dev, &i2c_mng_msg, PHYTI2C_MSG_CMD_DEFAULT,
					PHYTI2C_MSG_CMD_DEFAULT_RESUME);
	memcpy(&i2c_mng_msg.data[0], (char *)buf, sizeof(struct i2c_ft_default_cfg_msg));
	dev->total_shmem_len = FT_I2C_MSG_CMDDATA_SIZE;

	i2c_phyt_send_msg(dev, &i2c_mng_msg, true);
}

void i2c_phyt_disable_int(struct i2c_phyt_dev *dev)
{
	i2c_phyt_set_cmd32(dev, PHYTI2C_MSG_CMD_SET_INTERRUPT, 0);
}

void i2c_phyt_trig_rv_intr(struct i2c_phyt_dev *dev)
{
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_AP2RV_INTR_STATE,
				FT_I2C_REGFILE_AP2RV_SET_INTR_VAL);
}

static void i2c_phyt_send_msg(struct i2c_phyt_dev *dev,
				struct phyt_msg_info *set_msg, bool complete_flag)
{
	u64 *p_dest_msg = (u64 *)dev->tx_shmem_addr;
	u64 *p_src_msg = (u64 *)set_msg;
	int i, len;

	if (complete_flag) {
		reinit_completion(&dev->cmd_complete);
		dev->complete_flag = true;
		dev->mng.is_need_check = false;
	}
	len = DIV_ROUND_UP(dev->total_shmem_len, 8);

	for (i = 0; i < len; i++)
		p_dest_msg[i] = p_src_msg[i];
	/*For config cmd,no use tail and head,set this value to zero*/
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_TX_TAIL, 0);
	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_TX_HEAD, 0);
	dev->mng.tx_cmd_cnt = 1;
	dev->mng.cur_cmd_cnt = 0;
	i2c_phyt_trig_rv_intr(dev);
}

void i2c_phyt_notify_rv(struct i2c_phyt_dev *dev,
				    bool need_check)
{
	reinit_completion(&dev->cmd_complete);
	dev->complete_flag = true;

	dev->mng.is_need_check = need_check;
	i2c_phyt_trig_rv_intr(dev);
}

void i2c_phyt_set_suspend(struct i2c_phyt_dev *dev)
{
	i2c_phyt_set_cmd32(dev, PHYTI2C_MSG_CMD_SET_SUSPEND, 0);

	if (i2c_phyt_check_result(dev))
		dev_warn(dev->dev, "fail to set suspend\n");
}

void i2c_phyt_set_sda_hold(struct i2c_phyt_dev *dev, u32 data)
{
	i2c_phyt_set_cmd32(dev, PHYTI2C_MSG_CMD_SET_SDA_HOLD, data);
}

int i2c_phyt_check_status(struct i2c_phyt_dev *dev,
				 struct phyt_msg_info *msg)
{
	int result = FT_I2C_CHECK_STATUS_ERR;

	dev->abort_source = 1 << FT_I2C_CHECK_STATUS_ERR;
	/* RV Has update the result,but not sure the result is OK*/
	if (msg->head.status0 != FT_I2C_MSG_COMPLETE_UNKNOW) {
		if (!dev->mng.is_need_check ||
		    ((msg->head.status0 == FT_I2C_MSG_COMPLETE_OK) &&
		     (msg->head.status1 == FT_I2C_SUCCESS))) {
			dev->abort_source = 0;
			result = FT_I2C_SUCCESS;
		} else {
			i2c_phyt_show_log(dev);
			dev->abort_source = 1 << msg->head.status1;
		}
	}

	return result;
}

int i2c_phyt_check_result(struct i2c_phyt_dev *dev)
{
	if (!wait_for_completion_timeout(&dev->cmd_complete,
					 dev->adapter.timeout)) {
		dev_err(dev->dev, "check timed out\n");
		i2c_phyt_show_log(dev);
		dev->abort_source = BIT(FT_I2C_TIMEOUT);
		return -EINVAL;
	}
	if (!dev->abort_source)
		return 0;
	return -EINVAL;
}

void i2c_phyt_disable(struct i2c_phyt_dev *dev)
{
	i2c_phyt_set_module_en(dev, FT_I2C_ADAPTER_MODULE_OFF);

	i2c_phyt_check_result(dev);
}

void i2c_phyt_set_module_en(struct i2c_phyt_dev *dev, u8 data)
{
	i2c_phyt_set_cmd8(dev, PHYTI2C_MSG_CMD_SET_MODULE_EN, data);
}

void i2c_phyt_set_int_tl(struct i2c_phyt_dev *dev,
				u8 tx_threshold, u8 rx_threshold)
{
	struct phyt_msg_info i2c_mng_msg;
	struct i2c_phyt_fifo_threshold *ctrl =
			(struct i2c_phyt_fifo_threshold *)&i2c_mng_msg.data[0];

	memset(&i2c_mng_msg, 0, sizeof(i2c_mng_msg));

	i2c_phyt_common_set_cmd(dev, &i2c_mng_msg, PHYTI2C_MSG_CMD_SET,
					PHYTI2C_MSG_CMD_SET_INT_TL);

	ctrl->tx_fifo_threshold = tx_threshold;
	ctrl->rx_fifo_threshold = rx_threshold;

	dev->total_shmem_len = FT_I2C_MSG_CMDDATA_SIZE;

	i2c_phyt_send_msg(dev, &i2c_mng_msg, true);
}

void i2c_phyt_set_int_interrupt(struct i2c_phyt_dev *dev,
				u32 is_enable, u32 intr_mask)
{
	struct phyt_msg_info i2c_mng_msg;
	u32 *data = (u32 *)i2c_mng_msg.data;

	memset(&i2c_mng_msg, 0, sizeof(i2c_mng_msg));

	i2c_phyt_common_set_cmd(dev, &i2c_mng_msg, PHYTI2C_MSG_CMD_SET,
					PHYTI2C_MSG_CMD_SET_INTERRUPT);

	data[0] = is_enable;
	data[1] = intr_mask;
	dev->total_shmem_len = FT_I2C_MSG_CMDDATA_SIZE;

	i2c_phyt_send_msg(dev, &i2c_mng_msg, true);
}

static int i2c_phyt_report_cmd_handle(struct i2c_phyt_dev *dev,
					struct phyt_msg_info *rx_msg, u32 head, u32 tail)
{
	u16 sub_cmd = rx_msg->head.cmd_subid;
	int ret = -EINVAL;

	switch (sub_cmd) {
	case PHYTI2C_MSG_CMD_SLAVE_EVENT:
		ret = i2c_phyt_slave_event_process(dev, rx_msg, head, tail);
		break;
	case PHYTI2C_MSG_CMD_SMBALERT_IN_N:
		ret = i2c_phyt_master_smbus_alert_process(dev);
		break;
	default:
		break;
	}

	return ret;
}

void i2c_phyt_slave_isr_handle(struct i2c_phyt_dev *dev)
{
	struct phyt_msg_info *rx_msg = (struct phyt_msg_info *)dev->rx_shmem_addr;
	struct phyt_msg_info msg_buf;
	u32 head, tail;

	tail = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_TAIL);
	head = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_HEAD);

	/*There is only one rx_msg*/
	memcpy(&msg_buf, &rx_msg[0], sizeof(msg_buf));

	if (msg_buf.head.cmd_type == PHYTI2C_MSG_CMD_REPORT)
		i2c_phyt_report_cmd_handle(dev, &msg_buf, head, tail);
}

static void i2c_phyt_set_complete(struct i2c_phyt_dev *dev)
{
	struct phyt_msg_info *set_msg = (struct phyt_msg_info *)dev->rx_shmem_addr;
	struct phyt_msg_info tmp_msg;

	/*There is only one rx_msg*/
	memcpy(&tmp_msg, &set_msg[0], sizeof(struct phyt_msg_info));
	tmp_msg.head.status0 = FT_I2C_MSG_COMPLETE_OK;
	tmp_msg.head.cmd_type = 0xff;
	tmp_msg.head.cmd_subid = 0xff;
	memcpy(&set_msg[0], &tmp_msg, sizeof(struct phyt_msg_info));
}

void i2c_phyt_master_isr_handle(struct i2c_phyt_dev *dev)
{
	struct phyt_msg_info *rx_msg = (struct phyt_msg_info *)dev->rx_shmem_addr;
	struct phyt_msg_info msg_buf;

	memcpy(&msg_buf, &rx_msg[0], sizeof(struct phyt_msg_info));
	i2c_phyt_set_complete(dev);

	if (msg_buf.head.cmd_type == PHYTI2C_MSG_CMD_REPORT)
		i2c_phyt_report_cmd_handle(dev, &msg_buf, 0, 0);
}

MODULE_AUTHOR("Wu Jinyong <wujinyong1788@phytium.com.cn>");
MODULE_DESCRIPTION("Phytium I2C bus adapter core");
MODULE_LICENSE("GPL");

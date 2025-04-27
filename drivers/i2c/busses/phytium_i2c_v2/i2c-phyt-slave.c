// SPDX-License-Identifier: GPL-2.0
/*
 * Phytium I2C adapter driver (slave only).
 *
 * Copyright (C) 2021-2023, Phytium Technology Co., Ltd.
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include "i2c-phyt-core.h"

#define FT_I2C_RX_FIFO_FULL_ENABLE		1
#define FT_I2C_RX_FIFO_FULL_DISABLE		0

#define FT_I2C_CON_STOP_DET_IFADDR_ENABLE	1
#define FT_I2C_CON_STOP_DET_IFADDR_DISABLE	0

#define FT_I2C_SLAVE_EVNET_MAX_CNT		64
#define FT_I2C_SLAVE_DATA_INDEX(index)		(index+1)
/*one is cmd,the next one is data*/
#define FT_I2C_SLAVE_RX_INFO_SIZE		2

int i2c_phyt_slave_event_process(struct i2c_phyt_dev *dev,
			struct phyt_msg_info *rx_msg, u32 rx_head, u32 rx_tail)
{
	u8 buf[32] = {1};
	u32 head = rx_head, tail = rx_tail;

	if (!dev || !dev->slave) {
		dev_err(dev->dev, "dev is null\n");
		return -ENXIO;
	}

	if ((head >= FT_I2C_SLAVE_EVNET_MAX_CNT) || (tail >= FT_I2C_SLAVE_EVNET_MAX_CNT)) {
		dev_err(dev->dev, "head:%d, tail:%d\n", head, tail);
		return -EINVAL;
	}

	while (head != tail) {
		if (head % FT_I2C_SLAVE_RX_INFO_SIZE)
			head++;

		i2c_slave_event(dev->slave, rx_msg->data[head],
				&(rx_msg->data[FT_I2C_SLAVE_DATA_INDEX(head)]));
		/*send the read data to RV*/
		if ((rx_msg->data[head] == I2C_SLAVE_READ_REQUESTED) ||
				(rx_msg->data[head] == I2C_SLAVE_READ_PROCESSED)) {
			/*buf[0] is used to stor size,and  fixed to 1*/
			buf[1] = rx_msg->data[FT_I2C_SLAVE_DATA_INDEX(head)];
			i2c_phyt_data_cmd8_array(dev, PHYTI2C_MSG_CMD_DATA_SLAVE,
								buf, 2);
		}
		head += FT_I2C_SLAVE_RX_INFO_SIZE;
		if (head >= FT_I2C_SLAVE_EVNET_MAX_CNT)
			head = 0;
	}

	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_HEAD, head);

	return 0;
}

static irqreturn_t i2c_phyt_slave_regfile_isr(int this_irq, void *dev_id)
{
	u32 stat;
	struct i2c_phyt_dev *dev = (struct i2c_phyt_dev *)dev_id;

	stat = i2c_phyt_read_reg(dev, FT_I2C_REGFILE_RV2AP_INTR_STAT);

	if (!(stat & FT_I2C_RV2AP_INTR_BIT4)) {
		dev_warn(dev->dev, "intr stat=%#x\n",  stat);
		return IRQ_NONE;
	}

	i2c_phyt_common_regfile_clear_rv2ap_int(dev, stat);

	if (dev->complete_flag) {
		dev->complete_flag = false;
		complete(&dev->cmd_complete);
	}	else {
		i2c_phyt_slave_isr_handle(dev);
		i2c_phyt_show_log(dev);
	}

	return IRQ_HANDLED;
}

static int i2c_phyt_slave_init(struct i2c_phyt_dev *dev)
{
	u8 slave_mode = FT_I2C_MASTER_MODE_FLAG;
	u8 rx_fifo_full_en = FT_I2C_RX_FIFO_FULL_DISABLE;
	u8 stop_det_ifaddr_en = FT_I2C_CON_STOP_DET_IFADDR_DISABLE;
	int ret;

	i2c_phyt_set_module_en(dev, FT_I2C_ADAPTER_MODULE_OFF);
	ret = i2c_phyt_check_result(dev);
	if (ret) {
		dev_err(dev->dev, "fail to set module off: %d\n", ret);
		return ret;
	}

	if (dev->sda_hold_time) {
		i2c_phyt_set_sda_hold(dev, dev->sda_hold_time);
		ret = i2c_phyt_check_result(dev);
		if (ret) {
			dev_err(dev->dev, "fail to set sda hold time: %d\n", ret);
			return ret;
		}
	}

	/*set slave mode*/
	if ((dev->mode & FT_I2C_CON_MASTER_MODE_MASK) == phyt_IC_SLAVE)
		slave_mode = FT_I2C_SLAVE_MODE_FLAG;
	else {
		dev_err(dev->dev, "dev is not in slave mode\n");
		return -EINVAL;
	}

	i2c_phyt_set_cmd8(dev, PHYTI2C_MSG_CMD_SET_MODE, slave_mode);
	ret = i2c_phyt_check_result(dev);
	if (ret) {
		dev_err(dev->dev, "fail to set mode: %d\n", ret);
		return ret;
	}

	i2c_phyt_set_int_tl(dev, 0, 0);
	ret = i2c_phyt_check_result(dev);
	if (ret) {
		dev_err(dev->dev, "fail to set int tl: %d\n", ret);
		return ret;
	}

	if ((dev->slave_cfg & FT_I2C_CON_RX_FIFO_FULL_HLD_MASK) ==
			FT_IC_CON_RX_FIFO_FULL_HLD_CTRL)
		rx_fifo_full_en = FT_I2C_RX_FIFO_FULL_ENABLE;

	i2c_phyt_set_cmd8(dev, PHYTI2C_MSG_CMD_SET_RX_FIFO_FULL, rx_fifo_full_en);
	ret = i2c_phyt_check_result(dev);
	if (ret) {
		dev_err(dev->dev, "fail to set rx fifo full: %d\n", ret);
		return ret;
	}

	if ((dev->slave_cfg & FT_I2C_CON_STOP_DET_IFADDR_MASK) == FT_IC_CON_STOP_DET_IFADDRESSED)
		stop_det_ifaddr_en = FT_I2C_CON_STOP_DET_IFADDR_ENABLE;

	i2c_phyt_set_cmd8(dev, PHYTI2C_MSG_CMD_SET_STOP_DET_IF_ADDRESSED,
					stop_det_ifaddr_en);
	ret = i2c_phyt_check_result(dev);
	if (ret) {
		dev_err(dev->dev, "fail to set stop det ifaddr: %d\n", ret);
		return ret;
	}
	i2c_phyt_set_cmd32(dev, PHYTI2C_MSG_CMD_SET_INTERRUPT, FT_IC_INTR_SLAVE_MASK);
	ret = i2c_phyt_check_result(dev);
	if (ret) {
		dev_err(dev->dev, "fail to set interrupt: %d\n", ret);
		return ret;
	}

	return 0;
}

static int i2c_phyt_reg_slave(struct i2c_client *slave)
{
	int ret;
	struct i2c_phyt_dev *dev = i2c_get_adapdata(slave->adapter);

	if (dev->slave)
		return -EBUSY;
	if (slave->flags & I2C_CLIENT_TEN)
		return -EAFNOSUPPORT;

	pm_runtime_get_sync(dev->dev);

	/*
	 * Set slave address in the IC_SAR register,
	 * the address to which the i2c responds.
	 */
	i2c_phyt_set_module_en(dev, FT_I2C_ADAPTER_MODULE_OFF);
	ret = i2c_phyt_check_result(dev);
	if (ret) {
		dev_err(dev->dev, "fail to set module off: %d\n", ret);
		return ret;
	}

	i2c_phyt_set_cmd8(dev, PHYTI2C_MSG_CMD_SET_ADDR_MODE, FT_I2C_ADDR_7BIT_MODE);
	ret = i2c_phyt_check_result(dev);
	if (ret) {
		dev_err(dev->dev, "fail to set addr mode: %d\n", ret);
		return ret;
	}

	i2c_phyt_set_cmd8(dev, PHYTI2C_MSG_CMD_SET_ADDR, slave->addr);
	ret = i2c_phyt_check_result(dev);
	if (ret) {
		dev_err(dev->dev, "fail to set addr: %d\n", ret);
		return ret;
	}

	dev->slave = slave;

	i2c_phyt_set_module_en(dev, FT_I2C_ADAPTER_MODULE_ON);
	ret = i2c_phyt_check_result(dev);
	if (ret) {
		dev_err(dev->dev, "fail to set module on: %d\n", ret);
		return ret;
	}

	return 0;
}

static int i2c_phyt_unreg_slave(struct i2c_client *slave)
{
	struct i2c_phyt_dev *dev = i2c_get_adapdata(slave->adapter);

	dev->disable_int(dev);
	dev->disable(dev);
	dev->slave = NULL;
	pm_runtime_put(dev->dev);

	return 0;
}

static const struct i2c_algorithm i2c_phyt_slave_algo = {
	.functionality = i2c_phyt_func,
	.reg_slave = i2c_phyt_reg_slave,
	.unreg_slave = i2c_phyt_unreg_slave,
};


int i2c_phyt_slave_probe(struct i2c_phyt_dev *dev)
{
	struct i2c_adapter *adap = &dev->adapter;
	int ret;

	init_completion(&dev->cmd_complete);

	i2c_phyt_common_regfile_disable_int(dev);

	ret = devm_request_irq(dev->dev, dev->irq, i2c_phyt_slave_regfile_isr, IRQF_SHARED,
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

	dev->init = i2c_phyt_slave_init;
	dev->disable = i2c_phyt_disable;
	dev->disable_int = i2c_phyt_disable_int;

	snprintf(adap->name, sizeof(adap->name),
		 "Phytium I2C_2.0 Slave adapter");
	adap->retries = 3;
	adap->algo = &i2c_phyt_slave_algo;
	adap->dev.parent = dev->dev;

	ret = i2c_phyt_slave_init(dev);
	if (ret)
		return ret;

	i2c_phyt_write_reg(dev, FT_I2C_REGFILE_HEAD, 0);
	i2c_set_adapdata(adap, dev);

	ret = i2c_add_numbered_adapter(adap);
	if (ret)
		dev_err(dev->dev, "failure adding adapter: %d\n", ret);

	dev_info(dev->dev, "i2c_2.0 slave probe ok\n");

	return ret;
}
EXPORT_SYMBOL_GPL(i2c_phyt_slave_probe);

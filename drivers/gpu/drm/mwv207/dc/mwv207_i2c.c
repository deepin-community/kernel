/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include "mwv207.h"
#include "mwv207_vi.h"

static int i2c_scl_gpio[8] = {19, 21, 23, 25, 27, 29, 31, 33};
static int i2c_sda_gpio[8] = {20, 22, 24, 26, 28, 30, 32, 34};

struct mwv207_i2c {
	struct i2c_adapter adapter;
	struct mwv207_device *jdev;
	struct i2c_algo_bit_data bit;
	struct mutex *mutex;
	u32 sda_in_addr;
	u32 sda_out_addr;
	u32 sda_mask;
	u32 sda_dir;
	u32 scl_in_addr;
	u32 scl_out_addr;
	u32 scl_mask;
	u32 scl_dir;
};

static inline void mwv207_i2c_set_dir(struct mwv207_i2c *i2c, u32 mask,
		u32 reg, bool is_input)
{

	mb();

	jdev_modify(i2c->jdev, reg, mask, is_input ? mask : 0);

	mb();
}

static void mwv207_i2c_gpio_multi(struct mwv207_i2c *i2c, int mask)
{
	struct mwv207_device *jdev = i2c->jdev;

	switch (mask) {
	case (1 << 19):

		jdev_modify(jdev, (0x9b0918), 0x3 << 12, 0);
		jdev_modify(jdev, (0x9b0918), 0x3 << 16, 0);
		break;
	case (1 << 21):

		jdev_modify(jdev, (0x9b0918), 0x3 << 20, 0);
		jdev_modify(jdev, (0x9b0918), 0x3 << 24, 0);
		break;
	case (1 << 23):

		jdev_modify(jdev, (0x9b0918), 0x3 << 28, 0);
		jdev_modify(jdev, (0x9b091c), 0x3, 0);
		break;
	case (1 << 25):

		jdev_modify(jdev, (0x9b091c), 0x3 << 4, 0);
		jdev_modify(jdev, (0x9b091c), 0x3 << 8, 0);
		break;
	case (1 << 27):

		jdev_modify(jdev, (0x9b091c), 0x3 << 12, 0);
		jdev_modify(jdev, (0x9b091c), 0x3 << 16, 0);
		break;
	case (1 << 29):

		jdev_modify(jdev, (0x9b091c), 0x3 << 20, 0);
		jdev_modify(jdev, (0x9b091c), 0x3 << 24, 0);
		break;
	case (1 << 31):

		jdev_modify(jdev, (0x9b091c), 0x3 << 28, 0);
		jdev_modify(jdev, (0x9b0910), 0x3, 0);
		break;
	case (1 << 1):

		jdev_modify(jdev, (0x9b0910), 0x3 << 4, 0);
		jdev_modify(jdev, (0x9b0910), 0x3 << 8, 0);
		break;
	default:
		break;
	}
}

static int mwv207_i2c_pre_xfer(struct i2c_adapter *i2c_adap)
{
	struct mwv207_i2c *i2c = i2c_get_adapdata(i2c_adap);
	struct mwv207_device *jdev = i2c->jdev;

	mutex_lock(i2c->mutex);
	mwv207_i2c_gpio_multi(i2c, i2c->scl_mask);

	jdev_modify(jdev, i2c->scl_out_addr, i2c->scl_mask, 0);
	jdev_modify(jdev, i2c->sda_out_addr, i2c->sda_mask, 0);

	mwv207_i2c_set_dir(i2c, i2c->scl_mask, i2c->scl_dir, 1);
	mwv207_i2c_set_dir(i2c, i2c->sda_mask, i2c->sda_dir, 1);

	return 0;
}

static void mwv207_i2c_post_xfer(struct i2c_adapter *i2c_adap)
{
	struct mwv207_i2c *i2c = i2c_get_adapdata(i2c_adap);

	mutex_unlock(i2c->mutex);
}

static int mwv207_i2c_get_clock(void *i2c_priv)
{
	struct mwv207_i2c *i2c = (struct mwv207_i2c *)i2c_priv;
	struct mwv207_device *jdev = i2c->jdev;
	u32 val;

	mwv207_i2c_set_dir(i2c, i2c->scl_mask, i2c->scl_dir, 1);

	val = jdev_read(jdev, i2c->scl_in_addr);
	val &= i2c->scl_mask;

	return val ? 1 : 0;
}

static int mwv207_i2c_get_data(void *i2c_priv)
{
	struct mwv207_i2c *i2c = (struct mwv207_i2c *)i2c_priv;
	struct mwv207_device *jdev = i2c->jdev;
	u32 val;

	mwv207_i2c_set_dir(i2c, i2c->sda_mask, i2c->sda_dir, 1);

	val = jdev_read(jdev, i2c->sda_in_addr);
	val &= i2c->sda_mask;

	return val ? 1 : 0;
}

static void mwv207_i2c_set_clock(void *i2c_priv, int clock)
{
	struct mwv207_i2c *i2c = (struct mwv207_i2c *)i2c_priv;
	struct mwv207_device *jdev = i2c->jdev;

	mwv207_i2c_set_dir(i2c, i2c->scl_mask, i2c->scl_dir, 0);
	jdev_modify(jdev, i2c->scl_out_addr, i2c->scl_mask, clock ? i2c->scl_mask : 0);
}

static void mwv207_i2c_set_data(void *i2c_priv, int data)
{
	struct mwv207_i2c *i2c = (struct mwv207_i2c *)i2c_priv;

	if (data)
		mwv207_i2c_set_dir(i2c, i2c->sda_mask, i2c->sda_dir, 1);
	else
		mwv207_i2c_set_dir(i2c, i2c->sda_mask, i2c->sda_dir, 0);
}

static int mwv207_i2c_set_addr(struct mwv207_i2c *i2c, int channel)
{
	switch (channel) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		i2c->sda_in_addr = (0x9aa000);
		i2c->scl_in_addr = (0x9aa000);
		i2c->sda_out_addr = (0x9aa004);
		i2c->scl_out_addr = (0x9aa004);
		i2c->scl_mask = (1 << i2c_scl_gpio[channel]);
		i2c->sda_mask = (1 << i2c_sda_gpio[channel]);
		i2c->scl_dir = (0x9aa008);
		i2c->sda_dir = (0x9aa008);
		break;
	case 6:
		i2c->sda_in_addr = (0x9aa000);
		i2c->scl_in_addr = (0x9aa010);
		i2c->sda_out_addr = (0x9aa004);
		i2c->scl_out_addr = (0x9aa014);
		i2c->scl_mask = (1 << i2c_scl_gpio[channel]);
		i2c->sda_mask = (1 << (i2c_sda_gpio[channel] - 32));
		i2c->scl_dir = (0x9aa008);
		i2c->sda_dir = (0x9aa018);
		break;
	case 7:
		i2c->sda_in_addr = (0x9aa010);
		i2c->scl_in_addr = (0x9aa010);
		i2c->sda_out_addr = (0x9aa014);
		i2c->scl_out_addr = (0x9aa014);
		i2c->scl_mask = (1 << (i2c_scl_gpio[channel] - 32));
		i2c->sda_mask = (1 << (i2c_sda_gpio[channel] - 32));
		i2c->scl_dir = (0x9aa018);
		i2c->sda_dir = (0x9aa018);
		break;
	}

	return 0;
}

void mwv207_i2c_put_byte(struct i2c_adapter *i2c,
				 u8 slave_addr,
				 u8 addr,
				 u8 val)
{
	uint8_t out_buf[2];
	struct i2c_msg msg = {
		.addr = slave_addr,
		.flags = 0,
		.len = 2,
		.buf = out_buf,
	};

	out_buf[0] = addr;
	out_buf[1] = val;

	if (i2c_transfer(i2c, &msg, 1) != 1)
		pr_warn("i2c 0x%02x 0x%02x write failed\n",
			  addr, val);
}

void mwv207_i2c_destroy(struct i2c_adapter *adapter)
{
	i2c_del_adapter(adapter);
}

struct i2c_adapter *mwv207_i2c_create(struct mwv207_device *jdev, int i2c_chan)
{
	struct mwv207_i2c *i2c;
	int ret;

	if (i2c_chan < 0 || i2c_chan >= 6)
		return NULL;

	i2c = devm_kzalloc(jdev->dev, sizeof(*i2c), GFP_KERNEL);
	if (!i2c)
		return NULL;

	if (mwv207_i2c_set_addr(i2c, i2c_chan))
		return NULL;

	i2c->adapter.owner = THIS_MODULE;
	i2c->adapter.class = I2C_CLASS_DDC;
	i2c->adapter.dev.parent = jdev->dev;
	i2c_set_adapdata(&i2c->adapter, i2c);
	i2c->mutex = &jdev->gpio_lock;
	i2c->jdev = jdev;

	snprintf(i2c->adapter.name, sizeof(i2c->adapter.name),
			"MWV207_I2C_%d", i2c_chan);
	i2c->adapter.algo_data = &i2c->bit;
	i2c->bit.pre_xfer = mwv207_i2c_pre_xfer;
	i2c->bit.post_xfer = mwv207_i2c_post_xfer;
	i2c->bit.setsda = mwv207_i2c_set_data;
	i2c->bit.setscl = mwv207_i2c_set_clock;
	i2c->bit.getsda = mwv207_i2c_get_data;
	i2c->bit.getscl = mwv207_i2c_get_clock;
	i2c->bit.udelay = 10;
	i2c->bit.timeout = usecs_to_jiffies(2200);
	i2c->bit.data = i2c;

	ret = i2c_bit_add_bus(&i2c->adapter);
	if (ret)
		return NULL;

	return &i2c->adapter;
}

bool mwv207_i2c_probe(struct i2c_adapter *i2c_bus)
{
	u8 out = 0x0;
	u8 buf[8];
	int ret;
	struct i2c_msg msgs[] = {
		{
		 .addr = DDC_ADDR,
		 .flags = 0,
		 .len = 1,
		 .buf = &out,
		  },
		{
		 .addr = DDC_ADDR,
		 .flags = I2C_M_RD,
		 .len = 8,
		 .buf = buf,
		}
	};

	ret = i2c_transfer(i2c_bus, msgs, 2);
	if (ret != 2) {

		return false;
	}

	if (drm_edid_header_is_valid(buf) < 6) {
		return false;
	}
	return true;
}

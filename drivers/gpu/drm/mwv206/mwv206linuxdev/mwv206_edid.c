/*
 * JM7200 GPU driver
 *
 * Copyright (c) 2018 ChangSha JingJiaMicro Electronics Co., Ltd.
 *
 * Author:
 *      rfshen <jjwgpu@jingjiamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/freezer.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/pci.h>
#include <linux/mod_devicetable.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/pci.h>
#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>
#include <drm/drm_edid.h>
#include <drm/drm_crtc.h>
#include <asm/uaccess.h>
#include "mwv206.h"
#include "mwv206kconfig.h"
#include "mwv206reg.h"
#include "mwv206hal/mwv206dev.h"
#include "mwv206hal/mwv206dec.h"



struct device *event_dev;
static struct task_struct *event_task;

static char czc_project_str[VBIOS_PRJ_STR_LEN + 1] = {0x6d, 0x77, 0x75, 0x37, 0x34, 0x30, 0x34, 0};
static int czc_workaround;

#define MAC206LXDEV019    (0x405500 + 0x0)
#define MAC206LXDEV015      (0x405500 + 0x4)
#define MAC206LXDEV020  (0x405500 + 0x8)
#define MAC206LXDEV021   (0x405500 + 0xc)
#define MAC206LXDEV016      (0x405500 + 0x10)
#define MAC206LXDEV018   (0x405500 + 0x14)
#define MAC206LXDEV017   (0x405500 + 0x18)
#define MAC206LXDEV022   (0x405500 + 0x1c)
#define MAC206LXDEV014     (0x405500 + 0x28)
#define MAC206LXDEV013    (0x405920)

#define EDID_CEA_EXT          0x02
#define EDID_HDMI_IEEE_OUI    0x000c03
#define	EDID_VENDOR_BLOCK     0x03




static void gpio_mask(V206DEV025 *pDev, int mask)
{
	switch (mask) {
	case (1 << 26):
		V206DEV003(((0x400000) + 0x16C), 0x0000ff00, 0x00000000);
		V206DEV003(((0x400000) + 0xE64), 0x0000ff00, 0x00003300);
		break;
	case (1 << 29):
		V206DEV003(((0x400000) + 0x16C), 0x0ff00000, 0x00000000);
		V206DEV003(((0x400000) + 0xE64), 0x0ff00000, 0x03300000);
		break;
	case (1 << 0):
		V206DEV003(((0x400000) + 0x170), 0Xff, 0x00000000);
		V206DEV003(((0x400000) + 0xE68), 0Xff, 0x33033033);
		break;
	case (1 << 3):
		V206DEV003(((0x400000) + 0x170), 0x000ff000, 0x00000000);
		V206DEV003(((0x400000) + 0xE68), 0x000ff000, 0x33033033);
		break;
	case (1 << 6):
		V206DEV003(((0x400000) + 0x170), 0xff000000, 0x00000000);
		V206DEV003(((0x400000) + 0xE68), 0xff000000, 0x33033033);
		break;
	case (1 << 8):
		V206DEV003(((0x400000) + 0x174), 0xff, 0x0);
		V206DEV003(((0x400000) + 0xE6C), 0xff, 0x33);
		break;
	default:
		break;
	}
}


static int mwv206_i2c_pre_xfer(struct i2c_adapter *i2c_adap)
{
	struct mwv206_i2c *i2c = i2c_get_adapdata(i2c_adap);
	V206DEV025 *pDev = i2c->pDev;
	u32 val;

	mutex_lock(&i2c->mutex);

	gpio_mask(pDev, i2c->sda_mask);


	val = V206DEV001(i2c->sda_in_addr);
	val &= (~i2c->sda_mask);
	V206DEV002(i2c->sda_in_addr, val);

	val = V206DEV001(i2c->scl_in_addr);
	val &= (~i2c->scl_mask);
	V206DEV002(i2c->scl_in_addr, val);


	val = V206DEV001(i2c->sda_dir);
	val &= (~i2c->sda_mask);
	val |= i2c->sda_mask;
	V206DEV002(i2c->sda_dir, val);

	val = V206DEV001(i2c->scl_dir);
	val &= (~i2c->scl_mask);
	val |= i2c->scl_mask;
	V206DEV002(i2c->scl_dir, val);

	return 0;
}

static void mwv206_i2c_post_xfer(struct i2c_adapter *i2c_adap)
{
	struct mwv206_i2c *i2c = i2c_get_adapdata(i2c_adap);

	mutex_unlock(&i2c->mutex);
}


static int mwv206_get_clock(void *i2c_priv)
{
	struct mwv206_i2c *i2c = (struct mwv206_i2c *)i2c_priv;
	V206DEV025 *pDev = i2c->pDev;
	u32 val;

	val = V206DEV001(i2c->scl_in_addr);
	val &= i2c->scl_mask;

	return (val != 0);
}

static int mwv206_get_data(void *i2c_priv)
{
	struct mwv206_i2c *i2c = (struct mwv206_i2c *)i2c_priv;
	V206DEV025 *pDev = i2c->pDev;
	u32 val;

	val = V206DEV001(i2c->sda_in_addr);
	val &= i2c->sda_mask;

	return (val != 0);
}

static void mwv206_set_clock(void *i2c_priv, int clock)
{
	struct mwv206_i2c *i2c = (struct mwv206_i2c *)i2c_priv;
	V206DEV025 *pDev = i2c->pDev;
	u32 val;


	val = V206DEV001(i2c->scl_dir);
	val &= (~i2c->scl_mask);
	val |= clock ? i2c->scl_mask : 0;
	V206DEV002(i2c->scl_dir, val);
}

static void mwv206_set_data(void *i2c_priv, int data)
{
	struct mwv206_i2c *i2c = (struct mwv206_i2c *)i2c_priv;
	V206DEV025 *pDev = i2c->pDev;
	u32 val;


	val = V206DEV001(i2c->sda_dir);
	val &= (~i2c->sda_mask);
	val |= data ? i2c->sda_mask : 0;
	V206DEV002(i2c->sda_dir, val);
}

static struct mwv206_i2c *mwv206_i2c_create(V206DEV025 *pDev, int index)
{
	struct mwv206_i2c *i2c;
	char name[20];
	int ret;

	i2c = kzalloc(sizeof(struct mwv206_i2c), GFP_KERNEL);
	if (!i2c) {
		return NULL;
	}

	memset(name, 0, sizeof(name));

	switch (index) {
	case 0:
	case 1:
		i2c->sda_in_addr = 0x405400;
		i2c->scl_in_addr = 0x405400;
		i2c->scl_mask = (1 << (26 + 3 * index));
		i2c->sda_mask = (1 << (27 + 3 * index));
		i2c->scl_dir = 0x405408;
		i2c->sda_dir = 0x405408;
		sprintf(name, "MWV206_I2C_%d", index + 2);
		break;
	case 2:
	case 3:
		i2c->sda_in_addr = 0x405410;
		i2c->scl_in_addr = 0x405410;
		i2c->scl_mask = (1 << (0 + 3 * (index - 2)));
		i2c->sda_mask = (1 << (1 + 3 * (index - 2)));
		i2c->scl_dir = 0x405418;
		i2c->sda_dir = 0x405418;
		sprintf(name, "MWV206_I2C_%d", index + 2);
		break;

	case 4:
	case 5:
		i2c->sda_in_addr = 0x405410;
		i2c->scl_in_addr = 0x405410;
		i2c->scl_mask = (1 << (6 + 2 * (index - 4)));
		i2c->sda_mask = (1 << (7 + 2 * (index - 4)));
		i2c->scl_dir = 0x405418;
		i2c->sda_dir = 0x405418;
		sprintf(name, "MWV206_I2C_%d", 2 + index);
		break;
	}

	i2c->adapter.owner = THIS_MODULE;
	i2c->adapter.class = I2C_CLASS_DDC;
	i2c->adapter.dev.parent = &pDev->V206DEV103->dev;
	i2c->pDev = pDev;
	i2c_set_adapdata(&i2c->adapter, i2c);

	mutex_init(&i2c->mutex);

	snprintf(i2c->adapter.name, sizeof(i2c->adapter.name),
		 "MWV206_%s", name);

	i2c->adapter.algo_data = &i2c->bit;

	i2c->bit.udelay = 10;
	i2c->bit.timeout = usecs_to_jiffies(2200);
	i2c->bit.data = i2c;
	i2c->bit.pre_xfer = mwv206_i2c_pre_xfer;
	i2c->bit.post_xfer = mwv206_i2c_post_xfer;
	i2c->bit.setsda = mwv206_set_data;
	i2c->bit.setscl = mwv206_set_clock;
	i2c->bit.getsda = mwv206_get_data;
	i2c->bit.getscl = mwv206_get_clock;
	ret = i2c_bit_add_bus(&i2c->adapter);

	if (ret) {
		V206KDEBUG002("MWV206: failed to register bit i2c\n");
		goto out_free;
	}

	return i2c;
out_free:
	kfree(i2c);
	return NULL;
}

static void mwv206_i2c_destroy(struct mwv206_i2c *i2c)
{
	if (!i2c) {
		return;
	}

	i2c_del_adapter(&i2c->adapter);

	kfree(i2c);
}


int FUNC206LXDEV151(V206DEV025 *pDev, int arg)
{
	if (arg) {
		return FUNC206LXDEV116(pDev->V206DEV150);
	} else {
		return FUNC206LXDEV136(pDev->V206DEV150);
	}
}

static int FUNC206LXDEV051(V206DEV025 *pDev, unsigned char slave, unsigned char reg,
	unsigned char *buff, unsigned int len)
{
	unsigned char data;
	unsigned int i;
	int timeout;
	if (len > 32) {
		len = 32;
	}

	V206DEV002(MAC206LXDEV019, 0);
	udelay(200);
	V206DEV002(MAC206LXDEV019, 1);
	V206DEV002(MAC206LXDEV022, reg);
	V206DEV002(MAC206LXDEV018, 1);
	V206DEV002(MAC206LXDEV017, 0);
	V206DEV002(MAC206LXDEV015, 0x02000001 | ((slave & 0xfe) << 16));
	timeout = 1000;
	while (V206DEV001(MAC206LXDEV018) == 0 && timeout--) {
		udelay(1);
	}

	if (V206DEV001(MAC206LXDEV017)) {
		return -2;
	}

	V206DEV002(MAC206LXDEV018, 1);
	V206DEV002(MAC206LXDEV017, 0);
	V206DEV002(MAC206LXDEV015, 0x03010000 | ((slave & 0xfe) << 16) | len);
	timeout = 1000;
	while (V206DEV001(MAC206LXDEV018) == 0 && timeout--) {
		udelay(len);
	}
	if (V206DEV001(MAC206LXDEV017)) {
		return -4;
	}
	timeout = 1000;
	while (V206DEV001(MAC206LXDEV020) != len &&  timeout--) {
		udelay(len);
	}
	if (V206DEV001(MAC206LXDEV020) != len) {
		return -5;
	}
	for (i = 0; i < len; i++) {
		data = V206DEV001(MAC206LXDEV021);
		if (buff) {
			buff[i] = data;
		}
	}

	return 0;
}

#define DDC_SEGMENT_ADDR 0x30
static int mwv206_ddc_read_gpio(struct i2c_adapter *adapter,
		unsigned char *buf,  unsigned int block, int len)
{
	unsigned char start = block * EDID_LENGTH;
	unsigned char segment = block >> 1;
	unsigned char xfers = segment ? 3 : 2;
	int ret, retries = 10;


	do {
		struct i2c_msg msgs[] = {
			{
				.addr   = DDC_SEGMENT_ADDR,
				.flags  = 0,
				.len    = 1,
				.buf    = &segment,
			}, {
				.addr   = DDC_ADDR,
				.flags  = 0,
				.len    = 1,
				.buf    = &start,
			}, {
				.addr   = DDC_ADDR,
				.flags  = I2C_M_RD,
				.len    = len,
				.buf    = buf,
			}
		};


		ret = i2c_transfer(adapter, &msgs[3 - xfers], xfers);

		if (ret == -ENXIO) {
			break;
		}
	} while (ret != xfers && --retries);

	return ret == xfers ? 0 : -1;
}

static int mwv206_edid_read_gpio(V206DEV025 *pDev,
		int index, unsigned char *edid, int *len)
{
	struct i2c_adapter *i2c_adapter;
	int r, i;

	if (!pDev->edid_i2c[index]) {
		return -ENODEV;
	}
	i2c_adapter = &pDev->edid_i2c[index]->adapter;

	*len = 0;
	r = mwv206_ddc_read_gpio(i2c_adapter, edid, 0, EDID_LENGTH);
	if ((r) || !drm_edid_block_valid(edid, 0, false, NULL)) {
		return -1;
	}
	*len = EDID_LENGTH;


	if (edid[0x7e] && (edid[0x7e] < V206CONFIG010 / EDID_LENGTH)) {
		for (i = 1; i <= edid[0x7e]; i++) {
			r = mwv206_ddc_read_gpio(i2c_adapter, edid + (*len), i, *len);
			if (r) {
				return -1;
			}
			*len += EDID_LENGTH;
		}
	} else if (edid[0x7e] >= V206CONFIG010 / 128) {
		return -1;
	}

	return 0;
}

int FUNC206LXDEV152(V206DEV025 *pDev, int i2c_chan, unsigned char  *ddc_data, int *out_len)
{
	int ret = 0;
	int i = 0;
	int len = 0;

	if (pDev == NULL) {
		return -1;
	}

	if ((NULL == ddc_data) || (NULL == out_len)) {
		V206KDEBUG002("[ERROR] ddc read null\n");
		return -1;
	}

	if (MWV206_I2C_CHAN_DISABLED(pDev, i2c_chan)) {
		return -1;
	}

	*out_len = 0;
	if (MWV206_I2C_CHAN_IPCORE(pDev, i2c_chan)) {
		V206DEV002(MAC206LXDEV013, i2c_chan);
		for (i = 0; i < EDID_LENGTH; i += 32) {
			ret = FUNC206LXDEV051(pDev, 0xa0, i, ddc_data + i, 32);
			if (ret) {
				return -1;
			}
		}

		len = EDID_LENGTH;
		if ((ddc_data[0x7e] < V206CONFIG010 / 128) && ddc_data[0x7e]) {
			for (i = EDID_LENGTH; i < (ddc_data[0x7e] + 1) * EDID_LENGTH; i += 32) {
				ret = FUNC206LXDEV051(pDev, 0xa0, i, ddc_data + i, 32);
				if (ret) {
					return -1;
				}
			}
			len += ddc_data[0x7e] * EDID_LENGTH;
		}
		if (ddc_data[0x7e] >= V206CONFIG010 / 128) {

			return -1;
		}

		*out_len = len;
	} else {
		ret = mwv206_edid_read_gpio(pDev, i2c_chan - 2, ddc_data, &len);
		if (ret) {
			return -1;
		}
		*out_len = len;
	}
	return 0;
}

bool FUNC206LXDEV150(struct edid *edid)
{
	char edid_valid[V206CONFIG010];
	u8   valid_extensions = 0;
	u8   *raw = (u8 *)edid;
	int  i;

	if (!edid) {
		return false;
	}

	if (!drm_edid_block_valid(raw, 0, false, NULL)) {
		return false;
	}

	if (edid->extensions == 0) {
		return true;
	}

	memset(edid_valid, 0, V206CONFIG010);
	memcpy(edid_valid, raw, EDID_LENGTH);
	for (i = 1; i <= edid->extensions; i++) {
		if (drm_edid_block_valid(raw + i * EDID_LENGTH, i, false, NULL)) {
			memcpy(&edid_valid[(valid_extensions + 1) * EDID_LENGTH], raw + i * EDID_LENGTH, EDID_LENGTH);
			valid_extensions++;
			continue;
		}
	}

	if (valid_extensions != edid->extensions) {
		edid_valid[EDID_LENGTH - 1] += edid_valid[0x7e] - valid_extensions;
		edid_valid[0x7e] = valid_extensions;
		memset(raw, 0, V206CONFIG010);
		memcpy(raw, edid_valid, (valid_extensions + 1) * EDID_LENGTH);
	}

	return true;
}


char czc_bad_edid[256] = {
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x09, 0xe5, 0x3b, 0x2a, 0x01, 0x01, 0x01, 0x01,
	0x17, 0x15, 0x01, 0x03, 0x80, 0x23, 0x12, 0x78, 0xea, 0x1e, 0xc5, 0xae, 0x4f, 0x34, 0xb1, 0x26,
	0x0e, 0x50, 0x54, 0xa5, 0x4b, 0x00, 0x81, 0x80, 0xa9, 0x40, 0xd1, 0xc0, 0x71, 0x4f, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c,
	0x45, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0xff, 0x00, 0x4a, 0x32, 0x35,
	0x37, 0x4d, 0x39, 0x36, 0x42, 0x30, 0x30, 0x46, 0x4c, 0x0a, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x50,
	0x61, 0x6e, 0x65, 0x6c, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfd,
	0x00, 0x38, 0x4c, 0x1e, 0x51, 0x11, 0x00, 0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x7e,
	0x02, 0x03, 0x19, 0xb1, 0x48, 0x90, 0x05, 0x04, 0x03, 0x02, 0x07, 0x00, 0x01, 0x00, 0x03, 0x0c,
	0x00, 0x10, 0x00, 0x38, 0x2d, 0xe3, 0x05, 0x03, 0x01, 0x02, 0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d,
	0x40, 0x58, 0x2c, 0x45, 0x00, 0x06, 0x44, 0x21, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x97
};
char czc_good_edid[256] = {
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x09, 0xE5, 0x3B, 0x2A, 0x01, 0x01, 0x01, 0x01,
	0x17, 0x15, 0x01, 0x03, 0x80, 0x23, 0x12, 0x78, 0xEA, 0x1E, 0xC5, 0xAE, 0x4F, 0x34, 0xB1, 0x26,
	0x0E, 0x50, 0x54, 0xA5, 0x4B, 0x00, 0x81, 0x80, 0xA9, 0x40, 0xD1, 0xC0, 0x71, 0x4F, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,
	0x45, 0x00, 0x55, 0x50, 0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x4A, 0x32, 0x35,
	0x37, 0x4D, 0x39, 0x36, 0x42, 0x30, 0x30, 0x46, 0x4C, 0x0A, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x50,
	0x61, 0x6E, 0x65, 0x6C, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFD,
	0x00, 0x38, 0x4C, 0x1E, 0x51, 0x11, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x7E,
	0x02, 0x03, 0x2C, 0xB1, 0x48, 0x8F, 0x04, 0x03, 0x02, 0x01, 0x06, 0x00, 0x00, 0x38, 0x2D, 0x63,
	0x05, 0x03, 0x01, 0x00, 0x3A, 0x00, 0x18, 0x71, 0x38, 0x05, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x06,
	0x44, 0x21, 0x00, 0x00, 0x1E, 0x00, 0x65, 0x03, 0x0C, 0x00, 0x00, 0x00, 0x02, 0x3A, 0x80, 0x18,
	0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x06, 0x44, 0x21, 0x00, 0x00, 0x1E, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x24
};


int FUNC206LXDEV147(V206DEV025 *pDev)
{
	int i, j, ret, size, changed = 0;
	int disconnect = 0;
	char *edid = pDev->V206DEV148;
	char *check = pDev->V206DEV149;

	for (i = 0; i < 8; i++) {
		if (MWV206_I2C_CHAN_DISABLED(pDev, i)) {
			disconnect++;
			continue;
		}
		ret = FUNC206LXDEV152(pDev, i, edid, &size);
		pDev->V206DEV147.changed[i] = 0;
		if (!ret) {
			if (czc_workaround == 1) {
				if (i == 2 && memcmp(czc_bad_edid, edid, sizeof(czc_bad_edid)) == 0) {
					size = sizeof(czc_good_edid);
					memcpy(edid, czc_good_edid, size);
				}
			}

			if (memcmp(pDev->V206DEV147.edid[i], edid, size)) {

				memcpy(check, edid, size);
				if (FUNC206LXDEV150((struct edid *)check)) {
					memcpy(pDev->V206DEV147.edid[i], check, (check[0x7e] + 1) * EDID_LENGTH);
					pDev->V206DEV147.changed[i] = 1;
					pDev->V206DEV147.connect_status[i] = 1;
					changed = 1;
				}
			}
		} else {

			if (pDev->V206DEV147.connect_status[i] == 1) {
				for (j = 0; j < 50; j++) {
					ret = FUNC206LXDEV152(pDev, i, edid, &size);
					if (!ret) {
						break;
					}
					msleep(1);
				}
				if (ret) {
					changed = 1;
					pDev->V206DEV147.connect_status[i] = 0;
					pDev->V206DEV147.changed[i] = 1;
					disconnect++;
					memset(pDev->V206DEV147.edid[i], 0, V206CONFIG010);
				}
			} else {
				disconnect++;
			}
		}
	}
	pDev->V206DEV147.isfake = (disconnect == 8);
	return changed;
}

void mwv206_edid_custom_detect(V206DEV025 *pDev)
{
	FUNC206LXDEV151(pDev, 1);
	FUNC206LXDEV147(pDev);
	FUNC206LXDEV151(pDev, 0);
}

int FUNC206LXDEV149(V206DEV025 *pDev, unsigned long userdata)
{
	int ret;
	FUNC206LXDEV151(pDev, 1);
	ret = copy_to_user((void __user *)userdata, &pDev->V206DEV147, sizeof(EDID));
	FUNC206LXDEV151(pDev, 0);
	if (ret != 0) {
		V206KDEBUG002("[ERROR] MWV206 get edid failed, ret = %d.", ret);
		return ret;
	} else {
		return 0;
	}
}

static int cea_db_tag(const u8 *db)
{
	return db[0] >> 5;
}

static int cea_db_payload_len(const u8 *db)
{
	return db[0] & 0x1f;
}
static int cea_revision(const u8 *cea)
{
	if (!cea) {
		return 0;
	} else {
		return cea[1];
	}
}
static int cea_db_offsets(const u8 *cea, int *start, int *end)
{

	*start = 4;
	*end   = cea[2];

	if (*end == 0) {
		*end = 127;
	}

	if (*end < 4 || *end > 127) {
		return -1;
	}

	return 0;
}
static bool cea_db_is_hdmi_vsdb(const u8 *db)
{
	int hdmi_id;

	if (cea_db_tag(db) != EDID_VENDOR_BLOCK) {
		return false;
	}

	if (cea_db_payload_len(db) < 5) {
		return false;
	}

	hdmi_id = db[1] | (db[2] << 8) | (db[3] << 16);

	return hdmi_id == EDID_HDMI_IEEE_OUI;
}

static u8 *mwv206_find_edid_extension(unsigned char *edid, int ext_id)
{
	u8 *edid_ext = NULL;
	int i;
	if (0 == FUNC206LXDEV150((struct edid *)edid) || edid[126] == 0) {
		return NULL;
	}
	for (i = 0; i < edid[126]; i++) {
		edid_ext = (u8 *)edid + EDID_LENGTH * (i + 1);
		if (edid_ext[0] == ext_id) {
			break;
		}
	}

	return edid_ext;
}

#define for_each_cea_db(cea, i, start, end) \
			for ((i) = (start); (i) < (end) && (i) + cea_db_payload_len(&(cea)[(i)]) < (end); (i) += cea_db_payload_len(&(cea)[(i)]) + 1)

int mwv206_edid_is_dvi(unsigned char *edid)
{
	int isdvi = 1;

	if (edid[126] >= 1) {
		const u8 *cea = mwv206_find_edid_extension(edid, EDID_CEA_EXT);
		const u8 *db, *hdmi = NULL;
		if (cea_revision(cea) >= 3) {
			int i, start, end;
			if (cea_db_offsets(cea, &start, &end)) {
				return 0;
			}
			for_each_cea_db(cea, i, start, end) {
				db = &cea[i];
				if (cea_db_is_hdmi_vsdb(db)) {
					hdmi = db;
				}
			}
			isdvi = !hdmi;
		}
	}

	return isdvi;
}

int FUNC206LXDEV148(void *pData)
{
	V206DEV025 *pDev = (V206DEV025 *)pData;
	char *event_string = "HOTPLUG=1";
	char *envp[] = { event_string, NULL };
	int edid_changed;

	set_freezable();
	while (!kthread_should_stop()) {
		try_to_freeze();
		FUNC206LXDEV151(pDev, 1);
		edid_changed = FUNC206LXDEV147(pDev);
		FUNC206LXDEV151(pDev, 0);
		if (edid_changed && pDev->pm.V206DEV109 == 0) {
			kobject_uevent_env(&event_dev->kobj, KOBJ_CHANGE, envp);
		}
		msleep_interruptible(2000);
	}
	return 0;
}

void FUNC206LXDEV052(V206DEV025 *pDev)
{
	int err, i;

	if (memcmp(pDev->vbios_prj_str, czc_project_str, VBIOS_PRJ_STR_LEN) == 0) {
		czc_workaround = 1;
	} else {
		czc_workaround = 0;
	}

	for (i = 0; i < 8; i++) {
		if (MWV206_I2C_CHAN_DISABLED(pDev, i)) {
			V206KDEBUG003("[INFO]: skip mwv206 i2c channel %d, because it's disabled\n", i);
			continue;
		}

		if (MWV206_I2C_CHAN_IPCORE(pDev, i)) {
			continue;
		}


		if (i < 2) {
			continue;
		}
		pDev->edid_i2c[i - 2] = mwv206_i2c_create(pDev, i - 2);
	}

	event_task = kthread_create(FUNC206LXDEV148, pDev, "kedidreader");
	if (IS_ERR(event_task)) {
		V206KDEBUG002("[ERROR] Unable to start kernel thread.\n");
		err = PTR_ERR(event_task);
		event_task = NULL;
		return ;
	}
	wake_up_process(event_task);
}

void FUNC206LXDEV053(struct pci_dev *V206DEV103)
{
	V206DEV025 *priv = pci_get_drvdata(V206DEV103);
	int i;

	if (event_task) {
		kthread_stop(event_task);
		event_task = NULL;
	}

	for (i = 2; i < 8; i++) {
		if (!MWV206_I2C_CHAN_DISABLED(priv, i)
				&& !MWV206_I2C_CHAN_IPCORE(priv, i)) {
			mwv206_i2c_destroy(priv->edid_i2c[i - 2]);
		}
	}
}
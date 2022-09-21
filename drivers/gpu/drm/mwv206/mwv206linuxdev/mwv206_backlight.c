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
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/stat.h>
#include <linux/backlight.h>
#include <linux/delay.h>
#include "gljos.h"
#include "jmgpio.h"
#include "jmpwm.h"
#include "mwv206reg.h"
#include "mwv206dev.h"



#define MAC206LXDEV008         25
#define MAC206LXDEV009                     20
#define MAC206LXDEV003               1
#define MAC206LXDEV004                0
#define MAC206LXDEV006           100

#define MAC206LXDEV002               1
#define MAC206LXDEV007                1
#define MAC206LXDEV005                     (1 << MAC206LXDEV007)

typedef struct FUNC206LXDEV095 {
	int     brightness;
	int     old_brightness;
	int     blctr;
	int     bl_maxlevel;
	int     bl_gpio_group;
	int     bl_mask;
	int     bl_step;
	int     bl_minhighleveltime;
	V206DEV025    *priv;
} mwv206_backlight_t;
static mwv206_backlight_t *FUNC206LXDEV001;
static struct backlight_device *cdv_backlight_device;

static unsigned int FUNC206LXDEV041(void)
{
	return FUNC206LXDEV001->brightness;
}

static int FUNC206LXDEV042(unsigned int val)
{
	FUNC206LXDEV001->brightness = val;

	return 0;
}

static unsigned int FUNC206LXDEV058(void)
{
	return FUNC206LXDEV001->old_brightness;
}

static int FUNC206LXDEV059(unsigned int val)
{
	FUNC206LXDEV001->old_brightness = val;

	return 0;
}


static int FUNC206LXDEV002(V206DEV025 *V206DEV103, int val)
{
	FUNC206LXDEV042(val);
#if 0
	FUNC206HAL164 (V206DEV103, MAC206LXDEV005);
	FUNC206HAL163 (V206DEV103, MAC206LXDEV002,  MAC206LXDEV009 * val, 2000, 0x0);
	FUNC206HAL165 (V206DEV103, 1, MAC206LXDEV005);
#endif
#if 1
	FUNC206HAL164 (V206DEV103, FUNC206LXDEV001->bl_mask);
	FUNC206HAL163 (V206DEV103, FUNC206LXDEV001->bl_gpio_group,  FUNC206LXDEV001->bl_step * val + FUNC206LXDEV001->bl_minhighleveltime, 2000, 0x0);
	FUNC206HAL165 (V206DEV103, 1, FUNC206LXDEV001->bl_mask);
#endif
	return 0;
}


static int FUNC206LXDEV043(struct backlight_device *bd)
{
	return FUNC206LXDEV041();
}

static int FUNC206LXDEV044(struct backlight_device *bd)
{
	mwv206_backlight_t *pbl_data = bl_get_data(bd);
	V206DEV025 *V206DEV103 = pbl_data->priv;
	int level = bd->props.brightness;


	if (level < 0) {
		return -EINVAL;
	}

	level *= FUNC206LXDEV001->bl_maxlevel;
	level /= FUNC206LXDEV001->bl_maxlevel;

	FUNC206LXDEV002(V206DEV103, level);
	FUNC206LXDEV059(level);

	return 0;
}

static const struct backlight_ops cdv_ops = {
	.get_brightness = FUNC206LXDEV043,
	.update_status  = FUNC206LXDEV044,
};

void FUNC206LXDEV040(V206DEV025 *pDev)
{
	int address, bit;
	address = (int)(FUNC206LXDEV001->blctr / 8) * 4;
	bit = (FUNC206LXDEV001->blctr % 8) * 4;

	V206DEV003(((0x400000) + 0x160) + address, 0xf << bit, 0x0);
	FUNC206HAL142 (pDev, FUNC206LXDEV001->blctr, 0);

	address = (int)(FUNC206LXDEV001->priv->V206DEV106.bl_pwmgpio / 8) * 4;
	bit = (FUNC206LXDEV001->priv->V206DEV106.bl_pwmgpio % 8) * 4;

	V206DEV003(((0x400000) + 0x160) + address, 0xf << bit, 0x0);

}

int FUNC206LXDEV144 (struct pci_dev *V206DEV103)
{
	int bl_pwm_gpio;
	int bl_stepget;
	int bl_mindutyratio;
	struct backlight_properties props;
	FUNC206LXDEV001 = kzalloc(sizeof(mwv206_backlight_t), GFP_KERNEL);
	if (!FUNC206LXDEV001) {
		return -EFAULT;
	}

	FUNC206LXDEV001->priv = pci_get_drvdata(V206DEV103);

	FUNC206LXDEV001->blctr = FUNC206LXDEV001->priv->V206DEV105.chip.blctrl;

	if (FUNC206LXDEV001->blctr == 0) {
		V206KDEBUG003("[INFO] %s: gpio = 0.\n", __FUNCTION__);
		return 0;
	}

	bl_mindutyratio = FUNC206LXDEV001->priv->V206DEV106.bl_mindutyratio;
	FUNC206LXDEV001->bl_minhighleveltime = bl_mindutyratio * 20;

#if 1
	FUNC206LXDEV001->bl_maxlevel = FUNC206LXDEV001->priv->V206DEV106.bl_maxlevel;
	bl_pwm_gpio = FUNC206LXDEV001->priv->V206DEV106.bl_pwmgpio;
	if (bl_pwm_gpio >= 22 && bl_pwm_gpio <= 25) {
		FUNC206LXDEV001->bl_gpio_group = 1;
		FUNC206LXDEV001->bl_mask = 1 << (bl_pwm_gpio - 22);
	} else if (bl_pwm_gpio >= 34 && bl_pwm_gpio <= 37) {
		FUNC206LXDEV001->bl_gpio_group = 0;
		FUNC206LXDEV001->bl_mask = 1 << (bl_pwm_gpio - 34);
	} else {
		bl_pwm_gpio = 23;
		FUNC206LXDEV001->bl_gpio_group = 1;
		FUNC206LXDEV001->bl_mask = 1 << (bl_pwm_gpio - 22);
	}


	bl_stepget = FUNC206LXDEV001->priv->V206DEV106.bl_maxdutyratio;
	bl_stepget -= FUNC206LXDEV001->priv->V206DEV106.bl_mindutyratio;
	bl_stepget *= 20;
	bl_stepget /= FUNC206LXDEV001->bl_maxlevel;
	FUNC206LXDEV001->bl_step = bl_stepget;
#endif

	memset(&props, 0, sizeof(struct backlight_properties));

	props.max_brightness = FUNC206LXDEV001->bl_maxlevel;
	props.type = BACKLIGHT_PLATFORM;

	cdv_backlight_device = backlight_device_register("mwv206",
			NULL, (void *)FUNC206LXDEV001, &cdv_ops, &props);
	if (IS_ERR(cdv_backlight_device)) {
		V206KDEBUG003("[ERROR] %s failure.\n", __FUNCTION__);
		return PTR_ERR(cdv_backlight_device);
	}

	FUNC206HAL142 ((V206DEV025 *)FUNC206LXDEV001->priv, FUNC206LXDEV001->blctr, 0);

	FUNC206LXDEV040((V206DEV025 *)FUNC206LXDEV001->priv);




	FUNC206LXDEV002(FUNC206LXDEV001->priv, FUNC206LXDEV001->bl_maxlevel);
	FUNC206LXDEV059(MAC206LXDEV006);

	V206KDEBUG003("[INFO] %s done.\n", __FUNCTION__);

	return 0;
}

void FUNC206LXDEV038(void)
{
	if (!IS_ERR(cdv_backlight_device)) {
		backlight_device_unregister(cdv_backlight_device);
		kfree(FUNC206LXDEV001);
	}
}

int FUNC206LXDEV143(V206DEV025 *pDev, long arg)
{
	int mode = (int)arg;
	if ((mode < 0) || (mode > 1)) {
		return -EINVAL;
	}

	if (FUNC206LXDEV001 == NULL) {
		V206KDEBUG003("[ERROR] mwv206 backlight is not registered.\n");
		return -EINVAL;
	}

	if (FUNC206LXDEV001->blctr == 0) {
		return 0;
	}


	if (mode == 0) {
		FUNC206LXDEV002(pDev, 0);
		FUNC206HAL143(FUNC206LXDEV001->priv, FUNC206LXDEV001->blctr, MAC206LXDEV004);
	} else {
		msleep(1000);
		FUNC206LXDEV002(pDev, FUNC206LXDEV058());
		FUNC206HAL143(FUNC206LXDEV001->priv, FUNC206LXDEV001->blctr, MAC206LXDEV003);
	}

	return 0;
}

void FUNC206LXDEV039(V206DEV025 *pDev)
{
	if (FUNC206LXDEV001->blctr == 0) {
		return;
	}
	FUNC206LXDEV040(pDev);

	FUNC206LXDEV059(MAC206LXDEV006);
}
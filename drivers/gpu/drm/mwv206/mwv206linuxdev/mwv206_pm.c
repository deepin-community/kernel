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
#include <linux/moduleparam.h>
#include <linux/pci.h>
#include <linux/mod_devicetable.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/syscalls.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/jiffies.h>
#include <linux/vmalloc.h>
#include <linux/fb.h>
#include <linux/console.h>
#include "mwv206.h"
#include "mwv206kconfig.h"
#include "mwv206reg.h"
#include "mwv206dev.h"
#include "mwv206dec.h"
#include "videoadd_reg.h"

#define MAC206HAL156(...)
static int FUNC206LXDEV091(struct pci_dev *dev)
{
	V206DEV025 *pDev;

	pDev = pci_get_drvdata(dev);

	memcpy(pDev->pm.V206DEV121, pDev->V206DEV121, sizeof(pDev->pm.V206DEV121));
	return 0;
}

static int FUNC206LXDEV090(struct pci_dev *dev)
{
	V206DEV025 *pDev;
	unsigned int membar2d, membar3d, V206DEV043;

	pDev = pci_get_drvdata(dev);
	V206DEV043 = pDev->V206DEV043;
	membar2d = pDev->V206DEV044[0];
	membar3d = pDev->V206DEV044[1];

	FUNC206HAL236(pDev, V206DEV043, pDev->pm.V206DEV121[V206DEV043]);
	FUNC206HAL236(pDev, membar2d, pDev->pm.V206DEV121[membar2d]);
	FUNC206HAL236(pDev, membar3d, pDev->pm.V206DEV121[membar3d]);
	return 0;
}

int FUNC206LXDEV165(V206DEV025 *dev)
{
	int value;
	V206DEV025 *pDev = dev;
	int corefreq, memfreq;


	V206DEV002(((0x406000) + 0x0014), 0);
	V206DEV002(((0x406000) + 0x0018), 0);
	V206DEV002(((0x406000) + 0x001c), 0);
	V206DEV002(((0x406000) + 0x0020), 0);


	value = V206DEV001(0x00400048);
	value = (value >> 16);
	dev->V206DEV035 = value;


	dev->V206DEV050 = 50;

	corefreq = pDev->V206DEV073;
	memfreq = pDev->V206DEV075;

	MAC206HAL156("[INFO] corefreq %dMhz, memfreq %dMhz\n", corefreq, memfreq);

	FUNC206HAL379(dev);

	FUNC206HAL274(pDev, pDev->V206DEV096.mode, pDev->V206DEV097.mode);

	FUNC206HAL135(dev, V206IOCTL005, corefreq, 1);

	FUNC206HAL315(dev, memfreq);

	FUNC206HAL134(pDev, V206IOCTL005, &corefreq);
	FUNC206HAL134(pDev, V206IOCTL010, &memfreq);
	V206KDEBUG003("[INFO] corefreq %dMhz, memfreq %dMhz.\n", corefreq, memfreq);

	FUNC206HAL330(dev);

	FUNC206HAL351(dev);

	FUNC206HAL385(pDev);
	return 0;
}

static int FUNC206LXDEV087(V206DEV025 *pDev)
{
	int i, cnt;
	unsigned int reg;


	MAC206HAL156("[INFO] %s.\n", __FUNCTION__);

	if (!FUNC206HAL257(pDev)) {
		return 0;
	}

	reg = 0x2000; cnt = 16;
	pDev->pm.V206DEV134.V206DEV126 = MWV206SETREGCMD(reg, cnt);
	reg += 0x200000;
	for (i = 0; i < cnt; i++) {
		pDev->pm.V206DEV134.V206DEV127[i] = V206DEV001(reg);
		reg += 4;
	}


	reg = 0x7000; cnt = 639;
	pDev->pm.V206DEV134.V206DEV128 = MWV206SETREGCMD(reg, cnt);
	reg += 0x200000;
	for (i = 0; i < cnt; i++) {
		pDev->pm.V206DEV134.V206DEV129[i] = V206DEV001(reg);
		reg += 4;
	}

	reg = 0x1000; cnt = 20;
	pDev->pm.V206DEV134.V206DEV130 = MWV206SETREGCMD(reg, cnt);
	reg += 0x200000;
	for (i = 0; i < cnt; i++) {
		pDev->pm.V206DEV134.V206DEV131[i] = V206DEV001(reg);
		reg += 4;
	}

	reg = 0x1100; cnt = 64;
	pDev->pm.V206DEV134.V206DEV132 = MWV206SETREGCMD(reg, cnt);
	reg += 0x200000;
	for (i = 0; i < cnt; i++) {
		pDev->pm.V206DEV134.V206DEV133[i] = V206DEV001(reg);
		reg += 4;
	}

	return 0;
}

static int FUNC206LXDEV086(V206DEV025 *pDev)
{
	unsigned int cmds[16], len = 0;

	MAC206HAL156("[INFO] %s.\n", __FUNCTION__);
	FUNC206HAL255(pDev);
	if (!FUNC206HAL257(pDev)) {
		return 0;
	}

	pDev->V206DEV088 = 0;

	cmds[len++] = 0x40007150;
	cmds[len++] = 1;
	cmds[len++] = 0x85000000;
	cmds[len++] = 0x81000000;
	cmds[len++] = 0x81010000;
	cmds[len++] = 0x84000000;

	FUNC206HAL242(pDev, (unsigned char *)cmds, len * 4);
	FUNC206HAL242(pDev, (unsigned char *)&(pDev->pm.V206DEV134.V206DEV126), 17 * 4);

	FUNC206HAL242(pDev, (unsigned char *)&(pDev->pm.V206DEV134.V206DEV128), 640 * 4);
	FUNC206HAL242(pDev, (unsigned char *)&(pDev->pm.V206DEV134.V206DEV130), 21 * 4);
	FUNC206HAL242(pDev, (unsigned char *)&(pDev->pm.V206DEV134.V206DEV132), 65 * 4);
	V206DEV005("[%s: %d].\n", __FUNCTION__, __LINE__);
	return 0;
}

static void mwv206pm_cursor_save(V206DEV025 *pDev)
{
	unsigned int V206DEV033;
	int i;
	for (i = 0; i < 4; i++) {
		V206DEV033 = 0x94B4 + OUTPUT_REG_OFFSET(i);
		pDev->pm.cursor[i].frame_addr = V206DEV001(V206DEV033);
		V206DEV033 = 0x9454 + OUTPUT_REG_OFFSET(i);
		pDev->pm.cursor[i].bmp_val = V206DEV001(V206DEV033);
		V206DEV033 = 0x94A8 + OUTPUT_REG_OFFSET(i);
		pDev->pm.cursor[i].pos_val = V206DEV001(V206DEV033);
		V206DEV033 = 0x9478 + OUTPUT_REG_OFFSET(i);
		pDev->pm.cursor[i].enable = V206DEV001(V206DEV033);
	}
}

static void mwv206pm_cursor_reload(V206DEV025 *pDev)
{
	unsigned int V206DEV033;
	int i;
	for (i = 0; i < 4; i++) {
		V206DEV033 = 0x94B4 + OUTPUT_REG_OFFSET(i);
		V206DEV002(V206DEV033, pDev->pm.cursor[i].frame_addr);
		V206DEV033 = 0x9454 + OUTPUT_REG_OFFSET(i);
		V206DEV002(V206DEV033, pDev->pm.cursor[i].bmp_val);
		V206DEV033 = 0x94A8 + OUTPUT_REG_OFFSET(i);
		V206DEV002(V206DEV033, pDev->pm.cursor[i].pos_val);
		V206DEV033 = 0x9478 + OUTPUT_REG_OFFSET(i);
		V206DEV002(V206DEV033, pDev->pm.cursor[i].enable);
	}
}

static void mwv206pm_palette_reload(V206DEV025 *pDev)
{
	int i;
	for (i = 0; i < 4; i++) {
		if (pDev->pm.palette_valid[i]) {
			FUNC206HAL392(pDev, (long)&pDev->pm.palettes[i]);
		}
	}
}

static int FUNC206LXDEV089(V206DEV025 *pDev)
{
	unsigned int reg;
	int cnt = 0;

	for (reg = V206DECH023; reg <= V206DECH023 + 0x1D8; reg += 4) {
		pDev->pm.V206DEV135[cnt] = V206DEV001(reg);
		cnt++;
	}

	MAC206HAL156("[INFO] %s: save decoder reg, cnt = %d.\n", __FUNCTION__, cnt);
	return 0;
}

static int FUNC206LXDEV088(V206DEV025 *pDev)
{
	unsigned int reg;
	int cnt = 0;

	mwv206dec_reset(pDev->V206DEV138, pDev);

	for (reg = V206DECH023; reg <= V206DECH023 + 0x1D8; reg += 4) {
		V206DEV002(reg, pDev->pm.V206DEV135[cnt]);
		cnt++;
	}

	MAC206HAL156("[INFO] %s: save decoder reg, cnt = %d.\n", __FUNCTION__, reg);
	return 0;
}

int FUNC206LXDEV162(V206DEV025 *pDev)
{
	struct V206DEV026 *V206FB005;
	struct fb_info *info;

	V206FB005 = pDev->fb_info;
	if (V206FB005 == NULL) {
		return -1;
	}

	info = V206FB005->info;
	unregister_framebuffer(info);
	return 0;
}

int FUNC206LXDEV161(V206DEV025 *pDev)
{
	struct V206DEV026 *V206FB005;
	struct fb_info *info;

	V206FB005 = pDev->fb_info;
	if (V206FB005 == NULL) {
		V206KDEBUG002("fb is not register.\n");
		return -1;
	}

	info = V206FB005->info;
	register_framebuffer(info);
	mwv206fb_active(pDev);
	return 0;
}

int FUNC206LXDEV164(V206DEV025 *pDev)
{
	struct V206DEV026 *V206FB005 = pDev->fb_info;
	struct fb_info *info = V206FB005->info;

	console_lock();
	fb_set_suspend(info, FBINFO_STATE_SUSPENDED);
	console_unlock();
	mwv206fb_clear(V206FB005);
	return 0;
}

int FUNC206LXDEV163(V206DEV025 *pDev)
{
	struct V206DEV026 *V206FB005 = pDev->fb_info;
	struct fb_info *info = V206FB005->info;

	console_lock();
	fb_set_suspend(info, FBINFO_STATE_RUNNING);
	console_unlock();
	return 0;
}

int FUNC206LXDEV160(V206DEV025 *pDev)
{
	struct V206DEV026 *V206FB005 = pDev->fb_info;
	int i;

	mwv206fb_clear(V206FB005);

	for (i = 0; i < 4; i++) {
		memset(&pDev->scanctrl[i].V206DEV143, 0, sizeof(pDev->scanctrl[i].V206DEV143));
	}

	for (i = 0; i < 4; i++) {
		FUNC206HAL423(pDev, i);
	}

	for (i = 0; i < 4; i++) {
		if (pDev->pm.V206DEV110[i]) {
			FUNC206HAL390(pDev, (long)&pDev->pm.V206DEV115[i]);
		}

		if (pDev->pm.V206DEV111[i]) {
			FUNC206HAL388(pDev, (long)&pDev->pm.V206DEV116[i]);
		}
	}

	for (i = 0; i < 4; i++) {
		if ((i == pDev->pm.V206DEV119[i].V206HDMIAUDIO027) && (pDev->pm.V206DEV114[i]))  {
			mwv206_sethdmimode_params(pDev, (long)&pDev->pm.V206DEV119[i], pDev->pm.hdmi_legacy_mode, 1);
		}
	}

	for (i = 0; i < MWV206_DP_COUNT; i++) {
		if (pDev->pm.V206DEV112[i]) {
			FUNC206HAL393(pDev, (long)&pDev->pm.V206DEV117[i]);
		}
		if (pDev->pm.V206DEV113[i]) {
			FUNC206HAL394(pDev, (long)&pDev->pm.V206DEV118[i]);
		}
	}
	mwv206_resethdmiphy(pDev, 0xf);
	return 0;
}

static void mwv206_preserve_regbar(V206DEV025 *pDev)
{
	int accurate = 0;

	if (mwv206_vbios_cmp(pDev, 3, 3, 0, &accurate) >= 0 && accurate) {

	} else if (!pDev->V206DEV155) {
		FUNC206HAL183(pDev->V206DEV053);
	} else {

	}
}


static int mwv206_set_black_screen_enable(V206DEV025 *dev, int screen)
{
	unsigned char data[768] = {0};

	if (dev == NULL) {
		return -1;
	}

	if (screen >= 4 || screen < 0) {
		V206KDEBUG002("Invalid screen.\n");
		return -1;
	}

	FUNC206HAL191(dev, screen, 0, data);

	return 0;
}


static int mwv206_set_black_screen_disable(V206DEV025 *dev, int screen)
{
	if (dev == NULL) {
		return -1;
	}

	if (screen >= 4 || screen < 0) {
		V206KDEBUG002("Invalid screen.\n");
		return -1;
	}

	FUNC206HAL422(dev, screen);

	return 0;
}


#define MWV206_SET_REGISTER(pDev, reg, mask) 			\
		do { 						\
			int val = 0;				\
			val = V206DEV001(reg);			\
			val = ((val) & (mask));			\
			V206DEV002(reg, val);			\
		} while (0)

static void mwv206_disable_all_output_port(V206DEV025 *pDev)
{
	MWV206_SET_REGISTER(pDev, ((0x403000) + 0x0200), 0xFFFFFFFE);
	MWV206_SET_REGISTER(pDev, ((0x403000) + 0x0300), 0xFFFFFFFE);
	MWV206_SET_REGISTER(pDev, ((0x403000) + 0x0000), 0x7FFFFFFF);
	MWV206_SET_REGISTER(pDev, ((0x403000) + 0x0100), 0x7FFFFFFF);
	MWV206_SET_REGISTER(pDev, ((0x403000) + 0x0800), 0xFFFFFFFE);
	MWV206_SET_REGISTER(pDev, ((0x403000) + 0x0900), 0xFFFFFFFE);
	MWV206_SET_REGISTER(pDev, ((0x403000) + 0x0A00), 0x0);
}


int FUNC206LXDEV167(struct pci_dev *dev, bool suspend, bool fbcon)
{
	V206DEV025 *pDev;
	int ret;

	pDev = pci_get_drvdata(dev);
	if (pDev == NULL) {
		return -1;
	}

	MAC206HAL156("[INFO] mwv206_pci_suspend: %d.%d.%d r\n", dev->bus->number, PCI_SLOT(dev->devfn),
			PCI_FUNC(dev->devfn));

	if (suspend) {
		FUNC206HAL369(pDev, 1);
		pDev->pm.V206DEV109 = 1;

		mwv206_shadow_crtc_fallback_nolock(pDev);
		FUNC206HAL369(pDev, 0);
	}

	FUNC206LXDEV143(pDev, 0);
	if (fbcon) {
		FUNC206LXDEV164(pDev);
	}

	mwv206_preserve_regbar(pDev);

	mwv206_hdmi_suspend(pDev);


	FUNC206LXDEV091(dev);
	ret = FUNC206HAL420(pDev);
	if (ret != 0) {
		mwv206_hdmi_resume(pDev);
		if (fbcon) {
			FUNC206LXDEV163(pDev);
		}
		FUNC206LXDEV143(pDev, 1);
		if (suspend) {
			FUNC206HAL369(pDev, 1);
			pDev->pm.V206DEV109 = 0;
			FUNC206HAL369(pDev, 0);
		}
		return ret;
	}


	FUNC206LXDEV089(pDev);
	FUNC206LXDEV087(pDev);
	mwv206pm_cursor_save(pDev);


	if (pDev->V206DEV039 == 0) {
		mwv206pm_winreg_save(pDev);
	}


	mwv206_disable_all_output_port(pDev);

	return 0;
}

int FUNC206LXDEV166(struct pci_dev *dev, bool resume, bool fbcon)
{
	V206DEV025 *pDev;
	int ret;
	int i = 0;


	msleep(1000);

	if (pci_enable_device(dev)) {
		return -1;
	}
	pDev = pci_get_drvdata(dev);
	if (pDev == NULL) {
		return -1;
	}


	FUNC206HAL221(dev);

	FUNC206HAL236(pDev, pDev->V206DEV044[0], 0x40000000);
	FUNC206HAL236(pDev, pDev->V206DEV044[1], 0xA0000000);
	FUNC206HAL236(pDev, pDev->V206DEV043, 0x2000000);

	if (resume) {
		ret = FUNC206LXDEV165(pDev);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] mwv206DevInitConfig failed!\n");
			return ret;
		}
	}

	mwv206_hdmi_resume(pDev);

	FUNC206LXDEV039(pDev);
	FUNC206LXDEV143(pDev, 0);


	for (i = 0; i < 4; i++) {
		mwv206_set_black_screen_enable(pDev, i);
	}

	FUNC206HAL419((pDev));
	FUNC206LXDEV088(pDev);
	FUNC206LXDEV086(pDev);

	FUNC206LXDEV160(pDev);


	if (pDev->V206DEV039 == 0) {
		mwv206pm_winreg_reload(pDev);
	}


	for (i = 0; i < 4; i++) {
		mwv206_set_black_screen_disable(pDev, i);
	}

	if (fbcon) {
		FUNC206LXDEV163(pDev);
	}

	FUNC206LXDEV143(pDev, 1);

	FUNC206LXDEV090(dev);
	mwv206pm_cursor_reload(pDev);
	mwv206pm_palette_reload(pDev);

	if (resume) {
		FUNC206HAL369(pDev, 1);
		pDev->pm.V206DEV109 = 0;
		FUNC206HAL369(pDev, 0);
	}
	return 0;
}
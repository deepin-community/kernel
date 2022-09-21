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
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/version.h>
#include "mwv206dev.h"
#include "jmpwm.h"

#define MAC206LXDEV011 0x400004
#define MAC206LXDEV031 16

#define V206CONFIG017 0
#define V206CONFIG018 1

V206DEV025 *gst_timer_pdev;

#define MAC206LXDEV032(x, y)    \
	do {\
		uint8_t tmp = x;\
		x = y;\
		y = tmp;\
	} while (0)

static int FUNC206LXDEV016(void)
{
	static int x = 0x01000000;
	return *((volatile char *) (&x)) == 0x01;
}

unsigned int FUNC206LXDEV005(V206DEV025 *pDev)
{
	unsigned int ret;
	ret = FUNC206HAL418.FUNC206HAL213(pDev->V206DEV068[0]) +
		FUNC206HAL418.FUNC206HAL213(pDev->V206DEV068[2]);

	return ret;
}

unsigned int FUNC206LXDEV006(V206DEV025 *pDev)
{
	unsigned int ret;
	ret = FUNC206HAL418.FUNC206HAL213(pDev->V206DEV068[1]);

	return ret;
}


unsigned int FUNC206LXDEV008(V206DEV025 *pDev)
{
	unsigned int ret;
	ret = FUNC206HAL418.FUNC206HAL213(pDev->V206DEV068[0]) +
		FUNC206HAL418.FUNC206HAL213(pDev->V206DEV068[1]) +
		FUNC206HAL418.FUNC206HAL213(pDev->V206DEV068[2]);

	return ret;
}

int FUNC206LXDEV168(V206DEV025 *pDev, int temp)
{
	int totalTime = 40;
	int highlevelTime = 0;
	int flag = 0;


	temp = temp / 100;


	if (temp >= 50 && pDev->V206DEV152 == 0) {
		highlevelTime = totalTime / 10;
		pDev->V206DEV152 = 1;
		flag = 1;
	} else if (temp >= 80 && pDev->V206DEV152 == 1) {
		highlevelTime = totalTime / 5;
		pDev->V206DEV152 = 2;
		flag = 1;
	} else if (temp >= 90 && pDev->V206DEV152 == 2) {
		highlevelTime = totalTime / 4;
		pDev->V206DEV152 = 3;
		flag = 1;
	}


	if (temp < 45 && pDev->V206DEV152 == 1) {
		highlevelTime = totalTime / 10;
		pDev->V206DEV152 = 0;
		flag = 1;
	} else if (temp < 75 && pDev->V206DEV152 == 2) {
		highlevelTime = totalTime / 10;
		pDev->V206DEV152 = 1;
		flag = 1;
	} else if (temp < 85 && pDev->V206DEV152 == 3) {
		highlevelTime = totalTime / 5;
		pDev->V206DEV152 = 2;
		flag = 1;
	}

	if (pDev->V206DEV151 == V206CONFIG017) {
		highlevelTime = highlevelTime * 2;
	}

	if (flag) {
		FUNC206HAL163(pDev, 0x3, highlevelTime, totalTime, 0);
		FUNC206HAL165(pDev, 0x0, 0x8);
	}

	return 0;
}

int FUNC206LXDEV097(V206DEV025 *pDev, int *temp)
{
	signed int iotemp = 0;
	int ret;
	ret = FUNC206HAL384(pDev, &iotemp);
	*temp = 10 * iotemp / 16384;

	if (pDev->V206DEV151 != -1) {

		FUNC206LXDEV168(pDev, *temp);
	}

	return ret;
}

int FUNC206LXDEV156(V206DEV025 *pDev, long arg)
{
	int ret = 0;
	enum pcie_link_width width;
	unsigned int regValue;
	uint8_t SubVer[4];
	unsigned int free_size;
	int temprature;
	int corefreq, memfreq;

	memfreq = pDev->V206DEV075;
	corefreq = pDev->V206DEV073;

	switch (arg) {
	case 0:
		ret = corefreq;
		break;
	case 1:
		ret = (int)(corefreq * 4);
		break;
	case 2:
		ret = (int)(corefreq * 8);
		break;
	case 3:
		ret = (int)((pDev->V206DEV038) / 1024 / 1024 + (pDev->V206DEV039) / 1024 / 1024);
		break;
	case 4:
		ret = memfreq;
		break;
	case 5: {
		free_size = FUNC206LXDEV008(pDev);
		ret = (int)free_size / 1024 / 1024;
		break;
	}
	case 6: {
		FUNC206LXDEV097(pDev, &temprature);
		ret = (int)temprature;
		break;
	}
	case 7:
		ret = (int)pDev->V206DEV108.V206DEV172;
		break;
	case 8:
		ret = (int)pDev->V206DEV108.V206DEV173;
		break;
	case 9: {
		regValue = V206DEV001(MAC206LXDEV011);
		if ((regValue == 0xffffffff) || (regValue == 0)) {
			return 0;
		} else {
			if (FUNC206LXDEV016()) {
				SubVer[0] = (regValue >> (3 * 8)) & 0xff;
				SubVer[1] = (regValue >> (2 * 8)) & 0xff;
				SubVer[2] = (regValue >> (1 * 8)) & 0xff;
				SubVer[3] = (regValue >> (0 * 8)) & 0xff;
			} else {
				SubVer[0] = (regValue >> (0 * 8)) & 0xff;
				SubVer[1] = (regValue >> (1 * 8)) & 0xff;
				SubVer[2] = (regValue >> (2 * 8)) & 0xff;
				SubVer[3] = (regValue >> (3 * 8)) & 0xff;
			}
			return SubVer[0] + SubVer[1] * 100 + SubVer[2] * 10000;
		}
		break;
	}
	case 10: {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
		 enum pci_bus_speed speed;
		 pcie_get_minimum_link(pDev->V206DEV103, &speed, &width);
		 return width;
#else
		 width = pcie_get_width_cap(pDev->V206DEV103);
		 return width;
#endif
		 break;
	 }
	default:
		 ret = 0;
		 break;
	}
	return ret;
}

int FUNC206LXDEV157(V206DEV025 *pDev, long arg)
{
	if ((int)arg != -1) {
		pDev->V206DEV153 = (int)arg;
	}
	return pDev->V206DEV153;
}

static int FUNC206LXDEV014(struct seq_file *seq, void *offset)
{
	V206DEV025 *pDev = seq->private;
	enum pci_bus_speed speed;
	enum pcie_link_width width;
	unsigned int regValue;
	uint8_t SubVer[4];
	unsigned int free_size;
	int temprature;
	int tempmajor;
	int tempminor;
	int corefreq, memfreq;
	int cnt;
	unsigned int subsystemid = 0;

	unsigned int subsystemArr[MAC206LXDEV031] = {
		0x0A, 0x10, 0x01, 0x02,
		0x0B, 0x0C, 0x03, 0x04,
		0x0D, 0x05, 0x06, 0x07,
		0x0E, 0x08, 0x09, 0x0F
	};

	memfreq = pDev->V206DEV075;
	corefreq = pDev->V206DEV073;

	FUNC206LXDEV097(pDev, &temprature);
	tempmajor = temprature / 100;
	tempminor = temprature - tempmajor * 100;
	if (tempminor < 0) {
		tempminor = -tempminor;
	}

	seq_printf(seq, "Jiffies                 : %ld\n", jiffies);
	seq_printf(seq, "Vendor                  : %s\n", "Changsha JingJia Microelectronics Co.");
	seq_printf(seq, "Vendor ID               : %s\n", "0731");
	seq_printf(seq, "Device ID               : %s\n", "7200");

	subsystemid = pDev->chiptype;

	if (subsystemid > MAC206LXDEV031 || subsystemid == 0) {
		seq_printf(seq, "Subsystemid             : %s\n", "7200");
	} else if (subsystemid == 0x8) {
		seq_printf(seq, "Subsystemid             : 7500\n");
	} else {
		for (cnt = 0; cnt < MAC206LXDEV031; cnt++) {
			if (subsystemArr[cnt] == subsystemid) {
				seq_printf(seq, "Subsystemid             : 72%02d\n", cnt + 1);
				break;
			}
		}
	}


	seq_printf(seq, "Technology              : %s\n", "28 nm");

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
	pcie_get_minimum_link(pDev->V206DEV103, &speed, &width);
#else
	speed = pcie_get_speed_cap(pDev->V206DEV103);
	width = pcie_get_width_cap(pDev->V206DEV103);
#endif

	switch (speed) {
	case PCIE_SPEED_2_5GT:
		seq_printf(seq, "Bus Type                : %s%d\n", "PCIE1.0 x", width);
		break;
	case PCIE_SPEED_5_0GT:
		seq_printf(seq, "Bus Type                : %s%d\n", "PCIE2.0 x", width);
		break;
	case PCIE_SPEED_8_0GT:
		seq_printf(seq, "Bus Type                : %s%d\n", "PCIE3.0 x", width);
		break;
	default:
		break;
	}

	regValue = V206DEV001(MAC206LXDEV011);
	if (regValue == 0xffffffff || regValue == 0) {
		seq_printf(seq, "Firmware Version        : %s\n", "N/A");
	} else {
		uint32_t vbioscfgver;
		if (FUNC206LXDEV016()) {
			SubVer[0] = (regValue >> (3 * 8)) & 0xff;
			SubVer[1] = (regValue >> (2 * 8)) & 0xff;
			SubVer[2] = (regValue >> (1 * 8)) & 0xff;
			SubVer[3] = (regValue >> (0 * 8)) & 0xff;
		} else {
			SubVer[0] = (regValue >> (0 * 8)) & 0xff;
			SubVer[1] = (regValue >> (1 * 8)) & 0xff;
			SubVer[2] = (regValue >> (2 * 8)) & 0xff;
			SubVer[3] = (regValue >> (3 * 8)) & 0xff;
		}
		seq_printf(seq, "Firmware Version        : %d.%d.%d\n", SubVer[2], SubVer[1], SubVer[0]);

		vbioscfgver = SubVer[2] * 10000 + SubVer[1] * 100 + SubVer[0];

		if (vbioscfgver >= 30301) {
			seq_printf(seq, "Firmware Cfg Version    : %s\n", pDev->vbioscfgver);
		}
	}

	seq_printf(seq, "Pixel Fillrate          : %d %s\n", corefreq * 4, "Mpixel/s");
	seq_printf(seq, "Texture Fillrate        : %d %s\n", corefreq * 8, "Mpixel/s");
	seq_printf(seq, "Memory Type             : %s\n", "DDR3");

	seq_printf(seq, "Memory Size             : %d MB\n", (int)(pDev->V206DEV040));
	seq_printf(seq, "Memory Frequence        : %d MHz\n", memfreq);
	seq_printf(seq, "Memory Transfer Rates   : %d MT/s\n", memfreq * 4);

	free_size = FUNC206LXDEV005(pDev);
	seq_printf(seq, "Memory ddr0 Remain Size : %d MB\n", free_size / 1024 / 1024);
	free_size = FUNC206LXDEV006(pDev);
	seq_printf(seq, "Memory ddr1 Remain Size : %d MB\n", free_size / 1024 / 1024);

	seq_printf(seq, "Main Frequence          : %d MHz\n", corefreq);

	seq_printf(seq, "Realtime Temperature    : %d.%02d Degree\n", tempmajor, tempminor);
	seq_printf(seq, "GPU Use Rate(2d)        : %d.%02d%%\n", pDev->V206DEV108.V206DEV172 / 100, pDev->V206DEV108.V206DEV172 % 100);
	seq_printf(seq, "GPU Use Rate(3d)        : %d.%02d%%\n", pDev->V206DEV108.V206DEV173 / 100, pDev->V206DEV108.V206DEV173 % 100);

	return 0;
}

static int FUNC206LXDEV013(struct inode *inode, struct file *file)
{
	struct pci_dev *V206DEV103 = PDE_DATA(inode);

	return single_open(file, FUNC206LXDEV014, pci_get_drvdata(V206DEV103));
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops gpuinfo_file_ops = {
	.proc_open    = FUNC206LXDEV013,
	.proc_read    = seq_read,
	.proc_lseek  = seq_lseek,
	.proc_release = single_release
};
#else
static const struct file_operations gpuinfo_file_ops = {
	.owner   = THIS_MODULE,
	.open    = FUNC206LXDEV013,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};
#endif

#ifdef setup_timer
static void FUNC206LXDEV082(unsigned long data)
#else
static void FUNC206LXDEV082(struct timer_list *data)
#endif
{
	V206DEV025 *pDev = (V206DEV025 *)gst_timer_pdev;
	unsigned long jiff = jiffies;
	unsigned long flags;
	int temp = 0, gaps = 0;

	spin_lock_irqsave(&pDev->V206DEV108.lock, flags);
	if (pDev->V206DEV108.V206DEV176 != jiff) {
		gaps = jiff - pDev->V206DEV108.V206DEV176;
		if (!pDev->V206DEV096.isidle(pDev)) {
			pDev->V206DEV108.V206DEV174 += gaps;
		}
		if (!pDev->V206DEV097.isidle(pDev)) {
			pDev->V206DEV108.V206DEV175 += gaps;
		}
		pDev->V206DEV108.total += gaps;
		pDev->V206DEV108.V206DEV176 = jiff;
		if (pDev->V206DEV108.total >= HZ) {
			pDev->V206DEV108.V206DEV172 = pDev->V206DEV108.V206DEV174 * 10000 / pDev->V206DEV108.total;
			pDev->V206DEV108.V206DEV173 = pDev->V206DEV108.V206DEV175 * 10000 / pDev->V206DEV108.total;
			FUNC206LXDEV097(pDev, &temp);
			pDev->V206DEV108.V206DEV174 = 0;
			pDev->V206DEV108.V206DEV175 = 0;
			pDev->V206DEV108.total = 0;
		}
	}
	spin_unlock_irqrestore(&pDev->V206DEV108.lock, flags);
	mod_timer(&pDev->V206DEV108.timer, jiff + 1);
}

int FUNC206LXDEV158(struct pci_dev *V206DEV103)
{
	V206DEV025 *pDev = pci_get_drvdata(V206DEV103);
	struct proc_dir_entry *ent;
	char gpuName[10];
	sprintf(gpuName, "gpuinfo_%d", pDev->V206DEV099);
	ent = proc_create_data(gpuName, S_IRUGO, NULL, &gpuinfo_file_ops, V206DEV103);
	if (!ent) {
		return -ENOMEM;
	}
	gst_timer_pdev = pDev;
	spin_lock_init(&pDev->V206DEV108.lock);
#ifdef setup_timer
	setup_timer(&pDev->V206DEV108.timer, FUNC206LXDEV082, (unsigned long)pDev);
#else
	timer_setup(&pDev->V206DEV108.timer, FUNC206LXDEV082, 0);
#endif
	mod_timer(&pDev->V206DEV108.timer, jiffies + 1);

	if ((V206DEV001(0x0400004) & 0xffffff) < 0x20100) {


		pDev->V206DEV151 = V206CONFIG017;


		if (pDev->V206DEV151 == V206CONFIG017) {
			FUNC206HAL163(pDev, 0x3, 8, 40, 0);
		} else {
			FUNC206HAL163(pDev, 0x3, 4, 40, 0);
		}
		FUNC206HAL165(pDev, 0x0, 0x8);
		pDev->V206DEV152 = 0;
	} else {
		pDev->V206DEV151 = -1;
	}

	return 0;
}

void FUNC206LXDEV076(struct pci_dev *V206DEV103)
{
	V206DEV025 *dev = pci_get_drvdata(V206DEV103);
	char gpuName[10];
	sprintf(gpuName, "gpuinfo_%d", dev->V206DEV099);
	del_timer_sync(&dev->V206DEV108.timer);
	remove_proc_entry(gpuName, NULL);
}
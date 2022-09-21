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
#include "linux/delay.h"
#include "mwv206dev.h"

static int idle_add_sleep = 1;
module_param(idle_add_sleep, int, 0644);
MODULE_PARM_DESC(idle_add_sleep, "Add sleep in idle, improve Xorg performance, default enabled.");

unsigned int FUNC206HAL467(unsigned long addr)
{
	return ioread32((void *)addr);
}

void FUNC206HAL470(unsigned long addr, unsigned int data)
{
	iowrite32(data, (void *)addr);
}

unsigned short FUNC206HAL468(unsigned long addr)
{
	return ioread16((void *)addr);
}
void FUNC206HAL471(unsigned long addr, unsigned short data)
{
	iowrite16(data, (void *)addr);
}

unsigned char FUNC206HAL466(unsigned long addr)
{
	return ioread8((void *)addr);
}
void FUNC206HAL469(unsigned long addr, unsigned char data)
{
	iowrite8(data, (void *)addr);
}

static int mwv206ReadRegWithCheckNoSleep(V206DEV025 *pDev, int reg, int mask, int val)
{
	GLJ_TICK tick;
	int V206IOCTLCMD010;

	V206IOCTLCMD010 = V206DEV001(reg);
	mwv206_timed_loop(tick, (V206IOCTLCMD010 & mask) != val, MWV206CMDTIMEOUT) {
		V206IOCTLCMD010 = V206DEV001(reg);
	}

	if ((V206IOCTLCMD010 & (mask)) == val) {
		V206DEV005("\nread reg done: (0x%x:0x%x)\n\n",  reg, V206IOCTLCMD010);
		return 0;
	}

	V206KDEBUG002("\nread reg failure: (0x%x:0x%x), mask = 0x%x, expect val = 0x%x\n\n", reg, V206IOCTLCMD010, mask, val);
	return -1;
}

static int mwv206ReadRegWithCheckSleep(V206DEV025 *pDev, int reg, int mask, int val)
{
	GLJ_TICK tick;
	int V206IOCTLCMD010;
	int try_count = 0;
	int usleep_time = 4;

	V206IOCTLCMD010 = V206DEV001(reg);
	mwv206_timed_loop (tick, (V206IOCTLCMD010 & mask) != val, MWV206CMDTIMEOUT) {
		try_count++;
		if ((try_count > 32) && ((try_count & 1) == 0))
			usleep_range(usleep_time, usleep_time);
		V206IOCTLCMD010 = V206DEV001(reg);
	}

	if ((V206IOCTLCMD010 & (mask)) == val) {
		V206DEV005("\nread reg done: (0x%x:0x%x)\n\n",  reg, V206IOCTLCMD010);
		return 0;
	}

	V206KDEBUG002("\nread reg failure: (0x%x:0x%x), mask = 0x%x, expect val = 0x%x\n\n", reg, V206IOCTLCMD010, mask, val);
	return -1;
}

int FUNC206HAL231(V206DEV025 *pDev, int reg, int mask, int val)
{
	if (idle_add_sleep) {
		return mwv206ReadRegWithCheckSleep(pDev, reg, mask, val);
	} else {
		return mwv206ReadRegWithCheckNoSleep(pDev, reg, mask, val);
	}
}

int FUNC206HAL244(V206DEV025 *pDev, unsigned int reg, unsigned int val)
{
	unsigned int readval;
	GLJ_TICK tick;

	V206DEV002(reg, val);

	readval = V206DEV001(reg);
	mwv206_timed_loop (tick, readval != val, MWV206CMDTIMEOUT) {
		readval = V206DEV001(reg);
	}

	if (readval != val) {
		V206KDEBUG002("write 0x%x to 0x%x register timeout, readval = 0x%x.",
				val, reg, readval);
		return -1;
	}

	return 0;
}

int V206IOCTL130(V206DEV025 *pDev, long arg)
{
	V206IOCTL167 *getsetreg;
	unsigned long       temp;

	getsetreg = (V206IOCTL167 *)arg;

	switch (getsetreg->action) {
	case V206IOCTL001:
		V206DEV002(getsetreg->reg, getsetreg->setvalue);
		break;
	case V206IOCTL002:
		getsetreg->getvalue = V206DEV001(getsetreg->reg);
		break;
	case V206IOCTL003:
		getsetreg->getvalue = V206DEV001(getsetreg->reg);
		V206DEV002(getsetreg->reg, getsetreg->setvalue);
		break;
	case V206IOCTL004:
		getsetreg->getvalue = V206DEV001(getsetreg->reg);
		temp = getsetreg->getvalue;
		temp &= ~getsetreg->setmask;
		temp |= getsetreg->setvalue & getsetreg->setmask;
		V206DEV002(getsetreg->reg, temp);
		break;
	default:
		break;
	}

	return 0;
}



void FUNC206HAL237(V206DEV025 *pDev,
		unsigned int barno, jjuint32 targAddr)
{
	int regionIdx = barno;
	struct pci_dev *dev;
	unsigned int reg;
	GLJ_TICK tick;

	dev = pDev->V206DEV103;

	if (barno >= 16) {
		V206KDEBUG002("unsupported barno %d.\n", barno);
	}
	V206DEV005("[%s] barno %d, targAddr = 0x%llx.\n", __FUNCTION__, barno, targAddr);

	pDev->V206DEV121[barno] = targAddr;

	PCI_WRITE_U32(dev, 0x900,
			(1 << 31)
			| (regionIdx << 0));
	PCI_WRITE_U32(dev, 0x918,
			targAddr & 0xFFFFFFFF);
	PCI_WRITE_U32(dev, 0x91C,
			(targAddr >> 16) >> 16);
	PCI_WRITE_U32(dev, 0x904, 0x0);
	PCI_WRITE_U32(dev, 0x908,
			(1 << 31)
			| (0 << 30)
			| (barno << 8));

	PCI_READ_U32(dev, 0x918, &reg);
	mwv206_timed_loop (tick, reg != (targAddr & 0xFFFFFFFF), MWV206CMDTIMEOUT) {
		PCI_READ_U32(dev, 0x918, &reg);
	}

	if (reg != (targAddr & 0xFFFFFFFF)) {
		V206KDEBUG002("MWV206: switch inbound bar timeout\n");
	}
}

void FUNC206HAL236(V206DEV025 *pDev,
		unsigned int barno, jjuint32 targAddr)
{
	unsigned long flags;
	flags = FUNC206HAL094(pDev->V206DEV101);
	FUNC206HAL237(pDev, barno, targAddr);
	FUNC206HAL095(pDev->V206DEV101, flags);
	return;
}

void FUNC206HAL235(V206DEV025 *pDev,
		unsigned int barno, jjuint32 targAddr)
{
	unsigned long flags;


	if ((pDev->V206DEV039 == 0) && (targAddr >= 0xA0000000)) {
		return;
	}

	flags = FUNC206HAL094(pDev->V206DEV101);
	if (targAddr == pDev->V206DEV121[barno]) {
		FUNC206HAL095(pDev->V206DEV101, flags);
		return;
	}
	FUNC206HAL237(pDev, barno, targAddr);
	FUNC206HAL095(pDev->V206DEV101, flags);
	return;
}

void FUNC206HAL238(V206DEV025 *pDev,
		unsigned char regionIdx, unsigned int size,
		jjuint32 srcAddr, jjuintptr targAddr)
{
	unsigned long endAddr, flags;
	struct pci_dev *dev;

	dev = pDev->V206DEV103;

	endAddr = (srcAddr & 0xFFFFFFFF) + size;

	if (endAddr > 0xFFFFFFFF) {
		V206KDEBUG002("unsupported map, startAddr and endAddr should have same high-32 bit.");
	}

	if (regionIdx < 16) {
		V206KDEBUG002("unsupported barno %d.\n", regionIdx);
	}
	flags = FUNC206HAL094(pDev->V206DEV101);
	PCI_WRITE_U32(dev, 0x900,
			(0 << 31)
			| (regionIdx << 0));

	PCI_WRITE_U32(dev, 0x904, 0x0);
	PCI_WRITE_U32(dev, 0x908,
			(1 << 31)
			| (0 << 30));

	PCI_WRITE_U32(dev, 0x918, (unsigned long)(targAddr & 0xFFFFFFFF));

	PCI_WRITE_U32(dev, 0x91C, ((targAddr >> 16) >> 16));

	PCI_WRITE_U32(dev, 0x90C, srcAddr & 0xFFFFFFFF);
	PCI_WRITE_U32(dev, 0x910, ((srcAddr >> 16) >> 16));
	PCI_WRITE_U32(dev, 0x914, endAddr);


	FUNC206HAL095(pDev->V206DEV101, flags);
	return;
}


void V206DEV006(void *devInfo, unsigned int regAddr, unsigned int value)
{
	V206DEV025 *pDev = (V206DEV025 *)devInfo;

	V206DEV002(regAddr, value);
}


unsigned int V206DEV007(void *devInfo, unsigned int regAddr)
{
	V206DEV025 *pDev = (V206DEV025 *)devInfo;

	return V206DEV001(regAddr);
}


unsigned int FUNC206HAL472(unsigned int input, unsigned int num)
{
	unsigned int count, tempInput;

	tempInput = input;
	count = 0;

	while (tempInput >= num) {
		tempInput -= num;
		count += 1;
	}

	return count;
}
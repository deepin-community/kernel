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
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/input-polldev.h>

#include "gljos.h"
#include "mwv206dev.h"
#include "mwv206ioctl.h"
#include "mwv206dec.h"
#include "mwv206reg.h"
#include "mwv206kconfig.h"
#include "mwv206kdevconfig.h"
#include "videoadd_reg.h"
#include "cputype.h"

#define MAC206HAL147 1

#define VBIOS_STATUS_REG            (0x00400000)
#define VBIOS_PHASE4_SHIFT          (27)
#define MAC206HAL216           (0x00400004)
#define MAC206HAL223          (FUNC206LXDEV098())

#define VBIOS_VER_MAJOR(ver)        (((ver) >> 16) & 0xff)
#define VBIOS_VER_MINOR(ver)        (((ver) >>  8) & 0xff)
#define VBIOS_VER_REVISION(ver)     (((ver)) & 0xff)
#define VBIOS_VER_NUMBER(major, minor, rev) \
	((((major) & 0xff) << 16) | (((minor) & 0xff) << 8) | ((rev) & 0xff))
#define V206DEVCONPARSER004             (0x1FE0)
#define V206DEVCONPARSER003               (0x1FF0)

typedef enum {
	MWV206INTR_ROUTE_TO_CPU = 0,
	MWV206INTR_ROUTE_TO_PCIE,
	MWV206INTR_ROUTE_TO_CPU_PCIE,
	MWV206INTR_ROUTE_TO_OUTSIDE,
	MWV206INTR_ROUTE_MAX
} e_mwv206_intr_route_t;

typedef enum  {
	V206API032 = 0,
	V206API033,
	V206API034,
	V206API035,
	V206API036
} e_mwv206_intr_route_pcie_t;


int mwv206_vbios_cmp(V206DEV025 *pDev, int major, int minor, int rev, int *paccurate)
{
	int cur_ver  = VBIOS_VER_NUMBER(pDev->vbios_major, pDev->vbios_minor, pDev->vbios_revision);
	int base_ver = VBIOS_VER_NUMBER(major, minor, rev);
	int accurate = 1;


	if (base_ver == VBIOS_VER_NUMBER(4, 0, 6)
		|| VBIOS_VER_MAJOR(base_ver) == 0
		|| VBIOS_VER_MAJOR(base_ver) == 255) {
		V206KDEBUG003("mwv206: warning bad vbios base version\n");
		accurate = 0;
	}

	if (cur_ver == VBIOS_VER_NUMBER(4, 0, 6)) {
		cur_ver = VBIOS_VER_NUMBER(2, 1, 2);
		if ((base_ver >= VBIOS_VER_NUMBER(3, 1, 2) && base_ver < VBIOS_VER_NUMBER(3, 2, 6))
			|| (base_ver >= VBIOS_VER_NUMBER(2, 1, 2) && base_ver < VBIOS_VER_NUMBER(2, 2, 6))) {
			accurate = 0;
		}
	} else if (VBIOS_VER_MAJOR(cur_ver) == 0 || VBIOS_VER_MAJOR(cur_ver) == 255) {

		accurate = 0;
		cur_ver = VBIOS_VER_NUMBER(0, 0, 0);
	}

	switch (VBIOS_VER_MAJOR(base_ver)) {
	case 2:

		if (VBIOS_VER_MAJOR(cur_ver) == 3) {
			cur_ver -= VBIOS_VER_NUMBER(1, 0, 0);
		}
		break;
	case 3:

		if (VBIOS_VER_MAJOR(cur_ver) == 2) {
			cur_ver += VBIOS_VER_NUMBER(1, 0, 0);
		}
		break;
	default:
		break;
	}

	if (paccurate) {
		*paccurate = accurate;
	}

	return cur_ver - base_ver;
}


int FUNC206HAL006(V206DEV025 *pDev)
{
	unsigned int val, V206IOCTLCMD010;
	int ret;

	V206KDEBUG003("[INFO] software reset.\n");


	val = V206DEV001(0x400240);
	V206IOCTLCMD010 = val & 0xFFFFE000;
	FUNC206LXDEV128(FUNC206LXDEV098() / 2);
	ret = FUNC206HAL244(pDev, 0x400240, V206IOCTLCMD010);
	if (0 != ret) {
		V206KDEBUG002("[ERROR] mwv206WriteRegWithCheck  failed(%d)!\n", ret);
		return -1;
	} else {
		V206DEV005("[INFO] mwv206WriteRegWithCheck successed\n");
	}

	V206IOCTLCMD010 = val | 0x111F;
	ret = FUNC206HAL244(pDev, 0x400240, V206IOCTLCMD010);
	if (0 != ret) {
		V206KDEBUG002("[ERROR] mwv206WriteRegWithCheck  failed(%d)!\n", ret);
		return -2;
	} else {
		V206DEV005("[INFO] mwv206WriteRegWithCheck successed\n");
	}




	FUNC206LXDEV128(FUNC206LXDEV098() / 2);

	return ret;
}

int FUNC206HAL001(V206DEV025 *pDev)
{
	unsigned int val, V206IOCTLCMD010;


	val = V206DEV001(0x400240);
	V206IOCTLCMD010 = val & 0xFFFFE000;
	V206DEV002(0x400240, V206IOCTLCMD010);
	val = V206DEV001(0x400240);

	V206IOCTLCMD010 = val | 0x111F;
	V206DEV002(0x400240, V206IOCTLCMD010);
	val = V206DEV001(0x400240);

	return 0;
}

int FUNC206HAL004(V206DEV025 *pDev)
{
	int ret = 0;

	FUNC206HAL291(pDev, 1);

	FUNC206LXDEV128(FUNC206LXDEV098() / 100);
	ret = FUNC206HAL244(pDev, 0x400240, 0x31071100);
	if (0 != ret) {
		V206KDEBUG002("[ERROR] mwv206WriteRegWithCheck  failed(%d)!\n", ret);
		FUNC206HAL291(pDev, 0);
		return -1;
	} else {
		V206DEV005("[INFO] mwv206WriteRegWithCheck successed\n");
	}
	FUNC206LXDEV128(FUNC206LXDEV098() / 10);

	ret = FUNC206HAL244(pDev, 0x400240, 0x3107111F);
	if (0 != ret) {
		V206KDEBUG002("[ERROR] mwv206WriteRegWithCheck  failed(%d)!\n", ret);
		FUNC206HAL291(pDev, 0);
		return -2;
	} else {
		V206DEV005("[INFO] mwv206WriteRegWithCheck successed\n");
	}
	FUNC206LXDEV128(FUNC206LXDEV098() / 100);
	FUNC206HAL291(pDev, 0);
	return 0;
}

int FUNC206HAL065;
unsigned long FUNC206HAL067[4][2], FUNC206HAL068[4];


#define MAC206HAL146  (64)

#define MAC206HAL142  (512)


static int FUNC206HAL005(V206DEV025 *pDev)
{
	jjuint32 endaddr;
	jjuint32 startaddr;


	if (pDev->V206DEV039 == 0) {
		pDev->V206DEV083[0] = 0x00000000;
		endaddr = FUNC206HAL323(pDev, 32);
		pDev->V206DEV082[0] = endaddr;
		pDev->V206DEV083[1] = 0x80000000;
		pDev->V206DEV082[1] = 32;
	} else {
		pDev->V206DEV083[0] = 0x00000000;
		pDev->V206DEV082[0] = 32;


		startaddr = (jjuint32) 0x80000000;
		pDev->V206DEV083[1] = startaddr;
		endaddr = FUNC206HAL323(pDev, startaddr + 32);
		pDev->V206DEV082[1] = endaddr - startaddr;
	}

	return 0;
}

unsigned int FUNC206HAL224(void *dev, int size, int align, void *userdata);
unsigned int FUNC206HAL225(void *dev, int size, int align, void *userdata);



static int FUNC206HAL227(V206DEV025 *pDev, int V206DEV072,
		int V206DEV038, int V206DEV039)
{
	unsigned long addr;
	int blocksize;
	unsigned long memsize2d, memsize3d;
	int ddr0blockcnt, ddr1blockcnt;
	int ddr0resvsize = 0, ddr1resvsize = 0;
	unsigned long ddr02dsize, totolddrsize;

	if (V206DEV038 == 0) {
		V206KDEBUG002("[ERROR] ddrsize is not initialized.\n");
		return -1;
	}

	V206DEV005("pDev->memsize[MWV206_MEM2D_IDX] %#x, pDev->memsize[MWV206_MEM3D_IDX] %#x\n",
			pDev->V206DEV032[0],
			pDev->V206DEV032[1]);

	pDev->V206DEV038 = (unsigned long)V206DEV038 << 20;
	pDev->V206DEV039 = (unsigned long)V206DEV039 << 20;
	memsize2d = pDev->V206DEV032[0];
	memsize3d = pDev->V206DEV032[1];
	ddr02dsize = pDev->V206DEV038 / 2;
	totolddrsize = pDev->V206DEV038 + pDev->V206DEV039;


	if (pDev->V206DEV039 == 0) {
		pDev->V206DEV160 = FUNC206HAL224;
	} else {
		pDev->V206DEV160 = FUNC206HAL225;
	}


	if ((ddr02dsize % (64 << 20)) != 0) {
		V206KDEBUG002("2d reserved size should be aligh 64M.");
		return -1;
	}

	if (memsize2d > ddr02dsize) {
		memsize2d = ddr02dsize;
	}


	if (memsize2d == 0) {
		blocksize = memsize3d;
	} else if (memsize3d == 0) {
		blocksize = memsize2d;
	} else {
		blocksize = memsize2d > memsize3d ? memsize3d : memsize2d;
	}

	if (blocksize < (256 << 20)) {
		blocksize = 256 << 20;
	}

	V206DEV005("[INFO] ddr02dsize = %ldMbytes\n", ddr02dsize >> 20);
	V206KDEBUG003("[INFO] memblocksize = %dMbytes\n", blocksize >> 20);

	pDev->V206DEV045 = blocksize;
	if (pDev->V206DEV045 == 0 || ((pDev->V206DEV045 - 1) & pDev->V206DEV045)) {
		V206KDEBUG003("[INFO] memblocksize not aligned: 0x%x\n", pDev->V206DEV045);
	}


	FUNC206HAL005(pDev);
	ddr0resvsize = pDev->V206DEV082[0];
	ddr1resvsize = pDev->V206DEV082[1];
	if ((ddr0resvsize > blocksize) || (ddr1resvsize > blocksize)) {
		V206KDEBUG002("[ERROR] resv space(0x%x, 0x%x) is bigger than one block size(0x%x).\n",
				ddr0resvsize, ddr1resvsize, blocksize);
		return -2;
	}


	ddr0blockcnt = pDev->V206DEV038 / blocksize;
	V206DEV005("[INFO] ddr0blockcnt = %d\n", ddr0blockcnt);
	pDev->V206DEV068[0] = FUNC206HAL418.FUNC206HAL204(ddr0blockcnt);
	pDev->V206DEV068[2] = FUNC206HAL418.FUNC206HAL204(ddr0blockcnt);


	addr = 0x00000000;
	FUNC206HAL418.FUNC206HAL197(pDev->V206DEV068[2],
			addr + ddr0resvsize,
			blocksize - ddr0resvsize);
	addr += blocksize;


	while (addr < ddr02dsize) {
		FUNC206HAL418.FUNC206HAL197(pDev->V206DEV068[2], addr, blocksize);
		addr += blocksize;
	}


	if ((V206DEV072 < MAC206HAL146) || (V206DEV072 > MAC206HAL142)) {
		V206KDEBUG002("invalid resmemsize %dMbytes, should be in range of [%d..%d]Mbytes\n",
				V206DEV072, MAC206HAL146, MAC206HAL142);
		return -2;
	}
	V206DEV072 <<= 20;

	if ((unsigned long)V206DEV072 > totolddrsize / 4) {
		V206DEV072 = totolddrsize / 4;
	}

	if ((unsigned long)V206DEV072 > pDev->V206DEV038 - ddr02dsize) {
		pDev->V206DEV072 = pDev->V206DEV038 - ddr02dsize;
	} else {
		pDev->V206DEV072 = V206DEV072;
	}
	V206KDEBUG003("[INFO] resmemsize %dMbytes.\n", pDev->V206DEV072 >> 20);
	pDev->V206DEV077 = addr;
	addr += pDev->V206DEV072;


	if (addr < pDev->V206DEV038) {
		int size = blocksize - (addr % blocksize);
		FUNC206HAL418.FUNC206HAL197(pDev->V206DEV068[0], addr, size);
		addr += size;
	}

	while (addr < (pDev->V206DEV038 + 0x00000000)) {
		FUNC206HAL418.FUNC206HAL197(pDev->V206DEV068[0], addr, blocksize);
		addr += blocksize;
	}

	if (pDev->V206DEV039 == 0) {
		return 0;
	}



	ddr1blockcnt = pDev->V206DEV039 / blocksize;
	V206DEV005("[INFO] ddr1 blockcnt = %d\n", ddr1blockcnt);
	pDev->V206DEV068[1] = FUNC206HAL418.FUNC206HAL204(ddr1blockcnt);

	addr = 0x80000000;

	FUNC206HAL418.FUNC206HAL197(pDev->V206DEV068[1],
			addr + ddr1resvsize,
			blocksize - ddr1resvsize);

	addr += blocksize;

	while (addr < (pDev->V206DEV039 + 0x80000000)) {
		FUNC206HAL418.FUNC206HAL197(pDev->V206DEV068[1], addr, blocksize);
		addr += blocksize;
	}

	pDev->V206DEV084 = 0;
	pDev->V206DEV085 = 0;
	return 0;
}

static int FUNC206HAL003(V206DEV025 *pDev)
{
#define MAC206HAL207 (((0x200000) + 0x4000) + 0x0104)
	int oldval, V206IOCTLCMD010 = 0x5a, newval1;
	oldval = V206DEV001(MAC206HAL207);
	V206DEV005("[0x%x] 0x%x", MAC206HAL207, oldval);
	V206DEV002(MAC206HAL207, V206IOCTLCMD010);
	newval1 = V206DEV001(MAC206HAL207);
	if (V206IOCTLCMD010 != newval1) {
		V206KDEBUG002("[ERROR] reg_base_addr = 0x%lx\n", pDev->V206DEV033);
		V206KDEBUG002("[ERROR] register read/write ERROR!!![0x%x != 0x%x(expectedval)].\n", newval1, V206IOCTLCMD010);
		return -1;
	} else {
		V206KDEBUG003("[INFO] register read/write RIGHT!!![0x%x == 0x%x(expectedval)].\n", newval1, V206IOCTLCMD010);
		V206DEV002(MAC206HAL207, oldval);
		return 0;
	}
}

static int FUNC206HAL239(V206DEV025 *pDev, unsigned long mask, int reset)
{
	unsigned long val;
	val = V206DEV001(0x4);
	if (!reset) {
		val |= mask;
	} else {
		val &= ~mask;
	}
	V206DEV002(0x4, val);
	return 0;
}

static int mwv206_ddr_config(V206DEV025 *pDev, int memfreq)
{
	int ret;

	V206KDEBUG003("[INFO] mwv206 reconfig ddr to %d!\n", memfreq);


	if (memfreq >= 50) {
		ret = FUNC206HAL136(pDev, V206IOCTL010, memfreq * 1000000);
		if (ret != 0) {
			return ret;
		}
	}

	ret = FUNC206HAL215(pDev);
	if (ret != 0) {
		return ret;
	}

	V206KDEBUG003("[INFO] mwv206 ddr init done!\n");
	return 0;
}

int FUNC206HAL315(V206DEV025 *pDev, int memfreq)
{
	int current_memfreq;
	int accurate;
	int ret = 0;


	FUNC206LXDEV154(pDev, 0,
			0x0,
			3840 * 4,
			32,
			0, 0,
			3840, 2160,
			0,
			0xffffffff,
			3);



	 if (mwv206_vbios_cmp(pDev, 3, 3, 0, &accurate) >= 0 && accurate) {

	 } else {
		jme_chip_grade grade = mwv206_get_chipgrade(pDev);

		FUNC206HAL134(pDev, V206IOCTL010, &current_memfreq);

		if (grade == JMV_CHIP_COMMERCIAL) {
			if (current_memfreq == 266 && current_memfreq != memfreq) {
				ret = mwv206_ddr_config(pDev, memfreq);
			}
		} else {
			if (current_memfreq != memfreq) {
				ret = mwv206_ddr_config(pDev, memfreq);
			}
		}
	}

	mwv206_ddr_status(pDev);

	return ret;
}

static inline int FUNC206HAL070(V206DEV025 *pDev)
{
	int v = V206DEV001(0x400008);
	return (v & 0x3);
}

int FUNC206HAL379(V206DEV025 *dev)
{
	int i;
	int pcie_controller = 1;
	struct pci_dev *V206DEV103;

	pcie_controller = FUNC206HAL070(dev);

	if (pcie_controller == 0) {
		V206KDEBUG003("[INFO] pci controller 0.");
		dev->V206DEV036 = 0x0B000000;
		dev->V206DEV037 = 0x0B1FFFFF;
		dev->V206DEV090.V206DEV171 = 1024;

		for (i = 0; i < MWV206KINTR_COUNT; i++) {
			FUNC206HAL367(dev, i, MWV206INTR_ROUTE_TO_PCIE);
			FUNC206HAL366(dev, i, V206API032);
		}
	} else if (pcie_controller == 1) {
		V206KDEBUG003("[INFO] pci controller 1.");
		dev->V206DEV036 = 0x2B000000;
		dev->V206DEV037 = 0x2B0FFFFF;
		if (V206CTYPE009(dev->V206DEV028)) {
			dev->V206DEV090.V206DEV171 = 1024;
		} else {
			dev->V206DEV090.V206DEV171 = MWV206_MAX_DMA_SIZE;
		}

		for (i = 0; i < MWV206KINTR_COUNT; i++) {
			FUNC206HAL367(dev, i, MWV206INTR_ROUTE_TO_PCIE);
			FUNC206HAL366(dev, i, V206API034);
		}
	} else {
		V206KDEBUG002("invalid pcie controller %d.", pcie_controller);
	}

	V206DEV103 = dev->V206DEV103;

	PCI_WRITE_U32(V206DEV103, 0xA78, 0);
	return 0;
}

static void mwv206_set_i2c_usage_table(struct mwv206_port_config *port, int *usage_table)
{
	int chan;

	if (!V206DEVCONFIG036(port->flags)) {
		return;
	}

	chan = port->i2cchan;
	if (chan >= 0 && chan < 8) {
		usage_table[chan] = 1;
	} else {
		V206KDEBUG003("MWV206: invalid i2c-chan:%d configured\n", chan);
	}
}

static void mwv206_disable_unused_i2cchan(V206DEV025 *pDev)
{
	struct mwv206_dev_config *cfg = &pDev->V206DEV105;
	int  i2c_in_use[8] = {0};
	int  i;


	mwv206_set_i2c_usage_table(&cfg->hdmi[0], i2c_in_use);
	mwv206_set_i2c_usage_table(&cfg->hdmi[1], i2c_in_use);
	mwv206_set_i2c_usage_table(&cfg->hdmi[2], i2c_in_use);
	mwv206_set_i2c_usage_table(&cfg->hdmi[3], i2c_in_use);
	mwv206_set_i2c_usage_table(&cfg->vga[0], i2c_in_use);
	mwv206_set_i2c_usage_table(&cfg->vga[1], i2c_in_use);
	mwv206_set_i2c_usage_table(&cfg->lvds[0], i2c_in_use);
	mwv206_set_i2c_usage_table(&cfg->lvds[1], i2c_in_use);
	mwv206_set_i2c_usage_table(&cfg->dvo[0], i2c_in_use);
	mwv206_set_i2c_usage_table(&cfg->dvo[1], i2c_in_use);

	for (i = 0; i < 8; i++) {
		if (!i2c_in_use[i]) {
			V206KDEBUG003("[INFO]: mwv206 i2c-chan %d is disabled, because it's not used\n", i);
			MWV206_I2C_DISABLE_CHANNEL(pDev, i);
		}
	}


	switch (cfg->chip.blctrl) {
	case 26:
	case 27:
		MWV206_I2C_DISABLE_CHANNEL(pDev, 2);
		break;
	case 29:
	case 30:
		MWV206_I2C_DISABLE_CHANNEL(pDev, 3);
		break;
	case 32:
	case 33:
		MWV206_I2C_DISABLE_CHANNEL(pDev, 4);
		break;
	case 35:
	case 36:
		MWV206_I2C_DISABLE_CHANNEL(pDev, 5);
		break;
	case 38:
	case 39:
		MWV206_I2C_DISABLE_CHANNEL(pDev, 6);
		break;
	case 40:
	case 41:
		MWV206_I2C_DISABLE_CHANNEL(pDev, 7);
		break;
	default:
		break;
	}
}

void FUNC206HAL330(V206DEV025 *pDev)
{
	int accurate;


	MWV206_I2C_CHAN_SELECT_IPCORE(pDev, 0);
	MWV206_I2C_CHAN_SELECT_IPCORE(pDev, 1);


	V206DEV003(((0x400000) + 0x140), 0x0000ffff, 0x00001111);
	V206DEV003(((0x400000) + 0x144), 0xffffffff, 0x11111111);
	V206DEV003(((0x400000) + 0x16C), 0x0000ff00, 0x0);
	V206DEV003(((0x400000) + 0xE64), 0x0000ff00, 0x00003300);

	V206DEV003(((0x400000) + 0x16C), 0x0ff00000, 0x0);
	V206DEV003(((0x400000) + 0xE64), 0x0ff00000, 0x03300000);


	if (mwv206_vbios_cmp(pDev, 3, 2, 8, &accurate) >= 0 && accurate) {
		V206DEV003(((0x400000) + 0x170), 0xff0ff0ff, 0x0);
		V206DEV003(VBIOS_STATUS_REG, 0x00000002, 0x2);
		V206KDEBUG003("[INFO]: mwv206 i2c-chan %d goes by software\n", 4);
		V206KDEBUG003("[INFO]: mwv206 i2c-chan %d goes by software\n", 5);
		V206KDEBUG003("[INFO]: mwv206 i2c-chan %d goes by software\n", 6);
	} else {
		V206DEV003(((0x400000) + 0x170), 0xff0ff0ff, 0x22022022);
		MWV206_I2C_CHAN_SELECT_IPCORE(pDev, 4);
		MWV206_I2C_CHAN_SELECT_IPCORE(pDev, 5);
		MWV206_I2C_CHAN_SELECT_IPCORE(pDev, 6);
		V206KDEBUG003("[INFO]: mwv206 i2c-chan %d goes by ipcore\n", 4);
		V206KDEBUG003("[INFO]: mwv206 i2c-chan %d goes by ipcore\n", 5);
		V206KDEBUG003("[INFO]: mwv206 i2c-chan %d goes by ipcore\n", 6);
	}
	V206DEV003(((0x400000) + 0xE68), 0xff0ff0ff, 0x33033033);

	mwv206_disable_unused_i2cchan(pDev);

	if (!MWV206_I2C_CHAN_DISABLED(pDev, 7)) {
		V206DEV003(((0x400000) + 0x174), 0xff, 0x0);
		V206DEV003(((0x400000) + 0xE6C), 0xff, 0x33);
	}



	FUNC206HAL161(pDev->V206DEV051, 50);
}

static void FUNC206HAL318(V206DEV025 *pDev)
{
	pDev->V206DEV144[MWV206_DP_DVO_0].V206FB011 = 0;
	pDev->V206DEV144[MWV206_DP_DVO_0].dualpixel = V206DEVCONFIG037(pDev->V206DEV105.dvo[0].flags);
	pDev->V206DEV144[MWV206_DP_DVO_0].enable = V206DEVCONFIG036(pDev->V206DEV105.dvo[0].flags);
	pDev->V206DEV144[MWV206_DP_DVO_1].V206FB011 = 1;
	pDev->V206DEV144[MWV206_DP_DVO_1].dualpixel = V206DEVCONFIG037(pDev->V206DEV105.dvo[1].flags);
	pDev->V206DEV144[MWV206_DP_DVO_1].enable = V206DEVCONFIG036(pDev->V206DEV105.dvo[1].flags);
	pDev->V206DEV144[MWV206_DP_DAC_0].V206FB011 = 0;
	pDev->V206DEV144[MWV206_DP_DAC_0].dualpixel = V206DEVCONFIG037(pDev->V206DEV105.vga[0].flags);
	pDev->V206DEV144[MWV206_DP_DAC_0].enable = V206DEVCONFIG036(pDev->V206DEV105.vga[0].flags);
	pDev->V206DEV144[MWV206_DP_DAC_1].V206FB011 = 1;
	pDev->V206DEV144[MWV206_DP_DAC_1].dualpixel = V206DEVCONFIG037(pDev->V206DEV105.vga[1].flags);
	pDev->V206DEV144[MWV206_DP_DAC_1].enable = V206DEVCONFIG036(pDev->V206DEV105.vga[1].flags);
	pDev->V206DEV144[MWV206_DP_LVDS_0].V206FB011 = 0;
	pDev->V206DEV144[MWV206_DP_LVDS_0].dualpixel = V206DEVCONFIG037(pDev->V206DEV105.lvds[0].flags);
	pDev->V206DEV144[MWV206_DP_LVDS_0].enable = V206DEVCONFIG036(pDev->V206DEV105.lvds[0].flags);
	pDev->V206DEV144[MWV206_DP_LVDS_1].V206FB011 = 1;
	pDev->V206DEV144[MWV206_DP_LVDS_1].dualpixel = V206DEVCONFIG037(pDev->V206DEV105.lvds[1].flags);
	pDev->V206DEV144[MWV206_DP_LVDS_1].enable = V206DEVCONFIG036(pDev->V206DEV105.lvds[1].flags);
	pDev->V206DEV144[MWV206_DP_HDMI_0].V206FB011 = 0;
	pDev->V206DEV144[MWV206_DP_HDMI_0].dualpixel = V206DEVCONFIG037(pDev->V206DEV105.hdmi[0].flags);
	pDev->V206DEV144[MWV206_DP_HDMI_0].enable = V206DEVCONFIG036(pDev->V206DEV105.hdmi[0].flags);
	pDev->V206DEV144[MWV206_DP_HDMI_1].V206FB011 = 1;
	pDev->V206DEV144[MWV206_DP_HDMI_1].dualpixel = V206DEVCONFIG037(pDev->V206DEV105.hdmi[1].flags);
	pDev->V206DEV144[MWV206_DP_HDMI_1].enable = V206DEVCONFIG036(pDev->V206DEV105.hdmi[1].flags);
	pDev->V206DEV144[MWV206_DP_HDMI_2].V206FB011 = 2;
	pDev->V206DEV144[MWV206_DP_HDMI_2].dualpixel = V206DEVCONFIG037(pDev->V206DEV105.hdmi[2].flags);
	pDev->V206DEV144[MWV206_DP_HDMI_2].enable = V206DEVCONFIG036(pDev->V206DEV105.hdmi[2].flags);
	pDev->V206DEV144[MWV206_DP_HDMI_3].V206FB011 = 3;
	pDev->V206DEV144[MWV206_DP_HDMI_3].dualpixel = V206DEVCONFIG037(pDev->V206DEV105.hdmi[3].flags);
	pDev->V206DEV144[MWV206_DP_HDMI_3].enable = V206DEVCONFIG036(pDev->V206DEV105.hdmi[3].flags);
}


static int mwv206_get_chiptype(V206DEV025 *pDev)
{
	 uint32_t chiptype;

	chiptype = V206DEV001(V206DEVCONPARSER004) & 0xff;
	if (chiptype == 0) {
		chiptype = V206DEV001(V206DEVCONPARSER003);
		chiptype = (chiptype >> 24) & 0xff;
	}

	return (int)chiptype;
}

static int vbios_ver_fake;
module_param(vbios_ver_fake, int, 0644);
MODULE_PARM_DESC(vbios_ver_fake, "for debug, use fake vbios version to override real version");

static void mwv206_get_firmware_info(V206DEV025 *pDev)
{
	unsigned int vbios_ver;
	GLJ_TICK tick;

	if (!vbios_ver_fake) {
		vbios_ver = V206DEV001(MAC206HAL216);
		mwv206_timed_loop (tick, vbios_ver == 0xffffffff || VBIOS_VER_MAJOR(vbios_ver) == 0, MAC206HAL223) {
			vbios_ver = V206DEV001(MAC206HAL216);
		}
	} else {
		vbios_ver = vbios_ver_fake;
		V206KDEBUG003("[INFO]: use fake vbios version:0x%x for debug\n", vbios_ver);
	}

	pDev->vbios_ver      = vbios_ver;
	pDev->vbios_major    = VBIOS_VER_MAJOR(vbios_ver);
	pDev->vbios_minor    = VBIOS_VER_MINOR(vbios_ver);
	pDev->vbios_revision = VBIOS_VER_REVISION(vbios_ver);

	V206KDEBUG003("[INFO] vbios version: %d.%d.%d\n", pDev->vbios_major, pDev->vbios_minor, pDev->vbios_revision);

	pDev->chiptype  = mwv206_get_chiptype(pDev);
	V206KDEBUG003("[INFO] chiptype: 0x%x\n", pDev->chiptype);
}

static void FUNC206HAL167(V206DEV025 *pDev)
{
	unsigned int  status;
	int accurate;
	GLJ_TICK tick;


	if (mwv206_vbios_cmp(pDev, 3, 1, 3, &accurate) > 0 || !accurate) {

		status = V206DEV001(VBIOS_STATUS_REG);
		mwv206_timed_loop (tick, ((status >> VBIOS_PHASE4_SHIFT) & 1) == 0, MAC206HAL223) {
			status = V206DEV001(VBIOS_STATUS_REG);
			schedule_timeout_uninterruptible(1);
		}
	}
	V206DEV005("[INFO] vbios version reg = 0x%x, val = 0x%x\n", MAC206HAL216, pDev->vbios_ver);
	V206DEV005("[INFO] vbios status reg = 0x%x, val = 0x%x\n", VBIOS_STATUS_REG, status);
}

static void mwv206_expand_regbar(V206DEV025 *pDev)
{
	int accurate = 0;

	if (mwv206_vbios_cmp(pDev, 3, 3, 0, &accurate) >= 0 && accurate) {

	} else if (!pDev->V206DEV155) {
		FUNC206HAL184(pDev->V206DEV053);
	} else {

	}
}

static void mwv206_read_prj_info(V206DEV025 *pDev)
{
#define SPI_FLASH_VBIOS_PRJ_BASE 0x50020
	jmCfgSpiFlashReadEx(SPI_FLASH_VBIOS_PRJ_BASE, 3, pDev->vbios_prj_str, VBIOS_PRJ_STR_LEN);
#undef SPI_FLASH_VBIOS_PRJ_BASE
}

static int FUNC206HAL002(V206DEV025 *dev,
		int V206DEV072, int corefreq, int memfreq,
		int V206DEV038, int V206DEV039)
{
	int ret, value;
	V206DEV025 *pDev = dev;
	struct mwv206_dev_config  *cfg;


	mwv206_get_firmware_info(pDev);


	value = V206DEV001(0x00400048);
	value = (value >> 16);
	dev->V206DEV035 = value;
	V206DEV005("axi clock %d\n", dev->V206DEV035);


	dev->V206DEV050 = 50;

	dev->V206DEV061 = FUNC206LXDEV118();
	dev->V206DEV060 = FUNC206LXDEV118();
	dev->V206DEV059 = FUNC206LXDEV118();
	dev->V206DEV058 = FUNC206LXDEV118();
	dev->V206DEV056 = FUNC206LXDEV118();
	dev->V206DEV057 = FUNC206LXDEV118();
	dev->V206DEV062 = FUNC206LXDEV118();

	V206DEV005("[INFO] decodelock:0x%lx, \tdevlock:0x%lx\n", dev->V206DEV058, dev->V206DEV055);
	V206DEV005("[INFO] ddr0checklock:0x%lx, \tddrlock:0x%lx\n", dev->V206DEV057, dev->V206DEV056);
	V206DEV005("[INFO] dev:0x%lx, \tprolock:0x%lx\n", dev->V206DEV055, dev->prolock);


	{

		FUNC206HAL167(dev);

		FUNC206HAL170();


		ret = FUNC206HAL166(dev, 0, &dev->V206DEV053);
		if (0 != ret) {
			V206KDEBUG002("[ERROR] jmspiCreate failed(%d)!\n", ret);
			return -7;
		}

		ret = FUNC206HAL166(dev, 1, &dev->V206DEV054);
		if (0 != ret) {
			V206KDEBUG002("[ERROR] jmspiCreate failed(%d)!\n", ret);
			return -8;
		}


		FUNC206HAL171(dev->V206DEV053, 25000);
		FUNC206HAL171(dev->V206DEV054, 25000);


		V206DEV002(0x400034, 1);

		mwv206_expand_regbar(dev);

		FUNC206HAL112(dev->V206DEV053);

		dev->isdevcfgdefault = 0;
		if (FUNC206HAL110(dev, FUNC206HAL111, 0)) {
			if (FUNC206HAL110(dev, FUNC206HAL113, 0)) {
				V206KDEBUG003("[INFO] use default configuration!\n");
				FUNC206HAL110(dev, FUNC206HAL108, 1);
				dev->isdevcfgdefault = 1;
			}
		}
		mwv206_validate_cfg(dev);
		jmVbiosCfgVerGet(dev, jmVbiosCfgVerSpiFlashReader);

		mwv206_read_prj_info(dev);

		cfg = FUNC206HAL109(dev);
		if (cfg == NULL) {
			V206KDEBUG002("[ERROR] failed to get config\n");
			return -1;
		}

		V206DEV072 = cfg->chip.V206DEV072;
		corefreq   = cfg->chip.corefreq;
		V206DEV038   = cfg->chip.V206DEV038;
		V206DEV039   = cfg->chip.V206DEV039;
		memfreq    = cfg->chip.memfreq;
	}

	if (V206DEV038 == 0) {
		V206KDEBUG002("[ERROR] ddr0 size must set!\n");
		return -3;
	}

	FUNC206HAL379(dev);

	ret = FUNC206HAL321(dev);
	if (ret != 0) {
		return -5;
	}
	V206KDEBUG003("[INFO] mwv206 dma init successfully!\n");

	dev->V206DEV049 = 1;

	FUNC206HAL256(dev);

	dev->V206DEV030 = V206DEV009;
	dev->V206DEV046 = 0x7200;
	dev->V206DEV073 = corefreq;
	dev->V206DEV074 = corefreq;
	dev->V206DEV075 = memfreq;
	dev->V206DEV153 = 0;



	FUNC206HAL136(pDev, V206IOCTL005, corefreq);
	V206KDEBUG003("[INFO] mwv206 prim pll inited!\n");
	FUNC206HAL134(pDev, V206IOCTL005, &corefreq);

	FUNC206HAL274(pDev, MWV206_2D_SENDCMDMODE, MWV206_3D_SENDCMDMODE);

	ret = FUNC206HAL315(dev, memfreq);
	if (ret != 0) {
		V206KDEBUG002("[ERROR] mwv206 ddr init failure!\n");
		return -6;
	}

	FUNC206HAL134(pDev, V206IOCTL010, &memfreq);
	dev->V206DEV075 = memfreq;

	V206KDEBUG003("[INFO] corefreq %dMhz, memfreq %dMhz\n", corefreq, memfreq);
	V206KDEBUG003("[INFO] ddr0 %dMbytes, ddr1 %dMbytes\n", V206DEV038, V206DEV039);
	V206KDEBUG003("[INFO] VRAM Size %dMbytes.\n", V206DEV038 + V206DEV039);
	pDev->V206DEV040 = V206DEV038 + V206DEV039;


	if (V206DEV038 > 1024 + 512) {
		V206KDEBUG003("[INFO] ddr0 size:%dMbytes, clamp to 1.5GB\n", V206DEV038);
		pDev->V206DEV105.chip.V206DEV038 = 1024 + 512;
		V206DEV038   = pDev->V206DEV105.chip.V206DEV038;
	}
	if (V206DEV039 > 1024 + 512) {
		V206KDEBUG003("[INFO] ddr1 size:%dMbytes, clamp to 1.5GB\n", V206DEV039);
		pDev->V206DEV105.chip.V206DEV039 = 1024 + 512;
		V206DEV039   = pDev->V206DEV105.chip.V206DEV039;
	}


	FUNC206HAL003(dev);

	ret = FUNC206HAL227(dev, V206DEV072, V206DEV038, V206DEV039);
	if (ret != 0) {
		V206KDEBUG002("[ERROR] mwv206MemInit failed(%d)!\n", ret);
		return -7;
	}

	V206DEV002((0x40024C), 0xffffffff);

	FUNC206HAL351(dev);


	FUNC206HAL156();
	ret = FUNC206HAL151(dev, &dev->V206DEV051);
	if (0 != ret) {
		V206KDEBUG002("[ERROR] jmiicCreate failed(%d)!\n", ret);
		return -9;
	} else {
		V206DEV005("[INFO] jmiicCreate successed, mJmIicHandle=%p\n", dev->V206DEV051);
	}

	FUNC206HAL330(dev);
	FUNC206HAL318(dev);

	FUNC206HAL255(pDev);

	FUNC206HAL385(pDev);

	return 0;
}

struct mem_bar_info {
	unsigned int no;
	unsigned long size;
};

static void FUNC206HAL216(struct pci_dev *V206DEV103)
{
	int             region;
#define MAC206HAL043 32

	for (region = 0; region < MAC206HAL043; region++) {
		PCI_WRITE_U32(V206DEV103, 0x900,
				(1 << 31)
				| (region << 0));
		PCI_WRITE_U32(V206DEV103, 0x908, 0);
	}

}

static void FUNC206HAL234(struct pci_dev *V206DEV103, int region, unsigned long phys, unsigned long size)
{
	PCI_WRITE_U32(V206DEV103, 0x900,
			(1 << 31)
			| (region << 0));

	PCI_WRITE_U32(V206DEV103, 0x90C, (phys) & 0xffffffff);
	PCI_WRITE_U32(V206DEV103, 0x910, (phys) >> 32);
	PCI_WRITE_U32(V206DEV103, 0x914, (phys + size - 1) & 0xffffffff);
}


void FUNC206HAL221(struct pci_dev *V206DEV103)
{
	unsigned long phys;
	unsigned long size;
	int region;
	int bar;
#if defined(__arm__) || defined(__aarch64__)
	V206DEV025 *pDev = pci_get_drvdata(V206DEV103);
#endif


	FUNC206HAL216(V206DEV103);


	for (bar = 0; bar < 7; bar++) {
		region = bar;
		size   = MWV206_GET_PCI_BAR_LEN(V206DEV103, bar);
		phys   = pci_bus_address(V206DEV103, bar);
		if (size == 0) {
			continue;
		}

#if defined(__loongarch__)
	#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)) && (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 5, 0)))


		phys &= ~0xf0000000000;
	#endif
#endif

#if defined(__arm__) || defined(__aarch64__)
		if (V206CTYPE009(pDev->V206DEV028)) {

			phys = MWV206_GET_PCI_BAR_STARTADDR(V206DEV103, bar);
			phys &= ~0x80000000000;
		}
#endif

		FUNC206HAL234(V206DEV103, region, phys, size);
	}
}

int FUNC206HAL217(struct pci_dev *V206DEV103,
		unsigned int *pRegbar, unsigned int *pMem2dbar, unsigned int *pMem3dbar)
{
	unsigned long size;
	unsigned int bar;
	unsigned int V206DEV043;
	struct mem_bar_info V206DEV044[2] = { {0xff, 0}, {0xff, 0} };

	V206DEV043 = 0xff;

	for (bar = 0; bar < DEVICE_COUNT_RESOURCE; bar++) {
		size = (unsigned long)MWV206_GET_PCI_BAR_LEN(V206DEV103, bar);
		if (size == (16 << 20)) {
			V206DEV043 = bar;
		} else if (size == (32 << 20)) {
			V206DEV043 = bar;
		} else if (size > (16 << 20)) {
			if (V206DEV044[0].no == 0xff) {
				V206DEV044[0].no = bar;
				V206DEV044[0].size = size;
			} else {
				V206DEV044[1].no = bar;
				V206DEV044[1].size = size;
			}
		} else {

		}
		if ((V206DEV043 != 0xff) && (V206DEV044[0].no != 0xff) && (V206DEV044[1].no != 0xff)) {
			break;
		}
	}

	if (V206DEV044[0].no == 0xff) {
		V206KDEBUG002("[ERROR] mem bar is missing!!!!");
		return -2;
	}
	if (V206DEV043 == 0xff) {
		V206KDEBUG002("[ERROR] regbar is missing!!!!");
		return -3;
	}
	if (V206DEV044[1].no == 0xff) {
		V206DEV044[1].no = V206DEV044[0].no;
		V206DEV044[1].size = V206DEV044[0].size;
	}

	*pRegbar = V206DEV043;


	if (V206DEV044[0].size > V206DEV044[1].size) {
		*pMem2dbar = V206DEV044[0].no;
		*pMem3dbar = V206DEV044[1].no;
	} else {
		*pMem2dbar = V206DEV044[1].no;
		*pMem3dbar = V206DEV044[0].no;
	}

	V206KDEBUG003("[INFO] regbar: %d, mem2dbar: %d, mem3dbar: %d\n", V206DEV043, *pMem2dbar, *pMem3dbar);
	return 0;
}


int FUNC206HAL148(V206DEV025 *dev)
{
	unsigned long start, size;
	struct pci_dev *V206DEV103;
	int i, ret = 0;
	unsigned int V206DEV043, V206FB006, mem3dbar;

	V206DEV103 = dev->V206DEV103;

	if (V206DEV103 == NULL) {
		return -1;
	}
	dev->V206DEV028 = V206CTYPE013();


	start = (unsigned long)MWV206_GET_PCI_BAR_STARTADDR(V206DEV103, 4);
	size = (unsigned long)MWV206_GET_PCI_BAR_LEN(V206DEV103, 4);

	if (V206CTYPE009(dev->V206DEV028)) {
		start |= 0x80000000000;
	}

	dev->V206DEV034 = (unsigned long)ioremap(start, size);
	if (0 == dev->V206DEV034) {
		V206KDEBUG002("[ERROR] Can't ioremap BAR4\n");
		return -ENOMEM;
	}


	FUNC206HAL221(V206DEV103);

	ret = FUNC206HAL217(V206DEV103, &V206DEV043, &V206FB006, &mem3dbar);
	if (ret != 0) {
		goto FUNC206HAL054;
	}


	size = (unsigned long)MWV206_GET_PCI_BAR_LEN(V206DEV103, V206DEV043);
	if (size == (16 << 20)) {
		dev->V206DEV155 = 0;
	} else if (size == (32 << 20)) {
		dev->V206DEV155 = 1;
	}


	dev->V206DEV043 = V206DEV043;
	dev->V206DEV044[0] = V206FB006;
	dev->V206DEV044[1] = mem3dbar;


	start = (unsigned long)MWV206_GET_PCI_BAR_STARTADDR(V206DEV103, mem3dbar);

	if (V206CTYPE009(dev->V206DEV028)) {
		start |= 0x80000000000;
	}

	size = (unsigned long)MWV206_GET_PCI_BAR_LEN(V206DEV103, mem3dbar);
	V206DEV005("[INFO] MWV206 BAR%d - PHY: 0x%lx, size = 0x%lx\n", mem3dbar, start, size);

	V206DEV005("[INFO] size = 0x%lx\n", size);

	if (size > (2ULL << 30)) {
		size = (2ULL << 30);
	}

	V206DEV005("[INFO] size = 0x%lx\n", size);

	if (V206CTYPE010(dev->V206DEV028) || V206CTYPE009(dev->V206DEV028)
			|| V206CTYPE011(dev->V206DEV028)) {
		V206KDEBUG003("[INFO] wc is true.\n");
		dev->V206DEV145 = 1;
		dev->V206DEV031[1] = (unsigned long)ioremap_wc(start, size);
	} else {
		V206KDEBUG003("[INFO] wc is false.\n");
		dev->V206DEV145 = 0;
		dev->V206DEV031[1] = (unsigned long)ioremap(start, size);
	}

	dev->V206DEV032[1] = size;
	if (0 == dev->V206DEV031[1]) {
		V206KDEBUG002("[ERROR] Can't ioremap BAR%d  base = 0x%lx, size = %ldM\n", mem3dbar, start, size >> 20);
		ret = -ENOMEM;
		goto FUNC206HAL054;
	}
	FUNC206HAL067[FUNC206HAL065][1] = dev->V206DEV031[1];
	V206KDEBUG003("[INFO] MWV206 BAR%d - PHY: 0x%lx, size = %ldM, VIR: 0x%lx\n",
			mem3dbar, start, size >> 20, dev->V206DEV031[1]);


	size = (unsigned long)MWV206_GET_PCI_BAR_LEN(V206DEV103, V206FB006);
	dev->V206DEV032[0] = size;
	V206KDEBUG003("[INFO] MWV206 BAR%d - size = %ldM\n",
			V206FB006, size >> 20);


	start = (unsigned long)MWV206_GET_PCI_BAR_STARTADDR(V206DEV103, V206DEV043);
	size = (unsigned long)MWV206_GET_PCI_BAR_LEN(V206DEV103, V206DEV043);

	if (V206CTYPE009(dev->V206DEV028)) {
		start |= 0x80000000000;
	}


	dev->V206DEV033 = (unsigned long)ioremap(start, size);

	if (0 == dev->V206DEV033) {
		V206KDEBUG002("[ERROR] Can't ioremap BAR%d  base = 0x%lx\n", V206DEV043, start);
		ret = -ENOMEM;
		goto FUNC206HAL055;
	}
	FUNC206HAL068[FUNC206HAL065] = dev->V206DEV033;
	V206KDEBUG003("[INFO] MWV206 BAR%d - PHY: 0x%lx, size = %ldM, VIR: 0x%lx\n", V206DEV043, start, size >> 20, dev->V206DEV033);


	dev->V206DEV055 = FUNC206LXDEV118();
	for (i = 0; i < 4; i++) {
		dev->V206DEV093[i] = 65;
	}


	FUNC206HAL235(dev, V206DEV043, 0x2000000);
	FUNC206HAL235(dev, dev->V206DEV044[1], 0xA0000000);

	dev->V206DEV047 = V206DEV103->bus->number;

	ret = mwv206_intr_init(dev);
	if (ret) {
		goto err_intr_init;
	}
	dev->V206DEV104 = FUNC206LXDEV010(MWV206_MEMACCESS_CHECK_SLICE);
	if (!dev->V206DEV104) {
		V206KDEBUG002("[ERROR] kmemalloc failed!\n");
		ret = -1;
		goto err_intr_init;
	}


	ret = FUNC206HAL002(dev, 0, 0, 0, 0, 0);
	if (ret != 0) {
		V206KDEBUG002("[ERROR] mwv206DevInitConfig failed!\n");
		goto err_intr_init;
	}


	{
		unsigned long decRegBase  = start + V206DECH023;
		V206DEV005("[INFO] decRegBase = 0x%lx\n", decRegBase);
		ret = mwv206dec_init(&dev->V206DEV138, decRegBase,  dev);
		if (0 != ret) {
			V206KDEBUG002("[ERROR] mwv206dec_init failed(%d)!\n", ret);
			goto err_intr_init;
		} else {
			V206DEV005("[INFO] mwv206dec_init successed\n");
		}
	}

	dev->V206DEV029 = FUNC206HAL065;
	FUNC206HAL065++;
	V206KDEBUG003("[INFO] g_boardCnt is:%d!\n", FUNC206HAL065);

	return 0;

err_intr_init:
	iounmap((void *)FUNC206HAL068[FUNC206HAL065]);
FUNC206HAL055:
	iounmap((void *)FUNC206HAL067[FUNC206HAL065][1]);
FUNC206HAL054:
	iounmap((void *)dev->V206DEV034);
	return ret;
}

int FUNC206HAL149(V206DEV025 *pDev)
{

	mwv206_intr_destroy(pDev);


	FUNC206HAL265(pDev);


	FUNC206HAL324(pDev);

	if (pDev->V206DEV080.V206DEV162) {
#ifdef MWV206_RING_USE_DMABUF
		FUNC206LXDEV100(&(pDev->V206DEV103->dev), pDev->V206DEV080.V206DEV164,
				(void *)(uintptr_t)pDev->V206DEV080.V206DEV163, pDev->V206DEV080.V206DEV162);
#else
		FUNC206LXDEV120((void *)(uintptr_t)pDev->V206DEV080.V206DEV163);
#endif
	}


	if (pDev->V206DEV081.startaddr) {
		FUNC206HAL226(pDev, pDev->V206DEV081.startaddr);
	}

	mwv206dec_uninit(pDev->V206DEV138, pDev);

	if (pDev->V206DEV104) {
		FUNC206LXDEV120(pDev->V206DEV104);
	}
	return 0;
}
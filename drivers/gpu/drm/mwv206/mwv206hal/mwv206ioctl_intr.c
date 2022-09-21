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
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/input.h>

#include "gljos.h"
#include "mwv206dev.h"
#include "mwv206reg.h"
#include "mwv206kconfig.h"
#include "mwv206dec.h"
#include "mwv206_hdmiaudio.h"
#define MAC206HAL150      12
#define MAC206HAL151      13
#define MAC206HAL152      14
#define MAC206HAL153      15

DECLARE_WAIT_QUEUE_HEAD(vertical_wait_queue);

static struct input_dev *pinput_dev;

static void FUNC206HAL462(V206DEV025 *pDev, const char *str)
{
	V206KDEBUG002("\n!!!SHENRUIFEN:::%s\n", str);
	V206KDEBUG002("\tEnable: 0x%x:0x%x, 0x%x:0x%x, 0x%x:0x%x, 0x%x:0x%x\n",
			((0x406000) + 0x0014), V206DEV001(((0x406000) + 0x0014)),
			((0x406000) + 0x0018), V206DEV001(((0x406000) + 0x0018)),
			((0x406000) + 0x001c), V206DEV001(((0x406000) + 0x001c)),
			((0x406000) + 0x0020), V206DEV001(((0x406000) + 0x0020)));

	V206KDEBUG002("\tStatus: 0x%x:0x%x, 0x%x:0x%x, 0x%x:0x%x, 0x%x:0x%x\n",
			((0x406000) + 0x0024), V206DEV001(((0x406000) + 0x0024)),
			((0x406000) + 0x0028), V206DEV001(((0x406000) + 0x0028)),
			((0x406000) + 0x002c), V206DEV001(((0x406000) + 0x002c)),
			((0x406000) + 0x0030), V206DEV001(((0x406000) + 0x0030)));

	V206KDEBUG002("\tPending: 0x%x:0x%x, 0x%x:0x%x, 0x%x:0x%x, 0x%x:0x%x\n",
			((0x406000) + 0x0034), V206DEV001(((0x406000) + 0x0034)),
			((0x406000) + 0x0038), V206DEV001(((0x406000) + 0x0038)),
			((0x406000) + 0x003c), V206DEV001(((0x406000) + 0x003c)),
			((0x406000) + 0x0040), V206DEV001(((0x406000) + 0x0040)));


	V206KDEBUG002("\tClear: 0x%x:0x%x, 0x%x:0x%x, 0x%x:0x%x\n\n",
			((0x406000) + 0x0044), V206DEV001(((0x406000) + 0x0044)),
			((0x406000) + 0x0048), V206DEV001(((0x406000) + 0x0048)),
			((0x406000) + 0x004c), V206DEV001(((0x406000) + 0x004c)));
}


static void FUNC206HAL365(V206DEV025 *pDev, int no, int enable)
{
	int oldval, reg, ireg, sno;
	int enablereg[MWV206KINTR_GROUP] = {
		((0x406000) + 0x0014),
		((0x406000) + 0x0018),
		((0x406000) + 0x001c),
	};


	sno = no;


	if (no == MWV206KINTR_CMDPORT_2D) {
		no = MWV206KINTR_CMDPORT;
	}
	if (no < 0 || no >= MWV206KINTR_COUNT) {
		V206KDEBUG002("error interrupt no %d.", no);
		return;
	}

	if (enable < 0 || enable > 1) {
		V206KDEBUG002("error interrupt enable val %d.", enable);
		return;
	}


	pDev->V206DEV067[no] = enable;

	reg = enablereg[no >> 5];
	oldval = V206DEV001(reg);


	no = no & 31;


	if ((oldval & (1 << no)) == (enable << no)) {
		return;
	}


	if (enable) {
		V206DEV005("enable intr %d, enable reg: [ 0x%x]\n\n", sno, reg);

		if (sno == MWV206KINTR_CMDPORT_2D) {

			unsigned int V206IOCTLCMD010;
			ireg = (0x205200);
			V206IOCTLCMD010 = V206DEV001(ireg) | (1 << 0);
			V206DEV002(ireg,  V206IOCTLCMD010);
			while (V206DEV001(ireg) != V206IOCTLCMD010)
				;




			V206DEV002(((0x406000) + 0x0044), (1 << no));




			V206IOCTLCMD010 = oldval | (1 << no);
			V206DEV002(reg, V206IOCTLCMD010);

		} else if (sno == MWV206KINTR_CMDPORT) {





			V206DEV002(((0x406000) + 0x0044), (1 << no));
			FUNC206HAL231(pDev, (0x205204),
					1 << 1, 1 << 1);

			V206DEV002(reg, oldval | (1 << no));
		} else if (sno == MWV206KINTR_RENDER) {


			V206DEV002(((0x406000) + 0x0044), (1 << no));
			FUNC206HAL231(pDev, ((0x406000) + 0x0024), 1 << no, 0);

			V206DEV002(reg, oldval | (1 << no));
		} else if (sno >= MWV206KINTR_DISPLAY0 && sno <= MWV206KINTR_DISPLAY3) {

			ireg = ((0x409000) + 0x0600);
			V206DEV002(reg, oldval | (1 << no));

			V206DEV002(ireg, V206DEV001(ireg) | (1 << (sno - 2)));
		} else if (sno == MWV206KINTR_GRAPH_ABNORMAL) {

			V206DEV002(reg, oldval | (1 << no));

			ireg = ((0x409000) + 0x0600);
			V206DEV002(ireg, V206DEV001(ireg) | (1 << (3)));
		} else if (sno == MWV206KINTR_PCIEX16_DMA) {

			V206DEV002(reg, oldval | (1 << no));

			V206DEV002(0xC000A8, 0x0);
		} else {

			V206DEV002(reg, oldval | (1 << no));
		}

	} else {
		V206DEV005("disable intr %d, disable reg: [ 0x%x]\n\n", sno, reg);

		if (sno == MWV206KINTR_CMDPORT_2D) {

			ireg = (0x205200);
			V206DEV002(ireg, V206DEV001(ireg) & ~(1 << 0));
		} else if (sno == MWV206KINTR_CMDPORT) {




		} else if (sno >= MWV206KINTR_DISPLAY0 && sno <= MWV206KINTR_DISPLAY3) {

			ireg = ((0x409000) + 0x0600);
			V206DEV002(ireg, V206DEV001(ireg) & ~(1 << (no - 2)));
		} else if (sno == MWV206KINTR_GRAPH_ABNORMAL) {

			ireg = ((0x409000) + 0x0600);
			V206DEV002(ireg, V206DEV001(ireg) & ~(1 << (3)));
		} else if (sno == MWV206KINTR_PCIEX16_DMA) {

			V206DEV002(0xC000A8, 0xFF);
		}


		V206DEV002(reg, oldval & ~(1 << no));
	}

	return;
}

int FUNC206HAL331(V206DEV025 *pDev, long arg)
{
	int enable, no;

	V206DEV005("[INFO] mwv206_enableintr arg %lx.\n", arg);
	if (pDev->irq < 0 || pDev->irq == 0xff) {
		return 0;
	}
	enable = (arg & 0x80000000) ? 1 : 0;
	no = arg & 0x7fffffff;

	FUNC206HAL369(pDev, 1);


	FUNC206HAL365(pDev, no, enable);

	FUNC206HAL369(pDev, 0);
	return 0;
}

int FUNC206HAL368(V206DEV025 *pDev, int no)
{
#if 0
	int val;
	int enablereg[MWV206KINTR_GROUP] = {
		((0x406000) + 0x0014),
		((0x406000) + 0x0018),
		((0x406000) + 0x001c),
	};

	if (no < 0 || no >= MWV206KINTR_COUNT) {
		V206KDEBUG002("error interrupt no %d.", no);
		return 0;
	}

	val = V206DEV001(enablereg[no >> 5]);
	return (val & (1 << (no & 31))) ? 1 : 0;
#else
	return pDev->V206DEV067[no];
#endif
}

#define MWV206KINTR_ROUTE_MASK        0x3
#define MWV206KINTR_ROUTE_PCIE_MASK   0x3
int FUNC206HAL367(V206DEV025 *pDev, int no, int select)
{
	int oldval, reg, bit;
	int routereg[MWV206KINTR_GROUP * 2] = {
		((0x406000) + 0x0050), ((0x406000) + 0x0054),
		((0x406000) + 0x0058), ((0x406000) + 0x005c),
		((0x406000) + 0x0060), ((0x406000) + 0x0064),
	};

	reg = routereg[no >> 4];
	bit = (no & 15) * 2;
	oldval = V206DEV001(reg);
	V206DEV002(reg, (oldval & (~(MWV206KINTR_ROUTE_MASK << bit)))
			| (select << bit));
	return 0;
}


int FUNC206HAL366(V206DEV025 *pDev, int no, int select)
{
	int oldval, reg, bit;
	int pciereg[MWV206KINTR_GROUP * 2] = {
		((0x406000) + 0x00B8), ((0x406000) + 0x00BC),
		((0x406000) + 0x00C0), ((0x406000) + 0x00C4),
		((0x406000) + 0x00C8), ((0x406000) + 0x00CC),
	};

	reg = pciereg[no >> 4];
	bit = (no & 15) * 2;
	oldval = V206DEV001(reg);
	V206DEV002(reg, (oldval & (~(MWV206KINTR_ROUTE_PCIE_MASK << bit)))
			| (select << bit));

	return 0;
}


static int FUNC206HAL104;
static int FUNC206HAL105;
static int FUNC206HAL354(int intrsrc, V206DEV025 *pDev)
{
	FUNC206HAL104++;
	V206KDEBUG002("\nFrame %d ended!\n", FUNC206HAL104);
	return IRQ_HANDLED;
}

static int FUNC206HAL358(int intrsrc, V206DEV025 *pDev)
{
	FUNC206HAL105++;
	V206KDEBUG002("\ngraph abnormal %d!\n", FUNC206HAL105);
	return IRQ_HANDLED;
}

static int FUNC206HAL356(int intrsrc, V206DEV025 *pDev)
{
	return mwv206dec_decoder_isr(pDev->V206DEV138, pDev);
}

static int FUNC206HAL360(int intrsrc, V206DEV025 *pDev)
{
	V206DEV005("\n\nintrsrc: %d\n\n", intrsrc);
	FUNC206LXDEV103(pDev->V206DEV064[intrsrc]);
	return IRQ_WAKE_THREAD;
}

static int FUNC206HAL353(int intrsrc, V206DEV025 *pDev)
{
	int status, enable, val, pending, flag, scratch;
	GLJ_TICK tick;


	enable = V206DEV001((0x205200));
	status = V206DEV001((0x205204));


	val = 1 << MWV206KINTR_CMDPORT;

	V206DEV002(((0x406000) + 0x0044), val);


	mwv206_timed_do (tick, scratch, MWV206CMDTIMEOUT) {
		pending = V206DEV001(((0x406000) + 0x0024));
		if (((pending & val) == 0)) {
			flag = 1;
			break;
		} else {
			V206DEV002(((0x406000) + 0x0044), val);
		}
	}

	if (flag == 0) {
		V206KDEBUG002("\n\nMwv206Interrupt: clear cmdport interrupt error!!!\n"
				"pending register = [0x%x]! \n\n\n\n", pending);
		return 0;
	}


	if ((enable & (1 << 0)) && !(status & (1 << 0))) {
		V206DEV005("\n\n2d cmdport intr, enable 0x%x, status = 0x%x\n\n", enable, status);

		{
			FUNC206LXDEV103(pDev->V206DEV064[MWV206KINTR_CMDPORT_2D]);
		}
	} else if ((enable & (1 << 1)) && !(status & (1 << 1))) {
		V206DEV005("\n\n3d cmdport intr, enable 0x%x, status = 0x%x\n\n", enable, status);
		FUNC206LXDEV103(pDev->V206DEV064[MWV206KINTR_CMDPORT]);
	} else {
		V206DEV005("unexpected interrupt, status = 0x%x, enable = 0x%x.\n",
				status, enable);
	}
	return IRQ_HANDLED;
}

static void FUNC206HAL016(V206DEV025 *pDev, int start, int end)
{
	int i;
	int dointr;

	for (i = start; i < end; i++) {
		if (pDev->V206DEV070[i].type == 0) {
			continue;
		}
		dointr = pDev->V206DEV066[i >> 5] & (1 << (i & 31));
		if (dointr == 0) {
			continue;
		}
		V206DEV005("interrupt %d comming\n\n", i);

		pDev->V206DEV066[i >> 5] &= ~(1 << (i & 31));
		if (0 == pDev->V206DEV070[i].param) {
			pDev->V206DEV070[i].V206DEV070(i, pDev);
		} else {
			pDev->V206DEV070[i].V206DEV070(i, (void *)pDev->V206DEV070[i].param);
		}
	}
}

static int FUNC206HAL015(int irq, V206DEV025 *pDev)
{
	int flag = 0, scratch;
	GLJ_TICK tick;
	unsigned int status[3], clrlater;
	unsigned int pending[3] = {0};


	V206DEV005("Mwv206Interrupt started!\n");

	pDev->V206DEV066[0] |= V206DEV001(((0x406000) + 0x0034));
	pDev->V206DEV066[1] |= V206DEV001(((0x406000) + 0x0038));
	pDev->V206DEV066[2] |= V206DEV001(((0x406000) + 0x003c));

	status[0] = pDev->V206DEV066[0];
	status[1] = pDev->V206DEV066[1];
	status[2] = pDev->V206DEV066[2];

	V206DEV005("Mwv206Interrupt =[0x%x, 0x%x, 0x%x]!\n",
			status[0], status[1], status[2]);
	if (status[0] & (1 << MWV206KINTR_DISPLAY0)) {
		if (pinput_dev) {
			input_report_abs(pinput_dev, ABS_X, 2);
			input_sync(pinput_dev);
		}
	}
	if (status[0] & (1 << MWV206KINTR_DISPLAY1)) {
		if (pinput_dev) {
			input_report_abs(pinput_dev, ABS_X, 2);
			input_sync(pinput_dev);
		}
	}

#if 0

	enable[0] = V206DEV001(((0x406000) + 0x0014));
	enable[1] = V206DEV001(((0x406000) + 0x0018));
	enable[2] = V206DEV001(((0x406000) + 0x001c));


	V206DEV002(((0x406000) + 0x0014), enable[0] & ~(status[0]));
	V206DEV002(((0x406000) + 0x0018), enable[1] & ~(status[1]));
	V206DEV002(((0x406000) + 0x001c), enable[2] & ~(status[2]));
#endif

	clrlater = (1 << MWV206KINTR_PCIEX16_DMA) | (1 << MWV206KINTR_CMDPORT) | (1 << MWV206KINTR_HD_DECODE);
	V206DEV002(((0x406000) + 0x0044), status[0] & ~clrlater);
	V206DEV002(((0x406000) + 0x0048), status[1]);
	V206DEV002(((0x406000) + 0x004c), status[2]);

	mwv206_timed_do (tick, scratch, MWV206CMDTIMEOUT) {
		pending[0] = V206DEV001(((0x406000) + 0x0034));
		pending[1] = V206DEV001(((0x406000) + 0x0038));
		pending[2] = V206DEV001(((0x406000) + 0x003c));
		if (((pending[0] & status[0] & ~clrlater) == 0)
				&& ((pending[1] & status[1]) == 0)
				&& ((pending[2] & status[2]) == 0)) {
			flag = 1;
			break;
		}
	}

	if (flag == 0) {
		V206KDEBUG002("\n\nMwv206Interrupt: clear interrupt error!!!\n"
				"pending register = [0x%x, 0x%x, 0x%x]! \n\n\n\n",
				pending[0], pending[1], pending[2]);
		return 0;
	}


	if (status[0] != 0) {
		FUNC206HAL016(pDev, 0, 32);
	}
	if (status[1] != 0) {
		FUNC206HAL016(pDev, 32, 64);
	}
	if (status[2] != 0) {
		FUNC206HAL016(pDev, 64, MWV206KINTR_COUNT);
	}

#if 0

	V206DEV002(((0x406000) + 0x0014), enable[0]);
	V206DEV002(((0x406000) + 0x0018), enable[1]);
	V206DEV002(((0x406000) + 0x001c), enable[2]);
#endif
	return IRQ_HANDLED;
}

irqreturn_t FUNC206HAL364(int irq, void *devInfo)
{
	V206DEV025 *pDev;

	pDev = (V206DEV025 *)devInfo;

	if ((V206DEV001(((0x406000) + 0x0024)) == 0)
		&& (V206DEV001(((0x406000) + 0x0028)) == 0)
		&& (V206DEV001(((0x406000) + 0x002c)) == 0)
		&& (V206DEV001(((0x406000) + 0x0030)) == 0)) {
		return IRQ_NONE;
	}

	V206DEV005("mwv206_interrupt func start: time is 0x%lx\n", FUNC206LXDEV134());
	tasklet_schedule(&pDev->V206DEV154);

	V206DEV005("mwv206_interrupt func ended!\n");

	return IRQ_HANDLED;
}

void FUNC206HAL363(unsigned long arg)
{
	V206DEV025 *pDev = (V206DEV025 *)arg;
	FUNC206HAL015(0, pDev);
}

static int FUNC206HAL362(V206DEV025 *pDev)
{
	unsigned int i;
	int defaultTakenIntrList[] = {
		MWV206KINTR_CMDPORT,
		MWV206KINTR_DISPLAY0, MWV206KINTR_DISPLAY1,
		MWV206KINTR_DISPLAY2, MWV206KINTR_DISPLAY3
	};


	for (i = 0; i < MWV206KINTR_COUNT + 9; i++) {
		FUNC206LXDEV104(pDev->V206DEV064[i]);
	}


	for (i = 0; i < sizeof(defaultTakenIntrList) / sizeof(int); i++) {
		FUNC206LXDEV103(pDev->V206DEV064[defaultTakenIntrList[i]]);
	}


	V206DEV002(((0x406000) + 0x0014), 0);
	V206DEV002(((0x406000) + 0x0018), 0);
	V206DEV002(((0x406000) + 0x001c), 0);
	V206DEV002(((0x406000) + 0x0020), 0);

	return 0;
}


void FUNC206HAL355(V206DEV025 *pDev)
{
	int id;
	unsigned int i;

	int raiseEventIntrList[] = {
		MWV206KINTR_RENDER,
		MWV206KINTR_DISPLAY0, MWV206KINTR_DISPLAY1,
		MWV206KINTR_DISPLAY2, MWV206KINTR_DISPLAY3,
		MWV206KINTR_JJWLINK_MASTER, MWV206KINTR_JJWLINK_SLAVE
	};



	for (i = 0; i < MWV206KINTR_COUNT + 9; i++) {
		pDev->V206DEV064[i] = FUNC206LXDEV101();
	}


	for (i = 0; i < sizeof(raiseEventIntrList) / sizeof(int); i++) {
		id = raiseEventIntrList[i];
		pDev->V206DEV070[id].type = 1;
		pDev->V206DEV070[id].param = 0;
		pDev->V206DEV070[id].V206DEV070 = (MWV206INTRFUNC)FUNC206HAL360;
	}

	pDev->V206DEV070[MWV206KINTR_CMDPORT].type = 1;
	pDev->V206DEV070[MWV206KINTR_CMDPORT].param = 0;
	pDev->V206DEV070[MWV206KINTR_CMDPORT].V206DEV070 = (MWV206INTRFUNC)FUNC206HAL353;


	pDev->V206DEV070[MWV206KINTR_CMDSTATUS].type = 1;
	pDev->V206DEV070[MWV206KINTR_CMDSTATUS].param = 0;
	pDev->V206DEV070[MWV206KINTR_CMDSTATUS].V206DEV070 = (MWV206INTRFUNC)FUNC206HAL354;

	pDev->V206DEV070[MWV206KINTR_GRAPH_ABNORMAL].type = 1;
	pDev->V206DEV070[MWV206KINTR_GRAPH_ABNORMAL].param = 0;
	pDev->V206DEV070[MWV206KINTR_GRAPH_ABNORMAL].V206DEV070 = (MWV206INTRFUNC)FUNC206HAL358;

	pDev->V206DEV070[MWV206KINTR_PCIEX16_DMA].type = 1;
	pDev->V206DEV070[MWV206KINTR_PCIEX16_DMA].param = 0;
	pDev->V206DEV070[MWV206KINTR_PCIEX16_DMA].V206DEV070 = (MWV206INTRFUNC)FUNC206HAL359;

	pDev->V206DEV070[MWV206KINTR_HD_DECODE].type = 1;
	pDev->V206DEV070[MWV206KINTR_HD_DECODE].param = 0;
	pDev->V206DEV070[MWV206KINTR_HD_DECODE].V206DEV070 = (MWV206INTRFUNC)FUNC206HAL356;

	V206DEV005("init create.\n\n");
}

static void FUNC206HAL357(V206DEV025 *pDev)
{
	int i;
	V206KDEBUG002("interrupt event destroy.\n");
	for (i = 0; i < MWV206KINTR_COUNT + 8; i++) {
		FUNC206LXDEV102(pDev->V206DEV064[i]);
	}
}

int FUNC206HAL395(V206DEV025 *pDev, long arg)
{
	arg ? (pDev->V206DEV049 = 1) : (pDev->V206DEV049 = 0);
	return 0;
}

static int FUNC206HAL407(V206DEV025 *pDev, int display)
{
	int dis_status, scratch;
	GLJ_TICK tick;
	int timeout = MWV206CMDTIMEOUT;
	int offset = 0;

	if (display == 0) {
		offset = 0;
	} else if (display == 1) {
		offset = 0x100;
	} else if (display == 2) {
		offset = 0x800;
	} else if (display == 3) {
		offset = 0x900;
	} else {
		V206KDEBUG002("error display number %d.", display);
		return -1;
	}

	mwv206_timed_do (tick, scratch, timeout) {
		dis_status = V206DEV001(((0x409000) + 0x04AC) + offset);
		if (dis_status == 1) {
			V206DEV002((((0x409000) + 0x04AC) + offset), 0);
			V206DEV005("wait for display polling %d success:\n", display);
			return 0;
		}
	}
	V206KDEBUG002("wait for display polling %d timeout:\n", display);
	return -2;
}

static int FUNC206HAL406(V206DEV025 *pDev, int display)
{
	V206DEV005("wait for display %d interrupt:\n", display);

	if (FUNC206LXDEV105(pDev->V206DEV064[MWV206KINTR_DISPLAY0 + display], MWV206CMDTIMEOUT) == 0) {
		V206DEV005("wait for display-intr-%d successfully!\n", display);
		return 0;
	} else {
		V206KDEBUG002("wait for display-intr-%d: timeout!\n", display);
		return -1;
	}
}

int FUNC206HAL405(V206DEV025 *pDev, int display)
{
	int ret;

	if (!pDev->V206DEV049) {
		return 0;
	}

	V206DEV005("wait for display\n");

	if (FUNC206HAL368(pDev, MWV206KINTR_DISPLAY0 + display)) {
		V206DEV005("use display-intr!\n\n\n");
		ret =  FUNC206HAL406(pDev, display);
	} else {
		V206DEV005("use display-polling!\n\n\n");
		ret = FUNC206HAL407(pDev, display);
	}

	return ret;
}

void FUNC206HAL361(void)
{
	if (pinput_dev != NULL) {
		input_unregister_device(pinput_dev);
		pinput_dev = NULL;
	}
}

int FUNC206HAL408(V206DEV025 *pDev, long arg)
{
	static int ret;
	if (!ret) {
		pinput_dev = input_allocate_device();
		if (!pinput_dev) {
			V206KDEBUG002("error input allocate device");
			return -1;
		}
		pinput_dev->name = "jjInput";
		pinput_dev->phys = "jjinput/input0";
		pinput_dev->id.bustype = BUS_HOST;
		ret = input_register_device(pinput_dev);
		V206DEV005("ret_tf %d\n", ret);
		__set_bit(EV_SYN, pinput_dev->evbit);
		__set_bit(EV_ABS, pinput_dev->evbit);
		__set_bit (ABS_X, pinput_dev->absbit);

		ret = 1;
	}
	return 0;
}

static void FUNC206HAL383(void)
{
	FUNC206HAL105 = 0;
	FUNC206HAL104 = 0;
}

int mwv206_intr_init(V206DEV025 *pDev)
{
	int msi_on = 0;
	int ret = 0;

	if (pci_msi_enabled()) {
		if (pci_enable_msi(pDev->V206DEV103)) {
			V206KDEBUG003("[INFO] msi not available");
		} else {
			msi_on = 1;
			V206KDEBUG003("[INFO] msi on");
		}
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
	pDev->irq = pci_irq_vector(pDev->V206DEV103, 0);
#else
	pDev->irq = pDev->V206DEV103->irq;
#endif
	V206KDEBUG003("[INFO] irq number: %d", pDev->irq);

	if (pDev->irq >= 0 && pDev->irq != 0xff) {
		FUNC206HAL355(pDev);
		ret = request_irq(pDev->irq, FUNC206HAL364, IRQF_SHARED, "MWV206", (void *)pDev);
		if (ret) {
			V206KDEBUG002("[ERROR] request_irq (%d) failed, result = %d\n", pDev->irq, ret);
			goto FUNC206HAL056;
		}

		tasklet_init(&pDev->V206DEV154, FUNC206HAL363, (unsigned long)pDev);


		FUNC206HAL362(pDev);
	} else {
		V206DEV002(((0x406000) + 0x0014), 0);
		V206DEV002(((0x406000) + 0x0018), 0);
		V206DEV002(((0x406000) + 0x001c), 0);
		V206DEV002(((0x406000) + 0x0020), 0);
	}

	FUNC206HAL383();

	return ret;

FUNC206HAL056:
	FUNC206HAL357(pDev);
	if (msi_on) {
		pci_disable_msi(pDev->V206DEV103);
	}
	return ret;
}

void mwv206_intr_destroy(V206DEV025 *pDev)
{
	if (pDev->irq >= 0 && pDev->irq != 0xff) {

		disable_irq(pDev->irq);

		free_irq(pDev->irq, (void *)pDev);
		FUNC206HAL357(pDev);
	}
	if (pci_msi_enabled()) {

		pci_disable_msi(pDev->V206DEV103);
	}
}
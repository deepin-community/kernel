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
#include "mwv206dev.h"
#include "mwv206oshal.h"
#include "mwv206reg.h"
#include "mwv206kdebug.h"

#define MAC206HAL024          0
#define MAC206HAL025        1


#define MAC206HAL026        MAC206HAL025


#if (MAC206HAL026 == MAC206HAL024)

#define MAC206HAL006(result) do {\
	if (0 != result) {\
	V206DEV005("DDR config failed at %d\n", __LINE__);\
	\
	} \
} while (0)

#define MAC206HAL007(result) do {\
	if (0 != result) {\
	V206DEV005("DDR detect failed at %d\n", __LINE__);\
	\
	} \
} while (0)

#define MAC206HAL008(result) do {\
	if (0 != result) {\
	V206DEV005("DDR read failed at %d\n", __LINE__);\
	\
	} \
} while (0)
#else




#define MAC206HAL006(result)
#define MAC206HAL007(result)
#define MAC206HAL008(result)

#endif



#define MAC206HAL049            0x00


#define MAC206HAL050          0x04



#define MAC206HAL051           0x08


#define MAC206HAL052           0x0C



#define MAC206HAL053           0x40A000


#define MAC206HAL054           0x1000



#define MAC206HAL058              2000000



#define MAC206HAL055                   0
#define MAC206HAL056                         1
#define MAC206HAL057                      2


static void FUNC206HAL051(void *devInfo, unsigned int addr, unsigned int value)
{

	V206DEV006(devInfo, addr, value);
}



static unsigned int FUNC206HAL050(void *devInfo, unsigned int addr)
{
	unsigned int value;

	value = V206DEV007(devInfo, addr);

	return value;
}



static int FUNC206HAL053(void *devInfo, int index, int object, unsigned short addr, unsigned int data)
{
	unsigned int value, readCount;
	unsigned int tickStart;
	unsigned int regAddr, dataRegAddr;
	unsigned int writeData = 0;

	if (MAC206HAL056 == object) {
		writeData |= 1 << 20;
	}
	writeData |= (addr & 0x3FF);
	regAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL050;
	FUNC206HAL051(devInfo, regAddr, writeData);

	regAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL049;
	dataRegAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL052;

	tickStart = 0;
	while (1) {
		readCount = 0;
		FUNC206HAL051(devInfo, regAddr, 1);

		while (1) {
			value = FUNC206HAL050(devInfo, regAddr);
			if (0 == value) {
				break;
			}

			readCount += 1;
#if 1

			if (5 == readCount) {
				V206KDEBUG002("[ERROR] write reg 0x%x done failure.\n", addr);
				goto readstep;
			}
#endif
		}

readstep:
		value = FUNC206HAL050(devInfo, dataRegAddr);
		value = FUNC206HAL050(devInfo, dataRegAddr);
		if (data == value) {
			return 0;
		}

		tickStart += 1;


		if (tickStart > MAC206HAL058) {
			break;
		}

		FUNC206LXDEV128(2);
	}



	V206KDEBUG002("[ERROR] write reg 0x%x failure(val: 0x%x, expected value 0x%x).\n", addr, value, data);
	return 1;
}



#include "videoadd_reg.h"
static int FUNC206HAL042(void *devInfo, int index, int object, unsigned short addr, unsigned int data)
{
	unsigned int value;
	unsigned int readCount;
	unsigned int regAddr;
	unsigned int writeData = 1 << 16;

	if (MAC206HAL056 == object) {
		writeData |= 1 << 20;
	}

	writeData |= (addr & 0x3FF);
	regAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL050;
	FUNC206HAL051(devInfo, regAddr, writeData);

	regAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL051;
	FUNC206HAL051(devInfo, regAddr, data);

	regAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL049;
	FUNC206HAL051(devInfo, regAddr, 1);

	readCount = 0;

	while (1) {
		value = FUNC206HAL050(devInfo, regAddr);
		if (0 == value) {
			break;
		}

		readCount += 1;
#if 1

		if (5 == readCount) {
			int hvalue;
			V206KDEBUG002("[ERROR]:try time 5,write reg 0x%x done failure[0x%08x].\n", addr, value);

			FUNC206HAL051(devInfo, 0x400C44, 5);
			hvalue = FUNC206HAL050(devInfo, 0x400CB0);
			V206KDEBUG002("[ERROR] DDR lock state: 0x%08x(1-locked)\n", hvalue);

			FUNC206HAL051(devInfo, 0x400C54, 5);
			hvalue = FUNC206HAL050(devInfo, 0x400C54);
			V206KDEBUG002("[ERROR] bPll_Clkout_sel: 0x%08x(5-ddr)\n", hvalue);


			return 1;
		}
#endif
	}

	return 0;
}



static int FUNC206HAL463(void *devInfo, int index, int object, unsigned short addr, unsigned int *pRegValue)
{
	unsigned int value, readCount;
	unsigned int regAddr, dataRegAddr;
	unsigned int writeData = 0;

	if (MAC206HAL056 == object) {
		writeData |= 1 << 20;
	}
	writeData |= (addr & 0x3FF);
	regAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL050;
	FUNC206HAL051(devInfo, regAddr, writeData);

	regAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL049;
	dataRegAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL052;
	FUNC206HAL051(devInfo, regAddr, 1);

	readCount = 0;
	while (1) {
		value = FUNC206HAL050(devInfo, regAddr);
		if (0 == value) {
			break;
		}

		readCount += 1;
#if 0

		if (2 == readCount) {
			V206KDEBUG002("[ERROR] write reg 0x%x done failure.\n", addr);
			return 1;
		}
#endif
	}

	value = FUNC206HAL050(devInfo, dataRegAddr);
	value = FUNC206HAL050(devInfo, dataRegAddr);
	if (NULL != pRegValue) {
		*pRegValue = value;
	}

	return 0;
}



static int FUNC206HAL043(void *devInfo, int index)
{
	int result = 0;
	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;
	unsigned int ddrcontroler;
	unsigned int ddrfreq = pDeviceData->V206DEV075;
	switch (ddrfreq) {
	case 533:
		ddrcontroler = 534;
		break;
	case 465:
		ddrcontroler = 534;
		break;
	case 400:
		ddrcontroler = 401;
		break;
	case 266:
		ddrcontroler = 267;
		break;
	default:
		ddrcontroler = 0;
		V206KDEBUG002("[ERROR] unrecognized ddr freq %d!\n", ddrfreq);
		return 1;
		break;
	}



	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xC0, ddrcontroler);

#ifdef FAST_SIM
	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xC4, 1);
#else
	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xC4, 200);
#endif


#ifdef FAST_SIM
	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xC8, 1);
#else
	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xC8, 500);
#endif


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xCC, (ddrfreq + 9) / 10);

	return result;
}



static int FUNC206HAL040(void *devInfo, int index)
{
	int result = 0;




	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x7C, 0x0);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x80, 0x20021);

	return result;
}



static int FUNC206HAL102(void *devInfo, int index)
{
	int result = 0;
	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;
	unsigned int pllcr, ptr0, ptr1, ptr3, ptr3fast, ptr4fast, ptr4;
	unsigned int dtpr0, dtpr1, mr0, mr2;
	unsigned int ddrfreq = pDeviceData->V206DEV075;
	switch (ddrfreq) {
	case 533:
		pllcr = 0x0001c000;
		ptr0 = 16 | 1067 << 6 | 267 << 21 ;
		ptr1 = 1067 | 26667 << 16;
		ptr3fast = 5334 | 320 << 20;
		ptr3 = 533334 | 320 << 20;
		ptr4fast = 2134 | 683 << 18;
		ptr4 = 213334 | 683 << 18;
		dtpr0 = 0xc9e4ee88;
		dtpr1 = 0x2288b4d0;
		mr0 = 0x00000024;
		mr2 = 0x00000268;
		break;
	case 465:
		pllcr = 0x0001c000;
		ptr0 = 16 | 1067 << 6 | 267 << 21 ;
		ptr1 = 1067 | 26667 << 16;
		ptr3fast = 5334 | 320 << 20;
		ptr3 = 533334 | 320 << 20;
		ptr4fast = 2134 | 683 << 18;
		ptr4 = 213334 | 683 << 18;
		dtpr0 = 0xc9e4ee88;
		dtpr1 = 0x2288b4d0;
		mr0 = 0x00000024;
		mr2 = 0x00000268;
		break;
	case 400:
		pllcr = 0x0001c000;
		ptr0 = 16 |  800 << 6 | 200 << 21;
		ptr1 =  800 | 20000 << 16;
		ptr3fast = 4000 | 240 << 20;
		ptr3 = 400000 | 240 << 20;
		ptr4fast = 1600 | 512 << 18;
		ptr4 = 160000 | 512 << 18;
		dtpr0 = 0x995bbb66;
		dtpr1 = 0x22857380;
		mr0 = 0x00001c70;
		mr2 = 0x00000218;
		break;
	case 266:
		pllcr = 0x0005c000;
		ptr0 = 16 |  534 << 6 | 134 << 21;
		ptr1 = 534 | 13334 << 16;
		ptr3fast = 2667 | 160 << 20;
		ptr3 = 266667 | 160 << 20;
		ptr4fast = 1067 | 512 << 18;
		ptr4 = 106667 | 512 << 18;
		dtpr0 = 0x65127744;
		dtpr1 = 0x22845a60;
		mr0 = 0x00001830;
		mr2 = 0x00000208;
		break;
	default:
		pllcr = 0;
		ptr0 = 0;
		ptr1 = 0;
		ptr3fast = 0;
		ptr3 = 0;
		ptr4fast = 0;
		ptr4 = 0;
		dtpr0 = 0;
		dtpr1 = 0;
		mr0 = 0;
		mr2 = 0;
		V206KDEBUG002("[ERROR] unrecognized ddr freq!\n");
		return 1;
		break;
	}




	result = FUNC206HAL053(devInfo, index, MAC206HAL056, 0, 0x00130318);

	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x03, 0x0300c4e1);


	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x06, pllcr);


	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x07, ptr0);


	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x08,  ptr1);




#ifdef FAST_SIM
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x0A, ptr3fast);
#else
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x0A, ptr3);
#endif


#ifdef FAST_SIM
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x0B, ptr4fast);
#else
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x0B, ptr4);
#endif














#ifdef PRE_SYN_SIM
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x10, 0xf000641f);
#else
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x10, 0xf004649f);
#endif


	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x12, dtpr0);


	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x13, dtpr1);


	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x15, mr0);


	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x16, 0x00000004);


	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x17, mr2);


	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x18, 0);

	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x1A, 0x910035D7);


	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x1B, 0xF00F00);
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x1C, 0xF00F08);
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x1D, 0xF00F10);
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x1E, 0xF00F18);

	V206DEV005("ddr conf %d.\n", pDeviceData->V206DEV105.chip.memconf);



	V206DEV005("DDR_MWG6439.\n");

	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0xe, 0x3443c8f6);
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0xf, 0x44181885);
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x70, 0x7c000e87);
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x80, 0x7c000e87);
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x90, 0x7c000e87);
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0xa0, 0x7c000e87);
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x16, 0x6);
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x19, 0x1ffff);






	result = FUNC206HAL053(devInfo, index, MAC206HAL056, 0x4, 0x9000000F);

	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x1, 0x81);
	result = FUNC206HAL053(devInfo, index, MAC206HAL056, 0x1, 0x0);
	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x1, 0x101);
	result = FUNC206HAL053(devInfo, index, MAC206HAL056, 0x1, 0x0);
	result = FUNC206HAL053(devInfo, index, MAC206HAL056, 0x4, 0x9000001F);



	return result;
}



static int FUNC206HAL069(void *devInfo, int index)
{
	int result = 0;


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x2C4, 0x0);


	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x2C0, 0x1);

	return result;
}



static int FUNC206HAL101(void *devInfo, int index)
{
	int result = 0;


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x044, 0x1);


	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x048, 0x1);

	return result;
}



static int FUNC206HAL044(void *devInfo, int index)
{
	int result = 0;
	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;
	unsigned int tmfc, trp, tcl, tcwl, tras, trc, trcd, trrd, trefi;
	unsigned int trtp, twr, twtr, tzqzs, tcksre, tcksrx, tmod, tzqcl;
	unsigned int ddrfreq = pDeviceData->V206DEV075;
	switch (ddrfreq) {
	case 533:
		tmfc = 278; trp = 14; tcl = 14;
		tcwl = 10; tras = 36; trc = 50;
		trcd = 14; trrd = 7; trtp = 8;
		twr = 16; twtr = 8; tzqzs = 86;
		tcksre = 11; tcksrx = 11; tmod = 16;
		tzqcl = 683; trefi = 8315;
		break;
	case 465:
		tmfc = 278; trp = 14; tcl = 14;
		tcwl = 10; tras = 36; trc = 50;
		trcd = 14; trrd = 7; trtp = 8;
		twr = 16; twtr = 8; tzqzs = 86;
		tcksre = 11; tcksrx = 11; tmod = 16;
		tzqcl = 683; trefi = 8315;
		break;
	case 400:
		tmfc = 174; trp = 11; tcl = 11;
		tcwl = 8; tras = 27; trc = 38;
		trcd = 11; trrd = 5; trtp = 6;
		twr = 12; twtr = 6; tzqzs = 64;
		tcksre = 8; tcksrx = 8; tmod = 12;
		tzqcl = 512; trefi = 6240;
		break;
	case 266:
		tmfc = 139; trp = 7; tcl = 7;
		tcwl = 6; tras = 18; trc = 25;
		trcd = 7; trrd = 4; trtp = 4;
		twr = 8; twtr = 4; tzqzs = 64;
		tcksre = 6; tcksrx = 6; tmod = 12;
		tzqcl = 512; trefi = 4160;
		break;
	default:
		tmfc = 0; trp = 0; tcl = 0;
		tcwl = 0; tras = 0; trc = 0;
		trcd = 0; trrd = 0; trtp = 0;
		twr = 0; twtr = 0; tzqzs = 0;
		tcksre = 0; tcksrx = 0; tmod = 0;
		tzqcl = 0; trefi = 0;
		V206KDEBUG002("[ERROR] unrecognized ddr freq!\n");
		return 1;
		break;
	}




	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xD0, 78 | 1 << 31);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xD4, 0x4);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xD8, tmfc);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xDC, trp);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xE0, 2);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xE4, 0);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xE8, tcl);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xEC, tcwl);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xF0, tras);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xF4, trc);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xF8, trcd);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0xFC, trrd);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x100, trtp);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x104, twr);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x108, twtr);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x10C, 512);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x110, 7);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x114, 26);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x118, tzqzs);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x11C, 0);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x124, tcksre);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x128, tcksrx);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x12C, 6);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x130, tmod);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x134, 127);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x138, tzqcl);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x140, 7);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x148, trefi);

	return result;
}



static int FUNC206HAL039(void *devInfo, int index)
{
	int result = 0;
	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;
	unsigned int dfiphywrlat, dfirddataen;
	unsigned int ddrfreq = pDeviceData->V206DEV075;
	switch (ddrfreq) {
	case 533:
		dfiphywrlat = 3;
		dfirddataen = 5;
		break;
	case 465:
		dfiphywrlat = 3;
		dfirddataen = 5;
		break;
	case 400:
		dfiphywrlat = 2;
		dfirddataen = 4;
		break;
	case 266:
		dfiphywrlat = 1;
		dfirddataen = 2;
		break;
	default:
		dfiphywrlat = 0;
		dfirddataen = 0;
		V206KDEBUG002("[ERROR] unrecognized ddr freq!\n");
		return 1;
		break;
	}


	result = FUNC206HAL042(devInfo,  index, MAC206HAL055, 0x240, 0x3);

	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x244, 0x8);





	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x250, 1);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x254, dfiphywrlat);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x260, dfirddataen);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x264, 20);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x2D0, 0x1);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x2D4, 0x1);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x270, 0x10);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x274, 0x640);

	return result;
}



static int FUNC206HAL465(void *devInfo, int index)
{
	int result = 0;
	V206DEV025 *pDeviceData = (V206DEV025 *)devInfo;
	unsigned int mcmd0, mcmddct0, mcmd1, mcmddct1;
	unsigned int ddrfreq = pDeviceData->V206DEV075;
	switch (ddrfreq) {
	case 533:
		mcmd0 = 0x82142683;
		mcmddct0 = 0x02142683;
		mcmd1 = 0x80101243;
		mcmddct1 = 0x00101243;
		break;
	case 465:
		mcmd0 = 0x82142683;
		mcmddct0 = 0x02142683;
		mcmd1 = 0x80101243;
		mcmddct1 = 0x00101243;
		break;
	case 400:
		mcmd0 = 0x82142583;
		mcmddct0 = 0x02142583;
		mcmd1 = 0x8010d703;
		mcmddct1 = 0x0010d703;
		break;
	case 266:
		mcmd0 = 0x82142483;
		mcmddct0 = 0x02142483;
		mcmd1 = 0x80109303;
		mcmddct1 = 0x00109303;
		break;
	default:
		mcmd0 = 0;
		mcmddct0 = 0;
		mcmd1 = 0;
		mcmddct1 = 0;
		V206KDEBUG002("[ERROR] unrecognized ddr freq!\n");
		return 1;
		break;
	}


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x040, 0x86100000);


	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x040, 0x06100000);





	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x040, mcmd0);
	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x040, mcmddct0);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x040, 0x80160003);


	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x040, 0x00160003);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x040, 0x80120043);


	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x040, 0x00120043);
	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x040, mcmd1);
	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x040, mcmddct1);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x040, 0x80104005);


	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x040, 0x00104005);

	return result;
}




static int FUNC206HAL473(void *devInfo, int index)
{
	int result = 0;

	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x040, 0x8010000A);
	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x040, 0x0010000A);

	return result;
}



static int FUNC206HAL464(void *devInfo, int index)
{
	int result = 0;


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x04, 0x1);


	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x08, 0x1);

	return result;
}



static int FUNC206HAL052(void *devInfo, int index)
{
	int result = 0;


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x50, 0x1);


	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x4C, 0x1);

	return result;
}



static int FUNC206HAL041(void *devInfo, int index)
{
	int result = 0;

	result = FUNC206HAL042(devInfo, index, MAC206HAL056, 0x1, 0xfe01);
	result = FUNC206HAL053(devInfo, index, MAC206HAL056, 0x1, 0);
	result = FUNC206HAL053(devInfo, index, MAC206HAL056, 0x4, 0x90000fff);

	return result;
}




static int FUNC206HAL048(void *devInfo, int index)
{
	int result = 0;


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x04, 0x2);


	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x08, 0x3);

	return result;
}




static int FUNC206HAL049(void *devInfo, int index, unsigned int value)
{
	int result = 0;
	unsigned int testAddr = 0;
	unsigned int readValue;


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x210, value);

	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x214, value);

	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x218, value);

	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x21C, value);

	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x220, 0x0);


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x208, 0x401);

	while (testAddr < 0x100000) {

		result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x200, testAddr);

		result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x204, testAddr);


		result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x20C, 1);

		result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x20C, 0);
		if (0 != result) {

			result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x208, 0x400);

			return 1;
		}


		result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x94, 0);
		if (0 != result) {

			result = FUNC206HAL463(devInfo, index, MAC206HAL055, 0x224, &readValue);

			result = FUNC206HAL463(devInfo, index, MAC206HAL055, 0x228, &readValue);

			result = FUNC206HAL463(devInfo, index, MAC206HAL055, 0x22C, &readValue);

			result = FUNC206HAL463(devInfo, index, MAC206HAL055, 0x230, &readValue);


			result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x208, 0x400);

			return 1;
		}

		testAddr += 0x10;
	}


	result = FUNC206HAL042(devInfo, index, MAC206HAL055, 0x208, 0x400);

	return result;
}




static int FUNC206HAL036(void *devInfo, int index)
{
	int result;

	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x3F8, 0x3235302A);
	if (0 != result) {
		return 1;
	}

	result = FUNC206HAL053(devInfo, index, MAC206HAL055, 0x3FC, 0x44574300);
	if (0 != result) {
		return 1;
	}

	return 0;
}


int FUNC206HAL215(void *devInfo)
{
	int ret = 0;
	unsigned int i;

	for (i = 0; i < ((V206DEV025 *)devInfo)->V206DEV041; i++) {
		ret = FUNC206HAL043(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: configDdrTimer0 failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL040(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: configDdrCtrlMode failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL102(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: initAndStartDdrPhy failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL069(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: getDdrCtrlDfiStatus failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL101(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: initAndStartDdrCtroller failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL044(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: configDdrTimer1 failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL039(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: configDdrCtrlDfiTimer failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL465(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: startDdrSdram failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL473(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: updateDdrDfiCtrl failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL464(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: refershDdrCtrl failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL052(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: detectDdrCmdParam failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL041(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: configDdrPhyPub failure.\n", i);
			goto FUNC206HAL023;
		}

		ret = FUNC206HAL048(devInfo, i);
		if (ret != 0) {
			V206KDEBUG002("[ERROR] ddr%d: ddrConfigFinal failure.\n", i);
			goto FUNC206HAL023;
		}
	}

	V206DEV005("[INFO] mwv206 ddr init successfully.\n");
	return 0;

FUNC206HAL023:
	V206DEV005("[ERROR] mwv206 ddr init failure.\n");
	return ret;
}


static unsigned int mwv206_ddr_read_reg(V206DEV025 *dev, int index, int object, unsigned short addr)
{
	unsigned int value, readCnt;
	unsigned int regAddr, dataRegAddr;
	unsigned int writeData = 0;

	if (MAC206HAL056 == object) {
		writeData |= 1 << 20;
	}
	writeData |= (addr & 0x3FF);
	regAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL050;
	FUNC206HAL051(dev, regAddr, writeData);

	regAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL049;
	FUNC206HAL051(dev, regAddr, 1);

	for (readCnt = 0; readCnt < 5; readCnt++) {
		value = FUNC206HAL050(dev, regAddr);
		if (value == 0) {
			break;
		}
	}
	if (value) {
		V206KDEBUG002("[ERROR] mwv206_ddr_read_reg: write reg 0x%x done failure.\n", addr);
	}

	dataRegAddr = MAC206HAL053 + index * MAC206HAL054 + MAC206HAL052;
	value = FUNC206HAL050(dev, dataRegAddr);

	return value;
}

void mwv206_ddr_status(V206DEV025 *dev)
{
	unsigned int val, val_ex, addr;
	int i, is_consistent = 1;


	val_ex = mwv206_ddr_read_reg(dev, 0, MAC206HAL056, 0x7c);
	val_ex &= 0xf;

	for (i = 0; i < dev->V206DEV041; i++) {
		val = mwv206_ddr_read_reg(dev, i, MAC206HAL056, 0x4);
		V206KDEBUG003("[INFO] mwv206 ddr%d - addr: 0x04, value: 0x%08x", i, val);

		if ((val & 0xfff) != 0xfff) {
			is_consistent = 0;
		}


		for (addr = 0x7c; addr <= 0xac; addr += 0x10) {
			val = mwv206_ddr_read_reg(dev, i, MAC206HAL056, addr);
			V206KDEBUG003("[INFO] mwv206 ddr%d - addr: 0x%x, value: 0x%08x", i, addr, val);

			if ((val & 0xf) != val_ex) {
				is_consistent = 0;
			}
		}
	}

	if (is_consistent) {
		V206KDEBUG003("[INFO] mwv206 ddr channel training status is consistent");
	} else {
		V206KDEBUG003("[INFO] mwv206 ddr channel training status is not consistent");
	}

	return;
}
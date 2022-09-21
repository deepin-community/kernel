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
#include <linux/delay.h>
#include "mwv206dev.h"
#include "videoadd_reg.h"
#include "mwv206reg.h"
#include "mwv206displaymode.h"
#include "jmgraphoutput.h"

#define CEA_VIDEO_MODES_NUM 64
#define JMD_FIX_DVI_VOLTAGE 0x9001ffff

typedef enum {
    V_PHSYNC = 0x0001,
    V_NHSYNC = 0x0002,
    V_PVSYNC = 0x0004,
    V_NVSYNC = 0x0008,
    V_INTERLACE = 0x0010,
    V_DBLSCAN = 0x0020,
    V_CSYNC = 0x0040,
    V_PCSYNC = 0x0080,
    V_NCSYNC = 0x0100,
    V_HSKEW = 0x0200,
    V_BCAST = 0x0400,
    V_PIXMUX = 0x1000,
    V_DBLCLK = 0x2000,
    V_CLKDIV2 = 0x4000
} ModeFlags;

typedef struct _DisplayModeRec {

    int Clock;
    int HDisplay;
    int HSyncStart;
    int HSyncEnd;
    int HTotal;
    int HSkew;
    int VDisplay;
    int VSyncStart;
    int VSyncEnd;
    int VTotal;
    int VScan;
    int Flags;
} DisplayModeRec, *DisplayModePtr;

enum FUNC206HAL038 {
	CLK_FREQ_0 = 0,
	CLK_FREQ_1,
	CLK_FREQ_2,
	CLK_FREQ_3,
	CLK_FREQ_4,
	CLK_FREQ_MAX
};
const int clkfreq[CLK_FREQ_MAX] = {0x00280040, 0x00280230, 0x00280420, 0x00280810, 0x00281000};

static const DisplayModeRec CEAVideoModes[CEA_VIDEO_MODES_NUM] = {
	 {25175, 640, 656, 752, 800, 0, 480, 490, 492, 525, 0, V_NHSYNC | V_NVSYNC},
	 {27000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC},
	 {27000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC},
	 {74250, 1280, 1390, 1430, 1650, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC},
	 {74250, 1920, 2008, 2052, 2200, 0, 1080, 1084, 1094, 1125, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE},
	 {27000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {27000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {27000, 1440, 1478, 1602, 1716, 0, 240, 244, 247, 262, 0, V_NHSYNC | V_NVSYNC},
	 {27000, 1440, 1478, 1602, 1716, 0, 240, 244, 247, 262, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 2880, 2956, 3204, 3432, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {54000, 2880, 2956, 3204, 3432, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {54000, 2880, 2956, 3204, 3432, 0, 240, 244, 247, 262, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 2880, 2956, 3204, 3432, 0, 240, 244, 247, 262, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 1440, 1472, 1596, 1716, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 1440, 1472, 1596, 1716, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC},
	 {148500, 1920, 2008, 2052, 2200, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC},
	 {27000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {27000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {74250, 1280, 1720, 1760, 1980, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC},
	 {74250, 1920, 2448, 2492, 2640, 0, 1080, 1084, 1094, 1125, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE},
	 {27000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {27000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {27000, 1440, 1464, 1590, 1728, 0, 288, 290, 293, 312, 0, V_NHSYNC | V_NVSYNC},
	 {27000, 1440, 1464, 1590, 1728, 0, 288, 290, 293, 312, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 2880, 2928, 3180, 3456, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {54000, 2880, 2928, 3180, 3456, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {54000, 2880, 2928, 3180, 3456, 0, 288, 290, 293, 312, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 2880, 2928, 3180, 3456, 0, 288, 290, 293, 312, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 1440, 1464, 1592, 1728, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 1440, 1464, 1592, 1728, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {148500, 1920, 2448, 2492, 2640, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC},
	 {74250, 1920, 2558, 2602, 2750, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC},
	 {74250, 1920, 2448, 2492, 2640, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC},
	 {74250, 1920, 2008, 2052, 2200, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC},
	 {108000, 2880, 2944, 3192, 3432, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC},
	 {108000, 2880, 2944, 3192, 3432, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC},
	 {108000, 2880, 2928, 3184, 3456, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {108000, 2880, 2928, 3184, 3456, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {72000, 1920, 1952, 2120, 2304, 0, 1080, 1126, 1136, 1250, 0, V_PHSYNC | V_NVSYNC | V_INTERLACE},
	 {148500, 1920, 2448, 2492, 2640, 0, 1080, 1084, 1094, 1125, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE},
	 {148500, 1280, 1720, 1760, 1980, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC},
	 {54000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {148500, 1920, 2008, 2052, 2200, 0, 1080, 1084, 1094, 1125, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE},
	 {148500, 1280, 1390, 1430, 1650, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC},
	 {54000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC},
	 {54000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {54000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {108000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {108000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC},
	 {108000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {108000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {108000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC},
	 {108000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC},
	 {108000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {108000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE},
	 {59400, 1280, 3040, 3080, 3300, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC},
	 {74250, 1280, 3700, 3740, 3960, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC},
	 {74250, 1280, 3040, 3080, 3300, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC},
	 {297000, 1920, 2008, 2052, 2200, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC},
	 {297000, 1920, 2448, 2492, 2640, 0, 1080, 1084, 1094, 1125, 0, V_PHSYNC | V_PVSYNC},
 };

bool mwv206_pixel_freq_equal(V206IOCTL168 *mode, int clock)
{
	int delta;

	delta = mode->htotal * mode->vtotal * mode->framerate - clock * 1000;
	if (delta < 0) {
		delta = -delta;
	}


	return delta < 200 * 1000;
}

bool mwv206_modes_equal(V206IOCTL168 *mode, const DisplayModeRec *pMode)
{
	int flags;

	flags = (mode->hpol ? V_NHSYNC : V_PHSYNC) | (mode->vpol ? V_NVSYNC : V_PVSYNC);
	flags |= MWV206_PORT_IS_INTERLACED(mode->flags) ? V_INTERLACE : 0;

	if (mwv206_pixel_freq_equal(mode, pMode->Clock)
		 && mode->hactive == pMode->HDisplay
		 && mode->htotal == pMode->HTotal
		 && mode->hsync == pMode->HSyncEnd - pMode->HSyncStart
		 && mode->vactive == pMode->VDisplay
		 && mode->vtotal == pMode->VTotal
		 && mode->vsync == pMode->VSyncEnd - pMode->VSyncStart
		 && flags == pMode->Flags) {
		return true;
	} else {
		return false;
	}
}

int mwv206_get_vic(V206IOCTL168 *mode)
{
	int i ;
	for (i = 0; i < 64; i++) {
		if (mwv206_modes_equal(mode, CEAVideoModes + i)) {
			return i + 1;
		}
	}
	return 0;
}

int FUNC206HAL389(V206DEV025 *pDev, long arg)
{
	unsigned int value;
	unsigned int V206DEV033;
	V206IOCTL160 *setdispcursor;
	setdispcursor = (V206IOCTL160 *)arg;
	if ((setdispcursor->V206FB011 < 0) || (setdispcursor->V206FB011 > 3)) {
		return -2;
	}

	FUNC206HAL369(pDev, 1);

	if (setdispcursor->options & V206IOCTL016) {


		V206DEV033 = 0x9478 + OUTPUT_REG_OFFSET(setdispcursor->V206FB011);
		value = V206DEV001(V206DEV033);
		if (setdispcursor->enable) {
			value &= 0xFF00;
			value |= 6;
		} else {
			value &= ~1;
		}
		V206DEV002(V206DEV033, value);
	}

	if (setdispcursor->options & V206IOCTL017) {

		V206DEV033 = 0x9478 + OUTPUT_REG_OFFSET(setdispcursor->V206FB011);
		value = V206DEV001(V206DEV033);
		value &= 0xF;
		value |= ((setdispcursor->alpha & 0xFF) << 8);
		V206DEV002(V206DEV033, value);
	}

	if (setdispcursor->options & V206IOCTL018) {
		unsigned int bmpx, bmpy;
		int x, y, V206FB011;
		V206FB011 = setdispcursor->V206FB011;
		x = setdispcursor->x;
		y = setdispcursor->y;
		x -= pDev->V206DEV078[V206FB011][0];
		y -= pDev->V206DEV078[V206FB011][1];
		if (x < -31) {
			x = 0;
		}
		if (y < -31) {
			y = 0;
		}
		bmpx = 0;
		bmpy = 0;
		if (x < 0) {
			bmpx = -x;
			x = 0;
		}
		if (y < 0) {
			bmpy = -y;
			y = 0;
		}


		V206DEV033 = 0x9454 +  OUTPUT_REG_OFFSET(V206FB011);
		value = V206DEV001(V206DEV033);
		if (bmpx != (value & 0x3F) || bmpy != ((value >> 16) & 0x3F)) {
			value = (bmpx & 0x3F) | ((bmpy & 0x3F) << 16);
			V206DEV002(V206DEV033, value);
		}

		value = (x & 0xFFF) | ((y & 0xFFF) << 16);
		V206DEV033 = 0x94A8 + OUTPUT_REG_OFFSET(V206FB011);
		V206DEV002(V206DEV033, value);
	}

	if (setdispcursor->options & V206IOCTL019) {

		pDev->V206DEV078[setdispcursor->V206FB011][0] = setdispcursor->hotx;
		pDev->V206DEV078[setdispcursor->V206FB011][1] = setdispcursor->hoty;
	}

	FUNC206HAL369(pDev, 0);
	return 0;
}

static int FUNC206HAL037(int freq)
{
	if ((freq > 20) && (freq <= 21)) {
		return CLK_FREQ_0;
	} else if ((freq > 21) && (freq <= 42)) {
		return CLK_FREQ_1;
	} else if ((freq > 42) && (freq <= 85)) {
		return CLK_FREQ_2;
	} else if ((freq > 85) && (freq <= 170)) {
		return CLK_FREQ_3;
	} else if ((freq > 170) && (freq <= 348)) {
		return CLK_FREQ_4;
	} else {
		return CLK_FREQ_MAX;
	}
}

static int FUNC206HAL350(V206DEV025 *pDev, int winid, int V206HDMIAUDIO027, int FUNC206HAL038, int dualpixel,
		int hpol, int vpol)
{
	int val_car, formerval_car, newval_car;
	int pllreg, pllctrl, hdmireg;
	int hpolval, vpolval;

	if (FUNC206HAL038 < CLK_FREQ_0 || FUNC206HAL038 >= CLK_FREQ_MAX) {
		V206KDEBUG002("[ERROR] invalid pll clkindex %d!\n\n", FUNC206HAL038);
		return -2;
	}


	pllreg = (V206HDMIAUDIO027 == 0) ? ((0x403000) + 0x0400) :
		((V206HDMIAUDIO027 == 1)  ? ((0x403000) + 0x0500) :
		 ((V206HDMIAUDIO027 == 2)  ? ((0x403000) + 0x0600) :
		  ((0x403000) + 0x0700)));

	pllctrl = (V206HDMIAUDIO027 == 0) ? ((0x403000) + 0x0A04) :
		((V206HDMIAUDIO027 == 1)  ? ((0x403000) + 0x0A08) :
		 ((V206HDMIAUDIO027 == 2)  ? ((0x403000) + 0x0A0C) :
		  ((0x403000) + 0x0A10)));



	hpolval = 1; vpolval = 1;
	if (hpol == MWV206_POSITIVE)  {
		hpolval = 1;
	} else {
		hpolval = 0;
	}

	if (vpol == MWV206_POSITIVE)  {
		vpolval = 1;
	} else {
		vpolval = 0;
	}

	formerval_car = V206DEV001(pllreg);
	V206DEV005("[HDMICTRL]hpolval = %d, vpolval = %d.\n", hpolval, vpolval);
	newval_car  = (formerval_car & 0xFFFFFCFF) | (vpolval << 8) | (hpolval << 9);
	V206DEV002(pllreg, newval_car);
	V206DEV005("formerval_car = %x, Hpol = %d, Vpol = %d, value = %x\n", formerval_car, hpolval, vpolval, newval_car);



	hdmireg = MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1000));
	formerval_car = V206DEV001(hdmireg);
	newval_car  = (formerval_car & 0xFFFFFF9F) | (vpolval << 6) | (hpolval << 5);
	V206DEV002(hdmireg, newval_car);


	V206DEV002(pllctrl, clkfreq[FUNC206HAL038]);

	formerval_car = V206DEV001(pllreg);
	if (pDev->V206DEV093[winid] > 165) {
		val_car = 1 << 18;
	} else {
		val_car = 0 << 18;
	}
	newval_car = (formerval_car & 0xFFF3FFFF) | val_car;
	V206DEV002(pllreg, newval_car);

	return 0;
}

static int FUNC206HAL349(V206DEV025 *pDev, int V206HDMIAUDIO027, int FUNC206HAL038)
{
	GLJ_TICK tick;
	GLJ_TICK timeout;
	int regval, ret = 0;

	timeout = FUNC206LXDEV098()/1000 <= 3 ? 3 : FUNC206LXDEV098()/1000;
	regval = V206DEV001(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x3004)));
	mwv206_timed_loop (tick, (regval & 0x1) == 0, timeout) {
		regval = V206DEV001(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x3004)));
	}
	if ((V206DEV001(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x3004))) & 0x01) == 0) {
		V206KDEBUG003("[INFO] hdmi%d pll not locked, clkindex = %d.\n", V206HDMIAUDIO027, FUNC206HAL038);
		ret = -1;
	}

	return ret;
}


int FUNC206HAL394(V206DEV025 *pDev, long arg)
{
	V206IOCTL173 *modeparam;
	int regval;
	int V206DEV033;
	int port, type, value;

	modeparam = (V206IOCTL173 *)arg;
	port = modeparam->V206FB008;
	type = modeparam->type;
	value = modeparam->value;

	if (pDev->pm.V206DEV109 == 0) {
		pDev->pm.V206DEV118[port] = *modeparam;
		pDev->pm.V206DEV113[port] = 1;
	}

	switch (type) {
	case MWV206K_DP_LVDS_MODE:
		if ((port > MWV206_DP_LVDS_1) || (port < MWV206_DP_LVDS_0)) {
			return 0;
		}
		V206DEV033 = ((0x403000) + 0x0800) + (port - MWV206_DP_LVDS_0) * 0x100;
		regval = V206DEV001(V206DEV033);
		regval = regval & ~(0x30);
		switch (value) {
		case MWV206K_LVDS_MODE_18BIT_LDI:
		case MWV206K_LVDS_MODE_24BIT_LDI:
			regval = (0x1 << 4) | regval;
			V206DEV002(V206DEV033, regval);
			break;
		case MWV206K_LVDS_MODE_24BIT_VESA:
			regval = (0 << 4) | regval;
			V206DEV002(V206DEV033, regval);
			break;
		case MWV206K_LVDS_MODE_18BIT_LDI_SHAKE:
			regval = (0x3 << 4) | regval;
			V206DEV002(V206DEV033, regval);
			break;
		default:
			V206KDEBUG002("invalid lvds mode value.");
			return -1;
			break;
		}
		break;
	default:
		V206KDEBUG002("Unsupport paramtype %d!\n", type);
		return -1;
		break;
	}
	V206DEV005("[%s:%d] type = %d, value = %d, regval = 0x%x.\n", __FUNCTION__, __LINE__, type, value, regval);
	return 0;
}

static void FUNC206HAL387(V206DEV025 *pDev, int V206FB011, int V206HDMIAUDIO027, int dualpixel)
{
	int val_car, formerval_car, newval_car;
	int val_pll, formerval_pll, newval_pll;
	int pllreg, pllctrl;


	pllreg = (V206HDMIAUDIO027 == 0) ? ((0x403000) + 0x0400) :
		((V206HDMIAUDIO027 == 1)  ? ((0x403000) + 0x0500) :
		 ((V206HDMIAUDIO027 == 2)  ? ((0x403000) + 0x0600) :
		  ((0x403000) + 0x0700)));

	pllctrl = (V206HDMIAUDIO027 == 0) ? ((0x403000) + 0x0A04) :
		((V206HDMIAUDIO027 == 1)  ? ((0x403000) + 0x0A08) :
		 ((V206HDMIAUDIO027 == 2)  ? ((0x403000) + 0x0A0C) :
		  ((0x403000) + 0x0A10)));


	formerval_pll = V206DEV001(((0x400000) + 0x0014));
	val_pll = V206FB011 << (V206HDMIAUDIO027 * 8);
	if (dualpixel) {
		val_pll = (V206FB011 + 8) << (V206HDMIAUDIO027 * 8);
	}
	newval_pll = (formerval_pll & (~(0xFF << (V206HDMIAUDIO027 * 8)))) | val_pll;
	V206DEV002(((0x400000) + 0x0014), newval_pll);


	formerval_car = V206DEV001(pllreg);
	val_car = (V206FB011 << 4) | 1;
	if (dualpixel) {
		val_car |= 0x06;
	}
	newval_car = (formerval_car & 0xFFFFFFCF) | val_car;
	V206DEV002(pllreg, newval_car);
}

static void FUNC206HAL386(V206DEV025 *pDev, V206IOCTL172 *setdispport, int ctrlreg, int enable)
{
	int formerval, V206IOCTLCMD010, tempval;
	int V206HDMIAUDIO027 = setdispport->V206FB008 - MWV206_DP_HDMI_0;

	if (enable) {
		FUNC206HAL387(pDev, setdispport->V206FB011, V206HDMIAUDIO027, setdispport->dualpixel);

		formerval = V206DEV001(((0x403000) + 0x0A00));
		tempval = ((1 << V206HDMIAUDIO027) | (1 << V206HDMIAUDIO027 << 4));
		V206IOCTLCMD010 = formerval | tempval;
		V206DEV002(((0x403000) + 0x0A00), V206IOCTLCMD010);

		formerval = V206DEV001(ctrlreg);
		V206IOCTLCMD010 = formerval | 1;
		V206DEV002(ctrlreg, V206IOCTLCMD010);
	} else {

		formerval = V206DEV001(ctrlreg);
		tempval = 0xFFFFFFFC;
		V206IOCTLCMD010 = formerval & tempval;
		V206DEV002(ctrlreg, V206IOCTLCMD010);

		formerval = V206DEV001(((0x403000) + 0x0A00));
		tempval = (~((1 << V206HDMIAUDIO027) | (1 << V206HDMIAUDIO027 << 4)));
		V206IOCTLCMD010 = formerval & tempval;
		V206DEV002(((0x403000) + 0x0A00), V206IOCTLCMD010);
	}
}

static int mwv206_setdisplayport_nolock(V206DEV025 *pDev, long arg)
{
	int val_car, formerval_car, newval_car, val_clk, formerval_clk, newval_clk, en;
	int V206HDMIAUDIO027, hpol, vpol, i;
	V206IOCTL172 *setdispport;

	setdispport = (V206IOCTL172 *)arg;
	V206HDMIAUDIO027 = setdispport->V206FB008 - MWV206_DP_HDMI_0;
	if ((setdispport->V206FB011 < 0) || (setdispport->V206FB011 > 3)) {
		return -2;
	}
	if ((setdispport->V206FB008 < MWV206_DP_DVO_0) || (setdispport->V206FB008 > MWV206_DP_HDMI_3)) {
		return -2;
	}

	if (pDev->pm.V206DEV109 == 0) {
		int id = setdispport->V206FB008;
		pDev->pm.V206DEV117[id] = *setdispport;
		pDev->pm.V206DEV112[id] = 1;
	}

	hpol = pDev->V206DEV146[setdispport->V206FB011].hpol;
	vpol = pDev->V206DEV146[setdispport->V206FB011].vpol;

	en = (setdispport->enable) ? 1 : 0;

	switch (setdispport->V206FB008) {
	case MWV206_DP_DVO_0:
		formerval_car = V206DEV001(((0x403000) + 0x0200));
		if (en) {

			formerval_clk = V206DEV001(((0x400000) + 0x0010));
			val_clk = setdispport->V206FB011;
			if (setdispport->dualpixel) {
				val_clk += 8;
			}
			newval_clk = (formerval_clk & 0xFFFFFFF0) | val_clk;
			V206DEV002(((0x400000) + 0x0010), newval_clk);

			val_car = ((setdispport->V206FB011) << 2)
				| (setdispport->dualpixel << 1)
				| (hpol << 9)
				| (vpol << 8)
				| (en);
			newval_car = (formerval_car & 0xFFFFFCF1) | val_car;
			V206DEV002(((0x403000) + 0x0200), newval_car);
			pDev->V206DEV144[MWV206_DP_DVO_0].V206FB011 = setdispport->V206FB011;
			pDev->V206DEV144[MWV206_DP_DVO_0].dualpixel = setdispport->dualpixel;
		} else {
			val_car = 0xFFFFFFFE;
			newval_car = formerval_car & val_car;
			V206DEV002(((0x403000) + 0x0200), newval_car);
		}
		pDev->V206DEV144[MWV206_DP_DVO_0].enable = en;
		break;
	case MWV206_DP_DVO_1:
		formerval_car = V206DEV001(((0x403000) + 0x0300));
		if (en) {

			formerval_clk = V206DEV001(((0x400000) + 0x0010));
			val_clk = setdispport->V206FB011 << 8;
			if (setdispport->dualpixel) {
				val_clk = (setdispport->V206FB011 + 8) << 8;
			}
			newval_clk = (formerval_clk & 0xFFFFF0FF) | val_clk;
			V206DEV002(((0x400000) + 0x0010), newval_clk);

			val_car = ((setdispport->V206FB011) << 2)
				| (setdispport->dualpixel << 1)
				| (hpol << 9)
				| (vpol << 8)
				| (en);
			newval_car = (formerval_car & 0xFFFFFCF1) | val_car;
			V206DEV002(((0x403000) + 0x0300), newval_car);
			pDev->V206DEV144[MWV206_DP_DVO_1].V206FB011 = setdispport->V206FB011;
			pDev->V206DEV144[MWV206_DP_DVO_1].dualpixel = setdispport->dualpixel;
		} else {
			val_car = 0xFFFFFFFE;
			newval_car = formerval_car & val_car;
			V206DEV002(((0x403000) + 0x0300), newval_car);
		}
		pDev->V206DEV144[MWV206_DP_DVO_1].enable = en;
		break;
	case MWV206_DP_DAC_0:
		formerval_car = V206DEV001(((0x403000) + 0x0000));
		if (en) {

			formerval_clk = V206DEV001(((0x400000) + 0x000c));
			val_clk = setdispport->V206FB011;
			newval_clk = (formerval_clk & 0xFFFFFFF0) | val_clk;
			V206DEV002(((0x400000) + 0x000c), newval_clk);

			val_car = (setdispport->V206FB011)
				| (hpol << 2)
				| (vpol << 3)
				| (0x80000000);
			newval_car = (formerval_car & 0xFFFFFFF0) | val_car;
			V206DEV002(((0x403000) + 0x0000), newval_car);
			pDev->V206DEV144[MWV206_DP_DAC_0].V206FB011 = setdispport->V206FB011;
			pDev->V206DEV144[MWV206_DP_DAC_0].dualpixel = setdispport->dualpixel;
		} else {
			val_car = 0x7FFFFFFF;
			newval_car = formerval_car & val_car;
			V206DEV002(((0x403000) + 0x0000), newval_car);
		}
		pDev->V206DEV144[MWV206_DP_DAC_0].enable = en;
		break;
	case MWV206_DP_DAC_1:
		formerval_car = V206DEV001(((0x403000) + 0x0100));
		if (en) {

			formerval_clk = V206DEV001(((0x400000) + 0x000c));
			val_clk = setdispport->V206FB011 << 8;
			newval_clk = (formerval_clk & 0xFFFFF0FF) | val_clk;
			V206DEV002(((0x400000) + 0x000c), newval_clk);

			val_car = (setdispport->V206FB011)
				| (hpol << 2)
				| (vpol << 3)
				| (0x80000000);
			newval_car = (formerval_car & 0xFFFFFFF0) | val_car;
			V206DEV002(((0x403000) + 0x0100), newval_car);
			pDev->V206DEV144[MWV206_DP_DAC_1].V206FB011 = setdispport->V206FB011;
			pDev->V206DEV144[MWV206_DP_DAC_1].dualpixel = setdispport->dualpixel;
		} else {
			val_car = 0x7FFFFFFF;
			newval_car = formerval_car & val_car;
			V206DEV002(((0x403000) + 0x0100), newval_car);
		}
		pDev->V206DEV144[MWV206_DP_DAC_1].enable = en;
		break;
	case MWV206_DP_LVDS_0:
		formerval_car = V206DEV001(((0x403000) + 0x0800));
		if (en) {

			formerval_clk = V206DEV001(((0x400000) + 0x0018));
			val_clk = setdispport->V206FB011;
			if (setdispport->dualpixel) {
				val_clk += 8;
			}

			newval_clk = (formerval_clk & 0xFFFFFFF0) | val_clk;
			V206DEV002(((0x400000) + 0x0018), newval_clk);

			val_car = ((setdispport->V206FB011) << 2)
				| (setdispport->dualpixel << 1)
				| (hpol << 9)
				| (vpol << 8)
				| en;
			newval_car = (formerval_car & 0xFFFFFCF1) | val_car;
			V206DEV002(((0x403000) + 0x0800), newval_car);
			pDev->V206DEV144[MWV206_DP_LVDS_0].V206FB011 = setdispport->V206FB011;
			pDev->V206DEV144[MWV206_DP_LVDS_0].dualpixel = setdispport->dualpixel;
		} else {
			val_car = 0xFFFFFFFE;
			newval_car = formerval_car & val_car;
			V206DEV002(((0x403000) + 0x0800), newval_car);
		}
		pDev->V206DEV144[MWV206_DP_LVDS_0].enable = en;
		break;
	case MWV206_DP_LVDS_1:
		formerval_car = V206DEV001(((0x403000) + 0x0900));
		if (en) {

			formerval_clk = V206DEV001(((0x400000) + 0x0018));
			val_clk = setdispport->V206FB011 << 8;
			if (setdispport->dualpixel) {
				val_clk = (setdispport->V206FB011 + 8) << 8;
			}
			newval_clk = (formerval_clk & 0xFFFFF0FF) | val_clk;
			V206DEV002(((0x400000) + 0x0018), newval_clk);

			formerval_car = V206DEV001(((0x403000) + 0x0900));
			val_car = ((setdispport->V206FB011) << 2)
				| (setdispport->dualpixel << 1)
				| (hpol << 9)
				| (vpol << 8)
				| en;
			newval_car = (formerval_car & 0xFFFFFCF1) | val_car;
			V206DEV002(((0x403000) + 0x0900), newval_car);
			pDev->V206DEV144[MWV206_DP_LVDS_1].V206FB011 = setdispport->V206FB011;
			pDev->V206DEV144[MWV206_DP_LVDS_1].dualpixel = setdispport->dualpixel;
		} else {
			val_car = 0xFFFFFFFE;
			newval_car = formerval_car & val_car;
			V206DEV002(((0x403000) + 0x0900), newval_car);
		}
		pDev->V206DEV144[MWV206_DP_LVDS_1].enable = en;
		break;
	case MWV206_DP_HDMI_0:
		FUNC206HAL386(pDev, setdispport, ((0x403000) + 0x0400), en);
		pDev->V206DEV144[MWV206_DP_HDMI_0].V206FB011 = setdispport->V206FB011;
		pDev->V206DEV144[MWV206_DP_HDMI_0].dualpixel = setdispport->dualpixel;
		pDev->V206DEV144[MWV206_DP_HDMI_0].enable = en;
		break;
	case MWV206_DP_HDMI_1:
		FUNC206HAL386(pDev, setdispport, ((0x403000) + 0x0500), en);
		pDev->V206DEV144[MWV206_DP_HDMI_1].V206FB011 = setdispport->V206FB011;
		pDev->V206DEV144[MWV206_DP_HDMI_1].dualpixel = setdispport->dualpixel;
		pDev->V206DEV144[MWV206_DP_HDMI_1].enable = en;
		break;
	case MWV206_DP_HDMI_2:
		FUNC206HAL386(pDev, setdispport, ((0x403000) + 0x0600), en);
		pDev->V206DEV144[MWV206_DP_HDMI_2].V206FB011 = setdispport->V206FB011;
		pDev->V206DEV144[MWV206_DP_HDMI_2].dualpixel = setdispport->dualpixel;
		pDev->V206DEV144[MWV206_DP_HDMI_2].enable = en;
		break;
	case MWV206_DP_HDMI_3:
		FUNC206HAL386(pDev, setdispport, ((0x403000) + 0x0700), en);
		pDev->V206DEV144[MWV206_DP_HDMI_3].V206FB011 = setdispport->V206FB011;
		pDev->V206DEV144[MWV206_DP_HDMI_3].dualpixel = setdispport->dualpixel;
		pDev->V206DEV144[MWV206_DP_HDMI_3].enable = en;
		break;
	default:
		return -1;
		break;
	}


	for (i = 0; i < 4; i++) {
		pDev->scanctrl[i].crtc_connected = 0;
	}
	for (i = MWV206_DP_DVO_0; i < MWV206_DP_COUNT; i++) {
		if (pDev->V206DEV144[i].enable) {
			pDev->scanctrl[pDev->V206DEV144[i].V206FB011].crtc_connected = 1;
		}
	}
	return 0;
}

int FUNC206HAL393(V206DEV025 *pDev, long arg)
{
	int ret;

	FUNC206HAL369(pDev, 1);
	ret = mwv206_setdisplayport_nolock(pDev, arg);
	FUNC206HAL369(pDev, 0);

	return ret;
}

int FUNC206HAL390(V206DEV025 *pDev, long arg)
{
	V206IOCTL161 *setdispmode;
	int  hpol = 0, vpol = 0;

	setdispmode = (V206IOCTL161 *)arg;
	if ((setdispmode->mode < 0) || (setdispmode->mode > MWV206K_DPMODE_USER)) {
		return -1;
	}

	if ((setdispmode->V206FB011 < 0) || (setdispmode->V206FB011 > 3)) {
		return -2;
	}

	FUNC206HAL369(pDev, 1);
	if (pDev->pm.V206DEV109 == 0) {
		int id = setdispmode->V206FB011;
		pDev->pm.V206DEV115[id] = *setdispmode;
		pDev->pm.V206DEV110[id] = 1;
	}

	pDev->V206DEV079[setdispmode->V206FB011] = setdispmode->V206DEV079;
	if (setdispmode->mode == MWV206K_DPMODE_USER) {
		if (setdispmode->framerate > 1) {
			FUNC206HAL192(pDev,
					setdispmode->V206FB011,
					setdispmode->htotal,
					setdispmode->hactive,
					setdispmode->hfrontporch,
					setdispmode->hsync,
					setdispmode->vtotal,
					setdispmode->vactive,
					setdispmode->vfrontporch,
					setdispmode->vsync,
					setdispmode->framerate,
					setdispmode->V206DEV079,
					1);
			hpol =  setdispmode->hpol;
			vpol =  setdispmode->vpol;

		}
	} else {
		FUNC206HAL192(pDev,
				setdispmode->V206FB011,
				modeparams[setdispmode->mode][V206DISPMODE001],
				modeparams[setdispmode->mode][V206DISPMODE002],
				modeparams[setdispmode->mode][V206DISPMODE003] + modeparams[setdispmode->mode][V206DISPMODE005],
				modeparams[setdispmode->mode][V206DISPMODE004],
				modeparams[setdispmode->mode][V206DISPMODE006],
				modeparams[setdispmode->mode][V206DISPMODE007],
				modeparams[setdispmode->mode][V206DISPMODE008] + modeparams[setdispmode->mode][V206DISPMODE010],
				modeparams[setdispmode->mode][V206DISPMODE009],
				modeparams[setdispmode->mode][V206DISPMODE011],
				setdispmode->V206DEV079,
				1);
		hpol =  modeparams[setdispmode->mode][V206DISPMODE012];
		vpol =  modeparams[setdispmode->mode][V206DISPMODE013];
	}

	pDev->V206DEV146[setdispmode->V206FB011].hpol = hpol;
	pDev->V206DEV146[setdispmode->V206FB011].vpol = vpol;
	FUNC206HAL369(pDev, 0);

	return 0;
}

static void mwv206_set_scan_addr(V206DEV025 *pDev, int crtc, int shadow_on)
{
	V206IOCTL159 *setdispaddr;
	struct scanctrl *sc = &pDev->scanctrl[crtc];
	unsigned int offset;
	int display_idx = sc->last_update_idx;

	if (shadow_on && display_idx >= 0 && display_idx < 3 && sc->shadow_on) {
		setdispaddr = &sc->shadowaddr[display_idx];
	} else {

		sc->last_update_idx = -1;
		setdispaddr = &sc->V206DEV143;
	}

	if (crtc == 0) {
		offset = 0;
	} else if (crtc == 1) {
		offset = 0x100;
	} else if (crtc == 2) {
		offset = 0x800;
	} else if (crtc == 3) {
		offset = 0x900;
	} else {
		V206KDEBUG002("Invalid crtc, should be 0~3.\n");
		return;
	}

	if (pDev->pm.V206DEV109 == 0) {
		pDev->pm.V206DEV116[crtc]  = sc->V206DEV143;
		pDev->pm.V206DEV111[crtc] = 1;
	}


	if (pDev->zoom_on
		&& ((1 << crtc) & V206DEV001((0x400000) + 0x902C))
		&& V206DEV001((0x400000) + 0x9054)) {
		V206DEV002((0x400000) + 0x9050, setdispaddr->addr);
		V206DEV002((0x400000) + 0x9068 + WINDOW_REG_OFFSET(crtc),
				setdispaddr->V206KG2D033 | (setdispaddr->V206KG2D033 << 16));
	}

	V206DEV002(((0x409000) + 0x0434) + offset, setdispaddr->V206KG2D033 / 16);
	V206DEV002(((0x409000) + 0x0430) + offset, (setdispaddr->format == V206FB002) ? 0 : 1);
	V206DEV002(((0x409000) + 0x043C) + offset,
			((setdispaddr->width - 1) << 0) | ((setdispaddr->height - 1) << 16));
	V206DEV002(((0x409000) + 0x04AC) + offset, 0);
	V206DEV002(((0x409000) + 0x0438) + offset, setdispaddr->addr);

	if (pDev->V206DEV039 == 0) {
		if (((1 << crtc) & V206DEV001((0x400000) + 0x902C + WINDOW_REG_OFFSET(crtc)))
			&& V206DEV001((0x400000) + 0x9054 + WINDOW_REG_OFFSET(crtc))) {
			V206DEV002((0x400000) + 0x9050 + WINDOW_REG_OFFSET(crtc), setdispaddr->addr);
			V206DEV002((0x400000) + 0x9068 + WINDOW_REG_OFFSET(crtc),
					setdispaddr->V206KG2D033 | (setdispaddr->V206KG2D033 << 16));
		}
	}

	sc->crtc_configured = 1;
}

void mwv206_shadow_crtc_fallback_nolock(V206DEV025 *pDev)
{
	struct scanctrl *sc;
	int crtc;

	for (crtc = 0; crtc < 4; crtc++) {
		sc = &pDev->scanctrl[crtc];
		if (sc->shadow_on) {
			mwv206_set_scan_addr(pDev, crtc, 0);
		}
	}
}

static int disable_vblank_sync;
module_param(disable_vblank_sync, int, 0644);
MODULE_PARM_DESC(disable_vblank_sync, "Disable vblank sync, state change occurs when no 3D is running");

static enum hrtimer_restart mwv206_shadow_periodic_sync(struct hrtimer *hrtimer)
{
	struct scanctrl *sc = container_of(hrtimer, struct scanctrl, timer);
	V206DEV025 *pDev = sc->pDev;

	queue_work(pDev->wq, &sc->work);

	hrtimer_forward_now(hrtimer, sc->vsync_interval);

	return HRTIMER_RESTART;
}

static int mwv206_shadow_setup(V206DEV025 *pDev, int crtc)
{
	struct scanctrl *sc = &pDev->scanctrl[crtc];

	if (sc->V206DEV143.width == 0 || sc->V206DEV143.height == 0 || sc->V206DEV143.V206KG2D033 == 0) {
		return -1;
	}

	sc->shadowaddr[0] = sc->V206DEV143;


	if (!pDev->zoom_on) {
		if (sc->shadowaddr[0].format == V206FB002) {
			sc->shadowaddr[0].V206KG2D033 = ((sc->shadowaddr[0].width + 15) & 0xfffffff0) * 4;
		} else {
			sc->shadowaddr[0].V206KG2D033 = ((sc->shadowaddr[0].width + 15) & 0xfffffff0) * 2;
		}
	}

	sc->shadowaddr[0].addr = FUNC206HAL223(pDev,
			-sc->shadowaddr[0].V206KG2D033 * sc->shadowaddr[0].height,
			0x100);
	if (!sc->shadowaddr[0].addr) {
		return -1;
	}

	sc->shadowaddr[1] = sc->shadowaddr[0];
	sc->shadowaddr[1].addr = FUNC206HAL223(pDev,
			-sc->shadowaddr[1].V206KG2D033 * sc->shadowaddr[1].height,
			0x100);
	if (!sc->shadowaddr[1].addr) {
		FUNC206HAL226(pDev, sc->shadowaddr[0].addr);
		return -1;
	}

	sc->shadowaddr[2] = sc->shadowaddr[0];
	sc->shadowaddr[2].addr = FUNC206HAL223(pDev,
			-sc->shadowaddr[2].V206KG2D033 * sc->shadowaddr[2].height,
			0x100);
	if (!sc->shadowaddr[2].addr) {
		FUNC206HAL226(pDev, sc->shadowaddr[0].addr);
		FUNC206HAL226(pDev, sc->shadowaddr[1].addr);
		return -1;
	}

	sc->last_update_time = ktime_set(0, 0);
	sc->last_update_idx  = -1;
	sc->drawfb_dirty     = 0;
	sc->display_pending  = 0;
	sc->drawing          = 0;
	hrtimer_start(&sc->timer, sc->vsync_interval, HRTIMER_MODE_REL);

	return 0;
}

static void mwv206_shadow_teardown(V206DEV025 *pDev, int crtc)
{
	struct scanctrl *sc = &pDev->scanctrl[crtc];

	if (!sc->shadow_on) {
		return;
	}

	hrtimer_cancel(&sc->timer);

	mwv206_set_scan_addr(pDev, crtc, 0);

	msleep(ktime_to_ms(sc->vsync_interval) + 1);

	sc->shadow_on = 0;

	if (sc->shadowaddr[0].addr) {
		FUNC206HAL226(pDev, sc->shadowaddr[0].addr);
	}
	if (sc->shadowaddr[1].addr) {
		FUNC206HAL226(pDev, sc->shadowaddr[1].addr);
	}
	if (sc->shadowaddr[2].addr) {
		FUNC206HAL226(pDev, sc->shadowaddr[2].addr);
	}

	memset(&sc->shadowaddr[0], 0, sizeof(V206IOCTL159));
	memset(&sc->shadowaddr[1], 0, sizeof(V206IOCTL159));
	memset(&sc->shadowaddr[2], 0, sizeof(V206IOCTL159));
}


void mwv206_shadow_switch_on_nolock(V206DEV025 *pDev)
{
	struct scanctrl *sc;
	int crtc, dis_vsync_cur;

	dis_vsync_cur = disable_vblank_sync;
	if (pDev->disable_vblank_sync != dis_vsync_cur) {
		if (pDev->disable_vblank_sync == 0) {

			pDev->disable_vblank_sync = dis_vsync_cur;
		} else if (dis_vsync_cur == 0) {

			pDev->disable_vblank_sync = 0;
			mwv206_shadow_init(pDev);
		}
	}

	if (pDev->disable_vblank_sync) {
		return;
	}

	for (crtc = 0; crtc < 4; crtc++) {
		sc = &pDev->scanctrl[crtc];
		if (!sc->shadow_on && sc->crtc_configured) {
			if (mwv206_shadow_setup(pDev, crtc) == 0) {
				sc->shadow_on = 1;
			}
		}
	}
}

void mwv206_shadow_switch_off_nolock(V206DEV025 *pDev)
{
	struct scanctrl *sc;
	int crtc;

	for (crtc = 0; crtc < 4; crtc++) {
		sc = &pDev->scanctrl[crtc];
		if (sc->shadow_on) {
			mwv206_shadow_teardown(pDev, crtc);
		}
	}
}

void mwv206_shadow_mark_draw(V206DEV025 *pDev)
{
	struct scanctrl *sc;
	int crtc;


	if (pDev->vblank_sync) {
		FUNC206HAL369(pDev, 1);
		for (crtc = 0; crtc < 4; crtc++) {
			sc = &pDev->scanctrl[crtc];
			if (sc->shadow_on && !sc->drawing) {
				sc->drawing = 1;
			}
		}
		FUNC206HAL369(pDev, 0);
	}
}

static int mwv206_shadow_update(V206DEV025 *pDev, int crtc)
{
	struct scanctrl *sc = &pDev->scanctrl[crtc];
	V206IOCTL157 mbit;
	int    update_index;

	sc->last_update_time = ktime_get();

	mwv206_set_scan_addr(pDev, crtc, 1);
	update_index = (sc->last_update_idx + 1) % 3;

	mbit.V206KG2D023   = sc->V206DEV143.addr;
	mbit.V206KG2D004   = sc->shadowaddr[update_index].addr;
	mbit.V206KG2D032 = sc->V206DEV143.V206KG2D033;
	mbit.V206KG2D011 = sc->shadowaddr[update_index].V206KG2D033;
	mbit.V206KG2D001     = sc->V206DEV143.format == V206FB002 ? 32 : 16;
	mbit.sx      = 0;
	mbit.sy      = 0;
	mbit.dx      = 0;
	mbit.dy      = 0;
	mbit.width   = sc->V206DEV143.width;
	mbit.height  = sc->V206DEV143.height;
	mbit.mask    = 0xffffffff;
	mbit.rrop    = 0xf0;

	if (FUNC206HAL477(pDev, &mbit)) {
		return -1;
	}

	pDev->V206DEV096.flush(pDev);

	sc->last_update_idx = update_index;

	return 0;
}

static void mwv206_shadow_try_update(V206DEV025 *pDev, int crtc, int from_timer)
{
	struct V206DEV026 *V206FB005 = (struct V206DEV026 *) pDev->fb_info;
	struct scanctrl *sc = &pDev->scanctrl[crtc];
	int from_draw = !from_timer;
	ktime_t now;

	if (!sc->shadow_on || !sc->crtc_connected || !sc->crtc_configured) {
		return;
	}


	if (V206FB005 && (V206FB005->V206DEV181 == sc->V206DEV143.addr)) {
		return;
	}

	if (from_draw) {
		sc->drawfb_dirty = 1;
	}

	now = ktime_get();

	if (ktime_before(ktime_add_us(now, 1900), ktime_add(sc->last_update_time, sc->vsync_interval))) {
		return;
	}

	if (from_timer && !sc->drawfb_dirty) {
		sc->display_pending = 0;
	} else {
		sc->display_pending = 1;
	}

	mwv206_shadow_update(pDev, crtc);

	sc->drawfb_dirty = 0;
}

static void mwv206_shadow_sync(struct work_struct *work)
{
	struct scanctrl *sc = container_of(work, struct scanctrl, work);
	V206DEV025 *pDev = sc->pDev;

	FUNC206HAL369(pDev, 1);
	if ((sc->drawfb_dirty || sc->display_pending) && !pDev->pm.V206DEV109 && !sc->drawing) {
		mwv206_shadow_try_update(pDev, sc->V206DEV143.V206FB011, 1);
	}
	FUNC206HAL369(pDev, 0);

}

void mwv206_shadow_try_sync(V206DEV025 *pDev)
{
	int crtc;

	FUNC206HAL369(pDev, 1);
	for (crtc = 0; crtc < 4; crtc++) {
		pDev->scanctrl[crtc].drawing = 0;
		mwv206_shadow_try_update(pDev, crtc, 0);
	}
	FUNC206HAL369(pDev, 0);
}

static int is_port_zoom_on(struct mwv206_port_config *pcfg)
{
	return V206DEVCONFIG040(pcfg->flags);
}

static void mwv206_shadow_init_zoom_flags(V206DEV025 *pDev)
{
	int port;

	for (port = 0; port < 4; port++) {
		if (is_port_zoom_on(&(pDev->V206DEV105.hdmi[port]))) {
			pDev->zoom_on = 1;
			return;
		}
	}

	for (port = 0; port < 2; port++) {
		if (is_port_zoom_on(&(pDev->V206DEV105.vga[port]))) {
			pDev->zoom_on = 1;
			return;
		}
	}

	for (port = 0; port < 2; port++) {
		if (is_port_zoom_on(&(pDev->V206DEV105.lvds[port]))) {
			pDev->zoom_on = 1;
			return;
		}
	}

	for (port = 0; port < 2; port++) {
		if (is_port_zoom_on(&(pDev->V206DEV105.dvo[port]))) {
			pDev->zoom_on = 1;
			return;
		}
	}
}

void mwv206_shadow_init(V206DEV025 *pDev)
{
	int crtc;

	if (pDev->wq) {
		return;
	}

	pDev->wq = create_singlethread_workqueue("mwv206_vsync_worker");
	if (!pDev->wq) {
		V206KDEBUG002("MWV206: failed to create vsync worker\n");
		pDev->disable_vblank_sync = 1;
		return;
	}


	if (mwv206_dev_cnt_get() == 2) {
		disable_vblank_sync = 1;
	}

	mwv206_shadow_init_zoom_flags(pDev);

	FUNC206HAL369(pDev, 1);
	for (crtc = 0; crtc < 4; crtc++) {
		struct scanctrl *sc = &pDev->scanctrl[crtc];

		INIT_WORK(&sc->work, mwv206_shadow_sync);
		sc->pDev = pDev;

		if (ktime_before(sc->vsync_interval, ktime_set(0, 1000000))) {
			sc->vsync_interval = ktime_set(0, 1000000000UL / 60);
		}

		hrtimer_init(&sc->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		sc->timer.function = mwv206_shadow_periodic_sync;
	}
	FUNC206HAL369(pDev, 0);
}

void mwv206_shadow_deinit(V206DEV025 *pDev)
{

	if (!pDev) {
		return;
	}

	if (pDev->wq) {
		destroy_workqueue(pDev->wq);
		pDev->wq = NULL;
	}

}

int FUNC206HAL388(V206DEV025 *pDev, long arg)
{
	V206IOCTL159 *setdispaddr;
	struct scanctrl *sc;
	int  len, crtc;

	setdispaddr = (V206IOCTL159 *)arg;

	V206DEV005("setdispaddr: 0x%x\n", setdispaddr->addr);

	crtc = setdispaddr->V206FB011;
	if (crtc < 0 || crtc > 3) {
		V206KDEBUG002("MWV206: invalid crtc:%d\n", crtc);
		return -1;
	}

	len = sizeof(V206IOCTL159);
	sc = &pDev->scanctrl[crtc];

	FUNC206HAL369(pDev, 1);
	if (memcmp(&sc->V206DEV143, setdispaddr, len) == 0) {
		FUNC206HAL369(pDev, 0);
		return 0;
	}
	memcpy(&sc->V206DEV143, setdispaddr, len);

	mwv206_set_scan_addr(pDev, crtc, 0);

	if (sc->shadow_on) {
		mwv206_shadow_teardown(pDev, crtc);
	}

	if (mwv206_shadow_setup(pDev, crtc) == 0) {
		sc->shadow_on = 1;
	} else {
		sc->shadow_on = 0;
	}


	FUNC206HAL369(pDev, 0);

	return 0;
}



static int FUNC206HAL460(
		unsigned int targetOutFreq,
		int *pRefDiv,
		int *pFbIntDiv,
		int *pFbRacDiv,
		int *pPostDiv1,
		int *pPostDiv2)
{
	unsigned int refDiv, postDiv1, postDiv2;
	unsigned int vcoFreq;
	unsigned int fbRacDiv = 0;
	unsigned int compTargetOutFreq;
	unsigned int tempTargetFreq, mError;
	unsigned int mPostDiv1 = 0;
	unsigned int mPostDiv2 = 0;
	unsigned int mRefDiv = 0;
	unsigned int mFbIntDIV = 0;
	unsigned int mIntError;
	unsigned int mFbRacDIV = 0;
	unsigned int fbIntDiv = 0;
	unsigned int refDiv_base = 1;

	if (targetOutFreq > 1000000) {
		targetOutFreq /= 10000;
		refDiv_base = 1;
	} else {

		targetOutFreq *= 100;
		refDiv_base = 2;

	}

	compTargetOutFreq = targetOutFreq;

	if (NULL == pRefDiv
			|| NULL == pFbIntDiv
			|| NULL == pFbRacDiv
			|| NULL == pPostDiv1
			|| NULL == pPostDiv2)   {
		return -1;
	}

	if (1600u > compTargetOutFreq || 320000u < compTargetOutFreq) {

		V206DEV004(V206KDEBUG006, "[ERROR] targetOutFreq is outrange of 12MHz ~ 2.4GHz\n");
		return -2;
	}

	V206DEV004(V206KDEBUG008, "targetOutFreq=%d\n", targetOutFreq);


	postDiv1 = 1;
	postDiv2 = 1;
	refDiv = 1;
	mIntError = targetOutFreq / 200;
	if (mIntError < 2) {
		mIntError = 2;
	}
	for (postDiv2 = 7; postDiv2 > 0; postDiv2--) {
		for (postDiv1 = 7; postDiv1 > 0; postDiv1--) {
			for (refDiv = refDiv_base; refDiv < 64 ; refDiv++) {
				unsigned int temp;
				vcoFreq = targetOutFreq * postDiv1 * postDiv2;

				if (80000u > vcoFreq || 320000u < vcoFreq) {
					continue;
				}

				V206DEV004(V206KDEBUG008, "####################################################\n");
				V206DEV004(V206KDEBUG008, "postdiv1=%d, postdiv2=%d\n", postDiv1, postDiv2);

				fbIntDiv = (vcoFreq * refDiv) / 10000u;
				fbRacDiv = (vcoFreq * refDiv) % 10000u;
				if (((10000u / refDiv) < 1)
						|| ((10000u / refDiv) > (vcoFreq / 16))) {
					continue;
				}

				if (fbRacDiv == 0) {

					if ((fbIntDiv < 16) || (fbIntDiv > 3200)) {

						continue;
					}
					if (postDiv2 > postDiv1) {

						temp = postDiv1;
						postDiv1 = postDiv2;
						postDiv2 = temp;
					}
					mPostDiv1 = postDiv1;
					mPostDiv2 = postDiv2;
					mRefDiv = refDiv;
					mFbIntDIV = fbIntDiv;
					mFbRacDIV = fbRacDiv;
					goto CALCULATE_FREQDIV_SUCCESSED;
				}
			}
		}
	}


	postDiv1 = 1;
	postDiv2 = 1;
	refDiv = 1;
	mIntError = targetOutFreq / 200;
	if (mIntError < 2) {
		mIntError = 2;
	}
	for (postDiv2 = 7; postDiv2 > 0; postDiv2--) {
		for (postDiv1 = 7; postDiv1 > 0; postDiv1--) {

			for (refDiv = 1; refDiv < 11 ; refDiv++) {
				unsigned int temp;
				vcoFreq = targetOutFreq * postDiv1 * postDiv2;

				if (80000u > vcoFreq || 320000u < vcoFreq) {
					continue;
				}

				V206DEV004(V206KDEBUG008, "####################################################\n");
				V206DEV004(V206KDEBUG008, "postdiv1=%d, postdiv2=%d\n", postDiv1, postDiv2);

				fbIntDiv = (vcoFreq * refDiv) / 10000u;
				fbRacDiv = (vcoFreq * refDiv) % 10000u;

				if (((10000u / refDiv) < 10)
				    || ((10000u / refDiv) > (vcoFreq / 21))) {
					continue;
				}

				if (fbRacDiv != 0) {
					if ((fbIntDiv < 20) || (fbIntDiv > 320)) {

						continue;
					}

					temp = vcoFreq * refDiv;
					while (temp >= 10000u) {
						temp -= 10000u;
					}
					fbRacDiv = 0;
					{
						int bits = 24;
						while (bits >= 0) {
							if (temp >= 10000u) {
								fbRacDiv += 1 << bits;
								temp -= 10000u;
							}
							temp *= 2;
							bits--;
						}
					}


					tempTargetFreq = fbIntDiv * 10000u;
					temp = 10000u;
					{
						int bits = 24;
						while (bits >= 0) {
							if (fbRacDiv & (1 << bits)) {
								tempTargetFreq += temp;
							}
							bits--;
							temp /= 2;
						}
					}
					tempTargetFreq /= refDiv * postDiv1 * postDiv2;
					if (tempTargetFreq >= compTargetOutFreq) {
						mError = tempTargetFreq - compTargetOutFreq;
					} else {
						mError = compTargetOutFreq - tempTargetFreq;
					}

					if (mError < mIntError) {
						if (postDiv2 > postDiv1) {

							temp = postDiv1;
							postDiv1 = postDiv2;
							postDiv2 = temp;
						}
						mPostDiv1 = postDiv1;
						mPostDiv2 = postDiv2;
						mRefDiv = refDiv;
						mFbIntDIV = fbIntDiv;
						mIntError = mError;
						mFbRacDIV = fbRacDiv;
						goto CALCULATE_FREQDIV_SUCCESSED;
					} else {
						V206KDEBUG002("[%s:%d] no handled.\n", __FILE__, __LINE__);
						return -3;
					}
				}
			}
		}
	}

CALCULATE_FREQDIV_SUCCESSED:
	*pRefDiv = mRefDiv;
	*pFbIntDiv = mFbIntDIV;
	*pPostDiv1 = mPostDiv1;
	*pPostDiv2 = mPostDiv2;
	*pFbRacDiv = mFbRacDIV;

	V206DEV004(V206KDEBUG007, "\n*******************************************\n");
	V206DEV004(V206KDEBUG007, "targetOutFreq : %d\n", targetOutFreq);
	V206DEV004(V206KDEBUG007, "refdiv : %d\n",    mRefDiv);
	V206DEV004(V206KDEBUG007, "postdiv1 : %d\n", mPostDiv1);
	V206DEV004(V206KDEBUG007, "postdiv2 : %d\n", mPostDiv2);
	V206DEV004(V206KDEBUG007, "fbintdiv : %d\n", fbIntDiv);
	V206DEV004(V206KDEBUG007, "fbfracdiv : %d\n", mFbRacDIV);
	V206DEV004(V206KDEBUG007, "freq error : %d\n", mIntError);

	V206DEV004(V206KDEBUG007, "*******************************************\n");

	return 0;
}


int FUNC206HAL136(V206DEV025 *pDev,
		e_mwv206_pll_t pllID,
		int freq)
{
	int result = 0;
	GLJ_TICK tick;
	unsigned int value;
	int refDiv, fbIntDiv, fbRacDiv, postDiv1, postDiv2;

	result = FUNC206HAL460(freq, &refDiv, &fbIntDiv, &fbRacDiv, &postDiv1, &postDiv2);

	if (0 > result) {
		V206KDEBUG002("[ERROR] calculate the div parameters failed \n");
		return -2;
	}

	FUNC206HAL382(pDev, 1);
	V206DEV002(0x400CB0, 0);
	V206DEV002(0x400C44, pllID);

	V206DEV002(0x400C00, refDiv);
	V206DEV002(0x400C04, fbIntDiv);
	V206DEV002(0x400C08, fbRacDiv);
	V206DEV002(0x400C0C, postDiv1);
	V206DEV002(0x400C10, postDiv2);

	if (fbRacDiv == 0) {
		V206DEV002(0x400C1C, 1);
	} else {
		V206DEV002(0x400C1C, 0);
	}
	V206DEV002(0x400C48, 0);
	V206DEV002(0x400C40, 1);



#define MAC206HAL221     300



	value = V206DEV001(0x400CB0);
	mwv206_timed_loop (tick, (value & 0x1) != 1, MAC206HAL221) {
		value = V206DEV001(0x400CB0);
	}

	FUNC206HAL382(pDev, 0);

	if ((value & 0x1) != 1) {
		V206KDEBUG002("[ERROR] wait pll %d locked timeout[%08x]\n", pllID, value);
		return -3;
	}

	return 0;
}
int mwv206_resethdmiphy(V206DEV025 *pDev, int mask)
{
	int formerval, V206IOCTLCMD010, tempval;


	formerval = V206DEV001(((0x403000) + 0x0A00));
	tempval = (~(mask << 4));
	V206IOCTLCMD010 = formerval & tempval;
	V206DEV002(((0x403000) + 0x0A00), V206IOCTLCMD010);
	mdelay(20);

	formerval = V206DEV001(((0x403000) + 0x0A00));
	tempval = (mask << 4);
	V206IOCTLCMD010 = formerval | tempval;
	V206DEV002(((0x403000) + 0x0A00), V206IOCTLCMD010);

	return 0;
}

static void mwv206_set_output_voltage(V206DEV025 *pDev, int V206HDMIAUDIO027, int voltage)
{
	switch (V206HDMIAUDIO027) {
	case 0:
		V206DEV002(((0x403000) + 0x040C), voltage);
		break;
	case 1:
		V206DEV002(((0x403000) + 0x050C), voltage);
		break;
	case 2:
		V206DEV002(((0x403000) + 0x060C), voltage);
		break;
	case 3:
		V206DEV002(((0x403000) + 0x070C), voltage);
		break;
	default:
		V206KDEBUG002("invalid hdmi id.\n");
		break;
	}
}

static void mwv206_save_hdmimode(V206DEV025 *pDev, long arg)
{
	V206IOCTL161 dispmode;
	V206IOCTL168 *V206FB004;

	int id;
	V206FB004 = (V206IOCTL168 *)arg;

	dispmode.V206FB011 = V206FB004->V206FB011;
	dispmode.mode = V206FB004->mode;
	dispmode.htotal = V206FB004->htotal;
	dispmode.hactive = V206FB004->hactive;
	dispmode.hfrontporch = V206FB004->hfrontporch;
	dispmode.hsync = V206FB004->hsync;
	dispmode.hpol = V206FB004->hpol;

	dispmode.vtotal = V206FB004->vtotal;
	dispmode.vactive = V206FB004->vactive;
	dispmode.vfrontporch = V206FB004->vfrontporch;
	dispmode.vsync = V206FB004->vsync;
	dispmode.vpol = V206FB004->vpol;
	dispmode.framerate = V206FB004->framerate;
	dispmode.V206DEV079 = 0;

	if (pDev->pm.V206DEV109 == 0) {
		id = V206FB004->V206FB011;
		memcpy(&pDev->pm.V206DEV115[id], (void *)&dispmode, sizeof(V206IOCTL161));
		pDev->pm.V206DEV110[id] = 1;
	}

	return;
}


 int mwv206_sethdmimode_params(V206DEV025 *pDev, long arg, int legacy_mode, int mask_ioctl)
{
	V206IOCTL168 *V206FB004;
	V206IOCTL172 setdispport;
	int V206FB011;
	int V206HDMIAUDIO027;
	int clk, pllclkindex;
	int val, formerval, V206IOCTLCMD010;
	int dualpixel;
	int hdmiport, j;
	int enable;
	int mask = 0xf;

	V206FB004 = (V206IOCTL168 *)arg;

	V206FB011     = V206FB004->V206FB011;
	V206HDMIAUDIO027    = V206FB004->V206HDMIAUDIO027;
	dualpixel = V206FB004->dualpixel;
	hdmiport  = MWV206_DP_HDMI_0 + V206HDMIAUDIO027 ;

	if (V206HDMIAUDIO027 > 3 || V206HDMIAUDIO027 < 0) {
		V206KDEBUG002("mwv206_hdmi_config error hdmiid\n");
		return -1;
	}
	if (mask_ioctl == 1) {
		mask = 0xf;
	} else if (mask_ioctl == 2) {
		mask = (0x1 << V206HDMIAUDIO027);
	}

	FUNC206HAL369(pDev, 1);

	if (pDev->pm.V206DEV109 == 0) {
		int id = V206FB004->V206HDMIAUDIO027;
		pDev->pm.V206DEV119[id] = *V206FB004;
		pDev->pm.V206DEV114[id] = 1;
		pDev->pm.hdmi_legacy_mode = legacy_mode;
	}

	enable = pDev->V206DEV144[hdmiport].enable;
	if (dualpixel) {
		enable = 1;
	}

	if (enable) {
		setdispport.V206FB011 = V206FB011;
		setdispport.V206FB008 = hdmiport;
		setdispport.enable = 0;
		setdispport.dualpixel = dualpixel;
		mwv206_setdisplayport_nolock(pDev, (long)&setdispport);
	}

	if (dualpixel && ((V206HDMIAUDIO027 == 2) || (V206HDMIAUDIO027 == 0))) {
		setdispport.V206FB008 = hdmiport + 1;
		mwv206_setdisplayport_nolock(pDev, (long)&setdispport);
	}

	if (MWV206_PORT_IS_HDMI(V206FB004->flags)) {
		mwv206_set_output_voltage(pDev, V206HDMIAUDIO027, pDev->hdmi_voltage[V206HDMIAUDIO027]);
	} else if (MWV206_PORT_IS_DVI(V206FB004->flags)) {
		mwv206_set_output_voltage(pDev, V206HDMIAUDIO027, JMD_FIX_DVI_VOLTAGE);
	}



	{

#if 0
		phystat = V206DEV001(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x3004)));
		if ((phystat != 0xF1) && (phystat != 0xF3)) {
			V206KDEBUG002("The display device is not  connected correctly!\n");
			FUNC206HAL369(pDev, 0);
			return -1;
		}
#endif

		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x4002)), 0x0);
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x4002)), 0xFF);

		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x4001)), 0xFF);


		formerval = V206DEV001((0x400248));
		val = ~(1 << V206HDMIAUDIO027);
		V206IOCTLCMD010 = formerval & val;
		V206DEV002((0x400248), V206IOCTLCMD010);
		val = 1 << V206HDMIAUDIO027;
		V206IOCTLCMD010 = formerval | val;
		V206DEV002((0x400248), V206IOCTLCMD010);

		formerval = V206DEV001(((0x400000) + 0x0014));
		val = V206FB011 << (V206HDMIAUDIO027 * 8);
		V206IOCTLCMD010 = (formerval & (~(0xFF << (V206HDMIAUDIO027 * 8)))) | val;
		V206DEV002(((0x400000) + 0x0014), V206IOCTLCMD010);

		val = V206FB004->hactive;
		V206IOCTLCMD010 = val >> 8;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1002)), V206IOCTLCMD010);
		V206IOCTLCMD010 = val;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1001)), V206IOCTLCMD010);

		val = V206FB004->vactive;
		V206IOCTLCMD010 = val >> 8;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1006)), V206IOCTLCMD010);
		V206IOCTLCMD010 = val;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1005)), V206IOCTLCMD010);

		val = V206FB004->htotal - V206FB004->hactive;
		V206IOCTLCMD010 = val >> 8;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1004)), V206IOCTLCMD010);
		V206IOCTLCMD010 = val;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1003)), V206IOCTLCMD010);

		val = V206FB004->vtotal - V206FB004->vactive;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1007)), val);

		val = V206FB004->hfrontporch;
		V206IOCTLCMD010 = val >> 8;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1009)), V206IOCTLCMD010);
		V206IOCTLCMD010 = val;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1008)), V206IOCTLCMD010);

		val = V206FB004->hsync;
		V206IOCTLCMD010 = val >> 8;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x100B)), V206IOCTLCMD010);
		V206IOCTLCMD010 = val;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x100A)), V206IOCTLCMD010);

		val = V206FB004->vfrontporch;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x100C)), val);

		val = V206FB004->vsync;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x100D)), val);

		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1011)), 0x0C);
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1012)), 0x20);
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1013)), 0x0F);

		val = (V206FB004->framerate) * 1000;
		V206IOCTLCMD010 = val >> 16;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1010)), V206IOCTLCMD010);
		V206IOCTLCMD010 = val >> 8;
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x100F)), V206IOCTLCMD010);
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x100E)), val);

		formerval = V206DEV001(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1000)));
		val = 1 << 3;
		if (legacy_mode) {
			if (V206FB004->flags) {
				V206IOCTLCMD010 = formerval & (~val);
			} else {
				V206IOCTLCMD010 = formerval | val;
				pDev->pixelclock[V206HDMIAUDIO027] = ((unsigned int)V206FB004->htotal * (unsigned int)V206FB004->vtotal *
						(unsigned int)V206FB004->framerate);
			}
		} else {
			if (MWV206_PORT_IS_DVI(V206FB004->flags)) {
				V206IOCTLCMD010 = formerval & (~val);
			} else {
				V206IOCTLCMD010 = formerval | val;
				if (MWV206_PORT_AUDIO_PRESENT(V206FB004->flags)) {
					pDev->pixelclock[V206HDMIAUDIO027] = ((unsigned int)V206FB004->htotal * (unsigned int)V206FB004->vtotal *
							(unsigned int)V206FB004->framerate);
				}
			}
		}
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1000)), V206IOCTLCMD010);



		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1019)), 0);

		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x101A)), 8);


		val = mwv206_get_vic(V206FB004);
		if (val >= 0 && val <= 64) {
			V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x101C)), val);
		}


		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x4001)), 0);

		val = V206DEV001(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1000)));
		V206DEV002(MWV206_HDMI_REG(V206HDMIAUDIO027, (0x1000)), val);

	}

	for (j = 0; j < 2; j++) {


		clk = pDev->V206DEV093[V206FB011];
		if (dualpixel) {
			clk /= 2;
		}
		pllclkindex = FUNC206HAL037(clk);
		if (pllclkindex == CLK_FREQ_MAX) {
			V206KDEBUG002("pllclkindex == CLK_FREQ_MAX return -1\n");
			FUNC206HAL369(pDev, 0);
			return -1;
		}

		if (FUNC206HAL350(pDev, V206FB011, V206HDMIAUDIO027, pllclkindex, dualpixel, V206FB004->hpol, V206FB004->vpol) != 0) {
			V206KDEBUG002("mwv206_hdmi_config_phy_pll return -1\n");
			FUNC206HAL369(pDev, 0);
			return -1;
		}


		if (enable) {
			setdispport.enable = 1;
			mwv206_setdisplayport_nolock(pDev, (long)&setdispport);
			if (FUNC206HAL349(pDev, V206HDMIAUDIO027, pllclkindex) == 0) {
				break;
			} else {
				setdispport.enable = 0;
				mwv206_setdisplayport_nolock(pDev, (long)&setdispport);
			}
		}
	}


	if (legacy_mode) {
		if (!V206FB004->flags && pDev->V206DEV155) {
			FUNC206LXDEV138(pDev, V206HDMIAUDIO027);
			FUNC206LXDEV021(pDev, V206HDMIAUDIO027, 0);
		}
	} else {
		if (MWV206_PORT_AUDIO_PRESENT(V206FB004->flags) && pDev->V206DEV155) {
			FUNC206LXDEV138(pDev, V206HDMIAUDIO027);
			FUNC206LXDEV021(pDev, V206HDMIAUDIO027, 0);
		}
	}
	if (dualpixel == 0) {
		mwv206_resethdmiphy(pDev, mask);
	}
	FUNC206HAL369(pDev, 0);
	return 0;
}

int FUNC206HAL396(V206DEV025 *pDev, long arg, int legacy_mode, int mask_ioctl)
{
	V206IOCTL168 *V206FB004;
	int V206FB011;
	int V206HDMIAUDIO027;

	V206FB004 = (V206IOCTL168 *)arg;

	V206FB011     = V206FB004->V206FB011;
	V206HDMIAUDIO027    = V206FB004->V206HDMIAUDIO027;

	if (V206HDMIAUDIO027 > 3 || V206HDMIAUDIO027 < 0) {
		V206KDEBUG002("mwv206_hdmi_config error hdmiid\n");
		return -1;
	}

	mwv206_save_hdmimode(pDev, arg);

	FUNC206HAL369(pDev, 1);

	FUNC206HAL192(pDev, V206FB011,
			V206FB004->htotal, V206FB004->hactive, V206FB004->hfrontporch, V206FB004->hsync,
			V206FB004->vtotal, V206FB004->vactive, V206FB004->vfrontporch, V206FB004->vsync,
			V206FB004->framerate, 0, 1);
	FUNC206HAL369(pDev, 0);

	return mwv206_sethdmimode_params(pDev, arg, legacy_mode, mask_ioctl);
}

#define   V206IOCTLDISPLAY001       (0x1002<<2)
#define   V206IOCTLDISPLAY002       (0x1001<<2)
#define   V206IOCTLDISPLAY003       (0x1006<<2)
#define   V206IOCTLDISPLAY004       (0x1005<<2)
#define   V206IOCTLDISPLAY005      (0x1003<<2)
#define   V206IOCTLDISPLAY006      (0x1004<<2)
#define   V206IOCTLDISPLAY007       (0x1007<<2)
#define   V206IOCTLDISPLAY008  (0x1008<<2)
#define   V206IOCTLDISPLAY009  (0x1009<<2)
#define   V206IOCTLDISPLAY010   (0x100c<<2)
#define   V206IOCTLDISPLAY011  (0x100a<<2)
#define   V206IOCTLDISPLAY012  (0x100b<<2)
#define   V206IOCTLDISPLAY013   (0x100d<<2)
#define   V206IOCTLDISPLAY014        (0x100e<<2)
#define   V206IOCTLDISPLAY015        (0x100f<<2)
#define   V206IOCTLDISPLAY016        (0x1010<<2)
#define   V206IOCTLDISPLAY017       (0x101a<<2)
#define   V206IOCTLDISPLAY018      (0x1000<<2)
#define   V206IOCTLDISPLAY019        (0x1011<<2)
#define   V206IOCTLDISPLAY020      (0x1012<<2)
#define   V206IOCTLDISPLAY021     (0x1013<<2)

static void FUNC206HAL103(V206DEV025 *pDev, int sel)
{
	V206DEV002(V206IOCTLDISPLAY001 + sel, 4);
	V206DEV002(V206IOCTLDISPLAY002 + sel, 1024);
	V206DEV002(V206IOCTLDISPLAY003 + sel, 3);
	V206DEV002(V206IOCTLDISPLAY004 + sel, 768);
	V206DEV002(V206IOCTLDISPLAY006 + sel, 1);
	V206DEV002(V206IOCTLDISPLAY005 + sel, 280);
	V206DEV002(V206IOCTLDISPLAY007 + sel, 45);
	V206DEV002(V206IOCTLDISPLAY009 + sel, 0);
	V206DEV002(V206IOCTLDISPLAY008 + sel, 88);
	V206DEV002(V206IOCTLDISPLAY010 + sel, 5);
	V206DEV002(V206IOCTLDISPLAY012 + sel, 0);
	V206DEV002(V206IOCTLDISPLAY011 + sel, 44);
	V206DEV002(V206IOCTLDISPLAY013 + sel, 5);
	V206DEV002(V206IOCTLDISPLAY016 + sel, 0);
	V206DEV002(V206IOCTLDISPLAY015 + sel, 234);
	V206DEV002(V206IOCTLDISPLAY014 + sel, 60000);
	V206DEV002(V206IOCTLDISPLAY018 + sel, (1 << 6) | (1 << 5)
			| (1 << 4) | (1 << 3)
			| (2 << 1) |  0);

	V206DEV002(V206IOCTLDISPLAY019 + sel, 0x0C);
	V206DEV002(V206IOCTLDISPLAY020 + sel, 0x20);
	V206DEV002(V206IOCTLDISPLAY021 + sel, 0x0F);

	V206KDEBUG003("[INFO] init hdmi Select done 0x%x.", sel);

}

void FUNC206HAL351(V206DEV025 *pDev)
{
	int value;
	V206DEV002(0x40024C, 0xffffffff);
	V206DEV005("[INFO] video reset done.\n");

	value = V206DEV001(0x800008);
	if (value == 0) {
		V206DEV002(((0x403000) + 0x0408), 0x0000333F);
		V206DEV002(((0x403000) + 0x0508), 0x0000333F);
		V206DEV002(((0x403000) + 0x0608), 0x0000333F);
		V206DEV002(((0x403000) + 0x0708), 0x0000333F);
		V206DEV002(((0x403000) + 0x040C), 0x9000007F);
		V206DEV002(((0x403000) + 0x050C), 0x9000007F);
		V206DEV002(((0x403000) + 0x060C), 0x9000007F);
		V206DEV002(((0x403000) + 0x070C), 0x9000007F);
	}


	V206DEV002(0x400034, 1);


	V206DEV002(((0x400000) + 0x004C), 0x00000000);
	V206DEV002(((0x400000) + 0x014C), 0x00000000);


	pDev->hdmi_voltage[0] = V206DEV001(((0x403000) + 0x040C));
	pDev->hdmi_voltage[1] = V206DEV001(((0x403000) + 0x050C));
	pDev->hdmi_voltage[2] = V206DEV001(((0x403000) + 0x060C));
	pDev->hdmi_voltage[3] = V206DEV001(((0x403000) + 0x070C));
	return;
}



#define MAC206HAL155(reg, value)    V206DEV006(devInfo, reg, value)
#define MAC206HAL154(reg) V206DEV007(devInfo, reg)


#define V206IOCTLDISPLAY022   (0x400048)


#define V206IOCTLDISPLAY027 (0x400c50)

#define V206IOCTLDISPLAY024 (0x400C8C)


#define V206IOCTLDISPLAY025 (0x400C90)


#define V206IOCTLDISPLAY026 (0x400c80)


#define V206IOCTLDISPLAY023 (0x400c84)

#define bPLL_FBDIV_FRAC (0x400c88)

typedef struct FUNC206HAL030 {
	int POSTDIV1;
	int POSTDIV2;
	int min;
	int max;
} PLLRECONFPARAM;

static PLLRECONFPARAM FUNC206HAL100[] = {
	{2, 1, 0, 1600},
	{5, 4, 27,  700},
	{5, 4, 27,  700},
	{5, 4, 27,  700},
	{5, 4, 27,  700},
	{4, 1, 166, 533},
	{2, 1, 400, 1600},
	{4, 1, 50,  2350},
	{4, 1, 200, 800},
	{2, 1, 50,  1200},
};

static int FUNC206HAL010(V206DEV025 *devInfo, e_mwv206_pll_t index, int configfreq)
{
	int FBDIV = 0;
	int postdiv1 = 0;
	int postdiv2 = 0;
	int refdiv = 0;

	postdiv1 = MAC206HAL154(V206IOCTLDISPLAY024);
	postdiv2 = MAC206HAL154(V206IOCTLDISPLAY025);
	refdiv   = MAC206HAL154(V206IOCTLDISPLAY026);
	FBDIV    = configfreq * postdiv1 * postdiv2;
	FBDIV    = FBDIV * refdiv / 100;

	return FBDIV;
}

#define MAC206HAL222 (FUNC206LXDEV098()*5)


int FUNC206HAL135(V206DEV025 *devInfo, int plltype, int configfreq, int enable)
{
	int fbdiv;
	int value;
	GLJ_TICK tick;
	e_mwv206_pll_t index;
	index = plltype;

	if (index >= V206IOCTL015) {
		V206KDEBUG002("unsupport pll type! just support %d~%d\n",
				V206IOCTL005,
				V206IOCTL015-1);
		return -1;
	}

	if ((enable != 0) && (enable != 1)) {
		V206KDEBUG002("param 4 just support 0 or 1\n");
		return -4;
	}

	if ((configfreq < FUNC206HAL100[index].min)
			|| (configfreq > FUNC206HAL100[index].max)) {
		V206KDEBUG002("unsupport freq! just support %d~%d\n",
				FUNC206HAL100[index].min, FUNC206HAL100[index].max);
		return -2;
	}

	if (configfreq == 0) {
		enable = 0;
	}

	FUNC206HAL382(devInfo, 1);

	if (index == V206IOCTL013) {
		value = MAC206HAL154(V206IOCTLDISPLAY022);
		V206DEV005("PLL_default_freq %#x\n", value);
		value = value >> 16;
		V206DEV005("axi default pll %d\n", value);
		if (value == 600) {
			FUNC206HAL100[index].POSTDIV1 = 2;
		}
	}


	value = MAC206HAL154(V206IOCTLDISPLAY027);
	mwv206_timed_loop (tick, value != 0, MAC206HAL222) {
		value = MAC206HAL154(V206IOCTLDISPLAY027);
	}
	if (value != 0) {
		V206KDEBUG002("[ERROR] wait pll no busy timeout[%d]\n", value);
		FUNC206HAL382(devInfo, 0);
		return -3;
	}

	MAC206HAL155(0x400C44, index);
	if (enable == 1) {
		MAC206HAL155(0x400C48, 1);
	} else {
		MAC206HAL155(0x400C48, 2);
	}
	fbdiv = FUNC206HAL010(devInfo, index, configfreq);
	if (fbdiv != 0) {
		MAC206HAL155(0x400C04, fbdiv);
	}
	MAC206HAL155(0x400C40, 1);
	if (plltype == V206IOCTL005) {
		devInfo->V206DEV073 = configfreq;
	}
	FUNC206HAL382(devInfo, 0);
	return 0;
}


int FUNC206HAL134(V206DEV025 *devInfo, int plltype, int *freq)
{
	int postdiv1 = 0;
	int postdiv2 = 0;
	int refdiv = 0;
	int fbdiv = 0;
	int fbdiv_frac;

	FUNC206HAL382(devInfo, 1);

	MAC206HAL155(0x400C44, plltype);

	postdiv1 = MAC206HAL154(V206IOCTLDISPLAY024);
	postdiv2 = MAC206HAL154(V206IOCTLDISPLAY025);
	refdiv   = MAC206HAL154(V206IOCTLDISPLAY026);
	fbdiv    = MAC206HAL154(V206IOCTLDISPLAY023);
	fbdiv_frac   = MAC206HAL154(bPLL_FBDIV_FRAC);

	*freq    = (100 * fbdiv + 100UL * fbdiv_frac / (1 << 24)) / (refdiv * postdiv1 * postdiv2);
	V206DEV005("\n0x%x: 0x%x, 0x%x: 0x%x,0x%x: 0x%x, 0x%x: 0x%x.\n",
			V206IOCTLDISPLAY024, postdiv1, V206IOCTLDISPLAY025, postdiv2,
			V206IOCTLDISPLAY026, refdiv, V206IOCTLDISPLAY023, fbdiv);
	V206DEV005("\nfreq %d\n", *freq);
	FUNC206HAL382(devInfo, 0);
	return 0;
}



int FUNC206HAL381(V206DEV025 *pDev, long arg)
{
	V206IOCTL180 *kpll = (V206IOCTL180 *)arg;
	int ret;
	V206DEV005("plltype %d, configfreq %d, enable %d\n", kpll->plltype, kpll->configfreq, kpll->enable);
	FUNC206LXDEV128(FUNC206LXDEV098() / 10);
	ret = FUNC206HAL135(pDev, kpll->plltype, kpll->configfreq, kpll->enable);
	FUNC206LXDEV128(FUNC206LXDEV098() / 10);
	return ret;
}


int FUNC206HAL380(V206DEV025 *pDev, long arg)
{
	V206IOCTL179 *cpll = (V206IOCTL179 *)arg;
	int ret;
	V206DEV005("plltype %d\n", cpll->plltype);
	ret = FUNC206HAL134(pDev, cpll->plltype, &cpll->freq);
	cpll->maxfreq = FUNC206HAL100[cpll->plltype].max;
	cpll->minfreq = FUNC206HAL100[cpll->plltype].min;

	V206DEV005("pllVAL %d\n", cpll->freq);
	return ret;
}

int FUNC206HAL333(V206DEV025 *pDev, long arg)
{
	V206KDEBUG003("fall back to show fb\n");
	mwv206fb_active(pDev);
	return 0;
}
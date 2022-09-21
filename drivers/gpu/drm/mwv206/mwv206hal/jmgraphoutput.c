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
#include "jmgraphoutput.h"
#include "gljos.h"
#include "videoadd_reg.h"
#include "mwv206dev.h"

#define MAC206HAL206         0

#define MAC206HAL159   100

#define MAC206HAL218      1
#define MAC206HAL217     0

#define MAC206HAL220    1
#define MAC206HAL219   0

#define MAC206HAL009(x, minx, maxx)    \
	do { \
		if (x < minx) { \
			x = minx; \
		} \
		if (x > maxx) { \
			x = maxx; \
		} \
	} while (0)



#define MAC206HAL158(reg, value) \
	do {\
		V206DEV025 *pDev = dev;\
		V206DEV002(reg, value);\
	} while (0)

#define MAC206HAL157(reg) \
	do {\
		V206DEV025 *pDev = dev;\
		value = V206DEV001(reg);\
	} while (0)

#define MAC206HAL023         0x001
#define MAC206HAL163         0x002
#define MAC206HAL031    0x100
#define VIDEO_PALETTE_OFFSET(index)    (((0x14 * (index) + (index >> 1) * 0x24)))


typedef struct FUNC206HAL025 {
	unsigned int    mScreenWidth;
	unsigned int    mScreenHeight;
	unsigned int    mVFrequency;
	unsigned int    mVFreqActual;
	unsigned int    mHFrequency;
	unsigned int    mClock;
	unsigned int    mScreenFlag;
	unsigned int    mPixelClock;
} MWV206_SCREEN_DISPLAY_MODE, *P_MWV206_SCREEN_DISPLAY_MODE;


typedef enum FUNC206HAL024 {
	MWV206_VIDEO_OUTPUT_BLUESCREEN = 0,
	MWV206_VIDEO_OUTPUT_GRAPH,
	MWV206_VIDEO_OUTPUT_CONTENT_TYPE_NUMBER
} MWV206_OUTPUT_CONTENT_TYPE;




typedef struct FUNC206HAL026 {
	unsigned int        mHBlankBegin;
	unsigned int        mHBlankEnd;
	unsigned int        mHSBegin;
	unsigned int        mHSEnd;
	unsigned int        mHTotal;
	unsigned int        mVBlankBegin;
	unsigned int        mVBlankEnd;
	unsigned int        mVSBegin;
	unsigned int        mVSEnd;
	unsigned int        mVTotal;
} DISPLAY_MODE_CONFIG_VALUE, *P_DISPLAY_MODE_CONFIG_VALUE;


typedef struct FUNC206HAL027 {
	MWV206_SCREEN_DISPLAY_MODE  mDisplay;
	DISPLAY_MODE_CONFIG_VALUE   mConfigParam;
} MWV206_VADD_DISPCONFIG_INFO, *P_MWV206_VADD_DISPCONFIG_INFO;



static int FUNC206HAL186(V206DEV025 *dev,
		int screen,
		int isDoublePixel,
		P_DISPLAY_MODE_CONFIG_VALUE pConfigInfo)
{
	volatile unsigned int value;
	unsigned int V206DEV033;
	unsigned int WriteValue;

	V206DEV033 = 0x9400 + OUTPUT_REG_OFFSET(screen);
	if (0 != isDoublePixel) {
		WriteValue = (pConfigInfo->mHSBegin / 2) | ((pConfigInfo->mHSEnd / 2) << 16);
	} else {
		WriteValue = pConfigInfo->mHSBegin | (pConfigInfo->mHSEnd << 16);
	}
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x9404 + OUTPUT_REG_OFFSET(screen);
	if (0 != isDoublePixel) {
		WriteValue = (pConfigInfo->mHBlankBegin / 2) | ((pConfigInfo->mHBlankEnd / 2) << 16);
	} else {
		WriteValue = pConfigInfo->mHBlankBegin | (pConfigInfo->mHBlankEnd << 16);
	}
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x9408 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	WriteValue = value & 0xFFFF0000;
	if (0 != isDoublePixel) {
		WriteValue |= (pConfigInfo->mHTotal / 2);
	} else {
		WriteValue |= pConfigInfo->mHTotal;
	}
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x940C + OUTPUT_REG_OFFSET(screen);
	WriteValue = pConfigInfo->mVBlankBegin | (pConfigInfo->mVBlankEnd << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x9458 + OUTPUT_REG_OFFSET(screen);
	WriteValue = pConfigInfo->mVBlankBegin | (pConfigInfo->mVBlankEnd << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x9418 + OUTPUT_REG_OFFSET(screen);
	WriteValue = pConfigInfo->mVSBegin | (pConfigInfo->mVSEnd << 16);
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x9414 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	WriteValue = value & 0xFFFF0000;
	WriteValue |= pConfigInfo->mVTotal;
	MAC206HAL158(V206DEV033, WriteValue);

	return 0;
}


static int FUNC206HAL188(V206DEV025 *dev,
		int screen,
		int isDoublePixel)
{
	volatile unsigned int value;
	unsigned int V206DEV033;
	unsigned int WriteValue;

	V206DEV033 = 0x9400 + OUTPUT_REG_OFFSET(screen);
	if (0 != isDoublePixel) {
		WriteValue = 264 | (286 << 16);
	} else {
		WriteValue = 528 | (572 << 16);
	}
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x9404 + OUTPUT_REG_OFFSET(screen);
	if (0 != isDoublePixel) {
		WriteValue = 0 | (360 << 16);
	} else {
		WriteValue = 0 | (720 << 16);
	}
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x9408 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	WriteValue = value & 0xFFFF0000;
	if (0 != isDoublePixel) {
		WriteValue = 1320 | (660 << 16);
	} else {
		WriteValue = 2640 | (1320 << 16);
	}
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x940C + OUTPUT_REG_OFFSET(screen);
	WriteValue = 2246 | (40 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x9410 + OUTPUT_REG_OFFSET(screen);
	WriteValue = 1120 | (1166 << 16);
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x9418 + OUTPUT_REG_OFFSET(screen);
	WriteValue = 0 | (10 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x941C + OUTPUT_REG_OFFSET(screen);
	WriteValue = 1125 | (1130 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x9414 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	WriteValue = 2250 | (25 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	return 0;
}



static int FUNC206HAL190(V206DEV025 *dev,
		int screen,
		int isDoublePixel)
{
	volatile unsigned int value;
	unsigned int V206DEV033;
	unsigned int WriteValue;

	V206DEV033 = 0x9400 + OUTPUT_REG_OFFSET(screen);
	if (0 != isDoublePixel) {
		WriteValue = 6 | (38 << 16);
	} else {
		WriteValue = 12 | (76 << 16);
	}
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x9404 + OUTPUT_REG_OFFSET(screen);
	if (0 != isDoublePixel) {
		WriteValue = 0 | (72 << 16);
	} else {
		WriteValue = 0 | (144 << 16);
	}
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x9408 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	WriteValue = value & 0xFFFF0000;
	if (0 != isDoublePixel) {
		WriteValue = 432 | (216 << 16);
	} else {
		WriteValue = 864 | (432 << 16);
	}
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x940C + OUTPUT_REG_OFFSET(screen);
	WriteValue = 1245 | (44 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x9410 + OUTPUT_REG_OFFSET(screen);
	WriteValue = 620 | (670 << 16);
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x9418 + OUTPUT_REG_OFFSET(screen);
	WriteValue = 0 | (5 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x941C + OUTPUT_REG_OFFSET(screen);
	WriteValue = 625 | (630 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x9414 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	WriteValue = 1250 | (25 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	return 0;
}



static int FUNC206HAL189(V206DEV025 *dev,
		int screen,
		int isDoublePixel)
{
	volatile unsigned int value;
	unsigned int V206DEV033;
	unsigned int WriteValue;

	V206DEV033 = 0x9400 + OUTPUT_REG_OFFSET(screen);
	if (0 != isDoublePixel) {
		WriteValue = 9 | (28 << 16);
	} else {
		WriteValue = 19 | (57 << 16);
	}
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x9404 + OUTPUT_REG_OFFSET(screen);
	if (0 != isDoublePixel) {
		WriteValue = 0 | (69 << 16);
	} else {
		WriteValue = 0 | (138 << 16);
	}
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x9408 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	WriteValue = value & 0xFFFF0000;
	if (0 != isDoublePixel) {
		WriteValue = 429 | (429 << 16);
	} else {
		WriteValue = 858 | (429 << 16);
	}
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x940C + OUTPUT_REG_OFFSET(screen);
	WriteValue = 1048 | (42 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x9410 + OUTPUT_REG_OFFSET(screen);
	WriteValue = 522 | (568 << 16);
	MAC206HAL158(V206DEV033, WriteValue);


	V206DEV033 = 0x9418 + OUTPUT_REG_OFFSET(screen);
	WriteValue = 0 | (12 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x941C + OUTPUT_REG_OFFSET(screen);
	WriteValue = 531 | (537 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	V206DEV033 = 0x9414 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	WriteValue = 1050 | (13 << 16);
	MAC206HAL158(V206DEV033, WriteValue);

	return 0;
}


static int FUNC206HAL187(V206DEV025 *dev,
		int screen,
		int isDoublePixel,
		int isInterleaved,
		int pVAddDevInfo__,
		P_MWV206_VADD_DISPCONFIG_INFO pConfigInfo,
		MWV206_OUTPUT_CONTENT_TYPE outputContent)
{
	int result;
	unsigned int V206DEV033;
	unsigned int value;


	unsigned int newPixelColock;

	if (0 == pConfigInfo->mDisplay.mClock) {
		newPixelColock = (unsigned int)pConfigInfo->mConfigParam.mHTotal * (unsigned int)pConfigInfo->mConfigParam.mVTotal *
			(unsigned int)pConfigInfo->mDisplay.mVFrequency;
	} else {
		newPixelColock = (unsigned int)(pConfigInfo->mDisplay.mPixelClock);
	}

	if (0 != isDoublePixel) {
		newPixelColock /= 2;
	}

	if (1 == isInterleaved) {
		newPixelColock /= 2;
	}

	dev->V206DEV093[screen] = newPixelColock / 1000000;
	V206DEV005("[INFO]:pllfreq[%d]:%d\n", screen, dev->V206DEV093[screen]);
	result = FUNC206HAL136(dev, V206IOCTL006 + screen, newPixelColock);
	if (0 != result) {
		V206KDEBUG002("[ERROR] config pll screen failed : %dHz, result = %d\n", newPixelColock, result);
		return result;
	}


	V206DEV033 = 0x9464 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL158(V206DEV033, isInterleaved);

	V206DEV033 = 0x9420 + OUTPUT_REG_OFFSET(screen);

	MAC206HAL158(V206DEV033, outputContent);


	V206DEV033 = 0x943C + OUTPUT_REG_OFFSET(screen);
	value = pConfigInfo->mDisplay.mScreenHeight - 1;
	value = value << 16;
	value |= pConfigInfo->mDisplay.mScreenWidth - 1;
	MAC206HAL158(V206DEV033, value);


	if (1 == isInterleaved) {
		if (720 == pConfigInfo->mDisplay.mScreenWidth
				&& 288 == pConfigInfo->mDisplay.mScreenHeight) {
			if (50 == pConfigInfo->mDisplay.mVFrequency) {
				result = FUNC206HAL190(dev, screen, isDoublePixel);
			} else if (60 == pConfigInfo->mDisplay.mVFrequency) {
				result = FUNC206HAL189(dev, screen, isDoublePixel);
			}
		} else if (1920 == pConfigInfo->mDisplay.mScreenWidth
				&& 1080 == pConfigInfo->mDisplay.mScreenHeight) {
			result = FUNC206HAL188(dev, screen, isDoublePixel);
		}
	} else {
		result = FUNC206HAL186(dev, screen, isDoublePixel, &pConfigInfo->mConfigParam);
	}

	return result;
}



int FUNC206HAL191(V206DEV025 *dev, unsigned int channel, int paletteno, unsigned char data[768])
{
	volatile unsigned int value;
	unsigned int palette_ram_sel_regaddr = 0, palette_wr_data_regaddr = 0, palette_fifo_aclr_regaddr = 0,
		     palette_rgb_sel_regaddr = 0, palette_fifo_used_regaddr = 0;
	int rgb, curPaletteRam;
	unsigned int palette_enable_regaddr = 0;
	GLJ_TICK    nTick;
	int nWrited = 0;

	int i, writeBlockNumber;
	unsigned char *pWriteData = 0;

	if ((dev == NULL) || (4 <= channel) || (0 == data) || (3 <= paletteno)) {
		return -1;
	}

	switch (paletteno) {
	case 0:
		palette_ram_sel_regaddr = 0x9440 + OUTPUT_REG_OFFSET(channel);
		palette_wr_data_regaddr = 0x9444 + OUTPUT_REG_OFFSET(channel);
		palette_fifo_aclr_regaddr = 0x9448 + OUTPUT_REG_OFFSET(channel);
		palette_rgb_sel_regaddr = 0x944C + OUTPUT_REG_OFFSET(channel);
		palette_fifo_used_regaddr = 0x9450 + OUTPUT_REG_OFFSET(channel);
		palette_enable_regaddr = 0x9460 + OUTPUT_REG_OFFSET(channel);
		break;

	case 1:

		palette_ram_sel_regaddr = 0x947C + OUTPUT_REG_OFFSET(channel) + VIDEO_PALETTE_OFFSET(channel);
		palette_wr_data_regaddr = 0x9480 + OUTPUT_REG_OFFSET(channel) + VIDEO_PALETTE_OFFSET(channel);
		palette_fifo_aclr_regaddr = 0x9484 + OUTPUT_REG_OFFSET(channel) + VIDEO_PALETTE_OFFSET(channel);
		palette_rgb_sel_regaddr = 0x9488 + OUTPUT_REG_OFFSET(channel) + VIDEO_PALETTE_OFFSET(channel);
		palette_fifo_used_regaddr = 0x948C + OUTPUT_REG_OFFSET(channel) + VIDEO_PALETTE_OFFSET(channel);
		palette_enable_regaddr = (0x400000) + 0x9038 + WINDOW_REG_OFFSET(channel);
		break;

	case 2:
		palette_ram_sel_regaddr = 0x9490 + OUTPUT_REG_OFFSET(channel);
		palette_wr_data_regaddr = 0x9494 + OUTPUT_REG_OFFSET(channel);
		palette_fifo_aclr_regaddr = 0x9498 + OUTPUT_REG_OFFSET(channel);
		palette_rgb_sel_regaddr = 0x949C + OUTPUT_REG_OFFSET(channel);
		palette_fifo_used_regaddr = 0x94A0 + OUTPUT_REG_OFFSET(channel);
		palette_enable_regaddr = (0x400000) + 0x9460 + OUTPUT_REG_OFFSET(channel);
		break;

	default:
		return -1;
		break;
	}

	for (writeBlockNumber = 0; writeBlockNumber < 2; writeBlockNumber++) {

		MAC206HAL157(palette_ram_sel_regaddr);
		curPaletteRam = value;

		for (rgb = 0; rgb < 3; rgb++) {

			MAC206HAL158(palette_rgb_sel_regaddr, rgb);

			pWriteData = data + rgb * 256;


			MAC206HAL158(palette_fifo_aclr_regaddr, 1);
			MAC206HAL158(palette_fifo_aclr_regaddr, 0);

			nWrited = 0;
			while (1) {

				MAC206HAL157(palette_fifo_used_regaddr);
				mwv206_timed_loop (nTick, value > 0, FUNC206LXDEV098()) {
					FUNC206LXDEV128(2);
					MAC206HAL157(palette_fifo_used_regaddr);
				}

				if (value > 0) {
					V206DEV005("Error: timeout when writing palette, paletteno=%d, rgb=%d\n", paletteno, rgb);
					return -10;
				}

				if (nWrited >= 256) {

					break;
				}

				for (i = 0; i < 12; i++) {
					MAC206HAL158(palette_wr_data_regaddr, pWriteData[nWrited]);
					nWrited++;
					if (nWrited >= 256) {
						break;
					}
				}
			}
		}

		MAC206HAL158(palette_ram_sel_regaddr, 1 - curPaletteRam);
	}

	MAC206HAL157(palette_enable_regaddr);
	value &= ~(1 << 25);
	MAC206HAL158(palette_enable_regaddr, value);

	return 0;
}


int FUNC206HAL192(V206DEV025 *dev,
		int screen,
		int htotal,
		int hactive,
		int hfrontporch,
		int hsync,
		int vtotal,
		int vactive,
		int vfrontporch,
		int vsync,
		int framerate,
		int isInterleaved,
		int outputContent)
{
	int result;
	MWV206_VADD_DISPCONFIG_INFO dispConfigInfo;

	V206DEV005("[%s:%d]\n htotal = %d, hactive = %d, hfrontporch = %d, hsync = %d.\n"
			"vtotal = %d, vactive = %d, vfrontporch = %d, vsync = %d.\n",
			__FILE__, __LINE__,
			htotal, hactive, hfrontporch, hsync,
			vtotal, vactive, vfrontporch, vsync);

	if ((screen < 0) || (screen > 3)) {
		return -1;
	}

	dispConfigInfo.mDisplay.mScreenWidth = hactive;
	dispConfigInfo.mDisplay.mScreenHeight = vactive;
	dispConfigInfo.mDisplay.mVFrequency = framerate;
	dispConfigInfo.mDisplay.mVFreqActual = framerate;
	dispConfigInfo.mDisplay.mClock = 0;
	dispConfigInfo.mConfigParam.mHBlankBegin = 0;
	dispConfigInfo.mConfigParam.mHBlankEnd = htotal - hactive;
	dispConfigInfo.mConfigParam.mHTotal = htotal;
	V206DEV005("hactive = %d, vactive = %d.\n", hactive, vactive);
	V206DEV005("hfrontportch = %d, hsync = %d, vfrontporch = %d, vsync = %d.\n", hfrontporch, hsync, vfrontporch, vsync);


	dispConfigInfo.mConfigParam.mHSBegin = hfrontporch;
	dispConfigInfo.mConfigParam.mHSEnd = hfrontporch + hsync;

	dispConfigInfo.mConfigParam.mVBlankBegin = 0;
	dispConfigInfo.mConfigParam.mVBlankEnd = vtotal - vactive;

	dispConfigInfo.mConfigParam.mVSBegin = vfrontporch;
	dispConfigInfo.mConfigParam.mVSEnd = vfrontporch + vsync;

	dispConfigInfo.mConfigParam.mVTotal = vtotal;


	if (720 == hactive && 288 == vactive) {
		isInterleaved = 1;
	}


	result = FUNC206HAL187(dev,
			screen,
			0,
			isInterleaved,
			0,
			&dispConfigInfo,
			outputContent);
	if (result == 0) {
		if (framerate == 0) {
			dev->scanctrl[screen].vsync_interval = ktime_set(0, 1000000000UL / 60);
		} else {
			dev->scanctrl[screen].vsync_interval = ktime_set(0, 1000000000UL / framerate);
		}
	}
	return result;
}


int FUNC206HAL423(V206DEV025 *dev, int win)
{
	unsigned int V206DEV033;

	if (MAX_VIDEO_SCREEN_NUMBER <= win) {
		return -1;
	}

	V206DEV033 = 0x902C + WINDOW_REG_OFFSET(win) + 0x400000;
	MAC206HAL158(V206DEV033, 0);

	return 0;
}


int FUNC206HAL422(V206DEV025 *dev, int screen)
{
	unsigned int value = 0;
	unsigned int V206DEV033 = 0;

	if (MAX_VIDEO_SCREEN_NUMBER <= screen) {
		return -1;
	}

	V206DEV033 = 0x9460 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	value |= (1 << 25);
	MAC206HAL158(V206DEV033, value);

	return 0;
}




int FUNC206HAL417(V206DEV025 *dev, int screen, int ShowBmpX, int showBmpY)
{
	volatile unsigned int value;
	unsigned int V206DEV033;

	if (4 <= screen || dev == NULL) {
		return -1;
	}

	if (31 < ShowBmpX || 31 < showBmpY) {
		return -1;
	}


	V206DEV033 = 0x9454 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	if ((unsigned int)ShowBmpX != (value & 0x3F) || (unsigned int)showBmpY != ((value >> 16) & 0x3F)) {
		value = (ShowBmpX & 0x3F) | ((showBmpY & 0x3F) << 16);
		MAC206HAL158(V206DEV033, value);
	}

	return 0;
}



int FUNC206HAL416(V206DEV025 *dev, int screen, int alpha)
{
	volatile unsigned int value;
	unsigned int V206DEV033;

	if (4 <= screen || dev == NULL) {
		return -1;
	}


	V206DEV033 = 0x9478 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	value &= 0xF;
	value |= ((alpha & 0xFF) << 8);
	MAC206HAL158(V206DEV033, value);

	return 0;
}



int FUNC206HAL414(V206DEV025 *dev, int screen)
{
	volatile unsigned int value;
	unsigned int V206DEV033;

	if (4 <= screen || dev == NULL) {
		return -1;
	}


	V206DEV033 = 0x9478 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	value |= 1;
	MAC206HAL158(V206DEV033, value);

	return 0;
}



int FUNC206HAL413(V206DEV025 *dev, int screen)
{
	volatile unsigned int value;
	unsigned int V206DEV033;

	if (4 <= screen || dev == NULL) {
		return -1;
	}


	V206DEV033 = 0x9478 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL157(V206DEV033);
	value &= ~1;
	MAC206HAL158(V206DEV033, value);

	return 0;
}



int FUNC206HAL415(V206DEV025 *dev, int screen, int showXPos, int showYPos)
{
	unsigned int value;
	unsigned int V206DEV033;

	if (4 <= screen || dev == NULL) {
		return -1;
	}


	value = (showXPos & 0xFFF) | ((showYPos & 0xFFF) << 16);
	V206DEV033 = 0x94A8 + OUTPUT_REG_OFFSET(screen);
	MAC206HAL158(V206DEV033, value);

	return 0;
}


int FUNC206HAL412(V206DEV025 *dev, int screen, unsigned int data[4096])
{
	unsigned int *pWriteData = 0;

	if (4 <= screen || dev == NULL) {
		return -1;
	}

	pWriteData = data;

	FUNC206HAL413(dev, screen);
	FUNC206HAL414(dev, screen);

	return 0;
}

#define OUTPUT_REG_OFFSET_LOCAL(index) (OUTPUT_REG_OFFSET(index) - 0x400000)
#define MWV206_OVERLAY_READ(reg) V206DEV001(reg + (0x400000))
#define MWV206_OVERLAY_WRITE(reg, val) V206DEV002(reg + (0x400000), val)
#define VIDEO_PALETTE_OFFSET(index)    (((0x14 * (index) + (index >> 1) * 0x24)))
void mwv206pm_winreg_reload(V206DEV025 *pDev)
{
	int i = 0;
	int j = 0;

	for (i = 0; i < 4; i++) {
		j = 0;
		MWV206_OVERLAY_WRITE(0x9030 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x902C + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9008 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x900C + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9068 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x904C + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9084 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9050 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9054 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9064 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9028 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9464 + OUTPUT_REG_OFFSET_LOCAL(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9010 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x903C + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9090 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9094 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x967C + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9420 + VINPUT_REG_OFFSET(i), pDev->pm.win[i][j++]);


		V206DEV002(0x947C + OUTPUT_REG_OFFSET(i) + VIDEO_PALETTE_OFFSET(i), pDev->pm.win[i][j++]);
		V206DEV002(0x9480 + OUTPUT_REG_OFFSET(i) + VIDEO_PALETTE_OFFSET(i), pDev->pm.win[i][j++]);
		V206DEV002(0x9484 + OUTPUT_REG_OFFSET(i) + VIDEO_PALETTE_OFFSET(i), pDev->pm.win[i][j++]);
		V206DEV002(0x9488 + OUTPUT_REG_OFFSET(i) + VIDEO_PALETTE_OFFSET(i), pDev->pm.win[i][j++]);
		V206DEV002(0x948C + OUTPUT_REG_OFFSET(i) + VIDEO_PALETTE_OFFSET(i), pDev->pm.win[i][j++]);
		MWV206_OVERLAY_WRITE(0x9038 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j++]);

		MWV206_OVERLAY_WRITE(0x9014 + WINDOW_REG_OFFSET(i), pDev->pm.win[i][j]);
	}
}

void mwv206pm_winreg_save(V206DEV025 *pDev)
{
	int i = 0;
	int j = 0;

	for (i = 0; i < 4; i++) {
		j = 0;
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9030 + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x902C + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9008 + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x900C + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9068 + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x904C + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9084 + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9050 + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9054 + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9064 + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9028 + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9464 + OUTPUT_REG_OFFSET_LOCAL(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9010 + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x903C + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9090 + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9094 + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x967C + WINDOW_REG_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9420 + VINPUT_REG_OFFSET(i));


		pDev->pm.win[i][j++] = V206DEV001(0x947C + OUTPUT_REG_OFFSET(i) + VIDEO_PALETTE_OFFSET(i));
		pDev->pm.win[i][j++] = V206DEV001(0x9480 + OUTPUT_REG_OFFSET(i) + VIDEO_PALETTE_OFFSET(i));
		pDev->pm.win[i][j++] = V206DEV001(0x9484 + OUTPUT_REG_OFFSET(i) + VIDEO_PALETTE_OFFSET(i));
		pDev->pm.win[i][j++] = V206DEV001(0x9488 + OUTPUT_REG_OFFSET(i) + VIDEO_PALETTE_OFFSET(i));
		pDev->pm.win[i][j++] = V206DEV001(0x948C + OUTPUT_REG_OFFSET(i) + VIDEO_PALETTE_OFFSET(i));
		pDev->pm.win[i][j++] = MWV206_OVERLAY_READ(0x9038 + WINDOW_REG_OFFSET(i));

		pDev->pm.win[i][j] = MWV206_OVERLAY_READ(0x9014 + WINDOW_REG_OFFSET(i));
	}
}
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
#include "jmpwm.h"


int FUNC206HAL163(V206DEV025 *pDev,
		int chIndex,
		int highLevelTime,
		int totalTime,
		int forwareTime)
{
	unsigned int freq;
	unsigned int V206DEV033;
	unsigned int regoffset;
	unsigned int value;

	if (JM_PWM_CH_NUMBER <= chIndex) {
		V206KDEBUG002("Invalid channel index value: chIndex = %d!\n", chIndex);
		return -1;
	}

	freq = pDev->V206DEV035 / 2;
	regoffset = (0x5600) + JM_PWM_REG_OFFSET * chIndex;


	V206DEV033 = regoffset + JM_PWM_HIGH_CYCLE_NUMBER;
	value = highLevelTime * freq;
	JM_PWM_REG_SET(V206DEV033, value);


	V206DEV033 = regoffset + JM_TOTAL_LOW_CYCLE_NUMBER;
	value = totalTime * freq;
	JM_PWM_REG_SET(V206DEV033, value);



	V206DEV033 = regoffset + JM_FORWARD_TIME_REG;
	value = forwareTime * freq;
	JM_PWM_REG_SET(V206DEV033, value);

	return 0;
}


int FUNC206HAL165(V206DEV025 *pDev, int group, int chMask)
{
	unsigned int value;

	if (group == 0) {
		value = JM_PWM_REG_GET(0x0170);
		value &= 0xFFFFFFFF;
		if (chMask & 8) {
			value &= 0xFF0FFFFF;
		}
		if (chMask & 4) {
			value &= 0xFFF0FFFF;
		}
		if (chMask & 2) {
			value &= 0xFFFF0FFF;
		}
		if (chMask & 1) {
			value &= 0xFFFFF0FF;
		}
		value |= (((chMask & 8) << (20 - 3)) | ((chMask & 4) << (16 - 2)) | ((chMask & 2) << (12 - 1)) | ((chMask & 1) << 8));
		JM_PWM_REG_SET(0x0170, value);
	} else if (group == 1) {
		value = JM_PWM_REG_GET(0x0168);
		value &= 0xFFFFFFFF;
		if (chMask & 1) {
			value &= 0xF0FFFFFF;
		}
		if (chMask & 2) {
			value &= 0x0FFFFFFF;
		}
		value |= (((chMask & 2) << 28) | ((chMask & 1) << (24 + 1)));
		JM_PWM_REG_SET(0x0168, value);

		value = JM_PWM_REG_GET(0x016C);
		value &= 0xFFFFFFFF;
		if (chMask & 4) {
			value &= 0xFFFFFFF0;
		}
		if (chMask & 8) {
			value &= 0xFFFFFF0F;
		}

		value |= (((chMask & 8) << (4 - 2)) | ((chMask & 4) / 2));
		JM_PWM_REG_SET(0x016C, value);
	} else {
		V206KDEBUG002("Invalid parameters!\n");
		return -1;
	}


	value = JM_PWM_REG_GET((0x5600) + JM_PWM_OUTPUT_ENABLE_REG);
	value |= chMask;
	JM_PWM_REG_SET((0x5600) + JM_PWM_OUTPUT_ENABLE_REG, value);

	return 0;
}



int FUNC206HAL164(V206DEV025 *pDev, int chMask)
{
	unsigned int value;


	value = JM_PWM_REG_GET((0x5600) + JM_PWM_OUTPUT_ENABLE_REG);
	value &= ~chMask;
	JM_PWM_REG_SET((0x5600) + JM_PWM_OUTPUT_ENABLE_REG, value);

	return 0;
}
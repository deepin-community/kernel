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
#include "jmgpio.h"


int FUNC206HAL142(V206DEV025 *pDev, int port, int direction)
{
	unsigned int configValue, value;
	int moveNum = port;

	if (0 > port || 63 < port || 0 > direction || 1 < direction) {
		return -1;
	}
	FUNC206HAL369(pDev, 1);

	if (port >= 0 && port < 32) {
		value = V206GPIO007(V206GPIO005);
	} else {
		value = V206GPIO007(V206GPIO006);
		moveNum -= 32;
	}

	if (0 == direction) {
		configValue = value & (~(1 << moveNum));
	} else {
		configValue = value | (1 << moveNum);
	}

	if (port >= 0 && port < 32) {
		V206GPIO008(V206GPIO005, configValue);
	} else {
		V206GPIO008(V206GPIO006, configValue);
	}

	FUNC206HAL369(pDev, 0);
	return 0;
}


int FUNC206HAL144(V206DEV025 *pDev, int port, int *pValue)
{
	unsigned int value;
	int moveNum = port;

	if (0 > port || 63 < port) {
		return -1;
	}

	FUNC206HAL369(pDev, 1);
	if (port >= 0 && port < 32) {
		value = V206GPIO007(V206GPIO001);
	} else {
		value = V206GPIO007(V206GPIO002);
		moveNum -= 32;
	}

	FUNC206HAL369(pDev, 0);

	*pValue = 0x1 & (value >> moveNum);
	return 0;
}



int FUNC206HAL143(V206DEV025 *pDev, int port, int outValue)
{
	unsigned int configValue, value;
	int moveNum = port;

	if (0 > port || 63 < port) {
		return -1;
	}

	FUNC206HAL369(pDev, 1);
	if (port >= 0 && port < 32) {
		value = V206GPIO007(V206GPIO003);
	} else {
		value = V206GPIO007(V206GPIO004);
		moveNum -= 32;
	}


	outValue = outValue & 0x01;
	if (0 == outValue) {
		configValue = value & (~(1 << moveNum));
	} else {
		configValue = value | (1 << moveNum);
	}

	if (port >= 0 && port < 32) {
		V206GPIO008(V206GPIO003, configValue);
	} else {
		V206GPIO008(V206GPIO004, configValue);
	}
	FUNC206HAL369(pDev, 0);

	return 0;
}
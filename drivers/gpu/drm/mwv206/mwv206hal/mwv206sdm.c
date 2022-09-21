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
#include "gljos.h"
#include "mwv206dev.h"
#include "mwv206kdebug.h"



#define  MAC206HAL168         (0x0)


#define  MAC206HAL166        (0x4)


#define  MAC206HAL167       (0x8)


#define  MAC206HAL170          (0xC)


#define  MAC206HAL174        (0x10)


#define  MAC206HAL165         (0x14)


#define  MAC206HAL164        (0x18)


#define  MAC206HAL173         (0x1C)


#define  MAC206HAL169     (0x20)


#define MAC206HAL172(reg, value)    V206DEV006(devInfo, 0x405300 + reg, value)
#define MAC206HAL171(reg) V206DEV007(devInfo, 0x405300 + reg)


int FUNC206HAL384(V206DEV025 *devInfo, signed int *temp)
{
	int val, data;
	int a214, b214;

	data = MAC206HAL171(MAC206HAL173);
	if ((data >> 31) == 1) {
		V206KDEBUG002("[ht]OVERFLOW\n");
		return -1;
	}

	if ((data >> 24) & 0x1) {
		return -1;
	}

	data = data & 0x3fff;

	val  = MAC206HAL171(MAC206HAL170);
	a214 = 7214 + 3 * (val & 0x1f);
	val  = MAC206HAL171(MAC206HAL167);
	b214 = 2670 + 2 * (val & 0x1f);

	*temp = (data * a214) - b214 * 16384;

	return 0;
}


int FUNC206HAL385(V206DEV025 *devInfo)
{

	MAC206HAL172(MAC206HAL169, 0);


	MAC206HAL172(MAC206HAL168, 0);
	MAC206HAL172(MAC206HAL165, 0xd02);
	MAC206HAL172(MAC206HAL174, 0x128);


	MAC206HAL172(MAC206HAL169, 1);


	MAC206HAL172(MAC206HAL168, 1);

	return 0;
}
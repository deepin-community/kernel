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
#include "common.h"

static int FUNC206LXDEV092(int rop)
{
	int ret = -1;

	switch (rop) {
	case GXclear:
		ret = 0;
		break;
	case GXand:
		ret = 0xa0;
		break;
	case GXandReverse:
		ret = 0x50;
		break;
	case GXcopy:
		ret = 0xf0;
		break;
	case GXandInverted:
		ret = 0xa;
		break;
	case GXnoop:
		ret = 0xaa;
		break;
	case GXxor:
		ret = 0x5a;
		break;
	case GXor:
		ret = 0xfa;
		break;
	case GXnor:
		ret = 0x5;
		break;
	case GXequiv:
		ret = 0xa5;
		break;
	case GXinvert:
		ret = 0x55;
		break;
	case GXorReverse:
		ret = 0xf5;
		break;
	case GXcopyInverted:
		ret = 0xf;
		break;
	case GXorInverted:
		ret = 0xaf;
		break;
	case GXnand:
		ret = 0x5f;
		break;
	case GXset:
		ret = 0xff;
		break;
	default:
		ret = -1;
		break;
	}

	return ret;
}

int FUNC206LXDEV154(V206DEV025 *pDev,
		unsigned long mmio,
		unsigned long dst, int V206DEV182, int V206KG2D001,
		int x, int y, int width, int height,
		unsigned long color, unsigned long mask, int rop)
{
	V206IOCTL165 fill;
	int rrop;

	if (V206KG2D001 != 32 && V206KG2D001 != 16 && V206KG2D001 != 8) {
		return -1;
	}

	rrop = FUNC206LXDEV092(rop);
	if (rrop == -1) {
		return -1;
	}

	fill.addr = dst;
	fill.V206KG2D033 = V206DEV182;
	fill.V206KG2D001 = V206KG2D001;
	fill.x = x;
	fill.y = y;
	fill.width = width;
	fill.height = height;
	fill.color = color;
	fill.mask = mask;
	fill.rrop = rrop;

	if (FUNC206HAL250(pDev, &fill)) {
		return -1;
	}

	if (pDev->V206DEV096.wait(pDev, MWV206CMDTIMEOUT)) {
		FUNC206HAL001(pDev);
	}

	return 0;
}
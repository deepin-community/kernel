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

int FUNC206LXDEV169(int rop)
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

int FUNC206LXDEV145(V206DEV025 *pDev,
		unsigned long mmio,
		unsigned long src, unsigned long dst,
		int spitch, int dpitch,
		int V206KG2D001,
		int sx, int sy,
		int dx, int dy,
		int width, int height,
		unsigned long mask, int rop)
{
	V206IOCTL157 mbit;
	int rrop;

	if (V206KG2D001 != 32 && V206KG2D001 != 16 && V206KG2D001 != 8) {
		return -1;
	}

	rrop = FUNC206LXDEV169(rop);
	if (rrop == -1) {
		return -1;
	}

	mbit.V206KG2D023   = src;
	mbit.V206KG2D004   = dst;
	mbit.V206KG2D032 = spitch;
	mbit.V206KG2D011 = dpitch;
	mbit.V206KG2D001     = V206KG2D001;
	mbit.sx      = sx;
	mbit.sy      = sy;
	mbit.dx      = dx;
	mbit.dy      = dy;
	mbit.width   = width;
	mbit.height  = height;
	mbit.mask    = mask;
	mbit.rrop    = rrop;

	if (FUNC206HAL477(pDev, &mbit)) {
		return -1;
	}

	if (pDev->V206DEV096.wait(pDev, MWV206CMDTIMEOUT)) {
		FUNC206HAL001(pDev);
	}

	return 0;
}
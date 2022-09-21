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
#ifndef _MWV206_COMMON_H_
#define _MWV206_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <asm/io.h>
#include "mwv206dev.h"

#define GXclear         0x0
#define GXand           0x1
#define GXandReverse        0x2
#define GXcopy          0x3
#define GXandInverted       0x4
#define GXnoop          0x5
#define GXxor           0x6
#define GXor            0x7
#define GXnor           0x8
#define GXequiv         0x9
#define GXinvert        0xa
#define GXorReverse     0xb
#define GXcopyInverted      0xc
#define GXorInverted        0xd
#define GXnand          0xe
#define GXset           0xf

#define TRANSCOORD(base, V206DEV182, V206KG2D001, x, y) do { \
	int x1, y1; \
	int offset = (base) & 0xffff; \
	y1 = offset / (V206DEV182); \
	x1 = (offset - (y1 * (V206DEV182))) / (V206KG2D001 / 8); \
	x += x1; \
	y += y1; \
} while (0)


static inline int get_pitch(int width, int V206KG2D001)
{
	int V206DEV182;

	V206DEV182 = width * V206KG2D001 / 8;

	return (V206DEV182 + 0xf) & ~0xf;
};

int mwv206_fillrect(V206DEV025 *pDev, unsigned long mmio,
		unsigned long dst, int V206DEV182, int V206KG2D001,
		int x, int y, int width, int height,
		unsigned long color, unsigned long mask);

int FUNC206LXDEV154(V206DEV025 *pDev, unsigned long mmio,
		unsigned long dst, int V206DEV182, int V206KG2D001,
		int x, int y, int width, int height,
		unsigned long color, unsigned long mask, int rop);

int FUNC206LXDEV145(V206DEV025 *pDev, unsigned long mmio,
		unsigned long src, unsigned long dst,
		int spitch, int dpitch,
		int V206KG2D001,
		int sx, int sy,
		int dx, int dy,
		int width, int height,
		unsigned long mask, int rop);
int FUNC206HAL001(V206DEV025 *pDev);

#ifdef __cplusplus
}
#endif

#endif
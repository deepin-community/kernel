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
#include "mwv206kdebug.h"
#include "gljos.h"
#include "common.h"
#include <asm/string.h>

static int valid_fb_rect(V206DEV025 *pDev, unsigned int addr, int V206KG2D033,
		int width, int height, int x, int y, int V206KG2D001)
{
	unsigned int start;
	unsigned int size;
	unsigned int byte_per_pixel;

	if (unlikely(V206KG2D001 != 32 && V206KG2D001 != 16 && V206KG2D001 != 8)) {
		return 0;
	}

	if (unlikely(width <= 0 || height <= 0)) {
		return 0;
	}

	if (unlikely(x < 0 || y < 0)) {
		return 0;
	}

	if (unlikely(V206KG2D033 <= 0 && (height != 1 || y != 0))) {
		return 0;
	}

	byte_per_pixel = V206KG2D001 / 8;
	start = addr + x * byte_per_pixel + y * V206KG2D033;
	if (unlikely(!addr_in_fb_range(pDev, start))) {
		return 0;
	}

	if (unlikely(height == 1)) {
		size  = width * byte_per_pixel;
	} else {
		if (unlikely(((x + width) * byte_per_pixel) > V206KG2D033)) {
			return 0;
		}
		size  = V206KG2D033 * height;
		size -= x * byte_per_pixel;
		size -= V206KG2D033 - (x + width) * byte_per_pixel;
	}

	if (unlikely(!size_in_fb_range(pDev, start, size))) {
		return 0;
	}

	return 1;
}

static int FUNC206HAL335(int V206KG2D001, int *V206KG2D019, int *V206KG2D020)
{
	int ret = -1;
	switch (V206KG2D001) {
	case 32:
		*V206KG2D019 = 0;
		*V206KG2D020 = 0;
		ret = 0;
		break;
	case 16:
		*V206KG2D019 = 1;
		*V206KG2D020 = 8;
		ret = 0;
		break;
	case 8:
		*V206KG2D019 = 2;
		*V206KG2D020 = 16;
		ret = 0;
		break;
	default:
		ret = -1;
		break;
	}
	return ret;
}

int FUNC206HAL249(V206DEV025 *pDev, V206IOCTL165 *V206KG2D021)
{
	jjuint32 addr;
	int V206KG2D033, x, y;
	int V206KG2D001, V206KG2D002 = 0, V206KG2D006 = 0;
	unsigned int offset;
	unsigned int cmd[128];
	int V206KG2D003;
	int cnt, ret;

	if (pDev->V206DEV096.send == NULL) {
		return -1;
	}

	addr = V206KG2D021->addr;
	V206KG2D001 = V206KG2D021->V206KG2D001;
	V206KG2D033 = V206KG2D021->V206KG2D033;
	x = V206KG2D021->x;
	y = V206KG2D021->y;

	FUNC206HAL335(V206KG2D001, &V206KG2D002, &V206KG2D006);

	V206DEV005("[%s][1] addr = 0x%x, stride = 0x%x, bpp =  %d, x = %d, y = %d.\n",
			__FUNCTION__, addr, V206KG2D033, V206KG2D001, x, y);
	cnt = 0;
retry:
	V206KG2D003 = 0;
	offset = pDev->V206DEV096.offset;
	TRANSCOORD(addr, V206KG2D033, V206KG2D001, x, y);

	cmd[V206KG2D003++] = 0x40001008 + offset;
	cmd[V206KG2D003++] = 0;

	cmd[V206KG2D003++] = 0x4000100c + offset;
	cmd[V206KG2D003++] = V206KG2D006;

	cmd[V206KG2D003++] = 0x40001014 + offset;
	cmd[V206KG2D003++] = V206KG2D002;

	cmd[V206KG2D003++] = 0x40001020 + offset;
	cmd[V206KG2D003++] = V206KG2D021->color;

	cmd[V206KG2D003++] = 0x40001054 + offset;
	cmd[V206KG2D003++] = V206KG2D021->mask;

	cmd[V206KG2D003++] = 0x82000054;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = (V206KG2D033 / 16 << 16) | (addr >> 16);
	cmd[V206KG2D003++] = y << 16 | x;
	cmd[V206KG2D003++] = V206KG2D021->height << 16 | V206KG2D021->width;
	cmd[V206KG2D003++] = 0x81000000;

	ret = pDev->V206DEV096.send(pDev, cmd, V206KG2D003, offset);
	if (ret == V206DEV018 && cnt < 10) {
		cnt++;
		goto retry;
	}

	V206DEV005("fillrect done\n");

	return ret == V206KG2D003 ? 0 : -1;
}

int FUNC206HAL250(V206DEV025 *pDev, V206IOCTL165 *V206KG2D021)
{

	jjuint32 addr;
	int V206KG2D033, x, y;
	int V206KG2D001, V206KG2D002 = 0, V206KG2D006 = 0;
	unsigned int cmd[128];
	int V206KG2D003;
	unsigned int offset;
	int ret, cnt;

	if (pDev->V206DEV096.send == NULL) {
		return -1;
	}

	addr = V206KG2D021->addr;
	V206KG2D001 = V206KG2D021->V206KG2D001;
	V206KG2D033 = V206KG2D021->V206KG2D033;
	x = V206KG2D021->x;
	y = V206KG2D021->y;

	FUNC206HAL335(V206KG2D001, &V206KG2D002, &V206KG2D006);

	cnt = 0;
retry:
	V206KG2D003 = 0;
	offset = pDev->V206DEV096.offset;
	TRANSCOORD(addr, V206KG2D033, V206KG2D001, x, y);

	cmd[V206KG2D003++] = 0x40001008 + offset;
	cmd[V206KG2D003++] = 0;

	cmd[V206KG2D003++] = 0x4000100c + offset;
	cmd[V206KG2D003++] = V206KG2D006;

	cmd[V206KG2D003++] = 0x40001014 + offset;
	cmd[V206KG2D003++] = V206KG2D002;

	cmd[V206KG2D003++] = 0x40001020 + offset;
	cmd[V206KG2D003++] = V206KG2D021->color;

	cmd[V206KG2D003++] = 0x40001054 + offset;
	cmd[V206KG2D003++] = V206KG2D021->mask;

	cmd[V206KG2D003++] = 0x8200001c;
	cmd[V206KG2D003++] = V206KG2D021->rrop & 0xff;
	cmd[V206KG2D003++] = (V206KG2D033 / 16 << 16) | (addr >> 16);
	cmd[V206KG2D003++] = y << 16 | (x & 0x7fff);
	cmd[V206KG2D003++] = V206KG2D021->height << 16 | (V206KG2D021->width & 0x7fff);

	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;

	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;

	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0x81000000;

	ret = pDev->V206DEV096.send(pDev, cmd, V206KG2D003, offset);
	if (ret == V206DEV018 && cnt < 10) {
		cnt++;
		goto retry;
	}

	V206DEV005("fillrectx done\n");

	return ret == V206KG2D003 ? 0 : -1;
}

int FUNC206HAL248(V206DEV025 *pDev, long arg)
{
	V206IOCTL165 *V206KG2D021 = (V206IOCTL165 *)arg;

	if (!valid_fb_rect(pDev, V206KG2D021->addr, V206KG2D021->V206KG2D033, V206KG2D021->width, V206KG2D021->height,
		V206KG2D021->x, V206KG2D021->y, V206KG2D021->V206KG2D001)) {
		V206KDEBUG002("MWV206: invalid fill rect, ignored\n");
		return -1;
	}

	mwv206_shadow_mark_draw(pDev);

	if (V206KG2D021->rrop == -1) {
		return FUNC206HAL249(pDev, V206KG2D021);
	} else {
		return FUNC206HAL250(pDev, V206KG2D021);
	}
}

int FUNC206HAL251(V206DEV025 *pDev, long arg)
{
	V206IOCTL174 *V206KG2D022;
	unsigned int cmd[128];
	jjuint32 V206KG2D026, V206KG2D008;
	unsigned int V206KG2D030, V206KG2D036, V206KG2D038, V206KG2D031, V206KG2D028;
	unsigned int V206KG2D013, V206KG2D035, V206KG2D037, V206KG2D014, V206KG2D010;
	unsigned int offset;
	int V206KG2D016 = 0;
	int V206KG2D017    = 0;
	int V206KG2D003;
	int ret, cnt;

	if (pDev->V206DEV096.send == NULL) {
		return -1;
	}


	V206KG2D022 = (V206IOCTL174 *)arg;
	V206KG2D026   = V206KG2D022->V206KG2D026;
	V206KG2D030 = V206KG2D022->V206KG2D030;
	V206KG2D036  = V206KG2D022->V206KG2D036;
	V206KG2D038  = V206KG2D022->V206KG2D038;
	V206KG2D031  = V206KG2D022->V206KG2D031;
	V206KG2D028 = V206KG2D022->V206KG2D028;
	V206KG2D008   = V206KG2D022->V206KG2D008;
	V206KG2D013 = V206KG2D022->V206KG2D013;
	V206KG2D035  = V206KG2D022->V206KG2D035;
	V206KG2D037  = V206KG2D022->V206KG2D037;
	V206KG2D014  = V206KG2D022->V206KG2D014;
	V206KG2D010 = V206KG2D022->V206KG2D010;

	if (!valid_fb_rect(pDev, V206KG2D026, V206KG2D030,
		V206KG2D031, V206KG2D028, V206KG2D036, V206KG2D038, 32)) {
		V206KDEBUG002("MWV206: invalid 2d-resize src rect, ignored\n");
		return -1;
	}
	if (!valid_fb_rect(pDev, V206KG2D008, V206KG2D013,
		V206KG2D014, V206KG2D010, V206KG2D035, V206KG2D037, 32)) {
		V206KDEBUG002("MWV206: invalid 2d-resize dst rect, ignored\n");
		return -1;
	}

	TRANSCOORD(V206KG2D026, V206KG2D030, 32, V206KG2D036, V206KG2D038);
	TRANSCOORD(V206KG2D008, V206KG2D013, 32, V206KG2D035, V206KG2D037);
	cnt = 0;
retry:
	V206KG2D003 = 0;
	offset = pDev->V206DEV096.offset;


	cmd[V206KG2D003++] = 0x40001008 + offset;
	cmd[V206KG2D003++] = 0;


	cmd[V206KG2D003++] = 0x4000100c + offset;
	cmd[V206KG2D003++] = V206KG2D016;


	cmd[V206KG2D003++] = 0x40001030 + offset;
	cmd[V206KG2D003++] = V206KG2D016;

	cmd[V206KG2D003++] = 0x8200001C;
	cmd[V206KG2D003++] = (0xCC) | (0 << 8) | (0 << 16) | (0 << 31);
	cmd[V206KG2D003++] = (V206KG2D008 >> 16) | ((V206KG2D013 >> 4) << 16);
	cmd[V206KG2D003++] = ((int)V206KG2D035 & 0x7fff) | ((int)V206KG2D037 << 16);
	cmd[V206KG2D003++] = ((int)V206KG2D014 & 0x7fff) | ((int)V206KG2D010 << 16);

	cmd[V206KG2D003++] = (V206KG2D026 >> 16) | ((V206KG2D030 >> 4) << 16) | (V206KG2D017 << 29);
	cmd[V206KG2D003++] = ((int)V206KG2D036 & 0x7fff) | ((int)V206KG2D038 << 16);
	cmd[V206KG2D003++] = ((int)V206KG2D031 & 0x7fff) | ((int)V206KG2D028 << 16);
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0x81000000;

	ret = pDev->V206DEV096.send(pDev, cmd, V206KG2D003, offset);
	if (ret == V206DEV018 && cnt < 10) {
		cnt++;
		goto retry;
	}

	return ret == V206KG2D003 ? 0 : -1;
}

int FUNC206HAL247(V206DEV025 *pDev, V206IOCTL157 *mbit, unsigned int offset,  unsigned int *cmd,
		int opt)
{
	jjuint32 V206KG2D023, V206KG2D004;
	int V206KG2D032, V206KG2D011;
	int V206KG2D001, V206KG2D002 = 0, V206KG2D006 = 0;
	int sx, sy, dx, dy;
	int width, height;
	unsigned int mask, rrop;
	int V206KG2D003 = 0;

	V206KG2D023 = mbit->V206KG2D023;
	V206KG2D004 = mbit->V206KG2D004;
	V206KG2D032 = mbit->V206KG2D032;
	V206KG2D011 = mbit->V206KG2D011;
	V206KG2D001 = mbit->V206KG2D001;
	sx = mbit->sx;
	sy = mbit->sy;
	dx = mbit->dx;
	dy = mbit->dy;
	width = mbit->width;
	height = mbit->height;
	mask = mbit->mask;
	rrop = mbit->rrop;

	FUNC206HAL335(V206KG2D001, &V206KG2D002, &V206KG2D006);

	TRANSCOORD(V206KG2D023, V206KG2D032, V206KG2D001, sx, sy);
	TRANSCOORD(V206KG2D004, V206KG2D011, V206KG2D001, dx, dy);

	if (offset == 0 && opt) {

		cmd[V206KG2D003++] = 0x85000000;
		cmd[V206KG2D003++] = 0x81010000;
	}

	cmd[V206KG2D003++] = 0x40001008 + offset;
	cmd[V206KG2D003++] = 0;

	cmd[V206KG2D003++] = 0x4000100c + offset;
	cmd[V206KG2D003++] = V206KG2D006;

	cmd[V206KG2D003++] = 0x40001014 + offset;
	cmd[V206KG2D003++] = V206KG2D002;

	cmd[V206KG2D003++] = 0x40001010 + offset;
	cmd[V206KG2D003++] = (V206KG2D032 / 16 << 16) | (V206KG2D023 >> 16);
	cmd[V206KG2D003++] = 0x40001018 + offset;
	cmd[V206KG2D003++] = sy << 16 | (sx & 0x7fff);

	cmd[V206KG2D003++] = 0x40001054 + offset;
	cmd[V206KG2D003++] = mask;

	cmd[V206KG2D003++] = 0x8200001c;
	cmd[V206KG2D003++] = (2 << 16) | (rrop & 0xff);
	cmd[V206KG2D003++] = (V206KG2D011 / 16 << 16) | (V206KG2D004 >> 16);
	cmd[V206KG2D003++] = dy << 16 | (dx & 0x7fff);
	cmd[V206KG2D003++] = height << 16 | (width & 0x7fff);

	cmd[V206KG2D003++] = (V206KG2D032 / 16 << 16) | (V206KG2D023 >> 16);
	cmd[V206KG2D003++] = sy << 16 | (sx & 0x7fff);
	cmd[V206KG2D003++] = height << 16 | (width & 0x7fff);

	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;

	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0x81000000;

	return V206KG2D003;
}

static int FUNC206HAL046(V206IOCTL157 *mbit)
{
	unsigned int V206KG2D029, V206KG2D027;
	unsigned int V206KG2D012, V206KG2D009;

	V206KG2D029 = mbit->V206KG2D023 + mbit->sy * mbit->V206KG2D032 + mbit->sx * mbit->V206KG2D001 / 8;
	V206KG2D012 = mbit->V206KG2D004 + mbit->dy * mbit->V206KG2D011 + mbit->dx * mbit->V206KG2D001 / 8;
	V206KG2D027   = V206KG2D029 + mbit->height * mbit->V206KG2D032 + mbit->width * mbit->V206KG2D001 / 8;
	V206KG2D009   = V206KG2D012 + mbit->height * mbit->V206KG2D011 + mbit->width * mbit->V206KG2D001 / 8;

	return ((V206KG2D029 >= V206KG2D012 &&  V206KG2D029 <= V206KG2D009)
			|| (V206KG2D012 >= V206KG2D029 && V206KG2D012 <= V206KG2D027));
}


int FUNC206HAL477(V206DEV025 *pDev, V206IOCTL157 *mbit)
{
	if (pDev->V206DEV096.send == NULL) {
		return -1;
	}


	if (pDev->V206DEV096.mode != V206API041
			|| pDev->V206DEV097.mode != V206API040
			|| mbit->width * mbit->height < 100 * 100
			|| FUNC206HAL046(mbit)) {
		unsigned int offset;
		unsigned    cmd[128];
		int         V206KG2D003, ret, cnt;

		cnt = 0;
retry:
		offset = pDev->V206DEV096.offset;
		V206KG2D003 = FUNC206HAL247(pDev, mbit, offset, cmd, 0);
		ret    = pDev->V206DEV096.send(pDev, cmd, V206KG2D003, offset);
		if (ret == V206DEV018 && cnt < 10) {
			cnt++;
			goto retry;
		}
		return ret == V206KG2D003 ? 0 : -1;
	}


	return FUNC206HAL252(pDev, mbit);
}

int FUNC206HAL246(V206DEV025 *pDev, long arg)
{
	V206IOCTL157 *mbit = (V206IOCTL157 *)arg;

	if (!valid_fb_rect(pDev, mbit->V206KG2D023, mbit->V206KG2D032, mbit->width, mbit->height,
		mbit->sx, mbit->sy, mbit->V206KG2D001)) {
		V206KDEBUG002("MWV206: invalid 2d-copy src rect, ignored\n");
		return -1;
	}
	if (!valid_fb_rect(pDev, mbit->V206KG2D004, mbit->V206KG2D011, mbit->width, mbit->height,
		mbit->dx, mbit->dy, mbit->V206KG2D001)) {
		V206KDEBUG002("MWV206: invalid 2d-copy dst rect, ignored\n");
		return -1;
	}

	mwv206_shadow_mark_draw(pDev);

	return FUNC206HAL477(pDev, mbit);
}

int FUNC206HAL245(V206DEV025 *pDev, long arg)
{
	V206IOCTL158 *V206KG2D018;
	jjuint32 V206KG2D023, V206KG2D004;
	int V206KG2D032, V206KG2D011;
	int V206KG2D024, V206KG2D005;
	int sx, sy, dx, dy;
	int width, height;
	unsigned int cmd[128];
	int V206KG2D003;
	unsigned int offset;
	int ret, cnt;

	if (pDev->V206DEV096.send == NULL) {
		return -1;
	}

	V206KG2D018 = (V206IOCTL158 *)arg;

	V206KG2D023 = V206KG2D018->V206KG2D023;
	V206KG2D004 = V206KG2D018->V206KG2D004;
	V206KG2D032 = V206KG2D018->V206KG2D032;
	V206KG2D011 = V206KG2D018->V206KG2D011;
	V206KG2D024 = V206KG2D018->V206KG2D024;
	V206KG2D005 = V206KG2D018->V206KG2D005;
	sx = V206KG2D018->sx;
	sy = V206KG2D018->sy;
	dx = V206KG2D018->dx;
	dy = V206KG2D018->dy;
	width = V206KG2D018->width;
	height = V206KG2D018->height;

	if (!valid_fb_rect(pDev, V206KG2D023, V206KG2D032, width, height, sx, sy, V206KG2D024)) {
		V206KDEBUG002("MWV206: invalid 2d-blend src rect, ignored\n");
		V206KDEBUG002("saddr:0x%x sstride:%d width:%d height:%d sx:%d sy:%d sbpp:%d\n",
				V206KG2D023, V206KG2D032, width, height, sx, sy, V206KG2D024);
		return -1;
	}
	if (!valid_fb_rect(pDev, V206KG2D004, V206KG2D011, width, height, dx, dy, V206KG2D005)) {
		V206KDEBUG002("MWV206: invalid 2d-blend dst rect, ignored\n");
		return -1;
	}

	mwv206_shadow_mark_draw(pDev);

	TRANSCOORD(V206KG2D023, V206KG2D032, V206KG2D024, sx, sy);
	TRANSCOORD(V206KG2D004, V206KG2D011, V206KG2D005, dx, dy);

	cnt = 0;
retry:
	V206KG2D003 = 0;
	offset = pDev->V206DEV096.offset;

	cmd[V206KG2D003++] = 0x40001008 + offset;
	if (V206KG2D018->buffen) {
		cmd[V206KG2D003++] = 0x80000000 | (V206KG2D018->buffstride / 16 << 16) | (V206KG2D018->buffaddr >> 16);
	} else {
		cmd[V206KG2D003++] = 0;
	}

	cmd[V206KG2D003++] = 0x4000100c + offset;
	cmd[V206KG2D003++] = V206KG2D018->dfmt;

	cmd[V206KG2D003++] = 0x40001030 + offset;
	cmd[V206KG2D003++] = V206KG2D018->sfmt;

	cmd[V206KG2D003++] = 0x40001054 + offset;
	cmd[V206KG2D003++] = 0xffffffff;


	cmd[V206KG2D003++] = 0x82000039;
	cmd[V206KG2D003++] = V206KG2D018->dfactor  << 9 | V206KG2D018->sfactor << 5 | 0x1;

	cmd[V206KG2D003++] = (V206KG2D011 / 16 << 16) | (V206KG2D004 >> 16);
	cmd[V206KG2D003++] = dy << 16 | dx;
	cmd[V206KG2D003++] = height << 16 | width;

	cmd[V206KG2D003++] = (V206KG2D032 / 16 << 16) | (V206KG2D023 >> 16);
	cmd[V206KG2D003++] = sy << 16 | sx;
	cmd[V206KG2D003++] = height << 16 | width;

	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0x81000000;

	ret = pDev->V206DEV096.send(pDev, cmd, V206KG2D003, offset);
	if (ret == V206DEV018 && cnt < 10) {
		cnt++;
		goto retry;
	}

	V206DEV005("blend done\n");
	return ret == V206KG2D003 ? 0 : -1;
}

int FUNC206HAL253(V206DEV025 *pDev, long arg)
{
	V206IOCTL178 *V206KG2D018;
	jjuint32 V206KG2D023, V206KG2D004;
	int V206KG2D032, V206KG2D011;
	int V206KG2D024, V206KG2D005;
	int sx, sy, dx, dy;
	int V206KG2D034, V206KG2D025, V206KG2D015, V206KG2D007;
	unsigned int cmd[128];
	int V206KG2D003;
	unsigned int offset;
	int ret, cnt;

	if (pDev->V206DEV096.send == NULL) {
		return -1;
	}

	V206KG2D018 = (V206IOCTL178 *)arg;

	V206KG2D023 = V206KG2D018->V206KG2D023;
	V206KG2D004 = V206KG2D018->V206KG2D004;
	V206KG2D032 = V206KG2D018->V206KG2D032;
	V206KG2D011 = V206KG2D018->V206KG2D011;
	V206KG2D024 = V206KG2D018->V206KG2D024;
	V206KG2D005 = V206KG2D018->V206KG2D005;
	sx = V206KG2D018->sx;
	sy = V206KG2D018->sy;
	dx = V206KG2D018->dx;
	dy = V206KG2D018->dy;
	V206KG2D034 = V206KG2D018->V206KG2D034;
	V206KG2D025 = V206KG2D018->V206KG2D025;
	V206KG2D015 = V206KG2D018->V206KG2D015;
	V206KG2D007 = V206KG2D018->V206KG2D007;

	if (!valid_fb_rect(pDev, V206KG2D023, V206KG2D032, V206KG2D034, V206KG2D025, sx, sy, V206KG2D024)) {
		V206KDEBUG002("MWV206: invalid 2d-zoom-blend src rect, ignored\n");
		return -1;
	}
	if (!valid_fb_rect(pDev, V206KG2D004, V206KG2D011, V206KG2D015, V206KG2D007, dx, dy, V206KG2D005)) {
		V206KDEBUG002("MWV206: invalid 2d-zoom-blend dst rect, ignored\n");
		return -1;
	}

	TRANSCOORD(V206KG2D023, V206KG2D032, V206KG2D024, sx, sy);
	TRANSCOORD(V206KG2D004, V206KG2D011, V206KG2D005, dx, dy);

	cnt = 0;
retry:
	V206KG2D003 = 0;
	offset = pDev->V206DEV096.offset;

	cmd[V206KG2D003++] = 0x40001008 + offset;
	if (V206KG2D018->buffen) {
		cmd[V206KG2D003++] = 0x80000000 | (V206KG2D018->buffstride / 16 << 16) | (V206KG2D018->buffaddr >> 16);
	} else {
		cmd[V206KG2D003++] = 0;
	}

	cmd[V206KG2D003++] = 0x4000100c + offset;
	cmd[V206KG2D003++] = V206KG2D018->dfmt;

	cmd[V206KG2D003++] = 0x40001030 + offset;
	cmd[V206KG2D003++] = V206KG2D018->sfmt;

	cmd[V206KG2D003++] = 0x40001054 + offset;
	cmd[V206KG2D003++] = 0xffffffff;

	if (V206KG2D018->globalalpha & 0x80000000) {
		cmd[V206KG2D003++] = 0x40001044 + offset;
		cmd[V206KG2D003++] = (V206KG2D018->globalalpha & 0x000000ff) << 24;
	}


	cmd[V206KG2D003++] = 0x82000039;
	cmd[V206KG2D003++] = V206KG2D018->dfactor  << 9 | V206KG2D018->sfactor << 5 | V206KG2D018->mode << 2 | 0x1;

	cmd[V206KG2D003++] = (V206KG2D011 / 16 << 16) | (V206KG2D004 >> 16);
	cmd[V206KG2D003++] = dy << 16 | dx;
	cmd[V206KG2D003++] = V206KG2D007 << 16 | V206KG2D015;

	cmd[V206KG2D003++] = (V206KG2D032 / 16 << 16) | (V206KG2D023 >> 16);
	cmd[V206KG2D003++] = sy << 16 | sx;
	cmd[V206KG2D003++] = V206KG2D025 << 16 | V206KG2D034;

	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0;
	cmd[V206KG2D003++] = 0x81000000;


	ret = pDev->V206DEV096.send(pDev, cmd, V206KG2D003, offset);
	if (ret == V206DEV018 && cnt < 10) {
		cnt++;
		goto retry;
	}

	V206DEV005("blend done\n");
	return ret == V206KG2D003 ? 0 : -1;
}
/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_PORTMAP_H
#define _NE6X_PORTMAP_H

#include <linux/types.h>
#include <linux/kernel.h>

#define PBMP_DWORD_NUM      4
#define PBMP_WORD_WIDTH     32

typedef u32 pbmp_t[PBMP_DWORD_NUM];

#define SET_BIT(DAT, POS)     ((DAT) |= ((u32)0x1 << (POS)))
#define CLR_BIT(DAT, POS)     ((DAT) &= (~((u32)0x01 << (POS))))

#define PBMP_DWORD_GET(bm, word)      ((bm)[(word)])
#define PBMP_CLEAR(bm) \
	(PBMP_DWORD_GET(bm, 0) = PBMP_DWORD_GET(bm, 1) = \
				 PBMP_DWORD_GET(bm, 2) = \
				 PBMP_DWORD_GET(bm, 3) = 0)

#define PBMP_WNET(port)		((port) / PBMP_WORD_WIDTH)
#define PBMP_WBIT(port)		(1LU << ((port) % PBMP_WORD_WIDTH))

#define PBMP_ENTRY(bm, port) \
	(PBMP_DWORD_GET(bm, PBMP_WNET(port)))

#define PBMP_PORT_REMOVE(bm, port) \
	(PBMP_ENTRY(bm, port) &= ~(PBMP_WBIT(port)))

#define PBMP_PORT_ADD(bm, port) \
	(PBMP_ENTRY(bm, port) |= PBMP_WBIT(port))

#endif

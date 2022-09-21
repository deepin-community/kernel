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
#ifndef _MWV206KDEBUG_H_
#define _MWV206KDEBUG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "mwv206oshal.h"

#define V206KDEBUG006     0
#define V206KDEBUG007    1
#define V206KDEBUG008     2
#define V206KDEBUG009     3
#define V206KDEBUG010       4

#define V206KDEBUG011     V206KDEBUG006

#include <linux/kernel.h>

#define V206KDEBUG001(level, fmt, args...) do \
{\
	if (level <= V206KDEBUG011)\
	printk(KERN_ALERT fmt, ##args);\
} while (0)


#undef __DEBUG__

#define V206KDEBUG002(fmt, args...) \
do { \
	 \
	printk(KERN_ALERT fmt, ##args);\
} while (0)

#define V206KDEBUG003(fmt, args...) \
do { \
	printk(KERN_INFO fmt, ##args);\
} while (0)

#define V206KDEBUG004(fmt, args...) \
do { \
	printk(KERN_DEBUG fmt, ##args);\
} while (0)

#define V206KDEBUG005 do {\
printk("KERN_ALERT %s, %d, %s\n", __FILE__, __LINE__, __FUNCTION__); \
FUNC206LXDEV128(1000); \
} while (0)


#ifdef __DEBUG__
#define V206DEV005 V206KDEBUG002
#else
#define V206DEV005(...)
#endif


#define V206DEV004(level, fmt, args...) do \
{\
if (level <= V206KDEBUG011) { \
	V206DEV005(fmt, ##args);\
} \
} while (0)

#ifdef __cplusplus
}
#endif

#endif
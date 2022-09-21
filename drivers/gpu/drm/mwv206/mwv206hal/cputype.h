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
#ifndef _MWV206CPUTYPE_H_
#define _MWV206CPUTYPE_H_

#include <asm/syscall.h>
#include <linux/version.h>
#define V206CTYPE001     0x660
#define V206CTYPE002   0x661
#define V206CTYPE003  0x662
#define V206CTYPE004   0x663
#define V206CTYPE005    0xD01
#define V206CTYPE006      0x207



#define V206CTYPE008(V206DEV028)  (V206DEV028 == V206CTYPE002)
#define V206CTYPE009(V206DEV028) (V206DEV028 == V206CTYPE003)
#define V206CTYPE010(V206DEV028)  (V206DEV028 == V206CTYPE004)
#define V206CTYPE011(V206DEV028)    (V206DEV028 == V206CTYPE005)
#define V206CTYPE012(V206DEV028)   (V206DEV028 == V206CTYPE006)



#if (defined(__arm__) || defined(__aarch64__))
static inline u32 V206CTYPE013(void)
{
	u32 pn;
	pn = read_cpuid_part_number();
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	V206KDEBUG003("[INFO] cpu arch: 0x%x.\n", syscall_get_arch(current));
#else
	V206KDEBUG003("[INFO] cpu arch: 0x%x.\n", syscall_get_arch());
#endif
	V206KDEBUG003("[INFO] cpu part: 0x%x.\n", pn);
	return pn;
}
#elif defined(__sw_64__)
static inline u32 V206CTYPE013(void)
{
	return 0x207;
}
#else
static inline u32 V206CTYPE013(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)
	V206KDEBUG003("[INFO] cpu arch: 0x%x.\n", syscall_get_arch(current));
#else
	V206KDEBUG003("[INFO] cpu arch: 0x%x.\n", syscall_get_arch());
#endif

	return 0x12;
}
#endif
#endif
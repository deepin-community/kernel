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
#ifndef _MWV206_MWV206DEC_H_
#define _MWV206_MWV206DEC_H_

#include <linux/ioctl.h>
#include <linux/types.h>

#define V206DEV008  'k'

#define V206DECH001       _IO(V206DEV008, 1)
#define V206DECH002    _IO(V206DEV008, 2)
#define V206DECH003      _IOR(V206DEV008,  3, unsigned long *)
#define V206DECH004      _IOR(V206DEV008,  4, unsigned int *)
#define V206DECH005           _IO(V206DEV008,  5)
#define V206DECH006           _IO(V206DEV008,  6)
#define V206DECH007    _IOR(V206DEV008, 7, unsigned long *)
#define V206DECH008      _IOR(V206DEV008, 8, unsigned int *)
#define V206DECH009  _IOW(V206DEV008, 9, struct core_desc *)
#define V206DECH010   _IOW(V206DEV008, 10, struct core_desc *)
#define V206DECH011   _IO(V206DEV008, 11)
#define V206DECH012   _IO(V206DEV008, 12)
#define V206DECH013    _IO(V206DEV008, 13)
#define V206DECH014    _IO(V206DEV008, 14)
#define V206DECH015      _IOWR(V206DEV008, 15, struct core_desc *)
#define V206DECH016       _IOWR(V206DEV008, 16, struct core_desc *)
#define V206DECH017  _IOWR(V206DEV008, 17, struct core_desc *)
#define V206DECH018   _IOWR(V206DEV008, 18, struct core_desc *)
#define V206DECH019     _IOR(V206DEV008, 19, int *)
#define V206DECH020        _IOWR(V206DEV008, 20, __u32 *)
#define V206DECH021       _IO(V206DEV008, 29)

#define V206DECH022 29
#define V206DECH023               0x0B00000

struct core_desc {
	__u32 id;
	__u32 *regs;
	__u32 size;
};

int mwv206dec_release(void *inode, void *filp, void *decoder, void *dev);
long mwv206dec_ioctl(void *filp, unsigned int cmd, unsigned long arg, void *decoder, void *dev);
void mwv206dec_reset(void *decoder, void *dev);
int mwv206dec_init(void **decoder, unsigned long reg_base, void *dev);
void mwv206dec_uninit(void *decoder, void *dev);
int mwv206dec_decoder_isr(void *decoder, void *dev);

#endif
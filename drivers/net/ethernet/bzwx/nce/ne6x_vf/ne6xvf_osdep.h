/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6XVF_OSDEP_H
#define _NE6XVF_OSDEP_H

#include <linux/types.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/tcp.h>
#include <linux/pci.h>
#include <linux/highuid.h>
#include <linux/io.h>
#include <asm-generic/int-ll64.h>
#include <linux/io-64-nonatomic-lo-hi.h>

inline void ne6xvf_init_spinlock_d(struct ne6xvf_spinlock *sp);
void ne6xvf_destroy_spinlock_d(struct ne6xvf_spinlock *sp);
void ne6xvf_acquire_spinlock_d(struct ne6xvf_spinlock *sp);
void ne6xvf_release_spinlock_d(struct ne6xvf_spinlock *sp);

#endif /* _NE6XVF_OSDEP_H */


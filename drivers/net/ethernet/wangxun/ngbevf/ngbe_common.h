/*******************************************************************************

  WangXun(R) GbE PCI Express Virtual Function Linux Network Driver
  Copyright(c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Software Team <linux.nic@trustnetic.com>
  WangXun Technology, HuaXing Times Square A507, Hangzhou, China.

*******************************************************************************/

#ifndef _NGBE_COMMON_H_
#define _NGBE_COMMON_H_

#include "ngbe_type.h"
#include "ngbe.h"

void ngbe_update32(struct ngbevf_hw *, u32, u64 *, u64 *);
void ngbe_update36(struct ngbevf_hw *, u32, u32, u64 *, u64 *);

#endif /* NGBE_COMMON */

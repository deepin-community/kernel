/*******************************************************************************

  WangXun(R) 10GbE PCI Express Virtual Function Linux Network Driver
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

#include "txgbe_common.h"


void txgbe_update32(struct txgbe_hw *hw, u32 reg,
			   u64 *last, u64 *counter)
{
	u64 curr = rd32(hw, reg);

	if (curr < *last)
		*counter += 0x100000000LL;
	*last = curr;
	*counter &= 0xFFFFFFFF00000000LL;
	*counter |= curr;
}

void txgbe_update36(struct txgbe_hw *hw, u32 loreg, u32 hireg,
			   u64 *last, u64 *counter)
{
	u64 lo32 = rd32(hw, loreg); /* snapshot */
	u64 hi32 = rd32(hw, hireg);
	u64 curr = hi32 << 32 | lo32;

	if (curr < *last)
		*counter += 0x1000000000LL;
	*last = curr;
	*counter &= 0xFFFFFFF000000000LL;
	*counter |= curr;
}


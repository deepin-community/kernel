// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include "rnp_pcs.h"
#include "rnp_regs.h"
#include "rnp_common.h"

static u32 rnp_read_pcs(struct rnp_hw *hw, int num, u32 addr)
{
	u32 reg_hi, reg_lo;
	u32 value;

	reg_hi = addr >> 8;
	reg_lo = (addr & 0xff) << 2;
	wr32(hw, RNP_PCS_BASE(num) + (0xff << 2), reg_hi);
	value = rd32(hw, RNP_PCS_BASE(num) + reg_lo);
	return value;
}

static void rnp_write_pcs(struct rnp_hw *hw, int num, u32 addr, u32 value)
{
	u32 reg_hi, reg_lo;

	reg_hi = addr >> 8;
	reg_lo = (addr & 0xff) << 2;
	wr32(hw, RNP_PCS_BASE(num) + (0xff << 2), reg_hi);
	wr32(hw, RNP_PCS_BASE(num) + reg_lo, value);
}

struct rnp_pcs_operations pcs_ops_generic = {
	.read = rnp_read_pcs,
	.write = rnp_write_pcs,
};

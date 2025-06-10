// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#include "rnpm_pcs.h"
#include "rnpm_regs.h"
#include "rnpm_common.h"

#define VR_XS_PMA_SNPS_CR_CTRL (0x18000 + (0xa0))
#define VR_XS_PMA_SNPS_CR_ADDR (0x18000 + (0xa1))
#define VR_XS_PMA_SNPS_CR_DATA (0x18000 + (0xa2))

static u32 __pcs_ioread32(struct rnpm_hw *hw, int num, u32 pcs_reg)
{
	u32 reg_hi, reg_lo, val;

	reg_hi = pcs_reg >> 8;
	reg_lo = (pcs_reg & 0xff) << 2;
	wr32(hw, RNPM_PCS_BASE(num) + (0xff << 2), reg_hi);
	val = rd32(hw, RNPM_PCS_BASE(num) + reg_lo);
	return val;
}

static void __pcs_iowrite32(struct rnpm_hw *hw, int num, int pcs_reg,
			    u32 val)
{
	u32 reg_hi, reg_lo;

	reg_hi = pcs_reg >> 8;
	reg_lo = (pcs_reg & 0xff) << 2;

	wr32(hw, RNPM_PCS_BASE(num) + (0xff << 2), reg_hi);
	wr32(hw, RNPM_PCS_BASE(num) + reg_lo, val);
}

static void __pcs_ioread32_wait_cond(struct rnpm_hw *hw, int num, u32 addr)
{
	u32 reg_hi, reg_lo;
	u32 val;

	reg_hi = addr >> 8;
	reg_lo = (addr & 0xff) << 2;
	wr32(hw, RNPM_PCS_BASE(num) + (0xff << 2), reg_hi);
	while (1) {
		val = rd32(hw, RNPM_PCS_BASE(num) + reg_lo);
		if ((val & BIT(0)) == 0)
			break;
	}
}

static u32 rnpm_cr_read_pcs(struct rnpm_hw *hw, int num, u32 addr)
{
	u32 val;

	__pcs_ioread32_wait_cond(hw, num, VR_XS_PMA_SNPS_CR_CTRL);
	__pcs_iowrite32(hw, num, VR_XS_PMA_SNPS_CR_ADDR, addr);

	val = __pcs_ioread32(hw, num, VR_XS_PMA_SNPS_CR_CTRL);
	__pcs_iowrite32(hw, num, VR_XS_PMA_SNPS_CR_CTRL, val & 0x1);

	val = __pcs_ioread32(hw, num, VR_XS_PMA_SNPS_CR_CTRL);
	__pcs_iowrite32(hw, num, VR_XS_PMA_SNPS_CR_CTRL, val | BIT(0));
	__pcs_ioread32_wait_cond(hw, num, VR_XS_PMA_SNPS_CR_CTRL);

	val = __pcs_ioread32(hw, num, VR_XS_PMA_SNPS_CR_DATA);

	return val;
}

static void rnpm_cr_write_pcs(struct rnpm_hw *hw, int num, u32 addr,
			      u32 data)
{
	u32 val;

	__pcs_ioread32_wait_cond(hw, num, VR_XS_PMA_SNPS_CR_CTRL);

	__pcs_iowrite32(hw, num, VR_XS_PMA_SNPS_CR_ADDR, addr);
	__pcs_iowrite32(hw, num, VR_XS_PMA_SNPS_CR_DATA, data);

	val = __pcs_ioread32(hw, num, VR_XS_PMA_SNPS_CR_CTRL);
	__pcs_iowrite32(hw, num, VR_XS_PMA_SNPS_CR_CTRL, val & 0x1);

	val = __pcs_ioread32(hw, num, VR_XS_PMA_SNPS_CR_CTRL);
	__pcs_iowrite32(hw, num, VR_XS_PMA_SNPS_CR_CTRL, val | BIT(0));

	__pcs_ioread32_wait_cond(hw, num, VR_XS_PMA_SNPS_CR_CTRL);
}

static u32 rnpm_read_pcs(struct rnpm_hw *hw, int num, u32 addr)
{
	u32 reg_hi, reg_lo, value;

	reg_hi = addr >> 8;
	reg_lo = (addr & 0xff) << 2;
	wr32(hw, RNPM_PCS_BASE(num) + (0xff << 2), reg_hi);
	value = rd32(hw, RNPM_PCS_BASE(num) + reg_lo);
	return value;
}

static void rnpm_write_pcs(struct rnpm_hw *hw, int num, u32 addr,
			   u32 value)
{
	u32 reg_hi, reg_lo;

	reg_hi = addr >> 8;
	reg_lo = (addr & 0xff) << 2;
	wr32(hw, RNPM_PCS_BASE(num) + (0xff << 2), reg_hi);
	wr32(hw, RNPM_PCS_BASE(num) + reg_lo, value);
}

struct rnpm_pcs_operations pcs_ops_generic = {
	.read = rnpm_read_pcs,
	.write = rnpm_write_pcs,
	.cr_read = rnpm_cr_read_pcs,
	.cr_write = rnpm_cr_write_pcs,
};

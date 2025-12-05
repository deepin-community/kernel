// SPDX-License-Identifier: GPL-2.0
/* WangXun Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#include "ngbe_phy.h"
#include "ngbe_hw.h"
#include "ngbe_type.h"

/**
 * ngbe_check_reset_blocked - check status of MNG FW veto bit
 * @hw: pointer to the hardware structure
 * This function checks the MMNGC.MNG_VETO bit to see if there are
 * any constraints on link from manageability.  For MAC's that don't
 * have this bit just return faluse since the link can not be blocked
 * via this method.
 **/
bool ngbe_check_reset_blocked(struct ngbe_hw *hw)
{
	u32 mmngc;

	mmngc = rd32(hw, NGBE_MIS_ST);
	if (mmngc & NGBE_MIS_ST_MNG_VETO)
		return true;

	return false;
}

/* For internal phy only */
static int ngbe_phy_read_reg(struct ngbe_hw *hw, u32 reg_offset, u32 page,
			     u16 *phy_data)
{
	/* clear input */
	*phy_data = 0;

	if (!(page == 0xa43 && (reg_offset == 0x1a || reg_offset == 0x1d)))
		wr32(hw, NGBE_PHY_CONFIG(NGBE_INTERNAL_PHY_PAGE_SELECT_OFFSET),
		     page);

	*phy_data = 0xFFFF & rd32(hw, NGBE_PHY_CONFIG(reg_offset));

	return NGBE_OK;
}

/* For internal phy only */
static int ngbe_phy_write_reg(struct ngbe_hw *hw, u32 reg_offset, u32 page,
			      u16 phy_data)
{
	if (!(page == 0xa43 &&
	      (reg_offset == 0x1a || reg_offset == 0x1d)))
		wr32(hw, NGBE_PHY_CONFIG(NGBE_INTERNAL_PHY_PAGE_SELECT_OFFSET),
		     page);
	wr32(hw, NGBE_PHY_CONFIG(reg_offset), phy_data);

	return NGBE_OK;
}

static int ngbe_gphy_wait_mdio_access_on(struct ngbe_hw *hw)
{
	int i;
	u16 val = 0;
	struct ngbe_adapter *adapter = hw->back;

	for (i = 0; i < 100; i++) {
		hw->phy.ops.read_reg(hw, 29, 0xa43, &val);
		if (val & 0x20) {
			e_info(hw, "mdio_access ready\n");
			break;
		}
		usec_delay(1000);
	}

	if (i == 100)
		e_info(hw, "mdio_access_on timeout\n");

	return 0;
}

static int ngbe_check_internal_phy_id(struct ngbe_hw *hw)
{
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;
	u16 phy_id = 0;

	ngbe_gphy_wait_mdio_access_on(hw);

	hw->phy.ops.read_reg(hw, NGBE_MDI_PHY_ID1_OFFSET, 0, &phy_id_high);
	phy_id = phy_id_high << 6;
	hw->phy.ops.read_reg(hw, NGBE_MDI_PHY_ID2_OFFSET, 0, &phy_id_low);
	phy_id |= (phy_id_low & NGBE_MDI_PHY_ID_MASK) >> 10;

	if (phy_id != NGBE_INTERNAL_PHY_ID) {
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
			      "internal phy id 0x%x not supported.\n", phy_id);
		return NGBE_ERR_DEVICE_NOT_SUPPORTED;
	}
	hw->phy.id = (u32)phy_id;

	return NGBE_OK;
}

/**
 *  ngbe_read_phy_mdi - Reads a value from a specified PHY register without
 *  the SWFW lock
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit address of PHY register to read
 *  @phy_data: Pointer to read data from PHY register
 **/
static int ngbe_phy_read_reg_mdi(struct ngbe_hw *hw, u32 reg_addr,
				 u32 device_type, u16 *phy_data)
{
	u32 command;
	int status = 0;

	/* setup and write the address cycle command */
	command = NGBE_MSCA_RA(reg_addr) | NGBE_MSCA_PA(hw->phy.addr) |
		  NGBE_MSCA_DA(device_type);
	wr32(hw, NGBE_MSCA, command);

	command = NGBE_MSCC_CMD(NGBE_MSCA_CMD_READ) | NGBE_MSCC_BUSY |
		  NGBE_MDIO_CLK(6);
	wr32(hw, NGBE_MSCC, command);

	/* wait to complete */
	status = po32m(hw, NGBE_MSCC, NGBE_MSCC_BUSY, ~NGBE_MSCC_BUSY,
		       NGBE_MDIO_TIMEOUT, 10);
	if (status != 0) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
			      "PHY address command did not complete.\n");
		return NGBE_ERR_PHY;
	}

	/* read data from MSCC */
	*phy_data = 0xFFFF & rd32(hw, NGBE_MSCC);

	return 0;
}

/**
 *  ngbe_write_phy_reg_mdi - Writes a value to specified PHY register
 *  without SWFW lock
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit PHY register to write
 *  @device_type: 5 bit device type
 *  @phy_data: Data to write to the PHY register
 **/
static int ngbe_phy_write_reg_mdi(struct ngbe_hw *hw, u32 reg_addr,
				  u32 device_type, u16 phy_data)
{
	u32 command;
	int status = 0;

	/* setup and write the address cycle command */
	command = NGBE_MSCA_RA(reg_addr) | NGBE_MSCA_PA(hw->phy.addr) |
		  NGBE_MSCA_DA(device_type);
	wr32(hw, NGBE_MSCA, command);

	command = phy_data | NGBE_MSCC_CMD(NGBE_MSCA_CMD_WRITE) |
		  NGBE_MSCC_BUSY | NGBE_MDIO_CLK(6);
	wr32(hw, NGBE_MSCC, command);

	/* wait to complete */
	status = po32m(hw, NGBE_MSCC, NGBE_MSCC_BUSY, ~NGBE_MSCC_BUSY,
		       NGBE_MDIO_TIMEOUT, 10);
	if (status != 0) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
			      "PHY address command did not complete.\n");
		return NGBE_ERR_PHY;
	}

	return 0;
}

int ngbe_phy_read_reg_ext_yt8521s(struct ngbe_hw *hw, u32 reg_addr,
				  u32 device_type, u16 *phy_data)
{
	int status = 0;

	status = ngbe_phy_write_reg_mdi(hw, 0x1e, device_type, reg_addr);
	if (!status)
		status = ngbe_phy_read_reg_mdi(hw, 0x1f, device_type, phy_data);

	return status;
}

int ngbe_phy_write_reg_ext_yt8521s(struct ngbe_hw *hw, u32 reg_addr,
				   u32 device_type, u16 phy_data)
{
	int status = 0;

	status = ngbe_phy_write_reg_mdi(hw, 0x1e, device_type, reg_addr);
	if (!status)
		status =
			ngbe_phy_write_reg_mdi(hw, 0x1f, device_type, phy_data);

	return status;
}

static int ngbe_phy_read_reg_sds_ext_yt8521s(struct ngbe_hw *hw, u32 reg_addr,
					     u32 device_type, u16 *phy_data)
{
	int status = 0;

	status = ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x02);
	if (!status)
		status = ngbe_phy_read_reg_ext_yt8521s(hw, reg_addr,
						       device_type, phy_data);
	ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x00);

	return status;
}

static int ngbe_phy_write_reg_sds_ext_yt8521s(struct ngbe_hw *hw, u32 reg_addr,
					      u32 device_type, u16 phy_data)
{
	int status = 0;

	status = ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x02);
	if (!status)
		status = ngbe_phy_write_reg_ext_yt8521s(hw, reg_addr,
							device_type, phy_data);
	ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x00);

	return status;
}

int ngbe_phy_read_reg_sds_mii_yt8521s(struct ngbe_hw *hw, u32 reg_addr,
				      u32 device_type, u16 *phy_data)
{
	int status = 0;

	status = ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x02);
	if (!status)
		status = ngbe_phy_read_reg_mdi(hw, reg_addr, device_type,
					       phy_data);
	ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x00);

	return status;
}

static int ngbe_phy_write_reg_sds_mii_yt8521s(struct ngbe_hw *hw, u32 reg_addr,
					      u32 device_type, u16 phy_data)
{
	int status = 0;

	status = ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x02);
	if (!status)
		status = ngbe_phy_write_reg_mdi(hw, reg_addr, device_type,
						phy_data);
	ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x00);

	return status;
}

int ngbe_phy_write_reg_exphy(struct ngbe_hw *hw, u16 reg_offset, u16 page,
			     u16 phy_data)
{
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	hw->phy.ops.write_reg_mdi(hw, RTL821x_PAGE_SELECT, 0, page);
	hw->phy.ops.write_reg_mdi(hw, reg_offset, 0, phy_data);
	hw->phy.ops.write_reg_mdi(hw, RTL821x_PAGE_SELECT, 0, 0);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return NGBE_OK;
}

int ngbe_phy_read_reg_exphy(struct ngbe_hw *hw, u16 reg_offset, u16 page,
			    u16 *phy_data)
{
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	hw->phy.ops.write_reg_mdi(hw, RTL821x_PAGE_SELECT, 0, page);
	hw->phy.ops.read_reg_mdi(hw, reg_offset, 0, phy_data);
	hw->phy.ops.write_reg_mdi(hw, RTL821x_PAGE_SELECT, 0, 0);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return NGBE_OK;
}

static int ngbe_phy_write_reg_ext_yt8011a(struct ngbe_hw *hw, u16 reg_addr,
					  u16 phy_data)
{
	int status = 0;
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	status = hw->phy.ops.write_reg_mdi(hw, 0x1e, 0, reg_addr);
	if (!status)
		status = hw->phy.ops.write_reg_mdi(hw, 0x1f, 0, phy_data);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return status;
}

int ngbe_phy_read_reg_mmd_yt8011a(struct ngbe_hw *hw, u16 reg_addr,
				  u16 device_type, u16 *phy_data)
{
	int status = 0;
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	status = hw->phy.ops.write_reg_mdi(hw, 0xd, 0, device_type);
	if (!status) {
		status = hw->phy.ops.write_reg_mdi(hw, 0xe, 0, reg_addr);
		if (!status) {
			status = hw->phy.ops.write_reg_mdi(hw, 0xd, 0, 0x4000 + device_type);
			if (!status)
				status = hw->phy.ops.read_reg_mdi(hw, 0xe, 0,
								  phy_data);
		}
	}
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return status;
}

static int ngbe_phy_write_reg_mmd_yt8011a(struct ngbe_hw *hw, u16 reg_addr,
					  u16 device_type, u16 phy_data)
{
	int status = 0;
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	status = hw->phy.ops.write_reg_mdi(hw, 0xd, 0, device_type);
	if (!status) {
		status = hw->phy.ops.write_reg_mdi(hw, 0xe, 0, reg_addr);
		if (!status) {
			status = hw->phy.ops.write_reg_mdi(hw, 0xd, 0, 0x4000 + device_type);
			if (!status)
				status = hw->phy.ops.write_reg_mdi(hw, 0xe, 0,
								   phy_data);
		}
	}
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return status;
}

static int ngbe_phy_write_reg_mii_yt8011a(struct ngbe_hw *hw, u16 reg_addr,
					  u16 phy_data)
{
	int status = 0;
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	status = hw->phy.ops.write_reg_mdi(hw, reg_addr, 0, phy_data);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return status;
}

int ngbe_phy_read_reg_mii_yt8011a(struct ngbe_hw *hw, u16 reg_addr,
				  u16 *phy_data)
{
	int status = 0;
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	status = hw->phy.ops.read_reg_mdi(hw, reg_addr, 0, phy_data);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return status;
}

int ngbe_phy_write_reg_88e1111(struct ngbe_hw *hw, u16 reg_offset, u16 page,
			       u16 phy_data)
{
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	hw->phy.ops.write_reg_mdi(hw, reg_offset, 0, phy_data);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return NGBE_OK;
}

int ngbe_phy_read_reg_88e1111(struct ngbe_hw *hw, u16 reg_offset,
			      __always_unused u16 page, u16 *phy_data)
{
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	hw->phy.ops.read_reg_mdi(hw, reg_offset, 0, phy_data);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return NGBE_OK;
}

int ngbe_phy_read_reg_indirect(struct ngbe_hw *hw, u16 reg_offset,
			       u16 *phy_data)
{
	ngbe_phy_write_reg_exphy(hw, 0x1b, 0xa43, reg_offset);
	ngbe_phy_read_reg_exphy(hw, 0x1c, 0xa43, phy_data);

	return NGBE_OK;
}

static int ngbe_check_mdi_phy_id(struct ngbe_hw *hw)
{
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;
	u32 phy_id = 0;
	u8 value = 0;
	u32 phy_mode = 0;

	if (hw->phy.type == ngbe_phy_m88e1512) {
		/* select page 0 */
		ngbe_phy_write_reg_mdi(hw, 22, 0, 0);
	} else {
		/* select page 1 */
		ngbe_phy_write_reg_mdi(hw, 22, 0, 1);
	}

	ngbe_phy_read_reg_mdi(hw, NGBE_MDI_PHY_ID1_OFFSET, 0, &phy_id_high);
	phy_id = phy_id_high << 6;
	ngbe_phy_read_reg_mdi(hw, NGBE_MDI_PHY_ID2_OFFSET, 0, &phy_id_low);
	phy_id |= (phy_id_low & NGBE_MDI_PHY_ID_MASK) >> 10;

	if (phy_id != NGBE_M88E1512_PHY_ID) {
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
			      "MDI phy id 0x%x not supported.\n", phy_id);
		return NGBE_ERR_DEVICE_NOT_SUPPORTED;
	}
	hw->phy.id = phy_id;

	if (hw->phy.type == ngbe_phy_m88e1512_unknown) {
		ngbe_flash_read_dword(hw, 0xff010, &phy_mode);
		switch (hw->bus.lan_id) {
		case 0:
			value = (u8)phy_mode;
			break;
		case 1:
			value = (u8)(phy_mode >> 8);
			break;
		case 2:
			value = (u8)(phy_mode >> 16);
			break;
		case 3:
			value = (u8)(phy_mode >> 24);
			break;
		default:
			break;
		}
		if ((value & 0x7) == 0) {
			/* mode select to RGMII-to-copper */
			hw->phy.type = ngbe_phy_m88e1512;
		} else if ((value & 0x7) == 0x2) {
			/* mode select to RGMII-to-sfi */
			hw->phy.type = ngbe_phy_m88e1512_sfi;
		} else {
			ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
				      "marvell 88E1512 mode %x is not supported.\n", value);
			return NGBE_ERR_DEVICE_NOT_SUPPORTED;
		}
	}

	return NGBE_OK;
}

static bool ngbe_validate_phy_addr(struct ngbe_hw *hw, u32 phy_addr)
{
	u16 phy_id = 0;
	bool valid = false;
	unsigned long flags;

	hw->phy.addr = phy_addr;

	spin_lock_irqsave(&hw->phy_lock, flags);
	ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x3, 0, &phy_id);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	if (phy_id != 0xFFFF && phy_id != 0x0)
		valid = true;

	return valid;
}

static int ngbe_check_yt_phy_id(struct ngbe_hw *hw)
{
	u16 phy_id = 0;
	u16 phyid_match_model = 0;
	bool valid = false;
	u32 phy_addr;
	unsigned long flags;
	struct ngbe_adapter *adapter = hw->back;

	if (hw->ncsi_enabled) {
		ngbe_get_hw_control(adapter);
		msleep(50);
	}

	for (phy_addr = 0; phy_addr < 32; phy_addr++) {
		valid = ngbe_validate_phy_addr(hw, phy_addr);
		if (valid) {
			hw->phy.addr = phy_addr;
			break;
		}
	}
	if (!valid) {
		if (hw->ncsi_enabled)
			ngbe_release_hw_control(adapter);
		return NGBE_ERR_DEVICE_NOT_SUPPORTED;
	}

	spin_lock_irqsave(&hw->phy_lock, flags);
	ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x3, 0, &phy_id);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	phyid_match_model = phy_id >> NGBE_YT_PHY_ID_MATCH_MODEL_SHIFT;
	if (phyid_match_model != NGBE_YT8521S_PHY_ID_MATCH_MODEL &&
	    phyid_match_model != NGBE_YT8531S_PHY_ID_MATCH_MODEL &&
	    phyid_match_model != NGBE_SK21101_PHY_ID_MATCH_MODEL) {
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
			      "MDI phy id 0x%x not supported.\n", phy_id);
		if (hw->ncsi_enabled)
			ngbe_release_hw_control(adapter);
		return NGBE_ERR_DEVICE_NOT_SUPPORTED;
	}
	hw->phy.id = phy_id;

	if (hw->ncsi_enabled)
		ngbe_release_hw_control(adapter);

	return NGBE_OK;
}

static bool ngbe_validate_ex_genneric_phy_addr(struct ngbe_hw *hw, u32 phy_addr)
{
	u16 phy_id = 0;
	bool valid = false;
	unsigned long flags;

	hw->phy.addr = phy_addr;

	spin_lock_irqsave(&hw->phy_lock, flags);
	ngbe_phy_read_reg_mdi(hw, NGBE_MDI_PHY_ID2_OFFSET, 0, &phy_id);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	if (phy_id != 0xFFFF && phy_id != 0x0)
		valid = true;

	return valid;
}

static int ngbe_check_ex_generic_phy_id(struct ngbe_hw *hw)
{
	u16 phy_id1 = 0;
	u16 phy_id2 = 0;
	bool valid = false;
	u32 phy_addr;

	for (phy_addr = 0; phy_addr < 4; phy_addr++) {
		valid = ngbe_validate_ex_genneric_phy_addr(hw, phy_addr);
		if (valid) {
			hw->phy.addr = phy_addr;
			break;
		}
	}
	if (!valid)
		return NGBE_ERR_DEVICE_NOT_SUPPORTED;
	ngbe_phy_read_reg_mdi(hw, NGBE_MDI_PHY_ID1_OFFSET, 0, &phy_id1);
	ngbe_phy_read_reg_mdi(hw, NGBE_MDI_PHY_ID2_OFFSET, 0, &phy_id2);

	if (phy_id1 == NGBE_JL2201_PHY_ID1 && phy_id2 == NGBE_JL2201_PHY_ID2) {
		hw->phy.id = phy_id1;
		hw->phy.phydev_type = ngbe_phy_jl;
	} else if (phy_id1 == NGBE_RTL8211F_PHY_ID1 &&
		   phy_id2 == NGBE_RTL8211F_PHY_ID2) {
		hw->phy.id = phy_id1;
		hw->phy.phydev_type = ngbe_phy_rtl_8211f;
	} else if (phy_id1 == NGBE_YT8011A_PHY_ID1 &&
		   phy_id2 == NGBE_YT8011A_PHY_ID2) {
		hw->phy.id = phy_id1;
		hw->phy.phydev_type = ngbe_phy_yt_8011a;
	} else if (phy_id1 == NGBE_88E1111_PHY_ID1 &&
		   phy_id2 == NGBE_88E1111_PHY_ID2) {
		hw->phy.id = phy_id1;
		hw->phy.phydev_type = ngbe_phy_88e1111;
	} else {
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
			      "external phy id 0x%x not supported.\n",
			      phy_id1 << 16 | phy_id2);
		return NGBE_ERR_DEVICE_NOT_SUPPORTED;
	}

	return NGBE_OK;
}

/* ngbe_init_phy_ops - PHY/SFP specific init
 * @hw: pointer to hardware structure
 * Initialize any function pointers that were not able to be
 * set during init_shared_code because the PHY/SFP type was
 * not known.  Perform the SFP init if necessary.
 */
static int ngbe_phy_init(struct ngbe_hw *hw)
{
	int ret_val = 0;
	u16 value = 0;
	int i;
	u8 lan_id = hw->bus.lan_id;
	struct ngbe_adapter *adapter = hw->back;
	unsigned long flags;

	/* set fwsw semaphore mask for phy first */
	if (!hw->phy.phy_semaphore_mask)
		hw->phy.phy_semaphore_mask = NGBE_MNG_SWFW_SYNC_SW_PHY;

	if (hw->phy.type == ngbe_phy_fpga)
		return 0;
	/* init phy.addr according to HW design */

	hw->phy.addr = 0;

	/* Identify the PHY or SFP module */
	ret_val = hw->phy.ops.identify(hw);
	if (ret_val == NGBE_ERR_SFP_NOT_SUPPORTED)
		return ret_val;

	/* enable interrupts, only link status change and an done is allowed */
	if (hw->phy.type == ngbe_phy_internal ||
	    hw->phy.type == ngbe_phy_internal_yt8521s_sfi) {
		value = NGBE_INTPHY_INT_LSC | NGBE_INTPHY_INT_ANC;
		hw->phy.ops.write_reg(hw, 0x12, 0xa42, value);
		ngbe_flash_read_dword(hw, 0xfe010 + lan_id * 8,
				      &adapter->gphy_efuse[0]);
		ngbe_flash_read_dword(hw, 0xfe010 + lan_id * 8 + 4,
				      &adapter->gphy_efuse[1]);
	} else if (hw->phy.type == ngbe_phy_m88e1512 ||
		   hw->phy.type == ngbe_phy_m88e1512_sfi) {
		hw->phy.ops.write_reg_mdi(hw, 22, 0, 2);
		hw->phy.ops.read_reg_mdi(hw, 21, 0, &value);
		value &= ~NGBE_M88E1512_RGM_TTC;
		value |= NGBE_M88E1512_RGM_RTC;
		hw->phy.ops.write_reg_mdi(hw, 21, 0, value);
		if (hw->phy.type == ngbe_phy_m88e1512)
			hw->phy.ops.write_reg_mdi(hw, 22, 0, 0);
		else
			hw->phy.ops.write_reg_mdi(hw, 22, 0, 1);

		hw->phy.ops.write_reg_mdi(hw, 0, 0, NGBE_MDI_PHY_RESET);
		for (i = 0; i < 15; i++) {
			hw->phy.ops.read_reg_mdi(hw, 0, 0, &value);
			if (value & NGBE_MDI_PHY_RESET)
				msleep(20);
			else
				break;
		}

		if (i == 15) {
			ERROR_REPORT1(NGBE_ERROR_POLLING,
				      "phy reset exceeds maximum waiting period.\n");
			return NGBE_ERR_PHY_TIMEOUT;
		}

		ret_val = hw->phy.ops.reset(hw);
		if (ret_val)
			return ret_val;

		/* set LED2 to interrupt output and INTn active low */
		hw->phy.ops.write_reg_mdi(hw, 22, 0, 3);
		hw->phy.ops.read_reg_mdi(hw, 18, 0, &value);
		value |= NGBE_M88E1512_INT_EN;
		value &= ~(NGBE_M88E1512_INT_POL);
		hw->phy.ops.write_reg_mdi(hw, 18, 0, value);

		if (hw->phy.type == ngbe_phy_m88e1512_sfi) {
			hw->phy.ops.write_reg_mdi(hw, 22, 0, 1);
			hw->phy.ops.read_reg_mdi(hw, 16, 0, &value);
			value &= ~0x4;
			hw->phy.ops.write_reg_mdi(hw, 16, 0, value);
		}

		/* enable link status change and AN complete interrupts */
		value = NGBE_M88E1512_INT_ANC | NGBE_M88E1512_INT_LSC;
		if (hw->phy.type == ngbe_phy_m88e1512)
			hw->phy.ops.write_reg_mdi(hw, 22, 0, 0);
		else
			hw->phy.ops.write_reg_mdi(hw, 22, 0, 1);
		hw->phy.ops.write_reg_mdi(hw, 18, 0, value);

		hw->phy.ops.read_reg_mdi(hw, 0, 0, &value);
		value |= 0x800;
		hw->phy.ops.write_reg_mdi(hw, 0, 0, value);
	} else if (hw->phy.type == ngbe_phy_yt8521s_sfi) {
		/* check chip_mode first */
		spin_lock_irqsave(&hw->phy_lock, flags);
		ngbe_phy_read_reg_ext_yt8521s(hw, 0xA001, 0, &value);
		spin_unlock_irqrestore(&hw->phy_lock, flags);

		/* fix: yt8531sh-ca optimize -ca utp interface*/
		spin_lock_irqsave(&hw->phy_lock, flags);
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA023, 0, 0x4031);
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA000, 0, 0x0);
		ngbe_phy_read_reg_mdi(hw, 0x12, 0, &value);

		if (value != 0x31 && value != 0x32) {
			ngbe_phy_write_reg_ext_yt8521s(hw, 0xA071, 0, 0x9007);
			ngbe_phy_write_reg_ext_yt8521s(hw, 0x52, 0, 0x231D);
			ngbe_phy_write_reg_ext_yt8521s(hw, 0x51, 0, 0x04A9);
			ngbe_phy_write_reg_ext_yt8521s(hw, 0x57, 0, 0x274C);
			ngbe_phy_write_reg_ext_yt8521s(hw, 0xA006, 0, 0x10D);
		}
		spin_unlock_irqrestore(&hw->phy_lock, flags);

		if (NGBE_POLL_LINK_STATUS != 1) {
			/*enable yt8521s interrupt*/
			/* select sds area register */
			spin_lock_irqsave(&hw->phy_lock, flags);
			ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, 0, 0x00);

			/* enable interrupt */
			value = 0x0C0C;
			ngbe_phy_write_reg_mdi(hw, 0x12, 0, value);
			spin_unlock_irqrestore(&hw->phy_lock, flags);
		}
		if (!hw->ncsi_enabled) {
			/* power down in Fiber mode */
			spin_lock_irqsave(&hw->phy_lock, flags);
			ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x0, 0, &value);
			value |= 0x800;
			ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, value);

			/* power down in UTP mode */
			ngbe_phy_read_reg_mdi(hw, 0x0, 0, &value);
			value |= 0x800;
			ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);
			spin_unlock_irqrestore(&hw->phy_lock, flags);
		}

		/* initialize default type */
		hw->phy.phydev_type = ngbe_yt_serdes;
	} else if (hw->phy.type == ngbe_phy_external) {
		if (hw->phy.phydev_type == ngbe_phy_jl) {
			hw->phy.ops.check_event(hw);
			/* enable interrupt */
			ngbe_phy_read_reg_exphy(hw, 16, 158, &value);
			ngbe_phy_write_reg_exphy(hw, 16, 158,
						 value | (1 << 14));

			ngbe_phy_read_reg_exphy(hw, 21, 160, &value);
			ngbe_phy_write_reg_exphy(hw, 21, 160,
						 value | (1 << 11));

			ngbe_phy_read_reg_exphy(hw, RTL821x_INER, 0xa42,
						&value);
			ngbe_phy_write_reg_exphy(hw, RTL821x_INER, 0xa42,
						 value | RTL8211F_INER_LINK_CHANGE);

			ngbe_phy_read_reg_exphy(hw, 0x11, 0xd08, &value);
			value |= JL2201_TX_DELAY | JL2201_RX_DELAY;
			ngbe_phy_write_reg_exphy(hw, 0x11, 0xd08, value);

			/* power down */
			ngbe_phy_read_reg_exphy(hw, NGBE_EXPHY_MII_BMCR, 0,
						&value);
			value |= NGBE_EXPHY_BMCR_PDOWN;
			ngbe_phy_write_reg_exphy(hw, NGBE_EXPHY_MII_BMCR, 0,
						 value);
		} else if (hw->phy.phydev_type == ngbe_phy_rtl_8211f) {
			ngbe_phy_write_reg_exphy(hw, RTL821x_INER, 0xa42,
						 RTL8211F_INER_LINK_CHANGE);
			ngbe_phy_write_reg_exphy(hw, 0x11, 0xde1,
						 RTL8211F_INER_LINK_CHANGE);

			value = RTL8211F_RX_DELAY;
			ngbe_phy_write_reg_exphy(hw, 0x15, 0xd08, value);

			/* power down */
			ngbe_phy_read_reg_exphy(hw, NGBE_EXPHY_MII_BMCR, 0xdc0,
						&value);
			value |= NGBE_EXPHY_BMCR_PDOWN;
			ngbe_phy_write_reg_exphy(hw, NGBE_EXPHY_MII_BMCR, 0xdc0,
						 value);
		} else if (hw->phy.phydev_type == ngbe_phy_yt_8011a) {
			/* basic config base on yt application note release 1.0*/

			/* reduce power consumption */
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x1008, 0x2119);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x1092, 0x0712);

			/* under voltage config */
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x90bc, 0x6661);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x90b9, 0x620b);

			/* reduce phyc send_s */
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x2001, 0x6418);

			/* 100M template psd */
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x1019, 0x3712);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x101a, 0x3713);

			/* iop */
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x2005, 0x0810);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x2015, 0x1012);

			/* link up */
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x2013, 0xff06);

			/* pll */
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x1053, 0xf);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x105e, 0xa46c);

			/* sleep config */
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x1088, 0x2b);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x1088, 0x2b);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x1088, 0xb);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x3008, 0x143);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x3009, 0x1918);

			/* csd threshold */
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x9095, 0x1a1a);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x9096, 0x1a10);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x9097, 0x101a);

			/* config rgmii voltage 3.3v */
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x9000, 0x8000);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x0062, 0x0);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x9000, 0x0);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x9031, 0xb200);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x903b, 0x0040);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x903e, 0x3b3b);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x903c, 0x000f);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x903d, 0x1000);
			ngbe_phy_write_reg_ext_yt8011a(hw, 0x9030, 0x0);

			/* enable link status change interrupt */
			ngbe_phy_write_reg_mii_yt8011a(hw, 0x12, 0xc00);

			/* power down */
			ngbe_phy_read_reg_mii_yt8011a(hw, 0, &value);
			value |= NGBE_PHY_BMCR_PDOWN;
			ngbe_phy_write_reg_mii_yt8011a(hw, 0, value);
		} else if (hw->phy.phydev_type == ngbe_phy_88e1111) {
			if (NGBE_POLL_LINK_STATUS != 1) {
				/* enable interrupt */
				ngbe_phy_read_reg_88e1111(hw, NGBE_88E1111_INER,
							  0, &value);
				value |= (NGBE_88E1111_ANC |
					  NGBE_88E1111_LINK_CHANGE);
				ngbe_phy_write_reg_88e1111(hw, NGBE_88E1111_INER, 0, value);
			}

			value &= ~NGBE_88E1111_RX_DELAY;
			ngbe_phy_write_reg_88e1111(hw, NGBE_88E1111_ESCR, 0,
						   value);

			/* power down */
			ngbe_phy_read_reg_88e1111(hw, NGBE_EXPHY_MII_BMCR, 0,
						  &value);
			value |= NGBE_EXPHY_BMCR_PDOWN;
			ngbe_phy_write_reg_88e1111(hw, NGBE_EXPHY_MII_BMCR, 0,
						   value);
		}
	}

	return ret_val;
}

/**
 *  ngbe_identify_module - Identifies module type
 *  @hw: pointer to hardware structure
 *  Determines HW type and calls appropriate function.
 **/
static int ngbe_phy_identify(struct ngbe_hw *hw)
{
	int status = 0;

	switch (hw->phy.type) {
	case ngbe_phy_internal:
	case ngbe_phy_internal_yt8521s_sfi:
		status = ngbe_check_internal_phy_id(hw);
		break;
	case ngbe_phy_m88e1512:
	case ngbe_phy_m88e1512_sfi:
	case ngbe_phy_m88e1512_unknown:
		status = ngbe_check_mdi_phy_id(hw);
		break;
	case ngbe_phy_yt8521s_sfi:
		status = ngbe_check_yt_phy_id(hw);
		break;
	case ngbe_phy_external:
		status = ngbe_check_ex_generic_phy_id(hw);
		break;
	default:
		status = NGBE_ERR_PHY_TYPE;
		break;
	}

	return status;
}

static int ngbe_gphy_reset(struct ngbe_hw *hw, bool need_restart_AN)
{
	int status, i;
	u16 val;

	if (!need_restart_AN)
		return 0;

	val = NGBE_MDI_PHY_RESET;
	status = hw->phy.ops.write_reg(hw, 0, 0, val);
	for (i = 0; i < NGBE_PHY_RST_WAIT_PERIOD; i++) {
		status = hw->phy.ops.read_reg(hw, 0, 0, &val);
		if (!(val & NGBE_MDI_PHY_RESET))
			break;
		msleep(20);
	}

	if (i == NGBE_PHY_RST_WAIT_PERIOD) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
			      "PHY MODE RESET did not complete.\n");
		return NGBE_ERR_RESET_FAILED;
	}

	return status;
}

static int ngbe_phy_reset(struct ngbe_hw *hw)
{
	int status = 0;

	u16 value = 0;
	int i;

	/* only support internal phy */
	if (hw->phy.type != ngbe_phy_internal &&
	    hw->phy.type != ngbe_phy_internal_yt8521s_sfi) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
			      "%s: operation not supported.\n", __func__);
		return NGBE_ERR_PHY_TYPE;
	}

	/* Don't reset PHY if it's shut down due to overtemp. */
	if (!hw->phy.reset_if_overtemp &&
	    hw->phy.ops.check_overtemp(hw) == NGBE_ERR_OVERTEMP) {
		ERROR_REPORT1(NGBE_ERROR_CAUTION,
			      "OVERTEMP! Skip PHY reset.\n");
		return NGBE_ERR_OVERTEMP;
	}

	/* Blocked by MNG FW so bail */
	if (ngbe_check_reset_blocked(hw))
		return status;

	value |= NGBE_MDI_PHY_RESET;
	status = hw->phy.ops.write_reg(hw, 0, 0, value);
	for (i = 0; i < NGBE_PHY_RST_WAIT_PERIOD; i++) {
		status = hw->phy.ops.read_reg(hw, 0, 0, &value);
		if (!(value & NGBE_MDI_PHY_RESET))
			break;
		msleep(20);
	}

	if (i == NGBE_PHY_RST_WAIT_PERIOD) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
			      "PHY MODE RESET did not complete.\n");
		return NGBE_ERR_RESET_FAILED;
	}

	return status;
}

static u32 ngbe_phy_setup_link(struct ngbe_hw *hw, u32 speed,
			       bool need_restart_AN)
{
	u16 value = 0;
	int status = 0;

	status = ngbe_gphy_reset(hw, need_restart_AN);
	if (!hw->mac.autoneg) {
		if (status) {
			ERROR_REPORT1(NGBE_ERROR_POLLING,
				      "call phy reset return %d.\n", status);
			return NGBE_ERR_PHY;
		}

		switch (speed) {
		case NGBE_LINK_SPEED_1GB_FULL:
			value = NGBE_MDI_PHY_SPEED_SELECT1;
			break;
		case NGBE_LINK_SPEED_100_FULL:
			value = NGBE_MDI_PHY_SPEED_SELECT0;
			break;
		case NGBE_LINK_SPEED_10_FULL:
			value = 0;
			break;
		default:
			value = NGBE_MDI_PHY_SPEED_SELECT0 |
				NGBE_MDI_PHY_SPEED_SELECT1;
			ERROR_REPORT1(NGBE_ERROR_CAUTION,
				      "unknown speed = 0x%x.\n", speed);
			break;
		}
		/* duplex full */
		value |= NGBE_MDI_PHY_DUPLEX;
		hw->phy.ops.write_reg(hw, 0, 0, value);

		goto skip_an;
	}

	/* disable 10/100M Half Duplex */
	hw->phy.ops.read_reg(hw, 4, 0, &value);
	value &= 0xFF5F;
	hw->phy.ops.write_reg(hw, 4, 0, value);

	/* set advertise enable according to input speed */
	if (!(speed & NGBE_LINK_SPEED_1GB_FULL)) {
		hw->phy.ops.read_reg(hw, 9, 0, &value);
		value &= 0xFDFF;
		hw->phy.ops.write_reg(hw, 9, 0, value);
	} else {
		hw->phy.ops.read_reg(hw, 9, 0, &value);
		value |= 0x200;
		hw->phy.ops.write_reg(hw, 9, 0, value);
	}

	if (!(speed & NGBE_LINK_SPEED_100_FULL)) {
		hw->phy.ops.read_reg(hw, 4, 0, &value);
		value &= 0xFEFF;
		hw->phy.ops.write_reg(hw, 4, 0, value);
	} else {
		hw->phy.ops.read_reg(hw, 4, 0, &value);
		value |= 0x100;
		hw->phy.ops.write_reg(hw, 4, 0, value);
	}

	if (!(speed & NGBE_LINK_SPEED_10_FULL)) {
		hw->phy.ops.read_reg(hw, 4, 0, &value);
		value &= 0xFFBF;
		hw->phy.ops.write_reg(hw, 4, 0, value);
	} else {
		hw->phy.ops.read_reg(hw, 4, 0, &value);
		value |= 0x40;
		hw->phy.ops.write_reg(hw, 4, 0, value);
	}

	/* restart AN and wait AN done interrupt */
	if (hw->ncsi_enabled) {
		if (need_restart_AN)
			value = NGBE_MDI_PHY_RESTART_AN | NGBE_MDI_PHY_ANE;
		else
			value = NGBE_MDI_PHY_ANE;
	} else {
		value = NGBE_MDI_PHY_RESTART_AN | NGBE_MDI_PHY_ANE;
	}

	hw->phy.ops.write_reg(hw, 0, 0, value);
skip_an:
	hw->phy.ops.phy_led_ctrl(hw);

	hw->phy.ops.check_event(hw);

	return NGBE_OK;
}

static u32 ngbe_phy_led_ctrl(struct ngbe_hw *hw)
{
	u16 value = 0;
	struct ngbe_adapter *adapter = hw->back;

	if (adapter->led_conf != -1)
		value = adapter->led_conf & 0xffff;
	else
		value = 0x205B;
	hw->phy.ops.write_reg(hw, 16, 0xd04, value);
	hw->phy.ops.write_reg(hw, 17, 0xd04, 0);

	hw->phy.ops.read_reg(hw, 18, 0xd04, &value);
	if (adapter->led_conf != -1) {
		value &= ~0x73;
		value |= adapter->led_conf >> 16;
	} else {
		value = value & 0xFFFC;
		/*act led blinking mode set to 60ms*/
		value |= 0x2;
	}
	hw->phy.ops.write_reg(hw, 18, 0xd04, value);

	return 0;
}

static int ngbe_phy_reset_m88e1512(struct ngbe_hw *hw)
{
	int status = 0;

	u16 value = 0;
	int i;

	if (hw->phy.type != ngbe_phy_m88e1512 &&
	    hw->phy.type != ngbe_phy_m88e1512_sfi)
		return NGBE_ERR_PHY_TYPE;

	/* Don't reset PHY if it's shut down due to overtemp. */
	if (hw->phy.ops.check_overtemp) {
		if (!hw->phy.reset_if_overtemp &&
		    hw->phy.ops.check_overtemp(hw) == NGBE_ERR_OVERTEMP) {
			ERROR_REPORT1(NGBE_ERROR_CAUTION,
				      "OVERTEMP! Skip PHY reset.\n");
			return NGBE_ERR_OVERTEMP;
		}
	}

	/* Blocked by MNG FW so bail */
	if (ngbe_check_reset_blocked(hw))
		return status;

	/* select page 18 reg 20 */
	status = hw->phy.ops.write_reg_mdi(hw, 22, 0, 18);

	if (hw->phy.type == ngbe_phy_m88e1512)
		/* mode select to RGMII-to-copper */
		value = 0;
	else
		/* mode select to RGMII-to-sfi */
		value = 2;
	status = hw->phy.ops.write_reg_mdi(hw, 20, 0, value);
	/* mode reset */
	value |= NGBE_MDI_PHY_RESET;
	status = hw->phy.ops.write_reg_mdi(hw, 20, 0, value);

	for (i = 0; i < NGBE_PHY_RST_WAIT_PERIOD; i++) {
		status = hw->phy.ops.read_reg_mdi(hw, 20, 0, &value);
		if (!(value & NGBE_MDI_PHY_RESET))
			break;
		msleep(20);
	}

	if (i == NGBE_PHY_RST_WAIT_PERIOD) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
			      "M88E1512 MODE RESET did not complete.\n");
		return NGBE_ERR_RESET_FAILED;
	}

	return status;
}

static int ngbe_phy_reset_yt8521s(struct ngbe_hw *hw)
{
	int status = 0;

	u16 value = 0;
	int i;
	unsigned long flags;

	if (hw->phy.type != ngbe_phy_yt8521s_sfi)
		return NGBE_ERR_PHY_TYPE;

	if (hw->ncsi_enabled)
		return status;

	/* Don't reset PHY if it's shut down due to overtemp. */
	if (hw->phy.ops.check_overtemp) {
		if (!hw->phy.reset_if_overtemp &&
		    hw->phy.ops.check_overtemp(hw) == NGBE_ERR_OVERTEMP) {
			ERROR_REPORT1(NGBE_ERROR_CAUTION,
				      "OVERTEMP! Skip PHY reset.\n");
			return NGBE_ERR_OVERTEMP;
		}
	}

	/* Blocked by MNG FW so bail */
	if (ngbe_check_reset_blocked(hw))
		return status;

	/* check chip_mode first */
	spin_lock_irqsave(&hw->phy_lock, flags);
	ngbe_phy_read_reg_ext_yt8521s(hw, 0xa001, 0, &value);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	if ((value & 7) != 0) { /* fiber_to_rgmii */
		spin_lock_irqsave(&hw->phy_lock, flags);
		status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0, 0, &value);
		/* sds software reset */
		value |= 0x8000;
		status = ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0, 0, value);
		spin_unlock_irqrestore(&hw->phy_lock, flags);

		for (i = 0; i < NGBE_PHY_RST_WAIT_PERIOD; i++) {
			spin_lock_irqsave(&hw->phy_lock, flags);
			status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0, 0,
								   &value);
			spin_unlock_irqrestore(&hw->phy_lock, flags);

			if (!(value & 0x8000))
				break;
			msleep(20);
		}
	} else { /* utp_to_rgmii */
		spin_lock_irqsave(&hw->phy_lock, flags);
		status = ngbe_phy_read_reg_mdi(hw, 0, 0, &value);
		/* software reset */
		value |= 0x8000;
		status = ngbe_phy_write_reg_mdi(hw, 0, 0, value);
		spin_unlock_irqrestore(&hw->phy_lock, flags);

		for (i = 0; i < NGBE_PHY_RST_WAIT_PERIOD; i++) {
			spin_lock_irqsave(&hw->phy_lock, flags);
			status = ngbe_phy_read_reg_mdi(hw, 0, 0, &value);
			spin_unlock_irqrestore(&hw->phy_lock, flags);
			if (!(value & 0x8000))
				break;
			msleep(20);
		}
	}

	if (i == NGBE_PHY_RST_WAIT_PERIOD) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
			      "YT8521S Software RESET did not complete.\n");
		return NGBE_ERR_RESET_FAILED;
	}

	return status;
}

static u32 ngbe_phy_setup_link_m88e1512(struct ngbe_hw *hw, u32 speed,
					bool autoneg_wait_to_complete)
{
	u16 value_r4 = 0;
	u16 value_r9 = 0;
	u16 value = 0;
	struct ngbe_adapter *adapter = hw->back;

	if (adapter->led_conf == -1) {
		/* LED control */
		hw->phy.ops.write_reg_mdi(hw, 22, 0, 3);
		hw->phy.ops.read_reg_mdi(hw, 16, 0, &value);
		value &= ~0x00FF;
		value |= (NGBE_M88E1512_LED1_CONF << 4) |
			 NGBE_M88E1512_LED0_CONF;
		hw->phy.ops.write_reg_mdi(hw, 16, 0, value);
		hw->phy.ops.read_reg_mdi(hw, 17, 0, &value);
		value &= ~0x000F;
		value |= (NGBE_M88E1512_LED1_POL << 2) | NGBE_M88E1512_LED0_POL;
		hw->phy.ops.write_reg_mdi(hw, 17, 0, value);
	}

	hw->phy.autoneg_advertised = 0;
	if (hw->phy.type == ngbe_phy_m88e1512) {
		if (!hw->mac.autoneg) {
			switch (speed) {
			case NGBE_LINK_SPEED_1GB_FULL:
				value = NGBE_MDI_PHY_SPEED_SELECT1;
				break;
			case NGBE_LINK_SPEED_100_FULL:
				value = NGBE_MDI_PHY_SPEED_SELECT0;
				break;
			case NGBE_LINK_SPEED_10_FULL:
				value = 0;
				break;
			default:
				value = NGBE_MDI_PHY_SPEED_SELECT0 |
					NGBE_MDI_PHY_SPEED_SELECT1;
				ERROR_REPORT1(NGBE_ERROR_CAUTION,
					      "unknown speed = 0x%x.\n", speed);
				break;
			}
			/* duplex full */
			value |= NGBE_MDI_PHY_DUPLEX | 0x8000;
			ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);

			goto skip_an;
		}
		if (speed & NGBE_LINK_SPEED_1GB_FULL) {
			value_r9 |= NGBE_M88E1512_1000BASET_FULL;
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;
		}

		if (speed & NGBE_LINK_SPEED_100_FULL) {
			value_r4 |= NGBE_M88E1512_100BASET_FULL;
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_100_FULL;
		}

		if (speed & NGBE_LINK_SPEED_10_FULL) {
			value_r4 |= NGBE_M88E1512_10BASET_FULL;
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_10_FULL;
		}

		hw->phy.ops.write_reg_mdi(hw, 22, 0, 0);
		hw->phy.ops.read_reg_mdi(hw, 4, 0, &value);
		value &= ~(NGBE_M88E1512_100BASET_FULL |
			   NGBE_M88E1512_100BASET_HALF |
			   NGBE_M88E1512_10BASET_FULL |
			   NGBE_M88E1512_10BASET_HALF);
		value_r4 |= value;
		hw->phy.ops.write_reg_mdi(hw, 4, 0, value_r4);

		hw->phy.ops.write_reg_mdi(hw, 22, 0, 0);
		hw->phy.ops.read_reg_mdi(hw, 9, 0, &value);
		value &= ~(NGBE_M88E1512_1000BASET_FULL |
			   NGBE_M88E1512_1000BASET_HALF);
		value_r9 |= value;
		hw->phy.ops.write_reg_mdi(hw, 9, 0, value_r9);

		value = NGBE_MDI_PHY_RESTART_AN | NGBE_MDI_PHY_ANE |
			NGBE_MDI_PHY_RESET | NGBE_MDI_PHY_DUPLEX;
		hw->phy.ops.write_reg_mdi(hw, 0, 0, value);
	} else {
		hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;
		hw->phy.ops.write_reg_mdi(hw, 22, 0, 1);
		hw->phy.ops.read_reg_mdi(hw, 4, 0, &value);
		value &= ~0x60;
		value |= 0x20;
		hw->phy.ops.write_reg_mdi(hw, 4, 0, value);

		if (hw->mac.autoneg)
			value = NGBE_MDI_PHY_RESTART_AN | NGBE_MDI_PHY_ANE |
				NGBE_MDI_PHY_RESET | NGBE_MDI_PHY_DUPLEX |
				NGBE_MDI_PHY_SPEED_SELECT1;
		else
			value = NGBE_MDI_PHY_RESET | NGBE_MDI_PHY_DUPLEX |
				NGBE_MDI_PHY_SPEED_SELECT1;
		hw->phy.ops.write_reg_mdi(hw, 0, 0, value);
	}
	hw->phy.ops.read_reg_mdi(hw, 0, 0, &value);
skip_an:
	hw->phy.ops.read_reg_mdi(hw, 0, 0, &value);
	value &= ~0x800;
	hw->phy.ops.write_reg_mdi(hw, 0, 0, value);
	msleep(20);

	hw->phy.ops.check_event(hw);

	return NGBE_OK;
}

static u32 ngbe_phy_setup_link_yt8521s(struct ngbe_hw *hw, u32 speed,
				       bool autoneg_wait_to_complete)
{
	int ret_val = 0;
	u16 value = 0;
	u16 value_r4 = 0;
	u16 value_r9 = 0;
	unsigned long flags;

	if (hw->ncsi_enabled)
		return ret_val;
	hw->phy.autoneg_advertised = 0;

	/* check chip_mode first */
	spin_lock_irqsave(&hw->phy_lock, flags);
	ngbe_phy_read_reg_ext_yt8521s(hw, 0xA001, 0, &value);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	if ((value & 7) == 0) { /* utp_to_rgmii */
		hw->phy.phydev_type = ngbe_yt_utp;
		if (!hw->mac.autoneg) {
			switch (speed) {
			case NGBE_LINK_SPEED_1GB_FULL:
				value = NGBE_MDI_PHY_SPEED_SELECT1;
				break;
			case NGBE_LINK_SPEED_100_FULL:
				value = NGBE_MDI_PHY_SPEED_SELECT0;
				break;
			case NGBE_LINK_SPEED_10_FULL:
				value = 0;
				break;
			default:
				value = NGBE_MDI_PHY_SPEED_SELECT0 |
					NGBE_MDI_PHY_SPEED_SELECT1;
				ERROR_REPORT1(NGBE_ERROR_CAUTION,
					      "unknown speed = 0x%x.\n", speed);
				break;
			}
			/* duplex full */
			value |= NGBE_MDI_PHY_DUPLEX | 0x8000;
			spin_lock_irqsave(&hw->phy_lock, flags);
			ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);
			spin_unlock_irqrestore(&hw->phy_lock, flags);

			goto skip_an;
		}

		value_r4 = 0x1E0;
		value_r9 = 0x300;
		/*disable 100/10base-T Self-negotiation ability*/
		spin_lock_irqsave(&hw->phy_lock, flags);
		ngbe_phy_read_reg_mdi(hw, 0x4, 0, &value);
		value &= ~value_r4;
		ngbe_phy_write_reg_mdi(hw, 0x4, 0, value);

		/*disable 1000base-T Self-negotiation ability*/
		ngbe_phy_read_reg_mdi(hw, 0x9, 0, &value);
		value &= ~value_r9;
		ngbe_phy_write_reg_mdi(hw, 0x9, 0, value);

		value_r4 = 0x0;
		value_r9 = 0x0;

		if (speed & NGBE_LINK_SPEED_1GB_FULL) {
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;
			value_r9 |= 0x200;
		}
		if (speed & NGBE_LINK_SPEED_100_FULL) {
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_100_FULL;
			value_r4 |= 0x100;
		}
		if (speed & NGBE_LINK_SPEED_10_FULL) {
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_10_FULL;
			value_r4 |= 0x40;
		}

		/* enable 1000base-T Self-negotiation ability */
		ngbe_phy_read_reg_mdi(hw, 0x9, 0, &value);
		value |= value_r9;
		ngbe_phy_write_reg_mdi(hw, 0x9, 0, value);

		/* enable 100/10base-T Self-negotiation ability */
		ngbe_phy_read_reg_mdi(hw, 0x4, 0, &value);
		value |= value_r4;
		ngbe_phy_write_reg_mdi(hw, 0x4, 0, value);

		/* software reset to make the above configuration take effect*/
		ngbe_phy_read_reg_mdi(hw, 0x0, 0, &value);
		value |= 0x9200;
		ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);
		spin_unlock_irqrestore(&hw->phy_lock, flags);
skip_an:
		/* power on in UTP mode */
		spin_lock_irqsave(&hw->phy_lock, flags);
		ngbe_phy_read_reg_mdi(hw, 0x0, 0, &value);
		value &= ~0x800;
		ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);
		spin_unlock_irqrestore(&hw->phy_lock, flags);
	} else if ((value & 7) == 1) { /* fiber_to_rgmii */
		hw->phy.phydev_type = ngbe_yt_serdes;
		if (!hw->mac.autoneg) {
			switch (speed) {
			case NGBE_LINK_SPEED_1GB_FULL:
				value = NGBE_LINK_SPEED_1GB_FULL;
				break;
			case NGBE_LINK_SPEED_100_FULL:
				value = NGBE_LINK_SPEED_100_FULL;
				break;
			default:
				value = NGBE_LINK_SPEED_1GB_FULL;
				break;
			}
			hw->phy.autoneg_advertised |= value;
			goto skip_an_fiber;
		}

		value = 0;
		if (speed & NGBE_LINK_SPEED_1GB_FULL)
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;
		if (speed & NGBE_LINK_SPEED_100_FULL)
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_100_FULL;
skip_an_fiber:
		spin_lock_irqsave(&hw->phy_lock, flags);
		ngbe_phy_read_reg_ext_yt8521s(hw, 0xA006, 0, &value);
		if (hw->phy.autoneg_advertised & NGBE_LINK_SPEED_1GB_FULL)
			value |= 0x1;
		else if (hw->phy.autoneg_advertised & NGBE_LINK_SPEED_100_FULL)
			value &= ~0x1;
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA006, 0, value);

		/* close auto sensing */
		ngbe_phy_read_reg_sds_ext_yt8521s(hw, 0xA5, 0, &value);
		value &= ~0x8000;
		ngbe_phy_write_reg_sds_ext_yt8521s(hw, 0xA5, 0, value);

		ngbe_phy_read_reg_ext_yt8521s(hw, 0xA001, 0, &value);
		value &= ~0x8000;
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA001, 0, value);
		spin_unlock_irqrestore(&hw->phy_lock, flags);

		/* RGMII_Config1 : Config rx and tx training delay */
		spin_lock_irqsave(&hw->phy_lock, flags);
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA003, 0, 0x3cf1);
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA001, 0, 0x8041);

		if (!hw->phy.yt_unidirectional) {
			/* software reset */
			if (hw->mac.autoneg) {
				ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0,
								   0x9340);
			} else {
				value = NGBE_YT8521S_PHY_RESET |
					NGBE_YT8521S_PHY_DUPLEX;
				if (speed & NGBE_LINK_SPEED_1GB_FULL)
					value |= NGBE_YT8521S_PHY_SPEED_SELECT1;
				if (speed & NGBE_LINK_SPEED_100_FULL)
					value |= NGBE_YT8521S_PHY_SPEED_SELECT0;
				ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0,
								   value);
			}
		} else {
			hw->mac.autoneg = false;
			hw->phy.autoneg_advertised = NGBE_LINK_SPEED_1GB_FULL;

			/* unidirectional mode need to config cdr */
			ngbe_phy_write_reg_sds_ext_yt8521s(hw, 0x3, 0, 0x1434);
			ngbe_phy_write_reg_sds_ext_yt8521s(hw, 0xe, 0, 0x163);

			ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, 0x8140);
			ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, 0x160);
		}
		spin_unlock_irqrestore(&hw->phy_lock, flags);
	} else if ((value & 7) == 2) {
		/* power on in UTP mode */
		spin_lock_irqsave(&hw->phy_lock, flags);
		ngbe_phy_read_reg_mdi(hw, 0x0, 0, &value);
		value &= ~0x800;
		ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);

		/* power on in Fiber mode */
		ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x0, 0, &value);
		value &= ~0x800;
		ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, value);

		ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x11, 0, &value);
		spin_unlock_irqrestore(&hw->phy_lock, flags);

		if (value & 0x400) { /* fiber up */
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;
			hw->phy.phydev_type = ngbe_yt_serdes;
		} else { /* utp up */
			hw->phy.phydev_type = ngbe_yt_utp;
			value_r4 = 0x1E0;
			value_r9 = 0x300;
			/*disable 100/10base-T Self-negotiation ability*/
			spin_lock_irqsave(&hw->phy_lock, flags);
			ngbe_phy_read_reg_mdi(hw, 0x4, 0, &value);
			value &= ~value_r4;
			ngbe_phy_write_reg_mdi(hw, 0x4, 0, value);

			/*disable 1000base-T Self-negotiation ability*/
			ngbe_phy_read_reg_mdi(hw, 0x9, 0, &value);
			value &= ~value_r9;
			ngbe_phy_write_reg_mdi(hw, 0x9, 0, value);

			value_r4 = 0x0;
			value_r9 = 0x0;

			if (speed & NGBE_LINK_SPEED_1GB_FULL) {
				hw->phy.autoneg_advertised |=
					NGBE_LINK_SPEED_1GB_FULL;
				value_r9 |= 0x200;
			}
			if (speed & NGBE_LINK_SPEED_100_FULL) {
				hw->phy.autoneg_advertised |=
					NGBE_LINK_SPEED_100_FULL;
				value_r4 |= 0x100;
			}
			if (speed & NGBE_LINK_SPEED_10_FULL) {
				hw->phy.autoneg_advertised |=
					NGBE_LINK_SPEED_10_FULL;
				value_r4 |= 0x40;
			}

			/* enable 1000base-T Self-negotiation ability */
			ngbe_phy_read_reg_mdi(hw, 0x9, 0, &value);
			value |= value_r9;
			ngbe_phy_write_reg_mdi(hw, 0x9, 0, value);

			/* enable 100/10base-T Self-negotiation ability */
			ngbe_phy_read_reg_mdi(hw, 0x4, 0, &value);
			value |= value_r4;
			ngbe_phy_write_reg_mdi(hw, 0x4, 0, value);

			/* software reset to make the above configuration take effect*/
			ngbe_phy_read_reg_mdi(hw, 0x0, 0, &value);
			value |= 0x8000;
			ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);
			spin_unlock_irqrestore(&hw->phy_lock, flags);
		}
	} else if ((value & 7) == 4) {
		hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;

		spin_lock_irqsave(&hw->phy_lock, flags);
		ngbe_phy_read_reg_ext_yt8521s(hw, 0xA003, 0, &value);
		value |= 0x8000;
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA003, 0, value);

		ngbe_phy_read_reg_ext_yt8521s(hw, 0xA004, 0, &value);
		value &= ~0xf0;
		value |= 0xb0;
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA004, 0, value);

		ngbe_phy_read_reg_ext_yt8521s(hw, 0xA001, 0, &value);
		value &= ~0x8000;
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA001, 0, value);

		/* power on phy */
		ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x0, 0, &value);
		value &= ~0x800;
		ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, value);
		spin_unlock_irqrestore(&hw->phy_lock, flags);
		hw->phy.phydev_type = ngbe_yt_serdes;
	} else if ((value & 7) == 5) { /* sgmii_to_rgmii */
		hw->phy.phydev_type = ngbe_yt_serdes;
		if (!hw->mac.autoneg) {
			switch (speed) {
			case NGBE_LINK_SPEED_1GB_FULL:
				value = NGBE_MDI_PHY_SPEED_SELECT1;
				break;
			case NGBE_LINK_SPEED_100_FULL:
				value = NGBE_MDI_PHY_SPEED_SELECT0;
				break;
			case NGBE_LINK_SPEED_10_FULL:
				value = 0;
				break;
			default:
				value = NGBE_MDI_PHY_SPEED_SELECT0 |
					NGBE_MDI_PHY_SPEED_SELECT1;
				ERROR_REPORT1(NGBE_ERROR_CAUTION,
					      "unknown speed = 0x%x.\n", speed);
				break;
			}
			/* duplex full */
			value |= NGBE_MDI_PHY_DUPLEX | 0x8000;
			spin_lock_irqsave(&hw->phy_lock, flags);
			ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, value);
			spin_unlock_irqrestore(&hw->phy_lock, flags);

			goto skip_an_sr;
		}

		value = 0;
		if (speed & NGBE_LINK_SPEED_1GB_FULL) {
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;
			value |= 0x40;
		}
		if (speed & NGBE_LINK_SPEED_100_FULL) {
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_100_FULL;
			value |= 0x2000;
		}
		if (speed & NGBE_LINK_SPEED_10_FULL) {
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_10_FULL;
			value |= 0x0;
		}

		/* duplex full */
		value |= NGBE_MDI_PHY_DUPLEX | 0x8000;
		spin_lock_irqsave(&hw->phy_lock, flags);
		ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, value);

		/* software reset to make the above configuration take effect */
		ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x0, 0, &value);
		value |= 0x9200;
		ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, value);
		spin_unlock_irqrestore(&hw->phy_lock, flags);
skip_an_sr:
		/* power on in UTP mode */
		spin_lock_irqsave(&hw->phy_lock, flags);
		ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x0, 0, &value);
		value &= ~0x800;
		ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, value);
		spin_unlock_irqrestore(&hw->phy_lock, flags);
	}
	hw->phy.ops.check_event(hw);

	return ret_val;
}

static u32 ngbe_phy_setup_link_ex_generic(struct ngbe_hw *hw, u32 speed,
					  bool autoneg_wait_to_complete)
{
	u16 value = 0;

	if (hw->ncsi_enabled)
		return NGBE_OK;

	if (hw->phy.phydev_type == ngbe_phy_jl ||
	    hw->phy.phydev_type == ngbe_phy_rtl_8211f) {
		if (hw->mac.autoneg)
			value = NGBE_MDI_PHY_RESTART_AN | NGBE_MDI_PHY_ANE |
				NGBE_MDI_PHY_RESET | NGBE_MDI_PHY_DUPLEX |
				NGBE_MDI_PHY_SPEED_SELECT1;
		else
			value = NGBE_MDI_PHY_RESET | NGBE_MDI_PHY_DUPLEX |
				NGBE_MDI_PHY_SPEED_SELECT1;

		if (hw->phy.phydev_type == ngbe_phy_jl)
			ngbe_phy_write_reg_exphy(hw, NGBE_EXPHY_MII_BMCR, 0,
						 value);
		else if (hw->phy.phydev_type == ngbe_phy_rtl_8211f)
			ngbe_phy_write_reg_exphy(hw, NGBE_EXPHY_MII_BMCR, 0xdc0,
						 value);
	} else if (hw->phy.phydev_type == ngbe_phy_yt_8011a) {
		if (hw->mac.autoneg) {
			value = NGBE_MDI_PHY_RESTART_AN | NGBE_MDI_PHY_ANE |
				NGBE_MDI_PHY_RESET | NGBE_MDI_PHY_DUPLEX |
				NGBE_MDI_PHY_SPEED_SELECT1;
			hw->phy.autoneg_advertised = speed;
		} else {
			if (speed & NGBE_LINK_SPEED_1GB_FULL)
				value = NGBE_MDI_PHY_RESET |
					NGBE_MDI_PHY_DUPLEX |
					NGBE_MDI_PHY_SPEED_SELECT1;
			else if (speed & NGBE_LINK_SPEED_100_FULL)
				value = NGBE_MDI_PHY_RESET |
					NGBE_MDI_PHY_DUPLEX |
					NGBE_MDI_PHY_SPEED_SELECT0;
			hw->phy.autoneg_advertised = speed;
		}
		ngbe_phy_write_reg_mii_yt8011a(hw, 0, value);

		if (speed & NGBE_LINK_SPEED_1GB_FULL)
			ngbe_phy_write_reg_mmd_yt8011a(hw, 0x834, 1, 0xc001);
		else if (speed & NGBE_LINK_SPEED_100_FULL)
			ngbe_phy_write_reg_mmd_yt8011a(hw, 0x834, 1, 0xc000);
	} else if (hw->phy.phydev_type == ngbe_phy_88e1111) {
		if (hw->mac.autoneg)
			value = NGBE_MDI_PHY_RESTART_AN | NGBE_MDI_PHY_ANE |
				NGBE_MDI_PHY_RESET | NGBE_MDI_PHY_DUPLEX |
				NGBE_MDI_PHY_SPEED_SELECT1;
		else
			value = NGBE_MDI_PHY_RESET | NGBE_MDI_PHY_DUPLEX |
				NGBE_MDI_PHY_SPEED_SELECT1;

		ngbe_phy_write_reg_88e1111(hw, NGBE_EXPHY_MII_BMCR, 0, value);
	}

	hw->phy.ops.check_event(hw);

	return NGBE_OK;
}

/**
 *  ngbe_tn_check_overtemp - Checks if an overtemp occurred.
 *  @hw: pointer to hardware structure
 *  Checks if the LASI temp alarm status was triggered due to overtemp
 **/
static int ngbe_phy_check_overtemp(struct ngbe_hw *hw)
{
	int status = 0;
	u32 ts_state;

	/* Check that the LASI temp alarm status was triggered */
	ts_state = rd32(hw, NGBE_TS_ALARM_ST);

	if (ts_state & NGBE_TS_ALARM_ST_DALARM)
		status = NGBE_ERR_UNDERTEMP;
	else if (ts_state & NGBE_TS_ALARM_ST_ALARM)
		status = NGBE_ERR_OVERTEMP;

	return status;
}

static int ngbe_phy_check_event(struct ngbe_hw *hw)
{
	u16 value = 0;
	struct ngbe_adapter *adapter = hw->back;

	hw->phy.ops.read_reg(hw, 0x1d, 0xa43, &value);
	adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
	if (value & BIT(4))
		adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
	else if (value & BIT(3))
		adapter->flags |= NGBE_FLAG_NEED_ANC_CHECK;

	return NGBE_OK;
}

static int ngbe_phy_check_event_m88e1512(struct ngbe_hw *hw)
{
	u16 value = 0;
	struct ngbe_adapter *adapter = hw->back;

	if (hw->phy.type == ngbe_phy_m88e1512)
		hw->phy.ops.write_reg_mdi(hw, 22, 0, 0);
	else
		hw->phy.ops.write_reg_mdi(hw, 22, 0, 1);
	hw->phy.ops.read_reg_mdi(hw, 19, 0, &value);

	if (value & NGBE_M88E1512_LSC)
		adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;

	if (value & NGBE_M88E1512_ANC)
		adapter->flags |= NGBE_FLAG_NEED_ANC_CHECK;

	return NGBE_OK;
}

static int ngbe_phy_check_event_yt8521s(struct ngbe_hw *hw)
{
	u16 value = 0;
	struct ngbe_adapter *adapter = hw->back;
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, 0, 0x0);
	hw->phy.ops.read_reg_mdi(hw, 0x13, 0, &value);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	if ((value & (NGBE_YT8521S_SDS_LINK_UP | NGBE_YT8521S_SDS_LINK_DOWN)) ||
	    (value & (NGBE_YT8521S_UTP_LINK_UP | NGBE_YT8521S_UTP_LINK_DOWN))) {
		adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
	}

	return NGBE_OK;
}

static int ngbe_phy_check_event_ex(struct ngbe_hw *hw)
{
	u16 value = 0;
	struct ngbe_adapter *adapter = hw->back;

	if (hw->phy.phydev_type == ngbe_phy_jl ||
	    hw->phy.phydev_type == ngbe_phy_rtl_8211f) {
		if (hw->phy.phydev_type == ngbe_phy_jl) {
			ngbe_phy_read_reg_exphy(hw, RTL821x_INSR, 0xa43,
						&value);
		} else if (hw->phy.phydev_type == ngbe_phy_rtl_8211f) {
			ngbe_phy_read_reg_exphy(hw, RTL821x_INSR, 0xa43,
						&value);
			ngbe_phy_read_reg_exphy(hw, 0x12, 0xde1, &value);
		}

		if (value & RTL8211F_INSR_LINK_STATUS)
			adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
		else if (value & RTL8211F_ANC_STATUS)
			adapter->flags |= NGBE_FLAG_NEED_ANC_CHECK;
	} else if (hw->phy.phydev_type == ngbe_phy_yt_8011a) {
		ngbe_phy_read_reg_mii_yt8011a(hw, 0x13, &value);

		if (value &
		    (NGBE_YT8521S_UTP_LINK_UP | NGBE_YT8521S_UTP_LINK_DOWN))
			adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
	} else if (hw->phy.phydev_type == ngbe_phy_88e1111) {
		ngbe_phy_read_reg_88e1111(hw, NGBE_88E1111_INSR, 0, &value);

		if (value & NGBE_88E1111_LINK_CHANGE_STATUS)
			adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
	}

	return NGBE_OK;
}

static int ngbe_phy_get_advertised_pause(struct ngbe_hw *hw, u8 *pause_bit)
{
	u16 value = 0;
	int status = 0;

	status = hw->phy.ops.read_reg(hw, 4, 0, &value);
	*pause_bit = (u8)((value >> 10) & 0x3);
	return status;
}

static int ngbe_phy_get_advertised_pause_m88e1512(struct ngbe_hw *hw,
						  u8 *pause_bit)
{
	u16 value = 0;
	int status = 0;

	if (hw->phy.type == ngbe_phy_m88e1512) {
		status = hw->phy.ops.write_reg_mdi(hw, 22, 0, 0);
		status = hw->phy.ops.read_reg_mdi(hw, 4, 0, &value);
		*pause_bit = (u8)((value >> 10) & 0x3);
	} else {
		status = hw->phy.ops.write_reg_mdi(hw, 22, 0, 1);
		status = hw->phy.ops.read_reg_mdi(hw, 4, 0, &value);
		*pause_bit = (u8)((value >> 7) & 0x3);
	}
	return status;
}

static int ngbe_phy_get_advertised_pause_yt8521s(struct ngbe_hw *hw,
						 u8 *pause_bit)
{
	u16 value = 0;
	int status = 0;
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x04, 0, &value);
	spin_unlock_irqrestore(&hw->phy_lock, flags);
	*pause_bit = (u8)((value >> 7) & 0x3);

	return status;
}

static int ngbe_phy_get_advertised_pause_rtl821x(struct ngbe_hw *hw,
						 u8 *pause_bit)
{
	u16 value = 0;
	int status = 0;

	if (hw->phy.phydev_type == ngbe_phy_yt_8011a)
		return 0;

	status = ngbe_phy_read_reg_exphy(hw, 0x04, 0, &value);
	*pause_bit = (u8)((value >> 7) & 0x3);

	return status;
}

static int ngbe_phy_get_lp_advertised_pause(struct ngbe_hw *hw, u8 *pause_bit)
{
	u16 value = 0;
	int status = 0;

	status = hw->phy.ops.read_reg(hw, 0x1d, 0xa43, &value);

	status = hw->phy.ops.read_reg(hw, 0x1, 0, &value);
	value = (value >> 5) & 0x1;

	/* if AN complete then check lp adv pause */
	status = hw->phy.ops.read_reg(hw, 5, 0, &value);
	*pause_bit = (u8)((value >> 10) & 0x3);
	return status;
}

static int ngbe_phy_get_lp_advertised_pause_m88e1512(struct ngbe_hw *hw,
						     u8 *pause_bit)
{
	u16 value = 0;
	int status = 0;

	if (hw->phy.type == ngbe_phy_m88e1512) {
		status = hw->phy.ops.write_reg_mdi(hw, 22, 0, 0);
		status = hw->phy.ops.read_reg_mdi(hw, 5, 0, &value);
		*pause_bit = (u8)((value >> 10) & 0x3);
	} else {
		status = hw->phy.ops.write_reg_mdi(hw, 22, 0, 1);
		status = hw->phy.ops.read_reg_mdi(hw, 5, 0, &value);
		*pause_bit = (u8)((value >> 7) & 0x3);
	}
	return status;
}

static int ngbe_phy_get_lp_advertised_pause_yt8521s(struct ngbe_hw *hw,
						    u8 *pause_bit)
{
	u16 value = 0;
	int status = 0;
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x05, 0, &value);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	*pause_bit = (u8)((value >> 7) & 0x3);
	return status;
}

static int ngbe_phy_get_lp_advertised_pause_rtl821x(struct ngbe_hw *hw,
						    u8 *pause_bit)
{
	u16 value = 0;
	int status = 0;

	if (hw->phy.phydev_type == ngbe_phy_yt_8011a)
		return 0;

	status = ngbe_phy_read_reg_exphy(hw, 0x05, 0, &value);
	*pause_bit = (u8)((value >> 7) & 0x3);

	return status;
}

static int ngbe_phy_set_pause_advertisement(struct ngbe_hw *hw, u16 pause_bit)
{
	u16 value = 0;
	int status = 0;

	status = hw->phy.ops.read_reg(hw, 4, 0, &value);
	value &= ~0xC00;
	value |= pause_bit;
	status = hw->phy.ops.write_reg(hw, 4, 0, value);
	return status;
}

static int ngbe_phy_set_pause_advertisement_m88e1512(struct ngbe_hw *hw,
						     u16 pause_bit)
{
	u16 value = 0;
	int status = 0;

	if (hw->phy.type == ngbe_phy_m88e1512) {
		status = hw->phy.ops.write_reg_mdi(hw, 22, 0, 0);
		status = hw->phy.ops.read_reg_mdi(hw, 4, 0, &value);
		value &= ~0xC00;
		value |= pause_bit;
		status = hw->phy.ops.write_reg_mdi(hw, 4, 0, value);
	} else {
		status = hw->phy.ops.write_reg_mdi(hw, 22, 0, 1);
		status = hw->phy.ops.read_reg_mdi(hw, 4, 0, &value);
		value &= ~0x180;
		value |= pause_bit;
		status = hw->phy.ops.write_reg_mdi(hw, 4, 0, value);
	}

	return status;
}

static int ngbe_phy_set_pause_advertisement_yt8521s(struct ngbe_hw *hw,
						    u16 pause_bit)
{
	u16 value = 0;
	int status = 0;
	unsigned long flags;

	spin_lock_irqsave(&hw->phy_lock, flags);
	status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x04, 0, &value);
	value &= ~0x180;
	value |= pause_bit;
	status = ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x04, 0, value);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return status;
}

static int ngbe_phy_set_pause_advertisement_rtl821x(struct ngbe_hw *hw,
						    u16 pause_bit)
{
	u16 value = 0;
	int status = 0;

	if (hw->phy.phydev_type == ngbe_phy_yt_8011a)
		return 0;

	status = ngbe_phy_read_reg_exphy(hw, 0x04, 0, &value);
	value &= ~0x180;
	value |= pause_bit;
	status = ngbe_phy_write_reg_exphy(hw, 0x04, 0, value);

	return status;
}

static int ngbe_gphy_dis_eee(struct ngbe_hw *hw)
{
	u16 val = 0;

	hw->phy.ops.write_reg(hw, 0x11, 0xa4b, 0x1110);
	hw->phy.ops.write_reg(hw, 0xd, 0x0, 0x7);
	hw->phy.ops.write_reg(hw, 0xe, 0x0, 0x003c);
	hw->phy.ops.write_reg(hw, 0xd, 0x0, 0x4007);
	hw->phy.ops.write_reg(hw, 0xe, 0x0, 0);

	/* disable 10/100M Half Duplex */
	msleep(100);
	hw->phy.ops.read_reg(hw, 4, 0, &val);
	val &= 0xff5f;
	hw->phy.ops.write_reg(hw, 0x4, 0x0, val);

	return 0;
}

static int ngbe_gphy_efuse_calibration(struct ngbe_hw *hw)
{
	struct ngbe_adapter *adapter = hw->back;
	u32 efuse[2];
	u16 val;

	ngbe_gphy_wait_mdio_access_on(hw);
	efuse[0] = adapter->gphy_efuse[0];
	efuse[1] = adapter->gphy_efuse[1];

	e_info(hw, "=1=port %d efuse[0] = %08x, efuse[1] = %08x\n",
	       hw->bus.lan_id, efuse[0], efuse[1]);

	if (!efuse[0] && !efuse[1]) {
		efuse[0] = 0xFFFFFFFF;
		efuse[1] = 0xFFFFFFFF;
	}

	/* calibration */
	efuse[0] |= 0xF0000100;
	efuse[1] |= 0xFF807FFF;
	e_info(hw, "=2=port %d efuse[0] = %08x, efuse[1] = %08x\n",
	       hw->bus.lan_id, efuse[0], efuse[1]);

	/* EODR, Efuse Output Data Register */
	hw->phy.ops.write_reg(hw, 16, 0xa46, (efuse[0] >> 0) & 0xFFFF);
	hw->phy.ops.write_reg(hw, 17, 0xa46, (efuse[0] >> 16) & 0xFFFF);
	hw->phy.ops.write_reg(hw, 18, 0xa46, (efuse[1] >> 0) & 0xFFFF);
	hw->phy.ops.write_reg(hw, 19, 0xa46, (efuse[1] >> 16) & 0xFFFF);

	hw->phy.ops.write_reg(hw, 20, 0xa46, 0x01); /* set efuse ready */
	ngbe_gphy_wait_mdio_access_on(hw);
	hw->phy.ops.write_reg(hw, 27, 0xa43, 0x8011);
	hw->phy.ops.write_reg(hw, 28, 0xa43, 0x5737);
	/* dis fall to 100m */
	hw->phy.ops.read_reg(hw, 17, 0xa44, &val);
	val &= ~0x8;
	hw->phy.ops.write_reg(hw, 17, 0xa44, val);
	ngbe_gphy_dis_eee(hw);

	return 0;
}

static int ngbe_phy_setup(struct ngbe_hw *hw)
{
	struct ngbe_adapter *adapter = hw->back;
	u16 value = 0;
	int i;

	if (test_bit(__NGBE_NO_PHY_SET, &adapter->state))
		return 0;
	ngbe_gphy_efuse_calibration(hw);
	hw->phy.ops.write_reg(hw, 20, 0xa46, 2);
	ngbe_gphy_wait_mdio_access_on(hw);

	for (i = 0; i < 100; i++) {
		hw->phy.ops.read_reg(hw, 16, 0xa42, &value);
		if ((value & 0x7) == 3)
			break;
		usec_delay(1000);
	}

	if (i == 100)
		return NGBE_ERR_PHY_TIMEOUT;

	return 0;
}

static int ngbe_phy_read_reg_internal(struct ngbe_hw *hw, int phy_addr,
				      int regnum)
{
	if (phy_addr != 0)
		return 0xffff;
	return (u16)rd32(hw, NGBE_PHY_CONFIG(regnum));
}

static int ngbe_phy_write_reg_internal(struct ngbe_hw *hw, int phy_addr,
				       int regnum, u16 value)
{
	if (phy_addr == 0)
		wr32(hw, NGBE_PHY_CONFIG(regnum), value);
	return 0;
}

static int ngbe_phy_read_reg_mdi_c22(struct ngbe_hw *hw, int phy_addr,
				     int regnum)
{
	u32 command, device_type = 0;
	int ret;

	wr32(hw, NGBE_MDIO_CLAUSE_SELECT, 0xF);
	/* setup and write the address cycle command */
	command = NGBE_MSCA_RA(regnum) | NGBE_MSCA_PA(phy_addr) |
		  NGBE_MSCA_DA(device_type);
	wr32(hw, NGBE_MSCA, command);
	command = NGBE_MSCC_CMD(NGBE_MSCA_CMD_READ) | NGBE_MSCC_BUSY |
		  NGBE_MDIO_CLK(6);
	wr32(hw, NGBE_MSCC, command);

	/* wait to complete */
	ret = po32m(hw, NGBE_MSCC, NGBE_MSCC_BUSY, ~NGBE_MSCC_BUSY,
		    NGBE_MDIO_TIMEOUT, 10);
	if (ret)
		return ret;

	return (u16)rd32(hw, NGBE_MSCC);
}

static int ngbe_phy_write_reg_mdi_c22(struct ngbe_hw *hw, int phy_addr,
				      int regnum, u16 value)
{
	u32 command, device_type = 0;
	int ret;

	wr32(hw, NGBE_MDIO_CLAUSE_SELECT, 0xF);
	/* setup and write the address cycle command */
	command = NGBE_MSCA_RA(regnum) | NGBE_MSCA_PA(phy_addr) |
		  NGBE_MSCA_DA(device_type);
	wr32(hw, NGBE_MSCA, command);
	command = value | NGBE_MSCC_CMD(NGBE_MSCA_CMD_WRITE) | NGBE_MSCC_BUSY |
		  NGBE_MDIO_CLK(6);
	wr32(hw, NGBE_MSCC, command);

	/* wait to complete */
	ret = po32m(hw, NGBE_MSCC, NGBE_MSCC_BUSY, ~NGBE_MSCC_BUSY,
		    NGBE_MDIO_TIMEOUT, 10);

	return ret;
}

static int ngbe_phy_read_reg_c22(struct ngbe_hw *hw, int phy_addr, int regnum)
{
	u16 phy_data;

	if (hw->mac_type == em_mac_type_mdi)
		phy_data = ngbe_phy_read_reg_internal(hw, phy_addr, regnum);
	else
		phy_data = ngbe_phy_read_reg_mdi_c22(hw, phy_addr, regnum);

	return phy_data;
}

static int ngbe_phy_write_reg_c22(struct ngbe_hw *hw, int phy_addr, int regnum,
				  u16 value)
{
	int ret;

	if (hw->mac_type == em_mac_type_mdi)
		ret = ngbe_phy_write_reg_internal(hw, phy_addr, regnum, value);
	else
		ret = ngbe_phy_write_reg_mdi_c22(hw, phy_addr, regnum, value);

	return ret;
}

static int ngbe_genphy_suspend(struct ngbe_hw *hw)
{
	struct ngbe_adapter *adapter = hw->back;
	u16 val;

	if (ngbe_is_lldp(hw))
		e_dev_err("Can not get lldp flags from flash\n");
	if (adapter->eth_priv_flags & NGBE_ETH_PRIV_FLAG_LLDP ||
	    hw->ncsi_enabled)
		return 0;
	hw->phy.ops.read_reg(hw, 0x0, 0x0, &val);

	return hw->phy.ops.write_reg(hw, 0x0, 0x0, val | 0x800);
}

static int ngbe_mv_suspend(struct ngbe_hw *hw)
{
	struct ngbe_adapter *adapter = hw->back;
	u16 val;

	if (adapter->eth_priv_flags & NGBE_ETH_PRIV_FLAG_LLDP ||
	    hw->ncsi_enabled)
		return 0;

	if (hw->phy.type == ngbe_phy_m88e1512) {
		hw->phy.ops.write_reg_mdi(hw, 22, 0, 0);
		hw->phy.ops.read_reg_mdi(hw, 0, 0, &val);

		return hw->phy.ops.write_reg_mdi(hw, 0x0, 0x0, val | 0x800);
	}
	hw->phy.ops.write_reg_mdi(hw, 22, 0, 1);
	hw->phy.ops.read_reg_mdi(hw, 0, 0, &val);

	return hw->phy.ops.write_reg_mdi(hw, 0x0, 0x0, val | 0x800);
}

static int ngbe_yt_suspend(struct ngbe_hw *hw)
{
	struct ngbe_adapter *adapter = hw->back;
	u16 val;
	unsigned long flags;

	if (adapter->eth_priv_flags & NGBE_ETH_PRIV_FLAG_LLDP ||
	    hw->ncsi_enabled)
		return 0;

	spin_lock_irqsave(&hw->phy_lock, flags);
	ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x0, 0, &val);
	val |= 0x800;
	ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, val);

	/* power down in UTP mode */
	ngbe_phy_read_reg_mdi(hw, 0x0, 0, &val);
	val |= 0x800;
	ngbe_phy_write_reg_mdi(hw, 0x0, 0, val);
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return 0;
}

static int ngbe_exphy_suspend(struct ngbe_hw *hw)
{
	struct ngbe_adapter *adapter = hw->back;
	u16 value;

	if (adapter->eth_priv_flags & NGBE_ETH_PRIV_FLAG_LLDP ||
	    hw->ncsi_enabled)
		return 0;

	/* power down */
	if (hw->phy.phydev_type == ngbe_phy_jl ||
	    hw->phy.phydev_type == ngbe_phy_rtl_8211f) {
		ngbe_phy_read_reg_exphy(hw, NGBE_EXPHY_MII_BMCR, 0, &value);
		value |= NGBE_EXPHY_BMCR_PDOWN;
		ngbe_phy_write_reg_exphy(hw, NGBE_EXPHY_MII_BMCR, 0, value);
	} else if (hw->phy.phydev_type == ngbe_phy_yt_8011a) {
		ngbe_phy_read_reg_mii_yt8011a(hw, 0, &value);
		value |= 0x800;
		ngbe_phy_write_reg_mii_yt8011a(hw, 0, value);
	} else if (hw->phy.phydev_type == ngbe_phy_88e1111) {
		ngbe_phy_read_reg_88e1111(hw, NGBE_EXPHY_MII_BMCR, 0, &value);
		value |= NGBE_EXPHY_BMCR_PDOWN;
		ngbe_phy_write_reg_88e1111(hw, NGBE_EXPHY_MII_BMCR, 0, value);
	}

	return 0;
}

static int ngbe_genphy_resume(struct ngbe_hw *hw)
{
	u16 val;

	hw->phy.ops.read_reg(hw, 0x0, 0x0, &val);

	return hw->phy.ops.write_reg(hw, 0x0, 0x0, val & (~0x800));
}

static int ngbe_mv_resume(struct ngbe_hw *hw)
{
	u16 val;

	if (hw->phy.type == ngbe_phy_m88e1512)
		hw->phy.ops.write_reg_mdi(hw, 22, 0, 0);
	else
		hw->phy.ops.write_reg_mdi(hw, 22, 0, 1);
	hw->phy.ops.read_reg_mdi(hw, 0, 0, &val);
	return hw->phy.ops.write_reg_mdi(hw, 0x0, 0x0, val & (~0x800));
}

static int ngbe_yt_resume(struct ngbe_hw *hw)
{
	unsigned long flags;
	u16 val;

	spin_lock_irqsave(&hw->phy_lock, flags);
	ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x0, 0, &val);
	ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, val & (~0x800));

	/* power up in UTP mode */
	ngbe_phy_read_reg_mdi(hw, 0x0, 0, &val);
	ngbe_phy_write_reg_mdi(hw, 0x0, 0, val & (~0x800));
	spin_unlock_irqrestore(&hw->phy_lock, flags);

	return 0;
}

static int ngbe_exphy_resume(struct ngbe_hw *hw)
{
	u16 value;

	/* power up */
	ngbe_phy_read_reg_exphy(hw, 0, 0, &value);
	ngbe_phy_write_reg_exphy(hw, 0, 0, value & (~0x800));
	return 0;
}

void ngbe_init_phy_ops_common(struct ngbe_hw *hw)
{
	struct ngbe_phy_info *phy = &hw->phy;

	phy->ops.identify = ngbe_phy_identify;
	phy->ops.init = ngbe_phy_init;
	phy->ops.setup_once = NULL;
	phy->ops.phy_led_ctrl = NULL;
	phy->ops.check_overtemp = NULL;

	if (hw->phy.type == ngbe_phy_m88e1512 ||
	    hw->phy.type == ngbe_phy_m88e1512_sfi ||
	    hw->phy.type == ngbe_phy_m88e1512_unknown) {
		phy->ops.reset = ngbe_phy_reset_m88e1512;
		phy->ops.read = ngbe_phy_read_reg_c22;
		phy->ops.write = ngbe_phy_write_reg_c22;
		phy->ops.read_reg = NULL;
		phy->ops.write_reg = NULL;
		phy->ops.read_reg_mdi = ngbe_phy_read_reg_mdi;
		phy->ops.write_reg_mdi = ngbe_phy_write_reg_mdi;
		phy->ops.setup_link = ngbe_phy_setup_link_m88e1512;
		phy->ops.phy_suspend = ngbe_mv_suspend;
		phy->ops.phy_resume = ngbe_mv_resume;
		phy->ops.check_event = ngbe_phy_check_event_m88e1512;
		phy->ops.get_adv_pause = ngbe_phy_get_advertised_pause_m88e1512;
		phy->ops.get_lp_adv_pause =
			ngbe_phy_get_lp_advertised_pause_m88e1512;
		phy->ops.set_adv_pause =
			ngbe_phy_set_pause_advertisement_m88e1512;
	} else if (hw->phy.type == ngbe_phy_yt8521s_sfi) {
		phy->ops.reset = ngbe_phy_reset_yt8521s;
		phy->ops.read = ngbe_phy_read_reg_c22;
		phy->ops.write = ngbe_phy_write_reg_c22;
		phy->ops.read_reg = NULL;
		phy->ops.write_reg = NULL;
		phy->ops.read_reg_mdi = ngbe_phy_read_reg_mdi;
		phy->ops.write_reg_mdi = ngbe_phy_write_reg_mdi;
		phy->ops.setup_link = ngbe_phy_setup_link_yt8521s;
		phy->ops.phy_suspend = ngbe_yt_suspend;
		phy->ops.phy_resume = ngbe_yt_resume;
		phy->ops.check_event = ngbe_phy_check_event_yt8521s;
		phy->ops.get_adv_pause = ngbe_phy_get_advertised_pause_yt8521s;
		phy->ops.get_lp_adv_pause =
			ngbe_phy_get_lp_advertised_pause_yt8521s;
		phy->ops.set_adv_pause =
			ngbe_phy_set_pause_advertisement_yt8521s;
	} else if (hw->phy.type == ngbe_phy_external) {
		phy->ops.reset = NULL;
		phy->ops.read = ngbe_phy_read_reg_c22;
		phy->ops.write = ngbe_phy_write_reg_c22;
		phy->ops.read_reg = NULL;
		phy->ops.write_reg = NULL;
		phy->ops.read_reg_mdi = ngbe_phy_read_reg_mdi;
		phy->ops.write_reg_mdi = ngbe_phy_write_reg_mdi;
		phy->ops.setup_link = ngbe_phy_setup_link_ex_generic;
		phy->ops.phy_suspend = ngbe_exphy_suspend;
		phy->ops.phy_resume = ngbe_exphy_resume;
		phy->ops.check_event = ngbe_phy_check_event_ex;
		phy->ops.get_adv_pause = ngbe_phy_get_advertised_pause_rtl821x;
		phy->ops.get_lp_adv_pause =
			ngbe_phy_get_lp_advertised_pause_rtl821x;
		phy->ops.set_adv_pause =
			ngbe_phy_set_pause_advertisement_rtl821x;
	} else {
		phy->ops.reset = ngbe_phy_reset;
		phy->ops.read = ngbe_phy_read_reg_c22;
		phy->ops.write = ngbe_phy_write_reg_c22;
		phy->ops.read_reg = ngbe_phy_read_reg;
		phy->ops.write_reg = ngbe_phy_write_reg;
		phy->ops.read_reg_mdi = NULL;
		phy->ops.write_reg_mdi = NULL;
		phy->ops.setup_link = ngbe_phy_setup_link;
		phy->ops.phy_suspend = ngbe_genphy_suspend;
		phy->ops.phy_resume = ngbe_genphy_resume;
		phy->ops.check_event = ngbe_phy_check_event;
		phy->ops.get_adv_pause = ngbe_phy_get_advertised_pause;
		phy->ops.get_lp_adv_pause = ngbe_phy_get_lp_advertised_pause;
		phy->ops.set_adv_pause = ngbe_phy_set_pause_advertisement;

		phy->ops.setup_once = ngbe_phy_setup;
		phy->ops.phy_led_ctrl = ngbe_phy_led_ctrl;
		phy->ops.check_overtemp = ngbe_phy_check_overtemp;
	}
}

/*
 * WangXun 10 Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * based on ixgbe_phy.c, Copyright(c) 1999 - 2017 Intel Corporation.
 * Contact Information:
 * Linux NICS <linux.nics@intel.com>
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 */

#include "txgbe_phy.h"
#include "txgbe_mtd.h"

/**
 * txgbe_check_reset_blocked - check status of MNG FW veto bit
 * @hw: pointer to the hardware structure
 *
 * This function checks the MMNGC.MNG_VETO bit to see if there are
 * any constraints on link from manageability.  For MAC's that don't
 * have this bit just return faluse since the link can not be blocked
 * via this method.
 **/
s32 txgbe_check_reset_blocked(struct txgbe_hw *hw)
{
	u32 mmngc;

	DEBUGFUNC("\n");

	mmngc = rd32(hw, TXGBE_MIS_ST);
	if (mmngc & TXGBE_MIS_ST_MNG_VETO) {
		ERROR_REPORT1(TXGBE_ERROR_SOFTWARE,
			      "MNG_VETO bit detected.\n");
		return true;
	}

	return false;
}


/**
 *  txgbe_get_phy_id - Get the phy type
 *  @hw: pointer to hardware structure
 *
 **/
s32 txgbe_get_phy_id(struct txgbe_hw *hw)
{
	u32 status;
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;
	u8 numport, thisport;
	DEBUGFUNC("\n");

	status = mtdHwXmdioRead(&hw->phy_dev, hw->phy.addr,
				TXGBE_MDIO_PMA_PMD_DEV_TYPE,
				TXGBE_MDIO_PHY_ID_HIGH, &phy_id_high);

	if (status == 0) {
		hw->phy.id = (u32)(phy_id_high << 16);
		status = mtdHwXmdioRead(&hw->phy_dev, hw->phy.addr,
				TXGBE_MDIO_PMA_PMD_DEV_TYPE,
				TXGBE_MDIO_PHY_ID_LOW, &phy_id_low);
		hw->phy.id |= (u32)(phy_id_low & TXGBE_PHY_REVISION_MASK);
	}

	if (status == 0) {
		status = mtdGetPhyRevision(&hw->phy_dev, hw->phy.addr,
			(MTD_DEVICE_ID *)&hw->phy.revision, &numport, &thisport);
		if (status == MTD_FAIL) {
			ERROR_REPORT1(TXGBE_ERROR_INVALID_STATE,
				 "Error in mtdGetPhyRevision()\n");
		}
	}
	return status;
}

/**
 *  txgbe_get_phy_type_from_id - Get the phy type
 *  @phy_id: PHY ID information
 *
 **/
enum txgbe_phy_type txgbe_get_phy_type_from_id(struct txgbe_hw *hw)
{
	enum txgbe_phy_type phy_type;
	u16 ext_ability = 0;

	DEBUGFUNC("\n");

	switch (hw->phy.id) {
	case TN1010_PHY_ID:
		phy_type = txgbe_phy_tn;
		break;
	case QT2022_PHY_ID:
		phy_type = txgbe_phy_qt;
		break;
	case ATH_PHY_ID:
		phy_type = txgbe_phy_nl;
		break;
	default:
		phy_type = txgbe_phy_unknown;
		break;
	}
	if (phy_type == txgbe_phy_unknown) {
		mtdHwXmdioRead(&hw->phy_dev, hw->phy.addr,
				TXGBE_MDIO_PMA_PMD_DEV_TYPE,
				TXGBE_MDIO_PHY_EXT_ABILITY, &ext_ability);

		if (ext_ability & (TXGBE_MDIO_PHY_10GBASET_ABILITY |
			TXGBE_MDIO_PHY_1000BASET_ABILITY))
			phy_type = txgbe_phy_cu_unknown;
		else
			phy_type = txgbe_phy_generic;
	}
	return phy_type;
}

/**
 *  txgbe_reset_phy - Performs a PHY reset
 *  @hw: pointer to hardware structure
 **/
s32 txgbe_reset_phy(struct txgbe_hw *hw)
{
	s32 status = 0;

	DEBUGFUNC("\n");


	if (status != 0 || hw->phy.type == txgbe_phy_none)
		goto out;

	/* Don't reset PHY if it's shut down due to overtemp. */
	if (!hw->phy.reset_if_overtemp &&
	    (TXGBE_ERR_OVERTEMP == TCALL(hw, phy.ops.check_overtemp)))
		goto out;

	/* Blocked by MNG FW so bail */
	txgbe_check_reset_blocked(hw);
	if(((hw->subsystem_device_id & TXGBE_NCSI_MASK) == TXGBE_NCSI_SUP)||
		((hw->subsystem_device_id & TXGBE_WOL_MASK) == TXGBE_WOL_SUP))
		goto out;

	status = mtdHardwareReset(&hw->phy_dev, hw->phy.addr, 1000);

out:
	return status;
}

/**
 *  txgbe_read_phy_mdi - Reads a value from a specified PHY register without
 *  the SWFW lock
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit address of PHY register to read
 *  @phy_data: Pointer to read data from PHY register
 **/
s32 txgbe_read_phy_reg_mdi(struct txgbe_hw *hw, u32 reg_addr, u32 device_type,
		       u16 *phy_data)
{
	u32 command;
	s32 status = 0;

	/* setup and write the address cycle command */
	command = TXGBE_MSCA_RA(reg_addr) |
		TXGBE_MSCA_PA(hw->phy.addr) |
		TXGBE_MSCA_DA(device_type);
	wr32(hw, TXGBE_MSCA, command);

	command = TXGBE_MSCC_CMD(TXGBE_MSCA_CMD_READ) | TXGBE_MSCC_BUSY;
	wr32(hw, TXGBE_MSCC, command);

	/* wait to complete */
	status = po32m(hw, TXGBE_MSCC,
		TXGBE_MSCC_BUSY, ~TXGBE_MSCC_BUSY,
		TXGBE_MDIO_TIMEOUT, 10);
	if (status != 0) {
		ERROR_REPORT1(TXGBE_ERROR_POLLING,
			      "PHY address command did not complete.\n");
		return TXGBE_ERR_PHY;
	}

	/* read data from MSCC */
	*phy_data = 0xFFFF & rd32(hw, TXGBE_MSCC);

	return 0;
}

/**
 *  txgbe_read_phy_reg - Reads a value from a specified PHY register
 *  using the SWFW lock - this function is needed in most cases
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit address of PHY register to read
 *  @phy_data: Pointer to read data from PHY register
 **/
s32 txgbe_read_phy_reg(struct txgbe_hw *hw, u32 reg_addr,
			       u32 device_type, u16 *phy_data)
{
	s32 status;
	u32 gssr = hw->phy.phy_semaphore_mask;

	DEBUGFUNC("\n");

	if (0 == TCALL(hw, mac.ops.acquire_swfw_sync, gssr)) {
		status = txgbe_read_phy_reg_mdi(hw, reg_addr, device_type,
						phy_data);
		TCALL(hw, mac.ops.release_swfw_sync, gssr);
	} else {
		status = TXGBE_ERR_SWFW_SYNC;
	}

	return status;
}

/**
 *  txgbe_write_phy_reg_mdi - Writes a value to specified PHY register
 *  without SWFW lock
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit PHY register to write
 *  @device_type: 5 bit device type
 *  @phy_data: Data to write to the PHY register
 **/
s32 txgbe_write_phy_reg_mdi(struct txgbe_hw *hw, u32 reg_addr,
				u32 device_type, u16 phy_data)
{
	u32 command;
	s32 status = 0;

	/* setup and write the address cycle command */
	command = TXGBE_MSCA_RA(reg_addr) |
		TXGBE_MSCA_PA(hw->phy.addr) |
		TXGBE_MSCA_DA(device_type);
	wr32(hw, TXGBE_MSCA, command);

	command = phy_data | TXGBE_MSCC_CMD(TXGBE_MSCA_CMD_WRITE) |
		  TXGBE_MSCC_BUSY;
	wr32(hw, TXGBE_MSCC, command);

	/* wait to complete */
	status = po32m(hw, TXGBE_MSCC,
		TXGBE_MSCC_BUSY, ~TXGBE_MSCC_BUSY,
		TXGBE_MDIO_TIMEOUT, 10);
	if (status != 0) {
		ERROR_REPORT1(TXGBE_ERROR_POLLING,
			      "PHY address command did not complete.\n");
		return TXGBE_ERR_PHY;
	}

	return 0;
}

/**
 *  txgbe_write_phy_reg - Writes a value to specified PHY register
 *  using SWFW lock- this function is needed in most cases
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit PHY register to write
 *  @device_type: 5 bit device type
 *  @phy_data: Data to write to the PHY register
 **/
s32 txgbe_write_phy_reg(struct txgbe_hw *hw, u32 reg_addr,
				u32 device_type, u16 phy_data)
{
	s32 status;
	u32 gssr = hw->phy.phy_semaphore_mask;

	DEBUGFUNC("\n");

	if (TCALL(hw, mac.ops.acquire_swfw_sync, gssr) == 0) {
		status = txgbe_write_phy_reg_mdi(hw, reg_addr, device_type,
						 phy_data);
		TCALL(hw, mac.ops.release_swfw_sync, gssr);
	} else {
		status = TXGBE_ERR_SWFW_SYNC;
	}

	return status;
}

MTD_STATUS txgbe_read_mdio(
						MTD_DEV * dev,
						MTD_U16 port,
						MTD_U16 mmd,
						MTD_U16 reg,
						MTD_U16 *value)
{
	struct txgbe_hw *hw = (struct txgbe_hw *)(dev->appData);

	if (hw->phy.addr != port)
		return MTD_FAIL;
	return txgbe_read_phy_reg(hw, reg, mmd, value);
}

MTD_STATUS txgbe_write_mdio(
						MTD_DEV * dev,
						MTD_U16 port,
						MTD_U16 mmd,
						MTD_U16 reg,
						MTD_U16 value)
{
	struct txgbe_hw *hw = (struct txgbe_hw *)(dev->appData);

	if (hw->phy.addr != port)
		return MTD_FAIL;

	return txgbe_write_phy_reg(hw, reg, mmd, value);
}

/*
MTD_SEM txgbe_sem_create(MTD_SEM_BEGIN_STATE state)
{
	sema_init(struct semaphore * sem,int val)
		mutex_init(mutex)
}
*/

/**
 *  txgbe_setup_phy_link - Set and restart auto-neg
 *  @hw: pointer to hardware structure
 *
 *  Restart auto-negotiation and PHY and waits for completion.
 **/
u32 txgbe_setup_phy_link(struct txgbe_hw *hw, u32 speed_set, bool autoneg_wait_to_complete)
{
	u16 speed = MTD_ADV_NONE;
	MTD_DEV_PTR devptr = &hw->phy_dev;
	u16 port = hw->phy.addr;
	int i = 0;
	MTD_BOOL linkUp = MTD_FALSE;
	u16 linkSpeed = MTD_ADV_NONE;


	UNREFERENCED_PARAMETER(speed_set);
	DEBUGFUNC("\n");

	if (hw->phy.autoneg_advertised & TXGBE_LINK_SPEED_10GB_FULL)
		speed |= MTD_SPEED_10GIG_FD;
	if (hw->phy.autoneg_advertised & TXGBE_LINK_SPEED_1GB_FULL)
		speed |= MTD_SPEED_1GIG_FD;
	if (hw->phy.autoneg_advertised & TXGBE_LINK_SPEED_100_FULL)
		speed |= MTD_SPEED_100M_FD;
	if (hw->phy.autoneg_advertised & TXGBE_LINK_SPEED_10_FULL)
		speed |= MTD_SPEED_10M_FD;
	if (!autoneg_wait_to_complete) {
		mtdGetAutonegSpeedDuplexResolution(devptr, port, &linkSpeed);
		if (linkSpeed & speed) {
			speed = linkSpeed;
			goto out;
		}
	}

	mtdEnableSpeeds(devptr, port, speed, MTD_TRUE);
	msleep(10); 
	speed = MTD_ADV_NONE;
	for (i = 0; i < 300; i++) {
		mtdIsBaseTUp(devptr, port ,&speed, &linkUp);
		if (linkUp) {
			break;
		}
		msleep(10);
	}

out:
	switch (speed) {
	case MTD_SPEED_10GIG_FD:
		return TXGBE_LINK_SPEED_10GB_FULL;
	case MTD_SPEED_1GIG_FD:
		return TXGBE_LINK_SPEED_1GB_FULL;
	case MTD_SPEED_100M_FD:
		return TXGBE_LINK_SPEED_100_FULL;
	case MTD_SPEED_10M_FD:
		return TXGBE_LINK_SPEED_10_FULL;
	default:
		return TXGBE_LINK_SPEED_UNKNOWN;
	}

}

/**
 *  txgbe_setup_phy_link_speed - Sets the auto advertised capabilities
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 **/
u32 txgbe_setup_phy_link_speed(struct txgbe_hw *hw,
				       u32 speed,
				       bool autoneg_wait_to_complete)
{

	DEBUGFUNC("\n");

	/*
	 * Clear autoneg_advertised and set new values based on input link
	 * speed.
	 */
	hw->phy.autoneg_advertised = 0;

	if (speed & TXGBE_LINK_SPEED_10GB_FULL)
		hw->phy.autoneg_advertised |= TXGBE_LINK_SPEED_10GB_FULL;

	if (speed & TXGBE_LINK_SPEED_1GB_FULL)
		hw->phy.autoneg_advertised |= TXGBE_LINK_SPEED_1GB_FULL;

	if (speed & TXGBE_LINK_SPEED_100_FULL)
		hw->phy.autoneg_advertised |= TXGBE_LINK_SPEED_100_FULL;

	if (speed & TXGBE_LINK_SPEED_10_FULL)
		hw->phy.autoneg_advertised |= TXGBE_LINK_SPEED_10_FULL;

	/* Setup link based on the new speed settings */
	return txgbe_setup_phy_link(hw, speed, autoneg_wait_to_complete);
}

/**
 *  txgbe_get_copper_link_capabilities - Determines link capabilities
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @autoneg: boolean auto-negotiation value
 *
 *  Determines the supported link capabilities by reading the PHY auto
 *  negotiation register.
 **/
s32 txgbe_get_copper_link_capabilities(struct txgbe_hw *hw,
					       u32 *speed,
					       bool *autoneg)
{
	s32 status;
	u16 speed_ability;

	DEBUGFUNC("\n");

	*speed = 0;
	*autoneg = true;

	status = mtdHwXmdioRead(&hw->phy_dev, hw->phy.addr,
				TXGBE_MDIO_PMA_PMD_DEV_TYPE,
				TXGBE_MDIO_PHY_SPEED_ABILITY, &speed_ability);

	if (status == 0) {
		if (speed_ability & TXGBE_MDIO_PHY_SPEED_10G)
			*speed |= TXGBE_LINK_SPEED_10GB_FULL;
		if (speed_ability & TXGBE_MDIO_PHY_SPEED_1G)
			*speed |= TXGBE_LINK_SPEED_1GB_FULL;
		if (speed_ability & TXGBE_MDIO_PHY_SPEED_100M)
			*speed |= TXGBE_LINK_SPEED_100_FULL;
		if (speed_ability & TXGBE_MDIO_PHY_SPEED_10M)
			*speed |= TXGBE_LINK_SPEED_10_FULL;
	}

	return status;
}


/**
 *  txgbe_get_phy_firmware_version - Gets the PHY Firmware Version
 *  @hw: pointer to hardware structure
 *  @firmware_version: pointer to the PHY Firmware Version
 **/
s32 txgbe_get_phy_firmware_version(struct txgbe_hw *hw,
					   u16 *firmware_version)
{
	s32 status;
	u8 major, minor, inc, test;

	DEBUGFUNC("\n");

	status = mtdGetFirmwareVersion(&hw->phy_dev, hw->phy.addr,
		&major, &minor, &inc, &test);
	if (status == 0)
		*firmware_version = (major << 8) | minor;
	return status;
}

/**
 *  txgbe_identify_module - Identifies module type
 *  @hw: pointer to hardware structure
 *
 *  Determines HW type and calls appropriate function.
 **/
s32 txgbe_identify_module(struct txgbe_hw *hw)
{
	s32 status = TXGBE_ERR_SFP_NOT_PRESENT;

	DEBUGFUNC("\n");

	switch (TCALL(hw, mac.ops.get_media_type)) {
	case txgbe_media_type_fiber:
		status = txgbe_identify_sfp_module(hw);
		break;

	default:
		hw->phy.sfp_type = txgbe_sfp_type_not_present;
		status = TXGBE_ERR_SFP_NOT_PRESENT;
		break;
	}

	return status;
}

/**
 *  txgbe_identify_sfp_module - Identifies SFP modules
 *  @hw: pointer to hardware structure
 *
 *  Searches for and identifies the SFP module and assigns appropriate PHY type.
 **/
s32 txgbe_identify_sfp_module(struct txgbe_hw *hw)
{
	s32 status = TXGBE_ERR_PHY_ADDR_INVALID;
	u32 vendor_oui = 0;
	enum txgbe_sfp_type stored_sfp_type = hw->phy.sfp_type;
	u8 identifier = 0;
	u8 comp_codes_1g = 0;
	u8 comp_codes_10g = 0;
	u8 oui_bytes[3] = {0, 0, 0};
	u8 cable_tech = 0;
	u8 cable_spec = 0;

	DEBUGFUNC("\n");

	if (TCALL(hw, mac.ops.get_media_type) != txgbe_media_type_fiber) {
		hw->phy.sfp_type = txgbe_sfp_type_not_present;
		status = TXGBE_ERR_SFP_NOT_PRESENT;
		goto out;
	}

	/* LAN ID is needed for I2C access */
	txgbe_init_i2c(hw);
	status = TCALL(hw, phy.ops.read_i2c_eeprom,
					     TXGBE_SFF_IDENTIFIER,
					     &identifier);

	if (status != 0)
		goto err_read_i2c_eeprom;

	if (identifier != TXGBE_SFF_IDENTIFIER_SFP) {
		hw->phy.type = txgbe_phy_sfp_unsupported;
		status = TXGBE_ERR_SFP_NOT_SUPPORTED;
	} else {
		status = TCALL(hw, phy.ops.read_i2c_eeprom,
						     TXGBE_SFF_1GBE_COMP_CODES,
						     &comp_codes_1g);

		if (status != 0)
			goto err_read_i2c_eeprom;

		status = TCALL(hw, phy.ops.read_i2c_eeprom,
						     TXGBE_SFF_10GBE_COMP_CODES,
						     &comp_codes_10g);

		if (status != 0)
			goto err_read_i2c_eeprom;
		status = TCALL(hw, phy.ops.read_i2c_eeprom,
						     TXGBE_SFF_CABLE_TECHNOLOGY,
						     &cable_tech);

		if (status != 0)
			goto err_read_i2c_eeprom;

		 /* ID Module
		  * =========
		  * 0   SFP_DA_CU
		  * 1   SFP_SR
		  * 2   SFP_LR
		  * 3   SFP_DA_CORE0
		  * 4   SFP_DA_CORE1
		  * 5   SFP_SR/LR_CORE0
		  * 6   SFP_SR/LR_CORE1
		  * 7   SFP_act_lmt_DA_CORE0
		  * 8   SFP_act_lmt_DA_CORE1
		  * 9   SFP_1g_cu_CORE0
		  * 10  SFP_1g_cu_CORE1
		  * 11  SFP_1g_sx_CORE0
		  * 12  SFP_1g_sx_CORE1
		  */
		{
			if (cable_tech & TXGBE_SFF_DA_PASSIVE_CABLE) {
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type =
						     txgbe_sfp_type_da_cu_core0;
				else
					hw->phy.sfp_type =
						     txgbe_sfp_type_da_cu_core1;
			} else if (cable_tech & TXGBE_SFF_DA_ACTIVE_CABLE) {
				TCALL(hw, phy.ops.read_i2c_eeprom,
						TXGBE_SFF_CABLE_SPEC_COMP,
						&cable_spec);
				if (cable_spec &
				    TXGBE_SFF_DA_SPEC_ACTIVE_LIMITING) {
					if (hw->bus.lan_id == 0)
						hw->phy.sfp_type =
						txgbe_sfp_type_da_act_lmt_core0;
					else
						hw->phy.sfp_type =
						txgbe_sfp_type_da_act_lmt_core1;
				} else {
					hw->phy.sfp_type =
							txgbe_sfp_type_unknown;
				}
			} else if (comp_codes_10g &
				   (TXGBE_SFF_10GBASESR_CAPABLE |
				    TXGBE_SFF_10GBASELR_CAPABLE)) {
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type =
						      txgbe_sfp_type_srlr_core0;
				else
					hw->phy.sfp_type =
						      txgbe_sfp_type_srlr_core1;
			} else if (comp_codes_1g & TXGBE_SFF_1GBASET_CAPABLE) {
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type =
						txgbe_sfp_type_1g_cu_core0;
				else
					hw->phy.sfp_type =
						txgbe_sfp_type_1g_cu_core1;
			} else if (comp_codes_1g & TXGBE_SFF_1GBASESX_CAPABLE) {
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type =
						txgbe_sfp_type_1g_sx_core0;
				else
					hw->phy.sfp_type =
						txgbe_sfp_type_1g_sx_core1;
			} else if (comp_codes_1g & TXGBE_SFF_1GBASELX_CAPABLE) {
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type =
						txgbe_sfp_type_1g_lx_core0;
				else
					hw->phy.sfp_type =
						txgbe_sfp_type_1g_lx_core1;
			} else {
				hw->phy.sfp_type = txgbe_sfp_type_unknown;
			}
		}

		if (hw->phy.sfp_type != stored_sfp_type)
			hw->phy.sfp_setup_needed = true;

		/* Determine if the SFP+ PHY is dual speed or not. */
		hw->phy.multispeed_fiber = false;
		if (((comp_codes_1g & TXGBE_SFF_1GBASESX_CAPABLE) &&
		   (comp_codes_10g & TXGBE_SFF_10GBASESR_CAPABLE)) ||
		   ((comp_codes_1g & TXGBE_SFF_1GBASELX_CAPABLE) &&
		   (comp_codes_10g & TXGBE_SFF_10GBASELR_CAPABLE)))
			hw->phy.multispeed_fiber = true;

		/* Determine PHY vendor */
		if (hw->phy.type != txgbe_phy_nl) {
			hw->phy.id = identifier;
			status = TCALL(hw, phy.ops.read_i2c_eeprom,
						    TXGBE_SFF_VENDOR_OUI_BYTE0,
						    &oui_bytes[0]);

			if (status != 0)
				goto err_read_i2c_eeprom;

			status = TCALL(hw, phy.ops.read_i2c_eeprom,
						    TXGBE_SFF_VENDOR_OUI_BYTE1,
						    &oui_bytes[1]);

			if (status != 0)
				goto err_read_i2c_eeprom;

			status = TCALL(hw, phy.ops.read_i2c_eeprom,
						    TXGBE_SFF_VENDOR_OUI_BYTE2,
						    &oui_bytes[2]);

			if (status != 0)
				goto err_read_i2c_eeprom;

			vendor_oui =
			  ((oui_bytes[0] << TXGBE_SFF_VENDOR_OUI_BYTE0_SHIFT) |
			   (oui_bytes[1] << TXGBE_SFF_VENDOR_OUI_BYTE1_SHIFT) |
			   (oui_bytes[2] << TXGBE_SFF_VENDOR_OUI_BYTE2_SHIFT));

			switch (vendor_oui) {
			case TXGBE_SFF_VENDOR_OUI_TYCO:
				if (cable_tech & TXGBE_SFF_DA_PASSIVE_CABLE)
					hw->phy.type =
						    txgbe_phy_sfp_passive_tyco;
				break;
			case TXGBE_SFF_VENDOR_OUI_FTL:
				if (cable_tech & TXGBE_SFF_DA_ACTIVE_CABLE)
					hw->phy.type = txgbe_phy_sfp_ftl_active;
				else
					hw->phy.type = txgbe_phy_sfp_ftl;
				break;
			case TXGBE_SFF_VENDOR_OUI_AVAGO:
				hw->phy.type = txgbe_phy_sfp_avago;
				break;
			case TXGBE_SFF_VENDOR_OUI_INTEL:
				hw->phy.type = txgbe_phy_sfp_intel;
				break;
			default:
				if (cable_tech & TXGBE_SFF_DA_PASSIVE_CABLE)
					hw->phy.type =
						 txgbe_phy_sfp_passive_unknown;
				else if (cable_tech & TXGBE_SFF_DA_ACTIVE_CABLE)
					hw->phy.type =
						txgbe_phy_sfp_active_unknown;
				else
					hw->phy.type = txgbe_phy_sfp_unknown;
				break;
			}
		}

		/* Allow any DA cable vendor */
		if (cable_tech & (TXGBE_SFF_DA_PASSIVE_CABLE |
		    TXGBE_SFF_DA_ACTIVE_CABLE)) {
			status = 0;
			goto out;
		}

		/* Verify supported 1G SFP modules */
		if (comp_codes_10g == 0 &&
		    !(hw->phy.sfp_type == txgbe_sfp_type_1g_cu_core1 ||
		      hw->phy.sfp_type == txgbe_sfp_type_1g_cu_core0 ||
		      hw->phy.sfp_type == txgbe_sfp_type_1g_lx_core0 ||
		      hw->phy.sfp_type == txgbe_sfp_type_1g_lx_core1 ||
		      hw->phy.sfp_type == txgbe_sfp_type_1g_sx_core0 ||
		      hw->phy.sfp_type == txgbe_sfp_type_1g_sx_core1)) {
			hw->phy.type = txgbe_phy_sfp_unsupported;
			status = TXGBE_ERR_SFP_NOT_SUPPORTED;
			goto out;
		}
	}

out:
	return status;

err_read_i2c_eeprom:
	hw->phy.sfp_type = txgbe_sfp_type_not_present;
	if (hw->phy.type != txgbe_phy_nl) {
		hw->phy.id = 0;
		hw->phy.type = txgbe_phy_unknown;
	}
	return TXGBE_ERR_SFP_NOT_PRESENT;
}

s32 txgbe_init_i2c(struct txgbe_hw *hw)
{

	wr32(hw, TXGBE_I2C_ENABLE, 0);

	wr32(hw, TXGBE_I2C_CON,
		(TXGBE_I2C_CON_MASTER_MODE |
		TXGBE_I2C_CON_SPEED(1) |
		TXGBE_I2C_CON_RESTART_EN |
		TXGBE_I2C_CON_SLAVE_DISABLE));
	/* Default addr is 0xA0 ,bit 0 is configure for read/write! */
	wr32(hw, TXGBE_I2C_TAR, TXGBE_I2C_SLAVE_ADDR);
	wr32(hw, TXGBE_I2C_SS_SCL_HCNT, 600);
	wr32(hw, TXGBE_I2C_SS_SCL_LCNT, 600);
	wr32(hw, TXGBE_I2C_RX_TL, 0); /* 1byte for rx full signal */
	wr32(hw, TXGBE_I2C_TX_TL, 4);
	wr32(hw, TXGBE_I2C_SCL_STUCK_TIMEOUT, 0xFFFFFF);
	wr32(hw, TXGBE_I2C_SDA_STUCK_TIMEOUT, 0xFFFFFF);

	wr32(hw, TXGBE_I2C_INTR_MASK, 0);
	wr32(hw, TXGBE_I2C_ENABLE, 1);
	return 0;
}

s32 txgbe_clear_i2c(struct txgbe_hw *hw)
{
	s32 status = 0;

	/* wait for completion */
	status = po32m(hw, TXGBE_I2C_STATUS,
		TXGBE_I2C_STATUS_MST_ACTIVITY, ~TXGBE_I2C_STATUS_MST_ACTIVITY,
		TXGBE_I2C_TIMEOUT, 10);
	if (status != 0)
		goto out;

	wr32(hw, TXGBE_I2C_ENABLE, 0);

out:
	return status;
}

/**
 *  txgbe_read_i2c_eeprom - Reads 8 bit EEPROM word over I2C interface
 *  @hw: pointer to hardware structure
 *  @byte_offset: EEPROM byte offset to read
 *  @eeprom_data: value read
 *
 *  Performs byte read operation to SFP module's EEPROM over I2C interface.
 **/
s32 txgbe_read_i2c_eeprom(struct txgbe_hw *hw, u8 byte_offset,
				  u8 *eeprom_data)
{
	DEBUGFUNC("\n");

	return TCALL(hw, phy.ops.read_i2c_byte, byte_offset,
					 TXGBE_I2C_EEPROM_DEV_ADDR,
					 eeprom_data);
}

/**
 *  txgbe_read_i2c_sff8472 - Reads 8 bit word over I2C interface
 *  @hw: pointer to hardware structure
 *  @byte_offset: byte offset at address 0xA2
 *  @eeprom_data: value read
 *
 *  Performs byte read operation to SFP module's SFF-8472 data over I2C
 **/
s32 txgbe_read_i2c_sff8472(struct txgbe_hw *hw, u8 byte_offset,
					  u8 *sff8472_data)
{
	return TCALL(hw, phy.ops.read_i2c_byte, byte_offset,
					 TXGBE_I2C_EEPROM_DEV_ADDR2,
					 sff8472_data);
}

/**
 *  txgbe_write_i2c_eeprom - Writes 8 bit EEPROM word over I2C interface
 *  @hw: pointer to hardware structure
 *  @byte_offset: EEPROM byte offset to write
 *  @eeprom_data: value to write
 *
 *  Performs byte write operation to SFP module's EEPROM over I2C interface.
 **/
s32 txgbe_write_i2c_eeprom(struct txgbe_hw *hw, u8 byte_offset,
				   u8 eeprom_data)
{
	DEBUGFUNC("\n");

	return TCALL(hw, phy.ops.write_i2c_byte, byte_offset,
					  TXGBE_I2C_EEPROM_DEV_ADDR,
					  eeprom_data);
}

/**
 *  txgbe_read_i2c_byte_int - Reads 8 bit word over I2C
 *  @hw: pointer to hardware structure
 *  @byte_offset: byte offset to read
 *  @data: value read
 *  @lock: true if to take and release semaphore
 *
 *  Performs byte read operation to SFP module's EEPROM over I2C interface at
 *  a specified device address.
 **/
STATIC s32 txgbe_read_i2c_byte_int(struct txgbe_hw *hw, u8 byte_offset,
					   u8 dev_addr, u8 *data, bool lock)
{
	s32 status = 0;
	u32 swfw_mask = hw->phy.phy_semaphore_mask;

	UNREFERENCED_PARAMETER(dev_addr);

	if (lock && 0 != TCALL(hw, mac.ops.acquire_swfw_sync, swfw_mask))
		return TXGBE_ERR_SWFW_SYNC;

	/* wait tx empty */
	status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
		TXGBE_I2C_INTR_STAT_TX_EMPTY, TXGBE_I2C_INTR_STAT_TX_EMPTY,
		TXGBE_I2C_TIMEOUT, 10);
	if (status != 0)
		goto out;

	/* read data */
	wr32(hw, TXGBE_I2C_DATA_CMD,
			byte_offset | TXGBE_I2C_DATA_CMD_STOP);
	wr32(hw, TXGBE_I2C_DATA_CMD, TXGBE_I2C_DATA_CMD_READ);

	/* wait for read complete */
	status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
		TXGBE_I2C_INTR_STAT_RX_FULL, TXGBE_I2C_INTR_STAT_RX_FULL,
		TXGBE_I2C_TIMEOUT, 10);
	if (status != 0)
		goto out;

	*data = 0xFF & rd32(hw, TXGBE_I2C_DATA_CMD);

out:
	if (lock)
		TCALL(hw, mac.ops.release_swfw_sync, swfw_mask);
	return status;
}

/**
 *  txgbe_switch_i2c_slave_addr - Switch I2C slave address
 *  @hw: pointer to hardware structure
 *  @dev_addr: slave addr to switch
 *
 **/
s32 txgbe_switch_i2c_slave_addr(struct txgbe_hw *hw, u8 dev_addr)
{
	wr32(hw, TXGBE_I2C_ENABLE, 0);
	wr32(hw, TXGBE_I2C_TAR, dev_addr >> 1);
	wr32(hw, TXGBE_I2C_ENABLE, 1);
	return 0;
}


/**
 *  txgbe_read_i2c_byte - Reads 8 bit word over I2C
 *  @hw: pointer to hardware structure
 *  @byte_offset: byte offset to read
 *  @data: value read
 *
 *  Performs byte read operation to SFP module's EEPROM over I2C interface at
 *  a specified device address.
 **/
s32 txgbe_read_i2c_byte(struct txgbe_hw *hw, u8 byte_offset,
				u8 dev_addr, u8 *data)
{
	txgbe_switch_i2c_slave_addr(hw, dev_addr);

#ifndef SIMULATION_DEBUG
	return txgbe_read_i2c_byte_int(hw, byte_offset, dev_addr,
					       data, true);
#else
	return txgbe_read_i2c_byte_int(hw, byte_offset, dev_addr,
					       data, false);
#endif
}



/**
 *  txgbe_write_i2c_byte_int - Writes 8 bit word over I2C
 *  @hw: pointer to hardware structure
 *  @byte_offset: byte offset to write
 *  @data: value to write
 *  @lock: true if to take and release semaphore
 *
 *  Performs byte write operation to SFP module's EEPROM over I2C interface at
 *  a specified device address.
 **/
STATIC s32 txgbe_write_i2c_byte_int(struct txgbe_hw *hw, u8 byte_offset,
					    u8 dev_addr, u8 data, bool lock)
{
	s32 status = 0;
	u32 swfw_mask = hw->phy.phy_semaphore_mask;

	UNREFERENCED_PARAMETER(dev_addr);

	if (lock && 0 != TCALL(hw, mac.ops.acquire_swfw_sync, swfw_mask))
		return TXGBE_ERR_SWFW_SYNC;

	/* wait tx empty */
	status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
		TXGBE_I2C_INTR_STAT_TX_EMPTY, TXGBE_I2C_INTR_STAT_TX_EMPTY,
		TXGBE_I2C_TIMEOUT, 10);
	if (status != 0)
		goto out;

	wr32(hw, TXGBE_I2C_DATA_CMD,
			byte_offset | TXGBE_I2C_DATA_CMD_STOP);
	wr32(hw, TXGBE_I2C_DATA_CMD,
			data | TXGBE_I2C_DATA_CMD_WRITE);

	/* wait for write complete */
	status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
		TXGBE_I2C_INTR_STAT_RX_FULL, TXGBE_I2C_INTR_STAT_RX_FULL,
		TXGBE_I2C_TIMEOUT, 10);

out:
	if (lock)
		TCALL(hw, mac.ops.release_swfw_sync, swfw_mask);

	return status;
}

/**
 *  txgbe_write_i2c_byte - Writes 8 bit word over I2C
 *  @hw: pointer to hardware structure
 *  @byte_offset: byte offset to write
 *  @data: value to write
 *
 *  Performs byte write operation to SFP module's EEPROM over I2C interface at
 *  a specified device address.
 **/
s32 txgbe_write_i2c_byte(struct txgbe_hw *hw, u8 byte_offset,
				 u8 dev_addr, u8 data)
{
	return txgbe_write_i2c_byte_int(hw, byte_offset, dev_addr,
						data, true);
}



/**
 *  txgbe_tn_check_overtemp - Checks if an overtemp occurred.
 *  @hw: pointer to hardware structure
 *
 *  Checks if the LASI temp alarm status was triggered due to overtemp
 **/
s32 txgbe_tn_check_overtemp(struct txgbe_hw *hw)
{
	s32 status = 0;
	u32 ts_state;

	DEBUGFUNC("\n");

	/* Check that the LASI temp alarm status was triggered */
	ts_state = rd32(hw, TXGBE_TS_ALARM_ST);

	if (ts_state & TXGBE_TS_ALARM_ST_DALARM)
		status = TXGBE_ERR_UNDERTEMP;
	else if (ts_state & TXGBE_TS_ALARM_ST_ALARM)
		status = TXGBE_ERR_OVERTEMP;

	return status;
}


s32 txgbe_init_external_phy(struct txgbe_hw *hw)
{
	s32 status = 0;

	MTD_DEV_PTR  devptr = &(hw->phy_dev);

	hw->phy.addr = 0;

	devptr->appData = hw;
	status = mtdLoadDriver(txgbe_read_mdio,
		txgbe_write_mdio,
		MTD_FALSE,
		NULL,
		NULL,
		NULL,
		NULL,
		hw->phy.addr,
		devptr);
	if (status != 0) {
		ERROR_REPORT1(TXGBE_ERROR_INVALID_STATE,
			      "External PHY initilization failed.\n");
		return TXGBE_ERR_PHY;
	}

	return status;
}


s32 txgbe_uninit_external_phy(struct txgbe_hw *hw)
{
	return mtdUnloadDriver(&hw->phy_dev);
}

s32 txgbe_set_phy_pause_advertisement(struct txgbe_hw *hw, u32 pause_bit)
{
	return mtdSetPauseAdvertisement(&hw->phy_dev, hw->phy.addr,
						(pause_bit>>10)&0x3, MTD_FALSE);
}

s32 txgbe_get_phy_advertised_pause(struct txgbe_hw *hw, u8 *pause_bit)
{
	u16 value;
	s32 status = 0;

	status = mtdHwXmdioRead(&hw->phy_dev, hw->phy.addr,
				TXGBE_MDIO_AUTO_NEG_DEV_TYPE,
				TXGBE_MDIO_AUTO_NEG_ADVT, &value);
	*pause_bit = (u8)((value>>10)&0x3);
	return status;

}

s32 txgbe_get_lp_advertised_pause(struct txgbe_hw *hw, u8 *pause_bit)
{
	return mtdGetLPAdvertisedPause(&hw->phy_dev, hw->phy.addr, pause_bit);
}

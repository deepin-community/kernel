// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019 - 2022 Beijing WangXun Technology Co., Ltd. */

#include "txgbe_phy.h"
#include "txgbe_mtd.h"
#include "txgbe.h"

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

	mmngc = rd32(hw, TXGBE_MIS_ST);
	if (mmngc & TXGBE_MIS_ST_MNG_VETO)
		return true;

	return false;
}

/**
 * txgbe_get_phy_id - Get the phy type
 * @hw: pointer to hardware structure
 **/
s32 txgbe_get_phy_id(struct txgbe_hw *hw)
{
	u32 status;
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;
	u8 numport, thisport;
	u32 i = 0;
	struct txgbe_adapter *adapter = hw->back;

	if (hw->mac.type == txgbe_mac_aml) {
		hw->phy.addr = 0;

		for (i = 0; i < 32; i++) {
			hw->phy.addr = i;
			status = txgbe_read_phy_reg_mdi(hw, TXGBE_MDIO_PHY_ID_HIGH,
							0, &phy_id_high);
			if (status) {
				e_info(drv, "txgbe_read_phy_reg_mdi failed 1\n");
				return status;
			}
			e_info(drv, "%d: phy_id_high 0x%x\n", i, phy_id_high);
			if ((phy_id_high & 0xFFFF) == 0x0141)
				break;
		}

		if (i == 32) {
			e_info(drv, "txgbe_read_phy_reg_mdi failed\n");
			return TXGBE_ERR_PHY;
		}

		status = txgbe_read_phy_reg_mdi(hw, TXGBE_MDIO_PHY_ID_LOW,
						0, &phy_id_low);
		if (status) {
			e_info(drv, "txgbe_read_phy_reg_mdi failed 2\n");
			return status;
		}
		hw->phy.id = (u32)(phy_id_high & 0xFFFF) << 6;
		hw->phy.id |= (u32)((phy_id_low & 0xFC00) >> 10);

		e_info(drv, "%s: phy_id 0x%x", __func__, hw->phy.id);

		return status;
	}

	status = txgbe_mtd_hw_xmdio_rd(&hw->phy_dev, hw->phy.addr,
				       TXGBE_MDIO_PMA_PMD_DEV_TYPE,
				TXGBE_MDIO_PHY_ID_HIGH, &phy_id_high);

	if (status == 0) {
		hw->phy.id = (u32)(phy_id_high << 16);
		status = txgbe_mtd_hw_xmdio_rd(&hw->phy_dev, hw->phy.addr,
					       TXGBE_MDIO_PMA_PMD_DEV_TYPE,
				TXGBE_MDIO_PHY_ID_LOW, &phy_id_low);
		hw->phy.id |= (u32)(phy_id_low & TXGBE_PHY_REVISION_MASK);
	}

	if (status == 0) {
		status = txgbe_mtd_get_phy_revision(&hw->phy_dev, hw->phy.addr,
						    (enum txgbe_mtd_dev_id *)&hw->phy.revision,
			&numport, &thisport);
		if (status == MTD_FAIL) {
			ERROR_REPORT1(TXGBE_ERROR_INVALID_STATE,
				      "Error in txgbe_mtd_get_phy_revision()\n");
		}
	}
	return status;
}

/**
 * txgbe_get_phy_type_from_id - Get the phy type
 * @phy_id: PHY ID information
 **/
enum txgbe_phy_type txgbe_get_phy_type_from_id(struct txgbe_hw *hw)
{
	enum txgbe_phy_type phy_type;
	u16 ext_ability = 0;

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
		txgbe_mtd_hw_xmdio_rd(&hw->phy_dev, hw->phy.addr,
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
 * txgbe_reset_phy - Performs a PHY reset
 * @hw: pointer to hardware structure
 **/
s32 txgbe_reset_phy(struct txgbe_hw *hw)
{
	s32 status = 0;

	if (status != 0 || hw->phy.type == txgbe_phy_none)
		goto out;

	/* Don't reset PHY if it's shut down due to overtemp. */
	if (!hw->phy.reset_if_overtemp &&
	    hw->phy.ops.check_overtemp(hw) == TXGBE_ERR_OVERTEMP)
		goto out;

	/* Blocked by MNG FW so bail */
	txgbe_check_reset_blocked(hw);
	if (((hw->subsystem_device_id & TXGBE_NCSI_MASK) == TXGBE_NCSI_SUP) ||
	    ((hw->subsystem_device_id & TXGBE_WOL_MASK) == TXGBE_WOL_SUP))
		goto out;

	status = txgbe_mtd_hw_rst(&hw->phy_dev, hw->phy.addr, 1000);

out:
	return status;
}

/**
 * txgbe_read_phy_mdi - Reads a value from a specified PHY register without
 * the SWFW lock
 * @hw: pointer to hardware structure
 * @reg_addr: 32 bit address of PHY register to read
 * @phy_data: Pointer to read data from PHY register
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

	if (hw->mac.ops.acquire_swfw_sync(hw, gssr) == 0) {
		status = txgbe_read_phy_reg_mdi(hw, reg_addr, device_type,
						phy_data);
		hw->mac.ops.release_swfw_sync(hw, gssr);
	} else {
		status = TXGBE_ERR_SWFW_SYNC;
	}

	return status;
}

/**
 * txgbe_write_phy_reg_mdi - Writes a value to specified PHY register
 * without SWFW lock
 * @hw: pointer to hardware structure
 * @reg_addr: 32 bit PHY register to write
 * @device_type: 5 bit device type
 * @phy_data: Data to write to the PHY register
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

	if (hw->mac.ops.acquire_swfw_sync(hw, gssr) == 0) {
		status = txgbe_write_phy_reg_mdi(hw, reg_addr, device_type,
						 phy_data);
		hw->mac.ops.release_swfw_sync(hw, gssr);
	} else {
		status = TXGBE_ERR_SWFW_SYNC;
	}

	return status;
}

u32 txgbe_read_mdio(struct mtd_dev *dev,
		    u16 port,
						u16 mmd,
						u16 reg,
						u16 *value)
{
	struct txgbe_hw *hw = (struct txgbe_hw *)(dev->app_data);

	if (hw->phy.addr != port)
		return MTD_FAIL;
	return txgbe_read_phy_reg(hw, reg, mmd, value);
}

u32 txgbe_write_mdio(struct mtd_dev *dev,
		     u16 port,
						u16 mmd,
						u16 reg,
						u16 value)
{
	struct txgbe_hw *hw = (struct txgbe_hw *)(dev->app_data);

	if (hw->phy.addr != port)
		return MTD_FAIL;

	return txgbe_write_phy_reg(hw, reg, mmd, value);
}

/**
 *  txgbe_setup_phy_link - Set and restart auto-neg
 *  @hw: pointer to hardware structure
 *
 *  Restart auto-negotiation and PHY and waits for completion.
 **/
u32 txgbe_setup_phy_link(struct txgbe_hw *hw, u32 speed_set, bool autoneg_wait_to_complete)
{
	u16 speed = MTD_ADV_NONE;
	struct mtd_dev *devptr = &hw->phy_dev;
	u16 port = hw->phy.addr;
	int i = 0;
	bool link_up = false;
	u16 link_speed = MTD_ADV_NONE;

	if (hw->phy.autoneg_advertised & TXGBE_LINK_SPEED_10GB_FULL)
		speed |= MTD_SPEED_10GIG_FD;
	if (hw->phy.autoneg_advertised & TXGBE_LINK_SPEED_1GB_FULL)
		speed |= MTD_SPEED_1GIG_FD;
	if (hw->phy.autoneg_advertised & TXGBE_LINK_SPEED_100_FULL)
		speed |= MTD_SPEED_100M_FD;
	if (hw->phy.autoneg_advertised & TXGBE_LINK_SPEED_10_FULL)
		speed |= MTD_SPEED_10M_FD;
	if (!autoneg_wait_to_complete) {
		txgbe_mtd_get_autoneg_res(devptr, port, &link_speed);
		if (link_speed & speed) {
			speed = link_speed;
			goto out;
		}
	}

	txgbe_mtd_enable_speeds(devptr, port, speed, true);
	usleep_range(10000, 20000);
	speed = MTD_ADV_NONE;
	for (i = 0; i < 300; i++) {
		txgbe_mtd_is_baset_up(devptr, port, &speed, &link_up);
		if (link_up)
			break;

		usleep_range(10000, 20000);
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
	/* Clear autoneg_advertised and set new values based on input link
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
	*speed = 0;
	*autoneg = true;

	status = txgbe_mtd_hw_xmdio_rd(&hw->phy_dev, hw->phy.addr,
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

	status = txgbe_mtd_get_firmver(&hw->phy_dev, hw->phy.addr,
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

	switch (hw->mac.ops.get_media_type(hw)) {
	case txgbe_media_type_fiber_qsfp:
		status = txgbe_identify_qsfp_module(hw);
		break;
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
	struct txgbe_adapter *adapter = hw->back;
	u8 identifier = 0;
	u8 comp_codes_1g = 0;
	u8 comp_codes_10g = 0;
	u8 comp_codes_25g = 0;
	u8 comp_copper_len = 0;
	u8 oui_bytes[3] = {0, 0, 0};
	u8 cable_tech = 0;
	u8 cable_spec = 0;
	u8 vendor_name[3] = {0, 0, 0};
	u16 phy_data = 0;
	u8 sff8472_rev, addr_mode, databyte;
	bool page_swap = false;
	u32 swfw_mask = hw->phy.phy_semaphore_mask;
	u32 value;
	int i;

	if (hw->mac.type == txgbe_mac_aml) {
		value = rd32(hw, TXGBE_GPIO_EXT);
		if (value & TXGBE_SFP1_MOD_ABS_LS) {
			hw->phy.sfp_type = txgbe_sfp_type_not_present;
			return TXGBE_ERR_SFP_NOT_PRESENT;
		}
	}

	if (hw->mac.ops.acquire_swfw_sync(hw, swfw_mask) != 0)
		return TXGBE_ERR_SWFW_SYNC;

	if (hw->mac.ops.get_media_type(hw) != txgbe_media_type_fiber) {
		hw->phy.sfp_type = txgbe_sfp_type_not_present;
		status = TXGBE_ERR_SFP_NOT_PRESENT;
		goto out;
	}

	/* LAN ID is needed for I2C access */
	txgbe_init_i2c(hw);
	status = hw->phy.ops.read_i2c_eeprom(hw,
					     TXGBE_SFF_IDENTIFIER,
					     &identifier);

	if (status != 0)
		goto err_read_i2c_eeprom;

	if (identifier != TXGBE_SFF_IDENTIFIER_SFP) {
		hw->phy.type = txgbe_phy_sfp_unsupported;
		status = TXGBE_ERR_SFP_NOT_SUPPORTED;
	} else {
		status = hw->phy.ops.read_i2c_eeprom(hw,
						     TXGBE_SFF_1GBE_COMP_CODES,
						     &comp_codes_1g);
		if (status != 0)
			goto err_read_i2c_eeprom;

		status = hw->phy.ops.read_i2c_eeprom(hw,
						     TXGBE_SFF_10GBE_COMP_CODES,
						     &comp_codes_10g);
		if (status != 0)
			goto err_read_i2c_eeprom;

		status = hw->phy.ops.read_i2c_eeprom(hw,
						     TXGBE_SFF_25GBE_COMP_CODES,
						     &comp_codes_25g);
		if (status != 0)
			goto err_read_i2c_eeprom;

		status = hw->phy.ops.read_i2c_eeprom(hw,
						     TXGBE_SFF_COPPER_LENGTH,
						     &comp_copper_len);
		if (status != 0)
			goto err_read_i2c_eeprom;

		status = hw->phy.ops.read_i2c_eeprom(hw,
						     TXGBE_SFF_CABLE_TECHNOLOGY,
						     &cable_tech);
		if (status != 0)
			goto err_read_i2c_eeprom;

		hw->bypass_ctle = true;
		hw->dac_sfp = false;

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

				if (hw->phy.sfp_type == txgbe_sfp_type_da_cu_core0 ||
				    hw->phy.sfp_type == txgbe_sfp_type_da_cu_core1) {
					hw->dac_sfp = true;
				}

				if (comp_copper_len == 0x1)
					hw->bypass_ctle = true;
				else
					hw->bypass_ctle = false;

				if (comp_codes_25g == TXGBE_SFF_25GBASECR_91FEC ||
				    comp_codes_25g == TXGBE_SFF_25GBASECR_74FEC ||
				    comp_codes_25g == TXGBE_SFF_25GBASECR_NOFEC) {
					hw->phy.fiber_suppport_speed =
						TXGBE_LINK_SPEED_25GB_FULL |
						TXGBE_LINK_SPEED_10GB_FULL;
				} else {
					hw->phy.fiber_suppport_speed |=
						TXGBE_LINK_SPEED_10GB_FULL;
				}
			} else if (cable_tech & TXGBE_SFF_DA_ACTIVE_CABLE) {
				hw->dac_sfp = false;
				hw->phy.ops.read_i2c_eeprom(hw,
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
					hw->phy.sfp_type = txgbe_sfp_type_unknown;
				}

				if (comp_codes_25g == TXGBE_SFF_25GAUI_C2M_AOC_BER_5 ||
				    comp_codes_25g == TXGBE_SFF_25GAUI_C2M_ACC_BER_5 ||
					comp_codes_25g == TXGBE_SFF_25GAUI_C2M_AOC_BER_12 ||
					comp_codes_25g == TXGBE_SFF_25GAUI_C2M_ACC_BER_12) {
					if (hw->bus.lan_id == 0)
						hw->phy.sfp_type =
						txgbe_sfp_type_25g_aoc_core0;
					else
						hw->phy.sfp_type =
						txgbe_sfp_type_25g_aoc_core1;
				}
			} else if (comp_codes_25g == TXGBE_SFF_25GAUI_C2M_AOC_BER_5 ||
					comp_codes_25g == TXGBE_SFF_25GAUI_C2M_ACC_BER_5 ||
					comp_codes_25g == TXGBE_SFF_25GAUI_C2M_AOC_BER_12 ||
					comp_codes_25g == TXGBE_SFF_25GAUI_C2M_ACC_BER_12) {
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type = txgbe_sfp_type_25g_aoc_core0;
				else
					hw->phy.sfp_type = txgbe_sfp_type_25g_aoc_core1;
			} else if (comp_codes_25g == TXGBE_SFF_25GBASESR_CAPABLE ||
					comp_codes_25g == TXGBE_SFF_25GBASEER_CAPABLE) {
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type = txgbe_sfp_type_25g_sr_core0;
				else
					hw->phy.sfp_type = txgbe_sfp_type_25g_sr_core1;
			} else if (comp_codes_25g == TXGBE_SFF_25GBASELR_CAPABLE) {
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type = txgbe_sfp_type_25g_lr_core0;
				else
					hw->phy.sfp_type = txgbe_sfp_type_25g_lr_core1;
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
		if (hw->mac.type == txgbe_mac_aml) {
			if ((comp_codes_25g == TXGBE_SFF_25GBASESR_CAPABLE ||
			     comp_codes_25g == TXGBE_SFF_25GBASELR_CAPABLE ||
				comp_codes_25g == TXGBE_SFF_25GBASEER_CAPABLE) &&
			   ((comp_codes_10g & TXGBE_SFF_10GBASESR_CAPABLE) ||
			   (comp_codes_10g & TXGBE_SFF_10GBASELR_CAPABLE)))
				hw->phy.multispeed_fiber = true;
		} else {
			if (((comp_codes_1g & TXGBE_SFF_1GBASESX_CAPABLE) &&
			     (comp_codes_10g & TXGBE_SFF_10GBASESR_CAPABLE)) ||
			   ((comp_codes_1g & TXGBE_SFF_1GBASELX_CAPABLE) &&
			   (comp_codes_10g & TXGBE_SFF_10GBASELR_CAPABLE)))
				hw->phy.multispeed_fiber = true;
		}
		/* Determine PHY vendor */
		if (hw->phy.type != txgbe_phy_nl) {
			hw->phy.id = identifier;
			status = hw->phy.ops.read_i2c_eeprom(hw,
						    TXGBE_SFF_VENDOR_OUI_BYTE0,
						    &oui_bytes[0]);

			if (status != 0)
				goto err_read_i2c_eeprom;

			status = hw->phy.ops.read_i2c_eeprom(hw,
						    TXGBE_SFF_VENDOR_OUI_BYTE1,
						    &oui_bytes[1]);

			if (status != 0)
				goto err_read_i2c_eeprom;

			status = hw->phy.ops.read_i2c_eeprom(hw,
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

		/* vendor name match QAX and can access sfp internal phy */
		status = hw->phy.ops.read_i2c_eeprom(hw,
						     TXGBE_SFF_CABLE_VENDOR_NAME1,
						     &vendor_name[0]);
		if (status != 0)
			goto err_read_i2c_eeprom;
		status = hw->phy.ops.read_i2c_eeprom(hw,
						     TXGBE_SFF_CABLE_VENDOR_NAME2,
						     &vendor_name[1]);
		if (status != 0)
			goto err_read_i2c_eeprom;
		status = hw->phy.ops.read_i2c_eeprom(hw,
						     TXGBE_SFF_CABLE_VENDOR_NAME3,
						     &vendor_name[2]);
		if (status != 0)
			goto err_read_i2c_eeprom;

		if (vendor_name[0] == 0x51 &&
		    vendor_name[1] == 0x41 &&
			vendor_name[2] == 0x58) {
			status = hw->phy.ops.read_i2c_sfp_phy(hw,
							    0x8008,
							    &phy_data);
			if (status == 0 || phy_data != 0xffff) {
				hw->phy.multispeed_fiber = false;
				if (hw->bus.lan_id == 0)
					hw->phy.sfp_type =
						     txgbe_sfp_type_10g_cu_core0;
				else
					hw->phy.sfp_type =
						     txgbe_sfp_type_10g_cu_core1;
			}
		}

		/* Allow any DA cable vendor */
		if (cable_tech & (TXGBE_SFF_DA_PASSIVE_CABLE |
		    TXGBE_SFF_DA_ACTIVE_CABLE)) {
			status = 0;
			goto out;
		}

		/* Verify supported 1G SFP modules */
		if (comp_codes_10g == 0 && comp_codes_25g == 0 &&
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
	/*record eeprom info*/
	status = hw->phy.ops.read_i2c_eeprom(hw,
		       TXGBE_SFF_SFF_8472_COMP,
		       &sff8472_rev);
	if (status != 0)
		goto err_read_i2c_eeprom;

	/* addressing mode is not supported */
	status = hw->phy.ops.read_i2c_eeprom(hw,
					     TXGBE_SFF_SFF_8472_SWAP,
					     &addr_mode);
	if (status != 0)
		goto err_read_i2c_eeprom;

	if (addr_mode & TXGBE_SFF_ADDRESSING_MODE) {
		e_err(drv, "Address change required to access page 0xA2,");
		e_err(drv, "but not supported. Please report the module type to the driver maintainers.\n");
		page_swap = true;
	}

	if (sff8472_rev == TXGBE_SFF_SFF_8472_UNSUP || page_swap ||
	    !(addr_mode & TXGBE_SFF_DDM_IMPLEMENTED)) {
		/* We have a SFP, but it does not support SFF-8472 */
		adapter->eeprom_type = ETH_MODULE_SFF_8079;
		adapter->eeprom_len = ETH_MODULE_SFF_8079_LEN;
	} else {
		/* We have a SFP which supports a revision of SFF-8472. */
		adapter->eeprom_type = ETH_MODULE_SFF_8472;
		adapter->eeprom_len = ETH_MODULE_SFF_8472_LEN;
	}
	for (i = 0; i < adapter->eeprom_len; i++) {
		if (i < ETH_MODULE_SFF_8079_LEN)
			status = hw->phy.ops.read_i2c_eeprom(hw, i,
				       &databyte);
		else
			status = hw->phy.ops.read_i2c_sff8472(hw, i,
				       &databyte);

		if (status != 0)
			goto err_read_i2c_eeprom;

		adapter->i2c_eeprom[i] = databyte;
	}

out:
	hw->mac.ops.release_swfw_sync(hw, swfw_mask);

	return status;

err_read_i2c_eeprom:
	hw->mac.ops.release_swfw_sync(hw, swfw_mask);

	hw->phy.sfp_type = txgbe_sfp_type_not_present;
	if (hw->phy.type != txgbe_phy_nl) {
		hw->phy.id = 0;
		hw->phy.type = txgbe_phy_unknown;
	}
	return TXGBE_ERR_SFP_NOT_PRESENT;
}

s32 txgbe_identify_qsfp_module(struct txgbe_hw *hw)
{
	s32 status = TXGBE_ERR_PHY_ADDR_INVALID;
	u8 identifier = 0, transceiver_type = 0;
	u32 swfw_mask = hw->phy.phy_semaphore_mask;
	u32 value;

	if (hw->mac.type == txgbe_mac_aml40) {
		value = rd32(hw, TXGBE_GPIO_EXT);
		if (value & TXGBE_SFP1_MOD_PRST_LS) {
			hw->phy.sfp_type = txgbe_sfp_type_not_present;
			return TXGBE_ERR_SFP_NOT_PRESENT;
		}
	}

	if (hw->mac.ops.acquire_swfw_sync(hw, swfw_mask) != 0)
		return TXGBE_ERR_SWFW_SYNC;

	if (hw->mac.ops.get_media_type(hw) != txgbe_media_type_fiber_qsfp) {
		hw->phy.sfp_type = txgbe_sfp_type_not_present;
		status = TXGBE_ERR_SFP_NOT_PRESENT;
		goto out;
	}

	/* LAN ID is needed for I2C access */
	txgbe_init_i2c(hw);
	status = hw->phy.ops.read_i2c_eeprom(hw,
					     TXGBE_SFF_IDENTIFIER,
					     &identifier);

	if (status != 0)
		goto err_read_i2c_eeprom;

	if (identifier == TXGBE_SFF_IDENTIFIER_QSFP ||
	    identifier == TXGBE_SFF_IDENTIFIER_QSFP_PLUS) {
		hw->phy.type = txgbe_phy_sfp_unknown;

		status = hw->phy.ops.read_i2c_eeprom(hw,
						     TXGBE_ETHERNET_COMP_OFFSET,
						     &transceiver_type);
		if (status != 0)
			goto err_read_i2c_eeprom;

		if (transceiver_type & TXGBE_SFF_ETHERNET_40G_CR4) {
			if (hw->bus.lan_id == 0)
				hw->phy.sfp_type = txgbe_qsfp_type_40g_cu_core0;
			else
				hw->phy.sfp_type = txgbe_qsfp_type_40g_cu_core1;
			hw->phy.fiber_suppport_speed =
						TXGBE_LINK_SPEED_40GB_FULL |
						TXGBE_LINK_SPEED_10GB_FULL;
		}

		if (transceiver_type & TXGBE_SFF_ETHERNET_40G_SR4) {
			if (hw->bus.lan_id == 0)
				hw->phy.sfp_type = txgbe_qsfp_type_40g_sr_core0;
			else
				hw->phy.sfp_type = txgbe_qsfp_type_40g_sr_core1;
		}

		if (transceiver_type & TXGBE_SFF_ETHERNET_40G_LR4) {
			if (hw->bus.lan_id == 0)
				hw->phy.sfp_type = txgbe_qsfp_type_40g_lr_core0;
			else
				hw->phy.sfp_type = txgbe_qsfp_type_40g_lr_core1;
		}

		if (transceiver_type & TXGBE_SFF_ETHERNET_40G_ACTIVE) {
			if (hw->bus.lan_id == 0)
				hw->phy.sfp_type = txgbe_qsfp_type_40g_active_core0;
			else
				hw->phy.sfp_type = txgbe_qsfp_type_40g_active_core1;
		}

	} else {
		hw->phy.type = txgbe_phy_sfp_unsupported;
		status = TXGBE_ERR_SFP_NOT_SUPPORTED;
	}
out:
	hw->mac.ops.release_swfw_sync(hw, swfw_mask);

	return status;

err_read_i2c_eeprom:
	hw->mac.ops.release_swfw_sync(hw, swfw_mask);

	hw->phy.sfp_type = txgbe_sfp_type_not_present;
	hw->phy.id = 0;
	hw->phy.type = txgbe_phy_unknown;

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

	/* ic_clk = 1/156.25MHz
	 * SCL_High_time = [(HCNT + IC_*_SPKLEN + 7) * ic_clk] + SCL_Fall_time
	 * SCL_Low_time = [(LCNT + 1) * ic_clk] - SCL_Fall_time + SCL_Rise_time
	 * set I2C Frequency to Standard Speed Mode 100KHz
	 */
	if (hw->mac.type == txgbe_mac_aml || hw->mac.type == txgbe_mac_aml40) {
		wr32(hw, TXGBE_I2C_SS_SCL_HCNT, 2000);
		wr32(hw, TXGBE_I2C_SS_SCL_LCNT, 2000);

		wr32m(hw, TXGBE_I2C_SDA_HOLD,
		      TXGBE_I2C_SDA_RX_HOLD | TXGBE_I2C_SDA_TX_HOLD, 0x640064);
	} else if (hw->mac.type == txgbe_mac_sp) {
		wr32(hw, TXGBE_I2C_SS_SCL_HCNT, 780);
		wr32(hw, TXGBE_I2C_SS_SCL_LCNT, 780);
	}

	wr32(hw, TXGBE_I2C_RX_TL, 0); /* 1byte for rx full signal */
	wr32(hw, TXGBE_I2C_TX_TL, 4);

	wr32(hw, TXGBE_I2C_SCL_STUCK_TIMEOUT, 0xFFFFFF);
	wr32(hw, TXGBE_I2C_SDA_STUCK_TIMEOUT, 0xFFFFFF);

	wr32(hw, TXGBE_I2C_INTR_MASK, 0);
	wr32(hw, TXGBE_I2C_ENABLE, 1);

	return 0;
}

static s32 txgbe_init_i2c_sfp_phy(struct txgbe_hw *hw)
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

	wr32(hw, TXGBE_I2C_RX_TL, 1); /* 2bytes for rx full signal */
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
	txgbe_init_i2c(hw);
	return hw->phy.ops.read_i2c_byte(hw, byte_offset,
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
	txgbe_init_i2c(hw);
	return hw->phy.ops.read_i2c_byte(hw, byte_offset,
					 TXGBE_I2C_EEPROM_DEV_ADDR2,
					 sff8472_data);
}

/**
 *  txgbe_read_i2c_sff8636 - Reads 8 bit word over I2C interface
 *  @hw: pointer to hardware structure
 *  @byte_offset: byte offset at address 0xA2
 *  @eeprom_data: value read
 *
 *  Performs byte read operation to SFP module's SFF-8472 data over I2C
 **/
s32 txgbe_read_i2c_sff8636(struct txgbe_hw *hw, u8 page, u8 byte_offset,
			   u8 *sff8636_data)
{
	txgbe_init_i2c(hw);
	hw->phy.ops.write_i2c_byte(hw, TXGBE_SFF_QSFP_PAGE_SELECT,
					 TXGBE_I2C_EEPROM_DEV_ADDR,
					 page);

	return hw->phy.ops.read_i2c_byte(hw, byte_offset,
					 TXGBE_I2C_EEPROM_DEV_ADDR,
					 sff8636_data);
}

/**
 *  txgbe_read_i2c_sfp_phy - Reads 16 bit word over I2C interface
 *  @hw: pointer to hardware structure
 *  @byte_offset: byte offset at address 0xAC
 *  @eeprom_data: value read
 *
 *  Performs byte read operation to Fiber to Copper SFP module
 *  internal phy over I2C
 **/
s32 txgbe_read_i2c_sfp_phy(struct txgbe_hw *hw, u16 byte_offset,
			   u16 *data)
{
	txgbe_init_i2c_sfp_phy(hw);

	return txgbe_read_i2c_word(hw, byte_offset,
					 TXGBE_I2C_EEPROM_DEV_ADDR3,
					 data);
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
	txgbe_init_i2c(hw);
	return hw->phy.ops.write_i2c_byte(hw, byte_offset,
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
static s32 txgbe_read_i2c_byte_int(struct txgbe_hw *hw, u8 byte_offset,
				   u8 __always_unused dev_addr, u8 *data, bool __always_unused lock)
{
	s32 status = 0;

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
		TXGBE_I2C_TIMEOUT, 100);
	if (status != 0)
		goto out;

	*data = 0xFF & rd32(hw, TXGBE_I2C_DATA_CMD);

out:
	return status;
}

/**
 * txgbe_read_i2c_word_int - Reads 16 bit word over I2C
 * @hw: pointer to hardware structure
 * @byte_offset: byte offset to read
 * @data: value read
 * @lock: true if to take and release semaphore
 * Performs byte read operation to SFP module's EEPROM over I2C interface at
 * a specified device address.
 **/
static s32 txgbe_read_i2c_word_int(struct txgbe_hw *hw, u16 byte_offset,
				   u8 __always_unused dev_addr,
									u16 *data,
									bool __always_unused lock)
{
	s32 status = 0;

	if (hw->phy.sfp_type == txgbe_sfp_type_1g_cu_core0 ||
	    hw->phy.sfp_type == txgbe_sfp_type_1g_cu_core1) {
		/* reg offset format 0x000yyyyy */
		byte_offset &= 0x1f;

		/* wait tx empty */
		status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
			       TXGBE_I2C_INTR_STAT_TX_EMPTY, TXGBE_I2C_INTR_STAT_TX_EMPTY,
		   TXGBE_I2C_TIMEOUT, 10);
		if (status != 0)
			goto out;

		/* write reg_offset */
		wr32(hw, TXGBE_I2C_DATA_CMD, (u8)byte_offset | TXGBE_I2C_DATA_CMD_STOP);

		usec_delay(TXGBE_I2C_TIMEOUT);
		/* wait tx empty */
		status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
			       TXGBE_I2C_INTR_STAT_TX_EMPTY, TXGBE_I2C_INTR_STAT_TX_EMPTY,
		   TXGBE_I2C_TIMEOUT, 10);
		if (status != 0)
			goto out;

		/* read data */
		wr32(hw, TXGBE_I2C_DATA_CMD, TXGBE_I2C_DATA_CMD_READ);
		wr32(hw, TXGBE_I2C_DATA_CMD, TXGBE_I2C_DATA_CMD_READ |
				TXGBE_I2C_DATA_CMD_STOP);

		/* wait for read complete */
		status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
			       TXGBE_I2C_INTR_STAT_RX_FULL, TXGBE_I2C_INTR_STAT_RX_FULL,
		   TXGBE_I2C_TIMEOUT, 10);
		if (status != 0)
			goto out;

		*data = 0xFF & rd32(hw, TXGBE_I2C_DATA_CMD);
		*data <<= 8;
		*data += 0xFF & rd32(hw, TXGBE_I2C_DATA_CMD);
	} else if ((hw->phy.sfp_type == txgbe_sfp_type_10g_cu_core0) ||
		(hw->phy.sfp_type == txgbe_sfp_type_10g_cu_core1)) {
		/* wait tx empty */
		status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
			       TXGBE_I2C_INTR_STAT_TX_EMPTY,
							TXGBE_I2C_INTR_STAT_TX_EMPTY,
							TXGBE_I2C_TIMEOUT, 10);
		if (status != 0)
			goto out;

		/* write reg_offset */
		wr32(hw, TXGBE_I2C_DATA_CMD, 0x23);
		wr32(hw, TXGBE_I2C_DATA_CMD, byte_offset >> 8);
		wr32(hw, TXGBE_I2C_DATA_CMD, (u8)byte_offset | TXGBE_I2C_DATA_CMD_STOP);

		/* wait tx empty */
		status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
			       TXGBE_I2C_INTR_STAT_TX_EMPTY,
							TXGBE_I2C_INTR_STAT_TX_EMPTY,
							TXGBE_I2C_TIMEOUT, 10);
		if (status != 0)
			goto out;

		/* delay for mcu access sfp internal phy through MDIO
		 * delay time need larger than 1ms
		 */
		mdelay(5);

		/* read data */
		wr32(hw, TXGBE_I2C_DATA_CMD, TXGBE_I2C_DATA_CMD_READ);
		wr32(hw, TXGBE_I2C_DATA_CMD, TXGBE_I2C_DATA_CMD_READ |
				TXGBE_I2C_DATA_CMD_STOP);

		/* wait for read complete */
		status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
			       TXGBE_I2C_INTR_STAT_RX_FULL, TXGBE_I2C_INTR_STAT_RX_FULL,
						TXGBE_I2C_TIMEOUT, 100);
		if (status != 0)
			goto out;

		/* fixme LSB data is the data of the duplicate MSB */
		*data = 0xFF & rd32(hw, TXGBE_I2C_DATA_CMD);
		*data <<= 8;
		*data += 0xFF & rd32(hw, TXGBE_I2C_DATA_CMD);
	}

out:
	return status;
}

/**
 * txgbe_switch_i2c_slave_addr - Switch I2C slave address
 * @hw: pointer to hardware structure
 * @dev_addr: slave addr to switch
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

	return txgbe_read_i2c_byte_int(hw, byte_offset, dev_addr,
					       data, true);
}

/**
 *	txgbe_read_i2c_word - Reads 16 bit word over I2C
 *	@hw: pointer to hardware structure
 *	@byte_offset: byte offset to read
 *	@data: value read
 *
 *	Performs byte read operation to SFP module's EEPROM over I2C interface at
 *	a specified device address.
 **/
s32 txgbe_read_i2c_word(struct txgbe_hw *hw, u16 byte_offset,
			u8 dev_addr, u16 *data)
{
	txgbe_switch_i2c_slave_addr(hw, dev_addr);

	return txgbe_read_i2c_word_int(hw, byte_offset, dev_addr,
						   data, true);
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
static s32 txgbe_write_i2c_byte_int(struct txgbe_hw *hw, u8 byte_offset,
				    u8 __always_unused dev_addr, u8 data)
{
	s32 status = 0;

	/* wait tx empty */
	status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
		       TXGBE_I2C_INTR_STAT_TX_EMPTY, TXGBE_I2C_INTR_STAT_TX_EMPTY,
		TXGBE_I2C_TIMEOUT, 10);
	if (status != 0)
		goto out;

	wr32(hw, TXGBE_I2C_DATA_CMD, byte_offset);
	wr32(hw, TXGBE_I2C_DATA_CMD,
	     data | TXGBE_I2C_DATA_CMD_WRITE);

	/* wait for write complete */
	status = po32m(hw, TXGBE_I2C_RAW_INTR_STAT,
		       TXGBE_I2C_INTR_STAT_RX_FULL, TXGBE_I2C_INTR_STAT_RX_FULL,
		TXGBE_I2C_TIMEOUT, 10);
out:
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
						data);
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

	if (hw->mac.type == txgbe_mac_aml ||
	    hw->mac.type == txgbe_mac_aml40) {
		ts_state = rd32(hw, TXGBE_AML_INTR_HIGH_STS);
		if (ts_state) {
			wr32(hw, TXGBE_AML_INTR_RAW_HI, TXGBE_AML_INTR_CL_HI);
			wr32(hw, TXGBE_AML_INTR_RAW_LO, TXGBE_AML_INTR_CL_LO);
			status = TXGBE_ERR_OVERTEMP;
		} else {
			ts_state = rd32(hw, TXGBE_AML_INTR_LOW_STS);
			if (ts_state)
				status = TXGBE_ERR_UNDERTEMP;
		}
	} else {
		/* Check that the LASI temp alarm status was triggered */
		ts_state = rd32(hw, TXGBE_TS_ALARM_ST);

		if (ts_state & TXGBE_TS_ALARM_ST_DALARM)
			status = TXGBE_ERR_UNDERTEMP;
		else if (ts_state & TXGBE_TS_ALARM_ST_ALARM)
			status = TXGBE_ERR_OVERTEMP;
	}
	return status;
}

s32 txgbe_init_external_phy(struct txgbe_hw *hw)
{
	s32 status = 0;

	struct mtd_dev *devptr = &hw->phy_dev;

	hw->phy.addr = 0;

	devptr->app_data = hw;
	status = txgbe_mtd_load_driver(txgbe_read_mdio,
				       txgbe_write_mdio,
		false,
		NULL,
		NULL,
		NULL,
		NULL,
		hw->phy.addr,
		devptr);
	if (status != 0) {
		ERROR_REPORT1(TXGBE_ERROR_INVALID_STATE,
			      "External PHY initialization failed.\n");
		return TXGBE_ERR_PHY;
	}

	return status;
}

s32 txgbe_uninit_external_phy(struct txgbe_hw *hw)
{
	return txgbe_mtd_unload_driver(&hw->phy_dev);
}

s32 txgbe_set_phy_pause_advertisement(struct txgbe_hw *hw, u32 pause_bit)
{
	return txgbe_mtd_set_pause_adver(&hw->phy_dev, hw->phy.addr,
						(pause_bit >> 10) & 0x3, false);
}

s32 txgbe_get_phy_advertised_pause(struct txgbe_hw *hw, u8 *pause_bit)
{
	u16 value;
	s32 status = 0;

	status = txgbe_mtd_hw_xmdio_rd(&hw->phy_dev, hw->phy.addr,
				       TXGBE_MDIO_AUTO_NEG_DEV_TYPE,
				TXGBE_MDIO_AUTO_NEG_ADVT, &value);
	*pause_bit = (u8)((value >> 10) & 0x3);
	return status;
}

s32 txgbe_get_lp_advertised_pause(struct txgbe_hw *hw, u8 *pause_bit)
{
	return txgbe_mtd_get_lp_adver_pause(&hw->phy_dev,
				hw->phy.addr, pause_bit);
}

s32 txgbe_external_phy_suspend(struct txgbe_hw *hw)
{
	s32 status = 0;
	u16 value = 0;

	status = txgbe_mtd_hw_xmdio_rd(&hw->phy_dev, hw->phy.addr,
				       TXGBE_MDIO_VENDOR_SPECIFIC_2_DEV_TYPE,
				TXGBE_MDIO_VENDOR_SPECIFIC_2_PORT_CTRL, &value);

	if (status)
		goto out;

	value |= TXGBE_MDIO_VENDOR_SPECIFIC_2_POWER;

	status = txgbe_mtd_xmdio_wr(&hw->phy_dev, hw->phy.addr,
				    TXGBE_MDIO_VENDOR_SPECIFIC_2_DEV_TYPE,
				TXGBE_MDIO_VENDOR_SPECIFIC_2_PORT_CTRL, value);

out:
	return status;
}

s32 txgbe_external_phy_resume(struct txgbe_hw *hw)
{
	s32 status = 0;
	u16 value = 0;

	status = txgbe_mtd_hw_xmdio_rd(&hw->phy_dev, hw->phy.addr,
				       TXGBE_MDIO_VENDOR_SPECIFIC_2_DEV_TYPE,
				TXGBE_MDIO_VENDOR_SPECIFIC_2_PORT_CTRL, &value);

	if (status)
		goto out;

	if (!(value & ~TXGBE_MDIO_VENDOR_SPECIFIC_2_POWER))
		goto out;

	value |= TXGBE_MDIO_VENDOR_SPECIFIC_2_SW_RST;
	value &= ~TXGBE_MDIO_VENDOR_SPECIFIC_2_POWER;

	status = txgbe_mtd_xmdio_wr(&hw->phy_dev, hw->phy.addr,
				    TXGBE_MDIO_VENDOR_SPECIFIC_2_DEV_TYPE,
				TXGBE_MDIO_VENDOR_SPECIFIC_2_PORT_CTRL, value);

out:
	return status;
}


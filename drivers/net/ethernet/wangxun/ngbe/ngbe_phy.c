/*
 * WangXun Gigabit PCI Express Linux driver
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
 */

#include "ngbe_phy.h"

/**
 * ngbe_check_reset_blocked - check status of MNG FW veto bit
 * @hw: pointer to the hardware structure
 *
 * This function checks the MMNGC.MNG_VETO bit to see if there are
 * any constraints on link from manageability.  For MAC's that don't
 * have this bit just return faluse since the link can not be blocked
 * via this method.
 **/
bool ngbe_check_reset_blocked(struct ngbe_hw *hw)
{
	u32 mmngc;

	DEBUGFUNC("ngbe_check_reset_blocked");

	mmngc = rd32(hw, NGBE_MIS_ST);
	if (mmngc & NGBE_MIS_ST_MNG_VETO) {
		ERROR_REPORT1(NGBE_ERROR_SOFTWARE,
			      "MNG_VETO bit detected.\n");
		return true;
	}

	return false;
}



/* For internal phy only */
s32 ngbe_phy_read_reg(	struct ngbe_hw *hw,
						u32 reg_offset,
						u32 page,
						u16 *phy_data)
{
	/* clear input */
	*phy_data = 0;

	wr32(hw, NGBE_PHY_CONFIG(NGBE_INTERNAL_PHY_PAGE_SELECT_OFFSET),
					page);

	if (reg_offset >= NGBE_INTERNAL_PHY_OFFSET_MAX) {
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
						"input reg offset %d exceed maximum 31.\n", reg_offset);
		return NGBE_ERR_INVALID_ARGUMENT;
	}

	*phy_data = 0xFFFF & rd32(hw, NGBE_PHY_CONFIG(reg_offset));

	return NGBE_OK;
}

/* For internal phy only */
s32 ngbe_phy_write_reg(	struct ngbe_hw *hw,
						u32 reg_offset,
						u32 page,
						u16 phy_data)
{

	wr32(hw, NGBE_PHY_CONFIG(NGBE_INTERNAL_PHY_PAGE_SELECT_OFFSET),
					page);

	if (reg_offset >= NGBE_INTERNAL_PHY_OFFSET_MAX) {
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
						"input reg offset %d exceed maximum 31.\n", reg_offset);
		return NGBE_ERR_INVALID_ARGUMENT;
	}
	wr32(hw, NGBE_PHY_CONFIG(reg_offset), phy_data);

	return NGBE_OK;
}

s32 ngbe_check_internal_phy_id(struct ngbe_hw *hw)
{
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;
	u16 phy_id = 0;

	DEBUGFUNC("ngbe_check_internal_phy_id");

	ngbe_phy_read_reg(hw, NGBE_MDI_PHY_ID1_OFFSET, 0, &phy_id_high);
	phy_id = phy_id_high << 6;
	ngbe_phy_read_reg(hw, NGBE_MDI_PHY_ID2_OFFSET, 0, &phy_id_low);
	phy_id |= (phy_id_low & NGBE_MDI_PHY_ID_MASK) >> 10;

	if (NGBE_INTERNAL_PHY_ID != phy_id) {
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
					"internal phy id 0x%x not supported.\n", phy_id);
		return NGBE_ERR_DEVICE_NOT_SUPPORTED;
	} else
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
s32 ngbe_phy_read_reg_mdi(	struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 *phy_data)
{
	u32 command;
	s32 status = 0;

	/* setup and write the address cycle command */
	command = NGBE_MSCA_RA(reg_addr) |
		NGBE_MSCA_PA(hw->phy.addr) |
		NGBE_MSCA_DA(device_type);
	wr32(hw, NGBE_MSCA, command);

	command = NGBE_MSCC_CMD(NGBE_MSCA_CMD_READ) |
			  NGBE_MSCC_BUSY |
			  NGBE_MDIO_CLK(6);
	wr32(hw, NGBE_MSCC, command);

	/* wait to complete */
	status = po32m(hw, NGBE_MSCC,
		NGBE_MSCC_BUSY, ~NGBE_MSCC_BUSY,
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
s32 ngbe_phy_write_reg_mdi(	struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 phy_data)
{
	u32 command;
	s32 status = 0;

	/* setup and write the address cycle command */
	command = NGBE_MSCA_RA(reg_addr) |
		NGBE_MSCA_PA(hw->phy.addr) |
		NGBE_MSCA_DA(device_type);
	wr32(hw, NGBE_MSCA, command);

	command = phy_data | NGBE_MSCC_CMD(NGBE_MSCA_CMD_WRITE) |
		  NGBE_MSCC_BUSY | NGBE_MDIO_CLK(6);
	wr32(hw, NGBE_MSCC, command);

	/* wait to complete */
	status = po32m(hw, NGBE_MSCC,
		NGBE_MSCC_BUSY, ~NGBE_MSCC_BUSY,
		NGBE_MDIO_TIMEOUT, 10);
	if (status != 0) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
			      "PHY address command did not complete.\n");
		return NGBE_ERR_PHY;
	}

	return 0;
}

s32 ngbe_phy_read_reg_ext_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 *phy_data)
{	
	s32 status = 0;
	status = ngbe_phy_write_reg_mdi(hw, 0x1e, device_type, reg_addr);
	if (!status)
		status = ngbe_phy_read_reg_mdi(hw, 0x1f, device_type, phy_data);
	return status;
}

s32 ngbe_phy_write_reg_ext_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 phy_data)
{
	s32 status = 0;
	status = ngbe_phy_write_reg_mdi(hw, 0x1e, device_type, reg_addr);
	if (!status)
		status = ngbe_phy_write_reg_mdi(hw, 0x1f, device_type, phy_data);
	return status;	
}

s32 ngbe_phy_read_reg_sds_ext_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 *phy_data)
{
	s32 status = 0;	
	status = ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x02);
	if (!status)
		status = ngbe_phy_read_reg_ext_yt8521s(hw, reg_addr, device_type, phy_data);
	ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x00);		
	return status;
}

s32 ngbe_phy_write_reg_sds_ext_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 phy_data)
{
	s32 status = 0;	
	status = ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x02);
	if (!status)
		status = ngbe_phy_write_reg_ext_yt8521s(hw, reg_addr, device_type, phy_data);
	ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x00);		
	return status;
}


s32 ngbe_phy_read_reg_sds_mii_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 *phy_data)
{
	s32 status = 0;
	status = ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x02);
	if (!status)
		status = ngbe_phy_read_reg_mdi(hw, reg_addr, device_type, phy_data);
	ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x00);
	return status;
}

s32 ngbe_phy_write_reg_sds_mii_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 phy_data)
{
	s32 status = 0;
	status = ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x02);
	if (!status)
		status = ngbe_phy_write_reg_mdi(hw, reg_addr, device_type, phy_data);
	ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, device_type, 0x00);
	return status;
}


s32 ngbe_check_mdi_phy_id(struct ngbe_hw *hw)
{
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;
	u32 phy_id = 0;
	u16 value = 0;

	DEBUGFUNC("ngbe_check_mdi_phy_id");

	if (hw->phy.type == ngbe_phy_m88e1512) {
		/* select page 0 */
		ngbe_phy_write_reg_mdi(hw, 22, 0, 0);
	}
	else {
		/* select page 1 */
		ngbe_phy_write_reg_mdi(hw, 22, 0, 1);
	}
	
	ngbe_phy_read_reg_mdi(hw, NGBE_MDI_PHY_ID1_OFFSET, 0, &phy_id_high);
	phy_id = phy_id_high << 6;
	ngbe_phy_read_reg_mdi(hw, NGBE_MDI_PHY_ID2_OFFSET, 0, &phy_id_low);
	phy_id |= (phy_id_low & NGBE_MDI_PHY_ID_MASK) >> 10;

	if (NGBE_M88E1512_PHY_ID != phy_id) {
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
					"MDI phy id 0x%x not supported.\n", phy_id);
		return NGBE_ERR_DEVICE_NOT_SUPPORTED;
	} else
		hw->phy.id = phy_id;

	if (hw->phy.type == ngbe_phy_m88e1512_unknown) {
		/* select page 18 reg 20 */
		ngbe_phy_write_reg_mdi(hw, 22, 0, 18);
		ngbe_phy_read_reg_mdi(hw, 20, 0, &value);
		if ((value & 0x7) == 0)
			/* mode select to RGMII-to-copper */
			hw->phy.type = ngbe_phy_m88e1512;
		else if ((value & 0x7) == 0x2)
			/* mode select to RGMII-to-sfi */
			hw->phy.type = ngbe_phy_m88e1512_sfi;
		else {
			ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
					"marvell 88E1512 mode %x is not supported.\n", value);
			return NGBE_ERR_DEVICE_NOT_SUPPORTED;			
		}
	}
	
	return NGBE_OK;
}

bool ngbe_validate_phy_addr(struct ngbe_hw *hw, u32 phy_addr)
{
	u16 phy_id = 0;
	bool valid = false;

	DEBUGFUNC("ngbe_validate_phy_addr");

	hw->phy.addr = phy_addr;

	ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x3, 0, &phy_id);
	if (phy_id != 0xFFFF && phy_id != 0x0)
		valid = true;

	return valid;
}

s32 ngbe_check_yt_phy_id(struct ngbe_hw *hw)
{
	u16 phy_id = 0;
	bool valid = false;
	u32 phy_addr;
	DEBUGFUNC("ngbe_check_yt_phy_id");

//	hw->phy.addr = 1;
	for (phy_addr = 0; phy_addr < 32; phy_addr++) {
		valid = ngbe_validate_phy_addr(hw, phy_addr);
		if (valid) {
			hw->phy.addr = phy_addr;
		printk("valid phy addr is 0x%x\n", phy_addr);
			break;
		}
	}
	if (!valid) {
		printk("cannnot find valid phy address.\n");
			
		return NGBE_ERR_DEVICE_NOT_SUPPORTED;
	}
	ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x3, 0, &phy_id);
	if (NGBE_YT8521S_PHY_ID != phy_id) {
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
						"MDI phy id 0x%x not supported.\n", phy_id);
		printk("phy id is 0x%x\n", phy_id);
		return NGBE_ERR_DEVICE_NOT_SUPPORTED;
	} else
		hw->phy.id = phy_id;
	return NGBE_OK;
}

s32 ngbe_check_zte_phy_id(struct ngbe_hw *hw)
{
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;
	u16 phy_id = 0;

	DEBUGFUNC("ngbe_check_zte_phy_id");

	ngbe_phy_read_reg_mdi(hw, NGBE_MDI_PHY_ID1_OFFSET, 0, &phy_id_high);
	phy_id = phy_id_high << 6;
	ngbe_phy_read_reg_mdi(hw, NGBE_MDI_PHY_ID2_OFFSET, 0, &phy_id_low);
	phy_id |= (phy_id_low & NGBE_MDI_PHY_ID_MASK) >> 10;

	if (NGBE_INTERNAL_PHY_ID != phy_id) {
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
					"MDI phy id 0x%x not supported.\n", phy_id);
		return NGBE_ERR_DEVICE_NOT_SUPPORTED;
	} else
		hw->phy.id = (u32)phy_id;

	return NGBE_OK;
}

/**
 *  ngbe_init_phy_ops - PHY/SFP specific init
 *  @hw: pointer to hardware structure
 *
 *  Initialize any function pointers that were not able to be
 *  set during init_shared_code because the PHY/SFP type was
 *  not known.  Perform the SFP init if necessary.
 *
**/
s32 ngbe_phy_init(struct ngbe_hw *hw)
{
	s32 ret_val = 0;
	u16 value = 0;
	int i;

	DEBUGFUNC("\n");

	/* set fwsw semaphore mask for phy first */
	if (!hw->phy.phy_semaphore_mask) {
		hw->phy.phy_semaphore_mask = NGBE_MNG_SWFW_SYNC_SW_PHY;
	}

	/* init phy.addr according to HW design */

	hw->phy.addr = 0;

	/* Identify the PHY or SFP module */
	ret_val = TCALL(hw, phy.ops.identify);
	if (ret_val == NGBE_ERR_SFP_NOT_SUPPORTED)
		return ret_val;

	/* enable interrupts, only link status change and an done is allowed */
	if (hw->phy.type == ngbe_phy_internal || hw->phy.type == ngbe_phy_internal_yt8521s_sfi) {
		value = NGBE_INTPHY_INT_LSC | NGBE_INTPHY_INT_ANC;
		TCALL(hw, phy.ops.write_reg, 0x12, 0xa42, value);
	} else if (hw->phy.type == ngbe_phy_m88e1512 || 
				hw->phy.type == ngbe_phy_m88e1512_sfi) {
		TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 2);
		TCALL(hw, phy.ops.read_reg_mdi, 21, 0, &value);
		value &= ~NGBE_M88E1512_RGM_TTC;
		value |= NGBE_M88E1512_RGM_RTC;
		TCALL(hw, phy.ops.write_reg_mdi, 21, 0, value);
		if (hw->phy.type == ngbe_phy_m88e1512) 
			TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 0);
		else
			TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 1);

		TCALL(hw, phy.ops.write_reg_mdi, 0, 0, NGBE_MDI_PHY_RESET);
		for (i = 0; i < 15; i++) {
			TCALL(hw, phy.ops.read_reg_mdi, 0, 0, &value);
			if (value & NGBE_MDI_PHY_RESET)
				msleep(1);
			else
				break;
		}

		if (i == 15) {
			ERROR_REPORT1(NGBE_ERROR_POLLING,
					"phy reset exceeds maximum waiting period.\n");
			return NGBE_ERR_PHY_TIMEOUT;
		}

		ret_val = TCALL(hw, phy.ops.reset);
		if (ret_val) {
			return ret_val;
		}

		/* set LED2 to interrupt output and INTn active low */
		TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 3);
		TCALL(hw, phy.ops.read_reg_mdi, 18, 0, &value);
		value |= NGBE_M88E1512_INT_EN;
		value &= ~(NGBE_M88E1512_INT_POL);
		TCALL(hw, phy.ops.write_reg_mdi, 18, 0, value);
	
		if (hw->phy.type == ngbe_phy_m88e1512_sfi) {	
			TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 1);
                	TCALL(hw, phy.ops.read_reg_mdi, 16, 0, &value);
                	value &= ~0x4;
                	TCALL(hw, phy.ops.write_reg_mdi, 16, 0, value);
		}

		/* enable link status change and AN complete interrupts */
		value = NGBE_M88E1512_INT_ANC | NGBE_M88E1512_INT_LSC;
		if (hw->phy.type == ngbe_phy_m88e1512) 
			TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 0);
		else
			TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 1);
		TCALL(hw, phy.ops.write_reg_mdi, 18, 0, value);

		TCALL(hw, phy.ops.read_reg_mdi, 0, 0, &value);
		value |= 0x800;
		TCALL(hw, phy.ops.write_reg_mdi, 0, 0, value);
	} else if (hw->phy.type == ngbe_phy_yt8521s_sfi) {
#ifndef POLL_LINK_STATUS
		/*enable yt8521s interrupt*/
		/* select sds area register */
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000, 0, 0x00);
	
		/* enable interrupt */
		value = 0x0C0C;
		TCALL(hw, phy.ops.write_reg_mdi, 0x12, 0, value);
#endif	

		/* power down in Fiber mode */
		ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x0, 0, &value);
		value |= 0x800;
		ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, value);
		/* power down in UTP mode */
		ngbe_phy_read_reg_mdi(hw, 0x0, 0, &value);
		value |= 0x800;
		ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);
	}

	return ret_val;
}


/**
 *  ngbe_identify_module - Identifies module type
 *  @hw: pointer to hardware structure
 *
 *  Determines HW type and calls appropriate function.
 **/
s32 ngbe_phy_identify(struct ngbe_hw *hw)
{
	s32 status = 0;

	DEBUGFUNC("ngbe_phy_identify");

	switch(hw->phy.type) {
		case ngbe_phy_internal:
		case ngbe_phy_internal_yt8521s_sfi:
			status = ngbe_check_internal_phy_id(hw);
			break;
		case ngbe_phy_m88e1512:
		case ngbe_phy_m88e1512_sfi:
		case ngbe_phy_m88e1512_unknown:
			status = ngbe_check_mdi_phy_id(hw);
			break;
		case ngbe_phy_zte:
			status = ngbe_check_zte_phy_id(hw);
			break;
		case ngbe_phy_yt8521s_sfi:
			status = ngbe_check_yt_phy_id(hw);
			break;
		default:
			status = NGBE_ERR_PHY_TYPE;
	}

	return status;
}

s32 ngbe_phy_reset(struct ngbe_hw *hw)
{
	s32 status = 0;

	u16 value = 0;
	int i;

	DEBUGFUNC("ngbe_phy_reset");

	/* only support internal phy */
	if (hw->phy.type != ngbe_phy_internal || hw->phy.type != ngbe_phy_internal_yt8521s_sfi)
		return NGBE_ERR_PHY_TYPE;

	/* Don't reset PHY if it's shut down due to overtemp. */
	if (!hw->phy.reset_if_overtemp &&
		(NGBE_ERR_OVERTEMP == TCALL(hw, phy.ops.check_overtemp))) {
		ERROR_REPORT1(NGBE_ERROR_CAUTION,
						"OVERTEMP! Skip PHY reset.\n");
		return NGBE_ERR_OVERTEMP;
	}

	/* Blocked by MNG FW so bail */
	if (ngbe_check_reset_blocked(hw))
		return status;

	value |= NGBE_MDI_PHY_RESET;
	status = TCALL(hw, phy.ops.write_reg, 0, 0, value);
	for (i = 0; i < NGBE_PHY_RST_WAIT_PERIOD; i++) {
		status = TCALL(hw, phy.ops.read_reg, 0, 0, &value);
		if (!(value & NGBE_MDI_PHY_RESET))
			break;
		msleep(1);
	}

	if (i == NGBE_PHY_RST_WAIT_PERIOD) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
						"PHY MODE RESET did not complete.\n");
		return NGBE_ERR_RESET_FAILED;
	}

	return status;
}

u32 ngbe_phy_setup_link(struct ngbe_hw *hw,
						u32 speed,
						bool need_restart_AN)
{
	u16 value = 0;

	DEBUGFUNC("ngbe_phy_setup_link");

	if (!hw->mac.autoneg) {
		TCALL(hw, phy.ops.reset);

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
				value = NGBE_MDI_PHY_SPEED_SELECT0 | NGBE_MDI_PHY_SPEED_SELECT1;
				ERROR_REPORT1(NGBE_ERROR_CAUTION,
						"unknown speed = 0x%x.\n", speed);
				break;
		}
		//duplex full
		value |= NGBE_MDI_PHY_DUPLEX;
		TCALL(hw, phy.ops.write_reg, 0, 0, value);

		goto skip_an;
	}

	/* disable 10/100M Half Duplex */
	TCALL(hw, phy.ops.read_reg, 4, 0, &value);
	value &= 0xFF5F;
	TCALL(hw, phy.ops.write_reg, 4, 0, value);

	/* set advertise enable according to input speed */
	if (!(speed & NGBE_LINK_SPEED_1GB_FULL)) {
		TCALL(hw, phy.ops.read_reg, 9, 0, &value);
		value &= 0xFDFF;
		TCALL(hw, phy.ops.write_reg, 9, 0, value);
	} else {
		TCALL(hw, phy.ops.read_reg, 9, 0, &value);
		value |= 0x200;
		TCALL(hw, phy.ops.write_reg, 9, 0, value);
	}

	if (!(speed & NGBE_LINK_SPEED_100_FULL)) {
		TCALL(hw, phy.ops.read_reg, 4, 0, &value);
		value &= 0xFEFF;
		TCALL(hw, phy.ops.write_reg, 4, 0, value);
	} else {
		TCALL(hw, phy.ops.read_reg, 4, 0, &value);
		value |= 0x100;
		TCALL(hw, phy.ops.write_reg, 4, 0, value);
	}

	if (!(speed & NGBE_LINK_SPEED_10_FULL)) {
		TCALL(hw, phy.ops.read_reg, 4, 0, &value);
		value &= 0xFFBF;
		TCALL(hw, phy.ops.write_reg, 4, 0, value);
	} else {
		TCALL(hw, phy.ops.read_reg, 4, 0, &value);
		value |= 0x40;
		TCALL(hw, phy.ops.write_reg, 4, 0, value);
	}

	/* restart AN and wait AN done interrupt */
	if (((hw->subsystem_device_id & NCSI_SUP_MASK) == NCSI_SUP) ||
		((hw->subsystem_device_id & OEM_MASK) == OCP_CARD)){
		if (need_restart_AN)
			value = NGBE_MDI_PHY_RESTART_AN | NGBE_MDI_PHY_ANE;
		else
			value = NGBE_MDI_PHY_ANE;
	} else {
		value = NGBE_MDI_PHY_RESTART_AN | NGBE_MDI_PHY_ANE;
	}

	TCALL(hw, phy.ops.write_reg, 0, 0, value);
skip_an:
	TCALL(hw, phy.ops.phy_led_ctrl);

	TCALL(hw, phy.ops.check_event);

	return NGBE_OK;
}

u32 ngbe_phy_led_ctrl(struct ngbe_hw *hw)
{
	u16 value = 0;
	struct ngbe_adapter *adapter = hw->back;

	if (adapter->led_conf != -1)
		value = adapter->led_conf & 0xffff;
	else
		value =0x205B;
	TCALL(hw, phy.ops.write_reg, 16, 0xd04, value);
	TCALL(hw, phy.ops.write_reg, 17, 0xd04, 0); 

	TCALL(hw, phy.ops.read_reg, 18, 0xd04, &value);
	if (adapter->led_conf != -1) {
		value &= ~0x73;
		value |= adapter->led_conf >> 16;
	} else {
		if (HIGH_LV_EFFECT_GPHY_RGMII == 1)
			value = value & 0xFFFC;
		else
			value = value & 0xFF8C;
		/*act led blinking mode set to 60ms*/
		value |= 0x2;
	}
	TCALL(hw, phy.ops.write_reg, 18, 0xd04, value);

	return 0;
}

s32 ngbe_phy_reset_m88e1512(struct ngbe_hw *hw)
{
	s32 status = 0;

	u16 value = 0;
	int i;

	DEBUGFUNC("ngbe_phy_reset_m88e1512");

	if (hw->phy.type != ngbe_phy_m88e1512 && 
		hw->phy.type != ngbe_phy_m88e1512_sfi)
		return NGBE_ERR_PHY_TYPE;

	/* Don't reset PHY if it's shut down due to overtemp. */
	if (!hw->phy.reset_if_overtemp &&
		(NGBE_ERR_OVERTEMP == TCALL(hw, phy.ops.check_overtemp))) {
		ERROR_REPORT1(NGBE_ERROR_CAUTION,
						"OVERTEMP! Skip PHY reset.\n");
		return NGBE_ERR_OVERTEMP;
	}

	/* Blocked by MNG FW so bail */
	if (ngbe_check_reset_blocked(hw))
		return status;

	/* select page 18 reg 20 */
	status = TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 18);
	
	if (hw->phy.type == ngbe_phy_m88e1512)
		/* mode select to RGMII-to-copper */
		value = 0;
	else
		/* mode select to RGMII-to-sfi */
		value = 2;
	status = TCALL(hw, phy.ops.write_reg_mdi, 20, 0, value);
	/* mode reset */
	value |= NGBE_MDI_PHY_RESET;
	status = TCALL(hw, phy.ops.write_reg_mdi, 20, 0, value);

	for (i = 0; i < NGBE_PHY_RST_WAIT_PERIOD; i++) {
		status = TCALL(hw, phy.ops.read_reg_mdi, 20, 0, &value);
		if (!(value & NGBE_MDI_PHY_RESET))
			break;
		msleep(1);
	}

	if (i == NGBE_PHY_RST_WAIT_PERIOD) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
						"M88E1512 MODE RESET did not complete.\n");
		return NGBE_ERR_RESET_FAILED;
	}

	return status;
}

s32 ngbe_phy_reset_yt8521s(struct ngbe_hw *hw)
{
	s32 status = 0;

	u16 value = 0;
	int i;

	DEBUGFUNC("ngbe_phy_reset_yt8521s");

	if (hw->phy.type != ngbe_phy_yt8521s_sfi)
		return NGBE_ERR_PHY_TYPE;

	/* Don't reset PHY if it's shut down due to overtemp. */
	if (!hw->phy.reset_if_overtemp &&
		(NGBE_ERR_OVERTEMP == TCALL(hw, phy.ops.check_overtemp))) {
		ERROR_REPORT1(NGBE_ERROR_CAUTION,
						"OVERTEMP! Skip PHY reset.\n");
		return NGBE_ERR_OVERTEMP;
	}

	/* Blocked by MNG FW so bail */
	if (ngbe_check_reset_blocked(hw))
		return status;

	
	/* check chip_mode first */
	ngbe_phy_read_reg_ext_yt8521s(hw, 0xa001, 0, &value);
	if((value & 7) != 0) {//fiber_to_rgmii
		status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0, 0, &value);
		/* sds software reset */
		value |= 0x8000;
		status = ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0, 0, value);

		for (i = 0; i < NGBE_PHY_RST_WAIT_PERIOD; i++) {
			status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0, 0, &value);
			if (!(value & 0x8000))
				break;
			msleep(1);
		}
	}
	else {//utp_to_rgmii
		status = ngbe_phy_read_reg_mdi(hw, 0, 0, &value);
		/* software reset */
		value |= 0x8000;
		status = ngbe_phy_write_reg_mdi(hw, 0, 0, value);

		for (i = 0; i < NGBE_PHY_RST_WAIT_PERIOD; i++) {
			status = ngbe_phy_read_reg_mdi(hw, 0, 0, &value);
			if (!(value & 0x8000))
				break;
			msleep(1);
		}
	}
  
	if (i == NGBE_PHY_RST_WAIT_PERIOD) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
						"YT8521S Software RESET did not complete.\n");
		return NGBE_ERR_RESET_FAILED;
	}

	return status;
}


u32 ngbe_phy_setup_link_m88e1512(	struct ngbe_hw *hw,
									u32 speed,
									bool autoneg_wait_to_complete)
{
	u16 value_r4 = 0;
	u16 value_r9 = 0;
	u16 value;
	struct ngbe_adapter *adapter = hw->back;

	DEBUGFUNC("\n");
	UNREFERENCED_PARAMETER(autoneg_wait_to_complete);

	if (adapter->led_conf == -1) {
		/* LED control */
		TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 3);
		TCALL(hw, phy.ops.read_reg_mdi, 16, 0, &value);
		value &= ~0x00FF;
		value |= (NGBE_M88E1512_LED1_CONF << 4) | NGBE_M88E1512_LED0_CONF;
		TCALL(hw, phy.ops.write_reg_mdi, 16, 0, value);
		TCALL(hw, phy.ops.read_reg_mdi, 17, 0, &value);
		value &= ~0x000F;
		if(HIGH_LV_EFFECT_88E1512_SFP == 1)
			value |= (NGBE_M88E1512_LED1_POL << 2) | NGBE_M88E1512_LED0_POL;
		TCALL(hw, phy.ops.write_reg_mdi, 17, 0, value);
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
					value = NGBE_MDI_PHY_SPEED_SELECT0 | NGBE_MDI_PHY_SPEED_SELECT1;
					ERROR_REPORT1(NGBE_ERROR_CAUTION,
							"unknown speed = 0x%x.\n", speed);
					break;
			}
			//duplex full
			value |= NGBE_MDI_PHY_DUPLEX | 0x8000;
			ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);

			goto skip_an;
		}		
		if (speed & NGBE_LINK_SPEED_1GB_FULL) {
			value_r9 |=NGBE_M88E1512_1000BASET_FULL;
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


		TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 0);
		TCALL(hw, phy.ops.read_reg_mdi, 4, 0, &value);
		value &= ~(NGBE_M88E1512_100BASET_FULL |
				   NGBE_M88E1512_100BASET_HALF |
				   NGBE_M88E1512_10BASET_FULL |
				   NGBE_M88E1512_10BASET_HALF);
		value_r4 |= value;
		TCALL(hw, phy.ops.write_reg_mdi, 4, 0, value_r4);

		TCALL(hw, phy.ops.read_reg_mdi, 9, 0, &value);
		value &= ~(NGBE_M88E1512_1000BASET_FULL |
				   NGBE_M88E1512_1000BASET_HALF);
		value_r9 |= value;
		TCALL(hw, phy.ops.write_reg_mdi, 9, 0, value_r9);
		
		value = NGBE_MDI_PHY_RESTART_AN | 
				NGBE_MDI_PHY_ANE |
				NGBE_MDI_PHY_RESET;
		TCALL(hw, phy.ops.write_reg_mdi, 0, 0, value);
	} else {
		hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;
		TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 1);
		TCALL(hw, phy.ops.read_reg_mdi, 4, 0, &value);
		value &= ~0x60;
		value |= 0x20;
		TCALL(hw, phy.ops.write_reg_mdi, 4, 0, value);

		value = NGBE_MDI_PHY_RESTART_AN | 
				NGBE_MDI_PHY_ANE |
				NGBE_MDI_PHY_RESET;
		TCALL(hw, phy.ops.write_reg_mdi, 0, 0, value);
	}

skip_an:
	TCALL(hw, phy.ops.read_reg_mdi, 0, 0, &value);
	value &= ~0x800;
	TCALL(hw, phy.ops.write_reg_mdi, 0, 0, value);

	TCALL(hw, phy.ops.check_event);


	return NGBE_OK;
}

u32 ngbe_phy_setup_link_yt8521s(	struct ngbe_hw *hw,
									u32 speed,
									bool autoneg_wait_to_complete)
{
	s32 ret_val = 0;
	u16 value;
	u16 value_r4 = 0;
	u16 value_r9 = 0;

	DEBUGFUNC("\n");
	UNREFERENCED_PARAMETER(autoneg_wait_to_complete);
	UNREFERENCED_PARAMETER(speed);

	hw->phy.autoneg_advertised = 0;

	/* check chip_mode first */
	ngbe_phy_read_reg_ext_yt8521s(hw, 0xA001, 0, &value);
	if((value & 7) == 0) {//utp_to_rgmii
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
					value = NGBE_MDI_PHY_SPEED_SELECT0 | NGBE_MDI_PHY_SPEED_SELECT1;
					ERROR_REPORT1(NGBE_ERROR_CAUTION,
							"unknown speed = 0x%x.\n", speed);
					break;
			}
			//duplex full
			value |= NGBE_MDI_PHY_DUPLEX | 0x8000;
			ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);

			goto skip_an;
		}
		
		value_r4 = 0x1E0;
		value_r9 = 0x300;
		/*disable 100/10base-T Self-negotiation ability*/
		ngbe_phy_read_reg_mdi(hw, 0x4, 0, &value);
		value &=~value_r4;
		ngbe_phy_write_reg_mdi(hw, 0x4, 0, value);

		/*disable 1000base-T Self-negotiation ability*/
		ngbe_phy_read_reg_mdi(hw, 0x9, 0, &value);
		value &=~value_r9;
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
		value |=value_r9;
		ngbe_phy_write_reg_mdi(hw, 0x9, 0, value);
		
		/* enable 100/10base-T Self-negotiation ability */
		ngbe_phy_read_reg_mdi(hw, 0x4, 0, &value);
		value |=value_r4;
		ngbe_phy_write_reg_mdi(hw, 0x4, 0, value);
		
		/* software reset to make the above configuration take effect*/
		ngbe_phy_read_reg_mdi(hw, 0x0, 0, &value);
		value |= 0x9200;
		ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);
skip_an:
		/* power on in UTP mode */
		ngbe_phy_read_reg_mdi(hw, 0x0, 0, &value);
		value &= ~0x800;
		ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);

	} else if ((value & 7) == 1){//fiber_to_rgmii
		hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;

		/* Pkg Cfg_0 */
		//	ngbe_phy_write_reg_sds_ext_yt8521s(hw, 0x1A0, 0, 0xE8D0);
		//	ngbe_phy_write_reg_sds_ext_yt8521s(hw, 0x1A0, 0, 0xA8D0);
		
		/* RGMII_Config1 : Config rx and tx training delay */
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA003, 0, 0x3cf1);
		ngbe_phy_write_reg_ext_yt8521s(hw, 0xA001, 0, 0x8041);

		/* software reset */
		ngbe_phy_write_reg_sds_ext_yt8521s(hw, 0x0, 0,0x9140);

		/* power on phy */
		ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x0, 0, &value);
		value &= ~0x800;
		ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, value);
		
	} else if ((value & 7) == 2){
		/* power on in UTP mode */
		ngbe_phy_read_reg_mdi(hw, 0x0, 0, &value);
		value &= ~0x800;
		ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);
		/* power on in Fiber mode */
		ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x0, 0, &value);
		value &= ~0x800;
		ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x0, 0, value);

		ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x11, 0, &value);
		if (value & 0x400){ /* fiber up */
			hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;
		} else { /* utp up */
			value_r4 = 0x1E0;
			value_r9 = 0x300;
			/*disable 100/10base-T Self-negotiation ability*/
			ngbe_phy_read_reg_mdi(hw, 0x4, 0, &value);
			value &=~value_r4;
			ngbe_phy_write_reg_mdi(hw, 0x4, 0, value);

			/*disable 1000base-T Self-negotiation ability*/
			ngbe_phy_read_reg_mdi(hw, 0x9, 0, &value);
			value &=~value_r9;
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
			value |=value_r9;
			ngbe_phy_write_reg_mdi(hw, 0x9, 0, value);
			
			/* enable 100/10base-T Self-negotiation ability */
			ngbe_phy_read_reg_mdi(hw, 0x4, 0, &value);
			value |=value_r4;
			ngbe_phy_write_reg_mdi(hw, 0x4, 0, value);
			
			/* software reset to make the above configuration take effect*/
			ngbe_phy_read_reg_mdi(hw, 0x0, 0, &value);
			value |= 0x8000;
			ngbe_phy_write_reg_mdi(hw, 0x0, 0, value);			
		}
	} else if ((value & 7) == 4) {	
		hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;
		
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
	}

	TCALL(hw, phy.ops.check_event);

	return ret_val;
}

s32 ngbe_phy_reset_zte(struct ngbe_hw *hw)
{
	s32 status = 0;
    u16 value = 0;
	int i;

	DEBUGFUNC("ngbe_phy_reset_zte");

	if (hw->phy.type != ngbe_phy_zte)
		return NGBE_ERR_PHY_TYPE;

	/* Don't reset PHY if it's shut down due to overtemp. */
	if (!hw->phy.reset_if_overtemp &&
		(NGBE_ERR_OVERTEMP == TCALL(hw, phy.ops.check_overtemp))) {
		ERROR_REPORT1(NGBE_ERROR_CAUTION,
						"OVERTEMP! Skip PHY reset.\n");
		return NGBE_ERR_OVERTEMP;
	}

	/* Blocked by MNG FW so bail */
	if (ngbe_check_reset_blocked(hw))
		return status;

    /*zte phy*/
    /*set control register[0x0] to reset mode*/
    value = 1;
    /* mode reset */
	value |= NGBE_MDI_PHY_RESET;
	status = TCALL(hw, phy.ops.write_reg_mdi, 0, 0, value);

	for (i = 0; i < NGBE_PHY_RST_WAIT_PERIOD; i++) {
		status = TCALL(hw, phy.ops.read_reg_mdi, 0, 0, &value);
		if (!(value & NGBE_MDI_PHY_RESET))
			break;
		msleep(1);
	}

	if (i == NGBE_PHY_RST_WAIT_PERIOD) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
						"ZTE MODE RESET did not complete.\n");
		return NGBE_ERR_RESET_FAILED;
	}

	return status;
}

u32 ngbe_phy_setup_link_zte(struct ngbe_hw *hw,
							u32 speed,
							bool autoneg_wait_to_complete)
{
	u16 ngbe_phy_ccr = 0;

	DEBUGFUNC("\n");
	UNREFERENCED_PARAMETER(autoneg_wait_to_complete);
	/*
	 * Clear autoneg_advertised and set new values based on input link
	 * speed.
	 */
	hw->phy.autoneg_advertised = 0;
	TCALL(hw, phy.ops.read_reg_mdi, 0, 0, &ngbe_phy_ccr);
	
	if (speed & NGBE_LINK_SPEED_1GB_FULL) {
		hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_1GB_FULL;
		ngbe_phy_ccr |= NGBE_MDI_PHY_SPEED_SELECT1;/*bit 6*/
	}
	else if (speed & NGBE_LINK_SPEED_100_FULL) {
		hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_100_FULL;
		ngbe_phy_ccr |= NGBE_MDI_PHY_SPEED_SELECT0;/*bit 13*/
	}
	else if (speed & NGBE_LINK_SPEED_10_FULL)
		hw->phy.autoneg_advertised |= NGBE_LINK_SPEED_10_FULL;
	else
		return NGBE_LINK_SPEED_UNKNOWN;

	ngbe_phy_ccr |= NGBE_MDI_PHY_DUPLEX;/*restart autonegotiation*/
	TCALL(hw, phy.ops.write_reg_mdi, 0, 0, ngbe_phy_ccr);

	return speed;
}

/**
 *  ngbe_tn_check_overtemp - Checks if an overtemp occurred.
 *  @hw: pointer to hardware structure
 *
 *  Checks if the LASI temp alarm status was triggered due to overtemp
 **/
s32 ngbe_phy_check_overtemp(struct ngbe_hw *hw)
{
	s32 status = 0;
	u32 ts_state;

	DEBUGFUNC("ngbe_phy_check_overtemp");

	/* Check that the LASI temp alarm status was triggered */
	ts_state = rd32(hw, NGBE_TS_ALARM_ST);

	if (ts_state & NGBE_TS_ALARM_ST_DALARM)
		status = NGBE_ERR_UNDERTEMP;
	else if (ts_state & NGBE_TS_ALARM_ST_ALARM)
		status = NGBE_ERR_OVERTEMP;

	return status;
}

s32 ngbe_phy_check_event(struct ngbe_hw *hw)
{
	u16 value = 0;
	struct ngbe_adapter *adapter = hw->back;

	TCALL(hw, phy.ops.read_reg, 0x1d, 0xa43, &value);
	adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
	if (value & 0x10) {
		adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
	} else if (value & 0x08) {
		adapter->flags |= NGBE_FLAG_NEED_ANC_CHECK;
	}

	return NGBE_OK;
}

s32 ngbe_phy_check_event_m88e1512(struct ngbe_hw *hw)
{
	u16 value = 0;
	struct ngbe_adapter *adapter = hw->back;

	if (hw->phy.type == ngbe_phy_m88e1512)
		TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 0);
	else
		TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 1);
	TCALL(hw, phy.ops.read_reg_mdi, 19, 0, &value);

	if (value & NGBE_M88E1512_LSC) {
		adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
	}

	if (value & NGBE_M88E1512_ANC) {
		adapter->flags |= NGBE_FLAG_NEED_ANC_CHECK;
	}

	return NGBE_OK;
}

s32 ngbe_phy_check_event_yt8521s(struct ngbe_hw *hw)
{
	u16 value = 0;
	struct ngbe_adapter *adapter = hw->back;

	ngbe_phy_write_reg_ext_yt8521s(hw, 0xa000,0,0x0);
	TCALL(hw, phy.ops.read_reg_mdi, 0x13, 0, &value);

	if ((value & (NGBE_YT8521S_SDS_LINK_UP | NGBE_YT8521S_SDS_LINK_DOWN)) ||
		(value & (NGBE_YT8521S_UTP_LINK_UP | NGBE_YT8521S_UTP_LINK_DOWN))) {
		adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
	}

	return NGBE_OK;
}


s32 ngbe_phy_get_advertised_pause(struct ngbe_hw *hw, u8 *pause_bit)
{
	u16 value;
	s32 status = 0;

	status = TCALL(hw, phy.ops.read_reg, 4, 0, &value);
	*pause_bit = (u8)((value >> 10) & 0x3);
	return status;
}

s32 ngbe_phy_get_advertised_pause_m88e1512(struct ngbe_hw *hw, u8 *pause_bit)
{
	u16 value;
	s32 status = 0;

	if (hw->phy.type == ngbe_phy_m88e1512) {
		status = TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 0);
		status = TCALL(hw, phy.ops.read_reg_mdi, 4, 0, &value);
		*pause_bit = (u8)((value >> 10) & 0x3);
	} else {
		status = TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 1);
		status = TCALL(hw, phy.ops.read_reg_mdi, 4, 0, &value);
		*pause_bit = (u8)((value >> 7) & 0x3);
	}
	return status;
}

s32 ngbe_phy_get_advertised_pause_yt8521s(struct ngbe_hw *hw, u8 *pause_bit)
{
	u16 value;
	s32 status = 0;

	status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x04, 0, &value);
	*pause_bit = (u8)((value >> 7) & 0x3);
	return status;
}

s32 ngbe_phy_get_lp_advertised_pause(struct ngbe_hw *hw, u8 *pause_bit)
{
	u16 value;
	s32 status = 0;

	status = TCALL(hw, phy.ops.read_reg, 0x1d, 0xa43, &value);

	status = TCALL(hw, phy.ops.read_reg, 0x1, 0, &value);
	value = (value >> 5) & 0x1;

    /* if AN complete then check lp adv pause */
	status = TCALL(hw, phy.ops.read_reg, 5, 0, &value);
	*pause_bit = (u8)((value >> 10) & 0x3);
	return status;
}

s32 ngbe_phy_get_lp_advertised_pause_m88e1512(struct ngbe_hw *hw,
												u8 *pause_bit)
{
	u16 value;
	s32 status = 0;

	if (hw->phy.type == ngbe_phy_m88e1512) {
		status = TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 0);
		status = TCALL(hw, phy.ops.read_reg_mdi, 5, 0, &value);
		*pause_bit = (u8)((value >> 10) & 0x3);
	} else {
		status = TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 1);
		status = TCALL(hw, phy.ops.read_reg_mdi, 5, 0, &value);
		*pause_bit = (u8)((value >> 7) & 0x3);	
	}
	return status;
}

s32 ngbe_phy_get_lp_advertised_pause_yt8521s(struct ngbe_hw *hw,
												u8 *pause_bit)
{
	u16 value;
	s32 status = 0;
	
	status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x05, 0, &value);
	*pause_bit = (u8)((value >> 7) & 0x3);
	return status;

}

s32 ngbe_phy_set_pause_advertisement(struct ngbe_hw *hw, u16 pause_bit)
{
	u16 value;
	s32 status = 0;

	status = TCALL(hw, phy.ops.read_reg, 4, 0, &value);
	value &= ~0xC00;
	value |= pause_bit;
	status = TCALL(hw, phy.ops.write_reg, 4, 0, value);
	return status;
}

s32 ngbe_phy_set_pause_advertisement_m88e1512(struct ngbe_hw *hw,
												u16 pause_bit)
{
	u16 value;
	s32 status = 0;
	if (hw->phy.type == ngbe_phy_m88e1512) {
		status = TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 0);
		status = TCALL(hw, phy.ops.read_reg_mdi, 4, 0, &value);
		value &= ~0xC00;
		value |= pause_bit;
		status = TCALL(hw, phy.ops.write_reg_mdi, 4, 0, value);
	} else {
		status = TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 1);
		status = TCALL(hw, phy.ops.read_reg_mdi, 4, 0, &value);
		value &= ~0x180;
		value |= pause_bit;
		status = TCALL(hw, phy.ops.write_reg_mdi, 4, 0, value);
	}

	return status;
}

s32 ngbe_phy_set_pause_advertisement_yt8521s(struct ngbe_hw *hw,
												u16 pause_bit)
{
	u16 value;
	s32 status = 0;

	status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x04, 0, &value);
	value &= ~0x180;
	value |= pause_bit;
	status = ngbe_phy_write_reg_sds_mii_yt8521s(hw, 0x04, 0, value);

#if 0
	ret_val = TCALL(hw, phy.ops.reset);
	if (ret_val) {
			return ret_val;
	}
#endif
	return status;
}


s32 ngbe_phy_setup(struct ngbe_hw *hw)
{
	int i;
	u16 value = 0;

	for (i = 0;i < 15;i++) {
		if (!rd32m(hw, NGBE_MIS_ST, NGBE_MIS_ST_GPHY_IN_RST(hw->bus.lan_id))) {
			break;
		}
		msleep(1);
	}

	if (i == 15) {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
						"GPhy reset exceeds maximum times.\n");
		return NGBE_ERR_PHY_TIMEOUT;
	}

	TCALL(hw, eeprom.ops.phy_signal_set);

	for (i = 0; i<1000;i++) {
		TCALL(hw, phy.ops.read_reg, 29, 0xa43, &value);
		if (value & 0x20)
			break;
	}

	TCALL(hw, phy.ops.read_reg, 29, 0xa46, &value);
	if (!(value & 0x1))
	{
		TCALL(hw, phy.ops.write_reg, 20, 0xa46, 1);
		for (i = 0; i<1000;i++) {
			TCALL(hw, phy.ops.read_reg, 29, 0xa43, &value);
			if (value & 0x20)
				break;
		}
		if (i == 1000) {
			return NGBE_ERR_PHY_TIMEOUT;
		}
	}

	TCALL(hw, phy.ops.write_reg, 20, 0xa46, 2);
	for (i = 0; i<1000;i++) {
		TCALL(hw, phy.ops.read_reg, 29, 0xa43, &value);
		if (value & 0x20)
			break;
	}

	if (i == 1000) {
		return NGBE_ERR_PHY_TIMEOUT;

	}

	for (i = 0; i<1000;i++) {
		TCALL(hw, phy.ops.read_reg, 16, 0xa42, &value);
		if ((value & 0x7) == 3)
			break;
	}

	if (i == 1000) {
		return NGBE_ERR_PHY_TIMEOUT;
	}

	return NGBE_OK;
}

s32 ngbe_init_phy_ops_common(struct ngbe_hw *hw)
{
	struct ngbe_phy_info *phy = &hw->phy;

	phy->ops.reset = ngbe_phy_reset;
	phy->ops.read_reg = ngbe_phy_read_reg;
	phy->ops.write_reg = ngbe_phy_write_reg;
	phy->ops.setup_link = ngbe_phy_setup_link;
	phy->ops.phy_led_ctrl = ngbe_phy_led_ctrl;
	phy->ops.check_overtemp = ngbe_phy_check_overtemp;
	phy->ops.identify = ngbe_phy_identify;
	phy->ops.init = ngbe_phy_init;
	phy->ops.check_event = ngbe_phy_check_event;
	phy->ops.get_adv_pause = ngbe_phy_get_advertised_pause;
	phy->ops.get_lp_adv_pause = ngbe_phy_get_lp_advertised_pause;
	phy->ops.set_adv_pause = ngbe_phy_set_pause_advertisement;
	phy->ops.setup_once = ngbe_phy_setup;

	return NGBE_OK;
}


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
 * based on ixgbe_phy.h, Copyright(c) 1999 - 2017 Intel Corporation.
 * Contact Information:
 * Linux NICS <linux.nics@intel.com>
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 */


#ifndef _TXGBE_PHY_H_
#define _TXGBE_PHY_H_

#include "txgbe_type.h"
#define TXGBE_I2C_EEPROM_DEV_ADDR       0xA0
#define TXGBE_I2C_EEPROM_DEV_ADDR2      0xA2
#define TXGBE_I2C_EEPROM_BANK_LEN       0xFF

/* EEPROM byte offsets */
#define TXGBE_SFF_IDENTIFIER            0x0
#define TXGBE_SFF_IDENTIFIER_SFP        0x3
#define TXGBE_SFF_VENDOR_OUI_BYTE0      0x25
#define TXGBE_SFF_VENDOR_OUI_BYTE1      0x26
#define TXGBE_SFF_VENDOR_OUI_BYTE2      0x27
#define TXGBE_SFF_1GBE_COMP_CODES       0x6
#define TXGBE_SFF_10GBE_COMP_CODES      0x3
#define TXGBE_SFF_CABLE_TECHNOLOGY      0x8
#define TXGBE_SFF_CABLE_SPEC_COMP       0x3C
#define TXGBE_SFF_SFF_8472_SWAP         0x5C
#define TXGBE_SFF_SFF_8472_COMP         0x5E
#define TXGBE_SFF_SFF_8472_OSCB         0x6E
#define TXGBE_SFF_SFF_8472_ESCB         0x76
#define TXGBE_SFF_IDENTIFIER_QSFP_PLUS  0xD
#define TXGBE_SFF_QSFP_VENDOR_OUI_BYTE0 0xA5
#define TXGBE_SFF_QSFP_VENDOR_OUI_BYTE1 0xA6
#define TXGBE_SFF_QSFP_VENDOR_OUI_BYTE2 0xA7
#define TXGBE_SFF_QSFP_CONNECTOR        0x82
#define TXGBE_SFF_QSFP_10GBE_COMP       0x83
#define TXGBE_SFF_QSFP_1GBE_COMP        0x86
#define TXGBE_SFF_QSFP_CABLE_LENGTH     0x92
#define TXGBE_SFF_QSFP_DEVICE_TECH      0x93

/* Bitmasks */
#define TXGBE_SFF_DA_PASSIVE_CABLE      0x4
#define TXGBE_SFF_DA_ACTIVE_CABLE       0x8
#define TXGBE_SFF_DA_SPEC_ACTIVE_LIMITING       0x4
#define TXGBE_SFF_1GBASESX_CAPABLE      0x1
#define TXGBE_SFF_1GBASELX_CAPABLE      0x2
#define TXGBE_SFF_1GBASET_CAPABLE       0x8
#define TXGBE_SFF_10GBASESR_CAPABLE     0x10
#define TXGBE_SFF_10GBASELR_CAPABLE     0x20
#define TXGBE_SFF_SOFT_RS_SELECT_MASK   0x8
#define TXGBE_SFF_SOFT_RS_SELECT_10G    0x8
#define TXGBE_SFF_SOFT_RS_SELECT_1G     0x0
#define TXGBE_SFF_ADDRESSING_MODE       0x4
#define TXGBE_SFF_QSFP_DA_ACTIVE_CABLE  0x1
#define TXGBE_SFF_QSFP_DA_PASSIVE_CABLE 0x8
#define TXGBE_SFF_QSFP_CONNECTOR_NOT_SEPARABLE  0x23
#define TXGBE_SFF_QSFP_TRANSMITER_850NM_VCSEL   0x0
#define TXGBE_I2C_EEPROM_READ_MASK      0x100
#define TXGBE_I2C_EEPROM_STATUS_MASK    0x3
#define TXGBE_I2C_EEPROM_STATUS_NO_OPERATION    0x0
#define TXGBE_I2C_EEPROM_STATUS_PASS    0x1
#define TXGBE_I2C_EEPROM_STATUS_FAIL    0x2
#define TXGBE_I2C_EEPROM_STATUS_IN_PROGRESS     0x3

#define TXGBE_CS4227                    0xBE    /* CS4227 address */
#define TXGBE_CS4227_GLOBAL_ID_LSB      0
#define TXGBE_CS4227_SCRATCH            2
#define TXGBE_CS4227_GLOBAL_ID_VALUE    0x03E5
#define TXGBE_CS4227_SCRATCH_VALUE      0x5aa5
#define TXGBE_CS4227_RETRIES            5
#define TXGBE_CS4227_LINE_SPARE22_MSB   0x12AD  /* Reg to program speed */
#define TXGBE_CS4227_LINE_SPARE24_LSB   0x12B0  /* Reg to program EDC */
#define TXGBE_CS4227_HOST_SPARE22_MSB   0x1AAD  /* Reg to program speed */
#define TXGBE_CS4227_HOST_SPARE24_LSB   0x1AB0  /* Reg to program EDC */
#define TXGBE_CS4227_EDC_MODE_CX1       0x0002
#define TXGBE_CS4227_EDC_MODE_SR        0x0004
#define TXGBE_CS4227_RESET_HOLD         500     /* microseconds */
#define TXGBE_CS4227_RESET_DELAY        500     /* milliseconds */
#define TXGBE_CS4227_CHECK_DELAY        30      /* milliseconds */
#define TXGBE_PE                        0xE0    /* Port expander address */
#define TXGBE_PE_OUTPUT                 1       /* Output register offset */
#define TXGBE_PE_CONFIG                 3       /* Config register offset */
#define TXGBE_PE_BIT1                   (1 << 1)

/* Flow control defines */
#define TXGBE_TAF_SYM_PAUSE (0x1)
#define TXGBE_TAF_ASM_PAUSE (0x2)

/* Bit-shift macros */
#define TXGBE_SFF_VENDOR_OUI_BYTE0_SHIFT        24
#define TXGBE_SFF_VENDOR_OUI_BYTE1_SHIFT        16
#define TXGBE_SFF_VENDOR_OUI_BYTE2_SHIFT        8

/* Vendor OUIs: format of OUI is 0x[byte0][byte1][byte2][00] */
#define TXGBE_SFF_VENDOR_OUI_TYCO       0x00407600
#define TXGBE_SFF_VENDOR_OUI_FTL        0x00906500
#define TXGBE_SFF_VENDOR_OUI_AVAGO      0x00176A00
#define TXGBE_SFF_VENDOR_OUI_INTEL      0x001B2100

/* I2C SDA and SCL timing parameters for standard mode */
#define TXGBE_I2C_T_HD_STA      4
#define TXGBE_I2C_T_LOW         5
#define TXGBE_I2C_T_HIGH        4
#define TXGBE_I2C_T_SU_STA      5
#define TXGBE_I2C_T_HD_DATA     5
#define TXGBE_I2C_T_SU_DATA     1
#define TXGBE_I2C_T_RISE        1
#define TXGBE_I2C_T_FALL        1
#define TXGBE_I2C_T_SU_STO      4
#define TXGBE_I2C_T_BUF         5

#ifndef TXGBE_SFP_DETECT_RETRIES
#define TXGBE_SFP_DETECT_RETRIES        10
#endif /* TXGBE_SFP_DETECT_RETRIES */

/* SFP+ SFF-8472 Compliance */
#define TXGBE_SFF_SFF_8472_UNSUP        0x00


enum txgbe_phy_type txgbe_get_phy_type_from_id(struct txgbe_hw *hw);
s32 txgbe_get_phy_id(struct txgbe_hw *hw);
s32 txgbe_reset_phy(struct txgbe_hw *hw);
s32 txgbe_read_phy_reg_mdi(struct txgbe_hw *hw, u32 reg_addr, u32 device_type,
			   u16 *phy_data);
s32 txgbe_write_phy_reg_mdi(struct txgbe_hw *hw, u32 reg_addr, u32 device_type,
			    u16 phy_data);
s32 txgbe_read_phy_reg(struct txgbe_hw *hw, u32 reg_addr,
			       u32 device_type, u16 *phy_data);
s32 txgbe_write_phy_reg(struct txgbe_hw *hw, u32 reg_addr,
				u32 device_type, u16 phy_data);
u32 txgbe_setup_phy_link(struct txgbe_hw *hw, u32 speed_set, bool autoneg_wait_to_complete);
u32 txgbe_setup_phy_link_speed(struct txgbe_hw *hw,
				       u32 speed,
				       bool autoneg_wait_to_complete);
s32 txgbe_get_copper_link_capabilities(struct txgbe_hw *hw,
					       u32 *speed,
					       bool *autoneg);
s32 txgbe_check_reset_blocked(struct txgbe_hw *hw);

s32 txgbe_get_phy_firmware_version(struct txgbe_hw *hw,
					   u16 *firmware_version);

s32 txgbe_identify_module(struct txgbe_hw *hw);
s32 txgbe_identify_sfp_module(struct txgbe_hw *hw);
s32 txgbe_tn_check_overtemp(struct txgbe_hw *hw);
s32 txgbe_init_i2c(struct txgbe_hw *hw);
s32 txgbe_clear_i2c(struct txgbe_hw *hw);
s32 txgbe_switch_i2c_slave_addr(struct txgbe_hw *hw, u8 dev_addr);
s32 txgbe_read_i2c_byte(struct txgbe_hw *hw, u8 byte_offset,
				u8 dev_addr, u8 *data);

s32 txgbe_write_i2c_byte(struct txgbe_hw *hw, u8 byte_offset,
				 u8 dev_addr, u8 data);
s32 txgbe_read_i2c_eeprom(struct txgbe_hw *hw, u8 byte_offset,
				  u8 *eeprom_data);
s32 txgbe_write_i2c_eeprom(struct txgbe_hw *hw, u8 byte_offset,
				   u8 eeprom_data);
s32 txgbe_read_i2c_sff8472(struct txgbe_hw *hw, u8 byte_offset,
							u8 *sff8472_data);
s32 txgbe_init_external_phy(struct txgbe_hw *hw);
s32 txgbe_uninit_external_phy(struct txgbe_hw *hw);
s32 txgbe_set_phy_pause_advertisement(struct txgbe_hw *hw, u32 pause_bit);
s32 txgbe_get_phy_advertised_pause(struct txgbe_hw *hw, u8 *pause_bit);
s32 txgbe_get_lp_advertised_pause(struct txgbe_hw *hw, u8 *pause_bit);

MTD_STATUS txgbe_read_mdio(
						MTD_DEV * dev,
						MTD_U16 port,
						MTD_U16 mmd,
						MTD_U16 reg,
						MTD_U16 *value);

MTD_STATUS txgbe_write_mdio(
						MTD_DEV * dev,
						MTD_U16 port,
						MTD_U16 mmd,
						MTD_U16 reg,
						MTD_U16 value);


#endif /* _TXGBE_PHY_H_ */

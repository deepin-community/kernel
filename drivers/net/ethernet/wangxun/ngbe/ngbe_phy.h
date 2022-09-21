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
 */


#ifndef _NGBE_PHY_H_
#define _NGBE_PHY_H_

#include "ngbe_type.h"
#include "ngbe.h"

/* EEPROM byte offsets */
#define NGBE_SFF_IDENTIFIER            0x0
#define NGBE_SFF_IDENTIFIER_SFP        0x3
#define NGBE_SFF_VENDOR_OUI_BYTE0      0x25
#define NGBE_SFF_VENDOR_OUI_BYTE1      0x26
#define NGBE_SFF_VENDOR_OUI_BYTE2      0x27
#define NGBE_SFF_1GBE_COMP_CODES       0x6
#define NGBE_SFF_10GBE_COMP_CODES      0x3
#define NGBE_SFF_CABLE_TECHNOLOGY      0x8
#define NGBE_SFF_CABLE_SPEC_COMP       0x3C
#define NGBE_SFF_SFF_8472_SWAP         0x5C
#define NGBE_SFF_SFF_8472_COMP         0x5E
#define NGBE_SFF_SFF_8472_OSCB         0x6E
#define NGBE_SFF_SFF_8472_ESCB         0x76
#define NGBE_SFF_IDENTIFIER_QSFP_PLUS  0xD
#define NGBE_SFF_QSFP_VENDOR_OUI_BYTE0 0xA5
#define NGBE_SFF_QSFP_VENDOR_OUI_BYTE1 0xA6
#define NGBE_SFF_QSFP_VENDOR_OUI_BYTE2 0xA7
#define NGBE_SFF_QSFP_CONNECTOR        0x82
#define NGBE_SFF_QSFP_10GBE_COMP       0x83
#define NGBE_SFF_QSFP_1GBE_COMP        0x86
#define NGBE_SFF_QSFP_CABLE_LENGTH     0x92
#define NGBE_SFF_QSFP_DEVICE_TECH      0x93

/* Bitmasks */
#define NGBE_SFF_DA_PASSIVE_CABLE      0x4
#define NGBE_SFF_DA_ACTIVE_CABLE       0x8
#define NGBE_SFF_DA_SPEC_ACTIVE_LIMITING       0x4
#define NGBE_SFF_1GBASESX_CAPABLE      0x1
#define NGBE_SFF_1GBASELX_CAPABLE      0x2
#define NGBE_SFF_1GBASET_CAPABLE       0x8
#define NGBE_SFF_10GBASESR_CAPABLE     0x10
#define NGBE_SFF_10GBASELR_CAPABLE     0x20
#define NGBE_SFF_SOFT_RS_SELECT_MASK   0x8
#define NGBE_SFF_SOFT_RS_SELECT_10G    0x8
#define NGBE_SFF_SOFT_RS_SELECT_1G     0x0
#define NGBE_SFF_ADDRESSING_MODE       0x4
#define NGBE_SFF_QSFP_DA_ACTIVE_CABLE  0x1
#define NGBE_SFF_QSFP_DA_PASSIVE_CABLE 0x8
#define NGBE_SFF_QSFP_CONNECTOR_NOT_SEPARABLE  0x23
#define NGBE_SFF_QSFP_TRANSMITER_850NM_VCSEL   0x0
#define NGBE_I2C_EEPROM_READ_MASK      0x100
#define NGBE_I2C_EEPROM_STATUS_MASK    0x3
#define NGBE_I2C_EEPROM_STATUS_NO_OPERATION    0x0
#define NGBE_I2C_EEPROM_STATUS_PASS    0x1
#define NGBE_I2C_EEPROM_STATUS_FAIL    0x2
#define NGBE_I2C_EEPROM_STATUS_IN_PROGRESS     0x3

#define NGBE_CS4227                    0xBE    /* CS4227 address */
#define NGBE_CS4227_GLOBAL_ID_LSB      0
#define NGBE_CS4227_SCRATCH            2
#define NGBE_CS4227_GLOBAL_ID_VALUE    0x03E5
#define NGBE_CS4227_SCRATCH_VALUE      0x5aa5
#define NGBE_CS4227_RETRIES            5
#define NGBE_CS4227_LINE_SPARE22_MSB   0x12AD  /* Reg to program speed */
#define NGBE_CS4227_LINE_SPARE24_LSB   0x12B0  /* Reg to program EDC */
#define NGBE_CS4227_HOST_SPARE22_MSB   0x1AAD  /* Reg to program speed */
#define NGBE_CS4227_HOST_SPARE24_LSB   0x1AB0  /* Reg to program EDC */
#define NGBE_CS4227_EDC_MODE_CX1       0x0002
#define NGBE_CS4227_EDC_MODE_SR        0x0004
#define NGBE_CS4227_RESET_HOLD         500     /* microseconds */
#define NGBE_CS4227_RESET_DELAY        500     /* milliseconds */
#define NGBE_CS4227_CHECK_DELAY        30      /* milliseconds */
#define NGBE_PE                        0xE0    /* Port expander address */
#define NGBE_PE_OUTPUT                 1       /* Output register offset */
#define NGBE_PE_CONFIG                 3       /* Config register offset */
#define NGBE_PE_BIT1                   (1 << 1)

/* Flow control defines */
#define NGBE_TAF_SYM_PAUSE (0x1)
#define NGBE_TAF_ASM_PAUSE (0x2)

/* Bit-shift macros */
#define NGBE_SFF_VENDOR_OUI_BYTE0_SHIFT        24
#define NGBE_SFF_VENDOR_OUI_BYTE1_SHIFT        16
#define NGBE_SFF_VENDOR_OUI_BYTE2_SHIFT        8

/* Vendor OUIs: format of OUI is 0x[byte0][byte1][byte2][00] */
#define NGBE_SFF_VENDOR_OUI_TYCO       0x00407600
#define NGBE_SFF_VENDOR_OUI_FTL        0x00906500
#define NGBE_SFF_VENDOR_OUI_AVAGO      0x00176A00
#define NGBE_SFF_VENDOR_OUI_INTEL      0x001B2100

/* I2C SDA and SCL timing parameters for standard mode */
#define NGBE_I2C_T_HD_STA      4
#define NGBE_I2C_T_LOW         5
#define NGBE_I2C_T_HIGH        4
#define NGBE_I2C_T_SU_STA      5
#define NGBE_I2C_T_HD_DATA     5
#define NGBE_I2C_T_SU_DATA     1
#define NGBE_I2C_T_RISE        1
#define NGBE_I2C_T_FALL        1
#define NGBE_I2C_T_SU_STO      4
#define NGBE_I2C_T_BUF         5

#ifndef NGBE_SFP_DETECT_RETRIES
#define NGBE_SFP_DETECT_RETRIES        10
#endif /* NGBE_SFP_DETECT_RETRIES */

/* SFP+ SFF-8472 Compliance */
#define NGBE_SFF_SFF_8472_UNSUP        0x00


enum ngbe_phy_type ngbe_get_phy_type_from_id(struct ngbe_hw *hw);
s32 ngbe_init_phy_ops_common(struct ngbe_hw *hw);
s32 ngbe_phy_read_reg_mdi(	struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 *phy_data);
s32 ngbe_phy_write_reg_mdi(	struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 phy_data);

s32 ngbe_phy_read_reg_sds_mii_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 *phy_data);
s32 ngbe_phy_write_reg_sds_mii_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 phy_data);

s32 ngbe_phy_read_reg_ext_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 *phy_data);
s32 ngbe_phy_write_reg_ext_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 phy_data);

s32 ngbe_phy_read_reg_sds_ext_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 *phy_data);
s32 ngbe_phy_write_reg_sds_ext_yt8521s(struct ngbe_hw *hw,
							u32 reg_addr,
							u32 device_type,
							u16 phy_data);

s32 ngbe_phy_init(struct ngbe_hw *hw);
s32 ngbe_phy_identify(struct ngbe_hw *hw);
s32 ngbe_phy_reset(struct ngbe_hw *hw);
u32 ngbe_phy_setup_link(struct ngbe_hw *hw,
						u32 speed,
						bool need_restart_AN);
u32 ngbe_phy_led_ctrl(struct ngbe_hw *hw);
s32 ngbe_phy_reset_m88e1512(struct ngbe_hw *hw);
u32 ngbe_phy_setup_link_m88e1512(	struct ngbe_hw *hw,
									u32 speed,
									bool autoneg_wait_to_complete);
s32 ngbe_phy_check_overtemp(struct ngbe_hw *hw);

s32 ngbe_check_zte_phy_id(struct ngbe_hw *hw);
s32 ngbe_phy_reset_zte(struct ngbe_hw *hw);
u32 ngbe_phy_setup_link_zte(struct ngbe_hw *hw,
							u32 speed,
							bool autoneg_wait_to_complete);
s32 ngbe_phy_check_event(struct ngbe_hw *hw);
s32 ngbe_phy_check_event_m88e1512(struct ngbe_hw *hw);
s32 ngbe_phy_get_advertised_pause_m88e1512(struct ngbe_hw *hw, u8 *pause_bit);
s32 ngbe_phy_get_lp_advertised_pause_m88e1512(struct ngbe_hw *hw,
												u8 *pause_bit);
s32 ngbe_phy_set_pause_advertisement_m88e1512(struct ngbe_hw *hw,
												u16 pause_bit);

s32 ngbe_phy_reset_yt8521s(struct ngbe_hw *hw);
u32 ngbe_phy_setup_link_yt8521s(	struct ngbe_hw *hw,
									u32 speed,
									bool autoneg_wait_to_complete);

s32 ngbe_phy_check_event_yt8521s(struct ngbe_hw *hw);
s32 ngbe_phy_get_advertised_pause_yt8521s(struct ngbe_hw *hw, u8 *pause_bit);
s32 ngbe_phy_get_lp_advertised_pause_yt8521s(struct ngbe_hw *hw,
												u8 *pause_bit);
s32 ngbe_phy_set_pause_advertisement_yt8521s(struct ngbe_hw *hw,
												u16 pause_bit);


#endif /* _NGBE_PHY_H_ */

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

#ifndef _NGBE_STATUS_H_
#define _NGBE_STATUS_H_

/* Error Codes:
 * (-256, 256): reserved for non-ngbe defined error code
 */
#define NGBE_ERR_BASE (0x100)
enum ngbe_error {
	NGBE_ERR_NULL = NGBE_ERR_BASE, /* errline=__LINE__+errno-256 */
	NGBE_ERR_NOSUPP,
	NGBE_ERR_EEPROM,
	NGBE_ERR_EEPROM_CHECKSUM,
	NGBE_ERR_PHY,
	NGBE_ERR_CONFIG,
	NGBE_ERR_PARAM,
	NGBE_ERR_MAC_TYPE,
	NGBE_ERR_UNKNOWN_PHY,
	NGBE_ERR_LINK_SETUP,
	NGBE_ERR_ADAPTER_STOPPED,
	NGBE_ERR_INVALID_MAC_ADDR,
	NGBE_ERR_DEVICE_NOT_SUPPORTED,
	NGBE_ERR_MASTER_REQUESTS_PENDING,
	NGBE_ERR_INVALID_LINK_SETTINGS,
	NGBE_ERR_AUTONEG_NOT_COMPLETE,
	NGBE_ERR_RESET_FAILED,
	NGBE_ERR_SWFW_SYNC,
	NGBE_ERR_PHY_ADDR_INVALID,
	NGBE_ERR_I2C,
	NGBE_ERR_SFP_NOT_SUPPORTED,
	NGBE_ERR_SFP_NOT_PRESENT,
	NGBE_ERR_SFP_NO_INIT_SEQ_PRESENT,
	NGBE_ERR_NO_SAN_ADDR_PTR,
	NGBE_ERR_FDIR_REINIT_FAILED,
	NGBE_ERR_EEPROM_VERSION,
	NGBE_ERR_NO_SPACE,
	NGBE_ERR_OVERTEMP,
	NGBE_ERR_UNDERTEMP,
	NGBE_ERR_FC_NOT_NEGOTIATED,
	NGBE_ERR_FC_NOT_SUPPORTED,
	NGBE_ERR_SFP_SETUP_NOT_COMPLETE,
	NGBE_ERR_PBA_SECTION,
	NGBE_ERR_INVALID_ARGUMENT,
	NGBE_ERR_HOST_INTERFACE_COMMAND,
	NGBE_ERR_OUT_OF_MEM,
	NGBE_ERR_FEATURE_NOT_SUPPORTED,
	NGBE_ERR_EEPROM_PROTECTED_REGION,
	NGBE_ERR_FDIR_CMD_INCOMPLETE,
	NGBE_ERR_FLASH_LOADING_FAILED,
	NGBE_ERR_XPCS_POWER_UP_FAILED,
	NGBE_ERR_FW_RESP_INVALID,
	NGBE_ERR_PHY_INIT_NOT_DONE,
	NGBE_ERR_TOKEN_RETRY,
	NGBE_ERR_REG_TMOUT,
	NGBE_ERR_REG_ACCESS,
	NGBE_ERR_MBX,
};

#define NGBE_ERR_NOSUPP                      (-NGBE_ERR_NOSUPP)
#define NGBE_ERR_EEPROM                      (-NGBE_ERR_EEPROM)
#define NGBE_ERR_EEPROM_CHECKSUM             (-NGBE_ERR_EEPROM_CHECKSUM)
#define NGBE_ERR_PHY                         (-NGBE_ERR_PHY)
#define NGBE_ERR_CONFIG                      (-NGBE_ERR_CONFIG)
#define NGBE_ERR_PARAM                       (-NGBE_ERR_PARAM)
#define NGBE_ERR_MAC_TYPE                    (-NGBE_ERR_MAC_TYPE)
#define NGBE_ERR_UNKNOWN_PHY                 (-NGBE_ERR_UNKNOWN_PHY)
#define NGBE_ERR_LINK_SETUP                  (-NGBE_ERR_LINK_SETUP)
#define NGBE_ERR_ADAPTER_STOPPED             (-NGBE_ERR_ADAPTER_STOPPED)
#define NGBE_ERR_INVALID_MAC_ADDR            (-NGBE_ERR_INVALID_MAC_ADDR)
#define NGBE_ERR_DEVICE_NOT_SUPPORTED        (-NGBE_ERR_DEVICE_NOT_SUPPORTED)
#define NGBE_ERR_MASTER_REQUESTS_PENDING     (-NGBE_ERR_MASTER_REQUESTS_PENDING)
#define NGBE_ERR_INVALID_LINK_SETTINGS       (-NGBE_ERR_INVALID_LINK_SETTINGS)
#define NGBE_ERR_AUTONEG_NOT_COMPLETE        (-NGBE_ERR_AUTONEG_NOT_COMPLETE)
#define NGBE_ERR_RESET_FAILED                (-NGBE_ERR_RESET_FAILED)
#define NGBE_ERR_SWFW_SYNC                   (-NGBE_ERR_SWFW_SYNC)
#define NGBE_ERR_PHY_ADDR_INVALID            (-NGBE_ERR_PHY_ADDR_INVALID)
#define NGBE_ERR_I2C                         (-NGBE_ERR_I2C)
#define NGBE_ERR_SFP_NOT_SUPPORTED           (-NGBE_ERR_SFP_NOT_SUPPORTED)
#define NGBE_ERR_SFP_NOT_PRESENT             (-NGBE_ERR_SFP_NOT_PRESENT)
#define NGBE_ERR_SFP_NO_INIT_SEQ_PRESENT     (-NGBE_ERR_SFP_NO_INIT_SEQ_PRESENT)
#define NGBE_ERR_NO_SAN_ADDR_PTR             (-NGBE_ERR_NO_SAN_ADDR_PTR)
#define NGBE_ERR_FDIR_REINIT_FAILED          (-NGBE_ERR_FDIR_REINIT_FAILED)
#define NGBE_ERR_EEPROM_VERSION              (-NGBE_ERR_EEPROM_VERSION)
#define NGBE_ERR_NO_SPACE                    (-NGBE_ERR_NO_SPACE)
#define NGBE_ERR_OVERTEMP                    (-NGBE_ERR_OVERTEMP)
#define NGBE_ERR_UNDERTEMP                   (-NGBE_ERR_UNDERTEMP)
#define NGBE_ERR_FC_NOT_NEGOTIATED           (-NGBE_ERR_FC_NOT_NEGOTIATED)
#define NGBE_ERR_FC_NOT_SUPPORTED            (-NGBE_ERR_FC_NOT_SUPPORTED)
#define NGBE_ERR_SFP_SETUP_NOT_COMPLETE      (-NGBE_ERR_SFP_SETUP_NOT_COMPLETE)
#define NGBE_ERR_PBA_SECTION                 (-NGBE_ERR_PBA_SECTION)
#define NGBE_ERR_INVALID_ARGUMENT            (-NGBE_ERR_INVALID_ARGUMENT)
#define NGBE_ERR_HOST_INTERFACE_COMMAND      (-NGBE_ERR_HOST_INTERFACE_COMMAND)
#define NGBE_ERR_OUT_OF_MEM                  (-NGBE_ERR_OUT_OF_MEM)
#define NGBE_ERR_FEATURE_NOT_SUPPORTED       (-NGBE_ERR_FEATURE_NOT_SUPPORTED)
#define NGBE_ERR_EEPROM_PROTECTED_REGION     (-NGBE_ERR_EEPROM_PROTECTED_REGION)
#define NGBE_ERR_FDIR_CMD_INCOMPLETE         (-NGBE_ERR_FDIR_CMD_INCOMPLETE)
#define NGBE_ERR_FLASH_LOADING_FAILED        (-NGBE_ERR_FLASH_LOADING_FAILED)
#define NGBE_ERR_XPCS_POWER_UP_FAILED        (-NGBE_ERR_XPCS_POWER_UP_FAILED)
#define NGBE_ERR_FW_RESP_INVALID             (-NGBE_ERR_FW_RESP_INVALID)
#define NGBE_ERR_PHY_INIT_NOT_DONE           (-NGBE_ERR_PHY_INIT_NOT_DONE)
#define NGBE_ERR_TOKEN_RETRY                 (-NGBE_ERR_TOKEN_RETRY)
#define NGBE_ERR_REG_TMOUT                   (-NGBE_ERR_REG_TMOUT)
#define NGBE_ERR_REG_ACCESS                  (-NGBE_ERR_REG_ACCESS)
#define NGBE_ERR_MBX                         (-NGBE_ERR_MBX)

#endif /* _NGBE_STATUS_H_ */

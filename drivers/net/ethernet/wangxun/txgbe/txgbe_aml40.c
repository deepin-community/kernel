// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019 - 2022 Beijing WangXun Technology Co., Ltd. */

#include "txgbe_type.h"
#include "txgbe_hw.h"
#include "txgbe_aml40.h"
#include "txgbe_e56.h"
#include "txgbe_e56_bp.h"
#include "txgbe_phy.h"

#include "txgbe.h"

/**
 *  txgbe_get_media_type_aml40 - Get media type
 *  @hw: pointer to hardware structure
 *
 *  Returns the media type (fiber, copper, backplane)
 **/
enum txgbe_media_type txgbe_get_media_type_aml40(struct txgbe_hw *hw)
{
	u8 device_type = hw->subsystem_device_id & 0xF0;
	enum txgbe_media_type media_type;

	switch (device_type) {
	case TXGBE_ID_KR_KX_KX4:
		media_type = txgbe_media_type_backplane;
		break;
	case TXGBE_ID_SFP:
		media_type = txgbe_media_type_fiber_qsfp;
		break;
	default:
		media_type = txgbe_media_type_unknown;
		break;
	}

	return media_type;
}

/**
 *  txgbe_setup_mac_link_aml - Set MAC link speed
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: true when waiting for completion is needed
 *
 *  Set the link speed in the AUTOC register and restarts link.
 **/
static s32 txgbe_setup_mac_link_aml40(struct txgbe_hw *hw,
				      u32 speed,
			       bool autoneg_wait_to_complete)
{
	u32 link_capabilities = TXGBE_LINK_SPEED_UNKNOWN;
	u32 link_speed = TXGBE_LINK_SPEED_UNKNOWN;
	struct txgbe_adapter *adapter = hw->back;
	bool link_up = false;
	bool autoneg = false;
	s32 ret_status = 0;
	int i = 0;
	s32 status = 0;

	/* Check to see if speed passed in is supported. */
	status = hw->mac.ops.get_link_capabilities(hw,
				   &link_capabilities, &autoneg);
	if (status)
		goto out;

	speed &= link_capabilities;

	if (speed == TXGBE_LINK_SPEED_UNKNOWN) {
		status = TXGBE_ERR_LINK_SETUP;
		goto out;
	}

	if (hw->phy.sfp_type == txgbe_qsfp_type_40g_cu_core0 ||
	    hw->phy.sfp_type == txgbe_qsfp_type_40g_cu_core1 ||
	    txgbe_is_backplane(hw)) {
		txgbe_e56_check_phy_link(hw, &link_speed, &link_up);
		if (!adapter->backplane_an) {
			if (link_speed == speed && link_up)
				goto out;
		} else {
			if (link_up && adapter->an_done)
				goto out;
		}
		mutex_lock(&adapter->e56_lock);
		txgbe_e56_set_phylinkmode(adapter, 40, hw->bypass_ctle);
		mutex_unlock(&adapter->e56_lock);
		goto out;
	}

	for (i = 0; i < 4; i++) {
		txgbe_e56_check_phy_link(hw, &link_speed, &link_up);
		if (link_up)
			break;
		msleep(250);
	}

	if (link_speed == speed && link_up)
		goto out;

	mutex_lock(&adapter->e56_lock);
	ret_status = txgbe_set_link_to_amlite(hw, speed);
	mutex_unlock(&adapter->e56_lock);

	if (ret_status == TXGBE_ERR_TIMEOUT)
		adapter->link_valid = false;

	for (i = 0; i < 4; i++) {
		txgbe_e56_check_phy_link(hw, &link_speed, &link_up);
		if (link_up)
			goto out;
		msleep(250);
	}

	adapter->flags |= TXGBE_FLAG_NEED_LINK_CONFIG;

out:
	return status;
}

/**
 *  txgbe_get_link_capabilities_aml40 - Determines link capabilities
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @autoneg: true when autoneg or autotry is enabled
 *
 *  Determines the link capabilities by reading the AUTOC register.
 **/
static s32 txgbe_get_link_capabilities_aml40(struct txgbe_hw *hw,
					     u32 *speed,
					     bool *autoneg)
{
	s32 status = 0;

	if (hw->phy.sfp_type == txgbe_qsfp_type_40g_cu_core0 ||
	    hw->phy.sfp_type == txgbe_qsfp_type_40g_cu_core1) {
		*autoneg = true;
		*speed = TXGBE_LINK_SPEED_40GB_FULL;
	} else if (txgbe_is_backplane(hw)) {
		*speed = TXGBE_LINK_SPEED_40GB_FULL;
		*autoneg = true;
	} else {
		*speed = TXGBE_LINK_SPEED_40GB_FULL;
		*autoneg = true;
	}

	return status;
}

/**
 *  txgbe_check_mac_link_aml40 - Determine link and speed status
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @link_up: true when link is up
 *  @link_up_wait_to_complete: bool used to wait for link up or not
 *
 *  Reads the links register to determine if link is up and the current speed
 **/
static s32 txgbe_check_mac_link_aml40(struct txgbe_hw *hw, u32 *speed,
				      bool *link_up, bool link_up_wait_to_complete)
{
	struct txgbe_adapter *adapter = hw->back;
	u32 links_reg = 0;
	u32 i;

	if (link_up_wait_to_complete) {
		for (i = 0; i < TXGBE_LINK_UP_TIME; i++) {
			links_reg = rd32(hw,
					 TXGBE_CFG_PORT_ST);

			if (!adapter->link_valid) {
				*link_up = false;

				msleep(100);
				continue;
			}

			if (!(links_reg & TXGBE_CFG_PORT_ST_LINK_UP)) {
				*link_up = false;
			} else {
				*link_up = true;
				break;
			}
			msleep(100);
		}
	} else {
		links_reg = rd32(hw, TXGBE_CFG_PORT_ST);
		if (links_reg & TXGBE_CFG_PORT_ST_LINK_UP)
			*link_up = true;
		else
			*link_up = false;
	}

	if (!adapter->link_valid)
		*link_up = false;

	if (*link_up) {
		if ((links_reg & TXGBE_CFG_PORT_ST_AML_LINK_40G) ==
				TXGBE_CFG_PORT_ST_AML_LINK_40G)
			*speed = TXGBE_LINK_SPEED_40GB_FULL;
	} else {
		*speed = TXGBE_LINK_SPEED_UNKNOWN;
	}

	if (txgbe_is_backplane(hw)) {
		if (!adapter->an_done) {
			*link_up = false;
			*speed = TXGBE_LINK_SPEED_UNKNOWN;
		}
	}

	return 0;
}

static void txgbe_init_mac_link_ops_aml40(struct txgbe_hw *hw)
{
	struct txgbe_mac_info *mac = &hw->mac;

	mac->ops.disable_tx_laser =
			txgbe_disable_tx_laser_multispeed_fiber;
	mac->ops.enable_tx_laser =
			txgbe_enable_tx_laser_multispeed_fiber;
	mac->ops.flap_tx_laser = txgbe_flap_tx_laser_multispeed_fiber;

	mac->ops.setup_link = txgbe_setup_mac_link_aml40;
	mac->ops.set_rate_select_speed = txgbe_set_hard_rate_select_speed;
}

static s32 txgbe_setup_sfp_modules_aml40(struct txgbe_hw *hw)
{
	s32 ret_val = 0;

	if (hw->phy.sfp_type != txgbe_sfp_type_unknown) {
		txgbe_init_mac_link_ops_aml40(hw);

		hw->phy.ops.reset = NULL;
	}

	return ret_val;
}

/**
 *  txgbe_init_phy_ops - PHY/SFP specific init
 *  @hw: pointer to hardware structure
 *
 *  Initialize any function pointers that were not able to be
 *  set during init_shared_code because the PHY/SFP type was
 *  not known.  Perform the SFP init if necessary.
 *
 **/
s32 txgbe_init_phy_ops_aml40(struct txgbe_hw *hw)
{
	s32 ret_val = 0;

	txgbe_init_i2c(hw);
	wr32(hw, TXGBE_MAC_MDIO_CLAUSE_22_PORT,
	     TXGBE_MAC_MDIO_CLAUSE_ALL_PRTCL22);

	/* Identify the PHY or SFP module */
	ret_val = hw->phy.ops.identify(hw);
	if (ret_val == TXGBE_ERR_SFP_NOT_SUPPORTED)
		goto init_phy_ops_out;

	/* Setup function pointers based on detected SFP module and speeds */
	txgbe_init_mac_link_ops_aml40(hw);
	if (hw->phy.sfp_type != txgbe_sfp_type_unknown)
		hw->phy.ops.reset = NULL;

init_phy_ops_out:
	return ret_val;
}

s32 txgbe_init_ops_aml40(struct txgbe_hw *hw)
{
	struct txgbe_mac_info *mac = &hw->mac;
	struct txgbe_phy_info *phy = &hw->phy;
	s32 ret_val = 0;

	ret_val = txgbe_init_ops_generic(hw);

	/* PHY */
	phy->ops.init = txgbe_init_phy_ops_aml40;

	/* MAC */
	mac->ops.get_media_type = txgbe_get_media_type_aml40;
	mac->ops.setup_sfp = txgbe_setup_sfp_modules_aml40;

	/* LINK */
	mac->ops.check_link = txgbe_check_mac_link_aml40;
	mac->ops.setup_link = txgbe_setup_mac_link_aml40;
	mac->ops.get_link_capabilities = txgbe_get_link_capabilities_aml40;

	return ret_val;
}


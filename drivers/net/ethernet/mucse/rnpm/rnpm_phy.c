// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/mdio.h>
#include "rnpm_common.h"
#include "rnpm_phy.h"
#include "rnpm_mbx_fw.h"

#define RNPM_PHY_REVISION_MASK 0xFFFFFFF0
#define RNPM_MAX_PHY_ADDR 32

static s32 rnpm_get_phy_id(struct rnpm_hw *hw);

/**
 *  rnpm_get_phy_id - Get the phy type
 *  @hw: pointer to hardware structure
 *
 **/
__maybe_unused static s32 rnpm_get_phy_id(struct rnpm_hw *hw)
{
	u32 status;
	u16 phy_id_high = 0;
	u16 phy_id_low = 0;

	status = hw->phy.ops.read_reg(hw, MDIO_DEVID1, MDIO_MMD_PMAPMD,
				      &phy_id_high);

	if (status == 0) {
		hw->phy.id = (u32)(phy_id_high << 16);
		status = hw->phy.ops.read_reg(
			hw, MDIO_DEVID2, MDIO_MMD_PMAPMD, &phy_id_low);
		hw->phy.id |= (u32)(phy_id_low & RNPM_PHY_REVISION_MASK);
		hw->phy.revision =
			(u32)(phy_id_low & ~RNPM_PHY_REVISION_MASK);
	}
	return status;
}

/**
 *  rnpm_reset_phy_generic - Performs a PHY reset
 *  @hw: pointer to hardware structure
 **/
s32 rnpm_reset_phy_generic(struct rnpm_hw *hw)
{
	s32 status = 0;

	return status;
}

s32 rnpm_power_down_phy_generic(struct rnpm_hw *hw, bool on)
{
	u32 val;

	rnpm_mbx_phy_read(hw, 0x0, &val);
	if (on)
		val |= RNPM_MDI_PHY_POWER_DOWN;
	else
		val &= ~RNPM_MDI_PHY_POWER_DOWN;
	rnpm_mbx_phy_write(hw, 0x0, val);
	return 0;
}

s32 rnpm_sfp_tx_dis_generic(struct rnpm_hw *hw, bool on)
{
	if (on)
		rnpm_mbx_sfp_write(hw, 0xa2, 0x6e, 0x40);
	else
		rnpm_mbx_sfp_write(hw, 0xa2, 0x6e, 0x0);
	return 0;
}

/**
 *  rnpm_read_phy_reg_generic - Reads a value from a specified PHY register
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit address of PHY register to read
 *  @phy_data: Pointer to read data from PHY register
 **/
s32 rnpm_read_phy_reg_generic(struct rnpm_hw *hw, u32 reg_addr,
			      u32 device_type, u16 *phy_data)
{
	s32 status = 0, data = 0;

	status = rnpm_mbx_phy_read(hw, reg_addr, &data);
	*phy_data = data & 0xffff;
	return status;
}

/**
 *  rnpm_write_phy_reg_generic - Writes a value to specified PHY register
 *  @hw: pointer to hardware structure
 *  @reg_addr: 32 bit PHY register to write
 *  @device_type: 5 bit device type
 *  @phy_data: Data to write to the PHY register
 **/
s32 rnpm_write_phy_reg_generic(struct rnpm_hw *hw, u32 reg_addr,
			       u32 device_type, u16 phy_data)
{
	s32 status = 0;

	status = rnpm_mbx_phy_write(hw, reg_addr, (u32)phy_data);
	return status;
}

/**
 *  rnpm_setup_phy_link_generic - Set and restart autoneg
 *  @hw: pointer to hardware structure
 *
 *  Restart autonegotiation and PHY and waits for completion.
 **/
s32 rnpm_setup_phy_link_generic(struct rnpm_hw *hw)
{
	s32 status = 0;

	rnpm_mbx_phy_link_set(hw, hw->phy.autoneg_advertised);
	return status;
}

/**
 * rnpm_setup_phy_link_speed_generic - Sets the auto advertised capabilities
 * @hw: pointer to hardware structure
 * @speed: new link speed
 */
s32 rnpm_setup_phy_link_speed_generic(struct rnpm_hw *hw,
				      rnpm_link_speed speed,
				      bool autoneg_wait_to_complete)
{
	struct rnpm_adapter *adpt = hw->back;
	u32 value = 0, value_r4 = 0, value_r9 = 0;

	rnpm_logd(
		LOG_PHY,
		"%s setup phy: phy_addr=%d speed=%d duplex=%d autoneg=%d\n",
		__func__, adpt->phy_addr, speed, hw->mac.duplex,
		hw->mac.autoneg);
	rnpm_logd(LOG_PHY, "is_backplane=%d is_sgmii=%d mdix=%d\n",
		  hw->is_backplane, hw->is_sgmii, hw->phy.mdix);

	if (hw->is_backplane) {
		/* Backplane type, support AN, unsupport set speed */
		return rnpm_set_lane_fun(hw, LANE_FUN_AN, hw->mac.autoneg,
					 0, 0, 0);
	}

	/* Fiber only support force speed 1G/10G*/
	if (!hw->is_sgmii) {
		if (adpt->pf_adapter->force_10g_1g_speed_ablity) {
			rnpm_mbx_force_speed(hw,
					     hw->mac.autoneg ? 0 : speed);
			/* Update port link info when firber is absent */
			set_bit(RNPM_PF_LINK_CHANGE,
				&adpt->pf_adapter->flags);
		}
		return 0;
	}

	/* Set MDI/MDIX mode */
	rnpm_mbx_phy_read(hw, RNPM_YT8531_PHY_SPEC_CTRL, &value);
	value &= ~RNPM_YT8531_PHY_SPEC_CTRL_MDIX_CFG_MASK;
	/* Options: 0: Auto (default)  1: MDI mode  2: MDI-X mode */
	switch (hw->phy.mdix) {
	case 1:
		break;
	case 2:
		value |= RNPM_YT8531_PHY_SPEC_CTRL_FORCE_MDIX;
		break;
	case 0:
	default:
		value |= RNPM_YT8531_PHY_SPEC_CTRL_AUTO_MDI_MDIX;
		break;
	}
	rnpm_mbx_phy_write(hw, RNPM_YT8531_PHY_SPEC_CTRL, value);

	if (speed == RNPM_LINK_SPEED_UNKNOWN) {
		rnpm_mbx_phy_read(hw, 0x0, &value);
		value |= RNPM_MDI_PHY_RESET;
		rnpm_mbx_phy_write(hw, 0x0, value);
		goto skip_an;
	}
	/*
	 * Clear autoneg_advertised and set new values based on input link
	 * speed.
	 */
	hw->phy.autoneg_advertised = speed;
	adpt->fake_force_1000m = 0;

	if (!hw->mac.autoneg) {
		switch (speed) {
		case RNPM_LINK_SPEED_1GB_FULL:
		case RNPM_LINK_SPEED_1GB_HALF:
			value = RNPM_MDI_PHY_SPEED_SELECT1;
			speed = RNPM_LINK_SPEED_1GB_FULL;
			adpt->fake_force_1000m = 1;
			goto auto_neg;
		case RNPM_LINK_SPEED_100_FULL:
		case RNPM_LINK_SPEED_100_HALF:
			value = RNPM_MDI_PHY_SPEED_SELECT0;
			break;
		case RNPM_LINK_SPEED_10_FULL:
		case RNPM_LINK_SPEED_10_HALF:
			value = 0;
			break;
		default:
			value = RNPM_MDI_PHY_SPEED_SELECT0 |
				RNPM_MDI_PHY_SPEED_SELECT1;
			hw_dbg(hw, "unknown speed = 0x%x.\n", speed);
			break;
		}
		/* duplex full */
		if (hw->mac.duplex)
			value |= RNPM_MDI_PHY_DUPLEX;
		value |= RNPM_MDI_PHY_RESET;
		value &= ~RNPM_MDI_PHY_ANE;
		rnpm_mbx_phy_write(hw, 0x0, value);
		goto skip_an;
	}

auto_neg:
	value_r4 = 0x1E0;
	value_r9 = 0x300;
	/*disable 100/10base-T Self-negotiation ability*/
	rnpm_mbx_phy_read(hw, 0x4, &value);
	value &= ~value_r4;
	rnpm_mbx_phy_write(hw, 0x4, value);

	/*disable 1000base-T Self-negotiation ability*/
	rnpm_mbx_phy_read(hw, 0x9, &value);
	value &= ~value_r9;
	rnpm_mbx_phy_write(hw, 0x9, value);

	value_r4 = 0x0;
	value_r9 = 0x0;

	if (speed & RNPM_LINK_SPEED_1GB_FULL) {
		hw->phy.autoneg_advertised |= RNPM_LINK_SPEED_1GB_FULL;
		value_r9 |= 0x200;
	}
	if (speed & RNPM_LINK_SPEED_100_FULL) {
		hw->phy.autoneg_advertised |= RNPM_LINK_SPEED_100_FULL;
		value_r4 |= 0x100;
	}
	if (speed & RNPM_LINK_SPEED_10_FULL) {
		hw->phy.autoneg_advertised |= RNPM_LINK_SPEED_10_FULL;
		value_r4 |= 0x40;
	}

	if (speed & RNPM_LINK_SPEED_1GB_HALF) {
		hw->phy.autoneg_advertised |= RNPM_LINK_SPEED_1GB_HALF;
		value_r9 |= 0x100;
	}
	if (speed & RNPM_LINK_SPEED_100_HALF) {
		hw->phy.autoneg_advertised |= RNPM_LINK_SPEED_100_HALF;
		value_r4 |= 0x80;
	}
	if (speed & RNPM_LINK_SPEED_10_HALF) {
		hw->phy.autoneg_advertised |= RNPM_LINK_SPEED_10_HALF;
		value_r4 |= 0x20;
	}

	/* enable 1000base-T Self-negotiation ability */
	rnpm_mbx_phy_read(hw, 0x9, &value);
	value |= value_r9;
	rnpm_mbx_phy_write(hw, 0x9, value);

	if (hw->mac.autoneg) {
		/* enable 100/10base-T Self-negotiation ability */
		rnpm_mbx_phy_read(hw, 0x4, &value);
		value |= value_r4;
		rnpm_mbx_phy_write(hw, 0x4, value);
	}

	/* software reset to make the above configuration take effect*/
	rnpm_mbx_phy_read(hw, 0x0, &value);
	value |= 0x9200;
	rnpm_mbx_phy_write(hw, 0x0, value);
skip_an:
	/* power on in UTP mode */
	rnpm_mbx_phy_read(hw, 0x0, &value);
	value &= ~0x800;
	rnpm_mbx_phy_write(hw, 0x0, value);
	return 0;
}

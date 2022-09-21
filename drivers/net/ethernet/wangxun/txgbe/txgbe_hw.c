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
 * based on ixgbe_82599.c, Copyright(c) 1999 - 2017 Intel Corporation.
 * Contact Information:
 * Linux NICS <linux.nics@intel.com>
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 */


#include "txgbe_type.h"
#include "txgbe_hw.h"
#include "txgbe_phy.h"
#include "txgbe_dcb.h"
#include "txgbe.h"

#ifndef SIMULATION_DEBUG
#define TXGBE_SP_MAX_TX_QUEUES  128
#define TXGBE_SP_MAX_RX_QUEUES  128
#else
#define TXGBE_SP_MAX_TX_QUEUES  16
#define TXGBE_SP_MAX_RX_QUEUES  16
#endif
#define TXGBE_SP_RAR_ENTRIES    128
#define TXGBE_SP_MC_TBL_SIZE    128
#define TXGBE_SP_VFT_TBL_SIZE   128
#define TXGBE_SP_RX_PB_SIZE     512

STATIC s32 txgbe_get_eeprom_semaphore(struct txgbe_hw *hw);
STATIC void txgbe_release_eeprom_semaphore(struct txgbe_hw *hw);
STATIC s32 txgbe_mta_vector(struct txgbe_hw *hw, u8 *mc_addr);
STATIC s32 txgbe_get_san_mac_addr_offset(struct txgbe_hw *hw,
					 u16 *san_mac_offset);

STATIC s32 txgbe_setup_copper_link(struct txgbe_hw *hw,
					 u32 speed,
					 bool autoneg_wait_to_complete);
s32 txgbe_check_mac_link(struct txgbe_hw *hw, u32 *speed,
				  bool *link_up, bool link_up_wait_to_complete);



u32 rd32_ephy(struct txgbe_hw *hw, u32 addr)
{
	unsigned int portRegOffset;
	u32 data;

	/* Set the LAN port indicator to portRegOffset[1] */
	/* 1st, write the regOffset to IDA_ADDR register */
	portRegOffset = TXGBE_ETHPHY_IDA_ADDR;
	wr32(hw, portRegOffset, addr);

	/* 2nd, read the data from IDA_DATA register */
	portRegOffset = TXGBE_ETHPHY_IDA_DATA;
	data = rd32(hw, portRegOffset);
	return data;
}


u32 txgbe_rd32_epcs(struct txgbe_hw *hw, u32 addr)
{
	unsigned int portRegOffset;
	u32 data;
	/* Set the LAN port indicator to portRegOffset[1] */
	/* 1st, write the regOffset to IDA_ADDR register */
	portRegOffset = TXGBE_XPCS_IDA_ADDR;
	wr32(hw, portRegOffset, addr);

	/* 2nd, read the data from IDA_DATA register */
	portRegOffset = TXGBE_XPCS_IDA_DATA;
	data = rd32(hw, portRegOffset);

	return data;
}


void txgbe_wr32_ephy(struct txgbe_hw *hw, u32 addr, u32 data)
{
	unsigned int portRegOffset;

	/* Set the LAN port indicator to portRegOffset[1] */
	/* 1st, write the regOffset to IDA_ADDR register */
	portRegOffset = TXGBE_ETHPHY_IDA_ADDR;
	wr32(hw, portRegOffset, addr);

	/* 2nd, read the data from IDA_DATA register */
	portRegOffset = TXGBE_ETHPHY_IDA_DATA;
	wr32(hw, portRegOffset, data);
}

void txgbe_wr32_epcs(struct txgbe_hw *hw, u32 addr, u32 data)
{
	unsigned int portRegOffset;

	/* Set the LAN port indicator to portRegOffset[1] */
	/* 1st, write the regOffset to IDA_ADDR register */
	portRegOffset = TXGBE_XPCS_IDA_ADDR;
	wr32(hw, portRegOffset, addr);

	/* 2nd, read the data from IDA_DATA register */
	portRegOffset = TXGBE_XPCS_IDA_DATA;
	wr32(hw, portRegOffset, data);
}


/**
 * txgbe_dcb_get_rtrup2tc - read rtrup2tc reg
 * @hw: pointer to hardware structure
 * @map: pointer to u8 arr for returning map
 *
 * Read the rtrup2tc HW register and resolve its content into map
 **/
void txgbe_dcb_get_rtrup2tc(struct txgbe_hw *hw, u8 *map)
{
	u32 reg, i;

	reg = rd32(hw, TXGBE_RDB_UP2TC);
	for (i = 0; i < TXGBE_DCB_MAX_USER_PRIORITY; i++)
		map[i] = TXGBE_RDB_UP2TC_UP_MASK &
			(reg >> (i * TXGBE_RDB_UP2TC_UP_SHIFT));
	return;
}

/**
 *  txgbe_get_pcie_msix_count - Gets MSI-X vector count
 *  @hw: pointer to hardware structure
 *
 *  Read PCIe configuration space, and get the MSI-X vector count from
 *  the capabilities table.
 **/
u16 txgbe_get_pcie_msix_count(struct txgbe_hw *hw)
{
	u16 msix_count = 1;
	u16 max_msix_count;
	u32 pos;

	DEBUGFUNC("\n");

	max_msix_count = TXGBE_MAX_MSIX_VECTORS_SAPPHIRE;
	pos = pci_find_capability(((struct txgbe_adapter *)hw->back)->pdev, PCI_CAP_ID_MSIX);
	if (!pos)
		return msix_count;
	pci_read_config_word(((struct txgbe_adapter *)hw->back)->pdev,
			pos + PCI_MSIX_FLAGS, &msix_count);

	if (TXGBE_REMOVED(hw->hw_addr))
		msix_count = 0;
	msix_count &= TXGBE_PCIE_MSIX_TBL_SZ_MASK;

	/* MSI-X count is zero-based in HW */
	msix_count++;

	if (msix_count > max_msix_count)
		msix_count = max_msix_count;

	return msix_count;
}

/**
 *  txgbe_init_hw - Generic hardware initialization
 *  @hw: pointer to hardware structure
 *
 *  Initialize the hardware by resetting the hardware, filling the bus info
 *  structure and media type, clears all on chip counters, initializes receive
 *  address registers, multicast table, VLAN filter table, calls routine to set
 *  up link and flow control settings, and leaves transmit and receive units
 *  disabled and uninitialized
 **/
s32 txgbe_init_hw(struct txgbe_hw *hw)
{
	s32 status;

	DEBUGFUNC("\n");

	/* Reset the hardware */
	status = TCALL(hw, mac.ops.reset_hw);

	if (status == 0) {
		/* Start the HW */
		status = TCALL(hw, mac.ops.start_hw);
	}

	return status;
}


/**
 *  txgbe_clear_hw_cntrs - Generic clear hardware counters
 *  @hw: pointer to hardware structure
 *
 *  Clears all hardware statistics counters by reading them from the hardware
 *  Statistics counters are clear on read.
 **/
s32 txgbe_clear_hw_cntrs(struct txgbe_hw *hw)
{
	u16 i = 0;

	DEBUGFUNC("\n");

	rd32(hw, TXGBE_RX_CRC_ERROR_FRAMES_LOW);
	for (i = 0; i < 8; i++)
		rd32(hw, TXGBE_RDB_MPCNT(i));

	rd32(hw, TXGBE_RX_LEN_ERROR_FRAMES_LOW);
	rd32(hw, TXGBE_RDB_LXONTXC);
	rd32(hw, TXGBE_RDB_LXOFFTXC);
	rd32(hw, TXGBE_MAC_LXONRXC);
	rd32(hw, TXGBE_MAC_LXOFFRXC);

	for (i = 0; i < 8; i++) {
		rd32(hw, TXGBE_RDB_PXONTXC(i));
		rd32(hw, TXGBE_RDB_PXOFFTXC(i));
		rd32(hw, TXGBE_MAC_PXONRXC(i));
		wr32m(hw, TXGBE_MMC_CONTROL, TXGBE_MMC_CONTROL_UP, i<<16);
		rd32(hw, TXGBE_MAC_PXOFFRXC);
	}
	for (i = 0; i < 8; i++)
		rd32(hw, TXGBE_RDB_PXON2OFFCNT(i));
	for (i = 0; i < 128; i++) {
		wr32(hw, TXGBE_PX_MPRC(i), 0);
	}

	rd32(hw, TXGBE_PX_GPRC);
	rd32(hw, TXGBE_PX_GPTC);
	rd32(hw, TXGBE_PX_GORC_MSB);
	rd32(hw, TXGBE_PX_GOTC_MSB);

	rd32(hw, TXGBE_RX_BC_FRAMES_GOOD_LOW);
	rd32(hw, TXGBE_RX_UNDERSIZE_FRAMES_GOOD);
	rd32(hw, TXGBE_RX_OVERSIZE_FRAMES_GOOD);
	rd32(hw, TXGBE_RX_FRAME_CNT_GOOD_BAD_LOW);
	rd32(hw, TXGBE_TX_FRAME_CNT_GOOD_BAD_LOW);
	rd32(hw, TXGBE_TX_MC_FRAMES_GOOD_LOW);
	rd32(hw, TXGBE_TX_BC_FRAMES_GOOD_LOW);
	rd32(hw, TXGBE_RDM_DRP_PKT);
	return 0;
}

/**
 * txgbe_device_supports_autoneg_fc - Check if device supports autonegotiation
 * of flow control
 * @hw: pointer to hardware structure
 *
 * This function returns true if the device supports flow control
 * autonegotiation, and false if it does not.
 *
 **/
bool txgbe_device_supports_autoneg_fc(struct txgbe_hw *hw)
{
	bool supported = false;
	u32 speed;
	bool link_up;
	u8 device_type = hw->subsystem_id & 0xF0;

	DEBUGFUNC("\n");

	switch (hw->phy.media_type) {
	case txgbe_media_type_fiber:
		TCALL(hw, mac.ops.check_link, &speed, &link_up, false);
		/* if link is down, assume supported */
		if (link_up)
			supported = speed == TXGBE_LINK_SPEED_1GB_FULL ?
				true : false;
		else
			supported = true;
		break;
	case txgbe_media_type_backplane:
		supported = (device_type != TXGBE_ID_MAC_XAUI &&
					device_type != TXGBE_ID_MAC_SGMII);
		break;
	case txgbe_media_type_copper:
		/* only some copper devices support flow control autoneg */
		supported = true;
		break;
	default:
		break;
	}

	ERROR_REPORT2(TXGBE_ERROR_UNSUPPORTED,
		      "Device %x does not support flow control autoneg",
		      hw->device_id);
	return supported;
}

/**
 *  txgbe_setup_fc - Set up flow control
 *  @hw: pointer to hardware structure
 *
 *  Called at init time to set up flow control.
 **/
s32 txgbe_setup_fc(struct txgbe_hw *hw)
{
	s32 ret_val = 0;
	u32 pcap = 0;
	u32 value = 0;
	u32 pcap_backplane = 0;

	DEBUGFUNC("\n");

	/* Validate the requested mode */
	if (hw->fc.strict_ieee && hw->fc.requested_mode == txgbe_fc_rx_pause) {
		ERROR_REPORT1(TXGBE_ERROR_UNSUPPORTED,
			   "txgbe_fc_rx_pause not valid in strict IEEE mode\n");
		ret_val = TXGBE_ERR_INVALID_LINK_SETTINGS;
		goto out;
	}

	/*
	 * 10gig parts do not have a word in the EEPROM to determine the
	 * default flow control setting, so we explicitly set it to full.
	 */
	if (hw->fc.requested_mode == txgbe_fc_default)
		hw->fc.requested_mode = txgbe_fc_full;

	/*
	 * Set up the 1G and 10G flow control advertisement registers so the
	 * HW will be able to do fc autoneg once the cable is plugged in.  If
	 * we link at 10G, the 1G advertisement is harmless and vice versa.
	 */

	/*
	 * The possible values of fc.requested_mode are:
	 * 0: Flow control is completely disabled
	 * 1: Rx flow control is enabled (we can receive pause frames,
	 *    but not send pause frames).
	 * 2: Tx flow control is enabled (we can send pause frames but
	 *    we do not support receiving pause frames).
	 * 3: Both Rx and Tx flow control (symmetric) are enabled.
	 * other: Invalid.
	 */
	switch (hw->fc.requested_mode) {
	case txgbe_fc_none:
		/* Flow control completely disabled by software override. */
		break;
	case txgbe_fc_tx_pause:
		/*
		 * Tx Flow control is enabled, and Rx Flow control is
		 * disabled by software override.
		 */
		pcap |= TXGBE_SR_MII_MMD_AN_ADV_PAUSE_ASM;
		pcap_backplane |= TXGBE_SR_AN_MMD_ADV_REG1_PAUSE_ASM;
		break;
	case txgbe_fc_rx_pause:
		/*
		 * Rx Flow control is enabled and Tx Flow control is
		 * disabled by software override. Since there really
		 * isn't a way to advertise that we are capable of RX
		 * Pause ONLY, we will advertise that we support both
		 * symmetric and asymmetric Rx PAUSE, as such we fall
		 * through to the fc_full statement.  Later, we will
		 * disable the adapter's ability to send PAUSE frames.
		 */
	case txgbe_fc_full:
		/* Flow control (both Rx and Tx) is enabled by SW override. */
		pcap |= TXGBE_SR_MII_MMD_AN_ADV_PAUSE_SYM |
			TXGBE_SR_MII_MMD_AN_ADV_PAUSE_ASM;
		pcap_backplane |= TXGBE_SR_AN_MMD_ADV_REG1_PAUSE_SYM |
			TXGBE_SR_AN_MMD_ADV_REG1_PAUSE_ASM;
		break;
	default:
		ERROR_REPORT1(TXGBE_ERROR_ARGUMENT,
			     "Flow control param set incorrectly\n");
		ret_val = TXGBE_ERR_CONFIG;
		goto out;
		break;
	}

	/*
	 * Enable auto-negotiation between the MAC & PHY;
	 * the MAC will advertise clause 37 flow control.
	 */
	value = txgbe_rd32_epcs(hw, TXGBE_SR_MII_MMD_AN_ADV);
	value = (value & ~(TXGBE_SR_MII_MMD_AN_ADV_PAUSE_ASM |
		TXGBE_SR_MII_MMD_AN_ADV_PAUSE_SYM)) | pcap;
	txgbe_wr32_epcs(hw, TXGBE_SR_MII_MMD_AN_ADV, value);

	/*
	 * AUTOC restart handles negotiation of 1G and 10G on backplane
	 * and copper.
	 */
	if (hw->phy.media_type == txgbe_media_type_backplane) {
		value = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_ADV_REG1);
		value = (value & ~(TXGBE_SR_AN_MMD_ADV_REG1_PAUSE_ASM |
			TXGBE_SR_AN_MMD_ADV_REG1_PAUSE_SYM)) |
			pcap_backplane;
		txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_ADV_REG1, value);

	} else if ((hw->phy.media_type == txgbe_media_type_copper) &&
		   (txgbe_device_supports_autoneg_fc(hw))) {
		ret_val = txgbe_set_phy_pause_advertisement(hw, pcap_backplane);
	}
out:
	return ret_val;
}

/**
 *  txgbe_read_pba_string - Reads part number string from EEPROM
 *  @hw: pointer to hardware structure
 *  @pba_num: stores the part number string from the EEPROM
 *  @pba_num_size: part number string buffer length
 *
 *  Reads the part number string from the EEPROM.
 **/
s32 txgbe_read_pba_string(struct txgbe_hw *hw, u8 *pba_num,
				  u32 pba_num_size)
{
	s32 ret_val;
	u16 data;
	u16 pba_ptr;
	u16 offset;
	u16 length;

	DEBUGFUNC("\n");

	if (pba_num == NULL) {
		DEBUGOUT("PBA string buffer was null\n");
		return TXGBE_ERR_INVALID_ARGUMENT;
	}

	ret_val = TCALL(hw, eeprom.ops.read,
			hw->eeprom.sw_region_offset + TXGBE_PBANUM0_PTR,
			&data);
	if (ret_val) {
		DEBUGOUT("NVM Read Error\n");
		return ret_val;
	}

	ret_val = TCALL(hw, eeprom.ops.read,
			hw->eeprom.sw_region_offset + TXGBE_PBANUM1_PTR,
			&pba_ptr);
	if (ret_val) {
		DEBUGOUT("NVM Read Error\n");
		return ret_val;
	}

	/*
	 * if data is not ptr guard the PBA must be in legacy format which
	 * means pba_ptr is actually our second data word for the PBA number
	 * and we can decode it into an ascii string
	 */
	if (data != TXGBE_PBANUM_PTR_GUARD) {
		DEBUGOUT("NVM PBA number is not stored as string\n");

		/* we will need 11 characters to store the PBA */
		if (pba_num_size < 11) {
			DEBUGOUT("PBA string buffer too small\n");
			return TXGBE_ERR_NO_SPACE;
		}

		/* extract hex string from data and pba_ptr */
		pba_num[0] = (data >> 12) & 0xF;
		pba_num[1] = (data >> 8) & 0xF;
		pba_num[2] = (data >> 4) & 0xF;
		pba_num[3] = data & 0xF;
		pba_num[4] = (pba_ptr >> 12) & 0xF;
		pba_num[5] = (pba_ptr >> 8) & 0xF;
		pba_num[6] = '-';
		pba_num[7] = 0;
		pba_num[8] = (pba_ptr >> 4) & 0xF;
		pba_num[9] = pba_ptr & 0xF;

		/* put a null character on the end of our string */
		pba_num[10] = '\0';

		/* switch all the data but the '-' to hex char */
		for (offset = 0; offset < 10; offset++) {
			if (pba_num[offset] < 0xA)
				pba_num[offset] += '0';
			else if (pba_num[offset] < 0x10)
				pba_num[offset] += 'A' - 0xA;
		}

		return 0;
	}

	ret_val = TCALL(hw, eeprom.ops.read, pba_ptr, &length);
	if (ret_val) {
		DEBUGOUT("NVM Read Error\n");
		return ret_val;
	}

	if (length == 0xFFFF || length == 0) {
		DEBUGOUT("NVM PBA number section invalid length\n");
		return TXGBE_ERR_PBA_SECTION;
	}

	/* check if pba_num buffer is big enough */
	if (pba_num_size  < (((u32)length * 2) - 1)) {
		DEBUGOUT("PBA string buffer too small\n");
		return TXGBE_ERR_NO_SPACE;
	}

	/* trim pba length from start of string */
	pba_ptr++;
	length--;

	for (offset = 0; offset < length; offset++) {
		ret_val = TCALL(hw, eeprom.ops.read, pba_ptr + offset, &data);
		if (ret_val) {
			DEBUGOUT("NVM Read Error\n");
			return ret_val;
		}
		pba_num[offset * 2] = (u8)(data >> 8);
		pba_num[(offset * 2) + 1] = (u8)(data & 0xFF);
	}
	pba_num[offset * 2] = '\0';

	return 0;
}

/**
 *  txgbe_get_mac_addr - Generic get MAC address
 *  @hw: pointer to hardware structure
 *  @mac_addr: Adapter MAC address
 *
 *  Reads the adapter's MAC address from first Receive Address Register (RAR0)
 *  A reset of the adapter must be performed prior to calling this function
 *  in order for the MAC address to have been loaded from the EEPROM into RAR0
 **/
s32 txgbe_get_mac_addr(struct txgbe_hw *hw, u8 *mac_addr)
{
	u32 rar_high;
	u32 rar_low;
	u16 i;

	DEBUGFUNC("\n");

#ifdef SIMULATION_DEBUG
	wr32(hw, TXGBE_PSR_MAC_SWC_IDX, 0);
	wr32(hw, TXGBE_PSR_MAC_SWC_AD_H, 0x0202);
	wr32(hw, TXGBE_PSR_MAC_SWC_AD_L, 0x03040506);
#endif

	wr32(hw, TXGBE_PSR_MAC_SWC_IDX, 0);
	rar_high = rd32(hw, TXGBE_PSR_MAC_SWC_AD_H);
	rar_low = rd32(hw, TXGBE_PSR_MAC_SWC_AD_L);

	for (i = 0; i < 2; i++)
		mac_addr[i] = (u8)(rar_high >> (1 - i) * 8);

	for (i = 0; i < 4; i++)
		mac_addr[i + 2] = (u8)(rar_low >> (3 - i) * 8);

	return 0;
}

/**
 *  txgbe_set_pci_config_data - Generic store PCI bus info
 *  @hw: pointer to hardware structure
 *  @link_status: the link status returned by the PCI config space
 *
 *  Stores the PCI bus info (speed, width, type) within the txgbe_hw structure
 **/
void txgbe_set_pci_config_data(struct txgbe_hw *hw, u16 link_status)
{
	if (hw->bus.type == txgbe_bus_type_unknown)
		hw->bus.type = txgbe_bus_type_pci_express;

	switch (link_status & TXGBE_PCI_LINK_WIDTH) {
	case TXGBE_PCI_LINK_WIDTH_1:
		hw->bus.width = txgbe_bus_width_pcie_x1;
		break;
	case TXGBE_PCI_LINK_WIDTH_2:
		hw->bus.width = txgbe_bus_width_pcie_x2;
		break;
	case TXGBE_PCI_LINK_WIDTH_4:
		hw->bus.width = txgbe_bus_width_pcie_x4;
		break;
	case TXGBE_PCI_LINK_WIDTH_8:
		hw->bus.width = txgbe_bus_width_pcie_x8;
		break;
	default:
		hw->bus.width = txgbe_bus_width_unknown;
		break;
	}

	switch (link_status & TXGBE_PCI_LINK_SPEED) {
	case TXGBE_PCI_LINK_SPEED_2500:
		hw->bus.speed = txgbe_bus_speed_2500;
		break;
	case TXGBE_PCI_LINK_SPEED_5000:
		hw->bus.speed = txgbe_bus_speed_5000;
		break;
	case TXGBE_PCI_LINK_SPEED_8000:
		hw->bus.speed = txgbe_bus_speed_8000;
		break;
	default:
		hw->bus.speed = txgbe_bus_speed_unknown;
		break;
	}

}

/**
 *  txgbe_get_bus_info - Generic set PCI bus info
 *  @hw: pointer to hardware structure
 *
 *  Gets the PCI bus info (speed, width, type) then calls helper function to
 *  store this data within the txgbe_hw structure.
 **/
s32 txgbe_get_bus_info(struct txgbe_hw *hw)
{
	u16 link_status;

	DEBUGFUNC("\n");

	/* Get the negotiated link width and speed from PCI config space */
	link_status = TXGBE_READ_PCIE_WORD(hw, TXGBE_PCI_LINK_STATUS);

	txgbe_set_pci_config_data(hw, link_status);

	return 0;
}

/**
 *  txgbe_set_lan_id_multi_port_pcie - Set LAN id for PCIe multiple port devices
 *  @hw: pointer to the HW structure
 *
 *  Determines the LAN function id by reading memory-mapped registers
 *  and swaps the port value if requested.
 **/
void txgbe_set_lan_id_multi_port_pcie(struct txgbe_hw *hw)
{
	struct txgbe_bus_info *bus = &hw->bus;
	u32 reg;

	DEBUGFUNC("\n");

	reg = rd32(hw, TXGBE_CFG_PORT_ST);
	bus->lan_id = TXGBE_CFG_PORT_ST_LAN_ID(reg);

	/* check for a port swap */
	reg = rd32(hw, TXGBE_MIS_PWR);
	if (TXGBE_MIS_PWR_LAN_ID_1 == TXGBE_MIS_PWR_LAN_ID(reg))
		bus->func = 0;
	else
		bus->func = bus->lan_id;
}

/**
 *  txgbe_stop_adapter - Generic stop Tx/Rx units
 *  @hw: pointer to hardware structure
 *
 *  Sets the adapter_stopped flag within txgbe_hw struct. Clears interrupts,
 *  disables transmit and receive units. The adapter_stopped flag is used by
 *  the shared code and drivers to determine if the adapter is in a stopped
 *  state and should not touch the hardware.
 **/
s32 txgbe_stop_adapter(struct txgbe_hw *hw)
{
	u16 i;

	DEBUGFUNC("\n");

	/*
	 * Set the adapter_stopped flag so other driver functions stop touching
	 * the hardware
	 */
	hw->adapter_stopped = true;

	/* Disable the receive unit */
	TCALL(hw, mac.ops.disable_rx);

	/* Set interrupt mask to stop interrupts from being generated */
	txgbe_intr_disable(hw, TXGBE_INTR_ALL);

	/* Clear any pending interrupts, flush previous writes */
	wr32(hw, TXGBE_PX_MISC_IC, 0xffffffff);
	wr32(hw, TXGBE_BME_CTL, 0x3);

	/* Disable the transmit unit.  Each queue must be disabled. */
	for (i = 0; i < hw->mac.max_tx_queues; i++) {
		wr32m(hw, TXGBE_PX_TR_CFG(i),
			TXGBE_PX_TR_CFG_SWFLSH | TXGBE_PX_TR_CFG_ENABLE,
			TXGBE_PX_TR_CFG_SWFLSH);
	}

	/* Disable the receive unit by stopping each queue */
	for (i = 0; i < hw->mac.max_rx_queues; i++) {
		wr32m(hw, TXGBE_PX_RR_CFG(i),
			TXGBE_PX_RR_CFG_RR_EN, 0);
	}

	/* flush all queues disables */
	TXGBE_WRITE_FLUSH(hw);

	/*
	 * Prevent the PCI-E bus from hanging by disabling PCI-E master
	 * access and verify no pending requests
	 */
	return txgbe_disable_pcie_master(hw);
}

/**
 *  txgbe_led_on - Turns on the software controllable LEDs.
 *  @hw: pointer to hardware structure
 *  @index: led number to turn on
 **/
s32 txgbe_led_on(struct txgbe_hw *hw, u32 index)
{
	u32 led_reg = rd32(hw, TXGBE_CFG_LED_CTL);
	u16 value = 0;
	DEBUGFUNC("\n");
	
	if ((hw->subsystem_device_id & 0xF0) == TXGBE_ID_XAUI) {
		txgbe_read_mdio(&hw->phy_dev, hw->phy.addr, 31, 0xF021, &value);
		txgbe_write_mdio(&hw->phy_dev, hw->phy.addr, 31, 0xF021, (value & 0xFFFC) | 0x0);
	}
	/* To turn on the LED, set mode to ON. */
	led_reg |= index | (index << TXGBE_CFG_LED_CTL_LINK_OD_SHIFT);
	wr32(hw, TXGBE_CFG_LED_CTL, led_reg);
	TXGBE_WRITE_FLUSH(hw);

	return 0;
}

/**
 *  txgbe_led_off - Turns off the software controllable LEDs.
 *  @hw: pointer to hardware structure
 *  @index: led number to turn off
 **/
s32 txgbe_led_off(struct txgbe_hw *hw, u32 index)
{
	u32 led_reg = rd32(hw, TXGBE_CFG_LED_CTL);
	u16 value = 0;
	DEBUGFUNC("\n");

	if ((hw->subsystem_device_id & 0xF0) == TXGBE_ID_XAUI) {
		txgbe_read_mdio(&hw->phy_dev, hw->phy.addr, 31, 0xF021, &value);
		txgbe_write_mdio(&hw->phy_dev, hw->phy.addr, 31, 0xF021, (value & 0xFFFC) | 0x1);
	}

	/* To turn off the LED, set mode to OFF. */
	led_reg &= ~(index << TXGBE_CFG_LED_CTL_LINK_OD_SHIFT);
	led_reg |= index;
	wr32(hw, TXGBE_CFG_LED_CTL, led_reg);
	TXGBE_WRITE_FLUSH(hw);
	return 0;
}

/**
 *  txgbe_get_eeprom_semaphore - Get hardware semaphore
 *  @hw: pointer to hardware structure
 *
 *  Sets the hardware semaphores so EEPROM access can occur for bit-bang method
 **/
STATIC s32 txgbe_get_eeprom_semaphore(struct txgbe_hw *hw)
{
	s32 status = TXGBE_ERR_EEPROM;
	u32 timeout = 2000;
	u32 i;
	u32 swsm;

	/* Get SMBI software semaphore between device drivers first */
	for (i = 0; i < timeout; i++) {
		/*
		 * If the SMBI bit is 0 when we read it, then the bit will be
		 * set and we have the semaphore
		 */
		swsm = rd32(hw, TXGBE_MIS_SWSM);
		if (!(swsm & TXGBE_MIS_SWSM_SMBI)) {
			status = 0;
			break;
		}
		usec_delay(50);
	}

	if (i == timeout) {
		DEBUGOUT("Driver can't access the Eeprom - SMBI Semaphore "
			 "not granted.\n");
		/*
		 * this release is particularly important because our attempts
		 * above to get the semaphore may have succeeded, and if there
		 * was a timeout, we should unconditionally clear the semaphore
		 * bits to free the driver to make progress
		 */
		txgbe_release_eeprom_semaphore(hw);

		usec_delay(50);
		/*
		 * one last try
		 * If the SMBI bit is 0 when we read it, then the bit will be
		 * set and we have the semaphore
		 */
		swsm = rd32(hw, TXGBE_MIS_SWSM);
		if (!(swsm & TXGBE_MIS_SWSM_SMBI))
			status = 0;
	}

	/* Now get the semaphore between SW/FW through the SWESMBI bit */
	if (status == 0) {
		for (i = 0; i < timeout; i++) {
			if (txgbe_check_mng_access(hw)) {
			/* Set the SW EEPROM semaphore bit to request access */
				wr32m(hw, TXGBE_MNG_SW_SM,
					TXGBE_MNG_SW_SM_SM, TXGBE_MNG_SW_SM_SM);

				/*
				 * If we set the bit successfully then we got
				 * semaphore.
				 */
				swsm = rd32(hw, TXGBE_MNG_SW_SM);
				if (swsm & TXGBE_MNG_SW_SM_SM)
					break;
			}
			usec_delay(50);
		}

		/*
		 * Release semaphores and return error if SW EEPROM semaphore
		 * was not granted because we don't have access to the EEPROM
		 */
		if (i >= timeout) {
			ERROR_REPORT1(TXGBE_ERROR_POLLING,
			    "SWESMBI Software EEPROM semaphore not granted.\n");
			txgbe_release_eeprom_semaphore(hw);
			status = TXGBE_ERR_EEPROM;
		}
	} else {
		ERROR_REPORT1(TXGBE_ERROR_POLLING,
			     "Software semaphore SMBI between device drivers "
			     "not granted.\n");
	}

	return status;
}

/**
 *  txgbe_release_eeprom_semaphore - Release hardware semaphore
 *  @hw: pointer to hardware structure
 *
 *  This function clears hardware semaphore bits.
 **/
STATIC void txgbe_release_eeprom_semaphore(struct txgbe_hw *hw)
{
	if (txgbe_check_mng_access(hw)) {
		wr32m(hw, TXGBE_MNG_SW_SM,
			TXGBE_MNG_SW_SM_SM, 0);
		wr32m(hw, TXGBE_MIS_SWSM,
			TXGBE_MIS_SWSM_SMBI, 0);
		TXGBE_WRITE_FLUSH(hw);
	}
}

/**
 *  txgbe_validate_mac_addr - Validate MAC address
 *  @mac_addr: pointer to MAC address.
 *
 *  Tests a MAC address to ensure it is a valid Individual Address
 **/
s32 txgbe_validate_mac_addr(u8 *mac_addr)
{
	s32 status = 0;

	DEBUGFUNC("\n");

	/* Make sure it is not a multicast address */
	if (TXGBE_IS_MULTICAST(mac_addr)) {
		DEBUGOUT("MAC address is multicast\n");
		status = TXGBE_ERR_INVALID_MAC_ADDR;
	/* Not a broadcast address */
	} else if (TXGBE_IS_BROADCAST(mac_addr)) {
		DEBUGOUT("MAC address is broadcast\n");
		status = TXGBE_ERR_INVALID_MAC_ADDR;
	/* Reject the zero address */
	} else if (mac_addr[0] == 0 && mac_addr[1] == 0 && mac_addr[2] == 0 &&
		   mac_addr[3] == 0 && mac_addr[4] == 0 && mac_addr[5] == 0) {
		DEBUGOUT("MAC address is all zeros\n");
		status = TXGBE_ERR_INVALID_MAC_ADDR;
	}
	return status;
}

/**
 *  txgbe_set_rar - Set Rx address register
 *  @hw: pointer to hardware structure
 *  @index: Receive address register to write
 *  @addr: Address to put into receive address register
 *  @vmdq: VMDq "set" or "pool" index
 *  @enable_addr: set flag that address is active
 *
 *  Puts an ethernet address into a receive address register.
 **/
s32 txgbe_set_rar(struct txgbe_hw *hw, u32 index, u8 *addr, u64 pools,
			  u32 enable_addr)
{
	u32 rar_low, rar_high;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("\n");

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries) {
		ERROR_REPORT2(TXGBE_ERROR_ARGUMENT,
			     "RAR index %d is out of range.\n", index);
		return TXGBE_ERR_INVALID_ARGUMENT;
	}

	/* select the MAC address */
	wr32(hw, TXGBE_PSR_MAC_SWC_IDX, index);

	/* setup VMDq pool mapping */
	wr32(hw, TXGBE_PSR_MAC_SWC_VM_L, pools & 0xFFFFFFFF);
	wr32(hw, TXGBE_PSR_MAC_SWC_VM_H, pools >> 32);

	/*
	 * HW expects these in little endian so we reverse the byte
	 * order from network order (big endian) to little endian
	 *
	 * Some parts put the VMDq setting in the extra RAH bits,
	 * so save everything except the lower 16 bits that hold part
	 * of the address and the address valid bit.
	 */
	rar_low = ((u32)addr[5] |
		  ((u32)addr[4] << 8) |
		  ((u32)addr[3] << 16) |
		  ((u32)addr[2] << 24));
	rar_high = ((u32)addr[1] |
		   ((u32)addr[0] << 8));
	if (enable_addr != 0)
		rar_high |= TXGBE_PSR_MAC_SWC_AD_H_AV;

	wr32(hw, TXGBE_PSR_MAC_SWC_AD_L, rar_low);
	wr32m(hw, TXGBE_PSR_MAC_SWC_AD_H,
		(TXGBE_PSR_MAC_SWC_AD_H_AD(~0) |
		 TXGBE_PSR_MAC_SWC_AD_H_ADTYPE(~0) |
		 TXGBE_PSR_MAC_SWC_AD_H_AV),
		rar_high);

	return 0;
}

/**
 *  txgbe_clear_rar - Remove Rx address register
 *  @hw: pointer to hardware structure
 *  @index: Receive address register to write
 *
 *  Clears an ethernet address from a receive address register.
 **/
s32 txgbe_clear_rar(struct txgbe_hw *hw, u32 index)
{
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("\n");

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries) {
		ERROR_REPORT2(TXGBE_ERROR_ARGUMENT,
			     "RAR index %d is out of range.\n", index);
		return TXGBE_ERR_INVALID_ARGUMENT;
	}

	/*
	 * Some parts put the VMDq setting in the extra RAH bits,
	 * so save everything except the lower 16 bits that hold part
	 * of the address and the address valid bit.
	 */
	wr32(hw, TXGBE_PSR_MAC_SWC_IDX, index);

	wr32(hw, TXGBE_PSR_MAC_SWC_VM_L, 0);
	wr32(hw, TXGBE_PSR_MAC_SWC_VM_H, 0);

	wr32(hw, TXGBE_PSR_MAC_SWC_AD_L, 0);
	wr32m(hw, TXGBE_PSR_MAC_SWC_AD_H,
		(TXGBE_PSR_MAC_SWC_AD_H_AD(~0) |
		 TXGBE_PSR_MAC_SWC_AD_H_ADTYPE(~0) |
		 TXGBE_PSR_MAC_SWC_AD_H_AV),
		0);

	return 0;
}

/**
 *  txgbe_init_rx_addrs - Initializes receive address filters.
 *  @hw: pointer to hardware structure
 *
 *  Places the MAC address in receive address register 0 and clears the rest
 *  of the receive address registers. Clears the multicast table. Assumes
 *  the receiver is in reset when the routine is called.
 **/
s32 txgbe_init_rx_addrs(struct txgbe_hw *hw)
{
	u32 i;
	u32 rar_entries = hw->mac.num_rar_entries;
	u32 psrctl;

	DEBUGFUNC("\n");

	/*
	 * If the current mac address is valid, assume it is a software override
	 * to the permanent address.
	 * Otherwise, use the permanent address from the eeprom.
	 */
	if (txgbe_validate_mac_addr(hw->mac.addr) ==
		TXGBE_ERR_INVALID_MAC_ADDR) {
		/* Get the MAC address from the RAR0 for later reference */
		TCALL(hw, mac.ops.get_mac_addr, hw->mac.addr);

		DEBUGOUT3(" Keeping Current RAR0 Addr =%.2X %.2X %.2X %.2X %.2X %.2X\n",
			hw->mac.addr[0], hw->mac.addr[1],
			hw->mac.addr[2], hw->mac.addr[3],
			hw->mac.addr[4], hw->mac.addr[5]);
	} else {
		/* Setup the receive address. */
		DEBUGOUT("Overriding MAC Address in RAR[0]\n");
		DEBUGOUT3(" New MAC Addr =%.2X %.2X %.2X %.2X %.2X %.2X\n",
			hw->mac.addr[0], hw->mac.addr[1],
			hw->mac.addr[2], hw->mac.addr[3],
			hw->mac.addr[4], hw->mac.addr[5]);

		TCALL(hw, mac.ops.set_rar, 0, hw->mac.addr, 0,
			TXGBE_PSR_MAC_SWC_AD_H_AV);

		/* clear VMDq pool/queue selection for RAR 0 */
		TCALL(hw, mac.ops.clear_vmdq, 0, TXGBE_CLEAR_VMDQ_ALL);
	}
	hw->addr_ctrl.overflow_promisc = 0;

	hw->addr_ctrl.rar_used_count = 1;

	/* Zero out the other receive addresses. */
	DEBUGOUT1("Clearing RAR[1-%d]\n", rar_entries - 1);
	for (i = 1; i < rar_entries; i++) {
		wr32(hw, TXGBE_PSR_MAC_SWC_IDX, i);
		wr32(hw, TXGBE_PSR_MAC_SWC_AD_L, 0);
		wr32(hw, TXGBE_PSR_MAC_SWC_AD_H, 0);
	}

	/* Clear the MTA */
	hw->addr_ctrl.mta_in_use = 0;
	psrctl = rd32(hw, TXGBE_PSR_CTL);
	psrctl &= ~(TXGBE_PSR_CTL_MO | TXGBE_PSR_CTL_MFE);
	psrctl |= hw->mac.mc_filter_type << TXGBE_PSR_CTL_MO_SHIFT;
	wr32(hw, TXGBE_PSR_CTL, psrctl);
	DEBUGOUT(" Clearing MTA\n");
	for (i = 0; i < hw->mac.mcft_size; i++)
		wr32(hw, TXGBE_PSR_MC_TBL(i), 0);

	TCALL(hw, mac.ops.init_uta_tables);

	return 0;
}

/**
 *  txgbe_add_uc_addr - Adds a secondary unicast address.
 *  @hw: pointer to hardware structure
 *  @addr: new address
 *
 *  Adds it to unused receive address register or goes into promiscuous mode.
 **/
void txgbe_add_uc_addr(struct txgbe_hw *hw, u8 *addr, u32 vmdq)
{
	u32 rar_entries = hw->mac.num_rar_entries;
	u32 rar;

	DEBUGFUNC("\n");

	DEBUGOUT6(" UC Addr = %.2X %.2X %.2X %.2X %.2X %.2X\n",
		  addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	/*
	 * Place this address in the RAR if there is room,
	 * else put the controller into promiscuous mode
	 */
	if (hw->addr_ctrl.rar_used_count < rar_entries) {
		rar = hw->addr_ctrl.rar_used_count;
		TCALL(hw, mac.ops.set_rar, rar, addr, vmdq,
			TXGBE_PSR_MAC_SWC_AD_H_AV);
		DEBUGOUT1("Added a secondary address to RAR[%d]\n", rar);
		hw->addr_ctrl.rar_used_count++;
	} else {
		hw->addr_ctrl.overflow_promisc++;
	}

	DEBUGOUT("txgbe_add_uc_addr Complete\n");
}

/**
 *  txgbe_update_uc_addr_list - Updates MAC list of secondary addresses
 *  @hw: pointer to hardware structure
 *  @addr_list: the list of new addresses
 *  @addr_count: number of addresses
 *  @next: iterator function to walk the address list
 *
 *  The given list replaces any existing list.  Clears the secondary addrs from
 *  receive address registers.  Uses unused receive address registers for the
 *  first secondary addresses, and falls back to promiscuous mode as needed.
 *
 *  Drivers using secondary unicast addresses must set user_set_promisc when
 *  manually putting the device into promiscuous mode.
 **/
s32 txgbe_update_uc_addr_list(struct txgbe_hw *hw, u8 *addr_list,
				      u32 addr_count, txgbe_mc_addr_itr next)
{
	u8 *addr;
	u32 i;
	u32 old_promisc_setting = hw->addr_ctrl.overflow_promisc;
	u32 uc_addr_in_use;
	u32 vmdq;

	DEBUGFUNC("\n");

	/*
	 * Clear accounting of old secondary address list,
	 * don't count RAR[0]
	 */
	uc_addr_in_use = hw->addr_ctrl.rar_used_count - 1;
	hw->addr_ctrl.rar_used_count -= uc_addr_in_use;
	hw->addr_ctrl.overflow_promisc = 0;

	/* Zero out the other receive addresses */
	DEBUGOUT1("Clearing RAR[1-%d]\n", uc_addr_in_use+1);
	for (i = 0; i < uc_addr_in_use; i++) {
		wr32(hw, TXGBE_PSR_MAC_SWC_IDX, 1+i);
		wr32(hw, TXGBE_PSR_MAC_SWC_AD_L, 0);
		wr32(hw, TXGBE_PSR_MAC_SWC_AD_H, 0);
	}

	/* Add the new addresses */
	for (i = 0; i < addr_count; i++) {
		DEBUGOUT(" Adding the secondary addresses:\n");
		addr = next(hw, &addr_list, &vmdq);
		txgbe_add_uc_addr(hw, addr, vmdq);
	}

	if (hw->addr_ctrl.overflow_promisc) {
		/* enable promisc if not already in overflow or set by user */
		if (!old_promisc_setting && !hw->addr_ctrl.user_set_promisc) {
			DEBUGOUT(" Entering address overflow promisc mode\n");
			wr32m(hw, TXGBE_PSR_CTL,
				TXGBE_PSR_CTL_UPE, TXGBE_PSR_CTL_UPE);
		}
	} else {
		/* only disable if set by overflow, not by user */
		if (old_promisc_setting && !hw->addr_ctrl.user_set_promisc) {
			DEBUGOUT(" Leaving address overflow promisc mode\n");
			wr32m(hw, TXGBE_PSR_CTL,
				TXGBE_PSR_CTL_UPE, 0);
		}
	}

	DEBUGOUT("txgbe_update_uc_addr_list Complete\n");
	return 0;
}

/**
 *  txgbe_mta_vector - Determines bit-vector in multicast table to set
 *  @hw: pointer to hardware structure
 *  @mc_addr: the multicast address
 *
 *  Extracts the 12 bits, from a multicast address, to determine which
 *  bit-vector to set in the multicast table. The hardware uses 12 bits, from
 *  incoming rx multicast addresses, to determine the bit-vector to check in
 *  the MTA. Which of the 4 combination, of 12-bits, the hardware uses is set
 *  by the MO field of the MCSTCTRL. The MO field is set during initialization
 *  to mc_filter_type.
 **/
STATIC s32 txgbe_mta_vector(struct txgbe_hw *hw, u8 *mc_addr)
{
	u32 vector = 0;

	DEBUGFUNC("\n");

	switch (hw->mac.mc_filter_type) {
	case 0:   /* use bits [47:36] of the address */
		vector = ((mc_addr[4] >> 4) | (((u16)mc_addr[5]) << 4));
		break;
	case 1:   /* use bits [46:35] of the address */
		vector = ((mc_addr[4] >> 3) | (((u16)mc_addr[5]) << 5));
		break;
	case 2:   /* use bits [45:34] of the address */
		vector = ((mc_addr[4] >> 2) | (((u16)mc_addr[5]) << 6));
		break;
	case 3:   /* use bits [43:32] of the address */
		vector = ((mc_addr[4]) | (((u16)mc_addr[5]) << 8));
		break;
	default:  /* Invalid mc_filter_type */
		DEBUGOUT("MC filter type param set incorrectly\n");
		ASSERT(0);
		break;
	}

	/* vector can only be 12-bits or boundary will be exceeded */
	vector &= 0xFFF;
	return vector;
}

/**
 *  txgbe_set_mta - Set bit-vector in multicast table
 *  @hw: pointer to hardware structure
 *  @hash_value: Multicast address hash value
 *
 *  Sets the bit-vector in the multicast table.
 **/
void txgbe_set_mta(struct txgbe_hw *hw, u8 *mc_addr)
{
	u32 vector;
	u32 vector_bit;
	u32 vector_reg;

	DEBUGFUNC("\n");

	hw->addr_ctrl.mta_in_use++;

	vector = txgbe_mta_vector(hw, mc_addr);
	DEBUGOUT1(" bit-vector = 0x%03X\n", vector);

	/*
	 * The MTA is a register array of 128 32-bit registers. It is treated
	 * like an array of 4096 bits.  We want to set bit
	 * BitArray[vector_value]. So we figure out what register the bit is
	 * in, read it, OR in the new bit, then write back the new value.  The
	 * register is determined by the upper 7 bits of the vector value and
	 * the bit within that register are determined by the lower 5 bits of
	 * the value.
	 */
	vector_reg = (vector >> 5) & 0x7F;
	vector_bit = vector & 0x1F;
	hw->mac.mta_shadow[vector_reg] |= (1 << vector_bit);
}

/**
 *  txgbe_update_mc_addr_list - Updates MAC list of multicast addresses
 *  @hw: pointer to hardware structure
 *  @mc_addr_list: the list of new multicast addresses
 *  @mc_addr_count: number of addresses
 *  @next: iterator function to walk the multicast address list
 *  @clear: flag, when set clears the table beforehand
 *
 *  When the clear flag is set, the given list replaces any existing list.
 *  Hashes the given addresses into the multicast table.
 **/
s32 txgbe_update_mc_addr_list(struct txgbe_hw *hw, u8 *mc_addr_list,
				      u32 mc_addr_count, txgbe_mc_addr_itr next,
				      bool clear)
{
	u32 i;
	u32 vmdq;
	u32 psrctl;

	DEBUGFUNC("\n");

	/*
	 * Set the new number of MC addresses that we are being requested to
	 * use.
	 */
	hw->addr_ctrl.num_mc_addrs = mc_addr_count;
	hw->addr_ctrl.mta_in_use = 0;

	/* Clear mta_shadow */
	if (clear) {
		DEBUGOUT(" Clearing MTA\n");
		memset(&hw->mac.mta_shadow, 0, sizeof(hw->mac.mta_shadow));
	}

	/* Update mta_shadow */
	for (i = 0; i < mc_addr_count; i++) {
		DEBUGOUT(" Adding the multicast addresses:\n");
		txgbe_set_mta(hw, next(hw, &mc_addr_list, &vmdq));
	}

	/* Enable mta */
	for (i = 0; i < hw->mac.mcft_size; i++)
		wr32a(hw, TXGBE_PSR_MC_TBL(0), i,
				      hw->mac.mta_shadow[i]);

	if (hw->addr_ctrl.mta_in_use > 0) {
		psrctl = rd32(hw, TXGBE_PSR_CTL);
		psrctl &= ~(TXGBE_PSR_CTL_MO | TXGBE_PSR_CTL_MFE);
		psrctl |= TXGBE_PSR_CTL_MFE |
			(hw->mac.mc_filter_type << TXGBE_PSR_CTL_MO_SHIFT);
		wr32(hw, TXGBE_PSR_CTL, psrctl);
	}

	DEBUGOUT("txgbe_update_mc_addr_list Complete\n");
	return 0;
}

/**
 *  txgbe_enable_mc - Enable multicast address in RAR
 *  @hw: pointer to hardware structure
 *
 *  Enables multicast address in RAR and the use of the multicast hash table.
 **/
s32 txgbe_enable_mc(struct txgbe_hw *hw)
{
	struct txgbe_addr_filter_info *a = &hw->addr_ctrl;
	u32 psrctl;

	DEBUGFUNC("\n");

	if (a->mta_in_use > 0) {
		psrctl = rd32(hw, TXGBE_PSR_CTL);
		psrctl &= ~(TXGBE_PSR_CTL_MO | TXGBE_PSR_CTL_MFE);
		psrctl |= TXGBE_PSR_CTL_MFE |
			(hw->mac.mc_filter_type << TXGBE_PSR_CTL_MO_SHIFT);
		wr32(hw, TXGBE_PSR_CTL, psrctl);
	}

	return 0;
}

/**
 *  txgbe_disable_mc - Disable multicast address in RAR
 *  @hw: pointer to hardware structure
 *
 *  Disables multicast address in RAR and the use of the multicast hash table.
 **/
s32 txgbe_disable_mc(struct txgbe_hw *hw)
{
	struct txgbe_addr_filter_info *a = &hw->addr_ctrl;
	u32 psrctl;
	DEBUGFUNC("\n");

	if (a->mta_in_use > 0) {
		psrctl = rd32(hw, TXGBE_PSR_CTL);
		psrctl &= ~(TXGBE_PSR_CTL_MO | TXGBE_PSR_CTL_MFE);
		psrctl |= hw->mac.mc_filter_type << TXGBE_PSR_CTL_MO_SHIFT;
		wr32(hw, TXGBE_PSR_CTL, psrctl);
	}

	return 0;
}

/**
 *  txgbe_fc_enable - Enable flow control
 *  @hw: pointer to hardware structure
 *
 *  Enable flow control according to the current settings.
 **/
s32 txgbe_fc_enable(struct txgbe_hw *hw)
{
	s32 ret_val = 0;
	u32 mflcn_reg, fccfg_reg;
	u32 reg;
	u32 fcrtl, fcrth;
	int i;

	DEBUGFUNC("\n");


	/* Validate the water mark configuration */
	if (!hw->fc.pause_time) {
		ret_val = TXGBE_ERR_INVALID_LINK_SETTINGS;
		goto out;
	}

	/* Low water mark of zero causes XOFF floods */
	for (i = 0; i < TXGBE_DCB_MAX_TRAFFIC_CLASS; i++) {
		if ((hw->fc.current_mode & txgbe_fc_tx_pause) &&
		    hw->fc.high_water[i]) {
			if (!hw->fc.low_water[i] ||
			    hw->fc.low_water[i] >= hw->fc.high_water[i]) {
				DEBUGOUT("Invalid water mark configuration\n");
				ret_val = TXGBE_ERR_INVALID_LINK_SETTINGS;
				goto out;
			}
		}
	}

#ifndef SIMULATION_DEBUG
	/* Negotiate the fc mode to use */
	txgbe_fc_autoneg(hw);
#endif
	/* Disable any previous flow control settings */
	mflcn_reg = rd32(hw, TXGBE_MAC_RX_FLOW_CTRL);
	mflcn_reg &= ~(TXGBE_MAC_RX_FLOW_CTRL_PFCE |
		TXGBE_MAC_RX_FLOW_CTRL_RFE);

	fccfg_reg = rd32(hw, TXGBE_RDB_RFCC);
	fccfg_reg &= ~(TXGBE_RDB_RFCC_RFCE_802_3X |
		TXGBE_RDB_RFCC_RFCE_PRIORITY);

	/*
	 * The possible values of fc.current_mode are:
	 * 0: Flow control is completely disabled
	 * 1: Rx flow control is enabled (we can receive pause frames,
	 *    but not send pause frames).
	 * 2: Tx flow control is enabled (we can send pause frames but
	 *    we do not support receiving pause frames).
	 * 3: Both Rx and Tx flow control (symmetric) are enabled.
	 * other: Invalid.
	 */
	switch (hw->fc.current_mode) {
	case txgbe_fc_none:
		/*
		 * Flow control is disabled by software override or autoneg.
		 * The code below will actually disable it in the HW.
		 */
		break;
	case txgbe_fc_rx_pause:
		/*
		 * Rx Flow control is enabled and Tx Flow control is
		 * disabled by software override. Since there really
		 * isn't a way to advertise that we are capable of RX
		 * Pause ONLY, we will advertise that we support both
		 * symmetric and asymmetric Rx PAUSE.  Later, we will
		 * disable the adapter's ability to send PAUSE frames.
		 */
		mflcn_reg |= TXGBE_MAC_RX_FLOW_CTRL_RFE;
		break;
	case txgbe_fc_tx_pause:
		/*
		 * Tx Flow control is enabled, and Rx Flow control is
		 * disabled by software override.
		 */
		fccfg_reg |= TXGBE_RDB_RFCC_RFCE_802_3X;
		break;
	case txgbe_fc_full:
		/* Flow control (both Rx and Tx) is enabled by SW override. */
		mflcn_reg |= TXGBE_MAC_RX_FLOW_CTRL_RFE;
		fccfg_reg |= TXGBE_RDB_RFCC_RFCE_802_3X;
		break;
	default:
		ERROR_REPORT1(TXGBE_ERROR_ARGUMENT,
			     "Flow control param set incorrectly\n");
		ret_val = TXGBE_ERR_CONFIG;
		goto out;
		break;
	}

	/* Set 802.3x based flow control settings. */
	wr32(hw, TXGBE_MAC_RX_FLOW_CTRL, mflcn_reg);
	wr32(hw, TXGBE_RDB_RFCC, fccfg_reg);


	/* Set up and enable Rx high/low water mark thresholds, enable XON. */
	for (i = 0; i < TXGBE_DCB_MAX_TRAFFIC_CLASS; i++) {
		if ((hw->fc.current_mode & txgbe_fc_tx_pause) &&
		    hw->fc.high_water[i]) {
			fcrtl = (hw->fc.low_water[i] << 10) |
				TXGBE_RDB_RFCL_XONE;
			wr32(hw, TXGBE_RDB_RFCL(i), fcrtl);
			fcrth = (hw->fc.high_water[i] << 10) |
				TXGBE_RDB_RFCH_XOFFE;
		} else {
			wr32(hw, TXGBE_RDB_RFCL(i), 0);
			/*
			 * In order to prevent Tx hangs when the internal Tx
			 * switch is enabled we must set the high water mark
			 * to the Rx packet buffer size - 24KB.  This allows
			 * the Tx switch to function even under heavy Rx
			 * workloads.
			 */
			fcrth = rd32(hw, TXGBE_RDB_PB_SZ(i)) - 24576;
		}

		wr32(hw, TXGBE_RDB_RFCH(i), fcrth);
	}

	/* Configure pause time (2 TCs per register) */
	reg = hw->fc.pause_time * 0x00010001;
	for (i = 0; i < (TXGBE_DCB_MAX_TRAFFIC_CLASS / 2); i++)
		wr32(hw, TXGBE_RDB_RFCV(i), reg);

	/* Configure flow control refresh threshold value */
	wr32(hw, TXGBE_RDB_RFCRT, hw->fc.pause_time / 2);

out:
	return ret_val;
}

/**
 *  txgbe_negotiate_fc - Negotiate flow control
 *  @hw: pointer to hardware structure
 *  @adv_reg: flow control advertised settings
 *  @lp_reg: link partner's flow control settings
 *  @adv_sym: symmetric pause bit in advertisement
 *  @adv_asm: asymmetric pause bit in advertisement
 *  @lp_sym: symmetric pause bit in link partner advertisement
 *  @lp_asm: asymmetric pause bit in link partner advertisement
 *
 *  Find the intersection between advertised settings and link partner's
 *  advertised settings
 **/
STATIC s32 txgbe_negotiate_fc(struct txgbe_hw *hw, u32 adv_reg, u32 lp_reg,
			      u32 adv_sym, u32 adv_asm, u32 lp_sym, u32 lp_asm)
{
	if ((!(adv_reg)) ||  (!(lp_reg))) {
		ERROR_REPORT3(TXGBE_ERROR_UNSUPPORTED,
			     "Local or link partner's advertised flow control "
			     "settings are NULL. Local: %x, link partner: %x\n",
			     adv_reg, lp_reg);
		return TXGBE_ERR_FC_NOT_NEGOTIATED;
	}

	if ((adv_reg & adv_sym) && (lp_reg & lp_sym)) {
		/*
		 * Now we need to check if the user selected Rx ONLY
		 * of pause frames.  In this case, we had to advertise
		 * FULL flow control because we could not advertise RX
		 * ONLY. Hence, we must now check to see if we need to
		 * turn OFF the TRANSMISSION of PAUSE frames.
		 */
		if (hw->fc.requested_mode == txgbe_fc_full) {
			hw->fc.current_mode = txgbe_fc_full;
			DEBUGOUT("Flow Control = FULL.\n");
		} else {
			hw->fc.current_mode = txgbe_fc_rx_pause;
			DEBUGOUT("Flow Control=RX PAUSE frames only\n");
		}
	} else if (!(adv_reg & adv_sym) && (adv_reg & adv_asm) &&
		   (lp_reg & lp_sym) && (lp_reg & lp_asm)) {
		hw->fc.current_mode = txgbe_fc_tx_pause;
		DEBUGOUT("Flow Control = TX PAUSE frames only.\n");
	} else if ((adv_reg & adv_sym) && (adv_reg & adv_asm) &&
		   !(lp_reg & lp_sym) && (lp_reg & lp_asm)) {
		hw->fc.current_mode = txgbe_fc_rx_pause;
		DEBUGOUT("Flow Control = RX PAUSE frames only.\n");
	} else {
		hw->fc.current_mode = txgbe_fc_none;
		DEBUGOUT("Flow Control = NONE.\n");
	}
	return 0;
}

/**
 *  txgbe_fc_autoneg_fiber - Enable flow control on 1 gig fiber
 *  @hw: pointer to hardware structure
 *
 *  Enable flow control according on 1 gig fiber.
 **/
STATIC s32 txgbe_fc_autoneg_fiber(struct txgbe_hw *hw)
{

	u32 pcs_anadv_reg, pcs_lpab_reg;
	s32 ret_val = TXGBE_ERR_FC_NOT_NEGOTIATED;


	pcs_anadv_reg = txgbe_rd32_epcs(hw, TXGBE_SR_MII_MMD_AN_ADV);
	pcs_lpab_reg = txgbe_rd32_epcs(hw, TXGBE_SR_MII_MMD_LP_BABL);

	ret_val =  txgbe_negotiate_fc(hw, pcs_anadv_reg,
				      pcs_lpab_reg,
				      TXGBE_SR_MII_MMD_AN_ADV_PAUSE_SYM,
				      TXGBE_SR_MII_MMD_AN_ADV_PAUSE_ASM,
				      TXGBE_SR_MII_MMD_AN_ADV_PAUSE_SYM,
				      TXGBE_SR_MII_MMD_AN_ADV_PAUSE_ASM);

	return ret_val;
}

/**
 *  txgbe_fc_autoneg_backplane - Enable flow control IEEE clause 37
 *  @hw: pointer to hardware structure
 *
 *  Enable flow control according to IEEE clause 37.
 **/
STATIC s32 txgbe_fc_autoneg_backplane(struct txgbe_hw *hw)
{
	u32 anlp1_reg, autoc_reg;
	s32 ret_val = TXGBE_ERR_FC_NOT_NEGOTIATED;

	/*
	 * Read the 10g AN autoc and LP ability registers and resolve
	 * local flow control settings accordingly
	 */
	autoc_reg = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_ADV_REG1);
	anlp1_reg = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_LP_ABL1);

	ret_val = txgbe_negotiate_fc(hw, autoc_reg,
		anlp1_reg, TXGBE_SR_AN_MMD_ADV_REG1_PAUSE_SYM,
		TXGBE_SR_AN_MMD_ADV_REG1_PAUSE_ASM,
		TXGBE_SR_AN_MMD_ADV_REG1_PAUSE_SYM,
		TXGBE_SR_AN_MMD_ADV_REG1_PAUSE_ASM);

	return ret_val;
}

/**
 *  txgbe_fc_autoneg_copper - Enable flow control IEEE clause 37
 *  @hw: pointer to hardware structure
 *
 *  Enable flow control according to IEEE clause 37.
 **/
STATIC s32 txgbe_fc_autoneg_copper(struct txgbe_hw *hw)
{
	u8 technology_ability_reg = 0;
	u8 lp_technology_ability_reg = 0;

	txgbe_get_phy_advertised_pause(hw, &technology_ability_reg);
	txgbe_get_lp_advertised_pause(hw, &lp_technology_ability_reg);

	return txgbe_negotiate_fc(hw, (u32)technology_ability_reg,
				  (u32)lp_technology_ability_reg,
				  TXGBE_TAF_SYM_PAUSE, TXGBE_TAF_ASM_PAUSE,
				  TXGBE_TAF_SYM_PAUSE, TXGBE_TAF_ASM_PAUSE);
}

/**
 *  txgbe_fc_autoneg - Configure flow control
 *  @hw: pointer to hardware structure
 *
 *  Compares our advertised flow control capabilities to those advertised by
 *  our link partner, and determines the proper flow control mode to use.
 **/
void txgbe_fc_autoneg(struct txgbe_hw *hw)
{
	s32 ret_val = TXGBE_ERR_FC_NOT_NEGOTIATED;
	u32 speed;
	bool link_up;

	DEBUGFUNC("\n");

	/*
	 * AN should have completed when the cable was plugged in.
	 * Look for reasons to bail out.  Bail out if:
	 * - FC autoneg is disabled, or if
	 * - link is not up.
	 */
	if (hw->fc.disable_fc_autoneg) {
		ERROR_REPORT1(TXGBE_ERROR_UNSUPPORTED,
			     "Flow control autoneg is disabled");
		goto out;
	}

	TCALL(hw, mac.ops.check_link, &speed, &link_up, false);
	if (!link_up) {
		ERROR_REPORT1(TXGBE_ERROR_SOFTWARE, "The link is down");
		goto out;
	}

	switch (hw->phy.media_type) {
	/* Autoneg flow control on fiber adapters */
	case txgbe_media_type_fiber:
		if (speed == TXGBE_LINK_SPEED_1GB_FULL)
			ret_val = txgbe_fc_autoneg_fiber(hw);
		break;

	/* Autoneg flow control on backplane adapters */
	case txgbe_media_type_backplane:
		ret_val = txgbe_fc_autoneg_backplane(hw);
		break;

	/* Autoneg flow control on copper adapters */
	case txgbe_media_type_copper:
		if (txgbe_device_supports_autoneg_fc(hw))
			ret_val = txgbe_fc_autoneg_copper(hw);
		break;

	default:
		break;
	}

out:
	if (ret_val == 0) {
		hw->fc.fc_was_autonegged = true;
	} else {
		hw->fc.fc_was_autonegged = false;
		hw->fc.current_mode = hw->fc.requested_mode;
	}
}

/**
 *  txgbe_disable_pcie_master - Disable PCI-express master access
 *  @hw: pointer to hardware structure
 *
 *  Disables PCI-Express master access and verifies there are no pending
 *  requests. TXGBE_ERR_MASTER_REQUESTS_PENDING is returned if master disable
 *  bit hasn't caused the master requests to be disabled, else 0
 *  is returned signifying master requests disabled.
 **/
s32 txgbe_disable_pcie_master(struct txgbe_hw *hw)
{
	s32 status = 0;
	u32 i;
	struct txgbe_adapter *adapter = hw->back;
	unsigned int num_vfs = adapter->num_vfs;
	u16 dev_ctl;
	u32 vf_bme_clear = 0;

	DEBUGFUNC("\n");

	/* Always set this bit to ensure any future transactions are blocked */
	pci_clear_master(((struct txgbe_adapter *)hw->back)->pdev);

	/* Exit if master requests are blocked */
	if (!(rd32(hw, TXGBE_PX_TRANSACTION_PENDING)) ||
			TXGBE_REMOVED(hw->hw_addr))
		goto out;

	/* BME disable handshake will not be finished if any VF BME is 0 */
	for (i = 0; i < num_vfs; i++) {
		struct pci_dev *vfdev = adapter->vfinfo[i].vfdev;
		if (!vfdev) continue;
		pci_read_config_word(vfdev, 0x4, &dev_ctl);
		if ((dev_ctl & 0x4) == 0) {
			vf_bme_clear = 1;
		break;
		}
	}

	/* Poll for master request bit to clear */
	for (i = 0; i < TXGBE_PCI_MASTER_DISABLE_TIMEOUT; i++) {
		usec_delay(100);
		if (!(rd32(hw, TXGBE_PX_TRANSACTION_PENDING)))
			goto out;
	}

	if(!vf_bme_clear) {
		ERROR_REPORT1(TXGBE_ERROR_POLLING,
				 "PCIe transaction pending bit did not clear.\n");
		status = TXGBE_ERR_MASTER_REQUESTS_PENDING;
	}

out:
	return status;
}


/**
 *  txgbe_acquire_swfw_sync - Acquire SWFW semaphore
 *  @hw: pointer to hardware structure
 *  @mask: Mask to specify which semaphore to acquire
 *
 *  Acquires the SWFW semaphore through the GSSR register for the specified
 *  function (CSR, PHY0, PHY1, EEPROM, Flash)
 **/
s32 txgbe_acquire_swfw_sync(struct txgbe_hw *hw, u32 mask)
{
	u32 gssr = 0;
	u32 swmask = mask;
	u32 fwmask = mask << 16;
	u32 timeout = 200;
	u32 i;

	for (i = 0; i < timeout; i++) {
		/*
		 * SW NVM semaphore bit is used for access to all
		 * SW_FW_SYNC bits (not just NVM)
		 */
		if (txgbe_get_eeprom_semaphore(hw))
			return TXGBE_ERR_SWFW_SYNC;

		if (txgbe_check_mng_access(hw)) {
			gssr = rd32(hw, TXGBE_MNG_SWFW_SYNC);
			if (!(gssr & (fwmask | swmask))) {
				gssr |= swmask;
				wr32(hw, TXGBE_MNG_SWFW_SYNC, gssr);
				txgbe_release_eeprom_semaphore(hw);
				return 0;
			} else {
				/* Resource is currently in use by FW or SW */
				txgbe_release_eeprom_semaphore(hw);
				msec_delay(5);
			}
		}
	}

	/* If time expired clear the bits holding the lock and retry */
	if (gssr & (fwmask | swmask))
		txgbe_release_swfw_sync(hw, gssr & (fwmask | swmask));

	msec_delay(5);
	return TXGBE_ERR_SWFW_SYNC;
}

/**
 *  txgbe_release_swfw_sync - Release SWFW semaphore
 *  @hw: pointer to hardware structure
 *  @mask: Mask to specify which semaphore to release
 *
 *  Releases the SWFW semaphore through the GSSR register for the specified
 *  function (CSR, PHY0, PHY1, EEPROM, Flash)
 **/
void txgbe_release_swfw_sync(struct txgbe_hw *hw, u32 mask)
{
	txgbe_get_eeprom_semaphore(hw);
	if (txgbe_check_mng_access(hw))
		wr32m(hw, TXGBE_MNG_SWFW_SYNC, mask, 0);

	txgbe_release_eeprom_semaphore(hw);
}

/**
 *  txgbe_disable_sec_rx_path - Stops the receive data path
 *  @hw: pointer to hardware structure
 *
 *  Stops the receive data path and waits for the HW to internally empty
 *  the Rx security block
 **/
s32 txgbe_disable_sec_rx_path(struct txgbe_hw *hw)
{
#define TXGBE_MAX_SECRX_POLL 40

	int i;
	int secrxreg;

	DEBUGFUNC("\n");

	wr32m(hw, TXGBE_RSC_CTL,
		TXGBE_RSC_CTL_RX_DIS, TXGBE_RSC_CTL_RX_DIS);
	for (i = 0; i < TXGBE_MAX_SECRX_POLL; i++) {
		secrxreg = rd32(hw, TXGBE_RSC_ST);
		if (secrxreg & TXGBE_RSC_ST_RSEC_RDY)
			break;
		else
			/* Use interrupt-safe sleep just in case */
			usec_delay(1000);
	}

	/* For informational purposes only */
	if (i >= TXGBE_MAX_SECRX_POLL)
		DEBUGOUT("Rx unit being enabled before security "
			 "path fully disabled.  Continuing with init.\n");

	return 0;
}

/**
 *  txgbe_enable_sec_rx_path - Enables the receive data path
 *  @hw: pointer to hardware structure
 *
 *  Enables the receive data path.
 **/
s32 txgbe_enable_sec_rx_path(struct txgbe_hw *hw)
{
	DEBUGFUNC("\n");

	wr32m(hw, TXGBE_RSC_CTL,
		TXGBE_RSC_CTL_RX_DIS, 0);
	TXGBE_WRITE_FLUSH(hw);

	return 0;
}

/**
 *  txgbe_get_san_mac_addr_offset - Get SAN MAC address offset from the EEPROM
 *  @hw: pointer to hardware structure
 *  @san_mac_offset: SAN MAC address offset
 *
 *  This function will read the EEPROM location for the SAN MAC address
 *  pointer, and returns the value at that location.  This is used in both
 *  get and set mac_addr routines.
 **/
STATIC s32 txgbe_get_san_mac_addr_offset(struct txgbe_hw *hw,
					 u16 *san_mac_offset)
{
	s32 ret_val;

	DEBUGFUNC("\n");

	/*
	 * First read the EEPROM pointer to see if the MAC addresses are
	 * available.
	 */
	ret_val = TCALL(hw, eeprom.ops.read,
			hw->eeprom.sw_region_offset + TXGBE_SAN_MAC_ADDR_PTR,
			san_mac_offset);
	if (ret_val) {
		ERROR_REPORT2(TXGBE_ERROR_INVALID_STATE,
			      "eeprom at offset %d failed",
			      TXGBE_SAN_MAC_ADDR_PTR);
	}

	return ret_val;
}

/**
 *  txgbe_get_san_mac_addr - SAN MAC address retrieval from the EEPROM
 *  @hw: pointer to hardware structure
 *  @san_mac_addr: SAN MAC address
 *
 *  Reads the SAN MAC address from the EEPROM, if it's available.  This is
 *  per-port, so set_lan_id() must be called before reading the addresses.
 *  set_lan_id() is called by identify_sfp(), but this cannot be relied
 *  upon for non-SFP connections, so we must call it here.
 **/
s32 txgbe_get_san_mac_addr(struct txgbe_hw *hw, u8 *san_mac_addr)
{
	u16 san_mac_data, san_mac_offset;
	u8 i;
	s32 ret_val;

	DEBUGFUNC("\n");

	/*
	 * First read the EEPROM pointer to see if the MAC addresses are
	 * available.  If they're not, no point in calling set_lan_id() here.
	 */
	ret_val = txgbe_get_san_mac_addr_offset(hw, &san_mac_offset);
	if (ret_val || san_mac_offset == 0 || san_mac_offset == 0xFFFF)
		goto san_mac_addr_out;

	/* apply the port offset to the address offset */
	(hw->bus.func) ? (san_mac_offset += TXGBE_SAN_MAC_ADDR_PORT1_OFFSET) :
			 (san_mac_offset += TXGBE_SAN_MAC_ADDR_PORT0_OFFSET);
	for (i = 0; i < 3; i++) {
		ret_val = TCALL(hw, eeprom.ops.read, san_mac_offset,
					      &san_mac_data);
		if (ret_val) {
			ERROR_REPORT2(TXGBE_ERROR_INVALID_STATE,
				      "eeprom read at offset %d failed",
				      san_mac_offset);
			goto san_mac_addr_out;
		}
		san_mac_addr[i * 2] = (u8)(san_mac_data);
		san_mac_addr[i * 2 + 1] = (u8)(san_mac_data >> 8);
		san_mac_offset++;
	}
	return 0;

san_mac_addr_out:
	/*
	 * No addresses available in this EEPROM.  It's not an
	 * error though, so just wipe the local address and return.
	 */
	for (i = 0; i < 6; i++)
		san_mac_addr[i] = 0xFF;
	return 0;
}

/**
 *  txgbe_set_san_mac_addr - Write the SAN MAC address to the EEPROM
 *  @hw: pointer to hardware structure
 *  @san_mac_addr: SAN MAC address
 *
 *  Write a SAN MAC address to the EEPROM.
 **/
s32 txgbe_set_san_mac_addr(struct txgbe_hw *hw, u8 *san_mac_addr)
{
	s32 ret_val;
	u16 san_mac_data, san_mac_offset;
	u8 i;

	DEBUGFUNC("\n");

	/* Look for SAN mac address pointer.  If not defined, return */
	ret_val = txgbe_get_san_mac_addr_offset(hw, &san_mac_offset);
	if (ret_val || san_mac_offset == 0 || san_mac_offset == 0xFFFF)
		return TXGBE_ERR_NO_SAN_ADDR_PTR;

	/* Apply the port offset to the address offset */
	(hw->bus.func) ? (san_mac_offset += TXGBE_SAN_MAC_ADDR_PORT1_OFFSET) :
			 (san_mac_offset += TXGBE_SAN_MAC_ADDR_PORT0_OFFSET);

	for (i = 0; i < 3; i++) {
		san_mac_data = (u16)((u16)(san_mac_addr[i * 2 + 1]) << 8);
		san_mac_data |= (u16)(san_mac_addr[i * 2]);
		TCALL(hw, eeprom.ops.write, san_mac_offset, san_mac_data);
		san_mac_offset++;
	}

	return 0;
}


/**
 *  txgbe_insert_mac_addr - Find a RAR for this mac address
 *  @hw: pointer to hardware structure
 *  @addr: Address to put into receive address register
 *  @vmdq: VMDq pool to assign
 *
 *  Puts an ethernet address into a receive address register, or
 *  finds the rar that it is aleady in; adds to the pool list
 **/
s32 txgbe_insert_mac_addr(struct txgbe_hw *hw, u8 *addr, u32 vmdq)
{
	static const u32 NO_EMPTY_RAR_FOUND = 0xFFFFFFFF;
	u32 first_empty_rar = NO_EMPTY_RAR_FOUND;
	u32 rar;
	u32 rar_low, rar_high;
	u32 addr_low, addr_high;

	DEBUGFUNC("\n");

	/* swap bytes for HW little endian */
	addr_low  = addr[5] | (addr[4] << 8)
			    | (addr[3] << 16)
			    | (addr[2] << 24);
	addr_high = addr[1] | (addr[0] << 8);

	/*
	 * Either find the mac_id in rar or find the first empty space.
	 * rar_highwater points to just after the highest currently used
	 * rar in order to shorten the search.  It grows when we add a new
	 * rar to the top.
	 */
	for (rar = 0; rar < hw->mac.rar_highwater; rar++) {
		wr32(hw, TXGBE_PSR_MAC_SWC_IDX, rar);
		rar_high = rd32(hw, TXGBE_PSR_MAC_SWC_AD_H);

		if (((TXGBE_PSR_MAC_SWC_AD_H_AV & rar_high) == 0)
		    && first_empty_rar == NO_EMPTY_RAR_FOUND) {
			first_empty_rar = rar;
		} else if ((rar_high & 0xFFFF) == addr_high) {
			rar_low = rd32(hw, TXGBE_PSR_MAC_SWC_AD_L);
			if (rar_low == addr_low)
				break;    /* found it already in the rars */
		}
	}

	if (rar < hw->mac.rar_highwater) {
		/* already there so just add to the pool bits */
		TCALL(hw, mac.ops.set_vmdq, rar, vmdq);
	} else if (first_empty_rar != NO_EMPTY_RAR_FOUND) {
		/* stick it into first empty RAR slot we found */
		rar = first_empty_rar;
		TCALL(hw, mac.ops.set_rar, rar, addr, vmdq,
			       TXGBE_PSR_MAC_SWC_AD_H_AV);
	} else if (rar == hw->mac.rar_highwater) {
		/* add it to the top of the list and inc the highwater mark */
		TCALL(hw, mac.ops.set_rar, rar, addr, vmdq,
			       TXGBE_PSR_MAC_SWC_AD_H_AV);
		hw->mac.rar_highwater++;
	} else if (rar >= hw->mac.num_rar_entries) {
		return TXGBE_ERR_INVALID_MAC_ADDR;
	}

	/*
	 * If we found rar[0], make sure the default pool bit (we use pool 0)
	 * remains cleared to be sure default pool packets will get delivered
	 */
	if (rar == 0)
		TCALL(hw, mac.ops.clear_vmdq, rar, 0);

	return rar;
}

/**
 *  txgbe_clear_vmdq - Disassociate a VMDq pool index from a rx address
 *  @hw: pointer to hardware struct
 *  @rar: receive address register index to disassociate
 *  @vmdq: VMDq pool index to remove from the rar
 **/
s32 txgbe_clear_vmdq(struct txgbe_hw *hw, u32 rar, u32 vmdq)
{
	u32 mpsar_lo, mpsar_hi;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("\n");
	UNREFERENCED_PARAMETER(vmdq);

	/* Make sure we are using a valid rar index range */
	if (rar >= rar_entries) {
		ERROR_REPORT2(TXGBE_ERROR_ARGUMENT,
			     "RAR index %d is out of range.\n", rar);
		return TXGBE_ERR_INVALID_ARGUMENT;
	}

	wr32(hw, TXGBE_PSR_MAC_SWC_IDX, rar);
	mpsar_lo = rd32(hw, TXGBE_PSR_MAC_SWC_VM_L);
	mpsar_hi = rd32(hw, TXGBE_PSR_MAC_SWC_VM_H);

	if (TXGBE_REMOVED(hw->hw_addr))
		goto done;

	if (!mpsar_lo && !mpsar_hi)
		goto done;

	/* was that the last pool using this rar? */
	if (mpsar_lo == 0 && mpsar_hi == 0 && rar != 0)
		TCALL(hw, mac.ops.clear_rar, rar);
done:
	return 0;
}

/**
 *  txgbe_set_vmdq - Associate a VMDq pool index with a rx address
 *  @hw: pointer to hardware struct
 *  @rar: receive address register index to associate with a VMDq index
 *  @vmdq: VMDq pool index
 **/
s32 txgbe_set_vmdq(struct txgbe_hw *hw, u32 rar, u32 pool)
{
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("\n");
	UNREFERENCED_PARAMETER(pool);

	/* Make sure we are using a valid rar index range */
	if (rar >= rar_entries) {
		ERROR_REPORT2(TXGBE_ERROR_ARGUMENT,
			     "RAR index %d is out of range.\n", rar);
		return TXGBE_ERR_INVALID_ARGUMENT;
	}

	return 0;
}

/**
 *  This function should only be involved in the IOV mode.
 *  In IOV mode, Default pool is next pool after the number of
 *  VFs advertized and not 0.
 *  MPSAR table needs to be updated for SAN_MAC RAR [hw->mac.san_mac_rar_index]
 *
 *  txgbe_set_vmdq_san_mac - Associate default VMDq pool index with a rx address
 *  @hw: pointer to hardware struct
 *  @vmdq: VMDq pool index
 **/
s32 txgbe_set_vmdq_san_mac(struct txgbe_hw *hw, u32 vmdq)
{
	u32 rar = hw->mac.san_mac_rar_index;

	DEBUGFUNC("\n");

	wr32(hw, TXGBE_PSR_MAC_SWC_IDX, rar);
	if (vmdq < 32) {
		wr32(hw, TXGBE_PSR_MAC_SWC_VM_L, 1 << vmdq);
		wr32(hw, TXGBE_PSR_MAC_SWC_VM_H, 0);
	} else {
		wr32(hw, TXGBE_PSR_MAC_SWC_VM_L, 0);
		wr32(hw, TXGBE_PSR_MAC_SWC_VM_H, 1 << (vmdq - 32));
	}

	return 0;
}

/**
 *  txgbe_init_uta_tables - Initialize the Unicast Table Array
 *  @hw: pointer to hardware structure
 **/
s32 txgbe_init_uta_tables(struct txgbe_hw *hw)
{
	int i;

	DEBUGFUNC("\n");
	DEBUGOUT(" Clearing UTA\n");

	for (i = 0; i < 128; i++)
		wr32(hw, TXGBE_PSR_UC_TBL(i), 0);

	return 0;
}

/**
 *  txgbe_find_vlvf_slot - find the vlanid or the first empty slot
 *  @hw: pointer to hardware structure
 *  @vlan: VLAN id to write to VLAN filter
 *
 *  return the VLVF index where this VLAN id should be placed
 *
 **/
s32 txgbe_find_vlvf_slot(struct txgbe_hw *hw, u32 vlan)
{
	u32 bits = 0;
	u32 first_empty_slot = 0;
	s32 regindex;

	/* short cut the special case */
	if (vlan == 0)
		return 0;

	/*
	 * Search for the vlan id in the VLVF entries. Save off the first empty
	 * slot found along the way
	 */
	for (regindex = 1; regindex < TXGBE_PSR_VLAN_SWC_ENTRIES; regindex++) {
		wr32(hw, TXGBE_PSR_VLAN_SWC_IDX, regindex);
		bits = rd32(hw, TXGBE_PSR_VLAN_SWC);
		if (!bits && !(first_empty_slot))
			first_empty_slot = regindex;
		else if ((bits & 0x0FFF) == vlan)
			break;
	}

	/*
	 * If regindex is less than TXGBE_VLVF_ENTRIES, then we found the vlan
	 * in the VLVF. Else use the first empty VLVF register for this
	 * vlan id.
	 */
	if (regindex >= TXGBE_PSR_VLAN_SWC_ENTRIES) {
		if (first_empty_slot)
			regindex = first_empty_slot;
		else {
			ERROR_REPORT1(TXGBE_ERROR_SOFTWARE,
				     "No space in VLVF.\n");
			regindex = TXGBE_ERR_NO_SPACE;
		}
	}

	return regindex;
}

/**
 *  txgbe_set_vfta - Set VLAN filter table
 *  @hw: pointer to hardware structure
 *  @vlan: VLAN id to write to VLAN filter
 *  @vind: VMDq output index that maps queue to VLAN id in VFVFB
 *  @vlan_on: boolean flag to turn on/off VLAN in VFVF
 *
 *  Turn on/off specified VLAN in the VLAN filter table.
 **/
s32 txgbe_set_vfta(struct txgbe_hw *hw, u32 vlan, u32 vind,
			   bool vlan_on)
{
	s32 regindex;
	u32 bitindex;
	u32 vfta;
	u32 targetbit;
	s32 ret_val = 0;
	bool vfta_changed = false;

	DEBUGFUNC("\n");

	if (vlan > 4095)
		return TXGBE_ERR_PARAM;

	/*
	 * this is a 2 part operation - first the VFTA, then the
	 * VLVF and VLVFB if VT Mode is set
	 * We don't write the VFTA until we know the VLVF part succeeded.
	 */

	/* Part 1
	 * The VFTA is a bitstring made up of 128 32-bit registers
	 * that enable the particular VLAN id, much like the MTA:
	 *    bits[11-5]: which register
	 *    bits[4-0]:  which bit in the register
	 */
	regindex = (vlan >> 5) & 0x7F;
	bitindex = vlan & 0x1F;
	targetbit = (1 << bitindex);
	/* errata 5 */
	vfta = hw->mac.vft_shadow[regindex];
	if (vlan_on) {
		if (!(vfta & targetbit)) {
			vfta |= targetbit;
			vfta_changed = true;
		}
	} else {
		if ((vfta & targetbit)) {
			vfta &= ~targetbit;
			vfta_changed = true;
		}
	}

	/* Part 2
	 * Call txgbe_set_vlvf to set VLVFB and VLVF
	 */
	ret_val = txgbe_set_vlvf(hw, vlan, vind, vlan_on,
					 &vfta_changed);
	if (ret_val != 0)
		return ret_val;

	if (vfta_changed)
		wr32(hw, TXGBE_PSR_VLAN_TBL(regindex), vfta);
	/* errata 5 */
	hw->mac.vft_shadow[regindex] = vfta;
	return 0;
}

/**
 *  txgbe_set_vlvf - Set VLAN Pool Filter
 *  @hw: pointer to hardware structure
 *  @vlan: VLAN id to write to VLAN filter
 *  @vind: VMDq output index that maps queue to VLAN id in VFVFB
 *  @vlan_on: boolean flag to turn on/off VLAN in VFVF
 *  @vfta_changed: pointer to boolean flag which indicates whether VFTA
 *                 should be changed
 *
 *  Turn on/off specified bit in VLVF table.
 **/
s32 txgbe_set_vlvf(struct txgbe_hw *hw, u32 vlan, u32 vind,
			    bool vlan_on, bool *vfta_changed)
{
	u32 vt;

	DEBUGFUNC("\n");

	if (vlan > 4095)
		return TXGBE_ERR_PARAM;

	/* If VT Mode is set
	 *   Either vlan_on
	 *     make sure the vlan is in VLVF
	 *     set the vind bit in the matching VLVFB
	 *   Or !vlan_on
	 *     clear the pool bit and possibly the vind
	 */
	vt = rd32(hw, TXGBE_CFG_PORT_CTL);
	if (vt & TXGBE_CFG_PORT_CTL_NUM_VT_MASK) {
		s32 vlvf_index;
		u32 bits;

		vlvf_index = txgbe_find_vlvf_slot(hw, vlan);
		if (vlvf_index < 0)
			return vlvf_index;

		wr32(hw, TXGBE_PSR_VLAN_SWC_IDX, vlvf_index);
		if (vlan_on) {
			/* set the pool bit */
			if (vind < 32) {
				bits = rd32(hw,
						TXGBE_PSR_VLAN_SWC_VM_L);
				bits |= (1 << vind);
				wr32(hw,
						TXGBE_PSR_VLAN_SWC_VM_L,
						bits);
			} else {
				bits = rd32(hw,
					TXGBE_PSR_VLAN_SWC_VM_H);
				bits |= (1 << (vind - 32));
				wr32(hw,
					TXGBE_PSR_VLAN_SWC_VM_H,
					bits);
			}
		} else {
			/* clear the pool bit */
			if (vind < 32) {
				bits = rd32(hw,
						TXGBE_PSR_VLAN_SWC_VM_L);
				bits &= ~(1 << vind);
				wr32(hw,
						TXGBE_PSR_VLAN_SWC_VM_L,
						bits);
				bits |= rd32(hw,
					TXGBE_PSR_VLAN_SWC_VM_H);
			} else {
				bits = rd32(hw,
					TXGBE_PSR_VLAN_SWC_VM_H);
				bits &= ~(1 << (vind - 32));
				wr32(hw,
					TXGBE_PSR_VLAN_SWC_VM_H,
					bits);
				bits |= rd32(hw,
						TXGBE_PSR_VLAN_SWC_VM_L);
			}
		}

		/*
		 * If there are still bits set in the VLVFB registers
		 * for the VLAN ID indicated we need to see if the
		 * caller is requesting that we clear the VFTA entry bit.
		 * If the caller has requested that we clear the VFTA
		 * entry bit but there are still pools/VFs using this VLAN
		 * ID entry then ignore the request.  We're not worried
		 * about the case where we're turning the VFTA VLAN ID
		 * entry bit on, only when requested to turn it off as
		 * there may be multiple pools and/or VFs using the
		 * VLAN ID entry.  In that case we cannot clear the
		 * VFTA bit until all pools/VFs using that VLAN ID have also
		 * been cleared.  This will be indicated by "bits" being
		 * zero.
		 */
		if (bits) {
			wr32(hw, TXGBE_PSR_VLAN_SWC,
					(TXGBE_PSR_VLAN_SWC_VIEN | vlan));
			if ((!vlan_on) && (vfta_changed != NULL)) {
				/* someone wants to clear the vfta entry
				 * but some pools/VFs are still using it.
				 * Ignore it. */
				*vfta_changed = false;
			}
		} else
			wr32(hw, TXGBE_PSR_VLAN_SWC, 0);
	}

	return 0;
}

/**
 *  txgbe_clear_vfta - Clear VLAN filter table
 *  @hw: pointer to hardware structure
 *
 *  Clears the VLAN filer table, and the VMDq index associated with the filter
 **/
s32 txgbe_clear_vfta(struct txgbe_hw *hw)
{
	u32 offset;

	DEBUGFUNC("\n");

	for (offset = 0; offset < hw->mac.vft_size; offset++) {
		wr32(hw, TXGBE_PSR_VLAN_TBL(offset), 0);
		/* errata 5 */
		hw->mac.vft_shadow[offset] = 0;
	}

	for (offset = 0; offset < TXGBE_PSR_VLAN_SWC_ENTRIES; offset++) {
		wr32(hw, TXGBE_PSR_VLAN_SWC_IDX, offset);
		wr32(hw, TXGBE_PSR_VLAN_SWC, 0);
		wr32(hw, TXGBE_PSR_VLAN_SWC_VM_L, 0);
		wr32(hw, TXGBE_PSR_VLAN_SWC_VM_H, 0);
	}

	return 0;
}

/**
 *  txgbe_get_wwn_prefix - Get alternative WWNN/WWPN prefix from
 *  the EEPROM
 *  @hw: pointer to hardware structure
 *  @wwnn_prefix: the alternative WWNN prefix
 *  @wwpn_prefix: the alternative WWPN prefix
 *
 *  This function will read the EEPROM from the alternative SAN MAC address
 *  block to check the support for the alternative WWNN/WWPN prefix support.
 **/
s32 txgbe_get_wwn_prefix(struct txgbe_hw *hw, u16 *wwnn_prefix,
				 u16 *wwpn_prefix)
{
	u16 offset, caps;
	u16 alt_san_mac_blk_offset;

	DEBUGFUNC("\n");

	/* clear output first */
	*wwnn_prefix = 0xFFFF;
	*wwpn_prefix = 0xFFFF;

	/* check if alternative SAN MAC is supported */
	offset = hw->eeprom.sw_region_offset + TXGBE_ALT_SAN_MAC_ADDR_BLK_PTR;
	if (TCALL(hw, eeprom.ops.read, offset, &alt_san_mac_blk_offset))
		goto wwn_prefix_err;

	if ((alt_san_mac_blk_offset == 0) ||
	    (alt_san_mac_blk_offset == 0xFFFF))
		goto wwn_prefix_out;

	/* check capability in alternative san mac address block */
	offset = alt_san_mac_blk_offset + TXGBE_ALT_SAN_MAC_ADDR_CAPS_OFFSET;
	if (TCALL(hw, eeprom.ops.read, offset, &caps))
		goto wwn_prefix_err;
	if (!(caps & TXGBE_ALT_SAN_MAC_ADDR_CAPS_ALTWWN))
		goto wwn_prefix_out;

	/* get the corresponding prefix for WWNN/WWPN */
	offset = alt_san_mac_blk_offset + TXGBE_ALT_SAN_MAC_ADDR_WWNN_OFFSET;
	if (TCALL(hw, eeprom.ops.read, offset, wwnn_prefix)) {
		ERROR_REPORT2(TXGBE_ERROR_INVALID_STATE,
			      "eeprom read at offset %d failed", offset);
	}

	offset = alt_san_mac_blk_offset + TXGBE_ALT_SAN_MAC_ADDR_WWPN_OFFSET;
	if (TCALL(hw, eeprom.ops.read, offset, wwpn_prefix))
		goto wwn_prefix_err;

wwn_prefix_out:
	return 0;

wwn_prefix_err:
	ERROR_REPORT2(TXGBE_ERROR_INVALID_STATE,
		      "eeprom read at offset %d failed", offset);
	return 0;
}


/**
 *  txgbe_set_mac_anti_spoofing - Enable/Disable MAC anti-spoofing
 *  @hw: pointer to hardware structure
 *  @enable: enable or disable switch for anti-spoofing
 *  @pf: Physical Function pool - do not enable anti-spoofing for the PF
 *
 **/
void txgbe_set_mac_anti_spoofing(struct txgbe_hw *hw, bool enable, int pf)
{
	u64 pfvfspoof = 0;

	DEBUGFUNC("\n");

	if (enable) {
		/*
		 * The PF should be allowed to spoof so that it can support
		 * emulation mode NICs.  Do not set the bits assigned to the PF
		 * Remaining pools belong to the PF so they do not need to have
		 * anti-spoofing enabled.
		 */
		pfvfspoof = (1 << pf) - 1;
		wr32(hw, TXGBE_TDM_MAC_AS_L,
				pfvfspoof & 0xffffffff);
		wr32(hw, TXGBE_TDM_MAC_AS_H, pfvfspoof >> 32);
	} else {
		wr32(hw, TXGBE_TDM_MAC_AS_L, 0);
		wr32(hw, TXGBE_TDM_MAC_AS_H, 0);
	}
}

/**
 *  txgbe_set_vlan_anti_spoofing - Enable/Disable VLAN anti-spoofing
 *  @hw: pointer to hardware structure
 *  @enable: enable or disable switch for VLAN anti-spoofing
 *  @vf: Virtual Function pool - VF Pool to set for VLAN anti-spoofing
 *
 **/
void txgbe_set_vlan_anti_spoofing(struct txgbe_hw *hw, bool enable, int vf)
{
	u32 pfvfspoof;

	DEBUGFUNC("\n");

	if (vf < 32) {
		pfvfspoof = rd32(hw, TXGBE_TDM_VLAN_AS_L);
		if (enable)
			pfvfspoof |= (1 << vf);
		else
			pfvfspoof &= ~(1 << vf);
		wr32(hw, TXGBE_TDM_VLAN_AS_L, pfvfspoof);
	} else {
		pfvfspoof = rd32(hw, TXGBE_TDM_VLAN_AS_H);
		if (enable)
			pfvfspoof |= (1 << (vf - 32));
		else
			pfvfspoof &= ~(1 << (vf - 32));
		wr32(hw, TXGBE_TDM_VLAN_AS_H, pfvfspoof);
	}
}

/**
 *  txgbe_set_ethertype_anti_spoofing - Enable/Disable Ethertype anti-spoofing
 *  @hw: pointer to hardware structure
 *  @enable: enable or disable switch for Ethertype anti-spoofing
 *  @vf: Virtual Function pool - VF Pool to set for Ethertype anti-spoofing
 *
 **/
void txgbe_set_ethertype_anti_spoofing(struct txgbe_hw *hw,
		bool enable, int vf)
{
	u32 pfvfspoof;

	DEBUGFUNC("\n");

	if (vf < 32) {
		pfvfspoof = rd32(hw, TXGBE_TDM_ETYPE_AS_L);
		if (enable)
			pfvfspoof |= (1 << vf);
		else
			pfvfspoof &= ~(1 << vf);
		wr32(hw, TXGBE_TDM_ETYPE_AS_L, pfvfspoof);
	} else {
		pfvfspoof = rd32(hw, TXGBE_TDM_ETYPE_AS_H);
		if (enable)
			pfvfspoof |= (1 << (vf - 32));
		else
			pfvfspoof &= ~(1 << (vf - 32));
		wr32(hw, TXGBE_TDM_ETYPE_AS_H, pfvfspoof);
	}
}

/**
 *  txgbe_get_device_caps - Get additional device capabilities
 *  @hw: pointer to hardware structure
 *  @device_caps: the EEPROM word with the extra device capabilities
 *
 *  This function will read the EEPROM location for the device capabilities,
 *  and return the word through device_caps.
 **/
s32 txgbe_get_device_caps(struct txgbe_hw *hw, u16 *device_caps)
{
	DEBUGFUNC("\n");

	TCALL(hw, eeprom.ops.read,
		hw->eeprom.sw_region_offset + TXGBE_DEVICE_CAPS, device_caps);

	return 0;
}

/**
 *  txgbe_calculate_checksum - Calculate checksum for buffer
 *  @buffer: pointer to EEPROM
 *  @length: size of EEPROM to calculate a checksum for
 *  Calculates the checksum for some buffer on a specified length.  The
 *  checksum calculated is returned.
 **/
u8 txgbe_calculate_checksum(u8 *buffer, u32 length)
{
	u32 i;
	u8 sum = 0;

	DEBUGFUNC("\n");

	if (!buffer)
		return 0;

	for (i = 0; i < length; i++)
		sum += buffer[i];

	return (u8) (0 - sum);
}

/**
 *  txgbe_host_interface_command - Issue command to manageability block
 *  @hw: pointer to the HW structure
 *  @buffer: contains the command to write and where the return status will
 *   be placed
 *  @length: length of buffer, must be multiple of 4 bytes
 *  @timeout: time in ms to wait for command completion
 *  @return_data: read and return data from the buffer (true) or not (false)
 *   Needed because FW structures are big endian and decoding of
 *   these fields can be 8 bit or 16 bit based on command. Decoding
 *   is not easily understood without making a table of commands.
 *   So we will leave this up to the caller to read back the data
 *   in these cases.
 *
 *  Communicates with the manageability block.  On success return 0
 *  else return TXGBE_ERR_HOST_INTERFACE_COMMAND.
 **/
s32 txgbe_host_interface_command(struct txgbe_hw *hw, u32 *buffer,
				 u32 length, u32 timeout, bool return_data)
{
	u32 hicr, i, bi;
	u32 hdr_size = sizeof(struct txgbe_hic_hdr);
	u16 buf_len;
	u32 dword_len;
	s32 status = 0;
	u32 buf[64] = {};

	DEBUGFUNC("\n");

	if (length == 0 || length > TXGBE_HI_MAX_BLOCK_BYTE_LENGTH) {
		DEBUGOUT1("Buffer length failure buffersize=%d.\n", length);
		return TXGBE_ERR_HOST_INTERFACE_COMMAND;
	}

	if (TCALL(hw, mac.ops.acquire_swfw_sync, TXGBE_MNG_SWFW_SYNC_SW_MB)
	    != 0) {
		return TXGBE_ERR_SWFW_SYNC;
	}


	/* Calculate length in DWORDs. We must be DWORD aligned */
	if ((length % (sizeof(u32))) != 0) {
		DEBUGOUT("Buffer length failure, not aligned to dword");
		status = TXGBE_ERR_INVALID_ARGUMENT;
		goto rel_out;
	}

	dword_len = length >> 2;

	/* The device driver writes the relevant command block
	 * into the ram area.
	 */
	for (i = 0; i < dword_len; i++) {
		if (txgbe_check_mng_access(hw)) {
			wr32a(hw, TXGBE_MNG_MBOX,
						i, TXGBE_CPU_TO_LE32(buffer[i]));
			/* write flush */
			buf[i] = rd32a(hw, TXGBE_MNG_MBOX, i);
		}
		else {
			status = TXGBE_ERR_MNG_ACCESS_FAILED;
			goto rel_out;
		}
	}
	/* Setting this bit tells the ARC that a new command is pending. */
	if (txgbe_check_mng_access(hw))
		wr32m(hw, TXGBE_MNG_MBOX_CTL,
			TXGBE_MNG_MBOX_CTL_SWRDY, TXGBE_MNG_MBOX_CTL_SWRDY);
	else {
		status = TXGBE_ERR_MNG_ACCESS_FAILED;
		goto rel_out;
	}

	for (i = 0; i < timeout; i++) {
		if (txgbe_check_mng_access(hw)) {
			hicr = rd32(hw, TXGBE_MNG_MBOX_CTL);
			if ((hicr & TXGBE_MNG_MBOX_CTL_FWRDY))
				break;
		}
		msec_delay(1);
	}

	buf[0] = rd32(hw, TXGBE_MNG_MBOX);

	if ((buf[0] & 0xff0000) >> 16 == 0x80) {
		DEBUGOUT("It's unknown cmd.\n");
		status = TXGBE_ERR_MNG_ACCESS_FAILED;
		goto rel_out;
	}
	/* Check command completion */
	if (timeout != 0 && i == timeout) {
		ERROR_REPORT1(TXGBE_ERROR_CAUTION,
						"Command has failed with no status valid.\n");

		ERROR_REPORT1(TXGBE_ERROR_CAUTION, "write value:\n");
		for (i = 0; i < dword_len; i++) {
			ERROR_REPORT1(TXGBE_ERROR_CAUTION, "%x ", buffer[i]);
		}
		ERROR_REPORT1(TXGBE_ERROR_CAUTION, "read value:\n");
		for (i = 0; i < dword_len; i++) {
			ERROR_REPORT1(TXGBE_ERROR_CAUTION, "%x ", buf[i]);
		}
		printk("===%x= %x=\n", buffer[0] & 0xff, (~buf[0] >> 24));
		if( (buffer[0] & 0xff) != (~buf[0] >> 24)) {
			status = TXGBE_ERR_HOST_INTERFACE_COMMAND;
			goto rel_out;
		}
	}

	if (!return_data)
		goto rel_out;

	/* Calculate length in DWORDs */
	dword_len = hdr_size >> 2;

	/* first pull in the header so we know the buffer length */
	for (bi = 0; bi < dword_len; bi++) {
		if (txgbe_check_mng_access(hw)) {
			buffer[bi] = rd32a(hw, TXGBE_MNG_MBOX,
							bi);
			TXGBE_LE32_TO_CPUS(&buffer[bi]);
		} else {
			status = TXGBE_ERR_MNG_ACCESS_FAILED;
			goto rel_out;
		}
	}

	/* If there is any thing in data position pull it in */
	buf_len = ((struct txgbe_hic_hdr *)buffer)->buf_len;
	if (buf_len == 0)
		goto rel_out;

	if (length < buf_len + hdr_size) {
		DEBUGOUT("Buffer not large enough for reply message.\n");
		status = TXGBE_ERR_HOST_INTERFACE_COMMAND;
		goto rel_out;
	}

	/* Calculate length in DWORDs, add 3 for odd lengths */
	dword_len = (buf_len + 3) >> 2;

	/* Pull in the rest of the buffer (bi is where we left off) */
	for (; bi <= dword_len; bi++) {
		if (txgbe_check_mng_access(hw)) {
			buffer[bi] = rd32a(hw, TXGBE_MNG_MBOX,
							bi);
			TXGBE_LE32_TO_CPUS(&buffer[bi]);
		} else {
			status = TXGBE_ERR_MNG_ACCESS_FAILED;
			goto rel_out;
		}
	}

rel_out:
	TCALL(hw, mac.ops.release_swfw_sync, TXGBE_MNG_SWFW_SYNC_SW_MB);
	return status;
}

/**
 *  txgbe_set_fw_drv_ver - Sends driver version to firmware
 *  @hw: pointer to the HW structure
 *  @maj: driver version major number
 *  @min: driver version minor number
 *  @build: driver version build number
 *  @sub: driver version sub build number
 *
 *  Sends driver version number to firmware through the manageability
 *  block.  On success return 0
 *  else returns TXGBE_ERR_SWFW_SYNC when encountering an error acquiring
 *  semaphore or TXGBE_ERR_HOST_INTERFACE_COMMAND when command fails.
 **/
s32 txgbe_set_fw_drv_ver(struct txgbe_hw *hw, u8 maj, u8 min,
				 u8 build, u8 sub)
{
	struct txgbe_hic_drv_info fw_cmd;
	int i;
	s32 ret_val = 0;

	DEBUGFUNC("\n");

	fw_cmd.hdr.cmd = FW_CEM_CMD_DRIVER_INFO;
	fw_cmd.hdr.buf_len = FW_CEM_CMD_DRIVER_INFO_LEN;
	fw_cmd.hdr.cmd_or_resp.cmd_resv = FW_CEM_CMD_RESERVED;
	fw_cmd.port_num = (u8)hw->bus.func;
	fw_cmd.ver_maj = maj;
	fw_cmd.ver_min = min;
	fw_cmd.ver_build = build;
	fw_cmd.ver_sub = sub;
	fw_cmd.hdr.checksum = 0;
	fw_cmd.hdr.checksum = txgbe_calculate_checksum((u8 *)&fw_cmd,
				(FW_CEM_HDR_LEN + fw_cmd.hdr.buf_len));
	fw_cmd.pad = 0;
	fw_cmd.pad2 = 0;

	for (i = 0; i <= FW_CEM_MAX_RETRIES; i++) {
		ret_val = txgbe_host_interface_command(hw, (u32 *)&fw_cmd,
						       sizeof(fw_cmd),
						       TXGBE_HI_COMMAND_TIMEOUT,
						       true);
		if (ret_val != 0)
			continue;

		if (fw_cmd.hdr.cmd_or_resp.ret_status ==
		    FW_CEM_RESP_STATUS_SUCCESS)
			ret_val = 0;
		else
			ret_val = TXGBE_ERR_HOST_INTERFACE_COMMAND;

		break;
	}

	return ret_val;
}

/**
 *  txgbe_reset_hostif - send reset cmd to fw
 *  @hw: pointer to hardware structure
 *
 *  Sends reset cmd to firmware through the manageability
 *  block.  On success return 0
 *  else returns TXGBE_ERR_SWFW_SYNC when encountering an error acquiring
 *  semaphore or TXGBE_ERR_HOST_INTERFACE_COMMAND when command fails.
 **/
s32 txgbe_reset_hostif(struct txgbe_hw *hw)
{
	struct txgbe_hic_reset reset_cmd;
	int i;
	s32 status = 0;

	DEBUGFUNC("\n");

	reset_cmd.hdr.cmd = FW_RESET_CMD;
	reset_cmd.hdr.buf_len = FW_RESET_LEN;
	reset_cmd.hdr.cmd_or_resp.cmd_resv = FW_CEM_CMD_RESERVED;
	reset_cmd.lan_id = hw->bus.lan_id;
	reset_cmd.reset_type = (u16)hw->reset_type;
	reset_cmd.hdr.checksum = 0;
	reset_cmd.hdr.checksum = txgbe_calculate_checksum((u8 *)&reset_cmd,
				(FW_CEM_HDR_LEN + reset_cmd.hdr.buf_len));

	for (i = 0; i <= FW_CEM_MAX_RETRIES; i++) {
		status = txgbe_host_interface_command(hw, (u32 *)&reset_cmd,
						       sizeof(reset_cmd),
						       TXGBE_HI_COMMAND_TIMEOUT,
						       true);
		if (status != 0)
			continue;

		if (reset_cmd.hdr.cmd_or_resp.ret_status ==
			FW_CEM_RESP_STATUS_SUCCESS) {
			status = 0;
			hw->link_status = TXGBE_LINK_STATUS_NONE;
		}
		else
			status = TXGBE_ERR_HOST_INTERFACE_COMMAND;

		break;
	}

	return status;
}


s32 txgbe_setup_mac_link_hostif(struct txgbe_hw *hw, u32 speed)
{
	struct txgbe_hic_phy_cfg cmd;
	int i;
	s32 status = 0;

	DEBUGFUNC("\n");

	cmd.hdr.cmd = FW_SETUP_MAC_LINK_CMD;
	cmd.hdr.buf_len = FW_SETUP_MAC_LINK_LEN;
	cmd.hdr.cmd_or_resp.cmd_resv = FW_CEM_CMD_RESERVED;
	cmd.lan_id = hw->bus.lan_id;
	cmd.phy_mode = 0;
	cmd.phy_speed = (u16)speed;
	cmd.hdr.checksum = 0;
	cmd.hdr.checksum = txgbe_calculate_checksum((u8 *)&cmd,
				(FW_CEM_HDR_LEN + cmd.hdr.buf_len));

	for (i = 0; i <= FW_CEM_MAX_RETRIES; i++) {
		status = txgbe_host_interface_command(hw, (u32 *)&cmd,
						       sizeof(cmd),
						       TXGBE_HI_COMMAND_TIMEOUT,
						       true);
		if (status != 0)
			continue;

		if (cmd.hdr.cmd_or_resp.ret_status ==
		    FW_CEM_RESP_STATUS_SUCCESS)
			status = 0;
		else
			status = TXGBE_ERR_HOST_INTERFACE_COMMAND;

		break;
	}

	return status;

}

u16 txgbe_crc16_ccitt(const u8 *buf, int size)
{
	u16 crc = 0;
	int i;
	while (--size >= 0) {
		crc ^= (u16)*buf++ << 8;
		for (i = 0; i < 8; i++) {
			if (crc & 0x8000)
				crc = crc << 1 ^ 0x1021;
			else
				crc <<= 1;
		}
	}
	return crc;
}

s32 txgbe_upgrade_flash_hostif(struct txgbe_hw *hw, u32 region,
				const u8 *data, u32 size)
{
	struct txgbe_hic_upg_start start_cmd;
	struct txgbe_hic_upg_write write_cmd;
	struct txgbe_hic_upg_verify verify_cmd;
	u32 offset;
	s32 status = 0;

	DEBUGFUNC("\n");

	start_cmd.hdr.cmd = FW_FLASH_UPGRADE_START_CMD;
	start_cmd.hdr.buf_len = FW_FLASH_UPGRADE_START_LEN;
	start_cmd.hdr.cmd_or_resp.cmd_resv = FW_CEM_CMD_RESERVED;
	start_cmd.module_id = (u8)region;
	start_cmd.hdr.checksum = 0;
	start_cmd.hdr.checksum = txgbe_calculate_checksum((u8 *)&start_cmd,
				(FW_CEM_HDR_LEN + start_cmd.hdr.buf_len));
	start_cmd.pad2 = 0;
	start_cmd.pad3 = 0;

	status = txgbe_host_interface_command(hw, (u32 *)&start_cmd,
					       sizeof(start_cmd),
					       TXGBE_HI_FLASH_ERASE_TIMEOUT,
					       true);

	if (start_cmd.hdr.cmd_or_resp.ret_status == FW_CEM_RESP_STATUS_SUCCESS)
		status = 0;
	else {
		status = TXGBE_ERR_HOST_INTERFACE_COMMAND;
		return status;
	}

	for (offset = 0; offset < size;) {
		write_cmd.hdr.cmd = FW_FLASH_UPGRADE_WRITE_CMD;
		if (size - offset > 248) {
			write_cmd.data_len = 248 / 4;
			write_cmd.eof_flag = 0;
		} else {
			write_cmd.data_len = (u8)((size - offset) / 4);
			write_cmd.eof_flag = 1;
		}
		memcpy((u8 *)write_cmd.data, &data[offset], write_cmd.data_len * 4);
		write_cmd.hdr.buf_len = (write_cmd.data_len + 1) * 4;
		write_cmd.hdr.cmd_or_resp.cmd_resv = FW_CEM_CMD_RESERVED;
		write_cmd.check_sum = txgbe_crc16_ccitt((u8 *)write_cmd.data,
							write_cmd.data_len * 4);

		status = txgbe_host_interface_command(hw, (u32 *)&write_cmd,
						sizeof(write_cmd),
						TXGBE_HI_FLASH_UPDATE_TIMEOUT,
						true);
		if (start_cmd.hdr.cmd_or_resp.ret_status ==
						FW_CEM_RESP_STATUS_SUCCESS)
			status = 0;
		else {
			status = TXGBE_ERR_HOST_INTERFACE_COMMAND;
			return status;
		}
		offset += write_cmd.data_len * 4;
	}

	verify_cmd.hdr.cmd = FW_FLASH_UPGRADE_VERIFY_CMD;
	verify_cmd.hdr.buf_len = FW_FLASH_UPGRADE_VERIFY_LEN;
	verify_cmd.hdr.cmd_or_resp.cmd_resv = FW_CEM_CMD_RESERVED;
	switch (region) {
	case TXGBE_MODULE_EEPROM:
		verify_cmd.action_flag = TXGBE_RELOAD_EEPROM;
		break;
	case TXGBE_MODULE_FIRMWARE:
		verify_cmd.action_flag = TXGBE_RESET_FIRMWARE;
		break;
	case TXGBE_MODULE_HARDWARE:
		verify_cmd.action_flag = TXGBE_RESET_LAN;
		break;
	default:
		return status;
	}

	verify_cmd.hdr.checksum = txgbe_calculate_checksum((u8 *)&verify_cmd,
				(FW_CEM_HDR_LEN + verify_cmd.hdr.buf_len));

	status = txgbe_host_interface_command(hw, (u32 *)&verify_cmd,
					       sizeof(verify_cmd),
					       TXGBE_HI_FLASH_VERIFY_TIMEOUT,
					       true);

	if (verify_cmd.hdr.cmd_or_resp.ret_status == FW_CEM_RESP_STATUS_SUCCESS)
		status = 0;
	else {
		status = TXGBE_ERR_HOST_INTERFACE_COMMAND;
	}
	return status;
}

/* cmd_addr is used for some special command:
**	 1. to be sector address, when implemented erase sector command
**	 2. to be flash address when implemented read, write flash address
*/
u8 fmgr_cmd_op(struct txgbe_hw *hw, u32 cmd, u32 cmd_addr)
{
	u32 cmd_val = 0;
	u32 time_out = 0;

	cmd_val = (cmd << SPI_CLK_CMD_OFFSET) | (SPI_CLK_DIV << SPI_CLK_DIV_OFFSET) | cmd_addr;
	wr32(hw, SPI_H_CMD_REG_ADDR, cmd_val);
	while (1) {
		if (rd32(hw, SPI_H_STA_REG_ADDR) & 0x1)
			break;

		if (time_out == SPI_TIME_OUT_VALUE)
			return 1;

		time_out = time_out + 1;
		udelay(10);
	}

	return 0;
}
u8 fmgr_usr_cmd_op(struct txgbe_hw *hw, u32 usr_cmd)
{
	u8 status = 0;

	wr32(hw, SPI_H_USR_CMD_REG_ADDR, usr_cmd);
	status = fmgr_cmd_op(hw, SPI_CMD_USER_CMD, 0);

	return status;
}

u8 flash_erase_chip(struct txgbe_hw *hw)
{
	u8 status = fmgr_cmd_op(hw, SPI_CMD_ERASE_CHIP, 0);
	return status;
}

u8 flash_erase_sector(struct txgbe_hw *hw, u32 sec_addr)
{
	u8 status = fmgr_cmd_op(hw, SPI_CMD_ERASE_SECTOR, sec_addr);
	return status;
}
u32 flash_read_dword(struct txgbe_hw *hw, u32 addr)
{
	u8 status = fmgr_cmd_op(hw, SPI_CMD_READ_DWORD, addr);
	if (status)
		return (u32)status;
	
	return rd32(hw, SPI_H_DAT_REG_ADDR);
}
u8 flash_write_dword(struct txgbe_hw *hw, u32 addr, u32 dword)
{
	u8 status = 0;
		
	wr32(hw, SPI_H_DAT_REG_ADDR, dword);
	status = fmgr_cmd_op(hw, SPI_CMD_WRITE_DWORD, addr);
	if (status)
		return status;

	if (dword != flash_read_dword(hw, addr)) {
		return 1;
	}
	return 0;
}

int txgbe_flash_write_cab(struct txgbe_hw *hw,u32 addr, u32 value,u16 lan_id)
{
	int status;
	struct txgbe_hic_read_cab buffer;
	
	buffer.hdr.req.cmd = 0xE2;
	buffer.hdr.req.buf_lenh = 0x6;
	buffer.hdr.req.buf_lenl = 0x0;
	buffer.hdr.req.checksum = 0xFF;

	/* convert offset from words to bytes */
	buffer.dbuf.d16[0] = cpu_to_le16(lan_id);
	/* one word */
	buffer.dbuf.d32[0] = htonl(addr);
	buffer.dbuf.d32[1] = htonl(value);

	status = txgbe_host_interface_command(hw, (u32 *)&buffer,
						sizeof(buffer), 5000, true);
	printk("0x1e100 :%08x\n",rd32(hw,0x1e100));
	printk("0x1e104 :%08x\n",rd32(hw,0x1e104));
	printk("0x1e108 :%08x\n",rd32(hw,0x1e108));
	printk("0x1e10c :%08x\n",rd32(hw,0x1e10c));

	return status;
}

int txgbe_flash_read_cab(struct txgbe_hw *hw, u32 addr ,u16 lan_id )
{
	int status;
	struct txgbe_hic_read_cab buffer;
	u16 *data = NULL;

	buffer.hdr.req.cmd = 0xE1;
	buffer.hdr.req.buf_lenh = 0xaa;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = 0xFF;

	/* convert offset from words to bytes */
	buffer.dbuf.d16[0] = cpu_to_le16(lan_id);
	/* one word */
	buffer.dbuf.d32[0] = htonl(addr);

	status = txgbe_host_interface_command(hw, (u32 *)&buffer,
										sizeof(buffer), 5000, true);

	if (status)
		return status;
	if (txgbe_check_mng_access(hw)) {
		*data = (u16)rd32a(hw, 0x1e100,3);
		printk("0x1e100 :%08x\n",rd32(hw,0x1e100));
		printk("0x1e104 :%08x\n",rd32(hw,0x1e104));
		printk("0x1e108 :%08x\n",rd32(hw,0x1e108));
		printk("0x1e10c :%08x\n",rd32(hw,0x1e10c));
	} else {
		status = -147;
		return status;
	}

	return rd32(hw,0x1e108);
}
int txgbe_flash_write_unlock(struct txgbe_hw *hw)
{
	int status;
	struct txgbe_hic_read_shadow_ram buffer;

	buffer.hdr.req.cmd = 0x40;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = 0xFF;

	/* convert offset from words to bytes */
	buffer.address = 0;
	/* one word */
	buffer.length = 0;

	status = txgbe_host_interface_command(hw, (u32 *)&buffer,
										sizeof(buffer), 5000,false);
	if (status)
		return status;

	return status;
}

int txgbe_flash_write_lock(struct txgbe_hw *hw)
{
	int status;
	struct txgbe_hic_read_shadow_ram buffer;

	buffer.hdr.req.cmd = 0x39;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = 0xFF;

	/* convert offset from words to bytes */
	buffer.address = 0;
	/* one word */
	buffer.length = 0;

	status = txgbe_host_interface_command(hw, (u32 *)&buffer,
										sizeof(buffer), 5000, false);
	if (status)
		return status;

	return status;
}

int txgbe_upgrade_flash(struct txgbe_hw *hw, u32 region,
								const u8 *data, u32 size)
{
	u32 			sector_num = 0;
	u32 			read_data = 0;
	u8				status = 0;
	u8				skip = 0;
	u32 			i = 0;
	u8				flash_vendor = 0;
	u32 mac_addr0_dword0_t;
	u32 mac_addr0_dword1_t;
	u32 mac_addr1_dword0_t;
	u32 mac_addr1_dword1_t;
	u32 serial_num_dword0_t;
	u32 serial_num_dword1_t;
	u32 serial_num_dword2_t;

	/*check sub_id*/;
	printk("Checking sub_id .......\n");
	printk("The card's sub_id : %04x\n", hw->subsystem_id);
	printk("The image's sub_id : %04x\n", data[0xfffdc] << 8  | data[0xfffdd]);
	if ((hw->subsystem_id & 0xfff) == 
		((data[0xfffdc] << 8  | data[0xfffdd]) & 0xfff)){
		printk("It is a right image\n");
	} else if (hw->subsystem_id == 0xffff){
		printk("update anyway\n");
	} else {
		printk("====The Gigabit image is not match the Gigabit card====\n");
		printk("====Please check your image====\n");
		return -EOPNOTSUPP;
	}
	
	/*check dev_id*/
	printk("Checking dev_id .......\n");
	printk("The image's dev_id : %04x\n", data[0xfffde] << 8  | data[0xfffdf]);
	printk("The card's dev_id : %04x\n", hw->device_id);
	if (!((hw->device_id & 0xfff0)== ((data[0xfffde] << 8 | data[0xfffdf]) & 0xfff0))
		&& !(hw->device_id == 0xffff))
	{
		printk("====The Gigabit image is not match the Gigabit card====\n");
		printk("====Please check your image====\n");
		return -EOPNOTSUPP;
	}

	/* unlock flash write protect*/
	wr32(hw,TXGBE_SPI_CMDCFG0,0x9f050206);
	wr32(hw,0x10194,0x9f050206);

	msleep(1000);

	mac_addr0_dword0_t = flash_read_dword(hw, MAC_ADDR0_WORD0_OFFSET_1G);
	mac_addr0_dword1_t = flash_read_dword(hw, MAC_ADDR0_WORD1_OFFSET_1G) & 0xffff;
	mac_addr1_dword0_t = flash_read_dword(hw, MAC_ADDR1_WORD0_OFFSET_1G);
	mac_addr1_dword1_t = flash_read_dword(hw, MAC_ADDR1_WORD1_OFFSET_1G) & 0xffff;

	serial_num_dword0_t = flash_read_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G);
	serial_num_dword1_t = flash_read_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G+4);
	serial_num_dword2_t = flash_read_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G+8);
	printk("Old: MAC Address0 is: 0x%04x%08x\n", mac_addr0_dword1_t, mac_addr0_dword0_t);
	printk("	 MAC Address1 is: 0x%04x%08x\n", mac_addr1_dword1_t, mac_addr1_dword0_t);

	
	status = fmgr_usr_cmd_op(hw, 0x6);	/* write enable*/
	status = fmgr_usr_cmd_op(hw, 0x98); /* global protection un-lock*/
	msleep(1000);
	
	/*Note: for Spanish FLASH, first 8 sectors (4KB) in sector0 (64KB) 
	need to use a special erase command (4K sector erase)*/
	if (flash_vendor == 1) {
		wr32(hw, SPI_CMD_CFG1_ADDR, 0x0103c720);			
		for (i=0; i<8; i++) {
			flash_erase_sector(hw, i*128);
			msleep(20); // 20 ms
		}
		wr32(hw, SPI_CMD_CFG1_ADDR, 0x0103c7d8);
	}

	sector_num = size / SPI_SECTOR_SIZE;
	/* Winbond Flash, erase chip command is okay, but erase sector doestn't work*/
	if (flash_vendor == 2) {
		status = flash_erase_chip(hw);
		printk("Erase chip command, return status = %0d\n", status);
		msleep(1000);
	} else {
		wr32(hw, SPI_CMD_CFG1_ADDR, 0x0103c720);
		for (i=0; i<sector_num; i++) {
			status = flash_erase_sector(hw, i * SPI_SECTOR_SIZE);
			printk("Erase sector[%2d] command, return status = %0d\n", i, status);
			msleep(50);
		}
		wr32(hw, SPI_CMD_CFG1_ADDR, 0x0103c7d8);
	}

	/* Program Image file in dword*/
	for (i=0; i<size/4; i++) {
		read_data = data[4*i+3]<<24 | data[4*i+2]<<16 | data[4*i+1]<<8 | data[4*i];
		read_data = __le32_to_cpu(read_data);
		skip = ((i*4 == MAC_ADDR0_WORD0_OFFSET_1G) || (i*4 == MAC_ADDR0_WORD1_OFFSET_1G) ||
				(i*4 == MAC_ADDR1_WORD0_OFFSET_1G) || (i*4 == MAC_ADDR1_WORD1_OFFSET_1G) ||
				(i*4 >= PRODUCT_SERIAL_NUM_OFFSET_1G && i*4 <= PRODUCT_SERIAL_NUM_OFFSET_1G+8));
		if (read_data != 0xffffffff && !skip) {
			status = flash_write_dword(hw, i*4, read_data);
			if (status) {
				printk("ERROR: Program 0x%08x @addr: 0x%08x is failed !!\n", read_data, i*4);
				read_data = flash_read_dword(hw, i*4);
				printk("		 Read data from Flash is: 0x%08x\n", read_data);
				return 1;
			}
		}
		if (i%1024 == 0) {
			printk("\b\b\b\b%3d%%", (int)(i*4 * 100 / size));
		}
	}
		flash_write_dword(hw, MAC_ADDR0_WORD0_OFFSET_1G, mac_addr0_dword0_t);
		flash_write_dword(hw, MAC_ADDR0_WORD1_OFFSET_1G, (mac_addr0_dword1_t | 0x80000000));//lan0
		flash_write_dword(hw, MAC_ADDR1_WORD0_OFFSET_1G, mac_addr1_dword0_t);
		flash_write_dword(hw, MAC_ADDR1_WORD1_OFFSET_1G, (mac_addr1_dword1_t | 0x80000000));//lan1
		flash_write_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G, serial_num_dword0_t);
		flash_write_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G+4, serial_num_dword1_t);
		flash_write_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G+8, serial_num_dword2_t);

	return 0;
}


/**
 * txgbe_set_rxpba - Initialize Rx packet buffer
 * @hw: pointer to hardware structure
 * @num_pb: number of packet buffers to allocate
 * @headroom: reserve n KB of headroom
 * @strategy: packet buffer allocation strategy
 **/
void txgbe_set_rxpba(struct txgbe_hw *hw, int num_pb, u32 headroom,
			     int strategy)
{
	u32 pbsize = hw->mac.rx_pb_size;
	int i = 0;
	u32 rxpktsize, txpktsize, txpbthresh;

	DEBUGFUNC("\n");

	/* Reserve headroom */
	pbsize -= headroom;

	if (!num_pb)
		num_pb = 1;

	/* Divide remaining packet buffer space amongst the number of packet
	 * buffers requested using supplied strategy.
	 */
	switch (strategy) {
	case PBA_STRATEGY_WEIGHTED:
		/* txgbe_dcb_pba_80_48 strategy weight first half of packet
		 * buffer with 5/8 of the packet buffer space.
		 */
		rxpktsize = (pbsize * 5) / (num_pb * 4);
		pbsize -= rxpktsize * (num_pb / 2);
		rxpktsize <<= TXGBE_RDB_PB_SZ_SHIFT;
		for (; i < (num_pb / 2); i++)
			wr32(hw, TXGBE_RDB_PB_SZ(i), rxpktsize);
		/* fall through */
		/* Fall through to configure remaining packet buffers */
	case PBA_STRATEGY_EQUAL:
		rxpktsize = (pbsize / (num_pb - i)) << TXGBE_RDB_PB_SZ_SHIFT;
		for (; i < num_pb; i++)
			wr32(hw, TXGBE_RDB_PB_SZ(i), rxpktsize);
		break;
	default:
		break;
	}

	/* Only support an equally distributed Tx packet buffer strategy. */
	txpktsize = TXGBE_TDB_PB_SZ_MAX / num_pb;
	txpbthresh = (txpktsize / 1024) - TXGBE_TXPKT_SIZE_MAX;
	for (i = 0; i < num_pb; i++) {
		wr32(hw, TXGBE_TDB_PB_SZ(i), txpktsize);
		wr32(hw, TXGBE_TDM_PB_THRE(i), txpbthresh);
	}

	/* Clear unused TCs, if any, to zero buffer size*/
	for (; i < TXGBE_MAX_PB; i++) {
		wr32(hw, TXGBE_RDB_PB_SZ(i), 0);
		wr32(hw, TXGBE_TDB_PB_SZ(i), 0);
		wr32(hw, TXGBE_TDM_PB_THRE(i), 0);
	}
}


STATIC const u8 txgbe_emc_temp_data[4] = {
	TXGBE_EMC_INTERNAL_DATA,
	TXGBE_EMC_DIODE1_DATA,
	TXGBE_EMC_DIODE2_DATA,
	TXGBE_EMC_DIODE3_DATA
};
STATIC const u8 txgbe_emc_therm_limit[4] = {
	TXGBE_EMC_INTERNAL_THERM_LIMIT,
	TXGBE_EMC_DIODE1_THERM_LIMIT,
	TXGBE_EMC_DIODE2_THERM_LIMIT,
	TXGBE_EMC_DIODE3_THERM_LIMIT
};

/**
 *  txgbe_get_thermal_sensor_data - Gathers thermal sensor data
 *  @hw: pointer to hardware structure
 *  @data: pointer to the thermal sensor data structure
 *
 * algorithm:
 * T = (-4.8380E+01)N^0 + (3.1020E-01)N^1 + (-1.8201E-04)N^2 +
		       (8.1542E-08)N^3 + (-1.6743E-11)N^4
 * algorithm with 5% more deviation, easy for implementation
 * T = (-50)N^0 + (0.31)N^1 + (-0.0002)N^2 + (0.0000001)N^3
 *
 *  Returns the thermal sensor data structure
 **/
s32 txgbe_get_thermal_sensor_data(struct txgbe_hw *hw)
{
	s64 tsv;
	int i = 0;
	struct txgbe_thermal_sensor_data *data = &hw->mac.thermal_sensor_data;

	DEBUGFUNC("\n");

	/* Only support thermal sensors attached to physical port 0 */
	if (hw->bus.lan_id)
		return TXGBE_NOT_IMPLEMENTED;

	tsv = (s64)(rd32(hw, TXGBE_TS_ST) &
		TXGBE_TS_ST_DATA_OUT_MASK);

	tsv = tsv < 1200 ? tsv : 1200;
	tsv = -(48380 << 8) / 1000
		+ tsv * (31020 << 8) / 100000
		- tsv * tsv * (18201 << 8) / 100000000
		+ tsv * tsv * tsv * (81542 << 8) / 1000000000000
		- tsv * tsv * tsv * tsv * (16743 << 8) / 1000000000000000;
	tsv >>= 8;

	data->sensor.temp = (s16)tsv;

	for (i = 0; i < 100 ; i++){
			tsv = (s64)rd32(hw, TXGBE_TS_ST);
			if( tsv >> 16 == 0x1 ){
				tsv = tsv & TXGBE_TS_ST_DATA_OUT_MASK;
				tsv = tsv < 1200 ? tsv : 1200;
				tsv = -(48380 << 8) / 1000
						+ tsv * (31020 << 8) / 100000
						- tsv * tsv * (18201 << 8) / 100000000
						+ tsv * tsv * tsv * (81542 << 8) / 1000000000000
						- tsv * tsv * tsv * tsv * (16743 << 8) / 1000000000000000;
				tsv >>= 8;
	
				data->sensor.temp = (s16)tsv;
				break;
			}else{
				msleep(1);
				continue;
			}
	}
	
	return 0;
}

/**
 *  txgbe_init_thermal_sensor_thresh - Inits thermal sensor thresholds
 *  @hw: pointer to hardware structure
 *
 *  Inits the thermal sensor thresholds according to the NVM map
 *  and save off the threshold and location values into mac.thermal_sensor_data
 **/
s32 txgbe_init_thermal_sensor_thresh(struct txgbe_hw *hw)
{
	s32 status = 0;

	struct txgbe_thermal_sensor_data *data = &hw->mac.thermal_sensor_data;

	DEBUGFUNC("\n");

	memset(data, 0, sizeof(struct txgbe_thermal_sensor_data));

	/* Only support thermal sensors attached to SP physical port 0 */
	if (hw->bus.lan_id)
		return TXGBE_NOT_IMPLEMENTED;

	wr32(hw, TXGBE_TS_CTL, TXGBE_TS_CTL_EVAL_MD);
	wr32(hw, TXGBE_TS_INT_EN,
		TXGBE_TS_INT_EN_ALARM_INT_EN | TXGBE_TS_INT_EN_DALARM_INT_EN);
	wr32(hw, TXGBE_TS_EN, TXGBE_TS_EN_ENA);


	data->sensor.alarm_thresh = 100;
	wr32(hw, TXGBE_TS_ALARM_THRE, 677);
	data->sensor.dalarm_thresh = 90;
	wr32(hw, TXGBE_TS_DALARM_THRE, 614);

	return status;
}

void txgbe_disable_rx(struct txgbe_hw *hw)
{
	u32 pfdtxgswc;
	u32 rxctrl;

	DEBUGFUNC("\n");

	rxctrl = rd32(hw, TXGBE_RDB_PB_CTL);
	if (rxctrl & TXGBE_RDB_PB_CTL_RXEN) {
		pfdtxgswc = rd32(hw, TXGBE_PSR_CTL);
		if (pfdtxgswc & TXGBE_PSR_CTL_SW_EN) {
			pfdtxgswc &= ~TXGBE_PSR_CTL_SW_EN;
			wr32(hw, TXGBE_PSR_CTL, pfdtxgswc);
			hw->mac.set_lben = true;
		} else {
			hw->mac.set_lben = false;
		}
		rxctrl &= ~TXGBE_RDB_PB_CTL_RXEN;
		wr32(hw, TXGBE_RDB_PB_CTL, rxctrl);
		/* errata 14 */
		if (hw->revision_id == TXGBE_SP_MPW) {
			do {
				do {
					if (rd32m(hw,
						TXGBE_RDB_PB_CTL,
						TXGBE_RDB_PB_CTL_DISABLED) == 1)
						break;
					msleep(10);
				} while (1);
				if (rd32m(hw, TXGBE_RDB_TXSWERR,
					TXGBE_RDB_TXSWERR_TB_FREE) == 0x143)
					break;
				else {
					wr32m(hw,
						TXGBE_RDB_PB_CTL,
						TXGBE_RDB_PB_CTL_RXEN,
						TXGBE_RDB_PB_CTL_RXEN);
					wr32m(hw,
						TXGBE_RDB_PB_CTL,
						TXGBE_RDB_PB_CTL_RXEN,
						~TXGBE_RDB_PB_CTL_RXEN);

				}
			} while (1);
		}

		if (!(((hw->subsystem_device_id & TXGBE_NCSI_MASK) == TXGBE_NCSI_SUP)||
			((hw->subsystem_device_id & TXGBE_WOL_MASK) == TXGBE_WOL_SUP))){
			/* disable mac receiver */
			wr32m(hw, TXGBE_MAC_RX_CFG,
				TXGBE_MAC_RX_CFG_RE, 0);
		}
	}
}


void txgbe_enable_rx(struct txgbe_hw *hw)
{
	u32 pfdtxgswc;

	DEBUGFUNC("\n");

	/* enable mac receiver */
	wr32m(hw, TXGBE_MAC_RX_CFG,
		TXGBE_MAC_RX_CFG_RE, TXGBE_MAC_RX_CFG_RE);

	wr32m(hw, TXGBE_RDB_PB_CTL,
		TXGBE_RDB_PB_CTL_RXEN, TXGBE_RDB_PB_CTL_RXEN);

	if (hw->mac.set_lben) {
		pfdtxgswc = rd32(hw, TXGBE_PSR_CTL);
		pfdtxgswc |= TXGBE_PSR_CTL_SW_EN;
		wr32(hw, TXGBE_PSR_CTL, pfdtxgswc);
		hw->mac.set_lben = false;
	}
}

/**
 * txgbe_mng_present - returns true when management capability is present
 * @hw: pointer to hardware structure
 */
bool txgbe_mng_present(struct txgbe_hw *hw)
{
#ifndef SIMULATION_DEBUG

	u32 fwsm;

	fwsm = rd32(hw, TXGBE_MIS_ST);
	return fwsm & TXGBE_MIS_ST_MNG_INIT_DN;
#else
	UNREFERENCED_PARAMETER(hw);
	return false;
#endif
}

bool txgbe_check_mng_access(struct txgbe_hw *hw)
{
	bool ret = false;
	u32 rst_delay;
	u32 i;

	struct txgbe_adapter *adapter = hw->back;
	if (!txgbe_mng_present(hw))
		return false;
	if (adapter->hw.revision_id != TXGBE_SP_MPW)
		return true;
	if (!(adapter->flags2 & TXGBE_FLAG2_MNG_REG_ACCESS_DISABLED))
		return true;

	rst_delay = (rd32(&adapter->hw, TXGBE_MIS_RST_ST) &
			TXGBE_MIS_RST_ST_RST_INIT) >>
			TXGBE_MIS_RST_ST_RST_INI_SHIFT;
	for (i = 0; i < rst_delay + 2; i++) {
		if (!(adapter->flags2 & TXGBE_FLAG2_MNG_REG_ACCESS_DISABLED)) {
			ret = true;
			break;
		}
		msleep(100);
	}
	return ret;
}

/**
 *  txgbe_setup_mac_link_multispeed_fiber - Set MAC link speed
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: true when waiting for completion is needed
 *
 *  Set the link speed in the MAC and/or PHY register and restarts link.
 **/
s32 txgbe_setup_mac_link_multispeed_fiber(struct txgbe_hw *hw,
					  u32 speed,
					  bool autoneg_wait_to_complete)
{
	u32 link_speed = TXGBE_LINK_SPEED_UNKNOWN;
	u32 highest_link_speed = TXGBE_LINK_SPEED_UNKNOWN;
	s32 status = 0;
	u32 speedcnt = 0;
	u32 i = 0;
	bool autoneg, link_up = false;

	DEBUGFUNC("\n");

	/* Mask off requested but non-supported speeds */
	status = TCALL(hw, mac.ops.get_link_capabilities,
				&link_speed, &autoneg);
	if (status != 0)
		return status;

	speed &= link_speed;

	/* Try each speed one by one, highest priority first.  We do this in
	 * software because 10Gb fiber doesn't support speed autonegotiation.
	 */
	if (speed & TXGBE_LINK_SPEED_10GB_FULL) {
		speedcnt++;
		highest_link_speed = TXGBE_LINK_SPEED_10GB_FULL;

		/* If we already have link at this speed, just jump out */
		status = TCALL(hw, mac.ops.check_link,
					&link_speed, &link_up, false);
		if (status != 0)
			return status;

		if ((link_speed == TXGBE_LINK_SPEED_10GB_FULL) && link_up)
			goto out;

		/* Allow module to change analog characteristics (1G->10G) */
		msec_delay(40);

		status = TCALL(hw, mac.ops.setup_mac_link,
				TXGBE_LINK_SPEED_10GB_FULL,
				autoneg_wait_to_complete);
		if (status != 0)
			return status;

		/* Flap the Tx laser if it has not already been done */
		TCALL(hw, mac.ops.flap_tx_laser);

		/* Wait for the controller to acquire link.  Per IEEE 802.3ap,
		 * Section 73.10.2, we may have to wait up to 500ms if KR is
		 * attempted.  sapphire uses the same timing for 10g SFI.
		 */
		for (i = 0; i < 5; i++) {
			/* Wait for the link partner to also set speed */
			msec_delay(100);

			/* If we have link, just jump out */
			status = TCALL(hw, mac.ops.check_link,
						&link_speed, &link_up, false);
			if (status != 0)
				return status;

			if (link_up)
				goto out;
		}
	}

	if (speed & TXGBE_LINK_SPEED_1GB_FULL) {
		speedcnt++;
		if (highest_link_speed == TXGBE_LINK_SPEED_UNKNOWN)
			highest_link_speed = TXGBE_LINK_SPEED_1GB_FULL;

		/* If we already have link at this speed, just jump out */
		status = TCALL(hw, mac.ops.check_link,
					&link_speed, &link_up, false);
		if (status != 0)
			return status;

		if ((link_speed == TXGBE_LINK_SPEED_1GB_FULL) && link_up)
			goto out;

		/* Allow module to change analog characteristics (10G->1G) */
		msec_delay(40);

		status = TCALL(hw, mac.ops.setup_mac_link,
				TXGBE_LINK_SPEED_1GB_FULL,
				autoneg_wait_to_complete);
		if (status != 0)
			return status;

		/* Flap the Tx laser if it has not already been done */
		TCALL(hw, mac.ops.flap_tx_laser);

		/* Wait for the link partner to also set speed */
		msec_delay(100);

		/* If we have link, just jump out */
		status = TCALL(hw, mac.ops.check_link,
					&link_speed, &link_up, false);
		if (status != 0)
			return status;

		if (link_up)
			goto out;
	}

	/* We didn't get link.  Configure back to the highest speed we tried,
	 * (if there was more than one).  We call ourselves back with just the
	 * single highest speed that the user requested.
	 */
	if (speedcnt > 1)
		status = txgbe_setup_mac_link_multispeed_fiber(hw,
						      highest_link_speed,
						      autoneg_wait_to_complete);

out:
	/* Set autoneg_advertised value based on input link speed */
	hw->phy.autoneg_advertised = 0;

	if (speed & TXGBE_LINK_SPEED_10GB_FULL)
		hw->phy.autoneg_advertised |= TXGBE_LINK_SPEED_10GB_FULL;

	if (speed & TXGBE_LINK_SPEED_1GB_FULL)
		hw->phy.autoneg_advertised |= TXGBE_LINK_SPEED_1GB_FULL;

	return status;
}

int txgbe_check_flash_load(struct txgbe_hw *hw, u32 check_bit)
{
	u32 i = 0;
	u32 reg = 0;
	int err = 0;
	/* if there's flash existing */
	if (!(rd32(hw, TXGBE_SPI_STATUS) &
		TXGBE_SPI_STATUS_FLASH_BYPASS)) {
		/* wait hw load flash done */
		for (i = 0; i < TXGBE_MAX_FLASH_LOAD_POLL_TIME; i++) {
			reg = rd32(hw, TXGBE_SPI_ILDR_STATUS);
			if (!(reg & check_bit)) {
				/* done */
				break;
			}
			msleep(200);
		}
		if (i == TXGBE_MAX_FLASH_LOAD_POLL_TIME) {
			err = TXGBE_ERR_FLASH_LOADING_FAILED;
		}
	}
	return err;
}

/* The txgbe_ptype_lookup is used to convert from the 8-bit ptype in the
 * hardware to a bit-field that can be used by SW to more easily determine the
 * packet type.
 *
 * Macros are used to shorten the table lines and make this table human
 * readable.
 *
 * We store the PTYPE in the top byte of the bit field - this is just so that
 * we can check that the table doesn't have a row missing, as the index into
 * the table should be the PTYPE.
 *
 * Typical work flow:
 *
 * IF NOT txgbe_ptype_lookup[ptype].known
 * THEN
 *      Packet is unknown
 * ELSE IF txgbe_ptype_lookup[ptype].mac == TXGBE_DEC_PTYPE_MAC_IP
 *      Use the rest of the fields to look at the tunnels, inner protocols, etc
 * ELSE
 *      Use the enum txgbe_l2_ptypes to decode the packet type
 * ENDIF
 */

/* macro to make the table lines short */
#define TXGBE_PTT(ptype, mac, ip, etype, eip, proto, layer)\
	{       ptype, \
		1, \
		/* mac     */ TXGBE_DEC_PTYPE_MAC_##mac, \
		/* ip      */ TXGBE_DEC_PTYPE_IP_##ip, \
		/* etype   */ TXGBE_DEC_PTYPE_ETYPE_##etype, \
		/* eip     */ TXGBE_DEC_PTYPE_IP_##eip, \
		/* proto   */ TXGBE_DEC_PTYPE_PROT_##proto, \
		/* layer   */ TXGBE_DEC_PTYPE_LAYER_##layer }

#define TXGBE_UKN(ptype) \
		{ ptype, 0, 0, 0, 0, 0, 0, 0 }

/* Lookup table mapping the HW PTYPE to the bit field for decoding */
/* for ((pt=0;pt<256;pt++)); do printf "macro(0x%02X),\n" $pt; done */
txgbe_dptype txgbe_ptype_lookup[256] = {
	TXGBE_UKN(0x00),
	TXGBE_UKN(0x01),
	TXGBE_UKN(0x02),
	TXGBE_UKN(0x03),
	TXGBE_UKN(0x04),
	TXGBE_UKN(0x05),
	TXGBE_UKN(0x06),
	TXGBE_UKN(0x07),
	TXGBE_UKN(0x08),
	TXGBE_UKN(0x09),
	TXGBE_UKN(0x0A),
	TXGBE_UKN(0x0B),
	TXGBE_UKN(0x0C),
	TXGBE_UKN(0x0D),
	TXGBE_UKN(0x0E),
	TXGBE_UKN(0x0F),

	/* L2: mac */
	TXGBE_UKN(0x10),
	TXGBE_PTT(0x11, L2, NONE, NONE, NONE, NONE, PAY2),
	TXGBE_PTT(0x12, L2, NONE, NONE, NONE, TS,   PAY2),
	TXGBE_PTT(0x13, L2, NONE, NONE, NONE, NONE, PAY2),
	TXGBE_PTT(0x14, L2, NONE, NONE, NONE, NONE, PAY2),
	TXGBE_PTT(0x15, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x16, L2, NONE, NONE, NONE, NONE, PAY2),
	TXGBE_PTT(0x17, L2, NONE, NONE, NONE, NONE, NONE),

	/* L2: ethertype filter */
	TXGBE_PTT(0x18, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x19, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1A, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1B, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1C, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1D, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1E, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1F, L2, NONE, NONE, NONE, NONE, NONE),

	/* L3: ip non-tunnel */
	TXGBE_UKN(0x20),
	TXGBE_PTT(0x21, IP, FGV4, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x22, IP, IPV4, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x23, IP, IPV4, NONE, NONE, UDP,  PAY4),
	TXGBE_PTT(0x24, IP, IPV4, NONE, NONE, TCP,  PAY4),
	TXGBE_PTT(0x25, IP, IPV4, NONE, NONE, SCTP, PAY4),
	TXGBE_UKN(0x26),
	TXGBE_UKN(0x27),
	TXGBE_UKN(0x28),
	TXGBE_PTT(0x29, IP, FGV6, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x2A, IP, IPV6, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x2B, IP, IPV6, NONE, NONE, UDP,  PAY3),
	TXGBE_PTT(0x2C, IP, IPV6, NONE, NONE, TCP,  PAY4),
	TXGBE_PTT(0x2D, IP, IPV6, NONE, NONE, SCTP, PAY4),
	TXGBE_UKN(0x2E),
	TXGBE_UKN(0x2F),

	/* L2: fcoe */
	TXGBE_PTT(0x30, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x31, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x32, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x33, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x34, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_UKN(0x35),
	TXGBE_UKN(0x36),
	TXGBE_UKN(0x37),
	TXGBE_PTT(0x38, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x39, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x3A, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x3B, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x3C, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_UKN(0x3D),
	TXGBE_UKN(0x3E),
	TXGBE_UKN(0x3F),

	TXGBE_UKN(0x40),
	TXGBE_UKN(0x41),
	TXGBE_UKN(0x42),
	TXGBE_UKN(0x43),
	TXGBE_UKN(0x44),
	TXGBE_UKN(0x45),
	TXGBE_UKN(0x46),
	TXGBE_UKN(0x47),
	TXGBE_UKN(0x48),
	TXGBE_UKN(0x49),
	TXGBE_UKN(0x4A),
	TXGBE_UKN(0x4B),
	TXGBE_UKN(0x4C),
	TXGBE_UKN(0x4D),
	TXGBE_UKN(0x4E),
	TXGBE_UKN(0x4F),
	TXGBE_UKN(0x50),
	TXGBE_UKN(0x51),
	TXGBE_UKN(0x52),
	TXGBE_UKN(0x53),
	TXGBE_UKN(0x54),
	TXGBE_UKN(0x55),
	TXGBE_UKN(0x56),
	TXGBE_UKN(0x57),
	TXGBE_UKN(0x58),
	TXGBE_UKN(0x59),
	TXGBE_UKN(0x5A),
	TXGBE_UKN(0x5B),
	TXGBE_UKN(0x5C),
	TXGBE_UKN(0x5D),
	TXGBE_UKN(0x5E),
	TXGBE_UKN(0x5F),
	TXGBE_UKN(0x60),
	TXGBE_UKN(0x61),
	TXGBE_UKN(0x62),
	TXGBE_UKN(0x63),
	TXGBE_UKN(0x64),
	TXGBE_UKN(0x65),
	TXGBE_UKN(0x66),
	TXGBE_UKN(0x67),
	TXGBE_UKN(0x68),
	TXGBE_UKN(0x69),
	TXGBE_UKN(0x6A),
	TXGBE_UKN(0x6B),
	TXGBE_UKN(0x6C),
	TXGBE_UKN(0x6D),
	TXGBE_UKN(0x6E),
	TXGBE_UKN(0x6F),
	TXGBE_UKN(0x70),
	TXGBE_UKN(0x71),
	TXGBE_UKN(0x72),
	TXGBE_UKN(0x73),
	TXGBE_UKN(0x74),
	TXGBE_UKN(0x75),
	TXGBE_UKN(0x76),
	TXGBE_UKN(0x77),
	TXGBE_UKN(0x78),
	TXGBE_UKN(0x79),
	TXGBE_UKN(0x7A),
	TXGBE_UKN(0x7B),
	TXGBE_UKN(0x7C),
	TXGBE_UKN(0x7D),
	TXGBE_UKN(0x7E),
	TXGBE_UKN(0x7F),

	/* IPv4 --> IPv4/IPv6 */
	TXGBE_UKN(0x80),
	TXGBE_PTT(0x81, IP, IPV4, IPIP, FGV4, NONE, PAY3),
	TXGBE_PTT(0x82, IP, IPV4, IPIP, IPV4, NONE, PAY3),
	TXGBE_PTT(0x83, IP, IPV4, IPIP, IPV4, UDP,  PAY4),
	TXGBE_PTT(0x84, IP, IPV4, IPIP, IPV4, TCP,  PAY4),
	TXGBE_PTT(0x85, IP, IPV4, IPIP, IPV4, SCTP, PAY4),
	TXGBE_UKN(0x86),
	TXGBE_UKN(0x87),
	TXGBE_UKN(0x88),
	TXGBE_PTT(0x89, IP, IPV4, IPIP, FGV6, NONE, PAY3),
	TXGBE_PTT(0x8A, IP, IPV4, IPIP, IPV6, NONE, PAY3),
	TXGBE_PTT(0x8B, IP, IPV4, IPIP, IPV6, UDP,  PAY4),
	TXGBE_PTT(0x8C, IP, IPV4, IPIP, IPV6, TCP,  PAY4),
	TXGBE_PTT(0x8D, IP, IPV4, IPIP, IPV6, SCTP, PAY4),
	TXGBE_UKN(0x8E),
	TXGBE_UKN(0x8F),

	/* IPv4 --> GRE/NAT --> NONE/IPv4/IPv6 */
	TXGBE_PTT(0x90, IP, IPV4, IG, NONE, NONE, PAY3),
	TXGBE_PTT(0x91, IP, IPV4, IG, FGV4, NONE, PAY3),
	TXGBE_PTT(0x92, IP, IPV4, IG, IPV4, NONE, PAY3),
	TXGBE_PTT(0x93, IP, IPV4, IG, IPV4, UDP,  PAY4),
	TXGBE_PTT(0x94, IP, IPV4, IG, IPV4, TCP,  PAY4),
	TXGBE_PTT(0x95, IP, IPV4, IG, IPV4, SCTP, PAY4),
	TXGBE_UKN(0x96),
	TXGBE_UKN(0x97),
	TXGBE_UKN(0x98),
	TXGBE_PTT(0x99, IP, IPV4, IG, FGV6, NONE, PAY3),
	TXGBE_PTT(0x9A, IP, IPV4, IG, IPV6, NONE, PAY3),
	TXGBE_PTT(0x9B, IP, IPV4, IG, IPV6, UDP,  PAY4),
	TXGBE_PTT(0x9C, IP, IPV4, IG, IPV6, TCP,  PAY4),
	TXGBE_PTT(0x9D, IP, IPV4, IG, IPV6, SCTP, PAY4),
	TXGBE_UKN(0x9E),
	TXGBE_UKN(0x9F),

	/* IPv4 --> GRE/NAT --> MAC --> NONE/IPv4/IPv6 */
	TXGBE_PTT(0xA0, IP, IPV4, IGM, NONE, NONE, PAY3),
	TXGBE_PTT(0xA1, IP, IPV4, IGM, FGV4, NONE, PAY3),
	TXGBE_PTT(0xA2, IP, IPV4, IGM, IPV4, NONE, PAY3),
	TXGBE_PTT(0xA3, IP, IPV4, IGM, IPV4, UDP,  PAY4),
	TXGBE_PTT(0xA4, IP, IPV4, IGM, IPV4, TCP,  PAY4),
	TXGBE_PTT(0xA5, IP, IPV4, IGM, IPV4, SCTP, PAY4),
	TXGBE_UKN(0xA6),
	TXGBE_UKN(0xA7),
	TXGBE_UKN(0xA8),
	TXGBE_PTT(0xA9, IP, IPV4, IGM, FGV6, NONE, PAY3),
	TXGBE_PTT(0xAA, IP, IPV4, IGM, IPV6, NONE, PAY3),
	TXGBE_PTT(0xAB, IP, IPV4, IGM, IPV6, UDP,  PAY4),
	TXGBE_PTT(0xAC, IP, IPV4, IGM, IPV6, TCP,  PAY4),
	TXGBE_PTT(0xAD, IP, IPV4, IGM, IPV6, SCTP, PAY4),
	TXGBE_UKN(0xAE),
	TXGBE_UKN(0xAF),

	/* IPv4 --> GRE/NAT --> MAC+VLAN --> NONE/IPv4/IPv6 */
	TXGBE_PTT(0xB0, IP, IPV4, IGMV, NONE, NONE, PAY3),
	TXGBE_PTT(0xB1, IP, IPV4, IGMV, FGV4, NONE, PAY3),
	TXGBE_PTT(0xB2, IP, IPV4, IGMV, IPV4, NONE, PAY3),
	TXGBE_PTT(0xB3, IP, IPV4, IGMV, IPV4, UDP,  PAY4),
	TXGBE_PTT(0xB4, IP, IPV4, IGMV, IPV4, TCP,  PAY4),
	TXGBE_PTT(0xB5, IP, IPV4, IGMV, IPV4, SCTP, PAY4),
	TXGBE_UKN(0xB6),
	TXGBE_UKN(0xB7),
	TXGBE_UKN(0xB8),
	TXGBE_PTT(0xB9, IP, IPV4, IGMV, FGV6, NONE, PAY3),
	TXGBE_PTT(0xBA, IP, IPV4, IGMV, IPV6, NONE, PAY3),
	TXGBE_PTT(0xBB, IP, IPV4, IGMV, IPV6, UDP,  PAY4),
	TXGBE_PTT(0xBC, IP, IPV4, IGMV, IPV6, TCP,  PAY4),
	TXGBE_PTT(0xBD, IP, IPV4, IGMV, IPV6, SCTP, PAY4),
	TXGBE_UKN(0xBE),
	TXGBE_UKN(0xBF),

	/* IPv6 --> IPv4/IPv6 */
	TXGBE_UKN(0xC0),
	TXGBE_PTT(0xC1, IP, IPV6, IPIP, FGV4, NONE, PAY3),
	TXGBE_PTT(0xC2, IP, IPV6, IPIP, IPV4, NONE, PAY3),
	TXGBE_PTT(0xC3, IP, IPV6, IPIP, IPV4, UDP,  PAY4),
	TXGBE_PTT(0xC4, IP, IPV6, IPIP, IPV4, TCP,  PAY4),
	TXGBE_PTT(0xC5, IP, IPV6, IPIP, IPV4, SCTP, PAY4),
	TXGBE_UKN(0xC6),
	TXGBE_UKN(0xC7),
	TXGBE_UKN(0xC8),
	TXGBE_PTT(0xC9, IP, IPV6, IPIP, FGV6, NONE, PAY3),
	TXGBE_PTT(0xCA, IP, IPV6, IPIP, IPV6, NONE, PAY3),
	TXGBE_PTT(0xCB, IP, IPV6, IPIP, IPV6, UDP,  PAY4),
	TXGBE_PTT(0xCC, IP, IPV6, IPIP, IPV6, TCP,  PAY4),
	TXGBE_PTT(0xCD, IP, IPV6, IPIP, IPV6, SCTP, PAY4),
	TXGBE_UKN(0xCE),
	TXGBE_UKN(0xCF),

	/* IPv6 --> GRE/NAT -> NONE/IPv4/IPv6 */
	TXGBE_PTT(0xD0, IP, IPV6, IG,   NONE, NONE, PAY3),
	TXGBE_PTT(0xD1, IP, IPV6, IG,   FGV4, NONE, PAY3),
	TXGBE_PTT(0xD2, IP, IPV6, IG,   IPV4, NONE, PAY3),
	TXGBE_PTT(0xD3, IP, IPV6, IG,   IPV4, UDP,  PAY4),
	TXGBE_PTT(0xD4, IP, IPV6, IG,   IPV4, TCP,  PAY4),
	TXGBE_PTT(0xD5, IP, IPV6, IG,   IPV4, SCTP, PAY4),
	TXGBE_UKN(0xD6),
	TXGBE_UKN(0xD7),
	TXGBE_UKN(0xD8),
	TXGBE_PTT(0xD9, IP, IPV6, IG,   FGV6, NONE, PAY3),
	TXGBE_PTT(0xDA, IP, IPV6, IG,   IPV6, NONE, PAY3),
	TXGBE_PTT(0xDB, IP, IPV6, IG,   IPV6, UDP,  PAY4),
	TXGBE_PTT(0xDC, IP, IPV6, IG,   IPV6, TCP,  PAY4),
	TXGBE_PTT(0xDD, IP, IPV6, IG,   IPV6, SCTP, PAY4),
	TXGBE_UKN(0xDE),
	TXGBE_UKN(0xDF),

	/* IPv6 --> GRE/NAT -> MAC -> NONE/IPv4/IPv6 */
	TXGBE_PTT(0xE0, IP, IPV6, IGM,  NONE, NONE, PAY3),
	TXGBE_PTT(0xE1, IP, IPV6, IGM,  FGV4, NONE, PAY3),
	TXGBE_PTT(0xE2, IP, IPV6, IGM,  IPV4, NONE, PAY3),
	TXGBE_PTT(0xE3, IP, IPV6, IGM,  IPV4, UDP,  PAY4),
	TXGBE_PTT(0xE4, IP, IPV6, IGM,  IPV4, TCP,  PAY4),
	TXGBE_PTT(0xE5, IP, IPV6, IGM,  IPV4, SCTP, PAY4),
	TXGBE_UKN(0xE6),
	TXGBE_UKN(0xE7),
	TXGBE_UKN(0xE8),
	TXGBE_PTT(0xE9, IP, IPV6, IGM,  FGV6, NONE, PAY3),
	TXGBE_PTT(0xEA, IP, IPV6, IGM,  IPV6, NONE, PAY3),
	TXGBE_PTT(0xEB, IP, IPV6, IGM,  IPV6, UDP,  PAY4),
	TXGBE_PTT(0xEC, IP, IPV6, IGM,  IPV6, TCP,  PAY4),
	TXGBE_PTT(0xED, IP, IPV6, IGM,  IPV6, SCTP, PAY4),
	TXGBE_UKN(0xEE),
	TXGBE_UKN(0xEF),

	/* IPv6 --> GRE/NAT -> MAC--> NONE/IPv */
	TXGBE_PTT(0xF0, IP, IPV6, IGMV, NONE, NONE, PAY3),
	TXGBE_PTT(0xF1, IP, IPV6, IGMV, FGV4, NONE, PAY3),
	TXGBE_PTT(0xF2, IP, IPV6, IGMV, IPV4, NONE, PAY3),
	TXGBE_PTT(0xF3, IP, IPV6, IGMV, IPV4, UDP,  PAY4),
	TXGBE_PTT(0xF4, IP, IPV6, IGMV, IPV4, TCP,  PAY4),
	TXGBE_PTT(0xF5, IP, IPV6, IGMV, IPV4, SCTP, PAY4),
	TXGBE_UKN(0xF6),
	TXGBE_UKN(0xF7),
	TXGBE_UKN(0xF8),
	TXGBE_PTT(0xF9, IP, IPV6, IGMV, FGV6, NONE, PAY3),
	TXGBE_PTT(0xFA, IP, IPV6, IGMV, IPV6, NONE, PAY3),
	TXGBE_PTT(0xFB, IP, IPV6, IGMV, IPV6, UDP,  PAY4),
	TXGBE_PTT(0xFC, IP, IPV6, IGMV, IPV6, TCP,  PAY4),
	TXGBE_PTT(0xFD, IP, IPV6, IGMV, IPV6, SCTP, PAY4),
	TXGBE_UKN(0xFE),
	TXGBE_UKN(0xFF),
};


void txgbe_init_mac_link_ops(struct txgbe_hw *hw)
{
	struct txgbe_mac_info *mac = &hw->mac;

	DEBUGFUNC("\n");

	/*
	 * enable the laser control functions for SFP+ fiber
	 * and MNG not enabled
	 */
	if ((TCALL(hw, mac.ops.get_media_type) == txgbe_media_type_fiber) &&
	    !txgbe_mng_present(hw)) {
		mac->ops.disable_tx_laser =
				       txgbe_disable_tx_laser_multispeed_fiber;
		mac->ops.enable_tx_laser =
					txgbe_enable_tx_laser_multispeed_fiber;
		mac->ops.flap_tx_laser = txgbe_flap_tx_laser_multispeed_fiber;

	} else {
		mac->ops.disable_tx_laser =
				       txgbe_disable_tx_laser_multispeed_fiber;
		mac->ops.enable_tx_laser =
					txgbe_enable_tx_laser_multispeed_fiber;
		mac->ops.flap_tx_laser = txgbe_flap_tx_laser_multispeed_fiber;
	}

	if (hw->phy.multispeed_fiber) {
		/* Set up dual speed SFP+ support */
		mac->ops.setup_link = txgbe_setup_mac_link_multispeed_fiber;
		mac->ops.setup_mac_link = txgbe_setup_mac_link;
		mac->ops.set_rate_select_speed =
					       txgbe_set_hard_rate_select_speed;
	} else {
		mac->ops.setup_link = txgbe_setup_mac_link;
		mac->ops.set_rate_select_speed =
					       txgbe_set_hard_rate_select_speed;
	}
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
s32 txgbe_init_phy_ops(struct txgbe_hw *hw)
{
	struct txgbe_mac_info *mac = &hw->mac;
	s32 ret_val = 0;

	DEBUGFUNC("\n");

	txgbe_init_i2c(hw);
	/* Identify the PHY or SFP module */
	ret_val = TCALL(hw, phy.ops.identify);
	if (ret_val == TXGBE_ERR_SFP_NOT_SUPPORTED)
		goto init_phy_ops_out;

	/* Setup function pointers based on detected SFP module and speeds */
	txgbe_init_mac_link_ops(hw);
	if (hw->phy.sfp_type != txgbe_sfp_type_unknown)
		hw->phy.ops.reset = NULL;

	/* If copper media, overwrite with copper function pointers */
	if (TCALL(hw, mac.ops.get_media_type) == txgbe_media_type_copper) {
		hw->phy.type = txgbe_phy_xaui;
		if ((hw->subsystem_id & 0xF0) != TXGBE_ID_SFI_XAUI) {
			mac->ops.setup_link = txgbe_setup_copper_link;
			mac->ops.get_link_capabilities =
				  txgbe_get_copper_link_capabilities;
		}
	}

init_phy_ops_out:
	return ret_val;
}


/**
 *  txgbe_init_ops - Inits func ptrs and MAC type
 *  @hw: pointer to hardware structure
 *
 *  Initialize the function pointers and assign the MAC type for sapphire.
 *  Does not touch the hardware.
 **/

s32 txgbe_init_ops(struct txgbe_hw *hw)
{
	struct txgbe_mac_info *mac = &hw->mac;
	struct txgbe_phy_info *phy = &hw->phy;
	struct txgbe_eeprom_info *eeprom = &hw->eeprom;
	struct txgbe_flash_info *flash = &hw->flash;
	s32 ret_val = 0;

	DEBUGFUNC("\n");

	/* PHY */
	phy->ops.reset = txgbe_reset_phy;
	phy->ops.read_reg = txgbe_read_phy_reg;
	phy->ops.write_reg = txgbe_write_phy_reg;
	phy->ops.read_reg_mdi = txgbe_read_phy_reg_mdi;
	phy->ops.write_reg_mdi = txgbe_write_phy_reg_mdi;
	phy->ops.setup_link = txgbe_setup_phy_link;
	phy->ops.setup_link_speed = txgbe_setup_phy_link_speed;
	phy->ops.get_firmware_version = txgbe_get_phy_firmware_version;
	phy->ops.read_i2c_byte = txgbe_read_i2c_byte;
	phy->ops.write_i2c_byte = txgbe_write_i2c_byte;
	phy->ops.read_i2c_sff8472 = txgbe_read_i2c_sff8472;
	phy->ops.read_i2c_eeprom = txgbe_read_i2c_eeprom;
	phy->ops.write_i2c_eeprom = txgbe_write_i2c_eeprom;
	phy->ops.identify_sfp = txgbe_identify_module;
	phy->sfp_type = txgbe_sfp_type_unknown;
	phy->ops.check_overtemp = txgbe_tn_check_overtemp;
	phy->ops.identify = txgbe_identify_phy;
	phy->ops.init = txgbe_init_phy_ops;

	/* MAC */
	mac->ops.init_hw = txgbe_init_hw;
	mac->ops.clear_hw_cntrs = txgbe_clear_hw_cntrs;
	mac->ops.get_mac_addr = txgbe_get_mac_addr;
	mac->ops.stop_adapter = txgbe_stop_adapter;
	mac->ops.get_bus_info = txgbe_get_bus_info;
	mac->ops.set_lan_id = txgbe_set_lan_id_multi_port_pcie;
	mac->ops.acquire_swfw_sync = txgbe_acquire_swfw_sync;
	mac->ops.release_swfw_sync = txgbe_release_swfw_sync;
	mac->ops.reset_hw = txgbe_reset_hw;
	mac->ops.get_media_type = txgbe_get_media_type;
	mac->ops.disable_sec_rx_path = txgbe_disable_sec_rx_path;
	mac->ops.enable_sec_rx_path = txgbe_enable_sec_rx_path;
	mac->ops.enable_rx_dma = txgbe_enable_rx_dma;
	mac->ops.start_hw = txgbe_start_hw;
	mac->ops.get_san_mac_addr = txgbe_get_san_mac_addr;
	mac->ops.set_san_mac_addr = txgbe_set_san_mac_addr;
	mac->ops.get_device_caps = txgbe_get_device_caps;
	mac->ops.get_wwn_prefix = txgbe_get_wwn_prefix;
	mac->ops.setup_eee = txgbe_setup_eee;

	/* LEDs */
	mac->ops.led_on = txgbe_led_on;
	mac->ops.led_off = txgbe_led_off;

	/* RAR, Multicast, VLAN */
	mac->ops.set_rar = txgbe_set_rar;
	mac->ops.clear_rar = txgbe_clear_rar;
	mac->ops.init_rx_addrs = txgbe_init_rx_addrs;
	mac->ops.update_uc_addr_list = txgbe_update_uc_addr_list;
	mac->ops.update_mc_addr_list = txgbe_update_mc_addr_list;
	mac->ops.enable_mc = txgbe_enable_mc;
	mac->ops.disable_mc = txgbe_disable_mc;
	mac->ops.enable_rx = txgbe_enable_rx;
	mac->ops.disable_rx = txgbe_disable_rx;
	mac->ops.set_vmdq_san_mac = txgbe_set_vmdq_san_mac;
	mac->ops.insert_mac_addr = txgbe_insert_mac_addr;
	mac->rar_highwater = 1;
	mac->ops.set_vfta = txgbe_set_vfta;
	mac->ops.set_vlvf = txgbe_set_vlvf;
	mac->ops.clear_vfta = txgbe_clear_vfta;
	mac->ops.init_uta_tables = txgbe_init_uta_tables;
	mac->ops.set_mac_anti_spoofing = txgbe_set_mac_anti_spoofing;
	mac->ops.set_vlan_anti_spoofing = txgbe_set_vlan_anti_spoofing;
	mac->ops.set_ethertype_anti_spoofing =
				txgbe_set_ethertype_anti_spoofing;

	/* Flow Control */
	mac->ops.fc_enable = txgbe_fc_enable;
	mac->ops.setup_fc = txgbe_setup_fc;

	/* Link */
	mac->ops.get_link_capabilities = txgbe_get_link_capabilities;
	mac->ops.check_link = txgbe_check_mac_link;
	mac->ops.setup_rxpba = txgbe_set_rxpba;
	mac->mcft_size          = TXGBE_SP_MC_TBL_SIZE;
	mac->vft_size           = TXGBE_SP_VFT_TBL_SIZE;
	mac->num_rar_entries    = TXGBE_SP_RAR_ENTRIES;
	mac->rx_pb_size         = TXGBE_SP_RX_PB_SIZE;
	mac->max_rx_queues      = TXGBE_SP_MAX_RX_QUEUES;
	mac->max_tx_queues      = TXGBE_SP_MAX_TX_QUEUES;
	mac->max_msix_vectors   = txgbe_get_pcie_msix_count(hw);

	mac->arc_subsystem_valid = (rd32(hw, TXGBE_MIS_ST) &
				   TXGBE_MIS_ST_MNG_INIT_DN) ? true : false;

	hw->mbx.ops.init_params = txgbe_init_mbx_params_pf;

	/* EEPROM */
	eeprom->ops.init_params = txgbe_init_eeprom_params;
	eeprom->ops.calc_checksum = txgbe_calc_eeprom_checksum;
	eeprom->ops.read = txgbe_read_ee_hostif;
	eeprom->ops.read_buffer = txgbe_read_ee_hostif_buffer;
	eeprom->ops.write = txgbe_write_ee_hostif;
	eeprom->ops.write_buffer = txgbe_write_ee_hostif_buffer;
	eeprom->ops.update_checksum = txgbe_update_eeprom_checksum;
	eeprom->ops.validate_checksum = txgbe_validate_eeprom_checksum;

	/* FLASH */
	flash->ops.init_params = txgbe_init_flash_params;
	flash->ops.read_buffer = txgbe_read_flash_buffer;
	flash->ops.write_buffer = txgbe_write_flash_buffer;

	/* Manageability interface */
	mac->ops.set_fw_drv_ver = txgbe_set_fw_drv_ver;

	mac->ops.get_thermal_sensor_data =
					 txgbe_get_thermal_sensor_data;
	mac->ops.init_thermal_sensor_thresh =
				      txgbe_init_thermal_sensor_thresh;

	mac->ops.get_rtrup2tc = txgbe_dcb_get_rtrup2tc;

	return ret_val;
}

/**
 *  txgbe_get_link_capabilities - Determines link capabilities
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @autoneg: true when autoneg or autotry is enabled
 *
 *  Determines the link capabilities by reading the AUTOC register.
 **/
s32 txgbe_get_link_capabilities(struct txgbe_hw *hw,
				      u32 *speed,
				      bool *autoneg)
{
	s32 status = 0;
	u32 sr_pcs_ctl, sr_pma_mmd_ctl1, sr_an_mmd_ctl;
	u32 sr_an_mmd_adv_reg2;

	DEBUGFUNC("\n");

	/* Check if 1G SFP module. */
	if (hw->phy.sfp_type == txgbe_sfp_type_1g_cu_core0 ||
	    hw->phy.sfp_type == txgbe_sfp_type_1g_cu_core1 ||
	    hw->phy.sfp_type == txgbe_sfp_type_1g_lx_core0 ||
	    hw->phy.sfp_type == txgbe_sfp_type_1g_lx_core1 ||
	    hw->phy.sfp_type == txgbe_sfp_type_1g_sx_core0 ||
	    hw->phy.sfp_type == txgbe_sfp_type_1g_sx_core1) {
		*speed = TXGBE_LINK_SPEED_1GB_FULL;
		*autoneg = false;
	} else if (hw->phy.multispeed_fiber) {
		*speed = TXGBE_LINK_SPEED_10GB_FULL |
			  TXGBE_LINK_SPEED_1GB_FULL;
		*autoneg = true;
	}
	/* SFP */
	else if (txgbe_get_media_type(hw) == txgbe_media_type_fiber) {
		*speed = TXGBE_LINK_SPEED_10GB_FULL;
		*autoneg = false;
	}
	/* XAUI */
	else if ((txgbe_get_media_type(hw) == txgbe_media_type_copper) &&
			 ((hw->subsystem_id & 0xF0) == TXGBE_ID_XAUI ||
			 (hw->subsystem_id & 0xF0) == TXGBE_ID_SFI_XAUI)) {
		*speed = TXGBE_LINK_SPEED_10GB_FULL;
		*autoneg = false;
		hw->phy.link_mode = TXGBE_PHYSICAL_LAYER_10GBASE_T;
	}
	/* SGMII */
	else if ((hw->subsystem_id & 0xF0) == TXGBE_ID_SGMII) {
		*speed = TXGBE_LINK_SPEED_1GB_FULL |
			TXGBE_LINK_SPEED_100_FULL |
			TXGBE_LINK_SPEED_10_FULL;
		*autoneg = false;
		hw->phy.link_mode = TXGBE_PHYSICAL_LAYER_1000BASE_T |
				TXGBE_PHYSICAL_LAYER_100BASE_TX;
        /* MAC XAUI */
	} else if((hw->subsystem_id & 0xF0) == TXGBE_ID_MAC_XAUI) {
		*speed = TXGBE_LINK_SPEED_10GB_FULL;
		*autoneg = false;
		hw->phy.link_mode = TXGBE_PHYSICAL_LAYER_10GBASE_KX4;
		/* MAC SGMII */
    } else if ((hw->subsystem_id & 0xF0) == TXGBE_ID_MAC_SGMII) {
		*speed = TXGBE_LINK_SPEED_1GB_FULL;
		*autoneg = false;
		hw->phy.link_mode = TXGBE_PHYSICAL_LAYER_1000BASE_KX;
    }
	/* KR KX KX4 */
	else {
		/*
		 * Determine link capabilities based on the stored value,
		 * which represents EEPROM defaults.  If value has not
		 * been stored, use the current register values.
		 */
		if (hw->mac.orig_link_settings_stored) {
			sr_pcs_ctl = hw->mac.orig_sr_pcs_ctl2;
			sr_pma_mmd_ctl1 = hw->mac.orig_sr_pma_mmd_ctl1;
			sr_an_mmd_ctl = hw->mac.orig_sr_an_mmd_ctl;
			sr_an_mmd_adv_reg2 = hw->mac.orig_sr_an_mmd_adv_reg2;
		} else {
			sr_pcs_ctl = txgbe_rd32_epcs(hw, TXGBE_SR_PCS_CTL2);
			sr_pma_mmd_ctl1 = txgbe_rd32_epcs(hw,
						TXGBE_SR_PMA_MMD_CTL1);
			sr_an_mmd_ctl = txgbe_rd32_epcs(hw,
						TXGBE_SR_AN_MMD_CTL);
			sr_an_mmd_adv_reg2 = txgbe_rd32_epcs(hw,
						TXGBE_SR_AN_MMD_ADV_REG2);
		}

		if ((sr_pcs_ctl & TXGBE_SR_PCS_CTL2_PCS_TYPE_SEL_MASK) ==
			TXGBE_SR_PCS_CTL2_PCS_TYPE_SEL_X &&
			(sr_pma_mmd_ctl1 & TXGBE_SR_PMA_MMD_CTL1_SPEED_SEL_MASK)
			== TXGBE_SR_PMA_MMD_CTL1_SPEED_SEL_1G &&
			(sr_an_mmd_ctl & TXGBE_SR_AN_MMD_CTL_ENABLE) == 0) {
			/* 1G or KX - no backplane auto-negotiation */
			*speed = TXGBE_LINK_SPEED_1GB_FULL;
			*autoneg = false;
			hw->phy.link_mode = TXGBE_PHYSICAL_LAYER_1000BASE_KX;
		} else if ((sr_pcs_ctl & TXGBE_SR_PCS_CTL2_PCS_TYPE_SEL_MASK) ==
			TXGBE_SR_PCS_CTL2_PCS_TYPE_SEL_X &&
			(sr_pma_mmd_ctl1 & TXGBE_SR_PMA_MMD_CTL1_SPEED_SEL_MASK)
			== TXGBE_SR_PMA_MMD_CTL1_SPEED_SEL_10G &&
			(sr_an_mmd_ctl & TXGBE_SR_AN_MMD_CTL_ENABLE) == 0) {
			*speed = TXGBE_LINK_SPEED_10GB_FULL;
			*autoneg = false;
			hw->phy.link_mode = TXGBE_PHYSICAL_LAYER_10GBASE_KX4;
		} else if ((sr_pcs_ctl & TXGBE_SR_PCS_CTL2_PCS_TYPE_SEL_MASK) ==
			TXGBE_SR_PCS_CTL2_PCS_TYPE_SEL_R &&
			(sr_an_mmd_ctl & TXGBE_SR_AN_MMD_CTL_ENABLE) == 0) {
			/* 10 GbE serial link (KR -no backplane auto-negotiation) */
			*speed = TXGBE_LINK_SPEED_10GB_FULL;
			*autoneg = false;
			hw->phy.link_mode = TXGBE_PHYSICAL_LAYER_10GBASE_KR;
		} else if ((sr_an_mmd_ctl & TXGBE_SR_AN_MMD_CTL_ENABLE)) {
			/* KX/KX4/KR backplane auto-negotiation enable */
			*speed = TXGBE_LINK_SPEED_UNKNOWN;
			if (sr_an_mmd_adv_reg2 &
				TXGBE_SR_AN_MMD_ADV_REG2_BP_TYPE_KR)
				*speed |= TXGBE_LINK_SPEED_10GB_FULL;
			if (sr_an_mmd_adv_reg2 &
				TXGBE_SR_AN_MMD_ADV_REG2_BP_TYPE_KX4)
				*speed |= TXGBE_LINK_SPEED_10GB_FULL;
			if (sr_an_mmd_adv_reg2 &
				TXGBE_SR_AN_MMD_ADV_REG2_BP_TYPE_KX)
				*speed |= TXGBE_LINK_SPEED_1GB_FULL;
			*autoneg = true;
			hw->phy.link_mode = TXGBE_PHYSICAL_LAYER_10GBASE_KR |
					TXGBE_PHYSICAL_LAYER_10GBASE_KX4 |
					TXGBE_PHYSICAL_LAYER_1000BASE_KX;
		} else {
			status = TXGBE_ERR_LINK_SETUP;
			goto out;
		}
	}

out:
	return status;
}

/**
 *  txgbe_get_media_type - Get media type
 *  @hw: pointer to hardware structure
 *
 *  Returns the media type (fiber, copper, backplane)
 **/
enum txgbe_media_type txgbe_get_media_type(struct txgbe_hw *hw)
{
	enum txgbe_media_type media_type;
	u8 device_type = hw->subsystem_id & 0xF0;

	DEBUGFUNC("\n");

	/* Detect if there is a copper PHY attached. */
	switch (hw->phy.type) {
	case txgbe_phy_cu_unknown:
	case txgbe_phy_tn:
		media_type = txgbe_media_type_copper;
		goto out;
	default:
		break;
	}

	switch (device_type) {
	case TXGBE_ID_MAC_XAUI:
	case TXGBE_ID_MAC_SGMII:
	case TXGBE_ID_KR_KX_KX4:
		/* Default device ID is mezzanine card KX/KX4 */
		media_type = txgbe_media_type_backplane;
		break;
	case TXGBE_ID_SFP:
		media_type = txgbe_media_type_fiber;
		break;
	case TXGBE_ID_XAUI:
	case TXGBE_ID_SGMII:
		media_type = txgbe_media_type_copper;
		break;
	case TXGBE_ID_SFI_XAUI:
		if (hw->bus.lan_id == 0)
			media_type = txgbe_media_type_fiber;
		else
			media_type = txgbe_media_type_copper;
		break;
	default:
		media_type = txgbe_media_type_unknown;
		break;
	}
out:
	return media_type;
}

/**
 *  txgbe_stop_mac_link_on_d3 - Disables link on D3
 *  @hw: pointer to hardware structure
 *
 *  Disables link during D3 power down sequence.
 *
 **/
void txgbe_stop_mac_link_on_d3(struct txgbe_hw *hw)
{
	/* fix autoc2 */
	UNREFERENCED_PARAMETER(hw);
	return;
}


/**
 *  txgbe_disable_tx_laser_multispeed_fiber - Disable Tx laser
 *  @hw: pointer to hardware structure
 *
 *  The base drivers may require better control over SFP+ module
 *  PHY states.  This includes selectively shutting down the Tx
 *  laser on the PHY, effectively halting physical link.
 **/
void txgbe_disable_tx_laser_multispeed_fiber(struct txgbe_hw *hw)
{
	u32 esdp_reg = rd32(hw, TXGBE_GPIO_DR);

	if (!(TCALL(hw, mac.ops.get_media_type) == txgbe_media_type_fiber))
		return;
	/* Blocked by MNG FW so bail */
	txgbe_check_reset_blocked(hw);

	if (txgbe_close_notify(hw))
		/* over write led when ifconfig down */
		TCALL(hw, mac.ops.led_off, TXGBE_LED_LINK_UP | TXGBE_LED_LINK_10G | TXGBE_LED_LINK_1G | TXGBE_LED_LINK_ACTIVE);
	
	/* Disable Tx laser; allow 100us to go dark per spec */
	esdp_reg |= TXGBE_GPIO_DR_1 | TXGBE_GPIO_DR_0;
	wr32(hw, TXGBE_GPIO_DR, esdp_reg);
	TXGBE_WRITE_FLUSH(hw);
	usec_delay(100);
}

/**
 *  txgbe_enable_tx_laser_multispeed_fiber - Enable Tx laser
 *  @hw: pointer to hardware structure
 *
 *  The base drivers may require better control over SFP+ module
 *  PHY states.  This includes selectively turning on the Tx
 *  laser on the PHY, effectively starting physical link.
 **/
void txgbe_enable_tx_laser_multispeed_fiber(struct txgbe_hw *hw)
{
	if (!(TCALL(hw, mac.ops.get_media_type) == txgbe_media_type_fiber))
		return;
	if (txgbe_open_notify(hw))
		/* recover led configure when ifconfig up */
		wr32(hw, TXGBE_CFG_LED_CTL, 0);
	
	/* Enable Tx laser; allow 100ms to light up */
	wr32m(hw, TXGBE_GPIO_DR,
		TXGBE_GPIO_DR_0 | TXGBE_GPIO_DR_1, 0);
	TXGBE_WRITE_FLUSH(hw);
	msec_delay(100);
}

/**
 *  txgbe_flap_tx_laser_multispeed_fiber - Flap Tx laser
 *  @hw: pointer to hardware structure
 *
 *  When the driver changes the link speeds that it can support,
 *  it sets autotry_restart to true to indicate that we need to
 *  initiate a new autotry session with the link partner.  To do
 *  so, we set the speed then disable and re-enable the Tx laser, to
 *  alert the link partner that it also needs to restart autotry on its
 *  end.  This is consistent with true clause 37 autoneg, which also
 *  involves a loss of signal.
 **/
void txgbe_flap_tx_laser_multispeed_fiber(struct txgbe_hw *hw)
{
	DEBUGFUNC("\n");
	if (!(TCALL(hw, mac.ops.get_media_type) == txgbe_media_type_fiber))
		return;

	/* Blocked by MNG FW so bail */
	txgbe_check_reset_blocked(hw);

	if (hw->mac.autotry_restart) {
		txgbe_disable_tx_laser_multispeed_fiber(hw);
		txgbe_enable_tx_laser_multispeed_fiber(hw);
		hw->mac.autotry_restart = false;
	}
}

/**
 *  txgbe_set_hard_rate_select_speed - Set module link speed
 *  @hw: pointer to hardware structure
 *  @speed: link speed to set
 *
 *  Set module link speed via RS0/RS1 rate select pins.
 */
void txgbe_set_hard_rate_select_speed(struct txgbe_hw *hw,
					u32 speed)
{
	u32 esdp_reg = rd32(hw, TXGBE_GPIO_DR);

	switch (speed) {
	case TXGBE_LINK_SPEED_10GB_FULL:
		esdp_reg |= TXGBE_GPIO_DR_5 | TXGBE_GPIO_DR_4;
		break;
	case TXGBE_LINK_SPEED_1GB_FULL:
		esdp_reg &= ~(TXGBE_GPIO_DR_5 | TXGBE_GPIO_DR_4);
		break;
	default:
		DEBUGOUT("Invalid fixed module speed\n");
		return;
	}

	wr32(hw, TXGBE_GPIO_DDR,
		TXGBE_GPIO_DDR_5 | TXGBE_GPIO_DDR_4 |
		TXGBE_GPIO_DDR_1 | TXGBE_GPIO_DDR_0);

	wr32(hw, TXGBE_GPIO_DR, esdp_reg);

	TXGBE_WRITE_FLUSH(hw);
}

s32 txgbe_enable_rx_adapter(struct txgbe_hw *hw)
{
	u32 value;

	value = txgbe_rd32_epcs(hw, TXGBE_PHY_RX_EQ_CTL);
	value |= 1 << 12;
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL, value);

	value = 0;
	while (!(value >> 11)) {
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_RX_AD_ACK);
		msleep(1);
	}

	value = txgbe_rd32_epcs(hw, TXGBE_PHY_RX_EQ_CTL);
	value &= ~(1 << 12);
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL, value);

	return 0;
}

s32 txgbe_set_sgmii_an37_ability(struct txgbe_hw *hw)
{
	u32 value;

	txgbe_wr32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1, 0x3002);
	/* for sgmii + external phy, set to 0x0105 (phy sgmii mode) */
	/* for sgmii direct link, set to 0x010c (mac sgmii mode) */
	if ((hw->subsystem_id & 0xF0) == TXGBE_ID_MAC_SGMII) {
		txgbe_wr32_epcs(hw, TXGBE_SR_MII_MMD_AN_CTL, 0x010c);
	} else if ((hw->subsystem_id & 0xF0) == TXGBE_ID_SGMII ||
				(hw->subsystem_id & 0xF0) == TXGBE_ID_XAUI) {
		txgbe_wr32_epcs(hw, TXGBE_SR_MII_MMD_AN_CTL, 0x0105);
	}
	txgbe_wr32_epcs(hw, TXGBE_SR_MII_MMD_DIGI_CTL, 0x0200);
	value = txgbe_rd32_epcs(hw, TXGBE_SR_MII_MMD_CTL);
	value = (value & ~0x1200) | (0x1 << 12) | (0x1 << 9);
	txgbe_wr32_epcs(hw, TXGBE_SR_MII_MMD_CTL, value);
	return 0;
}


s32 txgbe_set_link_to_kr(struct txgbe_hw *hw, bool autoneg)
{
	u32 i;
	s32 status = 0;
	u32 value = 0;
	struct txgbe_adapter *adapter = hw->back;

	/* 1. Wait xpcs power-up good */
	for (i = 0; i < TXGBE_XPCS_POWER_GOOD_MAX_POLLING_TIME; i++) {
		if ((txgbe_rd32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS) &
			TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS_PSEQ_MASK) ==
			TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS_PSEQ_POWER_GOOD)
			break;
		msleep(10);
	}
	if (i == TXGBE_XPCS_POWER_GOOD_MAX_POLLING_TIME) {
		status = TXGBE_ERR_XPCS_POWER_UP_FAILED;
		goto out;
	}
	e_dev_info("It is set to kr.\n");

	txgbe_wr32_epcs(hw, 0x78001, 0x7);
	txgbe_wr32_epcs(hw, TXGBE_PHY_TX_POWER_ST_CTL, 0x00FC);
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_POWER_ST_CTL, 0x00FC);

	if (1) {
		/* 2. Disable xpcs AN-73 */
		
		if (adapter->backplane_an == 1){
			txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x3000);
			txgbe_wr32_epcs(hw, TXGBE_VR_AN_KR_MODE_CL, 0x1);
		} else {
			txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x0);
			txgbe_wr32_epcs(hw, TXGBE_VR_AN_KR_MODE_CL, 0x0);
		}
		/*
		txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_ADV_REG2, 0x80);
		txgbe_wr32_epcs(hw, 0x18037, 0x80);
		txgbe_wr32_epcs(hw, 0x18006, 0xffff);
		txgbe_wr32_epcs(hw, 0x18008, 0xA697);
		*/
		txgbe_wr32_epcs(hw, 0x70012, 0xc000 | txgbe_rd32_epcs(hw, 0x70012));
		if (KR_AN73_PRESET == 1) {
			txgbe_wr32_epcs(hw, 0x18037, 0x80 | txgbe_rd32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1));
		}

		if (KR_POLLING == 1) {
			txgbe_wr32_epcs(hw, 0x18006, 0xffff);
			txgbe_wr32_epcs(hw, 0x18008, 0xA697);
		}

		/* 3. Set VR_XS_PMA_Gen5_12G_MPLLA_CTRL3 Register */
		/* Bit[10:0](MPLLA_BANDWIDTH) = 11'd123 (default: 11'd16) */
		txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL3,
				TXGBE_PHY_MPLLA_CTL3_MULTIPLIER_BW_10GBASER_KR);

		/* 4. Set VR_XS_PMA_Gen5_12G_MISC_CTRL0 Register */
		/* Bit[12:8](RX_VREF_CTRL) = 5'hF (default: 5'h11) */
		txgbe_wr32_epcs(hw, TXGBE_PHY_MISC_CTL0,
				0xCF00);

		/* 5. Set VR_XS_PMA_Gen5_12G_RX_EQ_CTRL0 Register */
		/* Bit[15:8](VGA1/2_GAIN_0) = 8'h77, Bit[7:5](CTLE_POLE_0) = 3'h2
		 * Bit[4:0](CTLE_BOOST_0) = 4'hA 
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL0,
				0x774A);

		/* 6. Set VR_MII_Gen5_12G_RX_GENCTRL3 Register */
		/* Bit[2:0](LOS_TRSHLD_0) = 3'h4 (default: 3) */
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_GEN_CTL3,
				0x0004);
		/* 7. Initialize the mode by setting VR XS or PCS MMD Digital */
		/* Control1 Register Bit[15](VR_RST) */
		txgbe_wr32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1,
				0xA000);
		/* wait phy initialization done */
		for (i = 0; i < TXGBE_PHY_INIT_DONE_POLLING_TIME; i++) {
			if ((txgbe_rd32_epcs(hw,
				TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1) &
				TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1_VR_RST) == 0)
				break;
			msleep(100);
		}
		if (i == TXGBE_PHY_INIT_DONE_POLLING_TIME) {
			status = TXGBE_ERR_PHY_INIT_NOT_DONE;
			goto out;
		}
	} else {
		 txgbe_wr32_epcs(hw, TXGBE_VR_AN_KR_MODE_CL,
				0x1);
	}

	if (KR_SET == 1 || adapter->ffe_set == TXGBE_BP_M_KR) {
		e_dev_info("Set KR TX_EQ MAIN:%d PRE:%d POST:%d\n",
				adapter->ffe_main,adapter->ffe_pre,adapter->ffe_post);
		value = (0x1804 & ~0x3F3F);
		value |= adapter->ffe_main << 8 | adapter->ffe_pre;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0, value);

		value = (0x50 & ~0x7F) | (1 << 6)| adapter->ffe_post;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1, value);
	}
out:
	return status;
}

s32 txgbe_set_link_to_kx4(struct txgbe_hw *hw, bool autoneg)
{
	u32 i;
	s32 status = 0;
	u32 value;
	struct txgbe_adapter *adapter = hw->back;

	/* check link status, if already set, skip setting it again */
	if (hw->link_status == TXGBE_LINK_STATUS_KX4) {
		goto out;
	}
	e_dev_info("It is set to kx4.\n");
	txgbe_wr32_epcs(hw, 0x18035, 0x0000);
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_POWER_ST_CTL, 0x0000);

	/* 1. Wait xpcs power-up good */
	for (i = 0; i < TXGBE_XPCS_POWER_GOOD_MAX_POLLING_TIME; i++) {
		if ((txgbe_rd32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS) &
			TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS_PSEQ_MASK) ==
			TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS_PSEQ_POWER_GOOD)
			break;
		msleep(10);
	}
	if (i == TXGBE_XPCS_POWER_GOOD_MAX_POLLING_TIME) {
		status = TXGBE_ERR_XPCS_POWER_UP_FAILED;
		goto out;
	}

		wr32m(hw, TXGBE_MAC_TX_CFG, TXGBE_MAC_TX_CFG_TE,
			~TXGBE_MAC_TX_CFG_TE);

	/* 2. Disable xpcs AN-73 */
	if (!autoneg)
		txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x0);
	else
		txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x3000);

	if (hw->revision_id == TXGBE_SP_MPW) {
		/* Disable PHY MPLLA */
		txgbe_wr32_ephy(hw, 0x4, 0x2501);
		/* Reset rx lane0-3 clock */
		txgbe_wr32_ephy(hw, 0x1005, 0x4001);
		txgbe_wr32_ephy(hw, 0x1105, 0x4001);
		txgbe_wr32_ephy(hw, 0x1205, 0x4001);
		txgbe_wr32_ephy(hw, 0x1305, 0x4001);
	} else {
		/* Disable PHY MPLLA for eth mode change(after ECO) */
		txgbe_wr32_ephy(hw, 0x4, 0x250A);
		TXGBE_WRITE_FLUSH(hw);
		msleep(1);

		/* Set the eth change_mode bit first in mis_rst register
		 * for corresponding LAN port
		 */
		if (hw->bus.lan_id == 0)
			wr32(hw, TXGBE_MIS_RST,
				TXGBE_MIS_RST_LAN0_CHG_ETH_MODE);
		else
			wr32(hw, TXGBE_MIS_RST,
				TXGBE_MIS_RST_LAN1_CHG_ETH_MODE);
	}

	/* Set SR PCS Control2 Register Bits[1:0] = 2'b01  PCS_TYPE_SEL: non KR */
	txgbe_wr32_epcs(hw, TXGBE_SR_PCS_CTL2,
			TXGBE_SR_PCS_CTL2_PCS_TYPE_SEL_X);
	/* Set SR PMA MMD Control1 Register Bit[13] = 1'b1  SS13: 10G speed */
	txgbe_wr32_epcs(hw, TXGBE_SR_PMA_MMD_CTL1,
			TXGBE_SR_PMA_MMD_CTL1_SPEED_SEL_10G);

	value = (0xf5f0 & ~0x7F0) |  (0x5 << 8) | (0x7 << 5) | 0xF0;
	txgbe_wr32_epcs(hw, TXGBE_PHY_TX_GENCTRL1, value);

	if ((hw->subsystem_id & 0xF0) == TXGBE_ID_MAC_XAUI)
		txgbe_wr32_epcs(hw, TXGBE_PHY_MISC_CTL0, 0xCF00);
	else 
		txgbe_wr32_epcs(hw, TXGBE_PHY_MISC_CTL0, 0x4F00);

	for (i = 0; i < 4; i++) {
		if (i == 0)
			value = (0x45 & ~0xFFFF) | (0x7 << 12) | (0x7 << 8) | 0x6;
		else
			value = (0xff06 & ~0xFFFF) | (0x7 << 12) | (0x7 << 8) | 0x6;
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL0 + i, value);
	}

	value = 0x0 & ~0x7777;
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_ATT_LVL0, value);

	txgbe_wr32_epcs(hw, TXGBE_PHY_DFE_TAP_CTL0, 0x0);

	value = (0x6db & ~0xFFF) | (0x1 << 9) | (0x1 << 6) | (0x1 << 3) | 0x1;
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_GEN_CTL3, value);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY MPLLA */
	/* Control 0 Register Bit[7:0] = 8'd40  MPLLA_MULTIPLIER */
	txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL0,
			TXGBE_PHY_MPLLA_CTL0_MULTIPLIER_OTHER);
	/* Set VR XS, PMA or MII Synopsys Enterprise Gen5 12G PHY MPLLA */
	/* Control 3 Register Bit[10:0] = 11'd86  MPLLA_BANDWIDTH */
	txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL3,
			TXGBE_PHY_MPLLA_CTL3_MULTIPLIER_BW_OTHER);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY VCO */
	/* Calibration Load 0 Register  Bit[12:0] = 13'd1360 VCO_LD_VAL_0 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_LD0,
			TXGBE_PHY_VCO_CAL_LD0_OTHER);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY VCO */
	/* Calibration Load 1 Register  Bit[12:0] = 13'd1360 VCO_LD_VAL_1 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_LD1,
			TXGBE_PHY_VCO_CAL_LD0_OTHER);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY VCO */
	/* Calibration Load 2 Register  Bit[12:0] = 13'd1360 VCO_LD_VAL_2 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_LD2,
			TXGBE_PHY_VCO_CAL_LD0_OTHER);
	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY VCO */
	/* Calibration Load 3 Register  Bit[12:0] = 13'd1360 VCO_LD_VAL_3 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_LD3,
			TXGBE_PHY_VCO_CAL_LD0_OTHER);
	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY VCO */
	/* Calibration Reference 0 Register Bit[5:0] = 6'd34 VCO_REF_LD_0/1 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_REF0,
			0x2222);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY VCO */
	/* Calibration Reference 1 Register Bit[5:0] = 6'd34 VCO_REF_LD_2/3 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_REF1,
			0x2222);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY AFE-DFE */
	/* Enable Register Bit[7:0] = 8'd0  AFE_EN_0/3_1, DFE_EN_0/3_1 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_AFE_DFE_ENABLE,
			0x0);


	/* Set  VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Rx */
	/* Equalization Control 4 Register Bit[3:0] = 4'd0 CONT_ADAPT_0/3_1 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL,
			0x00F0);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Tx Rate */
	/* Control Register Bit[14:12], Bit[10:8], Bit[6:4], Bit[2:0],
	 * all rates to 3'b010  TX0/1/2/3_RATE
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_TX_RATE_CTL,
			0x2222);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Rx Rate */
	/* Control Register Bit[13:12], Bit[9:8], Bit[5:4], Bit[1:0],
	 * all rates to 2'b10  RX0/1/2/3_RATE
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_RATE_CTL,
			0x2222);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Tx General */
	/* Control 2 Register Bit[15:8] = 2'b01  TX0/1/2/3_WIDTH: 10bits */
	txgbe_wr32_epcs(hw, TXGBE_PHY_TX_GEN_CTL2,
			0x5500);
	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Rx General */
	/* Control 2 Register Bit[15:8] = 2'b01  RX0/1/2/3_WIDTH: 10bits */
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_GEN_CTL2,
			0x5500);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY MPLLA Control
	 * 2 Register Bit[10:8] = 3'b010
	 * MPLLA_DIV16P5_CLK_EN=0, MPLLA_DIV10_CLK_EN=1, MPLLA_DIV8_CLK_EN=0
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL2,
			TXGBE_PHY_MPLLA_CTL2_DIV_CLK_EN_10);

	txgbe_wr32_epcs(hw, 0x1f0000, 0x0);
	txgbe_wr32_epcs(hw, 0x1f8001, 0x0);
	txgbe_wr32_epcs(hw, TXGBE_SR_MII_MMD_DIGI_CTL, 0x0);

	if(KX4_TXRX_PIN == 1)
		txgbe_wr32_epcs(hw, 0x38001, 0xff);
	/* 10. Initialize the mode by setting VR XS or PCS MMD Digital Control1
	 * Register Bit[15](VR_RST)
	 */
	txgbe_wr32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1, 0xA000);
	/* wait phy initialization done */
	for (i = 0; i < TXGBE_PHY_INIT_DONE_POLLING_TIME; i++) {
		if ((txgbe_rd32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1) &
			TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1_VR_RST) == 0)
			break;
		msleep(100);
	}

	/* if success, set link status */
	hw->link_status = TXGBE_LINK_STATUS_KX4;

	if (i == TXGBE_PHY_INIT_DONE_POLLING_TIME) {
		status = TXGBE_ERR_PHY_INIT_NOT_DONE;
		goto out;
	}

	if (KX4_SET == 1 || adapter->ffe_set ) {
		e_dev_info("Set KX4 TX_EQ MAIN:%d PRE:%d POST:%d\n",
					adapter->ffe_main,adapter->ffe_pre,adapter->ffe_post);
		value = (0x1804 & ~0x3F3F);
		value |= adapter->ffe_main << 8 | adapter->ffe_pre;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0, value);

		value = (0x50 & ~0x7F) | (1 << 6)| adapter->ffe_post;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1, value);
	}

out:
	return status;

}


s32 txgbe_set_link_to_kx(struct txgbe_hw *hw,
			       u32 speed,
			       bool autoneg)
{
	u32 i;
	s32 status = 0;
	u32 wdata = 0;
	u32 value;
	struct txgbe_adapter *adapter = hw->back;

	/* check link status, if already set, skip setting it again */
	if (hw->link_status == TXGBE_LINK_STATUS_KX) {
		goto out;
	}
	e_dev_info("It is set to kx. speed =0x%x\n", speed);

	txgbe_wr32_epcs(hw, TXGBE_PHY_TX_POWER_ST_CTL, 0x00FC);
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_POWER_ST_CTL, 0x00FC);

	/* 1. Wait xpcs power-up good */
	for (i = 0; i < TXGBE_XPCS_POWER_GOOD_MAX_POLLING_TIME; i++) {
		if ((txgbe_rd32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS) &
			TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS_PSEQ_MASK) ==
			TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS_PSEQ_POWER_GOOD)
			break;
		msleep(10);
	}
	if (i == TXGBE_XPCS_POWER_GOOD_MAX_POLLING_TIME) {
		status = TXGBE_ERR_XPCS_POWER_UP_FAILED;
		goto out;
	}

	wr32m(hw, TXGBE_MAC_TX_CFG, TXGBE_MAC_TX_CFG_TE,
				~TXGBE_MAC_TX_CFG_TE);

	/* 2. Disable xpcs AN-73 */
	if (!autoneg)
		txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x0);
	else
		txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x3000);

	if (hw->revision_id == TXGBE_SP_MPW) {
		/* Disable PHY MPLLA */
		txgbe_wr32_ephy(hw, 0x4, 0x2401);
		/* Reset rx lane0 clock */
		txgbe_wr32_ephy(hw, 0x1005, 0x4001);
	} else {
		/* Disable PHY MPLLA for eth mode change(after ECO) */
		txgbe_wr32_ephy(hw, 0x4, 0x240A);
		TXGBE_WRITE_FLUSH(hw);
		msleep(1);

		/* Set the eth change_mode bit first in mis_rst register */
		/* for corresponding LAN port */
		if (hw->bus.lan_id == 0)
			wr32(hw, TXGBE_MIS_RST,
				TXGBE_MIS_RST_LAN0_CHG_ETH_MODE);
		else
			wr32(hw, TXGBE_MIS_RST,
				TXGBE_MIS_RST_LAN1_CHG_ETH_MODE);
	}

	/* Set SR PCS Control2 Register Bits[1:0] = 2'b01  PCS_TYPE_SEL: non KR */
	txgbe_wr32_epcs(hw, TXGBE_SR_PCS_CTL2,
			TXGBE_SR_PCS_CTL2_PCS_TYPE_SEL_X);

	/* Set SR PMA MMD Control1 Register Bit[13] = 1'b0 SS13: 1G speed */
	txgbe_wr32_epcs(hw, TXGBE_SR_PMA_MMD_CTL1,
			TXGBE_SR_PMA_MMD_CTL1_SPEED_SEL_1G);

	/* Set SR MII MMD Control Register to corresponding speed: {Bit[6],
	 * Bit[13]}=[2'b00,2'b01,2'b10]->[10M,100M,1G]
	 */
	if (speed == TXGBE_LINK_SPEED_100_FULL)
		wdata = 0x2100;
	else if (speed == TXGBE_LINK_SPEED_1GB_FULL)
		wdata = 0x0140;
	else if (speed == TXGBE_LINK_SPEED_10_FULL)
		wdata = 0x0100;
	txgbe_wr32_epcs(hw, TXGBE_SR_MII_MMD_CTL,
			wdata);

	value = (0xf5f0 & ~0x710) |  (0x5 << 8)| 0x10;
	txgbe_wr32_epcs(hw, TXGBE_PHY_TX_GENCTRL1, value);

	if (KX_SGMII == 1)
		txgbe_wr32_epcs(hw, TXGBE_PHY_MISC_CTL0, 0x4F00);
	else
		txgbe_wr32_epcs(hw, TXGBE_PHY_MISC_CTL0, 0xCF00);

	for (i = 0; i < 4; i++) {
		if (i) {
			value = 0xff06;
		} else {
			value = (0x45 & ~0xFFFF) | (0x7 << 12) | (0x7 << 8) | 0x6;
		}
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL0 + i, value);
	}

	value = 0x0 & ~0x7;
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_ATT_LVL0, value);

	txgbe_wr32_epcs(hw, TXGBE_PHY_DFE_TAP_CTL0, 0x0);

	value = (0x6db & ~0x7) | 0x4;
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_GEN_CTL3, value);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY MPLLA Control
	 * 0 Register Bit[7:0] = 8'd32  MPLLA_MULTIPLIER
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL0,
			TXGBE_PHY_MPLLA_CTL0_MULTIPLIER_1GBASEX_KX);

	/* Set VR XS, PMA or MII Synopsys Enterprise Gen5 12G PHY MPLLA Control 3
	 * Register Bit[10:0] = 11'd70  MPLLA_BANDWIDTH
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL3,
			TXGBE_PHY_MPLLA_CTL3_MULTIPLIER_BW_1GBASEX_KX);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY VCO
	 * Calibration Load 0 Register  Bit[12:0] = 13'd1344  VCO_LD_VAL_0
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_LD0,
			TXGBE_PHY_VCO_CAL_LD0_1GBASEX_KX);

	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_LD1, 0x549);
	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_LD2, 0x549);
	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_LD3, 0x549);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY VCO
	 * Calibration Reference 0 Register Bit[5:0] = 6'd42  VCO_REF_LD_0
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_REF0,
			TXGBE_PHY_VCO_CAL_REF0_LD0_1GBASEX_KX);

	txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_REF1, 0x2929);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY AFE-DFE Enable
	 * Register Bit[4], Bit[0] = 1'b0  AFE_EN_0, DFE_EN_0
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_AFE_DFE_ENABLE,
			0x0);
	/* Set	VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Rx
	 * Equalization Control 4 Register Bit[0] = 1'b0  CONT_ADAPT_0
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL,
			0x0010);
	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Tx Rate
	 * Control Register Bit[2:0] = 3'b011  TX0_RATE
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_TX_RATE_CTL,
			TXGBE_PHY_TX_RATE_CTL_TX0_RATE_1GBASEX_KX);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Rx Rate
	 * Control Register Bit[2:0] = 3'b011 RX0_RATE
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_RATE_CTL,
			TXGBE_PHY_RX_RATE_CTL_RX0_RATE_1GBASEX_KX);

	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Tx General
	 * Control 2 Register Bit[9:8] = 2'b01  TX0_WIDTH: 10bits
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_TX_GEN_CTL2,
			TXGBE_PHY_TX_GEN_CTL2_TX0_WIDTH_OTHER);
	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Rx General
	 * Control 2 Register Bit[9:8] = 2'b01  RX0_WIDTH: 10bits
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_RX_GEN_CTL2,
			TXGBE_PHY_RX_GEN_CTL2_RX0_WIDTH_OTHER);
	/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY MPLLA Control
	 * 2 Register Bit[10:8] = 3'b010	MPLLA_DIV16P5_CLK_EN=0,
	 * MPLLA_DIV10_CLK_EN=1, MPLLA_DIV8_CLK_EN=0
	 */
	txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL2,
			TXGBE_PHY_MPLLA_CTL2_DIV_CLK_EN_10);
	/* VR MII MMD AN Control Register Bit[8] = 1'b1 MII_CTRL */
	/* Set to 8bit MII (required in 10M/100M SGMII) */
	txgbe_wr32_epcs(hw, TXGBE_SR_MII_MMD_AN_CTL,
			0x0100);

	/* 10. Initialize the mode by setting VR XS or PCS MMD Digital Control1
	 * Register Bit[15](VR_RST)
	 */
	txgbe_wr32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1, 0xA000);
	/* wait phy initialization done */
	for (i = 0; i < TXGBE_PHY_INIT_DONE_POLLING_TIME; i++) {
		if ((txgbe_rd32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1) &
			TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1_VR_RST) == 0)
			break;
		msleep(100);
	}

	/* if success, set link status */
	hw->link_status = TXGBE_LINK_STATUS_KX;

	if (i == TXGBE_PHY_INIT_DONE_POLLING_TIME) {
		status = TXGBE_ERR_PHY_INIT_NOT_DONE;
		goto out;
	}

	if(KX_SET == 1 || adapter->ffe_set == TXGBE_BP_M_KX) {
		e_dev_info("Set KX TX_EQ MAIN:%d PRE:%d POST:%d\n",
					adapter->ffe_main,adapter->ffe_pre,adapter->ffe_post);
		/* 5. Set VR_XS_PMA_Gen5_12G_TX_EQ_CTRL0 Register Bit[13:8](TX_EQ_MAIN)
		* = 6'd30, Bit[5:0](TX_EQ_PRE) = 6'd4
		*/
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0);
		value = (value & ~0x3F3F) | (adapter->ffe_main << 8) | adapter->ffe_pre;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0, value);
		/* 6. Set VR_XS_PMA_Gen5_12G_TX_EQ_CTRL1 Register Bit[6](TX_EQ_OVR_RIDE)
		* = 1'b1, Bit[5:0](TX_EQ_POST) = 6'd36
		*/
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1);
		value = (value & ~0x7F) | adapter->ffe_post | (1 << 6);
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1, value);
	} 

out:
	return status;
}

s32 txgbe_set_link_to_sfi(struct txgbe_hw *hw,
			       u32 speed)
{
	u32 i;
	s32 status = 0;
	u32 value = 0;
	struct txgbe_adapter *adapter = hw->back;

	/* Set the module link speed */
	TCALL(hw, mac.ops.set_rate_select_speed,
		speed);
	
	/* 1. Wait xpcs power-up good */
	for (i = 0; i < TXGBE_XPCS_POWER_GOOD_MAX_POLLING_TIME; i++) {
		if ((txgbe_rd32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS) &
			TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS_PSEQ_MASK) ==
			TXGBE_VR_XS_OR_PCS_MMD_DIGI_STATUS_PSEQ_POWER_GOOD)
			break;
		msleep(10);
	}
	if (i == TXGBE_XPCS_POWER_GOOD_MAX_POLLING_TIME) {
		status = TXGBE_ERR_XPCS_POWER_UP_FAILED;
		goto out;
	}

	wr32m(hw, TXGBE_MAC_TX_CFG, TXGBE_MAC_TX_CFG_TE,
			~TXGBE_MAC_TX_CFG_TE);

	/* 2. Disable xpcs AN-73 */
	txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x0);

	if (hw->revision_id != TXGBE_SP_MPW) {
		/* Disable PHY MPLLA for eth mode change(after ECO) */
		txgbe_wr32_ephy(hw, 0x4, 0x243A);
		TXGBE_WRITE_FLUSH(hw);
		msleep(1);
		/* Set the eth change_mode bit first in mis_rst register
		 * for corresponding LAN port
		 */
		if (hw->bus.lan_id == 0)
			wr32(hw, TXGBE_MIS_RST,
				TXGBE_MIS_RST_LAN0_CHG_ETH_MODE);
		else
			wr32(hw, TXGBE_MIS_RST,
				TXGBE_MIS_RST_LAN1_CHG_ETH_MODE);
	}
	if (speed == TXGBE_LINK_SPEED_10GB_FULL) {
		/* @. Set SR PCS Control2 Register Bits[1:0] = 2'b00 PCS_TYPE_SEL: KR */
		txgbe_wr32_epcs(hw, TXGBE_SR_PCS_CTL2, 0);
		value = txgbe_rd32_epcs(hw, TXGBE_SR_PMA_MMD_CTL1);
		value = value | 0x2000;
		txgbe_wr32_epcs(hw, TXGBE_SR_PMA_MMD_CTL1, value);
		/* @. Set VR_XS_PMA_Gen5_12G_MPLLA_CTRL0 Register Bit[7:0] = 8'd33
		 * MPLLA_MULTIPLIER
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL0, 0x0021);
		/* 3. Set VR_XS_PMA_Gen5_12G_MPLLA_CTRL3 Register
		 * Bit[10:0](MPLLA_BANDWIDTH) = 11'd0
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL3, 0);
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_GENCTRL1);
		value = (value & ~0x700) | 0x500;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_GENCTRL1, value);
		/* 4.Set VR_XS_PMA_Gen5_12G_MISC_CTRL0 Register Bit[12:8](RX_VREF_CTRL)
		 * = 5'hF
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_MISC_CTL0, 0xCF00);
		/* @. Set VR_XS_PMA_Gen5_12G_VCO_CAL_LD0 Register  Bit[12:0] = 13'd1353
		 * VCO_LD_VAL_0
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_LD0, 0x0549);
		/* @. Set VR_XS_PMA_Gen5_12G_VCO_CAL_REF0 Register Bit[5:0] = 6'd41
		 * VCO_REF_LD_0
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_REF0, 0x0029);
		/* @. Set VR_XS_PMA_Gen5_12G_TX_RATE_CTRL Register Bit[2:0] = 3'b000
		 * TX0_RATE
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_RATE_CTL, 0);
		/* @. Set VR_XS_PMA_Gen5_12G_RX_RATE_CTRL Register Bit[2:0] = 3'b000
		 * RX0_RATE
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_RATE_CTL, 0);
		/* @. Set VR_XS_PMA_Gen5_12G_TX_GENCTRL2 Register Bit[9:8] = 2'b11
		 * TX0_WIDTH: 20bits
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_GEN_CTL2, 0x0300);
		/* @. Set VR_XS_PMA_Gen5_12G_RX_GENCTRL2 Register Bit[9:8] = 2'b11
		 * RX0_WIDTH: 20bits
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_GEN_CTL2, 0x0300);
		/* @. Set VR_XS_PMA_Gen5_12G_MPLLA_CTRL2 Register Bit[10:8] = 3'b110
		 * MPLLA_DIV16P5_CLK_EN=1, MPLLA_DIV10_CLK_EN=1, MPLLA_DIV8_CLK_EN=0
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL2, 0x0600);

		if (hw->phy.sfp_type == txgbe_sfp_type_da_cu_core0 ||
			hw->phy.sfp_type == txgbe_sfp_type_da_cu_core1) {
			/* 7. Set VR_XS_PMA_Gen5_12G_RX_EQ_CTRL0 Register
			 * Bit[15:8](VGA1/2_GAIN_0) = 8'h77, Bit[7:5]
			 * (CTLE_POLE_0) = 3'h2, Bit[4:0](CTLE_BOOST_0) = 4'hF
			 */
			txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL0, 0x774F);

		} else {
			/* 7. Set VR_XS_PMA_Gen5_12G_RX_EQ_CTRL0 Register Bit[15:8]
			 * (VGA1/2_GAIN_0) = 8'h00, Bit[7:5](CTLE_POLE_0) = 3'h2,
			 * Bit[4:0](CTLE_BOOST_0) = 4'hA
			 */
			value = txgbe_rd32_epcs(hw, TXGBE_PHY_RX_EQ_CTL0);
			value = (value & ~0xFFFF) | (2 << 5) | 0x05;
			txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL0, value);
		}
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_RX_EQ_ATT_LVL0);
		value = (value & ~0x7) | 0x0;
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_ATT_LVL0, value);

		if (hw->phy.sfp_type == txgbe_sfp_type_da_cu_core0 ||
			hw->phy.sfp_type == txgbe_sfp_type_da_cu_core1) {
			/* 8. Set VR_XS_PMA_Gen5_12G_DFE_TAP_CTRL0 Register Bit[7:0](DFE_TAP1_0)
			 * = 8'd20
			 */
			txgbe_wr32_epcs(hw, TXGBE_PHY_DFE_TAP_CTL0, 0x0014);
			value = txgbe_rd32_epcs(hw, TXGBE_PHY_AFE_DFE_ENABLE);
			value = (value & ~0x11) | 0x11;
			txgbe_wr32_epcs(hw, TXGBE_PHY_AFE_DFE_ENABLE, value);
		} else {
			/* 8. Set VR_XS_PMA_Gen5_12G_DFE_TAP_CTRL0 Register Bit[7:0](DFE_TAP1_0)
			 * = 8'd20
			 */
			txgbe_wr32_epcs(hw, TXGBE_PHY_DFE_TAP_CTL0, 0xBE);
			/* 9. Set VR_MII_Gen5_12G_AFE_DFE_EN_CTRL Register Bit[4](DFE_EN_0) =
			 * 1'b0, Bit[0](AFE_EN_0) = 1'b0
			 */
			value = txgbe_rd32_epcs(hw, TXGBE_PHY_AFE_DFE_ENABLE);
			value = (value & ~0x11) | 0x0;
			txgbe_wr32_epcs(hw, TXGBE_PHY_AFE_DFE_ENABLE, value);
		}
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_RX_EQ_CTL);
		value = value & ~0x1;
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL, value);
	} else {
		if (hw->revision_id == TXGBE_SP_MPW) {
			/* Disable PHY MPLLA */
			txgbe_wr32_ephy(hw, 0x4, 0x2401);
			/* Reset rx lane0 clock */
			txgbe_wr32_ephy(hw, 0x1005, 0x4001);
		}
		/* @. Set SR PCS Control2 Register Bits[1:0] = 2'b00 PCS_TYPE_SEL: KR */
		txgbe_wr32_epcs(hw, TXGBE_SR_PCS_CTL2, 0x1);
		/* Set SR PMA MMD Control1 Register Bit[13] = 1'b0  SS13: 1G speed */
		txgbe_wr32_epcs(hw, TXGBE_SR_PMA_MMD_CTL1, 0x0000);
		/* Set SR MII MMD Control Register to corresponding speed: */
		txgbe_wr32_epcs(hw, TXGBE_SR_MII_MMD_CTL, 0x0140);

		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_GENCTRL1);
		value = (value & ~0x710) | 0x500;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_GENCTRL1, value);
		/* 4. Set VR_XS_PMA_Gen5_12G_MISC_CTRL0 Register Bit[12:8](RX_VREF_CTRL)
		 * = 5'hF
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_MISC_CTL0, 0xCF00);
		/* 5. Set VR_XS_PMA_Gen5_12G_TX_EQ_CTRL0 Register Bit[13:8](TX_EQ_MAIN)
		 * = 6'd30, Bit[5:0](TX_EQ_PRE) = 6'd4
		 */
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0);
		value = (value & ~0x3F3F) | (24 << 8) | 4;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0, value);
		/* 6. Set VR_XS_PMA_Gen5_12G_TX_EQ_CTRL1 Register Bit[6](TX_EQ_OVR_RIDE)
		 * = 1'b1, Bit[5:0](TX_EQ_POST) = 6'd36
		 */
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1);
		value = (value & ~0x7F) | 16 | (1 << 6);
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1, value);
		if (hw->phy.sfp_type == txgbe_sfp_type_da_cu_core0 ||
			hw->phy.sfp_type == txgbe_sfp_type_da_cu_core1) {
			txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL0, 0x774F);
		} else {
			/* 7. Set VR_XS_PMA_Gen5_12G_RX_EQ_CTRL0 Register Bit[15:8]
			 * (VGA1/2_GAIN_0) = 8'h00, Bit[7:5](CTLE_POLE_0) = 3'h2,
			 * Bit[4:0](CTLE_BOOST_0) = 4'hA
			 */
			value = txgbe_rd32_epcs(hw, TXGBE_PHY_RX_EQ_CTL0);
			value = (value & ~0xFFFF) | 0x7706;
			txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL0, value);
		}
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_RX_EQ_ATT_LVL0);
		value = (value & ~0x7) | 0x0;
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_ATT_LVL0, value);
		/* 8. Set VR_XS_PMA_Gen5_12G_DFE_TAP_CTRL0 Register Bit[7:0](DFE_TAP1_0)
		 * = 8'd00
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_DFE_TAP_CTL0, 0x0);
		/* Set VR_XS_PMA_Gen5_12G_RX_GENCTRL3 Register Bit[2:0] LOS_TRSHLD_0 = 4 */
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_RX_GEN_CTL3);
		value = (value & ~0x7) | 0x4;
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_GEN_CTL3, value);
		/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY
		 * MPLLA Control 0 Register Bit[7:0] = 8'd32  MPLLA_MULTIPLIER
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL0, 0x0020);
		/* Set VR XS, PMA or MII Synopsys Enterprise Gen5 12G PHY MPLLA Control
		 * 3 Register Bit[10:0] = 11'd70  MPLLA_BANDWIDTH
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL3, 0x0046);
		/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY VCO 
		 * Calibration Load 0 Register  Bit[12:0] = 13'd1344  VCO_LD_VAL_0
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_LD0, 0x0540);
		/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY VCO 
		 * Calibration Reference 0 Register Bit[5:0] = 6'd42  VCO_REF_LD_0
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_VCO_CAL_REF0, 0x002A);
		/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY AFE-DFE 
		 * Enable Register Bit[4], Bit[0] = 1'b0  AFE_EN_0, DFE_EN_0
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_AFE_DFE_ENABLE, 0x0);
		/* Set  VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Rx 
		 * Equalization Control 4 Register Bit[0] = 1'b0  CONT_ADAPT_0
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_EQ_CTL, 0x0010);
		/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Tx Rate 
		 * Control Register Bit[2:0] = 3'b011  TX0_RATE
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_RATE_CTL, 0x0003);
		/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Rx Rate 
		 * Control Register Bit[2:0] = 3'b011 
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_RATE_CTL, 0x0003);
		/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Tx General 
		 * Control 2 Register Bit[9:8] = 2'b01  TX0_WIDTH: 10bits
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_GEN_CTL2, 0x0100);
		/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY Rx General 
		 * Control 2 Register Bit[9:8] = 2'b01  RX0_WIDTH: 10bits
		 */ 
		txgbe_wr32_epcs(hw, TXGBE_PHY_RX_GEN_CTL2, 0x0100);
		/* Set VR XS, PMA, or MII Synopsys Enterprise Gen5 12G PHY MPLLA 
		 * Control 2 Register Bit[10:8] = 3'b010  MPLLA_DIV16P5_CLK_EN=0, 
		 * MPLLA_DIV10_CLK_EN=1, MPLLA_DIV8_CLK_EN=0
		 */
		txgbe_wr32_epcs(hw, TXGBE_PHY_MPLLA_CTL2, 0x0200);
		/* VR MII MMD AN Control Register Bit[8] = 1'b1 MII_CTRL */
		txgbe_wr32_epcs(hw, TXGBE_SR_MII_MMD_AN_CTL, 0x0100);
	}
	/* 10. Initialize the mode by setting VR XS or PCS MMD Digital Control1
	 * Register Bit[15](VR_RST)
	 */
	txgbe_wr32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1, 0xA000);
	/* wait phy initialization done */
	for (i = 0; i < TXGBE_PHY_INIT_DONE_POLLING_TIME; i++) {
		if ((txgbe_rd32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1) &
			TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1_VR_RST) == 0)
			break;
		msleep(100);
	}
	if (i == TXGBE_PHY_INIT_DONE_POLLING_TIME) {
		status = TXGBE_ERR_PHY_INIT_NOT_DONE;
		goto out;
	}

	if(SFI_SET == 1 || adapter->ffe_set ) {
		e_dev_info("Set SFI TX_EQ MAIN:%d PRE:%d POST:%d\n",
					adapter->ffe_main,adapter->ffe_pre,adapter->ffe_post);
		/* 5. Set VR_XS_PMA_Gen5_12G_TX_EQ_CTRL0 Register Bit[13:8](TX_EQ_MAIN)
		* = 6'd30, Bit[5:0](TX_EQ_PRE) = 6'd4
		*/
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0);
		value = (value & ~0x3F3F) | (adapter->ffe_main << 8) | adapter->ffe_pre;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0, value);
		/* 6. Set VR_XS_PMA_Gen5_12G_TX_EQ_CTRL1 Register Bit[6](TX_EQ_OVR_RIDE)
		* = 1'b1, Bit[5:0](TX_EQ_POST) = 6'd36
		*/
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1);
		value = (value & ~0x7F) | adapter->ffe_post | (1 << 6);
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1, value);
	}
out:
	return status;
}


/**
 *  txgbe_setup_mac_link - Set MAC link speed
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: true when waiting for completion is needed
 *
 *  Set the link speed in the AUTOC register and restarts link.
 **/
s32 txgbe_setup_mac_link(struct txgbe_hw *hw,
			       u32 speed,
			       bool autoneg_wait_to_complete)
{
	bool autoneg = false;
	s32 status = 0;
	u32 link_capabilities = TXGBE_LINK_SPEED_UNKNOWN;
	struct txgbe_adapter *adapter = hw->back;
	u32 link_speed = TXGBE_LINK_SPEED_UNKNOWN;
	bool link_up = false;

	UNREFERENCED_PARAMETER(autoneg_wait_to_complete);
	DEBUGFUNC("\n");

	/* Check to see if speed passed in is supported. */
	status = TCALL(hw, mac.ops.get_link_capabilities,
			       &link_capabilities, &autoneg);
	if (status)
		goto out;

	speed &= link_capabilities;

	if (speed == TXGBE_LINK_SPEED_UNKNOWN) {
		status = TXGBE_ERR_LINK_SETUP;
		goto out;
	}

	if ( ! (((hw->subsystem_device_id & TXGBE_DEV_MASK) == TXGBE_ID_KR_KX_KX4) ||
			((hw->subsystem_device_id & TXGBE_DEV_MASK) == TXGBE_ID_MAC_XAUI) ||
			((hw->subsystem_device_id & TXGBE_DEV_MASK) == TXGBE_ID_MAC_SGMII))){
		status = TCALL(hw, mac.ops.check_link,
				&link_speed, &link_up, false);
		if (status != 0)
			goto out;
		if ((link_speed == speed) && link_up)
			goto out;
	}

	if ((hw->subsystem_id & TXGBE_DEV_MASK) == TXGBE_ID_KR_KX_KX4) {
		txgbe_set_link_to_kr(hw, autoneg);
#if 0
		if (!autoneg) {
			switch (hw->phy.link_mode) {
			case TXGBE_PHYSICAL_LAYER_10GBASE_KR:
				txgbe_set_link_to_kr(hw, autoneg);
				break;
			case TXGBE_PHYSICAL_LAYER_10GBASE_KX4:
				txgbe_set_link_to_kx4(hw, autoneg);
				break;
			case TXGBE_PHYSICAL_LAYER_1000BASE_KX:
				txgbe_set_link_to_kx(hw, speed, autoneg);
				break;
			default:
				status = TXGBE_ERR_PHY;
				goto out;
			}
		} else {
			txgbe_set_link_to_kr(hw, autoneg);
		}
#endif
	} else if ((hw->subsystem_id & 0xF0) == TXGBE_ID_XAUI ||
		((hw->subsystem_id & TXGBE_DEV_MASK) == TXGBE_ID_MAC_XAUI) ||
		(hw->subsystem_id & TXGBE_DEV_MASK) == TXGBE_ID_SGMII || 
		((hw->subsystem_id & TXGBE_DEV_MASK) == TXGBE_ID_MAC_SGMII) ||
		(txgbe_get_media_type(hw) == txgbe_media_type_copper && 
		(hw->subsystem_id & TXGBE_DEV_MASK) == TXGBE_ID_SFI_XAUI)) {
	if (speed == TXGBE_LINK_SPEED_10GB_FULL) {
		txgbe_set_link_to_kx4(hw, 0);
	} else {
		txgbe_set_link_to_kx(hw, speed, 0);
		if (adapter->an37)
			txgbe_set_sgmii_an37_ability(hw);
		}
	} else if (txgbe_get_media_type(hw) == txgbe_media_type_fiber) {
		txgbe_set_link_to_sfi(hw, speed);
	}

out:
	return status;
}

/**
 *  txgbe_setup_copper_link - Set the PHY autoneg advertised field
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: true if waiting is needed to complete
 *
 *  Restarts link on PHY and MAC based on settings passed in.
 **/
STATIC s32 txgbe_setup_copper_link(struct txgbe_hw *hw,
					 u32 speed,
					 bool autoneg_wait_to_complete)
{
	s32 status;
	u32 link_speed;

	DEBUGFUNC("\n");

	/* Setup the PHY according to input speed */
	link_speed = TCALL(hw, phy.ops.setup_link_speed, speed,
						autoneg_wait_to_complete);
	
	if (link_speed != TXGBE_LINK_SPEED_UNKNOWN)
		/* Set up MAC */
		status = txgbe_setup_mac_link(hw, link_speed, autoneg_wait_to_complete);
	else {
		status = 0;
	}
	return status;
}

int txgbe_reset_misc(struct txgbe_hw *hw)
{
	int i;
	u32 value;

	txgbe_init_i2c(hw);

	value = txgbe_rd32_epcs(hw, TXGBE_SR_PCS_CTL2);
	if ((value & 0x3) != TXGBE_SR_PCS_CTL2_PCS_TYPE_SEL_X) {
		hw->link_status = TXGBE_LINK_STATUS_NONE;
	}

	/* receive packets that size > 2048 */
	wr32m(hw, TXGBE_MAC_RX_CFG,
		TXGBE_MAC_RX_CFG_JE, TXGBE_MAC_RX_CFG_JE);

	/* clear counters on read */
	wr32m(hw, TXGBE_MMC_CONTROL,
		TXGBE_MMC_CONTROL_RSTONRD, TXGBE_MMC_CONTROL_RSTONRD);

	wr32m(hw, TXGBE_MAC_RX_FLOW_CTRL,
		TXGBE_MAC_RX_FLOW_CTRL_RFE, TXGBE_MAC_RX_FLOW_CTRL_RFE);

	wr32(hw, TXGBE_MAC_PKT_FLT,
		TXGBE_MAC_PKT_FLT_PR);

	wr32m(hw, TXGBE_MIS_RST_ST,
		TXGBE_MIS_RST_ST_RST_INIT, 0x1E00);

	/* errata 4: initialize mng flex tbl and wakeup flex tbl*/
	wr32(hw, TXGBE_PSR_MNG_FLEX_SEL, 0);
	for (i = 0; i < 16; i++) {
		wr32(hw, TXGBE_PSR_MNG_FLEX_DW_L(i), 0);
		wr32(hw, TXGBE_PSR_MNG_FLEX_DW_H(i), 0);
		wr32(hw, TXGBE_PSR_MNG_FLEX_MSK(i), 0);
	}
	wr32(hw, TXGBE_PSR_LAN_FLEX_SEL, 0);
	for (i = 0; i < 16; i++) {
		wr32(hw, TXGBE_PSR_LAN_FLEX_DW_L(i), 0);
		wr32(hw, TXGBE_PSR_LAN_FLEX_DW_H(i), 0);
		wr32(hw, TXGBE_PSR_LAN_FLEX_MSK(i), 0);
	}

	/* set pause frame dst mac addr */
	wr32(hw, TXGBE_RDB_PFCMACDAL, 0xC2000001);
	wr32(hw, TXGBE_RDB_PFCMACDAH, 0x0180);

	txgbe_init_thermal_sensor_thresh(hw);

	return 0;
}

/**
 *  txgbe_reset_hw - Perform hardware reset
 *  @hw: pointer to hardware structure
 *
 *  Resets the hardware by resetting the transmit and receive units, masks
 *  and clears all interrupts, perform a PHY reset, and perform a link (MAC)
 *  reset.
 **/
s32 txgbe_reset_hw(struct txgbe_hw *hw)
{
	s32 status;
	u32 reset = 0;
	u32 i;
#ifndef SIMULATION_DEBUG
	u32 sr_pcs_ctl, sr_pma_mmd_ctl1, sr_an_mmd_ctl, sr_an_mmd_adv_reg2;
	u32 vr_xs_or_pcs_mmd_digi_ctl1, curr_vr_xs_or_pcs_mmd_digi_ctl1;
	u32 curr_sr_pcs_ctl, curr_sr_pma_mmd_ctl1;
	u32 curr_sr_an_mmd_ctl, curr_sr_an_mmd_adv_reg2;
#endif
	u32 reset_status = 0;
	u32 rst_delay = 0;
	struct txgbe_adapter *adapter = hw->back;
	u32 value;

	DEBUGFUNC("\n");

	/* Call adapter stop to disable tx/rx and clear interrupts */
	status = TCALL(hw, mac.ops.stop_adapter);
	if (status != 0)
		goto reset_hw_out;

#ifndef SIMULATION_DEBUG

	/* Identify PHY and related function pointers */
	status = TCALL(hw, phy.ops.init);

	if (status == TXGBE_ERR_SFP_NOT_SUPPORTED)
		goto reset_hw_out;

	/* Reset PHY */
	if (txgbe_get_media_type(hw) == txgbe_media_type_copper)
		TCALL(hw, phy.ops.reset);

	/* remember internel phy regs from before we reset */
	curr_sr_pcs_ctl = txgbe_rd32_epcs(hw, TXGBE_SR_PCS_CTL2);
	curr_sr_pma_mmd_ctl1 = txgbe_rd32_epcs(hw, TXGBE_SR_PMA_MMD_CTL1);
	curr_sr_an_mmd_ctl = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_CTL);
	curr_sr_an_mmd_adv_reg2 = txgbe_rd32_epcs(hw,
						TXGBE_SR_AN_MMD_ADV_REG2);
	curr_vr_xs_or_pcs_mmd_digi_ctl1 =
		txgbe_rd32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1);

	/*
	 * Issue global reset to the MAC.  Needs to be SW reset if link is up.
	 * If link reset is used when link is up, it might reset the PHY when
	 * mng is using it.  If link is down or the flag to force full link
	 * reset is set, then perform link reset.
	 */
	if (hw->force_full_reset) {
		rst_delay = (rd32(hw, TXGBE_MIS_RST_ST) &
			TXGBE_MIS_RST_ST_RST_INIT) >>
			TXGBE_MIS_RST_ST_RST_INI_SHIFT;
		if (hw->reset_type == TXGBE_SW_RESET) {
			for (i = 0; i < rst_delay + 20; i++) {
				reset_status =
					rd32(hw, TXGBE_MIS_RST_ST);
				if (!(reset_status &
				    TXGBE_MIS_RST_ST_DEV_RST_ST_MASK))
					break;
				msleep(100);
			}

			if (reset_status & TXGBE_MIS_RST_ST_DEV_RST_ST_MASK) {
				status = TXGBE_ERR_RESET_FAILED;
				DEBUGOUT("Global reset polling failed to "
					"complete.\n");
				goto reset_hw_out;
			}
			status = txgbe_check_flash_load(hw,
						TXGBE_SPI_ILDR_STATUS_SW_RESET);
			if (status != 0)
				goto reset_hw_out;
			/* errata 7 */
			if (txgbe_mng_present(hw) &&
				hw->revision_id == TXGBE_SP_MPW) {
				struct txgbe_adapter *adapter =
					(struct txgbe_adapter *)hw->back;
				adapter->flags2 &=
					~TXGBE_FLAG2_MNG_REG_ACCESS_DISABLED;
			}
		} else if (hw->reset_type == TXGBE_GLOBAL_RESET) {
#ifndef _WIN32
			struct txgbe_adapter *adapter =
					(struct txgbe_adapter *)hw->back;
			msleep(100 * rst_delay + 2000);
			pci_restore_state(adapter->pdev);
			pci_save_state(adapter->pdev);
			pci_wake_from_d3(adapter->pdev, false);
#endif /*_WIN32*/
		}
	} else {
		if (txgbe_mng_present(hw)) {
			if (!(((hw->subsystem_device_id & TXGBE_NCSI_MASK) == TXGBE_NCSI_SUP)||
				((hw->subsystem_device_id & TXGBE_WOL_MASK) == TXGBE_WOL_SUP))){
				txgbe_reset_hostif(hw);
			}
		} else {

			if (hw->bus.lan_id == 0) {
				reset = TXGBE_MIS_RST_LAN0_RST;
			} else {
				reset = TXGBE_MIS_RST_LAN1_RST;
			}

			wr32(hw, TXGBE_MIS_RST,
				reset | rd32(hw, TXGBE_MIS_RST));
			TXGBE_WRITE_FLUSH(hw);
		}
		usec_delay(10);

		if (hw->bus.lan_id == 0) {
			status = txgbe_check_flash_load(hw,
					TXGBE_SPI_ILDR_STATUS_LAN0_SW_RST);
		} else {
			status = txgbe_check_flash_load(hw,
					TXGBE_SPI_ILDR_STATUS_LAN1_SW_RST);
		}
		if (status != 0)
			goto reset_hw_out;
	}

	status = txgbe_reset_misc(hw);
	if (status != 0)
		goto reset_hw_out;

	/*
	 * Store the original AUTOC/AUTOC2 values if they have not been
	 * stored off yet.  Otherwise restore the stored original
	 * values since the reset operation sets back to defaults.
	 */
	sr_pcs_ctl = txgbe_rd32_epcs(hw, TXGBE_SR_PCS_CTL2);
	sr_pma_mmd_ctl1 = txgbe_rd32_epcs(hw, TXGBE_SR_PMA_MMD_CTL1);
	sr_an_mmd_ctl = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_CTL);
	sr_an_mmd_adv_reg2 = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_ADV_REG2);
	vr_xs_or_pcs_mmd_digi_ctl1 =
		txgbe_rd32_epcs(hw, TXGBE_VR_XS_OR_PCS_MMD_DIGI_CTL1);

	if (hw->mac.orig_link_settings_stored == false) {
		hw->mac.orig_sr_pcs_ctl2 = sr_pcs_ctl;
		hw->mac.orig_sr_pma_mmd_ctl1 = sr_pma_mmd_ctl1;
		hw->mac.orig_sr_an_mmd_ctl = sr_an_mmd_ctl;
		hw->mac.orig_sr_an_mmd_adv_reg2 = sr_an_mmd_adv_reg2;
		hw->mac.orig_vr_xs_or_pcs_mmd_digi_ctl1 =
						vr_xs_or_pcs_mmd_digi_ctl1;
		hw->mac.orig_link_settings_stored = true;
	} else {

		/* If MNG FW is running on a multi-speed device that
		 * doesn't autoneg with out driver support we need to
		 * leave LMS in the state it was before we MAC reset.
		 * Likewise if we support WoL we don't want change the
		 * LMS state.
		 */

		hw->mac.orig_sr_pcs_ctl2 = curr_sr_pcs_ctl;
		hw->mac.orig_sr_pma_mmd_ctl1 = curr_sr_pma_mmd_ctl1;
		hw->mac.orig_sr_an_mmd_ctl = curr_sr_an_mmd_ctl;
		hw->mac.orig_sr_an_mmd_adv_reg2 =
					curr_sr_an_mmd_adv_reg2;
		hw->mac.orig_vr_xs_or_pcs_mmd_digi_ctl1 =
					curr_vr_xs_or_pcs_mmd_digi_ctl1;

	}
#else
#if 1
	hw->phy.type = txgbe_phy_sfp_intel;
	hw->phy.sfp_type = txgbe_sfp_type_1g_sx_core0;
	hw->phy.multispeed_fiber = false;
#endif
	if (status == TXGBE_ERR_SFP_NOT_SUPPORTED)
		goto reset_hw_out;

	if (hw->force_full_reset) {
		rst_delay = (rd32(hw, TXGBE_MIS_RST_ST) &
			TXGBE_MIS_RST_ST_RST_INIT) >>
			TXGBE_MIS_RST_ST_RST_INI_SHIFT;
		if (hw->reset_type == TXGBE_SW_RESET) {
			for (i = 0; i < rst_delay + 20; i++) {
				reset_status =
					rd32(hw, TXGBE_MIS_RST_ST);
				if (!(reset_status &
				    TXGBE_MIS_RST_ST_DEV_RST_ST_MASK))
					break;
				msleep(100);
			}

			if (reset_status & TXGBE_MIS_RST_ST_DEV_RST_ST_MASK) {
				status = TXGBE_ERR_RESET_FAILED;
				DEBUGOUT("Global reset polling failed to "
					"complete.\n");
				goto reset_hw_out;
			}

			msleep(100);
			/* errata 7 */
			if (txgbe_mng_present(hw) &&
				hw->revision_id == TXGBE_SP_MPW) {
				struct txgbe_adapter *adapter =
					(struct txgbe_adapter *)hw->back;
				adapter->flags2 &=
					~TXGBE_FLAG2_MNG_REG_ACCESS_DISABLED;
			}
		} else if (hw->reset_type == TXGBE_GLOBAL_RESET) {
#ifndef _WIN32
			struct txgbe_adapter *adapter =
					(struct txgbe_adapter *)hw->back;
			msleep(100 * rst_delay + 2000);
			pci_restore_state(adapter->pdev);
			pci_save_state(adapter->pdev);
			pci_wake_from_d3(adapter->pdev, false);
#endif /*_WIN32*/
		}

	} else {
		if (txgbe_mng_present(hw)) {
			txgbe_reset_hostif(hw);
		} else {

			if (hw->bus.lan_id == 0) {
				reset = TXGBE_MIS_RST_LAN0_RST;
			} else {
				reset = TXGBE_MIS_RST_LAN1_RST;
			}

			wr32(hw, TXGBE_MIS_RST,
				reset | rd32(hw, TXGBE_MIS_RST));
			TXGBE_WRITE_FLUSH(hw);
		}
		msleep(1000);
	}

	status = txgbe_reset_misc(hw);
	if (status != 0)
		goto reset_hw_out;

#endif

	/*make sure phy power is up*/
	msleep(100);
	/*A temporary solution for set to sfi*/
	if(SFI_SET == 1 || adapter->ffe_set == TXGBE_BP_M_SFI) {
		e_dev_info("Set SFI TX_EQ MAIN:%d PRE:%d POST:%d\n",
					adapter->ffe_main,adapter->ffe_pre,adapter->ffe_post);
		/* 5. Set VR_XS_PMA_Gen5_12G_TX_EQ_CTRL0 Register Bit[13:8](TX_EQ_MAIN)
		* = 6'd30, Bit[5:0](TX_EQ_PRE) = 6'd4
		*/
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0);
		value = (value & ~0x3F3F) | (adapter->ffe_main << 8) | adapter->ffe_pre;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0, value);
		/* 6. Set VR_XS_PMA_Gen5_12G_TX_EQ_CTRL1 Register Bit[6](TX_EQ_OVR_RIDE)
		* = 1'b1, Bit[5:0](TX_EQ_POST) = 6'd36
		*/
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1);
		value = (value & ~0x7F) | adapter->ffe_post | (1 << 6);
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1, value);
	}

	if (KR_SET == 1 || adapter->ffe_set == TXGBE_BP_M_KR) {
		e_dev_info("Set KR TX_EQ MAIN:%d PRE:%d POST:%d\n",
				adapter->ffe_main,adapter->ffe_pre,adapter->ffe_post);
		value = (0x1804 & ~0x3F3F);
		value |= adapter->ffe_main << 8 | adapter->ffe_pre;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0, value);

		value = (0x50 & ~0x7F) | (1 << 6)| adapter->ffe_post;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1, value);
	}

	if(KX_SET == 1 || adapter->ffe_set == TXGBE_BP_M_KX) {
		e_dev_info("Set KX TX_EQ MAIN:%d PRE:%d POST:%d\n",
					adapter->ffe_main,adapter->ffe_pre,adapter->ffe_post);
		/* 5. Set VR_XS_PMA_Gen5_12G_TX_EQ_CTRL0 Register Bit[13:8](TX_EQ_MAIN)
		* = 6'd30, Bit[5:0](TX_EQ_PRE) = 6'd4
		*/
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0);
		value = (value & ~0x3F3F) | (adapter->ffe_main << 8) | adapter->ffe_pre;
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL0, value);
		/* 6. Set VR_XS_PMA_Gen5_12G_TX_EQ_CTRL1 Register Bit[6](TX_EQ_OVR_RIDE)
		* = 1'b1, Bit[5:0](TX_EQ_POST) = 6'd36
		*/
		value = txgbe_rd32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1);
		value = (value & ~0x7F) | adapter->ffe_post | (1 << 6);
		txgbe_wr32_epcs(hw, TXGBE_PHY_TX_EQ_CTL1, value);
		
	}
	
	/* Store the permanent mac address */
	TCALL(hw, mac.ops.get_mac_addr, hw->mac.perm_addr);

	/*
	 * Store MAC address from RAR0, clear receive address registers, and
	 * clear the multicast table.  Also reset num_rar_entries to 128,
	 * since we modify this value when programming the SAN MAC address.
	 */
	hw->mac.num_rar_entries = 128;
	TCALL(hw, mac.ops.init_rx_addrs);

#ifndef SIMULATION_DEBUG
	/* Store the permanent SAN mac address */
	TCALL(hw, mac.ops.get_san_mac_addr, hw->mac.san_addr);

	/* Add the SAN MAC address to the RAR only if it's a valid address */
	if (txgbe_validate_mac_addr(hw->mac.san_addr) == 0) {
		TCALL(hw, mac.ops.set_rar, hw->mac.num_rar_entries - 1,
		      hw->mac.san_addr, 0, TXGBE_PSR_MAC_SWC_AD_H_AV);

		/* Save the SAN MAC RAR index */
		hw->mac.san_mac_rar_index = hw->mac.num_rar_entries - 1;

		/* Reserve the last RAR for the SAN MAC address */
		hw->mac.num_rar_entries--;
	}

	/* Store the alternative WWNN/WWPN prefix */
	TCALL(hw, mac.ops.get_wwn_prefix, &hw->mac.wwnn_prefix,
				   &hw->mac.wwpn_prefix);
#endif
	pci_set_master(((struct txgbe_adapter *)hw->back)->pdev);

reset_hw_out:
	return status;
}

#if defined(TXGBE_SUPPORT_KYLIN_FT)
void txgbe_disable_fdir(struct txgbe_hw *hw)
{
	int val = 0;

	if (!hw->Fdir_enabled) {
		wr32(hw, TXGBE_RDB_FDIR_CTL, 0x4AF07F18);
		TXGBE_WRITE_FLUSH(hw);
	}
	val = rd32(hw, TXGBE_RDB_FDIR_CTL);
}
#endif

/**
 * txgbe_fdir_check_cmd_complete - poll to check whether FDIRCMD is complete
 * @hw: pointer to hardware structure
 * @fdircmd: current value of FDIRCMD register
 */
STATIC s32 txgbe_fdir_check_cmd_complete(struct txgbe_hw *hw, u32 *fdircmd)
{
	int i;

	for (i = 0; i < TXGBE_RDB_FDIR_CMD_CMD_POLL; i++) {
		*fdircmd = rd32(hw, TXGBE_RDB_FDIR_CMD);
		if (!(*fdircmd & TXGBE_RDB_FDIR_CMD_CMD_MASK))
			return 0;
		usec_delay(10);
	}

	return TXGBE_ERR_FDIR_CMD_INCOMPLETE;
}

/**
 *  txgbe_reinit_fdir_tables - Reinitialize Flow Director tables.
 *  @hw: pointer to hardware structure
 **/
s32 txgbe_reinit_fdir_tables(struct txgbe_hw *hw)
{
	s32 err;
	int i;
	u32 fdirctrl = rd32(hw, TXGBE_RDB_FDIR_CTL);
	u32 fdircmd;
	fdirctrl &= ~TXGBE_RDB_FDIR_CTL_INIT_DONE;

	DEBUGFUNC("\n");

	/*
	 * Before starting reinitialization process,
	 * FDIRCMD.CMD must be zero.
	 */
	err = txgbe_fdir_check_cmd_complete(hw, &fdircmd);
	if (err) {
		DEBUGOUT("Flow Director previous command did not complete, "
			 "aborting table re-initialization.\n");
		return err;
	}

	wr32(hw, TXGBE_RDB_FDIR_FREE, 0);
	TXGBE_WRITE_FLUSH(hw);
	/*
	 * sapphire adapters flow director init flow cannot be restarted,
	 * Workaround sapphire silicon errata by performing the following steps
	 * before re-writing the FDIRCTRL control register with the same value.
	 * - write 1 to bit 8 of FDIRCMD register &
	 * - write 0 to bit 8 of FDIRCMD register
	 */
	wr32m(hw, TXGBE_RDB_FDIR_CMD,
		TXGBE_RDB_FDIR_CMD_CLEARHT, TXGBE_RDB_FDIR_CMD_CLEARHT);
	TXGBE_WRITE_FLUSH(hw);
	wr32m(hw, TXGBE_RDB_FDIR_CMD,
		TXGBE_RDB_FDIR_CMD_CLEARHT, 0);
	TXGBE_WRITE_FLUSH(hw);
	/*
	 * Clear FDIR Hash register to clear any leftover hashes
	 * waiting to be programmed.
	 */
	wr32(hw, TXGBE_RDB_FDIR_HASH, 0x00);
	TXGBE_WRITE_FLUSH(hw);

	wr32(hw, TXGBE_RDB_FDIR_CTL, fdirctrl);
	TXGBE_WRITE_FLUSH(hw);

	/* Poll init-done after we write FDIRCTRL register */
	for (i = 0; i < TXGBE_FDIR_INIT_DONE_POLL; i++) {
		if (rd32(hw, TXGBE_RDB_FDIR_CTL) &
				   TXGBE_RDB_FDIR_CTL_INIT_DONE)
			break;
		msec_delay(1);
	}
	if (i >= TXGBE_FDIR_INIT_DONE_POLL) {
		DEBUGOUT("Flow Director Signature poll time exceeded!\n");
		return TXGBE_ERR_FDIR_REINIT_FAILED;
	}

	/* Clear FDIR statistics registers (read to clear) */
	rd32(hw, TXGBE_RDB_FDIR_USE_ST);
	rd32(hw, TXGBE_RDB_FDIR_FAIL_ST);
	rd32(hw, TXGBE_RDB_FDIR_MATCH);
	rd32(hw, TXGBE_RDB_FDIR_MISS);
	rd32(hw, TXGBE_RDB_FDIR_LEN);

	return 0;
}

/**
 *  txgbe_fdir_enable - Initialize Flow Director control registers
 *  @hw: pointer to hardware structure
 *  @fdirctrl: value to write to flow director control register
 **/
STATIC void txgbe_fdir_enable(struct txgbe_hw *hw, u32 fdirctrl)
{
	int i;

	DEBUGFUNC("\n");

	/* Prime the keys for hashing */
	wr32(hw, TXGBE_RDB_FDIR_HKEY, TXGBE_ATR_BUCKET_HASH_KEY);
	wr32(hw, TXGBE_RDB_FDIR_SKEY, TXGBE_ATR_SIGNATURE_HASH_KEY);

	/*
	 * Poll init-done after we write the register.  Estimated times:
	 *      10G: PBALLOC = 11b, timing is 60us
	 *       1G: PBALLOC = 11b, timing is 600us
	 *     100M: PBALLOC = 11b, timing is 6ms
	 *
	 *     Multiple these timings by 4 if under full Rx load
	 *
	 * So we'll poll for TXGBE_FDIR_INIT_DONE_POLL times, sleeping for
	 * 1 msec per poll time.  If we're at line rate and drop to 100M, then
	 * this might not finish in our poll time, but we can live with that
	 * for now.
	 */
	wr32(hw, TXGBE_RDB_FDIR_CTL, fdirctrl);
	TXGBE_WRITE_FLUSH(hw);
	for (i = 0; i < TXGBE_RDB_FDIR_INIT_DONE_POLL; i++) {
		if (rd32(hw, TXGBE_RDB_FDIR_CTL) &
				   TXGBE_RDB_FDIR_CTL_INIT_DONE)
			break;
		msec_delay(1);
	}

	if (i >= TXGBE_RDB_FDIR_INIT_DONE_POLL)
		DEBUGOUT("Flow Director poll time exceeded!\n");
}

/**
 *  txgbe_init_fdir_signature -Initialize Flow Director sig filters
 *  @hw: pointer to hardware structure
 *  @fdirctrl: value to write to flow director control register, initially
 *           contains just the value of the Rx packet buffer allocation
 **/
s32 txgbe_init_fdir_signature(struct txgbe_hw *hw, u32 fdirctrl)
{
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)hw->back;
	int i = VMDQ_P(0) / 4;
	int j = VMDQ_P(0) % 4;
	u32 flex = rd32m(hw, TXGBE_RDB_FDIR_FLEX_CFG(i),
		~((TXGBE_RDB_FDIR_FLEX_CFG_BASE_MSK |
		TXGBE_RDB_FDIR_FLEX_CFG_MSK |
		TXGBE_RDB_FDIR_FLEX_CFG_OFST) <<
		(TXGBE_RDB_FDIR_FLEX_CFG_VM_SHIFT * j)));

	UNREFERENCED_PARAMETER(adapter);

	flex |= (TXGBE_RDB_FDIR_FLEX_CFG_BASE_MAC |
		0x6 << TXGBE_RDB_FDIR_FLEX_CFG_OFST_SHIFT) <<
		(TXGBE_RDB_FDIR_FLEX_CFG_VM_SHIFT * j);
	wr32(hw, TXGBE_RDB_FDIR_FLEX_CFG(i), flex);

	/*
	 * Continue setup of fdirctrl register bits:
	 *  Move the flexible bytes to use the ethertype - shift 6 words
	 *  Set the maximum length per hash bucket to 0xA filters
	 *  Send interrupt when 64 filters are left
	 */
	fdirctrl |= (0xF << TXGBE_RDB_FDIR_CTL_HASH_BITS_SHIFT) |
		    (0xA << TXGBE_RDB_FDIR_CTL_MAX_LENGTH_SHIFT) |
		    (4 << TXGBE_RDB_FDIR_CTL_FULL_THRESH_SHIFT);

	/* write hashes and fdirctrl register, poll for completion */
	txgbe_fdir_enable(hw, fdirctrl);

	if (hw->revision_id == TXGBE_SP_MPW) {
		/* errata 1: disable RSC of drop ring 0 */
		wr32m(hw, TXGBE_PX_RR_CFG(0),
				TXGBE_PX_RR_CFG_RSC, ~TXGBE_PX_RR_CFG_RSC);
	}
	return 0;
}

/**
 *  txgbe_init_fdir_perfect - Initialize Flow Director perfect filters
 *  @hw: pointer to hardware structure
 *  @fdirctrl: value to write to flow director control register, initially
 *           contains just the value of the Rx packet buffer allocation
 *  @cloud_mode: true - cloud mode, false - other mode
 **/
s32 txgbe_init_fdir_perfect(struct txgbe_hw *hw, u32 fdirctrl,
			bool cloud_mode)
{
	UNREFERENCED_PARAMETER(cloud_mode);
	DEBUGFUNC("\n");

	/*
	 * Continue setup of fdirctrl register bits:
	 *  Turn perfect match filtering on
	 *  Report hash in RSS field of Rx wb descriptor
	 *  Initialize the drop queue
	 *  Move the flexible bytes to use the ethertype - shift 6 words
	 *  Set the maximum length per hash bucket to 0xA filters
	 *  Send interrupt when 64 (0x4 * 16) filters are left
	 */
	fdirctrl |= TXGBE_RDB_FDIR_CTL_PERFECT_MATCH |
		    (TXGBE_RDB_FDIR_DROP_QUEUE <<
		    TXGBE_RDB_FDIR_CTL_DROP_Q_SHIFT) |
		    (0xF << TXGBE_RDB_FDIR_CTL_HASH_BITS_SHIFT) |
		    (0xA << TXGBE_RDB_FDIR_CTL_MAX_LENGTH_SHIFT) |
		    (4 << TXGBE_RDB_FDIR_CTL_FULL_THRESH_SHIFT);

	/* write hashes and fdirctrl register, poll for completion */
	txgbe_fdir_enable(hw, fdirctrl);

	if (hw->revision_id == TXGBE_SP_MPW) {
		if (((struct txgbe_adapter *)hw->back)->num_rx_queues >
			TXGBE_RDB_FDIR_DROP_QUEUE)
			/* errata 1: disable RSC of drop ring */
			wr32m(hw,
					TXGBE_PX_RR_CFG(TXGBE_RDB_FDIR_DROP_QUEUE),
					TXGBE_PX_RR_CFG_RSC, ~TXGBE_PX_RR_CFG_RSC);
	}
	return 0;
}

/*
 * These defines allow us to quickly generate all of the necessary instructions
 * in the function below by simply calling out TXGBE_COMPUTE_SIG_HASH_ITERATION
 * for values 0 through 15
 */
#define TXGBE_ATR_COMMON_HASH_KEY \
		(TXGBE_ATR_BUCKET_HASH_KEY & TXGBE_ATR_SIGNATURE_HASH_KEY)
#define TXGBE_COMPUTE_SIG_HASH_ITERATION(_n) \
do { \
	u32 n = (_n); \
	if (TXGBE_ATR_COMMON_HASH_KEY & (0x01 << n)) \
		common_hash ^= lo_hash_dword >> n; \
	else if (TXGBE_ATR_BUCKET_HASH_KEY & (0x01 << n)) \
		bucket_hash ^= lo_hash_dword >> n; \
	else if (TXGBE_ATR_SIGNATURE_HASH_KEY & (0x01 << n)) \
		sig_hash ^= lo_hash_dword << (16 - n); \
	if (TXGBE_ATR_COMMON_HASH_KEY & (0x01 << (n + 16))) \
		common_hash ^= hi_hash_dword >> n; \
	else if (TXGBE_ATR_BUCKET_HASH_KEY & (0x01 << (n + 16))) \
		bucket_hash ^= hi_hash_dword >> n; \
	else if (TXGBE_ATR_SIGNATURE_HASH_KEY & (0x01 << (n + 16))) \
		sig_hash ^= hi_hash_dword << (16 - n); \
} while (0)

/**
 *  txgbe_atr_compute_sig_hash - Compute the signature hash
 *  @stream: input bitstream to compute the hash on
 *
 *  This function is almost identical to the function above but contains
 *  several optimizations such as unwinding all of the loops, letting the
 *  compiler work out all of the conditional ifs since the keys are static
 *  defines, and computing two keys at once since the hashed dword stream
 *  will be the same for both keys.
 **/
u32 txgbe_atr_compute_sig_hash(union txgbe_atr_hash_dword input,
				     union txgbe_atr_hash_dword common)
{
	u32 hi_hash_dword, lo_hash_dword, flow_vm_vlan;
	u32 sig_hash = 0, bucket_hash = 0, common_hash = 0;

	/* record the flow_vm_vlan bits as they are a key part to the hash */
	flow_vm_vlan = TXGBE_NTOHL(input.dword);

	/* generate common hash dword */
	hi_hash_dword = TXGBE_NTOHL(common.dword);

	/* low dword is word swapped version of common */
	lo_hash_dword = (hi_hash_dword >> 16) | (hi_hash_dword << 16);

	/* apply flow ID/VM pool/VLAN ID bits to hash words */
	hi_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan >> 16);

	/* Process bits 0 and 16 */
	TXGBE_COMPUTE_SIG_HASH_ITERATION(0);

	/*
	 * apply flow ID/VM pool/VLAN ID bits to lo hash dword, we had to
	 * delay this because bit 0 of the stream should not be processed
	 * so we do not add the VLAN until after bit 0 was processed
	 */
	lo_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan << 16);

	/* Process remaining 30 bit of the key */
	TXGBE_COMPUTE_SIG_HASH_ITERATION(1);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(2);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(3);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(4);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(5);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(6);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(7);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(8);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(9);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(10);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(11);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(12);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(13);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(14);
	TXGBE_COMPUTE_SIG_HASH_ITERATION(15);

	/* combine common_hash result with signature and bucket hashes */
	bucket_hash ^= common_hash;
	bucket_hash &= TXGBE_ATR_HASH_MASK;

	sig_hash ^= common_hash << 16;
	sig_hash &= TXGBE_ATR_HASH_MASK << 16;

	/* return completed signature hash */
	return sig_hash ^ bucket_hash;
}

/**
 *  txgbe_atr_add_signature_filter - Adds a signature hash filter
 *  @hw: pointer to hardware structure
 *  @input: unique input dword
 *  @common: compressed common input dword
 *  @queue: queue index to direct traffic to
 **/
s32 txgbe_fdir_add_signature_filter(struct txgbe_hw *hw,
					  union txgbe_atr_hash_dword input,
					  union txgbe_atr_hash_dword common,
					  u8 queue)
{
	u32 fdirhashcmd = 0;
	u8 flow_type;
	u32 fdircmd;
	s32 err;

	DEBUGFUNC("\n");

	/*
	 * Get the flow_type in order to program FDIRCMD properly
	 * lowest 2 bits are FDIRCMD.L4TYPE, third lowest bit is FDIRCMD.IPV6
	 * fifth is FDIRCMD.TUNNEL_FILTER
	 */
	flow_type = input.formatted.flow_type;
	switch (flow_type) {
	case TXGBE_ATR_FLOW_TYPE_TCPV4:
	case TXGBE_ATR_FLOW_TYPE_UDPV4:
	case TXGBE_ATR_FLOW_TYPE_SCTPV4:
	case TXGBE_ATR_FLOW_TYPE_TCPV6:
	case TXGBE_ATR_FLOW_TYPE_UDPV6:
	case TXGBE_ATR_FLOW_TYPE_SCTPV6:
		break;
	default:
		DEBUGOUT(" Error on flow type input\n");
		return TXGBE_ERR_CONFIG;
	}

	/* configure FDIRCMD register */
	fdircmd = TXGBE_RDB_FDIR_CMD_CMD_ADD_FLOW |
		  TXGBE_RDB_FDIR_CMD_FILTER_UPDATE |
		  TXGBE_RDB_FDIR_CMD_LAST | TXGBE_RDB_FDIR_CMD_QUEUE_EN;
	fdircmd |= (u32)flow_type << TXGBE_RDB_FDIR_CMD_FLOW_TYPE_SHIFT;
	fdircmd |= (u32)queue << TXGBE_RDB_FDIR_CMD_RX_QUEUE_SHIFT;

	fdirhashcmd |= txgbe_atr_compute_sig_hash(input, common);
	fdirhashcmd |= 0x1 << TXGBE_RDB_FDIR_HASH_BUCKET_VALID_SHIFT;
	wr32(hw, TXGBE_RDB_FDIR_HASH, fdirhashcmd);

	wr32(hw, TXGBE_RDB_FDIR_CMD, fdircmd);

	err = txgbe_fdir_check_cmd_complete(hw, &fdircmd);
	if (err) {
		DEBUGOUT("Flow Director command did not complete!\n");
		return err;
	}

	DEBUGOUT2("Tx Queue=%x hash=%x\n", queue, (u32)fdirhashcmd);

	return 0;
}

#define TXGBE_COMPUTE_BKT_HASH_ITERATION(_n) \
do { \
	u32 n = (_n); \
	if (TXGBE_ATR_BUCKET_HASH_KEY & (0x01 << n)) \
		bucket_hash ^= lo_hash_dword >> n; \
	if (TXGBE_ATR_BUCKET_HASH_KEY & (0x01 << (n + 16))) \
		bucket_hash ^= hi_hash_dword >> n; \
} while (0)

/**
 *  txgbe_atr_compute_perfect_hash - Compute the perfect filter hash
 *  @atr_input: input bitstream to compute the hash on
 *  @input_mask: mask for the input bitstream
 *
 *  This function serves two main purposes.  First it applies the input_mask
 *  to the atr_input resulting in a cleaned up atr_input data stream.
 *  Secondly it computes the hash and stores it in the bkt_hash field at
 *  the end of the input byte stream.  This way it will be available for
 *  future use without needing to recompute the hash.
 **/
void txgbe_atr_compute_perfect_hash(union txgbe_atr_input *input,
					  union txgbe_atr_input *input_mask)
{
	u32 hi_hash_dword, lo_hash_dword, flow_vm_vlan;
	u32 bucket_hash = 0;
	u32 hi_dword = 0;
	u32 i = 0;

	/* Apply masks to input data */
	for (i = 0; i < 11; i++)
		input->dword_stream[i]  &= input_mask->dword_stream[i];

	/* record the flow_vm_vlan bits as they are a key part to the hash */
	flow_vm_vlan = TXGBE_NTOHL(input->dword_stream[0]);

	/* generate common hash dword */
	for (i = 1; i <= 10; i++)
		hi_dword ^= input->dword_stream[i];
	hi_hash_dword = TXGBE_NTOHL(hi_dword);

	/* low dword is word swapped version of common */
	lo_hash_dword = (hi_hash_dword >> 16) | (hi_hash_dword << 16);

	/* apply flow ID/VM pool/VLAN ID bits to hash words */
	hi_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan >> 16);

	/* Process bits 0 and 16 */
	TXGBE_COMPUTE_BKT_HASH_ITERATION(0);

	/*
	 * apply flow ID/VM pool/VLAN ID bits to lo hash dword, we had to
	 * delay this because bit 0 of the stream should not be processed
	 * so we do not add the VLAN until after bit 0 was processed
	 */
	lo_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan << 16);

	/* Process remaining 30 bit of the key */
	for (i = 1; i <= 15; i++)
		TXGBE_COMPUTE_BKT_HASH_ITERATION(i);

	/*
	 * Limit hash to 13 bits since max bucket count is 8K.
	 * Store result at the end of the input stream.
	 */
	input->formatted.bkt_hash = bucket_hash & 0x1FFF;
}

/**
 *  txgbe_get_fdirtcpm - generate a TCP port from atr_input_masks
 *  @input_mask: mask to be bit swapped
 *
 *  The source and destination port masks for flow director are bit swapped
 *  in that bit 15 effects bit 0, 14 effects 1, 13, 2 etc.  In order to
 *  generate a correctly swapped value we need to bit swap the mask and that
 *  is what is accomplished by this function.
 **/
STATIC u32 txgbe_get_fdirtcpm(union txgbe_atr_input *input_mask)
{
	u32 mask = TXGBE_NTOHS(input_mask->formatted.dst_port);
	mask <<= TXGBE_RDB_FDIR_TCP_MSK_DPORTM_SHIFT;
	mask |= TXGBE_NTOHS(input_mask->formatted.src_port);

	return mask;
}

/*
 * These two macros are meant to address the fact that we have registers
 * that are either all or in part big-endian.  As a result on big-endian
 * systems we will end up byte swapping the value to little-endian before
 * it is byte swapped again and written to the hardware in the original
 * big-endian format.
 */
#define TXGBE_STORE_AS_BE32(_value) \
	(((u32)(_value) >> 24) | (((u32)(_value) & 0x00FF0000) >> 8) | \
	 (((u32)(_value) & 0x0000FF00) << 8) | ((u32)(_value) << 24))

#define TXGBE_WRITE_REG_BE32(a, reg, value) \
	wr32((a), (reg), TXGBE_STORE_AS_BE32(TXGBE_NTOHL(value)))

#define TXGBE_STORE_AS_BE16(_value) \
	TXGBE_NTOHS(((u16)(_value) >> 8) | ((u16)(_value) << 8))

s32 txgbe_fdir_set_input_mask(struct txgbe_hw *hw,
				       union txgbe_atr_input *input_mask,
				       bool cloud_mode)
{
	/* mask IPv6 since it is currently not supported */
	u32 fdirm = 0;
	u32 fdirtcpm;
	u32 flex = 0;
	int i, j;
	struct txgbe_adapter *adapter = (struct txgbe_adapter *)hw->back;

	UNREFERENCED_PARAMETER(cloud_mode);
	UNREFERENCED_PARAMETER(adapter);

	DEBUGFUNC("\n");

	/*
	 * Program the relevant mask registers.  If src/dst_port or src/dst_addr
	 * are zero, then assume a full mask for that field.  Also assume that
	 * a VLAN of 0 is unspecified, so mask that out as well.  L4type
	 * cannot be masked out in this implementation.
	 *
	 * This also assumes IPv4 only.  IPv6 masking isn't supported at this
	 * point in time.
	 */

	/* verify bucket hash is cleared on hash generation */
	if (input_mask->formatted.bkt_hash)
		DEBUGOUT(" bucket hash should always be 0 in mask\n");

	/* Program FDIRM and verify partial masks */
	switch (input_mask->formatted.vm_pool & 0x7F) {
	case 0x0:
		fdirm |= TXGBE_RDB_FDIR_OTHER_MSK_POOL;
	case 0x7F:
		break;
	default:
		DEBUGOUT(" Error on vm pool mask\n");
		return TXGBE_ERR_CONFIG;
	}

	switch (input_mask->formatted.flow_type & TXGBE_ATR_L4TYPE_MASK) {
	case 0x0:
		fdirm |= TXGBE_RDB_FDIR_OTHER_MSK_L4P;
		if (input_mask->formatted.dst_port ||
		    input_mask->formatted.src_port) {
			DEBUGOUT(" Error on src/dst port mask\n");
			return TXGBE_ERR_CONFIG;
		}
	case TXGBE_ATR_L4TYPE_MASK:
		break;
	default:
		DEBUGOUT(" Error on flow type mask\n");
		return TXGBE_ERR_CONFIG;
	}

#if 0
	/* need fix */
	switch (input_mask->formatted.tunnel_type & 0xFFFFFFFF) {
	case 0x0:
		/* Mask turnnel type, fall through */
		fdirm |= TXGBE_RDB_FDIR_OTHER_MSK_TUN |
			TXGBE_RDB_FDIR_OTHER_MSK_TUN_OUTIP |
			TXGBE_RDB_FDIR_OTHER_MSK_TUN_TYPE;
	case 0xFFFFFFFF:
		break;
	default:
		DEBUGOUT(" Error on tunnel type byte mask\n");
		return TXGBE_ERR_CONFIG;
	}
#endif

	/* Now mask VM pool and destination IPv6 - bits 5 and 2 */
	wr32(hw, TXGBE_RDB_FDIR_OTHER_MSK, fdirm);

	i = VMDQ_P(0) / 4;
	j = VMDQ_P(0) % 4;
	flex = rd32m(hw, TXGBE_RDB_FDIR_FLEX_CFG(i),
		~((TXGBE_RDB_FDIR_FLEX_CFG_BASE_MSK |
		   TXGBE_RDB_FDIR_FLEX_CFG_MSK |
		   TXGBE_RDB_FDIR_FLEX_CFG_OFST) <<
		   (TXGBE_RDB_FDIR_FLEX_CFG_VM_SHIFT * j)));
	flex |= (TXGBE_RDB_FDIR_FLEX_CFG_BASE_MAC |
		0x6 << TXGBE_RDB_FDIR_FLEX_CFG_OFST_SHIFT) <<
		(TXGBE_RDB_FDIR_FLEX_CFG_VM_SHIFT * j);

	switch (input_mask->formatted.flex_bytes & 0xFFFF) {
	case 0x0000:
		/* Mask Flex Bytes, fall through */
		flex |= TXGBE_RDB_FDIR_FLEX_CFG_MSK <<
			(TXGBE_RDB_FDIR_FLEX_CFG_VM_SHIFT * j);
	case 0xFFFF:
		break;
	default:
		DEBUGOUT(" Error on flexible byte mask\n");
		return TXGBE_ERR_CONFIG;
	}
	wr32(hw, TXGBE_RDB_FDIR_FLEX_CFG(i), flex);

	/* store the TCP/UDP port masks, bit reversed from port
	 * layout */
	fdirtcpm = txgbe_get_fdirtcpm(input_mask);

	/* write both the same so that UDP and TCP use the same mask */
	wr32(hw, TXGBE_RDB_FDIR_TCP_MSK, ~fdirtcpm);
	wr32(hw, TXGBE_RDB_FDIR_UDP_MSK, ~fdirtcpm);
	wr32(hw, TXGBE_RDB_FDIR_SCTP_MSK, ~fdirtcpm);

	/* store source and destination IP masks (little-enian) */
	wr32(hw, TXGBE_RDB_FDIR_SA4_MSK,
			     TXGBE_NTOHL(~input_mask->formatted.src_ip[0]));
	wr32(hw, TXGBE_RDB_FDIR_DA4_MSK,
			     TXGBE_NTOHL(~input_mask->formatted.dst_ip[0]));
	return 0;
}

s32 txgbe_fdir_write_perfect_filter(struct txgbe_hw *hw,
					     union txgbe_atr_input *input,
					     u16 soft_id, u8 queue,
					     bool cloud_mode)
{
	u32 fdirport, fdirvlan, fdirhash, fdircmd;
	s32 err;

	DEBUGFUNC("\n");
	if (!cloud_mode) {
		/* currently IPv6 is not supported, must be programmed with 0 */
		wr32(hw, TXGBE_RDB_FDIR_IP6(2),
				     TXGBE_NTOHL(input->formatted.src_ip[0]));
		wr32(hw, TXGBE_RDB_FDIR_IP6(1),
				     TXGBE_NTOHL(input->formatted.src_ip[1]));
		wr32(hw, TXGBE_RDB_FDIR_IP6(0),
				     TXGBE_NTOHL(input->formatted.src_ip[2]));

		/* record the source address (little-endian) */
		wr32(hw, TXGBE_RDB_FDIR_SA,
			TXGBE_NTOHL(input->formatted.src_ip[0]));

		/* record the first 32 bits of the destination address
		 * (little-endian) */
		wr32(hw, TXGBE_RDB_FDIR_DA,
			TXGBE_NTOHL(input->formatted.dst_ip[0]));

		/* record source and destination port (little-endian)*/
		fdirport = TXGBE_NTOHS(input->formatted.dst_port);
		fdirport <<= TXGBE_RDB_FDIR_PORT_DESTINATION_SHIFT;
		fdirport |= TXGBE_NTOHS(input->formatted.src_port);
		wr32(hw, TXGBE_RDB_FDIR_PORT, fdirport);
	}

	/* record packet type and flex_bytes(little-endian) */
	fdirvlan = TXGBE_NTOHS(input->formatted.flex_bytes);
	fdirvlan <<= TXGBE_RDB_FDIR_FLEX_FLEX_SHIFT;

	fdirvlan |= TXGBE_NTOHS(input->formatted.vlan_id);
	wr32(hw, TXGBE_RDB_FDIR_FLEX, fdirvlan);


	/* configure FDIRHASH register */
	fdirhash = input->formatted.bkt_hash |
		0x1 << TXGBE_RDB_FDIR_HASH_BUCKET_VALID_SHIFT;
	fdirhash |= soft_id << TXGBE_RDB_FDIR_HASH_SIG_SW_INDEX_SHIFT;
	wr32(hw, TXGBE_RDB_FDIR_HASH, fdirhash);

	/*
	 * flush all previous writes to make certain registers are
	 * programmed prior to issuing the command
	 */
	TXGBE_WRITE_FLUSH(hw);

	/* configure FDIRCMD register */
	fdircmd = TXGBE_RDB_FDIR_CMD_CMD_ADD_FLOW |
		  TXGBE_RDB_FDIR_CMD_FILTER_UPDATE |
		  TXGBE_RDB_FDIR_CMD_LAST | TXGBE_RDB_FDIR_CMD_QUEUE_EN;
	if (queue == TXGBE_RDB_FDIR_DROP_QUEUE)
		fdircmd |= TXGBE_RDB_FDIR_CMD_DROP;
	fdircmd |= input->formatted.flow_type <<
			TXGBE_RDB_FDIR_CMD_FLOW_TYPE_SHIFT;
	fdircmd |= (u32)queue << TXGBE_RDB_FDIR_CMD_RX_QUEUE_SHIFT;
	fdircmd |= (u32)input->formatted.vm_pool <<
			TXGBE_RDB_FDIR_CMD_VT_POOL_SHIFT;

	wr32(hw, TXGBE_RDB_FDIR_CMD, fdircmd);
	err = txgbe_fdir_check_cmd_complete(hw, &fdircmd);
	if (err) {
		DEBUGOUT("Flow Director command did not complete!\n");
		return err;
	}

	return 0;
}

s32 txgbe_fdir_erase_perfect_filter(struct txgbe_hw *hw,
					  union txgbe_atr_input *input,
					  u16 soft_id)
{
	u32 fdirhash;
	u32 fdircmd;
	s32 err;

	/* configure FDIRHASH register */
	fdirhash = input->formatted.bkt_hash;
	fdirhash |= soft_id << TXGBE_RDB_FDIR_HASH_SIG_SW_INDEX_SHIFT;
	wr32(hw, TXGBE_RDB_FDIR_HASH, fdirhash);

	/* flush hash to HW */
	TXGBE_WRITE_FLUSH(hw);

	/* Query if filter is present */
	wr32(hw, TXGBE_RDB_FDIR_CMD,
			TXGBE_RDB_FDIR_CMD_CMD_QUERY_REM_FILT);

	err = txgbe_fdir_check_cmd_complete(hw, &fdircmd);
	if (err) {
		DEBUGOUT("Flow Director command did not complete!\n");
		return err;
	}

	/* if filter exists in hardware then remove it */
	if (fdircmd & TXGBE_RDB_FDIR_CMD_FILTER_VALID) {
		wr32(hw, TXGBE_RDB_FDIR_HASH, fdirhash);
		TXGBE_WRITE_FLUSH(hw);
		wr32(hw, TXGBE_RDB_FDIR_CMD,
				TXGBE_RDB_FDIR_CMD_CMD_REMOVE_FLOW);
	}

	return 0;
}


/**
 *  txgbe_start_hw - Prepare hardware for Tx/Rx
 *  @hw: pointer to hardware structure
 *
 *  Starts the hardware using the generic start_hw function
 *  and the generation start_hw function.
 *  Then performs revision-specific operations, if any.
 **/
s32 txgbe_start_hw(struct txgbe_hw *hw)
{
	int ret_val = 0;
	u32 i;

	DEBUGFUNC("\n");

	/* Set the media type */
	hw->phy.media_type = TCALL(hw, mac.ops.get_media_type);

	/* PHY ops initialization must be done in reset_hw() */

	/* Clear the VLAN filter table */
	TCALL(hw, mac.ops.clear_vfta);

	/* Clear statistics registers */
	TCALL(hw, mac.ops.clear_hw_cntrs);

	TXGBE_WRITE_FLUSH(hw);
#ifndef SIMULATION_DEBUG
	/* Setup flow control */
	ret_val = TCALL(hw, mac.ops.setup_fc);
#endif
	/* Clear the rate limiters */
	for (i = 0; i < hw->mac.max_tx_queues; i++) {
		wr32(hw, TXGBE_TDM_RP_IDX, i);
		wr32(hw, TXGBE_TDM_RP_RATE, 0);
	}
	TXGBE_WRITE_FLUSH(hw);

	/* Clear adapter stopped flag */
	hw->adapter_stopped = false;

	/* We need to run link autotry after the driver loads */
	hw->mac.autotry_restart = true;

	return ret_val;
}

/**
 *  txgbe_identify_phy - Get physical layer module
 *  @hw: pointer to hardware structure
 *
 *  Determines the physical layer module found on the current adapter.
 *  If PHY already detected, maintains current PHY type in hw struct,
 *  otherwise executes the PHY detection routine.
 **/
s32 txgbe_identify_phy(struct txgbe_hw *hw)
{
	/* Detect PHY if not unknown - returns success if already detected. */
	s32 status = TXGBE_ERR_PHY_ADDR_INVALID;
	enum txgbe_media_type media_type;

	DEBUGFUNC("\n");

	if (!hw->phy.phy_semaphore_mask) {
		hw->phy.phy_semaphore_mask = TXGBE_MNG_SWFW_SYNC_SW_PHY;
	}

	media_type = TCALL(hw, mac.ops.get_media_type);
	if (media_type == txgbe_media_type_copper) {
		status = txgbe_init_external_phy(hw);
		if (status != 0) {
			return status;
		}
		txgbe_get_phy_id(hw);
		hw->phy.type = txgbe_get_phy_type_from_id(hw);
		status = 0;
	} else if (media_type == txgbe_media_type_fiber) {
		status = txgbe_identify_module(hw);
	} else {
		hw->phy.type = txgbe_phy_none;
		status = 0;
	}

	/* Return error if SFP module has been detected but is not supported */
	if (hw->phy.type == txgbe_phy_sfp_unsupported)
		return TXGBE_ERR_SFP_NOT_SUPPORTED;

	return status;
}


/**
 *  txgbe_enable_rx_dma - Enable the Rx DMA unit on sapphire
 *  @hw: pointer to hardware structure
 *  @regval: register value to write to RXCTRL
 *
 *  Enables the Rx DMA unit for sapphire
 **/
s32 txgbe_enable_rx_dma(struct txgbe_hw *hw, u32 regval)
{

	DEBUGFUNC("\n");

	/*
	 * Workaround for sapphire silicon errata when enabling the Rx datapath.
	 * If traffic is incoming before we enable the Rx unit, it could hang
	 * the Rx DMA unit.  Therefore, make sure the security engine is
	 * completely disabled prior to enabling the Rx unit.
	 */

	TCALL(hw, mac.ops.disable_sec_rx_path);

	if (regval & TXGBE_RDB_PB_CTL_RXEN)
		TCALL(hw, mac.ops.enable_rx);
	else
		TCALL(hw, mac.ops.disable_rx);

	TCALL(hw, mac.ops.enable_sec_rx_path);

	return 0;
}

/**
 *  txgbe_init_flash_params - Initialize flash params
 *  @hw: pointer to hardware structure
 *
 *  Initializes the EEPROM parameters txgbe_eeprom_info within the
 *  txgbe_hw struct in order to set up EEPROM access.
 **/
s32 txgbe_init_flash_params(struct txgbe_hw *hw)
{
	struct txgbe_flash_info *flash = &hw->flash;
	u32 eec;

	DEBUGFUNC("\n");

	eec = 0x1000000;
	flash->semaphore_delay = 10;
	flash->dword_size = (eec >> 2);
	flash->address_bits = 24;
	DEBUGOUT3("FLASH params: size = %d, address bits: %d\n",
		  flash->dword_size,
		  flash->address_bits);

	return 0;
}

/**
 *  txgbe_read_flash_buffer - Read FLASH dword(s) using
 *  fastest available method
 *
 *  @hw: pointer to hardware structure
 *  @offset: offset of  dword in EEPROM to read
 *  @dwords: number of dwords
 *  @data: dword(s) read from the EEPROM
 *
 *  Retrieves 32 bit dword(s) read from EEPROM
 **/
s32 txgbe_read_flash_buffer(struct txgbe_hw *hw, u32 offset,
					  u32 dwords, u32 *data)
{
	s32 status = 0;
	u32 i;

	DEBUGFUNC("\n");

	TCALL(hw, eeprom.ops.init_params);

	if (!dwords || offset + dwords >= hw->flash.dword_size) {
		status = TXGBE_ERR_INVALID_ARGUMENT;
		ERROR_REPORT1(TXGBE_ERROR_ARGUMENT, "Invalid FLASH arguments");
		return status;
	}

	for (i = 0; i < dwords; i++) {
		wr32(hw, TXGBE_SPI_DATA, data[i]);
		wr32(hw, TXGBE_SPI_CMD,
			TXGBE_SPI_CMD_ADDR(offset + i) |
			TXGBE_SPI_CMD_CMD(0x0));

		status = po32m(hw, TXGBE_SPI_STATUS,
			TXGBE_SPI_STATUS_OPDONE, TXGBE_SPI_STATUS_OPDONE,
			TXGBE_SPI_TIMEOUT, 0);
		if (status) {
			DEBUGOUT("FLASH read timed out\n");
			break;
		}
	}

	return status;
}

/**
 *  txgbe_write_flash_buffer - Write FLASH dword(s) using
 *  fastest available method
 *
 *  @hw: pointer to hardware structure
 *  @offset: offset of  dword in EEPROM to write
 *  @dwords: number of dwords
 *  @data: dword(s) write from to EEPROM
 *
 **/
s32 txgbe_write_flash_buffer(struct txgbe_hw *hw, u32 offset,
					  u32 dwords, u32 *data)
{
	s32 status = 0;
	u32 i;

	DEBUGFUNC("\n");

	TCALL(hw, eeprom.ops.init_params);

	if (!dwords || offset + dwords >= hw->flash.dword_size) {
		status = TXGBE_ERR_INVALID_ARGUMENT;
		ERROR_REPORT1(TXGBE_ERROR_ARGUMENT, "Invalid FLASH arguments");
		return status;
	}

	for (i = 0; i < dwords; i++) {
		wr32(hw, TXGBE_SPI_CMD,
			TXGBE_SPI_CMD_ADDR(offset + i) |
			TXGBE_SPI_CMD_CMD(0x1));

		status = po32m(hw, TXGBE_SPI_STATUS,
			TXGBE_SPI_STATUS_OPDONE, TXGBE_SPI_STATUS_OPDONE,
			TXGBE_SPI_TIMEOUT, 0);
		if (status != 0) {
			DEBUGOUT("FLASH write timed out\n");
			break;
		}
		data[i] = rd32(hw, TXGBE_SPI_DATA);
	}

	return status;
}

/**
 *  txgbe_init_eeprom_params - Initialize EEPROM params
 *  @hw: pointer to hardware structure
 *
 *  Initializes the EEPROM parameters txgbe_eeprom_info within the
 *  txgbe_hw struct in order to set up EEPROM access.
 **/
s32 txgbe_init_eeprom_params(struct txgbe_hw *hw)
{
	struct txgbe_eeprom_info *eeprom = &hw->eeprom;
	u16 eeprom_size;
	s32 status = 0;
	u16 data;

	DEBUGFUNC("\n");

	if (eeprom->type == txgbe_eeprom_uninitialized) {
		eeprom->semaphore_delay = 10;
		eeprom->type = txgbe_eeprom_none;

		if (!(rd32(hw, TXGBE_SPI_STATUS) &
			TXGBE_SPI_STATUS_FLASH_BYPASS)) {
			eeprom->type = txgbe_flash;

			eeprom_size = 4096;
			eeprom->word_size = eeprom_size >> 1;

			DEBUGOUT2("Eeprom params: type = %d, size = %d\n",
				  eeprom->type, eeprom->word_size);
		}
	}

	status = TCALL(hw, eeprom.ops.read, TXGBE_SW_REGION_PTR,
			&data);
	if (status) {
		DEBUGOUT("NVM Read Error\n");
		return status;
	}
	eeprom->sw_region_offset = data >> 1;

	return status;
}

/**
 *  txgbe_read_ee_hostif - Read EEPROM word using a host interface cmd
 *  assuming that the semaphore is already obtained.
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to read
 *  @data: word read from the EEPROM
 *
 *  Reads a 16 bit word from the EEPROM using the hostif.
 **/
s32 txgbe_read_ee_hostif_data(struct txgbe_hw *hw, u16 offset,
				   u16 *data)
{
	s32 status;
	struct txgbe_hic_read_shadow_ram buffer;

	DEBUGFUNC("\n");
	buffer.hdr.req.cmd = FW_READ_SHADOW_RAM_CMD;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = FW_READ_SHADOW_RAM_LEN;
	buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;

	/* convert offset from words to bytes */
	buffer.address = TXGBE_CPU_TO_BE32(offset * 2);
	/* one word */
	buffer.length = TXGBE_CPU_TO_BE16(sizeof(u16));

	status = txgbe_host_interface_command(hw, (u32 *)&buffer,
					      sizeof(buffer),
					      TXGBE_HI_COMMAND_TIMEOUT, false);

	if (status)
		return status;
	if (txgbe_check_mng_access(hw))
		*data = (u16)rd32a(hw, TXGBE_MNG_MBOX,
							FW_NVM_DATA_OFFSET);
	else {
		status = TXGBE_ERR_MNG_ACCESS_FAILED;
		return status;
	}


	return 0;
}

/**
 *  txgbe_read_ee_hostif - Read EEPROM word using a host interface cmd
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to read
 *  @data: word read from the EEPROM
 *
 *  Reads a 16 bit word from the EEPROM using the hostif.
 **/
s32 txgbe_read_ee_hostif(struct txgbe_hw *hw, u16 offset,
			      u16 *data)
{
	s32 status = 0;

	DEBUGFUNC("\n");

	if (TCALL(hw, mac.ops.acquire_swfw_sync,
		TXGBE_MNG_SWFW_SYNC_SW_FLASH) == 0) {
		status = txgbe_read_ee_hostif_data(hw, offset, data);
		TCALL(hw, mac.ops.release_swfw_sync,
			TXGBE_MNG_SWFW_SYNC_SW_FLASH);
	} else {
		status = TXGBE_ERR_SWFW_SYNC;
	}

	return status;
}

/**
 *  txgbe_read_ee_hostif_buffer- Read EEPROM word(s) using hostif
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to read
 *  @words: number of words
 *  @data: word(s) read from the EEPROM
 *
 *  Reads a 16 bit word(s) from the EEPROM using the hostif.
 **/
s32 txgbe_read_ee_hostif_buffer(struct txgbe_hw *hw,
				     u16 offset, u16 words, u16 *data)
{
	struct txgbe_hic_read_shadow_ram buffer;
	u32 current_word = 0;
	u16 words_to_read;
	s32 status;
	u32 i;
	u32 value = 0;

	DEBUGFUNC("\n");

	/* Take semaphore for the entire operation. */
	status = TCALL(hw, mac.ops.acquire_swfw_sync,
			TXGBE_MNG_SWFW_SYNC_SW_FLASH);
	if (status) {
		DEBUGOUT("EEPROM read buffer - semaphore failed\n");
		return status;
	}
	while (words) {
		if (words > FW_MAX_READ_BUFFER_SIZE / 2)
			words_to_read = FW_MAX_READ_BUFFER_SIZE / 2;
		else
			words_to_read = words;

		buffer.hdr.req.cmd = FW_READ_SHADOW_RAM_CMD;
		buffer.hdr.req.buf_lenh = 0;
		buffer.hdr.req.buf_lenl = FW_READ_SHADOW_RAM_LEN;
		buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;

		/* convert offset from words to bytes */
		buffer.address = TXGBE_CPU_TO_BE32((offset + current_word) * 2);
		buffer.length = TXGBE_CPU_TO_BE16(words_to_read * 2);

		status = txgbe_host_interface_command(hw, (u32 *)&buffer,
						      sizeof(buffer),
						      TXGBE_HI_COMMAND_TIMEOUT,
						      false);

		if (status) {
			DEBUGOUT("Host interface command failed\n");
			goto out;
		}

		for (i = 0; i < words_to_read; i++) {
			u32 reg = TXGBE_MNG_MBOX + (FW_NVM_DATA_OFFSET << 2) +
				  2 * i;
			if (txgbe_check_mng_access(hw))
				value = rd32(hw, reg);
			else {
				status = TXGBE_ERR_MNG_ACCESS_FAILED;
				return status;
			}
			data[current_word] = (u16)(value & 0xffff);
			current_word++;
			i++;
			if (i < words_to_read) {
				value >>= 16;
				data[current_word] = (u16)(value & 0xffff);
				current_word++;
			}
		}
		words -= words_to_read;
	}

out:
	TCALL(hw, mac.ops.release_swfw_sync,
		TXGBE_MNG_SWFW_SYNC_SW_FLASH);
	return status;
}

/**
 *  txgbe_write_ee_hostif - Write EEPROM word using hostif
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to write
 *  @data: word write to the EEPROM
 *
 *  Write a 16 bit word to the EEPROM using the hostif.
 **/
s32 txgbe_write_ee_hostif_data(struct txgbe_hw *hw, u16 offset,
				    u16 data)
{
	s32 status;
	struct txgbe_hic_write_shadow_ram buffer;

	DEBUGFUNC("\n");

	buffer.hdr.req.cmd = FW_WRITE_SHADOW_RAM_CMD;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = FW_WRITE_SHADOW_RAM_LEN;
	buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;

	/* one word */
	buffer.length = TXGBE_CPU_TO_BE16(sizeof(u16));
	buffer.data = data;
	buffer.address = TXGBE_CPU_TO_BE32(offset * 2);

	status = txgbe_host_interface_command(hw, (u32 *)&buffer,
					      sizeof(buffer),
					      TXGBE_HI_COMMAND_TIMEOUT, false);

	return status;
}

s32 txgbe_close_notify(struct txgbe_hw *hw)
{
	int tmp;
	s32 status;
	struct txgbe_hic_write_shadow_ram buffer;

	DEBUGFUNC("\n");

	buffer.hdr.req.cmd = FW_DW_CLOSE_NOTIFY;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;

	/* one word */
	buffer.length = 0;
	buffer.address = 0;

	status = txgbe_host_interface_command(hw, (u32 *)&buffer,
					      sizeof(buffer),
					      TXGBE_HI_COMMAND_TIMEOUT, false);
	if (status)
		return status;

	if (txgbe_check_mng_access(hw)){
		tmp = (u32)rd32(hw, TXGBE_MNG_SW_SM);
		if(tmp == TXGBE_CHECKSUM_CAP_ST_PASS)
		{
			status = 0;
		} else {			
			status = TXGBE_ERR_EEPROM_CHECKSUM;
		}
	}else {
			status = TXGBE_ERR_MNG_ACCESS_FAILED;
			return status;
	}

	return status;
}

s32 txgbe_open_notify(struct txgbe_hw *hw)
{
	int tmp;
	s32 status;
	struct txgbe_hic_write_shadow_ram buffer;

	DEBUGFUNC("\n");

	buffer.hdr.req.cmd = FW_DW_OPEN_NOTIFY;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;

	/* one word */
	buffer.length = 0;
	buffer.address = 0;

	status = txgbe_host_interface_command(hw, (u32 *)&buffer,
					      sizeof(buffer),
					      TXGBE_HI_COMMAND_TIMEOUT, false);
		if (status)
		return status;

	if (txgbe_check_mng_access(hw)){
		tmp = (u32)rd32(hw, TXGBE_MNG_SW_SM);
		if(tmp == TXGBE_CHECKSUM_CAP_ST_PASS)
		{
			status = 0;
		} else {			
			status = TXGBE_ERR_EEPROM_CHECKSUM;
		}
	}else {
			status = TXGBE_ERR_MNG_ACCESS_FAILED;
			return status;
	}

	return status;
}

/**
 *  txgbe_write_ee_hostif - Write EEPROM word using hostif
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to write
 *  @data: word write to the EEPROM
 *
 *  Write a 16 bit word to the EEPROM using the hostif.
 **/
s32 txgbe_write_ee_hostif(struct txgbe_hw *hw, u16 offset,
			       u16 data)
{
	s32 status = 0;

	DEBUGFUNC("\n");

	if (TCALL(hw, mac.ops.acquire_swfw_sync,
		TXGBE_MNG_SWFW_SYNC_SW_FLASH) == 0) {
		status = txgbe_write_ee_hostif_data(hw, offset, data);
		TCALL(hw, mac.ops.release_swfw_sync,
			TXGBE_MNG_SWFW_SYNC_SW_FLASH);
	} else {
		DEBUGOUT("write ee hostif failed to get semaphore");
		status = TXGBE_ERR_SWFW_SYNC;
	}

	return status;
}

/**
 *  txgbe_write_ee_hostif_buffer - Write EEPROM word(s) using hostif
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to write
 *  @words: number of words
 *  @data: word(s) write to the EEPROM
 *
 *  Write a 16 bit word(s) to the EEPROM using the hostif.
 **/
s32 txgbe_write_ee_hostif_buffer(struct txgbe_hw *hw,
				      u16 offset, u16 words, u16 *data)
{
	s32 status = 0;
	u16 i = 0;

	DEBUGFUNC("\n");

	/* Take semaphore for the entire operation. */
	status = TCALL(hw, mac.ops.acquire_swfw_sync,
		       TXGBE_MNG_SWFW_SYNC_SW_FLASH);
	if (status != 0) {
		DEBUGOUT("EEPROM write buffer - semaphore failed\n");
		goto out;
	}

	for (i = 0; i < words; i++) {
		status = txgbe_write_ee_hostif_data(hw, offset + i,
							 data[i]);

		if (status != 0) {
			DEBUGOUT("Eeprom buffered write failed\n");
			break;
		}
	}

	TCALL(hw, mac.ops.release_swfw_sync, TXGBE_MNG_SWFW_SYNC_SW_FLASH);
out:

	return status;
}



/**
 *  txgbe_calc_eeprom_checksum - Calculates and returns the checksum
 *  @hw: pointer to hardware structure
 *
 *  Returns a negative error code on error, or the 16-bit checksum
 **/
s32 txgbe_calc_eeprom_checksum(struct txgbe_hw *hw)
{
	u16 *buffer = NULL;
	u32 buffer_size = 0;

	u16 *eeprom_ptrs = NULL;
	u16 *local_buffer;
	s32 status;
	u16 checksum = 0;
	u16 i;

	DEBUGFUNC("\n");

	TCALL(hw, eeprom.ops.init_params);

	if (!buffer) {
		eeprom_ptrs = (u16 *)vmalloc(TXGBE_EEPROM_LAST_WORD *
				sizeof(u16));
		if (!eeprom_ptrs)
			return TXGBE_ERR_NO_SPACE;
		/* Read pointer area */
		status = txgbe_read_ee_hostif_buffer(hw, 0,
						     TXGBE_EEPROM_LAST_WORD,
						     eeprom_ptrs);
		if (status) {
			DEBUGOUT("Failed to read EEPROM image\n");
			return status;
		}
		local_buffer = eeprom_ptrs;
	} else {
		if (buffer_size < TXGBE_EEPROM_LAST_WORD)
			return TXGBE_ERR_PARAM;
		local_buffer = buffer;
	}

	for (i = 0; i < TXGBE_EEPROM_LAST_WORD; i++)
		if (i != hw->eeprom.sw_region_offset + TXGBE_EEPROM_CHECKSUM)
			checksum += local_buffer[i];

	checksum = (u16)TXGBE_EEPROM_SUM - checksum;
	if (eeprom_ptrs)
		vfree(eeprom_ptrs);

	return (s32)checksum;
}

/**
 * txgbe_update_eeprom_checksum - Updates the EEPROM checksum and flash
 * @hw: pointer to hardware structure
 *
 * After writing EEPROM to shadow RAM using EEWR register, software calculates
 * checksum and updates the EEPROM and instructs the hardware to update
 * the flash.
 **/
s32 txgbe_update_eeprom_checksum(struct txgbe_hw *hw)
{
	s32 status;
	u16 checksum = 0;

	DEBUGFUNC("\n");

	/* Read the first word from the EEPROM. If this times out or fails, do
	 * not continue or we could be in for a very long wait while every
	 * EEPROM read fails
	 */
	status = txgbe_read_ee_hostif(hw, 0, &checksum);
	if (status) {
		DEBUGOUT("EEPROM read failed\n");
		return status;
	}

	status = txgbe_calc_eeprom_checksum(hw);
	if (status < 0)
		return status;

	checksum = (u16)(status & 0xffff);

	status = txgbe_write_ee_hostif(hw, TXGBE_EEPROM_CHECKSUM,
					    checksum);
	if (status)
		return status;

	return status;
}

/**
 *  txgbe_validate_eeprom_checksum - Validate EEPROM checksum
 *  @hw: pointer to hardware structure
 *  @checksum_val: calculated checksum
 *
 *  Performs checksum calculation and validates the EEPROM checksum.  If the
 *  caller does not need checksum_val, the value can be NULL.
 **/
s32 txgbe_validate_eeprom_checksum(struct txgbe_hw *hw,
					    u16 *checksum_val)
{
	s32 status;
	u16 checksum;
	u16 read_checksum = 0;

	DEBUGFUNC("\n");

	/* Read the first word from the EEPROM. If this times out or fails, do
	 * not continue or we could be in for a very long wait while every
	 * EEPROM read fails
	 */
	status = TCALL(hw, eeprom.ops.read, 0, &checksum);
	if (status) {
		DEBUGOUT("EEPROM read failed\n");
		return status;
	}

	status = TCALL(hw, eeprom.ops.calc_checksum);
	if (status < 0)
		return status;

	checksum = (u16)(status & 0xffff);

	status = txgbe_read_ee_hostif(hw, hw->eeprom.sw_region_offset +
					TXGBE_EEPROM_CHECKSUM,
					&read_checksum);
	if (status)
		return status;

	/* Verify read checksum from EEPROM is the same as
	 * calculated checksum
	 */
	if (read_checksum != checksum) {
		status = TXGBE_ERR_EEPROM_CHECKSUM;
		ERROR_REPORT1(TXGBE_ERROR_INVALID_STATE,
			     "Invalid EEPROM checksum\n");
	}

	/* If the user cares, return the calculated checksum */
	if (checksum_val)
		*checksum_val = checksum;

	return status;
}

/**
 *  txgbe_update_flash - Instruct HW to copy EEPROM to Flash device
 *  @hw: pointer to hardware structure
 *
 *  Issue a shadow RAM dump to FW to copy EEPROM from shadow RAM to the flash.
 **/
s32 txgbe_update_flash(struct txgbe_hw *hw)
{
	s32 status = 0;
	union txgbe_hic_hdr2 buffer;

	DEBUGFUNC("\n");

	buffer.req.cmd = FW_SHADOW_RAM_DUMP_CMD;
	buffer.req.buf_lenh = 0;
	buffer.req.buf_lenl = FW_SHADOW_RAM_DUMP_LEN;
	buffer.req.checksum = FW_DEFAULT_CHECKSUM;

	status = txgbe_host_interface_command(hw, (u32 *)&buffer,
					      sizeof(buffer),
					      TXGBE_HI_COMMAND_TIMEOUT, false);

	return status;
}


/**
 *  txgbe_check_mac_link - Determine link and speed status
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @link_up: true when link is up
 *  @link_up_wait_to_complete: bool used to wait for link up or not
 *
 *  Reads the links register to determine if link is up and the current speed
 **/
s32 txgbe_check_mac_link(struct txgbe_hw *hw, u32 *speed,
				bool *link_up, bool link_up_wait_to_complete)
{
	u32 links_reg = 0;
	u32 i;
	u16 value;

	DEBUGFUNC("\n");

	if (link_up_wait_to_complete) {
		for (i = 0; i < TXGBE_LINK_UP_TIME; i++) {
			if (TCALL(hw, mac.ops.get_media_type) == txgbe_media_type_copper &&
				 ((hw->subsystem_id & 0xF0) != TXGBE_ID_SFI_XAUI)) {
				/* read ext phy link status */
				txgbe_read_mdio(&hw->phy_dev, hw->phy.addr, 0x03, 0x8008, &value);
				if (value & 0x400) {
					*link_up = true;
				} else {
					*link_up = false;
				}
			} else {
				*link_up = true;
			}
			if (*link_up) {
				links_reg = rd32(hw,
							TXGBE_CFG_PORT_ST);
				if (links_reg & TXGBE_CFG_PORT_ST_LINK_UP) {
					*link_up = true;
					break;
				} else {
					*link_up = false;
				}
			}
			msleep(100);
		}
	} else {

#ifndef SIMULATION_DEBUG
				if (TCALL(hw, mac.ops.get_media_type) == txgbe_media_type_copper  &&
				 ((hw->subsystem_id & 0xF0) != TXGBE_ID_SFI_XAUI)) {
					/* read ext phy link status */
					txgbe_read_mdio(&hw->phy_dev, hw->phy.addr, 0x03, 0x8008, &value);
					if (value & 0x400) {
						*link_up = true;
					} else {
						*link_up = false;
					}
				} else {
					*link_up = true;
				}
				if (*link_up) {
#endif
				links_reg = rd32(hw, TXGBE_CFG_PORT_ST);
				if (links_reg & TXGBE_CFG_PORT_ST_LINK_UP) {
					*link_up = true;
				} else {
					*link_up = false;
				}
#ifndef SIMULATION_DEBUG
			}
#endif
	}

	if (*link_up) {
#ifndef SIMULATION_DEBUG
		if (TCALL(hw, mac.ops.get_media_type) == txgbe_media_type_copper  &&
				 ((hw->subsystem_id & 0xF0) != TXGBE_ID_SFI_XAUI)) {
			if ((value & 0xc000) == 0xc000) {
				*speed = TXGBE_LINK_SPEED_10GB_FULL;
			} else if ((value & 0xc000) == 0x8000) {
				*speed = TXGBE_LINK_SPEED_1GB_FULL;
			} else if ((value & 0xc000) == 0x4000) {
				*speed = TXGBE_LINK_SPEED_100_FULL;
			} else if ((value & 0xc000) == 0x0000) {
				*speed = TXGBE_LINK_SPEED_10_FULL;
			}
		} else {
#endif
			if ((links_reg & TXGBE_CFG_PORT_ST_LINK_10G) ==
					TXGBE_CFG_PORT_ST_LINK_10G) {
				*speed = TXGBE_LINK_SPEED_10GB_FULL;
			} else if ((links_reg & TXGBE_CFG_PORT_ST_LINK_1G) ==
					TXGBE_CFG_PORT_ST_LINK_1G){
				*speed = TXGBE_LINK_SPEED_1GB_FULL;
			} else if ((links_reg & TXGBE_CFG_PORT_ST_LINK_100M) ==
					TXGBE_CFG_PORT_ST_LINK_100M){
				*speed = TXGBE_LINK_SPEED_100_FULL;
			} else
				*speed = TXGBE_LINK_SPEED_10_FULL;
#ifndef SIMULATION_DEBUG
		}
#endif
	} else
		*speed = TXGBE_LINK_SPEED_UNKNOWN;

	return 0;
}

/**
 *  txgbe_setup_eee - Enable/disable EEE support
 *  @hw: pointer to the HW structure
 *  @enable_eee: boolean flag to enable EEE
 *
 *  Enable/disable EEE based on enable_eee flag.
 *  Auto-negotiation must be started after BASE-T EEE bits in PHY register 7.3C
 *  are modified.
 *
 **/
s32 txgbe_setup_eee(struct txgbe_hw *hw, bool enable_eee)
{
	/* fix eee */
	UNREFERENCED_PARAMETER(hw);
	UNREFERENCED_PARAMETER(enable_eee);
	DEBUGFUNC("\n");

	return 0;
}


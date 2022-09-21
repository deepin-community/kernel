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


#include "ngbe_type.h"
#include "ngbe_hw.h"
#include "ngbe_phy.h"
#include "ngbe.h"

#ifndef SIMULATION_DEBUG
#define NGBE_SP_MAX_TX_QUEUES  8
#define NGBE_SP_MAX_RX_QUEUES  8
#else
#define NGBE_SP_MAX_TX_QUEUES  8
#define NGBE_SP_MAX_RX_QUEUES  8
#endif
#define NGBE_SP_RAR_ENTRIES    32
#define NGBE_SP_MC_TBL_SIZE    128
#define NGBE_SP_VFT_TBL_SIZE   128
#define NGBE_SP_RX_PB_SIZE     42

STATIC s32 ngbe_get_eeprom_semaphore(struct ngbe_hw *hw);
STATIC void ngbe_release_eeprom_semaphore(struct ngbe_hw *hw);
STATIC s32 ngbe_mta_vector(struct ngbe_hw *hw, u8 *mc_addr);

STATIC s32 ngbe_setup_copper_link(struct ngbe_hw *hw,
					 u32 speed,
					 bool need_restart_AN);
s32 ngbe_check_mac_link(struct ngbe_hw *hw, u32 *speed,
				  bool *link_up, bool link_up_wait_to_complete);
s32 ngbe_check_mac_link_mdi(struct ngbe_hw *hw,
							u32 *speed,
							bool *link_up,
							bool link_up_wait_to_complete);
s32 ngbe_check_mac_link_yt8521s(struct ngbe_hw *hw,
							u32 *speed,
							bool *link_up,
							bool link_up_wait_to_complete);


u32 ngbe_rd32_epcs(struct ngbe_hw *hw, u32 addr)
{
	unsigned int portRegOffset;
	u32 data;
	/* Set the LAN port indicator to portRegOffset[1] */
	/* 1st, write the regOffset to IDA_ADDR register */
	portRegOffset = NGBE_XPCS_IDA_ADDR;
	wr32(hw, portRegOffset, addr);

	/* 2nd, read the data from IDA_DATA register */
	portRegOffset = NGBE_XPCS_IDA_DATA;
	data = rd32(hw, portRegOffset);

	return data;
}


void ngbe_wr32_ephy(struct ngbe_hw *hw, u32 addr, u32 data)
{
	unsigned int portRegOffset;

	/* Set the LAN port indicator to portRegOffset[1] */
	/* 1st, write the regOffset to IDA_ADDR register */
	portRegOffset = NGBE_ETHPHY_IDA_ADDR;
	wr32(hw, portRegOffset, addr);

	/* 2nd, read the data from IDA_DATA register */
	portRegOffset = NGBE_ETHPHY_IDA_DATA;
	wr32(hw, portRegOffset, data);
}

void ngbe_wr32_epcs(struct ngbe_hw *hw, u32 addr, u32 data)
{
	unsigned int portRegOffset;

	/* Set the LAN port indicator to portRegOffset[1] */
	/* 1st, write the regOffset to IDA_ADDR register */
	portRegOffset = NGBE_XPCS_IDA_ADDR;
	wr32(hw, portRegOffset, addr);

	/* 2nd, read the data from IDA_DATA register */
	portRegOffset = NGBE_XPCS_IDA_DATA;
	wr32(hw, portRegOffset, data);
}



/**
 *  ngbe_get_pcie_msix_count - Gets MSI-X vector count
 *  @hw: pointer to hardware structure
 *
 *  Read PCIe configuration space, and get the MSI-X vector count from
 *  the capabilities table.
 **/
u16 ngbe_get_pcie_msix_count(struct ngbe_hw *hw)
{
	u16 msix_count = 1;
	u16 max_msix_count;
	u32 pos;

	DEBUGFUNC("\n");

	/* ??? max_msix_count for emerald */
	max_msix_count = NGBE_MAX_MSIX_VECTORS_EMERALD;
	pos = pci_find_capability(((struct ngbe_adapter *)hw->back)->pdev,
								PCI_CAP_ID_MSIX);
	if (!pos)
		return msix_count;
	pci_read_config_word(((struct ngbe_adapter *)hw->back)->pdev,
			pos + PCI_MSIX_FLAGS, &msix_count);

	if (NGBE_REMOVED(hw->hw_addr))
		msix_count = 0;
	msix_count &= NGBE_PCIE_MSIX_TBL_SZ_MASK;

	/* MSI-X count is zero-based in HW */
	msix_count++;

	if (msix_count > max_msix_count)
		msix_count = max_msix_count;

	return msix_count;
}

/**
 *  ngbe_init_hw - Generic hardware initialization
 *  @hw: pointer to hardware structure
 *
 *  Initialize the hardware by resetting the hardware, filling the bus info
 *  structure and media type, clears all on chip counters, initializes receive
 *  address registers, multicast table, VLAN filter table, calls routine to set
 *  up link and flow control settings, and leaves transmit and receive units
 *  disabled and uninitialized
 **/
s32 ngbe_init_hw(struct ngbe_hw *hw)
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
 *  ngbe_clear_hw_cntrs - Generic clear hardware counters
 *  @hw: pointer to hardware structure
 *
 *  Clears all hardware statistics counters by reading them from the hardware
 *  Statistics counters are clear on read.
 **/
s32 ngbe_clear_hw_cntrs(struct ngbe_hw *hw)
{
	u16 i = 0;

	DEBUGFUNC("\n");

	rd32(hw, NGBE_RX_CRC_ERROR_FRAMES_LOW);
	rd32(hw, NGBE_RX_LEN_ERROR_FRAMES_LOW);
	rd32(hw, NGBE_RDB_LXONTXC);
	rd32(hw, NGBE_RDB_LXOFFTXC);
	/* ??? 1e0c not found */
	/* rd32(hw, NGBE_MAC_LXONRXC); */
	rd32(hw, NGBE_MAC_LXOFFRXC);

	for (i = 0; i < 8; i++) {
	/* ??? move 16? */
		wr32m(hw, NGBE_MMC_CONTROL, NGBE_MMC_CONTROL_UP, i<<16);
		rd32(hw, NGBE_MAC_PXOFFRXC);
	}

	for (i = 0; i < 8; i++) {
		wr32(hw, NGBE_PX_MPRC(i), 0);
	}
	/* BPRC */

	rd32(hw, NGBE_PX_GPRC);
	rd32(hw, NGBE_PX_GPTC);
	rd32(hw, NGBE_PX_GORC_MSB);
	rd32(hw, NGBE_PX_GOTC_MSB);

	rd32(hw, NGBE_RX_BC_FRAMES_GOOD_LOW);
	rd32(hw, NGBE_RX_UNDERSIZE_FRAMES_GOOD);
	rd32(hw, NGBE_RX_OVERSIZE_FRAMES_GOOD);
	rd32(hw, NGBE_RX_FRAME_CNT_GOOD_BAD_LOW);
	rd32(hw, NGBE_TX_FRAME_CNT_GOOD_BAD_LOW);
	rd32(hw, NGBE_TX_MC_FRAMES_GOOD_LOW);
	rd32(hw, NGBE_TX_BC_FRAMES_GOOD_LOW);
	rd32(hw, NGBE_RDM_DRP_PKT);
	return 0;
}



/**
 *  ngbe_setup_fc - Set up flow control
 *  @hw: pointer to hardware structure
 *
 *  Called at init time to set up flow control.
 **/
s32 ngbe_setup_fc(struct ngbe_hw *hw)
{
	s32 ret_val = 0;
	u16 pcap_backplane = 0;

	DEBUGFUNC("\n");

	/* Validate the requested mode */
	if (hw->fc.strict_ieee && hw->fc.requested_mode == ngbe_fc_rx_pause) {
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
						"ngbe_fc_rx_pause not valid in strict IEEE mode\n");
		ret_val = NGBE_ERR_INVALID_LINK_SETTINGS;
		goto out;
	}

	/*
	 * gig parts do not have a word in the EEPROM to determine the
	 * default flow control setting, so we explicitly set it to full.
	 */
	if (hw->fc.requested_mode == ngbe_fc_default)
		hw->fc.requested_mode = ngbe_fc_full;

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
	case ngbe_fc_none:
		/* Flow control completely disabled by software override. */
		break;
	case ngbe_fc_tx_pause:
		/*
		 * Tx Flow control is enabled, and Rx Flow control is
		 * disabled by software override.
		 */
		if (hw->phy.type != ngbe_phy_m88e1512_sfi &&
			hw->phy.type != ngbe_phy_yt8521s_sfi)
			pcap_backplane |= NGBE_SR_AN_MMD_ADV_REG1_PAUSE_ASM;
		else
			pcap_backplane |= 0x100;
		break;
	case ngbe_fc_rx_pause:
		/*
		 * Rx Flow control is enabled and Tx Flow control is
		 * disabled by software override. Since there really
		 * isn't a way to advertise that we are capable of RX
		 * Pause ONLY, we will advertise that we support both
		 * symmetric and asymmetric Rx PAUSE, as such we fall
		 * through to the fc_full statement.  Later, we will
		 * disable the adapter's ability to send PAUSE frames.
		 */
	case ngbe_fc_full:
		/* Flow control (both Rx and Tx) is enabled by SW override. */
		if (hw->phy.type != ngbe_phy_m88e1512_sfi &&
			hw->phy.type != ngbe_phy_yt8521s_sfi)
			pcap_backplane |= NGBE_SR_AN_MMD_ADV_REG1_PAUSE_SYM |
				NGBE_SR_AN_MMD_ADV_REG1_PAUSE_ASM;
		else
			pcap_backplane |= 0x80;
		break;
	default:
		ERROR_REPORT1(NGBE_ERROR_ARGUMENT,
			"Flow control param set incorrectly\n");
		ret_val = NGBE_ERR_CONFIG;
		goto out;
		break;
	}

	/*
	 * AUTOC restart handles negotiation of 1G on backplane
	 * and copper.
	 */
	if ((hw->phy.media_type == ngbe_media_type_copper) &&
		!((hw->subsystem_device_id & OEM_MASK) == RGMII_FPGA)) {
		ret_val = TCALL(hw, phy.ops.set_adv_pause, pcap_backplane);
	}

out:
	return ret_val;
}


/**
 *  ngbe_get_mac_addr - Generic get MAC address
 *  @hw: pointer to hardware structure
 *  @mac_addr: Adapter MAC address
 *
 *  Reads the adapter's MAC address from first Receive Address Register (RAR0)
 *  A reset of the adapter must be performed prior to calling this function
 *  in order for the MAC address to have been loaded from the EEPROM into RAR0
 **/
s32 ngbe_get_mac_addr(struct ngbe_hw *hw, u8 *mac_addr)
{
	u32 rar_high;
	u32 rar_low;
	u16 i;

	DEBUGFUNC("\n");

	wr32(hw, NGBE_PSR_MAC_SWC_IDX, 0);
	rar_high = rd32(hw, NGBE_PSR_MAC_SWC_AD_H);
	rar_low = rd32(hw, NGBE_PSR_MAC_SWC_AD_L);

	for (i = 0; i < 2; i++)
		mac_addr[i] = (u8)(rar_high >> (1 - i) * 8);

	for (i = 0; i < 4; i++)
		mac_addr[i + 2] = (u8)(rar_low >> (3 - i) * 8);

	return 0;
}

/**
 *  ngbe_set_pci_config_data - Generic store PCI bus info
 *  @hw: pointer to hardware structure
 *  @link_status: the link status returned by the PCI config space
 *
 *  Stores the PCI bus info (speed, width, type) within the ngbe_hw structure
 **/
void ngbe_set_pci_config_data(struct ngbe_hw *hw, u16 link_status)
{
	if (hw->bus.type == ngbe_bus_type_unknown)
		hw->bus.type = ngbe_bus_type_pci_express;

	switch (link_status & NGBE_PCI_LINK_WIDTH) {
	case NGBE_PCI_LINK_WIDTH_1:
		hw->bus.width = ngbe_bus_width_pcie_x1;
		break;
	case NGBE_PCI_LINK_WIDTH_2:
		hw->bus.width = ngbe_bus_width_pcie_x2;
		break;
	case NGBE_PCI_LINK_WIDTH_4:
		hw->bus.width = ngbe_bus_width_pcie_x4;
		break;
	case NGBE_PCI_LINK_WIDTH_8:
		hw->bus.width = ngbe_bus_width_pcie_x8;
		break;
	default:
		hw->bus.width = ngbe_bus_width_unknown;
		break;
	}

	switch (link_status & NGBE_PCI_LINK_SPEED) {
	case NGBE_PCI_LINK_SPEED_2500:
		hw->bus.speed = ngbe_bus_speed_2500;
		break;
	case NGBE_PCI_LINK_SPEED_5000:
		hw->bus.speed = ngbe_bus_speed_5000;
		break;
	case NGBE_PCI_LINK_SPEED_8000:
		hw->bus.speed = ngbe_bus_speed_8000;
		break;
	default:
		hw->bus.speed = ngbe_bus_speed_unknown;
		break;
	}
}

/**
 *  ngbe_get_bus_info - Generic set PCI bus info
 *  @hw: pointer to hardware structure
 *
 *  Gets the PCI bus info (speed, width, type) then calls helper function to
 *  store this data within the ngbe_hw structure.
 **/
s32 ngbe_get_bus_info(struct ngbe_hw *hw)
{
	u16 link_status;

	DEBUGFUNC("\n");

	/* Get the negotiated link width and speed from PCI config space */
	link_status = NGBE_READ_PCIE_WORD(hw, NGBE_PCI_LINK_STATUS);

	ngbe_set_pci_config_data(hw, link_status);

	return 0;
}

/**
 *  ngbe_set_lan_id_multi_port_pcie - Set LAN id for PCIe multiple port devices
 *  @hw: pointer to the HW structure
 *
 *  Determines the LAN function id by reading memory-mapped registers
 *  and swaps the port value if requested.
 **/
void ngbe_set_lan_id_multi_port_pcie(struct ngbe_hw *hw)
{
	struct ngbe_bus_info *bus = &hw->bus;
	u32 reg = 0;

	DEBUGFUNC("\n");

	reg = rd32(hw, NGBE_CFG_PORT_ST);
	bus->lan_id = NGBE_CFG_PORT_ST_LAN_ID(reg);
	bus->func = bus->lan_id;
}

/**
 *  ngbe_stop_adapter - Generic stop Tx/Rx units
 *  @hw: pointer to hardware structure
 *
 *  Sets the adapter_stopped flag within ngbe_hw struct. Clears interrupts,
 *  disables transmit and receive units. The adapter_stopped flag is used by
 *  the shared code and drivers to determine if the adapter is in a stopped
 *  state and should not touch the hardware.
 **/
s32 ngbe_stop_adapter(struct ngbe_hw *hw)
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
	ngbe_intr_disable(hw, NGBE_INTR_ALL);

	/* Clear any pending interrupts, flush previous writes */
	wr32(hw, NGBE_PX_MISC_IC, 0xffffffff);

	/* ??? 0bit RW->RO */
	wr32(hw, NGBE_BME_CTL, 0x3);


	/* Disable the transmit unit.  Each queue must be disabled. */
	for (i = 0; i < hw->mac.max_tx_queues; i++) {
		wr32m(hw, NGBE_PX_TR_CFG(i),
			NGBE_PX_TR_CFG_SWFLSH | NGBE_PX_TR_CFG_ENABLE,
			NGBE_PX_TR_CFG_SWFLSH);
	}

	/* Disable the receive unit by stopping each queue */
	for (i = 0; i < hw->mac.max_rx_queues; i++) {
		wr32m(hw, NGBE_PX_RR_CFG(i),
			NGBE_PX_RR_CFG_RR_EN, 0);
	}

	/* flush all queues disables */
	NGBE_WRITE_FLUSH(hw);
	msec_delay(2);

	/*
	 * Prevent the PCI-E bus from hanging by disabling PCI-E master
	 * access and verify no pending requests
	 */
	return ngbe_disable_pcie_master(hw);
}

/**
 *  ngbe_led_on - Turns on the software controllable LEDs.
 *  @hw: pointer to hardware structure
 *  @index: led number to turn on
 **/
s32 ngbe_led_on(struct ngbe_hw *hw, u32 index)
{
	u32 led_reg = rd32(hw, NGBE_CFG_LED_CTL);

	DEBUGFUNC("\n");

	/* ??? */
	/* To turn on the LED, set mode to ON. */
	led_reg |= index | (index << NGBE_CFG_LED_CTL_LINK_OD_SHIFT);
	wr32(hw, NGBE_CFG_LED_CTL, led_reg);
	NGBE_WRITE_FLUSH(hw);

	return 0;
}

/**
 *  ngbe_led_off - Turns off the software controllable LEDs.
 *  @hw: pointer to hardware structure
 *  @index: led number to turn off
 **/
s32 ngbe_led_off(struct ngbe_hw *hw, u32 index)
{
	u32 led_reg = rd32(hw, NGBE_CFG_LED_CTL);

	DEBUGFUNC("\n");

	/* To turn off the LED, set mode to OFF. */
	led_reg &= ~(index << NGBE_CFG_LED_CTL_LINK_OD_SHIFT);
	led_reg |= index;
	wr32(hw, NGBE_CFG_LED_CTL, led_reg);
	NGBE_WRITE_FLUSH(hw);
	return 0;
}

/**
 *  ngbe_get_eeprom_semaphore - Get hardware semaphore
 *  @hw: pointer to hardware structure
 *
 *  Sets the hardware semaphores so EEPROM access can occur for bit-bang method
 **/
STATIC s32 ngbe_get_eeprom_semaphore(struct ngbe_hw *hw)
{
	s32 status = NGBE_ERR_EEPROM;
	u32 timeout = 2000;
	u32 i;
	u32 swsm;

	/* Get SMBI software semaphore between device drivers first */
	for (i = 0; i < timeout; i++) {
		/*
		 * If the SMBI bit is 0 when we read it, then the bit will be
		 * set and we have the semaphore
		 */
		swsm = rd32(hw, NGBE_MIS_SWSM);
		if (!(swsm & NGBE_MIS_SWSM_SMBI)) {
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
		ngbe_release_eeprom_semaphore(hw);

		usec_delay(50);
		/*
		 * one last try
		 * If the SMBI bit is 0 when we read it, then the bit will be
		 * set and we have the semaphore
		 */
		swsm = rd32(hw, NGBE_MIS_SWSM);
		if (!(swsm & NGBE_MIS_SWSM_SMBI))
			status = 0;
	}

	/* Now get the semaphore between SW/FW through the SWESMBI bit */
	if (status == 0) {
		for (i = 0; i < timeout; i++) {
			if (ngbe_check_mng_access(hw)) {
			/* Set the SW EEPROM semaphore bit to request access */
				wr32m(hw, NGBE_MNG_SW_SM,
					NGBE_MNG_SW_SM_SM, NGBE_MNG_SW_SM_SM);

				/*
				 * If we set the bit successfully then we got
				 * semaphore.
				 */
				swsm = rd32(hw, NGBE_MNG_SW_SM);
				if (swsm & NGBE_MNG_SW_SM_SM)
					break;
			}
			usec_delay(50);
		}

		/*
		 * Release semaphores and return error if SW EEPROM semaphore
		 * was not granted because we don't have access to the EEPROM
		 */
		if (i >= timeout) {
			ERROR_REPORT1(NGBE_ERROR_POLLING,
			    "SWESMBI Software EEPROM semaphore not granted.\n");
			ngbe_release_eeprom_semaphore(hw);
			status = NGBE_ERR_EEPROM;
		}
	} else {
		ERROR_REPORT1(NGBE_ERROR_POLLING,
			     "Software semaphore SMBI between device drivers "
			     "not granted.\n");
	}

	return status;
}

/**
 *  ngbe_release_eeprom_semaphore - Release hardware semaphore
 *  @hw: pointer to hardware structure
 *
 *  This function clears hardware semaphore bits.
 **/
STATIC void ngbe_release_eeprom_semaphore(struct ngbe_hw *hw)
{
	if (ngbe_check_mng_access(hw)) {
		wr32m(hw, NGBE_MNG_SW_SM,
			NGBE_MNG_SW_SM_SM, 0);
		wr32m(hw, NGBE_MIS_SWSM,
			NGBE_MIS_SWSM_SMBI, 0);
		NGBE_WRITE_FLUSH(hw);
	}
}

/**
 *  ngbe_validate_mac_addr - Validate MAC address
 *  @mac_addr: pointer to MAC address.
 *
 *  Tests a MAC address to ensure it is a valid Individual Address
 **/
s32 ngbe_validate_mac_addr(u8 *mac_addr)
{
	s32 status = 0;

	DEBUGFUNC("\n");

	/* Make sure it is not a multicast address */
	if (NGBE_IS_MULTICAST(mac_addr)) {
		DEBUGOUT("MAC address is multicast\n");
		status = NGBE_ERR_INVALID_MAC_ADDR;
	/* Not a broadcast address */
	} else if (NGBE_IS_BROADCAST(mac_addr)) {
		DEBUGOUT("MAC address is broadcast\n");
		status = NGBE_ERR_INVALID_MAC_ADDR;
	/* Reject the zero address */
	} else if (mac_addr[0] == 0 && mac_addr[1] == 0 && mac_addr[2] == 0 &&
		   mac_addr[3] == 0 && mac_addr[4] == 0 && mac_addr[5] == 0) {
		DEBUGOUT("MAC address is all zeros\n");
		status = NGBE_ERR_INVALID_MAC_ADDR;
	}
	return status;
}

/**
 *  ngbe_set_rar - Set Rx address register
 *  @hw: pointer to hardware structure
 *  @index: Receive address register to write
 *  @addr: Address to put into receive address register
 *  @vmdq: VMDq "set" or "pool" index
 *  @enable_addr: set flag that address is active
 *
 *  Puts an ethernet address into a receive address register.
 **/
s32 ngbe_set_rar(struct ngbe_hw *hw, u32 index, u8 *addr, u64 pools,
			  u32 enable_addr)
{
	u32 rar_low, rar_high;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("\n");

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries) {
		ERROR_REPORT2(NGBE_ERROR_ARGUMENT,
			     "RAR index %d is out of range.\n", index);
		return NGBE_ERR_INVALID_ARGUMENT;
	}

	/* select the MAC address */
	wr32(hw, NGBE_PSR_MAC_SWC_IDX, index);

	/* setup VMDq pool mapping */
	wr32(hw, NGBE_PSR_MAC_SWC_VM, pools & 0xFFFFFFFF);

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
		rar_high |= NGBE_PSR_MAC_SWC_AD_H_AV;

	wr32(hw, NGBE_PSR_MAC_SWC_AD_L, rar_low);
	wr32m(hw, NGBE_PSR_MAC_SWC_AD_H,
		(NGBE_PSR_MAC_SWC_AD_H_AD(~0) |
		 NGBE_PSR_MAC_SWC_AD_H_ADTYPE(~0) |
		 NGBE_PSR_MAC_SWC_AD_H_AV),
		rar_high);

	return 0;
}

/**
 *  ngbe_clear_rar - Remove Rx address register
 *  @hw: pointer to hardware structure
 *  @index: Receive address register to write
 *
 *  Clears an ethernet address from a receive address register.
 **/
s32 ngbe_clear_rar(struct ngbe_hw *hw, u32 index)
{
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("\n");

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries) {
		ERROR_REPORT2(NGBE_ERROR_ARGUMENT,
			     "RAR index %d is out of range.\n", index);
		return NGBE_ERR_INVALID_ARGUMENT;
	}

	/*
	 * Some parts put the VMDq setting in the extra RAH bits,
	 * so save everything except the lower 16 bits that hold part
	 * of the address and the address valid bit.
	 */
	wr32(hw, NGBE_PSR_MAC_SWC_IDX, index);

	wr32(hw, NGBE_PSR_MAC_SWC_VM, 0);
	wr32(hw, NGBE_PSR_MAC_SWC_AD_L, 0);
	wr32m(hw, NGBE_PSR_MAC_SWC_AD_H,
		(NGBE_PSR_MAC_SWC_AD_H_AD(~0) |
		 NGBE_PSR_MAC_SWC_AD_H_ADTYPE(~0) |
		 NGBE_PSR_MAC_SWC_AD_H_AV),
		0);

	return 0;
}

/**
 *  ngbe_init_rx_addrs - Initializes receive address filters.
 *  @hw: pointer to hardware structure
 *
 *  Places the MAC address in receive address register 0 and clears the rest
 *  of the receive address registers. Clears the multicast table. Assumes
 *  the receiver is in reset when the routine is called.
 **/
s32 ngbe_init_rx_addrs(struct ngbe_hw *hw)
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
	if (ngbe_validate_mac_addr(hw->mac.addr) ==
		NGBE_ERR_INVALID_MAC_ADDR) {
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
			NGBE_PSR_MAC_SWC_AD_H_AV);
	}
	hw->addr_ctrl.overflow_promisc = 0;

	hw->addr_ctrl.rar_used_count = 1;

	/* Zero out the other receive addresses. */
	DEBUGOUT1("Clearing RAR[1-%d]\n", rar_entries - 1);
	for (i = 1; i < rar_entries; i++) {
		wr32(hw, NGBE_PSR_MAC_SWC_IDX, i);
		wr32(hw, NGBE_PSR_MAC_SWC_AD_L, 0);
		wr32(hw, NGBE_PSR_MAC_SWC_AD_H, 0);
	}

	/* Clear the MTA */
	hw->addr_ctrl.mta_in_use = 0;
	psrctl = rd32(hw, NGBE_PSR_CTL);
	psrctl &= ~(NGBE_PSR_CTL_MO | NGBE_PSR_CTL_MFE);
	psrctl |= hw->mac.mc_filter_type << NGBE_PSR_CTL_MO_SHIFT;
	wr32(hw, NGBE_PSR_CTL, psrctl);
	DEBUGOUT(" Clearing MTA\n");
	for (i = 0; i < hw->mac.mcft_size; i++)
		wr32(hw, NGBE_PSR_MC_TBL(i), 0);

	TCALL(hw, mac.ops.init_uta_tables);

	return 0;
}

/**
 *  ngbe_add_uc_addr - Adds a secondary unicast address.
 *  @hw: pointer to hardware structure
 *  @addr: new address
 *
 *  Adds it to unused receive address register or goes into promiscuous mode.
 **/
void ngbe_add_uc_addr(struct ngbe_hw *hw, u8 *addr, u32 vmdq)
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
			NGBE_PSR_MAC_SWC_AD_H_AV);
		DEBUGOUT1("Added a secondary address to RAR[%d]\n", rar);
		hw->addr_ctrl.rar_used_count++;
	} else {
		hw->addr_ctrl.overflow_promisc++;
	}

	DEBUGOUT("ngbe_add_uc_addr Complete\n");
}

/**
 *  ngbe_update_uc_addr_list - Updates MAC list of secondary addresses
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
s32 ngbe_update_uc_addr_list(struct ngbe_hw *hw, u8 *addr_list,
				      u32 addr_count, ngbe_mc_addr_itr next)
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
	DEBUGOUT1("Clearing RAR[1-%d]\n", uc_addr_in_use + 1);
	for (i = 0; i < uc_addr_in_use; i++) {
		wr32(hw, NGBE_PSR_MAC_SWC_IDX, 1 + i);
		wr32(hw, NGBE_PSR_MAC_SWC_AD_L, 0);
		wr32(hw, NGBE_PSR_MAC_SWC_AD_H, 0);
	}

	/* Add the new addresses */
	for (i = 0; i < addr_count; i++) {
		DEBUGOUT(" Adding the secondary addresses:\n");
		addr = next(hw, &addr_list, &vmdq);
		ngbe_add_uc_addr(hw, addr, vmdq);
	}

	if (hw->addr_ctrl.overflow_promisc) {
		/* enable promisc if not already in overflow or set by user */
		if (!old_promisc_setting && !hw->addr_ctrl.user_set_promisc) {
			DEBUGOUT(" Entering address overflow promisc mode\n");
			wr32m(hw, NGBE_PSR_CTL,
				NGBE_PSR_CTL_UPE, NGBE_PSR_CTL_UPE);
		}
	} else {
		/* only disable if set by overflow, not by user */
		if (old_promisc_setting && !hw->addr_ctrl.user_set_promisc) {
			DEBUGOUT(" Leaving address overflow promisc mode\n");
			wr32m(hw, NGBE_PSR_CTL,
				NGBE_PSR_CTL_UPE, 0);
		}
	}

	DEBUGOUT("ngbe_update_uc_addr_list Complete\n");
	return 0;
}

/**
 *  ngbe_mta_vector - Determines bit-vector in multicast table to set
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
STATIC s32 ngbe_mta_vector(struct ngbe_hw *hw, u8 *mc_addr)
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
 *  ngbe_set_mta - Set bit-vector in multicast table
 *  @hw: pointer to hardware structure
 *  @hash_value: Multicast address hash value
 *
 *  Sets the bit-vector in the multicast table.
 **/
void ngbe_set_mta(struct ngbe_hw *hw, u8 *mc_addr)
{
	u32 vector;
	u32 vector_bit;
	u32 vector_reg;

	DEBUGFUNC("\n");

	hw->addr_ctrl.mta_in_use++;

	vector = ngbe_mta_vector(hw, mc_addr);
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
 *  ngbe_update_mc_addr_list - Updates MAC list of multicast addresses
 *  @hw: pointer to hardware structure
 *  @mc_addr_list: the list of new multicast addresses
 *  @mc_addr_count: number of addresses
 *  @next: iterator function to walk the multicast address list
 *  @clear: flag, when set clears the table beforehand
 *
 *  When the clear flag is set, the given list replaces any existing list.
 *  Hashes the given addresses into the multicast table.
 **/
s32 ngbe_update_mc_addr_list(struct ngbe_hw *hw, u8 *mc_addr_list,
				      u32 mc_addr_count, ngbe_mc_addr_itr next,
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
		ngbe_set_mta(hw, next(hw, &mc_addr_list, &vmdq));
	}

	/* Enable mta */
	for (i = 0; i < hw->mac.mcft_size; i++)
		wr32a(hw, NGBE_PSR_MC_TBL(0), i,
				hw->mac.mta_shadow[i]);

	if (hw->addr_ctrl.mta_in_use > 0) {
		psrctl = rd32(hw, NGBE_PSR_CTL);
		psrctl &= ~(NGBE_PSR_CTL_MO | NGBE_PSR_CTL_MFE);
		psrctl |= NGBE_PSR_CTL_MFE |
			(hw->mac.mc_filter_type << NGBE_PSR_CTL_MO_SHIFT);
		wr32(hw, NGBE_PSR_CTL, psrctl);
	}

	DEBUGOUT("ngbe_update_mc_addr_list Complete\n");
	return 0;
}

/**
 *  ngbe_enable_mc - Enable multicast address in RAR
 *  @hw: pointer to hardware structure
 *
 *  Enables multicast address in RAR and the use of the multicast hash table.
 **/
s32 ngbe_enable_mc(struct ngbe_hw *hw)
{
	struct ngbe_addr_filter_info *a = &hw->addr_ctrl;
	u32 psrctl;

	DEBUGFUNC("\n");

	if (a->mta_in_use > 0) {
		psrctl = rd32(hw, NGBE_PSR_CTL);
		psrctl &= ~(NGBE_PSR_CTL_MO | NGBE_PSR_CTL_MFE);
		psrctl |= NGBE_PSR_CTL_MFE |
			(hw->mac.mc_filter_type << NGBE_PSR_CTL_MO_SHIFT);
		wr32(hw, NGBE_PSR_CTL, psrctl);
	}

	return 0;
}

/**
 *  ngbe_disable_mc - Disable multicast address in RAR
 *  @hw: pointer to hardware structure
 *
 *  Disables multicast address in RAR and the use of the multicast hash table.
 **/
s32 ngbe_disable_mc(struct ngbe_hw *hw)
{
	struct ngbe_addr_filter_info *a = &hw->addr_ctrl;
	u32 psrctl;
	DEBUGFUNC("\n");

	if (a->mta_in_use > 0) {
		psrctl = rd32(hw, NGBE_PSR_CTL);
		psrctl &= ~(NGBE_PSR_CTL_MO | NGBE_PSR_CTL_MFE);
		psrctl |= hw->mac.mc_filter_type << NGBE_PSR_CTL_MO_SHIFT;
		wr32(hw, NGBE_PSR_CTL, psrctl);
	}

	return 0;
}

/**
 *  ngbe_fc_enable - Enable flow control
 *  @hw: pointer to hardware structure
 *
 *  Enable flow control according to the current settings.
 **/
s32 ngbe_fc_enable(struct ngbe_hw *hw)
{
	s32 ret_val = 0;
	u32 mflcn_reg, fccfg_reg;
	u32 reg;
	u32 fcrtl, fcrth;
	
	DEBUGFUNC("\n");

	/* Validate the water mark configuration */
	if (!hw->fc.pause_time) {
		ret_val = NGBE_ERR_INVALID_LINK_SETTINGS;
		goto out;
	}


	/* Low water mark of zero causes XOFF floods */
	if ((hw->fc.current_mode & ngbe_fc_tx_pause) && hw->fc.high_water) {
		if (!hw->fc.low_water || hw->fc.low_water >= hw->fc.high_water) {
			DEBUGOUT("Invalid water mark configuration\n");
			ret_val = NGBE_ERR_INVALID_LINK_SETTINGS;
			goto out;
		}
	}

	/* Negotiate the fc mode to use */
	ngbe_fc_autoneg(hw);

	/* Disable any previous flow control settings */
	mflcn_reg = rd32(hw, NGBE_MAC_RX_FLOW_CTRL);
	mflcn_reg &= ~NGBE_MAC_RX_FLOW_CTRL_RFE;

	fccfg_reg = rd32(hw, NGBE_RDB_RFCC);
	fccfg_reg &= ~NGBE_RDB_RFCC_RFCE_802_3X;

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
	case ngbe_fc_none:
		/*
		 * Flow control is disabled by software override or autoneg.
		 * The code below will actually disable it in the HW.
		 */
		break;
	case ngbe_fc_rx_pause:
		/*
		 * Rx Flow control is enabled and Tx Flow control is
		 * disabled by software override. Since there really
		 * isn't a way to advertise that we are capable of RX
		 * Pause ONLY, we will advertise that we support both
		 * symmetric and asymmetric Rx PAUSE.  Later, we will
		 * disable the adapter's ability to send PAUSE frames.
		 */
		mflcn_reg |= NGBE_MAC_RX_FLOW_CTRL_RFE;
		break;
	case ngbe_fc_tx_pause:
		/*
		 * Tx Flow control is enabled, and Rx Flow control is
		 * disabled by software override.
		 */
		fccfg_reg |= NGBE_RDB_RFCC_RFCE_802_3X;
		break;
	case ngbe_fc_full:
		/* Flow control (both Rx and Tx) is enabled by SW override. */
		mflcn_reg |= NGBE_MAC_RX_FLOW_CTRL_RFE;
		fccfg_reg |= NGBE_RDB_RFCC_RFCE_802_3X;
		break;
	default:
		ERROR_REPORT1(NGBE_ERROR_ARGUMENT,
						"Flow control param set incorrectly\n");
		ret_val = NGBE_ERR_CONFIG;
		goto out;
		break;
	}

	/* Set 802.3x based flow control settings. */
	wr32(hw, NGBE_MAC_RX_FLOW_CTRL, mflcn_reg);
	wr32(hw, NGBE_RDB_RFCC, fccfg_reg);

	/* Set up and enable Rx high/low water mark thresholds, enable XON. */
	if ((hw->fc.current_mode & ngbe_fc_tx_pause) &&
			hw->fc.high_water) {
		/* 32Byte granularity */
		fcrtl = (hw->fc.low_water << 10) |
			NGBE_RDB_RFCL_XONE;
		wr32(hw, NGBE_RDB_RFCL, fcrtl);
		fcrth = (hw->fc.high_water << 10) |
			NGBE_RDB_RFCH_XOFFE;
	} else {
		wr32(hw, NGBE_RDB_RFCL, 0);
		/*
		 * In order to prevent Tx hangs when the internal Tx
		 * switch is enabled we must set the high water mark
		 * to the Rx packet buffer size - 24KB.  This allows
		 * the Tx switch to function even under heavy Rx
		 * workloads.
		 */
		fcrth = rd32(hw, NGBE_RDB_PB_SZ) - 24576;
	}

	wr32(hw, NGBE_RDB_RFCH, fcrth);

	/* Configure pause time (2 TCs per register) */
	reg = hw->fc.pause_time * 0x00010000;
	wr32(hw, NGBE_RDB_RFCV, reg);

	/* Configure flow control refresh threshold value */
	wr32(hw, NGBE_RDB_RFCRT, hw->fc.pause_time / 2);

out:
	return ret_val;
}

/**
 *  ngbe_negotiate_fc - Negotiate flow control
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
STATIC s32 ngbe_negotiate_fc(struct ngbe_hw *hw, u32 adv_reg, u32 lp_reg,
						u32 adv_sym, u32 adv_asm, u32 lp_sym, u32 lp_asm)
{
	if ((!(adv_reg)) ||  (!(lp_reg))) {
		ERROR_REPORT3(NGBE_ERROR_UNSUPPORTED,
			"Local or link partner's advertised flow control "
				"settings are NULL. Local: %x, link partner: %x\n",
					adv_reg, lp_reg);
		return NGBE_ERR_FC_NOT_NEGOTIATED;
	}

	if ((adv_reg & adv_sym) && (lp_reg & lp_sym)) {
		/*
		 * Now we need to check if the user selected Rx ONLY
		 * of pause frames.  In this case, we had to advertise
		 * FULL flow control because we could not advertise RX
		 * ONLY. Hence, we must now check to see if we need to
		 * turn OFF the TRANSMISSION of PAUSE frames.
		 */
		if (hw->fc.requested_mode == ngbe_fc_full) {
			hw->fc.current_mode = ngbe_fc_full;
			DEBUGOUT("Flow Control = FULL.\n");
		} else {
			hw->fc.current_mode = ngbe_fc_rx_pause;
			DEBUGOUT("Flow Control=RX PAUSE frames only\n");
		}
	} else if (!(adv_reg & adv_sym) && (adv_reg & adv_asm) &&
				(lp_reg & lp_sym) && (lp_reg & lp_asm)) {
		hw->fc.current_mode = ngbe_fc_tx_pause;
		DEBUGOUT("Flow Control = TX PAUSE frames only.\n");
	} else if ((adv_reg & adv_sym) && (adv_reg & adv_asm) &&
				!(lp_reg & lp_sym) && (lp_reg & lp_asm)) {
		hw->fc.current_mode = ngbe_fc_rx_pause;
		DEBUGOUT("Flow Control = RX PAUSE frames only.\n");
	} else {
		hw->fc.current_mode = ngbe_fc_none;
		DEBUGOUT("Flow Control = NONE.\n");
	}
	return 0;
}

/**
 *  ngbe_fc_autoneg_copper - Enable flow control IEEE clause 37
 *  @hw: pointer to hardware structure
 *
 *  Enable flow control according to IEEE clause 37.
 **/
STATIC s32 ngbe_fc_autoneg_copper(struct ngbe_hw *hw)
{
	u8 technology_ability_reg = 0;
	u8 lp_technology_ability_reg = 0;

	if (!((hw->subsystem_device_id & OEM_MASK) == RGMII_FPGA)) {
		TCALL(hw, phy.ops.get_adv_pause, &technology_ability_reg);
		TCALL(hw, phy.ops.get_lp_adv_pause, &lp_technology_ability_reg);
	}
	return ngbe_negotiate_fc(hw, (u32)technology_ability_reg,
								(u32)lp_technology_ability_reg,
									NGBE_TAF_SYM_PAUSE, NGBE_TAF_ASM_PAUSE,
										NGBE_TAF_SYM_PAUSE, NGBE_TAF_ASM_PAUSE);
}

/**
 *  ngbe_fc_autoneg - Configure flow control
 *  @hw: pointer to hardware structure
 *
 *  Compares our advertised flow control capabilities to those advertised by
 *  our link partner, and determines the proper flow control mode to use.
 **/
void ngbe_fc_autoneg(struct ngbe_hw *hw)
{
	s32 ret_val = NGBE_ERR_FC_NOT_NEGOTIATED;
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
		ERROR_REPORT1(NGBE_ERROR_UNSUPPORTED,
						"Flow control autoneg is disabled");
		goto out;
	}

	TCALL(hw, mac.ops.check_link, &speed, &link_up, false);
	if (!link_up) {
		ERROR_REPORT1(NGBE_ERROR_SOFTWARE, "The link is down");
		goto out;
	}

	switch (hw->phy.media_type) {
	/* Autoneg flow control on fiber adapters */
	case ngbe_media_type_fiber:
		break;

	/* Autoneg flow control on copper adapters */
	case ngbe_media_type_copper:
			ret_val = ngbe_fc_autoneg_copper(hw);
		break;

	default:
		break;
	}

out:
	if (ret_val == NGBE_OK) {
		hw->fc.fc_was_autonegged = true;
	} else {
		hw->fc.fc_was_autonegged = false;
		hw->fc.current_mode = hw->fc.requested_mode;
	}
}


/**
 *  ngbe_disable_pcie_master - Disable PCI-express master access
 *  @hw: pointer to hardware structure
 *
 *  Disables PCI-Express master access and verifies there are no pending
 *  requests. NGBE_ERR_MASTER_REQUESTS_PENDING is returned if master disable
 *  bit hasn't caused the master requests to be disabled, else 0
 *  is returned signifying master requests disabled.
 **/
s32 ngbe_disable_pcie_master(struct ngbe_hw *hw)
{
	s32 status = 0;
	u32 i;
	//struct ngbe_adapter *adapter = hw->back;

	DEBUGFUNC("\n");

	/* Always set this bit to ensure any future transactions are blocked */
	pci_clear_master(((struct ngbe_adapter *)hw->back)->pdev);

	/* Exit if master requests are blocked */
	if (!(rd32(hw, NGBE_PX_TRANSACTION_PENDING)) ||
		NGBE_REMOVED(hw->hw_addr))
		goto out;


	/* Poll for master request bit to clear */
	for (i = 0; i < NGBE_PCI_MASTER_DISABLE_TIMEOUT; i++) {
		usec_delay(100);
		if (!(rd32(hw, NGBE_PX_TRANSACTION_PENDING)))
			goto out;
	}


		ERROR_REPORT1(NGBE_ERROR_POLLING,
					"PCIe transaction pending bit did not clear.\n");
		status = NGBE_ERR_MASTER_REQUESTS_PENDING;

out:
	return status;
}

/**
 *  ngbe_acquire_swfw_sync - Acquire SWFW semaphore
 *  @hw: pointer to hardware structure
 *  @mask: Mask to specify which semaphore to acquire
 *
 *  Acquires the SWFW semaphore through the GSSR register for the specified
 *  function (CSR, PHY0, PHY1, EEPROM, Flash)
 **/
s32 ngbe_acquire_swfw_sync(struct ngbe_hw *hw, u32 mask)
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
		if (ngbe_get_eeprom_semaphore(hw))
			return NGBE_ERR_SWFW_SYNC;

		if (ngbe_check_mng_access(hw)) {
			gssr = rd32(hw, NGBE_MNG_SWFW_SYNC);
			if (!(gssr & (fwmask | swmask))) {
				gssr |= swmask;
				wr32(hw, NGBE_MNG_SWFW_SYNC, gssr);
				ngbe_release_eeprom_semaphore(hw);
				return 0;
			} else {
				/* Resource is currently in use by FW or SW */
				ngbe_release_eeprom_semaphore(hw);
				msec_delay(5);
			}
		}
	}

	ERROR_REPORT1(NGBE_ERROR_POLLING,
		"ngbe_acquire_swfw_sync: i = %u, gssr = %u\n", i, gssr);


	/* If time expired clear the bits holding the lock and retry */
	if (gssr & (fwmask | swmask))
		ngbe_release_swfw_sync(hw, gssr & (fwmask | swmask));

	msec_delay(5);
	return NGBE_ERR_SWFW_SYNC;
}

/**
 *  ngbe_release_swfw_sync - Release SWFW semaphore
 *  @hw: pointer to hardware structure
 *  @mask: Mask to specify which semaphore to release
 *
 *  Releases the SWFW semaphore through the GSSR register for the specified
 *  function (CSR, PHY0, PHY1, EEPROM, Flash)
 **/
void ngbe_release_swfw_sync(struct ngbe_hw *hw, u32 mask)
{
	ngbe_get_eeprom_semaphore(hw);
	if (ngbe_check_mng_access(hw))
		wr32m(hw, NGBE_MNG_SWFW_SYNC, mask, 0);

	ngbe_release_eeprom_semaphore(hw);
}

/**
 *  ngbe_disable_sec_rx_path - Stops the receive data path
 *  @hw: pointer to hardware structure
 *
 *  Stops the receive data path and waits for the HW to internally empty
 *  the Rx security block
 **/
s32 ngbe_disable_sec_rx_path(struct ngbe_hw *hw)
{
#define NGBE_MAX_SECRX_POLL 40

	int i;
	int secrxreg;

	DEBUGFUNC("\n");

	wr32m(hw, NGBE_RSEC_CTL,
		NGBE_RSEC_CTL_RX_DIS, NGBE_RSEC_CTL_RX_DIS);
	for (i = 0; i < NGBE_MAX_SECRX_POLL; i++) {
		secrxreg = rd32(hw, NGBE_RSEC_ST);
		if (secrxreg & NGBE_RSEC_ST_RSEC_RDY)
			break;
		else
			/* Use interrupt-safe sleep just in case */
			usec_delay(1000);
	}

	/* For informational purposes only */
	if (i >= NGBE_MAX_SECRX_POLL)
		DEBUGOUT("Rx unit being enabled before security "
			 "path fully disabled.  Continuing with init.\n");

	return 0;
}

/**
 *  ngbe_enable_sec_rx_path - Enables the receive data path
 *  @hw: pointer to hardware structure
 *
 *  Enables the receive data path.
 **/
s32 ngbe_enable_sec_rx_path(struct ngbe_hw *hw)
{
	DEBUGFUNC("\n");

	wr32m(hw, NGBE_RSEC_CTL,
		NGBE_RSEC_CTL_RX_DIS, 0);
	NGBE_WRITE_FLUSH(hw);

	return 0;
}

/**
 *  ngbe_insert_mac_addr - Find a RAR for this mac address
 *  @hw: pointer to hardware structure
 *  @addr: Address to put into receive address register
 *  @vmdq: VMDq pool to assign
 *
 *  Puts an ethernet address into a receive address register, or
 *  finds the rar that it is aleady in; adds to the pool list
 **/
s32 ngbe_insert_mac_addr(struct ngbe_hw *hw, u8 *addr, u32 vmdq)
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
		wr32(hw, NGBE_PSR_MAC_SWC_IDX, rar);
		rar_high = rd32(hw, NGBE_PSR_MAC_SWC_AD_H);

		if (((NGBE_PSR_MAC_SWC_AD_H_AV & rar_high) == 0)
		    && first_empty_rar == NO_EMPTY_RAR_FOUND) {
			first_empty_rar = rar;
		} else if ((rar_high & 0xFFFF) == addr_high) {
			rar_low = rd32(hw, NGBE_PSR_MAC_SWC_AD_L);
			if (rar_low == addr_low)
				break;    /* found it already in the rars */
		}
	}

	if (rar < hw->mac.rar_highwater) {

	} else if (first_empty_rar != NO_EMPTY_RAR_FOUND) {
		/* stick it into first empty RAR slot we found */
		rar = first_empty_rar;
		TCALL(hw, mac.ops.set_rar, rar, addr, vmdq,
			       NGBE_PSR_MAC_SWC_AD_H_AV);
	} else if (rar == hw->mac.rar_highwater) {
		/* add it to the top of the list and inc the highwater mark */
		TCALL(hw, mac.ops.set_rar, rar, addr, vmdq,
			       NGBE_PSR_MAC_SWC_AD_H_AV);
		hw->mac.rar_highwater++;
	} else if (rar >= hw->mac.num_rar_entries) {
		return NGBE_ERR_INVALID_MAC_ADDR;
	}

	return rar;
}

/**
 *  ngbe_clear_vmdq - Disassociate a VMDq pool index from a rx address
 *  @hw: pointer to hardware struct
 *  @rar: receive address register index to disassociate
 *  @vmdq: VMDq pool index to remove from the rar
 **/
s32 ngbe_clear_vmdq(struct ngbe_hw *hw, u32 rar, u32 vmdq)
{
	u32 mpsar_lo;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("\n");
	UNREFERENCED_PARAMETER(vmdq);

	/* Make sure we are using a valid rar index range */
	if (rar >= rar_entries) {
		ERROR_REPORT2(NGBE_ERROR_ARGUMENT,
			     "RAR index %d is out of range.\n", rar);
		return NGBE_ERR_INVALID_ARGUMENT;
	}

	wr32(hw, NGBE_PSR_MAC_SWC_IDX, rar);
	mpsar_lo = rd32(hw, NGBE_PSR_MAC_SWC_VM);

	if (NGBE_REMOVED(hw->hw_addr))
		goto done;

	if (!mpsar_lo)
		goto done;

	/* was that the last pool using this rar? */
	if (mpsar_lo == 0 && rar != 0)
		TCALL(hw, mac.ops.clear_rar, rar);
done:
	return 0;
}

/**
 *  ngbe_set_vmdq - Associate a VMDq pool index with a rx address
 *  @hw: pointer to hardware struct
 *  @rar: receive address register index to associate with a VMDq index
 *  @vmdq: VMDq pool index
 **/
s32 ngbe_set_vmdq(struct ngbe_hw *hw, u32 rar, u32 pool)
{
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("\n");
	UNREFERENCED_PARAMETER(pool);

	/* Make sure we are using a valid rar index range */
	if (rar >= rar_entries) {
		ERROR_REPORT2(NGBE_ERROR_ARGUMENT,
			     "RAR index %d is out of range.\n", rar);
		return NGBE_ERR_INVALID_ARGUMENT;
	}

	return 0;
}

/**
 *  This function should only be involved in the IOV mode.
 *  In IOV mode, Default pool is next pool after the number of
 *  VFs advertized and not 0.
 *  MPSAR table needs to be updated for SAN_MAC RAR [hw->mac.san_mac_rar_index]
 *
 *  ngbe_set_vmdq_san_mac - Associate default VMDq pool index with a rx address
 *  @hw: pointer to hardware struct
 *  @vmdq: VMDq pool index
 **/
s32 ngbe_set_vmdq_san_mac(struct ngbe_hw *hw, u32 vmdq)
{
	u32 rar = hw->mac.san_mac_rar_index;

	DEBUGFUNC("\n");
	/* ??? */
	if (vmdq > 32)
		return -1;

	wr32(hw, NGBE_PSR_MAC_SWC_IDX, rar);
	wr32(hw, NGBE_PSR_MAC_SWC_VM, 1 << vmdq);


	return 0;
}

/**
 *  ngbe_init_uta_tables - Initialize the Unicast Table Array
 *  @hw: pointer to hardware structure
 **/
s32 ngbe_init_uta_tables(struct ngbe_hw *hw)
{
	int i;

	DEBUGFUNC("\n");
	DEBUGOUT(" Clearing UTA\n");

	for (i = 0; i < 128; i++)
		wr32(hw, NGBE_PSR_UC_TBL(i), 0);

	return 0;
}

/**
 *  ngbe_find_vlvf_slot - find the vlanid or the first empty slot
 *  @hw: pointer to hardware structure
 *  @vlan: VLAN id to write to VLAN filter
 *
 *  return the VLVF index where this VLAN id should be placed
 *
 **/
s32 ngbe_find_vlvf_slot(struct ngbe_hw *hw, u32 vlan)
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
	for (regindex = 1; regindex < NGBE_PSR_VLAN_SWC_ENTRIES; regindex++) {
		wr32(hw, NGBE_PSR_VLAN_SWC_IDX, regindex);
		bits = rd32(hw, NGBE_PSR_VLAN_SWC);
		if (!bits && !(first_empty_slot))
			first_empty_slot = regindex;
		else if ((bits & 0x0FFF) == vlan)
			break;
	}

	/*
	 * If regindex is less than NGBE_VLVF_ENTRIES, then we found the vlan
	 * in the VLVF. Else use the first empty VLVF register for this
	 * vlan id.
	 */
	if (regindex >= NGBE_PSR_VLAN_SWC_ENTRIES) {
		if (first_empty_slot)
			regindex = first_empty_slot;
		else {
			ERROR_REPORT1(NGBE_ERROR_SOFTWARE,
				     "No space in VLVF.\n");
			regindex = NGBE_ERR_NO_SPACE;
		}
	}

	return regindex;
}

/**
 *  ngbe_set_vfta - Set VLAN filter table
 *  @hw: pointer to hardware structure
 *  @vlan: VLAN id to write to VLAN filter
 *  @vind: VMDq output index that maps queue to VLAN id in VFVFB
 *  @vlan_on: boolean flag to turn on/off VLAN in VFVF
 *
 *  Turn on/off specified VLAN in the VLAN filter table.
 **/
s32 ngbe_set_vfta(struct ngbe_hw *hw, u32 vlan, u32 vind,
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
		return NGBE_ERR_PARAM;

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
	 * Call ngbe_set_vlvf to set VLVFB and VLVF
	 */
	ret_val = ngbe_set_vlvf(hw, vlan, vind, vlan_on,
					 &vfta_changed);
	if (ret_val != 0)
		return ret_val;

	if (vfta_changed)
		wr32(hw, NGBE_PSR_VLAN_TBL(regindex), vfta);
	/* errata 5 */
	hw->mac.vft_shadow[regindex] = vfta;
	return 0;
}

/**
 *  ngbe_set_vlvf - Set VLAN Pool Filter
 *  @hw: pointer to hardware structure
 *  @vlan: VLAN id to write to VLAN filter
 *  @vind: VMDq output index that maps queue to VLAN id in VFVFB
 *  @vlan_on: boolean flag to turn on/off VLAN in VFVF
 *  @vfta_changed: pointer to boolean flag which indicates whether VFTA
 *                 should be changed
 *
 *  Turn on/off specified bit in VLVF table.
 **/
s32 ngbe_set_vlvf(struct ngbe_hw *hw, u32 vlan, u32 vind,
			    bool vlan_on, bool *vfta_changed)
{
	u32 vt;

	DEBUGFUNC("\n");

	if (vlan > 4095)
		return NGBE_ERR_PARAM;

	/* If VT Mode is set
	 *   Either vlan_on
	 *     make sure the vlan is in VLVF
	 *     set the vind bit in the matching VLVFB
	 *   Or !vlan_on
	 *     clear the pool bit and possibly the vind
	 */
	vt = rd32(hw, NGBE_CFG_PORT_CTL);
	if (vt & NGBE_CFG_PORT_CTL_NUM_VT_MASK) {
		s32 vlvf_index;
		u32 bits = 0;

		vlvf_index = ngbe_find_vlvf_slot(hw, vlan);
		if (vlvf_index < 0)
			return vlvf_index;

		wr32(hw, NGBE_PSR_VLAN_SWC_IDX, vlvf_index);
		if (vlan_on) {
			/* set the pool bit */
			if (vind < 32) {
				bits = rd32(hw,
						NGBE_PSR_VLAN_SWC_VM_L);
				bits |= (1 << vind);
				wr32(hw,
						NGBE_PSR_VLAN_SWC_VM_L,
						bits);
			}
		} else {
			/* clear the pool bit */
			if (vind < 32) {
				bits = rd32(hw,
						NGBE_PSR_VLAN_SWC_VM_L);
				bits &= ~(1 << vind);
				wr32(hw,
						NGBE_PSR_VLAN_SWC_VM_L,
						bits);
			} else {
				bits |= rd32(hw,
						NGBE_PSR_VLAN_SWC_VM_L);
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
			wr32(hw, NGBE_PSR_VLAN_SWC,
					(NGBE_PSR_VLAN_SWC_VIEN | vlan));
			if ((!vlan_on) && (vfta_changed != NULL)) {
				/* someone wants to clear the vfta entry
				 * but some pools/VFs are still using it.
				 * Ignore it. */
				*vfta_changed = false;
			}
		} else
			wr32(hw, NGBE_PSR_VLAN_SWC, 0);
	}

	return 0;
}

/**
 *  ngbe_clear_vfta - Clear VLAN filter table
 *  @hw: pointer to hardware structure
 *
 *  Clears the VLAN filer table, and the VMDq index associated with the filter
 **/
s32 ngbe_clear_vfta(struct ngbe_hw *hw)
{
	u32 offset;

	DEBUGFUNC("\n");

	for (offset = 0; offset < hw->mac.vft_size; offset++) {
		wr32(hw, NGBE_PSR_VLAN_TBL(offset), 0);
		/* errata 5 */
		hw->mac.vft_shadow[offset] = 0;
	}

	for (offset = 0; offset < NGBE_PSR_VLAN_SWC_ENTRIES; offset++) {
		wr32(hw, NGBE_PSR_VLAN_SWC_IDX, offset);
		wr32(hw, NGBE_PSR_VLAN_SWC, 0);
		wr32(hw, NGBE_PSR_VLAN_SWC_VM_L, 0);
	}

	return 0;
}


/**
 *  ngbe_set_mac_anti_spoofing - Enable/Disable MAC anti-spoofing
 *  @hw: pointer to hardware structure
 *  @enable: enable or disable switch for anti-spoofing
 *  @pf: Physical Function pool - do not enable anti-spoofing for the PF
 *
 **/
void ngbe_set_mac_anti_spoofing(struct ngbe_hw *hw, bool enable, int pf)
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
		wr32(hw, NGBE_TDM_MAC_AS_L,
				pfvfspoof & 0xff);
	} else {
		wr32(hw, NGBE_TDM_MAC_AS_L, 0);
	}
}

/**
 *  ngbe_set_vlan_anti_spoofing - Enable/Disable VLAN anti-spoofing
 *  @hw: pointer to hardware structure
 *  @enable: enable or disable switch for VLAN anti-spoofing
 *  @vf: Virtual Function pool - VF Pool to set for VLAN anti-spoofing
 *
 **/
void ngbe_set_vlan_anti_spoofing(struct ngbe_hw *hw, bool enable, int vf)
{
	u32 pfvfspoof;

	DEBUGFUNC("\n");

	if (vf > 8)
		return;

	pfvfspoof = rd32(hw, NGBE_TDM_VLAN_AS_L);
	if (enable)
		pfvfspoof |= (1 << vf);
	else
		pfvfspoof &= ~(1 << vf);
	wr32(hw, NGBE_TDM_VLAN_AS_L, pfvfspoof);

}

/**
 *  ngbe_set_ethertype_anti_spoofing - Enable/Disable Ethertype anti-spoofing
 *  @hw: pointer to hardware structure
 *  @enable: enable or disable switch for Ethertype anti-spoofing
 *  @vf: Virtual Function pool - VF Pool to set for Ethertype anti-spoofing
 *
 **/
void ngbe_set_ethertype_anti_spoofing(struct ngbe_hw *hw,
		bool enable, int vf)
{
	u32 pfvfspoof;

	DEBUGFUNC("\n");

	if (vf <= 8) {
		pfvfspoof = rd32(hw, NGBE_TDM_ETYPE_AS_L);
		if (enable)
			pfvfspoof |= (1 << vf);
		else
			pfvfspoof &= ~(1 << vf);
		wr32(hw, NGBE_TDM_ETYPE_AS_L, pfvfspoof);
	}
}

/**
 *  ngbe_get_device_caps - Get additional device capabilities
 *  @hw: pointer to hardware structure
 *  @device_caps: the EEPROM word with the extra device capabilities
 *
 *  This function will read the EEPROM location for the device capabilities,
 *  and return the word through device_caps.
 **/
s32 ngbe_get_device_caps(struct ngbe_hw *hw, u16 *device_caps)
{
	DEBUGFUNC("\n");

	TCALL(hw, eeprom.ops.read,
		hw->eeprom.sw_region_offset + NGBE_DEVICE_CAPS, device_caps);

	return 0;
}

/**
 *  ngbe_calculate_checksum - Calculate checksum for buffer
 *  @buffer: pointer to EEPROM
 *  @length: size of EEPROM to calculate a checksum for
 *  Calculates the checksum for some buffer on a specified length.  The
 *  checksum calculated is returned.
 **/
u8 ngbe_calculate_checksum(u8 *buffer, u32 length)
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


s32 ngbe_host_interface_pass_command(struct ngbe_hw *hw, u32 *buffer,
				 u32 length, u32 timeout, bool return_data)
{
	u32 i;
	u32 dword_len;
	s32 status = 0;

	DEBUGFUNC("\n");

	if (length == 0 || length > NGBE_HI_MAX_BLOCK_BYTE_LENGTH) {
		DEBUGOUT1("Buffer length failure buffersize=%d.\n", length);
		return NGBE_ERR_HOST_INTERFACE_COMMAND;
	}

	if (TCALL(hw, mac.ops.acquire_swfw_sync, NGBE_MNG_SWFW_SYNC_SW_MB)
	    != 0) {
		return NGBE_ERR_SWFW_SYNC;
	}

	/* Calculate length in DWORDs. We must be DWORD aligned */
	if ((length % (sizeof(u32))) != 0) {
		DEBUGOUT("Buffer length failure, not aligned to dword");
		status = NGBE_ERR_INVALID_ARGUMENT;
		goto rel_out;
	}

	dword_len = length >> 2;

	/* The device driver writes the relevant command block
	 * into the ram area.
	 */
	for (i = 0; i < dword_len; i++) {
		if (ngbe_check_mng_access(hw))
			wr32a(hw, NGBE_MNG_MBOX,
				      i, NGBE_CPU_TO_LE32(buffer[i]));
		else {
			status = NGBE_ERR_MNG_ACCESS_FAILED;
			goto rel_out;
		}
	}
	/* Setting this bit tells the ARC that a new command is pending. */
	if (ngbe_check_mng_access(hw))
		wr32m(hw, NGBE_MNG_MBOX_CTL,
			NGBE_MNG_MBOX_CTL_SWRDY, NGBE_MNG_MBOX_CTL_SWRDY);
	else {
		status = NGBE_ERR_MNG_ACCESS_FAILED;
		goto rel_out;
	}

rel_out:
	TCALL(hw, mac.ops.release_swfw_sync, NGBE_MNG_SWFW_SYNC_SW_MB);
	return status;
}



/**
 *  ngbe_host_interface_command - Issue command to manageability block
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
 *  else return NGBE_ERR_HOST_INTERFACE_COMMAND.
 **/
s32 ngbe_host_interface_command(struct ngbe_hw *hw, u32 *buffer,
				 u32 length, u32 timeout, bool return_data)
{
	u32 hicr, i, bi;
	u32 hdr_size = sizeof(struct ngbe_hic_hdr);
	u16 buf_len;
	u32 dword_len;
	s32 status = 0;
	u32 buf[64] = {};

	DEBUGFUNC("\n");

	if (length == 0 || length > NGBE_HI_MAX_BLOCK_BYTE_LENGTH) {
		DEBUGOUT1("Buffer length failure buffersize=%d.\n", length);
		return NGBE_ERR_HOST_INTERFACE_COMMAND;
	}

	if (TCALL(hw, mac.ops.acquire_swfw_sync, NGBE_MNG_SWFW_SYNC_SW_MB)
	    != 0) {
		return NGBE_ERR_SWFW_SYNC;
	}


	/* Calculate length in DWORDs. We must be DWORD aligned */
	if ((length % (sizeof(u32))) != 0) {
		DEBUGOUT("Buffer length failure, not aligned to dword");
		status = NGBE_ERR_INVALID_ARGUMENT;
		goto rel_out;
	}

	/*read to clean all status*/
	if (ngbe_check_mng_access(hw)) {
		hicr = rd32(hw, NGBE_MNG_MBOX_CTL);
		if ((hicr & NGBE_MNG_MBOX_CTL_FWRDY))
			ERROR_REPORT1(NGBE_ERROR_CAUTION,
							"fwrdy is set before command.\n");
	}
	
	dword_len = length >> 2;

	/* The device driver writes the relevant command block
	 * into the ram area.
	 */
	for (i = 0; i < dword_len; i++) {
		if (ngbe_check_mng_access(hw))
			wr32a(hw, NGBE_MNG_MBOX,
				      i, NGBE_CPU_TO_LE32(buffer[i]));
		else {
			status = NGBE_ERR_MNG_ACCESS_FAILED;
			goto rel_out;
		}
	}
	/* Setting this bit tells the ARC that a new command is pending. */
	if (ngbe_check_mng_access(hw))
		wr32m(hw, NGBE_MNG_MBOX_CTL,
			NGBE_MNG_MBOX_CTL_SWRDY, NGBE_MNG_MBOX_CTL_SWRDY);
	else {
		status = NGBE_ERR_MNG_ACCESS_FAILED;
		goto rel_out;
	}

	for (i = 0; i < timeout; i++) {
		if (ngbe_check_mng_access(hw)) {
			hicr = rd32(hw, NGBE_MNG_MBOX_CTL);
			if ((hicr & NGBE_MNG_MBOX_CTL_FWRDY))
				break;
		}
		msec_delay(1);
	}

	buf[0] = rd32(hw, NGBE_MNG_MBOX);
	/* Check command completion */
	if (timeout != 0 && i == timeout) {
		ERROR_REPORT1(NGBE_ERROR_CAUTION,
			     "Command has failed with no status valid.\n");
		printk("===%x= %x=\n", buffer[0] & 0xff, (~buf[0] >> 24));
		printk("===%08x\n", rd32(hw, 0x1e100));
		printk("===%08x\n", rd32(hw, 0x1e104));
		printk("===%08x\n", rd32(hw, 0x1e108));
		printk("===%08x\n", rd32(hw, 0x1e10c));
		printk("===%08x\n", rd32(hw, 0x1e044));
		printk("===%08x\n", rd32(hw, 0x10000));
		if( (buffer[0] & 0xff) != (~buf[0] >> 24)) {
			status = NGBE_ERR_HOST_INTERFACE_COMMAND;
			goto rel_out;
		}
	}

	if (!return_data)
		goto rel_out;

	/* Calculate length in DWORDs */
	dword_len = hdr_size >> 2;

	/* first pull in the header so we know the buffer length */
	for (bi = 0; bi < dword_len; bi++) {
		if (ngbe_check_mng_access(hw)) {
			buffer[bi] = rd32a(hw, NGBE_MNG_MBOX,
							bi);
			NGBE_LE32_TO_CPUS(&buffer[bi]);
		} else {
			status = NGBE_ERR_MNG_ACCESS_FAILED;
			goto rel_out;
		}
	}

	/* If there is any thing in data position pull it in */
	buf_len = ((struct ngbe_hic_hdr *)buffer)->buf_len;
	if (buf_len == 0)
		goto rel_out;

	if (length < buf_len + hdr_size) {
		DEBUGOUT("Buffer not large enough for reply message.\n");
		status = NGBE_ERR_HOST_INTERFACE_COMMAND;
		goto rel_out;
	}

	/* Calculate length in DWORDs, add 3 for odd lengths */
	dword_len = (buf_len + 3) >> 2;

	/* Pull in the rest of the buffer (bi is where we left off) */
	for (; bi <= dword_len; bi++) {
		if (ngbe_check_mng_access(hw)) {
			buffer[bi] = rd32a(hw, NGBE_MNG_MBOX,
							bi);
			NGBE_LE32_TO_CPUS(&buffer[bi]);
		} else {
			status = NGBE_ERR_MNG_ACCESS_FAILED;
			goto rel_out;
		}
	}

rel_out:
	TCALL(hw, mac.ops.release_swfw_sync, NGBE_MNG_SWFW_SYNC_SW_MB);
	return status;
}

/**
 *  ngbe_set_fw_drv_ver - Sends driver version to firmware
 *  @hw: pointer to the HW structure
 *  @maj: driver version major number
 *  @min: driver version minor number
 *  @build: driver version build number
 *  @sub: driver version sub build number
 *
 *  Sends driver version number to firmware through the manageability
 *  block.  On success return 0
 *  else returns NGBE_ERR_SWFW_SYNC when encountering an error acquiring
 *  semaphore or NGBE_ERR_HOST_INTERFACE_COMMAND when command fails.
 **/
s32 ngbe_set_fw_drv_ver(struct ngbe_hw *hw, u8 maj, u8 min,
				 u8 build, u8 sub)
{
	struct ngbe_hic_drv_info fw_cmd;
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
	fw_cmd.hdr.checksum = ngbe_calculate_checksum((u8 *)&fw_cmd,
				(FW_CEM_HDR_LEN + fw_cmd.hdr.buf_len));
	fw_cmd.pad = 0;
	fw_cmd.pad2 = 0;

	for (i = 0; i <= FW_CEM_MAX_RETRIES; i++) {
		ret_val = ngbe_host_interface_command(hw, (u32 *)&fw_cmd,
						       sizeof(fw_cmd),
						       NGBE_HI_COMMAND_TIMEOUT,
						       true);
		if (ret_val != 0)
			continue;

		if (fw_cmd.hdr.cmd_or_resp.ret_status ==
		    FW_CEM_RESP_STATUS_SUCCESS)
			ret_val = 0;
		else
			ret_val = NGBE_ERR_HOST_INTERFACE_COMMAND;

		break;
	}

	return ret_val;
}

/**
 *  ngbe_reset_hostif - send reset cmd to fw
 *  @hw: pointer to hardware structure
 *
 *  Sends reset cmd to firmware through the manageability
 *  block.  On success return 0
 *  else returns NGBE_ERR_SWFW_SYNC when encountering an error acquiring
 *  semaphore or NGBE_ERR_HOST_INTERFACE_COMMAND when command fails.
 **/
s32 ngbe_reset_hostif(struct ngbe_hw *hw)
{
	struct ngbe_hic_reset reset_cmd;
	int i;
	s32 status = 0;

	DEBUGFUNC("\n");

	reset_cmd.hdr.cmd = FW_RESET_CMD;
	reset_cmd.hdr.buf_len = FW_RESET_LEN;
	reset_cmd.hdr.cmd_or_resp.cmd_resv = FW_CEM_CMD_RESERVED;
	reset_cmd.lan_id = hw->bus.lan_id;
	reset_cmd.reset_type = (u16)hw->reset_type;
	reset_cmd.hdr.checksum = 0;
	reset_cmd.hdr.checksum = ngbe_calculate_checksum((u8 *)&reset_cmd,
				(FW_CEM_HDR_LEN + reset_cmd.hdr.buf_len));

	/* send reset request to FW and wait for response */
	for (i = 0; i <= FW_CEM_MAX_RETRIES; i++) {
		status = ngbe_host_interface_command(hw, (u32 *)&reset_cmd,
						       sizeof(reset_cmd),
						       NGBE_HI_COMMAND_TIMEOUT,
						       true);
		msleep(1);
		if (status != 0)
			continue;

		if (reset_cmd.hdr.cmd_or_resp.ret_status ==
			FW_CEM_RESP_STATUS_SUCCESS)
			status = 0;
		else
			status = NGBE_ERR_HOST_INTERFACE_COMMAND;

		break;
	}

	return status;
}


s32 ngbe_setup_mac_link_hostif(struct ngbe_hw *hw, u32 speed)
{
	struct ngbe_hic_phy_cfg cmd;
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
	cmd.hdr.checksum = ngbe_calculate_checksum((u8 *)&cmd,
				(FW_CEM_HDR_LEN + cmd.hdr.buf_len));

	for (i = 0; i <= FW_CEM_MAX_RETRIES; i++) {
		status = ngbe_host_interface_command(hw, (u32 *)&cmd,
						       sizeof(cmd),
						       NGBE_HI_COMMAND_TIMEOUT,
						       true);
		if (status != 0)
			continue;

		if (cmd.hdr.cmd_or_resp.ret_status ==
		    FW_CEM_RESP_STATUS_SUCCESS)
			status = 0;
		else
			status = NGBE_ERR_HOST_INTERFACE_COMMAND;

		break;
	}

	return status;

}

u16 ngbe_crc16_ccitt(const u8 *buf, int size)
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

s32 ngbe_upgrade_flash_hostif(struct ngbe_hw *hw, u32 region,
				const u8 *data, u32 size)
{
	struct ngbe_hic_upg_start start_cmd;
	struct ngbe_hic_upg_write write_cmd;
	struct ngbe_hic_upg_verify verify_cmd;
	u32 offset;
	s32 status = 0;

	DEBUGFUNC("\n");

	start_cmd.hdr.cmd = FW_FLASH_UPGRADE_START_CMD;
	start_cmd.hdr.buf_len = FW_FLASH_UPGRADE_START_LEN;
	start_cmd.hdr.cmd_or_resp.cmd_resv = FW_CEM_CMD_RESERVED;
	start_cmd.module_id = (u8)region;
	start_cmd.hdr.checksum = 0;
	start_cmd.hdr.checksum = ngbe_calculate_checksum((u8 *)&start_cmd,
				(FW_CEM_HDR_LEN + start_cmd.hdr.buf_len));
	start_cmd.pad2 = 0;
	start_cmd.pad3 = 0;

	status = ngbe_host_interface_command(hw, (u32 *)&start_cmd,
					       sizeof(start_cmd),
					       NGBE_HI_FLASH_ERASE_TIMEOUT,
					       true);

	if (start_cmd.hdr.cmd_or_resp.ret_status == FW_CEM_RESP_STATUS_SUCCESS)
		status = 0;
	else {
		status = NGBE_ERR_HOST_INTERFACE_COMMAND;
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
		write_cmd.check_sum = ngbe_crc16_ccitt((u8 *)write_cmd.data,
							write_cmd.data_len * 4);

		status = ngbe_host_interface_command(hw, (u32 *)&write_cmd,
						sizeof(write_cmd),
						NGBE_HI_FLASH_UPDATE_TIMEOUT,
						true);
		if (start_cmd.hdr.cmd_or_resp.ret_status ==
						FW_CEM_RESP_STATUS_SUCCESS)
			status = 0;
		else {
			status = NGBE_ERR_HOST_INTERFACE_COMMAND;
			return status;
		}
		offset += write_cmd.data_len * 4;
	}

	verify_cmd.hdr.cmd = FW_FLASH_UPGRADE_VERIFY_CMD;
	verify_cmd.hdr.buf_len = FW_FLASH_UPGRADE_VERIFY_LEN;
	verify_cmd.hdr.cmd_or_resp.cmd_resv = FW_CEM_CMD_RESERVED;
	switch (region) {
	case NGBE_MODULE_EEPROM:
		verify_cmd.action_flag = NGBE_RELOAD_EEPROM;
		break;
	case NGBE_MODULE_FIRMWARE:
		verify_cmd.action_flag = NGBE_RESET_FIRMWARE;
		break;
	case NGBE_MODULE_HARDWARE:
		verify_cmd.action_flag = NGBE_RESET_LAN;
		break;
	default:
		ERROR_REPORT1(NGBE_ERROR_ARGUMENT,
				"ngbe_upgrade_flash_hostif: region err %x\n", region);
		return status;
	}

	verify_cmd.hdr.checksum = ngbe_calculate_checksum((u8 *)&verify_cmd,
				(FW_CEM_HDR_LEN + verify_cmd.hdr.buf_len));

	status = ngbe_host_interface_command(hw, (u32 *)&verify_cmd,
					       sizeof(verify_cmd),
					       NGBE_HI_FLASH_VERIFY_TIMEOUT,
					       true);

	if (verify_cmd.hdr.cmd_or_resp.ret_status == FW_CEM_RESP_STATUS_SUCCESS)
		status = 0;
	else {
		status = NGBE_ERR_HOST_INTERFACE_COMMAND;
	}
	return status;
}

/* cmd_addr is used for some special command:
**	 1. to be sector address, when implemented erase sector command
**	 2. to be flash address when implemented read, write flash address
*/
u8 fmgr_cmd_op(struct ngbe_hw *hw, u32 cmd, u32 cmd_addr)
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
				
u8 fmgr_usr_cmd_op(struct ngbe_hw *hw, u32 usr_cmd)
{
	u8 status = 0;

	wr32(hw, SPI_H_USR_CMD_REG_ADDR, usr_cmd);
	status = fmgr_cmd_op(hw, SPI_CMD_USER_CMD, 0);

	return status;
}

u8 flash_erase_chip(struct ngbe_hw *hw)
{
	u8 status = fmgr_cmd_op(hw, SPI_CMD_ERASE_CHIP, 0);
	return status;
}

u8 flash_erase_sector(struct ngbe_hw *hw, u32 sec_addr)
{
	u8 status = fmgr_cmd_op(hw, SPI_CMD_ERASE_SECTOR, sec_addr);
	return status;
}

u32 flash_read_dword(struct ngbe_hw *hw, u32 addr)
{
	u8 status = fmgr_cmd_op(hw, SPI_CMD_READ_DWORD, addr);
	if (status)
		return (u32)status;

	return rd32(hw, SPI_H_DAT_REG_ADDR);
}

u8 flash_write_dword(struct ngbe_hw *hw, u32 addr, u32 dword)
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
int ngbe_flash_write_cab(struct ngbe_hw *hw,u32 addr, u32 value,u16 lan_id)
{
	int status;
	struct ngbe_hic_read_cab buffer;

	buffer.hdr.req.cmd = 0xE2;
	buffer.hdr.req.buf_lenh = 0x6;
	buffer.hdr.req.buf_lenl = 0x0;
	buffer.hdr.req.checksum = 0xFF;
	
	/* convert offset from words to bytes */
	buffer.dbuf.d16[0] = cpu_to_le16(lan_id);
	/* one word */
	buffer.dbuf.d32[0] = htonl(addr);
	buffer.dbuf.d32[1] = htonl(value);

	status = ngbe_host_interface_command(hw, (u32 *)&buffer,
						sizeof(buffer), 5000, true);
	printk("0x1e100 :%08x\n",rd32(hw,0x1e100));
	printk("0x1e104 :%08x\n",rd32(hw,0x1e104));
	printk("0x1e108 :%08x\n",rd32(hw,0x1e108));
	printk("0x1e10c :%08x\n",rd32(hw,0x1e10c));

	return status;
}
int ngbe_flash_read_cab(struct ngbe_hw *hw, u32 addr ,u16 lan_id )
{
	int status;
	struct ngbe_hic_read_cab buffer;
	u16 *data = NULL;

	buffer.hdr.req.cmd = 0xE1;
	buffer.hdr.req.buf_lenh = 0xaa;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = 0xFF;
	
	/* convert offset from words to bytes */
	buffer.dbuf.d16[0] = cpu_to_le16(lan_id);
	/* one word */
	buffer.dbuf.d32[0] = htonl(addr);

	status = ngbe_host_interface_command(hw, (u32 *)&buffer,
						sizeof(buffer), 5000, true);

	if (status)
		return status;
	if (ngbe_check_mng_access(hw)) {
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
int ngbe_flash_write_unlock(struct ngbe_hw *hw)
{
	int status;
	struct ngbe_hic_read_shadow_ram buffer;

	buffer.hdr.req.cmd = 0x40;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = 0xFF;
	
	/* convert offset from words to bytes */
	buffer.address = 0;
	/* one word */
	buffer.length = 0;

	status = ngbe_host_interface_command(hw, (u32 *)&buffer,
						sizeof(buffer), 5000,false);
	if (status)
		return status;

	return status;
}

int ngbe_flash_write_lock(struct ngbe_hw *hw)
{
	int status;
	struct ngbe_hic_read_shadow_ram buffer;

	buffer.hdr.req.cmd = 0x39;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = 0xFF;
	
	/* convert offset from words to bytes */
	buffer.address = 0;
	/* one word */
	buffer.length = 0;

	status = ngbe_host_interface_command(hw, (u32 *)&buffer,
						sizeof(buffer), 5000, false);
	if (status)
		return status;

	return status;
}

int ngbe_upgrade_flash(struct ngbe_hw *hw, u32 region,
				const u8 *data, u32 size)
{
	u32 			sector_num = 0;
	u32 			read_data = 0;
	u8				status = 0;
	u8				skip = 0;
	u32 			i = 0,k = 0, n = 0;
	u8				flash_vendor = 0;
	u32				num[256] = {0};
	u32		mac_addr0_dword0_t, mac_addr0_dword1_t;
	u32		mac_addr1_dword0_t, mac_addr1_dword1_t;
	u32		mac_addr2_dword0_t, mac_addr2_dword1_t;
	u32		mac_addr3_dword0_t, mac_addr3_dword1_t;
	u32		serial_num_dword0_t, serial_num_dword1_t, serial_num_dword2_t;

	/*check sub_id*/;
	printk("Checking sub_id .......\n");
	printk("The card's sub_id : %04x\n", hw->subsystem_device_id);
	printk("The image's sub_id : %04x\n", data[0xfffdc] << 8  | data[0xfffdd]);
	if ((hw->subsystem_device_id & 0xffff) == 
		((data[0xfffdc] << 8  | data[0xfffdd]) & 0xffff)){
		printk("It is a right image\n");
	} else if (hw->subsystem_device_id == 0xffff){
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
	if (!((hw->device_id & 0xffff) == ((data[0xfffde] << 8  | data[0xfffdf]) & 0xffff)) 
		&& !(hw->device_id == 0xffff))
	{
		printk("====The Gigabit image is not match the Gigabit card====\n");
		printk("====Please check your image====\n");
		return -EOPNOTSUPP;
	}

	// unlock flash write protect
	ngbe_release_eeprom_semaphore(hw);
	ngbe_flash_write_unlock(hw);

	wr32(hw,0x10114,0x9f050206);
	wr32(hw,0x10194,0x9f050206);

	ngbe_flash_write_cab(hw, 0x188, 0, 0);
	ngbe_flash_write_cab(hw, 0x184, 0x60000000, 0);
	msleep(1000);

	mac_addr0_dword0_t = flash_read_dword(hw, MAC_ADDR0_WORD0_OFFSET_1G);
	mac_addr0_dword1_t = flash_read_dword(hw, MAC_ADDR0_WORD1_OFFSET_1G) & 0xffff;
	mac_addr1_dword0_t = flash_read_dword(hw, MAC_ADDR1_WORD0_OFFSET_1G);
	mac_addr1_dword1_t = flash_read_dword(hw, MAC_ADDR1_WORD1_OFFSET_1G) & 0xffff;
	mac_addr2_dword0_t = flash_read_dword(hw, MAC_ADDR2_WORD0_OFFSET_1G);
	mac_addr2_dword1_t = flash_read_dword(hw, MAC_ADDR2_WORD1_OFFSET_1G) & 0xffff;
	mac_addr3_dword0_t = flash_read_dword(hw, MAC_ADDR3_WORD0_OFFSET_1G);
	mac_addr3_dword1_t = flash_read_dword(hw, MAC_ADDR3_WORD1_OFFSET_1G) & 0xffff;

	serial_num_dword0_t = flash_read_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G);
	serial_num_dword1_t = flash_read_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G+4);
	serial_num_dword2_t = flash_read_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G+8);
	printk("Old: MAC Address0 is: 0x%04x%08x\n", mac_addr0_dword1_t, mac_addr0_dword0_t);
	printk("	 MAC Address1 is: 0x%04x%08x\n", mac_addr1_dword1_t, mac_addr1_dword0_t);
	printk("	 MAC Address2 is: 0x%04x%08x\n", mac_addr2_dword1_t, mac_addr2_dword0_t);
	printk("	 MAC Address3 is: 0x%04x%08x\n", mac_addr3_dword1_t, mac_addr3_dword0_t);

	for (k=0;k<(1024/4);k++) {
		num[k] = flash_read_dword(hw, 0xfe000 + k*4);
	}

	status = fmgr_usr_cmd_op(hw, 0x6);  // write enable
	status = fmgr_usr_cmd_op(hw, 0x98); // global protection un-lock
	msleep(1000); // 1 s
	
	// Note: for Spanish FLASH, first 8 sectors (4KB) in sector0 (64KB) need to use a special erase command (4K sector erase)
	if (flash_vendor == 1) {
		wr32(hw, SPI_CMD_CFG1_ADDR, 0x0103c720);
		for (i=0; i<8; i++) {
			flash_erase_sector(hw, i*128);
			msleep(20); // 20 ms
		}
		wr32(hw, SPI_CMD_CFG1_ADDR, 0x0103c7d8);
	}

	/* Winbond Flash, erase chip command is okay, but erase sector doestn't work*/
	sector_num = size / SPI_SECTOR_SIZE;
	if (flash_vendor == 2) {
		status = flash_erase_chip(hw);
		printk("Erase chip command, return status = %0d\n", status);
		msleep(1000); // 1 s
	} else {
		wr32(hw, SPI_CMD_CFG1_ADDR, 0x0103c720);
		for (i=0; i<sector_num; i++) {
			status = flash_erase_sector(hw, i * SPI_SECTOR_SIZE);
			printk("Erase sector[%2d] command, return status = %0d\n", i, status);
			msleep(50); // 50 ms
		}
		wr32(hw, SPI_CMD_CFG1_ADDR, 0x0103c7d8);
	}

	// Program Image file in dword
	for (i=0; i<size/4; i++) {
		read_data = data[4*i+3]<<24 | data[4*i+2]<<16 | data[4*i+1]<<8 | data[4*i];
		read_data = __le32_to_cpu(read_data);
		skip = ((i*4 == MAC_ADDR0_WORD0_OFFSET_1G) || (i*4 == MAC_ADDR0_WORD1_OFFSET_1G) ||
				(i*4 == MAC_ADDR1_WORD0_OFFSET_1G) || (i*4 == MAC_ADDR1_WORD1_OFFSET_1G) ||
				(i*4 == MAC_ADDR2_WORD0_OFFSET_1G) || (i*4 == MAC_ADDR2_WORD1_OFFSET_1G) ||
				(i*4 == MAC_ADDR3_WORD0_OFFSET_1G) || (i*4 == MAC_ADDR3_WORD1_OFFSET_1G) ||
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
		flash_write_dword(hw, MAC_ADDR2_WORD0_OFFSET_1G, mac_addr2_dword0_t);
		flash_write_dword(hw, MAC_ADDR2_WORD1_OFFSET_1G, (mac_addr2_dword1_t | 0x80000000));//lan2
		flash_write_dword(hw, MAC_ADDR3_WORD0_OFFSET_1G, mac_addr3_dword0_t);
		flash_write_dword(hw, MAC_ADDR3_WORD1_OFFSET_1G, (mac_addr3_dword1_t | 0x80000000));//lan3
		flash_write_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G, serial_num_dword0_t);
		flash_write_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G+4, serial_num_dword1_t);
		flash_write_dword(hw, PRODUCT_SERIAL_NUM_OFFSET_1G+8, serial_num_dword2_t);
		for(n=0;n<1024/4;n++){
			if(!(num[n] == 0xffffffff))
				flash_write_dword(hw, 0xfe000 + n*4, num[n]);
		}
	return 0;
}

/**
 * ngbe_set_rxpba - Initialize Rx packet buffer
 * @hw: pointer to hardware structure
 * @num_pb: number of packet buffers to allocate
 * @headroom: reserve n KB of headroom
 * @strategy: packet buffer allocation strategy
 **/
void ngbe_set_rxpba(struct ngbe_hw *hw, int num_pb, u32 headroom,
			     int strategy)
{
	u32 pbsize = hw->mac.rx_pb_size;
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
	case PBA_STRATEGY_EQUAL:
		rxpktsize = (pbsize / num_pb) << NGBE_RDB_PB_SZ_SHIFT;
		wr32(hw, NGBE_RDB_PB_SZ, rxpktsize);
		break;
	default:
		break;
	}

	/* Only support an equally distributed Tx packet buffer strategy. */
	txpktsize = NGBE_TDB_PB_SZ_MAX / num_pb;
	txpbthresh = (txpktsize / NGBE_KB_TO_B) - NGBE_TXPKT_SIZE_MAX;

	wr32(hw, NGBE_TDB_PB_SZ, txpktsize);
	wr32(hw, NGBE_TDM_PB_THRE, txpbthresh);
}


STATIC const u8 ngbe_emc_temp_data[4] = {
	NGBE_EMC_INTERNAL_DATA,
	NGBE_EMC_DIODE1_DATA,
	NGBE_EMC_DIODE2_DATA,
	NGBE_EMC_DIODE3_DATA
};
STATIC const u8 ngbe_emc_therm_limit[4] = {
	NGBE_EMC_INTERNAL_THERM_LIMIT,
	NGBE_EMC_DIODE1_THERM_LIMIT,
	NGBE_EMC_DIODE2_THERM_LIMIT,
	NGBE_EMC_DIODE3_THERM_LIMIT
};

/**
 *  ngbe_get_thermal_sensor_data - Gathers thermal sensor data
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
s32 ngbe_get_thermal_sensor_data(struct ngbe_hw *hw)
{
	s64 tsv;
	struct ngbe_thermal_sensor_data *data = &hw->mac.thermal_sensor_data;

	DEBUGFUNC("\n");

	/* Only support thermal sensors attached to physical port 0 */
	if (hw->bus.lan_id)
		return NGBE_NOT_IMPLEMENTED;

	tsv = (s64)(rd32(hw, NGBE_TS_ST) &
		NGBE_TS_ST_DATA_OUT_MASK);
	/* 216 < tsv < 876 */

	tsv = tsv < 876 ? tsv : 876 ;

	tsv = tsv - 216;
	
	tsv = tsv/4;

	tsv = tsv - 40;


	data->sensor.temp = (s16)tsv;

	return 0;
}

/**
 *  ngbe_init_thermal_sensor_thresh - Inits thermal sensor thresholds
 *  @hw: pointer to hardware structure
 *
 *  Inits the thermal sensor thresholds according to the NVM map
 *  and save off the threshold and location values into mac.thermal_sensor_data
 **/
s32 ngbe_init_thermal_sensor_thresh(struct ngbe_hw *hw)
{
	s32 status = 0;

	struct ngbe_thermal_sensor_data *data = &hw->mac.thermal_sensor_data;

	DEBUGFUNC("\n");

	memset(data, 0, sizeof(struct ngbe_thermal_sensor_data));

	/* Only support thermal sensors attached to SP physical port 0 */
	if (hw->bus.lan_id)
		return NGBE_NOT_IMPLEMENTED;


	wr32(hw, NGBE_TS_INT_EN, NGBE_TS_INT_EN_DALARM_INT_EN | NGBE_TS_INT_EN_ALARM_INT_EN);

	wr32(hw, NGBE_TS_EN, NGBE_TS_EN_ENA);


	data->sensor.alarm_thresh = 115;
	wr32(hw, NGBE_TS_ALARM_THRE, 0x344);/* magic num */
	data->sensor.dalarm_thresh = 110;
	wr32(hw, NGBE_TS_DALARM_THRE, 0x330);/* magic num */

	return status;
}

void ngbe_disable_rx(struct ngbe_hw *hw)
{
	u32 pfdtxgswc;
	u32 rxctrl;

	DEBUGFUNC("\n");

	rxctrl = rd32(hw, NGBE_RDB_PB_CTL);
	if (rxctrl & NGBE_RDB_PB_CTL_PBEN) {
		pfdtxgswc = rd32(hw, NGBE_PSR_CTL);
		if (pfdtxgswc & NGBE_PSR_CTL_SW_EN) {
			pfdtxgswc &= ~NGBE_PSR_CTL_SW_EN;
			wr32(hw, NGBE_PSR_CTL, pfdtxgswc);
			hw->mac.set_lben = true;
		} else {
			hw->mac.set_lben = false;
		}
		rxctrl &= ~NGBE_RDB_PB_CTL_PBEN;
		wr32(hw, NGBE_RDB_PB_CTL, rxctrl);

		/*OCP NCSI BMC need it*/		
		if (! ( ( (hw->subsystem_device_id & OEM_MASK) == OCP_CARD) || 
			((hw->subsystem_device_id & WOL_SUP_MASK) == WOL_SUP) ||
			((hw->subsystem_device_id & NCSI_SUP_MASK) == NCSI_SUP))) {
		/* disable mac receiver */
			wr32m(hw, NGBE_MAC_RX_CFG,
				NGBE_MAC_RX_CFG_RE, 0);
		}
	}

}


void ngbe_enable_rx(struct ngbe_hw *hw)
{
	u32 pfdtxgswc;

	DEBUGFUNC("\n");

	/* enable mac receiver */
	wr32m(hw, NGBE_MAC_RX_CFG,
		NGBE_MAC_RX_CFG_RE, NGBE_MAC_RX_CFG_RE);

	wr32m(hw, NGBE_RSEC_CTL,
		0x2, 0);

	wr32m(hw, NGBE_RDB_PB_CTL,
		NGBE_RDB_PB_CTL_PBEN, NGBE_RDB_PB_CTL_PBEN);

	if (hw->mac.set_lben) {
		pfdtxgswc = rd32(hw, NGBE_PSR_CTL);
		pfdtxgswc |= NGBE_PSR_CTL_SW_EN;
		wr32(hw, NGBE_PSR_CTL, pfdtxgswc);
		hw->mac.set_lben = false;
	}
}

/**
 * ngbe_mng_present - returns true when manangbeent capability is present
 * @hw: pointer to hardware structure
 */
bool ngbe_mng_present(struct ngbe_hw *hw)
{
	u32 fwsm;

	fwsm = rd32(hw, NGBE_MIS_ST);
	return fwsm & NGBE_MIS_ST_MNG_INIT_DN;
}

bool ngbe_check_mng_access(struct ngbe_hw *hw)
{

	if (!ngbe_mng_present(hw))
		return false;
	return true;
}

int ngbe_check_flash_load(struct ngbe_hw *hw, u32 check_bit)
{
	u32 i = 0;
	u32 reg = 0;
	int err = 0;
	/* if there's flash existing */
	if (!(rd32(hw, NGBE_SPI_STATUS) &
		NGBE_SPI_STATUS_FLASH_BYPASS)) {
		/* wait hw load flash done */
		for (i = 0; i < NGBE_MAX_FLASH_LOAD_POLL_TIME; i++) {
			reg = rd32(hw, NGBE_SPI_ILDR_STATUS);
			if (!(reg & check_bit)) {
				/* done */
				break;
			}
			msleep(200);
		}
		if (i == NGBE_MAX_FLASH_LOAD_POLL_TIME) {
			err = NGBE_ERR_FLASH_LOADING_FAILED;
			ERROR_REPORT1(NGBE_ERROR_POLLING,
				"HW Loading Flash failed: %d\n", err);
		}
	}
	return err;
}

/* The ngbe_ptype_lookup is used to convert from the 8-bit ptype in the
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
 * IF NOT ngbe_ptype_lookup[ptype].known
 * THEN
 *      Packet is unknown
 * ELSE IF ngbe_ptype_lookup[ptype].mac == NGBE_DEC_PTYPE_MAC_IP
 *      Use the rest of the fields to look at the tunnels, inner protocols, etc
 * ELSE
 *      Use the enum ngbe_l2_ptypes to decode the packet type
 * ENDIF
 */

/* macro to make the table lines short */
#define NGBE_PTT(ptype, mac, ip, etype, eip, proto, layer)\
	{       ptype, \
		1, \
		/* mac     */ NGBE_DEC_PTYPE_MAC_##mac, \
		/* ip      */ NGBE_DEC_PTYPE_IP_##ip, \
		/* etype   */ NGBE_DEC_PTYPE_ETYPE_##etype, \
		/* eip     */ NGBE_DEC_PTYPE_IP_##eip, \
		/* proto   */ NGBE_DEC_PTYPE_PROT_##proto, \
		/* layer   */ NGBE_DEC_PTYPE_LAYER_##layer }

#define NGBE_UKN(ptype) \
		{ ptype, 0, 0, 0, 0, 0, 0, 0 }

/* Lookup table mapping the HW PTYPE to the bit field for decoding */
/* for ((pt=0;pt<256;pt++)); do printf "macro(0x%02X),\n" $pt; done */
ngbe_dptype ngbe_ptype_lookup[256] = {
	NGBE_UKN(0x00),
	NGBE_UKN(0x01),
	NGBE_UKN(0x02),
	NGBE_UKN(0x03),
	NGBE_UKN(0x04),
	NGBE_UKN(0x05),
	NGBE_UKN(0x06),
	NGBE_UKN(0x07),
	NGBE_UKN(0x08),
	NGBE_UKN(0x09),
	NGBE_UKN(0x0A),
	NGBE_UKN(0x0B),
	NGBE_UKN(0x0C),
	NGBE_UKN(0x0D),
	NGBE_UKN(0x0E),
	NGBE_UKN(0x0F),

	/* L2: mac */
	NGBE_UKN(0x10),
	NGBE_PTT(0x11, L2, NONE, NONE, NONE, NONE, PAY2),
	NGBE_PTT(0x12, L2, NONE, NONE, NONE, TS,   PAY2),
	NGBE_PTT(0x13, L2, NONE, NONE, NONE, NONE, PAY2),
	NGBE_PTT(0x14, L2, NONE, NONE, NONE, NONE, PAY2),
	NGBE_PTT(0x15, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x16, L2, NONE, NONE, NONE, NONE, PAY2),
	NGBE_PTT(0x17, L2, NONE, NONE, NONE, NONE, NONE),

	/* L2: ethertype filter */
	NGBE_PTT(0x18, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x19, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1A, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1B, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1C, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1D, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1E, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1F, L2, NONE, NONE, NONE, NONE, NONE),

	/* L3: ip non-tunnel */
	NGBE_UKN(0x20),
	NGBE_PTT(0x21, IP, FGV4, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x22, IP, IPV4, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x23, IP, IPV4, NONE, NONE, UDP,  PAY4),
	NGBE_PTT(0x24, IP, IPV4, NONE, NONE, TCP,  PAY4),
	NGBE_PTT(0x25, IP, IPV4, NONE, NONE, SCTP, PAY4),
	NGBE_UKN(0x26),
	NGBE_UKN(0x27),
	NGBE_UKN(0x28),
	NGBE_PTT(0x29, IP, FGV6, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x2A, IP, IPV6, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x2B, IP, IPV6, NONE, NONE, UDP,  PAY3),
	NGBE_PTT(0x2C, IP, IPV6, NONE, NONE, TCP,  PAY4),
	NGBE_PTT(0x2D, IP, IPV6, NONE, NONE, SCTP, PAY4),
	NGBE_UKN(0x2E),
	NGBE_UKN(0x2F),

	/* L2: fcoe */
	NGBE_PTT(0x30, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x31, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x32, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x33, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x34, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_UKN(0x35),
	NGBE_UKN(0x36),
	NGBE_UKN(0x37),
	NGBE_PTT(0x38, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x39, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x3A, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x3B, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x3C, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_UKN(0x3D),
	NGBE_UKN(0x3E),
	NGBE_UKN(0x3F),

	NGBE_UKN(0x40),
	NGBE_UKN(0x41),
	NGBE_UKN(0x42),
	NGBE_UKN(0x43),
	NGBE_UKN(0x44),
	NGBE_UKN(0x45),
	NGBE_UKN(0x46),
	NGBE_UKN(0x47),
	NGBE_UKN(0x48),
	NGBE_UKN(0x49),
	NGBE_UKN(0x4A),
	NGBE_UKN(0x4B),
	NGBE_UKN(0x4C),
	NGBE_UKN(0x4D),
	NGBE_UKN(0x4E),
	NGBE_UKN(0x4F),
	NGBE_UKN(0x50),
	NGBE_UKN(0x51),
	NGBE_UKN(0x52),
	NGBE_UKN(0x53),
	NGBE_UKN(0x54),
	NGBE_UKN(0x55),
	NGBE_UKN(0x56),
	NGBE_UKN(0x57),
	NGBE_UKN(0x58),
	NGBE_UKN(0x59),
	NGBE_UKN(0x5A),
	NGBE_UKN(0x5B),
	NGBE_UKN(0x5C),
	NGBE_UKN(0x5D),
	NGBE_UKN(0x5E),
	NGBE_UKN(0x5F),
	NGBE_UKN(0x60),
	NGBE_UKN(0x61),
	NGBE_UKN(0x62),
	NGBE_UKN(0x63),
	NGBE_UKN(0x64),
	NGBE_UKN(0x65),
	NGBE_UKN(0x66),
	NGBE_UKN(0x67),
	NGBE_UKN(0x68),
	NGBE_UKN(0x69),
	NGBE_UKN(0x6A),
	NGBE_UKN(0x6B),
	NGBE_UKN(0x6C),
	NGBE_UKN(0x6D),
	NGBE_UKN(0x6E),
	NGBE_UKN(0x6F),
	NGBE_UKN(0x70),
	NGBE_UKN(0x71),
	NGBE_UKN(0x72),
	NGBE_UKN(0x73),
	NGBE_UKN(0x74),
	NGBE_UKN(0x75),
	NGBE_UKN(0x76),
	NGBE_UKN(0x77),
	NGBE_UKN(0x78),
	NGBE_UKN(0x79),
	NGBE_UKN(0x7A),
	NGBE_UKN(0x7B),
	NGBE_UKN(0x7C),
	NGBE_UKN(0x7D),
	NGBE_UKN(0x7E),
	NGBE_UKN(0x7F),

	/* IPv4 --> IPv4/IPv6 */
	NGBE_UKN(0x80),
	NGBE_PTT(0x81, IP, IPV4, IPIP, FGV4, NONE, PAY3),
	NGBE_PTT(0x82, IP, IPV4, IPIP, IPV4, NONE, PAY3),
	NGBE_PTT(0x83, IP, IPV4, IPIP, IPV4, UDP,  PAY4),
	NGBE_PTT(0x84, IP, IPV4, IPIP, IPV4, TCP,  PAY4),
	NGBE_PTT(0x85, IP, IPV4, IPIP, IPV4, SCTP, PAY4),
	NGBE_UKN(0x86),
	NGBE_UKN(0x87),
	NGBE_UKN(0x88),
	NGBE_PTT(0x89, IP, IPV4, IPIP, FGV6, NONE, PAY3),
	NGBE_PTT(0x8A, IP, IPV4, IPIP, IPV6, NONE, PAY3),
	NGBE_PTT(0x8B, IP, IPV4, IPIP, IPV6, UDP,  PAY4),
	NGBE_PTT(0x8C, IP, IPV4, IPIP, IPV6, TCP,  PAY4),
	NGBE_PTT(0x8D, IP, IPV4, IPIP, IPV6, SCTP, PAY4),
	NGBE_UKN(0x8E),
	NGBE_UKN(0x8F),

	/* IPv4 --> GRE/NAT --> NONE/IPv4/IPv6 */
	NGBE_PTT(0x90, IP, IPV4, IG, NONE, NONE, PAY3),
	NGBE_PTT(0x91, IP, IPV4, IG, FGV4, NONE, PAY3),
	NGBE_PTT(0x92, IP, IPV4, IG, IPV4, NONE, PAY3),
	NGBE_PTT(0x93, IP, IPV4, IG, IPV4, UDP,  PAY4),
	NGBE_PTT(0x94, IP, IPV4, IG, IPV4, TCP,  PAY4),
	NGBE_PTT(0x95, IP, IPV4, IG, IPV4, SCTP, PAY4),
	NGBE_UKN(0x96),
	NGBE_UKN(0x97),
	NGBE_UKN(0x98),
	NGBE_PTT(0x99, IP, IPV4, IG, FGV6, NONE, PAY3),
	NGBE_PTT(0x9A, IP, IPV4, IG, IPV6, NONE, PAY3),
	NGBE_PTT(0x9B, IP, IPV4, IG, IPV6, UDP,  PAY4),
	NGBE_PTT(0x9C, IP, IPV4, IG, IPV6, TCP,  PAY4),
	NGBE_PTT(0x9D, IP, IPV4, IG, IPV6, SCTP, PAY4),
	NGBE_UKN(0x9E),
	NGBE_UKN(0x9F),

	/* IPv4 --> GRE/NAT --> MAC --> NONE/IPv4/IPv6 */
	NGBE_PTT(0xA0, IP, IPV4, IGM, NONE, NONE, PAY3),
	NGBE_PTT(0xA1, IP, IPV4, IGM, FGV4, NONE, PAY3),
	NGBE_PTT(0xA2, IP, IPV4, IGM, IPV4, NONE, PAY3),
	NGBE_PTT(0xA3, IP, IPV4, IGM, IPV4, UDP,  PAY4),
	NGBE_PTT(0xA4, IP, IPV4, IGM, IPV4, TCP,  PAY4),
	NGBE_PTT(0xA5, IP, IPV4, IGM, IPV4, SCTP, PAY4),
	NGBE_UKN(0xA6),
	NGBE_UKN(0xA7),
	NGBE_UKN(0xA8),
	NGBE_PTT(0xA9, IP, IPV4, IGM, FGV6, NONE, PAY3),
	NGBE_PTT(0xAA, IP, IPV4, IGM, IPV6, NONE, PAY3),
	NGBE_PTT(0xAB, IP, IPV4, IGM, IPV6, UDP,  PAY4),
	NGBE_PTT(0xAC, IP, IPV4, IGM, IPV6, TCP,  PAY4),
	NGBE_PTT(0xAD, IP, IPV4, IGM, IPV6, SCTP, PAY4),
	NGBE_UKN(0xAE),
	NGBE_UKN(0xAF),

	/* IPv4 --> GRE/NAT --> MAC+VLAN --> NONE/IPv4/IPv6 */
	NGBE_PTT(0xB0, IP, IPV4, IGMV, NONE, NONE, PAY3),
	NGBE_PTT(0xB1, IP, IPV4, IGMV, FGV4, NONE, PAY3),
	NGBE_PTT(0xB2, IP, IPV4, IGMV, IPV4, NONE, PAY3),
	NGBE_PTT(0xB3, IP, IPV4, IGMV, IPV4, UDP,  PAY4),
	NGBE_PTT(0xB4, IP, IPV4, IGMV, IPV4, TCP,  PAY4),
	NGBE_PTT(0xB5, IP, IPV4, IGMV, IPV4, SCTP, PAY4),
	NGBE_UKN(0xB6),
	NGBE_UKN(0xB7),
	NGBE_UKN(0xB8),
	NGBE_PTT(0xB9, IP, IPV4, IGMV, FGV6, NONE, PAY3),
	NGBE_PTT(0xBA, IP, IPV4, IGMV, IPV6, NONE, PAY3),
	NGBE_PTT(0xBB, IP, IPV4, IGMV, IPV6, UDP,  PAY4),
	NGBE_PTT(0xBC, IP, IPV4, IGMV, IPV6, TCP,  PAY4),
	NGBE_PTT(0xBD, IP, IPV4, IGMV, IPV6, SCTP, PAY4),
	NGBE_UKN(0xBE),
	NGBE_UKN(0xBF),

	/* IPv6 --> IPv4/IPv6 */
	NGBE_UKN(0xC0),
	NGBE_PTT(0xC1, IP, IPV6, IPIP, FGV4, NONE, PAY3),
	NGBE_PTT(0xC2, IP, IPV6, IPIP, IPV4, NONE, PAY3),
	NGBE_PTT(0xC3, IP, IPV6, IPIP, IPV4, UDP,  PAY4),
	NGBE_PTT(0xC4, IP, IPV6, IPIP, IPV4, TCP,  PAY4),
	NGBE_PTT(0xC5, IP, IPV6, IPIP, IPV4, SCTP, PAY4),
	NGBE_UKN(0xC6),
	NGBE_UKN(0xC7),
	NGBE_UKN(0xC8),
	NGBE_PTT(0xC9, IP, IPV6, IPIP, FGV6, NONE, PAY3),
	NGBE_PTT(0xCA, IP, IPV6, IPIP, IPV6, NONE, PAY3),
	NGBE_PTT(0xCB, IP, IPV6, IPIP, IPV6, UDP,  PAY4),
	NGBE_PTT(0xCC, IP, IPV6, IPIP, IPV6, TCP,  PAY4),
	NGBE_PTT(0xCD, IP, IPV6, IPIP, IPV6, SCTP, PAY4),
	NGBE_UKN(0xCE),
	NGBE_UKN(0xCF),

	/* IPv6 --> GRE/NAT -> NONE/IPv4/IPv6 */
	NGBE_PTT(0xD0, IP, IPV6, IG,   NONE, NONE, PAY3),
	NGBE_PTT(0xD1, IP, IPV6, IG,   FGV4, NONE, PAY3),
	NGBE_PTT(0xD2, IP, IPV6, IG,   IPV4, NONE, PAY3),
	NGBE_PTT(0xD3, IP, IPV6, IG,   IPV4, UDP,  PAY4),
	NGBE_PTT(0xD4, IP, IPV6, IG,   IPV4, TCP,  PAY4),
	NGBE_PTT(0xD5, IP, IPV6, IG,   IPV4, SCTP, PAY4),
	NGBE_UKN(0xD6),
	NGBE_UKN(0xD7),
	NGBE_UKN(0xD8),
	NGBE_PTT(0xD9, IP, IPV6, IG,   FGV6, NONE, PAY3),
	NGBE_PTT(0xDA, IP, IPV6, IG,   IPV6, NONE, PAY3),
	NGBE_PTT(0xDB, IP, IPV6, IG,   IPV6, UDP,  PAY4),
	NGBE_PTT(0xDC, IP, IPV6, IG,   IPV6, TCP,  PAY4),
	NGBE_PTT(0xDD, IP, IPV6, IG,   IPV6, SCTP, PAY4),
	NGBE_UKN(0xDE),
	NGBE_UKN(0xDF),

	/* IPv6 --> GRE/NAT -> MAC -> NONE/IPv4/IPv6 */
	NGBE_PTT(0xE0, IP, IPV6, IGM,  NONE, NONE, PAY3),
	NGBE_PTT(0xE1, IP, IPV6, IGM,  FGV4, NONE, PAY3),
	NGBE_PTT(0xE2, IP, IPV6, IGM,  IPV4, NONE, PAY3),
	NGBE_PTT(0xE3, IP, IPV6, IGM,  IPV4, UDP,  PAY4),
	NGBE_PTT(0xE4, IP, IPV6, IGM,  IPV4, TCP,  PAY4),
	NGBE_PTT(0xE5, IP, IPV6, IGM,  IPV4, SCTP, PAY4),
	NGBE_UKN(0xE6),
	NGBE_UKN(0xE7),
	NGBE_UKN(0xE8),
	NGBE_PTT(0xE9, IP, IPV6, IGM,  FGV6, NONE, PAY3),
	NGBE_PTT(0xEA, IP, IPV6, IGM,  IPV6, NONE, PAY3),
	NGBE_PTT(0xEB, IP, IPV6, IGM,  IPV6, UDP,  PAY4),
	NGBE_PTT(0xEC, IP, IPV6, IGM,  IPV6, TCP,  PAY4),
	NGBE_PTT(0xED, IP, IPV6, IGM,  IPV6, SCTP, PAY4),
	NGBE_UKN(0xEE),
	NGBE_UKN(0xEF),

	/* IPv6 --> GRE/NAT -> MAC--> NONE/IPv */
	NGBE_PTT(0xF0, IP, IPV6, IGMV, NONE, NONE, PAY3),
	NGBE_PTT(0xF1, IP, IPV6, IGMV, FGV4, NONE, PAY3),
	NGBE_PTT(0xF2, IP, IPV6, IGMV, IPV4, NONE, PAY3),
	NGBE_PTT(0xF3, IP, IPV6, IGMV, IPV4, UDP,  PAY4),
	NGBE_PTT(0xF4, IP, IPV6, IGMV, IPV4, TCP,  PAY4),
	NGBE_PTT(0xF5, IP, IPV6, IGMV, IPV4, SCTP, PAY4),
	NGBE_UKN(0xF6),
	NGBE_UKN(0xF7),
	NGBE_UKN(0xF8),
	NGBE_PTT(0xF9, IP, IPV6, IGMV, FGV6, NONE, PAY3),
	NGBE_PTT(0xFA, IP, IPV6, IGMV, IPV6, NONE, PAY3),
	NGBE_PTT(0xFB, IP, IPV6, IGMV, IPV6, UDP,  PAY4),
	NGBE_PTT(0xFC, IP, IPV6, IGMV, IPV6, TCP,  PAY4),
	NGBE_PTT(0xFD, IP, IPV6, IGMV, IPV6, SCTP, PAY4),
	NGBE_UKN(0xFE),
	NGBE_UKN(0xFF),
};


void ngbe_init_mac_link_ops(struct ngbe_hw *hw)
{
	struct ngbe_mac_info *mac = &hw->mac;

	DEBUGFUNC("\n");

	mac->ops.setup_link = ngbe_setup_mac_link;
}

/**
 *  ngbe_init_ops - Inits func ptrs and MAC type
 *  @hw: pointer to hardware structure
 *
 *  Initialize the function pointers and assign the MAC type for emerald.
 *  Does not touch the hardware.
 **/

s32 ngbe_init_ops(struct ngbe_hw *hw)
{
	struct ngbe_mac_info *mac = &hw->mac;
	struct ngbe_phy_info *phy = &hw->phy;

	DEBUGFUNC("ngbe_init_ops");

	ngbe_init_phy_ops_common(hw);
	ngbe_init_ops_common(hw);

	if (hw->phy.type == ngbe_phy_m88e1512 ||
		hw->phy.type == ngbe_phy_m88e1512_sfi ||
		hw->phy.type == ngbe_phy_m88e1512_unknown) {
		phy->ops.read_reg_mdi = ngbe_phy_read_reg_mdi;
		phy->ops.write_reg_mdi = ngbe_phy_write_reg_mdi;
		phy->ops.setup_link = ngbe_phy_setup_link_m88e1512;
		phy->ops.reset = ngbe_phy_reset_m88e1512;
		phy->ops.check_event = ngbe_phy_check_event_m88e1512;
		phy->ops.get_adv_pause = ngbe_phy_get_advertised_pause_m88e1512;
		phy->ops.get_lp_adv_pause = ngbe_phy_get_lp_advertised_pause_m88e1512;
		phy->ops.set_adv_pause = ngbe_phy_set_pause_advertisement_m88e1512;

		mac->ops.check_link = ngbe_check_mac_link_mdi;
	} else if (hw->phy.type == ngbe_phy_yt8521s_sfi) {
		phy->ops.read_reg_mdi = ngbe_phy_read_reg_mdi;
		phy->ops.write_reg_mdi = ngbe_phy_write_reg_mdi;
		phy->ops.setup_link = ngbe_phy_setup_link_yt8521s;
		phy->ops.reset = ngbe_phy_reset_yt8521s;
		phy->ops.check_event = ngbe_phy_check_event_yt8521s;
		phy->ops.get_adv_pause = ngbe_phy_get_advertised_pause_yt8521s;
		phy->ops.get_lp_adv_pause = ngbe_phy_get_lp_advertised_pause_yt8521s;
		phy->ops.set_adv_pause = ngbe_phy_set_pause_advertisement_yt8521s;

		mac->ops.check_link = ngbe_check_mac_link_yt8521s;
	}

	return NGBE_OK;
}

/**
 *  ngbe_get_link_capabilities - Determines link capabilities
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @autoneg: true when autoneg or autotry is enabled
 *
 *  Determines the link capabilities by reading the AUTOC register.
 **/
s32 ngbe_get_link_capabilities(struct ngbe_hw *hw,
				      u32 *speed,
				      bool *autoneg)
{
	s32 status = 0;

	DEBUGFUNC("\n");

	if (hw->device_id == NGBE_DEV_ID_EM_TEST ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860A2 ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860A2S ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860A4 ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860A4S ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860AL2 ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860AL2S ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860AL4 ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860AL4S ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860AL_W ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860A1 ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860A1L ||
		hw->device_id == 0x10c ||
		hw->device_id == NGBE_DEV_ID_EM_WX1860NCSI ) {
		*speed = NGBE_LINK_SPEED_1GB_FULL |
			NGBE_LINK_SPEED_100_FULL |
			NGBE_LINK_SPEED_10_FULL;
		*autoneg = false;
		hw->phy.link_mode = NGBE_PHYSICAL_LAYER_1000BASE_T |
				NGBE_PHYSICAL_LAYER_100BASE_TX;
	}

	if ((hw->subsystem_device_id & OEM_MASK) == RGMII_FPGA) {
		*speed = NGBE_LINK_SPEED_1GB_FULL;
		hw->phy.link_mode = NGBE_PHYSICAL_LAYER_1000BASE_T;
		*autoneg = false;
	}

	return status;
}

/**
 *  ngbe_get_copper_link_capabilities - Determines link capabilities
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @autoneg: boolean auto-negotiation value
 *
 *  Determines the supported link capabilities by reading the PHY auto
 *  negotiation register.
**/
s32 ngbe_get_copper_link_capabilities(struct ngbe_hw *hw,
						 u32 *speed,
						 bool *autoneg)
{
	s32 status = 0;
	u16 value = 0;
	DEBUGFUNC("\n");

	*speed = 0;

	if (hw->mac.autoneg)
		*autoneg = true;
	else
		*autoneg = false;

	if (status == 0) {
		*speed = NGBE_LINK_SPEED_10_FULL |
				 NGBE_LINK_SPEED_100_FULL |
				 NGBE_LINK_SPEED_1GB_FULL;
	}

	if (((hw->subsystem_device_id & OEM_MASK) == RGMII_FPGA) ||
		(hw->phy.type == ngbe_phy_m88e1512_sfi)) {
		*speed = NGBE_LINK_SPEED_1GB_FULL;
		hw->phy.link_mode = NGBE_PHYSICAL_LAYER_1000BASE_T;
		*autoneg = false;
	}

	if (hw->phy.type == ngbe_phy_yt8521s_sfi) {
		ngbe_phy_read_reg_ext_yt8521s(hw, 0xA001, 0, &value);
		if ((value & 7) == 1) {
			*speed = NGBE_LINK_SPEED_1GB_FULL;
			hw->phy.link_mode = NGBE_PHYSICAL_LAYER_1000BASE_T;			
		}			
	}

	return status;
}

/**
 *  ngbe_get_media_type - Get media type
 *  @hw: pointer to hardware structure
 *
 *  Returns the media type (fiber, copper, backplane)
 **/
enum ngbe_media_type ngbe_get_media_type(struct ngbe_hw *hw)
{
	enum ngbe_media_type media_type;

	DEBUGFUNC("\n");

	ERROR_REPORT1(NGBE_ERROR_ARGUMENT,
		"ngbe_get_media_type: hw->device_id = %u/n", hw->device_id);

	media_type = ngbe_media_type_copper;

	return media_type;
}

/**
 *  ngbe_stop_mac_link_on_d3 - Disables link on D3
 *  @hw: pointer to hardware structure
 *
 *  Disables link during D3 power down sequence.
 *
 **/
void ngbe_stop_mac_link_on_d3(struct ngbe_hw *hw)
{

	UNREFERENCED_PARAMETER(hw);
	return;
}

/**
 *  ngbe_setup_mac_link - Set MAC link speed
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: true when waiting for completion is needed
 *
 *  Set the link speed in the AUTOC register and restarts link.
 **/
s32 ngbe_setup_mac_link(struct ngbe_hw *hw,
			       u32 speed,
			       bool autoneg_wait_to_complete)
{
	bool autoneg = false;
	s32 status = 0;
	u32 link_capabilities = NGBE_LINK_SPEED_UNKNOWN;
	u32 link_speed = NGBE_LINK_SPEED_UNKNOWN;
	u32 lan_speed = 0;
	bool link_up = false;

	UNREFERENCED_PARAMETER(autoneg_wait_to_complete);
	DEBUGFUNC("\n");
	
	if ( ! (((hw->subsystem_device_id & OEM_MASK) == OCP_CARD) ||
		((hw->subsystem_device_id & WOL_SUP_MASK) == WOL_SUP) ||
		((hw->subsystem_device_id & NCSI_SUP_MASK) == NCSI_SUP))){
		/* Check to see if speed passed in is supported. */
		status = TCALL(hw, mac.ops.get_link_capabilities,
				       &link_capabilities, &autoneg);
		if (status)
			goto out;

		speed &= link_capabilities;

		if (speed == NGBE_LINK_SPEED_UNKNOWN) {
			status = NGBE_ERR_LINK_SETUP;
			goto out;
		}
	}
	
	status = TCALL(hw, mac.ops.check_link,
				&link_speed, &link_up, false);
	if (status != 0)
		goto out;
	if ((link_speed == speed) && link_up) {
		switch (link_speed) {
		case NGBE_LINK_SPEED_100_FULL:
			lan_speed = 1;
			break;
		case NGBE_LINK_SPEED_1GB_FULL:
			lan_speed = 2;
			break;
		case NGBE_LINK_SPEED_10_FULL:
			lan_speed = 0;
			break;
		default:
			break;
		}
		wr32m(hw, NGBE_CFG_LAN_SPEED,
			0x3, lan_speed);
	}

out:
	return status;
}


/**
 *  ngbe_setup_copper_link - Set the PHY autoneg advertised field
 *  @hw: pointer to hardware structure
 *  @speed: new link speed
 *  @autoneg_wait_to_complete: true if waiting is needed to complete
 *
 *  Restarts link on PHY and MAC based on settings passed in.
 **/
STATIC s32 ngbe_setup_copper_link(struct ngbe_hw *hw,
					 u32 speed,
					 bool need_restart_AN)
{
	s32 status = 0;
	struct ngbe_adapter *adapter = hw->back;

	DEBUGFUNC("\n");

	/* Setup the PHY according to input speed */
	if (!((hw->subsystem_device_id & OEM_MASK) == RGMII_FPGA)) {
		status = TCALL(hw, phy.ops.setup_link, speed,
							need_restart_AN);
	}
	adapter->flags |= NGBE_FLAG_NEED_ANC_CHECK;

	return status;
}

int ngbe_reset_misc(struct ngbe_hw *hw)
{
	int i;

	/* receive packets that size > 2048 */
	wr32m(hw, NGBE_MAC_RX_CFG,
		NGBE_MAC_RX_CFG_JE, NGBE_MAC_RX_CFG_JE);

	/* clear counters on read */
	wr32m(hw, NGBE_MMC_CONTROL,
		NGBE_MMC_CONTROL_RSTONRD, NGBE_MMC_CONTROL_RSTONRD);

	wr32m(hw, NGBE_MAC_RX_FLOW_CTRL,
		NGBE_MAC_RX_FLOW_CTRL_RFE, NGBE_MAC_RX_FLOW_CTRL_RFE);

	wr32(hw, NGBE_MAC_PKT_FLT,
		NGBE_MAC_PKT_FLT_PR);

	wr32m(hw, NGBE_MIS_RST_ST,
		NGBE_MIS_RST_ST_RST_INIT, 0x1E00);

	/* errata 4: initialize mng flex tbl and wakeup flex tbl*/
	wr32(hw, NGBE_PSR_MNG_FLEX_SEL, 0);
	for (i = 0; i < 16; i++) {
		wr32(hw, NGBE_PSR_MNG_FLEX_DW_L(i), 0);
		wr32(hw, NGBE_PSR_MNG_FLEX_DW_H(i), 0);
		wr32(hw, NGBE_PSR_MNG_FLEX_MSK(i), 0);
	}
	wr32(hw, NGBE_PSR_LAN_FLEX_SEL, 0);
	for (i = 0; i < 16; i++) {
		wr32(hw, NGBE_PSR_LAN_FLEX_DW_L(i), 0);
		wr32(hw, NGBE_PSR_LAN_FLEX_DW_H(i), 0);
		wr32(hw, NGBE_PSR_LAN_FLEX_MSK(i), 0);
	}

	/* set pause frame dst mac addr */
	wr32(hw, NGBE_RDB_PFCMACDAL, 0xC2000001);
	wr32(hw, NGBE_RDB_PFCMACDAH, 0x0180);

	wr32(hw, NGBE_MDIO_CLAUSE_SELECT, 0xF);
	
	if (hw->gpio_ctl == 1) {
		/* gpio0 is used to power on/off control*/
		wr32(hw, NGBE_GPIO_DDR, 0x1);
		wr32(hw, NGBE_GPIO_DR, NGBE_GPIO_DR_0);
	}
	
	ngbe_init_thermal_sensor_thresh(hw);

	return 0;
}

/**
 *  ngbe_reset_hw - Perform hardware reset
 *  @hw: pointer to hardware structure
 *
 *  Resets the hardware by resetting the transmit and receive units, masks
 *  and clears all interrupts, perform a PHY reset, and perform a link (MAC)
 *  reset.
 **/
s32 ngbe_reset_hw(struct ngbe_hw *hw)
{
	s32 status;
	u32 reset = 0;
	u32 i;
	struct ngbe_mac_info *mac = &hw->mac;

	u32 sr_pcs_ctl = 0, sr_pma_mmd_ctl1 = 0, sr_an_mmd_ctl = 0;
	u32 sr_an_mmd_adv_reg2 = 0;
	u32 vr_xs_or_pcs_mmd_digi_ctl1 = 0, curr_vr_xs_or_pcs_mmd_digi_ctl1 = 0;
	u32 curr_sr_pcs_ctl = 0, curr_sr_pma_mmd_ctl1 = 0;
	u32 curr_sr_an_mmd_ctl = 0, curr_sr_an_mmd_adv_reg2 = 0;

	u32 reset_status = 0;
	u32 rst_delay = 0;

	struct ngbe_adapter *adapter = NULL;

	DEBUGFUNC("\n");

	/* Call adapter stop to disable tx/rx and clear interrupts */
	status = TCALL(hw, mac.ops.stop_adapter);
	if (status != 0)
		goto reset_hw_out;

	/* Identify PHY and related function pointers */
	if (!((hw->subsystem_device_id & OEM_MASK) == RGMII_FPGA)) {
		status = TCALL(hw, phy.ops.init);
		if (status)
			goto reset_hw_out;
	}

	if (ngbe_get_media_type(hw) == ngbe_media_type_copper) {
		mac->ops.setup_link = ngbe_setup_copper_link;
		mac->ops.get_link_capabilities =
				 ngbe_get_copper_link_capabilities;
	}

	/*
	 * Issue global reset to the MAC.  Needs to be SW reset if link is up.
	 * If link reset is used when link is up, it might reset the PHY when
	 * mng is using it.  If link is down or the flag to force full link
	 * reset is set, then perform link reset.
	 */
	if (hw->force_full_reset) {
		rst_delay = (rd32(hw, NGBE_MIS_RST_ST) &
			NGBE_MIS_RST_ST_RST_INIT) >>
			NGBE_MIS_RST_ST_RST_INI_SHIFT;
		if (hw->reset_type == NGBE_SW_RESET) {
			for (i = 0; i < rst_delay + 20; i++) {
				reset_status =
					rd32(hw, NGBE_MIS_RST_ST);
				if (!(reset_status &
				    NGBE_MIS_RST_ST_DEV_RST_ST_MASK))
					break;
				msleep(100);
			}

			if (reset_status & NGBE_MIS_RST_ST_DEV_RST_ST_MASK) {
				status = NGBE_ERR_RESET_FAILED;
				DEBUGOUT("software reset polling failed to "
					"complete.\n");
				goto reset_hw_out;
			}
			status = ngbe_check_flash_load(hw,
						NGBE_SPI_ILDR_STATUS_SW_RESET);
			if (status != 0)
				goto reset_hw_out;

		} else if (hw->reset_type == NGBE_GLOBAL_RESET) {
#ifndef _WIN32
			adapter = (struct ngbe_adapter *)hw->back;
			msleep(100 * rst_delay + 2000);
			pci_restore_state(adapter->pdev);
			pci_save_state(adapter->pdev);
			pci_wake_from_d3(adapter->pdev, false);

#endif /*_WIN32*/
		}
	} else {

		if (hw->bus.lan_id == 0) {
			reset = NGBE_MIS_RST_LAN0_RST;
		} else if (hw->bus.lan_id == 1) {
			reset = NGBE_MIS_RST_LAN1_RST;
		} else if (hw->bus.lan_id == 2) {
			reset = NGBE_MIS_RST_LAN2_RST;
		} else if (hw->bus.lan_id == 3) {
			reset = NGBE_MIS_RST_LAN3_RST;
		}

		wr32(hw, NGBE_MIS_RST,
			reset | rd32(hw, NGBE_MIS_RST));
		NGBE_WRITE_FLUSH(hw);

		msleep(15);
	}

	status = ngbe_reset_misc(hw);
	if (status != 0)
		goto reset_hw_out;


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

	/* Store the permanent mac address */
	TCALL(hw, mac.ops.get_mac_addr, hw->mac.perm_addr);

	/*
	 * Store MAC address from RAR0, clear receive address registers, and
	 * clear the multicast table.  Also reset num_rar_entries to 128,
	 * since we modify this value when programming the SAN MAC address.
	 */
	hw->mac.num_rar_entries = NGBE_SP_RAR_ENTRIES;
	TCALL(hw, mac.ops.init_rx_addrs);
	
	pci_set_master(((struct ngbe_adapter *)hw->back)->pdev);


reset_hw_out:
	return status;
}

/*
 * These defines allow us to quickly generate all of the necessary instructions
 * in the function below by simply calling out NGBE_COMPUTE_SIG_HASH_ITERATION
 * for values 0 through 15
 */
#define NGBE_ATR_COMMON_HASH_KEY \
		(NGBE_ATR_BUCKET_HASH_KEY & NGBE_ATR_SIGNATURE_HASH_KEY)
#define NGBE_COMPUTE_SIG_HASH_ITERATION(_n) \
do { \
	u32 n = (_n); \
	if (NGBE_ATR_COMMON_HASH_KEY & (0x01 << n)) \
		common_hash ^= lo_hash_dword >> n; \
	else if (NGBE_ATR_BUCKET_HASH_KEY & (0x01 << n)) \
		bucket_hash ^= lo_hash_dword >> n; \
	else if (NGBE_ATR_SIGNATURE_HASH_KEY & (0x01 << n)) \
		sig_hash ^= lo_hash_dword << (16 - n); \
	if (NGBE_ATR_COMMON_HASH_KEY & (0x01 << (n + 16))) \
		common_hash ^= hi_hash_dword >> n; \
	else if (NGBE_ATR_BUCKET_HASH_KEY & (0x01 << (n + 16))) \
		bucket_hash ^= hi_hash_dword >> n; \
	else if (NGBE_ATR_SIGNATURE_HASH_KEY & (0x01 << (n + 16))) \
		sig_hash ^= hi_hash_dword << (16 - n); \
} while (0)


#define NGBE_COMPUTE_BKT_HASH_ITERATION(_n) \
do { \
	u32 n = (_n); \
	if (NGBE_ATR_BUCKET_HASH_KEY & (0x01 << n)) \
		bucket_hash ^= lo_hash_dword >> n; \
	if (NGBE_ATR_BUCKET_HASH_KEY & (0x01 << (n + 16))) \
		bucket_hash ^= hi_hash_dword >> n; \
} while (0)

/*
 * These two macros are meant to address the fact that we have registers
 * that are either all or in part big-endian.  As a result on big-endian
 * systems we will end up byte swapping the value to little-endian before
 * it is byte swapped again and written to the hardware in the original
 * big-endian format.
 */
#define NGBE_STORE_AS_BE32(_value) \
	(((u32)(_value) >> 24) | (((u32)(_value) & 0x00FF0000) >> 8) | \
	 (((u32)(_value) & 0x0000FF00) << 8) | ((u32)(_value) << 24))

#define NGBE_WRITE_REG_BE32(a, reg, value) \
	wr32((a), (reg), NGBE_STORE_AS_BE32(NGBE_NTOHL(value)))

#define NGBE_STORE_AS_BE16(_value) \
	NGBE_NTOHS(((u16)(_value) >> 8) | ((u16)(_value) << 8))

/**
 *  ngbe_start_hw - Prepare hardware for Tx/Rx
 *  @hw: pointer to hardware structure
 *
 *  Starts the hardware using the generic start_hw function
 *  and the generation start_hw function.
 *  Then performs revision-specific operations, if any.
 **/
s32 ngbe_start_hw(struct ngbe_hw *hw)
{
	int ret_val = 0;

	DEBUGFUNC("\n");

	/* Set the media type */
	hw->phy.media_type = TCALL(hw, mac.ops.get_media_type);

	/* PHY ops initialization must be done in reset_hw() */

	/* Clear the VLAN filter table */
	TCALL(hw, mac.ops.clear_vfta);

	/* Clear statistics registers */
	TCALL(hw, mac.ops.clear_hw_cntrs);

	NGBE_WRITE_FLUSH(hw);

	/* Setup flow control */
	ret_val = TCALL(hw, mac.ops.setup_fc);

	/* Clear adapter stopped flag */
	hw->adapter_stopped = false;

	/* We need to run link autotry after the driver loads */
	hw->mac.autotry_restart = true;

	return ret_val;
}

/**
 *  ngbe_enable_rx_dma - Enable the Rx DMA unit on emerald
 *  @hw: pointer to hardware structure
 *  @regval: register value to write to RXCTRL
 *
 *  Enables the Rx DMA unit for emerald
 **/
s32 ngbe_enable_rx_dma(struct ngbe_hw *hw, u32 regval)
{

	DEBUGFUNC("\n");

	/*
	 * Workaround for emerald silicon errata when enabling the Rx datapath.
	 * If traffic is incoming before we enable the Rx unit, it could hang
	 * the Rx DMA unit.  Therefore, make sure the security engine is
	 * completely disabled prior to enabling the Rx unit.
	 */

	TCALL(hw, mac.ops.disable_sec_rx_path);

	if (regval & NGBE_RDB_PB_CTL_PBEN)
		TCALL(hw, mac.ops.enable_rx);
	else
		TCALL(hw, mac.ops.disable_rx);

	TCALL(hw, mac.ops.enable_sec_rx_path);

	return 0;
}

/**
 *  ngbe_init_flash_params - Initialize flash params
 *  @hw: pointer to hardware structure
 *
 *  Initializes the EEPROM parameters ngbe_eeprom_info within the
 *  ngbe_hw struct in order to set up EEPROM access.
 **/
s32 ngbe_init_flash_params(struct ngbe_hw *hw)
{
	struct ngbe_flash_info *flash = &hw->flash;
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
 *  ngbe_read_flash_buffer - Read FLASH dword(s) using
 *  fastest available method
 *
 *  @hw: pointer to hardware structure
 *  @offset: offset of  dword in EEPROM to read
 *  @dwords: number of dwords
 *  @data: dword(s) read from the EEPROM
 *
 *  Retrieves 32 bit dword(s) read from EEPROM
 **/
s32 ngbe_read_flash_buffer(struct ngbe_hw *hw, u32 offset,
					  u32 dwords, u32 *data)
{
	s32 status = 0;
	u32 i;

	DEBUGFUNC("\n");

	TCALL(hw, eeprom.ops.init_params);

	if (!dwords || offset + dwords >= hw->flash.dword_size) {
		status = NGBE_ERR_INVALID_ARGUMENT;
		ERROR_REPORT1(NGBE_ERROR_ARGUMENT, "Invalid FLASH arguments");
		return status;
	}

	for (i = 0; i < dwords; i++) {
		wr32(hw, NGBE_SPI_DATA, data[i]);
		wr32(hw, NGBE_SPI_CMD,
			NGBE_SPI_CMD_ADDR(offset + i) |
			NGBE_SPI_CMD_CMD(0x0));

		status = po32m(hw, NGBE_SPI_STATUS,
			NGBE_SPI_STATUS_OPDONE, NGBE_SPI_STATUS_OPDONE,
			NGBE_SPI_TIMEOUT, 0);
		if (status) {
			DEBUGOUT("FLASH read timed out\n");
			break;
		}
	}

	return status;
}

/**
 *  ngbe_write_flash_buffer - Write FLASH dword(s) using
 *  fastest available method
 *
 *  @hw: pointer to hardware structure
 *  @offset: offset of  dword in EEPROM to write
 *  @dwords: number of dwords
 *  @data: dword(s) write from to EEPROM
 *
 **/
s32 ngbe_write_flash_buffer(struct ngbe_hw *hw, u32 offset,
					  u32 dwords, u32 *data)
{
	s32 status = 0;
	u32 i;

	DEBUGFUNC("\n");

	TCALL(hw, eeprom.ops.init_params);

	if (!dwords || offset + dwords >= hw->flash.dword_size) {
		status = NGBE_ERR_INVALID_ARGUMENT;
		ERROR_REPORT1(NGBE_ERROR_ARGUMENT, "Invalid FLASH arguments");
		return status;
	}

	for (i = 0; i < dwords; i++) {
		wr32(hw, NGBE_SPI_CMD,
			NGBE_SPI_CMD_ADDR(offset + i) |
			NGBE_SPI_CMD_CMD(0x1));

		status = po32m(hw, NGBE_SPI_STATUS,
			NGBE_SPI_STATUS_OPDONE, NGBE_SPI_STATUS_OPDONE,
			NGBE_SPI_TIMEOUT, 0);
		if (status != 0) {
			DEBUGOUT("FLASH write timed out\n");
			break;
		}
		data[i] = rd32(hw, NGBE_SPI_DATA);
	}

	return status;
}

/**
 *  ngbe_init_eeprom_params - Initialize EEPROM params
 *  @hw: pointer to hardware structure
 *
 *  Initializes the EEPROM parameters ngbe_eeprom_info within the
 *  ngbe_hw struct in order to set up EEPROM access.
 **/
s32 ngbe_init_eeprom_params(struct ngbe_hw *hw)
{
	struct ngbe_eeprom_info *eeprom = &hw->eeprom;
	u16 eeprom_size;
	s32 status = 0;

	DEBUGFUNC("\n");

	if (eeprom->type == ngbe_eeprom_uninitialized) {
		eeprom->semaphore_delay = 10;
		eeprom->type = ngbe_eeprom_none;

		if (!(rd32(hw, NGBE_SPI_STATUS) &
			NGBE_SPI_STATUS_FLASH_BYPASS)) {
			eeprom->type = ngbe_flash;
			eeprom_size = 4096;
			eeprom->word_size = eeprom_size >> 1;

			DEBUGOUT2("Eeprom params: type = %d, size = %d\n",
				  eeprom->type, eeprom->word_size);
		}
	}

	eeprom->sw_region_offset = 0x80;

	return status;
}

/**
 *  ngbe_read_ee_hostif - Read EEPROM word using a host interface cmd
 *  assuming that the semaphore is already obtained.
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to read
 *  @data: word read from the EEPROM
 *
 *  Reads a 16 bit word from the EEPROM using the hostif.
 **/
s32 ngbe_read_ee_hostif_data(struct ngbe_hw *hw, u16 offset,
				   u16 *data)
{
	s32 status;
	struct ngbe_hic_read_shadow_ram buffer;

	DEBUGFUNC("\n");
	buffer.hdr.req.cmd = FW_READ_SHADOW_RAM_CMD;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = FW_READ_SHADOW_RAM_LEN;
	buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;

	/* convert offset from words to bytes */
	buffer.address = NGBE_CPU_TO_BE32(offset * 2);
	/* one word */
	buffer.length = NGBE_CPU_TO_BE16(sizeof(u16));

	status = ngbe_host_interface_command(hw, (u32 *)&buffer,
					      sizeof(buffer),
					      NGBE_HI_COMMAND_TIMEOUT, false);

	if (status)
		return status;
	if (ngbe_check_mng_access(hw))
		*data = (u16)rd32a(hw, NGBE_MNG_MBOX,
							FW_NVM_DATA_OFFSET);
	else {
		status = NGBE_ERR_MNG_ACCESS_FAILED;
		return status;
	}


	return 0;
}

s32 ngbe_eepromcheck_cap(struct ngbe_hw *hw, u16 offset,
								u32 *data)
{
	int tmp;
	s32 status;
	struct ngbe_hic_read_shadow_ram buffer;

	DEBUGFUNC("\n");
	buffer.hdr.req.cmd = FW_EEPROM_CHECK_STATUS;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;

	/* convert offset from words to bytes */
	buffer.address = 0;
	/* one word */
	buffer.length = 0;


	status = ngbe_host_interface_command(hw, (u32 *)&buffer,
											sizeof(buffer),
											NGBE_HI_COMMAND_TIMEOUT, false);

	if (status)
		return status;
	if (ngbe_check_mng_access(hw)){
		tmp = (u32)rd32a(hw, NGBE_MNG_MBOX, 1);
		if(tmp == NGBE_CHECKSUM_CAP_ST_PASS )
		{
			status = 0;
		}else
			status = NGBE_ERR_EEPROM_CHECKSUM;
	}else {
			status = NGBE_ERR_MNG_ACCESS_FAILED;
			return status;
		
	}

	return status;
}

s32 ngbe_phy_led_oem_chk(struct ngbe_hw *hw, u32 *data)
{
	int tmp;
	s32 status;
	struct ngbe_hic_read_shadow_ram buffer;

	DEBUGFUNC("\n");
	buffer.hdr.req.cmd = FW_PHY_LED_CONF;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;

	/* convert offset from words to bytes */
	buffer.address = 0;
	/* one word */
	buffer.length = 0;


	status = ngbe_host_interface_command(hw, (u32 *)&buffer,
						sizeof(buffer),
						NGBE_HI_COMMAND_TIMEOUT, false);

	if (status)
		return status;

	if (ngbe_check_mng_access(hw)){
		tmp = (u32)rd32a(hw, NGBE_MNG_MBOX, 1);
		if (tmp == NGBE_CHECKSUM_CAP_ST_PASS)
		{
			tmp = (u32)rd32a(hw, NGBE_MNG_MBOX, 2);
			*data = tmp;
			status = 0;
		} else if (tmp == NGBE_CHECKSUM_CAP_ST_FAIL) {
			*data = tmp;
			status = -1;
		} else {			
			status = NGBE_ERR_EEPROM_CHECKSUM;
		}
	}else {
			status = NGBE_ERR_MNG_ACCESS_FAILED;
			return status;
	}

	return status;
}

s32 ngbe_phy_signal_set(struct ngbe_hw *hw)
{
	s32 status;
	struct ngbe_hic_read_shadow_ram buffer;

	DEBUGFUNC("\n");
	buffer.hdr.req.cmd = FW_PHY_SIGNAL;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = 0;
	buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;

	/* convert offset from words to bytes */
	buffer.address = 0;
	/* one word */
	buffer.length = 0;

	status = ngbe_host_interface_command(hw, (u32 *)&buffer,
											sizeof(buffer),
											NGBE_HI_COMMAND_TIMEOUT, false);

	if (status)
		return status;

	return status;
}


/**
 *  ngbe_read_ee_hostif - Read EEPROM word using a host interface cmd
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to read
 *  @data: word read from the EEPROM
 *
 *  Reads a 16 bit word from the EEPROM using the hostif.
 **/
s32 ngbe_read_ee_hostif(struct ngbe_hw *hw, u16 offset,
			      u16 *data)
{
	s32 status = 0;

	DEBUGFUNC("\n");

	if (TCALL(hw, mac.ops.acquire_swfw_sync,
		NGBE_MNG_SWFW_SYNC_SW_FLASH) == 0) {
		status = ngbe_read_ee_hostif_data(hw, offset, data);
		TCALL(hw, mac.ops.release_swfw_sync,
			NGBE_MNG_SWFW_SYNC_SW_FLASH);
	} else {
		status = NGBE_ERR_SWFW_SYNC;
	}

	return status;
}

/**
 *  ngbe_read_ee_hostif_buffer- Read EEPROM word(s) using hostif
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to read
 *  @words: number of words
 *  @data: word(s) read from the EEPROM
 *
 *  Reads a 16 bit word(s) from the EEPROM using the hostif.
 **/
s32 ngbe_read_ee_hostif_buffer(struct ngbe_hw *hw,
				     u16 offset, u16 words, u16 *data)
{
	struct ngbe_hic_read_shadow_ram buffer;
	u32 current_word = 0;
	u16 words_to_read;
	s32 status;
	u32 i;
	u32 value = 0;

	DEBUGFUNC("\n");

	/* Take semaphore for the entire operation. */
	status = TCALL(hw, mac.ops.acquire_swfw_sync,
			NGBE_MNG_SWFW_SYNC_SW_FLASH);
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
		buffer.address = NGBE_CPU_TO_BE32((offset + current_word) * 2);
		buffer.length = NGBE_CPU_TO_BE16(words_to_read * 2);

		status = ngbe_host_interface_command(hw, (u32 *)&buffer,
						      sizeof(buffer),
						      NGBE_HI_COMMAND_TIMEOUT,
						      false);

		if (status) {
			DEBUGOUT("Host interface command failed\n");
			goto out;
		}

		for (i = 0; i < words_to_read; i++) {
			u32 reg = NGBE_MNG_MBOX + (FW_NVM_DATA_OFFSET << 2) +
				  2 * i;
			if (ngbe_check_mng_access(hw))
				value = rd32(hw, reg);
			else {
				status = NGBE_ERR_MNG_ACCESS_FAILED;
				goto out;
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
		NGBE_MNG_SWFW_SYNC_SW_FLASH);
	return status;
}


/**
 *  ngbe_read_ee_hostif - Read EEPROM word using a host interface cmd
 *  assuming that the semaphore is already obtained.
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to read
 *  @data: word read from the EEPROM
 *
 *  Reads a 32 bit word from the EEPROM using the hostif.
 **/
s32 ngbe_read_ee_hostif_data32(struct ngbe_hw *hw, u16 offset,
					u32 *data)
{
	s32 status;
	struct ngbe_hic_read_shadow_ram buffer;

	DEBUGFUNC("\n");
	buffer.hdr.req.cmd = FW_READ_SHADOW_RAM_CMD;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = FW_READ_SHADOW_RAM_LEN;
	buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;
	
	/* convert offset from words to bytes */
	buffer.address = NGBE_CPU_TO_BE32(offset * 2);
	/* one word */
	buffer.length = NGBE_CPU_TO_BE16(sizeof(u32));
	
	status = ngbe_host_interface_command(hw, (u32 *)&buffer,
						sizeof(buffer),
						NGBE_HI_COMMAND_TIMEOUT, false);
					 
	if (status)
		return status;
	if (ngbe_check_mng_access(hw))
		*data = (u32)rd32a(hw, NGBE_MNG_MBOX, FW_NVM_DATA_OFFSET);
	else {
		status = NGBE_ERR_MNG_ACCESS_FAILED;
		return status;
	}
	return 0;
}


/**
 *  ngbe_read_ee_hostif - Read EEPROM word using a host interface cmd
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to read
 *  @data: word read from the EEPROM
 *
 *  Reads a 32 bit word from the EEPROM using the hostif.
 **/
s32 ngbe_read_ee_hostif32(struct ngbe_hw *hw, u16 offset,
					   u32 *data)
{
	s32 status = 0;

	DEBUGFUNC("\n");

	if (TCALL(hw, mac.ops.acquire_swfw_sync,NGBE_MNG_SWFW_SYNC_SW_FLASH) == 0) {
		status = ngbe_read_ee_hostif_data32(hw, offset, data);
		TCALL(hw, mac.ops.release_swfw_sync,
					NGBE_MNG_SWFW_SYNC_SW_FLASH);
	} else {
		status = NGBE_ERR_SWFW_SYNC;
	}

	return status;
}


/**
 *  ngbe_write_ee_hostif - Write EEPROM word using hostif
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to write
 *  @data: word write to the EEPROM
 *
 *  Write a 16 bit word to the EEPROM using the hostif.
 **/
s32 ngbe_write_ee_hostif_data(struct ngbe_hw *hw, u16 offset,
				    u16 data)
{
	s32 status;
	struct ngbe_hic_write_shadow_ram buffer;

	DEBUGFUNC("\n");

	buffer.hdr.req.cmd = FW_WRITE_SHADOW_RAM_CMD;
	buffer.hdr.req.buf_lenh = 0;
	buffer.hdr.req.buf_lenl = FW_WRITE_SHADOW_RAM_LEN;
	buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;

	/* one word */
	buffer.length = NGBE_CPU_TO_BE16(sizeof(u16));
	buffer.data = data;
	buffer.address = NGBE_CPU_TO_BE32(offset * 2);

	status = ngbe_host_interface_command(hw, (u32 *)&buffer,
					      sizeof(buffer),
					      NGBE_HI_COMMAND_TIMEOUT, false);

	return status;
}

/**
 *  ngbe_write_ee_hostif - Write EEPROM word using hostif
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to write
 *  @data: word write to the EEPROM
 *
 *  Write a 16 bit word to the EEPROM using the hostif.
 **/
s32 ngbe_write_ee_hostif(struct ngbe_hw *hw, u16 offset,
			       u16 data)
{
	s32 status = 0;

	DEBUGFUNC("\n");

	if (TCALL(hw, mac.ops.acquire_swfw_sync,
		NGBE_MNG_SWFW_SYNC_SW_FLASH) == 0) {
		status = ngbe_write_ee_hostif_data(hw, offset, data);
		TCALL(hw, mac.ops.release_swfw_sync,
			NGBE_MNG_SWFW_SYNC_SW_FLASH);
	} else {
		DEBUGOUT("write ee hostif failed to get semaphore");
		status = NGBE_ERR_SWFW_SYNC;
	}

	return status;
}

   /**
*  ngbe_write_ee_hostif - Write EEPROM word using hostif
*  @hw: pointer to hardware structure
*  @offset: offset of  word in the EEPROM to write
*  @data: word write to the EEPROM
*
*  Write a 16 bit word to the EEPROM using the hostif.
**/
  s32 ngbe_write_ee_hostif_data32(struct ngbe_hw *hw, u16 offset,
								   u32 data)
   {
   s32 status;
   struct ngbe_hic_write_shadow_ram buffer;
   
   DEBUGFUNC("\n");
   
   buffer.hdr.req.cmd = FW_WRITE_SHADOW_RAM_CMD;
   buffer.hdr.req.buf_lenh = 0;
   buffer.hdr.req.buf_lenl = FW_WRITE_SHADOW_RAM_LEN;
   buffer.hdr.req.checksum = FW_DEFAULT_CHECKSUM;

   /* one word */
   buffer.length = NGBE_CPU_TO_BE16(sizeof(u32));
   buffer.data = data;
   buffer.address = NGBE_CPU_TO_BE32(offset * 2);
   
   status = ngbe_host_interface_command(hw, (u32 *)&buffer,
										 sizeof(buffer),
										 NGBE_HI_COMMAND_TIMEOUT, false);

   return status;
   }
   
/***  ngbe_write_ee_hostif - Write EEPROM word using hostif
*  @hw: pointer to hardware structure
*  @offset: offset of  word in the EEPROM to write
*  @data: word write to the EEPROM
*
*  Write a 16 bit word to the EEPROM using the hostif.
**/
  s32 ngbe_write_ee_hostif32(struct ngbe_hw *hw, u16 offset,
							  u32 data)
{
   s32 status = 0;

   DEBUGFUNC("\n");

   if (TCALL(hw, mac.ops.acquire_swfw_sync,
	   NGBE_MNG_SWFW_SYNC_SW_FLASH) == 0) {
	   status = ngbe_write_ee_hostif_data32(hw, offset, data);
	   TCALL(hw, mac.ops.release_swfw_sync,
				   NGBE_MNG_SWFW_SYNC_SW_FLASH);	
   } else {
	   DEBUGOUT("write ee hostif failed to get semaphore");
			   status = NGBE_ERR_SWFW_SYNC;
   }

	return status;
}


/**
 *  ngbe_write_ee_hostif_buffer - Write EEPROM word(s) using hostif
 *  @hw: pointer to hardware structure
 *  @offset: offset of  word in the EEPROM to write
 *  @words: number of words
 *  @data: word(s) write to the EEPROM
 *
 *  Write a 16 bit word(s) to the EEPROM using the hostif.
 **/
s32 ngbe_write_ee_hostif_buffer(struct ngbe_hw *hw,
				      u16 offset, u16 words, u16 *data)
{
	s32 status = 0;
	u16 i = 0;

	DEBUGFUNC("\n");

	/* Take semaphore for the entire operation. */
	status = TCALL(hw, mac.ops.acquire_swfw_sync,
		       NGBE_MNG_SWFW_SYNC_SW_FLASH);
	if (status != 0) {
		DEBUGOUT("EEPROM write buffer - semaphore failed\n");
		return status;
	}

	for (i = 0; i < words; i++) {
		status = ngbe_write_ee_hostif_data(hw, offset + i,
							 data[i]);

		if (status != 0) {
			DEBUGOUT("Eeprom buffered write failed\n");
			break;
		}
	}

	TCALL(hw, mac.ops.release_swfw_sync, NGBE_MNG_SWFW_SYNC_SW_FLASH);
	return status;
}



/**
 *  ngbe_calc_eeprom_checksum - Calculates and returns the checksum
 *  @hw: pointer to hardware structure
 *
 *  Returns a negative error code on error, or the 16-bit checksum
 **/
s32 ngbe_calc_eeprom_checksum(struct ngbe_hw *hw)
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
		eeprom_ptrs = (u16 *)vmalloc(NGBE_EEPROM_LAST_WORD *
				sizeof(u16));
		if (!eeprom_ptrs)
			return NGBE_ERR_NO_SPACE;
		/* Read pointer area */
		status = ngbe_read_ee_hostif_buffer(hw, 0,
						     NGBE_EEPROM_LAST_WORD,
						     eeprom_ptrs);
		if (status) {
			DEBUGOUT("Failed to read EEPROM image\n");
			return status;
		}
		local_buffer = eeprom_ptrs;
	} else {
		if (buffer_size < NGBE_EEPROM_LAST_WORD)
			return NGBE_ERR_PARAM;
		local_buffer = buffer;
	}

	for (i = 0; i < NGBE_EEPROM_LAST_WORD; i++)
		if (i != hw->eeprom.sw_region_offset + NGBE_EEPROM_CHECKSUM)
			checksum += local_buffer[i];

	checksum = (u16)NGBE_EEPROM_SUM - checksum;
	if (eeprom_ptrs)
		vfree(eeprom_ptrs);

	return (s32)checksum;
}

/**
 * ngbe_update_eeprom_checksum - Updates the EEPROM checksum and flash
 * @hw: pointer to hardware structure
 *
 * After writing EEPROM to shadow RAM using EEWR register, software calculates
 * checksum and updates the EEPROM and instructs the hardware to update
 * the flash.
 **/
s32 ngbe_update_eeprom_checksum(struct ngbe_hw *hw)
{
	s32 status;
	u16 checksum = 0;

	DEBUGFUNC("\n");

	/* Read the first word from the EEPROM. If this times out or fails, do
	 * not continue or we could be in for a very long wait while every
	 * EEPROM read fails
	 */
	status = ngbe_read_ee_hostif(hw, 0, &checksum);
	if (status) {
		DEBUGOUT("EEPROM read failed\n");
		return status;
	}

	status = ngbe_calc_eeprom_checksum(hw);
	if (status < 0)
		return status;

	checksum = (u16)(status & 0xffff);

	status = ngbe_write_ee_hostif(hw, NGBE_EEPROM_CHECKSUM,
					    checksum);
	if (status)
		return status;

	return status;
}

/**
 *  ngbe_validate_eeprom_checksum - Validate EEPROM checksum
 *  @hw: pointer to hardware structure
 *  @checksum_val: calculated checksum
 *
 *  Performs checksum calculation and validates the EEPROM checksum.  If the
 *  caller does not need checksum_val, the value can be NULL.
 **/
s32 ngbe_validate_eeprom_checksum(struct ngbe_hw *hw,
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

	status = ngbe_read_ee_hostif(hw, hw->eeprom.sw_region_offset +
					NGBE_EEPROM_CHECKSUM,
					&read_checksum);
	if (status)
		return status;

	/* Verify read checksum from EEPROM is the same as
	 * calculated checksum
	 */
	if (read_checksum != checksum) {
		status = NGBE_ERR_EEPROM_CHECKSUM;
		ERROR_REPORT1(NGBE_ERROR_INVALID_STATE,
			     "Invalid EEPROM checksum\n");
	}

	/* If the user cares, return the calculated checksum */
	if (checksum_val)
		*checksum_val = checksum;

	return status;
}


/**
 *  ngbe_check_mac_link - Determine link and speed status
 *  @hw: pointer to hardware structure
 *  @speed: pointer to link speed
 *  @link_up: true when link is up
 *  @link_up_wait_to_complete: bool used to wait for link up or not
 *
 *  Reads the links register to determine if link is up and the current speed
 **/
s32 ngbe_check_mac_link(struct ngbe_hw *hw,
						u32 *speed,
						bool *link_up,
						bool link_up_wait_to_complete)
{
	u32 i;
	u16 value = 0;
	s32 status = 0;
	u16 speed_sta = 0;

	DEBUGFUNC("ngbe_check_mac_link");

	if ((hw->subsystem_device_id & OEM_MASK) == RGMII_FPGA) {
		*link_up = true;
		*speed = NGBE_LINK_SPEED_1GB_FULL;
		return status;
	}

	if (link_up_wait_to_complete) {
		for (i = 0; i < NGBE_LINK_UP_TIME; i++) {
			status = TCALL(hw, phy.ops.read_reg, 0x1A, 0xA43, &value);
			if (!status && (value & 0x4)) {
				*link_up = true;
				break;
			} else {
				*link_up = false;
			}
			msleep(100);
		}
	} else {
#ifdef POLL_LINK_STATUS
		//status = TCALL(hw, phy.ops.read_reg, 0xA, 0x0, &value);
		//if (!status && !(value & 0x2000)) {
		//	*link_up = false;	
		//} else {
#endif
		status = TCALL(hw, phy.ops.read_reg, 0x1A, 0xA43, &value);
		if (!status && (value & 0x4)) {
			*link_up = true;
		} else {
			*link_up = false;
		}
#ifdef POLL_LINK_STATUS
		//}
#endif
	}

	speed_sta = value & 0x38;
	if (*link_up) {
		if ( speed_sta == 0x28) {
			*speed = NGBE_LINK_SPEED_1GB_FULL;
		} else if ( speed_sta == 0x18) {
			*speed = NGBE_LINK_SPEED_100_FULL;
		} else if ( speed_sta == 0x8) {
			*speed = NGBE_LINK_SPEED_10_FULL;
		}
	} else
		*speed = NGBE_LINK_SPEED_UNKNOWN;

	if (*speed == NGBE_LINK_SPEED_1GB_FULL) {
		status = TCALL(hw, phy.ops.read_reg, 0xA, 0x0, &value);
		if (!status && !(value & 0x2000)) {
			*link_up = false;
		}
	}
	return status;
}

s32 ngbe_check_mac_link_mdi(struct ngbe_hw *hw,
							u32 *speed,
							bool *link_up,
							bool link_up_wait_to_complete)
{
	u32 i;
	u16 value = 0;
	s32 status = 0;
	u16 speed_sta = 0;

	DEBUGFUNC("ngbe_check_mac_link_mdi");

	
	if (hw->phy.type == ngbe_phy_m88e1512)
		/* select page 0 */
		status = TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 0);
	else
		/* select page 1 */
		status = TCALL(hw, phy.ops.write_reg_mdi, 22, 0, 1);
	status = TCALL(hw, phy.ops.read_reg_mdi, 17, 0, &value);
	if (link_up_wait_to_complete) {
		for (i = 0; i < NGBE_LINK_UP_TIME; i++) {
			status = TCALL(hw, phy.ops.read_reg_mdi, 17, 0, &value);
			if (value & 0x400) {
				*link_up = true;
				break;
			} else {
				*link_up = false;
			}
			msleep(100);
		}
	} else {
			status = TCALL(hw, phy.ops.read_reg_mdi, 17, 0, &value);
			if (value & 0x400) {
				*link_up = true;
			} else {
				*link_up = false;
			}
	}

	speed_sta = value & 0xC000;
	if (*link_up) {
			if ( speed_sta == 0x8000) {
				*speed = NGBE_LINK_SPEED_1GB_FULL;
			} else if ( speed_sta == 0x4000) {
				*speed = NGBE_LINK_SPEED_100_FULL;
			} else if ( speed_sta == 0x0000) {
				*speed = NGBE_LINK_SPEED_10_FULL;
			}
	} else
		*speed = NGBE_LINK_SPEED_UNKNOWN;

	return status;
}

s32 ngbe_check_mac_link_yt8521s(struct ngbe_hw *hw,
							u32 *speed,
							bool *link_up,
							bool link_up_wait_to_complete)
{
	u32 i;
	u16 value = 0;
	s32 status = 0;
	u16 speed_sta = 0;

	DEBUGFUNC("ngbe_check_mac_link_yt");

	if (link_up_wait_to_complete) {
		for (i = 0; i < NGBE_LINK_UP_TIME; i++) {
			status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x11, 0, &value);
			if (value & 0x400) {
				*link_up = true;
				break; 
			} else {
				*link_up = false;
			}
			msleep(100);
		}
	} else {
			status = ngbe_phy_read_reg_sds_mii_yt8521s(hw, 0x11, 0, &value);
			if (value & 0x400) {
				*link_up = true;
			} else {
				*link_up = false;
				ngbe_phy_read_reg_mdi(hw, 0x11, 0, &value);
				if (value & 0x400){
					*link_up = true;
					//printk("yt8521: diankou link is up\n");
				}else{
					*link_up = false;
					//printk("yt8521: diankou link is down\n");
				}
			}
	}

	speed_sta = value & 0xC000;
	if (*link_up) {
			if ( speed_sta == 0x8000) {
				*speed = NGBE_LINK_SPEED_1GB_FULL;
			} else if ( speed_sta == 0x4000) {
				*speed = NGBE_LINK_SPEED_100_FULL;
			} else if ( speed_sta == 0x0000) {
				*speed = NGBE_LINK_SPEED_10_FULL;
			}
	} else
		*speed = NGBE_LINK_SPEED_UNKNOWN;
	return status;
}

s32 ngbe_check_mac_link_zte(struct ngbe_hw *hw,
									u32 *speed,
									bool *link_up,
									bool link_up_wait_to_complete)
{
	u32 i;
	u16 value = 0;
	s32 status = 0;
	u16 speed_sta = 0;
	
	DEBUGFUNC("ngbe_check_mac_link_zte");

	/*PHY status register*/                        
	status = TCALL(hw, phy.ops.read_reg_mdi, 0x1a, 0, &value);


	if (link_up_wait_to_complete) {
		for (i = 0; i < NGBE_LINK_UP_TIME; i++) {
			status = TCALL(hw, phy.ops.read_reg_mdi, 0x1a, 0, &value);
			/*bit 6->0x0040*/
			if (value & 0x40) {
				*link_up = true;
				break;
			} else {
				*link_up = false;
			}
				msleep(100);
			}
	} else {
		status = TCALL(hw, phy.ops.read_reg_mdi, 0x1a, 0, &value);
		if (value & 0x40) {
			*link_up = true;
		} else {
			*link_up = false;
		}
	}

	speed_sta = value & 0xC000;
	if (*link_up) {
		if ( speed_sta == 0x0200) {
			*speed = NGBE_LINK_SPEED_1GB_FULL;
		} else if ( speed_sta == 0x0100) {
			*speed = NGBE_LINK_SPEED_100_FULL;
		} else if ( speed_sta == 0x0000) {
			*speed = NGBE_LINK_SPEED_10_FULL;
		}
	} else{
			*speed = NGBE_LINK_SPEED_UNKNOWN;
	}
	return status;
}

/**
 *  ngbe_setup_eee - Enable/disable EEE support
 *  @hw: pointer to the HW structure
 *  @enable_eee: boolean flag to enable EEE
 *
 *  Enable/disable EEE based on enable_eee flag.
 *  Auto-negotiation must be started after BASE-T EEE bits in PHY register 7.3C
 *  are modified.
 *
 **/
s32 ngbe_setup_eee(struct ngbe_hw *hw, bool enable_eee)
{
	/* fix eee */
	UNREFERENCED_PARAMETER(hw);
	UNREFERENCED_PARAMETER(enable_eee);
	DEBUGFUNC("\n");

	return 0;
}

s32 ngbe_init_ops_common(struct ngbe_hw *hw)
{
	struct ngbe_mac_info *mac = &hw->mac;
	struct ngbe_eeprom_info *eeprom = &hw->eeprom;
	struct ngbe_flash_info *flash = &hw->flash;

	/* MAC */
	mac->ops.init_hw = ngbe_init_hw;
	mac->ops.clear_hw_cntrs = ngbe_clear_hw_cntrs;
	mac->ops.get_mac_addr = ngbe_get_mac_addr;
	mac->ops.stop_adapter = ngbe_stop_adapter;
	mac->ops.get_bus_info = ngbe_get_bus_info;
	mac->ops.set_lan_id = ngbe_set_lan_id_multi_port_pcie;
	mac->ops.acquire_swfw_sync = ngbe_acquire_swfw_sync;
	mac->ops.release_swfw_sync = ngbe_release_swfw_sync;
	mac->ops.reset_hw = ngbe_reset_hw;
	mac->ops.get_media_type = ngbe_get_media_type;
	mac->ops.disable_sec_rx_path = ngbe_disable_sec_rx_path;
	mac->ops.enable_sec_rx_path = ngbe_enable_sec_rx_path;
	mac->ops.enable_rx_dma = ngbe_enable_rx_dma;
	mac->ops.start_hw = ngbe_start_hw;
	mac->ops.get_device_caps = ngbe_get_device_caps;
	mac->ops.setup_eee = ngbe_setup_eee;

	/* LEDs */
	mac->ops.led_on = ngbe_led_on;
	mac->ops.led_off = ngbe_led_off;

	/* RAR, Multicast, VLAN */
	mac->ops.set_rar = ngbe_set_rar;
	mac->ops.clear_rar = ngbe_clear_rar;
	mac->ops.init_rx_addrs = ngbe_init_rx_addrs;
	mac->ops.update_uc_addr_list = ngbe_update_uc_addr_list;
	mac->ops.update_mc_addr_list = ngbe_update_mc_addr_list;
	mac->ops.enable_mc = ngbe_enable_mc;
	mac->ops.disable_mc = ngbe_disable_mc;
	mac->ops.enable_rx = ngbe_enable_rx;
	mac->ops.disable_rx = ngbe_disable_rx;
	mac->ops.set_vmdq_san_mac = ngbe_set_vmdq_san_mac;
	mac->ops.insert_mac_addr = ngbe_insert_mac_addr;
	mac->rar_highwater = 1;
	mac->ops.set_vfta = ngbe_set_vfta;
	mac->ops.set_vlvf = ngbe_set_vlvf;
	mac->ops.clear_vfta = ngbe_clear_vfta;
	mac->ops.init_uta_tables = ngbe_init_uta_tables;
	mac->ops.set_mac_anti_spoofing = ngbe_set_mac_anti_spoofing;
	mac->ops.set_vlan_anti_spoofing = ngbe_set_vlan_anti_spoofing;
	mac->ops.set_ethertype_anti_spoofing =
				ngbe_set_ethertype_anti_spoofing;

	/* Flow Control */
	mac->ops.fc_enable = ngbe_fc_enable;
	mac->ops.setup_fc = ngbe_setup_fc;

	/* Link */
	mac->ops.get_link_capabilities = ngbe_get_link_capabilities;
	mac->ops.check_link = ngbe_check_mac_link;
	mac->ops.setup_rxpba = ngbe_set_rxpba;

	mac->mcft_size          = NGBE_SP_MC_TBL_SIZE;
	mac->vft_size           = NGBE_SP_VFT_TBL_SIZE;
	mac->num_rar_entries    = NGBE_SP_RAR_ENTRIES;
	mac->rx_pb_size         = NGBE_SP_RX_PB_SIZE;
	mac->max_rx_queues      = NGBE_SP_MAX_RX_QUEUES;
	mac->max_tx_queues      = NGBE_SP_MAX_TX_QUEUES;
	mac->max_msix_vectors   = ngbe_get_pcie_msix_count(hw);

	mac->arc_subsystem_valid = (rd32(hw, NGBE_MIS_ST) &
				   NGBE_MIS_ST_MNG_INIT_DN) ? true : false;

	hw->mbx.ops.init_params = ngbe_init_mbx_params_pf;

	/* EEPROM */
	eeprom->ops.init_params = ngbe_init_eeprom_params;
	eeprom->ops.calc_checksum = ngbe_calc_eeprom_checksum;
	eeprom->ops.read = ngbe_read_ee_hostif;
	eeprom->ops.read_buffer = ngbe_read_ee_hostif_buffer;
	eeprom->ops.read32 = ngbe_read_ee_hostif32;
	eeprom->ops.write = ngbe_write_ee_hostif;
	eeprom->ops.write_buffer = ngbe_write_ee_hostif_buffer;
	eeprom->ops.update_checksum = ngbe_update_eeprom_checksum;
	eeprom->ops.validate_checksum = ngbe_validate_eeprom_checksum;
	eeprom->ops.eeprom_chksum_cap_st = ngbe_eepromcheck_cap;
	eeprom->ops.phy_led_oem_chk = ngbe_phy_led_oem_chk;
	eeprom->ops.phy_signal_set = ngbe_phy_signal_set;

	/* FLASH */
	flash->ops.init_params = ngbe_init_flash_params;
	flash->ops.read_buffer = ngbe_read_flash_buffer;
	flash->ops.write_buffer = ngbe_write_flash_buffer;

	/* Manageability interface */
	mac->ops.set_fw_drv_ver = ngbe_set_fw_drv_ver;

	mac->ops.get_thermal_sensor_data =
					ngbe_get_thermal_sensor_data;
	mac->ops.init_thermal_sensor_thresh =
					ngbe_init_thermal_sensor_thresh;

	return NGBE_OK;
}




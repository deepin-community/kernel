// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2008 - 2023 Xel Technology. */
#include "xlnid_type.h"
#include "xlnid_westlake.h"
#include "xlnid_api.h"
#include "xlnid_common.h"
#include "xlnid_debug.h"

#define XLNID_WESTLAKE_MAX_TX_QUEUES 8
#define XLNID_WESTLAKE_MAX_RX_QUEUES 8
#define XLNID_WESTLAKE_RAR_ENTRIES   32
#define XLNID_WESTLAKE_MC_TBL_SIZE   128
#define XLNID_WESTLAKE_VFT_TBL_SIZE  128
#define XLNID_WESTLAKE_RX_PB_SIZE     512

extern xel_pci_info_t xel_pci_info[16];
extern xel_pci_info_t *xlnid_find_pci_info(unsigned char bus_number,
		unsigned int devfn);

STATIC s32 xlnid_setup_copper_link_westlake(struct xlnid_hw *hw,
		xlnid_link_speed speed, bool autoneg_wait_to_complete);
STATIC s32 xlnid_verify_fw_version_westlake(struct xlnid_hw *hw);
STATIC s32 xlnid_read_eeprom_westlake(struct xlnid_hw *hw, u16 offset,
										u8 *data);
STATIC s32 xlnid_read_eeprom_buffer_westlake(struct xlnid_hw *hw, u16 offset,
		u16 words, u8 *data);
STATIC s32 xlnid_write_eeprom_buffer_westlake(struct xlnid_hw *hw, u16 offset,
		u16 bytes, u8 *data);
STATIC s32 xlnid_read_phy_reg_westlake(struct xlnid_hw *hw, u32 reg_addr,
										u32 device_type, u16 *phy_data);
STATIC s32 xlnid_write_phy_reg_westlake(struct xlnid_hw *hw, u32 reg_addr,
										u32 device_type, u16 phy_data);
STATIC s32 xlnid_setup_fc_westlake(struct xlnid_hw *hw);
STATIC s32 xlnid_fc_enable_westlake(struct xlnid_hw *hw);

void xlnid_init_mac_link_ops_westlake(struct xlnid_hw *hw)
{
	struct xlnid_mac_info *mac = &hw->mac;

	DEBUGFUNC("xlnid_init_mac_link_ops_westlake");

	/*
		enable the laser control functions for SFP+ fiber
		and MNG not enabled
	*/
	if ((mac->ops.get_media_type(hw) == xlnid_media_type_fiber)
			&& !xlnid_mng_enabled(hw)) {
		mac->ops.disable_tx_laser = xlnid_disable_tx_laser_multispeed_fiber;
		mac->ops.enable_tx_laser = xlnid_enable_tx_laser_multispeed_fiber;
		mac->ops.flap_tx_laser = xlnid_flap_tx_laser_multispeed_fiber;

	}

	else {
		mac->ops.disable_tx_laser = NULL;
		mac->ops.enable_tx_laser = NULL;
		mac->ops.flap_tx_laser = NULL;
	}
}

/**
		xlnid_init_phy_ops_westlake - PHY/SFP specific init
		@hw: pointer to hardware structure

		Initialize any function pointers that were not able to be
		set during init_shared_code because the PHY/SFP type was
		not known.  Perform the SFP init if necessary.

	**/
s32 xlnid_init_phy_ops_westlake(struct xlnid_hw *hw)
{
	struct xlnid_mac_info *mac = &hw->mac;
	struct xlnid_phy_info *phy = &hw->phy;
	s32 ret_val = XLNID_SUCCESS;

	DEBUGFUNC("xlnid_init_phy_ops_westlake");

	/* Identify the PHY or SFP module */
	ret_val = phy->ops.identify(hw);

	if (ret_val == XLNID_ERR_SFP_NOT_SUPPORTED) {
		goto init_phy_ops_out;
	}

	/* Setup function pointers based on detected SFP module and speeds */
	xlnid_init_mac_link_ops_westlake(hw);

	if (hw->phy.sfp_type != xlnid_sfp_type_unknown) {
		hw->phy.ops.reset = NULL;
	}

	/* If copper media, overwrite with copper function pointers */
	if (mac->ops.get_media_type(hw) == xlnid_media_type_copper) {
		mac->ops.setup_link = xlnid_setup_copper_link_westlake;
	}

init_phy_ops_out:
	return ret_val;
}

s32 xlnid_set_phy_power_westlake(struct xlnid_hw *hw, bool on)
{
	u16 bmcr = 0;

	xlnid_read_phy_reg(hw, MII_BMCR, 0, &bmcr);

	if (on) {
		bmcr &= ~BMCR_PDOWN;
		xlnid_write_phy_reg(hw, MII_BMCR, 0, bmcr);
	}

	else {
		bmcr |= BMCR_PDOWN;
		xlnid_write_phy_reg(hw, MII_BMCR, 0, bmcr);
		msec_delay(1);
	}

	return XLNID_SUCCESS;
}

s32 xlnid_setup_sfp_modules_westlake(struct xlnid_hw *hw)
{
	return XLNID_SUCCESS;
}

/**
		prot_autoc_read_westlake - Hides MAC differences needed for AUTOC read
		@hw: pointer to hardware structure
		@locked: Return the if we locked for this read.
		@reg_val: Value we read from AUTOC

		For this part (westlake) we need to wrap read-modify-writes with a possible
		FW/SW lock.  It is assumed this lock will be freed with the next
		prot_autoc_write_westlake().
*/
s32 prot_autoc_read_westlake(struct xlnid_hw *hw, bool *locked, u32 *reg_val)
{
	return XLNID_SUCCESS;
}

/**
		prot_autoc_write_westlake - Hides MAC differences needed for AUTOC write
		@hw: pointer to hardware structure
		@autoc: value to write to AUTOC
		@locked: bool to indicate whether the SW/FW lock was already taken by
				previous proc_autoc_read_westlake.

		This part (westlake) may need to hold the SW/FW lock around all writes to
		AUTOC. Likewise after a write we need to do a pipeline reset.
*/
s32 prot_autoc_write_westlake(struct xlnid_hw *hw, u32 autoc, bool locked)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_get_device_caps_westlake - Get additional device capabilities
		@hw: pointer to hardware structure
		@device_caps: the EEPROM word with the extra device capabilities

		This function will read the EEPROM location for the device capabilities,
		and return the word through device_caps.
	**/
s32 xlnid_get_device_caps_westlake(struct xlnid_hw *hw, u16 *device_caps)
{
	DEBUGFUNC("xlnid_get_device_caps_westlake");

	*device_caps = XLNID_DEVICE_CAPS_NO_CROSSTALK_WR;

	return XLNID_SUCCESS;
}

/**
		xlnid_init_ops_westlake - Inits func ptrs and MAC type
		@hw: pointer to hardware structure

		Initialize the function pointers and assign the MAC type for WESTLAKE.
		Does not touch the hardware.
	**/

s32 xlnid_init_ops_westlake(struct xlnid_hw *hw)
{
	struct xlnid_mac_info *mac = &hw->mac;
	struct xlnid_phy_info *phy = &hw->phy;
	struct xlnid_eeprom_info *eeprom = &hw->eeprom;
	s32 ret_val;

	DEBUGFUNC("xlnid_init_ops_westlake");

	ret_val = xlnid_init_ops_generic(hw);

	/* PHY */
	phy->ops.identify = xlnid_identify_phy_westlake;
	phy->ops.init = xlnid_init_phy_ops_westlake;
	phy->ops.set_phy_power = xlnid_set_phy_power_westlake;
	phy->ops.read_reg = xlnid_read_phy_reg_westlake;
	phy->ops.write_reg = xlnid_write_phy_reg_westlake;

	/* MAC */
	mac->ops.reset_hw = xlnid_reset_hw_westlake;
	mac->ops.get_media_type = xlnid_get_media_type_westlake;
	mac->ops.get_supported_physical_layer = NULL;
	mac->ops.disable_sec_rx_path = NULL;
	mac->ops.enable_sec_rx_path = NULL;
	mac->ops.read_analog_reg8 = NULL;
	mac->ops.write_analog_reg8 = NULL;
	mac->ops.start_hw = xlnid_start_hw_westlake;
	mac->ops.get_san_mac_addr = xlnid_get_san_mac_addr_generic;
	mac->ops.get_device_caps = xlnid_get_device_caps_westlake;
	mac->ops.get_wwn_prefix = xlnid_get_wwn_prefix_generic;
	mac->ops.get_fcoe_boot_status = xlnid_get_fcoe_boot_status_generic;
	mac->ops.prot_autoc_read = prot_autoc_read_westlake;
	mac->ops.prot_autoc_write = prot_autoc_write_westlake;

	/* RAR, Multicast, VLAN */
	mac->ops.set_vmdq = xlnid_set_vmdq_generic;
	mac->ops.set_vmdq_san_mac = xlnid_set_vmdq_san_mac_generic;
	mac->ops.clear_vmdq = xlnid_clear_vmdq_generic;
	mac->ops.insert_mac_addr = xlnid_insert_mac_addr_generic;
	mac->rar_highwater = 1;
	mac->ops.set_vfta = xlnid_set_vfta_generic;
	mac->ops.set_vlvf = xlnid_set_vlvf_generic;
	mac->ops.clear_vfta = xlnid_clear_vfta_generic;
	mac->ops.init_uta_tables = xlnid_init_uta_tables_generic;
	mac->ops.setup_sfp = xlnid_setup_sfp_modules_westlake;
	mac->ops.set_mac_anti_spoofing = xlnid_set_mac_anti_spoofing;
	mac->ops.set_vlan_anti_spoofing = xlnid_set_vlan_anti_spoofing;

	/* Flow Control */
	mac->ops.fc_enable = xlnid_fc_enable_westlake;
	mac->ops.setup_fc = xlnid_setup_fc_westlake;
	mac->ops.fc_autoneg = xlnid_fc_autoneg_westlake;

	/* Link */
	mac->ops.get_link_capabilities = xlnid_get_link_capabilities_westlake;
	mac->ops.check_link = xlnid_check_mac_link_westlake;
	mac->ops.setup_link = xlnid_setup_mac_link_westlake;
	mac->ops.setup_rxpba = xlnid_set_rxpba_generic;

	xlnid_init_mac_link_ops_westlake(hw);

	mac->mcft_size = XLNID_WESTLAKE_MC_TBL_SIZE;
	mac->vft_size = XLNID_WESTLAKE_VFT_TBL_SIZE;
	mac->num_rar_entries = XLNID_WESTLAKE_RAR_ENTRIES;
	mac->rx_pb_size = XLNID_WESTLAKE_RX_PB_SIZE;
	mac->max_rx_queues = XLNID_WESTLAKE_MAX_RX_QUEUES;
	mac->max_tx_queues = XLNID_WESTLAKE_MAX_TX_QUEUES;
	mac->max_msix_vectors = xlnid_get_pcie_msix_count_generic(hw);

	mac->arc_subsystem_valid = false;

	//hw->mbx.ops.init_params = xlnid_init_mbx_params_pf;

	/* EEPROM */
	xlnid_init_eeprom_params_generic(hw);

	if (eeprom->type == xlnid_eeprom_spi) {
		eeprom->ops.read = xlnid_read_eeprom_westlake;
		eeprom->ops.read_buffer = xlnid_read_eeprom_buffer_westlake;
		eeprom->ops.write_buffer = xlnid_write_eeprom_buffer_westlake;
	}

	else {
		eeprom->ops.read = xlnid_read_eeprom_westlake;
		eeprom->ops.read_buffer = xlnid_read_eeprom_buffer_westlake;
		eeprom->ops.write_buffer = xlnid_write_eeprom_buffer_westlake;
	}

	/* Manageability interface */
	mac->ops.set_fw_drv_ver = xlnid_set_fw_drv_ver_generic;

	mac->ops.get_thermal_sensor_data = xlnid_get_thermal_sensor_data_generic;
	mac->ops.init_thermal_sensor_thresh = xlnid_init_thermal_sensor_thresh_generic;

	mac->ops.get_rtrup2tc = xlnid_dcb_get_rtrup2tc_generic;

	return ret_val;
}

/**
		xlnid_get_media_type_westlake - Get media type
		@hw: pointer to hardware structure

		Returns the media type (fiber, copper, backplane)
	**/
enum xlnid_media_type xlnid_get_media_type_westlake(struct xlnid_hw *hw)
{
	enum xlnid_media_type media_type;

	DEBUGFUNC("xlnid_get_media_type_westlake");

	media_type = xlnid_media_type_unknown;

	return media_type;
}

/**
		xlnid_stop_mac_link_on_d3_westlake - Disables link on D3
		@hw: pointer to hardware structure

		Disables link during D3 power down sequence.

	**/
void xlnid_stop_mac_link_on_d3_westlake(struct xlnid_hw *hw)
{
	return;
}

/**
		xlnid_start_mac_link_westlake - Setup MAC link settings
		@hw: pointer to hardware structure
		@autoneg_wait_to_complete: true when waiting for completion is needed

		Configures link settings based on values in the xlnid_hw struct.
		Restarts the link.  Performs autonegotiation if needed.
	**/
s32 xlnid_start_mac_link_westlake(struct xlnid_hw *hw,
									bool autoneg_wait_to_complete)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_disable_tx_laser_multispeed_fiber - Disable Tx laser
		@hw: pointer to hardware structure

		The base drivers may require better control over SFP+ module
		PHY states.  This includes selectively shutting down the Tx
		laser on the PHY, effectively halting physical link.
	**/
void xlnid_disable_tx_laser_multispeed_fiber(struct xlnid_hw *hw)
{
	return;
}

/**
		xlnid_enable_tx_laser_multispeed_fiber - Enable Tx laser
		@hw: pointer to hardware structure

		The base drivers may require better control over SFP+ module
		PHY states.  This includes selectively turning on the Tx
		laser on the PHY, effectively starting physical link.
	**/
void xlnid_enable_tx_laser_multispeed_fiber(struct xlnid_hw *hw)
{
	return;
}

/**
		xlnid_flap_tx_laser_multispeed_fiber - Flap Tx laser
		@hw: pointer to hardware structure

		When the driver changes the link speeds that it can support,
		it sets autotry_restart to true to indicate that we need to
		initiate a new autotry session with the link partner.  To do
		so, we set the speed then disable and re-enable the Tx laser, to
		alert the link partner that it also needs to restart autotry on its
		end.  This is consistent with true clause 37 autoneg, which also
		involves a loss of signal.
	**/
void xlnid_flap_tx_laser_multispeed_fiber(struct xlnid_hw *hw)
{
	DEBUGFUNC("xlnid_flap_tx_laser_multispeed_fiber");

	if (hw->mac.autotry_restart) {
		xlnid_disable_tx_laser_multispeed_fiber(hw);
		xlnid_enable_tx_laser_multispeed_fiber(hw);
		hw->mac.autotry_restart = false;
	}
}

/**
		xlnid_set_hard_rate_select_speed - Set module link speed
		@hw: pointer to hardware structure
		@speed: link speed to set

		Set module link speed via RS0/RS1 rate select pins.
*/
void xlnid_set_hard_rate_select_speed(struct xlnid_hw *hw,
										xlnid_link_speed speed)
{
	return;
}

/**
		xlnid_setup_mac_link_smartspeed - Set MAC link speed using SmartSpeed
		@hw: pointer to hardware structure
		@speed: new link speed
		@autoneg_wait_to_complete: true when waiting for completion is needed

		Implements the Intel SmartSpeed algorithm.
	**/
s32 xlnid_setup_mac_link_smartspeed(struct xlnid_hw *hw, xlnid_link_speed speed,
									bool autoneg_wait_to_complete)
{
	hw->phy.smart_speed_active = false;
	return XLNID_SUCCESS;
}

/**
		xlnid_setup_copper_link_westlake - Set the PHY autoneg advertised field
		@hw: pointer to hardware structure
		@speed: new link speed
		@autoneg_wait_to_complete: true if waiting is needed to complete

		Restarts link on PHY and MAC based on settings passed in.
	**/
STATIC s32 xlnid_setup_copper_link_westlake(struct xlnid_hw *hw,
		xlnid_link_speed speed, bool autoneg_wait_to_complete)
{
	s32 status;

	DEBUGFUNC("xlnid_setup_copper_link_westlake");

	/* Setup the PHY according to input speed */
	status = hw->phy.ops.setup_link_speed(hw, speed, autoneg_wait_to_complete);
	/* Set up MAC */
	xlnid_start_mac_link_westlake(hw, autoneg_wait_to_complete);

	return status;
}

void xlnid_init_phy_param(struct xlnid_hw *hw)
{
	u16 phy_data = 0;
	u32 val = 0;

	xlnid_write_phy_reg(hw, 0x1f, 0, 0x1);
	xlnid_read_phy_reg(hw, 0x1f, 0, &phy_data);

	if (phy_data != 0x1) {
		printk("%s:%d: gephy_0x1f = 0x%08x\r\n", __func__, __LINE__, phy_data);
	}

	xlnid_write_phy_reg(hw, 0x1e, 0, 0xd787);
	xlnid_read_phy_reg(hw, 0x1e, 0, &phy_data);

	if (phy_data != 0xd787) {
		printk("%s:%d: gephy_0x1e = 0x%08x\r\n", __func__, __LINE__, phy_data);
	}

	xlnid_write_phy_reg(hw, 0x1f, 0, 0x2);
	xlnid_read_phy_reg(hw, 0x1f, 0, &phy_data);

	if (phy_data != 0x2) {
		printk("%s:%d: gephy_0x1f = 0x%08x\r\n", __func__, __LINE__, phy_data);
	}

	xlnid_write_phy_reg(hw, 0x17, 0, 0xf000);
	xlnid_read_phy_reg(hw, 0x17, 0, &phy_data);

	if (phy_data != 0xf000) {
		printk("%s:%d: gephy_0x17 = 0x%08x\r\n", __func__, __LINE__, phy_data);
	}

	xlnid_write_phy_reg(hw, 0x1e, 0, 0xa7);
	xlnid_read_phy_reg(hw, 0x1e, 0, &phy_data);

	if (phy_data != 0xa7) {
		printk("%s:%d: gephy_0x1e = 0x%08x\r\n", __func__, __LINE__, phy_data);
	}

	xlnid_write_phy_reg(hw, 0x15, 0, 0x3c3);
	xlnid_read_phy_reg(hw, 0x15, 0, &phy_data);

	if (phy_data != 0x3c3) {
		printk("%s:%d: gephy_0x17 = 0x%08x\r\n", __func__, __LINE__, phy_data);
	}

#ifdef XLNID_GEPHY_OPTIMIZE
	xlnid_write_phy_reg(hw, 0x1f, 0, 0x0);
#endif

	val = XLNID_READ_REG_MAC(hw, WESTLAKE_PI_CFG2);
	val |= 0x1;
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG2, val);

	/* pi_sys_resetn */
	val = XLNID_READ_REG_MAC(hw, WESTLAKE_PI_CONFG0);
	val &= ~(0x10000);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CONFG0, val);

	usec_delay(1);

	val = XLNID_READ_REG_MAC(hw, WESTLAKE_PI_CONFG0);
	val |= 0x10000;
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CONFG0, val);
}

/**
		xlnid_reset_hw_westlake - Perform hardware reset
		@hw: pointer to hardware structure

		Resets the hardware by resetting the transmit and receive units, masks
		and clears all interrupts, perform a PHY reset, and perform a link (MAC)
		reset.
	**/
s32 xlnid_reset_hw_westlake(struct xlnid_hw *hw)
{
	s32 status;
	u32 val = 0;
	int i = 0;
	u32 pdbuf_stat;

#ifdef XLNID_AUTO_MEDIA_SUPPORT
	bool link_up;
	u32 link_speed;
#endif

	DEBUGFUNC("xlnid_reset_hw_westlake");

	/* Call adapter stop to disable tx/rx and clear interrupts */
	status = hw->mac.ops.stop_adapter(hw);

	if (status != XLNID_SUCCESS) {
		goto reset_hw_out;
	}

	/* flush pending Tx transactions */
	xlnid_clear_tx_pending(hw);

	/* PHY ops must be identified and initialized prior to reset */

	/* Identify PHY and related function pointers */
	status = hw->phy.ops.init(hw);

	if (status == XLNID_ERR_SFP_NOT_SUPPORTED) {
		goto reset_hw_out;
	}

	/* Setup SFP module if there is one present. */
	if (hw->phy.sfp_setup_needed) {
		status = hw->mac.ops.setup_sfp(hw);
		hw->phy.sfp_setup_needed = false;
	}

	if (status == XLNID_ERR_SFP_NOT_SUPPORTED) {
		goto reset_hw_out;
	}

	/* Reset PHY */
	if (hw->phy.reset_disable == false && hw->phy.ops.reset != NULL) {
		hw->phy.ops.reset(hw);
	}

	/* flush packet */
	/* 1. disable gmac rx & tx */
	val = xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_CONFIG);
	val &= ~(0xC);
	xlnid_write_mac_westlake(hw, WESTLAKE_GMAC_CONFIG, val);
//    netdev_info(xlnid_hw_to_netdev(hw),
//        "gmac = 0x%08x, rxctl = 0x%08x, fctrl = 0x%08x, dma fifo_status = 0x%08x\r\n",
//        xlnid_read_mac_westlake(hw, WESTLAKE_GMAC_CONFIG),
//        XLNID_READ_REG_MAC(hw, WESTLAKE_RXCTRL),
//        XLNID_READ_REG_MAC(hw, WESTLAKE_FCTRL),
//        XLNID_READ_REG_MAC_DIRECT(hw, WESTLAKE_DMA_FIFO_STATUS));

	/* 2. Reset RX ctrl */
	val = XLNID_READ_REG_MAC(hw, WESTLAKE_CRG_ETH_PORT_RST_N);
	val &= ~(0x200);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CRG_ETH_PORT_RST_N, val);
	val |= 0x200;
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CRG_ETH_PORT_RST_N, val);

	/* 3. wait rx pdbuf empty */
	for (i = 0; i < 3000; i++) {
		pdbuf_stat = XLNID_READ_REG_MAC(hw, WESTLAKE_RX_PBUF_STAT);

		if ((pdbuf_stat & 0x11) == 0x11) {
			break;
		}

		usec_delay(1);
	}

	if (i >= 3000) {
		printk("%s:%d: pdbuf_stat = 0x%08x\r\n", __func__, __LINE__, pdbuf_stat);
	}

//    val = XLNID_READ_REG_MAC(hw, (WESTLAKE_MACSEC_BASE + 0x150));
//    netdev_info(xlnid_hw_to_netdev(hw),
//        "macsec fifo_status = 0x%08x\r\n", val);
//    netdev_info(xlnid_hw_to_netdev(hw),
//        "rxctl pdbuf_stat = 0x%08x\r\n", pdbuf_stat);
//
//    val = XLNID_READ_REG_MAC_DIRECT(hw, (WESTLAKE_DMA_CTL_BASE + 0x88));
//    netdev_info(xlnid_hw_to_netdev(hw),
//        "dma fifo_status = 0x%08x\r\n", val);

	/* 4. reset ethport (only rx) */
	val = XLNID_READ_REG_MAC(hw, WESTLAKE_CRG_ETH_PORT_RST_N);
	val &= ~(0x20);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CRG_ETH_PORT_RST_N, val);
	val |= 0x20;
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CRG_ETH_PORT_RST_N, val);

	XLNID_WRITE_REG_MAC_DIRECT(hw, WESTLAKE_TX_PKTREQ_FIFO_TH, 0x102c);
	XLNID_WRITE_REG_MAC_DIRECT(hw, WESTLAKE_TX_DESC_USED_FIFO_TH, 0x10010808);

	/* GEPHY power on, in case fiber power down gephy and switch fiber to copper */
	val = XLNID_READ_REG_MAC(hw, WESTLAKE_PI_CFG1);

	if (val == 0x3) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG1, 0x10);
	}

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_IRST_IPD_CTRL, 0x7f);

	if (hw->device_id == XLNID_DEV_ID_1000BASEX) {
		XLNID_WRITE_REG_MAC(hw, WESTALEK_CFG_AN_EN, 0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1, 0x36);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x2);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0x64);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG1, 0x3);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG2, 0x1);

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_TXDEEMPH, 0x4500);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ0, 0x38B7);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ1, 0x0100);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ2, 0x0021);

		hw->phy.media_type = xlnid_media_type_fiber;
		goto out;
	}

	/* SGMII_MAC */
	if (hw->device_id == XLNID_DEV_ID_SGMII_MAC) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1, 0x7);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x2);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0x8E0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0x1);
		XLNID_WRITE_REG_MAC(hw, WESTALEK_CFG_AN_EN, 1);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG1, 0x3);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG2, 0x1);

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_TXDEEMPH, 0x4500);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ0, 0x38B7);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ1, 0x0100);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ2, 0x0021);

		hw->phy.media_type = xlnid_media_type_sgmii_mac;
		goto out;
	}

	/* SGMII_PHY */
	if (hw->device_id == XLNID_DEV_ID_SGMII_PHY) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1, 0x7);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x2);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0xE4);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0x1);
		XLNID_WRITE_REG_MAC(hw, WESTALEK_CFG_AN_EN, 1);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG1, 0x3);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG2, 0x1);

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_TXDEEMPH, 0x4500);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ0, 0x38B7);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ1, 0x0100);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ2, 0x0021);

		hw->phy.media_type = xlnid_media_type_sgmii_phy;

		goto out;
	}

#ifdef XLNID_GEPHY_SUPPORT
	/* gephy */
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_LINK_MASK, 0x0);
	//XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_PMA_REF_CTRL, 0x1220);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_IRST_IPD_CTRL, 0x4f);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0x171);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x0);
	//XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCTL, 0xf04);

	xlnid_init_phy_param(hw);

	hw->phy.media_type = xlnid_media_type_copper;
#endif

#ifdef XLNID_1000BASEX_SUPPORT
	XLNID_WRITE_REG_MAC(hw, WESTALEK_CFG_AN_EN, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1, 0x36);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x2);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0xE4);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_LINK_MASK, 0x2);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG1, 0x3);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG2, 0x1);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_TXDEEMPH, 0x4500);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ0, 0x38B7);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ1, 0x0100);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ2, 0x0021);
	//XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCTL, 0x40f);

	hw->phy.media_type = xlnid_media_type_fiber;
#endif

#ifdef XLNID_FORCE_1000BASEX_SUPPORT
	XLNID_WRITE_REG_MAC(hw, WESTALEK_CFG_AN_EN, 0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1, 0x36);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x2);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0x0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0x64);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_LINK_MASK, 0x2);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG1, 0x3);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG2, 0x1);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_TXDEEMPH, 0x4500);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ0, 0x38B7);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ1, 0x0100);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ2, 0x0021);

	hw->phy.media_type = xlnid_media_type_fiber;
#endif

#ifdef XLNID_AUTO_MEDIA_SUPPORT
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0xE4);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_TXDEEMPH, 0x4500);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ0, 0x38B7);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ1, 0x0100);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ2, 0x0021);

	xlnid_init_phy_param(hw);

	hw->mac.ops.check_link(hw, &link_speed, &link_up, false);
#endif

#ifdef XLNID_RMII_MAC_SUPPORT
	u32 regval = XLNID_READ_REG_MAC(hw, WESTLAKE_DEBUG_PHYSTATUS);

	regval = (regval & (~XLNID_PHYSTATUS_ADDR0)) | (0x1 << 25);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_DEBUG_PHYSTATUS, regval);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x2);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_EXT_POLL, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_LINK_MASK, 0x0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_LINK_UP, 0x1);

	hw->phy.media_type = xlnid_media_type_rmii_mac;
#endif

#ifdef XLNID_SGMII_MAC_SUPPORT
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1, 0x7);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x2);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0x8E0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTALEK_CFG_AN_EN, 1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG1, 0x3);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG2, 0x1);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_TXDEEMPH, 0x4500);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ0, 0x38B7);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ1, 0x0100);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ2, 0x0021);

	hw->phy.media_type = xlnid_media_type_sgmii_mac;
#endif

#ifdef XLNID_SGMII_PHY_SUPPORT
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1, 0x7);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x2);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, 0xE4);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_AN_EDIT_EN, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTALEK_CFG_AN_EN, 1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG1, 0x3);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_PI_CFG2, 0x1);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_TXDEEMPH, 0x4500);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ0, 0x38B7);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ1, 0x0100);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SDS_RX_EQ2, 0x0021);

	hw->phy.media_type = xlnid_media_type_sgmii_phy;
#endif

out:
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MAC_WDATA, 0x02F12E8C);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MAC_CMD, 0x80000000);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MAC_WDATA, 0x80000001);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MAC_CMD, 0x80000004);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MAC_WDATA, 0x00000004);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MAC_CMD, 0x80000100);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_TXBUF_RD_TH, 0x7);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MAXFRS, XLNID_MAXFRS_DEFAULT);

	//XLNID_WRITE_REG_MAC(hw, WESTLAKE_MACSEC_RX_DEAL_INTERVAL, 0x2);

	/* Get MAC address from EEPROM */
	hw->mac.ops.get_san_mac_addr(hw, hw->mac.perm_addr);

	if (xlnid_validate_mac_addr(hw->mac.perm_addr) != 0) {
		/* Get MAC address from register */
		hw->mac.ops.get_mac_addr(hw, hw->mac.perm_addr);
	}

	/*
		Store MAC address from RAR0, clear receive address registers, and
		clear the multicast table.  Also reset num_rar_entries to 32,
		since we modify this value when programming the SAN MAC address.
	*/
	hw->mac.num_rar_entries = 32;
	hw->mac.ops.init_rx_addrs(hw);

reset_hw_out:
	return status;
}

/**
		xlnid_fdir_check_cmd_complete - poll to check whether FDIRCMD is complete
		@hw: pointer to hardware structure
		@fdircmd: current value of FDIRCMD register
*/
STATIC s32 xlnid_fdir_check_cmd_complete(struct xlnid_hw *hw, u32 *fdircmd)
{
	int i;

	for (i = 0; i < XLNID_FDIRCMD_CMD_POLL; i++) {
		*fdircmd = XLNID_READ_REG(hw, FDIRCMD);

		if (!(*fdircmd & XLNID_FDIRCMD_CMD_MASK)) {
			return XLNID_SUCCESS;
		}

		usec_delay(10);
	}

	return XLNID_ERR_FDIR_CMD_INCOMPLETE;
}

/**
		xlnid_reinit_fdir_tables_westlake - Reinitialize Flow Director tables.
		@hw: pointer to hardware structure
	**/
s32 xlnid_reinit_fdir_tables_westlake(struct xlnid_hw *hw)
{
	s32 err;
	int i;
	u32 fdirctrl = XLNID_READ_REG(hw, FDIRCTRL);
	u32 fdircmd;

	fdirctrl &= ~XLNID_FDIRCTRL_INIT_DONE;

	DEBUGFUNC("xlnid_reinit_fdir_tables_westlake");

	/*
		Before starting reinitialization process,
		FDIRCMD.CMD must be zero.
	*/
	err = xlnid_fdir_check_cmd_complete(hw, &fdircmd);

	if (err) {
		hw_dbg(hw,
				"Flow Director previous command did not complete, aborting table re-initialization.\n");
		return err;
	}

	XLNID_WRITE_REG(hw, FDIRFREE, 0);
	XLNID_WRITE_FLUSH(hw);
	/*
		westlake adapters flow director init flow cannot be restarted,
		Workaround westlake silicon errata by performing the following steps
		before re-writing the FDIRCTRL control register with the same value.
		- write 1 to bit 8 of FDIRCMD register &
		- write 0 to bit 8 of FDIRCMD register
	*/
	XLNID_WRITE_REG(hw, FDIRCMD, (XLNID_READ_REG(hw,
									FDIRCMD) | XLNID_FDIRCMD_CLEARHT));
	XLNID_WRITE_FLUSH(hw);
	XLNID_WRITE_REG(hw, FDIRCMD, (XLNID_READ_REG(hw,
									FDIRCMD) & ~XLNID_FDIRCMD_CLEARHT));
	XLNID_WRITE_FLUSH(hw);
	/*
		Clear FDIR Hash register to clear any leftover hashes
		waiting to be programmed.
	*/
	XLNID_WRITE_REG(hw, FDIRHASH, 0x00);
	XLNID_WRITE_FLUSH(hw);

	XLNID_WRITE_REG(hw, FDIRCTRL, fdirctrl);
	XLNID_WRITE_FLUSH(hw);

	/* Poll init-done after we write FDIRCTRL register */
	for (i = 0; i < XLNID_FDIR_INIT_DONE_POLL; i++) {
		if (XLNID_READ_REG(hw, FDIRCTRL) & XLNID_FDIRCTRL_INIT_DONE) {
			break;
		}

		msec_delay(1);
	}

	if (i >= XLNID_FDIR_INIT_DONE_POLL) {
		hw_dbg(hw, "Flow Director Signature poll time exceeded!\n");
		return XLNID_ERR_FDIR_REINIT_FAILED;
	}

	/* Clear FDIR statistics registers (read to clear) */
	XLNID_READ_REG(hw, FDIRUSTAT);
	XLNID_READ_REG(hw, FDIRFSTAT);
	XLNID_READ_REG(hw, FDIRMATCH);
	XLNID_READ_REG(hw, FDIRMISS);

	return XLNID_SUCCESS;
}

/**
		xlnid_fdir_enable_westlake - Initialize Flow Director control registers
		@hw: pointer to hardware structure
		@fdirctrl: value to write to flow director control register
	**/
STATIC void xlnid_fdir_enable_westlake(struct xlnid_hw *hw, u32 fdirctrl)
{
	int i;

	DEBUGFUNC("xlnid_fdir_enable_westlake");

	/* Prime the keys for hashing */
	XLNID_WRITE_REG(hw, FDIRHKEY, XLNID_ATR_BUCKET_HASH_KEY);

	/*
		Poll init-done after we write the register.  Estimated times:
			10G: PBALLOC = 11b, timing is 60us
				1G: PBALLOC = 11b, timing is 600us
			100M: PBALLOC = 11b, timing is 6ms

			Multiple these timings by 4 if under full Rx load

		So we'll poll for XLNID_FDIR_INIT_DONE_POLL times, sleeping for
		1 msec per poll time.  If we're at line rate and drop to 100M, then
		this might not finish in our poll time, but we can live with that
		for now.
	*/
	XLNID_WRITE_REG(hw, FDIRCTRL, fdirctrl);
	XLNID_WRITE_FLUSH(hw);

	for (i = 0; i < XLNID_FDIR_INIT_DONE_POLL; i++) {
		if (XLNID_READ_REG(hw, FDIRCTRL) & XLNID_FDIRCTRL_INIT_DONE) {
			break;
		}

		msec_delay(1);
	}

	if (i >= XLNID_FDIR_INIT_DONE_POLL) {
		hw_dbg(hw, "Flow Director poll time exceeded!\n");
	}
}

/**
		xlnid_init_fdir_signature_westlake - Initialize Flow Director signature filters
		@hw: pointer to hardware structure
		@fdirctrl: value to write to flow director control register, initially
				contains just the value of the Rx packet buffer allocation
	**/
s32 xlnid_init_fdir_signature_westlake(struct xlnid_hw *hw, u32 fdirctrl)
{
	DEBUGFUNC("xlnid_init_fdir_signature_westlake");

	/*
		Continue setup of fdirctrl register bits:
		Move the flexible bytes to use the ethertype - shift 6 words
		Set the maximum length per hash bucket to 0xA filters
		Send interrupt when 64 filters are left
	*/
	fdirctrl |= (0x6 << XLNID_FDIRCTRL_FLEX_SHIFT) |
				(0xA << XLNID_FDIRCTRL_MAX_LENGTH_SHIFT) |
				(4 << XLNID_FDIRCTRL_FULL_THRESH_SHIFT);

	/* write hashes and fdirctrl register, poll for completion */
	xlnid_fdir_enable_westlake(hw, fdirctrl);

	return XLNID_SUCCESS;
}

/**
		xlnid_init_fdir_perfect_westlake - Initialize Flow Director perfect filters
		@hw: pointer to hardware structure
		@fdirctrl: value to write to flow director control register, initially
				contains just the value of the Rx packet buffer allocation
		@cloud_mode: true - cloud mode, false - other mode
	**/
s32 xlnid_init_fdir_perfect_westlake(struct xlnid_hw *hw, u32 fdirctrl,
										bool cloud_mode)
{
	UNREFERENCED_1PARAMETER(cloud_mode);
	DEBUGFUNC("xlnid_init_fdir_perfect_westlake");

	/*
		Continue setup of fdirctrl register bits:
		Turn perfect match filtering on
		Report hash in RSS field of Rx wb descriptor
		Initialize the drop queue to queue 127
		Move the flexible bytes to use the ethertype - shift 6 words
		Set the maximum length per hash bucket to 0xA filters
		Send interrupt when 64 (0x4 * 16) filters are left
	*/
	fdirctrl |= XLNID_FDIRCTRL_PERFECT_MATCH |
				XLNID_FDIRCTRL_REPORT_STATUS |
				(XLNID_FDIR_DROP_QUEUE << XLNID_FDIRCTRL_DROP_Q_SHIFT) |
				(0x6 << XLNID_FDIRCTRL_FLEX_SHIFT) |
				(0xA << XLNID_FDIRCTRL_MAX_LENGTH_SHIFT) |
				(4 << XLNID_FDIRCTRL_FULL_THRESH_SHIFT);

	if (cloud_mode) {
		fdirctrl |= (XLNID_FDIRCTRL_FILTERMODE_CLOUD <<
						XLNID_FDIRCTRL_FILTERMODE_SHIFT);
	}

	/* write hashes and fdirctrl register, poll for completion */
	xlnid_fdir_enable_westlake(hw, fdirctrl);

	return XLNID_SUCCESS;
}

/**
		xlnid_set_fdir_drop_queue_westlake - Set Flow Director drop queue
		@hw: pointer to hardware structure
		@dropqueue: Rx queue index used for the dropped packets
	**/
void xlnid_set_fdir_drop_queue_westlake(struct xlnid_hw *hw, u8 dropqueue)
{
	u32 fdirctrl;

	DEBUGFUNC("xlnid_set_fdir_drop_queue_westlake");
	/* Clear init done bit and drop queue field */
	fdirctrl = XLNID_READ_REG(hw, FDIRCTRL);
	fdirctrl &= ~(XLNID_FDIRCTRL_DROP_Q_MASK | XLNID_FDIRCTRL_INIT_DONE);

	/* Set drop queue */
	fdirctrl |= (dropqueue << XLNID_FDIRCTRL_DROP_Q_SHIFT);

	XLNID_WRITE_REG(hw, FDIRCMD, (XLNID_READ_REG(hw,
									FDIRCMD) | XLNID_FDIRCMD_CLEARHT));
	XLNID_WRITE_FLUSH(hw);
	XLNID_WRITE_REG(hw, FDIRCMD, (XLNID_READ_REG(hw,
									FDIRCMD) & ~XLNID_FDIRCMD_CLEARHT));
	XLNID_WRITE_FLUSH(hw);

	/* write hashes and fdirctrl register, poll for completion */
	xlnid_fdir_enable_westlake(hw, fdirctrl);
}

/*
		These defines allow us to quickly generate all of the necessary instructions
		in the function below by simply calling out XLNID_COMPUTE_SIG_HASH_ITERATION
		for values 0 through 15
*/
#define XLNID_ATR_COMMON_HASH_KEY \
		(XLNID_ATR_BUCKET_HASH_KEY & XLNID_ATR_SIGNATURE_HASH_KEY)
#define XLNID_COMPUTE_SIG_HASH_ITERATION(_n) \
do { \
	u32 n = (_n); \
	if (XLNID_ATR_COMMON_HASH_KEY & (0x01 << n)) \
		common_hash ^= lo_hash_dword >> n; \
	else if (XLNID_ATR_BUCKET_HASH_KEY & (0x01 << n)) \
		bucket_hash ^= lo_hash_dword >> n; \
	else if (XLNID_ATR_SIGNATURE_HASH_KEY & (0x01 << n)) \
		sig_hash ^= lo_hash_dword << (16 - n); \
	if (XLNID_ATR_COMMON_HASH_KEY & (0x01 << (n + 16))) \
		common_hash ^= hi_hash_dword >> n; \
	else if (XLNID_ATR_BUCKET_HASH_KEY & (0x01 << (n + 16))) \
		bucket_hash ^= hi_hash_dword >> n; \
	else if (XLNID_ATR_SIGNATURE_HASH_KEY & (0x01 << (n + 16))) \
		sig_hash ^= hi_hash_dword << (16 - n); \
} while (0)

/**
		xlnid_atr_compute_sig_hash_westlake - Compute the signature hash
		@input: input bitstream to compute the hash on
		@common: compressed common input dword

		This function is almost identical to the function above but contains
		several optimizations such as unwinding all of the loops, letting the
		compiler work out all of the conditional ifs since the keys are static
		defines, and computing two keys at once since the hashed dword stream
		will be the same for both keys.
	**/
u32 xlnid_atr_compute_sig_hash_westlake(union xlnid_atr_hash_dword input,
										union xlnid_atr_hash_dword common)
{
	u32 hi_hash_dword, lo_hash_dword, flow_vm_vlan;
	u32 sig_hash = 0, bucket_hash = 0, common_hash = 0;

	/* record the flow_vm_vlan bits as they are a key part to the hash */
	flow_vm_vlan = XLNID_NTOHL(input.dword);

	/* generate common hash dword */
	hi_hash_dword = XLNID_NTOHL(common.dword);

	/* low dword is word swapped version of common */
	lo_hash_dword = (hi_hash_dword >> 16) | (hi_hash_dword << 16);

	/* apply flow ID/VM pool/VLAN ID bits to hash words */
	hi_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan >> 16);

	/* Process bits 0 and 16 */
	XLNID_COMPUTE_SIG_HASH_ITERATION(0);

	/*
		apply flow ID/VM pool/VLAN ID bits to lo hash dword, we had to
		delay this because bit 0 of the stream should not be processed
		so we do not add the VLAN until after bit 0 was processed
	*/
	lo_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan << 16);

	/* Process remaining 30 bit of the key */
	XLNID_COMPUTE_SIG_HASH_ITERATION(1);
	XLNID_COMPUTE_SIG_HASH_ITERATION(2);
	XLNID_COMPUTE_SIG_HASH_ITERATION(3);
	XLNID_COMPUTE_SIG_HASH_ITERATION(4);
	XLNID_COMPUTE_SIG_HASH_ITERATION(5);
	XLNID_COMPUTE_SIG_HASH_ITERATION(6);
	XLNID_COMPUTE_SIG_HASH_ITERATION(7);
	XLNID_COMPUTE_SIG_HASH_ITERATION(8);
	XLNID_COMPUTE_SIG_HASH_ITERATION(9);
	XLNID_COMPUTE_SIG_HASH_ITERATION(10);
	XLNID_COMPUTE_SIG_HASH_ITERATION(11);
	XLNID_COMPUTE_SIG_HASH_ITERATION(12);
	XLNID_COMPUTE_SIG_HASH_ITERATION(13);
	XLNID_COMPUTE_SIG_HASH_ITERATION(14);
	XLNID_COMPUTE_SIG_HASH_ITERATION(15);

	/* combine common_hash result with signature and bucket hashes */
	bucket_hash ^= common_hash;
	bucket_hash &= XLNID_ATR_HASH_MASK;

	sig_hash ^= common_hash << 16;
	sig_hash &= XLNID_ATR_HASH_MASK << 16;

	/* return completed signature hash */
	return sig_hash ^ bucket_hash;
}

/**
		xlnid_atr_add_signature_filter_westlake - Adds a signature hash filter
		@hw: pointer to hardware structure
		@input: unique input dword
		@common: compressed common input dword
		@queue: queue index to direct traffic to

		Note that the tunnel bit in input must not be set when the hardware
		tunneling support does not exist.
	**/
void xlnid_fdir_add_signature_filter_westlake(struct xlnid_hw *hw,
		union xlnid_atr_hash_dword input,
		union xlnid_atr_hash_dword common,
		u8 queue)
{
	u64 fdirhashcmd;
	u8 flow_type;
	bool tunnel;
	u32 fdircmd;

	DEBUGFUNC("xlnid_fdir_add_signature_filter_westlake");

	/*
		Get the flow_type in order to program FDIRCMD properly
		lowest 2 bits are FDIRCMD.L4TYPE, third lowest bit is FDIRCMD.IPV6
		fifth is FDIRCMD.TUNNEL_FILTER
	*/
	tunnel = !!(input.formatted.flow_type & XLNID_ATR_L4TYPE_TUNNEL_MASK);
	flow_type = input.formatted.flow_type & (XLNID_ATR_L4TYPE_TUNNEL_MASK - 1);

	switch (flow_type) {
	case XLNID_ATR_FLOW_TYPE_TCPV4:
	case XLNID_ATR_FLOW_TYPE_UDPV4:
	case XLNID_ATR_FLOW_TYPE_SCTPV4:
	case XLNID_ATR_FLOW_TYPE_TCPV6:
	case XLNID_ATR_FLOW_TYPE_UDPV6:
	case XLNID_ATR_FLOW_TYPE_SCTPV6:
		break;

	default:
		hw_dbg(hw, " Error on flow type input\n");
		return;
	}

	/* configure FDIRCMD register */
	fdircmd = XLNID_FDIRCMD_CMD_ADD_FLOW | XLNID_FDIRCMD_FILTER_UPDATE |
				XLNID_FDIRCMD_LAST | XLNID_FDIRCMD_QUEUE_EN;
	fdircmd |= (u32) flow_type << XLNID_FDIRCMD_FLOW_TYPE_SHIFT;
	fdircmd |= (u32) queue << XLNID_FDIRCMD_RX_QUEUE_SHIFT;

	if (tunnel) {
		fdircmd |= XLNID_FDIRCMD_TUNNEL_FILTER;
	}

	/*
		The lower 32-bits of fdirhashcmd is for FDIRHASH, the upper 32-bits
		is for FDIRCMD.  Then do a 64-bit register write from FDIRHASH.
	*/
	fdirhashcmd = (u64) fdircmd << 32;
	fdirhashcmd |= xlnid_atr_compute_sig_hash_westlake(input, common);
	XLNID_WRITE_REG64(hw, FDIRHASH, fdirhashcmd);

	hw_dbg(hw, "Tx Queue=%x hash=%x\n", queue, (u32) fdirhashcmd);

	return;
}

#define XLNID_COMPUTE_BKT_HASH_ITERATION(_n) \
do { \
	u32 n = (_n); \
	if (XLNID_ATR_BUCKET_HASH_KEY & (0x01 << n)) \
		bucket_hash ^= lo_hash_dword >> n; \
	if (XLNID_ATR_BUCKET_HASH_KEY & (0x01 << (n + 16))) \
		bucket_hash ^= hi_hash_dword >> n; \
} while (0)

/**
		xlnid_atr_compute_perfect_hash_westlake - Compute the perfect filter hash
		@input: input bitstream to compute the hash on
		@input_mask: mask for the input bitstream

		This function serves two main purposes.  First it applies the input_mask
		to the atr_input resulting in a cleaned up atr_input data stream.
		Secondly it computes the hash and stores it in the bkt_hash field at
		the end of the input byte stream.  This way it will be available for
		future use without needing to recompute the hash.
	**/
void xlnid_atr_compute_perfect_hash_westlake(union xlnid_atr_input *input,
		union xlnid_atr_input *input_mask)
{

	u32 hi_hash_dword, lo_hash_dword, flow_vm_vlan;
	u32 bucket_hash = 0;
	u32 hi_dword = 0;
	u32 i = 0;

	/* Apply masks to input data */
	for (i = 0; i < 14; i++) {
		input->dword_stream[i] &= input_mask->dword_stream[i];
	}

	/* record the flow_vm_vlan bits as they are a key part to the hash */
	flow_vm_vlan = XLNID_NTOHL(input->dword_stream[0]);

	/* generate common hash dword */
	for (i = 1; i <= 13; i++) {
		hi_dword ^= input->dword_stream[i];
	}

	hi_hash_dword = XLNID_NTOHL(hi_dword);

	/* low dword is word swapped version of common */
	lo_hash_dword = (hi_hash_dword >> 16) | (hi_hash_dword << 16);

	/* apply flow ID/VM pool/VLAN ID bits to hash words */
	hi_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan >> 16);

	/* Process bits 0 and 16 */
	XLNID_COMPUTE_BKT_HASH_ITERATION(0);

	/*
		apply flow ID/VM pool/VLAN ID bits to lo hash dword, we had to
		delay this because bit 0 of the stream should not be processed
		so we do not add the VLAN until after bit 0 was processed
	*/
	lo_hash_dword ^= flow_vm_vlan ^ (flow_vm_vlan << 16);

	/* Process remaining 30 bit of the key */
	for (i = 1; i <= 15; i++) {
		XLNID_COMPUTE_BKT_HASH_ITERATION(i);
	}

	/*
		Limit hash to 13 bits since max bucket count is 8K.
		Store result at the end of the input stream.
	*/
	input->formatted.bkt_hash = bucket_hash & 0x1FFF;
}

/**
		xlnid_get_fdirtcpm_westlake - generate a TCP port from atr_input_masks
		@input_mask: mask to be bit swapped

		The source and destination port masks for flow director are bit swapped
		in that bit 15 effects bit 0, 14 effects 1, 13, 2 etc.  In order to
		generate a correctly swapped value we need to bit swap the mask and that
		is what is accomplished by this function.
	**/
STATIC u32 xlnid_get_fdirtcpm_westlake(union xlnid_atr_input *input_mask)
{
	u32 mask = XLNID_NTOHS(input_mask->formatted.dst_port);

	mask <<= XLNID_FDIRTCPM_DPORTM_SHIFT;
	mask |= XLNID_NTOHS(input_mask->formatted.src_port);
	mask = ((mask & 0x55555555) << 1) | ((mask & 0xAAAAAAAA) >> 1);
	mask = ((mask & 0x33333333) << 2) | ((mask & 0xCCCCCCCC) >> 2);
	mask = ((mask & 0x0F0F0F0F) << 4) | ((mask & 0xF0F0F0F0) >> 4);
	return ((mask & 0x00FF00FF) << 8) | ((mask & 0xFF00FF00) >> 8);
}

/*
		These two macros are meant to address the fact that we have registers
		that are either all or in part big-endian.  As a result on big-endian
		systems we will end up byte swapping the value to little-endian before
		it is byte swapped again and written to the hardware in the original
		big-endian format.
*/
#define XLNID_STORE_AS_BE32(_value) \
	(((u32)(_value) >> 24) | (((u32)(_value) & 0x00FF0000) >> 8) | \
		(((u32)(_value) & 0x0000FF00) << 8) | ((u32)(_value) << 24))

#define XLNID_WRITE_REG_BE32(a, reg, value) \
	(xlnid_write_reg((a), (WESTLAKE_##reg), XLNID_STORE_AS_BE32(XLNID_NTOHL(value)), (false)))

#define XLNID_STORE_AS_BE16(_value) \
	XLNID_NTOHS(((u16)(_value) >> 8) | ((u16)(_value) << 8))

s32 xlnid_fdir_set_input_mask_westlake(struct xlnid_hw *hw,
										union xlnid_atr_input *input_mask, bool cloud_mode)
{
	/* mask IPv6 since it is currently not supported */
	u32 fdirm = XLNID_FDIRM_DIPv6;
	u32 fdirtcpm;
	u32 fdirip6m;

	UNREFERENCED_1PARAMETER(cloud_mode);
	DEBUGFUNC("xlnid_fdir_set_atr_input_mask_westlake");

	/*
		Program the relevant mask registers.  If src/dst_port or src/dst_addr
		are zero, then assume a full mask for that field.  Also assume that
		a VLAN of 0 is unspecified, so mask that out as well.  L4type
		cannot be masked out in this implementation.

		This also assumes IPv4 only.  IPv6 masking isn't supported at this
		point in time.
	*/

	/* verify bucket hash is cleared on hash generation */
	if (input_mask->formatted.bkt_hash) {
		hw_dbg(hw, " bucket hash should always be 0 in mask\n");
	}

	/* Program FDIRM and verify partial masks */
	switch (input_mask->formatted.vm_pool & 0x7F) {
	case 0x0:
		fdirm |= XLNID_FDIRM_POOL;
		break;
	case 0x7F:
		break;

	default:
		hw_dbg(hw, " Error on vm pool mask\n");
		return XLNID_ERR_CONFIG;
	}

	switch (input_mask->formatted.flow_type & XLNID_ATR_L4TYPE_MASK) {
	case 0x0:
		fdirm |= XLNID_FDIRM_L4P;

		if (input_mask->formatted.dst_port || input_mask->formatted.src_port) {
		hw_dbg(hw, " Error on src/dst port mask\n");
		return XLNID_ERR_CONFIG;
		}
		break;
	case XLNID_ATR_L4TYPE_MASK:
		break;

	default:
		hw_dbg(hw, " Error on flow type mask\n");
		return XLNID_ERR_CONFIG;
	}

	switch (XLNID_NTOHS(input_mask->formatted.vlan_id) & 0xEFFF) {
	case 0x0000:
		/* mask VLAN ID */
		fdirm |= XLNID_FDIRM_VLANID;
		fallthrough;

	case 0x0FFF:
		/* mask VLAN ID */
		fdirm |= XLNID_FDIRM_VLANP;
		break;

	case 0xE000:
		/* mask VLAN ID only */
		fdirm |= XLNID_FDIRM_VLANID;
		fallthrough;

	case 0xEFFF:
		/* no VLAN fields masked */
		break;

	default:
		hw_dbg(hw, " Error on VLAN mask\n");
		return XLNID_ERR_CONFIG;
	}

	switch (input_mask->formatted.flex_bytes & 0xFFFF) {
	case 0x0000:
		/* Mask Flex Bytes */
		fdirm |= XLNID_FDIRM_FLEX;
		fallthrough;

	case 0xFFFF:
		break;

	default:
		hw_dbg(hw, " Error on flexible byte mask\n");
		return XLNID_ERR_CONFIG;
	}

	if (cloud_mode) {
		fdirm |= XLNID_FDIRM_L3P;
		fdirip6m = ((u32) 0xFFFFU << XLNID_FDIRIP6M_DIPM_SHIFT);
		fdirip6m |= XLNID_FDIRIP6M_ALWAYS_MASK;

		switch (input_mask->formatted.inner_mac[0] & 0xFF) {
		case 0x00:
			/* Mask inner MAC, fall through */
			fdirip6m |= XLNID_FDIRIP6M_INNER_MAC;
			fallthrough;
		case 0xFF:
			break;

		default:
			hw_dbg(hw, " Error on inner_mac byte mask\n");
			return XLNID_ERR_CONFIG;
		}

		switch (input_mask->formatted.tni_vni & 0xFFFFFFFF) {
		case 0x0:
			/* Mask vxlan id */
			fdirip6m |= XLNID_FDIRIP6M_TNI_VNI;
			break;

		case 0x00FFFFFF:
			fdirip6m |= XLNID_FDIRIP6M_TNI_VNI_24;
			break;

		case 0xFFFFFFFF:
			break;

		default:
			hw_dbg(hw, " Error on TNI/VNI byte mask\n");
			return XLNID_ERR_CONFIG;
		}

		switch (input_mask->formatted.tunnel_type & 0xFFFF) {
		case 0x0:
			/* Mask turnnel type, fall through */
			fdirip6m |= XLNID_FDIRIP6M_TUNNEL_TYPE;
			fallthrough;
		case 0xFFFF:
			break;

		default:
			hw_dbg(hw, " Error on tunnel type byte mask\n");
			return XLNID_ERR_CONFIG;
		}

		/*  Set all bits in FDIRTCPM, FDIRUDPM, FDIRSCTPM,
			FDIRSIP4M and FDIRDIP4M in cloud mode to allow
			L3/L3 packets to tunnel.
		*/
		XLNID_WRITE_REG(hw, FDIRTCPM, 0xFFFFFFFF);
		XLNID_WRITE_REG(hw, FDIRUDPM, 0xFFFFFFFF);
		XLNID_WRITE_REG(hw, FDIRDIP4M, 0xFFFFFFFF);
		XLNID_WRITE_REG(hw, FDIRSIP4M, 0xFFFFFFFF);
	}

	/* Now mask VM pool and destination IPv6 - bits 5 and 2 */
	XLNID_WRITE_REG(hw, FDIRM, fdirm);

	if (!cloud_mode) {
		/*  store the TCP/UDP port masks, bit reversed from port
			layout */
		fdirtcpm = xlnid_get_fdirtcpm_westlake(input_mask);

		/* write both the same so that UDP and TCP use the same mask */
		XLNID_WRITE_REG(hw, FDIRTCPM, ~fdirtcpm);
		XLNID_WRITE_REG(hw, FDIRUDPM, ~fdirtcpm);

		/* store source and destination IP masks (big-enian) */
		XLNID_WRITE_REG(hw, FDIRSIP4M, ~input_mask->formatted.src_ip[0]);
		XLNID_WRITE_REG(hw, FDIRDIP4M, ~input_mask->formatted.dst_ip[0]);
	}

	return XLNID_SUCCESS;
}

s32 xlnid_fdir_write_perfect_filter_westlake(struct xlnid_hw *hw,
		union xlnid_atr_input *input,
		u16 soft_id, u8 queue, bool cloud_mode)
{
	u32 fdirport, fdirvlan, fdirhash, fdircmd;
	u32 addr_low, addr_high;
	u32 cloud_type = 0;
	s32 err;

	UNREFERENCED_1PARAMETER(cloud_mode);

	DEBUGFUNC("xlnid_fdir_write_perfect_filter_westlake");

	if (!cloud_mode) {
		/* currently IPv6 is not supported, must be programmed with 0 */
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(0), input->formatted.src_ip[0]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(1), input->formatted.src_ip[1]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(2), input->formatted.src_ip[2]);

		/* record the source address (big-endian) */
		XLNID_WRITE_REG(hw, FDIRIPSA, input->formatted.src_ip[0]);

		/*  record the first 32 bits of the destination address
			(big-endian) */
		XLNID_WRITE_REG(hw, FDIRIPDA, input->formatted.dst_ip[0]);

		/* record source and destination port (little-endian) */
		fdirport = XLNID_NTOHS(input->formatted.dst_port);
		fdirport <<= XLNID_FDIRPORT_DESTINATION_SHIFT;
		fdirport |= XLNID_NTOHS(input->formatted.src_port);
		XLNID_WRITE_REG(hw, FDIRPORT, fdirport);
	}

	/* record VLAN (little-endian) and flex_bytes(big-endian) */
	fdirvlan = XLNID_STORE_AS_BE16(input->formatted.flex_bytes);
	fdirvlan <<= XLNID_FDIRVLAN_FLEX_SHIFT;
	fdirvlan |= XLNID_NTOHS(input->formatted.vlan_id);
	XLNID_WRITE_REG(hw, FDIRVLAN, fdirvlan);

	if (cloud_mode) {
		if (input->formatted.tunnel_type != 0) {
			cloud_type = 0x80000000;
		}

		addr_low = ((u32) input->formatted.inner_mac[0] |
					((u32) input->formatted.inner_mac[1] << 8) |
					((u32) input->formatted.inner_mac[2] << 16) |
					((u32) input->formatted.inner_mac[3] << 24));
		addr_high = ((u32) input->formatted.inner_mac[4] |
						((u32) input->formatted.inner_mac[5] << 8));
		cloud_type |= addr_high;
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(0), addr_low);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(1), cloud_type);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(2), input->formatted.tni_vni);
	}

	/* configure FDIRHASH register */

	fdirhash = input->formatted.bkt_hash;

	fdirhash |= soft_id << XLNID_FDIRHASH_SIG_SW_INDEX_SHIFT;
	fdirhash |= XLNID_FDIRHASH_BUCKET_VALID_ENABLE;
	XLNID_WRITE_REG(hw, FDIRHASH, fdirhash);

	/*
		flush all previous writes to make certain registers are
		programmed prior to issuing the command
	*/
	XLNID_WRITE_FLUSH(hw);

	/* configure FDIRCMD register */
	fdircmd = XLNID_FDIRCMD_CMD_ADD_FLOW | XLNID_FDIRCMD_FILTER_UPDATE |
				XLNID_FDIRCMD_LAST | XLNID_FDIRCMD_QUEUE_EN;

	if (queue == XLNID_FDIR_DROP_QUEUE) {
		fdircmd |= XLNID_FDIRCMD_DROP;
	}

	if (input->formatted.flow_type & XLNID_ATR_L4TYPE_TUNNEL_MASK) {
		fdircmd |= XLNID_FDIRCMD_TUNNEL_FILTER;
	}

	fdircmd |= input->formatted.flow_type << XLNID_FDIRCMD_FLOW_TYPE_SHIFT;
	fdircmd |= (u32) queue << XLNID_FDIRCMD_RX_QUEUE_SHIFT;
	fdircmd |= (u32) input->formatted.vm_pool << XLNID_FDIRCMD_VT_POOL_SHIFT;

	XLNID_WRITE_REG(hw, FDIRCMD, fdircmd);
	err = xlnid_fdir_check_cmd_complete(hw, &fdircmd);

	if (err) {
		hw_dbg(hw, "Flow Director command did not complete!\n");
		return err;
	}

	return XLNID_SUCCESS;
}

s32 xlnid_fdir_remove_perfect_filter_westlake(struct xlnid_hw *hw,
		union xlnid_atr_input *input, u16 soft_id, u8 queue, bool cloud_mode)
{
	u32 fdirport, fdirvlan, fdirhash, fdircmd;
	u32 addr_low, addr_high;
	u32 cloud_type = 0;
	s32 err;

	UNREFERENCED_1PARAMETER(cloud_mode);

	DEBUGFUNC("xlnid_fdir_write_perfect_filter_westlake");

	if (!cloud_mode) {
		/* currently IPv6 is not supported, must be programmed with 0 */
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(0), input->formatted.src_ip[0]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(1), input->formatted.src_ip[1]);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(2), input->formatted.src_ip[2]);

		/* record the source address (big-endian) */
		XLNID_WRITE_REG(hw, FDIRIPSA, input->formatted.src_ip[0]);

		/*  record the first 32 bits of the destination address
			(big-endian) */
		XLNID_WRITE_REG(hw, FDIRIPDA, input->formatted.dst_ip[0]);

		/* record source and destination port (little-endian) */
		fdirport = XLNID_NTOHS(input->formatted.dst_port);
		fdirport <<= XLNID_FDIRPORT_DESTINATION_SHIFT;
		fdirport |= XLNID_NTOHS(input->formatted.src_port);
		XLNID_WRITE_REG(hw, FDIRPORT, fdirport);
	}

	/* record VLAN (little-endian) and flex_bytes(big-endian) */
	fdirvlan = XLNID_STORE_AS_BE16(input->formatted.flex_bytes);
	fdirvlan <<= XLNID_FDIRVLAN_FLEX_SHIFT;
	fdirvlan |= XLNID_NTOHS(input->formatted.vlan_id);
	XLNID_WRITE_REG(hw, FDIRVLAN, fdirvlan);

	if (cloud_mode) {
		if (input->formatted.tunnel_type != 0) {
			cloud_type = 0x80000000;
		}

		addr_low = ((u32) input->formatted.inner_mac[0] |
					((u32) input->formatted.inner_mac[1] << 8) |
					((u32) input->formatted.inner_mac[2] << 16) |
					((u32) input->formatted.inner_mac[3] << 24));
		addr_high = ((u32) input->formatted.inner_mac[4] |
						((u32) input->formatted.inner_mac[5] << 8));
		cloud_type |= addr_high;
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(0), addr_low);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(1), cloud_type);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_FDISIPv6(2), input->formatted.tni_vni);
	}

	/* configure FDIRHASH register */

	fdirhash = input->formatted.bkt_hash;

	fdirhash |= soft_id << XLNID_FDIRHASH_SIG_SW_INDEX_SHIFT;
	fdirhash |= XLNID_FDIRHASH_BUCKET_VALID_ENABLE;
	XLNID_WRITE_REG(hw, FDIRHASH, fdirhash);

	/*
		flush all previous writes to make certain registers are
		programmed prior to issuing the command
	*/
	XLNID_WRITE_FLUSH(hw);

	/* configure FDIRCMD register */
	fdircmd = XLNID_FDIRCMD_CMD_REMOVE_FLOW | XLNID_FDIRCMD_FILTER_UPDATE |
				XLNID_FDIRCMD_LAST | XLNID_FDIRCMD_QUEUE_EN;

	if (queue == XLNID_FDIR_DROP_QUEUE) {
		fdircmd |= XLNID_FDIRCMD_DROP;
	}

	if (input->formatted.flow_type & XLNID_ATR_L4TYPE_TUNNEL_MASK) {
		fdircmd |= XLNID_FDIRCMD_TUNNEL_FILTER;
	}

	fdircmd |= input->formatted.flow_type << XLNID_FDIRCMD_FLOW_TYPE_SHIFT;
	fdircmd |= (u32) queue << XLNID_FDIRCMD_RX_QUEUE_SHIFT;
	fdircmd |= (u32) input->formatted.vm_pool << XLNID_FDIRCMD_VT_POOL_SHIFT;

	XLNID_WRITE_REG(hw, FDIRCMD, fdircmd);
	err = xlnid_fdir_check_cmd_complete(hw, &fdircmd);

	if (err) {
		hw_dbg(hw, "Flow Director command did not complete!\n");
		return err;
	}

	return XLNID_SUCCESS;
}

s32 xlnid_fdir_erase_perfect_filter_westlake(struct xlnid_hw *hw,
		union xlnid_atr_input *input,
		u16 soft_id)
{
	u32 fdirhash;
	u32 fdircmd;
	s32 err;

	/* configure FDIRHASH register */
	fdirhash = input->formatted.bkt_hash;
	fdirhash |= soft_id << XLNID_FDIRHASH_SIG_SW_INDEX_SHIFT;
	XLNID_WRITE_REG(hw, FDIRHASH, fdirhash);

	/* flush hash to HW */
	XLNID_WRITE_FLUSH(hw);

	/* Query if filter is present */
	XLNID_WRITE_REG(hw, FDIRCMD, XLNID_FDIRCMD_CMD_QUERY_REM_FILT);

	err = xlnid_fdir_check_cmd_complete(hw, &fdircmd);

	if (err) {
		hw_dbg(hw, "Flow Director command did not complete!\n");
		return err;
	}

	/* if filter exists in hardware then remove it */
	if (fdircmd & XLNID_FDIRCMD_FILTER_VALID) {
		XLNID_WRITE_REG(hw, FDIRHASH, fdirhash);
		XLNID_WRITE_FLUSH(hw);
		XLNID_WRITE_REG(hw, FDIRCMD, XLNID_FDIRCMD_CMD_REMOVE_FLOW);
	}

	return XLNID_SUCCESS;
}

/**
		xlnid_fdir_add_perfect_filter_westlake - Adds a perfect filter
		@hw: pointer to hardware structure
		@input: input bitstream
		@input_mask: mask for the input bitstream
		@soft_id: software index for the filters
		@queue: queue index to direct traffic to
		@cloud_mode: unused

		Note that the caller to this function must lock before calling, since the
		hardware writes must be protected from one another.
	**/
s32 xlnid_fdir_add_perfect_filter_westlake(struct xlnid_hw *hw,
		union xlnid_atr_input *input,
		union xlnid_atr_input *input_mask,
		u16 soft_id, u8 queue, bool cloud_mode)
{
	s32 err = XLNID_ERR_CONFIG;

	UNREFERENCED_1PARAMETER(cloud_mode);

	DEBUGFUNC("xlnid_fdir_add_perfect_filter_westlake");

	/*
		Check flow_type formatting, and bail out before we touch the hardware
		if there's a configuration issue
	*/
	switch (input->formatted.flow_type) {
	case XLNID_ATR_FLOW_TYPE_IPV4:
	case XLNID_ATR_FLOW_TYPE_TUNNELED_IPV4:
		input_mask->formatted.flow_type = XLNID_ATR_L4TYPE_IPV6_MASK;

		if (input->formatted.dst_port || input->formatted.src_port) {
		hw_dbg(hw, " Error on src/dst port\n");
		return XLNID_ERR_CONFIG;
			}

			break;

	case XLNID_ATR_FLOW_TYPE_SCTPV4:
	case XLNID_ATR_FLOW_TYPE_TUNNELED_SCTPV4:
		if (input->formatted.dst_port || input->formatted.src_port) {
			hw_dbg(hw, " Error on src/dst port\n");
			return XLNID_ERR_CONFIG;
		}

		fallthrough;

	case XLNID_ATR_FLOW_TYPE_TCPV4:
	case XLNID_ATR_FLOW_TYPE_TUNNELED_TCPV4:
	case XLNID_ATR_FLOW_TYPE_UDPV4:
	case XLNID_ATR_FLOW_TYPE_TUNNELED_UDPV4:
		input_mask->formatted.flow_type = XLNID_ATR_L4TYPE_IPV6_MASK |
											XLNID_ATR_L4TYPE_MASK;
		break;

	default:
		hw_dbg(hw, " Error on flow type input\n");
		return err;
	}

	/* program input mask into the HW */
	err = xlnid_fdir_set_input_mask_westlake(hw, input_mask, cloud_mode);

	if (err) {
		return err;
	}

	/* apply mask and compute/store hash */
	xlnid_atr_compute_perfect_hash_westlake(input, input_mask);

	/* program filters to filter memory */
	return xlnid_fdir_write_perfect_filter_westlake(hw, input,
			soft_id, queue, cloud_mode);
}

/**
	xlnid_start_hw_westlake - Prepare hardware for Tx/Rx
	@hw: pointer to hardware structure

	Starts the hardware using the generic start_hw function
	and the generation start_hw function.
	Then performs revision-specific operations, if any.
**/
s32 xlnid_start_hw_westlake(struct xlnid_hw *hw)
{
	s32 ret_val = XLNID_SUCCESS;

	DEBUGFUNC("xlnid_start_hw_westlake");

	ret_val = xlnid_start_hw_generic(hw);

	if (ret_val != XLNID_SUCCESS) {
		goto out;
	}

	/* We need to run link autotry after the driver loads */
	hw->mac.autotry_restart = true;

	if (ret_val == XLNID_SUCCESS) {
		ret_val = xlnid_verify_fw_version_westlake(hw);
	}

out:
	return ret_val;
}

/**
		xlnid_identify_phy_westlake - Get physical layer module
		@hw: pointer to hardware structure

		Determines the physical layer module found on the current adapter.
		If PHY already detected, maintains current PHY type in hw struct,
		otherwise executes the PHY detection routine.
	**/
s32 xlnid_identify_phy_westlake(struct xlnid_hw *hw)
{
	DEBUGFUNC("xlnid_identify_phy_westlake");

	hw->phy.type = xlnid_phy_none;
	hw->phy.addr = WESTLAKE_GEPHY_ADDR;
	return XLNID_SUCCESS;
}

/**
		xlnid_verify_fw_version_westlake - verify FW version for westlake
		@hw: pointer to hardware structure

		Verifies that installed the firmware version is 0.6 or higher
		for SFI devices. All westlake SFI devices should have version 0.6 or higher.

		Returns XLNID_ERR_EEPROM_VERSION if the FW is not present or
		if the FW version is not supported.
	**/
STATIC s32 xlnid_verify_fw_version_westlake(struct xlnid_hw *hw)
{
	return XLNID_SUCCESS;
}

STATIC u32 xlnid_read_eeprom_repeat_westlake(struct xlnid_hw *hw, uint32_t addr)
{
	uint32_t cmd = I2C_CMD_RD | I2C_RD_MODE_4B | I2C_SLAVE_ADDR;

	u32 value = 0;

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_I2C_SLAVE_ADDR, cmd);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_I2C_WORD_ADDR, (addr & I2C_WORD_ADDR_MASK));
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_I2C_SLAVE_ADDR, cmd);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_I2C_MST_EN, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_I2C_MST_EN, 0x0);
	usec_delay(100);

	value = XLNID_READ_REG_MAC(hw, WESTLAKE_I2C_RDATA);

	return value;
}

/*
	STATIC inline void xlnid_read_status_westlake(struct xlnid_hw *hw)
	{
	uint32_t i = 0, value = 1;

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_SSIENR, SPI_DISABLE_DWC_SSI);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_CTRLR0, SPI_CTRLR0_READ);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_CTRLR1, 0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_TXFTLR, 0x00000);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_RXFTLR, 0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_SSIENR, SPI_EABLE_DWC_SSI);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(0), 0x5);

	while((i < 100) && (value & 0x1)) {
		msec_delay(10);
		value = XLNID_READ_REG_MAC(hw, WESTLAKE_SPI_DRX(0));
		i++;
	}
	}
*/


STATIC inline void xlnid_write_addr_spi_westlake(
	struct xlnid_hw *hw, uint32_t bytes, uint32_t addr)
{
	uint32_t i = 0;
	uint32_t reg_addr = 0;

	while (i < bytes) {
		reg_addr = (addr >> (bytes - i - 1) * 8) & 0xff;
		i++;
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(i), reg_addr);
		//printk("%s:[%d]: i=%i, addr=%i", __FILE__, __LINE__, i, reg_addr);
	}

	msec_delay(10);
}

STATIC u32 xlnid_read_eeprom_spi_westlake(struct xlnid_hw *hw, uint32_t addr,
		u8 *value)
{
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_SSIENR, SPI_DISABLE_DWC_SSI);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_CTRLR0, SPI_CTRLR0_READ);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_CTRLR1, 3);
	//XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_TXFTLR, (XLNID_SPI_CONFIG_BYTE_ADDR) << 16);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_TXFTLR, 0x30000);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_RXFTLR, 3);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_SSIENR, SPI_EABLE_DWC_SSI);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(0), 0x3);

#ifdef XLNID_SPI_FLASH
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(1), (addr >> 16) & 0xff);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(2), (addr >> 8) & 0xff);
#else
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(1), (addr >> 8));
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(2), (addr & 0xff));
#endif /* #ifdef XLNID_SPI_FLASH */
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(3), addr & 0xff);
	usec_delay(100);
	//xlnid_write_addr_spi_westlake(hw, bytes, addr); XLNID-274
	value[0] = XLNID_READ_REG_MAC(hw, WESTLAKE_SPI_DRX(0));
	value[1] = XLNID_READ_REG_MAC(hw, WESTLAKE_SPI_DRX(1));
	value[2] = XLNID_READ_REG_MAC(hw, WESTLAKE_SPI_DRX(2));
	value[3] = XLNID_READ_REG_MAC(hw, WESTLAKE_SPI_DRX(3));

	return XLNID_SUCCESS;
}

/**
	xlnid_read_eeprom_buffer_westlake - Read EEPROM word(s) using
	fastest available method

	@hw: pointer to hardware structure
	@offset: offset of  word in EEPROM to read
	@words: number of words
	@data: word(s) read from the EEPROM

	Retrieves 16 bit word(s) read from EEPROM
**/
STATIC s32 xlnid_read_eeprom_buffer_westlake(struct xlnid_hw *hw, u16 offset,
		u16 words, u8 *data)
{
	struct xlnid_eeprom_info *eeprom = &hw->eeprom;
	uint32_t addr = 0;
	u32 i = 0;
	u32 value_i2c = 0;
	u8 value_spi[4];

	if (words == 0) {
		return XLNID_ERR_INVALID_ARGUMENT;
	}

	if (offset >= (hw->eeprom.word_size * 2)) {
		return XLNID_ERR_EEPROM;
	}

	if (eeprom->type == xlnid_eeprom_spi) {
		for (i = 0; i < words; i = i + 4) {
			addr = offset + i;
			xlnid_read_eeprom_spi_westlake(hw, addr, (u8 *) &value_spi);
			data[i] = value_spi[0];
			data[i + 1] = value_spi[1];
			data[i + 2] = value_spi[2];
			data[i + 3] = value_spi[3];
		}
	}

	else if (eeprom->type == xlnid_eeprom_i2c) {
		for (i = 0; i < words; i = i + 4) {
			addr = offset + i;
			value_i2c = xlnid_read_eeprom_repeat_westlake(hw, addr);
			/*  once read record 32bit in value
				data is 8bit so the value needs to shift
				value refers to 4 data
			*/
			data[i] = value_i2c >> 24;
			data[i + 1] = value_i2c >> 16;
			data[i + 2] = value_i2c >> 8;
			data[i + 3] = value_i2c;
		}
	}

	return XLNID_SUCCESS;
}

/**
		xlnid_read_eeprom_westlake - Read EEPROM word using
		fastest available method

		@hw: pointer to hardware structure
		@offset: offset of  word in the EEPROM to read
		@data: word read from the EEPROM

		Reads a 16 bit word from the EEPROM
	**/
STATIC s32 xlnid_read_eeprom_westlake(struct xlnid_hw *hw, u16 offset, u8 *data)
{
	return xlnid_read_eeprom_buffer_westlake(hw, offset, 1, data);
}

STATIC inline void xlnid_write_enable_spi_westlake(struct xlnid_hw *hw)
{
	/* Write Enable(06h) */
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_SSIENR, SPI_DISABLE_DWC_SSI);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_CTRLR0, SPI_CTRLR0_WRITE);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_TXFTLR, 0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_SSIENR, SPI_EABLE_DWC_SSI);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(0), 0x06);
	usec_delay(100);
}

#ifdef XLNID_SPI_FLASH
STATIC inline void xlnid_sector_erase_spi_westlake(struct xlnid_hw *hw)
{

	/* subSector Erase(20h), Sector Erase(d8h) */
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_SSIENR, SPI_DISABLE_DWC_SSI);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_TXFTLR, 0x30000);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_SSIENR, SPI_EABLE_DWC_SSI);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(0), 0x20);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(1), 0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(2), 0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(3), 0);
	msec_delay(XLNID_SPI_CONFIG_TESS);
	printk("%s:[%d]: subSector Erase(20h):addr = 0, byte-addr=%i, tEss=%ims",
			__FILE__, __LINE__, XLNID_SPI_CONFIG_BYTE_ADDR, XLNID_SPI_CONFIG_TESS);

}
#endif /* XLNID_SPI_FLASH */


STATIC inline void xlnid_write_date_spi_westlake(
	struct xlnid_hw *hw, uint32_t bytes, uint32_t addr, uint32_t data)
{
	/* write date(02h), offset  + 1 is WESTLAKE_SPI_DRX(i) numbers */
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_SSIENR, SPI_DISABLE_DWC_SSI);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_TXFTLR, (bytes  + 1) << 16);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_SSIENR, SPI_EABLE_DWC_SSI);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(0), 0x02);

	xlnid_write_addr_spi_westlake(hw, bytes, addr);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_DRX(bytes + 1), data);
	msec_delay(10);
}

STATIC u32 xlnid_write_eeprom_spi_westlake(struct xlnid_hw *hw, uint32_t addr,
		u8 *data)
{
	u32 value = *data;

#ifdef XLNID_SPI_FLASH

	if (addr == 0) {
		xlnid_write_enable_spi_westlake(hw);
		xlnid_sector_erase_spi_westlake(hw);
	}

#else
	addr += 1;
#endif /* XLNID_SPI_FLASH */

	xlnid_write_enable_spi_westlake(hw);
	xlnid_write_date_spi_westlake(hw, XLNID_SPI_CONFIG_BYTE_ADDR, addr, value);

	return XLNID_SUCCESS;

}

STATIC void xlnid_write_eeprom_repeat_westlake(struct xlnid_hw *hw,
		uint32_t addr, u8 *data)
{
	uint32_t cmd = I2C_CMD_WR | I2C_WR_MODE_1B | I2C_SLAVE_ADDR;
	u32 value = 0;

	value = *data;

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_I2C_SLAVE_ADDR, cmd);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_I2C_WORD_ADDR, (addr & I2C_WORD_ADDR_MASK));
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_I2C_WDATA, value);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_I2C_MST_EN, 0x1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_I2C_MST_EN, 0x0);
	msec_delay(10);
	return;
}

/**
		xlnid_write_eeprom_buffer_westlake
		@hw: pointer to hardware structure
		@offset: offset of byte in the EEPROM to write
		@bytes: number of bytes
		@data: byte(s) write to the EEPROM

		Write a 16 bit byte(s) to the EEPROM .
	**/
STATIC s32 xlnid_write_eeprom_buffer_westlake(struct xlnid_hw *hw, u16 offset,
		u16 bytes, u8 *data)
{
	struct xlnid_eeprom_info *eeprom = &hw->eeprom;
	uint32_t addr = 0;
	u16 i = 0;

	if (bytes == 0) {
		return XLNID_ERR_INVALID_ARGUMENT;
	}

	if ((offset + bytes) >= (hw->eeprom.word_size * 2)) {
		return XLNID_ERR_EEPROM;
	}

	if (eeprom->type == xlnid_eeprom_i2c) {
		for (i = 0; i < bytes; i++) {
			addr = offset + i;
			xlnid_write_eeprom_repeat_westlake(hw, addr, &data[i]);
		}
	}

	else if (eeprom->type == xlnid_eeprom_spi) {
		for (i = 0; i < bytes; i++) {
			addr = offset + i;
			xlnid_write_eeprom_spi_westlake(hw, addr, &data[i]);
		}
	}

	return XLNID_SUCCESS;
}

u32 xlnid_read_mac_westlake(struct xlnid_hw *hw, u32 reg)
{
	u32 data = 0;
	u32 cmd = (XLNID_MAC_CMD_RD | (reg & XLNID_MAC_RW_ADDR_MASK));

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MAC_CMD, cmd);

	data = XLNID_READ_REG_MAC(hw, WESTLAKE_MAC_RDATA);
	return data;
}

void xlnid_write_mac_westlake(struct xlnid_hw *hw, u32 reg, u32 data)
{
	u32 cmd = (XLNID_MAC_CMD_WR | (reg & XLNID_MAC_RW_ADDR_MASK));

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MAC_WDATA, data);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_MAC_CMD, cmd);
	return;
}

/**
		xlnid_read_phy_reg_westlake  - Reads a value from a specified PHY register
		@hw: pointer to hardware structure
		@reg_addr: 32 bit address of PHY register to read
		@device_type: 5 bit device type
		@phy_data: Pointer to read data from PHY register
	**/
STATIC s32 xlnid_read_phy_reg_westlake(struct xlnid_hw *hw, u32 reg_addr,
										u32 device_type, u16 *phy_data)
{
	u32 regval = 0;
	bool unbusy = false;
	int i;

	DEBUGFUNC("xlnid_read_phy_reg_westlake");
	UNREFERENCED_1PARAMETER(device_type);

	for (i = 0; i < XLNID_MDIO_WAIT_MAX; i++) {
		regval = XLNID_READ_REG_MAC(hw, (WESTLAKE_INNER_PHY_BASE + WESTLAKE_MDIO_BUSY));

		if (!(regval & XLNID_MDIO_BUSY)) {
			unbusy = true;
			break;
		}

		usec_delay(10);
	}

	if (!unbusy) {
		printk("xlnid_read_phy_reg_westlake: read failed, mdio busy\r\n");
		return XLNID_ERR_PHY;
	}

	regval = XLNID_MDIO_CL22_READ | ((hw->phy.addr & XLNID_MDIO_ADDR_MASK) << 8) |
				(reg_addr & XLNID_MDIO_ADDR_MASK);
	XLNID_WRITE_REG_MAC(hw, (WESTLAKE_INNER_PHY_BASE + WESTLAKE_MDIO_CTL), regval);

	XLNID_WRITE_REG_MAC(hw, (WESTLAKE_INNER_PHY_BASE + WESTLAKE_MDIO_EN), 0x1);
	usec_delay(60);
	XLNID_WRITE_REG_MAC(hw, (WESTLAKE_INNER_PHY_BASE + WESTLAKE_MDIO_EN), 0x0);

	regval = XLNID_READ_REG_MAC(hw,
								(WESTLAKE_INNER_PHY_BASE + WESTLAKE_MDIO_RDATA));
	*phy_data = (u16)regval;

	return XLNID_SUCCESS;
}

/**
		xlnid_write_phy_reg_westlake - Writes a value to specified PHY register
		@hw: pointer to hardware structure
		@reg_addr: 32 bit PHY register to write
		@device_type: 5 bit device type
		@phy_data: Data to write to the PHY register
	**/
STATIC s32 xlnid_write_phy_reg_westlake(struct xlnid_hw *hw, u32 reg_addr,
										u32 device_type, u16 phy_data)
{
	u32 regval = 0;
	bool unbusy = false;
	int i;

	DEBUGFUNC("xlnid_write_phy_reg_westlake");
	UNREFERENCED_1PARAMETER(device_type);

	for (i = 0; i < XLNID_MDIO_WAIT_MAX; i++) {
		regval = XLNID_READ_REG_MAC(hw, (WESTLAKE_INNER_PHY_BASE + WESTLAKE_MDIO_BUSY));

		if (!(regval & XLNID_MDIO_BUSY)) {
			unbusy = true;
			break;
		}

		usec_delay(10);
	}

	if (!unbusy) {
		printk("xlnid_write_gephy_reg: write failed, mdio busy\r\n");
		return XLNID_ERR_PHY;
	}

	XLNID_WRITE_REG_MAC(hw, (WESTLAKE_INNER_PHY_BASE + WESTLAKE_MDIO_WDATA),
						phy_data);

	regval = XLNID_MDIO_CL22_WRITE | ((hw->phy.addr & XLNID_MDIO_ADDR_MASK) << 8)
				| (reg_addr & XLNID_MDIO_ADDR_MASK);
	XLNID_WRITE_REG_MAC(hw, (WESTLAKE_INNER_PHY_BASE + WESTLAKE_MDIO_CTL), regval);

	XLNID_WRITE_REG_MAC(hw, (WESTLAKE_INNER_PHY_BASE + WESTLAKE_MDIO_EN), 0x1);
	usec_delay(60);
	XLNID_WRITE_REG_MAC(hw, (WESTLAKE_INNER_PHY_BASE + WESTLAKE_MDIO_EN), 0x0);

	return XLNID_SUCCESS;
}

/**
		xlnid_get_link_capabilities_westlake - Determines link capabilities
		@hw: pointer to hardware structure
		@speed: pointer to link speed
		@autoneg: boolean auto-negotiation value
	**/
s32 xlnid_get_link_capabilities_westlake(struct xlnid_hw *hw,
		xlnid_link_speed *speed, bool *autoneg)
{
	u32 regval;

	DEBUGFUNC("xlnid_get_link_capabilities_westlake");

	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL);

	if (regval == 0x0) {
		/* gephy */
		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_GEPHY_LED_STATUS);

		if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable && hw->bus.func == 0
				&& hw->ptsw_backup) {
			xel_pci_info_t *slave_pinfo = NULL;

			slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
			regval = XLNID_READ_REG_MAC(slave_pinfo->hw, WESTLAKE_GEPHY_LED_STATUS);
		}

		if (regval & XLNID_PO_FIBER) {
			/* 100basefx */
			*autoneg = true;
			*speed = XLNID_LINK_SPEED_100_FULL;
		}

		else {
			*autoneg = true;
			*speed = WESTLAKE_LINK_SPEED_AUTONEG;
		}
	}

	else if (regval == 0x1) {
		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1);

		if (!(regval & 0x1)) {
			/* 1000basex */
			regval = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_AN_EN);

			if (regval) {
				*autoneg = true;
			}

			else {
				*autoneg = false;
			}

			*speed = XLNID_LINK_SPEED_1GB_FULL;
		}

		else {
			/* sgmii */
			if (hw->phy.media_type == xlnid_media_type_sgmii_mac) {
				*autoneg = true;
			}

			else {
				*autoneg = false;
			}

			*speed = WESTLAKE_LINK_SPEED_AUTONEG;
		}
	}

	else if (regval == 0x2) {
		/* rmii */
		*autoneg = true;
		*speed = WESTLAKE_LINK_SPEED_RMII;
	}

	return XLNID_SUCCESS;
}

static void westlake_check_link_gephy(struct xlnid_hw *hw,
										xlnid_link_speed *speed, bool *link_up, bool link_up_wait_to_complete)
{
	u32 status = 0;
	u16 bmcr = 0;
	u16 bmsr = 0;
	u32 i = 0;
	bool loopback = false;

	status = XLNID_READ_REG_MAC(hw, WESTLAKE_GEPHY_LED_STATUS);

	if (status & XLNID_PO_FIBER) {
		/* 100basefx */
		if (link_up_wait_to_complete) {
			for (i = 0; i < hw->mac.max_link_up_time; i++) {
				if (status & XLNID_PO_LINK) {
					*link_up = true;
					*speed = XLNID_LINK_SPEED_100_FULL;
					break;
				}

				*link_up = false;
				*speed = XLNID_LINK_SPEED_UNKNOWN;
				msec_delay(100);
				status = XLNID_READ_REG_MAC(hw, WESTLAKE_GEPHY_LED_STATUS);
			}
		}

		else {
			if (status & XLNID_PO_LINK) {
				*link_up = true;
				*speed = XLNID_LINK_SPEED_100_FULL;
			}

			else {
				*link_up = false;
				*speed = XLNID_LINK_SPEED_UNKNOWN;
			}
		}

		return;
	}

#ifdef XLNID_COPPER_CROSSOVER_SUPPORT
reaneg:
#endif
	/* clear the old state */
	xlnid_read_phy_reg(hw, MII_BMSR, 0, &bmsr);

	xlnid_read_phy_reg(hw, MII_BMSR, 0, &bmsr);
	xlnid_read_phy_reg(hw, MII_BMCR, 0, &bmcr);

	if (bmcr & BMCR_LOOPBACK) {
		loopback = true;
	}

	if (link_up_wait_to_complete) {
		for (i = 0; i < hw->mac.max_link_up_time; i++) {
			if (((((loopback) || (bmsr & BMSR_ANEGCOMPLETE)) && (bmcr & BMCR_ANENABLE))
					|| ((loopback) || (!(bmcr & BMCR_ANENABLE)))) && (bmsr & BMSR_LSTATUS)) {
				*link_up = true;
				break;
			}

			else {
				*link_up = false;
			}

			msec_delay(100);
			xlnid_read_phy_reg(hw, MII_BMSR, 0, &bmsr);
		}
	}

	else {
		if (((((loopback) || (bmsr & BMSR_ANEGCOMPLETE)) && (bmcr & BMCR_ANENABLE))
				|| ((loopback) || (!(bmcr & BMCR_ANENABLE)))) && (bmsr & BMSR_LSTATUS)) {
			*link_up = true;
		}

		else {
			*link_up = false;
		}
	}

	if (!(*link_up)) {
		*speed = XLNID_LINK_SPEED_UNKNOWN;
		return;
	}

	xlnid_read_phy_reg(hw, MII_BMCR, 0, &bmcr);

	if (bmcr & BMCR_SPEED1000) {
		*speed = XLNID_LINK_SPEED_1GB_FULL;
	}

	else if (bmcr & BMCR_SPEED100) {
		*speed = (bmcr & BMCR_FULLDPLX) ? XLNID_LINK_SPEED_100_FULL :
					XLNID_LINK_SPEED_100_HALF;
	}

	else {
		*speed = (bmcr & BMCR_FULLDPLX) ? XLNID_LINK_SPEED_10_FULL :
					XLNID_LINK_SPEED_10_HALF;
	}

#ifdef XLNID_COPPER_CROSSOVER_SUPPORT

	if (!hw->phy.reautoneg && (bmcr & BMCR_SPEED100) &&
			(hw->phy.autoneg_advertised & XLNID_LINK_SPEED_1GB_FULL)) {
		xlnid_write_phy_reg(hw, MII_BMCR, 0, 0x1200);
		hw->phy.reautoneg = true;
		goto reaneg;
	}

#endif

	return;
}

/**
		xlnid_check_mac_link_westlake - Determine link and speed status
		@hw: pointer to hardware structure
		@speed: pointer to link speed
		@link_up: true when link is up
		@link_up_wait_to_complete: bool used to wait for link up or not

		Reads the links register to determine if link is up and the current speed
	**/
s32 xlnid_check_mac_link_westlake(struct xlnid_hw *hw, xlnid_link_speed *speed,
									bool *link_up, bool link_up_wait_to_complete)
{
	u32 regval;

	DEBUGFUNC("xlnid_check_mac_link_westlake");

#ifdef XLNID_AUTO_MEDIA_SUPPORT

	westlake_check_link_gephy(hw, speed, link_up, link_up_wait_to_complete);
	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_STAS0);

	if (*link_up && (!hw->link_mode || (hw->link_mode
										&& !((regval & XLNID_SGM_PCS_LINK_UP) && (regval & XLNID_SGM_PCS_RX_SYNC))))) {
		hw->phy.media_type = xlnid_media_type_copper;
		hw->phy.multispeed_fiber = 0;

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_LINK_MASK, 0x0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x0);

		/* copper use led1 */
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCTL,
							((XLNID_LED_OFF << XLNID_LED_MODE_SHIFT(0)) | (XLNID_LED_LINK_ACTIVE <<
									XLNID_LED_MODE_SHIFT(1))));
	}

	else {
		if ((regval & XLNID_SGM_PCS_LINK_UP) && (regval & XLNID_SGM_PCS_RX_SYNC)) {
			hw->phy.media_type = xlnid_media_type_fiber;
			hw->phy.multispeed_fiber = 1;

			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_LINK_MASK, 0x2);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL, 0x1);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_SPEED, 0x2);

			/* fiber use led0 */
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCTL,
								((XLNID_LED_OFF << XLNID_LED_MODE_SHIFT(1)) | (XLNID_LED_LINK_ACTIVE <<
										XLNID_LED_MODE_SHIFT(0))));
			*link_up = true;
			*speed = XLNID_LINK_SPEED_1GB_FULL;
		}

		else {
			if (hw->phy.media_type == xlnid_media_type_unknown) {
				if (!hw->link_mode) {
					hw->phy.media_type = xlnid_media_type_copper;
				}

				else {
					hw->phy.media_type = xlnid_media_type_fiber;
				}
			}

			*link_up = false;
			*speed = XLNID_LINK_SPEED_UNKNOWN;
		}
	}

#else

	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL);

	if (regval == 0x0) {
		/* gephy */
		westlake_check_link_gephy(hw, speed, link_up, link_up_wait_to_complete);
		return XLNID_SUCCESS;
	}

	else if (regval == 0x1) {
		u32 an_status = 0;
		u32 sgmii_speed = 0;
		u32 sgmii_duplex = 0;

		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_STAS0);

		if (!(regval & XLNID_SGM_PCS_LINK_UP)) {
			*link_up = false;
			*speed = XLNID_LINK_SPEED_UNKNOWN;
			return XLNID_SUCCESS;
		}

		*link_up = true;

		if (hw->phy.media_type == xlnid_media_type_sgmii_mac) {
			an_status = regval & XLNID_SGM_PCS_AN_STATUS;

			if (an_status == XLNID_SGM_PCS_AN_10_HALF) {
				*speed = XLNID_LINK_SPEED_10_HALF;
			}

			else if (an_status == XLNID_SGM_PCS_AN_10_FULL) {
				*speed = XLNID_LINK_SPEED_10_FULL;
			}

			else if (an_status == XLNID_SGM_PCS_AN_100_HALF) {
				*speed = XLNID_LINK_SPEED_100_HALF;
			}

			else if (an_status == XLNID_SGM_PCS_AN_100_FULL) {
				*speed = XLNID_LINK_SPEED_100_FULL;
			}

			else if (an_status == XLNID_SGM_PCS_AN_1000_HALF) {
				*speed = XLNID_LINK_SPEED_1GB_HALF;
			}

			else if (an_status == XLNID_SGM_PCS_AN_1000_FULL) {
				*speed = XLNID_LINK_SPEED_1GB_FULL;
			}
		}

		else {
			regval = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0);
			sgmii_duplex = regval & XLNID_SGM_DUPLEX;
			sgmii_speed = regval & XLNID_SGM_SPEED;

			if (sgmii_speed == XLNID_SGM_SPEED_10) {
				*speed = (sgmii_duplex) ? XLNID_LINK_SPEED_10_FULL : XLNID_LINK_SPEED_10_HALF;
			}

			else if (sgmii_speed == XLNID_SGM_SPEED_100) {
				*speed = (sgmii_duplex) ? XLNID_LINK_SPEED_100_FULL : XLNID_LINK_SPEED_100_HALF;
			}

			else if (sgmii_speed == XLNID_SGM_SPEED_1000) {
				*speed = (sgmii_duplex) ? XLNID_LINK_SPEED_1GB_FULL : XLNID_LINK_SPEED_1GB_HALF;
			}
		}
	}

	else if (regval == 0x2) {
		/* rmii only support port 0 */
		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_DEBUG_PHYSTATUS);

		if (regval & XLNID_PHYSTATUS_LINK0) {
			*link_up = true;

			if (regval & XLNID_PHYSTATUS_SPEED0) {
				if (regval & XLNID_PHYSTATUS_DUPLEX0) {
					*speed = XLNID_LINK_SPEED_100_FULL;
				}

				else {
					*speed = XLNID_LINK_SPEED_100_HALF;
				}
			}

			else {
				if (regval & XLNID_PHYSTATUS_DUPLEX0) {
					*speed = XLNID_LINK_SPEED_10_FULL;
				}

				else {
					*speed = XLNID_LINK_SPEED_10_HALF;
				}
			}
		}

		else {
			*link_up = false;
			*speed = XLNID_LINK_SPEED_UNKNOWN;
		}
	}

#endif

	return XLNID_SUCCESS;
}

#ifdef XLNID_GEPHY_OPTIMIZE
static s32 westlake_setup_link_gephy(struct xlnid_hw *hw,
										xlnid_link_speed speed, bool autoneg_wait_to_complete)
{
	s32 status = XLNID_SUCCESS;
	u32 led, adv, oldadv, bmcr, bmsr, ctrl, oldctrl;
	u32 i;
	bool anrestart = false;
	xel_pci_info_t *slave_pinfo = NULL;

	hw->phy.autoneg_advertised = 0;

	led = XLNID_READ_REG_MAC(hw, WESTLAKE_GEPHY_LED_STATUS);

	if (led & XLNID_PO_FIBER) {
		/* 100basefx */
		goto out;
	}

	/* Set 10M/100M/1000M advertise */
	ctrl = xlnid_read_gephy_westlake(hw, MII_CTRL1000);
	adv = xlnid_read_gephy_westlake(hw, MII_ADVERTISE);
	bmcr = xlnid_read_gephy_westlake(hw, MII_BMCR);

	if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable && hw->bus.func == 0
			&& hw->ptsw_backup) {
		slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
		bmcr = xlnid_read_gephy_westlake(hw, MII_BMCR);
		adv = xlnid_read_gephy_westlake(slave_pinfo->hw, MII_ADVERTISE);
		ctrl = xlnid_read_gephy_westlake(slave_pinfo->hw, MII_CTRL1000);
	}

	oldadv = adv;
	oldctrl = ctrl;

	if (speed & XLNID_LINK_SPEED_10_HALF) {
		hw->phy.autoneg_advertised |= XLNID_LINK_SPEED_10_HALF;
		ctrl &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
		adv |= ADVERTISE_10HALF;
		adv &= ~(ADVERTISE_100FULL | ADVERTISE_100HALF | ADVERTISE_10FULL);
	}

	if (speed & XLNID_LINK_SPEED_10_FULL) {
		hw->phy.autoneg_advertised |= XLNID_LINK_SPEED_10_FULL;
		ctrl &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
		adv |= (ADVERTISE_10FULL | ADVERTISE_10HALF);
		adv &= ~(ADVERTISE_100FULL | ADVERTISE_100HALF);
	}

	if (speed & XLNID_LINK_SPEED_100_HALF) {
		hw->phy.autoneg_advertised |= XLNID_LINK_SPEED_100_HALF;
		ctrl &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
		adv |= (ADVERTISE_100HALF | ADVERTISE_10FULL | ADVERTISE_10HALF);
		adv &= ~ADVERTISE_100FULL;
	}

	if (speed & XLNID_LINK_SPEED_100_FULL) {
		hw->phy.autoneg_advertised |= XLNID_LINK_SPEED_100_FULL;
		ctrl &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
		adv |= ADVERTISE_ALL;
	}

	if (speed & XLNID_LINK_SPEED_1GB_FULL) {
		hw->phy.autoneg_advertised |= XLNID_LINK_SPEED_1GB_FULL;
		ctrl |= (ADVERTISE_1000FULL | ADVERTISE_1000HALF);
		adv |= ADVERTISE_ALL;
	}

	if ((adv != oldadv) || (ctrl != oldctrl)) {
		if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable && hw->bus.func == 0
				&& hw->ptsw_backup) {
			slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
			xlnid_write_gephy_westlake(slave_pinfo->hw, MII_ADVERTISE, adv);
			xlnid_write_gephy_westlake(hw, MII_CTRL1000, ctrl);
		}

		else {
			xlnid_write_gephy_westlake(hw, MII_ADVERTISE, adv);
			xlnid_write_gephy_westlake(hw, MII_CTRL1000, ctrl);

		}

		anrestart = true;
	}

	if (!anrestart) {
		goto out;
	}

#if 0
	/* Restart autoneg */
	//bmcr = xlnid_read_gephy_westlake(hw, MII_BMCR);
	/*  if (bmcr == XLNID_FAILED_READ_REG)
		{
		//auto-negotiation
		xlnid_write_gephy_westlake(hw, MII_BMCR, 0x1340);
		return -1;
		} */

	//bmcr |= (BMCR_ANENABLE | BMCR_ANRESTART);

	/* Don't isolate the PHY if we-re negotiating */
	//bmcr &= ~BMCR_ISOLATE;
	//bmcr &= ~BMCR_CTST;
#endif
	/* set bit 9&12 of BMCR to enable autoneg and restart auto neg */
	bmcr |= 0x1200;
	xlnid_write_gephy_westlake(hw, MII_BMCR, bmcr);

	/* Only poll for autoneg to complete if specified to do so */
	if (autoneg_wait_to_complete) {
		for (i = 0; i < XLNID_AUTO_NEG_TIME; i++) {
			bmsr = xlnid_read_gephy_westlake(hw, MII_BMSR);

			if (bmsr & BMSR_ANEGCOMPLETE) {
				break;
			}

			msec_delay(100);
		}

		if (!(bmsr & BMSR_ANEGCOMPLETE)) {
			status = XLNID_ERR_AUTONEG_NOT_COMPLETE;
			hw_dbg(hw, "Autoneg did not complete.\n");
		}
	}

	/* Add delay to filter out noises during initial link setup */
	msec_delay(50);
out:
	return status;

}
#else
static s32 westlake_setup_link_gephy(struct xlnid_hw *hw,
										xlnid_link_speed speed, bool autoneg_wait_to_complete)
{
	s32 status = XLNID_SUCCESS;
	u32 led;
	u16 adv, oldadv, bmcr, bmsr;
	u32 i;
	bool anrestart = false;
	xel_pci_info_t *slave_pinfo = NULL;

	hw->phy.autoneg_advertised = 0;

	led = XLNID_READ_REG_MAC(hw, WESTLAKE_GEPHY_LED_STATUS);

	if (led & XLNID_PO_FIBER) {
		/* 100basefx */
		goto out;
	}

	/* Set 10M/100M advertise */
	xlnid_read_phy_reg(hw, MII_ADVERTISE, 0, &adv);

	if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable && hw->bus.func == 0
			&& hw->ptsw_backup) {
		slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
		xlnid_read_phy_reg(slave_pinfo->hw, MII_ADVERTISE, 0, &adv);
	}

	oldadv = adv;

	if (speed & XLNID_LINK_SPEED_10_HALF) {
		adv |= ADVERTISE_10HALF;
		hw->phy.autoneg_advertised |= XLNID_LINK_SPEED_10_HALF;
		bmcr = 0x1200;
	}

	else {
		adv &= ~ADVERTISE_10HALF;
	}

	if (speed & XLNID_LINK_SPEED_10_FULL) {
		adv |= ADVERTISE_10FULL;
		hw->phy.autoneg_advertised |= XLNID_LINK_SPEED_10_FULL;
		bmcr = 0x1300;
	}

	else {
		adv &= ~ADVERTISE_10FULL;
	}

	if (speed & XLNID_LINK_SPEED_100_HALF) {
		adv |= ADVERTISE_100HALF;
		hw->phy.autoneg_advertised |= XLNID_LINK_SPEED_100_HALF;
		bmcr = 0x3200;
	}

	else {
		adv &= ~ADVERTISE_100HALF;
	}

	if (speed & XLNID_LINK_SPEED_100_FULL) {
		adv |= ADVERTISE_100FULL;
		hw->phy.autoneg_advertised |= XLNID_LINK_SPEED_100_FULL;
		bmcr = 0x3300;
	}

	else {
		adv &= ~ADVERTISE_100FULL;
	}

	if (adv != oldadv) {
		xlnid_write_phy_reg(hw, MII_ADVERTISE, 0, adv);
		//xlnid_write_phy_reg(xel_pci_info[1].hw, MII_ADVERTISE, 0, adv);
		anrestart = true;
	}

	/* Set 1000M advertise */
	xlnid_read_phy_reg(hw, MII_CTRL1000, 0, &adv);

	if (hw->mac.type == xlnid_mac_WESTLAKE && hw->ptsw_enable && hw->bus.func == 0
			&& hw->ptsw_backup) {
		xlnid_read_phy_reg(slave_pinfo->hw, MII_CTRL1000, 0, &adv);
	}

	oldadv = adv;

	if (speed & XLNID_LINK_SPEED_1GB_FULL) {
		adv |= (ADVERTISE_1000FULL | ADVERTISE_1000HALF);
		hw->phy.autoneg_advertised |= XLNID_LINK_SPEED_1GB_FULL;
		bmcr = 0x1340;
	}

	else {
		adv &= ~(ADVERTISE_1000FULL | ADVERTISE_1000HALF);
	}

	if (adv != oldadv) {
		xlnid_write_phy_reg(hw, MII_CTRL1000, 0, adv);
		anrestart = true;
	}

	if (!anrestart) {
		goto out;
	}

#if 0
	/* Restart autoneg */
	//xlnid_read_phy_reg(hw, MII_BMCR, 0, &bmcr);
	/*  if (bmcr == XLNID_FAILED_READ_REG)
		{
		//auto-negotiation
		xlnid_write_phy_reg(hw, MII_BMCR, 0, 0x1340);
		return -1;
		} */

	//bmcr |= (BMCR_ANENABLE | BMCR_ANRESTART);

	/* Don't isolate the PHY if we-re negotiating */
	//bmcr &= ~BMCR_ISOLATE;
	//bmcr &= ~BMCR_CTST;
#endif

	xlnid_write_phy_reg(hw, MII_BMCR, 0, bmcr);

	/* Only poll for autoneg to complete if specified to do so */
	if (autoneg_wait_to_complete) {
		for (i = 0; i < XLNID_AUTO_NEG_TIME; i++) {
			xlnid_read_phy_reg(hw, MII_BMSR, 0, &bmsr);

			if (bmsr & BMSR_ANEGCOMPLETE) {
				break;
			}

			msec_delay(100);
		}

		if (!(bmsr & BMSR_ANEGCOMPLETE)) {
			status = XLNID_ERR_AUTONEG_NOT_COMPLETE;
			hw_dbg(hw, "Autoneg did not complete.\n");
		}
	}

	/* Add delay to filter out noises during initial link setup */
	msec_delay(50);

out:
	return status;

}
#endif

/**
		xlnid_setup_mac_link_westlake - Set MAC link speed
		@hw: pointer to hardware structure
		@speed: new link speed
		@autoneg_wait_to_complete: true when waiting for completion is needed

		Set the link speed in the MAC and/or PHY register and restarts link.
	**/
s32 xlnid_setup_mac_link_westlake(struct xlnid_hw *hw, xlnid_link_speed speed,
									bool autoneg_wait_to_complete)
{
	s32 status = XLNID_SUCCESS;
	u32 regval;
	bool autoneg = false;
	xlnid_link_speed link_capabilities = XLNID_LINK_SPEED_UNKNOWN;
	struct xlnid_adapter *adapter = hw->back;

	DEBUGFUNC("xlnid_setup_mac_link_westlake");

	/* Check to see if speed passed in is supported. */
	status = hw->mac.ops.get_link_capabilities(hw, &link_capabilities, &autoneg);

	if (status) {
		goto out;
	}

	speed &= link_capabilities;

	if (speed == XLNID_LINK_SPEED_UNKNOWN) {
		status = XLNID_ERR_LINK_SETUP;
		goto out;
	}

	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_INTF_CTRL);

	if (regval == 0x0) {
		/* gephy */
		status = westlake_setup_link_gephy(hw, speed, autoneg_wait_to_complete);
	}

	if (hw->phy.media_type == xlnid_media_type_sgmii_phy) {
		regval = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0);

		if (speed == XLNID_LINK_SPEED_10_HALF) {
			regval &= ~XLNID_SGM_DUPLEX;
			regval &= ~XLNID_SGM_SPEED;
			regval |= XLNID_SGM_SPEED_10;
			adapter->link_speed = XLNID_LINK_SPEED_10_HALF;
		}

		else if (speed == XLNID_LINK_SPEED_10_FULL) {
			regval |= XLNID_SGM_DUPLEX;
			regval &= ~XLNID_SGM_SPEED;
			regval |= XLNID_SGM_SPEED_10;
			adapter->link_speed = XLNID_LINK_SPEED_10_FULL;
		}

		else if (speed == XLNID_LINK_SPEED_100_HALF) {
			regval &= ~XLNID_SGM_DUPLEX;
			regval &= ~XLNID_SGM_SPEED;
			regval |= XLNID_SGM_SPEED_100;
			adapter->link_speed = XLNID_LINK_SPEED_100_HALF;
		}

		else if (speed == XLNID_LINK_SPEED_100_FULL) {
			regval |= XLNID_SGM_DUPLEX;
			regval &= ~XLNID_SGM_SPEED;
			regval |= XLNID_SGM_SPEED_100;
			adapter->link_speed = XLNID_LINK_SPEED_100_FULL;
		}

		else if (speed == XLNID_LINK_SPEED_1GB_HALF) {
			regval &= ~XLNID_SGM_DUPLEX;
			regval &= ~XLNID_SGM_SPEED;
			regval |= XLNID_SGM_SPEED_1000;
			adapter->link_speed = XLNID_LINK_SPEED_1GB_HALF;
		}

		else if (speed == XLNID_LINK_SPEED_1GB_FULL) {
			regval |= XLNID_SGM_DUPLEX;
			regval &= ~XLNID_SGM_SPEED;
			regval |= XLNID_SGM_SPEED_1000;
			adapter->link_speed = XLNID_LINK_SPEED_1GB_FULL;
		}

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG0, regval);
	}

out:
	return status;
}

/*  Function:
			xlnid_setup_fc_westlake
		Porpose:
			Set up flow control
		Parameters:
			hw    - (IN) pointer to hardware structure
		Returns:
			ret_val           XLNID_SUCCESS
*/
s32 xlnid_setup_fc_westlake(struct xlnid_hw *hw)
{
	u32 reg = 0;
	u16 reg_cu = 0;

	DEBUGFUNC("xlnid_setup_fc_generic");
	/* flow control auto-negotiation register configuration on fiber */
	reg = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1);
	reg |= (XLNID_PCS_CFG1_SYM_PAUSE | XLNID_PCS_CFG1_ASM_PAUSE);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1, reg);

	/* flow control auto-negotiation register configuration on copper */
	xlnid_read_phy_reg(hw, MII_ADVERTISE, 0, &reg_cu);
	reg_cu |= (XLNID_PHY_ANAR_SYM_PAUSE | XLNID_PHY_ANAR_ASM_PAUSE);
	xlnid_write_phy_reg(hw, MII_ADVERTISE, 0, reg_cu);

	return XLNID_SUCCESS;
}

/*  Function:
			xlnid_fc_enable_westlake
		Porpose:
			Enable flow control
		Parameters:
			hw    - (IN) pointer to hardware structure
		Returns:
			ret_val           XLNID_SUCCESS/XLNID_ERR_CONFIG
*/
s32 xlnid_fc_enable_westlake(struct xlnid_hw *hw)
{
	s32 ret_val = XLNID_SUCCESS;

	DEBUGFUNC("xlnid_fc_enable_generic");

	/* Restore default Settings */
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_FC_EN, XLNID_FC_EN);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_RX_BP_MASK, XLNID_RX_BP_MASK_DEFAULT);

	if (hw->bus.func == 0) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_RX_PBUF_ALF_POINT,
							XLNID_RX_PBUF_ALF_POINT_DEFAULT0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_RX_PDBUF_ALF_POINT,
							XLNID_RX_PDBUF_ALF_POINT_DEFAULT0);
	}

	else if (hw->bus.func == 1) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_RX_PBUF_ALF_POINT,
							XLNID_RX_PBUF_ALF_POINT_DEFAULT1);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_RX_PDBUF_ALF_POINT,
							XLNID_RX_PDBUF_ALF_POINT_DEFAULT1);
	}

	/* Negotiate the fc mode to use */
	hw->mac.ops.fc_autoneg(hw);

	/*
		The possible values of fc.current_mode are:
		0: Flow control is completely disabled
		1: Rx flow control is enabled (we can receive pause frames,
			but not send pause frames).
		2: Tx flow control is enabled (we can send pause frames but
			we do not support receiving pause frames).
		3: Both Rx and Tx flow control (symmetric) are enabled.
		other: Invalid.
	*/
	switch (hw->fc.current_mode) {
	case xlnid_fc_none:
		break;

	case xlnid_fc_rx_pause:
		break;

	case xlnid_fc_tx_pause:
		break;

	case xlnid_fc_full:
		/* Flow control (both Rx and Tx) is enabled by SW override. */
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_RX_BP_MASK, XLNID_RX_BP_MASK);

		if (hw->bus.func == 0) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_RX_PBUF_ALF_POINT, XLNID_RX_PBUF_ALF_POINT0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_RX_PDBUF_ALF_POINT, XLNID_RX_PDBUF_ALF_POINT0);
		}

		else if (hw->bus.func == 1) {
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_RX_PBUF_ALF_POINT, XLNID_RX_PBUF_ALF_POINT1);
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_RX_PDBUF_ALF_POINT, XLNID_RX_PDBUF_ALF_POINT1);
		}

		break;

	default:
		ERROR_REPORT1(XLNID_ERROR_ARGUMENT, "Flow control param set incorrectly\n");
		ret_val = XLNID_ERR_CONFIG;
		goto out;
		break;
	}

out:
	return ret_val;
}

/*  Function:
	xlnid_negotiate_fc_westlake
Porpose:
	Negotiate flow control
Parameters:
	hw         - (IN) pointer to hardware structure
	adv_reg    - (IN) flow control advertised settings
	lp_reg     - (IN) link partner's flow control settings
	adv_sym    - (IN) symmetric pause bit in advertisement
	adv_asm    - (IN) asymmetric pause bit in advertisement
	lp_sym     - (IN) symmetric pause bit in link partner advertisement
	lp_asm)    - (IN) asymmetric pause bit in link partner advertisement
Returns:
	XLNID_ERR_FC_NOT_NEGOTIATED    settings are NULL
	XLNID_SUCCESS                  negotiate success
*/
s32 xlnid_negotiate_fc_westlake(struct xlnid_hw *hw, u32 adv_reg, u32 lp_reg,
								u32 adv_sym, u32 adv_asm, u32 lp_sym, u32 lp_asm)
{
	if ((!(adv_reg)) || (!(lp_reg))) {
		ERROR_REPORT3(XLNID_ERROR_UNSUPPORTED,
						"Local or link partner's advertised flow control "
						"settings are NULL. Local: %x, link partner: %x\n", adv_reg, lp_reg);
		return XLNID_ERR_FC_NOT_NEGOTIATED;
	}

	if ((adv_reg & adv_sym) && (lp_reg & lp_sym)) {
		/*
			Now we need to check if the user selected Rx ONLY
			of pause frames.  In this case, we had to advertise
			FULL flow control because we could not advertise RX
			ONLY. Hence, we must now check to see if we need to
			turn OFF the TRANSMISSION of PAUSE frames.
		*/
		if (hw->fc.requested_mode == xlnid_fc_full) {
			hw->fc.current_mode = xlnid_fc_full;
			hw_dbg(hw, "Flow Control = FULL.\n");
		}

		else {
			hw->fc.current_mode = xlnid_fc_rx_pause;
			hw_dbg(hw, "Flow Control = RX PAUSE frames only\n");
		}
	}

	else if (!(adv_reg & adv_sym) && (adv_reg & adv_asm) && (lp_reg & lp_sym)
				&& (lp_reg & lp_asm)) {
		hw->fc.current_mode = xlnid_fc_tx_pause;
		hw_dbg(hw, "Flow Control = TX PAUSE frames only.\n");
	}

	else if ((adv_reg & adv_sym) && (adv_reg & adv_asm) && !(lp_reg & lp_sym)
				&& (lp_reg & lp_asm)) {
		hw->fc.current_mode = xlnid_fc_rx_pause;
		hw_dbg(hw, "Flow Control = RX PAUSE frames only.\n");
	}

	else {
		hw->fc.current_mode = xlnid_fc_none;
		hw_dbg(hw, "Flow Control = NONE.\n");
	}

	return XLNID_SUCCESS;
}

/*
		Function:
			xlnid_fc_autoneg_fiber
		Porpose:
			Enable flow control on fiber
		Parameters:
			hw    - (IN) pointer to hardware structure
		Returns:
			negotiation result
*/
STATIC s32 xlnid_fc_autoneg_fiber(struct xlnid_hw *hw)
{
	u32 pcs_cfg1_reg;
	u32 pcs_stas1_reg;
	u32 linkstat;
	s32 ret_val = XLNID_ERR_FC_NOT_NEGOTIATED;

	linkstat = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_STAS0);

	if (!!(linkstat & XLNID_SGM_PCS_AN_COMPLETE) == 0) {
		hw_dbg(hw, "Auto-Negotiation did not complete\n");
		goto out;
	}

	pcs_cfg1_reg = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_CFG1);
	pcs_stas1_reg = XLNID_READ_REG_MAC(hw, WESTLAKE_SGM_PCS_STAS1);

	ret_val = xlnid_negotiate_fc_westlake(hw, pcs_cfg1_reg, pcs_stas1_reg,
											XLNID_PCS_CFG1_SYM_PAUSE, XLNID_PCS_CFG1_ASM_PAUSE, XLNID_PCS_STAS1_SYM_PAUSE,
											XLNID_PCS_STAS1_ASM_PAUSE);
out:
	return ret_val;
}

/*
		Function:
			xlnid_fc_autoneg_copper
		Porpose:
			Enable flow control on copper
		Parameters:
			hw    - (IN) pointer to hardware structure
		Returns:
			negotiation result

*/
STATIC s32 xlnid_fc_autoneg_copper(struct xlnid_hw *hw)
{
	u16 ability_reg = 0;
	u16 lp_ability_reg = 0;

	xlnid_read_phy_reg(hw, MII_ADVERTISE, 0, &ability_reg);
	xlnid_read_phy_reg(hw, MII_LPA, 0, &lp_ability_reg);

	return xlnid_negotiate_fc_westlake(hw, ability_reg, lp_ability_reg,
										XLNID_PHY_ANAR_SYM_PAUSE, XLNID_PHY_ANAR_ASM_PAUSE, XLNID_PHY_ANLPAR_SYM_PAUSE,
										XLNID_PHY_ANLPAR_ASM_PAUSE);
}

/*
		Function:
			xlnid_fc_autoneg_westlake
		Porpose:
			Get local and peer flow control Settings
		Parameters:
			hw    - (IN) pointer to hardware structure
		Returns:
			void
*/
void xlnid_fc_autoneg_westlake(struct xlnid_hw *hw)
{
	s32 ret_val = XLNID_ERR_FC_NOT_NEGOTIATED;
	xlnid_link_speed speed;
	bool link_up;

	DEBUGFUNC("xlnid_fc_autoneg");

	/*
		AN should have completed when the cable was plugged in.
		Look for reasons to bail out.  Bail out if:
		- FC autoneg is disabled, or if
		- link is not up.
	*/
	if (hw->fc.disable_fc_autoneg) {
		/* TODO: This should be just an informative log */
		ERROR_REPORT1(XLNID_ERROR_CAUTION, "Flow control autoneg is disabled");
		goto out;
	}

	hw->mac.ops.check_link(hw, &speed, &link_up, false);

	if (!link_up) {
		ERROR_REPORT1(XLNID_ERROR_SOFTWARE, "The link is down");
		goto out;
	}

	switch (hw->phy.media_type) {
	case xlnid_media_type_fiber:
		ret_val = xlnid_fc_autoneg_fiber(hw);;
		break;

	case xlnid_media_type_copper:
		ret_val = xlnid_fc_autoneg_copper(hw);
		break;

	default:
		break;
	}

out:

	if (ret_val == XLNID_SUCCESS) {
		hw->fc.fc_was_autonegged = true;
	}

	else {
		hw->fc.fc_was_autonegged = false;
		hw->fc.current_mode = hw->fc.requested_mode;
	}
}

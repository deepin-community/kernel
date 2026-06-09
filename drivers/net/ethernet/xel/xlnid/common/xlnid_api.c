// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2008 - 2023 Xel Technology. */

#include "xlnid_api.h"
#include "xlnid_common.h"
#include "xlnid_westlake.h"

//#define XLNID_EMPTY_PARAM
//
//static const u32 xlnid_mvals_base[XLNID_MVALS_IDX_LIMIT] = {
//  XLNID_MVALS_INIT(XLNID_EMPTY_PARAM)
//};

/**
		xlnid_dcb_get_rtrup2tc - read rtrup2tc reg
		@hw: pointer to hardware structure
		@map: pointer to u8 arr for returning map

		Read the rtrup2tc HW register and resolve its content into map
	**/
void xlnid_dcb_get_rtrup2tc(struct xlnid_hw *hw, u8 *map)
{
	if (hw->mac.ops.get_rtrup2tc) {
		hw->mac.ops.get_rtrup2tc(hw, map);
	}
}

/**
		xlnid_init_shared_code - Initialize the shared code
		@hw: pointer to hardware structure

		This will assign function pointers and assign the MAC type and PHY code.
		Does not touch the hardware. This function must be called prior to any
		other function in the shared code. The xlnid_hw structure should be
		memset to 0 prior to calling this function.  The following fields in
		hw structure should be filled in prior to calling this function:
		hw_addr, back, device_id, vendor_id, subsystem_device_id,
		subsystem_vendor_id, and revision_id
	**/
s32 xlnid_init_shared_code(struct xlnid_hw *hw)
{
	s32 status;

	DEBUGFUNC("xlnid_init_shared_code");

	/*
		Set the mac type
	*/
	xlnid_set_mac_type(hw, true);

	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		status = xlnid_init_ops_westlake(hw);
		break;

	default:
		status = XLNID_ERR_DEVICE_NOT_SUPPORTED;
		break;
	}

	hw->mac.max_link_up_time = XLNID_LINK_UP_TIME;

	return status;
}

/**
		xlnid_set_mac_type - Sets MAC type
		@hw: pointer to the HW structure
		@report: if true output error report

		This function sets the mac type of the adapter based on the
		vendor ID and device ID stored in the hw structure.
	**/
s32 xlnid_set_mac_type(struct xlnid_hw *hw, bool report)
{
	s32 ret_val = XLNID_SUCCESS;

	//DEBUGFUNC("xlnid_set_mac_type\n");

	if (hw->vendor_id != XLNID_VENDOR_ID_XEL) {

		ERROR_REPORT2(XLNID_ERROR_UNSUPPORTED, "Unsupported vendor id: %x",
						hw->vendor_id);
		return XLNID_ERR_DEVICE_NOT_SUPPORTED;
	}

	switch (hw->device_id) {
	case XLNID_DEV_ID_WESTLAKE:
	case XLNID_DEV_ID_SGMII_MAC:
	case XLNID_DEV_ID_1000BASEX:
	case XLNID_DEV_ID_SGMII_PHY:
		hw->mac.type = xlnid_mac_WESTLAKE;
		break;

	default:
		ret_val = XLNID_ERR_DEVICE_NOT_SUPPORTED;

		if (report) {
		ERROR_REPORT2(XLNID_ERROR_UNSUPPORTED, "Unsupported device id: %x",
		hw->device_id);
		}

		break;
	}

	//DEBUGOUT2("xlnid_set_mac_type found mac: %d, returns: %d\n", __FUNC__, hw->mac.type, ret_val);
	return ret_val;
}

s32 xlnid_reinit_fdir_tables(struct xlnid_hw *hw)
{
	s32 status = XLNID_SUCCESS;

	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		status = xlnid_reinit_fdir_tables_westlake(hw);
		break;

	default:
		status = XLNID_NOT_IMPLEMENTED;
		break;
	}

	return status;
}

s32 xlnid_init_fdir_signature(struct xlnid_hw *hw, u32 fdirctrl)
{
	s32 status = XLNID_SUCCESS;

	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		status = xlnid_init_fdir_signature_westlake(hw, fdirctrl);
		break;

	default:
		status = XLNID_NOT_IMPLEMENTED;
		break;
	}

	return status;
}

s32 xlnid_init_fdir_perfect(struct xlnid_hw *hw, u32 fdirctrl, bool cloud_mode)
{
	s32 status = XLNID_SUCCESS;

	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		status = xlnid_init_fdir_perfect_westlake(hw, fdirctrl, cloud_mode);
		break;

	default:
		status = XLNID_NOT_IMPLEMENTED;
		break;
	}

	return status;
}

void xlnid_fdir_add_signature_filter(struct xlnid_hw *hw,
										union xlnid_atr_hash_dword input, union xlnid_atr_hash_dword common, u8 queue)
{
	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		xlnid_fdir_add_signature_filter_westlake(hw, input, common, queue);
		break;

	default:
		break;
	}
}

s32 xlnid_fdir_set_input_mask(struct xlnid_hw *hw,
								union xlnid_atr_input *input_mask, bool cloud_mode)
{
	s32 status = XLNID_SUCCESS;

	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		status = xlnid_fdir_set_input_mask_westlake(hw, input_mask, cloud_mode);
		break;

	default:
		status = XLNID_NOT_IMPLEMENTED;
		break;
	}

	return status;
}

s32 xlnid_fdir_write_perfect_filter(struct xlnid_hw *hw,
									union xlnid_atr_input *input, u16 soft_id, u8 queue, bool cloud_mode)
{
	s32 status = XLNID_SUCCESS;

	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		status = xlnid_fdir_write_perfect_filter_westlake(hw, input, soft_id, queue,
		cloud_mode);
		break;

	default:
		status = XLNID_NOT_IMPLEMENTED;
		break;
	}

	return status;
}

s32 xlnid_fdir_remove_perfect_filter(struct xlnid_hw *hw,
										union xlnid_atr_input *input, u16 soft_id, u8 queue, bool cloud_mode)
{
	s32 status = XLNID_SUCCESS;

	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		status = xlnid_fdir_remove_perfect_filter_westlake(hw, input, soft_id, queue,
		cloud_mode);
		break;

	default:
		status = XLNID_NOT_IMPLEMENTED;
		break;
	}

	return status;
}

s32 xlnid_fdir_erase_perfect_filter(struct xlnid_hw *hw,
									union xlnid_atr_input *input, u16 soft_id)
{
	s32 status = XLNID_SUCCESS;

	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		status = xlnid_fdir_erase_perfect_filter_westlake(hw, input, soft_id);
		break;

	default:
		status = XLNID_NOT_IMPLEMENTED;
		break;
	}

	return status;
}

s32 xlnid_fdir_add_perfect_filter(struct xlnid_hw *hw,
									union xlnid_atr_input *input, union xlnid_atr_input *mask, u16 soft_id,
									u8 queue, bool cloud_mode)
{
	s32 status = XLNID_SUCCESS;

	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		status = xlnid_fdir_add_perfect_filter_westlake(hw, input, mask, soft_id, queue,
		cloud_mode);
		break;

	default:
		status = XLNID_NOT_IMPLEMENTED;
		break;
	}

	return status;
}

void xlnid_atr_compute_perfect_hash(struct xlnid_hw *hw,
									union xlnid_atr_input *input, union xlnid_atr_input *mask)
{
	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		xlnid_atr_compute_perfect_hash_westlake(input, mask);
		break;

	default:
		break;
	}
}

u32 xlnid_atr_compute_sig_hash(struct xlnid_hw *hw,
								union xlnid_atr_hash_dword input, union xlnid_atr_hash_dword common)
{
	u32 hash = 0x0;

	switch (hw->mac.type) {
	case xlnid_mac_WESTLAKE:
		hash = xlnid_atr_compute_sig_hash_westlake(input, common);
		break;

	default:
		break;
	}

	return hash;
}

/**
		xlnid_init_hw - Initialize the hardware
		@hw: pointer to hardware structure

		Initialize the hardware by resetting and then starting the hardware
	**/
s32 xlnid_init_hw(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.init_hw, (hw), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_reset_hw - Performs a hardware reset
		@hw: pointer to hardware structure

		Resets the hardware by resetting the transmit and receive units, masks and
		clears all interrupts, performs a PHY reset, and performs a MAC reset
	**/
s32 xlnid_reset_hw(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.reset_hw, (hw), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_start_hw - Prepares hardware for Rx/Tx
		@hw: pointer to hardware structure

		Starts the hardware by filling the bus info structure and media type,
		clears all on chip counters, initializes receive address registers,
		multicast table, VLAN filter table, calls routine to setup link and
		flow control settings, and leaves transmit and receive units disabled
		and uninitialized.
	**/
s32 xlnid_start_hw(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.start_hw, (hw), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_clear_hw_cntrs - Clear hardware counters
		@hw: pointer to hardware structure

		Clears all hardware statistics counters by reading them from the hardware
		Statistics counters are clear on read.
	**/
s32 xlnid_clear_hw_cntrs(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.clear_hw_cntrs, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_get_media_type - Get media type
		@hw: pointer to hardware structure

		Returns the media type (fiber, copper, backplane)
	**/
enum xlnid_media_type xlnid_get_media_type(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.get_media_type, (hw),
							xlnid_media_type_unknown);
}

/**
		xlnid_get_mac_addr - Get MAC address
		@hw: pointer to hardware structure
		@mac_addr: Adapter MAC address

		Reads the adapter's MAC address from the first Receive Address Register
		(RAR0) A reset of the adapter must have been performed prior to calling
		this function in order for the MAC address to have been loaded from the
		EEPROM into RAR0
	**/
s32 xlnid_get_mac_addr(struct xlnid_hw *hw, u8 *mac_addr)
{
	return xlnid_call_func(hw, hw->mac.ops.get_mac_addr, (hw, mac_addr),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_get_device_caps - Get additional device capabilities
		@hw: pointer to hardware structure
		@device_caps: the EEPROM word for device capabilities

		Reads the extra device capabilities from the EEPROM
	**/
s32 xlnid_get_device_caps(struct xlnid_hw *hw, u16 *device_caps)
{
	return xlnid_call_func(hw, hw->mac.ops.get_device_caps, (hw, device_caps),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_get_wwn_prefix - Get alternative WWNN/WWPN prefix from the EEPROM
		@hw: pointer to hardware structure
		@wwnn_prefix: the alternative WWNN prefix
		@wwpn_prefix: the alternative WWPN prefix

		This function will read the EEPROM from the alternative SAN MAC address
		block to check the support for the alternative WWNN/WWPN prefix support.
	**/
s32 xlnid_get_wwn_prefix(struct xlnid_hw *hw, u16 *wwnn_prefix,
							u16 *wwpn_prefix)
{
	return xlnid_call_func(hw, hw->mac.ops.get_wwn_prefix, (hw, wwnn_prefix,
							wwpn_prefix), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_get_fcoe_boot_status -  Get FCOE boot status from EEPROM
		@hw: pointer to hardware structure
		@bs: the fcoe boot status

		This function will read the FCOE boot status from the iSCSI FCOE block
	**/
s32 xlnid_get_fcoe_boot_status(struct xlnid_hw *hw, u16 *bs)
{
	return xlnid_call_func(hw, hw->mac.ops.get_fcoe_boot_status, (hw, bs),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_get_bus_info - Set PCI bus info
		@hw: pointer to hardware structure

		Sets the PCI bus info (speed, width, type) within the xlnid_hw structure
	**/
s32 xlnid_get_bus_info(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.get_bus_info, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_get_num_of_tx_queues - Get Tx queues
		@hw: pointer to hardware structure

		Returns the number of transmit queues for the given adapter.
	**/
u32 xlnid_get_num_of_tx_queues(struct xlnid_hw *hw)
{
	return hw->mac.max_tx_queues;
}

/**
		xlnid_get_num_of_rx_queues - Get Rx queues
		@hw: pointer to hardware structure

		Returns the number of receive queues for the given adapter.
	**/
u32 xlnid_get_num_of_rx_queues(struct xlnid_hw *hw)
{
	return hw->mac.max_rx_queues;
}

/**
		xlnid_stop_adapter - Disable Rx/Tx units
		@hw: pointer to hardware structure

		Sets the adapter_stopped flag within xlnid_hw struct. Clears interrupts,
		disables transmit and receive units. The adapter_stopped flag is used by
		the shared code and drivers to determine if the adapter is in a stopped
		state and should not touch the hardware.
	**/
s32 xlnid_stop_adapter(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.stop_adapter, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_identify_phy - Get PHY type
		@hw: pointer to hardware structure

		Determines the physical layer module found on the current adapter.
	**/
s32 xlnid_identify_phy(struct xlnid_hw *hw)
{
	s32 status = XLNID_SUCCESS;

	if (hw->phy.type == xlnid_phy_unknown) {
		status = xlnid_call_func(hw, hw->phy.ops.identify, (hw),
									XLNID_NOT_IMPLEMENTED);
	}

	return status;
}

/**
		xlnid_reset_phy - Perform a PHY reset
		@hw: pointer to hardware structure
	**/
s32 xlnid_reset_phy(struct xlnid_hw *hw)
{
	s32 status = XLNID_SUCCESS;

	if (hw->phy.type == xlnid_phy_unknown) {
		if (xlnid_identify_phy(hw) != XLNID_SUCCESS) {
			status = XLNID_ERR_PHY;
		}
	}

	if (status == XLNID_SUCCESS) {
		status = xlnid_call_func(hw, hw->phy.ops.reset, (hw), XLNID_NOT_IMPLEMENTED);
	}

	return status;
}

/**
		xlnid_get_phy_firmware_version -
		@hw: pointer to hardware structure
		@firmware_version: pointer to firmware version
	**/
s32 xlnid_get_phy_firmware_version(struct xlnid_hw *hw, u16 *firmware_version)
{
	s32 status = XLNID_SUCCESS;

	status = xlnid_call_func(hw, hw->phy.ops.get_firmware_version, (hw,
								firmware_version), XLNID_NOT_IMPLEMENTED);
	return status;
}

/**
		xlnid_read_phy_reg - Read PHY register
		@hw: pointer to hardware structure
		@reg_addr: 32 bit address of PHY register to read
		@device_type: type of device you want to communicate with
		@phy_data: Pointer to read data from PHY register

		Reads a value from a specified PHY register
	**/
s32 xlnid_read_phy_reg(struct xlnid_hw *hw, u32 reg_addr, u32 device_type,
						u16 *phy_data)
{
	if (hw->phy.id == 0) {
		xlnid_identify_phy(hw);
	}

	return xlnid_call_func(hw, hw->phy.ops.read_reg, (hw, reg_addr, device_type,
							phy_data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_write_phy_reg - Write PHY register
		@hw: pointer to hardware structure
		@reg_addr: 32 bit PHY register to write
		@device_type: type of device you want to communicate with
		@phy_data: Data to write to the PHY register

		Writes a value to specified PHY register
	**/
s32 xlnid_write_phy_reg(struct xlnid_hw *hw, u32 reg_addr, u32 device_type,
						u16 phy_data)
{
	if (hw->phy.id == 0) {
		xlnid_identify_phy(hw);
	}

	return xlnid_call_func(hw, hw->phy.ops.write_reg, (hw, reg_addr, device_type,
							phy_data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_setup_phy_link - Restart PHY autoneg
		@hw: pointer to hardware structure

		Restart autonegotiation and PHY and waits for completion.
	**/
s32 xlnid_setup_phy_link(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->phy.ops.setup_link, (hw), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_setup_internal_phy - Configure integrated PHY
		@hw: pointer to hardware structure

		Reconfigure the integrated PHY in order to enable talk to the external PHY.
		Returns success if not implemented, since nothing needs to be done in this
		case.
*/
s32 xlnid_setup_internal_phy(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->phy.ops.setup_internal_link, (hw),
							XLNID_SUCCESS);
}

/**
		xlnid_check_phy_link - Determine link and speed status
		@hw: pointer to hardware structure
		@speed: link speed
		@link_up: true when link is up

		Reads a PHY register to determine if link is up and the current speed for
		the PHY.
	**/
s32 xlnid_check_phy_link(struct xlnid_hw *hw, xlnid_link_speed *speed,
							bool *link_up)
{
	return xlnid_call_func(hw, hw->phy.ops.check_link, (hw, speed, link_up),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_setup_phy_link_speed - Set auto advertise
		@hw: pointer to hardware structure
		@speed: new link speed
		@autoneg_wait_to_complete: true when waiting for completion is needed

		Sets the auto advertised capabilities
	**/
s32 xlnid_setup_phy_link_speed(struct xlnid_hw *hw, xlnid_link_speed speed,
								bool autoneg_wait_to_complete)
{
	return xlnid_call_func(hw, hw->phy.ops.setup_link_speed, (hw, speed,
							autoneg_wait_to_complete), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_set_phy_power - Control the phy power state
		@hw: pointer to hardware structure
		@on: true for on, false for off
*/
s32 xlnid_set_phy_power(struct xlnid_hw *hw, bool on)
{
	return xlnid_call_func(hw, hw->phy.ops.set_phy_power, (hw, on),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_check_link - Get link and speed status
		@hw: pointer to hardware structure
		@speed: pointer to link speed
		@link_up: true when link is up
		@link_up_wait_to_complete: bool used to wait for link up or not

		Reads the links register to determine if link is up and the current speed
	**/
s32 xlnid_check_link(struct xlnid_hw *hw, xlnid_link_speed *speed,
						bool *link_up, bool link_up_wait_to_complete)
{
	return xlnid_call_func(hw, hw->mac.ops.check_link, (hw, speed, link_up,
							link_up_wait_to_complete), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_disable_tx_laser - Disable Tx laser
		@hw: pointer to hardware structure

		If the driver needs to disable the laser on SFI optics.
	**/
void xlnid_disable_tx_laser(struct xlnid_hw *hw)
{
	if (hw->mac.ops.disable_tx_laser) {
		hw->mac.ops.disable_tx_laser(hw);
	}
}

/**
		xlnid_enable_tx_laser - Enable Tx laser
		@hw: pointer to hardware structure

		If the driver needs to enable the laser on SFI optics.
	**/
void xlnid_enable_tx_laser(struct xlnid_hw *hw)
{
	if (hw->mac.ops.enable_tx_laser) {
		hw->mac.ops.enable_tx_laser(hw);
	}
}

/**
		xlnid_flap_tx_laser - flap Tx laser to start autotry process
		@hw: pointer to hardware structure

		When the driver changes the link speeds that it can support then
		flap the tx laser to alert the link partner to start autotry
		process on its end.
	**/
void xlnid_flap_tx_laser(struct xlnid_hw *hw)
{
	if (hw->mac.ops.flap_tx_laser) {
		hw->mac.ops.flap_tx_laser(hw);
	}
}

/**
		xlnid_setup_link - Set link speed
		@hw: pointer to hardware structure
		@speed: new link speed
		@autoneg_wait_to_complete: true when waiting for completion is needed

		Configures link settings.  Restarts the link.
		Performs autonegotiation if needed.
	**/
s32 xlnid_setup_link(struct xlnid_hw *hw, xlnid_link_speed speed,
						bool autoneg_wait_to_complete)
{
	return xlnid_call_func(hw, hw->mac.ops.setup_link, (hw, speed,
							autoneg_wait_to_complete), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_setup_mac_link - Set link speed
		@hw: pointer to hardware structure
		@speed: new link speed
		@autoneg_wait_to_complete: true when waiting for completion is needed

		Configures link settings.  Restarts the link.
		Performs autonegotiation if needed.
	**/
s32 xlnid_setup_mac_link(struct xlnid_hw *hw, xlnid_link_speed speed,
							bool autoneg_wait_to_complete)
{
	return xlnid_call_func(hw, hw->mac.ops.setup_mac_link, (hw, speed,
							autoneg_wait_to_complete), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_get_link_capabilities - Returns link capabilities
		@hw: pointer to hardware structure
		@speed: link speed capabilities
		@autoneg: true when autoneg or autotry is enabled

		Determines the link capabilities of the current configuration.
	**/
s32 xlnid_get_link_capabilities(struct xlnid_hw *hw, xlnid_link_speed *speed,
								bool *autoneg)
{
	return xlnid_call_func(hw, hw->mac.ops.get_link_capabilities, (hw, speed,
							autoneg), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_led_on - Turn on LEDs
		@hw: pointer to hardware structure
		@index: led number to turn on

		Turns on the software controllable LEDs.
	**/
s32 xlnid_led_on(struct xlnid_hw *hw, u32 index)
{
	return xlnid_call_func(hw, hw->mac.ops.led_on, (hw, index),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_led_off - Turn off LEDs
		@hw: pointer to hardware structure
		@index: led number to turn off

		Turns off the software controllable LEDs.
	**/
s32 xlnid_led_off(struct xlnid_hw *hw, u32 index)
{
	return xlnid_call_func(hw, hw->mac.ops.led_off, (hw, index),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_blink_led_start - Blink LEDs
		@hw: pointer to hardware structure
		@index: led number to blink

		Blink LED based on index.
	**/
s32 xlnid_blink_led_start(struct xlnid_hw *hw, u32 index)
{
	return xlnid_call_func(hw, hw->mac.ops.blink_led_start, (hw, index),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_blink_led_stop - Stop blinking LEDs
		@hw: pointer to hardware structure
		@index: led number to stop

		Stop blinking LED based on index.
	**/
s32 xlnid_blink_led_stop(struct xlnid_hw *hw, u32 index)
{
	return xlnid_call_func(hw, hw->mac.ops.blink_led_stop, (hw, index),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_init_eeprom_params - Initialize EEPROM parameters
		@hw: pointer to hardware structure

		Initializes the EEPROM parameters xlnid_eeprom_info within the
		xlnid_hw struct in order to set up EEPROM access.
	**/
s32 xlnid_init_eeprom_params(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->eeprom.ops.init_params, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_write_eeprom - Write word to EEPROM
		@hw: pointer to hardware structure
		@offset: offset within the EEPROM to be written to
		@data: 16 bit word to be written to the EEPROM

		Writes 16 bit value to EEPROM. If xlnid_eeprom_update_checksum is not
		called after this function, the EEPROM will most likely contain an
		invalid checksum.
	**/
s32 xlnid_write_eeprom(struct xlnid_hw *hw, u16 offset, u16 data)
{
	return xlnid_call_func(hw, hw->eeprom.ops.write, (hw, offset, data),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_write_eeprom_buffer - Write word(s) to EEPROM
		@hw: pointer to hardware structure
		@offset: offset within the EEPROM to be written to
		@data: 16 bit word(s) to be written to the EEPROM
		@words: number of words

		Writes 16 bit word(s) to EEPROM. If xlnid_eeprom_update_checksum is not
		called after this function, the EEPROM will most likely contain an
		invalid checksum.
	**/
s32 xlnid_write_eeprom_buffer(struct xlnid_hw *hw, u16 offset, u16 bytes,
								u8 *data)
{
	return xlnid_call_func(hw, hw->eeprom.ops.write_buffer, (hw, offset, bytes,
							data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_read_eeprom - Read word from EEPROM
		@hw: pointer to hardware structure
		@offset: offset within the EEPROM to be read
		@data: read 16 bit value from EEPROM

		Reads 16 bit value from EEPROM
	**/
s32 xlnid_read_eeprom(struct xlnid_hw *hw, u16 offset, u8 *data)
{
	return xlnid_call_func(hw, hw->eeprom.ops.read, (hw, offset, data),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_read_eeprom_buffer - Read word(s) from EEPROM
		@hw: pointer to hardware structure
		@offset: offset within the EEPROM to be read
		@data: read 16 bit word(s) from EEPROM
		@words: number of words

		Reads 16 bit word(s) from EEPROM
	**/
s32 xlnid_read_eeprom_buffer(struct xlnid_hw *hw, u16 offset, u16 words,
								u8 *data)
{
	return xlnid_call_func(hw, hw->eeprom.ops.read_buffer, (hw, offset, words,
							data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_validate_eeprom_checksum - Validate EEPROM checksum
		@hw: pointer to hardware structure
		@checksum_val: calculated checksum

		Performs checksum calculation and validates the EEPROM checksum
	**/
s32 xlnid_validate_eeprom_checksum(struct xlnid_hw *hw, u16 *checksum_val)
{
	return xlnid_call_func(hw, hw->eeprom.ops.validate_checksum, (hw, checksum_val),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_eeprom_update_checksum - Updates the EEPROM checksum
		@hw: pointer to hardware structure
	**/
s32 xlnid_update_eeprom_checksum(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->eeprom.ops.update_checksum, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_insert_mac_addr - Find a RAR for this mac address
		@hw: pointer to hardware structure
		@addr: Address to put into receive address register
		@vmdq: VMDq pool to assign

		Puts an ethernet address into a receive address register, or
		finds the rar that it is aleady in; adds to the pool list
	**/
s32 xlnid_insert_mac_addr(struct xlnid_hw *hw, u8 *addr, u32 vmdq)
{
	return xlnid_call_func(hw, hw->mac.ops.insert_mac_addr, (hw, addr, vmdq),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_set_rar - Set Rx address register
		@hw: pointer to hardware structure
		@index: Receive address register to write
		@addr: Address to put into receive address register
		@vmdq: VMDq "set"
		@enable_addr: set flag that address is active

		Puts an ethernet address into a receive address register.
	**/
s32 xlnid_set_rar(struct xlnid_hw *hw, u32 index, u8 *addr, u32 vmdq,
					u32 enable_addr)
{
	return xlnid_call_func(hw, hw->mac.ops.set_rar, (hw, index, addr, vmdq,
							enable_addr), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_clear_rar - Clear Rx address register
		@hw: pointer to hardware structure
		@index: Receive address register to write

		Puts an ethernet address into a receive address register.
	**/
s32 xlnid_clear_rar(struct xlnid_hw *hw, u32 index)
{
	return xlnid_call_func(hw, hw->mac.ops.clear_rar, (hw, index),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_set_vmdq - Associate a VMDq index with a receive address
		@hw: pointer to hardware structure
		@rar: receive address register index to associate with VMDq index
		@vmdq: VMDq set or pool index
	**/
s32 xlnid_set_vmdq(struct xlnid_hw *hw, u32 rar, u32 vmdq)
{
	return xlnid_call_func(hw, hw->mac.ops.set_vmdq, (hw, rar, vmdq),
							XLNID_NOT_IMPLEMENTED);

}

/**
		xlnid_set_vmdq_san_mac - Associate VMDq index 127 with a receive address
		@hw: pointer to hardware structure
		@vmdq: VMDq default pool index
	**/
s32 xlnid_set_vmdq_san_mac(struct xlnid_hw *hw, u32 vmdq)
{
	return xlnid_call_func(hw, hw->mac.ops.set_vmdq_san_mac, (hw, vmdq),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_clear_vmdq - Disassociate a VMDq index from a receive address
		@hw: pointer to hardware structure
		@rar: receive address register index to disassociate with VMDq index
		@vmdq: VMDq set or pool index
	**/
s32 xlnid_clear_vmdq(struct xlnid_hw *hw, u32 rar, u32 vmdq)
{
	return xlnid_call_func(hw, hw->mac.ops.clear_vmdq, (hw, rar, vmdq),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_init_rx_addrs - Initializes receive address filters.
		@hw: pointer to hardware structure

		Places the MAC address in receive address register 0 and clears the rest
		of the receive address registers. Clears the multicast table. Assumes
		the receiver is in reset when the routine is called.
	**/
s32 xlnid_init_rx_addrs(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.init_rx_addrs, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_get_num_rx_addrs - Returns the number of RAR entries.
		@hw: pointer to hardware structure
	**/
u32 xlnid_get_num_rx_addrs(struct xlnid_hw *hw)
{
	return hw->mac.num_rar_entries;
}

/**
		xlnid_update_uc_addr_list - Updates the MAC's list of secondary addresses
		@hw: pointer to hardware structure
		@addr_list: the list of new multicast addresses
		@addr_count: number of addresses
		@func: iterator function to walk the multicast address list

		The given list replaces any existing list. Clears the secondary addrs from
		receive address registers. Uses unused receive address registers for the
		first secondary addresses, and falls back to promiscuous mode as needed.
	**/
s32 xlnid_update_uc_addr_list(struct xlnid_hw *hw, u8 *addr_list,
								u32 addr_count, xlnid_mc_addr_itr func)
{
	return xlnid_call_func(hw, hw->mac.ops.update_uc_addr_list, (hw, addr_list,
							addr_count, func), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_update_mc_addr_list - Updates the MAC's list of multicast addresses
		@hw: pointer to hardware structure
		@mc_addr_list: the list of new multicast addresses
		@mc_addr_count: number of addresses
		@func: iterator function to walk the multicast address list
		@clear: flag, when set clears the table beforehand

		The given list replaces any existing list. Clears the MC addrs from receive
		address registers and the multicast table. Uses unused receive address
		registers for the first multicast addresses, and hashes the rest into the
		multicast table.
	**/
s32 xlnid_update_mc_addr_list(struct xlnid_hw *hw, u8 *mc_addr_list,
								u32 mc_addr_count, xlnid_mc_addr_itr func, bool clear)
{
	return xlnid_call_func(hw, hw->mac.ops.update_mc_addr_list, (hw, mc_addr_list,
							mc_addr_count, func, clear), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_enable_mc - Enable multicast address in RAR
		@hw: pointer to hardware structure

		Enables multicast address in RAR and the use of the multicast hash table.
	**/
s32 xlnid_enable_mc(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.enable_mc, (hw), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_disable_mc - Disable multicast address in RAR
		@hw: pointer to hardware structure

		Disables multicast address in RAR and the use of the multicast hash table.
	**/
s32 xlnid_disable_mc(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.disable_mc, (hw), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_clear_vfta - Clear VLAN filter table
		@hw: pointer to hardware structure

		Clears the VLAN filer table, and the VMDq index associated with the filter
	**/
s32 xlnid_clear_vfta(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.clear_vfta, (hw), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_set_vfta - Set VLAN filter table
		@hw: pointer to hardware structure
		@vlan: VLAN id to write to VLAN filter
		@vind: VMDq output index that maps queue to VLAN id in VLVFB
		@vlan_on: boolean flag to turn on/off VLAN
		@vlvf_bypass: boolean flag indicating updating the default pool is okay

		Turn on/off specified VLAN in the VLAN filter table.
	**/
s32 xlnid_set_vfta(struct xlnid_hw *hw, u32 vlan, u32 vind, bool vlan_on,
					bool vlvf_bypass)
{
	return xlnid_call_func(hw, hw->mac.ops.set_vfta, (hw, vlan, vind, vlan_on,
							vlvf_bypass), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_set_vlvf - Set VLAN Pool Filter
		@hw: pointer to hardware structure
		@vlan: VLAN id to write to VLAN filter
		@vind: VMDq output index that maps queue to VLAN id in VLVFB
		@vlan_on: boolean flag to turn on/off VLAN in VLVF
		@vfta_delta: pointer to the difference between the current value of VFTA
				and the desired value
		@vfta: the desired value of the VFTA
		@vlvf_bypass: boolean flag indicating updating the default pool is okay

		Turn on/off specified bit in VLVF table.
	**/
s32 xlnid_set_vlvf(struct xlnid_hw *hw, u32 vlan, u32 vind, bool vlan_on,
					u32 *vfta_delta, u32 vfta, bool vlvf_bypass)
{
	return xlnid_call_func(hw, hw->mac.ops.set_vlvf, (hw, vlan, vind, vlan_on,
							vfta_delta, vfta, vlvf_bypass), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_fc_enable - Enable flow control
		@hw: pointer to hardware structure

		Configures the flow control settings based on SW configuration.
	**/
s32 xlnid_fc_enable(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.fc_enable, (hw), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_setup_fc - Set up flow control
		@hw: pointer to hardware structure

		Called at init time to set up flow control.
	**/
s32 xlnid_setup_fc(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.setup_fc, (hw), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_set_fw_drv_ver - Try to send the driver version number FW
		@hw: pointer to hardware structure
		@maj: driver major number to be sent to firmware
		@min: driver minor number to be sent to firmware
		@build: driver build number to be sent to firmware
		@ver: driver version number to be sent to firmware
		@len: length of driver_ver string
		@driver_ver: driver string
	**/
s32 xlnid_set_fw_drv_ver(struct xlnid_hw *hw, u8 maj, u8 min, u8 build, u8 ver,
							u16 len, char *driver_ver)
{
	return xlnid_call_func(hw, hw->mac.ops.set_fw_drv_ver, (hw, maj, min, build,
							ver, len, driver_ver), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_get_thermal_sensor_data - Gathers thermal sensor data
		@hw: pointer to hardware structure

		Updates the temperatures in mac.thermal_sensor_data
	**/
s32 xlnid_get_thermal_sensor_data(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.get_thermal_sensor_data, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_init_thermal_sensor_thresh - Inits thermal sensor thresholds
		@hw: pointer to hardware structure

		Inits the thermal sensor thresholds according to the NVM map
	**/
s32 xlnid_init_thermal_sensor_thresh(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.init_thermal_sensor_thresh, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_dmac_config - Configure DMA Coalescing registers.
		@hw: pointer to hardware structure

		Configure DMA coalescing. If enabling dmac, dmac is activated.
		When disabling dmac, dmac enable dmac bit is cleared.
	**/
s32 xlnid_dmac_config(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.dmac_config, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_dmac_update_tcs - Configure DMA Coalescing registers.
		@hw: pointer to hardware structure

		Disables dmac, updates per TC settings, and then enable dmac.
	**/
s32 xlnid_dmac_update_tcs(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.dmac_update_tcs, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_dmac_config_tcs - Configure DMA Coalescing registers.
		@hw: pointer to hardware structure

		Configure DMA coalescing threshold per TC and set high priority bit for
		FCOE TC. The dmac enable bit must be cleared before configuring.
	**/
s32 xlnid_dmac_config_tcs(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.dmac_config_tcs, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_setup_eee - Enable/disable EEE support
		@hw: pointer to the HW structure
		@enable_eee: boolean flag to enable EEE

		Enable/disable EEE based on enable_ee flag.
		Auto-negotiation must be started after BASE-T EEE bits in PHY register 7.3C
		are modified.

	**/
s32 xlnid_setup_eee(struct xlnid_hw *hw, bool enable_eee)
{
	return xlnid_call_func(hw, hw->mac.ops.setup_eee, (hw, enable_eee),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_set_source_address_pruning - Enable/Disable source address pruning
		@hw: pointer to hardware structure
		@enable: enable or disable source address pruning
		@pool: Rx pool - Rx pool to toggle source address pruning
	**/
void xlnid_set_source_address_pruning(struct xlnid_hw *hw, bool enable,
										unsigned int pool)
{
	if (hw->mac.ops.set_source_address_pruning) {
		hw->mac.ops.set_source_address_pruning(hw, enable, pool);
	}
}

/**
		xlnid_set_ethertype_anti_spoofing - Enable/Disable Ethertype anti-spoofing
		@hw: pointer to hardware structure
		@enable: enable or disable switch for Ethertype anti-spoofing
		@vf: Virtual Function pool - VF Pool to set for Ethertype anti-spoofing

	**/
void xlnid_set_ethertype_anti_spoofing(struct xlnid_hw *hw, bool enable, int vf)
{
	if (hw->mac.ops.set_ethertype_anti_spoofing) {
		hw->mac.ops.set_ethertype_anti_spoofing(hw, enable, vf);
	}
}

/**
		xlnid_read_iosf_sb_reg - Read 32 bit PHY register
		@hw: pointer to hardware structure
		@reg_addr: 32 bit address of PHY register to read
		@device_type: type of device you want to communicate with
		@phy_data: Pointer to read data from PHY register

		Reads a value from a specified PHY register
	**/
s32 xlnid_read_iosf_sb_reg(struct xlnid_hw *hw, u32 reg_addr, u32 device_type,
							u32 *phy_data)
{
	return xlnid_call_func(hw, hw->mac.ops.read_iosf_sb_reg, (hw, reg_addr,
							device_type, phy_data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_write_iosf_sb_reg - Write 32 bit register through IOSF Sideband
		@hw: pointer to hardware structure
		@reg_addr: 32 bit PHY register to write
		@device_type: type of device you want to communicate with
		@phy_data: Data to write to the PHY register

		Writes a value to specified PHY register
	**/
s32 xlnid_write_iosf_sb_reg(struct xlnid_hw *hw, u32 reg_addr, u32 device_type,
							u32 phy_data)
{
	return xlnid_call_func(hw, hw->mac.ops.write_iosf_sb_reg, (hw, reg_addr,
							device_type, phy_data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_disable_mdd - Disable malicious driver detection
		@hw: pointer to hardware structure

	**/
void xlnid_disable_mdd(struct xlnid_hw *hw)
{
	if (hw->mac.ops.disable_mdd) {
		hw->mac.ops.disable_mdd(hw);
	}
}

/**
		xlnid_enable_mdd - Enable malicious driver detection
		@hw: pointer to hardware structure

	**/
void xlnid_enable_mdd(struct xlnid_hw *hw)
{
	if (hw->mac.ops.enable_mdd) {
		hw->mac.ops.enable_mdd(hw);
	}
}

/**
		xlnid_mdd_event - Handle malicious driver detection event
		@hw: pointer to hardware structure
		@vf_bitmap: vf bitmap of malicious vfs

	**/
void xlnid_mdd_event(struct xlnid_hw *hw, u32 *vf_bitmap)
{
	if (hw->mac.ops.mdd_event) {
		hw->mac.ops.mdd_event(hw, vf_bitmap);
	}
}

/**
		xlnid_restore_mdd_vf - Restore VF that was disabled during malicious driver
		detection event
		@hw: pointer to hardware structure
		@vf: vf index

	**/
void xlnid_restore_mdd_vf(struct xlnid_hw *hw, u32 vf)
{
	if (hw->mac.ops.restore_mdd_vf) {
		hw->mac.ops.restore_mdd_vf(hw, vf);
	}
}

/**
		xlnid_fw_recovery_mode - Check if in FW NVM recovery mode
		@hw: pointer to hardware structure

	**/
bool xlnid_fw_recovery_mode(struct xlnid_hw *hw)
{
	if (hw->mac.ops.fw_recovery_mode) {
		return hw->mac.ops.fw_recovery_mode(hw);
	}

	return false;
}

/**
		xlnid_enter_lplu - Transition to low power states
		@hw: pointer to hardware structure

		Configures Low Power Link Up on transition to low power states
		(from D0 to non-D0).
	**/
s32 xlnid_enter_lplu(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->phy.ops.enter_lplu, (hw), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_handle_lasi - Handle external Base T PHY interrupt
		@hw: pointer to hardware structure

		Handle external Base T PHY interrupt. If high temperature
		failure alarm then return error, else if link status change
		then setup internal/external PHY link

		Return XLNID_ERR_OVERTEMP if interrupt is high temperature
		failure alarm, else return PHY access status.
*/
s32 xlnid_handle_lasi(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->phy.ops.handle_lasi, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_read_analog_reg8 - Reads 8 bit analog register
		@hw: pointer to hardware structure
		@reg: analog register to read
		@val: read value

		Performs write operation to analog register specified.
	**/
s32 xlnid_read_analog_reg8(struct xlnid_hw *hw, u32 reg, u8 *val)
{
	return xlnid_call_func(hw, hw->mac.ops.read_analog_reg8, (hw, reg, val),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_write_analog_reg8 - Writes 8 bit analog register
		@hw: pointer to hardware structure
		@reg: analog register to write
		@val: value to write

		Performs write operation to Atlas analog register specified.
	**/
s32 xlnid_write_analog_reg8(struct xlnid_hw *hw, u32 reg, u8 val)
{
	return xlnid_call_func(hw, hw->mac.ops.write_analog_reg8, (hw, reg, val),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_init_uta_tables - Initializes Unicast Table Arrays.
		@hw: pointer to hardware structure

		Initializes the Unicast Table Arrays to zero on device load.  This
		is part of the Rx init addr execution path.
	**/
s32 xlnid_init_uta_tables(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.init_uta_tables, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_read_i2c_byte - Reads 8 bit word over I2C at specified device address
		@hw: pointer to hardware structure
		@byte_offset: byte offset to read
		@dev_addr: I2C bus address to read from
		@data: value read

		Performs byte read operation to SFP module's EEPROM over I2C interface.
	**/
s32 xlnid_read_i2c_byte(struct xlnid_hw *hw, u8 byte_offset, u8 dev_addr,
						u8 *data)
{
	return xlnid_call_func(hw, hw->phy.ops.read_i2c_byte, (hw, byte_offset,
							dev_addr, data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_read_i2c_byte_unlocked - Reads 8 bit word via I2C from device address
		@hw: pointer to hardware structure
		@byte_offset: byte offset to read
		@dev_addr: I2C bus address to read from
		@data: value read

		Performs byte read operation to SFP module's EEPROM over I2C interface.
	**/
s32 xlnid_read_i2c_byte_unlocked(struct xlnid_hw *hw, u8 byte_offset,
									u8 dev_addr, u8 *data)
{
	return xlnid_call_func(hw, hw->phy.ops.read_i2c_byte_unlocked, (hw, byte_offset,
							dev_addr, data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_read_link - Perform read operation on link device
		@hw: pointer to the hardware structure
		@addr: bus address to read from
		@reg: device register to read from
		@val: pointer to location to receive read value

		Returns an error code on error.
*/
s32 xlnid_read_link(struct xlnid_hw *hw, u8 addr, u16 reg, u16 *val)
{
	return xlnid_call_func(hw, hw->link.ops.read_link, (hw, addr, reg, val),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_read_link_unlocked - Perform read operation on link device
		@hw: pointer to the hardware structure
		@addr: bus address to read from
		@reg: device register to read from
		@val: pointer to location to receive read value

		Returns an error code on error.
	**/
s32 xlnid_read_link_unlocked(struct xlnid_hw *hw, u8 addr, u16 reg, u16 *val)
{
	return xlnid_call_func(hw, hw->link.ops.read_link_unlocked, (hw, addr, reg,
							val), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_write_i2c_byte - Writes 8 bit word over I2C
		@hw: pointer to hardware structure
		@byte_offset: byte offset to write
		@dev_addr: I2C bus address to write to
		@data: value to write

		Performs byte write operation to SFP module's EEPROM over I2C interface
		at a specified device address.
	**/
s32 xlnid_write_i2c_byte(struct xlnid_hw *hw, u8 byte_offset, u8 dev_addr,
							u8 data)
{
	return xlnid_call_func(hw, hw->phy.ops.write_i2c_byte, (hw, byte_offset,
							dev_addr, data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_write_i2c_byte_unlocked - Writes 8 bit word over I2C
		@hw: pointer to hardware structure
		@byte_offset: byte offset to write
		@dev_addr: I2C bus address to write to
		@data: value to write

		Performs byte write operation to SFP module's EEPROM over I2C interface
		at a specified device address.
	**/
s32 xlnid_write_i2c_byte_unlocked(struct xlnid_hw *hw, u8 byte_offset,
									u8 dev_addr, u8 data)
{
	return xlnid_call_func(hw, hw->phy.ops.write_i2c_byte_unlocked, (hw,
							byte_offset, dev_addr, data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_write_link - Perform write operation on link device
		@hw: pointer to the hardware structure
		@addr: bus address to write to
		@reg: device register to write to
		@val: value to write

		Returns an error code on error.
*/
s32 xlnid_write_link(struct xlnid_hw *hw, u8 addr, u16 reg, u16 val)
{
	return xlnid_call_func(hw, hw->link.ops.write_link, (hw, addr, reg, val),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_write_link_unlocked - Perform write operation on link device
		@hw: pointer to the hardware structure
		@addr: bus address to write to
		@reg: device register to write to
		@val: value to write

		Returns an error code on error.
	**/
s32 xlnid_write_link_unlocked(struct xlnid_hw *hw, u8 addr, u16 reg, u16 val)
{
	return xlnid_call_func(hw, hw->link.ops.write_link_unlocked, (hw, addr, reg,
							val), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_write_i2c_eeprom - Writes 8 bit EEPROM word over I2C interface
		@hw: pointer to hardware structure
		@byte_offset: EEPROM byte offset to write
		@eeprom_data: value to write

		Performs byte write operation to SFP module's EEPROM over I2C interface.
	**/
s32 xlnid_write_i2c_eeprom(struct xlnid_hw *hw, u8 byte_offset, u8 eeprom_data)
{
	return xlnid_call_func(hw, hw->phy.ops.write_i2c_eeprom, (hw, byte_offset,
							eeprom_data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_read_i2c_eeprom - Reads 8 bit EEPROM word over I2C interface
		@hw: pointer to hardware structure
		@byte_offset: EEPROM byte offset to read
		@eeprom_data: value read

		Performs byte read operation to SFP module's EEPROM over I2C interface.
	**/
s32 xlnid_read_i2c_eeprom(struct xlnid_hw *hw, u8 byte_offset, u8 *eeprom_data)
{
	return xlnid_call_func(hw, hw->phy.ops.read_i2c_eeprom, (hw, byte_offset,
							eeprom_data), XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_get_supported_physical_layer - Returns physical layer type
		@hw: pointer to hardware structure

		Determines physical layer capabilities of the current configuration.
	**/
u64 xlnid_get_supported_physical_layer(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.get_supported_physical_layer, (hw),
							XLNID_PHYSICAL_LAYER_UNKNOWN);
}

/**
		xlnid_enable_rx_dma - Enables Rx DMA unit, dependent on device specifics
		@hw: pointer to hardware structure
		@regval: bitfield to write to the Rx DMA register

		Enables the Rx DMA unit of the device.
	**/
s32 xlnid_enable_rx_dma(struct xlnid_hw *hw, u32 regval)
{
	return xlnid_call_func(hw, hw->mac.ops.enable_rx_dma, (hw, regval),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_disable_sec_rx_path - Stops the receive data path
		@hw: pointer to hardware structure

		Stops the receive data path.
	**/
s32 xlnid_disable_sec_rx_path(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.disable_sec_rx_path, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_enable_sec_rx_path - Enables the receive data path
		@hw: pointer to hardware structure

		Enables the receive data path.
	**/
s32 xlnid_enable_sec_rx_path(struct xlnid_hw *hw)
{
	return xlnid_call_func(hw, hw->mac.ops.enable_sec_rx_path, (hw),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_acquire_swfw_semaphore - Acquire SWFW semaphore
		@hw: pointer to hardware structure
		@mask: Mask to specify which semaphore to acquire

		Acquires the SWFW semaphore through SW_FW_SYNC register for the specified
		function (CSR, PHY0, PHY1, EEPROM, Flash)
	**/
s32 xlnid_acquire_swfw_semaphore(struct xlnid_hw *hw, u32 mask)
{
	return xlnid_call_func(hw, hw->mac.ops.acquire_swfw_sync, (hw, mask),
							XLNID_NOT_IMPLEMENTED);
}

/**
		xlnid_release_swfw_semaphore - Release SWFW semaphore
		@hw: pointer to hardware structure
		@mask: Mask to specify which semaphore to release

		Releases the SWFW semaphore through SW_FW_SYNC register for the specified
		function (CSR, PHY0, PHY1, EEPROM, Flash)
	**/
void xlnid_release_swfw_semaphore(struct xlnid_hw *hw, u32 mask)
{
	if (hw->mac.ops.release_swfw_sync) {
		hw->mac.ops.release_swfw_sync(hw, mask);
	}
}

/**
		xlnid_init_swfw_semaphore - Clean up SWFW semaphore
		@hw: pointer to hardware structure

		Attempts to acquire the SWFW semaphore through SW_FW_SYNC register.
		Regardless of whether is succeeds or not it then release the semaphore.
		This is function is called to recover from catastrophic failures that
		may have left the semaphore locked.
	**/
void xlnid_init_swfw_semaphore(struct xlnid_hw *hw)
{
	if (hw->mac.ops.init_swfw_sync) {
		hw->mac.ops.init_swfw_sync(hw);
	}
}

void xlnid_disable_rx(struct xlnid_hw *hw)
{
	if (hw->mac.ops.disable_rx) {
		hw->mac.ops.disable_rx(hw);
	}
}

void xlnid_enable_rx(struct xlnid_hw *hw)
{
	if (hw->mac.ops.enable_rx) {
		hw->mac.ops.enable_rx(hw);
	}
}

/**
		xlnid_set_rate_select_speed - Set module link speed
		@hw: pointer to hardware structure
		@speed: link speed to set

		Set module link speed via the rate select.
*/
void xlnid_set_rate_select_speed(struct xlnid_hw *hw, xlnid_link_speed speed)
{
	if (hw->mac.ops.set_rate_select_speed) {
		hw->mac.ops.set_rate_select_speed(hw, speed);
	}
}

// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2008 - 2024 Xel Technology. */

#include "xlnid_common.h"
#include "xlnid_api.h"

STATIC s32 xlnid_mta_vector(struct xlnid_hw *hw, u8 *mc_addr);

/**
		xlnid_init_ops_generic - Inits function ptrs
		@hw: pointer to the hardware structure

		Initialize the function pointers.
	**/
s32 xlnid_init_ops_generic(struct xlnid_hw *hw)
{
	struct xlnid_eeprom_info *eeprom = &hw->eeprom;
	struct xlnid_mac_info *mac = &hw->mac;
	u32 eec = 0;

	DEBUGFUNC("xlnid_init_ops_generic");

	/* EEPROM */
	eeprom->ops.init_params = xlnid_init_eeprom_params_generic;

	/* If EEPROM is valid (bit 8 = 1), use EERD otherwise use bit bang */
	if (eec & XLNID_EEC_PRES) {
		eeprom->ops.read = xlnid_read_eerd_generic;
		eeprom->ops.read_buffer = xlnid_read_eerd_buffer_generic;
	}

	else {
		eeprom->ops.read = xlnid_read_eeprom_bit_bang_generic;
		eeprom->ops.read_buffer = xlnid_read_eeprom_buffer_bit_bang_generic;
	}

	eeprom->ops.write = xlnid_write_eeprom_generic;
	eeprom->ops.validate_checksum = xlnid_validate_eeprom_checksum_generic;
	eeprom->ops.update_checksum = xlnid_update_eeprom_checksum_generic;
	eeprom->ops.calc_checksum = xlnid_calc_eeprom_checksum_generic;

	/* MAC */
	mac->ops.init_hw = xlnid_init_hw_generic;
	mac->ops.reset_hw = NULL;
	mac->ops.start_hw = xlnid_start_hw_generic;
	mac->ops.clear_hw_cntrs = xlnid_clear_hw_cntrs_generic;
	mac->ops.get_media_type = NULL;
	mac->ops.get_supported_physical_layer = NULL;
	mac->ops.enable_rx_dma = xlnid_enable_rx_dma_generic;
	mac->ops.get_mac_addr = xlnid_get_mac_addr_generic;
	mac->ops.get_san_mac_addr = xlnid_get_san_mac_addr_generic;
	mac->ops.stop_adapter = xlnid_stop_adapter_generic;
	mac->ops.get_bus_info = xlnid_get_bus_info_generic;
	mac->ops.set_lan_id = xlnid_set_lan_id_multi_port_pcie;
	mac->ops.acquire_swfw_sync = xlnid_acquire_swfw_sync;
	mac->ops.release_swfw_sync = xlnid_release_swfw_sync;
	mac->ops.prot_autoc_read = prot_autoc_read_generic;
	mac->ops.prot_autoc_write = prot_autoc_write_generic;

	/* LEDs */
	mac->ops.led_on = xlnid_led_on_generic;
	mac->ops.led_off = xlnid_led_off_generic;
	mac->ops.blink_led_start = xlnid_blink_led_start_generic;
	mac->ops.blink_led_stop = xlnid_blink_led_stop_generic;
	mac->ops.init_led_link_act = xlnid_init_led_link_act_generic;

	/* RAR, Multicast, VLAN */
	mac->ops.set_rar = xlnid_set_rar_generic;
	mac->ops.clear_rar = xlnid_clear_rar_generic;
	mac->ops.insert_mac_addr = NULL;
	mac->ops.set_vmdq = NULL;
	mac->ops.clear_vmdq = NULL;
	mac->ops.init_rx_addrs = xlnid_init_rx_addrs_generic;
	mac->ops.update_uc_addr_list = xlnid_update_uc_addr_list_generic;
	mac->ops.update_mc_addr_list = xlnid_update_mc_addr_list_generic;
	mac->ops.enable_mc = xlnid_enable_mc_generic;
	mac->ops.disable_mc = xlnid_disable_mc_generic;
	mac->ops.clear_vfta = NULL;
	mac->ops.set_vfta = NULL;
	mac->ops.set_vlvf = NULL;
	mac->ops.init_uta_tables = NULL;
	mac->ops.enable_rx = xlnid_enable_rx_generic;
	mac->ops.disable_rx = xlnid_disable_rx_generic;

	/* Flow Control */
	mac->ops.fc_enable = xlnid_fc_enable_generic;
	mac->ops.setup_fc = xlnid_setup_fc_generic;
	mac->ops.fc_autoneg = xlnid_fc_autoneg;

	/* Link */
	mac->ops.get_link_capabilities = NULL;
	mac->ops.setup_link = NULL;
	mac->ops.check_link = NULL;
	mac->ops.dmac_config = NULL;
	mac->ops.dmac_update_tcs = NULL;
	mac->ops.dmac_config_tcs = NULL;

	return XLNID_SUCCESS;
}

/**
		xlnid_device_supports_autoneg_fc - Check if device supports autonegotiation
		of flow control
		@hw: pointer to hardware structure

		This function returns true if the device supports flow control
		autonegotiation, and false if it does not.

	**/
bool xlnid_device_supports_autoneg_fc(struct xlnid_hw *hw)
{

	return true;
}

/**
		xlnid_setup_fc_generic - Set up flow control
		@hw: pointer to hardware structure

		Called at init time to set up flow control.
	**/
s32 xlnid_setup_fc_generic(struct xlnid_hw *hw)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_start_hw_generic - Prepare hardware for Tx/Rx
		@hw: pointer to hardware structure

		Starts the hardware by filling the bus info structure and media type, clears
		all on chip counters, initializes receive address registers, multicast
		table, VLAN filter table, calls routine to set up link and flow control
		settings, and leaves transmit and receive units disabled and uninitialized
	**/
s32 xlnid_start_hw_generic(struct xlnid_hw *hw)
{
	s32 ret_val;
	u16 device_caps;

	DEBUGFUNC("xlnid_start_hw_generic");

	/* Set the media type */
	hw->phy.media_type = hw->mac.ops.get_media_type(hw);

	/* PHY ops initialization must be done in reset_hw() */

	/* Clear the VLAN filter table */
	hw->mac.ops.clear_vfta(hw);

	/* Clear statistics registers */
	hw->mac.ops.clear_hw_cntrs(hw);

	/* Setup flow control */
	ret_val = xlnid_setup_fc(hw);

	if (ret_val != XLNID_SUCCESS && ret_val != XLNID_NOT_IMPLEMENTED) {
		hw_dbg(hw, "Flow control setup failed, returning %d\n", ret_val);
		return ret_val;
	}

	/* Cache bit indicating need for crosstalk fix */
	hw->mac.ops.get_device_caps(hw, &device_caps);

	if (device_caps & XLNID_DEVICE_CAPS_NO_CROSSTALK_WR) {
		hw->need_crosstalk_fix = false;
	}

	else {
		hw->need_crosstalk_fix = true;
	}

	/* Clear adapter stopped flag */
	hw->adapter_stopped = false;

	return XLNID_SUCCESS;
}

/**
		xlnid_init_hw_generic - Generic hardware initialization
		@hw: pointer to hardware structure

		Initialize the hardware by resetting the hardware, filling the bus info
		structure and media type, clears all on chip counters, initializes receive
		address registers, multicast table, VLAN filter table, calls routine to set
		up link and flow control settings, and leaves transmit and receive units
		disabled and uninitialized
	**/
s32 xlnid_init_hw_generic(struct xlnid_hw *hw)
{
	s32 status;

	DEBUGFUNC("xlnid_init_hw_generic");

	/* Reset the hardware */
	status = hw->mac.ops.reset_hw(hw);

	if (status == XLNID_SUCCESS || status == XLNID_ERR_SFP_NOT_PRESENT) {
		/* Start the HW */
		status = hw->mac.ops.start_hw(hw);
	}

	/* Initialize the LED link active for LED blink support */
	if (hw->mac.ops.init_led_link_act) {
		hw->mac.ops.init_led_link_act(hw);
	}

	if (status != XLNID_SUCCESS) {
		hw_dbg(hw, "Failed to initialize HW, STATUS = %d\n", status);
	}

	return status;
}

/**
		xlnid_clear_hw_cntrs_generic - Generic clear hardware counters
		@hw: pointer to hardware structure

		Clears all hardware statistics counters by reading them from the hardware
		Statistics counters are clear on read.
	**/
s32 xlnid_clear_hw_cntrs_generic(struct xlnid_hw *hw)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_get_mac_addr_generic - Generic get MAC address
		@hw: pointer to hardware structure
		@mac_addr: Adapter MAC address

		Reads the adapter's MAC address from first Receive Address Register (RAR0)
		A reset of the adapter must be performed prior to calling this function
		in order for the MAC address to have been loaded from the EEPROM into RAR0
	**/
s32 xlnid_get_mac_addr_generic(struct xlnid_hw *hw, u8 *mac_addr)
{
	u32 rar_high;
	u32 rar_low;
	u16 i;

	DEBUGFUNC("xlnid_get_mac_addr_generic");

	rar_high = XLNID_READ_REG(hw, RAH(0));
	rar_low = XLNID_READ_REG(hw, RAL(0));

	for (i = 0; i < 4; i++) {
		mac_addr[i] = (u8)(rar_low >> (i * 8));
	}

	for (i = 0; i < 2; i++) {
		mac_addr[i + 4] = (u8)(rar_high >> (i * 8));
	}

	return XLNID_SUCCESS;
}

/**
		xlnid_set_pci_config_data_generic - Generic store PCI bus info
		@hw: pointer to hardware structure
		@link_status: the link status returned by the PCI config space

		Stores the PCI bus info (speed, width, type) within the xlnid_hw structure
	**/
void xlnid_set_pci_config_data_generic(struct xlnid_hw *hw, u16 link_status)
{
	struct xlnid_mac_info *mac = &hw->mac;

	if (hw->bus.type == xlnid_bus_type_unknown) {
		hw->bus.type = xlnid_bus_type_pci_express;
	}

	switch (link_status & XLNID_PCI_LINK_WIDTH) {
	case XLNID_PCI_LINK_WIDTH_1:
		hw->bus.width = xlnid_bus_width_pcie_x1;
		break;

	case XLNID_PCI_LINK_WIDTH_2:
		hw->bus.width = xlnid_bus_width_pcie_x2;
		break;

	case XLNID_PCI_LINK_WIDTH_4:
		hw->bus.width = xlnid_bus_width_pcie_x4;
		break;

	case XLNID_PCI_LINK_WIDTH_8:
		hw->bus.width = xlnid_bus_width_pcie_x8;
		break;

	default:
		hw->bus.width = xlnid_bus_width_unknown;
		break;
	}

	switch (link_status & XLNID_PCI_LINK_SPEED) {
	case XLNID_PCI_LINK_SPEED_2500:
		hw->bus.speed = xlnid_bus_speed_2500;
		break;

	case XLNID_PCI_LINK_SPEED_5000:
		hw->bus.speed = xlnid_bus_speed_5000;
		break;

	case XLNID_PCI_LINK_SPEED_8000:
		hw->bus.speed = xlnid_bus_speed_8000;
		break;

	case XLNID_PCI_LINK_SPEED_16000:
		hw->bus.speed = xlnid_bus_speed_16000;
		break;

	default:
		hw->bus.speed = xlnid_bus_speed_unknown;
		break;
	}

	mac->ops.set_lan_id(hw);
}

/**
		xlnid_get_bus_info_generic - Generic set PCI bus info
		@hw: pointer to hardware structure

		Gets the PCI bus info (speed, width, type) then calls helper function to
		store this data within the xlnid_hw structure.
	**/
s32 xlnid_get_bus_info_generic(struct xlnid_hw *hw)
{
	u16 link_status;

	DEBUGFUNC("xlnid_get_bus_info_generic");

	/* Get the negotiated link width and speed from PCI config space */
	link_status = XLNID_READ_PCIE_WORD(hw, XLNID_PCI_LINK_STATUS);

	xlnid_set_pci_config_data_generic(hw, link_status);

	return XLNID_SUCCESS;
}

/**
		xlnid_set_lan_id_multi_port_pcie - Set LAN id for PCIe multiple port devices
		@hw: pointer to the HW structure

		Determines the LAN function id by reading memory-mapped registers and swaps
		the port value if requested, and set MAC instance for devices that share
		CS4227.
	**/
void xlnid_set_lan_id_multi_port_pcie(struct xlnid_hw *hw)
{
	/* TODO */
#if 0
	struct xlnid_bus_info *bus = &hw->bus;
	u32 reg;
	u16 ee_ctrl_4;

	DEBUGFUNC("xlnid_set_lan_id_multi_port_pcie");

	reg = XLNID_READ_REG(hw, XLNID_STATUS);
	bus->func = (reg & XLNID_STATUS_LAN_ID) >> XLNID_STATUS_LAN_ID_SHIFT;
	bus->lan_id = (u8) bus->func;

	/* check for a port swap */
	reg = XLNID_READ_REG(hw, XLNID_FACTPS_BY_MAC(hw));

	if (reg & XLNID_FACTPS_LFS) {
		bus->func ^= 0x1;
	}

#endif
	return;
}

/**
		xlnid_stop_adapter_generic - Generic stop Tx/Rx units
		@hw: pointer to hardware structure

		Sets the adapter_stopped flag within xlnid_hw struct. Clears interrupts,
		disables transmit and receive units. The adapter_stopped flag is used by
		the shared code and drivers to determine if the adapter is in a stopped
		state and should not touch the hardware.
	**/
s32 xlnid_stop_adapter_generic(struct xlnid_hw *hw)
{
	u32 reg_val;
	u16 i;

	DEBUGFUNC("xlnid_stop_adapter_generic");

	/*
		Set the adapter_stopped flag so other driver functions stop touching
		the hardware
	*/
	hw->adapter_stopped = true;

	/* Disable the receive unit */
	xlnid_disable_rx(hw);

	/* Clear interrupt mask to stop interrupts from being generated */
	XLNID_WRITE_REG_DIRECT(hw, EIMC, XLNID_IRQ_CLEAR_MASK);

	/* Clear any pending interrupts, flush previous writes */
	XLNID_READ_REG_DIRECT(hw, EICR);

	/* Disable the transmit unit.  Each queue must be disabled. */
	for (i = 0; i < hw->mac.max_tx_queues; i++) {
		XLNID_WRITE_REG_DIRECT(hw, TXDCTL(i), XLNID_TXDCTL_SWFLSH);
	}

	/* Disable the receive unit by stopping each queue */
	for (i = 0; i < hw->mac.max_rx_queues; i++) {
		reg_val = XLNID_READ_REG_DIRECT(hw, RXDCTL(i));
		reg_val &= ~XLNID_RXDCTL_ENABLE;
		reg_val |= XLNID_RXDCTL_SWFLSH;
		XLNID_WRITE_REG_DIRECT(hw, RXDCTL(i), reg_val);
	}

	/* flush all queues disables */
	XLNID_WRITE_FLUSH(hw);
	msec_delay(2);

	/*
		Prevent the PCI-E bus from hanging by disabling PCI-E primary
		access and verify no pending requests
	*/
	return xlnid_disable_pcie_primary(hw);
}

/**
		xlnid_init_led_link_act_generic - Store the LED index link/activity.
		@hw: pointer to hardware structure

		Store the index for the link active LED. This will be used to support
		blinking the LED.
	**/
s32 xlnid_init_led_link_act_generic(struct xlnid_hw *hw)
{
	u32 led_reg = 0;
	u16 val = 0;
	struct xlnid_mac_info *mac = &hw->mac;

	if (mac->type == xlnid_mac_WESTLAKE) {
		/* change led blink timer to 250ms */
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LED_BLINK_TIMERH, 0x01DC);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LED_BLINK_TIMERL, 0xD650);

		/* led0 & led1 use link&activity mode */
		led_reg = (XLNID_LED_LINK_ACTIVE << XLNID_LED_MODE_SHIFT(1)) |
					(XLNID_LED_LINK_ACTIVE << XLNID_LED_MODE_SHIFT(0));
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCTL, led_reg);

		xlnid_write_phy_reg(hw, 0x1f, 0, 0x5);
		val = xlnid_read_phy_reg(hw, 0x16, 0, &val);
		val &= 0x7fff;
		xlnid_write_phy_reg(hw, 0x16, 0, val);
		xlnid_write_phy_reg(hw, 0x1f, 0, 0x0);
	}

	mac->led_link_act = 0;

	return XLNID_SUCCESS;
}

/**
		xlnid_led_on_generic - Turns on the software controllable LEDs.
		@hw: pointer to hardware structure
		@index: led number to turn on
	**/
s32 xlnid_led_on_generic(struct xlnid_hw *hw, u32 index)
{
	u32 led_reg = 0;

	DEBUGFUNC("xlnid_led_on_generic");
	(void)index;

	if (hw->mac.type == xlnid_mac_WESTLAKE) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCMD, 0x0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCMD, 0x0);
	}

	led_reg = XLNID_READ_REG(hw, LEDCTL);

	if (!hw->multifunction) {
		/* one_port use led0 */
		led_reg &= ~XLNID_LED_MODE_MASK(0);
		led_reg |= XLNID_LED_ON << XLNID_LED_MODE_SHIFT(0);
	}

	else {
		/* To turn on the LED, set mode to ON. */
		if (hw->phy.media_type == xlnid_media_type_copper) {
			/* copper use led1 */
			led_reg &= ~XLNID_LED_MODE_MASK(1);
			led_reg |= XLNID_LED_ON << XLNID_LED_MODE_SHIFT(1);
		}

		else if (hw->phy.media_type == xlnid_media_type_fiber) {
			/* fiber use led0 */
			led_reg &= ~XLNID_LED_MODE_MASK(0);
			led_reg |= XLNID_LED_ON << XLNID_LED_MODE_SHIFT(0);
		}
	}

	XLNID_WRITE_REG(hw, LEDCTL, led_reg);
	//XLNID_WRITE_FLUSH(hw);

	return XLNID_SUCCESS;
}

/**
		xlnid_led_off_generic - Turns off the software controllable LEDs.
		@hw: pointer to hardware structure
		@index: led number to turn off
	**/
s32 xlnid_led_off_generic(struct xlnid_hw *hw, u32 index)
{
	u32 led_reg = 0;

	DEBUGFUNC("xlnid_led_off_generic");
	(void)index;

	if (hw->mac.type == xlnid_mac_WESTLAKE) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCMD, 0x0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCMD, 0x0);
	}

	led_reg = XLNID_READ_REG(hw, LEDCTL);

	if (!hw->multifunction) {
		/* one_port use led0 */
		led_reg &= ~XLNID_LED_MODE_MASK(0);
		led_reg |= XLNID_LED_OFF << XLNID_LED_MODE_SHIFT(0);
	}

	else {
		/* To turn off the LED, set mode to OFF. */
		if (hw->phy.media_type == xlnid_media_type_copper) {
			/* copper use led1 */
			led_reg &= ~XLNID_LED_MODE_MASK(1);
			led_reg |= XLNID_LED_OFF << XLNID_LED_MODE_SHIFT(1);
		}

		else {
			/* fiber use led0 */
			led_reg &= ~XLNID_LED_MODE_MASK(0);
			led_reg |= XLNID_LED_OFF << XLNID_LED_MODE_SHIFT(0);
		}
	}

	XLNID_WRITE_REG(hw, LEDCTL, led_reg);
	//XLNID_WRITE_FLUSH(hw);

	return XLNID_SUCCESS;
}

/**
		xlnid_init_eeprom_params_generic - Initialize EEPROM params
		@hw: pointer to hardware structure

		Initializes the EEPROM parameters xlnid_eeprom_info within the
		xlnid_hw struct in order to set up EEPROM access.
	**/
s32 xlnid_init_eeprom_params_generic(struct xlnid_hw *hw)
{
	struct xlnid_eeprom_info *eeprom = &hw->eeprom;

	DEBUGFUNC("xlnid_init_eeprom_params_generic");

	if (hw->device_id == XLNID_DEV_ID_SGMII_MAC ||
			hw->device_id == XLNID_DEV_ID_1000BASEX ||
			hw->device_id == XLNID_DEV_ID_SGMII_PHY) {
		eeprom->type = xlnid_eeprom_i2c;
		goto out;
	}

#ifdef XLNID_SPI_FLASH
	eeprom->type = xlnid_eeprom_spi;
#else

	if (!hw->multifunction) {
		eeprom->type = xlnid_eeprom_spi;
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_SSIENR, SPI_DISABLE_DWC_SSI);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_SPI_BAUDR, SPI_BAUDR_SCKDV);
	}

	else {
		eeprom->type = xlnid_eeprom_i2c;
	}

#endif /* #ifdef XLNID_SPI_FLASH */

out:
	/*  Set default semaphore delay to 10ms which is a well
		tested value */
	eeprom->semaphore_delay = 10;
	/* Clear EEPROM page size, it will be initialized as needed */
	eeprom->word_page_size = 0;

	eeprom->address_bits = 8;
	eeprom->word_size = 128;
	hw_dbg(hw, "Eeprom params: type = %d, size = %d, address bits: "
			"%d\n", eeprom->type, eeprom->word_size,
			eeprom->address_bits);

	return XLNID_SUCCESS;
}

/**
		xlnid_write_eeprom_generic - Writes 16 bit value to EEPROM
		@hw: pointer to hardware structure
		@offset: offset within the EEPROM to be written to
		@data: 16 bit word to be written to the EEPROM

		If xlnid_eeprom_update_checksum is not called after this function, the
		EEPROM will most likely contain an invalid checksum.
	**/
s32 xlnid_write_eeprom_generic(struct xlnid_hw *hw, u16 offset, u16 data)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_read_eeprom_buffer_bit_bang_generic - Read EEPROM using bit-bang
		@hw: pointer to hardware structure
		@offset: offset within the EEPROM to be read
		@data: read 16 bit words(s) from EEPROM
		@words: number of word(s)

		Reads 16 bit word(s) from EEPROM through bit-bang method
	**/
s32 xlnid_read_eeprom_buffer_bit_bang_generic(struct xlnid_hw *hw, u16 offset,
		u16 words, u8 *data)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_read_eeprom_bit_bang_generic - Read EEPROM word using bit-bang
		@hw: pointer to hardware structure
		@offset: offset within the EEPROM to be read
		@data: read 8 bit value from EEPROM

		Reads 8 bit value from EEPROM through bit-bang method
	**/
s32 xlnid_read_eeprom_bit_bang_generic(struct xlnid_hw *hw, u16 offset,
										u8 *data)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_read_eerd_buffer_generic - Read EEPROM word(s) using EERD
		@hw: pointer to hardware structure
		@offset: offset of byte in the EEPROM to read
		@words: number of byte(s)
		@data: 8 bit byte(s) from the EEPROM

		Reads a 8 bit bytes(s) from the EEPROM using the EERD register.
	**/
s32 xlnid_read_eerd_buffer_generic(struct xlnid_hw *hw, u16 offset,
									u16 bytes, u8 *data)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_read_eerd_generic - Read EEPROM word using EERD
		@hw: pointer to hardware structure
		@offset: offset of  byte in the EEPROM to read
		@data: byte read from the EEPROM

		Reads a 8 bit byte from the EEPROM using the EERD register.
	**/
s32 xlnid_read_eerd_generic(struct xlnid_hw *hw, u16 offset, u8 *data)
{
	return xlnid_read_eerd_buffer_generic(hw, offset, 1, data);
}

/**
		xlnid_write_eewr_buffer_generic - Write EEPROM byte(s) using EEWR
		@hw: pointer to hardware structure
		@offset: offset of  word in the EEPROM to write
		@words: number of word(s)
		@data: word(s) write to the EEPROM

		Write a 16 bit word(s) to the EEPROM using the EEWR register.
	**/
s32 xlnid_write_eewr_buffer_generic(struct xlnid_hw *hw, u16 offset,
									u16 words, u16 *data)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_write_eewr_generic - Write EEPROM word using EEWR
		@hw: pointer to hardware structure
		@offset: offset of  word in the EEPROM to write
		@data: word write to the EEPROM

		Write a 16 bit word to the EEPROM using the EEWR register.
	**/
s32 xlnid_write_eewr_generic(struct xlnid_hw *hw, u16 offset, u16 data)
{
	return xlnid_write_eewr_buffer_generic(hw, offset, 1, &data);
}

/**
		xlnid_poll_eerd_eewr_done - Poll EERD read or EEWR write status
		@hw: pointer to hardware structure
		@ee_reg: EEPROM flag for polling

		Polls the status bit (bit 1) of the EERD or EEWR to determine when the
		read or write is done respectively.
	**/
s32 xlnid_poll_eerd_eewr_done(struct xlnid_hw *hw, u32 ee_reg)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_calc_eeprom_checksum_generic - Calculates and returns the checksum
		@hw: pointer to hardware structure

		Returns a negative error code on error, or the 16-bit checksum
	**/
s32 xlnid_calc_eeprom_checksum_generic(struct xlnid_hw *hw)
{
	u16 checksum = 0;

	checksum = (u16) XLNID_EEPROM_SUM;

	return (s32) checksum;
}

/**
		xlnid_validate_eeprom_checksum_generic - Validate EEPROM checksum
		@hw: pointer to hardware structure
		@checksum_val: calculated checksum

		Performs checksum calculation and validates the EEPROM checksum.  If the
		caller does not need checksum_val, the value can be NULL.
	**/
s32 xlnid_validate_eeprom_checksum_generic(struct xlnid_hw *hw,
		u16 *checksum_val)
{
	*checksum_val = (u16) XLNID_EEPROM_SUM;
	return XLNID_SUCCESS;
}

/**
		xlnid_update_eeprom_checksum_generic - Updates the EEPROM checksum
		@hw: pointer to hardware structure
	**/
s32 xlnid_update_eeprom_checksum_generic(struct xlnid_hw *hw)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_validate_mac_addr - Validate MAC address
		@mac_addr: pointer to MAC address.

		Tests a MAC address to ensure it is a valid Individual Address.
	**/
s32 xlnid_validate_mac_addr(u8 *mac_addr)
{
	s32 status = XLNID_SUCCESS;

	DEBUGFUNC("xlnid_validate_mac_addr");

	/* Make sure it is not a multicast address */
	if (XLNID_IS_MULTICAST(mac_addr)) {
		status = XLNID_ERR_INVALID_MAC_ADDR;
		/* Not a broadcast address */
	}

	else if (XLNID_IS_BROADCAST(mac_addr)) {
		status = XLNID_ERR_INVALID_MAC_ADDR;
		/* Reject the zero address */
	}

	else if (mac_addr[0] == 0 && mac_addr[1] == 0 && mac_addr[2] == 0 &&
				mac_addr[3] == 0 && mac_addr[4] == 0 && mac_addr[5] == 0) {
		status = XLNID_ERR_INVALID_MAC_ADDR;
	}

	return status;
}

/**
		xlnid_set_rar_generic - Set Rx address register
		@hw: pointer to hardware structure
		@index: Receive address register to write
		@addr: Address to put into receive address register
		@vmdq: VMDq "set" or "pool" index
		@enable_addr: set flag that address is active

		Puts an ethernet address into a receive address register.
	**/
s32 xlnid_set_rar_generic(struct xlnid_hw *hw, u32 index, u8 *addr, u32 vmdq,
							u32 enable_addr)
{
	u32 rar_low, rar_high;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("xlnid_set_rar_generic");

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries) {
		ERROR_REPORT2(XLNID_ERROR_ARGUMENT,
						"RAR index %d is out of range.\n", index);
		return XLNID_ERR_INVALID_ARGUMENT;
	}

	/* setup VMDq pool selection before this RAR gets enabled */
	hw->mac.ops.set_vmdq(hw, index, vmdq);

	/*
		HW expects these in little endian so we reverse the byte
		order from network order (big endian) to little endian
	*/
	rar_low = ((u32)addr[0] |
				((u32)addr[1] << 8) |
				((u32)addr[2] << 16) |
				((u32)addr[3] << 24));
	/*
		Some parts put the VMDq setting in the extra RAH bits,
		so save everything except the lower 16 bits that hold part
		of the address and the address valid bit.
	*/
	rar_high = XLNID_READ_REG(hw, RAH(index));
	rar_high &= ~(0x0000FFFF | XLNID_RAH_AV);
	rar_high |= ((u32)addr[4] | ((u32)addr[5] << 8));

	if (enable_addr != 0) {
		rar_high |= XLNID_RAH_AV;
	}

	XLNID_WRITE_REG(hw, RAL(index), rar_low);
	XLNID_WRITE_REG(hw, RAH(index), rar_high);

	return XLNID_SUCCESS;
}

/**
		xlnid_clear_rar_generic - Remove Rx address register
		@hw: pointer to hardware structure
		@index: Receive address register to write

		Clears an ethernet address from a receive address register.
	**/
s32 xlnid_clear_rar_generic(struct xlnid_hw *hw, u32 index)
{
	u32 rar_high;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("xlnid_clear_rar_generic");

	/* Make sure we are using a valid rar index range */
	if (index >= rar_entries) {
		ERROR_REPORT2(XLNID_ERROR_ARGUMENT,
						"RAR index %d is out of range.\n", index);
		return XLNID_ERR_INVALID_ARGUMENT;
	}

	/*
		Some parts put the VMDq setting in the extra RAH bits,
		so save everything except the lower 16 bits that hold part
		of the address and the address valid bit.
	*/
	rar_high = XLNID_READ_REG(hw, RAH(index));
	rar_high &= ~(0x0000FFFF | XLNID_RAH_AV);

	XLNID_WRITE_REG(hw, RAL(index), 0);
	XLNID_WRITE_REG(hw, RAH(index), rar_high);

	/* clear VMDq pool/queue selection for this RAR */
	hw->mac.ops.clear_vmdq(hw, index, XLNID_CLEAR_VMDQ_ALL);

	return XLNID_SUCCESS;
}

/**
		xlnid_init_rx_addrs_generic - Initializes receive address filters.
		@hw: pointer to hardware structure

		Places the MAC address in receive address register 0 and clears the rest
		of the receive address registers. Clears the multicast table. Assumes
		the receiver is in reset when the routine is called.
	**/
s32 xlnid_init_rx_addrs_generic(struct xlnid_hw *hw)
{
	u32 i;
	u32 rar_entries = hw->mac.num_rar_entries;

	DEBUGFUNC("xlnid_init_rx_addrs_generic");

	/*
		If the current mac address is valid, assume it is a software override
		to the permanent address.
		Otherwise, use the permanent address from the eeprom.
	*/
	if (xlnid_validate_mac_addr(hw->mac.addr) ==
			XLNID_ERR_INVALID_MAC_ADDR) {
		/* Get the MAC address from the RAR0 for later reference */
		hw->mac.ops.get_mac_addr(hw, hw->mac.addr);

		hw_dbg(hw, " Keeping Current RAR0 Addr =%.2X %.2X %.2X ",
				hw->mac.addr[0], hw->mac.addr[1],
				hw->mac.addr[2]);
		hw_dbg(hw, "%.2X %.2X %.2X\n", hw->mac.addr[3],
				hw->mac.addr[4], hw->mac.addr[5]);
	}

	else {
		/* Setup the receive address. */
		hw_dbg(hw, "Overriding MAC Address in RAR[0]\n");
		hw_dbg(hw, " New MAC Addr =%.2X %.2X %.2X ",
				hw->mac.addr[0], hw->mac.addr[1],
				hw->mac.addr[2]);
		hw_dbg(hw, "%.2X %.2X %.2X\n", hw->mac.addr[3],
				hw->mac.addr[4], hw->mac.addr[5]);

		hw->mac.ops.set_rar(hw, 0, hw->mac.addr, 0, XLNID_RAH_AV);
	}

	/* clear VMDq pool/queue selection for RAR 0 */
	hw->mac.ops.clear_vmdq(hw, 0, XLNID_CLEAR_VMDQ_ALL);

	hw->addr_ctrl.overflow_promisc = 0;

	hw->addr_ctrl.rar_used_count = 1;

	/* Zero out the other receive addresses. */
	hw_dbg(hw, "Clearing RAR[1-%d]\n", rar_entries - 1);

	for (i = 1; i < rar_entries; i++) {
		XLNID_WRITE_REG(hw, RAL(i), 0);
		XLNID_WRITE_REG(hw, RAH(i), 0);
	}

	/* Clear the MTA */
	hw->addr_ctrl.mta_in_use = 0;
	XLNID_WRITE_REG(hw, MCSTCTRL, hw->mac.mc_filter_type);

	hw_dbg(hw, " Clearing MTA\n");

	for (i = 0; i < hw->mac.mcft_size; i++) {
		XLNID_WRITE_REG(hw, MTA(i), 0);
	}

	xlnid_init_uta_tables(hw);

	return XLNID_SUCCESS;
}

/**
		xlnid_add_uc_addr - Adds a secondary unicast address.
		@hw: pointer to hardware structure
		@addr: new address
		@vmdq: VMDq "set" or "pool" index

		Adds it to unused receive address register or goes into promiscuous mode.
	**/
void xlnid_add_uc_addr(struct xlnid_hw *hw, u8 *addr, u32 vmdq)
{
	u32 rar_entries = hw->mac.num_rar_entries;
	u32 rar;

	DEBUGFUNC("xlnid_add_uc_addr");

	hw_dbg(hw, " UC Addr = %.2X %.2X %.2X %.2X %.2X %.2X\n",
			addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

	/*
		Place this address in the RAR if there is room,
		else put the controller into promiscuous mode
	*/
	if (hw->addr_ctrl.rar_used_count < rar_entries) {
		rar = hw->addr_ctrl.rar_used_count;
		hw->mac.ops.set_rar(hw, rar, addr, vmdq, XLNID_RAH_AV);
		hw_dbg(hw, "Added a secondary address to RAR[%d]\n", rar);
		hw->addr_ctrl.rar_used_count++;
	}

	else {
		hw->addr_ctrl.overflow_promisc++;
	}

	hw_dbg(hw, "xlnid_add_uc_addr Complete\n");
}

/**
		xlnid_update_uc_addr_list_generic - Updates MAC list of secondary addresses
		@hw: pointer to hardware structure
		@addr_list: the list of new addresses
		@addr_count: number of addresses
		@next: iterator function to walk the address list

		The given list replaces any existing list.  Clears the secondary addrs from
		receive address registers.  Uses unused receive address registers for the
		first secondary addresses, and falls back to promiscuous mode as needed.

		Drivers using secondary unicast addresses must set user_set_promisc when
		manually putting the device into promiscuous mode.
	**/
s32 xlnid_update_uc_addr_list_generic(struct xlnid_hw *hw, u8 *addr_list,
										u32 addr_count, xlnid_mc_addr_itr next)
{
	u8 *addr;
	u32 i;
	u32 old_promisc_setting = hw->addr_ctrl.overflow_promisc;
	u32 uc_addr_in_use;
	u32 fctrl;
	u32 vmdq;

	DEBUGFUNC("xlnid_update_uc_addr_list_generic");

	/*
		Clear accounting of old secondary address list,
		don't count RAR[0]
	*/
	uc_addr_in_use = hw->addr_ctrl.rar_used_count - 1;
	hw->addr_ctrl.rar_used_count -= uc_addr_in_use;
	hw->addr_ctrl.overflow_promisc = 0;

	/* Zero out the other receive addresses */
	hw_dbg(hw, "Clearing RAR[1-%d]\n", uc_addr_in_use + 1);

	for (i = 0; i < uc_addr_in_use; i++) {
		XLNID_WRITE_REG(hw, RAL(1 + i), 0);
		XLNID_WRITE_REG(hw, RAH(1 + i), 0);
	}

	/* Add the new addresses */
	for (i = 0; i < addr_count; i++) {
		hw_dbg(hw, " Adding the secondary addresses:\n");
		addr = next(hw, &addr_list, &vmdq);
		xlnid_add_uc_addr(hw, addr, vmdq);
	}

	if (hw->addr_ctrl.overflow_promisc) {
		/* enable promisc if not already in overflow or set by user */
		if (!old_promisc_setting && !hw->addr_ctrl.user_set_promisc) {
			hw_dbg(hw, " Entering address overflow promisc mode\n");
			fctrl = XLNID_READ_REG(hw, FCTRL);
			fctrl |= XLNID_FCTRL_UPE;
			XLNID_WRITE_REG(hw, FCTRL, fctrl);
		}
	}

	else {
		/* only disable if set by overflow, not by user */
		if (old_promisc_setting && !hw->addr_ctrl.user_set_promisc) {
			hw_dbg(hw, " Leaving address overflow promisc mode\n");
			fctrl = XLNID_READ_REG(hw, FCTRL);
			fctrl &= ~XLNID_FCTRL_UPE;
			XLNID_WRITE_REG(hw, FCTRL, fctrl);
		}
	}

	hw_dbg(hw, "xlnid_update_uc_addr_list_generic Complete\n");
	return XLNID_SUCCESS;
}

/**
		xlnid_mta_vector - Determines bit-vector in multicast table to set
		@hw: pointer to hardware structure
		@mc_addr: the multicast address

		Extracts the 12 bits, from a multicast address, to determine which
		bit-vector to set in the multicast table. The hardware uses 12 bits, from
		incoming rx multicast addresses, to determine the bit-vector to check in
		the MTA. Which of the 4 combination, of 12-bits, the hardware uses is set
		by the MO field of the MCSTCTRL. The MO field is set during initialization
		to mc_filter_type.
	**/
STATIC s32 xlnid_mta_vector(struct xlnid_hw *hw, u8 *mc_addr)
{
	u32 vector = 0;

	DEBUGFUNC("xlnid_mta_vector");

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
		hw_dbg(hw, "MC filter type param set incorrectly\n");
		ASSERT(0);
		break;
	}

	/* vector can only be 12-bits or boundary will be exceeded */
	vector &= 0xFFF;
	return vector;
}

/**
		xlnid_set_mta - Set bit-vector in multicast table
		@hw: pointer to hardware structure
		@mc_addr: Multicast address

		Sets the bit-vector in the multicast table.
	**/
void xlnid_set_mta(struct xlnid_hw *hw, u8 *mc_addr)
{
	u32 vector;
	u32 vector_bit;
	u32 vector_reg;

	DEBUGFUNC("xlnid_set_mta");

	hw->addr_ctrl.mta_in_use++;

	vector = xlnid_mta_vector(hw, mc_addr);
	hw_dbg(hw, " bit-vector = 0x%03X\n", vector);

	/*
		The MTA is a register array of 128 32-bit registers. It is treated
		like an array of 4096 bits.  We want to set bit
		BitArray[vector_value]. So we figure out what register the bit is
		in, read it, OR in the new bit, then write back the new value.  The
		register is determined by the upper 7 bits of the vector value and
		the bit within that register are determined by the lower 5 bits of
		the value.
	*/
	vector_reg = (vector >> 5) & 0x7F;
	vector_bit = vector & 0x1F;
	hw->mac.mta_shadow[vector_reg] |= (1 << vector_bit);
}

/**
		xlnid_update_mc_addr_list_generic - Updates MAC list of multicast addresses
		@hw: pointer to hardware structure
		@mc_addr_list: the list of new multicast addresses
		@mc_addr_count: number of addresses
		@next: iterator function to walk the multicast address list
		@clear: flag, when set clears the table beforehand

		When the clear flag is set, the given list replaces any existing list.
		Hashes the given addresses into the multicast table.
	**/
s32 xlnid_update_mc_addr_list_generic(struct xlnid_hw *hw, u8 *mc_addr_list,
										u32 mc_addr_count, xlnid_mc_addr_itr next,
										bool clear)
{
	u32 i;
	u32 vmdq;

	DEBUGFUNC("xlnid_update_mc_addr_list_generic");

	/*
		Set the new number of MC addresses that we are being requested to
		use.
	*/
	hw->addr_ctrl.num_mc_addrs = mc_addr_count;
	hw->addr_ctrl.mta_in_use = 0;

	/* Clear mta_shadow */
	if (clear) {
		hw_dbg(hw, " Clearing MTA\n");
		memset(&hw->mac.mta_shadow, 0, sizeof(hw->mac.mta_shadow));
	}

	/* Update mta_shadow */
	for (i = 0; i < mc_addr_count; i++) {
		hw_dbg(hw, " Adding the multicast addresses:\n");
		xlnid_set_mta(hw, next(hw, &mc_addr_list, &vmdq));
	}

	/* Enable mta */
	for (i = 0; i < hw->mac.mcft_size; i++)
		XLNID_WRITE_REG_ARRAY(hw, MTA(0), i,
								hw->mac.mta_shadow[i], false);

	if (hw->addr_ctrl.mta_in_use > 0)
		XLNID_WRITE_REG(hw, MCSTCTRL,
						XLNID_MCSTCTRL_MFE | hw->mac.mc_filter_type);

	hw_dbg(hw, "xlnid_update_mc_addr_list_generic Complete\n");
	return XLNID_SUCCESS;
}

/**
		xlnid_enable_mc_generic - Enable multicast address in RAR
		@hw: pointer to hardware structure

		Enables multicast address in RAR and the use of the multicast hash table.
	**/
s32 xlnid_enable_mc_generic(struct xlnid_hw *hw)
{
	struct xlnid_addr_filter_info *a = &hw->addr_ctrl;

	DEBUGFUNC("xlnid_enable_mc_generic");

	if (a->mta_in_use > 0)
		XLNID_WRITE_REG(hw, MCSTCTRL, XLNID_MCSTCTRL_MFE |
						hw->mac.mc_filter_type);

	return XLNID_SUCCESS;
}

/**
		xlnid_disable_mc_generic - Disable multicast address in RAR
		@hw: pointer to hardware structure

		Disables multicast address in RAR and the use of the multicast hash table.
	**/
s32 xlnid_disable_mc_generic(struct xlnid_hw *hw)
{
	struct xlnid_addr_filter_info *a = &hw->addr_ctrl;

	DEBUGFUNC("xlnid_disable_mc_generic");

	if (a->mta_in_use > 0) {
		XLNID_WRITE_REG(hw, MCSTCTRL, hw->mac.mc_filter_type);
	}

	return XLNID_SUCCESS;
}

/**
		xlnid_fc_enable_generic - Enable flow control
		@hw: pointer to hardware structure

		Enable flow control according to the current settings.
	**/
s32 xlnid_fc_enable_generic(struct xlnid_hw *hw)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_negotiate_fc - Negotiate flow control
		@hw: pointer to hardware structure
		@adv_reg: flow control advertised settings
		@lp_reg: link partner's flow control settings
		@adv_sym: symmetric pause bit in advertisement
		@adv_asm: asymmetric pause bit in advertisement
		@lp_sym: symmetric pause bit in link partner advertisement
		@lp_asm: asymmetric pause bit in link partner advertisement

		Find the intersection between advertised settings and link partner's
		advertised settings
	**/
s32 xlnid_negotiate_fc(struct xlnid_hw *hw, u32 adv_reg, u32 lp_reg,
						u32 adv_sym, u32 adv_asm, u32 lp_sym, u32 lp_asm)
{
	if ((!(adv_reg)) || (!(lp_reg))) {
		ERROR_REPORT3(XLNID_ERROR_UNSUPPORTED,
						"Local or link partner's advertised flow control "
						"settings are NULL. Local: %x, link partner: %x\n",
						adv_reg, lp_reg);
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

	else if (!(adv_reg & adv_sym) && (adv_reg & adv_asm) &&
				(lp_reg & lp_sym) && (lp_reg & lp_asm)) {
		hw->fc.current_mode = xlnid_fc_tx_pause;
		hw_dbg(hw, "Flow Control = TX PAUSE frames only.\n");
	}

	else if ((adv_reg & adv_sym) && (adv_reg & adv_asm) &&
				!(lp_reg & lp_sym) && (lp_reg & lp_asm)) {
		hw->fc.current_mode = xlnid_fc_rx_pause;
		hw_dbg(hw, "Flow Control = RX PAUSE frames only.\n");
	}

	else {
		hw->fc.current_mode = xlnid_fc_none;
		hw_dbg(hw, "Flow Control = NONE.\n");
	}

	return XLNID_SUCCESS;
}

/**
		xlnid_fc_autoneg - Configure flow control
		@hw: pointer to hardware structure

		Compares our advertised flow control capabilities to those advertised by
		our link partner, and determines the proper flow control mode to use.
	**/
void xlnid_fc_autoneg(struct xlnid_hw *hw)
{
	DEBUGFUNC("xlnid_fc_autoneg");

	hw->fc.fc_was_autonegged = false;
	hw->fc.current_mode = hw->fc.requested_mode;

	return;
}

/*
		xlnid_pcie_timeout_poll - Return number of times to poll for completion
		@hw: pointer to hardware structure

		System-wide timeout range is encoded in PCIe Device Control2 register.

		Add 10% to specified maximum and return the number of times to poll for
		completion timeout, in units of 100 microsec.  Never return less than
		800 = 80 millisec.
*/
STATIC u32 xlnid_pcie_timeout_poll(struct xlnid_hw *hw)
{
	s16 devctl2;
	u32 pollcnt;

	devctl2 = XLNID_READ_PCIE_WORD(hw, XLNID_PCI_DEVICE_CONTROL2);
	devctl2 &= XLNID_PCIDEVCTRL2_TIMEO_MASK;

	switch (devctl2) {
	case XLNID_PCIDEVCTRL2_65_130ms:
		pollcnt = 1300;     /* 130 millisec */
		break;

	case XLNID_PCIDEVCTRL2_260_520ms:
		pollcnt = 5200;     /* 520 millisec */
		break;

	case XLNID_PCIDEVCTRL2_1_2s:
		pollcnt = 20000;    /* 2 sec */
		break;

	case XLNID_PCIDEVCTRL2_4_8s:
		pollcnt = 80000;    /* 8 sec */
		break;

	case XLNID_PCIDEVCTRL2_17_34s:
		pollcnt = 34000;    /* 34 sec */
		break;

	case XLNID_PCIDEVCTRL2_50_100us:    /* 100 microsecs */
	case XLNID_PCIDEVCTRL2_1_2ms:   /* 2 millisecs */
	case XLNID_PCIDEVCTRL2_16_32ms: /* 32 millisec */
	case XLNID_PCIDEVCTRL2_16_32ms_def: /* 32 millisec default */
	default:
		pollcnt = 800;      /* 80 millisec minimum */
		break;
	}

	/* add 10% to spec maximum */
	return (pollcnt * 11) / 10;
}

/**
		xlnid_disable_pcie_primary - Disable PCI-express primary access
		@hw: pointer to hardware structure

		Disables PCI-Express primary access and verifies there are no pending
		requests. XLNID_ERR_PRIMARY_REQUESTS_PENDING is returned if primary disable
		bit hasn't caused the primary requests to be disabled, else XLNID_SUCCESS
		is returned signifying primary requests disabled.
	**/
s32 xlnid_disable_pcie_primary(struct xlnid_hw *hw)
{
	s32 status = XLNID_SUCCESS;
	u32 i, poll;
	u16 value;

	DEBUGFUNC("xlnid_disable_pcie_primary");

	/*
		Two consecutive resets are required via CTRL.RST per datasheet
		5.2.5.3.2 Primary Disable.  We set a flag to inform the reset routine
		of this need. The first reset prevents new primary requests from
		being issued by our device.  We then must wait 1usec or more for any
		remaining completions from the PCIe bus to trickle in, and then reset
		again to clear out any effects they may have had on our device.
	*/
	hw_dbg(hw, "GIO Primary Disable bit didn't clear - requesting resets\n");
	hw->mac.flags |= XLNID_FLAGS_DOUBLE_RESET_REQUIRED;

	/*
		Before proceeding, make sure that the PCIe block does not have
		transactions pending.
	*/
	poll = xlnid_pcie_timeout_poll(hw);

	for (i = 0; i < poll; i++) {
		usec_delay(100);
		value = XLNID_READ_PCIE_WORD(hw, XLNID_PCI_DEVICE_STATUS);

		if (XLNID_REMOVED(hw->hw_addr)) {
			goto out;
		}

		if (!(value & XLNID_PCI_DEVICE_STATUS_TRANSACTION_PENDING)) {
			goto out;
		}
	}

	ERROR_REPORT1(XLNID_ERROR_POLLING,
					"PCIe transaction pending bit also did not clear.\n");
	status = XLNID_ERR_PRIMARY_REQUESTS_PENDING;

out:
	return status;
}

/**
		xlnid_acquire_swfw_sync - Acquire SWFW semaphore
		@hw: pointer to hardware structure
		@mask: Mask to specify which semaphore to acquire

		Acquires the SWFW semaphore through the GSSR register for the specified
		function (CSR, PHY0, PHY1, EEPROM, Flash)
	**/
s32 xlnid_acquire_swfw_sync(struct xlnid_hw *hw, u32 mask)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_release_swfw_sync - Release SWFW semaphore
		@hw: pointer to hardware structure
		@mask: Mask to specify which semaphore to release

		Releases the SWFW semaphore through the GSSR register for the specified
		function (CSR, PHY0, PHY1, EEPROM, Flash)
	**/
void xlnid_release_swfw_sync(struct xlnid_hw *hw, u32 mask)
{
	return;
}

/**
		prot_autoc_read_generic - Hides MAC differences needed for AUTOC read
		@hw: pointer to hardware structure
		@locked: bool to indicate whether the SW/FW lock was taken
		@reg_val: Value we read from AUTOC

		The default case requires no protection so just to the register read.
*/
s32 prot_autoc_read_generic(struct xlnid_hw *hw, bool *locked, u32 *reg_val)
{
	*locked = false;
	*reg_val = 0;
	return XLNID_SUCCESS;
}

/**
		prot_autoc_write_generic - Hides MAC differences needed for AUTOC write
		@hw: pointer to hardware structure
		@reg_val: value to write to AUTOC
		@locked: bool to indicate whether the SW/FW lock was already taken by
				previous read.

		The default case requires no protection so just to the register write.
*/
s32 prot_autoc_write_generic(struct xlnid_hw *hw, u32 reg_val, bool locked)
{
	UNREFERENCED_1PARAMETER(locked);

	//XLNID_WRITE_REG(hw, XLNID_AUTOC, reg_val);
	return XLNID_SUCCESS;
}

/**
		xlnid_enable_rx_dma_generic - Enable the Rx DMA unit
		@hw: pointer to hardware structure
		@regval: register value to write to RXCTRL

		Enables the Rx DMA unit
	**/
s32 xlnid_enable_rx_dma_generic(struct xlnid_hw *hw, u32 regval)
{
	DEBUGFUNC("xlnid_enable_rx_dma_generic");

	if (regval & XLNID_RXCTRL_RXEN) {
		xlnid_enable_rx(hw);
	}

	else {
		xlnid_disable_rx(hw);
	}

	return XLNID_SUCCESS;
}

/**
		xlnid_blink_led_start_generic - Blink LED based on index.
		@hw: pointer to hardware structure
		@index: led number to blink
	**/
s32 xlnid_blink_led_start_generic(struct xlnid_hw *hw, u32 index)
{
	xlnid_link_speed speed = 0;
	bool link_up = 0;
	u32 autoc_reg = 0;
	u32 led_reg = 0;
	s32 ret_val = XLNID_SUCCESS;
	bool locked = false;

	DEBUGFUNC("xlnid_blink_led_start_generic");

	(void)index;

	if (hw->mac.type == xlnid_mac_WESTLAKE) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCMD, 0x0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCMD, 0x0);
	}

	led_reg = XLNID_READ_REG(hw, LEDCTL);

	/*
		Link must be up to auto-blink the LEDs;
		Force it if link is down.
	*/
	hw->mac.ops.check_link(hw, &speed, &link_up, false);

	if (!link_up) {
		ret_val = hw->mac.ops.prot_autoc_read(hw, &locked, &autoc_reg);

		if (ret_val != XLNID_SUCCESS) {
			goto out;
		}

		autoc_reg |= XLNID_AUTOC_AN_RESTART;
		autoc_reg |= XLNID_AUTOC_FLU;

		ret_val = hw->mac.ops.prot_autoc_write(hw, autoc_reg, locked);

		if (ret_val != XLNID_SUCCESS) {
			goto out;
		}

		XLNID_WRITE_FLUSH(hw);
		msec_delay(10);
	}

	if (hw->phy.media_type == xlnid_media_type_copper) {
		/* copper use led1 */
		led_reg &= ~XLNID_LED_MODE_MASK(1);
		led_reg |= XLNID_LED_BLINK(1);
	}

	else {
		/* fiber use led1 */
		led_reg &= ~XLNID_LED_MODE_MASK(0);
		led_reg |= XLNID_LED_BLINK(0);
	}

	XLNID_WRITE_REG(hw, LEDCTL, led_reg);

out:
	return ret_val;
}

/**
		xlnid_blink_led_stop_generic - Stop blinking LED based on index.
		@hw: pointer to hardware structure
		@index: led number to stop blinking
	**/
s32 xlnid_blink_led_stop_generic(struct xlnid_hw *hw, u32 index)
{
	u32 autoc_reg = 0;
	u32 led_reg = 0;
	s32 ret_val = XLNID_SUCCESS;
	bool locked = false;

	DEBUGFUNC("xlnid_blink_led_stop_generic");

	(void)index;

	if (hw->mac.type == xlnid_mac_WESTLAKE) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCMD, 0x0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_LEDCMD, 0x0);
	}

	led_reg = XLNID_READ_REG(hw, LEDCTL);

	ret_val = hw->mac.ops.prot_autoc_read(hw, &locked, &autoc_reg);

	if (ret_val != XLNID_SUCCESS) {
		goto out;
	}

	autoc_reg &= ~XLNID_AUTOC_FLU;
	autoc_reg |= XLNID_AUTOC_AN_RESTART;

	ret_val = hw->mac.ops.prot_autoc_write(hw, autoc_reg, locked);

	if (ret_val != XLNID_SUCCESS) {
		goto out;
	}

	if (hw->phy.media_type == xlnid_media_type_copper) {
		/* copper use led1 */
		led_reg &= ~XLNID_LED_MODE_MASK(1);
		led_reg &= ~XLNID_LED_BLINK(1);
		led_reg |= XLNID_LED_LINK_ACTIVE << XLNID_LED_MODE_SHIFT(1);
	}

	else {
		/* fiber use led0 */
		led_reg &= ~XLNID_LED_MODE_MASK(0);
		led_reg &= ~XLNID_LED_BLINK(0);
		led_reg |= XLNID_LED_LINK_ACTIVE << XLNID_LED_MODE_SHIFT(0);
	}

	XLNID_WRITE_REG(hw, LEDCTL, led_reg);

out:
	return ret_val;
}

/**
		xlnid_get_san_mac_addr_generic - SAN MAC address retrieval from the EEPROM
		@hw: pointer to hardware structure
		@san_mac_addr: SAN MAC address

		Reads the SAN MAC address from the EEPROM, if it's available.  This is
		per-port, so set_lan_id() must be called before reading the addresses.
		set_lan_id() is called by identify_sfp(), but this cannot be relied
		upon for non-SFP connections, so we must call it here.
	**/
s32 xlnid_get_san_mac_addr_generic(struct xlnid_hw *hw, u8 *san_mac_addr)
{
	u16 san_mac_offset_low = 0;
	u16 san_mac_offset_high = 0;
	u8 i = 0;
	s32 ret_val_low = 0;
	s32 ret_val_high = 0;
	u8 san_mac_data_low[4];
	u8 san_mac_data_high[4];

	DEBUGFUNC("xlnid_get_san_mac_addr_westlake");

	/* apply the port offset to the address offset */
	san_mac_offset_low = (hw->bus.func) ?
							XLNID_SAN_MAC_ADDR_PORT1_OFFSET_L : XLNID_SAN_MAC_ADDR_PORT0_OFFSET_L;
	san_mac_offset_high = (hw->bus.func) ?
							XLNID_SAN_MAC_ADDR_PORT1_OFFSET_H : XLNID_SAN_MAC_ADDR_PORT0_OFFSET_H;

	ret_val_high = hw->eeprom.ops.read_buffer(hw,
					san_mac_offset_high, 1, (u8 *) &san_mac_data_high);
	ret_val_low = hw->eeprom.ops.read_buffer(hw,
					san_mac_offset_low, 1, (u8 *) &san_mac_data_low);

	if (ret_val_low != 0 || ret_val_high != 0) {
		ERROR_REPORT2(XLNID_ERROR_INVALID_STATE, "eeprom read MAC failed");
		goto san_mac_addr_out;
	}

	for (i = 0; i < 4; i++) {
		san_mac_addr[i] = san_mac_data_low[3 - i];
	}

	for (i = 4; i < 6; i++) {
		san_mac_addr[i] = san_mac_data_high[7 - i];
	}

	return XLNID_SUCCESS;
san_mac_addr_out:

	/*
		No addresses available in this EEPROM.  It's not an
		error though, so just wipe the local address and return.
	*/
	for (i = 0; i < 6; i++) {
		san_mac_addr[i] = 0xFF;
	}

	return XLNID_SUCCESS;

}

/**
		xlnid_set_san_mac_addr_generic - Write the SAN MAC address to the EEPROM
		@hw: pointer to hardware structure
		@san_mac_addr: SAN MAC address

		Write a SAN MAC address to the EEPROM.
	**/
s32 xlnid_set_san_mac_addr_generic(struct xlnid_hw *hw, u8 *san_mac_addr)
{
	DEBUGFUNC("xlnid_set_san_mac_addr_generic");
	return XLNID_SUCCESS;
}

/**
		xlnid_get_pcie_msix_count_generic - Gets MSI-X vector count
		@hw: pointer to hardware structure

		Read PCIe configuration space, and get the MSI-X vector count from
		the capabilities table.
	**/
u16 xlnid_get_pcie_msix_count_generic(struct xlnid_hw *hw)
{
	u16 msix_count = 1;
	u16 max_msix_count;
	u16 pcie_offset;

	pcie_offset = XLNID_PCIE_MSIX_CAPS;
	max_msix_count = XLNID_MAX_MSIX_VECTORS;

	DEBUGFUNC("xlnid_get_pcie_msix_count_generic");
	msix_count = XLNID_READ_PCIE_WORD(hw, pcie_offset);

	if (XLNID_REMOVED(hw->hw_addr)) {
		msix_count = 0;
	}

	msix_count &= XLNID_PCIE_MSIX_TBL_SZ_MASK;

	/* MSI-X count is zero-based in HW */
	msix_count++;

	if (msix_count > max_msix_count) {
		msix_count = max_msix_count;
	}

	return msix_count;
}

/**
		xlnid_insert_mac_addr_generic - Find a RAR for this mac address
		@hw: pointer to hardware structure
		@addr: Address to put into receive address register
		@vmdq: VMDq pool to assign

		Puts an ethernet address into a receive address register, or
		finds the rar that it is aleady in; adds to the pool list
	**/
s32 xlnid_insert_mac_addr_generic(struct xlnid_hw *hw, u8 *addr, u32 vmdq)
{
	static const u32 NO_EMPTY_RAR_FOUND = 0xFFFFFFFF;
	u32 first_empty_rar = NO_EMPTY_RAR_FOUND;
	u32 rar;
	u32 rar_low, rar_high;
	u32 addr_low, addr_high;

	DEBUGFUNC("xlnid_insert_mac_addr_generic");

	/* swap bytes for HW little endian */
	addr_low  = addr[0] | (addr[1] << 8)
				| (addr[2] << 16)
				| (addr[3] << 24);
	addr_high = addr[4] | (addr[5] << 8);

	/*
		Either find the mac_id in rar or find the first empty space.
		rar_highwater points to just after the highest currently used
		rar in order to shorten the search.  It grows when we add a new
		rar to the top.
	*/
	for (rar = 0; rar < hw->mac.rar_highwater; rar++) {
		rar_high = XLNID_READ_REG(hw, RAH(rar));

		if (((XLNID_RAH_AV & rar_high) == 0)
				&& first_empty_rar == NO_EMPTY_RAR_FOUND) {
			first_empty_rar = rar;
		}

		else if ((rar_high & 0xFFFF) == addr_high) {
			rar_low = XLNID_READ_REG(hw, RAL(rar));

			if (rar_low == addr_low) {
				break;    /* found it already in the rars */
			}
		}
	}

	if (rar < hw->mac.rar_highwater) {
		/* already there so just add to the pool bits */
		xlnid_set_vmdq(hw, rar, vmdq);
	}

	else if (first_empty_rar != NO_EMPTY_RAR_FOUND) {
		/* stick it into first empty RAR slot we found */
		rar = first_empty_rar;
		xlnid_set_rar(hw, rar, addr, vmdq, XLNID_RAH_AV);
	}

	else if (rar == hw->mac.rar_highwater) {
		/* add it to the top of the list and inc the highwater mark */
		xlnid_set_rar(hw, rar, addr, vmdq, XLNID_RAH_AV);
		hw->mac.rar_highwater++;
	}

	else if (rar >= hw->mac.num_rar_entries) {
		return XLNID_ERR_INVALID_MAC_ADDR;
	}

	/*
		If we found rar[0], make sure the default pool bit (we use pool 0)
		remains cleared to be sure default pool packets will get delivered
	*/
	if (rar == 0) {
		xlnid_clear_vmdq(hw, rar, 0);
	}

	return rar;
}

/**
		xlnid_clear_vmdq_generic - Disassociate a VMDq pool index from a rx address
		@hw: pointer to hardware struct
		@rar: receive address register index to disassociate
		@vmdq: VMDq pool index to remove from the rar
	**/
s32 xlnid_clear_vmdq_generic(struct xlnid_hw *hw, u32 rar, u32 vmdq)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_set_vmdq_generic - Associate a VMDq pool index with a rx address
		@hw: pointer to hardware struct
		@rar: receive address register index to associate with a VMDq index
		@vmdq: VMDq pool index
	**/
s32 xlnid_set_vmdq_generic(struct xlnid_hw *hw, u32 rar, u32 vmdq)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_set_vmdq_san_mac_generic - Associate default VMDq pool index with
		a rx address
		@hw: pointer to hardware struct
		@vmdq: VMDq pool index

		This function should only be involved in the IOV mode.
		In IOV mode, Default pool is next pool after the number of
		VFs advertized and not 0.
		MPSAR table needs to be updated for SAN_MAC RAR [hw->mac.san_mac_rar_index]
	**/
s32 xlnid_set_vmdq_san_mac_generic(struct xlnid_hw *hw, u32 vmdq)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_init_uta_tables_generic - Initialize the Unicast Table Array
		@hw: pointer to hardware structure
	**/
s32 xlnid_init_uta_tables_generic(struct xlnid_hw *hw)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_set_vfta_generic - Set VLAN filter table
		@hw: pointer to hardware structure
		@vlan: VLAN id to write to VLAN filter
		@vind: VMDq output index that maps queue to VLAN id in VLVFB
		@vlan_on: boolean flag to turn on/off VLAN
		@vlvf_bypass: boolean flag indicating updating default pool is okay

		Turn on/off specified VLAN in the VLAN filter table.
	**/
s32 xlnid_set_vfta_generic(struct xlnid_hw *hw, u32 vlan, u32 vind,
							bool vlan_on, bool vlvf_bypass)
{
	u32 regidx, vfta_delta, vfta;
	s32 ret_val;

	DEBUGFUNC("xlnid_set_vfta_generic");

	if (vlan > 4095 || vind > 63) {
		return XLNID_ERR_PARAM;
	}

	/*
		this is a 2 part operation - first the VFTA, then the
		VLVF and VLVFB if VT Mode is set
		We don't write the VFTA until we know the VLVF part succeeded.
	*/

	/*  Part 1
		The VFTA is a bitstring made up of 128 32-bit registers
		that enable the particular VLAN id, much like the MTA:
			bits[11-5]: which register
			bits[4-0]:  which bit in the register
	*/
	regidx = vlan / 32;
	vfta_delta = 1 << (vlan % 32);
	vfta = XLNID_READ_REG(hw, VFTA(regidx));

	/*
		vfta_delta represents the difference between the current value
		of vfta and the value we want in the register.  Since the diff
		is an XOR mask we can just update the vfta using an XOR
	*/
	vfta_delta &= vlan_on ? ~vfta : vfta;
	vfta ^= vfta_delta;

	/*  Part 2
		Call xlnid_set_vlvf_generic to set VLVFB and VLVF
	*/
	ret_val = xlnid_set_vlvf_generic(hw, vlan, vind, vlan_on, &vfta_delta,
										vfta, vlvf_bypass);

	if (ret_val != XLNID_SUCCESS) {
		if (vlvf_bypass) {
			goto vfta_update;
		}

		return ret_val;
	}

vfta_update:

	/* Update VFTA now that we are ready for traffic */
	if (vfta_delta) {
		XLNID_WRITE_REG(hw, VFTA(regidx), vfta);
	}

	return XLNID_SUCCESS;
}

/**
		xlnid_set_vlvf_generic - Set VLAN Pool Filter
		@hw: pointer to hardware structure
		@vlan: VLAN id to write to VLAN filter
		@vind: VMDq output index that maps queue to VLAN id in VLVFB
		@vlan_on: boolean flag to turn on/off VLAN in VLVF
		@vfta_delta: pointer to the difference between the current value of VFTA
				and the desired value
		@vfta: the desired value of the VFTA
		@vlvf_bypass: boolean flag indicating updating default pool is okay

		Turn on/off specified bit in VLVF table.
	**/
s32 xlnid_set_vlvf_generic(struct xlnid_hw *hw, u32 vlan, u32 vind,
							bool vlan_on, u32 *vfta_delta, u32 vfta,
							bool vlvf_bypass)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_clear_vfta_generic - Clear VLAN filter table
		@hw: pointer to hardware structure

		Clears the VLAN filer table
	**/
s32 xlnid_clear_vfta_generic(struct xlnid_hw *hw)
{
	u32 offset;

	DEBUGFUNC("xlnid_clear_vfta_generic");

	for (offset = 0; offset < hw->mac.vft_size; offset++) {
		XLNID_WRITE_REG(hw, VFTA(offset), 0);
	}

	return XLNID_SUCCESS;
}

/**
		xlnid_check_mac_link_generic - Determine link and speed status
		@hw: pointer to hardware structure
		@speed: pointer to link speed
		@link_up: true when link is up
		@link_up_wait_to_complete: bool used to wait for link up or not

		Reads the links register to determine if link is up and the current speed
	**/
s32 xlnid_check_mac_link_generic(struct xlnid_hw *hw, xlnid_link_speed *speed,
									bool *link_up, bool link_up_wait_to_complete)
{
	DEBUGFUNC("xlnid_check_mac_link_generic");

	*link_up = true;

#ifdef XLNID_XGE_SUPPORT
	*speed = XLNID_LINK_SPEED_10GB_FULL;
#else
	*speed = XLNID_LINK_SPEED_1GB_FULL;
#endif

	return XLNID_SUCCESS;
}

/**
		xlnid_get_wwn_prefix_generic - Get alternative WWNN/WWPN prefix from
		the EEPROM
		@hw: pointer to hardware structure
		@wwnn_prefix: the alternative WWNN prefix
		@wwpn_prefix: the alternative WWPN prefix

		This function will read the EEPROM from the alternative SAN MAC address
		block to check the support for the alternative WWNN/WWPN prefix support.
	**/
s32 xlnid_get_wwn_prefix_generic(struct xlnid_hw *hw, u16 *wwnn_prefix,
									u16 *wwpn_prefix)
{
	DEBUGFUNC("xlnid_get_wwn_prefix_generic");
	return XLNID_SUCCESS;
}

/**
		xlnid_get_fcoe_boot_status_generic - Get FCOE boot status from EEPROM
		@hw: pointer to hardware structure
		@bs: the fcoe boot status

		This function will read the FCOE boot status from the iSCSI FCOE block
	**/
s32 xlnid_get_fcoe_boot_status_generic(struct xlnid_hw *hw, u16 *bs)
{
	DEBUGFUNC("xlnid_get_fcoe_boot_status_generic");
	return XLNID_SUCCESS;
}

/**
		xlnid_set_mac_anti_spoofing - Enable/Disable MAC anti-spoofing
		@hw: pointer to hardware structure
		@enable: enable or disable switch for MAC anti-spoofing
		@vf: Virtual Function pool - VF Pool to set for MAC anti-spoofing

	**/
void xlnid_set_mac_anti_spoofing(struct xlnid_hw *hw, bool enable, int vf)
{
	return;
}

/**
		xlnid_set_vlan_anti_spoofing - Enable/Disable VLAN anti-spoofing
		@hw: pointer to hardware structure
		@enable: enable or disable switch for VLAN anti-spoofing
		@vf: Virtual Function pool - VF Pool to set for VLAN anti-spoofing

	**/
void xlnid_set_vlan_anti_spoofing(struct xlnid_hw *hw, bool enable, int vf)
{
	return;
}

/**
		xlnid_get_device_caps_generic - Get additional device capabilities
		@hw: pointer to hardware structure
		@device_caps: the EEPROM word with the extra device capabilities

		This function will read the EEPROM location for the device capabilities,
		and return the word through device_caps.
	**/
s32 xlnid_get_device_caps_generic(struct xlnid_hw *hw, u16 *device_caps)
{
	DEBUGFUNC("xlnid_get_device_caps_generic");

	//hw->eeprom.ops.read(hw, XLNID_DEVICE_CAPS, device_caps);

	return XLNID_SUCCESS;
}

/**
		xlnid_set_fw_drv_ver_generic - Sends driver version to firmware
		@hw: pointer to the HW structure
		@maj: driver version major number
		@min: driver version minor number
		@build: driver version build number
		@sub: driver version sub build number
		@len: unused
		@driver_ver: unused

		Sends driver version number to firmware through the manageability
		block.  On success return XLNID_SUCCESS
		else returns XLNID_ERR_SWFW_SYNC when encountering an error acquiring
		semaphore or XLNID_ERR_HOST_INTERFACE_COMMAND when command fails.
	**/
s32 xlnid_set_fw_drv_ver_generic(struct xlnid_hw *hw, u8 maj, u8 min,
									u8 build, u8 sub, u16 len,
									const char *driver_ver)
{
	return XLNID_SUCCESS;
}

/**
		xlnid_set_rxpba_generic - Initialize Rx packet buffer
		@hw: pointer to hardware structure
		@num_pb: number of packet buffers to allocate
		@headroom: reserve n KB of headroom
		@strategy: packet buffer allocation strategy
	**/
void xlnid_set_rxpba_generic(struct xlnid_hw *hw, int num_pb, u32 headroom,
								int strategy)
{
	return;
}

/**
		xlnid_clear_tx_pending - Clear pending TX work from the PCIe fifo
		@hw: pointer to the hardware structure

		The skylake can experience issues if TX work is still pending
		when a reset occurs.  This function prevents this by flushing the PCIe
		buffers on the system.
	**/
void xlnid_clear_tx_pending(struct xlnid_hw *hw)
{
	u32 hlreg0, i, poll;
	u16 value;

	/*
		If double reset is not requested then all transactions should
		already be clear and as such there is no work to do
	*/
	if (!(hw->mac.flags & XLNID_FLAGS_DOUBLE_RESET_REQUIRED)) {
		return;
	}

	/*
		Set loopback enable to prevent any transmits from being sent
		should the link come up.  This assumes that the RXCTRL.RXEN bit
		has already been cleared.
	*/
	hlreg0 = XLNID_READ_REG(hw, HLREG0);
	XLNID_WRITE_REG(hw, HLREG0, hlreg0 | XLNID_HLREG0_LPBK);

	/* Wait for a last completion before clearing buffers */
	XLNID_WRITE_FLUSH(hw);
	msec_delay(3);

	/*
		Before proceeding, make sure that the PCIe block does not have
		transactions pending.
	*/
	poll = xlnid_pcie_timeout_poll(hw);

	for (i = 0; i < poll; i++) {
		usec_delay(100);
		value = XLNID_READ_PCIE_WORD(hw, XLNID_PCI_DEVICE_STATUS);

		if (XLNID_REMOVED(hw->hw_addr)) {
			goto out;
		}

		if (!(value & XLNID_PCI_DEVICE_STATUS_TRANSACTION_PENDING)) {
			goto out;
		}
	}

out:
	XLNID_WRITE_REG(hw, HLREG0, hlreg0);
}

STATIC const u8 xlnid_emc_temp_data[4] = {
	XLNID_EMC_INTERNAL_DATA,
	XLNID_EMC_DIODE1_DATA,
	XLNID_EMC_DIODE2_DATA,
	XLNID_EMC_DIODE3_DATA
};

STATIC const u8 xlnid_emc_therm_limit[4] = {
	XLNID_EMC_INTERNAL_THERM_LIMIT,
	XLNID_EMC_DIODE1_THERM_LIMIT,
	XLNID_EMC_DIODE2_THERM_LIMIT,
	XLNID_EMC_DIODE3_THERM_LIMIT
};

/**
		xlnid_get_thermal_sensor_data - Gathers thermal sensor data
		@hw: pointer to hardware structure

		Returns the thermal sensor data structure
	**/
s32 xlnid_get_thermal_sensor_data_generic(struct xlnid_hw *hw)
{
	return XLNID_NOT_IMPLEMENTED;
}

/**
		xlnid_init_thermal_sensor_thresh_generic - Inits thermal sensor thresholds
		@hw: pointer to hardware structure

		Inits the thermal sensor thresholds according to the NVM map
		and save off the threshold and location values into mac.thermal_sensor_data
	**/
s32 xlnid_init_thermal_sensor_thresh_generic(struct xlnid_hw *hw)
{
	return XLNID_NOT_IMPLEMENTED;
}

/**
		xlnid_dcb_get_rtrup2tc_generic - read rtrup2tc reg
		@hw: pointer to hardware structure
		@map: pointer to u8 arr for returning map

		Read the rtrup2tc HW register and resolve its content into map
	**/
void xlnid_dcb_get_rtrup2tc_generic(struct xlnid_hw *hw, u8 *map)
{
	*map = 0;
	return;
}

void xlnid_disable_rx_generic(struct xlnid_hw *hw)
{
	u32 rxctrl;

	rxctrl = XLNID_READ_REG(hw, RXCTRL);

	rxctrl &= ~XLNID_RXCTRL_RXEN;
	XLNID_WRITE_REG(hw, RXCTRL, rxctrl);
}

void xlnid_enable_rx_generic(struct xlnid_hw *hw)
{
	u32 rxctrl;

	rxctrl = XLNID_READ_REG(hw, RXCTRL);
	XLNID_WRITE_REG(hw, RXCTRL, (rxctrl | XLNID_RXCTRL_RXEN));
}

/**
		xlnid_mng_present - returns true when management capability is present
		@hw: pointer to hardware structure
*/
bool xlnid_mng_present(struct xlnid_hw *hw)
{
	return false;
}

/**
		xlnid_mng_enabled - Is the manageability engine enabled?
		@hw: pointer to hardware structure

		Returns true if the manageability engine is enabled.
	**/
bool xlnid_mng_enabled(struct xlnid_hw *hw)
{
	u32 manc;

	manc = XLNID_READ_REG(hw, MANC);

	if (!(manc & XLNID_MANC_RCV_TCO_EN)) {
		return false;
	}

	return true;
}

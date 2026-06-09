/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2008 - 2024 Xel Technology. */

#ifndef _XLNID_COMMON_H_
#define _XLNID_COMMON_H_

#include "xlnid_type.h"

void xlnid_dcb_get_rtrup2tc_generic(struct xlnid_hw *hw, u8 *map);

u16 xlnid_get_pcie_msix_count_generic(struct xlnid_hw *hw);
s32 xlnid_init_ops_generic(struct xlnid_hw *hw);
s32 xlnid_init_hw_generic(struct xlnid_hw *hw);
s32 xlnid_start_hw_generic(struct xlnid_hw *hw);
s32 xlnid_clear_hw_cntrs_generic(struct xlnid_hw *hw);
s32 xlnid_read_pba_string_generic(struct xlnid_hw *hw, u8 *pba_num,
									u32 pba_num_size);
s32 xlnid_get_mac_addr_generic(struct xlnid_hw *hw, u8 *mac_addr);
s32 xlnid_get_bus_info_generic(struct xlnid_hw *hw);
void xlnid_set_pci_config_data_generic(struct xlnid_hw *hw, u16 link_status);
void xlnid_set_lan_id_multi_port_pcie(struct xlnid_hw *hw);
s32 xlnid_stop_adapter_generic(struct xlnid_hw *hw);

s32 xlnid_led_on_generic(struct xlnid_hw *hw, u32 index);
s32 xlnid_led_off_generic(struct xlnid_hw *hw, u32 index);
s32 xlnid_init_led_link_act_generic(struct xlnid_hw *hw);

s32 xlnid_init_eeprom_params_generic(struct xlnid_hw *hw);
s32 xlnid_write_eeprom_generic(struct xlnid_hw *hw, u16 offset, u16 data);
s32 xlnid_read_eerd_generic(struct xlnid_hw *hw, u16 offset, u8 *data);
s32 xlnid_read_eerd_buffer_generic(struct xlnid_hw *hw, u16 offset,
									u16 words, u8 *data);
s32 xlnid_write_eewr_generic(struct xlnid_hw *hw, u16 offset, u16 data);
s32 xlnid_write_eewr_buffer_generic(struct xlnid_hw *hw, u16 offset,
									u16 words, u16 *data);
s32 xlnid_read_eeprom_bit_bang_generic(struct xlnid_hw *hw, u16 offset,
										u8 *data);
s32 xlnid_read_eeprom_buffer_bit_bang_generic(struct xlnid_hw *hw, u16 offset,
		u16 words, u8 *data);
s32 xlnid_calc_eeprom_checksum_generic(struct xlnid_hw *hw);
s32 xlnid_validate_eeprom_checksum_generic(struct xlnid_hw *hw,
		u16 *checksum_val);
s32 xlnid_update_eeprom_checksum_generic(struct xlnid_hw *hw);
s32 xlnid_poll_eerd_eewr_done(struct xlnid_hw *hw, u32 ee_reg);

s32 xlnid_set_rar_generic(struct xlnid_hw *hw, u32 index, u8 *addr, u32 vmdq,
							u32 enable_addr);
s32 xlnid_clear_rar_generic(struct xlnid_hw *hw, u32 index);
s32 xlnid_init_rx_addrs_generic(struct xlnid_hw *hw);
s32 xlnid_update_mc_addr_list_generic(struct xlnid_hw *hw, u8 *mc_addr_list,
										u32 mc_addr_count,
										xlnid_mc_addr_itr func, bool clear);
s32 xlnid_update_uc_addr_list_generic(struct xlnid_hw *hw, u8 *addr_list,
										u32 addr_count, xlnid_mc_addr_itr func);
s32 xlnid_enable_mc_generic(struct xlnid_hw *hw);
s32 xlnid_disable_mc_generic(struct xlnid_hw *hw);
s32 xlnid_enable_rx_dma_generic(struct xlnid_hw *hw, u32 regval);

s32 xlnid_fc_enable_generic(struct xlnid_hw *hw);
bool xlnid_device_supports_autoneg_fc(struct xlnid_hw *hw);
void xlnid_fc_autoneg(struct xlnid_hw *hw);
s32 xlnid_setup_fc_generic(struct xlnid_hw *hw);

s32 xlnid_validate_mac_addr(u8 *mac_addr);
s32 xlnid_acquire_swfw_sync(struct xlnid_hw *hw, u32 mask);
void xlnid_release_swfw_sync(struct xlnid_hw *hw, u32 mask);
s32 xlnid_disable_pcie_primary(struct xlnid_hw *hw);

s32 prot_autoc_read_generic(struct xlnid_hw *hw, bool *, u32 *reg_val);
s32 prot_autoc_write_generic(struct xlnid_hw *hw, u32 reg_val, bool locked);

s32 xlnid_blink_led_start_generic(struct xlnid_hw *hw, u32 index);
s32 xlnid_blink_led_stop_generic(struct xlnid_hw *hw, u32 index);

s32 xlnid_get_san_mac_addr_generic(struct xlnid_hw *hw, u8 *san_mac_addr);
s32 xlnid_set_san_mac_addr_generic(struct xlnid_hw *hw, u8 *san_mac_addr);

s32 xlnid_set_vmdq_generic(struct xlnid_hw *hw, u32 rar, u32 vmdq);
s32 xlnid_set_vmdq_san_mac_generic(struct xlnid_hw *hw, u32 vmdq);
s32 xlnid_clear_vmdq_generic(struct xlnid_hw *hw, u32 rar, u32 vmdq);
s32 xlnid_insert_mac_addr_generic(struct xlnid_hw *hw, u8 *addr, u32 vmdq);
s32 xlnid_init_uta_tables_generic(struct xlnid_hw *hw);
s32 xlnid_set_vfta_generic(struct xlnid_hw *hw, u32 vlan,
							u32 vind, bool vlan_on, bool vlvf_bypass);
s32 xlnid_set_vlvf_generic(struct xlnid_hw *hw, u32 vlan, u32 vind,
							bool vlan_on, u32 *vfta_delta, u32 vfta,
							bool vlvf_bypass);
s32 xlnid_clear_vfta_generic(struct xlnid_hw *hw);

s32 xlnid_check_mac_link_generic(struct xlnid_hw *hw,
									xlnid_link_speed *speed,
									bool *link_up, bool link_up_wait_to_complete);

s32 xlnid_get_wwn_prefix_generic(struct xlnid_hw *hw, u16 *wwnn_prefix,
									u16 *wwpn_prefix);

s32 xlnid_get_fcoe_boot_status_generic(struct xlnid_hw *hw, u16 *bs);
void xlnid_set_mac_anti_spoofing(struct xlnid_hw *hw, bool enable, int vf);
void xlnid_set_vlan_anti_spoofing(struct xlnid_hw *hw, bool enable, int vf);
s32 xlnid_get_device_caps_generic(struct xlnid_hw *hw, u16 *device_caps);
void xlnid_set_rxpba_generic(struct xlnid_hw *hw, int num_pb, u32 headroom,
								int strategy);
s32 xlnid_set_fw_drv_ver_generic(struct xlnid_hw *hw, u8 maj, u8 min,
									u8 build, u8 ver, u16 len, const char *str);
s32 xlnid_shutdown_fw_phy(struct xlnid_hw *);
s32 xlnid_fw_phy_activity(struct xlnid_hw *, u16 activity,
							u32(*data)[FW_PHY_ACT_DATA_COUNT]);
void xlnid_clear_tx_pending(struct xlnid_hw *hw);

extern void xlnid_stop_mac_link_on_d3_skylake(struct xlnid_hw *hw);
bool xlnid_mng_present(struct xlnid_hw *hw);
bool xlnid_mng_enabled(struct xlnid_hw *hw);

#define XLNID_I2C_THERMAL_SENSOR_ADDR   0xF8
#define XLNID_EMC_INTERNAL_DATA     0x00
#define XLNID_EMC_INTERNAL_THERM_LIMIT  0x20
#define XLNID_EMC_DIODE1_DATA       0x01
#define XLNID_EMC_DIODE1_THERM_LIMIT    0x19
#define XLNID_EMC_DIODE2_DATA       0x23
#define XLNID_EMC_DIODE2_THERM_LIMIT    0x1A
#define XLNID_EMC_DIODE3_DATA       0x2A
#define XLNID_EMC_DIODE3_THERM_LIMIT    0x30

s32 xlnid_get_thermal_sensor_data_generic(struct xlnid_hw *hw);
s32 xlnid_init_thermal_sensor_thresh_generic(struct xlnid_hw *hw);

void xlnid_disable_rx_generic(struct xlnid_hw *hw);
void xlnid_enable_rx_generic(struct xlnid_hw *hw);
#endif /* XLNID_COMMON */

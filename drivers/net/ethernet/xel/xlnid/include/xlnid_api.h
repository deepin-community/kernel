/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2008 - 2023 Xel Technology. */

#ifndef _XLNID_API_H_
#define _XLNID_API_H_

#include "xlnid_type.h"

void xlnid_dcb_get_rtrup2tc(struct xlnid_hw *hw, u8 *map);

s32 xlnid_init_shared_code(struct xlnid_hw *hw);

extern s32 xlnid_init_ops_westlake(struct xlnid_hw *hw);

s32 xlnid_set_mac_type(struct xlnid_hw *hw, bool report);
s32 xlnid_init_hw(struct xlnid_hw *hw);
s32 xlnid_reset_hw(struct xlnid_hw *hw);
s32 xlnid_start_hw(struct xlnid_hw *hw);
s32 xlnid_clear_hw_cntrs(struct xlnid_hw *hw);
enum xlnid_media_type xlnid_get_media_type(struct xlnid_hw *hw);
s32 xlnid_get_mac_addr(struct xlnid_hw *hw, u8 *mac_addr);
s32 xlnid_get_bus_info(struct xlnid_hw *hw);
u32 xlnid_get_num_of_tx_queues(struct xlnid_hw *hw);
u32 xlnid_get_num_of_rx_queues(struct xlnid_hw *hw);
s32 xlnid_stop_adapter(struct xlnid_hw *hw);

s32 xlnid_identify_phy(struct xlnid_hw *hw);
s32 xlnid_reset_phy(struct xlnid_hw *hw);
s32 xlnid_read_phy_reg(struct xlnid_hw *hw, u32 reg_addr, u32 device_type,
						u16 *phy_data);
s32 xlnid_write_phy_reg(struct xlnid_hw *hw, u32 reg_addr, u32 device_type,
						u16 phy_data);

s32 xlnid_setup_phy_link(struct xlnid_hw *hw);
s32 xlnid_setup_internal_phy(struct xlnid_hw *hw);
s32 xlnid_check_phy_link(struct xlnid_hw *hw,
							xlnid_link_speed *speed,
							bool *link_up);
s32 xlnid_setup_phy_link_speed(struct xlnid_hw *hw,
								xlnid_link_speed speed,
								bool autoneg_wait_to_complete);
s32 xlnid_set_phy_power(struct xlnid_hw *hw, bool on);
void xlnid_disable_tx_laser(struct xlnid_hw *hw);
void xlnid_enable_tx_laser(struct xlnid_hw *hw);
void xlnid_flap_tx_laser(struct xlnid_hw *hw);
s32 xlnid_setup_link(struct xlnid_hw *hw, xlnid_link_speed speed,
						bool autoneg_wait_to_complete);
s32 xlnid_setup_mac_link(struct xlnid_hw *hw, xlnid_link_speed speed,
							bool autoneg_wait_to_complete);
s32 xlnid_check_link(struct xlnid_hw *hw, xlnid_link_speed *speed,
						bool *link_up, bool link_up_wait_to_complete);
s32 xlnid_get_link_capabilities(struct xlnid_hw *hw, xlnid_link_speed *speed,
								bool *autoneg);
s32 xlnid_led_on(struct xlnid_hw *hw, u32 index);
s32 xlnid_led_off(struct xlnid_hw *hw, u32 index);
s32 xlnid_blink_led_start(struct xlnid_hw *hw, u32 index);
s32 xlnid_blink_led_stop(struct xlnid_hw *hw, u32 index);

s32 xlnid_init_eeprom_params(struct xlnid_hw *hw);
s32 xlnid_write_eeprom(struct xlnid_hw *hw, u16 offset, u16 data);
s32 xlnid_write_eeprom_buffer(struct xlnid_hw *hw, u16 offset,
								u16 bytes, u8 *data);
s32 xlnid_read_eeprom(struct xlnid_hw *hw, u16 offset, u8 *data);
s32 xlnid_read_eeprom_buffer(struct xlnid_hw *hw, u16 offset,
								u16 words, u8 *data);

s32 xlnid_validate_eeprom_checksum(struct xlnid_hw *hw, u16 *checksum_val);
s32 xlnid_update_eeprom_checksum(struct xlnid_hw *hw);

s32 xlnid_insert_mac_addr(struct xlnid_hw *hw, u8 *addr, u32 vmdq);
s32 xlnid_set_rar(struct xlnid_hw *hw, u32 index, u8 *addr, u32 vmdq,
					u32 enable_addr);
s32 xlnid_clear_rar(struct xlnid_hw *hw, u32 index);
s32 xlnid_set_vmdq(struct xlnid_hw *hw, u32 rar, u32 vmdq);
s32 xlnid_set_vmdq_san_mac(struct xlnid_hw *hw, u32 vmdq);
s32 xlnid_clear_vmdq(struct xlnid_hw *hw, u32 rar, u32 vmdq);
s32 xlnid_init_rx_addrs(struct xlnid_hw *hw);
u32 xlnid_get_num_rx_addrs(struct xlnid_hw *hw);
s32 xlnid_update_uc_addr_list(struct xlnid_hw *hw, u8 *addr_list,
								u32 addr_count, xlnid_mc_addr_itr func);
s32 xlnid_update_mc_addr_list(struct xlnid_hw *hw, u8 *mc_addr_list,
								u32 mc_addr_count, xlnid_mc_addr_itr func,
								bool clear);
void xlnid_add_uc_addr(struct xlnid_hw *hw, u8 *addr_list, u32 vmdq);
s32 xlnid_enable_mc(struct xlnid_hw *hw);
s32 xlnid_disable_mc(struct xlnid_hw *hw);
s32 xlnid_clear_vfta(struct xlnid_hw *hw);
s32 xlnid_set_vfta(struct xlnid_hw *hw, u32 vlan,
					u32 vind, bool vlan_on, bool vlvf_bypass);
s32 xlnid_set_vlvf(struct xlnid_hw *hw, u32 vlan, u32 vind,
					bool vlan_on, u32 *vfta_delta, u32 vfta,
					bool vlvf_bypass);
s32 xlnid_fc_enable(struct xlnid_hw *hw);
s32 xlnid_setup_fc(struct xlnid_hw *hw);
s32 xlnid_set_fw_drv_ver(struct xlnid_hw *hw, u8 maj, u8 min, u8 build,
							u8 ver, u16 len, char *driver_ver);
s32 xlnid_get_thermal_sensor_data(struct xlnid_hw *hw);
s32 xlnid_init_thermal_sensor_thresh(struct xlnid_hw *hw);
void xlnid_set_mta(struct xlnid_hw *hw, u8 *mc_addr);
s32 xlnid_get_phy_firmware_version(struct xlnid_hw *hw,
									u16 *firmware_version);
s32 xlnid_read_analog_reg8(struct xlnid_hw *hw, u32 reg, u8 *val);
s32 xlnid_write_analog_reg8(struct xlnid_hw *hw, u32 reg, u8 val);
s32 xlnid_init_uta_tables(struct xlnid_hw *hw);
s32 xlnid_read_i2c_eeprom(struct xlnid_hw *hw, u8 byte_offset, u8 *eeprom_data);
u64 xlnid_get_supported_physical_layer(struct xlnid_hw *hw);
s32 xlnid_enable_rx_dma(struct xlnid_hw *hw, u32 regval);
s32 xlnid_disable_sec_rx_path(struct xlnid_hw *hw);
s32 xlnid_enable_sec_rx_path(struct xlnid_hw *hw);
s32 xlnid_mng_fw_enabled(struct xlnid_hw *hw);
s32 xlnid_reinit_fdir_tables(struct xlnid_hw *hw);
s32 xlnid_init_fdir_signature(struct xlnid_hw *hw, u32 fdirctrl);
s32 xlnid_init_fdir_perfect(struct xlnid_hw *hw, u32 fdirctrl,
							bool cloud_mode);
void xlnid_fdir_add_signature_filter(struct xlnid_hw *hw,
										union xlnid_atr_hash_dword input,
										union xlnid_atr_hash_dword common,
										u8 queue);
s32 xlnid_fdir_set_input_mask(struct xlnid_hw *hw,
								union xlnid_atr_input *input_mask, bool cloud_mode);
s32 xlnid_fdir_write_perfect_filter(struct xlnid_hw *hw,
									union xlnid_atr_input *input,
									u16 soft_id, u8 queue, bool cloud_mode);
s32 xlnid_fdir_remove_perfect_filter(struct xlnid_hw *hw,
										union xlnid_atr_input *input,
										u16 soft_id, u8 queue, bool cloud_mode);
s32 xlnid_fdir_erase_perfect_filter(struct xlnid_hw *hw,
									union xlnid_atr_input *input,
									u16 soft_id);
s32 xlnid_fdir_add_perfect_filter(struct xlnid_hw *hw,
									union xlnid_atr_input *input,
									union xlnid_atr_input *mask,
									u16 soft_id,
									u8 queue,
									bool cloud_mode);
void xlnid_atr_compute_perfect_hash(struct xlnid_hw *hw,
									union xlnid_atr_input *input,
									union xlnid_atr_input *mask);
u32 xlnid_atr_compute_sig_hash(struct xlnid_hw *hw,
								union xlnid_atr_hash_dword input,
								union xlnid_atr_hash_dword common);
s32 xlnid_read_i2c_byte(struct xlnid_hw *hw, u8 byte_offset, u8 dev_addr,
						u8 *data);
s32 xlnid_read_i2c_byte_unlocked(struct xlnid_hw *hw, u8 byte_offset,
									u8 dev_addr, u8 *data);
s32 xlnid_read_link(struct xlnid_hw *hw, u8 addr, u16 reg, u16 *val);
s32 xlnid_read_link_unlocked(struct xlnid_hw *hw, u8 addr, u16 reg, u16 *val);
s32 xlnid_write_i2c_byte(struct xlnid_hw *hw, u8 byte_offset, u8 dev_addr,
							u8 data);
void xlnid_set_fdir_drop_queue_skylake(struct xlnid_hw *hw, u8 dropqueue);
s32 xlnid_write_i2c_byte_unlocked(struct xlnid_hw *hw, u8 byte_offset,
									u8 dev_addr, u8 data);
s32 xlnid_write_link(struct xlnid_hw *hw, u8 addr, u16 reg, u16 val);
s32 xlnid_write_link_unlocked(struct xlnid_hw *hw, u8 addr, u16 reg, u16 val);
s32 xlnid_write_i2c_eeprom(struct xlnid_hw *hw, u8 byte_offset, u8 eeprom_data);
s32 xlnid_get_san_mac_addr(struct xlnid_hw *hw, u8 *san_mac_addr);
s32 xlnid_set_san_mac_addr(struct xlnid_hw *hw, u8 *san_mac_addr);
s32 xlnid_get_device_caps(struct xlnid_hw *hw, u16 *device_caps);
s32 xlnid_acquire_swfw_semaphore(struct xlnid_hw *hw, u32 mask);
void xlnid_release_swfw_semaphore(struct xlnid_hw *hw, u32 mask);
void xlnid_init_swfw_semaphore(struct xlnid_hw *hw);
s32 xlnid_get_wwn_prefix(struct xlnid_hw *hw, u16 *wwnn_prefix,
							u16 *wwpn_prefix);
s32 xlnid_get_fcoe_boot_status(struct xlnid_hw *hw, u16 *bs);
s32 xlnid_dmac_config(struct xlnid_hw *hw);
s32 xlnid_dmac_update_tcs(struct xlnid_hw *hw);
s32 xlnid_dmac_config_tcs(struct xlnid_hw *hw);
s32 xlnid_setup_eee(struct xlnid_hw *hw, bool enable_eee);
void xlnid_set_source_address_pruning(struct xlnid_hw *hw, bool enable,
										unsigned int vf);
void xlnid_set_ethertype_anti_spoofing(struct xlnid_hw *hw, bool enable,
										int vf);
s32 xlnid_read_iosf_sb_reg(struct xlnid_hw *hw, u32 reg_addr,
							u32 device_type, u32 *phy_data);
s32 xlnid_write_iosf_sb_reg(struct xlnid_hw *hw, u32 reg_addr,
							u32 device_type, u32 phy_data);
void xlnid_disable_mdd(struct xlnid_hw *hw);
void xlnid_enable_mdd(struct xlnid_hw *hw);
void xlnid_mdd_event(struct xlnid_hw *hw, u32 *vf_bitmap);
void xlnid_restore_mdd_vf(struct xlnid_hw *hw, u32 vf);
bool xlnid_fw_recovery_mode(struct xlnid_hw *hw);
s32 xlnid_enter_lplu(struct xlnid_hw *hw);
s32 xlnid_handle_lasi(struct xlnid_hw *hw);
void xlnid_set_rate_select_speed(struct xlnid_hw *hw, xlnid_link_speed speed);
void xlnid_disable_rx(struct xlnid_hw *hw);
void xlnid_enable_rx(struct xlnid_hw *hw);
s32 xlnid_negotiate_fc(struct xlnid_hw *hw, u32 adv_reg, u32 lp_reg,
						u32 adv_sym, u32 adv_asm, u32 lp_sym, u32 lp_asm);

u32 xlnid_read_mac_westlake(struct xlnid_hw *hw, u32 reg);
void xlnid_write_mac_westlake(struct xlnid_hw *hw, u32 reg, u32 data);

#endif /* _XLNID_API_H_ */


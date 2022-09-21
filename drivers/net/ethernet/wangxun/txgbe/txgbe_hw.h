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
 */

#ifndef _TXGBE_HW_H_
#define _TXGBE_HW_H_

#define TXGBE_EMC_INTERNAL_DATA         0x00
#define TXGBE_EMC_INTERNAL_THERM_LIMIT  0x20
#define TXGBE_EMC_DIODE1_DATA           0x01
#define TXGBE_EMC_DIODE1_THERM_LIMIT    0x19
#define TXGBE_EMC_DIODE2_DATA           0x23
#define TXGBE_EMC_DIODE2_THERM_LIMIT    0x1A
#define TXGBE_EMC_DIODE3_DATA           0x2A
#define TXGBE_EMC_DIODE3_THERM_LIMIT    0x30

#define SPI_CLK_DIV                        2

#define SPI_CMD_ERASE_CHIP                 4  // SPI erase chip command
#define SPI_CMD_ERASE_SECTOR               3  // SPI erase sector command
#define SPI_CMD_WRITE_DWORD                0  // SPI write a dword command
#define SPI_CMD_READ_DWORD                 1  // SPI read a dword command
#define SPI_CMD_USER_CMD                   5  // SPI user command
				
#define SPI_CLK_CMD_OFFSET                28  // SPI command field offset in Command register
#define SPI_CLK_DIV_OFFSET                25  // SPI clock divide field offset in Command register
				
#define SPI_TIME_OUT_VALUE             10000
#define SPI_SECTOR_SIZE          (4 * 1024)  // FLASH sector size is 64KB
#define SPI_H_CMD_REG_ADDR           0x10104  // SPI Command register address
#define SPI_H_DAT_REG_ADDR           0x10108  // SPI Data register address
#define SPI_H_STA_REG_ADDR           0x1010c  // SPI Status register address
#define SPI_H_USR_CMD_REG_ADDR       0x10110  // SPI User Command register address
#define SPI_CMD_CFG1_ADDR            0x10118  // Flash command configuration register 1
#define MISC_RST_REG_ADDR            0x1000c  // Misc reset register address
#define MGR_FLASH_RELOAD_REG_ADDR    0x101a0  // MGR reload flash read

#define MAC_ADDR0_WORD0_OFFSET_1G    0x006000c  // MAC Address for LAN0, stored in external FLASH
#define MAC_ADDR0_WORD1_OFFSET_1G    0x0060014
#define MAC_ADDR1_WORD0_OFFSET_1G    0x007000c  // MAC Address for LAN1, stored in external FLASH
#define MAC_ADDR1_WORD1_OFFSET_1G    0x0070014
#define PRODUCT_SERIAL_NUM_OFFSET_1G    0x00f0000  // Product Serial Number, stored in external FLASH last sector

struct txgbe_hic_read_cab {
	union txgbe_hic_hdr2 hdr;
	union {
		u8 d8[252];
		u16 d16[126];
		u32 d32[63];
	} dbuf;
};


/**
 * Packet Type decoding
 **/
/* txgbe_dec_ptype.mac: outer mac */
enum txgbe_dec_ptype_mac {
	TXGBE_DEC_PTYPE_MAC_IP = 0,
	TXGBE_DEC_PTYPE_MAC_L2 = 2,
	TXGBE_DEC_PTYPE_MAC_FCOE = 3,
};

/* txgbe_dec_ptype.[e]ip: outer&encaped ip */
#define TXGBE_DEC_PTYPE_IP_FRAG (0x4)
enum txgbe_dec_ptype_ip {
	TXGBE_DEC_PTYPE_IP_NONE = 0,
	TXGBE_DEC_PTYPE_IP_IPV4 = 1,
	TXGBE_DEC_PTYPE_IP_IPV6 = 2,
	TXGBE_DEC_PTYPE_IP_FGV4 =
		(TXGBE_DEC_PTYPE_IP_FRAG | TXGBE_DEC_PTYPE_IP_IPV4),
	TXGBE_DEC_PTYPE_IP_FGV6 =
		(TXGBE_DEC_PTYPE_IP_FRAG | TXGBE_DEC_PTYPE_IP_IPV6),
};

/* txgbe_dec_ptype.etype: encaped type */
enum txgbe_dec_ptype_etype {
	TXGBE_DEC_PTYPE_ETYPE_NONE = 0,
	TXGBE_DEC_PTYPE_ETYPE_IPIP = 1, /* IP+IP */
	TXGBE_DEC_PTYPE_ETYPE_IG = 2, /* IP+GRE */
	TXGBE_DEC_PTYPE_ETYPE_IGM = 3, /* IP+GRE+MAC */
	TXGBE_DEC_PTYPE_ETYPE_IGMV = 4, /* IP+GRE+MAC+VLAN */
};

/* txgbe_dec_ptype.proto: payload proto */
enum txgbe_dec_ptype_prot {
	TXGBE_DEC_PTYPE_PROT_NONE = 0,
	TXGBE_DEC_PTYPE_PROT_UDP = 1,
	TXGBE_DEC_PTYPE_PROT_TCP = 2,
	TXGBE_DEC_PTYPE_PROT_SCTP = 3,
	TXGBE_DEC_PTYPE_PROT_ICMP = 4,
	TXGBE_DEC_PTYPE_PROT_TS = 5, /* time sync */
};

/* txgbe_dec_ptype.layer: payload layer */
enum txgbe_dec_ptype_layer {
	TXGBE_DEC_PTYPE_LAYER_NONE = 0,
	TXGBE_DEC_PTYPE_LAYER_PAY2 = 1,
	TXGBE_DEC_PTYPE_LAYER_PAY3 = 2,
	TXGBE_DEC_PTYPE_LAYER_PAY4 = 3,
};

struct txgbe_dec_ptype {
	u32 ptype:8;
	u32 known:1;
	u32 mac:2; /* outer mac */
	u32 ip:3; /* outer ip*/
	u32 etype:3; /* encaped type */
	u32 eip:3; /* encaped ip */
	u32 prot:4; /* payload proto */
	u32 layer:3; /* payload layer */
};
typedef struct txgbe_dec_ptype txgbe_dptype;


void txgbe_dcb_get_rtrup2tc(struct txgbe_hw *hw, u8 *map);
u16 txgbe_get_pcie_msix_count(struct txgbe_hw *hw);
s32 txgbe_init_hw(struct txgbe_hw *hw);
s32 txgbe_start_hw(struct txgbe_hw *hw);
s32 txgbe_clear_hw_cntrs(struct txgbe_hw *hw);
s32 txgbe_read_pba_string(struct txgbe_hw *hw, u8 *pba_num,
				  u32 pba_num_size);
s32 txgbe_get_mac_addr(struct txgbe_hw *hw, u8 *mac_addr);
s32 txgbe_get_bus_info(struct txgbe_hw *hw);
void txgbe_set_pci_config_data(struct txgbe_hw *hw, u16 link_status);
void txgbe_set_lan_id_multi_port_pcie(struct txgbe_hw *hw);
s32 txgbe_stop_adapter(struct txgbe_hw *hw);

s32 txgbe_led_on(struct txgbe_hw *hw, u32 index);
s32 txgbe_led_off(struct txgbe_hw *hw, u32 index);

s32 txgbe_set_rar(struct txgbe_hw *hw, u32 index, u8 *addr, u64 pools,
			  u32 enable_addr);
s32 txgbe_clear_rar(struct txgbe_hw *hw, u32 index);
s32 txgbe_init_rx_addrs(struct txgbe_hw *hw);
s32 txgbe_update_mc_addr_list(struct txgbe_hw *hw, u8 *mc_addr_list,
				      u32 mc_addr_count,
				      txgbe_mc_addr_itr func, bool clear);
s32 txgbe_update_uc_addr_list(struct txgbe_hw *hw, u8 *addr_list,
				      u32 addr_count, txgbe_mc_addr_itr func);
s32 txgbe_enable_mc(struct txgbe_hw *hw);
s32 txgbe_disable_mc(struct txgbe_hw *hw);
s32 txgbe_disable_sec_rx_path(struct txgbe_hw *hw);
s32 txgbe_enable_sec_rx_path(struct txgbe_hw *hw);

s32 txgbe_fc_enable(struct txgbe_hw *hw);
bool txgbe_device_supports_autoneg_fc(struct txgbe_hw *hw);
void txgbe_fc_autoneg(struct txgbe_hw *hw);
s32 txgbe_setup_fc(struct txgbe_hw *hw);

s32 txgbe_validate_mac_addr(u8 *mac_addr);
s32 txgbe_acquire_swfw_sync(struct txgbe_hw *hw, u32 mask);
void txgbe_release_swfw_sync(struct txgbe_hw *hw, u32 mask);
s32 txgbe_disable_pcie_master(struct txgbe_hw *hw);


s32 txgbe_get_san_mac_addr(struct txgbe_hw *hw, u8 *san_mac_addr);
s32 txgbe_set_san_mac_addr(struct txgbe_hw *hw, u8 *san_mac_addr);

s32 txgbe_set_vmdq(struct txgbe_hw *hw, u32 rar, u32 vmdq);
s32 txgbe_set_vmdq_san_mac(struct txgbe_hw *hw, u32 vmdq);
s32 txgbe_clear_vmdq(struct txgbe_hw *hw, u32 rar, u32 vmdq);
s32 txgbe_insert_mac_addr(struct txgbe_hw *hw, u8 *addr, u32 vmdq);
s32 txgbe_init_uta_tables(struct txgbe_hw *hw);
s32 txgbe_set_vfta(struct txgbe_hw *hw, u32 vlan,
			 u32 vind, bool vlan_on);
s32 txgbe_set_vlvf(struct txgbe_hw *hw, u32 vlan, u32 vind,
			   bool vlan_on, bool *vfta_changed);
s32 txgbe_clear_vfta(struct txgbe_hw *hw);
s32 txgbe_find_vlvf_slot(struct txgbe_hw *hw, u32 vlan);

s32 txgbe_get_wwn_prefix(struct txgbe_hw *hw, u16 *wwnn_prefix,
				 u16 *wwpn_prefix);

void txgbe_set_mac_anti_spoofing(struct txgbe_hw *hw, bool enable, int pf);
void txgbe_set_vlan_anti_spoofing(struct txgbe_hw *hw, bool enable, int vf);
void txgbe_set_ethertype_anti_spoofing(struct txgbe_hw *hw,
					bool enable, int vf);
s32 txgbe_get_device_caps(struct txgbe_hw *hw, u16 *device_caps);
void txgbe_set_rxpba(struct txgbe_hw *hw, int num_pb, u32 headroom,
			     int strategy);
s32 txgbe_set_fw_drv_ver(struct txgbe_hw *hw, u8 maj, u8 min,
				 u8 build, u8 ver);
s32 txgbe_reset_hostif(struct txgbe_hw *hw);
u8 txgbe_calculate_checksum(u8 *buffer, u32 length);
s32 txgbe_host_interface_command(struct txgbe_hw *hw, u32 *buffer,
				 u32 length, u32 timeout, bool return_data);

void txgbe_clear_tx_pending(struct txgbe_hw *hw);
void txgbe_stop_mac_link_on_d3(struct txgbe_hw *hw);
bool txgbe_mng_present(struct txgbe_hw *hw);
bool txgbe_check_mng_access(struct txgbe_hw *hw);

s32 txgbe_get_thermal_sensor_data(struct txgbe_hw *hw);
s32 txgbe_init_thermal_sensor_thresh(struct txgbe_hw *hw);
void txgbe_enable_rx(struct txgbe_hw *hw);
void txgbe_disable_rx(struct txgbe_hw *hw);
s32 txgbe_setup_mac_link_multispeed_fiber(struct txgbe_hw *hw,
					  u32 speed,
					  bool autoneg_wait_to_complete);
int txgbe_check_flash_load(struct txgbe_hw *hw, u32 check_bit);

#if defined(TXGBE_SUPPORT_KYLIN_FT)
void txgbe_disable_fdir(struct txgbe_hw *hw);
#endif

/* @txgbe_api.h */
s32 txgbe_reinit_fdir_tables(struct txgbe_hw *hw);
s32 txgbe_init_fdir_signature(struct txgbe_hw *hw, u32 fdirctrl);
s32 txgbe_init_fdir_perfect(struct txgbe_hw *hw, u32 fdirctrl,
					bool cloud_mode);
s32 txgbe_fdir_add_signature_filter(struct txgbe_hw *hw,
					  union txgbe_atr_hash_dword input,
					  union txgbe_atr_hash_dword common,
					  u8 queue);
s32 txgbe_fdir_set_input_mask(struct txgbe_hw *hw,
			union txgbe_atr_input *input_mask, bool cloud_mode);
s32 txgbe_fdir_write_perfect_filter(struct txgbe_hw *hw,
				  union txgbe_atr_input *input,
				  u16 soft_id, u8 queue, bool cloud_mode);
s32 txgbe_fdir_erase_perfect_filter(struct txgbe_hw *hw,
					  union txgbe_atr_input *input,
					  u16 soft_id);
s32 txgbe_fdir_add_perfect_filter(struct txgbe_hw *hw,
					union txgbe_atr_input *input,
					union txgbe_atr_input *mask,
					u16 soft_id,
					u8 queue,
					bool cloud_mode);
void txgbe_atr_compute_perfect_hash(union txgbe_atr_input *input,
					  union txgbe_atr_input *mask);
u32 txgbe_atr_compute_sig_hash(union txgbe_atr_hash_dword input,
				     union txgbe_atr_hash_dword common);

s32 txgbe_get_link_capabilities(struct txgbe_hw *hw,
				      u32 *speed, bool *autoneg);
enum txgbe_media_type txgbe_get_media_type(struct txgbe_hw *hw);
void txgbe_disable_tx_laser_multispeed_fiber(struct txgbe_hw *hw);
void txgbe_enable_tx_laser_multispeed_fiber(struct txgbe_hw *hw);
void txgbe_flap_tx_laser_multispeed_fiber(struct txgbe_hw *hw);
void txgbe_set_hard_rate_select_speed(struct txgbe_hw *hw,
					u32 speed);
s32 txgbe_setup_mac_link(struct txgbe_hw *hw, u32 speed,
			       bool autoneg_wait_to_complete);
void txgbe_init_mac_link_ops(struct txgbe_hw *hw);
s32 txgbe_reset_hw(struct txgbe_hw *hw);
s32 txgbe_identify_phy(struct txgbe_hw *hw);
s32 txgbe_init_phy_ops(struct txgbe_hw *hw);
s32 txgbe_enable_rx_dma(struct txgbe_hw *hw, u32 regval);
s32 txgbe_init_ops(struct txgbe_hw *hw);
s32 txgbe_setup_eee(struct txgbe_hw *hw, bool enable_eee);

s32 txgbe_init_flash_params(struct txgbe_hw *hw);
s32 txgbe_read_flash_buffer(struct txgbe_hw *hw, u32 offset,
					  u32 dwords, u32 *data);
s32 txgbe_write_flash_buffer(struct txgbe_hw *hw, u32 offset,
					  u32 dwords, u32 *data);

s32 txgbe_read_eeprom(struct txgbe_hw *hw,
				   u16 offset, u16 *data);
s32 txgbe_read_eeprom_buffer(struct txgbe_hw *hw, u16 offset,
					  u16 words, u16 *data);
s32 txgbe_init_eeprom_params(struct txgbe_hw *hw);
s32 txgbe_update_eeprom_checksum(struct txgbe_hw *hw);
s32 txgbe_calc_eeprom_checksum(struct txgbe_hw *hw);
s32 txgbe_validate_eeprom_checksum(struct txgbe_hw *hw,
					    u16 *checksum_val);
s32 txgbe_update_flash(struct txgbe_hw *hw);
int txgbe_upgrade_flash(struct txgbe_hw *hw, u32 region,
								const u8 *data, u32 size);
s32 txgbe_write_ee_hostif_buffer(struct txgbe_hw *hw,
				u16 offset, u16 words, u16 *data);
s32 txgbe_write_ee_hostif(struct txgbe_hw *hw, u16 offset,
				u16 data);
s32 txgbe_read_ee_hostif_buffer(struct txgbe_hw *hw,
				u16 offset, u16 words, u16 *data);
s32 txgbe_read_ee_hostif(struct txgbe_hw *hw, u16 offset, u16 *data);
u32 txgbe_rd32_epcs(struct txgbe_hw *hw, u32 addr);
void txgbe_wr32_epcs(struct txgbe_hw *hw, u32 addr, u32 data);
void txgbe_wr32_ephy(struct txgbe_hw *hw, u32 addr, u32 data);
u32 rd32_ephy(struct txgbe_hw *hw, u32 addr);

s32 txgbe_upgrade_flash_hostif(struct txgbe_hw *hw,  u32 region,
				const u8 *data, u32 size);

s32 txgbe_close_notify(struct txgbe_hw *hw);
s32 txgbe_open_notify(struct txgbe_hw *hw);

s32 txgbe_set_link_to_kr(struct txgbe_hw *hw, bool autoneg);
s32 txgbe_set_link_to_kx4(struct txgbe_hw *hw, bool autoneg);

s32 txgbe_set_link_to_kx(struct txgbe_hw *hw,
			       u32 speed,
			       bool autoneg);


#endif /* _TXGBE_HW_H_ */

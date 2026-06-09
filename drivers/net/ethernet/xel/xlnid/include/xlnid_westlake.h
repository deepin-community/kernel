/***************************************************************************************************
	Copyright (C), 2008-2023, XEL Tech. CO., Ltd.
	Filename: xlnid_westlake.h
	Description:
	Westlake MAC and GEPHY funcs
	History:
	<author>    <time>    <version>    <desc>
	jacobshi    2022/12/29    1.0    create file
***************************************************************************************************/

#ifndef __XLNID_WESTLAKE_H__
#define __XLNID_WESTLAKE_H__

extern enum xlnid_media_type xlnid_get_media_type_westlake(struct xlnid_hw *hw);

extern void xlnid_disable_tx_laser_multispeed_fiber(struct xlnid_hw *hw);

extern void xlnid_enable_tx_laser_multispeed_fiber(struct xlnid_hw *hw);

extern void xlnid_flap_tx_laser_multispeed_fiber(struct xlnid_hw *hw);

extern void xlnid_set_hard_rate_select_speed(struct xlnid_hw *hw,
		xlnid_link_speed speed);

extern s32 xlnid_setup_mac_link_smartspeed(struct xlnid_hw *hw,
		xlnid_link_speed speed,
		bool autoneg_wait_to_complete);

extern s32 xlnid_start_mac_link_westlake(struct xlnid_hw *hw,
		bool autoneg_wait_to_complete);

extern s32 xlnid_setup_mac_link_westlake(struct xlnid_hw *hw,
		xlnid_link_speed speed,
		bool autoneg_wait_to_complete);

extern s32 xlnid_setup_sfp_modules_westlake(struct xlnid_hw *hw);

extern void xlnid_init_mac_link_ops_westlake(struct xlnid_hw *hw);

extern s32 xlnid_reset_hw_westlake(struct xlnid_hw *hw);

extern s32 xlnid_read_analog_reg8_westlake(struct xlnid_hw *hw, u32 reg,
		u8 *val);

extern s32 xlnid_write_analog_reg8_westlake(struct xlnid_hw *hw, u32 reg,
		u8 val);

extern s32 xlnid_start_hw_westlake(struct xlnid_hw *hw);

extern s32 xlnid_identify_phy_westlake(struct xlnid_hw *hw);

extern s32 xlnid_init_phy_ops_westlake(struct xlnid_hw *hw);

extern s32 xlnid_set_phy_power_westlake(struct xlnid_hw *hw, bool on);

extern u64 xlnid_get_supported_physical_layer_westlake(struct xlnid_hw *hw);

extern s32 xlnid_enable_rx_dma_westlake(struct xlnid_hw *hw, u32 regval);

extern s32 prot_autoc_read_westlake(struct xlnid_hw *hw, bool *locked,
									u32 *reg_val);

extern s32 prot_autoc_write_westlake(struct xlnid_hw *hw, u32 reg_val,
										bool locked);

extern s32 xlnid_get_link_capabilities_westlake(struct xlnid_hw *hw,
		xlnid_link_speed *speed,
		bool *autoneg);

extern s32 xlnid_check_mac_link_westlake(struct xlnid_hw *hw,
		xlnid_link_speed *speed,
		bool *link_up, bool link_up_wait_to_complete);

extern s32 xlnid_setup_mac_link_westlake(struct xlnid_hw *hw,
		xlnid_link_speed speed,
		bool autoneg_wait_to_complete);

extern s32 xlnid_reinit_fdir_tables_westlake(struct xlnid_hw *hw);
extern s32 xlnid_init_fdir_signature_westlake(struct xlnid_hw *hw,
		u32 fdirctrl);
extern s32 xlnid_init_fdir_perfect_westlake(struct xlnid_hw *hw, u32 fdirctrl,
		bool cloud_mode);
extern u32 xlnid_atr_compute_sig_hash_westlake(union xlnid_atr_hash_dword input,
		union xlnid_atr_hash_dword common);
extern void xlnid_fdir_add_signature_filter_westlake(struct xlnid_hw *hw,
		union xlnid_atr_hash_dword input,
		union xlnid_atr_hash_dword common,
		u8 queue);
extern void xlnid_atr_compute_perfect_hash_westlake(union xlnid_atr_input
		*input,
		union xlnid_atr_input *input_mask);
extern s32 xlnid_fdir_set_input_mask_westlake(struct xlnid_hw *hw,
		union xlnid_atr_input *input_mask, bool cloud_mode);
extern s32 xlnid_fdir_write_perfect_filter_westlake(struct xlnid_hw *hw,
		union xlnid_atr_input *input,
		u16 soft_id, u8 queue, bool cloud_mode);
extern s32 xlnid_fdir_remove_perfect_filter_westlake(struct xlnid_hw *hw,
		union xlnid_atr_input *input,
		u16 soft_id, u8 queue, bool cloud_mode);
extern s32 xlnid_fdir_erase_perfect_filter_westlake(struct xlnid_hw *hw,
		union xlnid_atr_input *input,
		u16 soft_id);
extern s32 xlnid_fdir_add_perfect_filter_westlake(struct xlnid_hw *hw,
		union xlnid_atr_input *input,
		union xlnid_atr_input *input_mask,
		u16 soft_id, u8 queue, bool cloud_mode);
extern void xlnid_fc_autoneg_westlake(struct xlnid_hw *hw);

#endif /* __XLNID_WESTLAKE_H__ */

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2015 - 2022 Beijing WangXun Technology Co., Ltd. */

#ifndef _TXGBE_E56_BP_H_
#define _TXGBE_E56_BP_H_

#define TXGBE_E56_AN_TXDIS	BIT(3)
#define TXGBE_E56_AN_PG_RCV	BIT(2)
#define TXGBE_E56_AN_INC_LINK	BIT(1)
#define TXGBE_E56_AN_INT_CMPLT	BIT(0)

#define TXGBE_10G_FEC_REQ	BIT(15)
#define TXGBE_10G_FEC_ABL	BIT(14)
#define TXGBE_25G_BASE_FEC_REQ	BIT(13)
#define TXGBE_25G_RS_FEC_REQ	BIT(12)

union txgbe_e56_pmd_tx_ffe_cfg1 {
	struct {
		u32 tx0_cursor_factor : 7;
		u32 rsvd0             : 1;
		u32 tx1_cursor_factor : 7;
		u32 rsvd1             : 1;
		u32 tx2_cursor_factor : 7;
		u32 rsvd2             : 1;
		u32 tx3_cursor_factor : 7;
		u32 rsvd3             : 1;
	};
	u32 reg;
};

#define E56G__PMD_TX_FFE_CFG_1_NUM                                          1
#define E56G__PMD_TX_FFE_CFG_1_ADDR                   (E56G__BASEADDR + 0x141c)
#define E56G__PMD_TX_FFE_CFG_1_PTR		((union txgbe_e56_pmd_tx_ffe_cfg1 *)\
												(E56G__PMD_TX_FFE_CFG_1_ADDR))
#define E56G__PMD_TX_FFE_CFG_1_STRIDE                                       4
#define E56G__PMD_TX_FFE_CFG_1_SIZE                                        32
#define E56G__PMD_TX_FFE_CFG_1_ACC_SIZE                                    32
#define E56G__PMD_TX_FFE_CFG_1_READ_MSB                                    30
#define E56G__PMD_TX_FFE_CFG_1_READ_LSB                                     0
#define E56G__PMD_TX_FFE_CFG_1_WRITE_MSB                                   30
#define E56G__PMD_TX_FFE_CFG_1_WRITE_LSB                                    0
#define E56G__PMD_TX_FFE_CFG_1_RESET_VALUE                         0x3f3f3f3f

union txgbe_e56_pmd_tx_ffe_cfg2 {
	struct {
		u32 tx0_precursor1_factor : 6;
		u32 rsvd0                 : 2;
		u32 tx1_precursor1_factor : 6;
		u32 rsvd1                 : 2;
		u32 tx2_precursor1_factor : 6;
		u32 rsvd2                 : 2;
		u32 tx3_precursor1_factor : 6;
		u32 rsvd3                 : 2;
	};
	u32 reg;
};

#define E56G__PMD_TX_FFE_CFG_2_NUM                                          1
#define E56G__PMD_TX_FFE_CFG_2_ADDR                   (E56G__BASEADDR + 0x1420)
#define E56G__PMD_TX_FFE_CFG_2_PTR			((union txgbe_e56_pmd_tx_ffe_cfg2 *)\
												(E56G__PMD_TX_FFE_CFG_2_ADDR))
#define E56G__PMD_TX_FFE_CFG_2_STRIDE                                       4
#define E56G__PMD_TX_FFE_CFG_2_SIZE                                        32
#define E56G__PMD_TX_FFE_CFG_2_ACC_SIZE                                    32
#define E56G__PMD_TX_FFE_CFG_2_READ_MSB                                    29
#define E56G__PMD_TX_FFE_CFG_2_READ_LSB                                     0
#define E56G__PMD_TX_FFE_CFG_2_WRITE_MSB                                   29
#define E56G__PMD_TX_FFE_CFG_2_WRITE_LSB                                    0
#define E56G__PMD_TX_FFE_CFG_2_RESET_VALUE                                0x0

union txgbe_e56_pmd_tx_ffe_cfg3 {
	struct {
		u32 tx0_precursor2_factor : 6;
		u32 rsvd0                 : 2;
		u32 tx1_precursor2_factor : 6;
		u32 rsvd1                 : 2;
		u32 tx2_precursor2_factor : 6;
		u32 rsvd2                 : 2;
		u32 tx3_precursor2_factor : 6;
		u32 rsvd3                 : 2;
	};
	u32 reg;
};

#define E56G__PMD_TX_FFE_CFG_3_NUM                                          1
#define E56G__PMD_TX_FFE_CFG_3_ADDR                   (E56G__BASEADDR + 0x1424)
#define E56G__PMD_TX_FFE_CFG_3_PTR			((union txgbe_e56_pmd_tx_ffe_cfg3 *)\
												(E56G__PMD_TX_FFE_CFG_3_ADDR))
#define E56G__PMD_TX_FFE_CFG_3_STRIDE                                       4
#define E56G__PMD_TX_FFE_CFG_3_SIZE                                        32
#define E56G__PMD_TX_FFE_CFG_3_ACC_SIZE                                    32
#define E56G__PMD_TX_FFE_CFG_3_READ_MSB                                    29
#define E56G__PMD_TX_FFE_CFG_3_READ_LSB                                     0
#define E56G__PMD_TX_FFE_CFG_3_WRITE_MSB                                   29
#define E56G__PMD_TX_FFE_CFG_3_WRITE_LSB                                    0
#define E56G__PMD_TX_FFE_CFG_3_RESET_VALUE                                0x0

union txgbe_e56_pmd_tx_ffe_cfg4 {
	struct {
		u32 tx0_postcursor_factor : 6;
		u32 rsvd0                 : 2;
		u32 tx1_postcursor_factor : 6;
		u32 rsvd1                 : 2;
		u32 tx2_postcursor_factor : 6;
		u32 rsvd2                 : 2;
		u32 tx3_postcursor_factor : 6;
		u32 rsvd3                 : 2;
	};
	u32 reg;
};

#define E56G__PMD_TX_FFE_CFG_4_NUM                                          1
#define E56G__PMD_TX_FFE_CFG_4_ADDR                   (E56G__BASEADDR + 0x1428)
#define E56G__PMD_TX_FFE_CFG_4_PTR			((union txgbe_e56_pmd_tx_ffe_cfg4 *)\
												(E56G__PMD_TX_FFE_CFG_4_ADDR))
#define E56G__PMD_TX_FFE_CFG_4_STRIDE                                       4
#define E56G__PMD_TX_FFE_CFG_4_SIZE                                        32
#define E56G__PMD_TX_FFE_CFG_4_ACC_SIZE                                    32
#define E56G__PMD_TX_FFE_CFG_4_READ_MSB                                    29
#define E56G__PMD_TX_FFE_CFG_4_READ_LSB                                     0
#define E56G__PMD_TX_FFE_CFG_4_WRITE_MSB                                   29
#define E56G__PMD_TX_FFE_CFG_4_WRITE_LSB                                    0
#define E56G__PMD_TX_FFE_CFG_4_RESET_VALUE                                0x0

union txgbe_e56g_cms_ana_ovrdval7 {
	struct {
		u32 ana_lcpll_lf_vco_swing_ctrl_i    : 4;
		u32 ana_lcpll_lf_lpf_setcode_calib_i : 5;
		u32 rsvd0                            : 3;
		u32 ana_lcpll_lf_vco_coarse_bin_i    : 5;
		u32 rsvd1                            : 3;
		u32 ana_lcpll_lf_vco_fine_therm_i    : 8;
		u32 ana_lcpll_lf_clkout_fb_ctrl_i    : 2;
		u32 rsvd2                            : 2;
	};
	u32 reg;
};

#define E56G__CMS_ANA_OVRDVAL_7_NUM                                         1
#define E56G__CMS_ANA_OVRDVAL_7_ADDR                   (E56G__BASEADDR + 0xccc)
#define E56G__CMS_ANA_OVRDVAL_7_PTR ((union txgbe_e56g_cms_ana_ovrdval7 *)\
										(E56G__CMS_ANA_OVRDVAL_7_ADDR))
#define E56G__CMS_ANA_OVRDVAL_7_STRIDE                                      4
#define E56G__CMS_ANA_OVRDVAL_7_SIZE                                       32
#define E56G__CMS_ANA_OVRDVAL_7_ACC_SIZE                                   32
#define E56G__CMS_ANA_OVRDVAL_7_READ_MSB                                   29
#define E56G__CMS_ANA_OVRDVAL_7_READ_LSB                                    0
#define E56G__CMS_ANA_OVRDVAL_7_WRITE_MSB                                  29
#define E56G__CMS_ANA_OVRDVAL_7_WRITE_LSB                                   0
#define E56G__CMS_ANA_OVRDVAL_7_RESET_VALUE                               0x0

union txgbe_e56g_cms_ana_ovrden1 {
	struct {
		u32 ovrd_en_ana_lcpll_hf_vco_amp_status_o    : 1;
		u32 ovrd_en_ana_lcpll_hf_clkout_fb_ctrl_i    : 1;
		u32 ovrd_en_ana_lcpll_hf_clkdiv_ctrl_i       : 1;
		u32 ovrd_en_ana_lcpll_hf_en_odiv_i           : 1;
		u32 ovrd_en_ana_lcpll_hf_test_in_i           : 1;
		u32 ovrd_en_ana_lcpll_hf_test_out_o          : 1;
		u32 ovrd_en_ana_lcpll_lf_en_bias_i           : 1;
		u32 ovrd_en_ana_lcpll_lf_en_loop_i           : 1;
		u32 ovrd_en_ana_lcpll_lf_en_cp_i             : 1;
		u32 ovrd_en_ana_lcpll_lf_icp_base_i          : 1;
		u32 ovrd_en_ana_lcpll_lf_icp_fine_i          : 1;
		u32 ovrd_en_ana_lcpll_lf_lpf_ctrl_i          : 1;
		u32 ovrd_en_ana_lcpll_lf_lpf_setcode_calib_i : 1;
		u32 ovrd_en_ana_lcpll_lf_set_lpf_i           : 1;
		u32 ovrd_en_ana_lcpll_lf_en_vco_i            : 1;
		u32 ovrd_en_ana_lcpll_lf_vco_sel_i           : 1;
		u32 ovrd_en_ana_lcpll_lf_vco_swing_ctrl_i    : 1;
		u32 ovrd_en_ana_lcpll_lf_vco_coarse_bin_i    : 1;
		u32 ovrd_en_ana_lcpll_lf_vco_fine_therm_i    : 1;
		u32 ovrd_en_ana_lcpll_lf_vco_amp_status_o    : 1;
		u32 ovrd_en_ana_lcpll_lf_clkout_fb_ctrl_i    : 1;
		u32 ovrd_en_ana_lcpll_lf_clkdiv_ctrl_i       : 1;
		u32 ovrd_en_ana_lcpll_lf_en_odiv_i           : 1;
		u32 ovrd_en_ana_lcpll_lf_test_in_i           : 1;
		u32 ovrd_en_ana_lcpll_lf_test_out_o          : 1;
		u32 ovrd_en_ana_lcpll_hf_refclk_select_i     : 1;
		u32 ovrd_en_ana_lcpll_lf_refclk_select_i     : 1;
		u32 ovrd_en_ana_lcpll_hf_clk_ref_sel_i       : 1;
		u32 ovrd_en_ana_lcpll_lf_clk_ref_sel_i       : 1;
		u32 ovrd_en_ana_test_bias_i                  : 1;
		u32 ovrd_en_ana_test_slicer_i                : 1;
		u32 ovrd_en_ana_test_sampler_i               : 1;
	};
	u32 reg;
};

#define E56G__CMS_ANA_OVRDEN_1_NUM                                          1
#define E56G__CMS_ANA_OVRDEN_1_ADDR                    (E56G__BASEADDR + 0xca8)
#define E56G__CMS_ANA_OVRDEN_1_PTR		((union txgbe_e56g_cms_ana_ovrden1 *)\
										(E56G__CMS_ANA_OVRDEN_1_ADDR))
#define E56G__CMS_ANA_OVRDEN_1_STRIDE                                       4
#define E56G__CMS_ANA_OVRDEN_1_SIZE                                        32
#define E56G__CMS_ANA_OVRDEN_1_ACC_SIZE                                    32
#define E56G__CMS_ANA_OVRDEN_1_READ_MSB                                    31
#define E56G__CMS_ANA_OVRDEN_1_READ_LSB                                     0
#define E56G__CMS_ANA_OVRDEN_1_WRITE_MSB                                   31
#define E56G__CMS_ANA_OVRDEN_1_WRITE_LSB                                    0
#define E56G__CMS_ANA_OVRDEN_1_RESET_VALUE                                0x0

union txgbe_e56g_cms_ana_ovrdval9 {
	struct {
		u32 ana_lcpll_lf_test_in_i : 32;
	};
	u32 reg;
};

#define E56G__CMS_ANA_OVRDVAL_9_NUM                                         1
#define E56G__CMS_ANA_OVRDVAL_9_ADDR                   (E56G__BASEADDR + 0xcd4)
#define E56G__CMS_ANA_OVRDVAL_9_PTR ((union txgbe_e56g_cms_ana_ovrdval9 *)\
											(E56G__CMS_ANA_OVRDVAL_9_ADDR))
#define E56G__CMS_ANA_OVRDVAL_9_STRIDE                                      4
#define E56G__CMS_ANA_OVRDVAL_9_SIZE                                       32
#define E56G__CMS_ANA_OVRDVAL_9_ACC_SIZE                                   32
#define E56G__CMS_ANA_OVRDVAL_9_READ_MSB                                   31
#define E56G__CMS_ANA_OVRDVAL_9_READ_LSB                                    0
#define E56G__CMS_ANA_OVRDVAL_9_WRITE_MSB                                  31
#define E56G__CMS_ANA_OVRDVAL_9_WRITE_LSB                                   0
#define E56G__CMS_ANA_OVRDVAL_9_RESET_VALUE                               0x0

#define SFP2_RS0  5
#define SFP2_RS1  4
#define SFP2_TX_DISABLE  1
#define SFP2_TX_FAULT  0
#define SFP2_RX_LOS_BIT  3
#ifdef PHYINIT_TIMEOUT
#undef PHYINIT_TIMEOUT
#define PHYINIT_TIMEOUT   2000
#endif

//#define E56PHY_CMS_ANA_OVRDEN_0_ADDR   (E56PHY_CMS_BASE_ADDR+0xA4)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_REFCLK_BUF_DAISY_EN_I FORMAT_NOPARENTHERSES(0, 0)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_REFCLK_BUF_PAD_EN_I FORMAT_NOPARENTHERSES(1, 1)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_REFCLK_BUF_PAD_EN_I_LSB 1
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_VDDINOFF_DCORE_DIG_O FORMAT_NOPARENTHERSES(2, 2)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_BG_EN_I FORMAT_NOPARENTHERSES(11, 11)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_BG_EN_I_LSB 11
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_BG_TESTIN_I FORMAT_NOPARENTHERSES(12, 12)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_BG_TESTIN_I_LSB 12
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_EN_RESCAL_I FORMAT_NOPARENTHERSES(13, 13)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_EN_RESCAL_I_LSB 13
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_RESCAL_COMP_O FORMAT_NOPARENTHERSES(14, 14)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_RESCAL_COMP_O_LSB 14
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_RESCAL_CODE_I FORMAT_NOPARENTHERSES(15, 15)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_RESCAL_CODE_I_LSB 15
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_EN_LDO_CORE_I FORMAT_NOPARENTHERSES(16, 16)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_EN_LDO_CORE_I_LSB 16
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_TEST_LDO_I FORMAT_NOPARENTHERSES(17, 17)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_TEST_LDO_I_LSB 17
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_ANA_DEBUG_SEL_I FORMAT_NOPARENTHERSES(18, 18)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_ANA_DEBUG_SEL_I_LSB 18
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_EN_BIAS_I FORMAT_NOPARENTHERSES(19, 19)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_EN_BIAS_I_LSB 19
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_EN_LOOP_I FORMAT_NOPARENTHERSES(20, 20)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_EN_LOOP_I_LSB 20
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_EN_CP_I FORMAT_NOPARENTHERSES(21, 21)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_EN_CP_I_LSB 21
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_ICP_BASE_I FORMAT_NOPARENTHERSES(22, 22)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_ICP_BASE_I_LSB 22
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_ICP_FINE_I FORMAT_NOPARENTHERSES(23, 23)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_ICP_FINE_I_LSB 23
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_LPF_CTRL_I FORMAT_NOPARENTHERSES(24, 24)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_LPF_CTRL_I_LSB 24
#define E56PHY_CMS_ANA_OVRD_0_EN_ANA_LCPLL_HF_LPF_SETCODE_CALIB_I FORMAT_NOPARENTHERSES(25, 25)
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_LPF_SETCODE_CALIB_I_LSB 25
#define E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_SET_LPF_I FORMAT_NOPARENTHERSES(26, 26)

#define E56PHY_CMS_ANA_OVRDVAL_2_ANA_LCPLL_HF_LPF_SETCODE_CALIB_I FORMAT_NOPARENTHERSES(20, 16)
#define E56PHY_CMS_ANA_OVRDEN_1_EN_ANA_LCPLL_LF_LPF_SETCODE_CALIB_I FORMAT_NOPARENTHERSES(12, 12)
#define E56PHY_CMS_ANA_OVRDVAL_7_ADDR   (E56PHY_CMS_BASE_ADDR + 0xCC)
#define E56PHY_CMS_ANA_OVRDVAL_5_ADDR   (E56PHY_CMS_BASE_ADDR + 0xC4)
#define E56PHY_CMS_ANA_OVRDEN_1_OVRD_EN_ANA_LCPLL_LF_TEST_IN_I FORMAT_NOPARENTHERSES(23, 23)
#define E56PHY_CMS_ANA_OVRDVAL_9_ADDR   (E56PHY_CMS_BASE_ADDR + 0xD4)
#define E56PHY_CMS_ANA_OVRDVAL_10_ADDR   (E56PHY_CMS_BASE_ADDR + 0xD8)
#define E56PHY_CMS_ANA_OVRDVAL_7_ANA_LCPLL_LF_LPF_SETCODE_CALIB_I FORMAT_NOPARENTHERSES(8, 4)

int txgbe_e56_set_phylinkmode(struct txgbe_adapter *adapter,
			      unsigned char link_mode, unsigned int bypass_ctle);
void txgbe_e56_bp_watchdog_event(struct txgbe_adapter *adapter);

#endif

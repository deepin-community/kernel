// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019 - 2022 Beijing WangXun Technology Co., Ltd. */

#include "txgbe_e56.h"
#include "linux/delay.h"
#include "txgbe.h"
#include "txgbe_hw.h"

#include <linux/sort.h>

#define FIELD_PREP_M(_mask, _val) \
	({ ((typeof(_mask))(_val) << __bf_shf_m(_mask)) & (_mask); })

void txgbe_field_set(u32 *psrcdata, u32 bithigh, u32 bitlow, u32 setvalue)
{
	*psrcdata &= ~GENMASK(bithigh, bitlow);
	*psrcdata |= FIELD_PREP_M(GENMASK(bithigh, bitlow), setvalue);
}

s32 txgbe_e56_check_phy_link(struct txgbe_hw *hw, u32 *speed, bool *link_up)
{
	struct txgbe_adapter *adapter = hw->back;
	u32 rdata = 0;
	u32 links_reg = 0;

	/* must read it twice because the state may
	 * not be correct the first time you read it
	 */
	rdata = txgbe_rd32_epcs(hw, 0x30001);
	rdata = txgbe_rd32_epcs(hw, 0x30001);

	if (rdata & TXGBE_E56_PHY_LINK_UP)
		*link_up = true;
	else
		*link_up = false;

	if (!adapter->link_valid)
		*link_up = false;

	links_reg = rd32(hw, TXGBE_CFG_PORT_ST);
	if (*link_up) {
		if ((links_reg & TXGBE_CFG_PORT_ST_AML_LINK_40G) ==
		    TXGBE_CFG_PORT_ST_AML_LINK_40G)
			*speed = TXGBE_LINK_SPEED_40GB_FULL;
		else if ((links_reg & TXGBE_CFG_PORT_ST_AML_LINK_25G) ==
			 TXGBE_CFG_PORT_ST_AML_LINK_25G)
			*speed = TXGBE_LINK_SPEED_25GB_FULL;
		else if ((links_reg & TXGBE_CFG_PORT_ST_AML_LINK_10G) ==
			 TXGBE_CFG_PORT_ST_AML_LINK_10G)
			*speed = TXGBE_LINK_SPEED_10GB_FULL;
	} else {
		*speed = TXGBE_LINK_SPEED_UNKNOWN;
	}

	return 0;
}

static u32 txgbe_e56_phy_tx_ffe_cfg(struct txgbe_hw *hw, u32 speed)
{
	struct txgbe_adapter *adapter = hw->back;
	u32 addr;

	if (speed == TXGBE_LINK_SPEED_10GB_FULL) {
		adapter->aml_txeq.main = S10G_TX_FFE_CFG_MAIN;
		adapter->aml_txeq.pre1 = S10G_TX_FFE_CFG_PRE1;
		adapter->aml_txeq.pre2 = S10G_TX_FFE_CFG_PRE2;
		adapter->aml_txeq.post = S10G_TX_FFE_CFG_POST;
	} else if (speed == TXGBE_LINK_SPEED_25GB_FULL) {
		adapter->aml_txeq.main = S25G_TX_FFE_CFG_MAIN;
		adapter->aml_txeq.pre1 = S25G_TX_FFE_CFG_PRE1;
		adapter->aml_txeq.pre2 = S25G_TX_FFE_CFG_PRE2;
		adapter->aml_txeq.post = S25G_TX_FFE_CFG_POST;

		if (hw->phy.sfp_type == txgbe_sfp_type_da_cu_core0 ||
		    hw->phy.sfp_type == txgbe_sfp_type_da_cu_core1 ||
		    txgbe_is_backplane(hw)) {
			adapter->aml_txeq.main = S25G_TX_FFE_CFG_DAC_MAIN;
			adapter->aml_txeq.pre1 = S25G_TX_FFE_CFG_DAC_PRE1;
			adapter->aml_txeq.pre2 = S25G_TX_FFE_CFG_DAC_PRE2;
			adapter->aml_txeq.post = S25G_TX_FFE_CFG_DAC_POST;
		}
	} else if (speed == TXGBE_LINK_SPEED_40GB_FULL) {
		adapter->aml_txeq.main = S10G_TX_FFE_CFG_MAIN;
		adapter->aml_txeq.pre1 = S10G_TX_FFE_CFG_PRE1;
		adapter->aml_txeq.pre2 = S10G_TX_FFE_CFG_PRE2;
		adapter->aml_txeq.post = S10G_TX_FFE_CFG_POST;

		if (hw->phy.sfp_type == txgbe_qsfp_type_40g_cu_core0 ||
		    hw->phy.sfp_type == txgbe_qsfp_type_40g_cu_core1 ||
		    txgbe_is_backplane(hw)) {
			adapter->aml_txeq.main = 0x2b2b2b2b;
			adapter->aml_txeq.pre1 = 0x03030303;
			adapter->aml_txeq.pre2 = 0;
			adapter->aml_txeq.post = 0x11111111;
		}
	} else {
		return 0;
	}

	addr = 0x141c;
	txgbe_wr32_ephy(hw, addr, adapter->aml_txeq.main);

	addr = 0x1420;
	txgbe_wr32_ephy(hw, addr, adapter->aml_txeq.pre1);

	addr = 0x1424;
	txgbe_wr32_ephy(hw, addr, adapter->aml_txeq.pre2);

	addr = 0x1428;
	txgbe_wr32_ephy(hw, addr, adapter->aml_txeq.post);

	return 0;
}

int txgbe_e56_get_temp(struct txgbe_hw *hw, int *temp)
{
	int data_code, temp_data, temp_fraction;
	u32 rdata;
	u32 timer = 0;

	while (1) {
		rdata = rd32(hw, 0x1033c);
		if (((rdata >> 12) & 0x1) != 0)
			break;

		if (timer++ > PHYINIT_TIMEOUT)
			return -ETIMEDOUT;
	}

	data_code = rdata & 0xFFF;
	temp_data = 419400 + 2205 * (data_code * 1000 / 4094 - 500);

	//Change double Temperature to int
	*temp = temp_data / 10000;
	temp_fraction = temp_data - (*temp * 10000);
	if (temp_fraction >= 5000)
		*temp += 1;

	return 0;
}

static void txgbe_e56_ovrd_symdata(struct txgbe_hw *hw)
{
	u32 addr;
	u32 rdata = 0;
	int i;

	for (i = 0; i < 4; i++) {
		addr  = E56PHY_TXS_PIN_OVRDEN_0_ADDR + (E56PHY_TXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_TXS_PIN_OVRDEN_0_OVRD_EN_TX0_SYMDATA_I, 0x1);
		txgbe_wr32_ephy(hw, addr, rdata);
	}
}

static void txgbe_e56_clear_symdata(struct txgbe_hw *hw)
{
	u32 addr;
	u32 rdata = 0;
	int i;

	for (i = 0; i < 4; i++) {
		addr  = E56PHY_TXS_PIN_OVRDEN_0_ADDR + (E56PHY_TXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_TXS_PIN_OVRDEN_0_OVRD_EN_TX0_SYMDATA_I, 0x0);
		txgbe_wr32_ephy(hw, addr, rdata);
	}
}

u32 txgbe_e56_cfg_40g(struct txgbe_hw *hw)
{
	u32 addr;
	u32 rdata = 0;
	int i;

	//CMS Config Master
	addr = E56G_CMS_ANA_OVRDVAL_7_ADDR;
	rdata = rd32_ephy(hw, addr);
	((union txgbe_e56_cms_ana_ovrdval7 *)&rdata)->ana_lcpll_lf_vco_swing_ctrl_i = 0xf;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56G_CMS_ANA_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	((union txgbe_e56_cms_ana_ovrden1 *)&rdata)
		->ovrd_en_ana_lcpll_lf_vco_swing_ctrl_i = 0x1;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56G_CMS_ANA_OVRDVAL_9_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 23, 0, 0x260000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56G_CMS_ANA_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	((union txgbe_e56_cms_ana_ovrden1 *)&rdata)->ovrd_en_ana_lcpll_lf_test_in_i = 0x1;
	txgbe_wr32_ephy(hw, addr, rdata);

	//TXS Config Master
	for (i = 0; i < 4; i++) {
		addr = E56PHY_TXS_TXS_CFG_1_ADDR + (E56PHY_TXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_TXS_TXS_CFG_1_ADAPTATION_WAIT_CNT_X256,
				0xf);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_TXS_WKUP_CNT_ADDR + (E56PHY_TXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_TXS_WKUP_CNTLDO_WKUP_CNT_X32, 0xff);
		txgbe_field_set(&rdata, E56PHY_TXS_WKUP_CNTDCC_WKUP_CNT_X32, 0xff);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_TXS_PIN_OVRDVAL_6_ADDR + (E56PHY_TXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, 19, 16, 0x6);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_TXS_PIN_OVRDEN_0_ADDR + (E56PHY_TXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_TXS_PIN_OVRDEN_0_OVRD_EN_TX0_EFUSE_BITS_I,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_TXS_ANA_OVRDVAL_1_ADDR + (E56PHY_TXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_TXS_ANA_OVRDVAL_1_ANA_TEST_DAC_I, 0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_TXS_ANA_OVRDEN_0_ADDR + (E56PHY_TXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_TXS_ANA_OVRDEN_0_OVRD_EN_ANA_TEST_DAC_I, 0x1);
		txgbe_wr32_ephy(hw, addr, rdata);
	}
	//Setting TX FFE
	txgbe_e56_phy_tx_ffe_cfg(hw, TXGBE_LINK_SPEED_40GB_FULL);

	//RXS Config master
	for (i = 0; i < 4; i++) {
		addr = E56PHY_RXS_RXS_CFG_0_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_RXS_CFG_0_DSER_DATA_SEL, 0x0);
		txgbe_field_set(&rdata, E56PHY_RXS_RXS_CFG_0_TRAIN_CLK_GATE_BYPASS_EN,
				0x1fff);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_OSC_CAL_N_CDR_1_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		((union txgbe_e56_rxs0_osc_cal_n_cdr0 *)&rdata)->prediv0 = 0xfa0;
		((union txgbe_e56_rxs0_osc_cal_n_cdr0 *)&rdata)->target_cnt0 = 0x203a;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_OSC_CAL_N_CDR_4_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		((union txgbe_e56_rxs0_osc_cal_n_cdr4 *)&rdata)->osc_range_sel0 = 0x2;
		((union txgbe_e56_rxs0_osc_cal_n_cdr4 *)&rdata)->vco_code_init = 0x7ff;
		((union txgbe_e56_rxs0_osc_cal_n_cdr4 *)&rdata)->osc_current_boost_en0 =
			0x1;
		((union txgbe_e56_rxs0_osc_cal_n_cdr4 *)&rdata)->bbcdr_current_boost0 =
			0x0;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_OSC_CAL_N_CDR_5_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_SDM_WIDTH, 0x3);
		txgbe_field_set(&rdata,
				E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_PROP_STEP_PRELOCK,
			  0xf);
		txgbe_field_set(&rdata,
				E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_PROP_STEP_POSTLOCK,
			  0xf);
		txgbe_field_set(&rdata,
				E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_GAIN_CTRL_POSTLOCK,
			  0xc);
		txgbe_field_set(&rdata,
				E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_GAIN_CTRL_PRELOCK,
			  0xf);
		txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BBCDR_RDY_CNT,
				0x3);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_OSC_CAL_N_CDR_6_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS_OSC_CAL_N_CDR_6_PI_GAIN_CTRL_PRELOCK, 0x7);
		txgbe_field_set(&rdata,
				E56PHY_RXS_OSC_CAL_N_CDR_6_PI_GAIN_CTRL_POSTLOCK,
			  0x5);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_INTL_CONFIG_0_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		((union txgbe_e56_rxs0_intl_config0 *)&rdata)->adc_intl2slice_delay0 =
			0x5555;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_INTL_CONFIG_2_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		((union txgbe_e56_rxs0_intl_config2 *)&rdata)->interleaver_hbw_disable0 =
			0x1;
		txgbe_wr32_ephy(hw, addr, rdata);

		rdata = 0x0000;
		addr = E56PHY_RXS_TXFFE_TRAINING_0_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_0_ADC_DATA_PEAK_LTH,
				0x56);
		txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_0_ADC_DATA_PEAK_UTH,
				0x6a);
		txgbe_wr32_ephy(hw, addr, rdata);

		rdata = 0x0000;
		addr = E56PHY_RXS_TXFFE_TRAINING_1_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_1_C1_LTH, 0x1e8);
		txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_1_C1_UTH, 0x78);
		txgbe_wr32_ephy(hw, addr, rdata);

		rdata = 0x0000;
		addr = E56PHY_RXS_TXFFE_TRAINING_2_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_2_CM1_LTH, 0x100);
		txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_2_CM1_UTH, 0xff);
		txgbe_wr32_ephy(hw, addr, rdata);

		rdata = 0x0000;
		addr = E56PHY_RXS_TXFFE_TRAINING_3_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_CM2_LTH, 0x4);
		txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_CM2_UTH, 0x37);
		txgbe_field_set(&rdata,
				E56PHY_RXS_TXFFE_TRAINING_3_TXFFE_TRAIN_MOD_TYPE,
			  0x38);
		txgbe_wr32_ephy(hw, addr, rdata);

		rdata = 0x0000;
		addr = E56PHY_RXS_VGA_TRAINING_0_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_0_VGA_TARGET, 0x34);
		txgbe_wr32_ephy(hw, addr, rdata);

		rdata = 0x0000;
		addr = E56PHY_RXS_VGA_TRAINING_1_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA1_CODE_INIT0,
				0xa);
		txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA2_CODE_INIT0,
				0xa);
		txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA1_CODE_INIT123,
				0xa);
		txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA2_CODE_INIT123,
				0xa);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_CTLE_TRAINING_0_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_0_CTLE_CODE_INIT0,
				0x9);
		txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_0_CTLE_CODE_INIT123,
				0x9);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_CTLE_TRAINING_1_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_1_LFEQ_LUT,
				0x1ffffea);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_CTLE_TRAINING_2_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P1,
				S10G_PHY_RX_CTLE_TAP_FRACP1);
		txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P2,
				S10G_PHY_RX_CTLE_TAP_FRACP2);
		txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P3,
				S10G_PHY_RX_CTLE_TAP_FRACP3);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_CTLE_TRAINING_3_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P1,
				S10G_PHY_RX_CTLE_TAPWT_WEIGHT1);
		txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P2,
				S10G_PHY_RX_CTLE_TAPWT_WEIGHT2);
		txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P3,
				S10G_PHY_RX_CTLE_TAPWT_WEIGHT3);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADC_SLICE_DATA_AVG_CNT,
			  0x3);
		txgbe_field_set(&rdata,
				E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADC_DATA_AVG_CNT, 0x3);
		txgbe_field_set(&rdata,
				E56PHY_RXS_OFFSET_N_GAIN_CAL_0_FE_OFFSET_DAC_CLK_CNT_X8,
			0xc);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_OFFSET_N_GAIN_CAL_1_ADDR +
		       (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_1_SAMP_ADAPT_CFG,
				0x5);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_FFE_TRAINING_0_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_FFE_TRAINING_0_FFE_TAP_EN, 0xf9ff);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_IDLE_DETECT_1_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MAX,
				0xa);
		txgbe_field_set(&rdata, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MIN,
				0x5);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56G__RXS3_ANA_OVRDVAL_11_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		((E56G__RXS3_ANA_OVRDVAL_11 *)&rdata)->ana_test_adc_clkgen_i =
			0x0;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56G__RXS0_ANA_OVRDEN_2_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		((E56G__RXS0_ANA_OVRDEN_2 *)&rdata)
			->ovrd_en_ana_test_adc_clkgen_i = 0x0;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_ANA_OVRDVAL_0_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDVAL_0_ANA_EN_RTERM_I, 0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_ANA_OVRDEN_0_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS_ANA_OVRDEN_0_OVRD_EN_ANA_EN_RTERM_I, 0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_ANA_OVRDVAL_6_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, 4, 0, 0x6);
		txgbe_field_set(&rdata, 14, 13, 0x2);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_ANA_OVRDEN_1_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS_ANA_OVRDEN_1_OVRD_EN_ANA_BBCDR_VCOFILT_BYP_I,
			0x1);
		txgbe_field_set(&rdata,
				E56PHY_RXS_ANA_OVRDEN_1_OVRD_EN_ANA_TEST_BBCDR_I,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_ANA_OVRDVAL_15_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, 2, 0, 0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_ANA_OVRDVAL_17_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS_ANA_OVRDVAL_17_ANA_VGA2_BOOST_CSTM_I, 0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_ANA_OVRDEN_3_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS_ANA_OVRDEN_3_OVRD_EN_ANA_ANABS_CONFIG_I,
			  0x1);
		txgbe_field_set(&rdata,
				E56PHY_RXS_ANA_OVRDEN_3_OVRD_EN_ANA_VGA2_BOOST_CSTM_I,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_ANA_OVRDVAL_14_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, 13, 13, 0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_ANA_OVRDEN_4_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, 13, 13, 0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_EYE_SCAN_1_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS_EYE_SCAN_1_EYE_SCAN_REF_TIMER,
				0x400);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS_RINGO_0_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, 9, 4, 0x366);
		txgbe_wr32_ephy(hw, addr, rdata);
	}

	// PDIG Config master
	addr = E56PHY_PMD_CFG_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_3_CTRL_FSM_TIMEOUT_X64K, 0x80);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_4_TRAIN_DC_ON_PERIOD_X64K, 0x18);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_4_TRAIN_DC_PERIOD_X512K, 0x3e);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_5_USE_RECENT_MARKER_OFFSET, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_0_CONT_ON_ADC_GAIN_CAL_ERR, 0x1);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_0_DO_RX_ADC_OFST_CAL, 0x3);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_0_RX_ERR_ACTION_EN, 0x40);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST0_WAIT_CNT_X4096, 0xff);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST1_WAIT_CNT_X4096, 0xff);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST2_WAIT_CNT_X4096, 0xff);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST3_WAIT_CNT_X4096, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST4_WAIT_CNT_X4096, 0x1);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST5_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST6_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST7_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST8_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST9_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST10_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST11_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST12_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST13_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST14_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST15_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_7_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_7_TRAIN_ST4_EN, 0x4bf);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_7_TRAIN_ST5_EN, 0xc4bf);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_8_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_8_TRAIN_ST7_EN, 0x47ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_12_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_12_TRAIN_ST15_EN, 0x67ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_13_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_13_TRAIN_ST0_DONE_EN, 0x8001);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_13_TRAIN_ST1_DONE_EN, 0x8002);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_14_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_14_TRAIN_ST3_DONE_EN, 0x8008);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_15_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_15_TRAIN_ST4_DONE_EN, 0x8004);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_17_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_17_TRAIN_ST8_DONE_EN, 0x20c0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_18_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_18_TRAIN_ST10_DONE_EN, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_29_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_29_TRAIN_ST15_DC_EN, 0x3f6d);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_33_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_33_TRAIN0_RATE_SEL, 0x8000);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_33_TRAIN1_RATE_SEL, 0x8000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_34_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_34_TRAIN2_RATE_SEL, 0x8000);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_34_TRAIN3_RATE_SEL, 0x8000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_KRT_TFSM_CFG_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_MAX_WAIT_TIMER_X1000K,
			0x49);
	txgbe_field_set(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_MAX_WAIT_TIMER_X8000K,
			0x37);
	txgbe_field_set(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_HOLDOFF_TIMER_X256K,
			0x2f);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_FETX_FFE_TRAIN_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_FETX_FFE_TRAIN_CFG_0_KRT_FETX_INIT_FFE_CFG_2,
			0x2);
	txgbe_wr32_ephy(hw, addr, rdata);

	return 0;
}

u32 txgbe_e56_cfg_25g(struct txgbe_hw *hw)
{
	u32 addr;
	u32 rdata = 0;

	addr = E56PHY_CMS_PIN_OVRDVAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CMS_PIN_OVRDVAL_0_INT_PLL0_TX_SIGNAL_TYPE_I,
			0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CMS_PIN_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CMS_PIN_OVRDEN_0_OVRD_EN_PLL0_TX_SIGNAL_TYPE_I,
			0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CMS_ANA_OVRDVAL_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata,
			E56PHY_CMS_ANA_OVRDVAL_2_ANA_LCPLL_HF_VCO_SWING_CTRL_I, 0xf);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CMS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata,
			E56PHY_CMS_ANA_OVRDEN_0_OVRD_EN_ANA_LCPLL_HF_VCO_SWING_CTRL_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CMS_ANA_OVRDVAL_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 23, 0, 0x260000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CMS_ANA_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata,
			E56PHY_CMS_ANA_OVRDEN_1_OVRD_EN_ANA_LCPLL_HF_TEST_IN_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_TXS_CFG_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_TXS_TXS_CFG_1_ADAPTATION_WAIT_CNT_X256, 0xf);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_WKUP_CNT_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_TXS_WKUP_CNTLDO_WKUP_CNT_X32, 0xff);
	txgbe_field_set(&rdata, E56PHY_TXS_WKUP_CNTDCC_WKUP_CNT_X32, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_PIN_OVRDVAL_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 27, 24, 0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_PIN_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_TXS_PIN_OVRDEN_0_OVRD_EN_TX0_EFUSE_BITS_I,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_ANA_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_TXS_ANA_OVRDVAL_1_ANA_TEST_DAC_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_TXS_ANA_OVRDEN_0_OVRD_EN_ANA_TEST_DAC_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	txgbe_e56_phy_tx_ffe_cfg(hw, TXGBE_LINK_SPEED_25GB_FULL);

	addr = E56PHY_RXS_RXS_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_RXS_CFG_0_DSER_DATA_SEL, 0x0);
	txgbe_field_set(&rdata, E56PHY_RXS_RXS_CFG_0_TRAIN_CLK_GATE_BYPASS_EN,
			0x1fff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_1_PREDIV1, 0x700);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_1_TARGET_CNT1, 0x2418);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_4_OSC_RANGE_SEL1, 0x1);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_4_VCO_CODE_INIT, 0x7fb);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_4_OSC_CURRENT_BOOST_EN1,
			0x0);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_4_BBCDR_CURRENT_BOOST1, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_SDM_WIDTH, 0x3);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_PROP_STEP_PRELOCK,
			0xf);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_PROP_STEP_POSTLOCK,
			0x3);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_GAIN_CTRL_POSTLOCK,
			0xa);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_GAIN_CTRL_PRELOCK,
			0xf);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BBCDR_RDY_CNT, 0x3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_6_PI_GAIN_CTRL_PRELOCK, 0x7);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_6_PI_GAIN_CTRL_POSTLOCK,
			0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_INTL_CONFIG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_INTL_CONFIG_0_ADC_INTL2SLICE_DELAY1,
			0x3333);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_INTL_CONFIG_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_INTL_CONFIG_2_INTERLEAVER_HBW_DISABLE1,
			0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_TXFFE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_0_ADC_DATA_PEAK_LTH, 0x56);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_0_ADC_DATA_PEAK_UTH, 0x6a);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_TXFFE_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_1_C1_LTH, 0x1f8);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_1_C1_UTH, 0xf0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_TXFFE_TRAINING_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_2_CM1_LTH, 0x100);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_2_CM1_UTH, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_TXFFE_TRAINING_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_CM2_LTH, 0x4);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_CM2_UTH, 0x37);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_TXFFE_TRAIN_MOD_TYPE,
			0x38);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56G__RXS0_FOM_18__ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56G__RXS0_FOM_18__DFE_COEFFL_HINT__MSB,
			E56G__RXS0_FOM_18__DFE_COEFFL_HINT__LSB, 0x0);
	//change 0x90 to 0x0 to fix 25G link up keep when cable unplugged
	txgbe_field_set(&rdata, E56G__RXS0_FOM_18__DFE_COEFFH_HINT__MSB,
			E56G__RXS0_FOM_18__DFE_COEFFH_HINT__LSB, 0x0);
	txgbe_field_set(&rdata, E56G__RXS0_FOM_18__DFE_COEFF_HINT_LOAD__MSB,
			E56G__RXS0_FOM_18__DFE_COEFF_HINT_LOAD__LSB, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_VGA_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_0_VGA_TARGET, 0x34);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_VGA_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA1_CODE_INIT0, 0xa);
	txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA2_CODE_INIT0, 0xa);
	txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA1_CODE_INIT123, 0xa);
	txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA2_CODE_INIT123, 0xa);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_0_CTLE_CODE_INIT0, 0x9);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_0_CTLE_CODE_INIT123, 0x9);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_1_LFEQ_LUT, 0x1ffffea);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P1,
			S25G_PHY_RX_CTLE_TAP_FRACP1);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P2,
			S25G_PHY_RX_CTLE_TAP_FRACP2);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P3,
			S25G_PHY_RX_CTLE_TAP_FRACP3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P1,
			S25G_PHY_RX_CTLE_TAPWT_WEIGHT1);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P2,
			S25G_PHY_RX_CTLE_TAPWT_WEIGHT2);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P3,
			S25G_PHY_RX_CTLE_TAPWT_WEIGHT3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADC_SLICE_DATA_AVG_CNT,
			0x3);
	txgbe_field_set(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADC_DATA_AVG_CNT, 0x3);
	txgbe_field_set(&rdata,
			E56PHY_RXS_OFFSET_N_GAIN_CAL_0_FE_OFFSET_DAC_CLK_CNT_X8, 0xc);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OFFSET_N_GAIN_CAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_1_SAMP_ADAPT_CFG, 0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_FFE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_FFE_TRAINING_0_FFE_TAP_EN, 0xf9ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_IDLE_DETECT_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MAX, 0xa);
	txgbe_field_set(&rdata, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MIN, 0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	txgbe_e56_ephy_config(E56G__RXS3_ANA_OVRDVAL_11, ana_test_adc_clkgen_i,
			      0x0);
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_2,
			      ovrd_en_ana_test_adc_clkgen_i, 0x0);

	addr = E56PHY_RXS_ANA_OVRDVAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDVAL_0_ANA_EN_RTERM_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDEN_0_OVRD_EN_ANA_EN_RTERM_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 4, 0, 0x0);
	txgbe_field_set(&rdata, 14, 13, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata,
			E56PHY_RXS_ANA_OVRDEN_1_OVRD_EN_ANA_BBCDR_VCOFILT_BYP_I, 0x1);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDEN_1_OVRD_EN_ANA_TEST_BBCDR_I,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_15_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 2, 0, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_17_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDVAL_17_ANA_VGA2_BOOST_CSTM_I, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDEN_3_OVRD_EN_ANA_ANABS_CONFIG_I,
			0x1);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDEN_3_OVRD_EN_ANA_VGA2_BOOST_CSTM_I,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_14_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 13, 13, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 13, 13, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_EYE_SCAN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_EYE_SCAN_1_EYE_SCAN_REF_TIMER, 0x400);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_RINGO_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 21, 12, 0x366);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_3_CTRL_FSM_TIMEOUT_X64K, 0x80);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_4_TRAIN_DC_ON_PERIOD_X64K, 0x18);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_4_TRAIN_DC_PERIOD_X512K, 0x3e);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_5_USE_RECENT_MARKER_OFFSET, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_0_CONT_ON_ADC_GAIN_CAL_ERR, 0x1);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_0_DO_RX_ADC_OFST_CAL, 0x3);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_0_RX_ERR_ACTION_EN, 0x40);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST0_WAIT_CNT_X4096, 0xff);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST1_WAIT_CNT_X4096, 0xff);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST2_WAIT_CNT_X4096, 0xff);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST3_WAIT_CNT_X4096, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST4_WAIT_CNT_X4096, 0x1);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST5_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST6_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST7_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST8_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST9_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST10_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST11_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST12_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST13_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST14_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST15_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_7_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_7_TRAIN_ST4_EN, 0x4bf);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_7_TRAIN_ST5_EN, 0xc4bf);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_8_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_8_TRAIN_ST7_EN, 0x47ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_12_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_12_TRAIN_ST15_EN, 0x67ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_13_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_13_TRAIN_ST0_DONE_EN, 0x8001);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_13_TRAIN_ST1_DONE_EN, 0x8002);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_14_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_14_TRAIN_ST3_DONE_EN, 0x8008);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_15_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_15_TRAIN_ST4_DONE_EN, 0x8004);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_17_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_17_TRAIN_ST8_DONE_EN, 0x20c0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_18_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_18_TRAIN_ST10_DONE_EN, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_29_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_29_TRAIN_ST15_DC_EN, 0x3f6d);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_33_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_33_TRAIN0_RATE_SEL, 0x8000);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_33_TRAIN1_RATE_SEL, 0x8000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_34_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_34_TRAIN2_RATE_SEL, 0x8000);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_34_TRAIN3_RATE_SEL, 0x8000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_KRT_TFSM_CFG_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_MAX_WAIT_TIMER_X1000K,
			0x49);
	txgbe_field_set(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_MAX_WAIT_TIMER_X8000K,
			0x37);
	txgbe_field_set(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_HOLDOFF_TIMER_X256K,
			0x2f);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_FETX_FFE_TRAIN_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_FETX_FFE_TRAIN_CFG_0_KRT_FETX_INIT_FFE_CFG_2,
			0x2);
	txgbe_wr32_ephy(hw, addr, rdata);

	return 0;
}

u32 txgbe_e56_cfg_10g(struct txgbe_hw *hw)
{
	u32 addr;
	u32 rdata = 0;

	addr = E56G_CMS_ANA_OVRDVAL_7_ADDR;
	rdata = rd32_ephy(hw, addr);
	((union txgbe_e56_cms_ana_ovrdval7 *)&rdata)->ana_lcpll_lf_vco_swing_ctrl_i = 0xf;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56G_CMS_ANA_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	((union txgbe_e56_cms_ana_ovrden1 *)&rdata)
		->ovrd_en_ana_lcpll_lf_vco_swing_ctrl_i = 0x1;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56G_CMS_ANA_OVRDVAL_9_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 23, 0, 0x260000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56G_CMS_ANA_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	((union txgbe_e56_cms_ana_ovrden1 *)&rdata)->ovrd_en_ana_lcpll_lf_test_in_i = 0x1;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_TXS_CFG_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_TXS_TXS_CFG_1_ADAPTATION_WAIT_CNT_X256, 0xf);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_WKUP_CNT_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_TXS_WKUP_CNTLDO_WKUP_CNT_X32, 0xff);
	txgbe_field_set(&rdata, E56PHY_TXS_WKUP_CNTDCC_WKUP_CNT_X32, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_PIN_OVRDVAL_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 19, 16, 0x6);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_PIN_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_TXS_PIN_OVRDEN_0_OVRD_EN_TX0_EFUSE_BITS_I,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_ANA_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_TXS_ANA_OVRDVAL_1_ANA_TEST_DAC_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_TXS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_TXS_ANA_OVRDEN_0_OVRD_EN_ANA_TEST_DAC_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//Setting TX FFE
	txgbe_e56_phy_tx_ffe_cfg(hw, TXGBE_LINK_SPEED_10GB_FULL);

	addr = E56PHY_RXS_RXS_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_RXS_CFG_0_DSER_DATA_SEL, 0x0);
	txgbe_field_set(&rdata, E56PHY_RXS_RXS_CFG_0_TRAIN_CLK_GATE_BYPASS_EN,
			0x1fff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	((union txgbe_e56_rxs0_osc_cal_n_cdr0 *)&rdata)->prediv0 = 0xfa0;
	((union txgbe_e56_rxs0_osc_cal_n_cdr0 *)&rdata)->target_cnt0 = 0x203a;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	((union txgbe_e56_rxs0_osc_cal_n_cdr4 *)&rdata)->osc_range_sel0 = 0x2;
	((union txgbe_e56_rxs0_osc_cal_n_cdr4 *)&rdata)->vco_code_init = 0x7ff;
	((union txgbe_e56_rxs0_osc_cal_n_cdr4 *)&rdata)->osc_current_boost_en0 = 0x1;
	((union txgbe_e56_rxs0_osc_cal_n_cdr4 *)&rdata)->bbcdr_current_boost0 = 0x0;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_SDM_WIDTH, 0x3);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_PROP_STEP_PRELOCK,
			0xf);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_PROP_STEP_POSTLOCK,
			0xf);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_GAIN_CTRL_POSTLOCK,
			0xc);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BB_CDR_GAIN_CTRL_PRELOCK,
			0xf);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_5_BBCDR_RDY_CNT, 0x3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OSC_CAL_N_CDR_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_6_PI_GAIN_CTRL_PRELOCK, 0x7);
	txgbe_field_set(&rdata, E56PHY_RXS_OSC_CAL_N_CDR_6_PI_GAIN_CTRL_POSTLOCK,
			0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_INTL_CONFIG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	((union txgbe_e56_rxs0_intl_config0 *)&rdata)->adc_intl2slice_delay0 = 0x5555;
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_INTL_CONFIG_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	((union txgbe_e56_rxs0_intl_config2 *)&rdata)->interleaver_hbw_disable0 = 0x1;
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_TXFFE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_0_ADC_DATA_PEAK_LTH, 0x56);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_0_ADC_DATA_PEAK_UTH, 0x6a);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_TXFFE_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_1_C1_LTH, 0x1e8);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_1_C1_UTH, 0x78);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_TXFFE_TRAINING_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_2_CM1_LTH, 0x100);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_2_CM1_UTH, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_TXFFE_TRAINING_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_CM2_LTH, 0x4);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_CM2_UTH, 0x37);
	txgbe_field_set(&rdata, E56PHY_RXS_TXFFE_TRAINING_3_TXFFE_TRAIN_MOD_TYPE,
			0x38);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_VGA_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_0_VGA_TARGET, 0x34);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS_VGA_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA1_CODE_INIT0, 0xa);
	txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA2_CODE_INIT0, 0xa);
	txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA1_CODE_INIT123, 0xa);
	txgbe_field_set(&rdata, E56PHY_RXS_VGA_TRAINING_1_VGA2_CODE_INIT123, 0xa);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_0_CTLE_CODE_INIT0, 0x9);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_0_CTLE_CODE_INIT123, 0x9);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_1_LFEQ_LUT, 0x1ffffea);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P1,
			S10G_PHY_RX_CTLE_TAP_FRACP1);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P2,
			S10G_PHY_RX_CTLE_TAP_FRACP2);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_2_ISI_TH_FRAC_P3,
			S10G_PHY_RX_CTLE_TAP_FRACP3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_CTLE_TRAINING_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P1,
			S10G_PHY_RX_CTLE_TAPWT_WEIGHT1);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P2,
			S10G_PHY_RX_CTLE_TAPWT_WEIGHT2);
	txgbe_field_set(&rdata, E56PHY_RXS_CTLE_TRAINING_3_TAP_WEIGHT_P3,
			S10G_PHY_RX_CTLE_TAPWT_WEIGHT3);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADC_SLICE_DATA_AVG_CNT,
			0x3);
	txgbe_field_set(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_0_ADC_DATA_AVG_CNT, 0x3);
	txgbe_field_set(&rdata,
			E56PHY_RXS_OFFSET_N_GAIN_CAL_0_FE_OFFSET_DAC_CLK_CNT_X8, 0xc);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_OFFSET_N_GAIN_CAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_OFFSET_N_GAIN_CAL_1_SAMP_ADAPT_CFG, 0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_FFE_TRAINING_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_FFE_TRAINING_0_FFE_TAP_EN, 0xf9ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_IDLE_DETECT_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MAX, 0xa);
	txgbe_field_set(&rdata, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MIN, 0x5);
	txgbe_wr32_ephy(hw, addr, rdata);

	txgbe_e56_ephy_config(E56G__RXS3_ANA_OVRDVAL_11, ana_test_adc_clkgen_i,
			      0x0);
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_2,
			      ovrd_en_ana_test_adc_clkgen_i, 0x0);

	addr = E56PHY_RXS_ANA_OVRDVAL_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDVAL_0_ANA_EN_RTERM_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDEN_0_OVRD_EN_ANA_EN_RTERM_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_6_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 4, 0, 0x6);
	txgbe_field_set(&rdata, 14, 13, 0x2);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata,
			E56PHY_RXS_ANA_OVRDEN_1_OVRD_EN_ANA_BBCDR_VCOFILT_BYP_I, 0x1);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDEN_1_OVRD_EN_ANA_TEST_BBCDR_I,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_15_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 2, 0, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_17_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDVAL_17_ANA_VGA2_BOOST_CSTM_I, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDEN_3_OVRD_EN_ANA_ANABS_CONFIG_I,
			0x1);
	txgbe_field_set(&rdata, E56PHY_RXS_ANA_OVRDEN_3_OVRD_EN_ANA_VGA2_BOOST_CSTM_I,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDVAL_14_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 13, 13, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_ANA_OVRDEN_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 13, 13, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_EYE_SCAN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS_EYE_SCAN_1_EYE_SCAN_REF_TIMER, 0x400);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS_RINGO_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, 9, 4, 0x366);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_3_CTRL_FSM_TIMEOUT_X64K, 0x80);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_4_TRAIN_DC_ON_PERIOD_X64K, 0x18);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_4_TRAIN_DC_PERIOD_X512K, 0x3e);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_PMD_CFG_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_PMD_CFG_5_USE_RECENT_MARKER_OFFSET, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_0_CONT_ON_ADC_GAIN_CAL_ERR, 0x1);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_0_DO_RX_ADC_OFST_CAL, 0x3);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_0_RX_ERR_ACTION_EN, 0x40);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST0_WAIT_CNT_X4096, 0xff);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST1_WAIT_CNT_X4096, 0xff);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST2_WAIT_CNT_X4096, 0xff);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_1_TRAIN_ST3_WAIT_CNT_X4096, 0xff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST4_WAIT_CNT_X4096, 0x1);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST5_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST6_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_2_TRAIN_ST7_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_3_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST8_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST9_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST10_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_3_TRAIN_ST11_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_4_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST12_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST13_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST14_WAIT_CNT_X4096, 0x4);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_4_TRAIN_ST15_WAIT_CNT_X4096, 0x4);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_7_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_7_TRAIN_ST4_EN, 0x4bf);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_7_TRAIN_ST5_EN, 0xc4bf);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_8_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_8_TRAIN_ST7_EN, 0x47ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_12_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_12_TRAIN_ST15_EN, 0x67ff);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_13_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_13_TRAIN_ST0_DONE_EN, 0x8001);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_13_TRAIN_ST1_DONE_EN, 0x8002);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_14_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_14_TRAIN_ST3_DONE_EN, 0x8008);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_15_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_15_TRAIN_ST4_DONE_EN, 0x8004);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_17_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_17_TRAIN_ST8_DONE_EN, 0x20c0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_18_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_18_TRAIN_ST10_DONE_EN, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_29_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_29_TRAIN_ST15_DC_EN, 0x3f6d);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_33_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_33_TRAIN0_RATE_SEL, 0x8000);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_33_TRAIN1_RATE_SEL, 0x8000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_CTRL_FSM_CFG_34_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_34_TRAIN2_RATE_SEL, 0x8000);
	txgbe_field_set(&rdata, E56PHY_CTRL_FSM_CFG_34_TRAIN3_RATE_SEL, 0x8000);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_KRT_TFSM_CFG_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_MAX_WAIT_TIMER_X1000K,
			0x49);
	txgbe_field_set(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_MAX_WAIT_TIMER_X8000K,
			0x37);
	txgbe_field_set(&rdata, E56PHY_KRT_TFSM_CFGKRT_TFSM_HOLDOFF_TIMER_X256K,
			0x2f);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_FETX_FFE_TRAIN_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_FETX_FFE_TRAIN_CFG_0_KRT_FETX_INIT_FFE_CFG_2,
			0x2);
	txgbe_wr32_ephy(hw, addr, rdata);

	return 0;
}

static int txgbe_e56_rxs_oscinit_temp_track(struct txgbe_hw *hw, u32 speed)
{
	int status = 0;
	unsigned int addr, rdata, timer;
	int T = 40;
	int RX_COARSE_MID_TD, CMVAR_RANGE_H = 0, CMVAR_RANGE_L = 0;
	int OFFSET_CENTRE_RANGE_H, OFFSET_CENTRE_RANGE_L, RANGE_FINAL;
	int osc_freq_err_occur;
	int i = 0;
	int lane_num = 1;
	struct txgbe_adapter *adapter = hw->back;
	//1. Read the temperature T just before RXS is enabled.
	txgbe_e56_get_temp(hw, &T);

	if (T < -5)
		RX_COARSE_MID_TD = 10;
	else if (T < 30)
		RX_COARSE_MID_TD = 9;
	else if (T < 65)
		RX_COARSE_MID_TD = 8;
	else if (T < 100)
		RX_COARSE_MID_TD = 7;
	else
		RX_COARSE_MID_TD = 6;

	//Set CMVAR_RANGE_H/L based on the link speed mode
	if (speed == TXGBE_LINK_SPEED_10GB_FULL ||
	    speed == TXGBE_LINK_SPEED_40GB_FULL) { //10G mode
		CMVAR_RANGE_H = S10G_CMVAR_RANGE_H;
		CMVAR_RANGE_L = S10G_CMVAR_RANGE_L;
	} else if (speed == TXGBE_LINK_SPEED_25GB_FULL) { //25G mode
		CMVAR_RANGE_H = S25G_CMVAR_RANGE_H;
		CMVAR_RANGE_L = S25G_CMVAR_RANGE_L;
	}

	if (speed == TXGBE_LINK_SPEED_40GB_FULL)
		lane_num = 4;
	// TBD select all lane
	//3. Program ALIAS::RXS::RANGE_SEL = CMVAR::RANGE_H
	// RXS0_ANA_OVRDVAL[5]
	// ana_bbcdr_osc_range_sel_i[1:0]
	for (i = 0; i < lane_num; i++) {
		rdata = 0x0000;
		addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS_ANA_OVRDVAL_5_ANA_BBCDR_OSC_RANGE_SEL_I,
			  CMVAR_RANGE_H);
		txgbe_wr32_ephy(hw, addr, rdata);

		// RXS0_ANA_OVRDEN[0]
		// [29] ovrd_en_ana_bbcdr_osc_range_sel_i
		rdata = 0x0000;
		addr = E56PHY_RXS_ANA_OVRDEN_0_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS_ANA_OVRDEN_0_OVRD_EN_ANA_BBCDR_OSC_RANGE_SEL_I,
			0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		// RXS0_OVRDVAL[0]
		// [22] rxs0_rx0_samp_cal_done_o
		rdata = 0x0000;
		addr = E56PHY_RXS0_OVRDVAL_0_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_0_RXS0_RX0_SAMP_CAL_DONE_O, 0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		// RXS0_OVRDEN[0]
		// [27] ovrd_en_rxs0_rx0_samp_cal_done_o
		rdata = 0x0000;
		addr = E56PHY_RXS0_OVRDEN_0_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDEN_0_OVRD_EN_RXS0_RX0_SAMP_CAL_DONE_O,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		//Do SEQ::RX_ENABLE to enable RXS
		rdata = 0;
		addr = E56PHY_PMD_CFG_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, (0x1 << i));
		txgbe_wr32_ephy(hw, addr, rdata);

		//b. Poll ALIAS::PDIG::CTRL_FSM_RX_ST and confirm its value is RX_SAMP_CAL_ST
		// poll CTRL_FSM_RX_ST
		rdata = 0;
		timer = 0;
		osc_freq_err_occur = 0;
		while ((rdata >> (i * 8) & 0x3f) != 0x9) { //Bit[5:0]!= 0x9
			usleep_range(500, 1000);
			// INTR[0]
			// [11:8] intr_rx_osc_freq_err
			rdata = 0;
			addr = E56PHY_INTR_0_ADDR;
			rdata = rd32_ephy(hw, addr);
			// TBD is always osc_freq_err occur?
			if (rdata & (0x100 << i)) {
				osc_freq_err_occur = 1;
				break;
			}
			rdata = 0;
			addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
			rdata = rd32_ephy(hw, addr);

			if (timer++ > PHYINIT_TIMEOUT) {
				e_info(drv, "ERROR: Wait E56PHY_CTRL_FSM_RX_STAT_0_ADDR Timeout!!!\n");
				break;
			}
		}

		//5/6.Define software variable as OFFSET_CENTRE_RANGE_H = ALIAS::RXS::COARSE
		//- RX_COARSE_MID_TD. Clear the INTR.
		rdata = 0;
		addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		OFFSET_CENTRE_RANGE_H = (rdata >> 4) & 0xf;
		if (OFFSET_CENTRE_RANGE_H > RX_COARSE_MID_TD) {
			OFFSET_CENTRE_RANGE_H =
				OFFSET_CENTRE_RANGE_H - RX_COARSE_MID_TD;
		} else {
			OFFSET_CENTRE_RANGE_H =
				RX_COARSE_MID_TD - OFFSET_CENTRE_RANGE_H;
		}

		//7. Do SEQ::RX_DISABLE to disable RXS. Poll ALIAS::PDIG::CTRL_FSM_RX_ST and confirm
		//its value is POWERDN_ST

		rdata = 0;
		addr = E56PHY_PMD_CFG_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, 0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		timer = 0;
		while (1) {
			usleep_range(500, 1000);
			rdata = 0;
			addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
			rdata = rd32_ephy(hw, addr);
			if (((rdata >> (i * 8)) & 0x3f) == 0x21)
				break;

			if (timer++ > PHYINIT_TIMEOUT) {
				e_info(drv, "ERROR: Wait E56PHY_CTRL_FSM_RX_STAT_0_ADDR Timeout!!!\n");
				break;
			}
		}

		usleep_range(500, 1000);
		rdata = 0;
		addr = E56PHY_INTR_0_ADDR;
		rdata = rd32_ephy(hw, addr);

		usleep_range(500, 1000);
		addr = E56PHY_INTR_0_ADDR;
		txgbe_wr32_ephy(hw, addr, rdata);

		usleep_range(500, 1000);
		rdata = 0;
		addr = E56PHY_INTR_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		// next round

		//9. Program ALIAS::RXS::RANGE_SEL = CMVAR::RANGE_L
		// RXS0_ANA_OVRDVAL[5]
		// ana_bbcdr_osc_range_sel_i[1:0]
		rdata = 0x0000;
		addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS_ANA_OVRDVAL_5_ANA_BBCDR_OSC_RANGE_SEL_I,
			  CMVAR_RANGE_L);
		txgbe_wr32_ephy(hw, addr, rdata);

		// RXS0_ANA_OVRDEN[0]
		// [29] ovrd_en_ana_bbcdr_osc_range_sel_i
		rdata = 0x0000;
		addr = E56PHY_RXS_ANA_OVRDEN_0_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS_ANA_OVRDEN_0_OVRD_EN_ANA_BBCDR_OSC_RANGE_SEL_I,
			0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		//10. Do SEQ::RX_ENABLE to enable RXS, and let it stop after oscillator calibration.
		// RXS0_OVRDVAL[0]
		// [22] rxs0_rx0_samp_cal_done_o
		rdata = 0x0000;
		addr = E56PHY_RXS0_OVRDVAL_0_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_0_RXS0_RX0_SAMP_CAL_DONE_O, 0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		// RXS0_OVRDEN[0]
		// [27] ovrd_en_rxs0_rx0_samp_cal_done_o
		rdata = 0x0000;
		addr = E56PHY_RXS0_OVRDEN_0_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDEN_0_OVRD_EN_RXS0_RX0_SAMP_CAL_DONE_O,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		rdata = 0;
		addr = E56PHY_PMD_CFG_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, (0x1 << i));
		txgbe_wr32_ephy(hw, addr, rdata);

		// poll CTRL_FSM_RX_ST
		timer = 0;
		osc_freq_err_occur = 0;
		while (((rdata >> (i * 8)) & 0x3f) != 0x9) { //Bit[5:0]!= 0x9
			usleep_range(500, 1000);
			// INTR[0]
			// [11:8] intr_rx_osc_freq_err
			rdata = 0;
			addr = E56PHY_INTR_0_ADDR;
			rdata = rd32_ephy(hw, addr);
			// TBD is always osc_freq_err occur?
			if ((rdata & 0x100) == 0x100) {
				osc_freq_err_occur = 1;
				break;
			}
			rdata = 0;
			addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
			rdata = rd32_ephy(hw, addr);
			if (timer++ > PHYINIT_TIMEOUT) {
				e_info(drv, "ERROR: Wait E56PHY_CTRL_FSM_RX_STAT_0_ADDR Timeout!!!\n");
				break;
			}
		}

		//11/12.Define software variable as OFFSET_CENTRE_RANGE_L = ALIAS::RXS::COARSE -
		//RX_COARSE_MID_TD. Clear the INTR.
		rdata = 0;
		addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		OFFSET_CENTRE_RANGE_L = (rdata >> 4) & 0xf;
		if (OFFSET_CENTRE_RANGE_L > RX_COARSE_MID_TD) {
			OFFSET_CENTRE_RANGE_L =
				OFFSET_CENTRE_RANGE_L - RX_COARSE_MID_TD;
		} else {
			OFFSET_CENTRE_RANGE_L =
				RX_COARSE_MID_TD - OFFSET_CENTRE_RANGE_L;
		}

		if (OFFSET_CENTRE_RANGE_L < OFFSET_CENTRE_RANGE_H)
			RANGE_FINAL = CMVAR_RANGE_L;
		else
			RANGE_FINAL = CMVAR_RANGE_H;

		//14. Do SEQ::RX_DISABLE to disable RXS. Poll ALIAS::PDIG::CTRL_FSM_RX_ST
		//and confirm its value is POWERDN_ST
		rdata = 0;
		addr = E56PHY_PMD_CFG_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, 0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		timer = 0;
		while (1) {
			usleep_range(500, 1000);
			rdata = 0;
			addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
			rdata = rd32_ephy(hw, addr);
			if (((rdata >> (i * 8)) & 0x3f) == 0x21)
				break;

			if (timer++ > PHYINIT_TIMEOUT) {
				e_info(drv, "ERROR: Wait E56PHY_CTRL_FSM_RX_STAT_0_ADDR Timeout!!!\n");
				break;
			}
		}

		//15. Since RX power-up fsm is stopped in RX_SAMP_CAL_ST,
		//it is possible the timeout interrupt is set. Clear the same by clearing
		//ALIAS::PDIG::INTR_CTRL_FSM_RX_ERR. Also clear ALIAS::PDIG::INTR_RX_OSC_FREQ_ERR
		//which could also be set.
		usleep_range(500, 1000);
		rdata = 0;
		addr = E56PHY_INTR_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		usleep_range(500, 1000);
		txgbe_wr32_ephy(hw, addr, rdata);

		usleep_range(500, 1000);
		rdata = 0;
		addr = E56PHY_INTR_0_ADDR;
		rdata = rd32_ephy(hw, addr);

		//16. Program ALIAS::RXS::RANGE_SEL = RANGE_FINAL
		rdata = 0x0000;
		addr = E56PHY_RXS_ANA_OVRDVAL_5_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS_ANA_OVRDVAL_5_ANA_BBCDR_OSC_RANGE_SEL_I,
			  RANGE_FINAL);
		txgbe_wr32_ephy(hw, addr, rdata);

		rdata = 0x0000;
		addr = E56PHY_RXS0_OVRDEN_0_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDEN_0_OVRD_EN_RXS0_RX0_SAMP_CAL_DONE_O,
			  0x0);
		txgbe_wr32_ephy(hw, addr, rdata);
	}
	//Do SEQ::RX_ENABLE
	rdata = 0;
	addr = E56PHY_PMD_CFG_0_ADDR;
	rdata = rd32_ephy(hw, addr);
	if (speed == TXGBE_LINK_SPEED_40GB_FULL)
		txgbe_field_set(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, 0xf);
	else
		txgbe_field_set(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	return status;
}

static int txgbe_e56_set_rx_ufine_lemax40(struct txgbe_hw *hw, u32 speed)
{
	int status = 0;
	unsigned int rdata;
	unsigned int ULTRAFINE_CODE;
	int i = 0;
	unsigned int CMVAR_UFINE_MAX = 0;
	u32 addr;

	for (i = 0; i < 4; i++) {
		if (speed == TXGBE_LINK_SPEED_10GB_FULL ||
		    speed == TXGBE_LINK_SPEED_40GB_FULL)
			CMVAR_UFINE_MAX = S10G_CMVAR_UFINE_MAX;
		else if (speed == TXGBE_LINK_SPEED_25GB_FULL)
			CMVAR_UFINE_MAX = S25G_CMVAR_UFINE_MAX;

		//a. Assign software defined variables as below �C
		//ii. ULTRAFINE_CODE = ALIAS::RXS::ULTRAFINE
		addr = E56G__RXS0_ANA_OVRDVAL_5_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		ULTRAFINE_CODE = EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					   ana_bbcdr_ultrafine_i);

		//b. Perform the below logic sequence �C
		while (ULTRAFINE_CODE > CMVAR_UFINE_MAX) {
			ULTRAFINE_CODE = ULTRAFINE_CODE - 1;
			addr = E56G__RXS0_ANA_OVRDVAL_5_ADDR +
			       (E56PHY_RXS_OFFSET * i);
			rdata = rd32_ephy(hw, addr);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_ultrafine_i) = ULTRAFINE_CODE;
			txgbe_wr32_ephy(hw, addr, rdata);

			//Set ovrd_en=1 to override ASIC value
			addr = E56G__RXS0_ANA_OVRDEN_1_ADDR +
			       (E56PHY_RXS_OFFSET * i);
			rdata = rd32_ephy(hw, addr);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			txgbe_wr32_ephy(hw, addr, rdata);

			// Wait until 1milliseconds or greater
			usleep_range(10000, 20000);
		}
	}
	return status;
}

static int txgbe_e56_set_rxs_ufine_lemax(struct txgbe_hw *hw, u32 speed)
{
	int status = 0;
	unsigned int rdata;
	unsigned int ULTRAFINE_CODE;

	unsigned int CMVAR_UFINE_MAX = 0;

	if (speed == TXGBE_LINK_SPEED_10GB_FULL)
		CMVAR_UFINE_MAX = S10G_CMVAR_UFINE_MAX;
	else if (speed == TXGBE_LINK_SPEED_25GB_FULL)
		CMVAR_UFINE_MAX = S25G_CMVAR_UFINE_MAX;

	//a. Assign software defined variables as below �C
	//ii. ULTRAFINE_CODE = ALIAS::RXS::ULTRAFINE
	EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
	ULTRAFINE_CODE =
		EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_ultrafine_i);

	//b. Perform the below logic sequence �C
	while (ULTRAFINE_CODE > CMVAR_UFINE_MAX) {
		ULTRAFINE_CODE = ULTRAFINE_CODE - 1;
		txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDVAL_5,
				      ana_bbcdr_ultrafine_i, ULTRAFINE_CODE);
		//Set ovrd_en=1 to override ASIC value
		txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_1,
				      ovrd_en_ana_bbcdr_ultrafine_i, 1);
		// Wait until 1milliseconds or greater
		usleep_range(10000, 20000);
	}

	return status;
}

//--------------------------------------------------------------
//compare function for qsort()
//--------------------------------------------------------------
static int compare(const void *a, const void *b)
{
	const int *num1 = (const int *)a;
	const int *num2 = (const int *)b;

	if (*num1 < *num2)
		return -1;
	else if (*num1 > *num2)
		return 1;
	else
		return 0;
}

static int txgbe_e56_set_rxrd_sec_code_40g(struct txgbe_hw *hw, int *SECOND_CODE,
					   int lane)
{
	int status = 0, i, N, median;
	unsigned int rdata;
	u32 addr;
	int array_size, RXS_BBCDR_SECOND_ORDER_ST[5];

	//Set ovrd_en=0 to read ASIC value
	addr = E56G__RXS0_ANA_OVRDEN_1_ADDR + (lane * E56PHY_RXS_OFFSET);
	rdata = rd32_ephy(hw, addr);
	EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1, ovrd_en_ana_bbcdr_int_cstm_i) = 0;
	txgbe_wr32_ephy(hw, addr, rdata);

	N = 5;
	for (i = 0; i < N; i = i + 1) {
		//set RXS_BBCDR_SECOND_ORDER_ST[i] = RXS::ANA_OVRDVAL[5]::ana_bbcdr_int_cstm_i[4:0]
		addr = E56G__RXS0_ANA_OVRDVAL_5_ADDR +
		       (lane * E56PHY_RXS_OFFSET);
		rdata = rd32_ephy(hw, addr);
		RXS_BBCDR_SECOND_ORDER_ST[i] = EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
							 ana_bbcdr_int_cstm_i);
		usleep_range(100, 200);
	}

	//sort array RXS_BBCDR_SECOND_ORDER_ST[i]
	array_size = sizeof(RXS_BBCDR_SECOND_ORDER_ST) /
		    sizeof(RXS_BBCDR_SECOND_ORDER_ST[0]);
	sort(RXS_BBCDR_SECOND_ORDER_ST, array_size,
	     sizeof(int), compare, NULL);

	median = ((N + 1) / 2) - 1;
	*SECOND_CODE = RXS_BBCDR_SECOND_ORDER_ST[median];

	return status;
}

int txgbe_e56_rxrd_sec_code(struct txgbe_hw *hw, int *SECOND_CODE)
{
	int status = 0, i, N, median;
	unsigned int rdata;
	int array_size, RXS_BBCDR_SECOND_ORDER_ST[5];

	//Set ovrd_en=0 to read ASIC value
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_1,
			      ovrd_en_ana_bbcdr_int_cstm_i, 0);

	N = 5;
	for (i = 0; i < N; i = i + 1) {
		//set RXS_BBCDR_SECOND_ORDER_ST[i] = RXS::ANA_OVRDVAL[5]::ana_bbcdr_int_cstm_i[4:0]
		EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
		RXS_BBCDR_SECOND_ORDER_ST[i] = EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
							 ana_bbcdr_int_cstm_i);
		usleep_range(100, 200);
	}

	//sort array RXS_BBCDR_SECOND_ORDER_ST[i]
	array_size = sizeof(RXS_BBCDR_SECOND_ORDER_ST) /
		    sizeof(RXS_BBCDR_SECOND_ORDER_ST[0]);
	sort(RXS_BBCDR_SECOND_ORDER_ST, array_size,
	     sizeof(int), compare, NULL);

	median = ((N + 1) / 2) - 1;
	*SECOND_CODE = RXS_BBCDR_SECOND_ORDER_ST[median];

	return status;
}

int txgbe_temp_track_seq_40g(struct txgbe_hw *hw, u32 speed)
{
	int status = 0;
	unsigned int rdata;
	int SECOND_CODE;
	int COARSE_CODE;
	int FINE_CODE;
	int ULTRAFINE_CODE;

	int CMVAR_SEC_LOW_TH;
	int CMVAR_UFINE_MAX = 0;
	int CMVAR_FINE_MAX;
	int CMVAR_UFINE_UMAX_WRAP = 0;
	int CMVAR_COARSE_MAX;
	int CMVAR_UFINE_FMAX_WRAP = 0;
	int CMVAR_FINE_FMAX_WRAP = 0;
	int CMVAR_SEC_HIGH_TH;
	int CMVAR_UFINE_MIN;
	int CMVAR_FINE_MIN;
	int CMVAR_UFINE_UMIN_WRAP;
	int CMVAR_COARSE_MIN;
	int CMVAR_UFINE_FMIN_WRAP;
	int CMVAR_FINE_FMIN_WRAP;
	int i;
	u32 addr;
	int temp;
	struct txgbe_adapter *adapter = hw->back;

	for (i = 0; i < 4; i++) {
		if (speed == TXGBE_LINK_SPEED_10GB_FULL ||
		    speed == TXGBE_LINK_SPEED_40GB_FULL) {
			CMVAR_SEC_LOW_TH = S10G_CMVAR_SEC_LOW_TH;
			CMVAR_UFINE_MAX = S10G_CMVAR_UFINE_MAX;
			CMVAR_FINE_MAX = S10G_CMVAR_FINE_MAX;
			CMVAR_UFINE_UMAX_WRAP = S10G_CMVAR_UFINE_UMAX_WRAP;
			CMVAR_COARSE_MAX = S10G_CMVAR_COARSE_MAX;
			CMVAR_UFINE_FMAX_WRAP = S10G_CMVAR_UFINE_FMAX_WRAP;
			CMVAR_FINE_FMAX_WRAP = S10G_CMVAR_FINE_FMAX_WRAP;
			CMVAR_SEC_HIGH_TH = S10G_CMVAR_SEC_HIGH_TH;
			CMVAR_UFINE_MIN = S10G_CMVAR_UFINE_MIN;
			CMVAR_FINE_MIN = S10G_CMVAR_FINE_MIN;
			CMVAR_UFINE_UMIN_WRAP = S10G_CMVAR_UFINE_UMIN_WRAP;
			CMVAR_COARSE_MIN = S10G_CMVAR_COARSE_MIN;
			CMVAR_UFINE_FMIN_WRAP = S10G_CMVAR_UFINE_FMIN_WRAP;
			CMVAR_FINE_FMIN_WRAP = S10G_CMVAR_FINE_FMIN_WRAP;
		} else if (speed == TXGBE_LINK_SPEED_25GB_FULL) {
			CMVAR_SEC_LOW_TH = S25G_CMVAR_SEC_LOW_TH;
			CMVAR_UFINE_MAX = S25G_CMVAR_UFINE_MAX;
			CMVAR_FINE_MAX = S25G_CMVAR_FINE_MAX;
			CMVAR_UFINE_UMAX_WRAP = S25G_CMVAR_UFINE_UMAX_WRAP;
			CMVAR_COARSE_MAX = S25G_CMVAR_COARSE_MAX;
			CMVAR_UFINE_FMAX_WRAP = S25G_CMVAR_UFINE_FMAX_WRAP;
			CMVAR_FINE_FMAX_WRAP = S25G_CMVAR_FINE_FMAX_WRAP;
			CMVAR_SEC_HIGH_TH = S25G_CMVAR_SEC_HIGH_TH;
			CMVAR_UFINE_MIN = S25G_CMVAR_UFINE_MIN;
			CMVAR_FINE_MIN = S25G_CMVAR_FINE_MIN;
			CMVAR_UFINE_UMIN_WRAP = S25G_CMVAR_UFINE_UMIN_WRAP;
			CMVAR_COARSE_MIN = S25G_CMVAR_COARSE_MIN;
			CMVAR_UFINE_FMIN_WRAP = S25G_CMVAR_UFINE_FMIN_WRAP;
			CMVAR_FINE_FMIN_WRAP = S25G_CMVAR_FINE_FMIN_WRAP;
		} else {
			e_info(drv, "Error Speed\n");
			return 0;
		}

		status = txgbe_e56_get_temp(hw, &temp);
		if (status)
			return 0;

		adapter->amlite_temp = temp;

		//Assign software defined variables as below �C
		//a. SECOND_CODE = ALIAS::RXS::SECOND_ORDER
		status |= txgbe_e56_set_rxrd_sec_code_40g(hw, &SECOND_CODE, i);

		//b. COARSE_CODE = ALIAS::RXS::COARSE
		//c. FINE_CODE = ALIAS::RXS::FINE
		//d. ULTRAFINE_CODE = ALIAS::RXS::ULTRAFINE
		addr = E56G__RXS0_ANA_OVRDVAL_5_ADDR + (E56PHY_RXS_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		COARSE_CODE =
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_coarse_i);
		FINE_CODE =
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_fine_i);
		ULTRAFINE_CODE = EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					   ana_bbcdr_ultrafine_i);

		if (SECOND_CODE <= CMVAR_SEC_LOW_TH) {
			if (ULTRAFINE_CODE < CMVAR_UFINE_MAX) {
				addr = E56G__RXS0_ANA_OVRDVAL_5_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_ultrafine_i) =
					ULTRAFINE_CODE + 1;
				txgbe_wr32_ephy(hw, addr, rdata);

				//Set ovrd_en=1 to override ASIC value
				addr = E56G__RXS0_ANA_OVRDEN_1_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
				txgbe_wr32_ephy(hw, addr, rdata);
			} else if (FINE_CODE < CMVAR_FINE_MAX) {
				addr = E56G__RXS0_ANA_OVRDVAL_5_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_ultrafine_i) =
					CMVAR_UFINE_UMAX_WRAP;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_fine_i) = FINE_CODE + 1;
				txgbe_wr32_ephy(hw, addr, rdata);
				addr = E56G__RXS0_ANA_OVRDEN_1_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_fine_i) = 1;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
				txgbe_wr32_ephy(hw, addr, rdata);
			} else if (COARSE_CODE < CMVAR_COARSE_MAX) {
				addr = E56G__RXS0_ANA_OVRDVAL_5_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_ultrafine_i) =
					CMVAR_UFINE_FMAX_WRAP;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_fine_i) =
					CMVAR_FINE_FMAX_WRAP;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_coarse_i) = COARSE_CODE + 1;
				txgbe_wr32_ephy(hw, addr, rdata);

				addr = E56G__RXS0_ANA_OVRDEN_1_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_coarse_i) = 1;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_fine_i) = 1;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
				txgbe_wr32_ephy(hw, addr, rdata);
			} else {
				e_info(drv, "ERROR: (SECOND_CODE <= CMVAR_SEC_LOW_TH) temperature tracking occurs Error condition\n");
			}
		} else if (SECOND_CODE >= CMVAR_SEC_HIGH_TH) {
			if (ULTRAFINE_CODE > CMVAR_UFINE_MIN) {
				addr = E56G__RXS0_ANA_OVRDVAL_5_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_ultrafine_i) =
					ULTRAFINE_CODE - 1;
				txgbe_wr32_ephy(hw, addr, rdata);

				//Set ovrd_en=1 to override ASIC value
				addr = E56G__RXS0_ANA_OVRDEN_1_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
				txgbe_wr32_ephy(hw, addr, rdata);
			} else if (FINE_CODE > CMVAR_FINE_MIN) {
				addr = E56G__RXS0_ANA_OVRDVAL_5_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_ultrafine_i) =
					CMVAR_UFINE_UMIN_WRAP;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_fine_i) = FINE_CODE - 1;
				txgbe_wr32_ephy(hw, addr, rdata);

				addr = E56G__RXS0_ANA_OVRDEN_1_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_fine_i) = 1;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
				txgbe_wr32_ephy(hw, addr, rdata);
			} else if (COARSE_CODE > CMVAR_COARSE_MIN) {
				addr = E56G__RXS0_ANA_OVRDVAL_5_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_ultrafine_i) =
					CMVAR_UFINE_FMIN_WRAP;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_fine_i) =
					CMVAR_FINE_FMIN_WRAP;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
					  ana_bbcdr_coarse_i) = COARSE_CODE - 1;
				txgbe_wr32_ephy(hw, addr, rdata);

				addr = E56G__RXS0_ANA_OVRDEN_1_ADDR +
				       (E56PHY_RXS_OFFSET * i);
				rdata = rd32_ephy(hw, addr);
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_coarse_i) = 1;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_fine_i) = 1;
				EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
					  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
				txgbe_wr32_ephy(hw, addr, rdata);
			} else {
				e_info(drv, "ERROR: (SECOND_CODE >= CMVAR_SEC_HIGH_TH) temperature tracking occurs Error condition\n");
			}
		}
	}
	return status;
}

int txgbe_temp_track_seq(struct txgbe_hw *hw, u32 speed)
{
	struct txgbe_adapter *adapter = hw->back;
	int status = 0;
	unsigned int rdata;
	int SECOND_CODE;
	int COARSE_CODE;
	int FINE_CODE;
	int ULTRAFINE_CODE;

	int CMVAR_SEC_LOW_TH;
	int CMVAR_UFINE_MAX = 0;
	int CMVAR_FINE_MAX;
	int CMVAR_UFINE_UMAX_WRAP = 0;
	int CMVAR_COARSE_MAX;
	int CMVAR_UFINE_FMAX_WRAP = 0;
	int CMVAR_FINE_FMAX_WRAP = 0;
	int CMVAR_SEC_HIGH_TH;
	int CMVAR_UFINE_MIN;
	int CMVAR_FINE_MIN;
	int CMVAR_UFINE_UMIN_WRAP;
	int CMVAR_COARSE_MIN;
	int CMVAR_UFINE_FMIN_WRAP;
	int CMVAR_FINE_FMIN_WRAP;
	int temp;

	if (speed == TXGBE_LINK_SPEED_10GB_FULL) {
		CMVAR_SEC_LOW_TH = S10G_CMVAR_SEC_LOW_TH;
		CMVAR_UFINE_MAX = S10G_CMVAR_UFINE_MAX;
		CMVAR_FINE_MAX = S10G_CMVAR_FINE_MAX;
		CMVAR_UFINE_UMAX_WRAP = S10G_CMVAR_UFINE_UMAX_WRAP;
		CMVAR_COARSE_MAX = S10G_CMVAR_COARSE_MAX;
		CMVAR_UFINE_FMAX_WRAP = S10G_CMVAR_UFINE_FMAX_WRAP;
		CMVAR_FINE_FMAX_WRAP = S10G_CMVAR_FINE_FMAX_WRAP;
		CMVAR_SEC_HIGH_TH = S10G_CMVAR_SEC_HIGH_TH;
		CMVAR_UFINE_MIN = S10G_CMVAR_UFINE_MIN;
		CMVAR_FINE_MIN = S10G_CMVAR_FINE_MIN;
		CMVAR_UFINE_UMIN_WRAP = S10G_CMVAR_UFINE_UMIN_WRAP;
		CMVAR_COARSE_MIN = S10G_CMVAR_COARSE_MIN;
		CMVAR_UFINE_FMIN_WRAP = S10G_CMVAR_UFINE_FMIN_WRAP;
		CMVAR_FINE_FMIN_WRAP = S10G_CMVAR_FINE_FMIN_WRAP;
	} else if (speed == TXGBE_LINK_SPEED_25GB_FULL) {
		CMVAR_SEC_LOW_TH = S25G_CMVAR_SEC_LOW_TH;
		CMVAR_UFINE_MAX = S25G_CMVAR_UFINE_MAX;
		CMVAR_FINE_MAX = S25G_CMVAR_FINE_MAX;
		CMVAR_UFINE_UMAX_WRAP = S25G_CMVAR_UFINE_UMAX_WRAP;
		CMVAR_COARSE_MAX = S25G_CMVAR_COARSE_MAX;
		CMVAR_UFINE_FMAX_WRAP = S25G_CMVAR_UFINE_FMAX_WRAP;
		CMVAR_FINE_FMAX_WRAP = S25G_CMVAR_FINE_FMAX_WRAP;
		CMVAR_SEC_HIGH_TH = S25G_CMVAR_SEC_HIGH_TH;
		CMVAR_UFINE_MIN = S25G_CMVAR_UFINE_MIN;
		CMVAR_FINE_MIN = S25G_CMVAR_FINE_MIN;
		CMVAR_UFINE_UMIN_WRAP = S25G_CMVAR_UFINE_UMIN_WRAP;
		CMVAR_COARSE_MIN = S25G_CMVAR_COARSE_MIN;
		CMVAR_UFINE_FMIN_WRAP = S25G_CMVAR_UFINE_FMIN_WRAP;
		CMVAR_FINE_FMIN_WRAP = S25G_CMVAR_FINE_FMIN_WRAP;
	} else {
		e_info(drv, "Error Speed\n");
		return 0;
	}

	status = txgbe_e56_get_temp(hw, &temp);
	if (status)
		return 0;

	adapter->amlite_temp = temp;

	//Assign software defined variables as below �C
	//a. SECOND_CODE = ALIAS::RXS::SECOND_ORDER
	status |= txgbe_e56_rxrd_sec_code(hw, &SECOND_CODE);

	//b. COARSE_CODE = ALIAS::RXS::COARSE
	//c. FINE_CODE = ALIAS::RXS::FINE
	//d. ULTRAFINE_CODE = ALIAS::RXS::ULTRAFINE
	EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
	COARSE_CODE = EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_coarse_i);
	FINE_CODE = EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_fine_i);
	ULTRAFINE_CODE =
		EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_ultrafine_i);

	if (SECOND_CODE <= CMVAR_SEC_LOW_TH) {
		if (ULTRAFINE_CODE < CMVAR_UFINE_MAX) {
			txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDVAL_5,
					      ana_bbcdr_ultrafine_i,
					      ULTRAFINE_CODE + 1);
			//Set ovrd_en=1 to override ASIC value
			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else if (FINE_CODE < CMVAR_FINE_MAX) {
			EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_ultrafine_i) =
				CMVAR_UFINE_UMAX_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_fine_i) =
				FINE_CODE + 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDVAL_5);

			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_fine_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else if (COARSE_CODE < CMVAR_COARSE_MAX) {
			EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_ultrafine_i) =
				CMVAR_UFINE_FMAX_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_fine_i) =
				CMVAR_FINE_FMAX_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_coarse_i) = COARSE_CODE + 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDVAL_5);

			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_coarse_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_fine_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else {
			e_info(drv, "ERROR: (SECOND_CODE <= CMVAR_SEC_LOW_TH) temperature tracking occurs Error condition\n");
		}
	} else if (SECOND_CODE >= CMVAR_SEC_HIGH_TH) {
		if (ULTRAFINE_CODE > CMVAR_UFINE_MIN) {
			txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDVAL_5,
					      ana_bbcdr_ultrafine_i,
					      ULTRAFINE_CODE - 1);
			//Set ovrd_en=1 to override ASIC value
			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else if (FINE_CODE > CMVAR_FINE_MIN) {
			EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_ultrafine_i) =
				CMVAR_UFINE_UMIN_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_fine_i) =
				FINE_CODE - 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDVAL_5);
			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_fine_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else if (COARSE_CODE > CMVAR_COARSE_MIN) {
			EPHY_RREG(E56G__RXS0_ANA_OVRDVAL_5);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_ultrafine_i) =
				CMVAR_UFINE_FMIN_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5, ana_bbcdr_fine_i) =
				CMVAR_FINE_FMIN_WRAP;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDVAL_5,
				  ana_bbcdr_coarse_i) = COARSE_CODE - 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDVAL_5);
			EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_coarse_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_fine_i) = 1;
			EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
				  ovrd_en_ana_bbcdr_ultrafine_i) = 1;
			EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);
		} else {
			e_info(drv, "ERROR: (SECOND_CODE >= CMVAR_SEC_HIGH_TH) temperature\n");
		}
	}

	return status;
}

static int txgbe_e56_ctle_bypass_seq(struct txgbe_hw *hw, u32 speed)
{
	int status = 0;
	unsigned int rdata;

	//1. Program the following RXS registers as mentioned below.
	//RXS::ANA_OVRDVAL[0]::ana_ctle_bypass_i = 1��b1
	//RXS::ANA_OVRDEN[0]::ovrd_en_ana_ctle_bypass_i = 1��b1
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDVAL_0, ana_ctle_bypass_i, 1);
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_0,
			      ovrd_en_ana_ctle_bypass_i, 1);

	//RXS::ANA_OVRDVAL[3]::ana_ctle_cz_cstm_i[4:0] = 0
	//RXS::ANA_OVRDEN[0]::ovrd_en_ana_ctle_cz_cstm_i = 1��b1
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDVAL_3, ana_ctle_cz_cstm_i, 0);
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_0,
			      ovrd_en_ana_ctle_cz_cstm_i, 1);

	//2. Program the following PDIG registers as mentioned below.
	//PDIG::RXS<n>_OVRDVAL[1]::rxs<n>_rx0_ctle_train_en_i = 1��b0
	//PDIG::RXS<n>_OVRDEN[1]::ovrd_en_rxs<n>_rx0_ctle_train_en_i = 1��b1
	//
	//PDIG::RXS<n>_OVRDVAL[1]::rxs<n>_rx0_ctle_train_done_o = 1��b1
	//PDIG::RXS<n>_OVRDEN[1]::ovrd_en_rxs<n>_rx0_ctle_train_done_o = 1��b1
	EPHY_RREG(E56G__PMD_RXS0_OVRDVAL_1);
	EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1, rxs0_rx0_ctle_train_en_i) = 0;
	EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1, rxs0_rx0_ctle_train_done_o) = 1;
	EPHY_WREG(E56G__PMD_RXS0_OVRDVAL_1);

	EPHY_RREG(E56G__PMD_RXS0_OVRDEN_1);
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_ctle_train_en_i) =
		1;
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_ctle_train_done_o) =
		1;
	EPHY_WREG(E56G__PMD_RXS0_OVRDEN_1);

	if (speed == TXGBE_LINK_SPEED_40GB_FULL) {
		//1. Program the following RXS registers as mentioned below.
		//RXS::ANA_OVRDVAL[0]::ana_ctle_bypass_i = 1��b1
		//RXS::ANA_OVRDEN[0]::ovrd_en_ana_ctle_bypass_i = 1��b1
		txgbe_e56_ephy_config(E56G__RXS1_ANA_OVRDVAL_0,
				      ana_ctle_bypass_i, 1);
		txgbe_e56_ephy_config(E56G__RXS1_ANA_OVRDEN_0,
				      ovrd_en_ana_ctle_bypass_i, 1);
		txgbe_e56_ephy_config(E56G__RXS2_ANA_OVRDVAL_0,
				      ana_ctle_bypass_i, 1);
		txgbe_e56_ephy_config(E56G__RXS2_ANA_OVRDEN_0,
				      ovrd_en_ana_ctle_bypass_i, 1);
		txgbe_e56_ephy_config(E56G__RXS3_ANA_OVRDVAL_0,
				      ana_ctle_bypass_i, 1);
		txgbe_e56_ephy_config(E56G__RXS3_ANA_OVRDEN_0,
				      ovrd_en_ana_ctle_bypass_i, 1);

		//RXS::ANA_OVRDVAL[3]::ana_ctle_cz_cstm_i[4:0] = 0
		//RXS::ANA_OVRDEN[0]::ovrd_en_ana_ctle_cz_cstm_i = 1��b1
		txgbe_e56_ephy_config(E56G__RXS1_ANA_OVRDVAL_3,
				      ana_ctle_cz_cstm_i, 0);
		txgbe_e56_ephy_config(E56G__RXS1_ANA_OVRDEN_0,
				      ovrd_en_ana_ctle_cz_cstm_i, 1);
		txgbe_e56_ephy_config(E56G__RXS2_ANA_OVRDVAL_3,
				      ana_ctle_cz_cstm_i, 0);
		txgbe_e56_ephy_config(E56G__RXS2_ANA_OVRDEN_0,
				      ovrd_en_ana_ctle_cz_cstm_i, 1);
		txgbe_e56_ephy_config(E56G__RXS3_ANA_OVRDVAL_3,
				      ana_ctle_cz_cstm_i, 0);
		txgbe_e56_ephy_config(E56G__RXS3_ANA_OVRDEN_0,
				      ovrd_en_ana_ctle_cz_cstm_i, 1);

		//2. Program the following PDIG registers as mentioned below.
		//PDIG::RXS<n>_OVRDVAL[1]::rxs<n>_rx0_ctle_train_en_i = 1��b0
		//PDIG::RXS<n>_OVRDEN[1]::ovrd_en_rxs<n>_rx0_ctle_train_en_i = 1��b1
		//
		//PDIG::RXS<n>_OVRDVAL[1]::rxs<n>_rx0_ctle_train_done_o = 1��b1
		//PDIG::RXS<n>_OVRDEN[1]::ovrd_en_rxs<n>_rx0_ctle_train_done_o = 1��b1
		EPHY_RREG(E56G__PMD_RXS1_OVRDVAL_1);
		EPHY_XFLD(E56G__PMD_RXS1_OVRDVAL_1, rxs1_rx0_ctle_train_en_i) =
			0;
		EPHY_XFLD(E56G__PMD_RXS1_OVRDVAL_1,
			  rxs1_rx0_ctle_train_done_o) = 1;
		EPHY_WREG(E56G__PMD_RXS1_OVRDVAL_1);
		EPHY_RREG(E56G__PMD_RXS2_OVRDVAL_1);
		EPHY_XFLD(E56G__PMD_RXS2_OVRDVAL_1, rxs2_rx0_ctle_train_en_i) =
			0;
		EPHY_XFLD(E56G__PMD_RXS2_OVRDVAL_1,
			  rxs2_rx0_ctle_train_done_o) = 1;
		EPHY_WREG(E56G__PMD_RXS2_OVRDVAL_1);
		EPHY_RREG(E56G__PMD_RXS3_OVRDVAL_1);
		EPHY_XFLD(E56G__PMD_RXS3_OVRDVAL_1, rxs3_rx0_ctle_train_en_i) =
			0;
		EPHY_XFLD(E56G__PMD_RXS3_OVRDVAL_1,
			  rxs3_rx0_ctle_train_done_o) = 1;
		EPHY_WREG(E56G__PMD_RXS3_OVRDVAL_1);

		EPHY_RREG(E56G__PMD_RXS1_OVRDEN_1);
		EPHY_XFLD(E56G__PMD_RXS1_OVRDEN_1,
			  ovrd_en_rxs1_rx0_ctle_train_en_i) = 1;
		EPHY_XFLD(E56G__PMD_RXS1_OVRDEN_1,
			  ovrd_en_rxs1_rx0_ctle_train_done_o) = 1;
		EPHY_WREG(E56G__PMD_RXS1_OVRDEN_1);
		EPHY_RREG(E56G__PMD_RXS2_OVRDEN_1);
		EPHY_XFLD(E56G__PMD_RXS2_OVRDEN_1,
			  ovrd_en_rxs2_rx0_ctle_train_en_i) = 1;
		EPHY_XFLD(E56G__PMD_RXS2_OVRDEN_1,
			  ovrd_en_rxs2_rx0_ctle_train_done_o) = 1;
		EPHY_WREG(E56G__PMD_RXS2_OVRDEN_1);
		EPHY_RREG(E56G__PMD_RXS3_OVRDEN_1);
		EPHY_XFLD(E56G__PMD_RXS3_OVRDEN_1,
			  ovrd_en_rxs3_rx0_ctle_train_en_i) = 1;
		EPHY_XFLD(E56G__PMD_RXS3_OVRDEN_1,
			  ovrd_en_rxs3_rx0_ctle_train_done_o) = 1;
		EPHY_WREG(E56G__PMD_RXS3_OVRDEN_1);
	}
	return status;
}

static int txgbe_e56_rxs_calib_adapt_seq40(struct txgbe_hw *hw, u32 speed)
{
	int status = 0, i, j;
	u32 addr, timer;
	u32 rdata = 0x0;
	u32 bypass_ctle = true;

	for (i = 0; i < 4; i++) {
		rdata = 0x0000;
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR + (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
			  0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDEN_2_ADDR + (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDEN_2_OVRD_EN_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
			0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDVAL_1_ADDR + (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
			  0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDEN_2_ADDR + (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDEN_2_OVRD_EN_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
			0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		rdata = 0x0000;
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR + (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_EN_I,
			  0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDEN_1_ADDR + (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_ADC_INTL_CAL_EN_I,
			0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDVAL_1_ADDR + (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_DONE_O,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDEN_1_ADDR + (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_ADC_INTL_CAL_DONE_O,
			0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDVAL_1_ADDR + (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_ADAPT_EN_I,
			  0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDEN_2_ADDR + (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDEN_2_OVRD_EN_RXS0_RX0_ADC_INTL_ADAPT_EN_I,
			0x1);
		txgbe_wr32_ephy(hw, addr, rdata);
	}

	if (bypass_ctle == 1)
		txgbe_e56_ctle_bypass_seq(hw, speed);
	txgbe_e56_rxs_oscinit_temp_track(hw, speed);

	addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
	timer = 0;
	rdata = 0;
	while (EPHY_XFLD(E56G__PMD_CTRL_FSM_RX_STAT_0, ctrl_fsm_rx0_st) !=
		       E56PHY_RX_RDY_ST ||
	       EPHY_XFLD(E56G__PMD_CTRL_FSM_RX_STAT_0, ctrl_fsm_rx1_st) !=
		       E56PHY_RX_RDY_ST ||
	       EPHY_XFLD(E56G__PMD_CTRL_FSM_RX_STAT_0, ctrl_fsm_rx2_st) !=
		       E56PHY_RX_RDY_ST ||
	       EPHY_XFLD(E56G__PMD_CTRL_FSM_RX_STAT_0, ctrl_fsm_rx3_st) !=
		       E56PHY_RX_RDY_ST) {
		rdata = rd32_ephy(hw, addr);
		usleep_range(500, 1000);
		if (timer++ > PHYINIT_TIMEOUT) {
			//Do SEQ::RX_DISABLE
			rdata = 0;
			addr = E56PHY_PMD_CFG_0_ADDR;
			rdata = rd32_ephy(hw, addr);
			txgbe_field_set(&rdata, E56PHY_PMD_CFG_0_RX_EN_CFG, 0x0);
			txgbe_wr32_ephy(hw, addr, rdata);
			return TXGBE_ERR_TIMEOUT;
		}
	}

	//RXS ADC adaptation sequence
	//txgbe_e56_rxs_adc_adapt_seq
	rdata = 0;
	timer = 0;
	while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1, rxs0_rx0_cdr_rdy_o) != 1) {
		EPHY_RREG(E56G__PMD_RXS0_OVRDVAL_1);
		usleep_range(500, 1000);
		if (timer++ > PHYINIT_TIMEOUT)
			return TXGBE_ERR_TIMEOUT;
	}

	rdata = 0;
	timer = 0;
	while (EPHY_XFLD(E56G__PMD_RXS1_OVRDVAL_1, rxs1_rx0_cdr_rdy_o) != 1) {
		EPHY_RREG(E56G__PMD_RXS1_OVRDVAL_1);
		usleep_range(500, 1000);
		if (timer++ > PHYINIT_TIMEOUT)
			return TXGBE_ERR_TIMEOUT;
	}
	rdata = 0;
	timer = 0;
	while (EPHY_XFLD(E56G__PMD_RXS2_OVRDVAL_1, rxs2_rx0_cdr_rdy_o) != 1) {
		EPHY_RREG(E56G__PMD_RXS2_OVRDVAL_1);
		usleep_range(500, 1000);
		if (timer++ > PHYINIT_TIMEOUT)
			return TXGBE_ERR_TIMEOUT;
	}

	rdata = 0;
	timer = 0;
	while (EPHY_XFLD(E56G__PMD_RXS3_OVRDVAL_1, rxs3_rx0_cdr_rdy_o) != 1) {
		EPHY_RREG(E56G__PMD_RXS3_OVRDVAL_1);
		usleep_range(500, 1000);
		if (timer++ > PHYINIT_TIMEOUT)
			return TXGBE_ERR_TIMEOUT;
	}

	for (i = 0; i < 4; i++) {
		//4. Disable VGA and CTLE training so that they don't interfere with ADC calibration
		//a. Set ALIAS::RXS::VGA_TRAIN_EN = 0b0
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_VGA_TRAIN_EN_I,
				0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDEN_1_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_VGA_TRAIN_EN_I,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		//b. Set ALIAS::RXS::CTLE_TRAIN_EN = 0b0
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_CTLE_TRAIN_EN_I, 0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDEN_1_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_CTLE_TRAIN_EN_I,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		//5. Perform ADC interleaver calibration
		//a. Remove the OVERRIDE on ALIAS::RXS::ADC_INTL_CAL_DONE
		addr = E56PHY_RXS0_OVRDEN_1_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_ADC_INTL_CAL_DONE_O,
			0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDVAL_1_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_EN_I,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56PHY_RXS0_OVRDVAL_1_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		timer = 0;
		while (((rdata >>
			 E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_DONE_O_LSB) &
			1) != 1) {
			rdata = rd32_ephy(hw, addr);
			usleep_range(1000, 2000);

			if (timer++ > PHYINIT_TIMEOUT)
				break;
		}

		for (j = 0; j < 16; j++) {
			//a. ALIAS::RXS::ADC_OFST_ADAPT_EN = 0b1
			addr = E56PHY_RXS0_OVRDVAL_1_ADDR +
			       (E56PHY_PMD_RX_OFFSET * i);
			rdata = rd32_ephy(hw, addr);
			txgbe_field_set(&rdata,
					E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
				0x1);
			txgbe_wr32_ephy(hw, addr, rdata);

			//b. Wait for 1ms or greater
			addr = E56G__PMD_RXS0_OVRDEN_2_ADDR +
			       (E56PHY_PMD_RX_OFFSET * i);
			rdata = rd32_ephy(hw, addr);
			EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_2,
				  ovrd_en_rxs0_rx0_adc_ofst_adapt_done_o) = 0;
			txgbe_wr32_ephy(hw, addr, rdata);

			rdata = 0;
			addr = E56G__PMD_RXS0_OVRDVAL_1_ADDR +
			       (E56PHY_PMD_RX_OFFSET * i);
			timer = 0;
			while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1,
					 rxs0_rx0_adc_ofst_adapt_done_o) != 1) {
				rdata = rd32_ephy(hw, addr);
				usleep_range(500, 1000);
				if (timer++ > PHYINIT_TIMEOUT)
					break;
			}

			//c. ALIAS::RXS::ADC_OFST_ADAPT_EN = 0b0
			rdata = 0x0000;
			addr = E56PHY_RXS0_OVRDVAL_1_ADDR +
			       (E56PHY_PMD_RX_OFFSET * i);
			rdata = rd32_ephy(hw, addr);
			txgbe_field_set(&rdata,
					E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
				0x0);
			txgbe_wr32_ephy(hw, addr, rdata);

			//d. ALIAS::RXS::ADC_GAIN_ADAPT_EN = 0b1
			rdata = 0x0000;
			addr = E56PHY_RXS0_OVRDVAL_1_ADDR +
			       (E56PHY_PMD_RX_OFFSET * i);
			rdata = rd32_ephy(hw, addr);
			txgbe_field_set(&rdata,
					E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
				0x1);
			txgbe_wr32_ephy(hw, addr, rdata);

			//e. Wait for 1ms or greater
			addr = E56G__PMD_RXS0_OVRDEN_2_ADDR +
			       (E56PHY_PMD_RX_OFFSET * i);
			rdata = rd32_ephy(hw, addr);
			EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_2,
				  ovrd_en_rxs0_rx0_adc_ofst_adapt_done_o) = 0;
			txgbe_wr32_ephy(hw, addr, rdata);

			rdata = 0;
			timer = 0;
			addr = E56G__PMD_RXS0_OVRDVAL_1_ADDR +
			       (E56PHY_PMD_RX_OFFSET * i);
			while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1,
					 rxs0_rx0_adc_gain_adapt_done_o) != 1) {
				rdata = rd32_ephy(hw, addr);
				usleep_range(500, 1000);

				if (timer++ > PHYINIT_TIMEOUT)
					break;
			}

			//f. ALIAS::RXS::ADC_GAIN_ADAPT_EN = 0b0
			addr = E56PHY_RXS0_OVRDVAL_1_ADDR +
			       (E56PHY_PMD_RX_OFFSET * i);
			rdata = rd32_ephy(hw, addr);
			txgbe_field_set(&rdata,
					E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
				0x0);
			txgbe_wr32_ephy(hw, addr, rdata);
		}
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR + (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_ADAPT_EN_I,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);
		//b. Wait for 10ms or greater
		usleep_range(10000, 20000);

		//c. ALIAS::RXS::ADC_INTL_ADAPT_EN = 0b0
		addr = E56G__PMD_RXS0_OVRDEN_2_ADDR +
		       (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_2,
			  ovrd_en_rxs0_rx0_adc_intl_adapt_en_i) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56G__PMD_RXS0_OVRDVAL_1_ADDR +
		       (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1, rxs0_rx0_vga_train_en_i) =
			1;
		if (bypass_ctle == 0) {
			EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1,
				  rxs0_rx0_ctle_train_en_i) = 1;
		}
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56G__PMD_RXS0_OVRDEN_1_ADDR +
		       (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
			  ovrd_en_rxs0_rx0_vga_train_done_o) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);

		rdata = 0;
		timer = 0;
		addr = E56G__PMD_RXS0_OVRDVAL_1_ADDR +
		       (E56PHY_PMD_RX_OFFSET * i);
		while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1,
				 rxs0_rx0_vga_train_done_o) != 1) {
			rdata = rd32_ephy(hw, addr);
			usleep_range(500, 1000);

			if (timer++ > PHYINIT_TIMEOUT)
				break;
		}

		if (bypass_ctle == 0) {
			addr = E56G__PMD_RXS0_OVRDEN_1_ADDR +
			       (E56PHY_PMD_RX_OFFSET * i);
			rdata = rd32_ephy(hw, addr);
			EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
				  ovrd_en_rxs0_rx0_ctle_train_done_o) = 0;
			txgbe_wr32_ephy(hw, addr, rdata);

			rdata = 0;
			timer = 0;
			addr = E56G__PMD_RXS0_OVRDVAL_1_ADDR +
			       (E56PHY_PMD_RX_OFFSET * i);
			while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1,
					 rxs0_rx0_ctle_train_done_o) != 1) {
				rdata = rd32_ephy(hw, addr);
				usleep_range(500, 1000);

				if (timer++ > PHYINIT_TIMEOUT)
					break;
			} //while
		}

		//a. Remove the OVERRIDE on ALIAS::RXS::VGA_TRAIN_EN
		addr = E56G__PMD_RXS0_OVRDEN_1_ADDR +
		       (E56PHY_PMD_RX_OFFSET * i);
		rdata = rd32_ephy(hw, addr);
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
			  ovrd_en_rxs0_rx0_vga_train_en_i) = 0;
		//b. Remove the OVERRIDE on ALIAS::RXS::CTLE_TRAIN_EN
		if (bypass_ctle == 0) {
			EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
				  ovrd_en_rxs0_rx0_ctle_train_en_i) = 0;
		}
		////Remove the OVERRIDE on ALIAS::RXS::FFE_TRAIN_EN
		//printf("Setting RXS0_OVRDEN[1]::ovrd_en_rxs0_rx0_ffe_train_en_i to 0\n");
		//EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_ffe_train_en_i) = 0;
		////Remove the OVERRIDE on ALIAS::RXS::DFE_TRAIN_EN
		//printf("Setting RXS0_OVRDEN[1]::ovrd_en_rxs0_rx0_dfe_train_en_i to 0\n");
		//EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_dfe_train_en_i) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);
	}
	return status;
}

static int txgbe_e56_rxs_calib_adapt_seq(struct txgbe_hw *hw, u32 speed)
{
	int status = 0, i;
	u32 addr, timer;
	u32 rdata = 0x0;
	u32 bypass_ctle = true;

	if (hw->dac_sfp)
		bypass_ctle = false;

	if (hw->mac.type == txgbe_mac_aml) {
		msleep(350);
		rdata = rd32(hw, TXGBE_GPIO_EXT);
		if (rdata & (TXGBE_SFP1_MOD_ABS_LS | TXGBE_SFP1_RX_LOS_LS))
			return TXGBE_ERR_PHY_INIT_NOT_DONE;
	}

	rdata = 0x0000;
	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
			0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata,
			E56PHY_RXS0_OVRDEN_2_OVRD_EN_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
			0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata,
			E56PHY_RXS0_OVRDEN_2_OVRD_EN_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	rdata = 0x0000;
	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_EN_I,
			0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata,
			E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_ADC_INTL_CAL_EN_I, 0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_DONE_O,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata,
			E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_ADC_INTL_CAL_DONE_O,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_ADAPT_EN_I,
			0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_2_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata,
			E56PHY_RXS0_OVRDEN_2_OVRD_EN_RXS0_RX0_ADC_INTL_ADAPT_EN_I,
		  0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	if (bypass_ctle == 1)
		txgbe_e56_ctle_bypass_seq(hw, speed);

	txgbe_e56_rxs_oscinit_temp_track(hw, speed);

	addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
	timer = 0;
	rdata = 0;
	while (EPHY_XFLD(E56G__PMD_CTRL_FSM_RX_STAT_0, ctrl_fsm_rx0_st) !=
	       E56PHY_RX_RDY_ST) {
		rdata = rd32_ephy(hw, addr);
		usleep_range(500, 1000);
		EPHY_RREG(E56G__PMD_CTRL_FSM_RX_STAT_0);
		if (timer++ > PHYINIT_TIMEOUT)
			return TXGBE_ERR_TIMEOUT;
	}

	//RXS ADC adaptation sequence
	//txgbe_e56_rxs_adc_adapt_seq
	rdata = 0;
	timer = 0;
	while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1, rxs0_rx0_cdr_rdy_o) != 1) {
		EPHY_RREG(E56G__PMD_RXS0_OVRDVAL_1);
		usleep_range(500, 1000);
		if (timer++ > PHYINIT_TIMEOUT)
			return TXGBE_ERR_TIMEOUT;
	}

	//4. Disable VGA and CTLE training so that they don't interfere with ADC calibration
	//a. Set ALIAS::RXS::VGA_TRAIN_EN = 0b0
	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_VGA_TRAIN_EN_I, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_VGA_TRAIN_EN_I,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//b. Set ALIAS::RXS::CTLE_TRAIN_EN = 0b0
	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_CTLE_TRAIN_EN_I, 0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_CTLE_TRAIN_EN_I,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	//5. Perform ADC interleaver calibration
	//a. Remove the OVERRIDE on ALIAS::RXS::ADC_INTL_CAL_DONE
	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata,
			E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_ADC_INTL_CAL_DONE_O,
		  0x0);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_EN_I,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	timer = 0;
	while (((rdata >>
		 E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_CAL_DONE_O_LSB) &
		1) != 1) {
		rdata = rd32_ephy(hw, addr);
		usleep_range(1000, 2000);

		if (timer++ > PHYINIT_TIMEOUT)
			break;
	}

	for (i = 0; i < 16; i++) {
		//a. ALIAS::RXS::ADC_OFST_ADAPT_EN = 0b1
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		//b. Wait for 1ms or greater
		txgbe_e56_ephy_config(E56G__PMD_RXS0_OVRDEN_2,
				      ovrd_en_rxs0_rx0_adc_ofst_adapt_done_o,
				      0);
		rdata = 0;
		timer = 0;
		while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1,
				 rxs0_rx0_adc_ofst_adapt_done_o) != 1) {
			EPHY_RREG(E56G__PMD_RXS0_OVRDVAL_1);
			usleep_range(500, 1000);
			if (timer++ > PHYINIT_TIMEOUT)
				break;
		}

		//c. ALIAS::RXS::ADC_OFST_ADAPT_EN = 0b0
		rdata = 0x0000;
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_OFST_ADAPT_EN_I,
			  0x0);
		txgbe_wr32_ephy(hw, addr, rdata);

		//d. ALIAS::RXS::ADC_GAIN_ADAPT_EN = 0b1
		rdata = 0x0000;
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
			  0x1);
		txgbe_wr32_ephy(hw, addr, rdata);

		//e. Wait for 1ms or greater
		txgbe_e56_ephy_config(E56G__PMD_RXS0_OVRDEN_2,
				      ovrd_en_rxs0_rx0_adc_ofst_adapt_done_o,
				      0);
		rdata = 0;
		timer = 0;
		while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1,
				 rxs0_rx0_adc_gain_adapt_done_o) != 1) {
			EPHY_RREG(E56G__PMD_RXS0_OVRDVAL_1);
			usleep_range(500, 1000);

			if (timer++ > PHYINIT_TIMEOUT)
				break;
		}

		//f. ALIAS::RXS::ADC_GAIN_ADAPT_EN = 0b0
		addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
		rdata = rd32_ephy(hw, addr);
		txgbe_field_set(&rdata,
				E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_GAIN_ADAPT_EN_I,
			  0x0);
		txgbe_wr32_ephy(hw, addr, rdata);
	}

	addr = E56PHY_RXS0_OVRDVAL_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDVAL_1_RXS0_RX0_ADC_INTL_ADAPT_EN_I,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);
	//b. Wait for 10ms or greater
	usleep_range(10000, 20000);

	//c. ALIAS::RXS::ADC_INTL_ADAPT_EN = 0b0
	txgbe_e56_ephy_config(E56G__PMD_RXS0_OVRDEN_2,
			      ovrd_en_rxs0_rx0_adc_intl_adapt_en_i, 0);

	addr = E56PHY_RXS0_OVRDEN_1_ADDR;
	rdata = rd32_ephy(hw, addr);
	txgbe_field_set(&rdata, E56PHY_RXS0_OVRDEN_1_OVRD_EN_RXS0_RX0_VGA_TRAIN_EN_I,
			0x1);
	txgbe_wr32_ephy(hw, addr, rdata);

	EPHY_RREG(E56G__PMD_RXS0_OVRDVAL_1);
	EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1, rxs0_rx0_vga_train_en_i) = 1;
	if (bypass_ctle == 0) {
		EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1, rxs0_rx0_ctle_train_en_i) =
			1;
	}
	EPHY_WREG(E56G__PMD_RXS0_OVRDVAL_1);
	txgbe_e56_ephy_config(E56G__PMD_RXS0_OVRDEN_1,
			      ovrd_en_rxs0_rx0_vga_train_done_o, 0);
	rdata = 0;
	timer = 0;
	while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1, rxs0_rx0_vga_train_done_o) != 1) {
		EPHY_RREG(E56G__PMD_RXS0_OVRDVAL_1);
		usleep_range(500, 1000);

		if (timer++ > PHYINIT_TIMEOUT)
			break;
	}

	if (bypass_ctle == 0) {
		txgbe_e56_ephy_config(E56G__PMD_RXS0_OVRDEN_1,
				      ovrd_en_rxs0_rx0_ctle_train_done_o, 0);
		rdata = 0;
		timer = 0;
		while (EPHY_XFLD(E56G__PMD_RXS0_OVRDVAL_1,
				 rxs0_rx0_ctle_train_done_o) != 1) {
			EPHY_RREG(E56G__PMD_RXS0_OVRDVAL_1);
			usleep_range(500, 1000);

			if (timer++ > PHYINIT_TIMEOUT)
				break;
		} //while
	}

	//a. Remove the OVERRIDE on ALIAS::RXS::VGA_TRAIN_EN
	EPHY_RREG(E56G__PMD_RXS0_OVRDEN_1);
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_vga_train_en_i) = 0;
	//b. Remove the OVERRIDE on ALIAS::RXS::CTLE_TRAIN_EN
	if (bypass_ctle == 0) {
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
			  ovrd_en_rxs0_rx0_ctle_train_en_i) = 0;
	}
	////Remove the OVERRIDE on ALIAS::RXS::FFE_TRAIN_EN
	//printf("Setting RXS0_OVRDEN[1]::ovrd_en_rxs0_rx0_ffe_train_en_i to 0\n");
	//EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_ffe_train_en_i) = 0;
	////Remove the OVERRIDE on ALIAS::RXS::DFE_TRAIN_EN
	//printf("Setting RXS0_OVRDEN[1]::ovrd_en_rxs0_rx0_dfe_train_en_i to 0\n");
	//EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_dfe_train_en_i) = 0;
	EPHY_WREG(E56G__PMD_RXS0_OVRDEN_1);

	return status;
}

u32 txgbe_e56_cfg_temp(struct txgbe_hw *hw)
{
	u32 status;
	u32 value;
	int temp;

	status = txgbe_e56_get_temp(hw, &temp);
	if (status)
		temp = DEFAULT_TEMP;

	if (temp < DEFAULT_TEMP) {
		value = rd32_ephy(hw, CMS_ANA_OVRDEN0);
		txgbe_field_set(&value, 25, 25, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN0, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL2);
		txgbe_field_set(&value, 20, 16, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL2, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDEN1);
		txgbe_field_set(&value, 12, 12, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN1, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL7);
		txgbe_field_set(&value, 8, 4, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL7, value);
	} else if (temp > HIGH_TEMP) {
		value = rd32_ephy(hw, CMS_ANA_OVRDEN0);
		txgbe_field_set(&value, 25, 25, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN0, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL2);
		txgbe_field_set(&value, 20, 16, 0x3);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL2, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDEN1);
		txgbe_field_set(&value, 12, 12, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN1, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL7);
		txgbe_field_set(&value, 8, 4, 0x3);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL7, value);
	} else {
		value = rd32_ephy(hw, CMS_ANA_OVRDEN1);
		txgbe_field_set(&value, 4, 4, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN1, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL4);
		txgbe_field_set(&value, 24, 24, 0x1);
		txgbe_field_set(&value, 31, 29, 0x4);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL4, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL5);
		txgbe_field_set(&value, 1, 0, 0x0);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL5, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDEN1);
		txgbe_field_set(&value, 23, 23, 0x1);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDEN1, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL9);
		txgbe_field_set(&value, 24, 24, 0x1);
		txgbe_field_set(&value, 31, 29, 0x4);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL9, value);

		value = rd32_ephy(hw, CMS_ANA_OVRDVAL10);
		txgbe_field_set(&value, 1, 0, 0x0);
		txgbe_wr32_ephy(hw, CMS_ANA_OVRDVAL10, value);
	}

	return 0;
}

int txgbe_e56_config_rx_40G(struct txgbe_hw *hw, u32 speed)
{
	struct txgbe_adapter *adapter = hw->back;
	s32 status;

	status = txgbe_e56_rxs_calib_adapt_seq40(hw, speed);
	if (status)
		return status;

	//Step 2 of 2.3.4
	txgbe_e56_set_rx_ufine_lemax40(hw, speed);

	//2.3.4 RXS post CDR lock temperature tracking sequence
	txgbe_temp_track_seq_40g(hw, speed);

	adapter->link_valid = true;
	return 0;
}

static int txgbe_e56_config_rx(struct txgbe_hw *hw, u32 speed)
{
	s32 status;

	if (speed == TXGBE_LINK_SPEED_40GB_FULL) {
		txgbe_e56_config_rx_40G(hw, speed);
	} else {
		status = txgbe_e56_rxs_calib_adapt_seq(hw, speed);
		if (status)
			return status;

		//Step 2 of 2.3.4
		txgbe_e56_set_rxs_ufine_lemax(hw, speed);

		//2.3.4 RXS post CDR lock temperature tracking sequence
		txgbe_temp_track_seq(hw, speed);
	}
	return 0;
}

//--------------------------------------------------------------
//2.2.10 SEQ::RX_DISABLE
//Use PDIG::PMD_CFG[0]::rx_en_cfg[<lane no.>] = 0b0 to powerdown specific RXS lanes.
//Completion of RXS powerdown can be confirmed by observing ALIAS::PDIG::CTRL_FSM_RX_ST = POWERDN_ST
//--------------------------------------------------------------
static int txgbe_e56_disable_rx40G(struct txgbe_hw *hw)
{
	int status = 0;
	unsigned int rdata, timer;
	unsigned int addr, temp;
	int i;
	struct txgbe_adapter *adapter;

	for (i = 0; i < 4; i++) {
		//1. Disable OVERRIDE on below aliases
		//a. ALIAS::RXS::RANGE_SEL
		rdata = 0x0000;
		addr = E56G__RXS0_ANA_OVRDEN_0_ADDR + (i * E56PHY_RXS_OFFSET);
		rdata = rd32_ephy(hw, addr);
		EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_0,
			  ovrd_en_ana_bbcdr_osc_range_sel_i) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56G__RXS0_ANA_OVRDEN_1_ADDR + (i * E56PHY_RXS_OFFSET);
		rdata = rd32_ephy(hw, addr);
		//b. ALIAS::RXS::COARSE
		EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1, ovrd_en_ana_bbcdr_coarse_i) =
			0;
		//c. ALIAS::RXS::FINE
		EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1, ovrd_en_ana_bbcdr_fine_i) =
			0;
		//d. ALIAS::RXS::ULTRAFINE
		EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1,
			  ovrd_en_ana_bbcdr_ultrafine_i) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);

		//e. ALIAS::RXS::SAMP_CAL_DONE
		addr = E56G__PMD_RXS0_OVRDEN_0_ADDR +
		       (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_0,
			  ovrd_en_rxs0_rx0_samp_cal_done_o) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56G__PMD_RXS0_OVRDEN_2_ADDR +
		       (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		//f. ALIAS::RXS::ADC_OFST_ADAPT_EN
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_2,
			  ovrd_en_rxs0_rx0_adc_ofst_adapt_en_i) = 0;
		//g. ALIAS::RXS::ADC_GAIN_ADAPT_EN
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_2,
			  ovrd_en_rxs0_rx0_adc_gain_adapt_en_i) = 0;
		//j. ALIAS::RXS::ADC_INTL_ADAPT_EN
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_2,
			  ovrd_en_rxs0_rx0_adc_intl_adapt_en_i) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56G__PMD_RXS0_OVRDEN_1_ADDR +
		       (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		//h. ALIAS::RXS::ADC_INTL_CAL_EN
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
			  ovrd_en_rxs0_rx0_adc_intl_cal_en_i) = 0;
		//i. ALIAS::RXS::ADC_INTL_CAL_DONE
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
			  ovrd_en_rxs0_rx0_adc_intl_cal_done_o) = 0;
		//k. ALIAS::RXS::CDR_EN
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_cdr_en_i) =
			0;
		//l. ALIAS::RXS::VGA_TRAIN_EN
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
			  ovrd_en_rxs0_rx0_vga_train_en_i) = 0;
		//m. ALIAS::RXS::CTLE_TRAIN_EN
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
			  ovrd_en_rxs0_rx0_ctle_train_en_i) = 0;
		//p. ALIAS::RXS::RX_FETX_TRAIN_DONE
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
			  ovrd_en_rxs0_rx0_txffe_train_done_o) = 0;
		//r. ALIAS::RXS::RX_TXFFE_COEFF_CHANGE
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
			  ovrd_en_rxs0_rx0_txffe_coeff_change_o) = 0;
		//s. ALIAS::RXS::RX_TXFFE_TRAIN_ENACK
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
			  ovrd_en_rxs0_rx0_txffe_train_enack_o) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56G__PMD_RXS0_OVRDEN_3_ADDR +
		       (i * E56PHY_PMD_RX_OFFSET);
		rdata = rd32_ephy(hw, addr);
		//n. ALIAS::RXS::RX_FETX_MOD_TYPE
		//o. ALIAS::RXS::RX_FETX_MOD_TYPE_UPDATE
		temp = EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_3,
				 ovrd_en_rxs0_rx0_spareout_o);
		EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_3,
			  ovrd_en_rxs0_rx0_spareout_o) = temp & 0x8F;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56G__RXS0_DIG_OVRDEN_1_ADDR + (i * E56PHY_RXS_OFFSET);
		rdata = rd32_ephy(hw, addr);
		//q. ALIAS::RXS::SLICER_THRESHOLD_OVRD_EN
		EPHY_XFLD(E56G__RXS0_DIG_OVRDEN_1, top_comp_th_ovrd_en) = 0;
		EPHY_XFLD(E56G__RXS0_DIG_OVRDEN_1, mid_comp_th_ovrd_en) = 0;
		EPHY_XFLD(E56G__RXS0_DIG_OVRDEN_1, bot_comp_th_ovrd_en) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);

		//2. Disable pattern checker �C
		addr = E56G__RXS0_DFT_1_ADDR + (i * E56PHY_RXS_OFFSET);
		rdata = rd32_ephy(hw, addr);
		EPHY_XFLD(E56G__RXS0_DFT_1, ber_en) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);

		//3. Disable internal serial loopback mode �C
		addr = E56G__RXS0_ANA_OVRDEN_3_ADDR + (i * E56PHY_RXS_OFFSET);
		rdata = rd32_ephy(hw, addr);
		EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_3, ovrd_en_ana_sel_lpbk_i) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);

		addr = E56G__RXS0_ANA_OVRDEN_2_ADDR + (i * E56PHY_RXS_OFFSET);
		rdata = rd32_ephy(hw, addr);
		(((E56G__RXS0_ANA_OVRDEN_2 *)&rdata)->ovrd_en_ana_en_adccal_lpbk_i) = 0;
		txgbe_wr32_ephy(hw, addr, rdata);

		//4. Enable bypass of clock gates in RXS -
		addr = E56G__RXS0_RXS_CFG_0_ADDR + (i * E56PHY_RXS_OFFSET);
		rdata = rd32_ephy(hw, addr);
		EPHY_XFLD(E56G__RXS0_RXS_CFG_0, train_clk_gate_bypass_en) =
			0x1FFF;
		txgbe_wr32_ephy(hw, addr, rdata);
	}

	//5. Disable KR training mode �C
	//a. ALIAS::PDIG::KR_TRAINING_MODE = 0b0
	addr = E56G__PMD_BASER_PMD_CONTROL_ADDR;
	rdata = rd32_ephy(hw, addr);
	EPHY_XFLD(E56G__PMD_BASER_PMD_CONTROL, training_enable_ln0) = 0;
	EPHY_XFLD(E56G__PMD_BASER_PMD_CONTROL, training_enable_ln1) = 0;
	EPHY_XFLD(E56G__PMD_BASER_PMD_CONTROL, training_enable_ln2) = 0;
	EPHY_XFLD(E56G__PMD_BASER_PMD_CONTROL, training_enable_ln3) = 0;
	txgbe_wr32_ephy(hw, addr, rdata);

	//6. Disable RX to TX parallel loopback �C
	//a. ALIAS::PDIG::RX_TO_TX_LPBK_EN = 0b0
	addr = E56G__PMD_PMD_CFG_5_ADDR;
	rdata = rd32_ephy(hw, addr);
	EPHY_XFLD(E56G__PMD_PMD_CFG_5, rx_to_tx_lpbk_en) = 0x0;
	txgbe_wr32_ephy(hw, addr, rdata);

	//The FSM to disable RXS is present in PDIG. The FSM disables the RXS when �C
	//PDIG::PMD_CFG[0]::rx_en_cfg[<lane no.>] = 0b0
	txgbe_e56_ephy_config(E56G__PMD_PMD_CFG_0, rx_en_cfg, 0);

	//Wait RX FSM to be POWERDN_ST
	timer = 0;

	while (EPHY_XFLD(E56G__PMD_CTRL_FSM_RX_STAT_0, ctrl_fsm_rx0_st) !=
		       0x21 ||
	       EPHY_XFLD(E56G__PMD_CTRL_FSM_RX_STAT_0, ctrl_fsm_rx1_st) !=
		       0x21 ||
	       EPHY_XFLD(E56G__PMD_CTRL_FSM_RX_STAT_0, ctrl_fsm_rx2_st) !=
		       0x21 ||
	       EPHY_XFLD(E56G__PMD_CTRL_FSM_RX_STAT_0, ctrl_fsm_rx3_st) !=
		       0x21) {
		rdata = 0;
		addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		usleep_range(100, 200);
		if (timer++ > PHYINIT_TIMEOUT) {
			e_info(drv, "ERROR: Wait E56PHY_CTRL_FSM_RX_STAT_0_ADDR Timeout!!!\n");
			break;
		}
	}

	return status;
}

//--------------------------------------------------------------
//2.2.10 SEQ::RX_DISABLE
//Use PDIG::PMD_CFG[0]::rx_en_cfg[<lane no.>] = 0b0 to powerdown specific RXS lanes.
//Completion of RXS powerdown can be confirmed by observing ALIAS::PDIG::CTRL_FSM_RX_ST = POWERDN_ST
//--------------------------------------------------------------
static int txgbe_e56_disable_rx(struct txgbe_hw *hw)
{
	int status = 0;
	unsigned int rdata, timer;
	unsigned int addr, temp;
	struct txgbe_adapter *adapter = hw->back;

	//1. Disable OVERRIDE on below aliases
	//a. ALIAS::RXS::RANGE_SEL
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_0,
			      ovrd_en_ana_bbcdr_osc_range_sel_i, 0);

	EPHY_RREG(E56G__RXS0_ANA_OVRDEN_1);
	//b. ALIAS::RXS::COARSE
	EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1, ovrd_en_ana_bbcdr_coarse_i) = 0;
	//c. ALIAS::RXS::FINE
	EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1, ovrd_en_ana_bbcdr_fine_i) = 0;
	//d. ALIAS::RXS::ULTRAFINE
	EPHY_XFLD(E56G__RXS0_ANA_OVRDEN_1, ovrd_en_ana_bbcdr_ultrafine_i) = 0;
	EPHY_WREG(E56G__RXS0_ANA_OVRDEN_1);

	//e. ALIAS::RXS::SAMP_CAL_DONE
	txgbe_e56_ephy_config(E56G__PMD_RXS0_OVRDEN_0,
			      ovrd_en_rxs0_rx0_samp_cal_done_o, 0);

	EPHY_RREG(E56G__PMD_RXS0_OVRDEN_2);
	//f. ALIAS::RXS::ADC_OFST_ADAPT_EN
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_2,
		  ovrd_en_rxs0_rx0_adc_ofst_adapt_en_i) = 0;
	//g. ALIAS::RXS::ADC_GAIN_ADAPT_EN
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_2,
		  ovrd_en_rxs0_rx0_adc_gain_adapt_en_i) = 0;
	//j. ALIAS::RXS::ADC_INTL_ADAPT_EN
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_2,
		  ovrd_en_rxs0_rx0_adc_intl_adapt_en_i) = 0;
	EPHY_WREG(E56G__PMD_RXS0_OVRDEN_2);

	EPHY_RREG(E56G__PMD_RXS0_OVRDEN_1);
	//h. ALIAS::RXS::ADC_INTL_CAL_EN
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_adc_intl_cal_en_i) =
		0;
	//i. ALIAS::RXS::ADC_INTL_CAL_DONE
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
		  ovrd_en_rxs0_rx0_adc_intl_cal_done_o) = 0;
	//k. ALIAS::RXS::CDR_EN
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_cdr_en_i) = 0;
	//l. ALIAS::RXS::VGA_TRAIN_EN
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_vga_train_en_i) = 0;
	//m. ALIAS::RXS::CTLE_TRAIN_EN
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1, ovrd_en_rxs0_rx0_ctle_train_en_i) =
		0;
	//p. ALIAS::RXS::RX_FETX_TRAIN_DONE
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
		  ovrd_en_rxs0_rx0_txffe_train_done_o) = 0;
	//r. ALIAS::RXS::RX_TXFFE_COEFF_CHANGE
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
		  ovrd_en_rxs0_rx0_txffe_coeff_change_o) = 0;
	//s. ALIAS::RXS::RX_TXFFE_TRAIN_ENACK
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_1,
		  ovrd_en_rxs0_rx0_txffe_train_enack_o) = 0;
	EPHY_WREG(E56G__PMD_RXS0_OVRDEN_1);

	EPHY_RREG(E56G__PMD_RXS0_OVRDEN_3);
	//n. ALIAS::RXS::RX_FETX_MOD_TYPE
	//o. ALIAS::RXS::RX_FETX_MOD_TYPE_UPDATE
	temp = EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_3, ovrd_en_rxs0_rx0_spareout_o);
	EPHY_XFLD(E56G__PMD_RXS0_OVRDEN_3, ovrd_en_rxs0_rx0_spareout_o) = temp &
									  0x8F;
	EPHY_WREG(E56G__PMD_RXS0_OVRDEN_3);

	//q. ALIAS::RXS::SLICER_THRESHOLD_OVRD_EN
	EPHY_RREG(E56G__RXS0_DIG_OVRDEN_1);
	EPHY_XFLD(E56G__RXS0_DIG_OVRDEN_1, top_comp_th_ovrd_en) = 0;
	EPHY_XFLD(E56G__RXS0_DIG_OVRDEN_1, mid_comp_th_ovrd_en) = 0;
	EPHY_XFLD(E56G__RXS0_DIG_OVRDEN_1, bot_comp_th_ovrd_en) = 0;
	EPHY_WREG(E56G__RXS0_DIG_OVRDEN_1);

	//2. Disable pattern checker �C
	txgbe_e56_ephy_config(E56G__RXS0_DFT_1, ber_en, 0);

	//3. Disable internal serial loopback mode �C
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_3, ovrd_en_ana_sel_lpbk_i,
			      0);
	txgbe_e56_ephy_config(E56G__RXS0_ANA_OVRDEN_2,
			      ovrd_en_ana_en_adccal_lpbk_i, 0);

	//4. Enable bypass of clock gates in RXS -
	txgbe_e56_ephy_config(E56G__RXS0_RXS_CFG_0, train_clk_gate_bypass_en,
			      0x1FFF);

	//5. Disable KR training mode �C
	//a. ALIAS::PDIG::KR_TRAINING_MODE = 0b0
	txgbe_e56_ephy_config(E56G__PMD_BASER_PMD_CONTROL, training_enable_ln0,
			      0);

	//6. Disable RX to TX parallel loopback �C
	//a. ALIAS::PDIG::RX_TO_TX_LPBK_EN = 0b0
	txgbe_e56_ephy_config(E56G__PMD_PMD_CFG_5, rx_to_tx_lpbk_en, 0);

	//The FSM to disable RXS is present in PDIG. The FSM disables the RXS when �C
	//PDIG::PMD_CFG[0]::rx_en_cfg[<lane no.>] = 0b0
	txgbe_e56_ephy_config(E56G__PMD_PMD_CFG_0, rx_en_cfg, 0);

	//Wait RX FSM to be POWERDN_ST
	timer = 0;
	while (1) {
		rdata = 0;
		addr = E56PHY_CTRL_FSM_RX_STAT_0_ADDR;
		rdata = rd32_ephy(hw, addr);
		if ((rdata & 0x3f) == 0x21)
			break;

		usleep_range(100, 200);
		if (timer++ > PHYINIT_TIMEOUT) {
			e_info(drv, "ERROR: Wait E56PHY_CTRL_FSM_RX_STAT_0_ADDR Timeout!!!\n");
			break;
		}
	}

	return status;
}

int txgbe_e56_reconfig_rx(struct txgbe_hw *hw, u32 speed)
{
	int status = 0;
	u32 rdata;
	u32 addr;

	wr32m(hw, TXGBE_MAC_TX_CFG, TXGBE_MAC_TX_CFG_TE, ~TXGBE_MAC_TX_CFG_TE);
	wr32m(hw, TXGBE_MAC_RX_CFG, TXGBE_MAC_RX_CFG_RE, ~TXGBE_MAC_RX_CFG_RE);

	if (hw->mac.type == txgbe_mac_aml) {
		rdata = rd32(hw, TXGBE_GPIO_EXT);
		if (rdata & (TXGBE_SFP1_MOD_ABS_LS | TXGBE_SFP1_RX_LOS_LS))
			return TXGBE_ERR_TIMEOUT;
	}

	txgbe_wr32_ephy(hw, E56PHY_INTR_0_ENABLE_ADDR, 0x0);
	txgbe_wr32_ephy(hw, E56PHY_INTR_1_ENABLE_ADDR, 0x0);

	if (hw->mac.type == txgbe_mac_aml40) {
		//14. Do SEQ::RX_DISABLE to disable RXS. Poll ALIAS::PDIG::CTRL_FSM_RX_ST
		//and confirm its value is POWERDN_ST
		txgbe_e56_disable_rx40G(hw);
		status = txgbe_e56_config_rx_40G(hw, speed);
	} else {
		//14. Do SEQ::RX_DISABLE to disable RXS. Poll ALIAS::PDIG::CTRL_FSM_RX_ST
		//and confirm its value is POWERDN_ST
		txgbe_e56_disable_rx(hw);
		status = txgbe_e56_config_rx(hw, speed);
	}

	addr = E56PHY_INTR_0_ADDR;
	txgbe_wr32_ephy(hw, addr, E56PHY_INTR_0_IDLE_ENTRY1);

	addr = E56PHY_INTR_1_ADDR;
	txgbe_wr32_ephy(hw, addr, E56PHY_INTR_1_IDLE_EXIT1);

	txgbe_wr32_ephy(hw, E56PHY_INTR_0_ENABLE_ADDR,
			E56PHY_INTR_0_IDLE_ENTRY1);
	txgbe_wr32_ephy(hw, E56PHY_INTR_1_ENABLE_ADDR,
			E56PHY_INTR_1_IDLE_EXIT1);

	hw->mac.ops.enable_sec_tx_path(hw);

	return status;
}

//Reference setting code for SFP mode
int txgbe_set_link_to_amlite(struct txgbe_hw *hw, u32 speed)
{
	struct txgbe_adapter *adapter = hw->back;
	u32 value = 0;
	u32 ppl_lock = false;
	int status = 0;
	u32 reset = 0;

	if ((rd32(hw, TXGBE_EPHY_STAT) & TXGBE_EPHY_STAT_PPL_LOCK) ==
	    TXGBE_EPHY_STAT_PPL_LOCK) {
		ppl_lock = true;
		wr32m(hw, TXGBE_MAC_TX_CFG, TXGBE_MAC_TX_CFG_TE,
		      ~TXGBE_MAC_TX_CFG_TE);
		wr32m(hw, TXGBE_MAC_RX_CFG, TXGBE_MAC_RX_CFG_RE,
		      ~TXGBE_MAC_RX_CFG_RE);
		hw->mac.ops.disable_sec_tx_path(hw);
	}

	hw->mac.ops.disable_tx_laser(hw);

	if (hw->bus.lan_id == 0)
		reset = TXGBE_MIS_RST_LAN0_EPHY_RST;
	else
		reset = TXGBE_MIS_RST_LAN1_EPHY_RST;

	wr32(hw, TXGBE_MIS_RST, reset | rd32(hw, TXGBE_MIS_RST));
	TXGBE_WRITE_FLUSH(hw);
	usec_delay(10);

	/////////////////////////// XLGPCS REGS Start
	value = txgbe_rd32_epcs(hw, VR_PCS_DIG_CTRL1);
	value |= 0x8000;
	txgbe_wr32_epcs(hw, VR_PCS_DIG_CTRL1, value);

	usleep_range(1000, 2000);
	value = txgbe_rd32_epcs(hw, VR_PCS_DIG_CTRL1);
	if ((value & 0x8000)) {
		status = TXGBE_ERR_PHY_INIT_NOT_DONE;
		hw->mac.ops.enable_tx_laser(hw);
		goto out;
	}

	txgbe_e56_ovrd_symdata(hw);

	value = txgbe_rd32_epcs(hw, SR_AN_CTRL);
	txgbe_field_set(&value, 12, 12, 0);
	txgbe_wr32_epcs(hw, SR_AN_CTRL, value);

	if (speed == TXGBE_LINK_SPEED_40GB_FULL) {
		value = txgbe_rd32_epcs(hw, SR_PCS_CTRL1);
		txgbe_field_set(&value, 5, 2, 0x3);
		txgbe_wr32_epcs(hw, SR_PCS_CTRL1, value);

		value = txgbe_rd32_epcs(hw, SR_PCS_CTRL2);
		txgbe_field_set(&value, 3, 0, 0x4);
		txgbe_wr32_epcs(hw, SR_PCS_CTRL2, value);

		value = rd32_ephy(hw, ANA_OVRDVAL0);
		txgbe_field_set(&value, 29, 29, 0x1);
		txgbe_field_set(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDVAL0, value);

		value = rd32_ephy(hw, ANA_OVRDVAL5);
		txgbe_field_set(&value, 24, 24, 0x0);
		txgbe_wr32_ephy(hw, ANA_OVRDVAL5, value);

		value = rd32_ephy(hw, ANA_OVRDEN0);
		txgbe_field_set(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDEN0, value);

		value = rd32_ephy(hw, ANA_OVRDEN1);
		txgbe_field_set(&value, 30, 30, 0x1);
		txgbe_field_set(&value, 25, 25, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDEN1, value);

		value = rd32_ephy(hw, PLL0_CFG0);
		txgbe_field_set(&value, 25, 24, 0x1);
		txgbe_field_set(&value, 17, 16, 0x3);
		txgbe_wr32_ephy(hw, PLL0_CFG0, value);

		value = rd32_ephy(hw, PLL0_CFG2);
		txgbe_field_set(&value, 12, 8, 0x4);
		txgbe_wr32_ephy(hw, PLL0_CFG2, value);

		value = rd32_ephy(hw, PLL1_CFG0);
		txgbe_field_set(&value, 25, 24, 0x1);
		txgbe_field_set(&value, 17, 16, 0x3);
		txgbe_wr32_ephy(hw, PLL1_CFG0, value);

		value = rd32_ephy(hw, PLL1_CFG2);
		txgbe_field_set(&value, 12, 8, 0x8);
		txgbe_wr32_ephy(hw, PLL1_CFG2, value);

		value = rd32_ephy(hw, PLL0_DIV_CFG0);
		txgbe_field_set(&value, 18, 8, 0x294);
		txgbe_field_set(&value, 4, 0, 0x8);
		txgbe_wr32_ephy(hw, PLL0_DIV_CFG0, value);

		value = rd32_ephy(hw, DATAPATH_CFG0);
		txgbe_field_set(&value, 30, 28, 0x7);
		txgbe_field_set(&value, 26, 24, 0x5);
		txgbe_field_set(&value, 18, 16, 0x5);
		txgbe_field_set(&value, 14, 12, 0x5);
		txgbe_field_set(&value, 10, 8, 0x5);
		txgbe_wr32_ephy(hw, DATAPATH_CFG0, value);

		value = rd32_ephy(hw, DATAPATH_CFG1);
		txgbe_field_set(&value, 26, 24, 0x5);
		txgbe_field_set(&value, 10, 8, 0x5);
		txgbe_field_set(&value, 18, 16, 0x5);
		txgbe_field_set(&value, 2, 0, 0x5);
		txgbe_wr32_ephy(hw, DATAPATH_CFG1, value);

		value = rd32_ephy(hw, AN_CFG1);
		txgbe_field_set(&value, 4, 0, 0x2);
		txgbe_wr32_ephy(hw, AN_CFG1, value);

		txgbe_e56_cfg_temp(hw);
		txgbe_e56_cfg_40g(hw);

		value = rd32_ephy(hw, PMD_CFG0);
		txgbe_field_set(&value, 21, 20, 0x3);
		txgbe_field_set(&value, 19, 12, 0xf); //TX_EN set
		txgbe_field_set(&value, 8, 8, 0x0);
		txgbe_field_set(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, PMD_CFG0, value);
	}

	if (speed == TXGBE_LINK_SPEED_25GB_FULL) {
		value = txgbe_rd32_epcs(hw, SR_PCS_CTRL1);
		txgbe_field_set(&value, 5, 2, 5);
		txgbe_wr32_epcs(hw, SR_PCS_CTRL1, value);

		value = txgbe_rd32_epcs(hw, SR_PCS_CTRL2);
		txgbe_field_set(&value, 3, 0, 7);
		txgbe_wr32_epcs(hw, SR_PCS_CTRL2, value);

		value = txgbe_rd32_epcs(hw, SR_PMA_CTRL2);
		txgbe_field_set(&value, 6, 0, 0x39);
		txgbe_wr32_epcs(hw, SR_PMA_CTRL2, value);

		value = rd32_ephy(hw, ANA_OVRDVAL0);
		txgbe_field_set(&value, 29, 29, 0x1);
		txgbe_field_set(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDVAL0, value);

		value = rd32_ephy(hw, ANA_OVRDVAL5);
		//Update to 0 from SNPS for PIN CLKP/N: Enable the termination of the input buffer
		txgbe_field_set(&value, 24, 24, 0x0);
		txgbe_wr32_ephy(hw, ANA_OVRDVAL5, value);

		value = rd32_ephy(hw, ANA_OVRDEN0);
		txgbe_field_set(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDEN0, value);

		value = rd32_ephy(hw, ANA_OVRDEN1);
		txgbe_field_set(&value, 30, 30, 0x1);
		txgbe_field_set(&value, 25, 25, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDEN1, value);

		value = rd32_ephy(hw, PLL0_CFG0);
		txgbe_field_set(&value, 25, 24, 0x1);
		txgbe_field_set(&value, 17, 16, 0x3);
		txgbe_wr32_ephy(hw, PLL0_CFG0, value);

		value = rd32_ephy(hw, PLL0_CFG2);
		txgbe_field_set(&value, 12, 8, 0x4);
		txgbe_wr32_ephy(hw, PLL0_CFG2, value);

		value = rd32_ephy(hw, PLL1_CFG0);
		txgbe_field_set(&value, 25, 24, 0x1);
		txgbe_field_set(&value, 17, 16, 0x3);
		txgbe_wr32_ephy(hw, PLL1_CFG0, value);

		value = rd32_ephy(hw, PLL1_CFG2);
		txgbe_field_set(&value, 12, 8, 0x8);
		txgbe_wr32_ephy(hw, PLL1_CFG2, value);

		value = rd32_ephy(hw, PLL0_DIV_CFG0);
		txgbe_field_set(&value, 18, 8, 0x294);
		txgbe_field_set(&value, 4, 0, 0x8);
		txgbe_wr32_ephy(hw, PLL0_DIV_CFG0, value);

		value = rd32_ephy(hw, DATAPATH_CFG0);
		txgbe_field_set(&value, 30, 28, 0x7);
		txgbe_field_set(&value, 26, 24, 0x5);
		txgbe_field_set(&value, 18, 16, 0x3);
		txgbe_field_set(&value, 14, 12, 0x5);
		txgbe_field_set(&value, 10, 8, 0x5);
		txgbe_wr32_ephy(hw, DATAPATH_CFG0, value);

		value = rd32_ephy(hw, DATAPATH_CFG1);
		txgbe_field_set(&value, 26, 24, 0x5);
		txgbe_field_set(&value, 10, 8, 0x5);
		txgbe_field_set(&value, 18, 16, 0x3);
		txgbe_field_set(&value, 2, 0, 0x3);
		txgbe_wr32_ephy(hw, DATAPATH_CFG1, value);

		value = rd32_ephy(hw, AN_CFG1);
		txgbe_field_set(&value, 4, 0, 0x9);
		txgbe_wr32_ephy(hw, AN_CFG1, value);

		txgbe_e56_cfg_temp(hw);
		txgbe_e56_cfg_25g(hw);

		value = rd32_ephy(hw, PMD_CFG0);
		txgbe_field_set(&value, 21, 20, 0x3);
		txgbe_field_set(&value, 19, 12, 0x1); //TX_EN set
		txgbe_field_set(&value, 8, 8, 0x0);
		txgbe_field_set(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, PMD_CFG0, value);
	}

	if (speed == TXGBE_LINK_SPEED_10GB_FULL) {
		value = txgbe_rd32_epcs(hw, SR_PCS_CTRL1);
		txgbe_field_set(&value, 5, 2, 0);
		txgbe_wr32_epcs(hw, SR_PCS_CTRL1, value);

		value = txgbe_rd32_epcs(hw, SR_PCS_CTRL2);
		txgbe_field_set(&value, 3, 0, 0);
		txgbe_wr32_epcs(hw, SR_PCS_CTRL2, value);

		value = txgbe_rd32_epcs(hw, SR_PMA_CTRL2);
		txgbe_field_set(&value, 6, 0, 0xb);
		txgbe_wr32_epcs(hw, SR_PMA_CTRL2, value);

		value = rd32_ephy(hw, ANA_OVRDVAL0);
		txgbe_field_set(&value, 29, 29, 0x1);
		txgbe_field_set(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDVAL0, value);

		value = rd32_ephy(hw, ANA_OVRDVAL5);
		txgbe_field_set(&value, 24, 24, 0x0);
		txgbe_wr32_ephy(hw, ANA_OVRDVAL5, value);

		value = rd32_ephy(hw, ANA_OVRDEN0);
		txgbe_field_set(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDEN0, value);

		value = rd32_ephy(hw, ANA_OVRDEN1);
		txgbe_field_set(&value, 30, 30, 0x1);
		txgbe_field_set(&value, 25, 25, 0x1);
		txgbe_wr32_ephy(hw, ANA_OVRDEN1, value);

		value = rd32_ephy(hw, PLL0_CFG0);
		txgbe_field_set(&value, 25, 24, 0x1);
		txgbe_field_set(&value, 17, 16, 0x3);
		txgbe_wr32_ephy(hw, PLL0_CFG0, value);

		value = rd32_ephy(hw, PLL0_CFG2);
		txgbe_field_set(&value, 12, 8, 0x4);
		txgbe_wr32_ephy(hw, PLL0_CFG2, value);

		value = rd32_ephy(hw, PLL1_CFG0);
		txgbe_field_set(&value, 25, 24, 0x1);
		txgbe_field_set(&value, 17, 16, 0x3);
		txgbe_wr32_ephy(hw, PLL1_CFG0, value);

		value = rd32_ephy(hw, PLL1_CFG2);
		txgbe_field_set(&value, 12, 8, 0x8);
		txgbe_wr32_ephy(hw, PLL1_CFG2, value);

		value = rd32_ephy(hw, PLL0_DIV_CFG0);
		txgbe_field_set(&value, 18, 8, 0x294);
		txgbe_field_set(&value, 4, 0, 0x8);
		txgbe_wr32_ephy(hw, PLL0_DIV_CFG0, value);

		value = rd32_ephy(hw, DATAPATH_CFG0);
		txgbe_field_set(&value, 30, 28, 0x7);
		txgbe_field_set(&value, 26, 24, 0x5);
		txgbe_field_set(&value, 18, 16, 0x5);
		txgbe_field_set(&value, 14, 12, 0x5);
		txgbe_field_set(&value, 10, 8, 0x5);
		txgbe_wr32_ephy(hw, DATAPATH_CFG0, value);

		value = rd32_ephy(hw, DATAPATH_CFG1);
		txgbe_field_set(&value, 26, 24, 0x5);
		txgbe_field_set(&value, 10, 8, 0x5);
		txgbe_field_set(&value, 18, 16, 0x5);
		txgbe_field_set(&value, 2, 0, 0x5);
		txgbe_wr32_ephy(hw, DATAPATH_CFG1, value);

		value = rd32_ephy(hw, AN_CFG1);
		txgbe_field_set(&value, 4, 0, 0x2);
		txgbe_wr32_ephy(hw, AN_CFG1, value);

		txgbe_e56_cfg_temp(hw);
		txgbe_e56_cfg_10g(hw);

		value = rd32_ephy(hw, PMD_CFG0);
		txgbe_field_set(&value, 21, 20, 0x3);
		txgbe_field_set(&value, 19, 12, 0x1); //TX_EN set
		txgbe_field_set(&value, 8, 8, 0x0);
		txgbe_field_set(&value, 1, 1, 0x1);
		txgbe_wr32_ephy(hw, PMD_CFG0, value);
	}

	txgbe_e56_clear_symdata(hw);

	if (adapter->fec_link_mode != TXGBE_PHY_FEC_AUTO &&
	    speed == TXGBE_LINK_SPEED_25GB_FULL) {
		adapter->cur_fec_link = adapter->fec_link_mode;
		txgbe_e56_set_fec_mode(hw, adapter->cur_fec_link);
	}

	hw->mac.ops.enable_tx_laser(hw);

	status = txgbe_e56_config_rx(hw, speed);

	value = rd32_ephy(hw, E56PHY_RXS_IDLE_DETECT_1_ADDR);
	txgbe_field_set(&value, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MAX, 0x28);
	txgbe_field_set(&value, E56PHY_RXS_IDLE_DETECT_1_IDLE_TH_ADC_PEAK_MIN, 0xa);
	txgbe_wr32_ephy(hw, E56PHY_RXS_IDLE_DETECT_1_ADDR, value);

	txgbe_wr32_ephy(hw, E56PHY_INTR_0_ADDR, E56PHY_INTR_0_IDLE_ENTRY1);
	txgbe_wr32_ephy(hw, E56PHY_INTR_1_ADDR, E56PHY_INTR_1_IDLE_EXIT1);
	txgbe_wr32_ephy(hw, E56PHY_INTR_0_ENABLE_ADDR,
			E56PHY_INTR_0_IDLE_ENTRY1);
	txgbe_wr32_ephy(hw, E56PHY_INTR_1_ENABLE_ADDR,
			E56PHY_INTR_1_IDLE_EXIT1);

	if (status)
		goto out;

out:
	if (ppl_lock)
		hw->mac.ops.enable_sec_tx_path(hw);

	return status;
}

int txgbe_get_cur_fec_mode(struct txgbe_hw *hw)
{
	struct txgbe_adapter *adapter = hw->back;
	u32 value = 0;

	mutex_lock(&adapter->e56_lock);
	value = txgbe_rd32_epcs(hw, SR_PMA_RS_FEC_CTRL);
	mutex_unlock(&adapter->e56_lock);

	if (value & 0x4)
		return TXGBE_PHY_FEC_RS;

	mutex_lock(&adapter->e56_lock);
	value = txgbe_rd32_epcs(hw, SR_PMA_KR_FEC_CTRL);
	mutex_unlock(&adapter->e56_lock);

	if (value & 0x1)
		return TXGBE_PHY_FEC_BASER;

	return TXGBE_PHY_FEC_OFF;
}

int txgbe_e56_set_fec_mode(struct txgbe_hw *hw, u8 fec_mode)
{
	u32 value = 0;

	if (fec_mode & TXGBE_PHY_FEC_RS) {
		//disable BASER FEC
		value = txgbe_rd32_epcs(hw, SR_PMA_KR_FEC_CTRL);
		txgbe_field_set(&value, 0, 0, 0);
		txgbe_wr32_epcs(hw, SR_PMA_KR_FEC_CTRL, value);

		//enable RS FEC
		txgbe_wr32_epcs(hw, 0x180a3, 0x68c1);
		txgbe_wr32_epcs(hw, 0x180a4, 0x3321);
		txgbe_wr32_epcs(hw, 0x180a5, 0x973e);
		txgbe_wr32_epcs(hw, 0x180a6, 0xccde);

		txgbe_wr32_epcs(hw, 0x38018, 1024);
		value = txgbe_rd32_epcs(hw, 0x100c8);
		txgbe_field_set(&value, 2, 2, 1);
		txgbe_wr32_epcs(hw, 0x100c8, value);
	} else if (fec_mode & TXGBE_PHY_FEC_BASER) {
		//disable RS FEC
		txgbe_wr32_epcs(hw, 0x180a3, 0x7690);
		txgbe_wr32_epcs(hw, 0x180a4, 0x3347);
		txgbe_wr32_epcs(hw, 0x180a5, 0x896f);
		txgbe_wr32_epcs(hw, 0x180a6, 0xccb8);
		txgbe_wr32_epcs(hw, 0x38018, 0x3fff);
		value = txgbe_rd32_epcs(hw, 0x100c8);
		txgbe_field_set(&value, 2, 2, 0);
		txgbe_wr32_epcs(hw, 0x100c8, value);

		//enable BASER FEC
		value = txgbe_rd32_epcs(hw, SR_PMA_KR_FEC_CTRL);
		txgbe_field_set(&value, 0, 0, 1);
		txgbe_wr32_epcs(hw, SR_PMA_KR_FEC_CTRL, value);
	} else {
		//disable RS FEC
		txgbe_wr32_epcs(hw, 0x180a3, 0x7690);
		txgbe_wr32_epcs(hw, 0x180a4, 0x3347);
		txgbe_wr32_epcs(hw, 0x180a5, 0x896f);
		txgbe_wr32_epcs(hw, 0x180a6, 0xccb8);
		txgbe_wr32_epcs(hw, 0x38018, 0x3fff);
		value = txgbe_rd32_epcs(hw, 0x100c8);
		txgbe_field_set(&value, 2, 2, 0);
		txgbe_wr32_epcs(hw, 0x100c8, value);

		//disable BASER FEC
		value = txgbe_rd32_epcs(hw, SR_PMA_KR_FEC_CTRL);
		txgbe_field_set(&value, 0, 0, 0);
		txgbe_wr32_epcs(hw, SR_PMA_KR_FEC_CTRL, value);
	}

	return 0;
}

int txgbe_e56_fec_mode_polling(struct txgbe_hw *hw, bool *link_up)
{
	struct txgbe_adapter *adapter = hw->back;
	int i = 0, j = 0;
	u32 speed;

	do {
		if (!(adapter->fec_link_mode & BIT(j % 3))) {
			j += 1;
			continue;
		}

		adapter->cur_fec_link = adapter->fec_link_mode & BIT(j % 3);

		mutex_lock(&adapter->e56_lock);
		txgbe_e56_set_fec_mode(hw, adapter->cur_fec_link);
		mutex_unlock(&adapter->e56_lock);


		for (i = 0; i < 4; i++) {
			msleep(250);
			txgbe_e56_check_phy_link(hw, &speed, link_up);
			if (*link_up)
				return 0;
		}

		j += 1;
	} while (j < 4);

	return 0;
}

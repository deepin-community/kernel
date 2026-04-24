// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2019 - 2022 Beijing WangXun Technology Co., Ltd. */

#include "txgbe_bp.h"

void txgbe_bp_close_protect(struct txgbe_adapter *adapter)
{
	adapter->flags2 |= TXGBE_FLAG2_KR_PRO_DOWN;
	while (adapter->flags2 & TXGBE_FLAG2_KR_PRO_REINIT) {
		msleep(100);
		kr_dbg(KR_MODE, "wait to reinited ok..%x\n", adapter->flags2);
	}
}

int txgbe_bp_mode_setting(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;

	/*default to open an73*/
	if ((hw->subsystem_device_id & TXGBE_DEV_MASK) == TXGBE_ID_KR_KX_KX4)
		adapter->backplane_an = 1;

	switch (hw->mac.type) {
	case txgbe_mac_sp:
			adapter->backplane_an = 0;
		break;
	case txgbe_mac_aml40:
	case txgbe_mac_aml:
	default:
		adapter->backplane_an = 1;
		break;
	}

	adapter->autoneg = 1;
	switch (adapter->backplane_mode) {
	case TXGBE_BP_M_KR:
		hw->subsystem_device_id = TXGBE_ID_WX1820_KR_KX_KX4;
		break;
	case TXGBE_BP_M_KX4:
		hw->subsystem_device_id = TXGBE_ID_WX1820_MAC_XAUI;
		break;
	case TXGBE_BP_M_KX:
		hw->subsystem_device_id = TXGBE_ID_WX1820_MAC_SGMII;
		break;
	case TXGBE_BP_M_SFI:
		hw->subsystem_device_id = TXGBE_ID_WX1820_SFP;
		break;
	default:
		break;
	}

	if (adapter->backplane_auto == TXGBE_BP_M_AUTO) {
		adapter->backplane_an = 1;
		adapter->autoneg = 1;
	} else if (adapter->backplane_auto == TXGBE_BP_M_NAUTO) {
		adapter->backplane_an = 0;
		adapter->autoneg = 0;
	}

	return 0;
}

void txgbe_bp_watchdog_event(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 value = 0;
	int ret = 0;

	/* only continue if link is down */
	if (netif_carrier_ok(adapter->netdev))
		return;

	if (adapter->flags2 & TXGBE_FLAG2_KR_TRAINING) {
		value = txgbe_rd32_epcs(hw, 0x78002);
		if ((value & BIT(2)) == BIT(2)) {
			e_info(hw, "Enter training\n");
			ret = handle_bkp_an73_flow(0, adapter);
			if (ret)
				txgbe_set_link_to_kr(hw, 1);
		}
		adapter->flags2 &= ~TXGBE_FLAG2_KR_TRAINING;
	}
}

void txgbe_bp_down_event(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 val = 0, val1 = 0;

	if (adapter->backplane_an == 0)
		return;

	val = txgbe_rd32_epcs(hw, 0x78002);
	val1 = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_CTL);
	kr_dbg(KR_MODE, "AN INT : %x - AN CTL : %x - PL : %x\n",
	       val, val1, txgbe_rd32_epcs(hw, 0x70012));

	msleep(100);
	if ((val & BIT(2)) == BIT(2)) {
		if (!(adapter->flags2 & TXGBE_FLAG2_KR_TRAINING))
			adapter->flags2 |= TXGBE_FLAG2_KR_TRAINING;
	} else {
		txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0);
		txgbe_wr32_epcs(hw, 0x78002, 0x0000);
		txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x3000);
	}
}

int chk_bkp_an73_ability(struct bkpan73ability tbkp_an73_ability,
			 struct bkpan73ability tlpbkp_an73_ability,
							struct txgbe_adapter *adapter)
{
	unsigned int com_link_ability;

	kr_dbg(KR_MODE, "CheckBkpAn73Ability():\n");
	kr_dbg(KR_MODE, "------------------------\n");

	/*-- Check the common link ability and take action based on the result*/
	com_link_ability = tbkp_an73_ability.link_ability & tlpbkp_an73_ability.link_ability;
	kr_dbg(KR_MODE, "com_link_ability= 0x%x, link_ability= 0x%x, lpLinkAbility= 0x%x\n",
	       com_link_ability, tbkp_an73_ability.link_ability, tlpbkp_an73_ability.link_ability);

	/*only support kr*/
	if (com_link_ability == 0) {
		kr_dbg(KR_MODE, "WARNING: The Link Partner does not support any compatible speed mode!!!\n\n");
		return -1;
	} else if (com_link_ability & 0x80) {
		if (tbkp_an73_ability.cu_linkmode == 0) {
			kr_dbg(KR_MODE, "Link mode is matched with Link Partner: [LINK_KR].\n");
			goto out;
		} else {
			kr_dbg(KR_MODE, "Link mode is not matched with Link Partner: [LINK_KR].\n");
			kr_dbg(KR_MODE, "Set the local link mode to [LINK_KR] ...\n");
			return 1;
		}
	}

out:
	return 0;
}

static void txgbe_bp_print_page_status(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 rdata = 0;

	rdata = txgbe_rd32_epcs(hw, 0x70010);
	kr_dbg(KR_MODE, "read 70010 data %0x\n", rdata);
	rdata = txgbe_rd32_epcs(hw, 0x70011);
	kr_dbg(KR_MODE, "read 70011 data %0x\n", rdata);
	rdata = txgbe_rd32_epcs(hw, 0x70012);
	kr_dbg(KR_MODE, "read 70012 data %0x\n", rdata);
	rdata = txgbe_rd32_epcs(hw, 0x70013);
	kr_dbg(KR_MODE, "read 70013 data %0x\n", rdata);
	rdata = txgbe_rd32_epcs(hw, 0x70014);
	kr_dbg(KR_MODE, "read 70014 data %0x\n", rdata);
	rdata = txgbe_rd32_epcs(hw, 0x70015);
	kr_dbg(KR_MODE, "read 70015 data %0x\n", rdata);
	rdata = txgbe_rd32_epcs(hw, 0x70016);
	kr_dbg(KR_MODE, "read 70016 data %0x\n", rdata);
	rdata = txgbe_rd32_epcs(hw, 0x70017);
	kr_dbg(KR_MODE, "read 70017 data %0x\n", rdata);
	rdata = txgbe_rd32_epcs(hw, 0x70018);
	kr_dbg(KR_MODE, "read 70018 data %0x\n", rdata);
	rdata = txgbe_rd32_epcs(hw, 0x70019);
	kr_dbg(KR_MODE, "read 70019 data %0x\n", rdata);
	rdata = txgbe_rd32_epcs(hw, 0x70020);
	kr_dbg(KR_MODE, "read 70020 data %0x\n", rdata);
	rdata = txgbe_rd32_epcs(hw, 0x70021);
	kr_dbg(KR_MODE, "read 70021 data %0x\n", rdata);
}

static void txgbe_bp_exchange_page(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 an_int, base_page = 0;
	int count = 0;

	an_int = txgbe_rd32_epcs(hw, 0x78002);
	if (!(an_int & BIT(2)))
		return;
	/* 500ms timeout */
	for (count = 0; count < 5000; count++) {
		kr_dbg(KR_MODE, "-----count----- %d\n", count);
		if (an_int & BIT(2)) {
			u8 next_page = 0;
			u32 rdata, addr;

			txgbe_bp_print_page_status(adapter);
			addr = base_page == 0 ? 0x70013 : 0x70019;
			rdata = txgbe_rd32_epcs(hw, addr);
			if (rdata & BIT(14)) {
				if (rdata & BIT(15)) {
					/* always set null message */
					txgbe_wr32_epcs(hw, 0x70016, 0x2001);
					kr_dbg(KR_MODE, "write 70016 0x%0x\n",
					       0x2001);
					rdata = txgbe_rd32_epcs(hw, 0x70010);
					txgbe_wr32_epcs(hw, 0x70010,
							rdata | BIT(15));
					kr_dbg(KR_MODE, "write 70010 0x%0x\n",
					       rdata);
					next_page = 1;
				} else {
					next_page = 0;
				}
				base_page = 1;
			}
			/* clear an pacv int */
			txgbe_wr32_epcs(hw, 0x78002, 0x0000);
			kr_dbg(KR_MODE, "write 78002 0x%0x\n", 0x0000);
			usec_delay(100);
			if (next_page == 0)
				return;
		}
		usec_delay(100);
	}
}

int get_bkp_an73_ability(struct bkpan73ability *pt_bkp_an73_ability, unsigned char by_link_partner,
			 struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	unsigned int rdata;
	int status = 0;

	kr_dbg(KR_MODE, "by_link_partner = %d\n", by_link_partner);
	kr_dbg(KR_MODE, "----------------------------------------\n");

	/* Link Partner Base Page */
	if (by_link_partner == 1) {
		/*Read the link partner AN73 Base Page Ability Registers*/
		kr_dbg(KR_MODE, "Read the link partner AN73 Base Page Ability Registers...\n");
		rdata = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_LP_ABL1);
		kr_dbg(KR_MODE, "SR AN MMD LP Base Page Ability Register 1: 0x%x\n", rdata);
		pt_bkp_an73_ability->next_page = (rdata >> 15) & 0x01;
		kr_dbg(KR_MODE, "  Next Page (bit15): %d\n", pt_bkp_an73_ability->next_page);

		/* if have next pages, exchange next pages. */
		if (pt_bkp_an73_ability->next_page)
			txgbe_bp_exchange_page(adapter);

		rdata = txgbe_rd32_epcs(hw, 0x70014);
		kr_dbg(KR_MODE, "SR AN MMD LP Base Page Ability Register 2: 0x%x\n", rdata);
		pt_bkp_an73_ability->link_ability = rdata & 0xE0;
		kr_dbg(KR_MODE, "  Link Ability (bit[15:0]): 0x%x\n",
		       pt_bkp_an73_ability->link_ability);
		kr_dbg(KR_MODE, "  (0x20- KX_ONLY, 0x40- KX4_ONLY, 0x60- KX4_KX\n");
		kr_dbg(KR_MODE, "   0x80- KR_ONLY, 0xA0- KR_KX, 0xC0- KR_KX4, 0xE0- KR_KX4_KX)\n");

		rdata = txgbe_rd32_epcs(hw, 0x70015);
		kr_dbg(KR_MODE, "SR AN MMD LP Base Page Ability Register 3: 0x%x\n", rdata);
		kr_dbg(KR_MODE, "  FEC Request (bit15): %d\n", ((rdata >> 15) & 0x01));
		kr_dbg(KR_MODE, "  FEC Enable  (bit14): %d\n", ((rdata >> 14) & 0x01));
		pt_bkp_an73_ability->fec_ability = (rdata >> 14) & 0x03;
	} else if (by_link_partner == 2) {/*Link Partner Next Page*/
		/*Read the link partner AN73 Next Page Ability Registers*/
		kr_dbg(KR_MODE, "\nRead the link partner AN73 Next Page Ability Registers...\n");
		rdata = txgbe_rd32_epcs(hw, 0x70019);
		kr_dbg(KR_MODE, " SR AN MMD LP XNP Ability Register 1: 0x%x\n", rdata);
		pt_bkp_an73_ability->next_page = (rdata >> 15) & 0x01;
		if (KR_MODE)
			e_dev_info("  Next Page (bit15): %d\n", pt_bkp_an73_ability->next_page);
	} else {
		/*Read the local AN73 Base Page Ability Registers*/
		kr_dbg(KR_MODE, "\nRead the local AN73 Base Page Ability Registers...\n");
		rdata = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_ADV_REG1);
		kr_dbg(KR_MODE, "SR AN MMD Advertisement Register 1: 0x%x\n", rdata);
		pt_bkp_an73_ability->next_page = (rdata >> 15) & 0x01;
		kr_dbg(KR_MODE, "  Next Page (bit15): %d\n", pt_bkp_an73_ability->next_page);

		rdata = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_ADV_REG2);
		kr_dbg(KR_MODE, "SR AN MMD Advertisement Register 2: 0x%x\n", rdata);
		pt_bkp_an73_ability->link_ability = rdata & 0xE0;
		kr_dbg(KR_MODE, "  Link Ability (bit[15:0]): 0x%x\n",
		       pt_bkp_an73_ability->link_ability);
		kr_dbg(KR_MODE, "  (0x20- KX_ONLY, 0x40- KX4_ONLY, 0x60- KX4_KX\n");
		kr_dbg(KR_MODE, "   0x80- KR_ONLY, 0xA0- KR_KX, 0xC0- KR_KX4, 0xE0- KR_KX4_KX)\n");

		rdata = txgbe_rd32_epcs(hw, 0x70012);
		kr_dbg(KR_MODE, "SR AN MMD Advertisement Register 3: 0x%x\n", rdata);
		kr_dbg(KR_MODE, "  FEC Request (bit15): %d\n", ((rdata >> 15) & 0x01));
		kr_dbg(KR_MODE, "  FEC Enable  (bit14): %d\n", ((rdata >> 14) & 0x01));
		pt_bkp_an73_ability->fec_ability = (rdata >> 14) & 0x03;
	} /*if (by_link_partner == 1) Link Partner Base Page*/

	return status;
}

static void read_phy_lane_txeq(unsigned short lane, struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	unsigned int addr, rdata;

	/*LANEN_DIG_ASIC_TX_ASIC_IN_1[11:6]: TX_MAIN_CURSOR*/
	addr  = 0x100E | (lane << 8);
	rdata = rd32_ephy(hw, addr);
	kr_dbg(KR_MODE, "PHY LANE%0d TX EQ Read Value:\n", lane);
	kr_dbg(KR_MODE, "  TX_MAIN_CURSOR: %d\n", ((rdata >> 6) & 0x3F));

	/*LANEN_DIG_ASIC_TX_ASIC_IN_2[5 :0]: TX_PRE_CURSOR*/
	/*LANEN_DIG_ASIC_TX_ASIC_IN_2[11:6]: TX_POST_CURSOR*/
	addr  = 0x100F | (lane << 8);
	rdata = rd32_ephy(hw, addr);
	kr_dbg(KR_MODE, "  TX_PRE_CURSOR : %d\n", (rdata & 0x3F));
	kr_dbg(KR_MODE, "  TX_POST_CURSOR: %d\n", ((rdata >> 6) & 0x3F));
	kr_dbg(KR_MODE, "**********************************************\n");
}

static int en_cl72_krtr(unsigned int enable, struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	unsigned int wdata = 0;
	u32 val;

	if (enable == 1) {
		kr_dbg(KR_MODE, "\nDisable Clause 72 KR Training ...\n");
		read_phy_lane_txeq(0, adapter);
	} else if (enable == 3) {
		kr_dbg(KR_MODE, "\nEnable Clause 72 KR Training ...\n");

		val = txgbe_rd32_epcs(hw, 0x18003);
		wdata |= val;
		txgbe_wr32_epcs(hw, 0x18003, wdata);
		read_phy_lane_txeq(0, adapter);
	}

	/* Enable the Clause 72 start-up protocol by setting Bit 1 of SR_PMA_KR_PMD_CTRL Register.
	 * Restart the Clause 72 start-up protocol by setting Bit 0 of SR_PMA_KR_PMD_CTRL Register
	 */
	wdata = enable;
	txgbe_wr32_epcs(hw, 0x10096, wdata);
	return 0;
}

static int chk_cl72_krtr_status(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	unsigned int rdata = 0, rdata1;
	int status = 0;

	status = read_poll_timeout(txgbe_rd32_epcs, rdata1, (rdata1 & 0x9), 1000,
				   400000, false, hw, 0x10097);
	if (!status) {
		//Get the latest received coefficient update or status
		rdata = txgbe_rd32_epcs(hw, 0x010098);
		kr_dbg(KR_MODE, "SR PMA MMD 10GBASE-KR LP Coefficient Update Register: 0x%x\n",
		       rdata);
		rdata = txgbe_rd32_epcs(hw, 0x010099);
		kr_dbg(KR_MODE, "SR PMA MMD 10GBASE-KR LP Coefficient Status Register: 0x%x\n",
		       rdata);
		rdata = txgbe_rd32_epcs(hw, 0x01009a);
		kr_dbg(KR_MODE, "SR PMA MMD 10GBASE-KR LD Coefficient Update: 0x%x\n", rdata);

		rdata = txgbe_rd32_epcs(hw, 0x01009b);
		kr_dbg(KR_MODE, " SR PMA MMD 10GBASE-KR LD Coefficient Status: 0x%x\n", rdata);

		rdata = txgbe_rd32_epcs(hw, 0x010097);
		kr_dbg(KR_MODE, "SR PMA MMD 10GBASE-KR Status Register: 0x%x\n", rdata);
		kr_dbg(KR_MODE, "  Training Failure         (bit3): %d\n", ((rdata >> 3) & 0x01));
		kr_dbg(KR_MODE, "  Start-Up Protocol Status (bit2): %d\n", ((rdata >> 2) & 0x01));
		kr_dbg(KR_MODE, "  Frame Lock               (bit1): %d\n", ((rdata >> 1) & 0x01));
		kr_dbg(KR_MODE, "  Receiver Status          (bit0): %d\n", ((rdata >> 0) & 0x01));

		/*If bit3 is set, Training is completed with failure*/
		if ((rdata1 >> 3) & 0x01) {
			kr_dbg(KR_MODE, "Training is completed with failure!!!\n");
			read_phy_lane_txeq(0, adapter);
			return status;
		}

		/*If bit0 is set, Receiver trained and ready to receive data*/
		if ((rdata1 >> 0) & 0x01) {
			kr_dbg(KR_MODE, "Receiver trained and ready to receive data ^_^\n");
			e_info(hw, "Receiver ready.\n");
			read_phy_lane_txeq(0, adapter);
			return status;
		}
	}

	kr_dbg(KR_MODE, "ERROR: Check Clause 72 KR Training Complete Timeout!!!\n");

	return status;
}

static int txgbe_cl72_trainning(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	u32 rdata = 0, rdata1 = 0;
	bool lpld_all_rd = false;
	int ret = 0;

	txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0);

	ret |= en_cl72_krtr(3, adapter);
	kr_dbg(KR_MODE, "\nCheck the Clause 72 KR Training status ...\n");
	ret |= chk_cl72_krtr_status(adapter);

	ret = read_poll_timeout(txgbe_rd32_epcs, rdata, (rdata & 0x8000), 1000,
				200000, false, hw, 0x10099);
	if (!ret) {
		rdata1 = txgbe_rd32_epcs(hw, 0x1009b) & 0x8000;
		if (rdata1 == 0x8000)
			lpld_all_rd = true;
	}

	if (lpld_all_rd) {
		rdata = rd32_ephy(hw, 0x100E);
		rdata1 = rd32_ephy(hw, 0x100F);
		e_dev_info("Lp and Ld all Ready, FFE : %d-%d-%d.\n",
			   (rdata >> 6) & 0x3F, rdata1 & 0x3F, (rdata1 >> 6) & 0x3F);
		if (!hw->dac_sfp)
			if ((((rdata >> 6) & 0x3F) == 27) &&
			    ((rdata1 & 0x3F) == 8) &&
			    (((rdata1 >> 6) & 0x3F)) == 44)
				return -1;
		/* clear an pacv int */
		txgbe_wr32_epcs(hw, 0x78002, 0x0000);
		ret = read_poll_timeout(txgbe_rd32_epcs, rdata, (rdata & 0x1000), 1000,
					100000, false, hw, 0x30020);
		if (!ret)
			e_dev_info("INT_AN_INT_CMPLT =1, AN73 Done Success.\n");
		return 0;
	}
	/* clear an pacv int */
	txgbe_wr32_epcs(hw, 0x78002, 0x0000);

	return -1;
}

int handle_bkp_an73_flow(unsigned char bp_link_mode, struct txgbe_adapter *adapter)
{
	struct bkpan73ability tbkp_an73_ability, tlpbkp_an73_ability;
	struct txgbe_hw *hw = &adapter->hw;
	bool fec_en = false;
	u32 fec_ability = 0;
	int ret = 0;

	tbkp_an73_ability.cu_linkmode = bp_link_mode;

	kr_dbg(KR_MODE, "HandleBkpAn73Flow().\n");
	kr_dbg(KR_MODE, "---------------------------------\n");

	/*1. Get the local AN73 Base Page Ability*/
	kr_dbg(KR_MODE, "<1>. Get the local AN73 Base Page Ability ...\n");
	get_bkp_an73_ability(&tbkp_an73_ability, 0, adapter);
	/*2. Check the AN73 Interrupt Status*/
	kr_dbg(KR_MODE, "<2>. Check the AN73 Interrupt Status ...\n");

	/*3.1. Get the link partner AN73 Base Page Ability*/
	kr_dbg(KR_MODE, "<3.1>. Get the link partner AN73 Base Page Ability ...\n");
	get_bkp_an73_ability(&tlpbkp_an73_ability, 1, adapter);

	/*3.2. Check the AN73 Link Ability with Link Partner*/
	kr_dbg(KR_MODE, "<3.2>. Check the AN73 Link Ability with Link Partner ...\n");
	kr_dbg(KR_MODE, "Local Link Ability: 0x%x\n", tbkp_an73_ability.link_ability);
	kr_dbg(KR_MODE, "Link Partner Link Ability: 0x%x\n", tlpbkp_an73_ability.link_ability);

	chk_bkp_an73_ability(tbkp_an73_ability, tlpbkp_an73_ability, adapter);

	/*Check the FEC and KR Training for KR mode*/
	kr_dbg(KR_MODE, "<3.3>. Check the FEC for KR mode ...\n");
	fec_ability = tbkp_an73_ability.fec_ability & tlpbkp_an73_ability.fec_ability;
	fec_en = fec_ability >= 0x1 ? true : false;
	adapter->cur_fec_link = fec_en ?
				TXGBE_PHY_FEC_BASER : TXGBE_PHY_FEC_OFF;
	/* SR_PMA_KR_FEC_CTRL  bit0 */
	txgbe_wr32_epcs(hw, 0x100ab, fec_en);
	e_dev_info("KR FEC is %s.\n", fec_en ? "endabled" : "disabled");
	kr_dbg(KR_MODE, "\n<3.4>. Check the CL72 KR Training for KR mode ...\n");

	ret = txgbe_cl72_trainning(adapter);
	if (ret)
		kr_dbg(KR_MODE, "Trainning failure\n");
	return ret;
}

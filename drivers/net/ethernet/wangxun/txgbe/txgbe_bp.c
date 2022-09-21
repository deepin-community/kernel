#include "txgbe_bp.h"

int handle_bkp_an73_flow(unsigned char bp_link_mode, struct txgbe_adapter *adapter);
int wait_bkp_an73_xnp_done(struct txgbe_adapter *adapter);
int get_bkp_an73_ability(bkpan73ability *pt_bkp_an73_ability, unsigned char byLinkPartner , 
								struct txgbe_adapter *adapter);
int clr_bkp_an73_int(unsigned int intIndex, unsigned int intIndexHi, struct txgbe_adapter * adapter);
int chk_bkp_an73_Int(unsigned int intIndex, struct txgbe_adapter *adapter);
int chk_bkp_an73_ability(bkpan73ability tBkpAn73Ability, bkpan73ability tLpBkpAn73Ability,
								struct txgbe_adapter *adapter);

void txgbe_bp_close_protect(struct txgbe_adapter *adapter)
{
	adapter->flags2 |= TXGBE_FLAG2_KR_PRO_DOWN;
	while (adapter->flags2 & TXGBE_FLAG2_KR_PRO_REINIT){
		msleep(100);
		printk("wait to reinited ok..%x\n",adapter->flags2);
	}
}

int txgbe_bp_mode_setting(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;

	/*default to open an73*/

	adapter->backplane_an = AUTO?1:0;
	adapter->an37 = AUTO?1:0;
	
	if (adapter->backplane_mode == TXGBE_BP_M_KR) {
		hw->subsystem_device_id = TXGBE_ID_WX1820_KR_KX_KX4;
		hw->subsystem_id = TXGBE_ID_WX1820_KR_KX_KX4;
	} else if (adapter->backplane_mode == TXGBE_BP_M_KX4) {
		hw->subsystem_device_id = TXGBE_ID_WX1820_MAC_XAUI;
		hw->subsystem_id = TXGBE_ID_WX1820_MAC_XAUI;
	} else if (adapter->backplane_mode == TXGBE_BP_M_KX) {
		hw->subsystem_device_id = TXGBE_ID_WX1820_MAC_SGMII;
		hw->subsystem_id = TXGBE_ID_WX1820_MAC_SGMII;
	} else if (adapter->backplane_mode == TXGBE_BP_M_SFI) {
		hw->subsystem_device_id = TXGBE_ID_WX1820_SFP;
		hw->subsystem_id = TXGBE_ID_WX1820_SFP;
	} 

	if (adapter->backplane_auto == TXGBE_BP_M_AUTO) {
		adapter->backplane_an = 1;
		adapter->an37 = 1;
	} else if (adapter->backplane_auto == TXGBE_BP_M_NAUTO) {
		adapter->backplane_an = 0;
		adapter->an37 = 0;
	}

	if (adapter->ffe_set == TXGBE_BP_M_KR ||
		adapter->ffe_set == TXGBE_BP_M_KX4 ||
		adapter->ffe_set == TXGBE_BP_M_KX ||
		adapter->ffe_set == TXGBE_BP_M_SFI ) {
		goto out;
	}

	if (KR_SET == 1 ) {
		adapter->ffe_main = KR_MAIN;
		adapter->ffe_pre = KR_PRE;
		adapter->ffe_post = KR_POST;
	} else if (KX4_SET == 1) {
		adapter->ffe_main = KX4_MAIN;
		adapter->ffe_pre = KX4_PRE;
		adapter->ffe_post = KX4_POST;
	} else if (KX_SET == 1) {
		adapter->ffe_main = KX_MAIN;
		adapter->ffe_pre = KX_PRE;
		adapter->ffe_post = KX_POST;
	} else if (SFI_SET == 1) {
		adapter->ffe_main = SFI_MAIN;
		adapter->ffe_pre = SFI_PRE;
		adapter->ffe_post = SFI_POST;
	}
out:
	return 0;
}

static int txgbe_kr_subtask(struct txgbe_adapter *adapter)
{
	handle_bkp_an73_flow(0, adapter);
	return 0;
}

void txgbe_bp_watchdog_event(struct txgbe_adapter *adapter)
{
	u32 value = 0;
	struct txgbe_hw *hw = &adapter->hw;
	
	if (KR_POLLING == 1) {
		value = txgbe_rd32_epcs(hw, 0x78002);
		value = value & 0x4;
		if(value == 0x4){
			e_dev_info("Enter training\n");
			txgbe_kr_subtask(adapter);
		}
	} else {
		if(adapter->flags2 & TXGBE_FLAG2_KR_TRAINING){
			e_dev_info("Enter training\n");
			txgbe_kr_subtask(adapter);
			adapter->flags2 &= ~TXGBE_FLAG2_KR_TRAINING;
		}
	}
}

void txgbe_bp_down_event(struct txgbe_adapter *adapter)
{
	struct txgbe_hw *hw = &adapter->hw;
	if( adapter->backplane_an == 1 ){
		if(KR_NORESET == 1){
			txgbe_wr32_epcs(hw, TXGBE_VR_AN_KR_MODE_CL, 0x0000);
			txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x0000);
			txgbe_wr32_epcs(hw, 0x78001, 0x0000);
			txgbe_wr32_epcs(hw, 0x30007, 1);
			txgbe_wr32_epcs(hw, 0x10000, 0x2000);
			msleep(1050);
			txgbe_set_link_to_kr(hw, 1);
		} else if(KR_NOREINITED == 1){
			txgbe_wr32_epcs(hw, TXGBE_VR_AN_KR_MODE_CL, 0x0000);
			txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x0000);
			txgbe_wr32_epcs(hw, 0x78001, 0x0000);
			msleep(1050);
			txgbe_wr32_epcs(hw, TXGBE_VR_AN_KR_MODE_CL, 0x0001);
			txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x3200);
			txgbe_wr32_epcs(hw, 0x78001, 0x0007);
		} else {
			msleep(200);
			if (!(adapter->flags2&TXGBE_FLAG2_KR_PRO_DOWN)) {
				//adapter->flags2 |=  TXGBE_FLAG2_KR_PRO_REINIT;
				txgbe_reinit_locked(adapter);
				//adapter->flags2 &= ~TXGBE_FLAG2_KR_PRO_REINIT;
			}
		}
	}
}

int txgbe_kr_intr_handle(struct txgbe_adapter *adapter)
{ 
	bkpan73ability tBkpAn73Ability, tLpBkpAn73Ability;
	tBkpAn73Ability.currentLinkMode = 0;

	if (KR_MODE) e_dev_info("HandleBkpAn73Flow() \n");
	if (KR_MODE) e_dev_info("---------------------------------\n");
	
	/*1. Get the local AN73 Base Page Ability*/
	if (KR_MODE) e_dev_info("<1>. Get the local AN73 Base Page Ability ...\n");
	get_bkp_an73_ability(&tBkpAn73Ability, 0, adapter);
	/*2. Check the AN73 Interrupt Status*/
	if (KR_MODE) e_dev_info("<2>. Check the AN73 Interrupt Status ...\n");
	/*3.Clear the AN_PG_RCV interrupt*/
	clr_bkp_an73_int(2, 0x0, adapter);

	/*3.1. Get the link partner AN73 Base Page Ability*/
	if (KR_MODE) e_dev_info("<3.1>. Get the link partner AN73 Base Page Ability ...\n");
	get_bkp_an73_ability(&tLpBkpAn73Ability, 1, adapter);

	/*3.2. Check the AN73 Link Ability with Link Partner*/
	if (KR_MODE) e_dev_info("<3.2>. Check the AN73 Link Ability with Link Partner ...\n");
	if (KR_MODE) e_dev_info("		  Local Link Ability: 0x%x\n", tBkpAn73Ability.linkAbility);
	if (KR_MODE) e_dev_info("  Link Partner Link Ability: 0x%x\n", tLpBkpAn73Ability.linkAbility);

	chk_bkp_an73_ability(tBkpAn73Ability, tLpBkpAn73Ability, adapter);
	return 0;

}

/*Check Ethernet Backplane AN73 Base Page Ability
**return value:
**  -1 : none link mode matched, exit
**   0 : current link mode matched, wait AN73 to be completed
**   1 : current link mode not matched, set to matched link mode, re-start AN73 external
*/
int chk_bkp_an73_ability(bkpan73ability tBkpAn73Ability, bkpan73ability tLpBkpAn73Ability,
								struct txgbe_adapter *adapter)
{
	unsigned int comLinkAbility;
	struct txgbe_hw *hw = &adapter->hw;

	if (KR_MODE) e_dev_info("CheckBkpAn73Ability():\n");
	if (KR_MODE) e_dev_info("------------------------\n");

	/*-- Check the common link ability and take action based on the result*/
	comLinkAbility = tBkpAn73Ability.linkAbility & tLpBkpAn73Ability.linkAbility;
	if (KR_MODE) e_dev_info("comLinkAbility= 0x%x, linkAbility= 0x%x, lpLinkAbility= 0x%x\n", comLinkAbility, tBkpAn73Ability.linkAbility, tLpBkpAn73Ability.linkAbility);

	/*only support kr*/
	if (comLinkAbility == 0){
		if (KR_MODE) e_dev_info("WARNING: The Link Partner does not support any compatible speed mode!!!\n\n");
		return -1;
	} else if (comLinkAbility & 0x80) {
		if (tBkpAn73Ability.currentLinkMode == 0){
			if (KR_MODE) e_dev_info("Link mode is matched with Link Partner: [LINK_KR].\n");
			return 0;
		} else {
			if (KR_MODE) e_dev_info("Link mode is not matched with Link Partner: [LINK_KR].\n");
			if (KR_MODE) e_dev_info("Set the local link mode to [LINK_KR] ...\n");
			txgbe_set_link_to_kr(hw, 1);
			return 1;
		}
	}

#if 0
	if (comLinkAbility == 0){
		if (KR_MODE) e_dev_info("WARNING: The Link Partner does not support any compatible speed mode!!!\n\n");
		return -1;
	} else if (comLinkAbility & 0x80) {
		if (tBkpAn73Ability.currentLinkMode == 0){
			if (KR_MODE) e_dev_info("Link mode is matched with Link Partner: [LINK_KR].\n");
			return 0;
		}else{
			if (KR_MODE) e_dev_info("Link mode is not matched with Link Partner: [LINK_KR].\n");
			if (KR_MODE) e_dev_info("Set the local link mode to [LINK_KR] ...\n");
			txgbe_set_link_to_kr(hw, 1);
			return 1;
		}
	} else if (comLinkAbility & 0x40) {
		if (tBkpAn73Ability.currentLinkMode == 0x10){
			if (KR_MODE) e_dev_info("Link mode is matched with Link Partner: [LINK_KX4].\n");
			return 0;
		} else {
			if (KR_MODE) e_dev_info("Link mode is not matched with Link Partner: [LINK_KX4].\n");
			if (KR_MODE) e_dev_info("Set the local link mode to [LINK_KX4] ...\n");
			txgbe_set_link_to_kx4(hw, 1);
			return 1;
		}
	} else if (comLinkAbility & 0x20) {
		if (tBkpAn73Ability.currentLinkMode == 0x1){
			if (KR_MODE) e_dev_info("Link mode is matched with Link Partner: [LINK_KX].\n");
			return 0;
		} else {
			if (KR_MODE) e_dev_info("Link mode is not matched with Link Partner: [LINK_KX].\n");
			if (KR_MODE) e_dev_info("Set the local link mode to [LINK_KX] ...\n");
			txgbe_set_link_to_kx(hw, 1, 1);
			return 1;
		}
	}
#endif
	return 0;
}


/*Get Ethernet Backplane AN73 Base Page Ability
**byLinkPartner:
**- 1: Get Link Partner Base Page
**- 2: Get Link Partner Next Page (only get NXP Ability Register 1 at the moment)
**- 0: Get Local Device Base Page
*/
int get_bkp_an73_ability(bkpan73ability *pt_bkp_an73_ability, unsigned char byLinkPartner , 
								struct txgbe_adapter *adapter)
{
	int status = 0;
	unsigned int rdata;
	struct txgbe_hw *hw = &adapter->hw;

	if (KR_MODE) e_dev_info("get_bkp_an73_ability(): byLinkPartner = %d\n", byLinkPartner);
	if (KR_MODE) e_dev_info("----------------------------------------\n");

	if (byLinkPartner == 1) /*Link Partner Base Page*/
	{
		/*Read the link partner AN73 Base Page Ability Registers*/
		if (KR_MODE) e_dev_info("Read the link partner AN73 Base Page Ability Registers...\n");
		rdata = 0;
		rdata = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_LP_ABL1);
		if (KR_MODE) e_dev_info("SR AN MMD LP Base Page Ability Register 1: 0x%x\n", rdata);
		pt_bkp_an73_ability->nextPage = (rdata >> 15) & 0x01;
		if (KR_MODE) e_dev_info("  Next Page (bit15): %d\n", pt_bkp_an73_ability->nextPage);

		rdata = 0;
		rdata = txgbe_rd32_epcs(hw, 0x70014);
		if (KR_MODE) e_dev_info("SR AN MMD LP Base Page Ability Register 2: 0x%x\n", rdata);
		pt_bkp_an73_ability->linkAbility = rdata & 0xE0;
		if (KR_MODE) e_dev_info("  Link Ability (bit[15:0]): 0x%x\n", pt_bkp_an73_ability->linkAbility);
		if (KR_MODE) e_dev_info("  (0x20- KX_ONLY, 0x40- KX4_ONLY, 0x60- KX4_KX\n");
		if (KR_MODE) e_dev_info("   0x80- KR_ONLY, 0xA0- KR_KX, 0xC0- KR_KX4, 0xE0- KR_KX4_KX)\n");

		rdata = 0;
		rdata = txgbe_rd32_epcs(hw, 0x70015);
		if (KR_MODE) e_dev_info("SR AN MMD LP Base Page Ability Register 3: 0x%x\n", rdata);
		if (KR_MODE) e_dev_info("  FEC Request (bit15): %d\n", ((rdata >> 15) & 0x01));
		if (KR_MODE) e_dev_info("  FEC Enable  (bit14): %d\n", ((rdata >> 14) & 0x01));
		pt_bkp_an73_ability->fecAbility = (rdata >> 14) & 0x03;
	} else if (byLinkPartner == 2) {/*Link Partner Next Page*/
		/*Read the link partner AN73 Next Page Ability Registers*/
		if (KR_MODE) e_dev_info("\nRead the link partner AN73 Next Page Ability Registers...\n");
		rdata = 0;
		rdata = txgbe_rd32_epcs(hw, 0x70019);
		if (KR_MODE) e_dev_info(" SR AN MMD LP XNP Ability Register 1: 0x%x\n", rdata);
		pt_bkp_an73_ability->nextPage = (rdata >> 15) & 0x01;
		if (KR_MODE)e_dev_info("  Next Page (bit15): %d\n", pt_bkp_an73_ability->nextPage);
	} else {
		/*Read the local AN73 Base Page Ability Registers*/
		if (KR_MODE) e_dev_info("\nRead the local AN73 Base Page Ability Registers...\n");
		rdata = 0;
		rdata = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_ADV_REG1);
		if (KR_MODE) e_dev_info("SR AN MMD Advertisement Register 1: 0x%x\n", rdata);
		pt_bkp_an73_ability->nextPage = (rdata >> 15) & 0x01;
		if (KR_MODE) e_dev_info("  Next Page (bit15): %d\n", pt_bkp_an73_ability->nextPage);

		rdata = 0;
		rdata = txgbe_rd32_epcs(hw, TXGBE_SR_AN_MMD_ADV_REG2);
		if (KR_MODE) e_dev_info("SR AN MMD Advertisement Register 2: 0x%x\n", rdata);
		pt_bkp_an73_ability->linkAbility = rdata & 0xE0;
		if (KR_MODE) e_dev_info("  Link Ability (bit[15:0]): 0x%x\n", pt_bkp_an73_ability->linkAbility);
		if (KR_MODE) e_dev_info("  (0x20- KX_ONLY, 0x40- KX4_ONLY, 0x60- KX4_KX\n");
		if (KR_MODE) e_dev_info("   0x80- KR_ONLY, 0xA0- KR_KX, 0xC0- KR_KX4, 0xE0- KR_KX4_KX)\n");

		rdata = 0;
		rdata = txgbe_rd32_epcs(hw, 0x70012);
		if (KR_MODE) e_dev_info("SR AN MMD Advertisement Register 3: 0x%x\n", rdata);
		if (KR_MODE) e_dev_info("  FEC Request (bit15): %d\n", ((rdata >> 15) & 0x01));
		if (KR_MODE) e_dev_info("  FEC Enable  (bit14): %d\n", ((rdata >> 14) & 0x01));
		pt_bkp_an73_ability->fecAbility = (rdata >> 14) & 0x03;
	} /*if (byLinkPartner == 1) Link Partner Base Page*/

	if (KR_MODE) e_dev_info("get_bkp_an73_ability() done.\n");

	return status;
}

/* DESCRIPTION: Set the source data fields[bitHigh:bitLow] with setValue
** INPUTS:      *src_data: Source data pointer
**              bitHigh: High bit position of the fields
**              bitLow : Low bit position of the fields
**              setValue: Set value of the fields
** OUTPUTS:     return the updated source data
*/
static void set_fields(
    unsigned int *src_data,
    unsigned int bitHigh,
    unsigned int bitLow,
    unsigned int setValue) 
{
	int i;

	if (bitHigh == bitLow) {
		if (setValue == 0) { 
			*src_data &= ~(1 << bitLow);
		}
		else { 
			*src_data |= (1 << bitLow);
		}
	}else {
		for (i = bitLow; i <= bitHigh; i++) {
			*src_data &= ~(1 << i); 
		}
		*src_data |= (setValue << bitLow);
	}
}

/*Check Ethernet Backplane AN73 Interrupt status
**- return the value of select interrupt index
*/
int chk_bkp_an73_Int(unsigned int intIndex, struct txgbe_adapter *adapter)
{
	unsigned int rdata;
	struct txgbe_hw *hw = &adapter->hw;

	if (KR_MODE) e_dev_info("chk_bkp_an73_Int(): intIndex = %d\n", intIndex);
	if (KR_MODE) e_dev_info("----------------------------------------\n");

	rdata = 0x0000;
	rdata = txgbe_rd32_epcs(hw, 0x78002);
	if (KR_MODE) e_dev_info("Read VR AN MMD Interrupt Register: 0x%x\n", rdata);
	if (KR_MODE) e_dev_info("Interrupt: 0- AN_INT_CMPLT, 1-  AN_INC_LINK, 2- AN_PG_RCV\n\n"); 

	return ((rdata >> intIndex) & 0x01);
}

/*Clear Ethernet Backplane AN73 Interrupt status
**- intIndexHi  =0, only intIndex bit will be cleared
**- intIndexHi !=0, the [intIndexHi, intIndex] range will be cleared
*/
int clr_bkp_an73_int(unsigned int intIndex, unsigned int intIndexHi, struct txgbe_adapter * adapter)
{
	int status = 0;
	unsigned int rdata, wdata;
	struct txgbe_hw *hw = &adapter->hw;


	if (KR_MODE) e_dev_info("clr_bkp_an73_int(): intIndex = %d\n", intIndex);
	if (KR_MODE) e_dev_info("----------------------------------------\n");

	rdata = 0x0000;
	rdata = txgbe_rd32_epcs(hw, 0x78002);
	if (KR_MODE) e_dev_info("[Before clear] Read VR AN MMD Interrupt Register: 0x%x\n", rdata);

	wdata = rdata;
	if (intIndexHi) {
        	set_fields(&wdata, intIndexHi, intIndex, 0);
	} else {
		set_fields(&wdata, intIndex, intIndex, 0);
	} 
	txgbe_wr32_epcs(hw, 0x78002, wdata);

	rdata = 0x0000;
	rdata = txgbe_rd32_epcs(hw, 0x78002);
	if (KR_MODE) e_dev_info("[After clear] Read VR AN MMD Interrupt Register: 0x%x\n", rdata);
	if (KR_MODE) e_dev_info("\n");

	return status;
}

int wait_bkp_an73_xnp_done(struct txgbe_adapter *adapter)
{
	int status = 0;
	unsigned int timer = 0;
	bkpan73ability tLpBkpAn73Ability;

    	/*while(timer++ < BKPAN73_TIMEOUT)*/
	while(timer++ < 20) { 
		if (chk_bkp_an73_Int(2, adapter))
		{
	    		/*Clear the AN_PG_RCV interrupt*/
	    		clr_bkp_an73_int(2, 0, adapter);

			/*Get the link partner AN73 Next Page Ability*/
	    		get_bkp_an73_ability(&tLpBkpAn73Ability, 2, adapter);

	    		/*Return when AN_LP_XNP_NP == 0, (bit[15]: Next Page)*/
	    		if (tLpBkpAn73Ability.nextPage == 0)
	    		{
				return status;
	    		}
		}
		msleep(200);
    	}  /*while(timer++ < BKPAN73_TIMEOUT)*/
	if (KR_MODE) e_dev_info("ERROR: Wait all the AN73 next pages to be exchanged Timeout!!!\n");

	return -1;
}


int read_phy_lane_txeq(unsigned short lane, struct txgbe_adapter *adapter, int post_t, int mode)
{
	int status = 0;
	unsigned int addr, rdata;
	struct txgbe_hw *hw = &adapter->hw;
	u32 pre;
	u32 post;
	u32 lmain;

	/*LANEN_DIG_ASIC_TX_ASIC_IN_1[11:6]: TX_MAIN_CURSOR*/
	rdata = 0;
	addr  = 0x100E | (lane << 8);
	rdata = rd32_ephy(hw, addr);
	if (KR_MODE) e_dev_info("PHY LANE%0d TX EQ Read Value:\n", lane);
	if (KR_MODE) e_dev_info("  TX_MAIN_CURSOR: %d\n", ((rdata >> 6) & 0x3F));

	/*LANEN_DIG_ASIC_TX_ASIC_IN_2[5 :0]: TX_PRE_CURSOR*/
	/*LANEN_DIG_ASIC_TX_ASIC_IN_2[11:6]: TX_POST_CURSOR*/
	rdata = 0;
	addr  = 0x100F | (lane << 8);
	rdata = rd32_ephy(hw, addr);
	if (KR_MODE) e_dev_info("  TX_PRE_CURSOR : %d\n", (rdata & 0x3F));
	if (KR_MODE) e_dev_info("  TX_POST_CURSOR: %d\n", ((rdata >> 6) & 0x3F));
	if (KR_MODE) e_dev_info("**********************************************\n");

	if(mode == 1){
		pre = (rdata & 0x3F);
		post = ((rdata >> 6) & 0x3F);
		if((160 - pre -post ) < 88)
			lmain = 88;
		else	
			lmain = 160 - pre - post;
		if (post_t != 0)
			post = post_t;
		txgbe_wr32_epcs( hw, 0x1803b , post );
		txgbe_wr32_epcs( hw, 0x1803a , pre | (lmain << 8));
		txgbe_wr32_epcs( hw, 0x18037 , txgbe_rd32_epcs(hw, 0x18037) & 0xff7f);
	}
	if (KR_MODE) e_dev_info("**********************************************\n");

	return status;
}


/*Enable Clause 72 KR training
**
**Note:
**<1>. The Clause 72 start-up protocol should be initiated when all pages are exchanged during Clause 73 auto-
**negotiation and when the auto-negotiation process is waiting for link status to be UP for 500 ms after 
**exchanging all the pages.
**
**<2>. The local device and link partner should be enabled the CL72 KR training
**with in 500ms
**
**enable:
**- bits[1:0] =2'b11: Enable the CL72 KR training
**- bits[1:0] =2'b01: Disable the CL72 KR training
*/
int en_cl72_krtr(unsigned int enable, struct txgbe_adapter *adapter)
{
	int status = 0;
	unsigned int wdata=0;
	struct txgbe_hw *hw = &adapter->hw;

	if (enable == 1) {
		if (KR_MODE) e_dev_info("\nDisable Clause 72 KR Training ...\n");
    		status |= read_phy_lane_txeq(0, adapter, 0, 0);
	} else if (enable == 4) {
    		status |= read_phy_lane_txeq(0, adapter, 20, 1);
	} else if (enable == 8) {
    		status |= read_phy_lane_txeq(0, adapter, 16, 1);
	} else if (enable == 12) {
    		status |= read_phy_lane_txeq(0, adapter, 24, 1);
	} else if (enable == 5) {
    		status |= read_phy_lane_txeq(0, adapter, 0, 1);
	} else if (enable == 3) {
        	if (KR_MODE) e_dev_info("\nEnable Clause 72 KR Training ...\n");

		if (CL72_KRTR_PRBS_MODE_EN != 0xffff) {
			/*Set PRBS Timer Duration Control to maximum 6.7ms in VR_PMA_KRTR_PRBS_CTRL1 Register*/
			wdata = CL72_KRTR_PRBS_MODE_EN;
			txgbe_wr32_epcs(hw, 0x18005, wdata);
			/*Set PRBS Timer Duration Control to maximum 6.7ms in VR_PMA_KRTR_PRBS_CTRL1 Register*/
			wdata = 0xFFFF;
			txgbe_wr32_epcs(hw, 0x18004, wdata);

			/*Enable PRBS Mode to determine KR Training Status by setting Bit 0 of VR_PMA_KRTR_PRBS_CTRL0 Register*/
			wdata = 0;
			set_fields(&wdata, 0, 0, 1);
		}

#ifdef CL72_KRTR_PRBS31_EN
		/*Enable PRBS31 as the KR Training Pattern by setting Bit 1 of VR_PMA_KRTR_PRBS_CTRL0 Register*/
		set_fields(&wdata, 1, 1, 1);
#endif /*#ifdef CL72_KRTR_PRBS31_EN*/
		txgbe_wr32_epcs(hw, 0x18003, wdata);
		status |= read_phy_lane_txeq(0, adapter, 0, 0);
	} else {
		if (KR_MODE) e_dev_info("\nInvalid setting for Clause 72 KR Training!!!\n");
		return -1;
	}

	/*Enable the Clause 72 start-up protocol by setting Bit 1 of SR_PMA_KR_PMD_CTRL Register.
	**Restart the Clause 72 start-up protocol by setting Bit 0 of SR_PMA_KR_PMD_CTRL Register*/
	wdata = enable;
	txgbe_wr32_epcs(hw, 0x10096, wdata);
	return status;
}

int chk_cl72_krtr_status(struct txgbe_adapter *adapter)
{
	int status = 0;
	unsigned int addr, rdata, rdata1;
	unsigned int timer = 0, times = 0;
	struct txgbe_hw *hw = &adapter->hw;

	times = KR_POLLING?35:20;

	/*While loop to check clause 72 KR training status*/
	while(timer++ < times) { 
		//Get the latest received coefficient update or status
		rdata = 0;
		addr  = 0x010098;
		rdata = txgbe_rd32_epcs(hw, addr);
		if (KR_MODE) e_dev_info("SR PMA MMD 10GBASE-KR LP Coefficient Update Register: 0x%x\n", rdata);

		rdata = 0;
		addr  = 0x010099;
		rdata = txgbe_rd32_epcs(hw, addr);
		if (KR_MODE) e_dev_info("SR PMA MMD 10GBASE-KR LP Coefficient Status Register: 0x%x\n", rdata);

		rdata = 0;
		addr  = 0x01009a;
		rdata = txgbe_rd32_epcs(hw, addr);
		if (KR_MODE) e_dev_info("SR PMA MMD 10GBASE-KR LD Coefficient Update: 0x%x\n", rdata);

		rdata = 0;
		addr  = 0x01009b;
		rdata = txgbe_rd32_epcs(hw, addr);
		if (KR_MODE) e_dev_info(" SR PMA MMD 10GBASE-KR LD Coefficient Status: 0x%x\n", rdata);

		rdata = 0;
		addr  = 0x010097; 
		rdata = txgbe_rd32_epcs(hw, addr);
		if (KR_MODE) e_dev_info("SR PMA MMD 10GBASE-KR Status Register: 0x%x\n", rdata);
		if (KR_MODE) e_dev_info("  Training Failure         (bit3): %d\n", ((rdata >> 3) & 0x01));
		if (KR_MODE) e_dev_info("  Start-Up Protocol Status (bit2): %d\n", ((rdata >> 2) & 0x01));
		if (KR_MODE) e_dev_info("  Frame Lock               (bit1): %d\n", ((rdata >> 1) & 0x01));
		if (KR_MODE) e_dev_info("  Receiver Status          (bit0): %d\n", ((rdata >> 0) & 0x01));

		rdata1 = txgbe_rd32_epcs(hw, 0x10099) & 0x8000;
		if (rdata1 == 0x8000) {
			adapter->flags2 |= KR;
			if (KR_MODE) e_dev_info("TEST Coefficient Status Register: 0x%x\n", rdata);
		}
		/*If bit3 is set, Training is completed with failure*/
		if ((rdata >> 3) & 0x01) {
	    		if (KR_MODE) e_dev_info("Training is completed with failure!!!\n");
	    		status |= read_phy_lane_txeq(0, adapter, 0, 0);
	    		return status;
		}

		/*If bit0 is set, Receiver trained and ready to receive data*/
		if ((rdata >> 0) & 0x01) {
	    		if (KR_MODE) e_dev_info("Receiver trained and ready to receive data ^_^\n");
	    		status |= read_phy_lane_txeq(0, adapter, 0, 0);
	    		return status;
		}

		msleep(20);
    	} 

    	if (KR_MODE) e_dev_info("ERROR: Check Clause 72 KR Training Complete Timeout!!!\n");

    	return status;
}

int handle_bkp_an73_flow(unsigned char bp_link_mode, struct txgbe_adapter *adapter)
{
	int status = 0;
	unsigned int timer = 0;
	unsigned int addr, data;
	bkpan73ability tBkpAn73Ability , tLpBkpAn73Ability ;
	u32 i = 0;
	u32 rdata = 0;
	u32 rdata1 = 0;
	struct txgbe_hw *hw = &adapter->hw;
	tBkpAn73Ability.currentLinkMode = bp_link_mode;

	if (KR_MODE) e_dev_info("HandleBkpAn73Flow() \n");
	if (KR_MODE) e_dev_info("---------------------------------\n");

	if (adapter->an73_mode == 0) {
		txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x0);
		txgbe_wr32_epcs(hw, TXGBE_VR_AN_KR_MODE_CL, 0x0);
	}
	
	/*Check the FEC and KR Training for KR mode*/
	if ( 1 )
	{
		//FEC handling
		if (KR_MODE) e_dev_info("<3.3>. Check the FEC for KR mode ...\n");
		tBkpAn73Ability.fecAbility = 0x03;
		tLpBkpAn73Ability.fecAbility = 0x03;
		if ((tBkpAn73Ability.fecAbility & tLpBkpAn73Ability.fecAbility)  == 0x03)
		{
			if (KR_MODE) e_dev_info("Enable the Backplane KR FEC ...\n");
			//Write 1 to SR_PMA_KR_FEC_CTRL bit0 to enable the FEC
			data = 1;
			addr = 0x100ab; //SR_PMA_KR_FEC_CTRL 
			txgbe_wr32_epcs(hw, addr, data);
		} else {
			if (KR_MODE) e_dev_info("Backplane KR FEC is disabled.\n");
		}
#ifdef CL72_KR_TRAINING_ON
		for (i = 0; i < 2; i++) {
			if (KR_MODE) e_dev_info("\n<3.4>. Check the CL72 KR Training for KR mode ...\n");
			if  (KR_MODE)printk("===================%d=======================\n", i);
			status |= en_cl72_krtr(3, adapter);

			if (KR_MODE) e_dev_info("\nCheck the Clause 72 KR Training status ...\n");
			status |= chk_cl72_krtr_status(adapter);
			
			rdata = txgbe_rd32_epcs(hw, 0x10099) & 0x8000;
			if (KR_MODE) e_dev_info("SR PMA MMD 10GBASE-KR LP Coefficient Status Register: 0x%x\n", rdata);
			rdata1 = txgbe_rd32_epcs(hw, 0x1009b) & 0x8000;
			if (KR_MODE) e_dev_info("SR PMA MMD 10GBASE-KR LP Coefficient Status Register: 0x%x\n", rdata1);
			if (KR_POLLING == 0) {
				if(adapter->flags2 & KR){
					rdata = 0x8000;
					adapter->flags2 &= ~KR;
				}
			}
			if( (rdata == 0x8000) & (rdata1 == 0x8000)){
				if (KR_MODE) e_dev_info("====================out===========================\n");
				status |= en_cl72_krtr(1, adapter);
				clr_bkp_an73_int(2, 0, adapter);
				clr_bkp_an73_int(1, 0, adapter);
				clr_bkp_an73_int(0, 0, adapter);
				while(timer++ < 10) { 
					rdata = txgbe_rd32_epcs(hw, 0x30020);
					rdata = rdata & 0x1000;
					if (rdata  == 0x1000)
					{
						if (KR_MODE) e_dev_info("\nINT_AN_INT_CMPLT =1, AN73 Done Success.\n");
						e_dev_info("AN73 Done Success.\n");
						txgbe_wr32_epcs(hw, TXGBE_SR_AN_MMD_CTL, 0x0000);
						txgbe_wr32_epcs(hw, TXGBE_VR_AN_KR_MODE_CL, 0x0);
						return 0;
					}
					msleep(10);
				} 

				return 0;
			}	

			status |= en_cl72_krtr(1, adapter);
		}
#endif 
	}
	clr_bkp_an73_int(0, 0, adapter);
	clr_bkp_an73_int(1, 0, adapter);
	clr_bkp_an73_int(2, 0, adapter);

	return status;
}

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2015 - 2025 Beijing WangXun Technology Co., Ltd. */

#ifndef _TXGBE_BP_H_
#define _TXGBE_BP_H_

#include "txgbe.h"
#include "txgbe_type.h"
#include "txgbe_hw.h"

/* Backplane AN73 Base Page Ability struct*/
struct bkpan73ability {
	unsigned int next_page;    //Next Page (bit0)
	unsigned int link_ability; //Link Ability (bit[7:0])
	unsigned int fec_ability;  //FEC Request (bit1), FEC Enable  (bit0)
	unsigned int cu_linkmode; //current link mode for local device
};

enum ability_filed_encding {
	ABILITY_1000BASE_KX,
	ABILITY_10GBASE_KX4,
	ABILITY_10GBASE_KR,
	ABILITY_40GBASE_KR4,
	ABILITY_40GBASE_CR4,
	ABILITY_100GBASE_CR10,
	ABILITY_100GBASE_KP4,
	ABILITY_100GBASE_KR4,
	ABILITY_100GBASE_CR4,
	ABILITY_25GBASE_KRCR_S,
	ABILITY_25GBASE_KRCR,
	ABILITY_MAX,
};

#define KR_MODE 0

#define kr_dbg(KR_MODE, fmt, arg...) \
	do { \
		if (KR_MODE) \
			e_dev_info(fmt, ##arg); \
	} while (0)

void txgbe_bp_down_event(struct txgbe_adapter *adapter);
void txgbe_bp_watchdog_event(struct txgbe_adapter *adapter);
int txgbe_bp_mode_setting(struct txgbe_adapter *adapter);
void txgbe_bp_close_protect(struct txgbe_adapter *adapter);
int handle_bkp_an73_flow(unsigned char bp_link_mode, struct txgbe_adapter *adapter);
int get_bkp_an73_ability(struct bkpan73ability *pt_bkp_an73_ability,
			 unsigned char by_link_partner,
							struct txgbe_adapter *adapter);
int chk_bkp_an73_ability(struct bkpan73ability tbkp_an73_ability,
			 struct bkpan73ability tlpbkp_an73_ability,
							struct txgbe_adapter *adapter);
#endif


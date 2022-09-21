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
 *
 * based on ixgbe_dcb.c, Copyright(c) 1999 - 2017 Intel Corporation.
 * Contact Information:
 * Linux NICS <linux.nics@intel.com>
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 */

#include "txgbe_type.h"
#include "txgbe_dcb.h"
#include "txgbe.h"

/*
 * txgbe_dcb_calculate_tc_credits - This calculates the ieee traffic class
 * credits from the configured bandwidth percentages. Credits
 * are the smallest unit programmable into the underlying
 * hardware. The IEEE 802.1Qaz specification do not use bandwidth
 * groups so this is much simplified from the CEE case.
 */
s32 txgbe_dcb_calculate_tc_credits(u8 *bw, u16 *refill, u16 *max,
				   int max_frame_size)
{
	int min_percent = 100;
	int min_credit, multiplier;
	int i;

	min_credit = ((max_frame_size / 2) + TXGBE_DCB_CREDIT_QUANTUM - 1) /
			TXGBE_DCB_CREDIT_QUANTUM;

	for (i = 0; i < TXGBE_DCB_MAX_TRAFFIC_CLASS; i++) {
		if (bw[i] < min_percent && bw[i])
			min_percent = bw[i];
	}

	multiplier = (min_credit / min_percent) + 1;

	/* Find out the hw credits for each TC */
	for (i = 0; i < TXGBE_DCB_MAX_TRAFFIC_CLASS; i++) {
		int val = min(bw[i] * multiplier, TXGBE_DCB_MAX_CREDIT_REFILL);

		if (val < min_credit)
			val = min_credit;
		refill[i] = (u16)val;

		max[i] = (u16)(bw[i] ? (bw[i]*TXGBE_DCB_MAX_CREDIT)/100 : min_credit);
	}

	return 0;
}

/**
 * txgbe_dcb_calculate_tc_credits_cee - Calculates traffic class credits
 * @txgbe_dcb_config: Struct containing DCB settings.
 * @direction: Configuring either Tx or Rx.
 *
 * This function calculates the credits allocated to each traffic class.
 * It should be called only after the rules are checked by
 * txgbe_dcb_check_config_cee().
 */
s32 txgbe_dcb_calculate_tc_credits_cee(struct txgbe_hw *hw,
				   struct txgbe_dcb_config *dcb_config,
				   u32 max_frame_size, u8 direction)
{
	struct txgbe_dcb_tc_path *p;
	u32 min_multiplier      = 0;
	u16 min_percent         = 100;
	s32 ret_val =           0;
	/* Initialization values default for Tx settings */
	u32 min_credit          = 0;
	u32 credit_refill       = 0;
	u32 credit_max          = 0;
	u16 link_percentage     = 0;
	u8  bw_percent          = 0;
	u8  i;

	UNREFERENCED_PARAMETER(hw);

	if (dcb_config == NULL) {
		ret_val = TXGBE_ERR_CONFIG;
		goto out;
	}

	min_credit = ((max_frame_size / 2) + TXGBE_DCB_CREDIT_QUANTUM - 1) /
		     TXGBE_DCB_CREDIT_QUANTUM;

	/* Find smallest link percentage */
	for (i = 0; i < TXGBE_DCB_MAX_TRAFFIC_CLASS; i++) {
		p = &dcb_config->tc_config[i].path[direction];
		bw_percent = dcb_config->bw_percentage[direction][p->bwg_id];
		link_percentage = p->bwg_percent;

		link_percentage = (link_percentage * bw_percent) / 100;

		if (link_percentage && link_percentage < min_percent)
			min_percent = link_percentage;
	}

	/*
	 * The ratio between traffic classes will control the bandwidth
	 * percentages seen on the wire. To calculate this ratio we use
	 * a multiplier. It is required that the refill credits must be
	 * larger than the max frame size so here we find the smallest
	 * multiplier that will allow all bandwidth percentages to be
	 * greater than the max frame size.
	 */
	min_multiplier = (min_credit / min_percent) + 1;

	/* Find out the link percentage for each TC first */
	for (i = 0; i < TXGBE_DCB_MAX_TRAFFIC_CLASS; i++) {
		p = &dcb_config->tc_config[i].path[direction];
		bw_percent = dcb_config->bw_percentage[direction][p->bwg_id];

		link_percentage = p->bwg_percent;
		/* Must be careful of integer division for very small nums */
		link_percentage = (link_percentage * bw_percent) / 100;
		if (p->bwg_percent > 0 && link_percentage == 0)
			link_percentage = 1;

		/* Save link_percentage for reference */
		p->link_percent = (u8)link_percentage;

		/* Calculate credit refill ratio using multiplier */
		credit_refill = min(link_percentage * min_multiplier,
				    (u32)TXGBE_DCB_MAX_CREDIT_REFILL);

		/* Refill at least minimum credit */
		if (credit_refill < min_credit)
			credit_refill = min_credit;

		p->data_credits_refill = (u16)credit_refill;

		/* Calculate maximum credit for the TC */
		credit_max = (link_percentage * TXGBE_DCB_MAX_CREDIT) / 100;

		/*
		 * Adjustment based on rule checking, if the percentage
		 * of a TC is too small, the maximum credit may not be
		 * enough to send out a jumbo frame in data plane arbitration.
		 */
		if (credit_max < min_credit)
			credit_max = min_credit;

		if (direction == TXGBE_DCB_TX_CONFIG) {
			/*
			 * Adjustment based on rule checking, if the
			 * percentage of a TC is too small, the maximum
			 * credit may not be enough to send out a TSO
			 * packet in descriptor plane arbitration.
			 */

			dcb_config->tc_config[i].desc_credits_max =
								(u16)credit_max;
		}

		p->data_credits_max = (u16)credit_max;
	}

out:
	return ret_val;
}

/**
 * txgbe_dcb_unpack_pfc_cee - Unpack dcb_config PFC info
 * @cfg: dcb configuration to unpack into hardware consumable fields
 * @map: user priority to traffic class map
 * @pfc_up: u8 to store user priority PFC bitmask
 *
 * This unpacks the dcb configuration PFC info which is stored per
 * traffic class into a 8bit user priority bitmask that can be
 * consumed by hardware routines. The priority to tc map must be
 * updated before calling this routine to use current up-to maps.
 */
void txgbe_dcb_unpack_pfc_cee(struct txgbe_dcb_config *cfg, u8 *map, u8 *pfc_up)
{
	struct txgbe_dcb_tc_config *tc_config = &cfg->tc_config[0];
	int up;

	/*
	 * If the TC for this user priority has PFC enabled then set the
	 * matching bit in 'pfc_up' to reflect that PFC is enabled.
	 */
	for (*pfc_up = 0, up = 0; up < TXGBE_DCB_MAX_USER_PRIORITY; up++) {
		if (tc_config[map[up]].pfc != txgbe_dcb_pfc_disabled)
			*pfc_up |= 1 << up;
	}
}

void txgbe_dcb_unpack_refill_cee(struct txgbe_dcb_config *cfg, int direction,
			     u16 *refill)
{
	struct txgbe_dcb_tc_config *tc_config = &cfg->tc_config[0];
	int tc;

	for (tc = 0; tc < TXGBE_DCB_MAX_TRAFFIC_CLASS; tc++)
		refill[tc] = tc_config[tc].path[direction].data_credits_refill;
}

void txgbe_dcb_unpack_max_cee(struct txgbe_dcb_config *cfg, u16 *max)
{
	struct txgbe_dcb_tc_config *tc_config = &cfg->tc_config[0];
	int tc;

	for (tc = 0; tc < TXGBE_DCB_MAX_TRAFFIC_CLASS; tc++)
		max[tc] = tc_config[tc].desc_credits_max;
}

void txgbe_dcb_unpack_bwgid_cee(struct txgbe_dcb_config *cfg, int direction,
			    u8 *bwgid)
{
	struct txgbe_dcb_tc_config *tc_config = &cfg->tc_config[0];
	int tc;

	for (tc = 0; tc < TXGBE_DCB_MAX_TRAFFIC_CLASS; tc++)
		bwgid[tc] = tc_config[tc].path[direction].bwg_id;
}

void txgbe_dcb_unpack_tsa_cee(struct txgbe_dcb_config *cfg, int direction,
			   u8 *tsa)
{
	struct txgbe_dcb_tc_config *tc_config = &cfg->tc_config[0];
	int tc;

	for (tc = 0; tc < TXGBE_DCB_MAX_TRAFFIC_CLASS; tc++)
		tsa[tc] = tc_config[tc].path[direction].tsa;
}

u8 txgbe_dcb_get_tc_from_up(struct txgbe_dcb_config *cfg, int direction, u8 up)
{
	struct txgbe_dcb_tc_config *tc_config = &cfg->tc_config[0];
	u8 prio_mask = 1 << up;
	u8 tc = cfg->num_tcs.pg_tcs;

	/* If tc is 0 then DCB is likely not enabled or supported */
	if (!tc)
		goto out;

	/*
	 * Test from maximum TC to 1 and report the first match we find.  If
	 * we find no match we can assume that the TC is 0 since the TC must
	 * be set for all user priorities
	 */
	for (tc--; tc; tc--) {
		if (prio_mask & tc_config[tc].path[direction].up_to_tc_bitmap)
			break;
	}
out:
	return tc;
}

void txgbe_dcb_unpack_map_cee(struct txgbe_dcb_config *cfg, int direction,
			      u8 *map)
{
	u8 up;

	for (up = 0; up < TXGBE_DCB_MAX_USER_PRIORITY; up++)
		map[up] = txgbe_dcb_get_tc_from_up(cfg, direction, up);
}










/**
 * txgbe_dcb_config_tc_stats - Config traffic class statistics
 * @hw: pointer to hardware structure
 *
 * Configure queue statistics registers, all queues belonging to same traffic
 * class uses a single set of queue statistics counters.
 */
s32 txgbe_dcb_config_tc_stats(struct txgbe_hw *hw,
			      struct txgbe_dcb_config *dcb_config)
{
	UNREFERENCED_PARAMETER(hw);
	UNREFERENCED_PARAMETER(dcb_config);
	return 0;
}

/**
 * txgbe_dcb_hw_config_cee - Config and enable DCB
 * @hw: pointer to hardware structure
 * @dcb_config: pointer to txgbe_dcb_config structure
 *
 * Configure dcb settings and enable dcb mode.
 */
s32 txgbe_dcb_hw_config_cee(struct txgbe_hw *hw,
			struct txgbe_dcb_config *dcb_config)
{
	s32 ret = TXGBE_NOT_IMPLEMENTED;
	u8 pfc_en;
	u8 tsa[TXGBE_DCB_MAX_TRAFFIC_CLASS];
	u8 bwgid[TXGBE_DCB_MAX_TRAFFIC_CLASS];
	u8 map[TXGBE_DCB_MAX_USER_PRIORITY] = { 0 };
	u16 refill[TXGBE_DCB_MAX_TRAFFIC_CLASS];
	u16 max[TXGBE_DCB_MAX_TRAFFIC_CLASS];

	/* Unpack CEE standard containers */
	txgbe_dcb_unpack_refill_cee(dcb_config, TXGBE_DCB_TX_CONFIG, refill);
	txgbe_dcb_unpack_max_cee(dcb_config, max);
	txgbe_dcb_unpack_bwgid_cee(dcb_config, TXGBE_DCB_TX_CONFIG, bwgid);
	txgbe_dcb_unpack_tsa_cee(dcb_config, TXGBE_DCB_TX_CONFIG, tsa);
	txgbe_dcb_unpack_map_cee(dcb_config, TXGBE_DCB_TX_CONFIG, map);


	txgbe_dcb_config(hw, dcb_config);
	ret = txgbe_dcb_hw_config(hw,
					refill, max, bwgid,
					tsa, map);

	txgbe_dcb_config_tc_stats(hw, dcb_config);


	if (!ret && dcb_config->pfc_mode_enable) {
		txgbe_dcb_unpack_pfc_cee(dcb_config, map, &pfc_en);
		ret = txgbe_dcb_config_pfc(hw, pfc_en, map);
	}

	return ret;
}

/* Helper routines to abstract HW specifics from DCB netlink ops */
s32 txgbe_dcb_config_pfc(struct txgbe_hw *hw, u8 pfc_en, u8 *map)
{
	int ret = TXGBE_ERR_PARAM;

	u32 i, j, fcrtl, reg;
	u8 max_tc = 0;

	/* Enable Transmit Priority Flow Control */
	wr32(hw, TXGBE_RDB_RFCC, TXGBE_RDB_RFCC_RFCE_PRIORITY);

	/* Enable Receive Priority Flow Control */
	reg = 0;

	if (pfc_en)
		reg |= (TXGBE_MAC_RX_FLOW_CTRL_PFCE | 0x1);

	wr32(hw, TXGBE_MAC_RX_FLOW_CTRL, reg);

	for (i = 0; i < TXGBE_DCB_MAX_USER_PRIORITY; i++) {
		if (map[i] > max_tc)
			max_tc = map[i];
	}


	/* Configure PFC Tx thresholds per TC */
	for (i = 0; i <= max_tc; i++) {
		int enabled = 0;

		for (j = 0; j < TXGBE_DCB_MAX_USER_PRIORITY; j++) {
			if ((map[j] == i) && (pfc_en & (1 << j))) {
				enabled = 1;
				break;
			}
		}

		if (enabled) {
			reg = (hw->fc.high_water[i] << 10) |
			      TXGBE_RDB_RFCH_XOFFE;
			fcrtl = (hw->fc.low_water[i] << 10) |
				TXGBE_RDB_RFCL_XONE;
			wr32(hw, TXGBE_RDB_RFCH(i), fcrtl);
		} else {
			/*
			 * In order to prevent Tx hangs when the internal Tx
			 * switch is enabled we must set the high water mark
			 * to the Rx packet buffer size - 24KB.  This allows
			 * the Tx switch to function even under heavy Rx
			 * workloads.
			 */
			reg = rd32(hw, TXGBE_RDB_PB_SZ(i));
			wr32(hw, TXGBE_RDB_RFCL(i), 0);
		}

		wr32(hw, TXGBE_RDB_RFCH(i), reg);
	}

	for (; i < TXGBE_DCB_MAX_TRAFFIC_CLASS; i++) {
		wr32(hw, TXGBE_RDB_RFCL(i), 0);
		wr32(hw, TXGBE_RDB_RFCH(i), 0);
	}

	/* Configure pause time (2 TCs per register) */
	reg = hw->fc.pause_time | (hw->fc.pause_time << 16);
	for (i = 0; i < (TXGBE_DCB_MAX_TRAFFIC_CLASS / 2); i++)
		wr32(hw, TXGBE_RDB_RFCV(i), reg);

	/* Configure flow control refresh threshold value */
	wr32(hw, TXGBE_RDB_RFCRT, hw->fc.pause_time / 2);

	return ret;
}

s32 txgbe_dcb_hw_config(struct txgbe_hw *hw, u16 *refill, u16 *max,
			    u8 *bwg_id, u8 *tsa, u8 *map)
{
	txgbe_dcb_config_rx_arbiter(hw, refill, max, bwg_id,
					  tsa, map);
	txgbe_dcb_config_tx_desc_arbiter(hw, refill, max,
					      bwg_id, tsa);
	txgbe_dcb_config_tx_data_arbiter(hw, refill, max,
					      bwg_id, tsa, map);

	return 0;
}

/**
 * txgbe_dcb_config_rx_arbiter - Config Rx Data arbiter
 * @hw: pointer to hardware structure
 * @dcb_config: pointer to txgbe_dcb_config structure
 *
 * Configure Rx Packet Arbiter and credits for each traffic class.
 */
s32 txgbe_dcb_config_rx_arbiter(struct txgbe_hw *hw, u16 *refill,
				      u16 *max, u8 *bwg_id, u8 *tsa,
				      u8 *map)
{
	u32 reg = 0;
	u32 credit_refill = 0;
	u32 credit_max = 0;
	u8  i = 0;

	/*
	 * Disable the arbiter before changing parameters
	 * (always enable recycle mode; WSP)
	 */
	reg = TXGBE_RDM_ARB_CTL_RRM | TXGBE_RDM_ARB_CTL_RAC |
	      TXGBE_RDM_ARB_CTL_ARBDIS;
	wr32(hw, TXGBE_RDM_ARB_CTL, reg);

	/*
	 * map all UPs to TCs. up_to_tc_bitmap for each TC has corresponding
	 * bits sets for the UPs that needs to be mappped to that TC.
	 * e.g if priorities 6 and 7 are to be mapped to a TC then the
	 * up_to_tc_bitmap value for that TC will be 11000000 in binary.
	 */
	reg = 0;
	for (i = 0; i < TXGBE_DCB_MAX_USER_PRIORITY; i++)
		reg |= (map[i] << (i * TXGBE_RDB_UP2TC_UP_SHIFT));

	wr32(hw, TXGBE_RDB_UP2TC, reg);

	/* Configure traffic class credits and priority */
	for (i = 0; i < TXGBE_DCB_MAX_TRAFFIC_CLASS; i++) {
		credit_refill = refill[i];
		credit_max = max[i];
		reg = credit_refill |
		      (credit_max << TXGBE_RDM_ARB_CFG_MCL_SHIFT);

		reg |= (u32)(bwg_id[i]) << TXGBE_RDM_ARB_CFG_BWG_SHIFT;

		if (tsa[i] == txgbe_dcb_tsa_strict)
			reg |= TXGBE_RDM_ARB_CFG_LSP;

		wr32(hw, TXGBE_RDM_ARB_CFG(i), reg);
	}

	/*
	 * Configure Rx packet plane (recycle mode; WSP) and
	 * enable arbiter
	 */
	reg = TXGBE_RDM_ARB_CTL_RRM | TXGBE_RDM_ARB_CTL_RAC;
	wr32(hw, TXGBE_RDM_ARB_CTL, reg);

	return 0;
}

/**
 * txgbe_dcb_config_tx_desc_arbiter - Config Tx Desc. arbiter
 * @hw: pointer to hardware structure
 * @dcb_config: pointer to txgbe_dcb_config structure
 *
 * Configure Tx Descriptor Arbiter and credits for each traffic class.
 */
s32 txgbe_dcb_config_tx_desc_arbiter(struct txgbe_hw *hw, u16 *refill,
					      u16 *max, u8 *bwg_id, u8 *tsa)
{
	u32 reg, max_credits;
	u8  i;

	/* Clear the per-Tx queue credits; we use per-TC instead */
	for (i = 0; i < 128; i++) {
		wr32(hw, TXGBE_TDM_VM_CREDIT(i), 0);
	}

	/* Configure traffic class credits and priority */
	for (i = 0; i < TXGBE_DCB_MAX_TRAFFIC_CLASS; i++) {
		max_credits = max[i];
		reg = max_credits << TXGBE_TDM_PBWARB_CFG_MCL_SHIFT;
		reg |= refill[i];
		reg |= (u32)(bwg_id[i]) << TXGBE_TDM_PBWARB_CFG_BWG_SHIFT;

		if (tsa[i] == txgbe_dcb_tsa_group_strict_cee)
			reg |= TXGBE_TDM_PBWARB_CFG_GSP;

		if (tsa[i] == txgbe_dcb_tsa_strict)
			reg |= TXGBE_TDM_PBWARB_CFG_LSP;

		wr32(hw, TXGBE_TDM_PBWARB_CFG(i), reg);
	}

	/*
	 * Configure Tx descriptor plane (recycle mode; WSP) and
	 * enable arbiter
	 */
	reg = TXGBE_TDM_PBWARB_CTL_TDPAC | TXGBE_TDM_PBWARB_CTL_TDRM;
	wr32(hw, TXGBE_TDM_PBWARB_CTL, reg);

	return 0;
}

/**
 * txgbe_dcb_config_tx_data_arbiter - Config Tx Data arbiter
 * @hw: pointer to hardware structure
 * @dcb_config: pointer to txgbe_dcb_config structure
 *
 * Configure Tx Packet Arbiter and credits for each traffic class.
 */
s32 txgbe_dcb_config_tx_data_arbiter(struct txgbe_hw *hw, u16 *refill,
					      u16 *max, u8 *bwg_id, u8 *tsa,
					      u8 *map)
{
	u32 reg;
	u8 i;

	/*
	 * Disable the arbiter before changing parameters
	 * (always enable recycle mode; SP; arb delay)
	 */
	reg = TXGBE_TDB_PBRARB_CTL_TPPAC | TXGBE_TDB_PBRARB_CTL_TPRM |
	      TXGBE_RTTPCS_ARBDIS;
	wr32(hw, TXGBE_TDB_PBRARB_CTL, reg);

	/*
	 * map all UPs to TCs. up_to_tc_bitmap for each TC has corresponding
	 * bits sets for the UPs that needs to be mappped to that TC.
	 * e.g if priorities 6 and 7 are to be mapped to a TC then the
	 * up_to_tc_bitmap value for that TC will be 11000000 in binary.
	 */
	reg = 0;
	for (i = 0; i < TXGBE_DCB_MAX_USER_PRIORITY; i++)
		reg |= (map[i] << (i * TXGBE_TDB_UP2TC_UP_SHIFT));

	wr32(hw, TXGBE_TDB_UP2TC, reg);

	/* Configure traffic class credits and priority */
	for (i = 0; i < TXGBE_DCB_MAX_TRAFFIC_CLASS; i++) {
		reg = refill[i];
		reg |= (u32)(max[i]) << TXGBE_TDB_PBRARB_CFG_MCL_SHIFT;
		reg |= (u32)(bwg_id[i]) << TXGBE_TDB_PBRARB_CFG_BWG_SHIFT;

		if (tsa[i] == txgbe_dcb_tsa_group_strict_cee)
			reg |= TXGBE_TDB_PBRARB_CFG_GSP;

		if (tsa[i] == txgbe_dcb_tsa_strict)
			reg |= TXGBE_TDB_PBRARB_CFG_LSP;

		wr32(hw, TXGBE_TDB_PBRARB_CFG(i), reg);
	}

	/*
	 * Configure Tx packet plane (recycle mode; SP; arb delay) and
	 * enable arbiter
	 */
	reg = TXGBE_TDB_PBRARB_CTL_TPPAC | TXGBE_TDB_PBRARB_CTL_TPRM;
	wr32(hw, TXGBE_TDB_PBRARB_CTL, reg);

	return 0;
}

/**
 * txgbe_dcb_config - Configure general DCB parameters
 * @hw: pointer to hardware structure
 * @dcb_config: pointer to txgbe_dcb_config structure
 *
 * Configure general DCB parameters.
 */
s32 txgbe_dcb_config(struct txgbe_hw *hw,
			   struct txgbe_dcb_config *dcb_config)
{
	u32 n, value;

	struct txgbe_adapter *adapter = hw->back;

	if (dcb_config->vt_mode)
		adapter->flags |= TXGBE_FLAG_VMDQ_ENABLED;
	else
		adapter->flags &= ~TXGBE_FLAG_VMDQ_ENABLED;

	if (adapter->flags & TXGBE_FLAG_VMDQ_ENABLED) {
		if (dcb_config->num_tcs.pg_tcs == 8)
			/* 8 TCs */
			value = TXGBE_CFG_PORT_CTL_NUM_TC_8 |
			      TXGBE_CFG_PORT_CTL_NUM_VT_16 |
			      TXGBE_CFG_PORT_CTL_DCB_EN;
		else if (dcb_config->num_tcs.pg_tcs == 4)
			/* 4 TCs */
			value = TXGBE_CFG_PORT_CTL_NUM_TC_4 |
			      TXGBE_CFG_PORT_CTL_NUM_VT_32 |
			      TXGBE_CFG_PORT_CTL_DCB_EN;
		else if (adapter->ring_feature[RING_F_RSS].indices == 4)
			value = TXGBE_CFG_PORT_CTL_NUM_VT_32;
		else /* adapter->ring_feature[RING_F_RSS].indices <= 2 */
			value = TXGBE_CFG_PORT_CTL_NUM_VT_64;
	} else {
		if (dcb_config->num_tcs.pg_tcs == 8)
			value = TXGBE_CFG_PORT_CTL_NUM_TC_8 |
			      TXGBE_CFG_PORT_CTL_DCB_EN;
		else if (dcb_config->num_tcs.pg_tcs == 4)
			value = TXGBE_CFG_PORT_CTL_NUM_TC_4 |
			      TXGBE_CFG_PORT_CTL_DCB_EN;
		else
			value = 0;
	}

	value |= TXGBE_CFG_PORT_CTL_D_VLAN | TXGBE_CFG_PORT_CTL_QINQ;
	wr32m(hw, TXGBE_CFG_PORT_CTL,
		TXGBE_CFG_PORT_CTL_NUM_TC_MASK |
		TXGBE_CFG_PORT_CTL_NUM_VT_MASK |
		TXGBE_CFG_PORT_CTL_DCB_EN |
		TXGBE_CFG_PORT_CTL_D_VLAN |
		TXGBE_CFG_PORT_CTL_QINQ,
		value);

	/* Disable drop for all queues */
	for (n = 0; n < 4; n++) {
		wr32(hw, TXGBE_RDM_PF_QDE(n), 0x0);
	}

	return 0;
}




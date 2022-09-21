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
 * based on ixgbe_dcb.h, Copyright(c) 1999 - 2017 Intel Corporation.
 * Contact Information:
 * Linux NICS <linux.nics@intel.com>
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 */


#ifndef _TXGBE_DCB_H_
#define _TXGBE_DCB_H_

#include "txgbe_type.h"

/* DCB defines */
/* DCB credit calculation defines */
#define TXGBE_DCB_CREDIT_QUANTUM        64
#define TXGBE_DCB_MAX_CREDIT_REFILL     200   /* 200 * 64B = 12800B */
#define TXGBE_DCB_MAX_TSO_SIZE          (32 * 1024) /* Max TSO pkt size in DCB*/
#define TXGBE_DCB_MAX_CREDIT            (2 * TXGBE_DCB_MAX_CREDIT_REFILL)

/* 513 for 32KB TSO packet */
#define TXGBE_DCB_MIN_TSO_CREDIT        \
	((TXGBE_DCB_MAX_TSO_SIZE / TXGBE_DCB_CREDIT_QUANTUM) + 1)

/* DCB configuration defines */
#define TXGBE_DCB_MAX_USER_PRIORITY     8
#define TXGBE_DCB_MAX_BW_GROUP          8
#define TXGBE_DCB_BW_PERCENT            100

#define TXGBE_DCB_TX_CONFIG             0
#define TXGBE_DCB_RX_CONFIG             1

/* DCB capability defines */
#define TXGBE_DCB_PG_SUPPORT    0x00000001
#define TXGBE_DCB_PFC_SUPPORT   0x00000002
#define TXGBE_DCB_BCN_SUPPORT   0x00000004
#define TXGBE_DCB_UP2TC_SUPPORT 0x00000008
#define TXGBE_DCB_GSP_SUPPORT   0x00000010

/* DCB register definitions */
#define TXGBE_TDM_PBWARB_CTL_TDPAC      0x00000001 /* 0 Round Robin,
					    * 1 WSP - Weighted Strict Priority
					    */
#define TXGBE_TDM_PBWARB_CTL_TDRM       0x00000010 /* Transmit Recycle Mode */
#define TXGBE_TDM_PBWARB_CTL_ARBDIS     0x00000040 /* DCB arbiter disable */


/* Receive UP2TC mapping */
#define TXGBE_RDB_UP2TC_UP_SHIFT        4
#define TXGBE_RDB_UP2TC_UP_MASK         7
/* Transmit UP2TC mapping */
#define TXGBE_TDB_UP2TC_UP_SHIFT        4

#define TXGBE_RDM_ARB_CFG_MCL_SHIFT 12 /* Offset to Max Credit Limit setting */
#define TXGBE_RDM_ARB_CFG_BWG_SHIFT 9  /* Offset to BWG index */
#define TXGBE_RDM_ARB_CFG_GSP   0x40000000 /* GSP enable bit */
#define TXGBE_RDM_ARB_CFG_LSP   0x80000000 /* LSP enable bit */

/* RTRPCS Bit Masks */
#define TXGBE_RDM_ARB_CTL_RRM   0x00000002 /* Receive Recycle Mode enable */
/* Receive Arbitration Control: 0 Round Robin, 1 DFP */
#define TXGBE_RDM_ARB_CTL_RAC   0x00000004
#define TXGBE_RDM_ARB_CTL_ARBDIS        0x00000040 /* Arbitration disable bit */

/* RTTDT2C Bit Masks */
#define TXGBE_TDM_PBWARB_CFG_MCL_SHIFT  12
#define TXGBE_TDM_PBWARB_CFG_BWG_SHIFT  9
#define TXGBE_TDM_PBWARB_CFG_GSP        0x40000000
#define TXGBE_TDM_PBWARB_CFG_LSP        0x80000000

#define TXGBE_TDB_PBRARB_CFG_MCL_SHIFT  12
#define TXGBE_TDB_PBRARB_CFG_BWG_SHIFT  9
#define TXGBE_TDB_PBRARB_CFG_GSP        0x40000000
#define TXGBE_TDB_PBRARB_CFG_LSP        0x80000000

/* RTTPCS Bit Masks */
#define TXGBE_TDB_PBRARB_CTL_TPPAC      0x00000020 /* 0 Round Robin,
						    * 1 SP - Strict Priority
							*/
#define TXGBE_RTTPCS_ARBDIS     	0x00000040 /* Arbiter disable */
#define TXGBE_TDB_PBRARB_CTL_TPRM   0x00000100 /* Transmit Recycle Mode enable*/

#define TXGBE_TDM_PB_THRE_DCB   0xA /* THRESH value for DCB mode */


struct txgbe_dcb_support {
	u32 capabilities; /* DCB capabilities */

	/* Each bit represents a number of TCs configurable in the hw.
	 * If 8 traffic classes can be configured, the value is 0x80. */
	u8 traffic_classes;
	u8 pfc_traffic_classes;
};

enum txgbe_dcb_tsa {
	txgbe_dcb_tsa_ets = 0,
	txgbe_dcb_tsa_group_strict_cee,
	txgbe_dcb_tsa_strict
};

/* Traffic class bandwidth allocation per direction */
struct txgbe_dcb_tc_path {
	u8 bwg_id; /* Bandwidth Group (BWG) ID */
	u8 bwg_percent; /* % of BWG's bandwidth */
	u8 link_percent; /* % of link bandwidth */
	u8 up_to_tc_bitmap; /* User Priority to Traffic Class mapping */
	u16 data_credits_refill; /* Credit refill amount in 64B granularity */
	u16 data_credits_max; /* Max credits for a configured packet buffer
			       * in 64B granularity.*/
	enum txgbe_dcb_tsa tsa; /* Link or Group Strict Priority */
};

enum txgbe_dcb_pfc {
	txgbe_dcb_pfc_disabled = 0,
	txgbe_dcb_pfc_enabled,
	txgbe_dcb_pfc_enabled_txonly,
	txgbe_dcb_pfc_enabled_rxonly
};

/* Traffic class configuration */
struct txgbe_dcb_tc_config {
	struct txgbe_dcb_tc_path path[2]; /* One each for Tx/Rx */
	enum txgbe_dcb_pfc pfc; /* Class based flow control setting */

	u16 desc_credits_max; /* For Tx Descriptor arbitration */
	u8 tc; /* Traffic class (TC) */
};

enum txgbe_dcb_pba {
	/* PBA[0-7] each use 64KB FIFO */
	txgbe_dcb_pba_equal = PBA_STRATEGY_EQUAL,
	/* PBA[0-3] each use 80KB, PBA[4-7] each use 48KB */
	txgbe_dcb_pba_80_48 = PBA_STRATEGY_WEIGHTED
};

struct txgbe_dcb_num_tcs {
	u8 pg_tcs;
	u8 pfc_tcs;
};

struct txgbe_dcb_config {
	struct txgbe_dcb_tc_config tc_config[TXGBE_DCB_MAX_TRAFFIC_CLASS];
	struct txgbe_dcb_support support;
	struct txgbe_dcb_num_tcs num_tcs;
	u8 bw_percentage[2][TXGBE_DCB_MAX_BW_GROUP]; /* One each for Tx/Rx */
	bool pfc_mode_enable;
	bool round_robin_enable;

	enum txgbe_dcb_pba rx_pba_cfg;

	u32 dcb_cfg_version; /* Not used...OS-specific? */
	u32 link_speed; /* For bandwidth allocation validation purpose */
	bool vt_mode;
};

/* DCB driver APIs */

/* DCB credits calculation */
s32 txgbe_dcb_calculate_tc_credits(u8 *, u16 *, u16 *, int);
s32 txgbe_dcb_calculate_tc_credits_cee(struct txgbe_hw *,
				       struct txgbe_dcb_config *, u32, u8);

/* DCB PFC */
s32 txgbe_dcb_config_pfc(struct txgbe_hw *, u8, u8 *);

/* DCB stats */
s32 txgbe_dcb_config_tc_stats(struct txgbe_hw *,
								struct txgbe_dcb_config *);

/* DCB config arbiters */
s32 txgbe_dcb_config_tx_desc_arbiter(struct txgbe_hw *, u16 *, u16 *,
					   u8 *, u8 *);
s32 txgbe_dcb_config_tx_data_arbiter(struct txgbe_hw *, u16 *, u16 *,
					   u8 *, u8 *, u8 *);
s32 txgbe_dcb_config_rx_arbiter(struct txgbe_hw *, u16 *, u16 *, u8 *,
				      u8 *, u8 *);

/* DCB unpack routines */
void txgbe_dcb_unpack_pfc_cee(struct txgbe_dcb_config *, u8 *, u8 *);
void txgbe_dcb_unpack_refill_cee(struct txgbe_dcb_config *, int, u16 *);
void txgbe_dcb_unpack_max_cee(struct txgbe_dcb_config *, u16 *);
void txgbe_dcb_unpack_bwgid_cee(struct txgbe_dcb_config *, int, u8 *);
void txgbe_dcb_unpack_tsa_cee(struct txgbe_dcb_config *, int, u8 *);
void txgbe_dcb_unpack_map_cee(struct txgbe_dcb_config *, int, u8 *);
u8 txgbe_dcb_get_tc_from_up(struct txgbe_dcb_config *, int, u8);

/* DCB initialization */
s32 txgbe_dcb_config(struct txgbe_hw *,
			   struct txgbe_dcb_config *);
s32 txgbe_dcb_hw_config(struct txgbe_hw *, u16 *, u16 *, u8 *, u8 *, u8 *);
s32 txgbe_dcb_hw_config_cee(struct txgbe_hw *, struct txgbe_dcb_config *);
#endif /* _TXGBE_DCB_H_ */

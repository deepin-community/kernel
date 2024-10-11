/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef __RNP_DCB_H__
#define __RNP_DCB_H__
#include "rnp.h"

enum rnp_pause_low_thrsh {
	RNP_PAUSE_4_SLOT_TIME = 0,
	RNP_PAUSE_28_SLOT_TIME,
	RNP_PAUSE_36_SLOT_TIME,
	RNP_PAUSE_144_SLOT_TIME,
	RNP_PAUSE_256_SLOT_TIME,
};
/*Rx Flow Ctrl */
#define RNP_RX_RFE BIT(0) /* Receive Flow Control Enable */
#define RNP_UP BIT(1) /* Unicast Pause Packet Detect */
#define RNP_PFCE BIT(8) /*  Priority Based Flow Control Enable. */

/*Tx Flow Ctrl */
#define RNP_TX_FCB BIT(0) /* Tx Flow Control Busy. */
#define RNP_TX_TFE BIT(1) /* Transmit Flow Control Enable.*/
#define RNP_TX_PLT GENMASK(6, 4) /* Pause Low Threshold. */
#define RNP_DZPQ BIT(7) /*Disable Zero-Quanta Pause.*/
#define RNP_PT GENMASK(31, 16) /* Pause Time. */

#define RNP_DEFAULT_PAUSE_TIME (0x100) /* */
#define RNP_FC_TX_PLTH_OFFSET (4) /* Pause Low Threshold */
#define RNP_FC_TX_PT_OFFSET (16) /* Pause Time */

#define RNP_DCB_MAX_TCS_NUM (4)
#define RNP_DCB_MAX_PFC_NUM (4)

struct rnp_adapter;
int rnp_dcb_init(struct net_device *dev, struct rnp_adapter *adapter);
#endif

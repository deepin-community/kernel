/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#ifndef __RNPM_PTP_H__
#define __RNPM_PTP_H__

#define RNPM_ETH_PTP_TIMESTAMP_SEL (0x38010)
/* PTP Timestamp control register defines */
/* Timestamp Enable */
#define RNPM_PTP_TCR_TSENA BIT(0)
/* Timestamp Fine/Coarse Update */
#define RNPM_PTP_TCR_TSCFUPDT BIT(1)
/* Timestamp Initialize */
#define RNPM_PTP_TCR_TSINIT BIT(2)
/* Timestamp Update */
#define RNPM_PTP_TCR_TSUPDT BIT(3)
/* Timestamp Interrupt Trigger Enable */
#define RNPM_PTP_TCR_TSTRIG BIT(4)
/* Addend Reg Update */
#define RNPM_PTP_TCR_TSADDREG BIT(5)
/* Enable Timestamp for All Frames */
#define RNPM_PTP_TCR_TSENALL BIT(8)
/* Digital or Binary Rollover Control */
#define RNPM_PTP_TCR_TSCTRLSSR BIT(9)
/* Enable PTP packet Processing for Version 2 Format */
#define RNPM_PTP_TCR_TSVER2ENA BIT(10)
/* Enable Processing of PTP over Ethernet Frames */
#define RNPM_PTP_TCR_TSIPENA BIT(11)
/* Enable Processing of PTP Frames Sent over IPv6-UDP */
#define RNPM_PTP_TCR_TSIPV6ENA BIT(12)
/* Enable Processing of PTP Frames Sent over IPv4-UDP */
#define RNPM_PTP_TCR_TSIPV4ENA BIT(13)
/* Enable Timestamp Snapshot for Event Messages */
#define RNPM_PTP_TCR_TSEVNTENA BIT(14)
/* Enable Snapshot for Messages Relevant to Master */
#define RNPM_PTP_TCR_TSMSTRENA BIT(15)
/* Note 802.1 AS Is work Over Ethernet FramesC_Sub_Second_Incremen
 * and Normal PTP Is work Oveer UDP
 */

/* Select PTP packets for Taking Snapshots
 * On mac specifically:
 * Enable SYNC, Pdelay_Req, Pdelay_Resp when TSEVNTENA is enabled.
 * or
 * Enable  SYNC, Follow_Up, Delay_Req, Delay_Resp, Pdelay_Req, Pdelay_Resp,
 * Pdelay_Resp_Follow_Up if TSEVNTENA is disabled
 */
#define RNPM_PTP_TCR_SNAPTYPSEL_1 BIT(16)
/* Enable MAC address for PTP Frame Filtering */
#define RNPM_PTP_TCR_TSENMACADDR BIT(18)
/* External System Time Input Or MAC Internal Clock*/
#define RNPM_PTP_TCR_ESTI BIT(20)
/* AV802.1 AS Mode Enable*/
#define RNPM_PTP_TCR_AV8021ASMEN BIT(28)
/* Sub Second increament define */
/* Sub-second increment value */
#define RNPM_PTP_SSIR_SSINC_MASK (0xff)
/* Sub-second increment offset */
#define RNPM_PTP_SSIR_SSINC_SHIFT (16)
/* TX timestamp reg is fill complete */
#define RNPM_MAC_TXTSC BIT(15)
/*nano second avalid value  */
#define RNPM_MAC_TXTSSTSLO GENMASK(30, 0)

#define RNPM_RX_SEC_MASK GENMASK(30, 0)
#define RNPM_RX_NSEC_MASK GENMASK(30, 0)
#define RNPM_RX_TIME_RESERVE (8)
#define RNPM_RX_SEC_SIZE (4)
#define RNPM_RX_NANOSEC_SIZE (4)
#define RNPM_RX_HWTS_OFFSET \
	(RNPM_RX_SEC_SIZE + RNPM_RX_NANOSEC_SIZE + RNPM_RX_TIME_RESERVE)

#define PTP_STNSUR_ADDSUB_SHIFT (31)
/* 10e9-1 ns */
#define PTP_DIGITAL_ROLLOVER_MODE 0x3B9AC9ff
/* ns */
#define PTP_BINARY_ROLLOVER_MODE 0x7FFFFFFF

#define PTP_HWTX_TIME_VALUE_MASK GENMASK(31, 0)
#define PTP_GET_TX_HWTS_FINISH (1)
#define PTP_GET_TX_HWTS_UPDATE (0)
/*hardware ts can't so fake ts from the software clock */
#define DEBUG_PTP_HARD_SOFTWAY

int rnpm_ptp_get_ts_config(struct rnpm_adapter *pf, struct ifreq *ifr);
int rnpm_ptp_set_ts_config(struct rnpm_adapter *pf, struct ifreq *ifr);
int rnpm_ptp_register(struct rnpm_adapter *pf);
void rnpm_ptp_unregister(struct rnpm_adapter *pf);

void rnpm_ptp_get_rx_hwstamp(struct rnpm_adapter *pf,
			     union rnpm_rx_desc *desc,
			     struct sk_buff *skb);
void rnpm_tx_hwtstamp_work(struct work_struct *work);
void rnpm_ptp_reset(struct rnpm_adapter *adapter);
#endif

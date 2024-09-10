/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef __RNPGBE_PTP_H__
#define __RNPGBE_PTP_H__

struct rnpgbe_hwtimestamp {
	void (*config_hw_tstamping)(void __iomem *ioaddr, u32 data);
	void (*config_sub_second_increment)(void __iomem *ioaddr, u32 ptp_clock,
					    int gmac4, u32 *ssinc);
	void (*config_mac_irq_enable)(void __iomem *ioaddr, bool on);
	int (*init_systime)(void __iomem *ioaddr, u32 sec, u32 nsec);
	int (*config_addend)(void __iomem *ioaddr, u32 addend);
	int (*adjust_systime)(void __iomem *ioaddr, u32 sec, u32 nsec,
			      int add_sub, int gmac4);
	void (*get_systime)(void __iomem *ioaddr, u64 *systime);
};
/* IEEE 1588 PTP register offsets */
#define PTP_TCR 0x00 /* Timestamp Control Reg */
#define PTP_SSIR 0x04 /* Sub-Second Increment Reg */
#define PTP_STSR 0x08 /* System Time – Seconds Regr */
#define PTP_STNSR 0x0c /* System Time – Nanoseconds Reg */
#define PTP_STSUR 0x10 /* System Time – Seconds Update Reg */
#define PTP_STNSUR 0x14 /* System Time – Nanoseconds Update Reg */
#define PTP_TAR 0x18 /* Timestamp Addend Reg */
#define PTP_PPS_CONTROL 0x2c
#define RNP_PTP_STNSUR_ADDSUB_SHIFT 31
#define RNP_PTP_DIGITAL_ROLLOVER_MODE 0x3B9ACA00 /* 10e9-1 ns */
#define RNP_PTP_BINARY_ROLLOVER_MODE 0x80000000 /* ~0.466 ns */
/* PTP Timestamp control register defines */
#define RNP_PTP_TCR_TSENA BIT(0) /*Timestamp Enable*/
#define RNP_PTP_TCR_TSCFUPDT BIT(1) /* Timestamp Fine/Coarse Update */
#define RNP_PTP_TCR_TSINIT BIT(2) /* Timestamp Initialize */
#define RNP_PTP_TCR_TSUPDT BIT(3) /* Timestamp Update */
#define RNP_PTP_TCR_TSTRIG BIT(4) /* Timestamp Interrupt Trigger Enable */
#define RNP_PTP_TCR_TSADDREG BIT(5) /* Addend Reg Update */
#define RNP_PTP_TCR_TSENALL BIT(8) /* Enable Timestamp for All Frames */
#define RNP_PTP_TCR_TSCTRLSSR BIT(9) /* Digital or Binary Rollover Control */
#define RNP_PTP_TCR_TSVER2ENA                                                  \
	BIT(10) /* Enable PTP packet Processing for Version 2 Format */
#define RNP_PTP_TCR_TSIPENA                                                    \
	BIT(11) /* Enable Processing of PTP over Ethernet Frames */
#define RNP_PTP_TCR_TSIPV6ENA                                                  \
	BIT(12) /* Enable Processing of PTP Frames Sent over IPv6-UDP */
#define RNP_PTP_TCR_TSIPV4ENA                                                  \
	BIT(13) /* Enable Processing of PTP Frames Sent over IPv4-UDP */
#define RNP_PTP_TCR_TSEVNTENA                                                  \
	BIT(14) /* Enable Timestamp Snapshot for Event Messages */
#define RNP_PTP_TCR_TSMSTRENA                                                  \
	BIT(15) /* Enable Snapshot for Messages Relevant to Master */
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
#define RNP_PTP_TCR_SNAPTYPSEL_1 BIT(16)
#define RNP_PTP_TCR_TSENMACADDR                                                \
	BIT(18) /* Enable MAC address for PTP Frame Filtering */
#define RNP_PTP_TCR_ESTI                                                       \
	BIT(20) /* External System Time Input Or MAC Internal Clock*/
#define RNP_PTP_TCR_AV8021ASMEN BIT(28) /* AV802.1 AS Mode Enable*/
/* Sub Second increament define */
#define RNP_PTP_SSIR_SSINC_MASK (0xff) /* Sub-second increment value */
#define RNP_PTP_SSIR_SSINC_SHIFT (16) /* Sub-second increment offset */
#define RNP_MAC_TXTSC BIT(15) /* TX timestamp reg is fill complete */
#define RNP_MAC_TXTSSTSLO GENMASK(30, 0) /* nano second avalid value  */
#define RNP_RX_SEC_MASK GENMASK(30, 0)
#define RNP_RX_NSEC_MASK GENMASK(30, 0)
#define RNP_RX_TIME_RESERVE (8)
#define RNP_RX_SEC_SIZE (4)
#define RNP_RX_NANOSEC_SIZE (4)
#define RNP_RX_HWTS_OFFSET                                                     \
	(RNP_RX_SEC_SIZE + RNP_RX_NANOSEC_SIZE + RNP_RX_TIME_RESERVE)
#define PTP_HWTX_TIME_VALUE_MASK GENMASK(31, 0)
#define PTP_GET_TX_HWTS_FINISH (1)
#define PTP_GET_TX_HWTS_UPDATE (0)
/* hardware ts can't so fake ts from the software clock */
#define DEBUG_PTP_HARD_SOFTWAY

int rnpgbe_ptp_get_ts_config(struct rnpgbe_adapter *pf, struct ifreq *ifr);
int rnpgbe_ptp_set_ts_config(struct rnpgbe_adapter *pf, struct ifreq *ifr);
int rnpgbe_ptp_register(struct rnpgbe_adapter *pf);
void rnpgbe_ptp_unregister(struct rnpgbe_adapter *pf);
void rnpgbe_ptp_get_rx_hwstamp(struct rnpgbe_adapter *pf,
			       union rnpgbe_rx_desc *desc, struct sk_buff *skb);
void rnpgbe_tx_hwtstamp_work(struct work_struct *work);
void rnpgbe_ptp_reset(struct rnpgbe_adapter *adapter);

#endif /* __RNPGBE_PTP_H__ */

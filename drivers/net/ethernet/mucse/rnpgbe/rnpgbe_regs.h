/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef RNPGBE_REGS_H
#define RNPGBE_REGS_H

/*             BAR2 memory                   */
/* ------------------------------------------*/
/*	module  | size  |  start   |    end  */
/*	DMA	| 32KB	| 0_0000H  | 0_7FFFH */
/*	REG	| 32KB	| 0_8000H  | 0_FFFFH */
/*	ETH	| 64KB	| 1_0000H  | 1_FFFFH */
/*	GMAC	| 32KB	| 2_0000H  | 2_7FFFH */
/*	MSIX    | 32KB  | 2_8000H  | 2_FFFFH */
/* ------------------------------------------*/

/* ==================== RNP-DMA Global Registers ==================== */
/* n10 */
#define RNP10_RING_BASE (0x8000)
/* n20 */
#define RNP20_RING_BASE (0x8000)
/* n500 */
#define RNP500_RING_BASE (0x1000)
#define RING_OFFSET(queue_idx) (0x100 * (queue_idx))
#define RNP_DMA_VERSION (0x0000)
#define RNP_DMA_CONFIG (0x0004)
#define DMA_MAC_LOOPBACK (1 << 0)
#define DMA_SWITCH_LOOPBACK (1 << 1)
#define DMA_VEB_BYPASS (1 << 4)
#define DMA_AXI_ORDER (1 << 5)
#define DMA_RX_PADDING (1 << 8)
#define DMA_MAP_MODE(n) (n << 12)
#define DMA_RX_FRAGMENT_BYTES(n) (((n) / 16) << 16)
#define RNP_DMA_STATUS (0x0008)
#define RNP_DMA_RX_DATA_PROG_FULL_THRESH (0x00a0)
#define DMA_RING_NUM (0xff << 24)
#define RC_CONTROL_HW (0x01)
#define RC_CONTROL_PHY_DRIVER (0x02)
#define RC_JUMP_STATUS (0x04)
#define RC_PHY_LINK_DONE (0x08)
#define RC_LINK_CHANGE (0x10)
#define RNP_DMA_DUMY (0x000c)
#define RNP_DMA_RX_START (0x10)
#define RNP_DMA_RX_READY (0x14)
#define RNP_DMA_TX_START (0x18)
#define RNP_DMA_TX_READY (0x1c)
#define RNP_DMA_INT_STAT (0x20)
#define RNP_DMA_INT_MASK (0x24)
#define TX_INT_MASK (1 << 1)
#define RX_INT_MASK (1 << 0)
#define RNP_DMA_INT_CLR (0x28)
#define RNP_DMA_INT_TRIG (0x2c)
#define RNP_DMA_AXI_EN (0x0010)
#define RX_AXI_RW_EN (0x03 << 0)
#define TX_AXI_RW_EN (0x03 << 2)
#define RNP_DMA_AXI_STAT (0x0014)
#define RNP_VEB_MAC_MASK_LO (0x0020)
#define RNP_VEB_MAC_MASK_HI (0x0024)
#define RNP_VEB_VLAN_MASK (0x0028)
#define DEBUG_PROBE_NUM 16
#define RNP_DMA_DEBUG_PROBE_LO_REG(n) (0x0100 + 0x08 * (n))
#define RNP_DMA_DEBUG_PROBE_HI_REG(n) (0x0100 + 0x08 * (n))
#define DEBUG_CNT_NUM 76
#define RNP_DMA_DEBUG_CNT(n) (0x0200 + 0x04 * (n))
#define RNP_DMA_STATS_DMA_TO_MAC_CHANNEL_0 (RNP_DMA_DEBUG_CNT(17))
#define RNP_DMA_STATS_DMA_TO_MAC_CHANNEL_1 (RNP_DMA_DEBUG_CNT(18))
#define RNP_DMA_STATS_DMA_TO_MAC_CHANNEL_2 (RNP_DMA_DEBUG_CNT(19))
#define RNP_DMA_STATS_DMA_TO_MAC_CHANNEL_3 (RNP_DMA_DEBUG_CNT(20))
#define RNP_DMA_STATS_DMA_TO_SWITCH (RNP_DMA_DEBUG_CNT(21))
#define RNP_DMA_STATS_MAC_TO_DMA (RNP_DMA_DEBUG_CNT(22))
#define RNP_DMA_STATS_SWITCH_TO_DMA (RNP_DMA_DEBUG_CNT(23))
#define RNP_PCI_WR_TO_HOST (RNP_DMA_DEBUG_CNT(34))
/* RX-Queue Registers */
#define RNP_DMA_REG_RX_DESC_BUF_BASE_ADDR_HI (0x30)
#define RNP_DMA_REG_RX_DESC_BUF_BASE_ADDR_LO (0x34)
#define RNP_DMA_REG_RX_DESC_BUF_LEN (0x38)
#define RNP_DMA_REG_RX_DESC_BUF_HEAD (0x3c)
#define RNP_DMA_REG_RX_DESC_BUF_TAIL (0x40)
#define RNP_DMA_REG_RX_DESC_FETCH_CTRL (0x44)
#define RNP_DMA_REG_RX_INT_DELAY_TIMER (0x48)
#define RNP_DMA_REG_RX_INT_DELAY_PKTCNT (0x4c)
#define RNP_DMA_REG_RX_ARB_DEF_LVL (0x50)
#define PCI_DMA_REG_RX_DESC_TIMEOUT_TH (0x54)
#define PCI_DMA_REG_RX_SCATTER_LENGTH (0x58)
/* TX-Queue Registers */
#define RNP_DMA_REG_TX_DESC_BUF_BASE_ADDR_HI (0x60)
#define RNP_DMA_REG_TX_DESC_BUF_BASE_ADDR_LO (0x64)
#define RNP_DMA_REG_TX_DESC_BUF_LEN (0x68)
#define RNP_DMA_REG_TX_DESC_BUF_HEAD (0x6c)
#define RNP_DMA_REG_TX_DESC_BUF_TAIL (0x70)
#define RNP_DMA_REG_TX_DESC_FETCH_CTRL (0x74)
#define RNP_DMA_REG_TX_INT_DELAY_TIMER (0x78)
#define RNP_DMA_REG_TX_INT_DELAY_PKTCNT (0x7c)
#define RNP_DMA_REG_TX_ARB_DEF_LVL (0x80)
#define RNP_DMA_REG_TX_FLOW_CTRL_TH (0x84)
#define RNP_DMA_REG_TX_FLOW_CTRL_TM (0x88)
#define RNP_DMA_PKT_FIFO_DATA_PROG_FULL_THRESH (0x0098)
/* VEB Registers */
#define VEB_TBL_CNTS 64
#define RNP_DMA_PORT_VBE_MAC_LO_TBL(port, vf)                                  \
	(0x80A0 + 4 * (port) + 0x100 * (vf))
#define RNP_DMA_PORT_VBE_MAC_HI_TBL(port, vf)                                  \
	(0x80B0 + 4 * (port) + 0x100 * (vf))
#define RNP_DMA_PORT_VEB_VID_TBL(port, vf) (0x80C0 + 4 * (port) + 0x100 * (vf))
#define RNP_DMA_PORT_VEB_VF_RING_TBL(port, vf)                                 \
	(0x80D0 + 4 * (port) + 0x100 * (vf))
#define RNP_DMA_STATS_MAC_TO_MAC (0x1b0)
#define RNP_DMA_STATS_SWITCH_TO_SWITCH (0x1a4)

/* ================================================================== */
#define RNP500_NIC_BASE (0x8000)
#define RNP500_TOP_NIC_REST_N (0x8010 - RNP500_NIC_BASE)
#define RNP500_TOP_MAC_OUI (0xc004 - RNP500_NIC_BASE)
#define RNP500_TOP_MAC_SN (0xc008 - RNP500_NIC_BASE)
#define RNP500_TOP_NIC_CONFIG (0x0004)

/* ==================== RNP-ETH Global Registers ==================== */
#define RNP_ETH_BASE (0x10000)

/*
 * [3:0]:
 * 4'b0000：RSS disable
 * 4'b0001：RSS only
 * 4'b0100：DCB and RSS--8*16
 * 4'b1010：POOLS and RSS--32*4
 * [3] :virtual enable
 * [16]:ipv4_hash_tcp_enable
 * [17]:ipv4_hash_enable
 * [20]:ipv6_hash_enable
 * [21]:ipv6_hash_tcp_enable
 * [22]:ipv4_hash_udp_enable
 * [23]:ipv6_hash_udp_enable
 * [24]:ipv4_hash_sctp_enable
 * [25]:ipv6_hash_sctp_enable
 */

#define INNER_L4_BIT BIT(6)
#define PKT_LEN_ERR (2)
#define HDR_LEN_ERR (1)
#define DROP_ALL_THRESH (2046) /* drop all rx */
#define RECEIVE_ALL_THRESH (0x270) /* receive all rx */
#define RNP500_VEB_TBL_CNTS 8
#define RNP500_DMA_RBUF_FIFO (0x00b0)
#define RNP500_DMA_PORT_VBE_MAC_LO_TBL(port, vf)                               \
	(0x10c0 + 4 * (port) + 0x100 * (vf))
#define RNP500_DMA_PORT_VBE_MAC_HI_TBL(port, vf)                               \
	(0x10c4 + 4 * (port) + 0x100 * (vf))
#define RNP500_DMA_PORT_VEB_VID_TBL(port, vf)                                  \
	(0x10C8 + 4 * (port) + 0x100 * (vf))
#define RNP500_DMA_PORT_VEB_VF_RING_TBL(port, vf)                              \
	(0x10cc + 4 * (port) + 0x100 * (vf))
#define RNP500_ETH_BASE (0x10000)
#define RNP500_ETH_TUPLE5_SAQF(n) (0xc000 + 0x04 * (n))
#define RNP500_ETH_TUPLE5_DAQF(n) (0xc400 + 0x04 * (n))
#define RNP500_ETH_TUPLE5_SDPQF(n) (0xc800 + 0x04 * (n))
#define RNP500_ETH_TUPLE5_FTQF(n) (0xcc00 + 0x04 * (n))
#define RNP500_ETH_TUPLE5_POLICY(n) (0xce00 + 0x04 * (n))
#define RNP500_ETH_DEFAULT_RX_MIN_LEN (0x80f0)
#define RNP500_ETH_DEFAULT_RX_MAX_LEN (0x80f4)
#define RNP500_ETH_VLAN_VME_REG(n) (0x8040 + 0x04 * (n))
#define RNP500_ETH_ERR_MASK_VECTOR (0x8060)
#define RNP500_ETH_RSS_MASK (0x3ff0001)
#define RNP500_ETH_ENABLE_RSS_ONLY (0x3f30001)
#define RNP500_ETH_RSS_CONTROL (0x92a0)
#define RNP500_MRQC_IOV_EN (0x92a0)
#define RNP500_IOV_ENABLED (1 << 3)
#define RNP500_ETH_DISABLE_RSS (0)
#define RNP500_ETH_SYNQF (0x9290)
#define RNP500_ETH_SYNQF_PRIORITY (0x9294)
#define RNP500_ETH_FCS_EN (0x804c)
#define RNP500_ETH_HIGH_WATER(n) (0x80c0 + n * (0x08))
#define RNP500_ETH_LOW_WATER(n) (0x80c4 + n * (0x08))
#define RNP500_ETH_WRAP_FIELD_TYPE (0x805c)
#define RNP500_ETH_TX_VLAN_CONTROL_EANBLE (0x0070)
#define RNP500_ETH_TX_VLAN_TYPE (0x0074)
#define RNP500_ETH_RX_MAC_LEN_REG (0x80e0)
#define RNP500_ETH_WHOLE_PKT_LEN_ERR_DROP (0x807c)
#define RNP500_RAH_AV 0x80000000
#define RNP500_ETH_RAR_RL(n) (0xa000 + 0x04 * n)
#define RNP500_ETH_RAR_RH(n) (0xa400 + 0x04 * n)
#define RNP500_FCTRL_BPE BIT(10)
#define RNP500_FCTRL_UPE BIT(9)
#define RNP500_FCTRL_MPE BIT(8)
#define RNP500_ETH_DMAC_FCTRL (0x9110)
#define RNP500_ETH_DMAC_MCSTCTRL (0x9114)
#define RNP500_MCSTCTRL_MULTICASE_TBL_EN (1 << 4)
#define RNP500_MCSTCTRL_UNICASE_TBL_EN (1 << 3)
#define RNP500_VM_DMAC_MPSAR_RING(entry)                                       \
	(0xb400 + (4 * (entry)))
#define RNP500_ETH_MUTICAST_HASH_TABLE(n) (0xac00 + 0x04 * n)
#define RNP500_ETH_RSS_KEY (0x92d0)
#define RNP500_ETH_TC_IPH_OFFSET_TABLE(n) (0xe800 + 0x04 * (n))
#define RNP500_ETH_RSS_INDIR_TBL(n) (0xe000 + 0x04 * (n))
#define RNP500_ETH_VLAN_FILTER_TABLE(n) (0xb000 + 0x04 * (n))
#define RNP500_VFTA RNP500_ETH_VLAN_FILTER_TABLE
#define RNP500_VLVF(idx) (0xb600 + 4 * (idx))
#define RNP500_VLVF_TABLE(idx) (0xb700 + 4 * (idx))
#define RNP500_ETH_VLAN_FILTER_ENABLE (0x9118)
#define RNP500_PRIORITY_1_MARK (0x8080)
#define RNP500_PRIORITY_1 (400)
#define RNP500_PRIORITY_0 (300)
#define RNP500_PRIORITY_0_MARK (0x8084)
#define RNP500_PRIORITY_EN (0x8088)
#define RNP500_PRIORITY_EN_8023 (0x808c)
#define RNP500_ETH_LAYER2_ETQF(n) (0x9200 + 0x04 * (n))
#define RNP500_ETH_LAYER2_ETQS(n) (0x9240 + 0x04 * (n))
#define RNP500_ETH_BYPASS (0x8000)
#define RNP500_ETH_ERR_MASK_VECTOR (0x8060)
#define RNP500_ETH_PRIV_DATA_CONTROL_REG (0x8068)
#define RNP500_ETH_DEFAULT_RX_RING (0x806c)
#define RNP500_ETH_DOUBLE_VLAN_DROP (0x8078)
#define RNP500_HOST_FILTER_EN (0x800c)
#define RNP500_BAD_PACKETS_RECEIVE_EN (0x8024)
#define RNP500_REDIR_EN (0x8030)
#define WATCHDOG_TIMER_ERROR BIT(0)
#define RUN_FRAME_ERROR BIT(1)
#define GAINT_FRAME_ERROR BIT(2)
#define LATE_COLLISION_ERROR BIT(3)
#define GMII_ERROR BIT(4)
#define DRIBBLING_BIT_ERROR BIT(5)
#define CRC_ERROR BIT(6)
#define LENGTH_ERROR BIT(8)
#define DA_FILTER_ERROR BIT(9)
#define SA_FILTER_ERROR BIT(10)
#define RNP500_MAC_ERR_MASK (0x8034)
#define RNP500_ETH_SCTP_CHECKSUM_EN (0x8038)
#define RNP500_ETH_VLAN_RM_TYPE (0x8054)
#define RNP500_ETH_EXCEPT_DROP_PROC (0x0470)
#define RNP500_ETH_EMAC_PARSE_PROGFULL_THRESH (0x8098)
#define RNP500_ETH_TX_MUX_DROP (0x98)
#define RNP500_VEB_VFMPRC(n) (0x4018 + 0x100 * n)
#define RNP500_VEB_VFBPRC(n) (0x401c + 0x100 * n)
#define RNP500_RX_TIMEOUT_DROP(n) (0x404c + 0x100 * n)
#define RNP500_STATISTIC_CRL(n) (0x4048 + 0x100 * n)
/* n500 statistics REG */
#define RNP500_RX_MULTI_PKT_NUM (0x8224)
#define RNP500_RX_BROAD_PKT_NUM (0x8228)
#define RNP500_RX_MAC_CUT_NUM (0x8304)
#define RNP500_RX_MAC_LCS_ERR_NUM (0x8308)
#define RNP500_RX_MAC_LEN_ERR_NUM (0X830C)
#define RNP500_RX_MAC_SLEN_ERR_NUM (0x8310)
#define RNP500_RX_MAC_GLEN_ERR_NUM (0x8314)
#define RNP500_RX_MAC_FCS_ERR_NUM (0x8318)
#define RNP500_RX_MAC_SFCS_ERR_NUM (0x831c)
#define RNP500_RX_MAC_GFCS_ERR_NUM (0x8320)
#define RNP500_TX_MULTI_NUM (0x214)
#define RNP500_TX_BROADCAST_NUM (0x218)
#define RNP500_RX_DROP_PKT_NUM (0X8230)
#define RNP500_RXTRANS_DROP (0x8908)
#define RNP500_RXTRANS_CUT_ERR_PKTS (0x894c)
#define RNP500_DECAP_PKT_DROP1_NUM (0X82ec)
#define RNP500_MAC_COUNT_CONTROL (0x0100)
#define RNP500_MAC_GLEN_ERR_NUM (0X01a8)
#define RNP500_RX_DEBUG(n) (0x8400 + 0x04 * n)
#define RNP500_ETH_HOST_L2_DROP_PKTS RNP500_RX_DEBUG(4)
#define RNP500_ETH_REDIR_INPUT_MATCH_DROP_PKTS RNP500_RX_DEBUG(5)
#define RNP500_ETH_ETYPE_DROP_PKTS RNP500_RX_DEBUG(6)
#define RNP500_ETH_TCP_SYN_DROP_PKTS RNP500_RX_DEBUG(7)
#define RNP500_ETH_REDIR_TUPLE5_DROP_PKTS RNP500_RX_DEBUG(8)

/* ================================================================== */
#define ETH_ERR_SCTP (1 << 4)
#define ETH_ERR_L4 (1 << 3)
#define ETH_ERR_L3 (1 << 2)
#define ETH_ERR_PKT_LEN_ERR (1 << 1)
#define ETH_ERR_HDR_LEN_ERR (1 << 0)
#define ETH_IGNORE_ALL_ERR                                                     \
	(ETH_ERR_SCTP | ETH_ERR_L4 | ETH_ERR_L3 | ETH_ERR_PKT_LEN_ERR |        \
	 ETH_ERR_HDR_LEN_ERR)
#define VM_DMAC_TBL_SZ 128
#define RNP_ETH_ENABLE_RSS_ONLY (0x3f30001)
#define RNP_ETH_DISABLE_RSS (0)
#define RNP_ETH_TX_PROGFULL_THRESH_PORT(n) (RNP_ETH_BASE + 0x0060 + 0x08 * (n))
#define RNP_ETH_TX_PROGEMPTY_THRESH_PORT(n) (RNP_ETH_BASE + 0x0064 + 0x08 * (n))
#define RNP_ETH_EMAC_DMA_PROFULL_THRESH (RNP_ETH_BASE + 0x0080)
#define RNP_ETH_EMAC_DMA_PROEMPTY_THRESH (RNP_ETH_BASE + 0x0084)
#define RNP_ETH_EMAC_SW_PROFULL_THRESH (RNP_ETH_BASE + 0x0088)
#define RNP_ETH_EMAC_SW_PROEMPTY_THRESH (RNP_ETH_BASE + 0x008c)
#define RNP_ETH_EMAC_BMC_TX_PROFULL_THRESH (RNP_ETH_BASE + 0x0090)
#define RNP_ETH_EMAC_BMC_TX_PROEMPTY_THRESH (RNP_ETH_BASE + 0x0094)
#define RNP_ETH_CNT_PKT_EMAC_TX(n) (RNP_ETH_BASE + 0x00a0 + 0x04 * (n))
#define RNP_ETH_CNT_PKT_PECL_TX(n) (RNP_ETH_BASE + 0x00b0 + 0x04 * (n))
#define RNP_ETH_STATUS_TX_FLOWCTRL(n) (RNP_ETH_BASE + 0x00c0 + 0x04 * (n))
#define RNP_ETH_VERSION_FLOWWCTRL (RNP_ETH_BASE + 0x00d0)
#define RNP_ETH_CFG_ETH_MAC (RNP_ETH_BASE + 0x00d4)
#define RNP_ETH_SCA_TX_CS(port) (RNP_ETH_BASE + 0x0100 + 0x08 * (port))
#define RNP_ETH_SCA_TX_NS(port) (RNP_ETH_BASE + 0x0104 + 0x08 * (port))
#define RNP_ETH_TXTRANS_CS(port) (RNP_ETH_BASE + 0x0120 + 0x08 * (port))
#define RNP_ETH_TXTRANS_NS(port) (RNP_ETH_BASE + 0x0124 + 0x08 * (port))
#define RNP_ETH_1TO4_INST0_IN_PKTS (RNP_ETH_BASE + 0x0200)
#define RNP_ETH_1TO4_INST1_IN_PKTS (RNP_ETH_BASE + 0x0204)
#define RNP_ETH_1TO4_INST2_IN_PKTS (RNP_ETH_BASE + 0x0208)
#define RNP_ETH_1TO4_INST3_IN_PKTS (RNP_ETH_BASE + 0x020c)
#define RNP_ETH_IN_0_TX_PKT_NUM(port) (RNP_ETH_BASE + 0x0210 + 0x10 * (port))
#define RNP_ETH_IN_1_TX_PKT_NUM(port) (RNP_ETH_BASE + 0x0214 + 0x10 * (port))
#define RNP_ETH_IN_2_TX_PKT_NUM(port) (RNP_ETH_BASE + 0x0218 + 0x10 * (port))
#define RNP_ETH_IN_3_TX_PKT_NUM(port) (RNP_ETH_BASE + 0x021c + 0x10 * (port))
#define RNP_ETH_EMAC_TX_TO_PHY_PKTS(port) (RNP_ETH_BASE + 0x0250 + 4 * (port))
#define RNP_ETH_TXTRANS_PTP_PKT_NUM(port) (RNP_ETH_BASE + 0x0260 + 4 * (port))
#define RNP_ETH_TX_DEBUG(n) (RNP_ETH_BASE + 0x0300 + 0x04 * (n))
#define RNP_ETH_PTP_TX_STATUS(n) (RNP_ETH_BASE + 0x0400)
#define RNP_ETH_PTP_TX_HTIMES(n) (RNP_ETH_BASE + 0x0404)
#define RNP_ETH_PTP_TX_LTIMES(n) (RNP_ETH_BASE + 0x0408)
#define RNP_ETH_PTP_TX_TSVALUE_STATUS(n) (RNP_ETH_BASE + 0x040c)
#define RNP_ETH_PTP_TX_CLEAR(n) (RNP_ETH_BASE + 0x0410)
#define RNP_ETH_MAC_SPEED_PORT(n) (RNP_ETH_BASE + 0x0450 + 0x04 * (n))
#define RNP_ETH_MAC_LOOPBACK_MODE_PORT(n) (RNP_ETH_BASE + 0x0460 + 0x04 * (n))
#define RNP_ETH_EXCEPT_DROP_PROC (RNP_ETH_BASE + 0x0470)
#define RNP_ETH_IPP (RNP_ETH_BASE + 0x8000)
#define RNP_ETH_BYPASS (RNP_ETH_BASE + 0x8000)
#define RNP_ETH_TUNNEL_MOD (RNP_ETH_BASE + 0x8004)
#define RNP_ETH_LOOPBACK_EN (RNP_ETH_BASE + 0x8008)
#define RNP_FIFO_CTRL_MODE (RNP_ETH_BASE + 0x800c)
#define RNP_ETH_VXLAN_PORT (RNP_ETH_BASE + 0x8010)
#define RNP_ETH_NVGRE_PORT (RNP_ETH_BASE + 0x8014)
#define RNP_ETH_RDMA_PORT (RNP_ETH_BASE + 0x8018)
#define RNP_HOST_FILTER_EN (RNP_ETH_BASE + 0x801c)
#define RNP_MNG_FILTER_EN (RNP_ETH_BASE + 0x8020)
#define RNP_ETH_TCAM_EN (RNP_ETH_BASE + 0x8024)
#define RNP_CONGEST_DROP_EN (RNP_ETH_BASE + 0x8028)
#define RNP_REDIR_EN (RNP_ETH_BASE + 0x8030)
#define RNP_ETH_SCTP_CHECKSUM_EN (RNP_ETH_BASE + 0x8038)
#define RNP_ETH_ARP_FUNC_EN (RNP_ETH_BASE + 0x803c)
#define RNP_ETH_VLAN_VME_REG(n) (RNP_ETH_BASE + 0x8040 + 0x04 * (n))
#define RNP_ETH_CVLAN_RM_EN (RNP_ETH_BASE + 0x8050)
#define RNP_ETH_VLAN_RM_TYPE (RNP_ETH_BASE + 0x8054)
#define RNP_ETH_WRAP_FIELD_TYPE (RNP_ETH_BASE + 0x805c)
#define RNP_ETH_ERR_MASK_VECTOR (RNP_ETH_BASE + 0x8060)
#define RNP_ETH_DEFAULT_RX_RING (RNP_ETH_BASE + 0x806c)
#define RNP_ETH_RX_PROGFULL_THRESH_PORT(n) (RNP_ETH_BASE + 0x8070 + 0x08 * (n))
#define RNP_ETH_RX_PROGEMPTY_THRESH_PORT(n) (RNP_ETH_BASE + 0x8074 + 0x08 * (n))
#define RNP_ETH_EMAC_GAT_PROGFULL_THRESH (RNP_ETH_BASE + 0x8090)
#define RNP_ETH_EMAC_GAT_PROGEMPTY_THRESH (RNP_ETH_BASE + 0x8094)
#define RNP_ETH_EMAC_PARSE_PROGFULL_THRESH (RNP_ETH_BASE + 0x8098)
#define RNP_ETH_EMAC_PARSE_PROGEMPTY_THRESH (RNP_ETH_BASE + 0x809c)
#define RNP_ETH_FC_PROGFULL_THRESH (RNP_ETH_BASE + 0x80a0)
#define RNP_ETH_FC_PROGEMPTY_THRESH (RNP_ETH_BASE + 0x80a4)
#define RNP_ETH_DIS_PROGFULL_THRESH (RNP_ETH_BASE + 0x80a8)
#define RNP_ETH_DIS_PROGEMPTY_THRESH (RNP_ETH_BASE + 0x80ac)
#define RNP_ETH_COV_PROGFULL_THRESH (RNP_ETH_BASE + 0x80b0)
#define RNP_ETH_COV_PROGEMPTY_THRESH (RNP_ETH_BASE + 0x80b4)
#define RNP_ETH_BMC_RX_PROGFULL_THRESH (RNP_ETH_BASE + 0x80b8)
#define RNP_ETH_BMC_RX_PROGEMPTY_THRESH (RNP_ETH_BASE + 0x80bc)
#define RNP_ETH_HIGH_WATER(n) (RNP_ETH_BASE + 0x80c0 + n * (0x08))
#define RNP_ETH_LOW_WATER(n) (RNP_ETH_BASE + 0x80c4 + n * (0x08))
#define RNP_ETH_DEFAULT_RX_MIN_LEN (RNP_ETH_BASE + 0x80f0)
#define RNP_ETH_DEFAULT_RX_MAX_LEN (RNP_ETH_BASE + 0x80f4)
#define RNP_ETH_PTP_EVENT_PORT (RNP_ETH_BASE + 0x80f8)
#define RNP_ETH_PTP_GENER_PORT_REG (RNP_ETH_BASE + 0x80fc)
#define RNP_ETH_RX_TRANS_CS_PORT(n) (RNP_ETH_BASE + 0x8100 + 0x08 * (n))
#define RNP_ETH_RX_TRANS_NS_PORT(n) (RNP_ETH_BASE + 0x8104 + 0x08 * (n))
#define RNP_ETH_GAT_RX_CS (RNP_ETH_BASE + 0x8120)
#define RNP_ETH_GAT_RX_NS (RNP_ETH_BASE + 0x8124)
#define RNP_ETH_EMAC_PIP_CS (RNP_ETH_BASE + 0x8128)
#define RNP_ETH_EMAC_PIP_NS (RNP_ETH_BASE + 0x812c)
#define RNP_ETH_EMAC_FC_CS (RNP_ETH_BASE + 0x8138)
#define RNP_ETH_EMAC_FC_NS (RNP_ETH_BASE + 0x813c)
#define RNP_ETH_EMAC_DIS_CS (RNP_ETH_BASE + 0x8140)
#define RNP_ETH_EMAC_DIS_NS (RNP_ETH_BASE + 0x8144)
#define RNP_ETH_HOST_L2_FILTER_CS (RNP_ETH_BASE + 0x8150)
#define RNP_ETH_HOST_L2_FILTER_NS (RNP_ETH_BASE + 0x8154)
#define RNP_ETH_EMAC_DECAP_CS (RNP_ETH_BASE + 0x8158)
#define RNP_ETH_EMAC_DECAP_NS (RNP_ETH_BASE + 0x815c)
#define RNP_ETH_PFC_CONFIG_PROT(n) (RNP_ETH_BASE + 0x8180 + n * (0x04))
#define RNP_ETH_RX_PKT_NUM(port) (RNP_ETH_BASE + 0x8220 + 0x04 * (port))
#define RNP_ETH_RX_DROP_PKT_NUM(port) (RNP_ETH_BASE + 0x8230 + 0x04 * (port))
#define RNP_ETH_TOTAL_GAT_RX_PKT_NUM (RNP_ETH_BASE + 0x8240)
#define RNP_ETH_PKT_ARP_REQ_NUM (RNP_ETH_BASE + 0x8250)
#define RNP_ETH_PKT_ARP_RESPONSE_NUM (RNP_ETH_BASE + 0x8254)
#define RNP_ETH_ICMP_NUM (RNP_ETH_BASE + 0x8258)
#define RNP_ETH_PKT_UDP_NUM (RNP_ETH_BASE + 0x825c)
#define RNP_ETH_PKT_TCP_NUM (RNP_ETH_BASE + 0x8260)
#define RNP_ETH_PKT_ESP_NUM (RNP_ETH_BASE + 0x8264)
#define RNP_ETH_PKT_GRE_NUM (RNP_ETH_BASE + 0x8268)
#define RNP_ETH_PKT_SCTP_NUM (RNP_ETH_BASE + 0x826c)
#define RNP_ETH_PKT_TCPSYN_NUM (RNP_ETH_BASE + 0x8270)
#define RNP_ETH_PKT_VXLAN_NUM (RNP_ETH_BASE + 0x8274)
#define RNP_ETH_PKT_NVGRE_NUM (RNP_ETH_BASE + 0x8278)
#define RNP_ETH_PKT_FRAGMENT_NUM (RNP_ETH_BASE + 0x827c)
#define RNP_ETH_PKT_LAYER1_VLAN_NUM (RNP_ETH_BASE + 0x8280)
#define RNP_ETH_PKT_LAYER2_VLAN_NUM (RNP_ETH_BASE + 0x8284)
#define RNP_ETH_PKT_IPV4_NUM (RNP_ETH_BASE + 0x8288)
#define RNP_ETH_PKT_IPV6_NUM (RNP_ETH_BASE + 0x828c)
#define RNP_ETH_PKT_INGRESS_NUM (RNP_ETH_BASE + 0x8290)
#define RNP_ETH_PKT_EGRESS_NUM (RNP_ETH_BASE + 0x8294)
#define RNP_ETH_PKT_IP_HDR_LEN_ERR_NUM (RNP_ETH_BASE + 0x8298)
#define RNP_ETH_PKT_IP_PKT_LEN_ERR_NUM (RNP_ETH_BASE + 0x829c)
#define RNP_ETH_PKT_L3_HDR_CHK_ERR_NUM (RNP_ETH_BASE + 0x82a0)
#define RNP_ETH_PKT_L4_HDR_CHK_ERR_NUM (RNP_ETH_BASE + 0x82a4)
#define RNP_ETH_PKT_SCTP_CHK_ERR_NUM (RNP_ETH_BASE + 0x82a8)
#define RNP_ETH_PKT_VLAN_ERR_NUM (RNP_ETH_BASE + 0x82ac)
#define RNP_ETH_PKT_RDMA_NUM (RNP_ETH_BASE + 0x82b0)
#define RNP_ETH_PKT_ARP_AUTO_RESPONSE_NUM (RNP_ETH_BASE + 0x82b4)
#define RNP_ETH_PKT_ICMPV6_NUM (RNP_ETH_BASE + 0x82b8)
#define RNP_ETH_PKT_IPV6_EXTEND_NUM (RNP_ETH_BASE + 0x82bc)
#define RNP_ETH_PKT_802_3_NUM (RNP_ETH_BASE + 0x82c0)
#define RNP_ETH_PKT_EXCEPT_SHORT_NUM (RNP_ETH_BASE + 0x82c4)
#define RNP_ETH_PKT_PTP_NUM (RNP_ETH_BASE + 0x82c8)
#define RNP_ETH_DECAP_PKT_IN_NUM (RNP_ETH_BASE + 0x82d0)
#define RNP_ETH_DECAP_PKT_OUT_NUM (RNP_ETH_BASE + 0x82d4)
#define RNP_ETH_DECAP_DMAC_OUT_NUM (RNP_ETH_BASE + 0x82d8)
#define RNP_ETH_DECAP_BMC_OUT_NUM (RNP_ETH_BASE + 0x82dc)
#define RNP_ETH_DECAP_SW_OUT_NUM (RNP_ETH_BASE + 0x82e0)
#define RNP_ETH_DECAP_MIRROR_OUT_NUM (RNP_ETH_BASE + 0x82e4)
#define RNP_ETH_DECAP_PKT_DROP_NUM(port) (RNP_ETH_BASE + 0x82e8 + 0x04 * (port))
#define RNP_ETH_INVALID_DROP_PKTS RNP_ETH_DECAP_PKT_DROP_NUM(0)
#define RNP_ETH_FILTER_DROP_PKTS RNP_ETH_DECAP_PKT_DROP_NUM(1)
#define RNP_ETH_DECAP_DMAC_DROP_NUM (RNP_ETH_BASE + 0x82f0)
#define RNP_ETH_DECAP_BMC_DROP_NUM (RNP_ETH_BASE + 0x82f4)
#define RNP_ETH_DECAP_SWITCH_DROP_NUM (RNP_ETH_BASE + 0x82f8)
#define RNP_ETH_DECAP_RM_VLAN_NUM (RNP_ETH_BASE + 0x82fc)
#define RNP_ETH_RX_FC_PKT_IN_NUM (RNP_ETH_BASE + 0x8300)
#define RNP_ETH_RX_FC_PKT_OUT_NUM (RNP_ETH_BASE + 0x8304)
#define RNP_ETH_RX_FC_PKT_DROP0_NUM (RNP_ETH_BASE + 0x8308)
#define RNP_ETH_RX_FC_PKT_DROP1_NUM (RNP_ETH_BASE + 0x830c)
#define RNP_ETH_RING_FC_STATUS0 (RNP_ETH_BASE + 0x8310)
#define RNP_ETH_RING_FC_STATUS1 (RNP_ETH_BASE + 0x8314)
#define RNP_ETH_RING_FC_STATUS2 (RNP_ETH_BASE + 0x8318)
#define RNP_ETH_RING_FC_STATUS3 (RNP_ETH_BASE + 0x831c)
#define RNP_ETH_RX_DEBUG(n) (RNP_ETH_BASE + 0x8400 + 0x04 * (n))
#define RNP_ETH_RX_FC_DEBUG0_NUM RNP_ETH_RX_DEBUG(0)
#define RNP_ETH_RX_FC_DEBUG1_NUM RNP_ETH_RX_DEBUG(1)
#define RNP_ETH_RX_DIS_DEBUG0_NUM RNP_ETH_RX_DEBUG(2)
#define RNP_ETH_RX_DIS_DEBUG1_NUM RNP_ETH_RX_DEBUG(3)
#define RNP_ETH_HOST_L2_DROP_PKTS RNP_ETH_RX_DEBUG(4)
#define RNP_ETH_REDIR_INPUT_MATCH_DROP_PKTS RNP_ETH_RX_DEBUG(5)
#define RNP_ETH_ETYPE_DROP_PKTS RNP_ETH_RX_DEBUG(6)
#define RNP_ETH_TCP_SYN_DROP_PKTS RNP_ETH_RX_DEBUG(7)
#define RNP_ETH_REDIR_TUPLE5_DROP_PKTS RNP_ETH_RX_DEBUG(8)
#define RNP_ETH_REDIR_TCAM_DROP_PKTS RNP_ETH_RX_DEBUG(9)
#define RNP_ETH_VMARK_TC(n) (RNP_ETH_BASE + 0x8500 + 0x04 * (n))
#define RNP_RING_FC_ENABLE (RNP_ETH_BASE + 0x8520)
#define RNP_SELECT_RING_EN(n) (RNP_ETH_BASE + 0x8524 + (0x4 * n))
#define RNP_TC_FC_SW_EN (RNP_ETH_BASE + 0x8534)
#define RNP_ETH_LOCAL_DIP(n) (RNP_ETH_BASE + 0x8600 + 0x04 * (n))
#define RNP_ETH_LOCAL_DMAC_H(n) (RNP_ETH_BASE + 0x8700 + 0x04 * (n))
#define RNP_ETH_LOCAL_DMAC_L(n) (RNP_ETH_BASE + 0x8800 + 0x04 * (n))
#define RNP_RXTRANS_RX_PKTS(port) (RNP_ETH_BASE + 0x8900 + 0x40 * (port))
#define RNP_RXTRANS_DROP_PKTS(port) (RNP_ETH_BASE + 0x8904 + 0x40 * (port))
#define RNP_RXTRANS_WDT_ERR_PKTS(port) (RNP_ETH_BASE + 0x8908 + 0x40 * (port))
#define RNP_RXTRANS_CODE_ERR_PKTS(port) (RNP_ETH_BASE + 0x890c + 0x40 * (port))
#define RNP_RXTRANS_CRC_ERR_PKTS(port) (RNP_ETH_BASE + 0x8910 + 0x40 * (port))
#define RNP_RXTRANS_SLEN_ERR_PKTS(port) (RNP_ETH_BASE + 0x8914 + 0x40 * (port))
#define RNP_RXTRANS_GLEN_ERR_PKTS(port) (RNP_ETH_BASE + 0x8918 + 0x40 * (port))
#define RNP_RXTRANS_IPH_ERR_PKTS(port) (RNP_ETH_BASE + 0x891c + 0x40 * (port))
#define RNP_RXTRANS_CSUM_ERR_PKTS(port) (RNP_ETH_BASE + 0x8920 + 0x40 * (port))
#define RNP_RXTRANS_LEN_ERR_PKTS(port) (RNP_ETH_BASE + 0x8924 + 0x40 * (port))
#define RNP_RXTRANS_CUT_ERR_PKTS(port) (RNP_ETH_BASE + 0x8928 + 0x40 * (port))
#define RNP_RXTRANS_EXCEPT_BYTES(port) (RNP_ETH_BASE + 0x892c + 0x40 * (port))
#define RNP_RXTRANS_G1600_BYTES_PKTS(port)                                     \
	(RNP_ETH_BASE + 0x8930 + 0x40 * (port))
#define RNP_RX_RING_MAXRATE(n) (RNP_ETH_BASE + 0x8a00 + (0x4 * n))
#define RNP_ETH_RX_PROGFULL_RTRN(n) (RNP_ETH_BASE + 0x8c00 + 0x04 * (n))
#define RNP_ETH_CNT_PKT_EMAC_RX(n) (RNP_ETH_BASE + 0x8c10 + 0x04 * (n))
#define RNP_ETH_CNT_PKT_PECL_RX(n) (RNP_ETH_BASE + 0x8c20 + 0x04 * (n))
#define RNP_ETH_STATUS_RX_FLOWCTRL(n) (RNP_ETH_BASE + 0x8c30 + 0x04 * (n))
#define RNP_ETH_DMAC_FCTRL (RNP_ETH_BASE + 0x9110)
#define RNP_ETH_DMAC_MCSTCTRL (RNP_ETH_BASE + 0x9114)
#define RNP_MCSTCTRL_MULTICASE_TBL_EN (1 << 2)
#define RNP_MCSTCTRL_UNICASE_TBL_EN (1 << 3)
#define RNP_MCSTCTRL_DMAC_47 0x00
#define RNP_MCSTCTRL_DMAC_46 0x01
#define RNP_MCSTCTRL_DMAC_45 0x02
#define RNP_MCSTCTRL_DMAC_43 0x03
#define RNP_ETH_VLAN_FILTER_ENABLE (RNP_ETH_BASE + 0x9118)
#define RNP_ETH_INPORT_POLICY_VAL (RNP_ETH_BASE + 0x91d0)
#define RNP_ETH_INPORT_POLICY_REG(n) (RNP_ETH_BASE + 0x91e0 + 0x04 * (n))
#define ETH_LAYER2_NUM (16)
#define RNP_ETH_LAYER2_ETQF(n) (RNP_ETH_BASE + 0x9200 + 0x04 * (n))
#define RNP_ETH_LAYER2_ETQS(n) (RNP_ETH_BASE + 0x9240 + 0x04 * (n))
#define RNP_ETH_LAYER2_ETQS_DEFAULT (RNP_ETH_BASE + 0x9280)
#define RNP_ETH_ETQF_DEFAULT (RNP_ETH_BASE + 0x9284)
#define RNP_ETH_SYNQF (RNP_ETH_BASE + 0x9290)
#define RNP_ETH_SYNQF_PRIORITY (RNP_ETH_BASE + 0x9294)
/*
 * [3:0]:
 * 4'b0000：RSS disable
 * 4'b0001：RSS only
 * 4'b0100：DCB and RSS--8*16
 * 4'b1010：POOLS and RSS--32*4
 * [3] :virtual enable
 * [16]:ipv4_hash_tcp_enable
 * [17]:ipv4_hash_enable
 * [20]:ipv6_hash_enable
 * [21]:ipv6_hash_tcp_enable
 * [22]:ipv4_hash_udp_enable
 * [23]:ipv6_hash_udp_enable
 * [24]:ipv4_hash_sctp_enable
 * [25]:ipv6_hash_sctp_enable
 */
#define RNP_ETH_RSS_CONTROL (RNP_ETH_BASE + 0x92a0)
#define RNP_MRQC_IOV_EN (RNP_ETH_BASE + 0x92a0)
#define RNP_IOV_ENABLED (1 << 3)
#define RNP_ETH_RSS_KEY (RNP_ETH_BASE + 0x92d0)
#define RNP_ETH_RAR_RL(n) (RNP_ETH_BASE + 0xa000 + 0x04 * n)
#define RNP_ETH_RAR_RH(n) (RNP_ETH_BASE + 0xa400 + 0x04 * n)
#define RNP_ETH_UTA(n) (RNP_ETH_BASE + 0xa800 + 0x04 * n)
#define RNP_ETH_MUTICAST_HASH_TABLE(n) (RNP_ETH_BASE + 0xac00 + 0x04 * n)
#define RNP_MTA(n) RNP_ETH_MUTICAST_HASH_TABLE(n)
#define RNP_ETH_VLAN_FILTER_TABLE(n) (RNP_ETH_BASE + 0xb000 + 0x04 * (n))
#define RNP_VFTA RNP_ETH_VLAN_FILTER_TABLE
#define RNP_FCTRL_MULTICASE_BYPASS (1 << 8)
#define RNP_FCTRL_UNICASE_BYPASS (1 << 9)
#define RNP_FCTRL_BROADCASE_BYPASS (1 << 10)
#define RNP_ETH_ETYPE_TABLE(n) (RNP_ETH_BASE + 0xb300 + 0x04 * (n))
#define RNP_VM_DMAC_MPSAR_RING(entry)                                          \
	(RNP_ETH_BASE + 0xb400 + (4 * (entry)))
#define RNP_VLVF(idx) (RNP_ETH_BASE + 0xb600 + 4 * (idx))
#define RNP_VLVFB(idx) (RNP_ETH_BASE + 0xb700 + 4 * (idx))
#define RNP_VM_TUNNEL_PFVLVF_L(n) (RNP_ETH_BASE + 0xb800 + 0x04 * (n))
#define RNP_VM_TUNNEL_PFVLVF_H(n) (RNP_ETH_BASE + 0xb900 + 0x04 * (n))
/* 5 tuple */
#define ETH_TUPLE5_NUM 128
#define RNP_ETH_TUPLE5_SAQF(n) (RNP_ETH_BASE + 0xc000 + 0x04 * (n))
#define RNP_ETH_TUPLE5_DAQF(n) (RNP_ETH_BASE + 0xc400 + 0x04 * (n))
#define RNP_ETH_TUPLE5_SDPQF(n) (RNP_ETH_BASE + 0xc800 + 0x04 * (n))
#define RNP_ETH_TUPLE5_FTQF(n) (RNP_ETH_BASE + 0xcc00 + 0x04 * (n))
#define RNP_ETH_TUPLE5_POLICY(n) (RNP_ETH_BASE + 0xd000 + 0x04 * (n))
#define RNP_ETH_RSS_INDIR_TBL(p, n)                                            \
	(RNP_ETH_BASE + 0xe000 + 0x04 * (n) + 0x200 * (p))
#define RNP_ETH_TC_IPH_OFFSET_TABLE(n) (RNP_ETH_BASE + 0xe800 + 0x04 * (n))
#define RNP_ETH_TC_VLAN_OFFSET_TABLE(n) (RNP_ETH_BASE + 0xe820 + 0x04 * (n))
#define RNP_ETH_TC_PORT_OFFSET_TABLE(n) (RNP_ETH_BASE + 0xe840 + 0x04 * (n))
#define RNP_REDIR_RING_MASK (RNP_ETH_BASE + 0xe860)
#define RNP_ETH_RSS_MODE (0x6fe00)
#define RNP_ETH_RSS_INDIR_TBL_UV3P(n) (0x6ff00 + 0x04 * (n))

/* ================================================================== */

/* ==================== RNP-REG Global Registers ==================== */
#define RNP_COMM_REG0 0x30000
#define RNP_TOP_NIC_VERSION (RNP_COMM_REG0 + 0x0000)
#define RNP500_PHY_RELEASE (0x30000)
#define RNP500_TP_SFP (0x30200)
#define RNP500_TOP_NIC_VERSION (0x8000 + 0x0000)
#define RNP500_FPGA_VERSION (0x8020)
#define RNP500_FPGA_TIME (0x8024)
#define RNP500_LEGANCY_TIME (0xd000)
#define RNP500_LEGANCY_ENABLE (0xd004)
#define RNP_TOP_NIC_CONFIG (RNP_COMM_REG0 + 0x0004)
#define RNP_TOP_NIC_STAT (RNP_COMM_REG0 + 0x0008)
#define RNP_TOP_NIC_DUMMY (RNP_COMM_REG0 + 0x000c)
#define RNP_TOP_NIC_REST_N (RNP_COMM_REG0 + 0x0010)
#define NIC_RESET 0
#define RNP_TOP_DMA_MEM_SLP (RNP_COMM_REG0 + 0x4004)
#define RNP_TOP_DMA_MEM_SD (RNP_COMM_REG0 + 0x4008)
#define RNP_TOP_ETH_TIMESTAMP_SEL (RNP_COMM_REG0 + 0x8010)
#define RNP_TOP_ETH_MAC_CLK_SEL (RNP_COMM_REG0 + 0x8014)
#define RNP_TOP_ETH_INF_ETH_STATUS (RNP_COMM_REG0 + 0x8018)
#define RNP_TOP_ETH_BUG_40G_PATCH (RNP_COMM_REG0 + 0x801c)
#define RNP_TOP_ETH_PWR_PORT_NUM (4)
#define RNP_TOP_ETH_PWR_CLAMP_CTRL_PORT(n) (RNP_COMM_REG0 + 0x8020 + 0xc * (n))
#define RNP_TOP_ETH_PWR_ISOLATE_PORT(n) (RNP_COMM_REG0 + 0x8024 + 0xc * (n))
#define RNP_TOP_ETH_PWR_DOWN_PORT(n) (RNP_COMM_REG0 + 0x8028 + 0xc * (n))
#define RNP_TOP_ETH_TCAM_CONFIG_ENABLE (RNP_COMM_REG0 + 0x8050)
#define RNP_TOP_ETH_SLIP (RNP_COMM_REG0 + 0x8060)
#define RNP_TOP_ETH_SHUT_DOWN (RNP_COMM_REG0 + 0x8064)
#define RNP_TOP_ETH_OVS_SLIP (RNP_COMM_REG0 + 0x8068)
#define RNP_TOP_ETH_OVS_SHUT_DOWN (RNP_COMM_REG0 + 0x806c)
#define RNP_FC_PORT_ENABLE (RNP_COMM_REG0 + 0x9004)
#define RNP_FC_PORT_PRIO_MAP(n) (RNP_COMM_REG0 + 0x9008 + (0x04 * n))
#define RNP_FC_EN_CONF_AVAILBLE (RNP_COMM_REG0 + 0x9018)
#define RNP_FC_UNCTAGS_MAP_OFFSET (16)
#define RNP_TOP_MAC_OUI (RNP_COMM_REG0 + 0xc004)
#define RNP_TOP_MAC_SN (RNP_COMM_REG0 + 0xc008)
/* ================================================================== */

/* ==================== RNP-SERDES Global Registers ================= */
#define RNP_SERDES (0x40000)
#define RNP_PCS_OFFSET (0x1000)
#define RNP_PCS_BASE(i) (RNP_SERDES + RNP_PCS_OFFSET * i)
#define RNP_PCS_1G_OR_10G BIT(13)
#define RNP_PCS_SPPEED_MASK (0x1c)
#define RNP_PCS_SPPEED_10G (0x0)
#define RNP_PCS_SPPEED_40G (0xc)
#define RNP_PCS_LINK_SPEED (0x30000)
#define RNP_PCS_LINKUP BIT(2)
#define RNP_PCS_LINK_STATUS (0x30001)
/* ================================================================== */

/* ==================== RNP-MAC Global Registers ==================== */
#define RNP10_MAC_BASE (0x60000)
#define RNP_XLMAC (0x60000)
#define RNP10_MAC_TX_CFG (0x0000)
#define RNP10_MAC_RX_CFG (0x0004)
#define RNP_RX_ALL BIT(31)
#define RNP_RX_ALL_MUL BIT(4)
#define RNP10_MAC_PKT_FLT (0x0008)
#define RNP10_MAC_LPI_CTRL (0x00d0)
#define RNP10_MAC_Q0_TX_FLOW_CTRL(i) (0x0070 + 0x04 * (i))
#define RNP10_MAC_RX_FLOW_CTRL (0x0090)
#define RNP10_TX_FLOW_ENABLE_MASK (0x2)
#define RNP10_RX_FLOW_ENABLE_MASK (0x1)
#define RNP10_MAC_TX_VLAN_TAG (0x0050)
#define RNP10_MAC_TX_VLAN_MODE (0x0060)
#define RNP10_MAC_INNER_VLAN_INCL (0x0064)
#define RNP10_MAC_UNICAST_LOW(i) (0x304 + i * 0x08)
#define RNP10_MAC_UNICAST_HIGH(i) (0x300 + i * 0x08)
#define RNP500_MAC_BASE (0x20000)
#define RNP_MODE_NO_SA_INSER (0x0)
#define RNP_SARC_OFFSET (28)
#define RNP_TWOKPE_MASK BIT(27)
#define RNP_SFTERR_MASK BIT(26)
#define RNP_CST_MASK BIT(25)
#define RNP_TC_MASK BIT(24)
#define RNP_WD_MASK BIT(23)
#define RNP_JD_MASK BIT(22)
#define RNP_BE_MASK BIT(21)
#define RNP_JE_MASK BIT(20)
#define RNP_IFG_96 (0x00)
#define RNP_IFG_OFFSET (17)
#define RNP_DCRS_MASK BIT(16)
#define RNP_PS_MASK BIT(15)
#define RNP_FES_MASK BIT(14)
#define RNP_DO_MASK BIT(13)
#define RNP_LM_MASK BIT(12)
#define RNP_DM_MASK BIT(11)
#define RNP_IPC_MASK BIT(10)
#define RNP_DR_MASK BIT(9)
#define RNP_LUD_MASK BIT(8)
#define RNP_ACS_MASK BIT(7)
#define RNP_BL_MODE (0x00)
#define RNP_BL_OFFSET (5)
#define RNP_DC_MASK BIT(4)
#define RNP_TE_MASK BIT(3)
#define RNP_RE_MASK BIT(2)
#define RNP_PRELEN_MODE (0)
#define RNP500_MAC_UNICAST_LOW(i) (0x44 + i * 0x08)
#define RNP500_MAC_UNICAST_HIGH(i) (0x40 + i * 0x08)
#define GMAC_CONTROL 0x00000000 /* Configuration */
#define GMAC_FRAME_FILTER 0x00000004 /* Frame Filter */
#define GMAC_HASH_HIGH 0x00000008 /* Multicast Hash Table High */
#define GMAC_HASH_LOW 0x0000000c /* Multicast Hash Table Low */
#define GMAC_MII_ADDR 0x00000010 /* MII Address */
#define GMAC_MII_DATA 0x00000014 /* MII Data */
#define GMAC_FLOW_CTRL 0x00000018 /* Flow Control */
#define GMAC_PMT 0x0000002c

enum power_event {
	pointer_reset = 0x80000000,
	global_unicast = 0x00000200,
	wake_up_rx_frame = 0x00000040,
	magic_frame = 0x00000020,
	wake_up_frame_en = 0x00000004,
	magic_pkt_en = 0x00000002,
	power_down = 0x00000001,
};

#define GMAC_VTHM_MASK BIT(19)
#define GMAC_ESVL_MASK BIT(18)
#define GMAC_VTIM_MASK BIT(17)
#define GMAC_ETV_MASK BIT(16)
#define GMAC_VLAN_TAG_CTRL 0x0000001c
#define GMAC_CONTROL_DCRS 0x00010000 /* Disable carrier sense */
#define GMAC_CONTROL_PS 0x00008000 /* Port Select 0:GMI 1:MII */
#define GMAC_CONTROL_FES 0x00004000 /* Speed 0:10 1:100 */
#define GMAC_CONTROL_DO 0x00002000 /* Disable Rx Own */
#define GMAC_CONTROL_LM 0x00001000 /* Loop-back mode */
#define GMAC_CONTROL_DM 0x00000800 /* Duplex Mode */
#define GMAC_CONTROL_IPC 0x00000400 /* Checksum Offload */
#define GMAC_CONTROL_DR 0x00000200 /* Disable Retry */
#define GMAC_CONTROL_LUD 0x00000100 /* Link up/down */
#define GMAC_CONTROL_ACS 0x00000080 /* Auto Pad/FCS Stripping */
#define GMAC_CONTROL_DC 0x00000010 /* Deferral Check */
#define GMAC_CONTROL_TE 0x00000008 /* Transmitter Enable */
#define GMAC_CONTROL_RE 0x00000004 /* Receiver Enable */
/* GMAC Frame Filter defines */
#define GMAC_FRAME_FILTER_PR 0x00000001 /* Promiscuous Mode */
#define GMAC_FRAME_FILTER_HUC 0x00000002 /* Hash Unicast */
#define GMAC_FRAME_FILTER_HMC 0x00000004 /* Hash Multicast */
#define GMAC_FRAME_FILTER_DAIF 0x00000008 /* DA Inverse Filtering */
#define GMAC_FRAME_FILTER_PM 0x00000010 /* Pass all multicast */
#define GMAC_FRAME_FILTER_DBF 0x00000020 /* Disable Broadcast frames */
#define GMAC_FRAME_FILTER_PCF 0x00000080 /* Pass Control frames */
#define GMAC_FRAME_FILTER_SAIF 0x00000100 /* Inverse Filtering */
#define GMAC_FRAME_FILTER_SAF 0x00000200 /* Source Address Filter */
#define GMAC_FRAME_FILTER_HPF 0x00000400 /* Hash or perfect Filter */
#define GMAC_FRAME_FILTER_VLAN 0x00010000 /* vlan filter open */
#define GMAC_FRAME_FILTER_RA 0x80000000 /* Receive all mode */
/* GMII ADDR  defines */
#define GMAC_MII_ADDR_WRITE 0x00000002 /* MII Write */
#define GMAC_MII_ADDR_BUSY 0x00000001 /* MII Busy */
/* GMAC FLOW CTRL defines */
#define GMAC_FLOW_CTRL_PT_MASK 0xffff0000 /* Pause Time Mask */
#define GMAC_FLOW_CTRL_PT_SHIFT 16
#define GMAC_FLOW_CTRL_UP 0x00000008 /* Unicast pause frame enable */
#define GMAC_FLOW_CTRL_RFE 0x00000004 /* Rx Flow Control Enable */
#define GMAC_FLOW_CTRL_TFE 0x00000002 /* Tx Flow Control Enable */
#define GMAC_FLOW_CTRL_FCB_BPA 0x00000001 /* Flow Control Busy ... */
/* Energy Efficient Ethernet (EEE)
 *
 * LPI status, timer and control register offset
 */
/* EEE and LPI defines */
#define CORE_IRQ_TX_PATH_IN_LPI_MODE (1 << 0)
#define CORE_IRQ_TX_PATH_EXIT_LPI_MODE (1 << 1)
#define CORE_IRQ_RX_PATH_IN_LPI_MODE (1 << 2)
#define CORE_IRQ_RX_PATH_EXIT_LPI_MODE (1 << 3)
#define GMAC_LPI_CTRL_STATUS 0x0030
#define GMAC_LPI_TIMER_CTRL 0x0034
#define GMAC_INT_STATUS 0x00000038 /* interrupt status register */
#define GMAC_INT_STATUS_PMT BIT(3)
#define GMAC_INT_STATUS_MMCIS BIT(4)
#define GMAC_INT_STATUS_MMCRIS BIT(5)
#define GMAC_INT_STATUS_MMCTIS BIT(6)
#define GMAC_INT_STATUS_MMCCSUM BIT(7)
#define GMAC_INT_STATUS_TSTAMP BIT(9)
#define GMAC_INT_STATUS_LPIIS BIT(10)
/* LPI control and status defines */
#define LPI_CTRL_STATUS_LPITXA 0x00080000 /* Enable LPI TX Automate */
#define LPI_CTRL_STATUS_PLSEN 0x00040000 /* Enable PHY Link Status */
#define LPI_CTRL_STATUS_PLS 0x00020000 /* PHY Link Status */
#define LPI_CTRL_STATUS_LPIEN 0x00010000 /* LPI Enable */
#define LPI_CTRL_STATUS_RLPIST 0x00000200 /* Receive LPI state */
#define LPI_CTRL_STATUS_TLPIST 0x00000100 /* Transmit LPI state */
#define LPI_CTRL_STATUS_RLPIEX 0x00000008 /* Receive LPI Exit */
#define LPI_CTRL_STATUS_RLPIEN 0x00000004 /* Receive LPI Entry */
#define LPI_CTRL_STATUS_TLPIEX 0x00000002 /* Transmit LPI Exit */
#define LPI_CTRL_STATUS_TLPIEN 0x00000001 /* Transmit LPI Entry */
#define GMAC_MANAGEMENT_RX_UNDERSIZE (0x01a4)
#define GMAC_MANAGEMENT_TX_PAUSE (0x170)
#define GMAC_MANAGEMENT_RX_PAUSE (0x1D0)
#define RNP_MAC_TX_CFG (RNP_XLMAC + 0x0000)
#define RNP_MAC_RX_CFG (RNP_XLMAC + 0x0004)
#define RNP_MAC_PKT_FLT (RNP_XLMAC + 0x0008)
#define RNP_MAC_LPI_CTRL (RNP_XLMAC + 0x00d0)
#define RNP_MAC_TX_VLAN_TAG (RNP_XLMAC + 0x0050)
#define RNP_MAC_TX_VLAN_MODE (RNP_XLMAC + 0x0060)
#define RNP_MAC_INNER_VLAN_INCL (RNP_XLMAC + 0x0064)
#define RNP_MAC_Q0_TX_FLOW_CTRL(i) (RNP_XLMAC + 0x0070 + 0x04 * (i))
#define RNP_MAC_RX_FLOW_CTRL (RNP_XLMAC + 0x0090)
#define RNP_MAC_HW_FEATURE (RNP_XLMAC + 0x0120)
/*1588 */
#define RNP_MAC_TS_CTRL (RNP_XLMAC + 0X0d00)
#define RNP_MAC_SUB_SECOND_INCREMENT (RNP_XLMAC + 0x0d04)
#define RNP_MAC_SYS_TIME_SEC_CFG (RNP_XLMAC + 0x0d08)
#define RNP_MAC_SYS_TIME_NANOSEC_CFG (RNP_XLMAC + 0x0d0c)
#define RNP_MAC_SYS_TIME_SEC_UPDATE (RNP_XLMAC + 0x0d10)
#define RNP_MAC_SYS_TIME_NANOSEC_UPDATE (RNP_XLMAC + 0x0d14)
#define RNP_MAC_TS_ADDEND (RNP_XLMAC + 0x0d18)
#define RNP_MAC_TS_STATS (RNP_XLMAC + 0x0d20)
#define RNP_MAC_INTERRUPT_ENABLE (RNP_XLMAC + 0x00b4)
#define RNP_MAC_STATS_BROADCAST_LOW (RNP_XLMAC + 0x0918)
#define RNP_MAC_STATS_BROADCAST_HIGH (RNP_XLMAC + 0x091c)
#define RNP_MAC_STATS_MULTICAST_LOW (RNP_XLMAC + 0x0920)
#define RNP_MAC_STATS_MULTICAST_HIGH (RNP_XLMAC + 0x0924)
#define RNP_TX_FLOW_ENABLE_MASK (0x2)
#define RNP_RX_FLOW_ENABLE_MASK (0x1)
/* ================================================================== */

/* ==================== RNP-MSIX Global Registers ==================== */
#define RING_VECTOR(n) (0x04 * (n))
/* ================================================================== */

/* ==================== OTHER Global Registers ==================== */
/* =====  PF-VF Functions ==== */
#define VF_NUM_REG 0xa3000
/* 8bit: 7:vf_actiove 6:fun0/fun1 [5:0]:vf_num */
#define VF_NUM(vfnum, fun) ((1 << 7) | (((fun) & 0x1) << 6) | ((vfnum) & 0x3f))
#define PF_NUM(fun) (((fun) & 0x1) << 6)
#define IS_VF(vfnum) (((vfnum) & (1 << 7)) ? 1 : 0)
/* 8bit: 7:vf_actiove [6:5]:fun0/fun1 [4:0]:vf_num */
#define PF_NUM_N500(fun) (((fun) & 0x3) << 5)
/* PFC Flow Control*/

enum NIC_MODE {
	MODE_NIC_MODE_2PORT_40G = 0,
	MODE_NIC_MODE_2PORT_10G = 1,
	MODE_NIC_MODE_4PORT_10G = 2,
	MODE_NIC_MODE_8PORT_10G = 3,
};
/* ================================================================== */

#endif /* RNPGBE_REGS_H */

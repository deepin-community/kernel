/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2024, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_FEATURE_H
#define _NE6X_FEATURE_H

#define NE6X_F_RSS              BIT(0)
#define NE6X_F_PROMISC          BIT(1)
#define NE6X_F_RX_IPV4_CKSUM    BIT(2)
#define NE6X_F_RX_UDP_CKSUM     BIT(3)
#define NE6X_F_RX_TCP_CKSUM     BIT(4)
#define NE6X_F_RX_SCTP_CKSUM    BIT(5)
#define NE6X_F_RX_VLAN_STRIP    BIT(6)
#define NE6X_F_RX_QINQ_STRIP    BIT(7)
#define NE6X_F_RX_VLAN_FILTER   BIT(8)
#define NE6X_F_LRO              BIT(9)
#define NE6X_F_RX_DISABLE       BIT(10)
#define NE6X_F_RX_FW_LLDP       BIT(11)
#define NE6X_F_RX_ALLMULTI      BIT(12)
#define NE6X_F_FLOW_STEERING    BIT(15)
#define NE6X_F_TX_VLAN          BIT(16)
#define NE6X_F_TX_IP_CKSUM      BIT(17)
#define NE6X_F_TX_TCP_CKSUM     BIT(18)
#define NE6X_F_TX_UDP_CKSUM     BIT(19)
#define NE6X_F_TX_SCTP_CKSUM    BIT(20)
#define NE6X_F_TX_TCP_SEG       BIT(21)
#define NE6X_F_TX_UDP_SEG       BIT(22)
#define NE6X_F_TX_QINQ          BIT(23)
#define NE6X_F_TX_NIC_SWITCH    BIT(24)
#define NE6X_F_TX_MAC_LEARN     BIT(25)
#define NE6X_F_TX_DISABLE       BIT(26)
#define NE6X_F_TX_QOSBANDWIDTH  BIT(27)
#define NE6X_F_TX_UDP_TNL_SEG   BIT(28)
#define	NE6X_F_TX_UDP_TNL_CSUM  BIT(29)

#define NE6X_OFFLOAD_RSS	NE6X_F_RSS
#define NE6X_OFFLOAD_RXCSUM	(NE6X_F_RX_IPV4_CKSUM | \
				 NE6X_F_RX_UDP_CKSUM | \
				 NE6X_F_RX_TCP_CKSUM | \
				 NE6X_F_RX_SCTP_CKSUM)
#define NE6X_OFFLOAD_TXCSUM	(NE6X_F_TX_IP_CKSUM | \
				 NE6X_F_TX_TCP_CKSUM | \
				 NE6X_F_TX_UDP_CKSUM | \
				 NE6X_F_TX_UDP_TNL_CSUM)

#define NE6X_OFFLOAD_LRO	NE6X_F_LRO
#define NE6X_OFFLOAD_TSO	NE6X_F_TX_TCP_SEG
#define NE6X_OFFLOAD_UFO	NE6X_F_TX_UDP_SEG
#define NE6X_OFFLOAD_SCTP_CSUM	NE6X_F_TX_SCTP_CKSUM

#define NE6X_OFFLOAD_RXD_VLAN	(NE6X_F_RX_VLAN_STRIP | \
				 NE6X_F_RX_QINQ_STRIP | \
				 NE6X_F_RX_VLAN_FILTER)
#define NE6X_OFFLOAD_TXD_VLAN	(NE6X_F_TX_VLAN | NE6X_F_TX_QINQ)
#define NE6X_OFFLOAD_L2		 NE6X_F_TX_NIC_SWITCH

#define NE6X_F_SMART_ENABLED         BIT(0)
#define NE6X_F_SRIOV_ENABLED         BIT(1)
#define NE6X_F_SWITCH_ENABLED        BIT(2)
#define NE6X_F_L2FDB_LEARN_ENABLED   BIT(3)
#define NE6X_F_VLAN_ENABLED          BIT(4)
#define NE6X_F_WHITELIST_ENABLED     BIT(5)
#define NE6X_F_DDOS_ENABLED          BIT(6)
#define NE6X_F_TRUST_VLAN_ENABLED    BIT(7)
#define NE6X_F_S_ROCE_ICRC_ENABLED   BIT(8)

#define NE6X_F_ACK_FLOOD             BIT(0)
#define NE6X_F_PUSH_ACK_FLOOD        BIT(1)
#define NE6X_F_SYN_ACK_FLOOD         BIT(2)
#define NE6X_F_FIN_FLOOD             BIT(3)
#define NE6X_F_RST_FLOOD             BIT(4)
#define NE6X_F_PUSH_SYN_ACK_FLOOD    BIT(5)
#define NE6X_F_UDP_FLOOD             BIT(6)
#define NE6X_F_ICMP_FLOOD            BIT(7)
#define NE6X_F_FRAGMENT_FLOOD        BIT(8)

#endif

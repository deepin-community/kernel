/*******************************************************************************

  WangXun(R) 10GbE PCI Express Virtual Function Linux Network Driver
  Copyright(c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Software Team <linux.nic@trustnetic.com>
  WangXun Technology, HuaXing Times Square A507, Hangzhou, China.

*******************************************************************************/

#ifndef _TXGBE_TXRX_H_
#define _TXGBE_TXRX_H_

#include <net/ip.h>
#include "txgbe_type.h"
#include "txgbe.h"
#include "txgbe_common.h"

union network_header {
	struct iphdr *ipv4;
	struct ipv6hdr *ipv6;
	void *raw;
};

/**
 * Packet Type
 * PTYPE:8 = TUN:2 + PKT:2 + TYP:4
 **/
#define TXGBE_PTYPE_PKT(_pt) ((_pt) & 0x30)
#define TXGBE_PTYPE_TYP(_pt) ((_pt) & 0x0F)
#define TXGBE_PTYPE_TYPL4(_pt) ((_pt) & 0x07)

/* TUN */
#define TXGBE_PTYPE_TUN(_pt) ((_pt) & 0xC0)
#define TXGBE_PTYPE_TUN_IPV4            (0x80)
#define TXGBE_PTYPE_TUN_IPV6            (0xC0)

/* PKT for TUN */
#define TXGBE_PTYPE_PKT_IPIP            (0x00) /* IP+IP */
#define TXGBE_PTYPE_PKT_IG              (0x10) /* IP+GRE */
#define TXGBE_PTYPE_PKT_IGM             (0x20) /* IP+GRE+MAC */
#define TXGBE_PTYPE_PKT_IGMV            (0x30) /* IP+GRE+MAC+VLAN */
/* PKT for !TUN */
#define TXGBE_PTYPE_PKT_MAC             (0x10)
#define TXGBE_PTYPE_PKT_IP              (0x20)
#define TXGBE_PTYPE_PKT_FCOE            (0x30)

/* TYP for PKT=mac */
#define TXGBE_PTYPE_TYP_MAC             (0x01)
#define TXGBE_PTYPE_TYP_TS              (0x02) /* time sync */
#define TXGBE_PTYPE_TYP_FIP             (0x03)
#define TXGBE_PTYPE_TYP_LLDP            (0x04)
#define TXGBE_PTYPE_TYP_CNM             (0x05)
#define TXGBE_PTYPE_TYP_EAPOL           (0x06)
#define TXGBE_PTYPE_TYP_ARP             (0x07)
/* TYP for PKT=ip */
#define TXGBE_PTYPE_PKT_IPV6            (0x08)
#define TXGBE_PTYPE_TYP_IPFRAG          (0x01)
#define TXGBE_PTYPE_TYP_IPDATA          (0x02)
#define TXGBE_PTYPE_TYP_UDP             (0x03)
#define TXGBE_PTYPE_TYP_TCP             (0x04)
#define TXGBE_PTYPE_TYP_SCTP            (0x05)
/* TYP for PKT=fcoe */
#define TXGBE_PTYPE_PKT_VFT             (0x08)
#define TXGBE_PTYPE_TYP_FCOE            (0x00)
#define TXGBE_PTYPE_TYP_FCDATA          (0x01)
#define TXGBE_PTYPE_TYP_FCRDY           (0x02)
#define TXGBE_PTYPE_TYP_FCRSP           (0x03)
#define TXGBE_PTYPE_TYP_FCOTHER         (0x04)

/* packet type non-ip values */
enum txgbe_l2_ptypes {
	TXGBE_PTYPE_L2_ABORTED = (TXGBE_PTYPE_PKT_MAC),
	TXGBE_PTYPE_L2_MAC = (TXGBE_PTYPE_PKT_MAC | TXGBE_PTYPE_TYP_MAC),
	TXGBE_PTYPE_L2_TS = (TXGBE_PTYPE_PKT_MAC | TXGBE_PTYPE_TYP_TS),
	TXGBE_PTYPE_L2_FIP = (TXGBE_PTYPE_PKT_MAC | TXGBE_PTYPE_TYP_FIP),
	TXGBE_PTYPE_L2_LLDP = (TXGBE_PTYPE_PKT_MAC | TXGBE_PTYPE_TYP_LLDP),
	TXGBE_PTYPE_L2_CNM = (TXGBE_PTYPE_PKT_MAC | TXGBE_PTYPE_TYP_CNM),
	TXGBE_PTYPE_L2_EAPOL = (TXGBE_PTYPE_PKT_MAC | TXGBE_PTYPE_TYP_EAPOL),
	TXGBE_PTYPE_L2_ARP = (TXGBE_PTYPE_PKT_MAC | TXGBE_PTYPE_TYP_ARP),

	TXGBE_PTYPE_L2_IPV4_FRAG = (TXGBE_PTYPE_PKT_IP | TXGBE_PTYPE_TYP_IPFRAG),
	TXGBE_PTYPE_L2_IPV4 = (TXGBE_PTYPE_PKT_IP | TXGBE_PTYPE_TYP_IPDATA),
	TXGBE_PTYPE_L2_IPV4_UDP = (TXGBE_PTYPE_PKT_IP | TXGBE_PTYPE_TYP_UDP),
	TXGBE_PTYPE_L2_IPV4_TCP = (TXGBE_PTYPE_PKT_IP | TXGBE_PTYPE_TYP_TCP),
	TXGBE_PTYPE_L2_IPV4_SCTP = (TXGBE_PTYPE_PKT_IP | TXGBE_PTYPE_TYP_SCTP),
	TXGBE_PTYPE_L2_IPV6_FRAG = (TXGBE_PTYPE_PKT_IP | TXGBE_PTYPE_PKT_IPV6 |
			TXGBE_PTYPE_TYP_IPFRAG),
	TXGBE_PTYPE_L2_IPV6 = (TXGBE_PTYPE_PKT_IP | TXGBE_PTYPE_PKT_IPV6 |
			TXGBE_PTYPE_TYP_IPDATA),
	TXGBE_PTYPE_L2_IPV6_UDP = (TXGBE_PTYPE_PKT_IP | TXGBE_PTYPE_PKT_IPV6 |
			TXGBE_PTYPE_TYP_UDP),
	TXGBE_PTYPE_L2_IPV6_TCP = (TXGBE_PTYPE_PKT_IP | TXGBE_PTYPE_PKT_IPV6 |
			TXGBE_PTYPE_TYP_TCP),
	TXGBE_PTYPE_L2_IPV6_SCTP = (TXGBE_PTYPE_PKT_IP | TXGBE_PTYPE_PKT_IPV6 |
			TXGBE_PTYPE_TYP_SCTP),

	TXGBE_PTYPE_L2_FCOE = (TXGBE_PTYPE_PKT_FCOE |
			TXGBE_PTYPE_TYP_FCOE),
	TXGBE_PTYPE_L2_FCOE_FCDATA = (TXGBE_PTYPE_PKT_FCOE |
			TXGBE_PTYPE_TYP_FCDATA),
	TXGBE_PTYPE_L2_FCOE_FCRDY = (TXGBE_PTYPE_PKT_FCOE |
			TXGBE_PTYPE_TYP_FCRDY),
	TXGBE_PTYPE_L2_FCOE_FCRSP = (TXGBE_PTYPE_PKT_FCOE |
			TXGBE_PTYPE_TYP_FCRSP),
	TXGBE_PTYPE_L2_FCOE_FCOTHER = (TXGBE_PTYPE_PKT_FCOE |
			TXGBE_PTYPE_TYP_FCOTHER),
	TXGBE_PTYPE_L2_FCOE_VFT = (TXGBE_PTYPE_PKT_FCOE |
			TXGBE_PTYPE_PKT_VFT),
	TXGBE_PTYPE_L2_FCOE_VFT_FCDATA = (TXGBE_PTYPE_PKT_FCOE |
			TXGBE_PTYPE_PKT_VFT | TXGBE_PTYPE_TYP_FCDATA),
	TXGBE_PTYPE_L2_FCOE_VFT_FCRDY = (TXGBE_PTYPE_PKT_FCOE |
			TXGBE_PTYPE_PKT_VFT | TXGBE_PTYPE_TYP_FCRDY),
	TXGBE_PTYPE_L2_FCOE_VFT_FCRSP = (TXGBE_PTYPE_PKT_FCOE |
			TXGBE_PTYPE_PKT_VFT | TXGBE_PTYPE_TYP_FCRSP),
	TXGBE_PTYPE_L2_FCOE_VFT_FCOTHER = (TXGBE_PTYPE_PKT_FCOE |
			TXGBE_PTYPE_PKT_VFT | TXGBE_PTYPE_TYP_FCOTHER),

	TXGBE_PTYPE_L2_TUN4_MAC = (TXGBE_PTYPE_TUN_IPV4 | TXGBE_PTYPE_PKT_IGM),
	TXGBE_PTYPE_L2_TUN6_MAC = (TXGBE_PTYPE_TUN_IPV6 | TXGBE_PTYPE_PKT_IGM),
};

/**
 * Packet Type decoding
 **/
/* txgbe_dec_ptype.mac: outer mac */
enum txgbe_dec_ptype_mac {
	TXGBE_DEC_PTYPE_MAC_IP = 0,
	TXGBE_DEC_PTYPE_MAC_L2 = 2,
	TXGBE_DEC_PTYPE_MAC_FCOE = 3,
};

/* txgbe_dec_ptype.[e]ip: outer&encaped ip */
#define TXGBE_DEC_PTYPE_IP_FRAG (0x4)
enum txgbe_dec_ptype_ip {
	TXGBE_DEC_PTYPE_IP_NONE = 0,
	TXGBE_DEC_PTYPE_IP_IPV4 = 1,
	TXGBE_DEC_PTYPE_IP_IPV6 = 2,
	TXGBE_DEC_PTYPE_IP_FGV4 =
			(TXGBE_DEC_PTYPE_IP_FRAG | TXGBE_DEC_PTYPE_IP_IPV4),
	TXGBE_DEC_PTYPE_IP_FGV6 =
			(TXGBE_DEC_PTYPE_IP_FRAG | TXGBE_DEC_PTYPE_IP_IPV6),
};

/* txgbe_dec_ptype.etype: encaped type */
enum txgbe_dec_ptype_etype {
	TXGBE_DEC_PTYPE_ETYPE_NONE = 0,
	TXGBE_DEC_PTYPE_ETYPE_IPIP = 1, /* IP+IP */
	TXGBE_DEC_PTYPE_ETYPE_IG = 2, /* IP+GRE */
	TXGBE_DEC_PTYPE_ETYPE_IGM = 3, /* IP+GRE+MAC */
	TXGBE_DEC_PTYPE_ETYPE_IGMV = 4, /* IP+GRE+MAC+VLAN */
};

/* txgbe_dec_ptype.proto: payload proto */
enum txgbe_dec_ptype_prot {
	TXGBE_DEC_PTYPE_PROT_NONE = 0,
	TXGBE_DEC_PTYPE_PROT_UDP = 1,
	TXGBE_DEC_PTYPE_PROT_TCP = 2,
	TXGBE_DEC_PTYPE_PROT_SCTP = 3,
	TXGBE_DEC_PTYPE_PROT_ICMP = 4,
	TXGBE_DEC_PTYPE_PROT_TS = 5, /* time sync */
};

/* txgbe_dec_ptype.layer: payload layer */
enum txgbe_dec_ptype_layer {
	TXGBE_DEC_PTYPE_LAYER_NONE = 0,
	TXGBE_DEC_PTYPE_LAYER_PAY2 = 1,
	TXGBE_DEC_PTYPE_LAYER_PAY3 = 2,
	TXGBE_DEC_PTYPE_LAYER_PAY4 = 3,
};

struct txgbe_dec_ptype {
	u32 ptype:8;
	u32 known:1;
	u32 mac:2; /* outer mac */
	u32 ip:3; /* outer ip*/
	u32 etype:3; /* encaped type */
	u32 eip:3; /* encaped ip */
	u32 prot:4; /* payload proto */
	u32 layer:3; /* payload layer */
};
typedef struct txgbe_dec_ptype txgbe_dptype;

txgbe_dptype txgbe_rx_decode_ptype(const union txgbe_rx_desc*);
txgbe_dptype txgbe_tx_encode_ptype(const struct txgbe_tx_buffer*);

#endif /* _TXGBE_TXRX_H_ */

/*******************************************************************************

  WangXun(R) GbE PCI Express Virtual Function Linux Network Driver
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

#ifndef _NGBE_TXRX_H_
#define _NGBE_TXRX_H_

#include "ngbe_type.h"
#include "ngbe.h"
#include "ngbe_common.h"

union network_header {
	struct iphdr *ipv4;
	struct ipv6hdr *ipv6;
};

/**
 * Packet Type
 * PTYPE:8 = TUN:2 + PKT:2 + TYP:4
 **/
#define NGBE_PTYPE_PKT(_pt) ((_pt) & 0x30)
#define NGBE_PTYPE_TYP(_pt) ((_pt) & 0x0F)
#define NGBE_PTYPE_TYPL4(_pt) ((_pt) & 0x07)

/* TUN */
#define NGBE_PTYPE_TUN(_pt) ((_pt) & 0xC0)
#define NGBE_PTYPE_TUN_IPV4            (0x80)
#define NGBE_PTYPE_TUN_IPV6            (0xC0)

/* PKT for TUN */
#define NGBE_PTYPE_PKT_IPIP            (0x00) /* IP+IP */
#define NGBE_PTYPE_PKT_IG              (0x10) /* IP+GRE */
#define NGBE_PTYPE_PKT_IGM             (0x20) /* IP+GRE+MAC */
#define NGBE_PTYPE_PKT_IGMV            (0x30) /* IP+GRE+MAC+VLAN */
/* PKT for !TUN */
#define NGBE_PTYPE_PKT_MAC             (0x10)
#define NGBE_PTYPE_PKT_IP              (0x20)
#define NGBE_PTYPE_PKT_FCOE            (0x30)

/* TYP for PKT=mac */
#define NGBE_PTYPE_TYP_MAC             (0x01)
#define NGBE_PTYPE_TYP_TS              (0x02) /* time sync */
#define NGBE_PTYPE_TYP_FIP             (0x03)
#define NGBE_PTYPE_TYP_LLDP            (0x04)
#define NGBE_PTYPE_TYP_CNM             (0x05)
#define NGBE_PTYPE_TYP_EAPOL           (0x06)
#define NGBE_PTYPE_TYP_ARP             (0x07)
/* TYP for PKT=ip */
#define NGBE_PTYPE_PKT_IPV6            (0x08)
#define NGBE_PTYPE_TYP_IPFRAG          (0x01)
#define NGBE_PTYPE_TYP_IPDATA          (0x02)
#define NGBE_PTYPE_TYP_UDP             (0x03)
#define NGBE_PTYPE_TYP_TCP             (0x04)
#define NGBE_PTYPE_TYP_SCTP            (0x05)
/* TYP for PKT=fcoe */
#define NGBE_PTYPE_PKT_VFT             (0x08)
#define NGBE_PTYPE_TYP_FCOE            (0x00)
#define NGBE_PTYPE_TYP_FCDATA          (0x01)
#define NGBE_PTYPE_TYP_FCRDY           (0x02)
#define NGBE_PTYPE_TYP_FCRSP           (0x03)
#define NGBE_PTYPE_TYP_FCOTHER         (0x04)

/* packet type non-ip values */
enum ngbe_l2_ptypes {
	NGBE_PTYPE_L2_ABORTED = (NGBE_PTYPE_PKT_MAC),
	NGBE_PTYPE_L2_MAC = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_MAC),
	NGBE_PTYPE_L2_TS = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_TS),
	NGBE_PTYPE_L2_FIP = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_FIP),
	NGBE_PTYPE_L2_LLDP = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_LLDP),
	NGBE_PTYPE_L2_CNM = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_CNM),
	NGBE_PTYPE_L2_EAPOL = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_EAPOL),
	NGBE_PTYPE_L2_ARP = (NGBE_PTYPE_PKT_MAC | NGBE_PTYPE_TYP_ARP),

	NGBE_PTYPE_L2_IPV4_FRAG = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_TYP_IPFRAG),
	NGBE_PTYPE_L2_IPV4 = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_TYP_IPDATA),
	NGBE_PTYPE_L2_IPV4_UDP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_TYP_UDP),
	NGBE_PTYPE_L2_IPV4_TCP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_TYP_TCP),
	NGBE_PTYPE_L2_IPV4_SCTP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_TYP_SCTP),
	NGBE_PTYPE_L2_IPV6_FRAG = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_PKT_IPV6 |
			NGBE_PTYPE_TYP_IPFRAG),
	NGBE_PTYPE_L2_IPV6 = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_PKT_IPV6 |
			NGBE_PTYPE_TYP_IPDATA),
	NGBE_PTYPE_L2_IPV6_UDP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_PKT_IPV6 |
			NGBE_PTYPE_TYP_UDP),
	NGBE_PTYPE_L2_IPV6_TCP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_PKT_IPV6 |
			NGBE_PTYPE_TYP_TCP),
	NGBE_PTYPE_L2_IPV6_SCTP = (NGBE_PTYPE_PKT_IP | NGBE_PTYPE_PKT_IPV6 |
			NGBE_PTYPE_TYP_SCTP),

	NGBE_PTYPE_L2_FCOE = (NGBE_PTYPE_PKT_FCOE |
			NGBE_PTYPE_TYP_FCOE),
	NGBE_PTYPE_L2_FCOE_FCDATA = (NGBE_PTYPE_PKT_FCOE |
			NGBE_PTYPE_TYP_FCDATA),
	NGBE_PTYPE_L2_FCOE_FCRDY = (NGBE_PTYPE_PKT_FCOE |
			NGBE_PTYPE_TYP_FCRDY),
	NGBE_PTYPE_L2_FCOE_FCRSP = (NGBE_PTYPE_PKT_FCOE |
			NGBE_PTYPE_TYP_FCRSP),
	NGBE_PTYPE_L2_FCOE_FCOTHER = (NGBE_PTYPE_PKT_FCOE |
			NGBE_PTYPE_TYP_FCOTHER),
	NGBE_PTYPE_L2_FCOE_VFT = (NGBE_PTYPE_PKT_FCOE |
			NGBE_PTYPE_PKT_VFT),
	NGBE_PTYPE_L2_FCOE_VFT_FCDATA = (NGBE_PTYPE_PKT_FCOE |
			NGBE_PTYPE_PKT_VFT | NGBE_PTYPE_TYP_FCDATA),
	NGBE_PTYPE_L2_FCOE_VFT_FCRDY = (NGBE_PTYPE_PKT_FCOE |
			NGBE_PTYPE_PKT_VFT | NGBE_PTYPE_TYP_FCRDY),
	NGBE_PTYPE_L2_FCOE_VFT_FCRSP = (NGBE_PTYPE_PKT_FCOE |
			NGBE_PTYPE_PKT_VFT | NGBE_PTYPE_TYP_FCRSP),
	NGBE_PTYPE_L2_FCOE_VFT_FCOTHER = (NGBE_PTYPE_PKT_FCOE |
			NGBE_PTYPE_PKT_VFT | NGBE_PTYPE_TYP_FCOTHER),

	NGBE_PTYPE_L2_TUN4_MAC = (NGBE_PTYPE_TUN_IPV4 | NGBE_PTYPE_PKT_IGM),
	NGBE_PTYPE_L2_TUN6_MAC = (NGBE_PTYPE_TUN_IPV6 | NGBE_PTYPE_PKT_IGM),
};

/**
 * Packet Type decoding
 **/
/* ngbe_dec_ptype.mac: outer mac */
enum ngbe_dec_ptype_mac {
	NGBE_DEC_PTYPE_MAC_IP = 0,
	NGBE_DEC_PTYPE_MAC_L2 = 2,
	NGBE_DEC_PTYPE_MAC_FCOE = 3,
};

/* ngbe_dec_ptype.[e]ip: outer&encaped ip */
#define NGBE_DEC_PTYPE_IP_FRAG (0x4)
enum ngbe_dec_ptype_ip {
	NGBE_DEC_PTYPE_IP_NONE = 0,
	NGBE_DEC_PTYPE_IP_IPV4 = 1,
	NGBE_DEC_PTYPE_IP_IPV6 = 2,
	NGBE_DEC_PTYPE_IP_FGV4 =
			(NGBE_DEC_PTYPE_IP_FRAG | NGBE_DEC_PTYPE_IP_IPV4),
	NGBE_DEC_PTYPE_IP_FGV6 =
			(NGBE_DEC_PTYPE_IP_FRAG | NGBE_DEC_PTYPE_IP_IPV6),
};

/* ngbe_dec_ptype.etype: encaped type */
enum ngbe_dec_ptype_etype {
	NGBE_DEC_PTYPE_ETYPE_NONE = 0,
	NGBE_DEC_PTYPE_ETYPE_IPIP = 1, /* IP+IP */
	NGBE_DEC_PTYPE_ETYPE_IG = 2, /* IP+GRE */
	NGBE_DEC_PTYPE_ETYPE_IGM = 3, /* IP+GRE+MAC */
	NGBE_DEC_PTYPE_ETYPE_IGMV = 4, /* IP+GRE+MAC+VLAN */
};

/* ngbe_dec_ptype.proto: payload proto */
enum ngbe_dec_ptype_prot {
	NGBE_DEC_PTYPE_PROT_NONE = 0,
	NGBE_DEC_PTYPE_PROT_UDP = 1,
	NGBE_DEC_PTYPE_PROT_TCP = 2,
	NGBE_DEC_PTYPE_PROT_SCTP = 3,
	NGBE_DEC_PTYPE_PROT_ICMP = 4,
	NGBE_DEC_PTYPE_PROT_TS = 5, /* time sync */
};

/* ngbe_dec_ptype.layer: payload layer */
enum ngbe_dec_ptype_layer {
	NGBE_DEC_PTYPE_LAYER_NONE = 0,
	NGBE_DEC_PTYPE_LAYER_PAY2 = 1,
	NGBE_DEC_PTYPE_LAYER_PAY3 = 2,
	NGBE_DEC_PTYPE_LAYER_PAY4 = 3,
};

struct ngbe_dec_ptype {
	u32 ptype:8;
	u32 known:1;
	u32 mac:2; /* outer mac */
	u32 ip:3; /* outer ip*/
	u32 etype:3; /* encaped type */
	u32 eip:3; /* encaped ip */
	u32 prot:4; /* payload proto */
	u32 layer:3; /* payload layer */
};
typedef struct ngbe_dec_ptype ngbe_dptype;

ngbe_dptype ngbe_rx_decode_ptype(const union ngbe_rx_desc*);
ngbe_dptype ngbe_tx_encode_ptype(const struct ngbe_tx_buffer*);

#endif /* _NGBE_TXRX_H_ */

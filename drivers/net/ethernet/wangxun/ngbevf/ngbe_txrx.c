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

#include <linux/prefetch.h>
#include <net/busy_poll.h>
#include "ngbe_txrx.h"

/* The ngbe_ptype_lookup is used to convert from the 8-bit ptype in the
 * hardware to a bit-field that can be used by SW to more easily determine the
 * packet type.
 *
 * Macros are used to shorten the table lines and make this table human
 * readable.
 *
 * We store the PTYPE in the top byte of the bit field - this is just so that
 * we can check that the table doesn't have a row missing, as the index into
 * the table should be the PTYPE.
 *
 * Typical work flow:
 *
 * IF NOT ngbe_ptype_lookup[ptype].known
 * THEN
 *      Packet is unknown
 * ELSE IF ngbe_ptype_lookup[ptype].mac == NGBE_DEC_PTYPE_MAC_IP
 *      Use the rest of the fields to look at the tunnels, inner protocols, etc
 * ELSE
 *      Use the enum ngbe_l2_ptypes to decode the packet type
 * ENDIF
 */

/* macro to make the table lines short */
#define NGBE_PTT(ptype, mac, ip, etype, eip, proto, layer)\
	{       ptype, \
		1, \
		/* mac     */ NGBE_DEC_PTYPE_MAC_##mac, \
		/* ip      */ NGBE_DEC_PTYPE_IP_##ip, \
		/* etype   */ NGBE_DEC_PTYPE_ETYPE_##etype, \
		/* eip     */ NGBE_DEC_PTYPE_IP_##eip, \
		/* proto   */ NGBE_DEC_PTYPE_PROT_##proto, \
		/* layer   */ NGBE_DEC_PTYPE_LAYER_##layer }

#define NGBE_UKN(ptype) \
		{ ptype, 0, 0, 0, 0, 0, 0, 0 }

/* Lookup table mapping the HW PTYPE to the bit field for decoding */
/* for ((pt=0;pt<256;pt++)); do printf "macro(0x%02X),\n" $pt; done */
#define NGBE_PTYPE_MAX (256)
ngbe_dptype ngbe_ptype_lookup[NGBE_PTYPE_MAX] = {
	NGBE_UKN(0x00),
	NGBE_UKN(0x01),
	NGBE_UKN(0x02),
	NGBE_UKN(0x03),
	NGBE_UKN(0x04),
	NGBE_UKN(0x05),
	NGBE_UKN(0x06),
	NGBE_UKN(0x07),
	NGBE_UKN(0x08),
	NGBE_UKN(0x09),
	NGBE_UKN(0x0A),
	NGBE_UKN(0x0B),
	NGBE_UKN(0x0C),
	NGBE_UKN(0x0D),
	NGBE_UKN(0x0E),
	NGBE_UKN(0x0F),

	/* L2: mac */
	NGBE_UKN(0x10),
	NGBE_PTT(0x11, L2, NONE, NONE, NONE, NONE, PAY2),
	NGBE_PTT(0x12, L2, NONE, NONE, NONE, TS,   PAY2),
	NGBE_PTT(0x13, L2, NONE, NONE, NONE, NONE, PAY2),
	NGBE_PTT(0x14, L2, NONE, NONE, NONE, NONE, PAY2),
	NGBE_PTT(0x15, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x16, L2, NONE, NONE, NONE, NONE, PAY2),
	NGBE_PTT(0x17, L2, NONE, NONE, NONE, NONE, NONE),

	/* L2: ethertype filter */
	NGBE_PTT(0x18, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x19, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1A, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1B, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1C, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1D, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1E, L2, NONE, NONE, NONE, NONE, NONE),
	NGBE_PTT(0x1F, L2, NONE, NONE, NONE, NONE, NONE),

	/* L3: ip non-tunnel */
	NGBE_UKN(0x20),
	NGBE_PTT(0x21, IP, FGV4, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x22, IP, IPV4, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x23, IP, IPV4, NONE, NONE, UDP,  PAY4),
	NGBE_PTT(0x24, IP, IPV4, NONE, NONE, TCP,  PAY4),
	NGBE_PTT(0x25, IP, IPV4, NONE, NONE, SCTP, PAY4),
	NGBE_UKN(0x26),
	NGBE_UKN(0x27),
	NGBE_UKN(0x28),
	NGBE_PTT(0x29, IP, FGV6, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x2A, IP, IPV6, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x2B, IP, IPV6, NONE, NONE, UDP,  PAY3),
	NGBE_PTT(0x2C, IP, IPV6, NONE, NONE, TCP,  PAY4),
	NGBE_PTT(0x2D, IP, IPV6, NONE, NONE, SCTP, PAY4),
	NGBE_UKN(0x2E),
	NGBE_UKN(0x2F),

	/* L2: fcoe */
	NGBE_PTT(0x30, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x31, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x32, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x33, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x34, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x35, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x36, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x37, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x38, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x39, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_UKN(0x3A),
	NGBE_UKN(0x3B),
	NGBE_UKN(0x3C),
	NGBE_UKN(0x3D),
	NGBE_UKN(0x3E),
	NGBE_UKN(0x3F),

	NGBE_UKN(0x40),
	NGBE_UKN(0x41),
	NGBE_UKN(0x42),
	NGBE_UKN(0x43),
	NGBE_UKN(0x44),
	NGBE_UKN(0x45),
	NGBE_UKN(0x46),
	NGBE_UKN(0x47),
	NGBE_UKN(0x48),
	NGBE_UKN(0x49),
	NGBE_UKN(0x4A),
	NGBE_UKN(0x4B),
	NGBE_UKN(0x4C),
	NGBE_UKN(0x4D),
	NGBE_UKN(0x4E),
	NGBE_UKN(0x4F),
	NGBE_UKN(0x50),
	NGBE_UKN(0x51),
	NGBE_UKN(0x52),
	NGBE_UKN(0x53),
	NGBE_UKN(0x54),
	NGBE_UKN(0x55),
	NGBE_UKN(0x56),
	NGBE_UKN(0x57),
	NGBE_UKN(0x58),
	NGBE_UKN(0x59),
	NGBE_UKN(0x5A),
	NGBE_UKN(0x5B),
	NGBE_UKN(0x5C),
	NGBE_UKN(0x5D),
	NGBE_UKN(0x5E),
	NGBE_UKN(0x5F),
	NGBE_UKN(0x60),
	NGBE_UKN(0x61),
	NGBE_UKN(0x62),
	NGBE_UKN(0x63),
	NGBE_UKN(0x64),
	NGBE_UKN(0x65),
	NGBE_UKN(0x66),
	NGBE_UKN(0x67),
	NGBE_UKN(0x68),
	NGBE_UKN(0x69),
	NGBE_UKN(0x6A),
	NGBE_UKN(0x6B),
	NGBE_UKN(0x6C),
	NGBE_UKN(0x6D),
	NGBE_UKN(0x6E),
	NGBE_UKN(0x6F),
	NGBE_UKN(0x70),
	NGBE_UKN(0x71),
	NGBE_UKN(0x72),
	NGBE_UKN(0x73),
	NGBE_UKN(0x74),
	NGBE_UKN(0x75),
	NGBE_UKN(0x76),
	NGBE_UKN(0x77),
	NGBE_UKN(0x78),
	NGBE_UKN(0x79),
	NGBE_UKN(0x7A),
	NGBE_UKN(0x7B),
	NGBE_UKN(0x7C),
	NGBE_UKN(0x7D),
	NGBE_UKN(0x7E),
	NGBE_UKN(0x7F),

	/* IPv4 --> IPv4/IPv6 */
	NGBE_UKN(0x80),
	NGBE_PTT(0x81, IP, IPV4, IPIP, FGV4, NONE, PAY3),
	NGBE_PTT(0x82, IP, IPV4, IPIP, IPV4, NONE, PAY3),
	NGBE_PTT(0x83, IP, IPV4, IPIP, IPV4, UDP,  PAY4),
	NGBE_PTT(0x84, IP, IPV4, IPIP, IPV4, TCP,  PAY4),
	NGBE_PTT(0x85, IP, IPV4, IPIP, IPV4, SCTP, PAY4),
	NGBE_UKN(0x86),
	NGBE_UKN(0x87),
	NGBE_UKN(0x88),
	NGBE_PTT(0x89, IP, IPV4, IPIP, FGV6, NONE, PAY3),
	NGBE_PTT(0x8A, IP, IPV4, IPIP, IPV6, NONE, PAY3),
	NGBE_PTT(0x8B, IP, IPV4, IPIP, IPV6, UDP,  PAY4),
	NGBE_PTT(0x8C, IP, IPV4, IPIP, IPV6, TCP,  PAY4),
	NGBE_PTT(0x8D, IP, IPV4, IPIP, IPV6, SCTP, PAY4),
	NGBE_UKN(0x8E),
	NGBE_UKN(0x8F),

	/* IPv4 --> GRE/NAT --> NONE/IPv4/IPv6 */
	NGBE_PTT(0x90, IP, IPV4, IG, NONE, NONE, PAY3),
	NGBE_PTT(0x91, IP, IPV4, IG, FGV4, NONE, PAY3),
	NGBE_PTT(0x92, IP, IPV4, IG, IPV4, NONE, PAY3),
	NGBE_PTT(0x93, IP, IPV4, IG, IPV4, UDP,  PAY4),
	NGBE_PTT(0x94, IP, IPV4, IG, IPV4, TCP,  PAY4),
	NGBE_PTT(0x95, IP, IPV4, IG, IPV4, SCTP, PAY4),
	NGBE_UKN(0x96),
	NGBE_UKN(0x97),
	NGBE_UKN(0x98),
	NGBE_PTT(0x99, IP, IPV4, IG, FGV6, NONE, PAY3),
	NGBE_PTT(0x9A, IP, IPV4, IG, IPV6, NONE, PAY3),
	NGBE_PTT(0x9B, IP, IPV4, IG, IPV6, UDP,  PAY4),
	NGBE_PTT(0x9C, IP, IPV4, IG, IPV6, TCP,  PAY4),
	NGBE_PTT(0x9D, IP, IPV4, IG, IPV6, SCTP, PAY4),
	NGBE_UKN(0x9E),
	NGBE_UKN(0x9F),

	/* IPv4 --> GRE/NAT --> MAC --> NONE/IPv4/IPv6 */
	NGBE_PTT(0xA0, IP, IPV4, IGM, NONE, NONE, PAY3),
	NGBE_PTT(0xA1, IP, IPV4, IGM, FGV4, NONE, PAY3),
	NGBE_PTT(0xA2, IP, IPV4, IGM, IPV4, NONE, PAY3),
	NGBE_PTT(0xA3, IP, IPV4, IGM, IPV4, UDP,  PAY4),
	NGBE_PTT(0xA4, IP, IPV4, IGM, IPV4, TCP,  PAY4),
	NGBE_PTT(0xA5, IP, IPV4, IGM, IPV4, SCTP, PAY4),
	NGBE_UKN(0xA6),
	NGBE_UKN(0xA7),
	NGBE_UKN(0xA8),
	NGBE_PTT(0xA9, IP, IPV4, IGM, FGV6, NONE, PAY3),
	NGBE_PTT(0xAA, IP, IPV4, IGM, IPV6, NONE, PAY3),
	NGBE_PTT(0xAB, IP, IPV4, IGM, IPV6, UDP,  PAY4),
	NGBE_PTT(0xAC, IP, IPV4, IGM, IPV6, TCP,  PAY4),
	NGBE_PTT(0xAD, IP, IPV4, IGM, IPV6, SCTP, PAY4),
	NGBE_UKN(0xAE),
	NGBE_UKN(0xAF),

	/* IPv4 --> GRE/NAT --> MAC+VLAN --> NONE/IPv4/IPv6 */
	NGBE_PTT(0xB0, IP, IPV4, IGMV, NONE, NONE, PAY3),
	NGBE_PTT(0xB1, IP, IPV4, IGMV, FGV4, NONE, PAY3),
	NGBE_PTT(0xB2, IP, IPV4, IGMV, IPV4, NONE, PAY3),
	NGBE_PTT(0xB3, IP, IPV4, IGMV, IPV4, UDP,  PAY4),
	NGBE_PTT(0xB4, IP, IPV4, IGMV, IPV4, TCP,  PAY4),
	NGBE_PTT(0xB5, IP, IPV4, IGMV, IPV4, SCTP, PAY4),
	NGBE_UKN(0xB6),
	NGBE_UKN(0xB7),
	NGBE_UKN(0xB8),
	NGBE_PTT(0xB9, IP, IPV4, IGMV, FGV6, NONE, PAY3),
	NGBE_PTT(0xBA, IP, IPV4, IGMV, IPV6, NONE, PAY3),
	NGBE_PTT(0xBB, IP, IPV4, IGMV, IPV6, UDP,  PAY4),
	NGBE_PTT(0xBC, IP, IPV4, IGMV, IPV6, TCP,  PAY4),
	NGBE_PTT(0xBD, IP, IPV4, IGMV, IPV6, SCTP, PAY4),
	NGBE_UKN(0xBE),
	NGBE_UKN(0xBF),

	/* IPv6 --> IPv4/IPv6 */
	NGBE_UKN(0xC0),
	NGBE_PTT(0xC1, IP, IPV6, IPIP, FGV4, NONE, PAY3),
	NGBE_PTT(0xC2, IP, IPV6, IPIP, IPV4, NONE, PAY3),
	NGBE_PTT(0xC3, IP, IPV6, IPIP, IPV4, UDP,  PAY4),
	NGBE_PTT(0xC4, IP, IPV6, IPIP, IPV4, TCP,  PAY4),
	NGBE_PTT(0xC5, IP, IPV6, IPIP, IPV4, SCTP, PAY4),
	NGBE_UKN(0xC6),
	NGBE_UKN(0xC7),
	NGBE_UKN(0xC8),
	NGBE_PTT(0xC9, IP, IPV6, IPIP, FGV6, NONE, PAY3),
	NGBE_PTT(0xCA, IP, IPV6, IPIP, IPV6, NONE, PAY3),
	NGBE_PTT(0xCB, IP, IPV6, IPIP, IPV6, UDP,  PAY4),
	NGBE_PTT(0xCC, IP, IPV6, IPIP, IPV6, TCP,  PAY4),
	NGBE_PTT(0xCD, IP, IPV6, IPIP, IPV6, SCTP, PAY4),
	NGBE_UKN(0xCE),
	NGBE_UKN(0xCF),

	/* IPv6 --> GRE/NAT -> NONE/IPv4/IPv6 */
	NGBE_PTT(0xD0, IP, IPV6, IG,   NONE, NONE, PAY3),
	NGBE_PTT(0xD1, IP, IPV6, IG,   FGV4, NONE, PAY3),
	NGBE_PTT(0xD2, IP, IPV6, IG,   IPV4, NONE, PAY3),
	NGBE_PTT(0xD3, IP, IPV6, IG,   IPV4, UDP,  PAY4),
	NGBE_PTT(0xD4, IP, IPV6, IG,   IPV4, TCP,  PAY4),
	NGBE_PTT(0xD5, IP, IPV6, IG,   IPV4, SCTP, PAY4),
	NGBE_UKN(0xD6),
	NGBE_UKN(0xD7),
	NGBE_UKN(0xD8),
	NGBE_PTT(0xD9, IP, IPV6, IG,   FGV6, NONE, PAY3),
	NGBE_PTT(0xDA, IP, IPV6, IG,   IPV6, NONE, PAY3),
	NGBE_PTT(0xDB, IP, IPV6, IG,   IPV6, UDP,  PAY4),
	NGBE_PTT(0xDC, IP, IPV6, IG,   IPV6, TCP,  PAY4),
	NGBE_PTT(0xDD, IP, IPV6, IG,   IPV6, SCTP, PAY4),
	NGBE_UKN(0xDE),
	NGBE_UKN(0xDF),

	/* IPv6 --> GRE/NAT -> MAC -> NONE/IPv4/IPv6 */
	NGBE_PTT(0xE0, IP, IPV6, IGM,  NONE, NONE, PAY3),
	NGBE_PTT(0xE1, IP, IPV6, IGM,  FGV4, NONE, PAY3),
	NGBE_PTT(0xE2, IP, IPV6, IGM,  IPV4, NONE, PAY3),
	NGBE_PTT(0xE3, IP, IPV6, IGM,  IPV4, UDP,  PAY4),
	NGBE_PTT(0xE4, IP, IPV6, IGM,  IPV4, TCP,  PAY4),
	NGBE_PTT(0xE5, IP, IPV6, IGM,  IPV4, SCTP, PAY4),
	NGBE_UKN(0xE6),
	NGBE_UKN(0xE7),
	NGBE_UKN(0xE8),
	NGBE_PTT(0xE9, IP, IPV6, IGM,  FGV6, NONE, PAY3),
	NGBE_PTT(0xEA, IP, IPV6, IGM,  IPV6, NONE, PAY3),
	NGBE_PTT(0xEB, IP, IPV6, IGM,  IPV6, UDP,  PAY4),
	NGBE_PTT(0xEC, IP, IPV6, IGM,  IPV6, TCP,  PAY4),
	NGBE_PTT(0xED, IP, IPV6, IGM,  IPV6, SCTP, PAY4),
	NGBE_UKN(0xEE),
	NGBE_UKN(0xEF),

	/* IPv6 --> GRE/NAT -> MAC--> NONE/IPv */
	NGBE_PTT(0xF0, IP, IPV6, IGMV, NONE, NONE, PAY3),
	NGBE_PTT(0xF1, IP, IPV6, IGMV, FGV4, NONE, PAY3),
	NGBE_PTT(0xF2, IP, IPV6, IGMV, IPV4, NONE, PAY3),
	NGBE_PTT(0xF3, IP, IPV6, IGMV, IPV4, UDP,  PAY4),
	NGBE_PTT(0xF4, IP, IPV6, IGMV, IPV4, TCP,  PAY4),
	NGBE_PTT(0xF5, IP, IPV6, IGMV, IPV4, SCTP, PAY4),
	NGBE_UKN(0xF6),
	NGBE_UKN(0xF7),
	NGBE_UKN(0xF8),
	NGBE_PTT(0xF9, IP, IPV6, IGMV, FGV6, NONE, PAY3),
	NGBE_PTT(0xFA, IP, IPV6, IGMV, IPV6, NONE, PAY3),
	NGBE_PTT(0xFB, IP, IPV6, IGMV, IPV6, UDP,  PAY4),
	NGBE_PTT(0xFC, IP, IPV6, IGMV, IPV6, TCP,  PAY4),
	NGBE_PTT(0xFD, IP, IPV6, IGMV, IPV6, SCTP, PAY4),
	NGBE_UKN(0xFE),
	NGBE_UKN(0xFF),
};

static inline ngbe_dptype ngbe_decode_ptype(const u8 ptype)
{
	return ngbe_ptype_lookup[ptype];
}

static u8 get_ipv6_proto(struct sk_buff *skb, int offset)
{
	struct ipv6hdr *iphdr = (struct ipv6hdr*)(skb->data + offset);
	u8 nexthdr = iphdr->nexthdr;

	offset += sizeof(struct ipv6hdr);

	while (ipv6_ext_hdr(nexthdr)) {
		struct ipv6_opt_hdr _hdr, *hp;

		if (nexthdr == NEXTHDR_NONE)
			break;

		hp = skb_header_pointer(skb, offset, sizeof(_hdr), &_hdr);
		if (!hp)
			break;

		if (nexthdr == NEXTHDR_FRAGMENT)
			break;
		else if (nexthdr == NEXTHDR_AUTH)
			offset +=  ipv6_authlen(hp);
		else
			offset +=  ipv6_optlen(hp);

		nexthdr = hp->nexthdr;
	}

	return nexthdr;
}

static int parse_eth_nethdr(const struct ngbe_tx_buffer *first,
		u8 *ptype, union network_header *nethdr)
{
	struct sk_buff *skb = first->skb;

	switch (first->protocol) {
	case __constant_htons(ETH_P_1588):
		*ptype = NGBE_PTYPE_L2_TS;
		return 1;
	case __constant_htons(ETH_P_FIP):
		*ptype = NGBE_PTYPE_L2_FIP;
		return 1;
	case __constant_htons(ETH_P_LLDP):
		*ptype = NGBE_PTYPE_L2_LLDP;
		return 1;
	case __constant_htons(ETH_P_CNM):
		*ptype = NGBE_PTYPE_L2_CNM;
		return 1;
	case __constant_htons(ETH_P_PAE):
		*ptype = NGBE_PTYPE_L2_EAPOL;
		return 1;
	case __constant_htons(ETH_P_ARP):
		*ptype = NGBE_PTYPE_L2_ARP;
		return 1;
	case __constant_htons(ETH_P_IP):
		nethdr->ipv4 = ip_hdr(skb);
		*ptype = NGBE_PTYPE_PKT_IP;
		break;
#ifdef NETIF_F_IPV6_CSUM
	case __constant_htons(ETH_P_IPV6):
		nethdr->ipv6 = ipv6_hdr(skb);
		*ptype = NGBE_PTYPE_PKT_IP;
		break;
#endif
	default:
		*ptype = NGBE_PTYPE_L2_MAC;
		return 1;
	}

	return 0;
}

#ifdef HAVE_ENCAP_TSO_OFFLOAD
static int parse_tun_nethdr(const struct ngbe_tx_buffer *first,
	u8 *ptype, union network_header *nethdr)
{
	struct sk_buff *skb = first->skb;
	u8 tun_prot;

	switch (first->protocol) {
	case __constant_htons(ETH_P_IP):
		tun_prot = ip_hdr(skb)->protocol;
		if (ip_is_fragment(ip_hdr(skb)))
			return parse_eth_nethdr(first, ptype, nethdr);
		*ptype = NGBE_PTYPE_TUN_IPV4;
		break;
	case __constant_htons(ETH_P_IPV6):
		tun_prot = get_ipv6_proto(skb, (u8*)ipv6_hdr(skb) - skb->data);
		if (tun_prot == NEXTHDR_FRAGMENT)
			return parse_eth_nethdr(first, ptype, nethdr);
		*ptype = NGBE_PTYPE_TUN_IPV6;
		break;
	default:
		/* fallback to ethernet packet */
		return parse_eth_nethdr(first, ptype, nethdr);
	}

	switch (tun_prot) {
	case IPPROTO_IPIP:
		nethdr->ipv4 = inner_ip_hdr(skb);
		*ptype |= NGBE_PTYPE_PKT_IPIP;
		/* fall through */
	case IPPROTO_UDP:
		nethdr->ipv4 = inner_ip_hdr(skb);
#ifdef SUPPORT
		/* fixme: VXLAN-GPE neither ETHER nor IP */
		if (skb->inner_protocol_type != ENCAP_TYPE_ETHER ||
		    skb->inner_protocol != htons(ETH_P_TEB)) {
			*ptype |= NGBE_PTYPE_PKT_IG;
			/* fall through */
		} else {
			/* fall through */
			struct ethhdr *mac =
				(struct ethhdr *)skb_inner_mac_header(skb);
			if (mac->h_proto == htons(ETH_P_8021Q)) {
				*ptype |= NGBE_PTYPE_PKT_IGMV;
			} else {
				*ptype |= NGBE_PTYPE_PKT_IGM;
			}
		}
#endif
		/* fall through */
	case IPPROTO_GRE:
		nethdr->ipv4 = inner_ip_hdr(skb);
		if (skb->inner_protocol ==  htons(ETH_P_IP) ||
		    skb->inner_protocol ==  htons(ETH_P_IPV6)) {
			*ptype |= NGBE_PTYPE_PKT_IG;
			/* fall through */
		} else {
			struct ethhdr *mac =
				(struct ethhdr *)skb_inner_mac_header(skb);
			if (mac->h_proto == htons(ETH_P_8021Q)) {
				*ptype |= NGBE_PTYPE_PKT_IGMV;
			} else {
				*ptype |= NGBE_PTYPE_PKT_IGM;
			}
		}
		/* fall through */
	default:
		/* fallback to ethernet packet */
		return parse_eth_nethdr(first, ptype, nethdr);
	}

	return 0;
}
#endif /* HAVE_ENCAP_TSO_OFFLOAD */

ngbe_dptype ngbe_rx_decode_ptype(const union ngbe_rx_desc *rx_desc)
{
	return ngbe_decode_ptype(NGBE_RXD_PKTTYPE(rx_desc));
}

ngbe_dptype ngbe_tx_encode_ptype(const struct ngbe_tx_buffer *first)
{
	struct sk_buff *skb = first->skb;
	union network_header nethdr = { NULL };
	u8 proto, ptype;

#ifdef HAVE_ENCAP_TSO_OFFLOAD
	if (skb->encapsulation)
		parse_tun_nethdr(first, &ptype, &nethdr);
	else
#endif
		parse_eth_nethdr(first, &ptype, &nethdr);

	/* parse inner L3 header */
	if (nethdr.ipv4 == NULL)
		goto exit;

	switch (nethdr.ipv4->version) {
	case 4:
		proto = nethdr.ipv4->protocol;
		if (ip_is_fragment(nethdr.ipv4)) {
			ptype |= NGBE_PTYPE_TYP_IPFRAG;
			goto exit;
		}
		break;
	case 6:
		ptype |= NGBE_PTYPE_PKT_IPV6;
		proto = get_ipv6_proto(skb, (u8*)nethdr.ipv6 - skb->data);
		if (proto == NEXTHDR_FRAGMENT) {
			ptype |= NGBE_PTYPE_TYP_IPFRAG;
			goto exit;
		}
		break;
	default:
		ptype = NGBE_PTYPE_L2_MAC;
		goto exit;
	}

	/* parse inner L4 header */
	switch (proto) {
	case IPPROTO_TCP:
		ptype |= NGBE_PTYPE_TYP_TCP;
		break;
	case IPPROTO_UDP:
		ptype |= NGBE_PTYPE_TYP_UDP;
		break;
	case IPPROTO_SCTP:
		ptype |= NGBE_PTYPE_TYP_SCTP;
		break;
	default:
		ptype |= NGBE_PTYPE_TYP_IPDATA;
		break;
	}

exit:
	return ngbe_decode_ptype(ptype);
}


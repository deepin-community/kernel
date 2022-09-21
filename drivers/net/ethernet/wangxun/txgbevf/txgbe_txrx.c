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

#include <linux/prefetch.h>
#ifdef CONFIG_NET_RX_BUSY_POLL
#include <net/busy_poll.h>
#ifdef HAVE_NDO_BUSY_POLL
#define BP_EXTENDED_STATS
#endif
#endif/*CONFIG_NET_RX_BUSY_POLL*/
#include "txgbe_txrx.h"

/* The txgbe_ptype_lookup is used to convert from the 8-bit ptype in the
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
 * IF NOT txgbe_ptype_lookup[ptype].known
 * THEN
 *      Packet is unknown
 * ELSE IF txgbe_ptype_lookup[ptype].mac == TXGBE_DEC_PTYPE_MAC_IP
 *      Use the rest of the fields to look at the tunnels, inner protocols, etc
 * ELSE
 *      Use the enum txgbe_l2_ptypes to decode the packet type
 * ENDIF
 */

/* macro to make the table lines short */
#define TXGBE_PTT(ptype, mac, ip, etype, eip, proto, layer)\
	{       ptype, \
		1, \
		/* mac     */ TXGBE_DEC_PTYPE_MAC_##mac, \
		/* ip      */ TXGBE_DEC_PTYPE_IP_##ip, \
		/* etype   */ TXGBE_DEC_PTYPE_ETYPE_##etype, \
		/* eip     */ TXGBE_DEC_PTYPE_IP_##eip, \
		/* proto   */ TXGBE_DEC_PTYPE_PROT_##proto, \
		/* layer   */ TXGBE_DEC_PTYPE_LAYER_##layer }

#define TXGBE_UKN(ptype) \
		{ ptype, 0, 0, 0, 0, 0, 0, 0 }

/* Lookup table mapping the HW PTYPE to the bit field for decoding */
/* for ((pt=0;pt<256;pt++)); do printf "macro(0x%02X),\n" $pt; done */
#define TXGBE_PTYPE_MAX (256)
txgbe_dptype txgbe_ptype_lookup[TXGBE_PTYPE_MAX] = {
	TXGBE_UKN(0x00),
	TXGBE_UKN(0x01),
	TXGBE_UKN(0x02),
	TXGBE_UKN(0x03),
	TXGBE_UKN(0x04),
	TXGBE_UKN(0x05),
	TXGBE_UKN(0x06),
	TXGBE_UKN(0x07),
	TXGBE_UKN(0x08),
	TXGBE_UKN(0x09),
	TXGBE_UKN(0x0A),
	TXGBE_UKN(0x0B),
	TXGBE_UKN(0x0C),
	TXGBE_UKN(0x0D),
	TXGBE_UKN(0x0E),
	TXGBE_UKN(0x0F),

	/* L2: mac */
	TXGBE_UKN(0x10),
	TXGBE_PTT(0x11, L2, NONE, NONE, NONE, NONE, PAY2),
	TXGBE_PTT(0x12, L2, NONE, NONE, NONE, TS,   PAY2),
	TXGBE_PTT(0x13, L2, NONE, NONE, NONE, NONE, PAY2),
	TXGBE_PTT(0x14, L2, NONE, NONE, NONE, NONE, PAY2),
	TXGBE_PTT(0x15, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x16, L2, NONE, NONE, NONE, NONE, PAY2),
	TXGBE_PTT(0x17, L2, NONE, NONE, NONE, NONE, NONE),

	/* L2: ethertype filter */
	TXGBE_PTT(0x18, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x19, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1A, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1B, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1C, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1D, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1E, L2, NONE, NONE, NONE, NONE, NONE),
	TXGBE_PTT(0x1F, L2, NONE, NONE, NONE, NONE, NONE),

	/* L3: ip non-tunnel */
	TXGBE_UKN(0x20),
	TXGBE_PTT(0x21, IP, FGV4, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x22, IP, IPV4, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x23, IP, IPV4, NONE, NONE, UDP,  PAY4),
	TXGBE_PTT(0x24, IP, IPV4, NONE, NONE, TCP,  PAY4),
	TXGBE_PTT(0x25, IP, IPV4, NONE, NONE, SCTP, PAY4),
	TXGBE_UKN(0x26),
	TXGBE_UKN(0x27),
	TXGBE_UKN(0x28),
	TXGBE_PTT(0x29, IP, FGV6, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x2A, IP, IPV6, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x2B, IP, IPV6, NONE, NONE, UDP,  PAY3),
	TXGBE_PTT(0x2C, IP, IPV6, NONE, NONE, TCP,  PAY4),
	TXGBE_PTT(0x2D, IP, IPV6, NONE, NONE, SCTP, PAY4),
	TXGBE_UKN(0x2E),
	TXGBE_UKN(0x2F),

	/* L2: fcoe */
	TXGBE_PTT(0x30, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x31, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x32, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x33, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x34, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x35, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x36, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x37, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x38, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_PTT(0x39, FCOE, NONE, NONE, NONE, NONE, PAY3),
	TXGBE_UKN(0x3A),
	TXGBE_UKN(0x3B),
	TXGBE_UKN(0x3C),
	TXGBE_UKN(0x3D),
	TXGBE_UKN(0x3E),
	TXGBE_UKN(0x3F),

	TXGBE_UKN(0x40),
	TXGBE_UKN(0x41),
	TXGBE_UKN(0x42),
	TXGBE_UKN(0x43),
	TXGBE_UKN(0x44),
	TXGBE_UKN(0x45),
	TXGBE_UKN(0x46),
	TXGBE_UKN(0x47),
	TXGBE_UKN(0x48),
	TXGBE_UKN(0x49),
	TXGBE_UKN(0x4A),
	TXGBE_UKN(0x4B),
	TXGBE_UKN(0x4C),
	TXGBE_UKN(0x4D),
	TXGBE_UKN(0x4E),
	TXGBE_UKN(0x4F),
	TXGBE_UKN(0x50),
	TXGBE_UKN(0x51),
	TXGBE_UKN(0x52),
	TXGBE_UKN(0x53),
	TXGBE_UKN(0x54),
	TXGBE_UKN(0x55),
	TXGBE_UKN(0x56),
	TXGBE_UKN(0x57),
	TXGBE_UKN(0x58),
	TXGBE_UKN(0x59),
	TXGBE_UKN(0x5A),
	TXGBE_UKN(0x5B),
	TXGBE_UKN(0x5C),
	TXGBE_UKN(0x5D),
	TXGBE_UKN(0x5E),
	TXGBE_UKN(0x5F),
	TXGBE_UKN(0x60),
	TXGBE_UKN(0x61),
	TXGBE_UKN(0x62),
	TXGBE_UKN(0x63),
	TXGBE_UKN(0x64),
	TXGBE_UKN(0x65),
	TXGBE_UKN(0x66),
	TXGBE_UKN(0x67),
	TXGBE_UKN(0x68),
	TXGBE_UKN(0x69),
	TXGBE_UKN(0x6A),
	TXGBE_UKN(0x6B),
	TXGBE_UKN(0x6C),
	TXGBE_UKN(0x6D),
	TXGBE_UKN(0x6E),
	TXGBE_UKN(0x6F),
	TXGBE_UKN(0x70),
	TXGBE_UKN(0x71),
	TXGBE_UKN(0x72),
	TXGBE_UKN(0x73),
	TXGBE_UKN(0x74),
	TXGBE_UKN(0x75),
	TXGBE_UKN(0x76),
	TXGBE_UKN(0x77),
	TXGBE_UKN(0x78),
	TXGBE_UKN(0x79),
	TXGBE_UKN(0x7A),
	TXGBE_UKN(0x7B),
	TXGBE_UKN(0x7C),
	TXGBE_UKN(0x7D),
	TXGBE_UKN(0x7E),
	TXGBE_UKN(0x7F),

	/* IPv4 --> IPv4/IPv6 */
	TXGBE_UKN(0x80),
	TXGBE_PTT(0x81, IP, IPV4, IPIP, FGV4, NONE, PAY3),
	TXGBE_PTT(0x82, IP, IPV4, IPIP, IPV4, NONE, PAY3),
	TXGBE_PTT(0x83, IP, IPV4, IPIP, IPV4, UDP,  PAY4),
	TXGBE_PTT(0x84, IP, IPV4, IPIP, IPV4, TCP,  PAY4),
	TXGBE_PTT(0x85, IP, IPV4, IPIP, IPV4, SCTP, PAY4),
	TXGBE_UKN(0x86),
	TXGBE_UKN(0x87),
	TXGBE_UKN(0x88),
	TXGBE_PTT(0x89, IP, IPV4, IPIP, FGV6, NONE, PAY3),
	TXGBE_PTT(0x8A, IP, IPV4, IPIP, IPV6, NONE, PAY3),
	TXGBE_PTT(0x8B, IP, IPV4, IPIP, IPV6, UDP,  PAY4),
	TXGBE_PTT(0x8C, IP, IPV4, IPIP, IPV6, TCP,  PAY4),
	TXGBE_PTT(0x8D, IP, IPV4, IPIP, IPV6, SCTP, PAY4),
	TXGBE_UKN(0x8E),
	TXGBE_UKN(0x8F),

	/* IPv4 --> GRE/NAT --> NONE/IPv4/IPv6 */
	TXGBE_PTT(0x90, IP, IPV4, IG, NONE, NONE, PAY3),
	TXGBE_PTT(0x91, IP, IPV4, IG, FGV4, NONE, PAY3),
	TXGBE_PTT(0x92, IP, IPV4, IG, IPV4, NONE, PAY3),
	TXGBE_PTT(0x93, IP, IPV4, IG, IPV4, UDP,  PAY4),
	TXGBE_PTT(0x94, IP, IPV4, IG, IPV4, TCP,  PAY4),
	TXGBE_PTT(0x95, IP, IPV4, IG, IPV4, SCTP, PAY4),
	TXGBE_UKN(0x96),
	TXGBE_UKN(0x97),
	TXGBE_UKN(0x98),
	TXGBE_PTT(0x99, IP, IPV4, IG, FGV6, NONE, PAY3),
	TXGBE_PTT(0x9A, IP, IPV4, IG, IPV6, NONE, PAY3),
	TXGBE_PTT(0x9B, IP, IPV4, IG, IPV6, UDP,  PAY4),
	TXGBE_PTT(0x9C, IP, IPV4, IG, IPV6, TCP,  PAY4),
	TXGBE_PTT(0x9D, IP, IPV4, IG, IPV6, SCTP, PAY4),
	TXGBE_UKN(0x9E),
	TXGBE_UKN(0x9F),

	/* IPv4 --> GRE/NAT --> MAC --> NONE/IPv4/IPv6 */
	TXGBE_PTT(0xA0, IP, IPV4, IGM, NONE, NONE, PAY3),
	TXGBE_PTT(0xA1, IP, IPV4, IGM, FGV4, NONE, PAY3),
	TXGBE_PTT(0xA2, IP, IPV4, IGM, IPV4, NONE, PAY3),
	TXGBE_PTT(0xA3, IP, IPV4, IGM, IPV4, UDP,  PAY4),
	TXGBE_PTT(0xA4, IP, IPV4, IGM, IPV4, TCP,  PAY4),
	TXGBE_PTT(0xA5, IP, IPV4, IGM, IPV4, SCTP, PAY4),
	TXGBE_UKN(0xA6),
	TXGBE_UKN(0xA7),
	TXGBE_UKN(0xA8),
	TXGBE_PTT(0xA9, IP, IPV4, IGM, FGV6, NONE, PAY3),
	TXGBE_PTT(0xAA, IP, IPV4, IGM, IPV6, NONE, PAY3),
	TXGBE_PTT(0xAB, IP, IPV4, IGM, IPV6, UDP,  PAY4),
	TXGBE_PTT(0xAC, IP, IPV4, IGM, IPV6, TCP,  PAY4),
	TXGBE_PTT(0xAD, IP, IPV4, IGM, IPV6, SCTP, PAY4),
	TXGBE_UKN(0xAE),
	TXGBE_UKN(0xAF),

	/* IPv4 --> GRE/NAT --> MAC+VLAN --> NONE/IPv4/IPv6 */
	TXGBE_PTT(0xB0, IP, IPV4, IGMV, NONE, NONE, PAY3),
	TXGBE_PTT(0xB1, IP, IPV4, IGMV, FGV4, NONE, PAY3),
	TXGBE_PTT(0xB2, IP, IPV4, IGMV, IPV4, NONE, PAY3),
	TXGBE_PTT(0xB3, IP, IPV4, IGMV, IPV4, UDP,  PAY4),
	TXGBE_PTT(0xB4, IP, IPV4, IGMV, IPV4, TCP,  PAY4),
	TXGBE_PTT(0xB5, IP, IPV4, IGMV, IPV4, SCTP, PAY4),
	TXGBE_UKN(0xB6),
	TXGBE_UKN(0xB7),
	TXGBE_UKN(0xB8),
	TXGBE_PTT(0xB9, IP, IPV4, IGMV, FGV6, NONE, PAY3),
	TXGBE_PTT(0xBA, IP, IPV4, IGMV, IPV6, NONE, PAY3),
	TXGBE_PTT(0xBB, IP, IPV4, IGMV, IPV6, UDP,  PAY4),
	TXGBE_PTT(0xBC, IP, IPV4, IGMV, IPV6, TCP,  PAY4),
	TXGBE_PTT(0xBD, IP, IPV4, IGMV, IPV6, SCTP, PAY4),
	TXGBE_UKN(0xBE),
	TXGBE_UKN(0xBF),

	/* IPv6 --> IPv4/IPv6 */
	TXGBE_UKN(0xC0),
	TXGBE_PTT(0xC1, IP, IPV6, IPIP, FGV4, NONE, PAY3),
	TXGBE_PTT(0xC2, IP, IPV6, IPIP, IPV4, NONE, PAY3),
	TXGBE_PTT(0xC3, IP, IPV6, IPIP, IPV4, UDP,  PAY4),
	TXGBE_PTT(0xC4, IP, IPV6, IPIP, IPV4, TCP,  PAY4),
	TXGBE_PTT(0xC5, IP, IPV6, IPIP, IPV4, SCTP, PAY4),
	TXGBE_UKN(0xC6),
	TXGBE_UKN(0xC7),
	TXGBE_UKN(0xC8),
	TXGBE_PTT(0xC9, IP, IPV6, IPIP, FGV6, NONE, PAY3),
	TXGBE_PTT(0xCA, IP, IPV6, IPIP, IPV6, NONE, PAY3),
	TXGBE_PTT(0xCB, IP, IPV6, IPIP, IPV6, UDP,  PAY4),
	TXGBE_PTT(0xCC, IP, IPV6, IPIP, IPV6, TCP,  PAY4),
	TXGBE_PTT(0xCD, IP, IPV6, IPIP, IPV6, SCTP, PAY4),
	TXGBE_UKN(0xCE),
	TXGBE_UKN(0xCF),

	/* IPv6 --> GRE/NAT -> NONE/IPv4/IPv6 */
	TXGBE_PTT(0xD0, IP, IPV6, IG,   NONE, NONE, PAY3),
	TXGBE_PTT(0xD1, IP, IPV6, IG,   FGV4, NONE, PAY3),
	TXGBE_PTT(0xD2, IP, IPV6, IG,   IPV4, NONE, PAY3),
	TXGBE_PTT(0xD3, IP, IPV6, IG,   IPV4, UDP,  PAY4),
	TXGBE_PTT(0xD4, IP, IPV6, IG,   IPV4, TCP,  PAY4),
	TXGBE_PTT(0xD5, IP, IPV6, IG,   IPV4, SCTP, PAY4),
	TXGBE_UKN(0xD6),
	TXGBE_UKN(0xD7),
	TXGBE_UKN(0xD8),
	TXGBE_PTT(0xD9, IP, IPV6, IG,   FGV6, NONE, PAY3),
	TXGBE_PTT(0xDA, IP, IPV6, IG,   IPV6, NONE, PAY3),
	TXGBE_PTT(0xDB, IP, IPV6, IG,   IPV6, UDP,  PAY4),
	TXGBE_PTT(0xDC, IP, IPV6, IG,   IPV6, TCP,  PAY4),
	TXGBE_PTT(0xDD, IP, IPV6, IG,   IPV6, SCTP, PAY4),
	TXGBE_UKN(0xDE),
	TXGBE_UKN(0xDF),

	/* IPv6 --> GRE/NAT -> MAC -> NONE/IPv4/IPv6 */
	TXGBE_PTT(0xE0, IP, IPV6, IGM,  NONE, NONE, PAY3),
	TXGBE_PTT(0xE1, IP, IPV6, IGM,  FGV4, NONE, PAY3),
	TXGBE_PTT(0xE2, IP, IPV6, IGM,  IPV4, NONE, PAY3),
	TXGBE_PTT(0xE3, IP, IPV6, IGM,  IPV4, UDP,  PAY4),
	TXGBE_PTT(0xE4, IP, IPV6, IGM,  IPV4, TCP,  PAY4),
	TXGBE_PTT(0xE5, IP, IPV6, IGM,  IPV4, SCTP, PAY4),
	TXGBE_UKN(0xE6),
	TXGBE_UKN(0xE7),
	TXGBE_UKN(0xE8),
	TXGBE_PTT(0xE9, IP, IPV6, IGM,  FGV6, NONE, PAY3),
	TXGBE_PTT(0xEA, IP, IPV6, IGM,  IPV6, NONE, PAY3),
	TXGBE_PTT(0xEB, IP, IPV6, IGM,  IPV6, UDP,  PAY4),
	TXGBE_PTT(0xEC, IP, IPV6, IGM,  IPV6, TCP,  PAY4),
	TXGBE_PTT(0xED, IP, IPV6, IGM,  IPV6, SCTP, PAY4),
	TXGBE_UKN(0xEE),
	TXGBE_UKN(0xEF),

	/* IPv6 --> GRE/NAT -> MAC--> NONE/IPv */
	TXGBE_PTT(0xF0, IP, IPV6, IGMV, NONE, NONE, PAY3),
	TXGBE_PTT(0xF1, IP, IPV6, IGMV, FGV4, NONE, PAY3),
	TXGBE_PTT(0xF2, IP, IPV6, IGMV, IPV4, NONE, PAY3),
	TXGBE_PTT(0xF3, IP, IPV6, IGMV, IPV4, UDP,  PAY4),
	TXGBE_PTT(0xF4, IP, IPV6, IGMV, IPV4, TCP,  PAY4),
	TXGBE_PTT(0xF5, IP, IPV6, IGMV, IPV4, SCTP, PAY4),
	TXGBE_UKN(0xF6),
	TXGBE_UKN(0xF7),
	TXGBE_UKN(0xF8),
	TXGBE_PTT(0xF9, IP, IPV6, IGMV, FGV6, NONE, PAY3),
	TXGBE_PTT(0xFA, IP, IPV6, IGMV, IPV6, NONE, PAY3),
	TXGBE_PTT(0xFB, IP, IPV6, IGMV, IPV6, UDP,  PAY4),
	TXGBE_PTT(0xFC, IP, IPV6, IGMV, IPV6, TCP,  PAY4),
	TXGBE_PTT(0xFD, IP, IPV6, IGMV, IPV6, SCTP, PAY4),
	TXGBE_UKN(0xFE),
	TXGBE_UKN(0xFF),
};

static inline txgbe_dptype txgbe_decode_ptype(const u8 ptype)
{
	return txgbe_ptype_lookup[ptype];
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

txgbe_dptype txgbe_rx_decode_ptype(const union txgbe_rx_desc *rx_desc)
{
	return txgbe_decode_ptype(TXGBE_RXD_PKTTYPE(rx_desc));
}

#ifndef ETH_P_TEB
#define ETH_P_TEB       0x6558
#endif
txgbe_dptype txgbe_tx_encode_ptype(const struct txgbe_tx_buffer *first)
{
	struct sk_buff *skb = first->skb;
#ifdef HAVE_ENCAP_TSO_OFFLOAD
	u8 tun_prot = 0;
#endif
	u8 l4_prot = 0;
	u8 ptype = 0;

#ifdef HAVE_ENCAP_TSO_OFFLOAD
	if (skb->encapsulation) {
		union network_header hdr;

		switch (first->protocol) {
		case __constant_htons(ETH_P_IP):
			tun_prot = ip_hdr(skb)->protocol;
			if (ip_hdr(skb)->frag_off & htons(IP_MF | IP_OFFSET))
				goto encap_frag;
			ptype = TXGBE_PTYPE_TUN_IPV4;
			break;
		case __constant_htons(ETH_P_IPV6):
			tun_prot = get_ipv6_proto(skb, skb_network_offset(skb));
			if (tun_prot == NEXTHDR_FRAGMENT)
				goto encap_frag;
			ptype = TXGBE_PTYPE_TUN_IPV6;
			break;
		default:
			goto exit;
		}

		if (tun_prot == IPPROTO_IPIP) {
			hdr.raw = (void *)inner_ip_hdr(skb);
			ptype |= TXGBE_PTYPE_PKT_IPIP;
		} else if (tun_prot == IPPROTO_UDP) {
			hdr.raw = (void *)inner_ip_hdr(skb);
			/* fixme: VXLAN-GPE neither ETHER nor IP */
#ifdef ENCAP_TYPE_ETHER
			if (skb->inner_protocol_type != ENCAP_TYPE_ETHER ||
				skb->inner_protocol != htons(ETH_P_TEB)) {
				ptype |= TXGBE_PTYPE_PKT_IG;
			} else {
				if (((struct ethhdr *)
					skb_inner_mac_header(skb))->h_proto
					== htons(ETH_P_8021Q)) {
					ptype |= TXGBE_PTYPE_PKT_IGMV;
				} else {
					ptype |= TXGBE_PTYPE_PKT_IGM;
				}
			}
#endif
		} else if (tun_prot == IPPROTO_GRE) {
			hdr.raw = (void *)inner_ip_hdr(skb);
			if (skb->inner_protocol ==  htons(ETH_P_IP) ||
				skb->inner_protocol ==  htons(ETH_P_IPV6)) {
				ptype |= TXGBE_PTYPE_PKT_IG;
			} else {
				if (((struct ethhdr *)
					skb_inner_mac_header(skb))->h_proto
					== htons(ETH_P_8021Q)) {
					ptype |= TXGBE_PTYPE_PKT_IGMV;
				} else {
					ptype |= TXGBE_PTYPE_PKT_IGM;
				}
			}
		} else {
			goto exit;
		}

		switch (hdr.ipv4->version) {
		case IPVERSION:
			l4_prot = hdr.ipv4->protocol;
			if (hdr.ipv4->frag_off & htons(IP_MF | IP_OFFSET)) {
				ptype |= TXGBE_PTYPE_TYP_IPFRAG;
				goto exit;
			}
			break;
		case 6:
			l4_prot = get_ipv6_proto(skb,
						 skb_inner_network_offset(skb));
			ptype |= TXGBE_PTYPE_PKT_IPV6;
			if (l4_prot == NEXTHDR_FRAGMENT) {
				ptype |= TXGBE_PTYPE_TYP_IPFRAG;
				goto exit;
			}
			break;
		default:
			goto exit;
		}
	} else {
encap_frag:
#endif /* HAVE_ENCAP_TSO_OFFLOAD */
		switch (first->protocol) {
		case __constant_htons(ETH_P_IP):
			l4_prot = ip_hdr(skb)->protocol;
			ptype = TXGBE_PTYPE_PKT_IP;
			if (ip_hdr(skb)->frag_off & htons(IP_MF | IP_OFFSET)) {
				ptype |= TXGBE_PTYPE_TYP_IPFRAG;
				goto exit;
			}
			break;
#ifdef NETIF_F_IPV6_CSUM
		case __constant_htons(ETH_P_IPV6):
			l4_prot = get_ipv6_proto(skb, skb_network_offset(skb));
			ptype = TXGBE_PTYPE_PKT_IP | TXGBE_PTYPE_PKT_IPV6;
			if (l4_prot == NEXTHDR_FRAGMENT) {
				ptype |= TXGBE_PTYPE_TYP_IPFRAG;
				goto exit;
			}
			break;
#endif /* NETIF_F_IPV6_CSUM */
		case __constant_htons(ETH_P_1588):
			ptype = TXGBE_PTYPE_L2_TS;
			goto exit;
		case __constant_htons(ETH_P_FIP):
			ptype = TXGBE_PTYPE_L2_FIP;
			goto exit;
		case __constant_htons(0x88cc):
			ptype = TXGBE_PTYPE_L2_LLDP;
			goto exit;
		case __constant_htons(0x22e7):
			ptype = TXGBE_PTYPE_L2_CNM;
			goto exit;
		case __constant_htons(ETH_P_PAE):
			ptype = TXGBE_PTYPE_L2_EAPOL;
			goto exit;
		case __constant_htons(ETH_P_ARP):
			ptype = TXGBE_PTYPE_L2_ARP;
			goto exit;
		default:
			ptype = TXGBE_PTYPE_L2_MAC;
			goto exit;
		}
#ifdef HAVE_ENCAP_TSO_OFFLOAD
	}
#endif /* HAVE_ENCAP_TSO_OFFLOAD */

	switch (l4_prot) {
	case IPPROTO_TCP:
		ptype |= TXGBE_PTYPE_TYP_TCP;
		break;
	case IPPROTO_UDP:
		ptype |= TXGBE_PTYPE_TYP_UDP;
		break;
#ifdef HAVE_SCTP
	case IPPROTO_SCTP:
		ptype |= TXGBE_PTYPE_TYP_SCTP;
		break;
#endif /* HAVE_SCTP */
	default:
		ptype |= 0x2;
		break;
	}

exit:
	return txgbe_decode_ptype(ptype);
}




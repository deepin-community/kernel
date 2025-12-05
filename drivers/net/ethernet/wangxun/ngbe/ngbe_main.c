// SPDX-License-Identifier: GPL-2.0
/* WangXun Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */

#include <linux/ethtool.h>
#include <linux/highmem.h>
#include <linux/if_macvlan.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/pci.h>
#include <linux/pkt_sched.h>
#include <linux/string.h>
#include <linux/tcp.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <net/checksum.h>
#include <net/ip6_checksum.h>

#include "ngbe.h"
#include <linux/if_bridge.h>

#include <linux/atomic.h>
#include <linux/bpf.h>
#include <linux/bpf_trace.h>

#include "ngbe_xsk.h"
#include <net/xdp_sock_drv.h>

#include <linux/ktime.h>
#include <net/vxlan.h>

#include "ngbe_hw.h"
#include "ngbe_pcierr.h"
#include "ngbe_phy.h"
#include "ngbe_sriov.h"

char ngbe_driver_name[32] = NGBE_NAME;
static const char ngbe_driver_string[] = "WangXun Gigabit PCI Express Network Driver";
#define DRV_HW_PERF

#define FPGA

#define DRIVERIOV

#define BYPASS_TAG

#define RELEASE_TAG

#define DRV_VERSION __stringify(1.2.7deepin)
const char ngbe_driver_version[32] = DRV_VERSION;
static const char ngbe_copyright[] = "Copyright (c) 2018 -2019 Beijing WangXun Technology Co., Ltd";
static const char ngbe_overheat_msg[] =
	"adapter stopped because over heated. ";
static const char ngbe_underheat_msg[] =
	"adapter restart because not over heated";

/* ngbe_pci_tbl - PCI Device ID Table
 * Wildcard entries (PCI_ANY_ID) should come last
 * Last entry must be all 0s
 * { Vendor ID, Device ID, SubVendor ID, SubDevice ID,
 *   Class, Class Mask, private data (not used) }
 */
static const struct pci_device_id ngbe_pci_tbl[] = {
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_TEST), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860A2), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860A2S), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860A4), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860A4S), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860AL2), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860AL2S), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860AL4), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860AL4S), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860AL_W), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860NCSI), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860A1L), 0},
	{PCI_VDEVICE(TRUSTNETIC, NGBE_DEV_ID_EM_WX1860A1), 0},
	{PCI_VDEVICE(TRUSTNETIC, 0x10c), 0},
	/* required last entry */
	{.device = 0}};
MODULE_DEVICE_TABLE(pci, ngbe_pci_tbl);

MODULE_AUTHOR("Beijing WangXun Technology Co., Ltd, <linux.nic@trustnetic.com>");
MODULE_DESCRIPTION("WangXun(R) Gigabit PCI Express Network Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

DEFINE_STATIC_KEY_FALSE(ngbe_xdp_locking_key);
EXPORT_SYMBOL(ngbe_xdp_locking_key);
#define DEFAULT_DEBUG_LEVEL_SHIFT 3

static struct workqueue_struct *ngbe_wq;

static bool ngbe_check_cfg_remove(struct ngbe_hw *hw, struct pci_dev *pdev);

/* The ngbe_ptype_lookup is used to convert from the 8-bit ptype in the
 * hardware to a bit-field that can be used by SW to more easily determine the
 * packet type.
 * Macros are used to shorten the table lines and make this table human
 * readable.
 * We store the PTYPE in the top byte of the bit field - this is just so that
 * we can check that the table doesn't have a row missing, as the index into
 * the table should be the PTYPE.
 * Typical work flow:
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
#define NGBE_PTT(ptype, mac, ip, etype, eip, proto, layer) \
	{ptype,                                            \
	 1,                                                \
	 /* mac     */ NGBE_DEC_PTYPE_MAC_##mac,           \
	 /* ip      */ NGBE_DEC_PTYPE_IP_##ip,             \
	 /* etype   */ NGBE_DEC_PTYPE_ETYPE_##etype,       \
	 /* eip     */ NGBE_DEC_PTYPE_IP_##eip,            \
	 /* proto   */ NGBE_DEC_PTYPE_PROT_##proto,        \
	 /* layer   */ NGBE_DEC_PTYPE_LAYER_##layer}

#define NGBE_UKN(ptype) {ptype, 0, 0, 0, 0, 0, 0, 0}

/* Lookup table mapping the HW PTYPE to the bit field for decoding */
/* for ((pt=0;pt<256;pt++)); do printf "macro(0x%02X),\n" $pt; done */
struct ngbe_dec_ptype ngbe_ptype_lookup[256] = {
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
	NGBE_PTT(0x12, L2, NONE, NONE, NONE, TS, PAY2),
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
	NGBE_PTT(0x23, IP, IPV4, NONE, NONE, UDP, PAY4),
	NGBE_PTT(0x24, IP, IPV4, NONE, NONE, TCP, PAY4),
	NGBE_PTT(0x25, IP, IPV4, NONE, NONE, SCTP, PAY4),
	NGBE_UKN(0x26),
	NGBE_UKN(0x27),
	NGBE_UKN(0x28),
	NGBE_PTT(0x29, IP, FGV6, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x2A, IP, IPV6, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x2B, IP, IPV6, NONE, NONE, UDP, PAY3),
	NGBE_PTT(0x2C, IP, IPV6, NONE, NONE, TCP, PAY4),
	NGBE_PTT(0x2D, IP, IPV6, NONE, NONE, SCTP, PAY4),
	NGBE_UKN(0x2E),
	NGBE_UKN(0x2F),

	/* L2: fcoe */
	NGBE_PTT(0x30, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x31, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x32, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x33, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x34, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_UKN(0x35),
	NGBE_UKN(0x36),
	NGBE_UKN(0x37),
	NGBE_PTT(0x38, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x39, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x3A, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x3B, FCOE, NONE, NONE, NONE, NONE, PAY3),
	NGBE_PTT(0x3C, FCOE, NONE, NONE, NONE, NONE, PAY3),
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
	NGBE_PTT(0x83, IP, IPV4, IPIP, IPV4, UDP, PAY4),
	NGBE_PTT(0x84, IP, IPV4, IPIP, IPV4, TCP, PAY4),
	NGBE_PTT(0x85, IP, IPV4, IPIP, IPV4, SCTP, PAY4),
	NGBE_UKN(0x86),
	NGBE_UKN(0x87),
	NGBE_UKN(0x88),
	NGBE_PTT(0x89, IP, IPV4, IPIP, FGV6, NONE, PAY3),
	NGBE_PTT(0x8A, IP, IPV4, IPIP, IPV6, NONE, PAY3),
	NGBE_PTT(0x8B, IP, IPV4, IPIP, IPV6, UDP, PAY4),
	NGBE_PTT(0x8C, IP, IPV4, IPIP, IPV6, TCP, PAY4),
	NGBE_PTT(0x8D, IP, IPV4, IPIP, IPV6, SCTP, PAY4),
	NGBE_UKN(0x8E),
	NGBE_UKN(0x8F),

	/* IPv4 --> GRE/NAT --> NONE/IPv4/IPv6 */
	NGBE_PTT(0x90, IP, IPV4, IG, NONE, NONE, PAY3),
	NGBE_PTT(0x91, IP, IPV4, IG, FGV4, NONE, PAY3),
	NGBE_PTT(0x92, IP, IPV4, IG, IPV4, NONE, PAY3),
	NGBE_PTT(0x93, IP, IPV4, IG, IPV4, UDP, PAY4),
	NGBE_PTT(0x94, IP, IPV4, IG, IPV4, TCP, PAY4),
	NGBE_PTT(0x95, IP, IPV4, IG, IPV4, SCTP, PAY4),
	NGBE_UKN(0x96),
	NGBE_UKN(0x97),
	NGBE_UKN(0x98),
	NGBE_PTT(0x99, IP, IPV4, IG, FGV6, NONE, PAY3),
	NGBE_PTT(0x9A, IP, IPV4, IG, IPV6, NONE, PAY3),
	NGBE_PTT(0x9B, IP, IPV4, IG, IPV6, UDP, PAY4),
	NGBE_PTT(0x9C, IP, IPV4, IG, IPV6, TCP, PAY4),
	NGBE_PTT(0x9D, IP, IPV4, IG, IPV6, SCTP, PAY4),
	NGBE_UKN(0x9E),
	NGBE_UKN(0x9F),

	/* IPv4 --> GRE/NAT --> MAC --> NONE/IPv4/IPv6 */
	NGBE_PTT(0xA0, IP, IPV4, IGM, NONE, NONE, PAY3),
	NGBE_PTT(0xA1, IP, IPV4, IGM, FGV4, NONE, PAY3),
	NGBE_PTT(0xA2, IP, IPV4, IGM, IPV4, NONE, PAY3),
	NGBE_PTT(0xA3, IP, IPV4, IGM, IPV4, UDP, PAY4),
	NGBE_PTT(0xA4, IP, IPV4, IGM, IPV4, TCP, PAY4),
	NGBE_PTT(0xA5, IP, IPV4, IGM, IPV4, SCTP, PAY4),
	NGBE_UKN(0xA6),
	NGBE_UKN(0xA7),
	NGBE_UKN(0xA8),
	NGBE_PTT(0xA9, IP, IPV4, IGM, FGV6, NONE, PAY3),
	NGBE_PTT(0xAA, IP, IPV4, IGM, IPV6, NONE, PAY3),
	NGBE_PTT(0xAB, IP, IPV4, IGM, IPV6, UDP, PAY4),
	NGBE_PTT(0xAC, IP, IPV4, IGM, IPV6, TCP, PAY4),
	NGBE_PTT(0xAD, IP, IPV4, IGM, IPV6, SCTP, PAY4),
	NGBE_UKN(0xAE),
	NGBE_UKN(0xAF),

	/* IPv4 --> GRE/NAT --> MAC+VLAN --> NONE/IPv4/IPv6 */
	NGBE_PTT(0xB0, IP, IPV4, IGMV, NONE, NONE, PAY3),
	NGBE_PTT(0xB1, IP, IPV4, IGMV, FGV4, NONE, PAY3),
	NGBE_PTT(0xB2, IP, IPV4, IGMV, IPV4, NONE, PAY3),
	NGBE_PTT(0xB3, IP, IPV4, IGMV, IPV4, UDP, PAY4),
	NGBE_PTT(0xB4, IP, IPV4, IGMV, IPV4, TCP, PAY4),
	NGBE_PTT(0xB5, IP, IPV4, IGMV, IPV4, SCTP, PAY4),
	NGBE_UKN(0xB6),
	NGBE_UKN(0xB7),
	NGBE_UKN(0xB8),
	NGBE_PTT(0xB9, IP, IPV4, IGMV, FGV6, NONE, PAY3),
	NGBE_PTT(0xBA, IP, IPV4, IGMV, IPV6, NONE, PAY3),
	NGBE_PTT(0xBB, IP, IPV4, IGMV, IPV6, UDP, PAY4),
	NGBE_PTT(0xBC, IP, IPV4, IGMV, IPV6, TCP, PAY4),
	NGBE_PTT(0xBD, IP, IPV4, IGMV, IPV6, SCTP, PAY4),
	NGBE_UKN(0xBE),
	NGBE_UKN(0xBF),

	/* IPv6 --> IPv4/IPv6 */
	NGBE_UKN(0xC0),
	NGBE_PTT(0xC1, IP, IPV6, IPIP, FGV4, NONE, PAY3),
	NGBE_PTT(0xC2, IP, IPV6, IPIP, IPV4, NONE, PAY3),
	NGBE_PTT(0xC3, IP, IPV6, IPIP, IPV4, UDP, PAY4),
	NGBE_PTT(0xC4, IP, IPV6, IPIP, IPV4, TCP, PAY4),
	NGBE_PTT(0xC5, IP, IPV6, IPIP, IPV4, SCTP, PAY4),
	NGBE_UKN(0xC6),
	NGBE_UKN(0xC7),
	NGBE_UKN(0xC8),
	NGBE_PTT(0xC9, IP, IPV6, IPIP, FGV6, NONE, PAY3),
	NGBE_PTT(0xCA, IP, IPV6, IPIP, IPV6, NONE, PAY3),
	NGBE_PTT(0xCB, IP, IPV6, IPIP, IPV6, UDP, PAY4),
	NGBE_PTT(0xCC, IP, IPV6, IPIP, IPV6, TCP, PAY4),
	NGBE_PTT(0xCD, IP, IPV6, IPIP, IPV6, SCTP, PAY4),
	NGBE_UKN(0xCE),
	NGBE_UKN(0xCF),

	/* IPv6 --> GRE/NAT -> NONE/IPv4/IPv6 */
	NGBE_PTT(0xD0, IP, IPV6, IG, NONE, NONE, PAY3),
	NGBE_PTT(0xD1, IP, IPV6, IG, FGV4, NONE, PAY3),
	NGBE_PTT(0xD2, IP, IPV6, IG, IPV4, NONE, PAY3),
	NGBE_PTT(0xD3, IP, IPV6, IG, IPV4, UDP, PAY4),
	NGBE_PTT(0xD4, IP, IPV6, IG, IPV4, TCP, PAY4),
	NGBE_PTT(0xD5, IP, IPV6, IG, IPV4, SCTP, PAY4),
	NGBE_UKN(0xD6),
	NGBE_UKN(0xD7),
	NGBE_UKN(0xD8),
	NGBE_PTT(0xD9, IP, IPV6, IG, FGV6, NONE, PAY3),
	NGBE_PTT(0xDA, IP, IPV6, IG, IPV6, NONE, PAY3),
	NGBE_PTT(0xDB, IP, IPV6, IG, IPV6, UDP, PAY4),
	NGBE_PTT(0xDC, IP, IPV6, IG, IPV6, TCP, PAY4),
	NGBE_PTT(0xDD, IP, IPV6, IG, IPV6, SCTP, PAY4),
	NGBE_UKN(0xDE),
	NGBE_UKN(0xDF),

	/* IPv6 --> GRE/NAT -> MAC -> NONE/IPv4/IPv6 */
	NGBE_PTT(0xE0, IP, IPV6, IGM, NONE, NONE, PAY3),
	NGBE_PTT(0xE1, IP, IPV6, IGM, FGV4, NONE, PAY3),
	NGBE_PTT(0xE2, IP, IPV6, IGM, IPV4, NONE, PAY3),
	NGBE_PTT(0xE3, IP, IPV6, IGM, IPV4, UDP, PAY4),
	NGBE_PTT(0xE4, IP, IPV6, IGM, IPV4, TCP, PAY4),
	NGBE_PTT(0xE5, IP, IPV6, IGM, IPV4, SCTP, PAY4),
	NGBE_UKN(0xE6),
	NGBE_UKN(0xE7),
	NGBE_UKN(0xE8),
	NGBE_PTT(0xE9, IP, IPV6, IGM, FGV6, NONE, PAY3),
	NGBE_PTT(0xEA, IP, IPV6, IGM, IPV6, NONE, PAY3),
	NGBE_PTT(0xEB, IP, IPV6, IGM, IPV6, UDP, PAY4),
	NGBE_PTT(0xEC, IP, IPV6, IGM, IPV6, TCP, PAY4),
	NGBE_PTT(0xED, IP, IPV6, IGM, IPV6, SCTP, PAY4),
	NGBE_UKN(0xEE),
	NGBE_UKN(0xEF),

	/* IPv6 --> GRE/NAT -> MAC--> NONE/IPv */
	NGBE_PTT(0xF0, IP, IPV6, IGMV, NONE, NONE, PAY3),
	NGBE_PTT(0xF1, IP, IPV6, IGMV, FGV4, NONE, PAY3),
	NGBE_PTT(0xF2, IP, IPV6, IGMV, IPV4, NONE, PAY3),
	NGBE_PTT(0xF3, IP, IPV6, IGMV, IPV4, UDP, PAY4),
	NGBE_PTT(0xF4, IP, IPV6, IGMV, IPV4, TCP, PAY4),
	NGBE_PTT(0xF5, IP, IPV6, IGMV, IPV4, SCTP, PAY4),
	NGBE_UKN(0xF6),
	NGBE_UKN(0xF7),
	NGBE_UKN(0xF8),
	NGBE_PTT(0xF9, IP, IPV6, IGMV, FGV6, NONE, PAY3),
	NGBE_PTT(0xFA, IP, IPV6, IGMV, IPV6, NONE, PAY3),
	NGBE_PTT(0xFB, IP, IPV6, IGMV, IPV6, UDP, PAY4),
	NGBE_PTT(0xFC, IP, IPV6, IGMV, IPV6, TCP, PAY4),
	NGBE_PTT(0xFD, IP, IPV6, IGMV, IPV6, SCTP, PAY4),
	NGBE_UKN(0xFE),
	NGBE_UKN(0xFF),
};

static inline struct ngbe_dec_ptype ngbe_decode_ptype(const u8 ptype)
{
	return ngbe_ptype_lookup[ptype];
}

static inline struct ngbe_dec_ptype decode_rx_desc_ptype(const union ngbe_rx_desc *rx_desc)
{
	return ngbe_decode_ptype(NGBE_RXD_PKTTYPE(rx_desc));
}

void ngbe_print_tx_hang_status(struct ngbe_adapter *adapter)
{
	int pos;
	u32 value;
	struct pci_dev *pdev = adapter->pdev;
	u16 devctl2;

	pos = pci_find_ext_capability(pdev, PCI_EXT_CAP_ID_ERR);
	if (!pos)
		return;
	pci_read_config_dword(pdev, pos + PCI_ERR_UNCOR_STATUS, &value);
	e_info(probe, "AER Uncorrectable Error Status: 0x%08x\n", value);
	ngbe_aer_print_error(adapter, NGBE_AER_UNCORRECTABLE, value);
	pci_read_config_dword(pdev, pos + PCI_ERR_UNCOR_MASK, &value);
	e_info(probe, "AER Uncorrectable Error Mask: 0x%08x\n", value);
	pci_read_config_dword(pdev, pos + PCI_ERR_UNCOR_SEVER, &value);
	e_info(probe, "AER Uncorrectable Error Severity: 0x%08x\n", value);
	pci_read_config_dword(pdev, pos + PCI_ERR_COR_STATUS, &value);
	e_info(probe, "AER Correctable Error Status: 0x%08x\n", value);
	ngbe_aer_print_error(adapter, NGBE_AER_CORRECTABLE, value);
	pci_read_config_dword(pdev, pos + PCI_ERR_COR_MASK, &value);
	e_info(probe, "AER Correctable Error Mask: 0x%08x\n", value);
	pci_read_config_dword(pdev, pos + PCI_ERR_CAP, &value);
	e_info(probe, "AER Capabilities and Control Register: 0x%08x\n", value);

	pcie_capability_read_word(pdev, PCI_EXP_DEVCTL2, &devctl2);
	e_info(probe, "Device Control2 Register: 0x%04x\n", devctl2);

	e_info(probe, "Tx flow control Status[TDB_TFCS 0xCE00]: 0x%x\n",
	       rd32(&adapter->hw, NGBE_TDB_TFCS));

	e_info(probe, "Tx Desc Fatal Error[TDM_DESC_FATAL 0x80D0]: 0x%x\n",
	       rd32(&adapter->hw, NGBE_TDM_DESC_FATAL));
}

static void ngbe_dump_all_ring_desc(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	union ngbe_tx_desc *tx_desc;
	struct ngbe_ring *tx_ring;
	int i, j;

	if (!netif_msg_tx_err(adapter))
		return;

	e_warn(tx_err, "Dump desc base addr\n");

	for (i = 0; i < adapter->num_tx_queues; i++) {
		e_warn(tx_err, "q_%d:0x%x%x\n", i, rd32(hw, NGBE_PX_TR_BAH(i)),
		       rd32(hw, NGBE_PX_TR_BAL(i)));
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		tx_ring = adapter->tx_ring[i];
		for (j = 0; j < tx_ring->count; j++) {
			tx_desc = NGBE_TX_DESC(tx_ring, j);
			if (tx_desc->read.olinfo_status != 0x1)
				e_warn(tx_err, "queue[%d][%d]:0x%llx, 0x%x, 0x%x\n", i, j,
				       tx_desc->read.buffer_addr, tx_desc->read.cmd_type_len,
				       tx_desc->read.olinfo_status);
		}
	}
}

static void ngbe_check_minimum_link(struct ngbe_adapter *adapter, int expected_gts)
{
	struct ngbe_hw *hw = &adapter->hw;
	struct pci_dev *pdev;

	/* Some devices are not connected over PCIe and thus do not negotiate
	 * speed. These devices do not have valid bus info, and thus any report
	 * we generate may not be correct.
	 */
	if (hw->bus.type == ngbe_bus_type_internal)
		return;

	pdev = adapter->pdev;

	pcie_print_link_status(pdev);
}

/**
 * ngbe_enumerate_functions - Get the number of ports this device has
 * @adapter: adapter structure
 * This function enumerates the phsyical functions co-located on a single slot,
 * in order to determine how many ports a device has. This is most useful in
 * determining the required GT/s of PCIe bandwidth necessary for optimal
 * performance.
 **/
static inline int ngbe_enumerate_functions(struct ngbe_adapter *adapter)
{
	struct pci_dev *entry, *pdev = adapter->pdev;
	int physfns = 0;

	list_for_each_entry(entry, &pdev->bus->devices, bus_list) {
#ifdef CONFIG_PCI_IOV
		/* don't count virtual functions */
		if (entry->is_virtfn)
			continue;
#endif

		/* When the devices on the bus don't all match our device ID,
		 * we can't reliably determine the correct number of
		 * functions. This can occur if a function has been direct
		 * attached to a virtual machine using VT-d, for example. In
		 * this case, simply return -1 to indicate this.
		 */
		if (entry->vendor != pdev->vendor || entry->device != pdev->device)
			return -1;

		physfns++;
	}

	return physfns;
}

static void ngbe_service_event_schedule(struct ngbe_adapter *adapter)
{
	if (!test_bit(__NGBE_DOWN, &adapter->state) &&
	    !test_bit(__NGBE_REMOVING, &adapter->state) &&
	    !test_and_set_bit(__NGBE_SERVICE_SCHED, &adapter->state))
		queue_work(ngbe_wq, &adapter->service_task);
}

static void ngbe_service_event_complete(struct ngbe_adapter *adapter)
{
	WARN_ON_ONCE(!test_bit(__NGBE_SERVICE_SCHED, &adapter->state));

	/* flush memory to make sure state is correct before next watchdog */
	smp_mb__before_atomic();
	clear_bit(__NGBE_SERVICE_SCHED, &adapter->state);
}

static void ngbe_remove_adapter(struct ngbe_hw *hw)
{
	struct ngbe_adapter *adapter = hw->back;

	if (!hw->hw_addr)
		return;
	hw->hw_addr = NULL;
	e_dev_err("Adapter removed\n");
	if (test_bit(__NGBE_SERVICE_INITED, &adapter->state))
		ngbe_service_event_schedule(adapter);
}

static void ngbe_check_remove(struct ngbe_hw *hw, u32 reg)
{
	u32 value;

	/* The following check not only optimizes a bit by not
	 * performing a read on the status register when the
	 * register just read was a status register read that
	 * returned NGBE_FAILED_READ_REG. It also blocks any
	 * potential recursion.
	 */
	if (reg == NGBE_CFG_PORT_ST) {
		ngbe_remove_adapter(hw);
		return;
	}
	value = rd32(hw, NGBE_CFG_PORT_ST);
	if (value == NGBE_FAILED_READ_REG)
		ngbe_remove_adapter(hw);
}

static u32 ngbe_validate_register_read(struct ngbe_hw *hw, u32 reg, bool quiet)
{
	int i;
	u32 value;
	u8 __iomem *reg_addr;
	struct ngbe_adapter *adapter = hw->back;

	reg_addr = READ_ONCE(hw->hw_addr);
	if (NGBE_REMOVED(reg_addr))
		return NGBE_FAILED_READ_REG;
	for (i = 0; i < NGBE_DEAD_READ_RETRIES; ++i) {
		value = ngbe_rd32(reg_addr + reg);
		if (value != NGBE_DEAD_READ_REG)
			break;
	}
	if (quiet)
		return value;
	if (value == NGBE_DEAD_READ_REG)
		e_err(drv, "%s: register %x read unchanged\n", __func__, reg);
	else
		e_warn(hw, "%s: register %x read recovered after %d retries\n", __func__, reg,
		       i + 1);
	return value;
}

/**
 * ngbe_read_reg - Read from device register
 * @hw: hw specific details
 * @reg: offset of register to read
 * Returns : value read or NGBE_FAILED_READ_REG if removed
 * This function is used to read device registers. It checks for device
 * removal by confirming any read that returns all ones by checking the
 * status register value for all ones. This function avoids reading from
 * the hardware if a removal was previously detected in which case it
 * returns NGBE_FAILED_READ_REG (all ones).
 */
u32 ngbe_read_reg(struct ngbe_hw *hw, u32 reg, bool quiet)
{
	u32 value;
	u8 __iomem *reg_addr;

	reg_addr = READ_ONCE(hw->hw_addr);
	if (NGBE_REMOVED(reg_addr))
		return NGBE_FAILED_READ_REG;
	value = ngbe_rd32(reg_addr + reg);
	if (unlikely(value == NGBE_FAILED_READ_REG))
		ngbe_check_remove(hw, reg);
	if (unlikely(value == NGBE_DEAD_READ_REG))
		value = ngbe_validate_register_read(hw, reg, quiet);
	return value;
}

void ngbe_release_hw_control(struct ngbe_adapter *adapter)
{
	/* Let firmware take over control of h/w */
	wr32m(&adapter->hw, NGBE_CFG_PORT_CTL, NGBE_CFG_PORT_CTL_DRV_LOAD, 0);

	wr32(&adapter->hw, NGBE_TSC_LSEC_PKTNUM0, 0);
}

void ngbe_get_hw_control(struct ngbe_adapter *adapter)
{
	/* Let firmware know the driver has taken over */
	wr32m(&adapter->hw, NGBE_CFG_PORT_CTL, NGBE_CFG_PORT_CTL_DRV_LOAD,
	      NGBE_CFG_PORT_CTL_DRV_LOAD);
}

/**
 * ngbe_set_ivar - set the IVAR registers, mapping interrupt causes to vectors
 * @adapter: pointer to adapter struct
 * @direction: 0 for Rx, 1 for Tx, -1 for other causes
 * @queue: queue to map the corresponding interrupt to
 * @msix_vector: the vector to map to the corresponding queue
 **/
static void ngbe_set_ivar(struct ngbe_adapter *adapter, s8 direction, u16 queue, u16 msix_vector)
{
	u32 ivar, index;
	struct ngbe_hw *hw = &adapter->hw;

	if (direction == -1) {
		/* other causes */
		msix_vector |= NGBE_PX_IVAR_ALLOC_VAL;
		index = 0;
		ivar = rd32(&adapter->hw, NGBE_PX_MISC_IVAR);
		ivar &= ~(0xFF << index);
		ivar |= (msix_vector << index);
		/* if assigned VFs >= 7, the pf misc irq shall be remapped to 0x88. */
		if (adapter->flags2 & NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP)
			ivar = msix_vector;
		wr32(&adapter->hw, NGBE_PX_MISC_IVAR, ivar);
	} else {
		/* tx or rx causes */
		msix_vector |= NGBE_PX_IVAR_ALLOC_VAL;
		index = ((16 * (queue & 1)) + (8 * direction));
		ivar = rd32(hw, NGBE_PX_IVAR(queue >> 1));
		ivar &= ~(0xFF << index);
		ivar |= (msix_vector << index);
		wr32(hw, NGBE_PX_IVAR(queue >> 1), ivar);
	}
}

void ngbe_unmap_and_free_tx_resource(struct ngbe_ring *ring, struct ngbe_tx_buffer *tx_buffer)
{
	if (!ring_is_xdp(ring) && tx_buffer->skb) {
		dev_kfree_skb_any(tx_buffer->skb);
		if (dma_unmap_len(tx_buffer, len))
			dma_unmap_single(ring->dev, dma_unmap_addr(tx_buffer, dma),
					 dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
	} else if (dma_unmap_len(tx_buffer, len)) {
		dma_unmap_page(ring->dev, dma_unmap_addr(tx_buffer, dma),
			       dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
	}
	tx_buffer->next_to_watch = NULL;
	if (ring_is_xdp(ring))
		tx_buffer->xdpf = NULL;
	else
		tx_buffer->skb = NULL;
	dma_unmap_len_set(tx_buffer, len, 0);
	tx_buffer->va = NULL;
	/* tx_buffer must be completely set up in the transmit path */
}

static void ngbe_update_xoff_rx_lfc(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	struct ngbe_hw_stats *hwstats = &adapter->stats;
	int i;
	u32 data;

	if (hw->fc.current_mode != ngbe_fc_full && hw->fc.current_mode != ngbe_fc_rx_pause)
		return;

	data = rd32(hw, NGBE_MAC_LXOFFRXC);

	hwstats->lxoffrxc += data;

	/* refill credits (no tx hang) if we received xoff */
	if (!data)
		return;

	for (i = 0; i < adapter->num_tx_queues; i++)
		clear_bit(__NGBE_HANG_CHECK_ARMED, &adapter->tx_ring[i]->state);
	for (i = 0; i < adapter->num_xdp_queues; i++)
		clear_bit(__NGBE_HANG_CHECK_ARMED, &adapter->xdp_ring[i]->state);
}

static u64 ngbe_get_tx_completed(struct ngbe_ring *ring) { return ring->stats.packets; }

static u64 ngbe_get_tx_pending(struct ngbe_ring *ring)
{
	struct ngbe_adapter *adapter;
	struct ngbe_hw *hw;
	u32 head, tail;

	if (ring->accel)
		adapter = ring->accel->adapter;
	else
		adapter = ring->q_vector->adapter;

	hw = &adapter->hw;
	head = rd32(hw, NGBE_PX_TR_RP(ring->reg_idx));
	tail = rd32(hw, NGBE_PX_TR_WP(ring->reg_idx));

	return ((head <= tail) ? tail : tail + ring->count) - head;
}

static inline bool ngbe_check_tx_hang(struct ngbe_ring *tx_ring)
{
	u64 tx_done = ngbe_get_tx_completed(tx_ring);
	u64 tx_done_old = tx_ring->tx_stats.tx_done_old;
	u64 tx_pending = ngbe_get_tx_pending(tx_ring);

	clear_check_for_tx_hang(tx_ring);

	/* Check for a hung queue, but be thorough. This verifies
	 * that a transmit has been completed since the previous
	 * check AND there is at least one packet pending. The
	 * ARMED bit is set to indicate a potential hang. The
	 * bit is cleared if a pause frame is received to remove
	 * false hang detection due to PFC or 802.3x frames. By
	 * requiring this to fail twice we avoid races with
	 * pfc clearing the ARMED bit and conditions where we
	 * run the check_tx_hang logic with a transmit completion
	 * pending but without time to complete it yet.
	 */
	if (tx_done_old == tx_done && tx_pending) {
		/* make sure it is true for two checks in a row */
		return test_and_set_bit(__NGBE_HANG_CHECK_ARMED, &tx_ring->state);
	}
	/* update completed stats and continue */
	tx_ring->tx_stats.tx_done_old = tx_done;
	/* reset the countdown */
	clear_bit(__NGBE_HANG_CHECK_ARMED, &tx_ring->state);

	return false;
}

static void ngbe_tx_timeout_dorecovery(struct ngbe_adapter *adapter)
{
	/* schedule immediate reset if we believe we hung */
	if (adapter->hw.bus.lan_id == 0)
		adapter->flags2 |= NGBE_FLAG2_PCIE_NEED_RECOVER;
	else
		wr32(&adapter->hw, NGBE_MIS_PF_SM, 1);
	ngbe_service_event_schedule(adapter);
}

/**
 * ngbe_tx_timeout_reset - initiate reset due to Tx timeout
 * @adapter: driver private struct
 **/
static void ngbe_tx_timeout_reset(struct ngbe_adapter *adapter)
{
	if (!test_bit(__NGBE_DOWN, &adapter->state)) {
		adapter->flags2 |= NGBE_FLAG2_PF_RESET_REQUESTED;
		e_warn(drv, "initiating reset due to tx timeout\n");
		ngbe_service_event_schedule(adapter);
	}
}

/**
 * ngbe_tx_timeout - Respond to a Tx Hang
 * @netdev: network interface device structure
 **/
static void ngbe_tx_timeout(struct net_device *netdev, unsigned int txqueue)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	bool tdm_desc_fatal = false;
	u16 vid, pci_cmd;
	u32 head, tail;
	u32 value = 0;
	int i;

	pci_read_config_word(adapter->pdev, PCI_VENDOR_ID, &vid);
	ERROR_REPORT1(NGBE_ERROR_POLLING, "pci vendor id is 0x%x\n", vid);

	pci_read_config_word(adapter->pdev, PCI_COMMAND, &pci_cmd);
	ERROR_REPORT1(NGBE_ERROR_POLLING, "pci command reg is 0x%x.\n", pci_cmd);

	value = rd32(&adapter->hw, 0x10000);
	ERROR_REPORT1(NGBE_ERROR_POLLING, "reg 0x10000 value is 0x%08x\n", value);
	value = rd32(&adapter->hw, 0x180d0);
	ERROR_REPORT1(NGBE_ERROR_POLLING, "reg 0x180d0 value is 0x%08x\n", value);
	value = rd32(&adapter->hw, 0x180d4);
	ERROR_REPORT1(NGBE_ERROR_POLLING, "reg 0x180d4 value is 0x%08x\n", value);
	value = rd32(&adapter->hw, 0x180d8);
	ERROR_REPORT1(NGBE_ERROR_POLLING, "reg 0x180d8 value is 0x%08x\n", value);
	value = rd32(&adapter->hw, 0x180dc);
	ERROR_REPORT1(NGBE_ERROR_POLLING, "reg 0x180dc value is 0x%08x\n", value);

	for (i = 0; i < adapter->num_tx_queues; i++) {
		head = rd32(&adapter->hw, NGBE_PX_TR_RP(adapter->tx_ring[i]->reg_idx));
		tail = rd32(&adapter->hw, NGBE_PX_TR_WP(adapter->tx_ring[i]->reg_idx));

		ERROR_REPORT1(NGBE_ERROR_POLLING,
			      "tx ring %d next_to_use is %d, next_to_clean is %d\n", i,
			      adapter->tx_ring[i]->next_to_use, adapter->tx_ring[i]->next_to_clean);
		ERROR_REPORT1(NGBE_ERROR_POLLING, "tx ring %d hw rp is 0x%x, wp is 0x%x\n", i, head,
			      tail);
	}

	value = rd32(&adapter->hw, NGBE_PX_IMS);
	ERROR_REPORT1(NGBE_ERROR_POLLING, "PX_IMS value is 0x%08x\n", value);

	if (value && value != NGBE_FAILED_READ_CFG_DWORD) {
		ERROR_REPORT1(NGBE_ERROR_POLLING, "clear interrupt mask.\n");
		wr32(&adapter->hw, NGBE_PX_ICS, value);
		wr32(&adapter->hw, NGBE_PX_IMC, value);
	}

	/* only check pf queue tdm desc error : [0,7] is valid */
	if (rd32(&adapter->hw, NGBE_TDM_DESC_FATAL) & GENMASK(7, 0))
		tdm_desc_fatal = true;

	if (NGBE_RECOVER_CHECK == 1) {
		if (vid == NGBE_FAILED_READ_CFG_WORD || tdm_desc_fatal || !(pci_cmd & BIT(1))) {
			ngbe_tx_timeout_dorecovery(adapter);
		} else {
			ngbe_print_tx_hang_status(adapter);
			ngbe_tx_timeout_reset(adapter);
		}
	} else {
		ngbe_tx_timeout_dorecovery(adapter);
	}
}

static inline u16 ngbe_desc_buf_unmapped(struct ngbe_ring *ring, u16 ntc, u16 ntf)
{
	return ((ntc >= ntf) ? 0 : ring->count) + ntc - ntf;
}

/**
 * ngbe_clean_tx_irq - Reclaim resources after transmit completes
 * @q_vector: structure containing interrupt and ring information
 * @tx_ring: tx ring to clean
 **/
static bool ngbe_clean_tx_irq(struct ngbe_q_vector *q_vector, struct ngbe_ring *tx_ring)
{
	struct ngbe_adapter *adapter = q_vector->adapter;
	struct ngbe_tx_buffer *tx_buffer;
	union ngbe_tx_desc *tx_desc;
	unsigned int total_bytes = 0, total_packets = 0;
	unsigned int budget = q_vector->tx.work_limit;
	unsigned int i = tx_ring->next_to_clean;
	struct ngbe_hw *hw = &adapter->hw;
	u16 vid = 0;
	int j = 0;
	u32 size;
	unsigned int ntf;
	struct ngbe_tx_buffer *free_tx_buffer;
	s32 unmapped_descs = 0;
	bool first_dma;
	ktime_t now;
	u64 ms;

	if (test_bit(__NGBE_DOWN, &adapter->state))
		return true;

	now = ktime_get();
	ms = ktime_to_ms(now);

	tx_buffer = &tx_ring->tx_buffer_info[i];
	tx_desc = NGBE_TX_DESC(tx_ring, i);
	i -= tx_ring->count;
	do {
		union ngbe_tx_desc *eop_desc = tx_buffer->next_to_watch;

		/* if next_to_watch is not set then there is no work pending */
		if (!eop_desc)
			break;

		/* prevent any other reads prior to eop_desc */
		smp_rmb();

		/* if DD is not set pending work has not been completed */
		if (!(eop_desc->wb.status & cpu_to_le32(NGBE_TXD_STAT_DD)))
			break;

		/* clear next_to_watch to prevent false hangs */
		tx_buffer->next_to_watch = NULL;
		tx_buffer->done_time = ms;

		/* update the statistics for this packet */
		total_bytes += tx_buffer->bytecount;
		total_packets += tx_buffer->gso_segs;

		if (tx_buffer->skb) {
			if (!ring_is_xdp(tx_ring) &&
			    !(skb_shinfo(tx_buffer->skb)->tx_flags & SKBTX_IN_PROGRESS))
				skb_orphan(tx_buffer->skb);
		} else {
			dev_err(tx_ring->dev, "skb is NULL.\n");
		}

		/* unmap remaining buffers */
		while (tx_desc != eop_desc) {
			tx_buffer++;
			tx_desc++;
			i++;
			if (unlikely(!i)) {
				i -= tx_ring->count;
				tx_buffer = tx_ring->tx_buffer_info;
				tx_desc = NGBE_TX_DESC(tx_ring, 0);
			}
			tx_buffer->done_time = ms;
		}
		/* move us one more past the eop_desc for start of next pkt */
		tx_buffer++;
		tx_desc++;
		i++;
		if (unlikely(!i)) {
			i -= tx_ring->count;
			tx_buffer = tx_ring->tx_buffer_info;
			tx_desc = NGBE_TX_DESC(tx_ring, 0);
		}

		/* issue prefetch for next Tx descriptor */
		prefetch(tx_desc);

		/* update budget accounting */
		budget--;
	} while (likely(budget));
	i += tx_ring->count;

	first_dma = false;
	ntf = tx_ring->next_to_free;
	free_tx_buffer = &tx_ring->tx_buffer_info[ntf];
	ntf -= tx_ring->count;
	unmapped_descs = ngbe_desc_buf_unmapped(tx_ring, i, tx_ring->next_to_free);
	while ((unmapped_descs > DESC_RESERVED) ||
	       ((ms - free_tx_buffer->done_time >= 100) && (free_tx_buffer->done_time != 0))) {
		if (ring_is_xdp(tx_ring)) {
			if (free_tx_buffer->xdpf) {
				xdp_return_frame(free_tx_buffer->xdpf);
				first_dma = true;
			}
		} else {
			if (free_tx_buffer->skb) {
				dev_consume_skb_any(free_tx_buffer->skb);
				first_dma = true;
			}
		}
		if (first_dma) {
			if (dma_unmap_len(free_tx_buffer, len)) {
				/* unmap skb header data */
				dma_unmap_single(tx_ring->dev, dma_unmap_addr(free_tx_buffer, dma),
						 dma_unmap_len(free_tx_buffer, len), DMA_TO_DEVICE);
			}
			/* clear tx_buffer data */
			if (ring_is_xdp(tx_ring))
				free_tx_buffer->xdpf = NULL;
			else
				/* clear tx_buffer data */
				free_tx_buffer->skb = NULL;
			dma_unmap_len_set(free_tx_buffer, len, 0);
			free_tx_buffer->va = NULL;
			free_tx_buffer->done_time = 0;
			first_dma = false;
		} else {
			/* unmap any remaining paged data */
			if (dma_unmap_len(free_tx_buffer, len)) {
				dma_unmap_page(tx_ring->dev, dma_unmap_addr(free_tx_buffer, dma),
					       dma_unmap_len(free_tx_buffer, len), DMA_TO_DEVICE);
				dma_unmap_len_set(free_tx_buffer, len, 0);
				free_tx_buffer->va = NULL;
			}
			free_tx_buffer->done_time = 0;
		}

		free_tx_buffer++;
		ntf++;
		if (unlikely(!ntf)) {
			ntf -= tx_ring->count;
			free_tx_buffer = tx_ring->tx_buffer_info;
		}

		unmapped_descs--;
	}

	ntf += tx_ring->count;
	tx_ring->next_to_free = ntf;
	/* need update next_to_free before next_to_clean */
	wmb();

	tx_ring->next_to_clean = i;
	u64_stats_update_begin(&tx_ring->syncp);
	tx_ring->stats.bytes += total_bytes;
	tx_ring->stats.packets += total_packets;
	u64_stats_update_end(&tx_ring->syncp);
	q_vector->tx.total_bytes += total_bytes;
	q_vector->tx.total_packets += total_packets;

	if (check_for_tx_hang(tx_ring) && ngbe_check_tx_hang(tx_ring)) {
		if (adapter->hang_cnt < 5) {
			adapter->hang_cnt++;
			goto skip;
		}

		/* schedule immediate reset if we believe we hung */
		e_err(drv,
			"Detected Tx Unit Hang%s\n"
			"  Tx Queue             <%d>\n"
			"  TDH, TDT             <%x>, <%x>\n"
			"  next_to_use          <%x>\n"
			"  next_to_clean        <%x>\n"
			"tx_buffer_info[next_to_clean]\n"
			"  time_stamp           <%lx>\n"
			"  jiffies              <%lx>\n",
			ring_is_xdp(tx_ring) ? " (XDP)" : "", tx_ring->queue_index,
			rd32(hw, NGBE_PX_TR_RP(tx_ring->reg_idx)),
			rd32(hw, NGBE_PX_TR_WP(tx_ring->reg_idx)), tx_ring->next_to_use, i,
			tx_ring->tx_buffer_info[i].time_stamp, jiffies);

		if (netif_msg_tx_err(adapter)) {
			for (j = 0; j < tx_ring->count; j++) {
				tx_desc = NGBE_TX_DESC(tx_ring, j);
				if (tx_desc->read.olinfo_status != 0x1)
					e_warn(tx_err, "q_[%d][%d]:0x%llx, 0x%x, 0x%x\n",
						tx_ring->reg_idx, j,
						tx_desc->read.buffer_addr,
						tx_desc->read.cmd_type_len,
						tx_desc->read.olinfo_status);
			}
		}

		if (netif_msg_pktdata(adapter)) {
			for (j = 0; j < tx_ring->count; j++) {
				tx_buffer = &tx_ring->tx_buffer_info[j];
				size = dma_unmap_len(tx_buffer, len);
				if (size != 0 && tx_buffer->va) {
					e_err(pktdata, "tx buffer[%d][%d]:\n",
						tx_ring->queue_index, j);
					if (netif_msg_pktdata(adapter))
						print_hex_dump(KERN_ERR, "",
								DUMP_PREFIX_OFFSET, 16, 1,
								tx_buffer->va, size, true);
				}
			}

			for (j = 0; j < tx_ring->count; j++) {
				tx_buffer = &tx_ring->tx_buffer_info[j];
				if (tx_buffer->skb) {
					e_err(pktdata,
						"****skb in tx buffer[%d][%d]: *******\n",
						tx_ring->queue_index, j);
					if (netif_msg_pktdata(adapter))
						print_hex_dump(KERN_ERR, "",
								DUMP_PREFIX_OFFSET,
								16, 1, tx_buffer->skb,
								sizeof(struct sk_buff),
								true);
				}
			}
		}

		pci_read_config_word(adapter->pdev, PCI_VENDOR_ID, &vid);
		if (vid == NGBE_FAILED_READ_CFG_WORD)
			e_info(hw, "pcie link has been lost.\n");

		if (!ring_is_xdp(tx_ring))
			netif_stop_subqueue(tx_ring->netdev, tx_ring->queue_index);

		e_info(probe, "tx hang %d detected on queue %d, resetting adapter\n",
			adapter->tx_timeout_count + 1, tx_ring->queue_index);

		if (NGBE_RECOVER_CHECK == 1) {
			if (vid == NGBE_FAILED_READ_CFG_WORD) {
				ngbe_tx_timeout_dorecovery(adapter);
			} else {
				ngbe_print_tx_hang_status(adapter);
				ngbe_tx_timeout_reset(adapter);
			}
		} else {
			ngbe_tx_timeout_dorecovery(adapter);
		}

		/* the adapter is about to reset, no point in enabling stuff */
		return true;
	}

	adapter->hang_cnt = 0;
skip:
	if (ring_is_xdp(tx_ring))
		return !!budget;
	netdev_tx_completed_queue(txring_txq(tx_ring), total_packets, total_bytes);

#define TX_WAKE_THRESHOLD (DESC_NEEDED * 2)
	if (unlikely(total_packets && netif_carrier_ok(tx_ring->netdev) &&
		     (ngbe_desc_unused(tx_ring) >= TX_WAKE_THRESHOLD))) {
		/* Make sure that anybody stopping the queue after this
		 * sees the new next_to_clean.
		 */
		smp_mb();
		if (__netif_subqueue_stopped(tx_ring->netdev, tx_ring->queue_index) &&
		    !test_bit(__NGBE_DOWN, &adapter->state)) {
			netif_wake_subqueue(tx_ring->netdev, tx_ring->queue_index);
			++tx_ring->tx_stats.restart_queue;
		}
	}

	return !!budget;
}

#define NGBE_RSS_L4_TYPES_MASK                                                      \
	((1ul << NGBE_RXD_RSSTYPE_IPV4_TCP) | (1ul << NGBE_RXD_RSSTYPE_IPV4_UDP) |  \
	 (1ul << NGBE_RXD_RSSTYPE_IPV4_SCTP) | (1ul << NGBE_RXD_RSSTYPE_IPV6_TCP) | \
	 (1ul << NGBE_RXD_RSSTYPE_IPV6_UDP) | (1ul << NGBE_RXD_RSSTYPE_IPV6_SCTP))

static inline void ngbe_rx_hash(struct ngbe_ring *ring, union ngbe_rx_desc *rx_desc,
				struct sk_buff *skb)
{
	u16 rss_type;

	if (!(ring->netdev->features & NETIF_F_RXHASH))
		return;

	rss_type = le16_to_cpu(rx_desc->wb.lower.lo_dword.hs_rss.pkt_info) & NGBE_RXD_RSSTYPE_MASK;

	if (!rss_type)
		return;

	skb_set_hash(skb, le32_to_cpu(rx_desc->wb.lower.hi_dword.rss),
		     (NGBE_RSS_L4_TYPES_MASK & (1ul << rss_type)) ? PKT_HASH_TYPE_L4
								  : PKT_HASH_TYPE_L3);
}

/**
 * ngbe_rx_checksum - indicate in skb if hw indicated a good cksum
 * @ring: structure containing ring specific data
 * @rx_desc: current Rx descriptor being processed
 * @skb: skb currently being received and modified
 **/
static inline void ngbe_rx_checksum(struct ngbe_ring *ring, union ngbe_rx_desc *rx_desc,
				    struct sk_buff *skb)
{
	struct ngbe_dec_ptype dptype = decode_rx_desc_ptype(rx_desc);
	struct ngbe_adapter *adapter = ring->q_vector->adapter;
	unsigned int data_len, size;
	skb_frag_t *frag;

	skb->ip_summed = CHECKSUM_NONE;

	skb_checksum_none_assert(skb);

	/* Rx csum disabled */
	if (!(ring->netdev->features & NETIF_F_RXCSUM))
		return;

	/* if IPv4 header checksum error */
	if ((ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_IPCS) &&
	     ngbe_test_staterr(rx_desc, NGBE_RXD_ERR_IPE)) ||
	    (ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_OUTERIPCS) &&
	     ngbe_test_staterr(rx_desc, NGBE_RXD_ERR_OUTERIPER))) {
		ring->rx_stats.csum_err++;
		e_err(rx_err, "ip csum failed.rx desc: 0x%llx, 0x%llx\n", rx_desc->read.pkt_addr,
		      rx_desc->read.hdr_addr);
		if (netif_msg_rx_err(adapter)) {
			data_len = skb->data_len;
			print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, 16, 1, skb->data,
				       skb_headlen(skb), true);
			for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
				if (!data_len)
					break;
				size = skb_frag_size(frag);
				data_len -= size;
				print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, 16, 1,
					       skb_frag_address_safe(frag), size, true);
			}
		}
		return;
	}

	/* L4 checksum offload flag must set for the below code to work */
	if (!ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_L4CS))
		return;

	/*likely incorrect csum if IPv6 Dest Header found */
	if (dptype.prot != NGBE_DEC_PTYPE_PROT_SCTP && ngbe_test_staterr(rx_desc, NGBE_RXD_IPV6EX))
		return;

	/* if L4 checksum error */
	if (ngbe_test_staterr(rx_desc, NGBE_RXD_ERR_TCPE)) {
		ring->rx_stats.csum_err++;
		e_err(rx_err, "L4 csum failed.rx desc: 0x%llx, 0x%llx\n", rx_desc->read.pkt_addr,
		      rx_desc->read.hdr_addr);
		if (netif_msg_rx_err(adapter)) {
			data_len = skb->data_len;
			print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, 16, 1, skb->data,
				       skb_headlen(skb), true);
			for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
				if (!data_len)
					break;
				size = skb_frag_size(frag);
				data_len -= size;
				print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, 16, 1,
					       skb_frag_address_safe(frag), size, true);
			}
		}
		return;
	}
	/* If there is an outer header present that might contain a checksum
	 * we need to bump the checksum level by 1 to reflect the fact that
	 * we are indicating we validated the inner checksum.
	 */
	if (dptype.etype >= NGBE_DEC_PTYPE_ETYPE_IG)
		skb->csum_level = 1;

	/* It must be a TCP or UDP or SCTP packet with a valid checksum */
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	ring->rx_stats.csum_good_cnt++;
}

static bool ngbe_alloc_mapped_skb(struct ngbe_ring *rx_ring, struct ngbe_rx_buffer *bi)
{
	struct sk_buff *skb = bi->skb;
	dma_addr_t dma = bi->dma;

	if (unlikely(dma))
		return true;

	if (likely(!skb)) {
		skb = netdev_alloc_skb_ip_align(rx_ring->netdev, rx_ring->rx_buf_len);
		if (unlikely(!skb)) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			return false;
		}

		bi->skb = skb;
	}

	dma = dma_map_single(rx_ring->dev, skb->data, rx_ring->rx_buf_len, DMA_FROM_DEVICE);

	/* if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		dev_kfree_skb_any(skb);
		bi->skb = NULL;

		rx_ring->rx_stats.alloc_rx_buff_failed++;
		return false;
	}

	bi->dma = dma;
	return true;
}

static bool ngbe_alloc_mapped_page(struct ngbe_ring *rx_ring, struct ngbe_rx_buffer *bi)
{
	struct page *page = bi->page;
	dma_addr_t dma;

	/* since we are recycling buffers we should seldom need to alloc */
	if (likely(page))
		return true;

	/* alloc new page for storage */
	page = dev_alloc_pages(ngbe_rx_pg_order(rx_ring));
	if (unlikely(!page)) {
		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	/* map page for use */
	dma = dma_map_page(rx_ring->dev, page, 0, ngbe_rx_pg_size(rx_ring), DMA_FROM_DEVICE);

	/* if mapping failed free memory back to system since
	 * there isn't much point in holding memory we can't use
	 */
	if (dma_mapping_error(rx_ring->dev, dma)) {
		__free_pages(page, ngbe_rx_pg_order(rx_ring));

		rx_ring->rx_stats.alloc_rx_page_failed++;
		return false;
	}

	bi->page_dma = dma;
	bi->page = page;
	bi->page_offset = ngbe_rx_offset(rx_ring);
	page_ref_add(page, USHRT_MAX - 1);
	bi->pagecnt_bias = USHRT_MAX;

	return true;
}

/**
 * ngbe_alloc_rx_buffers - Replace used receive buffers
 * @rx_ring: ring to place buffers on
 * @cleaned_count: number of buffers to replace
 **/
void ngbe_alloc_rx_buffers(struct ngbe_ring *rx_ring, u16 cleaned_count)
{
	union ngbe_rx_desc *rx_desc;
	struct ngbe_rx_buffer *bi;
	u16 i = rx_ring->next_to_use;

	/* nothing to do */
	if (!cleaned_count)
		return;

	rx_desc = NGBE_RX_DESC(rx_ring, i);
	bi = &rx_ring->rx_buffer_info[i];
	i -= rx_ring->count;

	do {
		if (ring_is_hs_enabled(rx_ring)) {
			if (!ngbe_alloc_mapped_skb(rx_ring, bi))
				break;
			rx_desc->read.hdr_addr = cpu_to_le64(bi->dma);
		}

		if (!ngbe_alloc_mapped_page(rx_ring, bi))
			break;
		rx_desc->read.pkt_addr = cpu_to_le64(bi->page_dma + bi->page_offset);

		rx_desc++;
		bi++;
		i++;
		if (unlikely(!i)) {
			rx_desc = NGBE_RX_DESC(rx_ring, 0);
			bi = rx_ring->rx_buffer_info;
			i -= rx_ring->count;
		}

		/* clear the status bits for the next_to_use descriptor */
		rx_desc->wb.upper.status_error = 0;

		cleaned_count--;
	} while (cleaned_count);

	i += rx_ring->count;

	if (rx_ring->next_to_use != i) {
		rx_ring->next_to_use = i;
		/* update next to alloc since we have filled the ring */
		rx_ring->next_to_alloc = i;
		/* Force memory writes to complete before letting h/w
		 * know there are new descriptors to fetch.  (Only
		 * applicable for weak-ordered memory model archs,
		 * such as IA-64).
		 */
		wmb();
		writel(i, rx_ring->tail);
	}
}

static inline u16 ngbe_get_hlen(struct ngbe_ring *rx_ring, union ngbe_rx_desc *rx_desc)
{
	__le16 hdr_info = rx_desc->wb.lower.lo_dword.hs_rss.hdr_info;
	u16 hlen = le16_to_cpu(hdr_info) & NGBE_RXD_HDRBUFLEN_MASK;

	if (hlen > (NGBE_RX_HDR_SIZE << NGBE_RXD_HDRBUFLEN_SHIFT))
		hlen = 0;
	else
		hlen >>= NGBE_RXD_HDRBUFLEN_SHIFT;

	return hlen;
}

static void ngbe_rx_vlan(struct ngbe_ring *ring, union ngbe_rx_desc *rx_desc, struct sk_buff *skb)
{
	u8 idx = 0;
	u16 ethertype;

	if ((ring->netdev->features & (NETIF_F_HW_VLAN_CTAG_RX | NETIF_F_HW_VLAN_STAG_RX)) &&
	    ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_VP)) {
		idx = (le16_to_cpu(rx_desc->wb.lower.lo_dword.hs_rss.pkt_info) &
		       NGBE_RXD_TPID_MASK) >>
		      NGBE_RXD_TPID_SHIFT;
		ethertype = ring->q_vector->adapter->hw.tpid[idx];
		__vlan_hwaccel_put_tag(skb, htons(ethertype), le16_to_cpu(rx_desc->wb.upper.vlan));
	}
}

/**
 * ngbe_process_skb_fields - Populate skb header fields from Rx descriptor
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @rx_desc: pointer to the EOP Rx descriptor
 * @skb: pointer to current skb being populated
 * This function checks the ring, descriptor, and packet information in
 * order to populate the hash, checksum, VLAN, timestamp, protocol, and
 * other fields within the skb.
 **/
void ngbe_process_skb_fields(struct ngbe_ring *rx_ring, union ngbe_rx_desc *rx_desc,
			     struct sk_buff *skb)
{
	u32 flags = rx_ring->q_vector->adapter->flags;

	ngbe_rx_hash(rx_ring, rx_desc, skb);

	ngbe_rx_checksum(rx_ring, rx_desc, skb);
	if (unlikely(flags & NGBE_FLAG_RX_HWTSTAMP_ENABLED) &&
	    unlikely(ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_TS))) {
		ngbe_ptp_rx_hwtstamp(rx_ring->q_vector->adapter, skb);
		rx_ring->last_rx_timestamp = jiffies;
	}

	ngbe_rx_vlan(rx_ring, rx_desc, skb);

	skb_record_rx_queue(skb, rx_ring->queue_index);

	skb->protocol = eth_type_trans(skb, rx_ring->netdev);
}

void ngbe_rx_skb(struct ngbe_q_vector *q_vector, struct ngbe_ring *rx_ring,
		 union ngbe_rx_desc *rx_desc, struct sk_buff *skb)
{
	napi_gro_receive(&q_vector->napi, skb);
}

/**
 * ngbe_is_non_eop - process handling of non-EOP buffers
 * @rx_ring: Rx ring being processed
 * @rx_desc: Rx descriptor for current buffer
 * @skb: Current socket buffer containing buffer in progress
 * This function updates next to clean.  If the buffer is an EOP buffer
 * this function exits returning false, otherwise it will place the
 * sk_buff in the next buffer to be chained and return true indicating
 * that this is in fact a non-EOP buffer.
 **/
static bool ngbe_is_non_eop(struct ngbe_ring *rx_ring, union ngbe_rx_desc *rx_desc,
			    struct sk_buff *skb)
{
	struct ngbe_rx_buffer *rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
	u32 ntc = rx_ring->next_to_clean + 1;

	/* fetch, update, and store next to clean */
	ntc = (ntc < rx_ring->count) ? ntc : 0;
	rx_ring->next_to_clean = ntc;

	prefetch(NGBE_RX_DESC(rx_ring, ntc));

	/* if we are the last buffer then there is nothing else to do */
	if (likely(ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_EOP)))
		return false;

	/* place skb in next buffer to be received */
	if (ring_is_hs_enabled(rx_ring)) {
		rx_buffer->skb = rx_ring->rx_buffer_info[ntc].skb;
		rx_buffer->dma = rx_ring->rx_buffer_info[ntc].dma;
		rx_ring->rx_buffer_info[ntc].dma = 0;
	}
	rx_ring->rx_buffer_info[ntc].skb = skb;
	rx_ring->rx_stats.non_eop_descs++;

	return true;
}

#define NGBE_TXD_CMD (NGBE_TXD_EOP | NGBE_TXD_RS)

/**
 * ngbe_pull_tail - ngbe specific version of skb_pull_tail
 * @skb: pointer to current skb being adjusted
 * This function is an ngbe specific version of __pskb_pull_tail.  The
 * main difference between this version and the original function is that
 * this function can make several assumptions about the state of things
 * that allow for significant optimizations versus the standard function.
 * As a result we can do things like drop a frag and maintain an accurate
 * truesize for the skb.
 */
static void ngbe_pull_tail(struct sk_buff *skb)
{
	skb_frag_t *frag = &skb_shinfo(skb)->frags[0];
	unsigned char *va;
	unsigned int pull_len;

	/* it is valid to use page_address instead of kmap since we are
	 * working with pages allocated out of the lomem pool per
	 * alloc_page(GFP_ATOMIC)
	 */
	va = skb_frag_address(frag);

	/* we need the header to contain the greater of either ETH_HLEN or
	 * 60 bytes if the skb->len is less than 60 for skb_pad.
	 */
	pull_len = eth_get_headlen(skb->dev, va, NGBE_RX_HDR_SIZE);

	/* align pull length to size of long to optimize memcpy performance */
	skb_copy_to_linear_data(skb, va, ALIGN(pull_len, sizeof(long)));

	/* update all of the pointers */
	skb_frag_size_sub(frag, pull_len);
	skb_frag_off_add(frag, pull_len);
	skb->data_len -= pull_len;
	skb->tail += pull_len;
}

/**
 * ngbe_dma_sync_frag - perform DMA sync for first frag of SKB
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @skb: pointer to current skb being updated
 * This function provides a basic DMA sync up for the first fragment of an
 * skb.  The reason for doing this is that the first fragment cannot be
 * unmapped until we have reached the end of packet descriptor for a buffer
 * chain.
 */
static void ngbe_dma_sync_frag(struct ngbe_ring *rx_ring, struct sk_buff *skb)
{
	/* if the page was released unmap it, else just sync our portion */
	if (unlikely(NGBE_CB(skb)->page_released)) {
		dma_unmap_page_attrs(rx_ring->dev, NGBE_CB(skb)->dma, ngbe_rx_pg_size(rx_ring),
				     DMA_FROM_DEVICE, NGBE_RX_DMA_ATTR);
	} else if (ring_uses_build_skb(rx_ring)) {
		unsigned long offset = (unsigned long)(skb->data) & ~PAGE_MASK;

		dma_sync_single_range_for_cpu(rx_ring->dev, NGBE_CB(skb)->dma, offset,
					      skb_headlen(skb), DMA_FROM_DEVICE);
	} else {
		skb_frag_t *frag = &skb_shinfo(skb)->frags[0];

		dma_sync_single_range_for_cpu(rx_ring->dev, NGBE_CB(skb)->dma, skb_frag_off(frag),
					      skb_frag_size(frag), DMA_FROM_DEVICE);
	}
}

/**
 * ngbe_cleanup_headers - Correct corrupted or empty headers
 * @rx_ring: rx descriptor ring packet is being transacted on
 * @rx_desc: pointer to the EOP Rx descriptor
 * @skb: pointer to current skb being fixed
 * Check for corrupted packet headers caused by senders on the local L2
 * embedded NIC switch not setting up their Tx Descriptors right.  These
 * should be very rare.
 * Also address the case where we are pulling data in on pages only
 * and as such no data is present in the skb header.
 * In addition if skb is not at least 60 bytes we need to pad it so that
 * it is large enough to qualify as a valid Ethernet frame.
 * Returns true if an error was encountered and skb was freed.
 **/
bool ngbe_cleanup_headers(struct ngbe_ring *rx_ring, union ngbe_rx_desc *rx_desc,
			  struct sk_buff *skb)
{
	struct net_device *netdev = rx_ring->netdev;

	if (IS_ERR(skb))
		return true;

	/* verify that the packet does not have any known errors */
	if (unlikely(ngbe_test_staterr(rx_desc, NGBE_RXD_ERR_FRAME_ERR_MASK) &&
		     !(netdev->features & NETIF_F_RXALL))) {
		dev_kfree_skb_any(skb);
		return true;
	}

	/* place header in linear portion of buffer */
	if (skb_is_nonlinear(skb) && !skb_headlen(skb))
		ngbe_pull_tail(skb);

	/* if eth_skb_pad returns an error the skb was freed */
	if (eth_skb_pad(skb))
		return true;

	return false;
}

/**
 * ngbe_reuse_rx_page - page flip buffer and store it back on the ring
 * @rx_ring: rx descriptor ring to store buffers on
 * @old_buff: donor buffer to have page reused
 * Synchronizes page for reuse by the adapter
 **/
static void ngbe_reuse_rx_page(struct ngbe_ring *rx_ring, struct ngbe_rx_buffer *old_buff)
{
	struct ngbe_rx_buffer *new_buff;
	u16 nta = rx_ring->next_to_alloc;

	new_buff = &rx_ring->rx_buffer_info[nta];

	/* update, and store next to alloc */
	nta++;
	rx_ring->next_to_alloc = (nta < rx_ring->count) ? nta : 0;

	/* transfer page from old buffer to new buffer */
	new_buff->page_dma = old_buff->page_dma;
	new_buff->page = old_buff->page;
	new_buff->page_offset = old_buff->page_offset;
	new_buff->pagecnt_bias = old_buff->pagecnt_bias;

	/* sync the buffer for use by the device */
	dma_sync_single_range_for_device(rx_ring->dev, new_buff->page_dma, new_buff->page_offset,
					 ngbe_rx_bufsz(rx_ring), DMA_FROM_DEVICE);
}

static inline bool ngbe_page_is_reserved(struct page *page)
{
	return (page_to_nid(page) != numa_mem_id()) || page_is_pfmemalloc(page);
}

/**
 * ngbe_add_rx_frag - Add contents of Rx buffer to sk_buff
 * @rx_ring: rx descriptor ring to transact packets on
 * @rx_buffer: buffer containing page to add
 * @rx_desc: descriptor containing length of buffer written by hardware
 * @skb: sk_buff to place the data into
 * This function will add the data contained in rx_buffer->page to the skb.
 * This is done either through a direct copy if the data in the buffer is
 * less than the skb header size, otherwise it will just attach the page as
 * a frag to the skb.
 * The function will then update the page offset if necessary and return
 * true if the buffer can be reused by the adapter.
 **/
static bool ngbe_add_rx_frag(struct ngbe_ring *rx_ring, struct ngbe_rx_buffer *rx_buffer,
			     union ngbe_rx_desc *rx_desc, struct sk_buff *skb)
{
	struct page *page = rx_buffer->page;
	unsigned int size = le16_to_cpu(rx_desc->wb.upper.length);
#if (PAGE_SIZE < 8192)
	unsigned int truesize = ngbe_rx_bufsz(rx_ring);
#else
	unsigned int truesize = ALIGN(size, L1_CACHE_BYTES);
	unsigned int last_offset = ngbe_rx_pg_size(rx_ring) - ngbe_rx_bufsz(rx_ring);
#endif

	if (size <= NGBE_RX_HDR_SIZE && !skb_is_nonlinear(skb) && !ring_is_hs_enabled(rx_ring)) {
		unsigned char *va = page_address(page) + rx_buffer->page_offset;

		memcpy(__skb_put(skb, size), va, ALIGN(size, sizeof(long)));
		rx_buffer->pagecnt_bias++;
		/* page is not reserved, we can reuse buffer as-is */
		if (likely(!ngbe_page_is_reserved(page)))
			return true;

		return false;
	}

	skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, page, rx_buffer->page_offset, size,
			truesize);

	/* avoid re-using remote pages */
	if (unlikely(ngbe_page_is_reserved(page)))
		return false;

#if (PAGE_SIZE < 8192)
	/* if we are only owner of page we can reuse it */
	if (unlikely((page_ref_count(page) - rx_buffer->pagecnt_bias) > 1))
		return false;

	/* flip page offset to other buffer */
	rx_buffer->page_offset ^= truesize;
#else
	/* move offset up to the next cache line */
	rx_buffer->page_offset += truesize;

	if (rx_buffer->page_offset > last_offset)
		return false;
#endif

	/* Even if we own the page, we are not allowed to use atomic_set()
	 * This would break get_page_unless_zero() users.
	 */
	/* If we have drained the page fragment pool we need to update
	 * the pagecnt_bias and page count so that we fully restock the
	 * number of references the driver holds.
	 */
	if (unlikely(rx_buffer->pagecnt_bias == 1)) {
		page_ref_add(page, USHRT_MAX - 1);
		rx_buffer->pagecnt_bias = USHRT_MAX;
	}
	return true;
}

static struct sk_buff *ngbe_fetch_rx_buffer(struct ngbe_ring *rx_ring, union ngbe_rx_desc *rx_desc)
{
	struct ngbe_rx_buffer *rx_buffer;
	struct sk_buff *skb;
	struct page *page;

	rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
	page = rx_buffer->page;
	prefetchw(page);

	skb = rx_buffer->skb;

	if (likely(!skb)) {
		void *page_addr = page_address(page) + rx_buffer->page_offset;

		/* prefetch first cache line of first page */
		prefetch(page_addr);
#if L1_CACHE_BYTES < 128
		prefetch(page_addr + L1_CACHE_BYTES);
#endif

		/* allocate a skb to store the frags */
		skb = netdev_alloc_skb_ip_align(rx_ring->netdev, NGBE_RX_HDR_SIZE);
		if (unlikely(!skb)) {
			rx_ring->rx_stats.alloc_rx_buff_failed++;
			return NULL;
		}

		/* we will be copying header into skb->data in
		 * pskb_may_pull so it is in our interest to prefetch
		 * it now to avoid a possible cache miss
		 */
		prefetchw(skb->data);

		/* Delay unmapping of the first packet. It carries the
		 * header information, HW may still access the header
		 * after the writeback.  Only unmap it when EOP is
		 * reached
		 */
		if (likely(ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_EOP)))
			goto dma_sync;

		NGBE_CB(skb)->dma = rx_buffer->page_dma;
	} else {
		if (ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_EOP))
			ngbe_dma_sync_frag(rx_ring, skb);

dma_sync:
		/* we are reusing so sync this buffer for CPU use */
		dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->page_dma,
					      rx_buffer->page_offset, ngbe_rx_bufsz(rx_ring),
					      DMA_FROM_DEVICE);

		rx_buffer->skb = NULL;
	}

	/* pull page into skb */
	if (ngbe_add_rx_frag(rx_ring, rx_buffer, rx_desc, skb)) {
		/* hand second half of page back to the ring */
		ngbe_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		if (NGBE_CB(skb)->dma == rx_buffer->page_dma) {
			/* the page has been released from the ring */
			NGBE_CB(skb)->page_released = true;
		} else {
			/* we are not reusing the buffer so unmap it */
			dma_unmap_page(rx_ring->dev, rx_buffer->page_dma, ngbe_rx_pg_size(rx_ring),
				       DMA_FROM_DEVICE);
		}
		/* this page cannot be reused so discard it */
		__page_frag_cache_drain(rx_buffer->page, rx_buffer->pagecnt_bias);
	}
	/* clear contents of buffer_info */
	rx_buffer->page = NULL;

	return skb;
}

static struct sk_buff *ngbe_fetch_rx_buffer_hs(struct ngbe_ring *rx_ring,
					       union ngbe_rx_desc *rx_desc)
{
	struct ngbe_rx_buffer *rx_buffer;
	struct sk_buff *skb;
	struct page *page;
	int hdr_len = 0;

	rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
	page = rx_buffer->page;
	prefetchw(page);

	skb = rx_buffer->skb;
	rx_buffer->skb = NULL;
	prefetchw(skb->data);

	if (!skb_is_nonlinear(skb)) {
		hdr_len = ngbe_get_hlen(rx_ring, rx_desc);
		if (hdr_len > 0) {
			__skb_put(skb, hdr_len);
			NGBE_CB(skb)->dma_released = true;
			NGBE_CB(skb)->dma = rx_buffer->dma;
			rx_buffer->dma = 0;
		} else {
			dma_unmap_single(rx_ring->dev, rx_buffer->dma, rx_ring->rx_buf_len,
					 DMA_FROM_DEVICE);
			rx_buffer->dma = 0;
			if (likely(ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_EOP)))
				goto dma_sync;
			NGBE_CB(skb)->dma = rx_buffer->page_dma;
			goto add_frag;
		}
	}

	if (ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_EOP)) {
		if (skb_headlen(skb)) {
			if (NGBE_CB(skb)->dma_released) {
				dma_unmap_single(rx_ring->dev, NGBE_CB(skb)->dma,
						 rx_ring->rx_buf_len, DMA_FROM_DEVICE);
				NGBE_CB(skb)->dma = 0;
				NGBE_CB(skb)->dma_released = false;
			}
		} else {
			ngbe_dma_sync_frag(rx_ring, skb);
		}
	}

dma_sync:
	/* we are reusing so sync this buffer for CPU use */
	dma_sync_single_range_for_cpu(rx_ring->dev, rx_buffer->page_dma, rx_buffer->page_offset,
				      ngbe_rx_bufsz(rx_ring), DMA_FROM_DEVICE);
add_frag:
	/* pull page into skb */
	if (ngbe_add_rx_frag(rx_ring, rx_buffer, rx_desc, skb)) {
		/* hand second half of page back to the ring */
		ngbe_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		if (NGBE_CB(skb)->dma == rx_buffer->page_dma) {
			/* the page has been released from the ring */
			NGBE_CB(skb)->page_released = true;
		} else {
			/* we are not reusing the buffer so unmap it */
			dma_unmap_page(rx_ring->dev, rx_buffer->page_dma, ngbe_rx_pg_size(rx_ring),
				       DMA_FROM_DEVICE);
		}
		/* this page cannot be reused so discard it */
		__page_frag_cache_drain(rx_buffer->page, rx_buffer->pagecnt_bias);
	}

	/* clear contents of buffer_info */
	rx_buffer->page = NULL;

	return skb;
}

int ngbe_xmit_xdp_ring(struct ngbe_ring *ring, struct xdp_frame *xdpf)
{
	u8 nr_frags = 0;
	struct ngbe_tx_buffer *tx_buffer;
	union ngbe_tx_desc *tx_desc;
	u32 len, cmd_type = 0;
	dma_addr_t dma;
	u16 i = 0, index = ring->next_to_use;
	struct ngbe_tx_buffer *tx_head = &ring->tx_buffer_info[index];
	void *data;

	len = xdpf->len;
	data = xdpf->data;
	if (unlikely(!ngbe_desc_unused(ring)))
		return NGBE_XDP_CONSUMED;

	/* record the location of the first descriptor for this packet */
	tx_buffer = &ring->tx_buffer_info[ring->next_to_use];
	tx_buffer->bytecount = xdp_get_frame_len(xdpf);
	tx_buffer->gso_segs = 1;

	tx_buffer->xdpf = xdpf;
	tx_desc = NGBE_TX_DESC(ring, index);
	tx_desc->read.olinfo_status = cpu_to_le32(tx_buffer->bytecount << NGBE_TXD_PAYLEN_SHIFT);
	for (;;) {
		dma = dma_map_single(ring->dev, data, len, DMA_TO_DEVICE);
		if (dma_mapping_error(ring->dev, dma))
			goto unmap;
		dma_unmap_len_set(tx_buffer, len, len);
		dma_unmap_addr_set(tx_buffer, dma, dma);

		cmd_type = ngbe_tx_cmd_type(tx_buffer->tx_flags);
		cmd_type |= len;

		tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type);
		tx_desc->read.buffer_addr = cpu_to_le64(dma);
		tx_buffer->protocol = 0;

		if (++index == ring->count)
			index = 0;

		if (i == nr_frags)
			break;

		tx_buffer = &ring->tx_buffer_info[index];
		tx_desc = NGBE_TX_DESC(ring, index);
		tx_desc->read.olinfo_status = 0;
		i++;
	}
	tx_desc->read.cmd_type_len |= cpu_to_le32(NGBE_TXD_CMD);
	/* Avoid any potential race with xdp_xmit and cleanup */
	smp_wmb();

	tx_head->next_to_watch = tx_desc;
	ring->next_to_use = index;

	ring->xdp_tx_active += nr_frags;

	return NGBE_XDP_TX;
unmap:
	for (;;) {
		tx_buffer = &ring->tx_buffer_info[index];
		if (dma_unmap_len(tx_buffer, len))
			dma_unmap_page(ring->dev, dma_unmap_addr(tx_buffer, dma),
				       dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
		dma_unmap_len_set(tx_buffer, len, 0);
		if (tx_buffer == tx_head)
			break;

		if (!index)
			index += ring->count;
		index--;
	}
	return NGBE_XDP_CONSUMED;
}

static bool ngbe_can_reuse_rx_page(struct ngbe_rx_buffer *rx_buffer, struct ngbe_ring *rx_ring)
{
	unsigned int pagecnt_bias = rx_buffer->pagecnt_bias;
	struct page *page = rx_buffer->page;
#if (PAGE_SIZE < 8192)
	/* if we are only owner of page we can reuse it */
	if (unlikely((page_ref_count(page) - pagecnt_bias) > 1))
		return false;
#else
	unsigned int last_offset = SKB_WITH_OVERHEAD(PAGE_SIZE) - NGBE_RXBUFFER_3K;

	if (rx_buffer->page_offset > last_offset)
		return false;
#endif

	/* avoid re-using remote pages */
	if (unlikely(ngbe_page_is_reserved(page)))
		return false;

	/* If we have drained the page fragment pool we need to update
	 * the pagecnt_bias and page count so that we fully restock the
	 * number of references the driver holds.
	 */
	if (unlikely(pagecnt_bias == 1)) {
		page_ref_add(page, USHRT_MAX - 1);
		rx_buffer->pagecnt_bias = USHRT_MAX;
	}
	return true;
}

static void ngbe_put_rx_buffer(struct ngbe_ring *rx_ring, struct ngbe_rx_buffer *rx_buffer,
			       struct sk_buff *skb)
{
	if (ngbe_can_reuse_rx_page(rx_buffer, rx_ring)) {
		/* hand second half of page back to the ring */
		ngbe_reuse_rx_page(rx_ring, rx_buffer);
	} else {
		/* We are not reusing the buffer so unmap it and free
		 * any references we are holding to it
		 */
		if (IS_ERR(skb))
			dma_unmap_page(rx_ring->dev, rx_buffer->page_dma, ngbe_rx_pg_size(rx_ring),
				       DMA_FROM_DEVICE);
		__page_frag_cache_drain(rx_buffer->page, rx_buffer->pagecnt_bias);
	}

	/* clear contents of rx_buffer */
	rx_buffer->page = NULL;
	rx_buffer->skb = NULL;
}

static struct sk_buff *ngbe_run_xdp(struct ngbe_adapter __maybe_unused *adapter,
				    struct ngbe_ring __maybe_unused *rx_ring,
				    struct ngbe_rx_buffer __maybe_unused *rx_buffer,
				    struct xdp_buff __maybe_unused *xdp)
{
	int result = NGBE_XDP_PASS;
	struct bpf_prog *xdp_prog;
	struct ngbe_ring *ring;
	struct xdp_frame *xdpf;
	int err;
	u32 act;

	rcu_read_lock();
	xdp_prog = READ_ONCE(rx_ring->xdp_prog);

	if (!xdp_prog)
		goto xdp_out;

	prefetchw(xdp->data_hard_start); /* xdp_frame write */
	act = bpf_prog_run_xdp(xdp_prog, xdp);
	switch (act) {
	case XDP_PASS:
		break;
	case XDP_TX:
		xdpf = xdp_convert_buff_to_frame(xdp);
		if (unlikely(!xdpf)) {
			result = NGBE_XDP_CONSUMED;
			break;
		}
		ring = adapter->xdp_ring[rx_ring->queue_index % adapter->num_xdp_queues];
		if (static_branch_unlikely(&ngbe_xdp_locking_key))
			spin_lock(&ring->tx_lock);
		result = ngbe_xmit_xdp_ring(ring, xdpf);
		if (static_branch_unlikely(&ngbe_xdp_locking_key))
			spin_unlock(&ring->tx_lock);
		break;
	case XDP_REDIRECT:
		err = xdp_do_redirect(adapter->netdev, xdp, xdp_prog);
		if (!err)
			result = NGBE_XDP_REDIR;
		else
			result = NGBE_XDP_CONSUMED;
		break;
	default:
		bpf_warn_invalid_xdp_action(rx_ring->netdev, xdp_prog, act);
		fallthrough;
	case XDP_ABORTED:
		trace_xdp_exception(rx_ring->netdev, xdp_prog, act);
		/* handle aborts by dropping packet */
		fallthrough;
	case XDP_DROP:
		result = NGBE_XDP_CONSUMED;
		break;
	}
xdp_out:
	rcu_read_unlock();

	return ERR_PTR(-result);
}

static unsigned int ngbe_rx_frame_truesize(struct ngbe_ring *rx_ring, unsigned int size)
{
	unsigned int truesize;

#if (PAGE_SIZE < 8192)
	truesize = ngbe_rx_pg_size(rx_ring) / 2;
#else
	truesize = SKB_DATA_ALIGN(ngbe_rx_offset(rx_ring) + size) +
		   SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
#endif
	return truesize;
}

static void ngbe_rx_buffer_flip(struct ngbe_ring *rx_ring, struct ngbe_rx_buffer *rx_buffer,
				unsigned int size)
{
	unsigned int truesize = ngbe_rx_frame_truesize(rx_ring, size);
#if (PAGE_SIZE < 8192)
	rx_buffer->page_offset ^= truesize;
#else
	rx_buffer->page_offset += truesize;
#endif
}

/**
 * ngbe_clean_rx_irq - Clean completed descriptors from Rx ring - bounce buf
 * @q_vector: structure containing interrupt and ring information
 * @rx_ring: rx descriptor ring to transact packets on
 * @budget: Total limit on number of packets to process
 * This function provides a "bounce buffer" approach to Rx interrupt
 * processing.  The advantage to this is that on systems that have
 * expensive overhead for IOMMU access this provides a means of avoiding
 * it by maintaining the mapping of the page to the system.
 * Returns amount of work completed.
 **/
static int ngbe_clean_rx_irq(struct ngbe_q_vector *q_vector, struct ngbe_ring *rx_ring, int budget)
{
	unsigned int total_rx_bytes = 0, total_rx_packets = 0, xdp_xmit = 0;
	u16 cleaned_count = ngbe_desc_unused(rx_ring);
	struct ngbe_adapter *adapter = q_vector->adapter;
	struct xdp_buff xdp;

	xdp.data = NULL;
	xdp.data_end = NULL;
	if (rx_ring->xdp_prog)
		xdp.rxq = &rx_ring->xdp_rxq;

	/* Frame size depend on rx_ring setup when PAGE_SIZE=4K */
#if (PAGE_SIZE < 8192)
	if (rx_ring->xdp_prog)
		xdp.frame_sz = ngbe_rx_frame_truesize(rx_ring, 0);
#endif
	do {
		struct ngbe_rx_buffer *rx_buffer;
		union ngbe_rx_desc *rx_desc;
		struct sk_buff *skb = NULL;
		unsigned int size = 0;

		rx_buffer = &rx_ring->rx_buffer_info[rx_ring->next_to_clean];
		/* return some buffers to hardware, one at a time is too slow */
		if (cleaned_count >= NGBE_RX_BUFFER_WRITE) {
			ngbe_alloc_rx_buffers(rx_ring, cleaned_count);
			cleaned_count = 0;
		}

		rx_desc = NGBE_RX_DESC(rx_ring, rx_ring->next_to_clean);

		if (!ngbe_test_staterr(rx_desc, NGBE_RXD_STAT_DD))
			break;

		if (rx_ring->xdp_prog) {
			size = le16_to_cpu(rx_desc->wb.upper.length);
			if (!size)
				break;
		}
		/* This memory barrier is needed to keep us from reading
		 * any other fields out of the rx_desc until we know the
		 * descriptor has been written back
		 */
		dma_rmb();

		rx_buffer->pagecnt_bias--;
		if (rx_ring->xdp_prog) {
			prefetchw(rx_buffer->page);
			xdp.data = page_address(rx_buffer->page) + rx_buffer->page_offset;
			xdp.data_meta = xdp.data;
			xdp.data_hard_start = xdp.data - ngbe_rx_offset(rx_ring);
			xdp.data_end = xdp.data + size;
			xdp_buff_clear_frags_flag(&xdp);
#if (PAGE_SIZE > 4096)
			/* At larger PAGE_SIZE, frame_sz depend on len size */
			xdp.frame_sz = ngbe_rx_frame_truesize(rx_ring, size);
#endif
			skb = ngbe_run_xdp(adapter, rx_ring, rx_buffer, &xdp);
		}
		if (IS_ERR(skb)) {
			if ((PTR_ERR(skb) == -NGBE_XDP_TX) || (PTR_ERR(skb) == -NGBE_XDP_REDIR)) {
				xdp_xmit = (-PTR_ERR(skb));
				ngbe_rx_buffer_flip(rx_ring, rx_buffer, size);
			} else {
				rx_buffer->pagecnt_bias++;
			}
			total_rx_packets++;
			total_rx_bytes += size;
		} else {
			if (ring_is_hs_enabled(rx_ring))
				skb = ngbe_fetch_rx_buffer_hs(rx_ring, rx_desc);
			else
				skb = ngbe_fetch_rx_buffer(rx_ring, rx_desc);
		}
		/* exit if we failed to retrieve a buffer */
		if (!skb) {
			rx_buffer->pagecnt_bias++;
			break;
		}
		if (IS_ERR(skb))
			ngbe_put_rx_buffer(rx_ring, rx_buffer, skb);
		cleaned_count++;

		/* place incomplete frames back on ring for completion */
		if (ngbe_is_non_eop(rx_ring, rx_desc, skb))
			continue;

		/* verify the packet layout is correct */
		if (ngbe_cleanup_headers(rx_ring, rx_desc, skb))
			continue;

		/* probably a little skewed due to removing CRC */
		total_rx_bytes += skb->len;

		/* populate checksum, timestamp, VLAN, and protocol */
		ngbe_process_skb_fields(rx_ring, rx_desc, skb);

		ngbe_rx_skb(q_vector, rx_ring, rx_desc, skb);

		/* update budget accounting */
		total_rx_packets++;
	} while (likely(total_rx_packets < budget));
	if (xdp_xmit & NGBE_XDP_TX) {
		struct ngbe_ring *ring =
			adapter->xdp_ring[rx_ring->queue_index % adapter->num_xdp_queues];
		if (static_branch_unlikely(&ngbe_xdp_locking_key))
			spin_lock(&ring->tx_lock);
		/* Force memory writes to complete before letting h/w know there
		 * are new descriptors to fetch.
		 */
		wmb();
		writel(ring->next_to_use, ring->tail);
		if (static_branch_unlikely(&ngbe_xdp_locking_key))
			spin_unlock(&ring->tx_lock);
	}
	if (xdp_xmit & NGBE_XDP_REDIR)
		xdp_do_flush();
	u64_stats_update_begin(&rx_ring->syncp);
	rx_ring->stats.packets += total_rx_packets;
	rx_ring->stats.bytes += total_rx_bytes;
	u64_stats_update_end(&rx_ring->syncp);
	q_vector->rx.total_packets += total_rx_packets;
	q_vector->rx.total_bytes += total_rx_bytes;

	return total_rx_packets;
}

/**
 * ngbe_configure_msix - Configure MSI-X hardware
 * @adapter: board private structure
 * ngbe_configure_msix sets up the hardware to properly generate MSI-X
 * interrupts.
 **/
static void ngbe_configure_msix(struct ngbe_adapter *adapter)
{
	u16 v_idx;
	u32 i;
	u32 eitrsel = 0;

	/* Populate MSIX to EITR Select */
	if (!(adapter->flags & NGBE_FLAG_VMDQ_ENABLED)) {
		wr32(&adapter->hw, NGBE_PX_ITRSEL, eitrsel);
	} else {
		for (i = 0; i < adapter->num_vfs; i++)
			eitrsel |= 1 << i;

		wr32(&adapter->hw, NGBE_PX_ITRSEL, eitrsel);
	}

	/* Populate the IVAR table and set the ITR values to the
	 * corresponding register.
	 */
	for (v_idx = 0; v_idx < adapter->num_q_vectors; v_idx++) {
		struct ngbe_q_vector *q_vector = adapter->q_vector[v_idx];
		struct ngbe_ring *ring;

		ngbe_for_each_ring(ring, q_vector->rx)
			ngbe_set_ivar(adapter, 0, ring->reg_idx, v_idx);

		ngbe_for_each_ring(ring, q_vector->tx)
			ngbe_set_ivar(adapter, 1, ring->reg_idx, v_idx);

		ngbe_write_eitr(q_vector);
	}

	/* misc ivar from seq 1 to seq 8 */
	if (adapter->flags2 & NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP)
		v_idx += adapter->ring_feature[RING_F_VMDQ].offset;

	ngbe_set_ivar(adapter, -1, 0, v_idx);
	wr32(&adapter->hw, NGBE_PX_ITR(v_idx), 1950);
}

enum latency_range { lowest_latency = 0, low_latency = 1, bulk_latency = 2, latency_invalid = 255 };

/**
 * ngbe_update_itr - update the dynamic ITR value based on statistics
 * @q_vector: structure containing interrupt and ring information
 * @ring_container: structure containing ring performance data
 *      Stores a new ITR value based on packets and byte
 *      counts during the last interrupt.  The advantage of per interrupt
 *      computation is faster updates and more accurate ITR for the current
 *      traffic pattern.  Constants in this function were computed
 *      based on theoretical maximum wire speed and thresholds were set based
 *      on testing data as well as attempting to minimize response time
 *      while increasing bulk throughput.
 *      this functionality is controlled by the InterruptThrottleRate module
 *      parameter (see ngbe_param.c)
 **/

static void ngbe_update_itr(struct ngbe_q_vector *q_vector,
			    struct ngbe_ring_container *ring_container)
{
	int bytes = ring_container->total_bytes;
	int packets = ring_container->total_packets;
	u32 timepassed_us;
	u64 bytes_perint;
	u8 itr_setting = ring_container->itr;

	if (packets == 0)
		return;

	/* simple throttlerate manangbeent
	 *   0-10MB/s   lowest (100000 ints/s)
	 *  10-20MB/s   low    (20000 ints/s)
	 *  20-1249MB/s bulk   (12000 ints/s)
	 */
	/* what was last interrupt timeslice? */
	timepassed_us = q_vector->itr >> 2;
	if (timepassed_us == 0)
		return;
	bytes_perint = bytes / timepassed_us; /* bytes/usec */

	switch (itr_setting) {
	case lowest_latency:
		if (bytes_perint > 10)
			itr_setting = low_latency;
		break;
	case low_latency:
		if (bytes_perint > 20)
			itr_setting = bulk_latency;
		else if (bytes_perint <= 10)
			itr_setting = lowest_latency;
		break;
	case bulk_latency:
		if (bytes_perint <= 20)
			itr_setting = low_latency;
		break;
	}

	/* clear work counters since we have the values we need */
	ring_container->total_bytes = 0;
	ring_container->total_packets = 0;

	/* write updated itr to ring container */
	ring_container->itr = itr_setting;
}

/**
 * ngbe_write_eitr - write EITR register in hardware specific way
 * @q_vector: structure containing interrupt and ring information
 * This function is made to be called by ethtool and by the driver
 * when it needs to update EITR registers at runtime.  Hardware
 * specific quirks/differences are taken care of here.
 */
void ngbe_write_eitr(struct ngbe_q_vector *q_vector)
{
	struct ngbe_adapter *adapter = q_vector->adapter;
	struct ngbe_hw *hw = &adapter->hw;
	int v_idx = q_vector->v_idx;
	u32 itr_reg = q_vector->itr & NGBE_MAX_EITR;

	itr_reg |= NGBE_PX_ITR_CNT_WDIS;

	wr32(hw, NGBE_PX_ITR(v_idx), itr_reg);
}

static void ngbe_set_itr(struct ngbe_q_vector *q_vector)
{
	u16 new_itr = q_vector->itr;
	u8 current_itr;

	ngbe_update_itr(q_vector, &q_vector->tx);
	ngbe_update_itr(q_vector, &q_vector->rx);

	current_itr = max(q_vector->rx.itr, q_vector->tx.itr);

	switch (current_itr) {
	/* counts and packets in update_itr are dependent on these numbers */
	case lowest_latency:
		new_itr = NGBE_70K_ITR;
		break;
	case low_latency:
		new_itr = NGBE_20K_ITR;
		break;
	case bulk_latency:
		new_itr = NGBE_7K_ITR;
		break;
	default:
		break;
	}

	if (new_itr != q_vector->itr) {
		/* do an exponential smoothing */
		new_itr = (20 * new_itr * q_vector->itr) / ((19 * new_itr) + q_vector->itr);

		/* save the algorithm value here */
		q_vector->itr = new_itr;
		ngbe_write_eitr(q_vector);
	}
}

/**
 * ngbe_check_overtemp_subtask - check for over temperature
 * @adapter: pointer to adapter
 **/
static void ngbe_check_overtemp_subtask(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	struct ngbe_phy_info *phy = &hw->phy;
	u32 eicr = adapter->interrupt_event;
	int temp_state;

	if (test_bit(__NGBE_DOWN, &adapter->state))
		return;
	if (!(adapter->flags2 & NGBE_FLAG2_TEMP_SENSOR_CAPABLE))
		return;
	if (!(adapter->flags2 & NGBE_FLAG2_TEMP_SENSOR_EVENT))
		return;

	adapter->flags2 &= ~NGBE_FLAG2_TEMP_SENSOR_EVENT;

	/* Since the warning interrupt is for both ports
	 * we don't have to check if:
	 *  - This interrupt wasn't for our port.
	 *  - We may have missed the interrupt so always have to
	 *    check if we  got a LSC
	 */
	if (!(eicr & NGBE_PX_MISC_IC_OVER_HEAT))
		return;

	temp_state = phy->ops.check_overtemp(hw);
	if (!temp_state || temp_state == NGBE_NOT_IMPLEMENTED)
		return;

	if (temp_state == NGBE_ERR_UNDERTEMP && test_bit(__NGBE_HANGING, &adapter->state)) {
		e_crit(drv, "%s\n", ngbe_underheat_msg);
		wr32m(&adapter->hw, NGBE_RDB_PB_CTL, NGBE_RDB_PB_CTL_PBEN, NGBE_RDB_PB_CTL_PBEN);
		netif_carrier_on(adapter->netdev);

		clear_bit(__NGBE_HANGING, &adapter->state);
	} else if (temp_state == NGBE_ERR_OVERTEMP &&
		   !test_and_set_bit(__NGBE_HANGING, &adapter->state)) {
		e_crit(drv, "%s\n", ngbe_overheat_msg);
		netif_carrier_off(adapter->netdev);

		wr32m(&adapter->hw, NGBE_RDB_PB_CTL, NGBE_RDB_PB_CTL_PBEN, 0);

		/* wait for tx ring clean */
		mdelay(20);
	}

	adapter->interrupt_event = 0;
}

static void ngbe_check_overtemp_event(struct ngbe_adapter *adapter, u32 eicr)
{
	if (!(adapter->flags2 & NGBE_FLAG2_TEMP_SENSOR_CAPABLE))
		return;

	if (!(eicr & NGBE_PX_MISC_IC_OVER_HEAT))
		return;
	if (!test_bit(__NGBE_DOWN, &adapter->state)) {
		adapter->interrupt_event = eicr;
		adapter->flags2 |= NGBE_FLAG2_TEMP_SENSOR_EVENT;
		ngbe_service_event_schedule(adapter);
	}
}

static void ngbe_handle_phy_event(struct ngbe_hw *hw)
{
	struct ngbe_adapter *adapter = hw->back;
	u32 reg;

	reg = rd32(hw, NGBE_GPIO_INTSTATUS);
	wr32(hw, NGBE_GPIO_EOI, reg);

	if (!(hw->phy.type == ngbe_phy_fpga))
		hw->phy.ops.check_event(hw);
	adapter->lsc_int++;
	adapter->link_check_timeout = jiffies;
	if (!test_bit(__NGBE_DOWN, &adapter->state))
		ngbe_service_event_schedule(adapter);
}

/**
 * ngbe_irq_enable - Enable default interrupt generation settings
 * @adapter: board private structure
 **/
void ngbe_irq_enable(struct ngbe_adapter *adapter, bool queues, bool flush)
{
	u32 mask = 0;
	struct ngbe_hw *hw = &adapter->hw;

	/* enable misc interrupt */
	mask = NGBE_PX_MISC_IEN_MASK;

	if (adapter->flags2 & NGBE_FLAG2_TEMP_SENSOR_CAPABLE)
		mask |= NGBE_PX_MISC_IEN_OVER_HEAT;

	mask |= NGBE_PX_MISC_IEN_TIMESYNC;

	if (hw->gpio_ctl == 1)
		wr32(&adapter->hw, NGBE_GPIO_DDR, 0x1);
	else
		wr32(&adapter->hw, NGBE_GPIO_DDR, 0x0);

	wr32(&adapter->hw, NGBE_GPIO_INTEN, 0x3);
	wr32(&adapter->hw, NGBE_GPIO_INTTYPE_LEVEL, 0x0);
	if (adapter->hw.phy.type == ngbe_phy_yt8521s_sfi ||
	    adapter->hw.phy.type == ngbe_phy_internal_yt8521s_sfi ||
	    adapter->hw.phy.type == ngbe_phy_external)
		wr32(&adapter->hw, NGBE_GPIO_POLARITY, 0x0);
	else
		wr32(&adapter->hw, NGBE_GPIO_POLARITY, 0x3);

	if (adapter->hw.phy.type == ngbe_phy_yt8521s_sfi ||
	    adapter->hw.phy.type == ngbe_phy_internal_yt8521s_sfi ||
	    adapter->hw.phy.type == ngbe_phy_external)
		mask |= NGBE_PX_MISC_IEN_GPIO;
	//		mask &= ~NGBE_PX_MISC_IEN_GPIO;

	wr32(hw, NGBE_PX_MISC_IEN, mask);

	/* unmask interrupt */
	if (queues) {
		ngbe_intr_enable(&adapter->hw, NGBE_INTR_ALL);
	} else {
		if (!(adapter->flags2 & NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP))
			ngbe_intr_enable(&adapter->hw, NGBE_INTR_MISC(adapter));
		else
			ngbe_intr_enable(&adapter->hw, NGBE_INTR_MISC_VMDQ(adapter));
	}

	/* flush configuration */
	if (flush)
		NGBE_WRITE_FLUSH(&adapter->hw);
}

static irqreturn_t ngbe_msix_other(int __always_unused irq, void *data)
{
	struct ngbe_adapter *adapter = data;
	struct ngbe_hw *hw = &adapter->hw;
	u32 eicr;
	u32 ecc;

	eicr = ngbe_misc_isb(adapter, NGBE_ISB_MISC);

	if (eicr & (NGBE_PX_MISC_IC_PHY | NGBE_PX_MISC_IC_GPIO))
		ngbe_handle_phy_event(hw);

	if (eicr & NGBE_PX_MISC_IC_VF_MBOX)
		ngbe_msg_task(adapter);

	if (eicr & NGBE_PX_MISC_IC_PCIE_REQ_ERR) {
		ERROR_REPORT1(NGBE_ERROR_POLLING, "lan id %d, PCIe request error founded.\n",
			      hw->bus.lan_id);
		if (hw->bus.lan_id == 0) {
			adapter->flags2 |= NGBE_FLAG2_PCIE_NEED_RECOVER;
			ngbe_service_event_schedule(adapter);
		} else {
			wr32(&adapter->hw, NGBE_MIS_PF_SM, 1);
		}
	}

	if (eicr & NGBE_PX_MISC_IC_INT_ERR) {
		e_info(link, "Received unrecoverable ECC Err, initiating reset.\n");
		ecc = rd32(hw, NGBE_MIS_ST);
		e_info(link, "ecc error status is 0x%08x\n", ecc);
		if (((ecc & NGBE_MIS_ST_LAN0_ECC) && hw->bus.lan_id == 0) ||
		    ((ecc & NGBE_MIS_ST_LAN1_ECC) && hw->bus.lan_id == 1) ||
		    ((ecc & NGBE_MIS_ST_LAN2_ECC) && hw->bus.lan_id == 2) ||
		    ((ecc & NGBE_MIS_ST_LAN3_ECC) && hw->bus.lan_id == 3)) {
			adapter->flags2 |=
				NGBE_FLAG2_DEV_RESET_REQUESTED | NGBE_FLAG2_ECC_ERR_RESET;
		}
		ngbe_service_event_schedule(adapter);
	}
	if (eicr & NGBE_PX_MISC_IC_DEV_RST) {
		int value = rd32(hw,
			NGBE_TSC_LSEC_PKTNUM1); // This reg is used by fw to tell drv not to drv rst
		if (!(value & 0x1)) {
			adapter->flags2 |= NGBE_FLAG2_RESET_INTR_RECEIVED;
			ngbe_service_event_schedule(adapter);
		}
	}
	if ((eicr & NGBE_PX_MISC_IC_STALL) || (eicr & NGBE_PX_MISC_IC_ETH_EVENT)) {
		adapter->flags2 |= NGBE_FLAG2_PF_RESET_REQUESTED;
		ngbe_service_event_schedule(adapter);
	}

	ngbe_check_overtemp_event(adapter, eicr);

	if (unlikely(eicr & NGBE_PX_MISC_IC_TIMESYNC))
		ngbe_ptp_check_pps_event(adapter);

	/* re-enable the original interrupt state, no lsc, no queues */
	if (!test_bit(__NGBE_DOWN, &adapter->state))
		ngbe_irq_enable(adapter, false, false);

	return IRQ_HANDLED;
}

static irqreturn_t ngbe_msix_clean_rings(int __always_unused irq, void *data)
{
	struct ngbe_q_vector *q_vector = data;

	/* EIAM disabled interrupts (on this vector) for us */

	if (q_vector->rx.ring || q_vector->tx.ring)
		napi_schedule_irqoff(&q_vector->napi);

	return IRQ_HANDLED;
}

/**
 * ngbe_poll - NAPI polling RX/TX cleanup routine
 * @napi: napi struct with our devices info in it
 * @budget: amount of work driver is allowed to do this pass, in packets
 * This function will clean all queues associated with a q_vector.
 **/
int ngbe_poll(struct napi_struct *napi, int budget)
{
	struct ngbe_q_vector *q_vector = container_of(napi, struct ngbe_q_vector, napi);
	struct ngbe_adapter *adapter = q_vector->adapter;
	struct ngbe_ring *ring;
	int per_ring_budget;
	bool clean_complete = true;

	ngbe_for_each_ring(ring, q_vector->tx) {
		bool wd = ring->xsk_pool ? ngbe_clean_xdp_tx_irq(q_vector, ring)
					 : ngbe_clean_tx_irq(q_vector, ring);
		if (!wd)
			clean_complete = false;
	}

	/* Exit if we are called by netpoll */
	if (budget <= 0)
		return budget;

	/* attempt to distribute budget to each queue fairly, but don't allow
	 * the budget to go below 1 because we'll exit polling
	 */
	if (q_vector->rx.count > 1)
		per_ring_budget = max(budget / q_vector->rx.count, 1);
	else
		per_ring_budget = budget;

	ngbe_for_each_ring(ring, q_vector->rx) {
		int cleaned = ring->xsk_pool ? ngbe_clean_rx_irq_zc(q_vector, ring, per_ring_budget)
					     : ngbe_clean_rx_irq(q_vector, ring, per_ring_budget);

		if (cleaned >= per_ring_budget)
			clean_complete = false;
	}

	/* If all work not completed, return budget and keep polling */
	if (!clean_complete)
		return budget;

	/* all work done, exit the polling mode */
	napi_complete(napi);

	if (adapter->rx_itr_setting == 1 || adapter->tx_itr_setting == 1)
		ngbe_set_itr(q_vector);

	if (!test_bit(__NGBE_DOWN, &adapter->state))
		ngbe_intr_enable(&adapter->hw, NGBE_INTR_Q(q_vector->v_idx));

	return 0;
}

/**
 * ngbe_request_msix_irqs - Initialize MSI-X interrupts
 * @adapter: board private structure
 * ngbe_request_msix_irqs allocates MSI-X vectors and requests
 * interrupts from the kernel.
 **/
static int ngbe_request_msix_irqs(struct ngbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int vector, err;
	int ri = 0, ti = 0;

	for (vector = 0; vector < adapter->num_q_vectors; vector++) {
		struct ngbe_q_vector *q_vector = adapter->q_vector[vector];
		struct msix_entry *entry = &adapter->msix_entries[vector];

		if (q_vector->tx.ring && q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name) - 1, "%s-TxRx-%d",
				 netdev->name, ri++);
			ti++;
		} else if (q_vector->rx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name) - 1, "%s-rx-%d",
				 netdev->name, ri++);
		} else if (q_vector->tx.ring) {
			snprintf(q_vector->name, sizeof(q_vector->name) - 1, "%s-tx-%d",
				 netdev->name, ti++);
		} else {
			/* skip this unused q_vector */
			continue;
		}
		err = request_irq(entry->vector, &ngbe_msix_clean_rings, 0, q_vector->name,
				  q_vector);
		if (err) {
			e_err(probe, "request_irq failed for MSIX interrupt '%s' Error: %d\n",
			      q_vector->name, err);
			goto free_queue_irqs;
		}
	}

	if (adapter->flags2 & NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP)
		vector += adapter->irq_remap_offset;

	err = request_irq(adapter->msix_entries[vector].vector, ngbe_msix_other, 0, netdev->name,
			  adapter);

	if (adapter->flags2 & NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP)
		vector -= adapter->irq_remap_offset;

	if (err) {
		e_err(probe, "request_irq for msix_other failed: %d\n", err);
		goto free_queue_irqs;
	}

	return 0;

free_queue_irqs:
	while (vector) {
		vector--;

		irq_set_affinity_hint(adapter->msix_entries[vector].vector, NULL);

		free_irq(adapter->msix_entries[vector].vector, adapter->q_vector[vector]);
	}
	adapter->flags &= ~NGBE_FLAG_MSIX_ENABLED;
	pci_disable_msix(adapter->pdev);
	kfree(adapter->msix_entries);
	adapter->msix_entries = NULL;
	return err;
}

/**
 * ngbe_intr - legacy mode Interrupt Handler
 * @irq: interrupt number
 * @data: pointer to a network interface device structure
 **/
static irqreturn_t ngbe_intr(int __always_unused irq, void *data)
{
	struct ngbe_adapter *adapter = data;
	struct ngbe_hw *hw = &adapter->hw;
	struct ngbe_q_vector *q_vector = adapter->q_vector[0];
	u32 eicr;
	u32 eicr_misc;
	u32 ecc = 0;

	eicr = ngbe_misc_isb(adapter, NGBE_ISB_VEC0);
	if (!eicr) {
		/* shared interrupt alert!
		 * the interrupt that we masked before the EICR read.
		 */
		if (!test_bit(__NGBE_DOWN, &adapter->state))
			ngbe_irq_enable(adapter, true, true);
		return IRQ_NONE; /* Not our interrupt */
	}
	adapter->isb_mem[NGBE_ISB_VEC0] = 0;
	if (!(adapter->flags & NGBE_FLAG_MSI_ENABLED))
		wr32(&adapter->hw, NGBE_PX_INTA, 1);

	eicr_misc = ngbe_misc_isb(adapter, NGBE_ISB_MISC);
	if (eicr_misc & (NGBE_PX_MISC_IC_PHY | NGBE_PX_MISC_IC_GPIO))
		ngbe_handle_phy_event(hw);

	if (eicr_misc & NGBE_PX_MISC_IC_INT_ERR) {
		e_info(link, "Received unrecoverable ECC Err, initiating reset.\n");
		ecc = rd32(hw, NGBE_MIS_ST);
		e_info(link, "ecc error status is 0x%08x\n", ecc);
		adapter->flags2 |= NGBE_FLAG2_DEV_RESET_REQUESTED | NGBE_FLAG2_ECC_ERR_RESET;
		ngbe_service_event_schedule(adapter);
	}

	if (eicr_misc & NGBE_PX_MISC_IC_DEV_RST) {
		int value = rd32(hw,
			NGBE_TSC_LSEC_PKTNUM1); // This reg is used by fw to tell drv not to drv rst
		if (!(value & 0x1)) {
			adapter->flags2 |= NGBE_FLAG2_RESET_INTR_RECEIVED;
			ngbe_service_event_schedule(adapter);
		}
	}
	ngbe_check_overtemp_event(adapter, eicr_misc);

	if (unlikely(eicr_misc & NGBE_PX_MISC_IC_TIMESYNC))
		ngbe_ptp_check_pps_event(adapter);
	adapter->isb_mem[NGBE_ISB_MISC] = 0;
	/* would disable interrupts here but it is auto disabled */
	napi_schedule_irqoff(&q_vector->napi);

	/* re-enable link(maybe) and non-queue interrupts, no flush.
	 * ngbe_poll will re-enable the queue interrupts
	 */
	if (!test_bit(__NGBE_DOWN, &adapter->state))
		ngbe_irq_enable(adapter, false, false);

	return IRQ_HANDLED;
}

/**
 * ngbe_request_irq - initialize interrupts
 * @adapter: board private structure
 * Attempts to configure interrupts using the best available
 * capabilities of the hardware and kernel.
 **/
static int ngbe_request_irq(struct ngbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	int err;

	if (adapter->flags & NGBE_FLAG_MSIX_ENABLED)
		err = ngbe_request_msix_irqs(adapter);
	else if (adapter->flags & NGBE_FLAG_MSI_ENABLED)
		err = request_irq(adapter->pdev->irq, &ngbe_intr, 0, netdev->name, adapter);
	else
		err = request_irq(adapter->pdev->irq, &ngbe_intr, IRQF_SHARED, netdev->name,
				  adapter);

	if (err)
		e_err(probe, "request_irq failed, Error %d\n", err);

	return err;
}

static void ngbe_free_irq(struct ngbe_adapter *adapter)
{
	int vector;

	if (!(adapter->flags & NGBE_FLAG_MSIX_ENABLED)) {
		free_irq(adapter->pdev->irq, adapter);
		return;
	}

	for (vector = 0; vector < adapter->num_q_vectors; vector++) {
		struct ngbe_q_vector *q_vector = adapter->q_vector[vector];
		struct msix_entry *entry = &adapter->msix_entries[vector];

		/* free only the irqs that were actually requested */
		if (!q_vector->rx.ring && !q_vector->tx.ring)
			continue;

		/* clear the affinity_mask in the IRQ descriptor */
		irq_set_affinity_hint(entry->vector, NULL);

		free_irq(entry->vector, q_vector);
	}

	if (adapter->flags2 & NGBE_FLAG2_SRIOV_MISC_IRQ_REMAP)
		free_irq(adapter->msix_entries[vector + adapter->irq_remap_offset].vector, adapter);
	else
		free_irq(adapter->msix_entries[vector++].vector, adapter);
}

/**
 * ngbe_irq_disable - Mask off interrupt generation on the NIC
 * @adapter: board private structure
 **/
void ngbe_irq_disable(struct ngbe_adapter *adapter)
{
	wr32(&adapter->hw, NGBE_PX_MISC_IEN, 0);
	ngbe_intr_disable(&adapter->hw, NGBE_INTR_ALL);

	NGBE_WRITE_FLUSH(&adapter->hw);
	if (adapter->flags & NGBE_FLAG_MSIX_ENABLED) {
		int vector;

		for (vector = 0; vector < adapter->num_q_vectors; vector++)
			synchronize_irq(adapter->msix_entries[vector].vector);

		synchronize_irq(adapter->msix_entries[vector++].vector);
	} else {
		synchronize_irq(adapter->pdev->irq);
	}
}

/**
 * ngbe_configure_msi_and_legacy - Initialize PIN (INTA...) and MSI interrupts
 **/
static void ngbe_configure_msi_and_legacy(struct ngbe_adapter *adapter)
{
	struct ngbe_q_vector *q_vector = adapter->q_vector[0];
	struct ngbe_ring *ring;

	ngbe_write_eitr(q_vector);

	ngbe_for_each_ring(ring, q_vector->rx) ngbe_set_ivar(adapter, 0, ring->reg_idx, 0);

	ngbe_for_each_ring(ring, q_vector->tx) ngbe_set_ivar(adapter, 1, ring->reg_idx, 0);

	ngbe_set_ivar(adapter, -1, 0, 1);

	e_info(hw, "Legacy interrupt IVAR setup done\n");
}

/**
 * ngbe_configure_tx_ring - Configure Tx ring after Reset
 * @adapter: board private structure
 * @ring: structure containing ring specific data
 * Configure the Tx descriptor ring after a reset.
 **/
void ngbe_configure_tx_ring(struct ngbe_adapter *adapter, struct ngbe_ring *ring)
{
	struct ngbe_hw *hw = &adapter->hw;
	u64 tdba = ring->dma;
	int wait_loop = 10;
	u32 txdctl = NGBE_PX_TR_CFG_ENABLE;
	u8 reg_idx = ring->reg_idx;

	ring->xsk_pool = NULL;
	if (ring_is_xdp(ring))
		ring->xsk_pool = ngbe_xsk_umem(adapter, ring);

	/* disable queue to avoid issues while updating state */
	wr32(hw, NGBE_PX_TR_CFG(reg_idx), NGBE_PX_TR_CFG_SWFLSH);
	NGBE_WRITE_FLUSH(hw);

	wr32(hw, NGBE_PX_TR_BAL(reg_idx), tdba & DMA_BIT_MASK(32));
	wr32(hw, NGBE_PX_TR_BAH(reg_idx), tdba >> 32);

	/* reset head and tail pointers */
	wr32(hw, NGBE_PX_TR_RP(reg_idx), 0);
	wr32(hw, NGBE_PX_TR_WP(reg_idx), 0);
	ring->tail = adapter->io_addr + NGBE_PX_TR_WP(reg_idx);

	/* reset ntu and ntc to place SW in sync with hardwdare */
	ring->next_to_clean = 0;
	ring->next_to_use = 0;
	ring->next_to_free = 0;

	txdctl |= NGBE_RING_SIZE(ring) << NGBE_PX_TR_CFG_TR_SIZE_SHIFT;

	/* set WTHRESH to encourage burst writeback, it should not be set
	 * higher than 1 when:
	 * - ITR is 0 as it could cause false TX hangs
	 * - ITR is set to > 100k int/sec and BQL is enabled
	 *
	 * In order to avoid issues WTHRESH + PTHRESH should always be equal
	 * to or less than the number of on chip descriptors, which is
	 * currently 40.
	 */
	txdctl |= 0x20 << NGBE_PX_TR_CFG_WTHRESH_SHIFT;

	clear_bit(__NGBE_HANG_CHECK_ARMED, &ring->state);

	/* enable queue */
	wr32(hw, NGBE_PX_TR_CFG(reg_idx), txdctl);

	/* poll to verify queue is enabled */
	do {
		msleep(20);
		txdctl = rd32(hw, NGBE_PX_TR_CFG(reg_idx));
	} while (--wait_loop && !(txdctl & NGBE_PX_TR_CFG_ENABLE));
	if (!wait_loop)
		e_err(drv, "Could not enable Tx Queue %d\n", reg_idx);
}

/**
 * ngbe_configure_tx - Configure Transmit Unit after Reset
 * @adapter: board private structure
 * Configure the Tx unit of the MAC after a reset.
 **/
static void ngbe_configure_tx(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 i;

	/* TDM_CTL.TE must be before Tx queues are enabled */
	wr32m(hw, NGBE_TDM_CTL, NGBE_TDM_CTL_TE, NGBE_TDM_CTL_TE);

	/* Setup the HW Tx Head and Tail descriptor pointers */
	for (i = 0; i < adapter->num_tx_queues; i++)
		ngbe_configure_tx_ring(adapter, adapter->tx_ring[i]);
	for (i = 0; i < adapter->num_xdp_queues; i++)
		ngbe_configure_tx_ring(adapter, adapter->xdp_ring[i]);

	wr32m(hw, NGBE_TSEC_BUF_AE, 0x3FF, 0x10);
	wr32m(hw, NGBE_TSEC_CTL, 0x2, 0);

	wr32m(hw, NGBE_TSEC_CTL, 0x1, 1);

	/* enable mac transmitter */
	wr32m(hw, NGBE_MAC_TX_CFG, NGBE_MAC_TX_CFG_TE, NGBE_MAC_TX_CFG_TE);
}

static void ngbe_enable_rx_drop(struct ngbe_adapter *adapter, struct ngbe_ring *ring)
{
	struct ngbe_hw *hw = &adapter->hw;
	u16 reg_idx = ring->reg_idx;

	u32 srrctl = rd32(hw, NGBE_PX_RR_CFG(reg_idx));

	srrctl |= NGBE_PX_RR_CFG_DROP_EN;

	wr32(hw, NGBE_PX_RR_CFG(reg_idx), srrctl);
}

static void ngbe_disable_rx_drop(struct ngbe_adapter *adapter, struct ngbe_ring *ring)
{
	struct ngbe_hw *hw = &adapter->hw;
	u16 reg_idx = ring->reg_idx;

	u32 srrctl = rd32(hw, NGBE_PX_RR_CFG(reg_idx));

	srrctl &= ~NGBE_PX_RR_CFG_DROP_EN;

	wr32(hw, NGBE_PX_RR_CFG(reg_idx), srrctl);
}

void ngbe_set_rx_drop_en(struct ngbe_adapter *adapter)
{
	int i;

	/* We should set the drop enable bit if:
	 *  SR-IOV is enabled
	 *   or
	 *  Number of Rx queues > 1 and flow control is disabled
	 *
	 *  This allows us to avoid head of line blocking for security
	 *  and performance reasons.
	 */
	if (adapter->num_vfs ||
	    (adapter->num_rx_queues > 1 && !(adapter->hw.fc.current_mode & ngbe_fc_tx_pause))) {
		for (i = 0; i < adapter->num_rx_queues; i++)
			ngbe_enable_rx_drop(adapter, adapter->rx_ring[i]);
	} else {
		for (i = 0; i < adapter->num_rx_queues; i++)
			ngbe_disable_rx_drop(adapter, adapter->rx_ring[i]);
	}
}

static void ngbe_configure_srrctl(struct ngbe_adapter *adapter, struct ngbe_ring *rx_ring)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 srrctl;
	u16 reg_idx = rx_ring->reg_idx;

	srrctl = rd32m(hw, NGBE_PX_RR_CFG(reg_idx),
		       ~(NGBE_PX_RR_CFG_RR_HDR_SZ | NGBE_PX_RR_CFG_RR_BUF_SZ |
		       NGBE_PX_RR_CFG_SPLIT_MODE));

	/* configure header buffer length, needed for RSC */
	srrctl |= NGBE_RX_HDR_SIZE << NGBE_PX_RR_CFG_BSIZEHDRSIZE_SHIFT;

	/* configure the packet buffer length */
	srrctl |= ngbe_rx_bufsz(rx_ring) >> NGBE_PX_RR_CFG_BSIZEPKT_SHIFT;
	if (ring_is_hs_enabled(rx_ring))
		srrctl |= NGBE_PX_RR_CFG_SPLIT_MODE;

	wr32(hw, NGBE_PX_RR_CFG(reg_idx), srrctl);
}

/**
 * Return a number of entries in the RSS indirection table
 * @adapter: device handle
 */
u32 ngbe_rss_indir_tbl_entries(struct ngbe_adapter *adapter)
{
	if (adapter->flags & NGBE_FLAG_SRIOV_ENABLED)
		return 64;
	else
		return 128;
}

/**
 * Write the RETA table to HW
 * @adapter: device handle
 * Write the RSS redirection table stored in adapter.rss_indir_tbl[] to HW.
 */
void ngbe_store_reta(struct ngbe_adapter *adapter)
{
	u32 i, reta_entries = ngbe_rss_indir_tbl_entries(adapter);
	struct ngbe_hw *hw = &adapter->hw;
	u32 reta = 0;
	u8 *indir_tbl = adapter->rss_indir_tbl;

	/* Fill out the redirection table as follows:
	 *  - 8 bit wide entries containing 4 bit RSS index
	 */

	/* Write redirection table to HW */
	for (i = 0; i < reta_entries; i++) {
		reta |= indir_tbl[i] << (i & 0x3) * 8;
		if ((i & 3) == 3) {
			wr32(hw, NGBE_RDB_RSSTBL(i >> 2), reta);
			reta = 0;
		}
	}
}

static void ngbe_setup_reta(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 i, j;
	u32 reta_entries = ngbe_rss_indir_tbl_entries(adapter);
	u16 rss_i = adapter->ring_feature[RING_F_RSS].indices;

	/* Program table for at least 2 queues w/ SR-IOV so that VFs can
	 * make full use of any rings they may have.  We will use the
	 * PSRTYPE register to control how many rings we use within the PF.
	 */
	if (adapter->flags & NGBE_FLAG_SRIOV_ENABLED && rss_i < 2)
		rss_i = 1;

	/* Fill out hash function seeds */
	for (i = 0; i < 10; i++)
		wr32(hw, NGBE_RDB_RSSRK(i), adapter->rss_key[i]);

	/* Fill out redirection table */
	memset(adapter->rss_indir_tbl, 0, sizeof(adapter->rss_indir_tbl));

	for (i = 0, j = 0; i < reta_entries; i++, j++) {
		if (j == rss_i)
			j = 0;

		adapter->rss_indir_tbl[i] = j;
	}

	ngbe_store_reta(adapter);
}

static void ngbe_setup_mrqc(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 rss_field = 0;

	/* VT, and RSS do not coexist at the same time */
	if (adapter->flags & NGBE_FLAG_VMDQ_ENABLED) {
		//		pr_info("ngbe_setup_mrqc not process\n");
		return;
	}
	/* Disable indicating checksum in descriptor, enables RSS hash */
	wr32m(hw, NGBE_PSR_CTL, NGBE_PSR_CTL_PCSD, NGBE_PSR_CTL_PCSD);

	/* Perform hash on these packet types */
	rss_field = NGBE_RDB_RA_CTL_RSS_IPV4 | NGBE_RDB_RA_CTL_RSS_IPV4_TCP |
		    NGBE_RDB_RA_CTL_RSS_IPV6 | NGBE_RDB_RA_CTL_RSS_IPV6_TCP;

	if (adapter->flags2 & NGBE_FLAG2_RSS_FIELD_IPV4_UDP)
		rss_field |= NGBE_RDB_RA_CTL_RSS_IPV4_UDP;
	if (adapter->flags2 & NGBE_FLAG2_RSS_FIELD_IPV6_UDP)
		rss_field |= NGBE_RDB_RA_CTL_RSS_IPV6_UDP;

	netdev_rss_key_fill(adapter->rss_key, sizeof(adapter->rss_key));

	ngbe_setup_reta(adapter);

	if (adapter->flags2 & NGBE_FLAG2_RSS_ENABLED)
		rss_field |= NGBE_RDB_RA_CTL_RSS_EN;
	wr32(hw, NGBE_RDB_RA_CTL, rss_field);
}

static void ngbe_rx_desc_queue_enable(struct ngbe_adapter *adapter, struct ngbe_ring *ring)
{
	struct ngbe_hw *hw = &adapter->hw;
	int wait_loop = NGBE_MAX_RX_DESC_POLL;
	u32 rxdctl;
	u8 reg_idx = ring->reg_idx;

	if (NGBE_REMOVED(hw->hw_addr))
		return;

	do {
		msleep(20);
		rxdctl = rd32(hw, NGBE_PX_RR_CFG(reg_idx));
	} while (--wait_loop && !(rxdctl & NGBE_PX_RR_CFG_RR_EN));

	if (!wait_loop) {
		e_err(drv, "RXDCTL.ENABLE on Rx queue %d not set within the polling period\n",
		      reg_idx);
	}
}

/* disable the specified rx ring/queue */
void ngbe_disable_rx_queue(struct ngbe_adapter *adapter, struct ngbe_ring *ring)
{
	struct ngbe_hw *hw = &adapter->hw;
	int wait_loop = NGBE_MAX_RX_DESC_POLL;
	u32 rxdctl;
	u8 reg_idx = ring->reg_idx;

	if (NGBE_REMOVED(hw->hw_addr))
		return;

	/* write value back with RXDCTL.ENABLE bit cleared */
	wr32m(hw, NGBE_PX_RR_CFG(reg_idx), NGBE_PX_RR_CFG_RR_EN, 0);

	/* hardware may take up to 100us to actually disable rx queue */
	do {
		usleep_range(10, 20);
		rxdctl = rd32(hw, NGBE_PX_RR_CFG(reg_idx));
	} while (--wait_loop && (rxdctl & NGBE_PX_RR_CFG_RR_EN));

	if (!wait_loop) {
		e_err(drv, "RXDCTL.ENABLE on Rx queue %d not cleared within the polling period\n",
		      reg_idx);
	}
}

void ngbe_configure_rx_ring(struct ngbe_adapter *adapter, struct ngbe_ring *ring)
{
	struct ngbe_hw *hw = &adapter->hw;
	u64 rdba = ring->dma;
	u32 rxdctl;
	u16 reg_idx = ring->reg_idx;

	/* disable queue to avoid issues while updating state */
	rxdctl = rd32(hw, NGBE_PX_RR_CFG(reg_idx));
	ngbe_disable_rx_queue(adapter, ring);

	if (ring->q_vector)
		xdp_rxq_info_unreg_mem_model(&ring->xdp_rxq);
	ring->xsk_pool = ngbe_xsk_umem(adapter, ring);
	if (ring->xsk_pool) {
		WARN_ON(xdp_rxq_info_reg_mem_model(&ring->xdp_rxq, MEM_TYPE_XSK_BUFF_POOL, NULL));
		xsk_pool_set_rxq_info(ring->xsk_pool, &ring->xdp_rxq);
	} else {
		if (ring->q_vector)
			WARN_ON(xdp_rxq_info_reg_mem_model(&ring->xdp_rxq, MEM_TYPE_PAGE_SHARED,
							   NULL));
	}

	wr32(hw, NGBE_PX_RR_BAL(reg_idx), rdba & DMA_BIT_MASK(32));
	wr32(hw, NGBE_PX_RR_BAH(reg_idx), rdba >> 32);

	if (ring->count == NGBE_MAX_RXD)
		rxdctl |= 0 << NGBE_PX_RR_CFG_RR_SIZE_SHIFT;
	else
		rxdctl |= (ring->count / 128) << NGBE_PX_RR_CFG_RR_SIZE_SHIFT;

	rxdctl |= 0x1 << NGBE_PX_RR_CFG_RR_THER_SHIFT;
	wr32(hw, NGBE_PX_RR_CFG(reg_idx), rxdctl);

	/* reset head and tail pointers */
	wr32(hw, NGBE_PX_RR_RP(reg_idx), 0);
	wr32(hw, NGBE_PX_RR_WP(reg_idx), 0);
	ring->tail = adapter->io_addr + NGBE_PX_RR_WP(reg_idx);

	/* reset ntu and ntc to place SW in sync with hardwdare */
	ring->next_to_clean = 0;
	ring->next_to_use = 0;
	ring->next_to_alloc = 0;

	ngbe_configure_srrctl(adapter, ring);

	/* enable receive descriptor ring */
	wr32m(hw, NGBE_PX_RR_CFG(reg_idx), NGBE_PX_RR_CFG_RR_EN, NGBE_PX_RR_CFG_RR_EN);

	ngbe_rx_desc_queue_enable(adapter, ring);
	if (ring->xsk_pool)
		ngbe_alloc_rx_buffers_zc(ring, ngbe_desc_unused(ring));
	else
		ngbe_alloc_rx_buffers(ring, ngbe_desc_unused(ring));
}

static void ngbe_setup_psrtype(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	int pool;

	/* PSRTYPE must be initialized in adapters */
	u32 psrtype = NGBE_RDB_PL_CFG_L4HDR | NGBE_RDB_PL_CFG_L3HDR | NGBE_RDB_PL_CFG_L2HDR |
		      NGBE_RDB_PL_CFG_TUN_OUTER_L2HDR | NGBE_RDB_PL_CFG_TUN_TUNHDR;

	for_each_set_bit(pool, &adapter->fwd_bitmask, NGBE_MAX_MACVLANS) {
		wr32(hw, NGBE_RDB_PL_CFG(VMDQ_P(pool)), psrtype);
	}
}

/**
 * ngbe_configure_bridge_mode - common settings for configuring bridge mode
 * @adapter - the private structure
 * This function's purpose is to remove code duplication and configure some
 * settings require to switch bridge modes.
 **/
static void ngbe_configure_bridge_mode(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;

	if (adapter->flags & NGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE) {
		/* disable Tx loopback, rely on switch hairpin mode */
		wr32m(hw, NGBE_PSR_CTL, NGBE_PSR_CTL_SW_EN, 0);
	} else {
		/* enable Tx loopback for internal VF/PF communication */
		wr32m(hw, NGBE_PSR_CTL, NGBE_PSR_CTL_SW_EN, NGBE_PSR_CTL_SW_EN);
	}
}

static void ngbe_configure_virtualization(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 i;
	u8 vfe = 0;

	if (!(adapter->flags & NGBE_FLAG_VMDQ_ENABLED))
		return;

	wr32m(hw, NGBE_PSR_VM_CTL, NGBE_PSR_VM_CTL_POOL_MASK | NGBE_PSR_VM_CTL_REPLEN,
	      VMDQ_P(0) << NGBE_PSR_VM_CTL_POOL_SHIFT | NGBE_PSR_VM_CTL_REPLEN);

	for_each_set_bit(i, &adapter->fwd_bitmask, NGBE_MAX_MACVLANS) {
		/* accept untagged packets until a vlan tag is
		 * specifically set for the VMDQ queue/pool
		 */
		wr32m(hw, NGBE_PSR_VM_L2CTL(i), NGBE_PSR_VM_L2CTL_AUPE, NGBE_PSR_VM_L2CTL_AUPE);
	}

	vfe = 1 << (VMDQ_P(0));
	/* Enable only the PF pools for Tx/Rx */
	wr32(hw, NGBE_RDM_POOL_RE, vfe);
	wr32(hw, NGBE_TDM_POOL_TE, vfe);

	if (!(adapter->flags & NGBE_FLAG_SRIOV_ENABLED))
		return;

	/* configure default bridge settings */
	ngbe_configure_bridge_mode(adapter);

	/* Ensure LLDP and FC is set for Ethertype Antispoofing if we will be
	 * calling set_ethertype_anti_spoofing for each VF in loop below.
	 */
	if (hw->mac.ops.set_ethertype_anti_spoofing) {
		wr32(hw, NGBE_PSR_ETYPE_SWC(NGBE_PSR_ETYPE_SWC_FILTER_LLDP),
		     (NGBE_PSR_ETYPE_SWC_FILTER_EN | /* enable filter */
		      NGBE_PSR_ETYPE_SWC_TX_ANTISPOOF |
		      NGBE_ETH_P_LLDP)); /* LLDP eth procotol type */

		wr32(hw, NGBE_PSR_ETYPE_SWC(NGBE_PSR_ETYPE_SWC_FILTER_FC),
		     (NGBE_PSR_ETYPE_SWC_FILTER_EN | NGBE_PSR_ETYPE_SWC_TX_ANTISPOOF |
		      ETH_P_PAUSE));
	}

	for (i = 0; i < adapter->num_vfs; i++) {
		if (!adapter->vfinfo[i].spoofchk_enabled)
			ngbe_ndo_set_vf_spoofchk(adapter->netdev, i, false);
		/* enable ethertype anti spoofing if hw supports it */
		hw->mac.ops.set_ethertype_anti_spoofing(hw, true, i);
	}
}

static void ngbe_set_rx_buffer_len(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	u32 max_frame = netdev->mtu + ETH_HLEN + ETH_FCS_LEN;
	struct ngbe_ring *rx_ring;
	int i;
	u32 mhadd;

	/* adjust max frame to be at least the size of a standard frame */
	if (max_frame < (ETH_FRAME_LEN + ETH_FCS_LEN))
		max_frame = (ETH_FRAME_LEN + ETH_FCS_LEN);

	mhadd = rd32(hw, NGBE_PSR_MAX_SZ);
	if (max_frame != mhadd)
		wr32(hw, NGBE_PSR_MAX_SZ, max_frame);

	/* Setup the HW Rx Head and Tail Descriptor Pointers and
	 * the Base and Length of the Rx Descriptor Ring
	 */
	for (i = 0; i < adapter->num_rx_queues; i++) {
		rx_ring = adapter->rx_ring[i];
		clear_bit(__NGBE_RX_3K_BUFFER, &rx_ring->state);
		if (adapter->flags & NGBE_FLAG_RX_HS_ENABLED) {
			rx_ring->rx_buf_len = NGBE_RX_HDR_SIZE;
			set_ring_hs_enabled(rx_ring);
		} else {
			clear_ring_hs_enabled(rx_ring);
		}
#if (PAGE_SIZE < 8192)
		if (adapter->xdp_prog)
			if (NGBE_2K_TOO_SMALL_WITH_PADDING ||
			    (max_frame > (ETH_FRAME_LEN + ETH_FCS_LEN)))
				set_bit(__NGBE_RX_3K_BUFFER, &rx_ring->state);
#endif
	}
}

/**
 * ngbe_configure_rx - Configure Receive Unit after Reset
 * @adapter: board private structure
 * Configure the Rx unit of the MAC after a reset.
 **/
static void ngbe_configure_rx(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	int i;
	u32 rxctrl;

	/* disable receives while setting up the descriptors */
	hw->mac.ops.disable_rx(hw);

	ngbe_setup_psrtype(adapter);

	/* enable hw crc stripping */
	wr32m(hw, NGBE_RSEC_CTL, NGBE_RSEC_CTL_CRC_STRIP, NGBE_RSEC_CTL_CRC_STRIP);

	/* Program registers for the distribution of queues */
	ngbe_setup_mrqc(adapter);

	/* set_rx_buffer_len must be called before ring initialization */
	ngbe_set_rx_buffer_len(adapter);

	/* Setup the HW Rx Head and Tail Descriptor Pointers and
	 * the Base and Length of the Rx Descriptor Ring
	 */
	for (i = 0; i < adapter->num_rx_queues; i++)
		ngbe_configure_rx_ring(adapter, adapter->rx_ring[i]);

	rxctrl = rd32(hw, NGBE_RDB_PB_CTL);

	/* enable all receives */
	rxctrl |= NGBE_RDB_PB_CTL_PBEN;
	hw->mac.ops.enable_rx_dma(hw, rxctrl);
}

static int ngbe_vlan_rx_add_vid(struct net_device *netdev, __always_unused __be16 proto, u16 vid)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	int pool_ndx = VMDQ_P(0);

	/* add VID to filter table */
	if (hw->mac.ops.set_vfta) {
		if (vid < VLAN_N_VID)
			set_bit(vid, adapter->active_vlans);
		hw->mac.ops.set_vfta(hw, vid, pool_ndx, true);
		if (adapter->flags & NGBE_FLAG_VMDQ_ENABLED) {
			int i;
			/* enable vlan id for all pools */
			for_each_set_bit(i, &adapter->fwd_bitmask, NGBE_MAX_MACVLANS)
				hw->mac.ops.set_vfta(hw, vid, VMDQ_P(i), true);
		}
	}
	return 0;
}

static int ngbe_vlan_rx_kill_vid(struct net_device *netdev, __always_unused __be16 proto, u16 vid)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	int pool_ndx = VMDQ_P(0);

	/* User is not allowed to remove vlan ID 0 */
	if (!vid)
		return 0;

	/* remove VID from filter table */
	if (hw->mac.ops.set_vfta) {
		hw->mac.ops.set_vfta(hw, vid, pool_ndx, false);
		if (adapter->flags & NGBE_FLAG_VMDQ_ENABLED) {
			int i;
			/* remove vlan id from all pools */
			for_each_set_bit(i, &adapter->fwd_bitmask, NGBE_MAX_MACVLANS)
				hw->mac.ops.set_vfta(hw, vid, VMDQ_P(i), false);
		}
	}
	clear_bit(vid, adapter->active_vlans);
	return 0;
}

/**
 * ngbe_vlan_strip_disable - helper to disable vlan tag stripping
 * @adapter: driver data
 */
void ngbe_vlan_strip_disable(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	int i, j;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct ngbe_ring *ring = adapter->rx_ring[i];

		if (ring->accel)
			continue;
		j = ring->reg_idx;
		wr32m(hw, NGBE_PX_RR_CFG(j), NGBE_PX_RR_CFG_VLAN, 0);
	}
}

/**
 * ngbe_vlan_strip_enable - helper to enable vlan tag stripping
 * @adapter: driver data
 */
void ngbe_vlan_strip_enable(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	int i, j;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct ngbe_ring *ring = adapter->rx_ring[i];

		if (ring->accel)
			continue;
		j = ring->reg_idx;
		wr32m(hw, NGBE_PX_RR_CFG(j), NGBE_PX_RR_CFG_VLAN, NGBE_PX_RR_CFG_VLAN);
	}
}

void ngbe_vlan_mode(struct net_device *netdev, u32 features)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	bool enable;

	enable = !!(features & (NETIF_F_HW_VLAN_CTAG_RX | NETIF_F_HW_VLAN_STAG_RX));
	if (enable)
		/* enable VLAN tag insert/strip */
		ngbe_vlan_strip_enable(adapter);
	else
		/* disable VLAN tag insert/strip */
		ngbe_vlan_strip_disable(adapter);
}

static void ngbe_restore_vlan(struct ngbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	u16 vid;

	ngbe_vlan_mode(netdev, netdev->features);

	for_each_set_bit(vid, adapter->active_vlans, VLAN_N_VID)
		ngbe_vlan_rx_add_vid(netdev, htons(ETH_P_8021Q), vid);
}

static u8 *ngbe_addr_list_itr(struct ngbe_hw __maybe_unused *hw, u8 **mc_addr_ptr, u32 *vmdq)
{
	struct netdev_hw_addr *mc_ptr;
#ifdef CONFIG_PCI_IOV
	struct ngbe_adapter *adapter = hw->back;
#endif /* CONFIG_PCI_IOV */
	u8 *addr = *mc_addr_ptr;

	/* VMDQ_P implicitely uses the adapter struct when CONFIG_PCI_IOV is
	 * defined, so we have to wrap the pointer above correctly to prevent
	 * a warning.
	 */
	*vmdq = VMDQ_P(0);

	mc_ptr = container_of(addr, struct netdev_hw_addr, addr[0]);
	if (mc_ptr->list.next) {
		struct netdev_hw_addr *ha;

		ha = list_entry(mc_ptr->list.next, struct netdev_hw_addr, list);
		*mc_addr_ptr = ha->addr;
	} else {
		*mc_addr_ptr = NULL;
	}

	return addr;
}

/**
 * ngbe_write_mc_addr_list - write multicast addresses to MTA
 * @netdev: network interface device structure
 * Writes multicast address list to the MTA hash table.
 * Returns: -ENOMEM on failure
 *                0 on no addresses written
 *                X on writing X addresses to MTA
 **/
int ngbe_write_mc_addr_list(struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	struct netdev_hw_addr *ha;
	u8 *addr_list = NULL;
	int addr_count = 0;
	bool clear = false;

	if (!hw->mac.ops.update_mc_addr_list)
		return -ENOMEM;

	if (!netif_running(netdev))
		return 0;

#ifdef CONFIG_PCI_IOV
	ngbe_restore_vf_multicasts(adapter);
#else
	clear = true;
#endif

	if (netdev_mc_empty(netdev)) {
		hw->mac.ops.update_mc_addr_list(hw, NULL, 0, ngbe_addr_list_itr, clear);
	} else {
		ha = list_first_entry(&netdev->mc.list, struct netdev_hw_addr, list);
		addr_list = ha->addr;
		addr_count = netdev_mc_count(netdev);

		hw->mac.ops.update_mc_addr_list(hw, addr_list, addr_count, ngbe_addr_list_itr,
						clear);
	}

	return addr_count;
}

void ngbe_full_sync_mac_table(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	int i;

	for (i = 0; i < hw->mac.num_rar_entries; i++) {
		if (adapter->mac_table[i].state & NGBE_MAC_STATE_IN_USE) {
			hw->mac.ops.set_rar(hw, i, adapter->mac_table[i].addr,
					    adapter->mac_table[i].pools, NGBE_PSR_MAC_SWC_AD_H_AV);
		} else {
			hw->mac.ops.clear_rar(hw, i);
		}
		adapter->mac_table[i].state &= ~(NGBE_MAC_STATE_MODIFIED);
	}
}

static void ngbe_sync_mac_table(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	int i;

	for (i = 0; i < hw->mac.num_rar_entries; i++) {
		if (adapter->mac_table[i].state & NGBE_MAC_STATE_MODIFIED) {
			if (adapter->mac_table[i].state & NGBE_MAC_STATE_IN_USE) {
				hw->mac.ops.set_rar(hw, i, adapter->mac_table[i].addr,
						    adapter->mac_table[i].pools,
						    NGBE_PSR_MAC_SWC_AD_H_AV);
			} else {
				hw->mac.ops.clear_rar(hw, i);
			}
			adapter->mac_table[i].state &= ~(NGBE_MAC_STATE_MODIFIED);
		}
	}
}

int ngbe_available_rars(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 i, count = 0;

	for (i = 0; i < hw->mac.num_rar_entries; i++) {
		if (adapter->mac_table[i].state == 0)
			count++;
	}
	return count;
}

/* this function destroys the first RAR entry */
static void ngbe_mac_set_default_filter(struct ngbe_adapter *adapter, u8 *addr)
{
	struct ngbe_hw *hw = &adapter->hw;

	memcpy(&adapter->mac_table[0].addr, addr, ETH_ALEN);
	adapter->mac_table[0].pools = 1ULL << VMDQ_P(0);
	adapter->mac_table[0].state = (NGBE_MAC_STATE_DEFAULT | NGBE_MAC_STATE_IN_USE);
	hw->mac.ops.set_rar(hw, 0, adapter->mac_table[0].addr, adapter->mac_table[0].pools,
			    NGBE_PSR_MAC_SWC_AD_H_AV);
}

int ngbe_add_mac_filter(struct ngbe_adapter *adapter, const u8 *addr, u16 pool)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 i;

	if (is_zero_ether_addr(addr))
		return -EINVAL;

	for (i = 0; i < hw->mac.num_rar_entries; i++) {
		if (adapter->mac_table[i].state & NGBE_MAC_STATE_IN_USE) {
			if (ether_addr_equal(addr, adapter->mac_table[i].addr)) {
				if (adapter->mac_table[i].pools != (1ULL << pool)) {
					adapter->mac_table[i].pools |= (1ULL << pool);
					adapter->mac_table[i].state |= NGBE_MAC_STATE_MODIFIED;
					ngbe_sync_mac_table(adapter);
					return i;
				}
			}
		}
	}

	for (i = 0; i < hw->mac.num_rar_entries; i++) {
		if (adapter->mac_table[i].state & NGBE_MAC_STATE_IN_USE)
			continue;
		adapter->mac_table[i].state |= (NGBE_MAC_STATE_MODIFIED | NGBE_MAC_STATE_IN_USE);
		memcpy(adapter->mac_table[i].addr, addr, ETH_ALEN);
		adapter->mac_table[i].pools = (1ULL << pool);
		ngbe_sync_mac_table(adapter);
		return i;
	}
	return -ENOMEM;
}

static void ngbe_flush_sw_mac_table(struct ngbe_adapter *adapter)
{
	u32 i;
	struct ngbe_hw *hw = &adapter->hw;

	for (i = 0; i < hw->mac.num_rar_entries; i++) {
		adapter->mac_table[i].state |= NGBE_MAC_STATE_MODIFIED;
		adapter->mac_table[i].state &= ~NGBE_MAC_STATE_IN_USE;
		memset(adapter->mac_table[i].addr, 0, ETH_ALEN);
		adapter->mac_table[i].pools = 0;
	}
	ngbe_sync_mac_table(adapter);
}

int ngbe_del_mac_filter(struct ngbe_adapter *adapter, const u8 *addr, u16 pool)
{
	/* search table for addr, if found, set to 0 and sync */
	u32 i;
	struct ngbe_hw *hw = &adapter->hw;

	if (is_zero_ether_addr(addr))
		return -EINVAL;

	for (i = 0; i < hw->mac.num_rar_entries; i++) {
		if (ether_addr_equal(addr, adapter->mac_table[i].addr)) {
			if (adapter->mac_table[i].pools & (1ULL << pool)) {
				adapter->mac_table[i].state |= NGBE_MAC_STATE_MODIFIED;
				adapter->mac_table[i].pools &= ~(1ULL << pool);
				if (!adapter->mac_table[i].pools) {
					adapter->mac_table[i].state &= ~NGBE_MAC_STATE_IN_USE;
					memset(adapter->mac_table[i].addr, 0, ETH_ALEN);
				}
				ngbe_sync_mac_table(adapter);
				return 0;
			}
		}
	}
	return -ENOMEM;
}

/**
 * ngbe_write_uc_addr_list - write unicast addresses to RAR table
 * @netdev: network interface device structure
 * Writes unicast address list to the RAR table.
 * Returns: -ENOMEM on failure/insufficient address space
 *                0 on no addresses written
 *                X on writing X addresses to the RAR table
 **/
int ngbe_write_uc_addr_list(struct net_device *netdev, int pool)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	int count = 0;

	/* return ENOMEM indicating insufficient memory for addresses */
	if (netdev_uc_count(netdev) > ngbe_available_rars(adapter))
		return -ENOMEM;

	if (!netdev_uc_empty(netdev)) {
		struct netdev_hw_addr *ha;

		netdev_for_each_uc_addr(ha, netdev) {
			ngbe_del_mac_filter(adapter, ha->addr, pool);
			ngbe_add_mac_filter(adapter, ha->addr, pool);
			count++;
		}
	}
	return count;
}

static int ngbe_uc_sync(struct net_device *netdev, const unsigned char *addr)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	int ret;

	ret = ngbe_add_mac_filter(adapter, addr, VMDQ_P(0));

	return min_t(int, ret, 0);
}

static int ngbe_uc_unsync(struct net_device *netdev, const unsigned char *addr)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	ngbe_del_mac_filter(adapter, addr, VMDQ_P(0));

	return 0;
}

static void ngbe_vlan_promisc_enable(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 vlnctrl, i;
	u32 vlvfb;
	u32 vind;
	u32 bits;

	vlnctrl = rd32(hw, NGBE_PSR_VLAN_CTL);

	if (adapter->flags & NGBE_FLAG_VMDQ_ENABLED) {
		/* we need to keep the VLAN filter on in SRIOV */
		vlnctrl |= NGBE_PSR_VLAN_CTL_VFE;
		wr32(hw, NGBE_PSR_VLAN_CTL, vlnctrl);
	} else {
		vlnctrl &= ~NGBE_PSR_VLAN_CTL_VFE;
		wr32(hw, NGBE_PSR_VLAN_CTL, vlnctrl);
		return;
	}

	/* We are already in VLAN promisc, nothing to do */
	if (adapter->flags2 & NGBE_FLAG2_VLAN_PROMISC)
		return;

	/* Set flag so we don't redo unnecessary work */
	adapter->flags2 |= NGBE_FLAG2_VLAN_PROMISC;

	/* Add PF to all active pools */
	vind = VMDQ_P(0);
	for (i = NGBE_PSR_VLAN_SWC_ENTRIES; --i;) {
		wr32(hw, NGBE_PSR_VLAN_SWC_IDX, i);
		vlvfb = rd32(hw, NGBE_PSR_VLAN_SWC_IDX);

		bits = rd32(hw, NGBE_PSR_VLAN_SWC_VM_L);
		bits |= (1 << vind);
		wr32(hw, NGBE_PSR_VLAN_SWC_VM_L, bits);
	}

	/* Set all bits in the VLAN filter table array */
	for (i = 0; i < hw->mac.vft_size; i++)
		wr32(hw, NGBE_PSR_VLAN_TBL(i), ~0U);
}

static void ngbe_scrub_vfta(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 i, vid, bits;
	u32 vfta;
	u32 vind;
	u32 vlvf;

	for (i = NGBE_PSR_VLAN_SWC_ENTRIES; --i;) {
		wr32(hw, NGBE_PSR_VLAN_SWC_IDX, i);
		vlvf = rd32(hw, NGBE_PSR_VLAN_SWC_IDX);

		/* pull VLAN ID from VLVF */
		vid = vlvf & ~NGBE_PSR_VLAN_SWC_VIEN;

		if (vlvf & NGBE_PSR_VLAN_SWC_VIEN) {
			/* if PF is part of this then continue */
			if (test_bit(vid, adapter->active_vlans))
				continue;
		}

		/* remove PF from the pool */
		vind = VMDQ_P(0);
		bits = rd32(hw, NGBE_PSR_VLAN_SWC_VM_L);
		bits &= ~(1 << vind);
		wr32(hw, NGBE_PSR_VLAN_SWC_VM_L, bits);
	}

	/* extract values from vft_shadow and write back to VFTA */
	for (i = 0; i < hw->mac.vft_size; i++) {
		vfta = hw->mac.vft_shadow[i];
		wr32(hw, NGBE_PSR_VLAN_TBL(i), vfta);
	}
}

static void ngbe_vlan_promisc_disable(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 vlnctrl;

	/* configure vlan filtering */
	vlnctrl = rd32(hw, NGBE_PSR_VLAN_CTL);
	vlnctrl |= NGBE_PSR_VLAN_CTL_VFE;
	wr32(hw, NGBE_PSR_VLAN_CTL, vlnctrl);

	/* We are not in VLAN promisc, nothing to do */
	if (!(adapter->flags2 & NGBE_FLAG2_VLAN_PROMISC))
		return;

	/* Set flag so we don't redo unnecessary work */
	adapter->flags2 &= ~NGBE_FLAG2_VLAN_PROMISC;

	ngbe_scrub_vfta(adapter);
}

/**
 * ngbe_set_rx_mode - Unicast, Multicast and Promiscuous mode set
 * @netdev: network interface device structure
 * The set_rx_method entry point is called whenever the unicast/multicast
 * address list or the network interface flags are updated.  This routine is
 * responsible for configuring the hardware for proper unicast, multicast and
 * promiscuous mode.
 **/
void ngbe_set_rx_mode(struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	u32 fctrl, vmolr, vlnctrl;
	int count;

	netdev_features_t features = netdev->features;

	/* Check for Promiscuous and All Multicast modes */
	fctrl = rd32m(hw, NGBE_PSR_CTL, ~(NGBE_PSR_CTL_UPE | NGBE_PSR_CTL_MPE));
	vmolr = rd32m(hw, NGBE_PSR_VM_L2CTL(VMDQ_P(0)),
		      ~(NGBE_PSR_VM_L2CTL_UPE | NGBE_PSR_VM_L2CTL_MPE | NGBE_PSR_VM_L2CTL_ROPE |
			NGBE_PSR_VM_L2CTL_ROMPE));
	vlnctrl = rd32m(hw, NGBE_PSR_VLAN_CTL, ~(NGBE_PSR_VLAN_CTL_VFE | NGBE_PSR_VLAN_CTL_CFIEN));

	/* set all bits that we expect to always be set */
	fctrl |= NGBE_PSR_CTL_BAM | NGBE_PSR_CTL_MFE;
	vmolr |= NGBE_PSR_VM_L2CTL_BAM | NGBE_PSR_VM_L2CTL_AUPE | NGBE_PSR_VM_L2CTL_VACC;

	hw->addr_ctrl.user_set_promisc = false;
	if (netdev->flags & IFF_PROMISC) {
		hw->addr_ctrl.user_set_promisc = true;
		fctrl |= (NGBE_PSR_CTL_UPE | NGBE_PSR_CTL_MPE);
		/* pf don't want packets routing to vf, so clear UPE */
		vmolr |= NGBE_PSR_VM_L2CTL_MPE;
		if ((adapter->flags & (NGBE_FLAG_VMDQ_ENABLED | NGBE_FLAG_SRIOV_ENABLED)))
			vlnctrl |= NGBE_PSR_VLAN_CTL_VFE;
		features &= ~NETIF_F_HW_VLAN_CTAG_FILTER;
		features &= ~NETIF_F_HW_VLAN_STAG_FILTER;
	}

	if (netdev->flags & IFF_ALLMULTI) {
		fctrl |= NGBE_PSR_CTL_MPE;
		vmolr |= NGBE_PSR_VM_L2CTL_MPE;
	}

	/* This is useful for sniffing bad packets. */
	if (netdev->features & NETIF_F_RXALL) {
		vmolr |= (NGBE_PSR_VM_L2CTL_UPE | NGBE_PSR_VM_L2CTL_MPE);
		vlnctrl &= ~NGBE_PSR_VLAN_CTL_VFE;
		/* receive bad packets */
		wr32m(hw, NGBE_RSEC_CTL, NGBE_RSEC_CTL_SAVE_MAC_ERR, NGBE_RSEC_CTL_SAVE_MAC_ERR);
		features &= ~NETIF_F_HW_VLAN_CTAG_FILTER;
		features &= ~NETIF_F_HW_VLAN_STAG_FILTER;
	} else {
		vmolr |= NGBE_PSR_VM_L2CTL_ROPE | NGBE_PSR_VM_L2CTL_ROMPE;
	}

	/* Write addresses to available RAR registers, if there is not
	 * sufficient space to store all the addresses then enable
	 * unicast promiscuous mode
	 */
	if (__dev_uc_sync(netdev, ngbe_uc_sync, ngbe_uc_unsync)) {
		vmolr &= ~NGBE_PSR_VM_L2CTL_ROPE;
		fctrl |= NGBE_PSR_CTL_UPE;
	}

	/* Write addresses to the MTA, if the attempt fails
	 * then we should just turn on promiscuous mode so
	 * that we can at least receive multicast traffic
	 */
	count = ngbe_write_mc_addr_list(netdev);
	if (count < 0) {
		vmolr &= ~NGBE_PSR_VM_L2CTL_ROMPE;
		vmolr |= NGBE_PSR_VM_L2CTL_MPE;
	}

	//	wr32(hw, NGBE_PSR_VLAN_CTL, vlnctrl);
	wr32(hw, NGBE_PSR_CTL, fctrl);
	wr32(hw, NGBE_PSR_VM_L2CTL(VMDQ_P(0)), vmolr);

	if ((netdev->features & NETIF_F_HW_VLAN_CTAG_RX) &&
	    (netdev->features & NETIF_F_HW_VLAN_STAG_RX))
		ngbe_vlan_strip_enable(adapter);
	else
		ngbe_vlan_strip_disable(adapter);

	if (features & NETIF_F_HW_VLAN_CTAG_FILTER)
		ngbe_vlan_promisc_disable(adapter);
	else
		ngbe_vlan_promisc_enable(adapter);
}

static void ngbe_napi_enable_all(struct ngbe_adapter *adapter)
{
	struct ngbe_q_vector *q_vector;
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++) {
		q_vector = adapter->q_vector[q_idx];
		napi_enable(&q_vector->napi);
	}
}

static void ngbe_napi_disable_all(struct ngbe_adapter *adapter)
{
	struct ngbe_q_vector *q_vector;
	int q_idx;

	for (q_idx = 0; q_idx < adapter->num_q_vectors; q_idx++) {
		q_vector = adapter->q_vector[q_idx];
		napi_disable(&q_vector->napi);
	}
}

#define NGBE_GSO_PARTIAL_FEATURES                                                           \
	(NETIF_F_GSO_GRE | NETIF_F_GSO_GRE_CSUM | NETIF_F_GSO_IPXIP4 | NETIF_F_GSO_IPXIP6 | \
	 NETIF_F_GSO_UDP_TUNNEL | NETIF_F_GSO_UDP_TUNNEL_CSUM)

static inline unsigned long ngbe_tso_features(void)
{
	unsigned long features = 0;

	features |= NETIF_F_TSO;
	features |= NETIF_F_TSO6;
	features |= NETIF_F_GSO_PARTIAL | NGBE_GSO_PARTIAL_FEATURES;

	return features;
}

static void ngbe_configure_lli(struct ngbe_adapter *adapter)
{
	/* lli should only be enabled with MSI-X and MSI */
	if (!(adapter->flags & NGBE_FLAG_MSI_ENABLED) && !(adapter->flags & NGBE_FLAG_MSIX_ENABLED))
		return;

	if (adapter->lli_etype) {
		wr32(&adapter->hw, NGBE_RDB_5T_CTL1(0),
		     (NGBE_RDB_5T_CTL1_LLI | NGBE_RDB_5T_CTL1_SIZE_BP));
		wr32(&adapter->hw, NGBE_RDB_ETYPE_CLS(0), NGBE_RDB_ETYPE_CLS_LLI);
		wr32(&adapter->hw, NGBE_PSR_ETYPE_SWC(0),
		     (adapter->lli_etype | NGBE_PSR_ETYPE_SWC_FILTER_EN));
	}

	if (adapter->lli_port) {
		wr32(&adapter->hw, NGBE_RDB_5T_CTL1(0),
		     (NGBE_RDB_5T_CTL1_LLI | NGBE_RDB_5T_CTL1_SIZE_BP));

		wr32(&adapter->hw, NGBE_RDB_5T_CTL0(0),
		     (NGBE_RDB_5T_CTL0_POOL_MASK_EN |
		      (NGBE_RDB_5T_CTL0_PRIORITY_MASK << NGBE_RDB_5T_CTL0_PRIORITY_SHIFT) |
		      (NGBE_RDB_5T_CTL0_DEST_PORT_MASK << NGBE_RDB_5T_CTL0_5TUPLE_MASK_SHIFT)));

		wr32(&adapter->hw, NGBE_RDB_5T_SDP(0), (adapter->lli_port << 16));
	}

	if (adapter->lli_size) {
		wr32(&adapter->hw, NGBE_RDB_5T_CTL1(0), NGBE_RDB_5T_CTL1_LLI);
		wr32m(&adapter->hw, NGBE_RDB_LLI_THRE, NGBE_RDB_LLI_THRE_SZ(~0), adapter->lli_size);
		wr32(&adapter->hw, NGBE_RDB_5T_CTL0(0),
		     (NGBE_RDB_5T_CTL0_POOL_MASK_EN |
		      (NGBE_RDB_5T_CTL0_PRIORITY_MASK << NGBE_RDB_5T_CTL0_PRIORITY_SHIFT) |
		      (NGBE_RDB_5T_CTL0_5TUPLE_MASK_MASK << NGBE_RDB_5T_CTL0_5TUPLE_MASK_SHIFT)));
	}

	if (adapter->lli_vlan_pri) {
		wr32m(&adapter->hw, NGBE_RDB_LLI_THRE,
		      NGBE_RDB_LLI_THRE_PRIORITY_EN | NGBE_RDB_LLI_THRE_UP(~0),
		      NGBE_RDB_LLI_THRE_PRIORITY_EN |
			      (adapter->lli_vlan_pri << NGBE_RDB_LLI_THRE_UP_SHIFT));
	}
}

/* Additional bittime to account for NGBE framing */
#define NGBE_ETH_FRAMING 20

/* ngbe_hpbthresh - calculate high water mark for flow control
 * @adapter: board private structure to calculate for
 * @pb - packet buffer to calculate
 */
static int ngbe_hpbthresh(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	struct net_device *dev = adapter->netdev;
	int link, tc, kb, marker;
	u32 dv_id, rx_pba;

	/* Calculate max LAN frame size */
	link = dev->mtu + ETH_HLEN + ETH_FCS_LEN + NGBE_ETH_FRAMING;
	tc = link;

	/* Calculate delay value for device */
	dv_id = NGBE_DV(link, tc);

	/* Loopback switch introduces additional latency */
	if (adapter->flags & NGBE_FLAG_SRIOV_ENABLED)
		dv_id += NGBE_B2BT(tc);

	/* Delay value is calculated in bit times convert to KB */
	kb = NGBE_BT2KB(dv_id);
	rx_pba = rd32(hw, NGBE_RDB_PB_SZ) >> NGBE_RDB_PB_SZ_SHIFT;

	marker = rx_pba - kb;

	/* It is possible that the packet buffer is not large enough
	 * to provide required headroom. In this case throw an error
	 * to user and a do the best we can.
	 */
	if (marker < 0) {
		e_warn(drv, "Packet buffer headroom insufficient(fc). Lower MTU/traffic classes\n");
		marker = tc + 1;
	}

	return marker;
}

/* ngbe_lpbthresh - calculate low water mark for flow control
 * @adapter: board private structure to calculate for
 * @pb - packet buffer to calculate
 */
static int ngbe_lpbthresh(struct ngbe_adapter *adapter)
{
	struct net_device *dev = adapter->netdev;
	int tc;
	u32 dv_id;

	/* Calculate max LAN frame size */
	tc = dev->mtu + ETH_HLEN + ETH_FCS_LEN;

	/* Calculate delay value for device */
	dv_id = NGBE_LOW_DV(tc);

	/* Delay value is calculated in bit times convert to KB */
	return NGBE_BT2KB(dv_id);
}

/* ngbe_pbthresh_setup - calculate and setup high low water marks
 */

static void ngbe_pbthresh_setup(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	int num_tc = netdev_get_num_tc(adapter->netdev);

	if (!num_tc)
		num_tc = 1;

	hw->fc.high_water = ngbe_hpbthresh(adapter);
	hw->fc.low_water = ngbe_lpbthresh(adapter);

	/* Low water marks must not be larger than high water marks */
	if (hw->fc.low_water > hw->fc.high_water)
		hw->fc.low_water = 0;
}

static void ngbe_configure_pb(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	int hdrm = 0;
	int tc = netdev_get_num_tc(adapter->netdev);

	hw->mac.ops.setup_rxpba(hw, tc, hdrm, PBA_STRATEGY_EQUAL);
	ngbe_pbthresh_setup(adapter);
}

static void ngbe_configure_isb(struct ngbe_adapter *adapter)
{
	/* set ISB Address */
	struct ngbe_hw *hw = &adapter->hw;

	wr32(hw, NGBE_PX_ISB_ADDR_L, adapter->isb_dma & DMA_BIT_MASK(32));
#ifdef CONFIG_64BIT
	wr32(hw, NGBE_PX_ISB_ADDR_H, adapter->isb_dma >> 32);
#else
	wr32(hw, NGBE_PX_ISB_ADDR_H, 0);
#endif
}

static void ngbe_configure_port(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 value, i;

	value = (adapter->num_vfs == 0) ? NGBE_CFG_PORT_CTL_NUM_VT_NONE
					: NGBE_CFG_PORT_CTL_NUM_VT_8;

	/* enable double vlan and qinq, NONE VT at default */
	value |= NGBE_CFG_PORT_CTL_D_VLAN | NGBE_CFG_PORT_CTL_QINQ;
	wr32m(hw, NGBE_CFG_PORT_CTL,
	      NGBE_CFG_PORT_CTL_D_VLAN | NGBE_CFG_PORT_CTL_QINQ | NGBE_CFG_PORT_CTL_NUM_VT_MASK,
	      value);

	wr32(hw, NGBE_CFG_TAG_TPID(0), ETH_P_8021Q | ETH_P_8021AD << 16);
	adapter->hw.tpid[0] = ETH_P_8021Q;
	adapter->hw.tpid[1] = ETH_P_8021AD;
	for (i = 1; i < 4; i++)
		wr32(hw, NGBE_CFG_TAG_TPID(i), ETH_P_8021Q | ETH_P_8021Q << 16);
	for (i = 2; i < 8; i++)
		adapter->hw.tpid[i] = ETH_P_8021Q;
}

static void ngbe_configure(struct ngbe_adapter *adapter)
{
	ngbe_configure_pb(adapter);

	/* We must restore virtualization before VLANs or else
	 * the VLVF registers will not be populated
	 */
	ngbe_configure_virtualization(adapter);
	/* configure Double Vlan */
	ngbe_configure_port(adapter);

	ngbe_set_rx_mode(adapter->netdev);
	ngbe_restore_vlan(adapter);

	ngbe_configure_tx(adapter);
	ngbe_configure_rx(adapter);
	ngbe_configure_isb(adapter);
}

/**
 * ngbe_non_sfp_link_config - set up non-SFP+ link
 * @hw: pointer to private hardware struct
 * Returns 0 on success, negative on failure
 **/
static int ngbe_non_sfp_link_config(struct ngbe_hw *hw)
{
	u32 speed;
	u32 ret = NGBE_ERR_LINK_SETUP;
	struct ngbe_adapter *adapter = hw->back;

	if (hw->mac.autoneg)
		speed = hw->phy.autoneg_advertised;
	else
		speed = hw->phy.force_speed;
	if (ngbe_is_lldp(hw))
		e_dev_err("Can not get lldp flags from flash\n");
	/* power on phy first anyway */
	hw->phy.ops.phy_resume(hw);
	if (hw->ncsi_enabled || hw->phy.type == ngbe_phy_fpga ||
	    adapter->eth_priv_flags & NGBE_ETH_PRIV_FLAG_LLDP)
		return 0;

	if (hw->phy.type == ngbe_phy_internal || hw->phy.type == ngbe_phy_internal_yt8521s_sfi)
		hw->phy.ops.setup_once(hw);

	ret = hw->mac.ops.setup_link(hw, speed, false);

	return ret;
}

static void ngbe_setup_gpie(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 gpie = 0;

	if (adapter->flags & NGBE_FLAG_MSIX_ENABLED) {
		gpie = NGBE_PX_GPIE_MODEL;
		/* use EIAM to auto-mask when MSI-X interrupt is asserted
		 * this saves a register write for every interrupt
		 */
	} else {
		/* legacy interrupts, use EIAM to auto-mask when reading EICR,
		 * specifically only auto mask tx and rx interrupts
		 */
	}

	wr32(hw, NGBE_PX_GPIE, gpie);
}

static void ngbe_up_complete(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	int err;

	ngbe_get_hw_control(adapter);
	ngbe_setup_gpie(adapter);

	if (adapter->flags & NGBE_FLAG_MSIX_ENABLED)
		ngbe_configure_msix(adapter);
	else
		ngbe_configure_msi_and_legacy(adapter);
	/* Make sure to clear down flag */
	smp_mb__before_atomic();
	clear_bit(__NGBE_DOWN, &adapter->state);
	ngbe_napi_enable_all(adapter);
	ngbe_configure_lli(adapter);

	err = ngbe_non_sfp_link_config(hw);
	if (err)
		e_err(probe, "link_config FAILED %d\n", err);

	/* sellect GMII */
	wr32(hw, NGBE_MAC_TX_CFG,
	     (rd32(hw, NGBE_MAC_TX_CFG) & ~NGBE_MAC_TX_CFG_SPEED_MASK) | NGBE_MAC_TX_CFG_SPEED_1G);

	/* clear any pending interrupts, may auto mask */
	rd32(hw, NGBE_PX_IC);
	rd32(hw, NGBE_PX_MISC_IC);
	if (ngbe_check_reset_blocked(hw))
		hw->phy.ops.check_event(hw);
	ngbe_irq_enable(adapter, true, true);

	if (hw->gpio_ctl == 1)
		/* gpio0 is used to power on/off control*/
		wr32(hw, NGBE_GPIO_DR, 0);

	/* enable transmits */
	//	if (!((adapter->num_rx_queues == 0) && (adapter->num_tx_queues == 0)))
	netif_tx_start_all_queues(adapter->netdev);

	adapter->pf_running = true;
	adapter->need_delay = true;
	ngbe_ping_all_vfs_with_link_status(adapter, false);

	/* bring the link up in the watchdog, this could race with our first
	 * link up interrupt but shouldn't be a problem
	 */
	adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
	adapter->link_check_timeout = jiffies;
	if (NGBE_POLL_LINK_STATUS == 1)
		mod_timer(&adapter->link_check_timer, jiffies);
	mod_timer(&adapter->service_timer, jiffies);
	/* ngbe_clear_vf_stats_counters(adapter); */

	if (hw->bus.lan_id == 0)
		wr32m(hw, NGBE_MIS_PRB_CTL, NGBE_MIS_PRB_CTL_LAN0_UP, NGBE_MIS_PRB_CTL_LAN0_UP);
	else if (hw->bus.lan_id == 1)
		wr32m(hw, NGBE_MIS_PRB_CTL, NGBE_MIS_PRB_CTL_LAN1_UP, NGBE_MIS_PRB_CTL_LAN1_UP);
	else if (hw->bus.lan_id == 2)
		wr32m(hw, NGBE_MIS_PRB_CTL, NGBE_MIS_PRB_CTL_LAN2_UP, NGBE_MIS_PRB_CTL_LAN2_UP);
	else if (hw->bus.lan_id == 3)
		wr32m(hw, NGBE_MIS_PRB_CTL, NGBE_MIS_PRB_CTL_LAN3_UP, NGBE_MIS_PRB_CTL_LAN3_UP);
	else
		e_err(probe, "%s:invalid bus lan id %d\n", __func__, hw->bus.lan_id);

	/* Set PF Reset Done bit so PF/VF Mail Ops can work */
	wr32m(hw, NGBE_CFG_PORT_CTL, NGBE_CFG_PORT_CTL_PFRSTD, NGBE_CFG_PORT_CTL_PFRSTD);

	/* clear ecc reset flag if set */
	if (adapter->flags2 & NGBE_FLAG2_ECC_ERR_RESET)
		adapter->flags2 &= ~NGBE_FLAG2_ECC_ERR_RESET;

	/* update setting rx tx for all active vfs */
	ngbe_set_all_vfs(adapter);
}

void ngbe_reinit_locked(struct ngbe_adapter *adapter)
{
	WARN_ON(in_interrupt());
	/* put off any impending NetWatchDogTimeout */
	netif_trans_update(adapter->netdev);

	while (test_and_set_bit(__NGBE_RESETTING, &adapter->state))
		usleep_range(1000, 2000);
	ngbe_down(adapter);
	/* If SR-IOV enabled then wait a bit before bringing the adapter
	 * back up to give the VFs time to respond to the reset.  The
	 * two second wait is based upon the watchdog timer cycle in
	 * the VF driver.
	 */
	if (adapter->flags & NGBE_FLAG_SRIOV_ENABLED)
		msleep(2000);
	ngbe_up(adapter);
	clear_bit(__NGBE_RESETTING, &adapter->state);
}

void ngbe_up(struct ngbe_adapter *adapter)
{
	/* hardware has been reset, we need to reload some things */
	ngbe_configure(adapter);

	ngbe_up_complete(adapter);
}

void ngbe_reset(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	struct net_device *netdev = adapter->netdev;
	int err;
	u8 old_addr[ETH_ALEN];

	if (NGBE_REMOVED(hw->hw_addr))
		return;

	err = hw->mac.ops.init_hw(hw);
	switch (err) {
	case 0:
		break;
	case NGBE_ERR_MASTER_REQUESTS_PENDING:
		e_dev_err("master disable timed out\n");
		ngbe_tx_timeout_dorecovery(adapter);
		break;
	case NGBE_ERR_EEPROM_VERSION:
		/* We are running on a pre-production device, log a warning */
		e_dev_warn("This device is a pre-production adapter/LOM.\n");
		break;
	default:
		e_dev_err("Hardware Error: %d\n", err);
	}

	/* do not flush user set addresses */
	memcpy(old_addr, &adapter->mac_table[0].addr, netdev->addr_len);
	ngbe_flush_sw_mac_table(adapter);
	ngbe_mac_set_default_filter(adapter, old_addr);

	/* update SAN MAC vmdq pool selection */
	hw->mac.ops.set_vmdq_san_mac(hw, VMDQ_P(0));

	/* Clear saved DMA coalescing values except for watchdog_timer */
	hw->mac.dmac_config.fcoe_en = false;
	hw->mac.dmac_config.link_speed = 0;
	hw->mac.dmac_config.fcoe_tc = 0;
	hw->mac.dmac_config.num_tcs = 0;

	if (test_bit(__NGBE_PTP_RUNNING, &adapter->state))
		ngbe_ptp_reset(adapter);
}

/**
 * ngbe_clean_rx_ring - Free Rx Buffers per Queue
 * @rx_ring: ring to free buffers from
 **/
void ngbe_clean_rx_ring(struct ngbe_ring *rx_ring)
{
	struct device *dev = rx_ring->dev;
	unsigned long size;
	u16 i;

	if (rx_ring->xsk_pool) {
		ngbe_xsk_clean_rx_ring(rx_ring);
		goto skip_free;
	}

	/* ring already cleared, nothing to do */
	if (!rx_ring->rx_buffer_info)
		return;

	/* Free all the Rx ring sk_buffs */
	for (i = 0; i < rx_ring->count; i++) {
		struct ngbe_rx_buffer *rx_buffer = &rx_ring->rx_buffer_info[i];

		if (rx_buffer->dma) {
			dma_unmap_single(dev, rx_buffer->dma, rx_ring->rx_buf_len, DMA_FROM_DEVICE);
			rx_buffer->dma = 0;
		}

		if (rx_buffer->skb) {
			struct sk_buff *skb = rx_buffer->skb;

			if (NGBE_CB(skb)->dma_released) {
				dma_unmap_single(dev, NGBE_CB(skb)->dma, rx_ring->rx_buf_len,
						 DMA_FROM_DEVICE);
				NGBE_CB(skb)->dma = 0;
				NGBE_CB(skb)->dma_released = false;
			}

			if (NGBE_CB(skb)->page_released)
				dma_unmap_page(dev, NGBE_CB(skb)->dma, ngbe_rx_bufsz(rx_ring),
					       DMA_FROM_DEVICE);
			dev_kfree_skb(skb);
			rx_buffer->skb = NULL;
		}

		if (!rx_buffer->page)
			continue;

		dma_unmap_page(dev, rx_buffer->page_dma, ngbe_rx_pg_size(rx_ring), DMA_FROM_DEVICE);

		__page_frag_cache_drain(rx_buffer->page, rx_buffer->pagecnt_bias);
		rx_buffer->page = NULL;
	}

	size = sizeof(struct ngbe_rx_buffer) * rx_ring->count;
	memset(rx_ring->rx_buffer_info, 0, size);

	/* Zero out the descriptor ring */
	memset(rx_ring->desc, 0, rx_ring->size);
skip_free:
	rx_ring->next_to_alloc = 0;
	rx_ring->next_to_clean = 0;
	rx_ring->next_to_use = 0;
}

/**
 * ngbe_clean_tx_ring - Free Tx Buffers
 * @tx_ring: ring to be cleaned
 **/
void ngbe_clean_tx_ring(struct ngbe_ring *tx_ring)
{
	struct ngbe_tx_buffer *tx_buffer_info;
	unsigned long size;
	u16 i;

	if (tx_ring->xsk_pool) {
		ngbe_xsk_clean_tx_ring(tx_ring);
		return;
	}

	/* ring already cleared, nothing to do */
	if (!tx_ring->tx_buffer_info)
		return;

	/* Free all the Tx ring sk_buffs */
	for (i = 0; i < tx_ring->count; i++) {
		tx_buffer_info = &tx_ring->tx_buffer_info[i];
		if (ring_is_xdp(tx_ring)) {
			if (tx_buffer_info->xdpf)
				xdp_return_frame(tx_buffer_info->xdpf);
		}
		ngbe_unmap_and_free_tx_resource(tx_ring, tx_buffer_info);
	}
	if (!ring_is_xdp(tx_ring))
		netdev_tx_reset_queue(txring_txq(tx_ring));

	size = sizeof(struct ngbe_tx_buffer) * tx_ring->count;
	memset(tx_ring->tx_buffer_info, 0, size);

	/* Zero out the descriptor ring */
	memset(tx_ring->desc, 0, tx_ring->size);
}

/**
 * ngbe_clean_all_rx_rings - Free Rx Buffers for all queues
 * @adapter: board private structure
 **/
static void ngbe_clean_all_rx_rings(struct ngbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		ngbe_clean_rx_ring(adapter->rx_ring[i]);
}

/**
 * ngbe_clean_all_tx_rings - Free Tx Buffers for all queues
 * @adapter: board private structure
 **/
static void ngbe_clean_all_tx_rings(struct ngbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		ngbe_clean_tx_ring(adapter->tx_ring[i]);
	for (i = 0; i < adapter->num_xdp_queues; i++)
		ngbe_clean_tx_ring(adapter->xdp_ring[i]);
}

static void ngbe_disable_device(struct ngbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct ngbe_hw *hw = &adapter->hw;
	u32 i;

	/* signal that we are down to the interrupt handler */
	if (test_and_set_bit(__NGBE_DOWN, &adapter->state))
		return; /* do nothing if already down */

	adapter->pf_running = false;
	if (adapter->num_vfs) {
		/* Clear EITR Select mapping */
		wr32(&adapter->hw, NGBE_PX_ITRSEL, 0);

		/* Mark all the VFs as inactive */
		for (i = 0; i < adapter->num_vfs; i++)
			adapter->vfinfo[i].clear_to_send = 0;

		/* ping all the active vfs to let them know we are going down */
		ngbe_ping_all_vfs_with_link_status(adapter, false);

		/* Disable all VFTE/VFRE TX/RX */
		// ngbe_disable_tx_rx(adapter);
		ngbe_set_all_vfs(adapter);
	}

	if (!(adapter->flags2 & NGBE_FLAG2_ECC_ERR_RESET))
		ngbe_disable_pcie_master(hw);

	/* disable receives */
	hw->mac.ops.disable_rx(hw);

	/* disable all enabled rx queues */
	for (i = 0; i < adapter->num_rx_queues; i++)
		/* this call also flushes the previous write */
		ngbe_disable_rx_queue(adapter, adapter->rx_ring[i]);

	netif_tx_stop_all_queues(netdev);

	/* call carrier off first to avoid false dev_watchdog timeouts */
	netif_carrier_off(netdev);
	netif_tx_disable(netdev);

	if (hw->gpio_ctl == 1)
		/* gpio0 is used to power on/off control*/
		wr32(hw, NGBE_GPIO_DR, NGBE_GPIO_DR_0);

	/* synchronize_rcu() needed for pending XDP buffers to drain */
	if (adapter->xdp_ring[0])
		synchronize_rcu();

	ngbe_irq_disable(adapter);

	ngbe_napi_disable_all(adapter);

	adapter->flags2 &= ~(NGBE_FLAG2_PF_RESET_REQUESTED | NGBE_FLAG2_DEV_RESET_REQUESTED |
			     NGBE_FLAG2_GLOBAL_RESET_REQUESTED);
	adapter->flags &= ~NGBE_FLAG_NEED_LINK_UPDATE;

	del_timer_sync(&adapter->service_timer);
	if (NGBE_POLL_LINK_STATUS == 1)
		del_timer_sync(&adapter->link_check_timer);

	if (hw->bus.lan_id == 0)
		wr32m(hw, NGBE_MIS_PRB_CTL, NGBE_MIS_PRB_CTL_LAN0_UP, 0);
	else if (hw->bus.lan_id == 1)
		wr32m(hw, NGBE_MIS_PRB_CTL, NGBE_MIS_PRB_CTL_LAN1_UP, 0);
	else if (hw->bus.lan_id == 2)
		wr32m(hw, NGBE_MIS_PRB_CTL, NGBE_MIS_PRB_CTL_LAN2_UP, 0);
	else if (hw->bus.lan_id == 3)
		wr32m(hw, NGBE_MIS_PRB_CTL, NGBE_MIS_PRB_CTL_LAN3_UP, 0);
	else
		e_dev_err("%s:invalid bus lan id %d\n", __func__, hw->bus.lan_id);
	if (ngbe_is_lldp(hw))
		e_dev_err("Can not get lldp flags from flash\n");
	/*OCP NCSI need it*/
	if (!(hw->wol_enabled || hw->ncsi_enabled ||
	      adapter->eth_priv_flags & NGBE_ETH_PRIV_FLAG_LLDP))
		/* disable mac transmiter */
		wr32m(hw, NGBE_MAC_TX_CFG, NGBE_MAC_TX_CFG_TE, 0);

	/* disable transmits in the hardware now that interrupts are off */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		u8 reg_idx = adapter->tx_ring[i]->reg_idx;

		wr32(hw, NGBE_PX_TR_CFG(reg_idx), NGBE_PX_TR_CFG_SWFLSH);
	}

	for (i = 0; i < adapter->num_xdp_queues; i++) {
		u8 reg_idx = adapter->xdp_ring[i]->reg_idx;

		wr32(hw, NGBE_PX_TR_CFG(reg_idx), NGBE_PX_TR_CFG_SWFLSH);
	}
	/* Disable the Tx DMA engine */
	wr32m(hw, NGBE_TDM_CTL, NGBE_TDM_CTL_TE, 0);
}

void ngbe_down(struct ngbe_adapter *adapter)
{
	ngbe_disable_device(adapter);

	if (!pci_channel_offline(adapter->pdev))
		ngbe_reset(adapter);

	ngbe_clean_all_tx_rings(adapter);
	ngbe_clean_all_rx_rings(adapter);
}

/**
 *  ngbe_init_shared_code - Initialize the shared code
 *  @hw: pointer to hardware structure
 *  This will assign function pointers and assign the MAC type and PHY code.
 *  Does not touch the hardware. This function must be called prior to any
 *  other function in the shared code. The ngbe_hw structure should be
 *  memset to 0 prior to calling this function.  The following fields in
 *  hw structure should be filled in prior to calling this function:
 *  hw_addr, back, device_id, vendor_id, subsystem_device_id,
 *  subsystem_vendor_id, and revision_id
 **/
static int ngbe_init_shared_code(struct ngbe_hw *hw)
{
	int wol_mask = 0, ncsi_mask = 0;
	u16 type_mask = 0, val;
	u32 lan_en;
	bool ncsi_pin = false;

	lan_en = rd32(hw, NGBE_MIS_PWR);
	if (!(lan_en & BIT(hw->bus.lan_id + 28)))
		return -EIO;
	type_mask = (u16)(hw->subsystem_device_id & OEM_MASK);
	ncsi_mask = hw->subsystem_device_id & NCSI_SUP_MASK;
	wol_mask = hw->subsystem_device_id & WOL_SUP_MASK;

	val = rd32(hw, NGBE_CFG_PORT_ST);
	hw->mac_type = (val & BIT(7)) >> 7 ? em_mac_type_rgmii : em_mac_type_mdi;

	switch (type_mask) {
	case LY_YT8521S_SFP:
	case LY_M88E1512_SFP:
	case YT8521S_SFP_GPIO:
	case INTERNAL_YT8521S_SFP_GPIO:
	case NGBE_EX_PHY:
		hw->gpio_ctl = 1;
		break;
	default:
		hw->gpio_ctl = 0;
		break;
	}

	switch (type_mask) {
	case M88E1512_SFP:
	case LY_M88E1512_SFP:
		hw->phy.type = ngbe_phy_m88e1512_sfi;
		break;
	case M88E1512_RJ45:
		hw->phy.type = ngbe_phy_m88e1512;
		break;
	case M88E1512_MIX:
		hw->phy.type = ngbe_phy_m88e1512_unknown;
		break;
	case YT8521S_SFP:
	case YT8521S_SFP_GPIO:
	case LY_YT8521S_SFP:
		hw->phy.type = ngbe_phy_yt8521s_sfi;
		break;
	case INTERNAL_YT8521S_SFP:
	case INTERNAL_YT8521S_SFP_GPIO:
		hw->phy.type = ngbe_phy_internal_yt8521s_sfi;
		break;
	case NGBE_EX_PHY:
		hw->phy.type = ngbe_phy_external;
		break;
	case NGBE_PHY_MIX:
		hw->phy.type = ngbe_phy_mix;
		break;
	case RGMII_FPGA:
		hw->phy.type = ngbe_phy_fpga;
		break;
	default:
		hw->phy.type = ngbe_phy_internal;
		break;
	}

	/* select claus22 */
	wr32(hw, NGBE_MDIO_CLAUSE_SELECT, 0xF);

	/* external phy mix type need to determine phy_type */
	if (hw->phy.type == ngbe_phy_mix) {
		unsigned long port_mode = 0;
		u32 status = 0;
		unsigned long i = 0;
		enum ngbe_phy_type mix_phy_type[6] = {
			ngbe_phy_unknown,     ngbe_phy_m88e1512_unknown,
			ngbe_phy_yt8521s_sfi, ngbe_phy_external,
			ngbe_phy_fpga,	      ngbe_phy_unknown,
		};

		status = ngbe_get_phy_mix_mode(hw, &port_mode);
		if (status)
			return status;

		if (port_mode & 0x80) {
			for_each_set_bit(i, &port_mode, 6) hw->phy.type = mix_phy_type[i];

			if (port_mode & 0x40)
				hw->gpio_ctl = 1;
		}
	}

	ngbe_init_ops(hw);

	hw->wol_enabled = (wol_mask == WOL_SUP) ? true : false;
	hw->ncsi_enabled = (ncsi_mask == NCSI_SUP || type_mask == OCP_CARD) ? true : false;

	/* need to check ncsi pin status for oem ncsi card */
	if (hw->phy.type == ngbe_phy_internal && !hw->wol_enabled && !hw->ncsi_enabled) {
		ncsi_pin = ngbe_get_ncsi_pin_status(hw);
		hw->wol_enabled = ncsi_pin;
		hw->ncsi_enabled = hw->wol_enabled;
	}

	return 0;
}

/**
 * ngbe_sw_init - Initialize general software structures (struct ngbe_adapter)
 * @adapter: board private structure to initialize
 * ngbe_sw_init initializes the Adapter private data structure.
 * Fields are initialized based on PCI device information and
 * OS network device settings (MTU size).
 **/
static const u32 def_rss_key[10] = {0xE291D73D, 0x1805EC6C, 0x2A94B30D, 0xA54F2BEC, 0xEA49AF7C,
				    0xE214AD3D, 0xB855AABE, 0x6A3E67EA, 0x14364D17, 0x3BED200D};

static int ngbe_sw_init(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	struct pci_dev *pdev = adapter->pdev;
	int err;
	u32 ssid = 0;

	/* PCI config space info */
	hw->vendor_id = pdev->vendor;
	hw->device_id = pdev->device;
	pci_read_config_byte(pdev, PCI_REVISION_ID, &hw->revision_id);
	if (hw->revision_id == NGBE_FAILED_READ_CFG_BYTE && ngbe_check_cfg_remove(hw, pdev)) {
		e_err(probe, "read of revision id failed\n");
		err = -ENODEV;
		goto out;
	}

	hw->oem_svid = pdev->subsystem_vendor;
	hw->oem_ssid = pdev->subsystem_device;
	if (pdev->subsystem_vendor == 0x8088) {
		hw->subsystem_vendor_id = pdev->subsystem_vendor;
		hw->subsystem_device_id = pdev->subsystem_device;
	} else {
		err = ngbe_flash_read_dword(hw, 0xfffdc, &ssid);
		if (err) {
			e_err(probe, "read of internal subsystem device id failed\n");
			err = -ENODEV;
			goto out;
		}
		hw->subsystem_device_id = (u16)ssid;
		hw->subsystem_device_id = hw->subsystem_device_id >> 8 | hw->subsystem_device_id
										 << 8;
	}

	/* phy type, phy ops, mac ops */
	err = ngbe_init_shared_code(hw);
	if (err) {
		e_err(probe, "init_shared_code failed: %d\n", err);
		goto out;
	}

	adapter->mac_table =
		kzalloc(sizeof(struct ngbe_mac_addr) * hw->mac.num_rar_entries, GFP_ATOMIC);
	if (!adapter->mac_table) {
		err = NGBE_ERR_OUT_OF_MEM;
		e_err(probe, "mac_table allocation failed: %d\n", err);
		goto out;
	}

	memcpy(adapter->rss_key, def_rss_key, sizeof(def_rss_key));
	adapter->af_xdp_zc_qps = bitmap_zalloc(MAX_XDP_QUEUES, GFP_KERNEL);
	if (!adapter->af_xdp_zc_qps)
		return -ENOMEM;

	/* Set common capability flags and settings */
	adapter->max_q_vectors = NGBE_MAX_MSIX_Q_VECTORS_EMERALD;

	/* Set MAC specific capability flags and exceptions */
	adapter->flags |= NGBE_FLAGS_SP_INIT;
	adapter->flags2 |= NGBE_FLAG2_TEMP_SENSOR_CAPABLE;
	adapter->flags2 |= NGBE_FLAG2_EEE_CAPABLE;

	/* lock to protect mailbox accesses */
	spin_lock_init(&adapter->mbx_lock);
	spin_lock_init(&hw->phy_lock);
	/* init mailbox params */
	hw->mbx.ops.init_params(hw);

	/* default flow control settings */
	hw->fc.requested_mode = ngbe_fc_full;
	hw->fc.current_mode = ngbe_fc_full; /* init for ethtool output */

	adapter->last_lfc_mode = hw->fc.current_mode;
	hw->fc.pause_time = NGBE_DEFAULT_FCPAUSE;
	hw->fc.send_xon = true;
	hw->fc.disable_fc_autoneg = false;

	/* set default ring sizes */
	adapter->tx_ring_count = NGBE_DEFAULT_TXD;
	adapter->rx_ring_count = NGBE_DEFAULT_RXD;

	/* set default work limits */
	adapter->tx_work_limit = NGBE_DEFAULT_TX_WORK;
	adapter->rx_work_limit = NGBE_DEFAULT_RX_WORK;

	adapter->tx_timeout_recovery_level = 0;
	adapter->cmplt_to_dis = false;

	adapter->speed = NGBE_LINK_SPEED_1GB_FULL;
	/* PF holds first pool slot */
	adapter->num_vmdqs = 1;
	set_bit(0, &adapter->fwd_bitmask);
	set_bit(__NGBE_DOWN, &adapter->state);
	if (NGBE_LINK_RETRY == 1)
		hw->restart_an = 0;

out:
	return err;
}

/**
 * ngbe_setup_tx_resources - allocate Tx resources (Descriptors)
 * @tx_ring:    tx descriptor ring (for a specific queue) to setup
 * Return 0 on success, negative on failure
 **/
int ngbe_setup_tx_resources(struct ngbe_ring *tx_ring)
{
	struct device *dev = tx_ring->dev;
	int orig_node = dev_to_node(dev);
	int numa_node = -1;
	int size;

	size = sizeof(struct ngbe_tx_buffer) * tx_ring->count;

	if (tx_ring->q_vector)
		numa_node = tx_ring->q_vector->numa_node;

	tx_ring->tx_buffer_info = vzalloc_node(size, numa_node);
	if (!tx_ring->tx_buffer_info)
		tx_ring->tx_buffer_info = vzalloc(size);
	if (!tx_ring->tx_buffer_info)
		goto err;

	/* round up to nearest 4K */
	tx_ring->size = tx_ring->count * sizeof(union ngbe_tx_desc);
	tx_ring->size = ALIGN(tx_ring->size, 4096);

	set_dev_node(dev, numa_node);
	tx_ring->desc = dma_alloc_coherent(dev, tx_ring->size, &tx_ring->dma, GFP_KERNEL);
	set_dev_node(dev, orig_node);
	if (!tx_ring->desc)
		tx_ring->desc = dma_alloc_coherent(dev, tx_ring->size, &tx_ring->dma, GFP_KERNEL);
	if (!tx_ring->desc)
		goto err;

	return 0;

err:
	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;
	dev_err(dev, "Unable to allocate memory for the Tx descriptor ring\n");
	return -ENOMEM;
}

/**
 * ngbe_setup_all_tx_resources - allocate all queues Tx resources
 * @adapter: board private structure
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 * Return 0 on success, negative on failure
 **/
static int ngbe_setup_all_tx_resources(struct ngbe_adapter *adapter)
{
	int i = 0, j = 0, err = 0;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		err = ngbe_setup_tx_resources(adapter->tx_ring[i]);
		if (!err)
			continue;

		e_err(probe, "Allocation for Tx Queue %u failed\n", i);
		goto err_setup_tx;
	}

	for (j = 0; j < adapter->num_xdp_queues; j++) {
		err = ngbe_setup_tx_resources(adapter->xdp_ring[j]);
		if (!err)
			continue;

		e_err(probe, "Allocation for Tx(XDP) Queue %u failed\n", j);
		goto err_setup_tx;
	}

	return 0;
err_setup_tx:
	/* rewind the index freeing the rings as we go */
	while (j--)
		ngbe_free_tx_resources(adapter->xdp_ring[j]);
	while (i--)
		ngbe_free_tx_resources(adapter->tx_ring[i]);
	return err;
}

/**
 * ngbe_setup_rx_resources - allocate Rx resources (Descriptors)
 * @rx_ring:    rx descriptor ring (for a specific queue) to setup
 * Returns 0 on success, negative on failure
 **/
int ngbe_setup_rx_resources(struct ngbe_ring *rx_ring)
{
	struct device *dev = rx_ring->dev;
	int orig_node = dev_to_node(dev);
	int numa_node = -1;
	int size;

	size = sizeof(struct ngbe_rx_buffer) * rx_ring->count;

	if (rx_ring->q_vector)
		numa_node = rx_ring->q_vector->numa_node;

	rx_ring->rx_buffer_info = vzalloc_node(size, numa_node);
	if (!rx_ring->rx_buffer_info)
		rx_ring->rx_buffer_info = vzalloc(size);
	if (!rx_ring->rx_buffer_info)
		goto err;

	/* Round up to nearest 4K */
	rx_ring->size = rx_ring->count * sizeof(union ngbe_rx_desc);
	rx_ring->size = ALIGN(rx_ring->size, 4096);

	set_dev_node(dev, numa_node);
	rx_ring->desc = dma_alloc_coherent(dev, rx_ring->size, &rx_ring->dma, GFP_KERNEL);
	set_dev_node(dev, orig_node);
	if (!rx_ring->desc)
		rx_ring->desc = dma_alloc_coherent(dev, rx_ring->size, &rx_ring->dma, GFP_KERNEL);
	if (!rx_ring->desc)
		goto err;

	if (!rx_ring->q_vector)
		return 0;
	/* XDP RX-queue info */
	if (xdp_rxq_info_reg(&rx_ring->xdp_rxq, rx_ring->netdev, rx_ring->queue_index,
			     rx_ring->q_vector->napi.napi_id < 0))
		goto err;
	rx_ring->xdp_prog = rx_ring->q_vector->adapter->xdp_prog;

	return 0;
err:
	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;
	dev_err(dev, "Unable to allocate memory for the Rx descriptor ring\n");
	return -ENOMEM;
}

/**
 * ngbe_setup_all_rx_resources - allocate all queues Rx resources
 * @adapter: board private structure
 * If this function returns with an error, then it's possible one or
 * more of the rings is populated (while the rest are not).  It is the
 * callers duty to clean those orphaned rings.
 * Return 0 on success, negative on failure
 **/
static int ngbe_setup_all_rx_resources(struct ngbe_adapter *adapter)
{
	int i, err = 0;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		err = ngbe_setup_rx_resources(adapter->rx_ring[i]);
		if (!err)
			continue;

		e_err(probe, "Allocation for Rx Queue %u failed\n", i);
		goto err_setup_rx;
	}

	return 0;
err_setup_rx:
	/* rewind the index freeing the rings as we go */
	while (i--)
		ngbe_free_rx_resources(adapter->rx_ring[i]);
	return err;
}

/**
 * ngbe_setup_isb_resources - allocate interrupt status resources
 * @adapter: board private structure
 * Return 0 on success, negative on failure
 **/
static int ngbe_setup_isb_resources(struct ngbe_adapter *adapter)
{
	struct device *dev = pci_dev_to_dev(adapter->pdev);

	adapter->isb_mem =
		dma_alloc_coherent(dev, sizeof(u32) * NGBE_ISB_MAX, &adapter->isb_dma, GFP_KERNEL);
	if (!adapter->isb_mem) {
		e_err(probe, "%s: alloc isb_mem failed\n", __func__);
		return -ENOMEM;
	}
	memset(adapter->isb_mem, 0, sizeof(u32) * NGBE_ISB_MAX);
	return 0;
}

/**
 * ngbe_free_isb_resources - allocate all queues Rx resources
 * @adapter: board private structure
 * Return 0 on success, negative on failure
 **/
static void ngbe_free_isb_resources(struct ngbe_adapter *adapter)
{
	struct device *dev = pci_dev_to_dev(adapter->pdev);

	dma_free_coherent(dev, sizeof(u32) * NGBE_ISB_MAX, adapter->isb_mem, adapter->isb_dma);
	adapter->isb_mem = NULL;
}

/**
 * ngbe_free_tx_resources - Free Tx Resources per Queue
 * @tx_ring: Tx descriptor ring for a specific queue
 * Free all transmit software resources
 **/
void ngbe_free_tx_resources(struct ngbe_ring *tx_ring)
{
	ngbe_clean_tx_ring(tx_ring);

	vfree(tx_ring->tx_buffer_info);
	tx_ring->tx_buffer_info = NULL;

	/* if not set, then don't free */
	if (!tx_ring->desc)
		return;

	dma_free_coherent(tx_ring->dev, tx_ring->size, tx_ring->desc, tx_ring->dma);
	tx_ring->desc = NULL;
}

/**
 * ngbe_free_all_tx_resources - Free Tx Resources for All Queues
 * @adapter: board private structure
 * Free all transmit software resources
 **/
static void ngbe_free_all_tx_resources(struct ngbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++)
		ngbe_free_tx_resources(adapter->tx_ring[i]);
	for (i = 0; i < adapter->num_xdp_queues; i++)
		ngbe_free_tx_resources(adapter->xdp_ring[i]);
}

/**
 * ngbe_free_rx_resources - Free Rx Resources
 * @rx_ring: ring to clean the resources from
 * Free all receive software resources
 **/
void ngbe_free_rx_resources(struct ngbe_ring *rx_ring)
{
	ngbe_clean_rx_ring(rx_ring);
	rx_ring->xdp_prog = NULL;

	if (rx_ring->q_vector)
		xdp_rxq_info_unreg(&rx_ring->xdp_rxq);
	vfree(rx_ring->rx_buffer_info);
	rx_ring->rx_buffer_info = NULL;

	/* if not set, then don't free */
	if (!rx_ring->desc)
		return;

	dma_free_coherent(rx_ring->dev, rx_ring->size, rx_ring->desc, rx_ring->dma);

	rx_ring->desc = NULL;
}

/**
 * ngbe_free_all_rx_resources - Free Rx Resources for All Queues
 * @adapter: board private structure
 * Free all receive software resources
 **/
static void ngbe_free_all_rx_resources(struct ngbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_rx_queues; i++)
		ngbe_free_rx_resources(adapter->rx_ring[i]);
}

/**
 * ngbe_change_mtu - Change the Maximum Transfer Unit
 * @netdev: network interface device structure
 * @new_mtu: new value for maximum frame size
 * Returns 0 on success, negative on failure
 **/
static int ngbe_change_mtu(struct net_device *netdev, int new_mtu)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	if (new_mtu < 68 || new_mtu > 9414)
		return -EINVAL;

	if (adapter->xdp_prog) {
		int new_frame_size = new_mtu + ETH_HLEN + ETH_FCS_LEN + VLAN_HLEN;
		int i;

		for (i = 0; i < adapter->num_rx_queues; i++) {
			struct ngbe_ring *ring = adapter->rx_ring[i];

			if (new_frame_size > ngbe_rx_bufsz(ring)) {
				e_warn(probe, "Requested MTU size is not supported with XDP\n");
				return -EINVAL;
			}
		}
	}
	/* we cannot allow legacy VFs to enable their receive
	 * paths when MTU greater than 1500 is configured.  So display a
	 * warning that legacy VFs will be disabled.
	 */
	if (adapter->flags & NGBE_FLAG_SRIOV_ENABLED && new_mtu > ETH_DATA_LEN)
		e_warn(probe, "Setting MTU > 1500 will disable legacy VFs\n");

	e_info(probe, "changing MTU from %d to %d\n", netdev->mtu, new_mtu);

	/* must set new MTU before calling down or up */
	netdev->mtu = new_mtu;

	if (netif_running(netdev))
		ngbe_reinit_locked(adapter);

	return 0;
}

/**
 * ngbe_open - Called when a network interface is made active
 * @netdev: network interface device structure
 * Returns 0 on success, negative value on failure
 * The open entry point is called when a network interface is made
 * active by the system (IFF_UP).  At this point all resources needed
 * for transmit and receive operations are allocated, the interrupt
 * handler is registered with the OS, the watchdog timer is started,
 * and the stack is notified that the interface is ready.
 **/
int ngbe_open(struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	int err;

	/* disallow open during test */
	if (test_bit(__NGBE_TESTING, &adapter->state))
		return -EBUSY;

	netif_carrier_off(netdev);

	/* allocate transmit descriptors */
	err = ngbe_setup_all_tx_resources(adapter);
	if (err)
		goto err_setup_tx;

	/* allocate receive descriptors */
	err = ngbe_setup_all_rx_resources(adapter);
	if (err)
		goto err_setup_rx;

	err = ngbe_setup_isb_resources(adapter);
	if (err)
		goto err_req_isb;

	ngbe_configure(adapter);

	err = ngbe_request_irq(adapter);
	if (err)
		goto err_req_irq;

	if (adapter->num_tx_queues) {
		/* Notify the stack of the actual queue counts. */
		err = netif_set_real_num_tx_queues(netdev, adapter->num_vmdqs > 1
								   ? adapter->queues_per_pool
								   : adapter->num_tx_queues);
		if (err)
			goto err_set_queues;
	}

	if (adapter->num_rx_queues) {
		err = netif_set_real_num_rx_queues(netdev, adapter->num_vmdqs > 1
								   ? adapter->queues_per_pool
								   : adapter->num_rx_queues);
		if (err)
			goto err_set_queues;
	}

	ngbe_ptp_init(adapter);

	ngbe_up_complete(adapter);

	return 0;

err_set_queues:
	ngbe_free_irq(adapter);
err_req_irq:
	ngbe_free_isb_resources(adapter);
err_req_isb:
	ngbe_free_all_rx_resources(adapter);

err_setup_rx:
	ngbe_free_all_tx_resources(adapter);
err_setup_tx:
	ngbe_reset(adapter);
	return err;
}

/**
 * ngbe_close_suspend - actions necessary to both suspend and close flows
 * @adapter: the private adapter struct
 * This function should contain the necessary work common to both suspending
 * and closing of the device.
 */
static void ngbe_close_suspend(struct ngbe_adapter *adapter)
{
	ngbe_ptp_suspend(adapter);
	ngbe_disable_device(adapter);

	ngbe_clean_all_tx_rings(adapter);
	ngbe_clean_all_rx_rings(adapter);

	ngbe_free_irq(adapter);

	ngbe_free_isb_resources(adapter);
	ngbe_free_all_rx_resources(adapter);
	ngbe_free_all_tx_resources(adapter);
}

/**
 * ngbe_close - Disables a network interface
 * @netdev: network interface device structure
 * Returns 0, this is not allowed to fail
 * The close entry point is called when an interface is de-activated
 * by the OS.  The hardware is still under the drivers control, but
 * needs to be disabled.  A global MAC reset is issued to stop the
 * hardware, and all transmit and receive resources are freed.
 **/
int ngbe_close(struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	ngbe_ptp_stop(adapter);

	ngbe_down(adapter);
	ngbe_free_irq(adapter);

	ngbe_free_isb_resources(adapter);
	ngbe_free_all_rx_resources(adapter);
	ngbe_free_all_tx_resources(adapter);

	ngbe_release_hw_control(adapter);

	return 0;
}

static int ngbe_resume(struct device *dev)
{
	struct ngbe_adapter *adapter;
	struct net_device *netdev;
	u32 err;
	struct pci_dev *pdev = to_pci_dev(dev);

	adapter = pci_get_drvdata(pdev);
	netdev = adapter->netdev;
	adapter->hw.hw_addr = adapter->io_addr;
	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	/* pci_restore_state clears dev->state_saved so call
	 * pci_save_state to restore it.
	 */
	pci_save_state(pdev);
	wr32(&adapter->hw, NGBE_PSR_WKUP_CTL, adapter->wol);

	err = pci_enable_device_mem(pdev);
	if (err) {
		e_dev_err("Cannot enable PCI device from suspend\n");
		return err;
	}
	/* make sure to clear disable flag */
	smp_mb__before_atomic();
	clear_bit(__NGBE_DISABLED, &adapter->state);
	pci_set_master(pdev);

	pci_wake_from_d3(pdev, false);

	ngbe_reset(adapter);

	rtnl_lock();

	err = ngbe_init_interrupt_scheme(adapter);
	if (!err && netif_running(netdev))
		err = ngbe_open(netdev);

	rtnl_unlock();

	if (err)
		return err;

	netif_device_attach(netdev);

	return 0;
}

/**
 * ngbe_freeze - quiesce the device (no IRQ's or DMA)
 * @dev: The port's netdev
 */
static int ngbe_freeze(struct device *dev)
{
	struct ngbe_adapter *adapter = pci_get_drvdata(to_pci_dev(dev));
	struct net_device *netdev = adapter->netdev;

	netif_device_detach(netdev);

	if (netif_running(netdev)) {
		ngbe_down(adapter);
		ngbe_free_irq(adapter);
	}

	ngbe_reset_interrupt_capability(adapter);

	return 0;
}

/**
 * ngbe_thaw - un-quiesce the device
 * @dev: The port's netdev
 */
static int ngbe_thaw(struct device *dev)
{
	struct ngbe_adapter *adapter = pci_get_drvdata(to_pci_dev(dev));
	struct net_device *netdev = adapter->netdev;

	ngbe_set_interrupt_capability(adapter);

	if (netif_running(netdev)) {
		u32 err = ngbe_request_irq(adapter);

		if (err)
			return err;

		ngbe_up(adapter);
	}

	netif_device_attach(netdev);

	return 0;
}

/* __ngbe_shutdown is not used when power manangbeent
 * is disabled on older kernels (<2.6.12). causes a compile
 * warning/error, because it is defined and not used.
 */
static int __ngbe_shutdown(struct pci_dev *pdev, bool *enable_wake)
{
	struct ngbe_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;
	struct ngbe_hw *hw = &adapter->hw;
	u32 wufc = adapter->wol;
	int retval = 0;

	netif_device_detach(netdev);
	ngbe_mac_set_default_filter(adapter, hw->mac.perm_addr);

	rtnl_lock();
	if (netif_running(netdev))
		ngbe_close_suspend(adapter);
	rtnl_unlock();

	ngbe_clear_interrupt_scheme(adapter);

	retval = pci_save_state(pdev);
	if (retval)
		return retval;

	/* this won't stop link of managebility or WoL is enabled */
	ngbe_stop_mac_link_on_d3(hw);

	if (wufc) {
		ngbe_set_rx_mode(netdev);
		ngbe_configure_rx(adapter);

		/* turn on all-multi mode if wake on multicast is enabled */
		if (wufc & NGBE_PSR_WKUP_CTL_MC)
			wr32m(hw, NGBE_PSR_CTL, NGBE_PSR_CTL_MPE, NGBE_PSR_CTL_MPE);

		pci_clear_master(adapter->pdev);
		wr32(hw, NGBE_PSR_WKUP_CTL, wufc);
	} else {
		wr32(hw, NGBE_PSR_WKUP_CTL, 0);
	}

	pci_wake_from_d3(pdev, !!wufc);

	*enable_wake = !!wufc;
	ngbe_release_hw_control(adapter);

	if (!test_and_set_bit(__NGBE_DISABLED, &adapter->state))
		pci_disable_device(pdev);

	return 0;
}

static int ngbe_suspend(struct device *dev)
{
	int retval;
	bool wake;
	struct pci_dev *pdev = to_pci_dev(dev);

	retval = __ngbe_shutdown(pdev, &wake);
	if (retval)
		return retval;

	if (wake) {
		pci_prepare_to_sleep(pdev);
	} else {
		pci_wake_from_d3(pdev, false);
		pci_set_power_state(pdev, PCI_D3hot);
	}

	return 0;
}

static void ngbe_shutdown(struct pci_dev *pdev)
{
	bool wake = 0;

	__ngbe_shutdown(pdev, &wake);

	if (system_state == SYSTEM_POWER_OFF) {
		pci_wake_from_d3(pdev, wake);
		pci_set_power_state(pdev, PCI_D3hot);
	}
}

/**
 * ngbe_get_stats64 - Get System Network Statistics
 * @netdev: network interface device structure
 * @stats: storage space for 64bit statistics
 * Returns 64bit statistics, for use in the ndo_get_stats64 callback. This
 * function replaces ngbe_get_stats for kernels which support it.
 */
static void ngbe_get_stats64(struct net_device *netdev, struct rtnl_link_stats64 *stats)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	int i;

	rcu_read_lock();
	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct ngbe_ring *ring = READ_ONCE(adapter->rx_ring[i]);
		u64 bytes, packets;
		unsigned int start;

		if (ring) {
			do {
				start = u64_stats_fetch_begin(&ring->syncp);
				packets = ring->stats.packets;
				bytes = ring->stats.bytes;
			} while (u64_stats_fetch_retry(&ring->syncp, start));
			stats->rx_packets += packets;
			stats->rx_bytes += bytes;
		}
	}

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct ngbe_ring *ring = READ_ONCE(adapter->tx_ring[i]);
		u64 bytes, packets;
		unsigned int start;

		if (ring) {
			do {
				start = u64_stats_fetch_begin(&ring->syncp);
				packets = ring->stats.packets;
				bytes = ring->stats.bytes;
			} while (u64_stats_fetch_retry(&ring->syncp, start));
			stats->tx_packets += packets;
			stats->tx_bytes += bytes;
		}
	}
	for (i = 0; i < adapter->num_xdp_queues; i++) {
		struct ngbe_ring *ring = READ_ONCE(adapter->xdp_ring[i]);
		u64 bytes, packets;
		unsigned int start;

		if (ring) {
			do {
				start = u64_stats_fetch_begin(&ring->syncp);
				packets = ring->stats.packets;
				bytes = ring->stats.bytes;
			} while (u64_stats_fetch_retry(&ring->syncp, start));
			stats->tx_packets += packets;
			stats->tx_bytes += bytes;
		}
	}
	rcu_read_unlock();
	/* following stats updated by ngbe_watchdog_task() */
	stats->multicast = netdev->stats.multicast;
	stats->rx_errors = netdev->stats.rx_errors;
	stats->rx_length_errors = netdev->stats.rx_length_errors;
	stats->rx_crc_errors = netdev->stats.rx_crc_errors;
	stats->rx_missed_errors = netdev->stats.rx_missed_errors;
}

/**
 * ngbe_update_stats - Update the board statistics counters.
 * @adapter: board private structure
 **/
void ngbe_update_stats(struct ngbe_adapter *adapter)
{
	struct net_device_stats *net_stats = &adapter->netdev->stats;
	struct ngbe_hw *hw = &adapter->hw;
	struct ngbe_hw_stats *hwstats = &adapter->stats;
	u64 total_mpc = 0;
	u32 i, bprc, lxon, lxoff;
	u64 non_eop_descs = 0, restart_queue = 0, tx_busy = 0;
	u64 alloc_rx_page_failed = 0, alloc_rx_buff_failed = 0;
	u64 bytes = 0, packets = 0, hw_csum_rx_error = 0;
	u64 hw_csum_rx_good = 0;

	if (test_bit(__NGBE_DOWN, &adapter->state) || test_bit(__NGBE_RESETTING, &adapter->state))
		return;

	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct ngbe_ring *rx_ring = adapter->rx_ring[i];

		non_eop_descs += rx_ring->rx_stats.non_eop_descs;
		alloc_rx_page_failed += rx_ring->rx_stats.alloc_rx_page_failed;
		alloc_rx_buff_failed += rx_ring->rx_stats.alloc_rx_buff_failed;
		hw_csum_rx_error += rx_ring->rx_stats.csum_err;
		hw_csum_rx_good += rx_ring->rx_stats.csum_good_cnt;
		bytes += rx_ring->stats.bytes;
		packets += rx_ring->stats.packets;
	}

	adapter->non_eop_descs = non_eop_descs;
	adapter->alloc_rx_page_failed = alloc_rx_page_failed;
	adapter->alloc_rx_buff_failed = alloc_rx_buff_failed;
	adapter->hw_csum_rx_error = hw_csum_rx_error;
	adapter->hw_csum_rx_good = hw_csum_rx_good;
	net_stats->rx_bytes = bytes;
	net_stats->rx_packets = packets;

	bytes = 0;
	packets = 0;
	/* gather some stats to the adapter struct that are per queue */
	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct ngbe_ring *tx_ring = adapter->tx_ring[i];

		restart_queue += tx_ring->tx_stats.restart_queue;
		tx_busy += tx_ring->tx_stats.tx_busy;
		bytes += tx_ring->stats.bytes;
		packets += tx_ring->stats.packets;
	}
	for (i = 0; i < adapter->num_xdp_queues; i++) {
		struct ngbe_ring *xdp_ring = adapter->xdp_ring[i];

		restart_queue += xdp_ring->tx_stats.restart_queue;
		tx_busy += xdp_ring->tx_stats.tx_busy;
		bytes += xdp_ring->stats.bytes;
		packets += xdp_ring->stats.packets;
	}
	adapter->restart_queue = restart_queue;
	adapter->tx_busy = tx_busy;
	net_stats->tx_bytes = bytes;
	net_stats->tx_packets = packets;

	hwstats->crcerrs += rd32(hw, NGBE_RX_CRC_ERROR_FRAMES_LOW);

	hwstats->gprc += rd32(hw, NGBE_PX_GPRC);

	ngbe_update_xoff_rx_lfc(adapter);

	hwstats->o2bgptc += rd32(hw, NGBE_TDM_OS2BMC_CNT);
	if (ngbe_check_mng_access(&adapter->hw)) {
		hwstats->o2bspc += rd32(hw, NGBE_MNG_OS2BMC_CNT);
		hwstats->b2ospc += rd32(hw, NGBE_MNG_BMC2OS_CNT);
	}
	hwstats->b2ogprc += rd32(hw, NGBE_RDM_BMC2OS_CNT);
	hwstats->gorc += rd32(hw, NGBE_PX_GORC_LSB);
	hwstats->gorc += (u64)rd32(hw, NGBE_PX_GORC_MSB) << 32;

	hwstats->gotc += rd32(hw, NGBE_PX_GOTC_LSB);
	hwstats->gotc += (u64)rd32(hw, NGBE_PX_GOTC_MSB) << 32;

	adapter->hw_rx_no_dma_resources += rd32(hw, NGBE_RDM_DRP_PKT);
	bprc = rd32(hw, NGBE_RX_BC_FRAMES_GOOD_LOW);
	hwstats->bprc += bprc;
	hwstats->mprc = 0;
	hwstats->rdpc += rd32(hw, NGBE_RDB_PKT_CNT);
	hwstats->rddc += rd32(hw, NGBE_RDB_DRP_CNT);
	hwstats->psrpc += rd32(hw, NGBE_PSR_PKT_CNT);
	hwstats->psrdc += rd32(hw, NGBE_PSR_DBG_DRP_CNT);
	hwstats->untag += rd32(hw, NGBE_RSEC_LSEC_UNTAG_PKT);
	hwstats->tdmpc += rd32(hw, NGBE_TDM_PKT_CNT);
	hwstats->tdmdc += rd32(hw, NGBE_TDM_DRP_CNT);
	hwstats->tdbpc += rd32(hw, NGBE_TDB_OUT_PKT_CNT);

	for (i = adapter->num_vfs; i < 8; i++)
		hwstats->mprc += rd32(hw, NGBE_PX_MPRC(i));

	hwstats->roc += rd32(hw, NGBE_RX_OVERSIZE_FRAMES_GOOD);
	hwstats->rlec += rd32(hw, NGBE_RX_LEN_ERROR_FRAMES_LOW);
	lxon = rd32(hw, NGBE_RDB_LXONTXC);
	hwstats->lxontxc += lxon;
	lxoff = rd32(hw, NGBE_RDB_LXOFFTXC);
	hwstats->lxofftxc += lxoff;

	hwstats->gptc += rd32(hw, NGBE_PX_GPTC);
	hwstats->mptc += rd32(hw, NGBE_TX_MC_FRAMES_GOOD_LOW);
	hwstats->ruc += rd32(hw, NGBE_RX_UNDERSIZE_FRAMES_GOOD);
	hwstats->tpr += rd32(hw, NGBE_RX_FRAME_CNT_GOOD_BAD_LOW);
	hwstats->bptc += rd32(hw, NGBE_TX_BC_FRAMES_GOOD_LOW);
	/* Fill out the OS statistics structure */
	net_stats->multicast = hwstats->mprc;

	/* Rx Errors */
	net_stats->rx_errors = hwstats->crcerrs + hwstats->rlec;
	net_stats->rx_dropped = 0;
	net_stats->rx_length_errors = hwstats->rlec;
	net_stats->rx_crc_errors = hwstats->crcerrs;
	total_mpc = rd32(hw, NGBE_RDB_MPCNT);
	net_stats->rx_missed_errors += total_mpc;

	/* VF Stats Collection - skip while resetting because these
	 * are not clear on read and otherwise you'll sometimes get
	 * crazy values.
	 */
	if (!test_bit(__NGBE_RESETTING, &adapter->state)) {
		for (i = 0; i < adapter->num_vfs; i++) {
			UPDATE_VF_COUNTER_32bit(NGBE_VX_GPRC, adapter->vfinfo->last_vfstats.gprc,
						adapter->vfinfo->vfstats.gprc);
			UPDATE_VF_COUNTER_32bit(NGBE_VX_GPTC, adapter->vfinfo->last_vfstats.gptc,
						adapter->vfinfo->vfstats.gptc);
			UPDATE_VF_COUNTER_36bit(NGBE_VX_GORC_LSB, NGBE_VX_GORC_MSB,
						adapter->vfinfo->last_vfstats.gorc,
						adapter->vfinfo->vfstats.gorc);
			UPDATE_VF_COUNTER_36bit(NGBE_VX_GOTC_LSB, NGBE_VX_GOTC_MSB,
						adapter->vfinfo->last_vfstats.gotc,
						adapter->vfinfo->vfstats.gotc);
			UPDATE_VF_COUNTER_32bit(NGBE_VX_MPRC, adapter->vfinfo->last_vfstats.mprc,
						adapter->vfinfo->vfstats.mprc);
		}
	}
}

static void ngbe_irq_rearm_queues(struct ngbe_adapter *adapter, u32 qmask)
{
	u32 mask;

	mask = (qmask & 0xFFFFFFFF);
	wr32(&adapter->hw, NGBE_PX_IMC, mask);
	wr32(&adapter->hw, NGBE_PX_ICS, mask);
}

/**
 * ngbe_check_hang_subtask - check for hung queues and dropped interrupts
 * @adapter - pointer to the device adapter structure
 * This function serves two purposes.  First it strobes the interrupt lines
 * in order to make certain interrupts are occurring.  Secondly it sets the
 * bits needed to check for TX hangs.  As a result we should immediately
 * determine if a hang has occurred.
 */
static void ngbe_check_hang_subtask(struct ngbe_adapter *adapter)
{
	int i;
	u32 eics = 0;

	/* If we're down or resetting, just bail */
	if (test_bit(__NGBE_DOWN, &adapter->state) || test_bit(__NGBE_REMOVING, &adapter->state) ||
	    test_bit(__NGBE_RESETTING, &adapter->state))
		return;

	/* Force detection of hung controller */
	if (netif_carrier_ok(adapter->netdev)) {
		for (i = 0; i < adapter->num_tx_queues; i++)
			set_check_for_tx_hang(adapter->tx_ring[i]);
	}
	if (netif_carrier_ok(adapter->netdev)) {
		for (i = 0; i < adapter->num_xdp_queues; i++)
			set_check_for_tx_hang(adapter->xdp_ring[i]);
	}

	if (adapter->flags & NGBE_FLAG_MSIX_ENABLED) {
		/* get one bit for every active tx/rx interrupt vector */
		for (i = 0; i < adapter->num_q_vectors; i++) {
			struct ngbe_q_vector *qv = adapter->q_vector[i];

			if (qv->rx.ring || qv->tx.ring)
				eics |= BIT_ULL(i);
		}
	}
	/* Cause software interrupt to ensure rings are cleaned */
	ngbe_irq_rearm_queues(adapter, eics);
}

static void ngbe_watchdog_an_complete(struct ngbe_adapter *adapter)
{
	u32 link_speed = 0;
	u32 lan_speed = 0;
	bool link_up = true;
	struct ngbe_hw *hw = &adapter->hw;

	if (!(adapter->flags & NGBE_FLAG_NEED_ANC_CHECK))
		return;

	hw->mac.ops.check_link(hw, &link_speed, &link_up, false);

	adapter->link_speed = link_speed;
	switch (link_speed) {
	case NGBE_LINK_SPEED_100_FULL:
		lan_speed = 1;
		break;
	case NGBE_LINK_SPEED_1GB_FULL:
		lan_speed = 2;
		break;
	case NGBE_LINK_SPEED_10_FULL:
		lan_speed = 0;
		break;
	default:
		break;
	}
	wr32m(hw, NGBE_CFG_LAN_SPEED, 0x3, lan_speed);

	if (link_speed &
	    (NGBE_LINK_SPEED_1GB_FULL | NGBE_LINK_SPEED_100_FULL | NGBE_LINK_SPEED_10_FULL)) {
		wr32(hw, NGBE_MAC_TX_CFG,
		     (rd32(hw, NGBE_MAC_TX_CFG) & ~NGBE_MAC_TX_CFG_SPEED_MASK) |
			     NGBE_MAC_TX_CFG_TE | NGBE_MAC_TX_CFG_SPEED_1G);
	}

	adapter->flags &= ~NGBE_FLAG_NEED_ANC_CHECK;
	adapter->flags |= NGBE_FLAG_NEED_LINK_UPDATE;
}

/**
 * ngbe_watchdog_update_link - update the link status
 * @adapter - pointer to the device adapter structure
 * @link_speed - pointer to a u32 to store the link_speed
 **/
static void ngbe_watchdog_update_link_status(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 link_speed = adapter->link_speed;
	bool link_up = adapter->link_up;
	u32 lan_speed = 0;
	u32 reg;

	if (NGBE_POLL_LINK_STATUS != 1) {
		if (!(adapter->flags & NGBE_FLAG_NEED_LINK_UPDATE))
			return;
	}

	link_speed = NGBE_LINK_SPEED_1GB_FULL;
	link_up = true;

	hw->mac.ops.check_link(hw, &link_speed, &link_up, false);

	if (NGBE_POLL_LINK_STATUS != 1) {
		if (link_up ||
		    time_after(jiffies, (adapter->link_check_timeout + NGBE_TRY_LINK_TIMEOUT))) {
			adapter->flags &= ~NGBE_FLAG_NEED_LINK_UPDATE;
		}
	} else {
		if (adapter->link_up == link_up && adapter->link_speed == link_speed)
			return;
	}

	adapter->link_speed = link_speed;
	switch (link_speed) {
	case NGBE_LINK_SPEED_100_FULL:
		lan_speed = 1;
		break;
	case NGBE_LINK_SPEED_1GB_FULL:
		lan_speed = 2;
		break;
	case NGBE_LINK_SPEED_10_FULL:
		lan_speed = 0;
		break;
	default:
		break;
	}
	wr32m(hw, NGBE_CFG_LAN_SPEED, 0x3, lan_speed);

	if (link_up) {
		hw->mac.ops.fc_enable(hw);
		ngbe_set_rx_drop_en(adapter);
	}

	if (link_up) {
		adapter->last_rx_ptp_check = jiffies;

		if (test_bit(__NGBE_PTP_RUNNING, &adapter->state))
			ngbe_ptp_start_cyclecounter(adapter);

		if (link_speed & (NGBE_LINK_SPEED_1GB_FULL | NGBE_LINK_SPEED_100_FULL |
				  NGBE_LINK_SPEED_10_FULL)) {
			wr32(hw, NGBE_MAC_TX_CFG,
			     (rd32(hw, NGBE_MAC_TX_CFG) & ~NGBE_MAC_TX_CFG_SPEED_MASK) |
				     NGBE_MAC_TX_CFG_TE | NGBE_MAC_TX_CFG_SPEED_1G);
		}

		/* Re configure MAC RX */
		reg = rd32(hw, NGBE_MAC_RX_CFG);
		wr32(hw, NGBE_MAC_RX_CFG, reg);
		wr32(hw, NGBE_MAC_PKT_FLT, NGBE_MAC_PKT_FLT_PR);
		reg = rd32(hw, NGBE_MAC_WDG_TIMEOUT);
		wr32(hw, NGBE_MAC_WDG_TIMEOUT, reg);
	}

	adapter->link_up = link_up;
	/* hw->mac.ops.dmac_config is null*/
	if (hw->mac.ops.dmac_config && hw->mac.dmac_config.watchdog_timer) {
		u8 num_tcs = netdev_get_num_tc(adapter->netdev);

		if (hw->mac.dmac_config.link_speed != link_speed ||
		    hw->mac.dmac_config.num_tcs != num_tcs) {
			hw->mac.dmac_config.link_speed = link_speed;
			hw->mac.dmac_config.num_tcs = num_tcs;
			hw->mac.ops.dmac_config(hw);
		}
	}
}

static void ngbe_update_default_up(struct ngbe_adapter *adapter)
{
	u8 up = 0;

	adapter->default_up = up;
}

/**
 * ngbe_watchdog_link_is_up - update netif_carrier status and
 *                             print link up message
 * @adapter - pointer to the device adapter structure
 **/
static void ngbe_watchdog_link_is_up(struct ngbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct ngbe_hw *hw = &adapter->hw;
	u32 link_speed = adapter->link_speed;
	bool flow_rx, flow_tx;

	if (test_bit(__NGBE_HANGING, &adapter->state))
		return;

	/* only continue if link was previously down */
	if (netif_carrier_ok(netdev))
		return;

	adapter->flags2 &= ~NGBE_FLAG2_SEARCH_FOR_SFP;

	/* flow_rx, flow_tx report link flow control status */
	flow_rx = (rd32(hw, NGBE_MAC_RX_FLOW_CTRL) & 0x101) == 0x1;
	flow_tx = !!(NGBE_RDB_RFCC_RFCE_802_3X & rd32(hw, NGBE_RDB_RFCC));

	e_info(drv, "NIC Link is Up %s, Flow Control: %s\n",
	       (link_speed == NGBE_LINK_SPEED_1GB_FULL
			? "1 Gbps"
			: (link_speed == NGBE_LINK_SPEED_100_FULL
				   ? "100 Mbps"
				   : (link_speed == NGBE_LINK_SPEED_10_FULL ? "10 Mbps"
									    : "unknown speed"))),
	       ((flow_rx && flow_tx) ? "RX/TX" : (flow_rx ? "RX" : (flow_tx ? "TX" : "None"))));

	netif_carrier_on(netdev);

	adapter->speed = adapter->link_speed;

	netif_tx_wake_all_queues(netdev);
	/* update the default user priority for VFs */
	ngbe_update_default_up(adapter);

	/* ping all the active vfs to let them know link has changed */
	ngbe_ping_all_vfs_with_link_status(adapter, adapter->link_up);
}

/**
 * ngbe_watchdog_link_is_down - update netif_carrier status and
 *                               print link down message
 * @adapter - pointer to the adapter structure
 **/
static void ngbe_watchdog_link_is_down(struct ngbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	adapter->link_up = false;
	adapter->link_speed = 0;

	/* only continue if link was up previously */
	if (!netif_carrier_ok(netdev))
		return;

	if (test_bit(__NGBE_PTP_RUNNING, &adapter->state))
		ngbe_ptp_start_cyclecounter(adapter);

	e_info(drv, "NIC Link is Down\n");
	netif_carrier_off(netdev);
	netif_tx_stop_all_queues(netdev);

	/* ping all the active vfs to let them know link has changed */
	ngbe_ping_all_vfs_with_link_status(adapter, adapter->link_up);
}

static bool ngbe_ring_tx_pending(struct ngbe_adapter *adapter)
{
	int i;

	for (i = 0; i < adapter->num_tx_queues; i++) {
		struct ngbe_ring *tx_ring = adapter->tx_ring[i];

		if (tx_ring->next_to_use != tx_ring->next_to_clean)
			return true;
	}
	for (i = 0; i < adapter->num_xdp_queues; i++) {
		struct ngbe_ring *xdp_ring = adapter->xdp_ring[i];

		if (xdp_ring->next_to_use != xdp_ring->next_to_clean)
			return true;
	}

	return false;
}

static bool ngbe_vf_tx_pending(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 q_per_pool = 1;

	u32 i, j;

	if (!adapter->num_vfs)
		return false;

	for (i = 0; i < adapter->num_vfs; i++) {
		for (j = 0; j < q_per_pool; j++) {
			u32 h, t;

			h = rd32(hw, NGBE_PX_TR_RPn(q_per_pool, i, j));
			t = rd32(hw, NGBE_PX_TR_WPn(q_per_pool, i, j));

			if (h != t)
				return true;
		}
	}

	return false;
}

/**
 * ngbe_watchdog_flush_tx - flush queues on link down
 * @adapter - pointer to the device adapter structure
 **/
static void ngbe_watchdog_flush_tx(struct ngbe_adapter *adapter)
{
	if (!netif_carrier_ok(adapter->netdev)) {
		if (ngbe_ring_tx_pending(adapter) || ngbe_vf_tx_pending(adapter)) {
			/* We've lost link, so the controller stops DMA,
			 * but we've got queued Tx work that's never going
			 * to get done, so reset controller to flush Tx.
			 * (Do the reset outside of interrupt context).
			 */
			e_warn(drv, "initiating reset due to lost link with pending Tx work\n");
			adapter->flags2 |= NGBE_FLAG2_PF_RESET_REQUESTED;
		}
	}
}

#ifdef CONFIG_PCI_IOV
static inline void ngbe_issue_vf_flr(struct ngbe_adapter *adapter, struct pci_dev *vfdev)
{
	int pos, i;
	u16 status;

	/* wait for pending transactions on the bus */
	for (i = 0; i < 4; i++) {
		if (i)
			msleep((1 << (i - 1)) * 100);

		pcie_capability_read_word(vfdev, PCI_EXP_DEVSTA, &status);
		if (!(status & PCI_EXP_DEVSTA_TRPND))
			goto clear;
	}

	e_dev_warn("Issuing VFLR with pending transactions\n");

clear:
	pos = pci_find_capability(vfdev, PCI_CAP_ID_EXP);
	if (!pos)
		return;

	e_dev_err("Issuing VFLR for VF %s\n", pci_name(vfdev));
	pci_write_config_word(vfdev, pos + PCI_EXP_DEVCTL, PCI_EXP_DEVCTL_BCR_FLR);
	msleep(100);
}

static void ngbe_spoof_check(struct ngbe_adapter *adapter)
{
	u32 ssvpc;

	/* Do not perform spoof check if in non-IOV mode */
	if (adapter->num_vfs == 0)
		return;
	ssvpc = rd32(&adapter->hw, NGBE_TDM_SEC_DRP);

	/* ssvpc register is cleared on read, if zero then no
	 * spoofed packets in the last interval.
	 */
	if (!ssvpc)
		return;

	e_warn(drv, "%d Spoofed packets detected\n", ssvpc);
}

#endif /* CONFIG_PCI_IOV */

/**
 * ngbe_watchdog_subtask - check and bring link up
 * @adapter - pointer to the device adapter structure
 **/
static void ngbe_watchdog_subtask(struct ngbe_adapter *adapter)
{
	/* if interface is down do nothing */
	if (test_bit(__NGBE_DOWN, &adapter->state) || test_bit(__NGBE_REMOVING, &adapter->state) ||
	    test_bit(__NGBE_RESETTING, &adapter->state))
		return;

	ngbe_watchdog_an_complete(adapter);

	if (NGBE_POLL_LINK_STATUS != 1) {
		ngbe_watchdog_update_link_status(adapter);

		if (adapter->link_up)
			ngbe_watchdog_link_is_up(adapter);
		else
			ngbe_watchdog_link_is_down(adapter);
	}
#ifdef CONFIG_PCI_IOV
	ngbe_spoof_check(adapter);
#endif /* CONFIG_PCI_IOV */

	ngbe_update_stats(adapter);

	ngbe_watchdog_flush_tx(adapter);
}

/**
 * ngbe_service_timer - Timer Call-back
 * @data: pointer to adapter cast into an unsigned long
 **/
static void ngbe_service_timer(struct timer_list *t)
{
	struct ngbe_adapter *adapter = from_timer(adapter, t, service_timer);
	unsigned long next_event_offset;
	struct ngbe_hw *hw = &adapter->hw;
	u32 val = 0;

	/* poll faster when waiting for link */
	if ((adapter->flags & NGBE_FLAG_NEED_LINK_UPDATE) ||
	    (adapter->flags & NGBE_FLAG_NEED_ANC_CHECK))
		next_event_offset = HZ / 10;
	else
		next_event_offset = HZ * 2;

	/* flags to records which func to handle pcie recovery */
	if (rd32(&adapter->hw, NGBE_MIS_PF_SM) == 1) {
		val = rd32m(&adapter->hw, NGBE_MIS_PRB_CTL,
			    NGBE_MIS_PRB_CTL_LAN0_UP | NGBE_MIS_PRB_CTL_LAN1_UP |
				    NGBE_MIS_PRB_CTL_LAN2_UP | NGBE_MIS_PRB_CTL_LAN3_UP);
		if (val & NGBE_MIS_PRB_CTL_LAN0_UP) {
			if (hw->bus.lan_id == 0) {
				adapter->flags2 |= NGBE_FLAG2_PCIE_NEED_RECOVER;
				e_info(probe, "%s: set recover on Lan0\n", __func__);
			}
		} else if (val & NGBE_MIS_PRB_CTL_LAN1_UP) {
			if (hw->bus.lan_id == 1) {
				adapter->flags2 |= NGBE_FLAG2_PCIE_NEED_RECOVER;
				e_info(probe, "%s: set recover on Lan1\n", __func__);
			}
		} else if (val & NGBE_MIS_PRB_CTL_LAN2_UP) {
			if (hw->bus.lan_id == 2) {
				adapter->flags2 |= NGBE_FLAG2_PCIE_NEED_RECOVER;
				e_info(probe, "%s: set recover on Lan2\n", __func__);
			}
		} else if (val & NGBE_MIS_PRB_CTL_LAN3_UP) {
			if (hw->bus.lan_id == 3) {
				adapter->flags2 |= NGBE_FLAG2_PCIE_NEED_RECOVER;
				e_info(probe, "%s: set recover on Lan3\n", __func__);
			}
		}
	}

	/* Reset the timer */
	mod_timer(&adapter->service_timer, next_event_offset + jiffies);

	ngbe_service_event_schedule(adapter);
}

static void ngbe_link_check_timer(struct timer_list *t)
{
	struct ngbe_adapter *adapter = from_timer(adapter, t, link_check_timer);
	unsigned long next_event_offset = HZ / 1000;

	if (adapter->hw.phy.type == ngbe_phy_yt8521s_sfi)
		next_event_offset = HZ / 10;

	mod_timer(&adapter->link_check_timer, next_event_offset + jiffies);
	/* if interface is down do nothing */
	if (test_bit(__NGBE_DOWN, &adapter->state) || test_bit(__NGBE_REMOVING, &adapter->state) ||
	    test_bit(__NGBE_RESETTING, &adapter->state))
		return;

	ngbe_watchdog_update_link_status(adapter);

	if (adapter->link_up)
		ngbe_watchdog_link_is_up(adapter);
	else
		ngbe_watchdog_link_is_down(adapter);
}

static void ngbe_reset_subtask(struct ngbe_adapter *adapter)
{
	u32 reset_flag = 0;
	u32 value = 0;

	if (!(adapter->flags2 &
	      (NGBE_FLAG2_PF_RESET_REQUESTED | NGBE_FLAG2_DEV_RESET_REQUESTED |
	       NGBE_FLAG2_GLOBAL_RESET_REQUESTED | NGBE_FLAG2_RESET_INTR_RECEIVED)))
		return;

	/* If we're already down, just bail */
	if (test_bit(__NGBE_DOWN, &adapter->state) || test_bit(__NGBE_REMOVING, &adapter->state))
		return;

	netdev_err(adapter->netdev, "Reset adapter\n");
	adapter->tx_timeout_count++;

	rtnl_lock();
	if (adapter->flags2 & NGBE_FLAG2_GLOBAL_RESET_REQUESTED) {
		reset_flag |= NGBE_FLAG2_GLOBAL_RESET_REQUESTED;
		adapter->flags2 &= ~NGBE_FLAG2_GLOBAL_RESET_REQUESTED;
	}
	if (adapter->flags2 & NGBE_FLAG2_DEV_RESET_REQUESTED) {
		reset_flag |= NGBE_FLAG2_DEV_RESET_REQUESTED;
		adapter->flags2 &= ~NGBE_FLAG2_DEV_RESET_REQUESTED;
	}
	if (adapter->flags2 & NGBE_FLAG2_PF_RESET_REQUESTED) {
		reset_flag |= NGBE_FLAG2_PF_RESET_REQUESTED;
		adapter->flags2 &= ~NGBE_FLAG2_PF_RESET_REQUESTED;
	}

	if (adapter->flags2 & NGBE_FLAG2_RESET_INTR_RECEIVED) {
		/* If there's a recovery already waiting, it takes
		 * precedence before starting a new reset sequence.
		 */
		adapter->flags2 &= ~NGBE_FLAG2_RESET_INTR_RECEIVED;
		value = rd32m(&adapter->hw, NGBE_MIS_RST_ST, NGBE_MIS_RST_ST_DEV_RST_TYPE_MASK) >>
			NGBE_MIS_RST_ST_DEV_RST_TYPE_SHIFT;
		if (value == NGBE_MIS_RST_ST_DEV_RST_TYPE_SW_RST)
			adapter->hw.reset_type = NGBE_SW_RESET;
		else if (value == NGBE_MIS_RST_ST_DEV_RST_TYPE_GLOBAL_RST)
			adapter->hw.reset_type = NGBE_GLOBAL_RESET;
		adapter->hw.force_full_reset = true;
		ngbe_reinit_locked(adapter);
		adapter->hw.force_full_reset = false;
		goto unlock;
	}

	if (reset_flag & NGBE_FLAG2_DEV_RESET_REQUESTED) {
		/* Request a Device Reset
		 *
		 * This will start the chip's countdown to the actual full
		 * chip reset event, and a warning interrupt to be sent
		 * to all PFs, including the requestor.  Our handler
		 * for the warning interrupt will deal with the shutdown
		 * and recovery of the switch setup.
		 */
		/*debug to open*/
		/*ngbe_dump(adapter);*/

		wr32m(&adapter->hw, NGBE_MIS_RST, NGBE_MIS_RST_SW_RST, NGBE_MIS_RST_SW_RST);
		e_info(drv, "%s: sw reset\n", __func__);

	} else if (reset_flag & NGBE_FLAG2_PF_RESET_REQUESTED) {
		/*debug to open*/
		/*ngbe_dump(adapter);*/
		ngbe_reinit_locked(adapter);
	} else if (reset_flag & NGBE_FLAG2_GLOBAL_RESET_REQUESTED) {
		/* Request a Global Reset
		 *
		 * This will start the chip's countdown to the actual full
		 * chip reset event, and a warning interrupt to be sent
		 * to all PFs, including the requestor.  Our handler
		 * for the warning interrupt will deal with the shutdown
		 * and recovery of the switch setup.
		 */
		/*debug to open*/
		/*ngbe_dump(adapter);*/
		pci_save_state(adapter->pdev);
		if (ngbe_mng_present(&adapter->hw)) {
			ngbe_reset_hostif(&adapter->hw);
			e_info(drv, "%s: lan reset\n", __func__);

		} else {
			wr32m(&adapter->hw, NGBE_MIS_RST, NGBE_MIS_RST_GLOBAL_RST,
			      NGBE_MIS_RST_GLOBAL_RST);
			e_info(drv, "%s: global reset\n", __func__);
		}
	}

unlock:
	rtnl_unlock();
}

static void ngbe_check_pcie_subtask(struct ngbe_adapter *adapter)
{
	bool status;

	if (!(adapter->flags2 & NGBE_FLAG2_PCIE_NEED_RECOVER))
		return;

	ngbe_print_tx_hang_status(adapter);
	ngbe_dump_all_ring_desc(adapter);

	wr32m(&adapter->hw, NGBE_MIS_PF_SM, NGBE_MIS_PF_SM_SM, 0);
	if (NGBE_PCIE_RECOVER == 1 && !(adapter->flags & NGBE_FLAG_SRIOV_ENABLED)) {
		status = ngbe_check_recovery_capability(adapter->pdev);
		if (status) {
			e_info(probe, "do recovery\n");
			ngbe_pcie_do_recovery(adapter->pdev);
		} else {
			e_err(drv, "This platform can't support pcie recovery, skip it\n");
		}
	}
	adapter->flags2 &= ~NGBE_FLAG2_PCIE_NEED_RECOVER;
}

/**
 * ngbe_service_task - manages and runs subtasks
 * @work: pointer to work_struct containing our data
 **/
static void ngbe_service_task(struct work_struct *work)
{
	struct ngbe_adapter *adapter = container_of(work, struct ngbe_adapter, service_task);

	if (NGBE_REMOVED(adapter->hw.hw_addr)) {
		if (!test_bit(__NGBE_DOWN, &adapter->state)) {
			rtnl_lock();
			ngbe_down(adapter);
			rtnl_unlock();
		}
		ngbe_service_event_complete(adapter);
		return;
	}

	ngbe_check_pcie_subtask(adapter);
	ngbe_reset_subtask(adapter);
	ngbe_check_overtemp_subtask(adapter);
	ngbe_watchdog_subtask(adapter);
	ngbe_check_hang_subtask(adapter);
	if (test_bit(__NGBE_PTP_RUNNING, &adapter->state)) {
		ngbe_ptp_overflow_check(adapter);
		if (unlikely(adapter->flags & NGBE_FLAG_RX_HWTSTAMP_IN_REGISTER))
			ngbe_ptp_rx_hang(adapter);
	}

	ngbe_service_event_complete(adapter);
}

union network_header {
	struct iphdr *ipv4;
	struct ipv6hdr *ipv6;
	void *raw;
};

static struct ngbe_dec_ptype encode_tx_desc_ptype(const struct ngbe_tx_buffer *first)
{
	struct sk_buff *skb = first->skb;
	unsigned char *exthdr;
	unsigned char *l4_hdr;
	__be16 frag_off;
	u8 l4_prot = 0;
	u8 ptype = 0;

	switch (first->protocol) {
	case htons(ETH_P_IP):
		l4_prot = ip_hdr(skb)->protocol;
		ptype = NGBE_PTYPE_PKT_IP;
		if (ip_hdr(skb)->frag_off & htons(IP_MF | IP_OFFSET)) {
			ptype |= NGBE_PTYPE_TYP_IPFRAG;
			goto exit;
		}
		break;
	case htons(ETH_P_IPV6):
		l4_hdr = skb_transport_header(skb);
		exthdr = skb_network_header(skb) + sizeof(struct ipv6hdr);
		l4_prot = ipv6_hdr(skb)->nexthdr;
		if (l4_hdr != exthdr)
			ipv6_skip_exthdr(skb, exthdr - skb->data, &l4_prot, &frag_off);
		ptype = NGBE_PTYPE_PKT_IP | NGBE_PTYPE_PKT_IPV6;
		if (l4_prot == NEXTHDR_FRAGMENT) {
			ptype |= NGBE_PTYPE_TYP_IPFRAG;
			goto exit;
		}
		break;
	case htons(ETH_P_1588):
		ptype = NGBE_PTYPE_L2_TS;
		goto exit;
	case htons(ETH_P_FIP):
		ptype = NGBE_PTYPE_L2_FIP;
		goto exit;
	case htons(NGBE_ETH_P_LLDP):
		ptype = NGBE_PTYPE_L2_LLDP;
		goto exit;
	case htons(NGBE_ETH_P_CNM):
		ptype = NGBE_PTYPE_L2_CNM;
		goto exit;
	case htons(ETH_P_PAE):
		ptype = NGBE_PTYPE_L2_EAPOL;
		goto exit;
	case htons(ETH_P_ARP):
		ptype = NGBE_PTYPE_L2_ARP;
		goto exit;
	default:
		ptype = NGBE_PTYPE_L2_MAC;
		goto exit;
	}

	switch (l4_prot) {
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
		ptype |= NGBE_PTYPE_TYP_IP;
		break;
	}

exit:
	return ngbe_decode_ptype(ptype);
}

static int ngbe_tso(struct ngbe_ring *tx_ring, struct ngbe_tx_buffer *first, u8 *hdr_len,
		    struct ngbe_dec_ptype dptype)
{
	struct sk_buff *skb = first->skb;
	u32 vlan_macip_lens, type_tucmd;
	u32 mss_l4len_idx, l4len;
	struct tcphdr *tcph;
	struct iphdr *iph;
	u32 tunhdr_eiplen_tunlen = 0;
	struct ipv6hdr *ipv6h;

	if (skb->ip_summed != CHECKSUM_PARTIAL)
		return 0;

	if (!skb_is_gso(skb))
		return 0;

	if (skb_header_cloned(skb)) {
		int err = pskb_expand_head(skb, 0, 0, GFP_ATOMIC);

		if (err)
			return err;
	}

	iph = ip_hdr(skb);
	if (iph->version == 4) {
		tcph = tcp_hdr(skb);
		iph->tot_len = 0;
		iph->check = 0;
		tcph->check = ~csum_tcpudp_magic(iph->saddr, iph->daddr, 0, IPPROTO_TCP, 0);
		first->tx_flags |= NGBE_TX_FLAGS_TSO | NGBE_TX_FLAGS_CSUM | NGBE_TX_FLAGS_IPV4 |
				   NGBE_TX_FLAGS_CC;

	} else if (iph->version == 6 && skb_is_gso_v6(skb)) {
		ipv6h = ipv6_hdr(skb);
		tcph = tcp_hdr(skb);
		ipv6h->payload_len = 0;
		tcph->check = ~csum_ipv6_magic(&ipv6h->saddr, &ipv6h->daddr, 0, IPPROTO_TCP, 0);
		first->tx_flags |= NGBE_TX_FLAGS_TSO | NGBE_TX_FLAGS_CSUM | NGBE_TX_FLAGS_CC;
	}

	/* compute header lengths */
	l4len = tcp_hdrlen(skb);
	*hdr_len = skb_transport_offset(skb) + l4len;

	/* update gso size and bytecount with header size */
	first->gso_segs = skb_shinfo(skb)->gso_segs;
	first->bytecount += (first->gso_segs - 1) * *hdr_len;

	/* mss_l4len_id: use 0 as index for TSO */
	mss_l4len_idx = l4len << NGBE_TXD_L4LEN_SHIFT;
	mss_l4len_idx |= skb_shinfo(skb)->gso_size << NGBE_TXD_MSS_SHIFT;

	/* vlan_macip_lens: HEADLEN, MACLEN, VLAN tag */
	vlan_macip_lens = skb_network_header_len(skb) >> 1;
	vlan_macip_lens |= skb_network_offset(skb) << NGBE_TXD_MACLEN_SHIFT;
	vlan_macip_lens |= first->tx_flags & NGBE_TX_FLAGS_VLAN_MASK;

	type_tucmd = dptype.ptype << 24;
	if (skb->vlan_proto == htons(ETH_P_8021AD))
		type_tucmd |= NGBE_SET_FLAG(first->tx_flags, NGBE_TX_FLAGS_HW_VLAN,
					    0x1 << NGBE_TXD_TAG_TPID_SEL_SHIFT);
	ngbe_tx_ctxtdesc(tx_ring, vlan_macip_lens, tunhdr_eiplen_tunlen, type_tucmd, mss_l4len_idx);

	return 1;
}

static void ngbe_tx_csum(struct ngbe_ring *tx_ring, struct ngbe_tx_buffer *first,
			 struct ngbe_dec_ptype dptype)
{
	struct sk_buff *skb = first->skb;
	u32 vlan_macip_lens = 0;
	u32 mss_l4len_idx = 0;
	u32 tunhdr_eiplen_tunlen = 0;
	u32 type_tucmd;

	if (skb->ip_summed != CHECKSUM_PARTIAL) {
csum_failed:
		if (!(first->tx_flags & NGBE_TX_FLAGS_HW_VLAN) &&
		    !(first->tx_flags & NGBE_TX_FLAGS_CC))
			return;
		vlan_macip_lens = skb_network_offset(skb) << NGBE_TXD_MACLEN_SHIFT;
	} else {
		u8 l4_prot = 0;
		unsigned char *exthdr;
		unsigned char *l4_hdr;
		__be16 frag_off;

		switch (first->protocol) {
		case htons(ETH_P_IP):
			vlan_macip_lens |= skb_network_header_len(skb) >> 1;
			l4_prot = ip_hdr(skb)->protocol;
			break;
		case htons(ETH_P_IPV6):
			vlan_macip_lens |= skb_network_header_len(skb) >> 1;
			l4_hdr = skb_transport_header(skb);
			exthdr = skb_network_header(skb) + sizeof(struct ipv6hdr);
			l4_prot = ipv6_hdr(skb)->nexthdr;
			if (l4_hdr != exthdr)
				ipv6_skip_exthdr(skb, exthdr - skb->data, &l4_prot, &frag_off);
			break;
		default:
			break;
		}

		switch (l4_prot) {
		case IPPROTO_TCP:
			mss_l4len_idx = tcp_hdrlen(skb) << NGBE_TXD_L4LEN_SHIFT;
			break;
		case IPPROTO_SCTP:
			mss_l4len_idx = sizeof(struct sctphdr) << NGBE_TXD_L4LEN_SHIFT;
			break;
		case IPPROTO_UDP:
			mss_l4len_idx = sizeof(struct udphdr) << NGBE_TXD_L4LEN_SHIFT;
			break;
		default:
			skb_checksum_help(skb);
			goto csum_failed;
		}

		/* update TX checksum flag */
		first->tx_flags |= NGBE_TX_FLAGS_CSUM;
	}
	first->tx_flags |= NGBE_TX_FLAGS_CC;
	/* vlan_macip_lens: MACLEN, VLAN tag */
	vlan_macip_lens |= skb_network_offset(skb) << NGBE_TXD_MACLEN_SHIFT;
	vlan_macip_lens |= first->tx_flags & NGBE_TX_FLAGS_VLAN_MASK;

	type_tucmd = dptype.ptype << 24;
	if (skb->vlan_proto == htons(ETH_P_8021AD))
		type_tucmd |= NGBE_SET_FLAG(first->tx_flags, NGBE_TX_FLAGS_HW_VLAN,
					    0x1 << NGBE_TXD_TAG_TPID_SEL_SHIFT);
	ngbe_tx_ctxtdesc(tx_ring, vlan_macip_lens, tunhdr_eiplen_tunlen, type_tucmd, mss_l4len_idx);
}

u32 ngbe_tx_cmd_type(u32 tx_flags)
{
	/* set type for advanced descriptor with frame checksum insertion */
	u32 cmd_type = NGBE_TXD_DTYP_DATA | NGBE_TXD_IFCS;

	/* set HW vlan bit if vlan is present */
	cmd_type |= NGBE_SET_FLAG(tx_flags, NGBE_TX_FLAGS_HW_VLAN, NGBE_TXD_VLE);

	/* set segmentation enable bits for TSO/FSO */
	cmd_type |= NGBE_SET_FLAG(tx_flags, NGBE_TX_FLAGS_TSO, NGBE_TXD_TSE);

	/* set timestamp bit if present */
	cmd_type |= NGBE_SET_FLAG(tx_flags, NGBE_TX_FLAGS_TSTAMP, NGBE_TXD_MAC_TSTAMP);

	cmd_type |= NGBE_SET_FLAG(tx_flags, NGBE_TX_FLAGS_LINKSEC, NGBE_TXD_LINKSEC);

	return cmd_type;
}

static void ngbe_tx_olinfo_status(union ngbe_tx_desc *tx_desc, u32 tx_flags, unsigned int paylen)
{
	u32 olinfo_status = paylen << NGBE_TXD_PAYLEN_SHIFT;

	/* enable L4 checksum for TSO and TX checksum offload */
	olinfo_status |= NGBE_SET_FLAG(tx_flags, NGBE_TX_FLAGS_CSUM, NGBE_TXD_L4CS);

	/* enable IPv4 checksum for TSO */
	olinfo_status |= NGBE_SET_FLAG(tx_flags, NGBE_TX_FLAGS_IPV4, NGBE_TXD_IIPCS);
	/* enable outer IPv4 checksum for TSO */
	olinfo_status |= NGBE_SET_FLAG(tx_flags, NGBE_TX_FLAGS_OUTER_IPV4, NGBE_TXD_EIPCS);
	/* Check Context must be set if Tx switch is enabled, which it
	 * always is for case where virtual functions are running
	 */
	olinfo_status |= NGBE_SET_FLAG(tx_flags, NGBE_TX_FLAGS_CC, NGBE_TXD_CC);

	olinfo_status |= NGBE_SET_FLAG(tx_flags, NGBE_TX_FLAGS_IPSEC, NGBE_TXD_IPSEC);

	tx_desc->read.olinfo_status = cpu_to_le32(olinfo_status);
}

static int __ngbe_maybe_stop_tx(struct ngbe_ring *tx_ring, u16 size)
{
	netif_stop_subqueue(tx_ring->netdev, tx_ring->queue_index);

	/* Herbert's original patch had:
	 *  smp_mb__after_netif_stop_queue();
	 * but since that doesn't exist yet, just open code it.
	 */
	smp_mb();

	/* We need to check again in a case another CPU has just
	 * made room available.
	 */
	if (likely(ngbe_desc_unused(tx_ring) < size))
		return -EBUSY;

	/* A reprieve! - use start_queue because it doesn't call schedule */
	netif_start_subqueue(tx_ring->netdev, tx_ring->queue_index);
	++tx_ring->tx_stats.restart_queue;
	return 0;
}

static inline int ngbe_maybe_stop_tx(struct ngbe_ring *tx_ring, u16 size)
{
	if (likely(ngbe_desc_unused(tx_ring) >= size))
		return 0;

	return __ngbe_maybe_stop_tx(tx_ring, size);
}

static int ngbe_tx_map(struct ngbe_ring *tx_ring, struct ngbe_tx_buffer *first, const u8 hdr_len)
{
	struct sk_buff *skb = first->skb;
	struct ngbe_tx_buffer *tx_buffer;
	union ngbe_tx_desc *tx_desc;
	skb_frag_t *frag;
	dma_addr_t dma;
	unsigned int data_len, size;
	u32 tx_flags = first->tx_flags;
	u32 cmd_type = ngbe_tx_cmd_type(tx_flags);
	u16 i = tx_ring->next_to_use;

	tx_desc = NGBE_TX_DESC(tx_ring, i);

	ngbe_tx_olinfo_status(tx_desc, tx_flags, skb->len - hdr_len);

	size = skb_headlen(skb);
	data_len = skb->data_len;

	dma = dma_map_single(tx_ring->dev, skb->data, size, DMA_TO_DEVICE);

	tx_buffer = first;

	tx_buffer->va = skb->data;

	for (frag = &skb_shinfo(skb)->frags[0];; frag++) {
		if (dma_mapping_error(tx_ring->dev, dma)) {
			tx_buffer->va = NULL;
			goto dma_error;
		}
		/* record length, and DMA address */
		dma_unmap_len_set(tx_buffer, len, size);
		dma_unmap_addr_set(tx_buffer, dma, dma);

		tx_desc->read.buffer_addr = cpu_to_le64(dma);

		while (unlikely(size > NGBE_MAX_DATA_PER_TXD)) {
			tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type ^ NGBE_MAX_DATA_PER_TXD);

			i++;
			tx_desc++;
			if (i == tx_ring->count) {
				tx_desc = NGBE_TX_DESC(tx_ring, 0);
				i = 0;
			}
			tx_desc->read.olinfo_status = 0;

			dma += NGBE_MAX_DATA_PER_TXD;
			size -= NGBE_MAX_DATA_PER_TXD;

			tx_desc->read.buffer_addr = cpu_to_le64(dma);
		}

		if (likely(!data_len))
			break;

		tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type ^ size);

		i++;
		tx_desc++;
		if (i == tx_ring->count) {
			tx_desc = NGBE_TX_DESC(tx_ring, 0);
			i = 0;
		}
		tx_desc->read.olinfo_status = 0;

		size = skb_frag_size(frag);

		data_len -= size;

		dma = skb_frag_dma_map(tx_ring->dev, frag, 0, size, DMA_TO_DEVICE);

		tx_buffer = &tx_ring->tx_buffer_info[i];
		tx_buffer->va = skb_frag_address_safe(frag);
	}

	/* write last descriptor with RS and EOP bits */
	cmd_type |= size | NGBE_TXD_CMD;
	tx_desc->read.cmd_type_len = cpu_to_le32(cmd_type);

	netdev_tx_sent_queue(txring_txq(tx_ring), first->bytecount);

	/* set the timestamp */
	first->time_stamp = jiffies;
	skb_tx_timestamp(skb);

	/* Force memory writes to complete before letting h/w know there
	 * are new descriptors to fetch.  (Only applicable for weak-ordered
	 * memory model archs, such as IA-64).
	 *
	 * We also need this memory barrier to make certain all of the
	 * status bits have been updated before next_to_watch is written.
	 */
	wmb();

	/* set next_to_watch value indicating a packet is present */
	first->next_to_watch = tx_desc;

	i++;
	if (i == tx_ring->count)
		i = 0;

	tx_ring->next_to_use = i;

	ngbe_maybe_stop_tx(tx_ring, DESC_RESERVED + DESC_NEEDED);

	if (netif_xmit_stopped(txring_txq(tx_ring)) || !netdev_xmit_more() ||
	    (ngbe_desc_unused(tx_ring) <= (tx_ring->count >> 1))) {
		writel(i, tx_ring->tail);
	}

	return 0;
dma_error:
	dev_err(tx_ring->dev, "TX DMA map failed\n");

	/* clear dma mappings for failed tx_buffer_info map */
	for (;;) {
		tx_buffer = &tx_ring->tx_buffer_info[i];
		if (dma_unmap_len(tx_buffer, len))
			dma_unmap_page(tx_ring->dev, dma_unmap_addr(tx_buffer, dma),
				       dma_unmap_len(tx_buffer, len), DMA_TO_DEVICE);
		dma_unmap_len_set(tx_buffer, len, 0);
		tx_buffer->va = NULL;
		if (tx_buffer == first)
			break;
		if (i == 0)
			i += tx_ring->count;
		i--;
	}

	dev_kfree_skb_any(first->skb);
	first->skb = NULL;

	tx_ring->next_to_use = i;

	return -1;
}

#if IS_ENABLED(CONFIG_FCOE)

static u16 ngbe_select_queue(struct net_device *dev, struct sk_buff *skb, struct net_device *sb_dev)
{
	int txq;

	/* only execute the code below if protocol is FCoE
	 * or FIP and we have FCoE enabled on the adapter
	 */
	switch (vlan_get_protocol(skb)) {
	case htons(ETH_P_FIP):
		fallthrough;
	default:
		return netdev_pick_tx(dev, skb, sb_dev);
	}

	txq = skb_rx_queue_recorded(skb) ? skb_get_rx_queue(skb) : smp_processor_id();

	return txq;
}
#endif /* CONFIG_FCOE */

netdev_tx_t ngbe_xmit_frame_ring(struct sk_buff *skb, struct ngbe_adapter __maybe_unused *adapter,
				 struct ngbe_ring *tx_ring)
{
	struct ngbe_tx_buffer *first;
	int tso;
	u32 tx_flags = 0;
	unsigned short f;
	u16 count = TXD_USE_COUNT(skb_headlen(skb));
	__be16 protocol = skb->protocol;
	u8 hdr_len = 0;
	struct ngbe_dec_ptype dptype;

	/* need: 1 descriptor per page * PAGE_SIZE/NGBE_MAX_DATA_PER_TXD,
	 *       + 1 desc for skb_headlen/NGBE_MAX_DATA_PER_TXD,
	 *       + 2 desc gap to keep tail from touching head,
	 *       + 1 desc for context descriptor,
	 * otherwise try next time
	 */
	for (f = 0; f < skb_shinfo(skb)->nr_frags; f++)
		count += TXD_USE_COUNT(skb_frag_size(&skb_shinfo(skb)->frags[f]));

	if (ngbe_maybe_stop_tx(tx_ring, count + DESC_RESERVED + 3)) {
		tx_ring->tx_stats.tx_busy++;
		return NETDEV_TX_BUSY;
	}

	/* record the location of the first descriptor for this packet */
	first = &tx_ring->tx_buffer_info[tx_ring->next_to_use];
	first->skb = skb;
	first->bytecount = skb->len;
	first->gso_segs = 1;

	/* if we have a HW VLAN tag being added default to the HW one */
	if (skb_vlan_tag_present(skb)) {
		tx_flags |= skb_vlan_tag_get(skb) << NGBE_TX_FLAGS_VLAN_SHIFT;
		tx_flags |= NGBE_TX_FLAGS_HW_VLAN;
	}

	if (protocol == htons(ETH_P_8021Q) || protocol == htons(ETH_P_8021AD)) {
		struct vlan_hdr *vhdr, _vhdr;

		vhdr = skb_header_pointer(skb, ETH_HLEN, sizeof(_vhdr), &_vhdr);
		if (!vhdr)
			goto out_drop;

		protocol = vhdr->h_vlan_encapsulated_proto;
		tx_flags |= NGBE_TX_FLAGS_SW_VLAN;
	}

	protocol = vlan_get_protocol(skb);

	if (unlikely(skb_shinfo(skb)->tx_flags & SKBTX_HW_TSTAMP) && adapter->ptp_clock) {
		if (!test_and_set_bit_lock(__NGBE_PTP_TX_IN_PROGRESS, &adapter->state)) {
			skb_shinfo(skb)->tx_flags |= SKBTX_IN_PROGRESS;
			tx_flags |= NGBE_TX_FLAGS_TSTAMP;

			/* schedule check for Tx timestamp */
			adapter->ptp_tx_skb = skb_get(skb);
			adapter->ptp_tx_start = jiffies;
			schedule_work(&adapter->ptp_tx_work);
		} else {
			adapter->tx_hwtstamp_skipped++;
		}
	}

#ifdef CONFIG_PCI_IOV
	/* Use the l2switch_enable flag - would be false if the DMA
	 * Tx switch had been disabled.
	 */
	if (adapter->flags & NGBE_FLAG_SRIOV_L2SWITCH_ENABLE)
		tx_flags |= NGBE_TX_FLAGS_CC;

#endif
	/* record initial flags and protocol */
	first->tx_flags = tx_flags;
	first->protocol = protocol;

	dptype = encode_tx_desc_ptype(first);

	tso = ngbe_tso(tx_ring, first, &hdr_len, dptype);
	if (tso < 0)
		goto out_drop;
	else if (!tso)
		ngbe_tx_csum(tx_ring, first, dptype);

	ngbe_tx_map(tx_ring, first, hdr_len);

	return NETDEV_TX_OK;

out_drop:
	dev_kfree_skb_any(first->skb);
	first->skb = NULL;

	e_dev_err("%s drop\n", __func__);

	return NETDEV_TX_OK;
}

static netdev_tx_t ngbe_xmit_frame(struct sk_buff *skb, struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_ring *tx_ring;
	unsigned int r_idx = skb->queue_mapping;

	if (!netif_carrier_ok(netdev)) {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}

	/* The minimum packet size for olinfo paylen is 17 so pad the skb
	 * in order to meet this minimum size requirement.
	 */
	if (skb_put_padto(skb, 17))
		return NETDEV_TX_OK;

	if (r_idx >= adapter->num_tx_queues)
		r_idx = r_idx % adapter->num_tx_queues;
	tx_ring = adapter->tx_ring[r_idx];
	if (unlikely(test_bit(__NGBE_TX_DISABLED, &tx_ring->state)))
		return NETDEV_TX_BUSY;
	return ngbe_xmit_frame_ring(skb, adapter, tx_ring);
}

/**
 * ngbe_set_mac - Change the Ethernet Address of the NIC
 * @netdev: network interface device structure
 * @p: pointer to an address structure
 * Returns 0 on success, negative on failure
 **/
static int ngbe_set_mac(struct net_device *netdev, void *p)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	struct ngbe_hw *hw = &adapter->hw;
	struct sockaddr *addr = p;

	if (!is_valid_ether_addr((u8 *)addr->sa_data))
		return -EADDRNOTAVAIL;

	ngbe_del_mac_filter(adapter, hw->mac.addr, VMDQ_P(0));
	eth_hw_addr_set(netdev, (u8 *)addr->sa_data);
	memcpy(hw->mac.addr, addr->sa_data, netdev->addr_len);

	ngbe_mac_set_default_filter(adapter, hw->mac.addr);
	e_info(drv, "The mac has been set to %02X:%02X:%02X:%02X:%02X:%02X\n", hw->mac.addr[0],
	       hw->mac.addr[1], hw->mac.addr[2], hw->mac.addr[3], hw->mac.addr[4], hw->mac.addr[5]);

	return 0;
}

static int ngbe_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	switch (cmd) {
	case SIOCGHWTSTAMP:
		return ngbe_ptp_get_ts_config(adapter, ifr);
	case SIOCSHWTSTAMP:
		return ngbe_ptp_set_ts_config(adapter, ifr);
	case SIOCGMIIREG:
	case SIOCSMIIREG:
	default:
		return -EOPNOTSUPP;
	}
}

/* Polling 'interrupt' - used by things like netconsole to send skbs
 * without having to re-enable interrupts. It's not called while
 * the interrupt routine is executing.
 */
static void ngbe_netpoll(struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	/* if interface is down do nothing */
	if (test_bit(__NGBE_DOWN, &adapter->state))
		return;

	if (adapter->flags & NGBE_FLAG_MSIX_ENABLED) {
		int i;

		for (i = 0; i < adapter->num_q_vectors; i++) {
			adapter->q_vector[i]->netpoll_rx = true;
			ngbe_msix_clean_rings(0, adapter->q_vector[i]);
			adapter->q_vector[i]->netpoll_rx = false;
		}
	} else {
		ngbe_intr(0, adapter);
	}
}

/**
 * ngbe_setup_tc - routine to configure net_device for multiple traffic
 * classes.
 * @netdev: net device to configure
 * @tc: number of traffic classes to enable
 */
int ngbe_setup_tc(struct net_device *dev, u8 tc)
{
	struct ngbe_adapter *adapter = netdev_priv(dev);

	if (adapter->xdp_prog) {
		if (adapter->num_rx_queues > MAX_RX_QUEUES) {
			adapter->old_rss_limit = adapter->ring_feature[RING_F_RSS].limit;
			adapter->ring_feature[RING_F_RSS].limit = MAX_RX_QUEUES / 2;
			e_dev_info("limit tx rx ring to 4 because hw limit");
		} else {
			adapter->old_rss_limit = 0;
		}
	}

	if (netif_running(dev))
		ngbe_close(dev);
	else
		ngbe_reset(adapter);

	ngbe_clear_interrupt_scheme(adapter);

	if (tc)
		netdev_set_num_tc(dev, tc);
	else
		netdev_reset_tc(dev);

	ngbe_init_interrupt_scheme(adapter);
	if (netif_running(dev))
		ngbe_open(dev);

	return 0;
}

#ifdef CONFIG_PCI_IOV
void ngbe_sriov_reinit(struct ngbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;

	rtnl_lock();
	ngbe_setup_tc(netdev, netdev_get_num_tc(netdev));
	rtnl_unlock();
}
#endif

void ngbe_do_reset(struct net_device *netdev)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);

	if (netif_running(netdev))
		ngbe_reinit_locked(adapter);
	else
		ngbe_reset(adapter);
}

static netdev_features_t ngbe_fix_features(struct net_device *netdev, netdev_features_t features)

{
	/* If Rx checksum is disabled, then RSC/LRO should also be disabled */
	if (!(features & NETIF_F_RXCSUM))
		features &= ~NETIF_F_LRO;

	/* Turn off LRO if not RSC capable */
	features &= ~NETIF_F_LRO;

	if (!(features & NETIF_F_HW_VLAN_CTAG_RX))
		features &= ~NETIF_F_HW_VLAN_STAG_RX;
	else
		features |= NETIF_F_HW_VLAN_STAG_RX;
	if (!(features & NETIF_F_HW_VLAN_CTAG_TX))
		features &= ~NETIF_F_HW_VLAN_STAG_TX;
	else
		features |= NETIF_F_HW_VLAN_STAG_TX;
	if (!(features & NETIF_F_HW_VLAN_CTAG_FILTER))
		features &= ~NETIF_F_HW_VLAN_STAG_FILTER;
	else
		features |= NETIF_F_HW_VLAN_STAG_FILTER;
	return features;
}

static int ngbe_set_features(struct net_device *netdev, netdev_features_t features)
{
	struct ngbe_adapter *adapter = netdev_priv(netdev);
	bool need_reset = false;
	netdev_features_t changed = netdev->features ^ features;

#if (defined NETIF_F_HW_VLAN_CTAG_RX) && (defined NETIF_F_HW_VLAN_STAG_RX)
	if ((features & NETIF_F_HW_VLAN_CTAG_RX) && (features & NETIF_F_HW_VLAN_STAG_RX))
#elif (defined NETIF_F_HW_VLAN_CTAG_RX)
	if (features & NETIF_F_HW_VLAN_CTAG_RX)
#elif (defined NETIF_F_HW_VLAN_STAG_RX)
	if (features & NETIF_F_HW_VLAN_STAG_RX)
#else
	if (features & NETIF_F_HW_VLAN_RX)
#endif
		ngbe_vlan_strip_enable(adapter);
	else
		ngbe_vlan_strip_disable(adapter);

	if (features & NETIF_F_RXHASH) {
		if (!(adapter->flags2 & NGBE_FLAG2_RSS_ENABLED)) {
			wr32m(&adapter->hw, NGBE_RDB_RA_CTL, NGBE_RDB_RA_CTL_RSS_EN,
			      NGBE_RDB_RA_CTL_RSS_EN);
			adapter->flags2 |= NGBE_FLAG2_RSS_ENABLED;
		}
	} else {
		if (adapter->flags2 & NGBE_FLAG2_RSS_ENABLED) {
			wr32m(&adapter->hw, NGBE_RDB_RA_CTL, NGBE_RDB_RA_CTL_RSS_EN,
			      ~NGBE_RDB_RA_CTL_RSS_EN);
			adapter->flags2 &= ~NGBE_FLAG2_RSS_ENABLED;
		}
	}

	netdev->features = features;

	if (need_reset)
		ngbe_do_reset(netdev);
	else if (changed & NETIF_F_HW_VLAN_CTAG_FILTER)
		ngbe_set_rx_mode(netdev);

	return 0;
}

static int ngbe_ndo_fdb_add(struct ndmsg *ndm, struct nlattr *tb[], struct net_device *dev,
			    const unsigned char *addr, u16 vid, u16 flags,
			    struct netlink_ext_ack __always_unused *extack)
{
	/* guarantee we can provide a unique filter for the unicast address */
	if (is_unicast_ether_addr(addr) || is_link_local_ether_addr(addr)) {
		if (netdev_uc_count(dev) > NGBE_MAX_PF_MACVLANS)
			return -ENOMEM;
	}

	return ndo_dflt_fdb_add(ndm, tb, dev, addr, vid, flags);
}

static int ngbe_ndo_bridge_setlink(struct net_device *dev, struct nlmsghdr *nlh,
				   __always_unused u16 flags,
				   struct netlink_ext_ack __always_unused *ext)
{
	struct ngbe_adapter *adapter = netdev_priv(dev);
	struct nlattr *attr, *br_spec;
	int rem;

	if (!(adapter->flags & NGBE_FLAG_SRIOV_ENABLED))
		return -EOPNOTSUPP;

	br_spec = nlmsg_find_attr(nlh, sizeof(struct ifinfomsg), IFLA_AF_SPEC);

	nla_for_each_nested(attr, br_spec, rem) {
		__u16 mode;

		if (nla_type(attr) != IFLA_BRIDGE_MODE)
			continue;

		mode = nla_get_u16(attr);
		if (mode == BRIDGE_MODE_VEPA)
			adapter->flags |= NGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE;
		else if (mode == BRIDGE_MODE_VEB)
			adapter->flags &= ~NGBE_FLAG_SRIOV_VEPA_BRIDGE_MODE;
		else
			return -EINVAL;

		adapter->bridge_mode = mode;

		/* re-configure settings related to bridge mode */
		ngbe_configure_bridge_mode(adapter);

		e_info(drv, "enabling bridge mode: %s\n",
		       mode == BRIDGE_MODE_VEPA ? "VEPA" : "VEB");
	}

	return 0;
}

static int ngbe_ndo_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq, struct net_device *dev,
				   u32 __maybe_unused filter_mask, int nlflags)
{
	struct ngbe_adapter *adapter = netdev_priv(dev);
	u16 mode;

	if (!(adapter->flags & NGBE_FLAG_SRIOV_ENABLED))
		return 0;

	mode = adapter->bridge_mode;
	return ndo_dflt_bridge_getlink(skb, pid, seq, dev, mode, 0, 0, nlflags, filter_mask, NULL);
}

#define NGBE_MAX_TUNNEL_HDR_LEN 80
static netdev_features_t ngbe_features_check(struct sk_buff *skb, struct net_device *dev,
					     netdev_features_t features)
{
	u32 vlan_num = 0;
	u16 vlan_depth = skb->mac_len;
	__be16 type = skb->protocol;
	struct vlan_hdr *vh;

	if (skb_vlan_tag_present(skb))
		vlan_num++;

	if (vlan_depth)
		vlan_depth -= VLAN_HLEN;
	else
		vlan_depth = ETH_HLEN;

	while (type == htons(ETH_P_8021Q) || type == htons(ETH_P_8021AD)) {
		vlan_num++;
		vh = (struct vlan_hdr *)(skb->data + vlan_depth);
		type = vh->h_vlan_encapsulated_proto;
		vlan_depth += VLAN_HLEN;
	}

	if (vlan_num > 2)
		features &= ~(NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_HW_VLAN_STAG_TX);

	if (skb->encapsulation) {
		if (unlikely(skb_inner_mac_header(skb) - skb_transport_header(skb) >
			     NGBE_MAX_TUNNEL_HDR_LEN))
			return features & ~NETIF_F_CSUM_MASK;
	}
	return features;
}

static int ngbe_xdp_setup(struct net_device *dev, struct bpf_prog *prog)
{
	int i, frame_size = dev->mtu + ETH_HLEN + ETH_FCS_LEN + VLAN_HLEN;
	struct ngbe_adapter *adapter = netdev_priv(dev);
	struct bpf_prog *old_prog;
	bool need_reset;

	if (adapter->flags & NGBE_FLAG_SRIOV_ENABLED)
		return -EINVAL;

	if (adapter->flags & NGBE_FLAG_DCB_ENABLED)
		return -EINVAL;

	if (adapter->xdp_prog && prog) {
		e_dev_err("XDP can't be active at the same time");
		return -EBUSY;
	}

	/* verify ngbe ring attributes are sufficient for XDP */
	for (i = 0; i < adapter->num_rx_queues; i++) {
		struct ngbe_ring *ring = adapter->rx_ring[i];

		if (frame_size > ngbe_rx_bufsz(ring))
			return -EINVAL;
	}

	old_prog = adapter->xdp_prog;
	need_reset = (!!prog != !!old_prog);

	if (need_reset) {
		if (netif_running(dev))
			ngbe_close(dev);
		else
			ngbe_reset(adapter);
		if (!prog)
			xdp_features_clear_redirect_target(dev);
		if (nr_cpu_ids > MAX_XDP_QUEUES)
			static_branch_inc(&ngbe_xdp_locking_key);
	}

	old_prog = xchg(&adapter->xdp_prog, prog);

	/* If transitioning XDP modes reconfigure rings */
	if (need_reset) {
		if (adapter->xdp_prog) {
			if (adapter->num_rx_queues > MAX_RX_QUEUES / 2) {
				adapter->old_rss_limit = adapter->ring_feature[RING_F_RSS].limit;
				adapter->ring_feature[RING_F_RSS].limit = MAX_RX_QUEUES / 2;
				e_dev_info("limit tx rx ring to 4 because hw limit");
			} else {
				adapter->old_rss_limit = 0;
			}
		} else {
			if (adapter->old_rss_limit)
				adapter->ring_feature[RING_F_RSS].limit = adapter->old_rss_limit;
		}

		ngbe_clear_interrupt_scheme(adapter);

		ngbe_init_interrupt_scheme(adapter);

		if (netif_running(dev))
			ngbe_open(dev);

	} else {
		for (i = 0; i < adapter->num_rx_queues; i++)
			xchg(&adapter->rx_ring[i]->xdp_prog, adapter->xdp_prog);
	}

	/* Kick start the NAPI context if there is an AF_XDP socket open
	 * on that queue id. This so that receiving will start.
	 */
	if (need_reset && prog) {
		for (i = 0; i < adapter->num_rx_queues; i++)
			if (adapter->xdp_ring[i]->xsk_pool)
				(void)ngbe_xsk_wakeup(adapter->netdev, i, XDP_WAKEUP_RX);

		xdp_features_set_redirect_target(dev, true);
	}

	if (adapter->xdp_prog)
		e_dev_info("xdp program is setup");
	else
		e_dev_info("xdp program not load");
	if (old_prog)
		bpf_prog_put(old_prog);

	return 0;
}

static int ngbe_xdp(struct net_device *dev, struct netdev_bpf *xdp)
{
	struct ngbe_adapter *adapter = netdev_priv(dev);

	switch (xdp->command) {
	case XDP_SETUP_PROG:
		return ngbe_xdp_setup(dev, xdp->prog);
	case XDP_SETUP_XSK_POOL:
		return ngbe_xsk_umem_setup(adapter, xdp->xsk.pool, xdp->xsk.queue_id);
	default:
		return -EINVAL;
	}
}

static int ngbe_xdp_xmit(struct net_device *dev, int n, struct xdp_frame **frames, u32 flags)
{
	struct ngbe_adapter *adapter = netdev_priv(dev);
	struct ngbe_ring *ring;
	int nxmit = 0;
	int i;

	if (unlikely(test_bit(__NGBE_DOWN, &adapter->state)))
		return -ENETDOWN;

	if (unlikely(flags & ~XDP_XMIT_FLAGS_MASK))
		return -EINVAL;
	/* During program transitions its possible adapter->xdp_prog is assigned
	 * but ring has not been configured yet. In this case simply abort xmit.
	 */
	ring = adapter->xdp_prog ? adapter->xdp_ring[smp_processor_id() % adapter->num_xdp_queues]
				 : NULL;
	if (unlikely(!ring))
		return -ENXIO;

	if (unlikely(test_bit(__NGBE_TX_DISABLED, &ring->state)))
		return -ENXIO;
	if (static_branch_unlikely(&ngbe_xdp_locking_key))
		spin_lock(&ring->tx_lock);

	for (i = 0; i < n; i++) {
		struct xdp_frame *xdpf = frames[i];
		int err;

		err = ngbe_xmit_xdp_ring(ring, xdpf);
		if (err != NGBE_XDP_TX)
			break;
		nxmit++;
	}

	if (unlikely(flags & XDP_XMIT_FLUSH)) {
		/* Force memory writes to complete before letting h/w know there
		 * are new descriptors to fetch.
		 */
		wmb();
		writel(ring->next_to_use, ring->tail);
	}
	if (static_branch_unlikely(&ngbe_xdp_locking_key))
		spin_unlock(&ring->tx_lock);

	return nxmit;
}

static void ngbe_macvlan_set_rx_mode(struct net_device *dev, unsigned int pool,
				     struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 vmolr;

	/* No unicast promiscuous support for VMDQ devices. */
	vmolr = rd32m(hw, NGBE_PSR_VM_L2CTL(pool),
		      ~(NGBE_PSR_VM_L2CTL_UPE | NGBE_PSR_VM_L2CTL_MPE | NGBE_PSR_VM_L2CTL_ROPE |
			NGBE_PSR_VM_L2CTL_ROMPE));

	/* set all bits that we expect to always be set */
	vmolr |= NGBE_PSR_VM_L2CTL_ROPE | NGBE_PSR_VM_L2CTL_BAM | NGBE_PSR_VM_L2CTL_AUPE;

	if (dev->flags & IFF_ALLMULTI) {
		vmolr |= NGBE_PSR_VM_L2CTL_MPE;
	} else {
		vmolr |= NGBE_PSR_VM_L2CTL_ROMPE;
		ngbe_write_mc_addr_list(dev);
	}

	ngbe_write_uc_addr_list(adapter->netdev, pool);
	wr32(hw, NGBE_PSR_VM_L2CTL(pool), vmolr);
}

static void ngbe_fwd_psrtype(struct ngbe_fwd_adapter *accel)
{
	struct ngbe_adapter *adapter = accel->adapter;
	struct ngbe_hw *hw = &adapter->hw;
	u32 psrtype = NGBE_RDB_PL_CFG_L4HDR | NGBE_RDB_PL_CFG_L3HDR | NGBE_RDB_PL_CFG_L2HDR |
		      NGBE_RDB_PL_CFG_TUN_OUTER_L2HDR | NGBE_RDB_PL_CFG_TUN_TUNHDR;

	wr32(hw, NGBE_RDB_PL_CFG(VMDQ_P(accel->index)), psrtype);
}

static void ngbe_disable_fwd_ring(struct ngbe_fwd_adapter *accel, struct ngbe_ring *rx_ring)
{
	struct ngbe_adapter *adapter = accel->adapter;
	int index = rx_ring->queue_index + accel->rx_base_queue;

	/* shutdown specific queue receive and wait for dma to settle */
	ngbe_disable_rx_queue(adapter, rx_ring);
	usleep_range(10000, 20000);
	ngbe_intr_disable(&adapter->hw, NGBE_INTR_Q(index));
	ngbe_clean_rx_ring(rx_ring);
	rx_ring->accel = NULL;
}

static void ngbe_enable_fwd_ring(struct ngbe_fwd_adapter *accel, struct ngbe_ring *rx_ring)
{
	struct ngbe_adapter *adapter = accel->adapter;
	int index = rx_ring->queue_index + accel->rx_base_queue;

	ngbe_intr_enable(&adapter->hw, NGBE_INTR_Q(index));
}

static int ngbe_fwd_ring_down(struct net_device *vdev, struct ngbe_fwd_adapter *accel)
{
	struct ngbe_adapter *adapter = accel->adapter;
	unsigned int rxbase = accel->rx_base_queue;
	unsigned int txbase = accel->tx_base_queue;
	int i;

	netif_tx_stop_all_queues(vdev);

	for (i = 0; i < adapter->queues_per_pool; i++) {
		ngbe_disable_fwd_ring(accel, adapter->rx_ring[rxbase + i]);
		adapter->rx_ring[rxbase + i]->netdev = adapter->netdev;
	}

	for (i = 0; i < adapter->queues_per_pool; i++) {
		adapter->tx_ring[txbase + i]->accel = NULL;
		adapter->tx_ring[txbase + i]->netdev = adapter->netdev;
	}

	return 0;
}

static inline int ngbe_inc_vmdqs(struct ngbe_fwd_adapter *accel)
{
	struct ngbe_adapter *adapter = accel->adapter;

	if (++adapter->num_vmdqs > 1 || adapter->num_vfs > 0)
		adapter->flags |= NGBE_FLAG_VMDQ_ENABLED | NGBE_FLAG_SRIOV_ENABLED;
	accel->index = find_first_zero_bit(&adapter->fwd_bitmask, NGBE_MAX_MACVLANS);
	set_bit(accel->index, &adapter->fwd_bitmask);

	return 1 + find_last_bit(&adapter->fwd_bitmask, NGBE_MAX_MACVLANS);
}

static inline int ngbe_dec_vmdqs(struct ngbe_fwd_adapter *accel)
{
	struct ngbe_adapter *adapter = accel->adapter;

	if (--adapter->num_vmdqs == 1 && adapter->num_vfs == 0)
		adapter->flags &= ~(NGBE_FLAG_VMDQ_ENABLED | NGBE_FLAG_SRIOV_ENABLED);
	clear_bit(accel->index, &adapter->fwd_bitmask);

	return 1 + find_last_bit(&adapter->fwd_bitmask, NGBE_MAX_MACVLANS);
}

static int ngbe_fwd_ring_up(struct net_device *vdev, struct ngbe_fwd_adapter *accel)
{
	struct ngbe_adapter *adapter = accel->adapter;
	unsigned int rxbase, txbase, queues;
	int i, baseq, err = 0;

	if (!test_bit(accel->index, &adapter->fwd_bitmask))
		return 0;

	baseq = VMDQ_P(accel->index) * adapter->queues_per_pool;
	netdev_dbg(vdev, "pool %i:%i queues %i:%i VSI bitmask %lx\n", accel->index,
		   adapter->num_vmdqs, baseq, baseq + adapter->queues_per_pool,
		   adapter->fwd_bitmask);

	accel->vdev = vdev;
	accel->rx_base_queue = baseq;
	rxbase = baseq;
	accel->tx_base_queue = baseq;
	txbase = baseq;

	for (i = 0; i < adapter->queues_per_pool; i++)
		ngbe_disable_fwd_ring(accel, adapter->rx_ring[rxbase + i]);

	for (i = 0; i < adapter->queues_per_pool; i++) {
		adapter->rx_ring[rxbase + i]->netdev = vdev;
		adapter->rx_ring[rxbase + i]->accel = accel;
		ngbe_configure_rx_ring(adapter, adapter->rx_ring[rxbase + i]);
	}

	for (i = 0; i < adapter->queues_per_pool; i++) {
		adapter->tx_ring[txbase + i]->netdev = vdev;
		adapter->tx_ring[txbase + i]->accel = accel;
	}

	queues = min_t(unsigned int, adapter->queues_per_pool, vdev->num_tx_queues);
	err = netif_set_real_num_tx_queues(vdev, queues);
	if (err)
		goto fwd_queue_err;

	err = netif_set_real_num_rx_queues(vdev, queues);
	if (err)
		goto fwd_queue_err;

	if (is_valid_ether_addr(vdev->dev_addr))
		ngbe_add_mac_filter(adapter, vdev->dev_addr, VMDQ_P(accel->index));

	ngbe_fwd_psrtype(accel);
	ngbe_macvlan_set_rx_mode(vdev, VMDQ_P(accel->index), adapter);

	for (i = 0; i < adapter->queues_per_pool; i++)
		ngbe_enable_fwd_ring(accel, adapter->rx_ring[rxbase + i]);

	return err;
fwd_queue_err:
	ngbe_fwd_ring_down(vdev, accel);
	return err;
}

static void *ngbe_fwd_add(struct net_device *pdev, struct net_device *vdev)
{
	struct ngbe_fwd_adapter *accel = NULL;
	struct ngbe_adapter *adapter = netdev_priv(pdev);
	int used_pools = adapter->num_vfs + adapter->num_vmdqs;
	int err;

	if (test_bit(__NGBE_DOWN, &adapter->state))
		return ERR_PTR(-EPERM);

	/* Hardware has a limited number of available pools. Each VF, and the
	 * PF require a pool. Check to ensure we don't attempt to use more
	 * than the available number of pools.
	 */
	if (used_pools >= NGBE_MAX_VF_FUNCTIONS)
		return ERR_PTR(-EINVAL);

#ifdef CONFIG_RPS
	if (vdev->num_rx_queues != vdev->num_tx_queues) {
		netdev_info(pdev, "%s: Only supports a single queue count for TX and RX\n",
			    vdev->name);
		return ERR_PTR(-EINVAL);
	}
#endif
	/* Check for hardware restriction on number of rx/tx queues */
	if (vdev->num_tx_queues != 2 && vdev->num_tx_queues != 4) {
		netdev_info(pdev, "%s: Supports RX/TX Queue counts 2, and 4\n", pdev->name);
		return ERR_PTR(-EINVAL);
	}

	accel = kzalloc(sizeof(*accel), GFP_KERNEL);
	if (!accel)
		return ERR_PTR(-ENOMEM);
	accel->adapter = adapter;

	/* Enable VMDq flag so device will be set in VM mode */
	adapter->ring_feature[RING_F_VMDQ].limit = ngbe_inc_vmdqs(accel);
	adapter->ring_feature[RING_F_RSS].limit = vdev->num_tx_queues;

	/* Force reinit of ring allocation with VMDQ enabled */
	err = ngbe_setup_tc(pdev, netdev_get_num_tc(pdev));
	if (err)
		goto fwd_add_err;

	err = ngbe_fwd_ring_up(vdev, accel);
	if (err)
		goto fwd_add_err;

	netif_tx_start_all_queues(vdev);
	return accel;
fwd_add_err:
	/* unwind counter and free adapter struct */
	netdev_info(pdev, "%s: dfwd hardware acceleration failed\n", vdev->name);
	ngbe_dec_vmdqs(accel);
	kfree(accel);
	return ERR_PTR(err);
}

static void ngbe_fwd_del(struct net_device *pdev, void *fwd_priv)
{
	struct ngbe_fwd_adapter *accel = fwd_priv;
	struct ngbe_adapter *adapter = accel->adapter;

	if (!accel || adapter->num_vmdqs <= 1)
		return;

	adapter->ring_feature[RING_F_VMDQ].limit = ngbe_dec_vmdqs(accel);
	ngbe_fwd_ring_down(accel->vdev, accel);
	ngbe_setup_tc(pdev, netdev_get_num_tc(pdev));
	netdev_dbg(pdev, "pool %i:%i queues %i:%i VSI bitmask %lx\n", accel->index,
		   adapter->num_vmdqs, accel->rx_base_queue,
		   accel->rx_base_queue + adapter->queues_per_pool, adapter->fwd_bitmask);
	kfree(accel);
}

static const struct net_device_ops ngbe_netdev_ops = {
	.ndo_open = ngbe_open,
	.ndo_stop = ngbe_close,
	.ndo_start_xmit = ngbe_xmit_frame,
	.ndo_set_rx_mode = ngbe_set_rx_mode,
	.ndo_validate_addr = eth_validate_addr,
	.ndo_set_mac_address = ngbe_set_mac,
	.ndo_change_mtu = ngbe_change_mtu,
	.ndo_tx_timeout = ngbe_tx_timeout,
	.ndo_vlan_rx_add_vid = ngbe_vlan_rx_add_vid,
	.ndo_vlan_rx_kill_vid = ngbe_vlan_rx_kill_vid,
	.ndo_eth_ioctl = ngbe_ioctl,
	.ndo_set_vf_mac = ngbe_ndo_set_vf_mac,
	.ndo_set_vf_vlan = ngbe_ndo_set_vf_vlan,
	.ndo_set_vf_rate = ngbe_ndo_set_vf_bw,
	.ndo_set_vf_spoofchk = ngbe_ndo_set_vf_spoofchk,
	.ndo_set_vf_link_state = ngbe_ndo_set_vf_link_state,
	.ndo_set_vf_trust = ngbe_ndo_set_vf_trust,
	.ndo_get_vf_config = ngbe_ndo_get_vf_config,
	.ndo_get_stats64 = ngbe_get_stats64,
	.ndo_set_features = ngbe_set_features,
	.ndo_fix_features = ngbe_fix_features,
	.ndo_fdb_add = ngbe_ndo_fdb_add,
	.ndo_bridge_setlink = ngbe_ndo_bridge_setlink,
	.ndo_bridge_getlink = ngbe_ndo_bridge_getlink,
	.ndo_dfwd_add_station = ngbe_fwd_add,
	.ndo_dfwd_del_station = ngbe_fwd_del,
	.ndo_features_check = ngbe_features_check,
	.ndo_bpf = ngbe_xdp,
	.ndo_xdp_xmit = ngbe_xdp_xmit,
	.ndo_xsk_wakeup = ngbe_xsk_wakeup,
#ifdef CONFIG_NET_POLL_CONTROLLER
	.ndo_poll_controller = ngbe_netpoll,
#endif
#if IS_ENABLED(CONFIG_FCOE)
	.ndo_select_queue = ngbe_select_queue,
#endif
};

void ngbe_assign_netdev_ops(struct net_device *dev)
{
	dev->netdev_ops = &ngbe_netdev_ops;
	ngbe_set_ethtool_ops(dev);
	dev->watchdog_timeo = 5 * HZ;
}

/**
 * ngbe_probe - Device Initialization Routine
 * @pdev: PCI device information struct
 * @ent: entry in ngbe_pci_tbl
 * Returns 0 on success, negative on failure
 * ngbe_probe initializes an adapter identified by a pci_dev structure.
 * The OS initialization, configuring of the adapter private structure,
 * and a hardware reset occur.
 **/
static int ngbe_probe(struct pci_dev *pdev, const struct pci_device_id __always_unused *ent)
{
	struct net_device *netdev;
	struct ngbe_adapter *adapter = NULL;
	struct ngbe_hw *hw = NULL;
	static int cards_found;
	int err, pci_using_dac, expected_gts;
	u32 eeprom_verl = 0;
	u32 etrack_id = 0;
	char *info_string, *i_s_var;
	u32 eeprom_cksum_devcap = 0;
	u32 saved_version = 0;
	u32 devcap;
	u32 led_conf = 0;

	bool disable_dev = false;
	netdev_features_t hw_features;
	u16 pvalue = 0;

	err = pci_enable_device_mem(pdev);
	if (err)
		return err;

	if (!dma_set_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(64)) &&
	    !dma_set_coherent_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(64))) {
		pci_using_dac = 1;
	} else {
		err = dma_set_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(32));
		if (err) {
			err = dma_set_coherent_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(32));
			if (err) {
				dev_err(pci_dev_to_dev(pdev),
					"No usable DMA configuration, aborting\n");
				goto err_dma;
			}
		}
		pci_using_dac = 0;
	}

	err = pci_request_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM),
					   ngbe_driver_name);
	if (err) {
		dev_err(pci_dev_to_dev(pdev), "pci_request_selected_regions failed 0x%x\n", err);
		goto err_pci_reg;
	}
	pci_set_master(pdev);
	pcie_set_readrq(pdev, 128);

	netdev = alloc_etherdev_mq(sizeof(struct ngbe_adapter), NGBE_MAX_TX_QUEUES);

	if (!netdev) {
		err = -ENOMEM;
		goto err_alloc_etherdev;
	}

	SET_NETDEV_DEV(netdev, pci_dev_to_dev(pdev));

	adapter = netdev_priv(netdev);
	adapter->netdev = netdev;
	adapter->pdev = pdev;
	hw = &adapter->hw;
	hw->back = adapter;
	adapter->msg_enable = (1 << DEFAULT_DEBUG_LEVEL_SHIFT) - 1;

	hw->hw_addr = ioremap(pci_resource_start(pdev, 0), pci_resource_len(pdev, 0));

	adapter->io_addr = hw->hw_addr;
	if (!hw->hw_addr) {
		err = -EIO;
		goto err_ioremap;
	}

	/* default config: 10/100/1000M autoneg on */
	hw->mac.autoneg = true;
	hw->phy.autoneg_advertised = NGBE_LINK_SPEED_AUTONEG;
	hw->phy.force_speed = NGBE_LINK_SPEED_UNKNOWN;
	/* assign netdev ops and ethtool ops */
	ngbe_assign_netdev_ops(netdev);

	strscpy(netdev->name, pci_name(pdev), sizeof(netdev->name) - 1);

	adapter->bd_number = cards_found;

	/* setup the private structure */
	err = ngbe_sw_init(adapter);
	if (err)
		goto err_sw_init;

	/* check_options must be called before setup_link to set up
	 * hw->fc completely
	 */
	ngbe_check_options(adapter);

	hw->mac.ops.set_lan_id(hw);

	/* check if flash load is done after hw power up */
	err = ngbe_check_flash_load(hw, NGBE_SPI_ILDR_STATUS_PERST);
	if (err)
		goto err_sw_init;
	err = ngbe_check_flash_load(hw, NGBE_SPI_ILDR_STATUS_PWRRST);
	if (err)
		goto err_sw_init;

	if (ngbe_is_lldp(hw))
		e_dev_err("Can not get lldp flags from flash\n");
	/* reset_hw fills in the perm_addr as well */

	hw->phy.reset_if_overtemp = true;
	err = hw->mac.ops.reset_hw(hw);
	hw->phy.reset_if_overtemp = false;
	if (err) {
		e_dev_err("HW reset failed: %d\n", err);
		goto err_sw_init;
	}

	err = hw->phy.ops.init(hw);
	if (err)
		goto err_sw_init;

	netdev->features |= NETIF_F_SG | NETIF_F_IP_CSUM;

	netdev->features |= NETIF_F_IPV6_CSUM;

	netdev->features |= NETIF_F_HW_VLAN_CTAG_TX | NETIF_F_HW_VLAN_CTAG_RX;

	netdev->features |= ngbe_tso_features();
	if (adapter->flags2 & NGBE_FLAG2_RSS_ENABLED)
		netdev->features |= NETIF_F_RXHASH;
	netdev->features |= NETIF_F_RXCSUM;

	netdev->features |= NETIF_F_HW_VLAN_CTAG_FILTER;

	/* copy netdev features into list of user selectable features */
	hw_features = netdev->hw_features;
	hw_features |= netdev->features;

	netdev->features |= NETIF_F_HW_VLAN_STAG_TX | NETIF_F_HW_VLAN_STAG_RX;

	/* set this bit last since it cannot be part of hw_features */
	netdev->features |= NETIF_F_HW_VLAN_STAG_FILTER;
	/* set this bit last since it cannot be part of hw_features */
	netdev->features |= NETIF_F_NTUPLE;
	hw_features |= NETIF_F_NTUPLE;

	netdev->hw_features = hw_features;
	netdev->xdp_features =
		NETDEV_XDP_ACT_BASIC | NETDEV_XDP_ACT_REDIRECT | NETDEV_XDP_ACT_XSK_ZEROCOPY;
	netdev->vlan_features |=
		NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_TSO | NETIF_F_TSO6;

	netdev->priv_flags |= IFF_UNICAST_FLT;
	netdev->priv_flags |= IFF_SUPP_NOFCS;
	netdev->min_mtu = ETH_MIN_MTU;
	netdev->max_mtu = NGBE_MAX_JUMBO_FRAME_SIZE - (ETH_HLEN + ETH_FCS_LEN);

	if (pci_using_dac) {
		netdev->features |= NETIF_F_HIGHDMA;
		netdev->vlan_features |= NETIF_F_HIGHDMA;
	}

	if (hw->bus.lan_id == 0) {
		wr32(hw, NGBE_CALSUM_CAP_STATUS, 0x0);
		wr32(hw, NGBE_EEPROM_VERSION_STORE_REG, 0x0);
	} else {
		eeprom_cksum_devcap = rd32(hw, NGBE_CALSUM_CAP_STATUS);
		saved_version = rd32(hw, NGBE_EEPROM_VERSION_STORE_REG);
	}

	hw->eeprom.ops.init_params(hw);
	hw->mac.ops.release_swfw_sync(hw, NGBE_MNG_SWFW_SYNC_SW_MB);
	if (hw->bus.lan_id == 0 || eeprom_cksum_devcap == 0) {
		/* make sure the EEPROM is good */
		if (hw->eeprom.ops.eeprom_chksum_cap_st(hw, NGBE_CALSUM_COMMAND, &devcap)) {
			e_dev_err("The EEPROM Checksum Is Not Valid\n");
			err = -EIO;
			goto err_sw_init;
		}
	}

	if (hw->eeprom.ops.phy_led_oem_chk(hw, &led_conf))
		adapter->led_conf = -1;
	else
		adapter->led_conf = led_conf;

	eth_hw_addr_set(netdev, hw->mac.perm_addr);

	if (!is_valid_ether_addr(netdev->dev_addr)) {
		e_dev_err("invalid MAC address\n");
		err = -EIO;
		goto err_sw_init;
	}

	ngbe_mac_set_default_filter(adapter, hw->mac.perm_addr);

	timer_setup(&adapter->service_timer, ngbe_service_timer, 0);
	if (NGBE_POLL_LINK_STATUS == 1)
		timer_setup(&adapter->link_check_timer, ngbe_link_check_timer, 0);

	if (NGBE_REMOVED(hw->hw_addr)) {
		err = -EIO;
		goto err_sw_init;
	}
	INIT_WORK(&adapter->service_task, ngbe_service_task);
	set_bit(__NGBE_SERVICE_INITED, &adapter->state);
	clear_bit(__NGBE_SERVICE_SCHED, &adapter->state);

	err = ngbe_init_interrupt_scheme(adapter);
	if (err)
		goto err_sw_init;

#ifdef CONFIG_PCI_IOV
	if (adapter->max_vfs > 0) {
		e_dev_warn("Enabling SR-IOV VFs using the max_vfs parameter is deprecated.\n");
		e_dev_warn("Please use the pci sysfs interface instead. Ex:\n");
		e_dev_warn("echo '%d' > /sys/bus/pci/devices/%04x:%02x:%02x.%1x/sriov_numvfs\n",
			   adapter->max_vfs, pci_domain_nr(pdev->bus), pdev->bus->number,
			   PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn));
	}

	if (adapter->flags & NGBE_FLAG_SRIOV_CAPABLE) {
		pci_sriov_set_totalvfs(pdev, NGBE_MAX_VFS_DRV_LIMIT);
		ngbe_enable_sriov(adapter);
	}
#endif /* CONFIG_PCI_IOV */

	/* WOL not supported for all devices */
	adapter->wol = 0;
	if (hw->bus.lan_id == 0 || eeprom_cksum_devcap == 0) {
		hw->eeprom.ops.read(hw, hw->eeprom.sw_region_offset + NGBE_DEVICE_CAPS,
				    &adapter->eeprom_cap);
		/*only support in LAN0*/
		adapter->eeprom_cap = NGBE_DEVICE_CAPS_WOL_PORT0;
	} else {
		adapter->eeprom_cap = eeprom_cksum_devcap & 0xffff;
	}

	if (hw->wol_enabled) {
		adapter->wol = NGBE_PSR_WKUP_CTL_MAG;
		/*enable wol  first in shadow ram*/
		ngbe_write_ee_hostif(hw, 0x7FE, 0xa50F);
		ngbe_write_ee_hostif(hw, 0x7FF, 0x5a5a);
	}

	wr32(hw, NGBE_PSR_WKUP_CTL, adapter->wol);

	device_set_wakeup_enable(pci_dev_to_dev(adapter->pdev), adapter->wol);

	/* Save off EEPROM version number and Option Rom version which
	 * together make a unique identify for the eeprom
	 */
	if (hw->bus.lan_id == 0 || saved_version == 0) {
		hw->eeprom.ops.read32(hw, hw->eeprom.sw_region_offset + NGBE_EEPROM_VERSION_L,
				      &eeprom_verl);
		etrack_id = eeprom_verl;
		wr32(hw, NGBE_EEPROM_VERSION_STORE_REG, etrack_id);
		wr32(hw, NGBE_CALSUM_CAP_STATUS, 0x10000 | (u32)adapter->eeprom_cap);
	} else if (eeprom_cksum_devcap) {
		etrack_id = saved_version;
	} else {
		hw->eeprom.ops.read32(hw, hw->eeprom.sw_region_offset + NGBE_EEPROM_VERSION_L,
				      &eeprom_verl);
		etrack_id = eeprom_verl;
	}

	/* Make sure offset to SCSI block is valid */
	snprintf(adapter->eeprom_id, sizeof(adapter->eeprom_id), "0x%08x", etrack_id);

	/* reset the hardware with the new settings */
	err = hw->mac.ops.start_hw(hw);
	if (err == NGBE_ERR_EEPROM_VERSION) {
		/* We are running on a pre-production device, log a warning */
		e_dev_warn("This device is a pre-production adapter/LOM.\n");
	} else if (err) {
		e_dev_err("HW init failed, err = %d\n", err);
		goto err_register;
	}

	/* pick up the PCI bus settings for reporting later */
	hw->mac.ops.get_bus_info(hw);

	strscpy(netdev->name, "eth%d", sizeof(netdev->name) - 1);
	err = register_netdev(netdev);
	if (err)
		goto err_register;

	pci_set_drvdata(pdev, adapter);
	adapter->netdev_registered = true;
	/* call save state here in standalone driver because it relies on
	 * adapter struct to exist, and needs to call netdev_priv
	 */
	pci_save_state(pdev);

	/* carrier off reporting is important to ethtool even BEFORE open */
	netif_carrier_off(netdev);
	/* keep stopping all the transmit queues for older kernels */
	netif_tx_stop_all_queues(netdev);

	/* print all messages at the end so that we use our eth%d name */

	/* calculate the expected PCIe bandwidth required for optimal
	 * performance. Note that some older parts will never have enough
	 * bandwidth due to being older generation PCIe parts. We clamp these
	 * parts to ensure that no warning is displayed, as this could confuse
	 * users otherwise.
	 */

	expected_gts = ngbe_enumerate_functions(adapter) * 10;

	/* don't check link if we failed to enumerate functions */
	if (expected_gts > 0)
		ngbe_check_minimum_link(adapter, expected_gts);

	hw->mac.ops.set_fw_drv_ver(hw, 0xFF, 0xFF, 0xFF, 0xFF);

	if (hw->ncsi_enabled)
		e_info(probe, "NCSI : support");
	else
		e_info(probe, "NCSI : unsupported");

	e_info(probe, "PHY: %s, PBA No: Wang Xun GbE Family Controller\n",
	       hw->phy.type == ngbe_phy_internal ? "Internal" : "External");

	e_info(probe, "%02x:%02x:%02x:%02x:%02x:%02x\n", netdev->dev_addr[0], netdev->dev_addr[1],
	       netdev->dev_addr[2], netdev->dev_addr[3], netdev->dev_addr[4], netdev->dev_addr[5]);

#define INFO_STRING_LEN 255
	info_string = kzalloc(INFO_STRING_LEN, GFP_KERNEL);
	if (!info_string) {
		e_err(probe, "allocation for info string failed\n");
		goto no_info_string;
	}
	i_s_var = info_string;
	i_s_var += sprintf(info_string, "Enabled Features: ");
	i_s_var += sprintf(i_s_var, "RxQ: %d TxQ: %d ", adapter->num_rx_queues,
			   adapter->num_tx_queues);
	if (adapter->flags & NGBE_FLAG_TPH_ENABLED)
		i_s_var += sprintf(i_s_var, "TPH ");

	WARN_ON_ONCE(i_s_var > (info_string + INFO_STRING_LEN));
	/* end features printing */
	e_info(probe, "%s\n", info_string);
	kfree(info_string);
no_info_string:

#ifdef CONFIG_PCI_IOV
	if (adapter->flags & NGBE_FLAG_SRIOV_ENABLED) {
		int i;

		for (i = 0; i < adapter->num_vfs; i++)
			ngbe_vf_configuration(pdev, (i | 0x10000000));
	}
#endif

	e_info(probe, "WangXun(R) Gigabit Network Connection\n");
	cards_found++;

	if (ngbe_sysfs_init(adapter))
		e_err(probe, "failed to allocate sysfs resources\n");

	ngbe_dbg_adapter_init(adapter);

	if (NGBE_DIS_COMP_TIMEOUT == 1) {
		pcie_capability_read_word(pdev, PCI_EXP_DEVCTL2, &pvalue);
		pvalue = pvalue | 0x10;
		pcie_capability_write_word(pdev, PCI_EXP_DEVCTL2, pvalue);
		adapter->cmplt_to_dis = true;
	}

	return 0;

err_register:
	ngbe_clear_interrupt_scheme(adapter);
	ngbe_release_hw_control(adapter);
err_sw_init:
#ifdef CONFIG_PCI_IOV
	ngbe_disable_sriov(adapter);
#endif /* CONFIG_PCI_IOV */
	adapter->flags2 &= ~NGBE_FLAG2_SEARCH_FOR_SFP;
	kfree(adapter->mac_table);
	iounmap(adapter->io_addr);
	bitmap_free(adapter->af_xdp_zc_qps);
err_ioremap:
	disable_dev = !test_and_set_bit(__NGBE_DISABLED, &adapter->state);
	free_netdev(netdev);
err_alloc_etherdev:
	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
err_pci_reg:
err_dma:
	if (!adapter || disable_dev)
		pci_disable_device(pdev);

	return err;
}

/**
 * ngbe_remove - Device Removal Routine
 * @pdev: PCI device information struct
 * ngbe_remove is called by the PCI subsystem to alert the driver
 * that it should release a PCI device.  The could be caused by a
 * Hot-Plug event, or because the driver is going to be removed from
 * memory.
 **/
static void ngbe_remove(struct pci_dev *pdev)
{
	struct ngbe_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev;
	struct ngbe_hw *hw;
	bool disable_dev;

	/* if !adapter then we already cleaned up in probe */
	if (!adapter)
		return;

	hw = &adapter->hw;
	ngbe_mac_set_default_filter(adapter, hw->mac.perm_addr);

	netdev = adapter->netdev;
	ngbe_dbg_adapter_exit(adapter);

	set_bit(__NGBE_REMOVING, &adapter->state);
	cancel_work_sync(&adapter->service_task);

	ngbe_sysfs_exit(adapter);
	if (adapter->netdev_registered) {
		unregister_netdev(netdev);
		adapter->netdev_registered = false;
	}

#ifdef CONFIG_PCI_IOV
	ngbe_disable_sriov(adapter);
#endif

	ngbe_clear_interrupt_scheme(adapter);
	ngbe_release_hw_control(adapter);

	iounmap(adapter->io_addr);
	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));

	kfree(adapter->mac_table);
	bitmap_free(adapter->af_xdp_zc_qps);
	disable_dev = !test_and_set_bit(__NGBE_DISABLED, &adapter->state);
	free_netdev(netdev);
	if (disable_dev)
		pci_disable_device(pdev);
}

static bool ngbe_check_cfg_remove(struct ngbe_hw *hw, struct pci_dev *pdev)
{
	u16 value;

	pci_read_config_word(pdev, PCI_VENDOR_ID, &value);
	if (value == NGBE_FAILED_READ_CFG_WORD) {
		ngbe_remove_adapter(hw);
		return true;
	}
	return false;
}

u16 ngbe_read_pci_cfg_word(struct ngbe_hw *hw, u32 reg)
{
	struct ngbe_adapter *adapter = hw->back;
	u16 value;

	if (NGBE_REMOVED(hw->hw_addr))
		return NGBE_FAILED_READ_CFG_WORD;
	pci_read_config_word(adapter->pdev, reg, &value);
	if (value == NGBE_FAILED_READ_CFG_WORD && ngbe_check_cfg_remove(hw, adapter->pdev))
		return NGBE_FAILED_READ_CFG_WORD;
	return value;
}

#ifdef CONFIG_PCI_IOV
static u32 ngbe_read_pci_cfg_dword(struct ngbe_hw *hw, u32 reg)
{
	struct ngbe_adapter *adapter = hw->back;
	u32 value;

	if (NGBE_REMOVED(hw->hw_addr))
		return NGBE_FAILED_READ_CFG_DWORD;
	pci_read_config_dword(adapter->pdev, reg, &value);
	if (value == NGBE_FAILED_READ_CFG_DWORD && ngbe_check_cfg_remove(hw, adapter->pdev))
		return NGBE_FAILED_READ_CFG_DWORD;
	return value;
}
#endif /* CONFIG_PCI_IOV */

void ngbe_write_pci_cfg_word(struct ngbe_hw *hw, u32 reg, u16 value)
{
	struct ngbe_adapter *adapter = hw->back;

	if (NGBE_REMOVED(hw->hw_addr))
		return;
	pci_write_config_word(adapter->pdev, reg, value);
}

/**
 * ngbe_io_error_detected - called when PCI error is detected
 * @pdev: Pointer to PCI device
 * @state: The current pci connection state
 * This function is called after a PCI bus error affecting
 * this device has been detected.
 */
static pci_ers_result_t ngbe_io_error_detected(struct pci_dev *pdev, pci_channel_state_t state)
{
	struct ngbe_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;

#ifdef CONFIG_PCI_IOV
	struct ngbe_hw *hw = &adapter->hw;
	struct pci_dev *bdev, *vfdev;
	u32 dw0, dw1, dw2, dw3;
	int vf, pos;
	u16 req_id, pf_func;

	if (adapter->num_vfs == 0)
		goto skip_bad_vf_detection;

	bdev = pdev->bus->self;
	while (bdev && (pci_pcie_type(bdev) != PCI_EXP_TYPE_ROOT_PORT))
		bdev = bdev->bus->self;

	if (!bdev)
		goto skip_bad_vf_detection;

	pos = pci_find_ext_capability(bdev, PCI_EXT_CAP_ID_ERR);
	if (!pos)
		goto skip_bad_vf_detection;

	dw0 = ngbe_read_pci_cfg_dword(hw, pos + PCI_ERR_HEADER_LOG);
	dw1 = ngbe_read_pci_cfg_dword(hw, pos + PCI_ERR_HEADER_LOG + 4);
	dw2 = ngbe_read_pci_cfg_dword(hw, pos + PCI_ERR_HEADER_LOG + 8);
	dw3 = ngbe_read_pci_cfg_dword(hw, pos + PCI_ERR_HEADER_LOG + 12);
	if (NGBE_REMOVED(hw->hw_addr))
		goto skip_bad_vf_detection;

	req_id = dw1 >> 16;
	/* if bit 7 of the requestor ID is set then it's a VF */
	if (!(req_id & 0x0080))
		goto skip_bad_vf_detection;

	pf_func = req_id & 0x01;
	if ((pf_func & 1) == (pdev->devfn & 1)) {
		vf = (req_id & 0x7F) >> 1;
		e_dev_err("VF %d has caused a PCIe error\n", vf);
		e_dev_err("TLP: dw0: %8.8x\tdw1: %8.8x\tdw2: %8.8x\tdw3: %8.8x\n", dw0, dw1, dw2,
			  dw3);

		/* Find the pci device of the offending VF */
		vfdev = pci_get_device(PCI_VENDOR_ID_TRUSTNETIC, NGBE_VF_DEVICE_ID, NULL);
		while (vfdev) {
			if (vfdev->devfn == (req_id & 0xFF))
				break;
			vfdev = pci_get_device(PCI_VENDOR_ID_TRUSTNETIC, NGBE_VF_DEVICE_ID, vfdev);
		}
		/* There's a slim chance the VF could have been hot
		 * plugged, so if it is no longer present we don't need
		 * to issue the VFLR.Just clean up the AER in that case.
		 */
		if (vfdev) {
			ngbe_issue_vf_flr(adapter, vfdev);
			/* Free device reference count */
			pci_dev_put(vfdev);
		}

		pci_aer_clear_nonfatal_status(pdev);
	}

	/* Even though the error may have occurred on the other port
	 * we still need to increment the vf error reference count for
	 * both ports because the I/O resume function will be called
	 * for both of them.
	 */
	adapter->vferr_refcount++;

	return PCI_ERS_RESULT_RECOVERED;

skip_bad_vf_detection:
#endif /* CONFIG_PCI_IOV */

	if (!test_bit(__NGBE_SERVICE_INITED, &adapter->state))
		return PCI_ERS_RESULT_DISCONNECT;

	rtnl_lock();
	netif_device_detach(netdev);

	if (state == pci_channel_io_perm_failure) {
		rtnl_unlock();
		return PCI_ERS_RESULT_DISCONNECT;
	}

	if (netif_running(netdev))
		ngbe_close(netdev);

	if (!test_and_set_bit(__NGBE_DISABLED, &adapter->state))
		pci_disable_device(pdev);
	rtnl_unlock();

	/* Request a slot reset. */
	return PCI_ERS_RESULT_NEED_RESET;
}

/**
 * ngbe_io_slot_reset - called after the pci bus has been reset.
 * @pdev: Pointer to PCI device
 * Restart the card from scratch, as if from a cold-boot.
 */
static pci_ers_result_t ngbe_io_slot_reset(struct pci_dev *pdev)
{
	struct ngbe_adapter *adapter = pci_get_drvdata(pdev);
	pci_ers_result_t result = PCI_ERS_RESULT_RECOVERED;
	u16 value;

	if (adapter->cmplt_to_dis) {
		pcie_capability_read_word(adapter->pdev, PCI_EXP_DEVCTL2, &value);
		value |= 0x10;
		pcie_capability_write_word(adapter->pdev, PCI_EXP_DEVCTL2, value);
	}

	if (pci_enable_device_mem(pdev)) {
		e_err(probe, "Cannot re-enable PCI device after reset.\n");
		result = PCI_ERS_RESULT_DISCONNECT;
	} else {
		/* make sure to clear flag */
		smp_mb__before_atomic();
		clear_bit(__NGBE_DISABLED, &adapter->state);
		adapter->hw.hw_addr = adapter->io_addr;
		pci_set_master(pdev);
		pci_restore_state(pdev);
		/* After second error pci->state_saved is false, this
		 * resets it so EEH doesn't break.
		 */
		pci_save_state(pdev);

		pci_wake_from_d3(pdev, false);

		ngbe_reset(adapter);

		result = PCI_ERS_RESULT_RECOVERED;
	}

	pci_aer_clear_nonfatal_status(pdev);

	return result;
}

/**
 * ngbe_io_resume - called when traffic can start flowing again.
 * @pdev: Pointer to PCI device
 * This callback is called when the error recovery driver tells us that
 * its OK to resume normal operation.
 */
static void ngbe_io_resume(struct pci_dev *pdev)
{
	struct ngbe_adapter *adapter = pci_get_drvdata(pdev);
	struct net_device *netdev = adapter->netdev;

#ifdef CONFIG_PCI_IOV
	if (adapter->vferr_refcount) {
		e_info(drv, "Resuming after VF err\n");
		adapter->vferr_refcount--;
		return;
	}

#endif
	rtnl_lock();
	if (netif_running(netdev))
		ngbe_open(netdev);

	netif_device_attach(netdev);
	rtnl_unlock();
}

static const struct pci_error_handlers ngbe_err_handler = {
	.error_detected = ngbe_io_error_detected,
	.slot_reset = ngbe_io_slot_reset,
	.resume = ngbe_io_resume,
};

struct net_device *ngbe_hw_to_netdev(const struct ngbe_hw *hw)
{
	return ((struct ngbe_adapter *)hw->back)->netdev;
}

struct ngbe_msg *ngbe_hw_to_msg(const struct ngbe_hw *hw)
{
	struct ngbe_adapter *adapter = container_of(hw, struct ngbe_adapter, hw);

	return (struct ngbe_msg *)&adapter->msg_enable;
}

static const struct dev_pm_ops ngbe_pm_ops = {
	.suspend = ngbe_suspend,
	.resume = ngbe_resume,
	.freeze = ngbe_freeze,
	.thaw = ngbe_thaw,
	.poweroff = ngbe_suspend,
	.restore = ngbe_resume,
};

static struct pci_driver ngbe_driver = {.name = ngbe_driver_name,
					.id_table = ngbe_pci_tbl,
					.probe = ngbe_probe,
					.remove = ngbe_remove,
					.driver = {
							.pm = &ngbe_pm_ops,
						},
					.shutdown = ngbe_shutdown,

					.sriov_configure = ngbe_pci_sriov_configure,

					.err_handler = &ngbe_err_handler};

/**
 * ngbe_init_module - Driver Registration Routine
 * ngbe_init_module is the first routine called when the driver is
 * loaded. All it does is register with the PCI subsystem.
 **/
static int __init ngbe_init_module(void)
{
	int ret;

	pr_info("%s - version %s\n", ngbe_driver_string, ngbe_driver_version);
	pr_info("%s\n", ngbe_copyright);

	ngbe_wq = create_singlethread_workqueue(ngbe_driver_name);
	if (!ngbe_wq) {
		pr_err("%s: Failed to create workqueue\n", ngbe_driver_name);
		return -ENOMEM;
	}

	ngbe_dbg_init();

	ret = pci_register_driver(&ngbe_driver);
	return ret;
}

module_init(ngbe_init_module);

/**
 * ngbe_exit_module - Driver Exit Cleanup Routine
 * ngbe_exit_module is called just before the driver is removed
 * from memory.
 **/
static void __exit ngbe_exit_module(void)
{
	pci_unregister_driver(&ngbe_driver);
	destroy_workqueue(ngbe_wq);
	ngbe_dbg_exit();
}

module_exit(ngbe_exit_module);

/* ngbe_main.c */

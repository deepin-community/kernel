/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef _RNP_TYPE_H_
#define _RNP_TYPE_H_

#include <linux/types.h>
#include <linux/mdio.h>
#include <linux/netdevice.h>

#if defined(CONFIG_MXGBE_FIX_VF_QUEUE) && !defined(FIX_VF_BUG)
#define FIX_VF_BUG
#endif
#if defined(CONFIG_MXGBE) && !defined(N10)
#define N10
#endif

#if defined(CONFIG_MXGBE_FIX_MAC_PADDING) && !defined(FIX_MAC_PADDIN)
#define FIX_MAC_PADDIN
#endif

#if defined(CONFIG_MXGBE_OPTM_WITH_LARGE) && !defined(OPTM_WITH_LPAGE)
#define OPTM_WITH_LPAGE
#endif

#if defined(CONFIG_MXGBE_MSIX_COUNT)
#define RNP_N10_MSIX_VECTORS CONFIG_MXGBE_MSIX_COUNT
#endif

// if kylin os, try to set OPTM_WITH_LPAGE to reduce memory cost?
#if (PAGE_SIZE < 8192)
//error
#ifdef OPTM_WITH_LPAGE
//#error can't open OPTM_WITH_LPAGE with PAGE_SIZE small than 8192
#undef OPTM_WITH_LPAGE
#endif
#endif

/* not open it in default */
//#define VF_PROMISC_SUPPORT

#if (PAGE_SIZE < 8192)
/* if page_size is 4k, no need use this */
#ifdef OPTM_WITH_LPAGE
#undef OPTM_WITH_LPAGE
#endif /* OPTM_WITH_LARGE */
#endif

#include "rnp_regs.h"

#if IS_ENABLED(CONFIG_SYSFS)
#ifndef RNP_SYSFS
#define RNP_SYSFS
#endif /* RNP_SYSFS */
#endif /* CONFIG_SYSFS */

#if IS_ENABLED(CONFIG_HWMON)
#ifndef RNP_HWMON
#define RNP_HWMON
#endif /* RNP_HWMON */
#endif /* CONFIG_HWMON */

#ifdef CONFIG_DEBUG_FS
#define HAVE_RNP_DEBUG_FS
#endif /* CONFIG_DEBUG_FS */

/* Device IDs */
#define PCI_VENDOR_ID_MUCSE 0x8848
#define PCI_DEVICE_ID_N10_PF0 0x1000
#define PCI_DEVICE_ID_N10_PF1 0x1001

#define RNP_DEV_ID_N10_PF0 0x7001
#define RNP_DEV_ID_N10_PF1 0x7002

#define PCI_DEVICE_ID_N10 0x1000
#define PCI_DEVICE_ID_N10_TP 0x1004
#define PCI_DEVICE_ID_N10_X1 0x1002
#define PCI_DEVICE_ID_N10C 0x1C00
#define PCI_DEVICE_ID_N400 0x1001 /* N400  2-port */
#define PCI_DEVICE_ID_N400C 0x1C01 /* N400C 2-port */
#define PCI_DEVICE_ID_N400_X1 0x1003 /* N400  1-port */
#define PCI_DEVICE_ID_N400C_X1 0x1C03 /* N400C 1-port */
/* Wake Up Control */
#define RNP_WUC_PME_EN 0x00000002 /* PME Enable */
#define RNP_WUC_PME_STATUS 0x00000004 /* PME Status */
#define RNP_WUC_WKEN 0x00000010 /* Enable PE_WAKE_N pin assertion  */

/* Wake Up Filter Control */
#define RNP_WUFC_LNKC 0x00000001 /* Link Status Change Wakeup Enable */
#define RNP_WUFC_MAG 0x00000002 /* Magic Packet Wakeup Enable */
#define RNP_WUFC_EX 0x00000004 /* Directed Exact Wakeup Enable */
#define RNP_WUFC_MC 0x00000008 /* Directed Multicast Wakeup Enable */
#define RNP_WUFC_BC 0x00000010 /* Broadcast Wakeup Enable */
#define RNP_WUFC_ARP 0x00000020 /* ARP Request Packet Wakeup Enable */
#define RNP_WUFC_IPV4 0x00000040 /* Directed IPv4 Packet Wakeup Enable */
#define RNP_WUFC_IPV6 0x00000080 /* Directed IPv6 Packet Wakeup Enable */
#define RNP_WUFC_MNG 0x00000100 /* Directed Mgmt Packet Wakeup Enable */

#define RNP_WUFC_IGNORE_TCO 0x00008000 /* Ignore WakeOn TCO packets */
#define RNP_WUFC_FLX0 0x00010000 /* Flexible Filter 0 Enable */
#define RNP_WUFC_FLX1 0x00020000 /* Flexible Filter 1 Enable */
#define RNP_WUFC_FLX2 0x00040000 /* Flexible Filter 2 Enable */
#define RNP_WUFC_FLX3 0x00080000 /* Flexible Filter 3 Enable */
#define RNP_WUFC_FLX4 0x00100000 /* Flexible Filter 4 Enable */
#define RNP_WUFC_FLX5 0x00200000 /* Flexible Filter 5 Enable */
#define RNP_WUFC_FLX_FILTERS 0x000F0000 /* Mask for 4 flex filters */
#define RNP_WUFC_FLX_FILTERS_6 0x003F0000 /* Mask for 6 flex filters */
#define RNP_WUFC_FLX_FILTERS_8 0x00FF0000 /* Mask for 8 flex filters */
#define RNP_WUFC_FW_RST_WK 0x80000000 /* Ena wake on FW reset assertion */
/* Mask for Ext. flex filters */
#define RNP_WUFC_EXT_FLX_FILTERS 0x00300000
#define RNP_WUFC_ALL_FILTERS 0x000F00FF /* Mask all 4 flex filters */
#define RNP_WUFC_ALL_FILTERS_6 0x003F00FF /* Mask all 6 flex filters */
#define RNP_WUFC_ALL_FILTERS_8 0x00FF00FF /* Mask all 8 flex filters */
#define RNP_WUFC_FLX_OFFSET 16 /* Offset to the Flexible Filters bits */

#define RNP_MAX_SENSORS 1
struct rnp_thermal_diode_data {
	u8 location;
	u8 temp;
	u8 caution_thresh;
	u8 max_op_thresh;
};

struct rnp_thermal_sensor_data {
	struct rnp_thermal_diode_data sensor[RNP_MAX_SENSORS];
};

/* Proxy Status */
#define RNP_PROXYS_EX 0x00000004 /* Exact packet received */
#define RNP_PROXYS_ARP_DIR 0x00000020 /* ARP w/filter match received */
#define RNP_PROXYS_NS 0x00000200 /* IPV6 NS received */
#define RNP_PROXYS_NS_DIR 0x00000400 /* IPV6 NS w/DA match received */
#define RNP_PROXYS_ARP 0x00000800 /* ARP request packet received */
#define RNP_PROXYS_MLD 0x00001000 /* IPv6 MLD packet received */

/* Proxying Filter Control */
#define RNP_PROXYFC_ENABLE 0x00000001 /* Port Proxying Enable */
#define RNP_PROXYFC_EX 0x00000004 /* Directed Exact Proxy Enable */
#define RNP_PROXYFC_ARP_DIR 0x00000020 /* Directed ARP Proxy Enable */
#define RNP_PROXYFC_NS 0x00000200 /* IPv6 Neighbor Solicitation */
#define RNP_PROXYFC_ARP 0x00000800 /* ARP Request Proxy Enable */
#define RNP_PROXYFC_MLD 0x00000800 /* IPv6 MLD Proxy Enable */
#define RNP_PROXYFC_NO_TCO 0x00008000 /* Ignore TCO packets */

#define RNP_WUPL_LENGTH_MASK 0xFFFF

/* max 4 in n10 */
#define RNP_MAX_TRAFFIC_CLASS 4
#define TSRN10_TX_DEFAULT_BURST 16

#ifndef TSRN10_RX_DEFAULT_BURST
#define TSRN10_RX_DEFAULT_BURST 16
#endif

#ifndef TSRN10_RX_DEFAULT_LINE
#define TSRN10_RX_DEFAULT_LINE 64
#endif

#ifndef RNP_PKT_TIMEOUT
#define RNP_PKT_TIMEOUT 30
#endif

#ifndef RNP_RX_PKT_POLL_BUDGET
#define RNP_RX_PKT_POLL_BUDGET 64
#endif

#ifndef RNP_TX_PKT_POLL_BUDGET
#define RNP_TX_PKT_POLL_BUDGET 0x30
#endif

#ifndef RNP_PKT_TIMEOUT_TX
#define RNP_PKT_TIMEOUT_TX 100
#endif
/* VF Device IDs */
#define RNP_DEV_ID_N10_PF0_VF 0x8001
#define RNP_DEV_ID_N10_PF1_VF 0x8002

#define RNP_DEV_ID_N10_PF0_VF_N 0x1010
#define RNP_DEV_ID_N10_PF1_VF_N 0x1011

/* Transmit Descriptor - Advanced */
struct rnp_tx_desc {
	union {
		__le64 pkt_addr; // Packet buffer address
		struct {
			__le32 adr_lo;
			__le32 adr_hi;
		};
	};
	union {
		__le64 vlan_cmd_bsz;
		struct {
			__le32 blen_mac_ip_len;
			__le32 vlan_cmd;
		};
	};
#define RNP_TXD_FLAGS_VLAN_PRIO_MASK 0xe000
#define RNP_TX_FLAGS_VLAN_PRIO_SHIFT 13
#define RNP_TX_FLAGS_VLAN_CFI_SHIFT 12
#define RNP_TXD_VLAN_VALID (0x80000000)
#define RNP_TXD_SVLAN_TYPE (0x02000000)
#define RNP_TXD_VLAN_CTRL_NOP (0x00 << 13)
#define RNP_TXD_VLAN_CTRL_RM_VLAN (0x20000000)
#define RNP_TXD_VLAN_CTRL_INSERT_VLAN (0x40000000)
#define RNP_TXD_L4_CSUM (0x10000000) /* udp tcp sctp csum */
#define RNP_TXD_IP_CSUM (0x8000000)
#define RNP_TXD_TUNNEL_MASK (0x3000000)
#define RNP_TXD_TUNNEL_VXLAN (0x1000000)
#define RNP_TXD_TUNNEL_NVGRE (0x2000000)
#define RNP_TXD_L4_TYPE_UDP (0xc00000)
#define RNP_TXD_L4_TYPE_TCP (0x400000)
#define RNP_TXD_L4_TYPE_SCTP (0x800000)
#define RNP_TXD_FLAG_IPv4 (0)
#define RNP_TXD_FLAG_IPv6 (0x200000)
#define RNP_TXD_FLAG_TSO (0x100000)
#define RNP_TXD_FLAG_PTP (0x4000000)
#define RNP_TXD_CMD_RS (0x040000)
#define RNP_TXD_CMD_INNER_VLAN (0x08000000)
#define RNP_TXD_STAT_DD (0x020000)
#define RNP_TXD_CMD_EOP (0x010000)
#define RNP_TXD_PAD_CTRL (0x01000000)
};

struct rnp_tx_ctx_desc {
	__le32 mss_len_vf_num;
	__le32 inner_vlan_tunnel_len;
#define VF_VEB_MARK (1 << 24) /* bit 56 */
#define VF_VEB_IGNORE_VLAN (1 << 25) /* bit 57 */
	__le32 resv;
	__le32 resv_cmd;
#define RNP_TXD_FLAG_TO_RPU (1 << 15)
#define RNP_TXD_SMAC_CTRL_NOP (0x00 << 12)
#define RNP_TXD_SMAC_CTRL_REPLACE_MACADDR0 (0x02 << 12)
#define RNP_TXD_SMAC_CTRL_REPLACE_MACADDR1 (0x06 << 12)
#define RNP_TXD_CTX_VLAN_CTRL_NOP (0x00 << 10)
#define RNP_TXD_CTX_VLAN_CTRL_RM_VLAN (0x01 << 10)
#define RNP_TXD_CTX_VLAN_CTRL_INSERT_VLAN (0x02 << 10)
#define RNP_TXD_MTI_CRC_PAD_CTRL (0x01000000)
#define RNP_TXD_CTX_CTRL_DESC (0x080000)
#define RNP_TXD_CMD_RS (0x040000)
#define RNP_TXD_STAT_DD (0x020000)
};

/* Receive Descriptor - Advanced */
union rnp_rx_desc {
	struct {
		union {
			__le64 pkt_addr; /* Packet buffer address */
			struct {
				__le32 addr_lo;
				__le32 addr_hi;
			};
		};
		__le64 resv_cmd;
#define RNP_RXD_FLAG_RS (0)
	};

	struct {
		__le32 rss_hash;
		__le16 mark;
		__le16 rev1;
#define RNP_RX_L3_TYPE_MASK (1 << 15) /* 1 is ipv4 */
#define VEB_VF_PKG (1 << 0) /* bit 48 */
#define VEB_VF_IGNORE_VLAN (1 << 1) /* bit 49 */
#define REV_OUTER_VLAN (1 << 5)
		__le16 len;
		__le16 padding_len;
		__le16 vlan;
		__le16 cmd;
#define RNP_RXD_STAT_VLAN_VALID (1 << 15)
#define RNP_RXD_STAT_STAG (0x01 << 14)
#define RNP_RXD_STAT_TUNNEL_NVGRE (0x02 << 13)
#define RNP_RXD_STAT_TUNNEL_VXLAN (0x01 << 13)
#define RNP_RXD_STAT_TUNNEL_MASK (0x03 << 13)
#define RNP_RXD_STAT_ERR_MASK (0x1f << 8)
#define RNP_RXD_STAT_SCTP_MASK (0x04 << 8)
#define RNP_RXD_STAT_L4_MASK (0x02 << 8)
#define RNP_RXD_STAT_L4_SCTP (0x02 << 6)
#define RNP_RXD_STAT_L4_TCP (0x01 << 6)
#define RNP_RXD_STAT_L4_UDP (0x03 << 6)
#define RNP_RXD_STAT_IPV6 (1 << 5)
#define RNP_RXD_STAT_IPV4 (0 << 5)
#define RNP_RXD_STAT_PTP (1 << 4)
#define RNP_RXD_STAT_DD (1 << 1)
#define RNP_RXD_STAT_EOP (1 << 0)
	} wb;
} __packed;

/* Host Interface Command Structures */
struct rnp_hic_hdr {
	u8 cmd;
	u8 buf_len;
	union {
		u8 cmd_resv;
		u8 ret_status;
	} cmd_or_resp;
	u8 checksum;
};

struct rnp_hic_drv_info {
	struct rnp_hic_hdr hdr;
	u8 port_num;
	u8 ver_sub;
	u8 ver_build;
	u8 ver_min;
	u8 ver_maj;
	u8 pad; /* end spacing to ensure length is mult. of dword */
	u16 pad2; /* end spacing to ensure length is mult. of dword2 */
};

/* Context descriptors */
struct rnp_adv_tx_context_desc {
	__le32 vlan_macip_lens;
	__le32 seqnum_seed;
	__le32 type_tucmd_mlhl;
	__le32 mss_l4len_idx;
};

/* RAH */
#define RNP_RAH_VIND_MASK 0x003C0000
#define RNP_RAH_VIND_SHIFT 18
#define RNP_RAH_AV 0x80000000
#define RNP_CLEAR_VMDQ_ALL 0xFFFFFFFF

/* Autonegotiation advertised speeds */
typedef u32 rnp_autoneg_advertised;
/* Link speed */
typedef u32 rnp_link_speed;
#define RNP_LINK_SPEED_UNKNOWN 0
#define RNP_LINK_SPEED_10_FULL BIT(2)
#define RNP_LINK_SPEED_100_FULL BIT(3)
#define RNP_LINK_SPEED_1GB_FULL BIT(4)
#define RNP_LINK_SPEED_10GB_FULL BIT(5)
#define RNP_LINK_SPEED_40GB_FULL BIT(6)
#define RNP_LINK_SPEED_25GB_FULL BIT(7)
#define RNP_LINK_SPEED_50GB_FULL BIT(8)
#define RNP_LINK_SPEED_100GB_FULL BIT(9)
#define RNP_LINK_SPEED_10_HALF BIT(10)
#define RNP_LINK_SPEED_100_HALF BIT(11)
#define RNP_LINK_SPEED_1GB_HALF BIT(12)
#define RNP_SFP_MODE_10G_LR BIT(13)
#define RNP_SFP_MODE_10G_SR BIT(14)
#define RNP_SFP_MODE_10G_LRM BIT(15)
#define RNP_SFP_MODE_1G_T BIT(16)
#define RNP_SFP_MODE_1G_KX BIT(17)
#define RNP_SFP_MODE_1G_SX BIT(18)
#define RNP_SFP_MODE_1G_LX BIT(19)
#define RNP_SFP_MODE_40G_SR4 BIT(20)
#define RNP_SFP_MODE_40G_CR4 BIT(21)
#define RNP_SFP_MODE_40G_LR4 BIT(22)
#define RNP_SFP_MODE_1G_CX BIT(23)
#define RNP_SFP_MODE_10G_BASE_T BIT(24)
#define RNP_SFP_MODE_FIBER_CHANNEL_SPEED BIT(25)
#define RNP_SFP_CONNECTOR_DAC BIT(26)
#define RNP_SFP_TO_SGMII BIT(27)
#define RNP_SFP_25G_SR BIT(28)
#define RNP_SFP_25G_KR BIT(29)
#define RNP_SFP_25G_CR BIT(30)
#define RNP_LINK_SPEED_10GB_HALF BIT(31)

/* Flow Control Data Sheet defined values
 * Calculation and defines taken from 802.1bb Annex O
 */

enum rnp_atr_flow_type {
	RNP_ATR_FLOW_TYPE_IPV4 = 0x0,
	RNP_ATR_FLOW_TYPE_UDPV4 = 0x1,
	RNP_ATR_FLOW_TYPE_TCPV4 = 0x2,
	RNP_ATR_FLOW_TYPE_SCTPV4 = 0x3,
	RNP_ATR_FLOW_TYPE_IPV6 = 0x4,
	RNP_ATR_FLOW_TYPE_UDPV6 = 0x5,
	RNP_ATR_FLOW_TYPE_TCPV6 = 0x6,
	RNP_ATR_FLOW_TYPE_SCTPV6 = 0x7,
	RNP_ATR_FLOW_TYPE_TUNNELED_IPV4 = 0x10,
	RNP_ATR_FLOW_TYPE_TUNNELED_UDPV4 = 0x11,
	RNP_ATR_FLOW_TYPE_TUNNELED_TCPV4 = 0x12,
	RNP_ATR_FLOW_TYPE_TUNNELED_SCTPV4 = 0x13,
	RNP_ATR_FLOW_TYPE_TUNNELED_IPV6 = 0x14,
	RNP_ATR_FLOW_TYPE_TUNNELED_UDPV6 = 0x15,
	RNP_ATR_FLOW_TYPE_TUNNELED_TCPV6 = 0x16,
	RNP_ATR_FLOW_TYPE_TUNNELED_SCTPV6 = 0x17,
	RNP_ATR_FLOW_TYPE_ETHER = 0x18,
	RNP_ATR_FLOW_TYPE_USERDEF = 0x19,
};

#define RNP_FDIR_DROP_QUEUE (200)

enum {
	fdir_mode_tcam = 0,
	fdir_mode_tuple5,
};
/* Flow Director ATR input struct. */
union rnp_atr_input {
	/*
	 * Byte layout in order, all values with MSB first:
	 *
	 * vm_pool      - 1 byte
	 * flow_type    - 1 byte
	 * vlan_id      - 2 bytes
	 * src_ip       - 16 bytes
	 * inner_mac    - 6 bytes
	 * cloud_mode   - 2 bytes
	 * tni_vni      - 4 bytes
	 * dst_ip       - 16 bytes
	 * src_port     - 2 bytes
	 * dst_port     - 2 bytes
	 * flex_bytes   - 2 bytes
	 * bkt_hash     - 2 bytes
	 */
	struct {
		u8 vm_pool;
		u8 flow_type;
		__be16 vlan_id;
		__be32 dst_ip[4];
		__be32 dst_ip_mask[4];
		__be32 src_ip[4];
		__be32 src_ip_mask[4];
		u8 inner_mac[6];
		u8 inner_mac_mask[6];
		__be16 tunnel_type;
		__be32 tni_vni;
		__be16 src_port;
		__be16 src_port_mask;
		__be16 dst_port;
		__be16 dst_port_mask;
		__be16 flex_bytes;
		__be16 bkt_hash;
	} formatted;
	struct {
		u8 vm_poll;
		u8 flow_type;
		u16 vlan_id;
		__be16 proto;
		__be16 resv;
		__be32 nouse[12];
	} layer2_formate;
	__be32 dword_stream[14];
};

/* BitTimes (BT) conversion */
#define RNP_BT2KB(BT) ((BT + (8 * 1024 - 1)) / (8 * 1024))
#define RNP_B2BT(BT) (BT * 8)

/* Calculate Delay to respond to PFC */
#define RNP_PFC_D 672

/* Calculate Cable Delay */
#define RNP_CABLE_DC 5556 /* Delay Copper */
#define RNP_CABLE_DO 5000 /* Delay Optical */

/* Calculate Interface Delay X540 */
#define RNP_PHY_DC 25600 /* Delay 10G BASET */
#define RNP_MAC_DC 8192 /* Delay Copper XAUI interface */
#define RNP_XAUI_DC (2 * 2048) /* Delay Copper Phy */

#define RNP_ID_X540 (RNP_MAC_DC + RNP_XAUI_DC + RNP_PHY_DC)

/* Calculate Interface Delay 82598, n10 */
#define RNP_PHY_D 12800
#define RNP_MAC_D 4096
#define RNP_XAUI_D (2 * 1024)

/* PHY MDI STANDARD CONFIG */
#define RNP_MDI_PHY_ID1_OFFSET 2
#define RNP_MDI_PHY_ID2_OFFSET 3
#define RNP_MDI_PHY_ID_MASK 0xFFFFFC00U
#define RNP_MDI_PHY_SPEED_SELECT1 0x0040
#define RNP_MDI_PHY_DUPLEX 0x0100
#define RNP_MDI_PHY_RESTART_AN 0x0200
#define RNP_MDI_PHY_ANE 0x1000
#define RNP_MDI_PHY_SPEED_SELECT0 0x2000
#define RNP_MDI_PHY_RESET

#define NGBE_PHY_RST_WAIT_PERIOD 50

#define RNP_ID (RNP_MAC_D + RNP_XAUI_D + RNP_PHY_D)

/* Calculate Delay incurred from higher layer */
#define RNP_HD 6144

/* Calculate PCI Bus delay for low thresholds */
#define RNP_PCI_DELAY 10000

/* Flow Director compressed ATR hash input struct */
union rnp_atr_hash_dword {
	struct {
		u8 vm_pool;
		u8 flow_type;
		__be16 vlan_id;
	} formatted;
	__be32 ip;
	struct {
		__be16 src;
		__be16 dst;
	} port;
	__be16 flex_bytes;
	__be32 dword;
};

enum rnp_eeprom_type {
	rnp_eeprom_uninitialized = 0,
	rnp_eeprom_spi,
	rnp_flash,
	rnp_eeprom_none /* No NVM support */
};

enum mac_type {
	mac_dwc_xlg,
	mac_dwc_g,

};

enum rnp_mac_type {
	rnp_mac_unknown = 0,
	rnp_mac_n10g_x8_40G,
	rnp_mac_n10g_x2_10G,
	rnp_mac_n10g_x4_10G,
	rnp_mac_n10g_x8_10G,
	rnp_mac_n10l_x8_1G,
	rnp_num_macs
};

enum rnp_rss_type {
	rnp_rss_uv440 = 0,
	rnp_rss_uv3p,
	rnp_rss_n10,
	rnp_rss_n20,
};

enum rnp_hw_type {
	rnp_hw_uv440 = 0,
	rnp_hw_uv3p,
	rnp_hw_n10,
	rnp_hw_n20,
	rnp_hw_n400
};

enum rnp_eth_type { rnp_eth_n10 = 0 };

enum rnp_phy_type {
	rnp_phy_unknown = 0,
	rnp_phy_none,
	rnp_phy_sfp,
	rnp_phy_sfp_unsupported,
	rnp_phy_generic,
	rnp_phy_sfp_unknown,
	rnp_phy_sgmii,
};

enum rnp_sfp_type {
	rnp_sfp_type_da_cu = 0,
	rnp_sfp_type_sr = 1,
	rnp_sfp_type_lr = 2,
	rnp_sfp_type_da_cu_core0 = 3,
	rnp_sfp_type_da_cu_core1 = 4,
	rnp_sfp_type_srlr_core0 = 5,
	rnp_sfp_type_srlr_core1 = 6,
	rnp_sfp_type_da_act_lmt_core0 = 7,
	rnp_sfp_type_da_act_lmt_core1 = 8,
	rnp_sfp_type_1g_cu_core0 = 9,
	rnp_sfp_type_1g_cu_core1 = 10,
	rnp_sfp_type_1g_sx_core0 = 11,
	rnp_sfp_type_1g_sx_core1 = 12,
	rnp_sfp_type_1g_lx_core0 = 13,
	rnp_sfp_type_1g_lx_core1 = 14,
	rnp_sfp_type_not_present = 0xFFFE,
	rnp_sfp_type_unknown = 0xFFFF
};

enum rnp_media_type {
	rnp_media_type_unknown = 0,
	rnp_media_type_fiber,
	rnp_media_type_copper,
	rnp_media_type_backplane,
	rnp_media_type_cx4,
	rnp_media_type_da,
	rnp_media_type_virtual

};

/* Flow Control Settings */
enum rnp_fc_mode {
	rnp_fc_none = 0,
	rnp_fc_rx_pause,
	rnp_fc_tx_pause,
	rnp_fc_full,
	rnp_fc_default
};

#define PAUSE_TX (0x1)
#define PAUSE_RX (0x2)
#define PAUSE_AUTO (0x10)

#define ASYM_PAUSE BIT(11)
#define SYM_PAUSE BIT(10)

struct rnp_addr_filter_info {
	u32 num_mc_addrs;
	u32 rar_used_count;
	u32 mta_in_use;
	u32 overflow_promisc;
	bool uc_set_promisc;
	bool user_set_promisc;
};

/* Bus parameters */
struct rnp_bus_info {
	u16 func;
	u16 lan_id;
};

/* Flow control parameters */
struct rnp_fc_info {
	u32 high_water[RNP_MAX_TRAFFIC_CLASS]; /* Flow Control High-water */
	u32 low_water[RNP_MAX_TRAFFIC_CLASS]; /* Flow Control Low-water */
	u16 pause_time; /* Flow Control Pause timer */
	bool send_xon; /* Flow control send XON */
	bool strict_ieee; /* Strict IEEE mode */
	bool disable_fc_autoneg; /* Do not autonegotiate FC */
	bool fc_was_autonegged; /* Is current_mode the result of autonegging? */
	enum rnp_fc_mode current_mode; /* FC mode in effect */
	u32 requested_mode; /* FC mode requested by caller */
};

/* Statistics counters collected by the MAC */
struct rnp_hw_stats {
	u64 dma_to_dma;
	u64 dma_to_switch;
	u64 mac_to_mac;
	u64 switch_to_switch;
	u64 mac_to_dma;
	u64 switch_to_dma;
	u64 vlan_add_cnt;
	u64 vlan_strip_cnt;
	/* === error */
	u64 invalid_dropped_packets;
	u64 filter_dropped_packets;
	/* == drop == */
	u64 rx_capabity_lost;
	u64 host_l2_match_drop;
	u64 redir_input_match_drop;
	u64 redir_etype_match_drop;
	u64 redir_tcp_syn_match_drop;
	u64 redir_tuple5_match_drop;
	u64 redir_tcam_match_drop;

	u64 bmc_dropped_packets;
	u64 switch_dropped_packets;
	/* === rx */
	u64 dma_to_host;
	/* === dma-tx == */
	u64 port0_tx_packets;
	u64 port1_tx_packets;
	u64 port2_tx_packets;
	u64 port3_tx_packets;
	/* === emac 1to4 tx == */
	u64 in0_tx_pkts;
	u64 in1_tx_pkts;
	u64 in2_tx_pkts;
	u64 in3_tx_pkts;
	/* === phy tx == */
	u64 port0_to_phy_pkts;
	u64 port1_to_phy_pkts;
	u64 port2_to_phy_pkts;
	u64 port3_to_phy_pkts;
	/* === mac rx === */
	u64 mac_rx_broadcast;
	u64 mac_rx_multicast;
	u64 mac_rx_pause_count;
	u64 mac_tx_pause_count;
	u64 tx_broadcast;
	u64 tx_multicast;

	u64 dma_rx_drop_cnt_0;
	u64 dma_rx_drop_cnt_1;
	u64 dma_rx_drop_cnt_2;
	u64 dma_rx_drop_cnt_3;
	u64 dma_rx_drop_cnt_4;
	u64 dma_rx_drop_cnt_5;
	u64 dma_rx_drop_cnt_6;
	u64 dma_rx_drop_cnt_7;
};

/* forward declaration */
struct rnp_hw;
struct rnp_eth_info;
struct rnp_dma_info;
struct rnp_mac_info;

/* iterator type for walking multicast address lists */
typedef u8 *(*rnp_mc_addr_itr)(struct rnp_hw *hw, u8 **mc_addr_ptr, u32 *vmdq);

/* Function pointer table */
struct rnp_eeprom_operations {
	s32 (*init_params)(struct rnp_hw *hw);
	s32 (*read)(struct rnp_hw *hw, u16, u16 *);
	s32 (*read_buffer)(struct rnp_hw *, u16, u16, u16 *);
	s32 (*write)(struct rnp_hw *, u16, u16);
	s32 (*write_buffer)(struct rnp_hw *, u16, u16, u16 *);
	s32 (*validate_checksum)(struct rnp_hw *, u16 *);
	s32 (*update_checksum)(struct rnp_hw *);
	u16 (*calc_checksum)(struct rnp_hw *);
};

/* add nic operations */
struct rnp_eth_operations {
	/* RAR, Multicast, VLAN */
	s32 (*get_mac_addr)(struct rnp_eth_info *, u8 *);
	s32 (*set_rar)(struct rnp_eth_info *, u32, u8 *, bool);
	s32 (*clear_rar)(struct rnp_eth_info *, u32);
	s32 (*set_vmdq)(struct rnp_eth_info *, u32, u32);
	s32 (*clear_vmdq)(struct rnp_eth_info *, u32, u32);
	s32 (*update_mc_addr_list)(struct rnp_eth_info *, struct net_device *,
				   bool);
	void (*clr_mc_addr)(struct rnp_eth_info *);
	int (*set_rss_hfunc)(struct rnp_eth_info *, int hfunc);
	void (*set_rss_key)(struct rnp_eth_info *, bool);
	void (*set_rss_table)(struct rnp_eth_info *);
	void (*set_rx_hash)(struct rnp_eth_info *, bool, bool);
	/* ncsi */
	void (*ncsi_set_vfta)(struct rnp_eth_info *);
	void (*ncsi_set_uc_addr)(struct rnp_eth_info *);
	void (*ncsi_set_mc_mta)(struct rnp_eth_info *);
	void (*set_layer2_remapping)(struct rnp_eth_info *,
				     union rnp_atr_input *, u16, u8, bool);
	void (*clr_layer2_remapping)(struct rnp_eth_info *, u16);
	void (*clr_all_layer2_remapping)(struct rnp_eth_info *);
	void (*set_tuple5_remapping)(struct rnp_eth_info *,
				     union rnp_atr_input *, u16, u8, bool);
	void (*clr_tuple5_remapping)(struct rnp_eth_info *, u16);
	void (*clr_all_tuple5_remapping)(struct rnp_eth_info *);
	void (*set_tcp_sync_remapping)(struct rnp_eth_info *, int, bool, bool);
	void (*set_rx_skip)(struct rnp_eth_info *, int, bool);
	void (*set_min_max_packet)(struct rnp_eth_info *, int, int);
	void (*set_vlan_strip)(struct rnp_eth_info *, u16, bool);
	s32 (*set_vfta)(struct rnp_eth_info *, u32, bool);
	void (*clr_vfta)(struct rnp_eth_info *);
	void (*set_vlan_filter)(struct rnp_eth_info *, bool);
	void (*set_outer_vlan_type)(struct rnp_eth_info *, int type);
	void (*set_double_vlan)(struct rnp_eth_info *, bool);
	void (*set_vxlan_port)(struct rnp_eth_info *, u32);
	void (*set_vxlan_mode)(struct rnp_eth_info *, bool);
	s32 (*set_fc_mode)(struct rnp_eth_info *);
	void (*set_rx)(struct rnp_eth_info *, bool);
	void (*set_fcs)(struct rnp_eth_info *, bool);
	void (*set_vf_vlan_mode)(struct rnp_eth_info *, u16, int, bool);
};

enum {
	rnp_driver_insmod,
	rnp_driver_suspuse,
	rnp_driver_force_control_mac,
};

struct rnp_hw_operations {
	s32 (*init_hw)(struct rnp_hw *);
	s32 (*reset_hw)(struct rnp_hw *);
	s32 (*start_hw)(struct rnp_hw *);
	void (*set_mtu)(struct rnp_hw *, int);
	void (*set_vlan_filter_en)(struct rnp_hw *, bool);
	void (*set_vlan_filter)(struct rnp_hw *, u16, bool, bool);
	int (*set_veb_vlan_mask)(struct rnp_hw *, u16, int, bool);
	void (*set_vf_vlan_filter)(struct rnp_hw *, u16, int, bool, bool);
	void (*clr_vfta)(struct rnp_hw *);
	void (*set_vlan_strip)(struct rnp_hw *, u16, bool);
	void (*set_mac)(struct rnp_hw *, u8 *mac, bool);
	void (*set_rx_mode)(struct rnp_hw *, struct net_device *netdev, bool);
	void (*set_rar_with_vf)(struct rnp_hw *hw, u8 *mac, int, u32, bool);
	void (*clr_rar)(struct rnp_hw *hw, int idx);
	void (*clr_rar_all)(struct rnp_hw *hw);
	void (*clr_vlan_veb)(struct rnp_hw *);
	void (*set_txvlan_mode)(struct rnp_hw *, bool);
	void (*set_tx_maxrate)(struct rnp_hw *, bool);
	void (*set_fcs_mode)(struct rnp_hw *, bool);
	void (*set_vxlan_port)(struct rnp_hw *, u32);
	void (*set_vxlan_mode)(struct rnp_hw *, bool);
	void (*set_mac_speed)(struct rnp_hw *, bool, u32, bool);
	void (*set_mac_rx)(struct rnp_hw *, bool);
	void (*update_sriov_info)(struct rnp_hw *);
	void (*set_sriov_status)(struct rnp_hw *, bool);
	void (*set_sriov_vf_mc)(struct rnp_hw *, u16);
	void (*set_pause_mode)(struct rnp_hw *);
	void (*get_pause_mode)(struct rnp_hw *);
	void (*update_hw_info)(struct rnp_hw *);
	void (*set_rx_hash)(struct rnp_hw *, bool, bool);
	int (*set_rss_hfunc)(struct rnp_hw *, u8 hfunc);
	void (*set_rss_key)(struct rnp_hw *, bool);
	void (*set_rss_table)(struct rnp_hw *);
	void (*set_mbx_link_event)(struct rnp_hw *, int);
	void (*set_mbx_ifup)(struct rnp_hw *, int);
	s32 (*get_thermal_sensor_data)(struct rnp_hw *);
	s32 (*init_thermal_sensor_thresh)(struct rnp_hw *hw);
	void (*disable_tx_laser)(struct rnp_hw *);
	void (*enable_tx_laser)(struct rnp_hw *);
	void (*flap_tx_laser)(struct rnp_hw *);
	s32 (*check_link)(struct rnp_hw *, rnp_link_speed *, bool *, bool *,
			  bool);
	s32 (*setup_link)(struct rnp_hw *, rnp_link_speed, u32, u32, u32);
	void (*clean_link)(struct rnp_hw *);
	s32 (*get_link_capabilities)(struct rnp_hw *, rnp_link_speed *, bool *);
	s32 (*init_rx_addrs)(struct rnp_hw *);
	void (*set_layer2_remapping)(struct rnp_hw *, union rnp_atr_input *,
				     u16, u8, bool);
	void (*clr_layer2_remapping)(struct rnp_hw *, u16);
	void (*clr_all_layer2_remapping)(struct rnp_hw *);
	void (*set_tuple5_remapping)(struct rnp_hw *, union rnp_atr_input *,
				     u16, u8, bool);
	void (*clr_tuple5_remapping)(struct rnp_hw *, u16);
	void (*clr_all_tuple5_remapping)(struct rnp_hw *);
	void (*set_tcp_sync_remapping)(struct rnp_hw *, int queue, bool, bool);
	void (*set_rx_skip)(struct rnp_hw *, int count, bool);
	void (*set_outer_vlan_type)(struct rnp_hw *, int);
	void (*update_hw_status)(struct rnp_hw *, struct rnp_hw_stats *,
				 struct net_device_stats *);
	void (*update_msix_count)(struct rnp_hw *, int msix_count);
	void (*update_rx_drop)(struct rnp_hw *);
	void (*setup_ethtool)(struct net_device *);
	s32 (*phy_read_reg)(struct rnp_hw *, u32, u32, u16 *);
	s32 (*phy_write_reg)(struct rnp_hw *, u32, u32, u16);
	void (*setup_wol)(struct rnp_hw *, u32);
	void (*set_vf_vlan_mode)(struct rnp_hw *, u16, int, bool);
	void (*driver_status)(struct rnp_hw *, bool, int);
};

struct rnp_mac_operations {
	void (*set_mac_rx)(struct rnp_mac_info *mac, bool);
	void (*set_mac_speed)(struct rnp_mac_info *, bool, u32, bool);
	void (*set_mac_fcs)(struct rnp_mac_info *mac, bool);
	s32 (*set_fc_mode)(struct rnp_mac_info *mac);
	void (*check_link)(struct rnp_mac_info *, rnp_link_speed *, bool *,
			   bool);
	void (*set_mac)(struct rnp_mac_info *, u8 *, int);
	int (*mdio_write)(struct rnp_mac_info *, int phyreg, int phydata);
	int (*mdio_read)(struct rnp_mac_info *, int phyreg, int *regvalue);
	void (*pmt)(struct rnp_mac_info *, u32);
};

struct rnp_eeprom_info {
	struct rnp_eeprom_operations ops;
	enum rnp_eeprom_type type;
	u32 semaphore_delay;
	u16 word_size;
	u16 address_bits;
	u16 word_page_size;
};

struct rnp_dma_operations {
	void (*set_tx_maxrate)(struct rnp_dma_info *dma, u16, u32);
	void (*set_veb_mac)(struct rnp_dma_info *dma, u8 *, u32, u32);
	/* only set own vlan */
	void (*set_veb_vlan)(struct rnp_dma_info *dma, u16, u32);
	void (*set_veb_vlan_mask)(struct rnp_dma_info *dma, u16, u16, int);
	void (*clr_veb_all)(struct rnp_dma_info *dma);
};

struct rnp_dma_info {
	struct rnp_dma_operations ops;
	u8 __iomem *dma_base_addr;
	u8 __iomem *dma_ring_addr;
	void *back;
	u32 max_tx_queues;
	u32 max_rx_queues;
	u32 dma_version;
};

#define RNP_MAX_MTA 128
struct rnp_eth_info {
	struct rnp_eth_operations ops;
	u8 __iomem *eth_base_addr;
	enum rnp_eth_type eth_type;
	void *back;

	u32 mta_shadow[RNP_MAX_MTA];
	s32 mc_filter_type;
	u32 mcft_size;
	u32 vft_size;
	u32 num_rar_entries;
	u32 rar_highwater;
	u32 rx_pb_size;
	u32 max_tx_queues;
	u32 max_rx_queues;
	u32 reg_off;
	u32 orig_autoc;
	u32 cached_autoc;
	u32 orig_autoc2;
};

struct rnp_nic_info {
	u8 __iomem *nic_base_addr;
};

struct mii_regs {
	unsigned int addr; /* MII Address */
	unsigned int data; /* MII Data */
	unsigned int addr_shift; /* MII address shift */
	unsigned int reg_shift; /* MII reg shift */
	unsigned int addr_mask; /* MII address mask */
	unsigned int reg_mask; /* MII reg mask */
	unsigned int clk_csr_shift;
	unsigned int clk_csr_mask;
};

#define RNP_FLAGS_DOUBLE_RESET_REQUIRED 0x01
#define RNP_FLAGS_INIT_MAC_ADDRESS 0x02
struct rnp_mac_info {
	struct rnp_mac_operations ops;
	u8 __iomem *mac_addr;
	void *back;
	struct mii_regs mii;
	int phy_addr;
	int clk_csr;
	enum rnp_mac_type type;
	enum mac_type mac_type;
	u8 addr[ETH_ALEN];
	u8 perm_addr[ETH_ALEN];
	/* prefix for World Wide Node Name (WWNN) */
	u16 wwnn_prefix;
	/* prefix for World Wide Port Name (WWPN) */
	u16 wwpn_prefix;
	u16 max_msix_vectors;
	u32 mta_shadow[RNP_MAX_MTA];
	s32 mc_filter_type;
	u32 mcft_size;
	u32 vft_size;
	u32 num_rar_entries;
	u32 rar_highwater;
	u32 rx_pb_size;
	u32 max_tx_queues;
	u32 max_rx_queues;
	u32 reg_off;
	u32 orig_autoc;
	u32 cached_autoc;
	u32 orig_autoc2;
	bool orig_link_settings_stored;
	bool autotry_restart;
	u8 mac_flags;
};

struct rnp_phy_info {
	struct mdio_if_info mdio;
	enum rnp_phy_type type;
	u32 id;
	u32 phy_addr;
	bool is_mdix;
	u8 mdix;
	enum rnp_sfp_type sfp_type;
	bool sfp_setup_needed;
	u32 revision;
	enum rnp_media_type media_type;
	bool reset_disable;
	rnp_autoneg_advertised autoneg_advertised;
	bool smart_speed_active;
	bool multispeed_fiber;
	bool reset_if_overtemp;
};

#include "rnp_mbx.h"

struct rnp_pcs_operations {
	u32 (*read)(struct rnp_hw *hw, int num, u32 addr);
	void (*write)(struct rnp_hw *hw, int num, u32 addr, u32 value);
};

struct rnp_mbx_operations {
	s32 (*init_params)(struct rnp_hw *hw);
	s32 (*read)(struct rnp_hw *, u32 *, u16, enum MBX_ID);
	s32 (*write)(struct rnp_hw *, u32 *, u16, enum MBX_ID);
	s32 (*read_posted)(struct rnp_hw *, u32 *, u16, enum MBX_ID);
	s32 (*write_posted)(struct rnp_hw *, u32 *, u16, enum MBX_ID);
	s32 (*check_for_msg)(struct rnp_hw *, enum MBX_ID);
	s32 (*check_for_ack)(struct rnp_hw *, enum MBX_ID);
	s32 (*configure)(struct rnp_hw *hw, int nr_vec, bool enable);
};

struct rnp_mbx_stats {
	u32 msgs_tx;
	u32 msgs_rx;
	u32 acks;
	u32 reqs;
	u32 rsts;
};

struct rnp_pcs_info {
	struct rnp_pcs_operations ops;
	int pcs_count;
};

struct mbx_fw_cmd_reply;

typedef void (*cookie_cb)(struct mbx_fw_cmd_reply *reply, void *priv);

enum cookie_stat{
	COOKIE_FREE=0,
	COOKIE_FREE_WAIT_TIMEOUT,
	COOKIE_ALLOCED,
};

struct mbx_req_cookie {
	u64 alloced_jiffies;
	enum cookie_stat stat;
	cookie_cb cb;
	int timeout_jiffes;
	int errcode;
	wait_queue_head_t wait;
	int done;
	int priv_len;
#define MAX_PRIV_LEN 64
	char priv[MAX_PRIV_LEN];
};

struct mbx_req_cookie_pool {
#define MAX_COOKIES_ITEMS (20*400)
	struct mbx_req_cookie cookies[MAX_COOKIES_ITEMS];
	int next_idx;
};

struct rnp_mbx_info {
	struct rnp_mbx_operations ops;
	struct rnp_mbx_stats stats;
	u32 timeout;
	u32 usec_delay;
	u32 v2p_mailbox;
	u16 size;
	u16 vf_req[64];
	u16 vf_ack[64];
	u16 cpu_req;
	u16 cpu_ack;
	struct mutex lock;
	bool other_irq_enabled;
	int mbx_size;
	int mbx_mem_size;
#define MBX_FEATURE_NO_ZERO BIT(0)
#define MBX_FEATURE_WRITE_DELAY BIT(1)
	u32 mbx_feature;
	/* cm3 <-> pf mbx */
	u32 cpu_pf_shm_base;
	u32 pf2cpu_mbox_ctrl;
	u32 pf2cpu_mbox_mask;
	u32 cpu_pf_mbox_mask;
	u32 cpu2pf_mbox_vec;
	/* pf <--> vf mbx */
	u32 pf_vf_shm_base;
	u32 pf2vf_mbox_ctrl_base;
	u32 pf_vf_mbox_mask_lo;
	u32 pf_vf_mbox_mask_hi;
	u32 pf2vf_mbox_vec_base;
	u32 vf2pf_mbox_vec_base;
	u32 cpu_vf_share_ram;
	int share_size;
	struct mbx_req_cookie_pool cookie_pool;
};

struct vf_vebvlans {
	struct list_head l;
	bool free;
	int veb_entry;
	u16 vid;
	u16 mask;
};

#define RNP_MBX_VF_CPU_SHM_PF_BASE (0xA8000)
#define RNP_NCSI_MC_COUNT (11)
#define RNP_NCSI_VLAN_COUNT (1)

#define RNP_VF_CPU_SHM_BASE_NR62 (RNP_MBX_VF_CPU_SHM_PF_BASE + 62 * 64)
struct ncsi_shm_info {
	u32 valid;
#define RNP_NCSI_SHM_VALID 0xa5000000
#define RNP_NCSI_SHM_VALID_MASK 0xff000000
#define RNP_MC_VALID BIT(0)
#define RNP_UC_VALID BIT(1)
#define RNP_VLAN_VALID BIT(2)

	struct {
		u32 uc_addr_lo;
		u32 uc_addr_hi;
	} uc;

	struct {
		u32 mc_addr_lo;
		u32 mc_addr_hi;
	} mc[RNP_NCSI_MC_COUNT];
	u32 ncsi_vlan;
};

struct rnp_hw {
	void *back;
	u8 __iomem *hw_addr;
	u8 __iomem *ring_msix_base;
	u8 __iomem *rpu_addr;
	u8 pfvfnum;
	struct pci_dev *pdev;
	u16 device_id;
	u16 vendor_id;
	u16 subsystem_device_id;
	u16 subsystem_vendor_id;
	char lane_mask;
	u16 mac_type;
	u16 phy_type;
	int nr_lane;
	u8 is_backplane : 1;
	u8 is_sgmii : 1;
	u8 force_10g_1g_speed_ablity : 1;
	u8 force_speed_stat : 2;
#define FORCE_SPEED_STAT_DISABLED 0
#define FORCE_SPEED_STAT_1G 1
#define FORCE_SPEED_STAT_10G 2
	u8 rpu_en : 1;
	u8 rpu_availble : 1;
	u8 ncsi_en;
	u8 ncsi_rar_entries;
	u16 ncsi_mc_count;
	u16 ncsi_vlan_count;
	u32 ncsi_vf_cpu_shm_pf_base;
	u32 saved_force_link_speed;
	u32 pcode;
	u32 supported_link;
	u32 advertised_link;
	u32 autoneg;
	u32 tp_mdx;
	u32 tp_mdix_ctrl;
	u32 phy_id;
	u8 fw_lldp_ablity;
	u8 link;
	u8 pci_gen;
	u8 pci_lanes;
	u16 max_msix_vectors;
	int speed;
	int duplex;
	u32 dma_version;
	u32 wol;
	u32 eco;
	u32 force_status;
	u32 force_link_supported;
	u16 min_length;
	u16 max_length;
	u16 min_length_current;
	u16 max_length_current;
	/* rss info */
#define HW_MAX_RETA_ENTRIES 512
	u8 rss_indir_tbl[HW_MAX_RETA_ENTRIES];
#define HW_MAX_TC_ENTRIES 8
	u8 rss_tc_tbl[HW_MAX_TC_ENTRIES];
	int rss_indir_tbl_num;
	int rss_tc_tbl_num;
	u32 rss_tbl_setup_flag;
#define HW_RSS_KEY_SIZE 40 /* size of RSS Hash Key in bytes */
	u8 rss_key[HW_RSS_KEY_SIZE];
	u32 rss_key_setup_flag;
	u32 vfnum;
	int dma_split_size;
	int num_rar_entries;
	int max_vfs;
	int max_vfs_noari;
	int sriov_ring_limit;
	int max_pf_macvlans;
	int num_vebvlan_entries;
	int fdir_mode;
	int layer2_count;
	int tuple5_count;
	int veb_ring;
	int default_vf_num;
	int vf_promisc_mode;
	int vf_promisc_num;
	u32 fdir_pballoc;
	enum rnp_rss_type rss_type;
	enum rnp_hw_type hw_type;
	struct rnp_hw_operations ops;
	struct rnp_nic_info nic;
	struct rnp_dma_info dma;
	struct rnp_eth_info eth;
	struct rnp_mac_info mac;
	struct rnp_addr_filter_info addr_ctrl;
	struct rnp_fc_info fc;
	struct rnp_phy_info phy;
	struct rnp_eeprom_info eeprom;
	struct rnp_bus_info bus;
	struct rnp_mbx_info mbx;
	struct rnp_pcs_info pcs;
	bool adapter_stopped;
	bool force_full_reset;
	bool mng_fw_enabled;
	bool wol_enabled;
	unsigned long wol_supported;
	int fw_version;
	u8 sfp_connector;
	struct vf_vebvlans vf_vas;
	struct vf_vebvlans *vv_list;
	u32 axi_mhz;
	u32 bd_uid;
	union {
		u8 port_id[4];
		u32 port_ids;
	};
	int mode;
	int default_rx_queue;
	u32 usecstocount;
#define RNP_NET_FEATURE_SG ((u32)(1 << 0))
#define RNP_NET_FEATURE_TX_CHECKSUM ((u32)(1 << 1))
#define RNP_NET_FEATURE_RX_CHECKSUM ((u32)(1 << 2))
#define RNP_NET_FEATURE_TSO ((u32)(1 << 3))
#define RNP_NET_FEATURE_TX_UDP_TUNNEL ((1 << 4))
#define RNP_NET_FEATURE_VLAN_FILTER ((1 << 5))
#define RNP_NET_FEATURE_VLAN_OFFLOAD ((1 << 6))
#define RNP_NET_FEATURE_RX_NTUPLE_FILTER ((1 << 7))
#define RNP_NET_FEATURE_TCAM ((1 << 8))
#define RNP_NET_FEATURE_RX_HASH ((1 << 9))
#define RNP_NET_FEATURE_RX_FCS ((1 << 10))
#define RNP_NET_FEATURE_HW_TC ((1 << 11))
#define RNP_NET_FEATURE_USO ((1 << 12))
#define RNP_NET_FEATURE_STAG_FILTER ((1 << 13))
#define RNP_NET_FEATURE_STAG_OFFLOAD ((1 << 14))
#define RNP_NET_FEATURE_VF_FIXED ((1 << 15))
#define RNP_VEB_VLAN_MASK_EN ((1 << 16))

	u32 feature_flags;
	struct rnp_thermal_sensor_data thermal_sensor_data;

	struct {
		int version;
		int len;
		int flag;
	} dump;
};

struct rnp_info {
	enum rnp_mac_type mac;
	enum rnp_rss_type rss_type;
	enum rnp_hw_type hw_type;
	s32 (*get_invariants)(struct rnp_hw *);
	struct rnp_mac_operations *mac_ops;
	struct rnp_eeprom_operations *eeprom_ops;
	struct rnp_mbx_operations *mbx_ops;
	struct rnp_pcs_operations *pcs_ops;
	bool one_pf_with_two_dma;
	int reg_off;
	int adapter_cnt;
	char lane_mask;
	int hi_dma;
	int total_queue_pair_cnts;
	int dma2_in_1pf;
	char *hw_addr;
};

/* Error Codes */
#define RNP_ERR_EEPROM -1
#define RNP_ERR_EEPROM_CHECKSUM -2
#define RNP_ERR_PHY -3
#define RNP_ERR_CONFIG -4
#define RNP_ERR_PARAM -5
#define RNP_ERR_MAC_TYPE -6
#define RNP_ERR_UNKNOWN_PHY -7
#define RNP_ERR_LINK_SETUP -8
#define RNP_ERR_ADAPTER_STOPPED -9
#define RNP_ERR_INVALID_MAC_ADDR -10
#define RNP_ERR_DEVICE_NOT_SUPPORTED -11
#define RNP_ERR_MASTER_REQUESTS_PENDING -12
#define RNP_ERR_INVALID_LINK_SETTINGS -13
#define RNP_ERR_AUTONEG_NOT_COMPLETE -14
#define RNP_ERR_RESET_FAILED -15
#define RNP_ERR_SWFW_SYNC -16
#define RNP_ERR_PHY_ADDR_INVALID -17
#define RNP_ERR_I2C -18
#define RNP_ERR_SFP_NOT_SUPPORTED -19
#define RNP_ERR_SFP_NOT_PRESENT -20
#define RNP_ERR_SFP_NO_INIT_SEQ_PRESENT -21
#define RNP_ERR_FDIR_REINIT_FAILED -23
#define RNP_ERR_EEPROM_VERSION -24
#define RNP_ERR_NO_SPACE -25
#define RNP_ERR_OVERTEMP -26
#define RNP_ERR_FC_NOT_NEGOTIATED -27
#define RNP_ERR_FC_NOT_SUPPORTED -28
#define RNP_ERR_SFP_SETUP_NOT_COMPLETE -30
#define RNP_ERR_PBA_SECTION -31
#define RNP_ERR_INVALID_ARGUMENT -32
#define RNP_ERR_HOST_INTERFACE_COMMAND -33
#define RNP_NOT_IMPLEMENTED 0x7FFFFFFF

#define RNP_RAH_AV 0x80000000
/* eth fix code */
#define RNP_FCTRL_BPE BIT(10)
#define RNP_FCTRL_UPE BIT(9)
#define RNP_FCTRL_MPE BIT(8)

#define RNP_MCSTCTRL_MTA BIT(2)
#define RNP_MCSTCTRL_UTA BIT(3)

#define RNP_MAX_LAYER2_FILTERS (16)
#define RNP_MAX_TUPLE5_FILTERS (128)
#define RNP_MAX_TCAM_FILTERS (4096)

#define RNP_SRC_IP_MASK BIT(0)
#define RNP_DST_IP_MASK BIT(1)
#define RNP_SRC_PORT_MASK BIT(2)
#define RNP_DST_PORT_MASK BIT(3)
#define RNP_L4_PROTO_MASK BIT(4)
#endif /* _RNP_TYPE_H_ */

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#ifndef _RNPGBE_TYPE_H_
#define _RNPGBE_TYPE_H_

#include <linux/types.h>
#include <linux/mdio.h>
#include <linux/netdevice.h>

//#define OPTM_WITH_LPAGE

#if defined(CONFIG_MGBE_OPTM_WITH_LPAGE) && !defined(OPTM_WITH_LPAGE)
#define OPTM_WITH_LPAGE
#endif

#if defined(CONFIG_MXGBE_MSIX_COUNT)
#define RNP_N10_MSIX_VECTORS CONFIG_MXGBE_MSIX_COUNT
#endif

#if IS_ENABLED(CONFIG_SYSFS)
#ifndef RNPGBE_SYSFS
#define RNPGBE_SYSFS
#endif /* RNPGBE_SYSFS */
#endif /* CONFIG_SYSFS */
#if IS_ENABLED(CONFIG_HWMON)
#ifndef RNPGBE_HWMON
#define RNPGBE_HWMON
#endif /* RNPGBE_HWMON */
#endif /* CONFIG_HWMON */
//#define DISABLE_PACKET_SPLIT
//
#ifdef CONFIG_DEBUG_FS
#define HAVE_RNP_DEBUG_FS
#endif /* CONFIG_DEBUG_FS */

#if (PAGE_SIZE < 8192)
//error
#ifdef OPTM_WITH_LPAGE
//#error can't open OPTM_WITH_LPAGE with PAGE_SIZE small than 8192
#undef OPTM_WITH_LPAGE
#endif
#endif

#include "rnpgbe_regs.h"

static inline struct device *pci_dev_to_dev(struct pci_dev *pdev)
{
        return &pdev->dev;
}

/* Device IDs */
#define PCI_VENDOR_ID_MUCSE 0x8848
#define PCI_DEVICE_ID_N10_PF0 0x1000
#define PCI_DEVICE_ID_N10_PF1 0x1001

#define RNP_DEV_ID_N10_PF0 0x7001
#define RNP_DEV_ID_N10_PF1 0x7002

#define PCI_DEVICE_ID_N10 0x1000
#define PCI_DEVICE_ID_N10C 0x1C00
#define PCI_DEVICE_ID_N400 0x1001
#define PCI_DEVICE_ID_N500_QUAD_PORT 0x8308
#define PCI_DEVICE_ID_N500_DUAL_PORT 0x8318
#define PCI_DEVICE_ID_N500_VF 0x8309
#define PCI_DEVICE_ID_N210 0x8208
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

#define ADVERTISE_10_HALF 0x0001
#define ADVERTISE_10_FULL 0x0002
#define ADVERTISE_100_HALF 0x0004
#define ADVERTISE_100_FULL 0x0008
#define ADVERTISE_1000_HALF 0x0010 /* Not used, just FYI */
#define ADVERTISE_1000_FULL 0x0020
#define ADVERTISE_2500_HALF 0x0040 /* NOT used, just FYI */
#define ADVERTISE_2500_FULL 0x0080

#define RNP_MAX_SENSORS 1
struct rnpgbe_thermal_diode_data {
	u8 location;
	u8 temp;
	u8 caution_thresh;
	u8 max_op_thresh;
};

struct rnpgbe_thermal_sensor_data {
	struct rnpgbe_thermal_diode_data sensor[RNP_MAX_SENSORS];
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
#define TSRN10_TX_DEFAULT_BURST 8

#ifndef TSRN10_RX_DEFAULT_BURST
#define TSRN10_RX_DEFAULT_BURST 16
#endif

#ifndef TSRN10_RX_DEFAULT_LINE
#define TSRN10_RX_DEFAULT_LINE 32
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
#define RNP_PKT_TIMEOUT_TX 200
#endif
/* VF Device IDs */
#define RNP_DEV_ID_N10_PF0_VF 0x8001
#define RNP_DEV_ID_N10_PF1_VF 0x8002

#define RNP_DEV_ID_N10_PF0_VF_N 0x1010
#define RNP_DEV_ID_N10_PF1_VF_N 0x1011

/* Transmit Descriptor - Advanced */
struct rnpgbe_tx_desc {
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

#define RNP_TXD_L4_CSUM (0x10000000) //udp tcp sctp csum
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

struct rnpgbe_tx_ctx_desc {
	__le32 mss_len_vf_num;
	__le32 inner_vlan_tunnel_len;
#define VF_VEB_MARK (1 << 24) //bit 56
#define VF_VEB_IGNORE_VLAN (1 << 25) //bit 57
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
union rnpgbe_rx_desc {
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
#define RNP_RX_L3_TYPE_MASK (1 << 15) // 1 is ipv4
#define VEB_VF_PKG (1 << 1) // bit 49
#define VEB_VF_IGNORE_VLAN (1 << 0) //bit 48
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
struct rnpgbe_hic_hdr {
	u8 cmd;
	u8 buf_len;
	union {
		u8 cmd_resv;
		u8 ret_status;
	} cmd_or_resp;
	u8 checksum;
};

struct rnpgbe_hic_drv_info {
	struct rnpgbe_hic_hdr hdr;
	u8 port_num;
	u8 ver_sub;
	u8 ver_build;
	u8 ver_min;
	u8 ver_maj;
	u8 pad; /* end spacing to ensure length is mult. of dword */
	u16 pad2; /* end spacing to ensure length is mult. of dword2 */
};

/* Context descriptors */
struct rnpgbe_adv_tx_context_desc {
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
typedef u32 rnpgbe_autoneg_advertised;
/* Link speed */
typedef u32 rnpgbe_link_speed;
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
#define RNP_SFP_MODE_FIBER_CHANNEL_SPEED BIT(25) // sfp-a0-10 != 0
#define RNP_SFP_CONNECTOR_DAC BIT(26)
#define RNP_SFP_TO_SGMII BIT(27)
#define RNP_SFP_25G_SR BIT(28)
#define RNP_SFP_25G_KR BIT(29)
#define RNP_SFP_25G_CR BIT(30)

/* Flow Control Data Sheet defined values
 * Calculation and defines taken from 802.1bb Annex O
 */

enum rnpgbe_atr_flow_type {
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
union rnpgbe_atr_input {
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
union rnpgbe_atr_hash_dword {
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

enum rnpgbe_eeprom_type {
	rnpgbe_eeprom_uninitialized = 0,
	rnpgbe_eeprom_spi,
	rnpgbe_flash,
	rnpgbe_eeprom_none /* No NVM support */
};

enum mac_type {
	mac_dwc_xlg,
	mac_dwc_g,

};

enum rnpgbe_mac_type {
	rnpgbe_mac_unknown = 0,
	rnpgbe_mac_n10g_x8_40G,
	rnpgbe_mac_n10g_x2_10G,
	rnpgbe_mac_n10g_x4_10G,
	rnpgbe_mac_n10g_x8_10G,
	rnpgbe_mac_n10l_x8_1G,
	rnpgbe_num_macs
};

enum rnpgbe_rss_type {
	rnpgbe_rss_uv440 = 0,
	rnpgbe_rss_uv3p,
	rnpgbe_rss_n10,
	rnpgbe_rss_n20,
	rnpgbe_rss_n500
};

enum rnpgbe_hw_type {
	rnpgbe_hw_uv440 = 0,
	rnpgbe_hw_uv3p,
	rnpgbe_hw_n10,
	rnpgbe_hw_n20,
	rnpgbe_hw_n400,
	rnpgbe_hw_n500,
	rnpgbe_hw_n210,
};

enum rnpgbe_eth_type { rnpgbe_eth_n10 = 0, rnpgbe_eth_n500 };

enum rnpgbe_phy_type {
	rnpgbe_phy_unknown = 0,
	rnpgbe_phy_none,
	rnpgbe_phy_sfp,
	rnpgbe_phy_sfp_unsupported,
	rnpgbe_phy_generic,
	rnpgbe_phy_sfp_unknown,
	rnpgbe_phy_sgmii,
};

enum rnpgbe_sfp_type {
	rnpgbe_sfp_type_da_cu = 0,
	rnpgbe_sfp_type_sr = 1,
	rnpgbe_sfp_type_lr = 2,
	rnpgbe_sfp_type_da_cu_core0 = 3,
	rnpgbe_sfp_type_da_cu_core1 = 4,
	rnpgbe_sfp_type_srlr_core0 = 5,
	rnpgbe_sfp_type_srlr_core1 = 6,
	rnpgbe_sfp_type_da_act_lmt_core0 = 7,
	rnpgbe_sfp_type_da_act_lmt_core1 = 8,
	rnpgbe_sfp_type_1g_cu_core0 = 9,
	rnpgbe_sfp_type_1g_cu_core1 = 10,
	rnpgbe_sfp_type_1g_sx_core0 = 11,
	rnpgbe_sfp_type_1g_sx_core1 = 12,
	rnpgbe_sfp_type_1g_lx_core0 = 13,
	rnpgbe_sfp_type_1g_lx_core1 = 14,
	rnpgbe_sfp_type_not_present = 0xFFFE,
	rnpgbe_sfp_type_unknown = 0xFFFF
};

enum rnpgbe_media_type {
	rnpgbe_media_type_unknown = 0,
	rnpgbe_media_type_fiber,
	rnpgbe_media_type_copper,
	rnpgbe_media_type_backplane,
	rnpgbe_media_type_cx4,
	rnpgbe_media_type_da,
	rnpgbe_media_type_virtual

};

/* Flow Control Settings */
enum rnpgbe_fc_mode {
	rnpgbe_fc_none = 0,
	rnpgbe_fc_rx_pause,
	rnpgbe_fc_tx_pause,
	rnpgbe_fc_full,
	rnpgbe_fc_default
};

#define PAUSE_TX (0x1)
#define PAUSE_RX (0x2)
#define PAUSE_AUTO (0x10)

#define ASYM_PAUSE BIT(11)
#define SYM_PAUSE BIT(10)

struct rnpgbe_addr_filter_info {
	u32 num_mc_addrs;
	u32 rar_used_count;
	u32 mta_in_use;
	u32 overflow_promisc;
	bool uc_set_promisc;
	bool user_set_promisc;
};

/* Bus parameters */
struct rnpgbe_bus_info {
	u16 func;
	u16 lan_id;
};

/* Flow control parameters */
struct rnpgbe_fc_info {
	u32 high_water[RNP_MAX_TRAFFIC_CLASS]; /* Flow Control High-water */
	u32 low_water[RNP_MAX_TRAFFIC_CLASS]; /* Flow Control Low-water */
	u16 pause_time; /* Flow Control Pause timer */
	bool send_xon; /* Flow control send XON */
	bool strict_ieee; /* Strict IEEE mode */
	bool disable_fc_autoneg; /* Do not autonegotiate FC */
	bool fc_was_autonegged; /* Is current_mode the result of autonegging? */
	enum rnpgbe_fc_mode current_mode; /* FC mode in effect */
	u32 requested_mode; /* FC mode requested by caller */
};

/* Statistics counters collected by the MAC */
struct rnpgbe_hw_stats {
	u64 dma_to_dma;
	u64 dma_to_switch;
	u64 mac_to_mac;
	u64 switch_to_switch;
	u64 mac_to_dma;
	u64 switch_to_dma;
	u64 vlan_add_cnt;
	u64 vlan_strip_cnt;
	//=== error
	u64 invalid_droped_packets;
	u64 filter_dropped_packets;
	//== drop ==
	u64 rx_capabity_lost;
	u64 host_l2_match_drop;
	u64 redir_input_match_drop;
	u64 redir_etype_match_drop;
	u64 redir_tcp_syn_match_drop;
	u64 redir_tuple5_match_drop;
	u64 redir_tcam_match_drop;

	u64 bmc_dropped_packets;
	u64 switch_dropped_packets;
	//=== rx
	u64 dma_to_host;
	//=== dma-tx ==
	u64 port0_tx_packets;
	u64 port1_tx_packets;
	u64 port2_tx_packets;
	u64 port3_tx_packets;
	//=== emac 1to4 tx ==
	u64 in0_tx_pkts;
	u64 in1_tx_pkts;
	u64 in2_tx_pkts;
	u64 in3_tx_pkts;
	//=== phy tx ==
	u64 port0_to_phy_pkts;
	u64 port1_to_phy_pkts;
	u64 port2_to_phy_pkts;
	u64 port3_to_phy_pkts;
	//=== mac rx ===
	u64 mac_rx_broadcast;
	u64 mac_rx_multicast;
	u64 tx_broadcast;
	u64 tx_multicast;
	// n500 use this
	u64 ultra_short_cnt;
	u64 jumbo_cnt;

	u64 dma_rx_drop_cnt_0;
	u64 dma_rx_drop_cnt_1;
	u64 dma_rx_drop_cnt_2;
	u64 dma_rx_drop_cnt_3;
	u64 dma_rx_drop_cnt_4;
	u64 dma_rx_drop_cnt_5;
	u64 dma_rx_drop_cnt_6;
	u64 dma_rx_drop_cnt_7;
	u64 tx_pause;
	u64 rx_pause;
};

/* forward declaration */
struct rnpgbe_hw;
struct rnpgbe_eth_info;
struct rnpgbe_dma_info;
struct rnpgbe_mac_info;

/* iterator type for walking multicast address lists */
typedef u8 *(*rnpgbe_mc_addr_itr)(struct rnpgbe_hw *hw, u8 **mc_addr_ptr,
				  u32 *vmdq);

/* Function pointer table */
struct rnpgbe_eeprom_operations {
	s32 (*init_params)(struct rnpgbe_hw *hw);
	//	s32 (*read)(struct rnpgbe_hw *hw, u16, u16 *);
	//	s32 (*read_buffer)(struct rnpgbe_hw *hw, u16, u16, u16 *);
	//	s32 (*write)(struct rnpgbe_hw *hw, u16, u16);
	//	s32 (*write_buffer)(struct rnpgbe_hw *hw, u16, u16, u16 *);
	//	s32 (*validate_checksum)(struct rnpgbe_hw *hw, u16 *);
	s32 (*update_checksum)(struct rnpgbe_hw *hw);
	u16 (*calc_checksum)(struct rnpgbe_hw *hw);
};

/* add nic operations */
struct rnpgbe_eth_operations {
	/* RAR, Multicast, VLAN */
	s32 (*get_mac_addr)(struct rnpgbe_eth_info *eth, u8 *addr);
	s32 (*set_rar)(struct rnpgbe_eth_info *eth, u32 index, u8 *addr,
		       bool enable_addr);
	s32 (*clear_rar)(struct rnpgbe_eth_info *eth, u32 index);
	s32 (*set_vmdq)(struct rnpgbe_eth_info *eth, u32 rar, u32 vmdq);
	s32 (*clear_vmdq)(struct rnpgbe_eth_info *eth, u32 rar, u32 vmdq);

	s32 (*update_mc_addr_list)(struct rnpgbe_eth_info *eth,
				   struct net_device *netdev, bool sriov_on);
	void (*clr_mc_addr)(struct rnpgbe_eth_info *eth);

	int (*set_rss_hfunc)(struct rnpgbe_eth_info *eth, int hfunc);
	void (*set_rss_key)(struct rnpgbe_eth_info *eth, bool sriov_flag);
	void (*set_rss_table)(struct rnpgbe_eth_info *eth);
	void (*set_rx_hash)(struct rnpgbe_eth_info *eth, bool status,
			    bool sriov_flag);

	// ntuple function
	void (*set_layer2_remapping)(struct rnpgbe_eth_info *eth,
				     union rnpgbe_atr_input *input, u16 pri_id,
				     u8 queue, bool prio_flag);
	void (*clr_layer2_remapping)(struct rnpgbe_eth_info *eth, u16 pri_id);
	void (*clr_all_layer2_remapping)(struct rnpgbe_eth_info *eth);
	void (*set_tuple5_remapping)(struct rnpgbe_eth_info *eth,
				     union rnpgbe_atr_input *input, u16 pri_id,
				     u8 queue, bool prio_flag);
	void (*clr_tuple5_remapping)(struct rnpgbe_eth_info *eth, u16 pri_id);
	void (*clr_all_tuple5_remapping)(struct rnpgbe_eth_info *eth);
	void (*set_tcp_sync_remapping)(struct rnpgbe_eth_info *eth, int queue,
				       bool flag, bool prio);
	void (*set_rx_skip)(struct rnpgbe_eth_info *eth, int count, bool flag);

	void (*set_min_max_packet)(struct rnpgbe_eth_info *eth, int min,
				   int max);
	void (*set_vlan_strip)(struct rnpgbe_eth_info *eth, u16 queue,
			       bool enable);
	s32 (*set_vfta)(struct rnpgbe_eth_info *eth, u32 vlan, bool vlan_on);
	void (*clr_vfta)(struct rnpgbe_eth_info *eth);
	void (*set_vlan_filter)(struct rnpgbe_eth_info *eth, bool status);
	void (*set_outer_vlan_type)(struct rnpgbe_eth_info *eth, int type);
	void (*set_double_vlan)(struct rnpgbe_eth_info *eth, bool on);
	void (*set_vxlan_port)(struct rnpgbe_eth_info *eth, u32 port);
	void (*set_vxlan_mode)(struct rnpgbe_eth_info *eth, bool inner);
	s32 (*set_fc_mode)(struct rnpgbe_eth_info *eth);

	void (*set_rx)(struct rnpgbe_eth_info *eth, bool status);
	void (*set_fcs)(struct rnpgbe_eth_info *eth, bool status);

	void (*set_vf_vlan_mode)(struct rnpgbe_eth_info *eth, u16 vlan, int vf,
				 bool enable);
};

enum {
	rnpgbe_driver_insmod,
	rnpgbe_driver_suspuse,
	rnpgbe_driver_force_control_phy,
};

struct rnpgbe_hw_operations {
	s32 (*init_hw)(struct rnpgbe_hw *hw);
	s32 (*reset_hw)(struct rnpgbe_hw *hw);
	s32 (*start_hw)(struct rnpgbe_hw *hw);

	void (*set_mtu)(struct rnpgbe_hw *hw, int mtu);
	void (*set_vlan_filter_en)(struct rnpgbe_hw *hw, bool enable);
	void (*set_vlan_filter)(struct rnpgbe_hw *hw, u16 vid, bool enable,
				bool sriov_flag);
	int (*set_veb_vlan_mask)(struct rnpgbe_hw *hw, u16 vid, int vf,
				 bool enable);
	void (*set_vf_vlan_filter)(struct rnpgbe_hw *hw, u16 vid, int vf,
				   bool enable, bool veb_only);
	void (*clr_vfta)(struct rnpgbe_hw *hw);
	void (*set_vlan_strip)(struct rnpgbe_hw *hw, u16 queue, bool strip);
	void (*set_mac)(struct rnpgbe_hw *hw, u8 *mac, bool sriov_flag);
	void (*set_rx_mode)(struct rnpgbe_hw *hw, struct net_device *netdev,
			    bool sriov_flag);
	void (*set_rar_with_vf)(struct rnpgbe_hw *hw, u8 *mac, int idx,
				u32 vfnum, bool enable);
	void (*clr_rar)(struct rnpgbe_hw *hw, int idx);
	void (*clr_rar_all)(struct rnpgbe_hw *hw);
	void (*clr_vlan_veb)(struct rnpgbe_hw *hw);
	void (*set_txvlan_mode)(struct rnpgbe_hw *hw, bool vlan);
	void (*set_tx_maxrate)(struct rnpgbe_hw *hw, bool flag);
	void (*set_fcs_mode)(struct rnpgbe_hw *hw, bool status);
	void (*set_vxlan_port)(struct rnpgbe_hw *hw, u32 port);
	void (*set_vxlan_mode)(struct rnpgbe_hw *hw, bool inner);
	void (*set_mac_speed)(struct rnpgbe_hw *hw, bool link, u32 speed,
			      bool duplex);
	void (*set_mac_rx)(struct rnpgbe_hw *hw, bool status);
	void (*update_sriov_info)(struct rnpgbe_hw *hw);

	void (*set_sriov_status)(struct rnpgbe_hw *hw, bool status);
	//void (*set_sriov_vf_mac)(struct rnpgbe_hw *, u8 *, int, bool);
	void (*set_sriov_vf_mc)(struct rnpgbe_hw *hw, u16 mc_addr);

	void (*set_pause_mode)(struct rnpgbe_hw *hw);
	void (*get_pause_mode)(struct rnpgbe_hw *hw);
	void (*update_hw_info)(struct rnpgbe_hw *hw);
	void (*set_rx_hash)(struct rnpgbe_hw *hw, bool status, bool sriov_flag);
	int (*set_rss_hfunc)(struct rnpgbe_hw *hw, u8 hfunc);
	void (*set_rss_key)(struct rnpgbe_hw *hw, bool sriov_flag);
	void (*set_rss_table)(struct rnpgbe_hw *hw);

	//MBX_ID
	void (*set_mbx_link_event)(struct rnpgbe_hw *hw, int enable);
	void (*set_mbx_ifup)(struct rnpgbe_hw *hw, int enable);

	s32 (*get_thermal_sensor_data)(struct rnpgbe_hw *hw);
	s32 (*init_thermal_sensor_thresh)(struct rnpgbe_hw *hw);

	void (*disable_tx_laser)(struct rnpgbe_hw *hw);
	void (*enable_tx_laser)(struct rnpgbe_hw *hw);
	void (*flap_tx_laser)(struct rnpgbe_hw *hw);
	s32 (*check_link)(struct rnpgbe_hw *hw, rnpgbe_link_speed *speed,
			  bool *link_up, bool *duplex,
			  bool link_up_wait_to_complete);
	s32 (*setup_link)(struct rnpgbe_hw *hw, rnpgbe_link_speed adv,
			  u32 autoneg, u32 speed, u32 duplex);
	void (*clean_link)(struct rnpgbe_hw *hw);
	s32 (*get_link_capabilities)(struct rnpgbe_hw *hw,
				     rnpgbe_link_speed *speed, bool *autoneg);
	s32 (*init_rx_addrs)(struct rnpgbe_hw *hw);

	// ntuple function
	void (*set_layer2_remapping)(struct rnpgbe_hw *hw,
				     union rnpgbe_atr_input *input, u16 pri_id,
				     u8 queue, bool prio_flag);
	void (*clr_layer2_remapping)(struct rnpgbe_hw *hw, u16 pri_id);
	void (*clr_all_layer2_remapping)(struct rnpgbe_hw *hw);
	void (*set_tuple5_remapping)(struct rnpgbe_hw *hw,
				     union rnpgbe_atr_input *input, u16 pri_id,
				     u8 queue, bool prio_flag);
	void (*clr_tuple5_remapping)(struct rnpgbe_hw *hw, u16 pri_id);
	void (*clr_all_tuple5_remapping)(struct rnpgbe_hw *hw);
	void (*set_tcp_sync_remapping)(struct rnpgbe_hw *hw, int queue,
				       bool flag, bool prio);
	void (*set_rx_skip)(struct rnpgbe_hw *hw, int count, bool flag);
	void (*set_outer_vlan_type)(struct rnpgbe_hw *hw, int type);

	void (*update_hw_status)(struct rnpgbe_hw *hw,
				 struct rnpgbe_hw_stats *hw_stats,
				 struct net_device_stats *net_stats);
	void (*update_msix_count)(struct rnpgbe_hw *hw, int msix_count);

	void (*update_rx_drop)(struct rnpgbe_hw *hw);

	// ethtool
	void (*setup_ethtool)(struct net_device *netdev);

	s32 (*phy_read_reg)(struct rnpgbe_hw *hw, u32 reg_addr, u32 device_type,
			    u16 *phy_data);
	s32 (*phy_write_reg)(struct rnpgbe_hw *hw, u32 reg_addr,
			     u32 device_type, u16 phy_data);
	void (*setup_wol)(struct rnpgbe_hw *hw, u32 mode);
	void (*set_vf_vlan_mode)(struct rnpgbe_hw *hw, u16 vlan, int vf,
				 bool enable);
	void (*driver_status)(struct rnpgbe_hw *hw, bool enable, int mode);

	void (*setup_eee)(struct rnpgbe_hw *hw, int ls, int tw, u32 local_eee);

	void (*set_eee_mode)(struct rnpgbe_hw *hw, bool en_tx_lpi_clockgating);
	void (*reset_eee_mode)(struct rnpgbe_hw *hw);
	void (*set_eee_timer)(struct rnpgbe_hw *hw, int ls, int tw);
	void (*set_eee_pls)(struct rnpgbe_hw *hw, int link);

	u32 (*get_lpi_status)(struct rnpgbe_hw *hw);

	int (*get_ncsi_mac)(struct rnpgbe_hw *hw, u8 *addr, int idx);
	int (*get_ncsi_vlan)(struct rnpgbe_hw *hw, u16 *vlan, int idx);

	void (*set_lldp)(struct rnpgbe_hw *hw, bool enable);
	void (*get_lldp)(struct rnpgbe_hw *hw);
};

struct rnpgbe_mac_operations {
	void (*set_mac_rx)(struct rnpgbe_mac_info *mac, bool status);
	void (*set_mac_speed)(struct rnpgbe_mac_info *mac, bool link, u32 speed,
			      bool duplex);
	void (*set_mac_fcs)(struct rnpgbe_mac_info *mac, bool status);
	s32 (*set_fc_mode)(struct rnpgbe_mac_info *mac);
	void (*check_link)(struct rnpgbe_mac_info *mac,
			   rnpgbe_link_speed *speed, bool *link_up,
			   bool link_up_wait_to_complete);
	void (*set_mac)(struct rnpgbe_mac_info *mac, u8 *addr, int index);
	int (*mdio_write)(struct rnpgbe_mac_info *mac, int phyreg, int phydata);
	int (*mdio_read)(struct rnpgbe_mac_info *mac, u32 phyreg,
			 u32 *regvalue);
	void (*pmt)(struct rnpgbe_mac_info *mac, u32 mode, bool ncsi_en);
	void (*set_eee_mode)(struct rnpgbe_mac_info *mac,
			     bool en_tx_lpi_clockgating);
	void (*reset_eee_mode)(struct rnpgbe_mac_info *mac);
	void (*set_eee_timer)(struct rnpgbe_mac_info *mac, int ls, int tw);
	void (*set_eee_pls)(struct rnpgbe_mac_info *mac, int link);
	u32 (*get_lpi_status)(struct rnpgbe_mac_info *mac);
};

struct rnpgbe_phy_operations {
	s32 (*identify)(struct rnpgbe_hw *hw);
	s32 (*identify_sfp)(struct rnpgbe_hw *hw);
	s32 (*init)(struct rnpgbe_hw *hw);
	s32 (*reset)(struct rnpgbe_hw *hw);
	s32 (*read_reg)(struct rnpgbe_hw *hw, u32 reg_addr, u32 device_type,
			u16 *phy_data);
	s32 (*write_reg)(struct rnpgbe_hw *hw, u32 reg_addr, u32 device_type,
			 u16 phy_data);
	s32 (*setup_link)(struct rnpgbe_hw *hw);
	s32 (*setup_link_speed)(struct rnpgbe_hw *hw, rnpgbe_link_speed speed,
				bool wait_to_complete);
	s32 (*check_link)(struct rnpgbe_hw *hw, rnpgbe_link_speed *speed,
			  bool *flag);
	s32 (*get_firmware_version)(struct rnpgbe_hw *hw, u16 *version);
	s32 (*read_i2c_byte)(struct rnpgbe_hw *hw, u8 byte_offset, u8 dev_addr,
			     u8 *data);
	s32 (*write_i2c_byte)(struct rnpgbe_hw *hw, u8 byte_offset, u8 dev_addr,
			      u8 data);
	s32 (*read_i2c_sff8472)(struct rnpgbe_hw *hw, u8 byte_offset, u8 *data);
	s32 (*read_i2c_eeprom)(struct rnpgbe_hw *hw, u8 byte_offset, u8 *data);
	s32 (*write_i2c_eeprom)(struct rnpgbe_hw *hw, u8 byte_offset, u8 data);
	s32 (*check_overtemp)(struct rnpgbe_hw *hw);
};

struct rnpgbe_eeprom_info {
	struct rnpgbe_eeprom_operations ops;
	enum rnpgbe_eeprom_type type;
	u32 semaphore_delay;
	u16 word_size;
	u16 address_bits;
	u16 word_page_size;
};

struct rnpgbe_dma_operations {
	void (*set_tx_maxrate)(struct rnpgbe_dma_info *dma, u16 queue,
			       u32 max_rate);
	void (*set_veb_mac)(struct rnpgbe_dma_info *dma, u8 *mac, u32 vfnum,
			    u32 ring);
	/* only set own vlan */
	void (*set_veb_vlan)(struct rnpgbe_dma_info *dma, u16 vlan, u32 vfnum);
	void (*set_veb_vlan_mask)(struct rnpgbe_dma_info *dma, u16 vlan,
				  u16 mask, int entry);
	void (*clr_veb_all)(struct rnpgbe_dma_info *dma);
};

struct rnpgbe_dma_info {
	struct rnpgbe_dma_operations ops;
	u8 __iomem *dma_base_addr;
	u8 __iomem *dma_ring_addr;
	void *back;
	u32 max_tx_queues;
	u32 max_rx_queues;
	u32 dma_version;
};

#define RNP_MAX_MTA 128
struct rnpgbe_eth_info {
	struct rnpgbe_eth_operations ops;
	u8 __iomem *eth_base_addr;
	enum rnpgbe_eth_type eth_type;
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

struct rnpgbe_nic_info {
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
struct rnpgbe_mac_info {
	struct rnpgbe_mac_operations ops;
	u8 __iomem *mac_addr;
	void *back;
	struct mii_regs mii;
	int phy_addr;
	int clk_csr;
	enum rnpgbe_mac_type type;
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

struct rnpgbe_phy_info {
	struct rnpgbe_phy_operations ops;
	struct mdio_if_info mdio;
	enum rnpgbe_phy_type type;
	u32 id;
	u32 phy_addr;
	bool is_mdix;
	u8 mdix;
	enum rnpgbe_sfp_type sfp_type;
	bool sfp_setup_needed;
	u32 revision;
	enum rnpgbe_media_type media_type;
	bool reset_disable;
	rnpgbe_autoneg_advertised autoneg_advertised;
	bool smart_speed_active;
	bool multispeed_fiber;
	bool reset_if_overtemp;
};

#include "rnpgbe_mbx.h"

struct rnpgbe_pcs_operations {
	u32 (*read)(struct rnpgbe_hw *hw, int num, u32 addr);
	void (*write)(struct rnpgbe_hw *hw, int num, u32 addr, u32 value);
};

struct rnpgbe_mbx_operations {
	s32 (*init_params)(struct rnpgbe_hw *hw);
	s32 (*read)(struct rnpgbe_hw *hw, u32 *msg, u16 size, enum MBX_ID);
	s32 (*write)(struct rnpgbe_hw *hw, u32 *msg, u16 size, enum MBX_ID);
	s32 (*read_posted)(struct rnpgbe_hw *hw, u32 *msg, u16 size,
			   enum MBX_ID);
	s32 (*write_posted)(struct rnpgbe_hw *hw, u32 *msg, u16 size,
			    enum MBX_ID);
	s32 (*check_for_msg)(struct rnpgbe_hw *hw, enum MBX_ID);
	s32 (*check_for_ack)(struct rnpgbe_hw *hw, enum MBX_ID);
	//s32 (*check_for_rst)(struct rnpgbe_hw *, enum MBX_ID);
	s32 (*configure)(struct rnpgbe_hw *hw, int nr_vec, bool enable);
};

struct rnpgbe_mbx_stats {
	u32 msgs_tx;
	u32 msgs_rx;

	u32 acks;
	u32 reqs;
	u32 rsts;
};

struct rnpgbe_pcs_info {
	struct rnpgbe_pcs_operations ops;
	int pcs_count;
};

struct rnpgbe_mbx_info {
	struct rnpgbe_mbx_operations ops;
	struct rnpgbe_mbx_stats stats;
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
	// add reg define
	int mbx_size;

	int mbx_mem_size;
#define MBX_FEATURE_NO_ZERO BIT(0)
#define MBX_FEATURE_WRITE_DELAY BIT(1)
	u32 mbx_feature;
	// cm3 <-> pf mbx
	u32 cpu_pf_shm_base;
	u32 pf2cpu_mbox_ctrl;
	u32 pf2cpu_mbox_mask;
	u32 cpu_pf_mbox_mask;
	u32 cpu2pf_mbox_vec;

	// pf <--> vf mbx
	u32 pf_vf_shm_base;
	u32 pf2vf_mbox_ctrl_base;
	u32 pf_vf_mbox_mask_lo;
	u32 pf_vf_mbox_mask_hi;
	u32 pf2vf_mbox_vec_base;
	u32 vf2pf_mbox_vec_base;

	u32 cpu_vf_share_ram;
	int share_size;
};

struct vf_vebvlans {
	struct list_head l;
	bool free;
	int veb_entry;
	u16 vid;
	u16 mask;
};
struct lldp_status {
	int enable;
	int inteval;
};

struct rnpgbe_hw {
	void *back;
	u8 __iomem *hw_addr;
	u8 __iomem *ring_msix_base;
	u8 __iomem *rpu_addr; // 0x4000_0000
	u8 pfvfnum; // fun
	u8 pfvfnum_system;
	struct pci_dev *pdev;

	u16 device_id;
	u16 vendor_id;
	u16 subsystem_device_id;
	u16 subsystem_vendor_id;
	char lane_mask;
	u16 mac_type;
	u16 phy_type;
	int nr_lane;
	int sfc_boot;
	int pxe_en;
	int ncsi_en;
	int trim_valid;

	u8 is_backplane : 1;
	u8 is_sgmii : 1;
	u8 force_10g_1g_speed_ablity : 1;
	u8 force_speed_stat : 2;
#define FORCE_SPEED_STAT_DISABLED 0
#define FORCE_SPEED_STAT_1G 1
#define FORCE_SPEED_STAT_10G 2

	u32 supported_link;
	u32 advertised_link;
	u32 autoneg;
	u32 fake_autoneg;
	u32 tp_mdx;
	u32 tp_mdix_ctrl;
	u32 phy_id;

	u32 eee_capability;

	u8 link;
	u8 pci_gen;
	u8 pci_lanes;
	u16 max_msix_vectors;

	int speed;
	int duplex;
	u32 dma_version;
	u32 wol;
	u32 wol_en;
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

	struct lldp_status lldp_status;

	u32 fdir_pballoc; //total count
	enum rnpgbe_rss_type rss_type;
	enum rnpgbe_hw_type hw_type;
	struct rnpgbe_hw_operations ops;
	struct rnpgbe_nic_info nic;
	struct rnpgbe_dma_info dma;
	struct rnpgbe_eth_info eth;
	struct rnpgbe_mac_info mac;
	struct rnpgbe_addr_filter_info addr_ctrl;
	struct rnpgbe_fc_info fc;
	struct rnpgbe_phy_info phy;
	struct rnpgbe_eeprom_info eeprom;
	struct rnpgbe_bus_info bus;
	struct rnpgbe_mbx_info mbx;
	struct rnpgbe_pcs_info pcs;
	bool adapter_stopped;
	bool force_full_reset;
	bool mng_fw_enabled;
	bool wol_enabled;
	unsigned long wol_supported;
	int fw_version;
	int force_en;
	int force_cap;
	u32 driver_version;
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
#define RNP_NET_FEATURE_TX_UDP_TUNNEL (1 << 4)
#define RNP_NET_FEATURE_VLAN_FILTER (1 << 5)
#define RNP_NET_FEATURE_VLAN_OFFLOAD (1 << 6)
#define RNP_NET_FEATURE_RX_NTUPLE_FILTER (1 << 7)
#define RNP_NET_FEATURE_TCAM (1 << 8)
#define RNP_NET_FEATURE_RX_HASH (1 << 9)
#define RNP_NET_FEATURE_RX_FCS (1 << 10)
#define RNP_NET_FEATURE_HW_TC (1 << 11)
#define RNP_NET_FEATURE_USO (1 << 12)
#define RNP_NET_FEATURE_STAG_FILTER (1 << 13)
#define RNP_NET_FEATURE_STAG_OFFLOAD (1 << 14)
#define RNP_NET_FEATURE_VF_FIXED (1 << 15)
#define RNP_VEB_VLAN_MASK_EN (1 << 16)
#define RNP_HW_FEATURE_EEE (1 << 17)
#define RNP_HW_SOFT_MASK_OTHER_IRQ (1 << 18)

	u32 feature_flags;
	struct rnpgbe_thermal_sensor_data thermal_sensor_data;

	struct {
		int version;
		int len;
		int flag;
	} dump;
};

struct rnpgbe_info {
	enum rnpgbe_mac_type mac;
	enum rnpgbe_rss_type rss_type;
	enum rnpgbe_hw_type hw_type;
	s32 (*get_invariants)(struct rnpgbe_hw *hw);
	struct rnpgbe_mac_operations *mac_ops;
	struct rnpgbe_eeprom_operations *eeprom_ops;
	struct rnpgbe_phy_operations *phy_ops;
	struct rnpgbe_mbx_operations *mbx_ops;
	struct rnpgbe_pcs_operations *pcs_ops;

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
#endif /* _RNPGBE_TYPE_H_ */

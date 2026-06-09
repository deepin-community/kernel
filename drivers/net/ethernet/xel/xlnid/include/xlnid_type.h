/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright(c) 2008 - 2024 Xel Technology. */

#ifndef _XLNID_TYPE_H_
#define _XLNID_TYPE_H_

#include "xlnid_osdep.h"
#include "xlnid_register.h"

/* XLNID_MASK is a macro used on 32 bit registers */
#define XLNID_MASK(mask, shift) (mask << shift)

/* Override this by setting IOMEM in your xlnid_osdep.h header */
#ifndef IOMEM
#define IOMEM
#endif

//#define XLNID_GEPHY_SUPPORT      /* phy */
//#define XLNID_XGE_SUPPORT
//#define XLNID_1000BASEX_SUPPORT
//#define XLNID_GEPHY_SUPPORT
//#define XLNID_RMII_MAC_SUPPORT

/* Lodestar use 32byte descriptors */
//#define XLNID_LODESTAR

#define IP_PROTOCOL_TCP 0x6
#define IP_PROTOCOL_UDP 0x11

#define XLNID_RXDCTL_VME_VME    0x40000000 /* VLAN mode enable */


#define XLNID_TXBUFFER_VALUE    0x3F

#define XLNID_UDP_HDR_LEN       8
#define XLNID_IPV6_HDR_LEN      40

#define XLNID_MAXFRS_SKYLAKE    0x3FFB0000
#define XLNID_MAXFRS_DEFAULT    0x3FFF0000
#define XLNID_MHADD_MFS_MASK    0xFFFF0000
#define XLNID_MHADD_MFS_SHIFT   16

/* Vendor ID */
#define XLNID_VENDOR_ID_XEL             0xffe1

/* Device IDs */
#define XLNID_DEV_ID_SKYLAKE            0xdd10
#define XLNID_DEV_ID_WESTLAKE           0xd200
#define XLNID_DEV_ID_SGMII_MAC          0xd201
#define XLNID_DEV_ID_1000BASEX          0xd202
#define XLNID_DEV_ID_SGMII_PHY          0xd203
#define XLNID_DEV_ID_LK40               0xd300
#define XLNID_DEV_ID_LK10               0xd400
#define XLNID_DEV_ID_LODESTAR           0x5148
#define XLNID_DEV_ID_EASTLAKE           0x3108
#define XLNID_DEV_ID_WESTLAKEPLUS       0x4000

/* INDIRECT_CMD_BUS */
#define XLNID_CMD_BUS_WEN               0x80000000
#define XLNID_CMD_EEPROM_DONE           0x20000000
#define XLNID_CMD_BUS_DONE              0x10000000
#define XLNID_CMD_BUS_BUSY              XLNID_CMD_BUS_DONE
#define XLNID_CMD_BUS_ADDR_MASK         0x01FFFFFF
#define XLNID_MDIO_BUSY                 0x00010000

#define XLNID_CMD_WAIT_MAX              10
#define XLNID_CMD_TRYS_MAX              100
#define XLNID_MDIO_WAIT_MAX             10

/* mac_cmd register */
#define XLNID_MAC_CMD_RD                0x00000000
#define XLNID_MAC_CMD_WR                0x80000000
#define XLNID_MAC_RW_ADDR_MASK          0x00003FFF

/* MAC Configuration Register */
#define XLNID_GMAC_CONFIG_RE            0x00000004

/* sgm_pcs_cfg0 */
#define XLNID_SGM_DUPLEX                0x00000040
#define XLNID_SGM_SPEED                 0x00000003
#define XLNID_SGM_SPEED_10              0x2
#define XLNID_SGM_SPEED_100             0x3
#define XLNID_SGM_SPEED_1000            0x0

/* sgm_pcs_stas0 */
#define XLNID_SGM_PCS_LINK_UP           0x00000040
#define XLNID_SGM_PCS_AN_STATUS         0x00000007
#define XLNID_SGM_PCS_RX_SYNC           0x00000010
#define XLNID_SGM_PCS_AN_10_HALF        0x00
#define XLNID_SGM_PCS_AN_10_FULL        0x01
#define XLNID_SGM_PCS_AN_100_HALF       0x02
#define XLNID_SGM_PCS_AN_100_FULL       0x03
#define XLNID_SGM_PCS_AN_1000_HALF      0x04
#define XLNID_SGM_PCS_AN_1000_FULL      0x05
#define XLNID_SGM_PCS_AN_COMPLETE       0x10

/* sgm_pcs_cfg1 */
#define XLNID_PCS_CFG1_SYM_PAUSE        0x10
#define XLNID_PCS_CFG1_ASM_PAUSE        0x20

/* sgm_pcs_stas1 */
#define XLNID_PCS_STAS1_SYM_PAUSE       0x4
#define XLNID_PCS_STAS1_ASM_PAUSE       0x8

/* RX_PBUF_STAT Register */
#define XLNID_RXCTL_PBUF_EMPTY          0x00000010

/* cfg_ptsw_mode register */
#define XLNID_CRG_RX_PTSW_PORT          0x00000004
#define XLNID_CRG_RX_PTSW_SOFTWARE      0x00000002
#define XLNID_CRG_RX_PTSW_ENABLE        0x00000001
#define XLNID_CRG_RX_PTSW_SOFTEN        (XLNID_CRG_RX_PTSW_SOFTWARE | XLNID_CRG_RX_PTSW_ENABLE)
#define XLNID_CRG_TX_PTSW_ENABLE        0x00000008
#define XLNID_CFG_ETH_LGC_EN            0x00000002
#define XLNID_CFG_PTSW_ENABLE           0x00000001

/* gephy_led_status register */
#define XLNID_PO_FIBER                  0x00000080
#define XLNID_PO_LINK                   0x00000008

#define XLNID_MAX_SENSORS       3

struct xlnid_thermal_diode_data {
	u8 location;
	u8 temp;
	u8 caution_thresh;
	u8 max_op_thresh;
};

struct xlnid_thermal_sensor_data {
	struct xlnid_thermal_diode_data sensor[XLNID_MAX_SENSORS];
};

struct xlnid_nvm_version {
	u32 etk_id;
	u8  nvm_major;
	u16 nvm_minor;
	u8  nvm_id;

	bool oem_valid;
	u8   oem_major;
	u8   oem_minor;
	u16  oem_release;

	bool or_valid;
	u8  or_major;
	u16 or_build;
	u8  or_patch;

};

/* EITR is only 12 bits, with the lower 3 always zero */
#define XLNID_MAX_INT_RATE  488281
#define XLNID_MIN_INT_RATE  956
#define XLNID_MAX_EITR      0x00000FF8
#define XLNID_MIN_EITR      8
#define XLNID_EITR_ITR_INT_MASK 0x00000FF8
#define XLNID_EITR_LLI_MOD  0x00008000
#define XLNID_EITR_CNT_WDIS 0x80000000

#define XLNID_DMATXCTL_TE   0x1 /* Transmit Enable */
#define XLNID_DMATXCTL_NS   0x2 /* No Snoop LSO hdr buffer */
#define XLNID_DMATXCTL_GDV  0x8 /* Global Double VLAN */
#define XLNID_DMATXCTL_MDP_EN   0x20 /* Bit 5 */
#define XLNID_DMATXCTL_MBINTEN  0x40 /* Bit 6 */
#define XLNID_DMATXCTL_VT_SHIFT 16  /* VLAN EtherType */

/* masks for accessing VXLAN and GENEVE UDP ports */
#define XLNID_VXLANCTRL_VXLAN_UDPPORT_MASK  0x0000ffff /* VXLAN port */
#define XLNID_VXLANCTRL_GENEVE_UDPPORT_MASK 0xffff0000 /* GENEVE port */
#define XLNID_VXLANCTRL_ALL_UDPPORT_MASK    0xffffffff /* GENEVE/VXLAN */
#define XLNID_VXLANCTRL_GENEVE_UDPPORT_SHIFT    16

/* Wake Up Filter Control */
#define XLNID_WUFC_LNKC 0x00000001 /* Link Status Change Wakeup Enable */
#define XLNID_WUFC_MAG  0x00000002 /* Magic Packet Wakeup Enable */
#define XLNID_WUFC_EX   0x00000004 /* Directed Exact Wakeup Enable */
#define XLNID_WUFC_MC   0x00000008 /* Directed Multicast Wakeup Enable */
#define XLNID_WUFC_BC   0x00000010 /* Broadcast Wakeup Enable */
#define XLNID_WUFC_ARP  0x00000020 /* ARP Request Packet Wakeup Enable */
#define XLNID_WUFC_IPV4 0x00000040 /* Directed IPv4 Packet Wakeup Enable */
#define XLNID_WUFC_IPV6 0x00000080 /* Directed IPv6 Packet Wakeup Enable */
#define XLNID_WUFC_MNG  0x00000100 /* Directed Mgmt Packet Wakeup Enable */

/* DCB registers */
#define XLNID_DCB_MAX_TRAFFIC_CLASS 8

/* DMA Coalescing configuration */
struct xlnid_dmac_config {
	u16 watchdog_timer; /* usec units */
	bool    fcoe_en;
	u32 link_speed;
	u8  fcoe_tc;
	u8  num_tcs;
};

/* LinkSec (MacSec) Bit Fields and Masks */
#define XLNID_LSECTXCAP_SUM_MASK    0x00FF0000
#define XLNID_LSECTXCAP_SUM_SHIFT   16
#define XLNID_LSECRXCAP_SUM_MASK    0x00FF0000
#define XLNID_LSECRXCAP_SUM_SHIFT   16

#define XLNID_LSECTXCTRL_EN_MASK    0x00000003
#define XLNID_LSECTXCTRL_DISABLE    0x0
#define XLNID_LSECTXCTRL_AUTH       0x1
#define XLNID_LSECTXCTRL_AUTH_ENCRYPT   0x2
#define XLNID_LSECTXCTRL_AISCI      0x00000020
#define XLNID_LSECTXCTRL_PNTHRSH_MASK   0xFFFFFF00
#define XLNID_LSECTXCTRL_RSV_MASK   0x000000D8

#define XLNID_LSECRXCTRL_EN_MASK    0x0000000C
#define XLNID_LSECRXCTRL_EN_SHIFT   2
#define XLNID_LSECRXCTRL_DISABLE    0x0
#define XLNID_LSECRXCTRL_CHECK      0x1
#define XLNID_LSECRXCTRL_STRICT     0x2
#define XLNID_LSECRXCTRL_DROP       0x3
#define XLNID_LSECRXCTRL_PLSH       0x00000040
#define XLNID_LSECRXCTRL_RP     0x00000080
#define XLNID_LSECRXCTRL_RSV_MASK   0xFFFFFF33

/* Management Bit Fields and Masks */
#define XLNID_MANC_MPROXYE  0x40000000 /* Management Proxy Enable */
#define XLNID_MANC_RCV_TCO_EN   0x00020000 /* Rcv TCO packet enable */
#define XLNID_MANC_EN_BMC2OS    0x10000000 /* Ena BMC2OS and OS2BMC traffic */
#define XLNID_MANC_EN_BMC2OS_SHIFT  28

/* RDRXCTL Bit Masks */
#define XLNID_RDRXCTL_RDMTS_1_2     0x00000000 /* Rx Desc Min THLD Size */
#define XLNID_RDRXCTL_CRCSTRIP      0x00000002 /* CRC Strip */
#define XLNID_RDRXCTL_PSP       0x00000004 /* Pad Small Packet */
#define XLNID_RDRXCTL_MVMEN     0x00000020
#define XLNID_RDRXCTL_RSC_PUSH_DIS  0x00000020
#define XLNID_RDRXCTL_DMAIDONE      0x00000008 /* DMA init cycle done */
#define XLNID_RDRXCTL_RSC_PUSH      0x00000080
#define XLNID_RDRXCTL_AGGDIS        0x00010000 /* Aggregation disable */
#define XLNID_RDRXCTL_RSCFRSTSIZE   0x003E0000 /* RSC First packet size */
#define XLNID_RDRXCTL_RSCLLIDIS     0x00800000 /* Disable RSC compl on LLI*/
#define XLNID_RDRXCTL_RSCACKC       0x02000000 /* must set 1 when RSC ena */
#define XLNID_RDRXCTL_FCOE_WRFIX    0x04000000 /* must set 1 when RSC ena */
#define XLNID_RDRXCTL_MBINTEN       0x10000000
#define XLNID_RDRXCTL_MDP_EN        0x20000000

/* GEPHY */
#define WESTLAKE_GEPHY_ADDR         0x8
#define XLNID_MDIO_CL22_WRITE       0x00050000
#define XLNID_MDIO_CL22_READ        0x00060000
#define XLNID_MDIO_ADDR_MASK        0x1F

/* General purpose Interrupt Enable */
#define XLNID_GPIE_MSIX_MODE    0x00000010 /* MSI-X mode */
#define XLNID_GPIE_OCD      0x00000020 /* Other Clear Disable */
#define XLNID_GPIE_EIMEN    0x00000040 /* Immediate Interrupt Enable */
#define XLNID_GPIE_EIAME    0x40000000
#define XLNID_GPIE_PBA_SUPPORT  0x80000000
#define XLNID_GPIE_RSC_DELAY_SHIFT  11
#define XLNID_GPIE_VTMODE_MASK  0x0000C000 /* VT Mode Mask */
#define XLNID_GPIE_VTMODE_16    0x00004000 /* 16 VFs 8 queues per VF */
#define XLNID_GPIE_VTMODE_32    0x00008000 /* 32 VFs 4 queues per VF */
#define XLNID_GPIE_VTMODE_64    0x0000C000 /* 64 VFs 2 queues per VF */

/* Packet Buffer Initialization */
#define XLNID_MAX_PACKET_BUFFERS    8

/* TCP Timer */
#define XLNID_TCPTIMER_KS       0x00000100
#define XLNID_TCPTIMER_COUNT_ENABLE 0x00000200
#define XLNID_TCPTIMER_COUNT_FINISH 0x00000400
#define XLNID_TCPTIMER_LOOP     0x00000800
#define XLNID_TCPTIMER_DURATION_MASK    0x000000FF

/* HLREG0 Bit Masks */
#define XLNID_HLREG0_TXCRCEN        0x00000001 /* bit  0 */
#define XLNID_HLREG0_RXCRCSTRP      0x00000002 /* bit  1 */
#define XLNID_HLREG0_JUMBOEN        0x00000004 /* bit  2 */
#define XLNID_HLREG0_TXPADEN        0x00000400 /* bit 10 */
#define XLNID_HLREG0_TXPAUSEEN      0x00001000 /* bit 12 */
#define XLNID_HLREG0_RXPAUSEEN      0x00004000 /* bit 14 */
#define XLNID_HLREG0_LPBK       0x00008000 /* bit 15 */
#define XLNID_HLREG0_MDCSPD     0x00010000 /* bit 16 */
#define XLNID_HLREG0_CONTMDC        0x00020000 /* bit 17 */
#define XLNID_HLREG0_CTRLFLTR       0x00040000 /* bit 18 */
#define XLNID_HLREG0_PREPEND        0x00F00000 /* bits 20-23 */
#define XLNID_HLREG0_PRIPAUSEEN     0x01000000 /* bit 24 */
#define XLNID_HLREG0_RXPAUSERECDA   0x06000000 /* bits 25-26 */
#define XLNID_HLREG0_RXLNGTHERREN   0x08000000 /* bit 27 */
#define XLNID_HLREG0_RXPADSTRIPEN   0x10000000 /* bit 28 */

/* VMOLR bitmasks */
#define XLNID_VMOLR_UPE     0x00400000 /* unicast promiscuous */
#define XLNID_VMOLR_VPE     0x00800000 /* VLAN promiscuous */
#define XLNID_VMOLR_AUPE    0x01000000 /* accept untagged packets */
#define XLNID_VMOLR_ROMPE   0x02000000 /* accept packets in MTA tbl */
#define XLNID_VMOLR_ROPE    0x04000000 /* accept packets in UC tbl */
#define XLNID_VMOLR_BAM     0x08000000 /* accept broadcast packets */
#define XLNID_VMOLR_MPE     0x10000000 /* multicast promiscuous */

/* Receive Checksum Control */
#define XLNID_RXCSUM_IPPCSE 0x00001000 /* IP payload checksum enable */
#define XLNID_RXCSUM_PCSD   0x00002000 /* packet checksum disabled */

/* RX_DROP_EN_MASK bitmasks */
#define XLNID_RX_UNDERSIZE_DROP_MASK    0x00000010

/* Interrupt register bitmasks */

/* Extended Interrupt Cause Read */
#define XLNID_EICR_RTX_QUEUE    0x0000FFFF /* RTx Queue Interrupt */
#define XLNID_EICR_FLOW_DIR 0x00010000 /* FDir Exception */
#define XLNID_EICR_RX_MISS  0x00020000 /* Packet Buffer Overrun */
#define XLNID_EICR_PCI      0x00040000 /* PCI Exception */
#define XLNID_EICR_MAILBOX  0x00080000 /* VF to PF Mailbox Interrupt */
#define XLNID_EICR_LSC      0x00100000 /* Link Status Change */
#define XLNID_EICR_LINKSEC  0x00200000 /* PN Threshold */
#define XLNID_EICR_MNG      0x00400000 /* Manageability Event Interrupt */
#define XLNID_EICR_FIFO_OVF     0x00800000 /* FIFO Overflow Interrupt */
#define XLNID_EICR_FIFO_UNDF    0x01000000 /* FIFO Underflow Interrupt */
#define XLNID_EICR_GPI_SDP0 0x01000000 /* Gen Purpose Interrupt on SDP0 */
#define XLNID_EICR_GPI_SDP1 0x02000000 /* Gen Purpose Interrupt on SDP1 */
#define XLNID_EICR_GPI_SDP2 0x04000000 /* Gen Purpose Interrupt on SDP2 */
#define XLNID_EICR_ECC      0x10000000 /* ECC Error */
#define XLNID_EICR_PBUR     0x10000000 /* Packet Buffer Handler Error */
#define XLNID_EICR_DHER     0x20000000 /* Descriptor Handler Error */
#define XLNID_EICR_TCP_TIMER    0x40000000 /* TCP Timer */
#define XLNID_EICR_OTHER    0x80000000 /* Interrupt Cause Active */

/* Extended Interrupt Cause Set */
#define XLNID_EICS_RTX_QUEUE    XLNID_EICR_RTX_QUEUE /* RTx Queue Interrupt */
#define XLNID_EICS_FLOW_DIR XLNID_EICR_FLOW_DIR  /* FDir Exception */
#define XLNID_EICS_RX_MISS  XLNID_EICR_RX_MISS   /* Pkt Buffer Overrun */
#define XLNID_EICS_PCI      XLNID_EICR_PCI /* PCI Exception */
#define XLNID_EICS_MAILBOX  XLNID_EICR_MAILBOX   /* VF to PF Mailbox Int */
#define XLNID_EICS_LSC      XLNID_EICR_LSC /* Link Status Change */
#define XLNID_EICS_MNG      XLNID_EICR_MNG /* MNG Event Interrupt */
#define XLNID_EICS_TIMESYNC XLNID_EICR_TIMESYNC /* Timesync Event */
#define XLNID_EICS_GPI_SDP0 XLNID_EICR_GPI_SDP0 /* SDP0 Gen Purpose Int */
#define XLNID_EICS_GPI_SDP1 XLNID_EICR_GPI_SDP1 /* SDP1 Gen Purpose Int */
#define XLNID_EICS_GPI_SDP2 XLNID_EICR_GPI_SDP2 /* SDP2 Gen Purpose Int */
#define XLNID_EICS_ECC      XLNID_EICR_ECC /* ECC Error */
#define XLNID_EICS_PBUR     XLNID_EICR_PBUR /* Pkt Buf Handler Err */
#define XLNID_EICS_DHER     XLNID_EICR_DHER /* Desc Handler Error */
#define XLNID_EICS_TCP_TIMER    XLNID_EICR_TCP_TIMER /* TCP Timer */
#define XLNID_EICS_OTHER    XLNID_EICR_OTHER /* INT Cause Active */

/* Extended Interrupt Mask Set */
#define XLNID_EIMS_RTX_QUEUE    XLNID_EICR_RTX_QUEUE /* RTx Queue Interrupt */
#define XLNID_EIMS_FLOW_DIR XLNID_EICR_FLOW_DIR /* FDir Exception */
#define XLNID_EIMS_RX_MISS  XLNID_EICR_RX_MISS /* Packet Buffer Overrun */
#define XLNID_EIMS_PCI      XLNID_EICR_PCI /* PCI Exception */
#define XLNID_EIMS_MAILBOX  XLNID_EICR_MAILBOX   /* VF to PF Mailbox Int */
#define XLNID_EIMS_LSC      XLNID_EICR_LSC /* Link Status Change */
#define XLNID_EIMS_LINKSEC  XLNID_EICR_LINKSEC /* PN Threshold */
#define XLNID_EIMS_MNG      XLNID_EICR_MNG /* MNG Event Interrupt */
#define XLNID_EIMS_GPI_SDP0 XLNID_EICR_GPI_SDP0 /* SDP0 Gen Purpose Int */
#define XLNID_EIMS_GPI_SDP1 XLNID_EICR_GPI_SDP1 /* SDP1 Gen Purpose Int */
#define XLNID_EIMS_GPI_SDP2 XLNID_EICR_GPI_SDP2 /* SDP2 Gen Purpose Int */
#define XLNID_EIMS_ECC      XLNID_EICR_ECC /* ECC Error */
#define XLNID_EIMS_PBUR     XLNID_EICR_PBUR /* Pkt Buf Handler Err */
#define XLNID_EIMS_DHER     XLNID_EICR_DHER /* Descr Handler Error */
#define XLNID_EIMS_TCP_TIMER    XLNID_EICR_TCP_TIMER /* TCP Timer */
#define XLNID_EIMS_OTHER    XLNID_EICR_OTHER /* INT Cause Active */

/* Extended Interrupt Mask Clear */
#define XLNID_EIMC_RTX_QUEUE    XLNID_EICR_RTX_QUEUE /* RTx Queue Interrupt */
#define XLNID_EIMC_FLOW_DIR XLNID_EICR_FLOW_DIR /* FDir Exception */
#define XLNID_EIMC_RX_MISS  XLNID_EICR_RX_MISS /* Packet Buffer Overrun */
#define XLNID_EIMC_PCI      XLNID_EICR_PCI /* PCI Exception */
#define XLNID_EIMC_MAILBOX  XLNID_EICR_MAILBOX /* VF to PF Mailbox Int */
#define XLNID_EIMC_LSC      XLNID_EICR_LSC /* Link Status Change */
#define XLNID_EIMC_LINKSEC  XLNID_EICR_LINKSEC /* PN Threshold */
#define XLNID_EIMC_MNG      XLNID_EICR_MNG /* MNG Event Interrupt */
#define XLNID_EIMC_TIMESYNC XLNID_EICR_TIMESYNC /* Timesync Event */
#define XLNID_EIMC_GPI_SDP0 XLNID_EICR_GPI_SDP0 /* SDP0 Gen Purpose Int */
#define XLNID_EIMC_GPI_SDP1 XLNID_EICR_GPI_SDP1 /* SDP1 Gen Purpose Int */
#define XLNID_EIMC_GPI_SDP2 XLNID_EICR_GPI_SDP2  /* SDP2 Gen Purpose Int */
#define XLNID_EIMC_ECC      XLNID_EICR_ECC /* ECC Error */
#define XLNID_EIMC_PBUR     XLNID_EICR_PBUR /* Pkt Buf Handler Err */
#define XLNID_EIMC_DHER     XLNID_EICR_DHER /* Desc Handler Err */
#define XLNID_EIMC_TCP_TIMER    XLNID_EICR_TCP_TIMER /* TCP Timer */
#define XLNID_EIMC_OTHER    XLNID_EICR_OTHER /* INT Cause Active */

#define XLNID_EIMS_ENABLE_MASK ( \
				XLNID_EIMS_RTX_QUEUE    | \
				XLNID_EIMS_LSC      | \
				XLNID_EIMS_TCP_TIMER    | \
				XLNID_EIMS_OTHER)

/* Immediate Interrupt Rx (A.K.A. Low Latency Interrupt) */
#define XLNID_IMIR_SIZE_BP      0x00001000 /* Packet size bypass */
#define XLNID_IMIR_CTRL_URG     0x00002000 /* Check URG bit in header */
#define XLNID_IMIR_CTRL_ACK     0x00004000 /* Check ACK bit in header */
#define XLNID_IMIR_CTRL_PSH     0x00008000 /* Check PSH bit in header */
#define XLNID_IMIR_CTRL_RST     0x00010000 /* Check RST bit in header */
#define XLNID_IMIR_CTRL_SYN     0x00020000 /* Check SYN bit in header */
#define XLNID_IMIR_CTRL_FIN     0x00040000 /* Check FIN bit in header */
#define XLNID_IMIR_CTRL_BP          0x00080000 /* Bypass chk of ctrl bits */
#define XLNID_IMIR_LLI_EN           0x00100000 /* Enables low latency Int */
#define XLNID_IMIR_RX_QUEUE_MASK    0x0000007F /* Rx Queue Mask */
#define XLNID_IMIR_RX_QUEUE_SHIFT   21 /* Rx Queue Shift */
#define XLNID_IMIRVP_PRIORITY_MASK  0x00000007 /* VLAN priority mask */
#define XLNID_IMIRVP_PRIORITY_EN    0x00000008 /* VLAN priority enable */

#define XLNID_MAX_FTQF_FILTERS      128
#define XLNID_FTQF_PROTOCOL_MASK    0x00000003
#define XLNID_FTQF_PROTOCOL_TCP     0x00000000
#define XLNID_FTQF_PROTOCOL_UDP     0x00000001
#define XLNID_FTQF_PROTOCOL_SCTP    2
#define XLNID_FTQF_PRIORITY_MASK    0x00000007
#define XLNID_FTQF_PRIORITY_SHIFT   2
#define XLNID_FTQF_POOL_MASK        0x0000003F
#define XLNID_FTQF_POOL_SHIFT       8
#define XLNID_FTQF_5TUPLE_MASK_MASK 0x0000001F
#define XLNID_FTQF_5TUPLE_MASK_SHIFT    25
#define XLNID_FTQF_SOURCE_ADDR_MASK 0x1E
#define XLNID_FTQF_DEST_ADDR_MASK   0x1D
#define XLNID_FTQF_SOURCE_PORT_MASK 0x1B
#define XLNID_FTQF_DEST_PORT_MASK   0x17
#define XLNID_FTQF_PROTOCOL_COMP_MASK   0x0F
#define XLNID_FTQF_POOL_MASK_EN     0x40000000
#define XLNID_FTQF_QUEUE_ENABLE     0x80000000

/* Interrupt clear mask */
#define XLNID_IRQ_CLEAR_MASK    0xFFFFFFFF

/* Interrupt Vector Allocation Registers */
#define XLNID_IVAR_REG_NUM      64
#define XLNID_IVAR_TXRX_ENTRY       96
#define XLNID_IVAR_RX_ENTRY     64
#define XLNID_IVAR_RX_QUEUE(_i)     (0 + (_i))
#define XLNID_IVAR_TX_QUEUE(_i)     (64 + (_i))
#define XLNID_IVAR_TX_ENTRY     32

#define XLNID_IVAR_TCP_TIMER_INDEX  96 /* 0 based index */
#define XLNID_IVAR_OTHER_CAUSES_INDEX   97 /* 0 based index */

#define XLNID_MSIX_VECTOR(_i)       (0 + (_i))

#define XLNID_IVAR_ALLOC_VAL        0x80 /* Interrupt Allocation valid */

/* ETYPE Queue Filter/Select Bit Masks */
#define XLNID_MAX_ETQF_FILTERS      8
#define XLNID_ETQF_FCOE         0x08000000 /* bit 27 */
#define XLNID_ETQF_BCN          0x10000000 /* bit 28 */
#define XLNID_ETQF_TX_ANTISPOOF     0x20000000 /* bit 29 */
#define XLNID_ETQF_1588         0x40000000 /* bit 30 */
#define XLNID_ETQF_FILTER_EN        0x80000000 /* bit 31 */
#define XLNID_ETQF_POOL_ENABLE      (1 << 26) /* bit 26 */
#define XLNID_ETQF_POOL_SHIFT       20

#define XLNID_ETQS_RX_QUEUE     0x007F0000 /* bits 22:16 */
#define XLNID_ETQS_RX_QUEUE_SHIFT   16
#define XLNID_ETQS_LLI          0x20000000 /* bit 29 */
#define XLNID_ETQS_QUEUE_EN     0x80000000 /* bit 31 */

/*
		ETQF filter list: one static filter per filter consumer. This is
				to avoid filter collisions later. Add new filters
				here!!

		Current filters:
		EAPOL 802.1x (0x888e): Filter 0
		FCoE (0x8906):   Filter 2
		1588 (0x88f7):   Filter 3
		FIP  (0x8914):   Filter 4
		LLDP (0x88CC):   Filter 5
		LACP (0x8809):   Filter 6
		FC   (0x8808):   Filter 7
*/
#define XLNID_ETQF_FILTER_EAPOL     0
#define XLNID_ETQF_FILTER_FCOE      2
#define XLNID_ETQF_FILTER_1588      3
#define XLNID_ETQF_FILTER_FIP       4
#define XLNID_ETQF_FILTER_LLDP      5
#define XLNID_ETQF_FILTER_LACP      6
#define XLNID_ETQF_FILTER_FC        7
/* VLAN Control Bit Masks */
#define XLNID_VLNCTRL_VET       0x0000FFFF  /* bits 0-15 */
#define XLNID_VLNCTRL_CFI       0x10000000  /* bit 28 */
#define XLNID_VLNCTRL_CFIEN     0x20000000  /* bit 29 */
#define XLNID_VLNCTRL_VFE       0x40000000  /* bit 30 */
#define XLNID_VLNCTRL_VME       0x80000000  /* bit 31 */

/* VLAN pool filtering masks */
#define XLNID_VLVF_VIEN         0x80000000  /* filter is valid */
#define XLNID_VLVF_ENTRIES      64
#define XLNID_VLVF_VLANID_MASK      0x00000FFF

#define XLNID_ETHERNET_IEEE_VLAN_TYPE   0x8100  /* 802.1q protocol */

/* LEDCTL Bit Masks */
#define XLNID_LED_IVRT_BASE     0x00000040
#define XLNID_LED_BLINK_BASE        0x00000080
#define XLNID_LED_MODE_MASK_BASE    0x0000000F
#define XLNID_LED_OFFSET(_base, _i) (_base << (8 * (_i)))
#define XLNID_LED_MODE_SHIFT(_i)    (8*(_i))
#define XLNID_LED_IVRT(_i)  XLNID_LED_OFFSET(XLNID_LED_IVRT_BASE, _i)
#define XLNID_LED_BLINK(_i) XLNID_LED_OFFSET(XLNID_LED_BLINK_BASE, _i)
#define XLNID_LED_MODE_MASK(_i) XLNID_LED_OFFSET(XLNID_LED_MODE_MASK_BASE, _i)

/* LED modes */
#define XLNID_LED_LINK          0x0
#define XLNID_LED_ACTIVITY      0x1
#define XLNID_LED_SPPED0        0x2
#define XLNID_LED_SPEED1        0x3
#define XLNID_LED_LINK_ACTIVE   0x4
#define XLNID_LED_ON            0xE
#define XLNID_LED_OFF           0xF

/* GMAC Bit Masks */
#define XLNID_GMAC_PS_MASK      0x00008000
#define XLNID_GMAC_FES_MASK     0x00004000
#define XLNID_GMAC_DM_MASK      0x00000800
#define XLNID_GMAC_LUD_MASK     0x00000100
#define XLNID_GMAC_LM_MASK      0x00001000

/* PHYSTATUS Bit Masks */
#define XLNID_PHYSTATUS_ADDR0   0x3E000000
#define XLNID_PHYSTATUS_LINK0   0x00040000
#define XLNID_PHYSTATUS_SPEED0  0x0000C000
#define XLNID_PHYSTATUS_DUPLEX0 0x00001000

/* AUTOC Bit Masks */
#define XLNID_AUTOC_AN_RESTART  0x00001000
#define XLNID_AUTOC_FLU     0x00000001

#define XLNID_LINK_UP_TIME      150 /* 15 Seconds */
#define XLNID_AUTO_NEG_TIME     150 /* 15 Seconds */

/* EEC Register */
#define XLNID_EEC_PRES      0x00000100 /* EEPROM Present */

/* Checksum and EEPROM pointers */
#define XLNID_EEPROM_SUM        0xBABA
#define XLNID_SAN_MAC_ADDR_PORT0_OFFSET_L   0x0C
#define XLNID_SAN_MAC_ADDR_PORT0_OFFSET_H   0x1C
#define XLNID_SAN_MAC_ADDR_PORT1_OFFSET_L   0x2C
#define XLNID_SAN_MAC_ADDR_PORT1_OFFSET_H   0x3C

#define XLNID_MAX_MSIX_VECTORS      0x40
#define XLNID_PCIE_MSIX_CAPS        0xB2

/* MSI-X capability fields masks */
#define XLNID_PCIE_MSIX_TBL_SZ_MASK 0x7FF

#define XLNID_ETH_LENGTH_OF_ADDRESS 6

#define XLNID_DEVICE_CAPS_NO_CROSSTALK_WR   (1 << 7)

/* PCI Bus Info */
#define XLNID_PCI_DEVICE_STATUS     0x7A
#define XLNID_PCI_DEVICE_STATUS_TRANSACTION_PENDING 0x0020
#define XLNID_PCI_LINK_STATUS       0x82
#define XLNID_PCI_DEVICE_CONTROL2   0x98
#define XLNID_PCI_LINK_WIDTH        0x3F0
#define XLNID_PCI_LINK_WIDTH_1      0x10
#define XLNID_PCI_LINK_WIDTH_2      0x20
#define XLNID_PCI_LINK_WIDTH_4      0x40
#define XLNID_PCI_LINK_WIDTH_8      0x80
#define XLNID_PCI_LINK_SPEED        0xF
#define XLNID_PCI_LINK_SPEED_2500   0x1
#define XLNID_PCI_LINK_SPEED_5000   0x2
#define XLNID_PCI_LINK_SPEED_8000   0x3
#define XLNID_PCI_LINK_SPEED_16000  0x4
#define XLNID_PCI_HEADER_TYPE_REGISTER  0x0E
#define XLNID_PCI_HEADER_TYPE_MULTIFUNC 0x80
#define XLNID_PCI_DEVICE_CONTROL2_16ms  0x0005

#define XLNID_PCIDEVCTRL2_TIMEO_MASK    0xf
#define XLNID_PCIDEVCTRL2_16_32ms_def   0x0
#define XLNID_PCIDEVCTRL2_50_100us  0x1
#define XLNID_PCIDEVCTRL2_1_2ms     0x2
#define XLNID_PCIDEVCTRL2_16_32ms   0x5
#define XLNID_PCIDEVCTRL2_65_130ms  0x6
#define XLNID_PCIDEVCTRL2_260_520ms 0x9
#define XLNID_PCIDEVCTRL2_1_2s      0xa
#define XLNID_PCIDEVCTRL2_4_8s      0xd
#define XLNID_PCIDEVCTRL2_17_34s    0xe

/* Number of 100 microseconds we wait for PCI Express primary disable */
#define XLNID_PCI_PRIMARY_DISABLE_TIMEOUT   800

/* Check whether address is multicast. This is little-endian specific check.*/
#define XLNID_IS_MULTICAST(Address) \
		((bool)(((u8 *)(Address))[0] & ((u8)0x01)))

/* Check whether an address is broadcast. */
#define XLNID_IS_BROADCAST(Address) \
		((((u8 *)(Address))[0] == ((u8)0xff)) && \
		(((u8 *)(Address))[1] == ((u8)0xff)))

/* RAH */
#define XLNID_RAH_VIND_MASK 0x003C0000
#define XLNID_RAH_VIND_SHIFT    18
#define XLNID_RAH_AV        0x80000000
#define XLNID_CLEAR_VMDQ_ALL    0xFFFFFFFF

/* Transmit Config masks */
#define XLNID_TXDCTL_ENABLE     0x02000000 /* Ena specific Tx Queue */
#define XLNID_TXDCTL_SWFLSH     0x04000000 /* Tx Desc. wr-bk flushing */
#define XLNID_TXDCTL_WTHRESH_SHIFT  16 /* shift to WTHRESH bits */

/* Receive Config masks */
#define XLNID_RXCTRL_RXEN       0x00000001 /* Enable Receiver */
#define XLNID_RXCTRL_DMBYPS     0x00000002 /* Desc Monitor Bypass */
#define XLNID_RXDCTL_ENABLE     0x02000000 /* Ena specific Rx Queue */
#define XLNID_RXDCTL_SWFLSH     0x04000000 /* Rx Desc wr-bk flushing */
#define XLNID_RXDCTL_RLPML_EN       0x00008000
#define XLNID_RXDCTL_PTHRESH    0x10

#define XLNID_CFG_TX_TS_UPLOAD_EN   0x00000001 /* Tx snap enabled */
#define XLNID_CFG_RX_TS_UPLOAD_EN   0x00000002 /* Rx snap enabled */
#define XLNID_CFG_TX_CF_EN          0x00000001 /* Tx timestamping enabled */
#define XLNID_CFG_RX_CF_EN          0x00000010 /* Rx timestamping enabled */

#define XLNID_TX_SNAP_TS_RDY        0x00000002 /* Tx snap ready */
#define XLNID_RX_SNAP_TS_RDY        0x00000001 /* Rx snap ready */

#define XLNID_TSYNCTXCTL_ENABLED    0x00000001 /* Tx Timestamping enabled */
#define XLNID_TSYNCRXCTL_ENABLED    0x00000002 /* Rx Timestamping enabled */

#define XLNID_SYS_PLL_LOCKED        0x00001 /* Sys time stat */
#define XLNID_CFG_ETH_PTP_CLK_SEL   0x00001 /* Sys clk */

#define XLNID_FCTRL_SBP     0x00000002 /* Store Bad Packet */
#define XLNID_FCTRL_MPE     0x00000100 /* Multicast Promiscuous Ena*/
#define XLNID_FCTRL_UPE     0x00000200 /* Unicast Promiscuous Ena */
#define XLNID_FCTRL_BAM     0x00000400 /* Broadcast Accept Mode */
#define XLNID_FCTRL_PMCF    0x00001000 /* Pass MAC Control Frames */
#define XLNID_FCTRL_DPF     0x00002000 /* Discard Pause Frame */

/* Flow Control */
#define XLNID_FC_EN          0x00000001 /* flow contral enable */
#define XLNID_FC_UN          0x00000000 /* flow control unable */
#define XLNID_RX_PAUSE_EN    0x00000001 /* rx response */
#define XLNID_RX_BP_MASK_DEFAULT     0x00000013
#define XLNID_RX_BP_MASK             0x00000017
#define XLNID_RX_PBUF_ALF_POINT0    0x03D003FA /* high-low water */
#define XLNID_RX_PDBUF_ALF_POINT0   0x00F400FD
#define XLNID_RX_PBUF_ALF_POINT1    0x01D001FA /* high-low water */
#define XLNID_RX_PDBUF_ALF_POINT1   0x0074007D
#define XLNID_RX_PBUF_ALF_POINT_DEFAULT0    0x03FA03FA
#define XLNID_RX_PDBUF_ALF_POINT_DEFAULT0   0x00FD00FD
#define XLNID_RX_PBUF_ALF_POINT_DEFAULT1    0x01FA01FA
#define XLNID_RX_PDBUF_ALF_POINT_DEFAULT1   0x007D007D

#define XLNID_PHY_ANAR_SYM_PAUSE    0x400
#define XLNID_PHY_ANAR_ASM_PAUSE    0x800
#define XLNID_PHY_ANLPAR_SYM_PAUSE  0x400
#define XLNID_PHY_ANLPAR_ASM_PAUSE  0x800

/* Multiple Receive Queue Control */
#define XLNID_MRQC_RSSEN    0x00000001  /* RSS Enable */
#define XLNID_MRQC_MRQE_MASK    0xF /* Bits 3:0 */
#define XLNID_MRQC_RT8TCEN  0x00000002 /* 8 TC no RSS */
#define XLNID_MRQC_RT4TCEN  0x00000003 /* 4 TC no RSS */
#define XLNID_MRQC_RTRSS8TCEN   0x00000004 /* 8 TC w/ RSS */
#define XLNID_MRQC_RTRSS4TCEN   0x00000005 /* 4 TC w/ RSS */
#define XLNID_MRQC_VMDQEN   0x00000008 /* VMDq2 64 pools no RSS */
#define XLNID_MRQC_VMDQRSS32EN  0x0000000A /* VMDq2 32 pools w/ RSS */
#define XLNID_MRQC_VMDQRSS64EN  0x0000000B /* VMDq2 64 pools w/ RSS */
#define XLNID_MRQC_VMDQRT8TCEN  0x0000000C /* VMDq2/RT 16 pool 8 TC */
#define XLNID_MRQC_VMDQRT4TCEN  0x0000000D /* VMDq2/RT 32 pool 4 TC */
#define XLNID_MRQC_L3L4TXSWEN   0x00008000 /* Enable L3/L4 Tx switch */
#define XLNID_MRQC_RSS_FIELD_MASK   0xFFFF0000
#define XLNID_MRQC_RSS_FIELD_IPV4_TCP   0x00010000
#define XLNID_MRQC_RSS_FIELD_IPV4   0x00020000
#define XLNID_MRQC_RSS_FIELD_IPV6_EX_TCP 0x00040000
#define XLNID_MRQC_RSS_FIELD_IPV6_EX    0x00080000
#define XLNID_MRQC_RSS_FIELD_IPV6   0x00100000
#define XLNID_MRQC_RSS_FIELD_IPV6_TCP   0x00200000
#define XLNID_MRQC_RSS_FIELD_IPV4_UDP   0x00400000
#define XLNID_MRQC_RSS_FIELD_IPV6_UDP   0x00800000
#define XLNID_MRQC_RSS_FIELD_IPV6_EX_UDP 0x01000000
#define XLNID_MRQC_MULTIPLE_RSS     0x00002000

#define XLNID_TXD_POPTS_IXSM    0x01 /* Insert IP checksum */
#define XLNID_TXD_POPTS_TXSM    0x02 /* Insert TCP/UDP checksum */
#define XLNID_TXD_CMD_EOP   0x01000000 /* End of Packet */
#define XLNID_TXD_CMD_IFCS  0x02000000 /* Insert FCS (Ethernet CRC) */
#define XLNID_TXD_CMD_IC    0x04000000 /* Insert Checksum */
#define XLNID_TXD_CMD_RS    0x08000000 /* Report Status */
#define XLNID_TXD_CMD_DEXT  0x20000000 /* Desc extension (0 = legacy) */
#define XLNID_TXD_CMD_VLE   0x40000000 /* Add VLAN tag */
#define XLNID_TXD_STAT_DD   0x80000000 /* Descriptor Done */

/* Multiple Transmit Queue Command Register */
#define XLNID_MTQC_RT_ENA   0x1 /* DCB Enable */
#define XLNID_MTQC_VT_ENA   0x2 /* VMDQ2 Enable */
#define XLNID_MTQC_64Q_1PB  0x0 /* 64 queues 1 pack buffer */
#define XLNID_MTQC_32VF     0x8 /* 4 TX Queues per pool w/32VF's */
#define XLNID_MTQC_64VF     0x4 /* 2 TX Queues per pool w/64VF's */
#define XLNID_MTQC_4TC_4TQ  0x8 /* 4 TC if RT_ENA and VT_ENA */
#define XLNID_MTQC_8TC_8TQ  0xC /* 8 TC if RT_ENA or 8 TQ if VT_ENA */

/* Receive Descriptor bit definitions */
#define XLNID_RXD_STAT_DD   0x01 /* Descriptor Done */
#define XLNID_RXD_STAT_EOP  0x02 /* End of Packet */
#define XLNID_RXD_STAT_LLI  0x04 /* Low Latency Interrupt */
#define XLNID_RXD_STAT_FLM  0x08 /* FDir Match */
#define XLNID_RXD_STAT_VP   0x10 /* IEEE VLAN Packet */
#define XLNID_RXDADV_NEXTP_MASK 0x000FFFF0 /* Next Descriptor Index */
#define XLNID_RXDADV_NEXTP_SHIFT    0x00000004
#define XLNID_RXD_STAT_UDPCS    0x20 /* UDP xsum calculated */
#define XLNID_RXD_STAT_L4CS 0x40 /* L4 xsum calculated */
#define XLNID_RXD_STAT_IPCS 0x80 /* IP xsum calculated */
#define XLNID_RXD_STAT_PIF  0x100 /* passed in-exact filter */
#define XLNID_RXD_STAT_CRCV 0x100 /* Speculative CRC Valid */
#define XLNID_RXD_STAT_OUTERIPCS    0x100 /* Cloud IP xsum calculated */
#define XLNID_RXD_STAT_VEXT 0x400 /* 1st VLAN found */
#define XLNID_RXD_STAT_UDPV 0x800 /* Valid UDP checksum */
#define XLNID_RXD_STAT_DYNINT   0x800 /* Pkt caused INT via DYNINT */
#define XLNID_RXD_STAT_LLINT    0x800 /* Pkt caused Low Latency Interrupt */
#define XLNID_RXD_STAT_TSIP 0x08000 /* Time Stamp in packet buffer */
#define XLNID_RXD_STAT_TS   0x10000 /* Time Stamp */
#define XLNID_RXD_STAT_SECP 0x20000 /* Security Processing */
#define XLNID_RXD_STAT_LB   0x40000 /* Loopback Status */
#define XLNID_RXD_STAT_ACK  0x8000 /* ACK Packet indication */
#define XLNID_RXD_ERR_CE    0x01 /* CRC Error */
#define XLNID_RXD_ERR_LE    0x02 /* Length Error */
#define XLNID_RXD_ERR_PE    0x08 /* Packet Error */
#define XLNID_RXD_ERR_OSE   0x10 /* Oversize Error */
#define XLNID_RXD_ERR_USE   0x20 /* Undersize Error */
#define XLNID_RXD_ERR_TCPE  0x40 /* TCP/UDP Checksum Error */
#define XLNID_RXD_ERR_IPE   0x80 /* IP Checksum Error */
#define XLNID_RXDADV_ERR_MASK       0xfff00000 /* RDESC.ERRORS mask */
#define XLNID_RXDADV_ERR_SHIFT      20 /* RDESC.ERRORS shift */
#define XLNID_RXDADV_ERR_OUTERIPER  0x04000000 /* CRC IP Header error */
#define XLNID_RXDADV_ERR_RXE        0x20000000 /* Any MAC Error */
#define XLNID_RXDADV_ERR_FCEOFE     0x80000000 /* FCEOFe/IPE */
#define XLNID_RXDADV_ERR_FCERR      0x00700000 /* FCERR/FDIRERR */
#define XLNID_RXDADV_ERR_FDIR_LEN   0x00100000 /* FDIR Length error */
#define XLNID_RXDADV_ERR_FDIR_DROP  0x00200000 /* FDIR Drop error */
#define XLNID_RXDADV_ERR_FDIR_COLL  0x00400000 /* FDIR Collision error */
#define XLNID_RXDADV_ERR_HBO    0x00800000 /*Header Buffer Overflow */
#define XLNID_RXDADV_ERR_CE 0x01000000 /* CRC Error */
#define XLNID_RXDADV_ERR_LE 0x02000000 /* Length Error */
#define XLNID_RXDADV_ERR_PE 0x08000000 /* Packet Error */
#define XLNID_RXDADV_ERR_OSE    0x10000000 /* Oversize Error */
#define XLNID_RXDADV_ERR_USE    0x20000000 /* Undersize Error */
#define XLNID_RXDADV_ERR_TCPE   0x40000000 /* TCP/UDP Checksum Error */
#define XLNID_RXDADV_ERR_IPE    0x80000000 /* IP Checksum Error */
#define XLNID_RXD_VLAN_ID_MASK  0x0FFF  /* VLAN ID is in lower 12 bits */
#define XLNID_RXD_PRI_MASK  0xE000  /* Priority is in upper 3 bits */
#define XLNID_RXD_PRI_SHIFT 13
#define XLNID_RXD_CFI_MASK  0x1000  /* CFI is bit 12 */
#define XLNID_RXD_CFI_SHIFT 12

#define XLNID_RXDADV_STAT_DD        XLNID_RXD_STAT_DD  /* Done */
#define XLNID_RXDADV_STAT_EOP       XLNID_RXD_STAT_EOP /* End of Packet */
#define XLNID_RXDADV_STAT_FLM       XLNID_RXD_STAT_FLM /* FDir Match */
#define XLNID_RXDADV_STAT_VP        XLNID_RXD_STAT_VP  /* IEEE VLAN Pkt */
#define XLNID_RXDADV_STAT_MASK      0x000fffff /* Stat/NEXTP: bit 0-19 */
#define XLNID_RXDADV_STAT_FCEOFS    0x00000040 /* FCoE EOF/SOF Stat */
#define XLNID_RXDADV_STAT_FCSTAT    0x00000030 /* FCoE Pkt Stat */
#define XLNID_RXDADV_STAT_FCSTAT_NOMTCH 0x00000000 /* 00: No Ctxt Match */
#define XLNID_RXDADV_STAT_FCSTAT_NODDP  0x00000010 /* 01: Ctxt w/o DDP */
#define XLNID_RXDADV_STAT_FCSTAT_FCPRSP 0x00000020 /* 10: Recv. FCP_RSP */
#define XLNID_RXDADV_STAT_FCSTAT_DDP    0x00000030 /* 11: Ctxt w/ DDP */
#define XLNID_RXDADV_STAT_TS        0x00040000 /* IEEE1588 Time Stamp */
#define XLNID_RXDADV_STAT_TSIP      0x00008000 /* Time Stamp in packet buffer */

#define XLNID_RXD32_STAT_DD         0x8000  /* Done */
#define XLNID_RXD32_STAT_EOP        0x0100  /* End of Packet */

/* SRRCTL bit definitions */
#define XLNID_SRRCTL_BSIZEPKT_SHIFT 10 /* so many KBs */
#define XLNID_SRRCTL_BSIZEHDRSIZE_SHIFT 2 /* 64byte resolution (>> 6)
							+ at bit 8 offset (<< 8)
							= (<< 2)
*/
#define XLNID_SRRCTL_RDMTS_SHIFT    22
#define XLNID_SRRCTL_RDMTS_MASK     0x01C00000
#define XLNID_SRRCTL_DROP_EN        0x10000000
#define XLNID_SRRCTL_BSIZEPKT_MASK  0x0000007F
#define XLNID_SRRCTL_BSIZEHDR_MASK  0x00003F00
#define XLNID_SRRCTL_DESCTYPE_LEGACY    0x00000000
#define XLNID_SRRCTL_DESCTYPE_ADV_ONEBUF 0x02000000
#define XLNID_SRRCTL_DESCTYPE_HDR_SPLIT 0x04000000
#define XLNID_SRRCTL_DESCTYPE_HDR_REPLICATION_LARGE_PKT 0x08000000
#define XLNID_SRRCTL_DESCTYPE_HDR_SPLIT_ALWAYS 0x0A000000
#define XLNID_SRRCTL_DESCTYPE_MASK  0x0E000000

#define XLNID_RXDADV_RSSTYPE_MASK   0x0000000F
#define XLNID_RXDADV_PKTTYPE_MASK   0x0000FFF0
#define XLNID_RXDADV_PKTTYPE_MASK_EX    0x0001FFF0
#define XLNID_RXDADV_HDRBUFLEN_MASK 0x00007FE0
#define XLNID_RXDADV_RSCCNT_MASK    0x001E0000
#define XLNID_RXDADV_RSCCNT_SHIFT   17
#define XLNID_RXDADV_HDRBUFLEN_SHIFT    5
#define XLNID_RXDADV_SPLITHEADER_EN 0x00001000
#define XLNID_RXDADV_SPH        0x8000

/* RSS Hash results */
#define XLNID_RXDADV_RSSTYPE_NONE   0x00000000
#define XLNID_RXDADV_RSSTYPE_IPV4_TCP   0x00000001
#define XLNID_RXDADV_RSSTYPE_IPV4   0x00000002
#define XLNID_RXDADV_RSSTYPE_IPV6_TCP   0x00000003
#define XLNID_RXDADV_RSSTYPE_IPV6_EX    0x00000004
#define XLNID_RXDADV_RSSTYPE_IPV6   0x00000005
#define XLNID_RXDADV_RSSTYPE_IPV6_TCP_EX 0x00000006
#define XLNID_RXDADV_RSSTYPE_IPV4_UDP   0x00000007
#define XLNID_RXDADV_RSSTYPE_IPV6_UDP   0x00000008
#define XLNID_RXDADV_RSSTYPE_IPV6_UDP_EX 0x00000009

/* RSS Packet Types as indicated in the receive descriptor. */
#define XLNID_RXDADV_PKTTYPE_NONE   0x00000000
#define XLNID_RXDADV_PKTTYPE_IPV4   0x00000010 /* IPv4 hdr present */
#define XLNID_RXDADV_PKTTYPE_IPV4_EX    0x00000020 /* IPv4 hdr + extensions */
#define XLNID_RXDADV_PKTTYPE_IPV6   0x00000040 /* IPv6 hdr present */
#define XLNID_RXDADV_PKTTYPE_IPV6_EX    0x00000080 /* IPv6 hdr + extensions */
#define XLNID_RXDADV_PKTTYPE_TCP    0x00000100 /* TCP hdr present */
#define XLNID_RXDADV_PKTTYPE_UDP    0x00000200 /* UDP hdr present */
#define XLNID_RXDADV_PKTTYPE_SCTP   0x00000400 /* SCTP hdr present */
#define XLNID_RXDADV_PKTTYPE_NFS    0x00000800 /* NFS hdr present */
#define XLNID_RXDADV_PKTTYPE_GENEVE 0x00000800 /* GENEVE hdr present */
#define XLNID_RXDADV_PKTTYPE_VXLAN  0x00000800 /* VXLAN hdr present */
#define XLNID_RXDADV_PKTTYPE_TUNNEL 0x00010000 /* Tunnel type */
#define XLNID_RXDADV_PKTTYPE_IPSEC_ESP  0x00001000 /* IPSec ESP */
#define XLNID_RXDADV_PKTTYPE_IPSEC_AH   0x00002000 /* IPSec AH */
#define XLNID_RXDADV_PKTTYPE_LINKSEC    0x00004000 /* LinkSec Encap */
#define XLNID_RXDADV_PKTTYPE_ETQF   0x00008000 /* PKTTYPE is ETQF index */
#define XLNID_RXDADV_PKTTYPE_ETQF_MASK  0x00000070 /* ETQF has 8 indices */
#define XLNID_RXDADV_PKTTYPE_ETQF_SHIFT 4 /* Right-shift 4 bits */

/* Security Processing bit Indication */
#define XLNID_RXDADV_LNKSEC_STATUS_SECP     0x00020000
#define XLNID_RXDADV_LNKSEC_ERROR_NO_SA_MATCH   0x08000000
#define XLNID_RXDADV_LNKSEC_ERROR_REPLAY_ERROR  0x10000000
#define XLNID_RXDADV_LNKSEC_ERROR_BIT_MASK  0x18000000
#define XLNID_RXDADV_LNKSEC_ERROR_BAD_SIG   0x18000000

/* Masks to determine if packets should be dropped due to frame errors */
#define XLNID_RXD_ERR_FRAME_ERR_MASK ( \
				XLNID_RXD_ERR_CE | \
				XLNID_RXD_ERR_LE | \
				XLNID_RXD_ERR_PE | \
				XLNID_RXD_ERR_OSE | \
				XLNID_RXD_ERR_USE)

#define XLNID_RXDADV_ERR_FRAME_ERR_MASK ( \
				XLNID_RXDADV_ERR_CE | \
				XLNID_RXDADV_ERR_LE | \
				XLNID_RXDADV_ERR_PE | \
				XLNID_RXDADV_ERR_OSE | \
				XLNID_RXDADV_ERR_USE)

/* Multicast bit mask */
#define XLNID_MCSTCTRL_MFE  0x4

/* Number of Transmit and Receive Descriptors must be a multiple of 8 */
#define XLNID_REQ_TX_DESCRIPTOR_MULTIPLE    8
#define XLNID_REQ_RX_DESCRIPTOR_MULTIPLE    8
#define XLNID_REQ_TX_BUFFER_GRANULARITY     1024

/* Vlan-specific macros */
#define XLNID_RX_DESC_SPECIAL_VLAN_MASK 0x0FFF /* VLAN ID in lower 12 bits */
#define XLNID_RX_DESC_SPECIAL_PRI_MASK  0xE000 /* Priority in upper 3 bits */
#define XLNID_RX_DESC_SPECIAL_PRI_SHIFT 0x000D /* Priority in upper 3 of 16 */
#define XLNID_TX_DESC_SPECIAL_PRI_SHIFT XLNID_RX_DESC_SPECIAL_PRI_SHIFT

/* Little Endian defines */
#ifndef __le16
#define __le16  u16
#endif
#ifndef __le32
#define __le32  u32
#endif
#ifndef __le64
#define __le64  u64

#endif
#ifndef __be16
/* Big Endian defines */
#define __be16  u16
#define __be32  u32
#define __be64  u64

#endif
enum xlnid_fdir_pballoc_type {
	XLNID_FDIR_PBALLOC_NONE = 0,
	XLNID_FDIR_PBALLOC_64K  = 1,
	XLNID_FDIR_PBALLOC_128K = 2,
	XLNID_FDIR_PBALLOC_256K = 3,
};

/* Flow Director register values */
#define XLNID_FDIRCTRL_PBALLOC_64K      0x00000001
#define XLNID_FDIRCTRL_PBALLOC_128K     0x00000002
#define XLNID_FDIRCTRL_PBALLOC_256K     0x00000003
#define XLNID_FDIRCTRL_INIT_DONE        0x00000008
#define XLNID_FDIRCTRL_PERFECT_MATCH        0x00000010
#define XLNID_FDIRCTRL_REPORT_STATUS        0x00000020
#define XLNID_FDIRCTRL_REPORT_STATUS_ALWAYS 0x00000080
#define XLNID_FDIRCTRL_DROP_Q_SHIFT     8
#define XLNID_FDIRCTRL_DROP_Q_MASK      0x00007F00
#define XLNID_FDIRCTRL_FLEX_SHIFT       16
#define XLNID_FDIRCTRL_DROP_NO_MATCH        0x00008000
#define XLNID_FDIRCTRL_FILTERMODE_SHIFT     21
#define XLNID_FDIRCTRL_FILTERMODE_MACVLAN   0x0001 /* bit 23:21, 001b */
#define XLNID_FDIRCTRL_FILTERMODE_CLOUD     0x0002 /* bit 23:21, 010b */
#define XLNID_FDIRCTRL_SEARCHLIM        0x00800000
#define XLNID_FDIRCTRL_FILTERMODE_MASK      0x00E00000
#define XLNID_FDIRCTRL_MAX_LENGTH_SHIFT     24
#define XLNID_FDIRCTRL_FULL_THRESH_MASK     0xF0000000
#define XLNID_FDIRCTRL_FULL_THRESH_SHIFT    28

#define XLNID_FDIRTCPM_DPORTM_SHIFT     16
#define XLNID_FDIRUDPM_DPORTM_SHIFT     16
#define XLNID_FDIRIP6M_DIPM_SHIFT       16
#define XLNID_FDIRM_VLANID          0x00000001
#define XLNID_FDIRM_VLANP           0x00000002
#define XLNID_FDIRM_POOL            0x00000004
#define XLNID_FDIRM_L4P             0x00000008
#define XLNID_FDIRM_FLEX            0x00000010
#define XLNID_FDIRM_DIPv6           0x00000020
#define XLNID_FDIRM_L3P             0x00000040
#define XLNID_FDIRHASH_BUCKET_VALID_ENABLE    0x8000

#define XLNID_FDIRIP6M_INNER_MAC    0x03F0 /* bit 9:4 */
#define XLNID_FDIRIP6M_TUNNEL_TYPE  0x0800 /* bit 11 */
#define XLNID_FDIRIP6M_TNI_VNI      0xF000 /* bit 15:12 */
#define XLNID_FDIRIP6M_TNI_VNI_24   0x1000 /* bit 12 */
#define XLNID_FDIRIP6M_ALWAYS_MASK  0x040F /* bit 10, 3:0 */

#define XLNID_FDIRFREE_FREE_MASK        0xFFFF
#define XLNID_FDIRFREE_FREE_SHIFT       0
#define XLNID_FDIRFREE_COLL_MASK        0x7FFF0000
#define XLNID_FDIRFREE_COLL_SHIFT       16
#define XLNID_FDIRLEN_MAXLEN_MASK       0x3F
#define XLNID_FDIRLEN_MAXLEN_SHIFT      0
#define XLNID_FDIRLEN_MAXHASH_MASK      0x7FFF0000
#define XLNID_FDIRLEN_MAXHASH_SHIFT     16
#define XLNID_FDIRUSTAT_ADD_MASK        0xFFFF
#define XLNID_FDIRUSTAT_ADD_SHIFT       0
#define XLNID_FDIRUSTAT_REMOVE_MASK     0xFFFF0000
#define XLNID_FDIRUSTAT_REMOVE_SHIFT        16
#define XLNID_FDIRFSTAT_FADD_MASK       0x00FF
#define XLNID_FDIRFSTAT_FADD_SHIFT      0
#define XLNID_FDIRFSTAT_FREMOVE_MASK        0xFF00
#define XLNID_FDIRFSTAT_FREMOVE_SHIFT       8
#define XLNID_FDIRPORT_DESTINATION_SHIFT    16
#define XLNID_FDIRVLAN_FLEX_SHIFT       16
#define XLNID_FDIRHASH_BUCKET_VALID_SHIFT   15
#define XLNID_FDIRHASH_SIG_SW_INDEX_SHIFT   16

#define XLNID_FDIRCMD_CMD_MASK          0x00000003
#define XLNID_FDIRCMD_CMD_ADD_FLOW      0x00000001
#define XLNID_FDIRCMD_CMD_REMOVE_FLOW       0x00000002
#define XLNID_FDIRCMD_CMD_QUERY_REM_FILT    0x00000003
#define XLNID_FDIRCMD_FILTER_VALID      0x00000004
#define XLNID_FDIRCMD_FILTER_UPDATE     0x00000008
#define XLNID_FDIRCMD_IPv6DMATCH        0x00000010
#define XLNID_FDIRCMD_L4TYPE_UDP        0x00000020
#define XLNID_FDIRCMD_L4TYPE_TCP        0x00000040
#define XLNID_FDIRCMD_L4TYPE_SCTP       0x00000060
#define XLNID_FDIRCMD_IPV6          0x00000080
#define XLNID_FDIRCMD_CLEARHT           0x00000100
#define XLNID_FDIRCMD_DROP          0x00000200
#define XLNID_FDIRCMD_INT           0x00000400
#define XLNID_FDIRCMD_LAST          0x00000800
#define XLNID_FDIRCMD_COLLISION         0x00001000
#define XLNID_FDIRCMD_QUEUE_EN          0x00008000
#define XLNID_FDIRCMD_FLOW_TYPE_SHIFT       5
#define XLNID_FDIRCMD_RX_QUEUE_SHIFT        16
#define XLNID_FDIRCMD_TUNNEL_FILTER_SHIFT   23
#define XLNID_FDIRCMD_VT_POOL_SHIFT     24
#define XLNID_FDIR_INIT_DONE_POLL       10
#define XLNID_FDIRCMD_CMD_POLL          10
#define XLNID_FDIRCMD_TUNNEL_FILTER     0x00800000
#define XLNID_FDIR_DROP_QUEUE           127

/* CEM Support */
#define FW_CEM_DRIVER_VERSION_SIZE  39 /* +9 would send 48 bytes to fw */
#define FW_PHY_ACT_DATA_COUNT       4
#define FW_PHY_ACT_UD_2_10G_KR_EEE  (1u << 6)
#define FW_PHY_ACT_UD_2_10G_KX4_EEE (1u << 5)
#define FW_PHY_ACT_UD_2_1G_KX_EEE   (1u << 4)
#define FW_PHY_ACT_UD_2_10G_T_EEE   (1u << 3)
#define FW_PHY_ACT_UD_2_1G_T_EEE    (1u << 2)
#define FW_PHY_ACT_UD_2_100M_TX_EEE (1u << 1)

/* PTP */
#define XLNID_PTP_RX_L2_EN          0x00000020
#define XLNID_PTP_RX_IPV4_EN        0x00000010
#define XLNID_PTP_RX_IPV6_EN        0x00000008
#define XLNID_PTP_TX_L2_EN          0x00000004
#define XLNID_PTP_TX_IPV4_EN        0x00000002
#define XLNID_PTP_TX_IPV6_EN        0x00000001

/* gephy */
#define MII_EEROR_PACKET                0x1c
#define XLNID_PHY_BMCR_ISOLATE          0x00000400
#define XLNID_PHY_BMCR_RESART_AUTONEG   0x00000200
#define XLNID_PHY_BMCR_AUTONEG_ENBALE   0x00001000
#define XLNID_PHY_BMCR_SPEED            0x00002040
#define XLNID_PHY_1000CTRL_1000FULL     0x00000200
#define XLNID_PHY_1000STATUS_IDLE_ERROR 0x000000ff
#define XLNID_PHY_1000STATUS_STATUS     0x00003000


/* Host Interface Command Structures */

#pragma pack(push, 1)

struct xlnid_hic_hdr {
	u8 cmd;
	u8 buf_len;
	union {
		u8 cmd_resv;
		u8 ret_status;
	} cmd_or_resp;
	u8 checksum;
};

struct xlnid_hic_hdr2_req {
	u8 cmd;
	u8 buf_lenh;
	u8 buf_lenl;
	u8 checksum;
};

struct xlnid_hic_hdr2_rsp {
	u8 cmd;
	u8 buf_lenl;
	u8 buf_lenh_status; /* 7-5: high bits of buf_len, 4-0: status */
	u8 checksum;
};

union xlnid_hic_hdr2 {
	struct xlnid_hic_hdr2_req req;
	struct xlnid_hic_hdr2_rsp rsp;
};

struct xlnid_hic_drv_info {
	struct xlnid_hic_hdr hdr;
	u8 port_num;
	u8 ver_sub;
	u8 ver_build;
	u8 ver_min;
	u8 ver_maj;
	u8 pad; /* end spacing to ensure length is mult. of dword */
	u16 pad2; /* end spacing to ensure length is mult. of dword2 */
};

struct xlnid_hic_drv_info2 {
	struct xlnid_hic_hdr hdr;
	u8 port_num;
	u8 ver_sub;
	u8 ver_build;
	u8 ver_min;
	u8 ver_maj;
	char driver_string[FW_CEM_DRIVER_VERSION_SIZE];
};

/* These need to be dword aligned */
struct xlnid_hic_read_shadow_ram {
	union xlnid_hic_hdr2 hdr;
	u32 address;
	u16 length;
	u16 pad2;
	u16 data;
	u16 pad3;
};

struct xlnid_hic_write_shadow_ram {
	union xlnid_hic_hdr2 hdr;
	u32 address;
	u16 length;
	u16 pad2;
	u16 data;
	u16 pad3;
};

struct xlnid_hic_disable_rxen {
	struct xlnid_hic_hdr hdr;
	u8  port_number;
	u8  pad2;
	u16 pad3;
};

struct xlnid_hic_phy_token_req {
	struct xlnid_hic_hdr hdr;
	u8 port_number;
	u8 command_type;
	u16 pad;
};

struct xlnid_hic_internal_phy_req {
	struct xlnid_hic_hdr hdr;
	u8 port_number;
	u8 command_type;
	__be16 address;
	u16 rsv1;
	__be32 write_data;
	u16 pad;
};

struct xlnid_hic_internal_phy_resp {
	struct xlnid_hic_hdr hdr;
	__be32 read_data;
};

struct xlnid_hic_phy_activity_req {
	struct xlnid_hic_hdr hdr;
	u8 port_number;
	u8 pad;
	__le16 activity_id;
	__be32 data[FW_PHY_ACT_DATA_COUNT];
};

struct xlnid_hic_phy_activity_resp {
	struct xlnid_hic_hdr hdr;
	__be32 data[FW_PHY_ACT_DATA_COUNT];
};

#pragma pack(pop)

/* Transmit Descriptor - Legacy */
struct xlnid_legacy_tx_desc {
	u64 buffer_addr; /* Address of the descriptor's data buffer */
	union {
		__le32 data;
		struct {
			__le16 length; /* Data buffer length */
			u8 cso; /* Checksum offset */
			u8 cmd; /* Descriptor control */
		} flags;
	} lower;
	union {
		__le32 data;
		struct {
			u8 status; /* Descriptor status */
			u8 css; /* Checksum start */
			__le16 vlan;
		} fields;
	} upper;
};

/* Transmit Descriptor - Advanced */
union xlnid_adv_tx_desc {
	struct {
		__le64 buffer_addr; /* Address of descriptor's data buf */
		__le32 cmd_type_len;
		__le32 olinfo_status;
	} read;
	struct {
		__le64 rsvd; /* Reserved */
		__le32 nxtseq_seed;
		__le32 status;
	} wb;
};

/* Transmit Descriptor - Lodestar */
union xlnid_lodestar_tx_desc {
	struct {
		__le64  buffer_addr;    /* Address of descriptor's data buf */
		__le32  cmd_type_len;
		__le32  cpu_tag_lo;
		__le32  cpu_tag_hi;
		__le32  rsvd1;
		__le32  rsvd2;
		__le32  idx;
	} read;
	struct {
		__le64  rsvd1;
		__le32  status;
		__le32  rsvd2;
		__le64  rsvd3;
		__le64  rsvd4;
	} wb;
};

/* Receive Descriptor - Legacy */
struct xlnid_legacy_rx_desc {
	__le64 buffer_addr; /* Address of the descriptor's data buffer */
	__le16 length; /* Length of data DMAed into data buffer */
	__le16 csum; /* Packet checksum */
	u8 status;   /* Descriptor status */
	u8 errors;   /* Descriptor Errors */
	__le16 vlan;
};

/* Receive Descriptor - Advanced */
union xlnid_adv_rx_desc {
	struct {
		__le64 pkt_addr; /* Packet buffer address */
		__le64 hdr_addr; /* Header buffer address */
	} read;
	struct {
		struct {
			union {
				__le32 data;
				struct {
					__le16 pkt_info; /* RSS, Pkt type */
					__le16 hdr_info; /* Splithdr, hdrlen */
				} hs_rss;
			} lo_dword;
			union {
				__le32 rss; /* RSS Hash */
				struct {
					__le16 ip_id; /* IP id */
					__le16 csum; /* Packet Checksum */
				} csum_ip;
			} hi_dword;
		} lower;
		struct {
			__le32 status_error; /* ext status/error */
			__le16 length; /* Packet length */
			__le16 vlan; /* VLAN tag */
		} upper;
	} wb;  /* writeback */
};

/* Receive Descriptor - Lodestar */
union xlnid_lodestar_rx_desc {
	struct {
		__le64  pkt_addr;   /* Packet buffer address */
		__le64  rsvd1;
		__le64  rsvd2;
		__le64  rsvd3;
	} read;
	struct {
		__le64  rsvd1;
		struct {
			__le16  length; /* Packet length */
			__le16  status;
		} dword2;
		__le32  cpu_tag_lo;
		__le32  cpu_tag_hi;
		__le32  timestamp_lo;
		__le32  timestamp_hi;
		__le32  rsvd2;
	} wb;   /* writeback */
};

/* Context descriptors */
struct xlnid_adv_tx_context_desc {
	__le32 vlan_macip_lens;
	__le32 seqnum_seed;
	__le32 type_tucmd_mlhl;
	__le32 mss_l4len_idx;
};

/* Context Descriptor - Lodestar */
struct xlnid_lodestar_tx_context_desc {
	__le64  rsvd1;
	__le32  type;
	__le32  rsvd2;
	__le64  rsvd3;
	__le32  rsvd4;
	__le32  idx;
};

#ifdef XLNID_LODESTAR
#define xlnid_rx_desc   xlnid_lodestar_rx_desc
#else
#define xlnid_rx_desc   xlnid_adv_rx_desc
#endif

#ifdef XLNID_LODESTAR
#define xlnid_tx_desc   xlnid_lodestar_tx_desc
#define xlnid_tx_context_desc   xlnid_lodestar_tx_context_desc
#else
#define xlnid_tx_desc   xlnid_adv_tx_desc
#define xlnid_tx_context_desc   xlnid_adv_tx_context_desc
#endif

/* Adv Transmit Descriptor Config Masks */
#define XLNID_ADVTXD_DTALEN_MASK    0x0000FFFF /* Data buf length(bytes) */
#define XLNID_ADVTXD_MAC_LINKSEC    0x00040000 /* Insert LinkSec */
#define XLNID_ADVTXD_MAC_TSTAMP     0x00080000 /* IEEE1588 time stamp */
#define XLNID_ADVTXD_IPSEC_SA_INDEX_MASK 0x000003FF /* IPSec SA index */
#define XLNID_ADVTXD_IPSEC_ESP_LEN_MASK 0x000001FF /* IPSec ESP length */
#define XLNID_ADVTXD_DTYP_MASK      0x00F00000 /* DTYP mask */
#define XLNID_ADVTXD_DTYP_CTXT      0x00200000 /* Adv Context Desc */
#define XLNID_ADVTXD_DTYP_DATA      0x00300000 /* Adv Data Descriptor */
#define XLNID_ADVTXD_DCMD_EOP       XLNID_TXD_CMD_EOP  /* End of Packet */
#define XLNID_ADVTXD_DCMD_IFCS      XLNID_TXD_CMD_IFCS /* Insert FCS */
#define XLNID_ADVTXD_DCMD_RS        XLNID_TXD_CMD_RS /* Report Status */
#define XLNID_ADVTXD_DCMD_DDTYP_ISCSI   0x10000000 /* DDP hdr type or iSCSI */
#define XLNID_ADVTXD_DCMD_DEXT      XLNID_TXD_CMD_DEXT /* Desc ext 1 = Adv */
#define XLNID_ADVTXD_DCMD_VLE       XLNID_TXD_CMD_VLE  /* VLAN pkt enable */
#define XLNID_ADVTXD_DCMD_TSE       0x80000000 /* TCP Seg enable */
#define XLNID_ADVTXD_STAT_DD        XLNID_TXD_STAT_DD  /* Descriptor Done */
#define XLNID_ADVTXD_STAT_SN_CRC    0x00000002 /* NXTSEQ/SEED pres in WB */
#define XLNID_ADVTXD_STAT_RSV       0x0000000C /* STA Reserved */
#define XLNID_ADVTXD_IDX_SHIFT      4 /* Adv desc Index shift */
#define XLNID_ADVTXD_CC         0x00000080 /* Check Context */
#define XLNID_ADVTXD_POPTS_SHIFT    8  /* Adv desc POPTS shift */
#define XLNID_ADVTXD_POPTS_IXSM     (XLNID_TXD_POPTS_IXSM << \
						XLNID_ADVTXD_POPTS_SHIFT)
#define XLNID_ADVTXD_POPTS_TXSM     (XLNID_TXD_POPTS_TXSM << \
						XLNID_ADVTXD_POPTS_SHIFT)
#define XLNID_ADVTXD_POPTS_ISCO_1ST 0x00000000 /* 1st TSO of iSCSI PDU */
#define XLNID_ADVTXD_POPTS_ISCO_MDL 0x00000800 /* Middle TSO of iSCSI PDU */
#define XLNID_ADVTXD_POPTS_ISCO_LAST    0x00001000 /* Last TSO of iSCSI PDU */
/* 1st&Last TSO-full iSCSI PDU */
#define XLNID_ADVTXD_POPTS_ISCO_FULL    0x00001800
#define XLNID_ADVTXD_POPTS_RSV      0x00002000 /* POPTS Reserved */
#define XLNID_ADVTXD_PAYLEN_SHIFT   14 /* Adv desc PAYLEN shift */
#define XLNID_ADVTXD_MACLEN_SHIFT   9  /* Adv ctxt desc mac len shift */
#define XLNID_ADVTXD_VLAN_SHIFT     16  /* Adv ctxt vlan tag shift */
#define XLNID_ADVTXD_TUCMD_IPV4     0x00000400 /* IP Packet Type: 1 = IPv4 */
#define XLNID_ADVTXD_TUCMD_IPV6     0x00000000 /* IP Packet Type: 0 = IPv6 */
#define XLNID_ADVTXD_TUCMD_L4T_UDP  0x00000000 /* L4 Packet TYPE of UDP */
#define XLNID_ADVTXD_TUCMD_L4T_TCP  0x00000800 /* L4 Packet TYPE of TCP */
#define XLNID_ADVTXD_TUCMD_L4T_SCTP 0x00001000 /* L4 Packet TYPE of SCTP */
#define XLNID_ADVTXD_TUCMD_L4T_RSV  0x00001800 /* RSV L4 Packet TYPE */
#define XLNID_ADVTXD_TUCMD_MKRREQ   0x00002000 /* req Markers and CRC */
//#define XLNID_ADVTXD_POPTS_IPSEC  0x00000400 /* IPSec offload request */
#define XLNID_ADVTXD_POPTS_MACSEC   0x00000400 /* Insert MACSec */
#define XLNID_ADVTXD_TUCMD_IPSEC_TYPE_ESP 0x00002000 /* IPSec Type ESP */
#define XLNID_ADVTXD_TUCMD_IPSEC_ENCRYPT_EN 0x00004000/* ESP Encrypt Enable */
#define XLNID_ADVTXT_TUCMD_FCOE     0x00008000 /* FCoE Frame Type */
#define XLNID_ADVTXD_FCOEF_EOF_MASK (0x3 << 10) /* FC EOF index */
#define XLNID_ADVTXD_FCOEF_SOF      ((1 << 2) << 10) /* FC SOF index */
#define XLNID_ADVTXD_FCOEF_PARINC   ((1 << 3) << 10) /* Rel_Off in F_CTL */
#define XLNID_ADVTXD_FCOEF_ORIE     ((1 << 4) << 10) /* Orientation End */
#define XLNID_ADVTXD_FCOEF_ORIS     ((1 << 5) << 10) /* Orientation Start */
#define XLNID_ADVTXD_FCOEF_EOF_N    (0x0 << 10) /* 00: EOFn */
#define XLNID_ADVTXD_FCOEF_EOF_T    (0x1 << 10) /* 01: EOFt */
#define XLNID_ADVTXD_FCOEF_EOF_NI   (0x2 << 10) /* 10: EOFni */
#define XLNID_ADVTXD_FCOEF_EOF_A    (0x3 << 10) /* 11: EOFa */
#define XLNID_ADVTXD_L4LEN_SHIFT    8  /* Adv ctxt L4LEN shift */
#define XLNID_ADVTXD_MSS_SHIFT      16  /* Adv ctxt MSS shift */

#define XLNID_ADVTXD_OUTER_IPLEN    16 /* Adv ctxt OUTERIPLEN shift */
#define XLNID_ADVTXD_TUNNEL_LEN     24 /* Adv ctxt TUNNELLEN shift */
#define XLNID_ADVTXD_TUNNEL_TYPE_SHIFT  16 /* Adv Tx Desc Tunnel Type shift */
#define XLNID_ADVTXD_OUTERIPCS_SHIFT    17 /* Adv Tx Desc OUTERIPCS Shift */
#define XLNID_ADVTXD_TUNNEL_TYPE_NVGRE  1  /* Adv Tx Desc Tunnel Type NVGRE */

#define XLNID_TXD32_DEXT        XLNID_TXD_CMD_DEXT
#define XLNID_TXD32_RS          XLNID_TXD_CMD_RS
#define XLNID_TXD32_EOP         XLNID_TXD_CMD_EOP
#define XLNID_TXD32_DTYP_DATA   XLNID_ADVTXD_DTYP_DATA

/* Autonegotiation advertised speeds */
typedef u32 xlnid_autoneg_advertised;
/* Link speed */
typedef u32 xlnid_link_speed;
#define XLNID_LINK_SPEED_UNKNOWN    0
#define XLNID_LINK_SPEED_10_HALF    0x0001
#define XLNID_LINK_SPEED_10_FULL    0x0002
#define XLNID_LINK_SPEED_100_HALF   0x0004
#define XLNID_LINK_SPEED_100_FULL   0x0008
#define XLNID_LINK_SPEED_1GB_HALF   0x0010
#define XLNID_LINK_SPEED_1GB_FULL   0x0020
#define XLNID_LINK_SPEED_2_5GB_FULL 0x0400
#define XLNID_LINK_SPEED_5GB_FULL   0x0800
#define XLNID_LINK_SPEED_10GB_FULL  0x0080
#define WESTLAKE_LINK_SPEED_AUTONEG (XLNID_LINK_SPEED_10_HALF | \
						XLNID_LINK_SPEED_10_FULL | \
						XLNID_LINK_SPEED_100_HALF | \
						XLNID_LINK_SPEED_100_FULL | \
						XLNID_LINK_SPEED_1GB_FULL)
#define WESTLAKE_LINK_SPEED_RMII    (XLNID_LINK_SPEED_10_HALF | \
						XLNID_LINK_SPEED_10_FULL | \
						XLNID_LINK_SPEED_100_HALF | \
						XLNID_LINK_SPEED_100_FULL)

/* Link mode */
typedef u32 xlnid_link_mode;
#define XLNID_LINK_MODE_GEPHY        0
#define XLNID_LINK_MODE_1000BASEX    0x1

/* Physical layer type */
typedef u64 xlnid_physical_layer;
#define XLNID_PHYSICAL_LAYER_UNKNOWN        0
#define XLNID_PHYSICAL_LAYER_10GBASE_T      0x00001
#define XLNID_PHYSICAL_LAYER_1000BASE_T     0x00002
#define XLNID_PHYSICAL_LAYER_100BASE_TX     0x00004
#define XLNID_PHYSICAL_LAYER_SFP_PLUS_CU    0x00008
#define XLNID_PHYSICAL_LAYER_10GBASE_LR     0x00010
#define XLNID_PHYSICAL_LAYER_10GBASE_LRM    0x00020
#define XLNID_PHYSICAL_LAYER_10GBASE_SR     0x00040
#define XLNID_PHYSICAL_LAYER_10GBASE_KX4    0x00080
#define XLNID_PHYSICAL_LAYER_10GBASE_CX4    0x00100
#define XLNID_PHYSICAL_LAYER_1000BASE_KX    0x00200
#define XLNID_PHYSICAL_LAYER_1000BASE_BX    0x00400
#define XLNID_PHYSICAL_LAYER_10GBASE_KR     0x00800
#define XLNID_PHYSICAL_LAYER_10GBASE_XAUI   0x01000
#define XLNID_PHYSICAL_LAYER_SFP_ACTIVE_DA  0x02000
#define XLNID_PHYSICAL_LAYER_1000BASE_SX    0x04000
#define XLNID_PHYSICAL_LAYER_10BASE_T       0x08000
#define XLNID_PHYSICAL_LAYER_2500BASE_KX    0x10000

/*  Flow Control Data Sheet defined values
		Calculation and defines taken from 802.1bb Annex O
*/

/* BitTimes (BT) conversion */
#define XLNID_BT2KB(BT)     ((BT + (8 * 1024 - 1)) / (8 * 1024))
#define XLNID_B2BT(BT)      (BT * 8)

/* Calculate Delay to respond to PFC */
#define XLNID_PFC_D 672

/* Calculate Cable Delay */
#define XLNID_CABLE_DC  5556 /* Delay Copper */
#define XLNID_CABLE_DO  5000 /* Delay Optical */

/* Calculate Delay incurred from higher layer */
#define XLNID_HD    6144

/* Calculate PCI Bus delay for low thresholds */
#define XLNID_PCI_DELAY 10000

/* Calculate delay value in bit times */
#define XLNID_DV(_max_frame_link, _max_frame_tc) \
			((36 * \
				(XLNID_B2BT(_max_frame_link) + \
				XLNID_PFC_D + \
				(2 * XLNID_CABLE_DC) + \
				(2 * XLNID_ID) + \
				XLNID_HD) / 25 + 1) + \
				2 * XLNID_B2BT(_max_frame_tc))

/* Software ATR hash keys */
#define XLNID_ATR_BUCKET_HASH_KEY   0x3DAD14E2
#define XLNID_ATR_SIGNATURE_HASH_KEY    0x174D3614

/* Software ATR input stream values and masks */
#define XLNID_ATR_HASH_MASK     0x7fff
#define XLNID_ATR_L4TYPE_MASK       0x3
#define XLNID_ATR_L4TYPE_UDP        0x1
#define XLNID_ATR_L4TYPE_TCP        0x2
#define XLNID_ATR_L4TYPE_SCTP       0x3
#define XLNID_ATR_L4TYPE_IPV6_MASK  0x4
#define XLNID_ATR_L4TYPE_TUNNEL_MASK    0x10
enum xlnid_atr_flow_type {
	XLNID_ATR_FLOW_TYPE_IPV4    = 0x0,
	XLNID_ATR_FLOW_TYPE_UDPV4   = 0x1,
	XLNID_ATR_FLOW_TYPE_TCPV4   = 0x2,
	XLNID_ATR_FLOW_TYPE_SCTPV4  = 0x3,
	XLNID_ATR_FLOW_TYPE_IPV6    = 0x4,
	XLNID_ATR_FLOW_TYPE_UDPV6   = 0x5,
	XLNID_ATR_FLOW_TYPE_TCPV6   = 0x6,
	XLNID_ATR_FLOW_TYPE_SCTPV6  = 0x7,
	XLNID_ATR_FLOW_TYPE_TUNNELED_IPV4   = 0x10,
	XLNID_ATR_FLOW_TYPE_TUNNELED_UDPV4  = 0x11,
	XLNID_ATR_FLOW_TYPE_TUNNELED_TCPV4  = 0x12,
	XLNID_ATR_FLOW_TYPE_TUNNELED_SCTPV4 = 0x13,
	XLNID_ATR_FLOW_TYPE_TUNNELED_IPV6   = 0x14,
	XLNID_ATR_FLOW_TYPE_TUNNELED_UDPV6  = 0x15,
	XLNID_ATR_FLOW_TYPE_TUNNELED_TCPV6  = 0x16,
	XLNID_ATR_FLOW_TYPE_TUNNELED_SCTPV6 = 0x17,
};

/* Flow Director ATR input struct. */
union xlnid_atr_input {
	/*
		Byte layout in order, all values with MSB first:

		vm_pool  - 1 byte
		flow_type    - 1 byte
		vlan_id  - 2 bytes
		src_ip   - 16 bytes
		inner_mac    - 6 bytes
		cloud_mode   - 2 bytes
		tni_vni  - 4 bytes
		dst_ip   - 16 bytes
		src_port - 2 bytes
		dst_port - 2 bytes
		flex_bytes   - 2 bytes
		bkt_hash - 2 bytes
	*/
	struct {
		u8 vm_pool;
		u8 flow_type;
		__be16 vlan_id;
		__be32 dst_ip[4];
		__be32 src_ip[4];
		u8 inner_mac[6];
		__be16 tunnel_type;
		__be32 tni_vni;
		__be16 src_port;
		__be16 dst_port;
		__be16 flex_bytes;
		__be16 bkt_hash;
	} formatted;
	__be32 dword_stream[14];
};

/* Flow Director compressed ATR hash input struct */
union xlnid_atr_hash_dword {
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

/*
		Unavailable: The FCoE Boot Option ROM is not present in the flash.
		Disabled: Present; boot order is not set for any targets on the port.
		Enabled: Present; boot order is set for at least one target on the port.
*/
enum xlnid_fcoe_boot_status {
	xlnid_fcoe_bootstatus_disabled = 0,
	xlnid_fcoe_bootstatus_enabled = 1,
	xlnid_fcoe_bootstatus_unavailable = 0xFFFF
};

enum xlnid_eeprom_type {
	xlnid_eeprom_uninitialized = 0,
	xlnid_eeprom_spi,
	xlnid_flash,
	xlnid_eeprom_i2c /* No NVM support */
};

enum xlnid_mac_type {
	xlnid_mac_unknown = 0,
	xlnid_mac_SKYLAKE,
	xlnid_mac_WESTLAKE,
	xlnid_mac_lk40,
	xlnid_mac_lk10,
	xlnid_mac_lodestar,
	xlnid_mac_eastlake,
	xlnid_mac_westlakeplus,
	xlnid_num_macs
};

enum xlnid_phy_type {
	xlnid_phy_unknown = 0,
	xlnid_phy_none,
	xlnid_phy_tn,
	xlnid_phy_aq,
	xlnid_phy_ext_1g_t,
	xlnid_phy_cu_unknown,
	xlnid_phy_qt,
	xlnid_phy_xaui,
	xlnid_phy_nl,
	xlnid_phy_sfp_passive_tyco,
	xlnid_phy_sfp_passive_unknown,
	xlnid_phy_sfp_active_unknown,
	xlnid_phy_sfp_avago,
	xlnid_phy_sfp_ftl,
	xlnid_phy_sfp_ftl_active,
	xlnid_phy_sfp_unknown,
	xlnid_phy_qsfp_passive_unknown,
	xlnid_phy_qsfp_active_unknown,
	xlnid_phy_qsfp_unknown,
	xlnid_phy_sfp_unsupported, /*Enforce bit set with unsupported module*/
	xlnid_phy_sgmii,
	xlnid_phy_fw,
	xlnid_phy_generic
};

/*
		SFP+ module type IDs:

		ID    Module Type
		=============
		0 SFP_DA_CU
		1 SFP_SR
		2 SFP_LR
		3 SFP_DA_CU_CORE0
		4 SFP_DA_CU_CORE1
		5 SFP_SR/LR_CORE0
		6 SFP_SR/LR_CORE1
*/
enum xlnid_sfp_type {
	xlnid_sfp_type_da_cu = 0,
	xlnid_sfp_type_sr = 1,
	xlnid_sfp_type_lr = 2,
	xlnid_sfp_type_da_cu_core0 = 3,
	xlnid_sfp_type_da_cu_core1 = 4,
	xlnid_sfp_type_srlr_core0 = 5,
	xlnid_sfp_type_srlr_core1 = 6,
	xlnid_sfp_type_da_act_lmt_core0 = 7,
	xlnid_sfp_type_da_act_lmt_core1 = 8,
	xlnid_sfp_type_1g_cu_core0 = 9,
	xlnid_sfp_type_1g_cu_core1 = 10,
	xlnid_sfp_type_1g_sx_core0 = 11,
	xlnid_sfp_type_1g_sx_core1 = 12,
	xlnid_sfp_type_1g_lx_core0 = 13,
	xlnid_sfp_type_1g_lx_core1 = 14,
	xlnid_sfp_type_not_present = 0xFFFE,
	xlnid_sfp_type_unknown = 0xFFFF
};

enum xlnid_media_type {
	xlnid_media_type_unknown = 0,
	xlnid_media_type_copper,
	xlnid_media_type_fiber,
	xlnid_media_type_sgmii_mac,
	xlnid_media_type_sgmii_phy,
	xlnid_media_type_rmii_mac,
	xlnid_media_type_rmii_phy,
	xlnid_media_type_virtual
};

/* Flow Control Settings */
enum xlnid_fc_mode {
	xlnid_fc_none = 0,
	xlnid_fc_rx_pause,
	xlnid_fc_tx_pause,
	xlnid_fc_full,
	xlnid_fc_default
};

/* Smart Speed Settings */
#define XLNID_SMARTSPEED_MAX_RETRIES    3
enum xlnid_smart_speed {
	xlnid_smart_speed_auto = 0,
	xlnid_smart_speed_on,
	xlnid_smart_speed_off
};

/* PCI bus types */
enum xlnid_bus_type {
	xlnid_bus_type_unknown = 0,
	xlnid_bus_type_pci,
	xlnid_bus_type_pcix,
	xlnid_bus_type_pci_express,
	xlnid_bus_type_internal,
	xlnid_bus_type_reserved
};

/* PCI bus speeds */
enum xlnid_bus_speed {
	xlnid_bus_speed_unknown = 0,
	xlnid_bus_speed_33  = 33,
	xlnid_bus_speed_66  = 66,
	xlnid_bus_speed_100 = 100,
	xlnid_bus_speed_120 = 120,
	xlnid_bus_speed_133 = 133,
	xlnid_bus_speed_2500    = 2500,
	xlnid_bus_speed_5000    = 5000,
	xlnid_bus_speed_8000    = 8000,
	xlnid_bus_speed_16000   = 16000,
	xlnid_bus_speed_reserved
};

/* PCI bus widths */
enum xlnid_bus_width {
	xlnid_bus_width_unknown = 0,
	xlnid_bus_width_pcie_x1 = 1,
	xlnid_bus_width_pcie_x2 = 2,
	xlnid_bus_width_pcie_x4 = 4,
	xlnid_bus_width_pcie_x8 = 8,
	xlnid_bus_width_32  = 32,
	xlnid_bus_width_64  = 64,
	xlnid_bus_width_reserved
};

struct xlnid_addr_filter_info {
	u32 num_mc_addrs;
	u32 rar_used_count;
	u32 mta_in_use;
	u32 overflow_promisc;
	bool user_set_promisc;
};

/* Bus parameters */
struct xlnid_bus_info {
	enum xlnid_bus_speed speed;
	enum xlnid_bus_width width;
	enum xlnid_bus_type type;

	u16 func;
	u8 bus_id;
	u8 lan_id;
	u16 instance_id;
};

/* Flow control parameters */
struct xlnid_fc_info {
	u32 high_water[XLNID_DCB_MAX_TRAFFIC_CLASS]; /* Flow Ctrl High-water */
	u32 low_water[XLNID_DCB_MAX_TRAFFIC_CLASS]; /* Flow Ctrl Low-water */
	u16 pause_time; /* Flow Control Pause timer */
	bool send_xon; /* Flow control send XON */
	bool strict_ieee; /* Strict IEEE mode */
	bool disable_fc_autoneg; /* Do not autonegotiate FC */
	bool fc_was_autonegged; /* Is current_mode the result of autonegging? */
	enum xlnid_fc_mode current_mode; /* FC mode in effect */
	enum xlnid_fc_mode requested_mode; /* FC mode requested by caller */
};

/* Statistics counters collected by the MAC */
struct xlnid_hw_stats {
	u64 crcerrs;
	u64 illerrc;
	u64 errbc;
	u64 mspdc;
	u64 mpctotal;
	u64 mpc[8];
	u64 mlfc;
	u64 mrfc;
	u64 rlec;
	u64 lxontxc;
	u64 lxonrxc;
	u64 lxofftxc;
	u64 lxoffrxc;
	u64 pxontxc[8];
	u64 pxonrxc[8];
	u64 pxofftxc[8];
	u64 pxoffrxc[8];
	u64 prc64;
	u64 prc127;
	u64 prc255;
	u64 prc511;
	u64 prc1023;
	u64 prc1522;
	u64 gprc;
	u64 bprc;
	u64 mprc;
	u64 gptc;
	u64 gorc;
	u64 gotc;
	u64 rnbc[8];
	u64 ruc;
	u64 rfc;
	u64 roc;
	u64 rjc;
	u64 mngprc;
	u64 mngpdc;
	u64 mngptc;
	u64 tor;
	u64 tpr;
	u64 tpt;
	u64 ptc64;
	u64 ptc127;
	u64 ptc255;
	u64 ptc511;
	u64 ptc1023;
	u64 ptc1522;
	u64 mptc;
	u64 bptc;
	u64 xec;
	u64 qprc[16];
	u64 qptc[16];
	u64 qbrc[16];
	u64 qbtc[16];
	u64 qprdc[16];
	u64 pxon2offc[8];
	u64 fdirustat_add;
	u64 fdirustat_remove;
	u64 fdirfstat_fadd;
	u64 fdirfstat_fremove;
	u64 fdirmatch;
	u64 fdirmiss;
	u64 fccrc;
	u64 fclast;
	u64 fcoerpdc;
	u64 fcoeprc;
	u64 fcoeptc;
	u64 fcoedwrc;
	u64 fcoedwtc;
	u64 fcoe_noddp;
	u64 fcoe_noddp_ext_buff;
	u64 ldpcec;
	u64 pcrc8ec;
	u64 b2ospc;
	u64 b2ogprc;
	u64 o2bgptc;
	u64 o2bspc;
};

/* forward declaration */
struct xlnid_hw;

/* iterator type for walking multicast address lists */
typedef u8 *(*xlnid_mc_addr_itr)(struct xlnid_hw *hw, u8 **mc_addr_ptr,
									u32 *vmdq);

/* Function pointer table */
struct xlnid_eeprom_operations {
	s32(*init_params)(struct xlnid_hw *);
	s32(*read)(struct xlnid_hw *, u16, u8 *);
	s32(*read_buffer)(struct xlnid_hw *, u16, u16, u8 *);
	s32(*write)(struct xlnid_hw *, u16, u16);
	s32(*write_buffer)(struct xlnid_hw *, u16, u16, u8 *);
	s32(*validate_checksum)(struct xlnid_hw *, u16 *);
	s32(*update_checksum)(struct xlnid_hw *);
	s32(*calc_checksum)(struct xlnid_hw *);
};

struct xlnid_mac_operations {
	s32(*init_hw)(struct xlnid_hw *);
	s32(*reset_hw)(struct xlnid_hw *);
	s32(*start_hw)(struct xlnid_hw *);
	s32(*clear_hw_cntrs)(struct xlnid_hw *);
	enum xlnid_media_type(*get_media_type)(struct xlnid_hw *);
	u64(*get_supported_physical_layer)(struct xlnid_hw *);
	s32(*get_mac_addr)(struct xlnid_hw *, u8 *);
	s32(*get_san_mac_addr)(struct xlnid_hw *, u8 *);
	s32(*get_device_caps)(struct xlnid_hw *, u16 *);
	s32(*get_wwn_prefix)(struct xlnid_hw *, u16 *, u16 *);
	s32(*get_fcoe_boot_status)(struct xlnid_hw *, u16 *);
	s32(*stop_adapter)(struct xlnid_hw *);
	s32(*get_bus_info)(struct xlnid_hw *);
	void (*set_lan_id)(struct xlnid_hw *);
	s32(*read_analog_reg8)(struct xlnid_hw *, u32, u8 *);
	s32(*write_analog_reg8)(struct xlnid_hw *, u32, u8);
	s32(*setup_sfp)(struct xlnid_hw *);
	s32(*enable_rx_dma)(struct xlnid_hw *, u32);
	s32(*disable_sec_rx_path)(struct xlnid_hw *);
	s32(*enable_sec_rx_path)(struct xlnid_hw *);
	s32(*acquire_swfw_sync)(struct xlnid_hw *, u32);
	void (*release_swfw_sync)(struct xlnid_hw *, u32);
	void (*init_swfw_sync)(struct xlnid_hw *);
	s32(*prot_autoc_read)(struct xlnid_hw *, bool *, u32 *);
	s32(*prot_autoc_write)(struct xlnid_hw *, u32, bool);

	/* Link */
	void (*disable_tx_laser)(struct xlnid_hw *);
	void (*enable_tx_laser)(struct xlnid_hw *);
	void (*flap_tx_laser)(struct xlnid_hw *);
	s32(*setup_link)(struct xlnid_hw *, xlnid_link_speed, bool);
	s32(*setup_mac_link)(struct xlnid_hw *, xlnid_link_speed, bool);
	s32(*check_link)(struct xlnid_hw *, xlnid_link_speed *, bool *, bool);
	s32(*get_link_capabilities)(struct xlnid_hw *, xlnid_link_speed *,
								bool *);
	void (*set_rate_select_speed)(struct xlnid_hw *, xlnid_link_speed);

	/* Packet Buffer manipulation */
	void (*setup_rxpba)(struct xlnid_hw *, int, u32, int);

	/* LED */
	s32(*led_on)(struct xlnid_hw *, u32);
	s32(*led_off)(struct xlnid_hw *, u32);
	s32(*blink_led_start)(struct xlnid_hw *, u32);
	s32(*blink_led_stop)(struct xlnid_hw *, u32);
	s32(*init_led_link_act)(struct xlnid_hw *);

	/* RAR, Multicast, VLAN */
	s32(*set_rar)(struct xlnid_hw *, u32, u8 *, u32, u32);
	s32(*set_uc_addr)(struct xlnid_hw *, u32, u8 *);
	s32(*clear_rar)(struct xlnid_hw *, u32);
	s32(*insert_mac_addr)(struct xlnid_hw *, u8 *, u32);
	s32(*set_vmdq)(struct xlnid_hw *, u32, u32);
	s32(*set_vmdq_san_mac)(struct xlnid_hw *, u32);
	s32(*clear_vmdq)(struct xlnid_hw *, u32, u32);
	s32(*init_rx_addrs)(struct xlnid_hw *);
	s32(*update_uc_addr_list)(struct xlnid_hw *, u8 *, u32,
								xlnid_mc_addr_itr);
	s32(*update_mc_addr_list)(struct xlnid_hw *, u8 *, u32,
								xlnid_mc_addr_itr, bool clear);
	s32(*enable_mc)(struct xlnid_hw *);
	s32(*disable_mc)(struct xlnid_hw *);
	s32(*clear_vfta)(struct xlnid_hw *);
	s32(*set_vfta)(struct xlnid_hw *, u32, u32, bool, bool);
	s32(*set_vlvf)(struct xlnid_hw *, u32, u32, bool, u32 *, u32,
					bool);
	s32(*init_uta_tables)(struct xlnid_hw *);
	void (*set_mac_anti_spoofing)(struct xlnid_hw *, bool, int);
	void (*set_vlan_anti_spoofing)(struct xlnid_hw *, bool, int);

	/* Flow Control */
	s32(*fc_enable)(struct xlnid_hw *);
	s32(*setup_fc)(struct xlnid_hw *);
	void (*fc_autoneg)(struct xlnid_hw *);

	/* Manageability interface */
	s32(*set_fw_drv_ver)(struct xlnid_hw *, u8, u8, u8, u8, u16,
							const char *);
	s32(*get_thermal_sensor_data)(struct xlnid_hw *);
	s32(*init_thermal_sensor_thresh)(struct xlnid_hw *hw);
	void (*get_rtrup2tc)(struct xlnid_hw *hw, u8 *map);
	void (*disable_rx)(struct xlnid_hw *hw);
	void (*enable_rx)(struct xlnid_hw *hw);
	void (*set_source_address_pruning)(struct xlnid_hw *, bool,
										unsigned int);
	void (*set_ethertype_anti_spoofing)(struct xlnid_hw *, bool, int);
	s32(*dmac_update_tcs)(struct xlnid_hw *hw);
	s32(*dmac_config_tcs)(struct xlnid_hw *hw);
	s32(*dmac_config)(struct xlnid_hw *hw);
	s32(*setup_eee)(struct xlnid_hw *hw, bool enable_eee);
	s32(*read_iosf_sb_reg)(struct xlnid_hw *, u32, u32, u32 *);
	s32(*write_iosf_sb_reg)(struct xlnid_hw *, u32, u32, u32);
	void (*disable_mdd)(struct xlnid_hw *hw);
	void (*enable_mdd)(struct xlnid_hw *hw);
	void (*mdd_event)(struct xlnid_hw *hw, u32 *vf_bitmap);
	void (*restore_mdd_vf)(struct xlnid_hw *hw, u32 vf);
	bool (*fw_recovery_mode)(struct xlnid_hw *hw);
};

struct xlnid_phy_operations {
	s32(*identify)(struct xlnid_hw *);
	s32(*identify_sfp)(struct xlnid_hw *);
	s32(*init)(struct xlnid_hw *);
	s32(*reset)(struct xlnid_hw *);
	s32(*read_reg)(struct xlnid_hw *, u32, u32, u16 *);
	s32(*write_reg)(struct xlnid_hw *, u32, u32, u16);
	s32(*read_reg_mdi)(struct xlnid_hw *, u32, u32, u16 *);
	s32(*write_reg_mdi)(struct xlnid_hw *, u32, u32, u16);
	s32(*setup_link)(struct xlnid_hw *);
	s32(*setup_internal_link)(struct xlnid_hw *);
	s32(*setup_link_speed)(struct xlnid_hw *, xlnid_link_speed, bool);
	s32(*check_link)(struct xlnid_hw *, xlnid_link_speed *, bool *);
	s32(*get_firmware_version)(struct xlnid_hw *, u16 *);
	s32(*read_i2c_byte)(struct xlnid_hw *, u8, u8, u8 *);
	s32(*write_i2c_byte)(struct xlnid_hw *, u8, u8, u8);
	s32(*read_i2c_sff8472)(struct xlnid_hw *, u8, u8 *);
	s32(*read_i2c_eeprom)(struct xlnid_hw *, u8, u8 *);
	s32(*write_i2c_eeprom)(struct xlnid_hw *, u8, u8);
	void (*i2c_bus_clear)(struct xlnid_hw *);
	s32(*check_overtemp)(struct xlnid_hw *);
	s32(*set_phy_power)(struct xlnid_hw *, bool on);
	s32(*enter_lplu)(struct xlnid_hw *);
	s32(*handle_lasi)(struct xlnid_hw *hw);
	s32(*read_i2c_byte_unlocked)(struct xlnid_hw *, u8 offset, u8 addr,
									u8 *value);
	s32(*write_i2c_byte_unlocked)(struct xlnid_hw *, u8 offset, u8 addr,
									u8 value);
};

struct xlnid_link_operations {
	s32(*read_link)(struct xlnid_hw *, u8 addr, u16 reg, u16 *val);
	s32(*read_link_unlocked)(struct xlnid_hw *, u8 addr, u16 reg,
								u16 *val);
	s32(*write_link)(struct xlnid_hw *, u8 addr, u16 reg, u16 val);
	s32(*write_link_unlocked)(struct xlnid_hw *, u8 addr, u16 reg,
								u16 val);
};

struct xlnid_link_info {
	struct xlnid_link_operations ops;
	u8 addr;
};

struct xlnid_eeprom_info {
	struct xlnid_eeprom_operations ops;
	enum xlnid_eeprom_type type;
	u32 semaphore_delay;
	u16 word_size;
	u16 address_bits;
	u16 word_page_size;
	u16 ctrl_word_3;
};

#define XLNID_FLAGS_DOUBLE_RESET_REQUIRED   0x01
struct xlnid_mac_info {
	struct xlnid_mac_operations ops;
	enum xlnid_mac_type type;
	u8 addr[XLNID_ETH_LENGTH_OF_ADDRESS];
	u8 perm_addr[XLNID_ETH_LENGTH_OF_ADDRESS];
	u8 san_addr[XLNID_ETH_LENGTH_OF_ADDRESS];
	/* prefix for World Wide Node Name (WWNN) */
	u16 wwnn_prefix;
	/* prefix for World Wide Port Name (WWPN) */
	u16 wwpn_prefix;
#define XLNID_MAX_MTA           128
	u32 mta_shadow[XLNID_MAX_MTA];
	s32 mc_filter_type;
	u32 mcft_size;
	u32 vft_size;
	u32 num_rar_entries;
	u32 rar_highwater;
	u32 rx_pb_size;
	u32 max_tx_queues;
	u32 max_rx_queues;
	u32 orig_autoc;
	u8  san_mac_rar_index;
	bool get_link_status;
	u32 orig_autoc2;
	u16 max_msix_vectors;
	bool arc_subsystem_valid;
	bool orig_link_settings_stored;
	bool autotry_restart;
	u8 flags;
	struct xlnid_thermal_sensor_data  thermal_sensor_data;
	bool thermal_sensor_enabled;
	struct xlnid_dmac_config dmac_config;
	bool set_lben;
	u32  max_link_up_time;
	u8   led_link_act;
};

struct xlnid_phy_info {
	struct xlnid_phy_operations ops;
	enum xlnid_phy_type type;
	u32 addr;
	u32 id;
	enum xlnid_sfp_type sfp_type;
	bool sfp_setup_needed;
	u32 revision;
	enum xlnid_media_type media_type;
	u32 phy_semaphore_mask;
	bool reset_disable;
	xlnid_autoneg_advertised autoneg_advertised;
	xlnid_link_speed speeds_supported;
	xlnid_link_speed eee_speeds_supported;
	xlnid_link_speed eee_speeds_advertised;
	enum xlnid_smart_speed smart_speed;
	bool smart_speed_active;
	bool multispeed_fiber;
	bool reset_if_overtemp;
	bool qsfp_shared_i2c_bus;
	u32 nw_mng_if_sel;
	bool reautoneg;
};

//#include "xlnid_mbx.h"

struct xlnid_mbx_operations {
	void (*init_params)(struct xlnid_hw *hw);
	s32(*read)(struct xlnid_hw *, u32 *, u16,  u16);
	s32(*write)(struct xlnid_hw *, u32 *, u16, u16);
	s32(*read_posted)(struct xlnid_hw *, u32 *, u16,  u16);
	s32(*write_posted)(struct xlnid_hw *, u32 *, u16, u16);
	s32(*check_for_msg)(struct xlnid_hw *, u16);
	s32(*check_for_ack)(struct xlnid_hw *, u16);
	s32(*check_for_rst)(struct xlnid_hw *, u16);
};

struct xlnid_mbx_stats {
	u32 msgs_tx;
	u32 msgs_rx;

	u32 acks;
	u32 reqs;
	u32 rsts;
};

struct xlnid_mbx_info {
	struct xlnid_mbx_operations ops;
	struct xlnid_mbx_stats stats;
	u32 timeout;
	u32 usec_delay;
	u32 v2p_mailbox;
	u16 size;
};

struct xlnid_hw {
	u8 IOMEM *hw_addr;
	void *back;
	struct xlnid_mac_info mac;
	struct xlnid_addr_filter_info addr_ctrl;
	struct xlnid_fc_info fc;
	struct xlnid_phy_info phy;
	struct xlnid_link_info link;
	struct xlnid_eeprom_info eeprom;
	struct xlnid_bus_info bus;
	struct xlnid_mbx_info mbx;
	u16 device_id;
	u16 vendor_id;
	u16 subsystem_device_id;
	u16 subsystem_vendor_id;
	u8 revision_id;
	bool adapter_stopped;
	int api_version;
	bool multifunction;
	bool allow_unsupported_sfp;
	bool wol_enabled;
	bool need_crosstalk_fix;
	bool ptsw_enable;
	bool link_mode; /* false:gephy first true:fiber first*/
	bool ptsw_backup; /* true: port0 link down  port1 linkup */
	bool macsec_enable;
	spinlock_t  indirect_lock;
	u16 first_tag;
	u16 second_tag;
	bool is_double_vlan;
	bool is_outer_vlan_processing;
};

#define xlnid_call_func(hw, func, params, error) \
		((func != NULL) ? func params : error)

/* Error Codes */
#define XLNID_SUCCESS               0
#define XLNID_ERR_EEPROM            -1
#define XLNID_ERR_EEPROM_CHECKSUM       -2
#define XLNID_ERR_PHY               -3
#define XLNID_ERR_CONFIG            -4
#define XLNID_ERR_PARAM             -5
#define XLNID_ERR_MAC_TYPE          -6
#define XLNID_ERR_UNKNOWN_PHY           -7
#define XLNID_ERR_LINK_SETUP            -8
#define XLNID_ERR_ADAPTER_STOPPED       -9
#define XLNID_ERR_INVALID_MAC_ADDR      -10
#define XLNID_ERR_DEVICE_NOT_SUPPORTED      -11
#define XLNID_ERR_PRIMARY_REQUESTS_PENDING  -12
#define XLNID_ERR_INVALID_LINK_SETTINGS     -13
#define XLNID_ERR_AUTONEG_NOT_COMPLETE      -14
#define XLNID_ERR_RESET_FAILED          -15
#define XLNID_ERR_SWFW_SYNC         -16
#define XLNID_ERR_PHY_ADDR_INVALID      -17
#define XLNID_ERR_I2C               -18
#define XLNID_ERR_SFP_NOT_SUPPORTED     -19
#define XLNID_ERR_SFP_NOT_PRESENT       -20
#define XLNID_ERR_SFP_NO_INIT_SEQ_PRESENT   -21
#define XLNID_ERR_NO_SAN_ADDR_PTR       -22
#define XLNID_ERR_FDIR_REINIT_FAILED        -23
#define XLNID_ERR_EEPROM_VERSION        -24
#define XLNID_ERR_NO_SPACE          -25
#define XLNID_ERR_OVERTEMP          -26
#define XLNID_ERR_FC_NOT_NEGOTIATED     -27
#define XLNID_ERR_FC_NOT_SUPPORTED      -28
#define XLNID_ERR_SFP_SETUP_NOT_COMPLETE    -30
#define XLNID_ERR_PBA_SECTION           -31
#define XLNID_ERR_INVALID_ARGUMENT      -32
#define XLNID_ERR_HOST_INTERFACE_COMMAND    -33
#define XLNID_ERR_OUT_OF_MEM            -34
#define XLNID_ERR_FEATURE_NOT_SUPPORTED     -36
#define XLNID_ERR_EEPROM_PROTECTED_REGION   -37
#define XLNID_ERR_FDIR_CMD_INCOMPLETE       -38
#define XLNID_ERR_FW_RESP_INVALID       -39
#define XLNID_ERR_TOKEN_RETRY           -40
#define XLNID_ERR_TIMEOUT               -41

#define XLNID_NOT_IMPLEMENTED           0x7FFFFFFF

#define XLNID_FUSES0_GROUP(_i)      (0x11158 + ((_i) * 4))
#define XLNID_FUSES0_300MHZ     (1 << 5)
#define XLNID_FUSES0_REV_MASK       (3 << 6)

#define XLNID_KRM_PORT_CAR_GEN_CTRL(P)  ((P) ? 0x8010 : 0x4010)
#define XLNID_KRM_LINK_S1(P)        ((P) ? 0x8200 : 0x4200)
#define XLNID_KRM_LINK_CTRL_1(P)    ((P) ? 0x820C : 0x420C)
#define XLNID_KRM_AN_CNTL_1(P)      ((P) ? 0x822C : 0x422C)
#define XLNID_KRM_AN_CNTL_4(P)      ((P) ? 0x8238 : 0x4238)
#define XLNID_KRM_AN_CNTL_8(P)      ((P) ? 0x8248 : 0x4248)
#define XLNID_KRM_PCS_KX_AN(P)      ((P) ? 0x9918 : 0x5918)
#define XLNID_KRM_PCS_KX_AN_LP(P)   ((P) ? 0x991C : 0x591C)
#define XLNID_KRM_SGMII_CTRL(P)     ((P) ? 0x82A0 : 0x42A0)
#define XLNID_KRM_LP_BASE_PAGE_HIGH(P)  ((P) ? 0x836C : 0x436C)
#define XLNID_KRM_DSP_TXFFE_STATE_4(P)  ((P) ? 0x8634 : 0x4634)
#define XLNID_KRM_DSP_TXFFE_STATE_5(P)  ((P) ? 0x8638 : 0x4638)
#define XLNID_KRM_RX_TRN_LINKUP_CTRL(P) ((P) ? 0x8B00 : 0x4B00)
#define XLNID_KRM_PMD_DFX_BURNIN(P) ((P) ? 0x8E00 : 0x4E00)
#define XLNID_KRM_PMD_FLX_MASK_ST20(P)  ((P) ? 0x9054 : 0x5054)
#define XLNID_KRM_TX_COEFF_CTRL_1(P)    ((P) ? 0x9520 : 0x5520)
#define XLNID_KRM_RX_ANA_CTL(P)     ((P) ? 0x9A00 : 0x5A00)

#define XLNID_KRM_PMD_FLX_MASK_ST20_SFI_10G_DA      ~(0x3 << 20)
#define XLNID_KRM_PMD_FLX_MASK_ST20_SFI_10G_SR      (1u << 20)
#define XLNID_KRM_PMD_FLX_MASK_ST20_SFI_10G_LR      (0x2 << 20)
#define XLNID_KRM_PMD_FLX_MASK_ST20_SGMII_EN        (1u << 25)
#define XLNID_KRM_PMD_FLX_MASK_ST20_AN37_EN     (1u << 26)
#define XLNID_KRM_PMD_FLX_MASK_ST20_AN_EN       (1u << 27)
#define XLNID_KRM_PMD_FLX_MASK_ST20_SPEED_10M       ~(0x7 << 28)
#define XLNID_KRM_PMD_FLX_MASK_ST20_SPEED_100M      (1u << 28)
#define XLNID_KRM_PMD_FLX_MASK_ST20_SPEED_1G        (0x2 << 28)
#define XLNID_KRM_PMD_FLX_MASK_ST20_SPEED_10G       (0x3 << 28)
#define XLNID_KRM_PMD_FLX_MASK_ST20_SPEED_AN        (0x4 << 28)
#define XLNID_KRM_PMD_FLX_MASK_ST20_SPEED_2_5G      (0x7 << 28)
#define XLNID_KRM_PMD_FLX_MASK_ST20_SPEED_MASK      (0x7 << 28)
#define XLNID_KRM_PMD_FLX_MASK_ST20_FW_AN_RESTART   (1u << 31)

#define XLNID_KRM_PORT_CAR_GEN_CTRL_NELB_32B        (1 << 9)
#define XLNID_KRM_PORT_CAR_GEN_CTRL_NELB_KRPCS      (1 << 11)

#define XLNID_KRM_LINK_CTRL_1_TETH_FORCE_SPEED_MASK (0x7 << 8)
#define XLNID_KRM_LINK_CTRL_1_TETH_FORCE_SPEED_1G   (2 << 8)
#define XLNID_KRM_LINK_CTRL_1_TETH_FORCE_SPEED_10G  (4 << 8)
#define XLNID_KRM_LINK_CTRL_1_TETH_AN_SGMII_EN      (1 << 12)
#define XLNID_KRM_LINK_CTRL_1_TETH_AN_CLAUSE_37_EN  (1 << 13)
#define XLNID_KRM_LINK_CTRL_1_TETH_AN_FEC_REQ       (1 << 14)
#define XLNID_KRM_LINK_CTRL_1_TETH_AN_CAP_FEC       (1 << 15)
#define XLNID_KRM_LINK_CTRL_1_TETH_AN_CAP_KX        (1 << 16)
#define XLNID_KRM_LINK_CTRL_1_TETH_AN_CAP_KR        (1 << 18)
#define XLNID_KRM_LINK_CTRL_1_TETH_EEE_CAP_KX       (1 << 24)
#define XLNID_KRM_LINK_CTRL_1_TETH_EEE_CAP_KR       (1 << 26)
#define XLNID_KRM_LINK_S1_MAC_AN_COMPLETE       (1 << 28)
#define XLNID_KRM_LINK_CTRL_1_TETH_AN_ENABLE        (1 << 29)
#define XLNID_KRM_LINK_CTRL_1_TETH_AN_RESTART       (1 << 31)

#define XLNID_KRM_AN_CNTL_1_SYM_PAUSE           (1 << 28)
#define XLNID_KRM_AN_CNTL_1_ASM_PAUSE           (1 << 29)
#define XLNID_KRM_PCS_KX_AN_SYM_PAUSE           (1 << 1)
#define XLNID_KRM_PCS_KX_AN_ASM_PAUSE           (1 << 2)
#define XLNID_KRM_PCS_KX_AN_LP_SYM_PAUSE        (1 << 2)
#define XLNID_KRM_PCS_KX_AN_LP_ASM_PAUSE        (1 << 3)
#define XLNID_KRM_AN_CNTL_4_ECSR_AN37_OVER_73       (1 << 29)
#define XLNID_KRM_AN_CNTL_8_LINEAR          (1 << 0)
#define XLNID_KRM_AN_CNTL_8_LIMITING            (1 << 1)

#define XLNID_KRM_LP_BASE_PAGE_HIGH_SYM_PAUSE       (1 << 10)
#define XLNID_KRM_LP_BASE_PAGE_HIGH_ASM_PAUSE       (1 << 11)

#define XLNID_KRM_SGMII_CTRL_MAC_TAR_FORCE_100_D    (1 << 12)
#define XLNID_KRM_SGMII_CTRL_MAC_TAR_FORCE_10_D     (1 << 19)

#define XLNID_KRM_DSP_TXFFE_STATE_C0_EN         (1 << 6)
#define XLNID_KRM_DSP_TXFFE_STATE_CP1_CN1_EN        (1 << 15)
#define XLNID_KRM_DSP_TXFFE_STATE_CO_ADAPT_EN       (1 << 16)

#define XLNID_KRM_RX_TRN_LINKUP_CTRL_CONV_WO_PROTOCOL   (1 << 4)
#define XLNID_KRM_RX_TRN_LINKUP_CTRL_PROTOCOL_BYPASS    (1 << 2)

#define XLNID_KRM_PMD_DFX_BURNIN_TX_RX_KR_LB_MASK   (0x3 << 16)

#define XLNID_KRM_TX_COEFF_CTRL_1_CMINUS1_OVRRD_EN  (1 << 1)
#define XLNID_KRM_TX_COEFF_CTRL_1_CPLUS1_OVRRD_EN   (1 << 2)
#define XLNID_KRM_TX_COEFF_CTRL_1_CZERO_EN      (1 << 3)
#define XLNID_KRM_TX_COEFF_CTRL_1_OVRRD_EN      (1 << 31)

#define XLNID_SB_IOSF_INDIRECT_CTRL 0x00011144
#define XLNID_SB_IOSF_INDIRECT_DATA 0x00011148

#define XLNID_SB_IOSF_CTRL_ADDR_SHIFT       0
#define XLNID_SB_IOSF_CTRL_ADDR_MASK        0xFF
#define XLNID_SB_IOSF_CTRL_RESP_STAT_SHIFT  18
#define XLNID_SB_IOSF_CTRL_RESP_STAT_MASK   \
				(0x3 << XLNID_SB_IOSF_CTRL_RESP_STAT_SHIFT)
#define XLNID_SB_IOSF_CTRL_CMPL_ERR_SHIFT   20
#define XLNID_SB_IOSF_CTRL_CMPL_ERR_MASK    \
				(0xFF << XLNID_SB_IOSF_CTRL_CMPL_ERR_SHIFT)
#define XLNID_SB_IOSF_CTRL_TARGET_SELECT_SHIFT  28
#define XLNID_SB_IOSF_CTRL_TARGET_SELECT_MASK   0x7
#define XLNID_SB_IOSF_CTRL_BUSY_SHIFT       31
#define XLNID_SB_IOSF_CTRL_BUSY     (1 << XLNID_SB_IOSF_CTRL_BUSY_SHIFT)
#define XLNID_SB_IOSF_TARGET_KR_PHY 0

#define XLNID_NW_MNG_IF_SEL     0x00011178
#define XLNID_NW_MNG_IF_SEL_MDIO_ACT    (1u << 1)
#define XLNID_NW_MNG_IF_SEL_MDIO_IF_MODE    (1u << 2)
#define XLNID_NW_MNG_IF_SEL_EN_SHARED_MDIO  (1u << 13)
#define XLNID_NW_MNG_IF_SEL_PHY_SPEED_10M   (1u << 17)
#define XLNID_NW_MNG_IF_SEL_PHY_SPEED_100M  (1u << 18)
#define XLNID_NW_MNG_IF_SEL_PHY_SPEED_1G    (1u << 19)
#define XLNID_NW_MNG_IF_SEL_PHY_SPEED_2_5G  (1u << 20)
#define XLNID_NW_MNG_IF_SEL_PHY_SPEED_10G   (1u << 21)
#define XLNID_NW_MNG_IF_SEL_SGMII_ENABLE    (1u << 25)
#define XLNID_NW_MNG_IF_SEL_INT_PHY_MODE (1 << 24) /* X552 reg field only */
#define XLNID_NW_MNG_IF_SEL_MDIO_PHY_ADD_SHIFT 3
#define XLNID_NW_MNG_IF_SEL_MDIO_PHY_ADD    \
				(0x1F << XLNID_NW_MNG_IF_SEL_MDIO_PHY_ADD_SHIFT)

#include "xlnid_osdep2.h"

/* MACsec */
#define XLNID_MACSEC_MAX_SC                  1
#define XLNID_MACSEC_KEY_LEN_128_BIT         16
#define XLNID_MACSEC_TXCTL_MASK              0xfffffffc
#define XLNID_MACSEC_RXCTL_MASK              0xfffffff0
#define XLNID_MACSEC_PN_MAX                  0xffffffff
#define XLNID_MACSEC_PN_THRESHOLD            0xfffff0fe
#define XLNID_MACSEC_PN_INIT                 0x01000000
#define XLNID_MACSEC_RXCTL_PLSH              0x00000080
#define XLNID_MACSEC_RXCTL_RXEN_DISABLED     0x00000000
#define XLNID_MACSEC_RXCTL_RXEN_CHECK        0x00000004
#define XLNID_MACSEC_RXCTL_RXEN_STRICT       0x00000008
#define XLNID_MACSEC_RXCTL_RXEN_MASK         0x0000000C
#define XLNID_MACSEC_RX_SCI_PORT_MASK        0x0000FFFF

#define XLNID_MACSEC_RXSA_FRR                0x00000008
#define XLNID_MACSEC_TXSA_ACTSA              0x00000020

#define XLNID_MACSEC_TXCTL_PN_THRESHOLD      0xFFFFF020
#define XLNID_MACSEC_TXCTL_AISCI             0x00000020
#define XLNID_MACSEC_TXCTL_ENCRYPT           0x00000002
#define XLNID_MACSEC_TXCTL_ICV               0x00000001
#define XLNID_MACSEC_TXCTL_SA                0x00000010
#define XLNID_MACSEC_RXCTL_SA                0x00000004
#define XLNID_MACSEC_TXCTL_TXEN_DISABLED     0xfffff000
#define XLNID_MACSEC_RXCTL_SA_VALID          0x00000004
#define XLNID_MACSEC_RXCTL_SA_INVALID        0x00000000

#endif /* _XLNID_TYPE_H_ */

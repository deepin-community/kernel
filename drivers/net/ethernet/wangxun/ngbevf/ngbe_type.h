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

#ifndef _NGBE_TYPE_H_
#define _NGBE_TYPE_H_

#include "ngbe_osdep.h"
#include "ngbe_devids.h"
#include "ngbe_status.h"

/* Number of Transmit and Receive Descriptors(*8) */
#define NGBE_REQ_TX_DESCRIPTOR_MULTIPLE        8
#define NGBE_REQ_RX_DESCRIPTOR_MULTIPLE        8

/******************************************************************************
 * Receive Descriptor
******************************************************************************/
union ngbe_rx_desc {
	struct {
		__le64 pkt_addr; /* Packet buffer address */
		__le64 hdr_addr; /* Header buffer address */
	} rd;
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
				__le32 data; /* RSS Hash */
				struct {
					__le16 ipid; /* IP id */
					__le16 csum; /* Packet Checksum */
				} ip_csum;
			} hi_dword;
		} lower;
		struct {
			__le32 status; /* ext status/error */
			__le16 length; /* Packet length */
			__le16 vlan; /* VLAN tag */
		} upper;
	} wb;  /* writeback */
};

/* Context descriptors */
struct ngbe_adv_tx_context_desc {
	__le32 vlan_macip_lens;
	__le32 seqnum_seed;
	__le32 type_tucmd_mlhl;
	__le32 mss_l4len_idx;
};

/*** @ngbe_rx_desc.rd.lower.pkt_addr ***/
#define NGBE_RXD_PKTADDR(v)       cpu_to_le64((v))

/*** @ngbe_rx_desc.rd.lower.hdr_addr ***/
#define NGBE_RXD_HDRADDR(v)       cpu_to_le64((v))

/*** @ngbe_rx_desc.wb.lower.lo_dword ***/
/* RSS Hash results */
#define NGBE_RXD_RSSTYPE(rxd) \
	    ((le32_to_cpu((rxd)->wb.lower.lo_dword.data)) & 0xF)
#define   NGBE_RSSTYPE_NONE        (0)
#define   NGBE_RSSTYPE_IPV4_TCP    (1)
#define   NGBE_RSSTYPE_IPV4        (2)
#define   NGBE_RSSTYPE_IPV6_TCP    (3)
#define   NGBE_RSSTYPE_IPV4_SCTP   (4)
#define   NGBE_RSSTYPE_IPV6        (5)
#define   NGBE_RSSTYPE_IPV6_SCTP   (6)
#define   NGBE_RSSTYPE_IPV4_UDP    (7)
#define   NGBE_RSSTYPE_IPV6_UDP    (8)
#define   NGBE_RSSTYPE_FDIR        (15)
#define NGBE_RXD_SECTYPE(rxd) \
	    ((le32_to_cpu((rxd)->wb.lower.lo_dword.data) >> 4) & 0x3)
#define   NGBE_SECTYPE_NONE          (0)
#define   NGBE_SECTYPE_LINKSEC       (1)
#define   NGBE_SECTYPE_IPSECESP      (2)
#define   NGBE_SECTYPE_IPSECAH       (3)
#define NGBE_RXD_TPID_SEL(rxd) \
	    ((le32_to_cpu((rxd)->wb.lower.lo_dword.data) >> 6) & 0x7)
#define NGBE_RXD_PKTTYPE(rxd) \
	    ((le32_to_cpu((rxd)->wb.lower.lo_dword.data) >> 9) & 0xFF)
#define NGBE_RXD_RSCCNT(rxd) \
	    ((le32_to_cpu((rxd)->wb.lower.lo_dword.data) >> 17) & 0xF)
#define NGBE_RXD_HDRLEN(rxd) \
	    ((le32_to_cpu((rxd)->wb.lower.lo_dword.data) >> 21) & 0x3FF)
#define NGBE_RXD_SPH              ((0x1) << 31)

/*** @ngbe_rx_desc.wb.lower.hi_dword ***/
/** bit 0-31, as rss hash when  **/
#define NGBE_RXD_RSS_HASH(rxd) \
	    ((le32_to_cpu((rxd)->wb.lower.hi_dword.data)))

/** bit 0-31, as ip csum when  **/
#define NGBE_RXD_IPCSUM(rxd) \
	    ((le16_to_cpu((rxd)->wb.lower.hi_dword.ip_csum.ipid)))
#define NGBE_RXD_IPCSUM_CSUM(rxd) \
	    ((le16_to_cpu((rxd)->wb.lower.hi_dword.ip_csum.csum)))

/** bit 0-31, as fdir id when  **/
#define NGBE_RXD_FDIR_ID(rxd) \
	    ((le32_to_cpu((rxd)->wb.lower.hi_dword.data)))

/*** @ngbe_rx_desc.wb.upper.status ***/
#define NGBE_RXD_STATUS(rxd) \
	    (le32_to_cpu((rxd)->wb.upper.status)) /* All Status */
/** bit 0-1 **/
#define NGBE_RXD_STAT_DD       ((0x1) << 0) /* Descriptor Done */
#define NGBE_RXD_STAT_EOP      ((0x1) << 1) /* End of Packet */
/** bit 2-31, when EOP=1 **/
#define NGBE_RXD_NEXTP_RESV(v) ((0x3 & (v)) << 2)
#define NGBE_RXD_NEXTP(v)      ((0xFFFF & (v)) << 4) /* Next Descriptor Index */
/** bit 2-31, when EOP=0 **/
#define NGBE_RXD_STAT_CLASS(v)       ((0x7 & (v)) << 2) /* Packet Class */
#define   NGBE_PKT_CLASS(r)      (((r) >> 2) & 0x7)
#define   NGBE_PKT_CLASS_TC_RSS  (0) /* RSS Hash */
#define   NGBE_PKT_CLASS_FLM     (1) /* FDir Match */
#define   NGBE_PKT_CLASS_SYN     (2) /* TCP Sync */
#define   NGBE_PKT_CLASS_5TUPLE  (3) /* 5 Tuple */
#define   NGBE_PKT_CLASS_L2ETYPE (4) /* L2 Ethertype */
#define NGBE_RXD_STAT_VP       ((0x1) << 5) /* IEEE VLAN Packet */
#define NGBE_RXD_STAT_UDPCS    ((0x1) << 6) /* UDP xsum calculated */
#define NGBE_RXD_STAT_TPCS     ((0x1) << 7) /* L4 xsum calculated */
#define NGBE_RXD_STAT_IPCS     ((0x1) << 8) /* IP xsum calculated */
#define NGBE_RXD_STAT_PIF      ((0x1) << 9) /* Non-unicast address */
#define NGBE_RXD_STAT_EIPCS    ((0x1) << 10) /* Encap IP xsum calculated */
#define NGBE_RXD_STAT_VEXT     ((0x1) << 11) /* Multi-VLAN */
#define NGBE_RXD_STAT_IPV6EX   ((0x1) << 12) /* IPv6 with option header */
#define NGBE_RXD_STAT_LLINT    ((0x1) << 13) /* Pkt caused Low Latency Interrupt */
#define NGBE_RXD_STAT_TS       ((0x1) << 14) /* IEEE1588 Time Stamp */
#define NGBE_RXD_STAT_SECP     ((0x1) << 15) /* Security Processing */
#define NGBE_RXD_STAT_LB       ((0x1) << 16) /* Loopback Status */
/* bit 17-30, when PKTTYPE=IP */
#define NGBE_RXD_STAT_BMC      ((0x1) << 17) /* PKTTYPE=IP, BMC status */
#define NGBE_RXD_ERR_FDIRERR(v) ((0x7 & (v)) << 20) /* FDIRERR */
#define   NGBE_RXD_ERR_FDIR_LEN   ((0x1) << 20) /* FDIR Length error */
#define   NGBE_RXD_ERR_FDIR_DROP  ((0x1) << 21) /* FDIR Drop error */
#define   NGBE_RXD_ERR_FDIR_COLL  ((0x1) << 22) /* FDIR Collision error */
#define NGBE_RXD_ERR_HBO      ((0x1) << 23) /*Header Buffer Overflow */
#define NGBE_RXD_ERR_EIPERR   ((0x1) << 26) /* Encap IP header error */
#define NGBE_RXD_ERR_SECERR(v)   ((0x3 & (v)) << 27)
#define   NGBE_IP_SECERR_0    (0)
#define   NGBE_IP_SECERR_1    (1)
#define   NGBE_IP_SECERR_2    (2)
#define   NGBE_IP_SECERR_3    (3)
#define NGBE_RXD_ERR_RXE      ((0x1) << 29) /* Any MAC Error */
#define NGBE_RXD_ERR_TPE      ((0x1) << 30) /* TCP/UDP Checksum Error */
#define NGBE_RXD_ERR_IPE      ((0x1) << 31) /* IP Checksum Error */
/* bit 17-30, when PKTTYPE=FCOE */
#define NGBE_RXD_STAT_FCOEFS   ((0x1) << 17) /* PKTTYPE=FCOE, FCoE EOF/SOF Stat */
#define NGBE_RXD_STAT_FCSTAT(v)     ((0x3 & (v)) << 18) /* FCoE Pkt Stat */
#define   NGBE_FCOE_FCSTAT(r)      (((r) >> 18) & 0x7)
#define   NGBE_FCOE_FCSTAT_NOMTCH  (0) /* No Ctxt Match */
#define   NGBE_FCOE_FCSTAT_NODDP   (1) /* Ctxt w/o DDP */
#define   NGBE_FCOE_FCSTAT_FCPRSP  (2) /* Recv. FCP_RSP */
#define   NGBE_FCOE_FCSTAT_DDP     (3) /* Ctxt w/ DDP */
#define NGBE_RXD_ERR_FCERR(v)  ((0x7 & (v)) << 20) /* FCERR */

/*** @ngbe_rx_desc.wb.upper.length ***/
#define NGBE_RXD_LENGTH(rxd) \
	    ((le16_to_cpu((rxd)->wb.upper.length)))

/*** @ngbe_rx_desc.wb.upper.vlan ***/
#define NGBE_RXD_VLAN(rxd) \
	    ((le16_to_cpu((rxd)->wb.upper.vlan)))

/******************************************************************************
 * Transmit Descriptor
******************************************************************************/
/**
 * Transmit Context Descriptor (NGBE_TXD_DTYP_CTXT=1)
 **/
struct ngbe_tx_ctxt_desc {
	__le32 vlan_macip_lens;
	__le32 seqnum_seed;
	__le32 type_tucmd_mlhl;
	__le32 mss_l4len_idx;
};

/*** @ngbe_tx_ctxt_desc.rd.vlan_macip_lens ***/
#define NGBE_TXD_HDRLEN(v)           (((v) >> 1 & 0x1FF)) /* ip/fcoe header end offset */
#define NGBE_TXD_MACLEN(v)           (((v) & 0x7F) << 9)  /* desc mac len */
#define NGBE_TXD_VLAN(v)             (((v) & 0xFFFF) << 16)  /* vlan tag */

/*** @ngbe_tx_ctxt_desc.rd.seqnum_seed ***/
/* bit 0-31, when NGBE_TXD_DTYP_FCOE=0 */
#define NGBE_TXD_IPSEC_SAIDX(v)      (((v) & 0x3FF)) /* IPSec SA index */
#define NGBE_TXD_ENCAP_TYPE(v)       (((v) & 0x1) << 11) /* tunnel type */
#define     NGBE_ENCAP_TYPE_UDP       (0)
#define     NGBE_ENCAP_TYPE_GRE       (1)
#define NGBE_TXD_ENCAP_IPLEN(v)      (((v) & 0x7F) << 12) /* outer ip header length */
#define NGBE_TXD_DTYP_FCOE           ((0x1) << 16) /* FCoE/IP descriptor */
#define NGBE_TXD_ENCAP_TUNLEN(v)     (((v) & 0xFF) << 19) /* outer tunnel header length */
#define NGBE_TXD_DECTTL(v)           (((v) & 0xF) << 27) /* decrease ip TTL */
/* bit 0-31, when NGBE_TXD_DTYP_FCOE=1 */
#define NGBE_TXD_FCOEF_EOF(v)       (((v) & 0x3) << 10) /* FC EOF index */
#define     NGBE_FCOEF_EOF_N            (0) /* EOFn */
#define     NGBE_FCOEF_EOF_T            (1) /* EOFt */
#define     NGBE_FCOEF_EOF_NI           (2) /* EOFni */
#define     NGBE_FCOEF_EOF_A            (3) /* EOFa */
#define NGBE_TXD_FCOEF_SOF          (0x1 << 12) /* FC SOF index */
#define NGBE_TXD_FCOEF_PARINC       (0x1 << 13) /* Rel_Off in F_CTL */
#define NGBE_TXD_FCOEF_ORIE         (0x1 << 14) /* Orientation End */
#define NGBE_TXD_FCOEF_ORIS         (0x1 << 15) /* Orientation Start */

/*** @ngbe_tx_ctxt_desc.rd.type_tucmd_mlhl ***/
#define NGBE_TXD_IPSEC_ESPLEN(v)     (((v) & 0x1FF)) /* IPSec ESP length */
#define NGBE_TXD_SNAP                ((0x1) << 10) /* SNAP indication */
#define NGBE_TXD_TPID_SEL(v)         (((v) & 0x7) << 10) /* VLAN TPID index */
#define NGBE_TXD_IPSEC_TYPE(v)       (((v) & 0x1) << 14) /* IPSec Type */
#define     NGBE_IPSEC_TYPE_AH        (0)
#define     NGBE_IPSEC_TYPE_ESP       (1)
#define NGBE_TXD_IPSEC_ESPENC(v)     (((v) & 0x1) << 15) /* ESP encrypt */
#define NGBE_TXD_DTYP_CTXT           ((0x1) << 20) /* CTXT/DATA descriptor */
#define NGBE_TXD_PKTTYPE(v)          (((v) & 0xFF) << 24) /* packet type */
/*** @ngbe_tx_ctxt_desc.rd.mss_l4len_idx ***/
#define NGBE_TXD_CTX_DD              ((0x1)) /* Descriptor Done */
#define NGBE_TXD_TPLEN(v)            (((v) & 0xFF) << 8) /* transport header length */
#define NGBE_TXD_MSS(v)              (((v) & 0xFFFF) << 16) /* transport maximum segment size */

/**
 * Transmit Data Descriptor (NGBE_TXD_DTYP_CTXT=0)
 **/
struct ngbe_tx_desc {
	__le64 pkt_addr;
	__le32 cmd_type_len;
	__le32 status;
};
/*** @ngbe_tx_desc.pkt_addr ***/

/*** @ngbe_tx_desc.cmd_type_len ***/
#define NGBE_TXD_DTALEN(v)          (((v) & 0xFFFF)) /* data buffer length */
#define NGBE_TXD_TSTAMP             (0x1 << 19) /* IEEE1588 time stamp */
#define NGBE_TXD_EOP                (0x1 << 24) /* End of Packet */
#define NGBE_TXD_FCS                (0x1 << 25) /* Insert FCS */
#define NGBE_TXD_LINKSEC            (0x1 << 26) /* Insert LinkSec */
#define NGBE_TXD_RS                 (0x1 << 27) /* Report Status */
#define NGBE_TXD_ECU                (0x1 << 28) /* forward to ECU */
#define NGBE_TXD_CNTAG              (0x1 << 29) /* insert CN tag */
#define NGBE_TXD_VLE                (0x1 << 30) /* insert VLAN tag */
#define NGBE_TXD_TSE                (0x1 << 31) /* enable transmit segmentation */

/*** @ngbe_tx_desc.status ***/
#define NGBE_TXD_STAT_DD            NGBE_TXD_CTX_DD /* Descriptor Done */
#define NGBE_TXD_BAK_DESC           ((0x1) << 4) /* use backup descriptor */
#define NGBE_TXD_CC                 ((0x1) << 7) /* check context */
#define NGBE_TXD_IPSEC              ((0x1) << 8) /* request IPSec offload */
#define NGBE_TXD_TPCS               ((0x1) << 9) /* insert TCP/UDP checksum */
#define NGBE_TXD_IPCS               ((0x1) << 10) /* insert IP checksum */
#define NGBE_TXD_EIPCS              ((0x1) << 11) /* insert outer IP checksum */
#define NGBE_TXD_MNGFLT             ((0x1) << 12) /* enable management filter */
#define NGBE_TXD_PAYLEN(v)          (((v) & 0x7FFFF) << 13) /* payload length */

/******************************************************************************
 * Misc
******************************************************************************/
/* Link speed */
typedef u32 ngbe_link_speed;
#define NGBE_LINK_SPEED_UNKNOWN        0
#define NGBE_LINK_SPEED_10_FULL        0x0002
#define NGBE_LINK_SPEED_100_FULL       0x0008
#define NGBE_LINK_SPEED_1GB_FULL       0x0020

enum ngbe_mac_type {
	ngbe_mac_unknown = 0,
	ngbe_mac_em_vf,
	ngbe_num_macs
};

enum ngbe_media_type {
	ngbe_media_type_unknown = 0,
	ngbe_media_type_fiber,
	ngbe_media_type_copper,
	ngbe_media_type_backplane,
	ngbe_media_type_cx4,
	ngbe_media_type_virtual
};

struct ngbevf_hw;

/* iterator type for walking multicast address lists */
typedef u8* (*ngbe_mc_addr_itr) (struct ngbevf_hw *hw, u8 **mc_addr_ptr,
				  u32 *vmdq);

/******************************************************************************
 * kernel type redefinition
******************************************************************************/
#ifdef HAVE_NDO_GET_STATS64
typedef struct rtnl_link_stats64 ngbe_net_stats_t;
#else
typedef struct net_device_stats ngbe_net_stats_t;
#endif

/* Adv Transmit Descriptor Config Masks */
#define NGBE_ADVTXD_DTALEN_MASK	0x0000FFFF /* Data buf length(bytes) */
#define NGBE_ADVTXD_MAC_LINKSEC	0x00040000 /* Insert LinkSec */
#define NGBE_ADVTXD_MAC_TSTAMP		0x00080000 /* IEEE1588 time stamp */
#define NGBE_ADVTXD_IPSEC_SA_INDEX_MASK 0x000003FF /* IPSec SA index */
#define NGBE_ADVTXD_IPSEC_ESP_LEN_MASK	0x000001FF /* IPSec ESP length */
#define NGBE_ADVTXD_DTYP_MASK		0x00F00000 /* DTYP mask */
#define NGBE_ADVTXD_DTYP_CTXT		0x00200000 /* Adv Context Desc */
#define NGBE_ADVTXD_DTYP_DATA		0x00300000 /* Adv Data Descriptor */
//#define TXGBE_ADVTXD_DCMD_EOP		TXGBE_TXD_CMD_EOP  /* End of Packet */
//#define TXGBE_ADVTXD_DCMD_IFCS		TXGBE_TXD_CMD_IFCS /* Insert FCS */
//#define TXGBE_ADVTXD_DCMD_RS		TXGBE_TXD_CMD_RS /* Report Status */
#define NGBE_ADVTXD_DCMD_DDTYP_ISCSI	0x10000000 /* DDP hdr type or iSCSI */
#define NGBE_ADVTXD_DCMD_DEXT		NGBE_TXD_CMD_DEXT /* Desc ext 1=Adv */
//#define TXGBE_ADVTXD_DCMD_VLE		TXGBE_TXD_CMD_  /* VLAN pkt enable */
#define NGBE_ADVTXD_DCMD_TSE		0x80000000 /* TCP Seg enable */
#define NGBE_ADVTXD_STAT_DD		NGBE_TXD_STAT_DD  /* Descriptor Done */
#define NGBE_ADVTXD_STAT_SN_CRC	0x00000002 /* NXTSEQ/SEED pres in WB */
#define NGBE_ADVTXD_STAT_RSV		0x0000000C /* STA Reserved */
#define NGBE_ADVTXD_IDX_SHIFT		4 /* Adv desc Index shift */
#define NGBE_ADVTXD_CC			0x00000080 /* Check Context */
#define NGBE_ADVTXD_POPTS_SHIFT	8  /* Adv desc POPTS shift */

#define NGBE_ADVTXD_PAYLEN_SHIFT	14 /* Adv desc PAYLEN shift */
#define NGBE_ADVTXD_MACLEN_SHIFT	9  /* Adv ctxt desc mac len shift */
#define NGBE_ADVTXD_VLAN_SHIFT		16  /* Adv ctxt vlan tag shift */

#define NGBE_TXD_CMD_DEXT	0x20000000 /* Desc extension (0 = legacy) */


#endif /* _NGBE_TYPE_H_ */

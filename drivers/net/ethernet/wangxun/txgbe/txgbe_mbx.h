/*
 * WangXun 10 Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * based on ixgbe_mbx.h, Copyright(c) 1999 - 2017 Intel Corporation.
 * Contact Information:
 * Linux NICS <linux.nics@intel.com>
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 */

#ifndef _TXGBE_MBX_H_
#define _TXGBE_MBX_H_

#define TXGBE_VXMAILBOX_SIZE    (16 - 1)

/**
 * VF Registers
 **/
#define TXGBE_VXMAILBOX         0x00600
#define TXGBE_VXMAILBOX_REQ     ((0x1) << 0) /* Request for PF Ready bit */
#define TXGBE_VXMAILBOX_ACK     ((0x1) << 1) /* Ack PF message received */
#define TXGBE_VXMAILBOX_VFU     ((0x1) << 2) /* VF owns the mailbox buffer */
#define TXGBE_VXMAILBOX_PFU     ((0x1) << 3) /* PF owns the mailbox buffer */
#define TXGBE_VXMAILBOX_PFSTS   ((0x1) << 4) /* PF wrote a message in the MB */
#define TXGBE_VXMAILBOX_PFACK   ((0x1) << 5) /* PF ack the previous VF msg */
#define TXGBE_VXMAILBOX_RSTI    ((0x1) << 6) /* PF has reset indication */
#define TXGBE_VXMAILBOX_RSTD    ((0x1) << 7) /* PF has indicated reset done */
#define TXGBE_VXMAILBOX_R2C_BITS (TXGBE_VXMAILBOX_RSTD | \
	    TXGBE_VXMAILBOX_PFSTS | TXGBE_VXMAILBOX_PFACK)

#define TXGBE_VXMBMEM           0x00C00 /* 16*4B */

/**
 * PF Registers
 **/
#define TXGBE_PXMAILBOX(i)      (0x00600 + (4 * (i))) /* i=[0,63] */
#define TXGBE_PXMAILBOX_STS     ((0x1) << 0) /* Initiate message send to VF */
#define TXGBE_PXMAILBOX_ACK     ((0x1) << 1) /* Ack message recv'd from VF */
#define TXGBE_PXMAILBOX_VFU     ((0x1) << 2) /* VF owns the mailbox buffer */
#define TXGBE_PXMAILBOX_PFU     ((0x1) << 3) /* PF owns the mailbox buffer */
#define TXGBE_PXMAILBOX_RVFU    ((0x1) << 4) /* Reset VFU - used when VF stuck*/

#define TXGBE_PXMBMEM(i)        (0x5000 + (64 * (i))) /* i=[0,63] */

#define TXGBE_VFLRP(i)          (0x00490 + (4 * (i))) /* i=[0,1] */
#define TXGBE_VFLRE(i)          (0x004A0 + (4 * (i))) /* i=[0,1] */
#define TXGBE_VFLREC(i)         (0x004A8 + (4 * (i))) /* i=[0,1] */

/* SR-IOV specific macros */
#define TXGBE_MBVFICR(i)         (0x00480 + (4 * (i))) /* i=[0,3] */
#define TXGBE_MBVFICR_INDEX(vf) ((vf) >> 4)
#define TXGBE_MBVFICR_VFREQ_MASK (0x0000FFFF) /* bits for VF messages */
#define TXGBE_MBVFICR_VFREQ_VF1  (0x00000001) /* bit for VF 1 message */
#define TXGBE_MBVFICR_VFACK_MASK (0xFFFF0000) /* bits for VF acks */
#define TXGBE_MBVFICR_VFACK_VF1  (0x00010000) /* bit for VF 1 ack */

/**
 * Messages
 **/
/* If it's a TXGBE_VF_* msg then it originates in the VF and is sent to the
 * PF.  The reverse is true if it is TXGBE_PF_*.
 * Message ACK's are the value or'd with 0xF0000000
 */
#define TXGBE_VT_MSGTYPE_ACK    0x80000000 /* Messages below or'd with
					    * this are the ACK */
#define TXGBE_VT_MSGTYPE_NACK   0x40000000 /* Messages below or'd with
					    * this are the NACK */
#define TXGBE_VT_MSGTYPE_CTS    0x20000000 /* Indicates that VF is still
					    * clear to send requests */
#define TXGBE_VT_MSGINFO_SHIFT  16
/* bits 23:16 are used for extra info for certain messages */
#define TXGBE_VT_MSGINFO_MASK   (0xFF << TXGBE_VT_MSGINFO_SHIFT)

/* definitions to support mailbox API version negotiation */

/*
 * each element denotes a version of the API; existing numbers may not
 * change; any additions must go at the end
 */
enum txgbe_pfvf_api_rev {
	txgbe_mbox_api_null,
	txgbe_mbox_api_10,      /* API version 1.0, linux/freebsd VF driver */
	txgbe_mbox_api_11,      /* API version 1.1, linux/freebsd VF driver */
	txgbe_mbox_api_12,      /* API version 1.2, linux/freebsd VF driver */
	txgbe_mbox_api_13,	/* API version 1.3, linux/freebsd VF driver */
	txgbe_mbox_api_20,      /* API version 2.0, solaris Phase1 VF driver */
	txgbe_mbox_api_unknown, /* indicates that API version is not known */
};

/* mailbox API, legacy requests */
#define TXGBE_VF_RESET          0x01 /* VF requests reset */
#define TXGBE_VF_SET_MAC_ADDR   0x02 /* VF requests PF to set MAC addr */
#define TXGBE_VF_SET_MULTICAST  0x03 /* VF requests PF to set MC addr */
#define TXGBE_VF_SET_VLAN       0x04 /* VF requests PF to set VLAN */

/* mailbox API, version 1.0 VF requests */
#define TXGBE_VF_SET_LPE        0x05 /* VF requests PF to set VMOLR.LPE */
#define TXGBE_VF_SET_MACVLAN    0x06 /* VF requests PF for unicast filter */
#define TXGBE_VF_API_NEGOTIATE  0x08 /* negotiate API version */

/* mailbox API, version 1.1 VF requests */
#define TXGBE_VF_GET_QUEUES     0x09 /* get queue configuration */

/* mailbox API, version 1.2 VF requests */
#define TXGBE_VF_GET_RETA      0x0a    /* VF request for RETA */
#define TXGBE_VF_GET_RSS_KEY	0x0b    /* get RSS key */
#define TXGBE_VF_UPDATE_XCAST_MODE	0x0c
#define TXGBE_VF_BACKUP		0x8001 /* VF requests backup */

/* mode choices for IXGBE_VF_UPDATE_XCAST_MODE */
enum txgbevf_xcast_modes {
	TXGBEVF_XCAST_MODE_NONE = 0,
	TXGBEVF_XCAST_MODE_MULTI,
	TXGBEVF_XCAST_MODE_ALLMULTI,
	TXGBEVF_XCAST_MODE_PROMISC,
};

/* GET_QUEUES return data indices within the mailbox */
#define TXGBE_VF_TX_QUEUES      1       /* number of Tx queues supported */
#define TXGBE_VF_RX_QUEUES      2       /* number of Rx queues supported */
#define TXGBE_VF_TRANS_VLAN     3       /* Indication of port vlan */
#define TXGBE_VF_DEF_QUEUE      4       /* Default queue offset */

/* length of permanent address message returned from PF */
#define TXGBE_VF_PERMADDR_MSG_LEN       4
/* word in permanent address message with the current multicast type */
#define TXGBE_VF_MC_TYPE_WORD           3

#define TXGBE_PF_CONTROL_MSG            0x0100 /* PF control message */

/* mailbox API, version 2.0 VF requests */
#define TXGBE_VF_API_NEGOTIATE          0x08 /* negotiate API version */
#define TXGBE_VF_GET_QUEUES             0x09 /* get queue configuration */
#define TXGBE_VF_ENABLE_MACADDR         0x0A /* enable MAC address */
#define TXGBE_VF_DISABLE_MACADDR        0x0B /* disable MAC address */
#define TXGBE_VF_GET_MACADDRS           0x0C /* get all configured MAC addrs */
#define TXGBE_VF_SET_MCAST_PROMISC      0x0D /* enable multicast promiscuous */
#define TXGBE_VF_GET_MTU                0x0E /* get bounds on MTU */
#define TXGBE_VF_SET_MTU                0x0F /* set a specific MTU */

/* mailbox API, version 2.0 PF requests */
#define TXGBE_PF_TRANSPARENT_VLAN       0x0101 /* enable transparent vlan */

#define TXGBE_VF_MBX_INIT_TIMEOUT       2000 /* number of retries on mailbox */
#define TXGBE_VF_MBX_INIT_DELAY         500  /* microseconds between retries */

int txgbe_read_mbx(struct txgbe_hw *, u32 *, u16, u16);
int txgbe_write_mbx(struct txgbe_hw *, u32 *, u16, u16);
int txgbe_read_posted_mbx(struct txgbe_hw *, u32 *, u16, u16);
int txgbe_write_posted_mbx(struct txgbe_hw *, u32 *, u16, u16);
int txgbe_check_for_msg(struct txgbe_hw *, u16);
int txgbe_check_for_ack(struct txgbe_hw *, u16);
int txgbe_check_for_rst(struct txgbe_hw *, u16);
void txgbe_init_mbx_ops(struct txgbe_hw *hw);
void txgbe_init_mbx_params_vf(struct txgbe_hw *);
void txgbe_init_mbx_params_pf(struct txgbe_hw *);

#endif /* _TXGBE_MBX_H_ */

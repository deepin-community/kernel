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

#ifndef _NGBE_REGISTER_H_
#define _NGBE_REGISTER_H_

#define ngbe_conf_size(v, mwidth, uwidth) \
	((v) == 2 << (mwidth) ? 0 : (v) >> (uwidth))
#define ngbe_buf_len(v)    ngbe_conf_size(v, 13, 7)
#define ngbe_hdr_sz(v)     ngbe_conf_size(v, 10, 6)
#define ngbe_buf_sz(v)     ngbe_conf_size(v, 14, 10)
#define ngbe_pkt_thresh(v) ngbe_conf_size(v, 4, 0)

/**
 * VF Registers
 * r=ring index [0,7], i=local index,
 * g=value for register, f=value for field
 **/
#define NGBE_VXRXMEMWRAP           0x00000 /* i=[0,7] */
#define NGBE_VXSTATUS              0x00004

#define   NGBE_VXSTATUS_UP            (0x1)
#define   NGBE_VXSTATUS_SPEED(g)      (0x7 & ((g) >> 1))
#define     NGBE_VXSTATUS_SPEED_1G   (0x1)
#define     NGBE_VXSTATUS_SPEED_100M (0x2)
#define     NGBE_VXSTATUS_SPEED_10M  (0x4)
#define   NGBE_VXSTATUS_BUSY          ((0x1) << 4)
#define   NGBE_VXSTATUS_LANID         ((0x3) << 8)
#define NGBE_VXCTRL                0x00008
#define   NGBE_VXCTRL_RST      ((0x1) << 0)
#define NGBE_VXMRQC                  0x00078
#define   NGBE_VXMRQC_RSV         ((0x1) << 0)
#define   NGBE_VXMRQC_PSR(f)      ((0x1F & (f)) << 1)
#define     NGBE_VXMRQC_PSR_L4HDR     ((0x1) << 0)
#define     NGBE_VXMRQC_PSR_L3HDR     ((0x1) << 1)
#define     NGBE_VXMRQC_PSR_L2HDR     ((0x1) << 2)
#define     NGBE_VXMRQC_PSR_TUNHDR    ((0x1) << 3)
#define     NGBE_VXMRQC_PSR_TUNMAC    ((0x1) << 4)


#define NGBE_VXICR                 0x00100
#define NGBE_VXICS                 0x00104
#define NGBE_VXIMS                 0x00108
#define NGBE_VXIMC                 0x0010C
#define NGBE_VXLLI                 0x00118
#define NGBE_VXITR(i)               0x00200 + (4 * (i)) /* i=[0,1] */
#define   NGBE_VXITR_INTERVAL(f)    ((0x1FF & (f)) << 3)
#define   NGBE_VXITR_LLI            ((0x1) << 15)
#define   NGBE_VXITR_LLI_CREDIT(f)  ((0x1F & (f)) << 16)
#define   NGBE_VXITR_CNT(f)         ((0x7F & (f)) << 21)
#define   NGBE_VXITR_CNT_WDIS       ((0x1) << 31)
#define NGBE_VXIVAR(i)              0x00240 + (4 * (i)) /* i=[0,1] */
#define   NGBE_VXIVAR_ALLOC(i, f)   ((0x1 & (f)) << 8*(i))
#define   NGBE_VXIVAR_VALID(i, f)   ((0x80 & (f)) << 8*(i))
#define NGBE_VXIVAR_MISC            0x00260
#define   NGBE_VXIVAR_MISC_ALLOC(f) ((0x3 & (f)))
#define   NGBE_VXIVAR_MISC_VALID    ((0x80))

/* Receive Path */
#define NGBE_VXRDBAL        0x01000
#define NGBE_VXRDBAH        0x01004
#define NGBE_VXRDT          0x01008
#define NGBE_VXRDH          0x0100C
#define NGBE_VXRXDCTL       0x01010
#define   NGBE_VXRXDCTL_ENABLE     ((0x1) << 0)
#define   NGBE_VXRXDCTL_BUFLEN(f)  ((0x3F & (f)) << 1)
#define   NGBE_VXRXDCTL_BUFSZ(f)   ((0xF & (f)) << 8)
#define   NGBE_VXRXDCTL_HDRSZ(f)   ((0xF & (f)) << 12)
#define   NGBE_VXRXDCTL_WTHRESH(f) ((0x7 & (f)) << 16)
#define   NGBE_VXRXDCTL_ETAG       ((0x1) << 22)
#define   NGBE_VXRXDCTL_SPLIT      ((0x1) << 26)
#define   NGBE_VXRXDCTL_CNTAG      ((0x1) << 28)
#define   NGBE_VXRXDCTL_DROP       ((0x1) << 30)
#define   NGBE_VXRXDCTL_VLAN       ((0x1) << 31)

/* Transmit Path */
#define NGBE_VXTDBAL            0x03000
#define NGBE_VXTDBAH            0x03004
#define NGBE_VXTDT              0x03008
#define NGBE_VXTDH              0x0300C
#define NGBE_VXTXDCTL           0x03010
#define   NGBE_VXTXDCTL_ENABLE     ((0x1) << 0)
#define   NGBE_VXTXDCTL_BUFLEN(f)  ((0x3F & (f)) << 1)
#define   NGBE_VXTXDCTL_PTHRESH(f) ((0xF & (f)) << 8)
#define   NGBE_VXTXDCTL_WTHRESH(f) ((0x7F & (f)) << 16)
#define   NGBE_VXTXDCTL_FLUSH      ((0x1) << 26)


#define NGBE_VXGPRC            0x01014
#define NGBE_VXGORC_LSB        0x01018
#define NGBE_VXGORC_MSB        0x0101C
#define NGBE_VXMPRC            0x01020
#define NGBE_VXBPRC            0x01024

#define NGBE_VXGPTC            0x03014
#define NGBE_VXGOTC_LSB        0x03018
#define NGBE_VXGOTC_MSB        0x0301C
#define NGBE_VXMPTC            0x03020
#define NGBE_VXBPTC            0x03024

#endif /* _NGBE_REGISTER_H_ */

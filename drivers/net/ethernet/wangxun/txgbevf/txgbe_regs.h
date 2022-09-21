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

#ifndef _TXGBE_REGISTER_H_
#define _TXGBE_REGISTER_H_

#define txgbe_conf_size(v, mwidth, uwidth) \
	((v) == 2 << (mwidth) ? 0 : (v) >> (uwidth))
#define txgbe_buf_len(v)    txgbe_conf_size(v, 13, 7)
#define txgbe_hdr_sz(v)     txgbe_conf_size(v, 10, 6)
#define txgbe_buf_sz(v)     txgbe_conf_size(v, 14, 10)
#define txgbe_pkt_thresh(v) txgbe_conf_size(v, 4, 0)

/**
 * VF Registers
 * r=ring index [0,7], i=local index,
 * g=value for register, f=value for field
 **/
#define TXGBE_VXRXMEMWRAP           0x00000 /* i=[0,7] */
#define   TXGBE_VXRXMEMWRAP_WRAP(g, i)   ((0x7 << 4*(i) & (g)) >> 4*(i))
#define   TXGBE_VXRXMEMWRAP_EMPTY(g, i)  ((0x8 << 4*(i) & (g)) >> 4*(i))
#define TXGBE_VXSTATUS              0x00004
#define   TXGBE_VXSTATUS_UP            ((0x1) << 0)
#define   TXGBE_VXSTATUS_SPEED(g)      ((0x7 & (g)) >> 1)
#define     TXGBE_VXSTATUS_SPEED_10G   (0x1)
#define     TXGBE_VXSTATUS_SPEED_1G    (0x2)
#define     TXGBE_VXSTATUS_SPEED_100M  (0x4)
#define   TXGBE_VXSTATUS_BUSY          ((0x1) << 4)
#define   TXGBE_VXSTATUS_LANID         ((0x1) << 8)
#define TXGBE_VXCTRL                0x00008
#define   TXGBE_VXCTRL_RST      ((0x1) << 0)
#define TXGBE_VXMRQC                  0x00078
#define   TXGBE_VXMRQC_RSV         ((0x1) << 0)
#define   TXGBE_VXMRQC_PSR(f)      ((0x1F & (f)) << 1)
#define     TXGBE_VXMRQC_PSR_L4HDR     ((0x1) << 0)
#define     TXGBE_VXMRQC_PSR_L3HDR     ((0x1) << 1)
#define     TXGBE_VXMRQC_PSR_L2HDR     ((0x1) << 2)
#define     TXGBE_VXMRQC_PSR_TUNHDR    ((0x1) << 3)
#define     TXGBE_VXMRQC_PSR_TUNMAC    ((0x1) << 4)
#define   TXGBE_VXMRQC_RSS(f)      ((0xFFFF & (f)) << 16)
#define     TXGBE_VXMRQC_RSS_ALG(f)     ((0xFF) & (f)))
#define       TXGBE_VXMRQC_RSS_ALG_IPV4_TCP   ((0x1) << 0)
#define       TXGBE_VXMRQC_RSS_ALG_IPV4       ((0x1) << 1)
#define       TXGBE_VXMRQC_RSS_ALG_IPV6       ((0x1) << 4)
#define       TXGBE_VXMRQC_RSS_ALG_IPV6_TCP   ((0x1) << 5)
#define       TXGBE_VXMRQC_RSS_ALG_IPV4_UDP   ((0x1) << 6)
#define       TXGBE_VXMRQC_RSS_ALG_IPV6_UDP   ((0x1) << 7)
#define     TXGBE_VXMRQC_RSS_EN         ((0x1) << 8)
#define     TXGBE_VXMRQC_RSS_HASH(f)    ((0x7 & (f)) << 13)
#define TXGBE_VXRSSRK(i)        0x00080 + ((i) * 4) /* i=[0,9] */
#define TXGBE_VXRETA(i)         0x000C0 + ((i) * 4) /* i=[0,15] */
#define TXGBE_VXICR                 0x00100
#define   TXGBE_VXIC_MBOX         ((0x1) << 0)
#define   TXGBE_VXIC_DONE1        ((0x1) << 1)
#define   TXGBE_VXIC_DONE2        ((0x1) << 2)
#define TXGBE_VXICS                 0x00104
#define TXGBE_VXIMS                 0x00108
#define TXGBE_VXIMC                 0x0010C
#define TXGBE_VXLLI                 0x00118
#define TXGBE_VXITR(i)               0x00200 + (4 * (i)) /* i=[0,1] */
#define   TXGBE_VXITR_INTERVAL(f)    ((0x1FF & (f)) << 3)
#define   TXGBE_VXITR_LLI            ((0x1) << 15)
#define   TXGBE_VXITR_LLI_CREDIT(f)  ((0x1F & (f)) << 16)
#define   TXGBE_VXITR_CNT(f)         ((0x7F & (f)) << 21)
#define   TXGBE_VXITR_CNT_WDIS       ((0x1) << 31)
#define TXGBE_VXIVAR(i)              0x00240 + (4 * (i)) /* i=[0,3] */
#define   TXGBE_VXIVAR_ALLOC(i, f)   ((0x1 & (f)) << 8*(i))
#define   TXGBE_VXIVAR_VALID(i, f)   ((0x80 & (f)) << 8*(i))
#define TXGBE_VXIVAR_MISC            0x00260
#define   TXGBE_VXIVAR_MISC_ALLOC(f) ((0x3 & (f)))
#define   TXGBE_VXIVAR_MISC_VALID    ((0x80))

/* Receive Path */
#define TXGBE_VXRDBAL(r)        0x01000 + (0x40 * (r))
#define TXGBE_VXRDBAH(r)        0x01004 + (0x40 * (r))
#define TXGBE_VXRDT(r)          0x01008 + (0x40 * (r))
#define TXGBE_VXRDH(r)          0x0100C + (0x40 * (r))
#define TXGBE_VXRXDCTL(r)       0x01010 + (0x40 * (r))
#define   TXGBE_VXRXDCTL_ENABLE     ((0x1) << 0)
#define   TXGBE_VXRXDCTL_BUFLEN(f)  ((0x3F & (f)) << 1)
#define   TXGBE_VXRXDCTL_BUFSZ(f)   ((0xF & (f)) << 8)
#define   TXGBE_VXRXDCTL_HDRSZ(f)   ((0xF & (f)) << 12)
#define   TXGBE_VXRXDCTL_WTHRESH(f) ((0x7 & (f)) << 16)
#define   TXGBE_VXRXDCTL_ETAG       ((0x1) << 22)
#define   TXGBE_VXRXDCTL_RSCMAX(f)  ((0x3 & (f)) << 23)
#define     TXGBE_RSCMAX_1        (0)
#define     TXGBE_RSCMAX_4        (1)
#define     TXGBE_RSCMAX_8        (2)
#define     TXGBE_RSCMAX_16       (3)
#define   TXGBE_VXRXDCTL_STALL      ((0x1) << 25)
#define   TXGBE_VXRXDCTL_SPLIT      ((0x1) << 26)
#define   TXGBE_VXRXDCTL_RSCMODE    ((0x1) << 27)
#define   TXGBE_VXRXDCTL_CNTAG      ((0x1) << 28)
#define   TXGBE_VXRXDCTL_RSCEN      ((0x1) << 29)
#define   TXGBE_VXRXDCTL_DROP       ((0x1) << 30)
#define   TXGBE_VXRXDCTL_VLAN       ((0x1) << 31)

/* Transmit Path */
#define TXGBE_VXTDBAL(r)        0x03000 + (0x40 * (r))
#define TXGBE_VXTDBAH(r)        0x03004 + (0x40 * (r))
#define TXGBE_VXTDT(r)          0x03008 + (0x40 * (r))
#define TXGBE_VXTDH(r)          0x0300C + (0x40 * (r))
#define TXGBE_VXTXDCTL(r)           0x03010 + (0x40 * (r))
#define   TXGBE_VXTXDCTL_ENABLE     ((0x1) << 0)
#define   TXGBE_VXTXDCTL_BUFLEN(f)  ((0x3F & (f)) << 1)
#define   TXGBE_VXTXDCTL_PTHRESH(f) ((0xF & (f)) << 8)
#define   TXGBE_VXTXDCTL_WTHRESH(f) ((0x7F & (f)) << 16)
#define   TXGBE_VXTXDCTL_FLUSH      ((0x1) << 26)

#if 0
#define TXGBE_VXGPRC            0x01014
#define TXGBE_VXGORC_LSB        0x01018
#define TXGBE_VXGORC_MSB        0x0101C
#define TXGBE_VXMPRC            0x01020
#define TXGBE_VXGPTC            0x03014
#define TXGBE_VXGOTC_LSB        0x03018
#define TXGBE_VXGOTC_MSB        0x0301C
#endif

#define TXGBE_VXGPRC(r)            0x01014 + (0x40 * (r))
#define TXGBE_VXGORC_LSB(r)        0x01018 + (0x40 * (r))
#define TXGBE_VXGORC_MSB(r)        0x0101C + (0x40 * (r))
#define TXGBE_VXMPRC(r)            0x01020 + (0x40 * (r))
#define TXGBE_VXGPTC(r)            0x03014 + (0x40 * (r))
#define TXGBE_VXGOTC_LSB(r)        0x03018 + (0x40 * (r))
#define TXGBE_VXGOTC_MSB(r)        0x0301C + (0x40 * (r))
#endif /* _TXGBE_REGISTER_H_ */

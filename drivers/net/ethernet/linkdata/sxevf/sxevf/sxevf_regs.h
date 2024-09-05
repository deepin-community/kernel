
#ifndef __SXEVF_REGS_H__
#define __SXEVF_REGS_H__

#define SXEVF_REG_READ_FAIL  0xffffffffU
#define SXEVF_REG_READ_RETRY 5

#define SXE_VFLINKS_UP		0x00000008
#define SXE_VFLINKS_SPEED	0x00000006
#define SXE_VFLINKS_SPEED_10G	0x00000006
#define SXE_VFLINKS_SPEED_1G	0x00000004
#define SXE_VFLINKS_SPEED_100	0x00000002

#define SXE_VFCTRL        0x00000
#define SXE_VFSTATUS      0x00008
#define SXE_VFLINKS       0x00018
#define SXE_VFFRTIMER     0x00048
#define SXE_VFRXMEMWRAP   0x03190
#define SXE_VFEICR        0x00100
#define SXE_VFEICS        0x00104
#define SXE_VFEIMS        0x00108
#define SXE_VFEIMC        0x0010C
#define SXE_VFEIAM        0x00114
#define SXE_VFEITR(x)     (0x00820 + (4 * (x)))
#define SXE_VFIVAR(x)     (0x00120 + (4 * (x)))
#define SXE_VFIVAR_MISC    0x00140
#define SXE_VFRDBAL(x)    (0x01000 + (0x40 * (x)))
#define SXE_VFRDBAH(x)    (0x01004 + (0x40 * (x)))
#define SXE_VFRDLEN(x)    (0x01008 + (0x40 * (x)))
#define SXE_VFRDH(x)      (0x01010 + (0x40 * (x)))
#define SXE_VFRDT(x)      (0x01018 + (0x40 * (x)))
#define SXE_VFRXDCTL(x)   (0x01028 + (0x40 * (x)))
#define SXE_VFSRRCTL(x)   (0x01014 + (0x40 * (x)))
#define SXE_VFLROCTL(x)   (0x0102C + (0x40 * (x)))
#define SXE_VFPSRTYPE     0x00300
#define SXE_VFTDBAL(x)    (0x02000 + (0x40 * (x)))
#define SXE_VFTDBAH(x)    (0x02004 + (0x40 * (x)))
#define SXE_VFTDLEN(x)    (0x02008 + (0x40 * (x)))
#define SXE_VFTDH(x)      (0x02010 + (0x40 * (x)))
#define SXE_VFTDT(x)      (0x02018 + (0x40 * (x)))
#define SXE_VFTXDCTL(x)   (0x02028 + (0x40 * (x)))
#define SXE_VFTDWBAL(x)   (0x02038 + (0x40 * (x)))
#define SXE_VFTDWBAH(x)   (0x0203C + (0x40 * (x)))
#define SXE_VFDCA_RXCTRL(x)    (0x0100C + (0x40 * (x)))
#define SXE_VFDCA_TXCTRL(x)    (0x0200c + (0x40 * (x)))
#define SXE_VFGPRC        0x0101C
#define SXE_VFGPTC        0x0201C
#define SXE_VFGORC_LSB    0x01020
#define SXE_VFGORC_MSB    0x01024
#define SXE_VFGOTC_LSB    0x02020
#define SXE_VFGOTC_MSB    0x02024
#define SXE_VFMPRC        0x01034
#define SXE_VFMRQC        0x3000
#define SXE_VFRSSRK(x)    (0x3100 + ((x) * 4))
#define SXE_VFRETA(x)     (0x3200 + ((x) * 4))

#define SXEVF_VFEIMC_IRQ_MASK            (7)
#define SXEVF_IVAR_ALLOC_VALID    (0x80)

#define SXEVF_EITR_CNT_WDIS       (0x80000000)
#define SXEVF_EITR_ITR_MASK       (0x00000FF8)
#define SXEVF_EITR_ITR_SHIFT      (2)
#define SXEVF_EITR_ITR_MAX        (SXEVF_EITR_ITR_MASK >> SXEVF_EITR_ITR_SHIFT)

#define SXE_VFRXDCTL_ENABLE  0x02000000
#define SXE_VFTXDCTL_ENABLE  0x02000000
#define SXE_VFCTRL_RST       0x04000000

#define SXEVF_RXDCTL_ENABLE     0x02000000  
#define SXEVF_RXDCTL_VME	0x40000000  

#define SXEVF_PSRTYPE_RQPL_SHIFT               29 

#define SXEVF_SRRCTL_DROP_EN                   0x10000000
#define SXEVF_SRRCTL_DESCTYPE_DATA_ONEBUF      0x02000000
#define SXEVF_SRRCTL_BSIZEPKT_SHIFT            (10)
#define SXEVF_SRRCTL_BSIZEHDRSIZE_SHIFT        (2)
#define SXEVF_SRRCTL_BSIZEPKT_MASK	       0x0000007F
#define SXEVF_SRRCTL_BSIZEHDR_MASK	       0x00003F00

#define SXE_VFMAILBOX       0x002FC
#define SXE_VFMBMEM         0x00200

#define SXE_VFMAILBOX_REQ     0x00000001 
#define SXE_VFMAILBOX_ACK     0x00000002 
#define SXE_VFMAILBOX_VFU     0x00000004 
#define SXE_VFMAILBOX_PFU     0x00000008 
#define SXE_VFMAILBOX_PFSTS   0x00000010 
#define SXE_VFMAILBOX_PFACK   0x00000020 
#define SXE_VFMAILBOX_RSTI    0x00000040 
#define SXE_VFMAILBOX_RSTD    0x00000080 
#define SXE_VFMAILBOX_RC_BIT  0x000000B0  

#define SXEVF_TDBAL(_i)      (0x02000 + ((_i) * 0x40))
#define SXEVF_TDBAH(_i)      (0x02004 + ((_i) * 0x40))
#define SXEVF_TDLEN(_i)      (0x02008 + ((_i) * 0x40))
#define SXEVF_TDH(_i)        (0x02010 + ((_i) * 0x40))
#define SXEVF_TDT(_i)        (0x02018 + ((_i) * 0x40))
#define SXEVF_TXDCTL(_i)     (0x02028 + ((_i) * 0x40))
#define SXEVF_TDWBAL(_i)     (0x02038 + ((_i) * 0x40))
#define SXEVF_TDWBAH(_i)     (0x0203C + ((_i) * 0x40))

#define SXEVF_TXDCTL_SWFLSH  (0x02000000)  
#define SXEVF_TXDCTL_ENABLE  (0x02000000) 

#define SXEVF_VFGPRC          0x0101C
#define SXEVF_VFGPTC          0x0201C
#define SXEVF_VFGORC_LSB      0x01020
#define SXEVF_VFGORC_MSB      0x01024
#define SXEVF_VFGOTC_LSB      0x02020
#define SXEVF_VFGOTC_MSB      0x02024
#define SXEVF_VFMPRC          0x01034

#define SXEVF_EICR_MASK       0x07

#endif

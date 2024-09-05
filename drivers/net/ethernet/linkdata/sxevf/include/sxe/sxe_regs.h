 
#ifndef __SXE_REGS_H__
#define __SXE_REGS_H__

#define SXE_LINKSEC_MAX_SC_COUNT 1
#define SXE_LINKSEC_MAX_SA_COUNT 2

#define SXE_FLAGS_DOUBLE_RESET_REQUIRED	0x01


#define SXE_REG_READ_FAIL	0xffffffffU
#define SXE_REG_READ_RETRY	5
#ifdef SXE_TEST
#define SXE_PCI_MASTER_DISABLE_TIMEOUT	(1)
#else
#define SXE_PCI_MASTER_DISABLE_TIMEOUT	(800)
#endif


#define SXE_CTRL	0x00000 
#define SXE_STATUS	0x00008 
#define SXE_CTRL_EXT	0x00018 


#define SXE_CTRL_LNK_RST	0x00000008
#define SXE_CTRL_RST		0x04000000

#ifdef SXE_TEST
#define SXE_CTRL_RST_MASK	(0)
#define SXE_CTRL_GIO_DIS	(0)
#else
#define SXE_CTRL_RST_MASK	(SXE_CTRL_LNK_RST | SXE_CTRL_RST)
#define SXE_CTRL_GIO_DIS	0x00000004
#endif


#define SXE_STATUS_GIO		0x00080000


#define SXE_CTRL_EXT_PFRSTD	0x00004000
#define SXE_CTRL_EXT_NS_DIS	0x00010000
#define SXE_CTRL_EXT_DRV_LOAD	0x10000000


#define SXE_FCRTL(_i)	(0x03220 + ((_i) * 4))
#define SXE_FCRTH(_i)	(0x03260 + ((_i) * 4))
#define SXE_FCCFG	0x03D00


#define SXE_FCRTL_XONE		0x80000000
#define SXE_FCRTH_FCEN		0x80000000

#define SXE_FCCFG_TFCE_802_3X	0x00000008
#define SXE_FCCFG_TFCE_PRIORITY	0x00000010


#define SXE_GCR_EXT           0x11050 


#define SXE_GCR_CMPL_TMOUT_MASK		0x0000F000
#define SXE_GCR_CMPL_TMOUT_10ms		0x00001000
#define SXE_GCR_CMPL_TMOUT_RESEND	0x00010000
#define SXE_GCR_CAP_VER2		0x00040000
#define SXE_GCR_EXT_MSIX_EN		0x80000000
#define SXE_GCR_EXT_BUFFERS_CLEAR	0x40000000
#define SXE_GCR_EXT_VT_MODE_16		0x00000001
#define SXE_GCR_EXT_VT_MODE_32		0x00000002
#define SXE_GCR_EXT_VT_MODE_64		0x00000003
#define SXE_GCR_EXT_VT_MODE_MASK	0x00000003
#define SXE_GCR_EXT_SRIOV		(SXE_GCR_EXT_MSIX_EN | \
					SXE_GCR_EXT_VT_MODE_64)

#define SXE_PCI_DEVICE_STATUS		0x7A
#define SXE_PCI_DEVICE_STATUS_TRANSACTION_PENDING   0x0020
#define SXE_PCI_LINK_STATUS		0x82
#define SXE_PCI_DEVICE_CONTROL2		0x98
#define SXE_PCI_LINK_WIDTH		0x3F0
#define SXE_PCI_LINK_WIDTH_1		0x10
#define SXE_PCI_LINK_WIDTH_2		0x20
#define SXE_PCI_LINK_WIDTH_4		0x40
#define SXE_PCI_LINK_WIDTH_8		0x80
#define SXE_PCI_LINK_SPEED		0xF
#define SXE_PCI_LINK_SPEED_2500		0x1
#define SXE_PCI_LINK_SPEED_5000		0x2
#define SXE_PCI_LINK_SPEED_8000		0x3
#define SXE_PCI_HEADER_TYPE_REGISTER	0x0E
#define SXE_PCI_HEADER_TYPE_MULTIFUNC	0x80
#define SXE_PCI_DEVICE_CONTROL2_16ms	0x0005

#define SXE_PCIDEVCTRL2_TIMEO_MASK	0xf
#define SXE_PCIDEVCTRL2_16_32ms_def	0x0
#define SXE_PCIDEVCTRL2_50_100us	0x1
#define SXE_PCIDEVCTRL2_1_2ms		0x2
#define SXE_PCIDEVCTRL2_16_32ms		0x5
#define SXE_PCIDEVCTRL2_65_130ms	0x6
#define SXE_PCIDEVCTRL2_260_520ms	0x9
#define SXE_PCIDEVCTRL2_1_2s		0xa
#define SXE_PCIDEVCTRL2_4_8s		0xd
#define SXE_PCIDEVCTRL2_17_34s		0xe


#define SXE_EICR	0x00800
#define SXE_EICS	0x00808
#define SXE_EIMS	0x00880
#define SXE_EIMC	0x00888
#define SXE_EIAC	0x00810
#define SXE_EIAM	0x00890
#define SXE_EITRSEL	0x00894
#define SXE_GPIE	0x00898
#define SXE_IVAR(i)	(0x00900 + (i) * 4)
#define SXE_IVAR_MISC	0x00A00
#define SXE_EICS_EX(i)	(0x00A90 + (i) * 4)
#define SXE_EIMS_EX(i)	(0x00AA0 + (i) * 4)
#define SXE_EIMC_EX(i)	(0x00AB0 + (i) * 4)
#define SXE_EIAM_EX(i)	(0x00AD0 + (i) * 4)
#define SXE_EITR(i)	(((i) <= 23) ? (0x00820 + ((i) * 4)) : \
                        	(0x012300 + (((i) - 24) * 4)))

#define SXE_SPP_PROC	0x00AD8
#define SXE_SPP_STATE	0x00AF4



#define SXE_EICR_RTX_QUEUE	0x0000FFFF
#define SXE_EICR_FLOW_NAV	0x00010000
#define SXE_EICR_MAILBOX	0x00080000
#define SXE_EICR_LSC		0x00100000
#define SXE_EICR_LINKSEC	0x00200000
#define SXE_EICR_ECC		0x10000000
#define SXE_EICR_HDC		0x20000000
#define SXE_EICR_TCP_TIMER	0x40000000
#define SXE_EICR_OTHER		0x80000000


#define SXE_EICS_RTX_QUEUE	SXE_EICR_RTX_QUEUE
#define SXE_EICS_FLOW_NAV	SXE_EICR_FLOW_NAV 
#define SXE_EICS_MAILBOX	SXE_EICR_MAILBOX  
#define SXE_EICS_LSC		SXE_EICR_LSC      
#define SXE_EICS_ECC		SXE_EICR_ECC      
#define SXE_EICS_HDC		SXE_EICR_HDC      
#define SXE_EICS_TCP_TIMER	SXE_EICR_TCP_TIMER
#define SXE_EICS_OTHER		SXE_EICR_OTHER    


#define SXE_EIMS_RTX_QUEUE	SXE_EICR_RTX_QUEUE
#define SXE_EIMS_FLOW_NAV	SXE_EICR_FLOW_NAV
#define SXE_EIMS_MAILBOX	SXE_EICR_MAILBOX
#define SXE_EIMS_LSC		SXE_EICR_LSC
#define SXE_EIMS_ECC		SXE_EICR_ECC
#define SXE_EIMS_HDC		SXE_EICR_HDC
#define SXE_EIMS_TCP_TIMER	SXE_EICR_TCP_TIMER
#define SXE_EIMS_OTHER		SXE_EICR_OTHER
#define SXE_EIMS_ENABLE_MASK	(SXE_EIMS_RTX_QUEUE | SXE_EIMS_LSC | \
					SXE_EIMS_TCP_TIMER | SXE_EIMS_OTHER)

#define SXE_EIMC_FLOW_NAV	SXE_EICR_FLOW_NAV 
#define SXE_EIMC_LSC		SXE_EICR_LSC      
#define SXE_EIMC_HDC		SXE_EICR_HDC      


#define SXE_GPIE_SPP0_EN	0x00000001
#define SXE_GPIE_SPP1_EN	0x00000002
#define SXE_GPIE_SPP2_EN	0x00000004
#define SXE_GPIE_MSIX_MODE	0x00000010
#define SXE_GPIE_OCD		0x00000020
#define SXE_GPIE_EIMEN		0x00000040
#define SXE_GPIE_EIAME		0x40000000
#define SXE_GPIE_PBA_SUPPORT	0x80000000
#define SXE_GPIE_VTMODE_MASK	0x0000C000
#define SXE_GPIE_VTMODE_16	0x00004000
#define SXE_GPIE_VTMODE_32	0x00008000
#define SXE_GPIE_VTMODE_64	0x0000C000


#define SXE_IVAR_ALLOC_VALID	0x80


#define SXE_EITR_CNT_WDIS	0x80000000
#define SXE_EITR_ITR_MASK	0x00000FF8
#define SXE_EITR_ITR_SHIFT	2
#define SXE_EITR_ITR_MAX	(SXE_EITR_ITR_MASK >> SXE_EITR_ITR_SHIFT)


#define SXE_EICR_GPI_SPP0	0x01000000
#define SXE_EICR_GPI_SPP1	0x02000000
#define SXE_EICR_GPI_SPP2	0x04000000
#define SXE_EIMS_GPI_SPP0	SXE_EICR_GPI_SPP0
#define SXE_EIMS_GPI_SPP1	SXE_EICR_GPI_SPP1
#define SXE_EIMS_GPI_SPP2	SXE_EICR_GPI_SPP2


#define SXE_SPP_PROC_SPP2_TRIGGER	0x00300000
#define SXE_SPP_PROC_SPP2_TRIGGER_MASK	0xFFCFFFFF
#define SXE_SPP_PROC_DELAY_US_MASK	0x0000FFFF
#define SXE_SPP_PROC_DELAY_US		0x00000007


#define SXE_IRQ_CLEAR_MASK	0xFFFFFFFF


#define SXE_RXCSUM		0x05000
#define SXE_RFCTL		0x05008
#define SXE_FCTRL		0x05080
#define SXE_EXVET               0x05078
#define SXE_VLNCTRL		0x05088
#define SXE_MCSTCTRL		0x05090
#define SXE_ETQF(_i)		(0x05128 + ((_i) * 4))
#define SXE_ETQS(_i)		(0x0EC00 + ((_i) * 4))
#define SXE_SYNQF		0x0EC30
#define SXE_MTA(_i)		(0x05200 + ((_i) * 4))
#define SXE_UTA(_i)		(0x0F400 + ((_i) * 4))
#define SXE_VFTA(_i)		(0x0A000 + ((_i) * 4))
#define SXE_RAL(_i)		(0x0A200 + ((_i) * 8))
#define SXE_RAH(_i)		(0x0A204 + ((_i) * 8))
#define SXE_MPSAR_LOW(_i)	(0x0A600 + ((_i) * 8))
#define SXE_MPSAR_HIGH(_i)	(0x0A604 + ((_i) * 8))
#define SXE_PSRTYPE(_i)		(0x0EA00 + ((_i) * 4))
#define SXE_RETA(_i)		(0x0EB00 + ((_i) * 4)) 
#define SXE_RSSRK(_i)		(0x0EB80 + ((_i) * 4)) 
#define SXE_RQTC		0x0EC70
#define SXE_MRQC		0x0EC80
#define SXE_IEOI		0x0F654
#define SXE_PL			0x0F658
#define SXE_LPL			0x0F65C


#define SXE_ETQF_CNT			8
#define SXE_MTA_CNT				128
#define SXE_UTA_CNT				128
#define SXE_VFTA_CNT			128
#define SXE_RAR_CNT				128
#define SXE_MPSAR_CNT			128


#define SXE_EXVET_DEFAULT		0x81000000
#define SXE_VLNCTRL_DEFAULT		0x8100
#define SXE_IEOI_DEFAULT		0x060005DC
#define SXE_PL_DEFAULT			0x3e000016
#define SXE_LPL_DEFAULT			0x26000000


#define SXE_RXCSUM_IPPCSE	0x00001000  
#define SXE_RXCSUM_PCSD		0x00002000  


#define SXE_RFCTL_LRO_DIS	0x00000020
#define SXE_RFCTL_NFSW_DIS	0x00000040
#define SXE_RFCTL_NFSR_DIS	0x00000080


#define SXE_FCTRL_SBP		0x00000002
#define SXE_FCTRL_MPE		0x00000100
#define SXE_FCTRL_UPE		0x00000200
#define SXE_FCTRL_BAM		0x00000400
#define SXE_FCTRL_PMCF		0x00001000
#define SXE_FCTRL_DPF		0x00002000


#define SXE_VLNCTRL_VET		0x0000FFFF 
#define SXE_VLNCTRL_CFI		0x10000000 
#define SXE_VLNCTRL_CFIEN	0x20000000 
#define SXE_VLNCTRL_VFE		0x40000000 
#define SXE_VLNCTRL_VME		0x80000000 

#define SXE_EXVET_VET_EXT_SHIFT              16
#define SXE_EXTENDED_VLAN	             (1 << 26)


#define SXE_MCSTCTRL_MFE	4

#define SXE_ETQF_FILTER_EAPOL	0
#define SXE_ETQF_FILTER_1588	3
#define SXE_ETQF_FILTER_FIP	4
#define SXE_ETQF_FILTER_LLDP	5
#define SXE_ETQF_FILTER_LACP	6
#define SXE_ETQF_FILTER_FC	7
#define SXE_MAX_ETQF_FILTERS	8
#define SXE_ETQF_1588		0x40000000
#define SXE_ETQF_FILTER_EN	0x80000000
#define SXE_ETQF_POOL_ENABLE	BIT(26)
#define SXE_ETQF_POOL_SHIFT	20


#define SXE_ETQS_RX_QUEUE	0x007F0000
#define SXE_ETQS_RX_QUEUE_SHIFT	16
#define SXE_ETQS_LLI		0x20000000
#define SXE_ETQS_QUEUE_EN	0x80000000


#define SXE_SYN_FILTER_ENABLE         0x00000001
#define SXE_SYN_FILTER_QUEUE          0x000000FE
#define SXE_SYN_FILTER_QUEUE_SHIFT    1
#define SXE_SYN_FILTER_SYNQFP         0x80000000


#define SXE_RAH_VIND_MASK	0x003C0000
#define SXE_RAH_VIND_SHIFT	18
#define SXE_RAH_AV		0x80000000
#define SXE_CLEAR_VMDQ_ALL	0xFFFFFFFF


#define SXE_PSRTYPE_TCPHDR	0x00000010
#define SXE_PSRTYPE_UDPHDR	0x00000020
#define SXE_PSRTYPE_IPV4HDR	0x00000100
#define SXE_PSRTYPE_IPV6HDR	0x00000200
#define SXE_PSRTYPE_L2HDR	0x00001000


#define SXE_MRQC_RSSEN                 0x00000001 
#define SXE_MRQC_MRQE_MASK                    0xF
#define SXE_MRQC_RT8TCEN               0x00000002
#define SXE_MRQC_RT4TCEN               0x00000003
#define SXE_MRQC_RTRSS8TCEN            0x00000004
#define SXE_MRQC_RTRSS4TCEN            0x00000005
#define SXE_MRQC_VMDQEN                0x00000008
#define SXE_MRQC_VMDQRSS32EN           0x0000000A
#define SXE_MRQC_VMDQRSS64EN           0x0000000B
#define SXE_MRQC_VMDQRT8TCEN           0x0000000C
#define SXE_MRQC_VMDQRT4TCEN           0x0000000D
#define SXE_MRQC_RSS_FIELD_MASK        0xFFFF0000
#define SXE_MRQC_RSS_FIELD_IPV4_TCP    0x00010000
#define SXE_MRQC_RSS_FIELD_IPV4        0x00020000
#define SXE_MRQC_RSS_FIELD_IPV6_EX_TCP 0x00040000
#define SXE_MRQC_RSS_FIELD_IPV6_EX     0x00080000
#define SXE_MRQC_RSS_FIELD_IPV6        0x00100000
#define SXE_MRQC_RSS_FIELD_IPV6_TCP    0x00200000
#define SXE_MRQC_RSS_FIELD_IPV4_UDP    0x00400000
#define SXE_MRQC_RSS_FIELD_IPV6_UDP    0x00800000
#define SXE_MRQC_RSS_FIELD_IPV6_EX_UDP 0x01000000


#define SXE_RDBAL(_i)		(((_i) < 64) ? (0x01000 + ((_i) * 0x40)) : \
					(0x0D000 + (((_i) - 64) * 0x40)))
#define SXE_RDBAH(_i)		(((_i) < 64) ? (0x01004 + ((_i) * 0x40)) : \
					(0x0D004 + (((_i) - 64) * 0x40)))
#define SXE_RDLEN(_i)		(((_i) < 64) ? (0x01008 + ((_i) * 0x40)) : \
					(0x0D008 + (((_i) - 64) * 0x40)))
#define SXE_RDH(_i)		(((_i) < 64) ? (0x01010 + ((_i) * 0x40)) : \
					(0x0D010 + (((_i) - 64) * 0x40)))
#define SXE_SRRCTL(_i)		(((_i) < 64) ? (0x01014 + ((_i) * 0x40)) : \
					(0x0D014 + (((_i) - 64) * 0x40)))
#define SXE_RDT(_i)		(((_i) < 64) ? (0x01018 + ((_i) * 0x40)) : \
					(0x0D018 + (((_i) - 64) * 0x40)))
#define SXE_RXDCTL(_i)		(((_i) < 64) ? (0x01028 + ((_i) * 0x40)) : \
					(0x0D028 + (((_i) - 64) * 0x40)))
#define SXE_LROCTL(_i)		(((_i) < 64) ? (0x0102C + ((_i) * 0x40)) : \
					(0x0D02C + (((_i) - 64) * 0x40)))
#define SXE_RDRXCTL		0x02F00  
#define SXE_RXCTRL		0x03000 
#define SXE_LRODBU 		0x03028  
#define SXE_RXPBSIZE(_i)	(0x03C00 + ((_i) * 4))

#define SXE_DRXCFG		(0x03C20)


#define SXE_RXDCTL_CNT			128


#define SXE_RXDCTL_DEFAULT		0x40210


#define SXE_SRRCTL_DROP_EN		0x10000000
#define SXE_SRRCTL_BSIZEPKT_SHIFT	(10)
#define SXE_SRRCTL_BSIZEHDRSIZE_SHIFT	(2)
#define SXE_SRRCTL_DESCTYPE_DATA_ONEBUF	0x02000000
#define SXE_SRRCTL_BSIZEPKT_MASK	0x0000007F
#define SXE_SRRCTL_BSIZEHDR_MASK	0x00003F00


#define SXE_RXDCTL_ENABLE	0x02000000 
#define SXE_RXDCTL_SWFLSH	0x04000000 
#define SXE_RXDCTL_VME		0x40000000 
#define SXE_RXDCTL_DESC_FIFO_AE_TH_SHIFT	8
#define SXE_RXDCTL_PREFETCH_NUM_CFG_SHIFT	16


#define SXE_LROCTL_LROEN	0x01
#define SXE_LROCTL_MAXDESC_1	0x00
#define SXE_LROCTL_MAXDESC_4	0x04
#define SXE_LROCTL_MAXDESC_8	0x08
#define SXE_LROCTL_MAXDESC_16	0x0C


#define SXE_RDRXCTL_RDMTS_1_2	0x00000000
#define SXE_RDRXCTL_RDMTS_EN	0x00200000
#define SXE_RDRXCTL_CRCSTRIP	0x00000002
#define SXE_RDRXCTL_PSP		0x00000004
#define SXE_RDRXCTL_MVMEN	0x00000020
#define SXE_RDRXCTL_DMAIDONE	0x00000008
#define SXE_RDRXCTL_AGGDIS	0x00010000
#define SXE_RDRXCTL_LROFRSTSIZE	0x003E0000
#define SXE_RDRXCTL_LROLLIDIS	0x00800000
#define SXE_RDRXCTL_LROACKC	0x02000000
#define SXE_RDRXCTL_FCOE_WRFIX	0x04000000
#define SXE_RDRXCTL_MBINTEN	0x10000000
#define SXE_RDRXCTL_MDP_EN	0x20000000
#define SXE_RDRXCTL_MPBEN	0x00000010

#define SXE_RDRXCTL_MCEN	0x00000040



#define SXE_RXCTRL_RXEN		0x00000001


#define SXE_LRODBU_LROACKDIS	0x00000080


#define SXE_DRXCFG_GSP_ZERO    0x00000002
#define SXE_DRXCFG_DBURX_START 0x00000001


#define SXE_DMATXCTL		0x04A80   
#define SXE_TDBAL(_i)		(0x06000 + ((_i) * 0x40))
#define SXE_TDBAH(_i)		(0x06004 + ((_i) * 0x40))
#define SXE_TDLEN(_i)		(0x06008 + ((_i) * 0x40))
#define SXE_TDH(_i)		(0x06010 + ((_i) * 0x40))
#define SXE_TDT(_i)		(0x06018 + ((_i) * 0x40))
#define SXE_TXDCTL(_i)		(0x06028 + ((_i) * 0x40))
#define SXE_PVFTDWBAL(p)	(0x06038 + (0x40 * (p)))
#define SXE_PVFTDWBAH(p)	(0x0603C + (0x40 * (p)))
#define SXE_TXPBSIZE(_i)	(0x0CC00 + ((_i) * 4))
#define SXE_TXPBTHRESH(_i)	(0x04950 + ((_i) * 4))
#define SXE_MTQC		0x08120               
#define SXE_TXPBFCS		0x0CE00               
#define SXE_DTXCFG		0x0CE08               
#define SXE_DTMPCNT		0x0CE98               


#define SXE_DMATXCTL_DEFAULT		0x81000000


#define SXE_DMATXCTL_TE		0x1       
#define SXE_DMATXCTL_GDV	0x8       
#define SXE_DMATXCTL_VT_SHIFT	16        
#define SXE_DMATXCTL_VT_MASK    0xFFFF0000


#define SXE_TXDCTL_HTHRESH_SHIFT 8
#define SXE_TXDCTL_WTHRESH_SHIFT 16
#define SXE_TXDCTL_ENABLE     0x02000000
#define SXE_TXDCTL_SWFLSH     0x04000000

#define SXE_PVFTDWBAL_N(ring_per_pool, vf_idx, vf_ring_idx) \
		SXE_PVFTDWBAL((ring_per_pool) * (vf_idx) + vf_ring_idx)
#define SXE_PVFTDWBAH_N(ring_per_pool, vf_idx, vf_ring_idx) \
		SXE_PVFTDWBAH((ring_per_pool) * (vf_idx) + vf_ring_idx)


#define SXE_MTQC_RT_ENA		0x1
#define SXE_MTQC_VT_ENA		0x2
#define SXE_MTQC_64Q_1PB	0x0
#define SXE_MTQC_32VF		0x8
#define SXE_MTQC_64VF		0x4
#define SXE_MTQC_8TC_8TQ	0xC
#define SXE_MTQC_4TC_4TQ	0x8


#define SXE_TFCS_PB0_MASK	0x1
#define SXE_TFCS_PB1_MASK	0x2
#define SXE_TFCS_PB2_MASK	0x4
#define SXE_TFCS_PB3_MASK	0x8
#define SXE_TFCS_PB4_MASK	0x10
#define SXE_TFCS_PB5_MASK	0x20
#define SXE_TFCS_PB6_MASK	0x40
#define SXE_TFCS_PB7_MASK	0x80
#define SXE_TFCS_PB_MASK	0xff


#define SXE_DTXCFG_DBUTX_START	0x00000001   
#define SXE_DTXCFG_DBUTX_BUF_ALFUL_CFG	0x20


#define SXE_RTRPCS		0x02430
#define SXE_RTRPT4C(_i)		(0x02140 + ((_i) * 4))
#define SXE_RTRUP2TC		0x03020
#define SXE_RTTDCS		0x04900
#define SXE_RTTDQSEL		0x04904
#define SXE_RTTDT1C		0x04908
#define SXE_RTTDT2C(_i)		(0x04910 + ((_i) * 4))
#define SXE_RTTBCNRM		0x04980
#define SXE_RTTBCNRC		0x04984
#define SXE_RTTUP2TC		0x0C800
#define SXE_RTTPCS		0x0CD00
#define SXE_RTTPT2C(_i)		(0x0CD20 + ((_i) * 4))


#define SXE_RTRPCS_RRM		0x00000002
#define SXE_RTRPCS_RAC		0x00000004
#define SXE_RTRPCS_ARBDIS	0x00000040


#define SXE_RTRPT4C_MCL_SHIFT	12
#define SXE_RTRPT4C_BWG_SHIFT	9 
#define SXE_RTRPT4C_GSP		0x40000000
#define SXE_RTRPT4C_LSP		0x80000000


#define SXE_RTRUP2TC_UP_SHIFT 3
#define SXE_RTRUP2TC_UP_MASK	7


#define SXE_RTTDCS_ARBDIS	0x00000040
#define SXE_RTTDCS_TDPAC	0x00000001

#define SXE_RTTDCS_VMPAC	0x00000002

#define SXE_RTTDCS_TDRM		0x00000010
#define SXE_RTTDCS_ARBDIS	0x00000040
#define SXE_RTTDCS_BDPM		0x00400000
#define SXE_RTTDCS_BPBFSM	0x00800000

#define SXE_RTTDCS_SPEED_CHG	0x80000000


#define SXE_RTTDT2C_MCL_SHIFT	12
#define SXE_RTTDT2C_BWG_SHIFT	9
#define SXE_RTTDT2C_GSP		0x40000000
#define SXE_RTTDT2C_LSP		0x80000000


#define SXE_RTTBCNRC_RS_ENA		0x80000000
#define SXE_RTTBCNRC_RF_DEC_MASK	0x00003FFF
#define SXE_RTTBCNRC_RF_INT_SHIFT	14
#define SXE_RTTBCNRC_RF_INT_MASK	\
			(SXE_RTTBCNRC_RF_DEC_MASK << SXE_RTTBCNRC_RF_INT_SHIFT)


#define SXE_RTTUP2TC_UP_SHIFT	3


#define SXE_RTTPCS_TPPAC	0x00000020

#define SXE_RTTPCS_ARBDIS	0x00000040
#define SXE_RTTPCS_TPRM		0x00000100
#define SXE_RTTPCS_ARBD_SHIFT	22
#define SXE_RTTPCS_ARBD_DCB	0x4       


#define SXE_RTTPT2C_MCL_SHIFT 12
#define SXE_RTTPT2C_BWG_SHIFT 9
#define SXE_RTTPT2C_GSP       0x40000000
#define SXE_RTTPT2C_LSP       0x80000000


#define SXE_TPH_CTRL		0x11074
#define SXE_TPH_TXCTRL(_i)      (0x0600C + ((_i) * 0x40))
#define SXE_TPH_RXCTRL(_i)	(((_i) < 64) ? (0x0100C + ((_i) * 0x40)) : \
				 (0x0D00C + (((_i) - 64) * 0x40)))


#define SXE_TPH_CTRL_ENABLE		0x00000000
#define SXE_TPH_CTRL_DISABLE		0x00000001
#define SXE_TPH_CTRL_MODE_CB1		0x00      
#define SXE_TPH_CTRL_MODE_CB2		0x02      


#define SXE_TPH_RXCTRL_DESC_TPH_EN	BIT(5) 
#define SXE_TPH_RXCTRL_HEAD_TPH_EN	BIT(6) 
#define SXE_TPH_RXCTRL_DATA_TPH_EN	BIT(7) 
#define SXE_TPH_RXCTRL_DESC_RRO_EN	BIT(9) 
#define SXE_TPH_RXCTRL_DATA_WRO_EN	BIT(13)
#define SXE_TPH_RXCTRL_HEAD_WRO_EN	BIT(15)
#define SXE_TPH_RXCTRL_CPUID_SHIFT	24     

#define SXE_TPH_TXCTRL_DESC_TPH_EN	BIT(5) 
#define SXE_TPH_TXCTRL_DESC_RRO_EN	BIT(9) 
#define SXE_TPH_TXCTRL_DESC_WRO_EN	BIT(11)
#define SXE_TPH_TXCTRL_DATA_RRO_EN	BIT(13)
#define SXE_TPH_TXCTRL_CPUID_SHIFT	24     


#define SXE_SECTXCTRL		0x08800
#define SXE_SECTXSTAT		0x08804
#define SXE_SECTXBUFFAF		0x08808
#define SXE_SECTXMINIFG		0x08810
#define SXE_SECRXCTRL		0x08D00
#define SXE_SECRXSTAT		0x08D04
#define SXE_LSECTXCTRL            0x08A04
#define SXE_LSECTXSCL             0x08A08
#define SXE_LSECTXSCH             0x08A0C
#define SXE_LSECTXSA              0x08A10
#define SXE_LSECTXPN(_n)          (0x08A14 + (4 * (_n)))
#define SXE_LSECTXKEY(_n, _m)     (0x08A1C + ((0x10 * (_n)) + (4 * (_m))))
#define SXE_LSECRXCTRL            0x08B04
#define SXE_LSECRXSCL             0x08B08
#define SXE_LSECRXSCH             0x08B0C
#define SXE_LSECRXSA(_i)          (0x08B10 + (4 * (_i)))
#define SXE_LSECRXPN(_i)          (0x08B18 + (4 * (_i)))
#define SXE_LSECRXKEY(_n, _m)     (0x08B20 + ((0x10 * (_n)) + (4 * (_m))))  


#define SXE_SECTXCTRL_SECTX_DIS		0x00000001
#define SXE_SECTXCTRL_TX_DIS		0x00000002
#define SXE_SECTXCTRL_STORE_FORWARD	0x00000004


#define SXE_SECTXSTAT_SECTX_RDY		0x00000001
#define SXE_SECTXSTAT_SECTX_OFF_DIS	0x00000002
#define SXE_SECTXSTAT_ECC_TXERR		0x00000004


#define SXE_SECRXCTRL_SECRX_DIS		0x00000001
#define SXE_SECRXCTRL_RX_DIS		0x00000002
#define SXE_SECRXCTRL_RP              0x00000080


#define SXE_SECRXSTAT_SECRX_RDY		0x00000001
#define SXE_SECRXSTAT_SECRX_OFF_DIS	0x00000002
#define SXE_SECRXSTAT_ECC_RXERR		0x00000004

#define SXE_SECTX_DCB_ENABLE_MASK	0x00001F00

#define SXE_LSECTXCTRL_EN_MASK        0x00000003
#define SXE_LSECTXCTRL_EN_SHIFT       0
#define SXE_LSECTXCTRL_ES             0x00000010
#define SXE_LSECTXCTRL_AISCI          0x00000020
#define SXE_LSECTXCTRL_PNTHRSH_MASK   0xFFFFFF00
#define SXE_LSECTXCTRL_PNTHRSH_SHIFT  8
#define SXE_LSECTXCTRL_RSV_MASK       0x000000D8

#define SXE_LSECRXCTRL_EN_MASK        0x0000000C
#define SXE_LSECRXCTRL_EN_SHIFT       2
#define SXE_LSECRXCTRL_DROP_EN        0x00000010
#define SXE_LSECRXCTRL_DROP_EN_SHIFT  4
#define SXE_LSECRXCTRL_PLSH           0x00000040
#define SXE_LSECRXCTRL_PLSH_SHIFT     6
#define SXE_LSECRXCTRL_RP             0x00000080
#define SXE_LSECRXCTRL_RP_SHIFT       7
#define SXE_LSECRXCTRL_RSV_MASK       0xFFFFFF33

#define SXE_LSECTXSA_AN0_MASK         0x00000003
#define SXE_LSECTXSA_AN0_SHIFT        0
#define SXE_LSECTXSA_AN1_MASK         0x0000000C
#define SXE_LSECTXSA_AN1_SHIFT        2
#define SXE_LSECTXSA_SELSA            0x00000010
#define SXE_LSECTXSA_SELSA_SHIFT      4
#define SXE_LSECTXSA_ACTSA            0x00000020

#define SXE_LSECRXSA_AN_MASK          0x00000003
#define SXE_LSECRXSA_AN_SHIFT         0
#define SXE_LSECRXSA_SAV              0x00000004
#define SXE_LSECRXSA_SAV_SHIFT        2
#define SXE_LSECRXSA_RETIRED          0x00000010
#define SXE_LSECRXSA_RETIRED_SHIFT    4

#define SXE_LSECRXSCH_PI_MASK         0xFFFF0000
#define SXE_LSECRXSCH_PI_SHIFT        16

#define SXE_LSECTXCTRL_DISABLE	0x0
#define SXE_LSECTXCTRL_AUTH		0x1
#define SXE_LSECTXCTRL_AUTH_ENCRYPT	0x2

#define SXE_LSECRXCTRL_DISABLE	0x0
#define SXE_LSECRXCTRL_CHECK		0x1
#define SXE_LSECRXCTRL_STRICT		0x2
#define SXE_LSECRXCTRL_DROP		0x3
#define SXE_SECTXCTRL_STORE_FORWARD_ENABLE    0x4



#define SXE_IPSTXIDX		0x08900
#define SXE_IPSTXSALT		0x08904
#define SXE_IPSTXKEY(_i)	(0x08908 + (4 * (_i)))
#define SXE_IPSRXIDX		0x08E00
#define SXE_IPSRXIPADDR(_i)	(0x08E04 + (4 * (_i)))
#define SXE_IPSRXSPI		0x08E14
#define SXE_IPSRXIPIDX		0x08E18
#define SXE_IPSRXKEY(_i)	(0x08E1C + (4 * (_i)))
#define SXE_IPSRXSALT		0x08E2C
#define SXE_IPSRXMOD		0x08E30



#define SXE_FNAVCTRL		0x0EE00
#define SXE_FNAVHKEY		0x0EE68
#define SXE_FNAVSKEY		0x0EE6C
#define SXE_FNAVDIP4M		0x0EE3C
#define SXE_FNAVSIP4M		0x0EE40
#define SXE_FNAVTCPM		0x0EE44
#define SXE_FNAVUDPM		0x0EE48
#define SXE_FNAVIP6M		0x0EE74
#define SXE_FNAVM		0x0EE70

#define SXE_FNAVFREE		0x0EE38
#define SXE_FNAVLEN		0x0EE4C
#define SXE_FNAVUSTAT		0x0EE50
#define SXE_FNAVFSTAT		0x0EE54
#define SXE_FNAVMATCH		0x0EE58
#define SXE_FNAVMISS		0x0EE5C

#define SXE_FNAVSIPv6(_i)	(0x0EE0C + ((_i) * 4))
#define SXE_FNAVIPSA		0x0EE18
#define SXE_FNAVIPDA		0x0EE1C
#define SXE_FNAVPORT		0x0EE20
#define SXE_FNAVVLAN		0x0EE24
#define SXE_FNAVHASH		0x0EE28
#define SXE_FNAVCMD		0x0EE2C


#define SXE_FNAVCTRL_FLEX_SHIFT			16
#define SXE_FNAVCTRL_MAX_LENGTH_SHIFT		24
#define SXE_FNAVCTRL_FULL_THRESH_SHIFT		28
#define SXE_FNAVCTRL_DROP_Q_SHIFT		8
#define SXE_FNAVCTRL_PBALLOC_64K		0x00000001
#define SXE_FNAVCTRL_PBALLOC_128K		0x00000002
#define SXE_FNAVCTRL_PBALLOC_256K		0x00000003
#define SXE_FNAVCTRL_INIT_DONE			0x00000008
#define SXE_FNAVCTRL_SPECIFIC_MATCH		0x00000010
#define SXE_FNAVCTRL_REPORT_STATUS		0x00000020
#define SXE_FNAVCTRL_REPORT_STATUS_ALWAYS	0x00000080

#define SXE_FNAVCTRL_FLEX_MASK			(0x1F << SXE_FNAVCTRL_FLEX_SHIFT)

#define SXE_FNAVTCPM_DPORTM_SHIFT		16

#define SXE_FNAVM_VLANID			0x00000001
#define SXE_FNAVM_VLANP				0x00000002
#define SXE_FNAVM_POOL				0x00000004
#define SXE_FNAVM_L4P				0x00000008
#define SXE_FNAVM_FLEX				0x00000010
#define SXE_FNAVM_DIPv6				0x00000020

#define SXE_FNAVPORT_DESTINATION_SHIFT		16
#define SXE_FNAVVLAN_FLEX_SHIFT			16
#define SXE_FNAVHASH_SIG_SW_INDEX_SHIFT		16

#define SXE_FNAVCMD_CMD_MASK			0x00000003
#define SXE_FNAVCMD_CMD_ADD_FLOW		0x00000001
#define SXE_FNAVCMD_CMD_REMOVE_FLOW		0x00000002
#define SXE_FNAVCMD_CMD_QUERY_REM_FILT		0x00000003
#define SXE_FNAVCMD_FILTER_VALID		0x00000004
#define SXE_FNAVCMD_FILTER_UPDATE		0x00000008
#define SXE_FNAVCMD_IPv6DMATCH			0x00000010
#define SXE_FNAVCMD_L4TYPE_UDP			0x00000020
#define SXE_FNAVCMD_L4TYPE_TCP			0x00000040
#define SXE_FNAVCMD_L4TYPE_SCTP			0x00000060
#define SXE_FNAVCMD_IPV6			0x00000080
#define SXE_FNAVCMD_CLEARHT			0x00000100
#define SXE_FNAVCMD_DROP			0x00000200
#define SXE_FNAVCMD_INT				0x00000400
#define SXE_FNAVCMD_LAST			0x00000800
#define SXE_FNAVCMD_COLLISION			0x00001000
#define SXE_FNAVCMD_QUEUE_EN			0x00008000
#define SXE_FNAVCMD_FLOW_TYPE_SHIFT		5
#define SXE_FNAVCMD_RX_QUEUE_SHIFT		16
#define SXE_FNAVCMD_RX_TUNNEL_FILTER_SHIFT	23
#define SXE_FNAVCMD_VT_POOL_SHIFT		24
#define SXE_FNAVCMD_CMD_POLL			10
#define SXE_FNAVCMD_TUNNEL_FILTER		0x00800000


#define SXE_LXOFFRXCNT		0x041A8
#define SXE_PXOFFRXCNT(_i)	(0x04160 + ((_i) * 4))

#define SXE_EPC_GPRC		0x050E0
#define SXE_RXDGPC              0x02F50
#define SXE_RXDGBCL             0x02F54
#define SXE_RXDGBCH             0x02F58
#define SXE_RXDDGPC             0x02F5C
#define SXE_RXDDGBCL            0x02F60
#define SXE_RXDDGBCH            0x02F64
#define SXE_RXLPBKGPC           0x02F68
#define SXE_RXLPBKGBCL          0x02F6C
#define SXE_RXLPBKGBCH          0x02F70
#define SXE_RXDLPBKGPC          0x02F74
#define SXE_RXDLPBKGBCL         0x02F78
#define SXE_RXDLPBKGBCH         0x02F7C

#define SXE_RXTPCIN             0x02F88
#define SXE_RXTPCOUT            0x02F8C
#define SXE_RXPRDDC             0x02F9C

#define SXE_TXDGPC		0x087A0
#define SXE_TXDGBCL             0x087A4
#define SXE_TXDGBCH             0x087A8
#define SXE_TXSWERR             0x087B0
#define SXE_TXSWITCH            0x087B4
#define SXE_TXREPEAT            0x087B8
#define SXE_TXDESCERR           0x087BC
#define SXE_MNGPRC		0x040B4
#define SXE_MNGPDC		0x040B8
#define SXE_RQSMR(_i)		(0x02300 + ((_i) * 4))   
#define SXE_TQSM(_i)		(0x08600 + ((_i) * 4))   
#define SXE_QPRC(_i)		(0x01030 + ((_i) * 0x40))
#define SXE_QBRC_L(_i)		(0x01034 + ((_i) * 0x40))
#define SXE_QBRC_H(_i)		(0x01038 + ((_i) * 0x40))


#define SXE_QPRDC(_i)		(0x01430 + ((_i) * 0x40))
#define SXE_QPTC(_i)		(0x08680 + ((_i) * 0x4))
#define SXE_QBTC_L(_i)		(0x08700 + ((_i) * 0x8)) 
#define SXE_QBTC_H(_i)		(0x08704 + ((_i) * 0x8)) 
#define SXE_SSVPC		0x08780                  
#define SXE_MNGPTC		0x0CF90
#define SXE_MPC(_i)		(0x03FA0 + ((_i) * 4))

#define SXE_DBUDRTCICNT(_i)	(0x03C6C + ((_i) * 4))
#define SXE_DBUDRTCOCNT(_i)	(0x03C8C + ((_i) * 4))
#define SXE_DBUDRBDPCNT(_i)	(0x03D20 + ((_i) * 4))
#define SXE_DBUDREECNT(_i)	(0x03D40 + ((_i) * 4))
#define SXE_DBUDROFPCNT(_i)	(0x03D60 + ((_i) * 4))
#define SXE_DBUDTTCICNT(_i)	(0x0CE54 + ((_i) * 4))
#define SXE_DBUDTTCOCNT(_i)	(0x0CE74 + ((_i) * 4))



#define SXE_WUC                       0x05800
#define SXE_WUFC                      0x05808
#define SXE_WUS                       0x05810
#define SXE_IP6AT(_i)                 (0x05880 + ((_i) * 4))   


#define SXE_IP6AT_CNT                 4


#define SXE_WUC_PME_EN                0x00000002
#define SXE_WUC_PME_STATUS            0x00000004
#define SXE_WUC_WKEN                  0x00000010
#define SXE_WUC_APME                  0x00000020


#define SXE_WUFC_LNKC                 0x00000001
#define SXE_WUFC_MAG                  0x00000002
#define SXE_WUFC_EX                   0x00000004
#define SXE_WUFC_MC                   0x00000008
#define SXE_WUFC_BC                   0x00000010
#define SXE_WUFC_ARP                  0x00000020
#define SXE_WUFC_IPV4                 0x00000040
#define SXE_WUFC_IPV6                 0x00000080
#define SXE_WUFC_MNG                  0x00000100




#define SXE_TSCTRL              0x14800
#define SXE_TSES                0x14804
#define SXE_TSYNCTXCTL          0x14810
#define SXE_TSYNCRXCTL          0x14820
#define SXE_RXSTMPL             0x14824
#define SXE_RXSTMPH             0x14828
#define SXE_SYSTIML             0x14840
#define SXE_SYSTIMM             0x14844
#define SXE_SYSTIMH             0x14848
#define SXE_TIMADJL             0x14850
#define SXE_TIMADJH             0x14854
#define SXE_TIMINC              0x14860


#define SXE_TSYNCTXCTL_TXTT     0x0001
#define SXE_TSYNCTXCTL_TEN      0x0010


#define SXE_TSYNCRXCTL_RXTT     0x0001
#define SXE_TSYNCRXCTL_REN      0x0010


#define SXE_TSCTRL_TSSEL        0x00001
#define SXE_TSCTRL_TSEN         0x00002
#define SXE_TSCTRL_VER_2        0x00010
#define SXE_TSCTRL_ONESTEP      0x00100
#define SXE_TSCTRL_CSEN         0x01000
#define SXE_TSCTRL_PTYP_ALL     0x00C00
#define SXE_TSCTRL_L4_UNICAST   0x08000


#define SXE_TSES_TXES                   0x00200
#define SXE_TSES_RXES                   0x00800
#define SXE_TSES_TXES_V1_SYNC           0x00000
#define SXE_TSES_TXES_V1_DELAY_REQ      0x00100
#define SXE_TSES_TXES_V1_ALL            0x00200
#define SXE_TSES_RXES_V1_SYNC           0x00000
#define SXE_TSES_RXES_V1_DELAY_REQ      0x00400
#define SXE_TSES_RXES_V1_ALL            0x00800
#define SXE_TSES_TXES_V2_ALL            0x00200
#define SXE_TSES_RXES_V2_ALL            0x00800

#define SXE_IV_SNS              0
#define SXE_IV_NS               8
#define SXE_INCPD               0
#define SXE_BASE_INCVAL         8


#define SXE_VT_CTL		0x051B0
#define SXE_PFMAILBOX(_i)	(0x04B00 + (4 * (_i)))

#define SXE_PFMBICR(_i)		(0x00710 + (4 * (_i)))
#define SXE_VFLRE(i)		((i & 1)? 0x001C0 : 0x00600)
#define SXE_VFLREC(i)		(0x00700 + (i * 4))
#define SXE_VFRE(_i)		(0x051E0 + ((_i) * 4))
#define SXE_VFTE(_i)		(0x08110 + ((_i) * 4))
#define SXE_QDE			(0x02F04)             
#define SXE_SPOOF(_i)		(0x08200 + (_i) * 4)
#define SXE_PFDTXGSWC		0x08220
#define SXE_VMVIR(_i)		(0x08000 + ((_i) * 4))
#define SXE_VMOLR(_i)		(0x0F000 + ((_i) * 4))
#define SXE_VLVF(_i)		(0x0F100 + ((_i) * 4))
#define SXE_VLVFB(_i)		(0x0F200 + ((_i) * 4))
#define SXE_MRCTL(_i)		(0x0F600 + ((_i) * 4))
#define SXE_VMRVLAN(_i)	        (0x0F610 + ((_i) * 4))
#define SXE_VMRVM(_i)		(0x0F630 + ((_i) * 4))
#define SXE_VMECM(_i)		(0x08790 + ((_i) * 4))
#define SXE_PFMBMEM(_i)		(0x13000 + (64 * (_i)))


#define SXE_VMOLR_CNT			64
#define SXE_VLVF_CNT			64
#define SXE_VLVFB_CNT			128
#define SXE_MRCTL_CNT			4
#define SXE_VMRVLAN_CNT			8
#define SXE_VMRVM_CNT			8
#define SXE_SPOOF_CNT			8
#define SXE_VMVIR_CNT			64
#define SXE_VFRE_CNT			2


#define SXE_VMVIR_VLANA_MASK	0xC0000000
#define SXE_VMVIR_VLAN_VID_MASK	0x00000FFF
#define SXE_VMVIR_VLAN_UP_MASK	0x0000E000


#define SXE_MRCTL_VPME  0x01

#define SXE_MRCTL_UPME  0x02

#define SXE_MRCTL_DPME  0x04

#define SXE_MRCTL_VLME  0x08


#define SXE_VT_CTL_DIS_DEFPL  0x20000000
#define SXE_VT_CTL_REPLEN     0x40000000
#define SXE_VT_CTL_VT_ENABLE  0x00000001 
#define SXE_VT_CTL_POOL_SHIFT 7
#define SXE_VT_CTL_POOL_MASK  (0x3F << SXE_VT_CTL_POOL_SHIFT)


#define SXE_PFMAILBOX_STS         0x00000001
#define SXE_PFMAILBOX_ACK         0x00000002
#define SXE_PFMAILBOX_VFU         0x00000004
#define SXE_PFMAILBOX_PFU         0x00000008
#define SXE_PFMAILBOX_RVFU        0x00000010


#define SXE_PFMBICR_VFREQ         0x00000001
#define SXE_PFMBICR_VFACK         0x00010000
#define SXE_PFMBICR_VFREQ_MASK    0x0000FFFF
#define SXE_PFMBICR_VFACK_MASK    0xFFFF0000


#define SXE_QDE_ENABLE		(0x00000001)
#define SXE_QDE_HIDE_VLAN	(0x00000002)
#define SXE_QDE_IDX_MASK	(0x00007F00)
#define SXE_QDE_IDX_SHIFT	(8)
#define SXE_QDE_WRITE		(0x00010000)



#define SXE_SPOOF_VLAN_SHIFT  (8)


#define SXE_PFDTXGSWC_VT_LBEN	0x1 


#define SXE_VMVIR_VLANA_DEFAULT 0x40000000
#define SXE_VMVIR_VLANA_NEVER   0x80000000


#define SXE_VMOLR_UPE		0x00400000
#define SXE_VMOLR_VPE		0x00800000
#define SXE_VMOLR_AUPE		0x01000000
#define SXE_VMOLR_ROMPE		0x02000000
#define SXE_VMOLR_ROPE		0x04000000
#define SXE_VMOLR_BAM		0x08000000
#define SXE_VMOLR_MPE		0x10000000


#define SXE_VLVF_VIEN         0x80000000 
#define SXE_VLVF_ENTRIES      64
#define SXE_VLVF_VLANID_MASK  0x00000FFF


#define SXE_HDC_HOST_BASE       0x16000
#define SXE_HDC_SW_LK           (SXE_HDC_HOST_BASE + 0x00)
#define SXE_HDC_PF_LK           (SXE_HDC_HOST_BASE + 0x04)
#define SXE_HDC_SW_OV           (SXE_HDC_HOST_BASE + 0x08)
#define SXE_HDC_FW_OV           (SXE_HDC_HOST_BASE + 0x0C)
#define SXE_HDC_PACKET_HEAD0    (SXE_HDC_HOST_BASE + 0x10)

#define SXE_HDC_PACKET_DATA0    (SXE_HDC_HOST_BASE + 0x20)


#define SXE_HDC_MSI_STATUS_REG  0x17000
#define SXE_FW_STATUS_REG       0x17004
#define SXE_DRV_STATUS_REG      0x17008
#define SXE_FW_HDC_STATE_REG    0x1700C
#define SXE_R0_MAC_ADDR_RAL     0x17010
#define SXE_R0_MAC_ADDR_RAH     0x17014
#define SXE_CRC_STRIP_REG		0x17018


#define SXE_HDC_SW_LK_BIT       0x0001
#define SXE_HDC_PF_LK_BIT       0x0003
#define SXE_HDC_SW_OV_BIT       0x0001
#define SXE_HDC_FW_OV_BIT       0x0001
#define SXE_HDC_RELEASE_SW_LK   0x0000

#define SXE_HDC_LEN_TO_REG(n)        (n - 1)
#define SXE_HDC_LEN_FROM_REG(n)      (n + 1)


#define SXE_RX_PKT_BUF_SIZE_SHIFT    10
#define SXE_TX_PKT_BUF_SIZE_SHIFT    10

#define SXE_RXIDX_TBL_SHIFT           1
#define SXE_RXTXIDX_IPS_EN            0x00000001
#define SXE_RXTXIDX_IDX_SHIFT         3
#define SXE_RXTXIDX_READ              0x40000000
#define SXE_RXTXIDX_WRITE             0x80000000


#define SXE_KEEP_CRC_EN		      0x00000001


#define SXE_VMD_CTL			0x0581C


#define SXE_VMD_CTL_POOL_EN		0x00000001
#define SXE_VMD_CTL_POOL_FILTER		0x00000002


#define SXE_FLCTRL                    0x14300
#define SXE_PFCTOP                    0x14304
#define SXE_FCTTV0                    0x14310
#define SXE_FCTTV(_i)                (SXE_FCTTV0 + ((_i) * 4))
#define SXE_FCRTV                     0x14320
#define SXE_TFCS                      0x14324


#define SXE_FCTRL_TFCE_MASK           0x0018
#define SXE_FCTRL_TFCE_LFC_EN         0x0008
#define SXE_FCTRL_TFCE_PFC_EN         0x0010
#define SXE_FCTRL_TFCE_DPF_EN         0x0020
#define SXE_FCTRL_RFCE_MASK           0x0300
#define SXE_FCTRL_RFCE_LFC_EN         0x0100
#define SXE_FCTRL_RFCE_PFC_EN         0x0200

#define SXE_FCTRL_TFCE_FCEN_MASK      0x00FF0000
#define SXE_FCTRL_TFCE_XONE_MASK      0xFF000000


#define SXE_PFCTOP_FCT               0x8808
#define SXE_PFCTOP_FCOP_MASK         0xFFFF0000
#define SXE_PFCTOP_FCOP_PFC          0x01010000
#define SXE_PFCTOP_FCOP_LFC          0x00010000


#define SXE_COMCTRL                   0x14400
#define SXE_PCCTRL                    0x14404
#define SXE_LPBKCTRL                  0x1440C
#define SXE_MAXFS                     0x14410
#define SXE_SACONH                    0x14420
#define SXE_SACONL                    0x14424
#define SXE_VLANCTRL                  0x14430
#define SXE_VLANID                    0x14434
#define SXE_LINKS                     0x14454
#define SXE_FPGA_SDS_STS	      0x14704
#define SXE_MSCA                      0x14500
#define SXE_MSCD                      0x14504

#define SXE_HLREG0                    0x04240
#define SXE_MFLCN                     0x04294
#define SXE_MACC                      0x04330

#define SXE_PCS1GLSTA                 0x0420C
#define SXE_MFLCN                     0x04294
#define SXE_PCS1GANA                  0x04850
#define SXE_PCS1GANLP                 0x04854


#define SXE_LPBKCTRL_EN               0x00000001


#define SXE_MAC_ADDR_SACONH_SHIFT     32
#define SXE_MAC_ADDR_SACONL_MASK      0xFFFFFFFF


#define SXE_PCS1GLSTA_AN_COMPLETE     0x10000
#define SXE_PCS1GLSTA_AN_PAGE_RX      0x20000
#define SXE_PCS1GLSTA_AN_TIMED_OUT    0x40000
#define SXE_PCS1GLSTA_AN_REMOTE_FAULT 0x80000
#define SXE_PCS1GLSTA_AN_ERROR_RWS    0x100000

#define SXE_PCS1GANA_SYM_PAUSE        0x100
#define SXE_PCS1GANA_ASM_PAUSE        0x80 


#define SXE_LKSTS_PCS_LKSTS_UP        0x00000001
#define SXE_LINK_UP_TIME              90
#define SXE_AUTO_NEG_TIME             45


#define SXE_MSCA_NP_ADDR_MASK      0x0000FFFF
#define SXE_MSCA_NP_ADDR_SHIFT     0
#define SXE_MSCA_DEV_TYPE_MASK     0x001F0000
#define SXE_MSCA_DEV_TYPE_SHIFT    16        
#define SXE_MSCA_PHY_ADDR_MASK     0x03E00000
#define SXE_MSCA_PHY_ADDR_SHIFT    21        
#define SXE_MSCA_OP_CODE_MASK      0x0C000000
#define SXE_MSCA_OP_CODE_SHIFT     26        
#define SXE_MSCA_ADDR_CYCLE        0x00000000
#define SXE_MSCA_WRITE             0x04000000
#define SXE_MSCA_READ              0x0C000000
#define SXE_MSCA_READ_AUTOINC      0x08000000
#define SXE_MSCA_ST_CODE_MASK      0x30000000
#define SXE_MSCA_ST_CODE_SHIFT     28        
#define SXE_MSCA_NEW_PROTOCOL      0x00000000
#define SXE_MSCA_OLD_PROTOCOL      0x10000000
#define SXE_MSCA_BYPASSRA_C45      0x40000000
#define SXE_MSCA_MDI_CMD_ON_PROG   0x80000000


#define MDIO_MSCD_RDATA_LEN        16
#define MDIO_MSCD_RDATA_SHIFT      16


#define SXE_CRCERRS                   0x14A04
#define SXE_ERRBC                     0x14A10
#define SXE_RLEC                      0x14A14
#define SXE_PRC64                     0x14A18
#define SXE_PRC127                    0x14A1C
#define SXE_PRC255                    0x14A20
#define SXE_PRC511                    0x14A24
#define SXE_PRC1023                   0x14A28
#define SXE_PRC1522                   0x14A2C
#define SXE_BPRC                      0x14A30
#define SXE_MPRC                      0x14A34
#define SXE_GPRC                      0x14A38
#define SXE_GORCL                     0x14A3C
#define SXE_GORCH                     0x14A40
#define SXE_RUC                       0x14A44
#define SXE_RFC                       0x14A48
#define SXE_ROC                       0x14A4C
#define SXE_RJC                       0x14A50
#define SXE_TORL                      0x14A54
#define SXE_TORH                      0x14A58
#define SXE_TPR                       0x14A5C
#define SXE_PRCPF(_i)                 (0x14A60 + ((_i) * 4))
#define SXE_GPTC                      0x14B00
#define SXE_GOTCL                     0x14B04
#define SXE_GOTCH                     0x14B08
#define SXE_TPT                       0x14B0C
#define SXE_PTC64                     0x14B10
#define SXE_PTC127                    0x14B14
#define SXE_PTC255                    0x14B18
#define SXE_PTC511                    0x14B1C
#define SXE_PTC1023                   0x14B20
#define SXE_PTC1522                   0x14B24
#define SXE_MPTC                      0x14B28
#define SXE_BPTC                      0x14B2C
#define SXE_PFCT(_i)                  (0x14B30 + ((_i) * 4))

#define SXE_MACCFG                    0x0CE04
#define SXE_MACCFG_PAD_EN             0x00000001


#define SXE_COMCTRL_TXEN	      0x0001        
#define SXE_COMCTRL_RXEN	      0x0002        
#define SXE_COMCTRL_EDSEL	      0x0004        
#define SXE_COMCTRL_SPEED_1G	      0x0200        
#define SXE_COMCTRL_SPEED_10G	      0x0300        


#define SXE_PCCTRL_TXCE		      0x0001        
#define SXE_PCCTRL_RXCE		      0x0002        
#define SXE_PCCTRL_PEN		      0x0100        
#define SXE_PCCTRL_PCSC_ALL	      0x30000       


#define SXE_MAXFS_TFSEL		      0x0001        
#define SXE_MAXFS_RFSEL		      0x0002        
#define SXE_MAXFS_MFS_MASK	      0xFFFF0000    
#define SXE_MAXFS_MFS		      0x40000000    
#define SXE_MAXFS_MFS_SHIFT	      16            


#define SXE_LINKS_UP 	              0x00000001    

#define SXE_10G_LINKS_DOWN            0x00000006


#define SXE_LINK_SPEED_UNKNOWN        0             
#define SXE_LINK_SPEED_10_FULL        0x0002        
#define SXE_LINK_SPEED_100_FULL       0x0008        
#define SXE_LINK_SPEED_1GB_FULL       0x0020        
#define SXE_LINK_SPEED_10GB_FULL      0x0080        


#define SXE_HLREG0_TXCRCEN            0x00000001  
#define SXE_HLREG0_RXCRCSTRP          0x00000002  
#define SXE_HLREG0_JUMBOEN            0x00000004  
#define SXE_HLREG0_TXPADEN            0x00000400  
#define SXE_HLREG0_TXPAUSEEN          0x00001000  
#define SXE_HLREG0_RXPAUSEEN          0x00004000  
#define SXE_HLREG0_LPBK               0x00008000  
#define SXE_HLREG0_MDCSPD             0x00010000  
#define SXE_HLREG0_CONTMDC            0x00020000  
#define SXE_HLREG0_CTRLFLTR           0x00040000  
#define SXE_HLREG0_PREPEND            0x00F00000  
#define SXE_HLREG0_PRIPAUSEEN         0x01000000  
#define SXE_HLREG0_RXPAUSERECDA       0x06000000  
#define SXE_HLREG0_RXLNGTHERREN       0x08000000  
#define SXE_HLREG0_RXPADSTRIPEN       0x10000000  

#define SXE_MFLCN_PMCF                0x00000001  
#define SXE_MFLCN_DPF                 0x00000002  
#define SXE_MFLCN_RPFCE               0x00000004  
#define SXE_MFLCN_RFCE                0x00000008  
#define SXE_MFLCN_RPFCE_MASK	      0x00000FF4  
#define SXE_MFLCN_RPFCE_SHIFT         4

#define SXE_MACC_FLU                  0x00000001
#define SXE_MACC_FSV_10G              0x00030000
#define SXE_MACC_FS                   0x00040000

#define SXE_DEFAULT_FCPAUSE           0xFFFF


#define SXE_SAQF(_i)		(0x0E000 + ((_i) * 4)) 
#define SXE_DAQF(_i)		(0x0E200 + ((_i) * 4)) 
#define SXE_SDPQF(_i)		(0x0E400 + ((_i) * 4)) 
#define SXE_FTQF(_i)		(0x0E600 + ((_i) * 4)) 
#define SXE_L34T_IMIR(_i)	(0x0E800 + ((_i) * 4)) 

#define SXE_MAX_FTQF_FILTERS		128
#define SXE_FTQF_PROTOCOL_MASK		0x00000003
#define SXE_FTQF_PROTOCOL_TCP		0x00000000
#define SXE_FTQF_PROTOCOL_UDP		0x00000001
#define SXE_FTQF_PROTOCOL_SCTP		2
#define SXE_FTQF_PRIORITY_MASK		0x00000007
#define SXE_FTQF_PRIORITY_SHIFT		2
#define SXE_FTQF_POOL_MASK		0x0000003F
#define SXE_FTQF_POOL_SHIFT		8
#define SXE_FTQF_5TUPLE_MASK_MASK	0x0000001F
#define SXE_FTQF_5TUPLE_MASK_SHIFT	25
#define SXE_FTQF_SOURCE_ADDR_MASK	0x1E
#define SXE_FTQF_DEST_ADDR_MASK		0x1D
#define SXE_FTQF_SOURCE_PORT_MASK	0x1B
#define SXE_FTQF_DEST_PORT_MASK		0x17
#define SXE_FTQF_PROTOCOL_COMP_MASK	0x0F
#define SXE_FTQF_POOL_MASK_EN		0x40000000
#define SXE_FTQF_QUEUE_ENABLE		0x80000000

#define SXE_SDPQF_DSTPORT		0xFFFF0000
#define SXE_SDPQF_DSTPORT_SHIFT		16
#define SXE_SDPQF_SRCPORT		0x0000FFFF

#define SXE_L34T_IMIR_SIZE_BP		0x00001000
#define SXE_L34T_IMIR_RESERVE		0x00080000
#define SXE_L34T_IMIR_LLI			0x00100000
#define SXE_L34T_IMIR_QUEUE			0x0FE00000
#define SXE_L34T_IMIR_QUEUE_SHIFT	21

#define SXE_VMTXSW(_i)                (0x05180 + ((_i) * 4))   
#define SXE_VMTXSW_REGISTER_COUNT     2

#define SXE_TXSTMP_SEL		0x14510  
#define SXE_TXSTMP_VAL		0x1451c  

#define SXE_TXTS_MAGIC0		0x005a005900580057
#define SXE_TXTS_MAGIC1		0x005e005d005c005b

#endif

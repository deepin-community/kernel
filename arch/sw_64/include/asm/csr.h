/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_CSR_H
#define _ASM_SW64_CSR_H

#include <asm/hmcall.h>

#define CSR_EXC_SUM		0xd
#define CSR_INT_EN		0x1a
#define CSR_INT_STAT		0x1b
#define CSR_PCIE_MSI0_INT	0x1d
#define CSR_PCIE_MSI1_INT	0x1e
#define CSR_PCIE_MSI2_INT	0x1f
#define CSR_PCIE_MSI3_INT	0x20
#define CSR_INT_VEC		0x2d
#define CSR_PCIE_MSI0_INTEN	0x35
#define CSR_PCIE_MSI1_INTEN	0x36
#define CSR_PCIE_MSI2_INTEN	0x37
#define CSR_PCIE_MSI3_INTEN	0x38
#define CSR_EXC_GPA		0x3b
#define CSR_EXC_PC		0xe
#define CSR_AS_INFO		0x3c
#define CSR_DS_STAT		0x48
#define CSR_SOFTCID		0xc9
#define CSR_DVA			0x54
#define CSR_PTBR_SYS		0x68
#define CSR_PTBR_USR		0x69
#define CSR_APTP		0x6a
#define CSR_CID			0xc4
#define CSR_WR_FREGS		0xc8
#define CSR_SHTCLOCK		0xca
#define CSR_SHTCLOCK_OFFSET	0xcb

#ifdef CONFIG_SUBARCH_C4
#define CSR_IA_VPNMATCH		0xa
#define CSR_UPCR		0x15
#define CSR_VPCR		0x16
#define CSR_IA_MATCH		0x17
#define CSR_IA_MASK		0x18
#define CSR_IV_MATCH		0x19
#define CSR_IA_UPNMATCH		0x3a
#define CSR_DC_CTLP		0x4e
#define CSR_DA_MATCH		0x51
#define CSR_DA_MASK		0x52
#define CSR_DA_MATCH_MODE	0x53
#define CSR_DV_MATCH		0x56
#define CSR_DV_MASK		0x57
#define CSR_IDA_MATCH		0xc5
#define CSR_IDA_MASK		0xc6

#define DA_MATCH_EN_S		4
#define DV_MATCH_EN_S		6
#define DAV_MATCH_EN_S		7
#define DPM_MATCH		8
#define DPM_MATCH_EN_S		10
#define IDA_MATCH_EN_S		53
#define IV_PM_EN_S		61
#define IV_MATCH_EN_S		62
#define IA_MATCH_EN_S		63

#endif


#ifdef CONFIG_HAVE_CSRRW
#define read_csr(x)					\
	({ unsigned long __val;				\
	 __asm__ __volatile__("csrr %0,%1" : "=r"(__val) : "i"(x));	\
	 __val; })

#define write_csr(x, y)					\
	({ __asm__ __volatile__("csrw %0,%1" ::"r"(x), "i"(y)); })

#define write_csr_imb(x, y)				\
	({ __asm__ __volatile__("csrw %0,%1; imemb" ::"r"(x), "i"(y)); })


#ifndef __ASSEMBLY__
#include <asm/barrier.h>
static inline void update_ptbr_sys(unsigned long ptbr)
{
	imemb();
	write_csr_imb(ptbr, CSR_PTBR_SYS);
}
#endif
#else
#define read_csr(x)			(0)
#define write_csr(x, y)			do { } while (0)
#define write_csr_imb(x, y)		do { } while (0)

#ifndef __ASSEMBLY__
static inline void update_ptbr_sys(unsigned long ptbr)
{
	wrptbr(ptbr);
}
#endif

#endif
#endif /* _ASM_SW64_CSR_H */

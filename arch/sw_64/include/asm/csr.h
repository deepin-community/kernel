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
#define CSR_PFH_CTL		0x4f
#define CSR_SOFTCID		0xc9
#define CSR_DVA			0x54
#define CSR_PFH_CNT		0x5c
#define CSR_BRRETC		0x5e
#define CSR_BRFAILC		0x5f
#define CSR_PTBR_SYS		0x68
#define CSR_PTBR_USR		0x69
#define CSR_APTP		0x6a
#define CSR_IDR_PCCTL		0x7a
#define CSR_IACC		0x7b
#define CSR_IMISC		0x7c
#define CSR_DACC		0x7d
#define CSR_DMISC		0x7e
#define CSR_RETIC		0x7f
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
#define CSR_BASE_KREGS		0xe0
#define CSR_NMI_STACK		0xe5
#define CSR_NMI_SCRATCH		0xe6
#define CSR_NMI_MASK		0xe7
#define CSR_PS			0xe8
#define CSR_PC			0xe9
#define CSR_EARG0		0xea
#define CSR_EARG1		0xeb
#define CSR_EARG2		0xec
#define CSR_SCRATCH		0xed
#define CSR_SP			0xee
#define CSR_KTP			0xef
#define CSR_CAUSE		0xf0

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

#define SOFTCSR0	0xe0
#define SOFTCSR1	0xe1
#define SOFTCSR2	0xe2
#define SOFTCSR3	0xe3
#define SOFTCSR4	0xe4
#define SOFTCSR5	0xe5
#define SOFTCSR6	0xe6
#define SOFTCSR7	0xe7
#define SOFTCSR8	0xe8
#define SOFTCSR9	0xe9
#define SOFTCSR10	0xea
#define SOFTCSR11	0xeb
#define SOFTCSR12	0xec
#define SOFTCSR13	0xed
#define SOFTCSR14	0xee
#define SOFTCSR15	0xef
#define SOFTCSR16	0xf0
#define SOFTCSR17	0xf1
#define SOFTCSR18	0xf2
#define SOFTCSR19	0xf3
#define SOFTCSR20	0xf4
#define SOFTCSR21	0xf5
#define SOFTCSR22	0xf6
#define SOFTCSR23	0xf7
#define SOFTCSR24	0xf8
#define SOFTCSR25	0xf9
#define SOFTCSR26	0xfa
#define SOFTCSR27	0xfb
#define SOFTCSR28	0xfc
#define SOFTCSR29	0xfd
#define SOFTCSR30	0xfe
#define SOFTCSR31	0xff

#define SOFTCSR32	0xd0
#define SOFTCSR33	0xd1
#define SOFTCSR34	0xd2
#define SOFTCSR35	0xd3
#define SOFTCSR36	0xd4
#define SOFTCSR37	0xd5
#define SOFTCSR38	0xd6
#define SOFTCSR39	0xd7
#define SOFTCSR40	0xd8
#define SOFTCSR41	0xd9
#define SOFTCSR42	0xda
#define SOFTCSR43	0xdb
#define SOFTCSR44	0xdc
#define SOFTCSR45	0xdd
#define SOFTCSR46	0xde
#define SOFTCSR47	0xdf

#ifdef CONFIG_HAVE_CSRRW
#ifndef __ASSEMBLY__
static __always_inline unsigned long sw64_read_csr(unsigned long x)
{
	unsigned long __val;

	__asm__ __volatile__("csrr %0,%1; csrr %0,%1" : "=r"(__val) : "i"(x));
	return __val;
}

static __always_inline void sw64_write_csr(unsigned long x, unsigned long y)
{
	__asm__ __volatile__("csrw %0,%1" ::"r"(x), "i"(y));
}

static __always_inline void sw64_write_csr_imb(unsigned long x, unsigned long y)
{
	__asm__ __volatile__("csrw %0,%1; imemb" ::"r"(x), "i"(y));
}

#include <asm/barrier.h>
static inline void update_ptbr_sys(unsigned long ptbr)
{
	mb();
	imemb();
	sw64_write_csr_imb(ptbr, CSR_PTBR_SYS);
	tbiv();
}

static inline void update_ptbr_usr(unsigned long ptbr)
{
	imemb();
	sw64_write_csr_imb(ptbr, CSR_PTBR_USR);
}

#endif
#else
#define sw64_read_csr(x)                     (0)
#define sw64_write_csr(x, y)                 do { } while (0)
#define sw64_write_csr_imb(x, y)             do { } while (0)

#ifndef __ASSEMBLY__
static inline void update_ptbr_sys(unsigned long ptbr)
{
	wrptbr(ptbr);
}
#endif
#endif /* CONFIG_HAVE_CSRRW */

#ifndef __ASSEMBLY__
struct soft_csrs {
	unsigned long sc[48];
};

static inline void save_all_soft_csrs(struct soft_csrs *sc)
{
	sc->sc[0] = sw64_read_csr(SOFTCSR0);
	sc->sc[1] = sw64_read_csr(SOFTCSR1);
	sc->sc[2] = sw64_read_csr(SOFTCSR2);
	sc->sc[3] = sw64_read_csr(SOFTCSR3);
	sc->sc[4] = sw64_read_csr(SOFTCSR4);
	sc->sc[5] = sw64_read_csr(SOFTCSR5);
	sc->sc[6] = sw64_read_csr(SOFTCSR6);
	sc->sc[7] = sw64_read_csr(SOFTCSR7);
	sc->sc[8] = sw64_read_csr(SOFTCSR8);
	sc->sc[9] = sw64_read_csr(SOFTCSR9);
	sc->sc[10] = sw64_read_csr(SOFTCSR10);
	sc->sc[11] = sw64_read_csr(SOFTCSR11);
	sc->sc[12] = sw64_read_csr(SOFTCSR12);
	sc->sc[13] = sw64_read_csr(SOFTCSR13);
	sc->sc[14] = sw64_read_csr(SOFTCSR14);
	sc->sc[15] = sw64_read_csr(SOFTCSR15);
	sc->sc[16] = sw64_read_csr(SOFTCSR16);
	sc->sc[17] = sw64_read_csr(SOFTCSR17);
	sc->sc[18] = sw64_read_csr(SOFTCSR18);
	sc->sc[19] = sw64_read_csr(SOFTCSR19);
	sc->sc[20] = sw64_read_csr(SOFTCSR20);
	sc->sc[21] = sw64_read_csr(SOFTCSR21);
	sc->sc[22] = sw64_read_csr(SOFTCSR22);
	sc->sc[23] = sw64_read_csr(SOFTCSR23);
	sc->sc[24] = sw64_read_csr(SOFTCSR24);
	sc->sc[25] = sw64_read_csr(SOFTCSR25);
	sc->sc[26] = sw64_read_csr(SOFTCSR26);
	sc->sc[27] = sw64_read_csr(SOFTCSR27);
	sc->sc[28] = sw64_read_csr(SOFTCSR28);
	sc->sc[29] = sw64_read_csr(SOFTCSR29);
	sc->sc[30] = sw64_read_csr(SOFTCSR30);
	sc->sc[31] = sw64_read_csr(SOFTCSR31);
	sc->sc[32] = sw64_read_csr(SOFTCSR32);
	sc->sc[33] = sw64_read_csr(SOFTCSR33);
	sc->sc[34] = sw64_read_csr(SOFTCSR34);
	sc->sc[35] = sw64_read_csr(SOFTCSR35);
	sc->sc[36] = sw64_read_csr(SOFTCSR36);
	sc->sc[37] = sw64_read_csr(SOFTCSR37);
	sc->sc[38] = sw64_read_csr(SOFTCSR38);
	sc->sc[39] = sw64_read_csr(SOFTCSR39);
	sc->sc[40] = sw64_read_csr(SOFTCSR40);
	sc->sc[41] = sw64_read_csr(SOFTCSR41);
	sc->sc[42] = sw64_read_csr(SOFTCSR42);
	sc->sc[43] = sw64_read_csr(SOFTCSR43);
	sc->sc[44] = sw64_read_csr(SOFTCSR44);
	sc->sc[45] = sw64_read_csr(SOFTCSR45);
	sc->sc[46] = sw64_read_csr(SOFTCSR46);
	sc->sc[47] = sw64_read_csr(SOFTCSR47);
}

static inline void restore_all_soft_csrs(struct soft_csrs *sc)
{
	sw64_write_csr(sc->sc[0], SOFTCSR0);
	sw64_write_csr(sc->sc[1], SOFTCSR1);
	sw64_write_csr(sc->sc[2], SOFTCSR2);
	sw64_write_csr(sc->sc[3], SOFTCSR3);
	sw64_write_csr(sc->sc[4], SOFTCSR4);
	sw64_write_csr(sc->sc[5], SOFTCSR5);
	sw64_write_csr(sc->sc[6], SOFTCSR6);
	sw64_write_csr(sc->sc[7], SOFTCSR7);
	sw64_write_csr(sc->sc[8], SOFTCSR8);
	sw64_write_csr(sc->sc[9], SOFTCSR9);
	sw64_write_csr(sc->sc[10], SOFTCSR10);
	sw64_write_csr(sc->sc[11], SOFTCSR11);
	sw64_write_csr(sc->sc[12], SOFTCSR12);
	sw64_write_csr(sc->sc[13], SOFTCSR13);
	sw64_write_csr(sc->sc[14], SOFTCSR14);
	sw64_write_csr(sc->sc[15], SOFTCSR15);
	sw64_write_csr(sc->sc[16], SOFTCSR16);
	sw64_write_csr(sc->sc[17], SOFTCSR17);
	sw64_write_csr(sc->sc[18], SOFTCSR18);
	sw64_write_csr(sc->sc[19], SOFTCSR19);
	sw64_write_csr(sc->sc[20], SOFTCSR20);
	sw64_write_csr(sc->sc[21], SOFTCSR21);
	sw64_write_csr(sc->sc[22], SOFTCSR22);
	sw64_write_csr(sc->sc[23], SOFTCSR23);
	sw64_write_csr(sc->sc[24], SOFTCSR24);
	sw64_write_csr(sc->sc[25], SOFTCSR25);
	sw64_write_csr(sc->sc[26], SOFTCSR26);
	sw64_write_csr(sc->sc[27], SOFTCSR27);
	sw64_write_csr(sc->sc[28], SOFTCSR28);
	sw64_write_csr(sc->sc[29], SOFTCSR29);
	sw64_write_csr(sc->sc[30], SOFTCSR30);
	sw64_write_csr(sc->sc[31], SOFTCSR31);
	sw64_write_csr(sc->sc[32], SOFTCSR32);
	sw64_write_csr(sc->sc[33], SOFTCSR33);
	sw64_write_csr(sc->sc[34], SOFTCSR34);
	sw64_write_csr(sc->sc[35], SOFTCSR35);
	sw64_write_csr(sc->sc[36], SOFTCSR36);
	sw64_write_csr(sc->sc[37], SOFTCSR37);
	sw64_write_csr(sc->sc[38], SOFTCSR38);
	sw64_write_csr(sc->sc[39], SOFTCSR39);
	sw64_write_csr(sc->sc[40], SOFTCSR40);
	sw64_write_csr(sc->sc[41], SOFTCSR41);
	sw64_write_csr(sc->sc[42], SOFTCSR42);
	sw64_write_csr(sc->sc[43], SOFTCSR43);
	sw64_write_csr(sc->sc[44], SOFTCSR44);
	sw64_write_csr(sc->sc[45], SOFTCSR45);
	sw64_write_csr(sc->sc[46], SOFTCSR46);
	sw64_write_csr(sc->sc[47], SOFTCSR47);
}
#endif

#endif /* _ASM_SW64_CSR_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_FPU_H
#define _ASM_SW64_FPU_H

#include <uapi/asm/fpu.h>

#define EXC_SUM_SWC		(1UL << 0)

#define EXC_SUM_DZE_INT		(1UL << 39)

#define EXC_SUM_INV0		(1UL << 1)
#define EXC_SUM_DZE0		(1UL << 2)
#define EXC_SUM_OVF0		(1UL << 3)
#define EXC_SUM_UNF0		(1UL << 4)
#define EXC_SUM_INE0		(1UL << 5)
#define EXC_SUM_OVI0		(1UL << 6)
#define EXC_SUM_DNO0		(1UL << 40)

#define EXC_SUM_INV1		(1UL << 15)
#define EXC_SUM_DZE1		(1UL << 16)
#define EXC_SUM_OVF1		(1UL << 17)
#define EXC_SUM_UNF1		(1UL << 18)
#define EXC_SUM_INE1		(1UL << 19)
#define EXC_SUM_OVI1		(1UL << 20)
#define EXC_SUM_DNO1		(1UL << 41)

#define EXC_SUM_INV2		(1UL << 21)
#define EXC_SUM_DZE2		(1UL << 22)
#define EXC_SUM_OVF2		(1UL << 23)
#define EXC_SUM_UNF2		(1UL << 24)
#define EXC_SUM_INE2		(1UL << 25)
#define EXC_SUM_OVI2		(1UL << 26)
#define EXC_SUM_DNO2		(1UL << 42)

#define EXC_SUM_INV3		(1UL << 27)
#define EXC_SUM_DZE3		(1UL << 28)
#define EXC_SUM_OVF3		(1UL << 29)
#define EXC_SUM_UNF3		(1UL << 30)
#define EXC_SUM_INE3		(1UL << 31)
#define EXC_SUM_OVI3		(1UL << 32)
#define EXC_SUM_DNO3		(1UL << 43)

#define EXC_SUM_FP_STATUS0	(EXC_SUM_INV0 | EXC_SUM_DZE0 |	\
				 EXC_SUM_OVF0 | EXC_SUM_UNF0 |	\
				 EXC_SUM_INE0 | EXC_SUM_OVI0 |	\
				 EXC_SUM_DNO0)

#define EXC_SUM_FP_STATUS1	(EXC_SUM_INV1 | EXC_SUM_DZE1 |	\
				 EXC_SUM_OVF1 | EXC_SUM_UNF1 |	\
				 EXC_SUM_INE1 | EXC_SUM_OVI1 |	\
				 EXC_SUM_DNO1)

#define EXC_SUM_FP_STATUS2	(EXC_SUM_INV2 | EXC_SUM_DZE2 |	\
				 EXC_SUM_OVF2 | EXC_SUM_UNF2 |	\
				 EXC_SUM_INE2 | EXC_SUM_OVI2 |	\
				 EXC_SUM_DNO2)

#define EXC_SUM_FP_STATUS3	(EXC_SUM_INV3 | EXC_SUM_DZE3 |	\
				 EXC_SUM_OVF3 | EXC_SUM_UNF3 |	\
				 EXC_SUM_INE3 | EXC_SUM_OVI3 |	\
				 EXC_SUM_DNO3)

#define EXC_SUM_FP_STATUS_ALL	(EXC_SUM_FP_STATUS0 | EXC_SUM_FP_STATUS1 | \
				 EXC_SUM_FP_STATUS2 | EXC_SUM_FP_STATUS3)

#define EXC_SUM_INV		(EXC_SUM_INV0 | EXC_SUM_INV1 |	\
				 EXC_SUM_INV2 | EXC_SUM_INV3)
#define EXC_SUM_DZE		(EXC_SUM_DZE0 | EXC_SUM_DZE1 |	\
				 EXC_SUM_DZE2 | EXC_SUM_DZE3)
#define EXC_SUM_OVF		(EXC_SUM_OVF0 | EXC_SUM_OVF1 |	\
				 EXC_SUM_OVF2 | EXC_SUM_OVF3)
#define EXC_SUM_UNF		(EXC_SUM_UNF0 | EXC_SUM_UNF1 |	\
				 EXC_SUM_UNF2 | EXC_SUM_UNF3)
#define EXC_SUM_INE		(EXC_SUM_INE0 | EXC_SUM_INE1 |	\
				 EXC_SUM_INE2 | EXC_SUM_INE3)
#define EXC_SUM_OVI		(EXC_SUM_OVI0 | EXC_SUM_OVI1 |	\
				 EXC_SUM_OVI2 | EXC_SUM_OVI3)
#define EXC_SUM_DNO		(EXC_SUM_DNO0 | EXC_SUM_DNO1 |	\
				 EXC_SUM_DNO2 | EXC_SUM_DNO3)

#ifdef __KERNEL__

#include <asm/sfp-machine.h>

/*
 * The following two functions don't need trapb/excb instructions
 * around the mf_fpcr/mt_fpcr instructions because (a) the kernel
 * never generates arithmetic faults and (b) sys_call instructions
 * are implied trap barriers.
 */

static inline unsigned long
rdfpcr(void)
{
	unsigned long ret;
	unsigned long fp[4] __aligned(32);

	__asm__ __volatile__ (
		"	vstd	$f0, %0\n\t"
		"	rfpcr	$f0\n\t"
		"	fimovd	$f0, %1\n\t"
		"	vldd	$f0, %0\n\t"
		: "=m"(*fp), "=&r"(ret));

	return ret;
}

static inline void
wrfpcr(unsigned long val)
{
	unsigned long tmp;
	unsigned long fp[4] __aligned(32);

	__asm__ __volatile__ (
		"	vstd	$f0, %0\n\t"
		"	ifmovd	%2, $f0\n\t"
		"	wfpcr	$f0\n\t"
		"	and	%2, 0x3, %1\n\t"
		"	beq	%1, 1f\n\t"
		"	subl	%1, 1, %1\n\t"
		"	beq	%1, 2f\n\t"
		"	subl	%1, 1, %1\n\t"
		"	beq	%1, 3f\n\t"
		"	setfpec3\n\t"
		"	br	6f\n\t"
		"1:	setfpec0\n\t"
		"	br	6f\n\t"
		"2:	setfpec1\n\t"
		"	br	6f\n\t"
		"3:	setfpec2\n\t"
		"6:	vldd	$f0, %0\n\t"
		: "=m"(*fp), "=&r"(tmp) : "r"(val));
}

static inline unsigned long
swcr_update_status(unsigned long swcr, unsigned long fpcr)
{
	/*
	 * SW64 implements most of the bits in hardware.  Collect
	 * the acrued exception bits from the real fpcr.
	 */
	swcr &= ~IEEE_STATUS_MASK_ALL;
	swcr |= ieee_status_fpcr_to_swcr(fpcr & FPCR_STATUS_MASK_ALL);
	return swcr;
}

static inline unsigned long
swcr_status_to_fex(unsigned long swcr_status, int part)
{
	unsigned long fex = 0;

	if (part < -1 || part > 3) {
		pr_warn("%s: invalid part index, counting all parts\n", __func__);
		part = -1;
	}

	if (part == -1 || part == 0) {
		fex |= (swcr_status & (IEEE_STATUS_INV0 | IEEE_STATUS_DZE0 |
					IEEE_STATUS_OVF0 | IEEE_STATUS_UNF0 |
					IEEE_STATUS_INE0 | IEEE_STATUS_DNO0)) >>
			(17 - 1);
		fex |= fex & IEEE_STATUS_OVI0 ? FP_EX_OVERINT : 0;
	}

	if (part == -1 || part == 1) {
		fex |= (swcr_status & (IEEE_STATUS_INV1 | IEEE_STATUS_DZE1 |
					IEEE_STATUS_OVF1 | IEEE_STATUS_UNF1 |
					IEEE_STATUS_INE1 | IEEE_STATUS_DNO1)) >>
			(23 - 1);
		fex |= fex & IEEE_STATUS_OVI1 ? FP_EX_OVERINT : 0;
	}

	if (part == -1 || part == 2) {
		fex |= (swcr_status & (IEEE_STATUS_INV2 | IEEE_STATUS_DZE2 |
					IEEE_STATUS_OVF2 | IEEE_STATUS_UNF2 |
					IEEE_STATUS_INE2 | IEEE_STATUS_DNO2)) >>
			(34 - 1);
		fex |= fex & IEEE_STATUS_OVI2 ? FP_EX_OVERINT : 0;
	}

	if (part == -1 || part == 3) {
		fex |= (swcr_status & (IEEE_STATUS_INV3 | IEEE_STATUS_DZE3 |
					IEEE_STATUS_OVF3 | IEEE_STATUS_UNF3 |
					IEEE_STATUS_INE3 | IEEE_STATUS_DNO3)) >>
			(40 - 1);
		fex |= fex & IEEE_STATUS_OVI3 ? FP_EX_OVERINT : 0;
	}

	return fex;
}

static inline unsigned long
fex_to_swcr_status(unsigned long fex, int part)
{
	unsigned long swcr_status = 0;

	switch (part) {
	case 0:
		swcr_status |= (fex & (FP_EX_INVALID | FP_EX_OVERFLOW |
				FP_EX_UNDERFLOW | FP_EX_DIVZERO |
				FP_EX_INEXACT | FP_EX_DENORM)) << (17 - 1);
		swcr_status |= fex & FP_EX_OVERINT ? IEEE_STATUS_OVI0 : 0;
		break;
	case 1:
		swcr_status |= (fex & (FP_EX_INVALID | FP_EX_OVERFLOW |
				FP_EX_UNDERFLOW | FP_EX_DIVZERO |
				FP_EX_INEXACT | FP_EX_DENORM)) << (23 - 1);
		swcr_status |= fex & FP_EX_OVERINT ? IEEE_STATUS_OVI1 : 0;
		break;
	case 2:
		swcr_status |= (fex & (FP_EX_INVALID | FP_EX_OVERFLOW |
				FP_EX_UNDERFLOW | FP_EX_DIVZERO |
				FP_EX_INEXACT | FP_EX_DENORM)) << (34 - 1);
		swcr_status |= fex & FP_EX_OVERINT ? IEEE_STATUS_OVI2 : 0;
		break;
	case 3:
		swcr_status |= (fex & (FP_EX_INVALID | FP_EX_OVERFLOW |
				FP_EX_UNDERFLOW | FP_EX_DIVZERO |
				FP_EX_INEXACT | FP_EX_DENORM)) << (40 - 1);
		swcr_status |= fex & FP_EX_OVERINT ? IEEE_STATUS_OVI3 : 0;
		break;
	default:
		pr_err("%s: invalid part index\n", __func__);
	}

	return swcr_status;
}

extern unsigned long sw64_read_fp_reg(unsigned long reg);
extern void sw64_write_fp_reg(unsigned long reg, unsigned long val);
extern unsigned long sw64_read_fp_reg_s(unsigned long reg);
extern void sw64_write_fp_reg_s(unsigned long reg, unsigned long val);


extern void sw64_write_simd_fp_reg_s(unsigned long reg,
				      unsigned long f0, unsigned long f1);
extern void sw64_write_simd_fp_reg_d(unsigned long reg,
				      unsigned long f0, unsigned long f1,
				      unsigned long f2, unsigned long f3);
extern void sw64_write_simd_fp_reg_ldwe(unsigned long reg, int a);
extern void sw64_read_simd_fp_m_s(unsigned long reg, unsigned long *fp_value);
extern void sw64_read_simd_fp_m_d(unsigned long reg, unsigned long *fp_value);

#endif /* __KERNEL__ */

#endif /* _ASM_SW64_FPU_H */

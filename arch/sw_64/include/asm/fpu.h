/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_FPU_H
#define _ASM_SW64_FPU_H

#include <uapi/asm/fpu.h>
#ifdef __KERNEL__

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
		: "=m"(*fp), "=r"(ret));

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
	swcr &= ~(IEEE_STATUS_MASK0 | IEEE_STATUS_MASK1
				| IEEE_STATUS_MASK2 | IEEE_STATUS_MASK3);
	swcr |= (fpcr >> 35) & IEEE_STATUS_MASK0;
	swcr |= (fpcr >> 13) & IEEE_STATUS_MASK1;
	swcr |= (fpcr << 14) & IEEE_STATUS_MASK2;
	swcr |= (fpcr << 36) & IEEE_STATUS_MASK3;
	return swcr;
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

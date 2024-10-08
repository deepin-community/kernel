/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_SW64_FPU_H
#define _UAPI_ASM_SW64_FPU_H

/*
 * SW-64 floating-point control register defines:
 */
#define FPCR_DNOD	(1UL << 47)		/* denorm INV trap disable */
#ifdef CONFIG_SUBARCH_C3B
#define FPCR_DNZ	(1UL << 48)		/* denorms to zero */
#else
#define FPCR_DNOE	(1UL << 48)		/* hardware denormal support */
#endif
#define FPCR_INVD	(1UL << 49)		/* invalid op disable (opt.) */
#define FPCR_DZED	(1UL << 50)		/* division by zero disable (opt.) */
#define FPCR_OVFD	(1UL << 51)		/* overflow disable (optional) */
#define FPCR_INV	(1UL << 52)		/* invalid operation */
#define FPCR_DZE	(1UL << 53)		/* division by zero */
#define FPCR_OVF	(1UL << 54)		/* overflow */
#define FPCR_UNF	(1UL << 55)		/* underflow */
#define FPCR_INE	(1UL << 56)		/* inexact */
#define FPCR_IOV	(1UL << 57)		/* integer overflow */
#define FPCR_UNDZ	(1UL << 60)		/* underflow to zero (opt.) */
#define FPCR_UNFD	(1UL << 61)		/* underflow disable (opt.) */
#define FPCR_INED	(1UL << 62)		/* inexact disable (opt.) */
#define FPCR_SUM	(1UL << 63)		/* summary bit */

#define FPCR_DYN_SHIFT		58				/* first dynamic rounding mode bit */
#define FPCR_DYN_CHOPPED	(0x0UL << FPCR_DYN_SHIFT)	/* towards 0 */
#define FPCR_DYN_MINUS		(0x1UL << FPCR_DYN_SHIFT)	/* towards -INF */
#define FPCR_DYN_NORMAL		(0x2UL << FPCR_DYN_SHIFT)	/* towards nearest */
#define FPCR_DYN_PLUS		(0x3UL << FPCR_DYN_SHIFT)	/* towards +INF */
#define FPCR_DYN_MASK		(0x3UL << FPCR_DYN_SHIFT)

#define FPCR_MASK		0xffff800000000000L

#ifdef CONFIG_SUBARCH_C3B
#define FPCR_INIT		FPCR_DYN_NORMAL
#else
#define FPCR_INIT		(FPCR_DYN_NORMAL | FPCR_DNOE)
#endif

/* status bit coming from hardware fpcr . definde by fire3 */
#define FPCR_STATUS_INV0	(1UL << 52)
#define FPCR_STATUS_DZE0	(1UL << 53)
#define FPCR_STATUS_OVF0	(1UL << 54)
#define FPCR_STATUS_UNF0	(1UL << 55)
#define FPCR_STATUS_INE0	(1UL << 56)
#define FPCR_STATUS_OVI0	(1UL << 57)

#define FPCR_STATUS_INV1	(1UL << 36)
#define FPCR_STATUS_DZE1	(1UL << 37)
#define FPCR_STATUS_OVF1	(1UL << 38)
#define FPCR_STATUS_UNF1	(1UL << 39)
#define FPCR_STATUS_INE1	(1UL << 40)
#define FPCR_STATUS_OVI1	(1UL << 41)

#define FPCR_STATUS_INV2	(1UL << 20)
#define FPCR_STATUS_DZE2	(1UL << 21)
#define FPCR_STATUS_OVF2	(1UL << 22)
#define FPCR_STATUS_UNF2	(1UL << 23)
#define FPCR_STATUS_INE2	(1UL << 24)
#define FPCR_STATUS_OVI2	(1UL << 25)

#define FPCR_STATUS_INV3	(1UL << 4)
#define FPCR_STATUS_DZE3	(1UL << 5)
#define FPCR_STATUS_OVF3	(1UL << 6)
#define FPCR_STATUS_UNF3	(1UL << 7)
#define FPCR_STATUS_INE3	(1UL << 8)
#define FPCR_STATUS_OVI3	(1UL << 9)

#define FPCR_STATUS_MASK0	(FPCR_STATUS_INV0 | FPCR_STATUS_DZE0 |  \
				 FPCR_STATUS_OVF0 | FPCR_STATUS_UNF0 |	\
				 FPCR_STATUS_INE0 | FPCR_STATUS_OVI0)

#define FPCR_STATUS_MASK1	(FPCR_STATUS_INV1 | FPCR_STATUS_DZE1 |  \
				 FPCR_STATUS_OVF1 | FPCR_STATUS_UNF1 |	\
				 FPCR_STATUS_INE1 | FPCR_STATUS_OVI1)

#define FPCR_STATUS_MASK2	(FPCR_STATUS_INV2 | FPCR_STATUS_DZE2 |  \
				 FPCR_STATUS_OVF2 | FPCR_STATUS_UNF2 |	\
				 FPCR_STATUS_INE2 | FPCR_STATUS_OVI2)

#define FPCR_STATUS_MASK3	(FPCR_STATUS_INV3 | FPCR_STATUS_DZE3 |  \
				 FPCR_STATUS_OVF3 | FPCR_STATUS_UNF3 |	\
				 FPCR_STATUS_INE3 | FPCR_STATUS_OVI3)


/*
 * IEEE trap enables are implemented in software.  These per-thread
 * bits are stored in the "ieee_state" field of "struct thread_info".
 * Thus, the bits are defined so as not to conflict with the
 * floating-point enable bit (which is architected).
 */
#define IEEE_TRAP_ENABLE_INV	(1UL << 1)	/* invalid op */
#define IEEE_TRAP_ENABLE_DZE	(1UL << 2)	/* division by zero */
#define IEEE_TRAP_ENABLE_OVF	(1UL << 3)	/* overflow */
#define IEEE_TRAP_ENABLE_UNF	(1UL << 4)	/* underflow */
#define IEEE_TRAP_ENABLE_INE	(1UL << 5)	/* inexact */
#define IEEE_TRAP_ENABLE_DNO	(1UL << 6)	/* denorm */
#define IEEE_TRAP_ENABLE_MASK	(IEEE_TRAP_ENABLE_INV | IEEE_TRAP_ENABLE_DZE |\
				 IEEE_TRAP_ENABLE_OVF | IEEE_TRAP_ENABLE_UNF |\
				 IEEE_TRAP_ENABLE_INE | IEEE_TRAP_ENABLE_DNO)

/* Denorm and Underflow flushing */
#define IEEE_MAP_DMZ		(1UL << 12)	/* Map denorm inputs to zero */
#define IEEE_MAP_UMZ		(1UL << 13)	/* Map underflowed outputs to zero */

#define IEEE_MAP_MASK		(IEEE_MAP_DMZ | IEEE_MAP_UMZ)

/* status bits coming from fpcr: */
#define IEEE_STATUS_INV		(1UL << 17)
#define IEEE_STATUS_DZE		(1UL << 18)
#define IEEE_STATUS_OVF		(1UL << 19)
#define IEEE_STATUS_UNF		(1UL << 20)
#define IEEE_STATUS_INE		(1UL << 21)
#define IEEE_STATUS_DNO		(1UL << 22)


#define IEEE_STATUS_MASK	(IEEE_STATUS_INV | IEEE_STATUS_DZE |	\
				 IEEE_STATUS_OVF | IEEE_STATUS_UNF |	\
				 IEEE_STATUS_INE | IEEE_STATUS_DNO)

#define IEEE_SW_MASK		(IEEE_TRAP_ENABLE_MASK |		\
				 IEEE_STATUS_MASK | IEEE_MAP_MASK)

#define IEEE_CURRENT_RM_SHIFT	32
#define IEEE_CURRENT_RM_MASK	(3UL << IEEE_CURRENT_RM_SHIFT)

#define IEEE_STATUS_TO_EXCSUM_SHIFT	16

#define IEEE_INHERIT    (1UL << 63)	/* inherit on thread create? */

/* ieee_state expand to surport simd added by fire3 */

#define IEEE_STATUS_INV0	(1UL << 17)
#define IEEE_STATUS_DZE0	(1UL << 18)
#define IEEE_STATUS_OVF0	(1UL << 19)
#define IEEE_STATUS_UNF0	(1UL << 20)
#define IEEE_STATUS_INE0	(1UL << 21)
#define IEEE_STATUS_DNO0	(1UL << 22)
#define IEEE_STATUS_MASK0	(IEEE_STATUS_INV0 | IEEE_STATUS_DZE0 |	\
				 IEEE_STATUS_OVF0 | IEEE_STATUS_UNF0 |	\
				 IEEE_STATUS_INE0 | IEEE_STATUS_DNO0)

#define IEEE_STATUS0_TO_EXCSUM_SHIFT	16

#define IEEE_STATUS_INV1	(1UL << 23)
#define IEEE_STATUS_DZE1	(1UL << 24)
#define IEEE_STATUS_OVF1	(1UL << 25)
#define IEEE_STATUS_UNF1	(1UL << 26)
#define IEEE_STATUS_INE1	(1UL << 27)
#define IEEE_STATUS_DNO1	(1UL << 28)
#define IEEE_STATUS_MASK1	(IEEE_STATUS_INV1 | IEEE_STATUS_DZE1 |	\
				 IEEE_STATUS_OVF1 | IEEE_STATUS_UNF1 |	\
				 IEEE_STATUS_INE1 | IEEE_STATUS_DNO1)

#define IEEE_STATUS1_TO_EXCSUM_SHIFT	22

#define IEEE_STATUS_INV2	(1UL << 34)
#define IEEE_STATUS_DZE2	(1UL << 35)
#define IEEE_STATUS_OVF2	(1UL << 36)
#define IEEE_STATUS_UNF2	(1UL << 37)
#define IEEE_STATUS_INE2	(1UL << 38)
#define IEEE_STATUS_DNO2	(1UL << 39)
#define IEEE_STATUS_MASK2	(IEEE_STATUS_INV2 | IEEE_STATUS_DZE2 |	\
				 IEEE_STATUS_OVF2 | IEEE_STATUS_UNF2 |	\
				 IEEE_STATUS_INE2 | IEEE_STATUS_DNO2)

#define IEEE_STATUS2_TO_EXCSUM_SHIFT	33

#define IEEE_STATUS_INV3	(1UL << 40)
#define IEEE_STATUS_DZE3	(1UL << 41)
#define IEEE_STATUS_OVF3	(1UL << 42)
#define IEEE_STATUS_UNF3	(1UL << 43)
#define IEEE_STATUS_INE3	(1UL << 44)
#define IEEE_STATUS_DNO3	(1UL << 45)
#define IEEE_STATUS_MASK3	(IEEE_STATUS_INV3 | IEEE_STATUS_DZE3 |	\
				 IEEE_STATUS_OVF3 | IEEE_STATUS_UNF3 |	\
				 IEEE_STATUS_INE3 | IEEE_STATUS_DNO3)

#define IEEE_STATUS3_TO_EXCSUM_SHIFT	39


/*
 * Convert the software IEEE trap enable and status bits into the
 * hardware fpcr format.
 *
 * Digital Unix engineers receive my thanks for not defining the
 * software bits identical to the hardware bits.  The chip designers
 * receive my thanks for making all the not-implemented fpcr bits
 * RAZ forcing us to use system calls to read/write this value.
 */
static inline unsigned long
ieee_swcr_to_fpcr(unsigned long sw)
{
	unsigned long fp;

	fp = (sw & IEEE_STATUS_MASK0) << 35;
	fp |= (sw & IEEE_STATUS_MASK1) << 13;
	fp |= (sw & IEEE_STATUS_MASK2) >> 14;
	fp |= (sw & IEEE_STATUS_MASK3) >> 36;

	fp |= (sw & IEEE_MAP_DMZ) << 36;
	fp |= (sw & IEEE_STATUS_MASK0 ? FPCR_SUM : 0);
	fp |= (sw & IEEE_STATUS_MASK1 ? FPCR_SUM : 0);
	fp |= (sw & IEEE_STATUS_MASK2 ? FPCR_SUM : 0);
	fp |= (sw & IEEE_STATUS_MASK3 ? FPCR_SUM : 0);
	fp |= (~sw & (IEEE_TRAP_ENABLE_INV
				| IEEE_TRAP_ENABLE_DZE
				| IEEE_TRAP_ENABLE_OVF)) << 48;
	fp |= (~sw & (IEEE_TRAP_ENABLE_UNF | IEEE_TRAP_ENABLE_INE)) << 57;
	fp |= (sw & IEEE_MAP_UMZ ? FPCR_UNDZ | FPCR_UNFD : 0);
	fp |= (~sw & IEEE_TRAP_ENABLE_DNO) << 41;
	return fp;
}

static inline unsigned long
ieee_fpcr_to_swcr(unsigned long fp)
{
	unsigned long sw;

	sw = (fp >> 35) & IEEE_STATUS_MASK;
	sw |= (fp >> 36) & IEEE_MAP_DMZ;
	sw |= (~fp >> 48) & (IEEE_TRAP_ENABLE_INV
			| IEEE_TRAP_ENABLE_DZE
			| IEEE_TRAP_ENABLE_OVF);
	sw |= (~fp >> 57) & (IEEE_TRAP_ENABLE_UNF | IEEE_TRAP_ENABLE_INE);
	sw |= (fp >> 47) & IEEE_MAP_UMZ;
	sw |= (~fp >> 41) & IEEE_TRAP_ENABLE_DNO;
	return sw;
}
#endif /* _UAPI_ASM_SW64_FPU_H */

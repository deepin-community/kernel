/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_WORD_AT_A_TIME_H
#define _ASM_SW64_WORD_AT_A_TIME_H

#include <asm/compiler.h>

/*
 * word-at-a-time interface for SW64.
 */

/*
 * We do not use the word_at_a_time struct on SW64, but it needs to be
 * implemented to humour the generic code.
 */
struct word_at_a_time {
	const unsigned long unused;
};

#define WORD_AT_A_TIME_CONSTANTS { 0 }

/* Return nonzero if val has a zero */
static inline unsigned long has_zero(unsigned long val, unsigned long *bits, const struct word_at_a_time *c)
{
	unsigned long zero_locations = __kernel_cmpgeb(0, val);
	*bits = zero_locations;
	return zero_locations;
}

static inline unsigned long prep_zero_mask(unsigned long val, unsigned long bits, const struct word_at_a_time *c)
{
	return bits;
}

#define create_zero_mask(bits) (bits)

static inline unsigned long find_zero(unsigned long bits)
{
	return __kernel_cttz(bits);
}

#define zero_bytemask(mask) ((2ul << (find_zero(mask) * 8)) - 1)

#endif /* _ASM_SW64_WORD_AT_A_TIME_H */

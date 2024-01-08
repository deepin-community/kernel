// SPDX-License-Identifier: GPL-2.0
/*
 * csum_partial_copy - do IP checksumming and copy
 *
 * (C) Copyright 1996 Linus Torvalds
 *
 * Don't look at this too closely - you'll go mad. The things
 * we do for performance..
 */

#include <linux/types.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <asm/checksum.h>


#define ldl_u(x, y) \
	__asm__ __volatile__("ldl_u %0, %1":"=r" (x):"m" (*(const unsigned long *)(y)))

#define stl_u(x, y) \
	__asm__ __volatile__("stl_u %1, %0":"=m" (*(unsigned long *)(y)):"r" (x))

static inline void stll_u(unsigned long data, unsigned long *dst)
{
	int i = 0;
	unsigned long doff = (unsigned long)dst & 7;

	for (; doff < 8; i++, doff++)
		*((char *)dst + i) = *((char *)&data + i);
}

static inline void sthl_u(unsigned long data, unsigned long *dst)
{
	int i = 0;
	unsigned long doff = (unsigned long)dst & 7;

	for (; i < doff; i++)
		*((char *)dst + 8 - doff + i) = *((char *)&data + 8 - doff + i);
}

#define __get_word(insn, x, ptr)			\
({							\
	long __guu_err;					\
	__asm__ __volatile__(				\
	"1:	"#insn" %0,%2\n"			\
	"2:\n"						\
	".section __ex_table,\"a\"\n"			\
	"	.long 1b - .\n"				\
	"	ldi %0,2b-1b(%1)\n"			\
	".previous"					\
		: "=r"(x), "=r"(__guu_err)		\
		: "m"(__m(ptr)), "1"(0));		\
	__guu_err;					\
})

static inline unsigned long
csum_partial_cfu_dest_aligned(const unsigned long __user *src,
		unsigned long *dst, long len)
{
	unsigned long word;
	unsigned long checksum = ~0U;
	int err = 0;

	err = __copy_from_user(dst, src, len+8);

	while (len > 0) {
		word = *dst;
		checksum += word;
		checksum += (checksum < word);
		dst++;
		len -= 8;
	}
	len += 8;
	word = *dst;

	if (len != 8)
		maskll(word, len, word);
	checksum += word;
	checksum += (checksum < word);

	return checksum;
}

static inline unsigned long
csum_partial_cfu_dest_unaligned(const unsigned long __user *src,
		unsigned long *dst, unsigned long doff, long len)
{
	unsigned long word, patch;
	unsigned long partial_dest, second_dest;
	unsigned long checksum = ~0U;
	int err = 0;

	err = __copy_from_user(dst, src, len+8);

	dst = (unsigned long *)((unsigned long)dst & (~7UL));
	word = *dst;
	inshl(word, 8 - doff, partial_dest);
	dst++;

	while (len >= 0) {
		word = *dst;
		insll(word, 8 - doff, second_dest);
		patch = partial_dest | second_dest;
		checksum += patch;
		checksum += (checksum < patch);
		inshl(word, 8 - doff, partial_dest);
		dst++;
		len -= 8;
	}

	len += 8;
	word = *dst;
	insll(word, 8 - doff, second_dest);
	patch = partial_dest | second_dest;
	maskll(patch, len, patch);
	checksum += patch;
	checksum += (checksum < patch);

	return checksum;
}

static __wsum __csum_and_copy(const void __user *src, void *dst, int len)
{
	unsigned long checksum;
	unsigned long doff = 7 & (unsigned long) dst;

	if (!doff) {
		checksum = csum_partial_cfu_dest_aligned(
			(const unsigned long __user *) src,
			(unsigned long *) dst, len-8);
	} else {
		checksum = csum_partial_cfu_dest_unaligned(
			(const unsigned long __user *) src,
			(unsigned long *) dst, doff, len-8);
	}
	return (__force __wsum)from64to16(checksum);
}

__wsum
csum_and_copy_from_user(const void __user *src, void *dst, int len)
{
	if (!access_ok(src, len))
		return 0;
	return __csum_and_copy(src, dst, len);
}
EXPORT_SYMBOL(csum_and_copy_from_user);

__wsum
csum_partial_copy_nocheck(const void *src, void *dst, int len)
{
	return __csum_and_copy((__force const void __user *)src,
			dst, len);
}
EXPORT_SYMBOL(csum_partial_copy_nocheck);

// SPDX-License-Identifier: GPL-2.0
/*
 * This file contains network checksum routines that are better done
 * in an architecture-specific manner due to speed..
 * Comments in other versions indicate that the algorithms are from RFC1071
 */
#include <linux/module.h>
#include <linux/string.h>
#include <asm/byteorder.h>
#include <asm/checksum.h>

/*
 * computes the checksum of the TCP/UDP pseudo-header
 * returns a 16-bit checksum, already complemented.
 */
__sum16 csum_tcpudp_magic(__be32 saddr, __be32 daddr,
			  __u32 len, __u8 proto, __wsum sum)
{
	return (__force __sum16)~from64to16(
		(__force u64)saddr + (__force u64)daddr +
		(__force u64)sum + ((len + proto) << 8));
}
EXPORT_SYMBOL(csum_tcpudp_magic);

__wsum csum_tcpudp_nofold(__be32 saddr, __be32 daddr,
			  __u32 len, __u8 proto, __wsum sum)
{
	unsigned long result;

	result = (__force u64)saddr + (__force u64)daddr +
		 (__force u64)sum + ((len + proto) << 8);

	/*
	 * Fold down to 32-bits so we don't lose in the typedef-less
	 * network stack.
	 *
	 * 64 to 33
	 */
	result = (result & 0xffffffff) + (result >> 32);
	/* 33 to 32 */
	result = (result & 0xffffffff) + (result >> 32);
	return (__force __wsum)result;
}
EXPORT_SYMBOL(csum_tcpudp_nofold);

/*
 * Do a 64-bit checksum on an arbitrary memory area..
 */
static inline unsigned long do_csum(const unsigned char *buff, int len)
{
	const unsigned long *dst = (unsigned long *)buff;
	unsigned long doff = 7 & (unsigned long) dst;
	unsigned long checksum = 0;
	unsigned long word, patch;
	unsigned long partial_dest, second_dest;

	len -= 8;

	if (!doff) {
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
	} else {
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
	}

	return from64to16(checksum);
}

/*
 *	This is a version of ip_compute_csum() optimized for IP headers,
 *	which always checksum on 4 octet boundaries.
 */
__sum16 ip_fast_csum(const void *iph, unsigned int ihl)
{
	return (__force __sum16)~do_csum(iph, ihl*4);
}
EXPORT_SYMBOL(ip_fast_csum);

/*
 * computes the checksum of a memory block at buff, length len,
 * and adds in "sum" (32-bit)
 *
 * returns a 32-bit number suitable for feeding into itself
 * or csum_tcpudp_magic
 *
 * this function must be called with even lengths, except
 * for the last fragment, which may be odd
 *
 * it's best to have buff aligned on a 32-bit boundary
 */
__wsum csum_partial(const void *buff, int len, __wsum sum)
{
	unsigned long result = do_csum(buff, len);

	/* add in old sum, and carry.. */
	result += (__force u32)sum;
	/* 32+c bits -> 32 bits */
	result = (result & 0xffffffff) + (result >> 32);
	return (__force __wsum)result;
}
EXPORT_SYMBOL(csum_partial);

/*
 * this routine is used for miscellaneous IP-like checksums, mainly
 * in icmp.c
 */
__sum16 ip_compute_csum(const void *buff, int len)
{
	return (__force __sum16)~from64to16(do_csum(buff, len));
}
EXPORT_SYMBOL(ip_compute_csum);

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_CHECKSUM_H
#define _ASM_SW64_CHECKSUM_H

#include <linux/in6.h>

#define extll(x, y, z) \
	({__asm__ __volatile__("extll %1, %2, %0" : "=r" (z) \
			       : "r" (x), "r" (y)); })

#define exthl(x, y, z) \
	({__asm__ __volatile__("exthl %1, %2, %0" : "=r" (z) \
			       : "r" (x), "r" (y)); })

#define maskll(x, y, z) \
	({__asm__ __volatile__("maskll %1, %2, %0" : "=r" (z) \
			       : "r" (x), "r" (y)); })

#define maskhl(x, y, z) \
	({__asm__ __volatile__("maskhl %1, %2, %0" : "=r" (z) \
			       : "r" (x), "r" (y)); })

#define insll(x, y, z) \
	({__asm__ __volatile__("insll %1, %2, %0" : "=r" (z) \
			       : "r" (x), "r" (y)); })

#define inshl(x, y, z) \
	({__asm__ __volatile__("inshl %1, %2, %0" : "=r" (z) \
			       : "r" (x), "r" (y)); })

/*
 * This is a version of ip_compute_csum() optimized for IP headers,
 * which always checksum on 4 octet boundaries.
 */
extern __sum16 ip_fast_csum(const void *iph, unsigned int ihl);

/*
 * computes the checksum of the TCP/UDP pseudo-header
 * returns a 16-bit checksum, already complemented
 */
__sum16 csum_tcpudp_magic(__be32 saddr, __be32 daddr,
			  __u32 len, __u8 proto, __wsum sum);

__wsum csum_tcpudp_nofold(__be32 saddr, __be32 daddr,
			  __u32 len, __u8 proto, __wsum sum);

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
extern __wsum csum_partial(const void *buff, int len, __wsum sum);

/*
 * the same as csum_partial, but copies from src while it
 * checksums
 *
 * here even more important to align src and dst on a 32-bit (or even
 * better 64-bit) boundary
 */
#define _HAVE_ARCH_COPY_AND_CSUM_FROM_USER
#define _HAVE_ARCH_CSUM_AND_COPY
__wsum csum_and_copy_from_user(const void __user *src, void *dst, int len);

__wsum csum_partial_copy_nocheck(const void *src, void *dst, int len);

/*
 * this routine is used for miscellaneous IP-like checksums, mainly
 * in icmp.c
 */

extern __sum16 ip_compute_csum(const void *buff, int len);

/*
 * Fold a partial checksum without adding pseudo headers
 */

static inline __sum16 csum_fold(__wsum csum)
{
	u32 sum = (__force u32)csum;

	sum = (sum & 0xffff) + (sum >> 16);
	sum = (sum & 0xffff) + (sum >> 16);
	return (__force __sum16)~sum;
}

#define _HAVE_ARCH_IPV6_CSUM
extern __sum16 csum_ipv6_magic(const struct in6_addr *saddr,
			       const struct in6_addr *daddr, __u32 len,
			       __u8 proto, __wsum sum);

static inline unsigned short from64to16(unsigned long x)
{
	/*
	 * Using extract instructions is a bit more efficient
	 * than the original shift/bitmask version.
	 */

	union {
		unsigned long	ul;
		unsigned int	ui[2];
		unsigned short	us[4];
	} in_v, tmp_v, out_v;

	in_v.ul = x;
	tmp_v.ul = (unsigned long)in_v.ui[0] + (unsigned long)in_v.ui[1];

	/*
	 * Since the bits of tmp_v.sh[3] are going to always be zero,
	 * we don't have to bother to add that in.
	 */
	out_v.ul = (unsigned long)tmp_v.us[0] + (unsigned long)tmp_v.us[1]
			+ (unsigned long)tmp_v.us[2];

	/* Similarly, out_v.us[2] is always zero for the final add.  */
	return out_v.us[0] + out_v.us[1];
}

#endif /* _ASM_SW64_CHECKSUM_H */

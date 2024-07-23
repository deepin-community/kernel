/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_SW64_CACHE_H
#define _ASM_SW64_CACHE_H

#define L1_CACHE_SHIFT		7
#define L1_CACHE_BYTES		(1 << L1_CACHE_SHIFT)

#ifndef __ASSEMBLY__

enum sunway_cache_type {
	L1_ICACHE = 0,
	L1_DCACHE = 1,
	L2_CACHE  = 2,
	L3_CACHE  = 3
};

#endif

#endif /* _ASM_SW64_CACHE_H */

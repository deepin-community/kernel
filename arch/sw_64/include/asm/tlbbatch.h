/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ASM_SW_64_TLBBATCH_H
#define _ASM_SW_64_TLBBATCH_H

#include <linux/cpumask.h>

struct arch_tlbflush_unmap_batch {
	struct cpumask cpumask;
};

#endif /* _ASM_SW_64_TLBBATCH_H */

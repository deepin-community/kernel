/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_KVM_CMA_H
#define _ASM_SW64_KVM_CMA_H

#include <linux/cma.h>

extern int __init kvm_cma_declare_contiguous(phys_addr_t base,
			phys_addr_t size, phys_addr_t limit,
			phys_addr_t alignment, unsigned int order_per_bit,
			const char *name, struct cma **res_cma);
#endif /* _ASM_SW64_KVM_CMA_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_EARLY_IOREMAP_H
#define _ASM_SW64_EARLY_IOREMAP_H

#include <asm/page.h>
#include <asm/io.h>

static inline void  __iomem *
early_ioremap(unsigned long phys_addr, unsigned long size)
{
	unsigned long y = 0;

	if (phys_addr >= __START_KERNEL_map) {
		y = (unsigned long) phys_to_virt(__pa(phys_addr));
	} else {
		y = phys_addr;
		y |= PAGE_OFFSET;
	}

	return  (void __iomem *) y;
}
#define early_memremap(phys_addr, size)		early_ioremap(phys_addr, size)

static inline void early_iounmap(volatile void __iomem *addr, unsigned long size)
{
}
#define early_memunmap(addr, size)		early_iounmap(addr, size)

#endif /* _ASM_SW64_EARLY_IOREMAP_H */

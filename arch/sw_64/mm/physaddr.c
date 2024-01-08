// SPDX-License-Identifier: GPL-2.0
#include <linux/mmdebug.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <asm/page.h>

unsigned long __phys_addr(unsigned long addr)
{
	VIRTUAL_BUG_ON(addr < PAGE_OFFSET);
	addr &= ~PAGE_OFFSET;
	VIRTUAL_BUG_ON(!phys_addr_valid(addr));
	return addr;
}
EXPORT_SYMBOL(__phys_addr);

bool __virt_addr_valid(unsigned long addr)
{
	if (addr < PAGE_OFFSET)
		return false;
	addr &= ~PAGE_OFFSET;
	return pfn_valid(addr >> PAGE_SHIFT);
}
EXPORT_SYMBOL(__virt_addr_valid);

#ifdef CONFIG_SUBARCH_C3B
#define LEGACY_BOOT_VA		0xffffffff80000000
unsigned long __boot_phys_addr(unsigned long addr)
{
	if (addr >= LEGACY_BOOT_VA) {
		addr &= ~LEGACY_BOOT_VA;
		VIRTUAL_BUG_ON(addr >= KERNEL_IMAGE_SIZE);
	} else {
		VIRTUAL_BUG_ON(addr < PAGE_OFFSET);
		addr &= ~PAGE_OFFSET;
		VIRTUAL_BUG_ON(!phys_addr_valid(addr));
	}
	return addr;
}
#endif

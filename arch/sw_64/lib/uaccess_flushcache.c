// SPDX-License-Identifier: GPL-2.0

#include <linux/uaccess.h>
#include <linux/mm.h>
#include <asm/barrier.h>
#include <asm/cacheflush.h>

void memcpy_flushcache(void *dst, const void *src, size_t cnt)
{
	memcpy(dst, src, cnt);
	flush_cache_all();
}
EXPORT_SYMBOL_GPL(memcpy_flushcache);

void memcpy_page_flushcache(char *to, struct page *page, size_t offset,
			    size_t len)
{
	memcpy_flushcache(to, page_address(page) + offset, len);
}

unsigned long __copy_user_flushcache(void *to, const void __user *from,
				     unsigned long n)
{
	unsigned long rc = __copy_from_user(to, from, n);

	flush_cache_all();
	return rc;
}

#ifdef CONFIG_ARCH_HAS_PMEM_API
void arch_wb_cache_pmem(void *addr, size_t size)
{
	flush_cache_all();
}
EXPORT_SYMBOL_GPL(arch_wb_cache_pmem);

void arch_invalidate_pmem(void *addr, size_t size)
{
	flush_cache_all();
}
EXPORT_SYMBOL_GPL(arch_invalidate_pmem);
#endif

// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#include <asm/haoc/iee.h>
#ifdef CONFIG_IEE_PTRP
#include <asm/haoc/iee-token.h>
#endif
#include <linux/mm.h>

void _iee_memcpy(unsigned long __unused, void *dst, void *src, size_t n)
{
	memcpy(dst, src, n);
}

void _iee_memset(unsigned long __unused, void *ptr, int data, size_t n)
{
	memset(ptr, data, n);
}

void _iee_set_freeptr(unsigned long __unused, void **pptr, void *ptr)
{
	*pptr = ptr;
}

unsigned long _iee_test_and_clear_bit(unsigned long __unused, long nr, unsigned long *addr)
{
	kcsan_mb();
	instrument_atomic_read_write(addr + BIT_WORD(nr), sizeof(long));
	return arch_test_and_clear_bit(nr, addr);
}

#ifdef CONFIG_IEE_PTRP
struct task_token *iee_get_task_token(struct task_struct *task)
{
	unsigned long slab_addr;
	unsigned long task_addr;
	unsigned int index;

	slab_addr = (unsigned long)page_to_virt(virt_to_head_page(task));
	task_addr = (unsigned long)page_to_virt(virt_to_page(task));
	index = (task_addr - slab_addr) / PAGE_SIZE;

	return (struct task_token *)((unsigned long)__phys_to_iee(__pa(slab_addr)) +
				     index * IEE_TOKEN_BLOCK_SIZE);
}
#endif

// SPDX-License-Identifier: GPL-2.0

#include <linux/errno.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/syscalls.h>

#include <asm/current.h>

unsigned long
arch_get_unmapped_area(struct file *filp, unsigned long addr,
		       unsigned long len, unsigned long pgoff,
		       unsigned long flags)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	struct vm_unmapped_area_info info;
	unsigned long limit;

	/* Support 32 bit heap. */
	if (current->personality & ADDR_LIMIT_32BIT)
		limit = 0x80000000;
	else
		limit = TASK_SIZE;

	if (len > limit)
		return -ENOMEM;

	if (flags & MAP_FIXED) {
		if (addr + len > TASK_SIZE)
			return -EINVAL;

		return addr;
	}

	if (addr) {
		addr = PAGE_ALIGN(addr);

		vma = find_vma(mm, addr);
		if (TASK_SIZE - len >= addr &&
		    (!vma || addr + len <= vm_start_gap(vma)))
			return addr;
	}

	info.flags = 0;
	info.length = len;
	info.low_limit = mm->mmap_base;
	info.high_limit = limit;
	info.align_mask = 0;
	info.align_offset = pgoff << PAGE_SHIFT;

	return vm_unmapped_area(&info);
}

unsigned long arch_mmap_rnd(void)
{
	unsigned long rnd;

	/* 8MB for 32bit, 256MB for 64bit */
	if (current->personality & ADDR_LIMIT_32BIT)
		rnd = get_random_long() & 0x7ffffful;
	else
		rnd = get_random_long() & 0xffffffful;

	return rnd << PAGE_SHIFT;
}

/*
 * This function, called very early during the creation of a new process VM
 * image, sets up which VM layout function to use:
 */
void arch_pick_mmap_layout(struct mm_struct *mm, struct rlimit *rlim_stack)
{
	unsigned long random_factor = 0UL;

	if (current->flags & PF_RANDOMIZE)
		random_factor = arch_mmap_rnd();

	/*
	 * Fall back to the standard layout if the personality bit is set, or
	 * if the expected stack growth is unlimited:
	 */
	mm->mmap_base = TASK_UNMAPPED_BASE + random_factor;
	mm->get_unmapped_area = arch_get_unmapped_area;
}

SYSCALL_DEFINE6(mmap, unsigned long, addr, unsigned long, len,
		unsigned long, prot, unsigned long, flags, unsigned long, fd,
		unsigned long, off)
{
	unsigned long ret = -EINVAL;

	if ((off + PAGE_ALIGN(len)) < off)
		goto out;
	if (off & ~PAGE_MASK)
		goto out;
	ret = ksys_mmap_pgoff(addr, len, prot, flags, fd, off >> PAGE_SHIFT);
 out:
	return ret;
}

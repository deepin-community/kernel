// SPDX-License-Identifier: GPL-2.0

#include <linux/errno.h>
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/sizes.h>
#include <linux/syscalls.h>
#include <linux/security.h>

#include <asm/current.h>

/*
 * Top of mmap area (just below the process stack).
 * Leave at least a ~128 MB hole.
 */

#define MIN_GAP	(SZ_128M)
#define MAX_GAP	(STACK_TOP / 6 * 5)

unsigned long
arch_get_unmapped_area(struct file *filp, unsigned long addr,
		       unsigned long len, unsigned long pgoff,
		       unsigned long flags)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	struct vm_unmapped_area_info info;
	const unsigned long mmap_end = arch_get_mmap_end(addr, len, flags);

	if (unlikely(len > mmap_end - mmap_min_addr))
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
	info.high_limit = mmap_end;
	info.align_mask = 0;
	info.align_offset = pgoff << PAGE_SHIFT;

	return vm_unmapped_area(&info);
}

unsigned long
arch_get_unmapped_area_topdown(struct file *filp, unsigned long addr,
		       unsigned long len, unsigned long pgoff,
		       unsigned long flags)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	struct vm_unmapped_area_info info;
	const unsigned long mmap_end = arch_get_mmap_end(addr, len, flags);

	if (unlikely(len > mmap_end - mmap_min_addr))
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

	info.flags = VM_UNMAPPED_AREA_TOPDOWN;
	info.length = len;
	info.low_limit = FIRST_USER_ADDRESS;
	info.high_limit = arch_get_mmap_base(addr, mm->mmap_base);
	info.align_mask = 0;
	info.align_offset = pgoff << PAGE_SHIFT;
	addr = vm_unmapped_area(&info);

	/*
	 * A failed mmap() very likely causes application failure,
	 * so fall back to the bottom-up function here. This scenario
	 * can happen with large stack limits and large mmap()
	 * allocations.
	 */
	if (addr & ~PAGE_MASK) {
		VM_BUG_ON(addr != -ENOMEM);
		info.flags = 0;
		info.low_limit = mm->mmap_base;
		info.high_limit = mmap_end;
		addr = vm_unmapped_area(&info);
	}
	return addr;
}

unsigned long arch_mmap_rnd(void)
{
	unsigned long rnd = get_random_long() & 0x7fffffful;

	return rnd << PAGE_SHIFT;
}

unsigned long mmap_is_legacy(struct rlimit *rlim_stack)
{
	if (current->personality & ADDR_COMPAT_LAYOUT)
		return 1;
	if (rlim_stack->rlim_cur == RLIM_INFINITY)
		return 1;

	return sysctl_legacy_va_layout;
}

static unsigned long mmap_base(unsigned long rnd, struct rlimit *rlim_stack)
{
	unsigned long gap = rlim_stack->rlim_cur;
	unsigned long pad = stack_guard_gap;

	/* Account for stack randomization if necessary. 8M of VA. */
	if (current->flags & PF_RANDOMIZE)
		pad += 0x7ff00;
	/* Values close to RLIM_INFINITY can overflow. */
	if (gap + pad > gap)
		gap = MIN_GAP;
	else if (gap > MAX_GAP)
		gap = MAX_GAP;

	return PAGE_ALIGN(STACK_TOP - gap - rnd);
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
	if (mmap_is_legacy(rlim_stack)) {
		mm->mmap_base = TASK_UNMAPPED_BASE + random_factor;
		mm->get_unmapped_area = arch_get_unmapped_area;
	} else {
		mm->mmap_base = mmap_base(random_factor, rlim_stack);
		mm->get_unmapped_area = arch_get_unmapped_area_topdown;
	}
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

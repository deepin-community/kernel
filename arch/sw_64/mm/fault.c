// SPDX-License-Identifier: GPL-2.0
/*
 *  Copyright (C) 1995  Linus Torvalds
 */

#include <linux/extable.h>
#include <linux/perf_event.h>
#include <linux/kprobes.h>

#include <asm/mmu_context.h>

__read_mostly bool segv_debug_enabled;

#ifdef CONFIG_KPROBES
static inline int notify_page_fault(struct pt_regs *regs, unsigned long mmcsr)
{
	int ret = 0;
	/* kprobe_running() needs smp_processor_id() */
	if (!user_mode(regs)) {
		preempt_disable();
		if (kprobe_running() && kprobe_fault_handler(regs, mmcsr))
			ret = 1;
		preempt_enable();
	}
	return ret;
}
#else
static inline int notify_page_fault(struct pt_regs *regs, unsigned long mmcsr)
{
	return 0;
}
#endif

extern void die(char *, struct pt_regs *, long);
extern void show_regs(struct pt_regs *regs);

void show_all_vma(void)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;

	MA_STATE(mas, 0, 0, 0);

	if (!mm)
		return;

	mas.tree = &mm->mm_mt;

	for (int i = 0;	(vma = mas_find(&mas, ULONG_MAX)) != NULL; i++) {
		unsigned long start = vma->vm_start;
		unsigned long end = vma->vm_end;
		struct file *file = vma->vm_file;

		if (file)
			pr_info("vma[%d]: [%#lx, %#lx], len = %#lx, flags = %#lx, file = %s, name = %s\n",
					i, start, end, (end - start), vma->vm_flags,
					file->f_path.dentry->d_name.name, current->comm);
		else
			pr_info("vma[%d]: [%#lx, %#lx], len = %#lx, flags = %#lx, name = %s\n",
					i, start, end, (end - start), vma->vm_flags, current->comm);
	}
}

/*
 * This routine handles page faults.  It determines the address,
 * and the problem, and then passes it off to handle_mm_fault().
 *
 * mmcsr:
 *	0 = translation not valid
 *	1 = access violation
 *	2 = fault-on-read
 *	3 = fault-on-execute
 *	4 = fault-on-write
 *
 * cause:
 *	-1 = instruction fetch
 *	0 = load
 *	1 = store
 *
 * Registers $9 through $15 are saved in a block just prior to `regs' and
 * are saved and restored around the call to allow exception code to
 * modify them.
 */

unsigned long show_va_to_pa(struct mm_struct *mm, unsigned long addr)
{
	pgd_t *pgd = NULL;
	p4d_t *p4d = NULL;
	pud_t *pud  = NULL;
	pmd_t *pmd = NULL;
	pte_t *pte = NULL;
	unsigned long ret = 0UL;

	pgd = pgd_offset(mm, addr);
	if (pgd_none(*pgd)) {
		ret = 0;
		pr_debug("addr = %#lx, pgd = %#lx\n", addr, pgd_val(*pgd));
		goto out;
	}
	p4d = p4d_offset(pgd, addr);
	if (p4d_none(*p4d)) {
		ret = 0;
		pr_debug("addr = %#lx, pgd = %#lx, p4d = %#lx\n",
				addr, pgd_val(*pgd), p4d_val(*p4d));
		goto out;
	}
	pud = pud_offset(p4d, addr);
	if (pud_none(*pud)) {
		ret = 0;
		pr_debug("addr = %#lx, pgd = %#lx, pud = %#lx\n",
				addr, pgd_val(*pgd), pud_val(*pud));
		goto out;
	}
	pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd)) {
		ret = 0;
		pr_debug("addr = %#lx, pgd = %#lx, pud = %#lx, pmd = %#lx\n",
				addr, pgd_val(*pgd), pud_val(*pud), pmd_val(*pmd));
		goto out;

	}
	pte = pte_offset_map(pmd, addr);
	if (pte_present(*pte)) {
		ret = (unsigned long)pfn_to_virt(pte_pfn(*pte));
		pr_debug("addr = %#lx, pgd = %#lx, pud = %#lx, pmd = %#lx, pte = %#lx, ret = %#lx\n",
				addr, *(unsigned long *)pgd, *(unsigned long *)pud,
				*(unsigned long *)pmd, *(unsigned long *)pte, ret);
	}
out:
	return ret;
}

extern int do_match(unsigned long address, unsigned long mmcsr, long cause, struct pt_regs *regs);

asmlinkage void notrace
do_page_fault(unsigned long address, unsigned long mmcsr,
		long cause, struct pt_regs *regs)
{
	struct vm_area_struct *vma;
	struct mm_struct *mm = current->mm;
	int si_code = SEGV_MAPERR;
	vm_fault_t fault;
	unsigned int flags = FAULT_FLAG_DEFAULT;

	if (notify_page_fault(regs, mmcsr))
		return;

	if (unlikely(mmcsr >= MMCSR__DA_MATCH)) {
		if (do_match(address, mmcsr, cause, regs) == 1)
			return;
	}

	if (unlikely(mmcsr == MMCSR__ACV1)) {
		if (!user_mode(regs))
			goto no_context;
		else {
			mmap_read_unlock(mm);
			goto bad_area;
		}
	}

	/*
	 * If we're in an interrupt context, or have no user context,
	 * we must not take the fault.
	 */
	if (!mm || faulthandler_disabled())
		goto no_context;

	if (user_mode(regs))
		flags |= FAULT_FLAG_USER;

	perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS, 1, regs, address);

retry:
	vma = lock_mm_and_find_vma(mm, address, regs);
	if (!vma)
		goto bad_area_nosemaphore;

	/*
	 * Ok, we have a good vm_area for this memory access, so
	 * we can handle it.
	 */
	si_code = SEGV_ACCERR;
	if (cause < 0) {
		if (!(vma->vm_flags & VM_EXEC))
			goto bad_area;
	} else if (!cause) {
		/* Allow reads even for write-only mappings */
		if (!(vma->vm_flags & (VM_READ | VM_WRITE)))
			goto bad_area;
	} else {
		if (!(vma->vm_flags & VM_WRITE))
			goto bad_area;
		flags |= FAULT_FLAG_WRITE;
	}

	/*
	 * If for any reason at all we couldn't handle the fault,
	 * make sure we exit gracefully rather than endlessly redo
	 * the fault.
	 */
	fault = handle_mm_fault(vma, address, flags, regs);

	if (fault_signal_pending(fault, regs)) {
		if (!user_mode(regs))
			goto no_context;
		return;
	}

	/* The fault is fully completed (including releasing mmap lock) */
	if (fault & VM_FAULT_COMPLETED)
		return;

	if (unlikely(fault & VM_FAULT_ERROR)) {
		if (fault & VM_FAULT_OOM)
			goto out_of_memory;
		else if (fault & VM_FAULT_SIGSEGV)
			goto bad_area;
		else if (fault & VM_FAULT_SIGBUS)
			goto do_sigbus;
		BUG();
	}

	if (fault & VM_FAULT_MAJOR) {
		perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS_MAJ, 1,
				regs, address);
		current->maj_flt++;
	} else {
		perf_sw_event(PERF_COUNT_SW_PAGE_FAULTS_MIN, 1,
				regs, address);
		current->min_flt++;
	}

	if (fault & VM_FAULT_RETRY) {
		flags |= FAULT_FLAG_TRIED;

		/* No need to mmap_read_unlock(mm) as we would
		 * have already released it in __lock_page_or_retry
		 * in mm/filemap.c.
		 */

		goto retry;
	}

	mmap_read_unlock(mm);

	return;

	/*
	 * Something tried to access memory that isn't in our memory map.
	 * Fix it, but check if it's kernel or user first.
	 */
 bad_area:
	mmap_read_unlock(mm);

 bad_area_nosemaphore:
	if (user_mode(regs))
		goto do_sigsegv;

 no_context:
	/* Are we prepared to handle this fault as an exception?  */
	if (fixup_exception(regs, regs->pc))
		return;

	/*
	 * Oops. The kernel tried to access some bad page. We'll have to
	 * terminate things with extreme prejudice.
	 */
	pr_alert("Unable to handle kernel paging request at virtual address %016lx\n",
	       address);
	die("Oops", regs, cause);
	make_task_dead(SIGKILL);

	/*
	 * We ran out of memory, or some other thing happened to us that
	 * made us unable to handle the page fault gracefully.
	 */
 out_of_memory:
	mmap_read_unlock(mm);
	if (!user_mode(regs))
		goto no_context;
	pagefault_out_of_memory();
	return;

 do_sigbus:
	mmap_read_unlock(mm);
	/*
	 * Send a sigbus, regardless of whether we were in kernel
	 * or user mode.
	 */
	force_sig_fault(SIGBUS, BUS_ADRERR, (void __user *) address);
	if (!user_mode(regs))
		goto no_context;
	return;

 do_sigsegv:
	force_sig_fault(SIGSEGV, si_code, (void __user *) address);

	if (unlikely(segv_debug_enabled)) {
		pr_info("fault: want to send_segv: pid %d, cause = %#lx, mmcsr = %#lx, address = %#lx, pc %#lx\n",
				current->pid, cause, mmcsr, address, regs->pc);
		show_regs(regs);
		show_all_vma();
	}
}

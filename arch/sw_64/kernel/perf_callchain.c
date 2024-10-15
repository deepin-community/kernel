// SPDX-License-Identifier: GPL-2.0
/*
 * sw64 callchain support
 *
 * Copyright (C) 2023 SW64 Limited
 */
#include <linux/perf_event.h>
#include <linux/stacktrace.h>

#include <asm/stacktrace.h>

bool valid_utext_addr(unsigned long addr)
{
	return addr >= current->mm->start_code && addr <= current->mm->end_code;
}

bool valid_dy_addr(unsigned long addr)
{
	bool ret = false;
	struct vm_area_struct *vma;
	struct mm_struct *mm = current->mm;

	if (addr > TASK_SIZE || addr < TASK_UNMAPPED_BASE)
		return ret;
	vma = find_vma(mm, addr);
	if (vma && vma->vm_start <= addr && (vma->vm_flags & VM_EXEC))
		ret = true;
	return ret;
}

#ifdef CONFIG_FRAME_POINTER
void perf_callchain_user(struct perf_callchain_entry_ctx *entry,
		struct pt_regs *regs)
{

	struct stack_frame frame;
	unsigned long __user *fp;
	int err;

	perf_callchain_store(entry, regs->pc);

	fp = (unsigned long __user *)regs->regs[15];

	while (entry->nr < entry->max_stack &&
		(unsigned long)fp < current->mm->start_stack) {
		if (!access_ok(fp, sizeof(frame)))
			break;

		pagefault_disable();
		err =  __copy_from_user_inatomic(&frame, fp, sizeof(frame));
		pagefault_enable();

		if (err)
			break;

		if (valid_utext_addr(frame.return_address) ||
			valid_dy_addr(frame.return_address))
			perf_callchain_store(entry, frame.return_address);
		else
			break;
		fp = (void __user *)frame.next_frame;
	}
}
#else /* !CONFIG_FRAME_POINTER */
void perf_callchain_user(struct perf_callchain_entry_ctx *entry,
		struct pt_regs *regs)
{
	unsigned long usp = current_user_stack_pointer();
	unsigned long user_addr;
	int err;

	perf_callchain_store(entry, regs->pc);

	while (entry->nr < entry->max_stack && usp < current->mm->start_stack) {
		if (!access_ok((const void __user *)usp, 8))
			break;

		pagefault_disable();
		err = __get_user(user_addr, (unsigned long *)usp);
		pagefault_enable();

		if (err)
			break;

		if (valid_utext_addr(user_addr) || valid_dy_addr(user_addr))
			perf_callchain_store(entry, user_addr);
		usp = usp + 8;
	}
}
#endif/* CONFIG_FRAME_POINTER */

/*
 * Gets called by walk_stackframe() for every stackframe. This will be called
 * whist unwinding the stackframe and is like a subroutine return so we use
 * the PC.
 */
static int callchain_trace(unsigned long pc, void *data)
{
	struct perf_callchain_entry_ctx *entry = data;

	perf_callchain_store(entry, pc);
	return 0;
}

void perf_callchain_kernel(struct perf_callchain_entry_ctx *entry,
			   struct pt_regs *regs)
{
	walk_stackframe(NULL, regs, callchain_trace, entry);
}

/*
 * Gets the perf_instruction_pointer and perf_misc_flags for guest os.
 */
#undef is_in_guest

unsigned long perf_instruction_pointer(struct pt_regs *regs)
{
	if (perf_guest_state())
		return perf_guest_get_ip();

	return instruction_pointer(regs);
}

unsigned long perf_misc_flags(struct pt_regs *regs)
{
	unsigned int guest_state = perf_guest_state();
	int misc = 0;

	if (guest_state) {
		if (guest_state & PERF_GUEST_USER)
			misc |= PERF_RECORD_MISC_GUEST_USER;
		else
			misc |= PERF_RECORD_MISC_GUEST_KERNEL;
	} else {
		if (user_mode(regs))
			misc |= PERF_RECORD_MISC_USER;
		else
			misc |= PERF_RECORD_MISC_KERNEL;
	}

	return misc;
}

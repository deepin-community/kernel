// SPDX-License-Identifier: GPL-2.0
#include <linux/highmem.h>
#include <linux/kdebug.h>
#include <linux/uprobes.h>
#include <linux/ptrace.h>

/**
 * arch_uprobe_analyze_insn - instruction analysis including validity and fixups.
 * @mm: the probed address space.
 * @arch_uprobe: the probepoint information.
 * @addr: virtual address at which to install the probepoint
 * Return 0 on success or a -ve number on error.
 */
int arch_uprobe_analyze_insn(struct arch_uprobe *aup,
		struct mm_struct *mm, unsigned long addr)
{
	u32 inst;

	if (addr & 0x03)
		return -EINVAL;

	inst = aup->insn;

	aup->ixol[0] = aup->insn;
	aup->ixol[1] = UPROBE_BRK_UPROBE_XOL;		/* NOP  */

	return 0;
}

void arch_uprobe_copy_ixol(struct page *page, unsigned long vaddr,
		void *src, unsigned long len)
{
	unsigned long kaddr, kstart;

	/* Initialize the slot */
	kaddr = (unsigned long)kmap_local_page(page);
	kstart = kaddr + (vaddr & ~PAGE_MASK);
	memcpy((void *)kstart, src, len);
	flush_icache_range(kstart, kstart + len);
	kunmap_local((void *)kaddr);
}

/*
 * arch_uprobe_pre_xol - prepare to execute out of line.
 * @auprobe: the probepoint information.
 * @regs: reflects the saved user state of current task.
 */
int arch_uprobe_pre_xol(struct arch_uprobe *aup, struct pt_regs *regs)
{
	struct uprobe_task *utask = current->utask;

	/* Instruction points to execute ol */
	instruction_pointer_set(regs, utask->xol_vaddr);

	return 0;
}

int arch_uprobe_post_xol(struct arch_uprobe *aup, struct pt_regs *regs)
{
	struct uprobe_task *utask = current->utask;

	/* Instruction points to execute next to breakpoint address */
	instruction_pointer_set(regs, utask->vaddr + 4);

	return 0;
}

/*
 * If xol insn itself traps and generates a signal(Say,
 * SIGILL/SIGSEGV/etc), then detect the case where a singlestepped
 * instruction jumps back to its own address. It is assumed that anything
 * like do_page_fault/do_trap/etc sets thread.trap_nr != -1.
 *
 * arch_uprobe_pre_xol/arch_uprobe_post_xol save/restore thread.trap_nr,
 * arch_uprobe_xol_was_trapped() simply checks that ->trap_nr is not equal to
 * UPROBE_TRAP_NR == -1 set by arch_uprobe_pre_xol().
 */
bool arch_uprobe_xol_was_trapped(struct task_struct *tsk)
{
	return false;
}

int arch_uprobe_exception_notify(struct notifier_block *self,
		unsigned long val, void *data)
{
	struct die_args *args = data;
	struct pt_regs *regs = args->regs;

	/* regs == NULL is a kernel bug */
	if (WARN_ON(!regs))
		return NOTIFY_DONE;

	/* We are only interested in userspace traps */
	if (!user_mode(regs))
		return NOTIFY_DONE;

	switch (val) {
	case DIE_UPROBE:
		if (uprobe_pre_sstep_notifier(regs))
			return NOTIFY_STOP;
		break;
	case DIE_UPROBE_XOL:
		if (uprobe_post_sstep_notifier(regs))
			return NOTIFY_STOP;
	default:
		break;
	}

	return 0;
}

/*
 * This function gets called when XOL instruction either gets trapped or
 * the thread has a fatal signal. Reset the instruction pointer to its
 * probed address for the potential restart or for post mortem analysis.
 */
void arch_uprobe_abort_xol(struct arch_uprobe *aup,
		struct pt_regs *regs)
{
	struct uprobe_task *utask = current->utask;

	instruction_pointer_set(regs, utask->vaddr);
}

unsigned long arch_uretprobe_hijack_return_addr(
		unsigned long trampoline_vaddr, struct pt_regs *regs)
{
	unsigned long ra;

	ra = regs->regs[26];

	/* Replace the return address with the trampoline address */
	regs->regs[26] = trampoline_vaddr;

	return ra;
}

/*
 * See if the instruction can be emulated.
 * Returns true if instruction was emulated, false otherwise.
 *
 * For now we always emulate so this function just returns 0.
 */
bool arch_uprobe_skip_sstep(struct arch_uprobe *auprobe, struct pt_regs *regs)
{
	return 0;
}

/*
 * struct xol_area and get_trampoline_vaddr() are copied from
 * kernel/events/uprobes.c to avoid modifying arch-independent
 * code.
 */
struct xol_area {
	wait_queue_head_t		wq;
	atomic_t			slot_count;
	unsigned long			*bitmap;
	struct vm_special_mapping	xol_mapping;
	struct page			*pages[2];
	unsigned long			vaddr;
};

static unsigned long get_trampoline_vaddr(void)
{
	struct xol_area *area;
	unsigned long trampoline_vaddr = -1;

	area = READ_ONCE(current->mm->uprobes_state.xol_area);
	if (area)
		trampoline_vaddr = area->vaddr;

	return trampoline_vaddr;
}

void sw64_fix_uretprobe(struct pt_regs *regs, unsigned long exc_pc)
{
	/*
	 * regs->pc has been changed to orig_ret_vaddr in handle_trampoline().
	 */
	if (exc_pc == get_trampoline_vaddr())
		regs->regs[26] = regs->pc;
}

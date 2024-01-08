// SPDX-License-Identifier: GPL-2.0
/*
 *  Kernel Probes (KProbes)
 *  arch/sw_64/kernel/kprobes.c
 */

#include <linux/kprobes.h>
#include <linux/kdebug.h>
#include <linux/slab.h>

#include "common.h"

static u32 breakpoint_insn = BREAK_KPROBE;
static u32 breakpoint2_insn = BREAK_KPROBE_SS;

int post_kprobe_handler(struct pt_regs *regs);

DEFINE_PER_CPU(struct kprobe *, current_kprobe);
DEFINE_PER_CPU(struct kprobe_ctlblk, kprobe_ctlblk);

int __kprobes arch_prepare_kprobe(struct kprobe *p)
{
	int ret = 0;
	extern char __start_rodata[];
	extern char __end_rodata[];
	unsigned long probe_addr = (unsigned long)p->addr;

	if (probe_addr & 0x3)
		return -EINVAL;

	if (!sw64_insn_can_kprobe(p->addr))
		return -EINVAL;
	/* copy instruction */
	p->opcode = le32_to_cpu(*p->addr);


	if (probe_addr >= (unsigned long) __start_rodata &&
			probe_addr <= (unsigned long) __end_rodata)
		return -EINVAL;


	/* insn: must be on special executable page on mips. */
	p->ainsn.insn = get_insn_slot();
	if (!p->ainsn.insn) {
		ret = -ENOMEM;
		goto out;
	}
	/*
	 * In the kprobe->ainsn.insn[] array we store the original
	 * instruction at index zero and a break trap instruction at
	 * index one.
	 */
	p->ainsn.insn[0] = p->opcode;
	p->ainsn.insn[1] = breakpoint2_insn;
out:
	return ret;
}

void __kprobes arch_arm_kprobe(struct kprobe *p)
{
	sw64_insn_write(p->addr, breakpoint_insn);
	flush_insn_slot(p);
}

void __kprobes arch_disarm_kprobe(struct kprobe *p)
{
	sw64_insn_write(p->addr, p->opcode);
	flush_insn_slot(p);
}

void __kprobes arch_remove_kprobe(struct kprobe *p)
{
	if (p->ainsn.insn) {
		free_insn_slot(p->ainsn.insn, 0);
		p->ainsn.insn = NULL;
	}
}

static void save_previous_kprobe(struct kprobe_ctlblk *kcb)
{
	kcb->prev_kprobe.kp = kprobe_running();
	kcb->prev_kprobe.status = kcb->kprobe_status;
}

static void restore_previous_kprobe(struct kprobe_ctlblk *kcb)
{
	__this_cpu_write(current_kprobe, kcb->prev_kprobe.kp);
	kcb->kprobe_status = kcb->prev_kprobe.status;
}

static void __kprobes set_current_kprobe(struct kprobe *p)
{
	__this_cpu_write(current_kprobe, p);
}


static void __kprobes setup_singlestep(struct kprobe *p, struct pt_regs *regs,
		struct kprobe_ctlblk *kcb, int reenter)
{
	if (reenter) {
		save_previous_kprobe(kcb);
		set_current_kprobe(p);
		kcb->kprobe_status = KPROBE_REENTER;
	} else {
		kcb->kprobe_status = KPROBE_HIT_SS;
	}

	/* insn simulation */
	kcb->target_pc = regs->pc;
	regs->pc = (unsigned long)&p->ainsn.insn[0];
}

static int __kprobes reenter_kprobe(struct kprobe *p,
		struct pt_regs *regs,
		struct kprobe_ctlblk *kcb)
{
	switch (kcb->kprobe_status) {
	case KPROBE_HIT_SSDONE:
	case KPROBE_HIT_ACTIVE:
		kprobes_inc_nmissed_count(p);
		setup_singlestep(p, regs, kcb, 1);
		break;
	case KPROBE_HIT_SS:
	case KPROBE_REENTER:
		pr_warn("Unrecoverable kprobe detected.\n");
		dump_kprobe(p);
		BUG();
		break;
	default:
		WARN_ON(1);
		return 0;
	}
	return 1;
}

int __kprobes kprobe_handler(struct pt_regs *regs)
{
	struct kprobe *p;
	struct kprobe_ctlblk *kcb;
	unsigned long addr = instruction_pointer(regs);

	if (user_mode(regs))
		return 0;
	/*
	 * We don't want to be preempted for the entire
	 * duration of kprobe processing
	 */
	preempt_disable();
	kcb = get_kprobe_ctlblk();
	p = get_kprobe((kprobe_opcode_t *)(addr - 4));

	if (p) {
		if (kprobe_running()) {
			if (reenter_kprobe(p, regs, kcb))
				return 1;
		} else {
			set_current_kprobe(p);
			kcb->kprobe_status = KPROBE_HIT_ACTIVE;

			/*
			 * If we have no pre-handler or it returned 0, we
			 * continue with normal processing.  If we have a
			 * pre-handler and it returned non-zero, that means
			 * user handler setup registers to exit to another
			 * instruction, we must skip the single stepping.
			 */
			if (!p->pre_handler || !p->pre_handler(p, regs))
				setup_singlestep(p, regs, kcb, 0);
			else
				reset_current_kprobe();
			return 1;
		}
	}
	return 0;

}
int __kprobes post_kprobe_handler(struct pt_regs *regs)
{
	struct kprobe *cur = kprobe_running();
	struct kprobe_ctlblk *kcb = get_kprobe_ctlblk();

	if (!cur)
		return 0;

	if ((kcb->kprobe_status != KPROBE_REENTER) && cur->post_handler) {
		kcb->kprobe_status = KPROBE_HIT_SSDONE;
		cur->post_handler(cur, regs, 0);
	}

	//	resume_execution(cur, regs, kcb);
	regs->pc = kcb->target_pc;


	/* Restore back the original saved kprobes variables and continue. */
	if (kcb->kprobe_status == KPROBE_REENTER) {
		restore_previous_kprobe(kcb);
		goto out;
	}
	reset_current_kprobe();
out:
	preempt_enable_no_resched();

	return 1;
}

int __kprobes kprobe_fault_handler(struct pt_regs *regs, unsigned long mmcsr)
{
	struct kprobe_ctlblk *kcb = get_kprobe_ctlblk();

	if (kcb->kprobe_status & KPROBE_HIT_SS) {
		regs->pc = kcb->target_pc;

		reset_current_kprobe();
		preempt_enable_no_resched();
	}
	return 0;
}

/*
 * Wrapper routine for handling exceptions.
 */
int __kprobes kprobe_exceptions_notify(struct notifier_block *self,
		unsigned long val, void *data)
{

	struct die_args *args = (struct die_args *)data;
	int ret = NOTIFY_DONE;

	switch (val) {
	case DIE_BREAK:
		if (kprobe_handler(args->regs))
			ret = NOTIFY_STOP;
		break;
	case DIE_SSTEPBP:
		if (post_kprobe_handler(args->regs))
			ret = NOTIFY_STOP;
		break;
	default:
		break;
	}
	return ret;
}
/*
 * Function return probe trampoline:
 *	- init_kprobes() establishes a probepoint here
 *	- When the probed function returns, this probe causes the
 *	  handlers to fire
 */
static void __used kretprobe_trampoline_holder(void)
{
	asm volatile(
			/* Keep the assembler from reordering and placing JR here. */
			".set noreorder\n\t"
			"nop\n\t"
			".global __kretprobe_trampoline\n"
			"__kretprobe_trampoline:\n\t"
			"nop\n\t"
			: : : "memory");
}

void __kretprobe_trampoline(void);

void __kprobes arch_prepare_kretprobe(struct kretprobe_instance *ri,
		struct pt_regs *regs)
{
	ri->ret_addr = (kprobe_opcode_t *) regs->regs[26];
	ri->fp = NULL;

	/* Replace the return addr with trampoline addr */
	regs->regs[26] = (unsigned long)__kretprobe_trampoline;
}

/*
 * Called when the probe at kretprobe trampoline is hit
 */
static int __kprobes trampoline_probe_handler(struct kprobe *p,
		struct pt_regs *regs)
{
	unsigned long orig_ret_address;

	orig_ret_address = __kretprobe_trampoline_handler(regs, NULL);
	instruction_pointer(regs) = orig_ret_address;
	regs->regs[26] = orig_ret_address;

	/*
	 * By returning a non-zero value, we are telling
	 * kprobe_handler() that we don't want the post_handler
	 * to run (and have re-enabled preemption)
	 */
	return 1;
}

int __kprobes arch_trampoline_kprobe(struct kprobe *p)
{
	if (p->addr == (kprobe_opcode_t *)__kretprobe_trampoline)
		return 1;

	return 0;
}

static struct kprobe trampoline_p = {
	.addr = (kprobe_opcode_t *)__kretprobe_trampoline,
	.pre_handler = trampoline_probe_handler
};

int __init arch_init_kprobes(void)
{
	return register_kprobe(&trampoline_p);
}

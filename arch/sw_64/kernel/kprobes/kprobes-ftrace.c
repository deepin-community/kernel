// SPDX-License-Identifier: GPL-2.0
/*
 * Dynamic Ftrace based Kprobes Optimization
 */

#include <linux/kprobes.h>
#include <linux/ptrace.h>
#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/ftrace.h>

/* Ftrace callback handler for kprobes */
void kprobe_ftrace_handler(unsigned long ip, unsigned long parent_ip,
			   struct ftrace_ops *ops, struct ftrace_regs *fregs)
{
	struct pt_regs *regs = ftrace_get_regs(fregs);
	struct kprobe *p;
	struct kprobe_ctlblk *kcb;

	p = get_kprobe((kprobe_opcode_t *)ip);
	if (unlikely(!p) || kprobe_disabled(p))
		return;

	kcb = get_kprobe_ctlblk();
	if (kprobe_running()) {
		kprobes_inc_nmissed_count(p);
	} else {
		regs->regs[28] -= MCOUNT_INSN_SIZE;

		__this_cpu_write(current_kprobe, p);
		kcb->kprobe_status = KPROBE_HIT_ACTIVE;
		if (!p->pre_handler || !p->pre_handler(p, regs)) {
			regs->regs[28] += MCOUNT_INSN_SIZE;
			if (unlikely(p->post_handler)) {
				kcb->kprobe_status = KPROBE_HIT_SSDONE;
				p->post_handler(p, regs, 0);
			}
		}
		__this_cpu_write(current_kprobe, NULL);
	}
}
NOKPROBE_SYMBOL(kprobe_ftrace_handler);

int arch_prepare_kprobe_ftrace(struct kprobe *p)
{
	p->ainsn.insn = NULL;
	p->ainsn.boostable = -1;
	return 0;
}

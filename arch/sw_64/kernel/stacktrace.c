// SPDX-License-Identifier: GPL-2.0
/*
 * Stack trace management functions
 *
 *  Copyright (C) 2018 snyh <xiabin@deepin.com>
 */
#include <linux/sched.h>
#include <linux/stacktrace.h>
#include <linux/sched/task_stack.h>
#include <linux/sched/debug.h>
#include <linux/ftrace.h>
#include <linux/perf_event.h>
#include <linux/kallsyms.h>

#include <asm/stacktrace.h>

/*
 * sw_64 PCS assigns the frame pointer to r15.
 *
 * A simple function prologue looks like this:
 *	ldi     sp,-xx(sp)
 *	stl     ra,0(sp)
 *	stl     fp,8(sp)
 *	mov     sp,fp
 *
 * A simple function epilogue looks like this:
 *	mov     fp,sp
 *	ldl     ra,0(sp)
 *	ldl     fp,8(sp)
 *	ldi     sp,+xx(sp)
 */

#ifdef CONFIG_FRAME_POINTER

int unwind_frame(struct task_struct *tsk, struct stackframe *frame)
{
	unsigned long fp = frame->fp;

	if (fp & 0x7)
		return -EINVAL;

	if (!tsk)
		tsk = current;

	if (!on_accessible_stack(tsk, fp, NULL))
		return -EINVAL;

	frame->pc = READ_ONCE_NOCHECK(*(unsigned long *)(fp));
	frame->fp = READ_ONCE_NOCHECK(*(unsigned long *)(fp + 8));

	/*
	 * Frames created upon entry from user have NULL FP and PC values, so
	 * don't bother reporting these. Frames created by __noreturn functions
	 * might have a valid FP even if PC is bogus, so only terminate where
	 * both are NULL.
	 */
	if (!frame->fp && !frame->pc)
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL_GPL(unwind_frame);

void walk_stackframe(struct task_struct *tsk, struct pt_regs *regs,
		     int (*fn)(unsigned long, void *), void *data)
{
	unsigned long pc, fp;

	struct stackframe frame;

	if (regs) {
		unsigned long offset;

		pc = regs->pc;
		fp = regs->regs[15];
		if (kallsyms_lookup_size_offset(pc, NULL, &offset)
				&& offset < 16) {
			/* call stack has not been setup
			 * store pc first then loop from ra
			 */
			if (fn(pc, data))
				return;
			pc = regs->regs[26];
		}
	} else if (tsk == current || tsk == NULL) {
		fp = (unsigned long)__builtin_frame_address(0);
		pc = (unsigned long)walk_stackframe;
	} else {
		fp = tsk->thread.s[6];
		pc = tsk->thread.ra;
	}

	if (!__kernel_text_address(pc) || fn(pc, data))
		return;

	frame.pc = pc;
	frame.fp = fp;
	while (1) {
		int ret;

		ret = unwind_frame(tsk, &frame);
		if (ret < 0)
			break;

		if (fn(frame.pc, data))
			break;
	}
}
EXPORT_SYMBOL_GPL(walk_stackframe);

#else /* !CONFIG_FRAME_POINTER */
void walk_stackframe(struct task_struct *tsk, struct pt_regs *regs,
		     int (*fn)(unsigned long, void *), void *data)
{
	unsigned long *ksp;
	unsigned long sp, pc;

	if (regs) {
		sp = (unsigned long)(regs+1);
		pc = regs->pc;
	} else if (tsk == current || tsk == NULL) {
		register unsigned long current_sp __asm__ ("$30");
		sp = current_sp;
		pc = (unsigned long)walk_stackframe;
	} else {
		sp = tsk->thread.sp;
		pc = tsk->thread.ra;
	}

	ksp = (unsigned long *)sp;

	while (!kstack_end(ksp)) {
		if (__kernel_text_address(pc) && fn(pc, data))
			break;
		pc = *ksp++;
	}
}
EXPORT_SYMBOL_GPL(walk_stackframe);

#endif/* CONFIG_FRAME_POINTER */

static int print_address_trace(unsigned long pc, void *data)
{
	print_ip_sym((const char *)data, pc);
	return 0;
}

void show_stack(struct task_struct *task, unsigned long *sp, const char *loglvl)
{
	pr_info("Trace:\n");
	walk_stackframe(task, NULL, print_address_trace, (void *)loglvl);
}

#ifdef CONFIG_STACKTRACE
/*
 * Save stack-backtrace addresses into a stack_trace buffer.
 */
struct stack_trace_data {
	struct stack_trace *trace;
	unsigned int nosched;
};

int save_trace(unsigned long pc, void *d)
{
	struct stack_trace_data *data = d;
	struct stack_trace *trace = data->trace;

	if (data->nosched && in_sched_functions(pc))
		return 0;
	if (trace->skip > 0) {
		trace->skip--;
		return 0;
	}

	trace->entries[trace->nr_entries++] = pc;
	return (trace->nr_entries >= trace->max_entries);
}

void save_stack_trace_regs(struct pt_regs *regs, struct stack_trace *trace)
{
	struct stack_trace_data data;

	data.trace = trace;
	data.nosched = 0;

	walk_stackframe(current, regs, save_trace, &data);

	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = ULONG_MAX;
}

static void __save_stack_trace(struct task_struct *tsk,
		struct stack_trace *trace, unsigned int nosched)
{
	struct stack_trace_data data;

	data.trace = trace;
	data.nosched = nosched;

	walk_stackframe(tsk, NULL, save_trace, &data);

	if (trace->nr_entries < trace->max_entries)
		trace->entries[trace->nr_entries++] = ULONG_MAX;
}

void save_stack_trace_tsk(struct task_struct *tsk, struct stack_trace *trace)
{
	__save_stack_trace(tsk, trace, 1);
}
EXPORT_SYMBOL_GPL(save_stack_trace_tsk);

void save_stack_trace(struct stack_trace *trace)
{
	__save_stack_trace(current, trace, 0);
}
EXPORT_SYMBOL_GPL(save_stack_trace);
#endif

static int save_pc(unsigned long pc, void *data)
{
	unsigned long *p = data;
	*p = 0;

	if (!in_sched_functions(pc))
		*p = pc;

	return *p;
}

unsigned long __get_wchan(struct task_struct *tsk)
{
	unsigned long pc;

	if (!tsk || tsk == current || task_is_running(tsk))
		return 0;
	walk_stackframe(tsk, NULL, save_pc, &pc);

	return pc;
}

#ifdef CONFIG_HAVE_RELIABLE_STACKTRACE
int save_stack_trace_tsk_reliable(struct task_struct *tsk,
				  struct stack_trace *trace)
{
	return 0;
}
#endif

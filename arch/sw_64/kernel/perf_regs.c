// SPDX-License-Identifier: GPL-2.0

#include <linux/errno.h>
#include <linux/perf_event.h>

u64 perf_reg_value(struct pt_regs *regs, int idx)
{
	if (WARN_ON_ONCE((u32)idx >= PERF_REG_SW64_MAX))
		return 0;

	return ((unsigned long *)regs)[idx];
}

#define REG_RESERVED (~((1ULL << PERF_REG_SW64_MAX) - 1))

int perf_reg_validate(u64 mask)
{
	if (!mask || mask & REG_RESERVED)
		return -EINVAL;
	return 0;
}

u64 perf_reg_abi(struct task_struct *task)
{
	return PERF_SAMPLE_REGS_ABI_64;
}

void perf_get_regs_user(struct perf_regs *regs_user,
			struct pt_regs *regs)
{
	regs_user->regs = task_pt_regs(current);
	regs_user->abi = perf_reg_abi(current);
}

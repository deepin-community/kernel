// SPDX-License-Identifier: GPL-2.0

#include <linux/extable.h>
#include <linux/uaccess.h>

int fixup_exception(struct pt_regs *regs, unsigned long pc)
{
	const struct exception_table_entry *fixup;

	fixup = search_exception_tables(pc);
	if (fixup) {
		unsigned int valreg = fixup->fixup.bits.valreg;
		unsigned int errreg = fixup->fixup.bits.errreg;

		if (valreg != 31)
			regs->regs[valreg] = 0;
		if (errreg != 31)
			regs->regs[errreg] = -EFAULT;
		pc += fixup->fixup.bits.nextinsn;
		regs->pc = pc;

		return 1;
	}
	return 0;
}

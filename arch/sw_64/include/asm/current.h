/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_CURRENT_H
#define _ASM_SW64_CURRENT_H

#ifndef __ASSEMBLY__

struct task_struct;
static __always_inline struct task_struct *get_current(void)
{
	register struct task_struct *tp __asm__("$8");

	return tp;
}

#define current get_current()

#endif /* __ASSEMBLY__ */

#endif /* _ASM_SW64_CURRENT_H */

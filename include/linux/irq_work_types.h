/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_IRQ_WORK_TYPES_H
#define _LINUX_IRQ_WORK_TYPES_H

#include <linux/smp_types.h>
#include <linux/types.h>
#include <linux/deepin_kabi.h>

struct irq_work {
	struct __call_single_node	node;
	void				(*func)(struct irq_work *);
	struct rcuwait			irqwait;
	DEEPIN_KABI_RESERVE(1)
	DEEPIN_KABI_RESERVE(2)
	DEEPIN_KABI_RESERVE(3)
	DEEPIN_KABI_RESERVE(4)
};

#endif

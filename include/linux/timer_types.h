/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_TIMER_TYPES_H
#define _LINUX_TIMER_TYPES_H

#include <linux/lockdep_types.h>
#include <linux/types.h>
#include <linux/deepin_kabi.h>

struct timer_list {
	/*
	 * All fields that change during normal runtime grouped to the
	 * same cacheline
	 */
	struct hlist_node	entry;
	unsigned long		expires;
	void			(*function)(struct timer_list *);
	u32			flags;

#ifdef CONFIG_LOCKDEP
	struct lockdep_map	lockdep_map;
#endif

	DEEPIN_KABI_RESERVE(1)
	DEEPIN_KABI_RESERVE(2)
};

#endif /* _LINUX_TIMER_TYPES_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_KDEBUG_H
#define _ASM_SW64_KDEBUG_H

#include <linux/notifier.h>

enum die_val {
	DIE_OOPS = 1,
	DIE_BREAK,
	DIE_SSTEPBP,
	DIE_UPROBE,
	DIE_UPROBE_XOL,
};

#endif /* _ASM_SW64_KDEBUG_H */

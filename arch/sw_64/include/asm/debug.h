/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 Mao Minkai
 * Author: Mao Minkai
 *
 * This code is taken from arch/mips/include/asm/debug.h
 *	Copyright (C) 2015 Imagination Technologies
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef _ASM_SW64_DEBUG_H
#define _ASM_SW64_DEBUG_H

#include <linux/debugfs.h>

/*
 * sw64_debugfs_dir corresponds to the "sw_64" directory at the top level
 * of the DebugFS hierarchy. SW64-specific DebugFS entries should be
 * placed beneath this directory.
 */
extern struct dentry *sw64_debugfs_dir;

#define UNA_MAX_ENTRIES	64

struct unaligned_stat {
	unsigned long pc;
	unsigned long va;
};

extern char unaligned_task[];
extern unsigned long unaligned_count;
extern struct unaligned_stat unaligned[];

#endif /* _ASM_SW64_DEBUG_H */

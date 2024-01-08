// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Zhi Tongze
 * Author: Zhi Tongze
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 */

#include <linux/kernel.h>

#include <asm/debug.h>

extern bool segv_debug_enabled;

static int __init segv_debug_init(void)
{
	if (!sw64_debugfs_dir)
		return -ENODEV;

	debugfs_create_bool("segv_debug", 0644,
			sw64_debugfs_dir, &segv_debug_enabled);
	return 0;
}
late_initcall(segv_debug_init);

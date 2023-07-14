// SPDX-License-Identifier: GPL-2.0-only
/*
 * HYGON Memory Encryption Support
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DISABLE_BRANCH_PROFILING

#include <linux/cc_platform.h>
#include <linux/mem_encrypt.h>
#include <linux/printk.h>

void print_hygon_cc_feature_info(void)
{
	/* Secure Memory Encryption */
	if (cc_platform_has(CC_ATTR_HOST_MEM_ENCRYPT)) {
		/*
		 * HYGON SME is mutually exclusive with any of the
		 * HYGON CSV features below.
		 */
		pr_info(" HYGON SME");
		return;
	}

	/* Secure Encrypted Virtualization */
	if (cc_platform_has(CC_ATTR_GUEST_MEM_ENCRYPT))
		pr_info(" HYGON CSV");

	/* Encrypted Register State */
	if (cc_platform_has(CC_ATTR_GUEST_STATE_ENCRYPT))
		pr_info(" HYGON CSV2");
}

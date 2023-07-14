/* SPDX-License-Identifier: GPL-2.0 */
/*
 * The helpers to support Hygon CPU specific code path.
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 */

#ifndef _ASM_X86_PROCESSOR_HYGON_H
#define _ASM_X86_PROCESSOR_HYGON_H

#include <asm/processor.h>

/*
 * helper to determine HYGON CPU
 */
static inline bool is_x86_vendor_hygon(void)
{
	return boot_cpu_data.x86_vendor == X86_VENDOR_HYGON;
}

#endif	/* _ASM_X86_PROCESSOR_HYGON_H */

/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _LIB_CPU_TYPE_H
#define _LIB_CPU_TYPE_H

#include <linux/types.h>

#define CPU_VENDOR_UNKNOWN   0
#define CPU_VENDOR_PHYTIUM   1

/* Phytium CPU models (one bit per model) */
#define PHYTIUM_MODEL_FT2004    (1 << 0)
#define PHYTIUM_MODEL_D2000     (1 << 1)
#define PHYTIUM_MODEL_D3000     (1 << 2)
#define PHYTIUM_MODEL_D3000M    (1 << 3)

struct cpu_identifier {
	u32 vendor;
	u32 model_bits;
};

#ifdef CONFIG_ARM64
#include <asm/cputype.h>
#define MACHINE_TYPE_FUN_DEFS(cpu)  \
	bool cpu_is_##cpu(void)
#else
#define MACHINE_TYPE_FUN_DEFS(cpu)  \
	static bool __maybe_unused      \
	cpu_is_##cpu(void) { return false; }
#endif

MACHINE_TYPE_FUN_DEFS(ft_d2000);
MACHINE_TYPE_FUN_DEFS(ft2004);
MACHINE_TYPE_FUN_DEFS(ft_d3000);
MACHINE_TYPE_FUN_DEFS(ft_d3000m);

#ifdef CONFIG_ARM64
bool cpu_match(u32 vendor, u32 model_mask);
#else
static bool __maybe_unused
cpu_match(u32 vendor, u32 model_mask)
{
	return false;
}
#endif

#endif

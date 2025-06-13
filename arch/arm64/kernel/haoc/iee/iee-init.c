// SPDX-License-Identifier: GPL-2.0
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#include <asm/haoc/iee.h>
#include <linux/memblock.h>
#include <asm/cpufeature.h>
#include <asm/haoc/iee-mmu.h>
#include <asm/haoc/iee-init.h>
#ifdef CONFIG_IEE_PTRP
#include <asm/haoc/iee-token.h>
#endif
#include <asm/haoc/iee-asm.h>

__aligned(PAGE_SIZE) DEFINE_PER_CPU(u64*[(PAGE_SIZE/8)],
				iee_cpu_stack_ptr);

bool __ro_after_init __aligned(8) iee_init_done;
bool __ro_after_init haoc_enabled;
EXPORT_SYMBOL(haoc_enabled);

/* Allocate pages from IEE data pool to use as per-cpu IEE stack. */
static void __init iee_stack_alloc(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		u64 *cpu_stack_ptr = (u64 *)(SHIFT_PERCPU_PTR(iee_cpu_stack_ptr,
				__per_cpu_offset[cpu]));
		u64 *new_pages  = __va(early_iee_stack_alloc(IEE_STACK_ORDER));

		*cpu_stack_ptr = __virt_to_iee((u64)new_pages + IEE_STACK_SIZE);
	}

	flush_tlb_all();
}

void iee_setup_asid(void)
{
	unsigned long ttbr0 = read_sysreg(ttbr0_el1) | FIELD_PREP(TTBR_ASID_MASK, IEE_ASID);
	/* Load IEE ASID into ttbr0 to use it in IEE. */
	write_sysreg(ttbr0, ttbr0_el1);
	isb();
}

void __init iee_init_post(void)
{
	if (!haoc_enabled)
		return;

	iee_setup_asid();
	/* Flush tlb to enable IEE. */
	flush_tlb_all();

#ifdef CONFIG_IEE_PTRP
	iee_prepare_init_task_token();
#endif
	iee_init_done = true;

#ifdef CONFIG_IEE_SIP
	extern void iee_si_init(void);
	iee_si_init();
#endif
}

void __init iee_stack_init(void)
{
	if (!haoc_enabled)
		return;

	iee_stack_alloc();
}

static int __init parse_haoc_enabled(char *str)
{
	return kstrtobool(str, &haoc_enabled);
}
early_param("haoc", parse_haoc_enabled);

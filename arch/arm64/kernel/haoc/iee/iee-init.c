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

__aligned(PAGE_SIZE) DEFINE_PER_CPU(u64*[(PAGE_SIZE/8)],
				iee_cpu_stack_ptr);

bool __initdata iee_init_done;

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

void __init iee_init_post(void)
{
	/* Flush tlb to enable IEE. */
	flush_tlb_all();

	iee_init_done = true;
}

void __init iee_stack_init(void)
{
	iee_stack_alloc();
}

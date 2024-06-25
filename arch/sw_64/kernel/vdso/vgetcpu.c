// SPDX-License-Identifier: GPL-2.0
/*
 *
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation, version 2.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, GOOD TITLE or
 *   NON INFRINGEMENT.  See the GNU General Public License for
 *   more details.
 */

#include <asm/unistd.h>
#include <asm/vdso.h>
#include <asm/csr.h>
#include <asm/hmcall.h>
#include <linux/getcpu.h>

static void __getcpu(unsigned int *cpu, unsigned int *node,
		const struct vdso_data *data)
{
	unsigned int cpuid;
#ifdef CONFIG_SUBARCH_C3B
	asm volatile ("sys_call	%1\n"
		      "mov $0, %0\n"
		      : "=&r"(cpuid)
		      : "i"(HMC_uwhami));
	*cpu =	data->vdso_whami_to_cpu[cpuid];
	*node = data->vdso_whami_to_node[cpuid];
#else
	asm volatile ("csrr %0, %1" : "=&r"(cpuid) : "i"(CSR_SOFTCID));
	*cpu = cpuid;
	*node = data->vdso_cpu_to_node[*cpu];
#endif
}


long __vdso_getcpu(unsigned int *cpu, unsigned int *node,
		   struct getcpu_cache *unused)
{
	const struct vdso_data *data = get_vdso_data();

	__getcpu(cpu, node, data);
	return 0;
}


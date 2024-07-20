/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_TOPOLOGY_H
#define _ASM_SW64_TOPOLOGY_H

#include <linux/smp.h>
#include <linux/threads.h>
#include <linux/cpumask.h>
#include <linux/arch_topology.h>
#include <asm/core.h>
#include <asm/smp.h>

static inline int rcid_to_thread_id(int rcid)
{
	return (rcid & THREAD_ID_MASK) >> THREAD_ID_SHIFT;
}

static inline int rcid_to_core_id(int rcid)
{
	return (rcid & CORE_ID_MASK) >> CORE_ID_SHIFT;
}

static inline int rcid_to_domain_id(int rcid)
{
	return (rcid & DOMAIN_ID_MASK) >> DOMAIN_ID_SHIFT;
}

#ifdef CONFIG_NUMA

#ifndef CONFIG_DEBUG_PER_CPU_MAPS
extern cpumask_var_t node_to_cpumask_map[MAX_NUMNODES];
/* Returns a pointer to the cpumask of CPUs on Node 'node'. */
#define cpumask_of_node(node)	((node) == NUMA_NO_NODE ?	\
				cpu_all_mask :			\
				node_to_cpumask_map[node])
#else
extern const struct cpumask *cpumask_of_node(int node);
#endif /* CONFIG_DEBUG_PER_CPU_MAPS */

extern void numa_add_cpu(unsigned int cpu);
extern void numa_remove_cpu(unsigned int cpu);
extern void numa_store_cpu_info(unsigned int cpu);
extern int __node_distance(int from, int to);
#define node_distance(a, b) __node_distance(a, b)
#define parent_node(node) (node)
#define cpumask_of_pcibus(bus)	(cpu_online_mask)
#else /* !CONFIG_NUMA */
static inline void numa_add_cpu(unsigned int cpu) { }
static inline void numa_remove_cpu(unsigned int cpu) { }
static inline void numa_store_cpu_info(unsigned int cpu) { }
#endif /* CONFIG_NUMA */

#include <asm-generic/topology.h>

static inline void arch_fix_phys_package_id(int num, u32 slot) { }

#endif /* _ASM_SW64_TOPOLOGY_H */

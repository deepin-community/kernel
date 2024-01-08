/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _ASM_SW64_NUMA_H
#define _ASM_SW64_NUMA_H

#include <linux/nodemask.h>
#include <asm/memory.h>

#ifdef CONFIG_NUMA
extern nodemask_t numa_nodes_parsed __initdata;
extern int numa_off;

struct numa_memblk {
	u64			start;
	u64			end;
	int			nid;
};

#define NR_NODE_MEMBLKS		(MAX_NUMNODES*2)
struct numa_meminfo {
	int			nr_blks;
	struct numa_memblk	blk[NR_NODE_MEMBLKS];
};
extern int __init numa_add_memblk(int nodeid, u64 start, u64 end);
extern void numa_clear_node(unsigned int cpu);
extern void __init numa_set_distance(int from, int to, int distance);
extern void __init early_map_cpu_to_node(unsigned int cpu, int nid);

#else  /* CONFIG_NUMA */

static inline void numa_clear_node(unsigned int cpu) { }
static inline void early_map_cpu_to_node(unsigned int cpu, int nid) { }

#endif /* CONFIG_NUMA */

#endif /* _ASM_SW64_NUMA_H */

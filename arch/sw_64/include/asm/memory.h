/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_MEMORY_H
#define _ASM_SW64_MEMORY_H

#ifdef CONFIG_NUMA
#include <linux/numa.h>
#endif

#define MIN_MEMORY_BLOCK_SIZE_VM_MEMHP    (1UL << 30)
#define NODE0_START	(_TEXT_START - __START_KERNEL_map)

#define MAX_PHYSMEM_BITS	48

struct mem_desc_t {
	unsigned long phys_base;	/* start address of physical memory */
	unsigned long phys_size;	/* size of physical memory */
	phys_addr_t base;		/* start address of memory managed by kernel */
	phys_addr_t size;		/* size of memory managed by kernel */
};
extern struct mem_desc_t mem_desc;

struct numa_node_desc_t {
	phys_addr_t base;
	phys_addr_t size;
};
extern struct numa_node_desc_t numa_nodes_desc[];

void __init callback_init(void);
void __init mem_detect(void);
void __init sw64_memblock_init(void);
void __init zone_sizes_init(void);
void __init sw64_numa_init(void);
void __init sw64_memory_present(void);

#endif /* _ASM_SW64_MEMORY_H */

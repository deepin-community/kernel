// SPDX-License-Identifier: GPL-2.0
/*
 *  DISCONTIGMEM NUMA sw64 support.
 */

#include <linux/memblock.h>
#include <linux/cpuset.h>
#include <linux/acpi.h>
#include <linux/of.h>

#include <asm/core.h>
#include <asm/platform.h>

int cpu_to_node_map[NR_CPUS];
cpumask_var_t node_to_cpumask_map[MAX_NUMNODES];
EXPORT_SYMBOL(node_to_cpumask_map);

struct numa_node_desc_t numa_nodes_desc[MAX_NUMNODES];
nodemask_t numa_nodes_parsed __initdata;

static int numa_distance_cnt;
static u8 *numa_distance;
int numa_off;

static __init int numa_setup(char *opt)
{
	if (!opt)
		return -EINVAL;
	if (!strncmp(opt, "off", 3))
		numa_off = 1;
	return 0;
}
early_param("numa", numa_setup);

/*
 * Allocate node_to_cpumask_map based on number of available nodes
 * Requires node_possible_map to be valid.
 *
 * Note: cpumask_of_node() is not valid until after this is done.
 * (Use CONFIG_DEBUG_PER_CPU_MAPS to check this.)
 */
static void __init setup_node_to_cpumask_map(void)
{
	int node;

	/* setup nr_node_ids if not done yet */
	if (nr_node_ids == MAX_NUMNODES)
		setup_nr_node_ids();

	/* allocate and clear the mapping */
	for (node = 0; node < nr_node_ids; node++) {
		alloc_bootmem_cpumask_var(&node_to_cpumask_map[node]);
		cpumask_clear(node_to_cpumask_map[node]);
	}

	/* cpumask_of_node() will now work */
	pr_debug("Node to cpumask map for %d nodes\n", nr_node_ids);
}

/**
 * numa_add_memblk - Set node id to memblk
 * @nid: NUMA node ID of the new memblk
 * @start: Start address of the new memblk
 * @end:  End address of the new memblk
 *
 * RETURNS:
 * 0 on success, -errno on failure.
 */
int __init numa_add_memblk(int nid, u64 start, u64 end)
{
	int ret;

	ret = memblock_set_node(start, (end - start), &memblock.memory, nid);
	if (ret < 0) {
		pr_err("memblock [0x%llx - 0x%llx] failed to add on node %d\n",
			start, (end - 1), nid);
		return ret;
	}

	node_set(nid, numa_nodes_parsed);
	return ret;
}

/**
 * Initialize NODE_DATA for a node on the local memory
 */
static void __init setup_node_data(int nid, unsigned long start_pfn, unsigned long end_pfn)
{
	const size_t nd_size = roundup(sizeof(pg_data_t), SMP_CACHE_BYTES);
	u64 nd_pa;
	void *nd;
	int tnid;

	if (start_pfn >= end_pfn)
		pr_info("Initmem setup node %d [<memory-less node>]\n", nid);

	nd_pa = memblock_phys_alloc_try_nid(nd_size, SMP_CACHE_BYTES, nid);
	nd = __va(nd_pa);

	/* report and initialize */
	pr_info("NODE_DATA [mem %#018llx-%#018llx]\n",
		nd_pa, nd_pa + nd_size - 1);
	tnid = early_pfn_to_nid(nd_pa >> PAGE_SHIFT);
	if (tnid != nid)
		pr_info("NODE_DATA(%d) on node %d\n", nid, tnid);

	node_data[nid] = nd;
	memset(NODE_DATA(nid), 0, sizeof(pg_data_t));
	NODE_DATA(nid)->node_id = nid;
	NODE_DATA(nid)->node_start_pfn = start_pfn;
	NODE_DATA(nid)->node_spanned_pages = end_pfn - start_pfn;
}

/**
 * numa_free_distance
 *
 * Free current distance table.
 */
void __init numa_free_distance(void)
{
	size_t size;

	if (!numa_distance)
		return;

	size = numa_distance_cnt * numa_distance_cnt *
		sizeof(numa_distance[0]);

	memblock_free(numa_distance, size);
	numa_distance_cnt = 0;
	numa_distance = NULL;
}

/**
 *
 * Create a new NUMA distance table.
 *
 */
static int __init numa_alloc_distance(void)
{
	size_t size;
	phys_addr_t phys;
	int i, j;

	size = nr_node_ids * nr_node_ids * sizeof(numa_distance[0]);
	phys = memblock_phys_alloc(size, PAGE_SIZE);
	if (WARN_ON(!phys))
		return -ENOMEM;

	numa_distance = __va(phys);
	numa_distance_cnt = nr_node_ids;

	/* fill with the default distances */
	for (i = 0; i < numa_distance_cnt; i++)
		for (j = 0; j < numa_distance_cnt; j++) {
			numa_distance[i * numa_distance_cnt + j] = i == j ?
				LOCAL_DISTANCE : REMOTE_DISTANCE;
		}

	pr_info("Initialized distance table, cnt=%d\n", numa_distance_cnt);

	return 0;
}

/**
 * numa_set_distance - Set inter node NUMA distance from node to node.
 * @from: the 'from' node to set distance
 * @to: the 'to'  node to set distance
 * @distance: NUMA distance
 *
 * Set the distance from node @from to @to to @distance.
 * If distance table doesn't exist, a warning is printed.
 *
 * If @from or @to is higher than the highest known node or lower than zero
 * or @distance doesn't make sense, the call is ignored.
 *
 */
void __init numa_set_distance(int from, int to, int distance)
{
	if (!numa_distance) {
		pr_warn_once("Warning: distance table not allocated yet\n");
		return;
	}

	if (from >= numa_distance_cnt || to >= numa_distance_cnt ||
			from < 0 || to < 0) {
		pr_warn_once("Warning: node ids are out of bound, from=%d to=%d distance=%d\n",
			    from, to, distance);
		return;
	}

	if ((u8)distance != distance ||
	    (from == to && distance != LOCAL_DISTANCE)) {
		pr_warn_once("Warning: invalid distance parameter, from=%d to=%d distance=%d\n",
			     from, to, distance);
		return;
	}

	numa_distance[from * numa_distance_cnt + to] = distance;
}

/**
 * Return NUMA distance @from to @to
 */
int __node_distance(int from, int to)
{
	if (from >= numa_distance_cnt || to >= numa_distance_cnt)
		return from == to ? LOCAL_DISTANCE : REMOTE_DISTANCE;
	return numa_distance[from * numa_distance_cnt + to];
}
EXPORT_SYMBOL(__node_distance);

static int __init numa_register_nodes(void)
{
	int nid;
	struct memblock_region *mblk;

	/* Check that valid nid is set to memblks */
	for_each_mem_region(mblk) {
		pr_info("memblk node %d [mem %#018llx-%#018llx]\n",
				mblk->nid, mblk->base,
				mblk->base + mblk->size - 1);
		if (mblk->nid == NUMA_NO_NODE || mblk->nid >= MAX_NUMNODES) {
			pr_warn("Warning: invalid memblk node %d [mem %#018llx-%#018llx]\n",
				mblk->nid, mblk->base,
				mblk->base + mblk->size - 1);
			return -EINVAL;
		}
	}

	/* Finally register nodes */
	for_each_node_mask(nid, numa_nodes_parsed) {
		unsigned long start_pfn, end_pfn;

		get_pfn_range_for_nid(nid, &start_pfn, &end_pfn);
		setup_node_data(nid, start_pfn, end_pfn);
		node_set_online(nid);
	}

	/* Setup online nodes to actual nodes */
	node_possible_map = numa_nodes_parsed;

	return 0;
}

static int __init numa_init(int (*init_func)(void))
{
	int ret;

	nodes_clear(numa_nodes_parsed);
	nodes_clear(node_possible_map);
	nodes_clear(node_online_map);
	numa_free_distance();

	ret = numa_alloc_distance();
	if (ret < 0)
		return ret;

	ret = init_func();
	if (ret < 0)
		return ret;

	if (nodes_empty(numa_nodes_parsed)) {
		pr_info("No NUMA configuration found\n");
		return -EINVAL;
	}

	ret = numa_register_nodes();
	if (ret < 0)
		return ret;

	setup_node_to_cpumask_map();

	return 0;
}

static void __init get_numa_info_socket(void)
{
	int i;

	phys_addr_t base = 0;

	for (i = 0; i < MAX_NUMSOCKETS; i++) {
		if (socket_desc[i].is_online) {
			numa_nodes_desc[i].base = base;
			numa_nodes_desc[i].size = socket_desc[i].socket_mem;
			base += numa_nodes_desc[i].size;
		}
	}
}

static void cpu_set_node(void)
{
	int i;

	if (numa_off) {
		for (i = 0; i < nr_cpu_ids; i++)
			cpu_to_node_map[i] = 0;
	} else {
		for (i = 0; i < nr_cpu_ids; i++) {
			int nid = rcid_to_domain_id(cpu_to_rcid(i));

			cpu_to_node_map[i] = nid;
			node_set(nid, numa_nodes_parsed);
		}
	}
	/*
	 * Setup numa_node for cpu 0 before per_cpu area for booting.
	 * Actual setup of numa_node will be done in native_smp_prepare_cpus().
	 */
	set_cpu_numa_node(0, cpu_to_node_map[0]);
}

static int __init manual_numa_init(void)
{
	int ret, nid;
	struct memblock_region *mblk;
	phys_addr_t node_base, node_size, node_end;

	/**
	 * When boot magic is 0xDEED2024UL, legacy memory detection is
	 * completely bypassed, causing manual_numa_init failure. So we
	 * disable numa here.
	 */
	if (sunway_boot_magic == 0xDEED2024UL)
		numa_off = 1;

	if (numa_off) {
		pr_info("NUMA disabled\n"); /* Forced off on command line. */
		pr_info("Faking one node at [mem %#018llx-%#018llx]\n",
				memblock_start_of_DRAM(), memblock_end_of_DRAM() - 1);
		for_each_mem_region(mblk) {
			ret = numa_add_memblk(0, mblk->base, mblk->base + mblk->size);
			if (!ret)
				continue;

			pr_err("NUMA init failed\n");
			return ret;
		}
	} else {
		get_numa_info_socket();

		for (nid = 0; nid < MAX_NUMNODES; nid++) {
			node_base = numa_nodes_desc[nid].base;
			node_size = numa_nodes_desc[nid].size;
			node_end = node_base + node_size;
			ret = 0;

			if (!node_end)
				continue;

			for_each_mem_region(mblk) {
				if (mblk->base >= node_base && mblk->base < node_end) {
					if (mblk->base + mblk->size < node_end)
						ret = numa_add_memblk(nid, mblk->base, mblk->base + mblk->size);
					else
						ret = numa_add_memblk(nid, mblk->base, node_end);
				}
			}

			if (!node_size) {
				memblock_add_node(node_base, node_size, nid, MEMBLOCK_NONE);
				node_set(nid, numa_nodes_parsed);
				pr_info("Setup empty node %d from %#llx\n", nid, node_base);
			}

			if (!ret)
				continue;

			pr_err("NUMA init failed for node %d, [mem %#018llx-%#018llx]",
					nid, node_base, node_end - 1);
		}
	}

	cpu_set_node();

	return 0;
}

#ifdef CONFIG_ACPI_NUMA
static int __init sw64_acpi_numa_init(void)
{
	int ret;

	ret = acpi_numa_init();
	if (ret) {
		pr_info("Failed to initialise from firmware\n");
		return ret;
	}

	return srat_disabled() ? -EINVAL : 0;
}
#else
static int __init sw64_acpi_numa_init(void)
{
	return -EOPNOTSUPP;
}
#endif

void __init sw64_numa_init(void)
{
	if (!numa_off) {
		if (!acpi_disabled && !numa_init(sw64_acpi_numa_init))
			return;
		if (acpi_disabled && !numa_init(of_numa_init))
			return;
	}

	numa_init(manual_numa_init);
}

void numa_store_cpu_info(unsigned int cpu)
{
	set_cpu_numa_node(cpu, cpu_to_node_map[cpu]);
}

void __init early_map_cpu_to_node(unsigned int cpu, int nid)
{
	/* fallback to node 0 */
	if (nid < 0 || nid >= MAX_NUMNODES || numa_off)
		nid = 0;

	cpu_to_node_map[cpu] = nid;

	/*
	 * We should set the numa node of cpu0 as soon as possible, because it
	 * has already been set up online before. cpu_to_node(0) will soon be
	 * called.
	 */
	if (!cpu)
		set_cpu_numa_node(cpu, nid);
}

#ifdef CONFIG_DEBUG_PER_CPU_MAPS
/*
 * Returns a pointer to the bitmask of CPUs on Node 'node'.
 */
const struct cpumask *cpumask_of_node(int node)
{

	if (node == NUMA_NO_NODE) {
		pr_warn("%s: NUMA_NO_NODE\n", __func__);
		return cpu_all_mask;
	}

	if (WARN_ON(node < 0 || node >= nr_node_ids)) {
		pr_warn("%s: invalid node %d\n", __func__, node);
		return cpu_none_mask;
	}

	if (WARN_ON(node_to_cpumask_map[node] == NULL)) {
		pr_warn("%s: uninitialized node %d\n", __func__, node);
		return cpu_online_mask;
	}

	return node_to_cpumask_map[node];
}
EXPORT_SYMBOL(cpumask_of_node);
#endif

static void numa_update_cpu(unsigned int cpu, bool remove)
{
	int nid = cpu_to_node(cpu);

	if (nid == NUMA_NO_NODE)
		return;

	if (remove)
		cpumask_clear_cpu(cpu, node_to_cpumask_map[nid]);
	else
		cpumask_set_cpu(cpu, node_to_cpumask_map[nid]);
}

void numa_add_cpu(unsigned int cpu)
{
	numa_update_cpu(cpu, false);
}

void numa_remove_cpu(unsigned int cpu)
{
	numa_update_cpu(cpu, true);
}

void numa_clear_node(unsigned int cpu)
{
	numa_remove_cpu(cpu);
	set_cpu_numa_node(cpu, NUMA_NO_NODE);
}

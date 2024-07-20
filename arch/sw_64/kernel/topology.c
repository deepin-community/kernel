// SPDX-License-Identifier: GPL-2.0

#include <linux/acpi.h>
#include <linux/of.h>
#include <linux/printk.h>
#include <linux/arch_topology.h>
#include <linux/cacheinfo.h>

#include <asm/sw64io.h>
#include <asm/topology.h>

#define OFFSET_SMP_INFO	0x80UL

int topo_nr_threads, topo_nr_cores, topo_nr_maxcpus;

static int topo_nr_cpus;
static int topo_threads[NR_CPUS];
static int topo_cores[NR_CPUS];
static int topo_packages[NR_CPUS];

static void __init get_vt_smp_info(void)
{
	unsigned long smp_info;
	void __iomem *spbu_base = misc_platform_get_spbu_base(0);

	smp_info = readq(spbu_base + OFFSET_SMP_INFO);
	if (smp_info == -1UL)
		smp_info = 0;
	topo_nr_threads = (smp_info >> VT_THREADS_SHIFT) & VT_THREADS_MASK;
	topo_nr_cores = (smp_info >> VT_CORES_SHIFT) & VT_CORES_MASK;
	topo_nr_maxcpus = (smp_info >> VT_MAX_CPUS_SHIFT) & VT_MAX_CPUS_MASK;
}

static void __init init_topo_threads(void)
{
	int i, j;

	if (topo_nr_threads == 0)
		topo_nr_threads = 1;

	for (i = 0; i < topo_nr_cpus; i += topo_nr_threads) {
		for (j = 0; j < topo_nr_threads; j++)
			topo_threads[i+j] = j;
	}
}

static void __init init_topo_cores(void)
{
	int i, j;

	if (topo_nr_cores == 0)
		topo_nr_cores = topo_nr_cpus;

	for (i = 0; i < topo_nr_cpus; i += topo_nr_cores) {
		for (j = 0; j < topo_nr_cores; j++)
			topo_cores[i+j] = j;
	}
}

static void __init init_topo_packages(void)
{
	int i, j, packet_index = 0;
	int topo_nr_packages = topo_nr_cpus / (topo_nr_cores * topo_nr_threads);
	int div_package = topo_nr_cpus / topo_nr_packages;

	for (i = 0; i < topo_nr_cpus; i += div_package) {
		for (j = 0 ; j < div_package; j++)
			topo_packages[i+j] = packet_index;
		packet_index++;
	}
	if (packet_index > topo_nr_packages)
		pr_err("topo_cores init failed.\n");
}

static void __init init_topology_array(void)
{
	get_vt_smp_info();
	topo_nr_cpus = num_present_cpus();
	if (topo_nr_maxcpus > topo_nr_cpus)
		topo_nr_cpus = topo_nr_maxcpus;
	init_topo_threads();
	init_topo_cores();
	init_topo_packages();
}

/*
 * This function returns the logic cpu number of the node.
 * There are basically three kinds of return values:
 * (1) logic cpu number which is > 0.
 * (2) -ENODEV when the device tree(DT) node is valid and found in the DT but
 * there is no possible logical CPU in the kernel to match. This happens
 * when CONFIG_NR_CPUS is configure to be smaller than the number of
 * CPU nodes in DT. We need to just ignore this case.
 * (3) -1 if the node does not exist in the device tree
 */
static int __init get_cpu_for_node(struct device_node *node)
{
	struct device_node *cpu_node;
	int cpu;

	cpu_node = of_parse_phandle(node, "cpu", 0);
	if (!cpu_node)
		return -1;

	cpu = of_cpu_node_to_id(cpu_node);
	if (cpu >= 0)
		topology_parse_cpu_capacity(cpu_node, cpu);
	else
		pr_info("CPU node for %pOF exist but the possible cpu range is :%*pbl\n",
			cpu_node, cpumask_pr_args(cpu_possible_mask));

	of_node_put(cpu_node);
	return cpu;
}

static int __init parse_core(struct device_node *core, int package_id,
			     int cluster_id, int core_id)
{
	char name[20];
	bool leaf = true;
	int i = 0;
	int cpu;
	struct device_node *t;

	do {
		snprintf(name, sizeof(name), "thread%d", i);
		t = of_get_child_by_name(core, name);
		if (t) {
			leaf = false;
			cpu = get_cpu_for_node(t);
			if (cpu >= 0) {
				cpu_topology[cpu].package_id = package_id;
				cpu_topology[cpu].cluster_id = cluster_id;
				cpu_topology[cpu].core_id = core_id;
				cpu_topology[cpu].thread_id = i;
			} else if (cpu != -ENODEV) {
				pr_err("%pOF: Can't get CPU for thread\n", t);
				of_node_put(t);
				return -EINVAL;
			}
			of_node_put(t);
		}
		i++;
	} while (t);

	cpu = get_cpu_for_node(core);
	if (cpu >= 0) {
		if (!leaf) {
			pr_err("%pOF: Core has both threads and CPU\n",
			       core);
			return -EINVAL;
		}

		cpu_topology[cpu].package_id = package_id;
		cpu_topology[cpu].cluster_id = cluster_id;
		cpu_topology[cpu].core_id = core_id;
	} else if (leaf && cpu != -ENODEV) {
		pr_err("%pOF: Can't get CPU for leaf core\n", core);
		return -EINVAL;
	}

	return 0;
}

static int __init parse_cluster(struct device_node *cluster, int package_id,
				int cluster_id, int depth)
{
	char name[20];
	bool leaf = true;
	bool has_cores = false;
	struct device_node *c;
	int core_id = 0;
	int i, ret;

	/*
	 * First check for child clusters; we currently ignore any
	 * information about the nesting of clusters and present the
	 * scheduler with a flat list of them.
	 */
	i = 0;
	do {
		snprintf(name, sizeof(name), "cluster%d", i);
		c = of_get_child_by_name(cluster, name);
		if (c) {
			leaf = false;
			ret = parse_cluster(c, package_id, i, depth + 1);
			if (depth > 0)
				pr_warn("Topology for clusters of clusters not yet supported\n");
			of_node_put(c);
			if (ret != 0)
				return ret;
		}
		i++;
	} while (c);

	/* Now check for cores */
	i = 0;
	do {
		snprintf(name, sizeof(name), "core%d", i);
		c = of_get_child_by_name(cluster, name);
		if (c) {
			has_cores = true;

			if (depth == 0) {
				pr_err("%pOF: cpu-map children should be clusters\n",
				       c);
				of_node_put(c);
				return -EINVAL;
			}

			if (leaf) {
				ret = parse_core(c, package_id, cluster_id,
						 core_id++);
			} else {
				pr_err("%pOF: Non-leaf cluster with core %s\n",
				       cluster, name);
				ret = -EINVAL;
			}

			of_node_put(c);
			if (ret != 0)
				return ret;
		}
		i++;
	} while (c);

	if (leaf && !has_cores)
		pr_warn("%pOF: empty cluster\n", cluster);

	return 0;
}

static int __init parse_socket(struct device_node *socket)
{
	char name[20];
	struct device_node *c;
	bool has_socket = false;
	int package_id = 0, ret;

	do {
		snprintf(name, sizeof(name), "socket%d", package_id);
		c = of_get_child_by_name(socket, name);
		if (c) {
			has_socket = true;
			ret = parse_cluster(c, package_id, -1, 0);
			of_node_put(c);
			if (ret != 0)
				return ret;
		}
		package_id++;
	} while (c);

	if (!has_socket)
		ret = parse_cluster(socket, 0, -1, 0);

	return ret;
}

static int __init parse_dt_topology(void)
{
	struct device_node *cn, *map;
	int ret = 0;
	int cpu;

	cn = of_find_node_by_path("/cpus");
	if (!cn) {
		pr_err("No CPU information found in DT\n");
		return 0;
	}

	/*
	 * When topology is provided cpu-map is essentially a root
	 * cluster with restricted subnodes.
	 */
	map = of_get_child_by_name(cn, "cpu-map");
	if (!map)
		goto out;

	ret = parse_socket(map);
	if (ret != 0)
		goto out_map;

	topology_normalize_cpu_scale();

	/*
	 * Check that all cores are in the topology; the SMP code will
	 * only mark cores described in the DT as possible.
	 */
	for_each_possible_cpu(cpu)
		if (cpu_topology[cpu].package_id < 0) {
			ret = -EINVAL;
			break;
		}

out_map:
	of_node_put(map);
out:
	of_node_put(cn);
	return ret;
}

#ifdef CONFIG_ACPI
/*
 * Propagate the topology information of the processor_topology_node tree to the
 * cpu_topology array.
 */
int __init parse_acpi_topology(void)
{
	int cpu, topology_id;

	if (acpi_disabled)
		return 0;

	for_each_possible_cpu(cpu) {
		topology_id = find_acpi_cpu_topology(cpu, 0);
		if (topology_id < 0)
			return topology_id;

		if (acpi_pptt_cpu_is_thread(cpu) == 1) {
			cpu_topology[cpu].thread_id = topology_id;
			topology_id = find_acpi_cpu_topology(cpu, 1);
			cpu_topology[cpu].core_id   = topology_id;
		} else {
			cpu_topology[cpu].thread_id  = -1;
			cpu_topology[cpu].core_id    = topology_id;
		}
		topology_id = find_acpi_cpu_topology_cluster(cpu);
		cpu_topology[cpu].cluster_id = topology_id;
		topology_id = find_acpi_cpu_topology_package(cpu);
		cpu_topology[cpu].package_id = topology_id;
	}

	return 0;
}
#endif

void __init init_cpu_topology(void)
{
	struct cpu_topology *boot_cpu_topo = &cpu_topology[0];
	int cpu, ret;

	reset_cpu_topology();
	ret = parse_acpi_topology();
	if (!ret)
		ret = of_have_populated_dt() && parse_dt_topology();

	if (ret) {
		/*
		 * Discard anything that was parsed if we hit an error so we
		 * don't use partial information. But do not return yet to give
		 * arch-specific early cache level detection a chance to run.
		 */
		reset_cpu_topology();
	}

	/* Backward compatibility */
	if (is_guest_or_emul() && (boot_cpu_topo->package_id == -1))
		init_topology_array();

	for_each_possible_cpu(cpu) {
		ret = fetch_cache_info(cpu);
		if (!ret)
			continue;
		else if (ret != -ENOENT)
			pr_err("Early cacheinfo failed, ret = %d\n", ret);
		return;
	}
}

void store_cpu_topology(unsigned int cpu)
{
	struct cpu_topology *cpu_topo = &cpu_topology[cpu];

	if (cpu_topo->package_id != -1)
		goto topology_populated;

	if (is_guest_or_emul()) {
		cpu_topo->package_id = topo_packages[cpu];
		cpu_topo->core_id = topo_cores[cpu];
		cpu_topo->thread_id = topo_threads[cpu];
		goto topology_populated;
	}

	cpu_topo->package_id = rcid_to_domain_id(cpu_to_rcid(cpu));
	cpu_topo->core_id = rcid_to_core_id(cpu_to_rcid(cpu));
	cpu_topo->thread_id = rcid_to_thread_id(cpu_to_rcid(cpu));

	pr_debug("CPU%u: socket %d core %d thread %d\n",
		 cpu, cpu_topo->package_id, cpu_topo->core_id,
		 cpu_topo->thread_id);

topology_populated:
	update_siblings_masks(cpu);
}

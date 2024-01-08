// SPDX-License-Identifier: GPL-2.0

#include <linux/acpi.h>
#include <linux/of.h>
#include <linux/printk.h>
#include <asm/sw64io.h>
#include <asm/topology.h>

static int __init parse_dt_topology(void)
{
	return 0;
}

/*
 * cpu topology table
 */
struct cpu_topology cpu_topology[NR_CPUS];
EXPORT_SYMBOL_GPL(cpu_topology);

int topo_nr_threads, topo_nr_cores, topo_nr_maxcpus;

static int topo_nr_cpus;
static int topo_threads[NR_CPUS];
static int topo_cores[NR_CPUS];
static int topo_packages[NR_CPUS];

void __init get_vt_smp_info(void)
{
	unsigned long smp_info;

	smp_info = sw64_io_read(0, SMP_INFO);
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
	topo_nr_cpus = num_present_cpus();
	if (topo_nr_maxcpus > topo_nr_cpus)
		topo_nr_cpus = topo_nr_maxcpus;
	init_topo_threads();
	init_topo_cores();
	init_topo_packages();
}

const struct cpumask *cpu_coregroup_mask(int cpu)
{
	return topology_llc_cpumask(cpu);
}

static void update_siblings_masks(int cpu)
{
	struct cpu_topology *cpu_topo = &cpu_topology[cpu];
	int sib;

	/* update core and thread sibling masks */
	for_each_online_cpu(sib) {
		struct cpu_topology *sib_topo = &cpu_topology[sib];

		if (cpu_topo->package_id == sib_topo->package_id) {
			cpumask_set_cpu(cpu, &sib_topo->core_sibling);
			cpumask_set_cpu(sib, &cpu_topo->core_sibling);
			cpumask_set_cpu(cpu, &sib_topo->llc_sibling);
			cpumask_set_cpu(sib, &cpu_topo->llc_sibling);

			if (cpu_topo->core_id == sib_topo->core_id) {
				cpumask_set_cpu(cpu, &sib_topo->thread_sibling);
				cpumask_set_cpu(sib, &cpu_topo->thread_sibling);
			}
		}
	}
}

void store_cpu_topology(int cpu)
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

static void clear_cpu_topology(int cpu)
{
	struct cpu_topology *cpu_topo = &cpu_topology[cpu];

	cpumask_clear(&cpu_topo->llc_sibling);
	cpumask_set_cpu(cpu, &cpu_topo->llc_sibling);

	cpumask_clear(&cpu_topo->core_sibling);
	cpumask_set_cpu(cpu, &cpu_topo->core_sibling);
	cpumask_clear(&cpu_topo->thread_sibling);
	cpumask_set_cpu(cpu, &cpu_topo->thread_sibling);
}

static void __init reset_cpu_topology(void)
{
	int cpu;

	for_each_possible_cpu(cpu) {
		struct cpu_topology *cpu_topo = &cpu_topology[cpu];

		cpu_topo->thread_id = -1;
		cpu_topo->core_id = 0;
		cpu_topo->package_id = -1;

		clear_cpu_topology(cpu);
	}
}

void remove_cpu_topology(int cpu)
{
	int sibling;

	for_each_cpu(sibling, topology_core_cpumask(cpu))
		cpumask_clear_cpu(cpu, topology_core_cpumask(sibling));
	for_each_cpu(sibling, topology_sibling_cpumask(cpu))
		cpumask_clear_cpu(cpu, topology_sibling_cpumask(sibling));
	for_each_cpu(sibling, topology_llc_cpumask(cpu))
		cpumask_clear_cpu(cpu, topology_llc_cpumask(sibling));

	clear_cpu_topology(cpu);
}

#ifdef CONFIG_ACPI
static int __init parse_acpi_topology(void)
{
	return 0;
}
#else
static inline int __init parse_acpi_topology(void)
{
	return -EINVAL;
}
#endif

void __init init_cpu_topology(void)
{
	reset_cpu_topology();

	if (is_guest_or_emul())
		init_topology_array();
	/*
	 * Discard anything that was parsed if we hit an error so we
	 * don't use partial information.
	 */
	if (!acpi_disabled && parse_acpi_topology())
		reset_cpu_topology();
	else if (of_have_populated_dt() && parse_dt_topology())
		reset_cpu_topology();
}

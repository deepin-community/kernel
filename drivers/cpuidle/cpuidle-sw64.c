// SPDX-License-Identifier: GPL-2.0
/* SW64 generic CPU idle driver */

#include <linux/cpuidle.h>
#include <linux/cpumask.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <asm/sw64_init.h>
#include <asm/topology.h>

#define MAX_NODES 4
#define CORE_ID_BITMASK 0x1F
#define MAX_NR_LOCKS_PER_NODE 64

extern void arch_cpu_idle(void);
static void __iomem *spbu_base[MAX_NODES];
static int cpu_siblings[NR_CPUS];
static bool cpu_deeper_states[NR_CPUS];
static spinlock_t per_core_lock[MAX_NODES][MAX_NR_LOCKS_PER_NODE];

/* CLU_LV2_SEL offset for LV2 MUX */
#define OFFSET_CLU_LV2_SELH	0x3a00UL
#define OFFSET_CLU_LV2_SELL	0x3b00UL
#define OFFSET_CLU_LV2(core) ((core) > 31 ? OFFSET_CLU_LV2_SELH : OFFSET_CLU_LV2_SELL)

static bool can_switch_freq(int cpu_sibling)
{
	return  cpu_sibling < 0 ||
		!cpu_online(cpu_sibling) ||
		cpu_deeper_states[cpu_sibling];
}

/*
 * sw64_do_deeper_idle
 *
 * Handles frequency scaling by writing specific bits to
 * CLU_LV2_SEL when CPU core wants a deeper idle state.
 */
extern unsigned int cpufreq_quick_get(unsigned int cpu);
extern void sw64_cpuidle_updatevents(int freq);

static void sw64_do_deeper_idle(int cpu)
{
	int core_id, node_id, cpu_sibling;
	int cur_freq, downshift_freq = 200000;
	unsigned long flags;
	u64 freq_scaling;

	core_id = rcid_to_core_id(cpu_to_rcid(cpu));
	node_id = rcid_to_domain_id(cpu_to_rcid(cpu));
	cpu_sibling = cpu_siblings[cpu];

	cur_freq = cpufreq_quick_get(cpu);

	/* downshift frequency before idle if possible*/
	spin_lock_irqsave(&per_core_lock[node_id][core_id], flags);
	cpu_deeper_states[cpu] = true;
	if (can_switch_freq(cpu_sibling)) {
		freq_scaling = 0x1UL << (2 * (core_id & CORE_ID_BITMASK));
		writeq(freq_scaling, spbu_base[node_id] + OFFSET_CLU_LV2(core_id));
	}

	sw64_cpuidle_updatevents(downshift_freq);

	spin_unlock_irqrestore(&per_core_lock[node_id][core_id], flags);

	rcu_idle_enter();
	arch_cpu_idle();
	local_irq_disable();
	rcu_idle_exit();
	local_irq_enable();

	/* upshift frequency after idle */
	spin_lock_irqsave(&per_core_lock[node_id][core_id], flags);
	cpu_deeper_states[cpu] = false;
	freq_scaling = 0x3UL << (2 * (core_id & CORE_ID_BITMASK));
	writeq(freq_scaling, spbu_base[node_id] + OFFSET_CLU_LV2(core_id));
	sw64_cpuidle_updatevents(cur_freq);
	spin_unlock_irqrestore(&per_core_lock[node_id][core_id], flags);
}

/*
 * sw64_idle_enter - Programs CPU to enter the specified state
 *
 * dev: cpuidle device
 * drv: cpuidle driver
 * index: state index
 *
 * Called from the cpuidle framework to program the device to the
 * specified target state selected by the governor.
 */
static int sw64_idle_enter(struct cpuidle_device *dev,
			   struct cpuidle_driver *drv,
			   int index)
{

	int cpu = smp_processor_id();
	bool deeper_idle = IS_ENABLED(CONFIG_SW64_CPUFREQ);

	if (!is_in_host()) {
		arch_cpu_idle();
		return index;
	}

	if (cpu == 0)
		deeper_idle = false;

	if (index > 0 && deeper_idle)
		sw64_do_deeper_idle(cpu);
	else
		arch_cpu_idle();

	return index;
}

static struct cpuidle_driver sw64_idle_driver = {
	.name = "sw64_idle",
	.owner = THIS_MODULE,
	/*
	 * State at index 0 is halt and considered standard on
	 * all SW64 platforms. State at index 1 represents an
	 * idle state where the CPU enters a halt state combined
	 * with frequency scaling, which means the CPU core run
	 * at a reduced frequnecy (e.g., 200MHz) to achieve power
	 * savings.
	 *
	 * To enbale State 1, CONFIG_SW64_CPUFREQ must be enabled,
	 * otherwise the required frequency scaling support will
	 * be unavailable.
	 */
	.states[0] = {
		.name = "C1",
		.desc = "halt",
		.exit_latency = 1,
		.target_residency = 1,
		.enter = sw64_idle_enter,
	},
	.states[1] = {
		.name = "C6",
		.desc = "freq downshift",
		.exit_latency = 200,
		.target_residency = 200,
		.enter = sw64_idle_enter,
		.flags = CPUIDLE_FLAG_RCU_IDLE,
	},
	.state_count = 2,
};

static int get_sibling_cpu(int cpu)
{
	const cpumask_t *mask = topology_sibling_cpumask(cpu);
	int sibling;

	for_each_cpu(sibling, mask) {
		if (sibling != cpu)
			return sibling;
	}

	return -1;
}

static int __init sw64_cpuidle_init(void)
{
	int i, cpu;
	int core_id, node_id;

	for_each_online_cpu(cpu) {
		cpu_siblings[cpu] = get_sibling_cpu(cpu);
		core_id = rcid_to_core_id(cpu_to_rcid(cpu));
		node_id = rcid_to_domain_id(cpu_to_rcid(cpu));
		spin_lock_init(&per_core_lock[node_id][core_id]);
	}

	for (i = 0; i < MAX_NODES; ++i)
		spbu_base[i] = misc_platform_get_spbu_base(i);

	return cpuidle_register(&sw64_idle_driver, cpu_possible_mask);
}

static void __exit sw64_cpuidle_exit(void)
{
	cpuidle_unregister(&sw64_idle_driver);
}
device_initcall(sw64_cpuidle_init);

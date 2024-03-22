// SPDX-License-Identifier: GPL-2.0
#include <linux/export.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/jump_label.h>
#include <linux/kvm_para.h>
#include <asm/paravirt.h>
#include <linux/reboot.h>
#include <linux/static_call.h>

struct static_key paravirt_steal_enabled;
struct static_key paravirt_steal_rq_enabled;
static DEFINE_PER_CPU(struct kvm_steal_time, steal_time) __aligned(64);
static int has_steal_clock;

static u64 native_steal_clock(int cpu)
{
	return 0;
}

DEFINE_STATIC_CALL(pv_steal_clock, native_steal_clock);

static bool steal_acc = true;
static int __init parse_no_stealacc(char *arg)
{
	steal_acc = false;
	return 0;
}
early_param("no-steal-acc", parse_no_stealacc);

static u64 para_steal_clock(int cpu)
{
	u64 steal;
	struct kvm_steal_time *src;
	int version;

	src = &per_cpu(steal_time, cpu);
	do {

		version = src->version;
		/* Make sure that the version is read before the steal */
		virt_rmb();
		steal = src->steal;
		/* Make sure that the steal is read before the next version */
		virt_rmb();

	} while ((version & 1) || (version != src->version));
	return steal;
}

static int pv_register_steal_time(void)
{
	int cpu = smp_processor_id();
	struct kvm_steal_time *st;
	unsigned long addr;

	if (!has_steal_clock)
		return -EPERM;

	st = &per_cpu(steal_time, cpu);
	addr = per_cpu_ptr_to_phys(st);

	/* The whole structure kvm_steal_time should be one page */
	if (PFN_DOWN(addr) != PFN_DOWN(addr + sizeof(*st))) {
		pr_warn("Illegal PV steal time addr %lx\n", addr);
		return -EFAULT;
	}

	addr |= KVM_STEAL_PHYS_VALID;
	kvm_hypercall2(KVM_HCALL_FUNC_NOTIFY, KVM_FEATURE_STEAL_TIME, addr);
	return 0;
}

#ifdef CONFIG_SMP
static void pv_send_ipi_single(int cpu, unsigned int action)
{
	unsigned int min, old;
	irq_cpustat_t *info = &per_cpu(irq_stat, cpu);

	old = atomic_fetch_or(BIT(action), &info->message);
	if (old)
		return;

	min = cpu_logical_map(cpu);
	kvm_hypercall3(KVM_HCALL_FUNC_PV_IPI, 1, 0, min);
}

#define KVM_IPI_CLUSTER_SIZE		(2 * BITS_PER_LONG)
static void pv_send_ipi_mask(const struct cpumask *mask, unsigned int action)
{
	unsigned int cpu, i, min = 0, max = 0, old;
	__uint128_t bitmap = 0;
	irq_cpustat_t *info;

	if (cpumask_empty(mask))
		return;

	action = BIT(action);
	for_each_cpu(i, mask) {
		info = &per_cpu(irq_stat, i);
		old = atomic_fetch_or(action, &info->message);
		if (old)
			continue;

		cpu = cpu_logical_map(i);
		if (!bitmap) {
			min = max = cpu;
		} else if (cpu > min && cpu < min + KVM_IPI_CLUSTER_SIZE) {
			max = cpu > max ? cpu : max;
		} else if (cpu < min && (max - cpu) < KVM_IPI_CLUSTER_SIZE) {
			bitmap <<= min - cpu;
			min = cpu;
		} else {
			/*
			 * Physical cpuid is sorted in ascending order ascend
			 * for the next mask calculation, send IPI here
			 * directly and skip the remainding cpus
			 */
			kvm_hypercall3(KVM_HCALL_FUNC_PV_IPI,
				(unsigned long)bitmap,
				(unsigned long)(bitmap >> BITS_PER_LONG), min);
			min = max = cpu;
			bitmap = 0;
		}
		__set_bit(cpu - min, (unsigned long *)&bitmap);
	}

	if (bitmap)
		kvm_hypercall3(KVM_HCALL_FUNC_PV_IPI, (unsigned long)bitmap,
				(unsigned long)(bitmap >> BITS_PER_LONG), min);
}

static irqreturn_t loongson_do_swi(int irq, void *dev)
{
	irq_cpustat_t *info;
	long action;

	/* Clear swi interrupt */
	clear_csr_estat(1 << INT_SWI0);
	info = this_cpu_ptr(&irq_stat);
	action = atomic_xchg(&info->message, 0);
	if (action & SMP_CALL_FUNCTION) {
		generic_smp_call_function_interrupt();
		info->ipi_irqs[IPI_CALL_FUNCTION]++;
	}

	if (action & SMP_RESCHEDULE) {
		scheduler_ipi();
		info->ipi_irqs[IPI_RESCHEDULE]++;
	}

	return IRQ_HANDLED;
}

static void pv_init_ipi(void)
{
	int r, swi0;

	swi0 = get_percpu_irq(INT_SWI0);
	if (swi0 < 0)
		panic("SWI0 IRQ mapping failed\n");
	irq_set_percpu_devid(swi0);
	r = request_percpu_irq(swi0, loongson_do_swi, "SWI0", &irq_stat);
	if (r < 0)
		panic("SWI0 IRQ request failed\n");
}

static void pv_disable_steal_time(void)
{
	if (has_steal_clock)
		kvm_hypercall2(KVM_HCALL_FUNC_NOTIFY, KVM_FEATURE_STEAL_TIME, 0);
}

static int pv_cpu_online(unsigned int cpu)
{
	unsigned long flags;

	local_irq_save(flags);
	pv_register_steal_time();
	local_irq_restore(flags);
	return 0;
}

static int pv_cpu_down_prepare(unsigned int cpu)
{
	unsigned long flags;

	local_irq_save(flags);
	pv_disable_steal_time();
	local_irq_restore(flags);
	return 0;
}
#endif

static bool kvm_para_available(void)
{
	static int hypervisor_type;
	int config;

	if (!hypervisor_type) {
		config = read_cpucfg(CPUCFG_KVM_SIG);
		if (!memcmp(&config, KVM_SIGNATURE, 4))
			hypervisor_type = HYPERVISOR_KVM;
	}

	return hypervisor_type == HYPERVISOR_KVM;
}

int __init pv_ipi_init(void)
{
	int feature;

	if (!cpu_has_hypervisor)
		return 0;
	if (!kvm_para_available())
		return 0;

	/*
	 * check whether KVM hypervisor supports pv_ipi or not
	 */
	feature = read_cpucfg(CPUCFG_KVM_FEATURE);
#ifdef CONFIG_SMP
	if (feature & KVM_FEATURE_PV_IPI) {
		smp_ops.init_ipi		= pv_init_ipi;
		smp_ops.send_ipi_single		= pv_send_ipi_single;
		smp_ops.send_ipi_mask		= pv_send_ipi_mask;
	}
#endif

	return 1;
}

static void pv_cpu_reboot(void *unused)
{
	pv_disable_steal_time();
}

static int pv_reboot_notify(struct notifier_block *nb, unsigned long code,
		void *unused)
{
	on_each_cpu(pv_cpu_reboot, NULL, 1);
	return NOTIFY_DONE;
}

static struct notifier_block pv_reboot_nb = {
	.notifier_call  = pv_reboot_notify,
};

int __init pv_time_init(void)
{
	int feature;

	if (!cpu_has_hypervisor)
		return 0;
	if (!kvm_para_available())
		return 0;

	feature = read_cpucfg(CPUCFG_KVM_FEATURE);
	if (!(feature & KVM_FEATURE_STEAL_TIME))
		return 0;

	has_steal_clock = 1;
	if (pv_register_steal_time()) {
		has_steal_clock = 0;
		return 0;
	}

	register_reboot_notifier(&pv_reboot_nb);
	static_call_update(pv_steal_clock, para_steal_clock);
	static_key_slow_inc(&paravirt_steal_enabled);
	if (steal_acc)
		static_key_slow_inc(&paravirt_steal_rq_enabled);

#ifdef CONFIG_SMP
	if (cpuhp_setup_state_nocalls(CPUHP_AP_ONLINE_DYN, "loongarch/pv:online",
				pv_cpu_online, pv_cpu_down_prepare) < 0)
		pr_err("Failed to install cpu hotplug callbacks\n");
#endif
	pr_info("Using stolen time PV\n");
	return 0;
}

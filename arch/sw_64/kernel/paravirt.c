// SPDX-License-Identifier: GPL-2.0-only

#include <linux/cpuhotplug.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/jump_label.h>
#include <linux/printk.h>
#include <linux/psci.h>
#include <linux/reboot.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/static_call.h>
#include <linux/kvm_host.h>

#include <asm/paravirt.h>
#include <asm/pvtime.h>
#include <asm/qspinlock_paravirt.h>

struct static_key paravirt_steal_enabled;
struct static_key paravirt_steal_rq_enabled;

struct paravirt_patch_template pv_ops = {
#ifdef CONFIG_PARAVIRT_SPINLOCKS
	.lock.queued_spin_lock_slowpath	= native_queued_spin_lock_slowpath,
	.lock.queued_spin_unlock	= native_queued_spin_unlock,
#endif
	.lock.vcpu_is_preempted		= __native_vcpu_is_preempted,
};
EXPORT_SYMBOL_GPL(pv_ops);

#ifdef CONFIG_PARAVIRT_SPINLOCKS
static bool pvqspinlock;

static __init int parse_pvqspinlock(char *arg)
{
	pvqspinlock = true;
	return 0;
}
early_param("pvqspinlock", parse_pvqspinlock);

void __init pv_qspinlock_init(void)
{
	/* Don't use the PV qspinlock code if there is only 1 vCPU. */
	if (num_possible_cpus() == 1)
		return;

	if (!pvqspinlock) {
		pr_info("PV qspinlocks disabled\n");
		return;
	}

	pr_info("PV qspinlocks enabled\n");

	__pv_init_lock_hash();
	pv_ops.lock.queued_spin_lock_slowpath = __pv_queued_spin_lock_slowpath;
	pv_ops.lock.queued_spin_unlock = __pv_queued_spin_unlock;
	/* TODO: wait and kick */
	pv_ops.lock.wait = NULL;
	pv_ops.lock.kick = NULL;
}
#endif

static u64 native_steal_clock(int cpu)
{
	return 0;
}

DEFINE_STATIC_CALL(pv_steal_clock, native_steal_clock);

static DEFINE_PER_CPU(struct pvclock_vcpu_steal_time, pvclock_steal_time) __aligned(128);

static bool steal_acc = true;
static int __init parse_no_stealacc(char *arg)
{
	steal_acc = false;
	return 0;
}

early_param("no-steal-acc", parse_no_stealacc);

/* return steal time in ns */
static u64 para_steal_clock(int cpu)
{
	struct pvclock_vcpu_steal_time  *st = per_cpu_ptr(&pvclock_steal_time, cpu);
	u64 steal;
	int version;

	do {
		version = READ_ONCE(st->version);
		virt_rmb();
		steal = READ_ONCE(st->steal_time);
		virt_rmb();
	} while ((version & 1) ||
			version != READ_ONCE(st->version));

	return steal;
}

static int steal_time_cpu_online(unsigned int cpu)
{
	struct pvclock_vcpu_steal_time *st = this_cpu_ptr(&pvclock_steal_time);

	hcall(HCALL_SET_PVTIME_ST, __pa(st), 0, 0);

	return 0;
}

static int steal_time_cpu_down_prepare(unsigned int cpu)
{
	hcall(HCALL_SET_PVTIME_ST, INVALID_GPA, 0, 0);
	return 0;
}

int __init pv_steal_time_init(void)
{
	int ret;

	ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN,
				"sw64/pvtime:online",
				steal_time_cpu_online,
				steal_time_cpu_down_prepare);
	if (ret < 0)
		return ret;

	static_call_update(pv_steal_clock, para_steal_clock);

	static_key_slow_inc(&paravirt_steal_enabled);
	if (steal_acc)
		static_key_slow_inc(&paravirt_steal_rq_enabled);

	pr_info("using paravirt steal time\n");

	return 0;
}

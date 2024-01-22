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

#include <asm/paravirt.h>
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

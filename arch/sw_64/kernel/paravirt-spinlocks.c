// SPDX-License-Identifier: GPL-2.0-only

#include <linux/spinlock.h>
#include <asm/paravirt.h>

__visible bool __native_vcpu_is_preempted(int cpu)
{
	return false;
}

bool pv_is_native_spin_unlock(void)
{
	return false;
}

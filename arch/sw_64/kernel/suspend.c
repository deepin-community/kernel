// SPDX-License-Identifier: GPL-2.0
#include <linux/suspend.h>

#include <asm/suspend.h>
#include <asm/sw64_init.h>

#define	PME_EN	0x2

struct processor_state suspend_state;

static int native_suspend_state_valid(suspend_state_t pm_state)
{
	switch (pm_state) {
	case PM_SUSPEND_ON:
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		return 1;
	default:
		return 0;
	}
}

void disable_local_timer(void)
{
	wrtimer(0);
}

extern struct pci_controller *hose_head;

/*
 * Boot Core will enter suspend stat here.
 */
void sw64_suspend_enter(void)
{
	/* boot processor will go to deep sleep mode from here
	 * After wake up  boot processor, pc will go here
	 */
	disable_local_timer();
	current_thread_info()->pcb.tp = rtid();

	if (!is_junzhang_v1())
		sw64_suspend_deep_sleep(&suspend_state);
	else {
		pme_state = PME_WFW;
		sw64_write_csr_imb(PME_EN, CSR_INT_EN);
		asm("halt");
		local_irq_disable();
	}

	wrtp(current_thread_info()->pcb.tp);

	disable_local_timer();
}

static int native_suspend_enter(suspend_state_t state)
{
	if (is_in_guest())
		return 0;
	/* processor specific suspend */
	sw64_suspend_enter();
	return 0;
}

const struct platform_suspend_ops native_suspend_ops = {
	.valid = native_suspend_state_valid,
	.enter = native_suspend_enter,
};

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 - os kernal
 * Author: fire3 <fire3@example.com> yangzh <yangzh@gmail.com>
 */
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/kvm_host.h>
#include <asm/kvm_timer.h>

/*
 * The Guest Clock.
 *
 * There are two sources of virtual interrupts.  We saw one in lguest_user.c:
 * the Launcher sending interrupts for virtual devices.  The other is the Guest
 * timer interrupt.
 *
 * The Guest uses the LHCALL_SET_CLOCKEVENT hypercall to tell us how long to
 * the next timer interrupt (in ticks).  We use the high-resolution timer
 * infrastructure to set a callback at that time.
 *
 * 0 means "turn off the clock".
 */

void set_timer(struct kvm_vcpu *vcpu, unsigned long delta)
{
	ktime_t expires;

	if (unlikely(delta == 0)) {
		/* Clock event device is shutting down. */
		hrtimer_cancel(&vcpu->arch.hrt);
		return;
	}

	/* Convert clock event device ticks to nanoseconds */
	delta = delta * NSEC_PER_SEC;
	do_div(delta, vcpu->arch.vtimer_freq);

	/*
	 * We use wallclock time here, so the Guest might not be running for
	 * all the time between now and the timer interrupt it asked for.  This
	 * is almost always the right thing to do.
	 */

	expires = ktime_add_ns(ktime_get_real(), delta);
	vcpu->arch.timer_next_event = expires;
	hrtimer_start(&vcpu->arch.hrt, expires, HRTIMER_MODE_ABS);
}

/* And this is the routine when we want to set an interrupt for the Guest. */
void set_interrupt(struct kvm_vcpu *vcpu, unsigned int irq)
{
	/*
	 * Next time the Guest runs, the core code will see if it can deliver
	 * this interrupt.
	 */
	set_bit(irq, (vcpu->arch.irqs_pending));

	/*
	 * Make sure it sees it; it might be asleep (eg. halted), or running
	 * the Guest right now, in which case kick_process() will knock it out.
	 */
	kvm_vcpu_kick(vcpu);
}

enum hrtimer_restart clockdev_fn(struct hrtimer *timer)
{
	struct kvm_vcpu *vcpu;
	ktime_t now, delta;

	vcpu = container_of(timer, struct kvm_vcpu, arch.hrt);

	now = ktime_get_real();

	if (now < vcpu->arch.timer_next_event) {
		delta = vcpu->arch.timer_next_event - now;
		hrtimer_forward_now(timer, delta);
		return HRTIMER_RESTART;
	}

	set_interrupt(vcpu, SW64_KVM_IRQ_TIMER);
	return HRTIMER_NORESTART;
}

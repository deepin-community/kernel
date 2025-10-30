// SPDX-License-Identifier: GPL-2.0
// Copyright (C) 2025 SW64 Ltd.

#include <linux/kvm_host.h>
#include <linux/sched/stat.h>
#include <asm/pvtime.h>

void kvm_sw64_record_steal_time(struct kvm_vcpu *vcpu)
{
	struct kvm *kvm = vcpu->kvm;
	u64 base = vcpu->arch.steal.base;
	u64 last_steal = vcpu->arch.steal.last_steal;
	u64 __user *steal_ptr;
	u32 __user *version_ptr;
	u64 offset_s = offsetof(struct pvclock_vcpu_steal_time, steal_time);
	u64 offset_v = offsetof(struct pvclock_vcpu_steal_time, version);
	u64 steal, stealInc, hva;
	u32 version;
	gfn_t gfn;
	int idx;

	if (base == INVALID_GPA)
		return;

	idx = srcu_read_lock(&kvm->srcu);
	gfn = base >> PAGE_SHIFT;
	hva = kvm_vcpu_gfn_to_hva(vcpu, gfn);
	if (WARN_ON(kvm_is_error_hva(hva))) {
		vcpu->arch.steal.base = INVALID_GPA;
		return;
	}

	steal_ptr = (__u64 __user *)(hva + offset_in_page(base) + offset_s);
	version_ptr = (__u32 __user *)(hva + offset_in_page(base) + offset_v);

	if (WARN_ON(get_user(version, version_ptr)))
		return;

	version += 1;

	if (WARN_ON(put_user(version, version_ptr)))
		return;

	if (!WARN_ON(get_user(steal, steal_ptr))) {
		vcpu->arch.steal.last_steal = READ_ONCE(current->sched_info.run_delay);
		stealInc += vcpu->arch.steal.last_steal - last_steal;
		if (stealInc) {
			steal += stealInc;
			WARN_ON(put_user(steal, steal_ptr));
		}
	}

	version += 1;
	WARN_ON(put_user(version, version_ptr));

	kvm_vcpu_mark_page_dirty(vcpu, gfn);
	srcu_read_unlock(&kvm->srcu, idx);
}

void kvm_init_steal_time(struct kvm_vcpu *vcpu)
{
	struct pvclock_vcpu_steal_time init_values = {};
	struct kvm *kvm = vcpu->kvm;
	u64 base = vcpu->arch.steal.base;
	int idx;

	if (base == INVALID_GPA)
		return;

	/*
	 * Start counting stolen time from the time the guest
	 * requests to set pvtime.
	 */
	vcpu->arch.steal.last_steal = current->sched_info.run_delay;
	idx = srcu_read_lock(&kvm->srcu);
	kvm_write_guest(kvm, base, &init_values, sizeof(init_values));
	srcu_read_unlock(&kvm->srcu, idx);
}

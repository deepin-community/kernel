// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 - os kernal
 * Author: fire3 <fire3@example.com> yangzh <yangzh@gmail.com>
 * linhn <linhn@example.com>
 */
#include <asm/hmcall.h>
#include <asm/kvm_asm.h>
#include <asm/kvm_emulate.h>
#include <asm/kvm_timer.h>
#include <asm/kvm_mmu.h>
#include <linux/kvm.h>
#include <linux/kvm_host.h>

int handle_exit(struct kvm_vcpu *vcpu, struct kvm_run *run,
		int exception_index, struct hcall_args *hargs)
{
	gfn_t gfn __maybe_unused;

	switch (exception_index) {
	case SW64_KVM_EXIT_IO:
		vcpu->stat.io_exits++;
		return io_mem_abort(vcpu, run, hargs);
	case SW64_KVM_MIGRATION_SET_DIRTY_HM:
	case SW64_KVM_MIGRATION_SET_DIRTY:
		vcpu->stat.migration_set_dirty++;
		gfn = hargs->arg2 >> 24;
		mutex_lock(&vcpu->kvm->slots_lock);
		kvm_vcpu_mark_page_dirty(vcpu, gfn);
		mutex_unlock(&vcpu->kvm->slots_lock);
		return 1;
	case SW64_KVM_EXIT_HALT:
		vcpu->stat.halt_exits++;
		vcpu->arch.halted = 1;
		kvm_vcpu_block(vcpu);
		return 1;
	case SW64_KVM_EXIT_SHUTDOWN:
		vcpu->stat.shutdown_exits++;
		vcpu->run->exit_reason = KVM_EXIT_SYSTEM_EVENT;
		vcpu->run->system_event.type = KVM_SYSTEM_EVENT_SHUTDOWN;
		return 0;
	case SW64_KVM_EXIT_RESTART:
		vcpu->stat.restart_exits++;
		vcpu->run->exit_reason = KVM_EXIT_SYSTEM_EVENT;
		vcpu->run->system_event.type = KVM_SYSTEM_EVENT_RESET;
		return 0;
	case SW64_KVM_EXIT_STOP:
		vcpu->stat.stop_exits++;
		vcpu->arch.halted = 1;
		memset(&vcpu->arch.irqs_pending, 0, sizeof(vcpu->arch.irqs_pending));
		kvm_vcpu_block(vcpu);
		return 1;
	case SW64_KVM_EXIT_TIMER:
		vcpu->stat.timer_exits++;
		set_timer(vcpu, hargs->arg0);
		return 1;
	case SW64_KVM_EXIT_IPI:
		vcpu->stat.ipi_exits++;
		vcpu_send_ipi(vcpu, hargs->arg0, hargs->arg1);
		return 1;
	case SW64_KVM_EXIT_DEBUG:
		vcpu->stat.debug_exits++;
		vcpu->run->exit_reason = KVM_EXIT_DEBUG;
		vcpu->run->debug.arch.epc = vcpu->arch.regs.pc;
		return 0;
#ifdef CONFIG_SUBARCH_C4
	case SW64_KVM_EXIT_APT_FAULT:
		return kvm_handle_guest_abort(vcpu, run, hargs);
#endif
	case SW64_KVM_EXIT_FATAL_ERROR:
		vcpu->stat.fatal_error_exits++;
		pr_err("Guest fatal error: Reason=[%lx], EXC_PC=[%lx], DVA=[%lx]", hargs->arg0, hargs->arg1, hargs->arg2);
		vcpu->run->exit_reason = KVM_EXIT_UNKNOWN;
		vcpu->run->hw.hardware_exit_reason = hargs->arg0;
		return 0;
	}

	return 1;
}

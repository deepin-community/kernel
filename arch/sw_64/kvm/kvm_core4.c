// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 - os kernal
 * Author: fire3 <fire3@example.com> yangzh <yangzh@gmail.com>
 * linhn <linhn@example.com>
 */

#include <linux/errno.h>
#include <linux/kvm_host.h>
#include <linux/module.h>
#include <linux/mman.h>
#include <linux/sched/signal.h>
#include <linux/kvm.h>
#include <linux/uaccess.h>

#include <asm/kvm_timer.h>
#include <asm/kvm_emulate.h>
#include <asm/kvm_mmu.h>
#include <asm/barrier.h>
#include <asm/pci_impl.h>
#include "trace.h"

static unsigned long shtclock_offset;

void update_aptp(unsigned long pgd)
{
	imemb();
	sw64_write_csr_imb(pgd, CSR_APTP);
}

void kvm_sw64_update_vpn(struct kvm_vcpu *vcpu, unsigned long vpn)
{
	vcpu->arch.vcb.vpcr = vpn << 44;
	vcpu->arch.vcb.dtb_vpcr = vpn;
}

void kvm_flush_tlb_all(void)
{
	tbivpn(-1, 0, 0);
}

int kvm_sw64_init_vm(struct kvm *kvm)
{
	return kvm_alloc_addtional_stage_pgd(kvm);
}

void kvm_sw64_destroy_vm(struct kvm *kvm)
{
	kvm_destroy_vcpus(kvm);
}

int kvm_sw64_vcpu_reset(struct kvm_vcpu *vcpu)
{
	if (vcpu->arch.has_run_once)
		apt_unmap_vm(vcpu->kvm);

	hrtimer_cancel(&vcpu->arch.hrt);
	vcpu->arch.pcpu_id = -1; /* force flush tlb for the first time */
	vcpu->arch.power_off = 0;
	memset(&vcpu->arch.vcb.irqs_pending, 0, sizeof(vcpu->arch.vcb.irqs_pending));

	return 0;
}

long kvm_sw64_get_vcb(struct file *filp, unsigned long arg)
{
	struct kvm_vcpu *vcpu = filp->private_data;

	if (vcpu->arch.vcb.migration_mark)
		vcpu->arch.vcb.shtclock = sw64_read_csr(CSR_SHTCLOCK) +
			vcpu->arch.vcb.shtclock_offset;
	if (copy_to_user((void __user *)arg, &(vcpu->arch.vcb), sizeof(struct vcpucb)))
		return -EINVAL;

	return 0;
}

long kvm_sw64_set_vcb(struct file *filp, unsigned long arg)
{
	struct kvm_vcpu *vcpu = filp->private_data;
	struct vcpucb *kvm_vcb;

	kvm_vcb = memdup_user((void __user *)arg, sizeof(*kvm_vcb));
	memcpy(&(vcpu->arch.vcb), kvm_vcb, sizeof(struct vcpucb));

	if (vcpu->arch.vcb.migration_mark) {
		/* synchronize the longtime of source and destination */
		if (vcpu->arch.vcb.soft_cid == 0)
			shtclock_offset = vcpu->arch.vcb.shtclock -
						sw64_read_csr(CSR_SHTCLOCK);
		vcpu->arch.vcb.shtclock_offset = shtclock_offset;
		set_timer(vcpu, 200000000);
		vcpu->arch.vcb.migration_mark = 0;
	}
	return 0;
}

int kvm_cpu_has_pending_timer(struct kvm_vcpu *vcpu)
{
	return test_bit(SW64_KVM_IRQ_TIMER, vcpu->arch.vcb.irqs_pending);
}

int kvm_arch_vcpu_runnable(struct kvm_vcpu *vcpu)
{
	if (vcpu->arch.restart)
		return 1;

	if (vcpu->arch.vcb.vcpu_irq_disabled)
		return 0;

	return ((!bitmap_empty(vcpu->arch.vcb.irqs_pending, SWVM_IRQS) ||
			!vcpu->arch.halted) && !vcpu->arch.power_off);
}

int vcpu_interrupt_line(struct kvm_vcpu *vcpu, int number)
{
	int cpu = vcpu->cpu;
	int me = smp_processor_id();

	set_bit(number, (vcpu->arch.vcb.irqs_pending));

	if (vcpu->mode == IN_GUEST_MODE) {
		if (cpu != me && (unsigned int)cpu < nr_cpu_ids
				&& cpu_online(cpu)) {
			if (vcpu->arch.vcb.vcpu_irq_disabled)
				return 0;
			send_ipi(cpu, II_II1);
		}
	} else
		kvm_vcpu_kick(vcpu);
	return 0;
}

void vcpu_send_ipi(struct kvm_vcpu *vcpu, int target_vcpuid, int type)
{
	struct kvm_vcpu *target_vcpu = kvm_get_vcpu(vcpu->kvm, target_vcpuid);

	if (target_vcpu == NULL)
		return;
	if (type == II_RESET) {
		target_vcpu->arch.restart = 1;
		kvm_vcpu_kick(target_vcpu);
	} else
		vcpu_interrupt_line(target_vcpu, 1);
}

void sw64_kvm_clear_irq(struct kvm_vcpu *vcpu)
{
	memset(&vcpu->arch.vcb.irqs_pending, 0, sizeof(vcpu->arch.vcb.irqs_pending));
}

void sw64_kvm_try_deliver_interrupt(struct kvm_vcpu *vcpu)
{
}

int kvm_arch_prepare_memory_region(struct kvm *kvm,
				const struct kvm_memory_slot *old,
				struct kvm_memory_slot *new,
				enum kvm_mr_change change)
{
	return 0;
}

void vcpu_set_numa_affinity(struct kvm_vcpu *vcpu)
{
}

static int __init kvm_core4_init(void)
{
	int i, ret;

	for (i = 0; i < NR_CPUS; i++)
		last_vpn(i) = VPN_FIRST_VERSION;

	ret = kvm_init(sizeof(struct kvm_vcpu), 0, THIS_MODULE);

	if (ret)
		return ret;

	return 0;
}

static void __exit kvm_core4_exit(void)
{
	kvm_exit();
}

module_init(kvm_core4_init);
module_exit(kvm_core4_exit);

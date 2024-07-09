// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 - os kernal
 * Author: fire3 <fire3@example.com> yangzh <yangzh@gmail.com>
 * linhn <linhn@example.com>
 */

#include <linux/debugfs.h>
#include <linux/errno.h>
#include <linux/kvm_host.h>
#include <linux/module.h>
#include <linux/mman.h>
#include <linux/sched/signal.h>
#include <linux/kvm.h>
#include <linux/uaccess.h>

#include <asm/debug.h>
#include <asm/kvm_timer.h>
#include <asm/kvm_emulate.h>
#include <asm/kvm_mmu.h>
#include <asm/barrier.h>
#include <asm/pci_impl.h>

#include "trace.h"
#include "vmem.c"

#define OFFSET_LONG_TIME	0x180UL

__read_mostly bool bind_vcpu_enabled;

#if defined(CONFIG_DEBUG_FS) && defined(CONFIG_NUMA)
static int __init bind_vcpu_init(void)
{
	if (!sw64_debugfs_dir)
		return -ENODEV;
	debugfs_create_bool("bind_vcpu", 0644,
			sw64_debugfs_dir, &bind_vcpu_enabled);
	return 0;
}

static void bind_vcpu_exit(void)
{
	bind_vcpu_enabled = false;
}
#else
static int __init bind_vcpu_init(void)
{
	return 0;
}

static void bind_vcpu_exit(void) { }

#endif

static unsigned long longtime_offset;

static unsigned long get_vpcr(struct kvm_vcpu *vcpu, u64 vpn)
{
	unsigned long base, size;

	base = vcpu->kvm->arch.host_phys_addr;
	size = vcpu->kvm->arch.size;
	return (base >> 23) | ((size >> 23) << 16) | ((vpn & VPN_MASK) << 44);
}

void vcpu_set_numa_affinity(struct kvm_vcpu *vcpu)
{
	if (vcpu->arch.vcb.vpcr == 0) {
		vcpu->arch.vcb.vpcr = get_vpcr(vcpu, 0);
		if (unlikely(bind_vcpu_enabled)) {
			int nid;
			unsigned long end;

			end = vcpu->kvm->arch.host_phys_addr + vcpu->kvm->arch.size;
			nid = pfn_to_nid(PHYS_PFN(vcpu->kvm->arch.host_phys_addr));
			if (pfn_to_nid(PHYS_PFN(end)) == nid)
				set_cpus_allowed_ptr(vcpu->arch.tsk, cpumask_of_node(nid));
		}
		vcpu->arch.vcb.upcr = 0x7;
	}
}

void kvm_flush_tlb_all(void)
{
	tbia();
}

void kvm_sw64_update_vpn(struct kvm_vcpu *vcpu, unsigned long vpn)
{
	vcpu->arch.vcb.vpcr = ((vcpu->arch.vcb.vpcr) & (~(VPN_MASK << 44))) | (vpn << 44);
	vcpu->arch.vcb.dtb_vpcr = ((vcpu->arch.vcb.dtb_vpcr) & (~(VPN_MASK << VPN_SHIFT))) | (vpn << VPN_SHIFT);
}

int kvm_sw64_init_vm(struct kvm *kvm)
{
	return 0;
}

void kvm_sw64_destroy_vm(struct kvm *kvm)
{
	kvm_destroy_vcpus(kvm);
}

int kvm_arch_prepare_memory_region(struct kvm *kvm,
				const struct kvm_memory_slot *old,
				struct kvm_memory_slot *new,
				enum kvm_mr_change change)
{
	unsigned long addr;
	struct file *vm_file;
	struct vm_area_struct *vma;
	struct vmem_info *info;
	struct kvm_userspace_memory_region new_mem;
	struct kvm_userspace_memory_region *mem = &new_mem;
	unsigned long ret;
	size_t size;

	if (change == KVM_MR_FLAGS_ONLY || change == KVM_MR_DELETE)
		return 0;

	mem->guest_phys_addr = ((new->base_gfn) << PAGE_SHIFT);
	mem->memory_size = ((new->npages) << PAGE_SHIFT);
	mem->userspace_addr = new->userspace_addr;

	if (test_bit(IO_MARK_BIT, (unsigned long *)(&(mem->guest_phys_addr))))
		return 0;

	if (test_bit(IO_MARK_BIT + 1, (unsigned long *)(&(mem->guest_phys_addr))))
		return 0;

	if (!sw64_kvm_pool)
		return -ENOMEM;

	pr_info("%s: %#llx %#llx, user addr: %#llx\n", __func__,
			mem->guest_phys_addr, mem->memory_size, mem->userspace_addr);

	vma = find_vma(current->mm, mem->userspace_addr);
	if (!vma)
		return -ENOMEM;
	vm_file = vma->vm_file;

	if (!vm_file) {
		info = kzalloc(sizeof(struct vmem_info), GFP_KERNEL);

		size = round_up(mem->memory_size, 8<<20);
		addr = gen_pool_alloc(sw64_kvm_pool, size);
		if (!addr)
			return -ENOMEM;
		vm_munmap(mem->userspace_addr, mem->memory_size);
		ret = vm_mmap(vm_file, mem->userspace_addr, mem->memory_size,
				PROT_READ | PROT_WRITE,
				MAP_SHARED | MAP_FIXED, 0);
		if ((long)ret < 0)
			return ret;

		vma = find_vma(current->mm, mem->userspace_addr);
		if (!vma)
			return -ENOMEM;

		info->start = addr;
		info->size = size;
		vma->vm_private_data = (void *) info;

		vma->vm_ops = &vmem_vm_ops;
		vma->vm_ops->open(vma);

		ret = vmem_vm_insert_page(vma);
		if ((int)ret < 0)
			return ret;
	} else {
		info = vm_file->private_data;
		addr = info->start;
	}

	pr_info("guest phys addr = %#lx, size = %#lx\n",
			addr, vma->vm_end - vma->vm_start);

	kvm->arch.host_phys_addr = (u64)addr;
	kvm->arch.size = round_up(mem->memory_size, 8<<20);

	memset(__va(addr), 0, 0x2000000);

	return 0;
}

/*
 * kvm_mark_migration write the mark on every vcpucbs of the kvm, which tells
 * the system to do migration while the mark is on, and flush all vcpu's tlbs
 * at the beginning of the migration.
 */
void kvm_mark_migration(struct kvm *kvm, int mark)
{
	struct kvm_vcpu *vcpu;
	unsigned long cpu;

	kvm_for_each_vcpu(cpu, vcpu, kvm)
		vcpu->arch.vcb.migration_mark = mark << 2;

	kvm_flush_remote_tlbs(kvm);
}

void kvm_arch_commit_memory_region(struct kvm *kvm,
		struct kvm_memory_slot *old,
		const struct kvm_memory_slot *new,
		enum kvm_mr_change change)
{
	/*
	 * At this point memslot has been committed and there is an
	 * allocated dirty_bitmap[], dirty pages will be tracked while the
	 * memory slot is write protected.
	 */

	/* If dirty logging has been stopped, clear migration_mark for now. */
	if ((change == KVM_MR_FLAGS_ONLY) && (old->flags & KVM_MEM_LOG_DIRTY_PAGES)
		&& (!(new->flags & KVM_MEM_LOG_DIRTY_PAGES))) {
		kvm_mark_migration(kvm, 0);
		return;
	}

	/* If it's the first time dirty logging, flush all vcpu tlbs. */
	if ((change == KVM_MR_FLAGS_ONLY) && (!(old->flags & KVM_MEM_LOG_DIRTY_PAGES))
		&& (new->flags & KVM_MEM_LOG_DIRTY_PAGES))
		kvm_mark_migration(kvm, 1);
}

int kvm_sw64_vcpu_reset(struct kvm_vcpu *vcpu)
{
	unsigned long addr = vcpu->kvm->arch.host_phys_addr;

	hrtimer_cancel(&vcpu->arch.hrt);
	vcpu->arch.vcb.soft_cid = vcpu->vcpu_id;
	vcpu->arch.vcb.vcpu_irq_disabled = 1;
	vcpu->arch.pcpu_id = -1; /* force flush tlb for the first time */
	vcpu->arch.power_off = 0;
	memset(&vcpu->arch.irqs_pending, 0, sizeof(vcpu->arch.irqs_pending));

	if (vcpu->vcpu_id == 0)
		memset(__va(addr), 0, 0x2000000);

	return 0;
}

long kvm_sw64_get_vcb(struct file *filp, unsigned long arg)
{
	void __iomem *intpu_base = misc_platform_get_intpu_base(0);
	struct kvm_vcpu *vcpu = filp->private_data;

	if (vcpu->arch.vcb.migration_mark) {
		unsigned long result = readq(intpu_base + OFFSET_LONG_TIME)
			+ vcpu->arch.vcb.guest_longtime_offset;
		vcpu->arch.vcb.guest_longtime = result;
		vcpu->arch.vcb.guest_irqs_pending = vcpu->arch.irqs_pending[0];
	}

	if (copy_to_user((void __user *)arg, &(vcpu->arch.vcb), sizeof(struct vcpucb)))
		return -EINVAL;

	return 0;
}

long kvm_sw64_set_vcb(struct file *filp, unsigned long arg)
{
	unsigned long result;
	struct kvm_vcpu *vcpu = filp->private_data;
	struct vcpucb *kvm_vcb;
	void __iomem *intpu_base = misc_platform_get_intpu_base(0);

	kvm_vcb = memdup_user((void __user *)arg, sizeof(*kvm_vcb));
	memcpy(&(vcpu->arch.vcb), kvm_vcb, sizeof(struct vcpucb));

	if (vcpu->arch.vcb.migration_mark) {
		/* updated vpcr needed by destination vm */
		vcpu->arch.vcb.vpcr = get_vpcr(vcpu, 0);
		/* synchronize the longtime of source and destination */
		if (vcpu->arch.vcb.soft_cid == 0) {
			result = readq(intpu_base + OFFSET_LONG_TIME);
			vcpu->arch.vcb.guest_longtime_offset = vcpu->arch.vcb.guest_longtime - result;
			longtime_offset = vcpu->arch.vcb.guest_longtime_offset;
		} else
			vcpu->arch.vcb.guest_longtime_offset = longtime_offset;

		set_timer(vcpu, 200000000);
		vcpu->arch.vcb.migration_mark = 0;
	}

	return 0;
}

void kvm_mmu_free_memory_caches(struct kvm_vcpu *vcpu)
{
}

void kvm_arch_mmu_enable_log_dirty_pt_masked(struct kvm *kvm,
		struct kvm_memory_slot *slot, gfn_t gfn_offset,
		unsigned long mask)
{
}

void kvm_arch_flush_shadow_memslot(struct kvm *kvm,
				   struct kvm_memory_slot *slot)
{
}

void kvm_arch_flush_shadow_all(struct kvm *kvm)
{
}

void update_aptp(unsigned long pgd)
{
}

static int __init kvm_core3_init(void)
{
	int i, ret;

	bind_vcpu_init();

	ret = vmem_init();
	if (unlikely(ret))
		goto out;

	for (i = 0; i < NR_CPUS; i++)
		last_vpn(i) = VPN_FIRST_VERSION;

	ret = kvm_init(sizeof(struct kvm_vcpu), 0, THIS_MODULE);

	if (likely(!ret))
		return 0;

	vmem_exit();
out:
	bind_vcpu_exit();
	return ret;
}

static void __exit kvm_core3_exit(void)
{
	kvm_exit();
	vmem_exit();
	bind_vcpu_exit();
}

module_init(kvm_core3_init);
module_exit(kvm_core3_exit);

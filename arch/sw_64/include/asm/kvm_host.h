/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_KVM_HOST_H
#define _ASM_SW64_KVM_HOST_H

#include <linux/types.h>
#include <linux/hardirq.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/bug.h>
#include <linux/mm.h>
#include <linux/mmu_notifier.h>
#include <linux/preempt.h>
#include <linux/msi.h>
#include <linux/slab.h>
#include <linux/rcupdate.h>
#include <linux/ratelimit.h>
#include <linux/err.h>
#include <linux/bitmap.h>
#include <linux/compiler.h>
#include <asm/signal.h>
#include <asm/vcpu.h>

#include <generated/autoconf.h>
#include <asm/ptrace.h>

#include <asm/kvm_mmio.h>
#include <asm/cpu.h>

#define last_vpn(cpu)	(cpu_data[cpu].last_vpn)

#ifdef CONFIG_SUBARCH_C3B
#define VPN_BITS	8
#define GUEST_RESET_PC          0xffffffff80011100
#define KVM_MAX_VCPUS   64
#endif

#ifdef CONFIG_SUBARCH_C4
#define VPN_BITS	10
#define GUEST_RESET_PC          0xfff0000000011002
#define KVM_MAX_VCPUS   256
#endif

#define VPN_FIRST_VERSION	(1UL << VPN_BITS)
#define VPN_MASK		((1UL << VPN_BITS) - 1)
#define VPN_SHIFT		(64 - VPN_BITS)

#define KVM_INTERNAL_MEM_SLOTS	(KVM_MEM_SLOTS_NUM - 512)

#define KVM_HALT_POLL_NS_DEFAULT 0
#define KVM_IRQCHIP_NUM_PINS     256
/* KVM Hugepage definitions for sw64 */
#define KVM_NR_PAGE_SIZES   3
#define KVM_HPAGE_GFN_SHIFT(x)  (((x) - 1) * 9)
#define KVM_HPAGE_SHIFT(x)  (PAGE_SHIFT + KVM_HPAGE_GFN_SHIFT(x))
#define KVM_HPAGE_SIZE(x)   (1UL << KVM_HPAGE_SHIFT(x))
#define KVM_HPAGE_MASK(x)   (~(KVM_HPAGE_SIZE(x) - 1))
#define KVM_PAGES_PER_HPAGE(x)  (KVM_HPAGE_SIZE(x) / PAGE_SIZE)

/*
 * The architecture supports 48-bit GPA as input to the addtional stage translations.
 */
#define KVM_PHYS_SHIFT	(48)
#define KVM_PHYS_SIZE	(_AC(1, ULL) << KVM_PHYS_SHIFT)
#define KVM_PHYS_MASK	(KVM_PHYS_SIZE - _AC(1, ULL))

struct kvm_arch_memory_slot {
};

struct kvm_arch {
	unsigned long host_phys_addr;
	unsigned long size;

	struct swvm_mem mem;
	/* Addtional stage page table*/
	pgd_t *pgd;
};

struct kvm_vcpu_arch {
	struct kvm_regs regs __aligned(32);
	struct vcpucb vcb;
	struct task_struct *tsk;
	unsigned int pcpu_id; /* current running pcpu id */

	/* Virtual clock device */
	struct hrtimer hrt;
	unsigned long timer_next_event;
	unsigned long vtimer_freq;

	int first_run;
	int halted;
	int stopped;
	int restart;

	/* Pending virtual interrupts */
	DECLARE_BITMAP(irqs_pending, SWVM_IRQS);
	unsigned long vpnc[NR_CPUS];

	/* Detect first run of a vcpu */
	bool has_run_once;

	/* WAIT executed */
	int wait;

	/* vcpu power-off state */
	bool power_off;

	/* Don't run the guest (internal implementation need) */
	bool pause;

	struct kvm_decode mmio_decode;

	/* Cache some mmu pages needed inside spinlock regions */
	struct kvm_mmu_memory_cache mmu_page_cache;
};

struct vmem_info {
	unsigned long start;
	size_t size;
	atomic_t refcnt;
};

struct kvm_vm_stat {
	struct kvm_vm_stat_generic generic;
};

struct kvm_vcpu_stat {
	struct kvm_vcpu_stat_generic generic;
	u64 pid;
	u64 exits;
	u64 io_exits;
	u64 mmio_exits;
	u64 migration_set_dirty;
	u64 shutdown_exits;
	u64 restart_exits;
	u64 stop_exits;
	u64 ipi_exits;
	u64 timer_exits;
	u64 debug_exits;
	u64 fatal_error_exits;
	u64 halt_exits;
	u64 halt_successful_poll;
	u64 halt_attempted_poll;
	u64 halt_wakeup;
	u64 halt_poll_success_ns;
	u64 halt_poll_fail_ns;
	u64 halt_poll_invalid;
	u64 signal_exits;
	u64 steal;
	u64 st_max;
	u64 utime;
	u64 stime;
	u64 gtime;
};

#ifdef CONFIG_SUBARCH_C4
#define KVM_ARCH_WANT_MMU_NOTIFIER
#endif

void update_vcpu_stat_time(struct kvm_vcpu_stat *vcpu_stat);
void sw64_kvm_switch_vpn(struct kvm_vcpu *vcpu);
int vmem_init(void);
void vmem_exit(void);
int __sw64_vcpu_run(unsigned long vcb_pa, struct kvm_regs *regs,
		struct hcall_args *args);
int handle_exit(struct kvm_vcpu *vcpu, struct kvm_run *run,
		int exception_index, struct hcall_args *hargs);
void vcpu_send_ipi(struct kvm_vcpu *vcpu, int target_vcpuid, int type);
static inline void kvm_arch_hardware_disable(void) {}
static inline void kvm_arch_sync_events(struct kvm *kvm) {}
static inline void kvm_arch_vcpu_uninit(struct kvm_vcpu *vcpu) {}
static inline void kvm_arch_sched_in(struct kvm_vcpu *vcpu, int cpu) {}
static inline void kvm_arch_free_memslot(struct kvm *kvm,
		struct kvm_memory_slot *slot) {}
static inline void kvm_arch_memslots_updated(struct kvm *kvm, u64 gen) {}
static inline void kvm_arch_vcpu_blocking(struct kvm_vcpu *vcpu) {}
static inline void kvm_arch_vcpu_unblocking(struct kvm_vcpu *vcpu) {}
static inline void kvm_arch_vcpu_block_finish(struct kvm_vcpu *vcpu) {}

void kvm_arch_vcpu_free(struct kvm_vcpu *vcpu);

int kvm_sw64_perf_init(void);
int kvm_sw64_perf_teardown(void);
void kvm_flush_tlb_all(void);
void kvm_sw64_update_vpn(struct kvm_vcpu *vcpu, unsigned long vpn);
int kvm_sw64_init_vm(struct kvm *kvm);
void kvm_sw64_destroy_vm(struct kvm *kvm);
int kvm_sw64_vcpu_reset(struct kvm_vcpu *vcpu);
long kvm_sw64_set_vcb(struct file *filp, unsigned long arg);
long kvm_sw64_get_vcb(struct file *filp, unsigned long arg);

void update_aptp(unsigned long pgd);
void vcpu_set_numa_affinity(struct kvm_vcpu *vcpu);
#endif /* _ASM_SW64_KVM_HOST_H */

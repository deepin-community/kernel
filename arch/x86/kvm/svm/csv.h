/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * CSV driver for KVM
 *
 * HYGON CSV support
 *
 * Copyright (C) Hygon Info Technologies Ltd.
 */

#ifndef __SVM_CSV_H
#define __SVM_CSV_H

#include <asm/processor-hygon.h>

/* same to the ring buffer max num */
#define SVM_RING_BUFFER_MAX 4094

struct csv_ringbuf_info_item {
	struct page **pages;
	uintptr_t hdr_vaddr;
	uintptr_t trans_vaddr;
	uintptr_t data_vaddr;
	uintptr_t trans_uaddr;
	uintptr_t hdr_uaddr;
	unsigned long trans_len;
	unsigned long hdr_len;
	unsigned long n;
};

struct csv_ringbuf_infos {
	struct csv_ringbuf_info_item *item[SVM_RING_BUFFER_MAX];
	int num;
};

#ifdef CONFIG_KVM_SUPPORTS_CSV_REUSE_ASID

#define ASID_USERID_LENGTH 20

struct csv_asid_userid {
	int refcnt; // reference count of the ASID
	u32 userid_len;
	char userid[ASID_USERID_LENGTH];
};
extern struct csv_asid_userid *csv_asid_userid_array;

#endif	/* CONFIG_KVM_SUPPORTS_CSV_REUSE_ASID */

#ifdef CONFIG_HYGON_CSV

/*
 * Hooks table: a table of function and variable pointers filled in
 * when module init.
 */
extern struct hygon_kvm_hooks_table {
	bool sev_hooks_installed;
	bool *sev_enabled;
	unsigned long *sev_me_mask;
	int (*sev_issue_cmd)(struct kvm *kvm, int id, void *data, int *error);
	unsigned long (*get_num_contig_pages)(unsigned long idx,
					      struct page **inpages,
					      unsigned long npages);
	struct page **(*sev_pin_memory)(struct kvm *kvm, unsigned long uaddr,
					unsigned long ulen, unsigned long *n,
					int write);
	void (*sev_unpin_memory)(struct kvm *kvm, struct page **pages,
				 unsigned long npages);
	void (*sev_clflush_pages)(struct page *pages[], unsigned long npages);
} hygon_kvm_hooks;

void __init csv_init(struct kvm_x86_ops *ops);
void csv_exit(void);

void __init csv_hardware_setup(unsigned int max_csv_asid);
void csv_hardware_unsetup(void);

int csv_get_msr(struct kvm_vcpu *vcpu, struct msr_data *msr_info);
int csv_set_msr(struct kvm_vcpu *vcpu, struct msr_data *msr_info);
bool csv_has_emulated_ghcb_msr(struct kvm *kvm);
void csv2_sync_reset_vmsa(struct vcpu_svm *svm);
void csv2_free_reset_vmsa(struct vcpu_svm *svm);
int csv2_setup_reset_vmsa(struct vcpu_svm *svm);

static inline bool csv2_state_unstable(struct vcpu_svm *svm)
{
	return svm->sev_es.receiver_ghcb_map_fail;
}

#else	/* !CONFIG_HYGON_CSV */

static inline void __init csv_init(struct kvm_x86_ops *ops) { }
static inline void csv_exit(void) { }

static inline void __init csv_hardware_setup(unsigned int max_csv_asid) { }
static inline void csv_hardware_unsetup(void) { }

static inline
int csv_get_msr(struct kvm_vcpu *vcpu, struct msr_data *msr_info) { return 1; }
static inline
int csv_set_msr(struct kvm_vcpu *vcpu, struct msr_data *msr_info) { return 1; }
static inline bool csv_has_emulated_ghcb_msr(struct kvm *kvm) { return false; }
static inline bool csv2_state_unstable(struct vcpu_svm *svm) { return false; }
static inline void csv2_sync_reset_vmsa(struct vcpu_svm *svm) { }
static inline void csv2_free_reset_vmsa(struct vcpu_svm *svm) { }
static inline int csv2_setup_reset_vmsa(struct vcpu_svm *svm) { return 0; }

#endif	/* CONFIG_HYGON_CSV */

#include <asm/sev-common.h>

/*
 * CSV2 live migration support:
 *     If MSR_AMD64_SEV_ES_GHCB in migration didn't apply GHCB MSR protocol,
 *     reuse bits [52-63] to indicate vcpu status. The following status are
 *     currently included:
 *         * ghcb_map: indicate whether GHCB page was mapped. The mapped GHCB
 *                     page may be filled with GPRs before VMRUN, so we must
 *                     remap GHCB page on the recipient's side.
 *         * received_first_sipi: indicate AP's INIT-SIPI-SIPI stage. Reuse
 *                     these bits for received_first_sipi is acceptable cause
 *                     runtime stage of guest's linux only applies GHCB page
 *                     protocol.
 *                     It's unlikely that the migration encounter other stages
 *                     of guest's linux. Once encountered, AP bringup may fail
 *                     which will not impact user payload.
 *     Otherbits keep their's original meaning. (See GHCB Spec 2.3.1 for detail)
 */
#define GHCB_MSR_KVM_STATUS_POS		52
#define GHCB_MSR_KVM_STATUS_BITS	12
#define GHCB_MSR_KVM_STATUS_MASK				\
	((BIT_ULL(GHCB_MSR_KVM_STATUS_BITS) - 1)		\
			<< GHCB_MSR_KVM_STATUS_POS)
#define GHCB_MSR_MAPPED_POS		63
#define GHCB_MSR_MAPPED_BITS		1
#define GHCB_MSR_MAPPED_MASK					\
	((BIT_ULL(GHCB_MSR_MAPPED_BITS) - 1)			\
			 << GHCB_MSR_MAPPED_POS)
#define GHCB_MSR_RECEIVED_FIRST_SIPI_POS	62
#define GHCB_MSR_RECEIVED_FIRST_SIPI_BITS	1
#define GHCB_MSR_RECEIVED_FIRST_SIPI_MASK			\
	((BIT_ULL(GHCB_MSR_RECEIVED_FIRST_SIPI_BITS) - 1)	\
			 << GHCB_MSR_RECEIVED_FIRST_SIPI_POS)

#endif	/* __SVM_CSV_H */

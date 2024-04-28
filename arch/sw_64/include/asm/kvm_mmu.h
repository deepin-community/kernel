/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_KVM_MMU_H
#define _ASM_SW64_KVM_MMU_H

#define AF_ACCESS_TYPE_SHIFT		55
#define AF_INV_LEVEL_SHIFT		53
#define AF_FAULT_STATUS_SHIFT		48

#define AF_ACCESS_TYPE_MASK		0x3
#define AF_INV_LEVEL_MASK		0x3
#define AF_FAULT_STATUS_MASK		0x1f
#define AF_ENTRY_ADDR_MASK		((0x1UL << AF_FAULT_STATUS_SHIFT) - 1)

/* access type defination */
#define AF_READ_ACCESS_TYPE		0x1
#define AF_WRITE_ACCESS_TYPE		0x2
#define AF_EXEC_ACCESS_TYPE		0x3

/* invalid page level */
#define AF_INV_LEVEL_1			0
#define AF_INV_LEVEL_2			1
#define AF_INV_LEVEL_3			2
#define AF_INV_LEVEL_4			3

/* fault status */
#define AF_STATUS_MISCONFIG		0x1
#define AF_STATUS_FOR			0x2
#define AF_STATUS_FOW			0x4
#define AF_STATUS_FOE			0x8
#define AF_STATUS_INV			0x10

#define KVM_MMU_CACHE_MIN_PAGES		3

static inline void kvm_set_aptpte_readonly(pte_t *pte)
{
	pte_val(*pte) |= _PAGE_FOW;
}

static inline bool kvm_aptpte_readonly(pte_t *pte)
{
	return (pte_val(*pte) & _PAGE_FOW) == _PAGE_FOW;
}

static inline void kvm_set_aptpmd_readonly(pmd_t *pmd)
{
	pmd_val(*pmd) |= _PAGE_FOW;
}

static inline bool kvm_aptpmd_readonly(pmd_t *pmd)
{
	return (pmd_val(*pmd) & _PAGE_FOW) == _PAGE_FOW;
}

static inline void kvm_set_aptpud_readonly(pud_t *pud)
{
	pud_val(*pud) |= _PAGE_FOW;
}

static inline bool kvm_aptpud_readonly(pud_t *pud)
{
	return (pud_val(*pud) & _PAGE_FOW) == _PAGE_FOW;
}

static inline pte_t kvm_pte_mkwrite(pte_t pte)
{
	pte_val(pte) &= ~_PAGE_FOW;
	return pte;
}

static inline pte_t kvm_pte_mkexec(pte_t pte)
{
	pte_val(pte) &= ~_PAGE_FOE;
	return pte;
}

static inline bool kvm_pte_exec(pte_t *pte)
{
	return !(pte_val(*pte) & _PAGE_FOE);
}

static inline pmd_t kvm_pmd_mkwrite(pmd_t pmd)
{
	pmd_val(pmd) &= ~_PAGE_FOW;
	return pmd;
}

static inline pmd_t kvm_pmd_mkexec(pmd_t pmd)
{
	pmd_val(pmd) &= ~_PAGE_FOE;
	return pmd;
}

static inline bool kvm_pmd_exec(pmd_t *pmd)
{
	return !(pmd_val(*pmd) & _PAGE_FOE);
}

static inline pud_t kvm_pud_mkwrite(pud_t pud)
{
	pud_val(pud) &= ~_PAGE_FOW;
	return pud;
}

static inline pud_t kvm_pud_mkexec(pud_t pud)
{
	pud_val(pud) &= ~_PAGE_FOE;
	return pud;
}

static inline bool kvm_pud_exec(pud_t *pud)
{
	return !(pud_val(*pud) & _PAGE_FOE);
}

void kvm_core4_commit_memory_region(struct kvm *kvm,
		const struct kvm_userspace_memory_region *mem,
		const struct kvm_memory_slot *old,
		const struct kvm_memory_slot *new,
		enum kvm_mr_change change);
void kvm_core4_flush_shadow_memslot(struct kvm *kvm,
		struct kvm_memory_slot *slot);
void kvm_core4_flush_shadow_all(struct kvm *kvm);
void kvm_mmu_free_memory_caches(struct kvm_vcpu *vcpu);
int kvm_age_hva(struct kvm *kvm, unsigned long start, unsigned long end);
void kvm_handle_apt_fault(struct kvm_vcpu *vcpu);
int kvm_alloc_addtional_stage_pgd(struct kvm *kvm);
void kvm_arch_flush_shadow_memslot(struct kvm *kvm,
		struct kvm_memory_slot *slot);
int kvm_handle_guest_abort(struct kvm_vcpu *vcpu, struct kvm_run *run,
			   struct hcall_args *hargs);
void apt_unmap_vm(struct kvm *kvm);
#endif /* _ASM_SW64_KVM_MMU_H */

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 - os kernal
 * Author: lff <lff@example.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/cpu.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/kvm_host.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mman.h>
#include <linux/sched/signal.h>
#include <linux/freezer.h>
#include <linux/smp.h>
#include <linux/kvm.h>
#include <linux/uaccess.h>
#include <linux/hugetlb.h>

#include <asm/kvm_asm.h>
#include <asm/sw64io.h>

#include <asm/kvm_timer.h>
#include <asm/kvm_host.h>
#include <asm/kvm_emulate.h>
#include <asm/kvm_mmu.h>

#define KVM_APT_FLAG_LOGGING_ACTIVE	(1UL << 1)

static bool memslot_is_logging(struct kvm_memory_slot *memslot)
{
	return memslot->dirty_bitmap && !(memslot->flags & KVM_MEM_READONLY);
}

/*
 * Return values of kvm_handle_mmio_page_fault and mmu.page_fault:
 * RET_AF_RETRY: let CPU fault again on the address.
 * RET_AF_EMULATE: mmio page fault, emulate the instruction directly.
 *
 * For kvm_handle_mmio_page_fault only:
 * RET_AF_INVALID: the spte is invalid, let the real page fault path update it.
 */
enum {
	RET_AF_RETRY = 0,
	RET_AF_EMULATE = 1,
	RET_AF_INVALID = 2,
};

/**
 * apt_dissolve_pmd() - clear and flush huge PMD entry
 * @kvm:	pointer to kvm structure.
 * @addr:	IPA
 * @pmd:	pmd pointer for IPA
 *
 * Function clears a PMD entry, flushes TLBs.
 */
static void apt_dissolve_pmd(struct kvm *kvm, phys_addr_t addr, pmd_t *pmd)
{
	int i;

	if (!pmd_trans_huge(*pmd))
		return;

	if (pmd_trans_cont(*pmd)) {
		for (i = 0; i < CONT_PMDS; i++, pmd++)
			pmd_clear(pmd);
	} else
		pmd_clear(pmd);

	kvm_flush_remote_tlbs(kvm);
	put_page(virt_to_page(pmd));
}

/**
 * apt_dissolve_pud() - clear and flush huge PUD entry
 * @kvm:	pointer to kvm structure.
 * @addr:	IPA
 * @pud:	pud pointer for IPA
 *
 * Function clears a PUD entry, flushes TLBs.
 */
static void apt_dissolve_pud(struct kvm *kvm, phys_addr_t addr, pud_t *pudp)
{
	if (!pud_huge(*pudp))
		return;

	pud_clear(pudp);
	kvm_flush_remote_tlbs(kvm);
	put_page(virt_to_page(pudp));
}

static int mmu_topup_memory_cache(struct kvm_mmu_memory_cache *cache,
		int min, int max)
{
	void *page;

	BUG_ON(max > KVM_NR_MEM_OBJS);
	if (cache->nobjs >= min)
		return 0;
	while (cache->nobjs < max) {
		page = (void *)__get_free_page(GFP_KERNEL | __GFP_ZERO);
		if (!page)
			return -ENOMEM;
		cache->objects[cache->nobjs++] = page;
	}
	return 0;
}

static void mmu_free_memory_cache(struct kvm_mmu_memory_cache *mc)
{
	while (mc->nobjs)
		free_page((unsigned long)mc->objects[--mc->nobjs]);
}

void kvm_mmu_free_memory_caches(struct kvm_vcpu *vcpu)
{
	mmu_free_memory_cache(&vcpu->arch.mmu_page_cache);
}

static void *mmu_memory_cache_alloc(struct kvm_mmu_memory_cache *mc)
{
	void *p;

	BUG_ON(!mc || !mc->nobjs);
	p = mc->objects[--mc->nobjs];
	return p;
}

static void unmap_apt_ptes(struct kvm *kvm, pmd_t *pmd,
		phys_addr_t addr, phys_addr_t end)
{
	pte_t *pte, *start_pte;
	struct page *ptr_page;

	start_pte = pte = pte_offset_kernel(pmd, addr);
	do {
		if (!pte_none(*pte)) {
			/* Do we need WRITE_ONCE(pte, 0)? */
			set_pte(pte, __pte(0));
			put_page(virt_to_page(pte));
		}
	} while (pte++, addr += PAGE_SIZE, addr != end);

	ptr_page = virt_to_page(start_pte);
	if (page_count(ptr_page) == 1) {
		pte_t *pte_table = pte_offset_kernel(pmd, 0);

		pmd_clear(pmd);
		free_page((unsigned long)pte_table);
		put_page(virt_to_page(pmd));
	}
}

static void unmap_apt_pmds(struct kvm *kvm, pud_t *pud,
		phys_addr_t addr, phys_addr_t end)
{
	phys_addr_t next;
	pmd_t *pmd, *start_pmd;
	struct page *ptr_page;
	int i;

	start_pmd = pmd = pmd_offset(pud, addr);
	do {
		next = pmd_addr_end(addr, end);
		if (!pmd_none(*pmd)) {
			if (pmd_trans_huge(*pmd)) {
				if (pmd_trans_cont(*pmd)) {
					for (i = 0; i < CONT_PMDS; i++, pmd++)
						pmd_clear(pmd);
				} else
					pmd_clear(pmd);
				/* Do we need flush tlb???? edited by lff */
				kvm_flush_remote_tlbs(kvm);
				put_page(virt_to_page(pmd));
			} else {
				unmap_apt_ptes(kvm, pmd, addr, next);
			}
		}
	} while (pmd++, addr = next, addr != end);

	ptr_page = virt_to_page(start_pmd);
	if (page_count(ptr_page) == 1) {
		pmd_t *pmd_table __maybe_unused = pmd_offset(pud, 0UL);

		pud_clear(pud);
		free_page((unsigned long)pmd_table);
		put_page(virt_to_page(pud));
	}
}

static void unmap_apt_puds(struct kvm *kvm, p4d_t *p4d,
		phys_addr_t addr, phys_addr_t end)
{
	phys_addr_t next;
	pud_t *pud, *start_pud;
	struct page *ptr_page;

	start_pud = pud = pud_offset(p4d, addr);
	do {
		next = pud_addr_end(addr, end);
		if (!pud_none(*pud)) {
			if (pud_huge(*pud)) {
				pud_clear(pud);
				/* Do we need flush tlb???? edited by lff */
				kvm_flush_remote_tlbs(kvm);
				put_page(virt_to_page(pud));
			} else {
				unmap_apt_pmds(kvm, pud, addr, next);
			}
		}
	} while (pud++, addr = next, addr != end);

	ptr_page = virt_to_page(start_pud);
	if (page_count(ptr_page) == 1) {
		pud_t *pud_table __maybe_unused = pud_offset(p4d, 0UL);

		p4d_clear(p4d);
		kvm_flush_remote_tlbs(kvm);
		free_page((unsigned long)pud_table);
		put_page(virt_to_page(p4d));
	}
}

/**
 * unmap_apt_range -- Clear addtional page table entries to unmap a range
 * @kvm:	The VM pointer
 * @start:	The intermediate physical base address of the range to unmap
 * @size:	The size of the area to unmap
 *
 * Clear a range of apt mappings, lowering the various ref-counts. Must
 * be called while holding mmu_lock (unless for freeing the apt pgd before
 * destroying the VM), otherwise another faulting VCPU may come in and mess
 * with things behind our backs.
 */
static void unmap_apt_range(struct kvm *kvm, phys_addr_t start, u64 size)
{
	pgd_t *pgd;
	p4d_t *p4d;
	phys_addr_t addr = start, end = start + size;
	phys_addr_t next;

	assert_spin_locked(&kvm->mmu_lock);
	WARN_ON(size & ~PAGE_MASK);

	pgd = kvm->arch.pgd + pgd_index(addr);
	p4d = p4d_offset(pgd, addr);
	do {
		/*
		 * Make sure the page table is still active, as another thread
		 * could have possibly freed the page table, while we released
		 * the lock.
		 */
		if (!READ_ONCE(kvm->arch.pgd))
			break;
		next = p4d_addr_end(addr, end);
		if (!p4d_none(*p4d))
			unmap_apt_puds(kvm, p4d, addr, next);
		/*
		 * If the range is too large, release the kvm->mmu_lock
		 * to prevent starvation and lockup detector warnings.
		 */
		if (next != end)
			cond_resched_lock(&kvm->mmu_lock);
	} while (pgd++, addr = next, addr != end);
}

static void apt_unmap_memslot(struct kvm *kvm,
		struct kvm_memory_slot *memslot)
{
	hva_t hva = memslot->userspace_addr;
	phys_addr_t addr = memslot->base_gfn << PAGE_SHIFT;
	phys_addr_t size = PAGE_SIZE * memslot->npages;
	hva_t reg_end = hva + size;

	/*
	 * A memory region could potentially cover multiple VMAs, and any holes
	 * between them, so iterate over all of them to find out if we should
	 * unmap any of them.
	 *
	 *     +--------------------------------------------+
	 * +---------------+----------------+   +----------------+
	 * |   : VMA 1     |      VMA 2     |   |    VMA 3  :    |
	 * +---------------+----------------+   +----------------+
	 *     |               memory region                |
	 *     +--------------------------------------------+
	 */
	do {
		struct vm_area_struct *vma = find_vma(current->mm, hva);
		hva_t vm_start, vm_end;

		if (!vma || vma->vm_start >= reg_end)
			break;

		/*
		 * Take the intersection of this VMA with the memory region
		 */
		vm_start = max(hva, vma->vm_start);
		vm_end = min(reg_end, vma->vm_end);

		if (!(vma->vm_flags & VM_PFNMAP)) {
			gpa_t gpa = addr + (vm_start - memslot->userspace_addr);

			unmap_apt_range(kvm, gpa, vm_end - vm_start);
		}
		hva = vm_end;
	} while (hva < reg_end);
}

/**
 * apt_unmap_vm - Unmap Additional Stage RAM mappings
 * @kvm: The struct kvm pointer
 *
 * Go through the memregions and unmap any reguler RAM
 * backing memory already mapped to the VM.
 */
void apt_unmap_vm(struct kvm *kvm)
{
	struct kvm_memslots *slots;
	struct kvm_memory_slot *memslot;
	int idx;

	idx = srcu_read_lock(&kvm->srcu);
	down_read(&current->mm->mmap_lock);
	spin_lock(&kvm->mmu_lock);

	slots = kvm_memslots(kvm);
	kvm_for_each_memslot(memslot, slots)
		apt_unmap_memslot(kvm, memslot);
	spin_unlock(&kvm->mmu_lock);
	up_read(&current->mm->mmap_lock);
	srcu_read_unlock(&kvm->srcu, idx);
}

static pud_t *apt_get_pud(pgd_t *pgd, struct kvm_mmu_memory_cache *cache,
		phys_addr_t addr)
{
	p4d_t *p4d;
	pud_t *pud;

	pgd += pgd_index(addr);
	if (pgd_none(*pgd)) {
		/* Not used on SW64 yet */
		VM_BUG_ON(pgd);
		return NULL;
	}
	p4d = p4d_offset(pgd, addr);
	if (p4d_none(*p4d)) {
		if (!cache)
			return NULL;
		pud = mmu_memory_cache_alloc(cache);
		p4d_populate(NULL, p4d, pud);
		get_page(virt_to_page(p4d));
	}
	return pud_offset(p4d, addr);
}

static pmd_t *apt_get_pmd(struct kvm *kvm, struct kvm_mmu_memory_cache *cache,
		phys_addr_t addr, unsigned long sz)
{
	pud_t *pud;
	pmd_t *pmd;

	pud = apt_get_pud(kvm->arch.pgd, cache, addr);
	if (!pud || pud_huge(*pud))
		return NULL;

	if (pud_none(*pud)) {
		if (!cache)
			return NULL;
		pmd = mmu_memory_cache_alloc(cache);
		pud_populate(NULL, pud, pmd);
		get_page(virt_to_page(pud));
	}
	if (sz == CONT_PMD_SIZE)
		addr &= CONT_PMD_MASK;

	return pmd_offset(pud, addr);
}

static bool kvm_is_write_fault(unsigned long access_type)
{
	if (access_type == AF_WRITE_ACCESS_TYPE)
		return true;

	return false;
}

static bool kvm_is_exec_fault(unsigned long access_type)
{
	if (access_type == AF_EXEC_ACCESS_TYPE)
		return true;

	return false;
}
/**
 * apt_wp_ptes - write protect PMD range
 * @pmd:	pointer to pmd entry
 * @addr:	range start address
 * @end:	range end address
 */
static void apt_wp_ptes(pmd_t *pmd, phys_addr_t addr, phys_addr_t end)
{
	pte_t *pte;

	pte = pte_offset_kernel(pmd, addr);
	do {
		if (!pte_none(*pte)) {
			if (!kvm_aptpte_readonly(pte))
				kvm_set_aptpte_readonly(pte);
		}
	} while (pte++, addr += PAGE_SIZE, addr != end);
}

/**
 * apt_wp_pmds - write protect PUD range
 * @pud:	pointer to pud entry
 * @addr:	range start address
 * @end:	range end address
 */
static void apt_wp_pmds(pud_t *pud, phys_addr_t addr, phys_addr_t end)
{
	pmd_t *pmd;
	phys_addr_t next;

	pmd = pmd_offset(pud, addr);

	do {
		next = pmd_addr_end(addr, end);
		if (!pmd_none(*pmd)) {
			if (pmd_trans_huge(*pmd)) {
				if (!kvm_aptpmd_readonly(pmd))
					kvm_set_aptpmd_readonly(pmd);
			} else {
				apt_wp_ptes(pmd, addr, next);
			}
		}
	} while (pmd++, addr = next, addr != end);
}

/**
 * apt_wp_puds - write protect PGD range
 * @pgd:	pointer to pgd entry
 * @addr:	range start address
 * @end:	range end address
 *
 * Process PUD entries, for a huge PUD we cause a panic.
 */
static void apt_wp_puds(p4d_t *p4d, phys_addr_t addr, phys_addr_t end)
{
	pud_t *pud;
	phys_addr_t next;

	pud = pud_offset(p4d, addr);
	do {
		next = pud_addr_end(addr, end);
		if (!pud_none(*pud)) {
			if (pud_huge(*pud)) {
				if (!kvm_aptpud_readonly(pud))
					kvm_set_aptpud_readonly(pud);
			} else {
				/* TODO:PUD not supported, revisit later if supported */
//				BUG_ON(pud_trans_huge(*pud));
				apt_wp_pmds(pud, addr, next);
			}
		}
	} while (pud++, addr = next, addr != end);
}

/**
 * apt_wp_range() - write protect apt memory region range
 * @kvm:	The KVM pointer
 * @addr:	Start address of range
 * @end:	End address of range
 */
static void apt_wp_range(struct kvm *kvm, phys_addr_t addr, phys_addr_t end)
{
	pgd_t *pgd;
	p4d_t *p4d;
	phys_addr_t next;

	pgd = kvm->arch.pgd + pgd_index(addr);
	p4d = p4d_offset(pgd, addr);

	do {
		/*
		 * Release kvm_mmu_lock periodically if the memory region is
		 * large. Otherwise, we may see kernel panics with
		 * CONFIG_DETECT_HUNG_TASK, CONFIG_LOCKUP_DETECTOR,
		 * CONFIG_LOCKDEP. Additionally, holding the lock too long
		 * will also starve other vCPUs. We have to also make sure
		 * that the page tables are not freed while we released
		 * the lock.
		 */
		cond_resched_lock(&kvm->mmu_lock);
		if (!READ_ONCE(kvm->arch.pgd))
			break;
		next = p4d_addr_end(addr, end);
		if (p4d_present(*p4d))
			apt_wp_puds(p4d, addr, next);
	} while (p4d++, addr = next, addr != end);
}

/**
 * kvm_mmu_wp_memory_region() - write protect apt entries for memory slot
 * @kvm:	The KVM pointer
 * @slot:	The memory slot to write protect
 *
 * Called to start logging dirty pages after memory region
 * KVM_MEM_LOG_DIRTY_PAGES operation is called. After this function returns
 * all present PMD and PTEs are write protected in the memory region.
 * Afterwards read of dirty page log can be called.
 *
 * Acquires kvm_mmu_lock. Called with kvm->slots_lock mutex acquired,
 * serializing operations for VM memory regions.
 */
void kvm_mmu_wp_memory_region(struct kvm *kvm, int slot)
{
	struct kvm_memslots *slots = kvm_memslots(kvm);
	struct kvm_memory_slot *memslot = id_to_memslot(slots, slot);
	phys_addr_t start = memslot->base_gfn << PAGE_SHIFT;
	phys_addr_t end = (memslot->base_gfn + memslot->npages) << PAGE_SHIFT;

	spin_lock(&kvm->mmu_lock);
	apt_wp_range(kvm, start, end);
	spin_unlock(&kvm->mmu_lock);
	kvm_flush_remote_tlbs(kvm);	// 需要通知其他vcpu进行tlb刷新，利用request机制
}

void kvm_mark_migration(struct kvm *kvm, int mark)
{
	struct kvm_vcpu *vcpu;
	unsigned long cpu;

	kvm_for_each_vcpu(cpu, vcpu, kvm)
		vcpu->arch.migration_mark = mark;
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
	if (change == KVM_MR_FLAGS_ONLY && (!(old->flags & KVM_MEM_LOG_DIRTY_PAGES) &&
				new->flags & KVM_MEM_LOG_DIRTY_PAGES)) {
		kvm_mark_migration(kvm, 1);
		kvm_mmu_wp_memory_region(kvm, new->id);
	}
	/* If dirty logging has been stopped, do nothing for now. */
	if ((change != KVM_MR_DELETE)
			&& (old->flags & KVM_MEM_LOG_DIRTY_PAGES)
			&& (!(new->flags & KVM_MEM_LOG_DIRTY_PAGES))) {
		kvm_mark_migration(kvm, 0);
		return;
	}
}

void kvm_arch_flush_shadow_memslot(struct kvm *kvm,
		struct kvm_memory_slot *slot)
{
	gpa_t gpa = slot->base_gfn << PAGE_SHIFT;
	phys_addr_t size = slot->npages << PAGE_SHIFT;

	spin_lock(&kvm->mmu_lock);
//	flush_apt_tlbs(kvm);
	unmap_apt_range(kvm, gpa, size);
	spin_unlock(&kvm->mmu_lock);
}

/**
 * kvm_alloc_addtional_stage_pgd - allocate level-1 table for addtional stage translation.
 * @kvm:	The KVM struct pointer for the VM.
 *
 * Allocates only the addtional stage HW PGD level table(s) (can support full
 * 48-bit input addresses). Clears the allocated pages.
 *
 * Note we don't need locking here as this is only called when the VM is
 * created, which can only be done once.
 */
int kvm_alloc_addtional_stage_pgd(struct kvm *kvm)
{
	pgd_t *pgd;

	if (kvm->arch.pgd != NULL) {
		kvm_err("kvm_arch already initialized?\n");
		return -EINVAL;
	}

	/* Allocate the HW PGD, making sure that each page gets its own refcount */
	pgd = alloc_pages_exact(PAGE_SIZE, GFP_KERNEL | __GFP_ZERO);
	if (!pgd)
		return -ENOMEM;

	kvm->arch.pgd = pgd;
	return 0;
}

/**
 * kvm_free_apt_pgd - free all apt tables
 * @kvm:	The KVM struct pointer for the VM.
 *
 * Walks the level-1 page table pointed to by kvm->arch.pgd and frees all
 * underlying level-2 and level-3 tables before freeing the actual level-1 table
 * and setting the struct pointer to NULL.
 */
void kvm_free_apt_pgd(struct kvm *kvm)
{
	void *pgd = NULL;

	spin_lock(&kvm->mmu_lock);
	if (kvm->arch.pgd) {
		unmap_apt_range(kvm, 0, KVM_PHYS_SIZE);
		pgd = READ_ONCE(kvm->arch.pgd);
		kvm->arch.pgd = NULL;
	}
	spin_unlock(&kvm->mmu_lock);

	/* Free the HW pgd, one page at a time */
	if (pgd)
		free_pages_exact(pgd, PAGE_SIZE);
}

void kvm_arch_flush_shadow_all(struct kvm *kvm)
{
	kvm_free_apt_pgd(kvm);
}

static void kvm_send_hwpoison_signal(unsigned long address,
		struct vm_area_struct *vma)
{
	kernel_siginfo_t info;

	clear_siginfo(&info);
	info.si_signo	= SIGBUS;
	info.si_errno	= 0;
	info.si_code	= BUS_MCEERR_AR;
	info.si_addr	= (void __user *)address;

	if (is_vm_hugetlb_page(vma))
		info.si_addr_lsb = huge_page_shift(hstate_vma(vma));
	else
		info.si_addr_lsb = PAGE_SHIFT;

	send_sig_info(SIGBUS, &info, current);
}

static bool fault_supports_apt_huge_mapping(struct kvm_memory_slot *memslot,
		unsigned long hva,
		unsigned long map_size)
{
	gpa_t gpa_start;
	hva_t uaddr_start, uaddr_end;
	size_t size;

	/* The memslot and the VMA are guaranteed to be aligned to PAGE_SIZE */
	if (map_size == PAGE_SIZE)
		return true;

	size = memslot->npages * PAGE_SIZE;

	gpa_start = memslot->base_gfn << PAGE_SHIFT;

	uaddr_start = memslot->userspace_addr;
	uaddr_end = uaddr_start + size;

	/*
	 * Pages belonging to memslots that don't have the same alignment
	 * within a PMD/PUD for userspace and IPA cannot be mapped with stage-2
	 * PMD/PUD entries, because we'll end up mapping the wrong pages.
	 *
	 * Consider a layout like the following:
	 *
	 *    memslot->userspace_addr:
	 *    +-----+--------------------+--------------------+---+
	 *    |abcde|fgh  Stage-1 block  |    Stage-1 block tv|xyz|
	 *    +-----+--------------------+--------------------+---+
	 *
	 *    memslot->base_gfn << PAGE_SHIFT:
	 *      +---+--------------------+--------------------+-----+
	 *      |abc|def  Stage-2 block  |    Stage-2 block   |tvxyz|
	 *      +---+--------------------+--------------------+-----+
	 *
	 * If we create those stage-2 blocks, we'll end up with this incorrect
	 * mapping:
	 *   d -> f
	 *   e -> g
	 *   f -> h
	 */
	if ((gpa_start & (map_size - 1)) != (uaddr_start & (map_size - 1)))
		return false;

	/*
	 * Next, let's make sure we're not trying to map anything not covered
	 * by the memslot. This means we have to prohibit block size mappings
	 * for the beginning and end of a non-block aligned and non-block sized
	 * memory slot (illustrated by the head and tail parts of the
	 * userspace view above containing pages 'abcde' and 'xyz',
	 * respectively).
	 *
	 * Note that it doesn't matter if we do the check using the
	 * userspace_addr or the base_gfn, as both are equally aligned (per
	 * the check above) and equally sized.
	 */
	return (hva & ~(map_size - 1)) >= uaddr_start &&
		(hva & ~(map_size - 1)) + map_size <= uaddr_end;
}

/*
 * apt_get_leaf_entry - walk the stage2 VM page tables and return
 * true if a valid and present leaf-entry is found. A pointer to the
 * leaf-entry is returned in the appropriate level variable - pudpp,
 * pmdpp, ptepp.
 */
static bool apt_get_leaf_entry(struct kvm *kvm, phys_addr_t addr,
		pud_t **pudpp, pmd_t **pmdpp, pte_t **ptepp)
{
	pud_t *pudp;
	pmd_t *pmdp;
	pte_t *ptep;

	*pudpp = NULL;
	*pmdpp = NULL;
	*ptepp = NULL;

	pudp = apt_get_pud(kvm->arch.pgd, NULL, addr);
	if (!pudp || pud_none(*pudp) || !pud_present(*pudp))
		return false;

	if (pud_huge(*pudp)) {
		*pudpp = pudp;
		return true;
	}

	pmdp = pmd_offset(pudp, addr);
	if (!pmdp || pmd_none(*pmdp) || !pmd_present(*pmdp))
		return false;

	if (pmd_trans_huge(*pmdp)) {
		*pmdpp = pmdp;
		return true;
	}

	ptep = pte_offset_kernel(pmdp, addr);
	if (!ptep || pte_none(*ptep) || !pte_present(*ptep))
		return false;

	*ptepp = ptep;
	return true;
}

static bool apt_is_exec(struct kvm *kvm, phys_addr_t addr)
{
	pud_t *pudp;
	pmd_t *pmdp;
	pte_t *ptep;
	bool found;

	found = apt_get_leaf_entry(kvm, addr, &pudp, &pmdp, &ptep);
	if (!found)
		return false;

	if (pudp)
		return kvm_pud_exec(pudp);
	else if (pmdp)
		return kvm_pmd_exec(pmdp);
	else
		return kvm_pte_exec(ptep);
}

static int apt_set_pte_fast(struct kvm *kvm, struct kvm_mmu_memory_cache *cache,
		phys_addr_t addr, const pte_t *new_pte,
		unsigned long flags)
{
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte, old_pte;
	bool logging_active = flags & KVM_APT_FLAG_LOGGING_ACTIVE;
	int inv_level = ((read_csr(CSR_AS_INFO)) >> AF_INV_LEVEL_SHIFT) & AF_INV_LEVEL_MASK;
	unsigned long inv_hpa = read_csr(CSR_AS_INFO) & AF_ENTRY_ADDR_MASK;

	VM_BUG_ON(logging_active && !cache);

	if (inv_level == 1) {
		pud = (pud_t *)(inv_hpa | PAGE_OFFSET);
		goto find_pud;
	} else if (inv_level == 2) {
		pmd = (pmd_t *)(inv_hpa | PAGE_OFFSET);
		goto find_pmd;
	} else if (inv_level == 3) {
		pte = (pte_t *)(inv_hpa | PAGE_OFFSET);
		goto find_pte;
	}

	/* Create addtional page table mapping - Levels 0 and 1 */
	pud = apt_get_pud(kvm->arch.pgd, cache, addr);
	if (!pud) {
		/*
		 * Ignore calls from kvm_set_spte_hva for unallocated
		 * address ranges.
		 */
		return 0;
	}

	/*
	 * While dirty page logging - dissolve huge PUD, then continue
	 * on to allocate page.
	 */
	if (logging_active)
		apt_dissolve_pud(kvm, addr, pud);

find_pud:
	if (pud_none(*pud)) {
		if (!cache)
			return 0; /* ignore calls from kvm_set_spte_hva */
		pmd = mmu_memory_cache_alloc(cache);
		pud_populate(NULL, pud, pmd);
		get_page(virt_to_page(pud));
	}

	pmd = pmd_offset(pud, addr);
	if (!pmd) {
		/*
		 * Ignore calls from kvm_set_spte_hva for unallocated
		 * address ranges.
		 */
		return 0;
	}

	/*
	 * While dirty page logging - dissolve huge PMD, then continue on to
	 * allocate page.
	 */
	if (logging_active)
		apt_dissolve_pmd(kvm, addr, pmd);

find_pmd:
	/* Create stage-2 page mappings - Level 2 */
	if (pmd_none(*pmd)) {
		if (!cache)
			return 0; /* ignore calls from kvm_set_spte_hva */
		pte = mmu_memory_cache_alloc(cache);
		pmd_populate_kernel(NULL, pmd, pte);
		get_page(virt_to_page(pmd));
	}

	pte = pte_offset_kernel(pmd, addr);

find_pte:
	/* Create 2nd stage page table mapping - Level 3 */
	old_pte = *pte;

	/* new pte should be readonly? */
//	*new_pte = pte_wrprotect(*new_pte);

	if (pte_present(old_pte)) {
		/* Skip page table update if there is no change */
		if (pte_val(old_pte) == pte_val(*new_pte))
			return 0;

		/* Do we need WRITE_ONCE(pte, 0)? */
		set_pte(pte, __pte(0));
		kvm_flush_remote_tlbs(kvm);
	} else {
		get_page(virt_to_page(pte));
	}

	/* Do we need WRITE_ONCE(pte, new_pte)? */
	set_pte(pte, *new_pte);
	return 0;
}

static int apt_set_pte(struct kvm *kvm, struct kvm_mmu_memory_cache *cache,
		phys_addr_t addr, const pte_t *new_pte,
		unsigned long flags)
{
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte, old_pte;
	bool logging_active = flags & KVM_APT_FLAG_LOGGING_ACTIVE;

	VM_BUG_ON(logging_active && !cache);

	/* Create addtional page table mapping - Levels 0 and 1 */
	pud = apt_get_pud(kvm->arch.pgd, cache, addr);
	if (!pud) {
		/*
		 * Ignore calls from kvm_set_spte_hva for unallocated
		 * address ranges.
		 */
		return 0;
	}

	/*
	 * While dirty page logging - dissolve huge PUD, then continue
	 * on to allocate page.
	 */
	if (logging_active)
		apt_dissolve_pud(kvm, addr, pud);

	if (pud_none(*pud)) {
		if (!cache)
			return 0; /* ignore calls from kvm_set_spte_hva */
		pmd = mmu_memory_cache_alloc(cache);
		pud_populate(NULL, pud, pmd);
		get_page(virt_to_page(pud));
	}

	pmd = pmd_offset(pud, addr);
	if (!pmd) {
		/*
		 * Ignore calls from kvm_set_spte_hva for unallocated
		 * address ranges.
		 */
		return 0;
	}

	/*
	 * While dirty page logging - dissolve huge PMD, then continue on to
	 * allocate page.
	 */
	if (logging_active)
		apt_dissolve_pmd(kvm, addr, pmd);

	/* Create stage-2 page mappings - Level 2 */
	if (pmd_none(*pmd)) {
		if (!cache)
			return 0; /* ignore calls from kvm_set_spte_hva */
		pte = mmu_memory_cache_alloc(cache);
		pmd_populate_kernel(NULL, pmd, pte);
		get_page(virt_to_page(pmd));
	}

	pte = pte_offset_kernel(pmd, addr);

	/* Create 2nd stage page table mapping - Level 3 */
	old_pte = *pte;

	/* new pte should be readonly? */
//	*new_pte = pte_wrprotect(*new_pte);

	if (pte_present(old_pte)) {
		/* Skip page table update if there is no change */
		if (pte_val(old_pte) == pte_val(*new_pte))
			return 0;

		/* Do we need WRITE_ONCE(pte, 0)? */
		set_pte(pte, __pte(0));
		kvm_flush_remote_tlbs(kvm);
	} else {
		get_page(virt_to_page(pte));
	}

	/* Do we need WRITE_ONCE(pte, new_pte)? */
	set_pte(pte, *new_pte);
	return 0;
}



static int apt_set_pmd_huge(struct kvm *kvm, struct kvm_mmu_memory_cache
		*cache, phys_addr_t addr, const pmd_t *new_pmd, unsigned long sz)
{
	pmd_t *pmd, old_pmd, *ori_pmd;
	int i;
retry:
	pmd = apt_get_pmd(kvm, cache, addr, sz);
	VM_BUG_ON(!pmd);
	ori_pmd = pmd;
	old_pmd = *pmd;
	if (pmd_present(old_pmd)) {
		/*
		 * If we already have PTE level mapping for this block,
		 * we must unmap it to avoid inconsistent TLB state and
		 * leaking the table page. We could end up in this situation
		 * if the memory slot was marked for dirty logging and was
		 * reverted, leaving PTE level mappings for the pages accessed
		 * during the period. So, unmap the PTE level mapping for this
		 * block and retry, as we could have released the upper level
		 * table in the process.
		 *
		 * Normal THP split/merge follows mmu_notifier callbacks and do
		 * get handled accordingly.
		 */
		if (!pmd_trans_huge(old_pmd)) {
			unmap_apt_range(kvm, addr & PMD_MASK, PMD_SIZE);
			goto retry;
		}
		/*
		 * Multiple vcpus faulting on the same PMD entry, can
		 * lead to them sequentially updating the PMD with the
		 * same value. Following the break-before-make
		 * (pmd_clear() followed by tlb_flush()) process can
		 * hinder forward progress due to refaults generated
		 * on missing translations.
		 *
		 * Skip updating the page table if the entry is
		 * unchanged.
		 */
		if (pmd_val(old_pmd) == pmd_val(*new_pmd))
			return 0;

		/*
		 * Mapping in huge pages should only happen through a
		 * fault. If a page is merged into a transparent huge
		 * page, the individual subpages of that huge page
		 * should be unmapped through MMU notifiers before we
		 * get here.
		 *
		 * Merging of CompoundPages is not supported; they
		 * should become splitting first, unmapped, merged,
		 * and mapped back in on-demand.
		 */
		VM_BUG_ON(pmd_pfn(old_pmd) != pmd_pfn(*new_pmd));

		if (sz == CONT_PMD_SIZE) {
			for (i = 0; i < CONT_PMDS; i++, pmd++)
				pmd_clear(pmd);
		} else
			pmd_clear(pmd);
		kvm_flush_remote_tlbs(kvm);
	} else {
		get_page(virt_to_page(pmd));
	}

	/* Do we need WRITE_ONCE(pmd, new_pmd)? */
	if (sz == CONT_PMD_SIZE) {
		for (i = 0; i < CONT_PMDS; i++, ori_pmd++)
			set_pmd(ori_pmd, *new_pmd);
	} else
		set_pmd(pmd, *new_pmd);
	return 0;
}

static int apt_set_pud_huge(struct kvm *kvm, struct kvm_mmu_memory_cache *cache,
		phys_addr_t addr, const pud_t *new_pudp)
{
	pud_t *pudp, old_pud;

retry:
	pudp = apt_get_pud(kvm->arch.pgd, cache, addr);
	VM_BUG_ON(!pudp);

	old_pud = *pudp;

	/*
	 * A large number of vcpus faulting on the same stage 2 entry,
	 * can lead to a refault due to the stage2_pud_clear()/tlb_flush().
	 * Skip updating the page tables if there is no change.
	 */
	if (pud_val(old_pud) == pud_val(*new_pudp))
		return 0;

	if (pud_present(old_pud)) {
		/*
		 * If we already have table level mapping for this block, unmap
		 * the range for this block and retry.
		 */
		if (!pud_huge(old_pud)) {
			unmap_apt_range(kvm, addr & PUD_MASK, PUD_SIZE);
			goto retry;
		}

//		WARN_ON_ONCE(kvm_pud_pfn(old_pud) != kvm_pud_pfn(*new_pudp));
		pud_clear(pudp);
		kvm_flush_remote_tlbs(kvm);
	} else {
		get_page(virt_to_page(pudp));
	}

	set_pud(pudp, *new_pudp);
	return 0;
}

static unsigned long
transparent_hugepage_adjust(struct kvm_memory_slot *memslot,
			    unsigned long hva, kvm_pfn_t *pfnp,
			    phys_addr_t *gpap)
{
	kvm_pfn_t pfn = *pfnp;
	struct page *page = pfn_to_page(pfn);

	/*
	 * Make sure the adjustment is done only for THP pages. Also make
	 * sure that the HVA and IPA are sufficiently aligned and that the
	 * block map is contained within the memslot.
	 */
	if (!PageHuge(page) && PageTransCompoundMap(page) &&
	    fault_supports_apt_huge_mapping(memslot, hva, PMD_SIZE)) {
		/*
		 * The address we faulted on is backed by a transparent huge
		 * page. However, because we map the compound huge page and
		 * not the individual tail page, we need to transfer the
		 * refcount to the head page. We have to be careful that the
		 * THP doesn't start to split while we are adjusting the
		 * refcounts.
		 *
		 * We are sure this doesn't happen, because mmu_notifier_retry
		 * was successful and we are holding the mmu_lock, so if this
		 * THP is trying to split, it will be blocked in the mmu
		 * notifier before touching any of the pages, specifically
		 * before being able to call __split_huge_page_refcount().
		 *
		 * We can therefore safely transfer the refcount from PG_tail
		 * to PG_head and switch the pfn from a tail page to the head
		 * page accordingly.
		 */
		*gpap &= PMD_MASK;
		kvm_release_pfn_clean(pfn);
		pfn &= ~(PTRS_PER_PMD - 1);
		kvm_get_pfn(pfn);
		*pfnp = pfn;
		return PMD_SIZE;
	}

	return PAGE_SIZE;
}

static int user_mem_abort(struct kvm_vcpu *vcpu, phys_addr_t fault_gpa,
			  struct kvm_memory_slot *memslot, unsigned long hva,
			  unsigned long fault_status)
{
	int ret;
	bool write_fault, exec_fault, writable, force_pte = false;
	unsigned long mmu_seq;
	gfn_t gfn = fault_gpa >> PAGE_SHIFT;
	struct kvm *kvm = vcpu->kvm;
	struct kvm_mmu_memory_cache *memcache = &vcpu->arch.mmu_page_cache;
	struct vm_area_struct *vma;
	kvm_pfn_t pfn;
	pgprot_t mem_type = PAGE_READONLY;
	bool logging_active = memslot_is_logging(memslot);
	unsigned long vma_pagesize, flags = 0;
	unsigned long as_info, access_type;
	unsigned int vma_shift;

	as_info = read_csr(CSR_AS_INFO);
	access_type = (as_info >> AF_ACCESS_TYPE_SHIFT) & AF_ACCESS_TYPE_MASK;
	write_fault = kvm_is_write_fault(access_type);
	exec_fault = kvm_is_exec_fault(access_type);
	VM_BUG_ON(write_fault && exec_fault);

	if (fault_status == AF_STATUS_FOR) {
		kvm_err("Unexpected APT read permission error\n");
		return -EFAULT;
	}

	/* Let's check if we will get back a huge page backed by hugetlbfs */
	down_read(&current->mm->mmap_lock);
	vma = find_vma_intersection(current->mm, hva, hva + 1);
	if (unlikely(!vma)) {
		kvm_err("Failed to find VMA for hva 0x%lx\n", hva);
		up_read(&current->mm->mmap_lock);
		return -EFAULT;
	}

	if (is_vm_hugetlb_page(vma))
		vma_shift = huge_page_shift(hstate_vma(vma));
	else
		vma_shift = PAGE_SHIFT;

	vma_pagesize = 1ULL << vma_shift;
	if (logging_active || (vma->vm_flags & VM_PFNMAP) ||
	    !fault_supports_apt_huge_mapping(memslot, hva, vma_pagesize)) {
		force_pte = true;
		vma_pagesize = PAGE_SIZE;
	}

	if (vma_pagesize == PMD_SIZE || vma_pagesize == CONT_PMD_SIZE || vma_pagesize == PUD_SIZE)
		gfn = (fault_gpa & huge_page_mask(hstate_vma(vma))) >> PAGE_SHIFT;
	up_read(&current->mm->mmap_lock);
	/* We need minimum second+third level pages */
	ret = mmu_topup_memory_cache(memcache, KVM_MMU_CACHE_MIN_PAGES,
				     KVM_NR_MEM_OBJS);
	if (ret)
		return ret;

	mmu_seq = vcpu->kvm->mmu_notifier_seq;
	/*
	 * Ensure the read of mmu_notifier_seq happens before we call
	 * gfn_to_pfn_prot (which calls get_user_pages), so that we don't risk
	 * the page we just got a reference to gets unmapped before we have a
	 * chance to grab the mmu_lock, which ensure that if the page gets
	 * unmapped afterwards, the call to kvm_unmap_hva will take it away
	 * from us again properly. This smp_rmb() interacts with the smp_wmb()
	 * in kvm_mmu_notifier_invalidate_<page|range_end>.
	 */
	smp_rmb();

	pfn = gfn_to_pfn_prot(kvm, gfn, write_fault, &writable);
	if (pfn == KVM_PFN_ERR_HWPOISON) {
		kvm_send_hwpoison_signal(hva, vma);
		return 0;
	}
	if (is_error_noslot_pfn(pfn))
		return -EFAULT;

	if (logging_active) {
		/*
		 * Faults on pages in a memslot with logging enabled
		 * should not be mapped with huge pages (it introduces churn
		 * and performance degradation), so force a pte mapping.
		 */
		flags |= KVM_APT_FLAG_LOGGING_ACTIVE;

		/*
		 * Only actually map the page as writable if this was a write
		 * fault.
		 */
		if (!write_fault)
			writable = false;
	}

	spin_lock(&kvm->mmu_lock);
	if (mmu_notifier_retry(kvm, mmu_seq))
		goto out_unlock;

	/*
	 * If we are not forced to use page mapping, check if we are
	 * backed by a THP and thus use block mapping if possible.
	 */
	if (vma_pagesize == PAGE_SIZE && !force_pte) {
		vma_pagesize = transparent_hugepage_adjust(memslot, hva,
				&pfn, &fault_gpa);
	}

	if (vma_pagesize == PUD_SIZE) {
		pud_t new_pud = pfn_pud(pfn, mem_type);

		new_pud = pud_mkhuge(new_pud);

		if (writable) {
			new_pud = kvm_pud_mkwrite(new_pud);
			kvm_set_pfn_dirty(pfn);
		}

		if (exec_fault && fault_status == AF_STATUS_INV) {
			new_pud = kvm_pud_mkexec(new_pud);
		} else if (fault_status == AF_STATUS_FOE) {
			/* Preserve execute if FOE was already cleared */
			if (apt_is_exec(kvm, fault_gpa))
				new_pud = kvm_pud_mkexec(new_pud);
		}

		ret = apt_set_pud_huge(kvm, memcache, fault_gpa, &new_pud);
	} else if (vma_pagesize == CONT_PMD_SIZE) {
		pmd_t new_pmd = pfn_pmd(pfn, mem_type);

		new_pmd = pmd_mkhuge(new_pmd);
		new_pmd = pmd_mkcont(new_pmd);

		if (writable) {
			new_pmd = kvm_pmd_mkwrite(new_pmd);
			kvm_set_pfn_dirty(pfn);
		}

		if (exec_fault && fault_status == AF_STATUS_INV) {
			new_pmd = kvm_pmd_mkexec(new_pmd);
		} else if (fault_status == AF_STATUS_FOE) {
			/* Preserve execute if FOE was already cleared */
			if (apt_is_exec(kvm, fault_gpa))
				new_pmd = kvm_pmd_mkexec(new_pmd);
		}

		ret = apt_set_pmd_huge(kvm, memcache, fault_gpa, &new_pmd, vma_pagesize);
	} else if (vma_pagesize == PMD_SIZE) {
		pmd_t new_pmd = pfn_pmd(pfn, mem_type);

		new_pmd = pmd_mkhuge(new_pmd);

		if (writable) {
			new_pmd = kvm_pmd_mkwrite(new_pmd);
			kvm_set_pfn_dirty(pfn);
		}

		if (exec_fault && fault_status == AF_STATUS_INV) {
			new_pmd = kvm_pmd_mkexec(new_pmd);
		} else if (fault_status == AF_STATUS_FOE) {
			/* Preserve execute if FOE was already cleared */
			if (apt_is_exec(kvm, fault_gpa))
				new_pmd = kvm_pmd_mkexec(new_pmd);
		}

		ret = apt_set_pmd_huge(kvm, memcache, fault_gpa, &new_pmd, vma_pagesize);
	} else {
		pte_t new_pte = pfn_pte(pfn, mem_type);

		if (writable) {
			new_pte = kvm_pte_mkwrite(new_pte);
			kvm_set_pfn_dirty(pfn);
			mark_page_dirty(kvm, gfn);
		}

		if (exec_fault && fault_status == AF_STATUS_INV) {
			new_pte = kvm_pte_mkexec(new_pte);
		} else if (fault_status == AF_STATUS_FOE) {
			/* Preserve execute if FOE was already cleared */
			if (apt_is_exec(kvm, fault_gpa))
				new_pte = kvm_pte_mkexec(new_pte);
		}

		ret = apt_set_pte_fast(kvm, memcache, fault_gpa, &new_pte, flags);
		if (!ret)
			goto out_unlock;
	}

out_unlock:
	spin_unlock(&kvm->mmu_lock);
	kvm_set_pfn_accessed(pfn);
	kvm_release_pfn_clean(pfn);
	return ret;
}

/**
 * kvm_handle_guest_abort - handles all 2nd stage aborts
 * @vcpu:	the VCPU pointer
 * @run:	the kvm_run structure
 *
 * Any abort that gets to the host is almost guaranteed to be caused by a
 * missing second stage translation table entry, which can mean that either the
 * guest simply needs more memory and we must allocate an appropriate page or it
 * can mean that the guest tried to access I/O memory, which is emulated by user
 * space. The distinction is based on the IPA causing the fault and whether this
 * memory region has been registered as standard RAM by user space.
 */
#ifdef CONFIG_SUBARCH_C4
int kvm_handle_guest_abort(struct kvm_vcpu *vcpu, struct kvm_run *run)
{
	unsigned long as_info;		/* the value of CSR: AS_INFO */
	unsigned int access_type, inv_level;
	unsigned int fault_status;
	unsigned long fault_entry_addr;
	phys_addr_t fault_gpa;
	struct kvm_memory_slot *memslot;
	unsigned long hva;
	bool write_fault, writable;
	gfn_t gfn;

	int ret, idx;

	as_info = read_csr(CSR_AS_INFO);
	access_type = (as_info >> AF_ACCESS_TYPE_SHIFT) & AF_ACCESS_TYPE_MASK;
	inv_level = (as_info >> AF_INV_LEVEL_SHIFT) & AF_INV_LEVEL_MASK;
	fault_status = (as_info >> AF_FAULT_STATUS_SHIFT) & AF_FAULT_STATUS_MASK;
	fault_entry_addr = (as_info & AF_ENTRY_ADDR_MASK) >> 3;

	fault_gpa = read_csr(CSR_EXC_GPA);
	idx = srcu_read_lock(&vcpu->kvm->srcu);

	gfn = fault_gpa >> PAGE_SHIFT;
	memslot = gfn_to_memslot(vcpu->kvm, gfn);
	hva = gfn_to_hva_memslot_prot(memslot, gfn, &writable);

	write_fault = kvm_is_write_fault(access_type);

	/* The memory slot for IO doesn't register in memory region
	 * with kvm, if hva == KVM_HVA_ERR_BAD, the gpa used for MMIO
	 * needs emulation.
	 */

	if (hva == KVM_HVA_ERR_BAD) {
		ret = io_mem_abort(vcpu, run, NULL);
		goto out_unlock;
	}
	/* Userspace should not be able to register out-of-bounds IPAs */
	VM_BUG_ON(fault_gpa >= KVM_PHYS_SIZE);

	ret = user_mem_abort(vcpu, fault_gpa, memslot, hva, fault_status);
	if (ret == 0)
		ret = 1;
out_unlock:
	srcu_read_unlock(&vcpu->kvm->srcu, idx);
	return ret;
}
#endif
static int handle_hva_to_gpa(struct kvm *kvm, unsigned long start, unsigned long end,
		int (*handler)(struct kvm *kvm, gpa_t gpa, u64 size, void *data),
		void *data)
{
	struct kvm_memslots *slots;
	struct kvm_memory_slot *memslot;
	int ret = 0;

	slots = kvm_memslots(kvm);

	/* we only care about the pages that the guest sees */
	kvm_for_each_memslot(memslot, slots) {
		unsigned long hva_start, hva_end;
		gfn_t gpa;

		hva_start = max(start, memslot->userspace_addr);
		hva_end = min(end, memslot->userspace_addr +
				(memslot->npages << PAGE_SHIFT));
		if (hva_start >= hva_end)
			continue;

		gpa = hva_to_gfn_memslot(hva_start, memslot) << PAGE_SHIFT;
		ret |= handler(kvm, gpa, (u64)(hva_end - hva_start), data);
	}

	return ret;
}

static int kvm_unmap_hva_handler(struct kvm *kvm, gpa_t gpa, u64 size, void *data)
{
	unmap_apt_range(kvm, gpa, size);
	return 0;
}

int kvm_unmap_hva_range(struct kvm *kvm,
		unsigned long start, unsigned long end, bool blockable)
{
	if (!kvm->arch.pgd)
		return 0;

	handle_hva_to_gpa(kvm, start, end, &kvm_unmap_hva_handler, NULL);
	return 1;
}

static int apt_ptep_test_and_clear_young(pte_t *pte)
{
	if (pte_young(*pte)) {
		*pte = pte_mkold(*pte);
		return 1;
	}
	return 0;
}

static int apt_pmdp_test_and_clear_young(pmd_t *pmd)
{
	return apt_ptep_test_and_clear_young((pte_t *)pmd);
}

static int apt_pudp_test_and_clear_young(pud_t *pud)
{
	return apt_ptep_test_and_clear_young((pte_t *)pud);
}

static int kvm_age_hva_handler(struct kvm *kvm, gpa_t gpa, u64 size, void *data)
{
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	WARN_ON(size != PAGE_SIZE && size != PMD_SIZE && size != PUD_SIZE);
	if (!apt_get_leaf_entry(kvm, gpa, &pud, &pmd, &pte))
		return 0;

	if (pud)
		return apt_pudp_test_and_clear_young(pud);
	else if (pmd)
		return apt_pmdp_test_and_clear_young(pmd);
	else
		return apt_ptep_test_and_clear_young(pte);
}

static int kvm_test_age_hva_handler(struct kvm *kvm, gpa_t gpa, u64 size, void *data)
{
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	WARN_ON(size != PAGE_SIZE && size != PMD_SIZE && size != PUD_SIZE);
	if (!apt_get_leaf_entry(kvm, gpa, &pud, &pmd, &pte))
		return 0;

	if (pud)
		return apt_pudp_test_and_clear_young(pud);
	else if (pmd)
		return apt_pmdp_test_and_clear_young(pmd);
	else
		return apt_ptep_test_and_clear_young(pte);
}

int kvm_age_hva(struct kvm *kvm, unsigned long start, unsigned long end)
{
	if (!kvm->arch.pgd)
		return 0;

	return handle_hva_to_gpa(kvm, start, end, kvm_age_hva_handler, NULL);
}

int kvm_test_age_hva(struct kvm *kvm, unsigned long hva)
{
	if (!kvm->arch.pgd)
		return 0;
	return handle_hva_to_gpa(kvm, hva, hva, kvm_test_age_hva_handler, NULL);
}

static int kvm_set_apte_handler(struct kvm *kvm, gpa_t gpa, u64 size, void *data)
{
	pte_t *pte = (pte_t *)data;

	WARN_ON(size != PAGE_SIZE);

	apt_set_pte(kvm, NULL, gpa, pte, 0);
	return 0;
}

int kvm_set_spte_hva(struct kvm *kvm, unsigned long hva, pte_t pte)
{
	unsigned long end = hva + PAGE_SIZE;
	pte_t apt_pte;

	if (!kvm->arch.pgd)
		return 0;

	apt_pte = pte_wrprotect(pte);
	handle_hva_to_gpa(kvm, hva, end, &kvm_set_apte_handler, &apt_pte);
	return 0;
}

/**
 * kvm_mmu_write_protect_pt_masked() - write protect dirty pages
 * @kvm:	The KVM pointer
 * @slot:	The memory slot associated with mask
 * @gfn_offset:	The gfn offset in memory slot
 * @mask:	The mask of dirty pages at offset 'gfn_offset' in this memory
 *		slot to be write protected
 *
 * Walks bits set in mask write protects the associated pte's. Caller must
 * acquire kvm_mmu_lock.
 */
static void kvm_mmu_write_protect_pt_masked(struct kvm *kvm,
		struct kvm_memory_slot *slot,
		gfn_t gfn_offset, unsigned long mask)
{
	phys_addr_t base_gfn = slot->base_gfn + gfn_offset;
	phys_addr_t start = (base_gfn + __ffs(mask)) << PAGE_SHIFT;
	phys_addr_t end = (base_gfn + __fls(mask) + 1) << PAGE_SHIFT;

	apt_wp_range(kvm, start, end);
}

/*
 * kvm_arch_mmu_enable_log_dirty_pt_masked - enable dirty logging for selected
 * dirty pages.
 *
 * It calls kvm_mmu_write_protect_pt_masked to write protect selected pages to
 * enable dirty logging for them.
 */
void kvm_arch_mmu_enable_log_dirty_pt_masked(struct kvm *kvm,
		struct kvm_memory_slot *slot,
		gfn_t gfn_offset, unsigned long mask)
{
	kvm_mmu_write_protect_pt_masked(kvm, slot, gfn_offset, mask);
}

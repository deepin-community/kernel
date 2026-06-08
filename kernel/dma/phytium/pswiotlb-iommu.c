// SPDX-License-Identifier: GPL-2.0
/*
 * DMA operations based on Phytium software IO tlb that
 * map physical memory indirectly with an IOMMU.
 *
 * Copyright (c) 2024, Phytium Technology Co., Ltd.
 */

#define pr_fmt(fmt)    "pswiotlb iommu: " fmt

#include <linux/acpi_iort.h>
#include <linux/device.h>
#include <linux/dma-map-ops.h>
#include <linux/kernel.h>
#include <linux/bits.h>
#include <linux/bug.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/huge_mm.h>
#include <linux/iommu.h>
#include <linux/idr.h>
#include <linux/notifier.h>
#include <linux/err.h>
#include <linux/iova.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/pci.h>
#include <linux/bitops.h>
#include <linux/property.h>
#include <linux/fsl/mc.h>
#include <linux/module.h>
#include <trace/events/iommu.h>
#include <linux/swiotlb.h>
#include <linux/scatterlist.h>
#include <linux/vmalloc.h>
#include <linux/crash_dump.h>
#include <linux/dma-direct.h>

#include <linux/atomic.h>
#include <linux/crash_dump.h>
#include <linux/list_sort.h>
#include <linux/memremap.h>
#include <linux/of_iommu.h>
#include <linux/spinlock.h>
#include <linux/pswiotlb.h>
#ifdef CONFIG_ARCH_PHYTIUM
#include <asm/cputype.h>
#endif

#include "pswiotlb-dma.h"

enum iommu_dma_cookie_type {
	IOMMU_DMA_IOVA_COOKIE,
	IOMMU_DMA_MSI_COOKIE,
};

enum iommu_dma_queue_type {
	IOMMU_DMA_OPTS_PER_CPU_QUEUE,
	IOMMU_DMA_OPTS_SINGLE_QUEUE,
};

struct iommu_dma_options {
	enum iommu_dma_queue_type qt;
	size_t		fq_size;
	unsigned int	fq_timeout;
};

struct iommu_dma_cookie {
	enum iommu_dma_cookie_type	type;
	union {
		/* Full allocator for IOMMU_DMA_IOVA_COOKIE */
		struct {
			struct iova_domain	iovad;
			/* Flush queue */
			union {
				struct iova_fq	*single_fq;
				struct iova_fq	__percpu *percpu_fq;
			};
			/* Number of TLB flushes that have been started */
			atomic64_t		fq_flush_start_cnt;
			/* Number of TLB flushes that have been finished */
			atomic64_t		fq_flush_finish_cnt;
			/* Timer to regularily empty the flush queues */
			struct timer_list	fq_timer;
			/* 1 when timer is active, 0 when not */
			atomic_t		fq_timer_on;
		};
		/* Trivial linear page allocator for IOMMU_DMA_MSI_COOKIE */
		dma_addr_t		msi_iova;
	};
	struct list_head		msi_page_list;

	/* Domain for flush queue callback; NULL if flush queue not in use */
	struct iommu_domain		*fq_domain;
	/* Options for dma-iommu use */
	struct iommu_dma_options	options;
	struct mutex			mutex;

	struct work_struct free_iova_work;
};

static DEFINE_STATIC_KEY_FALSE(iommu_deferred_attach_enabled);

/* Flush queue entry for deferred flushing */
struct iova_fq_entry {
	unsigned long iova_pfn;
	unsigned long pages;
	struct list_head freelist;
	u64 counter; /* Flush counter when this entry was added */
};

/* Per-CPU flush queue structure */
struct iova_fq {
	spinlock_t lock;
	unsigned int head, tail;
	unsigned int mod_mask;
	struct iova_fq_entry entries[];
};

#define fq_ring_for_each(i, fq) \
	for ((i) = (fq)->head; (i) != (fq)->tail; (i) = ((i) + 1) & (fq)->mod_mask)
/*
 * The following functions are ported from
 * ./drivers/iommu/dma-iommu.c
 * ./drivers/iommu/iommu.c
 * static inline bool fq_full(struct iova_fq *fq);
 * static void fq_flush_iotlb(struct iommu_dma_cookie *cookie);
 * static inline unsigned int fq_ring_add(struct iova_fq *fq);
 * static void fq_ring_free_locked(struct iommu_dma_cookie *cookie, struct iova_fq *fq);
 * static size_t iommu_pgsize(struct iommu_domain *domain, unsigned long iova,
 *		phys_addr_t paddr, size_t size, size_t *count);
 * static int __iommu_map_pages(struct iommu_domain *domain, unsigned long iova,
 *		phys_addr_t paddr, size_t size, int prot, gfp_t gfp, size_t *mapped);
 * static int __iommu_map(struct iommu_domain *domain, unsigned long iova,
 *		phys_addr_t paddr, size_t size, int prot, gfp_t gfp);
 * static bool dev_use_swiotlb(struct device *dev, size_t size,
 *		enum dma_data_direction dir);
 * static bool dev_is_untrusted(struct device *dev);
 * static int dma_info_to_prot(enum dma_data_direction dir, bool coherent,
 *		unsigned long attrs);
 * static void queue_iova(struct iommu_dma_cookie *cookie,
 *		unsigned long pfn, unsigned long pages,
 *		struct list_head *freelist);
 * static dma_addr_t iommu_dma_alloc_iova(struct iommu_domain *domain,
 *		size_t size, u64 dma_limit, struct device *dev);
 * static void iommu_dma_free_iova(struct iommu_dma_cookie *cookie,
 *		dma_addr_t iova, size_t size, struct iommu_iotlb_gather *gather);
 * static void __iommu_dma_unmap(struct device *dev, dma_addr_t dma_addr,
 *		size_t size);
 * static dma_addr_t __iommu_dma_map(struct device *dev, phys_addr_t phys,
 *		size_t size, int prot, u64 dma_mask);
 * static int __finalise_sg(struct device *dev, struct scatterlist *sg, int nents,
 *		dma_addr_t dma_addr);
 * static void __invalidate_sg(struct scatterlist *sg, int nents);
 */

static inline bool fq_full(struct iova_fq *fq)
{
	assert_spin_locked(&fq->lock);
	return (((fq->tail + 1) & fq->mod_mask) == fq->head);
}

static inline unsigned int fq_ring_add(struct iova_fq *fq)
{
	unsigned int idx = fq->tail;

	assert_spin_locked(&fq->lock);

	fq->tail = (idx + 1) & fq->mod_mask;

	return idx;
}

static void fq_ring_free_locked(struct iommu_dma_cookie *cookie, struct iova_fq *fq)
{
	u64 counter = atomic64_read(&cookie->fq_flush_finish_cnt);
	unsigned int idx;

	assert_spin_locked(&fq->lock);

	fq_ring_for_each(idx, fq) {

		if (fq->entries[idx].counter >= counter)
			break;

		put_pages_list(&fq->entries[idx].freelist);
		free_iova_fast(&cookie->iovad,
			       fq->entries[idx].iova_pfn,
			       fq->entries[idx].pages);

		fq->head = (fq->head + 1) & fq->mod_mask;
	}
}

static void fq_flush_iotlb(struct iommu_dma_cookie *cookie)
{
	atomic64_inc(&cookie->fq_flush_start_cnt);
	cookie->fq_domain->ops->flush_iotlb_all(cookie->fq_domain);
	atomic64_inc(&cookie->fq_flush_finish_cnt);
}

static size_t iommu_pgsize(struct iommu_domain *domain, unsigned long iova,
			   phys_addr_t paddr, size_t size, size_t *count)
{
	unsigned int pgsize_idx, pgsize_idx_next;
	unsigned long pgsizes;
	size_t offset, pgsize, pgsize_next;
	unsigned long addr_merge = paddr | iova;

	/* Page sizes supported by the hardware and small enough for @size */
	pgsizes = domain->pgsize_bitmap & GENMASK(__fls(size), 0);

	/* Constrain the page sizes further based on the maximum alignment */
	if (likely(addr_merge))
		pgsizes &= GENMASK(__ffs(addr_merge), 0);

	/* Make sure we have at least one suitable page size */
	BUG_ON(!pgsizes);

	/* Pick the biggest page size remaining */
	pgsize_idx = __fls(pgsizes);
	pgsize = BIT(pgsize_idx);
	if (!count)
		return pgsize;

	/* Find the next biggest support page size, if it exists */
	pgsizes = domain->pgsize_bitmap & ~GENMASK(pgsize_idx, 0);
	if (!pgsizes)
		goto out_set_count;

	pgsize_idx_next = __ffs(pgsizes);
	pgsize_next = BIT(pgsize_idx_next);

	/*
	 * There's no point trying a bigger page size unless the virtual
	 * and physical addresses are similarly offset within the larger page.
	 */
	if ((iova ^ paddr) & (pgsize_next - 1))
		goto out_set_count;

	/* Calculate the offset to the next page size alignment boundary */
	offset = pgsize_next - (addr_merge & (pgsize_next - 1));

	/*
	 * If size is big enough to accommodate the larger page, reduce
	 * the number of smaller pages.
	 */
	if (offset + pgsize_next <= size)
		size = offset;

out_set_count:
	*count = size >> pgsize_idx;
	return pgsize;
}

static int __iommu_map_pages(struct iommu_domain *domain, unsigned long iova,
			     phys_addr_t paddr, size_t size, int prot,
			     gfp_t gfp, size_t *mapped)
{
	const struct iommu_domain_ops *ops = domain->ops;
	size_t pgsize, count;
	int ret;

	pgsize = iommu_pgsize(domain, iova, paddr, size, &count);

	pr_debug("mapping: iova 0x%lx pa %pa pgsize 0x%zx count %zu\n",
		 iova, &paddr, pgsize, count);

	if (ops->map_pages) {
		ret = ops->map_pages(domain, iova, paddr, pgsize, count, prot,
				     gfp, mapped);
	} else {
		ret = ops->map(domain, iova, paddr, pgsize, prot, gfp);
		*mapped = ret ? 0 : pgsize;
	}

	return ret;
}

static void queue_iova(struct iommu_dma_cookie *cookie,
		unsigned long pfn, unsigned long pages,
		struct list_head *freelist)
{
	struct iova_fq *fq;
	unsigned long flags;
	unsigned int idx;

	/*
	 * Order against the IOMMU driver's pagetable update from unmapping
	 * @pte, to guarantee that fq_flush_iotlb() observes that if called
	 * from a different CPU before we release the lock below. Full barrier
	 * so it also pairs with iommu_dma_init_fq() to avoid seeing partially
	 * written fq state here.
	 */
	smp_mb();

	if (cookie->options.qt == IOMMU_DMA_OPTS_SINGLE_QUEUE)
		fq = cookie->single_fq;
	else
		fq = raw_cpu_ptr(cookie->percpu_fq);

	spin_lock_irqsave(&fq->lock, flags);

	/*
	 * First remove all entries from the flush queue that have already been
	 * flushed out on another CPU. This makes the fq_full() check below less
	 * likely to be true.
	 */
	fq_ring_free_locked(cookie, fq);

	if (fq_full(fq)) {
		fq_flush_iotlb(cookie);
		fq_ring_free_locked(cookie, fq);
	}

	idx = fq_ring_add(fq);

	fq->entries[idx].iova_pfn = pfn;
	fq->entries[idx].pages    = pages;
	fq->entries[idx].counter  = atomic64_read(&cookie->fq_flush_start_cnt);
	list_splice(freelist, &fq->entries[idx].freelist);

	spin_unlock_irqrestore(&fq->lock, flags);

	/* Avoid false sharing as much as possible. */
	if (!atomic_read(&cookie->fq_timer_on) &&
	    !atomic_xchg(&cookie->fq_timer_on, 1))
		mod_timer(&cookie->fq_timer,
			  jiffies + msecs_to_jiffies(cookie->options.fq_timeout));
}

static int __iommu_map(struct iommu_domain *domain, unsigned long iova,
		       phys_addr_t paddr, size_t size, int prot, gfp_t gfp)
{
	const struct iommu_domain_ops *ops = domain->ops;
	unsigned long orig_iova = iova;
	unsigned int min_pagesz;
	size_t orig_size = size;
	phys_addr_t orig_paddr = paddr;
	int ret = 0;

	if (unlikely(!(ops->map || ops->map_pages) ||
		     domain->pgsize_bitmap == 0UL))
		return -ENODEV;

	if (unlikely(!(domain->type & __IOMMU_DOMAIN_PAGING)))
		return -EINVAL;

	/* find out the minimum page size supported */
	min_pagesz = 1 << __ffs(domain->pgsize_bitmap);

	/*
	 * both the virtual address and the physical one, as well as
	 * the size of the mapping, must be aligned (at least) to the
	 * size of the smallest page supported by the hardware
	 */
	if (!IS_ALIGNED(iova | paddr | size, min_pagesz)) {
		pr_err("unaligned: iova 0x%lx pa %pa size 0x%zx min_pagesz 0x%x\n",
		       iova, &paddr, size, min_pagesz);
		return -EINVAL;
	}

	pr_debug("map: iova 0x%lx pa %pa size 0x%zx\n", iova, &paddr, size);

	while (size) {
		size_t mapped = 0;

		ret = __iommu_map_pages(domain, iova, paddr, size, prot, gfp,
					&mapped);
		/*
		 * Some pages may have been mapped, even if an error occurred,
		 * so we should account for those so they can be unmapped.
		 */
		size -= mapped;

		if (ret)
			break;

		iova += mapped;
		paddr += mapped;
	}

	/* unroll mapping in case something went wrong */
	if (ret)
		iommu_unmap(domain, orig_iova, orig_size - size);
	else
		trace_map(orig_iova, orig_paddr, orig_size);

	return ret;
}

static ssize_t __iommu_map_sg_dma(struct device *dev, struct iommu_domain *domain,
		unsigned long iova, struct scatterlist *sg, unsigned int nents,
		int prot, gfp_t gfp, enum dma_data_direction dir, unsigned long attrs)
{
	const struct iommu_domain_ops *ops = domain->ops;
	size_t mapped = 0;
	int ret;
	struct iommu_dma_cookie *cookie = domain->iova_cookie;
	struct iova_domain *iovad = &cookie->iovad;
	size_t aligned_size;
	int nid = dev->numa_node;
	struct scatterlist *sg_orig = sg;
	struct scatterlist *s;
	int i;

	might_sleep_if(gfpflags_allow_blocking(gfp));

	/* Discourage passing strange GFP flags */
	if (WARN_ON_ONCE(gfp & (__GFP_COMP | __GFP_DMA | __GFP_DMA32 |
				__GFP_HIGHMEM)))
		return -EINVAL;

	for_each_sg(sg, s, nents, i) {
		phys_addr_t phys = page_to_phys(sg_page(s)) + s->offset;

		/* check whether dma addr is in local node */
		if (dir != DMA_TO_DEVICE) {
			aligned_size = s->length;
			if ((!dma_is_in_local_node(dev, nid, phys,
				aligned_size)) && (pswiotlb_force_disable != true)) {
				aligned_size = iova_align(iovad, s->length);
				phys = pswiotlb_tbl_map_single(dev, nid,
				phys, s->length, aligned_size, iova_mask(iovad), dir, attrs);
				if (phys == DMA_MAPPING_ERROR) {
					phys = page_to_phys(sg_page(s)) + s->offset;
					dev_warn_once(dev,
						"Failed to allocate memory from pswiotlb, fall back to non-local dma\n");
				}
			}
		}
		if (!dev_is_dma_coherent(dev) && !(attrs & DMA_ATTR_SKIP_CPU_SYNC))
			arch_sync_dma_for_device(phys, s->length, dir);

		ret = __iommu_map(domain, iova + mapped, phys,
				s->length, prot, gfp);
		if (ret)
			goto out_err;

		mapped += s->length;
	}

	if (ops->iotlb_sync_map)
		ops->iotlb_sync_map(domain, iova, mapped);
	return mapped;

out_err:
	/* undo mappings already done */
	iommu_dma_unmap_sg_pswiotlb(dev, sg_orig, iova,
				mapped, i, dir, attrs | DMA_ATTR_SKIP_CPU_SYNC);
	iommu_unmap(domain, iova, mapped);

	return ret;
}

static ssize_t pswiotlb_iommu_map_sg_atomic_dma(struct device *dev,
			struct iommu_domain *domain, unsigned long iova,
			struct scatterlist *sg, unsigned int nents, int prot,
			enum dma_data_direction dir, unsigned long attrs)
{
	return __iommu_map_sg_dma(dev, domain, iova, sg, nents, prot, GFP_ATOMIC, dir, attrs);
}

static bool dev_is_untrusted(struct device *dev)
{
	return dev_is_pci(dev) && to_pci_dev(dev)->untrusted;
}

static bool dev_use_swiotlb(struct device *dev, size_t size,
			    enum dma_data_direction dir)
{
	return IS_ENABLED(CONFIG_SWIOTLB) &&
		(dev_is_untrusted(dev) ||
		 dma_kmalloc_needs_bounce(dev, size, dir));
}

/**
 * dma_info_to_prot - Translate DMA API directions and attributes to IOMMU API
 *                    page flags.
 * @dir: Direction of DMA transfer
 * @coherent: Is the DMA master cache-coherent?
 * @attrs: DMA attributes for the mapping
 *
 * Return: corresponding IOMMU API page protection flags
 */
static int dma_info_to_prot(enum dma_data_direction dir, bool coherent,
		     unsigned long attrs)
{
	int prot = coherent ? IOMMU_CACHE : 0;

	if (attrs & DMA_ATTR_PRIVILEGED)
		prot |= IOMMU_PRIV;

	switch (dir) {
	case DMA_BIDIRECTIONAL:
		return prot | IOMMU_READ | IOMMU_WRITE;
	case DMA_TO_DEVICE:
		return prot | IOMMU_READ;
	case DMA_FROM_DEVICE:
		return prot | IOMMU_WRITE;
	default:
		return 0;
	}
}

static dma_addr_t iommu_dma_alloc_iova(struct iommu_domain *domain,
		size_t size, u64 dma_limit, struct device *dev)
{
	struct iommu_dma_cookie *cookie = domain->iova_cookie;
	struct iova_domain *iovad = &cookie->iovad;
	unsigned long shift, iova_len, iova;

	if (cookie->type == IOMMU_DMA_MSI_COOKIE) {
		cookie->msi_iova += size;
		return cookie->msi_iova - size;
	}

	shift = iova_shift(iovad);
	iova_len = size >> shift;

	dma_limit = min_not_zero(dma_limit, dev->bus_dma_limit);

	if (domain->geometry.force_aperture)
		dma_limit = min_t(u64, dma_limit, (u64)domain->geometry.aperture_end);

	/*
	 * Try to use all the 32-bit PCI addresses first. The original SAC vs.
	 * DAC reasoning loses relevance with PCIe, but enough hardware and
	 * firmware bugs are still lurking out there that it's safest not to
	 * venture into the 64-bit space until necessary.
	 *
	 * If your device goes wrong after seeing the notice then likely either
	 * its driver is not setting DMA masks accurately, the hardware has
	 * some inherent bug in handling >32-bit addresses, or not all the
	 * expected address bits are wired up between the device and the IOMMU.
	 */
	if (dma_limit > DMA_BIT_MASK(32) && dev->iommu->pci_32bit_workaround) {
		iova = alloc_iova_fast(iovad, iova_len,
				       DMA_BIT_MASK(32) >> shift, false);
		if (iova)
			goto done;

		dev->iommu->pci_32bit_workaround = false;
		dev_notice(dev, "Using %d-bit DMA addresses\n", bits_per(dma_limit));
	}

	iova = alloc_iova_fast(iovad, iova_len, dma_limit >> shift, true);
done:
	return (dma_addr_t)iova << shift;
}

static void iommu_dma_free_iova(struct iommu_dma_cookie *cookie,
		dma_addr_t iova, size_t size, struct iommu_iotlb_gather *gather)
{
	struct iova_domain *iovad = &cookie->iovad;

	/* The MSI case is only ever cleaning up its most recent allocation */
	if (cookie->type == IOMMU_DMA_MSI_COOKIE)
		cookie->msi_iova -= size;
	else if (gather && gather->queued)
		queue_iova(cookie, iova_pfn(iovad, iova),
				size >> iova_shift(iovad),
				&gather->freelist);
	else
		free_iova_fast(iovad, iova_pfn(iovad, iova),
				size >> iova_shift(iovad));
}

static void __iommu_dma_unmap(struct device *dev, dma_addr_t dma_addr,
		size_t size)
{
	struct iommu_domain *domain = iommu_get_dma_domain(dev);
	struct iommu_dma_cookie *cookie = domain->iova_cookie;
	struct iova_domain *iovad = &cookie->iovad;
	size_t iova_off = iova_offset(iovad, dma_addr);
	struct iommu_iotlb_gather iotlb_gather;
	size_t unmapped;

	dma_addr -= iova_off;
	size = iova_align(iovad, size + iova_off);
	iommu_iotlb_gather_init(&iotlb_gather);
	iotlb_gather.queued = READ_ONCE(cookie->fq_domain);

	unmapped = iommu_unmap_fast(domain, dma_addr, size, &iotlb_gather);
	WARN_ON(unmapped != size);

	if (!iotlb_gather.queued)
		iommu_iotlb_sync(domain, &iotlb_gather);
	iommu_dma_free_iova(cookie, dma_addr, size, &iotlb_gather);
}

static dma_addr_t __iommu_dma_map(struct device *dev, phys_addr_t phys,
		size_t size, int prot, u64 dma_mask)
{
	struct iommu_domain *domain = iommu_get_dma_domain(dev);
	struct iommu_dma_cookie *cookie = domain->iova_cookie;
	struct iova_domain *iovad = &cookie->iovad;
	size_t iova_off = iova_offset(iovad, phys);
	dma_addr_t iova;

	if (static_branch_unlikely(&iommu_deferred_attach_enabled) &&
	    iommu_deferred_attach(dev, domain))
		return DMA_MAPPING_ERROR;

	size = iova_align(iovad, size + iova_off);

	iova = iommu_dma_alloc_iova(domain, size, dma_mask, dev);
	if (!iova)
		return DMA_MAPPING_ERROR;

	if (iommu_map(domain, iova, phys - iova_off, size, prot, GFP_ATOMIC)) {
		iommu_dma_free_iova(cookie, iova, size, NULL);
		return DMA_MAPPING_ERROR;
	}
	return iova + iova_off;
}

void pswiotlb_iommu_dma_sync_single_for_cpu(struct device *dev,
		dma_addr_t dma_handle, size_t size, enum dma_data_direction dir)
{
	phys_addr_t phys;
	int nid = dev->numa_node;
	struct p_io_tlb_pool *pool;

	if (is_pswiotlb_active(dev)) {
		phys = iommu_iova_to_phys(iommu_get_dma_domain(dev), dma_handle);
		if (!dev_is_dma_coherent(dev))
			arch_sync_dma_for_cpu(phys, size, dir);

		if (is_pswiotlb_buffer(dev, nid, phys, &pool))
			pswiotlb_sync_single_for_cpu(dev, nid, phys, size, dir, pool);

		if (dev_is_dma_coherent(dev) && !dev_use_swiotlb(dev, size, dir))
			return;

		if (is_swiotlb_buffer(dev, phys))
			swiotlb_sync_single_for_cpu(dev, phys, size, dir);
	} else {
		if (dev_is_dma_coherent(dev) && !dev_use_swiotlb(dev, size, dir))
			return;
		phys = iommu_iova_to_phys(iommu_get_dma_domain(dev), dma_handle);
		if (!dev_is_dma_coherent(dev))
			arch_sync_dma_for_cpu(phys, size, dir);
		if (is_swiotlb_buffer(dev, phys))
			swiotlb_sync_single_for_cpu(dev, phys, size, dir);
	}
}

void pswiotlb_iommu_dma_sync_single_for_device(struct device *dev,
		dma_addr_t dma_handle, size_t size, enum dma_data_direction dir)
{
	phys_addr_t phys;
	int nid = dev->numa_node;
	struct p_io_tlb_pool *pool;

	if (is_pswiotlb_active(dev)) {
		phys = iommu_iova_to_phys(iommu_get_dma_domain(dev), dma_handle);
		if (is_pswiotlb_buffer(dev, nid, phys, &pool))
			pswiotlb_sync_single_for_device(dev, nid, phys, size, dir, pool);

		if (dev_is_dma_coherent(dev) && !dev_use_swiotlb(dev, size, dir))
			return;
	} else {
		if (dev_is_dma_coherent(dev) && !dev_use_swiotlb(dev, size, dir))
			return;

		phys = iommu_iova_to_phys(iommu_get_dma_domain(dev), dma_handle);
	}

	if (is_swiotlb_buffer(dev, phys))
		swiotlb_sync_single_for_device(dev, phys, size, dir);

	if (!dev_is_dma_coherent(dev))
		arch_sync_dma_for_device(phys, size, dir);
}

void pswiotlb_iommu_dma_sync_sg_for_cpu(struct device *dev,
		struct scatterlist *sgl, int nelems,
		enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;
	int nid = dev->numa_node;
	dma_addr_t start_orig;
	phys_addr_t phys;
	struct iommu_domain *domain = iommu_get_dma_domain(dev);
	struct iommu_dma_cookie *cookie = domain->iova_cookie;
	struct iova_domain *iovad = &cookie->iovad;
	struct p_io_tlb_pool *pool;

	if (is_pswiotlb_active(dev)) {
		start_orig = sg_dma_address(sgl);
		for_each_sg(sgl, sg, nelems, i) {
			if (dir != DMA_TO_DEVICE) {
				unsigned int s_iova_off = iova_offset(iovad, sg->offset);

				if (i > 0)
					start_orig += s_iova_off;
				phys = iommu_iova_to_phys(iommu_get_dma_domain(dev), start_orig);
				if (!dev_is_dma_coherent(dev))
					arch_sync_dma_for_cpu(phys, sg->length, dir);

				if (is_pswiotlb_buffer(dev, nid, phys, &pool))
					pswiotlb_sync_single_for_cpu(dev, nid, phys,
									sg->length, dir, pool);
				start_orig -= s_iova_off;
				start_orig += iova_align(iovad, sg->length + s_iova_off);
			} else {
				if (!dev_is_dma_coherent(dev))
					arch_sync_dma_for_cpu(sg_phys(sg), sg->length, dir);
			}
		}
	} else {
		if (dev_is_dma_coherent(dev) && !dev_is_untrusted(dev))
			return;

		for_each_sg(sgl, sg, nelems, i) {
			if (!dev_is_dma_coherent(dev))
				arch_sync_dma_for_cpu(sg_phys(sg), sg->length, dir);

			if (is_swiotlb_buffer(dev, sg_phys(sg)))
				swiotlb_sync_single_for_cpu(dev, sg_phys(sg),
							    sg->length, dir);
		}
	}
}

void pswiotlb_iommu_dma_sync_sg_for_device(struct device *dev,
		struct scatterlist *sgl, int nelems,
		enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;
	int nid = dev->numa_node;
	struct p_io_tlb_pool *pool;

	if (is_pswiotlb_active(dev)) {
		for_each_sg(sgl, sg, nelems, i) {
			if (is_pswiotlb_buffer(dev, nid, sg_phys(sg), &pool))
				pswiotlb_sync_single_for_device(dev, nid, sg_phys(sg),
								   sg->length, dir, pool);
			if (dev_is_dma_coherent(dev) && !sg_dma_is_swiotlb(sgl))
				continue;

			if (!dev_is_dma_coherent(dev))
				arch_sync_dma_for_device(sg_phys(sg), sg->length, dir);
		}
	} else {
		if (dev_is_dma_coherent(dev) && !sg_dma_is_swiotlb(sgl))
			return;

		for_each_sg(sgl, sg, nelems, i) {
			if (is_swiotlb_buffer(dev, sg_phys(sg)))
				swiotlb_sync_single_for_device(dev, sg_phys(sg),
							       sg->length, dir);

			if (!dev_is_dma_coherent(dev))
				arch_sync_dma_for_device(sg_phys(sg), sg->length, dir);
		}
	}
}

dma_addr_t pswiotlb_iommu_dma_map_page(struct device *dev, struct page *page,
		unsigned long offset, size_t size, enum dma_data_direction dir,
		unsigned long attrs)
{
	phys_addr_t phys = page_to_phys(page) + offset;
	bool coherent = dev_is_dma_coherent(dev);

	int prot = dma_info_to_prot(dir, coherent, attrs);
	struct iommu_domain *domain = iommu_get_dma_domain(dev);
	struct iommu_dma_cookie *cookie = domain->iova_cookie;
	struct iova_domain *iovad = &cookie->iovad;
	size_t aligned_size = size;
	dma_addr_t iova, dma_mask = dma_get_mask(dev);
	int nid = dev->numa_node;
	struct p_io_tlb_pool *pool;

	/*
	 * If both the physical buffer start address and size are
	 * page aligned, we don't need to use a bounce page.
	 */
	if (dev_use_swiotlb(dev, size, dir) &&
	    iova_offset(iovad, phys | size)) {
		void *padding_start;
		size_t padding_size;

		if (!is_swiotlb_active(dev)) {
			dev_warn_once(dev, "DMA bounce buffers are inactive, unable to map unaligned transaction.\n");
			return DMA_MAPPING_ERROR;
		}

		aligned_size = iova_align(iovad, size);
		phys = swiotlb_tbl_map_single(dev, phys, size, aligned_size,
					      iova_mask(iovad), dir, attrs);

		if (phys == DMA_MAPPING_ERROR)
			return DMA_MAPPING_ERROR;

		/* Cleanup the padding area. */
		padding_start = phys_to_virt(phys);
		padding_size = aligned_size;

		if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC) &&
		    (dir == DMA_TO_DEVICE || dir == DMA_BIDIRECTIONAL)) {
			padding_start += size;
			padding_size -= size;
		}

		memset(padding_start, 0, padding_size);
	}

	/* check whether dma addr is in local node */
	if (is_pswiotlb_active(dev)) {
		if (dir != DMA_TO_DEVICE) {
			if (unlikely(!dma_is_in_local_node(dev, nid, phys, aligned_size))) {
				aligned_size = iova_align(iovad, size);
				phys = pswiotlb_tbl_map_single(dev, nid, phys, size,
							aligned_size, iova_mask(iovad),
							dir, attrs);
				if (phys == DMA_MAPPING_ERROR) {
					phys = page_to_phys(page) + offset;
					dev_warn_once(dev,
						"Failed to allocate memory from pswiotlb, fall back to non-local dma\n");
				}
			}
		}
	}

	if (!coherent && !(attrs & DMA_ATTR_SKIP_CPU_SYNC))
		arch_sync_dma_for_device(phys, size, dir);

	iova = __iommu_dma_map(dev, phys, size, prot, dma_mask);
	if (iova == DMA_MAPPING_ERROR && is_swiotlb_buffer(dev, phys))
		swiotlb_tbl_unmap_single(dev, phys, size, dir, attrs);
	if (iova == DMA_MAPPING_ERROR && is_pswiotlb_buffer(dev, nid, phys, &pool))
		pswiotlb_tbl_unmap_single(dev, nid, phys, 0, size, dir, attrs, pool);
	return iova;
}

void pswiotlb_iommu_dma_unmap_page(struct device *dev, dma_addr_t dma_handle,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	struct iommu_domain *domain = iommu_get_dma_domain(dev);
	phys_addr_t phys;
	int nid = dev->numa_node;
	struct p_io_tlb_pool *pool;

	phys = iommu_iova_to_phys(domain, dma_handle);
	if (WARN_ON(!phys))
		return;

	if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC) && !dev_is_dma_coherent(dev))
		arch_sync_dma_for_cpu(phys, size, dir);

	__iommu_dma_unmap(dev, dma_handle, size);

	if (unlikely(is_swiotlb_buffer(dev, phys)))
		swiotlb_tbl_unmap_single(dev, phys, size, dir, attrs);

	if (is_pswiotlb_active(dev) &&
		is_pswiotlb_buffer(dev, nid, phys, &pool))
		pswiotlb_tbl_unmap_single(dev, nid, phys, 0, size, dir, attrs, pool);
}

static void iommu_dma_unmap_page_sg(struct device *dev, dma_addr_t dma_handle,
		size_t offset, size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	struct iommu_domain *domain = iommu_get_dma_domain(dev);
	phys_addr_t phys;
	int nid = dev->numa_node;
	struct p_io_tlb_pool *pool;

	phys = iommu_iova_to_phys(domain, dma_handle);

	if (WARN_ON(!phys))
		return;

	if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC) && !dev_is_dma_coherent(dev))
		arch_sync_dma_for_cpu(phys, size, dir);

	if (is_pswiotlb_buffer(dev, nid, phys, &pool))
		pswiotlb_tbl_unmap_single(dev, nid, phys, offset, size, dir, attrs, pool);
}

/*
 * Prepare a successfully-mapped scatterlist to give back to the caller.
 *
 * At this point the segments are already laid out by pswiotlb_iommu_dma_map_sg() to
 * avoid individually crossing any boundaries, so we merely need to check a
 * segment's start address to avoid concatenating across one.
 */
static int __finalise_sg(struct device *dev, struct scatterlist *sg, int nents,
		dma_addr_t dma_addr)
{
	struct scatterlist *s, *cur = sg;
	unsigned long seg_mask = dma_get_seg_boundary(dev);
	unsigned int cur_len = 0, max_len = dma_get_max_seg_size(dev);
	int i, count = 0;

	for_each_sg(sg, s, nents, i) {
		/* Restore this segment's original unaligned fields first */
		dma_addr_t s_dma_addr = sg_dma_address(s);
		unsigned int s_iova_off = sg_dma_address(s);
		unsigned int s_length = sg_dma_len(s);
		unsigned int s_iova_len = s->length;

		sg_dma_address(s) = DMA_MAPPING_ERROR;
		sg_dma_len(s) = 0;

		if (sg_dma_is_bus_address(s)) {
			if (i > 0)
				cur = sg_next(cur);

			sg_dma_unmark_bus_address(s);
			sg_dma_address(cur) = s_dma_addr;
			sg_dma_len(cur) = s_length;
			sg_dma_mark_bus_address(cur);
			count++;
			cur_len = 0;
			continue;
		}

		s->offset += s_iova_off;
		s->length = s_length;

		/*
		 * Now fill in the real DMA data. If...
		 * - there is a valid output segment to append to
		 * - and this segment starts on an IOVA page boundary
		 * - but doesn't fall at a segment boundary
		 * - and wouldn't make the resulting output segment too long
		 */
		if (cur_len && !s_iova_off && (dma_addr & seg_mask) &&
		    (max_len - cur_len >= s_length)) {
			/* ...then concatenate it with the previous one */
			cur_len += s_length;
		} else {
			/* Otherwise start the next output segment */
			if (i > 0)
				cur = sg_next(cur);
			cur_len = s_length;
			count++;

			sg_dma_address(cur) = dma_addr + s_iova_off;
		}

		sg_dma_len(cur) = cur_len;
		dma_addr += s_iova_len;

		if (s_length + s_iova_off < s_iova_len)
			cur_len = 0;
	}
	return count;
}

/*
 * If mapping failed, then just restore the original list,
 * but making sure the DMA fields are invalidated.
 */
static void __invalidate_sg(struct scatterlist *sg, int nents)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i) {
		if (sg_dma_is_bus_address(s)) {
			sg_dma_unmark_bus_address(s);
		} else {
			if (sg_dma_address(s) != DMA_MAPPING_ERROR)
				s->offset += sg_dma_address(s);
			if (sg_dma_len(s))
				s->length = sg_dma_len(s);
		}
		sg_dma_address(s) = DMA_MAPPING_ERROR;
		sg_dma_len(s) = 0;
	}
}

static void iommu_dma_unmap_sg_pswiotlb_pagesize(struct device *dev, struct scatterlist *sg,
		int nents, enum dma_data_direction dir, unsigned long attrs)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i)
		pswiotlb_iommu_dma_unmap_page(dev, sg_dma_address(s),
				sg_dma_len(s), dir, attrs);
}

void iommu_dma_unmap_sg_pswiotlb(struct device *dev, struct scatterlist *sg,
		unsigned long iova_start, size_t mapped, int nents,
		enum dma_data_direction dir, unsigned long attrs)
{
	dma_addr_t start, start_orig;
	struct scatterlist *s;
	struct scatterlist *sg_orig = sg;
	int i;

	start = iova_start;
	start_orig = start;
	for_each_sg(sg_orig, s, nents, i) {
		if (!mapped || (start_orig > (start + mapped)))
			break;
		if (s->length == 0)
			break;
		iommu_dma_unmap_page_sg(dev, start_orig, 0,
				s->length, dir, attrs);
		start_orig += s->length;
	}
}

static int iommu_dma_map_sg_pswiotlb_pagesize(struct device *dev, struct scatterlist *sg,
		int nents, enum dma_data_direction dir, unsigned long attrs)
{
	struct scatterlist *s;
	int i;

	for_each_sg(sg, s, nents, i) {
		sg_dma_address(s) = pswiotlb_iommu_dma_map_page(dev, sg_page(s),
				s->offset, s->length, dir, attrs);
		if (sg_dma_address(s) == DMA_MAPPING_ERROR)
			goto out_unmap;
		sg_dma_len(s) = s->length;
	}

	return nents;

out_unmap:
	iommu_dma_unmap_sg_pswiotlb_pagesize(dev, sg, i, dir, attrs | DMA_ATTR_SKIP_CPU_SYNC);
	return -EIO;
}

/*
 * The DMA API client is passing in a scatterlist which could describe
 * any old buffer layout, but the IOMMU API requires everything to be
 * aligned to IOMMU pages. Hence the need for this complicated bit of
 * impedance-matching, to be able to hand off a suitably-aligned list,
 * but still preserve the original offsets and sizes for the caller.
 */
int pswiotlb_iommu_dma_map_sg(struct device *dev, struct scatterlist *sg,
		int nents, enum dma_data_direction dir, unsigned long attrs)
{
	struct iommu_domain *domain = iommu_get_dma_domain(dev);
	struct iommu_dma_cookie *cookie = domain->iova_cookie;
	struct iova_domain *iovad = &cookie->iovad;
	struct scatterlist *s, *prev = NULL;
	int prot = dma_info_to_prot(dir, dev_is_dma_coherent(dev), attrs);
	struct pci_p2pdma_map_state p2pdma_state = {};
	enum pci_p2pdma_map_type map;
	dma_addr_t iova;
	size_t iova_len = 0;
	unsigned long mask = dma_get_seg_boundary(dev);
	ssize_t ret;
	int i;

	if (static_branch_unlikely(&iommu_deferred_attach_enabled)) {
		ret = iommu_deferred_attach(dev, domain);
		goto out;
	}

	if (dir != DMA_TO_DEVICE && is_pswiotlb_active(dev)
				&& ((nents == 1) && (sg->length < PAGE_SIZE)))
		return iommu_dma_map_sg_pswiotlb_pagesize(dev, sg, nents, dir, attrs);

	if ((dir == DMA_TO_DEVICE) && !(attrs & DMA_ATTR_SKIP_CPU_SYNC))
		pswiotlb_iommu_dma_sync_sg_for_device(dev, sg, nents, dir);

	/*
	 * Work out how much IOVA space we need, and align the segments to
	 * IOVA granules for the IOMMU driver to handle. With some clever
	 * trickery we can modify the list in-place, but reversibly, by
	 * stashing the unaligned parts in the as-yet-unused DMA fields.
	 */
	for_each_sg(sg, s, nents, i) {
		size_t s_iova_off = iova_offset(iovad, s->offset);
		size_t s_length = s->length;
		size_t pad_len = (mask - iova_len + 1) & mask;

		if (is_pci_p2pdma_page(sg_page(s))) {
			map = pci_p2pdma_map_segment(&p2pdma_state, dev, s);
			switch (map) {
			case PCI_P2PDMA_MAP_BUS_ADDR:
				/*
				 * iommu_map_sg() will skip this segment as
				 * it is marked as a bus address,
				 * __finalise_sg() will copy the dma address
				 * into the output segment.
				 */
				continue;
			case PCI_P2PDMA_MAP_THRU_HOST_BRIDGE:
				/*
				 * Mapping through host bridge should be
				 * mapped with regular IOVAs, thus we
				 * do nothing here and continue below.
				 */
				break;
			default:
				ret = -EREMOTEIO;
				goto out_restore_sg;
			}
		}

		sg_dma_address(s) = s_iova_off;
		sg_dma_len(s) = s_length;
		s->offset -= s_iova_off;
		s_length = iova_align(iovad, s_length + s_iova_off);
		s->length = s_length;

		/*
		 * Due to the alignment of our single IOVA allocation, we can
		 * depend on these assumptions about the segment boundary mask:
		 * - If mask size >= IOVA size, then the IOVA range cannot
		 *   possibly fall across a boundary, so we don't care.
		 * - If mask size < IOVA size, then the IOVA range must start
		 *   exactly on a boundary, therefore we can lay things out
		 *   based purely on segment lengths without needing to know
		 *   the actual addresses beforehand.
		 * - The mask must be a power of 2, so pad_len == 0 if
		 *   iova_len == 0, thus we cannot dereference prev the first
		 *   time through here (i.e. before it has a meaningful value).
		 */
		if (pad_len && pad_len < s_length - 1) {
			prev->length += pad_len;
			iova_len += pad_len;
		}

		iova_len += s_length;
		prev = s;
	}

	if (!iova_len)
		return __finalise_sg(dev, sg, nents, 0);

	iova = iommu_dma_alloc_iova(domain, iova_len, dma_get_mask(dev), dev);
	if (!iova) {
		ret = -ENOMEM;
		goto out_restore_sg;
	}

	/*
	 * We'll leave any physical concatenation to the IOMMU driver's
	 * implementation - it knows better than we do.
	 */
	if (dir != DMA_TO_DEVICE && is_pswiotlb_active(dev))
		ret = pswiotlb_iommu_map_sg_atomic_dma(dev, domain,
					iova, sg, nents, prot, dir, attrs);
	else
		ret = iommu_map_sg(domain, iova, sg, nents, prot, GFP_ATOMIC);

	if (ret < 0 || ret < iova_len)
		goto out_free_iova;

	return __finalise_sg(dev, sg, nents, iova);

out_free_iova:
	iommu_dma_free_iova(cookie, iova, iova_len, NULL);
out_restore_sg:
	__invalidate_sg(sg, nents);
out:
	if (ret != -ENOMEM && ret != -EREMOTEIO)
		return -EINVAL;
	return ret;
}

void pswiotlb_iommu_dma_unmap_sg(struct device *dev, struct scatterlist *sg,
		int nents, enum dma_data_direction dir, unsigned long attrs)
{
	dma_addr_t start, end = 0, start_orig;
	struct scatterlist *tmp, *s;
	struct scatterlist *sg_orig = sg;
	int i, j;
	struct iommu_domain *domain = iommu_get_dma_domain(dev);
	struct iommu_dma_cookie *cookie = domain->iova_cookie;
	struct iova_domain *iovad = &cookie->iovad;

	if ((dir != DMA_TO_DEVICE) && ((nents == 1) && (sg->length < PAGE_SIZE))) {
		iommu_dma_unmap_sg_pswiotlb_pagesize(dev, sg, nents, dir, attrs);
		return;
	}

	if ((dir == DMA_TO_DEVICE) && !(attrs & DMA_ATTR_SKIP_CPU_SYNC))
		pswiotlb_iommu_dma_sync_sg_for_cpu(dev, sg, nents, dir);

	/*
	 * The scatterlist segments are mapped into a single
	 * contiguous IOVA allocation, the start and end points
	 * just have to be determined.
	 */
	for_each_sg(sg, tmp, nents, i) {
		if (sg_dma_is_bus_address(tmp)) {
			sg_dma_unmark_bus_address(tmp);
			continue;
		}

		if (sg_dma_len(tmp) == 0)
			break;

		start = sg_dma_address(tmp);
		break;
	}

	if (is_pswiotlb_active(dev)) {
		/* check whether dma addr is in local node */
		start_orig = start;
		if (dir != DMA_TO_DEVICE) {
			for_each_sg(sg_orig, s, nents, j) {
				unsigned int s_iova_off = iova_offset(iovad, s->offset);

				if (j > 0)
					start_orig += s_iova_off;
				iommu_dma_unmap_page_sg(dev, start_orig,
						s_iova_off, s->length,
						dir, attrs);
				start_orig -= s_iova_off;
				start_orig += iova_align(iovad, s->length + s_iova_off);
			}
		}
	}

	nents -= i;
	for_each_sg(tmp, tmp, nents, i) {
		if (sg_dma_is_bus_address(tmp)) {
			sg_dma_unmark_bus_address(tmp);
			continue;
		}

		if (sg_dma_len(tmp) == 0)
			break;

		end = sg_dma_address(tmp) + sg_dma_len(tmp);
	}

	if (end)
		__iommu_dma_unmap(dev, start, end - start);
}

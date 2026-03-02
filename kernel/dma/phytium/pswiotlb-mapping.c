// SPDX-License-Identifier: GPL-2.0
/*
 * Auxiliary DMA operations used by arch-independent dma-mapping
 * routines when Phytium software IO tlb is required.
 *
 * Copyright (c) 2024, Phytium Technology Co., Ltd.
 */
#include <linux/memblock.h> /* for max_pfn */
#include <linux/acpi.h>
#include <linux/dma-map-ops.h>
#include <linux/export.h>
#include <linux/gfp.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/kmsan.h>
#include <linux/iova.h>
#include "../debug.h"
#include "../direct.h"
#include "pswiotlb-dma.h"

/*
 * The following functions are ported from
 * ./drivers/dma/mapping.c
 * static bool dma_go_direct(struct device *dev, dma_addr_t mask,
 *		const struct dma_map_ops *ops);
 * static inline bool dma_alloc_direct(struct device *dev,
 *		const struct dma_map_ops *ops);
 * static inline bool dma_map_direct(struct device *dev,
 *		const struct dma_map_ops *ops);
 * static struct page *__dma_alloc_pages(struct device *dev, size_t size,
 *		dma_addr_t *dma_handle, enum dma_data_direction dir, gfp_t gfp);
 * static struct sg_table *alloc_single_sgt(struct device *dev, size_t size,
 *		enum dma_data_direction dir, gfp_t gfp);
 * static void __dma_free_pages(struct device *dev, size_t size, struct page *page,
 *		dma_addr_t dma_handle, enum dma_data_direction dir);
 * static void free_single_sgt(struct device *dev, size_t size,
 *		struct sg_table *sgt, enum dma_data_direction dir);
 */

static bool dma_go_direct(struct device *dev, dma_addr_t mask,
		const struct dma_map_ops *ops)
{
	if (likely(!ops))
		return true;
#ifdef CONFIG_DMA_OPS_BYPASS
	if (dev->dma_ops_bypass)
		return min_not_zero(mask, dev->bus_dma_limit) >=
			    dma_direct_get_required_mask(dev);
#endif
	return false;
}

static inline bool dma_alloc_direct(struct device *dev,
		const struct dma_map_ops *ops)
{
	return dma_go_direct(dev, dev->coherent_dma_mask, ops);
}

static inline bool dma_map_direct(struct device *dev,
		const struct dma_map_ops *ops)
{
	return dma_go_direct(dev, *dev->dma_mask, ops);
}

dma_addr_t pswiotlb_dma_map_page_attrs_distribute(struct device *dev, struct page *page,
			size_t offset, size_t size, enum dma_data_direction dir,
			unsigned long attrs)
{
	const struct dma_map_ops *ops = dev->orig_dma_ops;
	dma_addr_t addr;

	if (dma_map_direct(dev, ops) ||
	    arch_dma_map_page_direct(dev, page_to_phys(page) + offset + size)) {
		if (!pswiotlb_bypass_is_needed(dev, 0, dir))
			addr = pswiotlb_dma_direct_map_page(dev, page, offset, size, dir, attrs);
		else
			addr = dma_direct_map_page(dev, page, offset, size, dir, attrs);
	} else {
		if (!pswiotlb_bypass_is_needed(dev, 0, dir))
			addr = pswiotlb_iommu_dma_map_page(dev, page, offset, size, dir, attrs);
		else
			addr = ops->map_page(dev, page, offset, size, dir, attrs);
	}
	kmsan_handle_dma(page, offset, size, dir);
	debug_dma_map_page(dev, page, offset, size, dir, addr, attrs);

	return addr;
}

void pswiotlb_dma_unmap_page_attrs_distribute(struct device *dev, dma_addr_t addr, size_t size,
		enum dma_data_direction dir, unsigned long attrs)
{
	const struct dma_map_ops *ops = dev->orig_dma_ops;

	if (dma_map_direct(dev, ops) ||
	    arch_dma_unmap_page_direct(dev, addr + size))
		pswiotlb_dma_direct_unmap_page(dev, addr, size, dir, attrs);
	else if (ops->unmap_page)
		pswiotlb_iommu_dma_unmap_page(dev, addr, size, dir, attrs);
	debug_dma_unmap_page(dev, addr, size, dir);
}

int pswiotlb_dma_map_sg_attrs_distribute(struct device *dev, struct scatterlist *sg,
	 int nents, enum dma_data_direction dir, unsigned long attrs)
{
	const struct dma_map_ops *ops = dev->orig_dma_ops;
	int ents;

	if (dma_map_direct(dev, ops) ||
	    arch_dma_map_sg_direct(dev, sg, nents)) {
		if (!pswiotlb_bypass_is_needed(dev, nents, dir))
			ents = pswiotlb_dma_direct_map_sg(dev, sg, nents, dir, attrs);
		else
			ents = dma_direct_map_sg(dev, sg, nents, dir, attrs);
	} else {
		if (!pswiotlb_bypass_is_needed(dev, nents, dir))
			ents = pswiotlb_iommu_dma_map_sg(dev, sg, nents, dir, attrs);
		else
			ents = ops->map_sg(dev, sg, nents, dir, attrs);
	}

	if (ents > 0)
		debug_dma_map_sg(dev, sg, nents, ents, dir, attrs);
	else if (WARN_ON_ONCE(ents != -EINVAL && ents != -ENOMEM &&
			      ents != -EIO))
		return -EIO;

	return ents;
}

void pswiotlb_dma_unmap_sg_attrs_distribute(struct device *dev, struct scatterlist *sg,
				      int nents, enum dma_data_direction dir,
				      unsigned long attrs)
{
	const struct dma_map_ops *ops = dev->orig_dma_ops;

	if (dma_map_direct(dev, ops) ||
	    arch_dma_unmap_sg_direct(dev, sg, nents))
		pswiotlb_dma_direct_unmap_sg(dev, sg, nents, dir, attrs);
	else if (ops->unmap_sg)
		pswiotlb_iommu_dma_unmap_sg(dev, sg, nents, dir, attrs);
}

void pswiotlb_dma_sync_single_for_cpu_distribute(struct device *dev, dma_addr_t addr, size_t size,
		enum dma_data_direction dir)
{
	const struct dma_map_ops *ops = dev->orig_dma_ops;

	if (dma_map_direct(dev, ops))
		pswiotlb_dma_direct_sync_single_for_cpu(dev, addr, size, dir);
	else if (ops->sync_single_for_cpu)
		pswiotlb_iommu_dma_sync_single_for_cpu(dev, addr, size, dir);
	debug_dma_sync_single_for_cpu(dev, addr, size, dir);
}

void pswiotlb_dma_sync_single_for_device_distribute(struct device *dev, dma_addr_t addr,
		size_t size, enum dma_data_direction dir)
{
	const struct dma_map_ops *ops = dev->orig_dma_ops;

	if (dma_map_direct(dev, ops))
		pswiotlb_dma_direct_sync_single_for_device(dev, addr, size, dir);
	else if (ops->sync_single_for_device)
		pswiotlb_iommu_dma_sync_single_for_device(dev, addr, size, dir);
	debug_dma_sync_single_for_device(dev, addr, size, dir);
}

void pswiotlb_dma_sync_sg_for_cpu_distribute(struct device *dev, struct scatterlist *sg,
		    int nelems, enum dma_data_direction dir)
{
	const struct dma_map_ops *ops = dev->orig_dma_ops;

	if (dma_map_direct(dev, ops))
		pswiotlb_dma_direct_sync_sg_for_cpu(dev, sg, nelems, dir);
	else if (ops->sync_sg_for_cpu)
		pswiotlb_iommu_dma_sync_sg_for_cpu(dev, sg, nelems, dir);
	debug_dma_sync_sg_for_cpu(dev, sg, nelems, dir);
}

void pswiotlb_dma_sync_sg_for_device_distribute(struct device *dev, struct scatterlist *sg,
		       int nelems, enum dma_data_direction dir)
{
	const struct dma_map_ops *ops = dev->orig_dma_ops;

	if (dma_map_direct(dev, ops))
		pswiotlb_dma_direct_sync_sg_for_device(dev, sg, nelems, dir);
	else if (ops->sync_sg_for_device)
		pswiotlb_iommu_dma_sync_sg_for_device(dev, sg, nelems, dir);
	debug_dma_sync_sg_for_device(dev, sg, nelems, dir);
}

static dma_addr_t pswiotlb_dma_map_resource_distribute(struct device *dev, phys_addr_t phys,
		size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	return dma_direct_map_resource(dev, phys, size, dir, attrs);
}

static struct page *__dma_alloc_pages(struct device *dev, size_t size,
		dma_addr_t *dma_handle, enum dma_data_direction dir, gfp_t gfp)
{
	const struct dma_map_ops *ops = get_dma_ops(dev);

	if (WARN_ON_ONCE(!dev->coherent_dma_mask))
		return NULL;
	if (WARN_ON_ONCE(gfp & (__GFP_DMA | __GFP_DMA32 | __GFP_HIGHMEM)))
		return NULL;
	if (WARN_ON_ONCE(gfp & __GFP_COMP))
		return NULL;

	size = PAGE_ALIGN(size);
	if (dma_alloc_direct(dev, ops))
		return dma_direct_alloc_pages(dev, size, dma_handle, dir, gfp);
	if (!ops->alloc_pages)
		return NULL;
	return ops->alloc_pages(dev, size, dma_handle, dir, gfp);
}

static struct sg_table *alloc_single_sgt(struct device *dev, size_t size,
		enum dma_data_direction dir, gfp_t gfp)
{
	struct sg_table *sgt;
	struct page *page;

	sgt = kmalloc(sizeof(*sgt), gfp);
	if (!sgt)
		return NULL;
	if (sg_alloc_table(sgt, 1, gfp))
		goto out_free_sgt;
	page = __dma_alloc_pages(dev, size, &sgt->sgl->dma_address, dir, gfp);
	if (!page)
		goto out_free_table;
	sg_set_page(sgt->sgl, page, PAGE_ALIGN(size), 0);
	sg_dma_len(sgt->sgl) = sgt->sgl->length;
	return sgt;
out_free_table:
	sg_free_table(sgt);
out_free_sgt:
	kfree(sgt);
	return NULL;
}

static struct sg_table *pswiotlb_dma_alloc_noncontiguous_distribute(struct device *dev,
		size_t size, enum dma_data_direction dir, gfp_t gfp,
		unsigned long attrs)
{
	return alloc_single_sgt(dev, size, dir, gfp);
}

static void __dma_free_pages(struct device *dev, size_t size, struct page *page,
		dma_addr_t dma_handle, enum dma_data_direction dir)
{
	const struct dma_map_ops *ops = get_dma_ops(dev);

	size = PAGE_ALIGN(size);
	if (dma_alloc_direct(dev, ops))
		dma_direct_free_pages(dev, size, page, dma_handle, dir);
	else if (ops->free_pages)
		ops->free_pages(dev, size, page, dma_handle, dir);
}

static void free_single_sgt(struct device *dev, size_t size,
		struct sg_table *sgt, enum dma_data_direction dir)
{
	__dma_free_pages(dev, size, sg_page(sgt->sgl), sgt->sgl->dma_address,
			 dir);
	sg_free_table(sgt);
	kfree(sgt);
}

static void pswiotlb_dma_free_noncontiguous_distribute(struct device *dev, size_t size,
		struct sg_table *sgt, enum dma_data_direction dir)
{
	free_single_sgt(dev, size, sgt, dir);
}

static int pswiotlb_dma_get_sgtable_distribute(struct device *dev, struct sg_table *sgt,
		void *cpu_addr, dma_addr_t dma_addr, size_t size,
		unsigned long attrs)
{
	return dma_direct_get_sgtable(dev, sgt, cpu_addr, dma_addr,
				size, attrs);
}

static int pswiotlb_dma_mmap_distribute(struct device *dev, struct vm_area_struct *vma,
		void *cpu_addr, dma_addr_t dma_addr, size_t size,
		unsigned long attrs)
{
	return dma_direct_mmap(dev, vma, cpu_addr, dma_addr, size,
			attrs);
}

static u64 pswiotlb_dma_get_required_mask_distribute(struct device *dev)
{
	return dma_direct_get_required_mask(dev);
}

static void *pswiotlb_dma_alloc_distribute(struct device *dev, size_t size,
		dma_addr_t *handle, gfp_t gfp, unsigned long attrs)
{
	const struct dma_map_ops *ops = dev->orig_dma_ops;
	void *cpu_addr;

	check_if_pswiotlb_is_applicable(dev);

	if (dma_alloc_direct(dev, ops))
		cpu_addr = dma_direct_alloc(dev, size, handle, gfp, attrs);
	else
		cpu_addr = ops->alloc(dev, size, handle, gfp, attrs);

	return cpu_addr;
}

static void pswiotlb_dma_free_distribute(struct device *dev, size_t size,
		void *cpu_addr, dma_addr_t handle, unsigned long attrs)
{
	const struct dma_map_ops *ops = dev->orig_dma_ops;

	if (dma_alloc_direct(dev, ops))
		dma_direct_free(dev, size, cpu_addr, handle, attrs);
	else
		ops->free(dev, size, cpu_addr, handle, attrs);
}

static struct page *pswiotlb_dma_common_alloc_pages_distribute(struct device *dev, size_t size,
		dma_addr_t *dma_handle, enum dma_data_direction dir, gfp_t gfp)
{
	return dma_direct_alloc_pages(dev, size, dma_handle, dir, gfp);
}

static void pswiotlb_dma_common_free_pages_distribute(struct device *dev, size_t size,
		struct page *page, dma_addr_t dma_handle, enum dma_data_direction dir)
{
	dma_direct_free_pages(dev, size, page, dma_handle, dir);
}

static int pswiotlb_dma_supported_distribute(struct device *dev, u64 mask)
{
	return dma_direct_supported(dev, mask);
}

static size_t pswiotlb_dma_max_mapping_size_distribute(struct device *dev)
{
	const struct dma_map_ops *ops = dev->orig_dma_ops;

	if (dma_map_direct(dev, ops))
		return dma_direct_max_mapping_size(dev);
	else
		return SIZE_MAX;
}

static size_t pswiotlb_iommu_dma_opt_mapping_size(void)
{
	if (iommu_default_passthrough())
		return SIZE_MAX;
	else
		return iova_rcache_range();
}

static size_t pswiotlb_dma_opt_mapping_size_distribute(void)
{
	size_t size;

	size = pswiotlb_iommu_dma_opt_mapping_size();

	return min(SIZE_MAX, size);
}

static unsigned long pswiotlb_dma_get_merge_boundary_distribute(struct device *dev)
{
	return 0;	/* can't merge */
}

static const struct dma_map_ops pswiotlb_noiommu_dma_ops = {
	.flags			= DMA_F_PCI_P2PDMA_SUPPORTED,
	.alloc			= pswiotlb_dma_alloc_distribute,
	.free			= pswiotlb_dma_free_distribute,
	.alloc_pages		= pswiotlb_dma_common_alloc_pages_distribute,
	.free_pages		= pswiotlb_dma_common_free_pages_distribute,
	.alloc_noncontiguous	= pswiotlb_dma_alloc_noncontiguous_distribute,
	.free_noncontiguous	= pswiotlb_dma_free_noncontiguous_distribute,
	.mmap			= pswiotlb_dma_mmap_distribute,
	.get_sgtable		= pswiotlb_dma_get_sgtable_distribute,
	.map_page		= pswiotlb_dma_map_page_attrs_distribute,
	.unmap_page		= pswiotlb_dma_unmap_page_attrs_distribute,
	.map_sg			= pswiotlb_dma_map_sg_attrs_distribute,
	.unmap_sg		= pswiotlb_dma_unmap_sg_attrs_distribute,
	.sync_single_for_cpu	= pswiotlb_dma_sync_single_for_cpu_distribute,
	.sync_single_for_device	= pswiotlb_dma_sync_single_for_device_distribute,
	.sync_sg_for_cpu	= pswiotlb_dma_sync_sg_for_cpu_distribute,
	.sync_sg_for_device	= pswiotlb_dma_sync_sg_for_device_distribute,
	.map_resource		= pswiotlb_dma_map_resource_distribute,
	.unmap_resource		= NULL,
	.get_merge_boundary	= pswiotlb_dma_get_merge_boundary_distribute,
	.get_required_mask  = pswiotlb_dma_get_required_mask_distribute,
	.dma_supported = pswiotlb_dma_supported_distribute,
	.max_mapping_size   = pswiotlb_dma_max_mapping_size_distribute,
	.opt_mapping_size   = pswiotlb_dma_opt_mapping_size_distribute,
};
struct pswiotlb_dma_map_ops *pswiotlb_clone_orig_dma_ops(struct device *dev,
			const struct dma_map_ops *ops)
{
	struct pswiotlb_dma_map_ops *new_dma_ops = kmalloc(sizeof(struct pswiotlb_dma_map_ops),
				GFP_KERNEL);
	if (!new_dma_ops)
		return NULL;

	memcpy(new_dma_ops, ops, sizeof(struct pswiotlb_dma_map_ops));

	return new_dma_ops;
}

void pswiotlb_setup_dma_ops(struct device *dev)
{
	const struct dma_map_ops *orig_ops = get_dma_ops(dev);
	struct pswiotlb_dma_map_ops *new_ops;
	struct pci_dev *pdev;

	if (dev && dev_is_pci(dev) && (pswiotlb_force_disable != true) &&
			is_phytium_ps_socs()) {
		pdev = to_pci_dev(dev);
		pdev->dev.can_use_pswiotlb = pswiotlb_is_dev_in_passthroughlist(pdev);
		dev_info(&pdev->dev, "The device %s use pswiotlb because vendor 0x%04x %s in pswiotlb passthroughlist\n",
					pdev->dev.can_use_pswiotlb ? "would" : "would NOT",
					pdev->vendor, pdev->dev.can_use_pswiotlb ? "is NOT" : "is");
	}

	if (check_if_pswiotlb_is_applicable(dev)) {
		if (!orig_ops)
			set_dma_ops(dev, &pswiotlb_noiommu_dma_ops);
		else {
			new_ops = pswiotlb_clone_orig_dma_ops(dev, orig_ops);
			if (!new_ops) {
				dev_warn(dev, "Failed to clone dma ops, pswiotlb is NOT applicable\n");
				return;
			}

			dev->orig_dma_ops = get_dma_ops(dev);
			new_ops->alloc		= pswiotlb_dma_alloc_distribute;
			new_ops->map_page	= pswiotlb_dma_map_page_attrs_distribute;
			new_ops->unmap_page	= pswiotlb_dma_unmap_page_attrs_distribute;
			new_ops->map_sg		= pswiotlb_dma_map_sg_attrs_distribute;
			new_ops->unmap_sg	= pswiotlb_dma_unmap_sg_attrs_distribute;
			new_ops->sync_single_for_cpu =
				pswiotlb_dma_sync_single_for_cpu_distribute;
			new_ops->sync_single_for_device	=
				pswiotlb_dma_sync_single_for_device_distribute;
			new_ops->sync_sg_for_cpu =
				pswiotlb_dma_sync_sg_for_cpu_distribute;
			new_ops->sync_sg_for_device	=
				pswiotlb_dma_sync_sg_for_device_distribute;
			new_ops->max_mapping_size =
				pswiotlb_dma_max_mapping_size_distribute;
			new_ops->opt_mapping_size =
				pswiotlb_dma_opt_mapping_size_distribute;

			set_dma_ops(dev, (const struct dma_map_ops *)new_ops);
		}
	}
}

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
#include "../debug.h"
#include "../direct.h"
#include "pswiotlb-dma.h"

/*
 * The following functions are ported from
 * ./drivers/dma/mapping.c
 * static bool dma_go_direct(struct device *dev, dma_addr_t mask,
 *		const struct dma_map_ops *ops);
 * static inline bool dma_map_direct(struct device *dev,
 *		const struct dma_map_ops *ops);
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

static inline bool dma_map_direct(struct device *dev,
		const struct dma_map_ops *ops)
{
	return dma_go_direct(dev, *dev->dma_mask, ops);
}
dma_addr_t pswiotlb_dma_map_page_distribute(struct device *dev, struct page *page,
			size_t offset, size_t size, enum dma_data_direction dir,
			unsigned long attrs)
{
	const struct dma_map_ops *ops = get_dma_ops(dev);
	dma_addr_t addr;

	if (dma_map_direct(dev, ops) ||
	    arch_dma_map_page_direct(dev, page_to_phys(page) + offset + size))
		addr = pswiotlb_dma_direct_map_page(dev, page, offset, size, dir, attrs);
	else
		addr = pswiotlb_iommu_dma_map_page(dev, page, offset, size, dir, attrs);
	debug_dma_map_page(dev, page, offset, size, dir, addr, attrs);

	return addr;
}

void pswiotlb_dma_unmap_page_attrs_distribute(struct device *dev, dma_addr_t addr, size_t size,
		enum dma_data_direction dir, unsigned long attrs)
{
	const struct dma_map_ops *ops = get_dma_ops(dev);

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
	const struct dma_map_ops *ops = get_dma_ops(dev);
	int ents;

	if (dma_map_direct(dev, ops) ||
	    arch_dma_map_sg_direct(dev, sg, nents))
		ents = pswiotlb_dma_direct_map_sg(dev, sg, nents, dir, attrs);
	else
		ents = pswiotlb_iommu_dma_map_sg(dev, sg, nents, dir, attrs);

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
	const struct dma_map_ops *ops = get_dma_ops(dev);

	if (dma_map_direct(dev, ops) ||
	    arch_dma_unmap_sg_direct(dev, sg, nents))
		pswiotlb_dma_direct_unmap_sg(dev, sg, nents, dir, attrs);
	else if (ops->unmap_sg)
		pswiotlb_iommu_dma_unmap_sg(dev, sg, nents, dir, attrs);
}

void pswiotlb_dma_sync_single_for_cpu_distribute(struct device *dev, dma_addr_t addr, size_t size,
		enum dma_data_direction dir)
{
	const struct dma_map_ops *ops = get_dma_ops(dev);

	if (dma_map_direct(dev, ops))
		pswiotlb_dma_direct_sync_single_for_cpu(dev, addr, size, dir);
	else if (ops->sync_single_for_cpu)
		pswiotlb_iommu_dma_sync_single_for_cpu(dev, addr, size, dir);
	debug_dma_sync_single_for_cpu(dev, addr, size, dir);
}

void pswiotlb_dma_sync_single_for_device_distribute(struct device *dev, dma_addr_t addr,
		size_t size, enum dma_data_direction dir)
{
	const struct dma_map_ops *ops = get_dma_ops(dev);

	if (dma_map_direct(dev, ops))
		pswiotlb_dma_direct_sync_single_for_device(dev, addr, size, dir);
	else if (ops->sync_single_for_device)
		pswiotlb_iommu_dma_sync_single_for_device(dev, addr, size, dir);
	debug_dma_sync_single_for_device(dev, addr, size, dir);
}

void pswiotlb_dma_sync_sg_for_cpu_distribute(struct device *dev, struct scatterlist *sg,
		    int nelems, enum dma_data_direction dir)
{
	const struct dma_map_ops *ops = get_dma_ops(dev);

	if (dma_map_direct(dev, ops))
		pswiotlb_dma_direct_sync_sg_for_cpu(dev, sg, nelems, dir);
	else if (ops->sync_sg_for_cpu)
		pswiotlb_iommu_dma_sync_sg_for_cpu(dev, sg, nelems, dir);
	debug_dma_sync_sg_for_cpu(dev, sg, nelems, dir);
}

void pswiotlb_dma_sync_sg_for_device_distribute(struct device *dev, struct scatterlist *sg,
		       int nelems, enum dma_data_direction dir)
{
	const struct dma_map_ops *ops = get_dma_ops(dev);

	if (dma_map_direct(dev, ops))
		pswiotlb_dma_direct_sync_sg_for_device(dev, sg, nelems, dir);
	else if (ops->sync_sg_for_device)
		pswiotlb_iommu_dma_sync_sg_for_device(dev, sg, nelems, dir);
	debug_dma_sync_sg_for_device(dev, sg, nelems, dir);
}

// SPDX-License-Identifier: GPL-2.0
/*
 * DMA operations based on Phytium software IO tlb that
 * map physical memory directly without using an IOMMU.
 *
 * Copyright (c) 2024, Phytium Technology Co., Ltd.
 */
#include <linux/memblock.h> /* for max_pfn */
#include <linux/export.h>
#include <linux/mm.h>
#include <linux/dma-map-ops.h>
#include <linux/scatterlist.h>
#include <linux/pfn.h>
#include <linux/vmalloc.h>
#include <linux/set_memory.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/pswiotlb.h>
#include "pswiotlb-dma.h"

/*
 * The following functions are ported from
 * ./drivers/dma/direct.c
 * static inline dma_addr_t phys_to_dma_direct(struct device *dev,
 *		phys_addr_t phys);
 */

static inline dma_addr_t phys_to_dma_direct(struct device *dev,
		phys_addr_t phys)
{
	if (force_dma_unencrypted(dev))
		return phys_to_dma_unencrypted(dev, phys);
	return phys_to_dma(dev, phys);
}

bool pswiotlb_dma_coherent_ok(struct device *dev, phys_addr_t phys, size_t size)
{
	dma_addr_t dma_addr = phys_to_dma_direct(dev, phys);

	if (dma_addr == DMA_MAPPING_ERROR)
		return false;
	return dma_addr + size - 1 <=
		min_not_zero(dev->coherent_dma_mask, dev->bus_dma_limit);
}

#if defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_DEVICE) || \
	defined(CONFIG_PSWIOTLB)
void pswiotlb_dma_direct_sync_sg_for_device(struct device *dev,
		struct scatterlist *sgl, int nents, enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;
	int nid = dev->numa_node;
	struct p_io_tlb_pool *pool;

	for_each_sg(sgl, sg, nents, i) {
		phys_addr_t paddr = dma_to_phys(dev, sg_dma_address(sg));

		if (unlikely(is_swiotlb_buffer(dev, paddr)))
			swiotlb_sync_single_for_device(dev, paddr, sg->length,
						       dir);

		if (is_pswiotlb_active(dev) &&
			unlikely(is_pswiotlb_buffer(dev, nid, paddr, &pool)))
			pswiotlb_sync_single_for_device(dev, nid, paddr,
						sg->length, dir, pool);

		if (!dev_is_dma_coherent(dev))
			arch_sync_dma_for_device(paddr, sg->length,
					dir);
	}
}
#endif

#if defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_CPU) || \
	defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_CPU_ALL) || \
	defined(CONFIG_PSWIOTLB)
void pswiotlb_dma_direct_sync_sg_for_cpu(struct device *dev,
		struct scatterlist *sgl, int nents, enum dma_data_direction dir)
{
	struct scatterlist *sg;
	int i;
	int nid = dev->numa_node;
	struct p_io_tlb_pool *pool;

	for_each_sg(sgl, sg, nents, i) {
		phys_addr_t paddr = dma_to_phys(dev, sg_dma_address(sg));

		if (!dev_is_dma_coherent(dev))
			arch_sync_dma_for_cpu(paddr, sg->length, dir);

		if (unlikely(is_swiotlb_buffer(dev, paddr)))
			swiotlb_sync_single_for_cpu(dev, paddr, sg->length,
						    dir);

		if (is_pswiotlb_active(dev) &&
			unlikely(is_pswiotlb_buffer(dev, nid, paddr, &pool)))
			pswiotlb_sync_single_for_cpu(dev, nid, paddr,
						sg->length, dir, pool);

		if (dir == DMA_FROM_DEVICE)
			arch_dma_mark_clean(paddr, sg->length);
	}

	if (!dev_is_dma_coherent(dev))
		arch_sync_dma_for_cpu_all();
}

/*
 * Unmaps segments, except for ones marked as pci_p2pdma which do not
 * require any further action as they contain a bus address.
 */
void pswiotlb_dma_direct_unmap_sg(struct device *dev, struct scatterlist *sgl,
		int nents, enum dma_data_direction dir, unsigned long attrs)
{
	struct scatterlist *sg;
	int i;

	for_each_sg(sgl, sg, nents, i)
		pswiotlb_dma_direct_unmap_page(dev, sg->dma_address, sg_dma_len(sg), dir,
			     attrs);
}
#endif

int pswiotlb_dma_direct_map_sg(struct device *dev, struct scatterlist *sgl, int nents,
		enum dma_data_direction dir, unsigned long attrs)
{
	struct scatterlist *sg;
	int i, ret;

	for_each_sg(sgl, sg, nents, i) {
		sg->dma_address = pswiotlb_dma_direct_map_page(dev, sg_page(sg),
				sg->offset, sg->length, dir, attrs);
		if (sg->dma_address == DMA_MAPPING_ERROR) {
			ret = -EIO;
			goto out_unmap;
		}
		sg_dma_len(sg) = sg->length;
	}

	return nents;

out_unmap:
	pswiotlb_dma_direct_unmap_sg(dev, sgl, i, dir, attrs | DMA_ATTR_SKIP_CPU_SYNC);
	return ret;
}

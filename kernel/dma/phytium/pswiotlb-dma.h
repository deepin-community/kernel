/* SPDX-License-Identifier: GPL-2.0 */
/*
 * DMA operations based on Phytium software IO tlb that
 * map physical memory.
 *
 * Copyright (c) 2024, Phytium Technology Co., Ltd.
 */
#ifndef _KERNEL_PSWIOTLB_DMA_DIRECT_H
#define _KERNEL_PSWIOTLB_DMA_DIRECT_H

#include <linux/dma-direct.h>
#include <linux/iommu.h>
#include <linux/pswiotlb.h>

extern bool pswiotlb_force_disable;
#if defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_DEVICE) || \
	defined(CONFIG_PSWIOTLB)
void pswiotlb_dma_direct_sync_sg_for_device(struct device *dev,
			struct scatterlist *sgl, int nents, enum dma_data_direction dir);
#else
static inline void pswiotlb_dma_direct_sync_sg_for_device(struct device *dev,
		struct scatterlist *sgl, int nents, enum dma_data_direction dir)
{
}
#endif

#if defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_CPU) || \
	defined(CONFIG_ARCH_HAS_SYNC_DMA_FOR_CPU_ALL) || \
	defined(CONFIG_PSWIOTLB)
void pswiotlb_dma_direct_unmap_sg(struct device *dev, struct scatterlist *sgl,
		int nents, enum dma_data_direction dir, unsigned long attrs);
void pswiotlb_dma_direct_sync_sg_for_cpu(struct device *dev,
		struct scatterlist *sgl, int nents, enum dma_data_direction dir);
#else
static inline void pswiotlb_dma_direct_unmap_sg(struct device *dev,
		struct scatterlist *sgl, int nents, enum dma_data_direction dir,
		unsigned long attrs)
{
}
static inline void pswiotlb_dma_direct_sync_sg_for_cpu(struct device *dev,
		struct scatterlist *sgl, int nents, enum dma_data_direction dir)
{
}
#endif

#ifdef CONFIG_PSWIOTLB
int pswiotlb_dma_direct_map_sg(struct device *dev, struct scatterlist *sgl,
			int nents, enum dma_data_direction dir, unsigned long attrs);
dma_addr_t pswiotlb_dma_map_page_distribute(struct device *dev, struct page *page,
			size_t offset, size_t size, enum dma_data_direction dir,
			unsigned long attrs);
void pswiotlb_dma_unmap_page_attrs_distribute(struct device *dev, dma_addr_t addr,
			size_t size, enum dma_data_direction dir, unsigned long attrs);
int pswiotlb_dma_map_sg_attrs_distribute(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir, unsigned long attrs);
void pswiotlb_dma_unmap_sg_attrs_distribute(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir, unsigned long attrs);
void pswiotlb_dma_sync_single_for_cpu_distribute(struct device *dev, dma_addr_t addr,
			size_t size, enum dma_data_direction dir);
void pswiotlb_dma_sync_single_for_device_distribute(struct device *dev, dma_addr_t addr,
			size_t size, enum dma_data_direction dir);
void pswiotlb_dma_sync_sg_for_cpu_distribute(struct device *dev, struct scatterlist *sg,
		    int nelems, enum dma_data_direction dir);
void pswiotlb_dma_sync_sg_for_device_distribute(struct device *dev, struct scatterlist *sg,
			int nelems, enum dma_data_direction dir);
dma_addr_t pswiotlb_iommu_dma_map_page(struct device *dev, struct page *page,
			unsigned long offset, size_t size, enum dma_data_direction dir,
			unsigned long attrs);
void pswiotlb_iommu_dma_unmap_page(struct device *dev, dma_addr_t dma_handle,
			size_t size, enum dma_data_direction dir, unsigned long attrs);
int pswiotlb_iommu_dma_map_sg(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir, unsigned long attrs);
void pswiotlb_iommu_dma_unmap_sg(struct device *dev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir, unsigned long attrs);
void pswiotlb_iommu_dma_sync_single_for_cpu(struct device *dev,
			dma_addr_t dma_handle, size_t size, enum dma_data_direction dir);
void pswiotlb_iommu_dma_sync_single_for_device(struct device *dev,
			dma_addr_t dma_handle, size_t size, enum dma_data_direction dir);
void pswiotlb_iommu_dma_sync_sg_for_cpu(struct device *dev,
			struct scatterlist *sgl, int nelems, enum dma_data_direction dir);
void pswiotlb_iommu_dma_sync_sg_for_device(struct device *dev,
			struct scatterlist *sgl, int nelems, enum dma_data_direction dir);

static inline bool check_if_pswiotlb_is_applicable(struct device *dev)
{
	if (dev && dev->can_use_pswiotlb && is_phytium_ps_socs()
				&& !pswiotlb_force_disable) {
		if (dev->numa_node == NUMA_NO_NODE ||
			dev->numa_node != dev->local_node)
			dev->numa_node = dev->local_node;

		if (dev_is_pci(dev) && (dev->numa_node != NUMA_NO_NODE))
			return true;
	}

	return false;
}

static inline void pswiotlb_dma_direct_sync_single_for_device(struct device *dev,
			dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
	phys_addr_t paddr = dma_to_phys(dev, addr);
	int nid = dev->numa_node;
	struct p_io_tlb_pool *pool;

	if (unlikely(is_swiotlb_buffer(dev, paddr)))
		swiotlb_sync_single_for_device(dev, paddr, size, dir);

	if (is_pswiotlb_active(dev)) {
		if (unlikely(is_pswiotlb_buffer(dev, nid, paddr, &pool)))
			pswiotlb_sync_single_for_device(dev, nid, paddr, size, dir, pool);
	}

	if (!dev_is_dma_coherent(dev))
		arch_sync_dma_for_device(paddr, size, dir);
}

static inline void pswiotlb_dma_direct_sync_single_for_cpu(struct device *dev,
			dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
	phys_addr_t paddr = dma_to_phys(dev, addr);
	int nid = dev->numa_node;
	struct p_io_tlb_pool *pool;

	if (!dev_is_dma_coherent(dev)) {
		arch_sync_dma_for_cpu(paddr, size, dir);
		arch_sync_dma_for_cpu_all();
	}

	if (unlikely(is_swiotlb_buffer(dev, paddr)))
		swiotlb_sync_single_for_cpu(dev, paddr, size, dir);

	if (is_pswiotlb_active(dev)) {
		if (unlikely(is_pswiotlb_buffer(dev, nid, paddr, &pool)))
			pswiotlb_sync_single_for_cpu(dev, nid, paddr, size, dir, pool);
	}

	if (dir == DMA_FROM_DEVICE)
		arch_dma_mark_clean(paddr, size);
}

static inline dma_addr_t pswiotlb_dma_direct_map_page(struct device *dev,
			struct page *page, unsigned long offset, size_t size,
			enum dma_data_direction dir, unsigned long attrs)
{
	phys_addr_t phys = page_to_phys(page) + offset;
	dma_addr_t dma_addr = phys_to_dma(dev, phys);
	int nid = dev->numa_node;

	if (is_swiotlb_force_bounce(dev))
		return swiotlb_map(dev, phys, size, dir, attrs);

	if (unlikely(!dma_capable(dev, dma_addr, size, true)) ||
	    dma_kmalloc_needs_bounce(dev, size, dir)) {
		if (is_swiotlb_active(dev))
			return swiotlb_map(dev, phys, size, dir, attrs);

		dev_WARN_ONCE(dev, 1,
			     "DMA addr %pad+%zu overflow (mask %llx, bus limit %llx).\n",
			     &dma_addr, size, *dev->dma_mask, dev->bus_dma_limit);
		return DMA_MAPPING_ERROR;
	}

	/* check whether dma addr is in local node */
	if (is_pswiotlb_active(dev)) {
		if (dir != DMA_TO_DEVICE) {
			if (unlikely(!dma_is_in_local_node(dev, nid, dma_addr, size))) {
				dma_addr = pswiotlb_map(dev, nid, phys, size, dir, attrs);
				if (dma_addr == DMA_MAPPING_ERROR) {
					dma_addr = phys_to_dma(dev, phys);
					dev_warn_once(dev,
						"Failed to allocate memory from pswiotlb, fall back to non-local dma\n");
				} else
					return dma_addr;
			}
		}
	}

	if (!dev_is_dma_coherent(dev) && !(attrs & DMA_ATTR_SKIP_CPU_SYNC))
		arch_sync_dma_for_device(phys, size, dir);
	return dma_addr;
}

static inline void pswiotlb_dma_direct_unmap_page(struct device *dev, dma_addr_t addr,
			size_t size, enum dma_data_direction dir, unsigned long attrs)
{
	phys_addr_t phys = dma_to_phys(dev, addr);
	int nid = dev->numa_node;
	struct p_io_tlb_pool *pool;

	if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC) &&
		!dev_is_dma_coherent(dev)) {
		arch_sync_dma_for_cpu(phys, size, dir);
		arch_sync_dma_for_cpu_all();
	}

	if (unlikely(is_swiotlb_buffer(dev, phys)))
		swiotlb_tbl_unmap_single(dev, phys, size, dir, attrs);

	if (is_pswiotlb_active(dev)) {
		if (unlikely(is_pswiotlb_buffer(dev, nid, phys, &pool)))
			pswiotlb_tbl_unmap_single(dev, nid, phys, 0, size, dir, attrs, pool);

		if (!(attrs & DMA_ATTR_SKIP_CPU_SYNC) && (dir == DMA_FROM_DEVICE))
			arch_dma_mark_clean(phys, size);
	}
}
#else
static inline int pswiotlb_dma_direct_map_sg(struct device *dev, struct scatterlist *sgl,
			int nents, enum dma_data_direction dir, unsigned long attrs)
{
	return 0;
}

static inline dma_addr_t pswiotlb_dma_map_page_distribute(struct device *dev,
			struct page *page, size_t offset, size_t size, enum dma_data_direction dir,
			unsigned long attrs)
{
	return 0;
}

static inline void pswiotlb_dma_unmap_page_attrs_distribute(struct device *dev,
			dma_addr_t addr, size_t size, enum dma_data_direction dir,
			unsigned long attrs)
{
}

static inline int pswiotlb_dma_map_sg_attrs_distribute(struct device *dev,
			struct scatterlist *sg, int nents, enum dma_data_direction dir,
			unsigned long attrs)
{
	return 0;
}

static inline void pswiotlb_dma_unmap_sg_attrs_distribute(struct device *dev,
			struct scatterlist *sg, int nents, enum dma_data_direction dir,
			unsigned long attrs)
{
}

static inline void pswiotlb_dma_sync_single_for_cpu_distribute(struct device *dev,
			dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
}

static inline void pswiotlb_dma_sync_single_for_device_distribute(struct device *dev,
			dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
}

static inline void pswiotlb_dma_sync_sg_for_cpu_distribute(struct device *dev,
			struct scatterlist *sg, int nelems, enum dma_data_direction dir)
{
}

static inline void pswiotlb_dma_sync_sg_for_device_distribute(struct device *dev,
			struct scatterlist *sg, int nelems, enum dma_data_direction dir)
{
}

static inline dma_addr_t pswiotlb_iommu_dma_map_page(struct device *dev,
			struct page *page, unsigned long offset, size_t size,
			enum dma_data_direction dir, unsigned long attrs)
{
	return 0;
}

static inline void pswiotlb_iommu_dma_unmap_page(struct device *dev,
			dma_addr_t dma_handle, size_t size, enum dma_data_direction dir,
			unsigned long attrs)
{
}

static inline int pswiotlb_iommu_dma_map_sg(struct device *dev,
			struct scatterlist *sg, int nents, enum dma_data_direction dir,
			unsigned long attrs)
{
	return 0;
}

static inline void pswiotlb_iommu_dma_unmap_sg(struct device *dev,
			struct scatterlist *sg, int nents, enum dma_data_direction dir,
			unsigned long attrs)
{
}

static inline void pswiotlb_iommu_dma_sync_single_for_cpu(struct device *dev,
			dma_addr_t dma_handle, size_t size, enum dma_data_direction dir)
{
}

static inline void pswiotlb_iommu_dma_sync_single_for_device(struct device *dev,
			dma_addr_t dma_handle, size_t size, enum dma_data_direction dir)
{
}

static inline void pswiotlb_iommu_dma_sync_sg_for_cpu(struct device *dev,
			struct scatterlist *sgl, int nelems, enum dma_data_direction dir)
{
}

static inline void pswiotlb_iommu_dma_sync_sg_for_device(struct device *dev,
			struct scatterlist *sgl, int nelems, enum dma_data_direction dir)
{
}

static inline bool check_if_pswiotlb_is_applicable(struct device *dev)
{
	return false;
}

static inline void pswiotlb_dma_direct_sync_single_for_device(struct device *dev,
			dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
}

static inline void pswiotlb_dma_direct_sync_single_for_cpu(struct device *dev,
			dma_addr_t addr, size_t size, enum dma_data_direction dir)
{
}

static inline dma_addr_t pswiotlb_dma_direct_map_page(struct device *dev,
			struct page *page, unsigned long offset, size_t size,
			enum dma_data_direction dir, unsigned long attrs)
{
	return 0;
}

static inline void pswiotlb_dma_direct_unmap_page(struct device *dev, dma_addr_t addr,
			size_t size, enum dma_data_direction dir, unsigned long attrs)
{
}
#endif /* CONFIG_PSWIOTLB*/
#endif /* _KERNEL_PSWIOTLB_DMA_DIRECT_H */

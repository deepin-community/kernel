/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2024 3snic Technologies Co., Ltd */

#ifndef SSS_PCI_H
#define SSS_PCI_H

#ifdef USE_OLD_PCI_FUNCTION
#include <linux/dmapool.h>

#define	pci_pool dma_pool
#define pci_pool_create(name, pdev, size, align, allocation) \
		dma_pool_create(name, &(pdev)->dev, size, align, allocation)
#define	pci_pool_destroy(pool) dma_pool_destroy(pool)
#define	pci_pool_alloc(pool, flags, handle) dma_pool_alloc(pool, flags, handle)
#define	pci_pool_zalloc(pool, flags, handle) \
		dma_pool_zalloc(pool, flags, handle)
#define	pci_pool_free(pool, vaddr, addr) dma_pool_free(pool, vaddr, addr)

static inline int pci_set_dma_mask(struct pci_dev *dev, u64 mask)
{
	return dma_set_mask(&dev->dev, mask);
}

static inline int pci_set_consistent_dma_mask(struct pci_dev *dev, u64 mask)
{
	return dma_set_coherent_mask(&dev->dev, mask);
}

int pci_disable_pcie_error_reporting(struct pci_dev *dev);
int pci_enable_pcie_error_reporting(struct pci_dev *dev);
#endif

#endif

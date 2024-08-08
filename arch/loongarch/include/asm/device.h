/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Arch specific extensions to struct device
 *
 * This file is released under the GPLv2
 * Copyright (C) 2020 Loongson Technology Corporation Limited
 */
#ifndef _ASM_LOONGARCH_DEVICE_H
#define _ASM_LOONGARCH_DEVICE_H

struct dev_archdata {
	/* hook for IOMMU specific extension */
	void *iommu;
	struct bus_dma_region *dma_range_map;
	/*
	 * On some old 7A chipset, dma address is different from physical
	 * address, the main difference is that node id. For dma address
	 * node id starts from bit 36, physical node id starts from
	 * bit 44. The remaining address below node id is the same.
	 */
	unsigned long   dma_node_mask;
	unsigned int	dma_node_off;
};

struct pdev_archdata {
};

struct dma_domain {
	struct list_head node;
	const struct dma_map_ops *dma_ops;
	int domain_nr;
};
void add_dma_domain(struct dma_domain *domain);
void del_dma_domain(struct dma_domain *domain);

#endif /* _ASM_LOONGARCH_DEVICE_H*/

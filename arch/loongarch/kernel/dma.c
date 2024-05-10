// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020-2022 Loongson Technology Corporation Limited
 */
#include <linux/acpi.h>
#include <linux/dma-direct.h>
#include <asm/loongson.h>

/*
 * We extract 4bit node id (bit 44~47) from Loongson-3's
 * 48bit physical address space and embed it into 40bit.
 */

static int node_id_offset;

dma_addr_t phys_to_dma(struct device *dev, phys_addr_t paddr)
{
	long nid = (paddr >> 44) & 0xf;

	return ((nid << 44) ^ paddr) | (nid << node_id_offset);
}

phys_addr_t dma_to_phys(struct device *dev, dma_addr_t daddr)
{
	long nid = (daddr >> node_id_offset) & 0xf;

	return ((nid << node_id_offset) ^ daddr) | (nid << 44);
}

void acpi_arch_dma_setup(struct device *dev)
{
	int ret;
	u64 mask, end = 0;
	const struct bus_dma_region *map = NULL;

	if (node_id_offset == 0) {
		node_id_offset = ((readl(LS7A_DMA_CFG) & LS7A_DMA_NODE_MASK) >> LS7A_DMA_NODE_SHF);
		node_id_offset += 36;
	}

	ret = acpi_dma_get_range(dev, &map);
	if (!ret && map) {
		const struct bus_dma_region *r = map;

		for (end = 0; r->size; r++) {
			if (r->dma_start + r->size - 1 > end)
				end = r->dma_start + r->size - 1;
		}

		mask = DMA_BIT_MASK(ilog2(end) + 1);
		dev->bus_dma_limit = end;
		dev->dma_range_map = map;
		dev->coherent_dma_mask = min(dev->coherent_dma_mask, mask);
		*dev->dma_mask = min(*dev->dma_mask, mask);
	}

}

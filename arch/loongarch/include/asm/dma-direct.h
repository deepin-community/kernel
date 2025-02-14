/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_LOONGARCH_DMA_DIRECT_H
#define _ASM_LOONGARCH_DMA_DIRECT_H

extern int node_id_offset;

static inline dma_addr_t phys_to_dma(struct device *dev, phys_addr_t paddr)
{
	long nid = (paddr >> 44) & 0xf;

	return ((nid << 44) ^ paddr) | (nid << node_id_offset);
}

static inline phys_addr_t dma_to_phys(struct device *dev, dma_addr_t daddr)
{
	long nid = (daddr >> node_id_offset) & 0xf;

	return ((nid << node_id_offset) ^ daddr) | (nid << 44);
}

#endif /* _ASM_LOONGARCH_DMA_DIRECT_H */

/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_SW64_DMA_MAPPING_H
#define _ASM_SW64_DMA_MAPPING_H


extern const struct dma_map_ops *dma_ops;

static inline const struct dma_map_ops *get_arch_dma_ops(void)
{
	return dma_ops;
}


#endif /* _ASM_SW64_DMA_MAPPING_H */

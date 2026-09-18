#ifndef _DPP_KERNEL_INIT_H_
#define _DPP_KERNEL_INIT_H_

#include "zxic_common.h"
#include "dpp_dtb_table_api.h"

ZXIC_SINT32 dpp_dtb_queue_dma_mem_alloc(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, ZXIC_UINT32 size);
ZXIC_SINT32 dpp_dtb_queue_dma_mem_get(DPP_DEV_T *dev, ZXIC_UINT32 queue_id, DTB_QUEUE_DMA_ADDR_INFO *dmaAddrInfo);
ZXIC_SINT32 dpp_dtb_queue_dma_mem_release(DPP_DEV_T *dev, ZXIC_UINT32 queue_id);
ZXIC_SINT32 dtb_sdt_dump_dma_alloc(DPP_DEV_T *dev,
                                   ZXIC_UINT32 dma_size, 
                                   ZXIC_UINT64 * p_dma_phy_addr, 
                                   ZXIC_UINT64 * p_dma_vir_addr);

ZXIC_SINT32 dtb_sdt_dump_dma_release(DPP_DEV_T *dev,
                            ZXIC_UINT32 dma_size, 
                            ZXIC_UINT64 dma_phy_addr, 
                            ZXIC_UINT64 dma_vir_addr);
#endif

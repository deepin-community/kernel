//*****************************************************************************
//  Copyright (c) 2021 Glenfly Tech Co., Ltd.. 
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China. 
//*****************************************************************************

#include "gf_adapter.h"
#include "vidsch.h"
#include "vidschi.h"
#include "vidsch_engine_e3k.h"
#include "chip_include_e3k.h"
#include "global.h"
#include "mm_e3k.h"

int vidschi_get_slot_count_e3k(adapter_t *adapter)
{
    unsigned int i, slot_cnt = 0;
    unsigned int mm_driver_last = MM_DRIVERID_LAST;

    //gf_info("%s, driver id slot count is not already yet, needs for split dma usage!!\n", util_remove_name_suffix(__func__));

    //for(i = 0; i < mm_driver_last; i++)
    //{
    //}

    return slot_cnt;
}

void vidschi_patch_allocation_e3k(adapter_t *adapter,
                                 task_dma_t *task,
                                 gf_patchlocation_list_t *patch_location,
                                 vidsch_allocation_t *sch_allocation)
{
    large_inter              address;
    unsigned int             *patch_offset;
    static unsigned int      videoDrawId_FE = 0;
    static unsigned int      videoDrawId_BE = 0;
    unsigned int             *pVideoDrawID  = NULL;
    engine_share_e3k_t       *share = (engine_share_e3k_t *)adapter->private_data;
    vidmm_allocation_t       *allocation    = sch_allocation->mm_allocation;

    // We use AllocationOffset=-1 to pass HardwareContext. We don't need a patch for it.
    // Problem: AllocationIndex is unsigned int ,  it's alwayls >= 0 ), needing confirm
    if(patch_location->AllocationOffset == -1 || patch_location->AllocationOffset == 0xFFFFFFFE)
    {
        return;
    }

    patch_offset = (unsigned int *)(task->dma_buffer_node->virt_base_addr + patch_location->PatchOffset);
    address.quad64 = sch_allocation->phy_addr + patch_location->AllocationOffset;

    //need add video patch code later
    switch (patch_location->DriverId)
    {
    case MM_DRIVERID_SIGNATURE:
    case MM_DRIVERID_VS_SHADER:
    case MM_DRIVERID_HS_SHADER:
    case MM_DRIVERID_DS_SHADER:
    case MM_DRIVERID_GS_SHADER:
    case MM_DRIVERID_PS_SHADER:
    case MM_DRIVERID_CS_SHADER:
    case MM_DRIVERID_INDEXBUFFER_ADDRESS:
    case MM_DRIVERID_CONTEXT:
    case MM_DRIVERID_SO_BUFFER_ADDRESS:
    case MM_DRIVERID_DIP_INDIRECT_BUFFER_ADDRESS:
    case MM_DRIVERID_DISPATCH_INDIRECT_BUFFER_ADDRESS:
    case MM_DRIVERID_DIP_PARAM_BUFFER_ADDRESS:
    case MM_DRIVERID_CSHARP:
    case MM_DRIVERID_FENCE:
    case MM_DRIVERID_MIUCOUNTER:
    case MM_DRIVERID_SO_BUFFER_FILL_SIZE_POOL:
      	patch_offset[0] = address.low32;
        patch_offset[1] = (patch_offset[1] & 0xFFFFFF00) | (address.high32 &0xFF);
        break;

    case MM_DRIVERID_VERTEXBUFFER_40BIT_ADDRESS:
        patch_offset[0] = address.low32;
        patch_offset[1] = address.high32 & 0xFF;
        break;
    /* video start*/
    case MM_DRIVERID_VIDEO_RESERVEDMEM:
    case MM_DRIVERID_VIDEO_FENCE:
    case MM_DRIVERID_VIDEO_PATCH:
    case MM_DRIVERID_VIDEO_DECODE_CMD:
    case MM_DRIVERID_VIDEO_DECODE_VCPDMA:
    case MM_DRIVERID_VIDEO_DECODE_VCPSBDMA:
    case MM_DRIVERID_VIDEO_DECODE_QTMDMA:
    case MM_DRIVERID_VIDEO_DECODE_DRVDMA:
    case MM_DRIVERID_VIDEO_SIGNATURE:
    case MM_DRIVERID_VIDEO_DECODE_COMPBUFF:
        patch_offset[0] = address.low32;
        patch_offset[1] = (patch_offset[1] & 0xFFFFFF00) | (address.high32 & 0xFF);
        break;
    case MM_DRIVERID_VIDEO_DECODE_MSVD:
        patch_offset[0] = (address.low32 & 0xFFFFFFE0) | (patch_offset[0] & 0x1F);
        patch_offset[1] = (patch_offset[1] & 0xFFFFFF00) | (address.high32 & 0xFF);
        break;
    case MM_DRIVERID_VIDEO_DECODE_INS:
	patch_offset[0] = address.low32 >> 8;
        patch_offset[1] = (patch_offset[1] & 0xFFFFFF00) | (address.high32 & 0xFF);
        break;
    case MM_DRIVERID_ISP_VPP_SRC:
    case MM_DRIVERID_ISP_VPP_DST:
        address.quad64 = address.quad64 >> 8;
        patch_offset[0] =  address.low32;
        break;
    case MM_DRIVERID_VIDEO_DRAWID:
        pVideoDrawID = (address.quad64 & 0x1) == 0? &videoDrawId_FE : &videoDrawId_BE;
        patch_offset[0] = (((*pVideoDrawID) & 3) << 1) |  // bit[2:1], drawID low
        (((*pVideoDrawID) & 0x3c) << 5)|  // bit[10:7], drawID high
        patch_offset[0];
        (*pVideoDrawID) = ((*pVideoDrawID) + 1) & 0x3F;
        break;
    /* video end*/

    case MM_DRIVERID_CSL1:
        patch_offset[0] = (share->ring_buffer_phy_addr[RB_INDEX_CSL1] >> 8);
        break;
    case MM_DRIVERID_CSL2:
        patch_offset[0] = (share->ring_buffer_phy_addr[RB_INDEX_CSL2] >> 8);
        break;
    case MM_DRIVERID_CSL3:
        patch_offset[0] = (share->ring_buffer_phy_addr[RB_INDEX_CSL3] >> 8);
        break;
    case MM_DRIVERID_SVM_POINTER:
        address.low32  = patch_offset[0] + address.low32;
        address.high32 = patch_offset[1] + address.high32;
        patch_offset[0] = address.low32;
        patch_offset[1] = address.high32;
        break;
    default:
        //case MM_DRIVERID_CSUNORDEREDACCESS: Uav_Base should be alined to 2kb(>>8)
        patch_offset[0]  = address.low32 >> 8;
        patch_offset[0] |= (address.high32 << 24);
        //gf_info("set driver id patch:%x, real PA:%x\n", patch_offset[0], address.quad64);
        break;
    }
}

/* only do patch after paging/mirgate */
int vidsch_patch_e3k(adapter_t *adapter, task_dma_t *task_dma)
{
    hw_ctxbuf_t                     *hwctx_buf        = task_dma->hw_ctx_info;
    gf_patchlocation_list_t         *patch_location   = task_dma->patch_location_list;
    vidsch_allocation_t             *sch_allocation   = task_dma->sch_allocation_list;
    unsigned int                    ret = S_OK;
    int                             i;

    if(task_dma->need_hwctx_switch)
    {
        if(patch_location[0].DriverId == MM_DRIVERID_CONTEXT)
        {
            hwctx_buf->context_buffer_address = sch_allocation[patch_location[0].AllocationIndex].phy_addr;
        }
        else
        {
            gf_error("## need do hwctx switch, but no hwctx allocation found in patch list, patch: offset[%d, %d], [index-0x%x, DriverId-0x%x].\n",
                task_dma->patch_start_offset, task_dma->patch_end_offset, patch_location[0].AllocationIndex, patch_location[0].DriverId);
        }
    }

    for(i = task_dma->patch_start_offset; i < task_dma->patch_end_offset; i++)
    {
        patch_location = &task_dma->patch_location_list[i];

         // Problem: AllocationIndex is unsigned int ,  it's alwayls >= 0 ), needing confirm
        if (patch_location->AllocationIndex < 0 ||
            patch_location->AllocationIndex > task_dma->allocation_list_size)
        {
            ret = E_FAIL;
            goto exit;
        }

        // Problem: AllocationIndex is unsigned int ,  it's alwayls != 0 ), needing confirm
        if (patch_location->PatchOffset > task_dma->dma_buffer_node->size &&
            patch_location->AllocationOffset != -1)
        {
            ret = E_FAIL;
            goto exit;
        }

        sch_allocation = task_dma->sch_allocation_list + patch_location->AllocationIndex;

        if (sch_allocation->segment_id)
        {
            vidschi_patch_allocation_e3k(adapter,task_dma, patch_location, sch_allocation);
        }
    }

exit:
    return ret;
}








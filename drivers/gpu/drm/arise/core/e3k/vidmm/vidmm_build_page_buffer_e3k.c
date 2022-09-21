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
#include "global.h"
#include "vidmm.h"
#include "vidmmi.h"
#include "mm_e3k.h"
#include "bit_op.h"
#include "chip_include_e3k.h"
#include "../vidsch/vidsch_blt_e3k.h"
#include "../vidsch/vidsch_engine_e3k.h"
#include "../vidsch/vidsch_debug_hang_e3k.h"

extern void vidsch_duplicate_hang_e3k(adapter_t * adapter);
extern reg_debug_mode_e3k debug_mode_e3k;

unsigned int vidmmi_invalidate_bl_slot_e3k(vidmm_mgr_t *mm_mgr, vidmm_allocation_t *allocation, unsigned int **ppDma)
{
    heap_t   *slot_buffer = &mm_mgr->blheap;
    unsigned long long start_addr = 0llu;
    unsigned long long end_addr = 0llu;
    unsigned long long mid_addr   = 0llu;
    unsigned long long inv_addr   = 0llu;
    unsigned int bl_range = util_align((allocation->size / BL_MAPPING_RATIO), slot_buffer->alignment);
    unsigned int slot_mask_idx = 0;
    unsigned int *pDma = *ppDma;


    start_addr = (allocation->slot_index * BL_SLOT_SIZE) + slot_buffer->start;
    end_addr = start_addr + bl_range;
    mid_addr = start_addr ^ end_addr;
    _BitScanReverse_64(&slot_mask_idx, mid_addr);
    mid_addr = end_addr & (~((1<< slot_mask_idx)-1));

    //gf_info("allocatoin: %x, start=%x,end=%x,mid=%x.\n", allocation->handle, start_addr, end_addr, mid_addr);

    bl_range = end_addr - mid_addr;
    inv_addr = mid_addr;
    while(bl_range)
    {

        _BitScanReverse(&slot_mask_idx, bl_range);
        _bittestandreset(&bl_range, slot_mask_idx);

         //gf_info("allocatoin: %x, addr=%x,mask=%x.\n", allocation->handle, inv_addr, ~((1<<slot_mask_idx)-1));

        *pDma++ = SEND_FLAGBUFFER_INVALIDATE_COMMAND_E3K();
        *pDma++ = inv_addr;
        *pDma++ = inv_addr >> 32;
        *pDma++ = ~((1<<slot_mask_idx)-1);
        *pDma++ = 0xffffffff;

        inv_addr += (1<< slot_mask_idx);
    
    }

    bl_range = mid_addr - start_addr;
    inv_addr = start_addr;
    while(bl_range)
    {

        _BitScanForward(&slot_mask_idx, bl_range);
        _bittestandreset(&bl_range, slot_mask_idx);

         //gf_info("allocatoin: %x, addr=%x,mask=%x.\n", allocation->handle, inv_addr, ~((1<<slot_mask_idx)-1));


        *pDma++ = SEND_FLAGBUFFER_INVALIDATE_COMMAND_E3K();
        *pDma++ = inv_addr;
        *pDma++ = inv_addr >> 32;
        *pDma++ = ~((1<<slot_mask_idx)-1);
        *pDma++ = 0xffffffff;

        inv_addr += (1<< slot_mask_idx);
    
    }



    *pDma++ = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_CSP_FENCE,_RBT_3DFE, HWM_SYNC_KMD_SLOT);
    *pDma++ = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_PRECSPFENCE);
    *pDma++ = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_PRECSPFENCE);
    *pDma++ = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_PRECSPFENCE);

    *ppDma = pDma;

    return S_OK;
}

static unsigned int vidmmi_clear_bl_slot_e3k(adapter_t *adapter, vidmm_private_build_paging_buffer_arg_t *data)
{
    vidmm_allocation_t *allocation = data->allocation;
    vidmm_mgr_t  *mm_mgr = adapter->mm_mgr;
    unsigned int *pDma = data->dma_buffer;    
    unsigned int bl_byte  = allocation->size >> 9;
    int cmd_loop = bl_byte/0x1FFFF + ((bl_byte % 0x1FFFF)?1:0);//max size of once clear should be 0x1FFFF
    large_inter start_addr;
    unsigned int clear_value = 0xFFFFFFFF;
    unsigned int clear_size = 0;
    unsigned int slot_num = bl_byte >> 7;
    int i=0;

    Cmd_Blk_Cmd_Csp_Blc_Dword1 clearAddrL = {0};
    Cmd_Blk_Cmd_Csp_Blc_Dword2 clearAddrHAndValue = {0};
    Cmd_Blk_Cmd_Csp_Blc_Dword3 clearMask = {0};


    start_addr.quad64 = allocation->slot_index << 7;    

    vidmmi_invalidate_bl_slot_e3k(mm_mgr, allocation, &pDma);  

    clearAddrHAndValue.Clear_Value0         = clear_value & 0xD;
    clearMask.Blc_Mask                      = 0;

    for (i = 0; i < cmd_loop; ++i)
    {
        start_addr.quad64 += i*0x1FFFF;
        clear_size = ((cmd_loop  == (i + 1)) && (bl_byte%0x1FFFF) )?(bl_byte%0x1FFFF):0x1FFFF;
        clearAddrL.Start_Address_Low32 = start_addr.low32;
        clearAddrHAndValue.Start_Address_High8  =( start_addr.high32) & 0xFF ;//actual PA is 36bits, maybe 0xF
    
        *pDma++ = SEND_BL_CLEAR_COMMAND_E3K(clear_size);//clear max byte: 2^17 
        *pDma++ = *(DWORD*)&clearAddrL;
        *pDma++ = *(DWORD*)&clearAddrHAndValue;
        *pDma++ = *(DWORD*)&clearMask;       
    }

    if (data->dma_size < ( (unsigned char *)data->dma_buffer - (unsigned char *)pDma))
    {
        gf_info("%s:%d,dma buffer not enough,request:%d, actual:%d\n", util_remove_name_suffix(__func__), __LINE__, ((unsigned char *)data->dma_buffer - (unsigned char *)pDma), data->dma_size);
        return E_INSUFFICIENT_DMA_BUFFER;
    }

    data->dma_buffer = (void *)pDma;

    return S_OK;
}

int vidmmi_fast_clear_e3k(adapter_t *adapter, vidmm_private_build_paging_buffer_arg_t *data)
{
    engine_share_e3k_t      *share                = adapter->private_data; 
    RB_PREDEFINE_DMA* pTemplate = (RB_PREDEFINE_DMA*) share->begin_end_vma->virt_addr;    
    CACHEFLUSH_DMA_E3K *pCacheFlushDMA3DL  = &(pTemplate->FlushDMA3DL);    
    FASTCLEAR_REGSETTING_E3K* pClearCmd0 = NULL;
    FASTCLEAR_REGSETTING_E3K ClearCmd1 = {0};
    FASTCLEAR_REGSETTING_E3K* pClearCmd = &ClearCmd1;
    vidmm_allocation_t *allocation = data->allocation;
    unsigned int fill_size = data->fill.fill_size;
    unsigned int fill_pattern = data->fill.fill_pattern;
    large_inter fill_addr;
    unsigned int *pDma = data->dma_buffer;  
    DWORD    max_fill_cmd_count = 0;
    DWORD    AddrOffset = data->multi_pass_offset;
    DWORD    StartX = 0;
    DWORD    Format = HSF_B8G8R8A8_UNORM;
    DWORD    dma_space_left = (DWORD)data->dma_size;
    large_inter dstAddress,dstAddrAligned;    
    unsigned int NumBytes,dstStartX;
    unsigned int stride = MAX_WIDTH_2DBLT; //in pixel,32bpp
    unsigned int Height = 0;
    unsigned int fill_count = 0;    
    int result = S_OK;
    int multi_pass = FALSE;    

    Reg_Proc_Sec                     reg_Proc_Sec = {0};
    reg_Proc_Sec.reg.Sec_Status  = 1;

    fill_addr.quad64 = data->fill.phy_addr;

    gf_assert((fill_addr.quad64 & 0xFF) == 0, NULL);//actual all allocation PA will be ensured at least 4k aligned in kmd

    dma_space_left = data->dma_size - util_get_ptr_span(pDma, data->dma_buffer) ;

    if(allocation->flag.secured)
    {
        gf_memcpy(pDma, pCacheFlushDMA3DL, sizeof(CACHEFLUSH_DMA_E3K));  
        pDma += sizeof(CACHEFLUSH_DMA_E3K)/sizeof(DWORD);
        *pDma ++ = SET_REGISTER_FD_E3K(MXU_BLOCK, Reg_Proc_Sec_Offset, 1);
        *pDma++ = reg_Proc_Sec.uint;

        //reserve more for reset 3d engine to non-secure status.
        dma_space_left = data->dma_size - util_get_ptr_span(pDma, data->dma_buffer) -sizeof(CACHEFLUSH_DMA_E3K) - sizeof(DWORD);        
    }
    
    max_fill_cmd_count = dma_space_left / sizeof(FASTCLEAR_REGSETTING_E3K);
    
    if(0 == max_fill_cmd_count)
    {
        result = E_INSUFFICIENT_DMA_BUFFER;
        goto exit;
    }
    
    dstAddress.quad64 = fill_addr.quad64 + data->multi_pass_offset;
    dstAddrAligned.quad64 = dstAddress.quad64 & ~0xFFll;//2kbits align
    dstStartX = (dstAddress.quad64 - dstAddrAligned.quad64) >> 2;//shift to 32bpp pixel 

    pClearCmd0 = (FASTCLEAR_REGSETTING_E3K*)pDma;
    do
    {
        gf_memcpy(pClearCmd, &share->fast_clear_cmd_e3k, sizeof(FASTCLEAR_REGSETTING_E3K));

        pClearCmd->DstFormat.reg.Format         = Format;
        pClearCmd->DstFormat.reg.Bl_Slot_Idx    = 0;
        pClearCmd->DstDepth.reg.Depth_Or_Arraysize= 1;
        pClearCmd->DstDepth.reg.Range_Type      = 0;
        pClearCmd->DstViewCtrl.reg.Arraysize    = 1;
        pClearCmd->DstViewCtrl.reg.First_Array_Idx = 0;

        pClearCmd->DstMisc.reg.Is_Tiling        = 0; //Always treate it as linear.
        pClearCmd->DstMisc.reg.Rt_Enable        = 1;
        pClearCmd->DstMisc.reg.Rt_Write_Mask    = 0xF;
        pClearCmd->DstMisc.reg.Resource_Type = RT_MISC_RESOURCE_TYPE_2D_TEXTURE;
        pClearCmd->DstMisc.reg.Eu_Blend_Enable  = 0; //Always treate it as linear.

        pClearCmd->Dzs_Ctrl.reg.Src_Read_Alloc_En = 0;

        pClearCmd->Rast_Ctrl.reg.Check_Board = RAST_CTRL_CHECK_BOARD_64X16;
        pClearCmd->Rast_Ctrl.reg.Eub_En = 0;

        pClearCmd->reg_Iu_Ctrl_Ex.reg.Eu_Blending_En = 0;

        pClearCmd->Zs_Req_Ctrl.reg.D_Alloc_En = 1;

        pClearCmd->Fast_Clear_Color0.reg.Color = fill_pattern;
        pClearCmd->Fast_Clear_Color1.reg.Color = fill_pattern;
        pClearCmd->Fast_Clear_Color2.reg.Color = fill_pattern;
        pClearCmd->Fast_Clear_Color3.reg.Color = fill_pattern;

        pClearCmd->DstAddr.reg.Base_Addr = (fill_addr.quad64 + data->multi_pass_offset) >> 8;        

        // the first partial line
        if(0 != dstStartX)
        {
            //consider total size is small than MAX_WIDTH_2DBLT 
            unsigned int width = ((fill_size - data->multi_pass_offset) < ((stride - dstStartX) << 2)) ? ((fill_size - data->multi_pass_offset) >> 2): (stride - dstStartX);

            pClearCmd->DstSize.reg.Width            = width;
            pClearCmd->DstSize.reg.Height           = 1;   
            pClearCmd->RectX.Xmin = dstStartX -1;
            pClearCmd->RectX.Xmax =stride - dstStartX -1;
            pClearCmd->RectY.Ymin = 0;
            pClearCmd->RectY.Ymax = 1-1;
     
           NumBytes = width << 2;
           dstStartX = 0;
        }
        // The last partial line
        else if((fill_size - data->multi_pass_offset) < (stride << 2))
        {
            pClearCmd->DstSize.reg.Width            = (fill_size - data->multi_pass_offset) >> 2;
            pClearCmd->DstSize.reg.Height           = 1;   
            pClearCmd->RectX.Xmin = 0;
            pClearCmd->RectX.Xmax = ((fill_size - data->multi_pass_offset) >> 2) -1;
            pClearCmd->RectY.Ymin = 0;
            pClearCmd->RectY.Ymax = 1 -1;

            NumBytes = (fill_size - data->multi_pass_offset);
        }
        // block fill
        else
        {
          Height = (fill_size - data->multi_pass_offset) / (stride << 2);

          if ( Height > MAX_HEIGHT_2DBLT )
          {
              Height = MAX_HEIGHT_2DBLT;
          }

          pClearCmd->DstSize.reg.Width            = stride;
          pClearCmd->DstSize.reg.Height           = Height;   
          pClearCmd->RectX.Xmin = 0;
          pClearCmd->RectX.Xmax = stride -1;
          pClearCmd->RectY.Ymin = 0;
          pClearCmd->RectY.Ymax = Height -1;

          NumBytes = Height * (stride << 2);
        }
        gf_memcpy(pClearCmd0, pClearCmd, sizeof(FASTCLEAR_REGSETTING_E3K));
        pClearCmd0++;
        fill_count++;
        data->multi_pass_offset += NumBytes;
        
        if(fill_count >= max_fill_cmd_count)
        {
            if(data->multi_pass_offset < data->fill.fill_size)
            {
                multi_pass = TRUE;
            }
            break;
        }
        
        }while(data->multi_pass_offset < data->fill.fill_size);


        if(multi_pass)
        {
            result = E_INSUFFICIENT_DMA_BUFFER;
        }
        else
        {
            data->multi_pass_offset = 0;
        }
        
exit:

    pDma = (unsigned int*)pClearCmd0;

    if(allocation->flag.secured)
    {
        reg_Proc_Sec.reg.Sec_Status  = 0; 
        gf_assert((data->dma_size - util_get_ptr_span(pDma, data->dma_buffer)) >= (sizeof(CACHEFLUSH_DMA_E3K)/sizeof(DWORD) + sizeof(DWORD)), "buf not enough for fast clear cmd");
        gf_memcpy(pDma, pCacheFlushDMA3DL, sizeof(CACHEFLUSH_DMA_E3K));  
        pDma += sizeof(CACHEFLUSH_DMA_E3K)/sizeof(DWORD);
        *pDma ++ = SET_REGISTER_FD_E3K(MXU_BLOCK, Reg_Proc_Sec_Offset, 1);
        *pDma++ = reg_Proc_Sec.uint;
    }    
    data->dma_buffer = (void *)pDma;
    return result;
}

//video only is for fpga validation, will not enable gfx engine, just skip it.
int vidmm_build_page_buffer_e3k(adapter_t *adapter, vidmm_private_build_paging_buffer_arg_t *data)
{
    unsigned int *dma = data->dma_buffer;
    vidmm_allocation_t *allocation = data->allocation;
    int result = S_OK;
    int multi_pass = FALSE;

    if(debug_mode_e3k.internal_dump_hw)
    {
        gf_info("vidmm_build_page_buffer_e3k start dump image, need skip this function...\n");
        return 0;
    }

    //secure mem should not page out, since content in range not allowed write to non-sec mem.
    if(!adapter->hw_caps.video_only && (BUILDING_PAGING_OPERATION_TRANSFER == data->operation))
    {
        engine_share_e3k_t      *share                = adapter->private_data;
        BITBLT_REGSETTING_E3K* p2DBltCmd = NULL;
        unsigned int *pDma = data->dma_buffer;  
        DWORD    max_transfer_cmd_count = 0;
        DWORD    start_offset = data->multi_pass_offset;
        DWORD    StartX = 0;
        DWORD    Format = HSF_B8G8R8A8_UNORM;
        DWORD    dma_space_left = data->dma_size;
        unsigned int NumBytes,dstStartX;
        unsigned int Height = 0;
        unsigned int Width = 0;
        unsigned int transfer_size = data->transfer.transfer_size;        

        gf_assert(((data->transfer.transfer_size&0x3) == 0) && //align to 4. driver will ensure it when buffer allocate. 
                   ((data->transfer.dst.phy_addr & 0xFF) ==0) &&//hw need addr 2kb aligned
                   ((data->transfer.src.phy_addr & 0xFF) ==0), NULL);
      
        dma_space_left = data->dma_size - util_get_ptr_span(pDma, data->dma_buffer);
        max_transfer_cmd_count = dma_space_left / sizeof(BITBLT_REGSETTING_E3K);
        
        if(0 == max_transfer_cmd_count)
        {
            return E_INSUFFICIENT_DMA_BUFFER;
        }

        p2DBltCmd =(BITBLT_REGSETTING_E3K*)pDma;
        
         while(transfer_size > 0 && max_transfer_cmd_count > 0)
         {
             Width = MAX_WIDTH_2DBLT;//in pixel
             Height = transfer_size/(Width << 2);
             Height = Height > MAX_HEIGHT_2DBLT ? MAX_HEIGHT_2DBLT : Height;
             
             if(transfer_size < (MAX_WIDTH_2DBLT << 2))
             {
                 Width  = transfer_size >> 2;
                 Height = 1;
             }
      
             StartX = (start_offset -(start_offset&~0xFF)) >>2;
             start_offset = start_offset&~0xFF;
         
             gf_memcpy(p2DBltCmd, &share->_2dblt_cmd_e3k, sizeof(BITBLT_REGSETTING_E3K));
         
             p2DBltCmd->SrcAddr.reg.Base_Addr = (data->transfer.src.phy_addr + start_offset) >> 8;
             p2DBltCmd->DstAddr.reg.Base_Addr = (data->transfer.dst.phy_addr + start_offset) >> 8;
         
             p2DBltCmd->SrcFormat.reg.Format         = Format;
             p2DBltCmd->SrcFormat.reg.Bl_Slot_Idx    = 0;
             p2DBltCmd->SrcSize.reg.Width            = Width;
             p2DBltCmd->SrcSize.reg.Height           = Height;
             p2DBltCmd->SrcDepth.reg.Depth_Or_Arraysize= 1;
             p2DBltCmd->SrcDepth.reg.Range_Type      = 0;
             p2DBltCmd->SrcViewCtrl.reg.Arraysize    = 1;
             p2DBltCmd->SrcViewCtrl.reg.First_Array_Idx = 0;
         
             p2DBltCmd->DstFormat = p2DBltCmd->SrcFormat;
             p2DBltCmd->DstSize   = p2DBltCmd->SrcSize;
             p2DBltCmd->DstDepth   = p2DBltCmd->SrcDepth;
             p2DBltCmd->DstViewCtrl = p2DBltCmd->SrcViewCtrl;
         
             p2DBltCmd->SrcMisc.reg.Is_Tiling        = 0;
             p2DBltCmd->SrcMisc.reg.Rt_Enable        = 1;
             p2DBltCmd->SrcMisc.reg.Resource_Type  = RT_MISC_RESOURCE_TYPE_2D_TEXTURE;
             p2DBltCmd->SrcMisc.reg.Eu_Blend_Enable  = 0;          
         
             p2DBltCmd->DstMisc.reg.Is_Tiling        = 0;
             p2DBltCmd->DstMisc.reg.Rt_Enable        = 1;
             p2DBltCmd->DstMisc.reg.Rt_Write_Mask    = 0xF;
             p2DBltCmd->DstMisc.reg.Resource_Type  = RT_MISC_RESOURCE_TYPE_2D_TEXTURE;
             p2DBltCmd->DstMisc.reg.Eu_Blend_Enable  = 0;             

             p2DBltCmd->Rast_Ctrl.reg.Eub_En         = 0;
             p2DBltCmd->reg_Iu_Ctrl_Ex.reg.Eu_Blending_En = 0;
 
             p2DBltCmd->Dzs_Ctrl.reg.Src_Read_Alloc_En = 0;
         
             p2DBltCmd->RectX.Xmin = StartX;
             p2DBltCmd->RectX.Xmax = Width - 1;
             p2DBltCmd->RectY.Ymin = 0;
             p2DBltCmd->RectY.Ymax = Height - 1;
             p2DBltCmd->SrcDxDy.Dx = 0;
             p2DBltCmd->SrcDxDy.Dy = 0;
         
             start_offset += (Width * Height) << 2;          
             transfer_size -=((Width * Height) << 2) + (StartX << 2);
             
             max_transfer_cmd_count--;         
             p2DBltCmd++;
         }
         
         if(transfer_size > 0)
         {
             data->multi_pass_offset = start_offset;
             result = E_INSUFFICIENT_DMA_BUFFER;
         }
         
         data->dma_buffer  = (VOID*)p2DBltCmd;       
    }
    else if(BUILDING_PAGING_OPERATION_FILL == data->operation)
    {
        if(allocation->compress_format != CP_OFF)//video only still need invalidate bl, BL is managed by MXU.
        {
             return vidmmi_clear_bl_slot_e3k(adapter, data);
        }
        else if(!adapter->hw_caps.video_only)//video only hasn't 3d module.
        {
            return vidmmi_fast_clear_e3k(adapter, data);
        }
    }
    return result;
}

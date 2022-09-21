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
#include "bit_op.h"

void enginei_init_context_save_restore_dma_e3k(engine_gfx_e3k_t *engine)
{
    engine_share_e3k_t *share   = engine->common.share;
    adapter_t          *adapter = engine->common.vidsch->adapter;
    RB_PREDEFINE_DMA* pCmdBuf = NULL;
    CONTEXT_RESTORE_DMA_E3K *pRestoreDMA        = NULL;

    CONTEXT_SAVE_DMA_E3K    *pSaveDMA           = NULL;
    CACHEFLUSH_DMA_E3K      *pCacheFlushDMA3DL  = NULL;
 
    DWORD                    regValue           = 0;
 

    gf_map_argu_t map_argu = {0};
    gf_vm_area_t  *vma     = NULL;

    unsigned int slice_mask = adapter->hw_caps.chip_slice_mask;
    unsigned int gpc_bit_mask = 0, shift_num = 1, bit_mask = 0;

    while(slice_mask)
    {
        if(slice_mask & 0xf)
        {
            gpc_bit_mask |= shift_num;
        }
        slice_mask >>= 4;
        shift_num <<= 1;
    }

 
        
    map_argu.flags.cache_type = GF_MEM_WRITE_COMBINED;
    map_argu.flags.mem_space  = GF_MEM_KERNEL;
    map_argu.flags.mem_type   = GF_SYSTEM_IO;
    map_argu.phys_addr = adapter->vidmm_bus_addr + share->begin_end_buffer_phy_addr;
    map_argu.size             = BEGIN_END_BUF_SIZE_E3K;

    vma = gf_map_io_memory(NULL, &map_argu);

    gf_assert(vma != NULL, "enginei_init_context_save_restore_dma");

    gf_memset(vma->virt_addr, 0, vma->size);

    pCmdBuf = (RB_PREDEFINE_DMA*) vma->virt_addr;

    pRestoreDMA        = &(pCmdBuf->RestoreDMA);

    pSaveDMA           = &(pCmdBuf->SaveDMA);
    pCacheFlushDMA3DL  = &(pCmdBuf->FlushDMA3DL);


    pRestoreDMA->RestoreContext.Major_Opcode         = CSP_OPCODE_Blk_Cmd_Csp_Save_Rsto;
    pRestoreDMA->RestoreContext.Block_Id             = CSP_GLOBAL_BLOCK;
    pRestoreDMA->RestoreContext.Type                 = BLOCK_COMMAND_CSP_TYPE_RESTORE;
    pRestoreDMA->RestoreContext.Address_Mode         = SET_REGISTER_ADDRESS_MODE_OFFSET;
    pRestoreDMA->RestoreContext.Dwc                  = 2;

    pRestoreDMA->ContextBufferOffset_H               = 0;

    pRestoreDMA->RestoreEuFullGlb.uint              = SET_REGISTER_ADDR_E3K(EU_FS_BLOCK, Reg_Eu_Full_Glb_Offset, SET_REGISTER_ADDRESS_MODE_OFFSET);
    pRestoreDMA->EuFullGlbOffset_L                  = SET_REGISTER_ADDR_LOW_E3K((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->HWContextbuffer.shadow_buf_3D.fe_be_regs.eufs_regs.reg_Eu_Full_Glb)));
    pRestoreDMA->EuFullGlbOffset_H                  = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(0x0, 1);

    pRestoreDMA->RestoreTsSharpCtrl.uint            = SET_REGISTER_ADDR_E3K(TUFE_BLOCK, Reg_Tu_Tssharp_Ctrl_Offset, SET_REGISTER_ADDRESS_MODE_OFFSET);
    pRestoreDMA->TsSharpCtrlOffset_L                = SET_REGISTER_ADDR_LOW_E3K((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->HWContextbuffer.shadow_buf_3D.fe_be_regs.tu_regs.reg_Tu_Tssharp_Ctrl)));
    pRestoreDMA->TsSharpCtrlOffset_H                = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(0x0, 1);

    pRestoreDMA->RestoreFFCUbufConfig.uint          = SET_REGISTER_ADDR_E3K(WLS_FE_BLOCK, Reg_Ffc_Ubuf_Golbalconfig_Offset, SET_REGISTER_ADDRESS_MODE_OFFSET);
    pRestoreDMA->FFCUbufConfigOffset_L              = SET_REGISTER_ADDR_LOW_E3K((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->HWContextbuffer.shadow_buf_3D.fe_be_regs.wls_regs.reg_Ffc_Ubuf_Golbalconfig)));
    pRestoreDMA->FFCUbufConfigOffset_H              = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(0x0, 1);

    pRestoreDMA->RestoreCsp.uint                     = SET_REGISTER_ADDR_E3K(CSP_GLOBAL_BLOCK, Reg_Csp_Ms_Total_Gpu_Timestamp_Offset,SET_REGISTER_ADDRESS_MODE_OFFSET);
    pRestoreDMA->CSPOffset_L                         = SET_REGISTER_ADDR_LOW_E3K((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->CSPRegs)));
    pRestoreDMA->CSPOffset_H                         = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(0x0, sizeof(CSP_CONTEXTBUFFER_REGS) >> 2);

    pRestoreDMA->RestoreGpcpFe.uint                  = SET_REGISTER_ADDR_E3K(GPCPFE_BLOCK, Reg_Gpcpfe_Iidcnt_Offset, SET_REGISTER_ADDRESS_MODE_OFFSET);
    pRestoreDMA->GpcpFeOffset_L                      = SET_REGISTER_ADDR_LOW_E3K((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpFeRegs)));
    pRestoreDMA->GpcpFeOffset_H                      = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(0x0, sizeof(GPCPFE_CONTEXTBUFFER_REGS) >> 2);

    pRestoreDMA->RestoreGpcpBe.uint                  = SET_REGISTER_ADDR_E3K(GPCPBE_BLOCK, Reg_Sto_Cfg_Offset, SET_REGISTER_ADDRESS_MODE_OFFSET);
    pRestoreDMA->GpcpBeOffset_L                      = SET_REGISTER_ADDR_LOW_E3K((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpBeRegs)));
    pRestoreDMA->GpcpBeOffset_H                      = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(0x0, sizeof(Gpcpbe_regs)>>2);

    bit_mask = gpc_bit_mask;
    while(bit_mask)
    {
        unsigned int gpc_index = 0;
        _BitScanForward(&gpc_index, bit_mask);
        _bittestandreset((int *)&bit_mask, gpc_index);
        pRestoreDMA->RestoreGpcTop[gpc_index].RestoreGpcTopCmd.uint = SET_REGISTER_ADDR_E3K(24, 0+ gpc_index *40, SET_REGISTER_ADDRESS_MODE_OFFSET);
        pRestoreDMA->RestoreGpcTop[gpc_index].GpcTopOffset_L = SET_REGISTER_ADDR_LOW_E3K((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcTopRegs[gpc_index])));
        pRestoreDMA->RestoreGpcTop[gpc_index].GpcTopOffset_H = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(0x0, 40);
    }

    pRestoreDMA->CspFenceCmd.uint                    = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_CSP_FENCE, _RBT_3DFE,  HWM_SYNC_KMD_SLOT);
    pRestoreDMA->CspFenceValue                       = FENCE_VALUE_BEGIN_DMA;

    pRestoreDMA->CspWaitCmd.uint                     = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_BEGIN_DMA);
    pRestoreDMA->CspWaitMainCmd.uint                 = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_BEGIN_DMA);

    pRestoreDMA->EUFSL1Invalidate.uint = SEND_FS_L1I_INVALIDATE_COMMAND_E3K();
    pRestoreDMA->EUPSL1Invalidate.uint = SEND_PS_L1I_INVALIDATE_COMMAND_E3K();

    pSaveDMA->SaveContext.Major_Opcode              = CSP_OPCODE_Blk_Cmd_Csp_Save_Rsto;
    pSaveDMA->SaveContext.Block_Id                  = CSP_GLOBAL_BLOCK;
    pSaveDMA->SaveContext.Type                      = BLOCK_COMMAND_CSP_TYPE_SAVE;
    pSaveDMA->SaveContext.Address_Mode              = SET_REGISTER_ADDRESS_MODE_OFFSET;
    pSaveDMA->SaveContext.Dwc                       = 2;
    pSaveDMA->ContextBufferOffset_L                 = 0;

    pSaveDMA->ContextBufferOffset_H                 = 0; 

    pSaveDMA->SaveCSPRegister.uint                  = SEND_QUERY_DUMP_E3K(CSP_GLOBAL_BLOCK, sizeof(CSP_CONTEXTBUFFER_REGS)>>2, QUERY_DUMP_ADDRESS_MODE_OFFSET);
    pSaveDMA->CSPOffset_L                           = SEND_QUERY_ADDR1_E3K((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->CSPRegs)));
    pSaveDMA->CSPOffset_H                           = SEND_QUERY_ADDR2_AND_OFFSET_E3K(0x0,Reg_Csp_Ms_Total_Gpu_Timestamp_Offset, TRUE, FALSE);

    pSaveDMA->SaveGpcpFeRegister.uint               = SEND_QUERY_DUMP_E3K(GPCPFE_BLOCK, sizeof(GPCPFE_CONTEXTBUFFER_REGS)>>2, QUERY_DUMP_ADDRESS_MODE_OFFSET);
    pSaveDMA->GpcpFeOffset_L                        = SEND_QUERY_ADDR1_E3K((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpFeRegs)));
    pSaveDMA->GpcpFeOffset_H                        = SEND_QUERY_ADDR2_AND_OFFSET_E3K(0x0,Reg_Gpcpfe_Iidcnt_Offset, TRUE, FALSE);

    pSaveDMA->SaveGpcpBeRegister.uint               = SEND_QUERY_DUMP_E3K(GPCPBE_BLOCK, sizeof(Gpcpbe_regs)>>2, QUERY_DUMP_ADDRESS_MODE_OFFSET);
    pSaveDMA->GpcpBeOffset_L                        = SEND_QUERY_ADDR1_E3K((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpBeRegs)));
    pSaveDMA->GpcpBeOffset_H                        = SEND_QUERY_ADDR2_AND_OFFSET_E3K(0x0,Reg_Sto_Cfg_Offset, TRUE, FALSE);

    bit_mask = gpc_bit_mask;
    while(bit_mask)
    {
        DWORD gpc_index = 0;
        _BitScanForward(&gpc_index, bit_mask);
        _bittestandreset((int *)&bit_mask, gpc_index);
        pSaveDMA->SaveGpcTop[gpc_index].SaveGpcTopRegister.uint = SEND_QUERY_DUMP_E3K(24, 40, SET_REGISTER_ADDRESS_MODE_OFFSET);
        pSaveDMA->SaveGpcTop[gpc_index].GpcTopOffset_L = SEND_QUERY_ADDR1_E3K((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcTopRegs[gpc_index])));
        pSaveDMA->SaveGpcTop[gpc_index].GpcTopOffset_H = SEND_QUERY_ADDR2_AND_OFFSET_E3K(0x0, 40* gpc_index, TRUE, FALSE);
    }

    pSaveDMA->CspFenceCmd.uint                      = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_CSP_FENCE, _RBT_3DFE,  HWM_SYNC_KMD_SLOT);
    pSaveDMA->CspFenceValue                         = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_END_DMA_CSPFENCE);
    pSaveDMA->CspWaitCmd.uint                       = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_END_DMA_CSPFENCE);
    pSaveDMA->CspWaitMainCmd.uint                   = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_END_DMA_CSPFENCE);

    pSaveDMA->GpcpBeFenceCmd.uint                   = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_FS_STO_FENCE, _RBT_3DFE,  HWM_SYNC_KMD_SLOT);
    pSaveDMA->GpcpBeFenceValue                      = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_END_DMA_STOFENCE);
    pSaveDMA->GpcpBeWaitCmd.uint                    = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_END_DMA_STOFENCE);
    pSaveDMA->GpcpBeWaitMainCmd.uint                = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_END_DMA_STOFENCE);   

    pCacheFlushDMA3DL->DZS_Flush.Type               = BLOCK_COMMAND_TEMPLATE_TYPE_FLUSH;
    pCacheFlushDMA3DL->DZS_Flush.Target             = BLOCK_COMMAND_FLUSH_TARGET_ALL_C;
    pCacheFlushDMA3DL->DZS_Flush.Block_Id           = FF_BLOCK;
    pCacheFlushDMA3DL->DZS_Flush.Major_Opcode       = CSP_OPCODE_Block_Command_Flush;

    pCacheFlushDMA3DL->DZS_Invalidate.Type          = BLOCK_COMMAND_TEMPLATE_TYPE_INVALIDATE_CACHE;
    pCacheFlushDMA3DL->DZS_Invalidate.Target        = BLOCK_COMMAND_FLUSH_TARGET_ALL_C;
    pCacheFlushDMA3DL->DZS_Invalidate.Block_Id      = FF_BLOCK;
    pCacheFlushDMA3DL->DZS_Invalidate.Major_Opcode  = CSP_OPCODE_Block_Command_Flush;

    pCacheFlushDMA3DL->UAVFE_Flush.uint             = SEND_UCACHE_3DFE_FLUSH_COMMAND_E3K();
    pCacheFlushDMA3DL->UAVFE_Invalidate.uint        = SEND_UCACHE_3DFE_INVALIDATE_COMMAND_E3K();
    pCacheFlushDMA3DL->UAVBE_Flush.uint             = SEND_UCACHE_3DBE_FLUSH_COMMAND_E3K();
    pCacheFlushDMA3DL->UAVBE_Invalidate.uint        = SEND_UCACHE_3DBE_INVALIDATE_COMMAND_E3K();


    pCacheFlushDMA3DL->L2Flush.Block_Id             = L2_BLOCK;
    pCacheFlushDMA3DL->L2Flush.Usage                = BLOCK_COMMAND_L2_USAGE_ALL;
    pCacheFlushDMA3DL->L2Flush.Type                 = BLOCK_COMMAND_L2_TYPE_FLUSH_L2;
    pCacheFlushDMA3DL->L2Flush.Major_Opcode         = CSP_OPCODE_Block_Command_L2;
    pCacheFlushDMA3DL->L2Flush.Dwc                  = 0;

    pCacheFlushDMA3DL->L2Invalidate.Block_Id        = L2_BLOCK;
    pCacheFlushDMA3DL->L2Invalidate.Usage           = BLOCK_COMMAND_L2_USAGE_ALL;
    pCacheFlushDMA3DL->L2Invalidate.Type            = BLOCK_COMMAND_L2_TYPE_INVALIDATE_L2;
    pCacheFlushDMA3DL->L2Invalidate.Major_Opcode    = CSP_OPCODE_Block_Command_L2;
    pCacheFlushDMA3DL->L2Invalidate.Dwc             = 0;

    pCacheFlushDMA3DL->TUFEL1Invalidate.uint        = SEND_TUFE_L1CACHE_INVALIDATE_COMMAND_E3K();
    pCacheFlushDMA3DL->TUBEL1Invalidate.uint        = SEND_TUBE_L1CACHE_INVALIDATE_COMMAND_E3K();

    pCacheFlushDMA3DL->EUFSL1Invalidate.uint        = SEND_FS_L1I_INVALIDATE_COMMAND_E3K();
    pCacheFlushDMA3DL->EUPSL1Invalidate.uint        = SEND_PS_L1I_INVALIDATE_COMMAND_E3K();

    pCacheFlushDMA3DL->PreUAVFEFenceCmd.uint = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_UAVFE_FENCE, _RBT_3DFE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->PreUAVFEFenceValue = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_UAVFEFENCE);
    pCacheFlushDMA3DL->PreUAVFEWaitCmd.uint = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_UAVFEFENCE);
    pCacheFlushDMA3DL->PreUAVFEWaitMainCmd.uint = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_UAVFEFENCE);

    pCacheFlushDMA3DL->PreUAVBEFenceCmd.uint = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_UAVBE_FENCE, _RBT_3DBE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->PreUAVBEFenceValue = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_UAVBEFENCE);
    pCacheFlushDMA3DL->PreUAVBEWaitCmd.uint = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_UAVBEFENCE);
    pCacheFlushDMA3DL->PreUAVBEWaitMainCmd.uint = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_UAVBEFENCE);

    pCacheFlushDMA3DL->PreDFenceCmd.uint = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_D_FENCE, _RBT_3DBE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->PreDFenceValue = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_DFENCE);
    pCacheFlushDMA3DL->PreDWaitCmd.uint = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_DFENCE);
    pCacheFlushDMA3DL->PreDWaitMainCmd.uint = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_DFENCE);


    pCacheFlushDMA3DL->PreZFenceCmd.uint = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_Z_FENCE, _RBT_3DBE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->PreZFenceValue = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_ZFENCE);
    pCacheFlushDMA3DL->PreZWaitCmd.uint = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_ZFENCE);
    pCacheFlushDMA3DL->PreZWaitMainCmd.uint = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_ZFENCE);

    //post is totally same as pre ones.
    pCacheFlushDMA3DL->PostUAVFEFenceCmd.uint       = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_UAVFE_FENCE,_RBT_3DFE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->PostUAVFEFenceValue          = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_UAVFEFENCE);
    pCacheFlushDMA3DL->PostUAVFEWaitCmd.uint        = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_UAVFEFENCE);
    pCacheFlushDMA3DL->PostUAVFEWaitMainCmd.uint    = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_UAVFEFENCE);

    pCacheFlushDMA3DL->PostUAVBEFenceCmd.uint       = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_UAVBE_FENCE,_RBT_3DBE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->PostUAVBEFenceValue          = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_UAVBEFENCE);
    pCacheFlushDMA3DL->PostUAVBEWaitCmd.uint        = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_UAVBEFENCE);
    pCacheFlushDMA3DL->PostUAVBEWaitMainCmd.uint    = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_UAVBEFENCE);

    pCacheFlushDMA3DL->PostDFenceCmd.uint           = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_D_FENCE,_RBT_3DBE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->PostDFenceValue              = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_DFENCE);
    pCacheFlushDMA3DL->PostDWaitCmd.uint            = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_DFENCE);
    pCacheFlushDMA3DL->PostDWaitMainCmd.uint        = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_DFENCE);

    pCacheFlushDMA3DL->PostZFenceCmd.uint           = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_Z_FENCE,_RBT_3DBE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->PostZFenceValue              = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_ZFENCE);
    pCacheFlushDMA3DL->PostZWaitCmd.uint            = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_ZFENCE);
    pCacheFlushDMA3DL->PostZWaitMainCmd.uint        = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_ZFENCE);

    //sto
    pCacheFlushDMA3DL->StoFenceCmd.uint             = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_FS_STO_FENCE, _RBT_3DFE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->StoFenceValue                = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_STOFENCE);
    pCacheFlushDMA3DL->StoWaitCmd.uint              = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_STOFENCE);
    pCacheFlushDMA3DL->StoWaitMainCmd.uint          = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_STOFENCE);

    pCacheFlushDMA3DL->L2FenceCmd.uint              = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_L2_FENCE,_RBT_3DBE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->L2FenceValue                 = FENCE_VALUE_CACHE_DMA_L2C_FENCE;

    pCacheFlushDMA3DL->L2WaitCmd.uint               = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_L2C_FENCE); 
    pCacheFlushDMA3DL->L2WaitMainCmd.uint           = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_L2C_FENCE); 


    /*we need first flush the cache, ensure last write will flush out to memory*/
    pCacheFlushDMA3DL->PreCspFenceCmd.uint          = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_CSP_FENCE, _RBT_3DFE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->PreCspFenceValue             = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_PRECSPFENCE);
    pCacheFlushDMA3DL->PreCspWaitCmd.uint           = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_PRECSPFENCE);
    pCacheFlushDMA3DL->PreCspWaitMainCmd.uint       = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_PRECSPFENCE);

    regValue                                        = SEND_FLAGBUFFER_FLUSH_COMMAND_E3K();
    pCacheFlushDMA3DL->BLC_Flush                    = *(Cmd_Block_Command_Mxu*)&regValue;
    pCacheFlushDMA3DL->FlushAddr_L                  = 0;
    pCacheFlushDMA3DL->FlushAddr_H                  = 0;
    pCacheFlushDMA3DL->FlushMask_L                  = 0;
    pCacheFlushDMA3DL->FlushMask_H                  = 0;

    pCacheFlushDMA3DL->PostCspFenceCmd.uint         = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_CSP_FENCE,_RBT_3DFE, HWM_SYNC_KMD_SLOT);
    pCacheFlushDMA3DL->PostCspFenceValue            = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_POSTCSPFENCE);
    pCacheFlushDMA3DL->PostCspWaitCmd.uint          = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_POSTCSPFENCE);
    pCacheFlushDMA3DL->PostCspWaitMainCmd.uint      = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_POSTCSPFENCE);

    share->begin_end_vma = vma;
    
}

void enginei_init_ring_buffer_commands_e3k(engine_gfx_e3k_t *engine)
{
    engine_share_e3k_t *share = engine->common.share;
    RB_PREDEFINE_DMA *pCmdBuf = share->begin_end_vma->virt_addr;
    large_inter dwSaveRestoreGPUBaseAddr;
    Cmd_Dma_Address_Dword2   cmd_dma_address_dword2     = {0};    
    RINGBUFFER_COMMANDS_E3K* pRingBufferCommands = (RINGBUFFER_COMMANDS_E3K*)share->RingBufferCommands;
    RINGBUFFER_COMMANDS_E3K* p0                         = &pRingBufferCommands[SUBMIT_CMD_HWCONTEX_SWITCH];
    RINGBUFFER_COMMANDS_E3K* p1                         = &pRingBufferCommands[SUBMIT_CMD_NO_HWCONTEXT_SWITCH];
    RINGBUFFER_COMMANDS_E3K* p2                         = &pRingBufferCommands[SUBMIT_CMD_FENCE_ONLY];
    int i = 0;
    large_inter TempAddr;        
    
    dwSaveRestoreGPUBaseAddr.quad64 = share->begin_end_buffer_phy_addr;


    gf_memset( p0, 0, sizeof(*p0) );
    gf_memset( p1, 0, sizeof(*p1) );
    gf_memset( p2, 0, sizeof(*p2) );

    //case 0
    p0->c0.pwmTrigger.Major_Opcode         = CSP_OPCODE_Blk_Cmd_Csp_Indicator;
    p0->c0.pwmTrigger.Block_Id             = CSP_GLOBAL_BLOCK;
    p0->c0.pwmTrigger.Type                 = BLOCK_COMMAND_CSP_TYPE_INDICATOR;
    p0->c0.pwmTrigger.Info                 = BLK_CMD_CSP_INDICATOR_INFO_3D_MODE;
    p0->c0.pwmTrigger.Dwc                  =  1;

    p0->c0.RestoreDMA.Major_Opcode         = CSP_OPCODE_Dma;
    p0->c0.RestoreDMA.Special_Dma_Type     = DMA_SPECIAL_DMA_TYPE_SAVE_REST_CMD;
    p0->c0.RestoreDMA.Mode                 = DMA_MODE_RESTORE;
    p0->c0.RestoreDMA.Dw_Num               = sizeof(CONTEXT_RESTORE_DMA_E3K) >> 2;
    p0->c0.RestoreDMA.Reset_Dma            = 1;

    TempAddr.quad64  = dwSaveRestoreGPUBaseAddr.quad64 + ((BYTE*)&(pCmdBuf->RestoreDMA) - (BYTE*)pCmdBuf);
    p0->c0.RestoreDMA_Address_L            = TempAddr.low32;

    cmd_dma_address_dword2.Address_High8 = TempAddr.high32 & 0xFF;
    cmd_dma_address_dword2.L2_Cachable     = 1;

    p0->c0.RestoreDMA_Address_H            = *(DWORD*)&cmd_dma_address_dword2;

    p0->c0.CommandDMA.Major_Opcode         = CSP_OPCODE_Dma;
    p0->c0.CommandDMA.Mode                 = DMA_MODE_NORMAL;

    p0->c0.CacheFlushDMA.Major_Opcode      = CSP_OPCODE_Dma;
    p0->c0.CacheFlushDMA.Dw_Num            = sizeof(CACHEFLUSH_DMA_E3K) >> 2;
    p0->c0.CacheFlushDMA.Mode              = DMA_MODE_FLUSH;
    p0->c0.CacheFlushDMA.Reset_Dma         = 1;

    TempAddr.quad64  = dwSaveRestoreGPUBaseAddr.quad64 + ((BYTE*)(&(pCmdBuf->FlushDMA3DL)) - (BYTE*)pCmdBuf);
    p0->c0.CacheFlushDMA_Address_L         = TempAddr.low32 ;
    p0->c0.CacheFlushDMA_Address_H         = TempAddr.high32 & 0xFF;

    p0->c0.SaveDMA.Major_Opcode            = CSP_OPCODE_Dma;
    p0->c0.SaveDMA.Dw_Num                  = sizeof(CONTEXT_SAVE_DMA_E3K) >> 2;
    p0->c0.SaveDMA.Mode                    = DMA_MODE_SAVE;
    p0->c0.SaveDMA.Special_Dma_Type        = DMA_SPECIAL_DMA_TYPE_SAVE_REST_CMD;
    p0->c0.SaveDMA.Reset_Dma               = 1;

    TempAddr.quad64  = dwSaveRestoreGPUBaseAddr.quad64 + ((BYTE*)&(pCmdBuf->SaveDMA) - (BYTE*)pCmdBuf);

    cmd_dma_address_dword2.Address_High8   = TempAddr.high32 & 0xFF;
    cmd_dma_address_dword2.L2_Cachable     = 1;    

    p0->c0.SaveDMA_Address_L               = TempAddr.low32;
    p0->c0.SaveDMA_Address_H               = *(DWORD*)&cmd_dma_address_dword2;

    for(i = 0;i<sizeof(p0->c0.Skip_1)/sizeof(DWORD);i++)
    {
        p0->c0.Skip_1[i] = SEND_SKIP_E3K(1);
    }

    *(DWORD*)(&p0->c0.Fence)               = SEND_EXTERNAL_FENCE_E3K(FENCE_IRQ_INTERRUPT_CPU);
    *(DWORD*)(&p0->c0.Fence_1)             = SEND_EXTERNAL_FENCE_E3K(FENCE_IRQ_INTERRUPT_CPU);

    p0->c0.pwmTriggerOff.Major_Opcode      = CSP_OPCODE_Blk_Cmd_Csp_Indicator;
    p0->c0.pwmTriggerOff.Block_Id          = CSP_GLOBAL_BLOCK;
    p0->c0.pwmTriggerOff.Type              = BLOCK_COMMAND_CSP_TYPE_INDICATOR;
    p0->c0.pwmTriggerOff.Info              = BLK_CMD_CSP_INDICATOR_INFO_OFF_MODE;
    p0->c0.pwmTriggerOff.Dwc                  =  1;

    for (i = 0; i < sizeof(p0->c0.Skip_2) / sizeof(DWORD); i++)
    {
        p0->c0.Skip_2[i] = SEND_SKIP_E3K(1);
    }

    //util_dump_memory(p0,sizeof(RINGBUFFER_COMMANDS_E3K),"ring buffer template");
    //case 1
    p1->c1.pwmTrigger                      = p0->c0.pwmTrigger;
    p1->c1.Fence                           = p0->c0.Fence;
    p1->c1.Fence_1                         = p0->c0.Fence_1;	

    for(i = 0;i<sizeof(p1->c1.Skip_1)/sizeof(DWORD);i++)
    {
        p1->c1.Skip_1[i] = SEND_SKIP_E3K(1);
    }
    
    p1->c1.CommandDMA                      = p0->c0.CommandDMA;

    p1->c1.CacheFlushDMA                   = p0->c0.CacheFlushDMA;
    p1->c1.CacheFlushDMA_Address_L         = p0->c0.CacheFlushDMA_Address_L;
    p1->c1.CacheFlushDMA_Address_H         = p0->c0.CacheFlushDMA_Address_H;

    p1->c1.Skip_2.Major_Opcode             = CSP_OPCODE_Skip;
    p1->c1.Skip_2.Dwc                      = sizeof(p1->c1.Dummy_2)/sizeof(DWORD);

    p1->c1.pwmTriggerOff                   = p0->c0.pwmTriggerOff;

    for (i = 0; i < sizeof(p1->c1.Skip_3) / sizeof(DWORD); i++)
    {
        p1->c1.Skip_3[i] = SEND_SKIP_E3K(1);
    }

    //case 2
    p2->c2.pwmTrigger                      = p1->c1.pwmTrigger;

    for(i = 0;i<sizeof(p2->c2.Skip_1)/sizeof(DWORD);i++)
    {
        p2->c2.Skip_1[i] = SEND_SKIP_E3K(1);
    }

    p2->c2.Fence                           = p1->c1.Fence;
    p2->c2.Fence_1                         = p1->c1.Fence_1;

    p2->c2.pwmTriggerOff                   = p1->c1.pwmTriggerOff;

    for (i = 0; i < sizeof(p2->c2.Skip_2) / sizeof(DWORD); i++)
    {
        p2->c2.Skip_2[i] = SEND_SKIP_E3K(1);
    }

}




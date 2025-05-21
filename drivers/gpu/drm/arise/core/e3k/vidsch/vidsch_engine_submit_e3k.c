/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
#include "gf_adapter.h"
#include "vidsch.h"
#include "vidschi.h"
#include "vidsch_engine_e3k.h"
#include "vidsch_debug_hang_e3k.h"
#include "ring_buffer.h"
#include "perfevent.h"

reg_debug_mode_e3k debug_mode_e3k = {0};
extern void test_manual_mode(adapter_t * adapter);

static int enginei_ring_buffer_wrap_e3k(ring_buffer_t *ring, int skip_cnt, void *argu)
{
    engine_e3k_t    *engine     = argu;
    unsigned int    RbIndex     = engine->vidsch->engine_index;
    adapter_t         *adapter    = engine->vidsch->adapter;
    unsigned int    head_reg_off = EngineRbOffset(RbIndex);
    unsigned int    *cmd        = engine->ring_buf_virt_addr + (ring->tail >> 2);

    if(0 && (skip_cnt > 7*4) && RbIndex != RB_INDEX_VCP0 &&
        RbIndex != RB_INDEX_VCP1 && RbIndex != RB_INDEX_VPP)//For raw skip cmd need add pwdIndicator, for High need add TBR indicator.
    {
        Csp_Opcodes_cmd                     skip_cmd         = {0};
        Csp_Opcodes_cmd                     cmdTbr           = {0};
        Cmd_Blk_Cmd_Csp_Indicator           cmdPwmTrigger    = {0};
        Cmd_Blk_Cmd_Csp_Indicator_Dword1    cmdTrigger_Dw    = {0};
        unsigned int                        dwSkip           = (RbIndex == RB_INDEX_GFXH)?((skip_cnt >> 2) - 7):((skip_cnt >> 2) - 5);

        cmdPwmTrigger.Major_Opcode         = CSP_OPCODE_Blk_Cmd_Csp_Indicator;
        cmdPwmTrigger.Block_Id             = CSP_GLOBAL_BLOCK;
        cmdPwmTrigger.Type                 = BLOCK_COMMAND_CSP_TYPE_INDICATOR;
        cmdPwmTrigger.Info                 = BLK_CMD_CSP_INDICATOR_INFO_3D_MODE;
        cmdPwmTrigger.Dwc                  = 1;

        cmdTrigger_Dw.Slice_Mask = adapter->hw_caps.chip_slice_mask;

        skip_cmd.cmd_Skip.Dwc = dwSkip;
        skip_cmd.cmd_Skip.Major_Opcode = CSP_OPCODE_Skip;

        if(RbIndex == RB_INDEX_GFXH)//add TBR indicator for high
        {
            cmdTbr.cmd_Tbr_Indicator.Skip_En = 1;
            cmdTbr.cmd_Tbr_Indicator.Indicator_Info = TBR_INDICATOR_INDICATOR_INFO_END;
            cmdTbr.cmd_Tbr_Indicator.Block_Id = TASBE_BLOCK;
            cmdTbr.cmd_Tbr_Indicator.Major_Opcode = 0xF;

            *cmd++ = cmdTbr.uint;
        }

        *cmd++ = *(DWORD*)&cmdPwmTrigger;
        *cmd++ = *(DWORD*)&cmdTrigger_Dw;

        *cmd++ = skip_cmd.uint;
        cmd   += dwSkip;

        if(RbIndex != RB_INDEX_GFXH)// only high leave with pwd indicator on.
        {
            cmdPwmTrigger.Info       = BLK_CMD_CSP_INDICATOR_INFO_OFF_MODE;
            cmdTrigger_Dw.Slice_Mask = 0;
        }

        *cmd++ = *(DWORD*)&cmdPwmTrigger;
        *cmd++ = *(DWORD*)&cmdTrigger_Dw;

        if(RbIndex == RB_INDEX_GFXH)
        {
            cmdTbr.cmd_Tbr_Indicator.Indicator_Info = TBR_INDICATOR_INDICATOR_INFO_BEGIN;

            *cmd++ = cmdTbr.uint;
        }
    }
    else if(skip_cnt > 0)//not enough for store indicators.
    {
       gf_memset(cmd, 0, skip_cnt);
    }

    return gf_read32(adapter->mmio + (head_reg_off + 2)*4 + MMIO_CSP_START_ADDRESS);
}

void enginei_enable_ring_buffer_e3k(engine_e3k_t *engine)
{
    adapter_t                  *adapter               = engine->vidsch->adapter;
    unsigned int RbIndex = engine->vidsch->engine_index;
    Reg_Run_List_Ctx_Addr1      reg_Run_List_Ctx_Addr1      = { 0 };
    Reg_Ring_Buf_Size           reg_Ring_Buf_Size           = { 0 };
    Reg_Ring_Buf_Head           reg_Ring_Buf_Head           = { 0 };
    Reg_Ring_Buf_Tail           reg_Ring_Buf_Tail           = { 0 };
    unsigned int                RegRbOffset = EngineRbOffset(RbIndex);

    ring_buffer_init(&engine->ring, RING_BUFFER_SIZE_E3K, 0, enginei_ring_buffer_wrap_e3k, engine);

    reg_Ring_Buf_Tail.reg.Rb_Tail =
    reg_Ring_Buf_Head.reg.Rb_Head = 0;
    reg_Ring_Buf_Size.reg.Rb_Size = RING_BUFFER_SIZE_E3K;

    reg_Run_List_Ctx_Addr1.reg.Addr = (engine->ring_buf_phys_addr >> 12) & 0xFFFFFFF;
    reg_Run_List_Ctx_Addr1.reg.Kickoff = 0;

    gf_write32(adapter->mmio + RegRbOffset     * 4 + MMIO_CSP_START_ADDRESS, reg_Run_List_Ctx_Addr1.uint);
    gf_write32(adapter->mmio + (RegRbOffset+1) * 4 + MMIO_CSP_START_ADDRESS, reg_Ring_Buf_Size.uint );
    gf_write32(adapter->mmio + (RegRbOffset+2) * 4 + MMIO_CSP_START_ADDRESS, reg_Ring_Buf_Head.uint );
    gf_write32(adapter->mmio + (RegRbOffset+3) * 4 + MMIO_CSP_START_ADDRESS, reg_Ring_Buf_Tail.uint );

    reg_Run_List_Ctx_Addr1.reg.Kickoff = 1;

    gf_write32( adapter->mmio + RegRbOffset * 4 + MMIO_CSP_START_ADDRESS, reg_Run_List_Ctx_Addr1.uint );
}

unsigned int *enginei_get_ring_buffer_space_e3k(engine_e3k_t *engine, int size_uint)
{
    unsigned int *ring_buffer = engine->ring_buf_virt_addr;
    int offset = -1;

    while(offset == -1)
    {
        offset = ring_buffer_get_space(&engine->ring, size_uint << 2);
    }

#if GF_TRACE_HW_HANG
    engine->last_ring_buffer   = ring_buffer + (offset >> 2);
    engine->last_ring_buffer_size = (size_uint << 2);
#endif

   // gf_info("idx:%d, offset:%x, end:%x\n", engine->vidsch->engine_index, offset, offset + (size_uint <<2));
    return ring_buffer + (offset >> 2);
}

void enginei_do_kickoff_cmd_e3k(engine_e3k_t *engine)
{
    adapter_t *adapter   = engine->vidsch->adapter;
    engine_share_e3k_t  *share = (engine_share_e3k_t*)adapter->private_data;
    unsigned int RbIndex       = engine->vidsch->engine_index;
    unsigned int RegRbOffset = EngineRbOffset(RbIndex);
    unsigned int offset = 0;
    vidsch_mgr_t *sch_mgr = engine->vidsch;
    int g_count = 0;
    static int print_once[2] = {0, 0};

    gf_mb();

    {
        vidsch_mgr_t *vidsch = engine->vidsch;

        *((volatile unsigned int*)share->flush_fifo_buffer->vma->virt_addr + RbIndex) =  (unsigned int)vidsch->last_send_fence_id;

        while(*((volatile unsigned int*)share->flush_fifo_buffer->vma->virt_addr + RbIndex) !=  (unsigned int)vidsch->last_send_fence_id)
        {
           if (print_once[0] == 0)
           {
#ifdef _DEBUG_
                //gf_warning("flush fifo index:%x\n",RbIndex);
                gf_warning("krnl virt addr: %p, write value: %x, read value: %x\n",
                    ((volatile unsigned int*)share->flush_fifo_buffer->vma->virt_addr + RbIndex),
                    (unsigned int)vidsch->last_send_fence_id,
                    *((volatile unsigned int*)share->flush_fifo_buffer->vma->virt_addr + RbIndex));
#endif
                print_once[0] = 1;
           }

           adapter->kickoff_error.error |= 0x1;
           adapter->kickoff_error.RbIndex = RbIndex;
           adapter->kickoff_error.last_send_fence_id = (unsigned int)vidsch->last_send_fence_id;
           adapter->kickoff_error.flush_fifo_buffer_value = *((volatile unsigned int*)share->flush_fifo_buffer->vma->virt_addr + RbIndex);

           // gf_assert(0, NULL);
           if (g_count++ > 10)
           {
                break;
           }
        }
    }

    gf_mb();

    if (adapter->pwm_level.EnableClockGating)
    {
        unsigned long flags;
        flags = gf_spin_lock_irqsave(sch_mgr->power_status_lock);

        {
            if(sch_mgr->chip_func->power_clock)
            {
                //gf_info("power on: last_send_fence_id=%d. \n", sch_mgr->last_send_fence_id);
                sch_mgr->chip_func->power_clock(sch_mgr, FALSE);
                sch_mgr->engine_dvfs_power_on = TRUE;
            }
        }

        gf_write32(adapter->mmio +  (RegRbOffset+3) * 4 + MMIO_CSP_START_ADDRESS, engine->ring.tail);

        gf_spin_unlock_irqrestore(sch_mgr->power_status_lock, flags);
    }
    else
    {
        gf_write32(adapter->mmio +  (RegRbOffset+3) * 4 + MMIO_CSP_START_ADDRESS, engine->ring.tail);
    }

    gf_mb();

    g_count = 0;
    do{
        offset = gf_read32(adapter->mmio +  (RegRbOffset+3) * 4 + MMIO_CSP_START_ADDRESS);

        if(offset != engine->ring.tail)
        {
            if (print_once[1] == 0)
            {
#ifdef _DEBUG_
                gf_warning("idx:%d, kick off:%x, read:%x\n", RbIndex, engine->ring.tail, offset);
#endif
                print_once[1] = 1;
            }
            adapter->kickoff_error.error |= 0x2;
            adapter->kickoff_error.RbIndex = RbIndex;
            adapter->kickoff_error.tail = engine->ring.tail;
            adapter->kickoff_error.offset = offset;
        }

        if (g_count++ > 10)
        {
            break;
        }
    }while(engine->ring.tail != offset);
}

void enginei_invalidate_gart_e3k(engine_e3k_t *engine)
{
    adapter_t               *adapter    = engine->vidsch->adapter;
    vidmm_gart_table_info_t *gart_table = adapter->gart_table_L3;
    Reg_Pt_Inv_Trig reg_Pt_Inv_Trig                 = {0};
    Reg_Pt_Inv_Addr reg_Pt_Inv_Addr                 = {0};
    Reg_Pt_Inv_Mask reg_Pt_Inv_Mask                 = {0};

    DWORD               DtlbShift    = 0x10; //Per HW, one Dtlb Cachelline is 512bit, one cache line size maps to 16*4KB virtual address.
    reg_Pt_Inv_Trig.reg.Target       = PT_INV_TRIG_TARGET_DTLB;

    gf_mutex_lock(adapter->gart_table_lock);

    if(gart_table->dirty == FALSE)
    {
        gf_mutex_unlock(adapter->gart_table_lock);
        return;
    }

    reg_Pt_Inv_Addr.reg.Address = (gart_table->gart_table_dirty_addr>>DtlbShift)<<4;
    reg_Pt_Inv_Mask.reg.Mask    = ((~gart_table->gart_table_dirty_mask )>>DtlbShift)<<4;

    gf_write32(adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Pt_Inv_Addr_Offset*4 , reg_Pt_Inv_Addr.uint);
    gf_write32(adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Pt_Inv_Mask_Offset*4,  reg_Pt_Inv_Mask.uint);
    gf_write32(adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Pt_Inv_Trig_Offset*4, reg_Pt_Inv_Trig.uint);

    gart_table->gart_table_dirty_mask = 0;
    gart_table->gart_table_dirty_addr = 0;
    gart_table->dirty = FALSE;

    gf_mutex_unlock(adapter->gart_table_lock);
}

static int enginei_submit_to_gfx_high_e3k(engine_e3k_t *engine, task_dma_t *task)
{
    engine_gfx_e3k_t *engine_gfx = (engine_gfx_e3k_t*) engine;
    engine_share_e3k_t      *share   = engine->share;
    adapter_t               *adapter = engine->vidsch->adapter;
    hw_ctxbuf_t             *hwctx   = task->hw_ctx_info;
    unsigned int            dwDmaBufferSize = (task->submit_end_offset - task->submit_start_offset) >> 2;
    large_inter             fence_offset;
    large_inter             fence_offset_fake;

    large_inter             fence_offset_local;
    large_inter             fence_offset_local_fake;

    large_inter             fence_offset_snoop;
    large_inter             fence_offset_snoop_fake;

    large_inter             fence_id;
    large_inter             context_buf_phy_addr;
    large_inter             context_buf_phy_addr_for_low;
    unsigned int * pRB = NULL;
    unsigned int * pRB0 = NULL;
    unsigned int * pRB1 = NULL;
    Cmd_Blk_Cmd_Csp_Indicator_Dword1 trigger_Dw = {0};
    Csp_Opcodes_cmd         cmd                 = {0};
    RINGBUFFER_COMMANDS_E3K *pRingBufferCommands = (RINGBUFFER_COMMANDS_E3K*)share->RingBufferCommands;
    RB_PREDEFINE_DMA        *pPredefine         = (RB_PREDEFINE_DMA*)share->begin_end_vma->virt_addr;
    CONTEXT_RESTORE_DMA_E3K *pRestoreDMA        = &(pPredefine->RestoreDMA);
    CONTEXT_SAVE_DMA_E3K    *pSaveDMA           = &(pPredefine->SaveDMA);
    CACHEFLUSH_DMA_E3K      *pCacheFlushDMA3DL  = &(pPredefine->FlushDMA3DL);

    // TBR indicator + pwmTrigger followed + saveLowRBContext + restore high + dwDmaSize + dwCacheFlush + save high+ dwRestoreLowRBContext+ dwFence + pwmOff followed + TBR indicator;
    unsigned int dwRbSize = 2 +
                       + (sizeof(CONTEXT_SAVE_DMA_E3K) / sizeof(DWORD)) //for low
                       + dwDmaBufferSize
                       + (sizeof(CACHEFLUSH_DMA_E3K) / sizeof(DWORD)) //for high
                       + (sizeof(CONTEXT_RESTORE_DMA_E3K) / sizeof(DWORD)) //for low
                       + 5*6 //double fence cmd
                       + 2
                       + 2;

    if(task->need_hwctx_switch)
    {
        dwRbSize += (sizeof(CONTEXT_SAVE_DMA_E3K) / sizeof(DWORD))
                  + (sizeof(CONTEXT_RESTORE_DMA_E3K) / sizeof(DWORD));

        if (hwctx->is_initialized)
        {
            context_buf_phy_addr.quad64 = hwctx->context_buffer_address;
        }
        else
        {
            context_buf_phy_addr.quad64 = share->context_buf_phy_addr;
            hwctx->is_initialized   = TRUE;
        }
    }

    fence_offset.quad64 = engine->fence_buffer_phy_addr;
    fence_offset_fake.quad64 = engine->fence_buffer_phy_addr_fake;

    fence_offset_local.quad64 = engine->fence_buffer_phy_addr_local;
    fence_offset_local_fake.quad64 = engine->fence_buffer_phy_addr_local_fake;

    fence_offset_snoop.quad64 = engine->fence_buffer_phy_addr_snoop;
    fence_offset_snoop_fake.quad64 = engine->fence_buffer_phy_addr_snoop_fake;

    fence_id.quad64     = task->desc.fence_id;

    context_buf_phy_addr_for_low.quad64 = engine_gfx->context_buf_phy_addr;
    dwRbSize = ((dwRbSize + 15) & ~15);

    pRB0 =
    pRB = gf_calloc((dwRbSize <<2));
    gf_memset(pRB0, 0, dwRbSize << 2);

    if(!pRB)
    {
       gf_info("mem calloc failed for high engine\n");
       gf_assert(0, NULL);
       return S_OK;
    }

    pRB1 = enginei_get_ring_buffer_space_e3k(engine, dwRbSize);
    gf_memset(pRB1, 0, dwRbSize << 2);

    cmd.cmd_Tbr_Indicator.Skip_En = 1;
    cmd.cmd_Tbr_Indicator.Indicator_Info = TBR_INDICATOR_INDICATOR_INFO_END;
    cmd.cmd_Tbr_Indicator.Block_Id = TASBE_BLOCK;
    cmd.cmd_Tbr_Indicator.Major_Opcode = 0xF;

    //1. pwmTrigger;
    trigger_Dw.Slice_Mask = adapter->hw_caps.chip_slice_mask;

    *pRB++ = cmd.uint;
    *pRB++ = *(DWORD*)&pRingBufferCommands->c1.pwmTrigger;
    *pRB++ = *(DWORD*)&trigger_Dw;

   //2. Save LowRB context;
    gf_memcpy(pRB, pSaveDMA, sizeof(CONTEXT_SAVE_DMA_E3K));

    ((CONTEXT_SAVE_DMA_E3K*)pRB)->SaveContext.Address_Mode = SET_REGISTER_ADDRESS_MODE_ADDRESS;
    ((CONTEXT_SAVE_DMA_E3K*)pRB)->ContextBufferOffset_H = context_buf_phy_addr_for_low.high32 & 0xFF;
    ((CONTEXT_SAVE_DMA_E3K*)pRB)->ContextBufferOffset_L = context_buf_phy_addr_for_low.low32;

    ((CONTEXT_SAVE_DMA_E3K*)pRB)->SaveCSPRegister.uint = SEND_QUERY_DUMP_E3K(CSP_GLOBAL_BLOCK, sizeof(CSP_CONTEXTBUFFER_REGS)>>2, QUERY_DUMP_ADDRESS_MODE_ADDRESS);
    ((CONTEXT_SAVE_DMA_E3K*)pRB)->CSPOffset_L = SEND_QUERY_ADDR1_E3K(context_buf_phy_addr_for_low.low32 + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->CSPRegs)));
    ((CONTEXT_SAVE_DMA_E3K*)pRB)->CSPOffset_H = SEND_QUERY_ADDR2_AND_OFFSET_E3K(context_buf_phy_addr_for_low.high32 & 0xFF, Reg_Csp_Ms_Total_Gpu_Timestamp_Offset, TRUE, FALSE);

    ((CONTEXT_SAVE_DMA_E3K*)pRB)->SaveGpcpFeRegister.uint = SEND_QUERY_DUMP_E3K(GPCPFE_BLOCK, sizeof(GPCPFE_CONTEXTBUFFER_REGS)>>2, QUERY_DUMP_ADDRESS_MODE_ADDRESS);
    ((CONTEXT_SAVE_DMA_E3K*)pRB)->GpcpFeOffset_L = SEND_QUERY_ADDR1_E3K(context_buf_phy_addr_for_low.low32 + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpFeRegs)));
    ((CONTEXT_SAVE_DMA_E3K*)pRB)->GpcpFeOffset_H = SEND_QUERY_ADDR2_AND_OFFSET_E3K(context_buf_phy_addr_for_low.high32 & 0xFF, Reg_Gpcpfe_Iidcnt_Offset, TRUE, FALSE);

    ((CONTEXT_SAVE_DMA_E3K*)pRB)->SaveGpcpBeRegister.uint = SEND_QUERY_DUMP_E3K(GPCPBE_BLOCK, sizeof(Gpcpbe_regs)>>2, QUERY_DUMP_ADDRESS_MODE_ADDRESS);
    ((CONTEXT_SAVE_DMA_E3K*)pRB)->GpcpBeOffset_L = SEND_QUERY_ADDR1_E3K(context_buf_phy_addr_for_low.low32 + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpBeRegs)));
    ((CONTEXT_SAVE_DMA_E3K*)pRB)->GpcpBeOffset_H = SEND_QUERY_ADDR2_AND_OFFSET_E3K(context_buf_phy_addr_for_low.high32 & 0xFF, Reg_Sto_Cfg_Offset, TRUE, FALSE);

    pRB += sizeof(CONTEXT_SAVE_DMA_E3K) >> 2;

    //3. restore high context
    if(task->need_hwctx_switch)
    {
        gf_memcpy(pRB, pRestoreDMA, sizeof(CONTEXT_RESTORE_DMA_E3K));

        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->RestoreContext.Address_Mode = SET_REGISTER_ADDRESS_MODE_ADDRESS;
        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->ContextBufferOffset_H = context_buf_phy_addr.high32 & 0xFF;
        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->ContextBufferOffset_L = context_buf_phy_addr.low32;

        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->RestoreCsp.uint = SET_REGISTER_ADDR_E3K(CSP_GLOBAL_BLOCK,Reg_Csp_Ms_Total_Gpu_Timestamp_Offset,SET_REGISTER_ADDRESS_MODE_ADDRESS);
        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->CSPOffset_L = SET_REGISTER_ADDR_LOW_E3K(context_buf_phy_addr.low32 + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->CSPRegs)));
        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->CSPOffset_H = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(context_buf_phy_addr.high32 & 0xFF, sizeof(CSP_CONTEXTBUFFER_REGS) >> 2);

        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->RestoreGpcpFe.uint = SET_REGISTER_ADDR_E3K(GPCPFE_BLOCK,Reg_Gpcpfe_Iidcnt_Offset,SET_REGISTER_ADDRESS_MODE_ADDRESS);
        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->GpcpFeOffset_L = SET_REGISTER_ADDR_LOW_E3K(context_buf_phy_addr.low32 + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpFeRegs)));
        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->GpcpFeOffset_H = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(context_buf_phy_addr.high32 & 0xFF, sizeof(GPCPFE_CONTEXTBUFFER_REGS) >> 2);

        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->RestoreGpcpBe.uint = SET_REGISTER_ADDR_E3K(GPCPBE_BLOCK, Reg_Sto_Cfg_Offset, QUERY_DUMP_ADDRESS_MODE_ADDRESS);
        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->GpcpBeOffset_L     = SET_REGISTER_ADDR_LOW_E3K(context_buf_phy_addr.low32+ ((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpBeRegs))));
        ((CONTEXT_RESTORE_DMA_E3K*)pRB)->GpcpBeOffset_H     = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(context_buf_phy_addr.high32 & 0xFF, sizeof(Gpcpbe_regs)>>2);

        pRB += sizeof(CONTEXT_RESTORE_DMA_E3K) >> 2;
    }

    //4. DmaCommand
    gf_memcpy(pRB, task->dma_buffer_node->virt_base_addr + task->submit_start_offset, dwDmaBufferSize << 2);
    pRB += dwDmaBufferSize;

    //5. CacheFlush, all pipe line
    gf_memcpy(pRB, pCacheFlushDMA3DL, sizeof(CACHEFLUSH_DMA_E3K));
    pRB += sizeof(CACHEFLUSH_DMA_E3K) >> 2;

    //6 save high context.
    if(task->need_hwctx_switch)
    {
        gf_memcpy(pRB, pSaveDMA, sizeof(CONTEXT_SAVE_DMA_E3K));

        ((CONTEXT_SAVE_DMA_E3K*)pRB)->SaveContext.Address_Mode = SET_REGISTER_ADDRESS_MODE_ADDRESS;
        ((CONTEXT_SAVE_DMA_E3K*)pRB)->ContextBufferOffset_H = (hwctx->context_buffer_address >>32) & 0xFF;
        ((CONTEXT_SAVE_DMA_E3K*)pRB)->ContextBufferOffset_L = hwctx->context_buffer_address & 0xFFFFFFFF;

        ((CONTEXT_SAVE_DMA_E3K*)pRB)->SaveCSPRegister.uint = SEND_QUERY_DUMP_E3K(CSP_GLOBAL_BLOCK, sizeof(CSP_CONTEXTBUFFER_REGS)>>2, QUERY_DUMP_ADDRESS_MODE_ADDRESS);
        ((CONTEXT_SAVE_DMA_E3K*)pRB)->CSPOffset_L = SEND_QUERY_ADDR1_E3K((hwctx->context_buffer_address & 0xFFFFFFFF) + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->CSPRegs)));
        ((CONTEXT_SAVE_DMA_E3K*)pRB)->CSPOffset_H = SEND_QUERY_ADDR2_AND_OFFSET_E3K((hwctx->context_buffer_address >>32) & 0xFF, Reg_Csp_Ms_Total_Gpu_Timestamp_Offset, TRUE, FALSE);

        ((CONTEXT_SAVE_DMA_E3K*)pRB)->SaveGpcpFeRegister.uint = SEND_QUERY_DUMP_E3K(GPCPFE_BLOCK, sizeof(GPCPFE_CONTEXTBUFFER_REGS)>>2, QUERY_DUMP_ADDRESS_MODE_ADDRESS);
        ((CONTEXT_SAVE_DMA_E3K*)pRB)->GpcpFeOffset_L = SEND_QUERY_ADDR1_E3K((hwctx->context_buffer_address & 0xFFFFFFFF) + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpFeRegs)));
        ((CONTEXT_SAVE_DMA_E3K*)pRB)->GpcpFeOffset_H = SEND_QUERY_ADDR2_AND_OFFSET_E3K((hwctx->context_buffer_address >>32) & 0xFF, Reg_Gpcpfe_Iidcnt_Offset, TRUE, FALSE);

        ((CONTEXT_SAVE_DMA_E3K*)pRB)->SaveGpcpBeRegister.uint = SEND_QUERY_DUMP_E3K(GPCPBE_BLOCK, sizeof(Gpcpbe_regs)>>2, QUERY_DUMP_ADDRESS_MODE_ADDRESS);
        ((CONTEXT_SAVE_DMA_E3K*)pRB)->GpcpBeOffset_L = SEND_QUERY_ADDR1_E3K((hwctx->context_buffer_address & 0xFFFFFFFF) + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpBeRegs)));
        ((CONTEXT_SAVE_DMA_E3K*)pRB)->GpcpBeOffset_H = SEND_QUERY_ADDR2_AND_OFFSET_E3K((hwctx->context_buffer_address >> 32) & 0xFF, Reg_Sto_Cfg_Offset, TRUE, FALSE);

        pRB += sizeof(CONTEXT_SAVE_DMA_E3K) >> 2;
   }

    //7. Restore LowRB context;
    gf_memcpy(pRB, pRestoreDMA, sizeof(CONTEXT_RESTORE_DMA_E3K));

    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->RestoreContext.Address_Mode = SET_REGISTER_ADDRESS_MODE_ADDRESS;
    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->ContextBufferOffset_H = context_buf_phy_addr_for_low.high32 & 0xFF;
    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->ContextBufferOffset_L = context_buf_phy_addr_for_low.low32;

    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->RestoreCsp.uint = SET_REGISTER_ADDR_E3K(CSP_GLOBAL_BLOCK,Reg_Csp_Ms_Total_Gpu_Timestamp_Offset,SET_REGISTER_ADDRESS_MODE_ADDRESS);
    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->CSPOffset_L = SET_REGISTER_ADDR_LOW_E3K(context_buf_phy_addr_for_low.low32 + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->CSPRegs)));
    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->CSPOffset_H = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(context_buf_phy_addr_for_low.high32 & 0xFF, sizeof(CSP_CONTEXTBUFFER_REGS) >> 2);

    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->RestoreGpcpFe.uint = SET_REGISTER_ADDR_E3K(GPCPFE_BLOCK,Reg_Gpcpfe_Iidcnt_Offset,SET_REGISTER_ADDRESS_MODE_ADDRESS);
    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->GpcpFeOffset_L = SET_REGISTER_ADDR_LOW_E3K(context_buf_phy_addr_for_low.low32 + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpFeRegs)));
    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->GpcpFeOffset_H = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(context_buf_phy_addr_for_low.high32 & 0xFF, sizeof(GPCPFE_CONTEXTBUFFER_REGS) >> 2);

    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->RestoreGpcpBe.uint = SET_REGISTER_ADDR_E3K(GPCPBE_BLOCK, Reg_Sto_Cfg_Offset, QUERY_DUMP_ADDRESS_MODE_ADDRESS);
    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->GpcpBeOffset_L     = SET_REGISTER_ADDR_LOW_E3K(context_buf_phy_addr_for_low.low32 + ((unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpBeRegs))));
    ((CONTEXT_RESTORE_DMA_E3K*)pRB)->GpcpBeOffset_H     = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(context_buf_phy_addr_for_low.high32 & 0xFF, sizeof(Gpcpbe_regs)>>2);

    pRB += sizeof(CONTEXT_RESTORE_DMA_E3K) >> 2;

    //8.external Fence
    *pRB++ = *(DWORD*)&pRingBufferCommands->c1.Fence;
    *pRB++ = fence_offset.low32;                     //FenceAddr_Low;
    *pRB++ = fence_offset.high32 &0xFF;              //FenceAddr_High
    *pRB++ = fence_id.low32;                         //FenceId_Low;
    *pRB++ = fence_id.high32;                        //FenceId_High;

    *pRB++ = *(DWORD*)&pRingBufferCommands->c1.Fence_1;
    *pRB++ = fence_offset_fake.low32;                //FenceAddr_Low;
    *pRB++ = fence_offset_fake.high32 &0xFF;         //FenceAddr_High
    *pRB++ = fence_id.low32;                         //FenceId_Low;
    *pRB++ = fence_id.high32;                        //FenceId_High;

    *pRB++ = *(DWORD*)&pRingBufferCommands->c1.Fence_2;
    *pRB++ = fence_offset_local.low32;             //FenceAddr_Low;
    *pRB++ = fence_offset_local.high32 &0xFF;              //FenceAddr_High
    *pRB++ = fence_id.low32;                         //FenceId_Low;
    *pRB++ = fence_id.high32;                        //FenceId_High;

    *pRB++ = *(DWORD*)&pRingBufferCommands->c1.Fence_3;
    *pRB++ = fence_offset_local_fake.low32;                //FenceAddr_Low;
    *pRB++ = fence_offset_local_fake.high32 &0xFF;         //FenceAddr_High
    *pRB++ = fence_id.low32;                         //FenceId_Low;
    *pRB++ = fence_id.high32;                        //FenceId_High;


    *pRB++ = *(DWORD*)&pRingBufferCommands->c1.Fence_4;
    *pRB++ = fence_offset_snoop.low32;                     //FenceAddr_Low;
    *pRB++ = fence_offset_snoop.high32 &0xFF;              //FenceAddr_High
    *pRB++ = fence_id.low32;                         //FenceId_Low;
    *pRB++ = fence_id.high32;                        //FenceId_High;

    *pRB++ = *(DWORD*)&pRingBufferCommands->c1.Fence_5;
    *pRB++ = fence_offset_snoop_fake.low32;                //FenceAddr_Low;
    *pRB++ = fence_offset_snoop_fake.high32 &0xFF;         //FenceAddr_High
    *pRB++ = fence_id.low32;                         //FenceId_Low;
    *pRB++ = fence_id.high32;                        //FenceId_High;

    //9.pwmOff;
    *pRB++ = *(DWORD*)&pRingBufferCommands->c1.pwmTrigger;
    *pRB++ = *(DWORD*)&trigger_Dw;

    cmd.cmd_Tbr_Indicator.Indicator_Info = TBR_INDICATOR_INDICATOR_INFO_BEGIN;
    *pRB++ = cmd.uint;

    gf_memcpy(pRB1, pRB0, (dwRbSize<<2));
    //util_dump_memory(pRB1, dwRbSize<<2, "high dma in rb");

    if(pRB0) gf_free(pRB0);

    enginei_do_kickoff_cmd_e3k(engine);

    return S_OK;
}

static void enginei_idle_ring_buffer_e3k(engine_e3k_t *engine)
{
    vidsch_mgr_t  *sch_mgr = engine->vidsch;
    adapter_t     *adapter = sch_mgr->adapter;
    unsigned int  engine_index    = sch_mgr->engine_index;
    EngineSatus_e3k engine_status;
    unsigned int  status_offset  = MMIO_CSP_START_ADDRESS + Reg_Block_Busy_Bits_Top_Offset*4;
    unsigned int  rb_head_offset = MMIO_CSP_START_ADDRESS + (Reg_Ring_Buf_Offset + engine_index * 4 +2) * 4;
    unsigned int  rb_tail_offset = MMIO_CSP_START_ADDRESS + (Reg_Ring_Buf_Offset + engine_index * 4 +3) * 4;
    unsigned int  RBHead = 0, RBTail=0;
    unsigned int  current_rb_dma =0;
    unsigned int  i=0;

    gf_msleep(5000);

    while(1)
    {
        engine_status.Top.uint    = gf_read32(adapter->mmio + status_offset + 0);
        engine_status.Gpc0_0.uint = gf_read32(adapter->mmio + status_offset + 4);
        engine_status.Gpc0_1.uint = gf_read32(adapter->mmio + status_offset + 8);
        engine_status.Gpc1_0.uint = gf_read32(adapter->mmio + status_offset + 12);
        engine_status.Gpc1_1.uint = gf_read32(adapter->mmio + status_offset + 16);
        engine_status.Gpc2_0.uint = gf_read32(adapter->mmio + status_offset + 20);
        engine_status.Gpc2_1.uint = gf_read32(adapter->mmio + status_offset + 24);
        RBHead = gf_read32(adapter->mmio + rb_head_offset);
        RBTail = gf_read32(adapter->mmio + rb_tail_offset);
        current_rb_dma = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS +(Reg_Cur_3d_Rbuf_Cmd_Offset + engine_index)*4);

        gf_info("engine index:%x,rb head(%x):%x,cur rb dma:%x tail(%x):%x,Engine Status: %08x,%08x,%08x,%08x,%08x,%08x,%08x,\n",
            engine_index, rb_head_offset, RBHead, current_rb_dma, rb_tail_offset, RBTail,engine_status.Top.uint,
            engine_status.Gpc0_0.uint, engine_status.Gpc0_1.uint, engine_status.Gpc1_0.uint, engine_status.Gpc1_1.uint, engine_status.Gpc2_0.uint, engine_status.Gpc2_1.uint);

        if(((RBTail != RBHead) || (engine_status.Top.uint !=0)) && (i++ < 10))
        {
            gf_msleep(5000);
        }
        else
        {
            break;
        }
    }
}

#define DBG_HW_RING_BUFFER 0
static int enginei_common_submit_e3k(engine_e3k_t *engine, task_dma_t *task)
{
    engine_share_e3k_t      *share   = engine->share;
    adapter_t               *adapter = engine->vidsch->adapter;
    hw_ctxbuf_t             *hwctx   = task->hw_ctx_info;
    unsigned int            RbIndex         = engine->vidsch->engine_index;
    unsigned int            dwDmaBufferSize = (task->submit_end_offset - task->submit_start_offset) >> 2;
    RINGBUFFER_COMMANDS_E3K *pRingBufferCommands = (RINGBUFFER_COMMANDS_E3K*)share->RingBufferCommands;
    RB_PREDEFINE_DMA        *pPredefine          = (RB_PREDEFINE_DMA*)share->begin_end_vma->virt_addr;

    RINGBUFFER_COMMANDS_E3K *pRbCmds = NULL;
    large_inter             fence_offset;
    large_inter             fence_offset_fake;

    large_inter             fence_offset_local;
    large_inter             fence_offset_local_fake;

    large_inter             fence_offset_snoop;
    large_inter             fence_offset_snoop_fake;

    large_inter             fence_id;
    large_inter             dma_phy_addr;
    large_inter             cacheflush3d_phy_addr;

    unsigned int            dwRbSize;
    RINGBUFFER_COMMANDS_E3K *pRb = NULL;
    unsigned int            RbCase;
    Cmd_Dma                 *p;
    unsigned int            slice_mask = 0;

#if DBG_HW_RING_BUFFER
    enginei_idle_ring_buffer_e3k(engine);
#endif

    fence_offset.quad64 = engine->fence_buffer_phy_addr;
    fence_offset_fake.quad64 = engine->fence_buffer_phy_addr_fake;

    fence_offset_local.quad64 = engine->fence_buffer_phy_addr_local;
    fence_offset_local_fake.quad64 = engine->fence_buffer_phy_addr_local_fake;

    fence_offset_snoop.quad64 = engine->fence_buffer_phy_addr_snoop;
    fence_offset_snoop_fake.quad64 = engine->fence_buffer_phy_addr_snoop_fake;

    fence_id.quad64 = task->desc.fence_id;
    dma_phy_addr.quad64= task->dma_buffer_node->gpu_phy_base_addr + task->submit_start_offset;
    cacheflush3d_phy_addr.quad64 = share->begin_end_buffer_phy_addr + ((BYTE*)(&(pPredefine->FlushDMA3DL)) - (BYTE*)pPredefine);

    dwRbSize = sizeof(RINGBUFFER_COMMANDS_E3K)/sizeof(DWORD);

    dwRbSize = (((dwRbSize) + 15) & ~15);
    pRb = (RINGBUFFER_COMMANDS_E3K *)enginei_get_ring_buffer_space_e3k(engine, dwRbSize);

    gf_memset(pRb, 0, (dwRbSize << 2));

    if( !dwDmaBufferSize )
    {
        RbCase = 2;
    }
    else if(task->need_hwctx_switch && RbIndex == RB_INDEX_GFXL)
    {
        RbCase = 0;
    }
    else
    {
        RbCase = 1;
    }

    gf_memcpy(pRb, &pRingBufferCommands[RbCase], sizeof(RINGBUFFER_COMMANDS_E3K));
    pRbCmds = pRb;
    if(RbCase == 0)
    {
        if (hwctx->is_initialized)
        {
            pRbCmds->c0.RestoreContext_Address      =  hwctx->context_buffer_address >> 11;//_Cmd_Ctx_Address_Dword3, hwctx addr need 512dw align.
        }
        else
        {
            pRbCmds->c0.RestoreContext_Address      = share->context_buf_phy_addr >> 11;
            hwctx->is_initialized   = TRUE;
        }
        pRbCmds->c0.SaveContext_Address             = hwctx->context_buffer_address >> 11;
#if NEW_DUMP
        if(adapter->ctl_flags.hang_dump == 2 && !task->null_rendering)
        {
            pRbCmds->c0.HangDumpContextSaveContext_Address = task->desc.hang_context >> 11;
        }
        else
        {
            p = &pRbCmds->c0.HangDumpContextSaveDMA;
            gf_memset(p, 0, sizeof(Cmd_Dma));
            pRbCmds->c0.HangDumpContextSaveContext_Address = 0;//task->desc.hang_context;
            pRbCmds->c0.HangDumpContextSaveDMA_Address_L = 0;
            pRbCmds->c0.HangDumpContextSaveDMA_Address_H = 0;
        }
#endif
    }

    // All cases
    pRbCmds->c0.Fence_Data_L         = fence_id.low32;                     //FenceId_Low;
    pRbCmds->c0.Fence_Data_H         = fence_id.high32;                    //FenceId_High;
    pRbCmds->c0.Fence_Address_L      = fence_offset.low32;                 //FenceAddr_Low;
    pRbCmds->c0.Fence_Address_H      = fence_offset.high32&0xFF;           //FenceAddr_High

    pRbCmds->c0.Fence_Data_L_1       = fence_id.low32;                     //FenceId_Low;
    pRbCmds->c0.Fence_Data_H_1       = fence_id.high32;                    //FenceId_High;
    pRbCmds->c0.Fence_Address_L_1    = fence_offset_fake.low32;            //FenceAddr_Low;
    pRbCmds->c0.Fence_Address_H_1    = fence_offset_fake.high32&0xFF;      //FenceAddr_High

    pRbCmds->c0.Fence_Data_L_2       = fence_id.low32;                     //FenceId_Low;
    pRbCmds->c0.Fence_Data_H_2       = fence_id.high32;                    //FenceId_High;
    pRbCmds->c0.Fence_Address_L_2    = fence_offset_local.low32;            //FenceAddr_Low;
    pRbCmds->c0.Fence_Address_H_2    = fence_offset_local.high32&0xFF;      //FenceAddr_High

    pRbCmds->c0.Fence_Data_L_3       = fence_id.low32;                     //FenceId_Low;
    pRbCmds->c0.Fence_Data_H_3       = fence_id.high32;                    //FenceId_High;
    pRbCmds->c0.Fence_Address_L_3    = fence_offset_local_fake.low32;            //FenceAddr_Low;
    pRbCmds->c0.Fence_Address_H_3    = fence_offset_local_fake.high32&0xFF;      //FenceAddr_High

    pRbCmds->c0.Fence_Data_L_4       = fence_id.low32;                     //FenceId_Low;
    pRbCmds->c0.Fence_Data_H_4       = fence_id.high32;                    //FenceId_High;
    pRbCmds->c0.Fence_Address_L_4    = fence_offset_snoop.low32;            //FenceAddr_Low;
    pRbCmds->c0.Fence_Address_H_4    = fence_offset_snoop.high32&0xFF;      //FenceAddr_High

    pRbCmds->c0.Fence_Data_L_5       = fence_id.low32;                     //FenceId_Low;
    pRbCmds->c0.Fence_Data_H_5       = fence_id.high32;                    //FenceId_High;
    pRbCmds->c0.Fence_Address_L_5    = fence_offset_snoop_fake.low32;            //FenceAddr_Low;
    pRbCmds->c0.Fence_Address_H_5    = fence_offset_snoop_fake.high32&0xFF;      //FenceAddr_High

    // Case 0/1
    if (RbCase != 2)
    {
        pRbCmds->c0.CommandDMA.Dw_Num = dwDmaBufferSize;
        pRbCmds->c0.CommandDMA_Address_L = dma_phy_addr.low32;
        pRbCmds->c0.CommandDMA_Address_H = dma_phy_addr.high32 & 0xFF;
    }

    //when move RT to PCIE and open backdoor , 2 slice's performance is better than 1 slice
#if 0
    if (adapter->vcp_index_cnt[VCP0_INDEX] || adapter->vcp_index_cnt[VCP1_INDEX]) {
        slice_mask = adapter->min_slice_mask;
    } else {
        slice_mask = adapter->hw_caps.chip_slice_mask;
    }
#endif

    //force 2 slice for occlusion query counter limitation
#if 1
    if (adapter->chip_id >= CHIP_ARISE2030)
    {
        slice_mask = task->desc.Flags.ForceSlice ? 0x3 : adapter->hw_caps.chip_slice_mask;

        if (adapter->current_slice_mask != slice_mask) {
            adapter->current_slice_mask = slice_mask;
            if (RbCase == 0) {
                gf_memcpy(&pRbCmds->c0.SliceSwitchInitCommands, &share->SliceSwitchInitCommands[0].c0, sizeof(pRbCmds->c0.SliceSwitchInitCommands));
            } else if (RbCase == 1) {
                gf_memcpy(&pRbCmds->c1.SliceSwitchInitCommands, &share->SliceSwitchInitCommands[1].c1, sizeof(pRbCmds->c1.SliceSwitchInitCommands));
            }
            //gf_info("switch slice mask to 0x%x\n", adapter->current_slice_mask);
        }
    }
#endif
    pRbCmds->c1.trigger_Dw.Slice_Mask = adapter->current_slice_mask;

    if (!dwDmaBufferSize ||
        (task->dma_type == PAGING_DMA))
    {
        pRbCmds->c0.pwmTrigger.Info      = BLK_CMD_CSP_INDICATOR_INFO_3D_MODE;
    }

    if(RbIndex == RB_INDEX_GFXL)
    {
        pRbCmds->c0.CacheFlushDMA_Address_L = cacheflush3d_phy_addr.low32;
        pRbCmds->c0.CacheFlushDMA_Address_H = cacheflush3d_phy_addr.high32 & 0xFF;
    }

#if DBG_HW_RING_BUFFER
    util_dump_memory(NULL, task->dma_buffer_node->virt_base_addr,(task->submit_end_offset - task->submit_start_offset),"dma cmd ");
    gf_info("rb case:%d,dma type:%x,rb get size:%x\n",RbCase,task->dma_type,dwRbSize<<2);
    util_dump_memory(NULL, pRb,dwRbSize<<2,"rb cmd");
#else
    //when compress on, haps alway meet random hang, should be rb cmd not really write in mem due to hw busy.
    //add a crc check here.
    //perhaps dump value is same but crc is diff, since read buf will trigger mem write back in mem.
    if(adapter->ctl_flags.run_on_qt && util_crc32((unsigned char*)pRb, sizeof(RINGBUFFER_COMMANDS_E3K)) != util_crc32((unsigned char*)pRbCmds, sizeof(RINGBUFFER_COMMANDS_E3K)))
    {
        gf_info("rb case:%d,dma type:%x,rb get size:%x, fence:%llx\n",RbCase,task->dma_type,dwRbSize<<2, fence_id.quad64);
        util_dump_memory(NULL, pRb, dwRbSize<<2, "rb cmd in mem");
        util_dump_memory(NULL, pRbCmds, sizeof(RINGBUFFER_COMMANDS_E3K), "template rb cmd");
    }
#endif
    if(debug_mode_e3k.pre_hang_dump || debug_mode_e3k.post_hang_dump)
    {
        unsigned int offset = 0;
        engine->last_ring_buffer_size      = dwRbSize << 2;
        engine->last_ring_buffer           = (unsigned int *)pRb;
#if 0
        if(dwDmaBufferSize)
        {
             offset = (unsigned int *)&pRb->c0.CommandDMA_Address_L - (unsigned int *)pRb;
             engine->last_ring_buffer_for_dma =(unsigned char *)((unsigned int*)pRb + offset);
        }
        else
        {
            engine->last_ring_buffer_for_dma = NULL;
        }
#endif
        offset = (unsigned int *)&pRb->c0.Fence - (unsigned int *)pRb;
        engine->last_ring_buffer_for_fence = (unsigned char *)((unsigned int*)pRb + offset);
#if 0
        if(pRb->c0.RestoreDMA.Major_Opcode == CSP_OPCODE_Dma)
        {
            offset = (unsigned int *)&pRb->c0.RestoreDMA - (unsigned int *)pRb;
            engine->last_ring_buffer_for_begin_dma = (unsigned char *)((unsigned int *)pRb + offset);

            offset = (unsigned int *)&pRb->c0.SaveDMA - (unsigned int *)pRb;
            engine->last_ring_buffer_for_end_dma   = (unsigned char *)((unsigned int *)pRb + offset);
        }
        else
        {
            engine->last_ring_buffer_for_begin_dma = NULL;
            engine->last_ring_buffer_for_end_dma   = NULL;
        }
#endif
      //  gf_info("enginei_common_submit_e3k fence_id:%lld", fence_id.quad64);
    }

    enginei_do_kickoff_cmd_e3k(engine);

    return S_OK;
}

static int enginei_write_commands_to_hw_e3k(engine_e3k_t *engine, task_dma_t *task)
{
    switch (engine->vidsch->engine_index) {
    case RB_INDEX_GFXH:
            enginei_submit_to_gfx_high_e3k(engine, task);
            break;
    default:
            enginei_common_submit_e3k(engine, task);
    }
    return S_OK;
}

static int enginei_write_commands_to_hw_video_e3k(engine_e3k_t *engine, task_dma_t *task)
{
    adapter_t          *adapter = engine->vidsch->adapter;
    unsigned long long FenceOffset = engine->fence_buffer_phy_addr;
    unsigned long long FenceOffsetfake = engine->fence_buffer_phy_addr_fake;
    unsigned long long FenceId = task->desc.fence_id;
    unsigned int dwDmasize = (task->submit_end_offset - task->submit_start_offset)>>2;
    unsigned int SubmitSize = 0, dwCmdSize = 0;
    unsigned int RbIndex = engine->vidsch->engine_index;
    unsigned int *pRb = NULL;
    Cmd_Vcp_Fence Cmd_Vcp_Fence={ 0 };

    if(adapter->ctl_flags.hwq_event_enable && (RbIndex == RB_INDEX_VCP0 || RbIndex == RB_INDEX_VCP1)) //vcp
    {
       static int i = -1;
       i++;
       if(i >= VCP_INFO_COUNT) i = 0;
       gf_get_nsecs(&adapter->vcp_task_info[i].vcp_inc_timestamp);
       adapter->vcp_task_info[i].vcp_fence_id = FenceId;
       adapter->vcp_task_info[i].vcp_pid = task->desc.context->pid;
    }

    dwCmdSize = dwDmasize + EXTERNAL_FENCE_DWSIZE*2; // 4 is for external fence;
    dwCmdSize = ((dwCmdSize +15)&~15);

    pRb = enginei_get_ring_buffer_space_e3k(engine, dwCmdSize);
    gf_memset(pRb, 0, (dwCmdSize << 2));
    gf_memcpy(pRb, (void *)task->dma_buffer_node->virt_base_addr,  (dwDmasize << 2));

    pRb += dwDmasize;

    if ((RbIndex == RB_INDEX_VCP0) || (RbIndex == RB_INDEX_VCP1))
    {
        Cmd_Vcp_Fence.Major_Opcode         = VCP_OPCODE_Vcp_Fence; // 0x8
        Cmd_Vcp_Fence.Fence_Type           = VCP_FENCE_FENCE_TYPE_EXTERNAL64;
        Cmd_Vcp_Fence.Fence_Update_Mode    = FENCE_FENCE_UPDATE_MODE_COPY;
        Cmd_Vcp_Fence.Dwc                  = 4;
        Cmd_Vcp_Fence.Irq                  = 1;
#if 0
        //On E3K chip, encoding use BE only, decoding use decouple mode, so always use BE to send external fence.
        if(EnableVcpDecouple)
        {
            Cmd_Vcp_Fence.Block_Id          = VCP_QUERY_DUMP_BLOCK_ID_VCP_BE;
            Cmd_Vcp_Fence.Major_Opcode      = VCP_OPCODE_Vcp_Fence; //0x8
        }
        else
        {
            //To do: for encoding, still needs BE to send external fence, but we need a flag to know it's encoding.
            Cmd_Vcp_Fence.Block_Id          = VCP_QUERY_DUMP_BLOCK_ID_VCP_FE;
            Cmd_Vcp_Fence.Major_Opcode      = VCP_OPCODE_Vcp_Fence; //0x8
        }
#endif

        Cmd_Vcp_Fence.Block_Id = VCP_QUERY_DUMP_BLOCK_ID_VCP_BE;
        *pRb++ = *(DWORD*)&Cmd_Vcp_Fence;
        *pRb++ = (FenceOffset & 0xFFFFFFFF);
        *pRb++ = ((FenceOffset >> 32) &0xFFFFFFFF);
        *pRb++ = (FenceId & 0xFFFFFFFF);
        *pRb++ = ((FenceId >> 32) &0xFFFFFFFF);

        Cmd_Vcp_Fence.Block_Id = VCP_QUERY_DUMP_BLOCK_ID_VCP_FE;
        *pRb++ = *(DWORD*)&Cmd_Vcp_Fence;
        *pRb++ = (FenceOffsetfake & 0xFFFFFFFF);
        *pRb++ = ((FenceOffsetfake >> 32) &0xFFFFFFFF);
        *pRb++ = (FenceId & 0xFFFFFFFF);
        *pRb++ = ((FenceId >> 32) &0xFFFFFFFF);
    }
    else if (RbIndex == RB_INDEX_VPP)
    {
        // VPP using the same fence struct as VCP.
        Cmd_Vcp_Fence.Major_Opcode      = CSP_OPCODE_Fence;
        Cmd_Vcp_Fence.Fence_Type        = FENCE_FENCE_TYPE_EXTERNAL64;
        Cmd_Vcp_Fence.Block_Id          = FENCE_ROUTE_ID_VPP_FENCE;
        Cmd_Vcp_Fence.Fence_Update_Mode = FENCE_FENCE_UPDATE_MODE_COPY;
        Cmd_Vcp_Fence.Dwc               = 4;
        Cmd_Vcp_Fence.Irq               = 1;

        *pRb++ = *(DWORD*)&Cmd_Vcp_Fence;
        *pRb++ = (FenceOffset & 0xFFFFFFFF);
        *pRb++ = ((FenceOffset >> 32) &0xFFFFFFFF);
        *pRb++ = (FenceId & 0xFFFFFFFF);
        *pRb++ = ((FenceId >> 32) &0xFFFFFFFF);

        *pRb++ = *(DWORD*)&Cmd_Vcp_Fence;
        *pRb++ = (FenceOffsetfake & 0xFFFFFFFF);
        *pRb++ = ((FenceOffsetfake >> 32) &0xFFFFFFFF);
        *pRb++ = (FenceId & 0xFFFFFFFF);
        *pRb++ = ((FenceId >> 32) &0xFFFFFFFF);
    }
    else
    {
        gf_error("%s() should not enter here, engine_index-%d\n", util_remove_name_suffix(__func__), RbIndex);
    }

    SubmitSize = ALIGNED_8(dwCmdSize);
    if (dwCmdSize < SubmitSize)
    {
        DWORD dwSkipSize = SubmitSize - dwCmdSize - 1;
        Cmd_Vcp_Skip cmd_Vcp_Skip = { 0 };

        if (dwSkipSize)
        {
            cmd_Vcp_Skip.Dwc          = dwSkipSize;
            cmd_Vcp_Skip.Major_Opcode = VCP_OPCODE_Vcp_Skip;
        }

        *pRb++ = *(DWORD*)&cmd_Vcp_Skip;
    }

    enginei_do_kickoff_cmd_e3k(engine);

    return S_OK;
}

int engine_submit_task_dma_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, task_dma_t *task_dma)
{
    unsigned long long timestamp = 0;
    gf_perf_event_t perf_event = {0};
    engine_e3k_t *engine = sch_mgr->private_data;

    if(debug_mode_e3k.internal_dump_hw)
    {
        gf_info("engine_submit_task_dma_e3k start dump image, need skip this function...\n");
        return 0;
    }

    if(sch_mgr->init_submit)
    {
#if GF_TRACE_HW_HANG
        debug_mode_e3k.print_debug_bus = 1;
#endif

        if(adapter->ctl_flags.hang_dump == 0)
        {
            debug_mode_e3k.pre_hang_dump   = 0;
            debug_mode_e3k.post_hang_dump  = 0;
            debug_mode_e3k.duplicate_hang  = 0;
        }
        else if(adapter->ctl_flags.hang_dump == 1)
        {
            debug_mode_e3k.pre_hang_dump   = 1;
            debug_mode_e3k.post_hang_dump  = 0;
            debug_mode_e3k.duplicate_hang  = 0;
        }
        else if(adapter->ctl_flags.hang_dump == 2)
        {
            debug_mode_e3k.pre_hang_dump   = 0;
            debug_mode_e3k.post_hang_dump  = 1;
            debug_mode_e3k.duplicate_hang  = 0;
        }
        if(adapter->ctl_flags.hang_dump == 3)
        {
            debug_mode_e3k.pre_hang_dump   = 0;
            debug_mode_e3k.post_hang_dump  = 0;
            debug_mode_e3k.duplicate_hang  = 1;
        }

        sch_mgr->init_submit = 0;
    }

    if(!adapter->hw_caps.local_only)
    {
        enginei_invalidate_gart_e3k(engine);
    }

    if(adapter->ctl_flags.hang_dump && !task_dma->null_rendering && sch_mgr->engine_index == RB_INDEX_GFXL)
    {
         engine->last_dma_buffer_size_uint  = (task_dma->submit_end_offset - task_dma->submit_start_offset) >> 2;
         engine->last_dma_buffer            = (unsigned char*)(task_dma->dma_buffer_node->virt_base_addr + task_dma->submit_start_offset);

         //util_dump_memory(sch_mgr->last_dma_buffer, sch_mgr->last_dma_buffer_size_uint, "LAST DMA Buffer");
         if(task_dma->dma_type != PAGING_DMA)
         {
            vidsch_handle_dbg_hang_dump_e3k( adapter, sch_mgr, task_dma);
         }
    }

    if(adapter->ctl_flags.hwq_event_enable)
    {
        if(timestamp==0)
        {
            gf_get_nsecs(&timestamp);
        }
        hwq_process_submit_event(adapter,timestamp,task_dma->desc.fence_id & 0xffffffff,sch_mgr->engine_index);
    }

    switch (sch_mgr->engine_index) {
    case RB_INDEX_GFXL:
    case RB_INDEX_GFXH:
        enginei_write_commands_to_hw_e3k(engine, task_dma);
        break;
    case RB_INDEX_VCP0:
    case RB_INDEX_VCP1:
    case RB_INDEX_VPP:
        enginei_write_commands_to_hw_video_e3k(engine, task_dma);
        break;
    default:
        gf_error("submit unsupport engine type\n");
        gf_assert(0, "submit unsupport engine type");
    }

    if(adapter->ctl_flags.perf_event_enable)
    {
        gf_get_nsecs(&timestamp);
        perf_event.header.timestamp_high = timestamp >> 32;
        perf_event.header.timestamp_low = (timestamp) & 0xffffffff;
        perf_event.header.size = sizeof(gf_perf_event_dma_buffer_submitted_t);
        perf_event.header.type = GF_PERF_EVENT_DMA_BUFFER_SUBMITTED;
        perf_event.dma_buffer_submitted.dma_type = task_dma->dma_type;
        perf_event.dma_buffer_submitted.gpu_context = (task_dma->desc.context ? task_dma->desc.context->handle : 0);
        perf_event.dma_buffer_submitted.dma_idx_low = task_dma->render_counter & 0xffffffff;
        perf_event.dma_buffer_submitted.dma_idx_high = (task_dma->render_counter >> 32) & 0xffffffff;
        perf_event.dma_buffer_submitted.engine_idx = sch_mgr->engine_index;
        perf_event.dma_buffer_submitted.fence_id_low = task_dma->desc.fence_id & 0xffffffff;
        perf_event.dma_buffer_submitted.fence_id_high = (task_dma->desc.fence_id >> 32) & 0xffffffff;

        perf_event_add_event(adapter, &perf_event);
    }

    if(adapter->ctl_flags.hang_dump && (debug_mode_e3k.pre_hang_dump || debug_mode_e3k.post_hang_dump) && sch_mgr->engine_index == RB_INDEX_GFXL)
    {
        if(task_dma->dma_type != PAGING_DMA)
        {
            vidsch_save_misc_e3k(adapter, sch_mgr, SAVE_RING_BUFFER);
        }
    }

#ifdef VMI_MODE
    vidschi_notify_fence_interrupt(adapter, ~0x0);
#endif

    return S_OK;
}


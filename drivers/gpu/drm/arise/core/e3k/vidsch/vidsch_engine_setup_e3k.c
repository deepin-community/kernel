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
#include "chip_include_e3k.h"
#include "vidsch_dfs_e3k.h"
#include "bit_op.h"
#include "kernel_interface.h"
#include "mm_e3k.h"
#include "vidsch_debug_hang_e3k.h"

static int enginei_common_share_init_e3k(engine_e3k_t *engine)
{
    engine_share_e3k_t *share = engine->share;

    vidschi_init_blt_cmd_e3k(share);

    return S_OK;
}

static int enginei_common_init_e3k(vidsch_mgr_t *sch_mgr, engine_e3k_t *engine,
                           unsigned int *current_pcie_offset, unsigned int *current_low_offset)
{
    engine_share_e3k_t      *share                = sch_mgr->adapter->private_data;
    vidmm_segment_memory_t  *pcie_reserved_memory = sch_mgr->pcie_reserved_memory;
    int                status = S_OK;

    engine->vidsch = sch_mgr;

    /* allocate fence buffer */
    status = vidsch_request_fence_space(sch_mgr->adapter, sch_mgr->adapter->fence_buf, &engine->fence_buffer_virt_addr, &engine->fence_buffer_phy_addr);
    //gf_info("%s:engine:%d,fence buf phy addr:%x\n",util_remove_name_suffix(__func__), sch_mgr->engine_index,  engine->fence_buffer_phy_addr);//for debug
    status = vidsch_request_fence_space(sch_mgr->adapter, sch_mgr->adapter->fence_buf, &engine->fence_buffer_virt_addr_fake, &engine->fence_buffer_phy_addr_fake);
    //gf_info("%s:engine:%d,fence buf phy addr fake:%x\n",util_remove_name_suffix(__func__), sch_mgr->engine_index,  engine->fence_buffer_phy_addr_fake);//for debug

    status = vidsch_request_fence_space(sch_mgr->adapter, sch_mgr->adapter->fence_buf_local, &engine->fence_buffer_virt_addr_local, &engine->fence_buffer_phy_addr_local);
    status = vidsch_request_fence_space(sch_mgr->adapter, sch_mgr->adapter->fence_buf_local, &engine->fence_buffer_virt_addr_local_fake, &engine->fence_buffer_phy_addr_local_fake);

    status = vidsch_request_fence_space(sch_mgr->adapter, sch_mgr->adapter->fence_buf_snoop, &engine->fence_buffer_virt_addr_snoop, &engine->fence_buffer_phy_addr_snoop);
    status = vidsch_request_fence_space(sch_mgr->adapter, sch_mgr->adapter->fence_buf_snoop, &engine->fence_buffer_virt_addr_snoop_fake, &engine->fence_buffer_phy_addr_snoop_fake);

    gf_assert(status == S_OK, "enginei_common_init");

    *engine->fence_buffer_virt_addr      = 0x0ll;
    *engine->fence_buffer_virt_addr_fake = 0x0ll;

    *engine->fence_buffer_virt_addr_local      = 0x0ll;
    *engine->fence_buffer_virt_addr_local_fake = 0x0ll;

    *engine->fence_buffer_virt_addr_snoop      = 0x0ll;
    *engine->fence_buffer_virt_addr_snoop_fake = 0x0ll;

    /* allocate ring buffer from pcie reserved */
    engine->ring_buf_virt_addr  = pcie_reserved_memory->vma->virt_addr;
    engine->ring_buf_virt_addr += *current_pcie_offset;

    share->ring_buffer_phy_addr[sch_mgr->engine_index] =
    engine->ring_buf_phys_addr = pcie_reserved_memory->gpu_virt_addr + *current_pcie_offset;

    //gf_info("%s:engine:%d, ring_buffer_phy_addr:%x, need 4k aligned!!!!\n",util_remove_name_suffix(__func__), sch_mgr->engine_index,  engine->ring_buf_phys_addr);//for debug

    *current_pcie_offset += RING_BUFFER_SIZE_E3K;

    enginei_enable_ring_buffer_e3k(engine);

    sch_mgr->slot_count    = vidschi_get_slot_count_e3k(sch_mgr->adapter);
    sch_mgr->private_data  = engine;

    share->ref_cnt++;
    engine->share = share;

    return S_OK;
}

int engine_save_e3k(vidsch_mgr_t *vidsch)
{
    adapter_t *adapter = vidsch->adapter;
    engine_e3k_t *engine = vidsch->private_data;
    engine_share_e3k_t      *share                 = engine->share;
    vidsch_mgr_t *sch_mgr = adapter->sch_mgr[vidsch->engine_index];
    int result = 0;

    if (adapter->suspend_blt_mode)
    {
        if(share->bl_buffer && share->bl_buffer->list_node->size && !share->bl_buffer_backup)
        {
            struct _vidmm_segment_memory *segment = NULL;
            result = vidmm_segment_memory_transfer_e3k(adapter, &segment, share->bl_buffer, 0);
            share->bl_buffer_backup = segment;
        }
    }
    else
    {
        if(share->bl_buffer && !share->bl_buffer->vma &&!share->bl_buffer_backup )
        {
            vidmm_map_flags_t flags = {0};
            flags.mem_space = GF_MEM_KERNEL;
            flags.cache_type = GF_MEM_UNCACHED;

            vidmm_map_segment_memory(adapter, NULL, share->bl_buffer, &flags);

            share->bl_buffer_backup = gf_calloc(share->bl_buffer->list_node->size);
            if (share->bl_buffer_backup && share->bl_buffer->vma)
            {
                //gf_info("engine_save_e3k: start mem copy bl_buffer size %d (engine %d)\n",share->bl_buffer->list_node->size, vidsch->engine_index);
                gf_memcpy(share->bl_buffer_backup, share->bl_buffer->vma->virt_addr, share->bl_buffer->list_node->size);
                //gf_info("engine_save_e3k: end save memcpy engine (engine %d)\n",vidsch->engine_index);
            }
            else
            {
                result = E_OUTOFMEMORY;
            }
            //util_dump_memory(share->bl_buffer->vma->virt_addr, 4*1024, "first 4k bl");
        }
    }

    if(share->begin_end_vma)
    {
        gf_unmap_io_memory(share->begin_end_vma);
        share->begin_end_vma = NULL;
    }

    gf_info("%s: engine index: %d, last send: %lld, last_return: %lld\n", util_remove_name_suffix(__func__),
        vidsch->engine_index, sch_mgr->last_send_fence_id, sch_mgr->returned_fence_id);

    return result;
}

void engine_restore_e3k(vidsch_mgr_t *vidsch, unsigned int pm)
{
    engine_e3k_t *engine = vidsch->private_data;

    enginei_enable_ring_buffer_e3k(engine);
}

int engine_destroy_e3k(vidsch_mgr_t *vidsch)
{
    adapter_t *adapter = vidsch->adapter;
    engine_e3k_t *engine = vidsch->private_data;

    vidsch->private_data = NULL;

    if(adapter->private_data)
    {
        engine_share_e3k_t *share = adapter->private_data;

        if(share->flush_fifo_buffer)
        {
            vidmm_unmap_segment_memory(adapter, share->flush_fifo_buffer, GF_MEM_KERNEL);

            vidmm_release_segment_memory(adapter, share->flush_fifo_buffer);

            share->flush_fifo_buffer = NULL;
        }

        if(share->bl_buffer)
        {
            heap_t *heap = NULL;

            heap = vidmm_get_burst_length_heap(adapter);

            vidmm_release_segment_memory(adapter, share->bl_buffer);

            share->bl_buffer = NULL;

            heap_destroy(heap);

            heap = NULL;
        }

        if(share->ring_buffer_for_hang)
        {
            vidmm_release_segment_memory(adapter, share->ring_buffer_for_hang);

            share->ring_buffer_for_hang = NULL;
        }

        if(share->dma_buffer_for_hang)
        {
            vidmm_release_segment_memory(adapter, share->dma_buffer_for_hang);

            share->dma_buffer_for_hang = NULL;
        }

        if(share->context_buffer_for_hang)
        {
            vidmm_release_segment_memory(adapter, share->context_buffer_for_hang);

            share->context_buffer_for_hang = NULL;
        }
        if(share->transfer_buffer_for_hang)
        {
            vidmm_release_segment_memory(adapter, share->transfer_buffer_for_hang);

            share->transfer_buffer_for_hang = NULL;
        }

        if(share->mirror_buffer_for_hang)
        {
            vidmm_release_segment_memory(adapter, share->mirror_buffer_for_hang);

            share->mirror_buffer_for_hang = NULL;
        }

        if(share->begin_end_vma)
        {
            gf_unmap_io_memory(share->begin_end_vma);
            share->begin_end_vma = NULL;
        }
    }

    if(engine->share != NULL)
    {
        engine_share_e3k_t *share = engine->share;

        share->ref_cnt--;

        if(share->ref_cnt == 0)
        {
            gf_destroy_spinlock(share->lock);

            share->lock = NULL;

            gf_destroy_mutex(share->_3dblt_data_lock);

            share->_3dblt_data_lock = NULL;

            gf_free(share);

            engine->share = NULL;
            vidsch->adapter->private_data = NULL;
        }
    }

    gf_free(engine);

    return S_OK;
}

unsigned long long engine_update_fence_id_e3k(vidsch_mgr_t *sch_mgr)
{
    unsigned long long fence_id = 0, fence_id_fake = 0, fence_id_max = 0;
    engine_e3k_t *engine = sch_mgr->private_data;

    fence_id = *(volatile unsigned long long *)engine->fence_buffer_virt_addr;
    fence_id_fake = *(volatile unsigned long long *)engine->fence_buffer_virt_addr_fake;

    fence_id_max = fence_id > fence_id_fake ? fence_id:fence_id_fake;

    fence_id = *(volatile unsigned long long *)engine->fence_buffer_virt_addr_local;
    fence_id_fake = *(volatile unsigned long long *)engine->fence_buffer_virt_addr_local_fake;

    if (fence_id_max < fence_id || fence_id_max < fence_id_fake)
        fence_id_max = fence_id > fence_id_fake ? fence_id:fence_id_fake;

    fence_id = *(volatile unsigned long long *)engine->fence_buffer_virt_addr_snoop;
    fence_id_fake = *(volatile unsigned long long *)engine->fence_buffer_virt_addr_snoop_fake;

    if (fence_id_max < fence_id || fence_id_max < fence_id_fake)
        fence_id_max = fence_id > fence_id_fake ? fence_id:fence_id_fake;

    return fence_id_max;
}

unsigned long long engine_update_vpp_fence_id_e3k(vidsch_mgr_t *sch_mgr)
{
    unsigned long long fence_id = 0;
    engine_e3k_t *engine = sch_mgr->private_data;
    fence_id = *(volatile unsigned long long *)engine->fence_buffer_virt_addr;

    return fence_id;
}

unsigned long long engine_update_vcp_fence_id_e3k(vidsch_mgr_t *sch_mgr)
{
    unsigned long long fence_fe, fence_be;
    unsigned int i = 0,j = 0;
    engine_e3k_t *engine = sch_mgr->private_data;
    fence_be = *(volatile unsigned long long *)engine->fence_buffer_virt_addr;
    fence_fe = *(volatile unsigned long long *)engine->fence_buffer_virt_addr_fake;

    if(sch_mgr->adapter->ctl_flags.hwq_event_enable && sch_mgr->last_returned_fence_id != sch_mgr->returned_fence_id)
    {
        for(i = 0; i < VCP_INFO_COUNT; i++)
        {
            if(sch_mgr->returned_fence_id == sch_mgr->adapter->vcp_task_info[i].vcp_fence_id)
            {
                for(j = 0; j < VCP_INFO_COUNT; j++)
                {
                    if(sch_mgr->adapter->vcp_task_info[i].vcp_pid == sch_mgr->adapter->vcp_info[j].pid)
                    {
                        unsigned long long   update_timestamp;         /* THE time update fence id */
                        unsigned long long   time_interval;
                        gf_get_nsecs(&update_timestamp);
                        time_interval = gf_do_div(update_timestamp - sch_mgr->adapter->vcp_task_info[i].vcp_inc_timestamp, 1000000);
                        sch_mgr->adapter->vcp_info[j].TotalDecodetime += time_interval;
                        sch_mgr->adapter->vcp_info[j].TotalDecodeFrameNum++;
                        sch_mgr->last_returned_fence_id = sch_mgr->returned_fence_id;
                        break;
                    }
                }
                break;
            }
        }
    }

    return fence_fe > fence_be ? fence_be: fence_fe;
}

static int enginei_restore_3dblt_data_e3k(engine_gfx_e3k_t *engine)
{
    adapter_t        *adapter      = engine->common.vidsch->adapter;
    engine_share_e3k_t *share = engine->common.share;
    gf_map_argu_t    map_argu     = {0};
    gf_vm_area_t    *vma          = NULL;

    if (!share->_3dblt_data_shadow)
        return S_OK;

    map_argu.flags.cache_type = GF_MEM_WRITE_COMBINED;
    map_argu.flags.mem_space  = GF_MEM_KERNEL;
    map_argu.flags.mem_type   = GF_SYSTEM_IO;
    map_argu.phys_addr        = adapter->vidmm_bus_addr + share->_3dblt_data_phy_addr;
    map_argu.size             = share->_3dblt_data_size;

    vma = gf_map_io_memory(NULL, &map_argu);

    gf_memcpy(vma->virt_addr, share->_3dblt_data_shadow, share->_3dblt_data_size);

    gf_unmap_io_memory(vma);

    return S_OK;
}

int enginei_init_hardware_context_e3k(engine_gfx_e3k_t *engine)
{
    CONTEXT_BUFFER_E3K   *pContext                      = NULL;
    engine_e3k_t         *engine_common                 = &engine->common;
    unsigned int*        pRB;
    unsigned int         RbSize;
    BOOL b3dSigZero = FALSE;

    Reg_Eu_Full_Glb                     reg_Eu_Full_Glb             = {0};
    Reg_Tu_Tssharp_Ctrl                 reg_Tu_Tssharp_Ctrl         = {0};
    Reg_Ffc_Ubuf_Golbalconfig           reg_Ffc_Ubuf_Golbalconfig   = {0};
    Reg_Fence_Mask                      reg_Fence_Mask              = {0};
    Reg_L2_Performance_Redundancy       reg_L2_Performance          = {0};
    Cmd_Blk_Cmd_Csp_Indicator           pwmTrigger                  = {0};
    Cmd_Blk_Cmd_Csp_Indicator_Dword1    trigger_Dw                  = {0};
    Cmd_Blk_Cmd_Csp_Indicator           pwmTriggerOff               = {0};
    Cmd_Blk_Cmd_Csp_Indicator_Dword1    triggerOff_Dw               = {0};

    engine_share_e3k_t   *share   = engine_common->share;
    adapter_t            *adapter = engine_common->vidsch->adapter;
    RB_PREDEFINE_DMA *pcmBuf = share->begin_end_vma->virt_addr;
    CONTEXT_RESTORE_DMA_E3K *pRestoreDMA  = NULL;

    gf_map_argu_t       map_argu = {0};
    gf_vm_area_t        *vma     = NULL;

    unsigned int  slice_mask = adapter->hw_caps.chip_slice_mask;
    unsigned int  gpc_bit_mask = 0, shift_num = 1;

    while (slice_mask)
    {
        if (slice_mask & 0xf)
        {
            gpc_bit_mask |= shift_num;
        }
        slice_mask >>= 4;
        shift_num <<= 1;
    }

    if(!adapter->hw_caps.local_only)
    {
        enginei_invalidate_gart_e3k(engine_common);
    }

    pwmTrigger.Major_Opcode         = CSP_OPCODE_Blk_Cmd_Csp_Indicator;
    pwmTrigger.Block_Id             = CSP_GLOBAL_BLOCK;
    pwmTrigger.Type                 = BLOCK_COMMAND_CSP_TYPE_INDICATOR;
    pwmTrigger.Info                 = BLK_CMD_CSP_INDICATOR_INFO_3D_MODE;
    pwmTrigger.Dwc                  =  1;
    trigger_Dw.Slice_Mask           = adapter->hw_caps.chip_slice_mask;

    pwmTriggerOff.Major_Opcode      = CSP_OPCODE_Blk_Cmd_Csp_Indicator;
    pwmTriggerOff.Block_Id          = CSP_GLOBAL_BLOCK;
    pwmTriggerOff.Type              = BLOCK_COMMAND_CSP_TYPE_INDICATOR;
    pwmTriggerOff.Info              = BLK_CMD_CSP_INDICATOR_INFO_OFF_MODE;
    pwmTriggerOff.Dwc               = 1;

    reg_Eu_Full_Glb.uint                        = 0;
    reg_Eu_Full_Glb.reg.Uav_En                  = 1;
    reg_Eu_Full_Glb.reg.Uav_Continue_Chk        = 1;
    reg_Eu_Full_Glb.reg.Uav_Buf_Size            = EU_FULL_GLB_UAV_BUF_SIZE_128LINES;
    reg_Eu_Full_Glb.reg.U_3d_Base               = 6;//U_SHARP_3D_SLOT_START / 8;
    reg_Eu_Full_Glb.reg.Cb_3d_Base              = 8;//CB_3D_SLOT_START >> 4;

    reg_Tu_Tssharp_Ctrl.uint                    = 0;

    reg_Ffc_Ubuf_Golbalconfig.uint              = 0;
    reg_Ffc_Ubuf_Golbalconfig.reg.U_3d_Base     = 6;//U_SHARP_3D_SLOT_START / 8;
    reg_Fence_Mask.reg.Mask                     = 0xFFFFFFFF;//init this reg as 0xffffffff per Wenni

    map_argu.flags.cache_type = GF_MEM_WRITE_COMBINED;
    map_argu.flags.mem_space  = GF_MEM_KERNEL;
    map_argu.flags.mem_type   = GF_SYSTEM_IO;
    map_argu.phys_addr = adapter->vidmm_bus_addr + engine->context_buf_phy_addr;
    map_argu.size             = HW_CONTEXT_SIZE_E3K;

    vma = gf_map_io_memory(NULL, &map_argu);
    gf_memset(vma->virt_addr, 0, vma->size);
    pContext = vma->virt_addr;
    pContext->CSPRegs.reg_Csp_Misc_Control.reg.Dump_3d_Signature_Zero = b3dSigZero;
    pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.ff_regs.reg_Ff_Glb_Ctrl.reg.Ffc_Config_Mode = FF_GLB_CTRL_FFC_CONFIG_MODE_ZS_D_U;
    pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.eufs_regs.reg_Eu_3d_Glb.reg.Cb_Ps_Base = reg_Eu_Full_Glb.reg.Cb_3d_Base; // umd will set it again, here is just for HW init check.
    pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.eufs_regs.reg_Eu_3d_Glb.reg.U_Ps_Base = reg_Eu_Full_Glb.reg.U_3d_Base; // umd will set it again, here is just for HW init check.
    pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.wls_regs.reg_Ffc_Ubuf_3dconfig.reg.U_Ps_Base = reg_Eu_Full_Glb.reg.U_3d_Base; // umd will set it again, here is just for HW init check.
    pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.tu_regs.reg_Tu_Tssharp_3d_Ctrl.reg.Base_Address_Be = reg_Tu_Tssharp_Ctrl.reg.Base_Address_3d;  // umd will set it again, here is just for HW init check.
    pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.ff_regs.reg_Ffc_Config.reg.En_Decompressed_Data_Reorder = 0x1;
    pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.ff_regs.reg_Ffc_Config.reg.Slot_Swz_Enable = 0x1;
    gf_unmap_io_memory(vma);

    if (engine->shadow_context_buf_phy_addr)
    {
        gf_map_argu_t       map_argu = {0};
        gf_vm_area_t        *vma     = NULL;

        map_argu.flags.cache_type = GF_MEM_WRITE_COMBINED;
        map_argu.flags.mem_space  = GF_MEM_KERNEL;
        map_argu.flags.mem_type   = GF_SYSTEM_IO;
        map_argu.phys_addr = adapter->vidmm_bus_addr + engine->shadow_context_buf_phy_addr;
        map_argu.size             = HW_CONTEXT_SIZE_E3K;

        vma = gf_map_io_memory(NULL, &map_argu);
        gf_memset(vma->virt_addr, 0xcd, vma->size);
        pContext = vma->virt_addr;

        pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.eufs_regs.reg_Eu_Full_Glb.uint = 0;
        pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.eufs_regs.reg_Eu_3d_Glb.uint = 0;
        pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.eufs_regs.reg_Fs_Cb_Cfg.uint = 0;
        pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.eups_regs.reg_Ps_Cb_Cfg.uint = 0;
        pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.wls_regs.reg_Ffc_Ubuf_3dconfig.uint = 0;
        pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.wls_regs.reg_Ffc_Ubuf_Golbalconfig.uint = 0;
        pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.tu_regs.reg_Tu_Tssharp_Ctrl.uint = 0;
        pContext->HWContextbuffer.shadow_buf_3D.fe_be_regs.tu_regs.reg_Tu_Tssharp_3d_Ctrl.uint = 0;
        gf_unmap_io_memory(vma);
    }

    if(!engine->is_high_engine)
    {
        RbSize = 22 + (sizeof(CONTEXT_RESTORE_DMA_E3K) >> 2);
        RbSize = (((RbSize) + 15) & ~15);//RB should 16 dw align
        pRB = enginei_get_ring_buffer_space_e3k(engine_common, RbSize);
        gf_memset(pRB, 0, (RbSize << 2));

        *pRB++ = *(DWORD*)&pwmTrigger;
        *pRB++ = *(DWORD*)&trigger_Dw;
        // skip 8dw for hw block sync
        pRB += 8;
        *pRB++ = SET_REGISTER_FD_E3K(EU_FS_BLOCK,Reg_Eu_Full_Glb_Offset,1);
        *pRB++ = reg_Eu_Full_Glb.uint;
        *pRB++ = SET_REGISTER_FD_E3K(TUFE_BLOCK,Reg_Tu_Tssharp_Ctrl_Offset,1);
        *pRB++ = reg_Tu_Tssharp_Ctrl.uint;
        *pRB++ = SET_REGISTER_FD_E3K(WLS_FE_BLOCK,Reg_Ffc_Ubuf_Golbalconfig_Offset,1);
        *pRB++ = reg_Ffc_Ubuf_Golbalconfig.uint;
        *pRB++ = SET_REGISTER_FD_E3K(CSP_GLOBAL_BLOCK, Reg_Fence_Mask_Offset, 1);
        *pRB++ = reg_Fence_Mask.uint;
        *pRB++ = SET_REGISTER_FD_E3K(L2_BLOCK, Reg_L2_Performance_Offset, 1);
        *pRB++ = reg_L2_Performance.uint;

        pRestoreDMA = (CONTEXT_RESTORE_DMA_E3K *)pRB;

        gf_memcpy(pRestoreDMA, &(pcmBuf->RestoreDMA), sizeof(CONTEXT_RESTORE_DMA_E3K));

        pRestoreDMA->RestoreContext.Address_Mode = SET_REGISTER_ADDRESS_MODE_ADDRESS;
        pRestoreDMA->ContextBufferOffset_L = engine->context_buf_phy_addr & 0xFFFFFFFF;
        pRestoreDMA->ContextBufferOffset_H = (engine->context_buf_phy_addr >> 32) & 0xFF;

        //Csp will initinal its registers by itself, no need driver to do it.
        pRestoreDMA->RestoreCsp.uint = 0;
        pRestoreDMA->CSPOffset_H = 0;
        pRestoreDMA->CSPOffset_L = 0;

        pRestoreDMA->RestoreEuFullGlb.uint = 0;
        pRestoreDMA->EuFullGlbOffset_L = 0;
        pRestoreDMA->EuFullGlbOffset_H = 0;

        pRestoreDMA->RestoreTsSharpCtrl.uint = 0;
        pRestoreDMA->TsSharpCtrlOffset_L= 0;
        pRestoreDMA->TsSharpCtrlOffset_H= 0;

        pRestoreDMA->RestoreFFCUbufConfig.uint = 0;
        pRestoreDMA->FFCUbufConfigOffset_L = 0;
        pRestoreDMA->FFCUbufConfigOffset_H = 0;

        pRestoreDMA->RestoreGpcpFe.uint = SET_REGISTER_ADDR_E3K(GPCPFE_BLOCK, Reg_Gpcpfe_Iidcnt_Offset, SET_REGISTER_ADDRESS_MODE_ADDRESS);
        pRestoreDMA->GpcpFeOffset_H     = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K((engine->context_buf_phy_addr >> 32) & 0xFF, sizeof(GPCPFE_CONTEXTBUFFER_REGS) >> 2);
        pRestoreDMA->GpcpFeOffset_L     = SET_REGISTER_ADDR_LOW_E3K((engine->context_buf_phy_addr & 0xFFFFFFFF) + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpFeRegs)));

        pRestoreDMA->RestoreGpcpBe.uint = SET_REGISTER_ADDR_E3K(GPCPBE_BLOCK, Reg_Sto_Cfg_Offset, SET_REGISTER_ADDRESS_MODE_ADDRESS);
        pRestoreDMA->GpcpBeOffset_H     = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K((engine->context_buf_phy_addr >> 32) & 0xFF, sizeof(Gpcpbe_regs) >> 2);;
        pRestoreDMA->GpcpBeOffset_L     = SET_REGISTER_ADDR_LOW_E3K((engine->context_buf_phy_addr & 0xFFFFFFFF) + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcpBeRegs)));

        while (gpc_bit_mask)
        {
            unsigned int gpc_index = 0;
            _BitScanForward(&gpc_index, gpc_bit_mask);
            _bittestandreset((int *)&gpc_bit_mask, gpc_index);
            pRestoreDMA->RestoreGpcTop[gpc_index].RestoreGpcTopCmd.uint = SET_REGISTER_ADDR_E3K(24, 0 + gpc_index * 40, SET_REGISTER_ADDRESS_MODE_ADDRESS);
            pRestoreDMA->RestoreGpcTop[gpc_index].GpcTopOffset_L = SET_REGISTER_ADDR_LOW_E3K((engine->context_buf_phy_addr & 0xFFFFFFFF)
                                                            + (unsigned long)(&(((CONTEXT_BUFFER_E3K*)0)->GpcTopRegs[gpc_index])));
            pRestoreDMA->RestoreGpcTop[gpc_index].GpcTopOffset_H = SET_REGISTER_ADDR_HIGH_AND_REGCNT_E3K(0x0, 40);
        }

        pRB = pRB + (sizeof(CONTEXT_RESTORE_DMA_E3K)>> 2);

        *pRB++ = *(DWORD*)&pwmTriggerOff;
        *pRB++ = *(DWORD*)&triggerOff_Dw;

        enginei_do_kickoff_cmd_e3k(engine_common);
    }

    return S_OK;
}

int enginei_init_pwm_reg_e3k(engine_e3k_t *engine)
{
    adapter_t *   adapter = engine->vidsch->adapter;
    unsigned int  cmd_mmio = 1;

    Reg_Pwr_Mgr_Cfg_En pwr_mgr_cfg_en     = { .uint = 0x0 };
    unsigned int cg_wait_cnt = 0x08202020;//default value

    pwr_mgr_cfg_en.reg.En_Clkgate_3d  = adapter->pm_caps.gfx_cg_auto;
    pwr_mgr_cfg_en.reg.En_Clkgate_Vcp = adapter->pm_caps.vcp_cg_auto;
    pwr_mgr_cfg_en.reg.En_Clkgate_Vpp = adapter->pm_caps.vpp_cg_auto;

    //Command mode
    if (cmd_mmio == 0)
    {
        unsigned int *pRB     = NULL;
        unsigned int *pRB0    = NULL;
        unsigned int rb_size_uint       = 21;
        unsigned int align_rb_size_uint = util_align(rb_size_uint, 16);

        Cmd_Blk_Cmd_Csp_Indicator        pwmTrigger    = { 0 };
        Cmd_Blk_Cmd_Csp_Indicator_Dword1 trigger_Dw    = { 0 };
        Cmd_Blk_Cmd_Csp_Indicator        pwmTriggerOff = { 0 };
        Cmd_Blk_Cmd_Csp_Indicator_Dword1 triggerOff_Dw = { 0 };

        pwmTrigger.Major_Opcode = CSP_OPCODE_Blk_Cmd_Csp_Indicator;
        pwmTrigger.Block_Id     = CSP_GLOBAL_BLOCK;
        pwmTrigger.Type         = BLOCK_COMMAND_CSP_TYPE_INDICATOR;
        pwmTrigger.Info         = BLK_CMD_CSP_INDICATOR_INFO_3D_MODE;
        pwmTrigger.Dwc          = 1;
        trigger_Dw.Slice_Mask   = adapter->hw_caps.chip_slice_mask;

        pwmTriggerOff.Major_Opcode = CSP_OPCODE_Blk_Cmd_Csp_Indicator;
        pwmTriggerOff.Block_Id     = CSP_GLOBAL_BLOCK;
        pwmTriggerOff.Type         = BLOCK_COMMAND_CSP_TYPE_INDICATOR;
        pwmTriggerOff.Info         = BLK_CMD_CSP_INDICATOR_INFO_OFF_MODE;
        pwmTriggerOff.Dwc          = 1;

        pRB = pRB0 = (unsigned int *)enginei_get_ring_buffer_space_e3k(engine, align_rb_size_uint);
        gf_memset(pRB, 0, (align_rb_size_uint << 2));

        *pRB++ = *(unsigned int *)&pwmTrigger;
        *pRB++ = *(unsigned int *)&trigger_Dw;
        // skip 8dw for hw block sync
        pRB += 8;

        *pRB++ = SET_REGISTER_FD_E3K(CSP_GLOBAL_BLOCK, Reg_Pwr_Mgr_Cfg_En_Offset, 1);
        *pRB++ = pwr_mgr_cfg_en.uint;

        *pRB++ = SET_REGISTER_FD_E3K(CSP_GLOBAL_BLOCK, Reg_Pwr_Mgr_Wait_Cnt_Offset, 1);
        *pRB++ = cg_wait_cnt;

        *pRB++ = SEND_EXTERNAL_FENCE_E3K(FENCE_IRQ_INTERRUPT_CPU);
        *pRB++ = (engine->fence_buffer_phy_addr + DH_FENCE_OFFSET * sizeof(unsigned long long)) & 0xFFFFFFFF;
        *pRB++ = ((engine->fence_buffer_phy_addr + DH_FENCE_OFFSET * sizeof(unsigned long long)) >> 32) & 0xFF;
        *pRB++ = (DH_FENCE_VALUE)&0xFFFFFFFF;
        *pRB++ = (((unsigned long long)(DH_FENCE_VALUE)) >> 32) & 0xFFFFFFFF;
        *pRB++ = *(unsigned int *)&pwmTriggerOff;
        *pRB++ = *(unsigned int *)&triggerOff_Dw;

        enginei_do_kickoff_cmd_e3k(engine);
    }
    else if (cmd_mmio == 1)
    {
        gf_write32(adapter->mmio + MMIO_CSP_START_ADDRESS + Reg_Pwr_Mgr_Cfg_En_Offset * 4, pwr_mgr_cfg_en.uint);
        gf_write32(adapter->mmio + MMIO_CSP_START_ADDRESS + Reg_Pwr_Mgr_Wait_Cnt_Offset * 4, cg_wait_cnt);
    }

    gf_info("init pwm auto mode %d done\n", pwr_mgr_cfg_en.uint);

    return S_OK;
}

int engine_gfx_low_init_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr)
{
    vidmm_segment_memory_t  *local_reserved_memory = sch_mgr->local_reserved_memory;
    engine_gfx_e3k_t       *engine                = gf_calloc(sizeof(engine_gfx_e3k_t));
    engine_share_e3k_t      *share                 = NULL;
    unsigned long long   local_reserved_start;
    unsigned int     current_pcie_offset   = 0;
    unsigned int     current_low_offset    = 0;
    unsigned int     start_align;

    local_reserved_start = local_reserved_memory->gpu_virt_addr;

    if (engine == NULL)
    {
        gf_error("%s, out of mem.\n", util_remove_name_suffix(__func__));
        gf_assert(0, "engine_gfx_low_init");
        return S_OK;
    }

    sch_mgr->boost_level = ENG_POWER_STATE_E2;

    start_align = ((local_reserved_start + HW_CONTEXT_E3K_ALIGN)& ~HW_CONTEXT_E3K_ALIGN) - local_reserved_start;
    current_low_offset += start_align;

    enginei_common_init_e3k(sch_mgr, &engine->common, &current_pcie_offset, &current_low_offset);

    share = engine->common.share;

    share->context_buf_phy_addr =   //high and low use same hwctx template
    engine->context_buf_phy_addr = local_reserved_start + current_low_offset;

    //gf_info("%s:context_buf_phy_addr:%x, need 0x4000 align\n",util_remove_name_suffix(__func__), engine->context_buf_phy_addr);//for debug

    current_low_offset += HW_CONTEXT_SIZE_E3K * HW_CONTEXT_COUNT_E3K;

    engine->shadow_context_buf_phy_addr = local_reserved_start + current_low_offset;
    current_low_offset += HW_CONTEXT_SIZE_E3K;

    engine->temp_context_buf_phy_addr = local_reserved_start + current_low_offset;
    current_low_offset += HW_CONTEXT_SIZE_E3K;

    /* allocate begin end buffer from low reserved */
    share->begin_end_buffer_phy_addr = local_reserved_start + current_low_offset;

    //gf_assert((share->begin_end_buffer_phy_addr & 0xFFF) == 0, NULL);
    //gf_info("%s:begin_end_buffer_phy_addr:%x, need 4k align\n",util_remove_name_suffix(__func__), share->begin_end_buffer_phy_addr);//for debug

    enginei_init_context_save_restore_dma_e3k(engine);
    current_low_offset += BEGIN_END_BUF_SIZE_E3K;

    enginei_init_ring_buffer_commands_e3k(engine);

    share->_3dblt_data_lock = gf_create_mutex();

    /* 3d blt Initialize */
    share->_3dblt_data_phy_addr = local_reserved_start + current_low_offset;

    share->_3dblt_data_size = 0;

    share->_3dblt_data_shadow = NULL;

    current_low_offset += _3DBLT_DATA_RESERVE_SIZE;

    engine->is_high_engine = 0;

    if (!adapter->hw_caps.video_only)
    {
        enginei_init_hardware_context_e3k(engine);
    }

    if (adapter->chip_id >= CHIP_ARISE2030 && adapter->pm_caps.pwm_auto)
    {
        enginei_init_pwm_reg_e3k(&engine->common);
    }

    enginei_common_share_init_e3k(&engine->common);

    gf_assert(current_low_offset <= local_reserved_memory->list_node->aligned_size, "current_low_offset <= local_reserved_memory->list_node->aligned_size");

    return S_OK;
}

int engine_gfx_high_init_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr)
{
    vidmm_segment_memory_t  *local_reserved_memory = sch_mgr->local_reserved_memory;
    engine_gfx_e3k_t       *engine                = gf_calloc(sizeof(engine_gfx_e3k_t));
    unsigned long long local_reserved_start;
    unsigned int     current_pcie_offset   = 0;
    unsigned int     current_low_offset    = 0;
    unsigned int     start_align;

    local_reserved_start = local_reserved_memory->gpu_virt_addr;

    if(engine == NULL)
    {
        gf_error("%s, out of mem.\n", util_remove_name_suffix(__func__));
        gf_assert(0, "engine_gfx_high_init");
        return S_OK;
    }

    sch_mgr->boost_level = ENG_POWER_STATE_E2;

    start_align = ((local_reserved_start + HW_CONTEXT_E3K_ALIGN)& ~HW_CONTEXT_E3K_ALIGN) - local_reserved_start;
    current_low_offset += start_align;

    engine->context_buf_phy_addr = local_reserved_start + current_low_offset;
    //gf_info("%s:context_buf_phy_addr:%x, need 0x4000 align\n", util_remove_name_suffix(__func__), engine->context_buf_phy_addr);//for debug

    current_low_offset += HW_CONTEXT_SIZE_E3K * HW_CONTEXT_COUNT_E3K;

    enginei_common_init_e3k(sch_mgr, &engine->common, &current_pcie_offset, &current_low_offset);

    engine->is_high_engine = 1;

    if (!adapter->hw_caps.video_only)
    {
        enginei_init_hardware_context_e3k(engine);
    }

    gf_assert(current_low_offset <= local_reserved_memory->list_node->aligned_size, "current_low_offset <= local_reserved_memory->list_node->aligned_size");

    return S_OK;
}

int engine_vcp_init_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr)
{
    engine_vcp_e3k_t       *engine                = gf_calloc(sizeof(engine_vcp_e3k_t));
    unsigned int     current_pcie_offset   = 0;
    unsigned int     current_low_offset    = 0;

    if(engine == NULL)
    {
        gf_error("%s, out of mem.\n", util_remove_name_suffix(__func__));
        gf_assert(0, "engine_vcp_init");
    }
    sch_mgr->boost_level = ENG_POWER_STATE_E1;
    enginei_common_init_e3k(sch_mgr, engine, &current_pcie_offset, &current_low_offset);

    return S_OK;
}

int engine_vpp_init_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr)
{
    engine_vpp_e3k_t       *engine                = gf_calloc(sizeof(engine_vpp_e3k_t));
    unsigned int     current_pcie_offset   = 0;
    unsigned int     current_low_offset    = 0;

    if(engine == NULL)
    {
        gf_error("%s, out of mem.\n", util_remove_name_suffix(__func__));
        gf_assert(0, "engine_vpp_init");
    }

    sch_mgr->boost_level = ENG_POWER_STATE_E1;
    enginei_common_init_e3k(sch_mgr, engine, &current_pcie_offset, &current_low_offset);

    return S_OK;
}

void engine_gfx_low_restore_e3k(vidsch_mgr_t *sch_mgr, unsigned int pm)
{
    adapter_t *adapter = sch_mgr->adapter;
    engine_gfx_e3k_t    *engine  = sch_mgr->private_data;
    engine_share_e3k_t  *share   = engine->common.share;

    if (!adapter->in_suspend_resume)
        vidschi_reset_adapter_e3k(sch_mgr->adapter);

    //fix issue 18315, before system sleep, vcp clk is off,
    //when resume, open vcp clk before disable vcp decouple in vidmm_init_mem_settings_e3k
    vidsch_power_clock_on_off_vcp(sch_mgr, FALSE);
    vidmm_init_mem_settings_e3k(sch_mgr->adapter);

    vidschi_init_hw_settings_e3k(sch_mgr->adapter);

    enginei_enable_ring_buffer_e3k(&engine->common);

    /* re-initialize video memory resource */
    enginei_init_context_save_restore_dma_e3k(engine);

    enginei_restore_3dblt_data_e3k(engine);

    if (!adapter->hw_caps.video_only)
    {
        enginei_init_hardware_context_e3k(engine);
    }

    if (adapter->chip_id >= CHIP_ARISE2030 &&  adapter->pm_caps.pwm_auto)
    {
        enginei_init_pwm_reg_e3k(&engine->common);
    }

    if (adapter->suspend_blt_mode == 1)
    {
        // In arm64, memcpy 32MB from Physical memory to IO memory will cost about 3S.
        // So we use vidmm_segment_memory_transfer_e3k() to submit a page task help us copy memory faster.
        if (share->bl_buffer_backup)
        {
            struct _vidmm_segment_memory *segment = (struct _vidmm_segment_memory *)share->bl_buffer_backup;
            vidmm_segment_memory_transfer_e3k(adapter, &share->bl_buffer, segment, 1);
            vidmm_release_segment_memory(adapter, segment);
            share->bl_buffer_backup = NULL;
        }
    }
    else if (adapter->suspend_blt_mode == 2)
    {
        if(pm && share->bl_buffer && share->bl_buffer->list_node->size && share->bl_buffer_backup)
        {

            struct _vidmm_segment_memory *segment = (struct _vidmm_segment_memory *)share->bl_buffer_backup;

            vidmm_map_flags_t flags = {0};
            flags.mem_space = GF_MEM_KERNEL;
            flags.cache_type = GF_MEM_UNCACHED;
            vidmm_map_segment_memory(adapter, NULL, share->bl_buffer, &flags);
            gf_memset(&flags, 0, sizeof(flags));
            flags.write_only = 1;
            flags.mem_space  = GF_MEM_KERNEL;
            vidmm_map_segment_memory(adapter, NULL, segment, &flags);

            //gf_info("engine_gfx_low_restore_e3k: start restore bl_buffer size %d (engine %d)\n",share->bl_buffer->list_node->size, sch_mgr->engine_index);
            gf_memcpy(share->bl_buffer->vma->virt_addr, segment->vma->virt_addr, share->bl_buffer->list_node->size);
            //gf_info("engine_gfx_low_restore_e3k: end restore bl_buffer\n");
            vidmm_unmap_segment_memory(adapter, segment, GF_MEM_KERNEL);
            vidmm_release_segment_memory(adapter, segment);
            vidmm_unmap_segment_memory(adapter, share->bl_buffer, GF_MEM_KERNEL);
            share->bl_buffer_backup = NULL;
        }
    }
    else
    {
        if(pm && share->bl_buffer &&share->bl_buffer->vma && share->bl_buffer_backup)
        {
            gf_memcpy(share->bl_buffer->vma->virt_addr, share->bl_buffer_backup, share->bl_buffer->list_node->size);

            //util_dump_memory(share->bl_buffer->vma->virt_addr, 4*1024, "first 4k bl");

            vidmm_unmap_segment_memory(adapter, share->bl_buffer, GF_MEM_KERNEL);

            gf_free(share->bl_buffer_backup);
            share->bl_buffer_backup = NULL;
        }
    }
}

void engine_gfx_high_restore_e3k(vidsch_mgr_t *sch_mgr, unsigned int pm)
{
    engine_gfx_e3k_t  *engine = sch_mgr->private_data;
    adapter_t *adapter = sch_mgr->adapter;

    enginei_enable_ring_buffer_e3k(&engine->common);

    if (!adapter->hw_caps.video_only)
    {
        enginei_init_hardware_context_e3k(engine);
    }
}

int engine_cil2_misc_e3k(vidsch_mgr_t *sch_mgr, krnl_cil2_misc_t *misc)
{
    int                 hRet = S_OK;
    engine_e3k_t        *engine = sch_mgr->private_data;
    engine_share_e3k_t *share = engine->share;
    gf_cil2_misc_t      *gf_misc = misc->gf_misc;

    switch(gf_misc->op_code)
    {
    case GF_MISC_GET_3DBLT_CODE:
        {
            gf_mutex_lock(share->_3dblt_data_lock);
            gf_misc->get_3dblt_code.size = share->_3dblt_data_size;
            gf_misc->get_3dblt_code.capacity = _3DBLT_DATA_RESERVE_SIZE;
            gf_misc->get_3dblt_code.base = share->_3dblt_data_phy_addr;
            gf_mutex_unlock(share->_3dblt_data_lock);
            break;
        }
    case GF_MISC_SET_3DBLT_CODE:
        {
            gf_map_argu_t    map_argu     = {0};
            gf_vm_area_t    *vma          = NULL;

            gf_mutex_lock(share->_3dblt_data_lock);
            if (share->_3dblt_data_size == 0)
            {
                gf_assert(gf_misc->set_3dblt_code.size <= _3DBLT_DATA_RESERVE_SIZE, "3dblt_data overflow");

                map_argu.flags.cache_type = GF_MEM_WRITE_COMBINED;
                map_argu.flags.mem_space  = GF_MEM_KERNEL;
                map_argu.flags.mem_type   = GF_SYSTEM_IO;
                map_argu.phys_addr        = sch_mgr->adapter->vidmm_bus_addr + share->_3dblt_data_phy_addr;
                map_argu.size             = gf_misc->set_3dblt_code.size;

                vma = gf_map_io_memory(NULL, &map_argu);
                if (vma)
                {
                    share->_3dblt_data_size = gf_misc->set_3dblt_code.size;
                    share->_3dblt_data_shadow = gf_malloc(share->_3dblt_data_size);
                    gf_copy_from_user(share->_3dblt_data_shadow, ptr64_to_ptr(gf_misc->set_3dblt_code.data), share->_3dblt_data_size);

                    gf_memcpy(vma->virt_addr, share->_3dblt_data_shadow, share->_3dblt_data_size);

                    gf_unmap_io_memory(vma);
                }
                else
                {
                    gf_error("map 3dblt_data_phy_addr failed.\n");
                    hRet = E_FAIL;
                }
            }
            gf_mutex_unlock(share->_3dblt_data_lock);
        }
        break;
    case GF_MISC_VIDEO_FENCE_GET:
        {
            if (gf_misc->video_fence_get.index < 8)
            {
                gf_spin_lock(share->lock);
                gf_misc->video_fence_get.fence = share->internal_fence_value[gf_misc->video_fence_get.index]++;
                gf_spin_unlock(share->lock);
            }
            else
            {
                hRet = E_FAIL;
                gf_error("VIDEO_FENCE_GET wrong index value: %d.\n", gf_misc->video_fence_get.index);
            }
        }
        break;
    case GF_MISC_VIDEO_FENCE_CLEAR:
        {
            if (gf_misc->video_fence_clear.index < 8)
            {
                gf_spin_lock(share->lock);
                share->internal_fence_value[gf_misc->video_fence_clear.index] = 0;
                gf_spin_unlock(share->lock);
            }
            else
            {
                hRet = E_FAIL;
                gf_error("VIDEO_FENCE_CLEAR wrong index value: %d.\n", gf_misc->video_fence_clear.index);
            }
        }
        break;
    case GF_MISC_MAP_GPUVA:
        break;
    default:
        hRet = E_INVALIDARG;
    }

    return hRet;
}

int vidsch_reset_hw_e3k(vidsch_mgr_t *sch_mgr)
{
    adapter_t           *adapter      = sch_mgr->adapter;
    EngineSatus_e3k     engine_status = {0};
    unsigned int        index   = 0;
    unsigned int        fence_back_engine_count = 0;
    engine_e3k_t        *engine;
    unsigned int        *status =(unsigned int*) &engine_status;
    int i;

    for( i = 0; i <7; i++)
    {
        *(status + i) = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + (Reg_Block_Busy_Bits_Top_Offset + i)*4);
    }

    //gf_info("<<----^^^^^^^^^-----[Before reset hw] Current HW status -----^^^^^^^^^^^--->>\n");
    gf_info("current submit engine is 0x%x, Engine Hang and Status: %08x,%08x,%08x,%08x,%08x,%08x,%08x\n",
        sch_mgr->engine_index, engine_status.Top.uint,engine_status.Gpc0_0.uint,engine_status.Gpc0_1.uint,
        engine_status.Gpc1_0.uint,engine_status.Gpc1_1.uint,engine_status.Gpc2_0.uint,engine_status.Gpc2_1.uint);

    if(adapter->ctl_flags.hang_dump)
    {
        vidsch_save_misc_e3k(adapter, sch_mgr, SAVE_AFTER_POSTHANG);//save engine status.
    }
    /* reset hardware engine by engine */
    for(index=0; index< adapter->active_engine_count; index++)
    {
        unsigned int     last_returned_fence_id;
        vidsch_mgr_t     *tmp_sch_mgr = adapter->sch_mgr[index];

        if(tmp_sch_mgr == NULL)
        {
            fence_back_engine_count++;
            continue;
        }

        engine = tmp_sch_mgr->private_data;
        last_returned_fence_id = *(volatile unsigned int *)engine->fence_buffer_virt_addr;

        if(last_returned_fence_id == (unsigned int)tmp_sch_mgr->last_send_fence_id)
        {
            //this engine is running normally
            fence_back_engine_count++;
        }
    }

    /* return fake engine hang */
    if(engine_status.Top.uint == 0 && fence_back_engine_count ==  adapter->active_engine_count)
    {
        gf_error("fake timeout detected\n");
        return E_FAIL;
    }

    /* update fence id skip the dead dma */
    for(index=0; index<adapter->active_engine_count; index++)
    {
        vidsch_mgr_t *tmp_sch_mgr = adapter->sch_mgr[index];

        if(tmp_sch_mgr)
        {
            engine_e3k_t    *engine_tmp  = tmp_sch_mgr->private_data;

            *(volatile unsigned int *)engine_tmp->fence_buffer_virt_addr = tmp_sch_mgr->last_send_fence_id & 0xFFFFFFFF;
            *((volatile unsigned int *)engine_tmp->fence_buffer_virt_addr + 1) = (tmp_sch_mgr->last_send_fence_id >> 32)& 0xFFFFFFFF;

            tmp_sch_mgr->chip_func->restore(tmp_sch_mgr, FALSE);

            tmp_sch_mgr->init_submit = TRUE;
        }
    }

    return S_OK;
}

#ifdef GF_HW_NULL
void engine_set_fence_id_e3k(vidsch_mgr_t *sch_mgr, unsigned long long fence_id)
{
    engine_e3k_t   *engine = sch_mgr->private_data;
    *(engine->fence_buffer_virt_addr) = fence_id;
    gf_info("fake set fence id: %lld, at engine %d\n", fence_id, sch_mgr->engine_index);
}
#endif


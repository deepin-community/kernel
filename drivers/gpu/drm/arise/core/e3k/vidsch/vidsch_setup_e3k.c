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
#include "chip_include_e3k.h"
#include "vidsch_engine_e3k.h"
#include "stm_context_e3k.h"
#include "vidsch_blt_e3k.h"
#include "mm_e3k.h"
#include "vidsch_dfs_e3k.h"

static vidsch_chip_func_t engine_gfx_low_chip_func_e3k;
static vidsch_chip_func_t engine_gfx_high_chip_func_e3k;
static vidsch_chip_func_t engine_vcp_low0_chip_func_e3k;
static vidsch_chip_func_t engine_vcp_low1_chip_func_e3k;
static vidsch_chip_func_t engine_vpp_chip_func_e3k;


//*****************************************************************************
//
//  vidschi_init_hw_settings_e3k
//
// function to init HW MXU register, so that PCIe Ring Buffer can
//  work properly.
//
//*****************************************************************************
void vidschi_init_hw_settings_e3k(adapter_t *adapter)
{
    Reg_Bl_Size                     reg_Bl_Size                   = {0};
    unsigned char *                 pRegAddr                      = NULL;
    vidmm_segment_t                 *segment                      = NULL;
    heap_t                          *heap                         = NULL;
    Reg_Rb0_Fl2                     KKKRb0                        = {0};
    Reg_Rb1_Fl2                     KKKRb1                        = {0};
    Reg_Rb2_Fl2                     KKKRb2                        = {0};
    Reg_Argument_Fl2                ArgumentBuffer                = {0};

    heap = vidmm_get_burst_length_heap(adapter);
    reg_Bl_Size.reg.Buffer_Offset = heap->start >> util_log2(BL_BUFFER_ALIGN);
    reg_Bl_Size.reg.Buffer_Size = util_log2(BL_BUFFER_SIZE >> 23);
    reg_Bl_Size.reg.Invalid_Bl_To_Rf_En = 1;
    pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Bl_Size_Offset*4;
    gf_write32(pRegAddr, reg_Bl_Size.uint);

    gf_info("reg_Bl_Size.uint 0x%x readvalue: 0x%x, Invalid BL enable:%d \n", reg_Bl_Size.uint, gf_read32(pRegAddr), reg_Bl_Size.reg.Invalid_Bl_To_Rf_En);

    KKKRb0.reg.Size             = RING_BUFFER_SIZE_E3K >> 16;
    KKKRb1.reg.Size             = RING_BUFFER_SIZE_E3K >> 16;
    KKKRb2.reg.Size             = RING_BUFFER_SIZE_E3K >> 16;
    ArgumentBuffer.reg.Size     = KKK_ARGUMENT_BUFFER_SIZE >> 16;

    pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Rb0_Fl2_Offset*4;
    gf_write32(pRegAddr, KKKRb0.uint);
    pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Rb1_Fl2_Offset*4;
    gf_write32(pRegAddr, KKKRb1.uint);
    pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Rb2_Fl2_Offset*4;
    gf_write32(pRegAddr, KKKRb2.uint);
    pRegAddr = adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Argument_Fl2_Offset*4;
    gf_write32(pRegAddr, ArgumentBuffer.uint);

    {
        // TODO: secure range regs is 64k aligned, needs align shift?? seems yes!!
        //per Xinzhao, secure regs can't be access through mmio, pending for refine to PMP.
        //BL set to secure range 0
        //Per Baldwin, driver should cfg each miu about secure info.
        int i = 0;

        for (i = 0; i < adapter->hw_caps.miu_channel_num; i++)
        {
            gf_write32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000,     ((heap->start >> 16) << 12));
            gf_write32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000 + 4, (((heap->start + heap->size) >> 16) << 12));

            //gf_info("BL:exp:%x,read:%x, exp:%x,read:%x\n", ((heap->start >> 16) << 12), gf_read32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000), (((heap->start + heap->size) >> 16) << 12), gf_read32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000 +4));

            if(adapter->hw_caps.secure_range_enable)
            {
                //set secure range1, secure range
                segment = vidmm_get_segment_by_id(adapter, SEGMENT_ID_SECURE_RANGE_E3K);
                gf_write32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000 + 1*8,     ((segment->gpu_vm_start >> 16) << 12));
                gf_write32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000 + 1*8 +4, (((segment->gpu_vm_start + segment->gpu_vm_size) >> 16) << 12));
                gf_info("SEC[1]exp:%x,read:%x, exp:%x,read:%x\n", ((segment->gpu_vm_start >> 16) << 12), gf_read32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000 +8 ), (((segment->gpu_vm_start + segment->gpu_vm_size) >> 16) << 12), gf_read32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000 +0xc));

                //set secure range2, non-secure range
                segment = vidmm_get_segment_by_id(adapter, SEGMENT_ID_SECURE_RANGE_E3K_1);
                gf_write32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000 + 2*8,     ((segment->gpu_vm_start >> 16) << 12));
                gf_write32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000 + 2*8 +4, (((segment->gpu_vm_start + segment->gpu_vm_size) >> 16) << 12));
                gf_info("SEC[2]exp:%x,read:%x, exp:%x,read:%x\n", ((segment->gpu_vm_start >> 16) << 12), gf_read32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000 +2*8 ), (((segment->gpu_vm_start + segment->gpu_vm_size) >> 16) << 12), gf_read32(adapter->mmio + MIU0_RANGE_BASE_ADDRESS + i*0x1000 + 2*8 +4));

                //set scramble bits[8,15]: range0~7; bit[0]:1 means enable scramble. Rang0 for BL, should not enable scramble
                gf_write32(adapter->mmio + SECURE_RANGE_ENCRYPTEN_ADDRESS + i*0x1000, ((1 << 9) | (1 << 0)));

                 // set the scramble pattern for later scramble/descramble use.
                gf_write32(adapter->mmio + SECURE_RANGE_SCRAMBLE_PATTEN_ADDRESS + i*0x1000   , 0xFFFFFFFF);
                gf_write32(adapter->mmio + SECURE_RANGE_SCRAMBLE_PATTEN_ADDRESS + i*0x1000 + 0x4, 0xFFFFFFFF);
                gf_write32(adapter->mmio + SECURE_RANGE_SCRAMBLE_PATTEN_ADDRESS + i*0x1000 + 0x8, 0xFFFFFFFF);
                gf_write32(adapter->mmio + SECURE_RANGE_SCRAMBLE_PATTEN_ADDRESS + i*0x1000 + 0xc, 0xFFFFFFFF);
            }
        }
    }
}
/*
assign SWRST[7:0] = REG_CRA3 | REG8524[7:0];
assign SWRST[15:8] = REG_CRA4 | REG8524[15:8];
assign SWRST[23:16] = REG_CRA5 | REG8524[23:16];
assign SWRST[31:24] = REG_CRA6 | REG8524[31:24];
assign SWRST[39:32] = REG_CRB4 | REG85B4[7:0];
assign SWRST[47:40] = REG_CRB5 | REG85B4[15:8];
assign SWRST[55:48] = REG_CRB6 | REG85B4[23:16];
assign SWRST[63:56] = REG_CRB7 | REG85B4[31:24];

The following is the detail for SWRST[23:0]
MIU_SwRst        = SWRST[1]  | SWRST[0];
For_3D_SwRst     = SWRST[2]  | SWRST[0];
For_3D1_SwRst    = SWRST[3]  | SWRST[0];
For_3D2_SwRst    = SWRST[4]  | SWRST[0];
For_3D3_SwRst    = SWRST[5]  | SWRST[0];
VPP_SwRst        = SWRST[6]  | SWRST[0];
GFVD0_SwRst      = SWRST[7]  | SWRST[0];
GFVD1_SwRst      = SWRST[8]  | SWRST[0];
mmio_timer_SwRst = SWRST[9]  | SWRST[0];
m0pll_div_SwRst  = SWRST[10] | SWRST[0];
m1pll_div_SwRst  = SWRST[11] | SWRST[0];
m2pll_div_SwRst  = SWRST[12] | SWRST[0];
epll_div_SwRst   = SWRST[13] | SWRST[0];
vpll_div_SwRst   = SWRST[14] | SWRST[0];
IGA1_SwRst       = SWRST[16] | SWRST[0];
IGA2_SwRst       = SWRST[17] | SWRST[0];
IGA3_SwRst       = SWRST[18] | SWRST[0];

If hang, just need reset 3D,3D1,3D2,3D3,VPP,GFVD0,GFVD1,so set 8524 to 0x01FC.
or we can set CRA3 and CRA4, the mmio offset is 0x8aa3,0x8aa4.
*/
void vidschi_reset_adapter_e3k(adapter_t *adapter)
{
    Reg_HWReset_E3K  dwResetHW = {0};
    volatile unsigned int     i, j;

    gf_disp_wait_idle(adapter->disp_info);

    dwResetHW.uint = 0x01FC;
    gf_write32(adapter->mmio + 0x8524, dwResetHW.uint);

    //wait a while
    j=0;
    for (i=0;i<0x10000;i++){j++;};

    dwResetHW.uint = 0; //clear reset value.
    gf_write32(adapter->mmio + 0x8524, dwResetHW.uint);

    //wait a while
    for (i=0;i<0x10000;i++) {j--;};

    {
        EngineSatus_e3k engine_status = {0};
        unsigned int *status = (unsigned int *)&engine_status;
        int i;

        for( i = 0; i <7; i++)
        {
            *(status + i) = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + (Reg_Block_Busy_Bits_Top_Offset + i)*4);
        }

        //gf_info("after reset......\n");
        gf_info("current engine Status: %08x,%08x,%08x,%08x,%08x,%08x,%08x\n",
            engine_status.Top.uint,engine_status.Gpc0_0.uint,engine_status.Gpc0_1.uint,
            engine_status.Gpc1_0.uint,engine_status.Gpc1_1.uint,engine_status.Gpc2_0.uint,engine_status.Gpc2_1.uint);
        //gf_info("done......\n");
    }
}


//*****************************************************************************
//
// vidschi_init_lookup_table_e3k
//
// Init ring buffer lookup table, this table map from node_ordinal and
// engine_affinity to specific engine index
//
//*****************************************************************************
static void vidschi_init_lookup_table_e3k(adapter_t  *adapter, vidsch_query_private_t *query)
{
    int i, total_size = 0;

    query->engine_count = RB_NUM;

    if(adapter->hw_caps.local_only)
    {
        query->pcie_segment_id         = SEGMENT_ID_LOCAL_E3K;
        query->local_segment_id        = SEGMENT_ID_LOCAL_E3K;
#if defined(__mips64__) || defined(__loongarch__)
        //PCIE cfg 0x68[14:12] defualt value is 010b, which means 512 Bytes maximum Read Request size
        //mips system BIOS(loonson PMON) modify this value to 000b, which means 128 Bytes maximum Read Request size
        //if fence buffer is in LOCAL, BIU will receive 8*256 bits Dummy Fence Read Request, which exceed 128 Bytes,
        //and BIU should split this Dummy Fence Request to two Dummy Fence Request,
        //but BIU has a bug which leads to the second Dummy Fence Read Request as a normal request
        query->fence_buffer_segment_id = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
#else
        query->fence_buffer_segment_id = SEGMENT_ID_LOCAL_E3K;
#endif
    }
    else
    {
        query->local_segment_id        = SEGMENT_ID_LOCAL_E3K;

        if (adapter->hw_caps.snoop_only)
        {
            query->fence_buffer_segment_id = SEGMENT_ID_GART_SNOOPABLE_E3K;
            query->pcie_segment_id         = SEGMENT_ID_GART_SNOOPABLE_E3K;
        }
        else
        {
#if defined(__mips64__) || defined(__loongarch__)
            //In mips or loongarch system, arise(1020,10C0), because of the HW issue, the FE/BE fence value may be not updated when fence buffer
            // is in pcie memory.
            query->fence_buffer_segment_id = SEGMENT_ID_LOCAL_E3K;
#else
            query->fence_buffer_segment_id = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
#endif
            query->pcie_segment_id         = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
        }

    }

    for ( i = 0; i < query->engine_count; i++)
    {
        //query->local_memory_size[i]= HW_CONTEXT_SIZE_E3K; // for hw context buffer start addr align requirement
        //query->local_memory_size[i]  += E3K_3DBLT_RESERVED_SIZE;  //kernel 3D blt buffer size
        query->engine_hw_queue_size[i] = E3K_ENGINE_HW_QUEUE_SIZE;

        switch(i)
        {
        case RB_INDEX_GFXL:
            adapter->engine_index_table[i].node_ordinal    = 0;
            adapter->engine_index_table[i].engine_affinity = 1;

            query->engine_caps[i]      = ENGINE_CAPS_PAGING | ENGINE_CAPS_PRESENT_ALL |
                                                    ENGINE_CAPS_2D_GRAPHICS | ENGINE_CAPS_3D_GRAPHICS;
            query->engine_ctrl[i]      = ENGINE_CTRL_THREAD_ENABLE;
            query->dma_buffer_size[i]  = 2*1024*1024;
            query->pcie_memory_size[i] = RING_BUFFER_SIZE_E3K;
            query->local_memory_size[i]= (HW_CONTEXT_SIZE_E3K * (HW_CONTEXT_COUNT_E3K + 3))
                                                                + BEGIN_END_BUF_SIZE_E3K;
            total_size += _3DBLT_DATA_RESERVE_SIZE;

            query->local_memory_size[i] += total_size;
            query->engine_func[i]        = &engine_gfx_low_chip_func_e3k;
            break;

        case RB_INDEX_GFXH:
            adapter->engine_index_table[i].node_ordinal    = 1;
            adapter->engine_index_table[i].engine_affinity = 1;

            query->engine_caps[i]      = ENGINE_CAPS_PRESENT_ALL | ENGINE_CAPS_2D_GRAPHICS | ENGINE_CAPS_3D_GRAPHICS;
            query->engine_ctrl[i]      = ENGINE_CTRL_DISABLE;
            query->dma_buffer_size[i]  = 1*1024*1024;
            query->pcie_memory_size[i] = RING_BUFFER_SIZE_E3K;

            query->local_memory_size[i]= (HW_CONTEXT_SIZE_E3K * (HW_CONTEXT_COUNT_E3K + 1));
            query->engine_func[i]      = &engine_gfx_high_chip_func_e3k;
            break;
        case RB_INDEX_CSH:
        case RB_INDEX_CSL0:
        case RB_INDEX_CSL1:
        case RB_INDEX_CSL2:
        case RB_INDEX_CSL3:
            adapter->engine_index_table[i].node_ordinal    = i;
            adapter->engine_index_table[i].engine_affinity = 1;
            query->engine_ctrl[i]      = ENGINE_CTRL_DISABLE;
            break;
        case RB_INDEX_VCP0:
            adapter->engine_index_table[i].node_ordinal    = 7;
            adapter->engine_index_table[i].engine_affinity = 1;

            query->engine_caps[i]      = ENGINE_CAPS_VIDEO_DECODE | ENGINE_CAPS_VIDEO_ENCODE | ENGINE_CAPS_NO_HWCTX;
            query->engine_ctrl[i]      = ENGINE_CTRL_THREAD_ENABLE;
            query->dma_buffer_size[i]  = 1*1024*1024;
            query->pcie_memory_size[i] = RING_BUFFER_SIZE_E3K;
            query->local_memory_size[i]= VIDEO_BRIDGE_BUF_SIZE;
            query->engine_func[i]      = &engine_vcp_low0_chip_func_e3k;
            break;
        case RB_INDEX_VCP1:
            adapter->engine_index_table[i].node_ordinal    = 8;
            adapter->engine_index_table[i].engine_affinity = 1;

            query->engine_caps[i]      = ENGINE_CAPS_VIDEO_DECODE | ENGINE_CAPS_VIDEO_ENCODE | ENGINE_CAPS_NO_HWCTX;
            query->engine_ctrl[i]      = ENGINE_CTRL_THREAD_ENABLE;
            query->dma_buffer_size[i]  = 1*1024*1024;
            query->pcie_memory_size[i] = RING_BUFFER_SIZE_E3K;
            query->local_memory_size[i]= VIDEO_BRIDGE_BUF_SIZE;
            query->engine_func[i]      = &engine_vcp_low1_chip_func_e3k;
            break;
        case RB_INDEX_VPP:
            adapter->engine_index_table[i].node_ordinal    = 9;
            adapter->engine_index_table[i].engine_affinity = 1;

            query->engine_caps[i]      = ENGINE_CAPS_VIDEO_VPP | ENGINE_CAPS_NO_HWCTX;
            query->engine_ctrl[i]      = ENGINE_CTRL_THREAD_ENABLE;
            query->dma_buffer_size[i]  = 1*1024*1024;
            query->pcie_memory_size[i] = RING_BUFFER_SIZE_E3K;
            query->local_memory_size[i]= 0;
            query->engine_func[i]      = &engine_vpp_chip_func_e3k;
            break;

        default:
            break;
        }

        if(adapter->hw_caps.local_only)
        {
            query->dma_segment[i]      = SEGMENT_ID_LOCAL_E3K;
        }
        else
        {
            query->dma_segment[i]      = SEGMENT_ID_GART_SNOOPABLE_E3K;

            if(! adapter->hw_caps.snoop_only)
            {
#if !ENABLE_UNSNOOPABLE_WROKAROUND
                query->dma_segment[i]      = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
#endif
            }
        }
    }

    if(adapter->ctl_flags.hang_dump)
    {
        engine_share_e3k_t      *share     = adapter->private_data;
        unsigned int            segment_id        = SEGMENT_ID_LOCAL_E3K;

        /* reserve ring buffer for hang */
        share->ring_buffer_for_hang   = vidmm_allocate_segment_memory(adapter, segment_id, E3K_RINGBUFFER_FOR_HANG_SIZE, 1);
        gf_info("ringbuffer: gpu virt=%x, size=%x\n", share->ring_buffer_for_hang->gpu_virt_addr, share->ring_buffer_for_hang->list_node->aligned_size);

        /* reserve context buffer for hang */
        share->context_buffer_for_hang   = vidmm_allocate_segment_memory(adapter, segment_id, E3K_CONTEXTBUFFER_FOR_HANG_SIZE, 1);
        gf_info("contextbuffer: gpu virt=%x, size=%x\n", share->context_buffer_for_hang->gpu_virt_addr, share->context_buffer_for_hang->list_node->aligned_size);

        /* reserve dma buffer for hang */
        share->dma_buffer_for_hang   = vidmm_allocate_segment_memory(adapter, segment_id, E3K_DMABUFFER_FOR_HANG_SIZE, 1);
        gf_info("dmabuffer: gpu virt=%x, size=%x\n", share->dma_buffer_for_hang->gpu_virt_addr, share->dma_buffer_for_hang->list_node->aligned_size);

        /* reserve pcie buffer for copy cpu invisble segment out*/
        segment_id = SEGMENT_ID_GART_UNSNOOPABLE_E3K;
        share->transfer_buffer_for_hang = vidmm_allocate_segment_memory(adapter, segment_id, E3K_TRANSFERBUFFER_FOR_HANG_SIZE, 0);
        gf_info("transfer buffer for hang: gpu virt=%x, size=%x\n", share->transfer_buffer_for_hang->gpu_virt_addr, share->transfer_buffer_for_hang->list_node->aligned_size);

        vidschi_map_e3k(adapter);
    }
}

static int vidschi_init_engine_share_e3k(adapter_t *adapter)
{
    engine_share_e3k_t  *share = gf_calloc(sizeof(engine_share_e3k_t));

    share->lock = gf_create_spinlock(0);

    adapter->private_data = share;

    //allocate burst length buffer in local reserved
    {
        heap_t *heap = NULL;
        unsigned long long    heap_size, heap_start;

        heap = vidmm_get_burst_length_heap(adapter);

        gf_assert(BL_SLICE_ALIGNMENT <= adapter->os_page_size, "slot buffer alignment is less than page size alignment.");

        share->bl_buffer = vidmm_allocate_segment_memory(adapter, SEGMENT_ID_LOCAL_E3K, (BL_BUFFER_SIZE + BL_BUFFER_ALIGN), 0);

        gf_info("bl_buffer: gpu_virt_addr=0x%llx, size=0x%llx\n", share->bl_buffer->gpu_virt_addr, share->bl_buffer->list_node->aligned_size);

        heap_start = (share->bl_buffer->gpu_virt_addr + BL_BUFFER_ALIGN - 1) & (~(BL_BUFFER_ALIGN-1));//align
        heap_size  = BL_BUFFER_SIZE;

        gf_info("bl_buffer aligned: gpu_virt_addr=0x%llx, size=0x%llx\n", heap_start, heap_size);

        heap_init(heap, 1, heap_start, heap_size, BL_SLICE_ALIGNMENT);
    }

    {
        vidmm_map_flags_t flags = {0};
        unsigned int segment_id =
        adapter->hw_caps.local_only ? SEGMENT_ID_LOCAL_E3K: (adapter->hw_caps.snoop_only ? SEGMENT_ID_GART_SNOOPABLE_E3K : SEGMENT_ID_GART_UNSNOOPABLE_E3K);

        flags.mem_space = GF_MEM_KERNEL;
        flags.cache_type = GF_MEM_UNCACHED;

        share->flush_fifo_buffer = vidmm_allocate_segment_memory(adapter, segment_id, 0x1000, 0);

        vidmm_map_segment_memory(adapter, NULL, share->flush_fifo_buffer, &flags);

        gf_info("flush_fifo_buffer: gpu_virt_addr=%x, kmd_virt_addr=%p\n", share->flush_fifo_buffer->gpu_virt_addr, share->flush_fifo_buffer->vma->virt_addr);
    }

    return S_OK;
}

static int vidschi_init_engine_schedule_profile_e3k(adapter_t *adapter, vidsch_query_private_t *query)
{
    int i;

    for ( i = 0; i < query->engine_count; i++)
    {
        switch(i)
        {
        case RB_INDEX_GFXL:
        //case RB_INDEX_GFXH:
        case RB_INDEX_VCP0:
        case RB_INDEX_VCP1:
        case RB_INDEX_VPP:
            query->schedule_serialize[i] = 1;
            break;
        default:
            query->schedule_serialize[i] = 0;
            break;
        }
    }

    return 0;
}

int vidsch_query_chip(adapter_t *adapter, vidsch_query_private_t *query)
{
    //enable reset for qt/HAPS test
    if(adapter->ctl_flags.run_on_qt)
    {
        //vidschi_reset_adapter_e3k(adapter);
    }

    vidschi_init_engine_share_e3k(adapter);

    vidschi_init_lookup_table_e3k(adapter, query);

    vidschi_init_hw_settings_e3k(adapter);

    vidschi_init_engine_schedule_profile_e3k(adapter, query);

    return S_OK;
}

static void vidsch_get_set_reg_e3k(adapter_t *adapter, gf_query_info_t *info)
{
    unsigned int *buf;
    int i=0;

    switch(info->type)
    {
        case GF_QUERY_GPU_TIME_STAMP:
        {
            unsigned int reg_offset = 0;
            unsigned long long time_stamp_hi, time_stamp_lo;

            reg_offset = (CHIP_ARISE1020 <= adapter->chip_id) ? Reg_Csp_Ref_Total_Gpu_Timestamp_Offset_Arise1020 : Reg_Csp_Ms_Total_Gpu_Timestamp_Offset;

            time_stamp_lo = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS  + reg_offset * 4);
            time_stamp_hi = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS  + reg_offset * 4 + 4);
            info->value64 = time_stamp_lo | (time_stamp_hi << 32);
        }
            break;

        case GF_QUERY_REGISTER_U32:
            buf=gf_calloc(info->buf_len*sizeof(unsigned int));
            if(buf)
            {
                for(i=0;i<info->buf_len;i++)
                {
                    *(buf+i)=gf_read32(adapter->mmio+info->argu+i*sizeof(unsigned int));
                }
            }
            gf_copy_to_user(info->buf, buf, info->buf_len*sizeof(unsigned int));
            gf_free(buf);
            break;

        case GF_SET_REGISTER_U32:
            gf_write32(adapter->mmio+info->argu, info->value);
            break;
        default:
            break;
    }
}

static void vidsch_set_miu_reg_e3k(adapter_t *adapter,gf_query_info_t *info)
{
    gf_write32(adapter->mmio+info->argu, info->value);
}

static void vidsch_read_miu_reg_e3k(adapter_t *adapter,gf_query_info_t *info)
{
    info->value=gf_read32(adapter->mmio+info->argu);
}

static void vidsch_dump_info_e3k(struct os_seq_file *seq_file, adapter_t *adapter)
{
    EngineSatus_e3k     engine_status = {0};
    unsigned int        *status =(unsigned int*) &engine_status;
    unsigned int        power_status = 0;
    unsigned int        gpc_count = (CHIP_ARISE1020 <= adapter->chip_id) ? 1 : 3;
    int i;

    power_status = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + Reg_Pwr_Mgr_Status0_Offset *4);

    gf_seq_printf(seq_file, "engine power/clock status:0x%x\n", power_status);

    for( i = 0; i <7; i++)
    {
        *(status + i) = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + (Reg_Block_Busy_Bits_Top_Offset + i)*4);
    }
    //print the engine status
    gf_seq_printf(seq_file, "E3K engine status:\n");
    gf_seq_printf(seq_file,"Top: 0x%x \n", engine_status.Top.uint);
    gf_seq_printf(seq_file,"\t Csp_Busy    :%d \n", engine_status.Top.reg.Csp_Busy);
    gf_seq_printf(seq_file,"\t Mxua_Busy   :%d \n", engine_status.Top.reg.Mxua_Busy);
    gf_seq_printf(seq_file,"\t Mxub_Busy   :%d \n", engine_status.Top.reg.Mxub_Busy);
    gf_seq_printf(seq_file,"\t L2_Busy     :%d \n", engine_status.Top.reg.L2_Busy);
    gf_seq_printf(seq_file,"\t Gpcpfe_Busy    :%d \n", engine_status.Top.reg.Gpcpfe_Busy);
    gf_seq_printf(seq_file,"\t Gpcpbe_Busy   :%d \n", engine_status.Top.reg.Gpcpbe_Busy);
    gf_seq_printf(seq_file,"\t Sg_Busy   :%d \n", engine_status.Top.reg.Sg_Busy);
    gf_seq_printf(seq_file,"\t Tasfe_Busy     :%d \n", engine_status.Top.reg.Tasfe_Busy);
    gf_seq_printf(seq_file,"\t Tasbe_Busy   :%d \n", engine_status.Top.reg.Tasbe_Busy);
    gf_seq_printf(seq_file,"\t Hub_Busy     :%d \n", engine_status.Top.reg.Hub_Busy);

    for (i = 0; i < gpc_count; i++)
    {
        Reg_Block_Busy_Bits_Gpc0_0 *s0 = (Reg_Block_Busy_Bits_Gpc0_0 *)(status + i * 2 + 1);
        Reg_Block_Busy_Bits_Gpc0_1 *s1 = (Reg_Block_Busy_Bits_Gpc0_1 *)(status + i * 2 + 2);

        gf_seq_printf(seq_file,"GPC : %d  HEX: 0x%x   0x%x\n",i, s0->uint, s1->uint);
        gf_seq_printf(seq_file,"\t Tgz_Busy    :0x%x \n", s0->reg.Tgz_Busy);
        gf_seq_printf(seq_file,"\t Iu_Busy    :0x%x \n", s0->reg.Iu_Busy);
        gf_seq_printf(seq_file,"\t Wbu_Busy    :0x%x \n", s0->reg.Wbu_Busy);
        gf_seq_printf(seq_file,"\t Wls_Busy    :0x%x \n", s0->reg.Wls_Busy);
        gf_seq_printf(seq_file,"\t Ffc_Busy    :0x%x \n", s0->reg.Ffc_Busy);
        gf_seq_printf(seq_file,"\t Tu_Busy    :0x%x \n", s0->reg.Tu_Busy);

        gf_seq_printf(seq_file,"\t Eu_Constructor_Busy    :0x%x \n", s1->reg.Eu_Constructor_Busy);
        gf_seq_printf(seq_file,"\t Euvs_Busy    :0x%x \n", s1->reg.Euvs_Busy);
        gf_seq_printf(seq_file,"\t Euhs_Busy    :0x%x \n", s1->reg.Euhs_Busy);
        gf_seq_printf(seq_file,"\t Eufe_Busy    :0x%x \n", s1->reg.Eufe_Busy);
        gf_seq_printf(seq_file,"\t Euds_Busy    :0x%x \n", s1->reg.Euds_Busy);
        gf_seq_printf(seq_file,"\t Eugs_Busy    :0x%x \n", s1->reg.Eugs_Busy);
        gf_seq_printf(seq_file,"\t Eups_Busy    :0x%x \n", s1->reg.Eups_Busy);
        gf_seq_printf(seq_file,"\t Eucs_Busy    :0x%x \n", s1->reg.Eucs_Busy);
    }
}

static void vidsch_dump_debugbus_e3k(struct os_printer *p, adapter_t *adapter)
{
    EngineSatus_e3k  engine_status;
    unsigned int     *status = (unsigned int *)&engine_status;
    int i;

    /*dump debug bus*/
    if(!adapter->display_debugbus_flag)
    {
        return;
    }

    for( i = 0; i <7; i++)
    {
        *(status + i) = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + (Reg_Block_Busy_Bits_Top_Offset + i)*4);
    }

    gf_printf(p, "ChipSliceMask: %08x\n", adapter->hw_caps.chip_slice_mask);
    gf_printf(p, "Engine Hang and Status: %08x,%08x,%08x,%08x,%08x,%08x,%08x.\n",
        engine_status.Top.uint, engine_status.Gpc0_0.uint, engine_status.Gpc0_1.uint,
        engine_status.Gpc1_0.uint, engine_status.Gpc1_1.uint, engine_status.Gpc2_0.uint, engine_status.Gpc2_1.uint);

    vidsch_display_debugbus_info_e3k(adapter, p, FALSE);
}

static vidsch_chip_func_t engine_gfx_low_chip_func_e3k =
{
    .initialize         = engine_gfx_low_init_e3k,
    .destroy            = engine_destroy_e3k,

    .submit             = engine_submit_task_dma_e3k,
    .update_fence_id    = engine_update_fence_id_e3k,
#ifdef GF_HW_NULL
    .set_fence_id       = engine_set_fence_id_e3k,
#endif

    .cil2_misc        = engine_cil2_misc_e3k,

    .save               = engine_save_e3k,
    .restore            = engine_gfx_low_restore_e3k,

    .render             = vidsch_render_e3k,
    .patch              = vidsch_patch_e3k,

    .reset_hw           = vidsch_reset_hw_e3k,
    .dump_hang_info     = vidsch_dump_hang_info_e3k,
    .dump_hang          = vidsch_dump_hang_e3k,

    .set_miu_reg        = vidsch_set_miu_reg_e3k,
    .read_miu_reg       = vidsch_read_miu_reg_e3k,
    .power_clock        = vidsch_power_clock_on_off_e3k,
};

static vidsch_chip_func_t engine_gfx_high_chip_func_e3k =
{
    .initialize         = engine_gfx_high_init_e3k,
    .destroy            = engine_destroy_e3k,

    .submit             = engine_submit_task_dma_e3k,
    .update_fence_id    = engine_update_fence_id_e3k,

    .cil2_misc        = engine_cil2_misc_e3k,

    .save               = engine_save_e3k,
    .restore            = engine_gfx_high_restore_e3k,

    .render             = vidsch_render_e3k,
    .patch              = vidsch_patch_e3k,

    .reset_hw           = vidsch_reset_hw_e3k,
    .dump_hang_info     = vidsch_dump_hang_info_e3k,
    .dump_hang          = vidsch_dump_hang_e3k,

    .power_clock        = vidsch_power_clock_on_off_e3k,
};

static vidsch_chip_func_t engine_vcp_low0_chip_func_e3k =
{
    .initialize         = engine_vcp_init_e3k,
    .destroy            = engine_destroy_e3k,

    .submit             = engine_submit_task_dma_e3k,
    .update_fence_id    = engine_update_vcp_fence_id_e3k,

    .cil2_misc          = engine_cil2_misc_e3k,

    .save               = engine_save_e3k,
    .restore            = engine_restore_e3k,

    .render             = vidsch_render_e3k,
    .patch              = vidsch_patch_e3k,

    .reset_hw           = vidsch_reset_hw_e3k,
    .dump_hang_info     = vidsch_dump_hang_info_e3k,
    .dump_hang          = vidsch_dump_hang_e3k,

    .power_clock        = vidsch_power_clock_on_off_e3k,
};

static vidsch_chip_func_t engine_vcp_low1_chip_func_e3k =
{
    .initialize         = engine_vcp_init_e3k,
    .destroy            = engine_destroy_e3k,

    .submit             = engine_submit_task_dma_e3k,
    .update_fence_id    = engine_update_vcp_fence_id_e3k,

    .cil2_misc          = engine_cil2_misc_e3k,

    .save               = engine_save_e3k,
    .restore            = engine_restore_e3k,

    .render             = vidsch_render_e3k,
    .patch              = vidsch_patch_e3k,

    .reset_hw           = vidsch_reset_hw_e3k,
    .dump_hang_info     = vidsch_dump_hang_info_e3k,
    .dump_hang          = vidsch_dump_hang_e3k,

    .power_clock        = vidsch_power_clock_on_off_e3k,
};

static vidsch_chip_func_t engine_vpp_chip_func_e3k =
{
    .initialize         = engine_vpp_init_e3k,
    .destroy            = engine_destroy_e3k,

    .submit             = engine_submit_task_dma_e3k,
    .update_fence_id    = engine_update_vpp_fence_id_e3k,

    .cil2_misc        = engine_cil2_misc_e3k,

    .save               = engine_save_e3k,
    .restore            = engine_restore_e3k,

    .render             = vidsch_render_e3k,
    .patch              = vidsch_patch_e3k,

    .reset_hw           = vidsch_reset_hw_e3k,
    .dump_hang_info     = vidsch_dump_hang_info_e3k,
    .dump_hang          = vidsch_dump_hang_e3k,

    .power_clock        = vidsch_power_clock_on_off_e3k,
};

vidschedule_chip_func_t vidschedule_chip_func =
{
    .dump_info             = vidsch_dump_info_e3k,
    .dump_debugbus         = vidsch_dump_debugbus_e3k,
    .power_tuning          = vidsch_power_tuning_e3k,
    .boost                 = vidsch_boost_e3k,
    .get_set_reg           = vidsch_get_set_reg_e3k,
};



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
#include "vidmm.h"
#include "vidsch.h"
#include "vidschi.h"
#include "chip_include_e3k.h"
#include "vidsch_debug_hang_e3k.h"
#include "vidsch_engine_e3k.h"
#include "mm_e3k.h"
#include "global.h"
#include "bit_op.h"

#define WRITE_DUMP_FILE_CHECK(request_size, returned_size) do {\
    if(returned_size != request_size) \
    { \
        gf_error("write error @%d. request: %d, actual: %d\n", __LINE__, request_size, returned_size); \
    }\
    else \
    { \
        gf_info("write file ok, len: 0x%x\n", returned_size); \
    } }while(0)

#define READ_DUMP_FILE_CHECK(request_size, returned_size) do {\
    if(returned_size != request_size) \
    { \
        gf_error("read error @%d. request: %d, actual: %d\n", __LINE__, request_size, returned_size); \
    }\
    else \
    { \
        gf_info("read file ok, len: 0x%x\n", returned_size); \
    }}while(0)

#define PRINT_DEBUG_BUS_INFO(adapter, printer, info)  do {\
    gf_printf(printer, info); \
    if(adapter->ctl_flags.dump_hang_info_to_file) \
    { \
        util_write_to_file(info, gf_strlen(info) , "", HW_HANG_LOG_FILE);\
    }}while(0)

#define DEBUG_BUS_VERSION   "0.0.1"

void vidsch_dump_debugbus_label(adapter_t *adapter, struct os_printer *p)
{
    unsigned char  debug_bus_label[256];

    gf_memset(debug_bus_label, 0, sizeof(debug_bus_label));
    gf_vsprintf(debug_bus_label, "OS: linux\n");
    PRINT_DEBUG_BUS_INFO(adapter, p, debug_bus_label);

    gf_memset(debug_bus_label, 0, sizeof(debug_bus_label));
    gf_vsprintf(debug_bus_label, "Debugbus version:"DEBUG_BUS_VERSION"\n");
    PRINT_DEBUG_BUS_INFO(adapter, p, debug_bus_label);

    gf_memset(debug_bus_label, 0, sizeof(debug_bus_label));
    if(adapter->chip_id < CHIP_ARISE1020)
    {
        gf_vsprintf(debug_bus_label, "Project: ARISE 10C0\n");
    }
    else if (adapter->chip_id == CHIP_ARISE1020)
    {
        gf_vsprintf(debug_bus_label, "Project: ARISE 1020\n");
    }
    else if (adapter->chip_id == CHIP_ARISE2020)
    {
        gf_vsprintf(debug_bus_label, "Project: ARISE 2020\n");
    }
    else if (adapter->chip_id == CHIP_ARISE2030)
    {
        gf_vsprintf(debug_bus_label, "Project: ARISE 2030\n");
    }

    PRINT_DEBUG_BUS_INFO(adapter, p, debug_bus_label);
}

void vidsch_display_debugbus_info_e3k(adapter_t *adapter, struct os_printer *p, int video)
{
    unsigned char  sr1a = 0;
    unsigned char  debug_bus_buffer[256];

    vidsch_dump_debugbus_label(adapter, p);

    gf_vsprintf(debug_bus_buffer, "\n~~~~~~~~~~~~~~debug bus info~~~~~~~~~~~~~~\n");
    PRINT_DEBUG_BUS_INFO(adapter, p, debug_bus_buffer);

    //enable read debug bus from diu
    sr1a = gf_read8(adapter->mmio + 0x861a);
    gf_write8(adapter->mmio + 0x861a, sr1a | 0x10);

    if(video)
    {
        int k = 0;
        int i = 0;
        unsigned char * video_mmio = NULL;
        for(k = 0; k <= 0x1ff; k++)
        {
            video_mmio = adapter->mmio + 0x4C000 + k*4;
            i = gf_read32(video_mmio);
            gf_vsprintf(debug_bus_buffer,"%08x: %08x\n", video_mmio, i);
            gf_info(debug_bus_buffer);
        }

        for(k = 0; k <= 0x1ff; k++)
        {
            video_mmio = adapter->mmio + 0x4A000 + k*4;
            i = gf_read32(video_mmio);
            gf_vsprintf(debug_bus_buffer,"%08x: %08x\n", video_mmio, i);
            gf_info(debug_bus_buffer);
        }

    }

    //3D
    {
        debug_bus_info *debugbus_info = NULL;
        unsigned int  debugbus_info_block_size = 0;
        unsigned int  block_index = 0;
        int           current_gpc = 0xFFFFFFFF;

        switch (adapter->chip_id)
        {
            case CHIP_ARISE1020:
            case CHIP_ARISE1010:
                debugbus_info = (debug_bus_info *)debug_bus_info_Arise1020;
                debugbus_info_block_size = sizeof(debug_bus_info_Arise1020) / sizeof(debug_bus_info);
                break;
            case CHIP_ARISE2030:
            case CHIP_ARISE2020:
                debugbus_info = (debug_bus_info *)debug_bus_info_Arise2030;
                debugbus_info_block_size = sizeof(debug_bus_info_Arise2030) / sizeof(debug_bus_info);
                break;
            case CHIP_ARISE:
            default:
                debugbus_info = (debug_bus_info *)debug_bus_info_E3K;
                debugbus_info_block_size = sizeof(debug_bus_info_E3K) / sizeof(debug_bus_info);
                break;
        }

        gf_info("debugbus_info_block_size %d\n", debugbus_info_block_size);
        for (block_index = 0; block_index < debugbus_info_block_size; block_index++)
        {
            unsigned int reg_start = 0;
            unsigned int reg_end = 0;
            unsigned int reg_index = 0;

            //GPC Title
            if ((debugbus_info[block_index].gpcIndex) != current_gpc)
            {
                current_gpc = debugbus_info[block_index].gpcIndex;

                gf_vsprintf(debug_bus_buffer, "-----------------GPC%d----------------\n", current_gpc);
                PRINT_DEBUG_BUS_INFO(adapter, p, debug_bus_buffer);
            }

            gf_vsprintf(debug_bus_buffer, "-----------------%s----------------\n", debugbus_info[block_index].group_name);
            PRINT_DEBUG_BUS_INFO(adapter, p, debug_bus_buffer);

            reg_start = debugbus_info[block_index].regOpinfo.debugBusWrite.group_start_offset;
            reg_end = debugbus_info[block_index].regOpinfo.debugBusWrite.group_end_offset;
            for (reg_index = reg_start; reg_index <= reg_end; reg_index++)
            {
                //Write Register
                unsigned int write_reg_count = 0;

                for (write_reg_count = 0; write_reg_count < debugbus_info[block_index].regOpinfo.debugBusWrite.write_reg_number; write_reg_count++)
                {
                    unsigned int mask = 0;
                    unsigned int mask_last_bit_index = 0;
                    unsigned int write_value = 0;
                    int mmio_offset = 0;

                    mask = debugbus_info[block_index].regOpinfo.debugBusWrite.write_reg_mask[write_reg_count];
                    _BitScanForward(&mask_last_bit_index, mask);
                    write_value = (reg_index&mask) >> (mask_last_bit_index/4)*4;

                    mmio_offset = debugbus_info[block_index].regOpinfo.debugBusWrite.write_reg_offset[write_reg_count];

                    //1020/2020/2030 BIU select need special write 0xA01F0 31bit as 1
                    if ((adapter->chip_id >= CHIP_ARISE1020) && mmio_offset == 0xA01F0)
                    {
                        gf_write32((unsigned int *)(adapter->mmio + mmio_offset), write_value | 0x80000000);
                    }
                    else
                    {
                        gf_write8((unsigned char *)(adapter->mmio + mmio_offset), (unsigned char)write_value);
                    }
                }

                //Read Register
                {
                    int value0 = 0;
                    int value1 = 0;
                    int value2 = 0;
                    int value3 = 0;
                    int mmio_offset = 0;

                    mmio_offset = debugbus_info[block_index].regOpinfo.debugBusReadback.mmio_offset_0;
                    value0 = gf_read8((unsigned char *)(adapter->mmio + mmio_offset));
                    mmio_offset = debugbus_info[block_index].regOpinfo.debugBusReadback.mmio_offset_1;
                    value1 = gf_read8((unsigned char *)(adapter->mmio + mmio_offset));
                    mmio_offset = debugbus_info[block_index].regOpinfo.debugBusReadback.mmio_offset_2;
                    value2 = gf_read8((unsigned char *)(adapter->mmio + mmio_offset));
                    mmio_offset = debugbus_info[block_index].regOpinfo.debugBusReadback.mmio_offset_3;
                    value3 = gf_read8((unsigned char *)(adapter->mmio + mmio_offset));

                    gf_vsprintf(debug_bus_buffer, "%08x: %08x\n", reg_index, (value0 << 24 | value1 << 16 | value2 << 8 | value3));
                    PRINT_DEBUG_BUS_INFO(adapter, p, debug_bus_buffer);
                }
            }
        }
    }

    gf_vsprintf(debug_bus_buffer, "~~~~~~~~~~~~~~end of debug bus info~~~~~~~~~~~~~~");
    PRINT_DEBUG_BUS_INFO(adapter, p, debug_bus_buffer);
}

void vidsch_dump_hang_info_e3k(vidsch_mgr_t *sch_mgr)
{
    adapter_t        *adapter                 = sch_mgr->adapter;
    engine_e3k_t     *engine                  = sch_mgr->private_data;
    unsigned int     engine_index             = sch_mgr->engine_index;
    unsigned int     RegRbOffset              = EngineRbOffset(engine_index);
    unsigned int     engine_status_offset     = MMIO_CSP_START_ADDRESS + Reg_Block_Busy_Bits_Top_Offset*4;
    unsigned int     cur_rb_head_offset       = MMIO_CSP_START_ADDRESS + (RegRbOffset + 2)*4;
    unsigned int     cur_rb_tail_offset       = MMIO_CSP_START_ADDRESS + (RegRbOffset + 3)*4;
    unsigned int     cur_rb_cmd_offset        = MMIO_CSP_START_ADDRESS + (Reg_Cur_3d_Rbuf_Cmd_Offset + engine_index)*4;
    unsigned int     cur_l1_dma_cmd_offset    = MMIO_CSP_START_ADDRESS + Reg_Cur_L1_Dma_Cmd_Offset*4;
    unsigned int     cur_l2_dma_cmd_offset    = MMIO_CSP_START_ADDRESS + Reg_Cur_L2_Dma_Cmd_Offset*4;
    unsigned int     cur_rb_cmd               = gf_read32(adapter->mmio + cur_rb_cmd_offset);
    unsigned int     cur_rb_head              = gf_read32(adapter->mmio + cur_rb_head_offset);
    unsigned int     cur_rb_tail              = gf_read32(adapter->mmio + cur_rb_tail_offset);
    unsigned int     cur_l1_dma_cmd           = gf_read32(adapter->mmio + cur_l1_dma_cmd_offset);
    unsigned int     cur_l2_dma_cmd           = gf_read32(adapter->mmio + cur_l2_dma_cmd_offset);
    unsigned int     last_returned_fence_id   = *(volatile unsigned int *)engine->fence_buffer_virt_addr;
    EngineSatus_e3k   *engine_status=&engine->dumped_engine_status;
    unsigned int      *status = (unsigned int *)&engine->dumped_engine_status;
    unsigned int      *video_engine_status = (unsigned int *)&engine_status->ES_VCP_VPP;
    int i;

    if(last_returned_fence_id == (unsigned int)sch_mgr->last_send_fence_id)
    {
        return;
    }

    for( i = 0; i <7; i++)
    {
        *(status + i) = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + (Reg_Block_Busy_Bits_Top_Offset + i)*4);
    }

    *video_engine_status = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + Reg_Vcp_Vpp_Block_Busy_Bits_Offset*4);

    if(adapter->display_debugbus_flag)
    {
        gf_info("Engine timeout and Status: %08x,%08x,%08x,%08x,%08x,%08x,%08x\n",
            status[0], status[1], status[2], status[3], status[4], status[5], status[6]);

        gf_info("pid=%d, tid=%d, engine %x is busy. \n",  gf_get_current_pid(), gf_get_current_tid(), engine_index);
        gf_info("last_send_fence_id:0x%x, returned_fence_id:0x%x, fence in mem:0x%x\n", (unsigned int)sch_mgr->last_send_fence_id,
            (unsigned int)sch_mgr->returned_fence_id, (unsigned int)last_returned_fence_id);
        gf_info("current rb  cmd(0x%x): 0x%08x, rb head(0x%x): 0x%08x, rb tail(0x%x): 0x%08x\n",
            cur_rb_cmd_offset, cur_rb_cmd, cur_rb_head_offset, cur_rb_head, cur_rb_tail_offset, cur_rb_tail);
        gf_info("current L1 dma cmd(%x):%x, L2 cmd(%x):%x\n", cur_l1_dma_cmd_offset, cur_l1_dma_cmd, cur_l2_dma_cmd_offset, cur_l2_dma_cmd);
    }

    /* dump to file */
    if(adapter->ctl_flags.dump_hang_info_to_file)
    {
        util_dump_memory_to_file(&engine_status_offset,          4, "Engine status offset: ",           HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&engine_status,                28, "Engine status: ",                  HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&engine_index,                  4, "Engine index: ",                   HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&last_returned_fence_id,        4, "current returned fence: ",         HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&sch_mgr->last_send_fence_id,   4, "last send fence: ",                HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&cur_rb_cmd_offset,             4, "current rb cmd offset: ",          HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&cur_rb_cmd,                    4, "current rb cmd: ",                 HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&cur_rb_head_offset,            4, "current rb head offset: ",         HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&cur_rb_head,                   4, "current rb head: ",                HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&cur_rb_tail_offset,            4, "current rb tail offset: ",         HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&cur_rb_tail,                   4, "current rb tail: ",                HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&cur_l1_dma_cmd_offset,         4, "current l1 dma cmd offset: ",      HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&cur_l2_dma_cmd_offset,         4, "current l2 dma cmd offset: ",      HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&cur_l1_dma_cmd,                4, "current l1 dma cmd: ",             HW_HANG_LOG_FILE);
        util_dump_memory_to_file(&cur_l2_dma_cmd,                4, "current l2 dma cmd: ",             HW_HANG_LOG_FILE);
        util_dump_memory_to_file(engine->ring_buf_virt_addr, RING_BUFFER_SIZE_E3K, "RING buffer", HW_HANG_LOG_FILE);
    }

    //gf_info("<<----^^^^^^^^^-----[HANG DETECTED] Current HW Hang Status-----^^^^^^^^^^^--->>\n");
    gf_info("ChipSliceMask: %08x\r\n", adapter->hw_caps.chip_slice_mask);
    gf_info("Engine timeout and Status: %08x,%08x,%08x,%08x,%08x,%08x,%08x,VCP0 = %x,VCP1 = %x,VPP = %x.\n",
        engine_status->Top.uint, engine_status->Gpc0_0.uint, engine_status->Gpc0_1.uint,
        engine_status->Gpc1_0.uint, engine_status->Gpc1_1.uint, engine_status->Gpc2_0.uint, engine_status->Gpc2_1.uint,
        ((*video_engine_status)&0x1),
        ((*video_engine_status)&0x2),
        ((*video_engine_status)&0x4));

    /*dump debug bus*/
    if(adapter->display_debugbus_flag)
    {
        vidsch_display_debugbus_info_e3k(adapter, NULL, (engine_index >= RB_INDEX_VCP0));
    }
}

void vidschi_map_e3k(adapter_t *adapter)
{
    engine_share_e3k_t  *share = adapter->private_data;
    gf_map_argu_t      map = {0};

    if(share->ring_buffer_for_hang->vma == NULL)
    {
        map.size             = E3K_RINGBUFFER_FOR_HANG_SIZE;
        map.flags.mem_space  = GF_MEM_KERNEL;
        map.flags.read_only  = 0;
        map.flags.write_only = 0;
        map.flags.cache_type = GF_MEM_WRITE_COMBINED;
        map.flags.mem_type   = GF_SYSTEM_IO;
        map.phys_addr         = adapter->vidmm_bus_addr + share->ring_buffer_for_hang->gpu_virt_addr;
        map.offset            = 0;

        share->ring_buffer_for_hang->vma = gf_map_io_memory(NULL, &map);

        gf_info("ring_buffer_for_hang->vma->virt_addr 0x%x, size 0x%x \n", share->ring_buffer_for_hang->vma->virt_addr, map.size);
        gf_memset(share->ring_buffer_for_hang->vma->virt_addr, 0, share->ring_buffer_for_hang->vma->size);
    }

    if(share->context_buffer_for_hang->vma == NULL)
    {
        map.size             = E3K_CONTEXTBUFFER_FOR_HANG_SIZE;
        map.flags.mem_space  = GF_MEM_KERNEL;
        map.flags.read_only  = 0;
        map.flags.write_only = 0;
        map.flags.cache_type = GF_MEM_WRITE_COMBINED;
        map.flags.mem_type   = GF_SYSTEM_IO;
        map.phys_addr         = adapter->vidmm_bus_addr + share->context_buffer_for_hang->gpu_virt_addr;
        map.offset            = 0;

        share->context_buffer_for_hang->vma = gf_map_io_memory(NULL, &map);
        gf_info("context_buffer_for_hang->vma->virt_addr 0x%x size 0x%x \n", share->context_buffer_for_hang->vma->virt_addr, map.size);
        gf_memset(share->context_buffer_for_hang->vma->virt_addr, 0, share->context_buffer_for_hang->vma->size);
    }

    if(share->dma_buffer_for_hang->vma == NULL)
    {
        map.size             = E3K_DMABUFFER_FOR_HANG_SIZE;
        map.flags.mem_space  = GF_MEM_KERNEL;
        map.flags.read_only  = 0;
        map.flags.write_only = 0;
        map.flags.cache_type = GF_MEM_WRITE_COMBINED;
        map.flags.mem_type   = GF_SYSTEM_IO;
        map.phys_addr         = adapter->vidmm_bus_addr + share->dma_buffer_for_hang->gpu_virt_addr;
        map.offset            = 0;

        share->dma_buffer_for_hang->vma = gf_map_io_memory(NULL, &map);
        gf_info("dma_buffer_for_hang->vma->virt_addr 0x%x, size 0x%x \n", share->dma_buffer_for_hang->vma->virt_addr, map.size);
        gf_memset(share->dma_buffer_for_hang->vma->virt_addr, 0, share->dma_buffer_for_hang->vma->size);
    }

    if(share->transfer_buffer_for_hang->vma == NULL)
    {
        vidmm_map_flags_t flags = {0};

        gf_memset(&flags, 0, sizeof(flags));
        flags.write_only = 1;
        flags.mem_space  = GF_MEM_KERNEL;

        vidmm_map_segment_memory(adapter, NULL, share->transfer_buffer_for_hang, &flags);

        if(share->transfer_buffer_for_hang->vma != NULL)
        {
            gf_memset(share->transfer_buffer_for_hang->vma->virt_addr, 0, E3K_TRANSFERBUFFER_FOR_HANG_SIZE);
        }
    }
}

static int vidschi_get_dump_data_gpu_offset_e3k(adapter_t *adapter, unsigned int type, unsigned int index)
{
    engine_share_e3k_t *share    = adapter->private_data;
    vidmm_segment_memory_t *dma_buffer_for_hang     = share->dma_buffer_for_hang;
    vidmm_segment_memory_t *context_buffer_for_hang = share->context_buffer_for_hang;
    vidmm_segment_memory_t *ring_buffer_for_hang    = share->ring_buffer_for_hang;

    unsigned int   dma_buffer_block_size       = 64 * 1024;
    unsigned int   context_buffer_block_size   = HW_CONTEXT_SIZE_E3K;
    unsigned int   ring_buffer_block_size      = sizeof(RINGBUFFER_COMMANDS_E3K) + sizeof(dh_rb_info_e3k);
    unsigned int   buffer_gpu_offset           = 0;

    index = index % MAX_HANG_DUMP_DATA_POOL_NUMBER;

    switch (type)
    {
    case HANG_DUMP_CONTEXT_BUFFER_OFFSET:
        buffer_gpu_offset = context_buffer_for_hang->gpu_virt_addr + index * context_buffer_block_size;
        break;
    case HANG_DUMP_DMA_BUFFER_OFFSET:
        buffer_gpu_offset = dma_buffer_for_hang->gpu_virt_addr + index * dma_buffer_block_size;
        break;
    case HANG_DUMP_RING_BUFFER_OFFSET:
        buffer_gpu_offset = ring_buffer_for_hang->gpu_virt_addr + index * ring_buffer_block_size;
        break;
    case HANG_DUMP_DH_COMMON_INFO_OFFSET:
        buffer_gpu_offset = ring_buffer_for_hang->gpu_virt_addr + MAX_HANG_DUMP_DATA_POOL_NUMBER * ring_buffer_block_size;
        break;
    default:
        break;
    }
    return buffer_gpu_offset;
}

unsigned char *vidschi_get_dump_data_cpu_virt_addr_e3k(adapter_t *adapter, unsigned int type, unsigned int index)
{
    engine_share_e3k_t *share    = adapter->private_data;
    vidmm_segment_memory_t *dma_buffer_for_hang     = share->dma_buffer_for_hang;
    vidmm_segment_memory_t *context_buffer_for_hang = share->context_buffer_for_hang;
    vidmm_segment_memory_t *ring_buffer_for_hang    = share->ring_buffer_for_hang;
    unsigned int   dma_buffer_block_size       = 64 * 1024;
    unsigned int   context_buffer_block_size   = HW_CONTEXT_SIZE_E3K;
    unsigned int   ring_buffer_block_size      = sizeof(RINGBUFFER_COMMANDS_E3K) + sizeof(dh_rb_info_e3k);
    unsigned char  *virt_addr                  = NULL;

    index = index % MAX_HANG_DUMP_DATA_POOL_NUMBER;

    switch (type)
    {
    case HANG_DUMP_CONTEXT_BUFFER_OFFSET:
        virt_addr = context_buffer_for_hang->vma->virt_addr + index * context_buffer_block_size;
        break;
    case HANG_DUMP_DMA_BUFFER_OFFSET:
        virt_addr = dma_buffer_for_hang->vma->virt_addr + index * dma_buffer_block_size;
        break;
    case HANG_DUMP_RING_BUFFER_OFFSET:
        virt_addr = ring_buffer_for_hang->vma->virt_addr + index * ring_buffer_block_size;
        break;
    case HANG_DUMP_DH_COMMON_INFO_OFFSET:
        virt_addr = ring_buffer_for_hang->vma->virt_addr + MAX_HANG_DUMP_DATA_POOL_NUMBER * ring_buffer_block_size;
        break;
    default:
        break;
    }

    return virt_addr;
}

void vidschi_copy_mem_e3k(adapter_t *adapter, unsigned long long dst_phys_addr,void *dst_virt_addr,unsigned long long src_phys_addr, void *src_virt_addr,unsigned int total_size)
{
    unsigned int       RbIndex  = RB_INDEX_GFXL;
    vidsch_mgr_t       *sch_mgr = adapter->sch_mgr[RbIndex];
    engine_share_e3k_t *share   = adapter->private_data;
    engine_e3k_t       *engine  = sch_mgr->private_data;
    vidschedule_t      *schedule = adapter->schedule;

    Hw_Surf_Format              Format  = HSF_R8G8B8A8_UNORM;
    DWORD                       Width   = MAX_WIDTH_2DBLT;
    DWORD                       Height  = 0;
    DWORD                       Length  = total_size;
    unsigned long long          oldFence= 0;
    DWORD                       i       = 0;
    DWORD                       StartX  = 0;
    DWORD                       CmdLength;
    DWORD                       AddrOffset = 0;
    static int                  bHwCopyEn  = TRUE;
    unsigned long long          Count   = 0;
    unsigned long long          DstAddr = dst_phys_addr;
    unsigned long long          SrcAddr = src_phys_addr;


    Cmd_Blk_Cmd_Csp_Indicator           pwmTrigger = {0};
    Cmd_Blk_Cmd_Csp_Indicator_Dword1    trigger_Dw = {0};
    BITBLT_REGSETTING_E3K       CopyCmd   = {0};
    BITBLT_REGSETTING_E3K*      p2DBltCmd = &CopyCmd;
    DWORD*                      pRB;
    DWORD*                      pRB0;

    gf_assert(total_size % 4 == 0, NULL);

    // SrcAddr and DstAddr should be aligned to 2kbit.
    gf_assert((SrcAddr & ~0xFFll) == 0, NULL);
    gf_assert((DstAddr & ~0xFFll) == 0, NULL);

    gf_memcpy(p2DBltCmd, &share->_2dblt_cmd_e3k, sizeof(BITBLT_REGSETTING_E3K));
    p2DBltCmd->BitBltCmd.Blt_Overlap = 1;

    if(!adapter->hw_caps.local_only)
    {
        enginei_invalidate_gart_e3k(engine);
    }

    //current not consider seperate engine.
    if(schedule->hw_hang)
    {
        return;
    }

    // HW copy
    if (bHwCopyEn)
    {
        pwmTrigger.Major_Opcode         = CSP_OPCODE_Blk_Cmd_Csp_Indicator;
        pwmTrigger.Block_Id             = CSP_GLOBAL_BLOCK;
        pwmTrigger.Type                 = BLOCK_COMMAND_CSP_TYPE_INDICATOR;
        pwmTrigger.Info                 = BLK_CMD_CSP_INDICATOR_INFO_3D_MODE;
        pwmTrigger.Dwc                  = 1;
        trigger_Dw.Slice_Mask           = adapter->hw_caps.chip_slice_mask;

        // save fence need more check
        oldFence = *(unsigned long long*)(engine->fence_buffer_virt_addr + DH_FENCE_OFFSET);
        *(engine->fence_buffer_virt_addr + DH_FENCE_OFFSET) = 0;

        while(Length && !schedule->hw_hang)
        {
            Width = MAX_WIDTH_2DBLT;
            Height = Length/(Width << 2);
            Height = Height > MAX_HEIGHT_2DBLT ? MAX_HEIGHT_2DBLT : Height;

            if(Length < (MAX_WIDTH_2DBLT << 2))
            {
                Width  = Length >> 2;
                Height = 1;
            }

            StartX     = AddrOffset & 0xFF;
            AddrOffset = AddrOffset - StartX;

            p2DBltCmd->SrcAddr.reg.Base_Addr = (SrcAddr + AddrOffset) >> 8;
            p2DBltCmd->DstAddr.reg.Base_Addr = (DstAddr + AddrOffset) >> 8;

            p2DBltCmd->SrcFormat.reg.Format         = Format;
            p2DBltCmd->SrcFormat.reg.Bl_Slot_Idx    = 0;
            p2DBltCmd->SrcSize.reg.Width            = Width;
            p2DBltCmd->SrcSize.reg.Height           = Height;
            p2DBltCmd->SrcDepth.reg.Depth_Or_Arraysize = 1;
            p2DBltCmd->SrcDepth.reg.Range_Type      = 0;
            p2DBltCmd->SrcViewCtrl.reg.Arraysize    = 1;
            p2DBltCmd->SrcViewCtrl.reg.First_Array_Idx = 0;

            p2DBltCmd->DstFormat    = p2DBltCmd->SrcFormat;
            p2DBltCmd->DstSize      = p2DBltCmd->SrcSize;
            p2DBltCmd->DstDepth     = p2DBltCmd->SrcDepth;
            p2DBltCmd->DstViewCtrl  = p2DBltCmd->SrcViewCtrl;

            p2DBltCmd->SrcMisc.reg.Is_Tiling        = 0;
            p2DBltCmd->SrcMisc.reg.Rt_Enable        = 1;
            p2DBltCmd->SrcMisc.reg.Resource_Type    = RT_MISC_RESOURCE_TYPE_2D_TEXTURE;

            p2DBltCmd->DstMisc.reg.Is_Tiling        = 0;
            p2DBltCmd->DstMisc.reg.Rt_Enable        = 1;
            p2DBltCmd->DstMisc.reg.Rt_Write_Mask    = 0xF;
            p2DBltCmd->DstMisc.reg.Resource_Type    = RT_MISC_RESOURCE_TYPE_2D_TEXTURE;
            p2DBltCmd->DstMisc.reg.Eu_Blend_Enable  = 0;

            p2DBltCmd->Rast_Ctrl.reg.Eub_En         = 0;
            p2DBltCmd->reg_Iu_Ctrl_Ex.reg.Eu_Blending_En = 0;
            p2DBltCmd->SrcMisc.reg.Eu_Blend_Enable  = 0;

            p2DBltCmd->Dzs_Ctrl.reg.Src_Read_Alloc_En = 0;

            p2DBltCmd->RectX.Xmin = StartX >> 2;//in pixel
            p2DBltCmd->RectX.Xmax = Width - 1;
            p2DBltCmd->RectY.Ymin = 0;
            p2DBltCmd->RectY.Ymax = Height - 1;
            p2DBltCmd->SrcDxDy.Dx = 0;
            p2DBltCmd->SrcDxDy.Dy = 0;

            p2DBltCmd->D_Flush.Type                  = BLOCK_COMMAND_TEMPLATE_TYPE_FLUSH;
            p2DBltCmd->D_Flush.Target                = BLOCK_COMMAND_FLUSH_TARGET_D_C;
            p2DBltCmd->D_Flush.Block_Id              = FF_BLOCK;
            p2DBltCmd->D_Flush.Major_Opcode          = CSP_OPCODE_Block_Command_Flush;

            p2DBltCmd->D_Invalidate.Type             = BLOCK_COMMAND_TEMPLATE_TYPE_INVALIDATE_CACHE;
            p2DBltCmd->D_Invalidate.Target           = BLOCK_COMMAND_FLUSH_TARGET_D_C;
            p2DBltCmd->D_Invalidate.Block_Id         = FF_BLOCK;
            p2DBltCmd->D_Invalidate.Major_Opcode     = CSP_OPCODE_Block_Command_Flush;

            p2DBltCmd->FenceCmd.uint = SEND_INTERNAL_WRITEFENCE_E3K(FENCE_ROUTE_ID_D_FENCE,_RBT_3DBE, HWM_SYNC_KMD_SLOT);
            p2DBltCmd->FenceValue = SEND_INTERNAL_FENCE_VALUE_E3K(FENCE_VALUE_CACHE_DMA_DFENCE);
            p2DBltCmd->WaitCmd.uint = SEND_INTERNAL_WAIT_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_DFENCE);
            p2DBltCmd->WaitMainCmd.uint = SEND_INTERNAL_WAIT_MAIN_E3K(HWM_SYNC_KMD_SLOT, WAIT_METHOD_EQUAL, FENCE_VALUE_CACHE_DMA_DFENCE);

            CmdLength = sizeof(BITBLT_REGSETTING_E3K)/sizeof(DWORD);
            CmdLength = (((CmdLength + 5 + 4 + 8) + 15) & ~15);

            pRB = enginei_get_ring_buffer_space_e3k(engine, CmdLength);
            pRB0 = pRB;

            gf_memset(pRB, 0, CmdLength << 2);

            *pRB++ = *(DWORD*)&pwmTrigger;
            *pRB++ = *(DWORD*)&trigger_Dw;
            pRB += 8;  // skip 8dw for hw block sync

            gf_memcpy(pRB, p2DBltCmd, sizeof(BITBLT_REGSETTING_E3K));
            pRB =pRB + sizeof(BITBLT_REGSETTING_E3K)/sizeof(DWORD);

            *pRB++ = SEND_EXTERNAL_FENCE_E3K(FENCE_IRQ_INTERRUPT_CPU);
            *pRB++ = (engine->fence_buffer_phy_addr + DH_FENCE_OFFSET * sizeof(unsigned long long)) & 0xFFFFFFFF;
            *pRB++ = ((engine->fence_buffer_phy_addr + DH_FENCE_OFFSET * sizeof(unsigned long long)) >> 32 )& 0xFF;

            *pRB++ = (DH_FENCE_VALUE + i)&0xFFFFFFFF;
            *pRB++ = (((unsigned long long)(DH_FENCE_VALUE + i)) >>32) &0xFFFFFFFF;

            *pRB++ = *(DWORD*)&pwmTrigger;
            *pRB++ = *(DWORD*)&trigger_Dw;

            //to ensure cmd write in mem
            if(adapter->ctl_flags.run_on_qt)
            {
                util_crc32((unsigned char*)pRB0, CmdLength << 2);
            }

            sch_mgr->hang_hw_copy_mem = 1;
            enginei_do_kickoff_cmd_e3k(engine);

            while((*(unsigned long long*)(engine->fence_buffer_virt_addr + DH_FENCE_OFFSET) + 2)<= (DH_FENCE_VALUE + i))//max kick 2 dma to hw
            {
                if(schedule->hw_hang)
                {
                    sch_mgr->hang_hw_copy_mem = 0;
                    return;
                }
                else if(++Count < 0x20)
                {
                     gf_msleep(10);
                }
                else
                {
                    gf_info("%s, i:%d, fence not back, expect:%llx, fence in mem:%llx, hw queue dma num:%d\n",util_remove_name_suffix(__func__), i, (DH_FENCE_VALUE + i), *(unsigned long long*)(engine->fence_buffer_virt_addr + DH_FENCE_OFFSET), 2);
                    Count = 0;
                    sch_mgr->hang_hw_copy_mem = 0;
                    return;
                }
            }

            sch_mgr->hang_hw_copy_mem = 0;
            i++;

            AddrOffset +=((Width * Height) << 2);
            Length -=((Width * Height) << 2) + StartX;
        }

        Count = 0;

        while(*(unsigned long long*)(engine->fence_buffer_virt_addr + DH_FENCE_OFFSET) != (DH_FENCE_VALUE + i -1))
        {
            if(schedule->hw_hang)
            {
                return;
            }
            else if(++Count < 0x20)
            {
                 gf_msleep(10);
            }
            else
            {
                gf_info("%s, i:%d, fence not back, expect:%llx, fence in mem:%llx, out of loop\n",util_remove_name_suffix(__func__), i, (DH_FENCE_VALUE + i), *(unsigned long long*)(engine->fence_buffer_virt_addr + DH_FENCE_OFFSET));
                Count = 0;
                return;
            }
        }

        // restore fence
        *(unsigned long long*)(engine->fence_buffer_virt_addr + DH_FENCE_OFFSET) = oldFence;
    }
    else if(src_virt_addr !=NULL && dst_virt_addr != NULL)
    {
        gf_memcpy(dst_virt_addr, src_virt_addr, total_size);
    }
    else
    {
        gf_error("hw copy not enable, VA is null\n");
        gf_assert(0, NULL);
    }
}

void vidsch_duplicate_hang_e3k(adapter_t *adapter)
{
    engine_share_e3k_t *share  = adapter->private_data;
    vidsch_mgr_t       *sch_mgr = NULL;
    engine_e3k_t       *engine = NULL;

    dh_file_info_e3k   dh_file_info    = {0};
    dh_common_info_e3k *dh_common_info = NULL;
    dh_rb_info_e3k   *dh_rb_info = NULL;
    unsigned int                    *rb             = NULL;
    unsigned char                   *ring_buffer    = NULL;
    RINGBUFFER_COMMANDS_E3K         *submit_cmd     = NULL;

    EngineSatus_e3k status_save = {0}, status_run = {0};
    unsigned int *status = NULL;
    unsigned long long  sent_counter = 0, returned_counter = 0;
    unsigned long long  *pReturnCounter = NULL;
    gf_vm_area_t *fb_vma = NULL;
    gf_map_argu_t map = {0};
    unsigned long long Dst_PA = 0;
    unsigned long long Src_PA = 0;
    int i = 0;
    long long length = 0;
    int len = 0;

    struct os_file *file = NULL;
    unsigned long long  pos = 0;

    int  pool_index = 0;
    unsigned int rb_index = 0;
    unsigned long long dma_buffer_pa = 0;
    unsigned long long context_buffer_pa = 0;
    void  *dma_buffer_va = NULL;
    void  *context_buffer_va = NULL;
    RINGBUFFER_COMMANDS_E3K  *rb_va = NULL;

    unsigned long long      start_time = 0, current_time, delta_time;
    long                    temp_sec, temp_usec;
    unsigned long long batch_size = 0;

#if COMPATIBLE_TO_WINDOW
    vidsch_duplicate_hang_compatible_e3k(adapter);
    return;
#endif

    if(!debug_mode_e3k.duplicate_hang)
    {
        return;
    }
    debug_mode_e3k.duplicate_hang = 0;

    file = gf_file_open("/var/fb.FB", OS_RDWR | OS_LARGEFILE, 0666);
    if(!file)
    {
        gf_error("vidsch_duplicate_hang_e3k open file /var/fb.FB fail.\n");
        return;
    }

    //1. load file info in file header
    len = gf_file_read(file, &dh_file_info, sizeof(dh_file_info_e3k), &pos);
    READ_DUMP_FILE_CHECK(sizeof(dh_file_info_e3k), len);
    gf_assert(share->dma_buffer_for_hang->gpu_virt_addr >= dh_file_info.FbCpuVisibleSize, NULL);
    gf_info("vidsch_duplicate_hang_e3k visible_size:0x%llx, invisible_size:0x%llx, dma_size:0x%llx, context_size:0x%llx, rb_size:0x%llx\n", dh_file_info.FbCpuVisibleSize, dh_file_info.FbNotCpuVisibleSize.quad64, dh_file_info.dma_for_hang_size, dh_file_info.context_for_hang_size, dh_file_info.ring_for_hang_size);

    //2. load dma buffer
    len = gf_file_read(file, share->dma_buffer_for_hang->vma->virt_addr, E3K_DMABUFFER_FOR_HANG_SIZE, &pos);
    READ_DUMP_FILE_CHECK(E3K_DMABUFFER_FOR_HANG_SIZE, len);

    //3. load context buffer
    len = gf_file_read(file, share->context_buffer_for_hang->vma->virt_addr, E3K_CONTEXTBUFFER_FOR_HANG_SIZE, &pos);
    READ_DUMP_FILE_CHECK(E3K_CONTEXTBUFFER_FOR_HANG_SIZE, len);

    //4. load ring buffer
    len = gf_file_read(file, share->ring_buffer_for_hang->vma->virt_addr, E3K_RINGBUFFER_FOR_HANG_SIZE, &pos);
    READ_DUMP_FILE_CHECK(E3K_RINGBUFFER_FOR_HANG_SIZE, len);

    //5 load cpu invisible segment mem
    gf_info("vidsch_duplicate_hang_e3k start load cpu invisible memory...\n");
    length = dh_file_info.FbNotCpuVisibleSize.quad64;
    Src_PA = share->transfer_buffer_for_hang->gpu_virt_addr;
    Dst_PA = adapter->Visible_vram_size;
    batch_size = (adapter->ctl_flags.run_on_qt)?(E3K_TRANSFERBUFFER_FOR_HANG_SIZE>>8) : E3K_TRANSFERBUFFER_FOR_HANG_SIZE;
    while(length > 0)
    {
        unsigned long long copy_size = (length > batch_size)?batch_size : length;
        len = gf_file_read(file, share->transfer_buffer_for_hang->vma->virt_addr, copy_size, &pos);
        READ_DUMP_FILE_CHECK(copy_size, len);
        vidschi_copy_mem_e3k(adapter,Dst_PA,NULL,Src_PA,NULL,copy_size);
        Dst_PA += copy_size;
        length -= copy_size;

        gf_info("vidsch_duplicate_hang_e3k left cpu invisible segment mem:0x%llx\n", length);
    }
    gf_info("vidsch_duplicate_hang_e3k end load cpu invisible memory...\n");

    //1.6 load cpu visible segment mem
    gf_info("vidsch_duplicate_hang_e3k start load cpu visible memory...\n");
    length = dh_file_info.FbCpuVisibleSize;
    while(length > 0)
    {
       map.size             = (length >= MIU_DYNAMIC_FB_ENTRY_SIZE) ? MIU_DYNAMIC_FB_ENTRY_SIZE : length;
       map.flags.mem_space  = GF_MEM_KERNEL;
       map.flags.read_only  = 0;
       map.flags.write_only = 0;
       map.flags.mem_type   = GF_SYSTEM_IO;
       map.flags.cache_type = GF_MEM_UNCACHED;
       map.phys_addr    = adapter->vidmm_bus_addr + i*MIU_DYNAMIC_FB_ENTRY_SIZE;

       fb_vma = gf_map_io_memory(NULL, &map);
       gf_assert(fb_vma != NULL, NULL);

       gf_file_read(file, fb_vma->virt_addr, map.size, &pos);
       gf_unmap_io_memory(fb_vma);
       fb_vma = NULL;

       i++;

       length -= map.size;
       gf_info("vidsch_duplicate_hang_e3k left cpu visible segment mem:%llx\n", length);
    }
    gf_info("vidsch_duplicate_hang_e3k end load cpu visible memory...\n");
    gf_file_close(file);

    dh_common_info = (dh_common_info_e3k*)(vidschi_get_dump_data_cpu_virt_addr_e3k(adapter, HANG_DUMP_DH_COMMON_INFO_OFFSET, 0));
    pool_index = dh_common_info->hang_dma_index;
    gf_info("vidsch_duplicate_hang_e3k hang_dma_fence_id:%d, hang_dma_index:%d\n", dh_common_info->hang_dma_fence_id, dh_common_info->hang_dma_index);

    dma_buffer_pa     = vidschi_get_dump_data_gpu_offset_e3k(adapter, HANG_DUMP_DMA_BUFFER_OFFSET, pool_index);
    dma_buffer_va     = vidschi_get_dump_data_cpu_virt_addr_e3k(adapter, HANG_DUMP_DMA_BUFFER_OFFSET, pool_index);
    context_buffer_pa = vidschi_get_dump_data_gpu_offset_e3k(adapter, HANG_DUMP_CONTEXT_BUFFER_OFFSET, pool_index);
    context_buffer_va = vidschi_get_dump_data_cpu_virt_addr_e3k(adapter, HANG_DUMP_CONTEXT_BUFFER_OFFSET, pool_index);
    ring_buffer       = vidschi_get_dump_data_cpu_virt_addr_e3k(adapter, HANG_DUMP_RING_BUFFER_OFFSET, pool_index);
    dh_rb_info   = (dh_rb_info_e3k*)ring_buffer;
    ring_buffer += sizeof(dh_rb_info_e3k);
    rb_va = (RINGBUFFER_COMMANDS_E3K*)ring_buffer;
    util_dump_memory(NULL, rb_va, sizeof(RINGBUFFER_COMMANDS_E3K), "vidsch_duplicate_hang_e3k ring buffer");
    gf_info("vidsch_duplicate_hang_e3k slice mask:0x%x\n", rb_va->c1.trigger_Dw.Slice_Mask);
    util_dump_memory(NULL, dma_buffer_va, 64*1024, "vidsch_duplicate_hang_e3k dma buffer");

    sch_mgr     = adapter->sch_mgr[dh_rb_info->last_rb_index];
    engine      = sch_mgr->private_data;
    rb_index        = dh_rb_info->last_rb_index;
    status_save   = dh_rb_info->status;
    length          = dh_rb_info->last_rb_size >>2;

    sent_counter    = DH_FENCE_VALUE;
    pReturnCounter  = engine->fence_buffer_virt_addr + DH_FENCE_OFFSET;
    *pReturnCounter = DH_FENCE_VALUE;
    returned_counter = *pReturnCounter;

    status = (unsigned int *)&status_run;
    for(i = 0; i < 7; i++)
    {
        *(status + i) = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + (Reg_Block_Busy_Bits_Top_Offset + i)*4);
    }
    gf_info("vidsch_duplicate_hang_e3k before do kickoff status: %08x,%08x,%08x,%08x,%08x,%08x,%08x\n",status_run.Top.uint,status_run.Gpc0_0.uint,status_run.Gpc0_1.uint,status_run.Gpc1_0.uint,status_run.Gpc1_1.uint,status_run.Gpc2_0.uint,status_run.Gpc2_1.uint);

    switch(rb_index)
    {
        case RB_INDEX_GFXL:
        {
            rb = enginei_get_ring_buffer_space_e3k(engine, length);
            gf_memcpy((void*)rb, (void*)ring_buffer, length << 2 );
            submit_cmd = (RINGBUFFER_COMMANDS_E3K*)rb;
            //patch restore dma address
            if(submit_cmd->c0.RestoreContext_Address != 0)
            {
                submit_cmd->c0.RestoreContext_Address = (context_buffer_pa >> 11);
            }

            //patch dma adress
            submit_cmd->c0.CommandDMA_Address_L       = (dma_buffer_pa & 0xFFFFFFFF);
            submit_cmd->c0.CommandDMA_Address_H       = ((dma_buffer_pa >>32) & 0xFF);

            //patch save dma address.
            //Assume the hang task dma not the first dma, since the context_buffer_pa for save will overwrite the template.
            gf_memset(&submit_cmd->c0.SaveDMA, 0, 4*sizeof(DWORD));
            sent_counter++;
            *(unsigned int*)&submit_cmd->c0.Fence = SEND_EXTERNAL_FENCE_E3K(FENCE_IRQ_INTERRUPT_CPU);
            submit_cmd->c0.Fence_Data_L = (sent_counter) & 0xFFFFFFFF;
            submit_cmd->c0.Fence_Data_H = ((sent_counter) >>32) & 0xFFFFFFFF;
            submit_cmd->c0.Fence_Address_L = (engine->fence_buffer_phy_addr + DH_FENCE_OFFSET * sizeof(unsigned long long)) & 0xFFFFFFFF;
            submit_cmd->c0.Fence_Address_H = ((engine->fence_buffer_phy_addr + DH_FENCE_OFFSET * sizeof(unsigned long long)) >> 32) & 0xFFFFFFFF;

            enginei_do_kickoff_cmd_e3k(engine);
            break;
        }
        case RB_INDEX_GFXH:
            gf_error("vidsch_duplicate_hang_e3k RB_INDEX_GFXH not supported...\n");
            break;
    }

    //wait for hang
    gf_getsecs(&temp_sec, &temp_usec);
    start_time = temp_sec;
    for(;;)
    {
        gf_getsecs(&temp_sec, &temp_usec);
        current_time = temp_sec;
        delta_time   = current_time - start_time;

        /* wait 10s, if fence still not back seems something wrong */
        if(delta_time > DUPLICATE_TIME_OUT)
        {
            break;
        }
    }
    returned_counter = *pReturnCounter;
    gf_info("vidsch_duplicate_hang_e3k after wait sent_counter=0x%llx, return_counter=0x%llx\n", sent_counter, returned_counter);

    status = (unsigned int *)&status_run;
    for( i = 0; i <7; i++)
    {
        *(status + i) = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + (Reg_Block_Busy_Bits_Top_Offset + i)*4);
    }

    gf_info("vidsch_duplicate_hang_e3k after do kickoff status: %08x,%08x,%08x,%08x,%08x,%08x,%08x\n",status_run.Top.uint,status_run.Gpc0_0.uint,status_run.Gpc0_1.uint,status_run.Gpc1_0.uint,status_run.Gpc1_1.uint,status_run.Gpc2_0.uint,status_run.Gpc2_1.uint);
    gf_info("vidsch_duplicate_hang_e3k hang dump save status: %08x,%08x,%08x,%08x,%08x,%08x,%08x\n",status_save.Top.uint,status_save.Gpc0_0.uint,status_save.Gpc0_1.uint,status_save.Gpc1_0.uint,status_save.Gpc1_1.uint,status_save.Gpc2_0.uint,status_save.Gpc2_1.uint);

    vidsch_display_debugbus_info_e3k(adapter, NULL, FALSE);

    if((status_save.Top.uint & 0xFFFFFFF9) == (status_run.Top.uint & 0xFFFFFFF9) && //exclude mxua/b, diu will use it, even 3d hang.
        (status_save.Gpc0_0.uint == status_run.Gpc0_0.uint) &&
        (status_save.Gpc0_1.uint == status_run.Gpc0_1.uint) &&
        (status_save.Gpc1_0.uint == status_run.Gpc1_0.uint) &&
        (status_save.Gpc1_1.uint == status_run.Gpc1_1.uint) &&
        (status_save.Gpc2_0.uint == status_run.Gpc2_0.uint) &&
        (status_save.Gpc2_1.uint == status_run.Gpc2_1.uint))
    {
        if (status_save.Top.uint == 0 && status_run.Top.uint == 0)
        {
            gf_info("vidsch_duplicate_hang_e3k <<-------------------:) not duplicate hang, Status is same but not hang------------------------>>.\n");
            gf_assert(0, "vidsch_duplicate_hang_e3k <<-------------------:( Do more check !! ------------------------>>\n");
        }
        else
        {
            gf_info("vidsch_duplicate_hang_e3k <<-------------------:) Good job!, Status is same.------------------------>>.\n");
            gf_assert(0, "vidsch_duplicate_hang_e3k <<-------------------:) Congratulation!! ------------------------>>\n");
        }
    }
    else
    {
        gf_info("vidsch_duplicate_hang_e3k <<------------------:( Sorry, Status is different.-------------------------->>.\n");
        gf_assert(0, "vidsch_duplicate_hang_e3k <<-------------------:( Do more check !! ------------------------>>\n");
    }
}

void vidsch_dump_hang_e3k(adapter_t *adapter)
{
    engine_share_e3k_t *share           = adapter->private_data;

    vidmm_segment_memory_t *dma_buffer_for_hang     = share->dma_buffer_for_hang;
    vidmm_segment_memory_t *context_buffer_for_hang = share->context_buffer_for_hang;
    vidmm_segment_memory_t *ring_buffer_for_hang    = share->ring_buffer_for_hang;

    struct os_file  *file = NULL;
    long long length = 0;
    dh_file_info_e3k dh_file_info;
    unsigned long long Dst_PA = 0;
    unsigned long long Src_PA = 0;

    int len = 0;
    EngineSatus_e3k current_status = {0};
    unsigned int *status = NULL;
    int i = 0;

    dh_common_info_e3k *dh_common_info = NULL;

#if COMPATIBLE_TO_WINDOW
    vidsch_dump_hang_compatible_e3k(adapter);

    return;
#endif

    if(!debug_mode_e3k.post_hang_dump)
    {
        gf_error("vidsch_dump_hang_e3k skip this function, status error...\n");
        return;
    }

    debug_mode_e3k.internal_dump_hw = 1;

    if(!file)
    {
        file = gf_file_open("/var/fb.FB", OS_RDWR | OS_CREAT | OS_LARGEFILE, 0666);
        if(!file)
        {
            gf_error("vidsch_dump_hang_e3k create file: create file /var/fb.FB fail!\n");
            return;
        }
    }

    dh_file_info.FbCpuVisibleSize = dma_buffer_for_hang->gpu_virt_addr;
    dh_file_info.FbNotCpuVisibleSize.quad64 = adapter->Real_vram_size - adapter->Visible_vram_size + adapter->gart_ram_size;
    dh_file_info.dma_for_hang_size     = E3K_DMABUFFER_FOR_HANG_SIZE;
    dh_file_info.context_for_hang_size = E3K_CONTEXTBUFFER_FOR_HANG_SIZE;
    dh_file_info.ring_for_hang_size    = E3K_RINGBUFFER_FOR_HANG_SIZE;
    gf_info("vidsch_dump_hang_e3k visible_size:0x%llx, invisible_size:0x%llx, dma_size:0x%llx, context_size:0x%llx, rb_size:0x%llx\n", dh_file_info.FbCpuVisibleSize, dh_file_info.FbNotCpuVisibleSize.quad64, dh_file_info.dma_for_hang_size, dh_file_info.context_for_hang_size, dh_file_info.ring_for_hang_size);

    //1. save file info
    len = gf_file_write(file, &dh_file_info, sizeof(dh_file_info_e3k));
    WRITE_DUMP_FILE_CHECK(sizeof(dh_file_info_e3k), len);

    status = (unsigned int *)&current_status;
    for(i = 0; i < 7; i++)
    {
        *(status + i) = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + (Reg_Block_Busy_Bits_Top_Offset + i)*4);
    }
    gf_info("vidsch_dump_hang_e3k current status: %08x,%08x,%08x,%08x,%08x,%08x,%08x\n",current_status.Top.uint,current_status.Gpc0_0.uint,current_status.Gpc0_1.uint,current_status.Gpc1_0.uint,current_status.Gpc1_1.uint,current_status.Gpc2_0.uint,current_status.Gpc2_1.uint);

    dh_common_info = (dh_common_info_e3k*)(vidschi_get_dump_data_cpu_virt_addr_e3k(adapter, HANG_DUMP_DH_COMMON_INFO_OFFSET, 0));
    gf_info("vidsch_dump_hang_e3k hang_dma_fence_id:%d, hang_dma_index:%d\n", dh_common_info->hang_dma_fence_id, dh_common_info->hang_dma_index);

    //2. save dma buffer
    len = gf_file_write(file, dma_buffer_for_hang->vma->virt_addr, E3K_DMABUFFER_FOR_HANG_SIZE);
    WRITE_DUMP_FILE_CHECK(E3K_DMABUFFER_FOR_HANG_SIZE, len);
    // util_dump_memory(NULL, dma_buffer_for_hang->vma->virt_addr + dh_common_info->hang_dma_index*dma_buffer_block_size,  dma_buffer_block_size, "vidsch_dump_hang_e3k dma buffer");

    //3. save context buffer
    len = gf_file_write(file, context_buffer_for_hang->vma->virt_addr, E3K_CONTEXTBUFFER_FOR_HANG_SIZE);
    WRITE_DUMP_FILE_CHECK(E3K_CONTEXTBUFFER_FOR_HANG_SIZE, len);
    // util_dump_memory(NULL, context_buffer_for_hang->vma->virt_addr+ dh_common_info->hang_dma_index*context_buffer_block_size,  context_buffer_block_size, "vidsch_dump_hang_e3k context buffer");

    //4. save ring buffer
    len = gf_file_write(file, ring_buffer_for_hang->vma->virt_addr, E3K_RINGBUFFER_FOR_HANG_SIZE);
    WRITE_DUMP_FILE_CHECK(E3K_RINGBUFFER_FOR_HANG_SIZE, len);
    // util_dump_memory(NULL, ring_buffer_for_hang->vma->virt_addr+ dh_common_info->hang_dma_index*ring_buffer_block_size,  ring_buffer_block_size, "vidsch_dump_hang_e3k ring buffer");

    //5. save cpu invisible segment mem.
    gf_info("vidsch_dump_hang_e3k start dump invisible memory...\n");
    length = dh_file_info.FbNotCpuVisibleSize.quad64;
    Dst_PA = share->transfer_buffer_for_hang->gpu_virt_addr;
    Src_PA = adapter->Visible_vram_size;
    while(length > adapter->gart_ram_size)
    {
        unsigned long long copy_size = (length > E3K_TRANSFERBUFFER_FOR_HANG_SIZE)?E3K_TRANSFERBUFFER_FOR_HANG_SIZE : length;

        vidschi_copy_mem_e3k(adapter, Dst_PA, NULL, Src_PA, NULL, copy_size);

        len = gf_file_write(file, share->transfer_buffer_for_hang->vma->virt_addr, copy_size);
        WRITE_DUMP_FILE_CHECK(copy_size, len);

        Src_PA += copy_size;
        length -= copy_size;

        gf_info("vidsch_dump_hang_e3k left cpu invisible segment mem:0x%llx\n", length - adapter->gart_ram_size);
    }

    //5.1 snoop/non-snoop segment mem.
    Src_PA = adapter->Real_vram_size;
    while(length > 0)
    {
        unsigned long long copy_size = (length > E3K_TRANSFERBUFFER_FOR_HANG_SIZE)?E3K_TRANSFERBUFFER_FOR_HANG_SIZE : length;

        vidschi_copy_mem_e3k(adapter,
                            Dst_PA,
                            NULL,
                            Src_PA,
                            NULL,
                            copy_size);

        len = gf_file_write(file, share->transfer_buffer_for_hang->vma->virt_addr, copy_size);
        WRITE_DUMP_FILE_CHECK(copy_size, len);

        Src_PA += copy_size;
        length -= copy_size;

        gf_info("vidsch_dump_hang_e3k left snoop/non-snoop segment mem:%llx\n", length);
    }

    gf_info("vidsch_dump_hang_e3k end dump invisible memory...\n");

    //6. save cpu visible segment memory.
    gf_info("vidsch_dump_hang_e3k start dump visible memory...\n");
    length = dh_file_info.FbCpuVisibleSize;
    Src_PA = 0;
    Dst_PA = share->transfer_buffer_for_hang->gpu_virt_addr;
    while(length > 0)
    {
        unsigned long long copy_size = (length > E3K_TRANSFERBUFFER_FOR_HANG_SIZE)?E3K_TRANSFERBUFFER_FOR_HANG_SIZE : length;

        vidschi_copy_mem_e3k(adapter,
                            Dst_PA,
                            NULL,
                            Src_PA,
                            NULL,
                            copy_size);

        len = gf_file_write(file, share->transfer_buffer_for_hang->vma->virt_addr, copy_size);
        WRITE_DUMP_FILE_CHECK(copy_size, len);

        Src_PA += copy_size;
        length -= copy_size;

        gf_info("vidsch_dump_hang_e3k left cpu visiable segment mem:%llx\n", length);
    }
#if 0
    while(length > 0)
    {
       map.size             = (length >= MIU_DYNAMIC_FB_ENTRY_SIZE) ? MIU_DYNAMIC_FB_ENTRY_SIZE : length;
       map.flags.mem_space  = GF_MEM_KERNEL;
       map.flags.cache_type = GF_MEM_UNCACHED;
       map.phys_addr        = adapter->vidmm_bus_addr + i*MIU_DYNAMIC_FB_ENTRY_SIZE;
       map.flags.mem_type   = GF_SYSTEM_IO;
       fb_vma = gf_map_io_memory(NULL, &map);

       gf_assert(fb_vma != NULL, NULL);

       //gf_info(" save fb vidmm base 0x%x, fb phys 0x%x, fb virt addr 0x%x \n", adapter->vidmm_bus_addr, map.phys_addr, fb_vma->virt_addr);

       len = gf_file_write(file, fb_vma->virt_addr, map.size);
       WRITE_DUMP_FILE_CHECK(map.size, len);

       gf_unmap_io_memory(fb_vma);
       fb_vma = NULL;
       i++;
       length -= map.size;
       gf_info("[DUMP HANG] left cpu visible segment mem:%llx\n", length);
    }
#endif
    gf_info("vidsch_dump_hang_e3k end dump visible memory...\n");
    gf_file_close(file);
    gf_msleep(50000);
    gf_assert(0, "vidsch_dump_hang_e3k save to file successed");
}

int vidsch_save_misc_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, unsigned int what_to_save)
{
    engine_e3k_t *engine                = sch_mgr->private_data;

    dh_common_info_e3k     *dh_common_info;

    dh_common_info = (dh_common_info_e3k*)(vidschi_get_dump_data_cpu_virt_addr_e3k(adapter, HANG_DUMP_DH_COMMON_INFO_OFFSET, 0));

    //save dma buffer and context buffer
    if(what_to_save == SAVE_BEFORE_POSTHANG)
    {
        dh_common_info->current_hang_dump_index = (dh_common_info->current_hang_dump_index + 1) % MAX_HANG_DUMP_DATA_POOL_NUMBER;
        dh_common_info->hang_dump_counter++;
#if NEW_DUMP

#else

        unsigned long long     buffer_phys_addr;
        unsigned char*         buffer_virt_addr;

        if(engine->last_dma_buffer)
        {
            buffer_virt_addr = vidschi_get_dump_data_cpu_virt_addr_e3k(adapter, HANG_DUMP_DMA_BUFFER_OFFSET,  dh_common_info->current_hang_dump_index);
            gf_memcpy((void*)buffer_virt_addr, engine->last_dma_buffer, engine->last_dma_buffer_size_uint<<2);
        }

        if(engine->context_buffer_to_restore_addr)
        {
             buffer_phys_addr = vidschi_get_dump_data_gpu_offset_e3k(adapter, HANG_DUMP_CONTEXT_BUFFER_OFFSET,  dh_common_info->current_hang_dump_index);
             buffer_virt_addr  = vidschi_get_dump_data_cpu_virt_addr_e3k(adapter, HANG_DUMP_CONTEXT_BUFFER_OFFSET, dh_common_info->current_hang_dump_index);

             vidschi_copy_mem_e3k(adapter,
                                 buffer_phys_addr,
                                 buffer_virt_addr,
                                 engine->context_buffer_to_restore_addr,
                                 NULL,
                                 CONTEXT_BUFFER_SIZE);
        }
#endif
    }

    else if(what_to_save == SAVE_AFTER_POSTHANG)
    {
        dh_rb_info_e3k  *dh_rb_info;
        unsigned char   *ring_buffer  = NULL;

        //save hang info to common info
        dh_common_info->hang_dma_fence_id = sch_mgr->hang_fence_id;
        dh_common_info->hang_dma_index = sch_mgr->hang_index;
        gf_info("vidsch_save_misc_e3k hang fence id:%d index:%d\n", dh_common_info->hang_dma_fence_id, dh_common_info->hang_dma_index);
        //save status
        ring_buffer  = vidschi_get_dump_data_cpu_virt_addr_e3k(adapter, HANG_DUMP_RING_BUFFER_OFFSET, dh_common_info->hang_dma_index);
        dh_rb_info   = (dh_rb_info_e3k*)ring_buffer;
        ring_buffer += sizeof(dh_rb_info_e3k);
        dh_rb_info->status = engine->dumped_engine_status;

        gf_info("vidsch_save_misc_e3k save status: %08x,%08x,%08x,%08x,%08x,%08x,%08x\n",
        dh_rb_info->status.Top.uint,dh_rb_info->status.Gpc0_0.uint,dh_rb_info->status.Gpc0_1.uint,
        dh_rb_info->status.Gpc1_0.uint,dh_rb_info->status.Gpc1_1.uint,dh_rb_info->status.Gpc2_0.uint,dh_rb_info->status.Gpc2_1.uint);
    }
    else if(what_to_save == SAVE_RING_BUFFER)
    {
        dh_rb_info_e3k  *dh_rb_info;
        unsigned char   *ring_buffer  = NULL;
        EngineSatus_e3k status        = {0};

        //save ring buffer
        ring_buffer  = vidschi_get_dump_data_cpu_virt_addr_e3k(adapter, HANG_DUMP_RING_BUFFER_OFFSET, dh_common_info->current_hang_dump_index);
        dh_rb_info   = (dh_rb_info_e3k*)ring_buffer;
        ring_buffer += sizeof(dh_rb_info_e3k);
        dh_rb_info->status = status;
        dh_rb_info->last_rb_index = sch_mgr->engine_index;

        switch(sch_mgr->engine_index)
        {
            case RB_INDEX_GFXL:
                dh_rb_info->last_rb_size  = engine->last_ring_buffer_size;
                gf_memcpy((void*)ring_buffer, engine->last_ring_buffer, engine->last_ring_buffer_size);
                break;
            case RB_INDEX_GFXH:
            case RB_INDEX_VCP0:
            case RB_INDEX_VCP1:
            case RB_INDEX_VPP:
            default:
                gf_error("Not yet add support engine hang..., engine:%d\n", sch_mgr->engine_index);
        }
    }
    return dh_common_info->current_hang_dump_index;
}

void vidsch_handle_dbg_hang_dump_e3k(adapter_t *adapter, vidsch_mgr_t *sch_mgr, task_dma_t *task_dma)
{
    engine_e3k_t *engine = sch_mgr->private_data;
    engine_share_e3k_t      *share   = engine->share;
    hw_ctxbuf_t *hwctx_info = task_dma->hw_ctx_info;

    if(debug_mode_e3k.post_hang_dump)
    {
        if (task_dma->need_hwctx_switch)
        {
            if (!hwctx_info->is_initialized)
            {
                engine->context_buffer_to_restore_addr = share->context_buf_phy_addr;
            }
            else
            {
                engine->context_buffer_to_restore_addr = hwctx_info->context_buffer_address;
            }
        }
        else
        {
            engine->context_buffer_to_restore_addr = 0;
        }
        task_dma->desc.hang_index = vidsch_save_misc_e3k(adapter, sch_mgr, SAVE_BEFORE_POSTHANG);
        if (task_dma->need_hwctx_switch)
        {
            task_dma->desc.hang_context = vidschi_get_dump_data_gpu_offset_e3k(adapter, HANG_DUMP_CONTEXT_BUFFER_OFFSET,  task_dma->desc.hang_index);
        }
    }
}

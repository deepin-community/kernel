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

typedef enum dump_data_type{
    HANG_DUMP_NONE,
    HANG_DUMP_CONTEXT_BUFFER,
    HANG_DUMP_DMA_BUFFER,
    HANG_DUMP_RING_BUFFER,
    HANG_DUMP_COMMON_INFO,

    HANG_DUMP_GART_TABLE_L2,
    HANG_DUMP_GART_TABLE_L3,
    HANG_DUMP_KICKOFF_RING_BUFFER,
    HANG_DUMP_FENCE_BUFFER,
    HANG_DUMP_FLUSH_FIFO_BUFFER,
    HANG_DUMP_BL_BUFFER,
    HANG_DUMP_CPU_UNVISIBLE_MEMORY
} dump_data_type;

#if 0
static void _get_engine_status(adapter_t *adapter)
{
    EngineSatus_e3k status = {0};
    unsigned int *p = (unsigned int *)&status;
    int i = 0;

    for(i=0; i<7; i++)
    {
        *(p + i) = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + (Reg_Block_Busy_Bits_Top_Offset + i)*4);
    }

    gf_info("[Duplicate] engine 0x%x, Duplicated Hang Status:Top=%x,Gpc0_0=%x,Gpc0_1=%x,Gpc1_0=%x, Gpc1_1=%x,Gpc2_0=%x,Gpc2_1=%x\n",
        0,
        status.Top.uint,
        status.Gpc0_0.uint,
        status.Gpc0_1.uint,
        status.Gpc1_0.uint,
        status.Gpc1_1.uint,
        status.Gpc2_0.uint,
        status.Gpc2_1.uint);
}
#endif

static void _set_fence_id(dup_hang_ctx_t *dup_hang_ctx, unsigned long long fence_id)
{
    submit_info_t *submit_info = &(dup_hang_ctx->submit_info);
    void          *fence_cpu_va = submit_info->fence_buffer_cpu_va;

    *(unsigned int *)fence_cpu_va = fence_id & 0xffffffff;
    *(((unsigned int *)fence_cpu_va) + 1) = (fence_id >> 32) & 0xffffffff;

}

static unsigned long long _get_fence_id(dup_hang_ctx_t *dup_hang_ctx)
{
    submit_info_t  *submit_info = &(dup_hang_ctx->submit_info);
    void           *fence_cpu_va = submit_info->fence_buffer_cpu_va;
    unsigned long long fence_id = 0;

    fence_id = *(unsigned long long *)fence_cpu_va;

    return fence_id;
}

static unsigned long long _get_dump_data_gpu_va(dup_hang_ctx_t *dup_hang_ctx, int type, int index)
{
    unsigned long long offset = 0;

    switch(type)
    {
        case HANG_DUMP_CONTEXT_BUFFER:
            offset = dup_hang_ctx->context_buffer_offset + index * dup_hang_ctx->single_context_buffer_size_in_byte;
            break;

        case HANG_DUMP_DMA_BUFFER:
            offset = dup_hang_ctx->dma_buffer_offset + index * dup_hang_ctx->single_dma_size_in_byte;
            break;

        case HANG_DUMP_RING_BUFFER:
            offset = dup_hang_ctx->ring_buffer_offset + index * dup_hang_ctx->single_ring_buffer_size_in_byte;
            break;

        case HANG_DUMP_COMMON_INFO:
            offset  = dup_hang_ctx->ring_buffer_offset + MAX_HANG_DUMP_DATA_POOL_NUMBER * dup_hang_ctx->single_ring_buffer_size_in_byte;
            break;

        case HANG_DUMP_GART_TABLE_L2:
            offset  = dup_hang_ctx->gart_table_l2_offset;
            break;

        case HANG_DUMP_GART_TABLE_L3:
            offset  = dup_hang_ctx->gart_table_l3_offset;
            break;

        case HANG_DUMP_FENCE_BUFFER:
            {
                dh_common_info_e3k  dummy_common_info = {0};
                unsigned long long  common_info_offset = 0;
                unsigned long long  fence_buffer_offset = 0;

                common_info_offset = _get_dump_data_gpu_va(dup_hang_ctx, HANG_DUMP_COMMON_INFO, index);

                fence_buffer_offset = common_info_offset + OFFSETOF(dh_common_info_e3k, fence_buffer)/sizeof(DWORD);

                offset = fence_buffer_offset;
            }
            break;

        case HANG_DUMP_KICKOFF_RING_BUFFER:
            {
                dh_common_info_e3k  dummy_common_info = {0};
                unsigned long long  common_info_offset = 0;
                unsigned long long  kick_off_ring_buffer_offset = 0;
                unsigned long long  kick_off_ring_buffer_offset_4k_aligned = 0;

                common_info_offset = _get_dump_data_gpu_va(dup_hang_ctx, HANG_DUMP_COMMON_INFO, index);

                kick_off_ring_buffer_offset = common_info_offset + OFFSETOF(dh_common_info_e3k, ring_buffer)/sizeof(DWORD);

                kick_off_ring_buffer_offset_4k_aligned = (kick_off_ring_buffer_offset + 0xfffULL) & (~0xfffULL);

                offset = kick_off_ring_buffer_offset_4k_aligned;
            }
            break;

        case HANG_DUMP_FLUSH_FIFO_BUFFER:
            {
                dh_common_info_e3k  dummy_common_info = {0};
                unsigned long long  common_info_offset = 0;
                unsigned long long  flush_fifo_buffer_offset = 0;

                common_info_offset = _get_dump_data_gpu_va(dup_hang_ctx, HANG_DUMP_COMMON_INFO, index);

                flush_fifo_buffer_offset = common_info_offset + OFFSETOF(dh_common_info_e3k, flush_fifo_buffer)/sizeof(DWORD);

                offset = flush_fifo_buffer_offset;
            }
            break;

        case HANG_DUMP_BL_BUFFER:
            offset = dup_hang_ctx->bl_buffer_offset;
            break;

        case HANG_DUMP_CPU_UNVISIBLE_MEMORY:
            offset = dup_hang_ctx->local_visible_size;
            break;

        default:
            gf_assert(0, "invalid type");
    }

    return offset;
}

static void* _get_dump_data_cpu_va(dup_hang_ctx_t *dup_hang_ctx, int type, int index)
{
    void            *virt_addr = 0;
    unsigned long long gpu_va = 0;

    gpu_va    = _get_dump_data_gpu_va(dup_hang_ctx, type, index);

    switch(type)
    {
        case HANG_DUMP_CONTEXT_BUFFER:
        case HANG_DUMP_DMA_BUFFER:
        case HANG_DUMP_RING_BUFFER:
        case HANG_DUMP_COMMON_INFO:
        case HANG_DUMP_GART_TABLE_L2:
        case HANG_DUMP_GART_TABLE_L3:
        case HANG_DUMP_FENCE_BUFFER:
        case HANG_DUMP_KICKOFF_RING_BUFFER:
        case HANG_DUMP_FLUSH_FIFO_BUFFER:
        case HANG_DUMP_BL_BUFFER:
            virt_addr = dup_hang_ctx->local_visible_memory_cpu_va + gpu_va;
            break;

        case HANG_DUMP_CPU_UNVISIBLE_MEMORY:
#ifdef VMI_MODE
            adapter_t       *adapter = dup_hang_ctx->adapter;
            virt_addr = ((void *)adapter->vidmm_bus_addr) + gpu_va;
#else
            gf_assert(0, "cannot get virt_addr for CPU_UNVISIBLE_MEMORY");
#endif
            break;

        default:
            gf_assert(0, "invalid type");
            break;
    }

    return virt_addr;
}

static void _fill_and_write_file_header(adapter_t *adapter, HangDumpFileHeader *file_header_info, struct os_file *file)
{
    engine_share_e3k_t *share           = adapter->private_data;

    gf_memset(file_header_info, 0, sizeof(HangDumpFileHeader));

    gf_info("[DUMP HANG] ................................Start dump cpu file header................................\n");

    file_header_info->nHeaderVersion = 0x0001;
    file_header_info->nDeviceID = adapter->bus_config.device_id;
    file_header_info->nSliceMask = adapter->hw_caps.chip_slice_mask;
    file_header_info->nHangDumpFileHeaderSizeInByte = sizeof(HangDumpFileHeader);
    file_header_info->nAdapterMemorySizeInByte = adapter->Real_vram_size;
    file_header_info->nPCIEMemorySizeInByte = adapter->gart_ram_size;
    file_header_info->nRecordNums = 6;

    gf_info("%s() verion-0x%x, device_id-0x%x, slice_mask-0x%x, header_size-%d, local_size-0x%llx, pcie_size-0x%llx\n", __func__,
              file_header_info->nHeaderVersion,
              file_header_info->nDeviceID,
              file_header_info->nSliceMask,
              file_header_info->nHangDumpFileHeaderSizeInByte,
              file_header_info->nAdapterMemorySizeInByte,
              file_header_info->nPCIEMemorySizeInByte);

    file_header_info->nDmaOffset = share->dma_buffer_for_hang->gpu_virt_addr;//hAdapter->HangDumpInfo.DMAForHang.GPUVirtualAddress.QuadPart;
    file_header_info->nSingleDmaSizeInByte = HangDump_SingleDmaBufferSize;//HangDump_SingleDmaBufferSize;

    file_header_info->nRingBufferOffset = share->ring_buffer_for_hang->gpu_virt_addr;
    file_header_info->nSingleRingBufferSizeInByte = HangDump_RingBufferBlockSize;

    file_header_info->nContextOffset = share->context_buffer_for_hang->gpu_virt_addr;
    file_header_info->nSingleContextSizeInByte = HangDump_ContextBufferBlockSize;

    file_header_info->nGartTableL2Offset = adapter->gart_table_L2->phys_addr;//hAdapter->GartTableL2.GPUVirtualAddress.QuadPart;
    file_header_info->nGartTableL3Offset = adapter->gart_table_L3->phys_addr;//hAdapter->GartTableL3.GPUVirtualAddress.QuadPart;

    file_header_info->nTransferBufferOffsetInFBFile = share->transfer_buffer_for_hang->gpu_virt_addr;//hAdapter->HangDumpInfo.TransferBuffer.GPUVirtualAddress.QuadPart;

    file_header_info->dummyPageEntry = adapter->dummy_page_addr;//dummyPageEntry.uint;

    file_header_info->nBlBufferOffset = share->bl_buffer->gpu_virt_addr;

    gf_file_write(file, file_header_info, sizeof(HangDumpFileHeader));

    gf_info("[DUMP HANG] ................................End dump cpu file header................................\n");

    return;
}

static void _write_local_to_file(adapter_t *adapter, struct os_file *file)
{
    engine_share_e3k_t *share = adapter->private_data;
    unsigned long long total_size  = adapter->Real_vram_size;
    unsigned long long left_size   = total_size;
    unsigned long long src_gpu_va  = 0;
    unsigned long long blit_size   = 0;
    vidsch_mgr_t       *sch_mgr    = adapter->sch_mgr[RB_INDEX_GFXL];
    task_desc_t        *task  = NULL;
    task_dma_t         *dma_task;
    unsigned char      *dma_copy_src;
    gf_info("[DUMP HANG] ................................Start dump local memory................................\n");

#if NEW_DUMP
    gf_mutex_lock(sch_mgr->task_list_lock);
    list_for_each_entry(task, &sch_mgr->submitted_task_list, list_item)
    {
         dma_task = (task_dma_t *)task;
         if(dma_task->dma_type != PAGING_DMA && task->hang_index >= 0 && task->hang_index < 5)
         {
             gf_info("the line is %d\n", __LINE__);
             dma_copy_src = vidschi_get_dump_data_cpu_virt_addr_e3k(adapter, HANG_DUMP_DMA_BUFFER_OFFSET, task->hang_index);
             gf_info("the dma_task->hang_index is %d\n" , task->hang_index);
             gf_info("the dma_task address is 0x%x\n", dma_task);
             gf_info("the size is 0x%x\n", dma_task->dma_buffer_node->cmd_size);
             gf_memcpy(dma_copy_src, dma_task->dma_buffer_node->virt_base_addr, dma_task->dma_buffer_node->cmd_size);
          }
    }
    gf_mutex_unlock(sch_mgr->task_list_lock);
#endif

    while(left_size)
    {
        blit_size = left_size > E3K_TRANSFERBUFFER_FOR_HANG_SIZE ? E3K_TRANSFERBUFFER_FOR_HANG_SIZE : left_size;

        vidschi_copy_mem_e3k(adapter,
                             share->transfer_buffer_for_hang->gpu_virt_addr,
                             NULL,
                             src_gpu_va,
                             NULL,
                             blit_size);

        gf_file_write(file, share->transfer_buffer_for_hang->vma->virt_addr, blit_size);

        src_gpu_va += blit_size;
        left_size -= blit_size;

        gf_info("[DUMP HANG] ................................End dump local memory %lldM / %lldM................................\n", (total_size - left_size) >> 20, total_size >> 20);
    }

    gf_info("[DUMP HANG] ................................End dump local memory................................\n");

    return;
}

static void _write_pcie_to_file(adapter_t *adapter, struct os_file *file)
{
    engine_share_e3k_t *share = adapter->private_data;
    unsigned long long total_size = adapter->gart_ram_size;
    unsigned long long left_size = total_size;
    unsigned long long src_gpu_va = adapter->Real_vram_size;
    unsigned long long blit_size = 0;

    gf_info("[DUMP HANG] ................................Start dump pcie memory................................\n");

    while(left_size)
    {
        blit_size = left_size > E3K_TRANSFERBUFFER_FOR_HANG_SIZE ? E3K_TRANSFERBUFFER_FOR_HANG_SIZE : left_size;

        vidschi_copy_mem_e3k(adapter,
                             share->transfer_buffer_for_hang->gpu_virt_addr,
                             NULL,
                             src_gpu_va,
                             NULL,
                             blit_size);

        gf_file_write(file, share->transfer_buffer_for_hang->vma->virt_addr, blit_size);

        src_gpu_va += blit_size;
        left_size -= blit_size;

        gf_info("[DUMP HANG] ................................End dump pcie memory %lldM / %lldM................................\n", (total_size - left_size) >> 20, total_size >> 20);
    }

    gf_info("[DUMP HANG] ................................End dump pcie memory................................\n");

    return;
}

void vidsch_dump_hang_compatible_e3k(adapter_t *adapter)
{
    struct os_file  *file;
    HangDumpFileHeader file_header_info = {0};

    if(!debug_mode_e3k.post_hang_dump)
    {
        gf_error("vidsch_dump_hang_e3k skip this function, status error...\n");
        return;
    }

    debug_mode_e3k.internal_dump_hw = 1;


    file = gf_file_open("/var/fb.FB", OS_RDWR | OS_CREAT | OS_APPEND | OS_LARGEFILE, 0666);
    if(!file)
    {
        gf_info("create file: create file /var/fb.FB fail!\n");
        return;
    }

    _fill_and_write_file_header(adapter, &file_header_info, file);

    _write_local_to_file(adapter, file);

    _write_pcie_to_file(adapter, file);

    gf_file_close(file);

    gf_info("[DUMP HANG] ................................End dump ................................\n");

    gf_msleep(50000);
    gf_assert(0, NULL);
}

static void _load_file_header(dup_hang_ctx_t *dup_hang_ctx)
{
    struct os_file    *file = dup_hang_ctx->file;
    adapter_t         *adapter = dup_hang_ctx->adapter;
    HangDumpFileHeader file_header_info = {0};
    unsigned long long len = 0;

    len = gf_file_read(file, &file_header_info, sizeof(file_header_info), NULL);

    gf_assert(len == sizeof(file_header_info), "");
    gf_assert(file_header_info.nHeaderVersion == 0x0001, "head version is wrong");
    gf_assert(file_header_info.nDeviceID == adapter->bus_config.device_id, "device id is wrong");
    gf_assert(file_header_info.nSliceMask == adapter->hw_caps.chip_slice_mask, "slice_mask is wrong");
    gf_assert(file_header_info.nHangDumpFileHeaderSizeInByte == sizeof(HangDumpFileHeader), "hang dump file header is wrong");
    gf_assert(file_header_info.nAdapterMemorySizeInByte == adapter->Real_vram_size, "local memory size is wrong");
    gf_assert(file_header_info.nPCIEMemorySizeInByte == adapter->gart_ram_size, "pcie memory size is wrong");

    dup_hang_ctx->device_id = file_header_info.nDeviceID;
    dup_hang_ctx->slice_mask = file_header_info.nSliceMask;
    dup_hang_ctx->local_memory_size = file_header_info.nAdapterMemorySizeInByte;
    dup_hang_ctx->pcie_memory_size = file_header_info.nPCIEMemorySizeInByte;
    dup_hang_ctx->record_number = file_header_info.nRecordNums;
    dup_hang_ctx->dma_buffer_offset = file_header_info.nDmaOffset;
    dup_hang_ctx->single_dma_size_in_byte = file_header_info.nSingleDmaSizeInByte;
    dup_hang_ctx->context_buffer_offset = file_header_info.nContextOffset;
    dup_hang_ctx->single_context_buffer_size_in_byte = file_header_info.nSingleContextSizeInByte;
    dup_hang_ctx->ring_buffer_offset = file_header_info.nRingBufferOffset;
    dup_hang_ctx->single_ring_buffer_size_in_byte = file_header_info.nSingleRingBufferSizeInByte;
    dup_hang_ctx->gart_table_l3_offset = file_header_info.nGartTableL3Offset;
    dup_hang_ctx->gart_table_l2_offset = file_header_info.nGartTableL2Offset;
    dup_hang_ctx->dummy_page_entry = file_header_info.dummyPageEntry;
    dup_hang_ctx->bl_buffer_offset = file_header_info.nBlBufferOffset;

    dup_hang_ctx->local_visible_size = 512*1024*1024;
    dup_hang_ctx->local_unvisible_size = dup_hang_ctx->local_memory_size - dup_hang_ctx->local_visible_size;
}

static void _load_local_unvisible_from_file(dup_hang_ctx_t *dup_hang_ctx)
{
    struct os_file     *file    = dup_hang_ctx->file;
    adapter_t          *adapter = dup_hang_ctx->adapter;
    engine_share_e3k_t *share  = adapter->private_data;
    unsigned long long local_visible_size = dup_hang_ctx->local_visible_size;
    unsigned long long local_unvisible_size = dup_hang_ctx->local_unvisible_size;
    unsigned long long total_size = local_unvisible_size;
    unsigned long long left_size  = total_size;
    unsigned long long dst_gpu_va = local_visible_size;
    unsigned int   blit_size  = 0;
    unsigned int   len   = 0;
    unsigned long long pos = sizeof(HangDumpFileHeader) + local_visible_size;
#ifdef VMI_MODE
    void *virt_addr = _get_dump_data_cpu_va(dup_hang_ctx, HANG_DUMP_CPU_UNVISIBLE_MEMORY, 0);
#endif

    gf_info("[Duplicate] ................................Start load invisible local mem................................\n");

    while(left_size)
    {
        blit_size = left_size >  E3K_TRANSFERBUFFER_FOR_HANG_SIZE ? E3K_TRANSFERBUFFER_FOR_HANG_SIZE : left_size;

#ifdef VMI_MODE
        len = gf_file_read(file, virt_addr, blit_size, &pos);
        gf_assert(blit_size == len, "");

        virt_addr += blit_size;
#else
        //1. read out to transfer buffer
        len = gf_file_read(file, share->transfer_buffer_for_hang->vma->virt_addr, blit_size, &pos);
        gf_assert(blit_size == len, "");

        //2.
        vidschi_copy_mem_e3k(adapter,
                             dst_gpu_va,
                             NULL,
                             share->transfer_buffer_for_hang->gpu_virt_addr,
                             NULL,
                             blit_size);

        dst_gpu_va += blit_size;
#endif
        left_size  -= blit_size;

        gf_info("[Duplicate] ................................End load local invisible memory %lldM / %lldM................................\n", (total_size - left_size) >> 20, total_size >> 20);
    }

    gf_info("[Duplicate] ................................End load invisible local mem................................\n");
}

static void _load_local_visible_from_file(dup_hang_ctx_t *dup_hang_ctx)
{
    struct os_file *file = dup_hang_ctx->file;
    adapter_t      *adapter = dup_hang_ctx->adapter;
    gf_map_argu_t   map     = {0};
    gf_vm_area_t   *vm_area = NULL;
    void           *addr = NULL;
    unsigned long long total_size = dup_hang_ctx->local_visible_size;
    unsigned long long left_size  = total_size;
    unsigned int    blit_size  = 0;
    unsigned int    len   = 0;
    unsigned long long pos = sizeof(HangDumpFileHeader);

    gf_info("[Duplicate] ................................Start load local mem................................\n");


    //1. map local
    //Note:
    // following mapping always fail in one disk with "conflicting" error, but pass in another disk, both of them are ubuntu2004
    map.flags.cache_type= GF_MEM_UNCACHED;
    map.flags.mem_space = GF_MEM_KERNEL;
    map.flags.mem_type  = GF_SYSTEM_IO;
    map.phys_addr       = adapter->vidmm_bus_addr;
    map.size            = total_size;

    vm_area = gf_map_io_memory(NULL, &map);

    addr = vm_area->virt_addr;
    while(left_size)
    {
        blit_size = left_size > E3K_TRANSFERBUFFER_FOR_HANG_SIZE ? E3K_TRANSFERBUFFER_FOR_HANG_SIZE : left_size;

        //1. copy
        len = gf_file_read(file, addr, blit_size, &pos);
        gf_assert(blit_size == len, "size of read out is wrong");

        //2. update pointer
        addr       += blit_size;
        left_size  -= blit_size;

        gf_info("[Duplicate] ................................End load local visible memory %lldM / %lldM................................\n", (total_size - left_size) >> 20, total_size >> 20);
    }

    gf_info("[Duplicate] ................................End load local mem................................\n");

    //save cpu visible memory cpu va
    dup_hang_ctx->local_visible_memory_cpu_va = vm_area->virt_addr;
}

static void _restore_ttbr(dup_hang_ctx_t *dup_hang_ctx)
{
    adapter_t       *adapter = dup_hang_ctx->adapter;
    unsigned char   *pRegAddr = 0;
    Reg_Ttbr         reg_Ttbr = {0};
    unsigned long long gart_table_l2_offset = dup_hang_ctx->gart_table_l2_offset;
    unsigned int    ttbrn_for_total = (dup_hang_ctx->local_memory_size + dup_hang_ctx->pcie_memory_size + 0x100000000 - 1) / 0x100000000;//per ttbr can index 4G mem, total has 256 ttbr regs for pa mode.
    unsigned int    i = 0;

    gf_assert(ttbrn_for_total <= 256,"total ttbr number excced");

    for(i = 0; i < ttbrn_for_total; i++)
    {
        reg_Ttbr.reg.Segment = 1;
        reg_Ttbr.reg.Valid   = 1;
        reg_Ttbr.reg.Addr = (gart_table_l2_offset + 4096 * i) >> 12;
        pRegAddr = adapter->mmio + MMIO_MMU_START_ADDRESS + (Reg_Mmu_Ttbr_Offset + i)*4;

        gf_write32(pRegAddr, reg_Ttbr.uint);
        //gf_info(" ttbr %d set 0x%x, readvalue: 0x%x \n", i,reg_Ttbr.uint, zxg_read32(pRegAddr));
    }
}

static void _restore_gart_table_l2(dup_hang_ctx_t *dup_hang_ctx)
{
    unsigned long long  gart_table_l3_offset = dup_hang_ctx->gart_table_l3_offset;
    void               *gart_table_l2_cpu_va = _get_dump_data_cpu_va(dup_hang_ctx, HANG_DUMP_GART_TABLE_L2, 0);
    unsigned int       *pte_list   = NULL;
    PTE_L2_t            page_entry  = {0};
    unsigned long long  L2EntryNumForL3      = dup_hang_ctx->pcie_memory_size/(4*1024*1024);
    unsigned long long  L2EntryNumForLocal   = dup_hang_ctx->local_memory_size/(4*1024*1024);
    unsigned long long  i=0;

    //1. update pte
    page_entry._4k_d.Valid = TRUE;
    page_entry._4k_d.Segment= 1; // 1 means this pte point to local mem
    page_entry._4k_d.Addr= 0;

    pte_list = gart_table_l2_cpu_va;

    for(i = 0; i < L2EntryNumForLocal; i++)
    {
        pte_list[i] = page_entry.uint;
    }

    for(i = 0; i < L2EntryNumForL3; i++)
    {
        page_entry._4k_d.Addr = ((gart_table_l3_offset >> 12) + i) &0xFFFFFF;
        pte_list[i + L2EntryNumForLocal] = page_entry.uint;
    }
}

static void _restore_gart_table_l3_and_pcie_memory_ext(dup_hang_ctx_t *dup_hang_ctx)
{
    struct os_file          *file = dup_hang_ctx->file;
    struct os_pages_memory  *memory = NULL;
    gf_vm_area_t            *vma = NULL;
    adapter_t               *adapter = dup_hang_ctx->adapter;
    void                    *gart_table_l3_cpu_va = _get_dump_data_cpu_va(dup_hang_ctx, HANG_DUMP_GART_TABLE_L3, 0);
    unsigned long long      pcie_mem_size = dup_hang_ctx->pcie_memory_size;
    unsigned long long      total_size = pcie_mem_size;
    unsigned long long      left_size = 0;
    unsigned long long      dst_gpu_va = 0;
    unsigned long long      pos = sizeof(HangDumpFileHeader) + dup_hang_ctx->local_memory_size;
    unsigned int            blit_size = 0;
    unsigned int            len = 0;

    Reg_Pt_Inv_Trig         reg_Pt_Inv_Trig = {0};
    Reg_Pt_Inv_Addr         reg_Pt_Inv_Addr = {0};
    Reg_Pt_Inv_Mask         reg_Pt_Inv_Mask = {0};


    dst_gpu_va = dup_hang_ctx->local_memory_size;
    left_size  = total_size;

    gf_info("[Duplicate] ................................Start load pcie mem................................\n");

    while(left_size)
    {
        blit_size = left_size > E3K_TRANSFERBUFFER_FOR_HANG_SIZE ? E3K_TRANSFERBUFFER_FOR_HANG_SIZE : left_size;

        //1. allocate pages
        {
            alloc_pages_flags_t alloc_flags = {0};

            alloc_flags.dma32 = FALSE;
            alloc_flags.need_zero = TRUE;
            alloc_flags.fixed_page = TRUE;
            alloc_flags.need_dma_map = FALSE;
            alloc_flags.need_flush  = FALSE;

            memory = gf_allocate_pages_memory(adapter->os_device.pdev, blit_size, 1 << 12, alloc_flags);

            gf_assert(memory != NULL, "fail to allocate pages");
        }

        //2. update gart-table
        {
            vidmm_update_gart_table_pte_e3k(adapter, memory, gart_table_l3_cpu_va, dst_gpu_va);
        }

        //3. map pcie memory
        {
            gf_map_argu_t map        = {0};

            map.flags.mem_space  = GF_MEM_KERNEL;
            map.flags.cache_type = GF_MEM_WRITE_COMBINED;
            map.flags.mem_type   = GF_SYSTEM_RAM;
            map.size             = blit_size;
            map.memory           = memory;

#ifdef VMI_MODE
            gf_set_pages_addr(memory, adapter->vidmm_bus_addr + dst_gpu_va);
#endif
            vma = gf_map_pages_memory(NULL, &map);
        }

        //4. copy pcie memory
        {
            len = gf_file_read(file, vma->virt_addr, blit_size, &pos);
            gf_assert(len==blit_size, "");
        }

        //5. unmap pcie memory
        {
            gf_unmap_pages_memory(vma);
        }

        dst_gpu_va += blit_size;
        left_size  -= blit_size;


        gf_info("[Duplicate] ................................End load pcie memory %dM / %dM................................\n", (total_size - left_size) >> 20, total_size >> 20);
    }

    gf_info("[Duplicate] ................................Start load pcie mem................................\n");


    // invalidate gart_table
    reg_Pt_Inv_Addr.uint = 0x0;
    reg_Pt_Inv_Mask.reg.Mask = 0x0;
    reg_Pt_Inv_Trig.reg.Target = PT_INV_TRIG_TARGET_DTLB;

    gf_write32(adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Pt_Inv_Addr_Offset*4 , reg_Pt_Inv_Addr.uint);
    gf_write32(adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Pt_Inv_Mask_Offset*4, reg_Pt_Inv_Mask.uint);
    gf_write32(adapter->mmio + MMIO_MXU_START_ADDRESS + Reg_Pt_Inv_Trig_Offset*4, reg_Pt_Inv_Trig.uint);
 }

static void _restore_gart_table_and_pcie_memory(dup_hang_ctx_t *dup_hang_ctx)
{
    //1. ttbr
    //actually no need to restore, if local/pcie is the same between dump image and the duplicate machine
    _restore_ttbr(dup_hang_ctx);

    //2. l2
    //actually seems no need to restore too, because l3 is in local
    _restore_gart_table_l2(dup_hang_ctx);

    //3. l3
    //_restore_gart_table_l3_and_pcie_memory(adapter, file, dh_file_info);
    _restore_gart_table_l3_and_pcie_memory_ext(dup_hang_ctx);
}

static void _load_data_from_file(dup_hang_ctx_t *dup_hang_ctx)
{
    struct os_file *file;
    file = gf_file_open("/var/fb.FB", OS_RDWR | OS_LARGEFILE, 0666);

    if(!file)
    {
        gf_error("open file: open file /var/fb.FB fail.\n");
        return;
    }

    dup_hang_ctx->file    = file;
    _load_file_header(dup_hang_ctx);

    if(dup_hang_ctx->local_unvisible_size)
    {
        _load_local_unvisible_from_file(dup_hang_ctx);
    }

    _load_local_visible_from_file(dup_hang_ctx);

    _restore_gart_table_and_pcie_memory(dup_hang_ctx);
}

static void _enable_new_ring_buffer(dup_hang_ctx_t *dup_hang_ctx)
{
    adapter_t                  *adapter                     = dup_hang_ctx->adapter;
    submit_info_t              *submit_info                 = &(dup_hang_ctx->submit_info);
    Reg_Run_List_Ctx_Addr1      reg_Run_List_Ctx_Addr1      = { 0 };
    Reg_Ring_Buf_Size           reg_Ring_Buf_Size           = { 0 };
    Reg_Ring_Buf_Head           reg_Ring_Buf_Head           = { 0 };
    Reg_Ring_Buf_Tail           reg_Ring_Buf_Tail           = { 0 };
    unsigned long long          ring_buffer_gpu_va          = submit_info->kickoff_ring_buffer_gpu_va;
    int                         RbIndex                     = submit_info->rb_index;
    unsigned int                RegRbOffset = EngineRbOffset(RbIndex);

    reg_Ring_Buf_Tail.reg.Rb_Tail =
    reg_Ring_Buf_Head.reg.Rb_Head = 0;
    reg_Ring_Buf_Size.reg.Rb_Size = RING_BUFFER_SIZE_E3K;

    reg_Run_List_Ctx_Addr1.reg.Addr = (ring_buffer_gpu_va >> 12) & 0xFFFFFFF;
    reg_Run_List_Ctx_Addr1.reg.Kickoff = 0;

    gf_write32(adapter->mmio + RegRbOffset     * 4 + MMIO_CSP_START_ADDRESS, reg_Run_List_Ctx_Addr1.uint );
    gf_write32(adapter->mmio + (RegRbOffset+1) * 4 + MMIO_CSP_START_ADDRESS, reg_Ring_Buf_Size.uint );
    gf_write32(adapter->mmio + (RegRbOffset+2) * 4 + MMIO_CSP_START_ADDRESS, reg_Ring_Buf_Head.uint );
    gf_write32(adapter->mmio + (RegRbOffset+3) * 4 + MMIO_CSP_START_ADDRESS, reg_Ring_Buf_Tail.uint );

    reg_Run_List_Ctx_Addr1.reg.Kickoff = 1;

    gf_write32( adapter->mmio + RegRbOffset * 4 + MMIO_CSP_START_ADDRESS, reg_Run_List_Ctx_Addr1.uint );
}

static void _fill_ring_buffer_and_cmd(dup_hang_ctx_t *dup_hang_ctx, unsigned long long fence_value)
{
    submit_info_t  *submit_info          = &(dup_hang_ctx->submit_info);
    unsigned long long context_buffer_gpu_va  = submit_info->context_buffer_gpu_va;
    unsigned long long dma_buffer_gpu_va     = submit_info->dma_buffer_gpu_va;
    unsigned long long fence_buffer_gpu_va   = submit_info->fence_buffer_gpu_va;
    void*          ring_buffer_cpu_va    = submit_info->ring_buffer_cpu_va;
    RINGBUFFER_COMMANDS_E3K *submit_cmd = submit_info->kickoff_ring_buffer_cpu_va;

    gf_memcpy(submit_cmd, ring_buffer_cpu_va, sizeof(RINGBUFFER_COMMANDS_E3K));

    //patch restore dma address
    if(submit_cmd->c0.RestoreContext_Address != 0)
    {
        submit_cmd->c0.RestoreContext_Address = (context_buffer_gpu_va >> 11);
    }

    //patch dma adress
    submit_cmd->c0.CommandDMA_Address_L       = (dma_buffer_gpu_va & 0xFFFFFFFF);
    submit_cmd->c0.CommandDMA_Address_H       = ((dma_buffer_gpu_va >>32) & 0xFF);

    //patch save dma address.
    //Assume the hang task dma not the first dma, since the context_buffer_pa for save will overwrite the template.
    gf_memset(&submit_cmd->c0.SaveDMA, 0, 4*sizeof(DWORD));

    *(unsigned int*)&submit_cmd->c0.Fence = SEND_EXTERNAL_FENCE_E3K(FENCE_IRQ_INTERRUPT_CPU);
    submit_cmd->c0.Fence_Data_L = (fence_value) & 0xFFFFFFFF;
    submit_cmd->c0.Fence_Data_H = ((fence_value) >>32) & 0xFFFFFFFF;
    submit_cmd->c0.Fence_Address_L = (fence_buffer_gpu_va) & 0xFFFFFFFF;
    submit_cmd->c0.Fence_Address_H = (fence_buffer_gpu_va >> 32) & 0xFFFFFFFF;

    //util_dump_memory(NULL, submit_cmd, sizeof(*submit_cmd), "duplicate ring buffer");
    //util_dump_memory(NULL, submit_info->dma_buffer_cpu_va, (submit_cmd->c0.CommandDMA.Dw_Num)<<2, "duplicate dma buffer");

}

static void _kick_off_cmd(dup_hang_ctx_t *dup_hang_ctx, unsigned long long last_send_fence_id)
{
    adapter_t   *adapter = dup_hang_ctx->adapter;
    submit_info_t *submit_info = &(dup_hang_ctx->submit_info);
    void        *flush_fifo_buffer_cpu_va = _get_dump_data_cpu_va(dup_hang_ctx, HANG_DUMP_FLUSH_FIFO_BUFFER, 0);
    unsigned int RbIndex = submit_info->rb_index;
    unsigned int RegRbOffset = EngineRbOffset(RbIndex);
    unsigned int offset = 0;
    unsigned int tail = sizeof(RINGBUFFER_COMMANDS_E3K);

    gf_mb();

    {
        *((volatile unsigned int*)flush_fifo_buffer_cpu_va + RbIndex) =  (unsigned int)last_send_fence_id;

        while(*((volatile unsigned int*)flush_fifo_buffer_cpu_va + RbIndex) !=  (unsigned int)last_send_fence_id)
        {
            gf_error("flush fifo issue index:%x\n",RbIndex);
            gf_error("krnl virt addr: %p, write value: %x, read value: %x\n",
                ((volatile unsigned int*)flush_fifo_buffer_cpu_va + RbIndex),
                (unsigned int)last_send_fence_id,
                *((volatile unsigned int*)flush_fifo_buffer_cpu_va + RbIndex));
           // gf_assert(0, NULL);
        }
    }

    gf_mb();

    gf_write32(adapter->mmio +  (RegRbOffset+3) * 4 + MMIO_CSP_START_ADDRESS, tail);

    gf_mb();

    do{
        offset = gf_read32(adapter->mmio +  (RegRbOffset+3) * 4 + MMIO_CSP_START_ADDRESS);
        if(offset != tail)
        {
            gf_error("idx:%d, kick off:%x, read:%x\n", RbIndex, tail, offset);
        }
    }while(tail != offset);
}

static void _check_hang(dup_hang_ctx_t *dup_hang_ctx, int pool_index, unsigned long long fence_value)
{
    adapter_t *adapter = dup_hang_ctx->adapter;
    submit_info_t *submit_info = &(dup_hang_ctx->submit_info);
    EngineSatus_e3k status_after = {0};
    EngineSatus_e3k *status_before = &submit_info->status;
    int             i = 0;

    unsigned long long      start_time = 0, current_time, delta_time;
    long                    temp_sec, temp_usec;

    gf_getsecs(&temp_sec, &temp_usec);
    start_time = temp_sec;

    gf_info("%s() start_time-%lld, send_fence-0x%llx, read_fence-0x%llx\n", __func__, start_time, fence_value, _get_fence_id(dup_hang_ctx));

    while(fence_value > _get_fence_id(dup_hang_ctx))
    {

        gf_getsecs(&temp_sec, &temp_usec);
        current_time = temp_sec;
        delta_time   = current_time - start_time;

        /* wait 60s, if fence still not back seems something wrong */
        if(delta_time > DUPLICATE_TIME_OUT)
        {
            unsigned int *status = (unsigned int *)&status_after;

            gf_info("%s() fail, current_time-%lld, delta_time-%lld, send_fence-0x%llx, read_fence-0x%llx\n", __func__, current_time, delta_time, fence_value, _get_fence_id(dup_hang_ctx));

            //get status
            for(i=0; i<7; i++)
            {
                *(status + i) = gf_read32(adapter->mmio + MMIO_CSP_START_ADDRESS + (Reg_Block_Busy_Bits_Top_Offset + i)*4);
            }
            gf_info("[Duplicate] engine 0x%x, Duplicated Hang Status:Top=%x,Gpc0_0=%x,Gpc0_1=%x,Gpc1_0=%x, Gpc1_1=%x,Gpc2_0=%x,Gpc2_1=%x\n",
                    submit_info->rb_index,
                    status_after.Top.uint,
                    status_after.Gpc0_0.uint,
                    status_after.Gpc0_1.uint,
                    status_after.Gpc1_0.uint,
                    status_after.Gpc1_1.uint,
                    status_after.Gpc2_0.uint,
                    status_after.Gpc2_1.uint);

            //vidsch_display_debugbus_info_e3k(adapter, NULL, FALSE);

            if((status_before->Top.uint & 0xFFFFFFF9) == (status_after.Top.uint & 0xFFFFFFF9) && //exclude mxua/b, diu will use it, even 3d hang.
                (status_before->Gpc0_0.uint == status_after.Gpc0_0.uint) &&
                (status_before->Gpc0_1.uint == status_after.Gpc0_1.uint) &&
                (status_before->Gpc1_0.uint == status_after.Gpc1_0.uint) &&
                (status_before->Gpc1_1.uint == status_after.Gpc1_1.uint) &&
                (status_before->Gpc2_0.uint == status_after.Gpc2_0.uint) &&
                (status_before->Gpc2_1.uint == status_after.Gpc2_1.uint))
            {
                gf_info("[Duplicate] <<-------------------:) Good job!, Status is same.------------------------>>.\n");
                gf_assert(0, "[Duplicate] <<-------------------:) Congratulation!! ------------------------>>\n");
            }
            else
            {
                gf_info("[Duplicate]<<------------------:( Sorry, Status is different.-------------------------->>.\n");

                gf_info("[Duplicate]engine 0x%x, Org Status: Top=%x,Gpc0_0=%x,Gpc0_1=%x,Gpc1_0=%x, Gpc1_1=%x,Gpc2_0=%x,Gpc2_1=%x\n",
                        submit_info->rb_index,
                        status_before->Top.uint,
                        status_before->Gpc0_0.uint,
                        status_before->Gpc0_1.uint,
                        status_before->Gpc1_0.uint,
                        status_before->Gpc1_1.uint,
                        status_before->Gpc2_0.uint,
                        status_before->Gpc2_1.uint);
                gf_assert(0, "[Duplicate] <<-------------------:( Do more check !! ------------------------>>\n");
            }
        }
    }

    gf_info("%s() pass, send_fence-0x%llx, read_fence-0x%llx\n", __func__, fence_value, _get_fence_id(dup_hang_ctx));

    return;
}

static void _get_submit_info(dup_hang_ctx_t *dup_hang_ctx, int pool_index)
{
    submit_info_t  *submit_info = &(dup_hang_ctx->submit_info);
    void           *ring_buffer_block = NULL;
    dh_rb_info_e3k *rb_info = NULL;

    submit_info->ring_buffer_gpu_va = _get_dump_data_gpu_va(dup_hang_ctx, HANG_DUMP_RING_BUFFER, pool_index) + sizeof(dh_rb_info_e3k);
    submit_info->context_buffer_gpu_va = _get_dump_data_gpu_va(dup_hang_ctx, HANG_DUMP_CONTEXT_BUFFER, pool_index);
    submit_info->dma_buffer_gpu_va = _get_dump_data_gpu_va(dup_hang_ctx, HANG_DUMP_DMA_BUFFER, pool_index);

    ring_buffer_block = _get_dump_data_cpu_va(dup_hang_ctx, HANG_DUMP_RING_BUFFER, pool_index);
    rb_info = (dh_rb_info_e3k *)ring_buffer_block;

    submit_info->ring_buffer_cpu_va = ring_buffer_block + sizeof(dh_rb_info_e3k);
    submit_info->context_buffer_cpu_va  = (void *)_get_dump_data_cpu_va(dup_hang_ctx, HANG_DUMP_CONTEXT_BUFFER, pool_index);
    submit_info->dma_buffer_cpu_va = (void *)_get_dump_data_cpu_va(dup_hang_ctx, HANG_DUMP_DMA_BUFFER, pool_index);

    submit_info->fence_buffer_gpu_va = _get_dump_data_gpu_va(dup_hang_ctx, HANG_DUMP_FENCE_BUFFER, pool_index);
    submit_info->fence_buffer_cpu_va = _get_dump_data_cpu_va(dup_hang_ctx, HANG_DUMP_FENCE_BUFFER, pool_index);
    submit_info->kickoff_ring_buffer_gpu_va = _get_dump_data_gpu_va(dup_hang_ctx, HANG_DUMP_KICKOFF_RING_BUFFER, pool_index);
    submit_info->kickoff_ring_buffer_cpu_va = _get_dump_data_cpu_va(dup_hang_ctx, HANG_DUMP_KICKOFF_RING_BUFFER, pool_index);

    gf_memcpy(&submit_info->status, &rb_info->status, sizeof(submit_info->status));

    submit_info->rb_index = rb_info->last_rb_index;

}


static void _duplicate_hang(dup_hang_ctx_t *dup_hang_ctx)
{
    int record_number = dup_hang_ctx->record_number;
    int pool_index = 0;
    unsigned long long fence_value = 0xff123456ff000001;
    dh_common_info_e3k *dh_common_info = _get_dump_data_cpu_va(dup_hang_ctx, HANG_DUMP_COMMON_INFO, 0);
    submit_info_t      *submit_info = &(dup_hang_ctx->submit_info);

    //pool_index = dh_common_info->current_hang_dump_index;
    pool_index = dh_common_info->hang_dma_index; // Arise is differen to CHX004: arise will store the exact dma_index of the hang pool

    do
    {
        pool_index = pool_index < record_number ? pool_index : 0;

        gf_info("[Duplicate] pool_index=%d, count=%d, dump_index=0x%x\n",
                    pool_index, dh_common_info->hang_dump_counter, dh_common_info->current_hang_dump_index);

        gf_memset(submit_info, 0, sizeof(submit_info_t));

        //0. get info about submit
        _get_submit_info(dup_hang_ctx, pool_index);

        _set_fence_id(dup_hang_ctx, fence_value -1);

        switch(submit_info->rb_index)
        {
            case RB_INDEX_GFXL:
            {

                //1. setup ringbuffer
                _enable_new_ring_buffer(dup_hang_ctx);

                //2. reproduce cmd
                _fill_ring_buffer_and_cmd(dup_hang_ctx, fence_value);

                //3. kick off cmd
                _kick_off_cmd(dup_hang_ctx, fence_value-1);

                break;
            }

            case RB_INDEX_GFXH:
            default:
                break;
        }

        //3. check hang
        _check_hang(dup_hang_ctx, pool_index, fence_value);

        pool_index++;
        fence_value++;
    }
    while (debug_mode_e3k.duplicate_hang);
}

void vidsch_duplicate_hang_compatible_e3k(adapter_t * adapter)
{
    dup_hang_ctx_t dup_hang_ctx = {0};

    if(!debug_mode_e3k.duplicate_hang)
    {
        return;
    }

    //block any other submit:
    debug_mode_e3k.internal_block_submit = 1;


    gf_memset(&dup_hang_ctx, 0, sizeof(dup_hang_ctx));

    dup_hang_ctx.adapter = adapter;

    //1. restore image to memory
    _load_data_from_file(&dup_hang_ctx);

    //2. duplicate hang
    _duplicate_hang(&dup_hang_ctx);

    debug_mode_e3k.internal_block_submit = 0;
}

void vidsch_trigger_duplicate_hang_e3k(adapter_t *adapter)
{
    debug_mode_e3k.duplicate_hang = 1;

    vidsch_duplicate_hang_e3k(adapter);

    debug_mode_e3k.duplicate_hang = 0;
}

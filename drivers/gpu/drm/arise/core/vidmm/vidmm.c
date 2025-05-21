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
#include "global.h"
#include "vidmm.h"
#include "vidmmi.h"
#include "heap_manager.h"
#include "vidsch.h"
#include "vidschi.h"

static int vidmmi_get_dummy_page_addr(void *arg, int pg_start, int pg_cnt, unsigned long long dma_addr)
{
    *(unsigned long long*)arg = dma_addr;

    return 1;
}

static int vidmmi_init_gart_table(adapter_t *adapter)
{
    vidmm_mgr_t             *mm_mgr         = adapter->mm_mgr;
    vidmm_gart_table_info_t *gart_table_L3  = NULL;
    vidmm_gart_table_info_t *gart_table_L2  = NULL;
    vidmm_segment_memory_t  *segment_memory = NULL;
    vidmm_map_flags_t        map_flags      = {0};

    int result = S_OK;

    if (adapter->dummy_page_addr)
    {
        return S_OK;
    }

    gf_pages_memory_for_each_continues(mm_mgr->dummy_page, &adapter->dummy_page_addr, vidmmi_get_dummy_page_addr);
    gf_info("dummy page phys addr %llx\n", adapter->dummy_page_addr);

    {
        if(adapter->hw_caps.page_4k_enable)
        {
            gart_table_L3 = gf_calloc(sizeof(vidmm_gart_table_info_t));
            gart_table_L2 = gf_calloc(sizeof(vidmm_gart_table_info_t));

            gf_assert(gart_table_L3 != NULL && gart_table_L2 != NULL, "gart_table_L3 != NULL && gart_table_L2 != NULL");

            adapter->gart_table_L3 = gart_table_L3;
            adapter->gart_table_L2 = gart_table_L2;

            mm_mgr->chip_func->query_gart_table_info(adapter);
        }

        if(gart_table_L3 && gart_table_L3->size)
        {
            segment_memory = vidmm_allocate_segment_memory(adapter, gart_table_L3->segment_id, gart_table_L3->size, 1);

            gart_table_L3->phys_addr = segment_memory->gpu_virt_addr; //pagetable address need aligned to cache line [E3K_CACHE_LINE 0xFF], heap mgr will aligned to 4k automaticlly.

            map_flags.mem_space = GF_MEM_KERNEL;
            map_flags.cache_type = GF_MEM_WRITE_COMBINED;

            vidmm_map_segment_memory(adapter, NULL, segment_memory, &map_flags);

            gart_table_L3->virt_addr  = segment_memory->vma->virt_addr;
            gart_table_L3->page_count = gart_table_L3->size / sizeof(int);//one entery point to one page
            gart_table_L3->segment_memory = segment_memory;
            gart_table_L3->dirty          = TRUE;
        }

        if(gart_table_L2 && gart_table_L2->size)
        {
            segment_memory = vidmm_allocate_segment_memory(adapter, gart_table_L2->segment_id, gart_table_L2->size, 1);

            gart_table_L2->phys_addr = segment_memory->gpu_virt_addr;

            map_flags.mem_space  = GF_MEM_KERNEL;
            map_flags.cache_type = GF_MEM_UNCACHED;

            vidmm_map_segment_memory(adapter, NULL, segment_memory, &map_flags);

            gart_table_L2->virt_addr  = segment_memory->vma->virt_addr;
            gart_table_L2->page_count = gart_table_L2->size / sizeof(int);//one entery point to one page
            gart_table_L2->segment_memory = segment_memory;
            gart_table_L2->dirty          = TRUE;
        }

        mm_mgr->chip_func->init_gart_table(adapter);

        return result;
    }
}

static void vidmmi_destroy_gart_table(adapter_t *adapter)
{
    vidmm_mgr_t             *mm_mgr     = adapter->mm_mgr;
    vidmm_gart_table_info_t *gart_table = NULL;

    if (adapter->dummy_page_addr)
    {
        adapter->dummy_page_addr = 0LL;
    }

    if(adapter->gart_table_L3)
    {
        gart_table = adapter->gart_table_L3;

        vidmm_unmap_segment_memory(adapter, gart_table->segment_memory, GF_MEM_KERNEL);

        vidmm_release_segment_memory(adapter, gart_table->segment_memory);

        gart_table->segment_memory = NULL;

        if(gart_table->chip_private)
        {
            mm_mgr->chip_func->deinit_gart_table(adapter);

            gart_table->chip_private = NULL;
        }

        gf_free(gart_table);

        adapter->gart_table_L3= NULL;
    }

    if(adapter->gart_table_L2)
    {
        gart_table = adapter->gart_table_L2;

        vidmm_unmap_segment_memory(adapter, gart_table->segment_memory, GF_MEM_KERNEL);

        vidmm_release_segment_memory(adapter, gart_table->segment_memory);

        gart_table->segment_memory = NULL;

        if(gart_table->chip_private)
        {
            mm_mgr->chip_func->deinit_gart_table(adapter);

            gart_table->chip_private = NULL;
        }

        gf_free(gart_table);

        adapter->gart_table_L2 = NULL;
    }
}

#if DEBUG_TEST
void vidmmi_test_video_memory(adapter_t *adapter)
{
       unsigned int  offset;
       volatile unsigned int *frame_buffer;
       unsigned long dummy_data = 0;
       unsigned int  result     = S_OK;
       unsigned int  scan_scope = 0x400000;//adapter->physical_bus_base_length[1];//cpu visiable
       vidmm_mgr_t   *mm_mgr    = adapter->mm_mgr;
       unsigned int  scan_line  = 0x400000;//remapio will assert in vmalloc if size is larger than vmalloc total size. add to avoid this. or can add vmalloc='scan_scope' in boot manu
       unsigned int  scan_loop  = 0;
       unsigned int  map_size   = scan_line;
       gf_map_argu_t    map    = {0};
       gf_vm_area_t     *vma   = NULL;
       unsigned int count = 0;

       gf_info("==================test local memory begin==========================\n");
       gf_info("total size = %x, vidmm_bus_addr=%x \n", scan_scope, adapter->vidmm_bus_addr);

       //test local video memory
       for(scan_loop=0; scan_loop<scan_scope; scan_loop+=scan_line)
       {
           map_size = (scan_scope-scan_loop) < scan_line ?  scan_scope-scan_loop : scan_line;
           {
               map.flags.cache_type= GF_MEM_UNCACHED;
               map.flags.mem_space = GF_MEM_KERNEL;
               map.flags.mem_type  = GF_SYSTEM_IO;
               map.phys_addr       = adapter->vidmm_bus_addr+scan_loop;
               map.size            = map_size;

               vma = gf_map_io_memory(NULL, &map);

               gf_assert(vma!=NULL, "vidmm_test vma!=NULL");

               frame_buffer = vma->virt_addr;
           }

           for(offset=0; offset<map_size/4; offset++)
           {
               dummy_data = offset;
               frame_buffer[offset] = dummy_data;
               dummy_data = frame_buffer[offset];
               frame_buffer[offset] = 0;

               if(dummy_data != offset)
               {
                   gf_info("test video momory failed: PA=%x, value=%x, expected:%x\n",(adapter->vidmm_bus_addr + scan_scope+ offset*4),dummy_data,offset);
                   if(count++ >500) break;
               }
           }

           {
               gf_unmap_io_memory(vma);
               vma = NULL;
           }
           gf_info("passed %xth 4M ~~~ %xth 4M.\n", scan_loop/scan_line, scan_loop/scan_line+1);
       }

       gf_info("==================test local memory end==========================\n");
}
#endif

static void vidmmi_map_flag_buffer(adapter_t *adapter)
{
    vidmm_segment_t    *segment    = NULL;
    int                segment_id  = 1;//hardcode for local segment id
    int                map_size    = 1024*1024;//1M
    gf_map_argu_t     map         = {0};
    gf_vm_area_t      *vm_area    = NULL;

    segment = vidmm_get_segment_by_id(adapter, segment_id);

    map.flags.mem_space  = GF_MEM_KERNEL;
    map.size             = map_size;
    map.memory           = segment->reserved_pages;
    map.offset           = 0;
    map.flags.mem_type   = GF_SYSTEM_RAM;
    map.flags.read_only  = 0;
    map.flags.write_only = 0;

    if(segment->reserved_pages == NULL)
    {
        gf_info("memory reserved is NULL \n");
        return;
    }

    vm_area = gf_map_pages_memory(NULL, &map);
    if(vm_area == NULL)
    {
        gf_error("map addr is NULL\n");
        gf_assert(0, "vidmm map flag buffer");
    }
    else
    {
        adapter->flag_buffer_virt_addr = vm_area->virt_addr;
        gf_info("%s: flag buffer virt_addr: %p\n", __func__, vm_area->virt_addr);

        gf_memset(vm_area->virt_addr, 0, map_size);
    }
}


#define VMEM_START_ALIGN           (64*1024)
#define SYSTEM_RESERVED_PAGES_SIZE (256 * 1024 * 1024)
#define EMERENCY_PAGES_SIZE        ( 64 * 1024 * 1024)

int vidmm_init(adapter_t *adapter, int reserved_vmem)
{
    vidmm_mgr_t               *mm_mgr             = NULL;
    vidmm_segment_t           *segment            = NULL;
    vidmm_chip_segment_info_t *chip_segment_info  = gf_calloc(sizeof(vidmm_chip_segment_info_t));
    int                        result             = S_OK;
    int                        id                 = 0;
    int                        priority           = 0;
    unsigned long long         max_size           = 0;
    mem_info_t                 mem                = {0};
    unsigned long long         heap_size, heap_start;

    mm_mgr = gf_calloc(sizeof(vidmm_mgr_t));

    if(NULL == mm_mgr)
    {
        gf_error("vidmm_init failed.\n");
        result = E_OUTOFMEMORY;
        goto exit;
    }

    adapter->mm_mgr       = mm_mgr;
    mm_mgr->adapter       = adapter;
    mm_mgr->reserved_vmem = (reserved_vmem + VMEM_START_ALIGN - 1) & (~(VMEM_START_ALIGN -1));
    mm_mgr->hw_lock       = gf_create_mutex();
    mm_mgr->list_lock     = gf_create_mutex();

    list_init_head(&mm_mgr->defer_destroy_list);
    list_init_head(&mm_mgr->paging_task_list);

    /* calc max use pages size by real system memory info to avoid use much system memory */

    gf_get_mem_info(&mem);

    if(mem.totalram > 2 * SYSTEM_RESERVED_PAGES_SIZE/adapter->os_page_size)
    {
        mm_mgr->max_pages_num   = mem.totalram - ((SYSTEM_RESERVED_PAGES_SIZE - EMERENCY_PAGES_SIZE)/adapter->os_page_size);
        mm_mgr->emergency_pages = EMERENCY_PAGES_SIZE/adapter->os_page_size;
    }
    else
    {
        mm_mgr->max_pages_num   = mem.totalram >> 1;
        mm_mgr->emergency_pages = mem.totalram >> 2;
    }

#ifdef ANDROID
    {
        mm_mgr->max_pages_num   = mem.totalram;
        mm_mgr->emergency_pages = mem.totalram;
    }
#endif

    gf_info("GPU can used system memory size: %dk. set GPU max used size as: %dk.\n",
         mem.totalram * adapter->os_page_size >> 10,  mm_mgr->max_pages_num * adapter->os_page_size>> 10);

    mm_mgr->chip_func = &vidmm_chip_func;

    /*
     * init segment desc & heap
     */
    mm_mgr->chip_func->query_segment_info(adapter, chip_segment_info);

    mm_mgr->cpu_visible_vidmm_size   = chip_segment_info->cpu_visible_vidmm_size;
    mm_mgr->cpu_unvisible_vidmm_size = chip_segment_info->cpu_unvisible_vidmm_size;
    mm_mgr->segment_cnt              = chip_segment_info->segment_cnt;
    mm_mgr->segment                  = gf_calloc(sizeof(vidmm_segment_t)*mm_mgr->segment_cnt);

    /* invalid segment id 0, not describe none video memory but just system cache, we name it as cache segment */
    segment = &mm_mgr->segment[SEGMENT_ID_INVALID];

    segment->segment_id        = SEGMENT_ID_INVALID;
    segment->segment_name      = "Segment Invalid";
    segment->flags.cpu_visible = TRUE;
    segment->lock              = gf_create_mutex();

    list_init_head(&segment->unpagable_resident_list);

    for(priority = PDISCARD; priority < PALL; priority++)
    {
        list_init_head(&segment->pagable_resident_list[priority]);
    }

    /* describe valid video segment */
    for(id = 1; id < mm_mgr->segment_cnt; id++)
    {
        vidmm_segment_desc_t *segment_desc = &chip_segment_info->segment_desc[id];

        segment = &mm_mgr->segment[id];

        segment->segment_id         = segment_desc->segment_id;
        segment->segment_name       = segment_desc->segment_name;
        segment->flags.value        = segment_desc->flags.value;
        segment->segment_alignment  = segment_desc->segment_alignment;
        segment->gpu_vm_start       = segment_desc->gpu_vm_start;
        segment->reserved_vm_end    = segment_desc->gpu_vm_start + segment_desc->reserved_vm_size;
        segment->gpu_vm_size        = segment_desc->gpu_vm_size;
        segment->small_heap_size    = segment_desc->small_heap_size;
        segment->reserved_vm_size   = segment_desc->reserved_vm_size;
        segment->phys_addr_start    = segment_desc->phys_addr_start;
        segment->lock               = gf_create_mutex();
        segment->small_heap_max_allocate_size = segment_desc->small_heap_max_allocate_size;

        list_init_head(&segment->unpagable_resident_list);

        for(priority = PDISCARD; priority < PALL; priority++)
        {
            list_init_head(&segment->pagable_resident_list[priority]);
        }

        /* only select the max pcie segment as swap segment for easy the logic. on swap func we first try swap cache segment,
         * if cache not enough, we paging out swap segment to cache, and try swap it.
         */
        if(segment->flags.require_system_pages && (segment->gpu_vm_size > max_size))
        {
            max_size = segment->gpu_vm_size;
        }

        if(segment->flags.secure_range &&(mm_mgr->secure_segment_id == SEGMENT_ID_INVALID) )
        {
            mm_mgr->secure_segment_id = segment->segment_id;
            segment->secure_lock      = gf_create_mutex();
        }

        heap_start = segment->reserved_vm_end;
        heap_size  = segment->gpu_vm_size - segment->reserved_vm_size;

        if(segment->flags.small_heap_available)
        {
            heap_size -= segment->small_heap_size;
        }

        heap_init(&segment->heap, id, heap_start, heap_size, (segment->flags.page_64kb_enable ? GF_PAGE_64KB_SIZE : segment->segment_alignment));

        if(segment->flags.small_heap_available)
        {
            heap_start = segment->heap.start + segment->heap.size;
            heap_size  = segment->small_heap_size;

            heap_init(&segment->small_heap, id, heap_start, heap_size, segment->segment_alignment);
        }

        if(segment->gpu_vm_size > 0)
        {
            gf_info("segment[%d]: range: [%lldM -> %lldM], size: %lldM, start reserved: %dk\n",
                id, segment->gpu_vm_start >> 20, (segment->gpu_vm_start + segment->gpu_vm_size) >> 20,
                segment->gpu_vm_size >> 20, segment->reserved_vm_size >> 10);
        }

        segment->mtrr_reg = -1;

        if(segment->flags.mtrr)
        {
            /* On DST, driver will reserver some mem to video, so the size not power of 2, which lead mtrr_add fail
             * Juat add 1M align here to bypass it.
             */

            unsigned long size = (segment->gpu_vm_size + 0xFFFFF)& (~0xFFFFF); //1M align

            segment->mtrr_reg = gf_mtrr_add(segment->gpu_vm_start + adapter->vidmm_bus_addr, size);
        }
    }

    if(adapter->hw_caps.fb_hdaudio_enable)
    {
        vidmm_segment_memory_t   *reserved_16m_memory = NULL;

        reserved_16m_memory = vidmm_allocate_segment_memory(adapter, 1, 16*1024*1024, 1); // reserve 16M

        adapter->reserve_16m_buffer = reserved_16m_memory;
        gf_info("reserved 16M for gf_hda, address=0x%llx\n", reserved_16m_memory->gpu_virt_addr);
    }

    if (adapter->ctl_flags.vesa_tempbuffer_enable)
    {
        adapter->vesafb_tempbuffer = vidmm_allocate_segment_memory(adapter, 1, 1920*1080*4, 0);
        gf_info("vesa_tempbuffer_enable: temp allocation 1920*1080*4\n");
    }

    if(adapter->UnAval_vram_size)
    {
        adapter->mem_unavailable_for_3miu = vidmm_allocate_segment_memory(adapter, 5, adapter->UnAval_vram_size, 1);
    }

    /*
     * for paging segment is common for all chip, so we initial it in share layer.
     */
    if(adapter->ctl_flags.paging_enable)
    {
        vidmm_segment_t *paging_segment      = NULL;
        list_node_t     *paging_segment_node = NULL;

        /* init paging semgent description */
        paging_segment                   = gf_calloc(sizeof(vidmm_segment_t));
        mm_mgr->paging_segment           = paging_segment;
        paging_segment->segment_id       = chip_segment_info->paging_segment_id;
        paging_segment->gpu_vm_size      = chip_segment_info->paging_segment_size;
        paging_segment->reserved_vm_size = 0;
        paging_segment->segment_alignment= adapter->os_page_size;

        segment = &mm_mgr->segment[paging_segment->segment_id];

        paging_segment_node              = heap_allocate(&segment->heap, paging_segment->gpu_vm_size - paging_segment->reserved_vm_size, adapter->os_page_size, 1);
        paging_segment->gpu_vm_start     = paging_segment_node->aligned_offset;
        paging_segment->reserved_vm_end  = paging_segment->gpu_vm_start + paging_segment->reserved_vm_size;
        paging_segment->flags.support_snoop   =  segment->flags.support_snoop;
        mm_mgr->paging_segment_node      = paging_segment_node;

        /* init paging heap */
        heap_init(&paging_segment->heap, -1, paging_segment->gpu_vm_start, paging_segment->gpu_vm_size - paging_segment->reserved_vm_size, adapter->os_page_size);
    }

    /*
     * create dummy page for gart table
     */
    {
       alloc_pages_flags_t alloc_flags = {0};
       alloc_flags.dma32 = TRUE;
       alloc_flags.need_zero = TRUE;
       alloc_flags.fixed_page = TRUE;
       alloc_flags.need_dma_map = adapter->sys_caps.iommu_enabled;
       mm_mgr->dummy_page = gf_allocate_pages_memory(adapter->os_device.pdev, adapter->os_page_size, adapter->os_page_size, alloc_flags);

#ifdef VMI_MODE
       gf_set_pages_addr(mm_mgr->dummy_page, 0);
#endif

       if (mm_mgr->dummy_page == NULL)
       {
           gf_error("vidmm_init: allocate dummy page failed.\n");
           goto exit;
       }
    }
    /*
     * init gart table
    */
    //if(!adapter->hw_caps.local_only)
    {
        result = vidmmi_init_gart_table(adapter);
        if(S_OK != result)
        {
            gf_error("vidmm_init: init gart table fail.\n");
            goto exit;
        }
    }

    if(mm_mgr->chip_func && mm_mgr->chip_func->mem_setting)
    {
        mm_mgr->chip_func->mem_setting(adapter);
    }

#if DEBUG_TEST
    vidmmi_test_video_memory(adapter);
#endif

    //This is used to check if hw access 4G~8G mem will overwrite to 0~4G.
#if DEBUG_HIGH_4G_MEM
    {
        vidmm_map_flags_t        flags            = {0};
        vidmm_segment_memory_t   *reserved_memory = NULL;

        reserved_memory = vidmm_allocate_segment_memory(adapter, 1, HIGH_4G_MEM_RESERVE_SIZE, 0);

        gf_memset(&flags, 0, sizeof(flags));

        flags.write_only = 1;
        flags.mem_space  = GF_MEM_KERNEL;

        vidmm_map_segment_memory(adapter, NULL, reserved_memory, &flags);
        gf_memset(reserved_memory->vma->virt_addr, 0, HIGH_4G_MEM_RESERVE_SIZE);

        adapter->Reserve_Buffer = reserved_memory;
        gf_info("Mem Reserve for high 4g test pa is:%x\n", reserved_memory->gpu_virt_addr);
    }
#endif
    if(adapter->ctl_flags.flag_buffer_verify)
    {
        vidmmi_map_flag_buffer(adapter);
    }

    //vidmmi_dump_segments(adapter);

exit:
    gf_free(chip_segment_info);

    return result;
}

void vidmm_destroy(adapter_t *adapter)
{
    vidmm_mgr_t     *mm_mgr  = adapter->mm_mgr;
    vidmm_segment_t *segment = NULL;
    int              id      = 0;

    if (adapter->vesafb_tempbuffer)
    {
        vidmm_release_segment_memory(adapter, adapter->vesafb_tempbuffer);
        adapter->vesafb_tempbuffer = NULL;

        gf_info("free vesa_tempbuffer_enable\n");
    }
    if(adapter->mem_unavailable_for_3miu)
    {
        vidmm_release_segment_memory(adapter, adapter->mem_unavailable_for_3miu);
        adapter->mem_unavailable_for_3miu = NULL;

        gf_info("free mem_unavailable_for_3miu\n");
    }

#if DEBUG_HIGH_4G_MEM
    if (adapter->Reserve_Buffer)
    {
        vidmm_release_segment_memory(adapter, adapter->Reserve_Buffer);
        adapter->Reserve_Buffer = NULL;
    }
#endif

    if (adapter->hw_caps.fb_hdaudio_enable && adapter->reserve_16m_buffer)
    {
        vidmm_release_segment_memory(adapter, adapter->reserve_16m_buffer);
        adapter->reserve_16m_buffer = NULL;
    }

    if (mm_mgr->dummy_page)
    {
        gf_free_pages_memory(adapter->os_device.pdev, mm_mgr->dummy_page);
        mm_mgr->dummy_page = NULL;
    }

    vidmmi_destroy_gart_table(adapter);

    if(mm_mgr->paging_segment)
    {
        list_node_t *paging_segment_node = mm_mgr->paging_segment_node;
        int          paging_segment_id   = mm_mgr->paging_segment->segment_id;

        segment = mm_mgr->paging_segment;

        mm_mgr->paging_segment = NULL;

        heap_destroy(&segment->heap);

        gf_free(segment);

        heap_release(&mm_mgr->segment[paging_segment_id].heap, paging_segment_node);
    }

    if(mm_mgr->segment)
    {
        segment = &mm_mgr->segment[0];

        gf_destroy_mutex(segment->lock);

        for(id = 1; id < mm_mgr->segment_cnt; id++)
        {
            segment = &mm_mgr->segment[id];

            if(segment->mtrr_reg >= 0)
            {
                unsigned long size = (segment->gpu_vm_size + 0xFFFFF)& (~0xFFFFF); //1M align

                gf_mtrr_del(segment->mtrr_reg, segment->gpu_vm_start + adapter->vidmm_bus_addr, size);

                segment->mtrr_reg = -1;
            }

            if(segment->reserved_pages)
            {
                gf_free_pages_memory(adapter->os_device.pdev, segment->reserved_pages);
                segment->reserved_pages = NULL;
            }

            if(segment->flags.small_heap_available)
            {
                heap_destroy(&segment->small_heap);
            }

            heap_destroy(&segment->heap);

            gf_destroy_mutex(segment->lock);

            if(segment->secure_lock)
            {
                gf_destroy_mutex(segment->secure_lock);
            }
        }

        gf_free(mm_mgr->segment);

        mm_mgr->segment = NULL;
    }

#if defined(GF_HW_NULL)
    if(adapter->mmio_mem)
    {
        gf_unmap_pages_memory(adapter->mmio_vma);
        adapter->mmio_vma = NULL;

        gf_free_pages_memory(adapter->os_device.pdev, adapter->mmio_mem);
        adapter->mmio_mem = NULL;
    }
#endif

    gf_destroy_mutex(mm_mgr->list_lock);
    gf_destroy_mutex(mm_mgr->hw_lock);

    if(adapter->mm_mgr)
    {
        gf_free(adapter->mm_mgr);
        adapter->mm_mgr = NULL;
    }
}

vidmm_segment_t *vidmm_get_segment_by_id(adapter_t *adapter, unsigned int segment_id)
{
    vidmm_mgr_t     *vidmm = adapter->mm_mgr;
    vidmm_segment_t *segment = NULL;

    if((SEGMENT_ID_INVALID == segment_id) || (segment_id >= vidmm->segment_cnt))
    {
        gf_info("%s(): invalid segment_id-%d\n", __func__, segment_id);
        gf_assert(0, "vidmm_get_segment_by_id invalid segment_id");
        return segment;
    }

    segment = &vidmm->segment[segment_id];
    return segment;
}

heap_t * vidmm_get_burst_length_heap(adapter_t *adapter)
{
    return &adapter->mm_mgr->blheap;
}
int vidmm_get_segment_count(adapter_t *adapter)
{
    return adapter->mm_mgr->segment_cnt;
}

#define GART_BLT_SUSPEND 1

int vidmm_save(adapter_t *adapter)
{
    vidmm_mgr_t *mm_mgr = adapter->mm_mgr;
    vidmm_segment_memory_t *segment = NULL;
    vidmm_gart_table_info_t *gart_table_L3 = adapter->gart_table_L3;
    vidmm_gart_table_info_t *gart_table_L2 = adapter->gart_table_L2;
    int result = E_OUTOFMEMORY;

    gart_table_L2->backup = gf_malloc(gart_table_L2->size);
    if (gart_table_L2->backup)
    {
        gf_memcpy(gart_table_L2->backup, gart_table_L2->virt_addr, gart_table_L2->size);
    }

#if GART_BLT_SUSPEND
    // In arm64, memcpy 8MB from IO memory to Physical memory will cost about 4s.
    // So we use vidmm_segment_memory_transfer_e3k() to submit a page task
    // Help us copy gart_table_L3->segment_memory to a new segment_memory.
    result = mm_mgr->chip_func->segment_memory_transfer(adapter, &segment, gart_table_L3->segment_memory, 0);
    gart_table_L3->backup = (void *)segment;
#else

    gart_table_L3->backup = gf_malloc(gart_table_L3->size);
    if (gart_table_L3->backup)
    {
        gf_memcpy(gart_table_L3->backup, gart_table_L3->virt_addr, gart_table_L3->size);
        result = S_OK;
    }
#endif

    adapter->fence_buf_local->backup = gf_malloc(68 * 1024);
    if (adapter->fence_buf_local->backup)
        gf_memcpy(adapter->fence_buf_local->backup, adapter->fence_buf_local->reserved_memory->vma->virt_addr, 68 * 1024);

    if (!adapter->fence_buf->reserved_memory->pages_mem)
    {
        adapter->fence_buf->backup = gf_malloc(512 * 1024);
        if (adapter->fence_buf->backup)
            gf_memcpy(adapter->fence_buf->backup, adapter->fence_buf->reserved_memory->vma->virt_addr, 512 * 1024);
    }

    return result;
}

void vidmm_restore(adapter_t *adapter)
{
    vidmm_gart_table_info_t *gart_table_L3 = adapter->gart_table_L3;
    vidmm_gart_table_info_t *gart_table_L2 = adapter->gart_table_L2;
    vidmm_mgr_t *mm_mgr = adapter->mm_mgr;

    if (mm_mgr->chip_func->restore != NULL)
    {
        mm_mgr->chip_func->restore(adapter);
    }

    if (gart_table_L2->backup)
    {
        gf_memcpy(gart_table_L2->virt_addr, gart_table_L2->backup, gart_table_L2->size);
        gf_free(gart_table_L2->backup);
    }

#if GART_BLT_SUSPEND
    if (gart_table_L3->backup)
    {
        struct _vidmm_segment_memory *segment = (struct _vidmm_segment_memory *)gart_table_L3->backup;
        vidmm_map_flags_t flags = {0};
        flags.write_only = 1;
        flags.mem_space = GF_MEM_KERNEL;
        vidmm_map_segment_memory(adapter, NULL, segment, &flags);
        gf_memcpy(gart_table_L3->virt_addr, segment->vma->virt_addr, gart_table_L3->size);
        vidmm_unmap_segment_memory(adapter, segment, GF_MEM_KERNEL);
        vidmm_release_segment_memory(adapter, segment);
    }
#else
    if (gart_table_L3->backup)
    {
        gf_memcpy(gart_table_L3->virt_addr, gart_table_L3->backup, gart_table_L3->size);
        gf_free(gart_table_L3->backup);
    }
#endif

    if (gart_table_L3->backup)
    {
        gf_mutex_lock(adapter->gart_table_lock);
        gart_table_L3->dirty = TRUE;
        gart_table_L3->gart_table_dirty_addr = 0;
        gart_table_L3->gart_table_dirty_mask = -1;
        gf_mutex_unlock(adapter->gart_table_lock);
    }

    if (adapter->fence_buf_local->backup)
    {
        gf_memcpy(adapter->fence_buf_local->reserved_memory->vma->virt_addr, adapter->fence_buf_local->backup, 68 * 1024);
        gf_free(adapter->fence_buf_local->backup);
    }

    if (adapter->fence_buf->backup)
    {
        gf_memcpy(adapter->fence_buf->reserved_memory->vma->virt_addr, adapter->fence_buf->backup, 512 * 1024);
        gf_free(adapter->fence_buf->backup);
    }

    adapter->fence_buf->backup = NULL;
    adapter->fence_buf_local->backup = NULL;
    gart_table_L3->backup = NULL;
    gart_table_L2->backup = NULL;

    return;
}

void vidmm_fill_allocation_info(adapter_t *adapter, vidmm_allocation_t *allocation, gf_open_allocation_t *info)
{
    vidmm_mgr_t *mm_mgr  = adapter->mm_mgr;
    vidmm_segment_t *segment = &mm_mgr->segment[allocation->segment_id];
    vidmm_get_allocation_info_t alloc_info = {0};

    info->device        = allocation->device ? allocation->device->handle : 0;
    info->allocation    = allocation->handle;
    info->size          = allocation->size;
    info->alignment     = allocation->alignment;
    info->width         = allocation->width;
    info->height        = allocation->height;
    info->aligned_width = allocation->aligned_width;
    info->aligned_height= allocation->aligned_height;
    info->tiled_width   = allocation->tiled_width;
    info->tiled_height  = allocation->tiled_height;
    info->bit_cnt       = allocation->bit_count;
    info->pitch         = allocation->pitch;
    info->unpagable     = allocation->flag.unpagable;
    info->tiled         = allocation->flag.swizzled;
    info->secured       = allocation->flag.secured;
    info->local         = allocation->flag.secured;//now force into secure
    info->compress_format   = allocation->compress_format;
    info->hw_format     = allocation->hw_format;
    info->segment_id    = allocation->segment_id;
    info->gpu_virt_addr = allocation->phys_addr;
    info->bl_slot_index = allocation->slot_index;
    info->cpu_visible   = segment->flags.cpu_visible;
    info->force_clear   = allocation->flag.force_clear;
#ifndef VMI_MODE
    if (!allocation->pages_mem)
#endif
    {
        info->cpu_phy_addr = allocation->phys_addr + adapter->vidmm_bus_addr;
    }
    info->sync_obj      = allocation->sync_obj;
    info->fence_addr    = allocation->fence_addr;

    /* get additional information */
    alloc_info.allocation = allocation;
    vidmm_get_allocation_info(adapter, &alloc_info);

    info->ForceLocal    = alloc_info.ForceLocal;
    info->has_pages     = segment->flags.require_system_pages;
    info->snoop         = alloc_info.snoop;
}

int vidmm_query_info(adapter_t *adapter, gf_query_info_t *query)
{
    vidmm_mgr_t *mm_mgr  = adapter->mm_mgr;
    int ret = 0;

    switch (query->type)
    {
    case GF_QUERY_TOTAL_VRAM_SIZE:
        query->value = adapter->Real_vram_size;
        break;

    case GF_QUERY_RESERV_VRAM_SIZE:
        query->value = mm_mgr->reserved_vmem;
        break;

    case GF_QUERY_CPU_VISIBLE_VRAM_SIZE:
        query->value = mm_mgr->cpu_visible_vidmm_size;
        break;

    case GF_QUERY_LOCAL_VRAM_TYPE:
        query->value = GF_SYSTEM_IO;
        break;

    case GF_QUERY_HEIGHT_ALIGN:
        query->value = 64;
        break;

    case GF_QUERY_SEGMENT_FREE_SIZE:
    case GF_QUERY_SEGMENT_MEM_INFO:
        if (mm_mgr->chip_func->query_info != NULL)
            ret = mm_mgr->chip_func->query_info(mm_mgr, query);
        break;

    case GF_QUERY_ALLOCATION_INFO:
    case GF_QUERY_ALLOCATION_INFO_KMD:
    {
        vidmm_allocation_t *allocation = get_from_handle_and_validate(&adapter->hdl_mgr, query->argu, HDL_TYPE_ALLOCATION);
        gf_open_allocation_t  _data = {0};
        gf_open_allocation_t  *info = (query->type == GF_QUERY_ALLOCATION_INFO) ? &_data : query->buf;

        vidmm_fill_allocation_info(adapter, allocation, info);
        if (query->type == GF_QUERY_ALLOCATION_INFO)
        {
            gf_copy_to_user(query->buf, info, sizeof(gf_open_allocation_t));
        }
        break;
    }

    case GF_QUERY_LOCAL_ALLOCATION_MAX_SIZE:
    {
        query->value = vidmmi_query_max_size_of_local_allocation(adapter);
        break;
    }

    case GF_QUERY_DIAGS:
    {
        query->diags.total_vram_size = adapter->Real_vram_size;
        query->diags.cpu_visible_size = adapter->mm_mgr->cpu_visible_vidmm_size;
        break;
    }

    case GF_SET_PERF_SWAP_HINT:
    case GF_GET_PERF_SWAP_HINT:
    {
        vidmm_allocation_t *allocation = get_from_handle_and_validate(&adapter->hdl_mgr, query->argu, HDL_TYPE_ALLOCATION);
        if (query->type == GF_SET_PERF_SWAP_HINT) {
            allocation->perf_swap_hint = query->value;
        } else if (query->type == GF_GET_PERF_SWAP_HINT) {
            query->value = allocation->perf_swap_hint;
        }
        break;
    }

    default:
        ret = -1;
        break;
    }

    return ret;
}

void vidmm_dump_flagbuffer_to_file(char *file_name, vidmm_allocation_t *allocation)
{
    gpu_device_t  *device = allocation->device;
    adapter_t * adapter = device->adapter;
    unsigned int offset = 0;
    unsigned int size   = 0;

    {
        heap_t *blheap    = &adapter->mm_mgr->blheap;
        gf_vm_area_t *vm_area       = NULL;
        gf_map_argu_t map          = {0};

        map.flags.cache_type= GF_MEM_UNCACHED;
        map.flags.mem_space = GF_MEM_KERNEL;
        map.flags.mem_type    = GF_SYSTEM_IO;
        map.phys_addr        = blheap->start + adapter->vidmm_bus_addr;
        map.size        = blheap->size;

        vm_area = gf_map_io_memory(NULL, &map);

        offset = allocation->bl_node->aligned_offset - blheap->start;
        size  = allocation->size/512; //1:512 cpmpress rate

        if(file_name)
        {
            util_dump_raw_memory_to_file(vm_area->virt_addr + offset, size, file_name);
        }
        else
        {
            util_dump_memory(NULL, vm_area->virt_addr + offset, size, "allocation bl:");
        }

        gf_unmap_io_memory(vm_area);
        vm_area = NULL;
    }

    return;
}

void vidmm_dump_allocation_to_file(char *file_name, unsigned int name_offset, unsigned int hAllocation, adapter_t *adapter)
{
    vidmm_allocation_t *allocation     = NULL;
    char                alloc_file_name[256];

    if(file_name == NULL)
    {
        return;
    }

    allocation = get_from_handle(&adapter->hdl_mgr, hAllocation);

    if(allocation == NULL)
    {
        gf_error("get allocaton failed, allocation %d\n", hAllocation);
        return;
    }

    gf_vsprintf(alloc_file_name, "%s_%d_%d_fmt%d.bin", file_name, allocation->aligned_width, allocation->aligned_height, allocation->hw_format);

    if(allocation->pages_mem)
    {
        gf_map_argu_t map        = {0};
        gf_vm_area_t  *pcie_addr = NULL;

        map.flags.mem_space  = GF_MEM_KERNEL;
        map.flags.cache_type = GF_MEM_WRITE_COMBINED;
        map.flags.mem_type   = GF_SYSTEM_RAM;
        map.size             = allocation->orig_size;
        map.memory           = allocation->pages_mem;

        pcie_addr  = gf_map_pages_memory(NULL, &map);

        util_dump_raw_memory_to_file(pcie_addr->virt_addr, allocation->orig_size, alloc_file_name);

        gf_unmap_pages_memory(pcie_addr);
    }
    else
    {
        gf_vm_area_t       *vm_area       = NULL;
        gf_map_argu_t      map            = {0};

        map.flags.cache_type = GF_MEM_UNCACHED;
        map.flags.mem_space  = GF_MEM_KERNEL;
        map.flags.mem_type     = GF_SYSTEM_IO;
        map.phys_addr         = allocation->phys_addr + adapter->vidmm_bus_addr;
        map.size             = allocation->orig_size;

        vm_area = gf_map_io_memory(NULL, &map);

        util_dump_raw_memory_to_file(vm_area->virt_addr, allocation->orig_size, alloc_file_name);

        gf_unmap_io_memory(vm_area);
        vm_area = NULL;
    }

    if(allocation->compress_format)
    {
        vidmm_dump_flagbuffer_to_file(file_name + name_offset, allocation);
    }
}

void vidmm_dump_allocation_content(vidmm_allocation_t *allocation)
{
    gpu_device_t  *device = allocation->device;
    adapter_t * adapter = device->adapter;

    if(allocation->pages_mem)
    {
        gf_map_argu_t map        = {0};
        gf_vm_area_t  *pcie_addr = NULL;

        map.flags.mem_space  = GF_MEM_KERNEL;
        map.flags.cache_type = GF_MEM_WRITE_COMBINED;
        map.flags.mem_type   = GF_SYSTEM_RAM;
        map.size             = allocation->orig_size;
        map.memory           = allocation->pages_mem;

        pcie_addr  = gf_map_pages_memory(NULL, &map);

        util_dump_memory(NULL, pcie_addr->virt_addr, allocation->orig_size, "allocation mem content");

        gf_unmap_pages_memory(pcie_addr);
    }
    else
    {
        gf_vm_area_t       *vm_area       = NULL;
        gf_map_argu_t      map            = {0};

        if (allocation->phys_addr >= adapter->Visible_vram_size)
        {
            gf_error("at_type %d, trying to dump cpu invisible mem!!!\n", allocation->at_type);
            return;
        }

        map.flags.cache_type = GF_MEM_UNCACHED;
        map.flags.mem_space  = GF_MEM_KERNEL;
        map.flags.mem_type   = GF_SYSTEM_IO;
        map.phys_addr         = allocation->phys_addr + adapter->vidmm_bus_addr;
        map.size             = allocation->orig_size;

        vm_area = gf_map_io_memory(NULL, &map);

        util_dump_memory(NULL, vm_area->virt_addr, allocation->orig_size, "allocation mem content");

        gf_unmap_io_memory(vm_area);
        vm_area = NULL;
    }

    return;
}


static void vidmmi_dump_allocations(vidmm_mgr_t *mm_mgr, struct list_head *allocation_list)
{
    vidmm_allocation_t *allocation = NULL, *allocation_next;

    list_for_each_entry_safe(allocation, allocation_next, allocation_list, list_item)
    {
        gf_info("allo:%x, refcount:%d, dev:%x, fmt:%d-%d-%d-%d, slotbuffer_index:%d, flag: 0x%x, W-H-P:%d-%d-%d, size:%6dk, gvm:%llx, AT:0x%x, status:%8x, pid:%d, tid: %d, proc:%s.\n",
            allocation->handle, allocation->ref_count, allocation->device->handle,
            allocation->hw_format, allocation->compress_format, allocation->flag.swizzled, allocation->bit_count,
            allocation->slot_index, allocation->flag,
            allocation->width, allocation->height, allocation->pitch,
            util_align(allocation->orig_size, util_max(allocation->alignment, mm_mgr->adapter->os_page_size)) >> 10,
            allocation->phys_addr, allocation->at_type, allocation->status.temp_unpagable,
            allocation->device->pid, allocation->device->tid, allocation->device->pname);
    }
}

void vidmm_dump_resource(adapter_t *adapter)
{
    vidmm_mgr_t        *mm_mgr     = adapter->mm_mgr;
    vidmm_segment_t    *segment    = NULL;

    unsigned int idx, priority;

    for(idx = 0; idx < mm_mgr->segment_cnt; idx++)
    {
        //if(idx > 2) break;

        segment = &mm_mgr->segment[idx];

        gf_info("\n\n");

        gf_mutex_lock(segment->lock);

        gf_info("**** segment[%d] size: %dK, used:%lldk, allocation_num:%d\n", idx,
             segment->gpu_vm_size >> 10, (segment->pagable_size + segment->unpagable_size + segment->reserved_size) >> 10,
             (segment->pagable_num + segment->unpagable_num));
        gf_info("@@       Heap start: 0x%08x, size: %4dM, free size: %10dk.\n",
            segment->heap.start, segment->heap.size >> 20, segment->heap.free.size >> 10);
        gf_info("@@ Small Heap start: 0x%08x, size: %4dM, free size: %10dk.\n\n",
            segment->small_heap.start, segment->small_heap.size >> 20, segment->small_heap.free.size >> 10);

        gf_info("## unpagable reserved size: %dk.\n", segment->reserved_size >> 10);
        gf_info("## unpagable allocation num: %3d. size: %lldk\n", segment->unpagable_num, segment->unpagable_size >> 10);

        vidmmi_dump_allocations(mm_mgr, &segment->unpagable_resident_list);

        gf_msleep(10);

        gf_info("##pagable allocation num: %3d. size: %lldk\n", segment->pagable_num, segment->pagable_size >> 10);

        for(priority = PDISCARD; priority < PALL; priority++)
        {
            gf_info("----priority: %d.\n", priority);

            vidmmi_dump_allocations(mm_mgr, &segment->pagable_resident_list[priority]);
        }

        gf_mutex_unlock(segment->lock);

        gf_msleep(10);
    }
}

void vidmm_dump_memtrack(struct os_seq_file *seq_file, adapter_t *adapter, int pid)
{
    gpu_device_t     *device       = NULL;
    gpu_device_t     *device_next  = NULL;

    cm_allocation_open_instance_t *instance = NULL, *instance_next;

    unsigned long long total = 0;

    UNUSED(pid);

    gf_seq_printf(seq_file, "Name                               PID              Usage(kB)\n");
    gf_seq_printf(seq_file, "================================================================\n");

    gf_mutex_lock(adapter->device_list_lock);

    list_for_each_entry_safe(device, device_next, &adapter->device_list, list_node)
    {
        unsigned long long mem_size = 0;

        cm_device_lock(device);

        list_for_each_entry_safe(instance, instance_next, &device->allocation_open_instance_list, list_node)
        {
            vidmm_allocation_t  *allocation = instance->allocation;

            if(device == allocation->device)
            {
                mem_size += (util_align(allocation->orig_size, util_max(allocation->alignment, adapter->os_page_size)) >> 10);
            }
        }

        total += mem_size;

        cm_device_unlock(device);

        gf_seq_printf(seq_file, "  %-32s %-16d %-d\n", device->pname, device->pid, mem_size);
    }

    gf_mutex_unlock(adapter->device_list_lock);

    gf_seq_printf(seq_file, "================================================================\n");
    gf_seq_printf(seq_file, "Total                                               %d(kB)\n", total);

}

void vidmm_dump_bl_heap(adapter_t *adapter)
{
    vidmm_mgr_t* mm_mgr  = adapter->mm_mgr;
    heap_t *     pblHeap = &mm_mgr->blheap;

    gf_info("********BL heap start: 0x%llx, size 0x%llx \n********************\n", pblHeap->start, pblHeap->size);

    heap_dump(pblHeap);
}


void vidmm_dump_heap(struct os_seq_file *seq_file, adapter_t *adapter, int id)
{
    vidmm_mgr_t        *mm_mgr     = adapter->mm_mgr;
    gpu_device_t        *device = NULL;
    gpu_device_t        *device_next = NULL;
    vidmm_segment_t    *segment    = NULL;
    unsigned int        priority;

    int cnt = vidmm_get_segment_count(adapter);

    if(id > cnt - 1)
    {
        gf_seq_printf(seq_file, "vidmm_dump_heap_allocation invalid id %x\n", id);
        return;
    }

    segment = &mm_mgr->segment[id];

    gf_seq_printf(seq_file, "\n\n");

    gf_mutex_lock(segment->lock);

    gf_seq_printf(seq_file, "**** segment[%d] size: %dK, used:%lldk, allocation_num:%d\n", id,
         segment->gpu_vm_size >> 10, (segment->pagable_size + segment->unpagable_size + segment->reserved_size) >> 10,
         (segment->pagable_num + segment->unpagable_num));
    gf_seq_printf(seq_file, "@@       Heap start: 0x%08llx, size: %4dM, free size: %10dk.\n",
        segment->heap.start, segment->heap.size >> 20, segment->heap.free.size >> 10);
    gf_seq_printf(seq_file, "@@ Small Heap start: 0x%08llx, size: %4dM, free size: %10dk.\n\n",
        segment->small_heap.start, segment->small_heap.size >> 20, segment->small_heap.free.size >> 10);

    gf_seq_printf(seq_file, "## unpagable reserved size: %dk.\n", segment->reserved_size >> 10);

    gf_mutex_lock(adapter->device_list_lock);

    list_for_each_entry_safe(device, device_next, &adapter->device_list, list_node)
    {
        vidmm_allocation_t *allocation = NULL, *allocation_next = NULL;

        int num=0;
        unsigned long long size=0;

        cm_device_lock(device);

        gf_seq_printf(seq_file, "----------------------------------------------------------------------\n");
        gf_seq_printf(seq_file, "          pid         dev         client\n");
        gf_seq_printf(seq_file, "   %10d    %8x         %s     \n", device->pid, device->handle, device->pname);

        list_for_each_entry_safe(allocation, allocation_next, &segment->unpagable_resident_list, list_item)
        {
            if(allocation->device->handle == device->handle)
            {
                gf_seq_printf(seq_file, "allo: %x, dev: %x, fmt: %3d-%2d-%d-%2d, W-H-P: %8d-%4d-%8d, size: %6dk, gvm: %llx, AT:0x%02x, page:%8x, prefer:%3d-%3d-%3d.\n",
                    allocation->handle, allocation->device->handle,
                    allocation->hw_format, allocation->compress_format, allocation->flag.swizzled, allocation->bit_count,
                    allocation->width, allocation->height, allocation->pitch,
                    util_align(allocation->orig_size, util_max(allocation->alignment, mm_mgr->adapter->os_page_size)) >> 10,
                    allocation->phys_addr, allocation->at_type,
                    allocation->status.temp_unpagable,
                    allocation->preferred_segment.segment_id_0,allocation->preferred_segment.segment_id_1,allocation->preferred_segment.segment_id_2);

                num++;
                size += util_align(allocation->orig_size, util_max(allocation->alignment, mm_mgr->adapter->os_page_size));
            }
        }

        allocation      = NULL;
        allocation_next = NULL;

        for(priority = PDISCARD; priority < PALL; priority++)
        {
            gf_seq_printf(seq_file, "----priority: %d.\n", priority);

            list_for_each_entry_safe(allocation, allocation_next, &segment->pagable_resident_list[priority], list_item)
            {
                if(allocation->device->handle == device->handle)
                {
                    gf_seq_printf(seq_file, "allo: %x, dev: %x, fmt: %3d-%2d-%d-%2d, W-H-P: %8d-%4d-%8d, size: %6dk, gvm: %llx, AT:0x%02x, page:%8x, prefer:%3d-%3d-%3d.\n",
                        allocation->handle, allocation->device->handle,
                        allocation->hw_format, allocation->compress_format, allocation->flag.swizzled, allocation->bit_count,
                        allocation->width, allocation->height, allocation->pitch,
                        util_align(allocation->orig_size, util_max(allocation->alignment, mm_mgr->adapter->os_page_size)) >> 10,
                        allocation->phys_addr, allocation->at_type,
                        allocation->status.temp_unpagable,
                        allocation->preferred_segment.segment_id_0,allocation->preferred_segment.segment_id_1,allocation->preferred_segment.segment_id_2);

                    num++;
                    size += util_align(allocation->orig_size, util_max(allocation->alignment, mm_mgr->adapter->os_page_size));
                }
            }

        }

        cm_device_unlock(device);

        gf_seq_printf(seq_file, "  allocation num %d, size %dk               \n\n", num, size >> 10);
    }

    gf_seq_printf(seq_file, "----------------------------------------------------------------------\n");

    gf_mutex_unlock(adapter->device_list_lock);

    gf_mutex_unlock(segment->lock);
#if DEBUG_HIGH_4G_MEM
    if((id == 1) && (adapter->Reserve_Buffer != NULL))
    {
        vidmm_segment_memory_t  *reserved_memory  = adapter->Reserve_Buffer;

        util_dump_memory(reserved_memory->vma->virt_addr, HIGH_4G_MEM_RESERVE_SIZE, "mem reserved for high 4g test");
    }
#endif

    vidmm_dump_bl_heap(adapter);
}

void* vidmm_get_from_gem_handle(adapter_t *adapter, gpu_device_t *device, unsigned int gem_handle)
{
    unsigned int handle;
    void *ret = NULL;

#ifdef VMI_MODE
    handle = gem_handle;
    ret = get_from_handle(&adapter->hdl_mgr, handle);
#else
    if (adapter->drm_cb)
    {
        handle = adapter->drm_cb->gem.get_from_handle(device->filp, gem_handle);
        if (handle)
        {
            ret = get_from_handle(&adapter->hdl_mgr, handle);
        }
    }
#endif

    return ret;
}

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
#include "kernel_interface.h"

void glb_init_chip_id(adapter_t *adapter, krnl_adapter_init_info_t *info)
{
    adapter->hw_caps.support_snooping = TRUE;
    adapter->max_present_count = DEFAULT_PRESENT_COUNT;
    adapter->pm_caps.pwm_mode = info->gf_pwm_mode;

    {
        adapter->family_id = FAMILY_ELT;
        adapter->generic_id= PCI_ID_ARISE;

        adapter->hw_caps.support_snooping = TRUE;
        //from loongson, loonson 3A3000 not support snoop
#if defined(__mips64__) || defined(__loongarch__)
        adapter->hw_caps.support_snooping = FALSE;
#endif
        adapter->hw_caps.page_4k_enable   = TRUE;

        adapter->pm_caps.pwm_manual       = FALSE;

        //for elite3000, only cg manual mode is supported.
        //gf_pwm_mode == 1 means enable cg manual mode.
        if (info->gf_pwm_mode == 1)
        {
            adapter->pwm_level.EnableClockGating = 1;
        }

        adapter->hw_caps.hw_patch_enable  = FALSE;
#ifndef GF_HW_NULL
        adapter->hw_caps.local_only       = FALSE;
#else
        adapter->hw_caps.local_only       = FALSE;
#endif

#ifdef VIDEO_ONLY_FPGA
        adapter->hw_caps.video_only        = TRUE;
#else
        adapter->hw_caps.video_only        = FALSE;
#endif

#ifdef GFX_ONLY_FPGA
        adapter->hw_caps.gfx_only          = TRUE;
#else
        adapter->hw_caps.gfx_only          = FALSE;
#endif

        adapter->hw_caps.svm_enable        = TRUE;

        if(info->gf_dfs_mode == 1)
        {
            adapter->hw_caps.dfs_enable    =  TRUE;
        }

        if (adapter->hw_caps.video_only)
        {
            adapter->ctl_flags.paging_enable = FALSE;
            adapter->ctl_flags.worker_thread_enable = FALSE;
        }
        else
        {
            adapter->ctl_flags.paging_enable        = TRUE;
            adapter->ctl_flags.worker_thread_enable = info->gf_worker_thread_enable;
        }

        adapter->ctl_flags.split_enable         = TRUE;
        adapter->ctl_flags.rename_enable        = TRUE;
        adapter->ctl_flags.one_shot_enable      = FALSE;

        adapter->ctl_flags.hotplug_thread_enable  = TRUE;
        adapter->ctl_flags.hotplug_polling_enable = info->gf_hotplug_polling_enable ? TRUE : FALSE;
        adapter->ctl_flags.compare_edid           = TRUE;
        adapter->ctl_flags.reboot_patch_enable    = info->gf_reboot_patch ? TRUE : FALSE;

        adapter->ctl_flags.vesa_tempbuffer_enable = info->gf_vesa_tempbuffer_enable ? TRUE : FALSE;

        adapter->ctl_flags.force_3dblt = info->gf_force_3dblt ? TRUE : FALSE;
        if((adapter->bus_config.device_id & CHIP_MASK) == CHIP_MASK_ARISE1020)
            adapter->chip_id = CHIP_ARISE1020;
        else 
            adapter->chip_id = CHIP_ARISE;

        adapter->max_present_count = 6;
        adapter->hw_patch_mask0 |= PATCH_FENCE_INTERRUPT_LOST;
    }

    adapter->ctl_flags.recovery_enable = info->gf_recovery_enable;
    adapter->ctl_flags.run_on_qt = info->gf_run_on_qt;

    if(info->gf_hang_dump)
    {
        adapter->ctl_flags.hang_dump            = info->gf_hang_dump;
        adapter->ctl_flags.recovery_enable      = TRUE;
        adapter->ctl_flags.flag_buffer_verify   = info->gf_flag_buffer_verify;
        adapter->Real_vram_size = ((unsigned long long)info->gf_local_size_g << 30) + ((unsigned long long)info->gf_local_size_m << 20);
        adapter->pcie_vram_size = ((unsigned long long)info->gf_pcie_size_g << 30) + ((unsigned long long)info->gf_pcie_size_m << 20);
        gf_info("hang dump avaiable, set local vram size: %d M, visible vram size: %d M , pcie vram size:%d M\n", adapter->Real_vram_size >> 20, adapter->Visible_vram_size >> 20, adapter->pcie_vram_size >> 20);
    }
    else
    {
        adapter->ctl_flags.hang_dump            = 0;
        adapter->Real_vram_size                 = 0;
        adapter->pcie_vram_size                 = 0;
    }

    if(info->gf_debug_secure)
    {
        adapter->ctl_flags.debug_secure             = info->gf_debug_secure;
        adapter->hw_caps.secure_range_enable        = TRUE;
    }
    else if(adapter->sys_caps.secure_on)
    {
        adapter->hw_caps.secure_range_enable        = TRUE;
        adapter->ctl_flags.debug_secure             =	FALSE;	 
    }
    else
    {
        adapter->ctl_flags.debug_secure            = FALSE;
        adapter->hw_caps.secure_range_enable       = FALSE;
    }

#ifdef _WRS_KERNEL
    adapter->hw_caps.gfx_only                = TRUE;
    adapter->hw_caps.secure_range_enable     = FALSE;
    adapter->ctl_flags.worker_thread_enable  = FALSE;
    adapter->ctl_flags.paging_enable         = FALSE;
    adapter->ctl_flags.rename_enable         = FALSE;
    adapter->ctl_flags.split_enable          = FALSE;
    adapter->ctl_flags.hotplug_thread_enable = FALSE;
    adapter->ctl_flags.perf_event_enable     = FALSE;
#ifdef GF_HW_NULL
	adapter->hw_caps.local_only              = FALSE;
#endif
#endif
    adapter->hw_caps.miu_channel_size   = info->miu_channel_size;

    //gf_info("adapter->ctl_flags.worker_thread_enable %x\n", adapter->ctl_flags.worker_thread_enable);
    //gf_info("hw caps: secure:%d, snoop:%d, 4K_page:%d, Paging:%d, local_only:%d, CG:%d, DFS:%d.\n",
    //    adapter->hw_caps.secure_range_enable, adapter->hw_caps.support_snooping, adapter->hw_caps.page_4k_enable,
    //    adapter->ctl_flags.paging_enable,
    //    adapter->hw_caps.local_only, adapter->pwm_level.EnableClockGating, adapter->hw_caps.dfs_enable);

    glb_init_chip_interface(adapter);
}

void glb_get_pci_config_info(adapter_t *adapter)
{
    unsigned long long mmio;
    unsigned long long fb;
    unsigned long  claim_fb_size;
    unsigned short link_status;
    bus_config_t*  bus_config = &adapter->bus_config;
    gf_map_argu_t map    = {0};
    unsigned char  cache_type  = 0;
    alloc_pages_flags_t alloc_flags = {0};

    gf_get_bus_config(adapter->os_device.pdev, bus_config);

    adapter->sys_caps.secure_on = bus_config->secure_on;

    claim_fb_size = bus_config->mem_end_addr[0] - bus_config->mem_start_addr[0] + 1;

//    gf_info("bus claimed cpu access-able vram size: 0x%x, %dM, secure_on: %d\n", 
//        claim_fb_size, claim_fb_size >> 20, adapter->sys_caps.secure_on);

#if defined(__i386__) || defined(__x86_64__) || defined(__mips64__) || defined(__loongarch__)
    adapter->primary = (bus_config->command & 0x01) ? 1 : 0;
#else
    adapter->primary = 0;
#endif

    adapter->link_width = (bus_config->link_status>>4) & 0x1F;

    mmio = bus_config->reg_start_addr[0];
    fb   = bus_config->mem_start_addr[0] & (~0xFF);/*The base address not right under QNX*/

#if defined(GF_PCIE_BUS)
    map.flags.cache_type = GF_MEM_UNCACHED;
    map.flags.mem_space  = GF_MEM_KERNEL;
    map.flags.mem_type   = GF_SYSTEM_IO;
    map.phys_addr        = mmio;
    map.size             = bus_config->reg_end_addr[0] - bus_config->reg_start_addr[0];

    adapter->mmio_vma = gf_map_io_memory(NULL, &map);

    adapter->mmio     = adapter->mmio_vma->virt_addr;
    adapter->bci_base = (unsigned int*)(adapter->mmio + 0x10000);

    adapter->vidmm_bus_addr = fb;

    adapter->physical_bus_base_length[0] = bus_config->reg_end_addr[0] - bus_config->reg_start_addr[0];
    adapter->physical_bus_base_length[1] = claim_fb_size;

    /*adapter->Real_vram_size will setup later by update adapter info*/
    adapter->Visible_vram_size = adapter->physical_bus_base_length[1];
#elif defined(GF_HW_NULL)
    //cache_type = GF_MEM_UNCACHED;
    //alloc_flags.cache_type = cache_type;

    alloc_flags.need_flush = 1;
    alloc_flags.need_zero  = 1;
    alloc_flags.fixed_page = 1;

    map.memory = gf_allocate_pages_memory(0x80000, adapter->os_page_size, alloc_flags);

    if(map.memory == NULL)
    {
        gf_error("allocate command buffer fail: out of memory!\n");
    }

    map.flags.mem_space  = GF_MEM_KERNEL;
    map.flags.mem_type   = GF_SYSTEM_RAM;
    map.size             = 0x80000;

    adapter->mmio_vma = gf_map_pages_memory(NULL, &map);
    adapter->mmio_mem = map.memory;

    adapter->mmio     = adapter->mmio_vma->virt_addr;
    adapter->bci_base = (unsigned int*)(adapter->mmio + 0x10000);
    adapter->vidmm_bus_addr = fb;

    adapter->pmu_mmio_vma = gf_map_pages_memory(NULL, &map);
    adapter->pmu_mmio     = adapter->pmu_mmio_vma->virt_addr;

    adapter->physical_bus_base_length[0] = 0x80000;
    adapter->physical_bus_base_length[1] = claim_fb_size;

    adapter->Visible_vram_size     = 
    adapter->Real_vram_size                = claim_fb_size;

    gf_info("video memory size: %dM.\n", claim_fb_size >> 20);
    gf_info("mmio virt addr: %x.\n", adapter->mmio);

#else
    //engine mmio
    map.flags.cache_type = GF_MEM_UNCACHED;
    map.flags.mem_space  = GF_MEM_KERNEL;
    map.flags.mem_type   = GF_SYSTEM_IO;
    map.phys_addr        = mmio;
    map.size             = bus_config->reg_end_addr[0] - bus_config->reg_start_addr[0];

    adapter->mmio_vma    = gf_map_io_memory(NULL, &map);
    adapter->mmio        = adapter->mmio_vma->virt_addr;
    adapter->bci_base    = (unsigned int*)(adapter->mmio + 0x10000);
    adapter->vidmm_bus_addr = fb;

    //pmu mmio
    map.phys_addr        = bus_config->reg_start_addr[3];
    map.size             = bus_config->reg_end_addr[3] - bus_config->reg_start_addr[3];

    adapter->pmu_mmio_vma = gf_map_io_memory(NULL, &map);
    adapter->pmu_mmio     = adapter->pmu_mmio_vma->virt_addr;
    adapter->pmu_phys_addr= bus_config->reg_start_addr[3];

    //gpio
    map.phys_addr        = bus_config->reg_start_addr[2];
    map.size             = bus_config->reg_end_addr[2] - bus_config->reg_start_addr[2];

    adapter->gpio_vma    = gf_map_io_memory(NULL, &map);
    adapter->gpio        = adapter->gpio_vma->virt_addr;

    //pmc
    map.phys_addr        = bus_config->reg_start_addr[1];
    map.size             = bus_config->reg_end_addr[1] - bus_config->reg_start_addr[1];

    adapter->pmc_vma      = gf_map_io_memory(NULL, &map);
    adapter->pmc          = adapter->pmc_vma->virt_addr;
    adapter->pmc_phys_addr= bus_config->reg_start_addr[1];

    adapter->physical_bus_base_length[0] = 0x80000;
    adapter->physical_bus_base_length[1] = claim_fb_size;

    gf_info("sys_secure_on: %d, mmio: %x-%d-%p, pmu %x-%d-%p, gpio %x-%d-%p, pmc %x-%d-%p\n",
        bus_config->secure_on, 
        bus_config->reg_end_addr[0], bus_config->reg_end_addr[0] - bus_config->reg_start_addr[0], adapter->mmio,
        bus_config->reg_end_addr[3], bus_config->reg_end_addr[3] - bus_config->reg_start_addr[3], adapter->pmu_mmio,
        bus_config->reg_end_addr[2], bus_config->reg_end_addr[2] - bus_config->reg_start_addr[2], adapter->gpio,
        bus_config->reg_end_addr[1], bus_config->reg_end_addr[1] - bus_config->reg_start_addr[1], adapter->pmc);

#endif
}
void glb_fini_bus_config(adapter_t *adapter)
{
    if(adapter->mmio_vma)
    {
        gf_unmap_io_memory(adapter->mmio_vma);
    }

    if(adapter->pmu_mmio_vma)
    {
        gf_unmap_io_memory(adapter->pmu_mmio_vma);
    }

    if(adapter->pmc_vma)
    {
        gf_unmap_io_memory(adapter->pmc_vma);
    }
}


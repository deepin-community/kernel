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
#include "kernel_interface.h"

void glb_init_chip_id(adapter_t *adapter, krnl_adapter_init_info_t *info)
{
    unsigned int enabled;

    adapter->hw_caps.support_snooping = TRUE;
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

        adapter->ctl_flags.split_enable         = FALSE;

        adapter->ctl_flags.vesa_tempbuffer_enable = info->gf_vesa_tempbuffer_enable ? TRUE : FALSE;

        if((adapter->bus_config.device_id & CHIP_MASK) == CHIP_MASK_ARISE1020)
            adapter->chip_id = CHIP_ARISE1020;
        else if((adapter->bus_config.device_id & CHIP_MASK) == CHIP_MASK_ARISE1040)
            adapter->chip_id = CHIP_ARISE1040;
        else if((adapter->bus_config.device_id & CHIP_MASK) == CHIP_MASK_ARISE1010)
            adapter->chip_id = CHIP_ARISE1010;
        else if((adapter->bus_config.device_id & CHIP_MASK) == CHIP_MASK_ARISE10C0T)
            adapter->chip_id = CHIP_ARISE10C0T;
        else if((adapter->bus_config.device_id & CHIP_MASK) == CHIP_MASK_ARISE10D0)
            adapter->chip_id = CHIP_ARISE10C0T;
        else if((adapter->bus_config.device_id & CHIP_MASK) == CHIP_MASK_ARISE2030)
            adapter->chip_id = CHIP_ARISE2030;
        else if((adapter->bus_config.device_id & CHIP_MASK) == CHIP_MASK_ARISE2020)
            adapter->chip_id = CHIP_ARISE2020;
        else
            adapter->chip_id = CHIP_ARISE;

        adapter->hw_patch_mask0 |= PATCH_FENCE_INTERRUPT_LOST;

        adapter->pm_caps.pwm_manual = FALSE;

        //for elite3000, only cg manual mode is supported.
        //gf_pwm_mode == 1 means enable cg manual mode.
        if (info->gf_pwm_mode)
        {
            unsigned int cg_manual_mode          = info->gf_pwm_mode & 0x1;
            unsigned int cg_auto_mode            = (info->gf_pwm_mode >> 4) & 0x7;
            unsigned int power_switch_mode       = (info->gf_pwm_mode >> 8) & 0x1;

            if (adapter->chip_id < CHIP_ARISE2030)
            {
                adapter->pwm_level.EnableClockGating = cg_manual_mode ? 1 : 0;
            }
            else
            {
                adapter->pm_caps.pwm_auto            = cg_auto_mode;
                adapter->pwm_level.EnableClockGating = cg_auto_mode ? 0 : (cg_manual_mode ? 1 : 0);
            }

            if (power_switch_mode)
            {
                enabled = glb_detect_power_switch(adapter);
                if (enabled)
                {
                    adapter->ctl_flags.perf_event_enable = TRUE;
                    adapter->ctl_flags.hwq_event_enable = TRUE;
                }

                adapter->pwm_level.EnablePowerSwitch = enabled;
            }
            else
            {
                adapter->pwm_level.EnablePowerSwitch = 0;
            }
        }
    }

    adapter->ctl_flags.recovery_enable = info->gf_recovery_enable;
    adapter->ctl_flags.run_on_qt = info->gf_run_on_qt;
    // qemu will set default subsystem id to PCI_SUBVENDOR_ID_REDHAT_QUMRANET:PCI_SUBDEVICE_ID_QEMU
    adapter->ctl_flags.run_on_qemu_device = adapter->bus_config.sub_sys_vendor_id == 0x1AF4 && adapter->bus_config.sub_sys_id == 0x1100;
    adapter->ctl_flags.hang_dump            = info->gf_hang_dump;
    adapter->ctl_flags.boost                = TRUE;

    adapter->power_state = ENG_POWER_STATE_E0;
    adapter->power_state_hold = FALSE;

    if(info->gf_hang_dump)
    {
        adapter->ctl_flags.recovery_enable      = TRUE;
        adapter->ctl_flags.flag_buffer_verify   = info->gf_flag_buffer_verify;

        if (info->gf_hang_dump == 3)
        {
            adapter->ctl_flags.paging_enable    = FALSE;
        }

        adapter->Real_vram_size = ((unsigned long long)info->gf_local_size_g << 30) + ((unsigned long long)info->gf_local_size_m << 20);
        adapter->gart_ram_size = ((unsigned long long)info->gf_pcie_size_g << 30) + ((unsigned long long)info->gf_pcie_size_m << 20);
        gf_info("hang dump avaiable, set local vram size: %d M, visible vram size: %d M , pcie vram size:%d M\n", adapter->Real_vram_size >> 20, adapter->Visible_vram_size >> 20, adapter->gart_ram_size >> 20);
    }
#ifndef VMI_MODE
    else
    {
        adapter->Real_vram_size                 = 0;
        adapter->gart_ram_size                 = 0;
    }
#endif

    if(adapter->sys_caps.secure_on)
    {
        adapter->hw_caps.secure_range_enable        = TRUE;
    }
    else
    {
        adapter->hw_caps.secure_range_enable       = FALSE;
    }

    adapter->hw_caps.miu_channel_size   = info->miu_channel_size;
#if defined(__mips64__) || defined(__loongarch__)
    adapter->hw_caps.gf_backdoor_enable = info->gf_backdoor_enable && adapter->chip_id < CHIP_ARISE1020 && adapter->link_speed != 1;
#else
    adapter->hw_caps.gf_backdoor_enable = info->gf_backdoor_enable && adapter->chip_id < CHIP_ARISE1020 && adapter->link_speed != 1 && adapter->link_speed != 2;
#endif
    adapter->hw_caps.chip_slice_mask    = info->chip_slice_mask;
#if defined(VMI_MODE)
     adapter->hw_caps.miu_channel_num    = 1;
#endif

    adapter->debugfs_mask = info->debugfs_mask;

    gf_info("%s() debugfs_mask-0x%x\n", __func__, adapter->debugfs_mask);

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
    bus_config_t*  bus_config = &adapter->bus_config;
    gf_map_argu_t map    = {0};

    gf_get_bus_config(adapter->os_device.pdev, bus_config);

    gf_info("bus_command: 0x%x, device id: 0x%x\n", bus_config->command, bus_config->device_id);

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
    adapter->link_speed = bus_config->link_status & 0xF;

    mmio = bus_config->reg_start_addr[0];
    fb   = bus_config->mem_start_addr[0] & (~0xFF);/*The base address not right under QNX*/

#if defined(VMI_MODE)
     adapter->mmio     = 0; //mmio is set 0, because cmodel knows virt_addr of mmio
     adapter->bci_base = (unsigned int*)(adapter->mmio + 0x10000);

     adapter->vidmm_bus_addr = fb;

     adapter->physical_bus_base_length[0] = bus_config->reg_end_addr[0] - bus_config->reg_start_addr[0];
     adapter->physical_bus_base_length[1] = claim_fb_size;
     adapter->Visible_vram_size = adapter->physical_bus_base_length[1];

     //Real_vram_size is transferred to core, by mem_start_addr[1]/mem_end_addr[1]
     adapter->Real_vram_size    = bus_config->mem_end_addr[1] - bus_config->mem_start_addr[1] + 1;

     gf_info("video memory size: %dM.\n", adapter->Visible_vram_size >> 20);
#elif defined(GF_PCIE_BUS)
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
    alloc_pages_flags_t alloc_flags = {0};

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

    adapter->physical_bus_base_length[0] = 0x80000;
    adapter->physical_bus_base_length[1] = claim_fb_size;

    gf_info("sys_secure_on: %d, mmio: %x-%d-%p\n",
        bus_config->secure_on,
        bus_config->reg_end_addr[0], bus_config->reg_end_addr[0] - bus_config->reg_start_addr[0], adapter->mmio);

#endif
}
void glb_fini_bus_config(adapter_t *adapter)
{
    if(adapter->mmio_vma)
    {
        gf_unmap_io_memory(adapter->mmio_vma);
    }
}


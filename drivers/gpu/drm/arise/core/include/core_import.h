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

#ifndef __CORE_IMPORT_H__
#define __CORE_IMPORT_H__

#include "kernel_import.h"

#define   GF_PAGE_64KB_SIZE 0x10000

extern krnl_import_func_list_t *gf;

/* CORE referenced funcs list */

#define gf_udelay                        gf->udelay
#define gf_begin_timer                   gf->begin_timer
#define gf_end_timer                     gf->end_timer
#define gf_do_div                        gf->do_div
#define gf_msleep                        gf->msleep
#define gf_getsecs                       gf->getsecs
#define gf_get_nsecs                     gf->get_nsecs
#define gf_assert                        gf->assert
#define gf_copy_from_user                gf->copy_from_user
#define gf_copy_to_user                  gf->copy_to_user
#define gf_slowbcopy_tobus               gf->slowbcopy_tobus
#define gf_slowbcopy_frombus             gf->slowbcopy_frombus
#define gf_memset                        gf->memset
#define gf_memcpy                        gf->memcpy
#define gf_memcmp                        gf->memcmp_priv
#define gf_byte_copy                     gf->byte_copy
#define gf_strcmp                        gf->strcmp
#define gf_strcpy                        gf->strcpy
#define gf_strncmp                       gf->strncmp
#define gf_strncpy                       gf->strncpy
#define gf_strlen                        gf->strlen
#define gf_read64                        gf->read64
#define gf_read32                        gf->read32
#define gf_read16                        gf->read16
#define gf_read8                         gf->read8
#define gf_write32                       gf->write32
#define gf_write16                       gf->write16
#define gf_write8                        gf->write8
#define gf_file_open                     gf->file_open
#define gf_file_close                    gf->file_close
#define gf_file_read                     gf->file_read
#define gf_file_write                    gf->file_write
#define gf_vsprintf                      gf->vsprintf
#define gf_vsnprintf                     gf->vsnprintf
#define gf_sscanf                        gf->sscanf
#define gf_printk                        gf->printk
#define gf_cb_printk                     gf->cb_printk
#define gf_seq_printf                    gf->seq_printf
#define gf_printf                        gf->os_printf
#define gf_find_first_zero_bit           gf->find_first_zero_bit
#define gf_find_next_zero_bit            gf->find_next_zero_bit
#define gf_set_bit                       gf->set_bit
#define gf_clear_bit                     gf->clear_bit
#define gf_getsecs                       gf->getsecs
#define gf_get_nsecs                     gf->get_nsecs
#define gf_create_thread                 gf->create_thread
#define gf_destroy_thread                gf->destroy_thread
#define gf_thread_should_stop            gf->thread_should_stop
#define gf_create_atomic                 gf->create_atomic
#define gf_destroy_atomic                gf->destroy_atomic
#define gf_atomic_read                   gf->atomic_read
#define gf_atomic_inc                    gf->atomic_inc
#define gf_atomic_dec                    gf->atomic_dec
#define gf_atomic_add                    gf->atomic_add
#define gf_atomic_sub                    gf->atomic_sub
#define gf_create_mutex                  gf->create_mutex
#define gf_destroy_mutex                 gf->destroy_mutex
#define gf_mutex_lock                    gf->mutex_lock
#define gf_mutex_lock_killable           gf->mutex_lock_killable
#define gf_mutex_trylock                 gf->mutex_trylock
#define gf_mutex_unlock                  gf->mutex_unlock
#define gf_create_sema                   gf->create_sema
#define gf_destroy_sema                  gf->destroy_sema
#define gf_down                          gf->down
#define gf_down_trylock                  gf->down_trylock
#define gf_up                            gf->up
#define gf_create_rwsema                 gf->create_rwsema
#define gf_destroy_rwsema                gf->destroy_rwsema
#define gf_down_read                     gf->down_read
#define gf_down_write                    gf->down_write
#define gf_up_read                       gf->up_read
#define gf_up_write                      gf->up_write
#define gf_create_spinlock               gf->create_spinlock
#define gf_destroy_spinlock              gf->destroy_spinlock
#define gf_spin_lock                     gf->spin_lock
#define gf_spin_try_lock                 gf->spin_try_lock
#define gf_spin_unlock                   gf->spin_unlock
#define gf_spin_lock_irqsave             gf->spin_lock_irqsave
#define gf_spin_unlock_irqrestore        gf->spin_unlock_irqrestore
#define gf_create_event                  gf->create_event
#define gf_destroy_event                 gf->destroy_event
#define gf_wait_event_thread_safe        gf->wait_event_thread_safe
#define gf_wait_event                    gf->wait_event
#define gf_wake_up_event                 gf->wake_up_event
#define gf_thread_wait                   gf->thread_wait
#define gf_create_wait_queue         gf->create_wait_queue         
#define thread_wait_for_events       gf->wait_for_events
#define gf_thread_wake_up                gf->thread_wake_up
#define general_thread_wake_up       gf->_thread_wake_up
#define gf_dump_stack                    gf->dump_stack
#define gf_try_to_freeze                 gf->try_to_freeze
#define gf_freezable                     gf->freezable
#define gf_clear_freezable               gf->clear_freezable
#define gf_set_freezable                 gf->set_freezable
#define gf_freezing                      gf->freezing
#define gf_get_current_pid               gf->get_current_pid
#define gf_get_current_tid               gf->get_current_tid
#define gf_flush_cache                   gf->flush_cache
#define gf_inv_cache                     gf->inv_cache
#define gf_pages_memory_for_each_continues      gf->pages_memory_for_each_continues
#define gf_get_page_count                gf->get_page_count
#define gf_fill_continues_page_memory    gf->fill_continues_page_memory
#define gf_allocate_pages_memory_dynamic gf->allocate_pages_memory_dynamic
#define gf_fill_pages_memory_dynamic     gf->fill_pages_memory_dynamic
#define gf_free_pages_memory_dynamic     gf->free_pages_memory_dynamic
#define gf_ioremap                       gf->ioremap
#define gf_iounmap                       gf->iounmap_priv
#define gf_mtrr_add                      gf->mtrr_add
#define gf_mtrr_del                      gf->mtrr_del
#define gf_get_mem_info                  gf->get_mem_info
#define gf_register_shrinker             gf->register_shrinker
#define gf_unregister_shrinker           gf->unregister_shrinker
#define gf_pages_memory_swapout          gf->pages_memory_swapout
#define gf_pages_memory_swapin           gf->pages_memory_swapin
#define gf_release_file_storage          gf->release_file_storage
#define gf_get_bus_config                gf->get_bus_config
#define gf_get_platform_config           gf->get_platform_config
#define gf_get_command_status16          gf->get_command_status16
#define gf_write_command_status16        gf->write_command_status16
#define gf_get_command_status32          gf->get_command_status32
#define gf_write_command_status32        gf->write_command_status32
#define gf_get_rom_start_addr            gf->get_rom_start_addr
#define gf_get_rom_save_addr             gf->get_rom_save_addr
#define gf_write_rom_save_addr           gf->write_rom_save_addr
#define gf_get_bar1                      gf->get_bar1
#define gf_logmsg_init                   gf->logmsg_init
#define gf_logmsg                        gf->logmsg
#define gf_logmsg_deinit                 gf->logmsg_deinit
#define gf_disp_wait_idle                gf->disp_wait_idle


#define gf_enable_interrupt              gf->enable_interrupt
#define gf_disable_interrupt             gf->disable_interrupt

#define gf_mb                            gf->mb
#define gf_rmb                           gf->rmb
#define gf_wmb                           gf->wmb
#define gf_flush_wc                      gf->flush_wc
#define gf_dsb                           gf->dsb
#define gf_is_own_pages                  gf->is_own_pages

#if GF_MALLOC_TRACK
#define gf_malloc(size)                  gf->malloc_track(size, __FILE__, __LINE__)
#define gf_calloc(size)                  gf->calloc_track(size, __FILE__, __LINE__)
#define gf_free(addr)                    gf->free_track(addr, __FILE__, __LINE__)
#else
#define gf_malloc(size)                  gf->malloc_priv(size)
#define gf_calloc(size)                  gf->calloc_priv(size)
#define gf_free(addr)                    gf->free_priv(addr)
#endif

#if GF_ALLOC_PAGE_TRACK
#define gf_allocate_pages_memory(pdev, size, page_size, flag) \
        gf->allocate_pages_memory_track(pdev, size, page_size, flag, __FILE__, __LINE__)
#define gf_free_pages_memory(pdev, memory)         \
        gf->free_pages_memory_track(pdev, memory, __FILE__, __LINE__)
#else
#define gf_allocate_pages_memory(pdev, size, page_size, flag) \
        gf->allocate_pages_memory_priv(pdev, size, page_size,  flag)
#define gf_free_pages_memory(pdev, memory)         \
        gf->free_pages_memory_priv(pdev, memory)
#endif

#if GF_MAP_PAGES_TRACK
#define gf_map_pages_memory(priv, argu)     \
        gf->map_pages_memory_track(priv, argu, __FILE__, __LINE__)
#define gf_unmap_pages_memory(map) \
        gf->unmap_pages_memory_track(map, __FILE__, __LINE__)
#else
#define gf_map_pages_memory(priv, argu)     \
        gf->map_pages_memory_priv(priv, argu)
#define gf_unmap_pages_memory(vma) \
        gf->unmap_pages_memory_priv(vma)
#endif

#if GF_MAP_IO_TRACK
#define gf_map_io_memory(priv, argu) \
        gf->map_io_memory_track(priv, argu, __FILE__, __LINE__)
#define gf_unmap_io_memory(argu) \
        gf->unmap_io_memory_track(argu, __FILE__, __LINE__)
#else
#define gf_map_io_memory(priv, argu) \
        gf->map_io_memory_priv(priv, argu)
#define gf_unmap_io_memory(argu) \
        gf->unmap_io_memory_priv(argu)
#endif

#ifdef _DEBUG_
#define GF_MSG_LEVEL GF_DRV_DEBUG
#define gf_debug(args...) gf_printk(GF_DRV_DEBUG, ##args)
#else
#define GF_MSG_LEVEL GF_DRV_INFO
#define gf_debug(args...)
#endif

#define gf_emerg(args...)   gf->printk(GF_DRV_EMERG, ##args)
#define gf_error(args...)   gf->printk(GF_DRV_ERROR, ##args)
#define gf_info(args...)    gf->printk(GF_DRV_INFO,  ##args)
#define gf_warning(args...) gf->printk(GF_DRV_WARNING, ##args)

#define gf_register_trace_events() gf->register_trace_events()
#define gf_unregister_trace_events() gf->unregister_trace_events()
#define gf_task_create_trace_event(engine_index, context, task_id, task_type) \
                gf->task_create_trace_event(engine_index, context, task_id, task_type)
#define gf_task_submit_trace_event(engine_index, context, task_id, task_type, fence_id, args) \
                gf->task_submit_trace_event(engine_index, context, task_id, task_type, fence_id, args)
#define gf_fence_back_trace_event(engine_index, fence_id) gf->fence_back_trace_event(engine_index, fence_id)
#define gf_begin_section_trace_event(desc) gf->begin_section_trace_event(desc)
#define gf_end_section_trace_event(result) gf->end_section_trace_event(result)
#define gf_counter_trace_event(desc, value) gf->counter_trace_event(desc, value)

#define gf_query_platform_caps             gf->query_platform_caps
#define gf_begin_timer                   gf->begin_timer
#define gf_end_timer                     gf->end_timer
#define gf_slowbcopy_tobus               gf->slowbcopy_tobus
#define gf_slowbcopy_frombus             gf->slowbcopy_frombus
#define gf_secure_read32                 gf->secure_read32
#define gf_secure_write32                gf->secure_write32
#define gf_memsetio                      gf->memsetio
#define gf_memcpy_fromio                 gf->memcpy_fromio
#define gf_memcpy_toio                   gf->memcpy_toio
#define gf_console_lock                  gf->console_lock
#endif /*__CORE_IMPORT_H__*/


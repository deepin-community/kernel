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
#ifndef __OS_INTERFACE_H__
#define __OS_INTERFACE_H__

#include "kernel_import.h"

#if !defined(GF_API_CALL)
#define GF_API_CALL
#endif

extern void gf_udelay(unsigned long long usecs_num);
extern unsigned long long gf_do_div(unsigned long long x, unsigned long long y);
extern void gf_msleep(int);
extern void gf_getsecs(long *secs, long *usecs);
extern void gf_get_nsecs(unsigned long long *nsecs);
extern void GF_API_CALL gf_assert(int a, const char *msg);
extern void gf_dump_stack(void);

extern int  gf_find_first_zero_bit(void *buf, unsigned int size);
extern int  gf_find_next_zero_bit(void *buf, unsigned int size, int offset);
extern void gf_set_bit(unsigned int nr, void *buf);
extern void gf_clear_bit(unsigned int nr, void *buf);

extern struct os_atomic *gf_create_atomic(int val);
extern void gf_destroy_atomic(struct os_atomic *atomic);
extern int  gf_atomic_read(struct os_atomic *atomic);
extern void gf_atomic_inc(struct os_atomic *atomic);
extern void gf_atomic_dec(struct os_atomic *atomic);
extern int gf_atomic_add(struct os_atomic *atomic, int v);
extern int gf_atomic_sub(struct os_atomic *atomic, int v);

extern struct os_mutex *gf_create_mutex(void);
extern void  gf_destroy_mutex(struct os_mutex *mutex);
extern void  gf_mutex_lock(struct os_mutex *mutex);
extern int   gf_mutex_lock_killable(struct os_mutex *mutex);
extern int   gf_mutex_trylock(struct os_mutex *mutex);
extern void  gf_mutex_unlock(struct os_mutex *mutex);

extern struct os_sema *gf_create_sema(int value);
extern void  gf_destroy_sema(struct os_sema *sema);
extern void  gf_down(struct os_sema *sema);
extern int   gf_down_trylock(struct os_sema *sema);
extern void  gf_up(struct os_sema *sema);

extern struct os_rwsema *gf_create_rwsema(void);
extern void  gf_destroy_rwsema(struct os_rwsema *sema);
extern void  gf_down_read(struct os_rwsema *sema);
extern void  gf_down_write(struct os_rwsema *sema);
extern void  gf_up_read(struct os_rwsema *sema);
extern void  gf_up_write(struct os_rwsema *sema);

extern struct os_spinlock *gf_create_spinlock(int type);
extern void   gf_destroy_spinlock(struct os_spinlock *spin);
extern void   gf_spin_lock(struct os_spinlock *spin);
extern int   gf_spin_try_lock(struct os_spinlock *spin);
extern void   gf_spin_unlock(struct os_spinlock *spin);
extern unsigned long gf_spin_lock_irqsave(struct os_spinlock *spin);
extern void   gf_spin_unlock_irqrestore(struct os_spinlock *spin, unsigned long flags);

extern int GF_API_CALL gf_copy_from_user(void *to, const void *from, unsigned long size);
extern int GF_API_CALL gf_copy_to_user(void *to, const void *from, unsigned long size);

extern void* GF_API_CALL gf_memset(void *s, int c, unsigned long count);
extern void* GF_API_CALL gf_memcpy(void *d, const void *s, unsigned long count);
extern int   GF_API_CALL gf_memcmp(const void *, const void *, unsigned long);
extern void  GF_API_CALL gf_byte_copy(char* dst, char* src, int len);

extern int   gf_strcmp(const char *, const char *);
extern char* gf_strcpy(char *, const char *);
extern int   gf_strncmp(const char *, const char *, unsigned long);
extern char* gf_strncpy(char *, const char *, unsigned long);
extern unsigned long gf_strlen(char *s);

extern void gf_usleep_range(long min, long max);
extern int gf_get_platform_config(void *dev, const char* config_name, int *buffer, int length);
extern void gf_printf(struct os_printer *p, const char *f, ...);
extern struct os_printer gf_info_printer(void *dev);
extern struct os_printer gf_seq_file_printer(struct os_seq_file *f);
extern unsigned long gf_get_current_pid(void);
extern unsigned long gf_get_current_tid(void);
extern void gf_get_current_pname(char *pname, int size);




/****************************** IO access functions*********************************/
extern unsigned long long GF_API_CALL  gf_read64(void *addr);
extern unsigned int GF_API_CALL  gf_read32(void *addr);
extern unsigned short GF_API_CALL gf_read16(void *addr);
extern unsigned char GF_API_CALL gf_read8(void *addr);
extern void GF_API_CALL  gf_write32(void *addr, unsigned int val);
extern void GF_API_CALL  gf_write16(void *addr, unsigned short val);
extern void GF_API_CALL  gf_write8(void *addr, unsigned char val);

extern int GF_API_CALL gf_vsprintf(char *buf, const char *fmt, ...);
extern int GF_API_CALL gf_vsnprintf(char *buf, unsigned long size, const char *fmt, ...);

extern int GF_API_CALL gf_sscanf(char *buf, char *fmt, ...);

extern void GF_API_CALL gf_printk(unsigned int msglevel, const char* fmt, ...);
extern void GF_API_CALL gf_cb_printk(const char* msg);
extern void GF_API_CALL gf_cb_vdbgprint(int enable, unsigned int print_level, const char* prefix, const char* msg, ...);


#ifdef _DEBUG_
#define GF_MSG_LEVEL GF_DRV_DEBUG
#define gf_debug(args...) gf_printk(GF_DRV_DEBUG, ##args)
#else
#define GF_MSG_LEVEL GF_DRV_INFO
#define gf_debug(args...)
#endif

#define gf_emerg(args...)   gf_printk(GF_DRV_EMERG, ##args)
#define gf_error(args...)   gf_printk(GF_DRV_ERROR, ##args)
#define gf_info(args...)    gf_printk(GF_DRV_INFO,  ##args)
#define gf_warning(args...) gf_printk(GF_DRV_WARNING, ##args)

extern struct os_wait_queue* gf_create_wait_queue(void);
extern struct os_wait_event* gf_create_event(int);
extern void gf_destroy_event(void *event);
extern gf_event_status_t gf_wait_event_thread_safe(struct os_wait_event *event, condition_func_t condition, void *argu, int msec);
extern void gf_wake_up_event(struct os_wait_event *event);
extern gf_event_status_t gf_wait_event(struct os_wait_event *event, int msec);

extern void* gf_create_thread(gf_thread_func_t func, void *data, const char *thread_name);
extern void gf_destroy_thread(void *thread);
extern int  gf_thread_should_stop(void);
extern void gf_thread_wake_up(struct os_wait_event *event);
extern gf_event_status_t gf_thread_wait(struct os_wait_event *event, int msec);

extern struct os_file *gf_file_open(const char *path, int flags, unsigned short mode);
extern void gf_file_close(struct os_file *file);
extern int  gf_file_read(struct os_file *file, void *buf, unsigned long size, unsigned long long *read_pos);
extern int  gf_file_write(struct os_file *file, void *buf, unsigned long size);

extern int  gf_try_to_freeze(void);
extern void gf_set_freezable(void);
extern void gf_old_set_freezable(void);
extern void gf_clear_freezable(void);
extern int  gf_freezing(void);
extern int  gf_freezable(void);

/* os private alloc pages func*/
extern struct os_pages_memory* gf_allocate_pages_memory_priv(void *pdev, int size, int page_size, alloc_pages_flags_t alloc_flags);
extern void gf_free_pages_memory_priv(void *pdev, struct os_pages_memory *memory);
extern gf_vm_area_t *gf_map_pages_memory_priv(void *process_context, gf_map_argu_t *map);
extern void gf_unmap_pages_memory_priv(gf_vm_area_t *vm_area);
extern gf_vm_area_t *gf_map_io_memory_priv(void *process_context, gf_map_argu_t *map);
extern void gf_unmap_io_memory_priv(gf_vm_area_t *map);
extern void gf_pages_memory_for_each_continues(struct os_pages_memory *memory, void *arg,
    int (*cb)(void *arg, int page_start, int page_cnt, unsigned long long dma_addr));

extern void gf_flush_cache(void *pdev, gf_vm_area_t *vma, struct os_pages_memory* memory, unsigned int offset, unsigned int size);
extern void gf_inv_cache(void *pdev, gf_vm_area_t *vma, struct os_pages_memory* memory, unsigned int offset, unsigned int size);
extern int  gf_mtrr_add(unsigned long base, unsigned long size);
extern int  gf_mtrr_del(int reg, unsigned long base, unsigned long size);

/* os private malloc func */
extern void* gf_malloc_priv(unsigned long size);
extern void* gf_calloc_priv(unsigned long size);
extern void  gf_free_priv(void *addr);

extern int gf_get_command_status16(void *dev, unsigned short *command);
extern int gf_get_command_status32(void *dev, unsigned int *command);
extern int gf_write_command_status16(void *dev, unsigned short command);
extern int gf_write_command_status32(void *dev, unsigned int command);
extern int gf_get_rom_save_addr(void *dev, unsigned int *romsave);
extern int gf_write_rom_save_addr(void *dev, unsigned int romsave);
extern int gf_get_bar1(void *dev, unsigned int *bar1);
extern int gf_pci_get_rom(void *dev, void *buf, unsigned int buf_size);

extern void gf_get_bus_config(void *pdev, bus_config_t *bus);
extern unsigned long gf_get_rom_start_addr(void *dev);
extern void         *gf_ioremap( unsigned int io_base, unsigned int size);
extern void          gf_iounmap(void *map_address);

/* mem track func*/
extern void gf_mem_track_init(void);
extern void gf_mem_track_list_result(void);
extern void  gf_mem_leak_list(void);

/* malloc track func*/
extern void* gf_malloc_track(unsigned long size, const char *file, unsigned int line);
extern void* gf_calloc_track(unsigned long size, const char *file, unsigned int line);
extern void  gf_free_track(void *addr, const char *file, unsigned int line);
extern void  gf_mem_leak_list(void);

#if GF_MALLOC_TRACK
#define gf_malloc(size)         gf_malloc_track(size, __FILE__, __LINE__)
#define gf_calloc(size)         gf_calloc_track(size, __FILE__, __LINE__)
#define gf_free(addr)           gf_free_track(addr, __FILE__, __LINE__)
#else
#define gf_malloc(size)         gf_malloc_priv(size)
#define gf_calloc(size)         gf_calloc_priv(size)
#define gf_free(addr)           gf_free_priv(addr)
#endif

/* alloc pages track func*/
extern struct os_pages_memory *gf_allocate_pages_memory_track(void *pdev, int size, int page_size,
                                   alloc_pages_flags_t alloc_flags,
                                   const char *file, unsigned int line);
extern void gf_free_pages_memory_track(void *pdev, struct os_pages_memory *memory,
                                        const char *file, unsigned int line);

#if GF_ALLOC_PAGE_TRACK
#define gf_allocate_pages_memory(pdev, size, flag) \
        gf_allocate_pages_memory_track(pdev, size, flag, __FILE__, __LINE__)
#define gf_free_pages_memory(pdev, memory)         \
        gf_free_pages_memory_track(pdev, memory, __FILE__, __LINE__)
#else
#define gf_allocate_pages_memory(pdev, size, flag) \
        gf_allocate_pages_memory_priv(pdev, size, flag)
#define gf_free_pages_memory(pdev, memory)         \
        gf_free_pages_memory_priv(pdev, memory)
#endif

extern gf_vm_area_t *gf_map_pages_memory_track(void *process_context,
                                        gf_map_argu_t *map,
                                        const char *file, unsigned int line);
extern void gf_unmap_pages_memory_track(gf_vm_area_t *vm_area,
                                         const char *file, unsigned int line);

#if GF_MAP_PAGES_TRACK
#define gf_map_pages_memory(priv, argu)     \
        gf_map_pages_memory_track(priv, argu, __FILE__, __LINE__)
#define gf_unmap_pages_memory(map) \
        gf_unmap_pages_memory_track(map, __FILE__, __LINE__)
#else
#define gf_map_pages_memory(priv, argu)     \
        gf_map_pages_memory_priv(priv, argu)
#define gf_unmap_pages_memory(vma) \
        gf_unmap_pages_memory_priv(vma)
#endif

extern gf_vm_area_t *gf_map_io_memory_track(void *process_context,
                                     gf_map_argu_t *map,
                                     const char *file, unsigned int line);
extern void gf_unmap_io_memory_track(gf_vm_area_t *vm_area,
                                      const char *file, unsigned int line);

#if GF_MAP_IO_TRACK
#define gf_map_io_memory(priv, argu) \
        gf_map_io_memory_track(priv, argu, __FILE__, __LINE__)
#define gf_unmap_io_memory(argu) \
        gf_unmap_io_memory_track(argu, __FILE__, __LINE__)
#else
#define gf_map_io_memory(priv, argu) \
        gf_map_io_memory_priv(priv, argu)
#define gf_unmap_io_memory(argu) \
        gf_unmap_io_memory_priv(argu)
#endif

extern int gf_get_mem_info(mem_info_t *mem);
extern int gf_query_platform_caps(void *pdev, platform_caps_t *caps);

extern void gf_pages_memory_extract_st(struct os_pages_memory *memory);
extern unsigned char gf_validate_page_cache(struct os_pages_memory *memory, int start_page, int end_page, unsigned char request_cache_type);

extern int   gf_pages_memory_swapin(struct os_pages_memory *pages_memory, void *file);
extern void *gf_pages_memory_swapout(struct os_pages_memory *pages_memory);
extern int gf_is_own_pages(struct os_pages_memory *pages_memory);

void gf_release_file_storage(void *file);

extern char *gf_mem_track_result_path;

#ifdef GF_TRACE_EVENT
extern void gf_register_trace_events(void);
extern void gf_unregister_trace_events(void);
extern void gf_task_create_trace_event(int engine_index, unsigned int context,
                                       unsigned long long task_id, unsigned int task_type);
extern void gf_task_submit_trace_event(int engine_index, unsigned int context,
                                       unsigned long long task_id, unsigned int task_type,
                                       unsigned long long fence_id, unsigned int args);
extern void gf_fence_back_trace_event(int engine_index, unsigned long long fence_id);
extern void gf_begin_section_trace_event(const char* desc);
extern void gf_end_section_trace_event(int result);
extern void gf_counter_trace_event(const char* desc, unsigned long long value);
#endif

extern int gf_seq_printf(struct os_seq_file *, const char *, ...);

extern void GF_API_CALL gf_outb(unsigned short port, unsigned char value);
extern char GF_API_CALL gf_inb(unsigned short port);
extern void gf_console_lock(int lock);
extern int disp_wait_idle(void *disp_info);
extern int gf_disp_wait_idle(void *disp_info);

extern void gf_mb(void);
extern void gf_rmb(void);
extern void gf_wmb(void);
extern void gf_flush_wc(void);
extern void gf_dsb(void);

#ifdef VMI_MODE
extern void gf_set_pages_addr(struct os_pages_memory *mem, unsigned long long addr);
#endif

#endif


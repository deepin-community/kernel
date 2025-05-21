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
#include "os_interface.h"
#include "gf.h"
#include "gf_driver.h"
#include "gf_params.h"
#include "gf_version.h"



#define __STR(x)    #x
#define STR(x)      __STR(x)

/* globals constants */
const unsigned int  GF_PAGE_SHIFT         = PAGE_SHIFT;
#ifndef __frv__
const unsigned int  GF_PAGE_SIZE          = PAGE_SIZE;
#else
const unsigned int  GF_PAGE_SIZE          = 0x1000;
#endif
const unsigned long GF_PAGE_MASK          = PAGE_MASK;
const unsigned long GF_LINUX_VERSION_CODE = LINUX_VERSION_CODE;


char *gf_mem_track_result_path      = "/var/log/gf-mem-track.txt";

void gf_udelay(unsigned long long usecs_num)
{
    unsigned long long msecs = usecs_num;
    unsigned long      usecs = do_div(msecs, 1000);

    if(msecs != 0)mdelay(msecs);
    if(usecs != 0)udelay(usecs);
}

unsigned long long gf_do_div(unsigned long long x, unsigned long long y)
{
    do_div(x, y);
    return x;
}

void gf_msleep(int num)
{
    msleep(num);
}

void gf_getsecs(long *secs, long *usecs)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
    struct timespec64 tv = {0};
    ktime_get_ts64(&tv);
#else
    struct timespec tv = {0};

    //do_posix_clock_monotonic_gettime(&tv);
    ktime_get_ts(&tv);
#endif

    if (secs)
    {
        *secs = tv.tv_sec;
    }

    if (usecs)
    {
        *usecs= tv.tv_nsec/1000;
    }
    return;
}

void gf_get_nsecs(unsigned long long *nsec)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
    struct timespec64 tv = {0};
    ktime_get_ts64(&tv);
#else
    struct timespec tv = {0};

    //do_posix_clock_monotonic_gettime(&tv);
    ktime_get_ts(&tv);
#endif

    *nsec = (unsigned long long)(tv.tv_sec) * 1000000000 + tv.tv_nsec;

    return;
}


void gf_usleep_range(long min, long max)
{
    usleep_range(min, max);
}

void gf_assert(int match, const char *msg)
{
   if(!match)
   {
       if(msg)
       {
           gf_info("%s\n", msg);
       }
       BUG();
   }
}

void gf_dump_stack(void)
{
    dump_stack();
}

int GF_API_CALL gf_copy_from_user(void* to, const void* from, unsigned long size)
{
    return copy_from_user(to, from, size);
}

int GF_API_CALL gf_copy_to_user(void* to, const void* from, unsigned long size)
{
    return copy_to_user(to, from, size);
}

void* GF_API_CALL gf_memset(void* s, int c, unsigned long count)
{
#if defined(__aarch64__)
    //aarch net support device memory memset, need more study.

    int i;
    char* set;

    set = (char*)s;

    for(i=0;i<count;i++)
    {
        set[i] = c;
    }
    return 0;
#else
    return memset(s, c, count);
#endif
}

void* GF_API_CALL gf_memcpy(void* d, const void* s, unsigned long count)
{
#if defined(__aarch64__)
    int i;
    char *dst;
    char *set;

    set = (char*)s;
    dst = (char*)d;

    for(i=0;i<count;i++)
    {
        dst[i] = set[i];
    }
    return 0;
#else
    return memcpy(d, s, count);
#endif
}

int   GF_API_CALL gf_memcmp(const void *s1, const void *s2, unsigned long count)
{
    return memcmp(s1, s2, count);
}

void GF_API_CALL gf_byte_copy(char* dst, char* src, int len)
{
#ifdef __BIG_ENDIAN__

    int i = 0;
    int left;

    for(i = 0; i < len/4; i++)
    {
        *(dst)   =  *(src+3);
        *(dst+1) =  *(src+2);
        *(dst+2) =  *(src+1);
        *(dst+3) =  *(src);
        dst     +=  4;
        src     +=  4;
    }

    left = len & 3;

    for(i = 0; i < left; i++)
    {
        *(dst+3-i) = *(src+i);
    }

#else
    gf_memcpy(dst, src, len);
#endif
}

void GF_API_CALL gf_console_lock(int lock)
{

   if(lock)
     console_lock();
   else
     console_unlock();
}

int   gf_strcmp(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

char* gf_strcpy(char *d, const char *s)
{
    return strcpy(d, s);
}

int   gf_strncmp(const char *s1, const char *s2, unsigned long count)
{
    return strncmp(s1, s2, count);
}

char* gf_strncpy(char *d, const char *s, unsigned long count)
{
    return strncpy(d, s, count);
}

unsigned long gf_strlen(char *s)
{
    return strlen(s);
}

/****************************** IO access functions*********************************/

unsigned long long GF_API_CALL gf_read64(void* addr)
{
#ifdef CONFIG_64BIT
    return readq(addr);
#else
    return 0ull;
#endif
}

unsigned int GF_API_CALL  gf_read32(void* addr)
{
#ifndef __BIG_ENDIAN__
    return readl(addr);
#else
    register unsigned int val;
    val = *(volatile unsigned int*)(addr);
    return val;

#endif
}

unsigned short GF_API_CALL gf_read16(void* addr)
{
#ifndef __BIG_ENDIAN__
   return readw(addr);
#else
    register unsigned short val;
    unsigned long alignedoffset = ((unsigned long)addr/4)*4;
    unsigned long mask = (unsigned long)addr - alignedoffset;

    val = *(volatile unsigned short*)((alignedoffset+(2-mask)));
    return val;
#endif
}

unsigned char GF_API_CALL gf_read8(void* addr)
{
#ifndef __BIG_ENDIAN__
    return readb((void*)addr);
#else
    register unsigned char val;
    unsigned long alignedoffset = ((unsigned long)addr /4) *4;
    unsigned long mask = (unsigned long)addr - alignedoffset;
    val = *(volatile unsigned char*)((alignedoffset+(3-mask)));

    return val;
#endif
}

void GF_API_CALL  gf_write32(void* addr, unsigned int val)
{
#ifndef __BIG_ENDIAN__
    writel(val, addr);
#else
    *(volatile unsigned int*)(addr) = val;


#endif
}

void GF_API_CALL  gf_write16(void* addr, unsigned short val)
{
#ifndef __BIG_ENDIAN__
    writew(val, addr);
#else
    unsigned long alignedoffset = ((unsigned long)addr /4) *4;
    unsigned long mask = (unsigned long)addr - alignedoffset;
    *(volatile unsigned short *)((alignedoffset+(2-mask))) = (val);
#endif
}

void GF_API_CALL  gf_write8(void* addr, unsigned char val)
{
#ifndef __BIG_ENDIAN__
    writeb(val, addr);
#else
     unsigned long alignedoffset = ((unsigned long)addr /4) *4;
     unsigned long mask = (unsigned long)addr - alignedoffset;
     *(volatile unsigned char *)(alignedoffset+(3-mask)) = (val);
#endif
}

static int gf_get_fcntl_flags(int os_flags)
{
    int o_flags = 0;
    int access  = os_flags & OS_ACCMODE;

    if(access == OS_RDONLY)
    {
        o_flags = O_RDONLY;
    }
    else if(access == OS_WRONLY)
    {
        o_flags = O_WRONLY;
    }
    else if(access == OS_RDWR)
    {
        o_flags = O_RDWR;
    }

    if(os_flags & OS_CREAT)
    {
        o_flags |= O_CREAT;
    }

    if(os_flags & OS_APPEND)
    {
        o_flags |= O_APPEND;
    }

    if (os_flags & OS_LARGEFILE)
    {
        o_flags |= O_LARGEFILE;
    }
    return o_flags;
}

struct os_file *gf_file_open(const char *path, int flags, unsigned short mode)
{
    struct os_file *file    = gf_calloc(sizeof(struct os_file));
    int             o_flags = gf_get_fcntl_flags(flags);

    file->filp = filp_open(path, o_flags, mode);

    if(IS_ERR(file->filp))
    {
        gf_error("open file %s failed %ld.\n", path, PTR_ERR(file->filp));

        gf_free(file);

        file = NULL;
    }

    return file;
}

void gf_file_close(struct os_file *file)
{
    filp_close(file->filp, current->files);

    gf_free(file);
}

int gf_file_read(struct os_file *file, void *buf, unsigned long size, unsigned long long *read_pos)
{
    struct file   *filp  = file->filp;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
    mm_segment_t   oldfs = get_fs();
#endif
    loff_t         pos   = 0;
    int            len   = 0;

    if(read_pos)
    {
        pos = *read_pos;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
    set_fs(KERNEL_DS);
    len = vfs_read(filp, buf, size, &pos);
    set_fs(oldfs);
#else
    len = kernel_read(filp, buf, size, &pos);
#endif

    if(read_pos)
    {
        *read_pos = pos;
    }

    return len;
}

int gf_file_write(struct os_file *file, void *buf, unsigned long size)
{
    struct file   *filp  = file->filp;
    int            len   = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
    mm_segment_t   oldfs = get_fs();
    set_fs(KERNEL_DS);
    len = vfs_write(filp, buf, size, &filp->f_pos);
    set_fs(oldfs);
#else
    len = kernel_write(filp, buf, size, &filp->f_pos);
#endif
    return len;
}

/*****************************************************************************/
int GF_API_CALL gf_vsprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);

    ret = vsprintf(buf, fmt, args);

    va_end(args);

    return ret;
}

int GF_API_CALL gf_vsnprintf(char *buf, unsigned long size, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);

    ret = vsnprintf(buf, size, fmt, args);

    va_end(args);

    return ret;
}

int GF_API_CALL gf_sscanf(char *buf, char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);

    ret = vsscanf(buf, fmt, args);

    va_end(args);

    return ret;
}


void GF_API_CALL gf_cb_printk(const char* msg)
{
    if(msg)
    {
        printk("%s", msg);
    }
}

void GF_API_CALL gf_cb_vdbgprint(int enable, unsigned int print_level, const char* prefix, const char* msg, ...)
{
    char buffer[256];
    va_list  args;
    unsigned int prefix_len = 0;

    UNUSED(print_level);

    if(!enable)
    {
        return;
    }

    if(prefix)
    {
        prefix_len = gf_strlen((char*)prefix);
        gf_strncpy(buffer, prefix, prefix_len);
    }

    va_start(args, msg);

    vsnprintf(buffer+prefix_len, 256-prefix_len, msg, args);

    va_end(args);

    printk("%s", buffer);
}


void GF_API_CALL gf_printk(unsigned int msglevel, const char* fmt, ...)
{
    char buffer[256];
    va_list marker;

    if(msglevel >= GF_MSG_LEVEL)
    {
        va_start(marker, fmt);
        vsnprintf(buffer, 256, fmt, marker);
        va_end(marker);

        switch ( msglevel )
        {
        case GF_DRV_DEBUG:
            printk(KERN_DEBUG"%s debug: %s", STR(DRIVER_NAME), buffer);
        break;
        case GF_DRV_WARNING:
            printk(KERN_WARNING"%s warning: %s", STR(DRIVER_NAME), buffer);
        break;
        case GF_DRV_INFO:
            printk(KERN_INFO"%s info: %s", STR(DRIVER_NAME), buffer);
        break;
        case GF_DRV_ERROR:
            printk(KERN_ERR"%s warning: %s", STR(DRIVER_NAME), buffer);
        break;
        case GF_DRV_EMERG:
            printk(KERN_EMERG"%s warning: %s", STR(DRIVER_NAME), buffer);
        break;
        default:
            /* invalidate message level */
            gf_assert(0, NULL);
        break;
        }
    }
}

#define GF_MAX_KMALLOC_SIZE 32 * 1024

void* gf_malloc_priv(unsigned long size)
{
    void* addr = NULL;

    if(size <= GF_MAX_KMALLOC_SIZE)
    {
        addr = kmalloc(size, GFP_KERNEL);
    }

    if(addr == NULL)
    {
        addr = vmalloc(size);
    }

    return addr;
}

void* gf_calloc_priv(unsigned long size)
{
    void* addr = gf_malloc_priv(size);
    if(addr != NULL)
    {
        memset(addr, 0, size);
    }
    return addr;
}

void gf_free_priv(void* addr)
{
    unsigned long addr_l = (unsigned long)addr;
    if(addr == NULL)  return;

#ifndef __frv__
    if(addr_l >= VMALLOC_START && addr_l < VMALLOC_END)
    {
        vfree(addr);
    }
    else
    {
        kfree(addr);
    }
#else
    kfree(addr);
#endif

}

/* bit ops */
int gf_find_first_zero_bit(void *buf, unsigned int size)
{
    return find_first_zero_bit(buf, size);
}

int gf_find_next_zero_bit(void *buf, unsigned int size, int offset)
{
    int pos = find_next_zero_bit(buf, size, offset);

    if(pos >= size)
    {
        pos = find_next_zero_bit(buf, size, 0);
    }

    return pos;
}

void gf_set_bit(unsigned int nr, void *buf)
{
   set_bit(nr, buf);
}

void gf_clear_bit(unsigned int nr, void *buf)
{
   clear_bit(nr, buf);
}

static void *gf_do_mmap(struct file *filp, gf_map_argu_t *map)
{
    struct drm_file *drm_file = filp->private_data;
    gf_file_t    *priv     = drm_file->driver_priv;
    void          *virtual  = NULL;
    phys_addr_t phys_addr = 0;

    if(map->flags.mem_type == GF_SYSTEM_IO)
    {
        /*Since the PA to be mapped in kernel always cpu visiable, whcih means the PA is in kernel range,
         * loss accuracy caused by [u32 <-- unsigned long long] will not happen.
         */
        phys_addr = map->phys_addr;
    }

    gf_mutex_lock(priv->lock);

#ifdef HAS_VM_MMAP
    priv->map = map;

    virtual = (void *)vm_mmap(filp, 0, map->size, PROT_READ | PROT_WRITE, MAP_SHARED, phys_addr);

    priv->map = NULL;
#else
    down_write(&current->mm->mmap_sem);

    priv->map = map;

    virtual = (void *)do_mmap(filp, 0, map->size, PROT_READ | PROT_WRITE, MAP_SHARED, phys_addr);

    priv->map = NULL;

    up_write(&current->mm->mmap_sem);
#endif

    gf_mutex_unlock(priv->lock);

    return virtual;
}

static int gf_do_munmap(gf_vm_area_t *vma)
{
    int ret = 0;

    if(current->mm)
    {
#ifdef HAS_VM_MMAP
        ret = vm_munmap((unsigned long)vma->virt_addr, vma->size);
#else
        down_write(&current->mm->mmap_sem);

        ret = do_munmap(current->mm, (unsigned long)vma->virt_addr, vma->size);

        up_write(&current->mm->mmap_sem);
#endif
    }

    return ret;
}

struct os_atomic *gf_create_atomic(int val)
{
    struct os_atomic *atomic = gf_calloc(sizeof(struct os_atomic));

    if(atomic)
    {
        atomic_set(&atomic->counter, val);
    }

    return atomic;
}

void gf_destroy_atomic(struct os_atomic *atomic)
{
    gf_free(atomic);
}

int gf_atomic_read(struct os_atomic *atomic)
{
    return atomic_read(&atomic->counter);
}

void gf_atomic_inc(struct os_atomic *atomic)
{
    return atomic_inc(&atomic->counter);
}

void gf_atomic_dec(struct os_atomic *atomic)
{
    return atomic_dec(&atomic->counter);
}

int gf_atomic_add(struct os_atomic *atomic, int v)
{
    return atomic_add_return(v, &atomic->counter);
}

int gf_atomic_sub(struct os_atomic *atomic, int v)
{
    return atomic_sub_return(v, &atomic->counter);
}

struct os_mutex *gf_create_mutex(void)
{
    struct os_mutex *mutex = gf_calloc(sizeof(struct os_mutex));

    if(mutex)
    {
        mutex_init(&mutex->lock);
    }
    return mutex;
}

void gf_destroy_mutex(struct os_mutex *mutex)
{
    if(mutex)
    {
        gf_free(mutex);
    }
}

void gf_mutex_lock(struct os_mutex *mutex)
{
    mutex_lock(&mutex->lock);
}

int gf_mutex_lock_killable(struct os_mutex *mutex)
{
    return mutex_lock_killable(&mutex->lock);
}

int gf_mutex_trylock(struct os_mutex *mutex)
{
    int status = GF_LOCKED;

    if(!mutex_trylock(&mutex->lock))
    {
        status = GF_LOCK_FAILED;
    }

    return status;
}

void gf_mutex_unlock(struct os_mutex *mutex)
{
    mutex_unlock(&mutex->lock);
}

struct os_sema *gf_create_sema(int val)
{
    struct os_sema *sem = gf_calloc(sizeof(struct os_sema));

    if(sem != NULL)
    {
        sema_init(&sem->lock, val);
    }

    return sem;
}

void gf_destroy_sema(struct os_sema *sem)
{
    gf_free(sem);
}

void gf_down(struct os_sema *sem)
{
    down(&sem->lock);
}

int gf_down_trylock(struct os_sema *sem)
{
    int status = GF_LOCKED;

    if(down_trylock(&sem->lock))
    {
        status = GF_LOCK_FAILED;
    }

    return status;
}

void gf_up(struct os_sema *sem)
{
    up(&sem->lock);
}

struct os_rwsema *gf_create_rwsema(void)
{
    struct os_rwsema *sem = gf_calloc(sizeof(struct os_rwsema));

    if(sem != NULL)
    {
        init_rwsem(&sem->lock);
    }

    return sem;
}

void gf_destroy_rwsema(struct os_rwsema *sem)
{
    gf_free(sem);
}

void gf_down_read(struct os_rwsema *sem)
{
    down_read(&sem->lock);
}

void gf_down_write(struct os_rwsema *sem)
{
    down_write(&sem->lock);
}

void gf_up_read(struct os_rwsema *sem)
{
    up_read(&sem->lock);
}

void gf_up_write(struct os_rwsema *sem)
{
    up_write(&sem->lock);
}

struct os_spinlock *gf_create_spinlock(int type)
{
    struct os_spinlock *spin = gf_calloc(sizeof(struct os_spinlock));

    if(spin != NULL)
    {
        spin_lock_init(&spin->lock);
    }

    return spin;
}

void gf_destroy_spinlock(struct os_spinlock *spin)
{
    if(spin_is_locked(&spin->lock))
    {
        spin_unlock(&spin->lock);
    }

    gf_free(spin);
}

int gf_spin_try_lock(struct os_spinlock *spin)
{

    int status = GF_LOCK_FAILED;

    if(spin_trylock(&spin->lock))
    {
        status = GF_LOCKED;
    }

    return status;

}
void gf_spin_lock(struct os_spinlock *spin)
{
    spin_lock(&spin->lock);
}

void gf_spin_unlock(struct os_spinlock *spin)
{
    spin_unlock(&spin->lock);
}

unsigned long gf_spin_lock_irqsave(struct os_spinlock *spin)
{
    unsigned long flags;

    spin_lock_irqsave(&spin->lock, flags);

    return flags;
}

void gf_spin_unlock_irqrestore(struct os_spinlock *spin, unsigned long flags)
{
    spin_unlock_irqrestore(&spin->lock, flags);
}

struct os_wait_queue* gf_create_wait_queue(void)
{
    struct os_wait_queue *queue = gf_calloc(sizeof(struct os_wait_queue));
    if(queue)
    {
        init_waitqueue_head(&queue->wait_queue);
    }

    return queue;

}

struct os_wait_event* gf_create_event(int task_id)
{
    struct os_wait_event *event = gf_calloc(sizeof(struct os_wait_event));

    if(event)
    {
        atomic_set(&event->state, 0);
        init_waitqueue_head(&event->wait_queue);
    }

    return event;
}

void gf_destroy_event(void *event)
{
    if(event)
    {
        gf_free(event);
    }
}

gf_event_status_t gf_wait_event_thread_safe(struct os_wait_event *event, condition_func_t condition, void *argu, int msec)
{
    gf_event_status_t status = GF_EVENT_UNKNOWN;

    if(msec)
    {
        long timeout = msecs_to_jiffies(msec);

        timeout = wait_event_timeout(event->wait_queue, (*condition)(argu), timeout);

        if(timeout > 0)
        {
            status = GF_EVENT_BACK;
        }
        else if(timeout == 0)
        {
            status = GF_EVENT_TIMEOUT;
        }
        else if(timeout == -ERESTARTSYS)
        {
            status = GF_EVENT_SIGNAL;
        }
    }
    else
    {
        wait_event(event->wait_queue, (*condition)(argu));
        status = GF_EVENT_BACK;
    }

    return status;
}

static int gf_generic_condition_func(void *argu)
{
    struct os_wait_event *event = argu;

    return atomic_xchg(&event->state, 0);
}

static int event_condition(void** events, int cnt,  wakeup_event  *wakeup)
{
    struct general_wait_event ** p_events = (struct general_wait_event **)events;
    int                   wake = FALSE;
    int                   i = 0;

    if(cnt > WAIT_EVENT_MAX)
        cnt = WAIT_EVENT_MAX;

    for(i = 0; i < cnt; i++)
    {
        if(p_events[i]->event)
        {
            wake = TRUE;
            break;
        }
    }

    if(wake)
    {
        (*wakeup) = WAIT_EVENT0 + i;
    }
    else
    {
        (*wakeup) = WAIT_EVENT_MAX;
    }
    return wake;
}

gf_event_status_t gf_wait_event(struct os_wait_event *event, int msec)
{
    return gf_wait_event_thread_safe(event, &gf_generic_condition_func, event, msec);
}

void gf_wake_up_event(struct os_wait_event *event)
{
    atomic_set(&event->state, 1);
    wake_up(&event->wait_queue);
}

void* gf_create_thread(gf_thread_func_t func, void *data, const char *thread_name)
{
    struct task_struct* thread;
    struct sched_param param = {.sched_priority = 99};

    thread = (struct task_struct*)kthread_run(func, data, thread_name);
#if DRM_VERSION_CODE < KERNEL_VERSION(5,9,0)
    sched_setscheduler(thread, SCHED_RR, &param);
#else
    sched_set_fifo(thread);
#endif

    return thread;
}

void gf_destroy_thread(void* thread)
{
    if(thread)
    {
        kthread_stop(thread);
    }
}

int gf_thread_should_stop(void)
{
    return kthread_should_stop();
}

gf_event_status_t gf_thread_wait(struct os_wait_event *event, int msec)
{
    gf_event_status_t status = GF_EVENT_UNKNOWN;

    if(msec)
    {
        long timeout = msecs_to_jiffies(msec);

        timeout = wait_event_interruptible_timeout(event->wait_queue,
                      gf_generic_condition_func(event) || freezing(current) || kthread_should_stop(),
                      timeout);

        if(timeout > 0)
        {
            status = GF_EVENT_BACK;
        }
        else if(timeout == 0)
        {
            status = GF_EVENT_TIMEOUT;
        }
        else if(timeout == -ERESTARTSYS)
        {
            status = GF_EVENT_SIGNAL;
        }
    }
    else
    {
        int ret = wait_event_interruptible(event->wait_queue,
                      gf_generic_condition_func(event) || freezing(current) || kthread_should_stop());

        if(ret == 0)
        {
            status = GF_EVENT_BACK;
        }
        else if(ret == -ERESTARTSYS)
        {
            status = GF_EVENT_SIGNAL;
        }
    }

    return status;
}

void gf_thread_wake_up(struct os_wait_event *event)
{
    atomic_set(&event->state, 1);
    wake_up_interruptible(&event->wait_queue);
}

int  gf_try_to_freeze(void)
{
#ifdef CONFIG_PM_SLEEP
    return try_to_freeze();
#else
    return 0;
#endif
}

int gf_freezable(void)
{
    return !(current->flags & PF_NOFREEZE);
}

void gf_clear_freezable(void)
{
    current->flags |= PF_NOFREEZE;
}

void gf_old_set_freezable(void)
{
    current->flags &= ~PF_NOFREEZE;
}

void gf_set_freezable(void)
{
#ifdef CONFIG_PM_SLEEP
    set_freezable();
#else
    current->flags &= ~PF_NOFREEZE;
#endif
}

int gf_freezing(void)
{
    return freezing(current);
}

unsigned long gf_get_current_pid(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
    return (unsigned long)current->tgid;
#else
    return (unsigned long)current;
#endif
}

unsigned long gf_get_current_tid(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
    return current->pid;
#else
    return (unsigned long)current;
#endif
}

void gf_get_current_pname(char *pname, int size)
{
    char buf[TASK_COMM_LEN];

    get_task_comm(buf, current);

    gf_strncpy(pname, buf, size);
}

#if defined(__mips64__) || defined(__loongarch__)
void gf_dma_cache_sync(struct device *dev, void *vaddr, size_t size, enum dma_data_direction direction)
{
    unsigned long long len;
    unsigned long addr = (unsigned long)vaddr;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
    dma_cache_sync(dev, vaddr, size, direction);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 19, 0)
    for (len = 0; len < size; len += PAGE_SIZE)
    {
        flush_data_cache_page(addr);
        addr += PAGE_SIZE;
    }
#endif

}
#endif

static void gf_flush_page_cache(struct device *dev, struct page *pages, unsigned int flush_size)
{
#if defined(__mips64__) || defined(__loongarch__)
    int  page_count = flush_size/PAGE_SIZE;
    void *ptr;

    while(page_count--)
    {
        ptr  = kmap(pages);
        //dma_cache_wback_inv
        gf_dma_cache_sync(dev, ptr, PAGE_SIZE, DMA_BIDIRECTIONAL);
        kunmap(pages);
        pages++;
    }
#elif !defined(CONFIG_X86)
    int  page_count = flush_size/PAGE_SIZE;
    void *ptr;

    while(page_count--)
    {
        ptr  = kmap(pages);
#if __ARM_ARCH >= 8
        flush_icache_range((unsigned long)ptr, (unsigned long)(ptr+PAGE_SIZE));
#else
#if !defined(__riscv)
        dmac_flush_range(ptr, ptr + PAGE_SIZE);
        outer_flush_range(page_to_phys(pages), page_to_phys(pages) + PAGE_SIZE);
#endif
#endif
        kunmap(pages);
        pages++;
    }
#else
    int  page_count = flush_size/PAGE_SIZE;
    unsigned long ptr;
    pte_t *pte;
    unsigned int level;
    while(page_count--)
    {
        if(PageHighMem(pages))
            ptr = (unsigned long)kmap(pages);
        else
            ptr = (unsigned long)page_address(pages);

        pte  = lookup_address(ptr,&level);

        if(pte&& (pte_val(*pte)& _PAGE_PRESENT))
        {
           clflush_cache_range((void*)ptr,PAGE_SIZE);
        }

        if(PageHighMem(pages))
            kunmap(pages);

        pages++;
   }

#endif
}

static void gf_free_st_internal(struct device *dev, struct sg_table *st, bool userptr, bool need_dma_unmap)
{
    struct scatterlist *sg;
    struct sg_page_iter sg_iter;

    if (need_dma_unmap)
    {
        dma_unmap_sg(dev, st->sgl, st->nents, DMA_BIDIRECTIONAL);
    }
    if (userptr)
    {
        for_each_sg_page(st->sgl, &sg_iter, st->nents, 0)
        {
            struct page *page = sg_page_iter_page(&sg_iter);
            put_page(page);
        }
    }
    else
    {
        for (sg = st->sgl; sg; sg = sg_next(sg))
        {
            __free_pages(sg_page(sg), get_order(sg->length));
        }
    }

    sg_free_table(st);
    gf_free(st);
}

void gf_free_pages_memory_priv(void *pdev, struct os_pages_memory *memory)
{
    int i;
    int page_cnt;
    int pg_arr_index = 0;
    struct pci_dev *pci_dev = pdev;

    if(memory == NULL)
    {
        return;
    }
    page_cnt = memory->size / PAGE_SIZE;

#ifdef CONFIG_X86_PAT
#ifdef HAS_SET_PAGES_ARRAY_WC
    for (i = 0; i < page_cnt; i++) {
        if (memory->cache_type_per_page[i] != GF_MEM_WRITE_BACK) {
            memory->pages_temp[pg_arr_index++] = memory->pages[i];
        }
    }
    set_pages_array_wb(memory->pages_temp, pg_arr_index);
#else
    for (i = 0; i < page_cnt; i++) {
        if (memory->cache_type_per_page[i] != GF_MEM_WRITE_BACK) {
            struct page *pg = memory->pages[i];
            set_memory_wb((unsigned long)page_address(pg), 1);
        }
    }
#endif
#endif

    /* shared means this pages was import from other, shouldn't free!! */
    if (!memory->shared) {
        gf_free_st_internal(&pci_dev->dev, memory->st, memory->userptr, memory->has_dma_map);
    }
    gf_free(memory);
}

struct os_pages_memory* gf_allocate_pages_memory_struct(int page_cnt, struct sg_table *st)
{
    int ret, i;
    int extra_size  = sizeof(struct page*) * page_cnt * 2 + sizeof(char) * page_cnt;
    struct os_pages_memory *memory = gf_calloc(sizeof(struct os_pages_memory) + extra_size);

    if (memory)
    {
        memory->pages = (void*)(memory + 1);
        memory->pages_temp = memory->pages + page_cnt;
        memory->cache_type_per_page = (char*)(memory->pages_temp + page_cnt);
        if (st)
        {
            memory->st = st;
        }
        else
        {
            memory->st  = gf_calloc(sizeof(*memory->st));
            ret = sg_alloc_table(memory->st, page_cnt, GFP_KERNEL);
            if (ret < 0)
                goto sg_alloc_failed;
        }
        #ifdef CONFIG_X86_PAT
        if (GF_MEM_WRITE_BACK != 0) {
            for(i = 0; i < page_cnt; i++)
                memory->cache_type_per_page[i] = GF_MEM_WRITE_BACK;
        }
        #endif
    }
    return memory;
sg_alloc_failed:
    gf_free(memory);
    return NULL;
}

void gf_pages_memory_extract_st(struct os_pages_memory *memory)
{
    int i = 0;
    struct sg_page_iter sg_iter;

    for_each_sg_page(memory->st->sgl, &sg_iter, memory->st->nents, 0)
    {
        struct page *page = sg_page_iter_page(&sg_iter);
        memory->pages[i] = page;
        i++;
    }
}

//mips PAGE_SIZE is 16k, others is 4k, suppose max page is 2M.
#define MAX_ALLOC_ORDER  ilog2((2*1024*1024/PAGE_SIZE))

// 2^order <= size
static int gf_get_alloc_order(int size)
{
    int order = get_order(size);

    if ((((1 << (order)) << PAGE_SHIFT) > size) && order)
       order--;

    return order > MAX_ALLOC_ORDER ? MAX_ALLOC_ORDER : order;
}

struct os_pages_memory *gf_allocate_pages_memory_priv(void *pdev, int size, int page_size, alloc_pages_flags_t alloc_flags)
{
    int rest_size;
    int order;
    int page_cnt = PAGE_ALIGN(size) / PAGE_SIZE;
    struct page *page;
    struct os_pages_memory *memory = NULL;
    struct scatterlist *sg = NULL, *end_sg = NULL;
    gfp_t  gfp_mask = GFP_KERNEL;
    struct pci_dev *pci_dev = pdev;
    struct page **pages = NULL;
    int  *orders = NULL;
    struct sg_table *st = NULL;
    int n_pages = 0;
    int i;

    if(size <= 0)
    {
        return NULL;
    }

    pages = gf_calloc(page_cnt * sizeof(struct page *));
    orders = gf_calloc(page_cnt * sizeof(int));
    st = gf_calloc(sizeof(struct sg_table));

    if(!pages || !orders || !st)
    {
        goto alloc_pages_failed;
    }

    if (alloc_flags.need_zero)
    {
        gfp_mask |= __GFP_ZERO;
    }
    if (alloc_flags.dma32 && sizeof(dma_addr_t) == 8)
    {
        gfp_mask |= pci_dev->dev.coherent_dma_mask < DMA_BIT_MASK(32) ? GFP_DMA : GFP_DMA32;
    }
    else
    {
#ifdef HAS_SET_PAGES_ARRAY_WC
        gfp_mask |= __GFP_HIGHMEM;
#endif
    }

    rest_size = PAGE_ALIGN(size);
    order = gf_get_alloc_order(rest_size);

    while(rest_size > 0)
    {
        page = alloc_pages(gfp_mask | (order ? (__GFP_NORETRY | __GFP_NOWARN) : 0), order);
        if (page == NULL)
        {
            if (order == 0)
                goto alloc_pages_failed;
            else
                order--;
        }
        else
        {
            orders[n_pages] = order;
            pages[n_pages++] = page;

            if (alloc_flags.need_flush)
                gf_flush_page_cache(&pci_dev->dev, page, PAGE_SIZE << order);
            rest_size -= (PAGE_SIZE << order);
            if (rest_size > 0)
            {
                order = min(gf_get_alloc_order(rest_size), order);
            }
        }
    }

    if(sg_alloc_table(st, n_pages, GFP_KERNEL))
    {
        goto alloc_pages_failed;
    }

    for_each_sg(st->sgl, sg, st->orig_nents, i)
    {
        sg_set_page(sg, pages[i], PAGE_SIZE << orders[i], 0);
        end_sg = sg;
    }

    sg_mark_end(end_sg);

    memory = gf_allocate_pages_memory_struct(page_cnt, st);
    gf_assert(alloc_flags.fixed_page == TRUE, GF_FUNC_NAME(__func__));
    gf_assert(alloc_flags.page_4K_64K == FALSE, "only support 4K page yet");
    gf_assert(page_size == PAGE_SIZE, "only support 4K page yet");

    memory->shared      = FALSE;
    memory->size        = PAGE_ALIGN(size);
    memory->fixed_page  = alloc_flags.fixed_page;
    memory->page_4K_64K = alloc_flags.page_4K_64K;
    memory->page_size   = memory->fixed_page ? PAGE_ALIGN(page_size) : 0;
    memory->dma32       = alloc_flags.dma32;
    memory->need_zero   = alloc_flags.need_zero;

    sg = memory->st->sgl;

    gf_pages_memory_extract_st(memory);

    if (alloc_flags.need_dma_map)
    {
        memory->st->nents = dma_map_sg(&pci_dev->dev, memory->st->sgl, memory->st->orig_nents, DMA_BIDIRECTIONAL);
        if (!memory->st->nents)
        {
            goto alloc_pages_failed;
        }
        memory->has_dma_map = 1;
        if (0)
        {
            gf_info("alloc[%p] st:%p pages:%d size:0x%x nents:%d orig_nents:%d\n",
                memory, memory->st, page_cnt, memory->size, memory->st->nents, memory->st->orig_nents);
            for_each_sg(memory->st->sgl, sg, memory->st->nents, i) {
                gf_info("  [%p] sg:%p i:%d pgcnt:%d off:%x length:%x dmalen:%x dma:[%llx ~ %llx] phys:%llx\n",
                    memory, sg, i, PAGE_ALIGN(sg->offset + sg_dma_len(sg)) >> PAGE_SHIFT, sg->offset,
                    sg->length, sg_dma_len(sg), sg_dma_address(sg), sg_dma_address(sg) + sg_dma_len(sg), sg_phys(sg));
            }
        }
    }

    gf_free(pages);
    gf_free(orders);
    return memory;
alloc_pages_failed:
    gf_error("%s: alloc page failed, size:0x%x\n", __func__, size);

    if(pages)
    {
        gf_free(pages);
    }

    if(orders)
    {
        gf_free(orders);
    }

    if(st)
    {
        gf_free_st_internal(&pci_dev->dev, st, FALSE, memory ? memory->has_dma_map : FALSE);
    }

    gf_free(memory);
    return NULL;
}

void gf_pages_memory_for_each_continues(struct os_pages_memory *memory, void *arg,
    int (*cb)(void* arg, int page_start, int page_cnt, unsigned long long dma_addr))
{
    int i;
    int page_start = 0;
    struct scatterlist *sg;

    for_each_sg(memory->st->sgl, sg, memory->st->nents, i)
    {
        unsigned int length = memory->has_dma_map ? sg_dma_len(sg) : sg->length;
        int page_cnt = PAGE_ALIGN(sg->offset + length) >> PAGE_SHIFT;

        if (0 != cb(arg, page_start, page_cnt, memory->has_dma_map ? sg_dma_address(sg) : sg_phys(sg))) {
            break;
        }
        page_start += page_cnt;
    }
}

static struct page** gf_acquire_os_pages(struct os_pages_memory *memory, unsigned int size, int m_offset, int *pg_cnt)
{
    unsigned int mapped_size = PAGE_ALIGN(size);
    int pages_cnt   = mapped_size / PAGE_SIZE;
    int offset      = _ALIGN_DOWN(m_offset, PAGE_SIZE);

    if (pg_cnt)
        *pg_cnt = pages_cnt;

    return memory->pages + (offset / PAGE_SIZE);
}

static void gf_release_os_pages(struct page **pages)
{
}

unsigned char gf_validate_page_cache(struct os_pages_memory *memory, int start_page, int end_page, unsigned char request_cache_type)
{
    int i;
    int pg_arr_index = 0;
#ifdef CONFIG_X86_PAT
#ifdef HAS_SET_PAGES_ARRAY_WC
    for (i = start_page; i < end_page; i++)
    {
        if (memory->cache_type_per_page[i] == GF_MEM_WRITE_BACK && request_cache_type != GF_MEM_WRITE_BACK)
        {
            memory->pages_temp[pg_arr_index++] = memory->pages[i];
            memory->cache_type_per_page[i] = request_cache_type;
        }
    }
    if (pg_arr_index > 0)
    {
        if(request_cache_type == GF_MEM_WRITE_COMBINED)
        {
           set_pages_array_wc(memory->pages_temp, pg_arr_index);
        }
        else if(request_cache_type == GF_MEM_UNCACHED)
        {
           set_pages_array_uc(memory->pages_temp, pg_arr_index);
        }
    }
#else
    for (i = start_page; i < end_page; i++)
    {
        if (memory->cache_type_per_page[i] == GF_MEM_WRITE_BACK && request_cache_type != GF_MEM_WRITE_BACK)
        {
            struct page* pg = memory->pages[i];
            if(request_cache_type == GF_MEM_WRITE_COMBINED)
            {
               set_memory_wc((unsigned long)page_address(pg), 1);
            }
            else if(request_cache_type == GF_MEM_UNCACHED)
            {
               set_memory_uc((unsigned long)page_address(pg), 1);
            }
            memory->cache_type_per_page[i] = request_cache_type;
        }
    }
#endif
#endif

#if defined(__aarch64__)
   //only map arm64 to write back because cache flush code is not ready. wb is safe for snoop path.
   //now only enable snoop path in arm64.
    return GF_MEM_WRITE_BACK;
//   map_argu->flags.cache_type = GF_MEM_WRITE_COMBINED;
#endif

#if defined(__mips64__) || defined(__loongarch__)
   //force to GF_MEM_WRITE_BACK, but acqually prot will still be PAGE_KERNEL
   //can not be other type, which will change prot to uncached and mips will random hang or blackscreen
    return GF_MEM_WRITE_BACK;
#endif

    return request_cache_type;
}

gf_vm_area_t *gf_map_pages_memory_priv(void *filp, gf_map_argu_t *map_argu)
{
    struct os_pages_memory *memory  = map_argu->memory;
    gf_vm_area_t          *vm_area = gf_calloc(sizeof(gf_vm_area_t));
    struct drm_file       *drm_file = filp;
    int start                       = _ALIGN_DOWN(map_argu->offset, PAGE_SIZE) / PAGE_SIZE;
    int end                         = start + PAGE_ALIGN(map_argu->size) / PAGE_SIZE;

    map_argu->flags.cache_type = gf_validate_page_cache(memory, start, end, map_argu->flags.cache_type);

    if(map_argu->flags.mem_space == GF_MEM_KERNEL)
    {
        pgprot_t     prot       = PAGE_KERNEL;
        unsigned int cache_type = map_argu->flags.cache_type;
        int          page_cnt   = 0;
        struct page  **pages;

        prot  = os_get_pgprot_val(&cache_type, prot, 0);

        pages = gf_acquire_os_pages(memory, map_argu->size, map_argu->offset, &page_cnt);

        vm_area->flags.value = map_argu->flags.value;
        vm_area->size        = map_argu->size;

        vm_area->virt_addr   = vmap(pages, page_cnt, VM_MAP, prot);

        gf_release_os_pages(pages);
    }
    else if(map_argu->flags.mem_space == GF_MEM_USER)
    {
        vm_area->size        = map_argu->size;
        vm_area->owner       = gf_get_current_pid();
        vm_area->virt_addr   = gf_do_mmap(drm_file->filp, map_argu);
        vm_area->flags.value = map_argu->flags.value;
    }

    return vm_area;
}

void gf_unmap_pages_memory_priv(gf_vm_area_t *vm_area)
{
    if(vm_area->flags.mem_space == GF_MEM_KERNEL)
    {
        vunmap(vm_area->virt_addr);
    }
    else if(vm_area->flags.mem_space == GF_MEM_USER)
    {
        gf_do_munmap(vm_area);
    }

    gf_free(vm_area);
}


void gf_flush_cache(void *pdev, gf_vm_area_t *vma, struct os_pages_memory* memory, unsigned int offset, unsigned int size)
{
#ifdef CONFIG_X86
    struct page **acquired_pages;
    struct page *pages;
    unsigned long ptr;
    pte_t *pte;
    int page_start = offset/PAGE_SIZE;
    int page_end   = (offset + size -1)/PAGE_SIZE;
    int page_start_offset = offset%PAGE_SIZE;
    int page_end_offset   = (offset + size -1)%PAGE_SIZE;
    int i;
    int page_count;
    int level;

    if((offset % 32 != 0) || (size % 32 != 0) )
    {
        gf_error("offset does not align to data cache line boundary(32B)");
    }

    gf_assert(size>0, "size>0" );
    gf_assert(offset+size <= memory->size, "offset+size <= memory->size");
    gf_assert(page_start <= page_end, "page_start <= page_end");

    acquired_pages = gf_acquire_os_pages(memory, PAGE_SIZE*(page_end-page_start+1), 0, &page_count);
    if(page_count != (page_end-page_start+1))
    {
        gf_error("page count unequal, need to check!");
    }

    if(page_start == page_end)
    {
        pages = acquired_pages[0];
        ptr  = (unsigned long)page_address(pages);
        pte  = lookup_address(ptr,&level);
        if(pte&& (pte_val(*pte)& _PAGE_PRESENT))
        {
            clflush_cache_range((void*)ptr+page_start_offset,page_end_offset-page_start_offset);
        }
    }
    else
    {
        pages = acquired_pages[0];

        ptr  = (unsigned long)page_address(pages);
        pte  = lookup_address(ptr,&level);
        if(pte&& (pte_val(*pte)& _PAGE_PRESENT))
        {
            clflush_cache_range((void*)ptr+page_start_offset,PAGE_SIZE);
        }

        pages = acquired_pages[page_end-page_start];

        ptr  = (unsigned long)page_address(pages);
        pte  = lookup_address(ptr,&level);
        if(pte&& (pte_val(*pte)& _PAGE_PRESENT))
        {
            clflush_cache_range((void*)ptr,page_end_offset);
        }
    }

    for(i = page_start+1 ; i <= page_end-1; i++)
    {
        pages = acquired_pages[i-page_start];

        ptr  = (unsigned long)page_address(pages);
        pte  = lookup_address(ptr,&level);
        if(pte&& (pte_val(*pte)& _PAGE_PRESENT))
        {
            clflush_cache_range((void*)ptr,PAGE_SIZE);
        }
    }
    gf_release_os_pages(acquired_pages);

#else
    struct page **acquired_pages;
    struct page *pages;
    void *ptr;
    int page_start = offset/PAGE_SIZE;
    int page_end   = (offset + size -1)/PAGE_SIZE;
    int page_start_offset = offset%PAGE_SIZE;
    int page_end_offset   = (offset + size -1)%PAGE_SIZE;
    int i;
    int page_count;
    struct device *dev = &((struct pci_dev *)pdev)->dev;

    if((offset % 32 != 0) || (size % 32 != 0) )
    {
        gf_error("offset does not align to data cache line boundary(32B)");
    }

    gf_assert(size>0, "size>0");
    gf_assert(offset+size <= memory->size, "offset+size <= memory->size");
    gf_assert(page_start <= page_end, "page_start <= page_end");

    acquired_pages = gf_acquire_os_pages(memory, PAGE_SIZE*(page_end-page_start+1), 0, &page_count);
    if(page_count != (page_end-page_start+1))
    {
        gf_error("page count unequal, need to check!");
    }

    if(page_start == page_end)
    {
        pages = acquired_pages[0];
        ptr  = kmap(pages);
#if defined(__mips64__) || defined(__loongarch__)
        gf_dma_cache_sync(dev, ptr + page_start_offset, page_end_offset - page_start_offset, DMA_BIDIRECTIONAL);
#elif __ARM_ARCH >= 8
        flush_icache_range((unsigned long)(ptr + page_start_offset), (unsigned long)(ptr + page_end_offset));
#else
#if !defined(__riscv)
        dmac_flush_range(ptr + page_start_offset, ptr + page_end_offset);
        outer_flush_range(page_to_phys(pages) + page_start_offset, page_to_phys(pages) + page_end_offset);
#endif
#endif
        kunmap(pages);
    }
    else
    {
        pages = acquired_pages[0];
        ptr  = kmap(pages);
#if defined(__mips64__) || defined(__loongarch__)
        gf_dma_cache_sync(dev, ptr + page_start_offset, PAGE_SIZE - page_start_offset, DMA_BIDIRECTIONAL);
#elif __ARM_ARCH >= 8
        flush_icache_range((unsigned long)(ptr + page_start_offset), (unsigned long)(ptr + PAGE_SIZE));
#else
#if !defined(__riscv)
        dmac_flush_range(ptr + page_start_offset, ptr + PAGE_SIZE);
        outer_flush_range(page_to_phys(pages) + page_start_offset, page_to_phys(pages) + PAGE_SIZE);
#endif
#endif
        kunmap(pages);

        pages = acquired_pages[page_end-page_start];
        ptr  = kmap(pages);
#if defined(__mips64__) || defined(__loongarch__)
        gf_dma_cache_sync(dev, ptr, page_end_offset, DMA_BIDIRECTIONAL);
#elif __ARM_ARCH >= 8
        flush_icache_range((unsigned long)ptr, (unsigned long)(ptr + page_end_offset));
#else
#if !defined(__riscv)
        dmac_flush_range(ptr, ptr + PAGE_SIZE);
        outer_flush_range(page_to_phys(pages), page_to_phys(pages) + page_end_offset);
#endif
#endif
        kunmap(pages);
    }

    for(i = page_start+1 ; i <= page_end-1; i++)
    {
        pages = acquired_pages[i-page_start];
        ptr  = kmap(pages);
#if defined(__mips64__) || defined(__loongarch__)
        gf_dma_cache_sync(dev, ptr, PAGE_SIZE, DMA_BIDIRECTIONAL);
#elif  __ARM_ARCH >= 8
        flush_icache_range((unsigned long)ptr, (unsigned long)(ptr + PAGE_SIZE));
#else
#if !defined(__riscv)
        dmac_flush_range(ptr, ptr + PAGE_SIZE);
        outer_flush_range(page_to_phys(pages), page_to_phys(pages) + PAGE_SIZE);
#endif
#endif
        kunmap(pages);
    }
    gf_release_os_pages(acquired_pages);
#endif
}

void gf_inv_cache(void *pdev, gf_vm_area_t *vma, struct os_pages_memory* memory, unsigned int offset, unsigned int size)
{
#ifdef CONFIG_X86
    //clflush_cache_range(start, size);
    wbinvd();
#else
    struct page **acquired_pages;
    struct page *pages;
    void *ptr;
    int page_start = offset/PAGE_SIZE;
    int page_end   = (offset + size -1)/PAGE_SIZE;
    int page_start_offset = offset%PAGE_SIZE;
    int page_end_offset   = (offset + size - 1)%PAGE_SIZE;
    int i;
    struct device *dev = &((struct pci_dev *)pdev)->dev;

    if((offset % 32 != 0) || (size % 32 != 0) )
    {
        gf_error("offset does not align to data cache line boundary(32B)");
    }

    gf_assert(offset>0, "offset>0");
    gf_assert(size>0, "size>0");
    gf_assert(offset+size <= memory->size, "offset+size <= memory->size");
    gf_assert(page_start <= page_end, "page_start <= page_end");

    acquired_pages = gf_acquire_os_pages(memory, PAGE_SIZE*(page_end-page_start+1), 0, NULL);
    if(page_start == page_end)
    {
        pages = acquired_pages[0];
        ptr  = kmap(pages);
#if defined(__mips64__) || defined(__loongarch__)
        gf_dma_cache_sync(dev, ptr + page_start_offset, page_end_offset - page_start_offset, DMA_BIDIRECTIONAL);
#elif __ARM_ARCH >= 8
        flush_icache_range((unsigned long)(ptr + page_start_offset), (unsigned long)(ptr + page_end_offset));
#else
#if !defined(__riscv)
        dmac_flush_range(ptr + page_start_offset, ptr + page_end_offset);
        outer_inv_range(page_to_phys(pages) + page_start_offset, page_to_phys(pages) + page_end_offset);
#endif
#endif
        kunmap(pages);
    }
    else
    {
        pages = acquired_pages[0];
        ptr  = kmap(pages);
#if defined(__mips64__) || defined(__loongarch__)
        gf_dma_cache_sync(dev, ptr + page_start_offset, PAGE_SIZE - page_start_offset, DMA_BIDIRECTIONAL);
#elif __ARM_ARCH >= 8
        flush_icache_range((unsigned long)(ptr + page_start_offset), (unsigned long)(ptr + PAGE_SIZE));
#else
#if !defined(__riscv)
        dmac_flush_range(ptr + page_start_offset, ptr + PAGE_SIZE);
        outer_inv_range(page_to_phys(pages) + page_start_offset, page_to_phys(pages) + PAGE_SIZE);
#endif
#endif
        kunmap(pages);

        pages = acquired_pages[page_end-page_start];
        ptr  = kmap(pages);
#if defined(__mips64__) || defined(__loongarch__)
        gf_dma_cache_sync(dev, ptr, page_end_offset, DMA_BIDIRECTIONAL);
#elif __ARM_ARCH >= 8
        flush_icache_range((unsigned long)ptr, (unsigned long)(ptr + PAGE_SIZE));
#else
#if !defined(__riscv)
        dmac_flush_range(ptr, ptr + PAGE_SIZE);
        outer_inv_range(page_to_phys(pages), page_to_phys(pages) + page_end_offset);
#endif
#endif
        kunmap(pages);
    }

    for(i = page_start+1 ; i <= page_end-1; i++)
    {
        pages = acquired_pages[i-page_start];
        ptr  = kmap(pages);
#if defined(__mips64__) || defined(__loongarch__)
        gf_dma_cache_sync(dev, ptr, PAGE_SIZE, DMA_BIDIRECTIONAL);
#elif __ARM_ARCH >= 8
        flush_icache_range((unsigned long)ptr, (unsigned long)(ptr + PAGE_SIZE));
#else
#if !defined(__riscv)
        dmac_flush_range(ptr, ptr + PAGE_SIZE);
        outer_inv_range(page_to_phys(pages), page_to_phys(pages) + PAGE_SIZE);
#endif
#endif
        kunmap(pages);
    }
    gf_release_os_pages(acquired_pages);
#endif
}
gf_vm_area_t *gf_map_io_memory_priv(void *filp, gf_map_argu_t *map_argu)
{
    gf_vm_area_t *vm_area   = gf_calloc(sizeof(gf_vm_area_t));
    void          *virt_addr = NULL;
    phys_addr_t phy_addr = map_argu->phys_addr;
#if defined(__mips64__) || defined(__loongarch__)
    //current on pass uc in phyton platform.need to be think over.
    // mips loogson 3A4000/3A3000 seems seems like wc buffer can not auto flush for io mem
    // need confirm with loogson
    map_argu->flags.cache_type = GF_MEM_UNCACHED;
#endif
    if(map_argu->flags.mem_space == GF_MEM_KERNEL)
    {
        if(map_argu->flags.cache_type == GF_MEM_UNCACHED)
        {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
            virt_addr = ioremap(map_argu->phys_addr, map_argu->size);
#else
            virt_addr = ioremap_nocache(phy_addr, map_argu->size);
#endif
        }
        else if(map_argu->flags.cache_type == GF_MEM_WRITE_COMBINED)
        {
#if defined(CONFIG_X86) && !defined(CONFIG_X86_PAT)
            virt_addr = ioremap_nocache(phy_addr, map_argu->size);

            map_argu->flags.cache_type = GF_MEM_UNCACHED;
#else
            if(gf_modparams.gf_hang_dump)
            {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
                virt_addr = ioremap(map_argu->phys_addr, map_argu->size);
#else
                virt_addr = ioremap_nocache(phy_addr, map_argu->size);
#endif
            }
            else
            {
                virt_addr = ioremap_wc(phy_addr, map_argu->size);
            }
#endif
        }
    }
    else if(map_argu->flags.mem_space == GF_MEM_USER)
    {
        virt_addr = gf_do_mmap(filp, map_argu);
    }

    vm_area->flags.value = map_argu->flags.value;
    vm_area->size        = map_argu->size;
    vm_area->virt_addr   = virt_addr;
    vm_area->owner       = gf_get_current_pid();

    return vm_area;
}

void gf_unmap_io_memory_priv(gf_vm_area_t *vm_area)
{
    if(vm_area->flags.mem_space == GF_MEM_KERNEL)
    {
        iounmap(vm_area->virt_addr);
    }
    else if(vm_area->flags.mem_space == GF_MEM_USER)
    {
        gf_do_munmap(vm_area);
    }

    gf_free(vm_area);
}

void *gf_ioremap(unsigned int io_base, unsigned int size)
{

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
    return ioremap(io_base, size);
#else
    return ioremap_nocache(io_base, size);
#endif
}

void gf_iounmap(void *map_address)
{
    iounmap(map_address);
}

int gf_mtrr_add(unsigned long start, unsigned long size)
{
    int reg = -1;

#ifdef CONFIG_MTRR
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,3,0)
    reg = mtrr_add(start, size, MTRR_TYPE_WRCOMB, 1);
#else
    reg = arch_phys_wc_add(start, size);
#endif

    if(reg < 0)
    {
         gf_info("set mtrr range %x -> %x failed. \n", start, start + size);
    }
#endif

    return reg;
}

int gf_mtrr_del(int reg, unsigned long base, unsigned long size)
{
    int err = -1;

#ifdef CONFIG_MTRR
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,3,0)
    err =  mtrr_del(reg, base, size);
#else
    /* avoid build warning */
    arch_phys_wc_del(reg);
    err = 0;
#endif

#endif

    return err;
}

int gf_get_mem_info(mem_info_t *mem)
{
    struct sysinfo si;

    si_meminfo(&si);

    /* we need set pages cache type accord usage, before set_pages_array_wc defined
     * kernel only support set cache type to normal zone pages, add check if can use hight
     */

#ifdef HAS_SET_PAGES_ARRAY_WC
    mem->totalram = si.totalram;
    mem->freeram  = si.freeram;
#else
    mem->totalram = si.totalram - si.totalhigh;
    mem->freeram  = si.freeram  - si.freehigh;
#endif

    return 0;
}

int gf_query_platform_caps(void *pdev, platform_caps_t *caps)
{
    struct sysinfo si;
    struct device* device = &((struct pci_dev *)pdev)->dev;

    si_meminfo(&si);

#if defined(__i386__) || defined(__x86_64__)
    caps->dcache_type = GF_CPU_CACHE_PIPT;
#elif defined(CONFIG_CPU_CACHE_VIPT)
    caps->dcache_type = cache_is_vipt_nonaliasing() ?
                        GF_CPU_CACHE_VIPT_NONALIASING :
                        GF_CPU_CACHE_VIPT_ALIASING;
#elif defined(CONFIG_CPU_CACHE_VIVT)
    caps->dcache_type = GF_CPU_CACHE_VIVT;
#else
    caps->dcache_type = GF_CPU_CACHE_UNKNOWN;
#endif

    caps->iommu_support = TRUE;

    caps->page_size  = PAGE_SIZE;
    caps->page_shift = PAGE_SHIFT;

    caps->system_need_dma32 = dma_get_required_mask(device) > DMA_BIT_MASK(40);
#if defined(__loongarch__)
    caps->system_need_dma32 = FALSE;
#endif

#if defined(__i386__) || defined(__x86_64__)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0)
    caps->suspend_blt_mode = (boot_cpu_data.x86_vendor == X86_VENDOR_HYGON) ? 2 : 1;
#else
    caps->suspend_blt_mode = (boot_cpu_data.x86_vendor == 9) ? 2 : 1;
#endif
#else
    caps->suspend_blt_mode = 1;
#endif

    return 0;
}

int gf_is_own_pages(struct os_pages_memory *pages_memory)
{
    // prevent import page and user page
    return pages_memory && !pages_memory->shared && !pages_memory->userptr;
}
void *gf_pages_memory_swapout(struct os_pages_memory *pages_memory)
{
    struct file          *file_storage = shmem_file_setup("gf gmem", pages_memory->size, 0);
    struct address_space *file_addr_space;
    struct page          **pages;
    struct page          *src_page, *dst_page;
    void                 *src_addr, *dst_addr;
    int i = 0, j = 0, page_count = 0;

    if(file_storage == NULL)
    {
        gf_info("create shmem file failed. size: %dk.\n", pages_memory->size << 10);

        return NULL;
    }

    file_addr_space = file_storage->f_path.dentry->d_inode->i_mapping;

    pages = gf_acquire_os_pages(pages_memory, pages_memory->size, 0, &page_count);
    if(pages == NULL) return NULL;

    for(i = 0; i < page_count; i++)
    {
        src_page = pages[i];

        for(j = 0; j < 100; j++)
        {
            dst_page = os_shmem_read_mapping_page(file_addr_space, i, NULL);

            if(unlikely(IS_ERR(dst_page)))
            {
                msleep(5);
            }
            else
            {
                break;
            }
        }

        if(unlikely(IS_ERR(dst_page)))
        {
            fput(file_storage);
            file_storage = NULL;
            gf_info("when swapout read mapping failed. %d, %d, size: %dk\n", i, j, pages_memory->size >> 10);

            goto __failed;
        }

        preempt_disable();

        src_addr = os_kmap_atomic(src_page, OS_KM_USER0);
        dst_addr = os_kmap_atomic(dst_page, OS_KM_USER1);

        memcpy(dst_addr, src_addr, PAGE_SIZE);

        os_kunmap_atomic(dst_addr, OS_KM_USER1);
        os_kunmap_atomic(src_addr, OS_KM_USER0);

        preempt_enable();

        set_page_dirty(dst_page);
        mark_page_accessed(dst_page);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
        page_cache_release(dst_page);
#else
        put_page(dst_page);
#endif
        balance_dirty_pages_ratelimited(file_addr_space);
        cond_resched();
    }

__failed:

    gf_release_os_pages(pages);
    return file_storage;
}

int gf_pages_memory_swapin(struct os_pages_memory *pages_memory, void *file)
{
    struct file          *file_storage = file;
    struct address_space *file_addr_space;
    struct page          **pages;
    struct page          *src_page, *dst_page;
    void                 *src_addr, *dst_addr;
    int i, page_count = 0;

    file_addr_space = file_storage->f_path.dentry->d_inode->i_mapping;

    pages = gf_acquire_os_pages(pages_memory, pages_memory->size, 0, &page_count);

    if(pages == NULL) return -1;

    for(i = 0; i < page_count; i++)
    {
        src_page = os_shmem_read_mapping_page(file_addr_space, i, NULL);
        dst_page = pages[i];

        if(unlikely(IS_ERR(src_page)))
        {
            gf_info("when swapin read mapping failed. %d\n", i);

            gf_release_os_pages(pages);

            pages = NULL;

            return -1;
        }

        preempt_disable();

        src_addr = os_kmap_atomic(src_page, OS_KM_USER0);
        dst_addr = os_kmap_atomic(dst_page, OS_KM_USER1);

        memcpy(dst_addr, src_addr, PAGE_SIZE);

        os_kunmap_atomic(dst_addr, OS_KM_USER1);
        os_kunmap_atomic(src_addr, OS_KM_USER0);

        preempt_enable();

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
        page_cache_release(src_page);
#else
        put_page(src_page);
#endif
    }

    fput(file_storage);

    gf_release_os_pages(pages);

    return 0;
}

void gf_release_file_storage(void *file)
{
    fput(file);
}

int gf_seq_printf(struct os_seq_file *seq_file, const char *f, ...)
{
    int ret;

#ifdef SEQ_PRINTF
    ret = seq_printf(seq_file->seq_file, f);
#else
    va_list args;

    va_start(args, f);
    seq_vprintf(seq_file->seq_file, f, args);
    va_end(args);

    ret = 0;
#endif
    return ret;
}

void GF_API_CALL gf_outb(unsigned short port, unsigned char value)
{
#if defined(__i386__) || defined(__x86_64__)
    outb(value, port);
#else
    /* No IO operation on non-x86 platform. */
    gf_assert(0, NULL);
#endif
}

char GF_API_CALL gf_inb(unsigned short port)
{
#if defined(__i386__) || defined(__x86_64__)
    return inb(port);
#else
    /* No IO operation on non-x86 platform. */
    gf_assert(0, NULL);

    return 0;
#endif
}

static void __gf_printfn_seq_file(struct os_printer *p, struct va_format *vaf)
{
    gf_seq_printf(p->arg, "%pV", vaf);
}

struct os_printer gf_seq_file_printer(struct os_seq_file *f)
{
    struct os_printer p = {
        .printfn = __gf_printfn_seq_file,
        .arg = f,
    };
    return p;
}

static void __gf_printfn_info(struct os_printer *p, struct va_format *vaf)
{
    gf_info("%pV", vaf);
}

struct os_printer gf_info_printer(void *dev)
{
    struct os_printer p = {
        .printfn = __gf_printfn_info,
        .arg = dev,
    };
    return p;
}

void gf_printf(struct os_printer *p, const char *f, ...)
{
    struct va_format vaf;
    va_list args;
    struct os_printer info = gf_info_printer(NULL);
    va_start(args, f);
    vaf.fmt = f;
    vaf.va = &args;
    if (p == NULL) {
        p = &info;
    }
    p->printfn(p, &vaf);
    va_end(args);
}

int gf_disp_wait_idle(void *disp_info)
{
    return disp_wait_idle(disp_info);
}

#if defined(__i386__) || defined(__x86_64__)
#define gf_mb_asm()       asm volatile("mfence":::"memory")
#define gf_rmb_asm()      asm volatile("lfence":::"memory")
#define gf_wmb_asm()      asm volatile("sfence":::"memory")
#define gf_flush_wc_asm() gf_wmb_asm()
#define gf_dsb_asm()
#elif defined(__mips64__) || defined(__loongarch__)
#include <asm/barrier.h>
#define gf_mb_asm()    mb()
#define gf_rmb_asm()   rmb()
#define gf_wmb_asm()   wmb()
#define gf_flush_wc_asm() mb()
#define gf_dsb_asm()
#elif defined(__aarch64__)
#if __ARM_ARCH >= 7
#define dmb(opt)        asm volatile("dmb " #opt : : : "memory")
#define dsb(opt)        asm volatile("dsb " #opt : : : "memory")

#define gf_mb_asm()    dsb(sy)
#define gf_rmb_asm()   dsb(ld)
#define gf_wmb_asm()   dsb(st)
#define gf_flush_wc_asm() dsb(sy)
#define gf_dsb_asm()   dsb(sy)
#endif
#elif defined(__riscv)
#define gf_mb_asm()
#define gf_rmb_asm()
#define gf_wmb_asm()
#define gf_flush_wc_asm()
#define gf_dsb_asm()
#endif

void gf_mb(void)
{
    gf_mb_asm();
}

void gf_rmb(void)
{
    gf_rmb_asm();
}

void gf_wmb(void)
{
    gf_wmb_asm();
}

void gf_flush_wc(void)
{
    gf_flush_wc_asm();
}

void gf_dsb(void)
{
    gf_dsb_asm();
}

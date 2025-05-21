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
#ifndef __VIDSCHI_H__
#define __VIDSCHI_H__

#include "queue.h"
#include "util.h"
#include "kernel_interface.h"

#define MAX_OBJECT_WAITED               8
#define MAX_OBJECT_SIGNALED             8

#if defined(ANDROID)
#define HW_HANG_LOG_FILE                "/data/gf_hw_hang.log"
#else
#define HW_HANG_LOG_FILE                "/var/log/gf_hw_hang.log"
#endif

#define DAEMON_THREAD_INTERVAL              500 //0.5s

#if defined(GFX_ONLY_FPGA) || defined(VMI_MODE)
#define HW_HANG_MAX_TIMEOUT_NS              (200000 *  1000000ll)  //200s
#define HW_HANG_FAST_RECOVERY_TIMEOUT_NS    HW_HANG_MAX_TIMEOUT_NS //200s
#define SYNC_OBJECT_MAX_SERVER_WAIT_TIME_NS (2000000 * 1000000ll) //2000s
#define ENGINE_IDLE_TIMEOUT_IN_MS           (300 * 1000000ll)      //300sec
#else
#define HW_HANG_MAX_TIMEOUT_NS              (60000 *  1000000ll)  //60s
#define HW_HANG_FAST_RECOVERY_TIMEOUT_NS    (6000 *  1000000ll)  //6s
#define SYNC_OBJECT_MAX_SERVER_WAIT_TIME_NS (90000 * 1000000ll) //90s
#define ENGINE_IDLE_TIMEOUT_IN_MS           (20 * 1000000ll)      //20sec
#endif

#ifdef VMI_MODE
#define CAN_SUBMIT_IN_MAIN_THREAD           TRUE
#else
#define CAN_SUBMIT_IN_MAIN_THREAD           FALSE
#endif

#define HW_HANG_MAX_TIMEOUT_NS_ON_QT                (200000 *  1000000ll) // 200s
#define SYNC_OBJECT_MAX_SERVER_WAIT_TIME_NS_ON_QT   (200000 * 1000000ll) // 200s

#define PRESENT_SRC_ALLOCATION_INDEX    1
#define PRESENT_DST_ALLOCATION_INDEX    2

#define PENDING_TASK_QUEUE_LENGTH       32
#define FLIP_QUEUE_LENGTH               16

#define PRESENT_BUFFER_SIZE             0x00004000
#define PRESENT_PATCH_COUNT             0x00000100
#define PRESENT_ALLOCATION_COUNT        0x00000004
#define PRESENT_COUNT_CMD_SIZE          0x00000020


/* engine support task type */
#define ENGINE_CAPS_PAGING              bit_0
#define ENGINE_CAPS_PRESENT_2DBLT       bit_1
#define ENGINE_CAPS_PRESENT_3DBLT       bit_2
#define ENGINE_CAPS_PRESENT_DMAFLIP     bit_3
#define ENGINE_CAPS_PRESENT_MMIOFLIP    bit_4
#define ENGINE_CAPS_PRESENT_OVERLAYFLIP bit_5
#define ENGINE_CAPS_2D_GRAPHICS         bit_6
#define ENGINE_CAPS_3D_GRAPHICS         bit_7
#define ENGINE_CAPS_VIDEO_DECODE        bit_8
#define ENGINE_CAPS_VIDEO_ENCODE        bit_9
#define ENGINE_CAPS_VIDEO_VPP           bit_10
#define ENGINE_CAPS_VIDEO_CAPTURE       bit_11
#define ENGINE_CAPS_CS                  bit_12


/* misc hw engine caps*/

#define ENGINE_CAPS_NO_HWCTX            bit_30


#define ENGINE_CAPS_PRESENT_ALL (ENGINE_CAPS_PRESENT_2DBLT | ENGINE_CAPS_PRESENT_3DBLT | \
                                 ENGINE_CAPS_PRESENT_DMAFLIP | ENGINE_CAPS_PRESENT_MMIOFLIP | \
                                 ENGINE_CAPS_PRESENT_OVERLAYFLIP)


#define ENGINE_CAPS_SUPPORT_ALL (ENGINE_CAPS_PAGING | ENGINE_CAPS_PRESENT_ALL | \
                                 ENGINE_CAPS_2D_GRAPHICS | ENGINE_CAPS_3D_GRAPHICS | \
                                 ENGINE_CAPS_VIDEO_DECODE | ENGINE_CAPS_VIDEO_ENCODE | \
                                 ENGINE_CAPS_VIDEO_VPP)


#define ENGINE_CAPS_VIDEO       (ENGINE_CAPS_VIDEO_DECODE | ENGINE_CAPS_VIDEO_ENCODE | \
                                 ENGINE_CAPS_VIDEO_VPP)


#define ENGINE_CAPS_GFX         (ENGINE_CAPS_2D_GRAPHICS | ENGINE_CAPS_3D_GRAPHICS)

#define ENGINE_CAPS_MANUAL_POWER_CLOCK (ENGINE_CAPS_VIDEO_DECODE | ENGINE_CAPS_VIDEO_ENCODE | \
                                 ENGINE_CAPS_VIDEO_VPP)

#define ENGINE_CTRL_DISABLE       bit_0
#define ENGINE_CTRL_THREAD_ENABLE bit_1


/* include heap and list node header files */
/* include wdk header files */

typedef struct _vidsch_query_private
{
    unsigned char               engine_count;            /* out: total engine count */
    unsigned int                pcie_segment_id;
    unsigned int                local_segment_id;
    unsigned int                fence_buffer_segment_id;
    unsigned int                engine_caps[MAX_ENGINE_COUNT];
    unsigned int                engine_ctrl[MAX_ENGINE_COUNT];
    unsigned int                engine_hw_queue_size[MAX_ENGINE_COUNT];
    unsigned int                dma_segment[MAX_ENGINE_COUNT];          /* out: dma heap segment */
    unsigned int                dma_buffer_size[MAX_ENGINE_COUNT];      /* out: dma heap size */
    unsigned int                pcie_memory_size[MAX_ENGINE_COUNT];     /* out: pcie memory size need be reserved */
    unsigned int                local_memory_size[MAX_ENGINE_COUNT];    /* out: local memory size need be reserved */
    unsigned int                schedule_serialize[MAX_ENGINE_COUNT];   /* out: if this engine schedule serial or percontext */
    struct vidsch_chip_func     *engine_func[MAX_ENGINE_COUNT];
}vidsch_query_private_t;

typedef struct _vidsch_private_send_count
{
    unsigned char       *dma_buffer0;
    unsigned char       *dma_buffer;
    unsigned int        dma_size;
    unsigned long long        present_id;
}vidsch_private_send_count_t;

typedef struct vidsch_chip_func vidsch_chip_func_t;
typedef struct vidschedule_chip_func vidschedule_chip_func_t;

typedef struct vidsch_wait_fence
{
    adapter_t          *adapter;
    int                engine;

    union {
        struct {
            unsigned long long fence_id;
        };

        struct {
            vidmm_allocation_t *allocation;
            int                 read_only;
        };
    };
}vidsch_wait_fence_t;

typedef struct vidsch_fence_buffer
{
    vidmm_segment_memory_t *reserved_memory;

    unsigned int           bitmap_size;
    unsigned int           total_num;
    unsigned int           left_num;
    int                    free_start;

    struct os_spinlock     *bitmap_lock;
    unsigned long          *bitmap;
    void                   *backup;
}vidsch_fence_buffer_t;


typedef struct __pending_task_pool
{
    struct os_spinlock *lock;
    struct list_head    task_list;
    unsigned int        task_num;
}pending_task_pool_t;

/* scheduler manager is corresponding to hardware engine */
typedef struct _vidsch_mgr
{
    adapter_t           *adapter;                   /* pointer to adapter */

    unsigned int        engine_caps;                /* hw caps for this engine */
    unsigned int        engine_ctrl;                /* how to use this engine */
    unsigned int        engine_index;               /* engine index */

    struct os_spinlock  *fence_lock;

    unsigned int        hw_queue_size;
    int                 uncomplete_task_num;
    int                 prepare_task_num;

    struct os_mutex     *task_id_lock;              // if NULL means percontext schedule
    unsigned long long  task_id;
    unsigned long long  last_submit_task_id;

    unsigned long long  last_send_fence_id;         /* last fence id sent to hardware engine */
    unsigned long long  returned_fence_id;          /* current fence id returned from hw, update in ISR */
    unsigned long long  last_returned_fence_id;

    unsigned long long  returned_timestamp;         /* THE time returned fence id */

    struct os_wait_event *fence_event;
    struct os_mutex      *engine_lock;
    struct os_spinlock   *power_status_lock;

    struct os_mutex     *task_list_lock;
    struct list_head    submitted_task_list;  /* submitted dma list */
    struct list_head    allocated_task_list;  /* allocated but not submit to HW */

    vidmm_segment_memory_t *dma_reserved_memory;
    heap_t              *dma_heap;

    unsigned int        dma_buffer_segment_id;      /* segment id of dma buffer heap */

    util_event_thread_t *worker_thread;

    //queue_t             pending_queue;
    struct os_sema      *normal_pool_sema;
    pending_task_pool_t normal_task_pool;
    struct os_atomic    *total_task_num;

    vidmm_segment_memory_t *local_reserved_memory;

    vidmm_segment_memory_t *pcie_reserved_memory;

    void                *local_reserved_backup;

    int                 init_submit;                /* need init submit ?*/

    void                *private_data;              /* chip private data */
    unsigned int        slot_count;

    unsigned int        engine_dvfs_power_on;

    vidsch_chip_func_t  *chip_func;

    int                 hang_index;
    unsigned long long  hang_fence_id;
    unsigned int        hang_hw_copy_mem;


    unsigned long long  last_idle;
    unsigned long long  last_busy;
    unsigned int  completely_idle;
    unsigned long long  idle_elapse;
    unsigned int boost_level;
}vidsch_mgr_t;

/* for histroy issue, vidsch_mgr_t acctuall mean HW engine, and vidsch_global is engine manager */
typedef struct vidschedule
{
    adapter_t           *adapter;

    int                 disable_schedule;

    util_event_thread_t *daemon_thread_destory_allocation;
    util_event_thread_t *daemon_thread_check_hang;
    util_event_thread_t *daemon_thread_power_control;

    unsigned int        hw_hang; /* one bit for one engine, added for video module checking hw hang by looping */

    unsigned int dvfs_init;
    struct os_spinlock  *dvfs_status_lock;
    struct os_spinlock  *power_status_lock;
    unsigned int        clock_patch_done;

    struct os_rwsema    *rw_lock;

    void                *private_data;

    unsigned int busy_engine_mask;

    vidschedule_chip_func_t  *chip_func;
}vidschedule_t;

struct vidsch_chip_func
{
    int  (*initialize)(adapter_t *adapter, vidsch_mgr_t *sch_mgr);
    int  (*destroy)(vidsch_mgr_t *sch_mgr);

    int (*save)(vidsch_mgr_t *sch_mgr);
    void (*restore)(vidsch_mgr_t *sch_mgr, unsigned int pm);

    int (*render)(gpu_context_t *gpu_context, task_dma_t *task_dma);

    int  (*patch)(adapter_t *adapter, task_dma_t *task_dma);
    int  (*submit)(adapter_t *adapter, vidsch_mgr_t *sch_mgr, task_dma_t *task_dma);

    unsigned long long (*update_fence_id)(vidsch_mgr_t *vidsch);
    void (*set_fence_id)(vidsch_mgr_t *sch_mgr, unsigned long long fence_id);//for hw null use only. set the fence id with cpu.
    int  (*reset_hw)(vidsch_mgr_t *vidsch);

    void (*dump_hang_info)(vidsch_mgr_t *sch_mgr);
    void (*dump_hang)(adapter_t *adapter);

    int  (*cil2_misc)(vidsch_mgr_t *sch_mgr, krnl_cil2_misc_t *cil2);
    void (*set_miu_reg)(adapter_t *adapter,gf_query_info_t *info);
    void (*read_miu_reg)(adapter_t *adapter,gf_query_info_t *info);
    int  (*power_clock)(vidsch_mgr_t *sch_mgr, unsigned int off);
};

struct vidschedule_chip_func
{
    void (*dvfs_tuning)(adapter_t* adapter);
    void (*power_tuning)(adapter_t* adapter, unsigned int gfx_only);
    void (*boost)(adapter_t *adapter, unsigned int state, unsigned int holding_ms, unsigned int force);

    void (*dump_info)(struct os_seq_file *seq_file, adapter_t *adapter);
    void (*dump_debugbus)(struct os_printer *p, adapter_t *adapter);
    void (*get_set_reg)(adapter_t *adapter, gf_query_info_t *info);
};

extern int  vidschi_can_prepare_task_dma(vidsch_mgr_t *sch_mgr, task_dma_t *dma);
extern int  vidschi_send_to_submit_queue(vidsch_mgr_t *sch_mgr, task_dma_t *task_dma);
extern void vidschi_deinit_render_sync_object_list(vidsch_mgr_t *sch_mgr, task_dma_t *render);

extern void vidschi_engine_dvfs_power_on(vidsch_mgr_t *sch_mgr);
extern void vidschi_try_power_tuning(adapter_t *adapter, unsigned int gfx_only);

extern unsigned long long vidschi_inc_send_fence_id(vidsch_mgr_t *sch_mgr, int dec_prepare_num);

extern void vidschi_dump_hang_info(adapter_t *adapter, unsigned int hang_engines);

extern void vidschi_dump_general_task_info(struct os_printer *p, task_desc_t *task);
extern void vidschi_dump_task(struct os_printer *p, adapter_t *adapter, task_desc_t *task, int idx, int dump_detail);
extern void vidschi_dump_task_pool(struct os_printer *p, adapter_t *adapter, pending_task_pool_t *pool, const char *log_str);

extern int vidsch_query_chip(adapter_t *adapter, vidsch_query_private_t *info);

extern int vidschi_init_daemon_thread(adapter_t *adapter);
extern int vidschi_deinit_daemon_thread(adapter_t *adapter);

extern dma_node_t * vidschi_allocate_dma_node(vidsch_mgr_t *vidsch_mgr, unsigned int dma_size);

extern void vidschi_update_and_release_dma_fence(adapter_t *adapter, task_desc_t *task);
extern void vidschi_notify_dma_fence(adapter_t *adapter);

#endif



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

#ifndef __VIDSCH_SYNC_H__
#define __VIDSCH_SYNC_H__

#include "list.h"

typedef struct vidsch_fence_sync_object
{
    unsigned long long   gpu_addr;
    unsigned long long   *virt_addr;
    unsigned long long   shared_value;  /* a value shared by multi process, general use this value a send fence value */
    int                  appid;
    int                  flip_limit;
    int                  discard; /* user mode info*/
}vidsch_fence_sync_object_t;

typedef struct vidsch_dma_sync_object
{
    void                *dma_sync_obj;
}vidsch_dma_sync_object_t;

typedef struct vidsch_sema_sync_object
{
//    int max_count;
    int initial_cnt;
    struct os_sema *slock; //sema used for client wait
}vidsch_sema_sync_object_t;

typedef struct vidsch_mutex_sync_object
{
    int initial_state;
    struct os_mutex *mlock;
}vidsch_mutex_sync_object_t;

typedef struct vidsch_sync_object
{
    struct list_head  list_node;

    gpu_device_t      *device; //creater
    unsigned int      binding : 1; //if binding set mean this sync object create by other resource like context, allocation, and binding to it,
                                   //not directly create by device, on this device, sync object lifetime maintenanced by create resource.
                                   // no need add it to device sync object list.
    unsigned int      type;
    unsigned int      handle;



    struct os_spinlock *lock;

    struct list_head   instance_list; //wait instance reference list.
    struct os_atomic   *ref_cnt;
//    int                ref_cnt;       //wait instance reference cnt, use for easy debug

    union
    {
        vidsch_sema_sync_object_t  sema;
        vidsch_mutex_sync_object_t mutex;
        vidsch_fence_sync_object_t fence;
        vidsch_dma_sync_object_t   dma;
    };
}vidsch_sync_object_t;

typedef struct vidsch_wait_instance
{
    struct list_head     list_node; /* link to sync_obj instance list */
    unsigned long long   fence_id; //only for fence sync
    unsigned long long   timeout;
    unsigned long long   delta_time; // server wait timeout adjust.
    unsigned int         signaled;   //for fence synce
    vidsch_sync_object_t *sync_obj;
}vidsch_wait_instance_t;

typedef struct task_wait
{
    task_desc_t   desc;
    int           only_check_status;
    int           instance_cnt;
    vidsch_wait_instance_t *instances;
    vidsch_wait_instance_t instances_builtin[4];
}task_wait_t;

typedef struct task_signal
{
    task_desc_t          desc;
    vidsch_sync_object_t *sync_obj;
}task_signal_t;

typedef struct vidch_wait_fence_signaled_arg
{
    task_wait_t            *task;
    vidsch_wait_instance_t *wait;
}vidsch_wait_fence_signaled_arg_t;

extern int vidschi_try_wait(task_wait_t *wait_task, int clent_wait);
extern int vidschi_try_signal(task_signal_t *signal);
extern int vidschi_is_server_wait_timeout(task_wait_t *task, unsigned long long curr_time);

extern void vidschi_dump_wait_task(struct os_printer *p, task_wait_t *wait, int idx, int dump_detail);
extern int vidschi_client_wait(task_wait_t *task);

extern void vidschi_release_wait_task(task_wait_t *task_wait);
extern void vidschi_force_signal_fence_sync_object(vidsch_sync_object_t *sync_obj, unsigned long long fence_value);
extern void vidschi_destroy_defer_sync_obj(adapter_t *adapter);
extern void vidschi_add_sync_to_defer_destroy(adapter_t *adapter, vidsch_sync_object_t *sync_obj);

static inline void vidschi_sync_obj_add_reference(vidsch_sync_object_t *sync_obj, vidsch_wait_instance_t *instance)
{
    gf_spin_lock(sync_obj->lock);

    list_add_tail(&instance->list_node, &sync_obj->instance_list);

    gf_atomic_inc(sync_obj->ref_cnt);

    gf_spin_unlock(sync_obj->lock);
}

static inline void vidschi_sync_obj_del_reference(vidsch_sync_object_t *sync_obj, vidsch_wait_instance_t *instance)
{
    gf_spin_lock(sync_obj->lock);

    list_del(&instance->list_node);

    gf_atomic_dec(sync_obj->ref_cnt);

    gf_spin_unlock(sync_obj->lock);
}

static inline void vidschi_sync_obj_inc_reference(vidsch_sync_object_t *sync_obj)
{
    gf_atomic_inc(sync_obj->ref_cnt);
}

static inline void vidschi_sync_obj_dec_reference(vidsch_sync_object_t *sync_obj)
{
    gf_atomic_dec(sync_obj->ref_cnt);
}

static inline int vidschi_sync_obj_referenced(vidsch_sync_object_t *sync_obj)
{
    int referenced;

    gf_spin_lock(sync_obj->lock);

    referenced = (gf_atomic_read(sync_obj->ref_cnt) == 0) ? FALSE : TRUE;

    gf_spin_unlock(sync_obj->lock);

    return referenced;
}

#endif


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

#ifndef __VIDSCH_WORKER_THREAD_H__
#define __VIDSCH_WORKER_THREAD_H__

static inline int vidschi_can_prepare_in_worker_thread(vidsch_mgr_t *sch_mgr, task_dma_t *task_dma)
{
    return vidschi_can_prepare_task_dma(sch_mgr, task_dma);
}

static inline int vidschi_can_submit_task(task_desc_t *task)
{
    int can = FALSE;

    if(task->context_task_id - task->context->last_submit_to_sw == 1)
    {
        can = TRUE;
    }

    return can;
}

static inline void vidschi_init_task_pool(pending_task_pool_t *pool, int need_lock)
{
    list_init_head(&pool->task_list);

    pool->task_num = 0;
    pool->lock     = need_lock ? gf_create_spinlock(0) : NULL;
}

static inline void vidschi_fini_task_pool(pending_task_pool_t *pool)
{
    if(pool->lock)
    {
        gf_destroy_spinlock(pool->lock);

        pool->lock = NULL;
    }
}

static inline void vidschi_add_task_to_pool(pending_task_pool_t *pool, task_desc_t *task)
{
    if(pool->lock)   gf_spin_lock(pool->lock);

    list_add_tail(&task->schedule_node, &pool->task_list);

    pool->task_num++;

    if(pool->lock)   gf_spin_unlock(pool->lock);
}

static inline void vidschi_add_task_to_pool_head(pending_task_pool_t *pool, task_desc_t *task)
{
    if(pool->lock)   gf_spin_lock(pool->lock);

    list_add(&task->schedule_node, &pool->task_list);

    pool->task_num++;

    if(pool->lock)   gf_spin_unlock(pool->lock);
}

/* fetch mean get and del */
static inline task_desc_t *vidschi_fetch_first_task_from_pool(pending_task_pool_t *pool)
{
    task_desc_t *task = NULL;

    if(pool->lock)   gf_spin_lock(pool->lock);

    if(!list_empty(&pool->task_list))
    {
        task = list_first_entry(&pool->task_list, task_desc_t, schedule_node);

        list_del(&task->schedule_node);

        pool->task_num--;
    }

    if(pool->lock)   gf_spin_unlock(pool->lock);

    return task;
}

/* get and return */
static inline task_desc_t *vidschi_get_first_task_from_pool(pending_task_pool_t *pool)
{
    task_desc_t *task = NULL;

    if(pool->lock)   gf_spin_lock(pool->lock);

    if(!list_empty(&pool->task_list))
    {
        task = list_first_entry(&pool->task_list, task_desc_t, schedule_node);
    }

    if(pool->lock)   gf_spin_unlock(pool->lock);

    return task;
}

static inline void vidschi_del_task_from_pool(pending_task_pool_t *pool, task_desc_t *task)
{
    if(pool->lock)   gf_spin_lock(pool->lock);

    list_del(&task->schedule_node);

    pool->task_num--;

    if(pool->lock)   gf_spin_unlock(pool->lock);
}

static inline void vidschi_splice_task_pool(pending_task_pool_t *dst, pending_task_pool_t *src)
{
    if(src->lock != NULL) gf_spin_lock(src->lock);
    if(dst->lock != NULL) gf_spin_lock(dst->lock);

    list_splice_init(&src->task_list, &dst->task_list);

    dst->task_num += src->task_num;
    src->task_num  = 0;

    if(dst->lock != NULL) gf_spin_unlock(dst->lock);
    if(src->lock != NULL) gf_spin_unlock(src->lock);

}

extern void vidschi_emit_dma_fence(vidsch_mgr_t *sch_mgr, task_desc_t *task);
static inline int vidschi_add_task_to_pending_queue(vidsch_mgr_t *sch_mgr, task_desc_t *task)
{
    gpu_context_t       *context = task->context;
    pending_task_pool_t *pool    = &sch_mgr->normal_task_pool;


    gf_down(sch_mgr->normal_pool_sema);
    if(context->context_ctrl)
    {
        gf_down(context->context_sema);
    }


    if (sch_mgr->task_id_lock)
    {
        gf_mutex_lock(sch_mgr->task_id_lock);
        task->global_task_id = ++sch_mgr->task_id;
        gf_mutex_unlock(sch_mgr->task_id_lock);
    }

    if (task->need_dma_fence)
    {
        vidschi_emit_dma_fence(sch_mgr, task);
    }

    gf_atomic_inc(sch_mgr->total_task_num);
    vidschi_add_task_to_pool(pool, task);

    util_wakeup_event_thread(sch_mgr->worker_thread);

    return S_OK;
}

static inline int vidschi_worker_thread_idle(vidsch_mgr_t *sch_mgr)
{
    pending_task_pool_t *normal    = &sch_mgr->normal_task_pool;

    int idle = (normal->task_num == 0);

    return idle;
}

extern int vidsch_worker_thread_event_handler(void *data, gf_event_status_t ret);
extern void vidschi_wait_worker_thread_idle(vidsch_mgr_t *sch_mgr);
extern void vidschi_dump_task_queue(adapter_t *adapter, queue_t *queue, const char *log_str);

#endif


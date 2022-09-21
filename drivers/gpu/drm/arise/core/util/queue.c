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

#include "queue.h"
#include "core_import.h"

#define calc_cyc_num(num) (num) % queue->entry_num

int queue_init(queue_t *queue, int entry_num, int data_size)
{
    int  size    = (data_size + 7) & ~7;
    void *entrys = gf_calloc(entry_num * size);
    int  status  = QUEUE_SUCCESS;

    if(entrys == NULL)
    {
        return QUEUE_FAIL;
    }

    queue->entrys     = entrys;
    queue->entry_size = size;
    queue->data_size  = data_size;
    queue->entry_num  = entry_num;
    queue->lock       = gf_create_spinlock(0);

    return status;
}

void queue_fini(queue_t *queue)
{
    if(queue->lock)
    {
        gf_destroy_spinlock(queue->lock);
    }

    if(queue->entrys)
    {
        gf_free(queue->entrys);
    }
} 

void queue_query_info(queue_t *queue, int *start, int *inuse_num)
{
    unsigned long flags;

    flags = gf_spin_lock_irqsave(queue->lock);

    if(start)
    {
        *start = queue->start;
    }

    if(inuse_num)
    {
        *inuse_num = queue->entry_inuse;
    }

    gf_spin_unlock_irqrestore(queue->lock, flags);
}

int queue_add_entry(queue_t *queue, void *data)
{
    void *entry = NULL;
    int  status = QUEUE_FULL;
    unsigned long flags;

    flags = gf_spin_lock_irqsave(queue->lock);

    if(queue->entry_inuse < queue->entry_num)
    {
        entry = queue->entrys + calc_cyc_num(queue->start + queue->entry_inuse) * queue->entry_size;

        gf_memcpy(entry, data, queue->data_size);

        queue->entry_inuse++;

        status = QUEUE_SUCCESS;
    }
    else
    {
        gf_error("add entry to a full queue. \n");
    }

    gf_spin_unlock_irqrestore(queue->lock, flags);

    return status;
}

int queue_get_first_entry(queue_t *queue, void *data)
{
    int  status = QUEUE_EMPTY;
    void *entry = NULL;
    unsigned long flags;

    flags = gf_spin_lock_irqsave(queue->lock);

    if(queue->entry_inuse > 0)
    {
        entry = queue->entrys + queue->start * queue->entry_size;

        gf_memcpy(data, entry, queue->data_size);

        status = QUEUE_SUCCESS;
    }

    gf_spin_unlock_irqrestore(queue->lock, flags);

    return status;
}

int queue_get_first_entry_and_free(queue_t *queue, void *data)
{
    int  status = QUEUE_EMPTY;
    void *entry = NULL;
    unsigned long flags;

    flags = gf_spin_lock_irqsave(queue->lock);

    if(queue->entry_inuse > 0)
    {
        entry = queue->entrys + queue->start * queue->entry_size;

        queue->start = calc_cyc_num(queue->start + 1);

        queue->entry_inuse--;

        gf_memcpy(data, entry, queue->data_size);

        status = QUEUE_SUCCESS;
    }

    gf_spin_unlock_irqrestore(queue->lock, flags);

    return status;
}

int queue_get_entry(queue_t *queue, int num, void *data)
{
    int  status = QUEUE_SUCCESS;
    int  index  = 0;
    void *entry = NULL;
    unsigned long flags;

    flags = gf_spin_lock_irqsave(queue->lock);
    
    index = calc_cyc_num(num);
    entry = queue->entrys + queue->entry_size * index;

    gf_memcpy(data, entry, queue->data_size);

    gf_spin_unlock_irqrestore(queue->lock, flags);

    return status;
}

int queue_free_entry(queue_t *queue, int num)
{
    int  i, privous_cnt = 0;
    void *src, *dst;
    unsigned long flags;

    flags = gf_spin_lock_irqsave(queue->lock);

    privous_cnt = calc_cyc_num(num - queue->start);

    for(i = privous_cnt; i > 0; i--)
    {
        src = queue->entrys + calc_cyc_num(queue->start + i - 1 ) * queue->entry_size;
        dst = queue->entrys + calc_cyc_num(queue->start + i     ) * queue->entry_size; 

        gf_memcpy(dst, src, queue->data_size);        
    }

    queue->start = calc_cyc_num(queue->start + 1);

    queue->entry_inuse--;

    gf_spin_unlock_irqrestore(queue->lock, flags);
    
    return QUEUE_SUCCESS;
}






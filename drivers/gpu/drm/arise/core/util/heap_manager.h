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

#ifndef __HEAP_MANAGER_H__
#define __HEAP_MANAGER_H__

#include "core_errno.h"
#include "list.h"

typedef struct _list_node
{
    struct list_head list_item;
    unsigned long long    size;               // Available Size, this node occupied space
    unsigned long long    offset;             // Actual offset from start of heap
    unsigned long long    aligned_offset;         // Aligned offset, offset that will be used 
    unsigned long long    aligned_size;           // Aligned size, the size that will be used
} list_node_t;

typedef struct _list_head
{
    struct list_head list;
    unsigned long long     size;
    int                    num;
    int                    order;
} list_head_t;

typedef struct _heap
{
    int                   id;
    unsigned long long    size;
    unsigned long long    start;
    unsigned int          alignment;
    list_head_t     free;
    list_head_t     inuse;

    struct os_mutex *lock;
} heap_t;


int  heap_init(heap_t *heap, int id, unsigned long long start, unsigned long long size, unsigned int alignment);
void heap_destroy(heap_t *heap);
list_node_t * heap_allocate(heap_t *heap, unsigned int size, unsigned int alignment, unsigned int direction);
void heap_release(heap_t *heap, list_node_t *list_node);
void heap_dump(heap_t *heap);

#endif


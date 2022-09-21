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

#include "heap_manager.h"
#include "core_import.h"
#include "util.h"


static void heap_internal_dump(heap_t *heap, list_node_t *list_node, list_node_t *prev_node, list_node_t *next_node);


static void heap_remove_node_from_list(heap_t *heap, list_head_t *list, list_node_t *list_node)
{
    list_del(&list_node->list_item);

    list->size -= list_node->size;
    list->num--;

    list_node->list_item.prev = NULL;
    list_node->list_item.next = NULL;
}

static void heap_add_node_to_list(heap_t *heap, list_head_t *list, list_node_t *list_node)
{
    if(list->order)
    {
        list_node_t *next = NULL;

        list_for_each_entry(next, &list->list, list_item)
        {
            if(next->size > list_node->size)
            {
                break;
            }
        }

        list_add(&list_node->list_item, next->list_item.prev);
    }
    else
    {
        list_add_tail(&list_node->list_item, &list->list);
    }

    list->size += list_node->size;
    list->num++;
}

int heap_init(heap_t *heap, int id, unsigned long long start, unsigned long long size, unsigned int alignment)
{
    list_node_t *new_node = NULL;
    int result = S_OK;

    gf_memset(heap, 0, sizeof(heap_t));

    heap->id             = id;
    heap->size           = size;
    heap->start          = start;
    heap->alignment      = alignment;
    heap->lock           = gf_create_mutex();

    list_init_head(&heap->free.list);

    heap->free.order = TRUE; //order by size increase

    list_init_head(&heap->inuse.list);

    heap->inuse.order = FALSE;

    new_node = gf_calloc(sizeof(list_node_t));

    if(NULL != new_node)
    {
        new_node->size   = size;
        new_node->offset = start;

        heap_add_node_to_list(heap, &heap->free, new_node); 
    }
    else
    {
        gf_destroy_mutex(heap->lock);

        gf_memset(heap, 0, sizeof(heap_t));

        result = E_FAIL;
    }

    gf_debug("heap[%X]:%p, range [0x%x -> 0x%x), size: %dk, alignment: %08x.\n", 
        heap->id, heap, start, start + size, size >> 10, alignment);

    return result;
}

void heap_destroy(heap_t *heap)
{
    if(heap->free.num == 1)
    {
        list_node_t *node = list_entry(heap->free.list.next, list_node_t, list_item);

        list_del(&node->list_item);

        gf_free(node);
    }
    else
    {
        gf_info("heap[%x], not merged. something wrong.\n", heap->id);
    }

    gf_destroy_mutex(heap->lock);

    gf_memset(heap, 0, sizeof(heap_t));
}

//heap allocate will not large than 4G
list_node_t *heap_allocate(heap_t *heap, unsigned int size, unsigned int alignment, unsigned int direction)
{
    list_node_t *new_node  = NULL;
    list_node_t *free_node = NULL;

    unsigned long long start_alignment = util_max(alignment, heap->alignment);
    unsigned long long aligned_mask    = ~(start_alignment - 1); //set to long long int to avoid offset aligh cut of 64bits to 32bits
    unsigned int  aligned_size   = util_align(size, heap->alignment);
    unsigned long long aligned_offset;
    unsigned int  alloc_size     = 0;                            // the size of the node, actually allocated size.
    unsigned long long alloc_offset;                          // the offset of the node, actually offset.

    int found = FALSE;

    if(aligned_size == 0)
    {
        gf_warning("%s, invalidate allocate size: %d.\n", __func__, size);
        gf_dump_stack();

        return NULL;
    }

    gf_mutex_lock(heap->lock);

    if(aligned_size > heap->free.size)
    {
        goto __quit;
    }

    list_for_each_entry(free_node, &heap->free.list, list_item)
    {
        if(aligned_size > free_node->size)
        {
            continue;
        }

        if(direction)
        {
            aligned_offset = (free_node->offset + free_node->size - aligned_size) & aligned_mask;

            if(aligned_offset >= free_node->offset)
            {
                alloc_offset = aligned_offset;
                alloc_size   = (free_node->offset + free_node->size) - alloc_offset;

                found = TRUE;
                break;
            }
        }
        else
        {
            aligned_offset = util_align(free_node->offset, start_alignment);

            if((aligned_offset + aligned_size) <= (free_node->size + free_node->offset))
            {
                alloc_offset = free_node->offset;
                alloc_size   = aligned_size + aligned_offset - free_node->offset;

                found = TRUE;
                break;
            }
        }
    }

    if(found)
    {
        heap_remove_node_from_list(heap, &heap->free, free_node);

        if(free_node->size == alloc_size)
        {
            new_node = free_node;
        }
        else
        {
            if(!direction)
            {
                free_node->offset += alloc_size;
            }

            free_node->size -= alloc_size;

            heap_add_node_to_list(heap, &heap->free, free_node); 

            new_node = gf_calloc(sizeof(list_node_t));
        }

        if(new_node)
        {
            new_node->aligned_offset = aligned_offset;
            new_node->aligned_size   = aligned_size;
            new_node->offset         = alloc_offset;
            new_node->size           = alloc_size;

            heap_add_node_to_list(heap, &heap->inuse, new_node);

            gf_assert(new_node->size >= size, "new_node->size >= size");
        }
        else
        {
            gf_error("calloc failed for new_node.\n");
        }
    }

__quit:
    gf_mutex_unlock(heap->lock);

    return new_node;
}

void heap_release(heap_t *heap, list_node_t *list_node)
{
    list_node_t *prev = NULL;
    list_node_t *next = NULL;
    list_node_t *temp_node = NULL;

    unsigned long long upper;
    unsigned long long lower;

    gf_mutex_lock(heap->lock);

    heap_remove_node_from_list(heap, &heap->inuse, list_node);

    upper = list_node->offset + list_node->size;
    lower = list_node->offset;

    list_for_each_entry(temp_node, &heap->free.list, list_item)
    {
        if(!prev && ((temp_node->offset + temp_node->size) == lower))
        {
            prev = temp_node;
        }

        if(!next && (temp_node->offset == upper))
        {
            next = temp_node;
        }

        if(prev && next)
        {
            break;
        }
    }

    if(prev != NULL)
    {
        list_node->size  += prev->size;
        list_node->offset = prev->offset;

        heap_remove_node_from_list(heap, &heap->free, prev);

        gf_free(prev);
    }

    if(next != NULL)
    {
        list_node->size += next->size;

        heap_remove_node_from_list(heap, &heap->free, next);

        gf_free(next);
    }

    heap_add_node_to_list(heap, &heap->free, list_node);

    gf_mutex_unlock(heap->lock);
}

void heap_list_dump(heap_t *heap, list_head_t *list, const char *desc)
{
    list_node_t *node = NULL;

    int i = 0;

    gf_info("%s, node_num: %d, node_size: 0x%llx.\n", desc, list->num, list->size);

    list_for_each_entry(node, &list->list, list_item)
    {
        gf_info("node[%04x]: %p, prev: %p, next: %p, offset: %llX-%llX, size: %llX-%llX\n", 
            i++, node, node->list_item.prev, node->list_item.next, node->offset, node->aligned_offset, node->size, node->aligned_size);
    }
}

static void heap_internal_dump(heap_t *heap, list_node_t *list_node, list_node_t *prev_node, list_node_t *next_node)
{
    list_head_t *list_head = NULL;

    gf_info("******* node: %p, prev: %p, next: %p.\n", list_node, prev_node, next_node);

    if(list_node != NULL)
    {
        gf_info("list_node: %p, size: %llx, offset: %llx.\n", list_node, list_node->size, list_node->offset);
    }

    if(prev_node != NULL)
    {
        gf_info("prev_node: %p, size: %llx, offset: %llx.\n", prev_node, prev_node->size, prev_node->offset);
    }

    if(next_node != NULL)
    {
        gf_info("next_node: %p, size: %llx, offset: %llx.\n", next_node, next_node->size, next_node->offset);
    }

    gf_info("###Heap[%2x]: %p: Size: %llX.\n", heap->id, heap, heap->size);

    heap_list_dump(heap, &heap->inuse, "**inused list");

    heap_list_dump(heap, &heap->free, "**free list");
}

void heap_dump(heap_t *heap)
{
    gf_mutex_lock(heap->lock);

    heap_internal_dump(heap, NULL, NULL, NULL);

    gf_mutex_unlock(heap->lock);
}


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

#include "ring_buffer.h"
#include "core_errno.h"
#include "core_import.h"

#define DEBUG_RING_BUFFER 0

#if DEBUG_RING_BUFFER
#define ring_debug gf_info
#else
#define ring_debug(args...)
#endif

int ring_buffer_init(ring_buffer_t *ring, int size, int reserved_size, ring_func_t wrap, void *private_data)
{
    ring->size          = size;
    ring->head          = 
    ring->tail          = reserved_size;
    ring->wrap          = wrap;
    ring->left_size     = size - reserved_size;
    ring->private_data  = private_data;
    ring->reserved_size = reserved_size;

    return S_OK;
}

#if DEBUG_RING_BUFFER
static int wrap_print = 0;
#endif

int ring_buffer_get_space(ring_buffer_t *ring, int requested_size)
{
    int space_offset = -1;

    /* ring wrapped, we will skip tail space and reset tail to header and only update head when this time */
    if(ring->tail + requested_size >= ring->size)
    {
        int skip_cnt = ring->size - ring->tail;

        ring_debug("^^ wrap :requ :%08x, tail:%08x, head:%08x, left:%08x.\n", requested_size, ring->tail, ring->head, ring->left_size);
        ring->head = ring->wrap(ring, skip_cnt, ring->private_data);

        ring->tail      = ring->reserved_size;
        ring->left_size = ring->head - ring->tail;

        ring_debug("-- wrap :skip :%08x, tail:%08x, head:%08x, left:%08x.\n", skip_cnt, ring->tail, ring->head, ring->left_size);
    }

    /* if left size not enough, update left size */
    if(ring->left_size < requested_size)
    {
        ring_debug("update ^^ :requ :%08x, tail:%08x, head:%08x, left:%08x.\n", requested_size, ring->tail, ring->head, ring->left_size);
        ring->head = ring->wrap(ring, 0, ring->private_data);
        ring->left_size = ring->size + ring->head - ring->tail - ring->reserved_size;

        ring_debug("update --:skip :%08x, tail:%08x, head:%08x, left:%08x.\n", 0, ring->tail, ring->head, ring->left_size);
    }

#if DEBUG_RING_BUFFER
    if(ring->tail > ring->size - (requested_size * 3))
    {
        wrap_print = 5;
    }
#endif

    if(ring->left_size >= requested_size)
    {
        ring->left_size -= requested_size;

        space_offset = ring->tail;
        ring->tail  += requested_size;
    }
    else
    {
        ring_debug("fail **:requ :%08x, tail:%08x, head:%08x, left:%08x.\n", requested_size, ring->tail, ring->head, ring->left_size);
    }

#if DEBUG_RING_BUFFER
    if(wrap_print > 0)
    {
        wrap_print--;

        ring_debug(" @@ offset:%x, tail:%x.\n", space_offset, ring->tail);
    }
#endif

    return space_offset;
}

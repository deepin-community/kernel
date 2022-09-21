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

#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H


typedef struct ring_buffer ring_buffer_t;

typedef int (*ring_func_t)(ring_buffer_t *ring, int skip_cnt, void *private_data);


struct ring_buffer
{
    unsigned int head; // buffer begin used offset 
    unsigned int tail; // buffer used end offset
    unsigned int size;
    unsigned int left_size;
    unsigned int reserved_size; // reserved space from buffer begin.

    ring_func_t  wrap;
    void         *private_data;
};
 

extern int ring_buffer_init(ring_buffer_t *ring, int size, int reserved_size, ring_func_t wrap, void *private_data);
extern int ring_buffer_get_space(ring_buffer_t *ring, int requested_size);

#endif



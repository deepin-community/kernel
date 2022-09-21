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

#ifndef __QUEUE_H_
#define __QUEUE_H_


#define QUEUE_SUCCESS 0
#define QUEUE_FAIL    1
#define QUEUE_FULL    2
#define QUEUE_EMPTY   3



typedef struct
{
    int  entry_num;
    int  entry_size;
    int  data_size;
    int  entry_inuse;
    int  start;
//    int  end;

    struct os_spinlock *lock;
    char *entrys;
}queue_t;

extern int  queue_init(queue_t *queue, int entry_num, int entry_size);
extern void queue_fini(queue_t *queue);
extern int  queue_add_entry(queue_t *queue, void *data);
extern int  queue_get_first_entry_and_free(queue_t *queue, void *data);
extern int  queue_get_entry(queue_t *queue, int num, void *data);
extern int  queue_get_first_entry(queue_t *queue, void *data);
extern int  queue_free_entry(queue_t *queue, int num);
extern void queue_query_info(queue_t *queue, int *start, int *inuse_num);

#endif /* __QUEUE_H_*/


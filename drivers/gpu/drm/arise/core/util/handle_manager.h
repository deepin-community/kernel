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

#ifndef __HANDLE_MANAGER_H__
#define __HANDLE_MANAGER_H__

#include "core_errno.h"
#include "core_import.h"

#define HDL_TYPE_ADAPTER    0x01
#define HDL_TYPE_DEVICE     0x02
#define HDL_TYPE_CONTEXT    0x03
#define HDL_TYPE_RESOURCE   0x04
#define HDL_TYPE_ALLOCATION 0x05
#define HDL_TYPE_SYNC_OBJ   0x06
#define HDL_TYPE_OVERLAY    0x07
#define HDL_TYPE_CAPTURE    0x08
#define HDL_TYPE_STREAM     0x09
#define HDL_TYPE_DI_CONTEXT 0x0a

typedef struct handle_mgr
{
    unsigned int           max_handle;
    long                   *table;

    unsigned int           left_num;
    int                    free_start;

    struct os_spinlock     *bitmap_lock;
    unsigned long          *bitmap;
    unsigned int           bitmap_size;
} handle_mgr_t;


static inline void *get_from_handle(handle_mgr_t *handle_mgr, unsigned int handle)
{
    void *data = NULL;
    unsigned int index = handle & 0x00FFFFFF;

    /* NOTE: currenly handle mgr impl by bitmap, and the table/bitmap preallocated, 
     * so the only need lock is search zero bit in bitmap, no need lock here
     */

    //flags = gf_spin_lock_irqsave(handle_mgr->bitmap_lock);

    if(index > 0 && index < handle_mgr->max_handle)
    {
        data = (void *)handle_mgr->table[index];
    }

    //gf_spin_unlock_irqrestore(handle_mgr->bitmap_lock, flags);

    return data;
}

extern int handle_mgr_create(handle_mgr_t *handle_mgr);
extern void handle_mgr_destroy(handle_mgr_t *handle_mgr);
extern unsigned int add_handle(handle_mgr_t *handle_mgr, unsigned int, void *data);
extern void *get_from_handle_and_validate(handle_mgr_t *handle_mgr, unsigned int handle, unsigned int type);
extern void remove_handle(handle_mgr_t *handle_mgr, unsigned int handle);

#endif

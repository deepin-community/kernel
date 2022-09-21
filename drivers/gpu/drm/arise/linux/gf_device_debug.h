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

#ifndef __GF_DEVICE_DEUBG_H__
#define __GF_DEVICE_DEUBG_H__
typedef struct gf_device_debug_info {
    struct dentry               *dentry;    //the device root directly
    struct dentry               *info;
    void                        *adapter;
    void                        *debugfs_dev; //the gf root device
    int                         id;
    char                        name[20];
    int                         min_id;
    struct list_head            list_item;
    unsigned int                hDevice;

    struct dentry               *d_alloc;
    unsigned long               user_pid;
}gf_device_debug_info_t;

#endif


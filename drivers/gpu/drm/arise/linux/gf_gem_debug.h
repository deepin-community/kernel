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

#ifndef __GF_GEM_DEUBG_H__
#define __GF_GEM_DEUBG_H__
#include "gf_device_debug.h"

typedef struct gf_gem_debug_info {
    gf_device_debug_info_t **parent_dev;  /*in*/
    char                    name[20];    /*in*/
    struct dentry           *root;      /*in*/


    void                    *parent_gem;

    struct dentry          *self_dir;
    struct dentry          *alloc_info;
  

    struct dentry           *data;
    bool                    is_cpu_accessable;

    struct dentry           *control;
    bool                    mark_unpagable;

    struct dentry           *link;
    struct dentry           *bl; //burst length for compress

    bool                    is_dma_buf_import;
}gf_gem_debug_info_t;



#endif


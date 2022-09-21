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

#ifndef __GF_DEUBGFS_H__
#define __GF_DEUBGFS_H__

#include "gf.h"
#include "gf_driver.h"
#include "gf_device_debug.h"

#define DEBUGFS_ESCAPE_CREATE   1
#define DEBUGFS_GF_CREATE      2

#define DEBUGFS_HEAP_NUM        8
#define DEBUGFS_MEMTRACK        (DEBUGFS_HEAP_NUM + 1)

struct gf_debugfs_device;

typedef struct gf_debugfs_node
{
    int                         type;
    void                        *adapter;
    struct gf_debugfs_device   *dev;
    int                         id;
    char                        name[20];
    struct dentry               *node_dentry;
    int                         create_hint;
    int                         min_id;
    struct list_head            list_item;
    unsigned int                hDevice;

}gf_debugfs_node_t;

struct gf_debugfs_device;
typedef struct gf_debugfs_mmio
{
    struct dentry                  *mmio_root;
    struct gf_debugfs_node         regs;
    struct gf_debugfs_node         info; 
    struct gf_debugfs_device       *debug_dev;
}gf_debugfs_mmio_t;

typedef struct gf_heap_info
{
    struct gf_debugfs_node         heap[DEBUGFS_HEAP_NUM];
    struct dentry                  *heap_dir;
}gf_heap_info_t;

typedef  struct gf_debugfs_crtcs
{
    struct gf_debugfs_device*  debug_dev;
    struct dentry*             crtcs_root;    
    struct gf_debugfs_node*    crtcs_nodes;
    int                        node_num;
}gf_debugfs_crtcs_t;

typedef struct gf_debugfs_device
{
    gf_card_t                      *gf;
    struct list_head                node_list;

    struct dentry                   *device_root;
    struct list_head                device_node_list;

    struct dentry                   *allocation_root;

    gf_heap_info_t                 heap_info;

    struct gf_debugfs_node         info;
    struct gf_debugfs_node         memtrack;
    struct gf_debugfs_node         vidsch;
    struct gf_debugfs_node         displayinfo;
    struct gf_debugfs_node         debugbus;
    struct gf_debugfs_node         umd_trace;
    struct dentry                   *debug_root;
    struct os_mutex                 *lock;
    gf_debugfs_mmio_t              mmio;
    gf_debugfs_crtcs_t             crtcs;
}gf_debugfs_device_t;



gf_debugfs_device_t* gf_debugfs_create(gf_card_t *gf, struct dentry *minor_root);
int gf_debugfs_destroy(gf_debugfs_device_t* dev);
gf_device_debug_info_t* gf_debugfs_add_device_node(gf_debugfs_device_t* dev, int id, unsigned int handle);
int gf_debugfs_remove_device_node(gf_debugfs_device_t* dev, gf_device_debug_info_t *dnode);

static __inline__ struct dentry *gf_debugfs_get_allocation_root(void *debugfs_dev)
{
    gf_debugfs_device_t *dev =  (gf_debugfs_device_t *)debugfs_dev;
    return dev->allocation_root;
}
#endif



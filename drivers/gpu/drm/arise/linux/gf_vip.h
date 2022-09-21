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

#ifndef __GF_VIP_H_
#define __GF_VIP_H_

#include "gf_capture_drv.h"

#define GF_VIP_ENABLE       0
#define GF_VIP_DISABLE      1
#define GF_VIP_SET_MODE     2
#define GF_VIP_SET_BUFFER   3
#define GF_VIP_QUERY_CAPS   4

#define GF_CAPTURE_VIP_INTERFACE

typedef struct
{
    gf_capture_t *mcapture;
    unsigned char fb_num;
    unsigned char vip;
    void (*cb)(gf_capture_id_t id, void *data, unsigned int flags);
    void *cb_data;
    struct work_struct vip_work;
}vip_spec_info_t;

typedef struct
{
    unsigned char vip;
    unsigned int  op;
    union{
        struct
        {
            unsigned int fmt;
            unsigned int chip;
            unsigned int x_res;
            unsigned int y_res;
            unsigned int refs;
        }mode;

        struct
        {
            unsigned char fb_num;
            unsigned char fb_idx;
            unsigned long long fb_addr;
        }fb;

        struct
        {
            unsigned int mode_num;
            gf_vip_mode_t *mode;
        }caps;
    };
}gf_vip_set_t;

void disp_register_vip_capture(disp_info_t *disp_info);
void disp_unregister_vip_capture(disp_info_t *disp_info);

#endif

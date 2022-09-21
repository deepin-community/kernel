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

#ifndef __GF_WB_H_
#define __GF_WB_H_

#include "gf_capture_drv.h"

#define GF_WB_ENABLE               (0x1 << 0)
#define GF_WB_DISABLE              (0x1 << 1)
#define GF_WB_SET_MODE             (0x1 << 2)
#define GF_WB_SET_BUFFER           (0x1 << 3)
#define GF_WB_QUERY_CAPS           (0x1 << 4)
#define GF_WB_SET_BYPASS_MODE      (0x1 << 5)
#define GF_WB_SET_DOWNSCALER_MODE  (0x1 << 6)
#define GF_WB_SET_VSYNC_OFF        (0x1 << 7)

#define GF_WB_MODE_SETTING_S   (0x1 << 0)
#define GF_WB_MODE_BYPASS_S    (0x1 << 1)
#define GF_WB_VSYNC_OFF_S      (0x1 << 2)


#define GF_CAPTURE_WB_INTERFACE

typedef struct
{
    unsigned int src_x;
    unsigned int src_y;
    unsigned int out_fmt;
    unsigned int work_mode;
    unsigned int dst_x;
    unsigned int dst_y;
    unsigned int double_buf;
}gf_wb_mode_set_t;

typedef struct
{
    gf_capture_t *mcapture;
    unsigned char iga;
    void (*cb)(gf_capture_id_t id, void *data, unsigned int flags);
    void *cb_data;
    gf_wb_mode_set_t mode_set;
    unsigned int wb_status;
    struct work_struct wb_work;
}wb_spec_info_t;


typedef struct
{
    unsigned int op_flags;
    unsigned char iga_idx;
    union{
        gf_wb_mode_set_t mode;

        struct
        {
            unsigned int src_x;
            unsigned int src_y;
            unsigned int refreshrate;
            unsigned int capture_fmt;
        }input_mode;

        struct
        {
            unsigned long long fb_addr;
        }fb;
    };
}gf_wb_set_t;

void disp_register_wb_capture(disp_info_t *disp_info);
void disp_unregister_wb_capture(disp_info_t *disp_info);

#endif

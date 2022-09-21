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

#ifndef __GF_CAPTURE_DRV_H_
#define __GF_CAPTURE_DRV_H_

#include "gf_capture.h"

#define CAPTURE_INIT_B          (0x1 << 1)
#define CAPTURE_ENABLE_B        (0x1 << 2)
#define CAPTURE_NEED_SKIP_B     (0x1 << 3)

typedef enum _gf_capture_event_t
{
    GF_CAPTURE_EVENT_NONE             = 0,
    GF_CAPTURE_EVENT_PLUG_OUT         = 1,
    GF_CAPTURE_EVENT_PLUG_IN          = 2,
    GF_CAPTURE_EVENT_SIGNAL_OFF       = 3,
    GF_CAPTURE_EVENT_SIGNAL_ON        = 4,
}gf_capture_event_t;

typedef struct _gf_capture_desc
{
    gf_capture_id_t cf_fmt_id;
    char* cf_name;
}gf_capture_desc;

struct gf_capture_type
{
    gf_capture_id_t cf_fmt_id;
    char *cf_name;
    int (*cf_ctl)(disp_info_t *disp_info, struct gf_capture_type *capture, void *params);
    void (*notify_interrupt)(disp_info_t *disp_info, struct gf_capture_type *capture);
    void (*event_handler)(disp_info_t *disp_info, struct gf_capture_type *capture, gf_capture_event_t event);
    unsigned int flags;
    void *priv_info;
    struct gf_capture_type *cf_next;
};

typedef struct gf_capture_type gf_capture_t;

typedef struct _gf_cf_irq_tbl_t
{
    unsigned int irq_mask;
    gf_capture_id_t cf_id;
}gf_cf_irq_tbl_t;


void gf_capture_interrupt_handle(disp_info_t *disp_info, unsigned int itrr);
void gf_capture_handle_event(disp_info_t *disp_info, gf_capture_id_t cf_id, gf_capture_event_t event);
void disp_register_capture(disp_info_t *disp_info, gf_capture_t *capture);
void disp_unregister_capture(disp_info_t *disp_info, gf_capture_t *capture);
gf_capture_t *disp_find_capture(disp_info_t *disp_info, int id);
void disp_capture_init(disp_info_t *disp_info);
void disp_capture_deinit(disp_info_t *disp_info);


#endif

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

#include "gf_disp.h"
#include "gf_vip.h"
#include "gf_wb.h"
#include "gf_capture_drv.h"
#include "gf_version.h"

static disp_info_t *g_disp_info = NULL;

void disp_register_capture(disp_info_t *disp_info, gf_capture_t *capture)
{
    capture->cf_next = disp_info->captures;
    disp_info->captures  = capture;
}

void disp_unregister_capture(disp_info_t *disp_info, gf_capture_t *capture)
{
    gf_capture_t **actcf;

    for (actcf = &disp_info->captures; *actcf && *actcf != capture; actcf = &((*actcf)->cf_next))
        ;

    if (*actcf)
        *actcf = (*actcf)->cf_next;
}


gf_capture_t *disp_find_capture(disp_info_t *disp_info, int id)
{

    gf_capture_t *actcf;

    for (actcf = disp_info->captures; actcf && actcf->cf_fmt_id != id; actcf = actcf->cf_next)
        ;

    return actcf;
}


int disp_capture_ctl(void *pdata, gf_capture_id_t id, void *params)
{
    disp_info_t *disp_info = (disp_info_t *)pdata;

    gf_capture_t *capture = disp_find_capture(disp_info, id);


    if (!capture)
    {
        gf_assert(0, "wrong capture fmt id !!!\n");
    }

    return capture->cf_ctl(disp_info, capture, params);
}


void disp_capture_init(disp_info_t *disp_info)
{
    gf_info("disp cature init enter \n");
    disp_register_vip_capture(disp_info);
    disp_register_wb_capture(disp_info);

    //TODO: refine the card index function
    g_disp_info = disp_info;

    gf_info("disp cature init out \n");
}

void disp_capture_deinit(disp_info_t *disp_info)
{
    gf_info("disp cature deinit enter \n");
    disp_unregister_vip_capture(disp_info);
    disp_unregister_wb_capture(disp_info);

    //TODO: refine the card index function
    g_disp_info = NULL;

    gf_info("disp cature deinit out \n");
}


#define GF_CF_INT_TBL_ENTRY(_cf_id, _irq_mask)       \
    {                                                \
        .cf_id    = _cf_id,                          \
        .irq_mask = _irq_mask,                       \
    }

static gf_cf_irq_tbl_t cf_irq_tbl[] =
{
    GF_CF_INT_TBL_ENTRY(GF_CAPTURE_VIP1, INT_VIP1),
    GF_CF_INT_TBL_ENTRY(GF_CAPTURE_VIP2, INT_VIP2),
    GF_CF_INT_TBL_ENTRY(GF_CAPTURE_VIP3, INT_VIP3),
    GF_CF_INT_TBL_ENTRY(GF_CAPTURE_VIP4, INT_VIP4),
    GF_CF_INT_TBL_ENTRY(GF_CAPTURE_WB1,  INT_VSYNC1),
    GF_CF_INT_TBL_ENTRY(GF_CAPTURE_WB2,  INT_VSYNC2),
    GF_CF_INT_TBL_ENTRY(GF_CAPTURE_WB3,  INT_VSYNC3),
    GF_CF_INT_TBL_ENTRY(GF_CAPTURE_WB4,  INT_VSYNC4),
};

void gf_capture_interrupt_handle(disp_info_t *disp_info, unsigned int itrr)
{
    gf_capture_t *capture = NULL;
    int i = 0;

    for (; i < sizeof(cf_irq_tbl) / sizeof(gf_cf_irq_tbl_t); i++)
    {
        if (itrr & cf_irq_tbl[i].irq_mask)
        {
            capture = disp_find_capture(disp_info, cf_irq_tbl[i].cf_id);
            if (capture != NULL && (capture->flags & CAPTURE_ENABLE_B))
            {
                capture->notify_interrupt(disp_info, capture);
            }
        }

    }

}

void gf_capture_handle_event(disp_info_t *disp_info, gf_capture_id_t cf_id, gf_capture_event_t event)
{
    gf_capture_t *capture = NULL;
    int i = 0;

    gf_info("capture %d, event: %d \n", cf_id, event);

    capture = disp_find_capture(disp_info, cf_id);
    if (capture != NULL)
    {
        capture->event_handler(disp_info, capture, event);
    }

}

#undef __CONCAT
#undef CONCAT
#define __CONCAT(x,y) x##y
#define CONCAT(x,y)     __CONCAT(x,y)

void CONCAT(gf_krnl_get_capture_interface_, DRIVER_NAME)(unsigned int id, void **pdata, PFN_GF_CAPTURE_OPS_T *interface)
{
    *pdata = (void *)(g_disp_info);
    *interface = disp_capture_ctl;
}

EXPORT_SYMBOL(CONCAT(gf_krnl_get_capture_interface_, DRIVER_NAME));

#undef __CONCAT
#undef CONCAT

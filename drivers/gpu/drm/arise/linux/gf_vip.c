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
#include "gf_cbios.h"

int disp_vip_ops(disp_info_t *disp_info, gf_capture_t *capture, void *params);
void disp_vip_notify_interrupt(disp_info_t *disp_info, gf_capture_t *capture);
void disp_vip_event_handler(disp_info_t *disp_info, gf_capture_t *capture, gf_capture_event_t event);
void disp_vip_work_func(struct work_struct *work);

static gf_capture_desc gf_vip_descs[] =
{
    {
        .cf_fmt_id = GF_CAPTURE_VIP1,
        .cf_name   = "VIP1"
    },
    {
        .cf_fmt_id = GF_CAPTURE_VIP2,
        .cf_name   = "VIP2"
    },
    {
        .cf_fmt_id = GF_CAPTURE_VIP3,
        .cf_name   = "VIP3"
    },
    {
        .cf_fmt_id = GF_CAPTURE_VIP4,
        .cf_name   = "VIP4"
    }
};

static unsigned char cf_id2vip(gf_capture_id_t cf_fmt_id)
{
    if (cf_fmt_id == GF_CAPTURE_VIP1)
    {
        return 0;
    }

    if (cf_fmt_id == GF_CAPTURE_VIP2)
    {
        return 1;
    }

    if (cf_fmt_id == GF_CAPTURE_VIP3)
    {
        return 2;
    }

    if (cf_fmt_id == GF_CAPTURE_VIP4)
    {
        return 3;
    }

    return -1;
}


void disp_register_vip_capture(disp_info_t *disp_info)
{
    gf_capture_t *capture = NULL;
    int i = 0;

    for (i = 0; i < sizeof(gf_vip_descs) / sizeof(gf_capture_desc); i++)
    {
        capture = (gf_capture_t *)gf_calloc(sizeof(gf_capture_t));
        capture->cf_fmt_id = gf_vip_descs[i].cf_fmt_id;
        capture->cf_name = gf_vip_descs[i].cf_name;
        capture->cf_ctl = disp_vip_ops;
        capture->notify_interrupt = disp_vip_notify_interrupt;
        capture->event_handler = disp_vip_event_handler;
        disp_register_capture(disp_info, capture);
    }

}

void disp_unregister_vip_capture(disp_info_t *disp_info)
{
    gf_capture_t *capture = NULL;
    int i = 0;

    for (i = 0; i < sizeof(gf_vip_descs) / sizeof(gf_capture_desc); i++)
    {
        capture = disp_find_capture(disp_info, gf_vip_descs[i].cf_fmt_id);
        if (capture)
        {
            disp_unregister_capture(disp_info, capture);
            gf_free(capture);
        }
    }
}

int disp_vip_capture_init(gf_capture_t *capture, vip_op_params_t *mparams)
{
    vip_spec_info_t *info = NULL;

    capture->priv_info = gf_calloc(sizeof(vip_spec_info_t));

    info = (vip_spec_info_t *)capture->priv_info;
    info->mcapture = capture;
    info->vip    = cf_id2vip(capture->cf_fmt_id);
    info->fb_num = mparams->init_param.fbNum;

    INIT_WORK(&info->vip_work, disp_vip_work_func);

    capture->flags  |=  CAPTURE_INIT_B;

    gf_info("capture %s init\n", capture->cf_name);

    return 0;
}

int disp_vip_capture_deinit(gf_capture_t *capture, vip_op_params_t *mparams)
{
    vip_spec_info_t*  info = (vip_spec_info_t *)capture->priv_info;

    capture->flags  &=  ~CAPTURE_INIT_B;

    info->mcapture = capture;

    cancel_work_sync(&info->vip_work);

    gf_free(capture->priv_info);
    capture->priv_info = NULL;

    gf_info("capture %s deinit\n", capture->cf_name);

    return 0;
}

int disp_vip_query_caps(disp_info_t *disp_info, gf_capture_t *capture, vip_op_params_t *mparams)
{
    vip_spec_info_t*  info = (vip_spec_info_t *)capture->priv_info;
    gf_vip_set_t v_set = {0};
    int status = 0;

    v_set.vip        = info->vip;
    v_set.op         = GF_VIP_QUERY_CAPS;
    v_set.caps.mode  = mparams->query_caps_param.mode;
    disp_cbios_vip_ctl(disp_info, &v_set);

    if (status)
    {
        gf_error("capture %s query caps failed!! \n", capture->cf_name);
    }
    else
    {
        mparams->query_caps_param.modeNum = v_set.caps.mode_num;
        gf_info("capture %s query caps\n", capture->cf_name);
    }

    return status;
}

int disp_vip_set_mode(disp_info_t *disp_info, gf_capture_t *capture, vip_op_params_t *mparams)
{
    vip_spec_info_t*  info = (vip_spec_info_t *)capture->priv_info;
    gf_vip_set_t v_set = {0};
    int status = 0;

    v_set.vip        = info->vip;
    v_set.op         = GF_VIP_SET_MODE;
    v_set.mode.fmt   = mparams->set_mode_param.fmt;
    v_set.mode.chip  = mparams->set_mode_param.chip;
    v_set.mode.x_res = mparams->set_mode_param.xres;
    v_set.mode.y_res = mparams->set_mode_param.yres;
    v_set.mode.refs  = mparams->set_mode_param.refreshrate;
    disp_cbios_vip_ctl(disp_info, &v_set);

    if (status)
    {
        gf_error("capture %s set mode failed!! \n", capture->cf_name);
    }
    else
    {
        gf_info("capture %s set mode\n", capture->cf_name);
    }

    return status;
}

int disp_vip_set_buffer(disp_info_t *disp_info, gf_capture_t *capture, vip_op_params_t *mparams)
{
    vip_spec_info_t*  info = (vip_spec_info_t *)capture->priv_info;
    gf_vip_set_t v_set = {0};
    int status = 0;

    //TODO: get buffer phys address interface
    v_set.vip    = info->vip;
    v_set.op     = GF_VIP_SET_BUFFER;
    v_set.fb.fb_num  = info->fb_num;
    v_set.fb.fb_idx  = mparams->set_buffer_param.fbIdx;
    v_set.fb.fb_addr = mparams->set_buffer_param.fbAddr;

    status = disp_cbios_vip_ctl(disp_info, &v_set);

    if (status)
    {
        gf_error("capture %s set buffer failed!! \n", capture->cf_name);
    }
    else
    {
        //gf_info("capture %s set buffer, idx %d, buf %x\n", capture->cf_name, mparams->set_buffer_param.fbIdx, mparams->set_buffer_param.fbAddr);
    }

    return status;
}

int disp_vip_capture_enable(disp_info_t *disp_info, gf_capture_t *capture, vip_op_params_t *mparams)
{
    vip_spec_info_t*  info = (vip_spec_info_t *)capture->priv_info;
    gf_vip_set_t v_set = {0};
    int status = 0;

    v_set.vip    = info->vip;
    v_set.op = GF_VIP_ENABLE;

    status = disp_cbios_vip_ctl(disp_info, &v_set);

    if (status)
    {
        gf_error("capture %s enable failed!! \n", capture->cf_name);
    }
    else
    {
        capture->flags  |=  CAPTURE_ENABLE_B;
        gf_info("capture %s enable\n", capture->cf_name);
    }

    return status;
}

int disp_vip_capture_disable(disp_info_t *disp_info, gf_capture_t *capture, vip_op_params_t *mparams)
{
    vip_spec_info_t*  info = (vip_spec_info_t *)capture->priv_info;
    gf_vip_set_t v_set = {0};
    int status = 0;

    v_set.vip    = info->vip;
    v_set.op = GF_VIP_DISABLE;

    status = disp_cbios_vip_ctl(disp_info, &v_set);

    if (status)
    {
        gf_error("capture %s disable failed!! \n", capture->cf_name);
    }
    else
    {
        capture->flags  &=  ~CAPTURE_ENABLE_B;
        gf_info("capture %s disable\n", capture->cf_name);
    }

    return status;
}

void disp_vip_notify_interrupt(disp_info_t *disp_info, gf_capture_t *capture)
{
    vip_spec_info_t*  info = (vip_spec_info_t *)capture->priv_info;

    schedule_work(&info->vip_work);
}

void disp_vip_event_handler(disp_info_t *disp_info, gf_capture_t *capture, gf_capture_event_t event)
{
    vip_spec_info_t*  info = (vip_spec_info_t *)capture->priv_info;

    unsigned int flags = 0;


    gf_info("capture %s, event: %d \n", capture->cf_name, event);

//    info->cb(capture->cf_fmt_id, info->cb_data, flags);
}

void disp_vip_work_func(struct work_struct *work)
{
    vip_spec_info_t*  info = container_of(work, vip_spec_info_t, vip_work);
    gf_capture_t* capture  = info->mcapture;
    unsigned int flags = 0;

    if (!(capture->flags & CAPTURE_ENABLE_B))
    {
        return;
    }

    flags |= GF_CAPTURE_SET_BUFFER;

    info->cb(capture->cf_fmt_id, info->cb_data, flags);
}

int disp_vip_ops(disp_info_t *disp_info, gf_capture_t *capture, void *params)
{
    vip_op_params_t *mparams = (vip_op_params_t *)params;

    int   status = 0;

    switch(mparams->op)
    {
    case GF_CAPTURE_OP_INIT:
    {
        status = disp_vip_capture_init(capture, mparams);
    }
    break;

    case GF_CAPTURE_OP_DEINIT:
    {
        status = disp_vip_capture_deinit(capture, mparams);
    }
    break;

    case GF_CAPTURE_OP_ENABLE:
    {
        status = disp_vip_capture_enable(disp_info, capture, mparams);
    }
    break;

    case GF_CAPTURE_OP_DISABLE:
    {
        status = disp_vip_capture_disable(disp_info, capture, mparams);
    }
    break;

    case GF_CAPTURE_OP_QUERY_CAPS:
    {
        status = disp_vip_query_caps(disp_info, capture, mparams);
    }
    break;

    case GF_CAPTURE_OP_SET_MODE:
    {
        status = disp_vip_set_mode(disp_info, capture, mparams);
    }
    break;

    case GF_CAPTURE_OP_SET_BUFF:
    {
        status = disp_vip_set_buffer(disp_info, capture, mparams);
    }
    break;

    case GF_CAPTURE_OP_SET_CALLBACK:
    {
        vip_spec_info_t *info = (vip_spec_info_t *)capture->priv_info;
        info->cb = mparams->set_callback_param.cb;
        info->cb_data = mparams->set_callback_param.cb_data;
    }
    break;

    case GF_CAPTURE_OP_QUERY_CHIPINFO:
    {
        mparams->query_chipinfo_param.vipCard = GF_VIP_CARD_AVD7611;
    }
    break;

    case GF_CAPTURE_OP_GET_FRONT_MODE:
    {
        //FIXME: get mode info from the vip card later
        mparams->get_front_mode_param.hasInput  = 1;
        mparams->get_front_mode_param.mode.xRes = 1920;
        mparams->get_front_mode_param.mode.yRes = 1080;
        mparams->get_front_mode_param.mode.refreshrate = 6000;
        mparams->get_front_mode_param.mode.fmt = GF_VIP_FMT_RGB444_24BIT_SDR;
    }
    break;

    case GF_CAPTURE_OP_SET_PREFER_MODE:
    {
        //TODO
    }
    break;

    default:
    {
        gf_error("op not support on vip\n");
    }
    break;
    }

    return status;
}

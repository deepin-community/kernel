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
#include "gf_cbios.h"
#include "os_interface.h"
#include "gf_i2c.h"
static int gf_i2c_xfer(struct i2c_adapter *i2c_adapter, struct i2c_msg *msgs, int num)
{
    struct gf_i2c_adapter *gf_adapter = i2c_get_adapdata(i2c_adapter);
    struct drm_device *dev = gf_adapter->dev;
    gf_connector_t *gf_connector = (gf_connector_t *)(gf_adapter->pgf_connector);
    gf_card_t *gf_card = dev->dev_private;
    disp_info_t *disp_info = (disp_info_t *)gf_card->disp_info;
    gf_i2c_param_t i2c_param;
    int i = 0, retval = 0;
    for (i = 0; i < num; i++)
    {
        DRM_DEBUG_KMS("xfer: num: %d/%d, len:%d, flags:%#x\n\n", i + 1,
                      num, msgs[i].len, msgs[i].flags);
        /*gf_info("xfer: num MUTEX: %d/%d, len:%d, flags:%#x\n\n", i + 1,
                      num, msgs[i].len, msgs[i].flags);*/
        gf_memset(&i2c_param, 0, sizeof(gf_i2c_param_t));
        i2c_param.use_dev_type = 1;
        i2c_param.device_id = gf_connector->output_type;
        i2c_param.slave_addr = ((msgs[i].addr)<<1);
        i2c_param.offset =  0;
        i2c_param.buf = msgs[i].buf;
        i2c_param.buf_len = msgs[i].len;
        i2c_param.request_type = GF_I2C_ERR_TYPE;
        if(i2c_param.slave_addr == 0x6E) //DDC operation
        {
            i2c_param.request_type = GF_I2C_DDCCI;
        }
        if (msgs[i].flags & I2C_M_RD)
            i2c_param.op = GF_I2C_READ;
        else
            i2c_param.op = GF_I2C_WRITE;

        //if i2c operation is write, but not used to adjust brightness, just skip it
        if ((i2c_param.op == GF_I2C_WRITE) && (i2c_param.request_type != GF_I2C_DDCCI))
        {
            continue;
        }

        gf_mutex_lock(gf_connector->conn_mutex);
        retval = disp_cbios_i2c_ctrl(disp_info, &i2c_param);
        gf_mutex_unlock(gf_connector->conn_mutex);
        if (retval < 0)
            return retval;
    }
    return num;
}

static u32 gf_i2c_func(struct i2c_adapter *adapter)
{
    return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm gf_i2c_algo =
{
    .master_xfer = gf_i2c_xfer,
    .functionality = gf_i2c_func,
};

struct gf_i2c_adapter *gf_i2c_adapter_create(struct drm_device *dev, struct drm_connector *connector)
{
    struct i2c_adapter *adapter;
    struct gf_i2c_adapter *gf_adapter;
    gf_connector_t *gf_connector = to_gf_connector(connector);
    char *name = NULL;
    int ret = 0;

    switch (gf_connector->output_type)
    {
    case DISP_OUTPUT_CRT:
    {
        name = "CRT";
    }
    break;
    case DISP_OUTPUT_DP1:
    {
        name = "DP1";
    }
    break;
    case DISP_OUTPUT_DP2:
    {
        name = "DP2";
    }
    break;
    case DISP_OUTPUT_DP3:
    {
        name = "DP3";
    }
    break;
    case DISP_OUTPUT_DP4:
    {
        name = "DP4";
    }
    break;
    default:
    {
        DRM_DEBUG_KMS("skip create i2c adapter for output 0x%x\n", gf_connector->output_type);
        return NULL;
    }
    }

    gf_adapter = gf_calloc(sizeof(struct gf_i2c_adapter));
    if (!gf_adapter)
    {
        DRM_ERROR("failed to allo gf_adapter\n");
        return ERR_PTR(-ENOMEM);
    }

    adapter = &gf_adapter->adapter;
    adapter->owner = THIS_MODULE;
#if DRM_VERSION_CODE < KERNEL_VERSION(6,8,0)
    adapter->class = I2C_CLASS_DDC;
#endif
    adapter->dev.parent = dev->dev;
    gf_adapter->dev = dev;
    gf_adapter->pgf_connector = (void *)gf_connector;
    i2c_set_adapdata(adapter, gf_adapter);

    gf_vsnprintf(adapter->name, sizeof(adapter->name), "gf_i2c_%s", name);

    adapter->algo = &gf_i2c_algo;

    ret = i2c_add_adapter(adapter);
    if (ret)
    {
        DRM_ERROR("failed to register i2c adapter\n");

        gf_free(gf_adapter);
        gf_adapter = NULL;

        return ERR_PTR(ret);
    }

    gf_connector->i2c_adapter = gf_adapter;

    return gf_adapter;
}

void gf_i2c_adapter_destroy(struct gf_i2c_adapter *gf_adapter)
{
    if (!gf_adapter)
    {
        return;
    }

    i2c_del_adapter(&gf_adapter->adapter);

    gf_free(gf_adapter);
}

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
#ifndef __GF_I2C_H__
#define __GF_I2C_H__

struct gf_i2c_adapter
{
    struct i2c_adapter adapter;
    struct drm_device *dev;
    void *pgf_connector;
};

struct gf_i2c_adapter *gf_i2c_adapter_create(struct drm_device *dev, struct drm_connector *connector);

void gf_i2c_adapter_destroy(struct gf_i2c_adapter *gf_adapter);

#endif

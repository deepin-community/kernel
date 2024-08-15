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

#ifndef _GF_SINK_H_
#define _GF_SINK_H_

struct gf_sink_create_data
{
    int output_type;
};

struct gf_sink
{
    unsigned char edid_data[EDID_BUF_SIZE];
    int output_type;
    struct kref refcount;
};

struct gf_sink* gf_sink_create(struct gf_sink_create_data *create_data);

void gf_sink_get(struct gf_sink *sink);

void gf_sink_put(struct gf_sink *sink);

bool gf_sink_is_edid_valid(struct gf_sink *sink);

#endif

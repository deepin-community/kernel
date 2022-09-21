/*
 * JM7200 GPU driver
 *
 * Copyright (c) 2018 ChangSha JingJiaMicro Electronics Co., Ltd.
 *
 * Author:
 *      rfshen <jjwgpu@jingjiamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef __MWV206_FB_INTERPRENT_EDID_H
#define __MWV206_FB_INTERPRENT_EDID_H

#include <linux/fb.h>


void mwv206fb_edid_to_monspecs(unsigned char *edid, struct fb_monspecs *specs);
void mwv206fb_destroy_modedb(struct fb_videomode *modedb);

#endif
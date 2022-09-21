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

#include "gf.h"
#include "os_interface.h"
#include "gf_driver.h"
#include "gf_ioctl.h"
#include "gf_version.h"
#include "gf_debugfs.h"

char *gf_fb_mode = NULL; //"800x600-32@60";
#ifdef GF_HW_NULL
int   gf_fb      = 0;
#else
int   gf_fb      = 1;
#endif

static int __init gf_init(void)
{
    int ret = -ENOMEM;

    gf_info("%s\n", DRIVER_VENDOR);
    //gf_info("Version: %0d.%02d.%02d%s Build on: %s\n", DRIVER_MAJOR, DRIVER_MINOR, DRIVER_PATCHLEVEL, DRIVER_BRANCH, DRIVER_DATE);    
    gf_info("Version: %0x.%02x.%02x-%02x%s Build on: %s\n", DRIVER_MAJOR, DRIVER_MINOR, DRIVER_PATCHLEVEL, DRIVER_RELEASE, DRIVER_CLASS, DRIVER_DATE);

#if GF_MALLOC_TRACK | GF_ALLOC_PAGE_TRACK | GF_MAP_PAGES_TRACK | GF_MAP_IO_TRACK
    gf_mem_track_init();
#endif
    ret = gf_register_driver();

    if(ret)
    {
        gf_error("register_driver() failed in init. ret:%x.\n", ret);
    }

    return 0;
}

static void __exit gf_exit(void)
{
    gf_unregister_driver();

#if GF_MALLOC_TRACK | GF_ALLOC_PAGE_TRACK | GF_MAP_PAGES_TRACK | GF_MAP_IO_TRACK
    gf_mem_track_list_result();
#endif

    gf_info("exit driver.\n");
}

module_init(gf_init);
module_exit(gf_exit);
MODULE_LICENSE("GPL");

#ifndef KERNEL_2_4
module_param(gf_fb, int, 0);
module_param(gf_fb_mode, charp, 0);
#else
MODULE_PARM(gf_fb_mode, "s");
#endif

MODULE_PARM_DESC(gf_fb, "enable gf fb driver");
MODULE_PARM_DESC(gf_fb_mode, "Initial video mode:<xres>x<yres>-<depth>@<refresh>");
MODULE_PARM_DESC(gf_flip, "enable frame buffer flip support");


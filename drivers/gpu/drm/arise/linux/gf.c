/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
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
    gf_info("Version: %02x.%02x.%02x%s Build on: %s\n", DRIVER_MAJOR, DRIVER_MINOR, DRIVER_PATCHLEVEL, DRIVER_CLASS, DRIVER_DATE);

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
MODULE_VERSION(DRIVER_VERSION_CHAR);
MODULE_DESCRIPTION("Glenfly DRM PRO Driver");

#ifndef KERNEL_2_4
module_param(gf_fb, int, 0);
module_param(gf_fb_mode, charp, 0);
#else
MODULE_PARM(gf_fb_mode, "s");
#endif

MODULE_PARM_DESC(gf_fb, "enable gf fb driver");
MODULE_PARM_DESC(gf_fb_mode, "Initial video mode:<xres>x<yres>-<depth>@<refresh>");
MODULE_PARM_DESC(gf_flip, "enable frame buffer flip support");


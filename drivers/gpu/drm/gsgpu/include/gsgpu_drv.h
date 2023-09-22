#ifndef __GSGPU_DRV_H__
#define __GSGPU_DRV_H__

#include <linux/firmware.h>
#include <linux/platform_device.h>

#include "gsgpu_shared.h"

#define DRIVER_AUTHOR		"Loongson graphics driver team"

#define DRIVER_NAME		"gsgpu"
#define DRIVER_DESC		"GS GPU Driver"
#define DRIVER_DATE		"20200501"

long gsgpu_drm_ioctl(struct file *filp,
		      unsigned int cmd, unsigned long arg);

#endif /* __GSGPU_DRV_H__ */

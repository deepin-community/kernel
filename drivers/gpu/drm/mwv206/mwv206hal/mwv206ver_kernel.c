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
#include "mwv206.h"
#include "mwv206dev.h"
#include "mwv206ioctl.h"

int FUNC206HAL336(V206DEV025 *pDev, long arg)
{
	int ret;

	ret = copy_to_user((char __user *)arg, MWV206_KVERSTR, strlen(MWV206_KVERSTR));
	if (ret != 0) {
		V206KDEBUG002("###ERR[%s] copy_to_user error [%d].\n", __FUNCTION__, ret);
		return ret;
	}
	return 0;
}
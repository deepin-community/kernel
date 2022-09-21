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
#ifndef _MWV206_GPIO_H_
#define _MWV206_GPIO_H_

#include "mwv206dev.h"


#define V206GPIO001               (0x400000 + 0x5400)
#define V206GPIO002              (0x400000 + 0x5410)

#define V206GPIO003              (0x400000 + 0x5404)
#define V206GPIO004             (0x400000 + 0x5414)


#define V206GPIO005        (0x400000 + 0x5408)
#define V206GPIO006       (0x400000 + 0x5418)

#define V206GPIO007(reg)        V206DEV001(reg)
#define V206GPIO008(reg, val)   V206DEV002(reg, val)



int FUNC206HAL142(V206DEV025 *pDev, int port, int direction);


int FUNC206HAL144(V206DEV025 *pDev, int port, int *pValue);


int FUNC206HAL143(V206DEV025 *pDev, int port, int outValue);

#endif
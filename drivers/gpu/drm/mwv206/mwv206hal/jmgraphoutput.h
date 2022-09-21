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
#ifndef _JMGRAPH_OUTPUT_H_
#define _JMGRAPH_OUTPUT_H_

#include "mwv206dev.h"

int FUNC206HAL192(V206DEV025 * dev,
		int screen,
		int htotal,
		int hactive,
		int hfrontporch,
		int hsync,
		int vtotal,
		int vactive,
		int vfrontporch,
		int vsync,
		int framerate,
		int isInterleaved,
		int outputContent);

#endif
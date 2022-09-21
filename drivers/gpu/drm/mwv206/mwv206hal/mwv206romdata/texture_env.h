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
#ifndef _MWV206_TEXTUREENV_H_
#define _MWV206_TEXTUREENV_H_

static unsigned int g_textureEnv_low[] = {

	0xba980000, 0xba980000, 0x0,        0xba980000, 0xba980000,
	0xbaa00000, 0xaaa0c000, 0x0,        0xaa890644, 0xbba00600,
	0xbaa00000, 0xaaa0c000, 0x0,        0xaa890644, 0xbba00600,
	0xbaa00000, 0xaaa0c000, 0x0,        0xaa890644, 0xbba00600,
	0xbaa00000, 0xaaa0c000, 0xbaa00000, 0xaa890644, 0xbba00600,
	0xbaa00000, 0xaaa0c000, 0xaaa1264c, 0xaa890644, 0xbba00600,
};

static unsigned int g_textureEnv_high[] = {

	0x02eaa000, 0x02aaa800, 0x0,        0x02aaa800, 0x02aaa800,
	0x02ea8000, 0x02ea8000, 0x0,        0x02ea8000, 0x02ea8000,
	0x02eaa000, 0x02aaa800, 0x0,        0x02aaa800, 0x02aaa800,
	0x02eaa000, 0x02aaa800, 0x0,        0x02aa4a8b, 0x02eea080,
	0x02ea8000, 0x02ea8000, 0x02ea8000, 0x02ea8000, 0x02ea8000,
	0x02eaa000, 0x02aaa800, 0x02ea8000, 0x02aaa800, 0x02aaa800,
};
#endif
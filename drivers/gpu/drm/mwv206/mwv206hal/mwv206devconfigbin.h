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
#ifndef _MWV206_DEV_CONFIG_BIN_H_
#define  _MWV206_DEV_CONFIG_BIN_H_

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct mwv206_dev_config_bin{
	char magic[4];

	uint8_t version[3];

	uint8_t resved;


	uint32_t crc32;
};

#define MWV206DEV_CONFIGBIN_HEADER_SIZE   (sizeof(struct mwv206_dev_config_bin))

#define MAJOR_VER    1
#define MINOR_VER    0
#define RELEASE_VER  4



#ifdef __cplusplus
}
#endif

#endif
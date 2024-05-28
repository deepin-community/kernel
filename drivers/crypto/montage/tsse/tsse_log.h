/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_LOG_H__
#define __TSSE_LOG_H__

#define tsse_dev_err(tssedev, fmt, ...)                                        \
	dev_err(TSSEDEV_TO_DEV(tssedev), "%s %d: " fmt, __func__, __LINE__,    \
		##__VA_ARGS__)
#define tsse_dev_warn(tssedev, fmt, ...)                                       \
	dev_warn(TSSEDEV_TO_DEV(tssedev), "%s %d: " fmt, __func__, __LINE__,   \
		 ##__VA_ARGS__)
#define tsse_dev_info(tssedev, fmt, ...)                                       \
	dev_info(TSSEDEV_TO_DEV(tssedev), "%s %d: " fmt, __func__, __LINE__,   \
		 ##__VA_ARGS__)
#define tsse_dev_dbg(tssedev, fmt, ...)                                        \
	dev_dbg(TSSEDEV_TO_DEV(tssedev), "%s %d: " fmt, __func__, __LINE__,    \
		##__VA_ARGS__)

#endif

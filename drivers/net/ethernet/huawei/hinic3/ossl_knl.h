/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef OSSL_KNL_H
#define OSSL_KNL_H

#include "ossl_knl_linux.h"
#include <linux/types.h>

#define sdk_err(dev, format, ...) dev_err(dev, "[COMM]" format, ##__VA_ARGS__)
#define sdk_warn(dev, format, ...) dev_warn(dev, "[COMM]" format, ##__VA_ARGS__)
#define sdk_notice(dev, format, ...) dev_notice(dev, "[COMM]" format, ##__VA_ARGS__)
#define sdk_info(dev, format, ...) dev_info(dev, "[COMM]" format, ##__VA_ARGS__)

#define nic_err(dev, format, ...) dev_err(dev, "[NIC]" format, ##__VA_ARGS__)
#define nic_warn(dev, format, ...) dev_warn(dev, "[NIC]" format, ##__VA_ARGS__)
#define nic_notice(dev, format, ...) dev_notice(dev, "[NIC]" format, ##__VA_ARGS__)
#define nic_info(dev, format, ...) dev_info(dev, "[NIC]" format, ##__VA_ARGS__)

#ifndef BIG_ENDIAN
#define BIG_ENDIAN    0x4321
#endif

#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN    0x1234
#endif

#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif
/* X86 */
#define BYTE_ORDER    LITTLE_ENDIAN
#define USEC_PER_MSEC	1000L
#define MSEC_PER_SEC	1000L

/* Waiting for 50 us */
#define WAIT_USEC_50    50L

#endif /* OSSL_KNL_H */

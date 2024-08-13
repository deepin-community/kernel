/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_KERNEL_H
#define SSS_KERNEL_H

#include "sss_linux_kernel.h"

#define sdk_err(dev, format, ...) dev_err(dev, "[BASE]" format, ##__VA_ARGS__)
#define sdk_warn(dev, format, ...) dev_warn(dev, "[BASE]" format, ##__VA_ARGS__)
#define sdk_notice(dev, format, ...) dev_notice(dev, "[BASE]" format, ##__VA_ARGS__)
#define sdk_info(dev, format, ...) dev_info(dev, "[BASE]" format, ##__VA_ARGS__)

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

#endif /* OSSL_KNL_H */

/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef PUB_PRINT_H
#define PUB_PRINT_H

#if defined(__KERNEL__)
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/inetdevice.h>
#include <linux/io.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/stddef.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/ethtool.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/fsl_devices.h>
#include <linux/mii.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/if_arp.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/poll.h>
#include <linux/workqueue.h>
#include <linux/proc_fs.h>
#include <linux/cpumask.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <asm/mman.h>
#include <asm/atomic.h>
#include <asm/smp.h>
#include <linux/kernel.h>
#else
#include <string.h>
#include <stdarg.h>
#endif
#include "cmdk.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PM_DEBUG ((u8)0x01) /**< 默认仅在debug版本显示 */
#define PM_INFO ((u8)0x02)
#define PM_WARN ((u8)0x04)
#define PM_ERROR ((u8)0x08)
#define PM_FATAL ((u8)0x10)
#define DEFAULT_LEVEL ((u8)0x1E) /**< 默认不显示debug信息 */
/** @} 输出控制级别 */

#define MAX_LEVEL_MASK ((u8)0x1F) /**< 全级别掩码 */

#define MAX_LEVEL_TYPE ((u8)0x05) /**< 定义5级打印 */
#define INVALID_MODULE_ID 0xFF /**< 无效的模块id */

#define MAX_MDL_NAME_LEN 24 /**< 打印模块名称最大长度 */
#define MAX_MODULE_ID ((u8)0x80) /**< 最大模块号,目前定义了128个模块 */
#define MAX_MDL_PRINT_BUF_LEN 512 /**< 打印最大buffer长度 */

#define PM_FLAG_ON 1 /**< 打印flag打开 */
#define PM_FLAG_OFF 0 /**< 打印flag关闭 */

extern u8 g_ucBySelfId; /**< 默认打印模块id */

/**************************************************************************
 *                               宏定义                                      *
 **************************************************************************/
/** 通用打印封装 */
#define PUB_PRINTF printk

#ifdef __cplusplus
}
#endif

#endif /* PUB_PRINT_H */

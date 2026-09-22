/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */
#ifndef DISTRO_VER_H
#define DISTRO_VER_H

#if defined(RHEL_RELEASE_CODE)

/*
 * RHEL 9.x 版本判断 - 必须从高到低！
 *
 * 原因：高版本号会满足低版本的条件 (9.6 >= 9.2 为真)
 *
 * 当前支持版本：
 *   - RHEL 9.6: 使用 RHEL_9_6 宏
 *   - RHEL 9.2-9.5: 使用 RHEL_9_2 宏（API 相同）
 *
 * 如何添加新版本（例如 RHEL 9.4）：
 *   1. 在对应位置插入 #elif 块（必须从高到低顺序）
 *   2. 在 rhel_kcompat.h 中添加 #ifdef RHEL_9_4 块
 *   3. 定义该版本所需的 API 兼容性宏
 *
 * 示例：
 *   #if (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 6))
 *   #define RHEL_9_6
 *   #elif (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 4))  ← 插入点
 *   #define RHEL_9_4
 *   #elif (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 2))
 *   #define RHEL_9_2
 *   #endif
 */

/* RHEL 9.6 及以上 */
#if (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 6))
#define RHEL_9_6

/* RHEL 9.2 - 9.5 */
#elif (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(9, 2))
#define RHEL_9_2

#endif

#if (RHEL_RELEASE_VERSION(8, 6) == RHEL_RELEASE_CODE)
#define RHEL_8_6
#endif

#if (RHEL_RELEASE_VERSION(8, 5) == RHEL_RELEASE_CODE)
#define RHEL_8_5
#endif

#if (RHEL_RELEASE_VERSION(8, 4) == RHEL_RELEASE_CODE)
#define RHEL_8_4
#endif

#if (RHEL_RELEASE_VERSION(8, 3) == RHEL_RELEASE_CODE)
#define RHEL_8_3
#endif

#if (RHEL_RELEASE_VERSION(7, 9) == RHEL_RELEASE_CODE)
#define RHEL_7_9
#endif

#if (RHEL_RELEASE_VERSION(8, 2) == RHEL_RELEASE_CODE)
#define RHEL_8_2
#endif

#if (RHEL_RELEASE_VERSION(8, 1) == RHEL_RELEASE_CODE)
#define RHEL_8_1
#endif

#if (RHEL_RELEASE_VERSION(7, 8) == RHEL_RELEASE_CODE)
#define RHEL_7_8
#endif

#if (RHEL_RELEASE_VERSION(7, 7) == RHEL_RELEASE_CODE)
#define RHEL_7_7
#endif

#if (RHEL_RELEASE_VERSION(8, 0) == RHEL_RELEASE_CODE)
#define RHEL_8_0
#endif

#if (RHEL_RELEASE_VERSION(7, 6) == RHEL_RELEASE_CODE)
#define RHEL_7_6
#endif

#if (RHEL_RELEASE_VERSION(7, 5) == RHEL_RELEASE_CODE)
#define RHEL_7_5
#endif

#if (RHEL_RELEASE_VERSION(7, 4) == RHEL_RELEASE_CODE)
#define RHEL_7_4
#endif

#if (RHEL_RELEASE_VERSION(7, 2) == RHEL_RELEASE_CODE)
#define RHEL_7_2
#endif

#endif /* RHEL_RELEASE_CODE */

#ifdef CONFIG_SUSE_KERNEL
#ifndef SLE_VERSION
#define SLE_VERSION(a, b, c) KERNEL_VERSION(a, b, c)
#endif
#define SLE_LOCALVERSION(a, b, c) KERNEL_VERSION(a, b, c)

#if (KERNEL_VERSION(4, 12, 14) == LINUX_VERSION_CODE &&     \
     (KERNEL_VERSION(94, 41, 0) == SLE_LOCALVERSION_CODE || \
      (KERNEL_VERSION(95, 0, 0) <= SLE_LOCALVERSION_CODE && \
       KERNEL_VERSION(96, 0, 0) > SLE_LOCALVERSION_CODE)))
/* SLES12 SP4 GM is 4.12.14-94.41 and update kernel is 4.12.14-95.x. */
#define SLE_VERSION_CODE SLE_VERSION(12, 4, 0)
#define SLES_12_SP_4
#elif (KERNEL_VERSION(4, 12, 14) == LINUX_VERSION_CODE && \
       KERNEL_VERSION(25, 23, 0) <= SLE_LOCALVERSION_CODE)
/* SLES15 SP1 Beta1 is 4.12.14-25.23 */
#define SLE_VERSION_CODE SLE_VERSION(15, 1, 0)
#define SLES_15_SP_1
#endif

#if (KERNEL_VERSION(4, 12, 14) == LINUX_VERSION_CODE &&     \
     (KERNEL_VERSION(23, 0, 0) == SLE_LOCALVERSION_CODE ||  \
      KERNEL_VERSION(2, 0, 0) == SLE_LOCALVERSION_CODE ||   \
      KERNEL_VERSION(136, 0, 0) == SLE_LOCALVERSION_CODE || \
      (KERNEL_VERSION(25, 0, 0) <= SLE_LOCALVERSION_CODE && \
       KERNEL_VERSION(25, 23, 0) > SLE_LOCALVERSION_CODE)))
#define SLE_VERSION_CODE SLE_VERSION(15, 0, 0)
#define SLES_15
#endif

#if KERNEL_VERSION(5, 3, 18) <= LINUX_VERSION_CODE
#if KERNEL_VERSION(46, 0, 0) <= SLE_LOCALVERSION_CODE
#define SLES_15_SP_3
#else
#define SLE_VERSION_CODE SLE_VERSION(15, 2, 0)
#define SLES_15_SP_2
#endif
#endif

#if ((KERNEL_VERSION(4, 4, 73) == LINUX_VERSION_CODE ||       \
      KERNEL_VERSION(4, 4, 82) == LINUX_VERSION_CODE ||       \
      KERNEL_VERSION(4, 4, 92) == LINUX_VERSION_CODE) ||      \
     (KERNEL_VERSION(4, 4, 103) == LINUX_VERSION_CODE &&      \
      (KERNEL_VERSION(6, 33, 0) == SLE_LOCALVERSION_CODE ||   \
       KERNEL_VERSION(6, 38, 0) == SLE_LOCALVERSION_CODE)) || \
     (KERNEL_VERSION(4, 4, 114) <= LINUX_VERSION_CODE &&      \
      KERNEL_VERSION(4, 5, 0) > LINUX_VERSION_CODE &&         \
      KERNEL_VERSION(94, 0, 0) <= SLE_LOCALVERSION_CODE &&    \
      KERNEL_VERSION(95, 0, 0) > SLE_LOCALVERSION_CODE))
/* SLES12 SP3 GM is 4.4.73-5 and update kernels are 4.4.82-6.3.
 * SLES12 SP3 updates not conflicting with SP2 are: 4.4.{82,92}
 * SLES12 SP3 updates conflicting with SP2 are:
 * - 4.4.103-6.33.1, 4.4.103-6.38.1
 * - 4.4.{114,120}-94.nn.y
 */
#define SLE_VERSION_CODE SLE_VERSION(12, 3, 0)
#define SLES_12_SP_3
#endif /* LINUX_VERSION_CODE == KERNEL_VERSION(x,y,z) */

#endif /* CONFIG_SUSE_KERENL */

#ifdef UTS_UBUNTU_RELEASE_ABI
#define UBUNTU_VERSION_CODE \
	(((~0xFF & LINUX_VERSION_CODE) << 8) + UTS_UBUNTU_RELEASE_ABI)

#define UBUNTU_VERSION(a, b, c, d) ((KERNEL_VERSION(a, b, 0) << 8) + (d))

/* Ubuntu 24.04 及以上 (kernel >= 6.5) */
#if (KERNEL_VERSION(6, 5, 0) <= LINUX_VERSION_CODE)
#define UBUNTU_2404
#elif (UBUNTU_VERSION(5, 15, 0, 30) <= UBUNTU_VERSION_CODE)
#define UBUNTU_220404
#elif (UBUNTU_VERSION(5, 13, 0, 28) <= UBUNTU_VERSION_CODE)
#define UBUNTU_200404
#elif (UBUNTU_VERSION(5, 11, 0, 27) <= UBUNTU_VERSION_CODE)
#define UBUNTU_200403
#elif (UBUNTU_VERSION(5, 8, 0, 48) <= UBUNTU_VERSION_CODE)
#define UBUNTU_200402
#elif (UBUNTU_VERSION(5, 4, 0, 26) <= UBUNTU_VERSION_CODE)
#define UBUNTU_2004
#else
#define UBUNTU_1804
#endif
#endif /* UTS_UBUNTU_RELEASE_ABI */

#ifdef KYLIN_RELEASE_CODE
#if (KYLIN_RELEASE_VERSION(10, 4) == KYLIN_RELEASE_CODE)
#define KYLIN_V10_4
#elif (KYLIN_RELEASE_VERSION(11, 0) == KYLIN_RELEASE_CODE)
#define KYLIN_V11_0
#endif
#endif

#ifdef __OFED_BUILD__
#define OFED_VERSION(a, b) ((a << 16) + (b << 8))

#if (OFED_VERSION(5, 8) == OFED_VERSION_CODE)
#define __OFED_5_8__
#elif (OFED_VERSION(24, 04) == OFED_VERSION_CODE)
#define __OFED_24_04__
#elif (OFED_VERSION(24, 10) == OFED_VERSION_CODE)
#define __OFED_24_10__
#elif (OFED_VERSION(26, 04) == OFED_VERSION_CODE)
#define __OFED_26_04__
#endif

#if (OFED_VERSION(23, 10) == OFED_VERSION_CODE)
#define __OFED_23_10__
#endif
#endif

#endif /* DISTRO_VER_H */

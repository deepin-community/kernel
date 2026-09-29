/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#ifndef _LINUX_IEE_INIT_H
#define _LINUX_IEE_INIT_H

#define NO_BLOCK_MAPPINGS	BIT(0)
#define NO_CONT_MAPPINGS	BIT(1)
#define NO_EXEC_MAPPINGS	BIT(2)	/* assumes FEAT_HPDS is not used */
#define IS_IEE_MAPPINGS		BIT(3)

#define TOP_PAGE_TABLE(level)	((level) == CONFIG_PGTABLE_LEVELS)

extern char iee_init_data_begin[];
extern char iee_init_data_end[];
#ifdef CONFIG_PTP
extern char __iee_ptp_data_start[];
extern char __iee_ptp_data_end[];
#endif
#ifdef CONFIG_CREDP
extern char __iee_cred_data_start[];
extern char __iee_cred_data_end[];
#endif
#ifdef CONFIG_IEE_SELINUX_P
extern char __iee_selinux_data_start[];
extern char __iee_selinux_data_end[];
#endif
#ifdef CONFIG_VARP
extern char __iee_varp_data_start[];
extern char __iee_varp_data_end[];
#endif

extern struct mutex fixmap_lock;

#endif

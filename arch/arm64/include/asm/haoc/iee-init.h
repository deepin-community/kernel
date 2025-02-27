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

extern char iee_init_data_begin[];
extern char iee_init_data_end[];

#endif

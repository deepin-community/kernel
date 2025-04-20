/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Lyu Jinglin <lvjl2022@zgclab.edu.cn>
 *          Zhang Shiyang <zhangsy2023@zgclab.edu.cn>
 */

#ifndef _LINUX_HAOC_H
#define _LINUX_HAOC_H

#include <linux/types.h>
#include <linux/mm.h>

void _iee_memset(unsigned long __unused, void *ptr, int data, size_t n);
#ifdef CONFIG_IEE_PTRP
void _iee_set_token_pgd(unsigned long __unused, struct task_struct *tsk,
			pgd_t *pgd);
void _iee_init_token(unsigned long __unused, struct task_struct *tsk);
void _iee_invalidate_token(unsigned long __unused, struct task_struct *tsk);
void _iee_validate_token(unsigned long __unused, struct task_struct *tsk);
#endif

#endif

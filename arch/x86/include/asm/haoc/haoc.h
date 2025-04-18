/* SPDX-License-Identifier: GPL-2.0 */
/*
 * HAOC feature support
 *
 * Copyright (C) 2025 ZGCLAB
 * Authors: Shu Hang <shuh2023@zgclab.edu.cn>
 *          Hu Bing <hubing2023@zgclab.edu.cn>
 */

#ifndef _LINUX_HAOC_H
#define _LINUX_HAOC_H

#include <linux/types.h>
#include <linux/sched.h>

void _iee_memcpy(unsigned long __unused, void *dst, void *src, size_t n);
void _iee_memset(unsigned long __unused, void *ptr, int data, size_t n);
void _iee_set_freeptr(unsigned long __unused, void **pptr, void *ptr);
unsigned long _iee_test_and_clear_bit(unsigned long __unused,
					long nr, unsigned long *addr);
#ifdef CONFIG_IEE_PTRP
void _iee_set_token_pgd(unsigned long __unused, struct task_struct *tsk,
		pgd_t *pgd);
void _iee_invalidate_token(unsigned long __unused, struct task_struct *tsk);
void _iee_validate_token(unsigned long __unused, struct task_struct *tsk);
#endif
#endif

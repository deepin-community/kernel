/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HYGON Platform Security Processor (PSP) driver interface
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 */

#ifndef __CCP_HYGON_PSP_DEV_H__
#define __CCP_HYGON_PSP_DEV_H__

#include <linux/mutex.h>

#include "sp-dev.h"

#include "../psp-dev.h"
#include "../sev-dev.h"

/*
 * Hooks table: a table of function and variable pointers filled in
 * when psp init.
 */
extern struct hygon_psp_hooks_table {
	bool sev_dev_hooks_installed;
	struct mutex *sev_cmd_mutex;
	int (*__sev_do_cmd_locked)(int cmd, void *data, int *psp_ret);
	int (*__sev_platform_init_locked)(int *error);
	long (*sev_ioctl)(struct file *file, unsigned int ioctl, unsigned long arg);
} hygon_psp_hooks;

int fixup_hygon_psp_caps(struct psp_device *psp);

#endif	/* __CCP_HYGON_PSP_DEV_H__ */

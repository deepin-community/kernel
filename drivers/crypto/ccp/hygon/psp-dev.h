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
#include <linux/bits.h>

#include "sp-dev.h"

#include "../psp-dev.h"
#include "../sev-dev.h"

#ifdef CONFIG_HYGON_PSP2CPU_CMD
#define PSP_X86_CMD			BIT(2)
#define P2C_NOTIFIERS_MAX		16
#endif

/*
 * Hooks table: a table of function and variable pointers filled in
 * when psp init.
 */
extern struct hygon_psp_hooks_table {
	bool sev_dev_hooks_installed;
	struct mutex *sev_cmd_mutex;
	bool *psp_dead;
	int *psp_timeout;
	int *psp_cmd_timeout;
	int (*sev_cmd_buffer_len)(int cmd);
	int (*__sev_do_cmd_locked)(int cmd, void *data, int *psp_ret);
	int (*__sev_platform_init_locked)(int *error);
	int (*__sev_platform_shutdown_locked)(int *error);
	int (*sev_wait_cmd_ioc)(struct sev_device *sev,
				unsigned int *reg, unsigned int timeout);
	long (*sev_ioctl)(struct file *file, unsigned int ioctl, unsigned long arg);
} hygon_psp_hooks;

int fixup_hygon_psp_caps(struct psp_device *psp);
int sp_request_hygon_psp_irq(struct sp_device *sp, irq_handler_t handler,
			     const char *name, void *data);

#endif	/* __CCP_HYGON_PSP_DEV_H__ */

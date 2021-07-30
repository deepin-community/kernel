/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * HYGON CSV driver interface
 *
 * Copyright (C) 2024 Hygon Info Technologies Ltd.
 *
 * Author: Liyang Han <hanliyang@hygon.cn>
 */

#ifndef __CCP_HYGON_CSV_DEV_H__
#define __CCP_HYGON_CSV_DEV_H__

#include <linux/fs.h>
#include <linux/psp-sev.h>
#include <linux/bits.h>

#define CSV_FW_FILE		"hygon/csv.fw"

#define PSP_RBCTL_X86_WRITES		BIT(31)
#define PSP_RBCTL_RBMODE_ACT		BIT(30)
#define PSP_RBCTL_CLR_INTSTAT		BIT(29)
#define PSP_RBTAIL_QHI_TAIL_SHIFT	16
#define PSP_RBTAIL_QHI_TAIL_MASK	0x7FF0000
#define PSP_RBTAIL_QLO_TAIL_MASK	0x7FF

#define PSP_RBHEAD_QHI_HEAD_SHIFT	16
#define PSP_RBHEAD_QHI_HEAD_MASK	0x7FF0000
#define PSP_RBHEAD_QLO_HEAD_MASK	0x7FF

#define PSP_RBHEAD_QPAUSE_INT_STAT	BIT(30)

extern u32 hygon_csv_build;
extern int csv_comm_mode;
extern const struct file_operations csv_fops;

void csv_update_api_version(struct sev_user_data_status *status);
int csv_cmd_buffer_len(int cmd);
void csv_restore_mailbox_mode_postprocess(void);

static inline bool csv_version_greater_or_equal(u32 build)
{
	return hygon_csv_build >= build;
}

static inline bool csv_in_ring_buffer_mode(void)
{
	return csv_comm_mode == CSV_COMM_RINGBUFFER_ON;
}

#endif	/* __CCP_HYGON_CSV_DEV_H__ */

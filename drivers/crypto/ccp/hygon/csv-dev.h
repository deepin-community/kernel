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

extern u32 hygon_csv_build;
extern const struct file_operations csv_fops;

void csv_update_api_version(struct sev_user_data_status *status);
int csv_cmd_buffer_len(int cmd);

#endif	/* __CCP_HYGON_CSV_DEV_H__ */

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_PROCFS_H
#define _NE6X_PROCFS_H

struct ne6x_pf;

void ne6x_proc_pf_init(struct ne6x_pf *pf);
void ne6x_proc_pf_exit(struct ne6x_pf *pf);
void ne6x_proc_init(void);
void ne6x_proc_exit(void);

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_NICTOOL_H
#define HINIC3_NICTOOL_H

#include "hinic3_mt.h"
#include "hinic3_crm.h"

#ifndef MAX_SIZE
#define MAX_SIZE (16)
#endif

#define DBGTOOL_PAGE_ORDER (10)

#define MAX_CARD_NUM (64)

int nictool_k_init(void *hwdev, void *chip_node);
void nictool_k_uninit(void *hwdev, void *chip_node);

void hinic3_get_os_hot_replace_info(void *oshr_info);

void hinic3_get_all_chip_id(void *id_info);

void hinic3_get_card_func_info_by_card_name
	(const char *chip_name, struct hinic3_card_func_info *card_func);

void hinic3_get_card_info(const void *hwdev, void *bufin);

bool hinic3_is_in_host(void);

int hinic3_get_pf_id(struct card_node *chip_node, u32 port_id, u32 *pf_id, u32 *isvalid);

void hinic3_get_mbox_cnt(const void *hwdev, void *bufin);

extern struct hinic3_uld_info g_hinic3_uld_info[SERVICE_T_MAX];

#endif


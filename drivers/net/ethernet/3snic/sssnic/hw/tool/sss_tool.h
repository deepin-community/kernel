/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSSNIC_NICTOOL_H
#define SSSNIC_NICTOOL_H

#include "sss_tool_chip.h"
#include "sss_tool_sdk.h"
#include "sss_tool_sm.h"
#include "sss_tool_comm.h"

#ifndef _LLT_TEST_
#define SSS_TOOL_PAGE_ORDER (10)
#else
#define SSS_TOOL_PAGE_ORDER (1)
#endif

#define SSS_TOOL_MEM_MAP_SIZE (PAGE_SIZE * (1 << SSS_TOOL_PAGE_ORDER))

#define SSS_TOOL_CARD_MAX (64)

int sss_tool_init(void *hwdev, void *chip_node);
void sss_tool_uninit(void *hwdev, void *chip_node);

extern u64 g_card_pa[SSS_TOOL_CARD_MAX];
extern void *g_card_va[SSS_TOOL_CARD_MAX];
extern int g_card_id;

#endif

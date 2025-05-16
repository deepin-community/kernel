// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#include "hinic3_nic_cmdq.h"

void hinic3_nic_cmdq_adapt_init(struct hinic3_nic_io *nic_io)
{
	if (!HINIC3_SUPPORT_FEATURE(nic_io->hwdev, HTN_CMDQ))
		nic_io->cmdq_ops = hinic3_nic_cmdq_get_sw_ops();
	else
		nic_io->cmdq_ops = hinic3_nic_cmdq_get_hw_ops();
}

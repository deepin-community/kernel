// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [NIC]" fmt

#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/errno.h>

#include "ossl_knl.h"
#include "hinic3_nic_dev.h"
#include "hinic3_profile.h"
#include "hinic3_nic_prof.h"

static bool is_match_nic_prof_default_adapter(void *device)
{
	/* always match default profile adapter in standard scene */
	return true;
}

struct hinic3_prof_adapter nic_prof_adap_objs[] = {
	/* Add prof adapter before default profile */
	{
		.type = PROF_ADAP_TYPE_DEFAULT,
		.match = is_match_nic_prof_default_adapter,
		.init = NULL,
		.deinit = NULL,
	},
};

void hinic3_init_nic_prof_adapter(struct hinic3_nic_dev *nic_dev)
{
	int num_adap = ARRAY_LEN(nic_prof_adap_objs);

	nic_dev->prof_adap = hinic3_prof_init(nic_dev, nic_prof_adap_objs, num_adap,
					      (void *)&nic_dev->prof_attr);
	if (nic_dev->prof_adap)
		nic_info(&nic_dev->pdev->dev, "Find profile adapter type: %d\n",
			 nic_dev->prof_adap->type);
}

void hinic3_deinit_nic_prof_adapter(struct hinic3_nic_dev *nic_dev)
{
	hinic3_prof_deinit(nic_dev->prof_adap, nic_dev->prof_attr);
}

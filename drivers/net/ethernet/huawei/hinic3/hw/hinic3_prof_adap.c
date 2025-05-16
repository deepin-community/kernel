// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [COMM]" fmt

#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>

#include "ossl_knl.h"
#include "hinic3_hwdev.h"
#include "hinic3_profile.h"
#include "hinic3_prof_adap.h"

static bool is_match_prof_default_adapter(void *device)
{
	/* always match default profile adapter in standard scene */
	return true;
}

struct hinic3_prof_adapter prof_adap_objs[] = {
	/* Add prof adapter before default profile */
	{
		.type = PROF_ADAP_TYPE_DEFAULT,
		.match = is_match_prof_default_adapter,
		.init = NULL,
		.deinit = NULL,
	},
};

void hisdk3_init_profile_adapter(struct hinic3_hwdev *hwdev)
{
	u16 num_adap = ARRAY_SIZE(prof_adap_objs);

	hwdev->prof_adap = hinic3_prof_init(hwdev, prof_adap_objs, num_adap,
					    (void *)&hwdev->prof_attr);
	if (hwdev->prof_adap)
		sdk_info(hwdev->dev_hdl, "Find profile adapter type: %d\n", hwdev->prof_adap->type);
}

void hisdk3_deinit_profile_adapter(struct hinic3_hwdev *hwdev)
{
	hinic3_prof_deinit(hwdev->prof_adap, hwdev->prof_attr);
}

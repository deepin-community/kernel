// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#define pr_fmt(fmt) KBUILD_MODNAME ": [BASE]" fmt

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/mutex.h>

#include "sss_kernel.h"
#include "sss_hwdev.h"
#include "sss_hw_svc_cap.h"
#include "sss_hwif_irq.h"

static int sss_init_ceq_info(struct sss_hwdev *hwdev)
{
	u8 i;
	struct sss_eq_info *ceq_info = &hwdev->mgmt_info->eq_info;
	struct sss_eq_cfg *ceq = NULL;

	ceq_info->ceq_num = SSS_GET_HWIF_CEQ_NUM(hwdev->hwif);
	ceq_info->remain_ceq_num = ceq_info->ceq_num;
	mutex_init(&ceq_info->eq_mutex);

	sdk_info(hwdev->dev_hdl, "Mgmt ceq info: ceq_num = 0x%x, remain_ceq_num = 0x%x\n",
		 ceq_info->ceq_num, ceq_info->remain_ceq_num);

	if (ceq_info->ceq_num == 0) {
		sdk_err(hwdev->dev_hdl, "Mgmt ceq info: ceq_num = 0\n");
		return -EFAULT;
	}

	ceq = kcalloc(ceq_info->ceq_num, sizeof(*ceq), GFP_KERNEL);
	if (!ceq)
		return -ENOMEM;

	for (i = 0; i < ceq_info->ceq_num; i++) {
		ceq[i].id = i + 1;
		ceq[i].free = SSS_CFG_FREE;
		ceq[i].type = SSS_SERVICE_TYPE_MAX;
	}
	ceq_info->eq = ceq;

	return 0;
}

static void sss_deinit_ceq_info(struct sss_hwdev *hwdev)
{
	struct sss_eq_info *ceq_info = &hwdev->mgmt_info->eq_info;

	kfree(ceq_info->eq);
}

int sss_init_mgmt_info(struct sss_hwdev *hwdev)
{
	int ret;
	struct sss_mgmt_info *mgmt_info;

	mgmt_info = kzalloc(sizeof(*mgmt_info), GFP_KERNEL);
	if (!mgmt_info)
		return -ENOMEM;

	mgmt_info->hwdev = hwdev;
	hwdev->mgmt_info = mgmt_info;

	ret = sss_init_ceq_info(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init ceq info, ret: %d\n", ret);
		goto init_ceq_info_err;
	}

	ret = sss_init_irq_info(hwdev);
	if (ret != 0) {
		sdk_err(hwdev->dev_hdl, "Fail to init irq info, ret: %d\n", ret);
		goto init_irq_info_err;
	}

	return 0;

init_irq_info_err:
	sss_deinit_ceq_info(hwdev);

init_ceq_info_err:
	kfree(mgmt_info);
	hwdev->mgmt_info = NULL;

	return ret;
}

void sss_deinit_mgmt_info(struct sss_hwdev *hwdev)
{
	sss_deinit_irq_info(hwdev);
	sss_deinit_ceq_info(hwdev);

	kfree(hwdev->mgmt_info);
	hwdev->mgmt_info = NULL;
}

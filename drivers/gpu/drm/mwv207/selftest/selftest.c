/*
* SPDX-License-Identifier: GPL
*
* Copyright (c) 2020 ChangSha JingJiaMicro Electronics Co., Ltd.
* All rights reserved.
*
* Author:
*      shanjinkui <shanjinkui@jingjiamicro.com>
*
* The software and information contained herein is proprietary and
* confidential to JingJiaMicro Electronics. This software can only be
* used by JingJiaMicro Electronics Corporation. Any use, reproduction,
* or disclosure without the written permission of JingJiaMicro
* Electronics Corporation is strictly prohibited.
*/
#include "mwv207.h"
#include "mwv207_bo.h"
#include "mwv207_dma.h"

static int mwv207_test_create_and_map_bo(struct mwv207_device *jdev, u64 size,
		u32 domain, u32 flags, struct mwv207_bo **pjbo, void **plogical)
{
	struct mwv207_bo *jbo;
	void *logical;
	int ret;

	ret = mwv207_bo_create(jdev, size, 0, ttm_bo_type_kernel,
			domain, flags, &jbo);
	if (ret)
		return ret;
	ret = mwv207_bo_reserve(jbo, true);
	if (ret)
		goto free_bo;
	ret = mwv207_bo_kmap_reserved(jbo, &logical);
	if (ret)
		goto unreserve_bo;

	*pjbo = jbo;
	*plogical = logical;
	return 0;

unreserve_bo:
	mwv207_bo_unreserve(jbo);
free_bo:
	mwv207_bo_unref(jbo);
	return ret;
}

static void mwv207_test_destroy_and_unmap_bo(struct mwv207_bo *jbo)
{
	if (!jbo)
		return;

	mwv207_bo_kunmap_reserved(jbo);
	mwv207_bo_unreserve(jbo);
	mwv207_bo_unref(jbo);
}

static int mwv207_test_bo_acc(struct mwv207_device *jdev)
{
	struct mwv207_bo *jbo;
	int i, ret;
	void *logical;

	ret = mwv207_test_create_and_map_bo(jdev, 0x1000,
			0x2, (1<<0), &jbo, &logical);
	if (ret)
		return ret;

	memset(logical, 0x5a, 0x1000);
	for (i = 0; i < 0x1000; i++) {
		if (*((char *)logical + i) != 0x5a) {
			ret = -EIO;
			break;
		}
	}

	mwv207_test_destroy_and_unmap_bo(jbo);

	return ret;
}

static int mwv207_test_bo(struct mwv207_device *jdev)
{
	int ret;

	ret = mwv207_test_bo_acc(jdev);
	if (ret) {
		pr_err("mwv207: bo access test failed with %d", ret);
		return ret;
	}

	return ret;
}

int mwv207_test(struct mwv207_device *jdev)
{
	int ret;

	ret = mwv207_test_bo(jdev);
	if (ret == 0)
		pr_info("mwv207 selftest passed");

	return ret;
}

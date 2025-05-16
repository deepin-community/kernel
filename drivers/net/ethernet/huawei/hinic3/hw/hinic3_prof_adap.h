/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_PROF_ADAP_H
#define HINIC3_PROF_ADAP_H

#include <linux/workqueue.h>

#include "hinic3_profile.h"
#include "hinic3_hwdev.h"

enum cpu_affinity_work_type {
	WORK_TYPE_AEQ,
	WORK_TYPE_MBOX,
	WORK_TYPE_MGMT_MSG,
	WORK_TYPE_COMM,
};

enum hisdk3_sw_features {
	HISDK3_SW_F_CHANNEL_LOCK = BIT(0),
};

struct hisdk3_prof_ops {
	void		(*fault_recover)(void *data, u16 src, u16 level);
	int		(*get_work_cpu_affinity)(void *data, u32 work_type);
	void		(*probe_success)(void *data);
	void		(*remove_pre_handle)(struct hinic3_hwdev *hwdev);
};

struct hisdk3_prof_attr {
	void			*priv_data;
	u64			hw_feature_cap;
	u64			sw_feature_cap;
	u64			dft_hw_feature;
	u64			dft_sw_feature;

	struct hisdk3_prof_ops	*ops;
};

#define GET_PROF_ATTR_OPS(hwdev)	\
		((hwdev)->prof_attr ? (hwdev)->prof_attr->ops : NULL)

#ifdef static
#undef static
#define LLT_STATIC_DEF_SAVED
#endif

static inline int hisdk3_get_work_cpu_affinity(struct hinic3_hwdev *hwdev,
					       enum cpu_affinity_work_type type)
{
	struct hisdk3_prof_ops *ops = GET_PROF_ATTR_OPS(hwdev);

	if (ops && ops->get_work_cpu_affinity)
		return ops->get_work_cpu_affinity(hwdev->prof_attr->priv_data, type);

	return WORK_CPU_UNBOUND;
}

static inline void hisdk3_fault_post_process(struct hinic3_hwdev *hwdev,
					     u16 src, u16 level)
{
	struct hisdk3_prof_ops *ops = GET_PROF_ATTR_OPS(hwdev);

	if (ops && ops->fault_recover)
		ops->fault_recover(hwdev->prof_attr->priv_data, src, level);
}

static inline void hisdk3_probe_success(struct hinic3_hwdev *hwdev)
{
	struct hisdk3_prof_ops *ops = GET_PROF_ATTR_OPS(hwdev);

	if (ops && ops->probe_success)
		ops->probe_success(hwdev->prof_attr->priv_data);
}

static inline bool hisdk3_sw_feature_en(const struct hinic3_hwdev *hwdev,
					u64 feature_bit)
{
	if (!hwdev->prof_attr)
		return false;

	return (hwdev->prof_attr->sw_feature_cap & feature_bit) &&
		(hwdev->prof_attr->dft_sw_feature & feature_bit);
}

#ifdef CONFIG_MODULE_PROF
static inline void hisdk3_remove_pre_process(struct hinic3_hwdev *hwdev)
{
	struct hisdk3_prof_ops *ops = NULL;

	if (!hwdev)
		return;

	ops = GET_PROF_ATTR_OPS(hwdev);

	if (ops && ops->remove_pre_handle)
		ops->remove_pre_handle(hwdev);
}
#else
static inline void hisdk3_remove_pre_process(struct hinic3_hwdev *hwdev) {};
#endif
#define SW_FEATURE_EN(hwdev, f_bit)	\
		hisdk3_sw_feature_en(hwdev, HISDK3_SW_F_##f_bit)
#define HISDK3_F_CHANNEL_LOCK_EN(hwdev)	SW_FEATURE_EN(hwdev, CHANNEL_LOCK)

void hisdk3_init_profile_adapter(struct hinic3_hwdev *hwdev);
void hisdk3_deinit_profile_adapter(struct hinic3_hwdev *hwdev);

#endif

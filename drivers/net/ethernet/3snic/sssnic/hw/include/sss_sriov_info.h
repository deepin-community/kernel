/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_SRIOV_INFO_H
#define SSS_SRIOV_INFO_H

#include <linux/types.h>

enum sss_sriov_state {
	SSS_SRIOV_DISABLE,
	SSS_SRIOV_ENABLE,
	SSS_SRIOV_PRESENT,
};

struct sss_sriov_info {
	u8 enabled;
	u8 rsvd[3];
	unsigned int vf_num;
	unsigned long state;
};

#endif

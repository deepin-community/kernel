/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef CFG_MGMT_MPU_CMD_H
#define CFG_MGMT_MPU_CMD_H

enum cfg_cmd {
	CFG_CMD_GET_DEV_CAP = 0, /**< Device capability of pf/vf, @see cfg_cmd_dev_cap */
	CFG_CMD_GET_HOST_TIMER = 1, /**< Capability of host timer, @see cfg_cmd_host_timer */
};

#endif

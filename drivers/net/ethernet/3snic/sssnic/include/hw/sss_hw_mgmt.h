/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_HW_MGMT_H
#define SSS_HW_MGMT_H

enum sss_hwdev_init_state {
	SSS_HW_NOT_INIT_OK = 0,
	SSS_HW_ADM_INIT_OK,
	SSS_HW_MBX_INIT_OK,
	SSS_HW_CTRLQ_INIT_OK,
};

typedef void (*sss_mgmt_msg_handler_t)(void *data, u16 cmd, void *in_buf,
				       u16 in_size, void *out_buf, u16 *out_size);

int sss_register_mgmt_msg_handler(void *hwdev, u8 mod_type, void *data,
				  sss_mgmt_msg_handler_t handler);

void sss_unregister_mgmt_msg_handler(void *hwdev, u8 mod_type);

#endif

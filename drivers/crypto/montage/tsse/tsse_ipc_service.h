/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_IPC_SERVICE_H__
#define __TSSE_IPC_SERVICE_H__

#include <linux/completion.h>
#include <linux/types.h>
#include "tsse_ipc_msg.h"
#include "tsse_dev.h"

#define TSSE_SERVICES_QUERY_CMD 0
#define TSSE_MANAGE_SERVICE_NAME "manage service"
#define TSSE_SERVICE_MAX_WAIT_MILLISECONDS 5000

enum tsse_service_cmd {
	TSSE_SERVICE_CMD_OPEN = 50,
	TSSE_SERVICE_CMD_CLOSE,
	TSSE_SERVICE_CMD_APP_MSG,
	TSSE_SERVICE_CMD_DELIMITER
};

#pragma pack(push, 4)
struct tsse_service_instance {
	u8 service_opened;
	u8 service_name[TSSE_IM_SERVICE_NAME_LEN];
	int device_handle;
	tsse_im_cb_func cb;
	u64 service_epid;
};

struct tsse_service_user_data {
	struct completion req_completion;
	int status;
};

struct tsse_service_open_req {
	struct tsse_im_msg_hdr hdr;
	uint8_t service_name[TSSE_IM_SERVICE_NAME_LEN];
};

struct tsse_service_close_req {
	struct tsse_im_msg_hdr hdr;
};

struct tsse_services_query_req {
	struct tsse_im_msg_hdr hdr;
};

struct tsse_services_query_resp {
	u16 type;
	u16 len;
	u8 data[];
};

struct tsse_service_comm_resp {
	struct tsse_im_msg_hdr hdr;
	int ret_code;
	u8 data[];
};

struct tsse_service_info {
	char name[TSSE_IM_SERVICE_NAME_LEN];
	u32 num_devices;
	u8 device_handles[TSSE_IM_DEVICE_NUM_MAX];
	struct list_head list;
};

struct tsse_ipc_ring_setup_req {
	u64 cookie;
	u32 is_create;
	u32 reserved[13];
};

struct tsse_ipc_ring_setup_resp {
	uint64_t cookie;
	int32_t ret;
};
#pragma pack(pop)

int tsse_service_msg_send(
	tsse_im_service_handle handle,
	u32 service_cmd,
	void *msg_payload,
	u32 payload_length);

int tsse_service_msg_receive(u64 epid, void *msg, u32 msg_len);

int tsse_service_open(tsse_im_service_handle handle);
int tsse_service_close(tsse_im_service_handle handle);
int tsse_services_query_request(tsse_im_service_handle handle);
int tsse_services_query_response(tsse_im_service_handle handle, void *payload, u32 length);
int tsse_schedule_device_handle(tsse_im_service_handle handle);
int tsse_ipc_setup_ring(int device_handle, u32 is_create);
int ipc_ring_setup_resp_receive(void *msg, u32 length);

int tsse_im_shutdown_for_dev(struct tsse_dev *tdev);
int tsse_im_startup_for_dev(struct tsse_dev *tdev);

typedef int (*post_process_func)(tsse_im_service_handle handle);
#endif

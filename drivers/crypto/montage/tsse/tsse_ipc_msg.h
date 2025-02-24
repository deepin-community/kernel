/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_IPC_MSG_H__
#define __TSSE_IPC_MSG_H__

#include <linux/types.h>

#define TSSE_IM_DEVICE_NUM_MAX 256
#define TSSE_IM_SERVICE_NAME_LEN 16

enum tsse_im_msg_type {
	IM_MSG_TYPE_REQ = 0,
	IM_MSG_TYPE_RSP,
	IM_MSG_TYPE_NOTIFY
};

#pragma pack(push, 4)
struct tsse_im_msg_hdr {
	/** @ref enum tsse_im_msg_type */
	uint16_t msg_type;
	/** internal command id of the service */
	uint16_t cmd;
	uint64_t cookie;
};

struct tsse_im_service_info {
	char service_name[TSSE_IM_SERVICE_NAME_LEN];
	uint32_t num_devices;
	uint8_t device_ids[TSSE_IM_DEVICE_NUM_MAX];
};
#pragma pack(pop)

#define tsse_im_service_handle void *

/**
 * tsse_im_cb_func - callback to process device-to-host IPC message,
 * also called response handler. Service layer should register it
 * when alloc service handle by tsse_im_service_handle_alloc.
 * @handle: handle to TSSE service
 * @msg_payload: actual data related to specific message class
 * @payload_length: length of msg_payload
 * Return: 0 on success, error code otherwise
 */
typedef int (*tsse_im_cb_func)(tsse_im_service_handle handle,
	void *msg_payload, u32 payload_length);

int tsse_im_startup(void);

int tsse_im_service_exist(const char *name);

int tsse_im_service_handle_alloc(const char *name,
	tsse_im_cb_func cb, tsse_im_service_handle *handle);

int tsse_im_service_handle_free(tsse_im_service_handle handle);

int tsse_im_service_msg_h2d(tsse_im_service_handle handle, void *msg_payload, u32 payload_length);

#endif

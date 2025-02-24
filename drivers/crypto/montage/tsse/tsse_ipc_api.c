// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */
#include <linux/errno.h>
#include <linux/string.h>
#include "tsse_ipc_msg.h"
#include "tsse_dev.h"
#include "tsse_ipc_hash.h"
#include "tsse_ipc_service.h"

/**
 * tsse_im_startup() - Startup TSSE IPC Message, will skip the device if it is already started.
 * Return: 0 if startup successfully, other values for failure.
 */
int tsse_im_startup(void)
{
	return tsse_process_for_all(tsse_im_startup_for_dev);
}
EXPORT_SYMBOL_GPL(tsse_im_startup);

/**
 * tsse_im_service_exist() - Check if the specific IPC Message service exists.
 * @name: IPC Message service name
 * Return: 0 if the service exists, otherwise -EINVAL.
 */
int tsse_im_service_exist(const char *name)
{
	struct service_info_entry *entry;

	entry = tsse_service_info_hash_get(name);
	if (!entry) {
		pr_err("%s(): service: %s not exist\n", __func__, name);
		return -EINVAL;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(tsse_im_service_exist);

/**
 * tsse_im_service_handle_alloc() - Allocate IPC Message service handle for specific service.
 * @name: IPC Message service name
 * @cb: request callback for the service
 * @handle: function output for the service handle
 * Return: 0 if allocated successfully, other values for failure
 */
int tsse_im_service_handle_alloc(
	const char *name,
	tsse_im_cb_func cb,
	tsse_im_service_handle *handle)
{
	struct tsse_service_instance *service_instance;
	int ret;

	service_instance = kzalloc(sizeof(struct tsse_service_instance), GFP_ATOMIC);
	if (!service_instance)
		return -ENOMEM;
	service_instance->service_opened = 0;
	service_instance->cb = cb;
	strscpy(service_instance->service_name, name, TSSE_IM_SERVICE_NAME_LEN);

	ret = tsse_schedule_device_handle(service_instance);
	if (ret) {
		kfree(service_instance);
		return ret;
	}

	ret = tsse_service_open(service_instance);
	if (ret) {
		pr_err("%s(): open service: %s failed: %d\n",
			__func__, service_instance->service_name, ret);
		kfree(service_instance);
		return ret;
	}
	*handle = service_instance;
	return 0;
}
EXPORT_SYMBOL_GPL(tsse_im_service_handle_alloc);

/**
 * tsse_im_service_handle_free() - Free IPC Message service handle
 * @handle: service handle to free
 * Return: 0 if free successfully, other values for failure
 */
int tsse_im_service_handle_free(tsse_im_service_handle handle)
{
	int ret = 0;

	if (handle) {
		ret = tsse_service_close(handle);
		kfree((void *)handle);
	}
	return ret;
}
EXPORT_SYMBOL_GPL(tsse_im_service_handle_free);

/**
 * tsse_im_service_msg_h2d() - Send message from host to device
 * @handle: service handle
 * @msg_payload: the message payload to send
 * @payload_length: length of msg_payload
 */
int tsse_im_service_msg_h2d(tsse_im_service_handle handle, void *msg_payload, u32 payload_length)
{
	if (!handle || !msg_payload || !payload_length)
		return -EINVAL;
	return tsse_service_msg_send(handle, TSSE_SERVICE_CMD_APP_MSG, msg_payload, payload_length);
}
EXPORT_SYMBOL_GPL(tsse_im_service_msg_h2d);

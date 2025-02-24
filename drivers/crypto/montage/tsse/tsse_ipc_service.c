// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */
#include <linux/completion.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "tsse_ipc_service.h"
#include "tsse_ipc_drv.h"
#include "tsse_ipc_epid.h"
#include "tsse_ipc_hash.h"
#include "tsse_ipc_msg.h"
#include "tsse_dev.h"

static DEFINE_MUTEX(tsse_device_table_lock);
static int tsse_device_last_used_index = -1;

static int service_request_post_process(
	int ret, void *req,
	struct tsse_service_user_data *user_data,
	tsse_im_service_handle handle,
	post_process_func func)
{
	if (ret)
		goto cleanup;
	if (!wait_for_completion_timeout(&user_data->req_completion,
		msecs_to_jiffies(TSSE_SERVICE_MAX_WAIT_MILLISECONDS))) {
		pr_err("%s(): completion timeout\n", __func__);
		ret = -EFAULT;
		goto cleanup;
	}
	ret = user_data->status;
	if ((ret == 0) && func)
		func(handle);
cleanup:
	kfree(user_data);
	kfree(req);
	return ret;
}

int tsse_service_msg_send(
	tsse_im_service_handle handle,
	u32 service_cmd,
	void *msg_payload,
	u32 payload_length)
{
	struct tsse_service_instance *service_instance;
	u64 epid;

	service_instance = (struct tsse_service_instance *)handle;
	if ((service_cmd == TSSE_SERVICE_CMD_APP_MSG) && !service_instance->service_opened) {
		pr_err("%s(): service handle is not ready to use\n", __func__);
		return -EPERM;
	}
	epid = APPEND_APP_ID_TO_EPID(service_instance->service_epid, service_cmd);
	return ipc_h2d_msg_send(service_instance->device_handle, epid, msg_payload, payload_length);
}

static int tsse_service_open_post_process(tsse_im_service_handle handle)
{
	struct tsse_service_instance *service_instance;
	int ret;

	service_instance = (struct tsse_service_instance *)handle;
	ret = tsse_service_handle_hash_set(service_instance->service_epid, handle);
	if (ret) {
		pr_err("%s() failed to save service handle: %d\n", __func__, ret);
		return ret;
	}
	service_instance->service_opened = 1;
	return 0;
}

int tsse_service_open(tsse_im_service_handle handle)
{
	struct tsse_service_open_req *req;
	struct tsse_service_user_data *user_data;
	struct tsse_service_instance *service_instance;
	int ret;

	ret = tsse_alloc_service_epid(handle);
	if (ret)
		return ret;

	req = kzalloc(sizeof(struct tsse_service_open_req), GFP_ATOMIC);
	if (!req)
		return -ENOMEM;

	user_data = kzalloc(sizeof(struct tsse_service_user_data), GFP_ATOMIC);
	if (!user_data) {
		kfree(req);
		return -ENOMEM;
	}

	init_completion(&user_data->req_completion);
	service_instance = (struct tsse_service_instance *)handle;
	req->hdr.msg_type = IM_MSG_TYPE_REQ;
	req->hdr.cookie = (u64)user_data;
	memcpy(req->service_name, service_instance->service_name, TSSE_IM_SERVICE_NAME_LEN);
	ret = tsse_service_msg_send(handle, TSSE_SERVICE_CMD_OPEN,
		req, sizeof(struct tsse_service_open_req));
	return service_request_post_process(ret, req, user_data,
		handle, tsse_service_open_post_process);
}

static int tsse_service_close_post_process(tsse_im_service_handle handle)
{
	struct tsse_service_instance *service_instance;

	service_instance = (struct tsse_service_instance *)handle;
	service_instance->service_opened = 0;
	tsse_service_handle_hash_remove(service_instance->service_epid);
	tsse_free_service_epid(handle);
	return 0;
}

int tsse_service_close(tsse_im_service_handle handle)
{
	struct tsse_service_close_req *req;
	struct tsse_service_user_data *user_data;
	int ret;

	req = kzalloc(sizeof(struct tsse_service_close_req), GFP_ATOMIC);
	if (!req)
		return -ENOMEM;

	user_data = kzalloc(sizeof(struct tsse_service_user_data), GFP_ATOMIC);
	if (!user_data) {
		kfree(req);
		return -ENOMEM;
	}

	init_completion(&user_data->req_completion);
	req->hdr.msg_type = IM_MSG_TYPE_REQ;
	req->hdr.cookie = (u64)user_data;
	ret = tsse_service_msg_send(handle, TSSE_SERVICE_CMD_CLOSE,
		req, sizeof(struct tsse_service_close_req));
	return service_request_post_process(ret, req, user_data,
		handle, tsse_service_close_post_process);
}

int  tsse_services_query_request(tsse_im_service_handle handle)
{
	struct tsse_services_query_req *req;
	struct tsse_service_user_data *user_data;
	int ret;

	req = kzalloc(sizeof(struct tsse_services_query_req), GFP_ATOMIC);
	if (!req)
		return -ENOMEM;

	user_data = kzalloc(sizeof(struct tsse_service_user_data), GFP_ATOMIC);
	if (!user_data) {
		kfree(req);
		return -ENOMEM;
	}

	init_completion(&user_data->req_completion);
	req->hdr.msg_type = IM_MSG_TYPE_REQ;
	req->hdr.cmd = TSSE_SERVICES_QUERY_CMD;
	req->hdr.cookie = (u64)user_data;
	ret = tsse_service_msg_send(handle, TSSE_SERVICE_CMD_APP_MSG,
		req, sizeof(struct tsse_services_query_req));
	return service_request_post_process(ret, req, user_data, handle, NULL);
}

int tsse_services_query_response(tsse_im_service_handle handle, void *payload, u32 length)
{
	struct tsse_service_instance *instance;
	struct tsse_service_comm_resp *resp;
	struct tsse_service_user_data *user_data;
	struct tsse_services_query_resp *resp_payload;
	struct tsse_service_info *service_info;
	struct service_info_entry *entry;
	char service_name[TSSE_IM_SERVICE_NAME_LEN] = {0};
	u32 buffer_len;
	u32 data_offset;
	u32 index;
	u32 device_exists = 0;

	instance = (struct tsse_service_instance *) handle;
	if (length < sizeof(struct tsse_service_comm_resp)) {
		pr_err("%s() invalid length: %u\n", __func__, length);
		return -EFAULT;
	}
	resp = (struct tsse_service_comm_resp *)payload;
	user_data = (struct tsse_service_user_data *)resp->hdr.cookie;
	if (resp->hdr.msg_type != IM_MSG_TYPE_RSP) {
		pr_err("%s() invalid msg_type: %u\n", __func__, resp->hdr.msg_type);
		return -EFAULT;
	}
	if (!user_data) {
		pr_err("%s() empty cookie in resp header\n", __func__);
		return -EFAULT;
	}
	length -= sizeof(struct tsse_service_comm_resp);
	data_offset = 0;
	while (data_offset < length) {
		resp_payload = (struct tsse_services_query_resp *)(
			resp->data + data_offset);
		buffer_len = resp_payload->len + 1;
		if (buffer_len > TSSE_IM_SERVICE_NAME_LEN)
			buffer_len = TSSE_IM_SERVICE_NAME_LEN;
		strscpy(service_name, resp_payload->data, buffer_len);
		entry = tsse_service_info_hash_get(service_name);
		if (entry) {
			service_info = (struct tsse_service_info *)entry->service_info;
			for (index = 0; index < service_info->num_devices; index++)
				device_exists |= (service_info->device_handles[index]
					== instance->device_handle);
			if (!device_exists) {
				service_info->device_handles[service_info->num_devices]
					= instance->device_handle;
				service_info->num_devices++;
			}
		} else {
			service_info = kzalloc(sizeof(struct tsse_service_info), GFP_ATOMIC);
			if (!service_info)
				return -ENOMEM;
			memcpy(service_info->name, service_name, TSSE_IM_SERVICE_NAME_LEN);
			service_info->num_devices = 1;
			service_info->device_handles[0] = instance->device_handle;
			tsse_service_info_hash_set(service_name, service_info);
		}
		data_offset += (sizeof(struct tsse_services_query_resp)
			+ resp_payload->len);
	}
	user_data->status = resp->ret_code;
	complete(&user_data->req_completion);
	return 0;
}

static int tsse_service_open_close_resp(void *msg, u32 msg_len)
{
	struct tsse_service_comm_resp *resp;
	struct tsse_service_user_data *user_data;

	if (msg_len < sizeof(struct tsse_service_comm_resp)) {
		pr_err("%s() invalid msg_len: %u\n", __func__, msg_len);
		return -EFAULT;
	}
	resp = (struct tsse_service_comm_resp *)msg;
	user_data = (struct tsse_service_user_data *)resp->hdr.cookie;
	if (resp->hdr.msg_type != IM_MSG_TYPE_RSP) {
		pr_err("%s() invalid msg_type: %u\n", __func__, resp->hdr.msg_type);
		return -EFAULT;
	}
	if (!user_data) {
		pr_err("%s() empty cookie in resp header\n", __func__);
		return -EFAULT;
	}
	user_data->status = resp->ret_code;
	complete(&user_data->req_completion);
	return 0;
}

static int tsse_service_app_resp(u64 epid, void *msg, u32 msg_len)
{
	struct service_handle_entry *entry;
	struct tsse_service_instance *instance;

	entry = tsse_service_handle_hash_get(SERVICE_LEVEL_EPID(epid));
	if (!entry || !entry->handle) {
		pr_err("%s() cannot find service handle for epid: 0x%llx\n", __func__, epid);
		return -EFAULT;
	}
	instance = (struct tsse_service_instance *)entry->handle;
	return instance->cb(instance, msg, msg_len);
}

int tsse_service_msg_receive(u64 epid, void *msg, u32 msg_len)
{
	u32 service_cmd;

	if (!msg || !msg_len) {
		pr_err("%s() service resp msg should not be empty\n", __func__);
		return -EFAULT;
	}
	service_cmd = GET_APP_SPECIFIC_ID(epid);
	switch (service_cmd) {
	case TSSE_SERVICE_CMD_OPEN:
	case TSSE_SERVICE_CMD_CLOSE:
		return tsse_service_open_close_resp(msg, msg_len);
	case TSSE_SERVICE_CMD_APP_MSG:
		return tsse_service_app_resp(epid, msg, msg_len);
	default:
		return -EFAULT;
	}
	return 0;
}

int tsse_schedule_device_handle(tsse_im_service_handle handle)
{
	struct tsse_service_info *service_info;
	struct tsse_service_instance *service_instance;
	struct service_info_entry *entry;
	u32 device_handle_index;

	service_instance = (struct tsse_service_instance *)handle;
	entry = tsse_service_info_hash_get(service_instance->service_name);
	if (!entry || !entry->service_info) {
		pr_err("%s(): service %s not exist\n", __func__, service_instance->service_name);
		return -EFAULT;
	}
	service_info = (struct tsse_service_info *)entry->service_info;
	if (service_info->num_devices == 0) {
		pr_err("%s(): no available device for service: %s\n",
			__func__, service_instance->service_name);
		return -EFAULT;
	}
	mutex_lock(&tsse_device_table_lock);
	if (tsse_device_last_used_index < 0)
		device_handle_index = 0;
	else
		device_handle_index = (tsse_device_last_used_index + 1) % service_info->num_devices;
	tsse_device_last_used_index = device_handle_index;
	mutex_unlock(&tsse_device_table_lock);
	service_instance->device_handle = service_info->device_handles[device_handle_index];
	return 0;
}

static u64 get_init_ring_epid(int device_handle)
{
	struct tsse_epid epid_data = {0};

	epid_data.service_id = EPID_MANAGE_SERVICE_ID;
	epid_data.pasid_en = 0;
	epid_data.vf_id = 0;
	epid_data.is_pf = 1;
	epid_data.device_id = device_handle;
	epid_data.app_id = TSSE_IPC_SPECIFIC_RING_SETUP_REQ;
	return EPID_TO_UINT64(&epid_data);
}

int tsse_ipc_setup_ring(int device_handle, u32 is_create)
{
	int ret;
	u64 epid;
	struct tsse_ipc_ring_setup_req *setup_req;
	struct tsse_service_user_data *user_data;

	setup_req = kzalloc(sizeof(struct tsse_ipc_ring_setup_req), GFP_ATOMIC);
	user_data = kzalloc(sizeof(struct tsse_service_user_data), GFP_ATOMIC);
	if (!setup_req || !user_data)
		return -ENOMEM;

	setup_req->cookie = (u64) user_data;
	setup_req->is_create = is_create > 0 ? 1 : 0;
	epid = get_init_ring_epid(device_handle);
	if (is_create)
		init_completion(&user_data->req_completion);

	ret = ipc_h2d_msg_send(device_handle, epid, setup_req,
		sizeof(struct tsse_ipc_ring_setup_req));
	if (ret)
		goto cleanup;
	if (is_create) {
		if (!wait_for_completion_timeout(&user_data->req_completion,
				msecs_to_jiffies(TSSE_SERVICE_MAX_WAIT_MILLISECONDS))) {
			pr_err("%s(): completion timeout\n", __func__);
			ret = -EFAULT;
			goto cleanup;
		}
		ret = user_data->status;
	}
cleanup:
	kfree(user_data);
	kfree(setup_req);
	return ret;
}

int ipc_ring_setup_resp_receive(void *msg, u32 length)
{
	struct tsse_ipc_ring_setup_resp *resp;
	struct tsse_service_user_data *user_data;

	if (length < sizeof(struct tsse_ipc_ring_setup_resp)) {
		pr_err("%s %d: invalid resp len: %u\n", __func__, __LINE__, length);
		return -EINVAL;
	}
	resp = (struct tsse_ipc_ring_setup_resp *)msg;
	user_data = (struct tsse_service_user_data *)resp->cookie;
	user_data->status = resp->ret;
	complete(&user_data->req_completion);
	return 0;
}

static int tsse_im_services_init(struct pci_dev *pdev)
{
	struct tsse_dev *tdev = pci_to_tsse_dev(pdev);
	struct tsse_service_instance *service_instance;
	int ret;

	service_instance = kzalloc(sizeof(struct tsse_service_instance), GFP_ATOMIC);
	if (!service_instance)
		return -ENOMEM;
	service_instance->service_opened = 0;
	service_instance->device_handle = tdev->id;
	service_instance->cb = tsse_services_query_response;
	strscpy(service_instance->service_name, TSSE_MANAGE_SERVICE_NAME, TSSE_IM_SERVICE_NAME_LEN);

	ret = tsse_service_open(service_instance);
	if (ret) {
		pr_err("%s(): open service: %s failed: %d\n",
			__func__, service_instance->service_name, ret);
		goto cleanup;
	}
	ret = tsse_services_query_request(service_instance);
	if (ret) {
		pr_err("%s(): services query failed: %d\n", __func__, ret);
		goto cleanup;
	}
	ret = tsse_service_close(service_instance);
	if (ret) {
		pr_err("%s(): close service: %s failed: %d\n",
			__func__, service_instance->service_name, ret);
		goto cleanup;
	}
cleanup:
	kfree(service_instance);
	return ret;
}

int tsse_im_startup_for_dev(struct tsse_dev *tdev)
{
	int ret;

	if (!tdev || !tdev->ipc) {
		pr_err("failed to startup im, the device is not ready\n");
		return -EPERM;
	}
	if (tdev->ipc->im_inited)
		return 0;
	ret = tsse_ipc_setup_ring(tdev->id, 1);
	if (ret == 0)
		ret = tsse_im_services_init(tdev->tsse_pci_dev.pci_dev);
	if (ret == 0) {
		tdev->ipc->im_inited = 1;
		return ret;
	}
	tsse_im_shutdown_for_dev(tdev);
	return ret;
}

int tsse_im_shutdown_for_dev(struct tsse_dev *tdev)
{
	struct tsse_ipc *tsseipc;
	int ret = 0;

	if (!tdev)
		return 0;

	tsseipc = tdev->ipc;
	if (tsseipc && tsseipc->im_inited) {
		ret = tsse_ipc_setup_ring(tdev->id, 0);
		if (ret == 0)
			tsseipc->im_inited = 0;
	}
	return ret;
}

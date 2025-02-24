/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */

#ifndef __TSSE_IPC_HASH_H__
#define __TSSE_IPC_HASH_H__

#define SERVICE_TABLE_BUCKET_BITS 8

struct service_info_entry {
	char *service_name;
	void *service_info;
	struct hlist_node node;
};

struct service_handle_entry {
	u64 epid;
	void *handle;
	struct hlist_node node;
};

int tsse_service_info_hash_set(const char *service, void *service_info);
struct service_info_entry *tsse_service_info_hash_get(const char *service);
void tsse_service_info_hash_remove_all(void);
int tsse_service_handle_hash_set(u64 epid, void *handle);
struct service_handle_entry *tsse_service_handle_hash_get(u64 epid);
void tsse_service_handle_hash_remove(u64 epid);
void tsse_service_handle_hash_remove_all(void);

#endif

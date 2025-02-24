// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * This file is part of tsse driver for Linux
 *
 * Copyright © 2023-2024 Montage Technology. All rights reserved.
 */
#include <linux/string.h>
#include <linux/types.h>
#include <linux/hashtable.h>
#include <linux/slab.h>
#include <linux/stringhash.h>
#include "tsse_ipc_hash.h"

DEFINE_HASHTABLE(service_info_table, SERVICE_TABLE_BUCKET_BITS);
DEFINE_HASHTABLE(service_handle_table, SERVICE_TABLE_BUCKET_BITS);

static u32 hash_string(const char *str)
{
	return full_name_hash(NULL, str, strlen(str));
}

int tsse_service_info_hash_set(const char *service, void *service_info)
{
	struct service_info_entry *new_entry;

	new_entry = kzalloc(sizeof(struct service_info_entry), GFP_KERNEL);
	if (!new_entry)
		return -ENOMEM;
	new_entry->service_name = kstrdup(service, GFP_KERNEL);
	new_entry->service_info = service_info;
	hash_add(service_info_table, &new_entry->node,
		hash_min(hash_string(service), SERVICE_TABLE_BUCKET_BITS));
	return 0;
}

struct service_info_entry *tsse_service_info_hash_get(const char *service)
{
	struct service_info_entry *entry;

	hash_for_each_possible(service_info_table, entry, node,
		hash_min(hash_string(service), SERVICE_TABLE_BUCKET_BITS)) {
		if (strcmp(entry->service_name, service) == 0)
			return entry;
	}
	return NULL;
}

void tsse_service_info_hash_remove_all(void)
{
	int bucket;
	struct service_info_entry *entry;
	struct hlist_node *tmp;

	hash_for_each_safe(service_info_table, bucket, tmp, entry, node) {
		kfree(entry->service_name);
		kfree(entry->service_info);
		hash_del(&entry->node);
		kfree(entry);
	}
}

int tsse_service_handle_hash_set(u64 epid, void *handle)
{
	struct service_handle_entry *new_entry;

	new_entry = kzalloc(sizeof(struct service_handle_entry), GFP_KERNEL);
	if (!new_entry)
		return -ENOMEM;
	new_entry->epid = epid;
	new_entry->handle = handle;
	hash_add(service_handle_table, &new_entry->node, hash_min(epid, SERVICE_TABLE_BUCKET_BITS));
	return 0;
}

struct service_handle_entry *tsse_service_handle_hash_get(u64 epid)
{
	struct service_handle_entry *entry;

	hash_for_each_possible(service_handle_table, entry, node,
		hash_min(epid, SERVICE_TABLE_BUCKET_BITS)) {
		if (entry->epid == epid)
			return entry;
	}
	return NULL;
}

void tsse_service_handle_hash_remove(u64 epid)
{
	struct service_handle_entry *entry = tsse_service_handle_hash_get(epid);

	if (entry) {
		hash_del(&entry->node);
		kfree(entry);
	}
}

void tsse_service_handle_hash_remove_all(void)
{
	int bucket;
	struct service_handle_entry *entry;
	struct hlist_node *tmp;

	hash_for_each_safe(service_handle_table, bucket, tmp, entry, node) {
		hash_del(&entry->node);
		kfree(entry);
	}
}

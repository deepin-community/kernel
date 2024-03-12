/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (c) 2016-2017 Hisilicon Limited. */

#ifndef __HCLGE_EXT_H
#define __HCLGE_EXT_H
#include <linux/types.h>

struct hclge_reset_fail_type_map {
	enum hnae3_reset_type reset_type;
	enum hnae3_event_type_custom custom_type;
};

typedef int (*hclge_priv_ops_fn)(struct hclge_dev *hdev, void *data,
				 size_t length);

/**
 * nic_event_fn_t - nic event handler prototype
 * @netdev:	net device
 * @hnae3_event_type_custom:	nic device event type
 */
typedef void (*nic_event_fn_t) (struct net_device *netdev,
				enum hnae3_event_type_custom);

/**
 * nic_register_event - register for nic event handling
 * @event_call:	nic event handler
 * return 0 - success , negative - fail
 */
int nic_register_event(nic_event_fn_t event_call);

/**
 * nic_unregister_event - unregister for nic event handling
 * return 0 - success , negative - fail
 */
int nic_unregister_event(void);

int hclge_ext_call_event(struct hclge_dev *hdev,
			 enum hnae3_event_type_custom event_t);
void hclge_ext_reset_end(struct hclge_dev *hdev, bool done);

int hclge_ext_ops_handle(struct hnae3_handle *handle, int opcode,
			 void *data, size_t length);
#endif

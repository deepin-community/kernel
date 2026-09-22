/* SPDX-License-Identifier: GPL-2.0 or Linux-OpenIB */
/* Copyright (c) 2023 - 2024 ZTE Corporation */

#ifndef ZXDH_WS_H
#define ZXDH_WS_H

#include "osdep.h"

struct zxdh_ws_node {
	struct list_head siblings;
	struct list_head child_list_head;
	struct zxdh_ws_node *parent;
	u64 lan_qs_handle; /* opaque handle used by LAN */
	u32 l2_sched_node_id;
	u16 index;
	u16 qs_handle;
	u16 vsi_index;
	u8 traffic_class;
	u8 user_pri;
	u8 rel_bw;
	u8 abstraction_layer; /* used for splitting a TC */
	u8 prio_type;
	u8 type_leaf : 1;
	u8 enable : 1;
};

struct zxdh_sc_vsi;

#endif /* ZXDH_WS_H */

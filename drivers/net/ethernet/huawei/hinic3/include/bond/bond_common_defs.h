/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved. */

#ifndef BOND_COMMON_DEFS_H
#define BOND_COMMON_DEFS_H

#define BOND_NAME_MAX_LEN	16
#define BOND_PORT_MAX_NUM	4
#define BOND_ID_INVALID		0xFFFF
#define OVS_PORT_NUM_MAX	BOND_PORT_MAX_NUM
#define DEFAULT_ROCE_BOND_FUNC	0xFFFFFFFF

#define BOND_ID_IS_VALID(_id)   \
			(((_id) >= BOND_FIRST_ID) && ((_id) <= BOND_MAX_ID))
#define BOND_ID_IS_INVALID(_id) (!(BOND_ID_IS_VALID(_id)))

enum bond_group_id {
	BOND_FIRST_ID = 1,
	BOND_MAX_ID = 4,
	BOND_MAX_NUM,
};

#pragma pack(push, 4)
/**
 * bond per port statistics
 */
struct tag_bond_port_stat {
	/** mpu provide */
	u64 rx_pkts;
	u64 rx_bytes;
	u64 rx_drops;
	u64 rx_errors;

	u64 tx_pkts;
	u64 tx_bytes;
	u64 tx_drops;
	u64 tx_errors;
};

#pragma pack(pop)

/**
 * bond port attribute
 */
struct tag_bond_port_attr {
	u8 duplex;
	u8 status;
	u8 rsvd0[2];
	u32 speed;
};

/**
 * Get bond information command struct defination
 * @see OVS_MPU_CMD_BOND_GET_ATTR
 */
struct tag_bond_get {
	u16 bond_id_vld;		/* 1: used bond_id get bond info, 0: used bond_name */
	u16 bond_id;			/* if bond_id_vld=1 input, else output */
	u8 bond_name[BOND_NAME_MAX_LEN];	/* if bond_id_vld=0 input, else output */

	u16 bond_mode;		/* 1 for active-backup,2 for balance-xor,4 for 802.3ad */
	u8 active_slaves;	/* active port slaves(bitmaps) */
	u8 slaves;		/* bond port id bitmaps */

	u8 lacp_collect_slaves; /* bond port id bitmaps */
	u8 xmit_hash_policy;	/* xmit hash:0 for layer 2, 1 for layer 2+3, 2 for layer 3+4 */
	u16 rsvd0;		/* in order to 4B aligned */

	struct tag_bond_port_stat stat[BOND_PORT_MAX_NUM];
	struct tag_bond_port_attr attr[BOND_PORT_MAX_NUM];
};

#endif /** BOND_COMMON_DEFS_H */

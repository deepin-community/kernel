/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_NETLINK_H
#define _NE6X_NETLINK_H

#define NE6X_NETLINK          31
#define NE6X_HASH_KEY_SIZE    64
#define NE6X_HASH_DATA_SIZE   64
#define NE6X_RULE_BATCH_MAX   64
#define NE6X_METER_TYPE_MAX   8
#define NE6X_METER_OPCODE_MAX 1
#define NE6X_ADDR_LEN         16

/* netlink message opcodes */
enum {
	NE6X_NLMSG_BASE = 0x10, /* the type < 0x10 is reserved for control messages */
	NE6X_NLMSG_TAB_ADD = NE6X_NLMSG_BASE,
	NE6X_NLMSG_TAB_DEL,
	NE6X_NLMSG_METER_WRITE,
	NE6X_NLMSG_MAX
};

struct ne6x_rule {
	u8 dst[NE6X_ADDR_LEN];
	u8 src[NE6X_ADDR_LEN];
	u32 proto;
} __packed;

struct ne6x_meter {
	u8 type_num;
	u8 opcode;
	u32 value;
} __packed;

void ne6x_netlink_init(void);
void ne6x_netlink_exit(void);

#endif

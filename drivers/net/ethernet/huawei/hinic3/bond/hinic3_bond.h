/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_BOND_H
#define HINIC3_BOND_H

#include <linux/netdevice.h>
#include <linux/types.h>
#include "mpu_inband_cmd_defs.h"
#include "bond_common_defs.h"

enum hinic3_bond_user {
	HINIC3_BOND_USER_OVS,
	HINIC3_BOND_USER_TOE,
	HINIC3_BOND_USER_ROCE,
	HINIC3_BOND_USER_NUM
};

enum bond_service_proc_pos {
	BOND_BEFORE_ACTIVE,
	BOND_AFTER_ACTIVE,
	BOND_BEFORE_MODIFY,
	BOND_AFTER_MODIFY,
	BOND_BEFORE_DEACTIVE,
	BOND_AFTER_DEACTIVE,
	BOND_POS_MAX
};

#define BITMAP_SET(bm, bit)     ((bm) |= (typeof(bm))(1U << (bit)))
#define BITMAP_CLR(bm, bit)     ((bm) &= ~((typeof(bm))(1U << (bit))))
#define BITMAP_JUDGE(bm, bit)    ((bm) & (typeof(bm))(1U << (bit)))

#define MPU_CMD_BOND_CREATE     17
#define MPU_CMD_BOND_DELETE     18
#define MPU_CMD_BOND_SET_ATTR   19
#define MPU_CMD_BOND_GET_ATTR   20

#define HINIC3_MAX_PORT 4
#define HINIC3_IFNAMSIZ 16
struct hinic3_bond_info_s {
	u8 slaves;
	u8 cnt;
	u8 srv[2];
	char slaves_name[HINIC3_MAX_PORT][HINIC3_IFNAMSIZ];
};

#pragma pack(push, 1)
struct netdev_lower_state_info {
	u8 link_up : 1;
	u8 tx_enabled : 1;
	u8 rsvd : 6;
};

#pragma pack(pop)

struct bond_tracker {
	struct netdev_lower_state_info netdev_state[BOND_PORT_MAX_NUM];
	struct net_device *ndev[BOND_PORT_MAX_NUM];
	u8 cnt;
	bool is_bonded;
};

struct bond_attr {
	u16 bond_mode;
	u16 bond_id;
	u16 up_delay;
	u16 down_delay;
	u8 active_slaves;
	u8 slaves;
	u8 lacp_collect_slaves;
	u8 xmit_hash_policy;
	u32 first_roce_func;
	u32 bond_pf_bitmap;
	u32 user_bitmap;
};

struct hinic3_bond_cmd {
	u8 ret_status;
	u8 version;
	u16 sub_cmd;
	struct bond_attr attr;
	char bond_name[16];
};

bool hinic3_is_bond_dev_status_actived(struct net_device *ndev);
void hinic3_bond_set_user_bitmap(struct bond_attr *attr, enum hinic3_bond_user user);
int hinic3_bond_attach(const char *name, enum hinic3_bond_user user, u16 *bond_id);
int hinic3_bond_detach(u16 bond_id, enum hinic3_bond_user user);
void hinic3_bond_clean_user(enum hinic3_bond_user user);
int hinic3_bond_get_uplink_id(u16 bond_id, u32 *uplink_id);
int hinic3_bond_register_service_func(enum hinic3_bond_user user, void (*func)
				      (const char *bond_name, void *bond_attr,
				      enum bond_service_proc_pos pos));
int hinic3_bond_unregister_service_func(enum hinic3_bond_user user);
int hinic3_bond_get_slaves(u16 bond_id, struct hinic3_bond_info_s *info);
struct net_device *hinic3_bond_get_netdev_by_portid(const char *bond_name, u8 port_id);
int hinic3_get_hw_bond_infos(void *hwdev, struct hinic3_hw_bond_infos *infos, u16 channel);
int hinic3_get_bond_tracker_by_name(const char *name, struct bond_tracker *tracker);
#endif /* HINIC3_BOND_H */

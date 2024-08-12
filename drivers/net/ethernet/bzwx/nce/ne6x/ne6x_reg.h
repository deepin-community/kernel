/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_REG_H
#define _NE6X_REG_H

#include <asm/types.h>

struct ne6x_diag_reg_test_info {
	u32 offset;   /* the base register */
	u64 mask;     /* bits that can be tested */
	u32 elements; /* number of elements if array */
	u32 stride;   /* bytes between each element */
};

enum ne6x_reg_table {
	NE6X_REG_RSS_TABLE = 0x0,
	NE6X_REG_L2FDB_TABLE,
	NE6X_REG_VLAN_TABLE,
	NE6X_REG_MAC_LEARN_TABLE,
	NE6X_REG_VF_STAT_TABLE,
	NE6X_REG_VF_BW_TABLE,
	NE6X_REG_ACL_TABLE,
	NE6X_REG_ARFS_TABLE,
	NE6X_REG_TABLE_LAST,
};

enum ne6x_reg_talk_port {
	NE6X_MSG_PORT_ENABLE = 0,
	NE6X_MSG_PORT_DUPLEX,
	NE6X_MSG_PORT_SPEED,
	NE6X_MSG_PORT_STATS,
	NE6X_MSG_PORT_SFP_SPEED,
	NE6X_MSG_PORT_FEC,
	NE6X_MSG_PORT_SPEED_MAX,
	NE6X_MSG_PORT_PAUSE,
	NE6X_MSG_PORT_PAUSE_ADDR,
	NE6X_MSG_PORT_LOOPBACK,
	NE6X_MSG_PORT_MAX_FRAME,
	NE6X_MSG_PORT_AUTO_NEG,
	NE6X_MSG_PORT_INFO,
	NE6X_MSG_PORT_LINK_STATUS,
	NE6X_MSG_PORT_DRV_I2C,
	NE6X_MSG_PORT_SELF_TEST,
	NE6X_MSG_PORT_SFP_TYPE_LEN,
	NE6X_MSG_PORT_SFP_EEPROM,
	NE6X_MSG_PORT_STATE,
};

enum ne6x_reg_talk_opcode {
	NE6X_TALK_SET = 0,
	NE6X_TALK_GET
};

extern struct ne6x_diag_reg_test_info ne6x_reg_list[];

struct table_info {
	u32 addr; /* 00 - 27: max_size
		   * 28 - 31: engine_idx
		   */
	u32 size;
	/* 00 - 15: length
	 * 16 - 20:
	 * 21 - 23: entry_num
	 * 24 - 26: mem_type
	 * 27 - 27: mem_type_bucekt
	 * 28 - 31: opcode
	 */
	u16 opcode_read;
	u16 opcode_write;
#define ADV_CMD_DISABLE 0x00
#define ADV_CMD_EBABLE 0x01
	u32 advanced_cmd;
	u16 opcode_insert;
	u16 opcode_delete;
	u16 opcode_update;
	u16 opcode_search;
	u16 size_insert;
	u16 size_delete;
	u16 size_search;
	u16 size_update;
};

struct rss_table {
	u32 resv;
	u32 flag;
	u32 hash_fun; /* 24-31, func, 23-1,type */
	u32 queue_base;
	u16 queue_def;
	u16 queue_size;
	u16 entry_num;
	u16 entry_size;
	u8  entry_data[128];
	u8  hash_key[352];
	u8  resv1[8];
};

struct l2fdb_dest_unicast {
	u8  flags; /* bit0 -- static,bit1---multicast */
	u8  rsv[3];
	u32 vp_bmp[3];
	u32 cnt; /* leaf num */
	u8  resv3[44];
};

struct l2fdb_dest_multicast {
	u8  flags; /* bit0 -- static,bit1---multicast */
	u8  resv3[3];
	u32 vp_bmp[3];
	u8  resv4[48];
};

struct l2fdb_search_result {
	u32 key_index;
	union {
		struct l2fdb_dest_unicast unicast;
		struct l2fdb_dest_multicast multicast;
	} fw_info;
};

struct l2fdb_table {
	u8  resv1;
	u8  pport;
	u8  mac[6];
	u32 vlanid;
	u8  resv2[52];
	union {
		struct l2fdb_dest_unicast unicast;
		struct l2fdb_dest_multicast multicast;
	} fw_info; /* forward info */
};

struct l2fdb_fast_table {
	u8 mac[6];
	u8 start_cos;
	u8 cos_num;
};

struct meter_table {
	u32 cir;
	u32 cbs;
	u32 pir;
	u32 pbs;
};

enum np_user_data {
	NP_USER_DATA_HW_FEATURES             = 0,
	NP_USER_DATA_HW_FLAGS                = 1,
	NP_USER_DATA_RSS_TABLE_SIZE          = 2,
	NP_USER_DATA_RSS_TABLE_ENTRY_WIDTH   = 3,
	NP_USER_DATA_RSS_HASH_KEY_BLOCK_SIZE = 4,
	NP_USER_DATA_PORT2PI_0               = 5,
	NP_USER_DATA_PI2PORT_0               = 25,
	NP_USER_DATA_VLAN_TYPE               = 33,
	NP_USER_DATA_RSV_0                   = 34,
	NP_USER_DATA_RSV_1                   = 35,
	NP_USER_DATA_RSV_2                   = 36,
	NP_USER_DATA_PI0_BROADCAST_LEAF      = 37,
	NP_USER_DATA_PORT_OLFLAGS_0          = 53,
	NP_USER_DATA_PORT_2_COS_0            = 121,
	NP_USER_DATA_VPORT0_LINK_STATUS      = 155,
	NP_USER_DATA_TSO_CKSUM_DISABLE       = 156,
	NP_USER_DATA_PORT0_MTU               = 157,
	NP_USER_DATA_PORT0_QINQ              = 161,
	NP_USER_DATA_CQ_SIZE                 = 229,
	NP_USER_DATA_FAST_MODE               = 230,
	NP_USER_DATA_SUB_FLAG                = 231,
	NP_USER_DATA_DDOS_FLAG               = 242,
	NP_USER_DATA_END                     = 255,
};

struct ne6x_diag_reg_info {
	u32 address;
	u32 value;
};

enum {
	NE6X_NORFLASH_OP_WRITE_E = 0,
	NE6X_NORFLASH_OP_READ_E  = 1,
	NE6X_NORFLASH_OP_ERASE_E = 2,
	NE6X_NORFLASH_OP_E_END,
};

void ne6x_reg_pci_write(struct ne6x_pf *pf, u32 base_addr,
			u32 offset_addr, u64 reg_value);
u64 ne6x_reg_pci_read(struct ne6x_pf *pf, u32 base_addr, u32 offset_addr);

u32 ne6x_reg_apb_read(struct ne6x_pf *pf, u64 offset);
void ne6x_reg_apb_write(struct ne6x_pf *pf, u64 offset, u32 value);
int ne6x_reg_reset_firmware(struct ne6x_pf *pf);
u32 ne6x_reg_apb_read(struct ne6x_pf *pf, u64 offset);
void ne6x_reg_apb_write(struct ne6x_pf *pf, u64 offset, u32 value);

int ne6x_reg_indirect_read(struct ne6x_pf *pf, u32 addr, u32 *value);
int ne6x_reg_indirect_write(struct ne6x_pf *pf, u32 addr, u32 value);
int ne6x_reg_table_read(struct ne6x_pf *pf, enum ne6x_reg_table table,
			int index, void *data, int size);
int ne6x_reg_table_write(struct ne6x_pf *pf, enum ne6x_reg_table table,
			 int index, void *data, int size);
int ne6x_reg_table_insert(struct ne6x_pf *pf, enum ne6x_reg_table table,
			  u32 *data, int size, u32 *table_id);
int ne6x_reg_table_delete(struct ne6x_pf *pf, enum ne6x_reg_table table,
			  u32 *data, int size);
int ne6x_reg_table_search(struct ne6x_pf *pf, enum ne6x_reg_table table,
			  u32 *data, int size, u32 *ret_data, int ret_size);

int ne6x_reg_e2prom_read(struct ne6x_pf *pf, u32 offset, void *pbuf, int size);
int ne6x_reg_e2prom_write(struct ne6x_pf *pf, u32 offset, void *pbuf, int size);
int ne6x_reg_set_fan_speed(struct ne6x_pf *pf, u32 speed);
int ne6x_reg_get_fan_speed(struct ne6x_pf *pf, u32 *speed);

int ne6x_reg_get_soc_info(struct ne6x_pf *pf, u32 class_type, u32 *ret, u32 size);
int ne6x_reg_talk_port(struct ne6x_pf *pf, enum ne6x_reg_talk_port talk,
		       enum ne6x_reg_talk_opcode opcode, int port,
		       void *pbuf, int size);
int ne6x_reg_upgrade_firmware(struct ne6x_pf *pf, u8 region, u8 *data, int size);

int ne6x_reg_get_ver(struct ne6x_pf *pf, struct ne6x_firmware_ver_info *version);

int ne6x_reg_get_sfp_eeprom(struct ne6x_pf *pf, int port, void *pbuf,
			    u32 offset, int size);

int ne6x_reg_nic_start(struct ne6x_pf *pf, u32 flag);
int ne6x_reg_nic_stop(struct ne6x_pf *pf, u32 flag);

int ne6x_reg_get_nic_state(struct ne6x_pf *pf, u32 *state);

int ne6x_reg_set_user_data(struct ne6x_pf *pf, enum np_user_data type, u32 data);
int ne6x_reg_get_user_data(struct ne6x_pf *pf, enum np_user_data type, u32 *data);

int ne6x_reg_set_led(struct ne6x_pf *pf, int port, bool state);
int ne6x_reg_config_meter(struct ne6x_pf *pf, u32 meter_id, u32 *data, int size);

int ne6x_reg_send_bit(struct ne6x_pf *pf, u32 port, u32 mode);

int ne6x_reg_set_unicast_for_fastmode(struct ne6x_pf *pf, u32 index,
				      u32 *data, u32 size);
int ne6x_reg_get_dump_data_len(struct ne6x_pf *pf, u32 *size);
int ne6x_reg_get_dump_data(struct ne6x_pf *pf, u32 *data, u32 size);
int ne6x_reg_clear_table(struct ne6x_pf *pf, u32 table_id);

int ne6x_reg_set_norflash_write_protect(struct ne6x_pf *pf, u32 write_protect);
int ne6x_reg_get_norflash_write_protect(struct ne6x_pf *pf, u32 *p_write_protect);

int ne6x_reg_write_norflash(struct ne6x_pf *pf, u32 offset, u32 length, u32 *pdata);
int ne6x_reg_erase_norflash(struct ne6x_pf *pf, u32 offset, u32 length);
int ne6x_reg_read_norflash(struct ne6x_pf *pf, u32 offset, u32 length, u32 *p);

#endif

// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include <linux/io.h>
#include <linux/slab.h>

#include "ne6x.h"
#include "ne6x_reg.h"
#include "ne6x_portmap.h"

#define AXIA_MBUS_READ_MEMORY_COMMAND     0x07
#define AXIA_MBUS_READ_MEMORY_ACK         0x08

#define AXIA_MBUS_WRITE_MEMORY_COMMAND    0x09
#define AXIA_MBUS_WRITE_MEMORY_ACK        0x0A

#define AXIA_MBUS_READ_REGISTER_COMMAND   0x0B
#define AXIA_MBUS_READ_REGISTER_ACK       0x0C

#define AXIA_MBUS_WRITE_REGISTER_COMMAND  0x0D
#define AXIA_MBUS_WRITE_REGISTER_ACK      0x0E

#define AXIA_MBUS_RESET_FIRMWARE_COMMAND  0x0F
#define AXIA_MBUS_RESET_FIRMWARE_ACK      0x10
#define AXIA_MBUS_READ_TABLE_COMMAND      0x11
#define AXIA_MBUS_READ_TABLE_ACK          0x12

#define AXIA_MBUS_WRITE_TABLE_COMMAND     0x13
#define AXIA_MBUS_WRITE_TABLE_ACK         0x14

#define AXIA_MBUS_CLEARUP_COMMAND         0x15
#define AXIA_MBUS_CLEARUP_ACK             0x16

/* hash table operator */
#define AXIA_MBUS_INSERT_COMMAND          0x17
#define AXIA_MBUS_INSERT_ACK              0x18

#define AXIA_MBUS_UPDATE_COMMAND          0x19
#define AXIA_MBUS_UPDATE_ACK              0x1A

#define AXIA_MBUS_DELETE_COMMAND          0x1B
#define AXIA_MBUS_DELETE_ACK              0x1C

#define AXIA_MBUS_LOOKUP_COMMAND          0x1D
#define AXIA_MBUS_LOOKUP_ACK              0x1E

/* data download operator */
#define AXIA_MBUS_DOWNLOAD_COMMAND        0x21
#define AXIA_MBUS_DOWNLOAD_ACK            0x22

#define AXIA_MBUS_OPERATOR_COMMAND        0x23
#define AXIA_MBUS_OPERATOR_ACK            0x24

#define AXIA_MBUS_SETUP_PORT_COMMAND      0x25
#define AXIA_MBUS_SETUP_PORT_ACK          0x26

#define AXIA_MBUS_SETUP_TABLE_COMMAND     0x27
#define AXIA_MBUS_SETUP_TABLE_ACK         0x28

#define AXIA_MBUS_SETUP_TAPI_COMMAND      0x29
#define AXIA_MBUS_SETUP_TAPI_ACK          0x2A

#define AXIA_MBUS_SETUP_HASH_COMMAND      0x2B
#define AXIA_MBUS_SETUP_HASH_ACK          0x2C

#define AXIA_MBUS_SETUP_DTAB_COMMAND      0x2D
#define AXIA_MBUS_SETUP_DTAB_ACK          0x2E

#define AXIA_MBUS_E2PROM_READ_COMMAND     0x2F
#define AXIA_MBUS_E2PROM_READ_ACK         0x30

#define AXIA_MBUS_E2PROM_WRITE_COMMAND    0x31
#define AXIA_MBUS_E2PROM_WRITE_ACK        0x32

#define AXIA_MBUS_SET_FAN_SPEED_COMMAND   0x33
#define AXIA_MBUS_SET_FAN_SPEED_ACK       0x34

#define AXIA_MBUS_GET_FAN_SPEED_COMMAND   0x35
#define AXIA_MBUS_GET_FAN_SPEED_ACK       0x36

#define AXIA_MBUS_GET_SYSTEM_INFO_COMMAND 0x37
#define AXIA_MBUS_GET_SYSTEM_INFO_ACK     0x38

#define AXIA_MBUS_UPGRADE_PRE_COMMAND     0x39
#define AXIA_MBUS_UPGRADE_PRE_COMMAND_ACK 0x3A
#define AXIA_MBUS_UPGRADE_COMMAND         0x3B
#define AXIA_MBUS_UPGRADE_COMMAND_ACK     0x3C

#define AXIA_MBUS_GET_VER_COMMAND         0x3D
#define AXIA_MBUS_GET_VER_COMMAND_ACK     0x3E

#define AXIA_MBUS_TALK_PORT_BASE          0x41

#define AXIA_MBUS_TALK_SET_PORT_ENABLE_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_ENABLE + 0)
#define AXIA_MBUS_TALK_SET_PORT_ENABLE_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_ENABLE + 1)

#define AXIA_MBUS_TALK_GET_PORT_ENABLE_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_ENABLE + 2)
#define AXIA_MBUS_TALK_GET_PORT_ENABLE_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_ENABLE + 3)

#define AXIA_MBUS_TALK_SET_PORT_DUPLEX_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_DUPLEX + 0)
#define AXIA_MBUS_TALK_SET_PORT_DUPLEX_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_DUPLEX + 1)

#define AXIA_MBUS_TALK_GET_PORT_DUPLEX_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_DUPLEX + 2)
#define AXIA_MBUS_TALK_GET_PORT_DUPLEX_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_DUPLEX + 3)

#define AXIA_MBUS_TALK_SET_PORT_SPEED_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SPEED + 0)
#define AXIA_MBUS_TALK_SET_PORT_SPEED_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SPEED + 1)

#define AXIA_MBUS_TALK_GET_PORT_SPEED_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SPEED + 2)
#define AXIA_MBUS_TALK_GET_PORT_SPEED_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SPEED + 3)

#define AXIA_MBUS_TALK_SET_PORT_STATS_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_STATS + 0)
#define AXIA_MBUS_TALK_SET_PORT_STATS_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_STATS + 1)

#define AXIA_MBUS_TALK_GET_PORT_STATS_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_STATS + 2)
#define AXIA_MBUS_TALK_GET_PORT_STATS_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_STATS + 3)

#define AXIA_MBUS_TALK_SET_PORT_SFP_SPEED_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_SPEED + 0)
#define AXIA_MBUS_TALK_SET_PORT_SFP_SPEED_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_SPEED + 1)

#define AXIA_MBUS_TALK_GET_PORT_SFP_SPEED_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_SPEED + 2)
#define AXIA_MBUS_TALK_GET_PORT_SFP_SPEED_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_SPEED + 3)

#define AXIA_MBUS_TALK_SET_PORT_FEC_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_FEC + 0)
#define AXIA_MBUS_TALK_SET_PORT_FEC_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_FEC + 1)

#define AXIA_MBUS_TALK_GET_PORT_FEC_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_FEC + 2)
#define AXIA_MBUS_TALK_GET_PORT_FEC_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_FEC + 3)

#define AXIA_MBUS_TALK_SET_PORT_SPEED_MAX_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SPEED_MAX + 0)
#define AXIA_MBUS_TALK_SET_PORT_SPEED_MAX_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SPEED_MAX + 1)

#define AXIA_MBUS_TALK_GET_PORT_SPEED_MAX_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SPEED_MAX + 2)
#define AXIA_MBUS_TALK_GET_PORT_SPEED_MAX_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SPEED_MAX + 3)

#define AXIA_MBUS_TALK_SET_PORT_PAUSE_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_PAUSE + 0)
#define AXIA_MBUS_TALK_SET_PORT_PAUSE_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_PAUSE + 1)

#define AXIA_MBUS_TALK_GET_PORT_PAUSE_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_PAUSE + 2)
#define AXIA_MBUS_TALK_GET_PORT_PAUSE_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_PAUSE + 3)

#define AXIA_MBUS_TALK_SET_PORT_PAUSE_ADDR_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_PAUSE_ADDR + 0)
#define AXIA_MBUS_TALK_SET_PORT_PAUSE_ADDR_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_PAUSE_ADDR + 1)

#define AXIA_MBUS_TALK_GET_PORT_PAUSE_ADDR_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_PAUSE_ADDR + 2)
#define AXIA_MBUS_TALK_GET_PORT_PAUSE_ADDR_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_PAUSE_ADDR + 3)

#define AXIA_MBUS_TALK_SET_PORT_LOOPBACK_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_LOOPBACK + 0)
#define AXIA_MBUS_TALK_SET_PORT_LOOPBACK_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_LOOPBACK + 1)

#define AXIA_MBUS_TALK_GET_PORT_LOOPBACK_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_LOOPBACK + 2)
#define AXIA_MBUS_TALK_GET_PORT_LOOPBACK_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_LOOPBACK + 3)

#define AXIA_MBUS_TALK_SET_PORT_MAX_FRAME_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_MAX_FRAME + 0)
#define AXIA_MBUS_TALK_SET_PORT_MAX_FRAME_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_MAX_FRAME + 1)

#define AXIA_MBUS_TALK_GET_PORT_MAX_FRAME_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_MAX_FRAME + 2)
#define AXIA_MBUS_TALK_GET_PORT_MAX_FRAME_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_MAX_FRAME + 3)

#define AXIA_MBUS_TALK_SET_PORT_AUTO_NEG_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_AUTO_NEG + 0)
#define AXIA_MBUS_TALK_SET_PORT_AUTO_NEG_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_AUTO_NEG + 1)

#define AXIA_MBUS_TALK_GET_PORT_AUTO_NEG_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_AUTO_NEG + 2)
#define AXIA_MBUS_TALK_GET_PORT_AUTO_NEG_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_AUTO_NEG + 3)

#define AXIA_MBUS_TALK_SET_PORT_INFO_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_INFO + 0)
#define AXIA_MBUS_TALK_SET_PORT_INFO_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_INFO + 1)

#define AXIA_MBUS_TALK_GET_PORT_INFO_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_INFO + 2)
#define AXIA_MBUS_TALK_GET_PORT_INFO_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_INFO + 3)

#define AXIA_MBUS_TALK_SET_PORT_LINK_STATUS_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_LINK_STATUS + 0)
#define AXIA_MBUS_TALK_SET_PORT_LINK_STATUS_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_LINK_STATUS + 1)

#define AXIA_MBUS_TALK_GET_PORT_LINK_STATUS_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_LINK_STATUS + 2)
#define AXIA_MBUS_TALK_GET_PORT_LINK_STATUS_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_LINK_STATUS + 3)

#define AXIA_MBUS_TALK_SET_PORT_DRV_I2C_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_DRV_I2C + 0)
#define AXIA_MBUS_TALK_SET_PORT_DRV_I2C_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_DRV_I2C + 1)

#define AXIA_MBUS_TALK_GET_PORT_DRV_I2C_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_DRV_I2C + 2)
#define AXIA_MBUS_TALK_GET_PORT_DRV_I2C_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_DRV_I2C + 3)

#define AXIA_MBUS_TALK_SET_PORT_SELF_TEST_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SELF_TEST + 0)
#define AXIA_MBUS_TALK_SET_PORT_SELF_TEST_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SELF_TEST + 1)

#define AXIA_MBUS_TALK_GET_PORT_SELF_TEST_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SELF_TEST + 2)
#define AXIA_MBUS_TALK_GET_PORT_SELF_TEST_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SELF_TEST + 3)

#define AXIA_MBUS_TALK_SET_PORT_SFP_TYPE_LEN_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_TYPE_LEN + 0)
#define AXIA_MBUS_TALK_SET_PORT_SFP_TYPE_LEN_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_TYPE_LEN + 1)

#define AXIA_MBUS_TALK_GET_PORT_SFP_TYPE_LEN_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_TYPE_LEN + 2)
#define AXIA_MBUS_TALK_GET_PORT_SFP_TYPE_LEN_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_TYPE_LEN + 3)

#define AXIA_MBUS_TALK_SET_PORT_SFP_EEPROM_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_EEPROM + 0)
#define AXIA_MBUS_TALK_SET_PORT_SFP_EEPROM_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_EEPROM + 1)

#define AXIA_MBUS_TALK_GET_PORT_SFP_EEPROM_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_EEPROM + 2)
#define AXIA_MBUS_TALK_GET_PORT_SFP_EEPROM_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_SFP_EEPROM + 3)

#define AXIA_MBUS_TALK_SET_PORT_STATE_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_STATE + 0)
#define AXIA_MBUS_TALK_SET_PORT_STATE_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_STATE + 1)

#define AXIA_MBUS_TALK_GET_PORT_STATE_COMMAND \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_STATE + 2)
#define AXIA_MBUS_TALK_GET_PORT_STATE_ACK \
	(AXIA_MBUS_TALK_PORT_BASE + 4 * NE6X_MSG_PORT_STATE + 3)

#define AXIA_MBUS_SET_NIC_START_COMMAND             0x9F
#define AXIA_MBUS_SET_NIC_START_ACK                 0xA0
#define AXIA_MBUS_SET_NIC_STOP_COMMAND              0xA1
#define AXIA_MBUS_SET_NIC_STOP_ACK                  0xA2
#define AXIA_MBUS_GET_NIC_STATE_COMMAND             0xA3
#define AXIA_MBUS_GET_NIC_STATE_ACK                 0xA4
#define AXIA_MBUS_SET_NP_USERDATA_COMMAND           0xA5
#define AXIA_MBUS_SET_NP_USERDATA_ACK               0xA6
#define AXIA_MBUS_GET_NP_USERDATA_COMMAND           0xA7
#define AXIA_MBUS_GET_NP_USERDATA_ACK               0xA8

#define AXIA_MBUS_SET_LED_STATE_COMMAND             0xA9
#define AXIA_MBUS_SET_LED_STATE_ACK                 0xAA

#define AXIA_MBUS_CONFIG_METER_COMMAND              0xAB
#define AXIA_MBUS_CONFIG_METER_ACK                  0xAC

#define AXIA_MBUS_CLEAR_CREDIT_COMMAND              0xAD
#define AXIA_MBUS_CLEAR_CREDIT_ACK                  0xAE

#define AXIA_MBUS_SET_FAST_L2FDB_COMMAND            0xD1
#define AXIA_MBUS_SET_FAST_L2FDB_ACK                0xD2

#define AXIA_MBUS_GET_DUMP_DATA_LEN_COMMAND         0xD3
#define AXIA_MBUS_GET_DUMP_DATA_LEN_ACK             0xD4

#define AXIA_MBUS_GET_DUMP_DATA_COMMAND             0xD5
#define AXIA_MBUS_GET_DUMP_DATA_ACK                 0xD6

#define AXIA_MBUS_CLR_TABLE_COMMAND                 0xD7
#define AXIA_MBUS_CLR_TABLE_ACK                     0xD8

#define AXIA_MBUS_SET_NOFLASH_WRITE_PROTECT_COMMAND 0xD9
#define AXIA_MBUS_SET_NOFLASH_WRITE_PROTECT_ACK     0xDA

#define AXIA_MBUS_GET_NOFLASH_WRITE_PROTECT_COMMAND 0xDB
#define AXIA_MBUS_GET_NOFLASH_WRITE_PROTECT_ACK     0xDC

#define AXIA_MBUS_OPT_NOFLASH_COMMAND               0xDD
#define AXIA_MBUS_OPT_NOFLASH_ACK                   0xDE

#define PCIE2C810_SHM_MBUS_BASE                     0x20878000
#define PCIE2C810_SHM_DATA_BASE                     0x20878004

#define MEM_ONCHIP_64BIT                            0x00
#define MEM_ONCHIP_512BIT                           0x01
#define MEM_ONXDDR_512BIT                           0x04

enum engine_idx {
	ENGINE_DIRECT_TABLE0 = 0x1,
	ENGINE_DIRECT_TABLE1,
	ENGINE_HASHA_TABLE,
	ENGINE_HASHB_TABLE,
};

struct axia_mbus_msg {
	union {
		u32 uint;
		struct {
#if defined(__BIG_ENDIAN_BITFIELD)
			u32 opcode    : 8;
			u32 dst_block : 4;
			u32 src_block : 4;
			u32 data_len  : 14;
			u32 e         : 2;
#elif defined(__LITTLE_ENDIAN_BITFIELD)
			u32 e         : 2;
			u32 data_len  : 14;
			u32 src_block : 4;
			u32 dst_block : 4;
			u32 opcode    : 8;
#endif
		} bits;
	} hdr;
	u32 data[];
} __packed;

struct ne6x_diag_reg_test_info ne6x_reg_list[] = {
	/* offset               mask                elements    stride */
	{NE6X_VP_BASE_ADDR, 0xFFFFFFFFFFFFFFFF, NE6X_VP_INT, 0},
	{0}
};

struct ne6x_reg_table_info {
	u32 addr; /* engine id  as base address */
	u32 size; /* 00 - 15: length
		   * 16 - 20:
		   * 21 - 23: entry_num
		   * 24 - 26: mem_type
		   * 27 - 27: mem_type_bucekt
		   * 28 - 31: opcode
		   */
	u32 opcode_read;
	u32 opcode_write;
#define ADV_CMD_DISABLE 0x00
#define ADV_CMD_EBABLE 0x01
	u32 advanced_cmd;
	u32 opcode_insert;
	u32 opcode_delete;
	u32 opcode_lookup;
	u32 opcode_update;
	u32 size_insert;
	u32 size_delete;
	u32 size_lookup;
	u32 size_update;
};

static struct ne6x_reg_table_info table_info[] = {
	/* address size(tableidx + memtype + bucket + entry_num + size)
	 * read  write	adv_cmd  insert delete lookup  size_insert size_delete size_lookup
	 */
	{0x00000000,
	 (ENGINE_DIRECT_TABLE0 << 28) | (MEM_ONCHIP_64BIT << 24) | (1 << 21) | (8 << 16) | 0x0200,
	 AXIA_MBUS_READ_TABLE_COMMAND, AXIA_MBUS_WRITE_TABLE_COMMAND, 0x00, 0x00, 0x00, 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00},

	{0x10000000,
	 (ENGINE_DIRECT_TABLE0 << 28) | (MEM_ONCHIP_64BIT << 24) | (1 << 21) | (2 << 16) | 0x0040,
	 AXIA_MBUS_READ_TABLE_COMMAND, AXIA_MBUS_WRITE_TABLE_COMMAND, 0x01,
	 AXIA_MBUS_INSERT_COMMAND, AXIA_MBUS_DELETE_COMMAND, AXIA_MBUS_LOOKUP_COMMAND,
	 AXIA_MBUS_UPDATE_COMMAND, 128, 64, 64, 64},

	{0x20000000,
	 (ENGINE_DIRECT_TABLE0 << 28) | (MEM_ONCHIP_64BIT << 24) | (1 << 21) | (2 << 16) | 0x0010,
	 AXIA_MBUS_READ_TABLE_COMMAND, AXIA_MBUS_WRITE_TABLE_COMMAND, 0x00, 0x31, 0x33, 0x35, 0x00,
	 0x00, 0x00, 0x00, 0x00},

	{0x30000000,
	 (ENGINE_DIRECT_TABLE0 << 28) | (MEM_ONCHIP_64BIT << 24) | (1 << 21) | (8 << 16) | 0x0008,
	 AXIA_MBUS_READ_TABLE_COMMAND, AXIA_MBUS_WRITE_TABLE_COMMAND, 0x00, 0x31, 0x33, 0x35, 0x00,
	 0x00, 0x00, 0x00, 0x00},

	{0x40000000,
	 (ENGINE_DIRECT_TABLE0 << 28) | (MEM_ONCHIP_64BIT << 24) | (1 << 21) | (4 << 16) | 0x0100,
	 AXIA_MBUS_READ_TABLE_COMMAND, AXIA_MBUS_WRITE_TABLE_COMMAND, 0x00, 0x31, 0x33, 0x35, 0x00,
	 0x00, 0x00, 0x00, 0x00},

	{0x50000000,
	 (ENGINE_DIRECT_TABLE0 << 28) | (MEM_ONCHIP_512BIT << 24) | (1 << 21) | (1 << 16) | 0x0040,
	 AXIA_MBUS_READ_TABLE_COMMAND, AXIA_MBUS_WRITE_TABLE_COMMAND, 0x00, 0x31, 0x33, 0x35, 0x00,
	 0x00, 0x00, 0x00, 0x00},

	{0x60000000,
	 (ENGINE_DIRECT_TABLE0 << 28) | (MEM_ONCHIP_64BIT << 24) | (1 << 21) | (2 << 16) | 0x0040,
	 AXIA_MBUS_READ_TABLE_COMMAND, AXIA_MBUS_WRITE_TABLE_COMMAND, 0x01,
	 AXIA_MBUS_INSERT_COMMAND, AXIA_MBUS_DELETE_COMMAND, AXIA_MBUS_LOOKUP_COMMAND,
	 AXIA_MBUS_UPDATE_COMMAND, 128, 64, 64, 64},

	{0x70000000,
	 (ENGINE_DIRECT_TABLE0 << 28) | (MEM_ONCHIP_64BIT << 24) | (1 << 21) | (2 << 16) | 0x0040,
	 AXIA_MBUS_READ_TABLE_COMMAND, AXIA_MBUS_WRITE_TABLE_COMMAND, 0x01,
	 AXIA_MBUS_INSERT_COMMAND, AXIA_MBUS_DELETE_COMMAND, AXIA_MBUS_LOOKUP_COMMAND,
	 AXIA_MBUS_UPDATE_COMMAND, 96, 64, 64, 32},
};

#define TABLE_ADDR(table)            (table_info[table].addr & 0xF0000000)
#define TABLE_SIZE(table)            (table_info[table].size & 0x00000FFF)
#define TABLE_XMEM(table)            (table_info[table].size & 0xFFE00000)
#define TABLE_XNUM(table)            ((table_info[table].size >> 16) & 0xF)

#define TABLE_OPCODE_WRITE(table)    (table_info[table].opcode_write & 0x3F)
#define TABLE_OPCODE_READ(table)     (table_info[table].opcode_read & 0x3F)
#define TABLE_ADVCMD_VALID(table)    (table_info[table].advanced_cmd == 0x01)
#define TABLE_OPCODE_INSERT(table)   (table_info[table].opcode_insert & 0x3F)
#define TABLE_OPCODE_DELETE(table)   (table_info[table].opcode_delete & 0x3F)
#define TABLE_OPCODE_LOOKUP(table)   (table_info[table].opcode_lookup & 0x3F)

#define TABLE_OPCODE_UPDATE(table)   (table_info[table].opcode_update & 0x3F)

#define TABLE_SIZE_INSERT(table)     (table_info[table].size_insert)
#define TABLE_SIZE_DELETE(table)     (table_info[table].size_delete)
#define TABLE_SIZE_LOOKUP(table)     (table_info[table].size_lookup)
#define TABLE_SIZE_UPDATE(table)     (table_info[table].size_update)
#define TABLE_SIZE_LOOKUP_RET(table) (table_info[table].size & 0xFFF)

#define NUM_TABLE(table)             (table_info[table].table_num)

static u64 local_module_base;

void ne6x_reg_lock(struct ne6x_pf *pf)
{
	mutex_lock(&pf->mbus_comm_mutex);
}

void ne6x_reg_unlock(struct ne6x_pf *pf)
{
	mutex_unlock(&pf->mbus_comm_mutex);
}

void ne6x_switch_pci_write(void *bar_base, u32 base_addr, u32 offset_addr, u64 reg_value)
{
	unsigned int reg_offset = 0;
	void __iomem *addr = NULL;

	reg_offset = (base_addr << 12) + (offset_addr << 4);
	addr = bar_base + reg_offset;
	writeq(reg_value, addr);
}

u64 ne6x_switch_pci_read(void *bar_base, u32 base_addr, u32 offset_addr)
{
	unsigned int reg_offset = 0;
	void __iomem *addr = NULL;
	u64 val = 0;

	reg_offset = (base_addr << 12) + (offset_addr << 4);
	addr = bar_base + reg_offset;
	val = readq(addr);

	return val;
}

void ne6x_reg_pci_write(struct ne6x_pf *pf, u32 base_addr, u32 offset_addr, u64 reg_value)
{
	ne6x_switch_pci_write(pf->hw.hw_addr4, base_addr, offset_addr, reg_value);
}

u64 ne6x_reg_pci_read(struct ne6x_pf *pf, u32 base_addr, u32 offset_addr)
{
	return ne6x_switch_pci_read(pf->hw.hw_addr4, base_addr, offset_addr);
}

#define BAR4_CSR_OFFSET 0x3C0
u32 ne6x_reg_axi_read(struct ne6x_pf *pf, u32 offset)
{
	u64 reg_offset = offset & 0xFFFFFFFC;
	u64 reg_value = 0x4000000000000000ULL + (reg_offset << 30);

	ne6x_reg_pci_write(pf, BAR4_CSR_OFFSET, 0x0, reg_value);
	reg_value = (reg_offset << 30);
	ne6x_reg_pci_write(pf, BAR4_CSR_OFFSET, 0x0, reg_value);
	reg_value = ne6x_reg_pci_read(pf, BAR4_CSR_OFFSET, 0x0);
	reg_value = ne6x_reg_pci_read(pf, BAR4_CSR_OFFSET, 0x0);

	return ne6x_reg_pci_read(pf, BAR4_CSR_OFFSET, 0x0) & 0xFFFFFFFFUL;
}

void ne6x_reg_axi_write(struct ne6x_pf *pf, u32 offset, u32 value)
{
	u64 reg_offset = offset & 0xFFFFFFFC;
	u64 reg_value = 0x4000000000000000ULL + (reg_offset << 30) + value;

	reg_offset = (reg_offset << 30);
	ne6x_reg_pci_write(pf, BAR4_CSR_OFFSET, 0x0, reg_value);
}

u32 _reg_apb_read(struct ne6x_pf *pf, u64 offset)
{
	u32 offset_l = 0x27A00000 | ((offset << 4) & 0xFFFF0);
	u32 offset_h;
	u32 data = 0;

	if ((offset & 0xFFFFF0000ULL) != local_module_base) {
		offset_h = 0x10000000 | ((offset >> 12) & 0xFFFFF0);
		ne6x_reg_axi_write(pf, offset_h, 0xA1B2C3D4);
	}

	data = ne6x_reg_axi_read(pf, offset_l);

	return data;
}

void _reg_apb_write(struct ne6x_pf *pf, u64 offset, u32 value)
{
	u32 offset_l;
	u32 offset_h;

	if ((offset & 0xFFFFF0000ULL) != local_module_base) {
		offset_h = 0x10000000 | ((offset >> 12) & 0xFFFFF0);
		ne6x_reg_axi_write(pf, offset_h, 0xA2B2C3D4);
	}

	offset_l = 0x2FA00000 | ((offset << 4) & 0xFFFF0);
	ne6x_reg_axi_write(pf, offset_l, value);
}

u32 NE6X_ACCESS_TIMEOUT = 9999;
int _ne6x_reg_perform(struct ne6x_pf *pf, u32 *data, u32 *pbuf, u32 len, u32 retlen)
{
	struct axia_mbus_msg resp;
	int timeout = 0, index = 0;

	memset(&resp, 0, sizeof(resp));

	/* Write Command(s) */
	for (index = 0; index < len; index++)
		_reg_apb_write(pf, PCIE2C810_SHM_MBUS_BASE + 4 * index, data[index]);

	/* Start mbus mechanism, notice c810 */
	_reg_apb_write(pf, 0x20680014, 0x3FEC);

	usleep_range(200, 300);

	/* check if c810 handle completed */
	while (timeout < NE6X_ACCESS_TIMEOUT) {
		resp.hdr.uint = _reg_apb_read(pf, PCIE2C810_SHM_MBUS_BASE);

		/* resp opcode is even number, request opcode is odd number */
		if ((resp.hdr.bits.opcode & 0x01) == 0x0)
			break;

		timeout++;
		usleep_range(200, 220);
	}

	if (timeout >= NE6X_ACCESS_TIMEOUT) {
		dev_info(ne6x_pf_to_dev(pf), "%s: timeout! (%d)\n", __func__, timeout);
		return -ETIMEDOUT;
	}

	if (resp.hdr.bits.e == 1) {
		dev_info(ne6x_pf_to_dev(pf), "%s: response.bits.e = 1 !\n", __func__);
		return -EAGAIN;
	}

	if (!pbuf)
		return 0;

	for (index = 0; index < retlen; index++)
		pbuf[index] = _reg_apb_read(pf, PCIE2C810_SHM_DATA_BASE + 4 * index);

	return 0;
}

int ne6x_reg_perform(struct ne6x_pf *pf, u32 *data, u32 *pbuf, u32 len, u32 retlen)
{
	int status;

	ne6x_reg_lock(pf);
	status = _ne6x_reg_perform(pf, data, pbuf, len, retlen);
	ne6x_reg_unlock(pf);

	return status;
}

u32 ne6x_reg_apb_read(struct ne6x_pf *pf, u64 offset)
{
	u32 data;

	ne6x_reg_lock(pf);
	data = _reg_apb_read(pf, offset);
	ne6x_reg_unlock(pf);

	return data;
}

void ne6x_reg_apb_write(struct ne6x_pf *pf, u64 offset, u32 value)
{
	ne6x_reg_lock(pf);
	_reg_apb_write(pf, offset, value);
	ne6x_reg_unlock(pf);
}

int ne6x_reg_indirect_read(struct ne6x_pf *pf, u32 addr, u32 *value)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(16, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_READ_REGISTER_COMMAND;
	msg->hdr.bits.data_len = 8;
	msg->data[0] = addr;

	status = ne6x_reg_perform(pf, (u32 *)msg, value, 2, 1);
	kfree(msg);

	return status;
}

int ne6x_reg_indirect_write(struct ne6x_pf *pf, u32 addr, u32 value)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(16, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_WRITE_REGISTER_COMMAND;
	msg->hdr.bits.data_len = 12;
	msg->data[0] = addr;
	msg->data[1] = value;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 3, 0);
	kfree(msg);

	return status;
}

static bool ne6x_reg_valid_table(struct ne6x_pf *pf, enum ne6x_reg_table table)
{
	if (pf->hw_flag != 0) {
		if (table > NE6X_REG_ARFS_TABLE)
			return false;
	} else {
		if (table > NE6X_REG_VF_BW_TABLE)
			return false;
	}

	return true;
}

int ne6x_reg_table_read(struct ne6x_pf *pf, enum ne6x_reg_table table,
			int index, void *data, int size)
{
	struct axia_mbus_msg *msg;
	int status;

	if (size % TABLE_SIZE(table) != 0x00)
		return -EINVAL;

	if (!ne6x_reg_valid_table(pf, table))
		return -EINVAL;

	msg = kzalloc(1028, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = (u32)(TABLE_OPCODE_READ(table));
	msg->hdr.bits.data_len = 12;
	msg->data[0] = TABLE_ADDR(table) + index * TABLE_XNUM(table);
	msg->data[1] = TABLE_XMEM(table) + size;

	status = ne6x_reg_perform(pf, (u32 *)msg, (u32 *)data, 3, size / 4);
	kfree(msg);

	return status;
}

int ne6x_reg_table_write(struct ne6x_pf *pf, enum ne6x_reg_table table,
			 int index, void *data, int size)
{
	struct axia_mbus_msg *msg;
	int status;

	if (TABLE_ADVCMD_VALID(table))
		return -EINVAL;

	if (!ne6x_reg_valid_table(pf, table))
		return -EINVAL;

	msg = kzalloc(1028, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = (u32)(TABLE_OPCODE_WRITE(table));
	msg->hdr.bits.data_len = 12 + size;
	msg->data[0] = TABLE_ADDR(table) + index * TABLE_XNUM(table);
	msg->data[1] = TABLE_XMEM(table) + size;
	memcpy(&msg->data[2], data, size);

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 3 + size / 4, 0);
	kfree(msg);

	return status;
}

int ne6x_reg_table_insert(struct ne6x_pf *pf, enum ne6x_reg_table table,
			  u32 *data, int size, u32 *table_id)
{
	struct axia_mbus_msg *msg;
	int status, count;

	if (TABLE_ADVCMD_VALID(table) == 0x0)
		return -EINVAL;

	if (size % TABLE_SIZE_INSERT(table) != 0x00)
		return -EINVAL;

	if (!ne6x_reg_valid_table(pf, table))
		return -EINVAL;

	msg = kzalloc(1028, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	count = size / TABLE_SIZE_INSERT(table);

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = (u32)(TABLE_OPCODE_INSERT(table));
	msg->hdr.bits.data_len = 12 + size;
	msg->data[0] = TABLE_ADDR(table);
	msg->data[1] = TABLE_XMEM(table) + TABLE_SIZE_INSERT(table);
	memcpy((void *)&msg->data[2], (void *)data, size);

	status = ne6x_reg_perform(pf, (u32 *)msg, table_id, 3 + (size >> 2),
				  (!table_id) ? 0 : count);
	kfree(msg);

	return status;
}

int ne6x_reg_table_delete(struct ne6x_pf *pf, enum ne6x_reg_table table, u32 *data, int size)
{
	struct axia_mbus_msg *msg;
	int status;

	if (TABLE_ADVCMD_VALID(table) == 0x0)
		return -EINVAL;

	if (TABLE_SIZE_DELETE(table) != size)
		return -EINVAL;

	if (!ne6x_reg_valid_table(pf, table))
		return -EINVAL;

	msg = kzalloc(1028, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = (u32)(TABLE_OPCODE_DELETE(table));
	msg->hdr.bits.data_len = 12 + size;
	msg->data[0] = TABLE_ADDR(table);
	msg->data[1] = TABLE_XMEM(table) + size;
	memcpy(&msg->data[2], data, size);

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 3 + (size >> 2), 0);
	kfree(msg);

	return status;
}

int ne6x_reg_table_search(struct ne6x_pf *pf, enum ne6x_reg_table table,
			  u32 *data, int size, u32 *ret_data, int ret_size)
{
	struct axia_mbus_msg *msg;
	int status;

	if (TABLE_ADVCMD_VALID(table) == 0x0)
		return -EINVAL;

	if (size % TABLE_SIZE_LOOKUP(table) != 0x00)
		return -EINVAL;

	if (!ne6x_reg_valid_table(pf, table))
		return -EINVAL;

	msg = kzalloc(1036, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = (u32)(TABLE_OPCODE_LOOKUP(table));
	msg->hdr.bits.data_len = 12 + size;
	msg->data[0] = TABLE_ADDR(table);
	msg->data[1] = TABLE_XMEM(table) + TABLE_SIZE_LOOKUP_RET(table);
	memcpy((void *)&msg->data[2], (void *)data, size);

	status = ne6x_reg_perform(pf, (u32 *)msg, ret_data, 3 + (size >> 2), ret_size / 4);
	kfree(msg);

	return (status != 0) ? -ENOENT : status;
}

int ne6x_reg_table_update(struct ne6x_pf *pf, enum ne6x_reg_table table,
			  u32 index, u32 *data, int size)
{
	struct axia_mbus_msg *msg;
	int status;

	if (TABLE_ADVCMD_VALID(table) == 0x0)
		return -EINVAL;

	if (size % TABLE_SIZE_UPDATE(table) != 0x00)
		return -EINVAL;

	if (!ne6x_reg_valid_table(pf, table))
		return -EINVAL;

	msg = kzalloc(1036, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = (u32)(TABLE_OPCODE_UPDATE(table));
	msg->hdr.bits.data_len = 16 + size;
	msg->data[0] = TABLE_ADDR(table);
	msg->data[1] = index;
	msg->data[2] = TABLE_SIZE_UPDATE(table);
	memcpy((void *)&msg->data[3], (void *)data, size);

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 4 + (size >> 2), 0);
	kfree(msg);

	return (status != 0) ? -ENOENT : status;
}

int ne6x_reg_talk_port(struct ne6x_pf *pf, enum ne6x_reg_talk_port talk,
		       enum ne6x_reg_talk_opcode opcode,
		       int port, void *pbuf, int size)
{
	struct axia_mbus_msg *msg;
	int status;

	if (((size % 4) != 0) || size > 512)
		return -EINVAL;

	msg = kzalloc(520, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = (AXIA_MBUS_TALK_PORT_BASE + 4 * talk + 2 * opcode);
	msg->hdr.bits.data_len = 8 + size;
	msg->data[0] = port;
	if (pbuf)
		memcpy(&msg->data[1], pbuf, size);

	status = ne6x_reg_perform(pf, (u32 *)msg, (opcode == NE6X_TALK_GET) ? pbuf : NULL,
				  2 + ((opcode == NE6X_TALK_GET) ? 0 : (size >> 2)),
				  (opcode == NE6X_TALK_GET) ? (size >> 2) : 0);
	kfree(msg);

	return status;
}

int ne6x_reg_reset_firmware(struct ne6x_pf *pf)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(32, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_RESET_FIRMWARE_COMMAND;
	msg->hdr.bits.data_len = 4;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 1, 0);
	kfree(msg);

	return status;
}

int ne6x_reg_e2prom_read(struct ne6x_pf *pf, u32 offset, void *pbuf, int size)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(1040, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	if (size > 2048)
		size = 2048;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_E2PROM_READ_COMMAND;
	msg->hdr.bits.data_len = 12;
	msg->data[0] = offset;
	msg->data[1] = size;

	status = ne6x_reg_perform(pf, (u32 *)msg, (u32 *)pbuf, 3, size / 4);
	kfree(msg);

	return status;
}

int ne6x_reg_e2prom_write(struct ne6x_pf *pf, u32 offset, void *pbuf, int size)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(1040, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	if (size > 1024)
		size = 1024;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_E2PROM_WRITE_COMMAND;
	msg->hdr.bits.data_len = 12 + (size / 4) * 4;
	msg->data[0] = (offset);
	msg->data[1] = (size);
	memcpy((void *)&msg->data[1], (void *)pbuf, (ssize_t)size);

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 3 + (size / 4), 0);
	kfree(msg);

	return status;
}

int ne6x_reg_get_fan_speed(struct ne6x_pf *pf, u32 *speed)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(32, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_GET_FAN_SPEED_COMMAND;
	msg->hdr.bits.data_len = 4;

	status = ne6x_reg_perform(pf, (u32 *)msg, (u32 *)speed, 1, 1);
	kfree(msg);

	return status;
}

int ne6x_reg_set_fan_speed(struct ne6x_pf *pf, u32 speed)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(32, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_SET_FAN_SPEED_COMMAND;
	msg->hdr.bits.data_len = 8;
	msg->data[0] = speed;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 2, 0);
	kfree(msg);

	return status;
}

int ne6x_reg_get_soc_info(struct ne6x_pf *pf, u32 class_type, u32 *ret, u32 size)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(32, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_GET_SYSTEM_INFO_COMMAND;
	msg->hdr.bits.data_len = 12;
	msg->data[0] = class_type;
	msg->data[1] = size;

	status = ne6x_reg_perform(pf, (u32 *)msg, (u32 *)ret, 3, size >> 2);
	kfree(msg);

	return status;
}

int ne6x_reg_send_bit(struct ne6x_pf *pf, u32 port, u32 mode)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(32, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_GET_SYSTEM_INFO_COMMAND;
	msg->hdr.bits.data_len = 16;
	msg->data[0] = 4;
	msg->data[1] = port;
	msg->data[2] = mode;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 4, 0);
	kfree(msg);

	return status;
}

int ne6x_reg_mem_read(struct ne6x_pf *pf, u32 addr, void *pbuf, u32 size)
{
	struct axia_mbus_msg *msg;
	int status;

	if (size > 1024)
		size = 1024;

	msg = kzalloc(520, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_READ_MEMORY_COMMAND;
	msg->hdr.bits.data_len = 12;
	msg->data[0] = addr;
	msg->data[1] = size;

	status = ne6x_reg_perform(pf, (u32 *)msg, (u32 *)pbuf, 3, size / 4);
	kfree(msg);

	return status;
}

int ne6x_reg_mem_write(struct ne6x_pf *pf, u32 addr, void *pbuf, u32 size)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(520, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	if (size > 1024)
		size = 1024;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_WRITE_MEMORY_COMMAND;
	msg->hdr.bits.data_len = 12 + (size / 4) * 4;
	msg->data[0] = addr;
	msg->data[1] = size;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 3 + (size / 4), 0);
	kfree(msg);

	return status;
}

#define NE6X_FW_MAX_FRG_SIZE (4 * 1024)
int ne6x_reg_upgrade_firmware(struct ne6x_pf *pf, u8 region, u8 *data, int size)
{
	struct axia_mbus_msg *msg;
	int offset = 0, left_size = 0, frag_size = 0;
	int status = 0;

	msg = kzalloc(NE6X_FW_MAX_FRG_SIZE + 16, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	ne6x_reg_lock(pf);
	/* scile begin */
	NE6X_ACCESS_TIMEOUT = 100000;
	left_size = size;
	while (left_size) {
		frag_size = (left_size >= NE6X_FW_MAX_FRG_SIZE) ? NE6X_FW_MAX_FRG_SIZE : left_size;

		msg->hdr.uint = 0;
		msg->hdr.bits.opcode = AXIA_MBUS_UPGRADE_COMMAND;
		msg->hdr.bits.data_len = 12 + frag_size;
		msg->data[0] = region;	 /* region */
		msg->data[1] = frag_size; /* size */
		memcpy(&msg->data[2], data + offset, frag_size);

		status |= _ne6x_reg_perform(pf, (u32 *)msg, NULL, 3 + (frag_size >> 2), 0);
		if (status)
			goto err_upgrade;

		left_size -= frag_size;
		offset += frag_size;
	}

err_upgrade:
	/* scile end */
	NE6X_ACCESS_TIMEOUT = 999;
	ne6x_reg_unlock(pf);
	kfree(msg);

	return status;
}

int ne6x_reg_get_ver(struct ne6x_pf *pf, struct ne6x_firmware_ver_info *version)
{
	struct axia_mbus_msg *msg;
	u32 *out_buffer = (u32 *)version;
	int status;

	msg = kzalloc(40, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_GET_VER_COMMAND;
	msg->hdr.bits.data_len = 4;

	status = ne6x_reg_perform(pf, (u32 *)msg, out_buffer, 1,
				  sizeof(struct ne6x_firmware_ver_info) / sizeof(u32));
	kfree(msg);

	return status;
}

int ne6x_reg_get_sfp_eeprom(struct ne6x_pf *pf, int port, void *pbuf, u32 offset, int size)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(1040, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	if (size > 2048)
		size = 2048;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_TALK_GET_PORT_SFP_EEPROM_COMMAND;
	msg->hdr.bits.data_len = 16;
	msg->data[0] = port;
	msg->data[1] = offset;
	msg->data[2] = size;

	status = ne6x_reg_perform(pf, (u32 *)msg, (u32 *)pbuf, 4, size / 4);
	kfree(msg);

	return status;
}

int ne6x_reg_nic_start(struct ne6x_pf *pf, u32 flag)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(32, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_SET_NIC_START_COMMAND;
	msg->hdr.bits.data_len = 8;
	msg->data[0] = flag;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 2, 0);
	kfree(msg);

	return status;
}

int ne6x_reg_nic_stop(struct ne6x_pf *pf, u32 flag)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(32, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_SET_NIC_STOP_COMMAND;
	msg->hdr.bits.data_len = 8;
	msg->data[0] = flag;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 2, 0);
	kfree(msg);

	return status;
}

int ne6x_reg_get_nic_state(struct ne6x_pf *pf, u32 *state)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(32, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_GET_NIC_STATE_COMMAND;
	msg->hdr.bits.data_len = 4;

	status = ne6x_reg_perform(pf, (u32 *)msg, (u32 *)state, 1, 1);
	kfree(msg);

	return status;
}

int ne6x_reg_set_user_data_template(struct ne6x_pf *pf, enum np_user_data type, u32 data)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(32, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_SET_NP_USERDATA_COMMAND;
	msg->hdr.bits.data_len = 12;
	msg->data[0] = type;
	msg->data[1] = data;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 3, 0);
	kfree(msg);

	return status;
}

int ne6x_reg_get_user_data_template(struct ne6x_pf *pf, enum np_user_data type, u32 *data)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(32, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_GET_NP_USERDATA_COMMAND;
	msg->hdr.bits.data_len = 4;
	msg->data[0] = type;

	status = ne6x_reg_perform(pf, (u32 *)msg, data, 2, 1);
	kfree(msg);

	return status;
}

int ne6x_reg_set_user_data(struct ne6x_pf *pf, enum np_user_data type, u32 data)
{
	return ne6x_reg_set_user_data_template(pf, type, data);
}

int ne6x_reg_get_user_data(struct ne6x_pf *pf, enum np_user_data type, u32 *data)
{
	int status = 0;

	status = ne6x_reg_get_user_data_template(pf, type, data);

	return status;
}

int ne6x_reg_set_led(struct ne6x_pf *pf, int port, bool state)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(32, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_SET_LED_STATE_COMMAND;
	msg->hdr.bits.data_len = 12;
	msg->data[0] = port;
	msg->data[1] = state;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 3, 0);
	kfree(msg);

	return status;
}

int ne6x_reg_config_meter(struct ne6x_pf *pf, u32 meter_id, u32 *data, int size)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(520, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_CONFIG_METER_COMMAND;
	msg->hdr.bits.data_len = size + 8;
	msg->data[0] = meter_id;
	memcpy((void *)&msg->data[1], (void *)data, size);

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 2 + (size / 4), 0);
	kfree(msg);

	return status;
}

int ne6x_reg_set_unicast_for_fastmode(struct ne6x_pf *pf, u32 index, u32 *data,
				      u32 size)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(40, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_SET_FAST_L2FDB_COMMAND;
	msg->hdr.bits.data_len = size + 8;
	msg->data[0] = index;
	memcpy((void *)&msg->data[1], (void *)data, size);

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 2 + (size / 4), 0);
	kfree(msg);

	return status;
}

int ne6x_reg_get_dump_data_len(struct ne6x_pf *pf, u32 *size)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(40, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_GET_DUMP_DATA_LEN_COMMAND;
	msg->hdr.bits.data_len = 4;

	status = ne6x_reg_perform(pf, (u32 *)msg, size, 1, 1);
	kfree(msg);

	return status;
}

void ne6x_reg_send(struct ne6x_pf *pf, u32 cmd, u32 *data, u32 size)
{
	struct axia_mbus_msg *msg;
	u32 *msg_data;
	int index;

	msg = kzalloc(size + 12, GFP_KERNEL);
	if (!msg)
		return;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = cmd;
	msg->hdr.bits.data_len = 4 + size;
	memcpy((void *)&msg->data[0], (void *)data, size);

	msg_data = (u32 *)msg;
	/* Write Command(s) */
	for (index = 0; index < ((size / 4) + 1); index++)
		_reg_apb_write(pf, PCIE2C810_SHM_MBUS_BASE + 4 * index, msg_data[index]);

	/* Start mbus mechanism, notice c810 */
	_reg_apb_write(pf, 0x20680014, 0x3FEC);
	usleep_range(1000, 1200);
	kfree(msg);
}

int ne6x_reg_polling(struct ne6x_pf *pf, u32 cmd, u32 *data, u32 buf_size,
		     u32 *real_size)
{
	int timeout = 0, offset = 0;
	struct axia_mbus_msg resp;
	int index, status;

	memset(&resp, 0, sizeof(resp));

	/* check if c810 handle completed */
	while (timeout < NE6X_ACCESS_TIMEOUT) {
		resp.hdr.uint = _reg_apb_read(pf, PCIE2C810_SHM_MBUS_BASE);
		if (resp.hdr.bits.opcode == cmd)
			break;

		timeout++;
		usleep_range(200, 220);
	}

	status = (timeout >= NE6X_ACCESS_TIMEOUT) ? -ETIMEDOUT : 0;
	status = (resp.hdr.bits.e == 1) ? -EAGAIN : status;
	if (status) {
		dev_info(ne6x_pf_to_dev(pf), "%s: cmd %d status (%d)\n", __func__, cmd, status);
		return status;
	}

	switch (cmd) {
	case AXIA_MBUS_GET_DUMP_DATA_ACK:
		*real_size = resp.hdr.bits.data_len - sizeof(resp) - sizeof(u32);
		offset = sizeof(u32);
		pf->dump_info = _reg_apb_read(pf, PCIE2C810_SHM_DATA_BASE);
		break;
	default:
		*real_size = resp.hdr.bits.data_len - sizeof(resp);
		offset = 0;
		break;
	}

	if (*real_size > buf_size)
		*real_size = buf_size;

	for (index = 0; index < (*real_size) / 4; index++)
		data[index] = _reg_apb_read(pf, PCIE2C810_SHM_DATA_BASE + 4 * index + offset);

	return 0;
}

int ne6x_reg_get_dump_data(struct ne6x_pf *pf, u32 *data, u32 size)
{
	u32 *temp_buff = data;
	u32 left_size = size;
	u32 real_size = 0;

	memset(&pf->dump_info, 0, sizeof(u32));

	ne6x_reg_lock(pf);
	while (left_size > 0) {
		temp_buff += real_size / 4;
		ne6x_reg_send(pf, AXIA_MBUS_GET_DUMP_DATA_COMMAND, (u32 *)&pf->dump_info, 4);
		if (ne6x_reg_polling(pf, AXIA_MBUS_GET_DUMP_DATA_ACK,
				     temp_buff, left_size, &real_size)) {
			ne6x_reg_unlock(pf);
			return -EAGAIN;
		}

		left_size -= real_size;
	}
	ne6x_reg_unlock(pf);

	return 0;
}

int ne6x_reg_clear_table(struct ne6x_pf *pf, u32 table_id)
{
	struct axia_mbus_msg *msg;
	int status;

	if (!ne6x_reg_valid_table(pf, table_id))
		return -EINVAL;

	msg = kzalloc(40, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	NE6X_ACCESS_TIMEOUT = 99999;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_CLR_TABLE_COMMAND;
	msg->hdr.bits.data_len = 8;
	msg->data[0] = table_id;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 2, 0);
	kfree(msg);

	NE6X_ACCESS_TIMEOUT = 9999;

	return status;
}

int ne6x_reg_set_norflash_write_protect(struct ne6x_pf *pf, u32 write_protect)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(40, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_SET_NOFLASH_WRITE_PROTECT_COMMAND;
	msg->hdr.bits.data_len = 8;
	msg->data[0] = write_protect;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 2, 0);
	kfree(msg);

	return status;
}

int ne6x_reg_get_norflash_write_protect(struct ne6x_pf *pf, u32 *p_write_protect)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(512, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_GET_NOFLASH_WRITE_PROTECT_COMMAND;
	msg->hdr.bits.data_len = 4;

	status = ne6x_reg_perform(pf, (u32 *)msg, p_write_protect, 1, 1);
	kfree(msg);

	return status;
}

int ne6x_reg_write_norflash(struct ne6x_pf *pf, u32 offset, u32 length, u32 *pdata)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(512, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_OPT_NOFLASH_COMMAND;
	msg->hdr.bits.data_len = 16 + length;
	msg->data[0] = NE6X_NORFLASH_OP_WRITE_E;
	msg->data[1] = offset;
	msg->data[2] = length;
	memcpy((void *)&msg->data[3], (void *)pdata, length);

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 4 + (length >> 2), 0);
	kfree(msg);

	return status;
}

int ne6x_reg_erase_norflash(struct ne6x_pf *pf, u32 offset, u32 length)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(40, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_OPT_NOFLASH_COMMAND;
	msg->hdr.bits.data_len = 16;
	msg->data[0] = NE6X_NORFLASH_OP_ERASE_E;
	msg->data[1] = offset;
	msg->data[2] = length;

	status = ne6x_reg_perform(pf, (u32 *)msg, NULL, 4, 0);
	kfree(msg);

	return status;
}

int ne6x_reg_read_norflash(struct ne6x_pf *pf, u32 offset, u32 length, u32 *p)
{
	struct axia_mbus_msg *msg;
	int status;

	msg = kzalloc(40, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	msg->hdr.uint = 0;
	msg->hdr.bits.opcode = AXIA_MBUS_OPT_NOFLASH_COMMAND;
	msg->hdr.bits.data_len = 16;
	msg->data[0] = NE6X_NORFLASH_OP_READ_E;
	msg->data[1] = offset;
	msg->data[2] = length;

	status = ne6x_reg_perform(pf, (u32 *)msg, p, 4, length >> 2);
	kfree(msg);

	return status;
}

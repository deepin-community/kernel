/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_CFG_MAG_DEFINE_H
#define SSS_NIC_CFG_MAG_DEFINE_H

#include <linux/types.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>

#include "sss_hw_mbx_msg.h"

/* *
 * Definition of the NIC receiving mode
 */
#define SSSNIC_RX_MODE_UC          0x01
#define SSSNIC_RX_MODE_MC          0x02
#define SSSNIC_RX_MODE_BC          0x04
#define SSSNIC_RX_MODE_MC_ALL      0x08
#define SSSNIC_RX_MODE_PROMISC     0x10

#define SSSNIC_RX_RATE_LOW 200000
#define SSSNIC_RX_COAL_TIME_LOW 25
#define SSSNIC_RX_PENDING_LIMIT_LOW 2

#define SSSNIC_RX_RATE_HIGH 700000
#define SSSNIC_RX_COAL_TIME_HIGH 225
#define SSSNIC_RX_PENDING_LIMIT_HIGH 8

#define SSSNIC_RX_RATE_THRESH 50000
#define SSSNIC_TX_RATE_THRESH 50000
#define SSSNIC_RX_RATE_LOW_VM 100000
#define SSSNIC_RX_PENDING_LIMIT_HIGH_VM 87

#define SSSNIC_MAX_LIMIT_BW 100

#define SSSNIC_MAG_OPCODE_PORT_DISABLE 0x0
#define SSSNIC_MAG_OPCODE_TX_ENABLE 0x1
#define SSSNIC_MAG_OPCODE_RX_ENABLE 0x2

#define SSSNIC_XSFP_INFO_MAX_SIZE 640

#define SSNSIC_PORT_PRESENT	0
#define SSNSIC_PORT_ABSENT	1

enum sss_nic_valid_link_settings {
	SSSNIC_LINK_SET_SPEED = 0x1,
	SSSNIC_LINK_SET_AUTONEG = 0x2,
	SSSNIC_LINK_SET_FEC = 0x4,
};

enum sss_nic_link_follow_status {
	SSSNIC_LINK_FOLLOW_DEFAULT,
	SSSNIC_LINK_FOLLOW_PORT,
	SSSNIC_LINK_FOLLOW_SEPARATE,
	SSSNIC_LINK_FOLLOW_STATUS_MAX,
};

/* serdes/mag message cmd define */
enum sss_nic_mag_opcode {
	SSSNIC_MAG_OPCODE_SERDES_PROCESS = 0,

	/* port configure, 0-29 */
	SSSNIC_MAG_OPCODE_SET_PORT_CFG = 1,
	SSSNIC_MAG_OPCODE_SET_PORT_ADAPT = 2,
	SSSNIC_MAG_OPCODE_CFG_LOOPBACK_MODE = 3,

	SSSNIC_MAG_OPCODE_GET_PORT_ENABLE = 5,
	SSSNIC_MAG_OPCODE_SET_PORT_ENABLE = 6,
	SSSNIC_MAG_OPCODE_LINK_STATUS = 7,
	SSSNIC_MAG_OPCODE_SET_LINK_FOLLOW = 8,
	SSSNIC_MAG_OPCODE_SET_PMA_ENABLE = 9,
	SSSNIC_MAG_OPCODE_CFG_FEC_MODE = 10,

	SSSNIC_MAG_OPCODE_CFG_AN_TYPE = 12, /* reserved for future use */
	SSSNIC_MAG_OPCODE_CFG_LINK_TIME = 13,

	/* bios link, 30-49 */
	SSSNIC_MAG_OPCODE_CFG_BIOS_LINK_CFG = 31,
	SSSNIC_MAG_OPCODE_RESTORE_LINK_CFG = 32,
	SSSNIC_MAG_OPCODE_ACTIVATE_BIOS_LINK_CFG = 33,

	/* LED */
	SSSNIC_MAG_OPCODE_SET_LED_CFG = 50,

	/* PHY */
	SSSNIC_MAG_OPCODE_GET_PHY_INIT_STATUS = 55, /* reserved for future use */

	/* sfp */
	SSSNIC_MAG_OPCODE_GET_XSFP_INFO = 60,
	SSSNIC_MAG_OPCODE_SET_XSFP_ENABLE = 61,
	SSSNIC_MAG_OPCODE_GET_XSFP_PRESENT = 62,
	/* sfp/qsfp single byte read/write, for equipment test */
	SSSNIC_MAG_OPCODE_SET_XSFP_RW = 63,
	SSSNIC_MAG_OPCODE_CFG_XSFP_TEMPERATURE = 64,

	/* event 100-149 */
	SSSNIC_MAG_OPCODE_WIRE_EVENT = 100,
	SSSNIC_MAG_OPCODE_LINK_ERR_EVENT = 101,

	/* DFX、Counter */
	SSSNIC_MAG_OPCODE_EVENT_PORT_INFO = 150,
	SSSNIC_MAG_OPCODE_GET_PORT_STAT = 151,
	SSSNIC_MAG_OPCODE_CLR_PORT_STAT = 152,
	SSSNIC_MAG_OPCODE_GET_PORT_INFO = 153,
	SSSNIC_MAG_OPCODE_GET_PCS_ERR_CNT = 154,
	SSSNIC_MAG_OPCODE_GET_MAG_CNT = 155,
	SSSNIC_MAG_OPCODE_DUMP_ANTRAIN_INFO = 156,

	SSSNIC_MAG_OPCODE_MAX = 0xFF
};

enum sss_nic_mag_opcode_port_speed {
	SSSNIC_PORT_SPEED_NOT_SET = 0,
	SSSNIC_PORT_SPEED_10MB = 1,
	SSSNIC_PORT_SPEED_100MB = 2,
	SSSNIC_PORT_SPEED_1GB = 3,
	SSSNIC_PORT_SPEED_10GB = 4,
	SSSNIC_PORT_SPEED_25GB = 5,
	SSSNIC_PORT_SPEED_40GB = 6,
	SSSNIC_PORT_SPEED_50GB = 7,
	SSSNIC_PORT_SPEED_100GB = 8,
	SSSNIC_PORT_SPEED_200GB = 9,
	SSSNIC_PORT_SPEED_UNKNOWN
};

enum sss_nic_mag_opcode_port_an {
	SSSNIC_PORT_AN_NOT_SET = 0,
	SSSNIC_PORT_CFG_AN_ON = 1,
	SSSNIC_PORT_CFG_AN_OFF = 2
};

/* mag supported/advertised link mode bitmap */
enum mag_cmd_link_mode {
	SSSNIC_LINK_MODE_GE = 0,
	SSSNIC_LINK_MODE_10GE_BASE_R = 1,
	SSSNIC_LINK_MODE_25GE_BASE_R = 2,
	SSSNIC_LINK_MODE_40GE_BASE_R4 = 3,
	SSSNIC_LINK_MODE_50GE_BASE_R = 4,
	SSSNIC_LINK_MODE_50GE_BASE_R2 = 5,
	SSSNIC_LINK_MODE_100GE_BASE_R = 6,
	SSSNIC_LINK_MODE_100GE_BASE_R2 = 7,
	SSSNIC_LINK_MODE_100GE_BASE_R4 = 8,
	SSSNIC_LINK_MODE_200GE_BASE_R2 = 9,
	SSSNIC_LINK_MODE_200GE_BASE_R4 = 10,
	SSSNIC_LINK_MODE_MAX_NUMBERS,

	SSSNIC_LINK_MODE_UNKNOWN = 0xFFFF
};

/* led type */
enum sss_nic_mag_led_type {
	SSSNIC_MAG_LED_TYPE_ALARM = 0x0,
	SSSNIC_MAG_LED_TYPE_LOW_SPEED = 0x1,
	SSSNIC_MAG_LED_TYPE_HIGH_SPEED = 0x2
};

/* led mode */
enum sss_nic_mag_led_mode {
	SSSNIC_MAG_LED_DEFAULT = 0x0,
	SSSNIC_MAG_LED_FORCE_ON = 0x1,
	SSSNIC_MAG_LED_FORCE_OFF = 0x2,
	SSSNIC_MAG_LED_FORCE_BLINK_1HZ = 0x3,
	SSSNIC_MAG_LED_FORCE_BLINK_2HZ = 0x4,
	SSSNIC_MAG_LED_FORCE_BLINK_4HZ = 0x5,
	SSSNIC_MAG_LED_1HZ = 0x6,
	SSSNIC_MAG_LED_2HZ = 0x7,
	SSSNIC_MAG_LED_4HZ = 0x8
};

/* xsfp wire type, refer to cmis protocol definition */
enum sss_nic_mag_wire_type {
	SSSNIC_MAG_WIRE_TYPE_UNKNOWN = 0x0,
	SSSNIC_MAG_WIRE_TYPE_MM = 0x1,
	SSSNIC_MAG_WIRE_TYPE_SM = 0x2,
	SSSNIC_MAG_WIRE_TYPE_COPPER = 0x3,
	SSSNIC_MAG_WIRE_TYPE_ACC = 0x4,
	SSSNIC_MAG_WIRE_TYPE_BASET = 0x5,
	SSSNIC_MAG_WIRE_TYPE_AOC = 0x40,
	SSSNIC_MAG_WIRE_TYPE_ELECTRIC = 0x41,
	SSSNIC_MAG_WIRE_TYPE_BACKPLANE = 0x42
};

enum sss_nic_link_status {
	SSSNIC_LINK_DOWN = 0,
	SSSNIC_LINK_UP
};

struct sss_nic_link_ksettings {
	u32 valid_bitmap;
	u8 speed;   /* enum nic_speed_level */
	u8 autoneg; /* 0 - off; 1 - on */
	u8 fec;	    /* 0 - RSFEC; 1 - BASEFEC; 2 - NOFEC */
};

struct sss_nic_port_info {
	u8 port_type;
	u8 autoneg_cap;
	u8 autoneg_state;
	u8 duplex;
	u8 speed;
	u8 fec;
	u32 supported_mode;
	u32 advertised_mode;
};

struct sss_nic_pause_cfg {
	u8 auto_neg;
	u8 rx_pause;
	u8 tx_pause;
};

struct sss_nic_mbx_mag_set_port_cfg {
	struct sss_mgmt_msg_head head;

	u8 port_id;
	u8 rsvd0[3];

	u32 config_bitmap;
	u8 speed;
	u8 autoneg;
	u8 fec;
	u8 lanes;
	u8 rsvd1[20];
};

struct sss_nic_mbx_get_port_info {
	struct sss_mgmt_msg_head head;

	u8 port_id;
	u8 rsvd0[3];

	u8 wire_type;
	u8 an_support;
	u8 an_en;
	u8 duplex;

	u8 speed;
	u8 fec;
	u8 lanes;
	u8 rsvd1;

	u32 supported_mode;
	u32 advertised_mode;
	u8 rsvd2[8];
};

struct sss_nic_mbx_loopback_mode {
	struct sss_mgmt_msg_head head;

	u8 port_id;
	u8 opcode; /* 0:get loopback mode  1:set loopback mode */
	u8 mode;
	u8 en; /* 0:disable  1:enable */

	u32 rsvd0[2];
};

struct sss_nic_mbx_set_port_mag_state {
	struct sss_mgmt_msg_head head;

	u16 function_id; /* function_id should not more than the max support pf_id(32) */
	u16 rsvd0;

	u8 state; /* bitmap bit0:tx_en bit1:rx_en */
	u8 rsvd1[3];
};

/* the physical port disable link follow only when all pf of the port are set to follow disable */
struct sss_nic_mbx_set_link_follow {
	struct sss_mgmt_msg_head head;

	u16 function_id; /* function_id should not more than the max support pf_id(32) */
	u16 rsvd0;

	u8 follow;
	u8 rsvd1[3];
};

/* firmware also use this cmd report link event to driver */
struct sss_nic_mbx_get_link_state {
	struct sss_mgmt_msg_head head;

	u8 port_id;
	u8 status; /* 0:link down  1:link up */
	u8 rsvd0[2];
};

/* the led is report alarm  when any pf of the port is alram */
struct sss_nic_mbx_set_led_cfg {
	struct sss_mgmt_msg_head head;

	u16 function_id;
	u8 type;
	u8 mode;
};

struct sss_nic_mbx_get_xsfp_info {
	struct sss_mgmt_msg_head head;

	u8 port_id;
	u8 wire_type;
	u16 out_len;
	u32 rsvd;
	u8 sfp_info[SSSNIC_XSFP_INFO_MAX_SIZE];
};

struct sss_nic_mbx_get_xsfp_present {
	struct sss_mgmt_msg_head head;

	u8 port_id;
	u8 abs_status; /* 0:present, 1:absent */
	u8 rsvd[2];
};

struct sss_nic_cache_port_sfp {
	u8 mpu_send_sfp_info;
	u8 mpu_send_sfp_abs;
	u8 rsvd[2];
	struct sss_nic_mbx_get_xsfp_info std_sfp_info;
	struct sss_nic_mbx_get_xsfp_present abs;
};

/* xsfp plug event */
struct sss_nic_mag_wire_event {
	struct sss_mgmt_msg_head head;

	u8 port_id;
	u8 status; /* 0:present, 1:absent */
	u8 rsvd[2];
};

struct sss_nic_mag_port_stats {
	u64 tx_fragment_pkts;
	u64 tx_undersize_pkts;
	u64 tx_undermin_pkts;
	u64 tx_64_oct_pkts;
	u64 tx_65_127_oct_pkts;
	u64 tx_128_255_oct_pkts;
	u64 tx_256_511_oct_pkts;
	u64 tx_512_1023_oct_pkts;
	u64 tx_1024_1518_oct_pkts;
	u64 tx_1519_2047_oct_pkts;
	u64 tx_2048_4095_oct_pkts;
	u64 tx_4096_8191_oct_pkts;
	u64 tx_8192_9216_oct_pkts;
	u64 tx_9217_12287_oct_pkts;
	u64 tx_12288_16383_oct_pkts;
	u64 tx_1519_max_bad_pkts;
	u64 tx_1519_max_good_pkts;
	u64 tx_oversize_pkts;
	u64 tx_jabber_pkts;
	u64 tx_bad_pkts;
	u64 tx_bad_octs;
	u64 tx_good_pkts;
	u64 tx_good_octs;
	u64 tx_total_pkts;
	u64 tx_total_octs;
	u64 tx_uni_pkts;
	u64 tx_multi_pkts;
	u64 tx_broad_pkts;
	u64 tx_pauses;
	u64 tx_pfc_pkts;
	u64 tx_pfc_pri0_pkts;
	u64 tx_pfc_pri1_pkts;
	u64 tx_pfc_pri2_pkts;
	u64 tx_pfc_pri3_pkts;
	u64 tx_pfc_pri4_pkts;
	u64 tx_pfc_pri5_pkts;
	u64 tx_pfc_pri6_pkts;
	u64 tx_pfc_pri7_pkts;
	u64 tx_control_pkts;
	u64 tx_err_all_pkts;
	u64 tx_from_app_good_pkts;
	u64 tx_from_app_bad_pkts;

	u64 rx_fragment_pkts;
	u64 rx_undersize_pkts;
	u64 rx_undermin_pkts;
	u64 rx_64_oct_pkts;
	u64 rx_65_127_oct_pkts;
	u64 rx_128_255_oct_pkts;
	u64 rx_256_511_oct_pkts;
	u64 rx_512_1023_oct_pkts;
	u64 rx_1024_1518_oct_pkts;
	u64 rx_1519_2047_oct_pkts;
	u64 rx_2048_4095_oct_pkts;
	u64 rx_4096_8191_oct_pkts;
	u64 rx_8192_9216_oct_pkts;
	u64 rx_9217_12287_oct_pkts;
	u64 rx_12288_16383_oct_pkts;
	u64 rx_1519_max_bad_pkts;
	u64 rx_1519_max_good_pkts;
	u64 rx_oversize_pkts;
	u64 rx_jabber_pkts;
	u64 rx_bad_pkts;
	u64 rx_bad_octs;
	u64 rx_good_pkts;
	u64 rx_good_octs;
	u64 rx_total_pkts;
	u64 rx_total_octs;
	u64 rx_uni_pkts;
	u64 rx_multi_pkts;
	u64 rx_broad_pkts;
	u64 rx_pauses;
	u64 rx_pfc_pkts;
	u64 rx_pfc_pri0_pkts;
	u64 rx_pfc_pri1_pkts;
	u64 rx_pfc_pri2_pkts;
	u64 rx_pfc_pri3_pkts;
	u64 rx_pfc_pri4_pkts;
	u64 rx_pfc_pri5_pkts;
	u64 rx_pfc_pri6_pkts;
	u64 rx_pfc_pri7_pkts;
	u64 rx_control_pkts;
	u64 rx_sym_err_pkts;
	u64 rx_fcs_err_pkts;
	u64 rx_send_app_good_pkts;
	u64 rx_send_app_bad_pkts;
	u64 rx_unfilter_pkts;
};

struct sss_nic_mbx_mag_port_stats_info {
	struct sss_mgmt_msg_head head;

	u8 port_id;
	u8 rsvd0[3];
};

struct sss_nic_mbx_mag_port_stats {
	struct sss_mgmt_msg_head head;

	struct sss_nic_mag_port_stats counter;
	u64 rsvd1[15];
};

struct sss_nic_mag_cfg {
	struct semaphore	cfg_lock;

	/* Valid when pfc is disable */
	u8			pause_set;
	u8			rsvd1[3];
	struct sss_nic_pause_cfg	nic_pause;

	u8			pfc_en;
	u8			pfc_bitmap;
	u8			rsvd2[2];

	struct sss_nic_port_info	port_info;

	/* percentage of pf link bandwidth */
	u32			pf_bw_limit;

	struct sss_nic_cache_port_sfp rt_cmd;
	struct mutex sfp_mutex; /* mutex used for copy sfp info */
};

#define SSSNIC_PF_LIMIT_BW_MAX 100

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2024 Huawei Technologies Co., Ltd */

#ifndef MAG_MPU_CMD_DEFS_H
#define MAG_MPU_CMD_DEFS_H

#include "mpu_cmd_base_defs.h"

/* serdes cmd struct define */
#define CMD_ARRAY_BUF_SIZE 64
#define SERDES_CMD_DATA_BUF_SIZE 512
#define RATE_MBPS_TO_GBPS 1000
struct serdes_in_info {
	u32 chip_id : 16;
	u32 macro_id : 16;
	u32 start_sds_id : 16;
	u32 sds_num : 16;

	u32 cmd_type : 8; /* reserved for iotype */
	u32 sub_cmd : 8;
	u32 rw : 1; /* 0: read, 1: write */
	u32 rsvd : 15;

	u32 val;
	union {
		char field[CMD_ARRAY_BUF_SIZE];
		u32 addr;
		u8 *ex_param;
	};
};

struct serdes_out_info {
	u32 str_len; /* out_str length */
	u32 result_offset;
	u32 type; /* 0:data; 1:string */
	char out_str[SERDES_CMD_DATA_BUF_SIZE];
};

struct serdes_cmd_in {
	struct mgmt_msg_head head;

	struct serdes_in_info serdes_in;
};

struct serdes_cmd_out {
	struct mgmt_msg_head head;

	struct serdes_out_info serdes_out;
};

enum mag_cmd_port_speed {
	PORT_SPEED_NOT_SET = 0,
	PORT_SPEED_10MB = 1,
	PORT_SPEED_100MB = 2,
	PORT_SPEED_1GB = 3,
	PORT_SPEED_10GB = 4,
	PORT_SPEED_25GB = 5,
	PORT_SPEED_40GB = 6,
	PORT_SPEED_50GB = 7,
	PORT_SPEED_100GB = 8,
	PORT_SPEED_200GB = 9,
	PORT_SPEED_UNKNOWN
};

enum mag_cmd_port_an {
	PORT_AN_NOT_SET = 0,
	PORT_CFG_AN_ON = 1,
	PORT_CFG_AN_OFF = 2
};

enum mag_cmd_port_adapt {
	PORT_ADAPT_NOT_SET = 0,
	PORT_CFG_ADAPT_ON = 1,
	PORT_CFG_ADAPT_OFF = 2
};

enum mag_cmd_port_sriov {
	PORT_SRIOV_NOT_SET = 0,
	PORT_CFG_SRIOV_ON = 1,
	PORT_CFG_SRIOV_OFF = 2
};

enum mag_cmd_port_fec {
	PORT_FEC_NOT_SET = 0,
	PORT_FEC_RSFEC = 1,
	PORT_FEC_BASEFEC = 2,
	PORT_FEC_NOFEC = 3,
	PORT_FEC_LLRSFEC = 4,
	PORT_FEC_AUTO = 5
};

enum mag_cmd_port_lanes {
	PORT_LANES_NOT_SET = 0,
	PORT_LANES_X1 = 1,
	PORT_LANES_X2 = 2,
	PORT_LANES_X4 = 4,
	PORT_LANES_X8 = 8 /* reserved for future use */
};

enum mag_cmd_port_duplex {
	PORT_DUPLEX_HALF = 0,
	PORT_DUPLEX_FULL = 1
};

enum mag_cmd_wire_node {
	WIRE_NODE_UNDEF = 0,
	CABLE_10G = 1,
	FIBER_10G = 2,
	CABLE_25G = 3,
	FIBER_25G = 4,
	CABLE_40G = 5,
	FIBER_40G = 6,
	CABLE_50G = 7,
	FIBER_50G = 8,
	CABLE_100G = 9,
	FIBER_100G = 10,
	CABLE_200G = 11,
	FIBER_200G = 12,
	WIRE_NODE_NUM
};

enum mag_cmd_cnt_type {
	MAG_RX_RSFEC_DEC_CW_CNT = 0,
	MAG_RX_RSFEC_CORR_CW_CNT = 1,
	MAG_RX_RSFEC_UNCORR_CW_CNT = 2,
	MAG_RX_PCS_BER_CNT = 3,
	MAG_RX_PCS_ERR_BLOCK_CNT = 4,
	MAG_RX_PCS_E_BLK_CNT = 5,
	MAG_RX_PCS_DEC_ERR_BLK_CNT = 6,
	MAG_RX_PCS_LANE_BIP_ERR_CNT = 7,
	MAG_RX_RSFEC_ERR_CW_CNT = 8,
	MAG_CNT_NUM
};

/* mag_cmd_set_port_cfg config bitmap */
#define MAG_CMD_SET_SPEED 0x1
#define MAG_CMD_SET_AUTONEG 0x2
#define MAG_CMD_SET_FEC 0x4
#define MAG_CMD_SET_LANES 0x8
struct mag_cmd_set_port_cfg {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 rsvd0[3];

	u32 config_bitmap;
	u8 speed;
	u8 autoneg;
	u8 fec;
	u8 lanes;
	u8 rsvd1[20];
};

/* mag supported/advertised link mode bitmap */
enum mag_cmd_link_mode {
	LINK_MODE_GE = 0,
	LINK_MODE_10GE_BASE_R = 1,
	LINK_MODE_25GE_BASE_R = 2,
	LINK_MODE_40GE_BASE_R4 = 3,
	LINK_MODE_50GE_BASE_R = 4,
	LINK_MODE_50GE_BASE_R2 = 5,
	LINK_MODE_100GE_BASE_R = 6,
	LINK_MODE_100GE_BASE_R2 = 7,
	LINK_MODE_100GE_BASE_R4 = 8,
	LINK_MODE_200GE_BASE_R2 = 9,
	LINK_MODE_200GE_BASE_R4 = 10,
	LINK_MODE_MAX_NUMBERS,

	LINK_MODE_UNKNOWN = 0xFFFF
};

#define LINK_MODE_GE_BIT 0x1u
#define LINK_MODE_10GE_BASE_R_BIT 0x2u
#define LINK_MODE_25GE_BASE_R_BIT 0x4u
#define LINK_MODE_40GE_BASE_R4_BIT 0x8u
#define LINK_MODE_50GE_BASE_R_BIT 0x10u
#define LINK_MODE_50GE_BASE_R2_BIT 0x20u
#define LINK_MODE_100GE_BASE_R_BIT 0x40u
#define LINK_MODE_100GE_BASE_R2_BIT 0x80u
#define LINK_MODE_100GE_BASE_R4_BIT 0x100u
#define LINK_MODE_200GE_BASE_R2_BIT 0x200u
#define LINK_MODE_200GE_BASE_R4_BIT 0x400u

#define CABLE_10GE_BASE_R_BIT LINK_MODE_10GE_BASE_R_BIT
#define CABLE_25GE_BASE_R_BIT (LINK_MODE_25GE_BASE_R_BIT | LINK_MODE_10GE_BASE_R_BIT)
#define CABLE_40GE_BASE_R4_BIT LINK_MODE_40GE_BASE_R4_BIT
#define CABLE_50GE_BASE_R_BIT (LINK_MODE_50GE_BASE_R_BIT | LINK_MODE_25GE_BASE_R_BIT | \
	LINK_MODE_10GE_BASE_R_BIT)
#define CABLE_50GE_BASE_R2_BIT LINK_MODE_50GE_BASE_R2_BIT
#define CABLE_100GE_BASE_R2_BIT (LINK_MODE_100GE_BASE_R2_BIT | LINK_MODE_50GE_BASE_R2_BIT)
#define CABLE_100GE_BASE_R4_BIT (LINK_MODE_100GE_BASE_R4_BIT | LINK_MODE_40GE_BASE_R4_BIT)
#define CABLE_200GE_BASE_R4_BIT (LINK_MODE_200GE_BASE_R4_BIT | LINK_MODE_100GE_BASE_R4_BIT | \
	LINK_MODE_40GE_BASE_R4_BIT)

struct mag_cmd_get_port_info {
	struct mgmt_msg_head head;

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
	u32 supported_fec_mode;
	u16 bond_speed;
	u8 rsvd2[2];
};

#define MAG_CMD_OPCODE_GET 0
#define MAG_CMD_OPCODE_SET 1
struct mag_cmd_set_port_adapt {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 opcode; /* 0:get adapt info  1:set adapt */
	u8 enable;
	u8 rsvd0;
	u32 speed_mode;
	u32 rsvd1[3];
};

#define MAG_CMD_LP_MODE_SDS_S_TX2RX 1
#define MAG_CMD_LP_MODE_SDS_P_RX2TX 2
#define MAG_CMD_LP_MODE_SDS_P_TX2RX 3
#define MAG_CMD_LP_MODE_MAC_RX2TX 4
#define MAG_CMD_LP_MODE_MAC_TX2RX 5
#define MAG_CMD_LP_MODE_TXDP2RXDP 6
struct mag_cmd_cfg_loopback_mode {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 opcode; /* 0:get loopback mode  1:set loopback mode */
	u8 lp_mode;
	u8 lp_en; /* 0:disable  1:enable */

	u32 rsvd0[2];
};

#define MAG_CMD_PORT_DISABLE 0x0
#define MAG_CMD_TX_ENABLE 0x1
#define MAG_CMD_RX_ENABLE 0x2
/* the physical port is disable only when all pf of the port are set to down,
 * if any pf is enable, the port is enable
 */
struct mag_cmd_set_port_enable {
	struct mgmt_msg_head head;

	u16 function_id; /* function_id should not more than the max support pf_id(32) */
	u16 rsvd0;

	u8 state; /* bitmap bit0:tx_en bit1:rx_en */
	u8 rsvd1[3];
};

struct mag_cmd_get_port_enable {
	struct mgmt_msg_head head;

	u8 port;
	u8 state; /* bitmap bit0:tx_en bit1:rx_en */
	u8 rsvd0[2];
};

#define PMA_FOLLOW_DEFAULT 0x0
#define PMA_FOLLOW_ENABLE 0x1
#define PMA_FOLLOW_DISABLE 0x2
#define PMA_FOLLOW_GET 0x4
/* the physical port disable link follow only when all pf of the port are set to follow disable */
struct mag_cmd_set_link_follow {
	struct mgmt_msg_head head;

	u16 function_id; /* function_id should not more than the max support pf_id(32) */
	u16 rsvd0;

	u8 follow;
	u8 rsvd1[3];
};

/* firmware also use this cmd report link event to driver */
struct mag_cmd_get_link_status {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 status; /* 0:link down  1:link up */
	u8 rsvd0[2];
};

/* firmware also use this cmd report bond event to driver */
struct mag_cmd_get_bond_status {
	struct mgmt_msg_head head;

	u8 status; /* 0:bond down  1:bond up */
	u8 rsvd0[3];
};

struct mag_cmd_set_pma_enable {
	struct mgmt_msg_head head;

	u16 function_id; /* function_id should not more than the max support pf_id(32) */
	u16 enable;
};

struct mag_cmd_cfg_an_type {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 opcode; /* 0:get an type  1:set an type */
	u8 rsvd0[2];

	u32 an_type; /* 0:ieee   1:25G/50 eth consortium */
};

struct mag_cmd_get_link_time {
	struct mgmt_msg_head head;
	u8 port_id;
	u8 rsvd0[3];

	u32 link_up_begin;
	u32 link_up_end;
	u32 link_down_begin;
	u32 link_down_end;
};

struct mag_cmd_cfg_fec_mode {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 opcode; /* 0:get fec mode  1:set fec mode */
	u8 advertised_fec;
	u8 supported_fec;
};

/* speed */
#define PANGEA_ADAPT_10G_BITMAP 0xd
#define PANGEA_ADAPT_25G_BITMAP 0x72
#define PANGEA_ADAPT_40G_BITMAP 0x680
#define PANGEA_ADAPT_100G_BITMAP 0x1900

/* speed and fec */
#define PANGEA_10G_NO_BITMAP 0x8
#define PANGEA_10G_BASE_BITMAP 0x4
#define PANGEA_25G_NO_BITMAP 0x10
#define PANGEA_25G_BASE_BITMAP 0x20
#define PANGEA_25G_RS_BITMAP 0x40
#define PANGEA_40G_NO_BITMAP 0x400
#define PANGEA_40G_BASE_BITMAP 0x200
#define PANGEA_100G_NO_BITMAP 0x800
#define PANGEA_100G_RS_BITMAP 0x1000

/* adapt or fec */
#define PANGEA_ADAPT_ADAPT_BITMAP 0x183
#define PANGEA_ADAPT_NO_BITMAP 0xc18
#define PANGEA_ADAPT_BASE_BITMAP 0x224
#define PANGEA_ADAPT_RS_BITMAP 0x1040

/* default cfg */
#define PANGEA_ADAPT_CFG_10G_CR 0x200d
#define PANGEA_ADAPT_CFG_10G_SRLR 0xd
#define PANGEA_ADAPT_CFG_25G_CR 0x207f
#define PANGEA_ADAPT_CFG_25G_SRLR 0x72
#define PANGEA_ADAPT_CFG_40G_CR4 0x2680
#define PANGEA_ADAPT_CFG_40G_SRLR4 0x680
#define PANGEA_ADAPT_CFG_100G_CR4 0x3f80
#define PANGEA_ADAPT_CFG_100G_SRLR4 0x1900

union pangea_adapt_bitmap_u {
	struct {
		u32 adapt_10g : 1;  /* [0]   adapt_10g  */
		u32 adapt_25g : 1;  /* [1]   adapt_25g  */
		u32 base_10g : 1;   /* [2]   base_10g   */
		u32 no_10g : 1;	 /* [3]   no_10g	 */
		u32 no_25g : 1;	 /* [4]   no_25g	 */
		u32 base_25g : 1;   /* [5]   base_25g   */
		u32 rs_25g : 1;	 /* [6]   rs_25g	 */
		u32 adapt_40g : 1;  /* [7]   adapt_40g  */
		u32 adapt_100g : 1; /* [8]   adapt_100g */
		u32 base_40g : 1;   /* [9]   base_40g   */
		u32 no_40g : 1;	 /* [10]  no_40g	 */
		u32 no_100g : 1;	/* [11]  no_100g	*/
		u32 rs_100g : 1;	/* [12]  rs_100g	*/
		u32 auto_neg : 1;   /* [13]  auto_neg   */
		u32 rsvd0 : 18;	 /* [31:14] reserved */
	} bits;

	u32 value;
};

#define PANGEA_ADAPT_GET 0x0
#define PANGEA_ADAPT_SET 0x1
struct mag_cmd_set_pangea_adapt {
	struct mgmt_msg_head head;

	u16 port_id;
	u8 opcode; /* 0:get adapt info  1:cfg adapt info */
	u8 wire_type;

	union pangea_adapt_bitmap_u cfg_bitmap;
	union pangea_adapt_bitmap_u cur_bitmap;
	u32 rsvd1[3];
};

struct mag_cmd_cfg_bios_link_cfg {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 opcode; /* 0:get bios link info  1:set bios link cfg */
	u8 clear;
	u8 rsvd0;

	u32 wire_type;
	u8 an_en;
	u8 speed;
	u8 fec;
	u8 rsvd1;
	u32 speed_mode;
	u32 rsvd2[3];
};

struct mag_cmd_restore_link_cfg {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 rsvd[7];
};

struct mag_cmd_activate_bios_link_cfg {
	struct mgmt_msg_head head;

	u32 rsvd[8];
};

/* led type */
enum mag_led_type {
	MAG_CMD_LED_TYPE_ALARM = 0x0,
	MAG_CMD_LED_TYPE_LOW_SPEED = 0x1,
	MAG_CMD_LED_TYPE_HIGH_SPEED = 0x2
};

/* led mode */
enum mag_led_mode {
	MAG_CMD_LED_MODE_DEFAULT = 0x0,
	MAG_CMD_LED_MODE_FORCE_ON = 0x1,
	MAG_CMD_LED_MODE_FORCE_OFF = 0x2,
	MAG_CMD_LED_MODE_FORCE_BLINK_1HZ = 0x3,
	MAG_CMD_LED_MODE_FORCE_BLINK_2HZ = 0x4,
	MAG_CMD_LED_MODE_FORCE_BLINK_4HZ = 0x5,
	MAG_CMD_LED_MODE_1HZ = 0x6,
	MAG_CMD_LED_MODE_2HZ = 0x7,
	MAG_CMD_LED_MODE_4HZ = 0x8
};

/* the led is report alarm  when any pf of the port is alram */
struct mag_cmd_set_led_cfg {
	struct mgmt_msg_head head;

	u16 function_id;
	u8 type;
	u8 mode;
};

#define XSFP_INFO_MAX_SIZE 640
/* xsfp wire type, refer to cmis protocol definition */
enum mag_wire_type {
	MAG_CMD_WIRE_TYPE_UNKNOWN = 0x0,
	MAG_CMD_WIRE_TYPE_MM = 0x1,
	MAG_CMD_WIRE_TYPE_SM = 0x2,
	MAG_CMD_WIRE_TYPE_COPPER = 0x3,
	MAG_CMD_WIRE_TYPE_ACC = 0x4,
	MAG_CMD_WIRE_TYPE_BASET = 0x5,
	MAG_CMD_WIRE_TYPE_AOC = 0x40,
	MAG_CMD_WIRE_TYPE_ELECTRIC = 0x41,
	MAG_CMD_WIRE_TYPE_BACKPLANE = 0x42
};

struct mag_cmd_get_xsfp_info {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 wire_type;
	u16 out_len;
	u32 rsvd;
	u8 sfp_info[XSFP_INFO_MAX_SIZE];
};

#define MAG_CMD_XSFP_DISABLE 0x0
#define MAG_CMD_XSFP_ENABLE 0x1
/* the sfp is disable only when all pf of the port are set sfp down,
 * if any pf is enable, the sfp is enable
 */
struct mag_cmd_set_xsfp_enable {
	struct mgmt_msg_head head;

	u32 port_id;
	u32 status; /* 0:on 1:off */
};

#define MAG_CMD_XSFP_PRESENT 0x0
#define MAG_CMD_XSFP_ABSENT 0x1
struct mag_cmd_get_xsfp_present {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 abs_status; /* 0:present, 1:absent */
	u8 rsvd[2];
};

#define MAG_CMD_XSFP_READ 0x0
#define MAG_CMD_XSFP_WRITE 0x1
struct mag_cmd_set_xsfp_rw {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 operation; /* 0: read; 1: write */
	u8 value;
	u8 rsvd0;
	u32 devaddr;
	u32 offset;
	u32 rsvd1;
};

struct mag_cmd_cfg_xsfp_temperature {
	struct mgmt_msg_head head;

	u8 opcode; /* 0:read  1:write */
	u8 rsvd0[3];
	s32 max_temp;
	s32 min_temp;
};

struct mag_cmd_get_xsfp_temperature {
	struct mgmt_msg_head head;

	s16 sfp_temp[8];
	u8 rsvd[32];
	s32 max_temp;
	s32 min_temp;
};

/* xsfp plug event */
struct mag_cmd_wire_event {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 status; /* 0:present, 1:absent */
	u8 rsvd[2];
};

/* link err type definition */
#define MAG_CMD_ERR_XSFP_UNKNOWN 0x0
struct mag_cmd_link_err_event {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 link_err_type;
	u8 rsvd[2];
};

#define MAG_PARAM_TYPE_DEFAULT_CFG 0x0
#define MAG_PARAM_TYPE_BIOS_CFG 0x1
#define MAG_PARAM_TYPE_TOOL_CFG 0x2
#define MAG_PARAM_TYPE_FINAL_CFG 0x3
#define MAG_PARAM_TYPE_WIRE_INFO 0x4
#define MAG_PARAM_TYPE_ADAPT_INFO 0x5
#define MAG_PARAM_TYPE_MAX_CNT 0x6
struct param_head {
	u8 valid_len;
	u8 info_type;
	u8 rsvd[2];
};

struct mag_port_link_param {
	struct param_head head;

	u8 an;
	u8 fec;
	u8 speed;
	u8 rsvd0;

	u32 used;
	u32 an_fec_ability;
	u32 an_speed_ability;
	u32 an_pause_ability;
};

struct mag_port_wire_info {
	struct param_head head;

	u8 status;
	u8 rsvd0[3];

	u8 wire_type;
	u8 default_fec;
	u8 speed;
	u8 rsvd1;
	u32 speed_ability;
};

struct mag_port_adapt_info {
	struct param_head head;

	u32 adapt_en;
	u32 flash_adapt;
	u32 rsvd0[2];

	u32 wire_node;
	u32 an_en;
	u32 speed;
	u32 fec;
};

struct mag_port_param_info {
	u8 parameter_cnt;
	u8 lane_id;
	u8 lane_num;
	u8 rsvd0;

	struct mag_port_link_param default_cfg;
	struct mag_port_link_param bios_cfg;
	struct mag_port_link_param tool_cfg;
	struct mag_port_link_param final_cfg;

	struct mag_port_wire_info wire_info;
	struct mag_port_adapt_info adapt_info;
};

#define XSFP_VENDOR_NAME_LEN 16
struct mag_cmd_event_port_info {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 event_type;
	u8 rsvd0[2];

	u8 vendor_name[XSFP_VENDOR_NAME_LEN];
	u32 port_type;		/* fiber / copper */
	u32 port_sub_type;	/* sr / lr */
	u32 cable_length;	/* 1/3/5m */
	u8 cable_temp;		/* temp */
	u8 max_speed;		/* Maximum rate of an optical module */
	u8 sfp_type;		/* sfp/qsfp/dsfp */
	u8 rsvd1;
	u32 power[4];		/* Optical Power */

	u8 an_state;
	u8 fec;
	u16 speed;

	u8 gpio_insert;		/* 0:present  1:absent */
	u8 alos;
	u8 rx_los;
	u8 pma_ctrl;

	u32 pma_fifo_reg;
	u32 pma_signal_ok_reg;
	u32 pcs_64_66b_reg;
	u32 rf_lf;
	u8 pcs_link;
	u8 pcs_mac_link;
	u8 tx_enable;
	u8 rx_enable;
	u32 pcs_err_cnt;

	u8 eq_data[38];
	u8 rsvd2[2];

	u32 his_link_machine_state;
	u32 cur_link_machine_state;
	u8 his_machine_state_data[128];
	u8 cur_machine_state_data[128];
	u8 his_machine_state_length;
	u8 cur_machine_state_length;

	struct mag_port_param_info param_info;
	u8 rsvd3[360];
};

struct mag_cmd_rsfec_stats {
	u32 rx_err_lane_phy;
};

struct mag_cmd_port_stats {
	u64 mac_tx_fragment_pkt_num;
	u64 mac_tx_undersize_pkt_num;
	u64 mac_tx_undermin_pkt_num;
	u64 mac_tx_64_oct_pkt_num;
	u64 mac_tx_65_127_oct_pkt_num;
	u64 mac_tx_128_255_oct_pkt_num;
	u64 mac_tx_256_511_oct_pkt_num;
	u64 mac_tx_512_1023_oct_pkt_num;
	u64 mac_tx_1024_1518_oct_pkt_num;
	u64 mac_tx_1519_2047_oct_pkt_num;
	u64 mac_tx_2048_4095_oct_pkt_num;
	u64 mac_tx_4096_8191_oct_pkt_num;
	u64 mac_tx_8192_9216_oct_pkt_num;
	u64 mac_tx_9217_12287_oct_pkt_num;
	u64 mac_tx_12288_16383_oct_pkt_num;
	u64 mac_tx_1519_max_bad_pkt_num;
	u64 mac_tx_1519_max_good_pkt_num;
	u64 mac_tx_oversize_pkt_num;
	u64 mac_tx_jabber_pkt_num;
	u64 mac_tx_bad_pkt_num;
	u64 mac_tx_bad_oct_num;
	u64 mac_tx_good_pkt_num;
	u64 mac_tx_good_oct_num;
	u64 mac_tx_total_pkt_num;
	u64 mac_tx_total_oct_num;
	u64 mac_tx_uni_pkt_num;
	u64 mac_tx_multi_pkt_num;
	u64 mac_tx_broad_pkt_num;
	u64 mac_tx_pause_num;
	u64 mac_tx_pfc_pkt_num;
	u64 mac_tx_pfc_pri0_pkt_num;
	u64 mac_tx_pfc_pri1_pkt_num;
	u64 mac_tx_pfc_pri2_pkt_num;
	u64 mac_tx_pfc_pri3_pkt_num;
	u64 mac_tx_pfc_pri4_pkt_num;
	u64 mac_tx_pfc_pri5_pkt_num;
	u64 mac_tx_pfc_pri6_pkt_num;
	u64 mac_tx_pfc_pri7_pkt_num;
	u64 mac_tx_control_pkt_num;
	u64 mac_tx_err_all_pkt_num;
	u64 mac_tx_from_app_good_pkt_num;
	u64 mac_tx_from_app_bad_pkt_num;

	u64 mac_rx_fragment_pkt_num;
	u64 mac_rx_undersize_pkt_num;
	u64 mac_rx_undermin_pkt_num;
	u64 mac_rx_64_oct_pkt_num;
	u64 mac_rx_65_127_oct_pkt_num;
	u64 mac_rx_128_255_oct_pkt_num;
	u64 mac_rx_256_511_oct_pkt_num;
	u64 mac_rx_512_1023_oct_pkt_num;
	u64 mac_rx_1024_1518_oct_pkt_num;
	u64 mac_rx_1519_2047_oct_pkt_num;
	u64 mac_rx_2048_4095_oct_pkt_num;
	u64 mac_rx_4096_8191_oct_pkt_num;
	u64 mac_rx_8192_9216_oct_pkt_num;
	u64 mac_rx_9217_12287_oct_pkt_num;
	u64 mac_rx_12288_16383_oct_pkt_num;
	u64 mac_rx_1519_max_bad_pkt_num;
	u64 mac_rx_1519_max_good_pkt_num;
	u64 mac_rx_oversize_pkt_num;
	u64 mac_rx_jabber_pkt_num;
	u64 mac_rx_bad_pkt_num;
	u64 mac_rx_bad_oct_num;
	u64 mac_rx_good_pkt_num;
	u64 mac_rx_good_oct_num;
	u64 mac_rx_total_pkt_num;
	u64 mac_rx_total_oct_num;
	u64 mac_rx_uni_pkt_num;
	u64 mac_rx_multi_pkt_num;
	u64 mac_rx_broad_pkt_num;
	u64 mac_rx_pause_num;
	u64 mac_rx_pfc_pkt_num;
	u64 mac_rx_pfc_pri0_pkt_num;
	u64 mac_rx_pfc_pri1_pkt_num;
	u64 mac_rx_pfc_pri2_pkt_num;
	u64 mac_rx_pfc_pri3_pkt_num;
	u64 mac_rx_pfc_pri4_pkt_num;
	u64 mac_rx_pfc_pri5_pkt_num;
	u64 mac_rx_pfc_pri6_pkt_num;
	u64 mac_rx_pfc_pri7_pkt_num;
	u64 mac_rx_control_pkt_num;
	u64 mac_rx_sym_err_pkt_num;
	u64 mac_rx_fcs_err_pkt_num;
	u64 mac_rx_send_app_good_pkt_num;
	u64 mac_rx_send_app_bad_pkt_num;
	u64 mac_rx_unfilter_pkt_num;
};

struct mag_port_stats {
	u64 tx_frag_pkts_port;
	u64 tx_under_frame_pkts_port;
	u64 tx_under_min_pkts_port;
	u64 tx_64_oct_pkts_port;
	u64 tx_127_oct_pkts_port;
	u64 tx_255_oct_pkts_port;
	u64 tx_511_oct_pkts_port;
	u64 tx_1023_oct_pkts_port;
	u64 tx_1518_oct_pkts_port;
	u64 tx_2047_oct_pkts_port;
	u64 tx_4095_oct_pkts_port;
	u64 tx_8191_oct_pkts_port;
	u64 tx_9216_oct_pkts_port;
	u64 tx_12287_oct_pkts_port;
	u64 tx_16383_oct_pkts_port;
	u64 tx_1519_to_max_bad_pkts_port;
	u64 tx_1519_to_max_good_pkts_port;
	u64 tx_oversize_pkts_port;
	u64 tx_jabber_pkts_port;
	u64 tx_bad_pkts_port;
	u64 tx_bad_octs_port;
	u64 tx_good_pkts_port;
	u64 tx_good_octs_port;
	u64 tx_total_pkts_port;
	u64 tx_total_octs_port;
	u64 tx_unicast_pkts_port;
	u64 tx_multicast_pkts_port;
	u64 tx_broadcast_pkts_port;
	u64 tx_pause_pkts_port;
	u64 tx_pfc_pkts_port;
	u64 tx_pri_0_pkts_port;
	u64 tx_pri_1_pkts_port;
	u64 tx_pri_2_pkts_port;
	u64 tx_pri_3_pkts_port;
	u64 tx_pri_4_pkts_port;
	u64 tx_pri_5_pkts_port;
	u64 tx_pri_6_pkts_port;
	u64 tx_pri_7_pkts_port;
	u64 tx_mac_control_pkts_port;
	u64 tx_y1731_pkts_port;
	u64 tx_1588_pkts_port;
	u64 tx_error_pkts_port;
	u64 tx_app_good_pkts_port;
	u64 tx_app_bad_pkts_port;
	u64 rx_frag_pkts_port;
	u64 rx_under_frame_pkts_port;
	u64 rx_under_min_pkts_port;
	u64 rx_64_oct_pkts_port;
	u64 rx_127_oct_pkts_port;
	u64 rx_255_oct_pkts_port;
	u64 rx_511_oct_pkts_port;
	u64 rx_1023_oct_pkts_port;
	u64 rx_1518_oct_pkts_port;
	u64 rx_2047_oct_pkts_port;
	u64 rx_4095_oct_pkts_port;
	u64 rx_8191_oct_pkts_port;
	u64 rx_9216_oct_pkts_port;
	u64 rx_12287_oct_pkts_port;
	u64 rx_16383_oct_pkts_port;
	u64 rx_1519_to_max_bad_pkts_port;
	u64 rx_1519_to_max_good_pkts_port;
	u64 rx_oversize_pkts_port;
	u64 rx_jabber_pkts_port;
	u64 rx_bad_pkts_port;
	u64 rx_bad_octs_port;
	u64 rx_good_pkts_port;
	u64 rx_good_octs_port;
	u64 rx_total_pkts_port;
	u64 rx_total_octs_port;
	u64 rx_unicast_pkts_port;
	u64 rx_multicast_pkts_port;
	u64 rx_broadcast_pkts_port;
	u64 rx_pause_pkts_port;
	u64 rx_pfc_pkts_port;
	u64 rx_pri_0_pkts_port;
	u64 rx_pri_1_pkts_port;
	u64 rx_pri_2_pkts_port;
	u64 rx_pri_3_pkts_port;
	u64 rx_pri_4_pkts_port;
	u64 rx_pri_5_pkts_port;
	u64 rx_pri_6_pkts_port;
	u64 rx_pri_7_pkts_port;
	u64 rx_mac_control_pkts_port;
	u64 rx_y1731_pkts_port;
	u64 rx_sym_err_pkts_port;
	u64 rx_fcs_err_pkts_port;
	u64 rx_app_good_pkts_port;
	u64 rx_app_bad_pkts_port;
	u64 rx_unfilter_pkts_port;
};

struct mag_cmd_port_stats_info {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 rsvd0[3];
};

struct mag_cmd_get_port_stat {
	struct mgmt_msg_head head;

	struct mag_cmd_port_stats counter;
	u64 rsvd1[15];
};

struct mag_cmd_get_pcs_err_cnt {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 rsvd0[3];

	u32 pcs_err_cnt;
};

struct mag_cmd_get_mag_cnt {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 len;
	u8 rsvd0[2];

	u32 mag_csr[128];
};

struct mag_cmd_dump_antrain_info {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 len;
	u8 rsvd0[2];

	u32 antrain_csr[256];
};

#define MAG_SFP_PORT_NUM 24
struct mag_cmd_sfp_temp_in_info {
	struct mgmt_msg_head head; /* 8B */
	u8 opt_type;	/* 0:read operation 1:cfg operation */
	u8 rsv[3];
	s32 max_temp;	/* Chip optical module threshold */
	s32 min_temp;	/* Chip optical module threshold */
};

struct mag_cmd_sfp_temp_out_info {
	struct mgmt_msg_head head;	     /* 8B */
	s16 sfp_temp_data[MAG_SFP_PORT_NUM]; /* Temperature read */
	s32 max_temp;			     /* Chip optical module threshold */
	s32 min_temp;			     /* Chip optical module threshold */
};

#define XSFP_CMIS_PARSE_PAGE_NUM	6
#define XSFP_CMIS_INFO_MAX_SIZE		1536
#define QSFP_CMIS_PAGE_SIZE		128
#define QSFP_CMIS_MAX_CHANNEL_NUM	0x8

/* Lower: Control and Essentials, Upper: Administrative Information */
#define QSFP_CMIS_PAGE_00H		    0x00
/* Advertising */
#define QSFP_CMIS_PAGE_01H		    0x01
/* Module and lane Thresholds */
#define QSFP_CMIS_PAGE_02H		    0x02
/* User EEPROM */
#define QSFP_CMIS_PAGE_03H		    0x03
/* Laser Capabilities Advertising (Page 04h, Optional) */
#define QSFP_CMIS_PAGE_04H		    0x04
#define QSFP_CMIS_PAGE_05H		    0x05
/* Lane and Data Path Control */
#define QSFP_CMIS_PAGE_10H		    0x10
/* Lane Status */
#define QSFP_CMIS_PAGE_11H		    0x11
#define QSFP_CMIS_PAGE_12H		    0x12

#define MGMT_TLV_U8_SIZE	1
#define MGMT_TLV_U16_SIZE	2
#define MGMT_TLV_U32_SIZE	4

#define MGMT_TLV_GET_U8(addr)          (*((u8 *)(void *)(addr)))
#define MGMT_TLV_SET_U8(addr, value) \
	((*((u8 *)(void *)(addr))) = ((u8)(value)))

#define MGMT_TLV_GET_U16(addr)         (*((u16 *)(void *)(addr)))
#define MGMT_TLV_SET_U16(addr, value) \
	((*((u16 *)(void *)(addr))) = ((u16)(value)))

#define MGMT_TLV_GET_U32(addr)         (*((u32 *)(void *)(addr)))
#define MGMT_TLV_SET_U32(addr, value) \
	((*((u32 *)(void *)(addr))) = ((u32)(value)))

#define MGMT_TLV_TYPE_END       0xFFFF

enum mag_xsfp_type {
	MAG_XSFP_TYPE_PAGE      = 0x01,
	MAG_XSFP_TYPE_WIRE_TYPE = 0x02,
	MAG_XSFP_TYPE_END       = MGMT_TLV_TYPE_END
};

struct qsfp_cmis_lower_page_00_s {
	u8 resv0[14];
	u8 temperature_msb;
	u8 temperature_lsb;
	u8 volt_supply[2];
	u8 resv1[67];
	u8 media_type;
	u8 electrical_interface_id;
	u8 media_interface_id;
	u8 lane_count;
	u8 resv2[39];
};

struct qsfp_cmis_upper_page_00_s {
	u8 identifier;
	u8 vendor_name[16];
	u8 vendor_oui[3];
	u8 vendor_pn[16];
	u8 vendor_rev[2];
	u8 vendor_sn[16];
	u8 date_code[8];
	u8 clei_code[10];
	u8 power_character[2];
	u8 cable_len;
	u8 connector;
	u8 copper_cable_attenuation[6];
	u8 near_end_implementation;
	u8 far_end_config;
	u8 media_technology;
	u8 resv0[43];
};

struct qsfp_cmis_upper_page_01_s {
	u8 firmware_rev[2];
	u8 hardware_rev[2];
	u8 smf_len_km;
	u8 om5_len;
	u8 om4_len;
	u8 om3_len;
	u8 om2_len;
	u8 resv0;
	u8 wavelength[2];
	u8 wavelength_tolerance[2];
	u8 pages_implement;
	u8 resv1[16];
	u8 monitor_implement[2];
	u8 resv2[95];
};

struct qsfp_cmis_upper_page_02_s {
	u8 temperature_high_alarm[2];
	u8 temperature_low_alarm[2];
	u8 temperature_high_warn[2];
	u8 temperature_low_warn[2];
	u8 volt_high_alarm[2];
	u8 volt_low_alarm[2];
	u8 volt_high_warn[2];
	u8 volt_low_warn[2];
	u8 resv0[32];
	u8 tx_power_high_alarm[2];
	u8 tx_power_low_alarm[2];
	u8 tx_power_high_warn[2];
	u8 tx_power_low_warn[2];
	u8 tx_bias_high_alarm[2];
	u8 tx_bias_low_alarm[2];
	u8 tx_bias_high_warn[2];
	u8 tx_bias_low_warn[2];
	u8 rx_power_high_alarm[2];
	u8 rx_power_low_alarm[2];
	u8 rx_power_high_warn[2];
	u8 rx_power_low_warn[2];
	u8 resv1[56];
};

struct qsfp_cmis_upper_page_03_s {
	u8 resv0[QSFP_CMIS_PAGE_SIZE]; /* Reg 128-255: Upper Memory: Page 03H */
};

struct qsfp_cmis_upper_page_10_s {
	u8 resv0[2];	/* Reg 128-129: Upper Memory: Page 10H */
	u8 tx_disable;	/* Reg 130: Tx disable, 0b=enabled, 1b=disabled */
	u8 resv1[125];	/* Reg 131-255 */
};

struct qsfp_cmis_upper_page_11_s {
	u8 resv0[7];
	u8 tx_fault;
	u8 tx_los;
	u8 resv1[10];
	u8 rx_los;
	u8 resv2[6];
	u8 tx_power[16];
	u8 tx_bias[16];
	u8 rx_power[16];
	u8 resv3[54];
};

struct qsfp_cmis_info_s {
	struct qsfp_cmis_lower_page_00_s lower_page_00;
	struct qsfp_cmis_upper_page_00_s upper_page_00;
	struct qsfp_cmis_upper_page_01_s upper_page_01;
	struct qsfp_cmis_upper_page_02_s upper_page_02;
	struct qsfp_cmis_upper_page_10_s upper_page_10;
	struct qsfp_cmis_upper_page_11_s upper_page_11;
};

struct qsfp_cmis_comm_power_s {
	u32 chl_power[QSFP_CMIS_MAX_CHANNEL_NUM];
};

struct qsfp_cmis_wire_info_s {
	struct qsfp_cmis_comm_power_s rx_power;
	u8			      rx_los;
	u8			      resv0[3];
};

struct mgmt_tlv_info {
	u16 type;
	u16 length;
	u8 value[];
};

struct mag_cmd_set_xsfp_tlv_req {
	struct mgmt_msg_head head;

	u8 tlv_buf[];
};

struct mag_cmd_set_xsfp_tlv_rsp {
	struct mgmt_msg_head head;
};

struct tag_mag_cmd_get_xsfp_tlv_req {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 rsvd;
	u16 rsp_buf_len;
};

struct mag_cmd_get_xsfp_tlv_rsp {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 rsvd[3];

	u8 tlv_buf[];
};

struct parse_tlv_info {
	u8 tlv_page_info[XSFP_CMIS_INFO_MAX_SIZE + 1];
	u32 tlv_page_info_len;
	u32 tlv_page_num[XSFP_CMIS_PARSE_PAGE_NUM];
	u32 wire_type;
	u8 id;
};

struct drv_tag_mag_cmd_get_xsfp_tlv_rsp {
	struct mgmt_msg_head head;

	u8 port_id;
	u8 rsvd[3];

	u8 tlv_buf[XSFP_CMIS_INFO_MAX_SIZE];
};

#endif

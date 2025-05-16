/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2024 Huawei Technologies Co., Ltd */

#ifndef MPU_OUTBAND_NCSI_CMD_DEFS_H
#define MPU_OUTBAND_NCSI_CMD_DEFS_H

#pragma pack(push, 1)

enum NCSI_RESPONSE_CODE_E {
	COMMAND_COMPLETED = 0x00,	/**< command completed */
	COMMAND_FAILED = 0x01,		/**< command failed */
	COMMAND_UNAVAILABLE = 0x02,	/**< command unavailable */
	COMMAND_UNSPORRTED = 0x03	/**< command unsporrted */
};

enum NCSI_REASON_CODE_E {
	NO_ERROR = 0x00,		/**< no error */
	INTERFACE_INIT_REQUIRED = 0x01,	/**< interface init required */
	INVALID_PARA = 0x02,		/**< invalid parameter */
	CHAN_NOT_READY = 0x03,		/**< channel not ready */
	PKG_NOT_READY = 0x04,		/**< package not ready */
	INVALID_PAYLOAD_LEN = 0x05,	/**< invalid payload len */
	LINK_STATUS_ERROR = 0xA06,	/**< get link status fail */
	VLAN_TAG_INVALID = 0xB07,	/**< vlan tag invalid */
	MAC_ADD_IS_ZERO = 0xE08,	/**< mac add is zero */
	FLOW_CONTROL_UNSUPPORTED = 0x09,	/**< flow control unsupported */
	CHECKSUM_ERR = 0xA,			/**< check sum error */
	/**< the command type is unsupported only when the response code is 0x03 */
	UNSUPPORTED_COMMAND_TYPE = 0x7FFF
};

enum NCSI_CLIENT_TYPE_E {
	NCSI_RMII_TYPE = 1,	/**< rmii client */
	NCSI_MCTP_TYPE = 2,	/**< MCTP client */
	NCSI_AEN_TYPE = 3	/**< AEN client */
};

/**
 * @brief ncsi ctrl packet header
 */
struct tag_ncsi_ctrl_packet_header {
	u8 mc_id;		/**< management control ID */
	u8 head_revision;	/**< head revision */
	u8 reserved0;		/**< reserved */
	u8 iid;			/**< instance ID */
	u8 pkt_type;		/**< packet type */
#ifdef NCSI_BIG_ENDIAN
	u8 pkg_id : 3;		/**< packet ID */
	u8 inter_chan_id : 5;	/**< channel ID */
#else
	u8 inter_chan_id : 5;	/**< channel ID */
	u8 pkg_id : 3;		/**< packet ID */
#endif
#ifdef BD_BIG_ENDIAN
	u8 reserved1 : 4;	/**< reserved1 */
	u8 payload_len_hi : 4;	/**< payload len have 12bits */
#else
	u8 payload_len_hi : 4;	/**< payload len have 12bits */
	u8 reserved1 : 4;	/**< reserved1 */
#endif
	u8 payload_len_lo;	/**< payload len lo */
	u32 reserved2;		/**< reserved2 */
	u32 reserved3;		/**< reserved3 */
};

#define NCSI_MAX_PAYLOAD_LEN 1500
#define NCSI_MAC_LEN 6

/**
 * @brief ncsi clear initial state command struct defination
 *
 */
struct tag_ncsi_ctrl_packet {
	struct tag_ncsi_ctrl_packet_header packet_head;	/**< ncsi ctrl packet header */
	u8 payload[NCSI_MAX_PAYLOAD_LEN];	/**< ncsi ctrl packet payload */
};

/**
 * @brief ethernet header description
 *
 */
struct tag_ethernet_header {
	u8 dst_addr[NCSI_MAC_LEN];	/**< ethernet destination address */
	u8 src_addr[NCSI_MAC_LEN];	/**< ethernet source address */
	u16 ether_type;			/**< ethernet type */
};

/**
 * @brief ncsi common packet description
 *
 */
struct tg_ncsi_common_packet {
	struct tag_ethernet_header frame_head;	/**< common packet ethernet frame header */
	struct tag_ncsi_ctrl_packet ctrl_packet;	/**< common packet ncsi ctrl packet */
};

/**
 * @brief ncsi clear initial state command struct defination
 */
struct tag_ncsi_client_info {
	u8 *name;	/**< client info client name */
	u32 type;	/**< client info type of ncsi media  @see enum NCSI_CLIENT_TYPE_E */
	u8 bmc_mac[NCSI_MAC_LEN];	/**< client info BMC mac addr */
	u8 ncsi_mac[NCSI_MAC_LEN];	/**< client info local mac addr */
	u8 reserve[2];			/**< client info reserved, Four-byte alignment */
	u32 rsp_len;			/**< client info include pad */
	struct tg_ncsi_common_packet ncsi_packet_rsp;	/**< ncsi common packet response */
};

/* AEN Enable Command (0x08)  */
#define AEN_ENABLE_REQ_LEN 8
#define AEN_ENABLE_RSP_LEN 4
#define AEN_CTRL_LINK_STATUS_SHIFT 0
#define AEN_CTRL_CONFIG_REQ_SHIFT 1
#define AEN_CTRL_DRV_CHANGE_SHIFT 2

/* AEN Type */
enum aen_type_e {
	AEN_LINK_STATUS_CHANGE_TYPE = 0x0,
	AEN_CONFIG_REQUIRED_TYPE    = 0x1,
	OEM_AEN_CONFIG_REQUEST_TYPE = 0x80,
	AEN_TYPE_MAX = 0x100
};

/* get link status 0x0A */
#define GET_LINK_STATUS_REQ_LEN 0
#define GET_LINK_STATUS_RSP_LEN 16
/* link speed(fc link speed is mapped to unknown) */
enum NCSI_CMD_LINK_SPEED_E {
	LINK_SPEED_10M = 0x2,	/**< 10M */
	LINK_SPEED_100M = 0x5,	/**< 100M */
	LINK_SPEED_1G = 0x7,	/**< 1G */
	LINK_SPEED_10G = 0x8,	/**< 10G */
	LINK_SPEED_20G = 0x9,	/**< 20G */
	LINK_SPEED_25G = 0xa,	/**< 25G */
	LINK_SPEED_40G = 0xb,	/**< 40G */
	LINK_SPEED_50G = 0xc,	/**< 50G */
	LINK_SPEED_100G = 0xd,	/**< 100G */
	LINK_SPEED_2_5G = 0xe,	/**< 2.5G */
	LINK_SPEED_UNKNOWN = 0xf
};

/* Set Vlan Filter (0x0B) */
/* Only VLAN-tagged packets that match the enabled VLAN Filter settings are accepted. */
#define VLAN_MODE_UNSET 0X00
#define VLAN_ONLY 0x01
/* if match the MAC address ,any vlan-tagged and non-vlan-tagged will be accepted */
#define ANYVLAN_NONVLAN 0x03
#define VLAN_MODE_SUPPORT 0x05

/* chanel vlan filter enable */
#define CHNL_VALN_FL_ENABLE 0x01
#define CHNL_VALN_FL_DISABLE 0x00

/* vlan id invalid */
#define VLAN_ID_VALID 0x01
#define VLAN_ID_INVALID 0x00

/* VLAN ID */
#define SET_VLAN_FILTER_REQ_LEN 8
#define SET_VLAN_FILTER_RSP_LEN 4

/* ncsi_get_controller_packet_statistics_config */
#define NO_INFORMATION_STATISTICS 0xff

/* Enable VLAN Command (0x0C) */
#define ENABLE_VLAN_REQ_LEN 4
#define ENABLE_VLAN_RSP_LEN 4
#define VLAN_FL_MAX_ID 8

/* NCSI channel capabilities */
struct tag_ncsi_chan_capa {
	u32 capa_flags;		/**< NCSI channel capabilities capa flags */
	u32 bcast_filter;	/**< NCSI channel capabilities bcast filter */
	u32 multicast_filter;	/**< NCSI channel capabilities multicast filter */
	u32 buffering;		/**< NCSI channel capabilities buffering */
	u32 aen_ctrl;		/**< NCSI channel capabilities aen ctrl */
	u8 vlan_count;		/**< NCSI channel capabilities vlan count */
	u8 mixed_count;		/**< NCSI channel capabilities mixed count */
	u8 multicast_count;	/**< NCSI channel capabilities multicast count */
	u8 unicast_count;	/**< NCSI channel capabilities unicast count */
	u16 rsvd;		/**< NCSI channel capabilities reserved */
	u8 vlan_mode;		/**< NCSI channel capabilities vlan mode */
	u8 chan_count;		/**< NCSI channel capabilities channel count */
};

struct tg_g_ncsi_parameters {
	u8 mac_address_count;
	u8 reserved1[2];
	u8 mac_address_flags;
	u8 vlan_tag_count;
	u8 reserved2;
	u16 vlan_tag_flags;
	u32 link_settings;
	u32 broadcast_packet_filter_settings;
	u8 broadcast_packet_filter_status : 1;
	u8 channel_enable : 1;
	u8 channel_network_tx_enable : 1;
	u8 global_mulicast_packet_filter_status : 1;
	/**< bit0-3:mac_add0——mac_add3 address type：0 unicast，1 multileaving */
	u8 config_flags_reserved1 : 4;
	u8 config_flags_reserved2[3];
	u8 vlan_mode;	/**< current vlan mode */
	u8 flow_control_enable;
	u16 reserved3;
	u32 AEN_control;
	u8 mac_add[4][6];
	u16 vlan_tag[VLAN_FL_MAX_ID];
};

#pragma pack(pop)

#endif

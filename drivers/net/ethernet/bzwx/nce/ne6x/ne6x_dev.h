/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_DEV_H
#define _NE6X_DEV_H

#include "ne6x_portmap.h"

#define NE6000AI_2S_X16H_100G_N5 0xA050
#define NE6000AI_2S_X16H_25G_N5  0xA030
#define NE6000AI_2S_X16H_25G_N6  0xA031

#define NE6000_IF_INTERFACE_UP     1
#define NE6000_IF_INTERFACE_DOWN   0

struct ne6x_flowctrl {
	u32 autoneg;
	u32 rx_pause;
	u32 tx_pause;
};

struct ne6x_sfp_mod_type_len {
	u32 type;
	u32 len;
};

enum {
	NE6X_SOC_TEMPERATURE = 0x0,
	NE6X_SOC_POWER_CONSUM,
	NE6X_SOC_DDR_TEST,
	NE6X_SOC_FRU,
	NE6X_SOC_SERDES_SEND_BIT,
	NE6X_SOC_I2C3_TEST,
};

struct ne6x_soc_temperature {
	u32 chip_temerature;
	u32 board_temperature;
};

struct ne6x_soc_power {
	u32 cur;
	u32 vol;
	u32 power;
};

#define NE6X_FW_SIG_OFFSET 0x0

#define NE6X_FW_SIG_LENGTH    8

#define NE6X_FW_810_LDR_SIG	"NE6K810L"
#define NE6X_FW_810_APP_SIG	"NE6K810A"
#define NE6X_FW_807_APP_SIG	"NE6K807A"
#define NE6X_FW_803_APP_SIG	"NE6K803A"
#define NE6X_FW_803_LDR_SIG	"NE6K803L"
#define NE6X_FW_NP_APP_SIG	"NE6KNPV1"
#define NE6X_FW_TBL_SIG		"NE6KTBL*"
#define NE6X_FW_PXE_SIG		"NE6KPXE*"
#define NE6X_FW_FRU_SIG		"NE6KFRU*"

struct ne6x_fw_common_header {
	u8 signature[NE6X_FW_SIG_LENGTH];
	u32 version;
	u32 length;
	u32 sections;
	u32 sect_start_addr;
	u32 type;
	u32 build_date;
	u8  reserved[16];
	u8 fw_ver[8];
	u32 package_crc;
	u32 header_crc;
}; /* 64B */

struct ne6x_fw_np_iwidth {
	char sig[4];
	u16 width;
	u16 ocp;
}; /* 8B */

struct ne6x_fw_np_isad {
	char sig[4];
	u32 isa_id;

	struct ne6x_fw_np_iwidth fp;
	struct ne6x_fw_np_iwidth dp;
	struct ne6x_fw_np_iwidth rp;
}; /* 32B */

struct ne6x_fw_np_atd {
	char sig[4];
	u32 at_id;

	struct ne6x_fw_np_iwidth te;
}; /* 16B */

struct ne6x_fw_np_header {
	char signature[NE6X_FW_SIG_LENGTH];
	u32 hdr_version;
	u32 hdr_length;

	u32 rsvd;
	u32 build_date;
	u32 img_version;
	u32 img_length;

	u32 npc_cnt;
	u32 npc_offset;
	u32 isa_cnt;
	u32 isa_offset;

	u32 at_cnt;
	u32 at_offset;
	u32 atd_cnt;
	u32 atd_offset;

	struct ne6x_fw_np_isad ISA[1];

	struct ne6x_fw_np_atd ATD[1];

	u32 cipher;     /* For future use */
	u32 comp;       /* For future use */
	u32 pkg_crc;
	u32 hdr_crc;
}; /* 128 B */

#define CRC32_REVERSED_POLYNOMIAL   0xEDB88320U
#define CRC32_INITIAL_REMAINDER     0xFFFFFFFFU
#define CRC32_FINALIZE_REMAINDER    0xFFFFFFFFU
#define CRC32_TABLE_SIZE            256U

enum {
	NE6X_FW_NOT_SUPPORT    = -1,
	NE6X_FW_HEADER_CRC_ERR = -2,
	NE6X_FW_LENGTH_ERR     = -3,
	NE6X_FW_PKG_CRC_ERR    = -4,
};

struct ne6x_key_filter {
	struct list_head list;
	struct ne6x_key key;
	struct {
		u8 is_new_key : 1;	/* filter is new, wait for PF answer */
		u8 remove     : 1;	/* filter needs to be removed */
		u8 add        : 1;	/* filter needs to be added */
		u8 padding    : 5;
		u8 refcnt;
	};
};

struct ne6x_vlan_filter {
	struct list_head list;
	struct ne6x_vlan vlan;
	struct {
		u8 is_new_vlan : 1;	/* filter is new, wait for PF answer */
		u8 remove      : 1;	/* filter needs to be removed */
		u8 add         : 1;	/* filter needs to be added */
		u8 padding     : 5;
		u8 refcnt;
	};
};

enum {
	NE6X_METER_SUBSET0 = 0x0,
	NE6X_METER_SUBSET1,
	NE6X_METER_SUBSET2,
	NE6X_METER_SUBSET3,
	NE6X_METER_SUBSET4,
	NE6X_METER_SUBSET5,
	NE6X_METER_SUBSET6,
	NE6X_METER_SUBSET7,
	NE6X_METER_SUBSET8,
	NE6X_METER_SUBSET9,
	NE6X_METER_SUBSET10,
	NE6X_METER_SUBSET11,
	NE6X_METER_SUBSET12,
	NE6X_METER_SUBSET13,
	NE6X_METER_SUBSET14,
	NE6X_METER_SUBSET15,
};

#define NE6X_METER0_TABLE			0x00000000U
#define NE6X_METER1_TABLE			0x80000000U
#define NE6X_METER_SUBSET(n)			(((n) & 0xf) << 27)

struct vf_stat {
	u64 rx_drop_pkts;
	u64 rx_broadcast_pkts;
	u64 rx_multicast_pkts;
	u64 rx_unicast_pkts;
	u64 tx_broadcast_pkts;
	u64 tx_multicast_pkts;
	u64 tx_unicast_pkts;
	u64 rx_malform_pkts;
	u64 tx_malform_pkts;
};

enum ne6x_fec_state {
	NE6X_FEC_NONE,
	NE6X_FEC_RS,
	NE6X_FEC_BASER,
	NE6X_FEC_AUTO,
};

int ne6x_dev_init(struct ne6x_pf *pf);
int ne6x_dev_get_port_num(struct ne6x_pf *pf);
int ne6x_dev_get_mac_addr(struct ne6x_adapter *adpt, u8 *mac);
int ne6x_dev_get_mac_stats(struct ne6x_adapter *adpt);
int ne6x_dev_get_link_status(struct ne6x_adapter *adpt, struct ne6x_link_info *status);
int ne6x_dev_set_speed(struct ne6x_adapter *adpt, u32 speed);
int ne6x_dev_set_sfp_speed(struct ne6x_adapter *adpt, u32 speed);
int ne6x_dev_get_sfp_speed(struct ne6x_adapter *adpt, u32 *speed);

int ne6x_dev_reset_firmware(struct ne6x_adapter *adpt);

int ne6x_dev_self_test_link(struct ne6x_adapter *adpt, int *verify);

u32 ne6x_dev_get_features(struct ne6x_adapter *adpt);
int ne6x_dev_set_features(struct ne6x_adapter *adpt, u32 value);

int ne6x_dev_set_mtu(struct ne6x_adapter *adpt, u32 mtu);
int ne6x_dev_get_mtu(struct ne6x_adapter *adpt, u32 *mtu);

void ne6x_dev_clear_vport(struct ne6x_pf *pf);
void ne6x_dev_set_port2pi(struct ne6x_adapter *adpt);
void ne6x_dev_set_pi2port(struct ne6x_adapter *adpt);
int ne6x_dev_set_vport(struct ne6x_adapter *adpt);

int ne6x_dev_set_vlan_port(struct ne6x_adapter *adpt, u16 vlan_id, pbmp_t pbmp);
int ne6x_dev_get_vlan_port(struct ne6x_adapter *adpt, u16 vlan_id, pbmp_t pbmp);
int ne6x_dev_vlan_add(struct ne6x_adapter *adpt, struct ne6x_vlan *vlan);
int ne6x_dev_vlan_del(struct ne6x_adapter *adpt, struct ne6x_vlan *vlan);
int ne6x_dev_add_vf_qinq(struct ne6x_vf *vf, __be16 proto, u16 vid);
int ne6x_dev_del_vf_qinq(struct ne6x_vf *vf, __be16 proto, u16 vid);
int ne6x_dev_clear_vlan_map(struct ne6x_pf *pf);

int ne6x_dev_set_rss(struct ne6x_adapter *adpt, struct ne6x_rss_info *info);

int ne6x_dev_get_flowctrl(struct ne6x_adapter *adpt, struct ne6x_flowctrl *fctrl);
int ne6x_dev_set_flowctrl(struct ne6x_adapter *adpt, struct ne6x_flowctrl *fctrl);
int ne6x_dev_get_port_fec(struct ne6x_adapter *adpt, int *status);

int ne6x_dev_write_eeprom(struct ne6x_adapter *adpt, int offset, u8 *pbuf, int size);
int ne6x_dev_read_eeprom(struct ne6x_adapter *adpt, int offset, u8 *pbuf, int size);

int ne6x_dev_clear_stats(struct ne6x_adapter *adpt);

int ne6x_dev_get_port_fec(struct ne6x_adapter *adpt, int *status);

int ne6x_dev_set_uc_promiscuous_enable(struct ne6x_adapter *adpt, int enable);
int ne6x_dev_set_mc_promiscuous_enable(struct ne6x_adapter *adpt, int enable);

int ne6x_dev_set_fec(struct ne6x_adapter *adpt, enum ne6x_fec_state fec);
int ne6x_dev_get_fec(struct ne6x_adapter *adpt, enum ne6x_fec_state *fec);

int ne6x_dev_add_unicast(struct ne6x_adapter *adpt, u8 *mac);
int ne6x_dev_del_unicast(struct ne6x_adapter *adpt, u8 *mac);

int ne6x_dev_add_multicast(struct ne6x_adapter *adpt, u8 *mac);
int ne6x_dev_del_multicast(struct ne6x_adapter *adpt, u8 *mac);
int ne6x_dev_enable_rxhash(struct ne6x_adapter *adpt, int enable);
int ne6x_dev_read_qsfp(struct ne6x_adapter *adpt, u8 regaddr, u8 *data, int len);

int ne6x_dev_upgrade_firmware(struct ne6x_adapter *adpt, u8 region, u8 *data, int size, int flags);

int ne6x_dev_get_sfp_type_len(struct ne6x_adapter *adpt, struct ne6x_sfp_mod_type_len *sfp_mode);

int ne6x_dev_get_sfp_eeprom(struct ne6x_adapter *adpt, u8 *data, int offset, int size, int flags);

int ne6x_dev_set_nic_stop(struct ne6x_pf *pf, u32 flag);
int ne6x_dev_set_nic_start(struct ne6x_pf *pf, u32 flag);
int ne6x_dev_get_temperature_info(struct ne6x_pf *pf, struct ne6x_soc_temperature *temp);
int ne6x_dev_get_power_consum(struct ne6x_pf *pf, struct ne6x_soc_power *power);
int ne6x_dev_get_fru(struct ne6x_pf *pf, u32 *buffer, u32 size);
int ne6x_dev_start_ddr_test(struct ne6x_pf *pf);
int ne6x_dev_i2c3_signal_test(struct ne6x_pf *pf, u32 *id);

int ne6x_dev_set_if_state(struct ne6x_adapter *adpt, u32 state);
int ne6x_dev_get_if_state(struct ne6x_adapter *adpt, u32 *state);

int ne6x_dev_get_sfp_status(struct ne6x_adapter *adpt, u8 *status);

int ne6x_dev_set_led(struct ne6x_adapter *adpt, bool state);
int ne6x_dev_get_vf_stat(struct ne6x_adapter *adpt, struct vf_stat *stat);
int ne6x_dev_reset_vf_stat(struct ne6x_adapter *adpt);
int ne6x_dev_check_speed(struct ne6x_adapter *adpt, u32 speed);

int ne6x_reg_table_update(struct ne6x_pf *pf, enum ne6x_reg_table table, u32 index,
			  u32 *data, int size);

int ne6x_dev_set_fw_lldp(struct ne6x_adapter *adpt, bool state);

int ne6x_dev_set_vf_bw(struct ne6x_adapter *adpt, int tx_rate);

int ne6x_dev_test_loopback(struct ne6x_adapter *adpt);
int ne6x_dev_test_reg(struct ne6x_adapter *adpt);
int ne6x_dev_test_intr(struct ne6x_adapter *adpt);
int ne6x_dev_set_port_mac(struct ne6x_adapter *adpt, u8 *data);
int ne6x_dev_add_broadcast_leaf(struct ne6x_adapter *adpt);
int ne6x_dev_del_broadcast_leaf(struct ne6x_adapter *adpt);
int ne6x_dev_validate_fw(const u8 *data, const u32 size, int *region);

int ne6x_dev_set_tx_rx_state(struct ne6x_adapter *adpt, int tx_state, int rx_state);
int ne6x_dev_set_fast_mode(struct ne6x_pf *pf, bool is_fast_mode, u8 num_queue);
int ne6x_dev_add_unicast_for_fastmode(struct ne6x_adapter *adpt, u8 *mac);

int ne6x_dev_get_dump_data_len(struct ne6x_pf *pf, u32 *size);
int ne6x_dev_get_dump_data(struct ne6x_pf *pf, u32 *data, u32 size);
int ne6x_dev_set_white_list(struct ne6x_pf *pf, bool enable);
void ne6x_dev_set_ddos(struct ne6x_pf *pf, bool enable);
int ne6x_dev_get_pport(struct ne6x_adapter *adpt);
int ne6x_dev_set_norflash_write_protect(struct ne6x_pf *pf, u32 write_protect);
int ne6x_dev_get_norflash_write_protect(struct ne6x_pf *pf, u32 *p_write_protect);

u32 ne6x_dev_crc32(const u8 *buf, u32 size);
void ne6x_dev_set_trust_vlan(struct ne6x_pf *pf, bool enable);
bool ne6x_dev_get_trust_vlan(struct ne6x_pf *pf);

#endif

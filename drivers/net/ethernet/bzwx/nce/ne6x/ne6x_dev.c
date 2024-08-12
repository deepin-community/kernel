// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include "ne6x.h"
#include "ne6x_portmap.h"
#include "ne6x_reg.h"
#include "ne6x_dev.h"
#include "reg.h"

#define NE6X_SDK_CRC32_DATA_LEN 256

#define NE6X_PPORT_BY_HWINFO(HWINFO, index) (((HWINFO) >> (8 * (index))) & 0xff)

#define to_be32_vector(s, e, p) \
({ \
	int __n; \
	u32 *__data = (u32 *)(p);\
	for (__n = (s); __n < (e); __n++) \
		__data[__n] = cpu_to_be32(__data[__n]); \
})

void ext_toeplitz_key(const unsigned char *key, unsigned char *ext_key)
{
	int i;

	for (i = 0; i < 39; i++) {
		ext_key[i] = key[i];
		ext_key[44 + i] = (key[i] << 1) | (key[i + 1] >> 7);
		ext_key[44 * 2 + i] = (key[i] << 2) | (key[i + 1] >> 6);
		ext_key[44 * 3 + i] = (key[i] << 3) | (key[i + 1] >> 5);
		ext_key[44 * 4 + i] = (key[i] << 4) | (key[i + 1] >> 4);
		ext_key[44 * 5 + i] = (key[i] << 5) | (key[i + 1] >> 3);
		ext_key[44 * 6 + i] = (key[i] << 6) | (key[i + 1] >> 2);
		ext_key[44 * 7 + i] = (key[i] << 7) | (key[i + 1] >> 1);
	}

	ext_key[39] = key[39];
	ext_key[44 + 39] = (key[39] << 1) | (key[1] >> 7);
	ext_key[44 * 2 + 39] = (key[39] << 2) | (key[1] >> 6);
	ext_key[44 * 3 + 39] = (key[39] << 3) | (key[1] >> 5);
	ext_key[44 * 4 + 39] = (key[39] << 4) | (key[1] >> 4);
	ext_key[44 * 5 + 39] = (key[39] << 5) | (key[1] >> 3);
	ext_key[44 * 6 + 39] = (key[39] << 6) | (key[1] >> 2);
	ext_key[44 * 7 + 39] = (key[39] << 7) | (key[1] >> 1);

	for (i = 0; i < 4; i++) {
		ext_key[40 + i] = ext_key[i];
		ext_key[44 + 40 + i] = ext_key[44 + i];
		ext_key[44 * 2 + 40 + i] = ext_key[44 * 2 + i];
		ext_key[44 * 3 + 40 + i] = ext_key[44 * 3 + i];
		ext_key[44 * 4 + 40 + i] = ext_key[44 * 4 + i];
		ext_key[44 * 5 + 40 + i] = ext_key[44 * 5 + i];
		ext_key[44 * 6 + 40 + i] = ext_key[44 * 6 + i];
		ext_key[44 * 7 + 40 + i] = ext_key[44 * 7 + i];
	}
}

static u32 ne6x_dev_bitrev(u32 input, int bw)
{
	u32 var = 0;
	int i;

	for (i = 0; i < bw; i++) {
		if (input & 0x01)
			var |= 1 << (bw - 1 - i);

		input >>= 1;
	}

	return var;
}

void ne6x_dev_crc32_init(u32 poly, u32 *table)
{
	u32 c;
	int i, j;

	poly = ne6x_dev_bitrev(poly, 32);

	for (i = 0; i < NE6X_SDK_CRC32_DATA_LEN; i++) {
		c = i;
		for (j = 0; j < 8; j++) {
			if (c & 1)
				c = poly ^ (c >> 1);
			else
				c = c >> 1;
		}
		table[i] = c;
	}
}

u32 ne6x_dev_crc32(const u8 *buf, u32 size)
{
	u32 ne6x_sdk_crc32tab[NE6X_SDK_CRC32_DATA_LEN];
	u32 i, crc;

	ne6x_dev_crc32_init(0x4C11DB7, ne6x_sdk_crc32tab);
	crc = 0xFFFFFFFF;

	for (i = 0; i < size; i++)
		crc = ne6x_sdk_crc32tab[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);

	return crc ^ 0xFFFFFFFF;
}

int ne6x_dev_spd_verify(struct ne6x_dev_eeprom_info *spd_info)
{
	if (be32_to_cpu(spd_info->spd_verify_value) ==
	    ne6x_dev_crc32((const u8 *)spd_info, sizeof(*spd_info) - 4))
		return 0;

	return -EINVAL;
}

int ne6x_dev_get_eeprom(struct ne6x_pf *pf)
{
	int retry = 3;

	while (retry-- > 0) {
		ne6x_reg_e2prom_read(pf, 0x0, (u8 *)&pf->sdk_spd_info, sizeof(pf->sdk_spd_info));
		if (!ne6x_dev_spd_verify(&pf->sdk_spd_info))
			return 0;
	}

	memset(&pf->sdk_spd_info, 0, sizeof(pf->sdk_spd_info));

	return -EINVAL;
}

static int ne6x_dev_get_dev_info(struct ne6x_pf *pf)
{
	int ret;

	ret = ne6x_dev_get_eeprom(pf);
	if (!ret) {
		pf->dev_type = be16_to_cpu(pf->sdk_spd_info.product_mode);
		pf->hw_flag = be32_to_cpu(pf->sdk_spd_info.hw_flag);
		if (!pf->hw_flag)
			pf->hw_flag = 1;
	} else {
		dev_err(ne6x_pf_to_dev(pf), "get eeprom  fail\n");
	}

	return ret;
}

int ne6x_dev_set_white_list(struct ne6x_pf *pf, bool enable)
{
	u32 data;

	if (enable) {
		if (pf->hw_flag == 1 || pf->hw_flag == 2) {
			ne6x_reg_get_user_data(pf, NP_USER_DATA_HW_FLAGS, &data);
			data |= NE6X_F_WHITELIST_ENABLED;
			ne6x_reg_set_user_data(pf, NP_USER_DATA_HW_FLAGS, data);
		} else {
			dev_info(ne6x_pf_to_dev(pf), "hw not support white list func\n");
			return -ENOTSUPP;
		}
	} else {
		ne6x_reg_get_user_data(pf, NP_USER_DATA_HW_FLAGS, &data);
		data &= ~NE6X_F_WHITELIST_ENABLED;
		ne6x_reg_set_user_data(pf, NP_USER_DATA_HW_FLAGS, data);
	}

	return 0;
}

void ne6x_dev_set_ddos(struct ne6x_pf *pf, bool enable)
{
	u32 data;

	if (enable) {
		ne6x_reg_get_user_data(pf, NP_USER_DATA_HW_FLAGS, &data);
		data |= NE6X_F_DDOS_ENABLED;
		ne6x_reg_set_user_data(pf, NP_USER_DATA_HW_FLAGS, data);
	} else {
		ne6x_reg_get_user_data(pf, NP_USER_DATA_HW_FLAGS, &data);
		data &= ~NE6X_F_DDOS_ENABLED;
		ne6x_reg_set_user_data(pf, NP_USER_DATA_HW_FLAGS, data);
	}
}

void ne6x_dev_set_trust_vlan(struct ne6x_pf *pf, bool enable)
{
	u32 data;

	if (enable) {
		ne6x_reg_get_user_data(pf, NP_USER_DATA_HW_FLAGS, &data);
		data |= NE6X_F_TRUST_VLAN_ENABLED;
		ne6x_reg_set_user_data(pf, NP_USER_DATA_HW_FLAGS, data);
	} else {
		ne6x_reg_get_user_data(pf, NP_USER_DATA_HW_FLAGS, &data);
		data &= ~NE6X_F_TRUST_VLAN_ENABLED;
		ne6x_reg_set_user_data(pf, NP_USER_DATA_HW_FLAGS, data);
	}
}

bool ne6x_dev_get_trust_vlan(struct ne6x_pf *pf)
{
	u32 data;

	ne6x_reg_get_user_data(pf, NP_USER_DATA_HW_FLAGS, &data);
	if (data & NE6X_F_TRUST_VLAN_ENABLED)
		return true;
	return false;
}

int ne6x_dev_get_pport(struct ne6x_adapter *adpt)
{
	u32 lport_to_phy;

	if (!adpt)
		return 0;

	switch (adpt->back->dev_type) {
	case NE6000AI_2S_X16H_25G_N5:
		return adpt->idx;
	default:
		break;
	}

	lport_to_phy = adpt->back->sdk_spd_info.logic_port_to_phyical;

	return NE6X_PPORT_BY_HWINFO(be32_to_cpu(lport_to_phy), adpt->idx);
}

static void ne6x_dev_set_roce_icrc_offload(struct ne6x_pf *pf, bool enable)
{
	u32 data;

	if (enable) {
		ne6x_reg_get_user_data(pf, NP_USER_DATA_HW_FLAGS, &data);
		data |= NE6X_F_S_ROCE_ICRC_ENABLED;
		ne6x_reg_set_user_data(pf, NP_USER_DATA_HW_FLAGS, data);
	} else {
		ne6x_reg_get_user_data(pf, NP_USER_DATA_HW_FLAGS, &data);
		data &= ~NE6X_F_S_ROCE_ICRC_ENABLED;
		ne6x_reg_set_user_data(pf, NP_USER_DATA_HW_FLAGS, data);
	}
}

int ne6x_dev_init(struct ne6x_pf *pf)
{
	if (unlikely(ne6x_dev_get_dev_info(pf)))
		return -EINVAL;

	ne6x_reg_get_ver(pf, &pf->verinfo);
	ne6x_dev_clear_vport(pf);
	ne6x_dev_set_fast_mode(pf, false, 0);
	ne6x_dev_set_roce_icrc_offload(pf, true);

	return 0;
}

int ne6x_dev_get_mac_addr(struct ne6x_adapter *adpt, u8 *mac)
{
	struct ne6x_dev_eeprom_info *info = &adpt->back->sdk_spd_info;

	memset(mac, 0, 6);
	switch (adpt->idx) {
	case 0:
		ether_addr_copy(mac, &info->port_0_mac[0]);
		break;
	case 1:
		ether_addr_copy(mac, &info->port_1_mac[0]);
		break;
	case 2:
		ether_addr_copy(mac, &info->port_2_mac[0]);
		break;
	case 3:
		ether_addr_copy(mac, &info->port_3_mac[0]);
		break;
	default:
		return -1;
	}

	return 0;
}

int ne6x_dev_get_port_num(struct ne6x_pf *pf)
{
	return pf->sdk_spd_info.number_of_physical_controllers;
}

int ne6x_dev_get_temperature_info(struct ne6x_pf *pf, struct ne6x_soc_temperature *temp)
{
	return ne6x_reg_get_soc_info(pf, NE6X_SOC_TEMPERATURE, (u32 *)temp, sizeof(*temp));
}

int  ne6x_dev_get_power_consum(struct ne6x_pf *pf, struct ne6x_soc_power *power)
{
	return ne6x_reg_get_soc_info(pf, NE6X_SOC_POWER_CONSUM, (u32 *)power, sizeof(*power));
}

int  ne6x_dev_i2c3_signal_test(struct ne6x_pf *pf, u32 *id)
{
	return ne6x_reg_get_soc_info(pf, NE6X_SOC_I2C3_TEST, (u32 *)id, sizeof(u32));
}

int ne6x_dev_get_fru(struct ne6x_pf *pf, u32 *buffer, u32 size)
{
	return ne6x_reg_get_soc_info(pf, NE6X_SOC_FRU, buffer, size);
}

int ne6x_dev_start_ddr_test(struct ne6x_pf *pf)
{
	return ne6x_reg_get_soc_info(pf, NE6X_SOC_DDR_TEST, NULL, 0);
}

int ne6x_dev_read_eeprom(struct ne6x_adapter *adpt, int offset, u8 *pbuf, int size)
{
	return ne6x_reg_e2prom_read(adpt->back, offset, pbuf, size);
}

int ne6x_dev_write_eeprom(struct ne6x_adapter *adpt, int offset, u8 *pbuf, int size)
{
	return ne6x_reg_e2prom_write(adpt->back, offset, pbuf, size);
}

int ne6x_dev_get_link_status(struct ne6x_adapter *adpt, struct ne6x_link_info *status)
{
	u32 link_speed = ne6x_reg_apb_read(adpt->back, 0x2087FB00 + 4 * ADPT_LPORT(adpt));

	status->link = link_speed >> 16;
	status->speed = link_speed & 0xffff;

	return 0;
}

int ne6x_dev_get_sfp_status(struct ne6x_adapter *adpt, u8 *status)
{
	u32 sfp_state;

	sfp_state = ne6x_reg_apb_read(adpt->back, 0x2087FB40 + 4 * ADPT_LPORT(adpt));
	*status = sfp_state & 0x1;

	return 0;
}

void ne6x_dev_update_status(struct ne6x_pf *pf, struct ne6x_port_info *port, bool is_up)
{
	u32 speed = NE6X_LINK_SPEED_25GB;
	struct ne6x_phy_info  *phy = &port->phy;
	struct ne6x_link_status *link = &phy->link_info;

	if (!is_up) {
		link->phy_type_low = NE6X_PHY_TYPE_UNKNOWN;
		link->link_speed = speed;
		link->link_info &= ~NE6X_AQ_LINK_UP;
		phy->media_type = NE6X_MEDIA_UNKNOWN;
		return;
	}

	link->link_info |= NE6X_AQ_LINK_UP;
	switch (speed) {
	case NE6X_LINK_SPEED_10GB:
		link->phy_type_low = NE6X_PHY_TYPE_10GBASE;
		link->link_speed   = NE6X_LINK_SPEED_10GB;
		break;
	case NE6X_LINK_SPEED_25GB:
		link->phy_type_low = NE6X_PHY_TYPE_25GBASE;
		link->link_speed   = NE6X_LINK_SPEED_25GB;
		break;
	case NE6X_LINK_SPEED_40GB:
		link->phy_type_low = NE6X_PHY_TYPE_40GBASE;
		link->link_speed   = NE6X_LINK_SPEED_40GB;
		break;
	case NE6X_LINK_SPEED_100GB:
		link->phy_type_low = NE6X_PHY_TYPE_100GBASE;
		link->link_speed   = NE6X_LINK_SPEED_100GB;
		break;
	case NE6X_LINK_SPEED_200GB:
		link->phy_type_low = NE6X_PHY_TYPE_200GBASE;
		link->link_speed   = NE6X_LINK_SPEED_200GB;
		break;
	default:
		dev_warn(ne6x_pf_to_dev(pf), "Unrecognized link_speed (0x%x).\n", speed);
		break;
	}

	phy->media_type = NE6X_MEDIA_FIBER;
}

int ne6x_dev_self_test_link(struct ne6x_adapter *adpt, int *verify)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_LINK_STATUS, NE6X_TALK_GET,
				  ADPT_LPORT(adpt), (void *)verify, sizeof(int));
}

int ne6x_dev_reset_firmware(struct ne6x_adapter *adpt)
{
	return ne6x_reg_reset_firmware(adpt->back);
}

int ne6x_dev_get_speed(struct ne6x_adapter *adpt, u32 *speed)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_SPEED, NE6X_TALK_GET,
				  ADPT_LPORT(adpt), (void *)speed, sizeof(u32));
}

int ne6x_dev_set_speed(struct ne6x_adapter *adpt, u32 speed)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_SPEED, NE6X_TALK_SET,
				  ADPT_LPORT(adpt), (void *)&speed, sizeof(u32));
}

int ne6x_dev_get_flowctrl(struct ne6x_adapter *adpt, struct ne6x_flowctrl *fctrl)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_PAUSE, NE6X_TALK_GET,
				  ADPT_LPORT(adpt), (void *)fctrl, sizeof(fctrl));
}

int ne6x_dev_set_flowctrl(struct ne6x_adapter *adpt, struct ne6x_flowctrl *fctrl)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_PAUSE, NE6X_TALK_SET,
				  ADPT_LPORT(adpt), (void *)fctrl, sizeof(*fctrl));
}

int ne6x_dev_get_mac_stats(struct ne6x_adapter *adpt)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_STATS, NE6X_TALK_GET,
				  ADPT_LPORT(adpt), (void *)&adpt->stats, sizeof(adpt->stats));
}

int ne6x_dev_set_mtu(struct ne6x_adapter *adpt, u32 mtu)
{
	u32 max_length = mtu + 18;

	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_MAX_FRAME, NE6X_TALK_SET,
				  ADPT_LPORT(adpt), (void *)&max_length, sizeof(max_length));
}

int ne6x_dev_get_mtu(struct ne6x_adapter *adpt, u32 *mtu)
{
	u32 max_length;
	int ret;

	ret = ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_MAX_FRAME, NE6X_TALK_GET,
				 ADPT_LPORT(adpt), (void *)&max_length, sizeof(max_length));
	*mtu = max_length  - 18;

	return ret;
}

static int fastlog2(int x)
{
	int idx;

	for (idx = 31; idx >= 0; idx--) {
		if (x & (1 << idx))
			break;
	}

	return idx;
}

int ne6x_dev_set_rss(struct ne6x_adapter *adpt, struct ne6x_rss_info *cfg)
{
	struct rss_table rss;
	u32 *rss_data = (u32 *)&rss;
	int ret, i;

	memset(&rss, 0x00, sizeof(rss));
	rss.flag       = cpu_to_be32(0x01); /* valid bit */
	rss.hash_fun   = (cfg->hash_func << 24) & 0xFF000000;
	rss.hash_fun  |= (cfg->hash_type & 0xFFFFFF);
	rss.hash_fun   = cpu_to_be32(rss.hash_fun);
	rss.queue_base = cpu_to_be32(ADPT_VPORTCOS(adpt));
	rss.queue_def  = cpu_to_be16(0x0);
	rss.queue_size = cpu_to_be16(adpt->num_queue);
	rss.entry_num  = fastlog2(cfg->ind_table_size);
	rss.entry_num  = cpu_to_be16(rss.entry_num);
	rss.entry_size = cpu_to_be16(0x0);

	for (i = 0; i < cfg->ind_table_size; i++)
		rss.entry_data[i] = cfg->ind_table[i];

	ext_toeplitz_key(&cfg->hash_key[0], &rss.hash_key[0]);

	for (i = 0; i < 128; i++)
		rss_data[i] = cpu_to_be32(rss_data[i]);

	ret = ne6x_reg_table_write(adpt->back, NE6X_REG_RSS_TABLE, ADPT_VPORT(adpt),
				   (void *)&rss, sizeof(rss));
	return ret;
}

int ne6x_dev_upgrade_firmware(struct ne6x_adapter *adpt, u8 region, u8 *data, int size, int flags)
{
	int ret;

	clear_bit(NE6X_LINK_POOLING, adpt->back->state);
	ret = ne6x_reg_upgrade_firmware(adpt->back, region, data, size);
	set_bit(NE6X_LINK_POOLING, adpt->back->state);

	return ret;
}

int ne6x_dev_get_sfp_type_len(struct ne6x_adapter *adpt, struct ne6x_sfp_mod_type_len *sfp_mode)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_SFP_TYPE_LEN, NE6X_TALK_GET,
				  ADPT_LPORT(adpt), sfp_mode, sizeof(*sfp_mode));
}

int ne6x_dev_get_sfp_eeprom(struct ne6x_adapter *adpt, u8 *data, int offset, int size, int flags)
{
	return ne6x_reg_get_sfp_eeprom(adpt->back, ADPT_LPORT(adpt), data, offset, size);
}

int ne6x_dev_clear_stats(struct ne6x_adapter *adpt)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_STATS, NE6X_TALK_SET,
				  ADPT_LPORT(adpt), NULL, 0);
}

/* panel port mapped to logical port */
void ne6x_dev_set_port2pi(struct ne6x_adapter *adpt)
{
	u32 val = (ADPT_LPORT(adpt) << 24) | (ADPT_VPORT(adpt) << 16) |
			 (adpt->port_info->hw_queue_base + 160);

	ne6x_reg_set_user_data(adpt->back, (NP_USER_DATA_PORT2PI_0 + ADPT_PPORT(adpt)), val);
}

/* logical port mapped to panel port */
void ne6x_dev_set_pi2port(struct ne6x_adapter *adpt)
{
	ne6x_reg_set_user_data(adpt->back, (NP_USER_DATA_PI2PORT_0 + ADPT_LPORT(adpt)),
			       ADPT_PPORT(adpt));
}

/* clear vport map */
void ne6x_dev_clear_vport(struct ne6x_pf *pf)
{
	int idx;

	for (idx = 0; idx < 32; idx++)
		ne6x_reg_set_user_data(pf, (NP_USER_DATA_PORT_2_COS_0 + idx), 0);

	for (idx = 0; idx < 64; idx++)
		ne6x_reg_set_user_data(pf, (NP_USER_DATA_PORT_OLFLAGS_0 + idx), 0);
}

/* automatically generating vp_base_cos */
int ne6x_dev_set_vport(struct ne6x_adapter *adpt)
{
	u16 port = adpt->vport >> 1;
	u32 val = 0;

	ne6x_reg_get_user_data(adpt->back, (NP_USER_DATA_PORT_2_COS_0 + port), &val);

	/* pf base cos */
	if (adpt->vport & 0x1) {
		val &= 0xFFFF;
		val |= ((adpt->port_info->hw_queue_base + 160) << 16);
		ne6x_reg_set_user_data(adpt->back, (NP_USER_DATA_PORT_2_COS_0 + port), val);
	} else {
		val &= 0xFFFF0000;
		val |= (adpt->port_info->hw_queue_base + 160);
		ne6x_reg_set_user_data(adpt->back, (NP_USER_DATA_PORT_2_COS_0 + port), val);
	}

	return 0;
}

int ne6x_dev_get_vlan_port(struct ne6x_adapter *adpt, u16 vlan_id, pbmp_t pbmp)
{
	pbmp_t new_pbmp;
	int ret;

	PBMP_CLEAR(new_pbmp);
	ret = ne6x_reg_table_read(adpt->back, NE6X_REG_VLAN_TABLE,
				  ADPT_LPORT(adpt) * 4096 + (vlan_id & 0xFFF),
				  (void *)new_pbmp,
				  sizeof(pbmp_t));

	PBMP_DWORD_GET(pbmp, 0) = PBMP_DWORD_GET(new_pbmp, 3);
	PBMP_DWORD_GET(pbmp, 1) = PBMP_DWORD_GET(new_pbmp, 2);
	PBMP_DWORD_GET(pbmp, 2) = PBMP_DWORD_GET(new_pbmp, 1);
	PBMP_DWORD_GET(pbmp, 3) = PBMP_DWORD_GET(new_pbmp, 0);

	return ret;
}

int ne6x_dev_set_vlan_port(struct ne6x_adapter *adpt, u16 vlan_id, pbmp_t pbmp)
{
	pbmp_t new_pbmp;

	PBMP_CLEAR(new_pbmp);
	PBMP_DWORD_GET(new_pbmp, 0) = PBMP_DWORD_GET(pbmp, 3);
	PBMP_DWORD_GET(new_pbmp, 1) = PBMP_DWORD_GET(pbmp, 2);
	PBMP_DWORD_GET(new_pbmp, 2) = PBMP_DWORD_GET(pbmp, 1);
	PBMP_DWORD_GET(new_pbmp, 3) = PBMP_DWORD_GET(pbmp, 0);

	return ne6x_reg_table_write(adpt->back, NE6X_REG_VLAN_TABLE,
				   ADPT_LPORT(adpt) * 4096 + (vlan_id & 0xFFF),
				   (void *)new_pbmp, sizeof(pbmp_t));
}

int ne6x_dev_vlan_add(struct ne6x_adapter *adpt, struct ne6x_vlan *vlan)
{
	pbmp_t pbmp, new_pbmp;
	u16 index = 0;

	if (vlan->tpid == ETH_P_8021Q)
		index = ADPT_LPORT(adpt) * 4096;
	else if (vlan->tpid == ETH_P_8021AD)
		index = 4 * 4096 + ADPT_LPORT(adpt) * 4096;

	memset(pbmp, 0, sizeof(pbmp_t));
	memset(new_pbmp, 0, sizeof(pbmp_t));

	ne6x_reg_table_read(adpt->back, NE6X_REG_VLAN_TABLE, index + (vlan->vid & 0xFFF),
			    (void *)&new_pbmp, sizeof(pbmp));
	PBMP_DWORD_GET(pbmp, 0) = PBMP_DWORD_GET(new_pbmp, 3);
	PBMP_DWORD_GET(pbmp, 1) = PBMP_DWORD_GET(new_pbmp, 2);
	PBMP_DWORD_GET(pbmp, 2) = PBMP_DWORD_GET(new_pbmp, 1);
	PBMP_DWORD_GET(pbmp, 3) = PBMP_DWORD_GET(new_pbmp, 0);

	memset(new_pbmp, 0, sizeof(pbmp));

	PBMP_PORT_ADD(pbmp, adpt->vport);

	PBMP_DWORD_GET(new_pbmp, 0) = PBMP_DWORD_GET(pbmp, 3);
	PBMP_DWORD_GET(new_pbmp, 1) = PBMP_DWORD_GET(pbmp, 2);
	PBMP_DWORD_GET(new_pbmp, 2) = PBMP_DWORD_GET(pbmp, 1);
	PBMP_DWORD_GET(new_pbmp, 3) = PBMP_DWORD_GET(pbmp, 0);

	ne6x_reg_table_write(adpt->back, NE6X_REG_VLAN_TABLE, index + (vlan->vid & 0xFFF),
			     (void *)&new_pbmp, sizeof(pbmp));

	return 0;
}

int ne6x_dev_vlan_del(struct ne6x_adapter *adpt, struct ne6x_vlan *vlan)
{
	pbmp_t pbmp, new_pbmp;
	u16 index = 0;

	if (vlan->tpid == ETH_P_8021Q)
		index = ADPT_LPORT(adpt) * 4096;
	else if (vlan->tpid == ETH_P_8021AD)
		index = 4 * 4096 + ADPT_LPORT(adpt) * 4096;

	memset(pbmp, 0, sizeof(pbmp));
	memset(new_pbmp, 0, sizeof(pbmp));

	ne6x_reg_table_read(adpt->back, NE6X_REG_VLAN_TABLE, index + (vlan->vid & 0xFFF),
			    (void *)&new_pbmp, sizeof(pbmp));

	PBMP_DWORD_GET(pbmp, 0) = PBMP_DWORD_GET(new_pbmp, 3);
	PBMP_DWORD_GET(pbmp, 1) = PBMP_DWORD_GET(new_pbmp, 2);
	PBMP_DWORD_GET(pbmp, 2) = PBMP_DWORD_GET(new_pbmp, 1);
	PBMP_DWORD_GET(pbmp, 3) = PBMP_DWORD_GET(new_pbmp, 0);

	memset(new_pbmp, 0, sizeof(pbmp));

	PBMP_PORT_REMOVE(pbmp, adpt->vport);

	PBMP_DWORD_GET(new_pbmp, 0) = PBMP_DWORD_GET(pbmp, 3);
	PBMP_DWORD_GET(new_pbmp, 1) = PBMP_DWORD_GET(pbmp, 2);
	PBMP_DWORD_GET(new_pbmp, 2) = PBMP_DWORD_GET(pbmp, 1);
	PBMP_DWORD_GET(new_pbmp, 3) = PBMP_DWORD_GET(pbmp, 0);

	ne6x_reg_table_write(adpt->back, NE6X_REG_VLAN_TABLE, index + (vlan->vid & 0xFFF),
			     (void *)&new_pbmp, sizeof(pbmp));

	return 0;
}

/* clear vlan table */
int ne6x_dev_clear_vlan_map(struct ne6x_pf *pf)
{
	pbmp_t pbmp;
	int index;

	PBMP_CLEAR(pbmp);
	for (index = 0; index < 8192; index++)
		ne6x_reg_table_write(pf, NE6X_REG_VLAN_TABLE, index, (void *)pbmp, sizeof(pbmp));

	return 0;
}

/* port add qinq */
int ne6x_dev_add_vf_qinq(struct ne6x_vf *vf, __be16 proto, u16 vid)
{
	struct ne6x_vf_vlan vlan;
	u32 val = 0;

	memset(&vlan, 0, sizeof(vlan));

	vlan.tpid = proto;
	vlan.vid = vid;

	memcpy(&val, &vlan, sizeof(u32));
	ne6x_reg_set_user_data(vf->adpt->back, NP_USER_DATA_PORT0_QINQ + ADPT_VPORT(vf->adpt), val);

	return 0;
}

/* port del qinq */
int ne6x_dev_del_vf_qinq(struct ne6x_vf *vf, __be16 proto, u16 vid)
{
	ne6x_reg_set_user_data(vf->adpt->back, NP_USER_DATA_PORT0_QINQ + ADPT_VPORT(vf->adpt), 0);

	return 0;
}

int ne6x_dev_set_uc_promiscuous_enable(struct ne6x_adapter *adpt, int enable)
{
	u32 val = 0;

	ne6x_reg_get_user_data(adpt->back, NP_USER_DATA_PORT_OLFLAGS_0 + ADPT_VPORT(adpt), &val);

	if (enable)
		val |= NE6X_F_PROMISC;
	else
		val &= ~NE6X_F_PROMISC;

	ne6x_reg_set_user_data(adpt->back, NP_USER_DATA_PORT_OLFLAGS_0 + ADPT_VPORT(adpt), val);

	return 0;
}

int ne6x_dev_set_mc_promiscuous_enable(struct ne6x_adapter *adpt, int enable)
{
	u32 val = 0;

	ne6x_reg_get_user_data(adpt->back, NP_USER_DATA_PORT_OLFLAGS_0 + ADPT_VPORT(adpt), &val);

	if (enable)
		val |= NE6X_F_RX_ALLMULTI;
	else
		val &= ~NE6X_F_RX_ALLMULTI;

	ne6x_reg_set_user_data(adpt->back, NP_USER_DATA_PORT_OLFLAGS_0 + ADPT_VPORT(adpt), val);

	return 0;
}

static void ne6x_dev_update_uc_leaf(struct l2fdb_dest_unicast *unicast, struct ne6x_adapter *adpt,
				    bool set_or_clear)
{
	u16 vport = ADPT_VPORT(adpt);

	set_or_clear ? SET_BIT(unicast->vp_bmp[vport / 32], vport % 32) :
		       CLR_BIT(unicast->vp_bmp[vport / 32], vport % 32);

	unicast->cnt = 0;
}

int ne6x_dev_add_unicast_for_fastmode(struct ne6x_adapter *adpt, u8 *mac)
{
	struct l2fdb_fast_table db;

	memcpy(&db.mac[0], mac, 6);
	db.start_cos = ADPT_VPORTCOS(adpt);
	db.cos_num   = adpt->num_queue;

	to_be32_vector(0, sizeof(db) / 4, &db);

	return ne6x_reg_set_unicast_for_fastmode(adpt->back, ADPT_VPORT(adpt),
						 (u32 *)&db, sizeof(db));
}

int ne6x_dev_add_unicast(struct ne6x_adapter *adpt, u8 *mac)
{
	struct l2fdb_search_result res;
	struct l2fdb_table db;
	u32 tid = 0xffffffff;
	int ret;

	if (adpt->back->is_fastmode)
		ne6x_dev_add_unicast_for_fastmode(adpt, mac);

	memset(&db, 0, sizeof(db));

	db.pport = ADPT_LPORT(adpt);
	memcpy(&db.mac[0], mac, 6);

	to_be32_vector(0, 16, &db);

	ret = ne6x_add_key(adpt, mac, 6);
	if (!ret) {
		memset(&db, 0, 128);
		memcpy(&db.mac[0], mac, 6);
		db.pport = ADPT_LPORT(adpt);
		db.vlanid = 0;

		memset(&db.fw_info.unicast, 0, sizeof(db.fw_info.unicast));
		db.fw_info.unicast.flags = 0x1;
		ne6x_dev_update_uc_leaf(&db.fw_info.unicast, adpt, true);

		to_be32_vector(0, 17, &db);

		ret = ne6x_reg_table_insert(adpt->back, NE6X_REG_L2FDB_TABLE,
					    (u32 *)&db, 128, &tid);
		if (ret)
			dev_err(ne6x_pf_to_dev(adpt->back),
				"insert unicast table  %x %02x %02x %02x %02x %02x %02x fail\n",
				ADPT_LPORT(adpt), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	} else {
		ret = ne6x_reg_table_search(adpt->back, NE6X_REG_L2FDB_TABLE,
					    (u32 *)&db, 64, (u32 *)&res, sizeof(res));
		db.fw_info.unicast.flags = 0x1;
		db.fw_info.unicast.vp_bmp[0] = res.fw_info.unicast.vp_bmp[0];
		db.fw_info.unicast.vp_bmp[1] = res.fw_info.unicast.vp_bmp[1];
		db.fw_info.unicast.vp_bmp[2] = res.fw_info.unicast.vp_bmp[2];
		db.fw_info.unicast.cnt = res.fw_info.unicast.cnt;
		ne6x_dev_update_uc_leaf(&db.fw_info.unicast, adpt, true);

		to_be32_vector(16, 17, &db);

		ret = ne6x_reg_table_update(adpt->back, NE6X_REG_L2FDB_TABLE,
					    res.key_index + 1, (u32 *)&db.fw_info, 64);
	}

	return 0;
}

static int ne6x_dev_del_unicast_for_fastmode(struct ne6x_adapter *adpt)
{
	struct l2fdb_fast_table db;

	memset(&db, 0, sizeof(db));

	return ne6x_reg_set_unicast_for_fastmode(adpt->back, ADPT_VPORT(adpt),
						 (u32 *)&db, sizeof(db));
}

int ne6x_dev_del_unicast(struct ne6x_adapter *adpt, u8 *mac)
{
	struct l2fdb_search_result res;
	struct l2fdb_table db;
	int ret = 0;

	if (adpt->back->is_fastmode)
		ne6x_dev_del_unicast_for_fastmode(adpt);

	ret = ne6x_del_key(adpt, mac, 6);

	memset(&db, 0, sizeof(db));

	db.pport = ADPT_LPORT(adpt);
	memcpy(&db.mac[0], mac, 6);

	to_be32_vector(0, 32, &db);

	ne6x_reg_table_search(adpt->back, NE6X_REG_L2FDB_TABLE,
			      (u32 *)&db, 64, (u32 *)&res, sizeof(res));

	memset(&db, 0, sizeof(db));
	memcpy(&db.mac[0], mac, 6);
	db.vlanid = 0;
	db.pport = ADPT_LPORT(adpt);
	db.fw_info.unicast.flags = 0x1;
	db.fw_info.unicast.vp_bmp[0] = res.fw_info.unicast.vp_bmp[0];
	db.fw_info.unicast.vp_bmp[1] = res.fw_info.unicast.vp_bmp[1];
	db.fw_info.unicast.vp_bmp[2] = res.fw_info.unicast.vp_bmp[2];
	db.fw_info.unicast.cnt       = res.fw_info.unicast.cnt;
	ne6x_dev_update_uc_leaf(&db.fw_info.unicast, adpt, false);

	to_be32_vector(0, 17, &db);

	if (!ret)
		ret = ne6x_reg_table_delete(adpt->back, NE6X_REG_L2FDB_TABLE, (u32 *)&db, 64);
	else
		ret = ne6x_reg_table_update(adpt->back, NE6X_REG_L2FDB_TABLE,
					    res.key_index + 1, (u32 *)&db.fw_info, 64);

	return 0;
}

static void ne6x_dev_update_mc_leaf(struct l2fdb_dest_multicast *multicast,
				    struct ne6x_adapter *adpt, bool set_or_clear)
{
	u16 vport = ADPT_VPORT(adpt);

	set_or_clear ? SET_BIT(multicast->vp_bmp[vport / 32], vport % 32) :
		       CLR_BIT(multicast->vp_bmp[vport / 32], vport % 32);
}

int ne6x_dev_add_multicast(struct ne6x_adapter *adpt, u8 *mac)
{
	struct l2fdb_search_result res;
	struct l2fdb_table db;
	u32 tid = 0xffffffff;
	int ret;

	memset(&db, 0, sizeof(db));

	db.pport = ADPT_LPORT(adpt);
	memcpy(&db.mac[0], mac, 6);

	to_be32_vector(0, 32, &db);

	ret = ne6x_add_key(adpt, mac, 6);
	if (!ret) {
		memset(&db, 0, sizeof(db));
		memcpy(&db.mac[0], mac, 6);
		db.pport = ADPT_LPORT(adpt);

		memset(&db.fw_info.multicast, 0, sizeof(db.fw_info.multicast));
		db.fw_info.multicast.flags = 0x3;
		ne6x_dev_update_mc_leaf(&db.fw_info.multicast, adpt, true);

		to_be32_vector(0, 17, &db);

		ret = ne6x_reg_table_insert(adpt->back, NE6X_REG_L2FDB_TABLE,
					    (u32 *)&db, 128, &tid);
		if (ret)
			dev_err(ne6x_pf_to_dev(adpt->back),
				"insert multicast table  %x %02x %02x %02x %02x %02x %02x fail\n",
				ADPT_LPORT(adpt), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	} else {
		ret = ne6x_reg_table_search(adpt->back, NE6X_REG_L2FDB_TABLE,
					    (u32 *)&db, 64, (u32 *)&res, sizeof(res));

		db.fw_info.multicast.flags = 0x3;
		db.fw_info.multicast.vp_bmp[0] = res.fw_info.multicast.vp_bmp[0];
		db.fw_info.multicast.vp_bmp[1] = res.fw_info.multicast.vp_bmp[1];
		db.fw_info.multicast.vp_bmp[2] = res.fw_info.multicast.vp_bmp[2];
		ne6x_dev_update_mc_leaf(&db.fw_info.multicast, adpt, true);

		to_be32_vector(16, 17, &db);

		ret = ne6x_reg_table_update(adpt->back, NE6X_REG_L2FDB_TABLE,
					    res.key_index + 1, (u32 *)&db.fw_info, 64);
	}

	return 0;
}

int ne6x_dev_del_multicast(struct ne6x_adapter *adpt, u8 *mac)
{
	struct l2fdb_search_result res;
	struct l2fdb_table db;
	int ret;

	ret = ne6x_del_key(adpt, mac, 6);

	memset(&db, 0, sizeof(db));

	/* hash_key */
	db.pport = ADPT_LPORT(adpt);
	memcpy(&db.mac[0], mac, 6);

	to_be32_vector(0, 32, &db);

	/* mac info */
	ne6x_reg_table_search(adpt->back, NE6X_REG_L2FDB_TABLE,
			      (u32 *)&db, 64, (u32 *)&res, sizeof(res));
	memset(&db, 0, 128);
	memcpy(&db.mac[0], mac, 6);
	db.vlanid = 0;
	db.pport = ADPT_LPORT(adpt);
	db.fw_info.multicast.flags = 0x3;
	db.fw_info.multicast.vp_bmp[0] = res.fw_info.multicast.vp_bmp[0];
	db.fw_info.multicast.vp_bmp[1] = res.fw_info.multicast.vp_bmp[1];
	db.fw_info.multicast.vp_bmp[2] = res.fw_info.multicast.vp_bmp[2];

	ne6x_dev_update_mc_leaf(&db.fw_info.multicast, adpt, false);

	to_be32_vector(0, 17, &db);

	if (!ret)
		ret = ne6x_reg_table_delete(adpt->back, NE6X_REG_L2FDB_TABLE, (u32 *)&db, 64);
	else
		ret = ne6x_reg_table_update(adpt->back, NE6X_REG_L2FDB_TABLE,
					    res.key_index + 1, (u32 *)&db.fw_info, 64);

	return ret;
}

inline void ne6x_dev_update_boradcast_leaf(u32 *leaf, struct ne6x_adapter *adpt, bool set_or_clear)
{
	u16 vport = ADPT_VPORT(adpt);

	set_or_clear ? SET_BIT(*leaf, vport % 32) : CLR_BIT(*leaf, vport % 32);
}

int ne6x_dev_add_broadcast_leaf(struct ne6x_adapter *adpt)
{
	u32 val = 0;

	ne6x_reg_get_user_data(adpt->back, (NP_USER_DATA_PI0_BROADCAST_LEAF +
			       ADPT_LPORT(adpt) * 4 + ADPT_VPORT(adpt) / 32), &val);
	ne6x_dev_update_boradcast_leaf(&val, adpt, true);
	ne6x_reg_set_user_data(adpt->back, (NP_USER_DATA_PI0_BROADCAST_LEAF +
			       ADPT_LPORT(adpt) * 4 + ADPT_VPORT(adpt) / 32), val);

	return 0;
}

int ne6x_dev_del_broadcast_leaf(struct ne6x_adapter *adpt)
{
	u32 val = 0;

	ne6x_reg_get_user_data(adpt->back, (NP_USER_DATA_PI0_BROADCAST_LEAF +
			       ADPT_LPORT(adpt) * 4 + ADPT_VPORT(adpt) / 32), &val);
	ne6x_dev_update_boradcast_leaf(&val, adpt, false);
	ne6x_reg_set_user_data(adpt->back, (NP_USER_DATA_PI0_BROADCAST_LEAF +
			       ADPT_LPORT(adpt) * 4 + ADPT_VPORT(adpt) / 32), val);

	return 0;
}

u32 ne6x_dev_get_features(struct ne6x_adapter *adpt)
{
	int val = 0;

	ne6x_reg_get_user_data(adpt->back, NP_USER_DATA_PORT_OLFLAGS_0 + ADPT_VPORT(adpt), &val);

	return val;
}

int ne6x_dev_set_features(struct ne6x_adapter *adpt, u32 val)
{
	ne6x_reg_set_user_data(adpt->back, NP_USER_DATA_PORT_OLFLAGS_0 + ADPT_VPORT(adpt), val);

	return 0;
}

int ne6x_dev_enable_rxhash(struct ne6x_adapter *adpt, int enable)
{
	u32 val = 0;

	ne6x_reg_get_user_data(adpt->back, NP_USER_DATA_PORT_OLFLAGS_0 + ADPT_VPORT(adpt), &val);
	if (enable)
		val |= NE6X_F_RSS;
	else
		val &= ~NE6X_F_RSS;

	ne6x_reg_set_user_data(adpt->back, NP_USER_DATA_PORT_OLFLAGS_0 + ADPT_VPORT(adpt), val);

	return 0;
}

int ne6x_dev_set_fec(struct ne6x_adapter *adpt, enum ne6x_fec_state fec)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_FEC, NE6X_TALK_SET,
				  ADPT_LPORT(adpt), (void *)&fec, sizeof(int));
}

int ne6x_dev_set_mac_inloop(struct ne6x_adapter *adpt, int enable)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_LOOPBACK, NE6X_TALK_SET,
				  ADPT_LPORT(adpt), (void *)&enable, sizeof(int));
}

int ne6x_dev_get_fec(struct ne6x_adapter *adpt, enum ne6x_fec_state *fec)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_FEC, NE6X_TALK_GET,
				  ADPT_LPORT(adpt), (void *)fec, sizeof(int));
}

int ne6x_dev_set_sfp_speed(struct ne6x_adapter *adpt, u32 speed)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_SFP_SPEED, NE6X_TALK_SET,
				  ADPT_LPORT(adpt), (void *)&speed, sizeof(u32));
}

int ne6x_dev_get_sfp_speed(struct ne6x_adapter *adpt, u32 *speed)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_SFP_SPEED, NE6X_TALK_GET,
				  ADPT_LPORT(adpt), (void *)speed, sizeof(u32));
}

int ne6x_dev_set_if_state(struct ne6x_adapter *adpt, u32 state)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_STATE, NE6X_TALK_SET,
				  ADPT_LPORT(adpt), (void *)&state, sizeof(u32));
}

int ne6x_dev_get_if_state(struct ne6x_adapter *adpt, u32 *state)
{
	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_STATE, NE6X_TALK_GET,
				  ADPT_LPORT(adpt), (void *)state, sizeof(u32));
}

int ne6x_dev_set_nic_stop(struct ne6x_pf *pf, u32 flag)
{
	return ne6x_reg_nic_stop(pf, flag);
}

int ne6x_dev_set_nic_start(struct ne6x_pf *pf, u32 flag)
{
	return ne6x_reg_nic_start(pf, flag);
}

int ne6x_dev_set_led(struct ne6x_adapter *adpt, bool state)
{
	return ne6x_reg_set_led(adpt->back, ADPT_LPORT(adpt), state);
}

void ne6x_dev_transform_vf_stat_format(u32 *stat_arr, struct vf_stat *stat)
{
	u32 start_pos = 0;

	stat->rx_malform_pkts   = ((u64)stat_arr[start_pos] << 32) + stat_arr[start_pos + 1];
	start_pos += 2;
	stat->rx_drop_pkts      = ((u64)stat_arr[start_pos] << 32) + stat_arr[start_pos + 1];
	start_pos += 2;
	stat->rx_broadcast_pkts = ((u64)stat_arr[start_pos] << 32) + stat_arr[start_pos + 1];
	start_pos += 2;
	stat->rx_multicast_pkts = ((u64)stat_arr[start_pos] << 32) + stat_arr[start_pos + 1];
	start_pos += 2;
	stat->rx_unicast_pkts   = ((u64)stat_arr[start_pos] << 32) + stat_arr[start_pos + 1];
	start_pos += 2;
	stat->tx_broadcast_pkts = ((u64)stat_arr[start_pos] << 32) + stat_arr[start_pos + 1];
	start_pos += 2;
	stat->tx_multicast_pkts = ((u64)stat_arr[start_pos] << 32) + stat_arr[start_pos + 1];
	start_pos += 2;
	stat->tx_unicast_pkts   = ((u64)stat_arr[start_pos] << 32) + stat_arr[start_pos + 1];
	start_pos += 16;
	stat->tx_malform_pkts   = ((u64)stat_arr[start_pos] << 32) + stat_arr[start_pos + 1];
}

int ne6x_dev_get_vf_stat(struct ne6x_adapter *adpt, struct vf_stat *stat)
{
	u32 stat_arr[64];
	int ret;

	ret = ne6x_reg_table_read(adpt->back, NE6X_REG_VF_STAT_TABLE, ADPT_VPORT(adpt),
				  (u32 *)&stat_arr[0], sizeof(stat_arr));
	ne6x_dev_transform_vf_stat_format(stat_arr, stat);

	return ret;
}

int ne6x_dev_reset_vf_stat(struct ne6x_adapter *adpt)
{
	u32 stat_arr[64] = {0};

	return ne6x_reg_table_write(adpt->back, NE6X_REG_VF_STAT_TABLE, ADPT_VPORT(adpt),
				   (u32 *)&stat_arr[0], sizeof(stat_arr));
}

int ne6x_dev_check_speed(struct ne6x_adapter *adpt, u32 speed)
{
	switch (adpt->back->dev_type) {
	case NE6000AI_2S_X16H_25G_N5:
	case NE6000AI_2S_X16H_25G_N6:
		if (speed == SPEED_25000 || speed == SPEED_10000)
			return 0;

		return -EOPNOTSUPP;
	case NE6000AI_2S_X16H_100G_N5:
		if (speed == SPEED_40000 || speed == SPEED_100000)
			return 0;

		return -EOPNOTSUPP;
	default:
		return -EOPNOTSUPP;
	}
}

int ne6x_dev_set_fw_lldp(struct ne6x_adapter *adpt, bool state)
{
	u32 val = 0;

	ne6x_reg_get_user_data(adpt->back, NP_USER_DATA_PORT_OLFLAGS_0 + ADPT_VPORT(adpt), &val);
	if (state)
		val |= NE6X_F_RX_FW_LLDP;
	else
		val &= ~NE6X_F_RX_FW_LLDP;

	ne6x_reg_set_user_data(adpt->back, NP_USER_DATA_PORT_OLFLAGS_0 + ADPT_VPORT(adpt), val);

	return 0;
}

#define NE6X_METER_STEP 152
#define NE6X_DF_METER_CBS_PBS (100 * 152)
int ne6x_dev_set_vf_bw(struct ne6x_adapter *adpt, int tx_rate)
{
	u32 val = 0, ret = 0;
	u32 cir = 0, cbs = 0;
	struct meter_table vf_bw;

	ne6x_reg_get_user_data(adpt->back, NP_USER_DATA_PORT_OLFLAGS_0 + ADPT_VPORT(adpt), &val);
	memset(&vf_bw, 0, sizeof(struct meter_table));

	if (tx_rate)
		val |= NE6X_F_TX_QOSBANDWIDTH;
	else
		val &= ~NE6X_F_TX_QOSBANDWIDTH;

	if (tx_rate) {
		cir = tx_rate;
		cbs = 0xffffff;
		vf_bw.pbs = cbs;
		vf_bw.cir = cir;
		vf_bw.cbs = cbs;
		vf_bw.pir = cir;
		ret = ne6x_reg_config_meter(adpt->back,
					    NE6X_METER0_TABLE |
					    NE6X_METER_SUBSET(NE6X_METER_SUBSET0) |
					    ADPT_VPORT(adpt),
					    (u32 *)&vf_bw, sizeof(vf_bw));
		ne6x_reg_set_user_data(adpt->back,
				       NP_USER_DATA_PORT_OLFLAGS_0 +
				       ADPT_VPORT(adpt),
				       val);
	} else {
		ne6x_reg_set_user_data(adpt->back,
				       NP_USER_DATA_PORT_OLFLAGS_0 +
				       ADPT_VPORT(adpt),
				       val);
		ret = ne6x_reg_config_meter(adpt->back,
					    NE6X_METER0_TABLE |
					    NE6X_METER_SUBSET(NE6X_METER_SUBSET0) |
					    ADPT_VPORT(adpt),
					    (u32 *)&vf_bw, sizeof(vf_bw));
	}

	return ret;
}

static int ne6x_dev_reg_pattern_test(struct ne6x_pf *pf, u32 reg, u32 val_arg)
{
	struct device *dev;
	u32 val, orig_val;

	orig_val = ne6x_reg_apb_read(pf, reg);
	dev = ne6x_pf_to_dev(pf);

	ne6x_reg_apb_write(pf, reg, val_arg);
	val = ne6x_reg_apb_read(pf, reg);
	if (val != val_arg) {
		dev_err(dev, "%s: reg pattern test failed - reg 0x%08x val 0x%08x\n",
			__func__, reg, val);
		return -1;
	}

	ne6x_reg_apb_write(pf, reg, orig_val);
	val = ne6x_reg_apb_read(pf, reg);
	if (val != orig_val) {
		dev_err(dev, "%s: reg restore test failed - reg 0x%08x orig 0x%08x val 0x%08x\n",
			__func__, reg, orig_val, val);
		return -1;
	}

	return 0;
}

#define NE6X_TEST_INT_SET_VALUE 0x1000000000000000   /* bit 60 */
int ne6x_dev_test_intr(struct ne6x_adapter *adpt)
{
	struct ne6x_pf *pf = adpt->back;
	struct ne6x_hw *hw = &pf->hw;
	int base = adpt->base_vector;
	union ne6x_vp_int vp_int;
	int ret = -1;

	if (base < NE6X_PF_VP0_NUM) {
		vp_int.val = rd64(hw, NE6X_VPINT_DYN_CTLN(base, NE6X_VP_INT));
		wr64(hw, NE6X_VPINT_DYN_CTLN(base, NE6X_VP_INT_SET),
		     NE6X_TEST_INT_SET_VALUE);
		vp_int.val = rd64(hw, NE6X_VPINT_DYN_CTLN(base, NE6X_VP_INT));
		if (vp_int.val & NE6X_TEST_INT_SET_VALUE) {
			ret = 0;
			vp_int.val &= ~NE6X_TEST_INT_SET_VALUE;
			wr64(hw, NE6X_VPINT_DYN_CTLN(base, NE6X_VP_INT), vp_int.val);
		}
	} else {
		vp_int.val = rd64_bar4(hw, NE6X_PFINT_DYN_CTLN(base - NE6X_PF_VP0_NUM,
							       NE6X_VP_INT));
		wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(base - NE6X_PF_VP0_NUM,
						  NE6X_VP_INT_SET),
			  NE6X_TEST_INT_SET_VALUE);
		vp_int.val = rd64_bar4(hw, NE6X_PFINT_DYN_CTLN(base - NE6X_PF_VP0_NUM,
							       NE6X_VP_INT));
		if (vp_int.val & NE6X_TEST_INT_SET_VALUE) {
			ret = 0;
			vp_int.val &= ~NE6X_TEST_INT_SET_VALUE;
			wr64_bar4(hw, NE6X_PFINT_DYN_CTLN(base - NE6X_PF_VP0_NUM,
							  NE6X_VP_INT), vp_int.val);
		}
	}

	return ret;
}

int ne6x_dev_test_reg(struct ne6x_adapter *adpt)
{
	struct ne6x_diag_reg_info test_reg[4] = {
		{0x20a00180, 0x5A5A5A5A},
		{0x20a00180, 0xA5A5A5A5},
		{0x20a00188, 0x00000000},
		{0x20a0018c, 0xFFFFFFFF}
	};
	u32 value, reg;
	int index;

	netdev_dbg(adpt->netdev, "Register test\n");
	for (index = 0; index < ARRAY_SIZE(test_reg); ++index) {
		value = test_reg[index].value;
		reg = test_reg[index].address;

		/* bail on failure (non-zero return) */
		if (ne6x_dev_reg_pattern_test(adpt->back, reg, value))
			return 1;
	}

	return 0;
}

#define NE6X_LOOP_TEST_TYPE 0x1234
/* handle hook packet */
int ne6x_dev_proto_recv(struct sk_buff *skb, struct net_device *dev,
			struct packet_type *ptype, struct net_device *ndev)
{
	struct ne6x_netdev_priv *np = netdev_priv(dev);
	struct ne6x_adapter *adpt = np->adpt;

	netdev_info(dev, "recv loopback test packet success!\n");
	adpt->recv_done = true;

	kfree_skb(skb);
	wake_up(&adpt->recv_notify);

	return 0;
}

static u8 loop_dst_mac[8] = {0x00, 0x00, 0x00, 0x11, 0x11, 0x01};
int ne6x_dev_proto_send(struct net_device *netdev, char *buf, int len)
{
	struct sk_buff *skb;
	u8 *pdata = NULL;
	u32 skb_len;

	skb_len = LL_RESERVED_SPACE(netdev) + len;
	skb = dev_alloc_skb(skb_len);
	if (!skb)
		return -1;

	skb_reserve(skb, LL_RESERVED_SPACE(netdev));
	skb->dev = netdev;
	skb->ip_summed = CHECKSUM_NONE;
	skb->priority = 0;
	pdata = skb_put(skb, len);
	if (pdata)
		memcpy(pdata, buf, len);

	/* send loop test packet */
	if (dev_queue_xmit(skb) < 0) {
		dev_put(netdev);
		kfree_skb(skb);
		netdev_err(netdev, "send pkt fail.\n");
		return -1;
	}
	netdev_info(netdev, "send loopback test packet success!\n");

	return 0;
}

int ne6x_dev_test_loopback(struct ne6x_adapter *adpt)
{
	struct packet_type prot_hook;
	struct ethhdr *ether_hdr;
	u32 old_value;
	int ret = 0;

	adpt->send_buffer = kzalloc(2048, GFP_KERNEL);
	if (!adpt->send_buffer)
		return -ENOMEM;

	/* config mac/pcs loopback */
	if (ne6x_dev_set_mac_inloop(adpt, true)) {
		netdev_err(adpt->netdev, "loopback test set_mac_inloop fail !\n");
		return -1;
	}

	old_value = ne6x_dev_get_features(adpt);
	ne6x_dev_set_uc_promiscuous_enable(adpt, true);
	memset(&prot_hook, 0, sizeof(struct packet_type));
	prot_hook.type = cpu_to_be16(NE6X_LOOP_TEST_TYPE);
	prot_hook.dev = adpt->netdev;
	prot_hook.func = ne6x_dev_proto_recv;
	dev_add_pack(&prot_hook);
	ether_hdr = (struct ethhdr *)adpt->send_buffer;
	memcpy(ether_hdr->h_source, &adpt->port_info->mac.perm_addr[0], ETH_ALEN);
	memcpy(ether_hdr->h_dest, loop_dst_mac, ETH_ALEN);
	ether_hdr->h_proto = cpu_to_be16(NE6X_LOOP_TEST_TYPE);
	adpt->send_buffer[14] = 0x45;
	ne6x_dev_proto_send(adpt->netdev, adpt->send_buffer, 1024);

	if (wait_event_interruptible_timeout(adpt->recv_notify, !!adpt->recv_done,
					     msecs_to_jiffies(2000)) <= 0) {
		netdev_info(adpt->netdev, "loopback test fail !\n");
		ret = -1;
	}

	adpt->recv_done = false;
	kfree(adpt->send_buffer);
	adpt->send_buffer = NULL;
	/* restore prosimc */
	ne6x_dev_set_features(adpt, old_value);
	dev_remove_pack(&prot_hook);
	if (ne6x_dev_set_mac_inloop(adpt, false)) {
		netdev_err(adpt->netdev, "loopback test cancel_mac_inloop fail\n");
		return -1;
	}

	return ret;
}

int ne6x_dev_set_port_mac(struct ne6x_adapter *adpt, u8 *data)
{
	u8 mac_info[8];

	memcpy(mac_info, data, 6);

	return ne6x_reg_talk_port(adpt->back, NE6X_MSG_PORT_INFO, NE6X_TALK_SET, ADPT_LPORT(adpt),
				 (void *)data, sizeof(mac_info));
}

static u32 crc_table[CRC32_TABLE_SIZE];  /* 1KB */
static void ne6x_dev_crc32_for_fw_init(void)
{
	u32 remainder;
	u32 dividend;
	s32 bit;

	for (dividend = 0U; dividend < CRC32_TABLE_SIZE; ++dividend) {
		remainder = dividend;
		for (bit = 8; bit > 0; --bit) {
			if ((remainder & 1U) != 0)
				remainder = (remainder >> 1) ^ CRC32_REVERSED_POLYNOMIAL;
			else
				remainder >>= 1;
		}

		crc_table[dividend] = remainder;
	}
}

static u32 ne6x_dev_crc32_for_fw(const void *message, u32 bytes)
{
	const u8 *buffer = (const u8 *)message;
	u32 remainder = CRC32_INITIAL_REMAINDER;
	u8 idx;

	ne6x_dev_crc32_for_fw_init();

	while (bytes-- > 0) {
		idx = (u8)(*buffer++ ^ remainder);
		remainder = crc_table[idx] ^ (remainder >> 8);
	}

	return remainder ^ CRC32_FINALIZE_REMAINDER;
}

static int ne6x_dev_get_fw_region(const u8 *data, u32 size, int *region)
{
	if (size < NE6X_FW_SIG_LENGTH)
		return NE6X_FW_NOT_SUPPORT;

	if (!memcmp(data, NE6X_FW_810_APP_SIG, NE6X_FW_SIG_LENGTH)) {
		*region = NE6X_ETHTOOL_FLASH_810_APP;
		return 0;
	} else if (!memcmp(data, NE6X_FW_NP_APP_SIG, NE6X_FW_SIG_LENGTH)) {
		*region = NE6X_ETHTOOL_FLASH_NP;
		return 0;
	} else if (!memcmp(data, NE6X_FW_PXE_SIG, NE6X_FW_SIG_LENGTH)) {
		*region = NE6X_ETHTOOL_FLASH_PXE;
		return 0;
	} else if (!memcmp(data, NE6X_FW_810_LDR_SIG, NE6X_FW_SIG_LENGTH)) {
		*region = NE6X_ETHTOOL_FLASH_810_LOADER;
		return 0;
	} else if (!memcmp(data, NE6X_FW_FRU_SIG, NE6X_FW_SIG_LENGTH)) {
		*region = NE6X_ETHTOOL_FRU;
		return 0;
	} else if (!memcmp(data, NE6X_FW_807_APP_SIG, NE6X_FW_SIG_LENGTH)) {
		*region = NE6X_ETHTOOL_FLASH_807_APP;
		return 0;
	} else {
		return NE6X_FW_NOT_SUPPORT;
	}
}

static int ne6x_dev_check_fw(const u8 *data, const u32 size, const int region)
{
	struct ne6x_fw_common_header *comm_hdr;
	struct ne6x_fw_np_header *np_hdr;
	u32 hcrc, pcrc, crc;

	switch (region) {
	case NE6X_ETHTOOL_FLASH_810_APP:
	case NE6X_ETHTOOL_FLASH_PXE:
	case NE6X_ETHTOOL_FLASH_810_LOADER:
	case NE6X_ETHTOOL_FLASH_807_APP:
		comm_hdr = (struct ne6x_fw_common_header *)&data[NE6X_FW_SIG_OFFSET];
		hcrc = comm_hdr->header_crc;
		pcrc = comm_hdr->package_crc;
		comm_hdr->header_crc = CRC32_INITIAL_REMAINDER;
		crc = ne6x_dev_crc32_for_fw(data, sizeof(*comm_hdr));
		if (crc != hcrc)
			return NE6X_FW_HEADER_CRC_ERR;

		if (comm_hdr->length != size)
			return NE6X_FW_LENGTH_ERR;

		comm_hdr->package_crc = CRC32_INITIAL_REMAINDER;
		comm_hdr->header_crc = CRC32_INITIAL_REMAINDER;
		crc = ne6x_dev_crc32_for_fw(data, comm_hdr->length);
		comm_hdr->package_crc = pcrc;
		comm_hdr->header_crc = hcrc;
		if (crc != pcrc)
			return NE6X_FW_PKG_CRC_ERR;

		break;
	case NE6X_ETHTOOL_FLASH_NP:
		np_hdr = (struct ne6x_fw_np_header *)&data[NE6X_FW_SIG_OFFSET];
		hcrc = np_hdr->hdr_crc;
		pcrc = np_hdr->pkg_crc;
		np_hdr->hdr_crc = CRC32_INITIAL_REMAINDER;
		crc = ne6x_dev_crc32_for_fw(data, sizeof(*np_hdr));
		if (crc != hcrc)
			return NE6X_FW_HEADER_CRC_ERR;

		if (np_hdr->img_length != size)
			return NE6X_FW_LENGTH_ERR;

		np_hdr->pkg_crc = CRC32_INITIAL_REMAINDER;
		np_hdr->hdr_crc = CRC32_INITIAL_REMAINDER;
		crc = ne6x_dev_crc32_for_fw(data, np_hdr->img_length);
		np_hdr->pkg_crc = pcrc;
		np_hdr->hdr_crc = hcrc;
		if (crc != pcrc)
			return NE6X_FW_PKG_CRC_ERR;

		break;
	}

	return 0;
}

int ne6x_dev_validate_fw(const u8 *data, const u32 size, int *region)
{
	if (ne6x_dev_get_fw_region(data, size, region))
		return NE6X_FW_NOT_SUPPORT;

	return ne6x_dev_check_fw(data, size, *region);
}

int ne6x_dev_set_tx_rx_state(struct ne6x_adapter *adpt, int tx_state, int rx_state)
{
	u32 value = ne6x_dev_get_features(adpt);

	if (tx_state)
		value &= ~NE6X_F_TX_DISABLE;
	else
		value |= NE6X_F_TX_DISABLE;

	if (rx_state)
		value &= ~NE6X_F_RX_DISABLE;
	else
		value |= NE6X_F_RX_DISABLE;

	ne6x_dev_set_features(adpt, value);

	return 0;
}

int ne6x_dev_set_fast_mode(struct ne6x_pf *pf, bool is_fast_mode, u8 number_queue)
{
	u32 mode;

	if (is_fast_mode) {
		mode = pf->num_alloc_vfs;
		mode |= 1 << 16;
		pf->is_fastmode = true;
	} else {
		mode = 0;
		pf->is_fastmode = false;
	}

	return ne6x_reg_set_user_data(pf, NP_USER_DATA_FAST_MODE, mode);
}

int ne6x_dev_get_dump_data_len(struct ne6x_pf *pf, u32 *size)
{
	return ne6x_reg_get_dump_data_len(pf, size);
}

int ne6x_dev_get_dump_data(struct ne6x_pf *pf, u32 *data, u32 size)
{
	return ne6x_reg_get_dump_data(pf, data, size);
}

int ne6x_dev_set_norflash_write_protect(struct ne6x_pf *pf, u32 write_protect)
{
	return ne6x_reg_set_norflash_write_protect(pf, write_protect);
}

int ne6x_dev_get_norflash_write_protect(struct ne6x_pf *pf, u32 *p_write_protect)
{
	return ne6x_reg_get_norflash_write_protect(pf, p_write_protect);
}

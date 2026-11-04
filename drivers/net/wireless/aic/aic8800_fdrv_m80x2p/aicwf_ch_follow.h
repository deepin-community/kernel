#ifndef _AICWF_CH_FOLLOW_H_
#define _AICWF_CH_FOLLOW_H_

#include "rwnx_main.h"

struct csa_ie {
	u8 eid;
	u8 len;
	u8 csm;
	u8 new_ch;
	u8 ch_sw_c;
};

struct ex_csa_ie {
	u8 eid;
	u8 len;
	u8 csm;
	u8 nrc;
	u8 new_ch;
	u8 ch_sw_c;
};

enum{
	BELOW,
	ABOVE
};

struct aicwf_ch_follow {
	char *bcn;//default ap bcn
	int bcn_len;//default ap bcn_len
	struct cfg80211_ap_settings settings;//default ap settings
	struct cfg80211_csa_settings csa_settings;//backup param
	//struct ieee80211_channel chan;
	int freq_expand_flag;
};

void aicwf_init_ap_bcn_info(struct rwnx_vif *rwnx_vif, char *bcn, int bcn_len, struct cfg80211_ap_settings *settings);
void aicwf_deinit_ap_bcn_info(struct rwnx_vif *rwnx_vif);
int aicwf_channel_switch(struct rwnx_hw *rwnx_hw, int freq);
int aicwf_find_sta_channel(struct rwnx_hw *rwnx_hw);
void aicwf_cfg80211_change_beacon_for_5g(struct rwnx_vif *vif, u8 *bcn, int bcn_len);

#endif


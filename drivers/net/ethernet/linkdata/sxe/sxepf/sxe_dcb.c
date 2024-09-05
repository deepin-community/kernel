
#include "sxe.h"

#ifdef SXE_DCB_CONFIGURE

#define SXE_TC_BWG_PERCENT_PER_CHAN		(12)	

void sxe_dcb_init(struct sxe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	struct sxe_dcb_cee_config *cee_cfg = &adapter->dcb_ctxt.cee_cfg;

	struct sxe_tc_config *tc;
	u32 tc_index;

	netdev->dcbnl_ops = &sxe_dcbnl_ops;

	cee_cfg->num_tcs.pg_tcs = MAX_TRAFFIC_CLASS;
	cee_cfg->num_tcs.pfc_tcs = MAX_TRAFFIC_CLASS;

	for (tc_index = 0; tc_index < MAX_TRAFFIC_CLASS; tc_index++) {
		tc = &cee_cfg->tc_config[tc_index];
		tc->channel[DCB_PATH_TX].bwg_id = 0;
		tc->channel[DCB_PATH_TX].bwg_percent = SXE_TC_BWG_PERCENT_PER_CHAN + (tc_index & 1);
		tc->channel[DCB_PATH_RX].bwg_id = 0;
		tc->channel[DCB_PATH_RX].bwg_percent = SXE_TC_BWG_PERCENT_PER_CHAN + (tc_index & 1);
		tc->pfc_type = pfc_disabled;
	}

	tc = &cee_cfg->tc_config[0];
	tc->channel[DCB_PATH_TX].up_to_tc_bitmap = 0xFF;
	tc->channel[DCB_PATH_RX].up_to_tc_bitmap = 0xFF;

	cee_cfg->bwg_link_percent[DCB_PATH_TX][0] = SXE_PERCENT_100;
	cee_cfg->bwg_link_percent[DCB_PATH_RX][0] = SXE_PERCENT_100;
	cee_cfg->pfc_mode_enable = false;
	adapter->dcb_ctxt.cee_cfg_bitmap = 0x00;
	adapter->dcb_ctxt.dcbx_cap = DCB_CAP_DCBX_HOST | DCB_CAP_DCBX_VER_CEE;

	memcpy(&adapter->dcb_ctxt.cee_temp_cfg, cee_cfg,
	       sizeof(adapter->dcb_ctxt.cee_temp_cfg));

	return;
}

static u32 sxe_dcb_min_credit_get(u32 max_frame)
{

	return ((max_frame / 2) + DCB_CREDIT_QUANTUM - 1) /
				DCB_CREDIT_QUANTUM;

}

static u16 sxe_dcb_cee_tc_link_percent_get(
			struct sxe_dcb_cee_config *cee_config,
			u8 direction, u8 tc_index)
{
	u8  bw_percent;
	u16 link_percentage;
	struct sxe_tc_bw_alloc *tc_info;

	tc_info = &cee_config->tc_config[tc_index].channel[direction];
	link_percentage =
		cee_config->bwg_link_percent[direction][tc_info->bwg_id];
	bw_percent = tc_info->bwg_percent;

	link_percentage = (link_percentage * bw_percent) / SXE_PERCENT_100;

	return link_percentage;
}

static u32 sxe_dcb_cee_min_link_percent_get(
			struct sxe_dcb_cee_config *cee_config, u8 direction)
{
	u8  tc_index;
	u16 link_percentage;
	u32 min_link_percent = SXE_PERCENT_100;

	for (tc_index = 0; tc_index < MAX_TRAFFIC_CLASS; tc_index++) {
		link_percentage = sxe_dcb_cee_tc_link_percent_get(
					cee_config, direction, tc_index);

		if (link_percentage && link_percentage < min_link_percent) {
			min_link_percent = link_percentage;
		}
	}

	return min_link_percent;
}

s32 sxe_dcb_cee_tc_credits_calculate(struct sxe_hw *hw,
				   struct sxe_dcb_cee_config *cee_config,
				   u32 max_frame, u8 direction)
{
	s32 ret = 0;
	struct sxe_adapter *adapter = hw->adapter;
	struct sxe_tc_bw_alloc *tc_info;
	u32 min_credit;
	u32 total_credit;
	u32 min_link_percent;
	u32 credit_refill;
	u32 credit_max;
	u16 link_percentage;
	u8  tc_index;

	if (!cee_config) {
		ret = -DCB_ERR_CONFIG;
		LOG_ERROR_BDF("config info is NULL\n");
		goto l_ret;
	}

	LOG_DEBUG_BDF("cee_config[%p] input max_frame[%u] direction[%s]\n",
			cee_config, max_frame, direction ? "RX" : "TX");

	min_credit = sxe_dcb_min_credit_get(max_frame);
	LOG_DEBUG_BDF("cee_config[%p] max_frame[%u] got min_credit[%u]\n",
			cee_config, max_frame, min_credit);

	min_link_percent = sxe_dcb_cee_min_link_percent_get(cee_config, direction);
	LOG_DEBUG_BDF("cee_config[%p] direction[%s] got min_link_percent[%u]\n",
			cee_config, direction ? "RX" : "TX", min_link_percent);

	total_credit = (min_credit / min_link_percent) + 1;
	LOG_DEBUG_BDF("cee_config[%p] total_credit=%u\n", cee_config, total_credit);

	for (tc_index = 0; tc_index < MAX_TRAFFIC_CLASS; tc_index++) {
		tc_info = &cee_config->tc_config[tc_index].channel[direction];

		link_percentage = sxe_dcb_cee_tc_link_percent_get(
					cee_config, direction, tc_index);
		LOG_DEBUG_BDF("tc[%u] bwg_percent=%u, link_percentage=%u\n",
			tc_index, tc_info->bwg_percent, link_percentage);

		if (tc_info->bwg_percent > 0 && link_percentage == 0) {
			link_percentage = 1;
		}

		tc_info->link_percent = (u8)link_percentage;

		credit_refill = min(link_percentage * total_credit,
				    (u32)MAX_CREDIT_REFILL);

		if (credit_refill < min_credit) {
			credit_refill = min_credit;
		}

		tc_info->data_credits_refill = (u16)credit_refill;
		LOG_DEBUG_BDF("tc[%u] credit_refill=%u\n",
					tc_index, credit_refill);

		credit_max = (link_percentage * MAX_CREDIT) / SXE_PERCENT_100;

		if (credit_max < min_credit) {
			credit_max = min_credit;
		}
		LOG_DEBUG_BDF("tc[%u] credit_max=%u\n",
					tc_index, credit_max);

		if (direction == DCB_PATH_TX) {
			cee_config->tc_config[tc_index].desc_credits_max =
				(u16)credit_max;
		}

		tc_info->data_credits_max = (u16)credit_max;
	}

l_ret:
	return ret;
}

void sxe_dcb_cee_pfc_parse(struct sxe_dcb_cee_config *cfg, u8 *pfc_en)
{
	u32 tc;
	struct sxe_tc_config *tc_config = &cfg->tc_config[0];

	for (*pfc_en = 0, tc = 0; tc < MAX_TRAFFIC_CLASS; tc++) {
		if (tc_config[tc].pfc_type != pfc_disabled) {
			*pfc_en |= BIT(tc);
		}
	}
	LOG_DEBUG("cfg[%p] pfc_en[0x%x]\n", cfg, *pfc_en);

	return;
}

void sxe_dcb_cee_refill_parse(struct sxe_dcb_cee_config *cfg,
					u8 direction, u16 *refill)
{
	u32 tc;
	struct sxe_tc_config *tc_config = &cfg->tc_config[0];

	for (tc = 0; tc < MAX_TRAFFIC_CLASS; tc++) {
		refill[tc] = tc_config[tc]. \
			channel[direction].data_credits_refill;
		LOG_DEBUG("tc[%u] --- refill[%u]\n", tc, refill[tc]);
	}

	return;
}

void sxe_dcb_cee_max_credits_parse(struct sxe_dcb_cee_config *cfg,
						u16 *max_credits)
{
	u32 tc;
	struct sxe_tc_config *tc_config = &cfg->tc_config[0];

	for (tc = 0; tc < MAX_TRAFFIC_CLASS; tc++) {
		max_credits[tc] = tc_config[tc].desc_credits_max;
		LOG_DEBUG("tc[%u] --- max_credits[%u]\n", tc, max_credits[tc]);
	}

	return;
}

void sxe_dcb_cee_bwgid_parse(struct sxe_dcb_cee_config *cfg,
					u8 direction, u8 *bwgid)
{
	u32 tc;
	struct sxe_tc_config *tc_config = &cfg->tc_config[0];

	for (tc = 0; tc < MAX_TRAFFIC_CLASS; tc++) {
		bwgid[tc] = tc_config[tc].channel[direction].bwg_id;
		LOG_DEBUG("tc[%u] --- bwgid[%u]\n", tc, bwgid[tc]);
	}

	return;
}

void sxe_dcb_cee_prio_parse(struct sxe_dcb_cee_config *cfg,
					u8 direction, u8 *ptype)
{
	u32 tc;
	struct sxe_tc_config *tc_config = &cfg->tc_config[0];

	for (tc = 0; tc < MAX_TRAFFIC_CLASS; tc++) {
		ptype[tc] = tc_config[tc].channel[direction].prio_type;
		LOG_DEBUG("tc[%u] --- ptype[%u]\n", tc, ptype[tc]);
	}

	return;
}

u8 sxe_dcb_cee_get_tc_from_up(struct sxe_dcb_cee_config *cfg,
					u8 direction, u8 up)
{
	struct sxe_tc_config *tc_config = &cfg->tc_config[0];
	u8 prio_mask = BIT(up);
	u8 tc = cfg->num_tcs.pg_tcs;

	if (!tc) {
		goto l_ret;
	}

	for (tc--; tc; tc--) {
		if (prio_mask & tc_config[tc].channel[direction].up_to_tc_bitmap) {
			break;
		}
	}

l_ret:
	LOG_DEBUG("up[%u] to tc[%u]\n", up, tc);
	return tc;
}

void sxe_dcb_cee_up2tc_map_parse(struct sxe_dcb_cee_config *cfg,
						u8 direction, u8 *map)
{
	u8 up;

	for (up = 0; up < MAX_USER_PRIORITY; up++) {
		map[up] = sxe_dcb_cee_get_tc_from_up(cfg, direction, up);
		LOG_DEBUG("up[%u] --- up2tc_map[%u]\n", up, map[up]);
	}

	return;
}

static void sxe_dcb_hw_cee_bw_alloc_configure(struct sxe_hw *hw,
					u16 *refill, u16 *max,
					u8 *bwg_id, u8 *prio_type,
					u8 *prio_tc)
{
	hw->dma.ops->dcb_rx_bw_alloc_configure(hw, refill, max, bwg_id,
					  prio_type, prio_tc, MAX_USER_PRIORITY);
	hw->dma.ops->dcb_tx_desc_bw_alloc_configure(hw, refill, max,
					       bwg_id, prio_type);
	hw->dma.ops->dcb_tx_data_bw_alloc_configure(hw, refill, max,
					       bwg_id, prio_type,
					       prio_tc, MAX_USER_PRIORITY);

	return;
}

static void sxe_dcb_hw_cee_non_bw_alloc_configure(struct sxe_hw *hw)
{
	hw->dma.ops->dcb_tc_stats_configure(hw);

	return;
}

static void sxe_dcb_hw_cee_configure(struct sxe_hw *hw,
			struct sxe_dcb_cee_config *dcb_config)
{
	u8 ptype[MAX_TRAFFIC_CLASS];
	u8 bwgid[MAX_TRAFFIC_CLASS];
	u8 prio_tc[MAX_TRAFFIC_CLASS];
	u16 refill[MAX_TRAFFIC_CLASS];
	u16 max[MAX_TRAFFIC_CLASS];

	sxe_dcb_cee_refill_parse(dcb_config, DCB_PATH_TX, refill);
	sxe_dcb_cee_max_credits_parse(dcb_config, max);
	sxe_dcb_cee_bwgid_parse(dcb_config, DCB_PATH_TX, bwgid);
	sxe_dcb_cee_prio_parse(dcb_config, DCB_PATH_TX, ptype);
	sxe_dcb_cee_up2tc_map_parse(dcb_config, DCB_PATH_TX, prio_tc);

	sxe_dcb_hw_cee_bw_alloc_configure(hw, refill, max,
					 bwgid, ptype, prio_tc);

	sxe_dcb_hw_cee_non_bw_alloc_configure(hw);

	return;
}

static void sxe_dcb_ieee_tc_credits_calculate(u8 *bw, u16 *refill,
			      u16 *max, u32 max_frame)
{
	u16 min_percent = 100;
	u32 min_credit, total_credits;
	u8 tc_index;

	min_credit = sxe_dcb_min_credit_get(max_frame);
	LOG_DEBUG("min_credit=%u, max_frame=%u\n", min_credit, max_frame);

	for (tc_index = 0; tc_index < MAX_TRAFFIC_CLASS; tc_index++) {
		if (bw[tc_index] < min_percent && bw[tc_index]) {
			min_percent = bw[tc_index];
		}
	}
	LOG_DEBUG("min_percent=%u\n", min_percent);

	total_credits = (min_credit / min_percent) + 1;
	LOG_DEBUG("total_credits=%u\n", total_credits);

	for (tc_index = 0; tc_index < MAX_TRAFFIC_CLASS; tc_index++) {
		u32 val = min(bw[tc_index] * total_credits, (u32)MAX_CREDIT_REFILL);

		if (val < min_credit) {
			val = min_credit;
		}

		refill[tc_index] = val;
		LOG_DEBUG("tc[%u] credits_refill=%u\n", tc_index, refill[tc_index]);

		max[tc_index] = bw[tc_index] ? (bw[tc_index] * MAX_CREDIT)/SXE_PERCENT_100 : min_credit;
		LOG_DEBUG("tc[%u] max_credits=%u\n", tc_index, max[tc_index]);
	}

	return;
}

void sxe_dcb_hw_ets_configure(struct sxe_hw *hw,
			    u16 *refill, u16 *max, u8 *bwg_id,
			    u8 *prio_type, u8 *prio_tc)
{
	hw->dma.ops->dcb_rx_bw_alloc_configure(hw, refill, max,
					  bwg_id, prio_type, prio_tc,
					  MAX_USER_PRIORITY);

	hw->dma.ops->dcb_tx_desc_bw_alloc_configure(hw, refill, max,
					       bwg_id, prio_type);
	hw->dma.ops->dcb_tx_data_bw_alloc_configure(hw, refill, max, bwg_id,
					       prio_type, prio_tc,
					       MAX_USER_PRIORITY);
	return;
}

s32 sxe_dcb_hw_ieee_ets_configure(struct sxe_hw *hw,
				struct ieee_ets *ets, u32 max_frame)
{
	u16 refill[IEEE_8021QAZ_MAX_TCS], max[IEEE_8021QAZ_MAX_TCS];
	u8 prio_type[IEEE_8021QAZ_MAX_TCS];
	u8 tc_index;
	s32 ret = 0;
	struct sxe_adapter *adapter = hw->adapter;

	u8 bwg_id[IEEE_8021QAZ_MAX_TCS] = {0, 1, 2, 3, 4, 5, 6, 7};

	for (tc_index = 0; tc_index < IEEE_8021QAZ_MAX_TCS; tc_index++) {
		switch (ets->tc_tsa[tc_index]) {
		case IEEE_8021QAZ_TSA_STRICT:
			prio_type[tc_index] = 2;
			break;
		case IEEE_8021QAZ_TSA_ETS:
			prio_type[tc_index] = 0;
			break;
		default:
			LOG_ERROR_BDF("unsupport tsa[%u]=%u\n",
					tc_index, ets->tc_tsa[tc_index]);
			ret = -EINVAL;
			goto l_ret;
		}
		LOG_DEBUG_BDF("tc[%u] prio_type=%u\n",tc_index, prio_type[tc_index]);
	}

	sxe_dcb_ieee_tc_credits_calculate(ets->tc_tx_bw, refill, max, max_frame);

	sxe_dcb_hw_ets_configure(hw, refill, max,
				       bwg_id, prio_type, ets->prio_tc);

l_ret:
	return ret;
}

void sxe_dcb_hw_pfc_configure(struct sxe_hw *hw, u8 pfc_en, u8 *prio_tc)
{
	hw->dma.ops->dcb_pfc_configure(hw, pfc_en, prio_tc, MAX_USER_PRIORITY);
	return;
}

void sxe_dcb_pfc_configure(struct sxe_adapter *adapter)
{
	u8 pfc_en = 0;
	u8 prio_tc[MAX_TRAFFIC_CLASS];

	struct sxe_dcb_cee_config *cee_cfg = &adapter->dcb_ctxt.cee_cfg;

	if (adapter->dcb_ctxt.dcbx_cap & DCB_CAP_DCBX_VER_CEE) {
		LOG_DEBUG_BDF("pfc in cee mode\n");
		sxe_dcb_cee_pfc_parse(cee_cfg, &pfc_en);
		sxe_dcb_cee_up2tc_map_parse(cee_cfg, DCB_PATH_TX, prio_tc);
	} else if (adapter->dcb_ctxt.ieee_ets && adapter->dcb_ctxt.ieee_pfc) {
		LOG_DEBUG_BDF("pfc in ieee mode\n");
		pfc_en = adapter->dcb_ctxt.ieee_pfc->pfc_en;
		memcpy(prio_tc, adapter->dcb_ctxt.ieee_ets->prio_tc,
			sizeof(prio_tc[0]) * MAX_TRAFFIC_CLASS);
	}

	if (pfc_en) {
		sxe_dcb_hw_pfc_configure(&adapter->hw, pfc_en, prio_tc);
	}

	return;
}

void sxe_dcb_configure(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	u32 max_frame = adapter->netdev->mtu + SXE_ETH_DEAD_LOAD;
	u16 rss = sxe_rss_num_get(adapter);

	if (!(adapter->cap & SXE_DCB_ENABLE)) {
		return;
	}

	if (adapter->dcb_ctxt.dcbx_cap & DCB_CAP_DCBX_VER_CEE) {
		LOG_DEBUG_BDF("dcb in cee mode\n");
		sxe_dcb_cee_tc_credits_calculate(hw, &adapter->dcb_ctxt.cee_cfg,
						max_frame, DCB_PATH_TX);
		sxe_dcb_cee_tc_credits_calculate(hw, &adapter->dcb_ctxt.cee_cfg,
						max_frame, DCB_PATH_RX);
		sxe_dcb_hw_cee_configure(hw, &adapter->dcb_ctxt.cee_cfg);
	} else if (adapter->dcb_ctxt.ieee_ets && adapter->dcb_ctxt.ieee_pfc) {
		LOG_DEBUG_BDF("dcb in ieee mode\n");
		sxe_dcb_hw_ieee_ets_configure(&adapter->hw,
				 adapter->dcb_ctxt.ieee_ets,
				 max_frame);
	}

	hw->dbu.ops->dcb_tc_rss_configure(hw, rss);

	return;
}

void sxe_dcb_exit(struct sxe_adapter* adapter)
{
	if (adapter->dcb_ctxt.ieee_pfc) {
		kfree(adapter->dcb_ctxt.ieee_pfc);
	}

	if (adapter->dcb_ctxt.ieee_ets) {
		kfree(adapter->dcb_ctxt.ieee_ets);
	}

	return;
}

#endif 

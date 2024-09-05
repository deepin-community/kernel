#include "sxe.h"

#ifdef SXE_DCB_CONFIGURE
#include <linux/dcbnl.h>
#include "sxe_phy.h"
#include "sxe_dcb.h"
#include "sxe_sriov.h"
#include "sxe_netdev.h"

#define BIT_PFC		0x02
#define BIT_PG_RX	0x04
#define BIT_PG_TX	0x08
#define BIT_APP_UPCHG	0x10

#define DCB_HW_CHG_RST  0  
#define DCB_NO_HW_CHG   1  
#define DCB_HW_CHG      2  

s32 sxe_dcb_tc_validate(struct sxe_adapter *adapter, u8 tc)
{
	s32 ret = 0;

	if (tc > adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs) {
		LOG_ERROR_BDF(" tc num [%u] is invalid, max tc num=%u\n",
			tc, adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs);
		ret = -EINVAL;
	}

	return ret;
}

static void sxe_prio_tc_map_set(struct sxe_adapter *adapter)
{
	u8 prio;
	u8 tc = 0;
	struct net_device *dev = adapter->netdev;
	struct sxe_dcb_cee_config *cee_cfg = &adapter->dcb_ctxt.cee_cfg;
	struct ieee_ets *ets = adapter->dcb_ctxt.ieee_ets;

	for (prio = 0; prio < MAX_USER_PRIORITY; prio++) {
		if (adapter->dcb_ctxt.dcbx_cap & DCB_CAP_DCBX_VER_CEE) {
			tc = sxe_dcb_cee_get_tc_from_up(cee_cfg,
					DCB_PATH_TX, prio);
		} else if (ets) {
			tc = ets->prio_tc[prio];
		}

		netdev_set_prio_tc_map(dev, prio, tc);
	}

	return ;
}

s32 sxe_dcb_tc_setup(struct sxe_adapter *adapter, u8 tc)
{
	s32 ret = 0;
	struct net_device *netdev = adapter->netdev;
	struct sxe_hw *hw = &adapter->hw;

	LOG_DEBUG_BDF("current dcb state=%x, tc_num=%u, cfg tc_num=%u\n",
		!!(adapter->cap & SXE_DCB_ENABLE), sxe_dcb_tc_get(adapter), tc);

	if (tc) {
		if (adapter->xdp_prog) {
			LOG_MSG_WARN(probe, "dcb is not supported with xdp\n");

			sxe_ring_irq_init(adapter);
			if (netif_running(netdev)) {
				sxe_open(netdev);
			}

			ret = -EINVAL;
			goto l_ret;
		}

		netdev_set_num_tc(netdev, tc);
		sxe_prio_tc_map_set(adapter);

		sxe_dcb_tc_set(adapter, tc);
		adapter->cap |= SXE_DCB_ENABLE;
		LOG_DEBUG_BDF("dcb enble, cfg tc_num=%u\n", tc);
	} else {
		netdev_reset_tc(netdev);

		adapter->cap &= ~SXE_DCB_ENABLE;
		sxe_dcb_tc_set(adapter, tc);

		adapter->dcb_ctxt.cee_temp_cfg.pfc_mode_enable = false;
		adapter->dcb_ctxt.cee_cfg.pfc_mode_enable = false;
		LOG_DEBUG_BDF("dcb disable, cfg tc_num=%u\n", tc);
	}

	hw->dma.ops->dcb_rx_up_tc_map_set(hw, tc);

l_ret:
	return ret;
}

static u8 sxe_dcbnl_state_get(struct net_device *netdev)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	LOG_DEBUG_BDF("dcb current state=%u\n", !!(adapter->cap & SXE_DCB_ENABLE));

	return (u8)(!!(adapter->cap & SXE_DCB_ENABLE));
}

static u8 sxe_dcbnl_state_set(struct net_device *netdev, u8 state)
{
	s32 ret = 1;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	if (!state == !(adapter->cap & SXE_DCB_ENABLE)) {
		LOG_INFO_BDF("dcb current state=%x, set state=%x, no change\n",
			!!(adapter->cap & SXE_DCB_ENABLE), state);
		ret = 0;
		goto l_end;
	}

	LOG_DEBUG_BDF("dcb current state=%u, set state=%u, setup tc\n",
			!!(adapter->cap & SXE_DCB_ENABLE), state);

	ret = !!sxe_ring_reassign(adapter, state ? \
			adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs : 0);

l_end:
	return ret;
}

static void sxe_dcbnl_perm_addr_get(struct net_device *netdev,
					u8 *perm_addr)
{
	u32 i;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	memset(perm_addr, 0xff, MAX_ADDR_LEN);

	for (i = 0; i < netdev->addr_len; i++) {
		perm_addr[i] = adapter->mac_filter_ctxt.def_mac_addr[i];
	}
	LOG_DEBUG_BDF("perm_addr=%pM\n", perm_addr);

	return;
}

static void sxe_dcbnl_tx_pg_tc_cfg_set(struct net_device *netdev, int tc,
					 u8 prio_type, u8 bwg_id, u8 bwg_pct,
					 u8 up_map)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_dcb_context *dcb_ctxt = &adapter->dcb_ctxt;

	LOG_DEBUG_BDF("tx pg tc config, tc=%d, prio=%u, bwg_id=%u, bwg_pct=%u, up_map=%u\n",
		tc, prio_type, bwg_id, bwg_pct, up_map);

	if (prio_type != DCB_ATTR_VALUE_UNDEFINED) {
		dcb_ctxt->cee_temp_cfg.tc_config[tc].\
			channel[DCB_PATH_TX].prio_type = prio_type;
	}
	if (bwg_id != DCB_ATTR_VALUE_UNDEFINED) {
		dcb_ctxt->cee_temp_cfg.tc_config[tc].\
			channel[DCB_PATH_TX].bwg_id = bwg_id;
	}
	if (bwg_pct != DCB_ATTR_VALUE_UNDEFINED) {
		dcb_ctxt->cee_temp_cfg.tc_config[tc].\
			channel[DCB_PATH_TX].bwg_percent = bwg_pct;
	}
	if (up_map != DCB_ATTR_VALUE_UNDEFINED) {
		dcb_ctxt->cee_temp_cfg.tc_config[tc].\
			channel[DCB_PATH_TX].up_to_tc_bitmap = up_map;
	}

	return;
}

static void sxe_dcbnl_tx_pg_bwg_cfg_set(struct net_device *netdev,
			int bwg_id, u8 bwg_pct)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	LOG_DEBUG_BDF("tx bw config, bwg_id=%d, bwg_pct=%u\n", bwg_id, bwg_pct);

	adapter->dcb_ctxt.cee_temp_cfg.\
		bwg_link_percent[DCB_PATH_TX][bwg_id] = bwg_pct;

	return;
}

static void sxe_dcbnl_rx_pg_tc_cfg_set(struct net_device *netdev, int tc,
					 u8 prio_type, u8 bwg_id, u8 bwg_pct,
					 u8 up_map)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_dcb_context *dcb_ctxt = &adapter->dcb_ctxt;

	LOG_DEBUG_BDF("rx pg tc config, tc=%d, prio=%u, bwg_id=%u, bwg_pct=%u, up_map=%u\n",
		tc, prio_type, bwg_id, bwg_pct, up_map);

	if (prio_type != DCB_ATTR_VALUE_UNDEFINED) {
		dcb_ctxt->cee_temp_cfg.tc_config[tc].\
			channel[DCB_PATH_RX].prio_type = prio_type;
	}
	if (bwg_id != DCB_ATTR_VALUE_UNDEFINED) {
		dcb_ctxt->cee_temp_cfg.tc_config[tc].\
			channel[DCB_PATH_RX].bwg_id = bwg_id;
	}
	if (bwg_pct != DCB_ATTR_VALUE_UNDEFINED) {
		dcb_ctxt->cee_temp_cfg.tc_config[tc].\
			channel[DCB_PATH_RX].bwg_percent = bwg_pct;
	}
	if (up_map != DCB_ATTR_VALUE_UNDEFINED) {
		dcb_ctxt->cee_temp_cfg.tc_config[tc].\
			channel[DCB_PATH_RX].up_to_tc_bitmap = up_map;
	}

	return;
}

static void sxe_dcbnl_rx_pg_bwg_cfg_set(struct net_device *netdev,
			int bwg_id, u8 bwg_pct)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	LOG_DEBUG_BDF("rx bw config, bwg_id=%d, bwg_pct=%u\n", bwg_id, bwg_pct);

	adapter->dcb_ctxt.cee_temp_cfg.\
		bwg_link_percent[DCB_PATH_RX][bwg_id] = bwg_pct;

	return;
}

static void sxe_dcbnl_tx_pg_tc_cfg_get(struct net_device *netdev, int tc,
					 u8 *prio, u8 *bwg_id, u8 *bwg_pct,
					 u8 *up_map)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_dcb_context *dcb_ctxt = &adapter->dcb_ctxt;

	*prio = dcb_ctxt->cee_cfg.tc_config[tc].\
			channel[DCB_PATH_TX].prio_type;
	*bwg_id = dcb_ctxt->cee_cfg.tc_config[tc].\
			channel[DCB_PATH_TX].bwg_id;
	*bwg_pct = dcb_ctxt->cee_cfg.tc_config[tc].\
			channel[DCB_PATH_TX].bwg_percent;
	*up_map = dcb_ctxt->cee_cfg.tc_config[tc].\
			channel[DCB_PATH_TX].up_to_tc_bitmap;

	LOG_DEBUG_BDF("get tx pg cfg: tc=%d, prio=%u, bwg_id=%u, bwg_pct=%u, up_map=%u\n",
		tc, *prio, *bwg_id, *bwg_pct, *up_map);

	return;
}

static void sxe_dcbnl_tx_pg_bwg_cfg_get(struct net_device *netdev, int bwg_id,
					  u8 *bwg_pct)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	*bwg_pct = adapter->dcb_ctxt.cee_cfg.\
		bwg_link_percent[DCB_PATH_TX][bwg_id];

	LOG_DEBUG_BDF("get tx bwg cfg: bwg_id=%u, bwg_pct=%d\n", bwg_id, *bwg_pct);

	return;
}

static void sxe_dcbnl_rx_pg_tc_cfg_get(struct net_device *netdev, int tc,
					 u8 *prio, u8 *bwg_id, u8 *bwg_pct,
					 u8 *up_map)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_dcb_context *dcb_ctxt = &adapter->dcb_ctxt;

	LOG_DEBUG_BDF("get rx pg cfg: tc=%d, prio=%u, bwg_id=%u, bwg_pct=%u, up_map=%u\n",
		tc, *prio, *bwg_id, *bwg_pct, *up_map);

	*prio   = dcb_ctxt->cee_cfg.tc_config[tc].\
				channel[DCB_PATH_RX].prio_type;
	*bwg_id = dcb_ctxt->cee_cfg.tc_config[tc].\
				channel[DCB_PATH_RX].bwg_id;
	*bwg_pct = dcb_ctxt->cee_cfg.tc_config[tc].\
				channel[DCB_PATH_RX].bwg_percent;
	*up_map = dcb_ctxt->cee_cfg.tc_config[tc].\
				channel[DCB_PATH_RX].up_to_tc_bitmap;

	return;
}

static void sxe_dcbnl_rx_pg_bwg_cfg_get(struct net_device *netdev, int bwg_id,
					  u8 *bwg_pct)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	*bwg_pct = adapter->dcb_ctxt.cee_cfg.bwg_link_percent[DCB_PATH_RX][bwg_id];

	LOG_DEBUG_BDF("get rx bwg cfg: bwg_id=%d, bwg_pct=%u\n", bwg_id, *bwg_pct);

	return;
}

static void sxe_dcbnl_pfc_cfg_set(struct net_device *netdev, int tc,
				    u8 setting)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	adapter->dcb_ctxt.cee_temp_cfg.tc_config[tc].pfc_type = setting;
	if (adapter->dcb_ctxt.cee_temp_cfg.tc_config[tc].pfc_type !=
	    adapter->dcb_ctxt.cee_cfg.tc_config[tc].pfc_type) {
		adapter->dcb_ctxt.cee_temp_cfg.pfc_mode_enable = true;
	}

	LOG_DEBUG_BDF("set pfc: tc=%d, setting=%u\n", tc, setting);

	return;
}

static void sxe_dcbnl_pfc_cfg_get(struct net_device *netdev, int tc,
				    u8 *setting)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	*setting = adapter->dcb_ctxt.cee_cfg.tc_config[tc].pfc_type;

	LOG_DEBUG_BDF("get pfc: priority=%d, setting=%u\n", tc, *setting);

	return;
}

static s32 sxe_dcb_cfg_copy(struct sxe_adapter *adapter, int tc_max)
{
	u32 i;
	u32 changes = 0;
	u32 tx = DCB_PATH_TX;
	u32 rx = DCB_PATH_RX;
	struct sxe_tc_config *src;
	struct sxe_tc_config *dst;
	struct sxe_dcb_cee_config *scfg = &adapter->dcb_ctxt.cee_temp_cfg;
	struct sxe_dcb_cee_config *dcfg = &adapter->dcb_ctxt.cee_cfg;

	for (i = 0; i < tc_max; i++) {
		src = &scfg->tc_config[i];
		dst = &dcfg->tc_config[i];

		if (dst->channel[tx].prio_type !=
					src->channel[tx].prio_type) {
			dst->channel[tx].prio_type =
				src->channel[tx].prio_type;
			changes |= BIT_PG_TX;
		}

		if (dst->channel[tx].bwg_id !=
					src->channel[tx].bwg_id) {
			dst->channel[tx].bwg_id =
				src->channel[tx].bwg_id;
			changes |= BIT_PG_TX;
		}

		if (dst->channel[tx].bwg_percent !=
					src->channel[tx].bwg_percent) {
			dst->channel[tx].bwg_percent =
				src->channel[tx].bwg_percent;
			changes |= BIT_PG_TX;
		}

		if (dst->channel[tx].up_to_tc_bitmap !=
					src->channel[tx].up_to_tc_bitmap) {
			dst->channel[tx].up_to_tc_bitmap =
				src->channel[tx].up_to_tc_bitmap;
			changes |= (BIT_PG_TX | BIT_PFC | BIT_APP_UPCHG);
		}

		if (dst->channel[rx].prio_type !=
					src->channel[rx].prio_type) {
			dst->channel[rx].prio_type =
				src->channel[rx].prio_type;
			changes |= BIT_PG_RX;
		}

		if (dst->channel[rx].bwg_id !=
					src->channel[rx].bwg_id) {
			dst->channel[rx].bwg_id =
				src->channel[rx].bwg_id;
			changes |= BIT_PG_RX;
		}

		if (dst->channel[rx].bwg_percent !=
					src->channel[rx].bwg_percent) {
			dst->channel[rx].bwg_percent =
				src->channel[rx].bwg_percent;
			changes |= BIT_PG_RX;
		}

		if (dst->channel[rx].up_to_tc_bitmap !=
					src->channel[rx].up_to_tc_bitmap) {
			dst->channel[rx].up_to_tc_bitmap =
				src->channel[rx].up_to_tc_bitmap;
			changes |= (BIT_PG_RX | BIT_PFC | BIT_APP_UPCHG);
		}
	}

	for (i = 0; i < SXE_DCB_TC_MAX; i++) {
		if (dcfg->bwg_link_percent[tx][i] !=
					scfg->bwg_link_percent[tx][i]) {
			dcfg->bwg_link_percent[tx][i] =
				scfg->bwg_link_percent[tx][i];
			changes |= BIT_PG_TX;
		}
		if (dcfg->bwg_link_percent[rx][i] !=
					scfg->bwg_link_percent[rx][i]) {
			dcfg->bwg_link_percent[rx][i] =
				scfg->bwg_link_percent[rx][i];
			changes |= BIT_PG_RX;
		}
	}

	for (i = 0; i < SXE_DCB_TC_MAX; i++) {
		if (dcfg->tc_config[i].pfc_type != scfg->tc_config[i].pfc_type) {
			dcfg->tc_config[i].pfc_type = scfg->tc_config[i].pfc_type;
			changes |= BIT_PFC;
		}
	}

	if (dcfg->pfc_mode_enable != scfg->pfc_mode_enable) {
		dcfg->pfc_mode_enable = scfg->pfc_mode_enable;
		changes |= BIT_PFC;
	}

	LOG_DEBUG_BDF("cee cfg cpy, change cfg=%x\n", changes);

	return changes;
}

static u8 sxe_dcbnl_cee_configure(struct net_device *netdev)
{
	u32 i;
	u8 pfc_en;
	u32 max_frame;
	u8 ret = DCB_NO_HW_CHG;
	u8 prio_tc[MAX_USER_PRIORITY];
	u16 refill[MAX_TRAFFIC_CLASS], max[MAX_TRAFFIC_CLASS];
	u8 bwg_id[MAX_TRAFFIC_CLASS], prio_type[MAX_TRAFFIC_CLASS];
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct sxe_hw *hw = &adapter->hw;
	struct sxe_dcb_cee_config *dcb_cfg = &adapter->dcb_ctxt.cee_cfg;

	LOG_DEBUG_BDF("dcbnl cfg setall\n");

	if (!(adapter->dcb_ctxt.dcbx_cap & DCB_CAP_DCBX_VER_CEE)) {
		LOG_DEBUG_BDF("not cee mode, settings are not supported\n");
		ret = DCB_NO_HW_CHG;
		goto l_end;
	}

	adapter->dcb_ctxt.cee_cfg_bitmap |=
		sxe_dcb_cfg_copy(adapter, MAX_TRAFFIC_CLASS);
	if (!adapter->dcb_ctxt.cee_cfg_bitmap) {
		LOG_DEBUG_BDF("cfg not change\n");
		ret =  DCB_NO_HW_CHG;
		goto l_end;
	}

	if (adapter->dcb_ctxt.cee_cfg_bitmap & (BIT_PG_TX|BIT_PG_RX)) {
		max_frame = adapter->netdev->mtu + SXE_ETH_DEAD_LOAD;

		sxe_dcb_cee_tc_credits_calculate(hw, dcb_cfg, max_frame,
					       DCB_PATH_TX);
		sxe_dcb_cee_tc_credits_calculate(hw, dcb_cfg, max_frame,
					       DCB_PATH_RX);

		sxe_dcb_cee_refill_parse(dcb_cfg, DCB_PATH_TX, refill);
		sxe_dcb_cee_max_credits_parse(dcb_cfg, max);
		sxe_dcb_cee_bwgid_parse(dcb_cfg, DCB_PATH_TX, bwg_id);
		sxe_dcb_cee_prio_parse(dcb_cfg, DCB_PATH_TX, prio_type);
		sxe_dcb_cee_up2tc_map_parse(dcb_cfg, DCB_PATH_TX, prio_tc);

		sxe_dcb_hw_ets_configure(hw, refill, max, bwg_id,
					prio_type, prio_tc);

		for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
			netdev_set_prio_tc_map(netdev, i, prio_tc[i]);
		}

		ret = DCB_HW_CHG_RST;
	}

	if (adapter->dcb_ctxt.cee_cfg_bitmap & BIT_PFC) {
		if (dcb_cfg->pfc_mode_enable) {
			sxe_dcb_cee_up2tc_map_parse(dcb_cfg, DCB_PATH_TX, prio_tc);
			sxe_dcb_cee_pfc_parse(dcb_cfg, &pfc_en);
			sxe_dcb_hw_pfc_configure(hw, pfc_en, prio_tc);
		} else {
			sxe_fc_enable(adapter);
		}

		sxe_rx_drop_mode_set(adapter);

		ret = DCB_HW_CHG;
	}

	adapter->dcb_ctxt.cee_cfg_bitmap = 0x0;
l_end:
	return ret;
}

static u8 sxe_dcbnl_all_set(struct net_device *netdev)
{
	return sxe_dcbnl_cee_configure(netdev);
}

static u8 sxe_dcbnl_cap_get(struct net_device *netdev, int capid, u8 *cap)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	switch (capid) {
	case DCB_CAP_ATTR_PG:
	case DCB_CAP_ATTR_PFC:
	case DCB_CAP_ATTR_GSP:
		*cap = true;
		break;
	case DCB_CAP_ATTR_UP2TC:
	case DCB_CAP_ATTR_BCN:
		*cap = false;
		break;
	case DCB_CAP_ATTR_PG_TCS:
	case DCB_CAP_ATTR_PFC_TCS:
		*cap = 0x80;
		break;
	case DCB_CAP_ATTR_DCBX:
		*cap = adapter->dcb_ctxt.dcbx_cap;
		break;
	default:
		*cap = false;
		break;
	}
	LOG_DEBUG_BDF("get dcb cap=%x\n", *cap);

	return 0;
}

static int sxe_dcbnl_num_tcs_get(struct net_device *netdev, int tcid, u8 *num)
{
	int ret = 0;
	struct sxe_adapter *adapter = netdev_priv(netdev);

	if (adapter->cap & SXE_DCB_ENABLE) {
		switch (tcid) {
		case DCB_NUMTCS_ATTR_PG:
			*num = adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs;
			break;
		case DCB_NUMTCS_ATTR_PFC:
			*num = adapter->dcb_ctxt.cee_cfg.num_tcs.pfc_tcs;
			break;
		default:
			LOG_ERROR_BDF("feature dont support=%x\n", tcid);
			ret = -EINVAL;
		}
	} else {
		LOG_ERROR_BDF("dcb disable\n");
		ret = -EINVAL;
	}

	LOG_DEBUG_BDF("tcid=%x, tcs=%u\n", tcid, *num);

	return ret;
}

static int sxe_dcbnl_num_tcs_set(struct net_device *netdev, int tcid, u8 num)
{
	LOG_WARN("configuring tc is not supported\n");
	return -EINVAL;
}

static u8 sxe_dcbnl_pfc_state_get(struct net_device *netdev)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	LOG_DEBUG_BDF("pfc state=%x\n", adapter->dcb_ctxt.cee_cfg.pfc_mode_enable);

	return adapter->dcb_ctxt.cee_cfg.pfc_mode_enable;
}

static void sxe_dcbnl_pfc_state_set(struct net_device *netdev, u8 state)
{
	struct sxe_adapter *adapter = netdev_priv(netdev);

	LOG_DEBUG_BDF("current pfc state=%x, set state=%x\n",
		adapter->dcb_ctxt.cee_cfg.pfc_mode_enable, state);

	adapter->dcb_ctxt.cee_temp_cfg.pfc_mode_enable = state;

	return;
}

#ifdef DCBNL_OPS_GETAPP_RETURN_U8
static u8 sxe_dcbnl_app_get(struct net_device *netdev, u8 idtype, u16 id)
#else
static int sxe_dcbnl_app_get(struct net_device *netdev, u8 idtype, u16 id)
#endif
{
	int ret;
	struct sxe_adapter *adapter = netdev_priv(netdev);
	struct dcb_app app = {
				.selector = idtype,
				.protocol = id,
			     };

	if (!(adapter->dcb_ctxt.dcbx_cap & DCB_CAP_DCBX_VER_CEE)) {
		LOG_DEBUG_BDF("not cee mode, not supported get\n");
#ifdef DCBNL_OPS_GETAPP_RETURN_U8
		ret = 0;
#else
		ret = -EINVAL;
#endif
		goto l_end;
	}

	ret = dcb_getapp(netdev, &app);
	LOG_DEBUG_BDF("idtype=%x, id=%x, app=%x\n", idtype, id, ret);

l_end:
	return ret;
}

static void sxe_dcbnl_devreset(struct net_device *dev)
{
	s32 ret;
	struct sxe_adapter *adapter = netdev_priv(dev);

	while (test_and_set_bit(SXE_RESETTING, &adapter->state)) {
		usleep_range(1000, 2000);
	}

	if (netif_running(dev)) {
		dev->netdev_ops->ndo_stop(dev);
	}

	sxe_ring_irq_exit(adapter);
	ret = sxe_ring_irq_init(adapter);
	if (ret) {
		LOG_ERROR_BDF("interrupt ring assign scheme init failed, err=%d\n", ret);
		goto l_end;
	}

	if (netif_running(dev)) {
		dev->netdev_ops->ndo_open(dev);
	}

	clear_bit(SXE_RESETTING, &adapter->state);
	LOG_DEBUG_BDF("dcbnl reset finish\n");
l_end:
	return;
}

static int sxe_dcbnl_ieee_getets(struct net_device *dev,
				   struct ieee_ets *ets)
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct ieee_ets *hw_ets = adapter->dcb_ctxt.ieee_ets;

	ets->ets_cap = adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs;

	if (!hw_ets) {
		LOG_DEBUG_BDF("dont have ets cfg\n");
		goto l_end;
	}

	ets->cbs = hw_ets->cbs;
	memcpy(ets->tc_tx_bw, hw_ets->tc_tx_bw, sizeof(ets->tc_tx_bw));
	memcpy(ets->tc_rx_bw, hw_ets->tc_rx_bw, sizeof(ets->tc_rx_bw));
	memcpy(ets->tc_tsa, hw_ets->tc_tsa, sizeof(ets->tc_tsa));
	memcpy(ets->prio_tc, hw_ets->prio_tc, sizeof(ets->prio_tc));

	LOG_DEBUG_BDF("get ets cfg ok\n");

l_end:
	return 0;
}

static int sxe_dcbnl_ieee_setets(struct net_device *dev,
				   struct ieee_ets *ets)
{
	int ret;
	u32 i;
	u8 max_tc = 0;
	u8 map_chg = 0;
	u32 max_frame = dev->mtu + SXE_ETH_DEAD_LOAD;
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct sxe_hw *hw = &adapter->hw;

	LOG_DEBUG_BDF("set ets\n");

	if (!(adapter->dcb_ctxt.dcbx_cap & DCB_CAP_DCBX_VER_IEEE)) {
		LOG_ERROR_BDF("not ieee, dont support\n");
		ret = -EINVAL;
		goto l_end;
	}

	if (!adapter->dcb_ctxt.ieee_ets) {
		adapter->dcb_ctxt.ieee_ets =
			kmalloc(sizeof(struct ieee_ets), GFP_KERNEL);
		if (!adapter->dcb_ctxt.ieee_ets) {
			LOG_ERROR_BDF("kmalloc failed\n");
			ret = -ENOMEM;
			goto l_end;
		}

		for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
			adapter->dcb_ctxt.ieee_ets->prio_tc[i] =
				IEEE_8021QAZ_MAX_TCS;
		}

		hw->dma.ops->dcb_rx_up_tc_map_get(hw,
				adapter->dcb_ctxt.ieee_ets->prio_tc);
	}

	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		if (ets->prio_tc[i] > max_tc) {
			max_tc = ets->prio_tc[i];
		}

		if (ets->prio_tc[i] != \
				adapter->dcb_ctxt.ieee_ets->prio_tc[i]) {
			map_chg = 1;
		}
	}

	memcpy(adapter->dcb_ctxt.ieee_ets,
		ets, sizeof(*adapter->dcb_ctxt.ieee_ets));

	if (max_tc) {
		max_tc++;
	}

	if (max_tc > adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs) {
		LOG_ERROR_BDF("set tc=%u > max tc=%u\n", max_tc,
			adapter->dcb_ctxt.cee_cfg.num_tcs.pg_tcs);
		ret = -EINVAL;
		goto l_end;
	}

	if (max_tc != adapter->dcb_ctxt.hw_tcs) {
		ret = sxe_ring_reassign(adapter, max_tc);
		if (ret) {
			LOG_ERROR_BDF("ring reassign failed, ret=%d\n", ret);
			goto l_end;
		}
	} else if (map_chg) {
		sxe_dcbnl_devreset(dev);
	}

	ret = sxe_dcb_hw_ieee_ets_configure(&adapter->hw, ets, max_frame);
	if (ret) {
		LOG_ERROR_BDF("ets config failed, max_frame=%u, ret=%u\n",
				max_frame, ret);
	}

l_end:
	return ret;
}

static int sxe_dcbnl_ieee_getpfc(struct net_device *dev,
				   struct ieee_pfc *pfc)
{
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct ieee_pfc *hw_pfc = adapter->dcb_ctxt.ieee_pfc;

	pfc->pfc_cap = adapter->dcb_ctxt.cee_cfg.num_tcs.pfc_tcs;

	if (!hw_pfc) {
		LOG_DEBUG_BDF("dont have pfc cfg\n");
		goto l_end;
	}

	pfc->pfc_en = hw_pfc->pfc_en;
	pfc->mbc = hw_pfc->mbc;
	pfc->delay = hw_pfc->delay;

	LOG_DEBUG_BDF("get pfc cfg ok\n");

l_end:
	return 0;
}

static int sxe_dcbnl_ieee_setpfc(struct net_device *dev,
				   struct ieee_pfc *pfc)
{
	int ret = 0;
	u8 *prio_tc;
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct sxe_hw *hw = &adapter->hw;

	if (!(adapter->dcb_ctxt.dcbx_cap & DCB_CAP_DCBX_VER_IEEE)) {
		LOG_ERROR_BDF("not ieee, dont support\n");
		ret = -EINVAL;
		goto l_end;
	}

	if (!adapter->dcb_ctxt.ieee_pfc) {
		adapter->dcb_ctxt.ieee_pfc = kmalloc(sizeof(struct ieee_pfc),
						  GFP_KERNEL);
		if (!adapter->dcb_ctxt.ieee_pfc) {
			LOG_ERROR_BDF("kmalloc failed\n");
			ret = -ENOMEM;
			goto l_end;
		}
	}

	prio_tc = adapter->dcb_ctxt.ieee_ets->prio_tc;
	memcpy(adapter->dcb_ctxt.ieee_pfc, pfc,
			sizeof(*adapter->dcb_ctxt.ieee_pfc));

	if (pfc->pfc_en) {
		sxe_dcb_hw_pfc_configure(hw, pfc->pfc_en, prio_tc);
	} else {
		sxe_fc_enable(adapter);
	}

	sxe_rx_drop_mode_set(adapter);

l_end:
	return ret;
}

static int sxe_dcbnl_ieee_setapp(struct net_device *dev,
				   struct dcb_app *app)
{
	int ret;
	u32 vf;
	struct sxe_vf_info  *vfinfo;
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct sxe_hw *hw = &adapter->hw;

	if (!(adapter->dcb_ctxt.dcbx_cap & DCB_CAP_DCBX_VER_IEEE)) {
		LOG_ERROR_BDF("not ieee, dont support\n");
		ret = -EINVAL;
		goto l_end;
	}

	ret = dcb_ieee_setapp(dev, app);
	if (ret) {
		LOG_ERROR_BDF("set app failed, ret=%d\n", ret);
		goto l_end;
	}

	if (app->selector == IEEE_8021QAZ_APP_SEL_ETHERTYPE &&
	    app->protocol == 0) {
		adapter->dcb_ctxt.default_up = app->priority;

		for (vf = 0; vf < adapter->vt_ctxt.num_vfs; vf++) {
			vfinfo = &adapter->vt_ctxt.vf_info[vf];

			if (!vfinfo->pf_qos) {
				hw->dma.ops->tx_vlan_tag_set(hw, vfinfo->pf_vlan,
						app->priority, vf);
			}
		}
	}

	LOG_DEBUG_BDF("set app ok\n");

l_end:
	return ret;
}

static int sxe_dcbnl_ieee_delapp(struct net_device *dev,
				   struct dcb_app *app)
{
	int ret;
	u32 vf;
	u16 qos;
	long unsigned int app_mask;
	struct sxe_vf_info  *vfinfo;
	struct sxe_adapter *adapter = netdev_priv(dev);
	struct sxe_hw *hw = &adapter->hw;

	if (!(adapter->dcb_ctxt.dcbx_cap & DCB_CAP_DCBX_VER_IEEE)) {
		LOG_ERROR_BDF("not ieee, dont support\n");
		ret = -EINVAL;
		goto l_end;
	}

	ret = dcb_ieee_delapp(dev, app);
	if (ret) {
		LOG_ERROR_BDF("del app failed, ret=%d\n", ret);
		goto l_end;
	}

	if (app->selector == IEEE_8021QAZ_APP_SEL_ETHERTYPE &&
	    app->protocol == 0 && adapter->dcb_ctxt.default_up == app->priority) {
		app_mask = dcb_ieee_getapp_mask(dev, app);
		qos = app_mask ? find_first_bit(&app_mask, 8) : 0;

		adapter->dcb_ctxt.default_up = qos;

		for (vf = 0; vf < adapter->vt_ctxt.num_vfs; vf++) {
			vfinfo = &adapter->vt_ctxt.vf_info[vf];

			if (!vfinfo->pf_qos) {
				hw->dma.ops->tx_vlan_tag_set(hw, vfinfo->pf_vlan,
						qos, vf);
			}
		}
	}

	LOG_DEBUG_BDF("del app ok\n");

l_end:
	return ret;
}

static u8 sxe_dcbnl_dcbx_get(struct net_device *dev)
{
	struct sxe_adapter *adapter = netdev_priv(dev);

	LOG_DEBUG_BDF("dcbx cap=%x\n", adapter->dcb_ctxt.dcbx_cap);

	return adapter->dcb_ctxt.dcbx_cap;
}

static u8 sxe_dcbnl_dcbx_set(struct net_device *dev, u8 mode)
{
	u8  ret = 0;
	s32 err = 0;
	struct ieee_ets ets = {0};
	struct ieee_pfc pfc = {0};
	struct sxe_adapter *adapter = netdev_priv(dev);

	if ((mode & DCB_CAP_DCBX_LLD_MANAGED) ||
	    ((mode & DCB_CAP_DCBX_VER_IEEE) && (mode & DCB_CAP_DCBX_VER_CEE)) ||
	    !(mode & DCB_CAP_DCBX_HOST)) {
		LOG_ERROR_BDF("dont support mode=%x\n", mode);
		ret = 1;
		goto l_end;
	}

	if (mode == adapter->dcb_ctxt.dcbx_cap) {
		goto l_end;
	}

	adapter->dcb_ctxt.dcbx_cap = mode;

	ets.ets_cap = 8;
	pfc.pfc_cap = 8;

	if (mode & DCB_CAP_DCBX_VER_IEEE) {
		sxe_dcbnl_ieee_setets(dev, &ets);
		sxe_dcbnl_ieee_setpfc(dev, &pfc);
	} else if (mode & DCB_CAP_DCBX_VER_CEE) {
		u8 mask = BIT_PFC | BIT_PG_TX | BIT_PG_RX | BIT_APP_UPCHG;

		adapter->dcb_ctxt.cee_cfg_bitmap |= mask;
		sxe_dcbnl_cee_configure(dev);
	} else {
		sxe_dcbnl_ieee_setets(dev, &ets);
		sxe_dcbnl_ieee_setpfc(dev, &pfc);
		err = sxe_ring_reassign(adapter, 0);
		if (err) {
			LOG_ERROR_BDF("ring reassign failed, err=%d\n", err);
			ret = 1;
		}
	}
l_end:
	return ret;
}

const struct dcbnl_rtnl_ops sxe_dcbnl_ops = {
	.ieee_getets	= sxe_dcbnl_ieee_getets,
	.ieee_setets	= sxe_dcbnl_ieee_setets,
	.ieee_getpfc	= sxe_dcbnl_ieee_getpfc,
	.ieee_setpfc	= sxe_dcbnl_ieee_setpfc,
	.ieee_setapp	= sxe_dcbnl_ieee_setapp,
	.ieee_delapp	= sxe_dcbnl_ieee_delapp,

	.getstate	= sxe_dcbnl_state_get,
	.setstate	= sxe_dcbnl_state_set,
	.getpermhwaddr	= sxe_dcbnl_perm_addr_get,
	.setpgtccfgtx	= sxe_dcbnl_tx_pg_tc_cfg_set,
	.setpgbwgcfgtx	= sxe_dcbnl_tx_pg_bwg_cfg_set,
	.setpgtccfgrx	= sxe_dcbnl_rx_pg_tc_cfg_set,
	.setpgbwgcfgrx	= sxe_dcbnl_rx_pg_bwg_cfg_set,
	.getpgtccfgtx	= sxe_dcbnl_tx_pg_tc_cfg_get,
	.getpgbwgcfgtx	= sxe_dcbnl_tx_pg_bwg_cfg_get,
	.getpgtccfgrx	= sxe_dcbnl_rx_pg_tc_cfg_get,
	.getpgbwgcfgrx	= sxe_dcbnl_rx_pg_bwg_cfg_get,
	.setpfccfg	= sxe_dcbnl_pfc_cfg_set,
	.getpfccfg	= sxe_dcbnl_pfc_cfg_get,
	.setall		= sxe_dcbnl_all_set,
	.getcap		= sxe_dcbnl_cap_get,
	.getnumtcs	= sxe_dcbnl_num_tcs_get,
	.setnumtcs	= sxe_dcbnl_num_tcs_set,
	.getpfcstate	= sxe_dcbnl_pfc_state_get,
	.setpfcstate	= sxe_dcbnl_pfc_state_set,
	.getapp		= sxe_dcbnl_app_get,
	.getdcbx	= sxe_dcbnl_dcbx_get,
	.setdcbx	= sxe_dcbnl_dcbx_set,
};
#endif 

void sxe_rx_drop_mode_set(struct sxe_adapter *adapter)
{
	u32 i;
	struct sxe_hw *hw = &adapter->hw;
	bool pfc_en = adapter->dcb_ctxt.cee_cfg.pfc_mode_enable;
	u32 current_mode = hw->mac.ops->fc_current_mode_get(hw);

#ifdef SXE_DCB_CONFIGURE
	if (adapter->dcb_ctxt.ieee_pfc) {
		pfc_en |= !!(adapter->dcb_ctxt.ieee_pfc->pfc_en);
	}
#endif

	if (adapter->vt_ctxt.num_vfs || (adapter->rx_ring_ctxt.num > 1 &&
		!(current_mode & SXE_FC_TX_PAUSE) && !pfc_en)) {
		for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
			hw->dma.ops->rx_drop_switch(hw,
				adapter->rx_ring_ctxt.ring[i]->reg_idx, true);
		}
	} else {
		for (i = 0; i < adapter->rx_ring_ctxt.num; i++) {
			hw->dma.ops->rx_drop_switch(hw,
				adapter->rx_ring_ctxt.ring[i]->reg_idx, false);
		}
	}

	return;
}


#include "sxe.h"
#include "sxe_ptp.h"
#include "sxe_log.h"
#include "sxe_hw.h"

static u64 sxe_ptp_read(const struct cyclecounter *cc)
{
	struct sxe_adapter *adapter =
		container_of(cc, struct sxe_adapter, ptp_ctxt.hw_cc);
	struct sxe_hw *hw = &adapter->hw;

	return hw->dbu.ops->ptp_systime_get(hw);
}

#ifdef HAVE_PTP_CLOCK_INFO_ADJFINE
static int sxe_ptp_adjfine(struct ptp_clock_info *ptp, long ppm)
{
	struct sxe_adapter *adapter =
		container_of(ptp, struct sxe_adapter, ptp_ctxt.ptp_clock_info);
	struct sxe_hw *hw = &adapter->hw;

	u32 adj_ns;
	u32 neg_adj = 0;

	if (ppm < 0) {
		neg_adj = SXE_TIMADJ_SIGN;
		adj_ns = (u32)(-((ppm * 125) >> 13));
	} else {
		adj_ns = (u32)((ppm * 125) >> 13);
	}

	LOG_DEBUG_BDF("ptp adjfreq adj_ns=%u, neg_adj=0x%x\n",adj_ns, neg_adj);
	hw->dbu.ops->ptp_freq_adjust(hw, (neg_adj | adj_ns));

	return 0;
}
#else
static int sxe_ptp_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
	struct sxe_adapter *adapter =
		container_of(ptp, struct sxe_adapter, ptp_ctxt.ptp_clock_info);
	struct sxe_hw *hw = &adapter->hw;

	u32 adj_ns;
	u32 neg_adj = 0;

	if (ppb < 0) {
		neg_adj = SXE_TIMADJ_SIGN;
		adj_ns = -ppb;
	} else {
		adj_ns = ppb;
	}

	LOG_DEBUG_BDF("ptp adjfreq adj_ns=%u, neg_adj=0x%x\n",adj_ns, neg_adj);
	hw->dbu.ops->ptp_freq_adjust(hw, (neg_adj | adj_ns));

	return 0;
}
#endif 

static int sxe_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	struct sxe_adapter *adapter =
		container_of(ptp, struct sxe_adapter, ptp_ctxt.ptp_clock_info);
	unsigned long flags;

	spin_lock_irqsave(&adapter->ptp_ctxt.ptp_timer_lock, flags);
	timecounter_adjtime(&adapter->ptp_ctxt.hw_tc, delta);
	spin_unlock_irqrestore(&adapter->ptp_ctxt.ptp_timer_lock, flags);

	LOG_INFO_BDF("ptp adjust systim, delta: %lld, after adj: %llu\n",
			delta, adapter->ptp_ctxt.hw_tc.nsec);;

	return 0;
}

static int sxe_ptp_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	unsigned long flags;
	struct sxe_adapter *adapter =
		container_of(ptp, struct sxe_adapter, ptp_ctxt.ptp_clock_info);
	struct sxe_hw *hw = &adapter->hw;
	u64 ns, systim_ns;

	systim_ns = hw->dbu.ops->ptp_systime_get(hw);
	LOG_DEBUG_BDF("ptp get time = %llu ns\n", systim_ns);

	spin_lock_irqsave(&adapter->ptp_ctxt.ptp_timer_lock, flags);
	ns = timecounter_cyc2time(&adapter->ptp_ctxt.hw_tc, systim_ns);
	spin_unlock_irqrestore(&adapter->ptp_ctxt.ptp_timer_lock, flags);

	LOG_DEBUG_BDF("timecounter_cyc2time = %llu ns\n", ns);

	*ts = ns_to_timespec64(ns);

	return 0;
}

static int sxe_ptp_settime(struct ptp_clock_info *ptp,
			     const struct timespec64 *ts)
{
	unsigned long flags;
	struct sxe_adapter *adapter =
		container_of(ptp, struct sxe_adapter, ptp_ctxt.ptp_clock_info);
	u64 ns = timespec64_to_ns(ts);

	LOG_DEBUG_BDF("ptp settime = %llu ns\n", ns);

	spin_lock_irqsave(&adapter->ptp_ctxt.ptp_timer_lock, flags);
	timecounter_init(&adapter->ptp_ctxt.hw_tc, &adapter->ptp_ctxt.hw_cc, ns);
	spin_unlock_irqrestore(&adapter->ptp_ctxt.ptp_timer_lock, flags);

	return 0;
}

static int sxe_ptp_feature_enable(struct ptp_clock_info *ptp,
				    struct ptp_clock_request *rq, int on)
{
	s32 ret = 0;
	struct sxe_adapter *adapter =
		container_of(ptp, struct sxe_adapter, ptp_ctxt.ptp_clock_info);
	if (rq->type != PTP_CLK_REQ_PPS || !adapter->ptp_ctxt.ptp_setup_spp) {
		ret = -ENOTSUPP;
		goto l_ret;
	}

	if (on) {
		adapter->cap |= SXE_PTP_PPS_ENABLED;
	} else {
		adapter->cap &= ~SXE_PTP_PPS_ENABLED;
	}

	adapter->ptp_ctxt.ptp_setup_spp(adapter);

l_ret:
	return ret;
}

static inline void sxe_ptp_clock_info_init(
				struct ptp_clock_info *ptp_clock_info, char *name)
{
	snprintf(ptp_clock_info->name,
		sizeof(ptp_clock_info->name), "%s", name);
	ptp_clock_info->owner = THIS_MODULE;
	ptp_clock_info->max_adj = SXE_PTP_MAX_ADJ;
	ptp_clock_info->n_alarm = 0;
	ptp_clock_info->n_ext_ts = 0;
	ptp_clock_info->n_per_out = 0;
	ptp_clock_info->pps = 0;
#ifdef HAVE_PTP_CLOCK_INFO_ADJFINE
	ptp_clock_info->adjfine = sxe_ptp_adjfine;
#else
	ptp_clock_info->adjfreq = sxe_ptp_adjfreq;
#endif
	ptp_clock_info->adjtime = sxe_ptp_adjtime;
	ptp_clock_info->gettime64 = sxe_ptp_gettime;
	ptp_clock_info->settime64 = sxe_ptp_settime;
	ptp_clock_info->enable = sxe_ptp_feature_enable;

	return;
}

static long sxe_ptp_clock_create(struct sxe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	long ret = 0;

	if (!IS_ERR_OR_NULL(adapter->ptp_ctxt.ptp_clock)) {
		goto l_ret;
	}

	sxe_ptp_clock_info_init(&adapter->ptp_ctxt.ptp_clock_info, netdev->name);
	LOG_DEBUG_BDF("init ptp[%s] info finish\n", adapter->ptp_ctxt.ptp_clock_info.name);

	adapter->ptp_ctxt.ptp_clock =
		ptp_clock_register(&adapter->ptp_ctxt.ptp_clock_info,
					&adapter->pdev->dev);
	if (IS_ERR(adapter->ptp_ctxt.ptp_clock)) {
		ret = PTR_ERR(adapter->ptp_ctxt.ptp_clock);
		adapter->ptp_ctxt.ptp_clock = NULL;
		LOG_DEV_ERR("ptp_clock_register failed\n");
		goto l_ret;
	} else if (adapter->ptp_ctxt.ptp_clock) {
		LOG_DEV_INFO("registered PHC device on %s\n", netdev->name);
	}

	adapter->ptp_ctxt.tstamp_config.rx_filter = HWTSTAMP_FILTER_NONE;
	adapter->ptp_ctxt.tstamp_config.tx_type = HWTSTAMP_TX_OFF;

l_ret:
	return ret;
}

static void sxe_ptp_clear_tx_timestamp(struct sxe_adapter *adapter)
{
	if (adapter->ptp_ctxt.ptp_tx_skb) {
		dev_kfree_skb_any(adapter->ptp_ctxt.ptp_tx_skb);
		adapter->ptp_ctxt.ptp_tx_skb = NULL;
	}

	clear_bit_unlock(SXE_PTP_TX_IN_PROGRESS, &adapter->state);

	return;
}

void sxe_ptp_overflow_check(struct sxe_adapter *adapter)
{
	unsigned long flags;
	bool timeout = time_is_before_jiffies(
					adapter->ptp_ctxt.last_overflow_check +
					SXE_OVERFLOW_PERIOD);

	if (timeout) {
		spin_lock_irqsave(&adapter->ptp_ctxt.ptp_timer_lock, flags);
		timecounter_read(&adapter->ptp_ctxt.hw_tc);
		spin_unlock_irqrestore(&adapter->ptp_ctxt.ptp_timer_lock, flags);

		adapter->ptp_ctxt.last_overflow_check = jiffies;
	}

	return ;
}

void sxe_ptp_rx_hang(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	bool rx_tmstamp_valid;
	struct sxe_ring *rx_ring;
	unsigned long rx_event;
	u16 n;

	rx_tmstamp_valid = hw->dbu.ops->ptp_is_rx_timestamp_valid(hw);
	if (!rx_tmstamp_valid) {
		adapter->ptp_ctxt.last_rx_ptp_check = jiffies;
		goto l_ret;
	}

	rx_event = adapter->ptp_ctxt.last_rx_ptp_check;
	for (n = 0; n < adapter->rx_ring_ctxt.num ; n++) {
		rx_ring = adapter->rx_ring_ctxt.ring[n];
		if (time_after(rx_ring->last_rx_timestamp, rx_event)) {
			rx_event = rx_ring->last_rx_timestamp;
		}
	}

	if (time_is_before_jiffies(rx_event + SXE_PTP_RX_TIMEOUT)) {
		hw->dbu.ops->ptp_rx_timestamp_clear(hw);
		adapter->ptp_ctxt.last_rx_ptp_check = jiffies;

		adapter->stats.sw.rx_hwtstamp_cleared++;

		LOG_MSG_DEBUG(drv, "clearing RX Timestamp hang\n");
	}

l_ret:
	return;
}

void sxe_ptp_tx_hang(struct sxe_adapter *adapter)
{
	bool timeout = time_is_before_jiffies(adapter->ptp_ctxt.ptp_tx_start +
					      SXE_PTP_TX_TIMEOUT);

	if (!adapter->ptp_ctxt.ptp_tx_skb) {
		LOG_INFO_BDF("no ptp skb to progress\n");
		goto l_ret;
	}

	if (!test_bit(SXE_PTP_TX_IN_PROGRESS, &adapter->state)) {
		LOG_INFO_BDF("tx ptp not in progress\n");
		goto l_ret;
	}

	if (timeout) {
		cancel_work_sync(&adapter->ptp_ctxt.ptp_tx_work);
		sxe_ptp_clear_tx_timestamp(adapter);
		adapter->stats.sw.tx_hwtstamp_timeouts++;
		LOG_MSG_WARN(drv, "clearing Tx timestamp hang\n");
	}

l_ret:
	return;
}

static void sxe_ptp_convert_to_hwtstamp(struct sxe_adapter *adapter,
					  struct skb_shared_hwtstamps *hwtstamp,
					  u64 timestamp)
{
	unsigned long flags;
	u64 ns;

	memset(hwtstamp, 0, sizeof(*hwtstamp));

	spin_lock_irqsave(&adapter->ptp_ctxt.ptp_timer_lock, flags);
	ns = timecounter_cyc2time(&adapter->ptp_ctxt.hw_tc, timestamp);
	spin_unlock_irqrestore(&adapter->ptp_ctxt.ptp_timer_lock, flags);

	hwtstamp->hwtstamp = ns_to_ktime(ns);

	return ;
}

static void sxe_ptp_tx_hwtstamp_process(struct sxe_adapter *adapter)
{
	struct sk_buff *skb = adapter->ptp_ctxt.ptp_tx_skb;
	struct skb_shared_hwtstamps shhwtstamps;
	struct timespec64 ts;
	u64 ns;

	ts.tv_nsec = adapter->ptp_ctxt.tx_hwtstamp_nsec;
	ts.tv_sec  = adapter->ptp_ctxt.tx_hwtstamp_sec;

	ns = (u64)timespec64_to_ns(&ts);
	LOG_DEBUG_BDF("get tx timestamp value=%llu\n", ns);

	sxe_ptp_convert_to_hwtstamp(adapter, &shhwtstamps, ns);

	adapter->ptp_ctxt.ptp_tx_skb = NULL;
	clear_bit_unlock(SXE_PTP_TX_IN_PROGRESS, &adapter->state);

#if 0	
	if (!(adapter->cap & SXE_1588V2_ONE_STEP)) {
		skb_tstamp_tx(skb, &shhwtstamps);
	} else {
		if (adapter->cap & SXE_1588V2_ONE_STEP) {
			adapter->cap &= ~SXE_1588V2_ONE_STEP;
		}
	}
#endif

	skb_tstamp_tx(skb, &shhwtstamps);

	dev_kfree_skb_any(skb);

	return;
}

void sxe_ptp_get_rx_tstamp_in_pkt(struct sxe_irq_data *irq_data,
			   struct sk_buff *skb)
{
	__le64 ptp_tm;
	struct sxe_adapter *adapter = irq_data->adapter;

	skb_copy_bits(skb, skb->len - SXE_TS_HDR_LEN, &ptp_tm,
		      SXE_TS_HDR_LEN);
	__pskb_trim(skb, skb->len - SXE_TS_HDR_LEN);

	LOG_DEBUG_BDF("ptp get timestamp in pkt end = %llu\n", le64_to_cpu(ptp_tm));
	sxe_ptp_convert_to_hwtstamp(adapter, skb_hwtstamps(skb),
				    le64_to_cpu(ptp_tm));
	return;
}

void sxe_ptp_get_rx_tstamp_in_reg(struct sxe_irq_data *irq_data,
			   struct sk_buff *skb)
{
	struct sxe_adapter *adapter = irq_data->adapter;
	struct sxe_hw *hw = &adapter->hw;
	u64 ptp_tm;
	bool rx_tstamp_valid;

	if (!irq_data || !irq_data->adapter) {
		goto l_ret;
	}

	rx_tstamp_valid = hw->dbu.ops->ptp_is_rx_timestamp_valid(hw);
	if (rx_tstamp_valid) {
		ptp_tm = hw->dbu.ops->ptp_rx_timestamp_get(hw);
		sxe_ptp_convert_to_hwtstamp(adapter, skb_hwtstamps(skb), ptp_tm);
	} else {
		LOG_INFO_BDF("rx timestamp not valid in rx hw rigister\n");
		goto l_ret;
	}

l_ret:
	return;
}

static void sxe_ptp_tx_work_handler(struct work_struct *work)
{
	struct sxe_adapter *adapter = container_of(work, struct sxe_adapter,
						     ptp_ctxt.ptp_tx_work);
	struct sxe_hw *hw = &adapter->hw;
	bool timeout = time_is_before_jiffies(adapter->ptp_ctxt.ptp_tx_start +
					      SXE_PTP_TX_TIMEOUT);
	u32 ts_sec;
	u32 ts_ns;
	u32 last_sec;
	u32 last_ns;
	bool tx_tstamp_valid = true;
	u8 i;

	if (!adapter->ptp_ctxt.ptp_tx_skb) {
		sxe_ptp_clear_tx_timestamp(adapter);
		goto l_ret;
	}

	hw->dbu.ops->ptp_tx_timestamp_get(hw, &ts_sec, &ts_ns);
	if (ts_ns  != adapter->ptp_ctxt.tx_hwtstamp_nsec ||
		ts_sec != adapter->ptp_ctxt.tx_hwtstamp_sec) {

		for (i = 0; i < SXE_TXTS_POLL_CHECK; i++) {
			hw->dbu.ops->ptp_tx_timestamp_get(hw, &last_sec, &last_ns);
		}

		for (; i < SXE_TXTS_POLL; i++) {
			hw->dbu.ops->ptp_tx_timestamp_get(hw, &ts_sec, &ts_ns);
			if ((last_ns != ts_ns) || (last_sec != ts_sec)) {
				tx_tstamp_valid = false;
				break;
			}
		}

		if (tx_tstamp_valid) {
			adapter->ptp_ctxt.tx_hwtstamp_nsec = ts_ns;
			adapter->ptp_ctxt.tx_hwtstamp_sec  = ts_sec;
			sxe_ptp_tx_hwtstamp_process(adapter);
			return;
		}

		LOG_MSG_DEBUG(drv, "Tx timestamp error, "
		      "ts: %u %u, last ts: %u %u\n",
		      ts_sec, ts_ns, last_sec, last_ns);
	}

	if (timeout) {
		sxe_ptp_clear_tx_timestamp(adapter);
		adapter->stats.sw.tx_hwtstamp_timeouts++;
		LOG_MSG_WARN(drv, "clearing Tx timestamp hang\n");
	} else {
		schedule_work(&adapter->ptp_ctxt.ptp_tx_work);
	}

l_ret:
	return;
}

static s32 sxe_ptp_tx_type_get(s32 tx_type, u32 *tsctl)
{
	s32 ret = 0;

	switch (tx_type) {
	case HWTSTAMP_TX_OFF:
		*tsctl = SXE_TSCTRL_VER_2;
		break;
	case HWTSTAMP_TX_ON:
		*tsctl |= SXE_TSCTRL_TSEN;
		break;
	default:
		ret = -ERANGE;
	}

	return ret;
}

static s32 sxe_ptp_rx_filter_get(s32 *rx_filter, u32 *cap,
					bool *is_v1, bool *is_l2, u32 *tses)
{
	s32 ret = 0;

	switch (*rx_filter) {
	case HWTSTAMP_FILTER_NONE:
		*cap &= ~(SXE_RX_HWTSTAMP_ENABLED |
				    SXE_RX_HWTSTAMP_IN_REGISTER);
		break;
	case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
		*is_v1 = true;
		*tses |= SXE_TSES_TXES_V1_SYNC | SXE_TSES_RXES_V1_SYNC;
		*cap |= (SXE_RX_HWTSTAMP_ENABLED |
				   SXE_RX_HWTSTAMP_IN_REGISTER);
		break;
	case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
		*is_v1 = true;
		*tses |= SXE_TSES_TXES_V1_DELAY_REQ | SXE_TSES_RXES_V1_DELAY_REQ;
		*cap |= (SXE_RX_HWTSTAMP_ENABLED |
				   SXE_RX_HWTSTAMP_IN_REGISTER);
		break;
	case HWTSTAMP_FILTER_PTP_V2_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_L2_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_L2_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
		*is_l2 = true;
		*tses |= SXE_TSES_TXES_V2_ALL | SXE_TSES_RXES_V2_ALL;
		*rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
		*cap |= (SXE_RX_HWTSTAMP_ENABLED |
				   SXE_RX_HWTSTAMP_IN_REGISTER);
		break;
	case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
		*is_v1 = true;
		*tses |= SXE_TSES_TXES_V1_ALL | SXE_TSES_RXES_V1_ALL;
		*rx_filter = HWTSTAMP_FILTER_ALL;
		*cap |= SXE_RX_HWTSTAMP_ENABLED;
		break;
#ifndef HAVE_NO_HWTSTAMP_FILTER_NTP_ALL
	case HWTSTAMP_FILTER_NTP_ALL:
#endif
	case HWTSTAMP_FILTER_ALL:
		*tses |= SXE_TSES_TXES_V2_ALL | SXE_TSES_RXES_V2_ALL;
		*rx_filter = HWTSTAMP_FILTER_ALL;
		*cap |= SXE_RX_HWTSTAMP_ENABLED;
		break;
	default:
		*cap &= ~(SXE_RX_HWTSTAMP_ENABLED |
				    SXE_RX_HWTSTAMP_IN_REGISTER);
		*rx_filter = HWTSTAMP_FILTER_NONE;
		ret = -ERANGE;
	}

	return ret;
}

static int sxe_ptp_set_timestamp_mode(struct sxe_adapter *adapter,
				 	struct hwtstamp_config *config)
{
	struct sxe_hw *hw = &adapter->hw;
	u32 tsctl = 0x0;
	u32 tses = 0x0;
	bool is_l2 = false;
	bool is_v1 = false;
	s32 ret;

	if (config->flags) {
		ret = -EINVAL;
		goto l_ret;
	}

	LOG_DEBUG_BDF("ptp set timestamp mode: tx_type[0x%x], rx_filter[0x%x]\n",
			config->tx_type, config->rx_filter);

	ret = sxe_ptp_tx_type_get(config->tx_type, &tsctl);
	if (ret) {
		LOG_ERROR_BDF("ptp get tx type err ret = %d\n", ret);
		goto l_ret;
	}

	ret = sxe_ptp_rx_filter_get(&config->rx_filter, &adapter->cap,
				&is_v1, &is_l2, &tses);
	if (ret) {
		LOG_ERROR_BDF("ptp get rx filter err ret = %d\n", ret);
		goto l_ret;
	}


	LOG_DEBUG_BDF("hw[%p] set hw timestamp: is_l2=%s, tsctl=0x%x, tses=0x%x\n",
			hw, is_l2 ? "true" : "false", tsctl, tses);
	hw->dbu.ops->ptp_timestamp_mode_set(hw, is_l2, tsctl, tses);

	hw->dbu.ops->ptp_timestamp_enable(hw);

	sxe_ptp_clear_tx_timestamp(adapter);
	hw->dbu.ops->ptp_rx_timestamp_clear(hw);

#if 0	
	adapter->cap &= ~SXE_1588V2_ONE_STEP;
#endif

l_ret:
	return ret;
}

int sxe_ptp_hw_tstamp_config_set(struct sxe_adapter *adapter, struct ifreq *ifr)
{
	struct hwtstamp_config config;
	int ret;

	if (copy_from_user(&config, ifr->ifr_data, sizeof(config))) {
		ret = -EFAULT;
		goto l_ret;
	}

	ret = sxe_ptp_set_timestamp_mode(adapter, &config);
	if (ret) {
		LOG_ERROR_BDF("ptp set timestamp mode failed, err=%d\n",ret);
		goto l_ret;
	}

	memcpy(&adapter->ptp_ctxt.tstamp_config, &config,
	       sizeof(adapter->ptp_ctxt.tstamp_config));

	return copy_to_user(ifr->ifr_data, &config, sizeof(config)) ?
		-EFAULT : 0;

l_ret:
	return ret;
}

int sxe_ptp_hw_tstamp_config_get(struct sxe_adapter *adapter,
						struct ifreq *ifr)
{
	struct hwtstamp_config *config = &adapter->ptp_ctxt.tstamp_config;

	return copy_to_user(ifr->ifr_data, config,
			    sizeof(*config)) ? -EFAULT : 0;
}

static void sxe_ptp_cyclecounter_start(struct sxe_adapter *adapter)
{
	struct cyclecounter cc;
	unsigned long flags;
	struct sxe_hw *hw = &adapter->hw;

	cc.mask = CLOCKSOURCE_MASK(64);
	cc.mult = 1;
	cc.shift = 0;
	cc.read = sxe_ptp_read;

	hw->dbu.ops->ptp_systime_init(hw);

	smp_mb();

	spin_lock_irqsave(&adapter->ptp_ctxt.ptp_timer_lock, flags);
	memcpy(&adapter->ptp_ctxt.hw_cc, &cc, sizeof(adapter->ptp_ctxt.hw_cc));
	spin_unlock_irqrestore(&adapter->ptp_ctxt.ptp_timer_lock, flags);

	return;
}

static void sxe_ptp_hw_init(struct sxe_adapter *adapter)
{
	struct sxe_hw *hw = &adapter->hw;
	hw->dbu.ops->ptp_init(hw);
	return;
}

static inline void sxe_ptp_systime_init(struct sxe_adapter *adapter)
{
	unsigned long flags;

	spin_lock_irqsave(&adapter->ptp_ctxt.ptp_timer_lock, flags);
	timecounter_init(&adapter->ptp_ctxt.hw_tc, &adapter->ptp_ctxt.hw_cc,
			ktime_get_real_ns());
	spin_unlock_irqrestore(&adapter->ptp_ctxt.ptp_timer_lock, flags);

}

void sxe_ptp_reset(struct sxe_adapter *adapter)
{
	sxe_ptp_hw_init(adapter);

	sxe_ptp_set_timestamp_mode(adapter, &adapter->ptp_ctxt.tstamp_config);

	sxe_ptp_cyclecounter_start(adapter);

	sxe_ptp_systime_init(adapter);

	adapter->ptp_ctxt.last_overflow_check = jiffies;
	adapter->ptp_ctxt.tx_hwtstamp_nsec = 0;
	adapter->ptp_ctxt.tx_hwtstamp_sec = 0;

	return;
}

void sxe_ptp_configure(struct sxe_adapter *adapter)
{
	spin_lock_init(&adapter->ptp_ctxt.ptp_timer_lock);

	if (sxe_ptp_clock_create(adapter)) {
		LOG_DEBUG_BDF("create ptp err in addr:[%p]\n",
				adapter->ptp_ctxt.ptp_clock);
		goto l_end;
	}

	INIT_WORK(&adapter->ptp_ctxt.ptp_tx_work, sxe_ptp_tx_work_handler);

	sxe_ptp_reset(adapter);

	set_bit(SXE_PTP_RUNNING, &adapter->state);

l_end:
	return;
}

void sxe_ptp_suspend(struct sxe_adapter *adapter)
{
	if (!test_and_clear_bit(SXE_PTP_RUNNING, &adapter->state)) {
		goto l_ret;
	}

	adapter->cap &= ~SXE_PTP_PPS_ENABLED;
	if (adapter->ptp_ctxt.ptp_setup_spp) {
		adapter->ptp_ctxt.ptp_setup_spp(adapter);
	}

	cancel_work_sync(&adapter->ptp_ctxt.ptp_tx_work);
	sxe_ptp_clear_tx_timestamp(adapter);

l_ret:
	return;
}

void sxe_ptp_stop(struct sxe_adapter *adapter)
{
	sxe_ptp_suspend(adapter);

	if (adapter->ptp_ctxt.ptp_clock) {
		ptp_clock_unregister(adapter->ptp_ctxt.ptp_clock);
		adapter->ptp_ctxt.ptp_clock = NULL;
		LOG_DEV_INFO("removed PHC on %s\n",
			   adapter->netdev->name);
	}

	return;
}

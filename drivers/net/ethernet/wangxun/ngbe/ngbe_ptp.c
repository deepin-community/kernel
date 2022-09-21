/*
 * WangXun Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */


#include "ngbe.h"
#include <linux/ptp_classify.h>

/*
 * SYSTIME is defined by a fixed point system which allows the user to
 * define the scale counter increment value at every level change of
 * the oscillator driving SYSTIME value. The time unit is determined by
 * the clock frequency of the oscillator and TIMINCA register.
 * The cyclecounter and timecounter structures are used to to convert
 * the scale counter into nanoseconds. SYSTIME registers need to be converted
 * to ns values by use of only a right shift.
 * The following math determines the largest incvalue that will fit into
 * the available bits in the TIMINCA register:
 *   Period * [ 2 ^ ( MaxWidth - PeriodWidth ) ]
 * PeriodWidth: Number of bits to store the clock period
 * MaxWidth: The maximum width value of the TIMINCA register
 * Period: The clock period for the oscillator, which changes based on the link
 * speed:
 *   At 10Gb link or no link, the period is 6.4 ns.
 *   At 1Gb link, the period is multiplied by 10. (64ns)
 *   At 100Mb link, the period is multiplied by 100. (640ns)
 * round(): discard the fractional portion of the calculation
 *
 * The calculated value allows us to right shift the SYSTIME register
 * value in order to quickly convert it into a nanosecond clock,
 * while allowing for the maximum possible adjustment value.
 *
 * LinkSpeed    ClockFreq   ClockPeriod  TIMINCA:IV
 * 10000Mbps    156.25MHz   6.4*10^-9    0xCCCCCC(0xFFFFF/ns)
 * 1000 Mbps    62.5  MHz   16 *10^-9    0x800000(0x7FFFF/ns)
 * 100  Mbps    6.25  MHz   160*10^-9    0xA00000(0xFFFF/ns)
 * 10   Mbps    0.625 MHz   1600*10^-9   0xC7F380(0xFFF/ns)
 * FPGA         31.25 MHz   32 *10^-9    0x800000(0x3FFFF/ns)
 *
 * These diagrams are only for the 10Gb link period
 *
 *       +--------------+  +--------------+
 *       |      32      |  | 8 | 3 |  20  |
 *       *--------------+  +--------------+
 *        \________ 43 bits ______/  fract
 *
 * The 43 bit SYSTIME overflows every
 *   2^43 * 10^-9 / 3600 = 2.4 hours
 */
#define NGBE_INCVAL_10GB 0xCCCCCC
#define NGBE_INCVAL_1GB  0x2000000/*in Emerald  all speed is same*/
#define NGBE_INCVAL_100  0xA00000
#define NGBE_INCVAL_10   0xC7F380
#define NGBE_INCVAL_FPGA 0x800000

#define NGBE_INCVAL_SHIFT_10GB  20
#define NGBE_INCVAL_SHIFT_1GB   22/*in Emerald  all speed is same*/
#define NGBE_INCVAL_SHIFT_100   15
#define NGBE_INCVAL_SHIFT_10    12
#define NGBE_INCVAL_SHIFT_FPGA  17

#define NGBE_OVERFLOW_PERIOD    (HZ * 30)
#define NGBE_PTP_TX_TIMEOUT     (HZ)

/**
 * ngbe_ptp_read - read raw cycle counter (to be used by time counter)
 * @hw_cc: the cyclecounter structure
 *
 * this function reads the cyclecounter registers and is called by the
 * cyclecounter structure used to construct a ns counter from the
 * arbitrary fixed point registers
 */
static u64 ngbe_ptp_read(const struct cyclecounter *hw_cc)
{
	struct ngbe_adapter *adapter =
		container_of(hw_cc, struct ngbe_adapter, hw_cc);
	struct ngbe_hw *hw = &adapter->hw;
	u64 stamp = 0;

	stamp |= (u64)rd32(hw, NGBE_TSEC_1588_SYSTIML);
	stamp |= (u64)rd32(hw, NGBE_TSEC_1588_SYSTIMH) << 32;

	return stamp;
}

/**
 * ngbe_ptp_convert_to_hwtstamp - convert register value to hw timestamp
 * @adapter: private adapter structure
 * @hwtstamp: stack timestamp structure
 * @systim: unsigned 64bit system time value
 *
 * We need to convert the adapter's RX/TXSTMP registers into a hwtstamp value
 * which can be used by the stack's ptp functions.
 *
 * The lock is used to protect consistency of the cyclecounter and the SYSTIME
 * registers. However, it does not need to protect against the Rx or Tx
 * timestamp registers, as there can't be a new timestamp until the old one is
 * unlatched by reading.
 *
 * In addition to the timestamp in hardware, some controllers need a software
 * overflow cyclecounter, and this function takes this into account as well.
 **/
static void ngbe_ptp_convert_to_hwtstamp(struct ngbe_adapter *adapter,
					  struct skb_shared_hwtstamps *hwtstamp,
					  u64 timestamp)
{
	unsigned long flags;
	u64 ns;

	memset(hwtstamp, 0, sizeof(*hwtstamp));

	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	ns = timecounter_cyc2time(&adapter->hw_tc, timestamp);
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	hwtstamp->hwtstamp = ns_to_ktime(ns);
}

/**
 * ngbe_ptp_adjfreq
 * @ptp: the ptp clock structure
 * @ppb: parts per billion adjustment from base
 *
 * adjust the frequency of the ptp cycle counter by the
 * indicated ppb from the base frequency.
 */
static int ngbe_ptp_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
	struct ngbe_adapter *adapter =
		container_of(ptp, struct ngbe_adapter, ptp_caps);
	struct ngbe_hw *hw = &adapter->hw;
	u64 freq, incval;
	u32 diff;
	int neg_adj = 0;

	if (ppb < 0) {
		neg_adj = 1;
		ppb = -ppb;
	}

	smp_mb();
	incval = READ_ONCE(adapter->base_incval);

	freq = incval;
	freq *= ppb;
	diff = div_u64(freq, 1000000000ULL);

	incval = neg_adj ? (incval - diff) : (incval + diff);
	/* temp setting*/

	if (incval > NGBE_TSEC_1588_INC_IV(~0))
		e_dev_warn("PTP ppb adjusted SYSTIME rate overflowed!\n");
	wr32(hw, NGBE_TSEC_1588_INC, NGBE_TSEC_1588_INC_IV(incval));

	return 0;
}


/**
 * ngbe_ptp_adjtime
 * @ptp: the ptp clock structure
 * @delta: offset to adjust the cycle counter by ns
 *
 * adjust the timer by resetting the timecounter structure.
 */
static int ngbe_ptp_adjtime(struct ptp_clock_info *ptp,
					 s64 delta)
{
	struct ngbe_adapter *adapter =
		container_of(ptp, struct ngbe_adapter, ptp_caps);
	unsigned long flags;

	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	timecounter_adjtime(&adapter->hw_tc, delta);
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	return 0;
}

/**
 * ngbe_ptp_gettime64
 * @ptp: the ptp clock structure
 * @ts: timespec64 structure to hold the current time value
 *
 * read the timecounter and return the correct value on ns,
 * after converting it into a struct timespec64.
 */
static int ngbe_ptp_gettime64(struct ptp_clock_info *ptp,
			       struct timespec64 *ts)
{
	struct ngbe_adapter *adapter =
		container_of(ptp, struct ngbe_adapter, ptp_caps);
	unsigned long flags;
	u64 ns;

	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	ns = timecounter_read(&adapter->hw_tc);
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	*ts = ns_to_timespec64(ns);

	return 0;
}

/**
 * ngbe_ptp_settime64
 * @ptp: the ptp clock structure
 * @ts: the timespec64 containing the new time for the cycle counter
 *
 * reset the timecounter to use a new base value instead of the kernel
 * wall timer value.
 */
static int ngbe_ptp_settime64(struct ptp_clock_info *ptp,
					   const struct timespec64 *ts)
{
	struct ngbe_adapter *adapter =
		container_of(ptp, struct ngbe_adapter, ptp_caps);
	u64 ns;
	unsigned long flags;

	ns = timespec64_to_ns(ts);

	/* reset the timecounter */
	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	timecounter_init(&adapter->hw_tc, &adapter->hw_cc, ns);
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	return 0;
}

#ifndef HAVE_PTP_CLOCK_INFO_GETTIME64
static int ngbe_ptp_gettime(struct ptp_clock_info *ptp, struct timespec *ts)
{
	struct timespec64 ts64;
	int err;

	err = ngbe_ptp_gettime64(ptp, &ts64);
	if (err)
		return err;

	*ts = timespec64_to_timespec(ts64);

	return 0;
}

static int ngbe_ptp_settime(struct ptp_clock_info *ptp,
					 const struct timespec *ts)
{
	struct timespec64 ts64;

	ts64 = timespec_to_timespec64(*ts);
	return ngbe_ptp_settime64(ptp, &ts64);
}
#endif

/**
 * ngbe_ptp_feature_enable
 * @ptp: the ptp clock structure
 * @rq: the requested feature to change
 * @on: whether to enable or disable the feature
 *
 * enable (or disable) ancillary features of the phc subsystem.
 * our driver only supports the PPS feature on the X540
 */
static int ngbe_ptp_feature_enable(struct ptp_clock_info *ptp,
				    struct ptp_clock_request *rq, int on)
{
	return -ENOTSUPP;
}

/**
 * ngbe_ptp_check_pps_event
 * @adapter: the private adapter structure
 * @eicr: the interrupt cause register value
 *
 * This function is called by the interrupt routine when checking for
 * interrupts. It will check and handle a pps event.
 */
void ngbe_ptp_check_pps_event(struct ngbe_adapter *adapter)
{
	struct ptp_clock_event event;

	event.type = PTP_CLOCK_PPS;

	/* this check is necessary in case the interrupt was enabled via some
	 * alternative means (ex. debug_fs). Better to check here than
	 * everywhere that calls this function.
	 */
	if (!adapter->ptp_clock)
		return;

	/* we don't config PPS on SDP yet, so just return.
	 * ptp_clock_event(adapter->ptp_clock, &event);
	 */
}

/**
 * ngbe_ptp_overflow_check - watchdog task to detect SYSTIME overflow
 * @adapter: private adapter struct
 *
 * this watchdog task periodically reads the timecounter
 * in order to prevent missing when the system time registers wrap
 * around. This needs to be run approximately twice a minute for the fastest
 * overflowing hardware. We run it for all hardware since it shouldn't have a
 * large impact.
 */
void ngbe_ptp_overflow_check(struct ngbe_adapter *adapter)
{
	bool timeout = time_is_before_jiffies(adapter->last_overflow_check +
										NGBE_OVERFLOW_PERIOD);
	struct timespec64 ts;

	if (timeout) {
		ngbe_ptp_gettime64(&adapter->ptp_caps, &ts);
		adapter->last_overflow_check = jiffies;
	}
}

/**
 * ngbe_ptp_rx_hang - detect error case when Rx timestamp registers latched
 * @adapter: private network adapter structure
 *
 * this watchdog task is scheduled to detect error case where hardware has
 * dropped an Rx packet that was timestamped when the ring is full. The
 * particular error is rare but leaves the device in a state unable to timestamp
 * any future packets.
 */
void ngbe_ptp_rx_hang(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	struct ngbe_ring *rx_ring;
	u32 tsyncrxctl = rd32(hw, NGBE_PSR_1588_CTL);
	unsigned long rx_event;
	int n;

	/* if we don't have a valid timestamp in the registers, just update the
	 * timeout counter and exit
	 */
	if (!(tsyncrxctl & NGBE_PSR_1588_CTL_VALID)) {
		adapter->last_rx_ptp_check = jiffies;
		return;
	}

	/* determine the most recent watchdog or rx_timestamp event */
	rx_event = adapter->last_rx_ptp_check;
	for (n = 0; n < adapter->num_rx_queues; n++) {
		rx_ring = adapter->rx_ring[n];
		if (time_after(rx_ring->last_rx_timestamp, rx_event))
			rx_event = rx_ring->last_rx_timestamp;
	}

	/* only need to read the high RXSTMP register to clear the lock */
	if (time_is_before_jiffies(rx_event + 5 * HZ)) {
		rd32(hw, NGBE_PSR_1588_STMPH);
		adapter->last_rx_ptp_check = jiffies;

		adapter->rx_hwtstamp_cleared++;
		e_warn(drv, "clearing RX Timestamp hang");
	}
}

/**
 * ngbe_ptp_clear_tx_timestamp - utility function to clear Tx timestamp state
 * @adapter: the private adapter structure
 *
 * This function should be called whenever the state related to a Tx timestamp
 * needs to be cleared. This helps ensure that all related bits are reset for
 * the next Tx timestamp event.
 */
static void ngbe_ptp_clear_tx_timestamp(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;

	rd32(hw, NGBE_TSEC_1588_STMPH);
	if (adapter->ptp_tx_skb) {
		dev_kfree_skb_any(adapter->ptp_tx_skb);
		adapter->ptp_tx_skb = NULL;
	}
	clear_bit_unlock(__NGBE_PTP_TX_IN_PROGRESS, &adapter->state);
}

/**
 * ngbe_ptp_tx_hwtstamp - utility function which checks for TX time stamp
 * @adapter: the private adapter struct
 *
 * if the timestamp is valid, we convert it into the timecounter ns
 * value, then store that result into the shhwtstamps structure which
 * is passed up the network stack
 */
static void ngbe_ptp_tx_hwtstamp(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	struct skb_shared_hwtstamps shhwtstamps;
	u64 regval = 0;

	regval |= (u64)rd32(hw, NGBE_TSEC_1588_STMPL);
	regval |= (u64)rd32(hw, NGBE_TSEC_1588_STMPH) << 32;

	ngbe_ptp_convert_to_hwtstamp(adapter, &shhwtstamps, regval);
	skb_tstamp_tx(adapter->ptp_tx_skb, &shhwtstamps);

	ngbe_ptp_clear_tx_timestamp(adapter);
}

/**
 * ngbe_ptp_tx_hwtstamp_work
 * @work: pointer to the work struct
 *
 * This work item polls TSYNCTXCTL valid bit to determine when a Tx hardware
 * timestamp has been taken for the current skb. It is necesary, because the
 * descriptor's "done" bit does not correlate with the timestamp event.
 */
static void ngbe_ptp_tx_hwtstamp_work(struct work_struct *work)
{
	struct ngbe_adapter *adapter = container_of(work, struct ngbe_adapter,
										ptp_tx_work);
	struct ngbe_hw *hw = &adapter->hw;
	bool timeout = time_is_before_jiffies(adapter->ptp_tx_start +
										NGBE_PTP_TX_TIMEOUT);
	u32 tsynctxctl;

	/* we have to have a valid skb to poll for a timestamp */
	if (!adapter->ptp_tx_skb) {
		ngbe_ptp_clear_tx_timestamp(adapter);
		return;
	}

	/* stop polling once we have a valid timestamp */
	tsynctxctl = rd32(hw, NGBE_TSEC_1588_CTL);
	if (tsynctxctl & NGBE_TSEC_1588_CTL_VALID) {
		ngbe_ptp_tx_hwtstamp(adapter);
		return;
	}

	/* check timeout last in case timestamp event just occurred */
	if (timeout) {
		ngbe_ptp_clear_tx_timestamp(adapter);
		adapter->tx_hwtstamp_timeouts++;
		e_warn(drv, "clearing Tx Timestamp hang");
	} else {
		/* reschedule to keep checking until we timeout */
		schedule_work(&adapter->ptp_tx_work);
	}
}

/**
 * ngbe_ptp_rx_rgtstamp - utility function which checks for RX time stamp
 * @q_vector: structure containing interrupt and ring information
 * @skb: particular skb to send timestamp with
 *
 * if the timestamp is valid, we convert it into the timecounter ns
 * value, then store that result into the shhwtstamps structure which
 * is passed up the network stack
 */
void ngbe_ptp_rx_hwtstamp(struct ngbe_adapter *adapter, struct sk_buff *skb)
{
	struct ngbe_hw *hw = &adapter->hw;
	u64 regval = 0;
	u32 tsyncrxctl;

	/*
	 * Read the tsyncrxctl register afterwards in order to prevent taking an
	 * I/O hit on every packet.
	 */
	tsyncrxctl = rd32(hw, NGBE_PSR_1588_CTL);
	if (!(tsyncrxctl & NGBE_PSR_1588_CTL_VALID))
		return;

	regval |= (u64)rd32(hw, NGBE_PSR_1588_STMPL);
	regval |= (u64)rd32(hw, NGBE_PSR_1588_STMPH) << 32;

	ngbe_ptp_convert_to_hwtstamp(adapter, skb_hwtstamps(skb), regval);
}

/**
 * ngbe_ptp_get_ts_config - get current hardware timestamping configuration
 * @adapter: pointer to adapter structure
 * @ifreq: ioctl data
 *
 * This function returns the current timestamping settings. Rather than
 * attempt to deconstruct registers to fill in the values, simply keep a copy
 * of the old settings around, and return a copy when requested.
 */
int ngbe_ptp_get_ts_config(struct ngbe_adapter *adapter, struct ifreq *ifr)
{
	struct hwtstamp_config *config = &adapter->tstamp_config;

	return copy_to_user(ifr->ifr_data, config,
						sizeof(*config)) ? -EFAULT : 0;
}

/**
 * ngbe_ptp_set_timestamp_mode - setup the hardware for the requested mode
 * @adapter: the private ngbe adapter structure
 * @config: the hwtstamp configuration requested
 *
 * Outgoing time stamping can be enabled and disabled. Play nice and
 * disable it when requested, although it shouldn't cause any overhead
 * when no packet needs it. At most one packet in the queue may be
 * marked for time stamping, otherwise it would be impossible to tell
 * for sure to which packet the hardware time stamp belongs.
 *
 * Incoming time stamping has to be configured via the hardware
 * filters. Not all combinations are supported, in particular event
 * type has to be specified. Matching the kind of event packet is
 * not supported, with the exception of "all V2 events regardless of
 * level 2 or 4".
 *
 * Since hardware always timestamps Path delay packets when timestamping V2
 * packets, regardless of the type specified in the register, only use V2
 * Event mode. This more accurately tells the user what the hardware is going
 * to do anyways.
 *
 * Note: this may modify the hwtstamp configuration towards a more general
 * mode, if required to support the specifically requested mode.
 */
static int ngbe_ptp_set_timestamp_mode(struct ngbe_adapter *adapter,
					struct hwtstamp_config *config)
{
	struct ngbe_hw *hw = &adapter->hw;
	u32 tsync_tx_ctl = NGBE_TSEC_1588_CTL_ENABLED;
	u32 tsync_rx_ctl = NGBE_PSR_1588_CTL_ENABLED;
	u32 tsync_rx_mtrl = PTP_EV_PORT << 16;
	bool is_l2 = false;
	u32 regval;

	/* reserved for future extensions */
	if (config->flags)
		return -EINVAL;

	switch (config->tx_type) {
	case HWTSTAMP_TX_OFF:
		tsync_tx_ctl = 0;
	case HWTSTAMP_TX_ON:
		break;
	default:
		return -ERANGE;
	}

	switch (config->rx_filter) {
	case HWTSTAMP_FILTER_NONE:
		tsync_rx_ctl = 0;
		tsync_rx_mtrl = 0;
		adapter->flags &= ~(NGBE_FLAG_RX_HWTSTAMP_ENABLED |
							NGBE_FLAG_RX_HWTSTAMP_IN_REGISTER);
		break;
	case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
		tsync_rx_ctl |= NGBE_PSR_1588_CTL_TYPE_L4_V1;
		tsync_rx_mtrl |= NGBE_PSR_1588_MSGTYPE_V1_SYNC_MSG;
		adapter->flags |= (NGBE_FLAG_RX_HWTSTAMP_ENABLED |
							NGBE_FLAG_RX_HWTSTAMP_IN_REGISTER);
		break;
	case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
		tsync_rx_ctl |= NGBE_PSR_1588_CTL_TYPE_L4_V1;
		tsync_rx_mtrl |= NGBE_PSR_1588_MSGTYPE_V1_DELAY_REQ_MSG;
		adapter->flags |= (NGBE_FLAG_RX_HWTSTAMP_ENABLED |
							NGBE_FLAG_RX_HWTSTAMP_IN_REGISTER);
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
		tsync_rx_ctl |= NGBE_PSR_1588_CTL_TYPE_EVENT_V2;
		is_l2 = true;
		config->rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
		adapter->flags |= (NGBE_FLAG_RX_HWTSTAMP_ENABLED |
							NGBE_FLAG_RX_HWTSTAMP_IN_REGISTER);
		break;
	case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
	case HWTSTAMP_FILTER_ALL:
	default:
		/* register RXMTRL must be set in order to do V1 packets,
		 * therefore it is not possible to time stamp both V1 Sync and
		 * Delay_Req messages unless hardware supports timestamping all
		 * packets => return error
		 */
		adapter->flags &= ~(NGBE_FLAG_RX_HWTSTAMP_ENABLED |
							NGBE_FLAG_RX_HWTSTAMP_IN_REGISTER);
		config->rx_filter = HWTSTAMP_FILTER_NONE;
		return -ERANGE;
	}

	/* define ethertype filter for timestamping L2 packets */
	if (is_l2)
		wr32(hw,
			NGBE_PSR_ETYPE_SWC(NGBE_PSR_ETYPE_SWC_FILTER_1588),
			(NGBE_PSR_ETYPE_SWC_FILTER_EN | /* enable filter */
			 NGBE_PSR_ETYPE_SWC_1588 | /* enable timestamping */
			 ETH_P_1588));     /* 1588 eth protocol type */
	else
		wr32(hw,
			NGBE_PSR_ETYPE_SWC(NGBE_PSR_ETYPE_SWC_FILTER_1588),
			0);

	/* enable/disable TX */
	regval = rd32(hw, NGBE_TSEC_1588_CTL);
	regval &= ~NGBE_TSEC_1588_CTL_ENABLED;
	regval |= tsync_tx_ctl;
	wr32(hw, NGBE_TSEC_1588_CTL, regval);

	/* enable/disable RX */
	regval = rd32(hw, NGBE_PSR_1588_CTL);
	regval &= ~(NGBE_PSR_1588_CTL_ENABLED | NGBE_PSR_1588_CTL_TYPE_MASK);
	regval |= tsync_rx_ctl;
	wr32(hw, NGBE_PSR_1588_CTL, regval);

	/* define which PTP packets are time stamped */
	wr32(hw, NGBE_PSR_1588_MSGTYPE, tsync_rx_mtrl);

	NGBE_WRITE_FLUSH(hw);

	/* clear TX/RX timestamp state, just to be sure */
	ngbe_ptp_clear_tx_timestamp(adapter);
	rd32(hw, NGBE_PSR_1588_STMPH);

	return 0;
}

/**
 * ngbe_ptp_set_ts_config - user entry point for timestamp mode
 * @adapter: pointer to adapter struct
 * @ifreq: ioctl data
 *
 * Set hardware to requested mode. If unsupported, return an error with no
 * changes. Otherwise, store the mode for future reference.
 */
int ngbe_ptp_set_ts_config(struct ngbe_adapter *adapter, struct ifreq *ifr)
{
	struct hwtstamp_config config;
	int err;

	if (copy_from_user(&config, ifr->ifr_data, sizeof(config)))
		return -EFAULT;

	err = ngbe_ptp_set_timestamp_mode(adapter, &config);
	if (err)
		return err;

	/* save these settings for future reference */
	memcpy(&adapter->tstamp_config, &config,
			sizeof(adapter->tstamp_config));

	return copy_to_user(ifr->ifr_data, &config, sizeof(config)) ?
		-EFAULT : 0;
}

static void ngbe_ptp_link_speed_adjust(struct ngbe_adapter *adapter,
					u32 *shift, u32 *incval)
{
	/**
	 * Scale the NIC cycle counter by a large factor so that
	 * relatively small corrections to the frequency can be added
	 * or subtracted. The drawbacks of a large factor include
	 * (a) the clock register overflows more quickly, (b) the cycle
	 * counter structure must be able to convert the systime value
	 * to nanoseconds using only a multiplier and a right-shift,
	 * and (c) the value must fit within the timinca register space
	 * => math based on internal DMA clock rate and available bits
	 *
	 * Note that when there is no link, internal DMA clock is same as when
	 * link speed is 10Gb. Set the registers correctly even when link is
	 * down to preserve the clock setting
	 */

	*shift = NGBE_INCVAL_SHIFT_1GB;
	*incval = NGBE_INCVAL_1GB;

	return;
}

/**
 * ngbe_ptp_start_cyclecounter - create the cycle counter from hw
 * @adapter: pointer to the adapter structure
 *
 * This function should be called to set the proper values for the TIMINCA
 * register and tell the cyclecounter structure what the tick rate of SYSTIME
 * is. It does not directly modify SYSTIME registers or the timecounter
 * structure. It should be called whenever a new TIMINCA value is necessary,
 * such as during initialization or when the link speed changes.
 */
void ngbe_ptp_start_cyclecounter(struct ngbe_adapter *adapter)
{
	struct ngbe_hw *hw = &adapter->hw;
	unsigned long flags;
	struct cyclecounter cc;
	u32 incval = 0;

	/* For some of the boards below this mask is technically incorrect.
	 * The timestamp mask overflows at approximately 61bits. However the
	 * particular hardware does not overflow on an even bitmask value.
	 * Instead, it overflows due to conversion of upper 32bits billions of
	 * cycles. Timecounters are not really intended for this purpose so
	 * they do not properly function if the overflow point isn't 2^N-1.
	 * However, the actual SYSTIME values in question take ~138 years to
	 * overflow. In practice this means they won't actually overflow. A
	 * proper fix to this problem would require modification of the
	 * timecounter delta calculations.
	 */
	cc.mask = CLOCKSOURCE_MASK(64);
	cc.mult = 1;
	cc.shift = 0;

	cc.read = ngbe_ptp_read;
	ngbe_ptp_link_speed_adjust(adapter, &cc.shift, &incval);
	wr32(hw, NGBE_TSEC_1588_INC, NGBE_TSEC_1588_INC_IV(incval));

	/* update the base incval used to calculate frequency adjustment */
	WRITE_ONCE(adapter->base_incval, incval);
	smp_mb();

	/* need lock to prevent incorrect read while modifying cyclecounter */
	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	memcpy(&adapter->hw_cc, &cc, sizeof(adapter->hw_cc));
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);
}

/**
 * ngbe_ptp_reset
 * @adapter: the ngbe private board structure
 *
 * When the MAC resets, all of the hardware configuration for timesync is
 * reset. This function should be called to re-enable the device for PTP,
 * using the last known settings. However, we do lose the current clock time,
 * so we fallback to resetting it based on the kernel's realtime clock.
 *
 * This function will maintain the hwtstamp_config settings, and it retriggers
 * the SDP output if it's enabled.
 */
void ngbe_ptp_reset(struct ngbe_adapter *adapter)
{
	unsigned long flags;

	/* reset the hardware timestamping mode */
	ngbe_ptp_set_timestamp_mode(adapter, &adapter->tstamp_config);
	ngbe_ptp_start_cyclecounter(adapter);

	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	timecounter_init(&adapter->hw_tc, &adapter->hw_cc,
			 ktime_to_ns(ktime_get_real()));
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	adapter->last_overflow_check = jiffies;
}

/**
 * ngbe_ptp_create_clock
 * @adapter: the ngbe private adapter structure
 *
 * This function performs setup of the user entry point function table and
 * initalizes the PTP clock device used by userspace to access the clock-like
 * features of the PTP core. It will be called by ngbe_ptp_init, and may
 * re-use a previously initialized clock (such as during a suspend/resume
 * cycle).
 */

static long ngbe_ptp_create_clock(struct ngbe_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	long err;

	/* do nothing if we already have a clock device */
	if (!IS_ERR_OR_NULL(adapter->ptp_clock))
		return 0;

	snprintf(adapter->ptp_caps.name, sizeof(adapter->ptp_caps.name),
		 "%s", netdev->name);
	adapter->ptp_caps.owner = THIS_MODULE;
	adapter->ptp_caps.max_adj = 500000000; /* 10^-9s */
	adapter->ptp_caps.n_alarm = 0;
	adapter->ptp_caps.n_ext_ts = 0;
	adapter->ptp_caps.n_per_out = 0;
	adapter->ptp_caps.pps = 0;
	adapter->ptp_caps.adjfreq = ngbe_ptp_adjfreq;
	adapter->ptp_caps.adjtime = ngbe_ptp_adjtime;
#ifdef HAVE_PTP_CLOCK_INFO_GETTIME64
	adapter->ptp_caps.gettime64 = ngbe_ptp_gettime64;
	adapter->ptp_caps.settime64 = ngbe_ptp_settime64;
#else
	adapter->ptp_caps.gettime = ngbe_ptp_gettime;
	adapter->ptp_caps.settime = ngbe_ptp_settime;
#endif
	adapter->ptp_caps.enable = ngbe_ptp_feature_enable;

	adapter->ptp_clock = ptp_clock_register(&adapter->ptp_caps,
						pci_dev_to_dev(adapter->pdev));
	if (IS_ERR(adapter->ptp_clock)) {
		err = PTR_ERR(adapter->ptp_clock);
		adapter->ptp_clock = NULL;
		e_dev_err("ptp_clock_register failed\n");
		return err;
	} else
		e_dev_info("registered PHC device on %s\n", netdev->name);

	/* Set the default timestamp mode to disabled here. We do this in
	 * create_clock instead of initialization, because we don't want to
	 * override the previous settings during a suspend/resume cycle.
	 */
	adapter->tstamp_config.rx_filter = HWTSTAMP_FILTER_NONE;
	adapter->tstamp_config.tx_type = HWTSTAMP_TX_OFF;

	return 0;
}

/**
 * ngbe_ptp_init
 * @adapter: the ngbe private adapter structure
 *
 * This function performs the required steps for enabling ptp
 * support. If ptp support has already been loaded it simply calls the
 * cyclecounter init routine and exits.
 */
void ngbe_ptp_init(struct ngbe_adapter *adapter)
{
	/* initialize the spin lock first, since the user might call the clock
	 * functions any time after we've initialized the ptp clock device.
	 */
	spin_lock_init(&adapter->tmreg_lock);

	/* obtain a ptp clock device, or re-use an existing device */
	if (ngbe_ptp_create_clock(adapter))
		return;

	/* we have a clock, so we can intialize work for timestamps now */
	INIT_WORK(&adapter->ptp_tx_work, ngbe_ptp_tx_hwtstamp_work);

	/* reset the ptp related hardware bits */
	ngbe_ptp_reset(adapter);

	/* enter the NGBE_PTP_RUNNING state */
	set_bit(__NGBE_PTP_RUNNING, &adapter->state);

	return;
}

/**
 * ngbe_ptp_suspend - stop ptp work items
 * @adapter: pointer to adapter struct
 *
 * This function suspends ptp activity, and prevents more work from being
 * generated, but does not destroy the clock device.
 */
void ngbe_ptp_suspend(struct ngbe_adapter *adapter)
{
	/* leave the NGBE_PTP_RUNNING STATE */
	if (!test_and_clear_bit(__NGBE_PTP_RUNNING, &adapter->state))
		return;

	adapter->flags2 &= ~NGBE_FLAG2_PTP_PPS_ENABLED;

	cancel_work_sync(&adapter->ptp_tx_work);
	ngbe_ptp_clear_tx_timestamp(adapter);
}

/**
 * ngbe_ptp_stop - destroy the ptp_clock device
 * @adapter: pointer to adapter struct
 *
 * Completely destroy the ptp_clock device, and disable all PTP related
 * features. Intended to be run when the device is being closed.
 */
void ngbe_ptp_stop(struct ngbe_adapter *adapter)
{
	/* first, suspend ptp activity */
	ngbe_ptp_suspend(adapter);

	/* now destroy the ptp clock device */
	if (adapter->ptp_clock) {
		ptp_clock_unregister(adapter->ptp_clock);
		adapter->ptp_clock = NULL;
		e_dev_info("removed PHC on %s\n",
					adapter->netdev->name);
	}
}

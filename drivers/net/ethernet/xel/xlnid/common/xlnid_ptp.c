// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2008 - 2023 Xel Technology. */

#include "xlnid.h"
#include <linux/ptp_classify.h>
#include "xlnid_debug.h"

#if 0
#define DEBUG_PTP   printk
#else
#define DEBUG_PTP(fmt, ...)
#endif

#define XLNID_INCVAL_10GB 0x66666666
#define XLNID_INCVAL_1GB  0x40000000
#define XLNID_INCVAL_100  0x50000000

#define XLNID_INCVAL_SHIFT_10GB  28
#define XLNID_INCVAL_SHIFT_1GB   24
#define XLNID_INCVAL_SHIFT_100   21

#define XLNID_INCVAL_SHIFT       7
#define XLNID_INCPER_SHIFT       24

#define XLNID_OVERFLOW_PERIOD    (HZ * 30)
#define XLNID_PTP_TX_TIMEOUT     (HZ)

/*  half of a one second clock period, for use with PPS signal. We have to use
		this instead of something pre-defined like XLNID_PTP_PPS_HALF_SECOND, in
		order to force at least 64bits of precision for shifting
*/
#define XLNID_PTP_PPS_HALF_SECOND 500000000ULL
#define INCVALUE_MASK   0x7FFFFFFF
#define ISGN        0x80000000
#define MAX_TIMADJ  0x7FFFFFFF

extern xel_pci_info_t xel_pci_info[16];
extern xel_pci_info_t *xlnid_find_pci_info(unsigned char bus_number,
		unsigned int devfn);

/**
		xlnid_ptp_convert_to_hwtstamp - convert register value to hw timestamp
		@adapter: private adapter structure
		@hwtstamp: stack timestamp structure
		@timestamp: unsigned 64bit system time value

		We need to convert the adapter's RX/TXSTMP registers into a hwtstamp value
		which can be used by the stack's ptp functions.

		The lock is used to protect consistency of the cyclecounter and the SYSTIME
		registers. However, it does not need to protect against the Rx or Tx
		timestamp registers, as there can't be a new timestamp until the old one is
		unlatched by reading.

		In addition to the timestamp in hardware, some controllers need a software
		overflow cyclecounter, and this function takes this into account as well.
	**/
static void xlnid_ptp_convert_to_hwtstamp(struct xlnid_adapter *adapter,
		struct skb_shared_hwtstamps *hwtstamp, u64 timestamp)
{
	struct timespec64 systime;
	u64 ns = 0;

	memset(hwtstamp, 0, sizeof(*hwtstamp));

	systime.tv_sec = timestamp >> 32;
	systime.tv_nsec = timestamp & 0xFFFFFFFF;

	ns = timespec64_to_ns(&systime);

	hwtstamp->hwtstamp = ns_to_ktime(ns);
}

/**
		xlnid_ptp_adjfreq_westlake
		@ptp: the ptp clock structure
		@ppb: parts per billion adjustment from base

		adjust the frequency of the ptp cycle counter by the
		indicated ppb from the base frequency.
*/
static int xlnid_ptp_adjfreq_westlake(struct ptp_clock_info *ptp, s32 ppb)
{
	struct xlnid_adapter *adapter = container_of(ptp, struct xlnid_adapter,
									ptp_caps);
	struct xlnid_hw *hw = &adapter->hw;
	u64 ns = 0;
	unsigned long flags;

	ns = abs(ppb);

	if (ppb == 0 || abs(ppb) > adapter->ptp_caps.max_adj) {
		return 0;
	}

	spin_lock_irqsave(&adapter->tmreg_lock, flags);

	/* update */
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDR_TS0, ns & 0xffffffff);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDR_TS1, ns >> 32);

	if (ppb < 0) {
		/* minus */
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_ADD_SUB_FLAG, 1);
	}

	else {
		/* plus */
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_ADD_SUB_FLAG, 0);
	}

	/* offset */
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_UPDATE_TYPE, 1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDT_VID, 1);
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	return 0;
}

#ifdef HAVE_PTP_CLOCK_INFO_ADJFINE

/**
		xlnid_scaled_ppm_to_ppb() - convert scaled ppm to ppb

		@ppm:    Parts per million, but with a 16 bit binary fractional field
*/
static inline long xlnid_scaled_ppm_to_ppb(long ppm)
{
	/*
		The 'freq' field in the 'struct timex' is in parts per
		million, but with a 16 bit binary fractional field.

		We want to calculate

			ppb = scaled_ppm * 1000 / 2^16

		which simplifies to

			ppb = scaled_ppm * 125 / 2^13
	*/
	s64 ppb = 1 + ppm;

	ppb *= 125;
	ppb >>= 13;
	return (long)ppb;
}

/**
	xlnid_ptp_adjfine_westlake
	@ptp: the ptp clock structure
	@scaled_ppm: scaled parts per million adjustment from base

	Adjust the frequency of the clock by the indicated scaled_ppm
	from base frequency.

	Scaled parts per million is ppm with a 16-bit binary fractional field.
*/
static int xlnid_ptp_adjfine_westlake(struct ptp_clock_info *ptp,
										long scaled_ppm)
{
	long ppb = 0;

	ppb = xlnid_scaled_ppm_to_ppb(scaled_ppm);
	return xlnid_ptp_adjfreq_westlake(ptp, ppb);
}
#endif

/**
		xlnid_ptp_adjtime_timecounter
		@ptp: the ptp clock structure
		@delta: offset to adjust the cycle counter by

		adjust the timer by resetting the timecounter structure.
*/
static int xlnid_ptp_adjtime_timecounter(struct ptp_clock_info *ptp, s64 delta)
{
	struct xlnid_adapter *adapter = container_of(ptp, struct xlnid_adapter,
									ptp_caps);
	struct xlnid_hw *hw = &adapter->hw;
	unsigned long flags;
	struct timespec64 ts;

	ts = ns_to_timespec64(abs(delta));

	spin_lock_irqsave(&adapter->tmreg_lock, flags);

	if (delta > 0) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_ADD_SUB_FLAG, 0);
	}

	else {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_ADD_SUB_FLAG, 1);
	}

	/* offset */
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_UPDATE_TYPE, 1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDR_TS0, ts.tv_nsec);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDR_TS1, ts.tv_sec);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDT_VID, 1);
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	//timecounter_adjtime(&adapter->hw_tc, delta);

	return 0;
}

/**
		xlnid_ptp_gettime64_timecounter
		@ptp: the ptp clock structure
		@ts: timespec64 structure to hold the current time value

		read the timecounter and return the correct value on ns,
		after converting it into a struct timespec64.
*/
static int xlnid_ptp_gettime64_timecounter(struct ptp_clock_info *ptp,
		struct timespec64 *ts)
{
	struct xlnid_adapter *adapter = container_of(ptp, struct xlnid_adapter,
									ptp_caps);
	struct xlnid_hw *hw = &adapter->hw;
	unsigned long flags;

	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	ts->tv_nsec = XLNID_READ_REG_MAC(hw, WESTLAKE_RPT_CURR_TS0);
	ts->tv_sec = XLNID_READ_REG_MAC(hw, WESTLAKE_RPT_CURR_TS1);

	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	return 0;
}

/**
		xlnid_ptp_settime64_timecounter
		@ptp: the ptp clock structure
		@ts: the timespec64 containing the new time for the cycle counter

		reset the timecounter to use a new base value instead of the kernel
		wall timer value.
*/
static int xlnid_ptp_settime64_timecounter(struct ptp_clock_info *ptp,
		const struct timespec64 *ts)
{
	struct xlnid_adapter *adapter = container_of(ptp, struct xlnid_adapter,
									ptp_caps);
	struct xlnid_hw *hw = &adapter->hw;
	unsigned long flags;

	spin_lock_irqsave(&adapter->tmreg_lock, flags);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDR_TS0, ts->tv_nsec);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDR_TS1, ts->tv_sec);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_UPDATE_TYPE, 0);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDT_VID, 1);
	spin_unlock_irqrestore(&adapter->tmreg_lock, flags);

	return 0;
}

#ifndef HAVE_PTP_CLOCK_INFO_GETTIME64
static int xlnid_ptp_gettime_timecounter(struct ptp_clock_info *ptp,
		struct timespec *ts)
{
	struct timespec64 ts64;
	int err;

	err = xlnid_ptp_gettime64_timecounter(ptp, &ts64);

	if (err) {
		return err;
	}

	*ts = timespec64_to_timespec(ts64);

	return 0;
}

static int xlnid_ptp_settime_timecounter(struct ptp_clock_info *ptp,
		const struct timespec *ts)
{
	struct timespec64 ts64;

	ts64 = timespec_to_timespec64(*ts);
	return xlnid_ptp_settime64_timecounter(ptp, &ts64);
}
#endif

/**
		xlnid_ptp_feature_enable
		@ptp: the ptp clock structure
		@rq: the requested feature to change
		@on: whether to enable or disable the feature

		enable (or disable) ancillary features of the phc subsystem.
*/
static int xlnid_ptp_feature_enable(struct ptp_clock_info *ptp,
									struct ptp_clock_request *rq, int on)
{
	struct xlnid_adapter *adapter = container_of(ptp, struct xlnid_adapter,
									ptp_caps);

	/**
		When PPS is enabled, unmask the interrupt for the ClockOut
		feature, so that the interrupt handler can send the PPS
		event when the clock SDP triggers. Clear mask when PPS is
		disabled
	*/
	if (rq->type == PTP_CLK_REQ_PPS && adapter->ptp_setup_sdp) {
		if (on) {
			adapter->flags2 |= XLNID_FLAG2_PTP_PPS_ENABLED;
		}

		else {
			adapter->flags2 &= ~XLNID_FLAG2_PTP_PPS_ENABLED;
		}

		adapter->ptp_setup_sdp(adapter);
		return 0;
	}

	return -ENOTSUPP;
}

/**
		xlnid_ptp_check_pps_event
		@adapter: the private adapter structure

		This function is called by the interrupt routine when checking for
		interrupts. It will check and handle a pps event.
*/
void xlnid_ptp_check_pps_event(struct xlnid_adapter *adapter)
{
	struct ptp_clock_event event;

	event.type = PTP_CLOCK_PPS;

	/*  this check is necessary in case the interrupt was enabled via some
		alternative means (ex. debug_fs). Better to check here than
		everywhere that calls this function.
	*/
	if (!adapter->ptp_clock) {
		return;
	}

	return;
}

/**
		xlnid_ptp_overflow_check - watchdog task to detect SYSTIME overflow
		@adapter: private adapter struct

		this watchdog task periodically reads the timecounter
		in order to prevent missing when the system time registers wrap
		around. This needs to be run approximately twice a minute for the fastest
		overflowing hardware. We run it for all hardware since it shouldn't have a
		large impact.
*/
void xlnid_ptp_overflow_check(struct xlnid_adapter *adapter)
{
	bool timeout = time_is_before_jiffies(adapter->last_overflow_check +
											XLNID_OVERFLOW_PERIOD);
	struct timespec64 ts;

	if (timeout) {
		xlnid_ptp_gettime64_timecounter(&adapter->ptp_caps, &ts);
		adapter->last_overflow_check = jiffies;
	}
}

/**
		xlnid_ptp_rx_hang - detect error case when Rx timestamp registers latched
		@adapter: private network adapter structure

		this watchdog task is scheduled to detect error case where hardware has
		dropped an Rx packet that was timestamped when the ring is full. The
		particular error is rare but leaves the device in a state unable to timestamp
		any future packets.
*/
void xlnid_ptp_rx_hang(struct xlnid_adapter *adapter)
{
	return;
}

/**
		xlnid_ptp_clear_tx_timestamp - utility function to clear Tx timestamp state
		@adapter: the private adapter structure

		This function should be called whenever the state related to a Tx timestamp
		needs to be cleared. This helps ensure that all related bits are reset for
		the next Tx timestamp event.
*/
static void xlnid_ptp_clear_tx_timestamp(struct xlnid_adapter *adapter)
{
	if (adapter->ptp_tx_skb) {
		dev_kfree_skb_any(adapter->ptp_tx_skb);
		adapter->ptp_tx_skb = NULL;
	}

	clear_bit_unlock(__XLNID_PTP_TX_IN_PROGRESS, &adapter->state);
}

/**
		xlnid_ptp_tx_hang - detect error case where Tx timestamp never finishes
		@adapter: private network adapter structure
*/
void xlnid_ptp_tx_hang(struct xlnid_adapter *adapter)
{
	return;
}

/**
		xlnid_ptp_tx_hwtstamp - utility function which checks for TX time stamp
		@adapter: the private adapter struct

		if the timestamp is valid, we convert it into the timecounter ns
		value, then store that result into the shhwtstamps structure which
		is passed up the network stack
*/
static void xlnid_ptp_tx_hwtstamp(struct xlnid_adapter *adapter,
									struct xlnid_hw *hw)
{
	struct sk_buff *skb = adapter->ptp_tx_skb;
	struct skb_shared_hwtstamps shhwtstamps;
	u64 regval = 0;

	regval |= (u64) XLNID_READ_REG_MAC(hw, WESTLAKE_RPT_TX_SNAP_TS0);
	regval |= (u64) XLNID_READ_REG_MAC(hw, WESTLAKE_RPT_TX_SNAP_TS1) << 32;

	DEBUG_PTP("%s:%d: regval=%lld\r\n", __func__, __LINE__, regval);
	xlnid_ptp_convert_to_hwtstamp(adapter, &shhwtstamps, regval);

	/*  Handle cleanup of the ptp_tx_skb ourselves, and unlock the state
		bit prior to notifying the stack via skb_tstamp_tx(). This prevents
		well behaved applications from attempting to timestamp again prior
		to the lock bit being clear.
	*/
	adapter->ptp_tx_skb = NULL;
	clear_bit_unlock(__XLNID_PTP_TX_IN_PROGRESS, &adapter->state);

	/* Notify the stack and then free the skb after we've unlocked */
	skb_tstamp_tx(skb, &shhwtstamps);
	dev_kfree_skb_any(skb);
}

static int xlnid_seqid_match(struct sk_buff *skb, unsigned int type,
								uint32_t reg_seqid, int tx_type)
{
	u16 *seqid = 0;
	unsigned int offset = 0;
	u8 *msgtype = 0;
	u8 *data = skb->data;

	/* check sequenceID */
	switch (type) {
	case PTP_CLASS_V2_IPV4:
		offset = ETH_HLEN + IPV4_HLEN(data) + UDP_HLEN;
		break;

	case PTP_CLASS_V2_IPV6:
		#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0))
		offset = ETH_HLEN + IP6_HLEN + UDP_HLEN;
		#else
		offset += ETH_HLEN + IP6_HLEN + UDP_HLEN;
		#endif
		break;

	case PTP_CLASS_V2_L2:
		offset = ETH_HLEN;
		break;

	case PTP_CLASS_V2_VLAN:
		offset = ETH_HLEN + VLAN_HLEN;
		break;

	default:
		return 0;
	}

	if (skb->len + ETH_HLEN < offset + OFF_PTP_SEQUENCE_ID + sizeof(*seqid)) {
		return 0;
	}

	msgtype = data + offset;

	seqid = (u16 *)(data + offset + OFF_PTP_SEQUENCE_ID);

	DEBUG_PTP("%s:%d: msgtype = 0x%x seqid=%d reg_seqid=%d\r\n", __func__, __LINE__,
				*msgtype, ntohs(*seqid), reg_seqid);

	/* WESTLAKE-644: onestep sync not report hwtstamp */
	if (tx_type == HWTSTAMP_TX_ONESTEP_SYNC && *msgtype == 0x00) {
		return 0;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))

	if (tx_type == HWTSTAMP_TX_ONESTEP_P2P && (*msgtype == 0x00
			|| *msgtype == 0x03)) {
		return 0;
	}

#endif

	return (reg_seqid == ntohs(*seqid));
}

/**
		xlnid_ptp_tx_hwtstamp_work
		@work: pointer to the work struct

		This work item polls TSYNCTXCTL valid bit to determine when a Tx hardware
		timestamp has been taken for the current skb. It is necesary, because the
		descriptor's "done" bit does not correlate with the timestamp event.
*/
static void xlnid_ptp_tx_hwtstamp_work(struct work_struct *work)
{
	struct xlnid_adapter *adapter = container_of(work, struct xlnid_adapter,
									ptp_tx_work);
	struct xlnid_hw *hw = &adapter->hw;
	bool timeout = time_is_before_jiffies(adapter->ptp_tx_start +
											XLNID_PTP_TX_TIMEOUT);
	u32 tsynctxctl = 0;
	unsigned int type = 0;
	uint32_t tx_seqid = 0;
	int tx_type = 0;
	xel_pci_info_t *slave_pinfo = NULL;

	if (hw->ptsw_enable && hw->bus.func == 0 && hw->ptsw_backup) {
		slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
		hw = slave_pinfo->hw;
	}

	/* we have to have a valid skb to poll for a timestamp */
	if (!adapter->ptp_tx_skb) {
		xlnid_ptp_clear_tx_timestamp(adapter);
		return;
	}

	tx_type = adapter->tstamp_config.tx_type;
	tsynctxctl = XLNID_READ_REG_MAC(hw, WESTLAKE_SNAP_TS_RDY);

	if ((tsynctxctl & XLNID_TX_SNAP_TS_RDY) == XLNID_TX_SNAP_TS_RDY) {
		type = ptp_classify_raw(adapter->ptp_tx_skb);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_TX_SNAPTS_RDEN, 0x1);
		tx_seqid = XLNID_READ_REG_MAC(hw, WESTLAKE_RPT_TX_SNAP_TS2);

		if (xlnid_seqid_match(adapter->ptp_tx_skb, type, tx_seqid, tx_type)) {
			xlnid_ptp_tx_hwtstamp(adapter, hw);
			return;
		}

		if (tx_type >= HWTSTAMP_TX_ONESTEP_SYNC) {
			xlnid_ptp_clear_tx_timestamp(adapter);
			return;
		}
	}

	/* check timeout last in case timestamp event just occurred */
	if (timeout) {
		xlnid_ptp_clear_tx_timestamp(adapter);
		adapter->tx_hwtstamp_timeouts++;
		e_warn(drv, "clearing Tx Timestamp hang");
	}

	else {
		/* reschedule to keep checking until we timeout */
		schedule_work(&adapter->ptp_tx_work);
	}
}

/**
		xlnid_ptp_rx_pktstamp - utility function to get RX time stamp from buffer
		@q_vector: structure containing interrupt and ring information
		@skb: the packet

		This function will be called by the Rx routine of the timestamp for this
		packet is stored in the buffer. The value is stored in little endian format
		starting at the end of the packet data.
*/
void xlnid_ptp_rx_pktstamp(struct xlnid_q_vector *q_vector, struct sk_buff *skb)
{
	__le64 regval;

	/* copy the bits out of the skb, and then trim the skb length */
	skb_copy_bits(skb, skb->len - XLNID_TS_HDR_LEN, &regval, XLNID_TS_HDR_LEN);
	__pskb_trim(skb, skb->len - XLNID_TS_HDR_LEN);

	/*  The timestamp is recorded in little endian format, and is stored at
		the end of the packet.

		DWORD: N              N + 1      N + 2
		Field: End of Packet  SYSTIMH    SYSTIML
	*/
	xlnid_ptp_convert_to_hwtstamp(q_vector->adapter, skb_hwtstamps(skb),
									le64_to_cpu(regval));
}

/**
		xlnid_ptp_rx_rgtstamp - utility function which checks for RX time stamp
		@q_vector: structure containing interrupt and ring information
		@skb: particular skb to send timestamp with

		if the timestamp is valid, we convert it into the timecounter ns
		value, then store that result into the shhwtstamps structure which
		is passed up the network stack
*/
void xlnid_ptp_rx_rgtstamp(struct xlnid_q_vector *q_vector, struct sk_buff *skb)
{
	struct xlnid_adapter *adapter;
	struct xlnid_hw *hw;
	u64 regval = 0;
	u32 tsyncrxctl = 0;
	unsigned int type = 0;
	uint32_t rx_seqid = 0;
	xel_pci_info_t *slave_pinfo = NULL;

	/* we cannot process timestamps on a ring without a q_vector */
	if (!q_vector || !q_vector->adapter) {
		return;
	}

	adapter = q_vector->adapter;
	hw = &adapter->hw;

	if (hw->ptsw_enable && hw->bus.func == 0 && hw->ptsw_backup) {
		slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
		hw = slave_pinfo->hw;
	}

	/*
		Read the tsyncrxctl register afterwards in order to prevent taking an
		I/O hit on every packet.
	*/
	tsyncrxctl = XLNID_READ_REG_MAC(hw, WESTLAKE_SNAP_TS_RDY);

	if (tsyncrxctl == XLNID_FAILED_READ_REG) {
		return;
	}

	if (!(tsyncrxctl & XLNID_RX_SNAP_TS_RDY)) {
		return;
	}

	while (tsyncrxctl & XLNID_RX_SNAP_TS_RDY) {
		type = ptp_classify_raw(skb);

		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_RX_SNAPTS_RDEN, 0x1);
		usec_delay(60);
		rx_seqid = XLNID_READ_REG_MAC(hw, WESTLAKE_RPT_RX_SNAP_TS2);

		if (xlnid_seqid_match(skb, type, rx_seqid, 0)) {
			regval = (u64) XLNID_READ_REG_MAC(hw, WESTLAKE_RPT_RX_SNAP_TS0);
			regval |= (u64) XLNID_READ_REG_MAC(hw, WESTLAKE_RPT_RX_SNAP_TS1) << 32;
			break;
		}

		tsyncrxctl = XLNID_READ_REG_MAC(hw, WESTLAKE_SNAP_TS_RDY);
	}

	if (regval != 0) {
		xlnid_ptp_convert_to_hwtstamp(adapter, skb_hwtstamps(skb), regval);
	}
}

/**
		xlnid_ptp_get_ts_config - get current hardware timestamping configuration
		@adapter: pointer to adapter structure
		@ifr: ioctl data

		This function returns the current timestamping settings. Rather than
		attempt to deconstruct registers to fill in the values, simply keep a copy
		of the old settings around, and return a copy when requested.
*/
int xlnid_ptp_get_ts_config(struct xlnid_adapter *adapter, struct ifreq *ifr)
{
	struct hwtstamp_config *config = &adapter->tstamp_config;

	return copy_to_user(ifr->ifr_data, config, sizeof(*config)) ? -EFAULT : 0;
}

int xlnid_ptp_ptsw_mode(struct xlnid_adapter *adapter, struct xlnid_hw *hw,
						struct hwtstamp_config *config, bool one_step)
{
	u32 tsync_tx_ctl = XLNID_TSYNCTXCTL_ENABLED;
	u32 tsync_rx_ctl = XLNID_TSYNCRXCTL_ENABLED;
	u32 tsync_rx_mtrl = PTP_EV_PORT << 16;
	u32 regval = 0;
	int i = 0;
	u32 rdy = 0;

	switch (config->rx_filter) {
	case HWTSTAMP_FILTER_NONE:
		tsync_rx_ctl = 0;
		tsync_rx_mtrl = 0;
		adapter->flags &= ~(XLNID_FLAG_RX_HWTSTAMP_ENABLED |
		XLNID_FLAG_RX_HWTSTAMP_IN_REGISTER);
		break;

	case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
	case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
	case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
		return -EINVAL;

	case HWTSTAMP_FILTER_PTP_V2_L2_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ:
	case HWTSTAMP_FILTER_PTP_V2_L2_SYNC:
		config->rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
		adapter->flags |= (XLNID_FLAG_RX_HWTSTAMP_ENABLED |
		XLNID_FLAG_RX_HWTSTAMP_IN_REGISTER);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_ETH_EN,
		(XLNID_PTP_RX_L2_EN | XLNID_PTP_TX_L2_EN));
		break;

	case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
		config->rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_EVENT;
		adapter->flags |= (XLNID_FLAG_RX_HWTSTAMP_ENABLED |
		XLNID_FLAG_RX_HWTSTAMP_IN_REGISTER);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_ETH_EN,
		(XLNID_PTP_RX_IPV4_EN | XLNID_PTP_RX_IPV6_EN | XLNID_PTP_TX_IPV4_EN |
		XLNID_PTP_TX_IPV6_EN));
		break;

	case HWTSTAMP_FILTER_PTP_V2_EVENT:
	case HWTSTAMP_FILTER_PTP_V2_SYNC:
	case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
	case HWTSTAMP_FILTER_ALL:
		config->rx_filter = HWTSTAMP_FILTER_ALL;
		adapter->flags |= (XLNID_FLAG_RX_HWTSTAMP_ENABLED |
		XLNID_FLAG_RX_HWTSTAMP_IN_REGISTER);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_ETH_EN,
		(XLNID_PTP_RX_IPV4_EN | XLNID_PTP_RX_IPV6_EN | XLNID_PTP_TX_IPV4_EN |
		XLNID_PTP_TX_IPV6_EN | XLNID_PTP_RX_L2_EN | XLNID_PTP_TX_L2_EN));
		break;

	default:
		/*  register RXMTRL must be set in order to do V1 packets,
		therefore it is not possible to time stamp both V1 Sync and
		Delay_Req messages unless hardware supports timestamping all
		packets => return error
		*/
		tsync_rx_ctl = 0;
		adapter->flags &= ~(XLNID_FLAG_RX_HWTSTAMP_ENABLED |
		XLNID_FLAG_RX_HWTSTAMP_IN_REGISTER);
		config->rx_filter = HWTSTAMP_FILTER_NONE;
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_ETH_EN, 0);
		return -ERANGE;
	}

	for (i = 0; i < 32; i++) {
		rdy = XLNID_READ_REG_MAC(hw, WESTLAKE_SNAP_TS_RDY);

		if (!rdy) {
			break;
		}

		if ((rdy & XLNID_TX_SNAP_TS_RDY) == XLNID_TX_SNAP_TS_RDY) {
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_TX_SNAPTS_RDEN, 0x1);
		}

		if ((rdy & XLNID_RX_SNAP_TS_RDY) == XLNID_RX_SNAP_TS_RDY) {
			XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_RX_SNAPTS_RDEN, 0x1);
		}
	}

	/* enable/disable TX\RX */
	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_CFG_TS_UPT_EN);
	regval &= ~(XLNID_CFG_TX_TS_UPLOAD_EN | XLNID_CFG_RX_TS_UPLOAD_EN);
	regval |= (tsync_tx_ctl | tsync_rx_ctl);

	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_TS_UPLOAD_EN, regval);
	//XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_CHKSUM_EN, regval);

	if (regval != 0) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_P1588_BYPASS, 0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_CF_EN, 0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_TS_UPT_EN, 0);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_RXDCTL_VME, 0);
	}

	else {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_P1588_BYPASS, 1);
	}

	if (regval & XLNID_TSYNCTXCTL_ENABLED) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_TX_MSGTYPE_CHK_EN, 0xf00);
	}

	if (regval & XLNID_TSYNCRXCTL_ENABLED) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_RX_MSGTYPE_CHK_EN, 0x00f);
	}

	if (one_step) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_TX_MSGTYPE_CHK_EN, 0xf00);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_CHKSUM_EN, 0x1);
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_TS_UPT_EN, 0x1);
	}

	XLNID_WRITE_FLUSH(hw);
	return 0;
}

/**
		xlnid_ptp_set_timestamp_mode - setup the hardware for the requested mode
		@adapter: the private xlnid adapter structure
		@config: the hwtstamp configuration requested

		Outgoing time stamping can be enabled and disabled. Play nice and
		disable it when requested, although it shouldn't cause any overhead
		when no packet needs it. At most one packet in the queue may be
		marked for time stamping, otherwise it would be impossible to tell
		for sure to which packet the hardware time stamp belongs.

		Incoming time stamping has to be configured via the hardware
		filters. Not all combinations are supported, in particular event
		type has to be specified. Matching the kind of event packet is
		not supported, with the exception of "all V2 events regardless of
		level 2 or 4".

		Since hardware always timestamps Path delay packets when timestamping V2
		packets, regardless of the type specified in the register, only use V2
		Event mode. This more accurately tells the user what the hardware is going
		to do anyways.

		Note: this may modify the hwtstamp configuration towards a more general
		mode, if required to support the specifically requested mode.
*/
static int xlnid_ptp_set_timestamp_mode(struct xlnid_adapter *adapter,
										struct hwtstamp_config *config)
{
	struct xlnid_hw *hw = &adapter->hw;
	u32 tsync_tx_ctl = XLNID_TSYNCTXCTL_ENABLED;
	bool one_step = false;
	xel_pci_info_t *slave_pinfo = NULL;

	/* reserved for future extensions */
	if (config->flags) {
		return -EINVAL;
	}

	DEBUG_PTP("%s:%d: tx_type = 0x%x\r\n", __func__, __LINE__, config->tx_type);

	switch (config->tx_type) {
	case HWTSTAMP_TX_OFF:
		tsync_tx_ctl = 0;
		break;

	case HWTSTAMP_TX_ON:
		break;

	case HWTSTAMP_TX_ONESTEP_SYNC:
		//tsync_tx_ctl = 0;
		one_step = true;
		break;

		#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0))

	case HWTSTAMP_TX_ONESTEP_P2P:
		one_step = true;
		break;
		#endif

	default:
		return -ERANGE;
	}

	xlnid_ptp_ptsw_mode(adapter, hw, config, one_step);

	if (hw->ptsw_enable && hw->bus.func == 0) {
		slave_pinfo = xlnid_find_pci_info(hw->bus.bus_id, 1);
		xlnid_ptp_ptsw_mode(adapter, slave_pinfo->hw, config, one_step);
	}

	/* clear TX/RX timestamp state, just to be sure */
	xlnid_ptp_clear_tx_timestamp(adapter);
	return 0;
}

/**
		xlnid_ptp_set_ts_config - user entry point for timestamp mode
		@adapter: pointer to adapter struct
		@ifr: ioctl data

		Set hardware to requested mode. If unsupported, return an error with no
		changes. Otherwise, store the mode for future reference.
*/
int xlnid_ptp_set_ts_config(struct xlnid_adapter *adapter, struct ifreq *ifr)
{
	struct hwtstamp_config config;
	int err;

	if (copy_from_user(&config, ifr->ifr_data, sizeof(config))) {
		return -EFAULT;
	}

	err = xlnid_ptp_set_timestamp_mode(adapter, &config);

	if (err) {
		return err;
	}

	/* save these settings for future reference */
	memcpy(&adapter->tstamp_config, &config, sizeof(adapter->tstamp_config));

	return copy_to_user(ifr->ifr_data, &config, sizeof(config)) ? -EFAULT : 0;
}

/**
		xlnid_ptp_reset
		@adapter: the xlnid private board structure

		When the MAC resets, all of the hardware configuration for timesync is
		reset. This function should be called to re-enable the device for PTP,
		using the last known settings. However, we do lose the current clock time,
		so we fallback to resetting it based on the kernel's realtime clock.

		This function will maintain the hwtstamp_config settings, and it retriggers
		the SDP output if it's enabled.
*/
void xlnid_ptp_reset(struct xlnid_adapter *adapter)
{
	/* reset the hardware timestamping mode */
	xlnid_ptp_set_timestamp_mode(adapter, &adapter->tstamp_config);

	adapter->last_overflow_check = jiffies;
}

/**
		xlnid_ptp_create_clock
		@adapter: the xlnid private adapter structure

		This functino performs setup of the user entry point function table and
		initalizes the PTP clock device used by userspace to access the clock-like
		features of the PTP core. It will be called by xlnid_ptp_init, and may
		re-use a previously initialized clock (such as during a suspend/resume
		cycle).
*/
static long xlnid_ptp_create_clock(struct xlnid_adapter *adapter)
{
	struct net_device *netdev = adapter->netdev;
	long err;
	int ptp_index;

	/* do nothing if we already have a clock device */
	if (!IS_ERR_OR_NULL(adapter->ptp_clock)) {
		return 0;
	}

	snprintf(adapter->ptp_caps.name, sizeof(adapter->ptp_caps.name), "%s",
				netdev->name);
	adapter->ptp_caps.owner = THIS_MODULE;
	adapter->ptp_caps.max_adj = 250000000;
	adapter->ptp_caps.n_alarm = 0;
	adapter->ptp_caps.n_ext_ts = 0;
	adapter->ptp_caps.n_per_out = 0;
	adapter->ptp_caps.pps = 0;
#ifdef HAVE_PTP_CLOCK_INFO_ADJFINE
	adapter->ptp_caps.adjfine = xlnid_ptp_adjfine_westlake;
#else
	adapter->ptp_caps.adjfreq = xlnid_ptp_adjfreq_westlake;
#endif
	adapter->ptp_caps.adjtime = xlnid_ptp_adjtime_timecounter;
#ifdef HAVE_PTP_CLOCK_INFO_GETTIME64
	adapter->ptp_caps.gettime64 = xlnid_ptp_gettime64_timecounter;
	adapter->ptp_caps.settime64 = xlnid_ptp_settime64_timecounter;
#else
	adapter->ptp_caps.gettime = xlnid_ptp_gettime_timecounter;
	adapter->ptp_caps.settime = xlnid_ptp_settime_timecounter;
#endif
	adapter->ptp_caps.enable = xlnid_ptp_feature_enable;

	adapter->ptp_clock = ptp_clock_register(&adapter->ptp_caps,
											pci_dev_to_dev(adapter->pdev));

	if (IS_ERR(adapter->ptp_clock)) {
		err = PTR_ERR(adapter->ptp_clock);
		adapter->ptp_clock = NULL;
		e_dev_err("ptp_clock_register failed\n");
		return err;
	}

	else if (adapter->ptp_clock) {
		ptp_index = ptp_clock_index(adapter->ptp_clock);
		e_dev_info("registered PHC device on %s\n", netdev->name);
		e_dev_info("PTP%d is working on %s\n", ptp_index, netdev->name);
	}

	/*  Set the default timestamp mode to disabled here. We do this in
		create_clock instead of initialization, because we don't want to
		override the previous settings during a suspend/resume cycle.
	*/
	adapter->tstamp_config.rx_filter = HWTSTAMP_FILTER_NONE;
	adapter->tstamp_config.tx_type = HWTSTAMP_TX_OFF;

	return 0;
}

/**
		xlnid_ptp_init
		@adapter: the xlnid private adapter structure

		This function performs the required steps for enabling ptp
		support. If ptp support has already been loaded it simply calls the
		cyclecounter init routine and exits.
*/
void xlnid_ptp_init(struct xlnid_adapter *adapter)
{
	struct xlnid_hw *hw = &adapter->hw;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))
	struct timespec64 now;
#else
	struct timespec now;
#endif
	u32 regval = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 15, 0))
	ptp_classify_init();
#endif

	/*  initialize the spin lock first, since the user might call the clock
		functions any time after we've initialized the ptp clock device.
	*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 20, 0))
	ktime_get_coarse_real_ts64(&now);
#else
	now = current_kernel_time();
#endif

	XLNID_READ_REG_MAC(hw, WESTLAKE_CRG_STAT_PLL_FUSE_READY);
	regval = XLNID_READ_REG_MAC(hw, WESTLAKE_CRG_STAT_PLL_FUSE_READY);

	if (regval & XLNID_SYS_PLL_LOCKED) {
		XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_ETH_PTP_CLK_SEL,
							XLNID_CFG_ETH_PTP_CLK_SEL);
	}

	/* init RTC */
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDR_TS0, now.tv_nsec);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDR_TS1, now.tv_sec);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_INIT_VID, 1);
	XLNID_WRITE_REG_MAC(hw, WESTLAKE_CFG_UPDT_VID, 1);

	spin_lock_init(&adapter->tmreg_lock);

	/* obtain a ptp clock device, or re-use an existing device */
	if (xlnid_ptp_create_clock(adapter)) {
		return;
	}

	/* we have a clock, so we can intialize work for timestamps now */
	INIT_WORK(&adapter->ptp_tx_work, xlnid_ptp_tx_hwtstamp_work);

	/* reset the ptp related hardware bits */
	xlnid_ptp_reset(adapter);

	/* enter the XLNID_PTP_RUNNING state */
	set_bit(__XLNID_PTP_RUNNING, &adapter->state);

	return;
}

/**
		xlnid_ptp_suspend - stop ptp work items
		@adapter: pointer to adapter struct

		This function suspends ptp activity, and prevents more work from being
		generated, but does not destroy the clock device.
*/
void xlnid_ptp_suspend(struct xlnid_adapter *adapter)
{
	/* leave the XLNID_PTP_RUNNING STATE */
	if (!test_and_clear_bit(__XLNID_PTP_RUNNING, &adapter->state)) {
		return;
	}

	adapter->flags2 &= ~XLNID_FLAG2_PTP_PPS_ENABLED;

	cancel_work_sync(&adapter->ptp_tx_work);
	xlnid_ptp_clear_tx_timestamp(adapter);
}

/**
		xlnid_ptp_stop - destroy the ptp_clock device
		@adapter: pointer to adapter struct

		Completely destroy the ptp_clock device, and disable all PTP related
		features. Intended to be run when the device is being closed.
*/
void xlnid_ptp_stop(struct xlnid_adapter *adapter)
{
	int ptp_index;

	/* first, suspend ptp activity */
	xlnid_ptp_suspend(adapter);

	/* now destroy the ptp clock device */
	if (adapter->ptp_clock) {
		ptp_index = ptp_clock_index(adapter->ptp_clock);
		ptp_clock_unregister(adapter->ptp_clock);
		adapter->ptp_clock = NULL;
		e_dev_info("removed PHC on %s\n", adapter->netdev->name);
		e_dev_info("PTP%d removed on %s\n", ptp_index, adapter->netdev->name);
	}
}

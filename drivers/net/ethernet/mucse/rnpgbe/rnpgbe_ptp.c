// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2024 Mucse Corporation. */

#include <linux/netdevice.h>
#include <linux/ptp_classify.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include "rnpgbe.h"
#include "rnpgbe_regs.h"
#include "rnpgbe_ptp.h"
#include "rnpgbe_mbx.h"

/* #define DEBUG_PTP_TX_TIMESTAMP */
/* #define DEBUG_PTP_RX_TIMESTAMP */

/* PTP and HW Timer ops */
static void config_hw_tstamping(void __iomem *ioaddr, u32 data)
{
	writel(data, ioaddr + PTP_TCR);
}

static void config_sub_second_increment(void __iomem *ioaddr, u32 ptp_clock,
					int gmac4, u32 *ssinc)
{
	u32 value = readl(ioaddr + PTP_TCR);
	unsigned long data;
	u32 reg_value;

	/* For GMAC3.x, 4.x versions, in "fine adjustement mode" set sub-second
	 * increment to twice the number of nanoseconds of a clock cycle.
	 * The calculation of the default_addend value by the caller will set it
	 * to mid-range = 2^31 when the remainder of this division is zero,
	 * which will make the accumulator overflow once every 2 ptp_clock
	 * cycles, adding twice the number of nanoseconds of a clock cycle :
	 * 2000000000ULL / ptp_clock.
	 */
	if (value & RNP_PTP_TCR_TSCFUPDT)
		data = (2000000000ULL / ptp_clock);
	else
		data = (1000000000ULL / ptp_clock);

	/* 0.465ns accuracy */
	if (!(value & RNP_PTP_TCR_TSCTRLSSR))
		data = (data * 1000) / 465;

	data &= RNP_PTP_SSIR_SSINC_MASK;

	reg_value = data;
	if (gmac4)
		reg_value <<= RNP_PTP_SSIR_SSINC_SHIFT;

	writel(reg_value, ioaddr + PTP_SSIR);

	if (ssinc)
		*ssinc = data;
}

static int config_addend(void __iomem *ioaddr, u32 addend)
{
	u32 value;
	int limit;

	writel(addend, ioaddr + PTP_TAR);
	/* issue command to update the addend value */
	value = readl(ioaddr + PTP_TCR);
	value |= RNP_PTP_TCR_TSADDREG;
	writel(value, ioaddr + PTP_TCR);

	/* wait for present addend update to complete */
	limit = 10;
	while (limit--) {
		if (!(readl(ioaddr + PTP_TCR) & RNP_PTP_TCR_TSADDREG))
			break;
		mdelay(10);
	}
	if (limit < 0)
		return -EBUSY;

	return 0;
}

static int init_systime(void __iomem *ioaddr, u32 sec, u32 nsec)
{
	int limit;
	u32 value;

	writel(sec, ioaddr + PTP_STSUR);
	writel(nsec, ioaddr + PTP_STNSUR);
	/* issue command to initialize the system time value */
	value = readl(ioaddr + PTP_TCR);
	value |= RNP_PTP_TCR_TSINIT;
	writel(value, ioaddr + PTP_TCR);

	/* wait for present system time initialize to complete */
	limit = 10;
	while (limit--) {
		if (!(readl(ioaddr + PTP_TCR) & RNP_PTP_TCR_TSINIT))
			break;
		mdelay(10);
	}
	if (limit < 0)
		return -EBUSY;
#ifdef FW_UART_SHOW_TSTAMPS
	/* setup pps control */
	writel(0x1, ioaddr + PTP_PPS_CONTROL);
#endif
	return 0;
}

static void get_systime(void __iomem *ioaddr, u64 *systime)
{
	u64 ns;

	/* Get the TSSS value */
	ns = readl(ioaddr + PTP_STNSR);
	/* Get the TSS and convert sec time value to nanosecond */
	ns += readl(ioaddr + PTP_STSR) * 1000000000ULL;

	if (systime)
		*systime = ns;
}

static void config_mac_interrupt_enable(void __iomem *ioaddr, bool on)
{
	rnpgbe_wr_reg(ioaddr + RNP_MAC_INTERRUPT_ENABLE, on);
}

static int adjust_systime(void __iomem *ioaddr, u32 sec, u32 nsec, int add_sub,
			  int gmac4)
{
	u32 value;
	int limit;

	if (add_sub) {
		/* If the new sec value needs to be subtracted with
		 * the system time, then MAC_STSUR reg should be
		 * programmed with (2^32 – <new_sec_value>)
		 */
		if (gmac4)
			sec = -sec;

		value = readl(ioaddr + PTP_TCR);
		if (value & RNP_PTP_TCR_TSCTRLSSR)
			nsec = (RNP_PTP_DIGITAL_ROLLOVER_MODE - nsec);
		else
			nsec = (RNP_PTP_BINARY_ROLLOVER_MODE - nsec);
	}

	writel(sec, ioaddr + PTP_STSUR);
	value = (add_sub << RNP_PTP_STNSUR_ADDSUB_SHIFT) | nsec;
	writel(value, ioaddr + PTP_STNSUR);

	/* issue command to initialize the system time value */
	value = readl(ioaddr + PTP_TCR);
	value |= RNP_PTP_TCR_TSUPDT;
	writel(value, ioaddr + PTP_TCR);

	/* wait for present system time adjust/update to complete */
	limit = 10;
	while (limit--) {
		if (!(readl(ioaddr + PTP_TCR) & RNP_PTP_TCR_TSUPDT))
			break;
		mdelay(10);
	}
	if (limit < 0)
		return -EBUSY;

	return 0;
}

const struct rnpgbe_hwtimestamp mac_ptp = {
	.config_hw_tstamping = config_hw_tstamping,
	.config_mac_irq_enable = config_mac_interrupt_enable,
	.init_systime = init_systime,
	.config_sub_second_increment = config_sub_second_increment,
	.config_addend = config_addend,
	.adjust_systime = adjust_systime,
	.get_systime = get_systime,
};

static int rnpgbe_ptp_adjfreq(struct ptp_clock_info *ptp, long scaled_ppm)
{
	struct rnpgbe_adapter *pf =
		container_of(ptp, struct rnpgbe_adapter, ptp_clock_ops);
	unsigned long flags;
	u32 addend;

	if (pf == NULL) {
		printk(KERN_DEBUG "adapter_of contail is null\n");
		return 0;
	}
	addend = adjust_by_scaled_ppm(pf->default_addend, scaled_ppm);
	spin_lock_irqsave(&pf->ptp_lock, flags);
	pf->hwts_ops->config_addend(pf->ptp_addr, addend);
	spin_unlock_irqrestore(&pf->ptp_lock, flags);

	return 0;
}

static int rnpgbe_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	struct rnpgbe_adapter *pf =
		container_of(ptp, struct rnpgbe_adapter, ptp_clock_ops);
	unsigned long flags;
	u32 sec, nsec;
	u32 quotient, reminder;
	int neg_adj = 0;

	if (delta < 0) {
		neg_adj = 1;
		delta = -delta;
	}

	if (delta == 0)
		return 0;

	quotient = div_u64_rem(delta, 1000000000ULL, &reminder);
	sec = quotient;
	nsec = reminder;

	spin_lock_irqsave(&pf->ptp_lock, flags);
	pf->hwts_ops->adjust_systime(pf->ptp_addr, sec, nsec, neg_adj,
				     pf->gmac4);
	spin_unlock_irqrestore(&pf->ptp_lock, flags);

	return 0;
}

static int rnpgbe_ptp_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
	struct rnpgbe_adapter *pf =
		container_of(ptp, struct rnpgbe_adapter, ptp_clock_ops);
	unsigned long flags;
	u64 ns = 0;

	spin_lock_irqsave(&pf->ptp_lock, flags);

	pf->hwts_ops->get_systime(pf->ptp_addr, &ns);

	spin_unlock_irqrestore(&pf->ptp_lock, flags);

	*ts = ns_to_timespec64(ns);

	return 0;
}

static int rnpgbe_ptp_settime(struct ptp_clock_info *ptp,
			      const struct timespec64 *ts)
{
	struct rnpgbe_adapter *pf =
		container_of(ptp, struct rnpgbe_adapter, ptp_clock_ops);
	unsigned long flags;

	spin_lock_irqsave(&pf->ptp_lock, flags);
	pf->hwts_ops->init_systime(pf->ptp_addr, ts->tv_sec, ts->tv_nsec);
	spin_unlock_irqrestore(&pf->ptp_lock, flags);

	return 0;
}

static int rnpgbe_ptp_feature_enable(struct ptp_clock_info *ptp,
				     struct ptp_clock_request *rq, int on)
{
	return -EOPNOTSUPP;
}

int rnpgbe_ptp_get_ts_config(struct rnpgbe_adapter *pf, struct ifreq *ifr)
{
	struct hwtstamp_config *config = &pf->tstamp_config;

	return copy_to_user(ifr->ifr_data, config, sizeof(*config)) ? -EFAULT :
								      0;
}

int rnpgbe_ptp_setup_ptp(struct rnpgbe_adapter *pf, u32 value)
{
	u32 sec_inc = 0;
	u64 temp = 0;
	struct timespec64 now;

	/*For now just use extrnal clock(the kernel-system clock)*/
	/* 1.Mask the Timestamp Trigger interrupt */
	/* 2.enable time stamping */
	/* 2.1 clear all bytes about time ctrl reg*/
	pf->hwts_ops->config_hw_tstamping(pf->ptp_addr, value);
	/* 3.Program the PTPclock frequency */
	/* program Sub Second Increment reg
	 * we use kernel-system clock
	 */
	pf->hwts_ops->config_sub_second_increment(
		pf->ptp_addr, pf->clk_ptp_rate, pf->gmac4, &sec_inc);
	/* 4.If use fine correction approash then,
	 * Program MAC_Timestamp_Addend register
	 */
	if (sec_inc == 0) {
		printk(KERN_DEBUG "%s:%d the sec_inc is zero this is a bug\n",
		       __func__, __LINE__);
		return -EFAULT;
	}
	temp = div_u64(1000000000ULL, sec_inc);
	/* Store sub second increment and flags for later use */
	pf->sub_second_inc = sec_inc;
	pf->systime_flags = value;
	/* calculate default added value:
	 * formula is :
	 * addend = (2^32)/freq_div_ratio;
	 * where, freq_div_ratio = 1e9ns/sec_inc
	 */
	temp = (u64)(temp << 32);

	if (pf->clk_ptp_rate == 0) {
		pf->clk_ptp_rate = 1000;
		printk(KERN_DEBUG "%s:%d clk_ptp_rate is zero\n", __func__,
		       __LINE__);
	}

	pf->default_addend = div_u64(temp, pf->clk_ptp_rate);

	pf->hwts_ops->config_addend(pf->ptp_addr, pf->default_addend);
	/* 5.Poll wait for the TCR Update Addend Register*/
	/* 6.enabled Fine Update method */
	/* 7.program the second and nanosecond register*/
	/*TODO If we need to enable one-step timestamp */

	/* initialize system time */
	ktime_get_real_ts64(&now);

	/* lower 32 bits of tv_sec are safe until y2106 */
	pf->hwts_ops->init_systime(pf->ptp_addr, (u32)now.tv_sec, now.tv_nsec);

	return 0;
}

int rnpgbe_ptp_set_ts_config(struct rnpgbe_adapter *pf, struct ifreq *ifr)
{
	struct hwtstamp_config config;
	u32 ptp_v2 = 0;
	u32 tstamp_all = 0;
	u32 ptp_over_ipv4_udp = 0;
	u32 ptp_over_ipv6_udp = 0;
	u32 ptp_over_ethernet = 0;
	u32 snap_type_sel = 0;
	u32 ts_master_en = 0;
	u32 ts_event_en = 0;
	u32 value = 0;
	s32 ret = -1;

	if (!(pf->flags2 & RNP_FLAG2_PTP_ENABLED)) {
		pci_alert(pf->pdev, "No support for HW time stamping\n");
		pf->ptp_tx_en = 0;
		pf->ptp_tx_en = 0;

		return -EOPNOTSUPP;
	}

	if (copy_from_user(&config, ifr->ifr_data, sizeof(config)))
		return -EFAULT;

	netdev_info(pf->netdev,
		    "%s config flags:0x%x, tx_type:0x%x, rx_filter:0x%x\n",
		    __func__, config.flags, config.tx_type, config.rx_filter);
	/* reserved for future extensions */
	if (config.flags)
		return -EINVAL;

	if (config.tx_type != HWTSTAMP_TX_OFF &&
	    config.tx_type != HWTSTAMP_TX_ON)
		return -ERANGE;

	switch (config.rx_filter) {
	case HWTSTAMP_FILTER_NONE:
		/* time stamp no incoming packet at all */
		config.rx_filter = HWTSTAMP_FILTER_NONE;
		break;

	case HWTSTAMP_FILTER_PTP_V1_L4_EVENT:
		/* PTP v1, UDP, any kind of event packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V1_L4_EVENT;
		/* 'mac' hardware can support Sync, Pdelay_Req and
		 * Pdelay_resp by setting bit14 and bits17/16 to 01
		 * This leaves Delay_Req timestamps out.
		 * Enable all events *and* general purpose message
		 * timestamping
		 */
		snap_type_sel = RNP_PTP_TCR_SNAPTYPSEL_1;
		ptp_over_ipv4_udp = RNP_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNP_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
		/* PTP v1, UDP, Sync packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V1_L4_SYNC;
		/* take time stamp for SYNC messages only */
		ts_event_en = RNP_PTP_TCR_TSEVNTENA;

		ptp_over_ipv4_udp = RNP_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNP_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
		/* PTP v1, UDP, Delay_req packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ;
		/* take time stamp for Delay_Req messages only */
		ts_master_en = RNP_PTP_TCR_TSMSTRENA;
		ts_event_en = RNP_PTP_TCR_TSEVNTENA;

		ptp_over_ipv4_udp = RNP_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNP_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
		/* PTP v2, UDP, any kind of event packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_EVENT;
		ptp_v2 = RNP_PTP_TCR_TSVER2ENA;

		/* take time stamp for all event messages */
		snap_type_sel = RNP_PTP_TCR_SNAPTYPSEL_1;

		ptp_over_ipv4_udp = RNP_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNP_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
		/* PTP v2, UDP, Sync packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_SYNC;
		ptp_v2 = RNP_PTP_TCR_TSVER2ENA;
		/* take time stamp for SYNC messages only */
		ts_event_en = RNP_PTP_TCR_TSEVNTENA;
		ptp_over_ipv4_udp = RNP_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNP_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
		/* PTP v2, UDP, Delay_req packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ;
		ptp_v2 = RNP_PTP_TCR_TSVER2ENA;
		/* take time stamp for Delay_Req messages only */
		ts_master_en = RNP_PTP_TCR_TSMSTRENA;
		ts_event_en = RNP_PTP_TCR_TSEVNTENA;
		ptp_over_ipv4_udp = RNP_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNP_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_EVENT:
		/* PTP v2/802.AS1 any layer, any kind of event packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
		ptp_v2 = RNP_PTP_TCR_TSVER2ENA;
		snap_type_sel = RNP_PTP_TCR_SNAPTYPSEL_1;
		ts_event_en = RNP_PTP_TCR_TSEVNTENA;
		ptp_over_ipv4_udp = RNP_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNP_PTP_TCR_TSIPV6ENA;
		ptp_over_ethernet = RNP_PTP_TCR_TSIPENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_SYNC:
		/* PTP v2/802.AS1, any layer, Sync packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_SYNC;
		ptp_v2 = RNP_PTP_TCR_TSVER2ENA;
		/* take time stamp for SYNC messages only */
		ts_event_en = RNP_PTP_TCR_TSEVNTENA;
		ptp_over_ipv4_udp = RNP_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNP_PTP_TCR_TSIPV6ENA;
		ptp_over_ethernet = RNP_PTP_TCR_TSIPENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
		/* PTP v2/802.AS1, any layer, Delay_req packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_DELAY_REQ;
		ptp_v2 = RNP_PTP_TCR_TSVER2ENA;
		/* take time stamp for Delay_Req messages only */
		ts_master_en = RNP_PTP_TCR_TSMSTRENA;
		ts_event_en = RNP_PTP_TCR_TSEVNTENA;

		ptp_over_ipv4_udp = RNP_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNP_PTP_TCR_TSIPV6ENA;
		ptp_over_ethernet = RNP_PTP_TCR_TSIPENA;
		break;

	case HWTSTAMP_FILTER_NTP_ALL:
	case HWTSTAMP_FILTER_ALL:
		/* time stamp any incoming packet */
		config.rx_filter = HWTSTAMP_FILTER_ALL;
		tstamp_all = RNP_PTP_TCR_TSENALL;
		break;

	default:
		return -ERANGE;
	}

	pf->ptp_rx_en = ((config.rx_filter == HWTSTAMP_FILTER_NONE) ? 0 : 1);
	pf->ptp_tx_en = config.tx_type == HWTSTAMP_TX_ON;

	netdev_info(
		pf->netdev,
		"ptp config rx filter 0x%.2x tx_type 0x%.2x rx_en[%d] tx_en[%d]\n",
		config.rx_filter, config.tx_type, pf->ptp_rx_en, pf->ptp_tx_en);
	if (!pf->ptp_rx_en && !pf->ptp_tx_en)
		/*rx and tx is not use hardware ts so clear the ptp register */
		pf->hwts_ops->config_hw_tstamping(pf->ptp_addr, 0);
	else {
		value = (RNP_PTP_TCR_TSENA | RNP_PTP_TCR_TSCFUPDT |
			 RNP_PTP_TCR_TSCTRLSSR | tstamp_all | ptp_v2 |
			 ptp_over_ethernet | ptp_over_ipv6_udp |
			 ptp_over_ipv4_udp | ts_master_en | snap_type_sel);

		ret = rnpgbe_ptp_setup_ptp(pf, value);
		if (ret < 0)
			return ret;
	}
	pf->ptp_config_value = value;
	memcpy(&pf->tstamp_config, &config, sizeof(config));

	return copy_to_user(ifr->ifr_data, &config, sizeof(config)) ? -EFAULT :
								      0;
}

/* structure describing a PTP hardware clock */
static struct ptp_clock_info rnpgbe_ptp_clock_ops = {
	.owner = THIS_MODULE,
	.name = "rnp ptp",
	.max_adj = 50000000,
	.n_alarm = 0,
	.n_ext_ts = 0,
	.n_per_out = 0, /* will be overwritten in stmmac_ptp_register */
	.n_pins = 0, /*should be 0 if not set*/
	.adjfine = rnpgbe_ptp_adjfreq,
	.adjtime = rnpgbe_ptp_adjtime,
	.gettime64 = rnpgbe_ptp_gettime,
	.settime64 = rnpgbe_ptp_settime,
	.enable = rnpgbe_ptp_feature_enable,
};

int rnpgbe_ptp_register(struct rnpgbe_adapter *pf)
{
	pf->hwts_ops = &mac_ptp;

	pf->ptp_tx_en = 0;
	pf->ptp_rx_en = 0;

	spin_lock_init(&pf->ptp_lock);
	pf->flags2 |= RNP_FLAG2_PTP_ENABLED;
	pf->ptp_clock_ops = rnpgbe_ptp_clock_ops;

	/*default mac clock rate is 50Mhz */
	pf->clk_ptp_rate = 50000000;
	if (pf->pdev == NULL)
		printk(KERN_DEBUG "pdev dev is null\n");

	pf->ptp_clock = ptp_clock_register(&pf->ptp_clock_ops, &pf->pdev->dev);
	if (pf->ptp_clock == NULL)
		pci_err(pf->pdev, "ptp clock register failed\n");

	if (IS_ERR(pf->ptp_clock)) {
		pci_err(pf->pdev, "ptp_clock_register failed\n");
		pf->ptp_clock = NULL;
	} else {
		pci_info(pf->pdev, "registered PTP clock\n");
	}

	return 0;
}

void rnpgbe_ptp_unregister(struct rnpgbe_adapter *pf)
{
	/*1. stop the ptp module*/
	if (pf->ptp_clock) {
		ptp_clock_unregister(pf->ptp_clock);
		pf->ptp_clock = NULL;
		pr_debug("Removed PTP HW clock successfully on %s\n",
			 "rnpgbe_ptp");
	}
}

#if defined(DEBUG_PTP_HARD_SOFTWAY_RX) || defined(DEBUG_PTP_HARD_SOFTWAY_TX)
static u64 rnpgbe_get_software_ts(void)
{
	struct timespec64 ts;

	ktime_get_real_ts64(&ts);
	return (ts.tv_nsec + ts.tv_sec * 1000000000ULL);
}
#endif

#if defined(DEBUG_PTP_TX_TIMESTAMP) || defined(DEBUG_PTP_RX_TIMESTAMP)
#define TIME_ZONE_CHINA (8)
char *asctime(const struct tm *timeptr)
{
	static const char wday_name[][4] = { "Sun", "Mon", "Tue", "Wed",
					     "Thu", "Fri", "Sat" };
	static const char mon_name[][4] = { "Jan", "Feb", "Mar", "Apr",
					    "May", "Jun", "Jul", "Aug",
					    "Sep", "Oct", "Nov", "Dec" };
	static char result[26];

	sprintf(result, "%.3s %.3s%3d %.2d:%.2d:%.2d %ld\n",
		wday_name[timeptr->tm_wday], mon_name[timeptr->tm_mon],
		timeptr->tm_mday, timeptr->tm_hour + TIME_ZONE_CHINA,
		timeptr->tm_min, timeptr->tm_sec, 1900 + timeptr->tm_year);
	return result;
}

static void rnpgbe_print_human_timestamp(uint64_t ns, uint8_t *direct)
{
	struct timespec64 ts;
	struct tm tms;
	ktime_t ktm = ns_to_ktime(ns);

	ts = ktime_to_timespec64(ktm);
	time64_to_tm(ts.tv_sec, ts.tv_nsec / 1000000000ULL, &tms);
	printk(KERN_DEBUG "[%s] %s ------\n", direct, asctime(&tms));
}
#endif

void rnpgbe_tx_hwtstamp_work(struct work_struct *work)
{
	struct rnpgbe_adapter *adapter =
		container_of(work, struct rnpgbe_adapter, tx_hwtstamp_work);
#ifdef FW_UART_SHOW_TSTAMPS
	struct rnpgbe_hw *hw = &adapter->hw;
#endif
	void __iomem *ioaddr = adapter->hw.hw_addr;

	/* 1. read port belone timestatmp status reg */
	/* 2. status enabled read nsec and sec reg*/
	/* 3. */
	u64 nanosec = 0, sec = 0;

	if (!adapter->ptp_tx_skb) {
		clear_bit_unlock(__RNP_PTP_TX_IN_PROGRESS, &adapter->state);
		return;
	}

	if (rnpgbe_rd_reg(ioaddr + RNP_ETH_PTP_TX_TSVALUE_STATUS(0)) & 0x01) {
		struct sk_buff *skb = adapter->ptp_tx_skb;
		struct skb_shared_hwtstamps shhwtstamps;
		u64 txstmp = 0;
		/* read  and add nsec, sec turn to nsec*/

		nanosec = rnpgbe_rd_reg(ioaddr + RNP_ETH_PTP_TX_LTIMES(0));
		sec = rnpgbe_rd_reg(ioaddr + RNP_ETH_PTP_TX_HTIMES(0));
		/* when we read the timestamp finish need to notice the hardware
		 * that the timestamp need to update via set tx_hwts_clear-reg
		 * from high to low
		 */
		//printk("tx time %llx-- %llx\n", nanosec, sec);
		rnpgbe_wr_reg(ioaddr + RNP_ETH_PTP_TX_CLEAR(0),
			      PTP_GET_TX_HWTS_FINISH);
		rnpgbe_wr_reg(ioaddr + RNP_ETH_PTP_TX_CLEAR(0),
			      PTP_GET_TX_HWTS_UPDATE);

		txstmp = nanosec & PTP_HWTX_TIME_VALUE_MASK;
		txstmp += (sec & PTP_HWTX_TIME_VALUE_MASK) * 1000000000ULL;

		/* Clear the global tx_hwtstamp_skb pointer and force writes
		 * prior to notifying the stack of a Tx timestamp.
		 */
		memset(&shhwtstamps, 0, sizeof(shhwtstamps));
		shhwtstamps.hwtstamp = ns_to_ktime(txstmp);
		adapter->ptp_tx_skb = NULL;
#ifdef DEBUG_PTP_TX_TIMESTAMP
		rnpgbe_print_human_timestamp(txstmp, "TX");
#endif
		/* force write prior to skb_tstamp_tx
		 * because the xmit will re used the point to store ptp skb
		 */
		wmb();

		skb_tstamp_tx(skb, &shhwtstamps);
		dev_consume_skb_any(skb);
		clear_bit_unlock(__RNP_PTP_TX_IN_PROGRESS, &adapter->state);
		/* send tstamps to hw */
#ifdef FW_UART_SHOW_TSTAMPS
		rnpgbe_mbx_tstamps_show(hw, sec, nanosec);
#endif
	} else if (time_after(jiffies,
			      adapter->tx_hwtstamp_start +
				      adapter->tx_timeout_factor * HZ)) {
		/* this function will mark the skb drop*/
		if (adapter->ptp_tx_skb)
			dev_kfree_skb_any(adapter->ptp_tx_skb);
		adapter->ptp_tx_skb = NULL;
		adapter->tx_hwtstamp_timeouts++;
		clear_bit_unlock(__RNP_PTP_TX_IN_PROGRESS, &adapter->state);
		netdev_warn(adapter->netdev, "clearing Tx timestamp hang\n");
	} else {
		/* reschedule to check later */
#ifdef DEBUG_PTP_HARD_SOFTWAY_TX
		struct skb_shared_hwtstamps shhwtstamp;
		u64 ns = 0;

		ns = rnpgbe_get_software_ts();
		shhwtstamp.hwtstamp = ns_to_ktime(ns);
		if (adapter->ptp_tx_skb) {
			skb_tstamp_tx(adapter->ptp_tx_skb, &shhwtstamp);
			dev_consume_skb_any(adapter->ptp_tx_skb);
			adapter->ptp_tx_skb = NULL;
		}
#else
		schedule_work(&adapter->tx_hwtstamp_work);
#endif
	}
}

void rnpgbe_ptp_get_rx_hwstamp(struct rnpgbe_adapter *adapter,
			       union rnpgbe_rx_desc *desc, struct sk_buff *skb)
{
	u64 ns = 0;
	u64 tsvalueh = 0, tsvaluel = 0;
	struct skb_shared_hwtstamps *hwtstamps = NULL;

	if (!skb || !adapter->ptp_rx_en) {
		netdev_dbg(adapter->netdev,
			   "hwstamp skb is null or "
			   "rx_en iszero %u\n",
			   adapter->ptp_rx_en);
		return;
	}

#ifdef DEBUG_PTP_HARD_SOFTWAY_RX
	ns = rnpgbe_get_software_ts();
#else
	if (likely(!((desc->wb.cmd) & RNP_RXD_STAT_PTP)))
		return;
	hwtstamps = skb_hwtstamps(skb);
	/* because of rx hwstamp store before the mac head
	 * skb->head and skb->data is point to same location when call alloc_skb
	 * so we must move 16 bytes the skb->data to the mac head location
	 * but for the head point if we need move the skb->head need to be diss
	 */
	/* low8bytes is null high8bytes is timestamp
	 * high32bit is seconds low32bits is nanoseconds
	 */
	skb_copy_from_linear_data_offset(skb, RNP_RX_TIME_RESERVE, &tsvalueh,
					 RNP_RX_SEC_SIZE);
	skb_copy_from_linear_data_offset(skb,
					 RNP_RX_TIME_RESERVE + RNP_RX_SEC_SIZE,
					 &tsvaluel, RNP_RX_NANOSEC_SIZE);
	skb_pull(skb, RNP_RX_HWTS_OFFSET);
	tsvalueh = ntohl(tsvalueh);
	tsvaluel = ntohl(tsvaluel);

	ns = tsvaluel & RNP_RX_NSEC_MASK;
	ns += ((tsvalueh & RNP_RX_SEC_MASK) * 1000000000ULL);

	netdev_dbg(adapter->netdev,
		   "ptp get hardware ts-sec %llu ts-nanosec %llu\n", tsvalueh,
		   tsvaluel);
#endif
	hwtstamps->hwtstamp = ns_to_ktime(ns);
#ifdef DEBUG_PTP_RX_TIMESTAMP
	rnpgbe_print_human_timestamp(ns, "RX");
#endif
}

void rnpgbe_ptp_reset(struct rnpgbe_adapter *adapter)
{
	rnpgbe_ptp_setup_ptp(adapter, adapter->ptp_config_value);
}

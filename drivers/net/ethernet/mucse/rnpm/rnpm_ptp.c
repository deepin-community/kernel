// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#include <linux/netdevice.h>
#include <linux/ptp_classify.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/clk.h>

#include "rnpm.h"
#include "rnpm_regs.h"
#include "rnpm_ptp.h"

/* PTP and HW Timer ops */
static void config_hw_tstamping(void __iomem *ioaddr, u8 port, u32 data)
{
	rnpm_wr_reg(ioaddr + RNPM_MAC_TS_CTRL(port), data);
}

static void config_sub_second_increment(void __iomem *ioaddr, u8 port,
					u32 ptp_clock, u32 *ssinc)
{
	u32 value = rnpm_rd_reg(ioaddr + RNPM_MAC_TS_CTRL(port));
	unsigned long data;
	u32 reg_value;

	/* in "fine adjustement mode" set sub-second
	 * increment to twice the number of nanoseconds of a clock cycle.
	 * The calculation of the default_addend value by the caller will set it
	 * to mid-range = 2^31 when the remainder of this division is zero,
	 * which will make the accumulator overflow once every 2 ptp_clock
	 * cycles, adding twice the number of nanoseconds of a clock cycle :
	 * 2000000000ULL / ptp_clock.
	 */
	if (ptp_clock == 0) {
		ptp_dbg("%s:%dthis is a bug that the syskernel clock is zero\n",
			__func__, __LINE__);
		return;
	}
	if (value & RNPM_PTP_TCR_TSCFUPDT)
		data = (2000000000ULL / ptp_clock);
	else
		data = (1000000000ULL / ptp_clock);

	/* 0.465ns accuracy */
	if (!(value & RNPM_PTP_TCR_TSCTRLSSR))
		data = (data * 1000) / 465;

	data &= RNPM_PTP_SSIR_SSINC_MASK;

	reg_value = data;

	reg_value <<= RNPM_PTP_SSIR_SSINC_SHIFT;

	rnpm_wr_reg(ioaddr + RNPM_MAC_SUB_SECOND_INCREMENT(port),
		    reg_value);

	if (ssinc)
		*ssinc = data;
}

static int config_addend(void __iomem *ioaddr, u8 port, u32 addend)
{
	u32 value;
	int limit;

	rnpm_wr_reg(ioaddr + RNPM_MAC_TS_ADDEND(port), addend);
	/* issue command to update the addend value */
	value = rnpm_rd_reg(ioaddr + RNPM_MAC_TS_CTRL(port));
	value |= RNPM_PTP_TCR_TSADDREG;
	rnpm_wr_reg(ioaddr + RNPM_MAC_TS_CTRL(port), value);

	/* wait for present addend update to complete */
	limit = 10;
	while (limit--) {
		if (!(rnpm_rd_reg(ioaddr + RNPM_MAC_TS_CTRL(port)) &
		      RNPM_PTP_TCR_TSADDREG))
			break;
		mdelay(10);
	}
	if (limit < 0)
		return -EBUSY;

	return 0;
}

static int init_systime(void __iomem *ioaddr, u8 port, u32 sec, u32 nsec)
{
	u32 value;
	u32 target;
	int timeout = 0;

	rnpm_wr_reg(ioaddr + RNPM_MAC_SYS_TIME_SEC_UPDATE(port), sec);
	rnpm_wr_reg(ioaddr + RNPM_MAC_SYS_TIME_NANOSEC_UPDATE(port), nsec);
	/* issue command to initialize the system time value */
	value = rnpm_rd_reg(ioaddr + RNPM_MAC_TS_CTRL(port));
	value |= RNPM_PTP_TCR_TSINIT;
	rnpm_wr_reg(ioaddr + RNPM_MAC_TS_CTRL(port), value);

	while (timeout <= 20) {
		target = rnpm_rd_reg(ioaddr + RNPM_MAC_TS_CTRL(port));
		if (!(value & RNPM_PTP_TCR_TSINIT))
			break;
		usleep_range(5000, 10000);
		timeout++;
	}

	if (timeout < 20)
		return 0;
	else
		return -1;
}

static void get_systime(void __iomem *ioaddr, u8 port, u64 *systime)
{
	u64 ns;

	/* Get the TSSS value */
	ns = rnpm_rd_reg(ioaddr + RNPM_MAC_SYS_TIME_NANOSEC_CFG(port));
	/* Get the TSS and convert sec time value to nanosecond */
	ns += rnpm_rd_reg(ioaddr + RNPM_MAC_SYS_TIME_SEC_CFG(port)) *
	      1000000000ULL;

	if (systime)
		*systime = ns;
}

static void config_mac_interrupt_enable(void __iomem *ioaddr, u8 port,
					bool on)
{
	rnpm_wr_reg(ioaddr + RNPM_MAC_INTERRUPT_ENABLE(port), on);
}

struct rnpm_hwtimestamp {
	void (*config_hw_tstamping)(void __iomem *ioaddr, u8 port,
				    u32 data);
	void (*config_sub_second_increment)(void __iomem *ioaddr, u8 port,
					    u32 ptp_clock, u32 *ssinc);
	void (*config_mac_irq_enable)(void __iomem *ioaddr, u8 port,
				      bool on);
	int (*init_systime)(void __iomem *ioaddr, u8 port, u32 sec,
			    u32 nsec);
	int (*config_addend)(void __iomem *ioaddr, u8 port, u32 addend);
	int (*adjust_systime)(void __iomem *ioaddr, u8 port, u32 sec,
			      u32 nsec, int add_sub);
	void (*get_systime)(void __iomem *ioaddr, u8 port, u64 *systime);
};

static int adjust_systime(void __iomem *ioaddr, u8 port, u32 sec, u32 nsec,
			  int add_sub)
{
	u32 value;
	int limit;

	if (add_sub) {
		/* If the new sec value needs to be subtracted with
		 * the system time, then RNPM_MAC_STSUR reg should be
		 * programmed with (2^32 – <new_sec_value>)
		 */
		sec = -sec;
		value = rnpm_rd_reg(ioaddr + RNPM_MAC_TS_CTRL(port));
		if (value & RNPM_PTP_TCR_TSCTRLSSR)
			nsec = (PTP_DIGITAL_ROLLOVER_MODE - nsec);
		else
			nsec = (PTP_BINARY_ROLLOVER_MODE - nsec);
	}

	rnpm_wr_reg(ioaddr + RNPM_MAC_SYS_TIME_SEC_UPDATE(port), sec);
	value = (add_sub << PTP_STNSUR_ADDSUB_SHIFT) | nsec;
	rnpm_wr_reg(ioaddr + RNPM_MAC_SYS_TIME_NANOSEC_UPDATE(port),
		    value);

	/* issue command to initialize the system time value */
	value = rnpm_rd_reg(ioaddr + RNPM_MAC_TS_CTRL(port));
	value |= RNPM_PTP_TCR_TSUPDT;
	rnpm_wr_reg(ioaddr + RNPM_MAC_TS_CTRL(port), value);

	/* wait for present system time adjust/update to complete */
	limit = 10;
	while (limit--) {
		if (!(rnpm_rd_reg(ioaddr + RNPM_MAC_TS_CTRL(port)) &
		      RNPM_PTP_TCR_TSUPDT))
			break;
		mdelay(10);
	}
	if (limit < 0)
		return -EBUSY;
	return 0;
}

const struct rnpm_hwtimestamp mac_ptp = {
	.config_hw_tstamping = config_hw_tstamping,
	.config_mac_irq_enable = config_mac_interrupt_enable,
	.init_systime = init_systime,
	.config_sub_second_increment = config_sub_second_increment,
	.config_addend = config_addend,
	.adjust_systime = adjust_systime,
	.get_systime = get_systime,
};

static int rnpm_ptp_adjfine(struct ptp_clock_info *ptp, long scaled_ppm)
{
	struct rnpm_adapter *pf =
		container_of(ptp, struct rnpm_adapter, ptp_clock_ops);
	unsigned long flags;
	u32 diff, addend;
	int neg_adj = 0;
	u64 adj;
	u8 port = pf->port;

	if (pf == NULL) {
		ptp_dbg("adapter_of contail is null\n");
		return 0;
	}

	if (scaled_ppm < 0) {
		neg_adj = 1;
		scaled_ppm = -scaled_ppm;
	}
	addend = pf->default_addend;
	adj = addend;

	adj *= (u64)scaled_ppm;
	diff = div64_u64(adj, 1000000ULL << 16);
	addend = neg_adj ? (addend - diff) : (addend + diff);
	spin_lock_irqsave(&pf->ptp_lock, flags);
	pf->hwts_ops->config_addend(pf->hw.hw_addr, port, addend);
	spin_unlock_irqrestore(&pf->ptp_lock, flags);
	return 0;
}

static int rnpm_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
	struct rnpm_adapter *pf =
		container_of(ptp, struct rnpm_adapter, ptp_clock_ops);
	unsigned long flags;
	u32 sec, nsec;
	u32 quotient, reminder;
	int neg_adj = 0;
	u8 port = pf->port;

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
	pf->hwts_ops->adjust_systime(pf->hw.hw_addr, port, sec, nsec,
				     neg_adj);
	spin_unlock_irqrestore(&pf->ptp_lock, flags);

	return 0;
}

static int rnpm_ptp_gettime(struct ptp_clock_info *ptp,
			    struct timespec64 *ts)
{
	struct rnpm_adapter *pf =
		container_of(ptp, struct rnpm_adapter, ptp_clock_ops);
	unsigned long flags;
	u64 ns = 0;
	u8 port = pf->port;

	spin_lock_irqsave(&pf->ptp_lock, flags);
	pf->hwts_ops->get_systime(pf->hw.hw_addr, port, &ns);
	spin_unlock_irqrestore(&pf->ptp_lock, flags);
	*ts = ns_to_timespec64(ns);
	return 0;
}

static int rnpm_ptp_settime(struct ptp_clock_info *ptp,
			    const struct timespec64 *ts)
{
	struct rnpm_adapter *pf =
		container_of(ptp, struct rnpm_adapter, ptp_clock_ops);
	unsigned long flags;
	u8 port = pf->port;

	spin_lock_irqsave(&pf->ptp_lock, flags);
	pf->hwts_ops->init_systime(pf->hw.hw_addr, port, ts->tv_sec,
				   ts->tv_nsec);
	spin_unlock_irqrestore(&pf->ptp_lock, flags);

	return 0;
}

static int rnpm_ptp_feature_enable(struct ptp_clock_info *ptp,
				   struct ptp_clock_request *rq, int on)
{
	return -EOPNOTSUPP;
}

int rnpm_ptp_get_ts_config(struct rnpm_adapter *pf, struct ifreq *ifr)
{
	struct hwtstamp_config *config = &pf->tstamp_config;

	return copy_to_user(ifr->ifr_data, config, sizeof(*config)) ?
		       -EFAULT :
		       0;
}

static int rnpm_ptp_setup_ptp(struct rnpm_adapter *pf, u32 value)
{
	u32 sec_inc = 0;
	u64 temp = 0;
	struct timespec64 now;
	u8 port = pf->port;

	/* For now just use extrnal clock(the kernel-system clock)
	 * Mask the Timestamp Trigger interrupt
	 */
	pf->hwts_ops->config_mac_irq_enable(pf->hw.hw_addr, port, false);
	/* enable time stamping
	 * clear all bytes about time ctrl reg
	 */
	pf->hwts_ops->config_hw_tstamping(pf->hw.hw_addr, port, 0);

	pf->hwts_ops->config_hw_tstamping(pf->hw.hw_addr, port, value);
	/* Program the PTPclock frequency
	 * program Sub Second Increment reg
	 * we use kernel-system clock
	 */
	pf->hwts_ops->config_sub_second_increment(
		pf->hw.hw_addr, port, pf->clk_ptp_rate, &sec_inc);
	/* If use fine correction approash then,
	 * Program MAC_Timestamp_Addend register
	 */
	if (sec_inc == 0) {
		ptp_dbg("%s:%d the sec_inc is zero this is a bug\n",
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
		ptp_dbg("%s:%d clk_ptp_rate is zero\n", __func__,
			__LINE__);
	}

	pf->default_addend = div_u64(temp, pf->clk_ptp_rate);

	pf->hwts_ops->config_addend(pf->hw.hw_addr, port,
				    pf->default_addend);
	ktime_get_real_ts64(&now);
	/* lower 32 bits of tv_sec are safe until y2106 */
	pf->hwts_ops->init_systime(pf->hw.hw_addr, port, (u32)now.tv_sec,
				   now.tv_nsec);
	pf->hwts_ops->config_mac_irq_enable(pf->hw.hw_addr, port, true);
	return 0;
}

int rnpm_ptp_set_ts_config(struct rnpm_adapter *pf, struct ifreq *ifr)
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
	u8 port = pf->port;

	if (!(pf->flags2 & RNPM_FLAG2_PTP_ENABLED)) {
		pci_alert(pf->pdev, "No support for HW time stamping\n");
		pf->ptp_tx_en = 0;
		pf->ptp_tx_en = 0;

		return -EOPNOTSUPP;
	}

	if (copy_from_user(&config, ifr->ifr_data, sizeof(config)))
		return -EFAULT;

	netdev_info(pf->netdev,
		    "%s config flags:0x%x, tx_type:0x%x, rx_filter:0x%x\n",
		    __func__, config.flags, config.tx_type,
		    config.rx_filter);
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
		snap_type_sel = RNPM_PTP_TCR_SNAPTYPSEL_1;
		ptp_over_ipv4_udp = RNPM_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNPM_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V1_L4_SYNC:
		/* PTP v1, UDP, Sync packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V1_L4_SYNC;
		/* take time stamp for SYNC messages only */
		ts_event_en = RNPM_PTP_TCR_TSEVNTENA;

		ptp_over_ipv4_udp = RNPM_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNPM_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ:
		/* PTP v1, UDP, Delay_req packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ;
		/* take time stamp for Delay_Req messages only */
		ts_master_en = RNPM_PTP_TCR_TSMSTRENA;
		ts_event_en = RNPM_PTP_TCR_TSEVNTENA;

		ptp_over_ipv4_udp = RNPM_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNPM_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_L4_EVENT:
		/* PTP v2, UDP, any kind of event packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_EVENT;
		ptp_v2 = RNPM_PTP_TCR_TSVER2ENA;

		/* take time stamp for all event messages */
		snap_type_sel = RNPM_PTP_TCR_SNAPTYPSEL_1;

		ptp_over_ipv4_udp = RNPM_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNPM_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_L4_SYNC:
		/* PTP v2, UDP, Sync packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_SYNC;
		ptp_v2 = RNPM_PTP_TCR_TSVER2ENA;
		/* take time stamp for SYNC messages only */
		ts_event_en = RNPM_PTP_TCR_TSEVNTENA;
		ptp_over_ipv4_udp = RNPM_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNPM_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ:
		/* PTP v2, UDP, Delay_req packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ;
		ptp_v2 = RNPM_PTP_TCR_TSVER2ENA;
		/* take time stamp for Delay_Req messages only */
		ts_master_en = RNPM_PTP_TCR_TSMSTRENA;
		ts_event_en = RNPM_PTP_TCR_TSEVNTENA;
		ptp_over_ipv4_udp = RNPM_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNPM_PTP_TCR_TSIPV6ENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_EVENT:
		/* PTP v2/802.AS1 any layer, any kind of event packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_EVENT;
		ptp_v2 = RNPM_PTP_TCR_TSVER2ENA;
		snap_type_sel = RNPM_PTP_TCR_SNAPTYPSEL_1;
		ptp_over_ipv4_udp = RNPM_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNPM_PTP_TCR_TSIPV6ENA;
		ptp_over_ethernet = RNPM_PTP_TCR_TSIPENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_SYNC:
		/* PTP v2/802.AS1, any layer, Sync packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_SYNC;
		ptp_v2 = RNPM_PTP_TCR_TSVER2ENA;
		/* take time stamp for SYNC messages only */
		ts_event_en = RNPM_PTP_TCR_TSEVNTENA;
		ptp_over_ipv4_udp = RNPM_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNPM_PTP_TCR_TSIPV6ENA;
		ptp_over_ethernet = RNPM_PTP_TCR_TSIPENA;
		break;

	case HWTSTAMP_FILTER_PTP_V2_DELAY_REQ:
		/* PTP v2/802.AS1, any layer, Delay_req packet */
		config.rx_filter = HWTSTAMP_FILTER_PTP_V2_DELAY_REQ;
		ptp_v2 = RNPM_PTP_TCR_TSVER2ENA;
		/* take time stamp for Delay_Req messages only */
		ts_master_en = RNPM_PTP_TCR_TSMSTRENA;
		ts_event_en = RNPM_PTP_TCR_TSEVNTENA;

		ptp_over_ipv4_udp = RNPM_PTP_TCR_TSIPV4ENA;
		ptp_over_ipv6_udp = RNPM_PTP_TCR_TSIPV6ENA;
		ptp_over_ethernet = RNPM_PTP_TCR_TSIPENA;
		break;

#ifdef HWTSTAMP_FILTER_NTP_ALL
	case HWTSTAMP_FILTER_NTP_ALL:
#endif
	case HWTSTAMP_FILTER_ALL:
		/* time stamp any incoming packet */
		config.rx_filter = HWTSTAMP_FILTER_ALL;
		tstamp_all = RNPM_PTP_TCR_TSENALL;
		break;

	default:
		return -ERANGE;
	}

	pf->ptp_rx_en =
		((config.rx_filter == HWTSTAMP_FILTER_NONE) ? 0 : 1);
	pf->ptp_tx_en = config.tx_type == HWTSTAMP_TX_ON;

	netdev_info(
		pf->netdev,
		"ptp config rx filter 0x%.2x tx_type 0x%.2x rx_en[%d] tx_en[%d]\n",
		config.rx_filter, config.tx_type, pf->ptp_rx_en,
		pf->ptp_tx_en);
	if (!pf->ptp_rx_en && !pf->ptp_tx_en)
		/*rx and tx is not use hardware ts so clear the ptp register */
		pf->hwts_ops->config_hw_tstamping(pf->hw.hw_addr, port, 0);
	else {
		value = (RNPM_PTP_TCR_TSENA | RNPM_PTP_TCR_TSCFUPDT |
			 RNPM_PTP_TCR_TSCTRLSSR | tstamp_all | ptp_v2 |
			 ptp_over_ethernet | ptp_over_ipv6_udp |
			 ptp_over_ipv4_udp | ts_event_en | ts_master_en |
			 snap_type_sel);
#ifdef RNPM_CMCC_PTP_EXTCLOCK
		if (rnpm_is_pf1(pf->pdev) && (pf->port == 1))
			value |= RNPM_PTP_TCR_ESTI;
#endif
		ret = rnpm_ptp_setup_ptp(pf, value);
		if (ret < 0)
			return ret;
	}
	pf->ptp_config_value = value;
	memcpy(&pf->tstamp_config, &config, sizeof(config));

	return copy_to_user(ifr->ifr_data, &config, sizeof(config)) ?
		       -EFAULT :
		       0;
}

/* structure describing a PTP hardware clock */
static struct ptp_clock_info rnpm_ptp_clock_ops = {
	.owner = THIS_MODULE,
	.name = "rnpm ptp",
	.max_adj = 500000000,
	.n_alarm = 0,
	.n_ext_ts = 0,
	.n_per_out = 0, /* will be overwritten in stmmac_ptp_register */
	.n_pins = 0, /*should be 0 if not set*/
	.adjfine = rnpm_ptp_adjfine,
	.adjtime = rnpm_ptp_adjtime,
	.gettime64 = rnpm_ptp_gettime,
	.settime64 = rnpm_ptp_settime,
	.enable = rnpm_ptp_feature_enable,
};

int rnpm_ptp_register(struct rnpm_adapter *pf)
{
	pf->hwts_ops = &mac_ptp;

	pf->ptp_tx_en = 0;
	pf->ptp_rx_en = 0;

	spin_lock_init(&pf->ptp_lock);
	pf->flags2 |= RNPM_FLAG2_PTP_ENABLED;
	pf->ptp_clock_ops = rnpm_ptp_clock_ops;

	/*default mac clock rate is 50Mhz */
	pf->clk_ptp_rate = 50000000;
	if (pf->pdev == NULL)
		ptp_dbg("pdev dev is null\n");

	pf->ptp_clock =
		ptp_clock_register(&pf->ptp_clock_ops, &pf->pdev->dev);
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

void rnpm_ptp_unregister(struct rnpm_adapter *pf)
{
	/*1. stop the ptp module*/
	if (pf->ptp_clock) {
		ptp_clock_unregister(pf->ptp_clock);
		pf->ptp_clock = NULL;
		pr_debug("Removed PTP HW clock successfully on %s\n",
			 "rnpm_ptp");
	}
}

#if defined(DEBUG_PTP_HARD_SOFTWAY_RX) || \
	defined(DEBUG_PTP_HARD_SOFTWAY_TX)
static u64 rnpm_get_software_ts(void)
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

static void rnpm_print_human_timestamp(uint64_t ns, uint8_t *direct)
{
	struct timespec64 ts;
	struct tm tms;
	ktime_t ktm = ns_to_ktime(ns);

	ts = ktime_to_timespec64(ktm);
	ptp_dbg("[%s] %s ------\n", direct, ns);
}
#endif

void rnpm_tx_hwtstamp_work(struct work_struct *work)
{
	struct rnpm_adapter *adapter =
		container_of(work, struct rnpm_adapter, tx_hwtstamp_work);
	void __iomem *ioaddr = adapter->hw.hw_addr;

	u64 nanosec = 0, sec = 0;
	u8 port = adapter->port;

	if (!adapter->ptp_tx_skb)
		return;

	if (rnpm_rd_reg(ioaddr + RNPM_ETH_PTP_TX_TSVALUE_STATUS(port)) &
	    0x01) {
		struct sk_buff *skb = adapter->ptp_tx_skb;
		struct skb_shared_hwtstamps shhwtstamps;
		u64 txstmp = 0;

		nanosec =
			rnpm_rd_reg(ioaddr + RNPM_ETH_PTP_TX_LTIMES(port));
		sec = rnpm_rd_reg(ioaddr + RNPM_ETH_PTP_TX_HTIMES(port));
		rnpm_wr_reg(ioaddr + RNPM_ETH_PTP_TX_CLEAR(port),
			    PTP_GET_TX_HWTS_FINISH);
		rnpm_wr_reg(ioaddr + RNPM_ETH_PTP_TX_CLEAR(port),
			    PTP_GET_TX_HWTS_UPDATE);
		txstmp = nanosec & PTP_HWTX_TIME_VALUE_MASK;
		txstmp += (sec & PTP_HWTX_TIME_VALUE_MASK) * 1000000000ULL;

		/* Clear the global tx_hwtstamp_skb pointer and force writes
		 * prior to notifying the stack of a Tx timestamp.
		 */
		memset(&shhwtstamps, 0, sizeof(shhwtstamps));
		shhwtstamps.hwtstamp = ns_to_ktime(txstmp);
		adapter->ptp_tx_skb = NULL;
		clear_bit_unlock(__RNPM_PTP_TX_IN_PROGRESS,
				 &adapter->state);
#ifdef DEBUG_PTP_TX_TIMESTAMP
		rnpm_print_human_timestamp(txstmp, "TX");
#endif
		/*
		 * Force memory writes to complete before letting h/w
		 * know there are new descriptors to fetch.  (Only
		 * applicable for weak-ordered memory model archs,
		 * such as IA-64).
		 */
		wmb();
		/* force write prior to skb_tstamp_tx
		 * because the xmit will re used the point to store ptp skb
		 */
		skb_tstamp_tx(skb, &shhwtstamps);
		dev_consume_skb_any(skb);
	} else if (time_after(jiffies,
			      adapter->tx_hwtstamp_start +
				      adapter->tx_timeout_factor * HZ)) {
		/* this function will mark the skb drop*/
		if (adapter->ptp_tx_skb)
			dev_kfree_skb_any(adapter->ptp_tx_skb);
		adapter->ptp_tx_skb = NULL;
		adapter->tx_hwtstamp_timeouts++;
		clear_bit_unlock(__RNPM_PTP_TX_IN_PROGRESS,
				 &adapter->state);
		netdev_warn(adapter->netdev,
			    "clearing Tx timestamp hang\n");
	} else {
		/* reschedule to check later */
#ifdef DEBUG_PTP_HARD_SOFTWAY_TX
		struct skb_shared_hwtstamps shhwtstamp;
		u64 ns = 0;

		ns = rnpm_get_software_ts();
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

void rnpm_ptp_get_rx_hwstamp(struct rnpm_adapter *adapter,
			     union rnpm_rx_desc *desc, struct sk_buff *skb)
{
	u64 ns = 0;
	u64 tsvalueh = 0, tsvaluel = 0;
	struct skb_shared_hwtstamps *hwtstamps = NULL;

	if (!skb || !adapter->ptp_rx_en) {
		netdev_dbg(adapter->netdev,
			   "hwstamp skb is null or rx_en is zero %u\n",
			   adapter->ptp_rx_en);
		return;
	}

#ifdef DEBUG_PTP_HARD_SOFTWAY_RX
	ns = rnpm_get_software_ts();
#else
	if (likely(!((desc->wb.cmd) & RNPM_RXD_STAT_PTP)))
		return;
	hwtstamps = skb_hwtstamps(skb);
	/* because of rx hwstamp store before the mac head
	 * skb->head and skb->data is point to same location when call alloc_skb
	 * so we must move 16 bytes the skb->data to the mac head location
	 * but for the head point if we need move the skb->head need to be diss
	 */
	/* low8 bytes is null high8bytes is timestamp
	 * high 32bit is seconds low32 bits is nanoseconds
	 */
	skb_copy_from_linear_data_offset(skb, RNPM_RX_TIME_RESERVE,
					 &tsvalueh, RNPM_RX_SEC_SIZE);
	skb_copy_from_linear_data_offset(
		skb, RNPM_RX_TIME_RESERVE + RNPM_RX_SEC_SIZE, &tsvaluel,
		RNPM_RX_NANOSEC_SIZE);
	skb_pull(skb, RNPM_RX_HWTS_OFFSET);
	tsvalueh = ntohl(tsvalueh);
	tsvaluel = ntohl(tsvaluel);

	ns = tsvaluel & RNPM_RX_NSEC_MASK;
	ns += ((tsvalueh & RNPM_RX_SEC_MASK) * 1000000000ULL);
	netdev_dbg(adapter->netdev,
		   "ptp get hardware ts-sec %llu ts-nanosec %llu\n",
		   tsvalueh, tsvaluel);
	hwtstamps->hwtstamp = ns_to_ktime(ns);
#endif
#ifdef DEBUG_PTP_RX_TIMESTAMP
	rnpm_print_human_timestamp(ns, "RX");
#endif
}

void rnpm_ptp_reset(struct rnpm_adapter *adapter)
{
	rnpm_ptp_setup_ptp(adapter, adapter->ptp_config_value);
}


#ifndef __SXE_PTP_H__
#define __SXE_PTP_H__

#include <linux/ptp_classify.h>
#include <linux/clocksource.h>
#include "sxe.h"

#define SXE_OVERFLOW_PERIOD     (HZ * 30)
#define SXE_PTP_TX_TIMEOUT      (HZ)
#define SXE_TS_HDR_LEN        8
#define SXE_PTP_RX_TIMEOUT      (5 * HZ)	

#define SXE_TIMADJ_SIGN         0x80000000

#define SXE_PTP_MSG_TYPE_SYNC           0x0000
#define SXE_PTP_MSG_TYPE_DELAY_REQ      0x0001
#define SXE_PTP_MSG_TYPE_MASK           0x000F

#define SXE_PTP_FLAGFIELD_OFFSET        0x0006
#define SXE_PTP_FLAGFIELD_TWOSTEP       0x0002
#define SXE_PTP_FLAGFIELD_UNICAST       0x0004
#define SXE_PTP_FLAGFIELD_MASK          0xFFFF

#define SXE_PTP_MAX_ADJ					125000000

void sxe_ptp_configure(struct sxe_adapter *adapter);

void sxe_ptp_suspend(struct sxe_adapter *adapter);

void sxe_ptp_stop(struct sxe_adapter *adapter);

void sxe_ptp_overflow_check(struct sxe_adapter *adapter);

void sxe_ptp_rx_hang(struct sxe_adapter *adapter);

void sxe_ptp_tx_hang(struct sxe_adapter *adapter);

void sxe_ptp_reset(struct sxe_adapter *adapter);

int sxe_ptp_hw_tstamp_config_set(struct sxe_adapter *adapter, 
						struct ifreq *ifr);

int sxe_ptp_hw_tstamp_config_get(struct sxe_adapter *adapter, 
						struct ifreq *ifr);

void sxe_ptp_get_rx_tstamp_in_pkt(struct sxe_irq_data *irq_data,
			   struct sk_buff *skb);

void sxe_ptp_get_rx_tstamp_in_reg(struct sxe_irq_data *irq_data,
			   struct sk_buff *skb);

static inline void sxe_ptp_rx_hwtstamp_process(struct sxe_ring *rx_ring,
					 union sxe_rx_data_desc *rx_desc,
					 struct sk_buff *skb)
{
	LOG_DEBUG("process rx hwtsamp of ring[%u]\n", rx_ring->idx);

	if (unlikely(sxe_status_err_check(rx_desc, SXE_RXD_STAT_TSIP))) {
		sxe_ptp_get_rx_tstamp_in_pkt(rx_ring->irq_data, skb);
		LOG_DEBUG("we got the time stamp in the end of packet\n");
		goto l_ret;
	}

	if (unlikely(!sxe_status_err_check(rx_desc, SXE_RXDADV_STAT_TS))) {
		LOG_DEBUG("the ptp time stamp is not ready in register\n");
		goto l_ret;
	}

	sxe_ptp_get_rx_tstamp_in_reg(rx_ring->irq_data, skb);
	LOG_DEBUG("we got the time stamp in the time register\n");

	rx_ring->last_rx_timestamp = jiffies;

l_ret:
	return;
}

#endif

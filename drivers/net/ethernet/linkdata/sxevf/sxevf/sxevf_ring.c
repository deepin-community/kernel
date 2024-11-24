#include <linux/kernel.h>
#include <linux/raid/pq.h>

#include "sxevf.h"
#include "sxe_log.h"
#include "sxevf_msg.h"

void sxevf_ring_feature_init(struct sxevf_adapter *adapter)
{
	u16 rss;

	rss = min_t(u16, SXEVF_RSS_RING_NUM_MAX, num_online_cpus());
	adapter->ring_f.rss_limit = rss;

	adapter->tx_ring_ctxt.depth  = SXEVF_TX_DEFAULT_DESC_CNT;
	adapter->rx_ring_ctxt.depth  = SXEVF_RX_DEFAULT_DESC_CNT;

	adapter->xdp_ring_ctxt.depth = 0;
	LOG_INFO_BDF("rss_limit:%u descriptor cnt tx:%u rx:%u xdp:%u.\n",
		     adapter->ring_f.rss_limit,
		     adapter->tx_ring_ctxt.depth,
		     adapter->rx_ring_ctxt.depth,
		     adapter->xdp_ring_ctxt.depth);

	return;
}

s32 sxevf_ring_info_get(struct sxevf_adapter *adapter,
			u8 *tc_num, u8 *default_tc, u8 *max_tx_num)
{
	struct sxevf_hw *hw = &adapter->hw;
	struct sxevf_ring_info_msg req = {};
	s32 ret;

	req.msg_type = SXEVF_RING_INFO_GET;
	spin_lock_bh(&adapter->mbx_lock);
	ret = sxevf_send_and_rcv_msg(hw, (u32 *)&req,
				     SXEVF_MSG_NUM(sizeof(req)));
	spin_unlock_bh(&adapter->mbx_lock);

	if (ret) {
		LOG_ERROR_BDF("msg:0x%x send or rcv reply failed.(err:%d)\n",
			   req.msg_type, ret);
		goto l_out;
	}

	if (req.msg_type != (SXEVF_MSGTYPE_ACK | SXEVF_RING_INFO_GET)) {
		ret = -SXEVF_ERR_REPLY_INVALID;
		LOG_WARN_BDF("msg:0x%x not expected.(err:%d)\n", req.msg_type, ret);
		goto l_out;
	}

	if ((req.max_tx_num == 0) ||
	    (req.max_tx_num > SXEVF_TXRX_RING_NUM_MAX)) {
		req.max_tx_num = SXEVF_TXRX_RING_NUM_MAX;
	}
	*max_tx_num = req.max_tx_num;

	if ((req.max_rx_num == 0) ||
	    (req.max_rx_num > SXEVF_TXRX_RING_NUM_MAX)) {
		req.max_rx_num = SXEVF_TXRX_RING_NUM_MAX;
	}

	if (req.tc_num > req.max_rx_num) {
		req.tc_num = SXEVF_DEFAULT_TC_NUM;
	}
	*tc_num = req.tc_num;

	if (req.default_tc > req.max_tx_num) {
		req.default_tc = 0;
	}
	*default_tc = req.default_tc;

	LOG_INFO_BDF("ring info max_tx_num:%u max_rx_num:%u "
		 "tc_num:%u default_tc:%u.\n",
		 req.max_tx_num, req.max_rx_num, req.tc_num, req.default_tc);

l_out:
	return ret;
}

void sxevf_ring_num_set(struct sxevf_adapter *adapter)
{
	u8 tc_num;
	u8 default_tc;
	u8 max_tx_num; 
	s32 ret;

	adapter->rx_ring_ctxt.num  = SXEVF_TXRX_RING_NUM_DEFAULT;
	adapter->tx_ring_ctxt.num  = SXEVF_TXRX_RING_NUM_DEFAULT;
	adapter->xdp_ring_ctxt.num = 0;

	ret = sxevf_ring_info_get(adapter, &tc_num, &default_tc, &max_tx_num);
	if (ret) {
		LOG_ERROR_BDF("get pf ring cfg info fail, use default_tc ring num."
			  "(err:%d)\n", ret);
		goto l_out;
	}

	if (tc_num > 1) { 
		adapter->cap |= SXEVF_DCB_ENABLE;
		adapter->ring_f.tc_per_pool = tc_num;
		adapter->rx_ring_ctxt.num = tc_num;
	} else {
		u16 rss = adapter->ring_f.rss_limit;

		switch(adapter->mbx_version) {
		case SXEVF_MBX_API_11:
		case SXEVF_MBX_API_12:
		case SXEVF_MBX_API_13:
		case SXEVF_MBX_API_14:
			if (adapter->xdp_prog && (max_tx_num == rss)) {
				rss = rss > 3 ? 2 : 1;
			}

			adapter->tx_ring_ctxt.num = rss;
			adapter->rx_ring_ctxt.num = rss;
			adapter->xdp_ring_ctxt.num = adapter->xdp_prog ? rss : 0;
			fallthrough;
		default:
			break;
		}
	}

	LOG_INFO_BDF("cap = 0x%x mbx version:%u rss:%u rx_ring_num:%d "
		 "tx_ring_num:%d xdp_ring_num:%d tc_num:%u\n",
		 adapter->cap,
		 adapter->mbx_version,
		 adapter->ring_f.rss_limit,
		 adapter->rx_ring_ctxt.num,
		 adapter->tx_ring_ctxt.num,
		 adapter->xdp_ring_ctxt.num,
		 tc_num);
l_out:
	return;
}


static void sxevf_add_ring(struct sxevf_ring *ring,
			struct sxevf_list *head)
{
	ring->next = head->next;
	head->next = ring;
	head->cnt++;

	return;
}

void sxevf_tx_ring_init(struct sxevf_adapter *adapter, u16 base,
		    u16  txr_cnt, u16 ring_idx, u16 irq_idx, u16 reg_idx)
{
	struct sxevf_irq_data *irq_data = adapter->irq_ctxt.irq_data[irq_idx];
	struct sxevf_ring *ring = &(irq_data->ring[base]);
	u16 txr_idx = ring_idx;

	while(txr_cnt) {
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;
		ring->irq_data = irq_data;

		sxevf_add_ring(ring, &irq_data->tx.list);
		irq_data->tx.irq_rate.next_update = jiffies + 1;

		ring->depth = adapter->tx_ring_ctxt.depth;
		ring->idx = txr_idx;
		ring->reg_idx = reg_idx;

		adapter->tx_ring_ctxt.ring[txr_idx] = ring;
		txr_cnt--;
		txr_idx++;
		ring++;
	}

	return;
}

void sxevf_xdp_ring_init(struct sxevf_adapter *adapter, u16 base,
		      u16  xdp_cnt, u16 ring_idx, u16 irq_idx, u16 reg_idx)
{
	struct sxevf_irq_data *irq_data = adapter->irq_ctxt.irq_data[irq_idx];
	struct sxevf_ring *ring = &(irq_data->ring[base]);
	u16 xdp_idx = ring_idx;

	while(xdp_cnt) {
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;
		ring->irq_data = irq_data;

		irq_data->tx.xdp_ring = ring;

		ring->depth = adapter->tx_ring_ctxt.depth;
		ring->idx = xdp_idx;
		ring->reg_idx = reg_idx;
		vf_set_ring_xdp(ring);

		adapter->xdp_ring_ctxt.ring[xdp_idx] = ring;

		xdp_cnt--;
		xdp_idx++;
		ring++;
	}

	return;
}

void sxevf_rx_ring_init(struct sxevf_adapter *adapter, u16 base,
		     u16  rxr_cnt, u16 ring_idx, u16 irq_idx, u16 reg_idx)
{
	struct sxevf_irq_data *irq_data = adapter->irq_ctxt.irq_data[irq_idx];
	struct sxevf_ring *ring = &(irq_data->ring[base]);
	u16 rxr_idx = ring_idx;

	while(rxr_cnt) {
		ring->dev = &adapter->pdev->dev;
		ring->netdev = adapter->netdev;
		ring->irq_data = irq_data;

		sxevf_add_ring(ring, &irq_data->rx.list);
		irq_data->rx.irq_rate.next_update = jiffies + 1;

		ring->depth = adapter->rx_ring_ctxt.depth;
		ring->idx = rxr_idx;
		ring->reg_idx = rxr_idx;

		adapter->rx_ring_ctxt.ring[rxr_idx] = ring;
		rxr_cnt--;
		rxr_idx++;
		ring++;
	}

	return;
}


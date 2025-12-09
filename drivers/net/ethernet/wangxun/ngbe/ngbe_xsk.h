/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 1999 - 2022 Intel Corporation. */

#ifndef _NGBE_TXRX_COMMON_H_
#define _NGBE_TXRX_COMMON_H_

#ifndef NGBE_TXD_CMD
#define NGBE_TXD_CMD (NGBE_TXD_EOP | \
		      NGBE_TXD_RS)
#endif

#define NGBE_XDP_PASS		0
#define NGBE_XDP_CONSUMED	BIT(0)
#define NGBE_XDP_TX		BIT(1)
#define NGBE_XDP_REDIR		BIT(2)

int ngbe_xmit_xdp_ring(struct ngbe_ring *ring, struct xdp_frame *xdpf);

void ngbe_txrx_ring_disable(struct ngbe_adapter *adapter, int ring);
void ngbe_txrx_ring_enable(struct ngbe_adapter *adapter, int ring);

struct xsk_buff_pool *ngbe_xsk_umem(struct ngbe_adapter *adapter, struct ngbe_ring *ring);
int ngbe_xsk_umem_setup(struct ngbe_adapter *adapter, struct xsk_buff_pool *umem, u16 qid);

bool ngbe_alloc_rx_buffers_zc(struct ngbe_ring *rx_ring, u16 cleaned_count);
int ngbe_clean_rx_irq_zc(struct ngbe_q_vector *q_vector, struct ngbe_ring *rx_ring,
			 const int budget);
void ngbe_xsk_clean_rx_ring(struct ngbe_ring *rx_ring);
bool ngbe_clean_xdp_tx_irq(struct ngbe_q_vector *q_vector, struct ngbe_ring *tx_ring);
int ngbe_xsk_wakeup(struct net_device *dev, u32 queue_id, u32 flags);
void ngbe_xsk_clean_tx_ring(struct ngbe_ring *tx_ring);
bool ngbe_xsk_any_rx_ring_enabled(struct ngbe_adapter *adapter);
bool ngbe_cleanup_headers(struct ngbe_ring __maybe_unused *rx_ring, union ngbe_rx_desc *rx_desc,
			  struct sk_buff *skb);
void ngbe_process_skb_fields(struct ngbe_ring *rx_ring, union ngbe_rx_desc *rx_desc,
			     struct sk_buff *skb);
void ngbe_rx_skb(struct ngbe_q_vector *q_vector, struct ngbe_ring *rx_ring,
		 union ngbe_rx_desc *rx_desc, struct sk_buff *skb);

#endif /* _NGBE_TXRX_COMMON_H_ */

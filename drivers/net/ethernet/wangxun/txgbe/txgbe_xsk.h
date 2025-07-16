/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2015 - 2022 Beijing WangXun Technology Co., Ltd. */

#ifndef _TXGBE_TXRX_COMMON_H_
#define _TXGBE_TXRX_COMMON_H_

#include "txgbe.h"

#ifndef TXGBE_TXD_CMD
#define TXGBE_TXD_CMD (TXGBE_TXD_EOP | \
		      TXGBE_TXD_RS)
#endif

#define TXGBE_XDP_PASS		0
#define TXGBE_XDP_CONSUMED	BIT(0)
#define TXGBE_XDP_TX		BIT(1)
#define TXGBE_XDP_REDIR		BIT(2)

int txgbe_xmit_xdp_ring(struct txgbe_ring *ring, struct xdp_frame *xdpf);

void txgbe_txrx_ring_disable(struct txgbe_adapter *adapter, int ring);
void txgbe_txrx_ring_enable(struct txgbe_adapter *adapter, int ring);

struct xsk_buff_pool *txgbe_xsk_umem(struct txgbe_adapter *adapter,
				     struct txgbe_ring *ring);
int txgbe_xsk_umem_setup(struct txgbe_adapter *adapter, struct xsk_buff_pool *umem,
			 u16 qid);

bool txgbe_alloc_rx_buffers_zc(struct txgbe_ring *rx_ring, u16 cleaned_count);

int txgbe_clean_rx_irq_zc(struct txgbe_q_vector *q_vector,
			  struct txgbe_ring *rx_ring,
			  const int budget);
void txgbe_xsk_clean_rx_ring(struct txgbe_ring *rx_ring);
bool txgbe_clean_xdp_tx_irq(struct txgbe_q_vector *q_vector,
			    struct txgbe_ring *tx_ring);

int txgbe_xsk_wakeup(struct net_device *dev, u32 qid, u32 __maybe_unused flags);
void txgbe_xsk_clean_tx_ring(struct txgbe_ring *tx_ring);
bool txgbe_xsk_any_rx_ring_enabled(struct txgbe_adapter *adapter);

bool txgbe_cleanup_headers(struct txgbe_ring __maybe_unused *rx_ring,
			   union txgbe_rx_desc *rx_desc,
			   struct sk_buff *skb);
void txgbe_process_skb_fields(struct txgbe_ring *rx_ring,
			      union txgbe_rx_desc *rx_desc,
			      struct sk_buff *skb);
void txgbe_rx_skb(struct txgbe_q_vector *q_vector,
		  struct txgbe_ring *rx_ring,
		  union txgbe_rx_desc *rx_desc,
		  struct sk_buff *skb);

#endif /* _TXGBE_TXRX_COMMON_H_ */

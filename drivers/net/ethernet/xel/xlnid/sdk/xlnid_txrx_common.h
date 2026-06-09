/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2008 - 2024 Xel Technology. */

#ifndef _XLNID_TXRX_COMMON_H_
#define _XLNID_TXRX_COMMON_H_

#define XLNID_TXD_CMD (XLNID_TXD_CMD_EOP | XLNID_TXD_CMD_RS)

#define XLNID_XDP_PASS 0
#define XLNID_XDP_CONSUMED BIT(0)
#define XLNID_XDP_TX BIT(1)
#define XLNID_XDP_REDIR BIT(2)

#define XLNID_PKT_HDR_PAD (ETH_HLEN + ETH_FCS_LEN + (VLAN_HLEN * 2))

void xlnid_xdp_ring_update_tail(struct xlnid_ring *ring);
void xlnid_xdp_ring_update_tail_locked(struct xlnid_ring *ring);

#ifdef HAVE_XDP_SUPPORT
#ifdef HAVE_XDP_FRAME_STRUCT
int xlnid_xmit_xdp_ring(struct xlnid_ring *ring, struct xdp_frame *xdpf);
#else
int xlnid_xmit_xdp_ring(struct xlnid_ring *ring, struct xdp_buff *xdp);
#endif
#ifdef HAVE_AF_XDP_ZC_SUPPORT
void xlnid_txrx_ring_disable(struct xlnid_adapter *adapter, int ring);
void xlnid_txrx_ring_enable(struct xlnid_adapter *adapter, int ring);

#ifndef HAVE_NETDEV_BPF_XSK_POOL
struct xdp_umem *xlnid_xsk_umem(struct xlnid_adapter *adapter,
								struct xlnid_ring *ring);
int xlnid_xsk_umem_setup(struct xlnid_adapter *adapter, struct xdp_umem *umem,
							u16 qid);
#else
struct xsk_buff_pool *xlnid_xsk_umem(struct xlnid_adapter *adapter,
										struct xlnid_ring *ring);
int xlnid_xsk_umem_setup(struct xlnid_adapter *adapter,
							struct xsk_buff_pool *umem, u16 qid);
#endif /* HAVE_NETDEV_BPF_XSK_POOL */

#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL
void xlnid_alloc_rx_buffers_zc(struct xlnid_ring *rx_ring, u16 cleaned_count);
void xlnid_zca_free(struct zero_copy_allocator *alloc, unsigned long handle);
#else
bool xlnid_alloc_rx_buffers_zc(struct xlnid_ring *rx_ring, u16 cleaned_count);
#endif
int xlnid_clean_rx_irq_zc(struct xlnid_q_vector *q_vector,
							struct xlnid_ring *rx_ring, const int budget);
void xlnid_xsk_clean_rx_ring(struct xlnid_ring *rx_ring);
bool xlnid_clean_xdp_tx_irq(struct xlnid_q_vector *q_vector,
							struct xlnid_ring *tx_ring);
#ifdef HAVE_NDO_XSK_WAKEUP
int xlnid_xsk_wakeup(struct net_device *dev, u32 queue_id, u32 flags);
#else
int xlnid_xsk_async_xmit(struct net_device *dev, u32 queue_id);
#endif
void xlnid_xsk_clean_tx_ring(struct xlnid_ring *tx_ring);
bool xlnid_xsk_any_rx_ring_enabled(struct xlnid_adapter *adapter);
#endif /* HAVE_AF_XDP_ZC_SUPPORT */
#endif /* HAVE_XDP_SUPPORT */

bool xlnid_cleanup_headers(struct xlnid_ring __maybe_unused *rx_ring,
							union xlnid_adv_rx_desc *rx_desc,
							struct sk_buff *skb);
void xlnid_process_skb_fields(struct xlnid_ring *rx_ring,
								union xlnid_adv_rx_desc *rx_desc,
								struct sk_buff *skb);
void xlnid_rx_skb(struct xlnid_q_vector *q_vector, struct xlnid_ring *rx_ring,
					union xlnid_adv_rx_desc *rx_desc, struct sk_buff *skb);

void xlnid_irq_rearm_queues(struct xlnid_adapter *adapter, u64 qmask);
#endif /* _XLNID_TXRX_COMMON_H_ */


#ifndef __SXE_XDP_H__
#define __SXE_XDP_H__

#include "sxe.h"
#ifdef HAVE_XDP_SUPPORT
#include <linux/bpf.h>
#include <linux/bpf_trace.h>
#endif
#ifndef HAVE_NO_XDP_BUFF_RXQ
#include <net/xdp.h>
#endif

#define SXE_XDP_PASS		(0)
#define SXE_XDP_CONSUMED	BIT(0)
#define SXE_XDP_TX		BIT(1)
#define SXE_XDP_REDIR		BIT(2)

#ifdef HAVE_AF_XDP_ZERO_COPY
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
#include <net/xdp_sock_drv.h>
#else
#include <net/xdp_sock.h>
#endif

static inline bool sxe_xdp_adapter_enabled(struct sxe_adapter *adapter)
{
	return !!adapter->xdp_prog;
}

#ifndef HAVE_MEM_TYPE_XSK_BUFF_POOL

bool sxe_xdp_tx_ring_irq_clean(struct sxe_irq_data *irq_data,
			    struct sxe_ring *tx_ring, int napi_budget);

int sxe_zc_rx_ring_irq_clean(struct sxe_irq_data *irq_data,
			  struct sxe_ring *rx_ring, const int budget);

int sxe_xsk_async_xmit(struct net_device *dev, u32 qid);

void sxe_zca_free(struct zero_copy_allocator *alloc,
					unsigned long handle_addr);

void sxe_xsk_rx_ring_clean(struct sxe_ring *rx_ring);

void sxe_xsk_tx_ring_clean(struct sxe_ring *tx_ring);

void sxe_zc_rx_ring_buffers_alloc(struct sxe_ring *rx_ring, u16 count);

#else

bool sxe_zc_rx_ring_buffers_alloc(struct sxe_ring *rx_ring, u16 count);

s32 sxe_zc_rx_ring_irq_clean(struct sxe_irq_data *irq_data,
			  struct sxe_ring *rx_ring,
			  const int budget);

void sxe_xsk_rx_ring_clean(struct sxe_ring *rx_ring);

bool sxe_xdp_tx_ring_irq_clean(struct sxe_irq_data *irq_data,
			    struct sxe_ring *tx_ring, int napi_budget);

void sxe_xsk_tx_ring_clean(struct sxe_ring *tx_ring);

#endif

#ifdef HAVE_NETDEV_BPF_XSK_BUFF_POOL
struct xsk_buff_pool *sxe_xsk_pool_get(struct sxe_adapter *adapter,
				     struct sxe_ring *ring);
#else
struct xdp_umem *sxe_xsk_pool_get(struct sxe_adapter *adapter,
				struct sxe_ring *ring);
#endif

#ifdef HAVE_NDO_XSK_WAKEUP
int sxe_xsk_wakeup(struct net_device *dev, u32 qid,
					u32 __maybe_unused flags);
#endif

#endif 

#ifdef HAVE_XDP_SUPPORT
DECLARE_STATIC_KEY_FALSE(sxe_xdp_tx_lock_key);
static inline
struct sxe_ring *sxe_xdp_tx_ring_pick(struct sxe_adapter *adapter)
{
	s32 cpu = smp_processor_id();
	u16 idx = static_key_enabled(&sxe_xdp_tx_lock_key) ? \
				cpu % SXE_XDP_RING_NUM_MAX : cpu;

	return adapter->xdp_ring_ctxt.ring[idx];
}

void sxe_xdp_ring_tail_update_locked(struct sxe_ring *ring);

int sxe_xdp(struct net_device *dev, struct netdev_bpf *xdp);

int sxe_xdp_xmit(struct net_device *dev, int n,
			  struct xdp_frame **frames, u32 flags);

u32 sxe_max_xdp_frame_size(struct sxe_adapter *adapter);
#endif 

struct sk_buff *sxe_xdp_run(struct sxe_adapter *adapter,
				     struct sxe_ring *rx_ring,
				     struct xdp_buff *xdp);

#endif 

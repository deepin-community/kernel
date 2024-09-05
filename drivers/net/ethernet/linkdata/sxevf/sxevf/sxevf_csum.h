
#ifndef __SXEVF_CSUM_H__
#define __SXEVF_CSUM_H__

#include <net/ipv6.h>
#include <net/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>

#include "sxevf.h"
#include "sxevf_ring.h"

#ifdef NOT_INCLUDE_SCTP_H
typedef struct sctphdr {
	__be16 source;
	__be16 dest;
	__be32 vtag;
	__le32 checksum;
} __packed sctp_sctphdr_t;
#else
#include <linux/sctp.h>
#endif

#define    SXEVF_TCP_CSUM_OFFSET    (offsetof(struct tcphdr, check))
#define    SXEVF_UDP_CSUM_OFFSET    (offsetof(struct udphdr, check))
#define    SXEVF_SCTP_CSUM_OFFSET   (offsetof(struct sctphdr, checksum))

void sxevf_tx_csum_offload(struct sxevf_ring *tx_ring,
				  struct sxevf_tx_buffer *first,
				  struct sxevf_tx_context_desc *ctxt_desc);

void sxevf_rx_csum_verify(struct sxevf_ring *ring,
			      union sxevf_rx_data_desc *desc,
			      struct sk_buff *skb);

#endif

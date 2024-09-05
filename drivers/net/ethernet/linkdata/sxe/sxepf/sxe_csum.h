
#ifndef __SXE_CSUM_H__
#define __SXE_CSUM_H__

#include <net/ipv6.h>
#include <net/ip.h>
#include <linux/udp.h>
#include <linux/tcp.h>

#include "sxe_ipsec.h"
#include "sxe.h"

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

#define    SXE_TCP_CSUM_OFFSET    (offsetof(struct tcphdr, check))
#define    SXE_UDP_CSUM_OFFSET    (offsetof(struct udphdr, check))
#define    SXE_SCTP_CSUM_OFFSET   (offsetof(struct sctphdr, checksum))

void sxe_tx_csum_offload(struct sxe_ring *tx_ring,
				  struct sxe_tx_buffer *first,
				  struct sxe_tx_context_desc *ctxt_desc);

void sxe_rx_csum_verify(struct sxe_ring *ring,
			      union sxe_rx_data_desc *desc,
			      struct sk_buff *skb);

#endif


#ifndef __SXEVF_XDP_H__
#define __SXEVF_XDP_H__

#include "sxevf.h"
#ifdef HAVE_XDP_SUPPORT
#include <linux/bpf.h>
#include <linux/bpf_trace.h>
#endif

#ifdef HAVE_AF_XDP_ZERO_COPY
#ifdef HAVE_MEM_TYPE_XSK_BUFF_POOL
#include <net/xdp_sock_drv.h>
#else
#include <net/xdp_sock.h>
#endif
#endif

#define SXEVF_XDP_PASS 		0
#define SXEVF_XDP_CONSUMED 	1
#define SXEVF_XDP_TX 		2

#ifdef HAVE_XDP_SUPPORT
s32 sxevf_xdp(struct net_device *dev, struct netdev_bpf *xdp);

#endif
struct sk_buff *sxevf_xdp_run(struct sxevf_adapter *adapter,
				       struct sxevf_ring  *rx_ring,
				       struct xdp_buff *xdp);

#endif 

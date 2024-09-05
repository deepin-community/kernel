
#ifndef __SXE_DEBUG_H__
#define __SXE_DEBUG_H__

#include <linux/skbuff.h>
#include "sxe_log.h"

void sxevf_dump_skb(struct sk_buff *skb);

#if defined SXE_DRIVER_RELEASE
#define SKB_DUMP(skb)
#else
#define SKB_DUMP(skb)  sxevf_dump_skb(skb)
#endif

#endif


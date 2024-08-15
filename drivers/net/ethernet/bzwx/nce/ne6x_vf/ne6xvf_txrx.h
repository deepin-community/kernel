/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6XVF_TXRX_H
#define _NE6XVF_TXRX_H

void ne6xvf_unmap_and_free_tx_resource(struct ne6x_ring *ring, struct ne6x_tx_buf *tx_buffer);
int ne6xvf_napi_poll(struct napi_struct *napi, int budget);
netdev_tx_t ne6xvf_lan_xmit_frame(struct sk_buff *skb, struct net_device *netdev);

#endif

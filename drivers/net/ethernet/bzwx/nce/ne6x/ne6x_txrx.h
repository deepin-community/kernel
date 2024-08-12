/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#ifndef _NE6X_TXRX_H
#define _NE6X_TXRX_H

int         ne6x_napi_poll(struct napi_struct *napi, int budget);
netdev_tx_t ne6x_lan_xmit_frame(struct sk_buff *skb, struct net_device *netdev);
bool        ne6x_alloc_rx_buffers(struct ne6x_ring *rx_ring, u16 cleaned_count);

#endif

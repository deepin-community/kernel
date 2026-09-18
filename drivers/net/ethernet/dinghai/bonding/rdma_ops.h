#ifndef ZXDH_RDMA_OPS_H
#define ZXDH_RDMA_OPS_H

#include <linux/dinghai/driver.h>
#include <linux/netdevice.h>

struct zxdh_rdma_hb_if {
    int32_t (*cfg_rdma_hb_master)(struct net_device *primary_netdev, struct net_device *linux_bond_netdev, bool hb_enable);
    int32_t (*cfg_rdma_hb_speed)(struct net_device *netdev, uint32_t bps);
};

int32_t zxdh_set_rdma_hwbond_master(struct net_device *primary_netdev, struct net_device *linux_bond_netdev, bool hb_enable);
int32_t zxdh_set_rdma_hwbond_speed(struct net_device *netdev, uint32_t bps);

extern void zxdh_update_rdma_hwbond_master(void);

#endif
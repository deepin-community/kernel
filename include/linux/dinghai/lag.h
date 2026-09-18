#ifndef __DINGHAI_LAG_H__
#define __DINGHAI_LAG_H__

#include <linux/netdevice.h>
#include <linux/dinghai/driver.h>

#define ZXDH_PF_VFID(ep, pf)     (1152 + ep * 8 + pf)


enum zxdh_netdev_lag_tx_type
{
    ZXDH_NETDEV_LAG_TX_TYPE_UNKNOWN         = 0,
    ZXDH_NETDEV_LAG_TX_TYPE_ACTIVEBACKUP    = 1,
    ZXDH_NETDEV_LAG_TX_TYPE_HASH            = 2,
    /* not surpported follow */
    ZXDH_NETDEV_LAG_TX_TYPE_RANDOM,
    ZXDH_NETDEV_LAG_TX_TYPE_BROADCAST,
    ZXDH_NETDEV_LAG_TX_TYPE_ROUNDROBIN,
};

enum zxdh_netdev_lag_hash
{
    ZXDH_NETDEV_LAG_HASH_NONE   = 0,    /* L2 default */
    ZXDH_NETDEV_LAG_HASH_L2     = 1,
    ZXDH_NETDEV_LAG_HASH_L23    = 2,
    ZXDH_NETDEV_LAG_HASH_L34    = 4,
    // ZXDH_NETDEV_LAG_HASH_E23,
    // ZXDH_NETDEV_LAG_HASH_E34,
    // ZXDH_NETDEV_LAG_HASH_VLAN_SRCMAC,
    ZXDH_NETDEV_LAG_HASH_UNKNOWN,
};

enum bond_port_type {
    BOND_TWO_PORT_TYPE = 0,              /* 2口网卡配置bond */
    BOND_MULTI_PORT_TYPE = 1,            /* 多口(大于等于3)网卡配置bond*/
};

struct zxdh_lag_attrs
{
    uint16_t pannel_id;
    uint16_t vport;
    uint32_t qid[2];
    uint16_t slot_id;
    uint16_t pcie_id;
    uint8_t  phy_port;
    uint8_t  rsv;
};

void zxdh_regitster_ldev(struct dh_core_dev *dh_devs);
void zxdh_unregitster_ldev(struct dh_core_dev *dh_dev);

int32_t zxdh_ldev_add_netdev(struct dh_core_dev *dev, uint16_t ida, struct net_device *netdev, struct zxdh_lag_attrs *attr);
void zxdh_ldev_remove_netdev(struct dh_core_dev *dh_dev, struct net_device *netdev, struct zxdh_lag_attrs *attr);

/* sriov netdev hardware bond */
int32_t zxdh_hardware_bond_init(struct net_device *netdev);
enum bond_port_type zxdh_get_bond_port_type(enum dh_board_type board_type);
int32_t zxdh_recover_hwbond_in_reload(struct net_device *netdev);

#endif /* __DINGHAI_LAG_H__ */
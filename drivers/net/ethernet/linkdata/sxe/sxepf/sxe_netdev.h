#ifndef __SXE_NETDEV_H__
#define __SXE_NETDEV_H__

#include <linux/netdevice.h>
#include <linux/if_bridge.h>

#include "sxe.h"

#define SXE_GSO_PARTIAL_FEATURES (NETIF_F_GSO_GRE | \
				NETIF_F_GSO_GRE_CSUM | \
				NETIF_F_GSO_IPXIP4 | \
				NETIF_F_GSO_IPXIP6 | \
				NETIF_F_GSO_UDP_TUNNEL | \
				NETIF_F_GSO_UDP_TUNNEL_CSUM)

s32 sxe_link_config(struct sxe_adapter *adapter);

int sxe_open(struct net_device *netdev);

int sxe_close(struct net_device *netdev);

void sxe_set_rx_mode(struct net_device *netdev);

void __sxe_set_rx_mode(struct net_device *netdev, bool lock);

bool netif_is_sxe(struct net_device *dev);

void sxe_netdev_init(struct net_device *netdev, struct pci_dev *pdev);

void sxe_down(struct sxe_adapter *adapter);

void sxe_up(struct sxe_adapter *adapter);

void sxe_terminate(struct sxe_adapter *adapter);

void sxe_hw_reinit(struct sxe_adapter *adapter);

void sxe_reset(struct sxe_adapter *adapter);

void sxe_do_reset(struct net_device *netdev);

s32 sxe_ring_reassign(struct sxe_adapter *adapter, u8 tc);

s32 sxe_vlan_rx_add_vid(struct net_device *netdev,
				 __be16 proto, u16 vid);

#ifndef NO_NEED_POOL_DEFRAG
void sxe_macvlan_pools_defrag(struct net_device *dev);
#endif

void sxe_macvlan_configure(struct sxe_adapter *adapter);

u32 sxe_sw_mtu_get(struct sxe_adapter *adapter);

void sxe_stats_update(struct sxe_adapter *adapter);

u32 sxe_mbps_link_speed_get(u32 speed);
#endif
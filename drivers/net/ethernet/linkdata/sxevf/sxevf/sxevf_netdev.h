#ifndef __SXEVF_NETDEV_H__
#define __SXEVF_NETDEV_H__

#include <linux/netdevice.h>
#include "sxevf.h"

#define SXEVF_GSO_PARTIAL_FEATURES (NETIF_F_GSO_GRE | \
				NETIF_F_GSO_GRE_CSUM | \
				NETIF_F_GSO_IPXIP4 | \
				NETIF_F_GSO_IPXIP6 | \
				NETIF_F_GSO_UDP_TUNNEL | \
				NETIF_F_GSO_UDP_TUNNEL_CSUM)

int sxevf_open(struct net_device *netdev);

void sxevf_netdev_init(struct sxevf_adapter *adapter, struct pci_dev *pdev);

void sxevf_reset(struct sxevf_adapter *adapter);

void sxevf_terminate(struct sxevf_adapter *adapter);

void sxevf_down(struct sxevf_adapter *adapter);

int sxevf_close(struct net_device *netdev);

void sxevf_up(struct sxevf_adapter *adapter);

void sxevf_hw_reinit(struct sxevf_adapter *adapter);

void sxevf_set_rx_mode(struct net_device *netdev);

int sxevf_vlan_rx_add_vid(struct net_device *netdev,
				   __be16 proto, u16 vid);

int sxevf_vlan_rx_kill_vid(struct net_device *netdev,
				    __be16 proto, u16 vid);

void sxevf_update_stats(struct sxevf_adapter *adapter);

u32 sxevf_sw_mtu_get(struct sxevf_adapter *adapter);

void sxevf_sw_mtu_set(struct sxevf_adapter *adapter, u32 new_mtu);

#endif

#ifndef __EN_AUX_EVENTS_H__
#define __EN_AUX_EVENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>
#include <net/ip.h>
#include <net/vxlan.h>
#include <linux/ip.h>
#include "en_aux.h"
#include "../en_np/table/include/dpp_tbl_comm.h"
#include "en_aux_ioctl.h"

#define MULTI_FLAG      (0x01)
#define IPV4_TYPE_FLAG  (0x00)
#define GLOBAL_FLAG     (0x5E)
#define BIT16           (16)
#define BIT8            (8)
#define BIT_23_L        (0x7F)
#define BIT_15_L        (0xFF)
#define BIT_7_L         (0xFF)

enum {
    ZXDH_RDMA_HEALTH_EVENT = 1,
    ZXDH_RDMA_SRIOV_EVENT = 2,
    ZXDH_RDMA_ETS_EVENT = 3,
    ZXDH_RDMA_TC_MAX_RATE_EVENT = 4,
    ZXDH_RDMA_ETS_SWITCH_EVENT = 5,
    ZXDH_RDMA_TRUST_MODE_EVENT = 6,
    ZXDH_RDMA_VLAN_EVENT = 7,
    ZXDH_RDMA_SPEED_CHANGE_EVENT = 8,
};

struct zxdh_rdma_sriov_event_info
{
    struct pci_dev *pdev;
    uint64_t bar0_virt_addr;
    uint16_t vport_id;
    uint16_t num_vfs;
};

struct zxdh_rdma_vlan_event_info
{
    uint16_t vf_idx;
    uint16_t vlan_id;
    uint16_t protocol;
    uint8_t qos;
}__attribute__((__packed__));

int32_t dh_aux_events_init(struct zxdh_en_priv *en_priv);
void dh_aux_events_uninit(struct zxdh_en_priv *en_priv);
int32_t dh_aux_msg_recv_func_register(void);
void dh_aux_msg_recv_func_unregister(void);
int32_t dh_aux_ipv6_notifier_init(struct zxdh_en_priv *en_priv);
int32_t dh_aux_vxlan_netdev_notifier_init(struct zxdh_en_priv *en_priv);
int32_t dh_aux_vlan_netdev_notifier_init(struct zxdh_en_device *en_dev);
int32_t dh_aux_vlan_netdev_notifier_uninit(struct zxdh_en_device *en_dev);
int32_t dh_ip_mac_init(struct zxdh_en_priv *en_priv);
int32_t zxdh_rdma_events_call(struct net_device *netdev, uint8_t event_type, void *data);
int32_t zxdh_pf_set_vf_vlan_info(struct zxdh_en_device *en_dev, int32_t vf_idx,
                            uint16_t vlan_id, uint8_t qos, uint16_t vlan_proto);
void zxdh_cap_pkt_uninit(struct zxdh_en_device *en_dev, bool offload_mode);
void zxdh_rdma_device_lock_init(void);
void zxdh_rdma_device_lock_deinit(void);
void update_vf_link_info(struct zxdh_en_device *en_dev, uint8_t link_info, bool update_all);
void bond_ipv6_mcast_update_slave(struct net_device *bond_dev, struct zxdh_en_device *slave_en_dev, bool is_link);

#ifdef __cplusplus
}
#endif

#endif

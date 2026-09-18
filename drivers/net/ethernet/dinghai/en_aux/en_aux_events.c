#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/dinghai/events.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include "en_aux_events.h"
#include "en_aux_eq.h"
#include "en_aux_cmd.h"
#include "../msg_common.h"
#include "../en_np/table/include/dpp_tbl_api.h"
#include "../zxdh_tools/zxdh_tools_netlink.h"
#include "../zxdh_tools/zxdh_tools_ioctl.h"
#include "dcbnl/en_dcbnl_api.h"
#include "zxic_common.h"
#include <linux/timer.h>
#include <linux/rtc.h>
#include <linux/if_ether.h>
#include <linux/in6.h>
#include <net/addrconf.h>
#include <linux/if_vlan.h>       // 对于VLAN设备
#include <linux/if_bonding.h>    // 对于bonding设备
#include <net/bonding.h>
#ifndef CGS_V5_693
#include <linux/umh.h>
#endif
#include "../en_ethtool/ethtool.h"
#include "../bonding/zxdh_lag.h"

static struct mutex rdma_lock;
int32_t zxdh_cfg_reload(struct zxdh_en_device *en_dev);
void zxdh_aux_unload(struct zxdh_en_priv *en_priv);
int32_t zxdh_aux_load(struct zxdh_en_priv *en_priv);
static int32_t pf2vf_notifier(struct notifier_block *, unsigned long, void *);
static int32_t riscv2aux_notifier(struct notifier_block *, unsigned long, void *);
static int32_t aux_unload_notifier(struct notifier_block *, unsigned long, void *);
static int32_t aux_load_notifier(struct notifier_block *, unsigned long, void *);
static int32_t rdma_load_notifier(struct notifier_block *nb, unsigned long type, void *data);
static int32_t aux_state_notifier(struct notifier_block *, unsigned long, void *);
static int32_t aux_info_notifier(struct notifier_block *, unsigned long, void *);
static int32_t rdma_event_notifier(struct notifier_block *nb, unsigned long type, void *data);
void zxdh_eth_config_show(struct net_device *netdev);
int32_t zxdh_eth_config_recover(struct net_device *netdev);

void rx_mode_set_handler(struct work_struct *work);

static struct dh_nb aux_events[] = {
    {.nb.notifier_call = pf2vf_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_PF_TO_VF},
    {.nb.notifier_call = aux_unload_notifier, .event_type = DH_EVENT_TYPE_AUX_UNLOAD},
    {.nb.notifier_call = aux_load_notifier, .event_type = DH_EVENT_TYPE_AUX_LOAD},
    {.nb.notifier_call = rdma_load_notifier, .event_type = DH_EVENT_TYPE_RDMA_LOAD},
    {.nb.notifier_call = aux_state_notifier, .event_type = DH_EVENT_TYPE_AUX_STATE},
    {.nb.notifier_call = riscv2aux_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_RISCV_TO_AUX},
    {.nb.notifier_call = aux_info_notifier, .event_type = DH_EVENT_TYPE_AUX_INFO},
    {.nb.notifier_call = rdma_event_notifier, .event_type = DH_EVENT_TYPE_RDMA_EVENT_NOTIFY},
};

void zxdh_rdma_device_lock_init(void)
{
    mutex_init(&rdma_lock);
}

void zxdh_rdma_device_lock_deinit(void)
{
    mutex_destroy(&rdma_lock);
}

static int32_t do_pf_vf_inet6_update_mac_to_np(struct zxdh_en_device *en_dev, const struct in6_addr *ipv6_addr, unsigned long action)
{
    int32_t ret = 0;
    struct in6_addr sol_addr={0};
    uint8_t mcast_mac[ETH_ALEN];

    // 打印IPv6地址，使用%pI6c格式化IPv6地址，确保正确显示
    DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "IPv6 address changed on interface %s, %s address: %pI6c\n",
        en_dev->netdev->name, (action == 1) ? "add" : (action == 2) ? "del" : "unknown action with", ipv6_addr);
    // Calculate the multicast MAC address from the IPv6 address
    addrconf_addr_solict_mult(ipv6_addr, &sol_addr);
    ipv6_eth_mc_map(&sol_addr, mcast_mac);
    DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "Multicast MAC Address: %pM\n", mcast_mac);

    switch (action) {
        case NETDEV_UP:
        {
            ret = zxdh_ip6mac_add_safe(en_dev, ipv6_addr->s6_addr32, mcast_mac);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "zxdh_ip6mac_add_safe failed");
            }
            break;
        }
        case NETDEV_DOWN:
        {
            ret = zxdh_ip6mac_del_safe(en_dev, ipv6_addr->s6_addr32, mcast_mac);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "zxdh_ip6mac_del_safe failed");
            }
            break;
        }
        default:
            break;
    }
    return ret;
}

static int32_t do_pf_vf_vxlan_update_mac_to_np(struct zxdh_en_device *en_dev, uint8_t *mcast_mac, unsigned long action)
{
    int32_t ret = 0;

    switch (action) {
        case NETDEV_UP:
        {
            ret = zxdh_ip4mac_add(en_dev, mcast_mac, action);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "zxdh_ip4mac_add failed\n");
                return ret;
            }
            break;
        }
        case NETDEV_DOWN:
        {
            ret = zxdh_ip4mac_del(en_dev, mcast_mac, action);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "zxdh_ip6mac_del failed\n");
                return ret;
            }
            break;
        }
        default:
            break;
    }
    return ret;
}


static int32_t do_pf_vf_vlan_update_mac_to_np(struct zxdh_en_device *en_dev, struct net_device *vlan_dev, uint16_t vlan_id, \
                                                const uint8_t *vlan_mac, unsigned long action)
{
    struct net_device *netdev = en_dev->netdev;
    uint8_t old_vlan_mac[ETH_ALEN] = {0};
    int32_t ret = 0;

    switch (action)
    {
        case NETDEV_UP:
        {
            /* vlan子接口和父设备的mac地址相同，则不需要做任何操作，直接返回 */
            if (ether_addr_equal(vlan_mac, netdev->dev_addr))
                return ret;

            /* vlan子接口和父设备的mac地址不相同，需要重配该mac表项 */
            ret = zxdh_vlan_mac_add(en_dev, vlan_id, vlan_mac);
            if (ret)
            {
                LOG_ERR("zxdh_vlan_mac_add failed\n");
                return ret;
            }
            break;
        }
        case NETDEV_UNREGISTER:
        case NETDEV_DOWN:
        {
            /* vlan子接口和父设备的mac地址相同，则不需要做任何操作，直接返回 */
            if (ether_addr_equal(vlan_mac, netdev->dev_addr))
                return ret;

            /* 防止多个事件重复调用删除 */
            if (!zxdh_old_vlan_mac_get(en_dev, old_vlan_mac, vlan_id))
                return ret;

            /* vlan子接口和父设备的mac地址不相同，需要从np中删除表项，且从映射表中删除 */
            ret = zxdh_vlan_mac_del(en_dev, vlan_id, vlan_mac);
            if (ret != 0)
            {
                LOG_ERR("zxdh_vlan_mac_del failed\n");
                return ret;
            }
            break;
        }
        case NETDEV_CHANGEADDR:
        {
            if (zxdh_old_vlan_mac_get(en_dev, old_vlan_mac, vlan_id))
            {
                /* 找到旧mac地址，并从映射表中删除*/
                ret = zxdh_vlan_mac_del(en_dev, vlan_id, old_vlan_mac);
                if (ret != 0)
                {
                    LOG_ERR("zxdh_vlan_mac_del failed\n");
                    return ret;
                }
            }

            /* 改配之后，如果vlan子接口和父设备的mac地址相同，则直接返回 */
            if (!memcmp(vlan_mac, netdev->dev_addr, ETH_ALEN))
                return ret;

            if(!(vlan_dev->flags & IFF_UP)) /* vlan子接口未up之前改配的mac先不配到np中*/
                return ret;

             /* 改配之后，如果vlan子接口和父设备的mac地址不同，则需要配置该mac表项， */
            ret = zxdh_vlan_mac_add(en_dev, vlan_id, vlan_mac);
            if (ret != 0)
            {
                LOG_ERR("zxdh_vlan_mac_add failed\n");
                return ret;
            }
            break;
        }
        default:
            break;
    }
    return ret;
}


static int32_t do_bond_master_inet6_update_mac_to_np(struct net_device *notifier_dev, const struct in6_addr *ipv6_addr, struct zxdh_en_device *en_dev, unsigned long action)
{
    int32_t ret = 0;
    struct list_head *iter = NULL;
    struct slave *slave_dev = NULL;
    struct bonding *bond = netdev_priv(notifier_dev);

    // 遍历所有slave设备
    if (!bond_has_slaves(bond))
    {
        DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "Bond device %s don't have slave\n", notifier_dev->name);
        return 0;
    }

    bond_for_each_slave(bond, slave_dev, iter)
    {
        if (strcmp(en_dev->netdev->name, slave_dev->dev->name) != 0)
        {
            continue;
        }
        DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "Bond device %s have slave device: %s\n",
            notifier_dev->name, slave_dev->dev->name);
        ret = do_pf_vf_inet6_update_mac_to_np(en_dev, ipv6_addr, action);
        if (ret != 0)
        {
            return ret;
        }
    }
    return 0;
}

/* Started by AICoder, pid:3defdocdc2l81c61479f08131055a94bd1f52059 */
static bool find_lower_dev_matching_ipvlan(struct net_device **notifier_dev, struct zxdh_en_device *en_dev)
{
    struct net_device *lower = NULL;
    struct list_head *lower_iter = NULL;
    struct slave *slave_dev = NULL;
    struct list_head *slave_iter = NULL;
    struct bonding *bond = NULL;

    netdev_for_each_lower_dev(*notifier_dev, lower, lower_iter)
    {
        if (lower && strcmp(en_dev->netdev->name, lower->name) == 0)
        {
            *notifier_dev = lower;
            return true;
        }

        if (netif_is_bond_master(lower))
        {
            bond = netdev_priv(lower);
            if (!bond || !bond_has_slaves(bond))
            {
                continue;
            }

            bond_for_each_slave(bond, slave_dev, slave_iter)
            {
                if (slave_dev && slave_dev->dev &&
                    strcmp(en_dev->netdev->name, slave_dev->dev->name) == 0)
                {
                    *notifier_dev = lower;
                    return true;
                }
            }
        }
    }
    return false;
}
/* Ended by AICoder, pid:3defdocdc2l81c61479f08131055a94bd1f52059 */

static int32_t inet6_addr_change_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
    struct inet6_ifaddr *ifa = NULL;
    struct net_device *notifier_dev = NULL;  //触发事件的网络设备
    bool found = false;
    struct zxdh_en_device *en_dev = container_of(nb, struct zxdh_en_device, ipv6_notifier);  //处理此回调函数的设备

    if (data == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "data is NULL");
        return NOTIFY_OK;
    }

    ifa = (struct inet6_ifaddr *)data;
    notifier_dev = ifa->idev->dev;

    if (notifier_dev == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "notifier_dev is NULL");
        return NOTIFY_OK;
    }

    // 检查是否为vlan设备
    if (is_vlan_dev(notifier_dev))
    {
        notifier_dev = vlan_dev_real_dev(notifier_dev);
        if (notifier_dev == NULL)
        {
            LOG_ERR("notifier_dev is NULL");
            return NOTIFY_OK;
        }
    }

    /* Started by AICoder, pid:s7b48x97ca7915c1436008c93027a42005b640c8 */
    if (notifier_dev->rtnl_link_ops && strcmp(notifier_dev->rtnl_link_ops->kind, "ipvlan") == 0)
    {
        found = find_lower_dev_matching_ipvlan(&notifier_dev, en_dev);
        if (!found || notifier_dev == NULL)
        {
            LOG_DEBUG_DEV(en_dev->parent, "Failed to find lower device matching ipvlan\n");
            return NOTIFY_OK;
        }
    }
    /* Ended by AICoder, pid:s7b48x97ca7915c1436008c93027a42005b640c8 */

    // 检查是否为bond master设备
    if (netif_is_bond_master(notifier_dev))
        return do_bond_master_inet6_update_mac_to_np(notifier_dev, &ifa->addr, en_dev, action);

    // 检查是否为自定义设备
    if (strcmp(en_dev->netdev->name, notifier_dev->name) == 0)
        return do_pf_vf_inet6_update_mac_to_np(en_dev, &ifa->addr, action);

    return NOTIFY_OK;
}

static void multicast_ipv4_to_mac(struct in_addr ipv4_addr, uint8_t *mac_addr)
{
    uint32_t ip = ntohl(ipv4_addr.s_addr);

    mac_addr[0] = MULTI_FLAG;
    mac_addr[1] = IPV4_TYPE_FLAG;
    mac_addr[2] = GLOBAL_FLAG;
    mac_addr[3] = (ip >> BIT16) & BIT_23_L; /* Take bits 16-23 from the IP address*/
    mac_addr[4] = (ip >> BIT8) & BIT_15_L;  /* Take bits 8-15 from the IP address */
    mac_addr[5] = ip & BIT_7_L;             /* Take bits 0-7 from the IP address */

    return;
}

static int32_t vxlan_netdev_change_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
    struct vxlan_dev *vxlan = NULL;
    struct net_device *notifier_dev = netdev_notifier_info_to_dev(data); /* 通知设备 */
    struct zxdh_en_device *en_dev = container_of(nb, struct zxdh_en_device, vxlan_notifier); /* 父设备相关 */
    struct vxlan_config *cfg = NULL;
    uint32_t ipv4_addr = 0;
    struct in6_addr *ipv6_addr = NULL;
    uint8_t mac_addr[6] = {0};

    int32_t ret = 0;

    if (notifier_dev == NULL)
    {
        LOG_ERR("notifier_dev is NULL\n");
        return NOTIFY_BAD;
    }

    if (en_dev == NULL)
    {
        LOG_ERR("en_dev is NULL\n");
        return NOTIFY_BAD;
    }

    /* 检查是否为vxlan设备 */
    if (!(notifier_dev->rtnl_link_ops && strcmp(notifier_dev->rtnl_link_ops->kind, "vxlan") == 0))
    {
        return NOTIFY_DONE;
    }

    /* 获取vxlan设备的父设备信息 */
    en_dev = container_of(nb, struct zxdh_en_device, vxlan_notifier);
    if (en_dev == NULL)
    {
        LOG_ERR("en_dev is NULL\n");
        return NOTIFY_BAD;
    }

    vxlan = netdev_priv(notifier_dev);
    cfg = &vxlan->cfg;

    /* 判断ip地址类型 */
    if (cfg->remote_ip.sa.sa_family == AF_INET)
    {
        ipv4_addr = cfg->remote_ip.sin.sin_addr.s_addr;
        if ((ipv4_addr & htonl(0xF0000000)) != htonl(0xE0000000))
        {
            return NOTIFY_DONE;
        }
        /* 转成对应的组播mac */
        multicast_ipv4_to_mac(cfg->remote_ip.sin.sin_addr, mac_addr);
        LOG_DEBUG_DEV(en_dev->parent, "VXLAN device %s IPv4 address %pI4 to multi mac %pM\n",
                notifier_dev->name, &cfg->remote_ip.sin.sin_addr, mac_addr);
    }
    else if (cfg->remote_ip.sa.sa_family == AF_INET6)
    {
        ipv6_addr = &cfg->remote_ip.sin6.sin6_addr;
        if (ipv6_addr->s6_addr[0] != 0xFF)
        {
            return NOTIFY_DONE;
        }
        /* 转成对应的组播mac */
        ipv6_eth_mc_map(ipv6_addr, mac_addr);
        LOG_DEBUG_DEV(en_dev->parent, "VXLAN device %s IPv6 address %pI6c to multi mac %pM\n",
                 notifier_dev->name, ipv6_addr, mac_addr);
    }
    else
    {
        LOG_INFO_DEV(en_dev->parent, "Unsupported address family\n");
    }

    ret = do_pf_vf_vxlan_update_mac_to_np(en_dev, mac_addr, action);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "do_pf_vf_vxlan_update_mac_to_np failed\n");
        return NOTIFY_BAD;
    }
    return NOTIFY_OK;
}

/*
    * nb:    我们的notifier_block
    * action: 事件类型（NETDEV_REGISTER等）
    * data: netdev_notifier_info结构
*/
static int32_t vlan_netdev_change_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
    struct net_device *vlan_dev = NULL;
    struct zxdh_en_device *en_dev = NULL;
    struct net_device *real_dev = NULL;
    struct vlan_dev_priv *vlan_priv = NULL; /* vlan子接口的私有结构体 */
    int32_t ret = 0;

    vlan_dev = netdev_notifier_info_to_dev(data); /* vlan子接口网络设备 */
    if (vlan_dev == NULL)
    {
        LOG_ERR("vlan_dev is NULL\n");
        return NOTIFY_BAD;
    }

    /* 检查是否为vlan设备 */
    if (!is_vlan_dev(vlan_dev))
        return NOTIFY_DONE;

    vlan_priv = vlan_dev_priv(vlan_dev); /* vlan子接口私有设备 */
    if (vlan_priv == NULL)
    {
        LOG_ERR("vlan_priv is NULL\n");
        return NOTIFY_BAD;
    }

    en_dev = container_of(nb, struct zxdh_en_device, vlan_notifier); /* 通知块所在的网络设备的私有结构体 */
    if (en_dev == NULL)
    {
        LOG_ERR("en_dev is NULL\n");
        return NOTIFY_BAD;
    }

    real_dev = vlan_dev_real_dev(vlan_dev);
    if (!real_dev)
    {
        LOG_ERR("%s can not get real_dev\n", vlan_dev->name);
        return NOTIFY_DONE;
    }

    /* 判断此次事件是否属于vlan的父设备*/
    if (real_dev != en_dev->netdev)
    {
        LOG_DEBUG("[%s] is not [%s] real_dev[%s]\n", en_dev->netdev->name, vlan_dev->name, real_dev->name);
        return NOTIFY_DONE;
    }

    LOG_DEBUG("vlan_netdev_change_notifier action %lu\n", action);
    LOG_DEBUG("real_dev is %s\n",en_dev->netdev->name);
    LOG_DEBUG("vlan_id: %d, real_dev_addr:%pM\n",vlan_priv->vlan_id, vlan_priv->real_dev_addr); /* 父设备的mac地址 */
    LOG_DEBUG("%s mac: %pM\n", vlan_dev->name, vlan_dev->dev_addr); /* vlan子接口的mac地址 */

    ret = do_pf_vf_vlan_update_mac_to_np(en_dev, vlan_dev, vlan_priv->vlan_id, vlan_dev->dev_addr, action);
    if (ret != 0)
    {
        LOG_ERR("do_pf_vf_vlan_update_mac_to_np failed\n");
        return NOTIFY_BAD;
    }
    return NOTIFY_OK;
}


static void vf_link_info_update_handler(struct work_struct *_work)
{
    struct zxdh_en_device *en_dev = container_of(_work, struct zxdh_en_device, vf_link_info_update_work);
    union zxdh_msg *msg = NULL;
    struct zxdh_vf_item *vf_item = NULL;
    int32_t err = 0;
    uint16_t vf_idx = 0;
    struct pci_dev *pdev = NULL;
    uint16_t num_vfs = 0;
    bool pf_link_up = false;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    ZXDH_AUX_INIT_COMP_CHECK(en_dev);
    pf_link_up = en_dev->ops->get_pf_link_up(en_dev->parent);
    pdev = en_dev->ops->get_pdev(en_dev->parent);
    num_vfs = pci_num_vf(pdev);

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return;
    }
    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        msg->payload.hdr_vf.op_code = ZXDH_SET_VF_LINK_STATE;
        msg->payload.link_state_msg.is_link_force_set = FALSE;
        msg->payload.link_state_msg.link_up = pf_link_up;
        msg->payload.link_state_msg.speed = en_dev->speed;
        msg->payload.link_state_msg.autoneg_enable = en_dev->autoneg_enable;
        msg->payload.link_state_msg.supported_speed_modes = en_dev->supported_speed_modes;
        msg->payload.link_state_msg.advertising_speed_modes = en_dev->advertising_speed_modes;
        msg->payload.hdr_vf.dst_pcie_id = FIND_VF_PCIE_ID(en_dev->pcie_id, vf_idx);
        vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_idx);
        if(vf_item->is_probed)
        {
            msg->payload.link_state_msg.link_forced = vf_item->link_forced;
            err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_PF_BAR_MSG_TO_VF, msg, msg, &para);
            if (err != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "failed to update VF[%d]\n", vf_idx);
            }
        }
    }
    kfree(msg);
}
//update_all：true 通知所有的VF，false 只通知vqm状态寄存器的值和link_info值不一样的VF
void update_vf_link_info(struct zxdh_en_device *en_dev, uint8_t link_info, bool update_all)
{
    struct zxdh_vf_item *vf_item = NULL;
    int32_t err = 0;
    uint16_t vf_idx = 0;
    struct pci_dev *pdev = NULL;
    uint16_t num_vfs = 0;
    uint16_t func_no = 0;
    uint16_t pf_no = FIND_PF_ID(en_dev->pcie_id);
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};
    bool is_same = true;

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    ZXDH_AUX_INIT_COMP_CHECK(en_dev);

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_DEV_STATUS_NOTIFY;
    msg->payload.hdr_to_agt.pcie_id = en_dev->pcie_id;

    pdev = en_dev->ops->get_pdev(en_dev->parent);
    num_vfs = pci_num_vf(pdev);
    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_idx);
        if(vf_item->link_forced == FALSE)
        {
            is_same = en_dev->ops->set_vf_link_info(en_dev->parent, vf_idx, link_info);
            if(vf_item->is_probed && (update_all || (!update_all && !is_same)))
            {
                func_no = GET_FUNC_NO(pf_no, vf_idx);
                msg->payload.pcie_msix_msg.func_no[msg->payload.pcie_msix_msg.num++] = func_no;
            }
        }
        LOG_INFO_DEV(en_dev->parent, "vf_idx:%d, vf_item->link_forced %d, is_probed %d, update_all %d, is_same %d\n",vf_idx, vf_item->link_forced, vf_item->is_probed, update_all, is_same);
    }
    if(msg->payload.pcie_msix_msg.num > 0)
    {
        LOG_INFO_DEV(en_dev->parent, "%s update %d vf link info\n", en_dev->netdev->name, msg->payload.pcie_msix_msg.num);
        err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "failed to update VF link info\n");
        }
    }

    kfree(msg);
}

static void link_info_irq_update_vf_handler(struct work_struct *_work)
{
    struct zxdh_en_device *en_dev = container_of(_work, struct zxdh_en_device, link_info_irq_update_vf_work);
    bool pf_link_up = en_dev->ops->get_pf_link_up(en_dev->parent);
    uint8_t link_info = 0;
    struct zxdh_lag_tracker *tracker = NULL;
    struct zxdh_lag_dev *ldev = NULL;

    ZXDH_AUX_INIT_COMP_CHECK(en_dev);
    if(en_dev->ops->is_upf(en_dev->parent))
    {
        link_info = (en_dev->phy_port & 0x0F) << 4 | (en_dev->link_up & 0x0F);
        LOG_DEBUG_DEV(en_dev->parent, "upf update vf link_info: %u\n", link_info);
    }
    else
    {
        link_info = pf_link_up ? 1 : 0;
    }

    // 判断是否处于硬bond场景
    ldev = en_dev->ldev;
    if (!ldev)
    {
        goto update_vf;
    }
    tracker = &ldev->tracker;
    if (!tracker)
    {
        goto update_vf;
    }

    mutex_lock(&ldev->mlock);
    if(ldev->state == LAG_DEV_ACTIVE && ldev->is_active && ldev->upper_netdev && tracker->bond_type == HARDWARE_BOND && (en_dev->panel_id == ldev->primary_pf_idx))
    {
        LOG_INFO_DEV(en_dev->parent, "primary port %s(link up %d) is hardware-bond mode, no need to update vf\n", netdev_name(en_dev->netdev), en_dev->link_up);
        mutex_unlock(&ldev->mlock);
        return;
    }
    mutex_unlock(&ldev->mlock);

update_vf:
    update_vf_link_info(en_dev, link_info, TRUE);
    return;
}

/* Started by AICoder, pid:j8e98a852f12dff14e2509c1108ec510c3f3fe88 */
bool is_hardware_bond(struct zxdh_en_device *en_dev)
{
    struct zxdh_lag_dev *ldev = NULL;
    struct zxdh_lag_tracker *tracker = NULL;

    ldev = en_dev->ldev;
    if (!ldev)
    {
        return false;
    }

    tracker = &ldev->tracker;
    if (!tracker)
    {
        return false;
    }

    mutex_lock(&ldev->mlock);
    if(ldev->state == LAG_DEV_ACTIVE && ldev->is_active && ldev->upper_netdev && tracker->bond_type == HARDWARE_BOND)
    {
        mutex_unlock(&ldev->mlock);
        return true;
    }
    mutex_unlock(&ldev->mlock);
    return false;
}
/* Ended by AICoder, pid:j8e98a852f12dff14e2509c1108ec510c3f3fe88 */

/* Started by AICoder, pid:7d423k4c47g704f14b910831b03b911d161227c8 */
void send_speed_change_event_to_rdma(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    ZXDH_AUX_INIT_COMP_CHECK(en_dev);

    if (is_hardware_bond(en_dev))
    {
        LOG_INFO_DEV(en_dev->parent, "%s(link up %d) is hardware-bond mode, no need to set RDMA speed\n", en_dev->netdev->name, en_dev->link_up);
        return;
    }

    if (!en_dev->ops->is_rdma_enable(en_dev->parent))
    {
        return;
    }

    ret = zxdh_rdma_events_call(en_dev->netdev, ZXDH_RDMA_SPEED_CHANGE_EVENT, &en_dev->speed);
    if (ret)
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to set RDMA speed: %d\n", ret);
    }
    return;
}
/* Ended by AICoder, pid:7d423k4c47g704f14b910831b03b911d161227c8 */

static void link_info_irq_process_handler(struct work_struct *_work)
{
    struct zxdh_en_device *en_dev = container_of(_work, struct zxdh_en_device, link_info_irq_process_work);
    int32_t ret = 0;
    struct link_info_struct link_info_val = {0};
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    ZXDH_AUX_INIT_COMP_CHECK(en_dev);

    if (!zxdh_en_is_panel_port(en_dev))
        return;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_MAC_LINK_INFO_GET;
    msg->payload.hdr_to_agt.phyport = en_dev->phy_port;
    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "get speed and duplex from agent failed: %d\n", ret);
        kfree(msg);
        return;
    }
    en_dev->speed = msg->reps.mac_set_msg.speed;
    en_dev->curr_speed_modes = msg->reps.mac_set_msg.speed_modes;
    en_dev->duplex = msg->reps.mac_set_msg.duplex;
    LOG_INFO_DEV(en_dev->parent, "netdev:%s, phy_port:0x%x, speed:%d, duplex:0x%x\n",
                en_dev->netdev->name, en_dev->phy_port, en_dev->speed, en_dev->duplex);

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        link_info_val.speed = en_dev->speed;
        link_info_val.autoneg_enable = en_dev->autoneg_enable;
        link_info_val.supported_speed_modes = en_dev->supported_speed_modes;
        link_info_val.advertising_speed_modes = en_dev->advertising_speed_modes;
        link_info_val.duplex = en_dev->duplex;

        en_dev->ops->update_pf_link_info(en_dev->parent, &link_info_val);

        send_speed_change_event_to_rdma(en_dev);
    }

    if (en_dev->speed != SPEED_UNKNOWN || en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
        netif_carrier_on(en_dev->netdev);

    kfree(msg);
    en_dev->ops->set_rdma_speed(en_dev->parent, en_dev->speed);

    return;
}

static void vf_ro_info_update_handler(struct work_struct *_work)
{
    struct zxdh_en_device *en_dev = container_of(_work, struct zxdh_en_device, vf_ro_info_update_work);
    int32_t err = 0;
    uint16_t vf_idx = 0;
    struct pci_dev *pdev = NULL;
    uint16_t num_vfs = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    ZXDH_AUX_INIT_COMP_CHECK(en_dev);

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return;
    }

    pdev = en_dev->ops->get_pdev(en_dev->parent);
    num_vfs = pci_num_vf(pdev);
    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        if (en_dev->ops->get_vf_is_probe(en_dev->parent, vf_idx))
        {
            msg->payload.hdr_vf.op_code = ZXDH_PF_UPDATE_VF_RO_FLAG;
            msg->payload.hdr_vf.dst_pcie_id = FIND_VF_PCIE_ID(en_dev->pcie_id, vf_idx);
            err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_PF_BAR_MSG_TO_VF, msg, msg, &para);
            if (err != 0)
            {
                if (err == ZXDH_INVALID_OP_CODE)
                {
                    LOG_INFO_DEV(en_dev->parent, "%s update VF %d ro_flag action is not supported, please update vf driver\n",
                                en_dev->netdev->name, vf_idx);
                }
                else
                {
                    LOG_ERR_DEV(en_dev->parent, "%s failed to update VF %d ro info, err %d\n", en_dev->netdev->name, vf_idx, err);
                }
            }
            else
            {
                LOG_INFO_DEV(en_dev->parent, "%s success to update VF %d ro info\n",en_dev->netdev->name, vf_idx);
            }
        }
    }

    kfree(msg);
}

/* Started by AICoder, pid:972dbm1963z5a26143740bed10b4803875b10597 */
int32_t zxdh_pf_set_vf_vlan_info(struct zxdh_en_device *en_dev, int32_t vf_idx,
                            uint16_t vlan_id, uint8_t qos, uint16_t vlan_proto)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    if (!en_dev->ops->get_vf_is_probe(en_dev->parent, vf_idx))
    {
        LOG_INFO("VF %d is not probed\n", vf_idx);
        return 0;
    }

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr_vf.op_code = ZXDH_PF_SET_VF_VLAN;
    msg->payload.hdr_vf.dst_pcie_id = FIND_VF_PCIE_ID(en_dev->pcie_id, vf_idx);
    msg->payload.vf_vlan_msg.vf_idx = vf_idx;
    msg->payload.vf_vlan_msg.vlan_id = vlan_id;
    msg->payload.vf_vlan_msg.qos = qos;
    msg->payload.vf_vlan_msg.protocol = htons(vlan_proto);
    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_PF_BAR_MSG_TO_VF, msg, msg, &para);
    if (ret) {
        LOG_ERR_DEV(en_dev->parent, "send set_vf_vlan msg failed: %d\n", ret);
    }

    kfree(msg);
    return ret;
}

/* Ended by AICoder, pid:972dbm1963z5a26143740bed10b4803875b10597 */

void update_link_down_event_for_hard_bond(struct zxdh_en_device *en_dev)
{
    struct zxdh_lag_dev *ldev = NULL;
    struct zxdh_lag_tracker *tracker = NULL;
    ZXDH_AUX_INIT_COMP_CHECK(en_dev);
    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF || en_dev->ops->is_bond(en_dev->parent) || en_dev->is_special_bond || !zxdh_en_is_panel_port(en_dev))
    {
        return;
    }
    // 判断是否处于硬bond场景
    ldev = en_dev->ldev;
    if (!ldev)
    {
        return;
    }
    tracker = &ldev->tracker;
    if (!tracker)
    {
        return;
    }
    if(ldev->state == LAG_DEV_ACTIVE && ldev->is_active && ldev->upper_netdev && tracker->bond_type == HARDWARE_BOND && en_dev->is_hwbond)
    {
        LOG_INFO_DEV(en_dev->parent, "%s(link up %d) is hardware-bond mode, update psn and bond_dpp_table\n", en_dev->netdev->name, en_dev->link_up);
        zxdh_update_hw_bond_panel_state(tracker, ldev, en_dev, true);
#ifndef CGS_V5_693
        zxdh_bond_set_dpp_member_port(en_dev, FALSE, NULL, ldev->group_ida);
#endif
    }
    return;
}

static void link_info_irq_update_np_work_handler(struct work_struct *_work)
{
    int32_t ret = 0;
    struct zxdh_en_device *en_dev = container_of(_work, struct zxdh_en_device, link_info_irq_update_np_work);
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    ZXDH_AUX_INIT_COMP_CHECK(en_dev);
    if (!en_dev->ops->is_bond(en_dev->parent))
    {
        if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
        {
            if (!en_dev->link_up)
            {
                update_link_down_event_for_hard_bond(en_dev);
            }
        }
        if (!netif_running(en_dev->netdev))
        {
            return;
        }
        if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
        {
            zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_IS_UP, en_dev->link_up, 0);
        }
        else
        {
            ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_IS_UP, en_dev->link_up);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set SRIOV_VPORT_IS_UP %d failed, ret:%d\n", en_dev->link_up, ret);
                return;
            }
            if (en_dev->is_hwbond || en_dev->ops->is_special_bond(en_dev->parent))
            {
                dpp_uplink_phy_attr_set(&pf_info, en_dev->phy_port, UPLINK_PHY_PORT_IS_UP, en_dev->link_up);
            }
        }
        return;
    }

    if (!en_dev->link_up)
    {
        zxdh_uplink_phy_attr_set(&pf_info, en_dev->phy_port, UPLINK_PHY_PORT_IS_UP, 0);
    }
    else
    {
        if (en_dev->netdev->flags & IFF_UP)
        {
            zxdh_uplink_phy_attr_set(&pf_info, en_dev->phy_port, UPLINK_PHY_PORT_IS_UP, 1);
        }
    }
}

static void en_aux_spoof_check(struct zxdh_en_device *en_dev)
{
    uint64_t prev_ssvpc_num = 0;
    uint16_t en_aux_pf_id = 0;
    uint32_t ret = 0;
    uint16_t num_vfs = 0;
    uint64_t ssvpc_incr = 0;
    struct pci_dev *pdev = NULL;
    struct dh_core_dev *dh_dev = NULL;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    dh_dev = en_dev->parent;
    pdev = en_dev->ops->get_pdev(dh_dev);
    num_vfs = pci_num_vf(pdev);

    if (!IS_PF(en_dev->vport))
    {
        return;
    }
    if (num_vfs == 0)
    {
        return;
    }
    prev_ssvpc_num = en_dev->last_tx_vport_ssvpc_packets;
    en_aux_pf_id = DH_AUX_PF_ID_OFFSET(en_dev->vport);
    // spoof static register not clear to 0 after read
    ret = dpp_stat_spoof_packet_drop_cnt_get(&pf_info, en_aux_pf_id, \
                                             NP_GET_PKT_CNT,\
                                             &(en_dev->last_tx_vport_ssvpc_packets));
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to get spoof check dropped packets number.\n");
        return;
    }
    ssvpc_incr = en_dev->last_tx_vport_ssvpc_packets - prev_ssvpc_num;
    if (!ssvpc_incr)
    {
        return;
    }
    LOG_DEBUG_DEV(en_dev->parent, "%llu Spoofed packets detected in EP%d, PF%d\n",
                ssvpc_incr, EPID(en_dev->vport), FUNC_NUM(en_dev->vport));
    return;
}

static void en_aux_service_task(struct work_struct *_work)
{
    struct zxdh_en_device *en_dev = container_of(_work, struct zxdh_en_device, service_task);

    ZXDH_AUX_INIT_COMP_CHECK(en_dev);
    en_aux_spoof_check(en_dev);
}

static bool en_aux_all_vfs_spoof_check_off(struct zxdh_en_device *en_dev)
{
    uint16_t vf_idx = 0;
    int32_t num_vfs = 0;
    struct pci_dev *pdev = NULL;
    struct zxdh_pf_device *pf_dev = NULL;
    struct dh_core_dev *dh_dev = NULL;
    dh_dev = en_dev->parent;
    pdev = en_dev->ops->get_pdev(dh_dev);
    pf_dev = dh_core_priv(dh_dev->parent);

    num_vfs = pci_num_vf(pdev);
    if (num_vfs == 0)
    {
        return true;
    }

    for (vf_idx = 0; vf_idx < num_vfs; vf_idx++)
    {
        if (pf_dev->vf_item[vf_idx].spoofchk == true)
        {
            return false;
        }
    }
    return true;
}

static void en_aux_service_timer(struct timer_list *t)
{
    unsigned long next_event_offset = HZ * 2;
    struct zxdh_en_device *en_dev = from_timer(en_dev, t, service_timer);
    struct zxdh_en_priv *en_priv = container_of(en_dev, struct zxdh_en_priv, edev);
    bool all_vfs_spoof_check_off_flag = en_aux_all_vfs_spoof_check_off(en_dev);

    /* Reset the timer */
    mod_timer(&en_dev->service_timer, next_event_offset + jiffies);
    if (!all_vfs_spoof_check_off_flag)
    {
        queue_work(en_priv->events->wq, &en_dev->service_task);
    }
}

static void en_aux_service_riscv_task(struct work_struct *_work)
{
    int32_t retval = 0;
    time64_t   time64;
    struct rtc_time tm;
    struct zxdh_en_device *en_dev = container_of(_work, struct zxdh_en_device, service_riscv_task);
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    ZXDH_AUX_INIT_COMP_CHECK(en_dev);

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return;
    }

    if (!IS_PF(en_dev->vport))
    {
        kfree(msg);
        return;
    }

    msg->payload.hdr_to_cmn.pcie_id =  en_dev->pcie_id;;
    msg->payload.hdr_to_cmn.write_bytes = 9;
    msg->payload.hdr_to_cmn.type = RISC_SERVER_TIME;
    msg->payload.hdr_to_cmn.field = 0;

#ifdef CGS_V5_693
    /* 3.10 内核不支持 ktime_get_real_seconds，使用 ktime_get_real_ns 转换 */
    time64 = ktime_get_real_ns() / NSEC_PER_SEC;
    /* 3.10 内核不支持 rtc_time64_to_tm，使用 rtc_time_to_tm */
    time64 += 28800;//CST比UST晚八个小时
    rtc_time_to_tm((time_t)time64, &tm);
#else
    time64 = ktime_get_real_seconds();
    time64 += 28800;//CST比UST晚八个小时
    rtc_time64_to_tm(time64, &tm);
#endif

    msg->payload.time_cfg_msg.tmmng_type = 0xF0;
    msg->payload.time_cfg_msg.dir = 0x2;
    msg->payload.time_cfg_msg.year = tm.tm_year + 1900;
    msg->payload.time_cfg_msg.month = tm.tm_mon + 1;
    msg->payload.time_cfg_msg.day = tm.tm_mday;
    msg->payload.time_cfg_msg.hour = tm.tm_hour;
    msg->payload.time_cfg_msg.min = tm.tm_min;
    msg->payload.time_cfg_msg.sec = tm.tm_sec;

    retval = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_PF_TIMER_TO_RISC_MSG, msg, msg, &para);
    if (retval != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_riscv failed: %d\n", retval);
        en_dev->time_sync_done = false;
    }
    else
    {
        LOG_DEBUG_DEV(en_dev->parent, "send msg timer:%d-%d-%d %d:%d:%d\n",
                    msg->payload.time_cfg_msg.year, msg->payload.time_cfg_msg.month,
                    msg->payload.time_cfg_msg.day, msg->payload.time_cfg_msg.hour,
                    msg->payload.time_cfg_msg.min, msg->payload.time_cfg_msg.sec);
        en_dev->time_sync_done = true;
    }

    kfree(msg);
}

static void en_aux_service_riscv_timer(struct timer_list *t)
{
    unsigned long next_event_offset;
    struct zxdh_en_device *en_dev = from_timer(en_dev, t, service_riscv_timer);
    struct zxdh_en_priv *en_priv = container_of(en_dev, struct zxdh_en_priv, edev);

    if (en_dev->time_sync_done)
    {
        next_event_offset = HZ * 259200; // 3天
    }
    else
    {
        next_event_offset = HZ * 60; // 60秒
    }

    /* Reset the timer */
    mod_timer(&en_dev->service_riscv_timer, next_event_offset + jiffies);
    queue_work(en_priv->events->wq, &en_dev->service_riscv_task);
}

static void pf2vf_msg_proc_work_handler(struct work_struct *_work)
{
    struct zxdh_en_device *en_dev = container_of(_work, struct zxdh_en_device, pf2vf_msg_proc_work);
    uint64_t virt_addr = 0;

    LOG_DEBUG_DEV(en_dev->parent, "is called\n");
    ZXDH_AUX_INIT_COMP_CHECK(en_dev);
    virt_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0) + ZXDH_BAR_MSG_OFFSET + ZXDH_BAR_PFVF_MSG_OFFSET;
    zxdh_bar_irq_recv(MSG_CHAN_END_PF, MSG_CHAN_END_VF, virt_addr, en_dev);
}

static int32_t pf2vf_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)event_nb->ctx;

    LOG_DEBUG_DEV(en_priv->edev.parent, "is called\n");
    queue_work(en_priv->events->wq, &en_priv->edev.pf2vf_msg_proc_work);

    return NOTIFY_OK;
}

static void riscv2aux_msg_proc_work_handler(struct work_struct *_work)
{
    struct zxdh_en_device *en_dev = container_of(_work, struct zxdh_en_device, riscv2aux_msg_proc_work);
    uint64_t virt_addr = 0;
    uint16_t src = MSG_CHAN_END_RISC;
    uint16_t dst = MSG_CHAN_END_PF;

    ZXDH_AUX_INIT_COMP_CHECK(en_dev);
    virt_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0) + ZXDH_BAR_MSG_OFFSET;
    zxdh_bar_irq_recv(src, dst, virt_addr, en_dev);
}

static int32_t riscv2aux_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)event_nb->ctx;

    LOG_DEBUG_DEV(en_priv->edev.parent, "is called\n");
    queue_work(en_priv->events->wq, &en_priv->edev.riscv2aux_msg_proc_work);
    return NOTIFY_OK;
}

typedef int32_t (*zxdh_rdma_event_handler)(struct net_device *netdev, uint8_t event_type, void *data);
static zxdh_rdma_event_handler zxdh_rdma_events_handler;
void zxdh_rdma_events_register(zxdh_rdma_event_handler callback)
{
    if (zxdh_rdma_events_handler == NULL)
        zxdh_rdma_events_handler = callback;
}
EXPORT_SYMBOL(zxdh_rdma_events_register);

void zxdh_rdma_events_unregister(void)
{
    zxdh_rdma_events_handler = NULL;
}
EXPORT_SYMBOL(zxdh_rdma_events_unregister);

int32_t zxdh_rdma_events_call(struct net_device *netdev, uint8_t event_type, void *data)
{
    if (zxdh_rdma_events_handler)
        return zxdh_rdma_events_handler(netdev, event_type, data);

    return 0;
}
EXPORT_SYMBOL(zxdh_rdma_events_call);

static int32_t aux_unload_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)event_nb->ctx;
    struct zxdh_en_device *en_dev = &en_priv->edev;

    HEAL_INFO_DEV(en_dev->parent, "%s %s is called\n", en_dev->netdev->name, __func__);
    zxdh_rdma_events_call(en_dev->netdev, ZXDH_RDMA_HEALTH_EVENT, NULL);
    zxdh_aux_unload(en_priv);
    return NOTIFY_OK;
}

static int32_t aux_load_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)event_nb->ctx;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t err = 0;

    HEAL_INFO_DEV(en_dev->parent, "%s %s is called\n", en_dev->netdev->name, __func__);
    err = zxdh_aux_load(en_priv);
    *((int32_t *)data) = err;

    return NOTIFY_OK;
}

static int32_t rdma_load_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)event_nb->ctx;
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (en_dev == NULL) {
        HEAL_ERR("en_dev is NULL\n");
        return NOTIFY_OK;
    }

    HEAL_INFO_DEV(en_dev->parent, "%s %s is called\n", en_dev->netdev->name, __func__);
    if (en_dev->is_rdma_aux_plug) {
        mutex_lock(&rdma_lock);
        if (en_dev == NULL) {
            HEAL_ERR("en_dev is NULL\n");
            mutex_unlock(&rdma_lock);
            return NOTIFY_OK;
        }
        if (en_dev->parent == NULL) {
            HEAL_ERR("en_dev->parent is NULL\n");
            mutex_unlock(&rdma_lock);
            return NOTIFY_OK;
        }
        en_dev->ops->unplug_adev(en_dev->parent, RDMA_AUX_DEVICE);
        en_dev->ops->plug_adev(en_dev->parent, RDMA_AUX_DEVICE);
        mutex_unlock(&rdma_lock);
    }

    return NOTIFY_OK;
}

static int32_t rdma_event_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)event_nb->ctx;
    struct zxdh_en_device *en_dev;

    if (en_priv == NULL) {
        HEAL_ERR("en_priv is NULL\n");
        return NOTIFY_OK;
    }
    en_dev = &en_priv->edev;

    if (en_dev->netdev == NULL) {
        HEAL_ERR_DEV(en_dev->parent, "%s netdev is NULL\n", __func__);
        return NOTIFY_OK;
    }

    HEAL_INFO_DEV(en_dev->parent, "%s %s is called\n", en_dev->netdev->name, __func__);
    zxdh_rdma_events_call(en_dev->netdev, ZXDH_RDMA_HEALTH_EVENT, NULL);

    return NOTIFY_OK;
}

static int32_t aux_state_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)event_nb->ctx;
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (en_dev->device_state == *((uint8_t *)data))
        return NOTIFY_OK;

    HEAL_INFO_DEV(en_dev->parent, "%s device_state update: %d\n", en_dev->netdev->name, *((uint8_t *)data));
    en_dev->device_state = *((uint8_t *)data);

    if (en_dev->device_state == ZXDH_DEVICE_STATE_RELOAD) {
        mutex_lock(&en_dev->parent->lock);
        zxdh_cfg_reload(en_dev);
        en_dev->device_state = ZXDH_DEVICE_STATE_UP;
        mutex_unlock(&en_dev->parent->lock);
    }

    if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR) {
        netif_tx_stop_all_queues(en_dev->netdev);
        netif_carrier_off(en_dev->netdev);
        en_dev->link_up = false;
    } else if (en_dev->device_state == ZXDH_DEVICE_STATE_UP) {
        netif_tx_wake_all_queues(en_dev->netdev);
        if (en_dev->ops->is_bond(en_dev->parent))
            dh_bond_pf_link_info_get(en_priv);
        else
            dh_eq_async_link_info_int_process(en_priv);
    }

    return NOTIFY_OK;
}

/* Started by AICoder, pid:i80e88097d0c43f14de0097bb063233ec9d70326 */
static void zxdh_aux_dev_info_show(struct zxdh_en_device *en_dev)
{
    struct dh_core_dev *dh_dev = en_dev->parent;
    int vq_cnt = en_dev->max_queue_pairs * 2;
    uint8_t i = 0;

    LOG_INFO_DEV(dh_dev, "***************** %s basic aux device info *****************\n", pci_name(dh_dev->pdev));
    LOG_INFO_DEV(dh_dev, "** name: %s\n", en_dev->netdev->name);
    LOG_INFO_DEV(dh_dev, "** pannel_id: %hu\n", en_dev->pannel_id);
    LOG_INFO_DEV(dh_dev, "** vport: 0x%hx\n", en_dev->vport);
    LOG_INFO_DEV(dh_dev, "** pcie_id: 0x%x\n", en_dev->pcie_id);
    LOG_INFO_DEV(dh_dev, "** slot_id: 0x%x\n", en_dev->slot_id);
    LOG_INFO_DEV(dh_dev, "** phy_port: %d\n", en_dev->phy_port);
    LOG_INFO_DEV(dh_dev, "** ep_bdf: 0x%x\n", en_dev->ep_bdf);
    LOG_INFO_DEV(dh_dev, "** ro_flag: 0x%d\n", en_dev->ro_flag);
    LOG_INFO_DEV(dh_dev, "** is_bond: %s\n", en_dev->ops->is_bond(en_dev->parent) ? "true" : "false");
    LOG_INFO_DEV(dh_dev, "** vepa_en_off: %s\n", en_dev->ops->get_vepa(en_dev->parent) ? "vepa" : "veb");
    LOG_INFO_DEV(dh_dev, "** multicast_num: %d\n", en_dev->curr_multicast_num);
    LOG_INFO_DEV(dh_dev, "** packed_status: %d\n", en_dev->packed_status);
    LOG_INFO_DEV(dh_dev, "** device_feature: 0x%llx\n", en_dev->device_feature);
    LOG_INFO_DEV(dh_dev, "** guest_feature: 0x%llx\n", en_dev->guest_feature);
    if (!en_dev->packed_status) {
        LOG_INFO_DEV(dh_dev, "** logical_index_split: ");
        for (i = 0; i < vq_cnt; i++)
        {
            printk(KERN_CONT "%u ", en_dev->logic_index_split[i]);
        }
        printk(KERN_CONT "\n");
    }
    LOG_INFO_DEV(dh_dev, "** phy_index: ");
    for (i = 0; i < vq_cnt; i++)
    {
        printk(KERN_CONT "%u ", en_dev->phy_index[i]);
    }
    printk(KERN_CONT "\n");
    LOG_INFO_DEV(dh_dev, "*****************eth_config******************\n");
    LOG_INFO_DEV(dh_dev, "** rx_queue_size: %d\n", en_dev->eth_config.rx_queue_size);
    LOG_INFO_DEV(dh_dev, "** tx_queue_size: %d\n", en_dev->eth_config.tx_queue_size);
    LOG_INFO_DEV(dh_dev, "** num_rxq: %d\n", en_dev->eth_config.num_rxq);
    LOG_INFO_DEV(dh_dev, "** num_txq: %d\n", en_dev->eth_config.num_txq);
    LOG_INFO_DEV(dh_dev, "****************************************************\n");
}
/* Ended by AICoder, pid:i80e88097d0c43f14de0097bb063233ec9d70326 */

/* Started by AICoder, pid:w8b47l500cee29914c8e08a900524d1742f1506a */
static int32_t aux_info_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)event_nb->ctx;
    struct zxdh_en_device *en_dev = &en_priv->edev;

    zxdh_aux_dev_info_show(en_dev);

    return NOTIFY_OK;
}
/* Ended by AICoder, pid:w8b47l500cee29914c8e08a900524d1742f1506a */

void plug_adev_work_handler(struct work_struct *work)
{
    struct zxdh_en_device *en_dev = container_of(work, struct zxdh_en_device, plug_adev_work);

    en_dev->ops->plug_adev(en_dev->parent, RDMA_AUX_DEVICE);
    en_dev->is_rdma_aux_plug = true;
    en_dev->ops->is_rdma_aux_plug(en_dev->parent, en_dev->is_rdma_aux_plug, TRUE);
}

void unplug_adev_work_handler(struct work_struct *work)
{
    struct zxdh_en_device *en_dev = container_of(work, struct zxdh_en_device, unplug_adev_work);

    en_dev->ops->unplug_adev(en_dev->parent, RDMA_AUX_DEVICE);
    en_dev->is_rdma_aux_plug = false;
    en_dev->ops->is_rdma_aux_plug(en_dev->parent, en_dev->is_rdma_aux_plug, TRUE);
}

typedef uint32_t (*zxdh_pf_msg_func)(zxdh_msg_info *msg, zxdh_reps_info *reps, struct zxdh_en_device *en_dev);

typedef struct
{
    zxdh_msg_op_code op_code;
    uint8_t proc_name[64];
    zxdh_pf_msg_func msg_proc;
} zxdh_pf_msg_proc;

static uint32_t zxdh_set_vf_link_state(zxdh_msg_info *msg, zxdh_reps_info *reps, struct zxdh_en_device *en_dev)
{
    uint32_t ret = 0;
    uint16_t vf_idx = msg->hdr_vf.dst_pcie_id & (0xff);

    if(!msg->link_state_msg.is_link_force_set)
    {
        en_dev->speed = msg->link_state_msg.speed;
        en_dev->autoneg_enable = msg->link_state_msg.autoneg_enable;
        en_dev->supported_speed_modes = msg->link_state_msg.supported_speed_modes;
        en_dev->advertising_speed_modes = msg->link_state_msg.advertising_speed_modes;
        if(msg->link_state_msg.link_forced)
        {
            return 0;
        }
    }

    en_dev->ops->set_pf_link_up(en_dev->parent, msg->link_state_msg.link_up);
    if(en_dev->ops->get_pf_link_up(en_dev->parent))
    {
        netif_carrier_on(en_dev->netdev);
    }
    else
    {
        netif_carrier_off(en_dev->netdev);
    }
    LOG_DEBUG_DEV(en_dev->parent, "[VF GET MSG FROM PF]--VF[%d] link_state[%s] update success!\n",
                vf_idx, en_dev->ops->get_pf_link_up(en_dev->parent)?"TRUE":"FALSE");
    return ret;
}

static uint32_t zxdh_set_vf_reset(zxdh_msg_info *msg, zxdh_reps_info *reps, struct zxdh_en_device *en_dev)
{
    return 0;
}

/* Started by AICoder, pid:81ef9ha898l17cd140830a8bc088d622fd8167af */
static uint32_t zxdh_set_vf_vlan(zxdh_msg_info *msg, zxdh_reps_info *reps, struct zxdh_en_device *edev)
{
    struct zxdh_rdma_vlan_event_info vlan_info;

    /* update local var*/
    edev->vlan_dev.vlan_id = msg->vf_vlan_msg.vlan_id;
    edev->vlan_dev.qos = msg->vf_vlan_msg.qos;
    edev->vlan_dev.protocol = msg->vf_vlan_msg.protocol;
    LOG_INFO_DEV(edev->parent, "zxdh_set_vf_vlan: vf_idx=%d, vlan_id=%d, qos=%d, protcol=0x%x\n",
                    msg->vf_vlan_msg.vf_idx, edev->vlan_dev.vlan_id, edev->vlan_dev.qos, edev->vlan_dev.protocol);

    vlan_info.vf_idx = msg->vf_vlan_msg.vf_idx;
    vlan_info.vlan_id = msg->vf_vlan_msg.vlan_id;
    vlan_info.qos = msg->vf_vlan_msg.qos;
    vlan_info.protocol = msg->vf_vlan_msg.protocol;

    zxdh_rdma_events_call(edev->netdev, ZXDH_RDMA_VLAN_EVENT, &vlan_info);

    return 0;
}
/* Ended by AICoder, pid:81ef9ha898l17cd140830a8bc088d622fd8167af */

static uint32_t zxdh_pf_update_vf_ro_info(zxdh_msg_info *msg, zxdh_reps_info *reps, struct zxdh_en_device *edev)
{
    uint32_t ret = 0;
    // 1、前往fwshrd读取RO状态, 更新延时标志位
    edev->ro_flag = edev->ops->get_ro_info_from_fwshrd(edev->parent);
    LOG_INFO_DEV(edev->parent, "netdev %s update ro_flag %d\n", edev->netdev->name, edev->ro_flag);

    return ret;
}

static uint32_t zxdh_pf_get_vf_queue(zxdh_msg_info *msg, zxdh_reps_info *reps, struct zxdh_en_device *edev)
{
    uint32_t ret = 0;
    uint32_t vir_queue_start;
    uint32_t vir_queue_num;
    uint32_t queue_index;
    uint32_t queue_num;
    uint32_t max_queue_num = edev->curr_queue_pairs;

    PLCR_LOG_INFO_DEV(edev->parent, "vf's edev->vport     = 0x%x\n", edev->vport);
    PLCR_LOG_INFO_DEV(edev->parent, "vf's max_queue_num(pairs) = 0x%x\n", max_queue_num);
    PLCR_LOG_INFO_DEV(edev->parent, "edev->device_id = %x\n", edev->device_id);
    PLCR_LOG_INFO_DEV(edev->parent, "edev->rq[0].vq->phy_index = %x\n", edev->rq[0].vq->phy_index);
    PLCR_LOG_INFO_DEV(edev->parent, "edev->sq[0].vq->phy_index = %x\n", edev->sq[0].vq->phy_index);

    vir_queue_start = msg->plcr_pf_get_vf_queue_info_msg.vir_queue_start;
    vir_queue_num   = msg->plcr_pf_get_vf_queue_info_msg.vir_queue_num;

    PLCR_LOG_INFO_DEV(edev->parent, "vir_queue_start = 0x%x\n", vir_queue_start);
    PLCR_LOG_INFO_DEV(edev->parent, "vir_queue_num   = 0x%x\n", vir_queue_num);

    if(max_queue_num > (vir_queue_num + vir_queue_num))
    {
        max_queue_num = vir_queue_num + vir_queue_num;
    }

    for(queue_index=vir_queue_start, queue_num = 0; queue_index<max_queue_num; queue_index++, queue_num++)
    {
        //get rx&tx queue info
        reps->plcr_pf_get_vf_queue_info_rsp.phy_rxq[queue_num] = edev->rq[queue_num].vq->phy_index;
        reps->plcr_pf_get_vf_queue_info_rsp.phy_txq[queue_num] = edev->sq[queue_num].vq->phy_index;
    }

    reps->plcr_pf_get_vf_queue_info_rsp.phy_queue_num = queue_num;

    PLCR_LOG_INFO_DEV(edev->parent, "queue_num   = 0x%x\n", queue_num);

    return ret;
}

static uint32_t zxdh_vf_queue_info_show(zxdh_msg_info *msg, zxdh_reps_info *reps, struct zxdh_en_device *edev)
{
    uint32_t ret = 0;
    struct zxdh_en_reg *reg = NULL;
    uint32_t size = sizeof(struct zxdh_en_reg);

    reg = kzalloc(size, GFP_KERNEL);
    CHECK_EQUAL_ERR(reg, NULL, -EADDRNOTAVAIL, "reg is null!\n");

    reg->offset = msg->vf_queue_info_msg.queue_idx;
    reg->num = msg->vf_queue_info_msg.desc_start;
    reg->data[0] = msg->vf_queue_info_msg.desc_num;
    reg->data[2] = msg->vf_queue_info_msg.vf_idx;

    ret = zxdh_queue_info_common_print(edev, reg);
    if (ret != 0)
    {
        LOG_ERR_DEV(edev->parent, "zxdh_queue_info_common_print failed\n");
        ret = 1;
        goto err_reg;
    }

err_reg:
    kfree(reg);
    return ret;
}

zxdh_pf_msg_proc pf_msg_proc[] =
{
    {ZXDH_SET_VF_LINK_STATE,      "set_vf_link_state",         zxdh_set_vf_link_state},
    {ZXDH_SET_VF_RESET,           "set_vf_reset",              zxdh_set_vf_reset},
    {ZXDH_PF_SET_VF_VLAN,         "pf_set_vf_vlan",            zxdh_set_vf_vlan},
    {ZXDH_PF_GET_VF_QUEUE_INFO,   "pf_get_vf_queue_info",      zxdh_pf_get_vf_queue},
    {ZXDH_PF_UPDATE_VF_RO_FLAG,   "pf_update_vf_ro_flag",      zxdh_pf_update_vf_ro_info},
    {ZXDH_VF_QUEUE_INFO_SHOW,     "vf_queue_info_show",        zxdh_vf_queue_info_show}
};

int32_t zxdh_vf_msg_recv_func(void *pay_load, uint16_t len, void *reps_buffer, uint16_t *reps_len, void *dev)
{
    zxdh_msg_info *msg = (zxdh_msg_info *)pay_load;
    zxdh_reps_info *reps = (zxdh_reps_info *)reps_buffer;
    struct zxdh_en_device *en_dev = (struct zxdh_en_device *)dev;
    int32_t ret = 0;
    int32_t i = 0;
    int32_t num = 0;

    LOG_DEBUG("is called\n");
    if (len != sizeof(union zxdh_msg))
    {
        LOG_ERR("invalid data_len\n");
        return -1;
    }

    if (en_dev == NULL)
    {
        LOG_ERR("dev is NULL\n");
        return -1;
    }

    num = sizeof(pf_msg_proc)/sizeof(zxdh_pf_msg_proc);

    for (i = 0; i < num; i++)
    {
        *reps_len = sizeof(union zxdh_msg);
        if (pf_msg_proc[i].op_code == msg->hdr_vf.op_code)
        {
            LOG_DEBUG_DEV(en_dev->parent, "%s is called", pf_msg_proc[i].proc_name);
            ret = pf_msg_proc[i].msg_proc(msg, reps, en_dev);
            if (ret != 0)
            {
                reps->flag = ZXDH_REPS_FAIL;
                LOG_ERR_DEV(en_dev->parent, "%s failed, ret: %d\n", pf_msg_proc[i].proc_name, ret);
                return -1;
            }
            reps->flag = ZXDH_REPS_SUCC;
            return 0;
        }
    }

    LOG_ERR_DEV(en_dev->parent, "invalid op_code: [%u]\n", msg->hdr_vf.op_code);
    reps->flag = ZXDH_INVALID_OP_CODE;
    return -2;
}

int32_t dh_ip_mac_init(struct zxdh_en_priv *en_priv)
{
    int32_t err = 0;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    DPP_PF_INFO_T pf_info = {0};
    uint8_t ip4_mac[6] = {0x01, 0x00, 0x5e, 0x00, 0x00, 0x01};
    uint8_t ip6_mac[6] = {0x33, 0x33, 0x00, 0x00, 0x00, 0x01};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* 判断目前所配置组播mac地址数量是否超过上限 */
    if (en_dev->curr_multicast_num >= DEV_MULTICAST_MAX_NUM)
    {
        LOG_ERR_DEV(en_dev->parent, "curr_multicast_num is beyond maximum\n");
        return -ENOSPC;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    { /* PF流程 */
        err = dpp_multi_mac_add_member(&pf_info, ip4_mac);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_multi_mac_add_member mac:%pM failed, err:%d\n", ip4_mac, err);
            return err;
        }
        en_dev->curr_multicast_num++;
        err = dpp_multi_mac_add_member(&pf_info, ip6_mac);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_multi_mac_add_member mac:%pM failed, err:%d\n", ip6_mac, err);
            return err;
        }
        en_dev->curr_multicast_num++;
        LOG_DEBUG_DEV(en_dev->parent, "current multicast num: %d", en_dev->curr_multicast_num);
    }
    else
    { /* VF流程*/
        err = zxdh_vf_dpp_add_ipv6_mac(en_dev, ip4_mac);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_ip_mac_init mac:%pM failed, err:%d\n", ip4_mac, err);
            return err;
        }
        en_dev->curr_multicast_num++;

        err = zxdh_vf_dpp_add_ipv6_mac(en_dev, ip6_mac);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_ip_mac_init mac:%pM failed, err:%d\n", ip6_mac, err);
            return err;
        }
        en_dev->curr_multicast_num++;
        LOG_DEBUG_DEV(en_dev->parent, "current multicast num is %d", en_dev->curr_multicast_num);
    }

    LOG_DEBUG_DEV(en_dev->parent, "config exist mac to np\n");
    return 0;
}

int32_t dh_aux_ipv6_notifier_init(struct zxdh_en_priv *en_priv)
{
    int32_t ret = 0;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    en_dev->ipv6_notifier.notifier_call = inet6_addr_change_notifier;
    en_dev->ipv6_notifier.priority = 0;
    ret = dh_inet6_addr_change_notifier_register(&(en_dev->ipv6_notifier));
    if (ret)
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to register inet6addr_notifier, ret:%d\n",ret);
        return ret;
    }
    return ret;
}

int32_t dh_aux_vxlan_netdev_notifier_init(struct zxdh_en_priv *en_priv)
{
    int32_t ret = 0;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    en_dev->vxlan_notifier.notifier_call = vxlan_netdev_change_notifier;
    en_dev->vxlan_notifier.priority = 0;
    ret = dh_vxlan_netdev_change_notifier_register(&(en_dev->vxlan_notifier));
    if (ret)
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to register vxlan_notifier, ret:%d\n",ret);
        return ret;
    }
    LOG_DEBUG_DEV(en_dev->parent, "netdev:%s vxlan_notifier_init success\n", en_dev->netdev->name);
    return ret;
}

/* 初始化vlan_mac映射表 */
void vlan_mac_list_init(struct zxdh_vlan_to_mac_list *list)
{
    INIT_LIST_HEAD(&list->list);
    spin_lock_init(&list->lock);
}

void vlan_mac_list_uninit( struct zxdh_vlan_to_mac_list *list)
{
    struct zxdh_vlan_to_mac *vtm = NULL;
    struct zxdh_vlan_to_mac *tmp = NULL;

    spin_lock(&list->lock);
    list_for_each_entry_safe(vtm, tmp, &list->list, list)
    {
        list_del_rcu(&vtm->list);
        kfree_rcu(vtm, rcu_head);
    }
    list->count = 0;
    spin_unlock(&list->lock);
    return;
}

int32_t dh_aux_vlan_netdev_notifier_init(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;

    en_dev->vlan_notifier.notifier_call = vlan_netdev_change_notifier;
    en_dev->vlan_notifier.priority = 0;

    /* 初始化vlan-mac链表*/
    vlan_mac_list_init(&en_dev->vuc);

    /* 注册Vlan子接口回调函数  */
    ret = dh_vlan_netdev_change_notifier_register(&(en_dev->vlan_notifier));
    if (ret)
    {
        LOG_ERR("Failed to register vlan_notifier, ret:%d\n",ret);
        return ret;
    }

    return ret;
}

int32_t dh_aux_vlan_netdev_notifier_uninit(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;

    /* 销毁vlan-mac链表*/
    vlan_mac_list_uninit(&en_dev->vuc);

    /* 注销Vlan子接口回调函数  */
    ret = dh_vlan_netdev_change_notifier_unregister(&(en_dev->vlan_notifier));
    if (ret)
    {
        LOG_ERR("Failed to register vlan_notifier, ret:%d\n",ret);
        return ret;
    }

    return ret;
}

static void run_cfg_shell_script(struct work_struct *work)
{
    static const char command[] = "/etc/zxdh_cfg/smart_nic_cfg_proc.sh";
    char *argv[] = {(char *)command, "c", NULL};
    static char *envp[] = {"HOME=/",
                        "TERM=linux",
                        "PATH=/bin:/sbin:/usr/bin:/usr/sbin:/bin",
                        NULL};
    int32_t ret = 0;

#ifdef CGS_V5_693
    /* 3.10 内核 call_usermodehelper 第一个参数不是 const，需要类型转换 */
    ret = call_usermodehelper((char *)command, argv, envp, UMH_WAIT_PROC);
#else
    ret = call_usermodehelper(command, argv, envp, UMH_WAIT_PROC);
#endif
    if (ret < 0)
    {
        LOG_DEBUG("Failed to execute shell script(err:%d)\n", ret);
    }
    else
    {
        LOG_DEBUG("Shell script executed successfully,ret:%d\n", ret);
    }
}

void zxdh_cap_pkt_uninit(struct zxdh_en_device *en_dev, bool offload_mode)
{
    uint32_t ret = 0;
    uint32_t i = 0;
    uint32_t buf_num = ZXDH_MQ_PAIRS_NUM * ZXDH_PF_MAX_DESC_NUM(en_dev);
    DPP_PF_INFO_T pf_info = {0};

    if (en_dev->pkt_dev_flag == 1)
    {
        pf_info.slot = en_dev->slot_id;
        pf_info.vport = en_dev->vport;
        if (en_dev->pkt_wq)
        {
            destroy_workqueue(en_dev->pkt_wq);
            en_dev->pkt_wq = NULL;
        }

        if (offload_mode)
        {
            ret = dpp_pkt_capture_disable_all(&pf_info);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "dpp_pkt_capture_disable_all failed, ret:%d!!!\n", ret);
            }

            ret = dpp_pkt_capture_table_flush(&pf_info);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "dpp_pkt_capture_table_flush failed, ret:%d!!!\n", ret);
            }

            ret = dpp_pkt_capture_speed_set(&pf_info, ZXDH_PKT_INIT_SPEED);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "dpp_pkt_capture_speed_set failed, ret:%d!!!\n", ret);
            }

            en_dev->pkt_dev_flag = 0;
            en_dev->pkt_cap_switch = 1;
        }

        en_dev->pkt_save_file_flag = 0;
        en_dev->pkt_file_num = 0;
        en_dev->pkt_save_file.enable_pkt_num_mode = 0;
        en_dev->pkt_save_file.pkt_file_size = 0;
        en_dev->pkt_save_file.pkt_set_count = 0;
        en_dev->pkt_save_file.pkt_cur_num = 0;
        en_dev->pkt_addr_marked = 0;
        en_dev->pkt_dev_speed = ZXDH_PKT_INIT_SPEED;
        en_dev->pkt_save_file.file_pos = 0;

        for (i=0; i< buf_num; i++)
        {
            if (en_dev->pkt_file_info && en_dev->pkt_file_info[i].pkt_addr_array)
            {
                SAFE_KFREE(en_dev->pkt_file_info[i].pkt_addr_array);
            }
        }

        if (en_dev->pkt_file_info)
        {
            kfree(en_dev->pkt_file_info);
            en_dev->pkt_file_info = NULL;
        }

        if (en_dev->pkt_save_file.log_file != NULL)
        {
            close_log_file(en_dev->pkt_save_file.log_file);
            en_dev->pkt_save_file.log_file = NULL;
        }

        en_dev->pkt_save_file.pkt_ubuf_idx = 0;
        en_dev->pkt_save_file.pkt_rbuf_idx = 0;
    }
}

int32_t dh_aux_events_init(struct zxdh_en_priv *en_priv)
{
    struct dh_events *events = NULL;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t i = 0;
    int32_t ret = 0;
    uint32_t evt_num = ARRAY_SIZE(aux_events);

    if (!en_dev->ops->if_init(en_dev->parent))
        evt_num -= 1;//TODO

    events = kzalloc((sizeof(*events) + evt_num * sizeof(struct dh_event_nb)), GFP_KERNEL);
    if (unlikely(events == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "events kzalloc failed: %p\n", events);
        ret = -ENOMEM;
        goto err_events_kzalloc;
    }

    events->evt_num = evt_num;
    events->dev = NULL;
    en_priv->events = events;
    events->wq = create_singlethread_workqueue("dh_aux_events");
    if (!events->wq)
    {
        LOG_ERR_DEV(en_dev->parent, "events->wq create_singlethread_workqueue failed: %p\n", events->wq);
        ret = -ENOMEM;
        goto err_create_wq;
    }

    INIT_WORK(&en_dev->vf_link_info_update_work, vf_link_info_update_handler);
    INIT_WORK(&en_dev->link_info_irq_update_vf_work, link_info_irq_update_vf_handler);
    INIT_WORK(&en_dev->link_info_irq_process_work, link_info_irq_process_handler);
    INIT_WORK(&en_dev->link_info_irq_update_np_work, link_info_irq_update_np_work_handler);
    INIT_WORK(&en_dev->rx_mode_set_work, rx_mode_set_handler);
    INIT_WORK(&en_dev->pf2vf_msg_proc_work, pf2vf_msg_proc_work_handler);
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        INIT_WORK(&en_dev->service_task, en_aux_service_task);
        INIT_WORK(&en_dev->vf_ro_info_update_work, vf_ro_info_update_handler);
        INIT_WORK(&en_dev->service_riscv_task, en_aux_service_riscv_task);
    }

    INIT_WORK(&en_dev->riscv2aux_msg_proc_work, riscv2aux_msg_proc_work_handler);
    INIT_WORK(&en_dev->plug_adev_work, plug_adev_work_handler);
    INIT_WORK(&en_dev->unplug_adev_work, unplug_adev_work_handler);

    INIT_WORK(&en_dev->smart_nic_copy_work, run_cfg_shell_script);
    queue_work(events->wq, &en_dev->smart_nic_copy_work);

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        timer_setup(&en_dev->service_timer, en_aux_service_timer, 0);
        ret = mod_timer(&en_dev->service_timer, jiffies);
        if (ret)
        {
            LOG_ERR_DEV(en_dev->parent, "timer add failed\n");
            goto err_mod_timer;
        }

        timer_setup(&en_dev->service_riscv_timer, en_aux_service_riscv_timer, 0);
        ret = mod_timer(&en_dev->service_riscv_timer, jiffies);
        if (ret)
        {
            LOG_ERR_DEV(en_dev->parent, "timer add failed\n");
            goto err_riscv_timer;
        }
    }

    for (i = 0; i < evt_num; i++)
    {
        events->notifiers[i].nb = aux_events[i];
        events->notifiers[i].ctx = en_priv;
        en_dev->ops->aux_nh_attach(en_dev->parent, &events->notifiers[i].nb, true);
    }

    return ret;

err_riscv_timer:
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        del_timer_sync(&en_dev->service_riscv_timer);
    }
err_mod_timer:
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        del_timer_sync(&en_dev->service_timer);
    }
    destroy_workqueue(events->wq);
err_create_wq:
    kfree(events);
err_events_kzalloc:
    return ret;
}

void dh_aux_events_uninit(struct zxdh_en_priv *en_priv)
{
    struct dh_events *events = en_priv->events;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t i = 0;

    for (i = events->evt_num - 1; i >= 0 ; i--)
    {
        // dh_eq_notifier_unregister(&en_priv->eq_table, &events->notifiers[i].nb);
        en_dev->ops->aux_nh_attach(en_dev->parent, &events->notifiers[i].nb, false);
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        del_timer_sync(&en_dev->service_timer);
        del_timer_sync(&en_dev->service_riscv_timer);
        zxdh_cap_pkt_uninit(en_dev, true);
    }

    destroy_workqueue(en_priv->events->wq);
    kfree(en_priv->events);

    return;
}

static int32_t mgr_test_cnt(void *data, uint16_t len, void *reps, uint16_t *reps_len, void *dev)
{
    uint8_t *pay_load = (uint8_t *)data;
    uint8_t *reps_buffer = (uint8_t *)reps;
    uint16_t idx = 0;
    uint16_t sum = 0;

    if (reps_buffer == NULL)
    {
        return 0;
    }

    for (idx = 0; idx < len; idx++)
    {
        sum += pay_load[idx];
    }

    reps_buffer[0] = (uint8_t)sum;
    reps_buffer[1] = (uint8_t)(sum >> 8);
    *reps_len = 2;
    return 0;
}

static int32_t msgq_test_func(void *data, uint16_t len, void *reps, uint16_t *reps_len, void *dev)
{
    if (reps == NULL)
    {
        return 0;
    }

    *reps_len = len;
    return 0;
}

typedef uint32_t (*zxdh_vqmb_msg_func)(vqmb_to_host_msg *msg, zxdh_reps_info *reps, struct zxdh_en_device *en_dev);

typedef struct
{
    uint8_t proc_name[64];
    zxdh_vqmb_msg_func msg_proc;
} zxdh_vqmb_msg_proc;

enum
{
    MSG_BIT_VQMB_CTRL_NP = 1,
    VQMB_MSG_TYPE_MAX = 63,
};

static uint32_t vqmb_port_ctrl_func(vqmb_to_host_msg *msg, zxdh_reps_info *reps, struct zxdh_en_device *en_dev)
{
    uint32_t err = 0;
    bool port_enable = msg->vqmb_port_ctrl_msg.port_enable;

    if (port_enable) {
        en_dev->vqmb_port_ctl = !port_enable;
        if (netif_running(en_dev->netdev))
            err = zxdh_port_enable(en_dev, port_enable);
    } else {
        if (netif_running(en_dev->netdev))
            err = zxdh_port_enable(en_dev, port_enable);
        en_dev->vqmb_port_ctl = !port_enable;
    }
    LOG_INFO_DEV(en_dev->parent, "port_enable: %d, vfid: %d\n", port_enable, msg->vqmb_hdr.vfid);
    return err;
}

zxdh_vqmb_msg_proc vqmb_msg_proc[] =
{
    {"invalid",                   NULL},
    {"vqmb_port_ctrl_func",       vqmb_port_ctrl_func},
};

int32_t zxdh_vqmb_msg_recv_func(void *pay_load, uint16_t len, void *reps_buffer, uint16_t *reps_len, void *dev)
{
    vqmb_to_host_msg *msg = (vqmb_to_host_msg *)pay_load;
    zxdh_reps_info *reps = (zxdh_reps_info *)reps_buffer;
    struct zxdh_en_device *en_dev = (struct zxdh_en_device *)dev;
    int32_t ret = 0;
    uint32_t num = 0;
    uint32_t i = 0;

    if (en_dev == NULL)
    {
        LOG_ERR("dev is NULL\n");
        return -1;
    }

    *reps_len = sizeof(reps->flag);
    num = ARRAY_SIZE(vqmb_msg_proc);
    for (i = MSG_BIT_VQMB_CTRL_NP; i < VQMB_MSG_TYPE_MAX; i++)
    {
        if (i >= num)
            break;
        if (((msg->vqmb_hdr.bits & (1 << i)) == 0))
            continue;
        LOG_DEBUG_DEV(en_dev->parent, "%s is called", vqmb_msg_proc[i].proc_name);
        if (!vqmb_msg_proc[i].msg_proc)
            continue;
        ret = vqmb_msg_proc[i].msg_proc(msg, reps, en_dev);
        if (ret != 0)
        {
            reps->flag = ZXDH_REPS_FAIL;
            LOG_ERR_DEV(en_dev->parent, "%s failed, ret: %d\n", vqmb_msg_proc[i].proc_name, ret);
            return -1;
        }
    }

    reps->flag = ZXDH_REPS_SUCC;
    LOG_DEBUG_DEV(en_dev->parent, "reps->flag: 0x%x, reps_len: %d\n", reps->flag, *reps_len);
    return 0;
}

int32_t dh_aux_msg_recv_func_register(void)
{
    int32_t ret = 0;

    ret = zxdh_bar_chan_msg_recv_register(MODULE_PF_BAR_MSG_TO_VF, zxdh_vf_msg_recv_func);
    if (0 != ret)
    {
        LOG_ERR("event_id[%d] register failed: %d\n", MODULE_PF_BAR_MSG_TO_VF, ret);
        return ret;
    }

    ret = zxdh_bar_chan_msg_recv_register(MODULE_DHTOOL, zxdh_tools_sendto_user_netlink);
    if (0 != ret)
    {
        LOG_ERR("event_id[%d] register failed: %d\n", MODULE_DHTOOL, ret);
        goto unregister_pf_to_vf;
    }

    ret = zxdh_bar_chan_msg_recv_register(MODULE_DEMO, mgr_test_cnt);
    if (0 != ret)
    {
        LOG_ERR("event_id[%d] register failed: %d\n", MODULE_DEMO, ret);
        goto unregister_dhtool;
    }

    ret = zxdh_bar_chan_msg_recv_register(MODULE_MSGQ, msgq_test_func);
    if (0 != ret)
    {
        LOG_ERR("event_id[%d] register failed: %d\n", MODULE_MSGQ, ret);
        goto unregister_demo;
    }

    ret = zxdh_bar_chan_msg_recv_register(MODULE_VQMB, zxdh_vqmb_msg_recv_func);
    if (0 != ret)
    {
        LOG_ERR("event_id[%d] register failed: %d\n", MODULE_VQMB, ret);
        goto unregister_msgq;
    }

    return ret;
unregister_msgq:
    zxdh_bar_chan_msg_recv_unregister(MODULE_MSGQ);
unregister_demo:
    zxdh_bar_chan_msg_recv_unregister(MODULE_DEMO);
unregister_dhtool:
    zxdh_bar_chan_msg_recv_unregister(MODULE_DHTOOL);
unregister_pf_to_vf:
    zxdh_bar_chan_msg_recv_unregister(MODULE_PF_BAR_MSG_TO_VF);
    return ret;
}

void dh_aux_msg_recv_func_unregister(void)
{
    zxdh_bar_chan_msg_recv_unregister(MODULE_VQMB);
    zxdh_bar_chan_msg_recv_unregister(MODULE_MSGQ);
    zxdh_bar_chan_msg_recv_unregister(MODULE_DEMO);
    zxdh_bar_chan_msg_recv_unregister(MODULE_DHTOOL);
    zxdh_bar_chan_msg_recv_unregister(MODULE_PF_BAR_MSG_TO_VF);
    return;
}

void bond_ipv6_mcast_update_slave(struct net_device *bond_dev, struct zxdh_en_device *slave_en_dev, bool is_link)
{
    struct inet6_dev *idev = NULL;
    struct inet6_ifaddr *ifa = NULL;
    struct in6_addr *ipv6_addr = NULL;
    struct in6_addr sol_addr = {0};
    uint8_t mcast_mac[ETH_ALEN] = {0};
    int32_t ret = 0;

    if (slave_en_dev == NULL || bond_dev == NULL)
    {
        LOG_ERR("slave_en_dev or bond_dev is NULL\n");
        return;
    }

    LOG_INFO_DEV(slave_en_dev->parent, "member %s %s IPv6 multicast MAC from bond %s\n",
        slave_en_dev->netdev->name, is_link ?  "add" : "clear", bond_dev->name);

    rcu_read_lock();
    idev = __in6_dev_get(bond_dev);
    if (idev == NULL)
    {
        LOG_DEBUG_DEV(slave_en_dev->parent, "bond device %s has no inet6_dev\n", bond_dev->name);
        rcu_read_unlock();
        return;
    }

    // 遍历 bond 设备的 IPv6 地址列表
    list_for_each_entry_rcu(ifa, &idev->addr_list, if_list)
    {
        ipv6_addr = &ifa->addr;

        // 跳过链路本地地址 (fe80::/10，即 scope == IFA_LINK)
        if (ifa->scope == IFA_LINK)
        {
            continue;
        }

        // 计算组播 MAC 地址
        addrconf_addr_solict_mult(ipv6_addr, &sol_addr);
        ipv6_eth_mc_map(&sol_addr, mcast_mac);

        if (is_link)
        {
            LOG_INFO_DEV(slave_en_dev->parent, "Sync IPv6 address %pI6c to slave %s, multicast MAC: %pM\n",
                         ipv6_addr, slave_en_dev->netdev->name, mcast_mac);
            // zxdh_ip6mac_add_safe 内部已有引用计数检查：
            // - 如果 MAC 已存在，只增加引用计数，不会重复下发到 NP
            // - 如果 MAC 不存在，才会真正下发到 NP
            ret = zxdh_ip6mac_add_safe(slave_en_dev, ipv6_addr->s6_addr32, mcast_mac);
            if (ret != 0)
            {
                LOG_ERR_DEV(slave_en_dev->parent, "zxdh_ip6mac_add_safe failed for IPv6 %pI6c\n", ipv6_addr);
            }
        }
        else
        {
            LOG_INFO_DEV(slave_en_dev->parent, "Clear IPv6 multicast MAC %pM from slave %s (IPv6: %pI6c)\n",
                         mcast_mac, slave_en_dev->netdev->name, ipv6_addr);
            ret = zxdh_ip6mac_del_safe(slave_en_dev, ipv6_addr->s6_addr32, mcast_mac);
            if (ret != 0)
            {
                LOG_ERR_DEV(slave_en_dev->parent, "zxdh_ip6mac_del_safe failed for IPv6 %pI6c\n", ipv6_addr);
            }
        }
    }
    rcu_read_unlock();
}
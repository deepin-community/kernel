#include <linux/dinghai/driver.h>
#include <linux/netdevice.h>
#include <linux/kref.h>
#include <net/bonding.h>
#include "zxdh_lag.h"
#include "rdma_ops.h"
#include "dual_tor.h"
#include <linux/ethtool.h>
#include <linux/mutex.h>
#include "../en_aux/en_aux_events.h"

/* 兼容性定义：NETDEV_LAG_HASH 宏 */
#ifndef NETDEV_LAG_HASH_L2
#define NETDEV_LAG_HASH_L2      0
#endif
#ifndef NETDEV_LAG_HASH_L23
#define NETDEV_LAG_HASH_L23     1
#endif
#ifndef NETDEV_LAG_HASH_L34
#define NETDEV_LAG_HASH_L34     2
#endif
#ifndef NETDEV_LAG_HASH_L23S
#define NETDEV_LAG_HASH_L23S    3
#endif
#ifndef NETDEV_LAG_HASH_L34S
#define NETDEV_LAG_HASH_L34S    4
#endif
#ifndef NETDEV_LAG_HASH_L2S
#define NETDEV_LAG_HASH_L2S     5
#endif
#ifndef NETDEV_LAG_HASH_UNKNOWN
#define NETDEV_LAG_HASH_UNKNOWN 6
#endif

/* 定义全局变量，记录bond组信息 */
static LIST_HEAD(zxdh_bond_list);
static LIST_HEAD(zxdh_aux_netdev_list);
static DEFINE_IDA(zxdh_bond_group_ids);
static struct mutex mlock;
extern const struct net_device_ops zxdh_netdev_ops;
#define RDMA_PHY_PORT_0_bit 17
#define RDMA_PHY_PORT_1_bit 16


void zxdh_lag_lock_init(void)
{
    mutex_init(&mlock);
    psn_init();
}

void zxdh_lag_lock_deinit(void)
{
    psn_exit();
    mutex_destroy(&mlock);
}

static bool netif_is_zxdh_aux(struct net_device *dev)
{
    return dev && (dev->netdev_ops == &zxdh_netdev_ops);
}

#ifndef CGS_V5_693
static uint32_t zxdh_covert_hash_type(uint32_t hash_type)
{
    uint32_t np_hash_type = 0;

    switch (hash_type)
    {
        case NETDEV_LAG_HASH_L2:
        {
            np_hash_type = ZXDH_NETDEV_LAG_HASH_L2;
            break;
        }
        case NETDEV_LAG_HASH_L23:
        {
            np_hash_type = ZXDH_NETDEV_LAG_HASH_L23;
            break;
        }
        case NETDEV_LAG_HASH_L34:
        {
            np_hash_type = ZXDH_NETDEV_LAG_HASH_L34;
            break;
        }
        default:
        {
            np_hash_type = ZXDH_NETDEV_LAG_HASH_NONE;
            break;
        }
    }

    return np_hash_type;
}
#endif /* !CGS_V5_693 */

static uint32_t zxdh_covert_bond_tx_type(uint32_t tx_type)
{
    uint32_t np_tx_type = 0;

    switch (tx_type)
    {
        case NETDEV_LAG_TX_TYPE_ACTIVEBACKUP:
        {
            np_tx_type = ZXDH_NETDEV_LAG_TX_TYPE_ACTIVEBACKUP;
            break;
        }
        case NETDEV_LAG_TX_TYPE_HASH:
        {
            np_tx_type = ZXDH_NETDEV_LAG_TX_TYPE_HASH;
            break;
        }
        default:
        {
            np_tx_type = ZXDH_NETDEV_LAG_TX_TYPE_UNKNOWN;
            break;
        }
    }

    return np_tx_type;
}

static bool zxdh_is_lower_state_change(struct netdev_lag_lower_state_info *cur_lag_lower_info,
                                           struct netdev_lag_lower_state_info *new_lag_lower_info)
{
    bool flag = true;

    if (cur_lag_lower_info->link_up == new_lag_lower_info->link_up
        && cur_lag_lower_info->tx_enabled == new_lag_lower_info->tx_enabled)
    {
        flag = false;
    }

    return flag;
}

static int32_t zxdh_hardware_bond_set_mac_to_primary(struct zxdh_en_device *cur_en_dev,
                                                          struct zxdh_en_device *primary_en_dev, 
                                                          struct zxdh_lag_tracker *tracker,
                                                          struct net_device *upper_netdev)
{
    int32_t ret               = 0;
    uint16_t sriov_vlan_tpid  = 0;
    uint16_t sriov_vlan_id    = 0;
    uint16_t current_vport    = 0;
    struct netdev_hw_addr *ha = NULL;
    bool delete_flag          = true;
    bool add_flag             = true;
    struct net_device *cur_netdev, *primary_netdev = NULL;
    DPP_PF_INFO_T primary_dpp_pf_info = {
        .slot  = primary_en_dev->slot_id,
        .vport = primary_en_dev->vport,
    };

    cur_netdev     = cur_en_dev->netdev;
    primary_netdev = primary_en_dev->netdev;

    if (cur_netdev == primary_netdev)  // primary口无需给自己更新mac
        return 0;

    if (cur_en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
    {
        LOG_INFO_DEV(cur_en_dev->parent, "%s state is INTERNAL_ERROR, can't process mac change event, exit\n", netdev_name(cur_en_dev->netdev));
        return -1;
    }

    if (primary_en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
    {
        LOG_INFO_DEV(cur_en_dev->parent, "%s state is INTERNAL_ERROR, can't process mac change event, exit\n", netdev_name(primary_en_dev->netdev));
        return -1;
    }

    if (tracker->tx_type != ZXDH_NETDEV_LAG_TX_TYPE_ACTIVEBACKUP)
    {
        LOG_DEBUG_DEV(cur_en_dev->parent, "%s tx_type is not ACTIVEBACKUP\n", upper_netdev->name);
        return 0;
    }

    if (!memcmp(primary_netdev->dev_addr, cur_netdev->dev_addr,
                cur_netdev->addr_len))  // 若副口mac地址与主口mac地址一致，则不允许add
    {
        LOG_INFO_DEV(cur_en_dev->parent, "primary pf %s netdev mac %pM is same with temp pf %s netdev mac %pM, can't add\n",
                 primary_netdev->name, primary_netdev->dev_addr, cur_netdev->name, cur_netdev->dev_addr);
        goto err;
    }
    if (!memcmp(primary_netdev->dev_addr, cur_en_dev->last_mac_addr.sa_data,
                cur_netdev->addr_len))  // 若副口last_mac_addr地址与主口mac地址一致，则不允许del
    {
        LOG_INFO_DEV(cur_en_dev->parent, "primary pf %s netdev mac %pM is same with temp pf %s last add mac %pM, can't del\n",
                 primary_netdev->name, primary_netdev->dev_addr, cur_netdev->name, cur_en_dev->last_mac_addr.sa_data);
        goto err;
    }

    list_for_each_entry(ha, &primary_netdev->uc.list, list)  // 遍历primary PF通过bridge fdb配置的MAC
    {
        if (!memcmp(ha->addr, cur_en_dev->last_mac_addr.sa_data,
                    cur_netdev->addr_len))  // 如果last_mac_addr在primary PF的uc.list中，则不允许删除
        {
            delete_flag = false;
            LOG_INFO_DEV(cur_en_dev->parent, "%pM is used by uc.list of primary pf %s, can't del\n", cur_en_dev->last_mac_addr.sa_data,
                     primary_netdev->name);
            goto err;
        }
        if (!memcmp(ha->addr, cur_netdev->dev_addr,
                    cur_netdev->addr_len))  // 如果当前mac在primary PF的uc.list中，则不允许添加
        {
            add_flag = false;
            LOG_INFO_DEV(cur_en_dev->parent, "%pM is used by uc.list of primary pf %s, can't add\n", cur_netdev->dev_addr,
                     primary_netdev->name);
            goto err;
        }
    }

    if (delete_flag)
    {
        ret = dpp_unicast_mac_search(&primary_dpp_pf_info, cur_en_dev->last_mac_addr.sa_data, sriov_vlan_tpid,
                                     sriov_vlan_id, &current_vport);     // 在primary PF的转发域内搜索
        if ((ret == 0) && (primary_dpp_pf_info.vport == current_vport))  // 说明当前mac属于primary PF，需要删除
        {
            ret = dpp_del_mac(&primary_dpp_pf_info, cur_en_dev->last_mac_addr.sa_data, sriov_vlan_tpid, sriov_vlan_id);
            if (ret != 0)
            {
                LOG_ERR_DEV(cur_en_dev->parent, "pf del mac failed, retval: %d\n", ret);
                goto err;
            }
        }
        else if ((ret == 0)
                 && (primary_dpp_pf_info.vport != current_vport))  // 如果找到mac, 属于primary PF的vf, 则报错返回
        {
            LOG_INFO_DEV(cur_en_dev->parent, "%pM is used by primary pf(%s)-vf 0x%x, can't del\n", cur_en_dev->last_mac_addr.sa_data,
                     primary_netdev->name, current_vport);
            goto err;
        }
        else if (ret == DPP_HASH_RC_SRH_FAIL)  // 如果没找到mac，则无需删除
        {
            LOG_DEBUG_DEV(cur_en_dev->parent, "don't find %pM in primary pf(%s), don't need del\n", cur_netdev->dev_addr, primary_netdev->name);
        }
        else
        {
            LOG_ERR_DEV(cur_en_dev->parent, "dpp_unicast_mac_search err ,ret%d\n", ret);
            goto err;
        }
    }

    if (add_flag)
    {
        ret = dpp_unicast_mac_search(&primary_dpp_pf_info, cur_netdev->dev_addr, sriov_vlan_tpid, sriov_vlan_id,
                                     &current_vport);                    // 在primary PF的转发域内搜索
        if ((ret == 0) && (primary_dpp_pf_info.vport == current_vport))  // 说明当前mac属于primary PF，不需要添加
        {
            LOG_DEBUG_DEV(cur_en_dev->parent, "%pM is used by primary pf(%s), don't need add\n", cur_netdev->dev_addr, primary_netdev->name);
        }

        if ((ret == 0) && (primary_dpp_pf_info.vport != current_vport))  // 如果找到mac, 属于primary PF的vf, 则报错返回
        {
            LOG_ERR_DEV(cur_en_dev->parent, "%pM is used by primary pf(%s) vport %d, can't add\n", cur_netdev->dev_addr, primary_netdev->name,
                    current_vport);
            goto err;
        }
        else if (ret == DPP_HASH_RC_SRH_FAIL)  // 没找到mac。则需要添加
        {
            ret = dpp_add_mac(&primary_dpp_pf_info, cur_netdev->dev_addr, sriov_vlan_tpid, sriov_vlan_id);
            if (ret != 0)
            {
                LOG_ERR_DEV(cur_en_dev->parent, "pf add mac failed, retval: %d\n", ret);
                goto err;
            }
        }
        else
        {
            LOG_ERR_DEV(cur_en_dev->parent, "dpp_unicast_mac_search err ,ret%d\n", ret);
            goto err;
        }
    }
    ether_addr_copy(cur_en_dev->last_mac_addr.sa_data, cur_netdev->dev_addr);
    return 0;
err:
    return -1;
}

static uint32_t zxdh_hardware_del_mac_from_primary(struct zxdh_en_device *primary_en_dev,
                                                          struct zxdh_en_device *other_en_dev, 
                                                          struct zxdh_lag_tracker *tracker,
                                                          struct net_device *upper_netdev)
{
    int32_t ret               = 0;
    uint16_t sriov_vlan_tpid  = 0;
    uint16_t sriov_vlan_id    = 0;
    uint16_t current_vport    = 0;
    struct netdev_hw_addr *ha = NULL;
    bool delete_flag          = true;
    struct net_device *other_netdev, *primary_netdev = NULL;
    struct dh_core_dev *dh_dev = primary_en_dev->parent;
    DPP_PF_INFO_T primary_dpp_pf_info = {
        .slot  = primary_en_dev->slot_id,
        .vport = primary_en_dev->vport,
    };

    other_netdev = other_en_dev->netdev;
    primary_netdev = primary_en_dev->netdev;

    if (other_netdev == primary_netdev)  // primary口无需给自己更新mac
        return 0;

    if (tracker->tx_type != ZXDH_NETDEV_LAG_TX_TYPE_ACTIVEBACKUP)
    {
        LOG_DEBUG_DEV(dh_dev, "%s tx_type is not ACTIVEBACKUP\n", upper_netdev->name);
        return 0;
    }

    if (!memcmp(primary_netdev->dev_addr, other_en_dev->last_mac_addr.sa_data,
                other_netdev->addr_len))  // 若副口last_mac_addr地址与主口mac地址一致，则不允许del
    {
        LOG_INFO_DEV(dh_dev, "primary pf %s netdev mac %pM is same with temp pf %s last add mac %pM, can't del\n",
                 primary_netdev->name, primary_netdev->dev_addr, other_netdev->name, other_en_dev->last_mac_addr.sa_data);
        goto err;
    }

    list_for_each_entry(ha, &primary_netdev->uc.list, list)  // 遍历primary PF通过bridge fdb配置的MAC
    {
        if (!memcmp(ha->addr, other_en_dev->last_mac_addr.sa_data,
                    other_netdev->addr_len))  // 如果last_mac_addr在primary PF的uc.list中，则不允许删除
        {
            delete_flag = false;
            LOG_INFO_DEV(dh_dev, "%pM is used by uc.list of primary pf %s, can't del\n", other_en_dev->last_mac_addr.sa_data,
                     primary_netdev->name);
            goto err;
        }
    }

    if (delete_flag)
    {
        ret = dpp_unicast_mac_search(&primary_dpp_pf_info, other_en_dev->last_mac_addr.sa_data, sriov_vlan_tpid,
                                     sriov_vlan_id, &current_vport);     // 在primary PF的转发域内搜索
        if ((ret == 0) && (primary_dpp_pf_info.vport == current_vport))  // 说明当前mac属于primary PF，需要删除
        {
            ret = dpp_del_mac(&primary_dpp_pf_info, other_en_dev->last_mac_addr.sa_data, sriov_vlan_tpid, sriov_vlan_id);
            if (ret != 0)
            {
                LOG_ERR_DEV(dh_dev, "pf del mac failed, retval: %d\n", ret);
                goto err;
            }
        }
        else if ((ret == 0)
                 && (primary_dpp_pf_info.vport != current_vport))  // 如果找到mac, 属于primary PF的vf, 则报错返回
        {
            LOG_INFO_DEV(dh_dev, "%pM is used by primary pf(%s)-vf 0x%x, can't del\n", other_en_dev->last_mac_addr.sa_data,
                     primary_netdev->name, current_vport);
            goto err;
        }
        else if (ret == DPP_HASH_RC_SRH_FAIL)  // 如果没找到mac，则无需删除
        {
            LOG_DEBUG_DEV(dh_dev, "don't find %pM in primary pf(%s), don't need del\n", other_netdev->dev_addr, primary_netdev->name);
        }
        else
        {
            LOG_ERR_DEV(dh_dev, "dpp_unicast_mac_search err ,ret%d\n", ret);
            goto err;
        }
    }

    zte_memset_s(other_en_dev->last_mac_addr.sa_data, 0, ETH_ALEN);
    return 0;
err:
    return -1;
}

static void zxdh_hardware_del_all_mac_from_primary(struct zxdh_lag_tracker *tracker, struct zxdh_en_device *primary_en_dev, struct net_device *upper_netdev)
{
    struct slave_state *slave             = NULL;
    struct zxdh_en_device *en_dev         = NULL;
    int i = 0;

    // 遍历所有slave端口
    for (i = 0; i < DH_MAX_PORTS; i++)
    {
        slave = &tracker->slaves[i];

        // 第一步：筛选符合条件的slave（必须同时满足以下所有条件）
        // 1. slave指针非空 2. slave处于激活状态 3. en_dev指针有效 4. 支持硬件bond
        if (!(slave && slave->slave_info.is_active && slave->slave_info.en_dev))
        {
            continue;  // 不符合条件，跳过当前slave
        }
        en_dev = slave->slave_info.en_dev;
        zxdh_hardware_del_mac_from_primary(primary_en_dev, en_dev, tracker, upper_netdev);
    }
}

static uint16_t zxdh_convert_pcie_id_2_vfid(uint16_t pcie_id)
{
    uint16_t pf_id = 0;
    uint16_t ep_id = 0;

    pf_id = (pcie_id >> 8) & 0x7;
    ep_id = (pcie_id >> 12) & 0x7;

    return ZXDH_PF_VFID(ep_id, pf_id);
}

static uint16_t zxdh_covert_netdev_2_vfid(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    return zxdh_convert_pcie_id_2_vfid(en_dev->pcie_id);
}

void zxdh_bond_set_dpp_member_port(struct zxdh_en_device *en_dev, bool enable, struct event_node *node, int32_t group_ida)
{
    uint8_t phy_port              = en_dev->phy_port;
    uint32_t actual_enable_bit    = en_dev->phy_port == 0 ? RDMA_PHY_PORT_0_bit : RDMA_PHY_PORT_1_bit;
    DPP_PF_INFO_T pf_info = {
        .slot  = en_dev->slot_id,
        .vport = en_dev->vport,
    };

    if (en_dev->device_state != ZXDH_DEVICE_STATE_INTERNAL_ERROR)
    {
        if (enable)
        {
            dpp_lag_group_member_add(&pf_info, group_ida, phy_port);
        }
        else
        {
            dpp_lag_group_member_del(&pf_info, group_ida, phy_port);
        }
        /* set panel attribute: BOND_LINK_UP */
        dpp_uplink_phy_attr_set(&pf_info, phy_port, UPLINK_PHY_PORT_BOND_LINK_UP, !!enable);
    }
    if (en_dev->ops->get_bond_port_type(en_dev->parent) == BOND_TWO_PORT_TYPE)
    {
        dpp_pktrx_mcode_glb_cfg_write(&pf_info, actual_enable_bit, actual_enable_bit, !!enable);  //自愈时，此表项必须下成功；否则触发 no-primary pf 的 flr 自愈时，会导致负载均衡 bond 断流
    }

    if (node == NULL)
    {
        LOG_INFO_DEV(en_dev->parent, "%s set members: group_ida %hhu, phyport %hhu  bond_link_up %s\n", netdev_name(en_dev->netdev),
                 group_ida, phy_port, enable ? "true" : "false");
    }
    else
    {
        LOG_INFO_DEV(en_dev->parent, "%s node %d set members: group_ida %hhu, phyport %hhu  bond_link_up %s\n", netdev_name(en_dev->netdev),
                 node->idx, group_ida, phy_port, enable ? "true" : "false");
    }

    return;
}

void zxdh_update_lag_id_in_rdma_port_cfg(struct zxdh_en_device *en_dev, DPP_PF_INFO_T *pf_info, int32_t lag_id)
{
    int rdma_port_start = RDMA_START_PORT_IN_MCODE;
    int rdma_port_end   = RDMA_END_PORT_IN_MCODE;
    int rdma_port_idx   = 0;
    uint8_t phy_port    = en_dev->phy_port;
    if (en_dev->ops->get_bond_port_type(en_dev->parent) == BOND_MULTI_PORT_TYPE)
    {
        LOG_INFO_DEV(en_dev->parent, "bond_dev: %s, vport: 0x%x, lag_id: %d\n", en_dev->netdev->name, pf_info->vport, lag_id);
        for (rdma_port_idx = rdma_port_start; rdma_port_idx <= rdma_port_end; rdma_port_idx++)
        {
            dpp_pktrx_mcode_port_cfg_write(pf_info, rdma_port_idx, LAG_ID_DATA_IDX, (phy_port * LAG_ID_DATA_GAP),
                                           (phy_port * LAG_ID_DATA_GAP) + (LAG_ID_DATA_GAP - 1), lag_id);
            LOG_INFO_DEV(en_dev->parent, "rdma_port_idx %d, start bit %d, end bit %d\n", rdma_port_idx, (phy_port * LAG_ID_DATA_GAP),
                     (phy_port * LAG_ID_DATA_GAP) + (LAG_ID_DATA_GAP - 1));
        }
    }
}

uint16_t update_bond_link_info_bit(uint16_t bond_link_info, uint8_t idx, uint8_t val)
{
    uint16_t tmp_bond_link_info = bond_link_info;
    if (idx >= 16)
    {
        return 0;
    }

    if (val == 1)
    {
        tmp_bond_link_info |= (1 << idx);
    }
    else if (val == 0)
    {
        tmp_bond_link_info &= ~(1 << idx);
    }
    return tmp_bond_link_info;
}

bool are_equal(uint16_t tmp_bond_link_info, uint16_t bond_link_info)
{
    // 情况1: 两个变量都至少有一个bit为1
    bool both_non_zero = (tmp_bond_link_info != 0) && (bond_link_info != 0);

    // 情况2: 两个变量都等于0
    bool both_zero = (tmp_bond_link_info == 0) && (bond_link_info == 0);

    // 如果满足任一条件，则认为相等
    return both_non_zero || both_zero;
}

int32_t zxdh_bond_cofig_rdma_speed(struct zxdh_lag_dev *cur_lag)
{

    if (!cur_lag->tracker.is_bonded || !cur_lag->upper_netdev)
    {
        return 0;
    }

    // 速率计算，通过bond组master,由内核bonding模块计算，不再关心bond模式，协商状态；
    // 此处，获取数值，和ethtool bond2 看到的一致
    /*rtnl_lock();
    if (cur_lag->upper_netdev->ethtool_ops)
    {
        // 获取bond口速率,非自定义设备
        cur_lag->upper_netdev->ethtool_ops->get_link_ksettings(cur_lag->upper_netdev, &ks);
    }
    rtnl_unlock();

    zxdh_set_rdma_hwbond_speed(cur_lag->upper_netdev, ks.base.speed);*/
    zxdh_set_rdma_hwbond_speed(cur_lag->upper_netdev, cur_lag->bond_speed);

    return 0;
}

void zxdh_hardware_bond_primary_update(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    en_dev->ops->is_primary_port(en_dev->parent, en_dev->is_primary_port, TRUE);
    flush_work(&en_dev->plug_adev_work);
    flush_work(&en_dev->unplug_adev_work);
    if (en_dev->ops->is_rdma_enable(en_dev->parent))
    {
        LOG_INFO_DEV(en_dev->parent, "%s is_primary_port %d, is_rdma_aux_plug %d\n", netdev->name, en_dev->is_primary_port, en_dev->is_rdma_aux_plug);
        if (en_dev->is_primary_port && !en_dev->is_rdma_aux_plug)
        {
            LOG_INFO_DEV(en_dev->parent, "plug rdma auxiliary device of %s\n", netdev->name);
            queue_work(en_priv->events->wq, &en_dev->plug_adev_work);
        }
        else if(!en_dev->is_primary_port && en_dev->is_rdma_aux_plug)
        {
            LOG_INFO_DEV(en_dev->parent, "unplug rdma auxiliary device of %s\n", netdev->name);
            queue_work(en_priv->events->wq, &en_dev->unplug_adev_work);
        }
    }
    return;
}

static void zxdh_lag_remove_rdma_devices_without_primary(struct zxdh_lag_tracker *tracker, uint16_t primary_pf_idx)
{
    int i;
    struct slave_state *slave;
    for (i = 0; i < DH_MAX_PORTS; i++)
    {
        if (i == primary_pf_idx)
        {
            continue;
        }
        slave = &tracker->slaves[i];
        if (slave && slave->slave_info.is_active && slave->slave_info.en_dev)
        {
            slave->slave_info.en_dev->is_primary_port = FALSE;
            zxdh_hardware_bond_primary_update(slave->slave_info.en_dev->netdev);  // 移除rdma设备
        }
    }
}

static int16_t zxdh_lag_check_primary_pf(struct zxdh_lag_tracker *tracker, uint16_t primary_pf_idx)
{
    struct slave_state *slave = NULL;

    if (!tracker)
    {
        return -1;
    }

    slave = &tracker->slaves[primary_pf_idx];

    // 1. slave指针非空 2. slave处于激活状态 3. en_dev指针有效 4. 支持硬件bond
    if (slave && slave->slave_info.is_active && slave->slave_info.en_dev)
    {
        return 0;
    }

    return -1;
}

static void zxdh_lag_add_rdma_devices(struct zxdh_lag_tracker *tracker, int16_t primary_pf_idx)
{  // primary_pf_idx小于0时，恢复所有rdma设备；否则，只恢复primary_pf_idx的rdma设备
    int i;
    struct slave_state *slave = NULL;
    if (primary_pf_idx >= 0)
    {
        slave = &tracker->slaves[primary_pf_idx];
        if (slave && slave->slave_info.is_active && slave->slave_info.en_dev)
        {
            slave->slave_info.en_dev->is_primary_port = TRUE;
            zxdh_hardware_bond_primary_update(slave->slave_info.en_dev->netdev);  // 恢复rdma设备
        }
        return;
    }
    for (i = 0; i < DH_MAX_PORTS; i++)
    {
        slave = &tracker->slaves[i];
        if (slave && slave->slave_info.is_active && slave->slave_info.en_dev)
        {
            slave->slave_info.en_dev->is_primary_port = TRUE;
            zxdh_hardware_bond_primary_update(slave->slave_info.en_dev->netdev);  // 恢复rdma设备
        }
    }
    return;
}

static void zxdh_lag_init_bond_dpp(struct zxdh_lag_tracker *tracker, struct slave_state *usable_slave, int32_t group_ida)
{
    struct zxdh_en_device *en_dev = usable_slave->slave_info.en_dev;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot  = en_dev->slot_id,
        .vport = en_dev->vport,
    };
    uint8_t lag_group_create = 0;
    dpp_lag_hit_flag_get(&dpp_pf_info, group_ida, &lag_group_create);
    LOG_INFO("%s check lag_bond %d hist_flag %d\n", netdev_name(en_dev->netdev), group_ida, lag_group_create);
    if (lag_group_create == 0) // 当某设备触发flr自愈时，lag_group表项hit_flag的值不变，所以无需再次配置
        dpp_lag_group_create(&dpp_pf_info, group_ida);
    dpp_lag_mode_set(&dpp_pf_info, group_ida, tracker->tx_type);
    dpp_lag_group_hash_factor_set(&dpp_pf_info, group_ida, tracker->hash_type);
}

static int32_t zxdh_del_hardware_bond_slave(struct zxdh_en_device *en_dev, struct zxdh_en_device *primary_en_dev, struct zxdh_lag_tracker *tracker, struct net_device *upper_netdev, int32_t group_ida)
{
    int32_t ret               = 0;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot  = en_dev->slot_id,
        .vport = en_dev->vport,
    };
    uint16_t mask = 1 << en_dev->phy_port;
    en_dev->ops->set_bond_slave_flag(en_dev->parent, FALSE);

    /* vport attr: LAG ID，LAG ENABLE */
    ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_LAG_EN_OFF, 0);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set SRIOV_VPORT_LAG_EN_OFF 0 failed\n");
        return ret;
    }
    ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_LAG_ID, 0);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set SRIOV_VPORT_LAG_ID %d failed\n", 0);
        return ret;
    }
    en_dev->ops->optim_hardware_bond_time(en_dev->parent, FALSE); // RDMA时延优化
    dpp_uplink_phy_attr_set(&dpp_pf_info, en_dev->phy_port, UPLINK_PHY_PORT_SRIOV_HD_BOND_EN, 0);  // 关闭标卡硬bond标识
    dpp_uplink_phy_hardware_bond_set(&dpp_pf_info, en_dev->phy_port, 0);
    dpp_uplink_phy_attr_set(&dpp_pf_info, en_dev->phy_port, UPLINK_PHY_PORT_PRIMARY_PF_VQM_VFID, 0); // 将primary pf的vfid从自身的np表中删除
    zxdh_bond_set_dpp_member_port(en_dev, FALSE, NULL, group_ida);
    zxdh_update_lag_id_in_rdma_port_cfg(en_dev, &dpp_pf_info, 0);

    if (en_dev == primary_en_dev)
    {
        // 1、让rdma驱动释放bond的netdev
        zxdh_set_rdma_hwbond_master(en_dev->netdev, upper_netdev, FALSE);
        // 2、将自身最新的speed传给rdma驱动
        zxdh_set_rdma_hwbond_speed(en_dev->netdev, en_dev->speed);
        // 3、从转发域中删除各个no-primary pf的mac
        zxdh_hardware_del_all_mac_from_primary(tracker, primary_en_dev, upper_netdev);
        // 4、清除自身的bond_link_info变量，更新自身VF的链路状态
        en_dev->bond_link_info = update_bond_link_info_bit(en_dev->bond_link_info, en_dev->phy_port, en_dev->link_up ? 1 : 0); //获取更新后的bond_link_info
        en_dev->bond_link_info &= mask; //保留 primary pf phyport 对应的位，其他位清零
        // 通知VF更新link状态
        if (en_dev->device_state != ZXDH_DEVICE_STATE_INTERNAL_ERROR)
            update_vf_link_info(en_dev, en_dev->link_up ? 1 : 0, TRUE);
        en_dev->ops->set_bond_link_info(en_dev->parent, en_dev->bond_link_info);
    }

    LOG_INFO_DEV(en_dev->parent, "%s slave %s disabled\n", netdev_name(upper_netdev), netdev_name(en_dev->netdev));
    return 0;
}

static struct slave_state *zxdh_lag_find_usable_slave(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag)
{
    struct slave_state *slave             = NULL;
    struct zxdh_en_device *en_dev         = NULL;
    int i = 0;

    // 遍历所有slave端口
    for (i = 0; i < DH_MAX_PORTS; i++)
    {
        slave = &tracker->slaves[i];

        // 第一步：筛选符合条件的slave（必须同时满足以下所有条件）
        // 1. slave指针非空 2. slave处于激活状态 3. en_dev指针有效 4. 支持硬件bond
        if (!(slave->slave_info.is_active && slave->slave_info.en_dev))
        {
            continue;  // 不符合条件，跳过当前slave
        }
        en_dev = slave->slave_info.en_dev;
        if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
        {
            LOG_INFO("lag_dev[%d] %s state is INTERNAL_ERROR, can't use, exit\n", cur_lag->idx, netdev_name(en_dev->netdev));
            continue;
        }
        // 找到第一个可用的slave，立即返回
        LOG_INFO("lag_dev[%d] %s is usable, return it\n", 
                 cur_lag->idx, netdev_name(en_dev->netdev));
        return slave;
    }
    LOG_INFO("lag_dev[%d] no usable slave found", cur_lag->idx);
    return NULL;
}

static void zxdh_lag_del_hardbond_slave_dpp(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag)
{
    struct slave_state *slave             = NULL;
    struct zxdh_en_device *en_dev         = NULL;
    struct slave_state *primary_slave     = &cur_lag->tracker.slaves[cur_lag->primary_pf_idx];
    struct zxdh_en_device *primary_en_dev = primary_slave->slave_info.en_dev;
    int i = 0;

    // 遍历所有slave端口
    for (i = 0; i < DH_MAX_PORTS; i++)
    {
        slave = &tracker->slaves[i];

        // 第一步：筛选符合条件的slave（必须同时满足以下所有条件）
        // 1. slave指针非空 2. slave处于激活状态 3. en_dev指针有效 4. 支持硬件bond
        if (!(slave && slave->slave_info.is_active && slave->slave_info.en_dev))
        {
            continue;  // 不符合条件，跳过当前slave
        }
        en_dev = slave->slave_info.en_dev;
        if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
        {
            LOG_INFO("lag_dev[%d] %s state is INTERNAL_ERROR, can't delete slave dpp, exit\n", cur_lag->idx, netdev_name(en_dev->netdev));
            continue;
        }
        zxdh_del_hardware_bond_slave(en_dev, primary_en_dev, tracker, cur_lag->upper_netdev, cur_lag->group_ida);
    }
}

static void zxdh_update_primary_vfid_to_self(struct zxdh_en_device *en_dev, struct zxdh_en_device *primary_en_dev, bool reset)
{
    DPP_PF_INFO_T dpp_pf_info = {
        .slot  = en_dev->slot_id,
        .vport = en_dev->vport,
    };
    uint16_t primary_vfid = reset ? 0 : zxdh_covert_netdev_2_vfid(primary_en_dev->netdev);
    // 配置 primary PF 的 vfid 到自身NP中
    dpp_uplink_phy_attr_set(&dpp_pf_info, en_dev->phy_port, UPLINK_PHY_PORT_PRIMARY_PF_VQM_VFID, primary_vfid);
    LOG_INFO("%s set primary pf vqm vfid %hu, phyport %hu\n", en_dev->netdev->name, primary_vfid,
             en_dev->phy_port);
}

static int32_t zxdh_init_hardware_bond_slave(struct zxdh_en_device *en_dev, struct zxdh_en_device *primary_en_dev, struct zxdh_lag_tracker *tracker, struct net_device *upper_netdev, bool lagstat, int32_t group_ida)
{
    int32_t ret               = 0;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot  = en_dev->slot_id,
        .vport = en_dev->vport,
    };
    en_dev->ops->set_bond_slave_flag(en_dev->parent, TRUE);

    /* vport attr: LAG ID，LAG ENABLE */
    ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_LAG_EN_OFF, 1);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set SRIOV_VPORT_LAG_EN_OFF 1 failed\n");
        return ret;
    }
    ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_LAG_ID, group_ida);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set SRIOV_VPORT_LAG_ID %d failed\n", group_ida);
        return ret;
    }
    en_dev->ops->optim_hardware_bond_time(en_dev->parent, TRUE); // RDMA时延优化
    dpp_uplink_phy_attr_set(&dpp_pf_info, en_dev->phy_port, UPLINK_PHY_PORT_SRIOV_HD_BOND_EN, 1);  // 使能标卡硬bond标识
    dpp_uplink_phy_hardware_bond_set(&dpp_pf_info, en_dev->phy_port, 1);
    zxdh_update_lag_id_in_rdma_port_cfg(en_dev, &dpp_pf_info, group_ida);
    zxdh_bond_set_dpp_member_port(en_dev, lagstat, NULL, group_ida);

    if (en_dev == primary_en_dev) // primary pf
    {
        // 1、将bond的netdev传给rdma驱动
        zxdh_set_rdma_hwbond_master(en_dev->netdev, upper_netdev, true);
        // 2、配置primary口的vfid到自身NP中
        zxdh_update_primary_vfid_to_self(en_dev, en_dev, FALSE);
    }
    else // no-primary pf
    {
        // 1、配置 primary口 的 vfid 到自身NP中
        zxdh_update_primary_vfid_to_self(en_dev, primary_en_dev, FALSE);
        // 2、将自己的mac地址更新到primary pf的转发域中
        zxdh_hardware_bond_set_mac_to_primary(en_dev, primary_en_dev, tracker, upper_netdev);
    }
    LOG_INFO_DEV(en_dev->parent, "%s slave %s enabled\n", netdev_name(upper_netdev), netdev_name(en_dev->netdev));
    return 0;
}

static void zxdh_lag_init_hardbond_slave_dpp(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct event_node *node)
{
    struct slave_state *slave             = NULL;
    struct zxdh_en_device *en_dev         = NULL;
    struct slave_state *primary_slave     = &tracker->slaves[cur_lag->primary_pf_idx];
    struct zxdh_en_device *primary_en_dev = primary_slave->slave_info.en_dev;
    int i = 0;
        // 2. 更新slave的dpp表
    bool lagstat = FALSE;

    // 遍历所有slave端口
    for (i = 0; i < DH_MAX_PORTS; i++)
    {
        slave = &tracker->slaves[i];

        // 第一步：筛选符合条件的slave（必须同时满足以下所有条件）
        // 1. slave指针非空 2. slave处于激活状态 3. en_dev指针有效 4. 支持硬件bond
        if (!(slave && slave->slave_info.is_active && slave->slave_info.en_dev))
        {
            continue;  // 不符合条件，跳过当前slave
        }
        en_dev = slave->slave_info.en_dev;
        if ((en_dev->device_state != ZXDH_DEVICE_STATE_INTERNAL_ERROR) || (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR && node->from_recover && node->event_slave_id == en_dev->panel_id))
        {
            lagstat = slave->lower_state.link_up && slave->lower_state.tx_enabled;
            zxdh_init_hardware_bond_slave(en_dev, primary_en_dev, tracker, cur_lag->upper_netdev, lagstat, cur_lag->group_ida);
        }
        else
        {
            LOG_INFO_DEV(en_dev->parent, "lag_dev[%d] %s state is INTERNAL_ERROR, can't init slave dpp, exit\n", cur_lag->idx, netdev_name(en_dev->netdev));
            continue;
        }
    }
}

static int32_t zxdh_lag_request_lag_id(struct zxdh_lag_dev *cur_lag, struct event_node *node)
{
    /* create bond group id, range [0, 100] */
    if (node->tracker.target_bond_type == SPECIAL_BOND)
    {
        cur_lag->group_ida = 0;
    }
    else if(node->tracker.target_bond_type == HARDWARE_BOND && cur_lag->group_ida < 0) //若自愈前是硬bond，则自愈后group_ida值不变
    {
        cur_lag->group_ida = ida_alloc_range(&zxdh_bond_group_ids, 1, 100, GFP_KERNEL);
    }
    if (cur_lag->group_ida < 0)
    {
        return -1;
    }
    return cur_lag->group_ida;
}

static void zxdh_lag_release_lag_id(struct zxdh_lag_dev *cur_lag)
{
    if (cur_lag->tracker.bond_type == HARDWARE_BOND)
        ida_free(&zxdh_bond_group_ids, cur_lag->group_ida);
    cur_lag->group_ida = -1;
}

void zxdh_hardware_bond_update_link_for_primary(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, bool force_update)
{
    struct slave_state *slave             = NULL;
    struct zxdh_en_device *en_dev         = NULL;
    struct slave_state *primary_slave     = &tracker->slaves[cur_lag->primary_pf_idx];
    struct zxdh_en_device *primary_en_dev = primary_slave->slave_info.en_dev;
    uint16_t tmp_bond_link_info = 0;
    uint8_t link_status;
    bool has_changed;
    int i = 0;

    // 1. 遍历所有slave端口，获取最新的link状态，更新到tmp_bond_link_info
    for (i = 0; i < DH_MAX_PORTS; i++)
    {
        slave = &tracker->slaves[i];

        // 筛选符合条件的slave（必须同时满足以下所有条件）
        // 1) slave指针非空 2) slave处于激活状态 3) en_dev指针有效 4) 支持硬件bond
        if (!(slave && slave->slave_info.is_active && slave->slave_info.en_dev))
        {
            continue;  // 不符合条件，跳过当前slave
        }
        en_dev = slave->slave_info.en_dev;
        // 2. 将slave的链路状态更新到primary PF bond_link_info上
        tmp_bond_link_info = update_bond_link_info_bit(tmp_bond_link_info, en_dev->phy_port, slave->lower_state.link_up);
    }
    // 3. 判断tmp_bond_link_info和primary pf当前的bond_link_info是否一致，如果一致，则无需更新vf的link状态；否则需要更新。
    if (primary_en_dev->device_state != ZXDH_DEVICE_STATE_INTERNAL_ERROR)
    {
        // 计算link状态值
        link_status = (tmp_bond_link_info == 0) ? 0 : 1;
        has_changed = !are_equal(tmp_bond_link_info, primary_en_dev->bond_link_info);
        update_vf_link_info(primary_en_dev, link_status, has_changed || force_update);
        LOG_INFO_DEV(primary_en_dev->parent, "primary pf %s bond_link_info %d -> %d\n", netdev_name(primary_en_dev->netdev), primary_en_dev->bond_link_info, tmp_bond_link_info);
    }
    primary_en_dev->bond_link_info = tmp_bond_link_info;
    primary_en_dev->ops->set_bond_link_info(primary_en_dev->parent, primary_en_dev->bond_link_info);
    return;
}

/* 获取面板口对应的面板口以及状态信息*/
void zxdh_update_two_bond_panel_state(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct zxdh_en_device *en_dev, bool use_en_dev_linkup)
{
    struct slave_state *slave = NULL;
    struct bond_ports_info bond_info = {0};
    struct net_device *cur_ndev = en_dev->netdev;
    bool link_up;
    uint8_t port_type = 0;
    uint8_t panel_id_max;

    slave = &tracker->slaves[en_dev->panel_id];

    /* 筛选符合条件的slave（必须同时满足以下所有条件*/
    /* 1. slave指针非空 2. slave处于激活状态 3. en_dev指针有效 4. 支持硬件bond*/
    if (!(slave && slave->slave_info.is_active && slave->slave_info.en_dev))
    {
        return;  // 不符合条件，跳过当前slave
    }
    link_up = use_en_dev_linkup ? en_dev->link_up : slave->lower_state.link_up;
    port_type = en_dev->ops->get_bond_port_type(en_dev->parent);
    panel_id_max = (port_type == BOND_MULTI_PORT_TYPE)? 4 : 2;
    if (en_dev->panel_id >= panel_id_max)
    {
        return;
    }

    bond_info.slave_devs[en_dev->panel_id].link_state = link_up ? BOND_STATE_UP : BOND_STATE_DOWN;
    bond_info.slave_devs[en_dev->panel_id].np_port = en_dev->phy_port;
    bond_info.slave_devs[en_dev->panel_id].is_enable = 1;
    LOG_DEBUG_DEV(en_dev->parent, "slave%u %s states: %u, np_port: %u\n",   en_dev->panel_id, cur_ndev->name,
                                                                    bond_info.slave_devs[en_dev->panel_id].link_state,
                                                                    bond_info.slave_devs[en_dev->panel_id].np_port);


    bond_dev_update_event(cur_lag->upper_name, &bond_info);
}

void zxdh_update_hw_bond_panel_state(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct zxdh_en_device *en_dev, bool use_en_dev_linkup)
{
    if (cur_lag->tracker.bond_type != HARDWARE_BOND)
    {
        LOG_DEBUG_DEV(en_dev->parent, "don't need to update psn when lag_dev[%d] isn't hardware_bond\n", cur_lag->idx);
        return;
    }

    return zxdh_update_two_bond_panel_state(tracker, cur_lag, en_dev, use_en_dev_linkup);
}

static void zxdh_hardware_bond_lower_event_process(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct event_node *node)
{
    // 1. 获得触发本次事件的slave端口
    struct slave_state *cur_slave     = &tracker->slaves[node->event_slave_id];
    struct zxdh_en_device *cur_en_dev = cur_slave->slave_info.en_dev;
    bool lagstat = cur_slave->lower_state.link_up && cur_slave->lower_state.tx_enabled;
    bool is_primary_pf_link_up = (node->event_slave_id == cur_lag->primary_pf_idx) && cur_slave->lower_state.link_up;
    LOG_INFO_DEV(cur_en_dev->parent, "%s node %d link_up: %u, tx_enable: %u.\n", netdev_name(cur_en_dev->netdev), node->idx, cur_slave->lower_state.link_up, cur_slave->lower_state.tx_enabled);
    // 2. 更新slave的dpp表
    zxdh_bond_set_dpp_member_port(cur_en_dev, lagstat, node, cur_lag->group_ida);
    // 3. 获取bond网络设备速率，更新给rdma驱动
    zxdh_bond_cofig_rdma_speed(cur_lag);
    // 4. 更新双平面
    if (cur_slave->lower_state.link_up == cur_slave->lower_state.tx_enabled)
    {
        /* (link_up = 0, tx_enable = 0) or (link_up = 1, tx_enable = 1) */
        zxdh_update_hw_bond_panel_state(tracker, cur_lag, cur_en_dev, false);
    }
    // 5. 更新primary pf的bond_link_info, 用于更新primary pf的vf链路状态
    if (cur_slave->slave_link_change)
        zxdh_hardware_bond_update_link_for_primary(tracker, cur_lag, is_primary_pf_link_up);
    return;
}

static void zxdh_hardware_bond_macaddr_event_process(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct event_node *node)
{
    // 1. 获得触发本次事件的slave端口
    struct slave_state *cur_slave     = &tracker->slaves[node->event_slave_id];
    struct zxdh_en_device *cur_en_dev = cur_slave->slave_info.en_dev;
    struct net_device *cur_netdev         = cur_en_dev->netdev;
    struct slave_state *primary_slave     = &tracker->slaves[cur_lag->primary_pf_idx];
    struct zxdh_en_device *primary_en_dev = primary_slave->slave_info.en_dev;
    struct net_device *primary_netdev     = NULL;

    if (!(primary_slave && primary_slave->slave_info.is_active && primary_slave->slave_info.en_dev))
    {
        return;
    }
    primary_netdev = primary_en_dev->netdev;

    // 2. 将no-primary pf的mac下表到primary pf的转发域中
    if (zxdh_hardware_bond_set_mac_to_primary(cur_en_dev, primary_en_dev, tracker, cur_lag->upper_netdev))
    {
        LOG_ERR("%s update %pM to primary pf %s failed\n", netdev_name(cur_netdev), cur_netdev->dev_addr, netdev_name(primary_netdev));
        return;
    }
    LOG_INFO("%s update %pM to primary pf %s success\n", netdev_name(cur_netdev), cur_netdev->dev_addr, netdev_name(primary_netdev));
    return;
}

void zxdh_lag_init_special_bond_dpp(struct zxdh_lag_tracker *tracker, struct slave_state *cur_slave, int32_t group_ida)
{
    struct zxdh_en_device *cur_en_dev = cur_slave->slave_info.en_dev;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot  = cur_en_dev->slot_id,
        .vport = cur_en_dev->vport,
    };
    uint8_t lag_group_create = 0;
    dpp_lag_hit_flag_get(&dpp_pf_info, group_ida, &lag_group_create);
    LOG_INFO("%s check lag_bond %d hist_flag %d\n", netdev_name(cur_en_dev->netdev), group_ida, lag_group_create);
    if (lag_group_create == 0) // 当某设备触发flr自愈时，lag_group表项hit_flag的值不变，所以无需再次配置
        dpp_lag_group_create(&dpp_pf_info, group_ida);

    if (tracker->tx_type == ZXDH_NETDEV_LAG_TX_TYPE_ACTIVEBACKUP)
    {
        dpp_lag_mode_set(&dpp_pf_info, group_ida, tracker->tx_type);
    }
    else if(tracker->tx_type == ZXDH_NETDEV_LAG_TX_TYPE_HASH)
    {
        dpp_lag_mode_set(&dpp_pf_info, group_ida, tracker->tx_type);
        dpp_lag_group_hash_factor_set(&dpp_pf_info, group_ida, ZXDH_NETDEV_LAG_HASH_L34); /* HASH模式直接写死Layer3+4 */
    }
}

static void zxdh_lag_del_special_bond_dpp(struct slave_state *cur_slave, int32_t group_ida)
{
    struct zxdh_en_device *cur_en_dev = cur_slave->slave_info.en_dev;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot  = cur_en_dev->slot_id,
        .vport = cur_en_dev->vport,
    };
    if (cur_en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
    {
        LOG_INFO("%s state is INTERNAL_ERROR, can't used to delete dpp_lag_group, exit\n", netdev_name(cur_en_dev->netdev));
        return;
    }
    dpp_lag_group_delete(&dpp_pf_info, group_ida);
}

int32_t zxdh_lag_init_special_bond_slave_dpp(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct slave_state *cur_slave)
{
    int32_t ret = 0;
    struct zxdh_en_device *cur_en_dev = cur_slave->slave_info.en_dev;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot  = cur_en_dev->slot_id,
        .vport = cur_en_dev->vport,
    };
    uint16_t ovs_vfid = cur_en_dev->ops->get_ovs_pf_vfid(cur_en_dev->parent);

    if (cur_en_dev->ops->get_dev_type(cur_en_dev->parent) == ZXDH_DEV_ROCE_SPECIAL_BOND)
    {
        ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_LAG_EN_OFF, 1); //lag_id should config 0. because default 0, no separate config and restore
        if (ret != 0)
        {
            LOG_ERR_DEV(cur_en_dev->parent, " dpp_vport_attr_set lag_enable fail,ret: %d\n", ret);
            return ret;
        }
    }
    else
    {
        ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_HW_BOND_EN_OFF, 1);
        if (ret != 0)
        {
            LOG_ERR_DEV(cur_en_dev->parent, "zxdh_update_special_bond_slave dpp_vport_attr_set SRIOV_VPORT_HW_BOND_EN_OFF fail,ret: %d\n", ret);
            return ret;
        }
    }

    dpp_uplink_phy_hardware_bond_set(&dpp_pf_info, cur_en_dev->phy_port, 1);
    dpp_uplink_phy_attr_set(&dpp_pf_info, cur_en_dev->phy_port, UPLINK_PHY_PORT_PF_VQM_VFID, ovs_vfid);
    dpp_uplink_phy_attr_set(&dpp_pf_info, cur_en_dev->phy_port, UPLINK_PHY_PORT_PTP_PORT_VFID, ovs_vfid);
    dpp_uplink_phy_attr_set(&dpp_pf_info, cur_en_dev->phy_port, UPLINK_PHY_PORT_PTP_TC_ENABLE, 0);
    LOG_INFO_DEV(cur_en_dev->parent, "%s slave %s enabled\n", cur_lag->upper_name, netdev_name(cur_en_dev->netdev));
    return 0;
}

static int32_t zxdh_lag_deactive_special_bond_slave_dpp(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct slave_state *cur_slave)
{
    int32_t ret = 0;
    struct zxdh_en_device *cur_en_dev = cur_slave->slave_info.en_dev;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot  = cur_en_dev->slot_id,
        .vport = cur_en_dev->vport,
    };

    if (cur_en_dev->ops->get_dev_type(cur_en_dev->parent) == ZXDH_DEV_ROCE_SPECIAL_BOND)
    {
        ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_LAG_EN_OFF, 0);
        if (ret != 0)
        {
            LOG_ERR(" dpp_vport_attr_set lag_enable fail,ret: %d\n", ret);
            return ret;
        }
    }
    else
    {
        ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_HW_BOND_EN_OFF, 0);
        if (ret != 0)
        {
            LOG_ERR("zxdh_update_special_bond_group dpp_vport_attr_set SRIOV_VPORT_HW_BOND_EN_OFF fail,ret: %d\n", ret);
            return ret;
        }
    }

    dpp_uplink_phy_hardware_bond_set(&dpp_pf_info, cur_en_dev->phy_port, 0);
    dpp_uplink_phy_attr_set(&dpp_pf_info, cur_en_dev->phy_port, UPLINK_PHY_PORT_PF_VQM_VFID, zxdh_convert_pcie_id_2_vfid(cur_en_dev->pcie_id));
    dpp_uplink_phy_attr_set(&dpp_pf_info, cur_en_dev->phy_port, UPLINK_PHY_PORT_PTP_PORT_VFID, zxdh_convert_pcie_id_2_vfid(cur_en_dev->pcie_id));
    dpp_uplink_phy_attr_set(&dpp_pf_info, cur_en_dev->phy_port, UPLINK_PHY_PORT_PTP_TC_ENABLE, 0);
    LOG_INFO("%s slave %s disabled\n", cur_lag->upper_name, netdev_name(cur_en_dev->netdev));
    return 0;
}

static void zxdh_lag_update_special_bond_member_port(struct zxdh_lag_tracker *tracker, struct slave_state *cur_slave, struct event_node *node, bool enable, int32_t group_ida)
{
    struct zxdh_en_device *cur_en_dev = cur_slave->slave_info.en_dev;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot  = cur_en_dev->slot_id,
        .vport = cur_en_dev->vport,
    };

    if (enable)
    {
        dpp_lag_group_member_add(&dpp_pf_info, group_ida, cur_en_dev->phy_port);
    }
    else
    {
        dpp_lag_group_member_del(&dpp_pf_info, group_ida, cur_en_dev->phy_port);
    }

    /* set panel attribute: BOND_LINK_UP */
    dpp_uplink_phy_attr_set(&dpp_pf_info, cur_en_dev->phy_port, UPLINK_PHY_PORT_BOND_LINK_UP, !!enable);

    LOG_INFO_DEV(cur_en_dev->parent, "%s node %d set members: lagid %hhu, phyport %hhu  bond_link_up %s\n",
            netdev_name(cur_en_dev->netdev), node->idx, group_ida, cur_en_dev->phy_port, enable ? "true" : "false");

    return;
}

// 以下函数可以复用于active和update-upper-in事件
static void zxdh_active_special_bond(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct event_node *node)//激活special bond
{
    struct zxdh_en_device *cur_en_dev = NULL;
    struct slave_state *cur_slave = NULL;
    bool lagstat = FALSE;
    cur_slave = &tracker->slaves[node->event_slave_id];
    if (!(cur_slave && cur_slave->slave_info.is_active && cur_slave->slave_info.en_dev))
    {
        return;
    }
    cur_en_dev = cur_slave->slave_info.en_dev;
    if (cur_en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
    {
        LOG_INFO_DEV(cur_en_dev->parent, "lag_dev[%d] %s state is INTERNAL_ERROR, can't active or update special bond, exit\n", cur_lag->idx, netdev_name(cur_en_dev->netdev));
        return;
    }
    lagstat = cur_slave->lower_state.link_up && cur_slave->lower_state.tx_enabled;
    // 下表操作
    // 1. bond相关np表（整个组bond过程仅执行一次）
    zxdh_lag_init_special_bond_dpp(tracker, cur_slave, cur_lag->group_ida);
    // 2. slave相关np表（每个slave加入bond组时执行一次）
    zxdh_lag_init_special_bond_slave_dpp(tracker, cur_lag, cur_slave);
    // 3. 基于slave状态更新lag_group表项
    zxdh_lag_update_special_bond_member_port(tracker, cur_slave, node, lagstat, cur_lag->group_ida);
    return;
}

// 以下函数可以复用于deactive和update-upper-out事件：init_bond_dpp为true，用于active事件；false，用于update-upper-out事件
static void zxdh_deactive_special_bond(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct event_node *node, bool deactive_all)//slave移出special bond或关闭bond
{
    struct slave_state *cur_slave = NULL;
    struct slave_state *last_slave = NULL;
    struct zxdh_en_device *en_dev = NULL;
    int i;
    if (deactive_all)
    {
        LOG_INFO("%s deactive all slave start\n", cur_lag->upper_name);
        // 遍历所有slave端口
        for (i = 0; i < DH_MAX_PORTS; i++)
        {
            cur_slave = &tracker->slaves[i];

            // 第一步：筛选符合条件的slave（必须同时满足以下所有条件）
            // 1. slave指针非空 2. slave处于激活状态 3. en_dev指针有效 4. 支持硬件bond
            if (!(cur_slave && cur_slave->slave_info.is_active && cur_slave->slave_info.en_dev))
            {
                continue;  // 不符合条件，跳过当前slave
            }
            en_dev = cur_slave->slave_info.en_dev;
            if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
            {
                LOG_INFO_DEV(en_dev->parent, "lag_dev[%d] %s state is INTERNAL_ERROR, can't deactive slave dpp, exit\n", cur_lag->idx, netdev_name(en_dev->netdev));
                continue;
            }
            last_slave = cur_slave;
            // 1. 基于slave状态更新lag_group表项
            zxdh_lag_update_special_bond_member_port(tracker, cur_slave, node, FALSE, cur_lag->group_ida);
            // 2. slave相关np表（每个slave移出bond组时执行一次）
            zxdh_lag_deactive_special_bond_slave_dpp(tracker, cur_lag, cur_slave);
        }
        // 3. bond相关np表（整个解除bond过程仅执行一次）
        if (last_slave)
            zxdh_lag_del_special_bond_dpp(last_slave, cur_lag->group_ida);
    }
    else
    {
        if (node->event_slave_id < 0)
        {
            return;
        }
        cur_slave = &tracker->slaves[node->event_slave_id];
        if (!(cur_slave && cur_slave->slave_info.is_active && cur_slave->slave_info.en_dev))
        {
            return;
        }
        en_dev = cur_slave->slave_info.en_dev;
        if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
        {
            LOG_INFO_DEV(en_dev->parent, "lag_dev[%d] %s state is INTERNAL_ERROR, can't deactive slave dpp, exit\n", cur_lag->idx, netdev_name(en_dev->netdev));
            return;
        }
        LOG_INFO_DEV(en_dev->parent, "%s deactive %s start\n", cur_lag->upper_name, netdev_name(en_dev->netdev));
        // 下表操作
        // 1. 基于slave状态更新lag_group表项
        zxdh_lag_update_special_bond_member_port(tracker, cur_slave, node, FALSE, cur_lag->group_ida);
        // 2. slave相关np表（每个slave移出bond组时执行一次）
        zxdh_lag_deactive_special_bond_slave_dpp(tracker, cur_lag, cur_slave);
    }
    return;
}

static void zxdh_lag_special_bond_upper_event_process(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct event_node *node)
{
    if (node->linking) // 新的slave加入bond组
    {
        zxdh_active_special_bond(tracker, cur_lag, node);
    }
    else // slave移出bond组
    {
        zxdh_deactive_special_bond(tracker, cur_lag, node, FALSE); //不删除bond相关np表
    }
    return;
}

static void zxdh_lag_special_bond_lower_event_process(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct event_node *node)
{
    struct slave_state *cur_slave = NULL;
    struct zxdh_en_device *cur_en_dev = NULL;
    bool lagstat = FALSE;
    if (node->event_slave_id < 0)
    {
        return;
    }
    cur_slave = &tracker->slaves[node->event_slave_id];
    if (!(cur_slave && cur_slave->slave_info.is_active && cur_slave->slave_info.en_dev))
    {
        return;
    }
    cur_en_dev = cur_slave->slave_info.en_dev;
    if (cur_en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
    {
        LOG_INFO("lag_dev[%d] %s state is INTERNAL_ERROR, can't process lower event, exit\n", cur_lag->idx, netdev_name(cur_en_dev->netdev));
        return;
    }
    lagstat = cur_slave->lower_state.link_up && cur_slave->lower_state.tx_enabled;
    // 下表操作: 基于slave状态更新lag_group表项
    zxdh_lag_update_special_bond_member_port(tracker, cur_slave, node, lagstat, cur_lag->group_ida);
    return;
}

static void zxdh_update_special_bond(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct event_node *node)//更新special bond
{
    struct slave_state *cur_slave = NULL;
    cur_slave = &tracker->slaves[node->event_slave_id];
    if (!(cur_slave && cur_slave->slave_info.is_active && cur_slave->slave_info.en_dev))
    {
        return;
    }
    switch (node->event)
    {
        case NETDEV_CHANGEUPPER:
        {
            LOG_INFO("%s node %d process %s upper event\n", netdev_name(cur_slave->slave_info.netdev), node->idx, cur_lag->upper_name);
            zxdh_lag_special_bond_upper_event_process(tracker, cur_lag, node);
            break;
        }
        case NETDEV_CHANGELOWERSTATE:
        {
            LOG_INFO("%s node %d process %s lower event\n", netdev_name(cur_slave->slave_info.netdev), node->idx, cur_lag->upper_name);
            zxdh_lag_special_bond_lower_event_process(tracker, cur_lag, node);
            break;
        }
    }
    return;
}

int fid_gen_from_en_dev(struct zxdh_en_device *en_dev, uint32_t *fid_out)
{
    uint32_t fid = 0;

    if (!en_dev)
    {
        LOG_ERR("err ptr, null ptr en_dev.\n");
        return -1;
    }

    fid = ((en_dev->slot_id & 0x0000ffff) << 16);
    fid |= en_dev->pcie_id;

    *fid_out = fid;
    return 0;
}

static void zxdh_set_quad_tor_rdma_switch(struct zxdh_en_device *en_dev, bool is_on)
{
    uint8_t psn_version = 0;
    bool dual_tor = FALSE;
    bool quad_tor = FALSE;
    uint64_t dual_label_addr = 0;

    en_dev->ops->get_psn_feature_info(en_dev->parent, &psn_version, &dual_tor, &quad_tor);
    if (!quad_tor)
    {
        return;
    }

    dual_label_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0) + ZXDH_DUALTOR_LABEL_OFFSET;
    *(uint32_t *)dual_label_addr = is_on? ZXDH_BAR_DUALTOR_LABEL_ON : 0;
    LOG_DEBUG("quad_tor set rdma switch %s.\n", is_on ?  "on" : "off")
    return;
}

static bool is_dev_support_psn(struct zxdh_en_device *en_dev)
{
    uint8_t psn_version = 0;
    bool dual_tor = FALSE;
    bool quad_tor = FALSE;
    uint8_t port_type = 0;

    port_type = en_dev->ops->get_bond_port_type(en_dev->parent);
    en_dev->ops->get_psn_feature_info(en_dev->parent, &psn_version, &dual_tor, &quad_tor);

    if (port_type == BOND_TWO_PORT_TYPE)
    {
        return (psn_version == 0) || ((psn_version > 0) && dual_tor);
    }
    return (psn_version > 0) && quad_tor;
}

static void zxdh_active_psn_bond_dev(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag)
{
    struct slave_state *slave             = NULL;
    struct zxdh_en_device *en_dev         = NULL;
    struct slave_state *primary_slave     = &tracker->slaves[cur_lag->primary_pf_idx];
    struct zxdh_en_device *primary_en_dev = primary_slave->slave_info.en_dev;
    struct bond_ports_info bond_info = {0};
    int slave_num = 0;
    int i = 0;
    uint8_t panel_id_max = 0;

    if (!is_dev_support_psn(primary_en_dev))
    {
        LOG_DEBUG("not support psn while add hwbond %s.\n", cur_lag->upper_name);
        return;
    }

    if (primary_en_dev->ops->get_bond_port_type(primary_en_dev->parent) == BOND_TWO_PORT_TYPE)
    {
        panel_id_max = 2;
    }
    else
    {
        panel_id_max = 4;
    }

    // 遍历所有slave端口
    for (i = 0; i < panel_id_max; i++)
    {
        slave = &tracker->slaves[i];

        // 筛选符合条件的slave（必须同时满足以下所有条件）
        // 1. slave指针非空 2. slave处于激活状态 3. en_dev指针有效 4. 支持硬件bond
        if (!(slave && slave->slave_info.is_active && slave->slave_info.en_dev))
        {
            continue;  // 不符合条件，跳过当前slave
        }
        slave_num++;
        en_dev = slave->slave_info.en_dev;
        if (en_dev->panel_id >= panel_id_max)
        {
            return;
        }

        bond_info.slave_devs[en_dev->panel_id].link_state = ( slave->lower_state.link_up && slave->lower_state.tx_enabled) ? 1 : 0;
        bond_info.slave_devs[en_dev->panel_id].np_port = en_dev->phy_port;
        bond_info.slave_devs[en_dev->panel_id].is_enable = SLAVE_ENABLE;
        fid_gen_from_en_dev(en_dev, &bond_info.slave_devs[en_dev->panel_id].fid);
        LOG_DEBUG("slave1 fid 0x%x states: %u, is_pri: %u, np_port: %u\n", bond_info.slave_devs[en_dev->panel_id].fid,
                                                                        bond_info.slave_devs[en_dev->panel_id].link_state,
                                                                        en_dev->is_primary_port,
                                                                        bond_info.slave_devs[en_dev->panel_id].np_port);
        if (bond_info.slaves_num > panel_id_max)
        {
            return;
        }
    }
    if (slave_num == panel_id_max)
    {
        bond_info.slaves_num = slave_num;
        bond_dev_create_remove_event(cur_lag->upper_name, &bond_info, 1);
        zxdh_set_quad_tor_rdma_switch(primary_en_dev, TRUE);
    }
    else
    {
        LOG_DEBUG("lag_dev[%d]-%s slave_num not equal to panel_id_max %u, can't enable psn\n", cur_lag->idx, cur_lag->upper_name, panel_id_max);
    }
}

static void zxdh_active_hw_bond_panel(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag)
{
    return zxdh_active_psn_bond_dev(tracker, cur_lag);
}

static int32_t zxdh_active_hardware_bond(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct event_node *node)//激活hardware bond
{
    struct slave_state *usable_slave = NULL;
    LOG_INFO("hardware bond active\n");
    // 检查primary_pf是否可用
    if (zxdh_lag_check_primary_pf(tracker, cur_lag->primary_pf_idx))
    {
        zxdh_lag_add_rdma_devices(tracker, -1);
        return -1;
    }
    // 恢复primary_pf的rdma设备，移除其余pf的rdma设备
    tracker->slaves[cur_lag->primary_pf_idx].slave_info.en_dev->is_primary_port = TRUE;
    zxdh_lag_add_rdma_devices(tracker, cur_lag->primary_pf_idx);
    zxdh_lag_remove_rdma_devices_without_primary(tracker, cur_lag->primary_pf_idx);
    // 下表操作
    if (node->from_recover)
    {
        usable_slave = &tracker->slaves[node->event_slave_id];
    }
    else
    {
        usable_slave = zxdh_lag_find_usable_slave(tracker, cur_lag);
    }
    // 1. bond相关np表（整个组bond过程仅执行一次）
    if (usable_slave)
        zxdh_lag_init_bond_dpp(tracker, usable_slave, cur_lag->group_ida);
    // 2. 双平面相关的表项，类似zxdh_create_hw_bond_panel
    zxdh_active_hw_bond_panel(tracker, cur_lag);
    // 3. slave相关np表（每个slave加入bond组时执行一次）
    zxdh_lag_init_hardbond_slave_dpp(tracker, cur_lag, node);
    // 4. 获取bond网络设备速率，更新给rdma驱动
    zxdh_bond_cofig_rdma_speed(cur_lag);
    // 5. primary pf 的 vf link更新
    zxdh_hardware_bond_update_link_for_primary(tracker, cur_lag, TRUE);
    return 0;
}

static void zxdh_update_hardware_bond(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct event_node *node)//更新hardware_bond
{
    struct slave_state *cur_slave = NULL;
    cur_slave = &tracker->slaves[node->event_slave_id];
    if (!(cur_slave && cur_slave->slave_info.is_active && cur_slave->slave_info.en_dev))
    {
        return;
    }
    switch (node->event)
    {
        case NETDEV_CHANGELOWERSTATE:
        {
            LOG_INFO("%s node %d process %s lower event\n", netdev_name(cur_slave->slave_info.netdev), node->idx, cur_lag->upper_name);
            zxdh_hardware_bond_lower_event_process(tracker, cur_lag, node);
            break;
        }
        case NETDEV_CHANGEADDR:
        {
            LOG_INFO("%s node %d process %s mac change event\n", netdev_name(cur_slave->slave_info.netdev), node->idx, cur_lag->upper_name);
            zxdh_hardware_bond_macaddr_event_process(tracker, cur_lag, node);
            break;
        }
    }
    return;
}

static int32_t zxdh_active_bond(struct zxdh_lag_dev *cur_lag, struct event_node *node)//激活bond
{
    int32_t ret = 0;
    if (node->event_slave_id < 0)
    {
        return ret;
    }
    if (node->tracker.target_bond_type == SPECIAL_BOND)
    {
        zxdh_active_special_bond(&node->tracker, cur_lag, node);
    }
    else if(node->tracker.target_bond_type == HARDWARE_BOND)
    {
        ret = zxdh_active_hardware_bond(&node->tracker, cur_lag, node);
    }
    return ret;
}

static void zxdh_update_bond(struct zxdh_lag_dev *cur_lag, struct event_node *node)//更新bond
{

    if (node->event_slave_id < 0)
    {
        return;
    }
    if (cur_lag->tracker.bond_type == SPECIAL_BOND)
    {
        zxdh_update_special_bond(&node->tracker, cur_lag, node);
    }
    else if(cur_lag->tracker.bond_type == HARDWARE_BOND)
    {
        zxdh_update_hardware_bond(&node->tracker, cur_lag, node);
    }
    return;
}

static void zxdh_lag_del_bond_dpp(struct slave_state *usable_slave, int32_t group_ida)
{
    struct zxdh_en_device *en_dev = usable_slave->slave_info.en_dev;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot  = en_dev->slot_id,
        .vport = en_dev->vport,
    };
    dpp_lag_group_delete(&dpp_pf_info, group_ida);
}

static void zxdh_deactive_psn_bond_dev(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct slave_state *usable_slave)
{
    struct bond_ports_info bond_info = {0};
    struct zxdh_en_device *en_dev = usable_slave->slave_info.en_dev;

    if (!is_dev_support_psn(en_dev))
    {
        LOG_DEBUG("not support psn while del hwbond %s.\n", cur_lag->upper_name);
        return;
    }

    bond_dev_create_remove_event(cur_lag->upper_name, &bond_info, 0);
    zxdh_set_quad_tor_rdma_switch(en_dev, FALSE);
    return;
}

static void zxdh_deactive_hardware_bond(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag)
{
    struct slave_state *usable_slave = NULL;
    LOG_INFO("%s deactive start\n", cur_lag->upper_name);
    // 1、slave相关np表（每个slave移出bond组时执行一次）
    zxdh_lag_del_hardbond_slave_dpp(tracker, cur_lag);
    usable_slave = zxdh_lag_find_usable_slave(tracker, cur_lag);
    if (usable_slave)
    {
        // 2、bond相关配置（整个解bond过程仅执行一次）
        zxdh_lag_del_bond_dpp(usable_slave, cur_lag->group_ida);
        // 3、双平面相关配置
        zxdh_deactive_psn_bond_dev(tracker, cur_lag, usable_slave);
    }
    // 4、恢复所有pf的rdma设备
    zxdh_lag_add_rdma_devices(tracker, -1);
    return;
}

static void zxdh_deactive_bond(struct zxdh_lag_dev *cur_lag, struct event_node *node)//关闭bond
{
    if (cur_lag->tracker.bond_type == SPECIAL_BOND)
    {
        zxdh_deactive_special_bond(&node->tracker, cur_lag, node, TRUE);
    }
    else if(cur_lag->tracker.bond_type == HARDWARE_BOND)
    {
        zxdh_deactive_hardware_bond(&node->tracker, cur_lag);
    }
    return;
}

static void zxdh_do_hardware_bond_work(struct work_struct *work)
{
    struct delayed_work *delayed_work = to_delayed_work(work);
    struct event_ctx *ctx             = container_of(delayed_work, struct event_ctx, bond_work);
    struct zxdh_lag_dev *cur_lag      = container_of(ctx, struct zxdh_lag_dev, ctx);
    struct event_node *node, *tmp;
    bool do_bond = FALSE;
    LIST_HEAD(local_list);  // 临时链表
    // 原子化转移队列内容到临时链表
    spin_lock(&ctx->lock);
    list_splice_init(&ctx->event_list, &local_list);  // 切割整个队列
    spin_unlock(&ctx->lock);
    LOG_DEBUG("lag_dev[%d] enter zxdh_do_hardware_bond_work\n", cur_lag->idx);
    mutex_lock(&cur_lag->mlock);
    LOG_INFO("lag_dev[%d] success get mlock\n", cur_lag->idx);
    // 按顺序处理临时链表中的每个事件
    list_for_each_entry_safe(node, tmp, &local_list, list)
    {
        list_del(&node->list);  // 从临时链表移除
        LOG_INFO("%s node %d addr %p get from list, event %ld linking %d\n",
                 netdev_name(node->event_netdev), node->idx, (void *)node, node->event, node->linking);

        do_bond = node->tracker.is_bonded;
        if (do_bond && (!cur_lag->is_active || node->from_recover))  // 可以做硬bond/special bond，但是还没激活，需要激活
        {
            // lag_id使用本地的，避免并发
            if (zxdh_lag_request_lag_id(cur_lag, node) < 0)
            {
                LOG_ERR("lag_dev[%d] zxdh_lag_request_lag_id failed\n", cur_lag->idx);
                goto out;
            }
            if(zxdh_active_bond(cur_lag, node))
            {
                zxdh_lag_release_lag_id(cur_lag);
                goto out;
            }
            cur_lag->tracker.bond_type = node->tracker.target_bond_type;;
            cur_lag->is_active = TRUE;
            LOG_INFO("%s %s-lag_dev[%d]-group_id[%d] active success\n", netdev_name(node->event_netdev), cur_lag->upper_name, cur_lag->idx, cur_lag->group_ida);
        }
        else if(do_bond && cur_lag->is_active)  // 可以做硬bond/special bond，且已经激活，需要更新
        {
            zxdh_update_bond(cur_lag, node);
            LOG_INFO("%s %s-lag_dev[%d]-group_id[%d] update success\n", netdev_name(node->event_netdev), cur_lag->upper_name, cur_lag->idx, cur_lag->group_ida);
        }
        else if(!do_bond && cur_lag->is_active)  // 关闭硬bond
        {
            zxdh_deactive_bond(cur_lag, node);
            cur_lag->is_active = FALSE;
            zxdh_lag_release_lag_id(cur_lag);
            cur_lag->tracker.bond_type = node->tracker.target_bond_type;
            LOG_INFO("%s %s-lag_dev[%d]-group_id[%d] deactive success\n", netdev_name(node->event_netdev), cur_lag->upper_name, cur_lag->idx, cur_lag->group_ida);
        }
        out:
        LOG_INFO("%s node %d addr %p has been processed\n", node->event_netdev->name, node->idx, (void *)node);
        kfree(node);
    }
    mutex_unlock(&cur_lag->mlock);
    LOG_INFO("lag_dev[%d] release mlock\n", cur_lag->idx);
}

static void zxdh_queue_hardware_bond_work(struct zxdh_lag_dev *curr_lag, struct event_ctx *ctx, unsigned long delay)
{
    queue_delayed_work(curr_lag->wq, &ctx->bond_work, delay);
}

static bool zxdh_lag_contains_en_dev(struct zxdh_lag_dev *curr_lag, struct zxdh_en_device *en_dev)
{
    struct slave_state *slave = &curr_lag->tracker.slaves[en_dev->panel_id];
    return slave->slave_info.is_active && slave->slave_info.en_dev == en_dev && en_dev->ldev == curr_lag;
}

/* Forward declaration */
extern uint16_t zxdh_update_slave_bit_by_panel(uint16_t cur_slaves, uint8_t panel_id, uint8_t value);

/**
 * zxdh_lag_count_bond_slaves - Count slave statistics from bond device
 * @ldev: LAG device
 * @upper_dev: Upper bond net device
 * @stats: Output structure to hold slave statistics
 *
 * Count various slave statistics from the bond device including:
 * - stats.same_ldev_slaves: DH card slave count
 * - stats.hardbond_slaves: Hardware-bond enabled slave count
 * - stats.total_slaves: Total slave count in kernel bond
 * - stats.specbond_slaves: Special-bond slave count
 * - cur_slaves: Current slave configuration (bitmask)
 */
void zxdh_lag_count_bond_slaves(struct zxdh_lag_dev *ldev,
                                 struct net_device *upper_dev,
                                 struct zxdh_bond_slave_stats *stats)
{
    struct net_device *ndev_tmp = NULL;
    struct zxdh_en_priv *tmp_en_priv = NULL;
    struct zxdh_en_device *tmp_en_dev = NULL;

    zte_memset_s(stats, 0, sizeof(*stats));

    rcu_read_lock();
    for_each_netdev_in_bond_rcu(upper_dev, ndev_tmp)
    {
        if (netif_is_zxdh_aux(ndev_tmp))
        {
            tmp_en_priv = netdev_priv(ndev_tmp);
            tmp_en_dev = &tmp_en_priv->edev;
            if (zxdh_lag_contains_en_dev(ldev, tmp_en_dev))
            {
                stats->same_ldev_slaves++;
                stats->cur_slaves = zxdh_update_slave_bit_by_panel(stats->cur_slaves,
                                                                   tmp_en_dev->panel_id, 1);
                if (zxdh_netdev_is_hwbond(ndev_tmp))
                    stats->hardbond_slaves++;
                if (zxdh_netdev_is_special_bond(ndev_tmp))
                    stats->specbond_slaves++;
            }
        }
        stats->total_slaves++;
    }
    rcu_read_unlock();
}

static bool zxdh_lag_linked_to_bond_netdev(struct zxdh_lag_dev *curr_lag, struct net_device *bond_netdev)
{
    return curr_lag->state == LAG_DEV_ACTIVE && curr_lag->upper_netdev && curr_lag->upper_netdev == bond_netdev;
}

static bool zxdh_slave_belong_to_bond(struct net_device *netdev, struct net_device *bond_netdev)
{
    struct net_device *tmp_netdev = NULL;
    bool find = FALSE;
    rcu_read_lock();
    for_each_netdev_in_bond_rcu(bond_netdev, tmp_netdev)
    {
        if (tmp_netdev == netdev)
        {
            find = TRUE;
            break;
        }
    }
    rcu_read_unlock();
    return find;
}

/**
 * zxdh_lag_update_bond_type - 根据slave状态判断bond类型
 * @same_ldev_slaves: 同ldev的slave数量
 * @hardbond_slaves: 开启hardware-bond的slave数量
 * @total_slaves: 总slave数量
 * @specbond_slaves: special-bond的slave数量
 * @cur_slaves: 当前加入内核bond的slave数
 * @expect_slaves: 期望加入内核bond的slave数
 *
 * 返回：匹配的bond类型，异常返回SOFTWARE_BOND（兜底）
 */
static enum zxdh_bond_type zxdh_lag_update_bond_type(uint16_t same_ldev_slaves,
                                                     uint16_t hardbond_slaves,
                                                     uint16_t total_slaves,
                                                     uint16_t specbond_slaves,
                                                     uint16_t cur_slaves,
                                                     uint16_t expect_slaves)
{
    const bool is_all_same_ldev = (total_slaves == same_ldev_slaves);
    const bool is_cur_match_expect = (cur_slaves == expect_slaves);
    const bool is_hardbond_full = (same_ldev_slaves == hardbond_slaves);
    const bool is_specbond_full = (same_ldev_slaves == specbond_slaves);
    const bool is_slave_empty = total_slaves == 0;

    LOG_INFO("total_slaves %d, same_ldev_slaves %d, cur_slaves 0x%x, expect_slaves 0x%x, hardbond_slaves %d, specbond_slaves %d\n", \
    total_slaves, same_ldev_slaves, cur_slaves, expect_slaves, hardbond_slaves, specbond_slaves);

    // 判断SOFTWARE_BOND
    if (is_slave_empty || (same_ldev_slaves == 0)) {
        // 子条件1：无任何slave
        LOG_INFO("return software_bond: bond has no slave\n");
        return SOFTWARE_BOND;
    }

    // 判断SPECIAL_BOND: 同ldev且全部是special_bond
    if (is_specbond_full && specbond_slaves != 0) {
        LOG_INFO("return special_bond\n");
        return SPECIAL_BOND;
    }

    if (!is_all_same_ldev) {
        // 存在不同ldev 或 不同DH卡的slave
        LOG_INFO("return software_bond: bond has different ldev or different dh card slave\n");
        return SOFTWARE_BOND;
    }


    if (!is_cur_match_expect) {
        // 同ldev但slave未全部加入内核bond
        LOG_INFO("return software_bond: bond has not all ldev slave in kernel bond, cur_slaves 0x%x vs expect_slaves 0x%x\n", cur_slaves, expect_slaves);
        return SOFTWARE_BOND;
    }
    if (!is_hardbond_full) {
        LOG_INFO("return software_bond: bond has all ldev slave in kernel bond, but not all hardware bond\n");
        // 同ldev且全加入内核bond，但hardware-bond未全开
        return SOFTWARE_BOND;
    }

    // 判断HARDWARE_BOND
    if (is_hardbond_full && is_cur_match_expect) {
        if (same_ldev_slaves >= 2) // 网卡硬bond组配置需要大于等于2个slave，才能开启hardbond
        {
            LOG_INFO("return hardware_bond: bond has all ldev slave in kernel bond and hardware bond\n");
            return HARDWARE_BOND;
        }
        else
        {
            LOG_INFO("return software_bond: bond has all ldev slave in kernel bond and hardware bond, but slave num < 2\n");
            return SOFTWARE_BOND;
        }
    }

    // 异常场景（理论上不会走到）
    LOG_INFO("unexpected bond type, total=%u same_ldev=%u hardbond=%u\n",
            total_slaves, same_ldev_slaves, hardbond_slaves);
    return SOFTWARE_BOND;
}

/**
 * zxdh_lag_has_any_standby_vf_enabled - Check if any standby port has VF enabled
 * @tracker: bond tracker
 * @primary_panel_id: panel_id of primary port
 *
 * Check if any standby port (non-primary port) has VF enabled
 * Common function used by both bond creation and VF disable scenarios
 *
 * Return: true if any standby port has VF enabled, false otherwise
 */
bool zxdh_lag_has_any_standby_vf_enabled(struct zxdh_lag_tracker *tracker, uint8_t primary_panel_id)
{
    struct slave_state *slave;
    struct zxdh_en_device *en_dev;
    struct pci_dev *pdev;
    int i;
    int num_vfs;

    /* Traverse all slaves to check if any standby port has VF enabled */
    for (i = 0; i < DH_MAX_PORTS; i++)
    {
        slave = &tracker->slaves[i];
        en_dev = slave->slave_info.en_dev;

        if (!en_dev || !en_dev->parent || !en_dev->ops)
            continue;

        /* Skip primary port */
        if (en_dev->panel_id == primary_panel_id)
            continue;

        /* Skip inactive slaves */
        if (!slave->slave_info.is_active)
            continue;

        /* Get VF count using PCI layer function */
        pdev = en_dev->ops->get_pdev(en_dev->parent);
        if (!pdev)
            continue;

        num_vfs = pci_num_vf(pdev);

        LOG_INFO("Checking standby port[%d]: panel_id=%d, num_vfs=%d\n",
                 i, en_dev->panel_id, num_vfs);

        /* Check if this standby port has VF enabled */
        if (num_vfs > 0)
        {
            LOG_INFO("Standby port (panel_id=%d) has %d VFs enabled\n",
                     en_dev->panel_id, num_vfs);
            return true;  /* Found standby port with VF enabled */
        }
    }

    LOG_INFO("No standby port has VF enabled\n");
    return false;  /* No standby port has VF enabled */
}

/**
 * @brief 根据panel_id更新cur_slaves中对应bit位的值
 * @param cur_slaves 待更新的16位无符号整数（bit0~bit9对应panel_id0~9）
 * @param panel_id 要更新的bit位序号（有效范围：0~9，超出则返回原cur_slaves）
 * @param value 要设置的bit值（非0视为1，0视为0）
 * @return uint16_t 更新后的16位无符号整数；若panel_id无效，返回原cur_slaves
 */
uint16_t zxdh_update_slave_bit_by_panel(uint16_t cur_slaves, uint8_t panel_id, uint8_t value)
{
    // 1. 归一化value：bit位仅支持0/1，非0值统一视为1
    uint8_t bit_value = (value != 0) ? 1 : 0;

    // 2. 校验panel_id有效性：uint16_t仅16位，panel_id需在0~15范围内
    if (panel_id >= DH_MAX_PORTS)
    {
        return cur_slaves; // 超出范围，直接返回原值
    }


    // 3. 清除cur_slaves中panel_id对应的bit位（先置0，避免原有值干扰）
    // 掩码：~(1U << panel_id) → 仅目标bit位为0，其余为1
    cur_slaves &= ~(1U << panel_id);

    // 4. 设置目标bit位为指定值（bit_value左移后与cur_slaves做或运算）
    if (bit_value == 1)
    {
        cur_slaves |= (1U << panel_id);
    }

    return cur_slaves;
}

static void zxdh_lag_sync_ipv6_to_slave(struct zxdh_lag_tracker *tracker,
    struct zxdh_lag_dev *cur_lag, struct net_device *event_netdev, bool is_link)
{
    struct zxdh_en_priv *event_en_priv  = NULL;
    struct zxdh_en_device *event_en_dev = NULL;
    struct slave_state *slave = NULL;

    event_en_priv = netdev_priv(event_netdev);
    event_en_dev  = &event_en_priv->edev;

    slave = &tracker->slaves[event_en_dev->panel_id];

    if (!(slave && slave->slave_info.is_active && slave->slave_info.en_dev))
    {
        return;
    }

    bond_ipv6_mcast_update_slave(cur_lag->upper_netdev, event_en_dev, is_link);
    return;
}

static int32_t zxdh_changeupper_event_handler(struct zxdh_lag_dev *curr_lag, struct net_device *event_netdev, void *ptr, bool *linking)
{
    struct netdev_notifier_changeupper_info *info = (struct netdev_notifier_changeupper_info *)ptr;
    struct net_device *upper_dev                  = NULL;
    struct zxdh_lag_tracker *tracker              = &curr_lag->tracker;
    struct zxdh_bond_slave_stats stats;
    bool is_bonded, is_in_lag, mode_supported;
    int32_t change = 0;
    struct zxdh_en_priv *tmp_en_priv  = NULL;
    struct zxdh_en_device *tmp_en_dev = NULL;
    bool is_en_dev_in_lag, is_upper_linked_to_lag;
    enum zxdh_bond_type target_bond_type = SOFTWARE_BOND;

    /* upper_dev can be NULL in some cases, check it first */
    if (!info->upper_dev)
    {
        LOG_INFO("upper_dev is NULL, ignore event for %s\n", event_netdev->name);
        return 0;
    }

    if (!netif_is_lag_master(info->upper_dev))
    {
        return 0;
    }
    upper_dev = info->upper_dev;
    is_upper_linked_to_lag = zxdh_lag_linked_to_bond_netdev(curr_lag, upper_dev);
    if (is_upper_linked_to_lag)
    {
        LOG_DEBUG("lag[%d] is already linked to %s\n", curr_lag->idx, upper_dev->name);
        goto check_bond;
    } else {
        if(!netif_is_zxdh_aux(event_netdev))
        {
            return 0;
        }
    }

    *linking = info->linking;

    tmp_en_priv = netdev_priv(event_netdev);
    tmp_en_dev  = &tmp_en_priv->edev;
    is_en_dev_in_lag = zxdh_lag_contains_en_dev(curr_lag, tmp_en_dev);
    if (is_en_dev_in_lag)
    {
        // en_dev属于lag，且lag未关联bond → 激活lag并绑定upper_netdev
        curr_lag->state = LAG_DEV_ACTIVE;
        curr_lag->upper_netdev = upper_dev;
        zte_strncpy_s(curr_lag->upper_name, dev_name(&upper_dev->dev), IFNAMSIZ - 1);
        curr_lag->upper_name[IFNAMSIZ - 1] = '\0';
        LOG_DEBUG("%s is belong to lag_dev[%d], activate lag_dev with %s\n",
                event_netdev->name, curr_lag->idx, upper_dev->name);
    } else {
        LOG_DEBUG("%s is not belong to lag_dev[%d](it's not link to %s), exit\n",
                event_netdev->name, curr_lag->idx, upper_dev->name);
        // en_dev不属于lag，且lag未关联bond → 直接返回
        return 0;
    }

check_bond:
    LOG_INFO("%s %s %s\n", event_netdev->name, info->linking ? "LINK" : "UNLINK", upper_dev->name);
    if(netif_is_zxdh_aux(event_netdev))
    {
        zxdh_lag_sync_ipv6_to_slave(tracker, curr_lag, event_netdev, info->linking);
    }
    zxdh_lag_count_bond_slaves(curr_lag, upper_dev, &stats);

    if (stats.same_ldev_slaves == 0)
    { // bond中没有包含 ldev 的任何网口
        curr_lag->state = LAG_DEV_IDLE;
        LOG_INFO("%s don't contain slave belong to lag_dev[%d], reset lag_dev[%d] state to IDLE\n", upper_dev->name, curr_lag->idx, curr_lag->idx);
    }

    if (info->linking)
    {
        tracker->tx_type =
            (uint8_t)zxdh_covert_bond_tx_type(((struct netdev_lag_upper_info *)info->upper_info)->tx_type);
#ifdef CGS_V5_693
        /* CGS_V5_693 (3.10.0-693) kernel: netdev_lag_upper_info has no hash_type member, use default value */
        tracker->hash_type = ZXDH_NETDEV_LAG_HASH_UNKNOWN;
#else
        tracker->hash_type =
            (uint8_t)zxdh_covert_hash_type(((struct netdev_lag_upper_info *)info->upper_info)->hash_type);
#endif
    }

    /* Lag mode must be activebackup or hash. */
    mode_supported =
        tracker->tx_type == ZXDH_NETDEV_LAG_TX_TYPE_ACTIVEBACKUP || tracker->tx_type == ZXDH_NETDEV_LAG_TX_TYPE_HASH;

    /* Check if standby port has VF enabled (scenario: VF enabled first, then bond created) */
    if (zxdh_lag_has_any_standby_vf_enabled(tracker, curr_lag->primary_pf_idx))
    {
        /* Standby port has VF enabled: force software bond */
        target_bond_type = SOFTWARE_BOND;
        LOG_INFO("Standby port has VF enabled, force SOFTWARE_BOND\n");
    }
    else
    {
        /* Standby port has no VF: use original logic to determine bond type */
        target_bond_type = zxdh_lag_update_bond_type(stats.same_ldev_slaves, stats.hardbond_slaves, stats.total_slaves, stats.specbond_slaves, stats.cur_slaves, curr_lag->expect_slaves);
    }

    is_in_lag = (target_bond_type == SPECIAL_BOND || target_bond_type == HARDWARE_BOND);
    is_bonded = is_in_lag && mode_supported;
    LOG_INFO("%s has %d slave, target_bond_type is %d (vs lag_dev[%d]'s target_bond_type %d), is_bonded is %d\n", upper_dev->name, stats.total_slaves, target_bond_type, curr_lag->idx, tracker->target_bond_type, is_bonded);
    if (tracker->is_bonded != is_bonded || target_bond_type == SPECIAL_BOND)
    {
        LOG_INFO("lag_dev[%d]->target_is_bonded need change from %d to %d\n", curr_lag->idx, tracker->is_bonded, is_bonded);
        tracker->is_bonded = is_bonded;
        tracker->target_bond_type = target_bond_type;
        change = 1;
    }

    return change;
}

static int32_t zxdh_changelowerstate_event_handler(struct zxdh_lag_dev *curr_lag, struct net_device *event_netdev, void *ptr)
{
    struct netdev_lag_lower_state_info *lag_lower_info;
    struct slave_state *cur_slave_state;
    struct netdev_notifier_changelowerstate_info *info;
    int32_t change = 0;
    struct zxdh_en_priv *tmp_en_priv  = NULL;
    struct zxdh_en_device *tmp_en_dev = NULL;

    if (!netif_is_lag_port(event_netdev))
    {
        return 0;
    }

    info = (struct netdev_notifier_changelowerstate_info *)ptr;
    lag_lower_info = info->lower_state_info;
    if (!lag_lower_info)
    {
        return 0;
    }

    if(!netif_is_zxdh_aux(event_netdev)) // event_netdev不属于DH网卡
    {
        return 0;
    }

    tmp_en_priv = netdev_priv(event_netdev);
    tmp_en_dev  = &tmp_en_priv->edev;

    // 判断触发本次事件的slave是否属于处理当前事件的zxdh_lag
    if (!zxdh_lag_contains_en_dev(curr_lag, tmp_en_dev))
    {
        // LOG_INFO("slave %s is not belong to lag_dev[%d]\n", event_netdev->name, curr_lag->idx);
        return 0;
    }

    cur_slave_state = &curr_lag->tracker.slaves[tmp_en_dev->panel_id];
    /* check if lower device state changed */
    if (zxdh_is_lower_state_change(&cur_slave_state->lower_state, lag_lower_info))
    {
        change = 1;
        cur_slave_state->slave_link_change =
            (cur_slave_state->lower_state.link_up != lag_lower_info->link_up) ? TRUE : FALSE;
    }
    LOG_INFO("%s change: %d, link up: %hhu -> %hhu, tx enable %hhu -> %hhu, slave_link_change %d\n", event_netdev->name,
             change, cur_slave_state->lower_state.link_up, lag_lower_info->link_up,
             cur_slave_state->lower_state.tx_enabled, lag_lower_info->tx_enabled, cur_slave_state->slave_link_change);

    cur_slave_state->lower_state.link_up    = lag_lower_info->link_up;
    cur_slave_state->lower_state.tx_enabled = lag_lower_info->tx_enabled;  //主备模式下，当硬bond未组成时，bond会下发lower事件，等硬bond组成后，bond可能不会再下发lower事件，因此无法更新lag_bond表

    if (!(curr_lag->state == LAG_DEV_ACTIVE && curr_lag->upper_netdev))
    {
        LOG_DEBUG("lag_dev[%d] is not active, exit\n", curr_lag->idx);
        return 0;
    }
    // 判断触发本次事件的slave是否属于处理当前事件的zxdh_lag关联的bond
    if (!zxdh_slave_belong_to_bond(event_netdev, curr_lag->upper_netdev))
    {
        // LOG_INFO("slave %s is not belong to %s(lag_dev[%d])\n", event_netdev->name, curr_lag->upper_netdev->name, curr_lag->idx);
        return 0;
    }

    // 判断处理本次事件的zxdh_lag是否为软bond
    if (curr_lag->tracker.target_bond_type == SOFTWARE_BOND)
    {
        LOG_DEBUG("lag_dev[%d] is SOFTWARE_BOND, exit\n", curr_lag->idx);
        return 0;
    }

    if (!curr_lag->tracker.is_bonded)
    {
        LOG_INFO("lag_dev[%d] isn't bonded, exit\n", curr_lag->idx);
        return 0;
    }
    return change;
}

static int32_t zxdh_changeaddr_event_handler(struct zxdh_lag_dev *curr_lag, struct net_device *event_netdev)
{
    struct slave_state *cur_slave_state;
    int32_t change = 0;
    struct zxdh_en_priv *tmp_en_priv  = NULL;
    struct zxdh_en_device *tmp_en_dev = NULL;

    if (!netif_is_lag_port(event_netdev))
    {
        return 0;
    }

    if(!netif_is_zxdh_aux(event_netdev)) // event_netdev不属于DH网卡
    {
        return 0;
    }

    tmp_en_priv = netdev_priv(event_netdev);
    tmp_en_dev  = &tmp_en_priv->edev;

    if (!(curr_lag->state == LAG_DEV_ACTIVE && curr_lag->upper_netdev))
    {
        LOG_DEBUG("lag_dev[%d] is not active\n", curr_lag->idx);
        return 0;
    }

    if (!curr_lag->tracker.is_bonded)
    {
        LOG_INFO("lag_dev[%d] isn't bonded, exit\n", curr_lag->idx);
        return 0;
    }

    // 判断触发本次事件的slave是否属于处理当前事件的zxdh_lag
    if (!zxdh_lag_contains_en_dev(curr_lag, tmp_en_dev))
    {
        // LOG_INFO("slave %s is not belong to lag_dev[%d]\n", event_netdev->name, curr_lag->idx);
        return 0;
    }

    // 判断触发本次事件的slave是否属于处理当前事件的zxdh_lag关联的bond
    if (!zxdh_slave_belong_to_bond(event_netdev, curr_lag->upper_netdev))
    {
        // LOG_INFO("slave %s is not belong to lag_dev[%d] (bond %s)\n", event_netdev->name, curr_lag->idx, curr_lag->upper_netdev->name);
        return 0;
    }

    // special_bond和软bond无需处理mac变化事件
    if (curr_lag->tracker.target_bond_type == SPECIAL_BOND || curr_lag->tracker.target_bond_type == SOFTWARE_BOND)
    {
        LOG_DEBUG("don't neet exec zxdh_changeaddr_event_handler when netdev %s is special_bond or software bond\n",
                 event_netdev->name);
        return 0;
    }

    // 只有主备模式才需要处理mac变化事件
    if (curr_lag->tracker.tx_type != ZXDH_NETDEV_LAG_TX_TYPE_ACTIVEBACKUP)
    {
        LOG_DEBUG("tracker %d tx_type is not ZXDH_NETDEV_LAG_TX_TYPE_ACTIVEBACKUP\n", curr_lag->idx);
        return 0;
    }

    cur_slave_state = &curr_lag->tracker.slaves[tmp_en_dev->panel_id];
    /* check if lower device state changed */
    if (!memcmp(event_netdev->dev_addr, tmp_en_dev->last_mac_addr.sa_data, event_netdev->addr_len))  // 判断mac地址是否真的发生变化
    { // mac地址无变化
        cur_slave_state->slave_mac_change = FALSE;
        change = 0;
        LOG_INFO("slave %s mac no changed %d\n", event_netdev->name, cur_slave_state->slave_mac_change);
    }
    else
    {
        cur_slave_state->slave_mac_change = TRUE;
        change = 1;
        LOG_INFO("slave %s mac changed %d\n", event_netdev->name, cur_slave_state->slave_mac_change);
    }
    return change;
}

static void zxdh_update_event_slave_id_to_node(struct event_node *node, struct net_device *event_netdev, struct zxdh_lag_dev *curr_lag)
{
    struct zxdh_en_priv *event_en_priv = NULL;
    struct zxdh_en_device *event_en_dev = NULL;
    // 只需处理属于当前lag的网口bond事件
    node->event_netdev = event_netdev;
    if(!netif_is_zxdh_aux(event_netdev)) // event_netdev不属于DH网卡
    {
        node->event_slave_id = -1;
        return;
    }
    event_en_priv = netdev_priv(event_netdev);
    event_en_dev  = &event_en_priv->edev;
    if (zxdh_lag_contains_en_dev(curr_lag, event_en_dev))
    {
        node->event_slave_id = event_en_dev->panel_id;
    }
    else
    {
        node->event_slave_id = -1;
    }
    return;
}

static int zxdh_hardware_bond_event_handler(struct notifier_block *nb, unsigned long event, void *ptr)
{
    struct zxdh_lag_dev *curr_lag    = container_of(nb, struct zxdh_lag_dev, nb);
    struct net_device *event_netdev  = netdev_notifier_info_to_dev(ptr);
    int32_t changed                  = 0;
    struct event_ctx *ctx            = NULL;
    struct event_node *node          = NULL;
    bool linking = TRUE;
    const char *event_str = NULL;
    struct ethtool_link_ksettings ks = {0};
#ifdef CGS_V5_693
    struct ethtool_cmd ecmd = {0};
#endif

    if (curr_lag->state == LAG_DEV_INVALID)
    {
        LOG_INFO("lag %d is invalid, don't need to process\n", curr_lag->idx);
        return NOTIFY_DONE;
    }

    /* Check that the netdev is in the working namespace */
    if (!net_eq(dev_net(event_netdev), &init_net))
    {
        return NOTIFY_DONE;
    }
    switch (event)
    {
        case NETDEV_CHANGEUPPER:
        {
            event_str = "NETDEV_CHANGEUPPER";
            changed = zxdh_changeupper_event_handler(curr_lag, event_netdev, ptr, &linking);
            break;
        }
        case NETDEV_CHANGELOWERSTATE:
        {
            event_str = "NETDEV_CHANGELOWERSTATE";
            changed = zxdh_changelowerstate_event_handler(curr_lag, event_netdev, ptr);
            break;
        }
        case NETDEV_CHANGEADDR:
        {
            event_str = "NETDEV_CHANGEADDR";
            changed = zxdh_changeaddr_event_handler(curr_lag, event_netdev);
            break;
        }
        default:
        {
            event_str = "UNKNOWN_EVENT";
            changed = 0;
            break;
        }
    }
    if (changed)
    {
        ctx  = &(curr_lag->ctx);
        node = kmalloc(sizeof(*node), GFP_ATOMIC);
        if (!node)
        {
            LOG_ERR("Failed to allocate event node!\n");
            return NOTIFY_OK;
        }
        node->event = event;
        node->linking = linking;
        node->tracker = curr_lag->tracker;
        node->from_recover = FALSE;
        zxdh_update_event_slave_id_to_node(node, event_netdev, curr_lag);
        // 将节点加入队列
        spin_lock(&ctx->lock);
        node->idx = ctx->idx;
        ctx->idx++;
        list_add_tail(&node->list, &ctx->event_list);  // 添加到队尾
        LOG_INFO("%s node %d addr %p add to list, event %s linking %d \n", event_netdev->name,
                node->idx, (void *)node, event_str, node->linking);
        spin_unlock(&ctx->lock);
        if (curr_lag->upper_netdev->ethtool_ops)
        {
        // 获取bond口速率,非自定义设备
#ifdef CGS_V5_693
        /* CGS_V5_693 (3.10.0-693) kernel: 使用 get_settings 接口 */
        if (curr_lag->upper_netdev->ethtool_ops->get_settings)
        {
            ecmd.cmd = ETHTOOL_GSET;
            curr_lag->upper_netdev->ethtool_ops->get_settings(curr_lag->upper_netdev, &ecmd);
            ks.base.speed = (ecmd.speed_hi << 16) | ecmd.speed;
        }
#else
        curr_lag->upper_netdev->ethtool_ops->get_link_ksettings(curr_lag->upper_netdev, &ks);
#endif
        }
        curr_lag->bond_speed = ks.base.speed;
        zxdh_queue_hardware_bond_work(curr_lag, ctx, 0);
    }
    return NOTIFY_OK;
}

/**
 * zxdh_lag_change_bond_type_common - Common function to change bond type
 * @curr_lag: LAG device
 * @tracker: bond tracker
 * @en_dev: EN device triggering the change
 * @target_bond_type: target bond type (SOFTWARE_BOND or HARDWARE_BOND)
 *
 * Common function to create and queue an event node for bond type conversion.
 *
 * Return: 0 on success, negative on failure
 */
static int32_t zxdh_lag_change_bond_type_common(struct zxdh_lag_dev *curr_lag,
                                                 struct zxdh_lag_tracker *tracker,
                                                 struct zxdh_en_device *en_dev,
                                                 enum zxdh_bond_type target_bond_type)
{
    struct event_ctx *ctx = NULL;
    struct event_node *node = NULL;
    int linking;

    ctx = &(curr_lag->ctx);
    node = kmalloc(sizeof(*node), GFP_ATOMIC);
    if (!node)
    {
        LOG_ERR("Failed to allocate event node!\n");
        return -1;
    }

    /* Set parameters based on target bond type */
    if (target_bond_type == HARDWARE_BOND)
    {
        linking = 1;
        tracker->is_bonded = TRUE;
        tracker->target_bond_type = HARDWARE_BOND;
    }
    else
    {
        linking = 0;
        tracker->is_bonded = FALSE;
        tracker->target_bond_type = SOFTWARE_BOND;
    }

    node->event = NETDEV_CHANGEUPPER;
    node->linking = linking;
    node->tracker = curr_lag->tracker;
    node->event_slave_id = en_dev->panel_id;
    node->event_netdev = en_dev->netdev;
    node->from_recover = FALSE;

    /* Add node to event queue */
    spin_lock(&ctx->lock);
    node->idx = ctx->idx;
    ctx->idx++;
    list_add_tail(&node->list, &ctx->event_list);
    LOG_INFO("%s node %d addr %p added to list, event NETDEV_CHANGEUPPER linking %d for %s\n",
            netdev_name(en_dev->netdev), node->idx, (void *)node, linking,
            target_bond_type == HARDWARE_BOND ? "HARDWARE_BOND" : "SOFTWARE_BOND");
    spin_unlock(&ctx->lock);

    zxdh_queue_hardware_bond_work(curr_lag, ctx, 0);
    return 0;
}

/**
 * zxdh_lag_change_to_software_bond - Change bond type from hardware to software
 * @curr_lag: LAG device
 * @tracker: bond tracker
 * @en_dev: EN device triggering the change
 *
 * Convert hardware bond to software bond by creating and queuing an event node.
 *
 * Return: 0 on success, negative on failure
 */
int32_t zxdh_lag_change_to_software_bond(struct zxdh_lag_dev *curr_lag, struct zxdh_lag_tracker *tracker, struct zxdh_en_device *en_dev)
{
    return zxdh_lag_change_bond_type_common(curr_lag, tracker, en_dev, SOFTWARE_BOND);
}
EXPORT_SYMBOL(zxdh_lag_change_to_software_bond);

/**
 * zxdh_lag_change_to_hardware_bond - Change bond type from software to hardware
 * @curr_lag: LAG device
 * @tracker: bond tracker
 * @en_dev: EN device triggering the change
 *
 * Convert software bond to hardware bond by creating and queuing an event node.
 *
 * Return: 0 on success, negative on failure
 */
int32_t zxdh_lag_change_to_hardware_bond(struct zxdh_lag_dev *curr_lag, struct zxdh_lag_tracker *tracker, struct zxdh_en_device *en_dev)
{
    return zxdh_lag_change_bond_type_common(curr_lag, tracker, en_dev, HARDWARE_BOND);
}
EXPORT_SYMBOL(zxdh_lag_change_to_hardware_bond);

/**
 * zxdh_lag_update_primary_port_hwbond - Update primary port's is_hwbond status
 * @ldev: LAG device
 * @en_dev: Current EN device that triggered the update
 * @is_hwbond: New is_hwbond value (TRUE or FALSE)
 *
 * Update the primary port's is_hwbond field and notify PF layer.
 * This is used to keep primary port's state consistent with standby ports.
 */
static void zxdh_lag_update_primary_port_hwbond(struct zxdh_lag_dev *ldev,
                                                 struct zxdh_en_device *en_dev,
                                                 bool is_hwbond)
{
    struct slave_state *primary_slave = NULL;
    struct zxdh_en_device *primary_en_dev = NULL;
    struct dh_core_dev *core_dev = NULL;

    if (!ldev || !en_dev)
    {
        LOG_ERR("Invalid parameters: ldev=%p, en_dev=%p\n", ldev, en_dev);
        return;
    }

    core_dev = en_dev->parent;

    /* Get primary port's slave state */
    primary_slave = &ldev->tracker.slaves[ldev->primary_pf_idx];
    if (!primary_slave || !primary_slave->slave_info.is_active || !primary_slave->slave_info.en_dev)
    {
        LOG_INFO_DEV(core_dev, "Primary port (panel_id=%d) is not active, skip hwbond update\n",
                     ldev->primary_pf_idx);
        return;
    }

    primary_en_dev = primary_slave->slave_info.en_dev;

    /* Skip if primary and current device are the same */
    if (primary_en_dev == en_dev)
    {
        LOG_DEBUG_DEV(core_dev, "Primary port is the same as current device, skip update\n");
        return;
    }

    /* Update primary port's is_hwbond */
    LOG_INFO_DEV(core_dev, "Update primary port (panel_id=%d) is_hwbond from %d to %d\n",
                 primary_en_dev->panel_id, primary_en_dev->is_hwbond, is_hwbond);
    primary_en_dev->is_hwbond = is_hwbond;
    primary_en_dev->ops->is_hwbond(primary_en_dev->parent, primary_en_dev->is_hwbond, TRUE);
}

/**
 * zxdh_lag_vf_enable_to_sw_bond - Switch to software bond when standby port enables VF
 * @pf_dev: PF device that just enabled VF
 *
 * This function is called when VF is enabled on a port.
 * If this port is a standby port, switch to SOFTWARE_BOND immediately.
 *
 * Call timing: After VF enabled (in dh_pf_sriov_enable)
 */
void zxdh_lag_vf_enable_to_sw_bond(struct zxdh_pf_device *pf_dev)
{
    struct zxdh_lag_dev *ldev = NULL;
    struct zxdh_lag_tracker *tracker = NULL;
    struct zxdh_en_device *en_dev = NULL;
    struct dh_core_dev *core_dev = NULL;
    int32_t ret;

    /* Get en_dev from pf_dev */
    en_dev = pf_dev_get_edev(pf_dev);
    if (!en_dev || !en_dev->ldev)
    {
        LOG_DEBUG("Not in bond, skip bond type check\n");
        return;
    }

    core_dev = en_dev->parent;

    ldev = en_dev->ldev;

    mutex_lock(&ldev->mlock);
    tracker = &ldev->tracker;
    if (!tracker)
    {
        LOG_INFO_DEV(core_dev, "tracker is NULL\n");
        goto unlock;
    }

    /* Optimization: Skip check if this is primary port VF operation
     * Primary port VF enable should NOT affect bond type
     * Only standby port VF operations trigger bond type adjustment
     */
    if (en_dev->panel_id == ldev->primary_pf_idx)
    {
        LOG_INFO_DEV(core_dev,
                     "Primary port (panel_id=%d) VF enabled, skip bond type check\n",
                     en_dev->panel_id);
        goto unlock;
    }

    /* Standby port enabled VF, switch to SOFTWARE_BOND immediately */
    LOG_INFO_DEV(core_dev,
                "Standby port (panel_id=%d) VF enabled, switch to SOFTWARE_BOND\n",
                en_dev->panel_id);

    ret = zxdh_lag_change_to_software_bond(ldev, tracker, en_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(core_dev, "Failed to switch to SOFTWARE_BOND: %d\n", ret);
    } else {
        /* Update primary port's is_hwbond to FALSE */
        zxdh_lag_update_primary_port_hwbond(ldev, en_dev, FALSE);
    }

unlock:
    mutex_unlock(&ldev->mlock);
}
EXPORT_SYMBOL(zxdh_lag_vf_enable_to_sw_bond);

/**
 * zxdh_lag_vf_disable_to_hw_bond - Check if all standby ports disabled VF and switch to hardware bond
 * @pf_dev: PF device that just disabled VF
 *
 * This function is called when VF is disabled on a port.
 * If this port is a standby port, check if ALL standby ports have disabled VF.
 * If yes, evaluate if we can switch back to HARDWARE_BOND.
 *
 * Call timing: Before VF disabled (in dh_pf_sriov_disable)
 */
void zxdh_lag_vf_disable_to_hw_bond(struct zxdh_pf_device *pf_dev)
{
    struct zxdh_bond_slave_stats stats;
    struct zxdh_en_device *en_dev = pf_dev_get_edev(pf_dev);
    struct zxdh_lag_dev *ldev;
    struct zxdh_lag_tracker *tracker = NULL;
    struct dh_core_dev *core_dev;
    int32_t ret;
    bool any_standby_has_vf = false;

    if (!en_dev || !en_dev->ldev)
    {
        LOG_DEBUG("Not in bond, skip bond type check\n");
        return;
    }

    ldev = en_dev->ldev;
    core_dev = en_dev->parent;

    mutex_lock(&ldev->mlock);
    tracker = &ldev->tracker;
    if (!tracker)
    {
        LOG_INFO_DEV(core_dev, "tracker is NULL\n");
        goto unlock;
    }

    /* Optimization: Skip check if this is primary port VF operation
     * Primary port VF disable should NOT affect bond type
     * Only standby port VF operations trigger bond type adjustment
     */
    if (en_dev->panel_id == ldev->primary_pf_idx)
    {
        LOG_INFO_DEV(core_dev,
                     "Primary port (panel_id=%d) VF disabled, skip bond type check\n",
                     en_dev->panel_id);
        goto unlock;
    }

    /* Check all standby ports (except primary) for VF
     * Use common function to check if any standby port has VF enabled
     */
    any_standby_has_vf = zxdh_lag_has_any_standby_vf_enabled(tracker, ldev->primary_pf_idx);

    /* If no standby port has VF, check if we can switch to HARDWARE_BOND */
    if (!any_standby_has_vf && tracker->bond_type == SOFTWARE_BOND)
    {
        enum zxdh_bond_type target_bond_type;

        LOG_INFO_DEV(core_dev,
                    "All standby ports have no VF, checking if can switch to HARDWARE_BOND\n");
        zxdh_lag_update_primary_port_hwbond(ldev, en_dev, TRUE);
        /* Count slave statistics from current bond configuration */
        zxdh_lag_count_bond_slaves(ldev, ldev->upper_netdev, &stats);
        /* Use zxdh_lag_update_bond_type logic to determine if should switch to hard bond */
        target_bond_type = zxdh_lag_update_bond_type(stats.same_ldev_slaves, stats.hardbond_slaves,
                                                      stats.total_slaves, stats.specbond_slaves,
                                                      stats.cur_slaves, ldev->expect_slaves);

        if (target_bond_type == HARDWARE_BOND)
        {
            LOG_INFO_DEV(core_dev,
                         "All standby ports have no VF and conditions met for HARDWARE_BOND, "
                         "switch to HARDWARE_BOND\n");

            ret = zxdh_lag_change_to_hardware_bond(ldev, tracker, en_dev);
            if (ret != 0)
            {
                zxdh_lag_update_primary_port_hwbond(ldev, en_dev, FALSE);
                LOG_ERR_DEV(core_dev, "Failed to switch to HARDWARE_BOND: %d\n", ret);
            }
        }
    }

unlock:
    mutex_unlock(&ldev->mlock);
}
EXPORT_SYMBOL(zxdh_lag_vf_disable_to_hw_bond);

int32_t zxdh_lag_active_hardware_bond_under_recover(struct zxdh_lag_dev *curr_lag, struct zxdh_lag_tracker *tracker, struct zxdh_en_device *en_dev)
{
    struct event_ctx *ctx            = NULL;
    struct event_node *node          = NULL;
    ctx  = &(curr_lag->ctx);
    node = kmalloc(sizeof(*node), GFP_ATOMIC);
    if (!node)
    {
        LOG_ERR("Failed to allocate event node!\n");
        return -1;
    }
    node->event = NETDEV_CHANGEUPPER;
    node->linking = 1;
    tracker->is_bonded = TRUE;
    tracker->target_bond_type = HARDWARE_BOND;
    node->tracker = *tracker;
    node->event_slave_id = en_dev->panel_id;
    node->event_netdev = en_dev->netdev;
    node->from_recover = TRUE;
    // 将节点加入队列
    spin_lock(&ctx->lock);
    node->idx = ctx->idx;
    ctx->idx++;
    list_add_tail(&node->list, &ctx->event_list);  // 添加到队尾
    LOG_INFO("%s node %d addr %p add to list, event NETDEV_CHANGEUPPER linking 1 \n", netdev_name(en_dev->netdev),
            node->idx, (void *)node);
    spin_unlock(&ctx->lock);
    zxdh_queue_hardware_bond_work(curr_lag, ctx, 0);
    return 0;
}

/**
 * @brief 从bond配置中获取指定bond_id对应的网口掩码
 * @param cur_bond_config 32位bond配置，前20bit按panel_id顺序存储每个网口的bond组ID（每个网口占2bit）
 * @param bond_id 要匹配的bond组ID（0-1）
 * @return uint16_t 匹配的网口对应的panel_id位掩码（bit位为1表示对应panel_id的网口属于该bond组）
 */
uint16_t zxdh_get_expect_bond_slaves(uint32_t cur_bond_config, int bond_id)
{
    // 初始化返回值，所有bit位默认为0
    uint16_t expect_slaves = 0;

    int panel_id, bit_offset, port_bond_id;
    
    // 前20bit对应10个网口（panel_id 0~9），每个网口占2bit
    int BITS_PER_PORT = 2; // 每个网口的bond ID占2bit
    
    // 遍历所有10个网口（panel_id 0到9）
    for (panel_id = 0; panel_id <= MAX_PANEL_ID; panel_id++)
    {
        // 计算当前panel_id对应的bit偏移量（从低位开始）
        bit_offset = panel_id * BITS_PER_PORT;

        // 提取当前panel_id对应的2bit值（bond组ID）
        // 掩码0x3表示2bit全1，通过移位和与操作提取目标bit段
        port_bond_id = (cur_bond_config >> bit_offset) & 0x03;

        // 如果当前网口的bond组ID匹配目标bond_id，置位对应bit
        if (port_bond_id == (uint32_t)bond_id)
        {
            // 将expect_slaves的第panel_id位设置为1
            expect_slaves |= (1U << panel_id);
        }
    }

    return expect_slaves;
}

int find_first_set_bit(uint16_t expect_slaves)
{
    int bit_idx = 0;
    uint16_t mask = 0;
    LOG_DEBUG("expect_slaves 0x%x\n", expect_slaves);
    // 遍历bit0到bit9（共10位）
    for (bit_idx = 0; bit_idx < 9; bit_idx++)
    {
        // 构造仅当前bit为1的掩码，与输入值按位与
        mask = 1U << bit_idx;  // 用1U避免移位溢出
        if (expect_slaves & mask)
        {
            // 找到第一个为1的bit，返回索引
            return bit_idx;
        }
    }

    // 所有bit均为0，返回-1表示无有效bit
    LOG_DEBUG("No set bit found in expect_slaves\n");
    return -1;
}

uint32_t set_bond_config(uint8_t panel_num) {
    // 处理panel_num >= 16的情况
    if (panel_num >= 10) {
        return 0;
    }

    // 将全1数左移panel_num*2位，低位自动补0
    return ~0U << (panel_num * 2);
}

static struct zxdh_lag_manager *zxdh_lag_manager_alloc(struct zxdh_en_device *en_dev)
{
    struct zxdh_lag_manager *ldev_manager = NULL;
    int i, j;
    char queue_name[64];
    uint32_t rp_sbdf = en_dev->spec_sbdf >> 8;
    int primary_pf_idx = 0;
    uint8_t panel_nums = 0;
    int ret = 0;
    // 初始化 网卡级变量ldev_manager
    ldev_manager = kzalloc(sizeof(*ldev_manager), GFP_KERNEL);
    if (!ldev_manager)
        return NULL;
    mutex_init(&ldev_manager->mlock);

    //1、读取fwfeature, 判断是否支持网卡硬bond组配置
    if (en_dev->ops->is_special_bond(en_dev->parent)) //I511、X512
    {
        ldev_manager->cur_bond_config = 0;// 代表所有设备在同一个网卡硬bond组
        LOG_INFO_DEV(en_dev->parent, "%s is special bond, set fw_bond_config to 0x%x", pci_name(en_dev->ops->get_pdev(en_dev->parent)), ldev_manager->cur_bond_config);
    }
    else if (!en_dev->ops->is_support_bond_config(en_dev->parent)) 
    {
        if (en_dev->ops->is_rdma_enable(en_dev->parent) || IS_STD_BOARD(en_dev->board_type))//旧的标卡（rdma版本/网络版本）/(旧)存储IO卡/(新/旧)HCE卡固件，未设置support_bond_config标志位；
        {
            panel_nums = en_dev->ops->get_no_bondpf_panel_num(en_dev->parent) == 0 ? 2 : en_dev->ops->get_no_bondpf_panel_num(en_dev->parent); //此处用于兼容25.30及以前的固件版本
            ldev_manager->cur_bond_config = set_bond_config(panel_nums);
        }
        else //旧/新的DPU卡/I512卡/VGCF卡，未设置support_bond_config标志位;
        {
            ldev_manager->cur_bond_config = set_bond_config(0);//此时bond_config为全F，自定义PF组bond会触发软bond流程；
        }
        LOG_INFO_DEV(en_dev->parent, "%s is not support bond config, set fw_bond_config to 0x%x", pci_name(en_dev->ops->get_pdev(en_dev->parent)), ldev_manager->cur_bond_config);
    }
    else //新的标卡/存储IO卡，设置support_bond_config标志位
    {
        // 读取fwshrd区域, 更新本地网卡硬bond组配置
        ldev_manager->cur_bond_config = en_dev->ops->get_bond_config_from_fwshrd(en_dev->parent);
        LOG_INFO_DEV(en_dev->parent, "%s get_bond_config_from_fwshrd: 0x%x", pci_name(en_dev->ops->get_pdev(en_dev->parent)), ldev_manager->cur_bond_config);
        //将当前生效的网卡硬BOND配置更新到fwshrd区域
        en_dev->ops->update_active_bond_config_to_fwshrd(en_dev->parent, ldev_manager->cur_bond_config);
    }

    for (j = 0; j < DH_MAX_PORTS; j++)
    {
        ldev_manager->pf[j].en_dev = NULL;
        ldev_manager->pf[j].netdev = NULL;
        ldev_manager->pf[j].is_active = false;
    }

    // 初始化bond级变量ldev
    for (i = 0; i < DH_MAX_LAG; i++)
    {
        // 识别网卡硬bond组配置中,不同硬bond组包含的slave配置,比如 0000 0000 0000 0011 表示需要用到panel_0和panel_1
        ldev_manager->ldev[i].expect_slaves = zxdh_get_expect_bond_slaves(ldev_manager->cur_bond_config, i);
        primary_pf_idx = find_first_set_bit(ldev_manager->ldev[i].expect_slaves);
        ldev_manager->ldev[i].primary_pf_idx = primary_pf_idx >= 0 ? primary_pf_idx : 0;
        ret = zte_snprintf_s(queue_name, sizeof(queue_name), "dh_lag_0x%x", ((rp_sbdf) << 8) | (i));
        if (ret < 0) 
        {
            LOG_ERR_DEV(en_dev->parent, "zte_snprintf_s failed to generate queue name for lag[%d], rp_sbdf=0x%x", i, rp_sbdf);
            kfree(ldev_manager); // 释放已分配内存
            return NULL;
        }
        ldev_manager->ldev[i].wq = create_singlethread_workqueue(queue_name);  // workqueue需要全环境唯一名称
        if (!ldev_manager->ldev[i].wq) {
            LOG_ERR_DEV(en_dev->parent, "Failed to create workqueue '%s' for lag[%d]", queue_name, i);
            kfree(ldev_manager); // 释放已分配内存
            return NULL;
        }
        INIT_DELAYED_WORK(&ldev_manager->ldev[i].ctx.bond_work, zxdh_do_hardware_bond_work);
        INIT_LIST_HEAD(&ldev_manager->ldev[i].ctx.event_list);
        ldev_manager->ldev[i].nb.notifier_call   = zxdh_hardware_bond_event_handler;
        ldev_manager->ldev[i].upper_netdev       = NULL;
        ldev_manager->ldev[i].state              = LAG_DEV_INVALID;
        ldev_manager->ldev[i].idx                = i;
        ldev_manager->ldev[i].tracker.target_bond_type  = SOFTWARE_BOND;
        ldev_manager->ldev[i].tracker.bond_type  = SOFTWARE_BOND;
        ldev_manager->ldev[i].group_ida  = -1;
        LOG_DEBUG_DEV(en_dev->parent, "lag_dev[%d] work-queue name: %s", ldev_manager->ldev[i].idx, queue_name);
        for (j = 0; j < DH_MAX_PORTS; j++)
        {
            ldev_manager->ldev[i].tracker.slaves[j].lower_state.link_up    = 0;
            ldev_manager->ldev[i].tracker.slaves[j].lower_state.tx_enabled = 0;
            ldev_manager->ldev[i].tracker.slaves[j].slave_info.en_dev      = NULL;
            ldev_manager->ldev[i].tracker.slaves[j].slave_info.netdev      = NULL;
            ldev_manager->ldev[i].tracker.slaves[j].slave_info.is_active   = FALSE;
        }
        mutex_init(&ldev_manager->ldev[i].mlock);
    }
    return ldev_manager;
}

static void zxdh_lag_manager_add_dev(struct zxdh_lag_manager *ldev_manager, struct zxdh_en_device *en_dev)
{
    ldev_manager->pf[en_dev->panel_id].en_dev = en_dev;
    ldev_manager->pf[en_dev->panel_id].netdev = en_dev->netdev;
    ldev_manager->pf[en_dev->panel_id].is_active = true;
    en_dev->ldev_manager = ldev_manager;
    LOG_DEBUG_DEV(en_dev->parent, "ldev_manager add dev: %s panel_id %d", pci_name(en_dev->ops->get_pdev(en_dev->parent)), en_dev->panel_id);
}

void zxdh_lag_manager_remove_dev(struct zxdh_lag_manager *ldev_manager, struct zxdh_en_device *en_dev)
{
    if (en_dev->panel_id >= DH_MAX_PORTS)
        return;
    
    ldev_manager->pf[en_dev->panel_id].is_active = false;
    ldev_manager->pf[en_dev->panel_id].en_dev = NULL;
    ldev_manager->pf[en_dev->panel_id].netdev = NULL;
    en_dev->ldev_manager = NULL;
}

void zxdh_lag_add_dev(struct zxdh_lag_dev *ldev, struct zxdh_en_device *en_dev, struct zxdh_lag_manager *ldev_manager)
{
    struct zxdh_en_device *tmp_en_dev = NULL;
    if (ldev->state == LAG_DEV_INVALID)
    {
        ldev->state = LAG_DEV_IDLE;
#ifdef CGS_V5_693
        register_netdevice_notifier_rh(&ldev->nb);
#else
        register_netdevice_notifier(&ldev->nb);
#endif
    }
    ldev->tracker.slaves[en_dev->panel_id].slave_info = ldev_manager->pf[en_dev->panel_id];
    en_dev->ldev = ldev;
    tmp_en_dev = ldev->tracker.slaves[en_dev->panel_id].slave_info.en_dev;
    LOG_INFO_DEV(en_dev->parent, "lag_dev[%d] add %s panel_id %d, en_dev %p, ldev %p", ldev->idx, pci_name(tmp_en_dev->ops->get_pdev(tmp_en_dev->parent)), tmp_en_dev->panel_id, tmp_en_dev, tmp_en_dev->ldev);
    return;
}

/**
 * @brief 根据panel_id提取cur_bond_config中对应组的2bit值
 * @param cur_bond_config 32位bond配置，前20bit按panel_id顺序存储每组2bit的bond组ID
 * @param panel_id 要提取的2bit组序号（有效范围：0~9，对应前20bit的10组2bit）
 * @return uint8_t 对应组的2bit数值（0~3）；若panel_id超出0~9范围，返回0
 */
uint8_t zxdh_get_ldev_idx_from_bond_cfg(uint32_t cur_bond_config, uint8_t panel_id)
{
    // 计算当前panel_id对应的2bit在cur_bond_config中的偏移量（每个panel占2bit）
    uint8_t bit_offset = panel_id * 2;

    // 右移偏移量后，与0x03（二进制11）做与运算，提取仅有的2bit值
    uint8_t ldev_idx = (cur_bond_config >> bit_offset) & 0x03;

    return ldev_idx;
}

/* Must be called with intf_mutex held */
int zxdh_lag_manager_add(struct zxdh_en_device *en_dev)
{
    struct zxdh_lag_manager *ldev_manager = NULL;
    struct zxdh_lag_dev *ldev = NULL;
    struct zxdh_en_device *tmp_dev;

    if (en_dev->panel_id >= DH_MAX_PORTS)
    {
        LOG_ERR_DEV(en_dev->parent, "ldev_manager add dev: %s failed, panel_id %d >= 10\n", pci_name(en_dev->ops->get_pdev(en_dev->parent)), en_dev->panel_id);
        return -1;
    }

    // 1、寻找同网卡的en_dev
    tmp_dev = dh_get_next_phys_dev(en_dev);
    if (tmp_dev)
        ldev_manager = tmp_dev->ldev_manager;

    if (!ldev_manager)
    {
        ldev_manager = zxdh_lag_manager_alloc(en_dev);
        if (!ldev_manager)
        {
            LOG_ERR_DEV(en_dev->parent, "Failed to alloc ldev_manager\n");
            return -1;
        }
    }

    // 2、将自身信息更新到lag_manager中
    zxdh_lag_manager_add_dev(ldev_manager, en_dev);
    // 3、将自身信息更新到lag_manager->ldev[ldev_idx]中
    en_dev->ldev_idx = zxdh_get_ldev_idx_from_bond_cfg(ldev_manager->cur_bond_config, en_dev->panel_id);  //从ldev_manager->cur_bond_config中获取en_dev对应的ldev_idx
    LOG_INFO_DEV(en_dev->parent, "%s get ldev_idx %d from cur_bond_config 0x%x", pci_name(en_dev->ops->get_pdev(en_dev->parent)), en_dev->ldev_idx, ldev_manager->cur_bond_config);
    if (en_dev->ldev_idx >= DH_MAX_LAG)
    {
        return 0;
    }
    ldev = &ldev_manager->ldev[en_dev->ldev_idx];
    zxdh_lag_add_dev(ldev, en_dev, ldev_manager);

    return 0;
}

void zxdh_lag_dev_remove_dev(struct zxdh_lag_dev *ldev, struct zxdh_en_device *en_dev)
{
    struct zxdh_lag_tracker *tracker = NULL;
    struct zxdh_en_device *tmp_en_dev = NULL;
    struct event_node *node, *tmp;
    int i;

    // 获取锁
    mutex_lock(&ldev->mlock);
    tracker = &ldev->tracker;

    tracker->slaves[en_dev->panel_id].slave_info.en_dev = NULL;
    tracker->slaves[en_dev->panel_id].slave_info.netdev = NULL;
    tracker->slaves[en_dev->panel_id].slave_info.is_active = FALSE;
    en_dev->ldev = NULL;
    for (i = 0; i < DH_MAX_PORTS; i++)
        if (tracker->slaves[i].slave_info.is_active)
        {
            tmp_en_dev = tracker->slaves[i].slave_info.en_dev;
            LOG_INFO_DEV(en_dev->parent, "%s is still active, can't destory lag_dev[%d]\n", pci_name(tmp_en_dev->ops->get_pdev(tmp_en_dev->parent)), ldev->idx);
            break;
        }

    if (i == DH_MAX_PORTS) { //ldev no slave left
        if (ldev->nb.notifier_call) {
#ifdef CGS_V5_693
            unregister_netdevice_notifier_rh(&ldev->nb);
#else
            unregister_netdevice_notifier(&ldev->nb);
#endif
            ldev->nb.notifier_call = NULL;
        }
        ldev->state = LAG_DEV_INVALID;
        spin_lock(&(ldev->ctx.lock));
        list_for_each_entry_safe(node, tmp, &(ldev->ctx.event_list), list) 
        {
            list_del(&node->list);
            LOG_INFO_DEV(en_dev->parent, "lag_dev[%d] del node %d addr %p from list, %s event %ld linking %d\n", ldev->idx, node->idx, (void*)node, node->event_netdev->name, node->event, node->linking);
            kfree(node);
        }
        spin_unlock(&(ldev->ctx.lock));
        cancel_delayed_work_sync(&ldev->ctx.bond_work);
        destroy_workqueue(ldev->wq);
        mutex_unlock(&ldev->mlock);
        mutex_destroy(&ldev->mlock);
        LOG_INFO("lag_dev[%d] no slave left, destory\n", ldev->idx);
    }
    else
    {
        // 释放锁
        mutex_unlock(&ldev->mlock);
    }

    return;
}

void zxdh_lag_manager_free(struct zxdh_lag_manager *ldev_manager)
{
    kfree(ldev_manager);
    //释放ldev_manager->mlock
}

void zxdh_lag_manager_remove(struct zxdh_en_device *en_dev)
{
	struct zxdh_lag_dev *ldev = NULL;
    struct zxdh_lag_manager *ldev_manager = NULL;
	int i, j;

    // 获取zxdh_lag_manager、ldev
    ldev_manager = en_dev->ldev_manager;
    if (!ldev_manager)
		return;

    mutex_lock(&ldev_manager->mlock);

    // 从zxdh_lag_manager中删除en_dev信息
    zxdh_lag_manager_remove_dev(ldev_manager, en_dev);

	ldev = en_dev->ldev;
	if (ldev)
    {
        // 从zxdh_lag_manager->ldev[ldev_idx]中删除en_dev信息
        zxdh_lag_dev_remove_dev(ldev, en_dev);
    }

	for (i = 0; i < DH_MAX_PORTS; i++)
    {
		if (ldev_manager->pf[i].is_active)
			break;
    }

    for (j = 0; j < DH_MAX_LAG; j++)
    {
        LOG_INFO_DEV(en_dev->parent, "ldev_manager->ldev[%d] state = %d\n", j, ldev_manager->ldev[j].state);
        if (ldev_manager->ldev[j].state != LAG_DEV_INVALID)
			break;
    }

    LOG_INFO_DEV(en_dev->parent, "i = %d, j = %d\n", i, j);
	if (i == DH_MAX_PORTS && j == DH_MAX_LAG) {
        en_dev->ops->update_active_bond_config_to_fwshrd(en_dev->parent, 0xFFFFFFFF);
        mutex_unlock(&ldev_manager->mlock);
        mutex_destroy(&ldev_manager->mlock);
		kfree(ldev_manager);
        LOG_INFO_DEV(en_dev->parent, "last dev %s is removed, free ldev_manager\n", pci_name(en_dev->ops->get_pdev(en_dev->parent)));
        return;
	}
    mutex_unlock(&ldev_manager->mlock);
    return;
}

int32_t zxdh_rdma_bond_lacp_dpp_init(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot = en_dev->slot_id,
        .vport = en_dev->vport,
    };
    uint16_t vf_id = zxdh_covert_netdev_2_vfid(en_dev->netdev);
    uint32_t lacp_rxq = en_dev->phy_index[0];

    ret = dpp_uplink_phy_lacp_pf_memport_qid_set(&dpp_pf_info, en_dev->phy_port, lacp_rxq);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_uplink_phy_lacp_pf_memport_qid_set failed: %d\n", ret);
        goto out;
    }
    ret = dpp_uplink_phy_lacp_pf_vqm_vfid_set(&dpp_pf_info, en_dev->phy_port, vf_id);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_uplink_phy_lacp_pf_vqm_vfid_set failed: %d\n", ret);
        goto out;
    }

out:
    return ret;
}

int32_t zxdh_special_bond_lacp_dpp_init(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    DPP_PF_INFO_T dpp_pf_info = {
        .slot = en_dev->slot_id,
        .vport = en_dev->vport,
    };
    uint16_t vf_id = zxdh_covert_netdev_2_vfid(en_dev->netdev);
    uint32_t lacp_rxq = en_dev->phy_index[0];

    ret = dpp_uplink_phy_bond_vport(&dpp_pf_info, en_dev->phy_port);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_uplink_phy_bond_vport failed: %d\n", ret);
        goto out;
    }
    ret = dpp_uplink_phy_lacp_pf_vqm_vfid_set(&dpp_pf_info, en_dev->phy_port, vf_id);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_uplink_phy_lacp_pf_vqm_vfid_set failed: %d\n", ret);
        goto out;
    }
    ret = dpp_uplink_phy_lacp_pf_memport_qid_set(&dpp_pf_info, en_dev->phy_port, lacp_rxq);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_uplink_phy_lacp_pf_memport_qid_set failed: %d\n", ret);
        goto out;
    }

out:
    return ret;
}

int32_t zxdh_lag_bond_lacp_dpp_init(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    if (en_dev->ops->is_special_bond(en_dev->parent))
    {
        ret = zxdh_special_bond_lacp_dpp_init(en_dev);
    }
    else
    {
        ret = zxdh_rdma_bond_lacp_dpp_init(en_dev);
    }
    return ret;
}

int32_t zxdh_recover_hwbond_in_reload(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_lag_tracker *tracker = NULL;
    struct zxdh_lag_dev *ldev = NULL;
    struct zxdh_lag_manager *ldev_manager = NULL;
    struct slave_state *cur_slave = NULL;
    int32_t ret = 0;

    if ((!zxdh_en_is_panel_port(en_dev)) || (en_dev->ops->is_bond(en_dev->parent)))
    {
        return 0;
    }

    // 判断是否处于硬bond场景
    ldev_manager = en_dev->ldev_manager;
    if (!ldev_manager)
    {
        LOG_INFO("%s ldev_manager is NULL, no need to recover hwbond\n", netdev_name(netdev));
        return 0;
    }
    ldev = en_dev->ldev;
    if (!ldev)
    {
        LOG_INFO("%s ldev is NULL, no need to recover hwbond\n", netdev_name(netdev));
        return 0;
    }
    tracker = &ldev->tracker;
    if (!tracker)
    {
        LOG_INFO("%s tracker is NULL, no need to recover hwbond\n", netdev_name(netdev));
        return 0;
    }

    zxdh_lag_bond_lacp_dpp_init(en_dev);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_lag_bond_lacp_dpp_init failed: %d\n", ret);
        return -1;
    }

    mutex_lock(&ldev->mlock);
    if(!(ldev->state == LAG_DEV_ACTIVE && ldev->upper_netdev && !(tracker->bond_type == SOFTWARE_BOND)))
    {
        LOG_INFO_DEV(en_dev->parent, "%s lag_dev[%d] bond_type isn't HARDWARE_BOND or SPECIAL_BOND, no need to recover hwbond\n", netdev_name(netdev), ldev->idx);
        goto out;
    }
    if (!(en_dev->ops->is_special_bond(en_dev->parent) || en_dev->is_hwbond)) // no-special-bond且hwbond开关未开，则无需恢复硬bond相关表项
    {
        goto out;
    }
    if (en_dev->ops->is_special_bond(en_dev->parent)) // I511硬bond：每个设备触发自愈，都需要自行处理upper相关的表项
    {
        cur_slave = &tracker->slaves[en_dev->panel_id];
        // 1. bond相关np表（整个组bond过程仅执行一次）
        zxdh_lag_init_special_bond_dpp(tracker, cur_slave, ldev->group_ida);
        // 2. slave相关np表（每个slave加入bond组时执行一次）
        zxdh_lag_init_special_bond_slave_dpp(tracker, ldev, cur_slave);
    }
    else // 标卡、存储io卡、HCE卡硬bond：需要在bond事件中统一处理
    {
        en_dev->ops->update_active_bond_config_to_fwshrd(en_dev->parent, ldev_manager->cur_bond_config);
        zxdh_lag_active_hardware_bond_under_recover(ldev, tracker, en_dev);
    }
out:
    mutex_unlock(&ldev->mlock);
    return 0;
}

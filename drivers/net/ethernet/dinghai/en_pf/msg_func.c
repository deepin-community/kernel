#include <linux/dinghai/driver.h>
#include <linux/dinghai/dh_cmd.h>
#include "../msg_common.h"
#include "../en_pf.h"
#include "../en_aux/en_aux_cmd.h"
#include "../en_aux.h"
#include "../en_np/init/include/dpp_np_init.h"
#include "en_pf_eq.h"
#include "msg_func.h"
#include "../plcr.h"
#include "../en_np/driver/include/dpp_drv_sdt.h"
#include "../slib.h"

#define FUNC_NAME_SIZE_MAX       32
#define ZXDH_MAX_VF              256
#define PF_HAS_MAX_ENCAP1_NUM    256

#define ETH_PKT_IPV4 0x0800
#define ETH_PKT_IPV6 0x86dd

typedef uint32_t (*zxdh_vf_msg_func)(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev);

typedef struct
{
    zxdh_msg_op_code op_code;
    uint8_t proc_name[FUNC_NAME_SIZE_MAX];
    zxdh_vf_msg_func msg_proc;
} zxdh_vf_msg_proc;

extern int debug_print;
void zxdh_u32_array_print(uint32_t *array, uint16_t size)
{
    uint16_t i;

    if (debug_print == 0)
        return;

    for (i = 0; i < size; ++i)
    {
        printk(KERN_CONT "%u    ", array[i]);
        if ((i + 1) % 8 == 0)
        {
            printk(KERN_CONT "\n");
        }
    }
}
EXPORT_SYMBOL(zxdh_u32_array_print);

static void zxdh_vf_link_state_get_proc(struct zxdh_pf_device *pf_dev, struct zxdh_vf_item *vf_item, uint16_t vf_idx)
{
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    uint32_t dev_link_up_reg = 0;
    uint8_t vf_link_up = 0;

    if(vf_item->link_forced)
    {
        vf_link_up = vf_item->link_up ? 1 : 0;
    }
    else
    {
        LOG_DEBUG_DEV(dh_dev, "vf %d, pf(pcieid 0x%x, bond_link_info %d, pf_dev->link_up %d) is_special_bond %d is_hwbond %d is_primary_port %d is_bond_slave %d\n", vf_idx, pf_dev->pcie_id, pf_dev->bond_link_info, pf_dev->link_up,\
        pf_dev->is_special_bond, pf_dev->is_hwbond, pf_dev->is_primary_port, pf_dev->is_bond_slave);
        if(!pf_dev->is_special_bond && pf_dev->is_hwbond && pf_dev->is_primary_port && pf_dev->is_bond_slave)
        {
            vf_link_up = pf_dev->bond_link_info == 0 ? 0 : 1;
            vf_link_up = vf_link_up || pf_dev->link_up;
        }
        else
        {   /*VF state does not follow PF because PF is not taken by DVS， PF is down*/
            /* Started by AICoder, pid:za07a32fc3yc24b1419c0968d03763180be33e34 */
            if (zxdh_pf_get_dev_type(dh_dev) == ZXDH_DEV_ROCE_RDMA)
            {
                if (pf_dev->pf_sriov_cap_base)
                {
                    dev_link_up_reg = ioread32((void __iomem *)(pf_dev->pf_sriov_cap_base + (pf_dev->sriov_bar_size) * vf_idx \
                        + pf_dev->dev_cfg_bar_off + ZXDH_DEV_MAC_HIGH_OFFSET));
                    dev_link_up_reg = (dev_link_up_reg >> 16) & 0xff;
                    LOG_INFO_DEV(dh_dev, "RDMA vf[%d] link_forced is [%s], link state[%s].\n",
                        vf_idx, vf_item->link_forced ? "TRUE" : "FALSE", (dev_link_up_reg == 1 ) ? "UP" : "DOWN");
                }
                return;
            }
            /* Ended by AICoder, pid:za07a32fc3yc24b1419c0968d03763180be33e34 */
            vf_link_up = pf_dev->link_up ? 1 : 0;
        }
    }

    if (pf_dev->pf_sriov_cap_base)
    {
        dev_link_up_reg = ioread32((void __iomem *)(pf_dev->pf_sriov_cap_base + (pf_dev->sriov_bar_size) * vf_idx \
                                + pf_dev->dev_cfg_bar_off + ZXDH_DEV_MAC_HIGH_OFFSET));
        dev_link_up_reg = (dev_link_up_reg & ~(0xFF << 16)) | ((uint32_t)(vf_link_up) << 16);
        iowrite32(dev_link_up_reg, (void __iomem *)(pf_dev->pf_sriov_cap_base + (pf_dev->sriov_bar_size) * vf_idx \
                                + pf_dev->dev_cfg_bar_off + ZXDH_DEV_MAC_HIGH_OFFSET));
    }
    LOG_INFO_DEV(dh_dev, "vf[%d] link_forced is [%s], link state[%s] update ok.\n", vf_idx, vf_item->link_forced?"TRUE":"FALSE", (vf_link_up==1)?"UP":"DOWN");

}

int32_t zxdh_vf_flush_mac(DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item)
{
    int32_t err  = 0;
    uint8_t  i    = 0;
    uint8_t *addr = NULL;
    uint16_t sriov_vlan_tpid = 0;
    uint16_t sriov_vlan_id = 0;

    mutex_lock(&vf_item->lock);

    sriov_vlan_tpid = vf_item->vlan_proto;
    sriov_vlan_id = vf_item->vlan;

    /* 删除此VF的所有单播mac地址 */
    for (i = 0; i < DEV_UNICAST_MAX_NUM; ++i)
    {
        addr = vf_item->vf_mac_info.unicast_mac[i].mac_addr;

        if (!is_zero_ether_addr(addr))   /* mac不全为0 */
        {
            LOG_DEBUG("the deleted unicast mac is %pM\n", addr);
            err = dpp_del_mac(pf_info, addr, sriov_vlan_tpid, sriov_vlan_id);
            if (err != 0)
            {
                LOG_ERR("dpp_del_mac failed\n");
                mutex_unlock(&vf_item->lock);
                return err;
            }
        }
    }

    /* 删除此VF的所有组播mac地址 */
    for (i = 0; i < DEV_MULTICAST_MAX_NUM; ++i)
    {
        addr = vf_item->vf_mac_info.multicast_mac[i].mac_addr;

        if (!is_zero_ether_addr(addr))    /* mac不全为0 */
        {
            LOG_DEBUG("the deleted multicasat mac is %pM\n", addr);
            err = dpp_multi_mac_del_member(pf_info, addr);
            if (err != 0)
            {
                LOG_ERR("dpp_multi_mac_del_member failed\n");
                mutex_unlock(&vf_item->lock);
                return err;
            }
        }
    }

    /* 将vf_mac_info结构体全部清零 */
    memset(&vf_item->vf_mac_info, 0, sizeof(vf_item->vf_mac_info));

    mutex_unlock(&vf_item->lock);
    return err;
}

static int zxdh_vf_enable_sriov_vlan_tbl(DPP_PF_INFO_T *pf_info, u16 vlan_tci, uint16_t vlan_proto)
{
    int ret = 0;

    ret = dpp_vport_vlan_offload_en_set(pf_info, 1);
    if (ret != 0)
    {
        goto err;
    }

    ret = dpp_vqm_vfid_vlan_set(pf_info, VLAN_SRIOV_VLAN_TCI, vlan_tci);
    if (ret != 0)
    {
        goto err;
    }

    ret = dpp_vqm_vfid_vlan_set(pf_info, VLAN_SRIOV_VLAN_TPID, vlan_proto);
    if (ret != 0)
    {
        goto err;
    }

err:
    return ret;
}

static int zxdh_vf_init_vlan_recfg(struct zxdh_vf_item *vf_item, DPP_PF_INFO_T *pf_info)
{
    int ret = 0;
    uint16_t vlan_tci = 0;

    /* 先清空vlan 二级表，vqm Vlan表*/
    ret = dpp_vqm_vfid_vlan_init(pf_info);
    if (ret != 0)
    {
        LOG_ERR("dpp_vqm_vfid_vlan_init, ret: %d\n", ret);
        goto out;
    }

    ret = dpp_vlan_filter_init(pf_info);
    if (ret != 0)
    {
        LOG_ERR("dpp_vlan_filter_init failed: %d\n", ret);
        goto out;
    }

    ret = dpp_add_vlan_filter(pf_info, 0);
    if (ret != 0)
    {
        LOG_ERR("dpp_add_vlan_filter 0 failed: %d\n", ret);
        goto out;
    }

    /* 再次从vf_item中获取vlanID信息， 如果非0就重配*/
    if (vf_item->vlan != 0)
    {
        vlan_tci = ZXDH_VLAN_TCI_GEN(vf_item->vlan, vf_item->qos);
        ret = zxdh_vf_enable_sriov_vlan_tbl(pf_info, vlan_tci, vf_item->vlan_proto);
        if (ret != 0)
        {
            LOG_ERR("zxdh_enable_sriov_vlan_tbl failed, ret: %d\n", ret);
            return ret;
        }

        LOG_DEBUG("recover vf vlan: %d.\n", vf_item->vlan);
    }

out:
    return ret;
}

int32_t zxdh_vf_roce_overlay_init(struct zxdh_pf_device *pf_dev, DPP_PF_INFO_T *pf_info)
{
    int32_t ret = 0;
    struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);

    if (zxdh_pf_get_dev_type(dh_dev) == ZXDH_DEV_ROCE_RDMA)
    {
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_BUSINESS_EN_OFF, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set business_enable failed: %d\n", ret);
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_ROCE_OVERLAY_EN, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set roce_overlay_enable failed: %d\n", ret);
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_LAG_EN_OFF, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_vport, "dpp_vport_attr_set lag_enable failed: %d\n", ret);
    }

    return 0;

err_vport:
    return ret;
}

static uint32_t zxdh_vf_port_init(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t ret = 0;
    uint8_t mac[6] = {0};
    int32_t vf_idx = msg->hdr.pcie_id & (0xff);
    uint8_t addr_type = NET_ADDR_PERM;
    uint16_t sriov_vlan_tpid = vf_item->vlan_proto;
    uint16_t sriov_vlan_id = vf_item->vlan;
    struct dh_core_dev *dh_dev = container_of((void*)pf_dev, struct dh_core_dev, priv);
    uint32_t hash_mode = ZXDH_HASH_MODE_BY_MCODE_FLAG(pf_dev->mcode_feature);

    LOG_DEBUG_DEV(dh_dev, "zxdh_vf_port_init, vfindex%d\n", vf_idx);
    ret = dpp_vport_create(pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_create failed, ret: %d\n", ret);
        return ret;
    }

    if (msg->vf_init_msg.is_upf)
    {
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_LAG_ID, 0); /* 不需要配置 VF LAG属性跟随PF */
        if (ret != 0)
        {
            LOG_ERR_DEV(dh_dev, "dpp_vport_attr_set panel_id %d failed: %d\n", pf_dev->phy_port, ret);
            goto err_init;
        }

        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_LAG_EN_OFF, 1);
        if (ret != 0)
        {
            LOG_ERR_DEV(dh_dev, "dpp_vport_attr_set hash_search_idx %u failed: %d\n", msg->vf_init_msg.hash_search_idx, ret);
            goto err_init;
        }
    }
    else
    {
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_UPLINK_PHY_PORT_ID, pf_dev->phy_port);
        if (ret != 0)
        {
            LOG_ERR_DEV(dh_dev, "dpp_vport_attr_set panel_id %d failed: %d\n", pf_dev->phy_port, ret);
            goto err_init;
        }
    }

    ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_HASH_SEARCH_INDEX, msg->vf_init_msg.hash_search_idx);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_attr_set hash_search_idx %u failed: %d\n", msg->vf_init_msg.hash_search_idx, ret);
        goto err_init;
    }

    ret = zxdh_vf_roce_overlay_init(pf_dev, pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_vf_roce_overlay_init failed: %d\n", ret);
        goto err_init;
    }

    ret = dpp_vport_bond_pf(pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_bond_pf failed, ret: %d\n", ret);
        goto err_init;
    }

    ret = dpp_vport_rss_en_set(pf_info, msg->vf_init_msg.rss_enable);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_rss_en_set failed, ret: %d\n", ret);
        goto err_init;
    }

    ret = dpp_vport_hash_funcs_set(pf_info, ZXDH_FUNC_TOP);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_hash_funcs_set failed, ret: %d\n", ret);
        goto err_init;
    }

    ret = dpp_vport_rx_flow_hash_set(pf_info, hash_mode);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_rx_flow_hash_set failed, ret: %d\n", ret);
        goto err_init;
    }

    ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_VEPA_EN_OFF, (uint32_t)pf_dev->vepa);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_attr_set vport(0x%x) %s mode failed: %d\n", msg->hdr.vport, pf_dev->vepa?"vepa":"veb", ret);
        goto err_init;
    }
    LOG_DEBUG_DEV(dh_dev, "Initialize vport(0x%x) to %s mode\n", msg->hdr.vport, pf_dev->vepa?"vepa":"veb");

    ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_PORT_BASE_QID, msg->vf_init_msg.base_qid);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "set_base_qid %d failed: %d\n", msg->vf_init_msg.base_qid, ret);
        goto err_init;
    }

    ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_SPOOFCHK_EN_OFF, vf_item->spoofchk);
    if (0 != ret)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_attr_set spookchk %s failed: %d\n", vf_item->spoofchk? "on":"off", ret);
        goto err_init;
    }

    ret = zxdh_vf_init_vlan_recfg(vf_item, pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_vf_init_vlan_recfg  %d\n", ret);
        goto err_init;
    }

    ret = zxdh_vf_flush_mac(pf_info, vf_item);
    if (ret != 0)
    {
        goto err_init;
    }

    ret = dpp_fd_acl_all_delete(pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_fd_acl_all_delete failed! %d\n", ret);
        goto err_init;
    }

    ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_FD_VXLAN_OFFLOAD_EN, 0);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_attr_set vxlan offload ip checksum failed: %d\n", ret);
        goto err_init;
    }

    if ((dh_dev->pdev->device == ZXDH_PF_DPUB_ROCE_RDMA0_DEVICE_ID) || (dh_dev->pdev->device == ZXDH_PF_DPUB_ROCE_RDMA1_DEVICE_ID))
    {
        zxdh_pf_get_vf_mac(dh_dev, mac, vf_idx);
    }
    else
    {
        ether_addr_copy(mac, vf_item->mac);
    }
    if (is_zero_ether_addr(mac))
    {
        get_random_bytes(mac, 6);
        mac[0] &= 0xfe;
        addr_type = NET_ADDR_RANDOM;
        LOG_INFO_DEV(dh_dev, "vf set random mac %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }

    LOG_DEBUG_DEV(dh_dev, "zxdh_vf_port_init mac %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ret = dpp_add_mac(pf_info, mac, sriov_vlan_tpid, sriov_vlan_id);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_add_mac failed, ret: %d\n", ret);
        goto err_init;
    }
    vf_item->vf_mac_info.current_unicast_num = 1;
    zxdh_pf_set_vf_mac_reg(pf_dev, mac, vf_idx);
    dpp_vport_uc_promisc_set(pf_info, 0);
    dpp_vport_mc_promisc_set(pf_info, 0);

    ether_addr_copy(vf_item->vf_mac_info.unicast_mac[0].mac_addr, mac);
    ether_addr_copy(reps->vf_init_msg.mac_addr, mac);
    reps->vf_init_msg.addr_assign_type = addr_type;
    reps->vf_init_msg.phy_port = pf_dev->phy_port;
    reps->vf_init_msg.link_up = pf_dev->link_up;
    reps->vf_init_msg.speed = pf_dev->speed;
    reps->vf_init_msg.duplex = pf_dev->duplex;
    reps->vf_init_msg.autoneg_enable = pf_dev->autoneg_enable;
    reps->vf_init_msg.sup_link_modes = pf_dev->supported_speed_modes;
    reps->vf_init_msg.adv_link_modes = pf_dev->advertising_speed_modes;
    reps->vf_init_msg.vlan_id = vf_item->vlan;
    reps->vf_init_msg.vlan_qos = vf_item->qos;

    zxdh_plcr_recover_cfg(vf_item,pf_dev,vf_idx);

    zxdh_vf_link_state_get_proc(pf_dev, vf_item, vf_idx);

    vf_item->is_probed = true;
    return 0;

err_init:
    dpp_vport_delete(pf_info);
    return ret;
}

static int32_t zxdh_vf_rate_clear(uint16_t vf_idx, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    zxdh_plcr_rate_limit_paras rate_limit_paras = {0};
    int32_t rtn = 0;

    rate_limit_paras.req_type  = E_RATE_LIMIT_REQ_VF_BYTE;
    rate_limit_paras.direction = E_RATE_LIMIT_TX;
    rate_limit_paras.mode      = E_RATE_LIMIT_BYTE;
    rate_limit_paras.max_rate  = 0;
    rate_limit_paras.min_rate  = 0;
    rate_limit_paras.queue_id  = PLCR_INVALID_PARAM;
    rate_limit_paras.vf_idx    = vf_idx;
    rate_limit_paras.vfid      = PLCR_INVALID_PARAM;
    rate_limit_paras.group_id  = PLCR_INVALID_PARAM;

    rtn = zxdh_plcr_unified_set_rate_limit(pf_dev, &rate_limit_paras);
    if (PLCR_REMOVE_RATE_LIMIT == rtn || PLCR_DUPLICATE_RATE == rtn)
    {
        return 0;
    }

    return rtn;
}

static int32_t zxdh_vf_rate_limit_health_set(uint16_t vf_idx, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    zxdh_plcr_rate_limit_paras rate_limit_paras = {0};
    int32_t rtn = 0;

    // PLCR_FUNC_DBG_ENTER();

    rate_limit_paras.req_type  = E_RATE_LIMIT_REQ_VF_BYTE;
    rate_limit_paras.direction = E_RATE_LIMIT_TX;
    rate_limit_paras.mode      = E_RATE_LIMIT_BYTE;
    rate_limit_paras.max_rate  = vf_item->max_tx_rate;
    rate_limit_paras.min_rate  = vf_item->min_tx_rate;
    rate_limit_paras.queue_id  = PLCR_INVALID_PARAM;
    rate_limit_paras.vf_idx    = vf_idx;
    rate_limit_paras.vfid      = PLCR_INVALID_PARAM;
    rate_limit_paras.group_id  = PLCR_INVALID_PARAM;

    rtn = zxdh_plcr_unified_set_rate_limit(pf_dev, &rate_limit_paras);
    if (PLCR_REMOVE_RATE_LIMIT == rtn || PLCR_DUPLICATE_RATE == rtn)
    {
        return 0;
    }

    return rtn;
}

static uint32_t zxdh_vf_mac_recover(DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item)
{
    uint16_t sriov_vlan_id = vf_item->vlan;
    uint16_t sriov_vlan_tpid = vf_item->vlan_proto;
    uint32_t i = 0;
    uint32_t err = 0;

    /* 遍历vf_item的单播数组 */
    for (i = 0; i < DEV_UNICAST_MAX_NUM; ++i) {
        if (!is_zero_ether_addr(vf_item->vf_mac_info.unicast_mac[i].mac_addr)) {
            err = dpp_add_mac(pf_info, vf_item->vf_mac_info.unicast_mac[i].mac_addr,
                    sriov_vlan_tpid, sriov_vlan_id);
            ZXDH_CHECK_RET_RETURN(err, "dpp_add_unicast_mac[%d] failed: %d\n", i, err);
        }
    }

    /* 遍历vf_item的组播数组 */
    for (i = 0; i <  DEV_MULTICAST_MAX_NUM; ++i) {
        if (!is_zero_ether_addr(vf_item->vf_mac_info.multicast_mac[i].mac_addr)) {
            err = dpp_multi_mac_add_member(pf_info, vf_item->vf_mac_info.multicast_mac[i].mac_addr);
            ZXDH_CHECK_RET_RETURN(err, "dpp_add_multicast_mac[%d] failed: %d\n", i, err);
        }
    }

    return 0;
}

static uint32_t zxdh_vf_item_reload(struct zxdh_pf_device *pf_dev, DPP_PF_INFO_T *pf_info,
                                    struct zxdh_vf_item *vf_item, uint16_t vf_idx)
{
    uint32_t ret = 0;

    ret = zxdh_vf_mac_recover(pf_info, vf_item);
    ZXDH_CHECK_RET_RETURN(ret, "zxdh_vf_mac_recover failed! %d\n", ret);

    ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_SPOOFCHK_EN_OFF, vf_item->spoofchk);
    ZXDH_CHECK_RET_RETURN(ret, "spookchk %s failed: %d\n", vf_item->spoofchk? "on":"off", ret);

    ret = zxdh_vf_init_vlan_recfg(vf_item, pf_info);
    ZXDH_CHECK_RET_RETURN(ret, "zxdh_vf_init_vlan_recfg  %d\n", ret);

    vf_item->is_probed = true;
    zxdh_vf_link_state_get_proc(pf_dev, vf_item, vf_idx);

    return ret;
}

int32_t zxdh_vlan_trunk_recover(DPP_PF_INFO_T *pf_info, uint8_t *vlan_trunk_bitmap)
{
    int ret = 0;
    uint16_t vlan_idx = 0;
    uint16_t byte_index = 0;
    uint8_t bit_idx= 0;

    for (vlan_idx = 0; vlan_idx < 4096; vlan_idx++)
    {
        byte_index = vlan_idx / 8;
        bit_idx = vlan_idx % 8;
        if (vlan_trunk_bitmap[byte_index] & (1 << bit_idx))
        {
            ret = dpp_add_vlan_filter(pf_info, vlan_idx);
            if (0 != ret)
            {
                LOG_ERR("failed to recover vlan bit %d\n", vlan_idx);
                return -1;
            }
            LOG_DEBUG("dev-0x%x recover vlan-%d.\n", pf_info->vport, vlan_idx);
        }
    }
    return ret;
}
EXPORT_SYMBOL(zxdh_vlan_trunk_recover);

static uint32_t zxdh_vf_port_reload(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    uint16_t vf_idx = msg->hdr.pcie_id & (0xff);
    zxdh_vf_reload_msg *eth_config = &msg->vf_reload_msg;
    uint32_t ret = 0;

    LOG_INFO_DEV(dh_dev, "zxdh_vf_port_reload, vfindex%d\n", vf_idx);
    ret = dpp_vport_create(pf_info);
    ZXDH_CHECK_RET_RETURN(ret, "dpp_vport_create failed, ret: %d\n", ret);

    if (eth_config->is_upf) {
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_LAG_ID, 0); /* 不需要配置 VF LAG属性跟随PF */
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "dpp_vport_attr_set panel_id %d failed: %d\n", pf_dev->phy_port, ret);

        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_LAG_EN_OFF, 1);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "dpp_vport_attr_set SRIOV_VPORT_LAG_EN_OFF failed: %d\n", ret);
    } else {
        ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_UPLINK_PHY_PORT_ID, pf_dev->phy_port);
        ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "dpp_vport_attr_set panel_id %d failed: %d\n", pf_dev->phy_port, ret);
    }

    ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_HASH_SEARCH_INDEX, eth_config->hash_search_idx);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "dpp_vport_attr_set hash_search_idx %u failed: %d\n", eth_config->hash_search_idx, ret);

    ret = dpp_vport_bond_pf(pf_info);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "dpp_vport_bond_pf failed, ret: %d\n", ret);

    ret = dpp_vport_hash_funcs_set(pf_info, eth_config->hash_func);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "dpp_vport_hash_funcs_set failed, ret: %d\n", ret);

    ret = dpp_vport_rx_flow_hash_set(pf_info, eth_config->hash_mode);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "dpp_vport_rx_flow_hash_set failed, ret: %d\n", ret);

    ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_VEPA_EN_OFF, (uint32_t)pf_dev->vepa);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "dpp_vport_attr_set vport(0x%x) %s mode failed: %d\n", msg->hdr.vport, pf_dev->vepa?"vepa":"veb", ret);
    LOG_DEBUG_DEV(dh_dev, "Initialize vport(0x%x) to %s mode\n", msg->hdr.vport, pf_dev->vepa?"vepa":"veb");

    ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_PORT_BASE_QID, eth_config->base_qid);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "set_base_qid %d failed: %d\n", eth_config->base_qid, ret);

    ret = dpp_rxfh_set(pf_info, eth_config->queue_map, ZXDH_INDIR_RQT_SIZE);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "dpp_rxfh_set failed: %d\n", ret);

    ret = dpp_fd_acl_all_delete(pf_info);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "dpp_fd_acl_all_delete failed: %d\n", ret);

    if (vf_item->trusted) {
        dpp_vport_uc_promisc_set(pf_info, eth_config->uc_promisc);
        dpp_vport_mc_promisc_set(pf_info, eth_config->mc_promisc);
        if (eth_config->uc_promisc)
            dpp_vport_promisc_en_set(pf_info, 1);
    }

    //SRIOV_CONFIG
    ret = zxdh_vf_item_reload(pf_dev, pf_info, vf_item, vf_idx);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "sriov_config_recover failed! %d\n", ret);

    ret = zxdh_vlan_trunk_recover(pf_info, eth_config->vlan_trunk_bitmap);
    ZXDH_CHECK_RET_GOTO_ERR(ret, err_init, "vlan_trunk_tbl_recover failed! %d\n", ret);

    ret = zxdh_vf_rate_clear(vf_idx, vf_item, pf_dev);
    ZXDH_CHECK_RET_RETURN(ret, "zxdh_vf_rate_clear failed: %d\n", ret);

    ret = zxdh_vf_rate_limit_health_set(vf_idx, vf_item, pf_dev);
    ZXDH_CHECK_RET_RETURN(ret, "zxdh_vf_rate_limit_health_set failed: %d\n", ret);

    ret = zxdh_vqm_vf_set_rate_limit(pf_dev, vf_item->vport, vf_item->max_tx_rate);
    ZXDH_CHECK_RET_RETURN(ret, "zxdh_vqm_vf_set_rate_limit failed: %d\n", ret);
    
    return 0;

err_init:
    dpp_vport_delete(pf_info);
    return ret;
}

static uint32_t zxdh_vf_port_uninit(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    int32_t ret = 0;

    dpp_vport_uc_promisc_set(pf_info, 0);
    dpp_vport_mc_promisc_set(pf_info, 0);

    ret = dpp_fd_acl_all_delete(pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_fd_acl_all_delete failed! %d\n", ret);
        return ret;
    }
    /* 退出时， 都先清除vlan相关的配置*/
    ret = dpp_vqm_vfid_vlan_delete(pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_vlan_filter_en_set failed, ret: %d\n", ret);
        return ret;
    }

    ret = dpp_vlan_filter_init(pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vlan_filter_init failed: %d\n", ret);
       return ret;
    }

    ret = zxdh_vf_flush_mac(pf_info, vf_item);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_vf_flush_macf failed, ret: %d\n", ret);
        return ret;
    }

    ret = dpp_vport_unbond_pf(pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_unbond_pf failed, ret: %d\n", ret);
        return ret;
    }

    ret = dpp_vport_delete(pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_delete failed, ret: %d\n", ret);
        return ret;
    }

    ret = dpp_vport_unregister(pf_info);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_vport_unregister failed, ret: %d\n", ret);
        return ret;
    }

    vf_item->is_probed = false;
    return ret;
}

void zxdh_vf_item_mac_add(struct zxdh_vf_item *vf_item, uint8_t *mac_addr, uint8_t dhtool_mac_set_flag)
{
    uint8_t *addr = NULL;
    uint8_t i = 0;

    if (is_unicast_ether_addr(mac_addr))
    {
        for (i = 0; i < DEV_UNICAST_MAX_NUM; ++i)
        {
            if(ether_addr_equal(mac_addr, vf_item->vf_mac_info.unicast_mac[i].mac_addr))
            {
                /* mac已经存在 */
                vf_item->vf_mac_info.unicast_mac[i].dhtool_mac_set_flag = dhtool_mac_set_flag;
                return;
            }
        }
        for (i = 1; i < DEV_UNICAST_MAX_NUM; ++i) /* 保留本机mac */
        {
            addr = vf_item->vf_mac_info.unicast_mac[i].mac_addr;
            if (is_zero_ether_addr(addr)) /*查询没使用的数组*/
            {
                /* 将此mac添加到zxdh_vf_item中 */
                memcpy(addr, mac_addr, ETH_ALEN);
                vf_item->vf_mac_info.unicast_mac[i].dhtool_mac_set_flag = dhtool_mac_set_flag;
                vf_item->vf_mac_info.current_unicast_num++;
                break;
            }
        }
    }
    else
    {
        for (i = 0; i < DEV_MULTICAST_MAX_NUM; ++i)
        {
            if(ether_addr_equal(mac_addr, vf_item->vf_mac_info.multicast_mac[i].mac_addr))
            {
                /* mac已经存在 */
                vf_item->vf_mac_info.multicast_mac[i].dhtool_mac_set_flag = dhtool_mac_set_flag;
                return;
            }
        }
        for (i = 0; i < DEV_MULTICAST_MAX_NUM; ++i)
        {
            addr = vf_item->vf_mac_info.multicast_mac[i].mac_addr;
            if (is_zero_ether_addr(addr))
            {
                /* 将此mac添加到zxdh_vf_item中 */
                memcpy(addr, mac_addr, ETH_ALEN);
                vf_item->vf_mac_info.multicast_mac[i].dhtool_mac_set_flag = dhtool_mac_set_flag;
                vf_item->vf_mac_info.current_multicast_num++;
                break;
            }
        }
    }

    return;
}
EXPORT_SYMBOL(zxdh_vf_item_mac_add);

void zxdh_vf_item_mac_del(struct zxdh_vf_item *vf_item, uint8_t *mac_addr)
{
    uint8_t i = 0;
    uint8_t *addr = NULL;

    if (is_unicast_ether_addr(mac_addr))
    {
        for (i = 1; i < DEV_UNICAST_MAX_NUM; ++i)
        {
            /* 获取此mac地址 */
            addr = vf_item->vf_mac_info.unicast_mac[i].mac_addr;

            if (ether_addr_equal(addr, mac_addr)) /* 查询到此mac */
            {
                LOG_DEBUG("the mac is %pM\n", addr);
                /* 在地址数组中将此mac清空*/
                memset(&vf_item->vf_mac_info.unicast_mac[i], 0, ETH_ALEN);
                vf_item->vf_mac_info.current_unicast_num--;
                break;
            }
        }
    }
    else
    {
        for (i = 0; i < DEV_MULTICAST_MAX_NUM; ++i)
        {
            addr = vf_item->vf_mac_info.multicast_mac[i].mac_addr;

            if (ether_addr_equal(addr, mac_addr))/* 查询到此mac */
            {
                LOG_DEBUG("the mac is %pM\n", addr);
                /* 在地址数组中将此mac清空 */
                memset(&vf_item->vf_mac_info.multicast_mac[i], 0, ETH_ALEN);
                vf_item->vf_mac_info.current_multicast_num--;
                break;
            }
        }
    }

    return;
}
EXPORT_SYMBOL(zxdh_vf_item_mac_del);

static uint32_t zxdh_vf_mac_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t ret = 0;
    uint16_t sriov_vlan_tpid = vf_item->vlan_proto;
    uint16_t sriov_vlan_id = vf_item->vlan;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    ether_addr_copy(vf_item->vf_mac_info.unicast_mac[0].mac_addr, msg->mac_addr_set_msg.mac_addr);
    ret = dpp_add_mac(pf_info, msg->mac_addr_set_msg.mac_addr, sriov_vlan_tpid, sriov_vlan_id);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_add_mac failed, ret: %d\n", ret);
    }

    return ret;
}

static uint32_t zxdh_vf_mac_del(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t ret = 0;
    uint8_t i = 0;
    uint8_t *addr = NULL;
    uint16_t sriov_vlan_tpid = vf_item->vlan_proto;
    uint16_t sriov_vlan_id = vf_item->vlan;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    if (msg->mac_addr_set_msg.mac_flag)
    {
        ret = dpp_del_mac(pf_info, msg->mac_addr_set_msg.mac_addr, sriov_vlan_tpid, sriov_vlan_id);
        if (ret != 0)
        {
            LOG_ERR_DEV(dh_dev, "dpp_del_mac failed, ret: %d\n", ret);
            return ret;
        }
    }
    else if (msg->mac_addr_set_msg.mac_addr == vf_item->vf_mac_info.unicast_mac[0].mac_addr)
    {
        for (i = 1; i < DEV_UNICAST_MAX_NUM; ++i)
        {
            addr = vf_item->vf_mac_info.unicast_mac[i].mac_addr;
            if (is_zero_ether_addr(addr))
            {
                memcpy(addr, vf_item->vf_mac_info.unicast_mac[0].mac_addr, ETH_ALEN);
                break;
            }
        }
    }

    return ret;
}

static uint32_t zxdh_vf_filter_mac_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    uint16_t sriov_vlan_tpid = vf_item->vlan_proto;
    uint16_t sriov_vlan_id = vf_item->vlan;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    /* 将获取到的mac地址配置到NP中 */
    LOG_DEBUG("msg->mac_addr_set_msg.mac_addr is %pM\n",  msg->mac_addr_set_msg.mac_addr);

    err = dpp_add_mac(pf_info, msg->mac_addr_set_msg.mac_addr, sriov_vlan_tpid, sriov_vlan_id);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_add_mac failed \n");
        return err;
    }

    /* 将此mac地址添加到zxdh_vf_item */
    zxdh_vf_item_mac_add(vf_item, msg->mac_addr_set_msg.mac_addr,0);

    return err;
}

static uint32_t zxdh_vf_filter_mac_del(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    uint16_t sriov_vlan_tpid = vf_item->vlan_proto;
    uint16_t sriov_vlan_id = vf_item->vlan;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    /* 将获取到的mac地址从NP中删除 */
    LOG_DEBUG_DEV(dh_dev, "msg->mac_addr_set_msg.mac_addr is %pM\n", msg->mac_addr_set_msg.mac_addr);

    err = dpp_del_mac(pf_info, msg->mac_addr_set_msg.mac_addr, sriov_vlan_tpid, sriov_vlan_id);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_mac_del failed\n");
        return err;
    }

    /* 将此mac从zxdh_vf_item中删除 */
    zxdh_vf_item_mac_del(vf_item, msg->mac_addr_set_msg.mac_addr);

    return err;
}

static uint32_t zxdh_vf_multi_mac_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    /* 将此mac地址添加到np中 */
    LOG_DEBUG_DEV(dh_dev, "msg->mac_addr_set_msg.mac_addr is %pM\n", msg->mac_addr_set_msg.mac_addr);

    /* 判断是否超过本机组播mac数量 */
    if (vf_item->vf_mac_info.current_multicast_num >= VF_MAX_MULTICAST_MAC)
    {
        LOG_ERR_DEV(dh_dev, "vf multicast mac num:%u beyond %d\n", vf_item->vf_mac_info.current_multicast_num, VF_MAX_MULTICAST_MAC);
        return ZXDH_REPS_BEYOND_MAC;
    }

    err = dpp_multi_mac_add_member(pf_info, msg->mac_addr_set_msg.mac_addr);
    if (err != 0)
    {
        if (err == DPP_RC_TBL_IS_FULL)
        {
            LOG_ERR_DEV(dh_dev, "multicast mac is beyond whole transfer num\n");
            return ZXDH_REPS_BEYOND_MAC;
        }
        LOG_ERR_DEV(dh_dev, "dpp_multi_mac_add_member failed %d\n", err);
        return err;
    }

    /* 将此mac添加到zxdh_vf_item中 */
    zxdh_vf_item_mac_add(vf_item, msg->mac_addr_set_msg.mac_addr, 0);

    return err;
}

static uint32_t zxdh_vf_multi_mac_del(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    LOG_DEBUG_DEV(dh_dev, "msg->mac_addr_set_msg.mac_addr is %pM\n", msg->mac_addr_set_msg.mac_addr);

    /* 将此组播mac地址从np中删除 */
    err = dpp_multi_mac_del_member(pf_info, msg->mac_addr_set_msg.mac_addr);
    if (err != 0)
    {
        LOG_INFO_DEV(dh_dev, "dpp_multi_mac_del_member failed %d\n", err);
        return err;
    }

    /* 将此mac从zxdh_vf_item中删除 */
    zxdh_vf_item_mac_del(vf_item, msg->mac_addr_set_msg.mac_addr);

    return err;
}

static bool zxdh_check_item_mac_exists(DPP_PF_INFO_T *pf_info, struct zxdh_pf_device *pf_dev,
                                 uint16_t vf_idx, struct zxdh_vf_item *vf_item,
                                 const unsigned char *target_mac)
{
    uint32_t i = 0;
    struct zxdh_vf_item *cur_vf_item = NULL;
    uint16_t sriov_vlan_id = 0;
    uint16_t sriov_vlan_tpid = 0;
    uint16_t sriov_vlan_tci = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    struct pci_dev *pdev = dh_dev->pdev;
    int num_vfs = pci_num_vf(pdev);

    sriov_vlan_id = vf_item->vlan;
    sriov_vlan_tpid = vf_item->vlan_proto;
    sriov_vlan_tci = ZXDH_VLAN_TCI_GEN(vf_item->vlan, vf_item->qos);

    for (i = 0; i < num_vfs; i++)
    {
        if (i == vf_idx)
            continue;

        cur_vf_item = &pf_dev->vf_item[i];
        if (ether_addr_equal(cur_vf_item->mac, target_mac) && \
           ((ZXDH_VLAN_TCI_GEN(cur_vf_item->vlan, cur_vf_item->qos) == sriov_vlan_tci) && \
           (cur_vf_item->vlan_proto == sriov_vlan_tpid)))
        {
            LOG_INFO_DEV(dh_dev, "%s Mac already exists vf %d\n", __func__, i);
            return true;
        }
    }

    return false;
}

static uint32_t zxdh_vf_all_mac_dump(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    uint16_t current_vport = 0;
    uint16_t vport = vf_item->vport;
    uint16_t sriov_vlan_id = vf_item->vlan;
    uint16_t sriov_vlan_tpid = vf_item->vlan_proto;
    uint16_t vf_idx = msg->hdr.pcie_id & (0xff);
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    /* 遍历整个转发域，判断此转发域是否存在此单播mac地址 */
    err = dpp_unicast_mac_search(pf_info, msg->mac_addr_set_msg.mac_addr, sriov_vlan_tpid, sriov_vlan_id, &current_vport);
    if ((err == 0) && (vport == current_vport))
    {
        return 0;
    }
    else if ((err == 0) && (vport != current_vport))
    {
        LOG_ERR_DEV(dh_dev, "Mac already exists\n");
        return ZXDH_REPS_EXIST_MAC;
    }
    else if ((err != 0) && (err != DPP_HASH_RC_SRH_FAIL))
    {
       LOG_ERR_DEV(dh_dev, "dpp_unicast_mac_search failed, ret:%d\n", err);
       return 1;
    }

    if (zxdh_check_item_mac_exists(pf_info, pf_dev, vf_idx, vf_item, msg->mac_addr_set_msg.mac_addr))
    {
        LOG_ERR_DEV(dh_dev, "Mac already exists\n");
        return ZXDH_REPS_EXIST_MAC;
    }

    return 0;
}

static uint32_t zxdh_vf_all_mac_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    MAC_VPORT_INFO *p_mac_arr = NULL;
    uint32_t p_mac_num = 0;
    uint16_t current_vport = 0;
    uint16_t vport = vf_item->vport;
    uint16_t sriov_vlan_id = 0;
    uint16_t sriov_vlan_tpid = 0;
    uint32_t max_unicast_num = 0;
    uint16_t vf_idx = msg->hdr.pcie_id & (0xff);
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    /* 配置组播mac*/
    if (!is_unicast_ether_addr(msg->mac_addr_set_msg.mac_addr) && !is_link_local_ether_addr(msg->mac_addr_set_msg.mac_addr))
    {
        return zxdh_vf_multi_mac_add(msg, reps, pf_info, vf_item, pf_dev);
    }

    mutex_lock(&vf_item->lock);
    sriov_vlan_id = vf_item->vlan;
    sriov_vlan_tpid = vf_item->vlan_proto;

    /* 遍历整个转发域，判断此转发域是否存在此单播mac地址 */
    err = dpp_unicast_mac_search(pf_info, msg->mac_addr_set_msg.mac_addr, sriov_vlan_tpid, sriov_vlan_id, &current_vport);
    if ((err == 0) && (vport == current_vport))
    {
        mutex_unlock(&vf_item->lock);
        return 0;
    }
    else if ((err == 0) && (vport != current_vport))
    {
        LOG_ERR_DEV(dh_dev, "Mac already exists\n");
        mutex_unlock(&vf_item->lock);
        return ZXDH_REPS_EXIST_MAC;
    }
    else if ((err != 0) && (err != DPP_HASH_RC_SRH_FAIL))
    {
        LOG_ERR_DEV(dh_dev, "dpp_unicast_mac_search failed, ret:%d\n", err);
        mutex_unlock(&vf_item->lock);
        return 1;
    }

    if (zxdh_check_item_mac_exists(pf_info, pf_dev, vf_idx, vf_item, msg->mac_addr_set_msg.mac_addr))
    {
        LOG_ERR_DEV(dh_dev, "Mac already exists\n");
        mutex_unlock(&vf_item->lock);
        return ZXDH_REPS_EXIST_MAC;
    }

    /* 配置本机mac */
    if (msg->mac_addr_set_msg.filter_flag == UNFILTER_MAC)
    {
        err = zxdh_vf_mac_add(msg, reps, pf_info, vf_item, pf_dev);
        if (err != 0)
        {
            LOG_ERR_DEV(dh_dev, "zxdh_vf_mac_add failed\n");
        }
        mutex_unlock(&vf_item->lock);
        return err;
    }

    /* 判断是否超过本机单播mac数量 */
    if (vf_item->vf_mac_info.current_unicast_num >= VF_MAX_UNICAST_MAC)
    {
        LOG_ERR_DEV(dh_dev, "vf unicast mac num:%u beyond %d\n", vf_item->vf_mac_info.current_unicast_num, VF_MAX_UNICAST_MAC);
        mutex_unlock(&vf_item->lock);
        return ZXDH_REPS_BEYOND_MAC;
    }

    /* dump整个转发域已经配置的单播mac数量 */
    err = dpp_unicast_mac_dump(pf_info, p_mac_arr, &p_mac_num);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_unicast_mac_dump failed, ret:%d\n", err);
        mutex_unlock(&vf_item->lock);
        return err;
    }
    LOG_INFO_DEV(dh_dev, "%s p_mac_num is %d\n", __func__, p_mac_num);

    /* 获取当前pf级最大单播mac数量 */
    err = dpp_unicast_mac_max_get(pf_info, &max_unicast_num);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_unicast_mac_max_get failed %u\n", max_unicast_num);
        mutex_unlock(&vf_item->lock);
        return err;
    }
    /* 判断整个转发域配置的单播mac数量是否超过上限 */
    if (p_mac_num >= max_unicast_num)
    {
        LOG_ERR_DEV(dh_dev, "curr_all_unicast_num is beyond maximum\n");
        mutex_unlock(&vf_item->lock);
        return ZXDH_REPS_BEYOND_MAC;
    }

    /* 配置过滤用的单播mac */
    err = zxdh_vf_filter_mac_add(msg, reps, pf_info, vf_item, pf_dev);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_unicast_mac_dump failed\n");
    }
    mutex_unlock(&vf_item->lock);
    return err;
}
static uint32_t zxdh_vf_all_mac_del(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    /* 删除组播mac */
    if (!is_unicast_ether_addr(msg->mac_addr_set_msg.mac_addr) && !is_link_local_ether_addr(msg->mac_addr_set_msg.mac_addr))
    {
        return zxdh_vf_multi_mac_del(msg, reps, pf_info, vf_item, pf_dev);
    }

    mutex_lock(&vf_item->lock);
    /* 删除本机mac */
    if (msg->mac_addr_set_msg.filter_flag == UNFILTER_MAC)
    {
        err = zxdh_vf_mac_del(msg, reps, pf_info, vf_item, pf_dev);
        if (err != 0)
        {
            LOG_ERR_DEV(dh_dev, "zxdh_vf_mac_del failed\n");
        }
        mutex_unlock(&vf_item->lock);
        return err;
    }

    /* 删除过滤用的单播mac */
    err = zxdh_vf_filter_mac_del(msg, reps, pf_info, vf_item, pf_dev);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_vf_filter_mac_del failed\n");
    }

    mutex_unlock(&vf_item->lock);
    return err;
}

static uint32_t zxdh_vf_ipv6_mac_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    mutex_lock(&pf_dev->ip6mac_tbl->mlock);

    err = zxdh_vf_multi_mac_add(msg, reps, pf_info, vf_item, pf_dev);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_vf_multi_mac_add failed\n");
    }

    mutex_unlock(&pf_dev->ip6mac_tbl->mlock);

    return err;
}

static uint32_t zxdh_vf_ipv6_mac_del(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    mutex_lock(&pf_dev->ip6mac_tbl->mlock);

    err = zxdh_vf_multi_mac_del(msg, reps, pf_info, vf_item, pf_dev);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_vf_multi_mac_del failed\n");
    }

    mutex_unlock(&pf_dev->ip6mac_tbl->mlock);

    return err;
}


static uint32_t zxdh_vf_lacp_mac_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    err = zxdh_vf_multi_mac_add(msg, reps, pf_info, vf_item, pf_dev);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_vf_multi_mac_add failed\n");
    }

    return err;
}

static uint32_t zxdh_vf_lacp_mac_del(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    err = zxdh_vf_multi_mac_del(msg, reps, pf_info, vf_item, pf_dev);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_vf_multi_mac_del failed\n");
    }

    return err;
}

static uint32_t zxdh_vf_mac_get(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    ether_addr_copy(reps->vf_mac_addr_get_msg.mac_addr, vf_item->mac);
    return 0;
}

static uint32_t zxdh_vf_rx_num_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);
    mutex_lock(&vf_item->lock);
    vf_item->rx_num = msg->vf_rx_num_msg.rx_num;
    mutex_unlock(&vf_item->lock);

    LOG_DEBUG_DEV(dh_dev, "vf_item(vport: 0x%x), rx_num set to %u", vf_item->vport,
                                        msg->vf_rx_num_msg.rx_num);
    return 0;
}

static uint32_t zxdh_vf_rss_state_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_vport_rss_en_set(pf_info, msg->rss_enable_msg.rss_enable);
}

static uint32_t zxdh_vf_fd_state_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_vport_fd_en_set(pf_info, msg->vf_fd_enable_msg.fd_enable);
}

static uint32_t zxdh_vf_rxfh_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_rxfh_set(pf_info, msg->rxfh_set_msg.queue_map, ZXDH_INDIR_RQT_SIZE);
}

static uint32_t zxdh_vf_rxfh_get(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    err = dpp_rxfh_get(pf_info, reps->rxfh_get_msg.queue_map, ZXDH_INDIR_RQT_SIZE);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_rxfh_get failed: %d\n", err);
        return err;
    }

    return 0;
}

static uint32_t zxdh_vf_rxfh_del(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_rxfh_del(pf_info);
}

static uint32_t zxdh_vf_thash_key_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_thash_key_set(pf_info, msg->thash_key_set_msg.key_map, ZXDH_NET_HASH_KEY_SIZE);
}

static uint32_t zxdh_vf_thash_key_get(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_thash_key_get(pf_info, reps->thash_key_set_msg.key_map, ZXDH_NET_HASH_KEY_SIZE);
}

static uint32_t zxdh_vf_hash_funcs_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_vport_hash_funcs_set(pf_info, msg->hfunc_set_msg.func);
}

static uint32_t zxdh_vf_rx_flow_hash_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_vport_rx_flow_hash_set(pf_info, msg->rx_flow_hash_set_msg.hash_mode);
}

/**
 * zxdh_vf_switch_business_vlan - 配置bussness vlan子开关
 * @pf_info: pf信息
 * @type: vlan二级开关
 * @wanted_feature: 切换的值
 */
int zxdh_vf_switch_business_vlan(DPP_PF_INFO_T *pf_info, uint8_t type, uint32_t wanted_feature)
{
    int ret = 0;
    ZXDH_VQM_VFID_VLAN_T vf_vlan_attr = {0};
    bool old_vport_bit = 0;
    bool wanted_vport_bit = 0;
    uint32_t *changed_vlan_attr = NULL;
    /* sizeof(vf_vlan_attr)/sizeof(vf_vlan_attr.rsv): 结构体成员数量*/
    if (type >= sizeof(vf_vlan_attr)/sizeof(vf_vlan_attr.rsv))
    {
        LOG_ERR("zxdh_vf_switch_business_vlan para type err: %u.\n", type);
        return -1;
    }
    changed_vlan_attr = (uint32_t *)&vf_vlan_attr + type;
    ret = dpp_vqm_vfid_vlan_get(pf_info, &vf_vlan_attr);
    if (ret != 0)
    {
        LOG_ERR("dpp_vqm_vfid_vlan_get failed: %d.\n", ret);
        return -1;
    }
    old_vport_bit = vf_vlan_attr.sriov_business_qinq_vlan_strip_offload | vf_vlan_attr.sriov_business_vlan_filter | vf_vlan_attr.sriov_business_vlan_strip_offload;
    *changed_vlan_attr = wanted_feature;
    wanted_vport_bit = vf_vlan_attr.sriov_business_qinq_vlan_strip_offload | vf_vlan_attr.sriov_business_vlan_filter | vf_vlan_attr.sriov_business_vlan_strip_offload;

    /* 先将二级开关切换*/
    ret = dpp_vqm_vfid_vlan_set(pf_info, type, wanted_feature);
    if (ret != 0)
    {
        LOG_ERR("dpp_vqm_vfid_vlan_set, ret: %d\n", ret);
        return -1;
    }

    /* 如果vport没有发生变化*/
    if (!(old_vport_bit ^ wanted_vport_bit))
    {
        return 0;
    }

    ret = dpp_vport_business_vlan_offload_en_set(pf_info, wanted_vport_bit);
    if (ret != 0)
    {
        LOG_ERR("dpp_vport_business_vlan_offload_en_set, ret: %d\n", ret);
        return -1;
    }

    return 0;
}

static uint32_t zxdh_vf_vlan_strip_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    if (msg->vlan_strip_msg.flag == VLAN_STRIP_MSG_TYPE)
    {
        return zxdh_vf_switch_business_vlan(pf_info, VLAN_SRIOV_BUSINESS_VLAN_STRIP_OFFLIAD, msg->vlan_strip_msg.enable);
    }
    else
    {
        return zxdh_vf_switch_business_vlan(pf_info, VLAN_SRIOV_BUSINESS_QINQ_VLAN_STRIP_OFFLOAD, msg->vlan_strip_msg.enable);
    }
}

static uint32_t zxdh_vf_vxlan_offload_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t mcode_glb_cfg = 0;
    uint32_t ret = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    ret = dpp_glb_cfg_get_0(pf_info, &mcode_glb_cfg);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_pktrx_mcode_glb_cfg_get_0 failed: %d\n", ret);
        return -1;
    }

    mcode_glb_cfg = (mcode_glb_cfg & 0xFFFF0000) | msg->vf_vxlan_port.port;
    ret = dpp_glb_cfg_set_0(pf_info, mcode_glb_cfg);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_pktrx_mcode_glb_cfg_set_0 failed: %d\n", ret);
        return -1;
    }

    return 0;
}

static uint32_t zxdh_vf_vxlan_offload_del(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t mcode_glb_cfg = 0;
    uint16_t vxlan_port_cfg = 0;
    uint32_t ret = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    ret = dpp_glb_cfg_get_0(pf_info, &mcode_glb_cfg);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_pktrx_mcode_glb_cfg_get_0 failed: %d\n", ret);
        return -1;
    }

    vxlan_port_cfg = mcode_glb_cfg & 0x0000FFFF;
    if (vxlan_port_cfg != msg->vf_vxlan_port.port)
    {
        LOG_ERR_DEV(dh_dev, "del vxlan offload failed,port[%d] no equals to del_port[%d]\n", vxlan_port_cfg, msg->vf_vxlan_port.port);
        return -1;
    }

    mcode_glb_cfg = mcode_glb_cfg & 0xFFFF0000;
    ret = dpp_glb_cfg_set_0(pf_info, mcode_glb_cfg);
    if (ret != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_pktrx_mcode_glb_cfg_set_0 failed: %d\n", ret);
        return -1;
    }

    return 0;
}

static uint32_t zxdh_vf_qinq_tpid_cfg(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_vqm_vfid_vlan_set(pf_info, VLAN_SRIOV_BUSINESS_VLAN_TPID, msg->tpid_cfg_msg.tpid);
}

static uint32_t zxdh_vf_rx_flow_hash_get(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_vport_rx_flow_hash_get(pf_info, &reps->rx_flow_hash_set_msg.hash_mode);
}

static uint32_t zxdh_vf_port_attrs_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_vport_attr_set(pf_info, msg->port_attr_set_msg.mode, msg->port_attr_set_msg.value);
}

static uint32_t zxdh_vf_port_attrs_get(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    return dpp_vport_attr_get(pf_info, &reps->port_attr_get_msg.port_attr_entry);
}

static uint32_t zxdh_vf_promisc_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    if (msg->promisc_set_msg.mode == ZXDH_PROMISC_MODE)
    {
        LOG_INFO_DEV(dh_dev, "promisc config changed: promisc %d -> %d\n",
            vf_item->promisc, msg->promisc_set_msg.value);
        vf_item->promisc = msg->promisc_set_msg.value;
        if (msg->promisc_set_msg.mc_follow != 0)
        {
            LOG_INFO_DEV(dh_dev, "mc_promisc config changed: mc_promisc %d -> %d (mc_follow enabled)\n",
                vf_item->mc_promisc, msg->promisc_set_msg.value);
            vf_item->mc_promisc = msg->promisc_set_msg.value;
        }
    } else if (msg->promisc_set_msg.mode == ZXDH_ALLMULTI_MODE)
    {
        LOG_INFO_DEV(dh_dev, "mc_promisc config changed: %d -> %d, promisc: %d\n",
            vf_item->mc_promisc, msg->promisc_set_msg.value, vf_item->promisc);
        vf_item->mc_promisc = msg->promisc_set_msg.value;
    }
    if (!vf_item->trusted)
    {
        LOG_ERR_DEV(dh_dev, "vf untrusted!\n");
        return 0;
    }

    if (msg->promisc_set_msg.mode == ZXDH_PROMISC_MODE)
    {
        LOG_INFO_DEV(dh_dev, "PROMISC_EN_SET: %d", msg->promisc_set_msg.value);
        err = dpp_vport_uc_promisc_set(pf_info, msg->promisc_set_msg.value);
        if (err != 0)
        {
            LOG_ERR_DEV(dh_dev, "dpp_vport_uc_promisc_set failed: %d\n", err);
            return err;
        }
        err = dpp_vport_promisc_en_set(pf_info, msg->promisc_set_msg.value);
        if (err != 0)
        {
            LOG_ERR_DEV(dh_dev, "dpp_vport_promisc_en_set failed: %d\n", err);
            return err;
        }
        if (msg->promisc_set_msg.mc_follow != 0)
        {
            LOG_DEBUG("allmulti_follow\n");
            err = dpp_vport_mc_promisc_set(pf_info, msg->promisc_set_msg.value);
            if (err != 0)
            {
                LOG_ERR_DEV(dh_dev, "dpp_vport_mc_promisc_set failed: %d\n", err);
                return err;
            }
        }
    }
    else if (msg->promisc_set_msg.mode == ZXDH_ALLMULTI_MODE)
    {
        LOG_INFO_DEV(dh_dev, "ALLMULTI_EN_SET: %d", msg->promisc_set_msg.value);
        err = dpp_vport_mc_promisc_set(pf_info, msg->promisc_set_msg.value);
        if (err != 0)
        {
            LOG_ERR_DEV(dh_dev, "dpp_vport_mc_promisc_set failed: %d\n", err);
            return err;
        }
    }
    else
    {
        LOG_ERR_DEV(dh_dev, "promisc_set_msg.mode[%d] error\n", msg->promisc_set_msg.mode);
        return 1;
    }

    return err;
}

static uint32_t zxdh_vf_vlan_filter_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    bool vf_vlan_filter_enable = msg->vlan_filter_set_msg.enable;

    return zxdh_vf_switch_business_vlan(pf_info, VLAN_SRIOV_BUSINESS_VLAN_FILTER, vf_vlan_filter_enable);
}

static uint32_t zxdh_vf_rx_vid_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint16_t vid = msg->rx_vid_add_msg.vlan_id;

    return dpp_add_vlan_filter(pf_info, vid);
}

static uint32_t zxdh_vf_rx_vid_del(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint16_t vid = msg->rx_vid_del_msg.vlan_id;

    return dpp_del_vlan_filter(pf_info, vid);
}

static uint32_t zxdh_vf_np_stats_get(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    uint32_t vf_id = msg->hdr.vf_id;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    LOG_DEBUG_DEV(dh_dev, "zxdh_vf_np_stats_get is called, vport: 0x%x, vf_id %d, pf_info.slot %u\n", pf_info->vport, vf_id, pf_info->slot);
    dpp_stat_port_uc_packet_rx_cnt_get(pf_info, vf_id, msg->np_stats_get_msg.clear_mode, &(reps->np_stats_msg.np_rx_vport_unicast_bytes), &(reps->np_stats_msg.np_rx_vport_unicast_packets));
    dpp_stat_port_uc_packet_tx_cnt_get(pf_info, vf_id, msg->np_stats_get_msg.clear_mode, &(reps->np_stats_msg.np_tx_vport_unicast_bytes), &(reps->np_stats_msg.np_tx_vport_unicast_packets));
    dpp_stat_port_mc_packet_rx_cnt_get(pf_info, vf_id, msg->np_stats_get_msg.clear_mode, &(reps->np_stats_msg.np_rx_vport_multicast_bytes), &(reps->np_stats_msg.np_rx_vport_multicast_packets));
    dpp_stat_port_mc_packet_tx_cnt_get(pf_info, vf_id, msg->np_stats_get_msg.clear_mode, &(reps->np_stats_msg.np_tx_vport_multicast_bytes), &(reps->np_stats_msg.np_tx_vport_multicast_packets));
    dpp_stat_port_bc_packet_rx_cnt_get(pf_info, vf_id, msg->np_stats_get_msg.clear_mode, &(reps->np_stats_msg.np_rx_vport_broadcast_bytes), &(reps->np_stats_msg.np_rx_vport_broadcast_packets));
    dpp_stat_port_bc_packet_tx_cnt_get(pf_info, vf_id, msg->np_stats_get_msg.clear_mode, &(reps->np_stats_msg.np_tx_vport_broadcast_bytes), &(reps->np_stats_msg.np_tx_vport_broadcast_packets));
    dpp_stat_MTU_packet_msg_rx_cnt_get(pf_info, vf_id, msg->np_stats_get_msg.clear_mode, &(reps->np_stats_msg.np_rx_vport_mtu_drop_bytes), &(reps->np_stats_msg.np_rx_vport_mtu_drop_packets));
    dpp_stat_MTU_packet_msg_tx_cnt_get(pf_info, vf_id, msg->np_stats_get_msg.clear_mode, &(reps->np_stats_msg.np_tx_vport_mtu_drop_bytes), &(reps->np_stats_msg.np_tx_vport_mtu_drop_packets));
    dpp_stat_plcr_packet_drop_rx_cnt_get(pf_info, vf_id, msg->np_stats_get_msg.clear_mode, &(reps->np_stats_msg.np_rx_vport_plcr_drop_bytes), &(reps->np_stats_msg.np_rx_vport_plcr_drop_packets));
    dpp_stat_plcr_packet_drop_tx_cnt_get(pf_info, vf_id, msg->np_stats_get_msg.clear_mode, &(reps->np_stats_msg.np_tx_vport_plcr_drop_bytes), &(reps->np_stats_msg.np_tx_vport_plcr_drop_packets));
    reps->np_stats_msg.np_tx_vport_ssvpc_packets = 0;
    reps->np_stats_msg.rx_vport_idma_drop_packets = 0; //这里VF来查询统计，对VF不存在的统计需要清零，否则可能是随机数。
    err = zxdh_get_np_fd_stats(pf_info, msg->np_stats_get_msg.clear_mode, msg->np_stats_get_msg.fd_enable, &(reps->np_stats_msg.np_rx_vport_fdir_hits_bytes), &(reps->np_stats_msg.np_rx_vport_fdir_hits_packets), \
                        &(reps->np_stats_msg.np_rx_vport_fdir_drop_bytes), &(reps->np_stats_msg.np_rx_vport_fdir_drop_packets));
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_get_np_fd_stats failed!\n");
        return err;
    }

    if(msg->np_stats_get_msg.is_init_get)
    {
        memcpy(vf_item->init_np_stats, &reps->np_stats_msg, sizeof(struct zxdh_en_vport_np_stats));
    }

    return 0;
}

static uint32_t zxdh_vf_rate_limit_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t  rtn;
    uint16_t vport     = msg->hdr.vport;
    uint32_t flowid    = msg->rate_limit_set_msg.flowid;
    uint32_t car_type  = msg->rate_limit_set_msg.car_type;
    uint32_t max_rate  = msg->rate_limit_set_msg.max_rate;
    uint32_t min_rate  = msg->rate_limit_set_msg.min_rate;
    uint32_t is_packet = msg->rate_limit_set_msg.is_packet;

    // PLCR_FUNC_DBG_ENTER();

    rtn = zxdh_plcr_set_rate_limit(pf_dev, is_packet, car_type, vport, flowid, max_rate, min_rate);
    reps->rate_limit_set_rsp.err_code = rtn;

    if (PLCR_REMOVE_RATE_LIMIT == rtn || PLCR_DUPLICATE_RATE == rtn)
    {
        return 0;
    }
    else
    {
        return rtn;
    }
}

static uint32_t zxdh_vf_plcr_uninit(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint16_t        vport;
    unsigned long   flow_id;
    E_PLCR_CAR_TYPE car_index;
    struct xarray  *xarray_flow;
    struct zxdh_plcr_flow *flow = NULL;

    // PLCR_FUNC_DBG_ENTER();

    vport = msg->hdr.vport;

    //deal with car A's flowid
    for(car_index=E_PLCR_CAR_A; car_index <= E_PLCR_CAR_B; car_index++)
    {
        xarray_flow = &(pf_dev->plcr_table.plcr_flows[car_index]);
        xa_for_each_range(xarray_flow, flow_id, flow, 0, gaudPlcrCarxFlowIdNum[car_index])
        {
            if(flow->vport == vport)
            {
                zxdh_plcr_remove_rate_limit(pf_dev, car_index, (uint32_t)flow_id, 0);

                //clear vport mappings between car B and car C.
                if(E_PLCR_CAR_B == car_index)
                {
                    zxdh_plcr_clear_map(pf_dev, car_index, flow_id);
                }
            }
        }
    }

    zxdh_plcr_count_profiles(pf_dev);

    return 0;
}

static uint32_t zxdh_vf_plcr_flowid_map(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t  rtn        = 0;
    uint32_t car_type   = 0;
    uint32_t flowid     = 0;
    uint32_t map_flowid = 0;
    uint32_t map_sp     = 0;
    struct dh_core_dev *dh_dev = NULL;

    /*提取消息中的参数字段*/
    car_type    = msg->plcr_flowid_map_msg.car_type;
    flowid      = msg->plcr_flowid_map_msg.flowid;
    map_flowid  = msg->plcr_flowid_map_msg.map_flowid;
    map_sp      = msg->plcr_flowid_map_msg.sp;
    dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    //前面的流程会判断是否需要进行映射，不会出现vf端口原来是非0group，现在会被group 0覆盖的情况
    PLCR_LOG_DEBUG_DEV(dh_dev, "dpp_car_queue_map_set: pf_info->vport = 0x%x, car_type = %d, flowid = %d, map_flowid = %d\n", pf_info->vport, car_type, flowid, map_flowid);
    rtn = dpp_car_queue_map_set(pf_info, car_type, flowid, map_flowid, map_sp);
    PLCR_COMM_ASSERT(rtn);

    zxdh_plcr_stroe_map(pf_dev, car_type, flowid, map_flowid);

    return 0;
}

static uint32_t zxdh_vf_plcr_get_mode(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t rtn = 0;
    uint16_t vport = 0;
    E_RATE_LIMIT_MODE mode = 0;

    /*提取消息中的参数字段*/
    vport  = msg->plcr_work_mode_msg.vport;

    /*获取plcr vport的工作模式*/
    rtn = zxdh_pf_plcr_get_mode(pf_dev, vport, &mode);
    PLCR_COMM_ASSERT(rtn);

    reps->plcr_work_mode_rsp.mode = mode;

    return rtn;
}

static uint32_t zxdh_vf_plcr_set_mode(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t  rtn = 0;
    uint16_t vport = 0;
    E_RATE_LIMIT_MODE mode = 0;

    /*提取消息中的参数字段*/
    vport  = msg->plcr_work_mode_msg.vport;
    mode   = msg->plcr_work_mode_msg.mode;

    /*设置plcr vport的工作模式*/
    rtn = zxdh_pf_plcr_set_mode(pf_dev, vport, mode);
    PLCR_COMM_ASSERT(rtn);

    return rtn;
}

static uint32_t zxdh_vf_plcr_flow_init(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int rtn = 0;
    uint32_t car_type;
    uint32_t flowid;
    struct dh_core_dev *dh_dev = NULL;

    car_type = msg->plcr_flow_init_msg.car_type;
    flowid = msg->plcr_flow_init_msg.flowid;
    pf_info->slot = pf_dev->slot_id;
    pf_info->vport = pf_dev->vport;
    dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    //对于vf队列限速，carC flow的初始化时，group_id为0，不需检查目标group中num_vfs是否为0，直接进行初始化

    PLCR_LOG_DEBUG_DEV(dh_dev, "dpp_car_queue_cfg_set: vport = 0x%x, car_type = %d, flowid = %d, plcr_en = 0\n",
                    pf_dev->vport, car_type, flowid);
    rtn = dpp_car_queue_cfg_set(pf_info, (uint32_t)car_type, flowid, DROP_DISABLE, PLCR_DISABLE, 0);
    if (rtn)
    {
        PLCR_LOG_ERR("failed to call dpp_car_queue_cfg_set()\n");
    }

    return rtn;
}

static uint32_t zxdh_vf_plcr_profile_id_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t                     rtn        = 0;
    uint32_t                    car_type   = 0;
    uint16_t                    profile_id = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    /*1. 提取消息中的模板参数*/
    car_type = msg->vf_plcr_profile_id_add_msg.car_type;

    /*2. 申请新的profile*/
    rtn = zxdh_plcr_req_profile(pf_dev, car_type, &profile_id);
    if (rtn)
    {
        LOG_ERR_DEV(dh_dev, "%s-%d : failed !\n", __FUNCTION__, __LINE__);
        return rtn;
    }

    /*3. 返回消息*/
    reps->vf_plcr_profile_id_add_rsp.profile_id = profile_id;

    return 0;
}

static uint32_t zxdh_vf_plcr_profile_id_delete(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t  rtn        = 0;
    uint32_t car_type   = 0;
    uint16_t profile_id = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    /*1. 提取消息中的模板参数*/
    car_type   = msg->vf_plcr_profile_id_delete_msg.car_type;
    profile_id = msg->vf_plcr_profile_id_delete_msg.profile_id;

    /*2. 限速模板使用计数-1：在下面的解除绑定zxdh_vf_plcr_queue_cfg_set()负责对使用计数-1*/

    /*3. 释放限速模板资源*/
    rtn = zxdh_plcr_release_profile(pf_dev, car_type, profile_id, 0);
    if (rtn)
    {
        LOG_ERR_DEV(dh_dev, "%s-%d : failed !\n", __FUNCTION__, __LINE__);
        return rtn;
    }

    /*4. 没有返回消息*/

    return rtn;
}

static uint32_t zxdh_vf_plcr_profile_cfg_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t                     rtn            = 0;
    uint32_t                    car_type       = 0;
    uint32_t                    pkt_mode       = 0;
    uint16_t                    profile_id     = 0;
    uint32_t                    max_rate       = 0;
    uint32_t                    min_rate       = 0;
    struct xarray              *xarray_profile = NULL;
    struct zxdh_plcr_profile   *plcr_profile   = NULL;
    union zxdh_plcr_profile_cfg profile_cfg;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    /*1. 提取消息中的模板参数*/
    car_type    = msg->vf_plcr_profile_cfg_set_msg.car_type;
    pkt_mode    = msg->vf_plcr_profile_cfg_set_msg.pkt_mode;
    profile_id  = msg->vf_plcr_profile_cfg_set_msg.profile_id;

    /*2. 校验外面的profile id和里面的profile id必须一致：限速模板联合体下包限速和字节限速结构体的前面2个成员是一样的（profile_id和pkt_sign）*/
    if(profile_id != msg->vf_plcr_profile_cfg_set_msg.profile_cfg.byte_profile_cfg.profile_id)
    {
        LOG_ERR_DEV(dh_dev, "%s-%d : failed\n", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    /*3. 校验外面的pkt_mode和里面的pkt_sign必须一致：限速模板联合体下包限速和字节限速结构体的前面2个成员是一样的（profile_id和pkt_sign）*/
    if(pkt_mode != msg->vf_plcr_profile_cfg_set_msg.profile_cfg.byte_profile_cfg.pkt_sign)
    {
        LOG_ERR_DEV(dh_dev, "%s-%d : failed\n", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    /*2. 获取profile*/
    xarray_profile = &(pf_dev->plcr_table.plcr_profiles[car_type]);
    plcr_profile   = xa_load(xarray_profile, profile_id);

    /*3. 包限速处理*/
    if(1 ==pkt_mode)
    {
        /*3.1 获取用户传递的字节限速模板参数*/
        profile_cfg.pkt_profile_cfg = msg->vf_plcr_profile_cfg_set_msg.profile_cfg.pkt_profile_cfg;

        /*3.2 包模式下直接使用限速模板里的包限速值*/
        max_rate = profile_cfg.pkt_profile_cfg.cir;
        min_rate = profile_cfg.pkt_profile_cfg.cir;

        /*3.2 将限速模板的参数，配置到寄存器中去：包限速模板和字节限速模板前面2个成员profile_id和pkt_sign一样，保证了下面接口可以兼容包和字节限速配置*/
        rtn = zxdh_plcr_cfg_profile(pf_dev, car_type, &profile_cfg.byte_profile_cfg);
        if (rtn)
        {
            LOG_ERR_DEV(dh_dev, "%s-%d : failed\n", __FUNCTION__, __LINE__);
            return -EINVAL;
        }
    }
    /*4. 字节限速处理*/
    else
    {
        /*4.1 获取用户传递的字节限速模板参数*/
        profile_cfg.byte_profile_cfg = msg->vf_plcr_profile_cfg_set_msg.profile_cfg.byte_profile_cfg;

        /*4.2 将寄存器中的配置值，转换成用户限速值（单位：Mbit/s）*/
        max_rate = zxdh_plcr_reg_maxrate_user(profile_cfg.byte_profile_cfg.eir);
        min_rate = zxdh_plcr_reg_maxrate_user(profile_cfg.byte_profile_cfg.cir);

        /*4.3 将限速模板的参数，配置到寄存器中去*/
        rtn = zxdh_plcr_cfg_profile(pf_dev, car_type, &profile_cfg.byte_profile_cfg);
        if (rtn)
        {
            LOG_ERR_DEV(dh_dev, "%s-%d : failed\n", __FUNCTION__, __LINE__);
            return -EINVAL;
        }
    }

    /*5. 将限速模板参数保存到profile下面*/
    rtn = zxdh_plcr_store_profile(pf_dev, car_type, max_rate, min_rate, &profile_cfg.byte_profile_cfg);
    if (rtn)
    {
        LOG_ERR_DEV(dh_dev, "%s-%d : failed\n", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    /*6. 没有返回消息*/

    return 0;
}

static uint32_t zxdh_vf_plcr_profile_cfg_get(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t  rtn        = 0;
    uint32_t car_type   = 0;
    uint32_t pkt_mode   = 0;
    uint16_t profile_id = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    /*1. 提取消息中的模板参数*/
    car_type    = msg->vf_plcr_profile_cfg_get_msg.car_type;
    pkt_mode    = msg->vf_plcr_profile_cfg_get_msg.pkt_mode;
    profile_id  = msg->vf_plcr_profile_cfg_get_msg.profile_id;

    /*2. 从寄存器中，获取限速模板的参数*/
    rtn = zxdh_plcr_get_profile(pf_dev, car_type, pkt_mode, profile_id, &reps->vf_plcr_profile_cfg_get_rsp.profile_cfg.byte_profile_cfg);
    if (rtn)
    {
        LOG_ERR_DEV(dh_dev, "%s-%d : failed to call zxdh_plcr_cfg_profile()\n", __FUNCTION__, __LINE__);
        return rtn;
    }

    /*3. 返回消息：上面已经填充好返回消息*/

    return 0;
}

static uint32_t zxdh_vf_plcr_queue_cfg_set(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t  rtn        = 0;
    uint32_t car_type   = 0;
    uint32_t drop_flag  = 0;
    uint32_t plcr_en    = 0;
    uint32_t flow_id    = 0;
    uint32_t profile_id = 0;
    uint16_t vport      = msg->hdr.vport;
    struct xarray            *xarray_profile = NULL;
    struct zxdh_plcr_flow    *plcr_flow      = NULL;
    struct zxdh_plcr_profile *plcr_profile   = NULL;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    LOG_INFO_DEV(dh_dev, "%s-%d:vport = 0x%x\n", __FUNCTION__, __LINE__, vport);

    /*1. 提取消息中的模板参数*/
    car_type    = msg->vf_plcr_queue_cfg_set_msg.car_type;
    drop_flag   = msg->vf_plcr_queue_cfg_set_msg.drop_flag;
    plcr_en     = msg->vf_plcr_queue_cfg_set_msg.plcr_en;
    flow_id     = msg->vf_plcr_queue_cfg_set_msg.flow_id;
    profile_id  = msg->vf_plcr_queue_cfg_set_msg.profile_id;

    /*2. 获取xarray中存储的profile*/
    xarray_profile = &(pf_dev->plcr_table.plcr_profiles[car_type]);
    plcr_profile   = xa_load(xarray_profile, profile_id);
    if(NULL == plcr_profile)
    {
        LOG_ERR_DEV(dh_dev, "%s-%d : failed\n", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    /*3. 绑定flow和profile*/
    if(PLCR_ENABLE == plcr_en)
    {
        /*3.1 申请flow结构体，并存储到xarray*/
        rtn = zxdh_plcr_req_flow(pf_dev, car_type, flow_id, &plcr_flow);
        if (rtn)
        {
            LOG_ERR_DEV(dh_dev, "%s-%d : kzalloc failed\n", __FUNCTION__, __LINE__);
            return -EINVAL;
        }

        /*3.2 更新flow信息*/
        zxdh_plcr_update_flow(plcr_flow, vport, plcr_profile->max_rate, plcr_profile->min_rate);

        /*3.3 更新flow信息*/
        plcr_flow->profile_id = profile_id;

        /*3.4 将flow与profile进行绑定*/
        rtn = dpp_car_queue_cfg_set(pf_info, car_type, flow_id, drop_flag, plcr_en, profile_id);
        if (rtn)
        {
            LOG_ERR_DEV(dh_dev, "%s-%d : failed to call dpp_car_queue_cfg_set()\n", __FUNCTION__, __LINE__);

            /*释放先前申请的flow*/
            zxdh_plcr_release_flow(pf_dev, car_type, flow_id);
            return -EINVAL;
        }

        /*3.4 模板使用计数+1*/
        zxdh_plcr_count_up_profile(pf_dev, car_type, profile_id);
    }
    /*4. 解除绑定flow和profile*/
    else
    {
        /*4.1 解除绑定flow与profile*/
        rtn = dpp_car_queue_cfg_set(pf_info, car_type, flow_id, drop_flag, plcr_en, profile_id);
        if (rtn)
        {
            LOG_ERR_DEV(dh_dev, "%s-%d : failed to call dpp_car_queue_cfg_set()\n", __FUNCTION__, __LINE__);
            return -EINVAL;
        }

        /*4.2 释放flow*/
        zxdh_plcr_release_flow(pf_dev, car_type, flow_id);

        /*4.3 模板使用计数-1*/
        zxdh_plcr_count_down_profile(pf_dev, car_type, profile_id);
    }

    /*5. 没有返回消息*/

    return 0;
}

static uint32_t zxdh_vf_plcr_port_meter_stat_clr(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t  rtn     = 0;
    uint64_t pkB_cnt = 0;
    uint64_t pk_cnt  = 0;

    /*1. dpp接口函数原型如下：mode = 1，表示读清零*/
    dpp_stat_plcr_packet_drop_tx_cnt_get(pf_info, msg->hdr.vf_id, 1, &pkB_cnt, &pk_cnt);
    dpp_stat_plcr_packet_drop_rx_cnt_get(pf_info, msg->hdr.vf_id, 1, &pkB_cnt, &pk_cnt);

    /*2. 没有返回消息*/
    return rtn;
}

static uint32_t zxdh_vf_plcr_port_meter_stat_get(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t  rtn        = 0;
    uint32_t direction  = 0;
    uint32_t is_clr     = 0;
    uint64_t *p_pkB_cnt = NULL;
    uint64_t *p_pk_cnt  = NULL;

    /*1. 提取消息中的参数*/
    direction = msg->vf_plcr_port_meter_stat_get_msg.direction;
    is_clr    = msg->vf_plcr_port_meter_stat_get_msg.is_clr;

    /*2. 获取返回信息地址*/
    p_pkB_cnt = &(reps->vf_plcr_port_meter_stat_get_rsp.drop_pkB_cnt);
    p_pk_cnt  = &(reps->vf_plcr_port_meter_stat_get_rsp.drop_pk_cnt);

    /*2. 获取丢包统计*/
    if(1 == direction)
    {
        dpp_stat_plcr_packet_drop_tx_cnt_get(pf_info, msg->hdr.vf_id, is_clr, p_pkB_cnt, p_pk_cnt);
    }
    else
    {
        dpp_stat_plcr_packet_drop_rx_cnt_get(pf_info, msg->hdr.vf_id, is_clr, p_pkB_cnt, p_pk_cnt);
    }

    /*3. 返回丢包统计值*/

    return rtn;
}

static uint32_t zxdh_vf_call_np_1588(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t vfid = msg->vf_1588_call_np.vfid;
    uint32_t interface_num = msg->vf_1588_call_np.call_np_interface_num;
    uint32_t opt = msg->vf_1588_call_np.ptp_tc_enable_opt;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    switch (interface_num)
    {
        case PTP_PORT_VFID_SET:
        {
            LOG_INFO_DEV(dh_dev, "call dpp_ptp_port_vfid_set\n");
            dpp_ptp_port_vfid_set(pf_info, vfid);
            break;
        }
        case PTP_TC_ENABLE_SET:
        {
            LOG_INFO_DEV(dh_dev, "call dpp_ptp_tc_enable_set\n");
            dpp_ptp_tc_enable_set(pf_info, opt);
            break;
        }
        default:
        {
            LOG_ERR_DEV(dh_dev, "cannot found the interface_num %u\n", interface_num);
            return -1;
        }
    }

    return 0;
}

static uint32_t zxdh_vf_slot_id_get(zxdh_msg_info *msg, zxdh_reps_info *reps,
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    reps->slot_info.slot_id = pf_dev->slot_id;
    return 0;
}

static uint32_t zxdh_vf_mcode_feature_get(zxdh_msg_info *msg, zxdh_reps_info *reps,
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    reps->mcode_feature_rsp.len = sizeof(reps->mcode_feature_rsp.feature);
    reps->mcode_feature_rsp.feature = pf_dev->mcode_feature;
    return 0;
}

static uint32_t zxdh_vf_k_cmpat_get(zxdh_msg_info *msg, zxdh_reps_info *reps,
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    reps->kernel_cmpat_rsp.k_msg_idmax = ZXDH_MSG_TYPE_CNT_MAX;
    return 0;
}

static uint32_t zxdh_vf_1588_enable_proc(zxdh_msg_info *msg, zxdh_reps_info *reps,
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t proc_cmd = 0;
    uint32_t enable = 0;
    uint32_t ret = 0;
    ZXDH_SRIOV_VPORT_T port_attr_entry = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    proc_cmd = msg->vf_1588_enable.proc_cmd;
    switch (proc_cmd)
    {
        case ZXDH_VF_1588_ENABLE_SET:
        {
            enable = msg->vf_1588_enable.enable_1588_vf;
            pf_info->vport = msg->hdr.vport;
            ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_1588_EN, (uint32_t)enable);
            if (ret != 0)
            {
                LOG_ERR_DEV(dh_dev, "dpp_vport_attr_set SRIOV_VPORT_1588_EN failed, ret:%d\n", ret);
                return ret;
            }
            break;
        }
        case ZXDH_VF_1588_ENABLE_GET:
        {
            pf_info->vport = msg->hdr.vport;
            ret = dpp_vport_attr_get(pf_info, &port_attr_entry);
            if (ret != 0)
            {
                LOG_ERR_DEV(dh_dev, "dpp_vport_attr_get SRIOV_VPORT_1588_EN failed, ret:%d\n", ret);
                return ret;
            }
            reps->vf_1588_enable_rsp.enable_1588_vf_rsp = port_attr_entry.flag_1588_enable;
            break;
        }
        default:
        {
            LOG_ERR_DEV(dh_dev, "cannot found proc_cmd %u\n", proc_cmd);
            break;
        }
    }
    return 0;
}

static uint32_t zxdh_vf_rsskey_ipid_proc(zxdh_msg_info *msg, zxdh_reps_info *reps,
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t proc_cmd = 0;
    uint32_t enable = 0;
    uint32_t ret = 0;
    ZXDH_SRIOV_VPORT_T port_attr_entry = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    proc_cmd = msg->vf_rsskey_ipid.proc_cmd;
    switch (proc_cmd)
    {
        case ZXDH_VF_RSSKEY_IPID_SET:
        {
            enable = msg->vf_rsskey_ipid.enable_vf_rsskey_ipid;
            pf_info->vport = msg->hdr.vport;
            ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_FRAG_PKT_USE_IPID, (uint32_t)enable);
            if (ret != 0)
            {
                LOG_ERR_DEV(dh_dev, "dpp_vport_attr_set SRIOV_VPORT_FRAG_PKT_USE_IPID failed, ret:%d\n", ret);
                return ret;
            }
            break;
        }
        case ZXDH_VF_RSSKEY_IPID_GET:
        {
            pf_info->vport = msg->hdr.vport;
            ret = dpp_vport_attr_get(pf_info, &port_attr_entry);
            if (ret != 0)
            {
                LOG_ERR_DEV(dh_dev, "dpp_vport_attr_get SRIOV_VPORT_FRAG_PKT_USE_IPID failed, ret:%d\n", ret);
                return ret;
            }
            reps->vf_rsskey_ipid_rsp.enable_rsskey_ipid_rsp = port_attr_entry.frag_pkt_use_ipid;
            break;
        }
        default:
        {
            LOG_ERR_DEV(dh_dev, "cannot found proc_cmd %u\n", proc_cmd);
            break;
        }
    }
    return 0;
}

static uint32_t zxdh_vf_flow_hw_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t err = 0;
    uint32_t handle = 0;
    uint8_t *key = NULL;
    uint8_t *key_mask = NULL;
    uint8_t *result = NULL;
    int32_t vf_idx = 0;
    uint8_t eth_type_bit = 0;
    uint16_t sriov_tunnel_encap0_index = 0;
    uint16_t sriov_tunnel_encap1_index = 0;
    zxdh_flow_op_msg *f_msg = &msg->flow_msg;
    zxdh_flow_op_rsp *f_rsp = &reps->flow_rsp;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    vf_idx = msg->hdr.pcie_id & (0xff);
    // 申请handle
    err = dpp_fd_acl_index_request(pf_info, &handle);
    if (err)
    {
        LOG_ERR_DEV(dh_dev, "failed to request index!!!\n");
        /* Started by AICoder, pid:u97cfh8123k1f9c14867087ab092ec03e7f1b105 */
        zte_strncpy_s(f_rsp->error.reason, "failed to request index!!!", sizeof(f_rsp->error.reason)-1);
        f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
        /* Ended by AICoder, pid:u97cfh8123k1f9c14867087ab092ec03e7f1b105 */
        return -EINVAL;
    }

    // 配置规则
    key = (uint8_t *)&f_msg->dh_flow.flowentry.fd_flow.key;
    key_mask = (uint8_t *) &f_msg->dh_flow.flowentry.fd_flow.key_mask;
    if ((f_msg->dh_flow.flowentry.fd_flow.result.action_idx & (1 << FD_ACTION_COUNT_BIT)) != 0)
    {
        f_msg->dh_flow.flowentry.fd_flow.result.countid = handle;
    }
    result = (uint8_t *)&f_msg->dh_flow.flowentry.fd_flow.result;

    if ((f_msg->dh_flow.flowentry.fd_flow.result.action_idx & (1 << FD_ACTION_VXLAN_ENCAP)) != 0)
    {
        f_msg->dh_flow.flowentry.fd_flow.result.sriov_tunnel_encap0_index = handle;
        sriov_tunnel_encap0_index = handle;

        if (vf_idx < (ZXDH_MAX_VF - 1))
        {
            f_msg->dh_flow.flowentry.fd_flow.result.sriov_tunnel_encap1_index = f_msg->dh_flow.hash_search_idx * PF_HAS_MAX_ENCAP1_NUM  + vf_idx + 1;
        }
        else
        {
            LOG_ERR_DEV(dh_dev, "encap1 vf_index is too big:%d\n", vf_idx);
            zte_strncpy_s(f_rsp->error.reason, "encap1 vf_index is too big!!!", sizeof(f_rsp->error.reason)-1);
            f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
            return -EINVAL;
        }

        sriov_tunnel_encap1_index = f_msg->dh_flow.flowentry.fd_flow.result.sriov_tunnel_encap1_index;
        eth_type_bit = f_msg->encap0.eth_type;
        err = dpp_eram_entry_insert(pf_info, ZXDH_SDT_TUNNEL_ENCAP0_TABLE, sriov_tunnel_encap0_index * 2, (uint8_t *)&(f_msg->encap0));
        if (err)
        {
            LOG_ERR_DEV(dh_dev, "dpp_eram_entry_insert encap0 table failed\n");
            zte_strncpy_s(f_rsp->error.reason, "dpp_eram_entry_insert encap0 table failed", sizeof(f_rsp->error.reason)-1);
            f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
            return -EINVAL;
        }

        err = dpp_eram_entry_insert(pf_info, ZXDH_SDT_TUNNEL_ENCAP0_TABLE, sriov_tunnel_encap0_index * 2 + 1, (uint8_t *)&(f_msg->encap0.dip));
        if (err)
        {
            LOG_ERR_DEV(dh_dev, "dpp_eram_entry_insert encap0 dip table failed\n");
            zte_strncpy_s(f_rsp->error.reason, "dpp_eram_entry_insert encap0 dip table failed", sizeof(f_rsp->error.reason)-1);
            f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
            return -EINVAL;
        }

        if (eth_type_bit == 0)
        {
            err = dpp_eram_entry_insert(pf_info, ZXDH_SDT_TUNNEL_ENCAP1_TABLE, sriov_tunnel_encap1_index * 4, (uint8_t *)&(f_msg->encap1));
            if (err)
            {
                LOG_ERR_DEV(dh_dev, "dpp_eram_entry_insert ipv4 encap1 table failed\n");
                zte_strncpy_s(f_rsp->error.reason, "dpp_eram_entry_insert ipv4 encap1 table failed", sizeof(f_rsp->error.reason)-1);
                f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
                return -EINVAL;
            }

            err = dpp_eram_entry_insert(pf_info, ZXDH_SDT_TUNNEL_ENCAP1_TABLE, sriov_tunnel_encap1_index * 4 + 2, (uint8_t *)&(f_msg->encap1.sip));
            if (err)
            {
                LOG_ERR_DEV(dh_dev, "dpp_eram_entry_insert ipv4 encap1 sip table failed\n");
                zte_strncpy_s(f_rsp->error.reason, "dpp_eram_entry_insert ipv4 encap1 sip table failed", sizeof(f_rsp->error.reason)-1);
                f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
                return -EINVAL;
            }
        }
        else
        {
            err = dpp_eram_entry_insert(pf_info, ZXDH_SDT_TUNNEL_ENCAP1_TABLE, sriov_tunnel_encap1_index * 4 + 1, (uint8_t *)&(f_msg->encap1));
            if (err)
            {
                LOG_ERR_DEV(dh_dev, "dpp_eram_entry_insert ipv6 encap1 table failed\n");
                zte_strncpy_s(f_rsp->error.reason, "dpp_eram_entry_insert ipv6 encap1 table failed", sizeof(f_rsp->error.reason)-1);
                f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
                return -EINVAL;
            }

            err = dpp_eram_entry_insert(pf_info, ZXDH_SDT_TUNNEL_ENCAP1_TABLE, sriov_tunnel_encap1_index * 4 + 3, (uint8_t *)&(f_msg->encap1.sip));
            if (err)
            {
                LOG_ERR_DEV(dh_dev, "dpp_eram_entry_insert ipv6 encap1 sip table failed\n");
                zte_strncpy_s(f_rsp->error.reason, "dpp_eram_entry_insert ipv6 encap1 sip table failed", sizeof(f_rsp->error.reason)-1);
                f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
                return -EINVAL;
            }
        }
    }

    err = dpp_fd_acl_entry_add(pf_info, handle, key, key_mask, result);
    if (err)
    {
        LOG_ERR_DEV(dh_dev, "failed to call dpp_fd_acl_entry_add()\n");
        zte_strncpy_s(f_rsp->error.reason, "failed to call dpp_fd_acl_entry_add()", sizeof(f_rsp->error.reason)-1);
        f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
        return -EINVAL;
    }

    // 清空统计空间
    err = dpp_stat_fd_stat_cnt_get(pf_info, handle, RD_CLR_MODE_CLR, &f_rsp->count.bytes, &f_rsp->count.hits);
    if (err)
    {
        LOG_ERR_DEV(dh_dev, "failed to clear fd cnt!!!\n");
        zte_strncpy_s(f_rsp->error.reason, "failed to clear fd cnt!!!", sizeof(f_rsp->error.reason)-1);
        f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
        return -EINVAL;
    }

    f_rsp->dh_flow.flowentry.hw_idx = handle;

    return 0;
}

static uint32_t zxdh_vf_flow_hw_del(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t err = 0;
    uint32_t handle = 0;
    zxdh_flow_op_msg *f_msg = &msg->flow_msg;
    zxdh_flow_op_rsp *f_rsp = &reps->flow_rsp;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    handle = f_msg->dh_flow.flowentry.hw_idx;

    // 删表
    err = dpp_fd_acl_entry_del(pf_info, handle);
    if (err)
    {
        LOG_ERR_DEV(dh_dev, "failed to call dpp_fd_acl_entry_del()\n");
        zte_strncpy_s(f_rsp->error.reason, "failed to call dpp_fd_acl_entry_del()", sizeof(f_rsp->error.reason)-1);
        f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
        return -EINVAL;
    }

    // 清空统计空间
    err = dpp_stat_fd_stat_cnt_get(pf_info, handle, RD_CLR_MODE_CLR, &f_rsp->count.bytes, &f_rsp->count.hits);
    if (err)
    {
        LOG_ERR_DEV(dh_dev, "failed to clear fd cnt!!!\n");
        zte_strncpy_s(f_rsp->error.reason, "failed to clear fd cnt!!!", sizeof(f_rsp->error.reason)-1);
        f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
        return -EINVAL;
    }

    // 释放handle
    err = dpp_fd_acl_index_release(pf_info, handle);
    if (err)
    {
        LOG_ERR_DEV(dh_dev, "failed to release index!!!\n");
        zte_strncpy_s(f_rsp->error.reason, "failed to release index!!!", sizeof(f_rsp->error.reason)-1);
        f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
        return -EINVAL;
    }

    f_rsp->dh_flow.flowentry.fd_flow.result = f_msg->dh_flow.flowentry.fd_flow.result;

    return 0;
}

static uint32_t zxdh_vf_flow_hw_get(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t err = 0;
    uint32_t handle = 0;
    uint8_t *key = NULL;
    uint8_t *key_mask = NULL;
    uint8_t *result = NULL;
    zxdh_flow_op_msg *f_msg = &msg->flow_msg;
    zxdh_flow_op_rsp *f_rsp = &reps->flow_rsp;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    handle = f_msg->dh_flow.flowentry.hw_idx;

    // 查询
    key = (uint8_t *)&f_rsp->dh_flow.flowentry.fd_flow.key;
    key_mask = (uint8_t *) &f_rsp->dh_flow.flowentry.fd_flow.key_mask;
    result = (uint8_t *)&f_rsp->dh_flow.flowentry.fd_flow.result;

    err = dpp_fd_acl_entry_get(pf_info, handle, key, key_mask, result);
    if (err)
    {
        LOG_ERR_DEV(dh_dev, "failed to get fd rule!!!\n");
        zte_strncpy_s(f_rsp->error.reason, "failed to get fd rule!!!", sizeof(f_rsp->error.reason)-1);
        f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
        return -EINVAL;
    }

    // 统计
    f_rsp->count.bytes = 0;
    f_rsp->count.hits = 0;
    err = dpp_stat_fd_stat_cnt_get(pf_info, handle, RD_CLR_MODE_UNCLR, &f_rsp->count.bytes, &f_rsp->count.hits);
    if (err)
    {
        LOG_ERR_DEV(dh_dev, "failed to get fd cnt!!!\n");
        zte_strncpy_s(f_rsp->error.reason, "failed to get fd cnt!!!", sizeof(f_rsp->error.reason)-1);
        f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
        return -EINVAL;
    }

    return 0;
}

static uint32_t zxdh_vf_flow_hw_flush(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t err = 0;
    zxdh_flow_op_rsp *f_rsp = &reps->flow_rsp;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    // 删除所有vport表项
    err = dpp_fd_acl_all_delete(pf_info);
    if (err)
    {
        LOG_ERR_DEV(dh_dev, "failed to detele all fd!!!\n");
        zte_strncpy_s(f_rsp->error.reason, "failed to detele all fd!!!", sizeof(f_rsp->error.reason)-1);
        f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
        return -EINVAL;
    }

    // 清除所有vport统计
    err = dpp_fd_acl_stat_clear(pf_info);
    if (err)
    {
        LOG_ERR_DEV(dh_dev, "failed to clear fd stat!!!\n");
        zte_strncpy_s(f_rsp->error.reason, "failed to gclear fd stat!!!", sizeof(f_rsp->error.reason)-1);
        f_rsp->error.reason[sizeof(f_rsp->error.reason) - 1] = '\0';
        return -EINVAL;
    }

    return 0;
}

static int32_t zxdh_flow_table_vf_action_add(struct ethtool_rx_flow_spec *fs,
            ZXDH_FD_CFG_T *p_fd_cfg, DPP_PF_INFO_T *pf_info)
{
    uint8_t vf_id = 0;
    uint32_t queue_id = 0;
    uint32_t base_qid = 0;
    int32_t ret = 0;

    /* 默认打开cnt统计功能 */
    p_fd_cfg->as_rlt.action_index |= ACTION_TYPE_COUNT;

    if (fs->ring_cookie == RX_CLS_FLOW_DISC) {
        p_fd_cfg->as_rlt.action_index |= ACTION_TYPE_DROP;
        return 0;
    }

    vf_id = ethtool_get_flow_spec_ring_vf(fs->ring_cookie);
    queue_id = ethtool_get_flow_spec_ring(fs->ring_cookie);

    if (queue_id == QUEUE_RSS) {
        p_fd_cfg->as_rlt.action_index |= ACTION_TYPE_RSS;
        return 0;
    }

    ret = dpp_vport_base_qid_get(pf_info, &base_qid);
    if (ret) {
        LOG_ERR("zxdh_cfg_fd_add: get vf base qid failed");
        return ret;
    }

    p_fd_cfg->as_rlt.action_index |= ACTION_TYPE_QUEUE;
    p_fd_cfg->as_rlt.v_qid = queue_id * 2 + base_qid;
    LOG_INFO("zxdh_cfg_vf_fd_add, phy queue id is %u", p_fd_cfg->as_rlt.v_qid);
    return 0;
}

void zxdh_flow_table_add(struct ethtool_rx_flow_spec *fs, ZXDH_FD_CFG_T *p_fd_cfg,
                            DPP_PF_INFO_T *pf_info)
{
    /* 设置默认掩码为不关心 */
    zte_memset_s(&p_fd_cfg->mask, 0xff, sizeof(ZXDH_FD_CFG_MASK));

    p_fd_cfg->key.vqm_vfid = VQM_VFID(pf_info->vport);
    p_fd_cfg->mask.vqm_vfid = ETHTOOL_TRUE_MASK;

    /* 根据不同流类型，设置掩码和键值 */
    switch (fs->flow_type & ~(FLOW_EXT | FLOW_MAC_EXT))
    {
        case ETHER_FLOW:
            if (!is_zero_ether_addr(fs->m_u.ether_spec.h_dest)) {
                zte_memcpy_s(p_fd_cfg->key.dmac, fs->h_u.ether_spec.h_dest, ETH_ALEN);
                zte_memset_s(p_fd_cfg->mask.dmac, ETHTOOL_TRUE_MASK, ETH_ALEN);
                LOG_INFO("dmac is %pM\n", p_fd_cfg->key.dmac);
            }
            if (!is_zero_ether_addr(fs->m_u.ether_spec.h_source)) {
                zte_memcpy_s(p_fd_cfg->key.smac, fs->h_u.ether_spec.h_source, ETH_ALEN);
                zte_memset_s(p_fd_cfg->mask.smac, ETHTOOL_TRUE_MASK, ETH_ALEN);
                LOG_INFO("smac is %pM\n", p_fd_cfg->key.smac);
            }
            if (fs->m_u.ether_spec.h_proto) {
                p_fd_cfg->key.ethtype = ntohs(fs->h_u.ether_spec.h_proto);
                p_fd_cfg->mask.ethtype = ETHTOOL_TRUE_MASK;
                LOG_INFO("ethertype is 0x%x\n", p_fd_cfg->key.ethtype);
            }
            break;
        case IPV4_USER_FLOW:
            if (fs->m_u.usr_ip4_spec.ip4src) {
                zte_memcpy_s((uint8_t *)p_fd_cfg->key.sip + 12, &fs->h_u.usr_ip4_spec.ip4src, ETHTOOL_IP4_LEN);
                zte_memset_s((uint8_t *)p_fd_cfg->mask.sip + 12, ETHTOOL_TRUE_MASK, ETHTOOL_IP4_LEN);
                LOG_INFO("sip: %d.%d.%d.%d\n", p_fd_cfg->key.sip[12], p_fd_cfg->key.sip[13], \
                    p_fd_cfg->key.sip[14], p_fd_cfg->key.sip[15]);
            }
            if (fs->m_u.usr_ip4_spec.ip4dst) {
                zte_memcpy_s((uint8_t *)p_fd_cfg->key.dip + 12 , &fs->h_u.usr_ip4_spec.ip4dst, ETHTOOL_IP4_LEN);
                zte_memset_s((uint8_t *)p_fd_cfg->mask.dip + 12, ETHTOOL_TRUE_MASK, ETHTOOL_IP4_LEN);
                LOG_INFO("dip: %d.%d.%d.%d\n", p_fd_cfg->key.dip[12], p_fd_cfg->key.dip[13], \
                    p_fd_cfg->key.dip[14], p_fd_cfg->key.dip[15]);
            }
            if (fs->m_u.usr_ip4_spec.proto) {
                p_fd_cfg->key.proto = fs->h_u.usr_ip4_spec.proto;
                p_fd_cfg->mask.proto = ETHTOOL_TRUE_MASK;
                LOG_INFO("proto: %d\n", p_fd_cfg->key.proto);
            }
            p_fd_cfg->key.ethtype = ETH_PKT_IPV4;
            p_fd_cfg->mask.ethtype = ETHTOOL_TRUE_MASK;
            LOG_INFO("ethertype is 0x%x\n", p_fd_cfg->key.ethtype);
            break;
        case TCP_V4_FLOW:
            if (fs->m_u.tcp_ip4_spec.ip4src) {
                zte_memcpy_s((uint8_t *)p_fd_cfg->key.sip + 12, &fs->h_u.tcp_ip4_spec.ip4src, ETHTOOL_IP4_LEN);
                zte_memset_s((uint8_t *)p_fd_cfg->mask.sip + 12, ETHTOOL_TRUE_MASK, ETHTOOL_IP4_LEN);
                LOG_INFO("sip: %d.%d.%d.%d\n", p_fd_cfg->key.sip[12], p_fd_cfg->key.sip[13], \
                    p_fd_cfg->key.sip[14], p_fd_cfg->key.sip[15]);
            }
            if (fs->m_u.tcp_ip4_spec.ip4dst) {
                zte_memcpy_s((uint8_t *)p_fd_cfg->key.dip + 12, &fs->h_u.tcp_ip4_spec.ip4dst, ETHTOOL_IP4_LEN);
                zte_memset_s((uint8_t *)p_fd_cfg->mask.dip + 12, ETHTOOL_TRUE_MASK, ETHTOOL_IP4_LEN);
                LOG_INFO("dip: %d.%d.%d.%d\n", p_fd_cfg->key.dip[12], p_fd_cfg->key.dip[13], \
                    p_fd_cfg->key.dip[14], p_fd_cfg->key.dip[15]);
            }
            if (fs->m_u.tcp_ip4_spec.psrc) {
                p_fd_cfg->key.sport = ntohs(fs->h_u.tcp_ip4_spec.psrc);
                p_fd_cfg->mask.sport = ETHTOOL_TRUE_MASK;
                LOG_INFO("sport is %d\n", p_fd_cfg->key.sport);
            }
            if (fs->m_u.tcp_ip4_spec.pdst) {
                p_fd_cfg->key.dport = ntohs(fs->h_u.tcp_ip4_spec.pdst);
                p_fd_cfg->mask.dport = ETHTOOL_TRUE_MASK;
                LOG_INFO("dport is %d\n", p_fd_cfg->key.dport);
            }
            p_fd_cfg->key.ethtype = ETH_PKT_IPV4;
            p_fd_cfg->mask.ethtype = ETHTOOL_TRUE_MASK;
            LOG_INFO("ethertype is 0x%x\n", p_fd_cfg->key.ethtype);
            p_fd_cfg->key.proto = IPPROTO_TCP;
            p_fd_cfg->mask.proto = ETHTOOL_TRUE_MASK;
            LOG_INFO("proto is %d\n", p_fd_cfg->key.proto);
            break;
        case UDP_V4_FLOW:
            if (fs->m_u.udp_ip4_spec.ip4src) {
                zte_memcpy_s((uint8_t *)p_fd_cfg->key.sip + 12, &fs->h_u.udp_ip4_spec.ip4src, ETHTOOL_IP4_LEN);
                zte_memset_s((uint8_t *)p_fd_cfg->mask.sip + 12, ETHTOOL_TRUE_MASK, ETHTOOL_IP4_LEN);
                LOG_INFO("sip: %d.%d.%d.%d\n", p_fd_cfg->key.sip[12], p_fd_cfg->key.sip[13], \
                    p_fd_cfg->key.sip[14], p_fd_cfg->key.sip[15]);
            }
            if (fs->m_u.udp_ip4_spec.ip4dst) {
                zte_memcpy_s((uint8_t *)p_fd_cfg->key.dip + 12, &fs->h_u.udp_ip4_spec.ip4dst, ETHTOOL_IP4_LEN);
                zte_memset_s((uint8_t *)p_fd_cfg->mask.dip + 12, ETHTOOL_TRUE_MASK, ETHTOOL_IP4_LEN);
                LOG_INFO("dip: %d.%d.%d.%d\n", p_fd_cfg->key.dip[12], p_fd_cfg->key.dip[13], \
                    p_fd_cfg->key.dip[14], p_fd_cfg->key.dip[15]);
            }
            if (fs->m_u.udp_ip4_spec.psrc) {
                p_fd_cfg->key.sport = ntohs(fs->h_u.udp_ip4_spec.psrc);
                p_fd_cfg->mask.sport =  ETHTOOL_TRUE_MASK;
                LOG_INFO("sport is %d\n", p_fd_cfg->key.sport);
            }
            if (fs->m_u.udp_ip4_spec.pdst) {
                p_fd_cfg->key.dport = ntohs(fs->h_u.udp_ip4_spec.pdst);
                p_fd_cfg->mask.dport = ETHTOOL_TRUE_MASK;
                LOG_INFO("dport is %d\n", p_fd_cfg->key.dport);
            }
            p_fd_cfg->key.ethtype = ETH_PKT_IPV4;
            p_fd_cfg->mask.ethtype = ETHTOOL_TRUE_MASK;
            LOG_INFO("ethertype is 0x%x\n", p_fd_cfg->key.ethtype);
            p_fd_cfg->key.proto = IPPROTO_UDP;
            p_fd_cfg->mask.proto = ETHTOOL_TRUE_MASK;
            LOG_INFO("proto is %d\n", p_fd_cfg->key.proto);
            break;
        case IPV6_USER_FLOW:
            if (!ipv6_addr_any((struct in6_addr *)fs->m_u.usr_ip6_spec.ip6src)) {
                zte_memcpy_s(p_fd_cfg->key.sip, &fs->h_u.usr_ip6_spec.ip6src, ETHTOOL_IP6_LEN);
                zte_memset_s(p_fd_cfg->mask.sip, ETHTOOL_TRUE_MASK, ETHTOOL_IP6_LEN);
                LOG_INFO("SIP: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n", \
                    p_fd_cfg->key.sip[0], p_fd_cfg->key.sip[1], p_fd_cfg->key.sip[2], p_fd_cfg->key.sip[3], \
                    p_fd_cfg->key.sip[4], p_fd_cfg->key.sip[5], p_fd_cfg->key.sip[6], p_fd_cfg->key.sip[7], \
                    p_fd_cfg->key.sip[8], p_fd_cfg->key.sip[9], p_fd_cfg->key.sip[10], p_fd_cfg->key.sip[11], \
                    p_fd_cfg->key.sip[12], p_fd_cfg->key.sip[13], p_fd_cfg->key.sip[14], p_fd_cfg->key.sip[15]);
            }
            if (!ipv6_addr_any((struct in6_addr *)fs->m_u.usr_ip6_spec.ip6dst)) {
                zte_memcpy_s(p_fd_cfg->key.dip, &fs->h_u.usr_ip6_spec.ip6dst, ETHTOOL_IP6_LEN);
                zte_memset_s(p_fd_cfg->mask.dip, ETHTOOL_TRUE_MASK, ETHTOOL_IP6_LEN);
                LOG_INFO("DIP: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n", \
                    p_fd_cfg->key.dip[0], p_fd_cfg->key.dip[1], p_fd_cfg->key.dip[2], p_fd_cfg->key.dip[3], \
                    p_fd_cfg->key.dip[4], p_fd_cfg->key.dip[5], p_fd_cfg->key.dip[6], p_fd_cfg->key.dip[7], \
                    p_fd_cfg->key.dip[8], p_fd_cfg->key.dip[9], p_fd_cfg->key.dip[10], p_fd_cfg->key.dip[11], \
                    p_fd_cfg->key.dip[12], p_fd_cfg->key.dip[13], p_fd_cfg->key.dip[14], p_fd_cfg->key.dip[15]);
            }
            if (fs->m_u.usr_ip6_spec.l4_proto) {
                p_fd_cfg->key.proto = fs->h_u.usr_ip6_spec.l4_proto;
                p_fd_cfg->mask.proto = ETHTOOL_TRUE_MASK;
                LOG_INFO("proto: %d\n", p_fd_cfg->key.proto);
            }
            p_fd_cfg->key.ethtype = ETH_PKT_IPV6;
            p_fd_cfg->mask.ethtype = ETHTOOL_TRUE_MASK;
            LOG_INFO("ethertype is 0x%x\n", p_fd_cfg->key.ethtype);
            break;
        case TCP_V6_FLOW:
            if (!ipv6_addr_any((struct in6_addr *)fs->m_u.tcp_ip6_spec.ip6src)) {
                zte_memcpy_s(p_fd_cfg->key.sip, &fs->h_u.tcp_ip6_spec.ip6src, ETHTOOL_IP6_LEN);
                zte_memset_s(p_fd_cfg->mask.sip, ETHTOOL_TRUE_MASK, ETHTOOL_IP6_LEN);
                LOG_INFO("SIP: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n", \
                    p_fd_cfg->key.sip[0], p_fd_cfg->key.sip[1], p_fd_cfg->key.sip[2], p_fd_cfg->key.sip[3], \
                    p_fd_cfg->key.sip[4], p_fd_cfg->key.sip[5], p_fd_cfg->key.sip[6], p_fd_cfg->key.sip[7], \
                    p_fd_cfg->key.sip[8], p_fd_cfg->key.sip[9], p_fd_cfg->key.sip[10], p_fd_cfg->key.sip[11], \
                    p_fd_cfg->key.sip[12], p_fd_cfg->key.sip[13], p_fd_cfg->key.sip[14], p_fd_cfg->key.sip[15]);
            }
            if (!ipv6_addr_any((struct in6_addr *)fs->m_u.tcp_ip6_spec.ip6dst)) {
                zte_memcpy_s(p_fd_cfg->key.dip, &fs->h_u.tcp_ip6_spec.ip6dst, ETHTOOL_IP6_LEN);
                zte_memset_s(p_fd_cfg->mask.dip, ETHTOOL_TRUE_MASK, ETHTOOL_IP6_LEN);
                LOG_INFO("DIP: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n", \
                    p_fd_cfg->key.dip[0], p_fd_cfg->key.dip[1], p_fd_cfg->key.dip[2], p_fd_cfg->key.dip[3], \
                    p_fd_cfg->key.dip[4], p_fd_cfg->key.dip[5], p_fd_cfg->key.dip[6], p_fd_cfg->key.dip[7], \
                    p_fd_cfg->key.dip[8], p_fd_cfg->key.dip[9], p_fd_cfg->key.dip[10], p_fd_cfg->key.dip[11], \
                    p_fd_cfg->key.dip[12], p_fd_cfg->key.dip[13], p_fd_cfg->key.dip[14], p_fd_cfg->key.dip[15]);
            }
            if (fs->m_u.tcp_ip6_spec.psrc) {
                p_fd_cfg->key.sport = ntohs(fs->h_u.tcp_ip6_spec.psrc);
                p_fd_cfg->mask.sport =  ETHTOOL_TRUE_MASK;
                LOG_INFO("sport is %d\n", p_fd_cfg->key.sport);
            }
            if (fs->m_u.tcp_ip6_spec.pdst) {
                p_fd_cfg->key.dport = ntohs(fs->h_u.tcp_ip6_spec.pdst);
                p_fd_cfg->mask.dport = ETHTOOL_TRUE_MASK;
                LOG_INFO("dport is %d\n", p_fd_cfg->key.dport);
            }
            p_fd_cfg->key.ethtype = ETH_PKT_IPV6;
            p_fd_cfg->mask.ethtype = ETHTOOL_TRUE_MASK;
            LOG_INFO("ethertype is 0x%x\n", p_fd_cfg->key.ethtype);
            p_fd_cfg->key.proto = IPPROTO_TCP;
            p_fd_cfg->mask.proto = ETHTOOL_TRUE_MASK;
            LOG_INFO("proto is %d\n", p_fd_cfg->key.proto);
            break;
        case UDP_V6_FLOW:
            if (!ipv6_addr_any((struct in6_addr *)fs->m_u.udp_ip6_spec.ip6src)) {
                zte_memcpy_s(p_fd_cfg->key.sip, &fs->h_u.udp_ip6_spec.ip6src, ETHTOOL_IP6_LEN);
                zte_memset_s(p_fd_cfg->mask.sip, ETHTOOL_TRUE_MASK, ETHTOOL_IP6_LEN);
                LOG_INFO("SIP: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n", \
                    p_fd_cfg->key.sip[0], p_fd_cfg->key.sip[1], p_fd_cfg->key.sip[2], p_fd_cfg->key.sip[3], \
                    p_fd_cfg->key.sip[4], p_fd_cfg->key.sip[5], p_fd_cfg->key.sip[6], p_fd_cfg->key.sip[7], \
                    p_fd_cfg->key.sip[8], p_fd_cfg->key.sip[9], p_fd_cfg->key.sip[10], p_fd_cfg->key.sip[11], \
                    p_fd_cfg->key.sip[12], p_fd_cfg->key.sip[13], p_fd_cfg->key.sip[14], p_fd_cfg->key.sip[15]);
            }
            if (!ipv6_addr_any((struct in6_addr *)fs->m_u.udp_ip6_spec.ip6dst)) {
                zte_memcpy_s(p_fd_cfg->key.dip, &fs->h_u.udp_ip6_spec.ip6dst, ETHTOOL_IP6_LEN);
                zte_memset_s(p_fd_cfg->mask.dip, ETHTOOL_TRUE_MASK, ETHTOOL_IP6_LEN);
                LOG_INFO("DIP: %02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x:%02x%02x\n", \
                    p_fd_cfg->key.dip[0], p_fd_cfg->key.dip[1], p_fd_cfg->key.dip[2], p_fd_cfg->key.dip[3], \
                    p_fd_cfg->key.dip[4], p_fd_cfg->key.dip[5], p_fd_cfg->key.dip[6], p_fd_cfg->key.dip[7], \
                    p_fd_cfg->key.dip[8], p_fd_cfg->key.dip[9], p_fd_cfg->key.dip[10], p_fd_cfg->key.dip[11], \
                    p_fd_cfg->key.dip[12], p_fd_cfg->key.dip[13], p_fd_cfg->key.dip[14], p_fd_cfg->key.dip[15]);
            }
            if (fs->m_u.udp_ip6_spec.psrc) {
                p_fd_cfg->key.sport = ntohs(fs->h_u.udp_ip6_spec.psrc);
                p_fd_cfg->mask.sport =  ETHTOOL_TRUE_MASK;
                LOG_INFO("sport is %d\n", p_fd_cfg->key.dport);
            }
            if (fs->m_u.udp_ip6_spec.pdst) {
                p_fd_cfg->key.dport = ntohs(fs->h_u.udp_ip6_spec.pdst);
                p_fd_cfg->mask.dport = ETHTOOL_TRUE_MASK;
                LOG_INFO("dport is %d\n", p_fd_cfg->key.dport);
            }
            p_fd_cfg->key.ethtype = ETH_PKT_IPV6;
            p_fd_cfg->mask.ethtype = ETHTOOL_TRUE_MASK;
            LOG_INFO("ethertype is 0x%x\n", p_fd_cfg->key.ethtype);
            p_fd_cfg->key.proto = IPPROTO_UDP;
            p_fd_cfg->mask.proto = ETHTOOL_TRUE_MASK;
            LOG_INFO("proto is %d\n", p_fd_cfg->key.proto);
            break;
        default:
            break;
    }

    /* 添加Vlan扩展字段 */
    if ((fs->flow_type & FLOW_EXT))
    {
        LOG_INFO("fs->h_ext.vlan_tci is %d\n", ntohs(fs->h_ext.vlan_tci));
        if (fs->m_ext.vlan_tci) {
            p_fd_cfg->key.cvlan_pri = (ntohs(fs->h_ext.vlan_tci) & VLAN_PCP_MASK) >> VLAN_PCP_SHIFT;
            p_fd_cfg->mask.cvlan_pri = ETHTOOL_TRUE_MASK;
            p_fd_cfg->key.cvlanid = ntohs(fs->h_ext.vlan_tci) & VLAN_VID_MASK;
            p_fd_cfg->mask.cvlanid = ETHTOOL_TRUE_MASK;
            LOG_INFO("VLAN TCI: PRI=%u, VID=%u\n", p_fd_cfg->key.cvlan_pri, p_fd_cfg->key.cvlanid);
        }
    }

    /* 添加mac扩展字段 */
    if ((fs->flow_type & FLOW_MAC_EXT) && (!is_zero_ether_addr(fs->m_ext.h_dest)))
    {
        zte_memcpy_s(p_fd_cfg->key.dmac, fs->h_ext.h_dest, ETH_ALEN);
        zte_memset_s(p_fd_cfg->mask.dmac, ETHTOOL_TRUE_MASK, ETH_ALEN);
        LOG_INFO("dmac is %pM\n", p_fd_cfg->key.dmac);
    }

}
EXPORT_SYMBOL(zxdh_flow_table_add);

static uint32_t zxdh_vf_fd_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    ZXDH_FD_CFG_T p_fd_cfg = {0};
    uint32_t handle = 0;
    uint32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    /* 填写fd表 */
    zxdh_flow_table_add(&msg->vf_fd_cfg_msg.fs, &p_fd_cfg, pf_info);
    err = zxdh_flow_table_vf_action_add(&msg->vf_fd_cfg_msg.fs, &p_fd_cfg, pf_info);
    if (err != 0) {
        LOG_ERR_DEV(dh_dev, "failed to add vf_action!\n");
        return 1;
    }

    /* 获取index */
    if (msg->vf_fd_cfg_msg.index == DEFAULT_ADD_INDEX) {
        /* 申请新的index */
        err = dpp_fd_acl_index_request(pf_info, &handle);
        if (err != 0) {
            LOG_ERR_DEV(dh_dev, "failed to request index!!!\n");
            return 1;
        }
    } else {
        handle = msg->vf_fd_cfg_msg.index;   /* 使用旧的index */
    }
    reps->fd_cfg_resp.index = handle;

    /* 统计功能使用的count_id为index */
    p_fd_cfg.as_rlt.count_id = handle;

    /* 配置到np */
    err = dpp_tbl_fd_cfg_add(pf_info, ZXDH_SDT_FD_CFG_TABLE, handle, &p_fd_cfg);
    if (err != 0) {
        LOG_ERR_DEV(dh_dev, "failed to add fd in np!!!\n");
        return 1;
    }

    return 0;
}

static uint32_t zxdh_vf_fd_get(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    ZXDH_FD_CFG_T p_fd_cfg = {0};
    uint32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    err = dpp_tbl_fd_cfg_get(pf_info, ZXDH_SDT_FD_CFG_TABLE, msg->vf_fd_cfg_msg.index, &p_fd_cfg);
    if (err != 0) {
        LOG_ERR_DEV(dh_dev, "failed to get fd in np!\n");
        return 1;
    }
    return 0;
}

static uint32_t zxdh_vf_fd_del(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t index = 0;
    uint32_t err = 0;
    DPP_STAT_VALUE_U stat_value = {0};
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    index = msg->vf_fd_cfg_msg.index;
    if (index >= ETHTOOL_FD_MAX_NUM)
    {
        LOG_ERR_DEV(dh_dev, "the index is invlaid: %d\n", index);
        return 1;
    }

    /* 配置到np */
    err = dpp_tbl_fd_cfg_del(pf_info, ZXDH_SDT_FD_CFG_TABLE, index);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "failed to del fd in np!\n");
        return 1;
    }

    /* 释放index */
    err = dpp_fd_acl_index_release(pf_info, index);
    if (err)
    {
        LOG_ERR_DEV(dh_dev, "failed to release index!\n");
        return EINVAL;
    }

    err = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_FD_FLOW_STAT, index, STAT_RD_CLR_MODE_CLR, &stat_value);
    ZXDH_CHECK_RET_RETURN(err, "clear index[0x%x] failed!\n", index);

    return 0;
}

static uint32_t zxdh_vf_udp_stats_get(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    int32_t err = 0;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    err = dpp_stat_asn_phyport_rx_pkt_cnt_get(pf_info, pf_dev->phy_port, STAT_RD_CLR_MODE_UNCLR, \
                                               &reps->udp_phy_stats_msg.rx_arn_phy);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_stat_asn_phyport_rx_pkt_cnt_get failed: %d\n", err);
        return err;
    }

    err = dpp_stat_psn_phyport_tx_pkt_cnt_get(pf_info, pf_dev->phy_port, STAT_RD_CLR_MODE_UNCLR, \
                                                &reps->udp_phy_stats_msg.tx_psn_phy);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_stat_psn_phyport_tx_pkt_cnt_get failed: %d\n", err);
        return err;
    }

    err = dpp_stat_psn_phyport_rx_pkt_cnt_get(pf_info, pf_dev->phy_port, STAT_RD_CLR_MODE_UNCLR, \
                                                &reps->udp_phy_stats_msg.rx_psn_phy);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_stat_psn_phyport_rx_pkt_cnt_get failed: %d\n", err);
        return err;
    }

    err = dpp_stat_psn_ack_phyport_tx_pkt_cnt_get(pf_info, pf_dev->phy_port, STAT_RD_CLR_MODE_UNCLR, \
                                                 &reps->udp_phy_stats_msg.tx_psn_ack_phy);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_stat_psn_ack_phyport_tx_pkt_cnt_get failed: %d\n", err);
        return err;
    }

    err = dpp_stat_psn_ack_phyport_rx_pkt_cnt_get(pf_info, pf_dev->phy_port, STAT_RD_CLR_MODE_UNCLR, \
                                                    &reps->udp_phy_stats_msg.rx_psn_ack_phy);
    if (err != 0)
    {
        LOG_ERR_DEV(dh_dev, "dpp_stat_psn_ack_phyport_rx_pkt_cnt_get failed: %d\n", err);
        return err;
    }

    return 0;
}

static uint32_t zxdh_tc_vf_fd_add(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t handle = 0;
    uint32_t err = 0;
    DPP_STAT_VALUE_U stat_value = {0};
    ZXDH_FD_CFG_T *p_fd_cfg = &msg->tc_vf_fd_cfg_msg.fd_cfg;
    struct dh_core_dev *dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    LOG_DEBUG_DEV(dh_dev, "zxdh_vf_fd_add start\n");

    /* 申请新的index */
    reps->fd_cfg_resp.index = DEFAULT_ADD_INDEX;
    err = dpp_fd_acl_index_request(pf_info, &handle);
    ZXDH_CHECK_RET_RETURN(err, "failed to request index!! 0x%x\n", err);
    reps->fd_cfg_resp.index = handle;

    /* 配置到np */
    p_fd_cfg->as_rlt.count_id = handle;
    err = dpp_tbl_fd_cfg_add(pf_info, ZXDH_SDT_FD_CFG_TABLE, handle, p_fd_cfg);
    if (err != 0) {
        LOG_ERR_DEV(dh_dev, "failed to add fd in np!!!\n");    
        return err;
    }

    err = dpp_stat_item_cnt_get(pf_info, DPP_STAT_ITEM_FD_FLOW_STAT, handle, STAT_RD_CLR_MODE_CLR, &stat_value);
    ZXDH_CHECK_RET_RETURN(err, "clear index[0x%x] stat fail!!\n", handle);

    return 0;
}

static uint32_t zxdh_tc_vf_ppu_stat(zxdh_msg_info *msg, zxdh_reps_info *reps, \
    DPP_PF_INFO_T *pf_info, struct zxdh_vf_item *vf_item, struct zxdh_pf_device *pf_dev)
{
    uint32_t err = 0;
    DPP_STAT_VALUE_U stat_value = {0};
    uint32_t count_id = msg->vf_ppu_stat_msg.index;
    uint32_t stat_item = msg->vf_ppu_stat_msg.stat_item;
    uint32_t rd_clr = msg->vf_ppu_stat_msg.rd_clr;

    err = dpp_stat_item_cnt_get(pf_info, stat_item, count_id, rd_clr, &stat_value);
    ZXDH_CHECK_RET_RETURN(err, "get index[0x%x] stat fail!!\n", count_id);

    reps->vf_ppu_stat_rsp.byte_cnt = stat_value.stat_cnt_128.bytes;
    reps->vf_ppu_stat_rsp.pkt_cnt = stat_value.stat_cnt_128.pkts;

    LOG_DEBUG("stat_item=%u count_id=%u rd_clr=%u\n",stat_item,count_id,rd_clr);
    LOG_DEBUG("byte_cnt=0x%llx pkt_cnt=0x%llx\n",reps->vf_ppu_stat_rsp.byte_cnt,reps->vf_ppu_stat_rsp.pkt_cnt);

    return 0;
}

zxdh_vf_msg_proc vf_msg_proc[] =
{
    {ZXDH_VF_PORT_INIT,      "vf_port_init",         zxdh_vf_port_init},
    {ZXDH_VF_PORT_UNINIT,    "vf_port_uninit",       zxdh_vf_port_uninit},
    {ZXDH_VF_PORT_RELOAD,    "vf_port_reload",       zxdh_vf_port_reload},
    {ZXDH_MAC_ADD,           "vf_all_mac_add",       zxdh_vf_all_mac_add},
    {ZXDH_MAC_DEL,           "vf_all_mac_del",       zxdh_vf_all_mac_del},
    {ZXDH_MAC_DUMP,          "vf_all_mac_dump",      zxdh_vf_all_mac_dump},
    {ZXDH_IPV6_MAC_ADD,      "vf_ipv6_mac_add",      zxdh_vf_ipv6_mac_add},
    {ZXDH_IPV6_MAC_DEL,      "vf_ipv6_mac_del",      zxdh_vf_ipv6_mac_del},
    {ZXDH_LACP_MAC_ADD,      "vf_lacp_mac_add",      zxdh_vf_lacp_mac_add},
    {ZXDH_LACP_MAC_DEL,      "vf_lacp_mac_del",      zxdh_vf_lacp_mac_del},
    {ZXDH_MAC_GET,           "vf_mac_get",           zxdh_vf_mac_get},
    {ZXDH_RSS_EN_SET,        "vf_rss_state_set",     zxdh_vf_rss_state_set},
    {ZXDH_RXFH_SET,          "vf_rxfh_set",          zxdh_vf_rxfh_set},
    {ZXDH_RXFH_GET,          "vf_rxfh_get",          zxdh_vf_rxfh_get},
    {ZXDH_RXFH_DEL,          "vf_rxfh_del",          zxdh_vf_rxfh_del},
    {ZXDH_THASH_KEY_SET,     "vf_thash_key_set",     zxdh_vf_thash_key_set},
    {ZXDH_THASH_KEY_GET,     "vf_thash_key_get",     zxdh_vf_thash_key_get},
    {ZXDH_HASH_FUNC_SET,     "vf_hash_funcs_set",    zxdh_vf_hash_funcs_set},
    {ZXDH_RX_FLOW_HASH_SET,  "vf_rx_flow_hash_set",  zxdh_vf_rx_flow_hash_set},
    {ZXDH_RX_FLOW_HASH_GET,  "vf_rx_flow_hash_get",  zxdh_vf_rx_flow_hash_get},
    {ZXDH_PORT_ATTRS_SET,    "vf_port_attrs_set",    zxdh_vf_port_attrs_set},
    {ZXDH_PORT_ATTRS_GET,    "vf_port_attrs_get",    zxdh_vf_port_attrs_get},
    {ZXDH_PROMISC_SET,       "vf_promisc_set",       zxdh_vf_promisc_set},
    {ZXDH_VLAN_FILTER_SET,   "vf_vlan_filter_set",   zxdh_vf_vlan_filter_set},
    {ZXDH_VLAN_FILTER_ADD,   "vf_rx_vid_add",        zxdh_vf_rx_vid_add},
    {ZXDH_VLAN_FILTER_DEL,   "vf_rx_vid_del",        zxdh_vf_rx_vid_del},
    {ZXDH_GET_NP_STATS,      "vf_np_stats_get",      zxdh_vf_np_stats_get},
    {ZXDH_VF_GET_UDP_STATS,  "vf_udp_stats_get",     zxdh_vf_udp_stats_get},
    {ZXDH_VF_RATE_LIMIT_SET, "vf_rate_limit_set",    zxdh_vf_rate_limit_set},
    {ZXDH_PLCR_UNINIT,       "vf_plcr_uninit",       zxdh_vf_plcr_uninit},
    {ZXDH_MAP_PLCR_FLOWID,   "vf_map_plcr_flowid",   zxdh_vf_plcr_flowid_map},
    {ZXDH_PLCR_FLOW_INIT,    "vf_plcr_flow_init",    zxdh_vf_plcr_flow_init},
    {ZXDH_PLCR_GET_MODE,     "vf_plcr_get_mode",     zxdh_vf_plcr_get_mode},
    {ZXDH_PLCR_SET_MODE,     "vf_plcr_set_mode",     zxdh_vf_plcr_set_mode},
    {ZXDH_FLOW_HW_ADD,       "vf_flow_hw_add",       zxdh_vf_flow_hw_add},
    {ZXDH_FLOW_HW_DEL,       "vf_flow_hw_del",       zxdh_vf_flow_hw_del},
    {ZXDH_FLOW_HW_GET,       "vf_flow_hw_get",       zxdh_vf_flow_hw_get},
    {ZXDH_FLOW_HW_FLUSH,     "vf_flow_hw_flush",     zxdh_vf_flow_hw_flush},
    {ZXDH_VLAN_OFFLOAD_SET,  "vf_vlan_strip_set",    zxdh_vf_vlan_strip_set},
    {ZXDH_VXLAN_OFFLOAD_ADD, "vf_vxlan_offload_add", zxdh_vf_vxlan_offload_add},
    {ZXDH_VXLAN_OFFLOAD_DEL, "vf_vxlan_offload_del", zxdh_vf_vxlan_offload_del},
    {ZXDH_SET_TPID,          "vf_qinq_tpid_cfg",     zxdh_vf_qinq_tpid_cfg},
    {ZXDH_FD_ADD,            "vf_fd_add",            zxdh_vf_fd_add},
    {ZXDH_FD_GET,            "vf_fd_get",            zxdh_vf_fd_get},
    {ZXDH_FD_DEL,            "vf_fd_del",            zxdh_vf_fd_del},
    {ZXDH_FD_EN_SET,         "vf_fd_state_set",      zxdh_vf_fd_state_set},
    {ZXDH_VF_RX_NUM_SET,     "vf_rx_num_set",        zxdh_vf_rx_num_set},

    {ZXDH_PLCR_CAR_PROFILE_ID_ADD,      "vf_plcr_profile_id_add",       zxdh_vf_plcr_profile_id_add},
    {ZXDH_PLCR_CAR_PROFILE_ID_DELETE,   "vf_plcr_profile_id_detele",    zxdh_vf_plcr_profile_id_delete},
    {ZXDH_PLCR_CAR_PROFILE_CFG_SET,     "vf_plcr_profile_cfg_set",      zxdh_vf_plcr_profile_cfg_set},
    {ZXDH_PLCR_CAR_PROFILE_CFG_GET,     "vf_plcr_profile_cfg_get",      zxdh_vf_plcr_profile_cfg_get},
    {ZXDH_PLCR_CAR_QUEUE_CFG_SET,       "vf_plcr_queue_cfg_set",        zxdh_vf_plcr_queue_cfg_set},
    {ZXDH_PORT_METER_STAT_CLR,          "vf_plcr_port_meter_stat_clr",  zxdh_vf_plcr_port_meter_stat_clr},
    {ZXDH_PORT_METER_STAT_GET,          "vf_plcr_port_meter_stat_get",  zxdh_vf_plcr_port_meter_stat_get},
    {ZXDH_VF_1588_CALL_NP,              "vf_1588_call_np",              zxdh_vf_call_np_1588},
    {ZXDH_VF_SLOT_ID_GET,               "vf_slot_id_get",               zxdh_vf_slot_id_get},
    {ZXDH_MC_CMPAT_VERINFO,             "vf_mcode_feature_get",         zxdh_vf_mcode_feature_get},
    {ZXDH_GET_K_CMPAT_VERINFO,          "vf_k_cmpat_get",               zxdh_vf_k_cmpat_get},
    {ZXDH_VF_1588_ENABLE,               "vf_1588_enable_proc",          zxdh_vf_1588_enable_proc},
    {ZXDH_VF_RSSKEY_IPID_ENABLE,        "vf_rsskey_ipid_enable",        zxdh_vf_rsskey_ipid_proc},

    {ZXDH_VF_PPU_STAT,      "tc_vf_ppu_stat",      zxdh_tc_vf_ppu_stat},
    {ZXDH_TC_FD_ADD,        "tc_vf_fd_add",        zxdh_tc_vf_fd_add},
    
};

int32_t dh_pf_msg_recv_func(void *pay_load, uint16_t len, void *reps_buffer, uint16_t *reps_len, void *dev)
{
    zxdh_msg_info *msg = (zxdh_msg_info *)pay_load;
    zxdh_reps_info *reps = (zxdh_reps_info *)reps_buffer;
    struct zxdh_pf_device *pf_dev = (struct zxdh_pf_device *)dev;
    struct dh_core_dev *dh_dev = NULL;
    struct zxdh_vf_item *vf_item = NULL;
    uint32_t ret = 0;
    int32_t i = 0;
    int32_t num = 0;
    DPP_PF_INFO_T pf_info = {0};

    if (pf_dev == NULL)
    {
        LOG_ERR("dev is NULL\n");
        return -1;
    }
    dh_dev = container_of((void*)(pf_dev), struct dh_core_dev, priv);

    LOG_DEBUG_DEV(dh_dev, "vport: 0x%x vfitem indx %d\n", msg->hdr.vport,(msg->hdr.pcie_id & (0xff)));
    pf_info.slot = pf_dev->slot_id;
    pf_info.vport = msg->hdr.vport;
    num = sizeof(vf_msg_proc)/sizeof(zxdh_vf_msg_proc);
    vf_item = &pf_dev->vf_item[(msg->hdr.pcie_id & (0xff))];
    for (i = 0; i < num; i++)
    {
        *reps_len = sizeof(union zxdh_msg);
        if (msg->hdr.op_code < ZXDH_GET_SW_STATS) //op_code=57之前的PF VF兼容性场景考虑
        {
            *reps_len = ZXDH_REPS_MAX_SIZE_BEFORE57;
        }

        if (vf_msg_proc[i].op_code == msg->hdr.op_code)
        {
            ret = vf_msg_proc[i].msg_proc(msg, reps, &pf_info, vf_item, pf_dev);
            if (ret != 0)
            {
                if ((msg->hdr.op_code == ZXDH_MAC_ADD)||(msg->hdr.op_code == ZXDH_IPV6_MAC_ADD)||(msg->hdr.op_code == ZXDH_MAC_DUMP)||(msg->hdr.op_code == ZXDH_LACP_MAC_ADD))
                {
                    if (ret == ZXDH_REPS_BEYOND_MAC)
                    {
                        reps->vf_mac_set_msg.mac_err_flag = ZXDH_REPS_BEYOND_MAC;
                    }
                    else if (ret == ZXDH_REPS_EXIST_MAC)
                    {
                        reps->vf_mac_set_msg.mac_err_flag = ZXDH_REPS_EXIST_MAC;
                    }
                }
                reps->flag = ZXDH_REPS_FAIL;
                LOG_ERR_DEV(dh_dev, "%s failed, ret: %d\n", vf_msg_proc[i].proc_name, ret);
                return -1;
            }

            reps->flag = ZXDH_REPS_SUCC;
            return 0;
        }
    }

    reps->flag = ZXDH_INVALID_OP_CODE;
    LOG_ERR_DEV(dh_dev, "invalid op_code: [%u]\n", msg->hdr.op_code);
    return -2;
}

int32_t dh_pf_msg_recv_func_register(void)
{
    int32_t ret = 0;

    ret = zxdh_bar_chan_msg_recv_register(MODULE_VF_BAR_MSG_TO_PF, dh_pf_msg_recv_func);
    if (ret != 0)
    {
        LOG_ERR("event_id[%d] register failed: %d\n", MODULE_VF_BAR_MSG_TO_PF, ret);
        return ret;
    }

    return ret;
}

void dh_pf_msg_recv_func_unregister(void)
{
    zxdh_bar_chan_msg_recv_unregister(MODULE_VF_BAR_MSG_TO_PF);
}

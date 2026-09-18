#include <linux/dinghai/zxdh_auxiliary_bus.h>
#include <linux/dinghai/driver.h>
#include <net/devlink.h>
#include <net/udp_tunnel.h>
#include <linux/dinghai/devlink.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/netdevice.h>
#include <linux/dinghai/en_sf.h>
#include <linux/etherdevice.h>
#include <linux/dinghai/helper.h>
#include <linux/dinghai/kcompat.h>
#include <linux/if_bridge.h>
#include <net/sch_generic.h>
#include "en_aux.h"
#include "en_ethtool/ethtool.h"
#include "en_np/table/include/dpp_tbl_api.h"
#include "en_np/table/include/dpp_tbl_comm.h"
#include "en_np/table/include/dpp_tbl_tm.h"
#include "en_aux/en_aux_events.h"
#include "en_aux/en_aux_eq.h"
#include "en_aux/en_aux_cmd.h"
#include "msg_common.h"
#include "en_pf.h"
#include "en_aux/en_aux_ioctl.h"
#include <linux/dinghai/lag.h>
#include "slib.h"
#include "en_tc/en_tc.h"
#include "en_aux/en_1588_pkt_proc.h"
#include "en_aux/en_aux_cmd.h"
#include "zxdh_tools/zxdh_tools_netlink.h"
#include <linux/jhash.h>
#include "en_pf/msg_func.h"
#include "bonding/zxdh_lag.h"
#include "bonding/rdma_ops.h"
#ifdef ZXDH_DCBNL_OPEN
#include "en_aux/dcbnl/en_dcbnl.h"
#endif
#ifndef CGS_V5_693
#include <linux/umh.h>
#endif
#include "en_aux/dcbnl/en_dcbnl_api.h"

#define IS_1588_MESSAGE 0
#define IS_NOT_1588_MESSAGE 1
#define IS_LB_PKT 0
#define IS_NOT_LB_PKT 1
#define ETHER_MAC_LEN 6
#define ETHER_TYPE_LEN 2
#define IP_PROT_OFFSET 9 /* IP头中protocol字段的偏移 */
#define IPV4_HDR_LEN 20
#define UDP_HDR_LEN 8
#define ETH_LEN 42


struct work_struct work_cfg_del = {0};
uint8_t card_num = 0;
extern struct slot_id_array dh_slot[DPP_PCIE_SLOT_MAX];
const uint8_t BOND_MCAST_ADDR[ETH_ALEN] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x02};
static unsigned int mac_hash(struct zxdh_ipv6_mac_tbl *mac_tbl, const uint8_t *mac_addr);
#ifdef PTP_DRIVER_INTERFACE_EN
extern int32_t zxdh_get_ptp_clock_index(struct zxdh_en_device *en_dev, uint32_t *ptp_clock_idx);
#endif /* PTP_DRIVER_INTERFACE_EN */
uint32_t max_pairs = ZXDH_MQ_PAIRS_NUM;
/* intf dev list mutex */
static DEFINE_MUTEX(dh_intf_mutex);
static LIST_HEAD(zxdh_en_dev_list);
module_param(max_pairs, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(max_pairs, "Max queue pairs");
EXPORT_SYMBOL(max_pairs);

MODULE_LICENSE("Dual BSD/GPL");

/* Started by AICoder, pid:ae164w7d10p1b7e14a9f0a44b04881643aa1d7be */
DPP_PF_INFO_T dpp_arp_info[DPP_PCIE_SLOT_MAX] = {0};
static bool vf_broadcast = 0;

bool is_vxlan_packet(struct sk_buff *skb);

#define DPP_VF_BROADCAST_START_BIT (27)
#define DPP_VF_BROADCAST_END_BIT (27)
static void zxdh_enable_dpp_vf_broad(bool enable_flag) {
    int i = 0;
    int ret;
    int value_to_write = enable_flag ? 1 : 0;

    /* Traverse all valid slots and call dpp_pktrx_mcode_glb_cfg_write */
    for (i = 1; i < DPP_PCIE_SLOT_MAX; i++) {
        /* Check if slot is valid (not invalid sn_code) */
        if (!is_sn_invalid(dh_slot[i].sn_code) && !IS_GPU_BOARD(dh_slot[i].board_type)) {
            /* enable vf boardcast, set bit 27 for NP */
            ret = dpp_pktrx_mcode_glb_cfg_write(&dh_slot[i].pf_info,
                                   DPP_VF_BROADCAST_START_BIT, DPP_VF_BROADCAST_END_BIT, value_to_write);
            if (ret != DPP_OK) {
                LOG_ERR("Failed to call dpp_pktrx_mcode_glb_cfg_write for slot %d, vport %d, board_type:%d, ret: %d\n",
                        dh_slot[i].pf_info.slot, dh_slot[i].pf_info.vport, dh_slot[i].board_type, ret);
            } else {
                LOG_DEBUG("Successfully called dpp_pktrx_mcode_glb_cfg_write for slot %d, vport %d, set %s\n",
                         dh_slot[i].pf_info.slot, dh_slot[i].pf_info.vport, value_to_write ? "enable" : "disable");
            }
        }
    }
}

static int arp_broadmask_param_set(const char *val, const struct kernel_param *kp)
{
    bool new_val;
    int ret = 0;
    /* Parse the input value */
    ret = kstrtobool(val, &new_val);
    if (ret < 0) {
        LOG_ERR("Invalid arp_broadmask value: %s\n", val);
        return ret;
    }

    zxdh_enable_dpp_vf_broad(new_val);
    /* Update the parameter value */
    vf_broadcast = new_val;
    return ret;
}

static int arp_broadmask_param_get(char *buffer, const struct kernel_param *kp)
{
    return zte_sprintf_s(buffer, "%u\n", vf_broadcast);
}

static const struct kernel_param_ops arp_broadcast_param_ops = {
    .set = arp_broadmask_param_set,
    .get = arp_broadmask_param_get,
};

module_param_cb(vf_broadcast, &arp_broadcast_param_ops, &vf_broadcast, 0644);
MODULE_PARM_DESC(vf_broadcast, "Must Set to 1 to enable ARP broadcast mask configuration, Set to 0 to disable");
/* Ended by AICoder, pid:ae164w7d10p1b7e14a9f0a44b04881643aa1d7be */

int32_t zxdh_port_enable(struct zxdh_en_device *en_dev, bool enable)
{
    DPP_PF_INFO_T pf_info = {0};
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (en_dev->vqmb_port_ctl)
        return 0;

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        return zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_IS_UP, enable, 0);
    }
    if (en_dev->is_hwbond || en_dev->ops->is_special_bond(en_dev->parent))
    {
        dpp_uplink_phy_attr_set(&pf_info, en_dev->phy_port, UPLINK_PHY_PORT_IS_UP, enable);
    }

    /* 标卡/dpu卡link-down-on-close开关打开 */
    if (en_dev->link_down_on_close)
    {
        zxdh_spm_port_enable_cfg(en_dev, enable);
    }
    return dpp_vport_attr_set(&pf_info, SRIOV_VPORT_IS_UP, enable);
}

void cleanup_rx_queues(struct zxdh_en_device *en_dev, int32_t max_index)
{
    int32_t i;

    cancel_delayed_work_sync(&en_dev->refill);

    if (max_index > en_dev->max_vq_pairs)
        max_index = en_dev->max_vq_pairs;

    for (i = 0; i < max_index; i++) {
        virtnet_napi_tx_disable(&en_dev->sq[i].napi);
        napi_disable(&en_dev->rq[i].napi);
#ifndef CGS_V5_693
        xdp_rxq_info_unreg(&en_dev->rq[i].xdp_rxq);
#endif
    }
}

/* WARNING Do not use netif_carrier_on/off(),
   it may affect the ethtool function */
static int32_t en_open(struct net_device *netdev, bool boot)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    DPP_PF_INFO_T pf_info = {0};
    int32_t err = 0;
    int32_t i = 0;
    int32_t j = 0;

    mutex_lock(&en_priv->lock);
    if (test_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state)) {
        mutex_unlock(&en_priv->lock);
        return 0;
    }

    for (i = 0; i < en_dev->max_vq_pairs; i++) {
        j = i;
        if (i < en_dev->eth_config.num_rxq) {
            if (!try_fill_recv(&en_dev->rq[i], GFP_KERNEL)) {
                LOG_WARN_DEV(en_dev->parent, "Failed to fill receive buffers for queue %d, scheduling refill\n", i);
                schedule_delayed_work(&en_dev->refill, 0);
            }
        }

#ifndef CGS_V5_693
        err = xdp_rxq_info_reg(&en_dev->rq[i].xdp_rxq, netdev, i,
                              en_dev->rq[i].napi.napi_id);
        if (err) {
            LOG_ERR_DEV(en_dev->parent, "Failed to register XDP RX queue %d: %d\n", i, err);
            goto unlock_and_exit;
        }
#endif

        virtnet_napi_enable(en_dev->rq[i].vq, &en_dev->rq[i].napi);
        virtnet_napi_tx_enable(netdev, en_dev->sq[i].vq, &en_dev->sq[i].napi);

        j = i + 1;
#ifndef CGS_V5_693
        err = xdp_rxq_info_reg_mem_model(&en_dev->rq[i].xdp_rxq,
                                       MEM_TYPE_PAGE_SHARED, NULL);
        if (err) {
            LOG_ERR_DEV(en_dev->parent, "Failed to register XDP memory model for queue %d: %d\n", i, err);
            goto unlock_and_exit;
        }
#endif
    }

    if (boot) {
#ifdef ZXDH_CONFIG_SPECIAL_SQ_EN
        err = zxdh_flow_map_init(en_priv);
        if (err) {
            LOG_ERR_DEV(en_dev->parent, "Failed to initialize flow map: %d\n", err);
            goto unlock_and_exit;
        }
#endif
    }

    if (!en_dev->link_up && !en_dev->link_down_on_close) {
        goto out;
    }

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (!en_dev->ops->is_bond(en_dev->parent)) {
        err = zxdh_port_enable(en_dev, true);
        if (err) {
            LOG_ERR_DEV(en_dev->parent, "Failed to enable port for slot %d, vport %d: %d\n",
                    pf_info.slot, pf_info.vport, err);
            goto err_port_enable;
        }
        goto out;
    }

    if (en_dev->link_down_on_close) {
        zxdh_spm_port_enable_cfg(en_dev, 1);
    }

    err = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_IS_UP, 1);
    if (err) {
        LOG_ERR_DEV(en_dev->parent, "Failed to set bond PF port attributes for slot %d, vport %d: %d\n",
                pf_info.slot, pf_info.vport, err);
        goto err_vport_up;
    }

    err = zxdh_uplink_phy_attr_set(&pf_info, en_dev->phy_port, UPLINK_PHY_PORT_IS_UP, 1);
    if (err) {
        LOG_ERR_DEV(en_dev->parent, "Failed to set uplink PHY attributes for slot %d, vport %d, phy_port %d: %d\n",
                pf_info.slot, pf_info.vport, en_dev->phy_port, err);
        goto err_phy_port_up;
    }

out:
    set_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state);
    mutex_unlock(&en_priv->lock);
    return err;

err_phy_port_up:
    err = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_IS_UP, 0);
    if (err) {
        LOG_ERR_DEV(en_dev->parent, "Failed to set bond PF port attributes for slot %d, vport %d: %d\n",
                pf_info.slot, pf_info.vport, err);
    }
err_vport_up:
    if (en_dev->link_down_on_close)
        zxdh_spm_port_enable_cfg(en_dev, 0);
    if (!en_dev->ops->is_bond(en_dev->parent))
        zxdh_port_enable(en_dev, false);
err_port_enable:
    if (boot) {
#ifdef ZXDH_CONFIG_SPECIAL_SQ_EN
        zxdh_flow_map_cleanup(en_priv);
#endif
    }
unlock_and_exit:
    cleanup_rx_queues(en_dev, j);
    mutex_unlock(&en_priv->lock);
    return -EPERM;
}

int32_t zxdh_en_open(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    return en_open(netdev, true);
}

static int32_t en_phyport_close(struct zxdh_en_device *en_dev)
{
    DPP_PF_INFO_T pf_info = {0};
    int32_t err = 0;

    if (!en_dev->link_up)
    {
        /* link-down-on-close 开关打开 */
        if (!en_dev->link_down_on_close)
        {
            return 0;
        }
    }

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    if (!en_dev->ops->is_bond(en_dev->parent))
    {
       return zxdh_port_enable(en_dev, FALSE);
    }

    /* 智卡link-down-on-close 开关打开 */
    if (en_dev->link_down_on_close)
    {
        /* 端口关闭时无法读400g mac统计 提前刷新一次 */
        zxdh_mac_stats_get(en_dev);
        zxdh_spm_port_enable_cfg(en_dev, 0);
    }

    /* 给bond-pf的端口属性表配置为down */
    err = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_IS_UP, 0);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set bond pf failed\n");
        return err;
    }

    return zxdh_uplink_phy_attr_set(&pf_info, en_dev->phy_port, UPLINK_PHY_PORT_IS_UP, 0);
}

void zxdh_napi_close(struct zxdh_en_priv *en_priv)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;

    mutex_lock(&en_priv->lock);
    if (!test_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state))
    {
        mutex_unlock(&en_priv->lock);
        return;
    }

    cleanup_rx_queues(en_dev, en_dev->max_vq_pairs);
    clear_bit(ZXDH_DEVICE_STATE_OPENED, &en_dev->state);
    netif_tx_stop_all_queues(en_dev->netdev);
    netif_tx_disable(en_dev->netdev);

    mutex_unlock(&en_priv->lock);
}

int32_t zxdh_en_close(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

#ifdef ZXDH_CONFIG_SPECIAL_SQ_EN
    zxdh_flow_map_cleanup(en_priv);
#endif
    zxdh_napi_close(en_priv);
    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    return en_phyport_close(en_dev);
}

void pkt_transport_protocol_parse(int8_t next_protocol, struct zxdh_net_hdr_tx *hdr)
{
    if (next_protocol == IPPROTO_UDP)
    {
        hdr->pipd_hdr.pi_hdr.pt.type_ctx.pkt_code = PCODE_UDP;
    }
    else if (next_protocol == IPPROTO_TCP)
    {
        hdr->pipd_hdr.pi_hdr.pt.type_ctx.pkt_code = PCODE_TCP;
    }
    else
    {
        hdr->pipd_hdr.pi_hdr.pt.type_ctx.pkt_code = PCODE_IP;
    }

    return;
}

void pkt_protocol_parse(struct sk_buff *skb, struct zxdh_net_hdr_tx *hdr)
{
    struct ethhdr *inner_eth = NULL;
    struct iphdr *ipv4h = NULL;
    struct ipv6hdr *ipv6h = NULL;
    struct vlan_hdr *vlanhdr = NULL;
    struct vlan_hdr vh = {0};
    uint16_t inner_l3_pro = 0;
    unsigned int offset = 0;

    if (skb->inner_protocol == htons(ETH_P_TEB))
    {
        inner_eth = (struct ethhdr *)skb_inner_mac_header(skb);

        if(eth_type_vlan(inner_eth->h_proto))   /* 内层vlan判断 */
        {
            offset  = skb_inner_network_offset(skb);
            vlanhdr = skb_header_pointer(skb, offset - sizeof(vh), sizeof(vh), &vh);
            if (unlikely(vlanhdr == NULL))
            {
                LOG_ERR("zxdh_en_send error: skb_header_pointer failed\n");
                return;
            }
            inner_l3_pro = vlanhdr->h_vlan_encapsulated_proto;
        }
        else
        {
            inner_l3_pro = inner_eth->h_proto;
        }
    }
    else
    {
        /* IPIP tunnel: Determine protocol type directly from inner IP header */
        ipv4h = (struct iphdr *)skb_inner_network_header(skb);
        if (ipv4h->version == 4)
        {
            inner_l3_pro = htons(ETH_P_IP);
        }
        else if (ipv4h->version == 6)
        {
            inner_l3_pro = htons(ETH_P_IPV6);
        }
        else
        {
            inner_l3_pro = skb->inner_protocol;
        }
    }

    if (inner_l3_pro == htons(ETH_P_IP))
    {
        ipv4h = (struct iphdr *)skb_inner_network_header(skb);
        hdr->pipd_hdr.pi_hdr.pt.type_ctx.ip_type = IPV4_TYPE;
        pkt_transport_protocol_parse(ipv4h->protocol, hdr);
    }
    else if (inner_l3_pro == htons(ETH_P_IPV6))
    {
        ipv6h = (struct ipv6hdr *)skb_inner_network_header(skb);
        hdr->pipd_hdr.pi_hdr.pt.type_ctx.ip_type = IPV6_TYPE;
        pkt_transport_protocol_parse(ipv6h->nexthdr, hdr);
    }
    else
    {
        hdr->pipd_hdr.pi_hdr.pt.type_ctx.ip_type = NOT_IP_TYPE;
        hdr->pipd_hdr.pi_hdr.pt.type_ctx.pkt_code = PCODE_NO_IP;
    }
}

int32_t vxlan_tso_cksum(struct sk_buff *skb)
{
    union
    {
        struct iphdr *v4;
        struct ipv6hdr *v6;
        unsigned char *hdr;
    } out_ip, in_ip;
    union
    {
        struct tcphdr *tcp;
        struct udphdr *udp;
        unsigned char *hdr;
    } out_l4;
    unsigned char *out_trans_start = 0;
    unsigned short out_udp_len_temp = 0;
    struct vlan_hdr *vlanhdr = NULL;
    struct vlan_hdr vh = {0};
    uint16_t l3_protocol = skb->protocol;

    /* Only VXLAN packets are processed here. GRE/IP-in-IP have no outer UDP
     * header; other UDP encapsulations (e.g. GUE on port 6080) use a
     * different inner-header layout. Writing the UDP checksum field for any
     * of those would corrupt the packet. Geneve is excluded because its
     * UDP/inner checksum is handled by hardware offload; software does not
     * need to compute it. */
    if (!is_vxlan_packet(skb))
    {
        return -1;
    }
    if ((skb->protocol == htons(ETH_P_8021Q)) || (skb->protocol == htons(ETH_P_8021AD)))
    {
        vlanhdr = skb_header_pointer(skb, ETH_HLEN, sizeof(vh), &vh);
        if (vlanhdr == NULL)
        {
            return -1;
        }
        l3_protocol = vlanhdr->h_vlan_encapsulated_proto;
    }

    out_ip.hdr  = skb_network_header(skb);
    out_l4.hdr  = skb_transport_header(skb);
    if(l3_protocol == htons(ETH_P_IP))
    {
        out_trans_start  = (unsigned char *)&out_ip.v4->saddr;
    }
    else if(l3_protocol == htons(ETH_P_IPV6))
    {
        out_trans_start  = (unsigned char *)&out_ip.v6->saddr;
    }
    else
    {
        return -1;
    }

    out_l4.udp->check = 0;
    out_udp_len_temp  = out_l4.udp->len;
    out_l4.udp->len   = 0;
    in_ip.hdr = skb_inner_network_header(skb);
    out_l4.udp->check = csum_fold(csum_partial(out_trans_start, in_ip.hdr - out_trans_start, 0));
    out_l4.udp->len = out_udp_len_temp;

    return 0;
}

/* 判断是否为Geneve报文 */
bool is_geneve_packet(struct sk_buff *skb)
{

    const struct udphdr *udph;
    const struct genevehdr *geneveh;

    // 检查 UDP 头部是否完整
    if (unlikely(!pskb_may_pull(skb, sizeof(struct udphdr))))
    {
        return false;
    }

    udph = udp_hdr(skb);
    if (udph->dest!= htons(GENEVE_UDP_PORT))
    {
        return false;
    }

    // 检查Geneve头是否完整
    if (unlikely(!pskb_may_pull(skb,  sizeof(struct udphdr) + sizeof(struct genevehdr))))
    {
        return false;
    }

    geneveh = (const struct genevehdr *)(udph + 1);
    if ((geneveh->ver) != 0) /*验证Geneve版本字段（保留前两位）*/
        return false;

    return true;
}

/* Plain IP-in-IP (without UDP/GRE): ipip / sit / ip6tnl, etc.
 * Outer IPv4: protocol is 4(IPIP) or 41(IPv6-in-IPv4, SIT);
 * Outer IPv6: after extension header chain, nexthdr is 4(IPv4-in-IPv6) or 41(IPv6-in-IPv6). */
static bool is_ip_in_ip_encap_tunnel(struct sk_buff *skb)
{
    unsigned int offset = 0;
    u8 nxt;
    __be16 ethproto = vlan_get_protocol(skb);

    if (ethproto == htons(ETH_P_IP))
    {
        if (unlikely(!pskb_may_pull(skb, sizeof(struct iphdr))))
            return false;
        nxt = ip_hdr(skb)->protocol;
        return (nxt == IPPROTO_IPIP || nxt == IPPROTO_IPV6);
    }

    if (ethproto == htons(ETH_P_IPV6))
    {
        if (unlikely(!pskb_may_pull(skb, sizeof(struct ipv6hdr))))
            return false;
        nxt = ipv6_find_hdr(skb, &offset, -1, NULL, NULL);
        return (nxt == IPPROTO_IPIP || nxt == IPPROTO_IPV6);
    }

    return false;
}

/* 判断是否为GRE报文 */
static bool is_gre_packet(struct sk_buff *skb)
{
    unsigned int offset = 0;
    u8 proto;

    switch (vlan_get_protocol(skb))
    {
        case htons(ETH_P_IP):
            if (unlikely(!pskb_may_pull(skb, sizeof(struct iphdr))))
            {
                return false;
            }
            proto = ip_hdr(skb)->protocol;
            break;
        case htons(ETH_P_IPV6):
            if (unlikely(!pskb_may_pull(skb, sizeof(struct ipv6hdr))))
            {
                return false;
            }
            proto = ipv6_find_hdr(skb, &offset, -1, NULL, NULL);
            break;
        default:
            return false;
    }

    return (proto == IPPROTO_GRE);
}

/* 判断是否为VXLAN报文 */
bool is_vxlan_packet(struct sk_buff *skb)
{
    const struct udphdr *udph;
    __be16 ethproto;
    /* IANA-assigned VXLAN UDP port (RFC 7348). The same value in
     * include/linux/dinghai/kcompat.h is only defined for kernel 4.19.90/91,
     * so we define a private copy here to stay portable. */
#ifndef IANA_VXLAN_UDP_PORT_LOCAL
#define IANA_VXLAN_UDP_PORT_LOCAL 4789
#endif

    ethproto = vlan_get_protocol(skb);

    /* VXLAN 只通过 IPv4/IPv6 承载 */
    if (ethproto != htons(ETH_P_IP) && ethproto != htons(ETH_P_IPV6))
        return false;

    /* 检查 UDP 头部是否完整 */
    if (unlikely(!pskb_may_pull(skb, sizeof(struct udphdr))))
        return false;

    udph = udp_hdr(skb);

    /* 直接判断目标端口是否为 VXLAN 端口 */
    return (udph->dest == htons(IANA_VXLAN_UDP_PORT_LOCAL));
}

int32_t zxdh_tx_checksum_offload(struct zxdh_en_device *en_dev, struct sk_buff *skb, struct zxdh_net_hdr_tx *hdr)
{
    struct zxdh_pf_device *pf_dev = dh_core_priv(en_dev->parent->parent);
    struct firmware_capability *fwcap = &pf_dev->fwcap;
    if (skb->ip_summed != CHECKSUM_PARTIAL)
    {
        return 0;
    }

    /* Tunnel packets support checksum offload for VXLAN, Geneve, GRE, IP-in-IP (ipip/sit/ip6tnl) */
    if(skb->encapsulation == 1)
    {
        hdr->pipd_hdr.pi_hdr.bttl_pi_len = ENABLE_PI_FLAG_32B;
        hdr->pipd_hdr.pd_hdr.ol_flag |= htons(0x1 << OUTER_IP_CHECKSUM_OFFSET);
        hdr->pipd_hdr.pd_hdr.ol_flag |= htons(0x1 << NP_IS_VXLAN_FLAG);

        if(is_geneve_packet(skb))
        {
            hdr->pipd_hdr.pd_hdr.ol_flag &= ~htons(0x1 << NP_IS_VXLAN_FLAG); /* 清除vxlan标志 */
        }
        hdr->pipd_hdr.pd_hdr.ol_flag |= htons(0x1 << NP_UDP_TUNNEL_CHCKSUM_ENABLE);  /*VXLAN和Geneve隧道报文卸载共用一个标志位*/

        if (is_gre_packet(skb) || is_ip_in_ip_encap_tunnel(skb))
        {
            hdr->pipd_hdr.pd_hdr.ol_flag &= ~htons(0x1 << NP_IS_VXLAN_FLAG);
            hdr->pipd_hdr.pd_hdr.ol_flag &= ~htons(0x1 << NP_UDP_TUNNEL_CHCKSUM_ENABLE);
        }

        pkt_protocol_parse(skb, hdr);
        hdr->pipd_hdr.pi_hdr.hdr_l3_offset = htons(en_dev->hdr_len + skb_inner_network_offset(skb));
        hdr->pipd_hdr.pi_hdr.hdr_l4_offset = htons(en_dev->hdr_len + skb_inner_transport_offset(skb));

        if (FW_FEATURE_GET(fwcap->fw_feature, FW_FEATURE_VXLAN_TSO_CKSUM) == 0
            && (skb_shinfo(skb)->gso_size != 0))
        {
            vxlan_tso_cksum(skb);
        }
    }

    hdr->pipd_hdr.pi_hdr.pkt_action_flag1 |= htons(0x1 << INNER_IP_CHECKSUM_OFFSET);
    hdr->pipd_hdr.pi_hdr.pkt_action_flag2 |= 0x1 << INNER_L4_CHECKSUM_OFFSET;

    return 0;
}

static int pipd_hdr_validate_vlan(struct zxdh_en_device *en_dev, struct sk_buff *skb, struct zxdh_net_hdr_tx *hdr)
{
    if (skb && skb_vlan_tag_present(skb))
    {
        hdr->pipd_hdr.pd_hdr.ctci =  htons(skb_vlan_tag_get(skb));
        hdr->pipd_hdr.pd_hdr.ol_flag |= htons(TXCAP_CTAG_INSERT_EN_BIT);
        en_dev->hw_stats.netdev_stats.tx_added_vlan_packets++;
    }
    return 0;
}

static int pd_hdr_validate_vlan(struct zxdh_en_device *en_dev, struct sk_buff *skb, struct pd_net_hdr_tx *pd_hdr)
{
    if (skb && skb_vlan_tag_present(skb))
    {
        pd_hdr->ctci =  htons(skb_vlan_tag_get(skb));
        pd_hdr->ol_flag |= htons(TXCAP_CTAG_INSERT_EN_BIT);
        en_dev->hw_stats.netdev_stats.tx_added_vlan_packets++;
    }
    return 0;
}

int32_t pipd_net_hdr_from_skb(struct zxdh_en_device *en_dev, struct sk_buff *skb, struct zxdh_net_hdr_tx *hdr, bool is_lb)
{
    uint32_t gso_type = 0;
    uint16_t mss = 0;

    hdr->tx_port = TX_PORT_DTP;
    hdr->pipd_hdr.pi_hdr.bttl_pi_len = DISABLE_PI_FIELD_PARSE + ENABLE_PI_FLAG_32B;
    hdr->pipd_hdr.pi_hdr.pt.type_ctx.pkt_src = PKT_SRC_CPU;
    hdr->pipd_hdr.pi_hdr.eth_port_id = INVALID_ETH_PORT_ID;

    pipd_hdr_validate_vlan(en_dev, skb, hdr);

    mss = skb_shinfo(skb)->gso_size;
    gso_type = skb_shinfo(skb)->gso_type;

    if(gso_type & SKB_GSO_TCPV4)
    {
        mss = (mss > 0) ? min(skb_shinfo(skb)->gso_size, (uint16_t)(en_dev->netdev->mtu - IP_BASE_HLEN - TCP_BASE_HLEN))
                        : (uint16_t)(en_dev->netdev->mtu - IP_BASE_HLEN - TCP_BASE_HLEN);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag1 |= htons((mss / ETH_MTU_4B_UNIT) + NOT_IP_FRG_CSUM_FLAG);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag2 |= TCP_FRG_CSUM_FLAG; /*0x24  bit21,18: 带pi,tso,计算checksum */
    }
    else if(gso_type & SKB_GSO_TCPV6)
    {
        mss = (mss > 0) ? min(skb_shinfo(skb)->gso_size, (uint16_t)(en_dev->netdev->mtu - IPV6_BASE_HLEN - TCP_BASE_HLEN))
                        : (uint16_t)(en_dev->netdev->mtu - IPV6_BASE_HLEN - TCP_BASE_HLEN);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag1 |= htons((mss / ETH_MTU_4B_UNIT) + NOT_IP_FRG_CSUM_FLAG);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag2 |= TCP_FRG_CSUM_FLAG; /*0x24  bit21,18: 带pi,tso,计算checksum */
    }
#ifndef CGS_V5_693
    else if(gso_type & (SKB_GSO_UDP | SKB_GSO_UDP_L4 | SKB_GSO_UDP_TUNNEL | SKB_GSO_UDP_TUNNEL_CSUM))
#else
    else if(gso_type & (SKB_GSO_UDP | SKB_GSO_UDP_TUNNEL | SKB_GSO_UDP_TUNNEL_CSUM))
#endif
    {
        hdr->pipd_hdr.pi_hdr.pkt_action_flag1 = htons((uint16_t)(en_dev->netdev->mtu / ETH_MTU_4B_UNIT) + IP_FRG_CSUM_FLAG);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag2 |= NOT_TCP_FRG_CSUM_FLAG;
    }
#ifndef CGS_V5_693
    else if (gso_type & SKB_GSO_IPXIP4)
    {
        mss = (mss > 0) ? mss : (uint16_t)(en_dev->netdev->mtu - IP_BASE_HLEN - TCP_BASE_HLEN);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag1 |= htons((mss / ETH_MTU_4B_UNIT) + NOT_IP_FRG_CSUM_FLAG);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag2 |= TCP_FRG_CSUM_FLAG; /* 带 pi,tso,计算 checksum */
    }
    else if (gso_type & SKB_GSO_IPXIP6)
    {
        mss = (mss > 0) ? mss : (uint16_t)(en_dev->netdev->mtu - IPV6_BASE_HLEN - TCP_BASE_HLEN);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag1 |= htons((mss / ETH_MTU_4B_UNIT) + NOT_IP_FRG_CSUM_FLAG);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag2 |= TCP_FRG_CSUM_FLAG; /* 带 pi,tso,计算 checksum */
    }
#else
    else if (gso_type & SKB_GSO_IPIP)
    {
        mss = (mss > 0) ? mss : (uint16_t)(en_dev->netdev->mtu - IP_BASE_HLEN - TCP_BASE_HLEN);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag1 |= htons((mss / ETH_MTU_4B_UNIT) + NOT_IP_FRG_CSUM_FLAG);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag2 |= TCP_FRG_CSUM_FLAG; /* 带 pi,tso,计算 checksum */
    }
#endif
    else
    {
        hdr->pipd_hdr.pi_hdr.pkt_action_flag1 |= htons((en_dev->netdev->mtu / ETH_MTU_4B_UNIT) + NOT_IP_FRG_CSUM_FLAG);
        hdr->pipd_hdr.pi_hdr.pkt_action_flag2 |= NOT_TCP_FRG_CSUM_FLAG;
    }
#ifdef CONFIG_INET
    /* 将这个报文打上lb标识 */
    if (is_lb == IS_LB_PKT)
    {
        hdr->pipd_hdr.pd_hdr.ol_flag |= htons(LB_EN);
    }
#endif

    if (en_dev->netdev->features & NETIF_F_HW_CSUM)
    {
        zxdh_tx_checksum_offload(en_dev, skb, hdr);
    }

    if ((en_dev->ops->is_bond(en_dev->parent) || en_dev->ops->is_special_bond(en_dev->parent) || en_dev->is_hwbond) &&
        (skb->protocol == htons(ETH_P_SLOW) || skb->protocol == htons(ETH_P_PAUSE)))
    {
        hdr->pipd_hdr.pd_hdr.ol_flag |= htons(PANELID_EN);
        hdr->pipd_hdr.pd_hdr.panel_id = en_dev->phy_port;
    }

#ifdef ZXDH_DCBNL_OPEN
    if (NULL != skb->sk)
    {
        hdr->pipd_hdr.pd_hdr.ol_flag |= htons(ZXDH_DCBNL_SET_SK_PRIO(skb->sk->sk_priority));
    }
#endif

    return 0;
}

int32_t pd_net_hdr_from_skb(struct zxdh_en_device *en_dev, struct sk_buff *skb, struct zxdh_net_hdr_tx *hdr, bool is_lb)
{
    hdr->tx_port = TX_PORT_NP;

    pd_hdr_validate_vlan(en_dev, skb, &hdr->pd_hdr);

#ifdef CONFIG_INET
    /* 将这个报文打上lb标识 */
    if (is_lb == IS_LB_PKT)
    {
        hdr->pd_hdr.ol_flag |= htons(LB_EN);
    }
#endif

    if ((en_dev->ops->is_bond(en_dev->parent) || en_dev->ops->is_special_bond(en_dev->parent) || en_dev->is_hwbond) &&
        (skb->protocol == htons(ETH_P_SLOW) || skb->protocol == htons(ETH_P_PAUSE)))
    {
        hdr->pd_hdr.ol_flag |= htons(PANELID_EN);
        hdr->pd_hdr.panel_id = en_dev->phy_port;
    }

#ifdef ZXDH_DCBNL_OPEN
    if (NULL != skb->sk)
    {
        hdr->pd_hdr.ol_flag |= htons(ZXDH_DCBNL_SET_SK_PRIO(skb->sk->sk_priority));
    }
#endif

    return 0;
}

int32_t net_hdr_from_skb(struct zxdh_en_device *en_dev, struct sk_buff *skb, struct zxdh_net_hdr_tx *hdr, bool is_lb)
{
    struct iphdr *ipv4h = NULL;
    struct ipv6hdr *ipv6h = NULL;
    struct ethhdr *eth_header = eth_hdr(skb);
    memset(hdr, 0, en_dev->hdr_len);
    hdr->pd_len = en_dev->hdr_len / HDR_2B_UNIT;

    if (!en_dev->ops->is_lowlatency(en_dev->parent))
    {
        goto default_process;
    }

    if (!eth_header)
    {
        goto default_process;
    }

    if(en_dev->delay_statistics_enable)
    {
        pkt_delay_statistics_proc(skb, &hdr->pd_hdr, en_dev);
    }

    if (skb->protocol == htons(ETH_P_IP))
    {
        ipv4h = (struct iphdr *)skb_network_header(skb);
        if (ipv4h->protocol == IPPROTO_ICMP)
        {
            pd_net_hdr_from_skb(en_dev, skb, hdr, is_lb);
            return 0;
        }
    }
    else if (skb->protocol == htons(ETH_P_IPV6))
    {
        ipv6h = (struct ipv6hdr *)skb_network_header(skb);
        if (ipv6h->nexthdr == IPPROTO_ICMPV6)
        {
            pd_net_hdr_from_skb(en_dev, skb, hdr, is_lb);
            return 0;
        }
    }

default_process:
    if (en_dev->dtp_drs_offload == true)
    {
        if(en_dev->delay_statistics_enable)
        {
            pkt_delay_statistics_proc(skb, &hdr->pipd_hdr.pd_hdr, en_dev);
        }
        pipd_net_hdr_from_skb(en_dev, skb, hdr, is_lb);
    }
    else
    {
        if(en_dev->delay_statistics_enable)
        {
            pkt_delay_statistics_proc(skb, &hdr->pd_hdr, en_dev);
        }
        pd_net_hdr_from_skb(en_dev, skb, hdr, is_lb);
    }

    return 0;
}

void zxdh_netdev_features_over_dtp(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    en_dev->dtp_drs_offload = false;

    if ((netdev->features & NETIF_F_TSO) || (netdev->features & NETIF_F_TSO6) || (netdev->features & NETIF_F_HW_CSUM) ||
        (netdev->features & NETIF_F_GSO_UDP_TUNNEL_CSUM))
    {
        en_dev->dtp_drs_offload = true;
    }
    LOG_DEBUG_DEV(en_dev->parent, "offload=%d. TSO:%lld, TSO6:%lld, HWCSUM:%lld, GSO_UDP_TUNNEL_CSUM:%lld\n",
              en_dev->dtp_drs_offload, netdev->features & NETIF_F_TSO, netdev->features & NETIF_F_TSO6,
              netdev->features & NETIF_F_HW_CSUM, netdev->features & NETIF_F_GSO_UDP_TUNNEL_CSUM);

    if (!en_dev->packed_status)
    {
        en_dev->hdr_len = sizeof(struct zxdh_split_hdr_tx);
        en_dev->hdr_len_rx_split = sizeof(struct zxdh_split_hdr_rx);
        return;
    }

    en_dev->hdr_len = sizeof(struct zxdh_net_hdr_tx);
    if (en_dev->dtp_drs_offload == false)
    {
        en_dev->hdr_len = sizeof(struct zxdh_net_hdr_tx) - sizeof(struct pi_hdr);
    }

    return;
}

int32_t is_udp_loopback_pkt(uint8_t *pData)
{
    uint16_t eth_type_lay3 = ntohs(*((uint16_t*)(pData + (2 * ETHER_MAC_LEN))));   /* get Eth Type */
    uint8_t  eth_type_lay4 = 0;
    struct zxdh_ehdr *zxdhh = NULL;

    if (eth_type_lay3 != ETH_P_IP)
    {
        return 1;
    }

    eth_type_lay4 = *(pData + (2 * ETHER_MAC_LEN) + ETHER_TYPE_LEN + IP_PROT_OFFSET);
    if (eth_type_lay4 !=  IPPROTO_UDP)
    {
        return 1;
    }

    zxdhh = (struct zxdh_ehdr *)(pData + (2 * ETHER_MAC_LEN) + ETHER_TYPE_LEN + IPV4_HDR_LEN + UDP_HDR_LEN);
    if (zxdhh->magic != cpu_to_be64(ZXDH_TEST_MAGIC))
    {
        return 1;
    }
    LOG_DEBUG("it is udp lb pkt\n");
    return 0;
}

int32_t pd_net_hdr_from_skb_split(struct zxdh_en_device *en_dev, struct sk_buff *skb, struct zxdh_split_hdr_tx *hdr, bool is_lb)
{
    pd_hdr_validate_vlan(en_dev, skb, &hdr->pd_hdr);

#ifdef CONFIG_INET
    /* 将这个报文打上lb标识 */
    if (is_lb == IS_LB_PKT)
    {
        hdr->pd_hdr.ol_flag |= htons(LB_EN);
    }
#endif

    if ((en_dev->ops->is_bond(en_dev->parent) || en_dev->ops->is_special_bond(en_dev->parent) || en_dev->is_hwbond) &&
        (skb->protocol == htons(ETH_P_SLOW) || skb->protocol == htons(ETH_P_PAUSE)))
    {
        hdr->pd_hdr.ol_flag |= htons(PANELID_EN);
        hdr->pd_hdr.panel_id = en_dev->phy_port;
    }

#ifdef ZXDH_DCBNL_OPEN
    if (NULL != skb->sk)
    {
        hdr->pd_hdr.ol_flag |= htons(ZXDH_DCBNL_SET_SK_PRIO(skb->sk->sk_priority));
    }
#endif

    return 0;
}

int32_t net_hdr_from_skb_split(struct zxdh_en_device *en_dev, struct sk_buff *skb, struct zxdh_split_hdr_tx *hdr, bool little_endian, bool is_lb)
{
    struct skb_shared_info *sinfo = skb_shinfo(skb);

    zte_memset_s(hdr, 0, sizeof(*hdr));

    if (skb_is_gso(skb))
    {
        /* This is a hint as to how much should be linear. */
        hdr->split_hdr.hdr_len = __cpu_to_vqm16(little_endian, skb_headlen(skb));
        hdr->split_hdr.gso_size = __cpu_to_vqm16(little_endian, sinfo->gso_size);
        if (sinfo->gso_type & SKB_GSO_TCPV4)
            hdr->split_hdr.gso_type = ZXDH_NET_HDR_GSO_TCPV4;
        else if (sinfo->gso_type & SKB_GSO_TCPV6)
            hdr->split_hdr.gso_type = ZXDH_NET_HDR_GSO_TCPV6;
        else
            return -EINVAL;
        if (sinfo->gso_type & SKB_GSO_TCP_ECN)
            hdr->split_hdr.gso_type |= ZXDH_NET_HDR_GSO_ECN;
    }
    else
        hdr->split_hdr.gso_type = ZXDH_NET_HDR_GSO_NONE;

    if (skb->ip_summed == CHECKSUM_PARTIAL)
    {
        hdr->split_hdr.flags = ZXDH_NET_HDR_F_NEEDS_CSUM;
        hdr->split_hdr.csum_start = __cpu_to_vqm16(little_endian, skb_checksum_start_offset(skb));
        hdr->split_hdr.csum_offset = __cpu_to_vqm16(little_endian, skb->csum_offset);
    } /* else everything is zero */

    if(en_dev->delay_statistics_enable)
    {
        pkt_delay_statistics_proc(skb, &hdr->pd_hdr, en_dev);
    }

    pd_net_hdr_from_skb_split(en_dev, skb, hdr, is_lb);

    return 0;
}

int32_t xmit_skb_split(struct net_device *netdev, struct send_queue *sq, struct sk_buff *skb)
{
    struct zxdh_split_hdr_tx *hdr = NULL;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t num_sg = 0;
    uint hdr_len = en_dev->hdr_len;
    bool can_push = false;
    uint8_t *pData = NULL;
    int32_t is_lb = IS_NOT_LB_PKT;

    CHECK_EQUAL_ERR(skb, NULL, -EADDRNOTAVAIL, "skb is null\n");
    pData = skb->data;
    CHECK_EQUAL_ERR(pData, NULL, -EADDRNOTAVAIL, "skb->data is null\n");

#ifdef CONFIG_INET
    /* Determine if it is a udp lb test message*/
    if (en_dev->local_lb_enable == true)
    {
        if (is_udp_loopback_pkt(pData) == IS_LB_PKT)
        {
            is_lb = IS_LB_PKT;
        }
    }
#endif

    can_push = en_dev->any_header_sg &&
        !((unsigned long)skb->data & (__alignof__(*hdr) - 1)) &&
        !skb_header_cloned(skb) && skb_headroom(skb) >= hdr_len;
    /* Even if we can, don't push here yet as this would skew
     * csum_start offset below. */
    if (can_push)
    {
        hdr = (struct zxdh_split_hdr_tx *)(skb->data - hdr_len);
    }
    else
    {
        hdr = skb_vnet_hdr_tx(skb);
    }

    if (net_hdr_from_skb_split(en_dev, skb, hdr, zxdh_is_little_endian(en_dev), is_lb))
    {
        return -EPROTO;
    }

    sg_init_table(sq->sg, skb_shinfo(skb)->nr_frags + (can_push ? 1 : 2));
    if (can_push)
    {
        __skb_push(skb, hdr_len);
        num_sg = skb_to_sgvec(skb, sq->sg, 0, skb->len);
        if (unlikely(num_sg < 0))
        {
            return num_sg;
        }
        /* Pull header back to avoid skew in tx bytes calculations. */
        __skb_pull(skb, hdr_len);
    }
    else
    {
        sg_set_buf(sq->sg, hdr, hdr_len);
        num_sg = skb_to_sgvec(skb, sq->sg + 1, 0, skb->len);
        if (unlikely(num_sg < 0))
        {
            return num_sg;
        }
        num_sg++;
    }

    return virtqueue_add_outbuf(sq->vq, sq->sg, num_sg, skb, GFP_ATOMIC);
}

int32_t xmit_skb(struct net_device *netdev, struct send_queue *sq, struct sk_buff *skb)
{
    struct zxdh_net_hdr_tx *hdr = NULL;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t num_sg = 0;
    uint hdr_len = en_dev->hdr_len;
    bool can_push = false;
    uint8_t *hdr_buf = sq->hdr_buf;
    uint8_t *pData = NULL;
    uint8_t *ptpHdr = NULL;
    uint8_t ts_offset = 0;
    struct zxdh_net_1588_hdr *hdr_1588 = NULL;
    struct zxdh_net_1588_nopi_hdr *hdr_1588_nopi = NULL;
    int32_t ret = 0;
    int32_t is_1588_flag = IS_NOT_1588_MESSAGE;
    int32_t is_lb = IS_NOT_LB_PKT;

    CHECK_EQUAL_ERR(skb, NULL, -EADDRNOTAVAIL, "skb is null\n");
    pData = skb->data;
    CHECK_EQUAL_ERR(pData, NULL, -EADDRNOTAVAIL, "skb->data is null\n");

    /* Determine if it is a 1588 message */
    if (en_dev->enable_1588 == true)
    {
        is_1588_flag = get_hdr_point(pData, &ts_offset, &ptpHdr);
        if (is_1588_flag == IS_1588_MESSAGE)
        {
            LOG_DEBUG_DEV(en_dev->parent, "pkt_1588_proc_xmit offload %d\n",en_dev->dtp_drs_offload);
            if(en_dev->dtp_drs_offload == true)
            {
                hdr_len = sizeof(struct zxdh_net_1588_hdr);
            }
            else
            {
                hdr_len = sizeof(struct zxdh_net_1588_nopi_hdr);
            }
        }
    }

#ifdef CONFIG_INET
    /* Determine if it is a udp lb test message*/
    if (en_dev->local_lb_enable == true)
    {
        if (is_udp_loopback_pkt(pData) == IS_LB_PKT)
        {
            is_lb = IS_LB_PKT;
        }
    }
#endif

    can_push = en_dev->any_header_sg &&
        !((unsigned long)skb->data & (__alignof__(*hdr) - 1)) &&
        !skb_header_cloned(skb) && skb_headroom(skb) >= hdr_len;
    /* Even if we can, don't push here yet as this would skew
     * csum_start offset below. */
    if (can_push)
    {
        hdr = (struct zxdh_net_hdr_tx *)(skb->data - hdr_len);
    }
    else
    {
        hdr_buf += sq->hdr_idx * HDR_BUFFER_LEN;
        memset(hdr_buf, 0, HDR_BUFFER_LEN);
        hdr = (struct zxdh_net_hdr_tx *)(hdr_buf);
        sq->hdr_idx++;
        sq->hdr_idx = sq->hdr_idx % en_dev->eth_config.tx_queue_size;
    }

    if (net_hdr_from_skb(en_dev, skb, hdr, is_lb))
    {
        return -EPROTO;
    }

    if (en_dev->enable_1588 == true)
    {
        if (is_1588_flag == IS_1588_MESSAGE)
        {
            if(en_dev->dtp_drs_offload == true)
            {
                hdr->pd_len = sizeof(struct zxdh_net_1588_hdr) / HDR_2B_UNIT;
                hdr_1588 = (struct zxdh_net_1588_hdr *)hdr;

                /* Started by AICoder, pid:ye867r46b9yf81f14cf8095a80a7ac0190011532 */
                memset(&(hdr_1588->pd_1588), 0, sizeof(struct zxdh_1588_pd_tx));
                /* Ended by AICoder, pid:ye867r46b9yf81f14cf8095a80a7ac0190011532 */
                hdr_1588->pd_1588.ts_offset = ts_offset;
                ret = pkt_1588_proc_xmit(skb, &(hdr_1588->pd_1588), en_dev->clock_no, en_dev, ptpHdr);
                if (ret != 0)
                {
                    DEBUG_1588_DEV(en_dev->parent, "dev %s vport 0x%x pkt_1588_proc_xmit ERR, ret: %d\n", en_dev->netdev->name, en_dev->vport,ret);
                }
                DEBUG_1588_DEV(en_dev->parent, "NET HDR:");
                DEBUG_1588_DATA((uint8_t *)hdr_1588, sizeof(struct zxdh_net_1588_hdr));
            }
            else
            {
                hdr->pd_len = sizeof(struct zxdh_net_1588_nopi_hdr) / HDR_2B_UNIT;
                hdr_1588_nopi = (struct zxdh_net_1588_nopi_hdr *)hdr;
                memset(&(hdr_1588_nopi->pd_1588), 0, sizeof(struct zxdh_1588_pd_tx));
                hdr_1588_nopi->pd_1588.ts_offset = ts_offset;
                ret = pkt_1588_proc_xmit(skb, &(hdr_1588_nopi->pd_1588), en_dev->clock_no, en_dev, ptpHdr);
                if (ret != 0)
                {
                    LOG_ERR_DEV(en_dev->parent, "dev %s vport 0x%x pkt_1588_proc_nopi_xmit ERR, ret: %d\n",
                                en_dev->netdev->name, en_dev->vport, ret);
                    return ret;
                }
                DEBUG_1588_DEV(en_dev->parent, "NET HDR:");
                DEBUG_1588_DATA((uint8_t *)hdr_1588_nopi, sizeof(struct zxdh_net_1588_nopi_hdr));
            }

        }
        DEBUG_1588_DEV(en_dev->parent, "skb->data:");
        DEBUG_1588_DATA((uint8_t *)skb->data, skb->len);
    }

    sg_init_table(sq->sg, skb_shinfo(skb)->nr_frags + (can_push ? 1 : 2));
    if (can_push)
    {
        __skb_push(skb, hdr_len);
        num_sg = skb_to_sgvec(skb, sq->sg, 0, skb->len);
        if (unlikely(num_sg < 0))
        {
            return num_sg;
        }
        /* Pull header back to avoid skew in tx bytes calculations. */
        __skb_pull(skb, hdr_len);
    }
    else
    {
        sg_set_buf(sq->sg, hdr, hdr_len);
        num_sg = skb_to_sgvec(skb, sq->sg + 1, 0, skb->len);
        if (unlikely(num_sg < 0))
        {
            return num_sg;
        }
        num_sg++;
    }

    return virtqueue_add_outbuf(sq->vq, sq->sg, num_sg, skb, GFP_ATOMIC);
}

netdev_tx_t zxdh_en_xmit(struct sk_buff *skb, struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t qnum = skb_get_queue_mapping(skb);
    struct send_queue *sq = &en_dev->sq[qnum];
    int32_t err = 0;
    struct netdev_queue *txq = netdev_get_tx_queue(netdev, qnum);
    bool kick = !netdev_xmit_more();
    bool use_napi = sq->napi.weight;
    bool packed_status = en_dev->packed_status;

    /* Free up any pending old buffers before queueing new ones. */
    do {
        if (use_napi)
        {
            virtqueue_disable_cb(sq->vq);
        }

        free_old_xmit_skbs(netdev, sq, false);

    } while (use_napi && kick && unlikely(!virtqueue_enable_cb_delayed(sq->vq)));

    /* timestamp packet in software */
    skb_tx_timestamp(skb);

    if (packed_status)
    {
        /* Try to packed transmit */
        err = xmit_skb(netdev, sq, skb);
    }
    else
    {
        /* Try to split transmit */
        err = xmit_skb_split(netdev, sq, skb);
    }

    /* This should not happen! */
    if (unlikely(err))
    {
        netdev->stats.tx_fifo_errors++;
        netdev->stats.tx_errors++;
        if (net_ratelimit())
        {
            LOG_WARN_DEV(en_dev->parent, "unexpected TXQ (%d) queue failure: %d\n", qnum, err);
        }
        netdev->stats.tx_dropped++;
        en_dev->hw_stats.q_stats[qnum].q_tx_dropped++;
        dev_kfree_skb_any(skb);
        return NETDEV_TX_OK;
    }

    /* If running out of space, stop queue to avoid getting packets that we
     * are then unable to transmit.
     * An alternative would be to force queuing layer to requeue the skb by
     * returning NETDEV_TX_BUSY. However, NETDEV_TX_BUSY should not be
     * returned in a normal path of operation: it means that driver is not
     * maintaining the TX queue stop/start state properly, and causes
     * the stack to do a non-trivial amount of useless work.
     * Since most packets only take 1 or 2 ring slots, stopping the queue
     * early means 16 slots are typically wasted.
     */
    if (sq->vq->num_free < 2 + MAX_SKB_FRAGS)
    {
        netif_stop_subqueue(netdev, qnum);
        en_dev->hw_stats.q_stats[qnum].q_tx_stopped++;
        if (!use_napi && unlikely(!virtqueue_enable_cb_delayed(sq->vq)))
        {
            /* More just got used, free them then recheck. */
            free_old_xmit_skbs(netdev, sq, false);
            if (sq->vq->num_free >= 2 + MAX_SKB_FRAGS)
            {
                netif_start_subqueue(netdev, qnum);
                virtqueue_disable_cb(sq->vq);
            }
        }
    }

    if (kick || netif_xmit_stopped(txq))
    {
        if (virtqueue_kick_prepare(sq->vq) && virtqueue_notify(sq->vq))
        {
            u64_stats_update_begin(&sq->stats.syncp);
            sq->stats.kicks++;
            u64_stats_update_end(&sq->stats.syncp);
        }
    }

    return NETDEV_TX_OK;
}

#if defined(NO_U64_STATS_FETCH_BEGIN_IRQ) || defined(Rocky_9_3) || defined(ZXDH_ADAPT_REDHAT_9_6) || defined(KYLIN_V11_6_6)
static inline unsigned int u64_stats_fetch_begin_irq(const struct u64_stats_sync *syncp)
{
    return u64_stats_fetch_begin(syncp);
}

static inline bool u64_stats_fetch_retry_irq(const struct u64_stats_sync *syncp, unsigned int start)
{
    return u64_stats_fetch_retry(syncp, start);
}
#endif

#ifdef HAVE_NDO_GET_STATS64
#ifdef HAVE_VOID_NDO_GET_STATS64
static void zxdh_en_get_netdev_stats_struct(struct net_device *netdev, struct rtnl_link_stats64 *stats)
#else
static struct rtnl_link_stats64 *zxdh_en_get_netdev_stats_struct(struct net_device *netdev, struct rtnl_link_stats64 *stats)
#endif
{
    struct zxdh_en_device *en_dev = netdev_priv(netdev);
    struct receive_queue *rq = NULL;
    struct send_queue *sq = NULL;
    uint32_t start = 0;
    uint32_t i = 0;
    uint64_t tpackets = 0;
    uint64_t tbytes = 0;
    uint64_t rpackets = 0;
    uint64_t rbytes = 0;
    uint64_t rdrops = 0;
    uint32_t loop_cnt = en_dev->max_queue_pairs;
    int32_t ret = 0;
    uint64_t rx_vport_mtu_drop_bytes = 0;
    uint64_t rx_vport_mtu_drop_packets = 0;
    uint32_t vf_id = GET_VFID(en_dev->vport);
    DPP_PF_INFO_T pf_info = {0};
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (en_dev->device_state != ZXDH_DEVICE_STATE_INTERNAL_ERROR) {
        ret = zxdh_mac_stats_get(en_dev);
        if (ret != 0) {
            LOG_ERR_DEV(en_dev->parent, "zxdh_mac_stats_get failed, ret: %d\n", ret);
#ifdef HAVE_VOID_NDO_GET_STATS64
            return;
#else
            return stats;
#endif
        }
    }

    if (en_dev->ops->is_bond(en_dev->parent) || (netif_is_lag_port(netdev) && en_dev->is_special_bond))
    {
        stats->rx_packets = en_dev->hw_stats.phy_stats.rx_packets_phy;
        stats->rx_bytes   = en_dev->hw_stats.phy_stats.rx_bytes_phy;
        stats->rx_errors  = en_dev->hw_stats.phy_stats.rx_error_phy;
        stats->rx_dropped = en_dev->hw_stats.phy_stats.rx_drop_phy;
        stats->tx_packets = en_dev->hw_stats.phy_stats.tx_packets_phy;
        stats->tx_bytes   = en_dev->hw_stats.phy_stats.tx_bytes_phy;
        stats->tx_errors  = en_dev->hw_stats.phy_stats.tx_error_phy;
        stats->tx_dropped = en_dev->hw_stats.phy_stats.tx_drop_phy;
#ifdef HAVE_VOID_NDO_GET_STATS64
        return;
#else
        return stats;
#endif
    }

    for (i = 0; i < loop_cnt; ++i)
    {
        sq = &en_dev->sq[i];
        rq = &en_dev->rq[i];

        do
        {
            start = u64_stats_fetch_begin_irq(&sq->stats.syncp);
            tpackets = sq->stats.packets;
            tbytes   = sq->stats.bytes;
        } while (u64_stats_fetch_retry_irq(&sq->stats.syncp, start));

        do
        {
            start = u64_stats_fetch_begin_irq(&rq->stats.syncp);
            rpackets = rq->stats.packets;
            rbytes   = rq->stats.bytes;
            rdrops   = rq->stats.drops;
        } while (u64_stats_fetch_retry_irq(&rq->stats.syncp, start));

        stats->rx_packets += rpackets;
        stats->rx_bytes   += rbytes;
        stats->rx_dropped += rdrops;
        stats->tx_packets += tpackets;
        stats->tx_bytes   += tbytes;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        if (en_dev->device_state != ZXDH_DEVICE_STATE_INTERNAL_ERROR)
            dpp_stat_MTU_packet_msg_rx_cnt_get(&pf_info, vf_id, NP_GET_PKT_CNT, &rx_vport_mtu_drop_bytes, &rx_vport_mtu_drop_packets);
        stats->rx_errors = netdev->stats.rx_errors + en_dev->hw_stats.phy_stats.rx_error_phy + rx_vport_mtu_drop_packets - en_dev->pre_stats.np_stats.rx_vport_mtu_drop_packets;
        stats->tx_errors = netdev->stats.tx_errors + en_dev->hw_stats.phy_stats.tx_drop_phy;
    }
    else
    {
        stats->rx_errors = netdev->stats.rx_errors;
        stats->tx_errors = netdev->stats.tx_errors;
    }
    stats->tx_dropped = netdev->stats.tx_dropped;
    stats->tx_carrier_errors = netdev->stats.tx_carrier_errors;
#ifdef HAVE_VOID_NDO_GET_STATS64
    return;
#else
    return stats;
#endif
}
#endif/* HAVE_NDO_GET_STATS64 */

static void zxdh_en_set_rx_mode(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (en_dev->ops->is_bond(en_dev->parent))
        return;
    ZXDH_AUX_INIT_COMP_CHECK(en_dev);
    queue_work(en_priv->events->wq, &en_dev->rx_mode_set_work);
}

bool is_standard_predefined_ipv6_multicast_mac(const uint8_t *mac)
{
    return ((mac[0] == 0x33) && (mac[1] == 0x33) && (mac[2] == 0x00));
}

bool is_standard_ptp4_multicast_mac(const uint8_t *mac)
{
    return ((mac[0] == 0x01) && (mac[1] == 0x00) && (mac[2] == 0x5e));
}

bool zxdh_check_special_addr(const u8 *addr)
{
    if (ether_addr_equal(BOND_MCAST_ADDR, addr) || is_standard_predefined_ipv6_multicast_mac(addr) || is_standard_ptp4_multicast_mac(addr))
    {
        return true;
    }
    return false;
}

static int zxdh_addr_sync(struct net_device *netdev, const u8 *addr, mac_queue *add_queue)
{
    if (!zxdh_check_special_addr(addr))
    {
        return 0;
    }

    if (add_queue->count >= DEV_MULTICAST_MAX_NUM)
    {
        LOG_ERR("mac num is larger max size\n");
        return -1;
    }

    ether_addr_copy(add_queue->addr[add_queue->count], addr);
    add_queue->count++;

    return  0;
}

static int zxdh_addr_unsync(struct net_device *netdev, const u8 *addr, mac_queue *del_queue)
{
    if (!zxdh_check_special_addr(addr))
    {
        return 0;
    }

    if (del_queue->count >= DEV_MULTICAST_MAX_NUM)
    {
        LOG_ERR("mac num is larger max size\n");
        return -1;
    }

    ether_addr_copy(del_queue->addr[del_queue->count], addr);
    del_queue->count++;

    return  0;
}

static int __hw_addr_del_entry(struct netdev_hw_addr_list *list,
                struct netdev_hw_addr *ha, bool global,
                bool sync)
{
    if (global && !ha->global_use)
        return -ENOENT;

    if (sync && !ha->synced)
        return -ENOENT;

    if (global)
        ha->global_use = false;

    if (sync)
        ha->synced--;

    if (--ha->refcount)
        return 0;

#ifdef HAVE_RB_TREE_OPS
    /* 检查节点是否在树中，避免重复删除导致 GPF */
    if (!RB_EMPTY_NODE(&ha->node))
    {
        rb_erase(&ha->node, &list->tree);
        RB_CLEAR_NODE(&ha->node);
    }
#endif

    list_del_rcu(&ha->list);
    kfree_rcu(ha, rcu_head);
    list->count--;
    return 0;
}

static int zxdh_hw_addr_sync_dev(struct netdev_hw_addr_list *list,
            struct net_device *dev,
            int (*sync)(struct net_device *, const unsigned char *, mac_queue *),
            int (*unsync)(struct net_device *,const unsigned char *, mac_queue *),
            mac_queue *add_queue,
            mac_queue *del_queue)
{
    struct netdev_hw_addr *ha, *tmp;
    int err;

    list_for_each_entry_safe(ha, tmp, &list->list, list)
    {
        if (!ha->sync_cnt || ha->refcount != 1)
            continue;

        if (unsync && unsync(dev, ha->addr, del_queue))
            continue;

        if (zxdh_check_special_addr(ha->addr))
        {
            ha->sync_cnt--;
            __hw_addr_del_entry(list, ha, false, false);
        }
    }

    list_for_each_entry_safe(ha, tmp, &list->list, list)
    {
        if (ha->sync_cnt)
            continue;

        err = sync(dev, ha->addr, add_queue);
        if (err)
            return err;

        if (zxdh_check_special_addr(ha->addr))
        {
            ha->sync_cnt++;
            ha->refcount++;
        }
    }

    return 0;
}

static int zxdh_dev_mc_sync(struct net_device *dev,
            int (*sync)(struct net_device *, const unsigned char *, mac_queue *),
            int (*unsync)(struct net_device *, const unsigned char *, mac_queue *),
            mac_queue *add_queue,
            mac_queue *del_queue)
{
    LOG_DEBUG("%s is called", dev->name);
    return zxdh_hw_addr_sync_dev(&dev->mc, dev, sync, unsync, add_queue, del_queue);
}

static int zxdh_handle_mc_operation(struct zxdh_en_device *en_dev, DPP_PF_INFO_T *pf_info, const u8 *addr, bool is_add)
{
    int err = 0;

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        if (is_add)
        {
            err = dpp_multi_mac_add_member(pf_info, addr);
            if (err != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "dpp_multi_mac_add_member failed:0x%x\n", err);
                return err;
            }
        }
        else
        {
            err = dpp_multi_mac_del_member(pf_info, addr);
            if (err != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "dpp_multi_mac_del_member failed:0x%x\n", err);
                return err;
            }
        }
    }
    else
    {
        if (is_add)
        {
            err = zxdh_vf_dpp_add_lacp_mac(en_dev, addr);
            if (err != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "zxdh_vf_dpp_add_mac failed:0x%x\n", err);
                return err;
            }
        }
        else
        {
            err = zxdh_vf_dpp_del_lacp_mac(en_dev, addr);
            if (err != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "zxdh_vf_dpp_del_mac failed:0x%x\n", err);
                return err;
            }
        }
    }

    return 0;
}

static int zxdh_dev_mc_proc(struct zxdh_en_device *en_dev, DPP_PF_INFO_T *pf_info, mac_queue *add_queue, mac_queue *del_queue)
{
    int32_t err = 0;
    uint8_t i = 0;

    for (i = 0; i < add_queue->count; i++)
    {
        err = zxdh_handle_mc_operation(en_dev, pf_info, add_queue->addr[i], true);
        if (err != 0)
        {
            return err;
        }
        en_dev->curr_multicast_num++;
    }

    for (i = 0; i < del_queue->count; i++)
    {
        err = zxdh_handle_mc_operation(en_dev, pf_info, del_queue->addr[i], false);
        if (err != 0)
        {
            return err;
        }
        en_dev->curr_multicast_num--;
    }

    return 0;
}

/* 更新用户allmulti配置记录 */
static inline void zxdh_record_user_allmulti_cfg(struct zxdh_en_device *en_dev,
                                                 bool allmulti)
{
    en_dev->user_allmulti_cfg = allmulti ? OPENED_ALLMULTI : CLOSED_ALLMULTI;
}

/* 判断是否需要同步操作MC(multicast)：
 * - 开启promisc时，必须同时开启allmulti
 * - 关闭promisc时，如果allmulti_current为开启（即将要开），则不关闭MC，避免冗余操作
 * - 关闭promisc时，如果用户未配置或配置关闭allmulti，需要跟着关闭MC
 * - 关闭promisc时，如果用户配置开启allmulti，则保持MC开启，不操作
 */
static inline bool zxdh_promisc_need_set_mc(struct zxdh_en_device *en_dev,
                                            bool promisc, bool allmulti_current)
{
    if (promisc)
        return true;

    /* 关闭promisc时，如果allmulti_current=1，说明紧接着会开启MC，无需关闭 */
    if (allmulti_current)
        return false;

    return (en_dev->user_allmulti_cfg == DO_NOT_CONFIGED) ||
           (en_dev->user_allmulti_cfg == CLOSED_ALLMULTI);
}

/* 更新promisc相关状态：promisc_enabled和allmulti_enabled */
static inline void zxdh_promisc_update_state(struct zxdh_en_device *en_dev,
                                             bool enable_value, bool need_set_mc)
{
    mutex_lock(&en_dev->parent->lock);
    en_dev->promisc_enabled = enable_value;
    if (need_set_mc) {
        en_dev->allmulti_enabled = enable_value;
    }
    mutex_unlock(&en_dev->parent->lock);
}

static int zxdh_promisc_process_pf(struct zxdh_en_device *en_dev,
                                   DPP_PF_INFO_T *pf_info, bool promisc,
                                   bool allmulti_current)
{
    int32_t err = 0;
    bool need_set_mc = zxdh_promisc_need_set_mc(en_dev, promisc, allmulti_current);

    LOG_INFO_DEV(en_dev->parent,
                  "promisc process: promisc=%d, need_set_mc=%d\n",
                  promisc, need_set_mc);

    err = dpp_vport_uc_promisc_set(pf_info, promisc ? 1 : 0);
    if (err)
        return err;

    err = dpp_vport_promisc_en_set(pf_info, promisc ? 1 : 0);
    if (err)
        return err;

    if (need_set_mc) {
        err = dpp_vport_mc_promisc_set(pf_info, promisc ? 1 : 0);
        if (err)
            return err;
    }

    zxdh_promisc_update_state(en_dev, promisc, need_set_mc);

    return 0;
}

static int zxdh_promisc_process_vf(struct zxdh_en_device *en_dev,
                                   bool promisc, bool allmulti_current)
{
    int32_t err = 0;
    bool need_set_mc = zxdh_promisc_need_set_mc(en_dev, promisc, allmulti_current);

    LOG_INFO_DEV(en_dev->parent,
                  "promisc process: promisc=%d, need_set_mc=%d\n",
                  promisc, need_set_mc);

    err = zxdh_vf_port_promisc_set(en_dev, ZXDH_PROMISC_MODE, promisc,
                                           need_set_mc ? 1 : 0);
    if (err)
        return err;

    zxdh_promisc_update_state(en_dev, promisc, need_set_mc);

    return 0;
}

static int zxdh_allmulti_process_pf(struct zxdh_en_device *en_dev,
                                            DPP_PF_INFO_T *pf_info, bool allmulti)
{
    int32_t err = 0;

    err = dpp_vport_mc_promisc_set(pf_info, allmulti);
    if (err)
        return err;

    mutex_lock(&en_dev->parent->lock);
    en_dev->allmulti_enabled = allmulti;
    mutex_unlock(&en_dev->parent->lock);

    return 0;
}

static int zxdh_allmulti_process_vf(struct zxdh_en_device *en_dev,
                                            bool allmulti)
{
    int32_t err = 0;

    err = zxdh_vf_port_promisc_set(en_dev, ZXDH_ALLMULTI_MODE, allmulti, 0);
    if (err)
        return err;

    mutex_lock(&en_dev->parent->lock);
    en_dev->allmulti_enabled = allmulti;
    mutex_unlock(&en_dev->parent->lock);

    return 0;
}

static int32_t zxdh_promisc_process(struct zxdh_en_device *en_dev,
                                    bool is_pf, bool enable, bool allmulti_current)
{
    if (is_pf) {
        DPP_PF_INFO_T pf_info = {
            .slot = en_dev->slot_id,
            .vport = en_dev->vport,
        };
        return zxdh_promisc_process_pf(en_dev, &pf_info, enable, allmulti_current);
    }
    return zxdh_promisc_process_vf(en_dev, enable, allmulti_current);
}

static int32_t zxdh_allmulti_process(struct zxdh_en_device *en_dev,
                                     bool is_pf, bool enable)
{
    if (is_pf) {
        DPP_PF_INFO_T pf_info = {
            .slot = en_dev->slot_id,
            .vport = en_dev->vport,
        };
        return zxdh_allmulti_process_pf(en_dev, &pf_info, enable);
    }
    return zxdh_allmulti_process_vf(en_dev, enable);
}

int32_t zxdh_dev_promisc_sync(struct zxdh_en_device *en_dev)
{
    bool promisc_current = !!(en_dev->netdev->flags & IFF_PROMISC);
    bool allmulti_current = !!(en_dev->netdev->flags & IFF_ALLMULTI);
    bool is_pf = (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF);
    bool need_record_allmulti_cfg = false;
    int32_t ret = 0;
    /* 处理promisc模式变化 */
    if (en_dev->promisc_enabled != promisc_current) {
        LOG_INFO_DEV(en_dev->parent,
              "promisc need sync: promisc_current=%d, promisc_enabled=%d\n",
              promisc_current, en_dev->promisc_enabled);
        ret = zxdh_promisc_process(en_dev, is_pf, promisc_current, allmulti_current);
        LOG_INFO_DEV(en_dev->parent,
              "promisc sync done: promisc_enabled=%d\n",
              en_dev->promisc_enabled);
        if (ret)
            return ret;
    }

    /* 检查是否需要记录用户allmulti配置：
     * 当allmulti_current与记录的用户配置不一致时需要记录：
     * - allmulti_current=true，记录不是OPENED -> 记录为OPENED
     * - allmulti_current=false，记录是OPENED -> 记录为CLOSED
     * 默认状态(DO_NOT_CONFIGED)与allmulti_current=false一致，无需记录
     */
    need_record_allmulti_cfg =
        (allmulti_current && en_dev->user_allmulti_cfg != OPENED_ALLMULTI) ||
        (!allmulti_current && en_dev->user_allmulti_cfg == OPENED_ALLMULTI);

    if (need_record_allmulti_cfg) {
        LOG_INFO_DEV(en_dev->parent,
            "allmulti user cfg need record: allmulti_current=%d, user_allmulti_cfg=%d\n",
            allmulti_current, en_dev->user_allmulti_cfg);

        /* 记录用户配置 */
        zxdh_record_user_allmulti_cfg(en_dev, allmulti_current);
        LOG_INFO_DEV(en_dev->parent,
              "allmulti user cfg record done: user_allmulti_cfg=%d\n",
              en_dev->user_allmulti_cfg);
    }

    /* 处理allmulti模式变化 */
    if (en_dev->allmulti_enabled != allmulti_current) {
        /* promisc已开启时，allmulti已被强制开启，无需重复设置 */
        if (en_dev->promisc_enabled) {
            LOG_DEBUG_DEV(en_dev->parent, "promisc is enabled, skip allmulti setting\n");
            return 0;
        }

        LOG_INFO_DEV(en_dev->parent,
              "allmulti need sync: allmulti_current=%d, allmulti_enabled=%d\n",
              allmulti_current, en_dev->allmulti_enabled);
        ret = zxdh_allmulti_process(en_dev, is_pf, allmulti_current);
        LOG_INFO_DEV(en_dev->parent,
              "allmulti sync done: allmulti_enabled=%d\n",
              en_dev->allmulti_enabled);
        if (ret)
            return ret;
    }
    return 0;
}

void rx_mode_set_handler(struct work_struct *work)
{
    struct zxdh_en_device *en_dev = container_of(work, struct zxdh_en_device, rx_mode_set_work);
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
    struct mac_queue add_queue = {0};
    struct mac_queue del_queue = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    netif_addr_lock_bh(en_dev->netdev);
    zxdh_dev_mc_sync(en_dev->netdev, zxdh_addr_sync, zxdh_addr_unsync, &add_queue, &del_queue);
    netif_addr_unlock_bh(en_dev->netdev);

    err = zxdh_dev_mc_proc(en_dev, &pf_info, &add_queue, &del_queue);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dev_mc_proc err:0x%x\n", err);
    }

    err = zxdh_dev_promisc_sync(en_dev);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dev_promisc_sync err:0x%x\n", err);
    }
}

void zxdh_netdev_addr_set(struct net_device *dev, const u8 *addr)
{
    struct zxdh_en_priv *en_priv = netdev_priv(dev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
#if defined(USE_DEV_ADDR_SET) || defined(KYLIN_V11_6_6)
    dev_addr_set(dev, addr);
#else
    ether_addr_copy(dev->dev_addr, addr);
#endif
#ifdef CGS_V5_693
    /* 3.10 内核要求 perm_addr 也必须设置 */
    ether_addr_copy(dev->perm_addr, addr);
#endif
    ether_addr_copy(en_dev->eth_config.dev_addr, dev->dev_addr);
}

static int zxdh_en_set_mac(struct net_device *netdev, void *p)
{
    struct  sockaddr *addr         = (struct  sockaddr *)p;
    struct  zxdh_en_device *en_dev = NULL;
    struct  zxdh_en_priv *en_priv  = NULL;
    struct  netdev_hw_addr *ha     = NULL;
    bool    delete_flag            = true;
    bool    add_flag               = true;
    int32_t ret                    = 0;
    uint16_t sriov_vlan_tpid       = 0;
    uint16_t sriov_vlan_id         = 0;
    uint16_t current_vport         = 0;
    uint16_t vport                 = 0;
    DPP_PF_INFO_T pf_info = {0};

    en_priv = netdev_priv(netdev);
    en_dev  = &en_priv->edev;

    if (!is_valid_ether_addr(addr->sa_data))
    {
        LOG_INFO_DEV(en_dev->parent, "invalid mac address %pM\n", addr->sa_data);
        return -EADDRNOTAVAIL;
    }

    if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR)
    {
        LOG_INFO_DEV(en_dev->parent, "update %s mac %p in INTERNAL_ERROR\n", netdev->name, addr->sa_data);
        return -ENXIO;
    }

    if (ether_addr_equal(netdev->dev_addr, addr->sa_data))
    {
        if ((!en_dev->ops->is_bond(en_dev->parent)) && (!ether_addr_equal(en_dev->last_np_mac_addr.sa_data, netdev->dev_addr)))
        {
            zxdh_netdev_addr_set(netdev, en_dev->last_np_mac_addr.sa_data);
            goto continue_run;
        }
        LOG_INFO_DEV(en_dev->parent, "already using mac address %pM\n", addr->sa_data);
        return 0;
    }
continue_run:
    list_for_each_entry(ha, &netdev->uc.list, list)
    {
        if (!memcmp(ha->addr, netdev->dev_addr, netdev->addr_len))
        {
            delete_flag = false;
        }

        if (!memcmp(ha->addr, addr->sa_data, netdev->addr_len))
        {
            add_flag = false;
        }
    }

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        zxdh_netdev_addr_set(netdev, addr->sa_data);
        return 0;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        vport = pf_info.vport;
        ret = dpp_unicast_mac_search(&pf_info, addr->sa_data, sriov_vlan_tpid, sriov_vlan_id, &current_vport);
        if ((ret == 0) && (vport == current_vport))
        {
            return 0;
        }
        else if ((ret == 0) && (vport != current_vport))
        {
            LOG_ERR_DEV(en_dev->parent, "Mac already exists\n");
            return -EEXIST;
        }
        else if((ret != 0) && (ret != DPP_HASH_RC_SRH_FAIL))
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_search failed, ret:%d\n", ret);
            return -1;
        }

        if (delete_flag)
        {
            ret = dpp_del_mac(&pf_info, netdev->dev_addr, sriov_vlan_tpid, sriov_vlan_id);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "pf del mac failed, retval: %d\n", ret);
                return -1;
            }
        }

        if (add_flag)
        {
            ret = dpp_add_mac(&pf_info, addr->sa_data, sriov_vlan_tpid, sriov_vlan_id);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "pf add mac failed: %d\n", ret);
                return -1;
            }
        }

        LOG_DEBUG_DEV(en_dev->parent, "set pf new mac address %pM\n", addr->sa_data);
        zxdh_netdev_addr_set(netdev, addr->sa_data);
    }
    else
    {
        ret = zxdh_vf_dpp_dump_mac(en_dev, addr->sa_data);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "Mac already exists");
            return -EEXIST;
        }

        ret = zxdh_vf_dpp_del_mac(en_dev, netdev->dev_addr, UNFILTER_MAC, delete_flag);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh vf dpp del mac failed: %d\n", ret);
            return -1;
        }

        if (add_flag)
        {
            ret = zxdh_vf_dpp_add_mac(en_dev, addr->sa_data, UNFILTER_MAC);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "zxdh vf dpp add mac failed: %d\n", ret);
                return -1;
            }
            en_dev->ops->set_mac(en_dev->parent, addr->sa_data);
        }

        LOG_DEBUG_DEV(en_dev->parent, "set vf new mac address %pM\n", addr->sa_data);
        zxdh_netdev_addr_set(netdev, addr->sa_data);
    }
    ether_addr_copy(en_dev->last_np_mac_addr.sa_data, en_dev->netdev->dev_addr);
    LOG_DEBUG_DEV(en_dev->parent, "update last_np_mac_addr %pM\n", en_dev->last_np_mac_addr.sa_data);
    return ret;
}

int32_t zxdh_en_config_mtu_to_np(struct net_device *netdev, int32_t mtu_value)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA) {
        return ret;
    }

    if (!zxdh_en_is_panel_port(en_dev))
        return ret;

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        dpp_uplink_phy_attr_set(&pf_info, en_dev->phy_port, UPLINK_PHY_PORT_MTU_OFFLOAD_ENABLE, 1);
        dpp_uplink_phy_attr_set(&pf_info, en_dev->phy_port, UPLINK_PHY_PORT_MTU, mtu_value);
        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_MTU_OFFLOAD_EN_OFF, 1);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_pf_egr_port_attr_set config mtu enable failed: %d\n", ret);
            return ret;
        }
        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_MTU, mtu_value);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_pf_egr_port_attr_set config mtu value failed: %d\n", ret);
            return ret;
        }
    }
    else
    {
        ret = zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_MTU_OFFLOAD_EN_OFF, 1, 0);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_egr_port_attr_set config mtu enable failed: %d\n", ret);
            return ret;
        }
        ret = zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_MTU, mtu_value, 0);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_egr_port_attr_set config mut value failed: %d\n", ret);
            return ret;
        }
    }

    return 0;
}

static int zxdh_en_change_mtu(struct net_device *netdev, int new_mtu)
{
    int32_t ret = 0;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    if ((new_mtu < ETH_MIN_MTU) || (new_mtu > ZXDH_MAX_MTU))
    {
        LOG_ERR_DEV(en_dev->parent, "changing MTU over %d-%d\n", ETH_MIN_MTU, ZXDH_MAX_MTU);
        return -EINVAL;
    }
    LOG_DEBUG_DEV(en_dev->parent, "changing MTU from %d to %d\n", netdev->mtu, new_mtu);

    netdev->mtu = new_mtu;

    ret = zxdh_en_config_mtu_to_np(netdev, new_mtu);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_config_mtu_to_np failed: %d\n", ret);
        return -1;
    }

    return 0;
}

#ifdef HAVE_TX_TIMEOUT_TXQUEUE
static void zxdh_en_tx_timeout(struct net_device *netdev, unsigned int txqueue)
{
    return;
}
#else
static void zxdh_en_tx_timeout(struct net_device *netdev)
{
    return;
}
#endif

#ifdef HAVE_VLAN_RX_REGISTER
static void zxdh_en_vlan_rx_register(struct net_device *netdev, struct vlan_group *grp)
{
    return;
}
#endif

static int __attribute__((unused)) vf_vlan_rx_add_vid(struct net_device *netdev, u16 vid)
{
    int ret = 0;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_VLAN_FILTER_ADD;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.rx_vid_add_msg.vlan_id = vid;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0 || msg->reps.flag != ZXDH_REPS_SUCC)
    {
        LOG_ERR_DEV(en_dev->parent, "pcieid:0x%x send msg to pf add vlan:%d failed! ret = %d, flag = 0x%x\n",
                                                    en_dev->pcie_id,
                                                    vid,
                                                    ret,
                                                    msg->reps.flag);
        kfree(msg);
        return -1;
    }
    kfree(msg);
    return 0;
}

int vlan_set_bit(uint16_t vlan_id, void *bit_map)
{
    uint8_t *bitmap = 0;
    uint16_t byte_index = 0;
    uint8_t bit_index = 0;

    LOG_DEBUG("lan_set_bit, id:%d.\n", vlan_id);
    if (!bit_map) {
        return -1;
    }
    if (vlan_id >= VLAN_BITMAP_LENGTH) {
        return -2;
    }

    bitmap = (uint8_t *)bit_map;
    byte_index = vlan_id / BIT_NUM_PER_BYTE;
    bit_index = vlan_id % BIT_NUM_PER_BYTE;

    bitmap[byte_index] |= (1 << bit_index);

    return 0;
}

int vlan_reset_bit(uint16_t vlan_id, void *bit_map)
{
    uint8_t *bitmap = 0;
    uint16_t byte_index = 0;
    uint8_t bit_index = 0;

    LOG_DEBUG("lan_reset_bit, id:%d.\n", vlan_id);

    if (!bit_map) {
        return -1;
    }
    if (vlan_id >= VLAN_BITMAP_LENGTH) {
        return -2;
    }

    bitmap = (uint8_t *)bit_map;
    byte_index = vlan_id / BIT_NUM_PER_BYTE;
    bit_index = vlan_id % BIT_NUM_PER_BYTE;

    bitmap[byte_index] &= ~(1 << bit_index);

    return 0;
}

#if defined(HAVE_INT_NDO_VLAN_RX_ADD_VID) && defined(NETIF_F_HW_VLAN_CTAG_RX)
static int zxdh_en_vlan_rx_add_vid(struct net_device *netdev, __always_unused __be16 proto, u16 vid)
{
    int retval = 0;

    struct zxdh_en_priv *zxdev = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &zxdev->edev;
    DPP_PF_INFO_T pf_info = {0};

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (vid > MAX_VLAN_ID)
    {
        LOG_ERR_DEV(en_dev->parent, "vlan id:%d input is err!\n", vid);
        return -EINVAL;
    }

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) /* VF */
    {
        retval = vf_vlan_rx_add_vid(netdev, vid);
        goto exit;
    }

    retval = dpp_add_vlan_filter(&pf_info, vid);
    if (0 != retval)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to add vlan: %d\n",vid);
        goto exit;
    }
    LOG_DEBUG_DEV(en_dev->parent, "pf add vlan %d succeed, retval %d.\n", vid, retval);


exit:
    if (retval == 0)
    {
        retval = vlan_set_bit(vid, en_dev->eth_config.vlan_trunk_bitmap);
    }

    return retval;
}
#elif defined(HAVE_INT_NDO_VLAN_RX_ADD_VID) && !defined(NETIF_F_HW_VLAN_CTAG_RX)
static int zxdh_en_vlan_rx_add_vid(struct net_device *netdev, u16 vid)
{
    return 0;
}
#else
static void zxdh_en_vlan_rx_add_vid(struct net_device *netdev, u16 vid)
{
    return;
}
#endif

static int vf_vlan_rx_del_vid(struct net_device *netdev, u16 vid)
{
    int ret = 0;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (unlikely(NULL == msg))
    {
        LOG_ERR_DEV(en_dev->parent, "failed to kzalloc\n");
        return -ENOMEM;
    }

    msg->payload.hdr.op_code = ZXDH_VLAN_FILTER_DEL;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.rx_vid_del_msg.vlan_id = vid;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0 || msg->reps.flag != ZXDH_REPS_SUCC)
    {
        LOG_ERR_DEV(en_dev->parent, "pcieid:0x%x send msg to pf del vlan:%d failed! ret = %d, flag = 0x%x\n",
                                                    en_dev->pcie_id,
                                                    vid,
                                                    ret,
                                                    msg->reps.flag);
        kfree(msg);
        return -1;
    }
    kfree(msg);
    return 0;
}

#if defined(HAVE_INT_NDO_VLAN_RX_ADD_VID) && defined(NETIF_F_HW_VLAN_CTAG_RX)
static int zxdh_en_vlan_rx_kill_vid(struct net_device *netdev, __always_unused __be16 proto, u16 vid)
{
    int retval = 0;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    DPP_PF_INFO_T pf_info = {0};

    LOG_DEBUG_DEV(en_dev->parent, "del vid: %d.\n", vid);
    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (vid > MAX_VLAN_ID)
    {
        LOG_ERR_DEV(en_dev->parent, "vlan id:%d input is err!\n", vid);
        return -EINVAL;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF) /* VF */
    {
        retval = vf_vlan_rx_del_vid(netdev, vid);
        goto exit;
    }

    retval = dpp_del_vlan_filter(&pf_info, vid);
    if (0 != retval)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to del vlan: %d\n", vid);
        goto exit;
    }
    LOG_DEBUG_DEV(en_dev->parent, "pf del vlan %d succeed.\n", vid);

exit:
    if (!retval)
    {
        retval = vlan_reset_bit(vid, en_dev->eth_config.vlan_trunk_bitmap);
    }

    return retval;
}
#elif defined(HAVE_INT_NDO_VLAN_RX_ADD_VID) && !defined(NETIF_F_HW_VLAN_CTAG_RX)
static int zxdh_en_vlan_rx_kill_vid(struct net_device *netdev, u16 vid)
{
    return 0;
}
#else
static void zxdh_en_vlan_rx_kill_vid(struct net_device *netdev, u16 vid)
{
    return;
}
#endif

#ifndef CGS_V5_693
static int zxdh_en_xdp_set(struct net_device *dev, struct bpf_prog *prog,
                            struct netlink_ext_ack *extack)
{
    unsigned long int max_sz = PAGE_SIZE - sizeof(struct padded_zxdh_net_hdr);
    struct zxdh_en_priv *en_priv = netdev_priv(dev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct bpf_prog *old_prog = NULL;
    uint16_t xdp_qp = 0;
    uint16_t curr_qp = 0;
    int i = 0;

    if ((dev->features & NETIF_F_GRO_HW) || (dev->features & NETIF_F_HW_CSUM)) {
        LOG_ERR_DEV(en_dev->parent, "Can't set XDP while host is implementing GRO_HW/CSUM, disable GRO_HW/CSUM first\n");
        return -EOPNOTSUPP;
    }

    if (en_dev->mergeable_rx_bufs && !en_dev->any_header_sg) {
        LOG_ERR_DEV(en_dev->parent, "XDP expects header/data in single page, any_header_sg required\n");
        return -EINVAL;
    }

    if (dev->mtu > max_sz) {
        LOG_ERR_DEV(en_dev->parent, "MTU too large to enable XDP\n");
        netdev_warn(dev, "XDP requires MTU less than %lu\n", max_sz);
        return -EINVAL;
    }

    if (en_dev->eth_config.num_txq != en_dev->eth_config.num_rxq) {
        LOG_ERR_DEV(en_dev->parent, "num_txq:%d != num_rxq:%d, not support to enable xdp\n",
                    en_dev->eth_config.num_txq, en_dev->eth_config.num_rxq);
        return -EINVAL;
    }

    curr_qp = en_dev->curr_queue_pairs - en_dev->xdp_queue_pairs;
    if (prog)
        xdp_qp = 0;//nr_cpu_ids

    /* XDP requires extra queues for XDP_TX */
    if (curr_qp + xdp_qp > en_dev->max_vq_pairs) {
        netdev_warn_once(dev, "XDP request %i queues but max is %i. XDP_TX and XDP_REDIRECT will operate in a slower locked tx mode.\n",
                 curr_qp + xdp_qp, en_dev->max_vq_pairs);
        xdp_qp = 0;
    }

    old_prog = rtnl_dereference(en_dev->rq[0].xdp_prog);
    if (!prog && !old_prog)
        return 0;

    if (prog) {
#if ( defined (BCLINUX_HG) || defined(KYLIN_V10) || defined(CTYUNOS_4_19_5_10) ||  defined(HAVE_UOS_RELEASE_CODE) || \
     (RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8, 6)) || \
     (!RHEL_RELEASE_CODE && (LINUX_VERSION_CODE > KERNEL_VERSION(5,4,0))))
        bpf_prog_add(prog, en_dev->max_vq_pairs - 1);
#else
        if (bpf_prog_add(prog, en_dev->max_vq_pairs - 1) != 0)
            LOG_DEBUG_DEV(en_dev->parent, "bpf_prog_add failed\n");
#endif
    }

    /* Make sure NAPI is not using any XDP TX queues for RX. */
    if (netif_running(dev)) {
        for (i = 0; i < en_dev->max_vq_pairs; i++) {
            napi_disable(&en_dev->rq[i].napi);
            virtnet_napi_tx_disable(&en_dev->sq[i].napi);
        }
    }

    en_dev->xdp_enabled = !!prog;
    for (i = 0; i < en_dev->max_vq_pairs; i++) {
        rcu_assign_pointer(en_dev->rq[i].xdp_prog, prog);
    }

    for (i = 0; i < en_dev->max_vq_pairs; i++) {
        if (old_prog)
            bpf_prog_put(old_prog);
        if (netif_running(dev)) {
            virtnet_napi_enable(en_dev->rq[i].vq, &en_dev->rq[i].napi);
            virtnet_napi_tx_enable(dev, en_dev->sq[i].vq, &en_dev->sq[i].napi);
        }
    }

    return 0;
}
#endif /* CGS_V5_693 */

#if (!((RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8, 6)) || \
     (!RHEL_RELEASE_CODE && (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)))))
#ifndef CTYUNOS_4_19_5_10
#ifndef CGS_V5_693
static uint32_t zxdh_xdp_query(struct net_device *dev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(dev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    const struct bpf_prog *xdp_prog = NULL;
    int32_t i;

    for (i = 0; i < en_dev->max_vq_pairs; i++) {
        xdp_prog = rtnl_dereference(en_dev->rq[i].xdp_prog);
        if (xdp_prog)
            return xdp_prog->aux->id;
    }
    return 0;
}
#endif /* CGS_V5_693 */
#endif
#endif

#ifndef CGS_V5_693
int zxdh_en_xdp(struct net_device *dev, struct netdev_bpf *xdp)
{
    switch (xdp->command) {
    case XDP_SETUP_PROG:
        return zxdh_en_xdp_set(dev, xdp->prog, xdp->extack);
#if (!((RHEL_RELEASE_CODE >= RHEL_RELEASE_VERSION(8, 6)) || \
     (!RHEL_RELEASE_CODE && (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)))))
#ifndef CTYUNOS_4_19_5_10
    case XDP_QUERY_PROG:
        xdp->prog_id = zxdh_xdp_query(dev);
        return 0;
#endif
#endif
    default:
        return -EINVAL;
    }
}
#endif /* CGS_V5_693 */

#ifdef CONFIG_NET_POLL_CONTROLLER
static void zxdh_en_netpoll(struct net_device *netdev)
{
    return;
}
#endif

#ifdef HAVE_SETUP_TC
int zxdh_en_setup_tc(struct net_device *netdev, u8 tc)
{
    return 0;
}

#ifdef NETIF_F_HW_TC
#ifdef HAVE_NDO_SETUP_TC_REMOVE_TC_TO_NETDEV
static LIST_HEAD(zxdh_en_block_cb_list);
static int __zxdh_en_setup_tc(struct net_device *netdev, enum tc_setup_type type, void *type_data)
{
    #ifdef HAVE_TC_FLOW_OFFLOAD
    struct zxdh_en_priv *priv = netdev_priv(netdev);
    struct flow_block_offload *f = type_data;
    bool tc_unbind = false;

    LOG_DEBUG_DEV(priv->edev.parent, "__zxdh_en_setup_tc f=%p slot=%u vport=0x%x type=%d\n",
                   f,priv->edev.slot_id,priv->edev.vport,type);

    if ((type == TC_SETUP_BLOCK) && (f->command == FLOW_BLOCK_UNBIND)) {
        tc_unbind = true;
    }

    if (!netif_device_present(netdev) && !tc_unbind) {
        return -ENODEV;
    }

    switch (type)
    {
        case TC_SETUP_BLOCK:
        {
            f->unlocked_driver_cb = true;
            return flow_block_cb_setup_simple(type_data,
                          &zxdh_en_block_cb_list,
                          zxdh_setup_tc_block_cb,
                          priv, priv, true);
        }

        default:
            return -EOPNOTSUPP;
    }
    #endif
    return 0;
}
#elif defined(HAVE_NDO_SETUP_TC_CHAIN_INDEX)
static int __zxdh_en_setup_tc(struct net_device *netdev, u32 handle,
                              u32 chain_index, __be16 proto,
                              struct tc_to_netdev *tc)
{
    return 0;
}
#else
static int __zxdh_en_setup_tc(struct net_device *netdev, u32 handle, __be16 proto,
                              struct tc_to_netdev *tc)
{
    return 0;
}
#endif
#endif  /* NETIF_F_HW_TC */
#endif  /* HAVE_SETUP_TC */

static int32_t zxdh_dtp_offload_set(struct zxdh_en_device *en_dev, DPP_PF_INFO_T *pf_info)
{
    ZXDH_SRIOV_VPORT_T port_attr_entry = {0};
    int32_t ret = 0;

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        ret = dpp_vport_attr_get(pf_info, &port_attr_entry);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_get failed: %d\n", ret);
            return ret;
        }

        if (!port_attr_entry.lro_offload && !port_attr_entry.ip_recombine_offload &&
            !port_attr_entry.ip_checksum_offload && !port_attr_entry.tcp_udp_checksum_offload)
        {
            ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_ACCELERATOR_OFFLOAD_FLAG, 0);
        }
        else
        {
            ret = dpp_vport_attr_set(pf_info, SRIOV_VPORT_ACCELERATOR_OFFLOAD_FLAG, 1);
        }

        return ret;
    }

    ret = zxdh_vf_egr_port_attr_get(en_dev, &port_attr_entry);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_get failed: %d\n", ret);
        return ret;
    }

    if (!port_attr_entry.lro_offload && !port_attr_entry.ip_recombine_offload &&
        !port_attr_entry.ip_checksum_offload && !port_attr_entry.tcp_udp_checksum_offload)
    {
        ret = zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_ACCELERATOR_OFFLOAD_FLAG, 0, 0);
    }
    else
    {
        ret = zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_ACCELERATOR_OFFLOAD_FLAG, 1, 0);
    }

    return ret;
}

static int32_t set_feature_rx_checksum(struct zxdh_en_device *en_dev, bool enable)
{
    int en_value = enable ? 1 : 0;
    DPP_PF_INFO_T pf_info = {0};
    int32_t ret = 0;

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_IP_CHKSUM, enable);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_IP_CHKSUM set failed: %d\n", ret);
            return ret;
        }
        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_TCP_UDP_CHKSUM, enable);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_TCP_UDP_CHKSUM set failed: %d\n", ret);
            return ret;
        }
        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_OUTER_IP_CHECKSUM_OFFLOAD, enable);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_OUTER_IP_CHECKSUM_OFFLOAD set failed: %d\n", ret);
            return ret;
        }
        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_OUTER_L4_CHECKSUM_OFFLOAD, enable);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_OUTER_L4_CHECKSUM_OFFLOAD set failed: %d\n", ret);
            return ret;
        }
    }
    else if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        ret = zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_IP_CHKSUM, en_value, 0);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_IP_CHKSUM set failed: %d\n", ret);
            return ret;
        }
        ret = zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_TCP_UDP_CHKSUM, en_value, 0);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_TCP_UDP_CHKSUM set failed: %d\n", ret);
            return ret;
        }
        ret = zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_OUTER_IP_CHECKSUM_OFFLOAD, en_value, 0);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_OUTER_IP_CHECKSUM_OFFLOAD set failed: %d\n", ret);
            return ret;
        }
        ret = zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_OUTER_L4_CHECKSUM_OFFLOAD, en_value, 0);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_OUTER_L4_CHECKSUM_OFFLOAD set failed: %d\n", ret);
            return ret;
        }
    }

    return zxdh_dtp_offload_set(en_dev, &pf_info);
}

static int set_feature_tx_checksum(struct zxdh_en_device *en_dev, bool enable)
{
    return 0;
}

static int set_feature_tso(struct zxdh_en_device *en_dev, bool enable)
{
    return 0;
}

static int set_feature_tso6(struct zxdh_en_device *en_dev, bool enable)
{
    return 0;
}

static int set_feature_udp_tunnel_checksum(struct zxdh_en_device *en_dev, bool enable)
{
    return 0;
}

int32_t set_feature_rxhash(struct zxdh_en_device *en_dev, bool enable)
{
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        return dpp_vport_rss_en_set(&pf_info, enable);
    }

    return zxdh_vf_rss_en_set(en_dev, enable);
}

int32_t set_feature_ntuple(struct zxdh_en_device *en_dev, bool enable)
{
    DPP_PF_INFO_T pf_info = {0};
    bool tc_enable = !!(en_dev->netdev->features & NETIF_F_HW_TC);

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

     if(!enable && tc_enable)
    {
        LOG_INFO_DEV(en_dev->parent, "when ntuple_filter off and hw_tc_offload on,keep fd_en status\n");
        return 0;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        return dpp_vport_fd_en_set(&pf_info, enable);
    }

    return zxdh_vf_fd_en_set(en_dev, enable);
}

int32_t set_feature_tc(struct zxdh_en_device *en_dev, bool enable)
{
#ifdef CGS_V5_693
    /* 3.10 内核 TC offload 框架不完整，直接返回成功 */
    return 0;
#else
    DPP_PF_INFO_T pf_info = {0};
    struct zxdh_en_priv *en_priv = netdev_priv(en_dev->netdev);
    bool ntuple_enable = !!(en_dev->netdev->features & NETIF_F_NTUPLE);

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if( !enable && zxdh_tc_num_filters(en_priv, 0))
    {
        LOG_ERR_DEV(en_dev->parent, "Active offloaded tc filters, can't turn hw_tc_offload off\n");
        return -EINVAL;
    }

    if(!enable && ntuple_enable)
    {
        LOG_INFO_DEV(en_dev->parent, "when ntuple_filter on and hw_tc_offload off,keep fd_en status\n");
        return 0;
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        return dpp_vport_fd_en_set(&pf_info, enable);
    }

    return zxdh_vf_fd_en_set(en_dev, enable);
#endif
}

static int32_t set_vf_cvlan_filter(struct zxdh_en_device *en_dev, bool enable)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr.op_code = ZXDH_VLAN_FILTER_SET;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.vlan_filter_set_msg.enable = enable;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0 || msg->reps.flag != ZXDH_REPS_SUCC)
    {
        LOG_ERR_DEV(en_dev->parent, "pcieid:0x%x send msg to pf set vlan filter enable:%s failed! ret = %d, flag = 0x%x\n",
                                                    en_dev->pcie_id,
                                                    enable ? "enable":"disable",
                                                    ret,
                                                    msg->reps.flag);
    }

    kfree(msg);
    return ret;
}

/**
 * zxdh_pf_switch_business_vlan - 配置bussness vlan子开关
 * @pf_info: pf信息
 * @type: vlan二级开关
 * @wanted_feature: 切换的值
 */
int zxdh_pf_switch_business_vlan(DPP_PF_INFO_T *pf_info, uint8_t type, uint32_t wanted_feature)
{
    int ret = 0;
    ZXDH_VQM_VFID_VLAN_T vf_vlan_attr = {0};
    bool old_vport_bit = 0;
    bool wanted_vport_bit = 0;
    uint32_t *changed_vlan_attr = NULL;

    /* sizeof(vf_vlan_attr)/sizeof(vf_vlan_attr.rsv): 结构体成员数量*/
    if (type >= sizeof(vf_vlan_attr)/sizeof(vf_vlan_attr.rsv))
    {
        LOG_ERR("zxdh_pf_switch_business_vlan para type err: %u.\n", type);
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

    /* 先切换二级开关*/
    ret = dpp_vqm_vfid_vlan_set(pf_info, type, wanted_feature);
    if (ret != 0)
    {
        LOG_ERR("dpp_vqm_vfid_vlan_set, ret: %d\n", ret);
        return -1;
    }

    /* 如果一级开关不需要切换则退出*/
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

static int set_feature_cvlan_filter(struct zxdh_en_device *en_dev, bool enable)
{
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if ((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)) /* VF */
    {
        return set_vf_cvlan_filter(en_dev, enable);
    }

    return zxdh_pf_switch_business_vlan(&pf_info, VLAN_SRIOV_BUSINESS_VLAN_FILTER, enable);
}

static int __attribute__((unused)) set_feature_svlan_filter(struct zxdh_en_device *en_dev, bool enable)
{
    int ret = 0;
#if 0 //TODO：STAG 暂时没有设置
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    ret = dpp_vport_vlan_qinq_en_set(en_dev->vport, enable);
#endif
    return ret;
}

int set_vf_qinq_tpid(struct zxdh_en_device *en_dev, uint16_t tpid)
{
    union zxdh_msg *msg = NULL;
    int ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -1;
    }

    msg->payload.hdr.op_code = ZXDH_SET_TPID;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.tpid_cfg_msg.tpid = tpid;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0 || msg->reps.flag != ZXDH_REPS_SUCC)
    {
        LOG_ERR_DEV(en_dev->parent, "pcieid:0x%x send msg to vfs set tpid: 0x%x failed! ret = %d.\n",
                                                    en_dev->pcie_id,
                                                    tpid,
                                                    ret);
        kfree(msg);
        return -EINVAL;
    }

    kfree(msg);
    return 0;
}

static int set_vf_vlan_strip(struct zxdh_en_device *en_dev, bool enable, uint8_t flag)
{
    union zxdh_msg *msg = NULL;
    int ret = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr.op_code = ZXDH_VLAN_OFFLOAD_SET;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    msg->payload.vlan_strip_msg.enable = enable;
    msg->payload.vlan_strip_msg.flag = flag;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (ret != 0 || msg->reps.flag != ZXDH_REPS_SUCC)
    {
        LOG_ERR_DEV(en_dev->parent, "pcieid:0x%x send msg to vfs set vlan strip enable:%s failed! ret = %d, flag = 0x%x\n",
                                                    en_dev->pcie_id,
                                                    enable ? "enable":"disable",
                                                    ret,
                                                    msg->reps.flag);
        ret = -EINVAL;
    }

    kfree(msg);
    return ret;
}

static int set_feature_vlan_strip(struct zxdh_en_device *en_dev, bool enable)
{
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        return zxdh_pf_switch_business_vlan(&pf_info, VLAN_SRIOV_BUSINESS_VLAN_STRIP_OFFLIAD, enable);
    }

    return set_vf_vlan_strip(en_dev, enable, VLAN_STRIP_MSG_TYPE);
}


static int set_feature_qinq_strip(struct zxdh_en_device *en_dev, bool enable)
{
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        return zxdh_pf_switch_business_vlan(&pf_info, VLAN_SRIOV_BUSINESS_QINQ_VLAN_STRIP_OFFLOAD, enable);
    }

    return set_vf_vlan_strip(en_dev, enable, QINQ_STRIP_MSG_TYPE);
}


static int32_t set_feature_lro(struct zxdh_en_device *en_dev, bool enable)
{
    uint32_t en_value = enable ? 1 : 0;
    DPP_PF_INFO_T pf_info = {0};
    int32_t ret = 0;

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_IPV4_TCP_ASSEMBLE, en_value);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_IPV4_TCP_ASSEMBLE set failed: %d\n", ret);
            return ret;
        }
        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_IPV6_TCP_ASSEMBLE, en_value);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_IPV6_TCP_ASSEMBLE set failed: %d\n", ret);
            return ret;
        }
    }
    else
    {
        ret = zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_IPV4_TCP_ASSEMBLE, en_value, 0);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_IPV4_TCP_ASSEMBLE set failed: %d\n", ret);
            return ret;
        }
        ret = zxdh_vf_egr_port_attr_set(en_dev, SRIOV_VPORT_IPV6_TCP_ASSEMBLE, en_value, 0);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "SRIOV_VPORT_IPV6_TCP_ASSEMBLE set failed: %d\n", ret);
            return ret;
        }
    }

    return zxdh_dtp_offload_set(en_dev, &pf_info);
}

static const struct {
    netdev_features_t feature;
    zxdh_feature_handler handler;
} feature_handlers[] = {
    {NETIF_F_RXCSUM,              set_feature_rx_checksum },
    {NETIF_F_HW_CSUM,             set_feature_tx_checksum },
    {NETIF_F_TSO,                 set_feature_tso },
    {NETIF_F_TSO6,                set_feature_tso6 },
    {NETIF_F_GSO_UDP_TUNNEL_CSUM, set_feature_udp_tunnel_checksum },
    {NETIF_F_RXHASH,              set_feature_rxhash },
    {NETIF_F_LRO,                 set_feature_lro },
    {NETIF_F_NTUPLE,              set_feature_ntuple },
    {NETIF_F_HW_VLAN_CTAG_RX,     set_feature_vlan_strip },
    {NETIF_F_HW_VLAN_STAG_RX,     set_feature_qinq_strip },
    {NETIF_F_HW_VLAN_CTAG_FILTER, set_feature_cvlan_filter },
    {NETIF_F_HW_TC,               set_feature_tc},
};

int32_t zxdh_en_sync_features(struct zxdh_en_device *en_dev, netdev_features_t want_features)
{
    int32_t err = 0;
    int32_t i = 0;
    netdev_features_t feature;
    bool enable;

    LOG_DEBUG_DEV(en_dev->parent, "curr_features: 0x%llx, want_features: 0x%llx\n", en_dev->netdev->features, want_features);
    for (i = 0; i < ARRAY_SIZE(feature_handlers); i++) {
        feature = feature_handlers[i].feature;
        enable = !!(want_features & feature);

        if (feature_handlers[i].handler)
            err = feature_handlers[i].handler(en_dev, enable);
        if (err) {
            LOG_ERR_DEV(en_dev->parent, "%s feature %pNF (%#llx) failed %d\n",
                enable ? "Enable" : "Disable", &feature, feature, err);
        } else {
            en_dev->netdev->features = enable ? (en_dev->netdev->features | feature) :
                            (en_dev->netdev->features & ~feature);
        }
    }

    en_dev->features = en_dev->netdev->features;
    zxdh_netdev_features_over_dtp(en_dev->netdev);
    return err;
}

int32_t zxdh_en_set_features(struct net_device *netdev, netdev_features_t wanted_features)
{
    const netdev_features_t changes = wanted_features ^ netdev->features;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t err = 0;
    int32_t ret = 0;
    int32_t i = 0;
    netdev_features_t feature;
    bool enable;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    LOG_DEBUG_DEV(en_dev->parent, "curr_features: 0x%llx, wanted_features: 0x%llx\n", netdev->features, wanted_features);
#ifndef CGS_V5_693
    if (en_dev->xdp_enabled)
    {
        LOG_ERR_DEV(en_dev->parent, "XDP is enabled, can't change features\n");
        return -EBUSY;
    }
#endif

    for (i = 0; i < ARRAY_SIZE(feature_handlers); i++) {
        feature = feature_handlers[i].feature;

        if (!(changes & feature)) {
            continue;
        }

        enable = !!(wanted_features & feature);

        if (feature_handlers[i].handler) {
            LOG_DEBUG_DEV(en_dev->parent, "%s feature %pNF (%#llx), err %d\n",
                    enable ? "Enable" : "Disable", &feature, feature, err);
            err = feature_handlers[i].handler(en_dev, enable);
        }
        if (err) {
            ret |= err;
            continue;
        }
        netdev->features = enable ? (netdev->features | feature) :
                            (netdev->features & ~feature);
    }

    en_dev->features = netdev->features;
    zxdh_netdev_features_over_dtp(netdev);
    return ret;
}

/* 创建新节点 */
static uint32_t list_vlan_mac_create(struct zxdh_vlan_to_mac_list *list, uint16_t vlan_id, const uint8_t *addr)
{
    struct zxdh_vlan_to_mac *vtm = NULL;

    if (list == NULL || addr == NULL)
    {
        LOG_ERR("Invalid input: list=%p, addr=%p\n", list, addr);
        return 1;
    }

    vtm = kzalloc(sizeof(struct zxdh_vlan_to_mac), GFP_KERNEL);
    if (vtm == NULL)
    {
        LOG_ERR("Kzalloc zxdh_vlan_to_mac struct failed\n");
        return 1;
    }

    ether_addr_copy(vtm->addr, addr);
    vtm->vlan_id = vlan_id;

    spin_lock(&list->lock);
    list_add_tail_rcu(&vtm->list, &list->list);
    list->count++; /* 链表节点加1 */
    spin_unlock(&list->lock);

    return 0;
}

static uint32_t list_hw_addr_create(struct netdev_hw_addr_list *list, \
                                    const uint8_t *addr, int32_t addr_len,  \
                                    uint8_t addr_type, bool global,   \
                                    bool sync, struct rb_node *parent,
                                    struct rb_node **ins_point)
{
    struct netdev_hw_addr *ha = NULL;

    ha = kzalloc(sizeof(struct netdev_hw_addr), GFP_KERNEL);
    if (ha == NULL)
    {
        LOG_ERR("Kzalloc struct netdev_hw_addr failed \n");
        return 1;
    }

    /* 结构体赋值 */
    memcpy(ha->addr, addr, addr_len);
    ha->type       = addr_type;
    ha->refcount   = 1; /* 引用计数 */
    ha->global_use = global;
    ha->synced     = sync ? 1 : 0;
    ha->sync_cnt   = 0;
#ifdef HAVE_RB_TREE_OPS
    rb_link_node(&ha->node, parent, ins_point);
    rb_insert_color(&ha->node, &list->tree);
#endif
    list_add_tail_rcu(&ha->list, &list->list);
    list->count++; /* 链表节点加1 */

    return 0;
}

static uint32_t list_hw_addr_del(struct netdev_hw_addr_list *list, struct netdev_hw_addr *ha)
{
    int32_t refcount = ha->refcount;

    /* 引用的计数大于1，则不能删除此mac地址 */
    if (--refcount)
        return 1;

#ifdef HAVE_RB_TREE_OPS
    /* 检查节点是否在树中，避免重复删除导致 GPF */
    if (!RB_EMPTY_NODE(&ha->node))
    {
        rb_erase(&ha->node, &list->tree);
        RB_CLEAR_NODE(&ha->node);
    }
#endif

    /* 从链表中删除此条目 */
    list_del_rcu(&ha->list);

    /* 释放ha结构体占用内存，rcu_head可以安全地释放ha占用的内存*/
    kfree_rcu(ha, rcu_head);
    list->count--;

    return 0;
}

static void list_vlan_mac_del(struct zxdh_vlan_to_mac_list *list, uint16_t vlan_id,const uint8_t *vlan_mac)
{
    struct zxdh_vlan_to_mac *vtm = NULL;
    struct zxdh_vlan_to_mac *tmp = NULL;

    spin_lock(&list->lock);
    list_for_each_entry_safe(vtm, tmp, &list->list, list)
    {
        /* 找到该Vlan_id对应的mac地址 */
        if ((ether_addr_equal(vtm->addr, vlan_mac)) && (vtm->vlan_id == vlan_id))
        {
            /* 从链表中删除此条目 */
            list_del_rcu(&vtm->list);
            kfree_rcu(vtm, rcu_head);
            list->count--;
            break;
        }
    }
    spin_unlock(&list->lock);
    return;
}

#ifdef HAVE_RB_TREE_OPS
bool is_this_mac_exist(struct net_device *netdev, const uint8_t *addr, struct netdev_hw_addr **ha, struct rb_node ***ins_point, struct rb_node **parent)
{
    bool isexist = false;
    struct netdev_hw_addr *entry = NULL;
    unsigned char addr_type;
    int diff;

    if (is_unicast_ether_addr(addr))
    {
        addr_type = NETDEV_HW_ADDR_T_UNICAST;
        *ins_point = &netdev->uc.tree.rb_node;
    }
    else
    {
        addr_type = NETDEV_HW_ADDR_T_MULTICAST;
        *ins_point = &netdev->mc.tree.rb_node;
    }

    /* 给net_device结构体上锁 */
    netif_addr_lock_bh(netdev);

    while (**ins_point)
    {

        entry = rb_entry(**ins_point, struct netdev_hw_addr, node);
        diff = memcmp(addr, entry->addr, netdev->addr_len);
        if (diff == 0)
        {
            diff = memcmp(&addr_type, &entry->type, sizeof(addr_type));
        }
        *parent = **ins_point;
        if (diff < 0)
        {
            *ins_point = &((*parent)->rb_left);
        }
        else if (diff > 0)
        {
            *ins_point = &((*parent)->rb_right);
        }
        else //存在此MAC
        {
            isexist = true;
            *ha = entry;
            goto out;
        }
    }

out:
    if (!isexist)
    {
        *ha = NULL;
    }
    /* 给net_device结构体释放锁 */
    netif_addr_unlock_bh(netdev);

    return isexist;
}
#else
bool is_this_mac_exist(struct net_device *netdev, const uint8_t *addr, struct netdev_hw_addr **ha)
{
    bool isexist = false;
    struct netdev_hw_addr *entry = NULL;

    /* 给net_device结构体上锁 */
    netif_addr_lock_bh(netdev);

    /* 判断此mac地址类型 */
    if (is_unicast_ether_addr(addr) || is_link_local_ether_addr(addr))
    {
        /* 遍历单播mac地址链表 */
        list_for_each_entry(entry, &netdev->uc.list, list)
        {
            /* 检查该单播地址链表中是否存在此mac，且此mac地址标志为单播 */
            if ((!memcmp(entry->addr, addr, netdev->addr_len)) \
                && (entry->type == NETDEV_HW_ADDR_T_UNICAST))
            {
                isexist = true;
                *ha = entry;
                goto out;
            }
        }
    }
    else
    {
        /* 遍历组播mac地址链表 */
        list_for_each_entry(entry, &netdev->mc.list, list)
        {
            /* 检查该组播地址链表中是否存在此mac，且此mac地址类型为组播 */
            if ((!memcmp(entry->addr, addr, netdev->addr_len)) \
                && (entry->type == NETDEV_HW_ADDR_T_MULTICAST))
            {
                isexist = true;
                *ha = entry;
                goto out;
            }
        }
    }

out:
    if (!isexist)
    {
        *ha = NULL;
    }
    /* 给net_device结构体释放锁 */
    netif_addr_unlock_bh(netdev);

    return isexist;
}
#endif

/**
 * zxdh_dev_list_addr_add - 在地址链表中添加此mac地址
 * @netdev: 网络设备结构体
 * @addr: 要添加的mac地址
 * @addr_type: mac地址类型
 */
int32_t zxdh_dev_list_addr_add(struct net_device *netdev, const uint8_t *addr, struct rb_node *parent,
                                    struct rb_node **ins_point)
{
    int32_t err = 0;

    /* 给net_device结构体上锁 */
    netif_addr_lock_bh(netdev);

    /* 判断此mac地址类型 */
    if (is_unicast_ether_addr(addr) || is_link_local_ether_addr(addr))
    {
        /* 将此mac地址添加到地址链表中 */
        err = list_hw_addr_create(&netdev->uc, addr, netdev->addr_len, \
                                  NETDEV_HW_ADDR_T_UNICAST, false, false, parent, ins_point);
        if (err != 0)
        {
            LOG_ERR("list_hw_addr_create failed\n");
        }
    }
    else
    {
        err = list_hw_addr_create(&netdev->mc, addr, netdev->addr_len, \
                                 NETDEV_HW_ADDR_T_MULTICAST, false, false, parent, ins_point);
        if (err != 0)
        {
            LOG_ERR("list_hw_addr_create failed\n");
        }
    }

    /* 给net_device结构体释放锁 */
    netif_addr_unlock_bh(netdev);

    return err;
}

/**
 * zxdh_dev_list_addr_del - 在地址链表中删除此mac地址
 * @netdev: 网络设备结构体
 * @addr: 要删除的mac地址
 * @addr_type: mac地址类型
 */
int32_t zxdh_dev_list_addr_del(struct net_device *netdev, const uint8_t *addr)
{
    struct netdev_hw_addr *ha = NULL;
    int32_t err = 0;

    /* 给net_device上锁 */
    netif_addr_lock_bh(netdev);

    if (is_unicast_ether_addr(addr) || is_link_local_ether_addr(addr))
    {
        /* 遍历单播mac地址链表 */
        list_for_each_entry(ha, &netdev->uc.list, list)
        {
            /* 检查该单播地址链表中是否存在此mac，且此mac地址标志为单播 */
            if ((!memcmp(ha->addr, addr, netdev->addr_len)) \
                && (ha->type == NETDEV_HW_ADDR_T_UNICAST))
            {
                /* 从单播地址链表中删除此mac */
                err = list_hw_addr_del(&netdev->uc, ha);
                if (err != 0)
                {
                    LOG_ERR("list_hw_addr_del failed\n");
                }
                goto out;
            }
        }
    }
    else
    {
        /* 遍历组播mac地址链表 */
        list_for_each_entry(ha, &netdev->mc.list, list)
        {
            /* 检查该组播地址链表中是否存在此mac，且此mac地址标志为组播 */
            if ((!memcmp(ha->addr, addr, netdev->addr_len)) \
                && (ha->type == NETDEV_HW_ADDR_T_MULTICAST))
            {
                /* 从组播地址链表中删除此mac */
                err = list_hw_addr_del(&netdev->mc, ha);
                if (err != 0)
                {
                    LOG_ERR("list_hw_addr_del failed\n");
                }
                goto out;
            }
        }
    }

out:
    /* 给net_device结构体释放锁 */
    netif_addr_unlock_bh(netdev);

    return err;
}

#ifdef MAC_CONFIG_DEBUG
int32_t zxdh_pf_dump_all_mac(struct zxdh_en_device *en_dev)
{
    MAC_VPORT_INFO *unicast_mac_arry   = NULL;
    MAC_VPORT_INFO *multicast_mac_arry = NULL;
    uint32_t current_unicast_num   = 0;
    uint32_t current_multicast_num = 0;
    int32_t err = 1;
    int32_t i   = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* 开辟单播数组和组播数组*/
    unicast_mac_arry = (MAC_VPORT_INFO *)kzalloc(sizeof(MAC_VPORT_INFO)*UNICAST_MAX_NUM, GFP_KERNEL);
    if (unicast_mac_arry == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc unicast_mac_arry failed \n");
        return err;
    }

    multicast_mac_arry = (MAC_VPORT_INFO *)kzalloc(sizeof(MAC_VPORT_INFO)*MULTICAST_MAX_NUM, GFP_KERNEL);
    if (multicast_mac_arry == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc multicast_mac_arry failed \n");
        goto out1;
    }

    /* 从NP中dump所有单播mac地址*/
    err = dpp_unicast_mac_dump(&pf_info, unicast_mac_arry, &current_unicast_num);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_dump failed, ret:%d\n", err);
        goto out2;
    }

    /* 从NP中dump所有组播mac地址*/
    err = dpp_multicast_mac_dump(&pf_info, multicast_mac_arry, &current_multicast_num);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_multicast_mac_dump failed\n");
        goto out2;
    }

    for(i = 0; i < current_unicast_num; ++i)
    {
        LOG_INFO_DEV(en_dev->parent, "unicast_mac_arry[%d].vport is %#x\n", i, unicast_mac_arry[i].vport);
        LOG_INFO_DEV(en_dev->parent, "unicast_mac_arry[%d].mac is %pM\n", i, unicast_mac_arry[i].addr);
    }
    for(i = 0; i < current_multicast_num; ++i)
    {
        LOG_INFO_DEV(en_dev->parent, "multicast_mac_arry[%d].vport is %#x\n", i,  multicast_mac_arry[i].vport);
        LOG_INFO_DEV(en_dev->parent, "multicast_mac_arry[%d].mac is %pM\n", i,  multicast_mac_arry[i].addr);
    }

out2:
    if (multicast_mac_arry != NULL)
    {
        kfree(multicast_mac_arry);
    }

out1:
    if (unicast_mac_arry != NULL)
    {
        kfree(unicast_mac_arry);
    }

    return err;
}
#endif /* MAC_CONFIG_DEBUG */

int32_t unicast_mac_add(struct zxdh_en_device *en_dev, struct net_device *dev, \
                        const uint8_t* addr, uint16_t flags)
{
    struct netdev_hw_addr *ha = NULL;
    int32_t err = 0;
    MAC_VPORT_INFO *p_mac_arr = NULL;
    uint32_t p_mac_num = 0;
    uint16_t current_vport = 0;
    uint16_t sriov_vlan_tpid = 0;
    uint16_t sriov_vlan_id = 0;
    DPP_PF_INFO_T pf_info = {0};
#ifdef HAVE_RB_TREE_OPS
    struct rb_node **ins_point = NULL, *parent = NULL;
#endif

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* 判断单个设备所配置mac地址数量是否超过32上限 */
    if (en_dev->curr_unicast_num >= DEV_UNICAST_MAX_NUM-1)
    {
        LOG_ERR_DEV(en_dev->parent, "curr_unicast_num is beyond maximum\n");
        return -ENOSPC;
    }

    /* 遍历单播地址链表，判断是否此设备存在此单播mac */
#ifdef HAVE_RB_TREE_OPS
    if (is_this_mac_exist(dev, addr, &ha, &ins_point, &parent))
#else
    if (is_this_mac_exist(dev, addr, &ha))
#endif
    {
        LOG_DEBUG_DEV(en_dev->parent, "Mac already exists\n");
        if (!(flags & NLM_F_EXCL))
        {
            return 0;
        }
        return -EEXIST;
    }

    /* 如果待配置mac和本机mac相同，则不配置到NP中， 只将此mac添加到单播地址链表中 */
    if (!memcmp(addr, dev->dev_addr, dev->addr_len))
    {
        goto out;
    }

    /* 将此mac地址配置到np中 */
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        /* dump整个转发域已经配置的单播mac数量 */
        err = dpp_unicast_mac_dump(&pf_info, p_mac_arr, &p_mac_num);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_dump failed, ret:%d\n", err);
            return -1;
        }
        LOG_DEBUG_DEV(en_dev->parent, "p_mac_num is %d\n", p_mac_num);

        /* 判断整个转发域配置的单播mac数量是否超过上限 */
        if (p_mac_num >= UNICAST_MAX_NUM)
        {
            LOG_ERR_DEV(en_dev->parent, "curr_all_unicast_num is beyond maximum\n");
            return -ENOSPC;
        }

        /* 遍历整个转发域，判断此转发域是否存在此单播mac地址 */
        err = dpp_unicast_mac_search(&pf_info, addr, sriov_vlan_tpid, sriov_vlan_id, &current_vport);
        if (err == 0)
        {
            LOG_DEBUG_DEV(en_dev->parent, "Mac already exists\n");
            return -EEXIST;
        }
        else if (err != DPP_HASH_RC_SRH_FAIL)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_search failed, ret:%d\n", err);
            return -1;
        }

        err = dpp_add_mac(&pf_info, addr, sriov_vlan_tpid, sriov_vlan_id);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_add_mac failed, ret:%d\n", err);
            return -1;
        }
    }
    else
    {
        err = zxdh_vf_dpp_add_mac(en_dev, addr, FILTER_MAC);
        if (err != 0)
        {
            if (err == ZXDH_REPS_BEYOND_MAC)
            {
                LOG_ERR_DEV(en_dev->parent, "curr_all_unicast_num is beyond maximum\n");
                return -ENOSPC;
            }
            else if (err == ZXDH_REPS_EXIST_MAC)
            {
                LOG_DEBUG_DEV(en_dev->parent, "Mac already exists\n");
                return -EEXIST;
            }
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_dpp_add_mac failed, ret:%d\n", err);
            return -1;
        }
    }

out:
    /* 将此单播mac地址添加到地址链表中 */
#ifdef HAVE_RB_TREE_OPS
    err = zxdh_dev_list_addr_add(dev, addr, parent, ins_point);
#else
    err = zxdh_dev_list_addr_add(dev, addr, NULL, NULL);
#endif
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dev_list_addr_add failed, ret:%d\n", err);
        return -1;
    }
    en_dev->curr_unicast_num++;
    LOG_DEBUG_DEV(en_dev->parent, "curr_unicast_num is %d\n", en_dev->curr_unicast_num);
    return err;
}

bool is_ipv6_multicast_mac(const uint8_t *mac)
{
    return ((mac[0] == 0x33) && (mac[1] == 0x33) && (mac[2] == 0xff));
}

bool ipv6_mac_refcount_get(struct zxdh_en_device *en_dev, const uint8_t *ip6mac, int32_t *ipv6_mac_refconut)
{
    uint32_t mac_hash_val = 0;
    struct zxdh_ipv6_mac_tbl *ip6mac_tbl = en_dev->ops->get_ip6mac_tbl(en_dev->parent);
    struct zxdh_ipv6_mac_entry *ce = NULL;
    struct zxdh_ipv6_mac_entry *cte = NULL;

    if (!ip6mac_tbl)
    {
        LOG_ERR_DEV(en_dev->parent, "ip6mac_tbl is NULL\n");
        return -ENXIO;
    }

    mac_hash_val = mac_hash(ip6mac_tbl, ip6mac);

    mutex_lock(&ip6mac_tbl->mlock);

    list_for_each_entry(cte, &ip6mac_tbl->hash_list[mac_hash_val], list)
    {
        if (memcmp(cte->ipv6_mac, ip6mac, ETH_ALEN) == 0)
        {   /* MAC已经存在 */
            ce = cte;
            *ipv6_mac_refconut = refcount_read(&ce->refcnt);
            mutex_unlock(&ip6mac_tbl->mlock);
            return true;
        }
    }
    mutex_unlock(&ip6mac_tbl->mlock);
    LOG_INFO_DEV(en_dev->parent, "ipv6_mac_refcount_get end\n");
    return false;
}

int32_t multicast_mac_add_operate(struct zxdh_en_device *en_dev, const uint8_t *addr, uint16_t flags, \
                                  struct netdev_hw_addr *ha)
{
    int32_t ipv6_mac_refconut = 0;

    if (!ipv6_mac_refcount_get(en_dev, addr, &ipv6_mac_refconut))
    {
        goto exist_flag;
    }

    if (ipv6_mac_refconut == ha->refcount)
    {
        ++ha->refcount;
        LOG_INFO_DEV(en_dev->parent, "%s ipv6_mac_refconut == ha->refcount\n", __func__);
        return 0;
    }

    if (ipv6_mac_refconut < ha->refcount)
    {
        LOG_INFO_DEV(en_dev->parent, "%s ipv6_mac_refconut < ha->refcount\n", __func__);
        goto exist_flag;
    }
    LOG_ERR_DEV(en_dev->parent, "ipv6_mac_refconut[%d] < ha->refcount[%d]\n", ipv6_mac_refconut, ha->refcount);
    return -1;

exist_flag:
    if (!(flags & NLM_F_EXCL))
    {
        return 0;
    }
    return -EEXIST;
}

int32_t multicast_mac_add(struct zxdh_en_device *en_dev, struct net_device *dev, \
                          const uint8_t *addr, uint16_t flags)
{
    struct netdev_hw_addr *ha = NULL;
    int64_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
#ifdef HAVE_RB_TREE_OPS
    struct rb_node **ins_point = NULL, *parent = NULL;
#endif

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (en_dev->curr_multicast_num >= DEV_MULTICAST_MAX_NUM)
    {
        LOG_ERR_DEV(en_dev->parent, "curr_multicast_num is beyond maximum\n");
        return -ENOSPC;
    }

    /* 遍历组播地址链表，判断是否存在此mac */
#ifdef HAVE_RB_TREE_OPS
    if (is_this_mac_exist(dev, addr, &ha, &ins_point, &parent))
#else
    if (is_this_mac_exist(dev, addr, &ha))
#endif
    {
        if (!is_ipv6_multicast_mac(addr))/* 非ipv6组播mac地址 */
        {
            LOG_DEBUG_DEV(en_dev->parent, "Mac already exists\n");
            if (!(flags & NLM_F_EXCL))
            {
                return 0;
            }
            return -EEXIST;
        }

        if (ha == NULL)
        {
            LOG_ERR_DEV(en_dev->parent, "ha is NULL");
            return -1;
        }
        /* ipv6组播mac地址 */
        return multicast_mac_add_operate(en_dev, addr, flags, ha);
    }

    /* 将此组播mac地址配置到np中 */
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        err = dpp_multi_mac_add_member(&pf_info, addr);
        if (err != 0)
        {
            if (err == DPP_RC_TBL_IS_FULL)/*判断整个转发域已配置组播mac数量是否超过上限*/
            {
                LOG_ERR_DEV(en_dev->parent, "multicast mac beyond all mac num\n");
                return -ENOSPC;
            }
            LOG_ERR_DEV(en_dev->parent, "dpp_multi_mac_add_member failed\n");
            return -1;
        }
    }
    else
    {
        err = zxdh_vf_dpp_add_mac(en_dev, addr, FILTER_MAC);
        if (err != 0)
        {
            if (err == ZXDH_REPS_BEYOND_MAC) /*判断整个转发域已配置组播mac数量超过上限*/
            {
                LOG_ERR_DEV(en_dev->parent, "multicast mac is beyond all mac num\n");
                return -ENOSPC;
            }
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_dpp_add_mac failed, ret:%lld\n", err);
            return -1;
        }
    }

    /* 将此组播mac地址添加到地址链表中 */
#ifdef HAVE_RB_TREE_OPS
    err = zxdh_dev_list_addr_add(dev, addr, parent, ins_point);
#else
    err = zxdh_dev_list_addr_add(dev, addr, NULL, NULL);
#endif
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dev_list_addr_add failed, ret:%lld\n", err);
        return -1;
    }
    en_dev->curr_multicast_num++;
    LOG_DEBUG_DEV(en_dev->parent, "curr_multicast_num is %d\n", en_dev->curr_multicast_num);
    return 0;
}

int32_t unicast_mac_del(struct zxdh_en_device *en_dev, struct net_device *dev, const uint8_t* addr)
{
    struct netdev_hw_addr *ha = NULL;
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
    uint16_t sriov_vlan_tpid = 0;
    uint16_t sriov_vlan_id = 0;
#ifdef HAVE_RB_TREE_OPS
    struct rb_node **ins_point = NULL, *parent = NULL;
#endif

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* 判断目前所配置mac地址数量是否小于0 */
    if (en_dev->curr_unicast_num <= 0)
    {
        LOG_ERR_DEV(en_dev->parent, "curr_unicast_num is less than 0\n");
        return -ENOENT;
    }

    /* 遍历单播地址链表，判断是否存在此mac */
#ifdef HAVE_RB_TREE_OPS
    if (!is_this_mac_exist(dev, addr, &ha, &ins_point, &parent))
#else
    if (!is_this_mac_exist(dev, addr, &ha))
#endif
    {
        LOG_DEBUG_DEV(en_dev->parent, "Mac is not exists\n");
        return -ENOENT;
    }

    /* 如果待删除mac和本机mac相同，则不从NP中删除，只从链表中删除 */
    if (!memcmp(addr, dev->dev_addr, dev->addr_len))
    {
       goto out;
    }

    /* 从np中删除此单播mac */
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        /* 此设备为PF */
        err = dpp_del_mac(&pf_info, addr, sriov_vlan_tpid, sriov_vlan_id);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_del_mac failed, ret:%d\n", err);
            return -1;
        }
        LOG_DEBUG_DEV(en_dev->parent, "dpp_del_mac succeed\n");
    }
    else
    {
        /* 此设备为VF */
        err = zxdh_vf_dpp_del_mac(en_dev, addr, FILTER_MAC, true);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_dpp_del_mac failed, ret:%d\n", err);
            return -1;
        }
        LOG_DEBUG_DEV(en_dev->parent, "zxdh_vf_dpp_del_mac succeed\n");
    }

out:
    /* 从链表中删除单播mac */
    err = zxdh_dev_list_addr_del(dev, addr);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dev_list_addr_del failed, ret:%d\n", err);
        return -1;
    }
    en_dev->curr_unicast_num--;
    LOG_DEBUG_DEV(en_dev->parent, "curr_unicast_num is %d\n", en_dev->curr_unicast_num);
    return err;
}

int32_t multicast_mac_del_operate(struct zxdh_en_device *en_dev, const uint8_t* addr, struct netdev_hw_addr *ha)
{
    int32_t ipv6_mac_refconut = 0;

    if (!ipv6_mac_refcount_get(en_dev, addr, &ipv6_mac_refconut))
    {
        return 0;
    }

    if (ipv6_mac_refconut == ha->refcount)
    {
        LOG_INFO_DEV(en_dev->parent, "%s ipv6_mac_refconut == ha->refcount", __func__);
        return -1;
    }

    if ((ipv6_mac_refconut < ha->refcount) && (ipv6_mac_refconut > 0))
    {
        --ha->refcount;
        LOG_INFO_DEV(en_dev->parent, "%s ipv6_mac_refconut < ha->refcount", __func__);
        return -1;
    }
    LOG_ERR_DEV(en_dev->parent, "ipv6_mac_refconut[%d] < ha->refcount[%d]", ipv6_mac_refconut, ha->refcount);
    return -1;
}

int32_t multicast_mac_del(struct zxdh_en_device *en_dev, struct net_device *dev, const uint8_t* addr)
{
    struct netdev_hw_addr *ha = NULL;
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
#ifdef HAVE_RB_TREE_OPS
    struct rb_node **ins_point = NULL, *parent = NULL;
#endif

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* 遍历组播地址链表，判断是否存在此组播mac，如果不存在，则返回报错 */
#ifdef HAVE_RB_TREE_OPS
    if (!is_this_mac_exist(dev, addr, &ha, &ins_point, &parent))
#else
    if (!is_this_mac_exist(dev, addr, &ha))
#endif
    {
        LOG_DEBUG_DEV(en_dev->parent, "Mac is not exists\n");
        return -ENOENT;
    }

    if (ha == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "ha is NULL");
        return -1;
    }

    if (is_ipv6_multicast_mac(addr))/* ipv6组播mac地址*/
    {
        err = multicast_mac_del_operate(en_dev, addr, ha);
        if (err != 0)
        {
            LOG_DEBUG_DEV(en_dev->parent, "Mac is not permitted del\n");
            return 0;
        }
    }

    /* 从np中删除此组播mac */
    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        /* 此设备为PF */
        err = dpp_multi_mac_del_member(&pf_info, addr);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_multi_mac_del_member failed, ret:%d\n", err);
            return -1;
        }
        LOG_DEBUG_DEV(en_dev->parent, "dpp_multi_mac_del_member succeed\n");
    }
    else
    {
        /* 此设备为VF */
        err = zxdh_vf_dpp_del_mac(en_dev, addr, FILTER_MAC, true);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_dpp_del_mac failed, ret:%d\n", err);
            return -1;
        }
    }

    /* 从链表中删除组播mac */
    err = zxdh_dev_list_addr_del(dev, addr);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_dev_list_addr_del failed, ret:%d\n", err);
        return -1;
    }
    en_dev->curr_multicast_num--;
    LOG_DEBUG_DEV(en_dev->parent, "curr_multicast_num is %d\n", en_dev->curr_multicast_num);
    return err;
}

static unsigned int mac_hash(struct zxdh_ipv6_mac_tbl *mac_tbl, const uint8_t *mac_addr)
{
    unsigned int mact_size_half = mac_tbl->ip6mact_size / 2;
    uint32_t mac_part1 = (mac_addr[0] << 24) | (mac_addr[1] << 16) | (mac_addr[2] << 8) | mac_addr[3];
    uint32_t mac_part2 = (mac_addr[4] << 8) | mac_addr[5];

    uint32_t xor = mac_part1 ^ mac_part2;

    return (jhash_1word(xor, 0) % mact_size_half);
}

int32_t zxdh_ip6mac_to_np(struct zxdh_en_device *en_dev, struct zxdh_ipv6_mac_tbl *ip6mac_tbl, const uint8_t *ip6mac, uint8_t action)
{
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    switch (action)
    {
        case ADD_IP6MAC:
        {
            if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
            { /* PF流程 */
                /* 将此组播mac地址配置到np中 */
                err = dpp_multi_mac_add_member(&pf_info, ip6mac);
                if (err != 0)
                {
                    LOG_ERR_DEV(en_dev->parent, "dpp_multi_mac_add_member failed, err:%d\n", err);
                }
            }
            else
            { /* VF流程*/
                err = zxdh_vf_dpp_add_ipv6_mac(en_dev, ip6mac);
                if (err != 0)
                {
                    LOG_ERR_DEV(en_dev->parent, "zxdh_vf_dpp_add_ipv6_mac failed, err:%d\n", err);
                }
            }
            break;
        }
        case DEL_IP6MAC:
        {
            if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
            {/* PF流程 */
                err = dpp_multi_mac_del_member(&pf_info, ip6mac);
                if (err != 0)
                {
                    LOG_ERR_DEV(en_dev->parent, "dpp_multi_mac_del_member failed, err:%d\n", err);
                }
            }
            else
            {/* VF流程 */
                err = zxdh_vf_dpp_del_ipv6_mac(en_dev, ip6mac);
                if (err != 0)
                {
                    LOG_ERR_DEV(en_dev->parent, "zxdh_vf_dpp_del_ipv6_mac failed, err:%d\n", err);
                }
            }
            break;
        }
    }
    return err;
}

int32_t zxdh_ip4mac_to_np(struct zxdh_en_device *en_dev, const uint8_t *ip4mac, uint8_t action)
{
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    switch (action)
    {
        case NETDEV_UP:
        {
            if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
            { /* PF流程 */
                /* 将此组播mac地址配置到np中 */
                err = dpp_multi_mac_add_member(&pf_info, ip4mac);
                if (err != 0)
                {
                    LOG_ERR_DEV(en_dev->parent, "pf config ipv4 mac failed, err:%d\n", err);
                }
            }
            else
            { /* VF流程*/
                err = zxdh_vf_dpp_add_ipv6_mac(en_dev, ip4mac);
                if (err != 0)
                {
                    LOG_ERR_DEV(en_dev->parent, "vf config ipv4 mac failed, err:%d\n", err);
                }
            }
            break;
        }
        case NETDEV_DOWN:
        {
            if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
            {/* PF流程 */
                err = dpp_multi_mac_del_member(&pf_info, ip4mac);
                if (err != 0)
                {
                    LOG_ERR_DEV(en_dev->parent, "vf del ipv4 mac failed, err:%d\n", err);
                }
            }
            else
            {/* VF流程 */
                err = zxdh_vf_dpp_del_ipv6_mac(en_dev, ip4mac);
                if (err != 0)
                {
                    LOG_ERR_DEV(en_dev->parent, "vf del ipv4 mac failed, err:%d\n", err);
                }
            }
            break;
        }
    }
    return err;
}

int32_t zxdh_ip6mac_add(struct zxdh_en_device *en_dev, const uint32_t *addr6, const uint8_t *ip6mac)
{
    int32_t err = 0;
    unsigned int mac_hash_val;
    struct zxdh_ipv6_mac_tbl *ip6mac_tbl = en_dev->ops->get_ip6mac_tbl(en_dev->parent);
    struct zxdh_ipv6_mac_entry *ce, *cte;

    if (!ip6mac_tbl)
    {
        LOG_ERR_DEV(en_dev->parent, "ip6mac_tbl is NULL\n");
        return -ENXIO;
    }

    if (en_dev->curr_multicast_num >= DEV_MULTICAST_MAX_NUM)
    {
        LOG_ERR_DEV(en_dev->parent, "curr_multicast_num is beyond maximum\n");
        return -ENOSPC;
    }

#ifdef CGS_V5_693
    /* 3.10 内核中，为避免死锁，先执行 NP 操作再获取锁更新本地表 */
    err = zxdh_ip6mac_to_np(en_dev, ip6mac_tbl, ip6mac, ADD_IP6MAC);
    if (err != 0)
    {
        return err;
    }
    mutex_lock(&ip6mac_tbl->mlock);
    /* 重新检查 MAC 是否已存在（在解锁期间可能被其他线程添加） */
    mac_hash_val = mac_hash(ip6mac_tbl, ip6mac);
    list_for_each_entry(cte, &ip6mac_tbl->hash_list[mac_hash_val], list)
    {
        if (memcmp(cte->ipv6_mac, ip6mac, ETH_ALEN) == 0)
        {
            ce = cte;
            refcount_inc(&ce->refcnt);
            DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "Eth:%s, Increase Multicast MAC Address(%pM) refcnt:%d (race)\n",
                en_dev->netdev->name, ip6mac, refcount_read(&ce->refcnt));
            mutex_unlock(&ip6mac_tbl->mlock);
            return 0;
        }
    }
#else
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        mutex_lock(&ip6mac_tbl->mlock);
        err = zxdh_ip6mac_to_np(en_dev, ip6mac_tbl, ip6mac, ADD_IP6MAC);
        if (err != 0)
        {
            mutex_unlock(&ip6mac_tbl->mlock);
            return err;
        }
    }else {
        err = zxdh_ip6mac_to_np(en_dev, ip6mac_tbl, ip6mac, ADD_IP6MAC);
        if (err != 0)
        {
            return err;
        }
        mutex_lock(&ip6mac_tbl->mlock);
    }
#endif

    mac_hash_val = mac_hash(ip6mac_tbl, ip6mac);
    //如果没有报错，则说明MAC已经存在或成功存入NP
    list_for_each_entry(cte, &ip6mac_tbl->hash_list[mac_hash_val], list)
    {
        if (memcmp(cte->ipv6_mac, ip6mac, ETH_ALEN) == 0)
        {//MAC已经存在
            ce = cte;
            refcount_inc(&ce->refcnt);
            DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "Eth:%s, Increase Multicast MAC Address(%pM) refcnt:%d\n",
                en_dev->netdev->name, ip6mac, refcount_read(&ce->refcnt));
            mutex_unlock(&ip6mac_tbl->mlock);
            return 0;
        }
    }

    /* 成功新增 MAC 至 NP，需要更新 ip6mac_tbl */
    if (list_empty(&ip6mac_tbl->ip6mac_free_head))
    {
#ifdef CGS_V5_693
        /* 3.10 内核中，表已满但 NP 操作已成功，只需解锁并返回错误 */
        mutex_unlock(&ip6mac_tbl->mlock);
        LOG_ERR_DEV(en_dev->parent, "ip6mac_tbl overflow, can't add (NP already added)\n");
        return -ENOMEM;
#else
        if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
        {
            err = zxdh_ip6mac_to_np(en_dev, ip6mac_tbl, ip6mac, DEL_IP6MAC);
            mutex_unlock(&ip6mac_tbl->mlock);
        }
        else
        {
            mutex_unlock(&ip6mac_tbl->mlock);
            err = zxdh_ip6mac_to_np(en_dev, ip6mac_tbl, ip6mac, DEL_IP6MAC);
        }
#endif
        LOG_ERR_DEV(en_dev->parent, "ip6mac_tbl overflow, can't add; del mac from NP, ret:%d\n", err);
        return -ENOMEM;
    }
    ce = list_first_entry(&ip6mac_tbl->ip6mac_free_head, struct zxdh_ipv6_mac_entry, list);
    list_del(&ce->list);
    INIT_LIST_HEAD(&ce->list);
    spin_lock_init(&ce->lock);
    refcount_set(&ce->refcnt, 0);
    list_add_tail(&ce->list, &ip6mac_tbl->hash_list[mac_hash_val]);
    memcpy(ce->ipv6_mac, ip6mac, ETH_ALEN);
    refcount_set(&ce->refcnt, 1);
    mutex_unlock(&ip6mac_tbl->mlock);
    en_dev->curr_multicast_num++;
    DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "curr_multicast_num is %d\n", en_dev->curr_multicast_num);
    DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "Eth:%s, Add New Multicast MAC Address: %pM, refcnt:%d\n",
        en_dev->netdev->name, ip6mac, refcount_read(&ce->refcnt));

    return 0;
}

int32_t zxdh_ip4mac_add(struct zxdh_en_device *en_dev, const uint8_t *ip4mac, uint8_t action)
{
    struct net_device *netdev = en_dev->netdev;
    struct netdev_hw_addr *entry = NULL;
    int32_t err = 0;

    if (en_dev->curr_multicast_num >= DEV_MULTICAST_MAX_NUM)
    {
        LOG_ERR_DEV(en_dev->parent, "curr_multicast_num is beyond maximum\n");
        return -ENOSPC;
    }

    /* 遍历组播mac地址链表，判断此mac地址是否已经配置 */
    list_for_each_entry(entry, &netdev->mc.list, list)
    {
        if ((!memcmp(entry->addr, ip4mac, ETH_ALEN)) \
                && (entry->type == NETDEV_HW_ADDR_T_MULTICAST))
        {
            LOG_DEBUG_DEV(en_dev->parent, "entry->refcount is %d\n", entry->refcount);
            if (entry->refcount > 1)
            {
                LOG_INFO_DEV(en_dev->parent, "MAC:%pM already config to nic\n", ip4mac);
                return NOTIFY_OK;
            }
        }
    }

    err = zxdh_ip4mac_to_np(en_dev, ip4mac, action);
    if (err != 0)
    {
        return err;
    }

    /* 如果没有报错，则说明MAC已经存在或成功存入NP */
    en_dev->curr_multicast_num++;
    LOG_DEBUG_DEV(en_dev->parent, "curr_multicast_num is %d\n", en_dev->curr_multicast_num);
    return 0;
}

int32_t zxdh_vlan_mac_add(struct zxdh_en_device *en_dev, uint16_t vlan_id, const uint8_t *vlan_mac)
{
    struct net_device *netdev = en_dev->netdev;
    DPP_PF_INFO_T pf_info = {0};
    uint16_t sriov_vlan_tpid = 0;
    uint16_t sriov_vlan_id = 0;
    int32_t err = 0;

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* 判断目前所配128播mac地址数量是否超过128个上限 */
    if (en_dev->curr_unicast_num >= DEV_UNICAST_MAX_NUM)
    {
        LOG_ERR("[%s]curr_unicast_num[%d] is beyond maximum\n", netdev->name, en_dev->curr_unicast_num);
        return -ENOSPC;
    }

    /* 将该mac地址配置到np中 */
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        err = dpp_add_mac(&pf_info, vlan_mac, sriov_vlan_tpid, sriov_vlan_id);
        if(err)
        {
            LOG_ERR("%s dpp_add_mac[%pM] failed, err:%d\n", netdev->name, vlan_mac, err);
            return err;
        }
    }
    else
    {
        err = zxdh_vf_dpp_add_mac(en_dev, vlan_mac, FILTER_MAC);
        if(err)
        {
            LOG_ERR("%s zxdh_vf_dpp_add_mac[%pM] failed, err:%d\n", netdev->name, vlan_mac, err);
            return err;
        }
    }

    /* 将mac地址存入驱动维护的Vlan和mac的映射表中*/
    err = list_vlan_mac_create(&en_dev->vuc, vlan_id, vlan_mac);
    if (err)
    {
        LOG_ERR("list_vlan_mac_create faied[%d], [%pM]\n", vlan_id, vlan_mac);
        return err;
    }
    en_dev->curr_unicast_num++;
    LOG_INFO("%s curr_unicast_num is %d\n", netdev->name, en_dev->curr_unicast_num);
    return 0;
}


/* Started by AICoder, pid:l27ba73757rbe3e145c6096b701cf73c85789c7c */
/* Work queue handler for IPv6 MAC deletion using dynamic work items */
void zxdh_ip6mac_del_work_handler(struct work_struct *work)
{
    struct zxdh_ip6mac_work_item *work_item = container_of(work, struct zxdh_ip6mac_work_item, work);
    struct zxdh_en_device *en_dev = work_item->en_dev;
    zxdh_ip6mac_del(en_dev, work_item->data.addr6, work_item->data.ip6mac);
    kfree(work_item);
}

/* Safe version of zxdh_ip6mac_del that handles atomic context with dynamic work items */
int32_t zxdh_ip6mac_del_safe(struct zxdh_en_device *en_dev, const uint32_t *addr6, const uint8_t *ip6mac)
{
    struct zxdh_ip6mac_work_item *work_item;
    struct zxdh_en_priv *en_priv = container_of(en_dev, struct zxdh_en_priv, edev);
    /* Allocate work item dynamically - each call gets its own work item with embedded data */
    work_item = kmalloc(sizeof(struct zxdh_ip6mac_work_item), GFP_ATOMIC);
    if (!work_item) {
        return -ENOMEM;
    }
    INIT_WORK(&work_item->work, zxdh_ip6mac_del_work_handler);
    work_item->en_dev = en_dev;
    zte_memcpy_s(work_item->data.addr6, addr6, sizeof(work_item->data.addr6));
    zte_memcpy_s(work_item->data.ip6mac, ip6mac, sizeof(work_item->data.ip6mac));

    if (!queue_work(en_priv->events->wq, &work_item->work)) {
        kfree(work_item);
        return -EBUSY;
    }

    return 0;
}

/* Work queue handler for IPv6 MAC addition using dynamic work items */
void zxdh_ip6mac_add_work_handler(struct work_struct *work)
{
    struct zxdh_ip6mac_work_item *work_item = container_of(work, struct zxdh_ip6mac_work_item, work);
    struct zxdh_en_device *en_dev = work_item->en_dev;
    zxdh_ip6mac_add(en_dev, work_item->data.addr6, work_item->data.ip6mac);
    kfree(work_item);
}

/* Safe version of zxdh_ip6mac_add that handles atomic context with dynamic work items */
int32_t zxdh_ip6mac_add_safe(struct zxdh_en_device *en_dev, const uint32_t *addr6, const uint8_t *ip6mac)
{
    struct zxdh_ip6mac_work_item *work_item;
    struct zxdh_en_priv *en_priv = container_of(en_dev, struct zxdh_en_priv, edev);
    work_item = kmalloc(sizeof(struct zxdh_ip6mac_work_item), GFP_ATOMIC);
    if (!work_item) {
        return -ENOMEM;
    }
    INIT_WORK(&work_item->work, zxdh_ip6mac_add_work_handler);
    work_item->en_dev = en_dev;
    zte_memcpy_s(work_item->data.addr6, addr6, sizeof(work_item->data.addr6));
    zte_memcpy_s(work_item->data.ip6mac, ip6mac, sizeof(work_item->data.ip6mac));

    if (!queue_work(en_priv->events->wq, &work_item->work)) {
        kfree(work_item);
        return -EBUSY;
    }

    return 0;
}
/* Ended by AICoder, pid:l27ba73757rbe3e145c6096b701cf73c85789c7c */

int32_t zxdh_ip6mac_del(struct zxdh_en_device *en_dev, const uint32_t *addr6, const uint8_t *ip6mac)
{
    int32_t err = 0;
    struct zxdh_ipv6_mac_tbl *ip6mac_tbl = en_dev->ops->get_ip6mac_tbl(en_dev->parent);
    struct zxdh_ipv6_mac_entry *ce, *cte;
    unsigned int mac_hash_val;
    int32_t refcnt = 0;
    DPP_PF_INFO_T pf_info = {0};

    if (!ip6mac_tbl)
    {
        LOG_ERR_DEV(en_dev->parent, "ip6mac_tbl is NULL");
        return -ENXIO;
    }

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    mac_hash_val = mac_hash(ip6mac_tbl, ip6mac);

#ifdef CGS_V5_693
    /* 3.10 内核中，为避免死锁，先获取锁查找，再解锁执行 NP 操作 */
    mutex_lock(&ip6mac_tbl->mlock);
    list_for_each_entry(cte, &ip6mac_tbl->hash_list[mac_hash_val], list)
    {
        if (memcmp(cte->ipv6_mac, ip6mac, ETH_ALEN) == 0)
        {
            ce = cte;
            goto found;
        }
    }
    /* MAC 不存在，解锁并执行 NP 删除 */
    mutex_unlock(&ip6mac_tbl->mlock);
    err = zxdh_ip6mac_to_np(en_dev, ip6mac_tbl, ip6mac, DEL_IP6MAC);
    return err;

found:
    spin_lock_bh(&ce->lock);
    if (!refcount_dec_and_test(&ce->refcnt))
    {
        DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "Eth:%s, Decrease Multicast MAC Address(%pM) refcnt:%d\n",
            en_dev->netdev->name, ip6mac, refcount_read(&ce->refcnt));
        spin_unlock_bh(&ce->lock);
        mutex_unlock(&ip6mac_tbl->mlock);
        return err;
    }
    /* 如果引用计数减到 0 */
    list_del(&ce->list);
    INIT_LIST_HEAD(&ce->list);
    list_add_tail(&ce->list, &ip6mac_tbl->ip6mac_free_head);
    refcnt = refcount_read(&ce->refcnt);
    spin_unlock_bh(&ce->lock);
    /* 先解锁再执行 NP 操作 */
    mutex_unlock(&ip6mac_tbl->mlock);
    err = zxdh_ip6mac_to_np(en_dev, ip6mac_tbl, ip6mac, DEL_IP6MAC);
#else
    mutex_lock(&ip6mac_tbl->mlock);
    list_for_each_entry(cte, &ip6mac_tbl->hash_list[mac_hash_val], list)
    {
        if (memcmp(cte->ipv6_mac, ip6mac, ETH_ALEN) == 0)
        {//MAC 存在
            ce = cte;
            goto found;
        }
    }
    DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "Don't Found Multicast MAC Address: %pM in Hash List\n", ip6mac);

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        err = zxdh_ip6mac_to_np(en_dev, ip6mac_tbl, ip6mac, DEL_IP6MAC);
        mutex_unlock(&ip6mac_tbl->mlock);
    }
    else {
        mutex_unlock(&ip6mac_tbl->mlock);
        err = zxdh_ip6mac_to_np(en_dev, ip6mac_tbl, ip6mac, DEL_IP6MAC);
    }
    return err;

found:
    spin_lock_bh(&ce->lock);
    if (!refcount_dec_and_test(&ce->refcnt))
    {
        DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "Eth:%s, Decrease Multicast MAC Address(%pM) refcnt:%d\n",
            en_dev->netdev->name, ip6mac, refcount_read(&ce->refcnt));
        spin_unlock_bh(&ce->lock);
        mutex_unlock(&ip6mac_tbl->mlock);
        return err;
    }
    //如果引用计数减到 0
    list_del(&ce->list);
    INIT_LIST_HEAD(&ce->list);
    list_add_tail(&ce->list, &ip6mac_tbl->ip6mac_free_head);
    refcnt = refcount_read(&ce->refcnt);
    spin_unlock_bh(&ce->lock);
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {   //PF 设备需要带锁下表
        err = zxdh_ip6mac_to_np(en_dev, ip6mac_tbl, ip6mac, DEL_IP6MAC);
        mutex_unlock(&ip6mac_tbl->mlock);
    }
    else
    {   //VF 设备需要先释放锁再发消息下表，因为 PF 侧会加锁
        mutex_unlock(&ip6mac_tbl->mlock);
        err = zxdh_ip6mac_to_np(en_dev, ip6mac_tbl, ip6mac, DEL_IP6MAC);
    }
#endif
    en_dev->curr_multicast_num--;
    DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "curr_multicast_num is %d\n", en_dev->curr_multicast_num);
    DH_LOG_DEBUG_DEV(MODULE_PF, en_dev->parent, "Eth:%s, Del Multicast MAC Address: %pM Completely, refcnt:%d, np ret:%d\n",
        en_dev->netdev->name, ip6mac, refcnt, err);
    return err;
}

int32_t zxdh_ip4mac_del(struct zxdh_en_device *en_dev, const uint8_t *ip4mac, uint8_t action)
{
    int32_t err = 0;
    struct netdev_hw_addr *entry = NULL;
    struct net_device *netdev = en_dev->netdev;

    /* 遍历组播mac地址链表，判断此mac地址是否已经删除 */
    list_for_each_entry(entry, &netdev->mc.list, list)
    {
        /* 检查该组播地址链表中是否存在此mac，且此mac地址类型为组播 */
        if ((!memcmp(entry->addr, ip4mac, ETH_ALEN)) \
                && (entry->type == NETDEV_HW_ADDR_T_MULTICAST))
        {
            LOG_DEBUG_DEV(en_dev->parent, "entry->refcount is %d\n", entry->refcount);
            if (entry->refcount > 0)
            {
                LOG_INFO_DEV(en_dev->parent, "MAC:%pM is used by other dev or operation\n", ip4mac);
                return NOTIFY_OK;
            }
        }
    }

    err = zxdh_ip4mac_to_np(en_dev, ip4mac, action);
    if (err != 0)
    {
        return err;
    }

    en_dev->curr_multicast_num--;
    LOG_INFO_DEV(en_dev->parent, "curr_multicast_num is %d\n", en_dev->curr_multicast_num);
    return err;
}

bool zxdh_old_vlan_mac_get(struct zxdh_en_device *en_dev, uint8_t *old_vlan_mac, uint16_t vlan_id)
{
    struct zxdh_vlan_to_mac *vtm_entry = NULL;

    rcu_read_lock();
    /* 找到该Vlan对应的mac地址 */
    list_for_each_entry_rcu(vtm_entry, &en_dev->vuc.list, list)
    {
        if (vtm_entry->vlan_id == vlan_id)
        {
            ether_addr_copy(old_vlan_mac, vtm_entry->addr);
            rcu_read_unlock();
            return true;
        }
    }
    rcu_read_unlock();
    return false; /* 未找到对应的mac地址 */
}

int32_t zxdh_vlan_mac_del(struct zxdh_en_device *en_dev, uint16_t vlan_id, const uint8_t *vlan_mac)
{
    struct net_device *netdev = en_dev->netdev;
    uint16_t sriov_vlan_tpid = 0;
    uint16_t sriov_vlan_id = 0;
    DPP_PF_INFO_T pf_info = {0};
    int32_t err = 0;

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;


    /* 从np中删除mac地址 */
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        err = dpp_del_mac(&pf_info, vlan_mac, sriov_vlan_tpid, sriov_vlan_id);
        if(err)
        {
            LOG_ERR("%s dpp_del_mac[%pM] failed, err:%d\n", netdev->name, vlan_mac, err);
            return err;
        }
    }
    else
    {
        err = zxdh_vf_dpp_del_mac(en_dev, vlan_mac, FILTER_MAC, true);
        if(err)
        {
            LOG_ERR("%s zxdh_vf_dpp_del_mac[%pM] failed, err:%d\n", netdev->name, vlan_mac, err);
            return err;
        }
    }

    list_vlan_mac_del(&en_dev->vuc, vlan_id, vlan_mac); /* 从vlan_to_mac地址链表中删除此mac */

    en_dev->curr_unicast_num--;
    LOG_INFO("%s curr_unicast_num is %d\n", netdev->name, en_dev->curr_unicast_num);
    return err;
}

int32_t zxdh_pf_add_vf_unicast_mac(struct zxdh_en_device *en_dev, struct dhtool_set_vf_mac_msg *msg)
{
    struct  zxdh_vf_item *vf_item  = NULL;
    uint16_t current_vport         = 0;
    int32_t ret                    = 0;
    uint32_t i                     = 0;
    uint16_t sriov_vlan_tpid       = 0;
    uint16_t sriov_vlan_id         = 0;
    uint32_t p_mac_num             = 0;
    uint32_t max_unicast_num       = 0;
    DPP_PF_INFO_T pf_info          = {0};
    ZXDH_L2_FWD_KEY *l2_entry      = NULL ;
    MAC_VPORT_INFO *p_mac_arr      = NULL;

    /* 判断vf是否probe */
    if (!en_dev->ops->get_vf_is_probe(en_dev->parent, msg->mac_config.target_vf))
    {
        LOG_ERR_DEV(en_dev->parent, "vf(%u) is not probed\n", msg->mac_config.target_vf);
        return VF_ERROR;
    }

    /* 获取vf_item */
    vf_item = en_dev->ops->get_vf_item(en_dev->parent, msg->mac_config.target_vf);
    if (IS_ERR_OR_NULL(vf_item))
    {
        LOG_ERR_DEV(en_dev->parent, "get_vf(%u)_item failed\n", msg->mac_config.target_vf);
        return MAC_CONFIG_FAILED;
    }

    mutex_lock(&vf_item->lock);
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = vf_item->vport;

    /* 判断单播mac是否超过vf的mac上限 */
    LOG_DEBUG_DEV(en_dev->parent, "unicast_add_count is %u\n", msg->mac_config.unicast_add_count);
    LOG_DEBUG_DEV(en_dev->parent, "vf_item->vf_mac_info.current_unicast_num is %u\n", vf_item->vf_mac_info.current_unicast_num);

    if ((vf_item->vf_mac_info.current_unicast_num + msg->mac_config.unicast_add_count) > VF_MAX_UNICAST_MAC)
    {
        LOG_ERR_DEV(en_dev->parent, "current mac num beyond 128\n");
        mutex_unlock(&vf_item->lock);
        return UNICAST_MAC_NUM_BEYOND_MAXNUM;
    }

    /* 遍历整个转发域，获取前pf级已经配置单播mac数量 */
    ret = dpp_unicast_mac_dump(&pf_info, p_mac_arr, &p_mac_num);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_dump failed\n");
        mutex_unlock(&vf_item->lock);
        return ret;
    }
    LOG_DEBUG_DEV(en_dev->parent, "p_mac_num is %d\n", p_mac_num);

    /* 获取当前pf级最大单播mac数量 */
    ret = dpp_unicast_mac_max_get(&pf_info, &max_unicast_num);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_max_get failed %u\n", max_unicast_num);
        mutex_unlock(&vf_item->lock);
        return MAC_CONFIG_FAILED;
    }

    if ((p_mac_num + msg->mac_config.unicast_add_count) > max_unicast_num)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_dump failed\n");
        mutex_unlock(&vf_item->lock);
        return UNICAST_MAC_NUM_BEYOND_MAXNUM;
    }

    /* 获取vlan信息 */
    sriov_vlan_tpid = vf_item->vlan_proto;
    sriov_vlan_id = vf_item->vlan;

    pf_info.vport = vf_item->vport;
    /* 判断此单播mac是否存在 */
    for (i = 0; i < msg->mac_config.unicast_add_count; i++)
    {
        ret = dpp_unicast_mac_search(&pf_info, msg->mac_config.unicast_mac_array[i].mac_addr,
                                    sriov_vlan_tpid, sriov_vlan_id, &current_vport);
        if (ret == 0) /* 找到此单播mac */
        {
            if (current_vport == vf_item->vport)
            {
                continue;
            }
            else
            {
                LOG_ERR_DEV(en_dev->parent, "Mac:%pM Already exists in other vf\n", msg->mac_config.unicast_mac_array[i].mac_addr);
                mutex_unlock(&vf_item->lock);
                return MAC_ALREADY_EXISTS_IN_OTHER_VF;
            }
        }
    }

    /* 构建mac数组 */
    l2_entry = kzalloc(sizeof(ZXDH_L2_FWD_KEY) * msg->mac_config.unicast_add_count, GFP_KERNEL);
    if (l2_entry == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "l2_entry malloc fialed\n");
        mutex_unlock(&vf_item->lock);
        return MAC_CONFIG_FAILED;
    }
    for (i = 0; i < msg->mac_config.unicast_add_count; i++)
    {
        memcpy(l2_entry[i].dmac_addr, msg->mac_config.unicast_mac_array[i].mac_addr, ETH_ALEN);
        l2_entry[i].sriov_vlan_id = sriov_vlan_id;
        l2_entry[i].sriov_vlan_tpid = sriov_vlan_tpid;
    }

    /* 配置单播mac */
    ret = dpp_batch_add_unicast_mac(&pf_info, msg->mac_config.unicast_add_count, l2_entry);
    if (ret != 0)
    {
        kfree(l2_entry);
        LOG_ERR_DEV(en_dev->parent, "config unicast mac failed\n");
        mutex_unlock(&vf_item->lock);
        return MAC_CONFIG_FAILED;
    }

    /* 将此单播mac地址添加到vf_item */
    for (i = 0; i < msg->mac_config.unicast_add_count; i++)
    {
        zxdh_vf_item_mac_add(vf_item, l2_entry[i].dmac_addr, 1);
    }

    LOG_DEBUG_DEV(en_dev->parent, "current_unicast_num is %u\n", vf_item->vf_mac_info.current_unicast_num);
    kfree(l2_entry);
    mutex_unlock(&vf_item->lock);
    return ret;
}

int32_t zxdh_pf_add_vf_multicast_mac(struct zxdh_en_device *en_dev, struct dhtool_set_vf_mac_msg *msg)
{
    struct  zxdh_vf_item *vf_item  = NULL;
    uint32_t ret                   = 0;
    uint32_t i                     = 0;
    DPP_PF_INFO_T pf_info          = {0};
    MacAddress *hash_entry         = NULL;

    pf_info.slot = en_dev->slot_id;

    /* 判断vf是否probe */
    if (!en_dev->ops->get_vf_is_probe(en_dev->parent, msg->mac_config.target_vf))
    {
        LOG_ERR_DEV(en_dev->parent, "vf(%u) is not probed\n", msg->mac_config.target_vf);
        return VF_ERROR;
    }

    /* 获取vf_item */
    vf_item = en_dev->ops->get_vf_item(en_dev->parent, msg->mac_config.target_vf);
    if (IS_ERR_OR_NULL(vf_item))
    {
        LOG_ERR_DEV(en_dev->parent, "get_vf(%u)_item failed\n", msg->mac_config.target_vf);
        return MAC_CONFIG_FAILED;
    }

    mutex_lock(&vf_item->lock);
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = vf_item->vport;

    /* 判断组播mac是否超过vf的mac上限 */
    if ((vf_item->vf_mac_info.current_multicast_num + msg->mac_config.multicast_add_count) > VF_MAX_MULTICAST_MAC)
    {
        LOG_ERR_DEV(en_dev->parent, "current multicast mac num beyond 32\n");
        mutex_unlock(&vf_item->lock);
        return MULTICAST_MAC_NUM_BEYOND_MAXNUM;
    }

    /* 构建mac数组 */
    hash_entry = kzalloc(sizeof(MacAddress) * msg->mac_config.multicast_add_count, GFP_KERNEL);
    if (hash_entry == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "hash_entry malloc failed\n");
        mutex_unlock(&vf_item->lock);
        return MAC_CONFIG_FAILED;
    }
    for (i = 0; i < msg->mac_config.multicast_add_count; i++)
    {
        memcpy(hash_entry[i].mac_addr, msg->mac_config.multicast_mac_array[i].mac_addr, ETH_ALEN);
    }

    /* 配置组播mac */
    ret = dpp_batch_add_multicast_mac(&pf_info, msg->mac_config.multicast_add_count, hash_entry);
    if (ret != 0)
    {
        kfree(hash_entry);
        mutex_unlock(&vf_item->lock);
        if (ret == DPP_RC_TBL_IS_FULL) /* 超过整个pf级转发域组播mac上限 */
        {
            LOG_ERR_DEV(en_dev->parent, "current multicast mac num PF beyond mac\n");
            return MULTICAST_MAC_NUM_BEYOND_MAXNUM;
        }
        else
        {
            LOG_ERR_DEV(en_dev->parent, "multicast config failed\n");
            return MAC_CONFIG_FAILED;
        }
    }

    /* 添加到vf_item */
    for (i = 0; i < msg->mac_config.multicast_add_count; i++)
    {
        /* 将此组播mac地址添加到vf_item */
        zxdh_vf_item_mac_add(vf_item, hash_entry[i].mac_addr, 1);
    }

    LOG_DEBUG_DEV(en_dev->parent, "current_multicast_num is %u\n", vf_item->vf_mac_info.current_multicast_num);
    kfree(hash_entry);
    mutex_unlock(&vf_item->lock);
    return MAC_CONFIG_SUCCESS;
}

int32_t zxdh_pf_del_vf_unicast_mac(struct zxdh_en_device *en_dev, struct dhtool_set_vf_mac_msg *msg)
{
    ZXDH_L2_FWD_KEY *l2_entry      = NULL;
    struct  zxdh_vf_item *vf_item  = NULL;
    uint16_t current_vport         = 0;
    int32_t ret                    = 0;
    uint32_t i                     = 0;
    uint16_t sriov_vlan_tpid       = 0;
    uint16_t sriov_vlan_id         = 0;
    DPP_PF_INFO_T pf_info          = {0};

    /* 判断vf是否probe */
    if (!en_dev->ops->get_vf_is_probe(en_dev->parent, msg->mac_config.target_vf))
    {
        LOG_ERR_DEV(en_dev->parent, "vf(%u) is not probed\n", msg->mac_config.target_vf);
        return VF_ERROR;
    }

    /* 获取vf_item */
    vf_item = en_dev->ops->get_vf_item(en_dev->parent, msg->mac_config.target_vf);
    if (IS_ERR_OR_NULL(vf_item))
    {
        LOG_ERR_DEV(en_dev->parent, "get_vf(%u)_item failed\n", msg->mac_config.target_vf);
        return MAC_CONFIG_FAILED;
    }

    mutex_lock(&vf_item->lock);
    /* 获取vlan信息 */
    sriov_vlan_tpid = vf_item->vlan_proto;
    sriov_vlan_id = vf_item->vlan;

    /* 判断单播mac是否存在 */
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = vf_item->vport;

    for (i = 0; i < msg->mac_config.unicast_del_count; i++)
    {
        ret = dpp_unicast_mac_search(&pf_info, msg->mac_config.unicast_mac_array[i].mac_addr,
                                    sriov_vlan_tpid, sriov_vlan_id, &current_vport);
        if ((ret == 0) && (current_vport == vf_item->vport)) /* 找到此单播mac,且属于此vf */
        {
            continue;
        }
        else
        {
            LOG_INFO_DEV(en_dev->parent, "Mac:%pM not exists\n", msg->mac_config.unicast_mac_array[i].mac_addr);
            mutex_unlock(&vf_item->lock);
            return MAC_CONFIG_SUCCESS;
        }
    }

    /* 构建单播mac数组 */
    l2_entry = kzalloc(sizeof(ZXDH_L2_FWD_KEY) * msg->mac_config.unicast_del_count, GFP_KERNEL);
    if (l2_entry == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "l2_entry kzalloc failed\n");
        mutex_unlock(&vf_item->lock);
        return MAC_CONFIG_FAILED;
    }
    for (i = 0; i < msg->mac_config.unicast_del_count; i++)
    {
        memcpy(l2_entry[i].dmac_addr, msg->mac_config.unicast_mac_array[i].mac_addr, ETH_ALEN);
        l2_entry[i].sriov_vlan_id = sriov_vlan_id;
        l2_entry[i].sriov_vlan_tpid = sriov_vlan_tpid;
    }

    /* 删除单播mac */
    ret = dpp_batch_del_unicast_mac(&pf_info, msg->mac_config.unicast_del_count, l2_entry);
    if (ret != 0)
    {
        kfree(l2_entry);
        mutex_unlock(&vf_item->lock);
        LOG_ERR_DEV(en_dev->parent, "del unicast failed\n");
        return MAC_CONFIG_FAILED;
    }

    /* 从vf_item中删除 */
    for (i = 0; i < msg->mac_config.unicast_del_count; i++)
    {
        zxdh_vf_item_mac_del(vf_item, l2_entry[i].dmac_addr);
    }

    LOG_DEBUG_DEV(en_dev->parent, "current_unicast_num is %u\n", vf_item->vf_mac_info.current_unicast_num);
    kfree(l2_entry);
    mutex_unlock(&vf_item->lock);
    return MAC_CONFIG_SUCCESS;
}

int32_t zxdh_pf_del_vf_multicast_mac(struct zxdh_en_device *en_dev, struct dhtool_set_vf_mac_msg *msg)
{
    struct  zxdh_vf_item *vf_item  = NULL;
    int32_t ret                    = 0;
    uint32_t i                     = 0;
    uint32_t j                     = 0;
    uint32_t is_exists             = 0;
    DPP_PF_INFO_T pf_info          = {0};
    MacAddress *hash_entry         = NULL;

    /* 判断vf是否probe */
    if (!en_dev->ops->get_vf_is_probe(en_dev->parent, msg->mac_config.target_vf))
    {
        LOG_ERR_DEV(en_dev->parent, "vf(%u) is not probed\n", msg->mac_config.target_vf);
        return VF_ERROR;
    }

    /* 获取vf_item */
    vf_item = en_dev->ops->get_vf_item(en_dev->parent, msg->mac_config.target_vf);
    if (IS_ERR_OR_NULL(vf_item))
    {
        LOG_ERR_DEV(en_dev->parent, "get_vf(%u)_item failed\n", msg->mac_config.target_vf);
        return MAC_CONFIG_FAILED;
    }

    mutex_lock(&vf_item->lock);
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = vf_item->vport;

    /* 判断此组播mac是否存在 */
    for (i = 0; i < msg->mac_config.multicast_del_count; i++)
    {
        for (j = 0; j < VF_MAX_MULTICAST_MAC; j++)
        {
            if (ether_addr_equal(msg->mac_config.multicast_mac_array[i].mac_addr,
                vf_item->vf_mac_info.multicast_mac[j].mac_addr))
            {
                is_exists = 1;
            }
        }
        if (is_exists != 1)
        {
            LOG_INFO_DEV(en_dev->parent, "multicast mac:%pM not exists\n", msg->mac_config.multicast_mac_array[i].mac_addr);
            mutex_unlock(&vf_item->lock);
            return MAC_CONFIG_SUCCESS;
        }
    }

    /* 构建mac数组 */
    hash_entry = kzalloc(sizeof(MacAddress) * msg->mac_config.multicast_del_count, GFP_KERNEL);
    if (hash_entry == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "hash_entry kzalloc failed\n");
        mutex_unlock(&vf_item->lock);
        return MAC_CONFIG_FAILED;
    }
    for (i = 0; i < msg->mac_config.multicast_del_count; i++)
    {
        memcpy(hash_entry[i].mac_addr, msg->mac_config.multicast_mac_array[i].mac_addr, ETH_ALEN);
    }

    /* 删除组播mac */
    ret = dpp_batch_del_multicast_mac(&pf_info, msg->mac_config.multicast_del_count, hash_entry);
    if (ret != 0)
    {
        kfree(hash_entry);
        mutex_unlock(&vf_item->lock);
        LOG_ERR_DEV(en_dev->parent, "del multicast failed\n");
        return MAC_CONFIG_FAILED;
    }

    /* 从vf_item中删除 */
    for (i = 0; i < msg->mac_config.multicast_del_count; i++)
    {
        zxdh_vf_item_mac_del(vf_item, hash_entry[i].mac_addr);
    }

    LOG_DEBUG_DEV(en_dev->parent, "current_multicast_num is %u\n", vf_item->vf_mac_info.current_multicast_num);
    kfree(hash_entry);
    mutex_unlock(&vf_item->lock);
    return MAC_CONFIG_SUCCESS;
}

void vf_item_unicast_mac_transfer(struct zxdh_vf_item *src_vf_item, struct zxdh_vf_item *dst_vf_item)
{
    int32_t i = 0;
    int32_t j = 0;

    /* 迁移单播mac */
    for (i = 0; i < VF_MAX_UNICAST_MAC; i++)
    {
        /* 先找到待迁移的单播mac */
        if (is_zero_ether_addr(src_vf_item->vf_mac_info.unicast_mac[i].mac_addr))
        {
            continue;
        }

        /* 判断是否有相同mac */
        for (j = 0; j < VF_MAX_UNICAST_MAC; j++)
        {
            if (!is_zero_ether_addr(dst_vf_item->vf_mac_info.unicast_mac[j].mac_addr))
            {
                if (ether_addr_equal(src_vf_item->vf_mac_info.unicast_mac[i].mac_addr,
                    dst_vf_item->vf_mac_info.unicast_mac[j].mac_addr))
                {
                    /* src_vf和dst_vf存在相同mac，不做迁移*/
                    break;
                }
            }
        }

        /* 添加到dst_vf的vf_item */
        zxdh_vf_item_mac_add(dst_vf_item, src_vf_item->vf_mac_info.unicast_mac[i].mac_addr, 1);
    }
    return;
}

void vf_item_multicast_mac_transfer(struct zxdh_vf_item *src_vf_item, struct zxdh_vf_item *dst_vf_item)
{
    int32_t i = 0;
    int32_t j = 0;

    /* 迁移单播mac */
    for (i = 0; i < VF_MAX_MULTICAST_MAC; i++)
    {
        /* 先找到待迁移的单播mac */
        if (is_zero_ether_addr(src_vf_item->vf_mac_info.multicast_mac[i].mac_addr))
        {
            continue;
        }

        /* 判断是否有相同mac */
        for (j = 0; j < VF_MAX_MULTICAST_MAC; j++)
        {
            if (!is_zero_ether_addr(dst_vf_item->vf_mac_info.multicast_mac[j].mac_addr))
            {
                if (ether_addr_equal(src_vf_item->vf_mac_info.multicast_mac[i].mac_addr,
                    dst_vf_item->vf_mac_info.multicast_mac[j].mac_addr))
                {
                    /* src_vf和dst_vf存在相同mac，不做迁移*/
                    break;
                }
            }
        }

        /* 添加到dst_vf的vf_item */
        zxdh_vf_item_mac_add(dst_vf_item, src_vf_item->vf_mac_info.multicast_mac[i].mac_addr, 1);
    }
    return;
}

void vf_item_mac_print(struct zxdh_vf_item *vf_item, uint32_t vf_id)
{
    int32_t i = 0;

    for (i = 0; i < VF_MAX_UNICAST_MAC; i++)
    {
        if (!is_zero_ether_addr(vf_item->vf_mac_info.unicast_mac[i].mac_addr))
        {
            LOG_DEBUG("the %u unicast mac is %pM\n", i, vf_item->vf_mac_info.unicast_mac[i].mac_addr);
        }
    }

    for (i = 0; i < VF_MAX_MULTICAST_MAC; i++)
    {
        if (!is_zero_ether_addr(vf_item->vf_mac_info.multicast_mac[i].mac_addr))
        {
            LOG_DEBUG("the %u multicast mac is %pM\n", i, vf_item->vf_mac_info.multicast_mac[i].mac_addr);
        }
    }

    return;
}

/* 判断MAC地址对应的是组播IPv6还是组播IPv4地址 */
int32_t MulticastType_get(uint8_t *mac_addr)
{
    if (mac_addr[0] == 0x33 && mac_addr[1] == 0x33)
    {
        return 0;
    } else if (mac_addr[0] == 0x01 && mac_addr[1] == 0x00 && mac_addr[2] == 0x5e)
    {
        return 0;
    }
    return -1;  /* 返回-1表示不是已知类型的组播MAC地址*/
}


int32_t ip4_ip6_multicast_mac_del(struct zxdh_en_device *en_dev, struct zxdh_vf_item *src_vf_item, uint32_t src_vf)
{
    struct dhtool_set_vf_mac_msg msg = {0};
    int32_t i = 0;
    int32_t ret = 0;

    msg.mac_config.target_vf = src_vf;

    for (i = 0; i < VF_MAX_MULTICAST_MAC; i++)
    {
        /* 删除条件：非dhtool添加的ipv6和Ipv4组播mac */
        if ((MulticastType_get(src_vf_item->vf_mac_info.multicast_mac[i].mac_addr) == 0)
            && (src_vf_item->vf_mac_info.multicast_mac[i].dhtool_mac_set_flag == 0))
        {
            memcpy(msg.mac_config.multicast_mac_array[msg.mac_config.multicast_del_count].mac_addr,
                  src_vf_item->vf_mac_info.multicast_mac[i].mac_addr, ETH_ALEN);
            msg.mac_config.multicast_del_count++;
        }
    }

    if (msg.mac_config.multicast_del_count != 0)
    {
        ret = zxdh_pf_del_vf_multicast_mac(en_dev, &msg);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_pf_del_vf_multicast_mac failed  before transfer\n");
            return MAC_CONFIG_FAILED;
        }
    }
    return 0;
}

int32_t local_host_unciast_del(struct zxdh_en_device *en_dev, struct zxdh_vf_item *src_vf_item, uint32_t src_vf)
{
    struct dhtool_set_vf_mac_msg msg = {0};
    int32_t ret = 0;

    /* 先删除待迁移的vf0的本机mac */
    memcpy(msg.mac_config.unicast_mac_array[0].mac_addr, src_vf_item->vf_mac_info.unicast_mac[0].mac_addr, ETH_ALEN);
    msg.mac_config.target_vf = src_vf;
    msg.mac_config.unicast_del_count = 1;

    ret = zxdh_pf_del_vf_unicast_mac(en_dev, &msg);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "del mac before transfer failed\n");
        return MAC_CONFIG_FAILED;
    }

    /* 删除vf_item中存放的本机mac */
    memset(src_vf_item->vf_mac_info.unicast_mac[0].mac_addr, 0, ETH_ALEN);
    src_vf_item->vf_mac_info.current_unicast_num--;

    return 0;
}

int32_t zxdh_pf_transfer_vf_mac(struct zxdh_en_device *en_dev, uint32_t src_vf, uint32_t dst_vf)
{
    struct zxdh_vf_item *src_vf_item = NULL;
    struct zxdh_vf_item *dst_vf_item = NULL;
    DPP_PF_INFO_T src_pf_info = {0};
    DPP_PF_INFO_T dst_pf_info = {0};
    int32_t ret = 0;

    /* 判断src_vf是否probe */
    if (!en_dev->ops->get_vf_is_probe(en_dev->parent, src_vf))
    {
        LOG_ERR_DEV(en_dev->parent, "vf(%u) is not probed\n", src_vf);
        return VF_ERROR;
    }

    /* 判断dst_vf是否probe */
    if (!en_dev->ops->get_vf_is_probe(en_dev->parent, dst_vf))
    {
        LOG_ERR_DEV(en_dev->parent, "vf(%u) is not probed\n", dst_vf);
        return VF_ERROR;
    }

    /* 获取src_vf的vf_item */
    src_vf_item = en_dev->ops->get_vf_item(en_dev->parent, src_vf);
    if (IS_ERR_OR_NULL(src_vf_item))
    {
        LOG_ERR_DEV(en_dev->parent, "get_vf(%u)_item failed\n", src_vf);
        return MAC_CONFIG_FAILED;
    }
    src_pf_info.slot = en_dev->slot_id;
    src_pf_info.vport = src_vf_item->vport;

    /* 获取dst_vf的vf_item */
    dst_vf_item = en_dev->ops->get_vf_item(en_dev->parent, dst_vf);
    if (IS_ERR_OR_NULL(dst_vf_item))
    {
        LOG_ERR_DEV(en_dev->parent, "get_vf(%u)_item failed\n", dst_vf);
        return MAC_CONFIG_FAILED;
    }
    dst_pf_info.slot = en_dev->slot_id;
    dst_pf_info.vport = dst_vf_item->vport;

    /* 本机mac删除 */
    ret = local_host_unciast_del(en_dev, src_vf_item, src_vf);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "del local mac failed\n");
        return MAC_CONFIG_FAILED;
    }

    /* 删除ipv4和ipv6对应的组播mac */
    ret = ip4_ip6_multicast_mac_del(en_dev, src_vf_item, src_vf);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "ip4_ip6_multicast_mac_del failed\n");
        return MAC_CONFIG_FAILED;
    }

    mutex_lock(&src_vf_item->lock);
    mutex_lock(&dst_vf_item->lock);
    /* 组播mac迁移 */
    if (src_vf_item->vf_mac_info.current_multicast_num != 0)
    {
        ret = dpp_multicast_mac_transfer(&src_pf_info, &dst_pf_info);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_multicast_mac_transfer from %u to %u failed\n", src_vf, dst_vf);
            mutex_unlock(&dst_vf_item->lock);
            mutex_unlock(&src_vf_item->lock);
            return MULTICAST_MAC_TRANSFER_FAILED;
        }
    }

    /* 单播mac迁移 */
    if (src_vf_item->vf_mac_info.current_unicast_num != 0)
    {
        ret = dpp_unicast_mac_transfer(&src_pf_info, &dst_pf_info);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_transfer from %u to %u failed\n", src_vf, dst_vf);
            mutex_unlock(&dst_vf_item->lock);
            mutex_unlock(&src_vf_item->lock);
            return UNICAST_MAC_TRANSFER_FAILED;
        }
    }

    /* 将src_vf的vf_item中的的mac迁移到dst_vf中 */
    vf_item_unicast_mac_transfer(src_vf_item, dst_vf_item);
    vf_item_multicast_mac_transfer(src_vf_item, dst_vf_item);

    /* 清除src_vf的vf_item中存放的mac数组*/
    memset(&src_vf_item->vf_mac_info, 0, sizeof(src_vf_item->vf_mac_info));

    LOG_DEBUG_DEV(en_dev->parent, "src_vf(%u) current unicast mac num is %u\n",
        src_vf, src_vf_item->vf_mac_info.current_unicast_num);
    LOG_DEBUG_DEV(en_dev->parent, "src_vf(%u) current multicast mac num is %u\n",
        src_vf, src_vf_item->vf_mac_info.current_multicast_num);
    LOG_DEBUG_DEV(en_dev->parent, "dst_vf(%u) current unicast mac num is %u\n",
        dst_vf, dst_vf_item->vf_mac_info.current_unicast_num);
    LOG_DEBUG_DEV(en_dev->parent, "dst_vf(%u) current multicast mac num is %u\n",
        dst_vf, dst_vf_item->vf_mac_info.current_multicast_num);

    vf_item_mac_print(src_vf_item, src_vf);
    vf_item_mac_print(dst_vf_item, dst_vf);

    mutex_unlock(&dst_vf_item->lock);
    mutex_unlock(&src_vf_item->lock);

    return MAC_CONFIG_SUCCESS;
}

int32_t zxdh_en_set_vepa(struct zxdh_en_device *en_dev, bool setting)
{
    struct zxdh_vf_item *vf_item = NULL;
    bool vepa = false;
    uint16_t vf_idx = 0;
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    vepa = en_dev->ops->get_vepa(en_dev->parent);
    if (setting == vepa)
    {
        LOG_ERR_DEV(en_dev->parent, "vport(0x%x) is now %s mode\n", en_dev->vport, vepa?"vepa":"veb");
        return 0;
    }

    en_dev->ops->set_vepa(en_dev->parent, setting);
    ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_VEPA_EN_OFF, (uint32_t)setting);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to setup vport(0x%x) %s mode, ret: %d\n", en_dev->vport, setting?"vepa":"veb", ret);
        return ret;
    }

    for (vf_idx = 0; vf_idx < ZXDH_VF_NUM_MAX; vf_idx++)
    {
        vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_idx);
        if (IS_ERR_OR_NULL(vf_item))
        {
            break;
        }

        if (vf_item->is_probed)
        {
            pf_info.vport = vf_item->vport;
            ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_VEPA_EN_OFF, (uint32_t)setting);
            if (ret != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "Failed to setup vport(0x%x) %s mode, ret: %d\n",
                            vf_item->vport, setting?"vepa":"veb", ret);
                return ret;
            }
            LOG_DEBUG_DEV(en_dev->parent, "Configure vport(0x%x) to %s mode\n", vf_item->vport, setting?"vepa":"veb");
        }
    }

    LOG_INFO_DEV(en_dev->parent, "Configure vport(0x%x) to %s mode\n", en_dev->vport, setting?"vepa":"veb");

    return ret;
}

#ifdef HAVE_FDB_OPS
#if defined(HAVE_NDO_FDB_ADD_EXTACK)
#if defined(NDO_FDB_HAVE_MODIFIED)
static int zxdh_en_ndo_fdb_add(struct ndmsg *ndm, struct nlattr *tb[],
                                         struct net_device *dev, const unsigned char *addr,
                                         u16 vid, u16 flags, bool *modified, struct netlink_ext_ack *extack)
#else
static int zxdh_en_ndo_fdb_add(struct ndmsg *ndm, struct nlattr *tb[],
                                         struct net_device *dev, const unsigned char *addr,
                                         u16 vid, u16 flags, struct netlink_ext_ack *extack)
#endif
#elif defined(HAVE_NDO_FDB_ADD_VID)
static int zxdh_en_ndo_fdb_add(struct ndmsg *ndm, struct nlattr *tb[],
                               struct net_device *dev, const unsigned char *addr,
                               u16 vid, u16 flags)
#elif defined(HAVE_NDO_FDB_ADD_NLATTR)
static int zxdh_en_ndo_fdb_add(struct ndmsg *ndm, struct nlattr *tb[],
                               struct net_device *dev, const unsigned char *addr,
                               u16 flags)
#elif defined(USE_CONST_DEV_UC_CHAR)
static int zxdh_en_ndo_fdb_add(struct ndmsg *ndm, struct net_device *dev,
                               const unsigned char *addr, u16 flags)
#else
static int zxdh_en_ndo_fdb_add(struct ndmsg *ndm, struct net_device *dev,
                               unsigned char *addr, u16 flags)
#endif
{
    struct zxdh_en_priv *en_priv  = netdev_priv(dev);
    struct zxdh_en_device *en_dev = &en_priv->edev; /*aux层net_device的私有结构体 */
    int32_t err = 0;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    LOG_DEBUG_DEV(en_dev->parent, "vport is %#x\n", en_dev->vport);
    LOG_DEBUG_DEV(en_dev->parent, "addr is %pM\n", addr);
    LOG_DEBUG_DEV(en_dev->parent, "ndm_state is %u\n", ndm->ndm_state);

    /* 检查这个设备的ndm状态是否是静态的 */
    if (ndm->ndm_state && !(ndm->ndm_state & NUD_PERMANENT))
    {
        LOG_ERR_DEV(en_dev->parent, "FDB only supports static addresses\n");
        return -EINVAL;
    }

    /* 判断mac地址是否全0 */
    if (is_zero_ether_addr(addr))
    {
        LOG_ERR_DEV(en_dev->parent, "Invalid mac\n");
        return -EINVAL;
    }

    if (is_unicast_ether_addr(addr) || is_link_local_ether_addr(addr))
    {
        err = unicast_mac_add(en_dev, dev, addr, flags);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "unicast_mac_add failed");
            return err;
        }
    }
    else if (is_multicast_ether_addr(addr))
    {
        err = multicast_mac_add(en_dev, dev, addr, flags);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "multicast_mac_add failed");
            return err;
        }
    }
    else
    {
        err = -EINVAL;
    }

#ifdef MAC_CONFIG_DEBUG
    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        err = zxdh_pf_dump_all_mac(en_dev);
        if (err != 0)
        {
            LOG_INFO_DEV(en_dev->parent, "zxdh_pf_dump_all_mac failed, ret:%d\n", err);
            return -1;
        }
    }
#endif /* MAC_CONFIG_DEBUG */

    LOG_DEBUG_DEV(en_dev->parent, "zxdh_en_ndo_fdb_add end\n");
    return err;
}

#ifdef HAVE_NDO_FEATURES_CHECK
static netdev_features_t zxdh_en_features_check(struct sk_buff *skb, struct net_device *dev,
                                                           netdev_features_t features)
{
    return features;
}
#endif /* HAVE_NDO_FEATURES_CHECK */

#if defined(ZXDH_ADAPT_ANOLIS_6_6) || defined(Rocky_9_3) || defined(ZXDH_ADAPT_REDHAT_9_6) || defined(KYLIN_V11_6_6) || defined(NDO_FDB_DEL_HAVE_ACK_NO_MOD)
static int zxdh_en_ndo_fdb_del(struct ndmsg *ndm, struct nlattr **nla, struct net_device *dev,
                             const unsigned char *addr, u16 vid, struct netlink_ext_ack *ack)
#elif defined(NDO_FDB_HAVE_MODIFIED)
static int zxdh_en_ndo_fdb_del(struct ndmsg *ndm, struct nlattr **nla, struct net_device *dev,
                             const unsigned char *addr, u16 vid, bool *modified, struct netlink_ext_ack *ack)
#else
#ifdef USE_CONST_DEV_UC_CHAR
#ifdef HAVE_NDO_FDB_DEL_VID
static int zxdh_en_ndo_fdb_del(struct ndmsg *ndm, struct nlattr **nla, struct net_device *dev,
                            const unsigned char *addr, u16 vid)
#else
static int zxdh_en_ndo_fdb_del(struct ndmsg *ndm, struct net_device *dev,
                            const unsigned char *addr)
#endif
#else
#ifdef HAVE_NDO_FDB_DEL_VID
static int zxdh_en_ndo_fdb_del(struct ndmsg *ndm, struct net_device *dev,
                            unsigned char *addr, u16 vid)
#else
static int zxdh_en_ndo_fdb_del(struct ndmsg *ndm, struct net_device *dev,
                            unsigned char *addr)
#endif
#endif
#endif
{
    struct zxdh_en_priv *en_priv  = netdev_priv(dev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t err = 0;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    LOG_DEBUG_DEV(en_dev->parent, "the vport is %#x", en_dev->vport);
    LOG_DEBUG_DEV(en_dev->parent, "the addr is %pM\n", addr);
    LOG_DEBUG_DEV(en_dev->parent, "ndm_state is %u,\n", ndm->ndm_state);


    /* 检查这个设备的ndm状态是否是静态的 */
    if (!(ndm->ndm_state & NUD_PERMANENT))
    {
        LOG_ERR_DEV(en_dev->parent, "FDB only supports static addresses\n");
        return -EINVAL;
    }

    /* 地址是否全为0 */
    if (is_zero_ether_addr(addr))
    {
        LOG_ERR_DEV(en_dev->parent, "Invalid mac address\n");
        return -EINVAL;
    }

    /* 根据mac地址类型，对相对应地址链表做删除操作 */
    if (is_unicast_ether_addr(addr) || is_link_local_ether_addr(addr))
    {
        err = unicast_mac_del(en_dev, dev, addr);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "unicast_mac_del failed\n");
            return err;
        }
    }
    else if (is_multicast_ether_addr(addr))
    {
        err = multicast_mac_del(en_dev, dev, addr);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "multicast_mac_del failed\n");
            return err;
        }
    }
    else
    {
        return -EINVAL;
    }

#ifdef MAC_CONFIG_DEBUG
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        /*先dump所有mac地址*/
        err = zxdh_pf_dump_all_mac(en_dev);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_pf_dump_all_mac failed, ret:%d\n", err);
            return -1;
        }
    }
#endif /* MAC_CONFIG_DEBUG */

    LOG_DEBUG_DEV(en_dev->parent, "zxdh_en_ndo_fdb_del end\n");
    return err;
}

#ifdef HAVE_BRIDGE_ATTRIBS
#if defined(HAVE_NDO_BRIDGE_SETLINK_EXTACK)
static int zxdh_en_ndo_bridge_setlink(struct net_device *dev, struct nlmsghdr *nlh,
                                      u16 flags, struct netlink_ext_ack *extack)
#elif defined(HAVE_NDO_BRIDGE_SET_DEL_LINK_FLAGS)
static int zxdh_en_ndo_bridge_setlink(struct net_device *dev, struct nlmsghdr *nlh, u16 flags)
#else
static int zxdh_en_ndo_bridge_setlink(struct net_device *dev, struct nlmsghdr *nlh)
#endif
{
    struct zxdh_en_priv *en_priv = netdev_priv(dev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct nlattr *attr = NULL;
    struct nlattr *br_spec = NULL;
    int32_t rem = 0;
    uint16_t mode = BRIDGE_MODE_UNDEF;
    bool setting = false;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA) {
        return -EINVAL;
    }

    if(en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        return -EOPNOTSUPP;
    }

    br_spec = nlmsg_find_attr(nlh, sizeof(struct ifinfomsg), IFLA_AF_SPEC);
    if (br_spec == NULL)
    {
        return -EINVAL;
    }

    nla_for_each_nested(attr, br_spec, rem)
    {
        if (nla_type(attr) != IFLA_BRIDGE_MODE)
        {
            continue;
        }

        if (nla_len(attr) < sizeof(mode))
        {
            return -EINVAL;
        }

        mode = nla_get_u16(attr);
        if (mode > BRIDGE_MODE_VEPA)
        {
            return -EINVAL;
        }
        break;
    }

    if (mode == BRIDGE_MODE_UNDEF)
    {
        return -EINVAL;
    }

    setting = (mode == BRIDGE_MODE_VEPA) ? 1 : 0;

    return zxdh_en_set_vepa(en_dev, setting);
}

#ifdef HAVE_NDO_BRIDGE_GETLINK_NLFLAGS
static int zxdh_en_ndo_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
                                      struct net_device *dev, u32 __always_unused filter_mask,
                                      int nlflags)
#elif defined(HAVE_BRIDGE_FILTER)
static int zxdh_en_ndo_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
                                      struct net_device *dev, u32 __always_unused filter_mask)
#else
static int zxdh_en_ndo_bridge_getlink(struct sk_buff *skb, u32 pid, u32 seq,
                                      struct net_device *dev)
#endif /* NDO_BRIDGE_STUFF */
{
    struct zxdh_en_priv *en_priv = netdev_priv(dev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t mode = 0;
    bool vepa = false;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        return -EOPNOTSUPP;
    }

    vepa = en_dev->ops->get_vepa(en_dev->parent);
    mode = vepa ? BRIDGE_MODE_VEPA : BRIDGE_MODE_VEB;

    return ndo_dflt_bridge_getlink(skb, pid, seq, dev, mode, 0, 0, nlflags, filter_mask, NULL);
}
#endif /* HAVE_BRIDGE_ATTRIBS */
#endif /* HAVE_FDB_OPS */

static int32_t zxdh_pf_notify_vf_set_link_state(struct zxdh_en_device *en_dev, int vf_idx, bool link_up)
{
    int32_t retval = 0;
    uint16_t func_no = 0;
    uint16_t pf_no = FIND_PF_ID(en_dev->pcie_id);
    uint8_t link_info = 0;
    uint8_t link_up_val = 0;
    uint8_t phyport_val = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr_to_agt.op_code = AGENT_DEV_STATUS_NOTIFY;
    msg->payload.hdr_to_agt.pcie_id = en_dev->pcie_id;

    func_no = GET_FUNC_NO(pf_no, vf_idx);
    LOG_DEBUG_DEV(en_dev->parent, "vf_idx:%d, func_no=0x%x\n",vf_idx, func_no);
    msg->payload.pcie_msix_msg.func_no[msg->payload.pcie_msix_msg.num++] = func_no;
    if(en_dev->ops->is_bond(en_dev->parent))
    {
        link_up_val = link_up ? 1 : 0;
        phyport_val = en_dev->ops->get_pf_phy_port(en_dev->parent);
        link_info = (phyport_val & 0x0F) << 4 | (link_up_val & 0x0F);
        LOG_DEBUG_DEV(en_dev->parent, "phyport and link_up need write , val: 0x%x\n", link_info);
        en_dev->ops->set_vf_link_info(en_dev->parent, vf_idx, link_info);
    }
    else
    {
        en_dev->ops->set_vf_link_info(en_dev->parent, vf_idx, link_up ? 1 : 0);
    }
    LOG_DEBUG_DEV(en_dev->parent, "msg->pcie_msix_msg.num:%d\n", msg->payload.pcie_msix_msg.num);
    retval = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_MAC, msg, msg, &para);
    if (retval != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to update VF link info\n");
    }
    kfree(msg);
    return retval;
}

static int32_t zxdh_pf_set_vf_link_state(struct zxdh_en_device *en_dev, int vf_idx, int link_status)
{
    int32_t retval   = 0;
    struct zxdh_vf_item *vf_item = NULL;
    struct zxdh_lag_dev *ldev = NULL;
    bool pf_link_up = en_dev->ops->get_pf_link_up(en_dev->parent);

    vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_idx);
    switch (link_status)
    {
        case IFLA_VF_LINK_STATE_AUTO:
            LOG_DEBUG_DEV(en_dev->parent, "[SET_VF_LINK_STATE]--NDO set VF %d link state auto\n", vf_idx);
            ldev = en_dev->ldev;
            if (ldev)
            {
                mutex_lock(&ldev->mlock);
                if(ldev->state == LAG_DEV_ACTIVE && ldev->is_active && ldev->upper_netdev && ldev->tracker.bond_type == HARDWARE_BOND && (en_dev->panel_id == ldev->primary_pf_idx))
                {  // 处于硬bond场景
                    LOG_INFO_DEV(en_dev->parent, "primary port %s(link up %d) is hardware-bond mode, use bond_link_info %d to update vf\n", netdev_name(en_dev->netdev), en_dev->link_up, en_dev->bond_link_info);
                    vf_item->link_forced = FALSE;
                    vf_item->link_up = en_dev->bond_link_info == 0 ? 0 : 1;
                }
                else
                {
                    vf_item->link_forced = FALSE;
                    vf_item->link_up = pf_link_up;
                }
                mutex_unlock(&ldev->mlock);
            }
            else
            {
                vf_item->link_forced = FALSE;
                vf_item->link_up = pf_link_up;
            }
            break;
        case IFLA_VF_LINK_STATE_ENABLE:
            LOG_DEBUG_DEV(en_dev->parent, "[SET_VF_LINK_STATE]--NDO set VF %d link state enable\n", vf_idx);
            vf_item->link_forced = TRUE;
            vf_item->link_up = TRUE;
            break;
        case IFLA_VF_LINK_STATE_DISABLE:
            LOG_DEBUG_DEV(en_dev->parent, "[SET_VF_LINK_STATE]--NDO set VF %d link state disable\n", vf_idx);
            vf_item->link_forced = TRUE;
            vf_item->link_up = FALSE;
            break;
        default:
            LOG_ERR_DEV(en_dev->parent, "[SET_VF_LINK_STATE]--NDO set VF %d - invalid link status %d\n", vf_idx, link_status);
            return -EINVAL;
    }
    LOG_DEBUG_DEV(en_dev->parent, "vf_item->is_probed: %s\n", vf_item->is_probed?"TRUE":"FALSE");
    if(vf_item->is_probed)
    {
        /* Notify the VF of its new link state */
        retval = zxdh_pf_notify_vf_set_link_state(en_dev, vf_idx, vf_item->link_up);
        if (0 != retval)
        {
            LOG_ERR_DEV(en_dev->parent, "[SET_VF_LINK_STATE]--Failed to set VF %d link state %d\n", vf_idx, vf_item->link_up);
            return retval;
        }
    }
    return retval;
}

int zxdh_en_ndo_set_vf_link_state(struct net_device *netdev, int vf_idx, int link_status)
{
    int num_vfs = 0;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct pci_dev *pdev = NULL;
    struct dh_core_dev *dh_dev = NULL;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    dh_dev = en_dev->parent;
    pdev = en_dev->ops->get_pdev(dh_dev);
    num_vfs = pci_num_vf(pdev);
    if ((vf_idx < 0) || (vf_idx >= num_vfs))
    {
        LOG_ERR_DEV(dh_dev, "[SET_VF_LINK_STATE]--NDO set VF link - invalid VF idx: %d\n", vf_idx);
        return -EINVAL;
    }
    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA) {
        LOG_ERR_DEV(en_dev->parent, "[SET_VF_LINK_STATE]--NDO set VF link - RDMA dev unsupport set vf link.\n");
        return -EOPNOTSUPP;
    }

    return zxdh_pf_set_vf_link_state(en_dev, vf_idx, link_status);
}

static int zxdh_enable_sriov_vlan_tbl(DPP_PF_INFO_T *pf_info, uint16_t vlan_tci, uint16_t vlan_proto)
{
    int ret = 0;

    ret = dpp_vport_vlan_offload_en_set(pf_info, 1);
    if (ret != 0)
    {
        return ret;
    }

    dpp_vqm_vfid_vlan_set(pf_info, VLAN_SRIOV_VLAN_TCI, vlan_tci);

    dpp_vqm_vfid_vlan_set(pf_info, VLAN_SRIOV_VLAN_TPID, vlan_proto);

    return 0;
}

static int zxdh_disable_sriov_vlan_tbl(DPP_PF_INFO_T *pf_info)
{
    int ret = 0;

    ret = dpp_vport_vlan_offload_en_set(pf_info, 0);
    if (ret != 0)
    {
        return ret;
    }

    dpp_vqm_vfid_vlan_set(pf_info, VLAN_SRIOV_VLAN_TCI, 0);

    dpp_vqm_vfid_vlan_set(pf_info, VLAN_SRIOV_VLAN_TPID, 0);

    return 0;
}

static int32_t zxdh_handle_mac_operations(DPP_PF_INFO_T *pf_info, MAC_VPORT_INFO *unicast_mac_arry, uint32_t current_unicast_num, \
                                          struct zxdh_vf_item *vf_item, uint16_t new_vlan_tci, uint16_t vlan_proto, int add)
{
    int32_t retval = 0;
    uint32_t i = 0;
    for (i = 0; i < current_unicast_num; i++)
    {
        if (vf_item->vport == unicast_mac_arry[i].vport)
        {
            if (add)
            {
                retval = dpp_add_mac(pf_info, unicast_mac_arry[i].addr, htons(vlan_proto), new_vlan_tci);
            }
            else
            {
                retval = dpp_del_mac(pf_info, unicast_mac_arry[i].addr, unicast_mac_arry[i].sriov_vlan_tpid, unicast_mac_arry[i].sriov_vlan_id);
            }

            if (retval != 0)
            {
                return retval;
            }
        }
    }
    return 0;
}

static int32_t zxdh_pf_set_vf_port_vlan(struct zxdh_en_device *en_dev, int vf_idx, u16 vid, u8 qos, uint16_t vlan_proto)
{
    int32_t retval = 0;
    uint32_t i = 0;
    struct zxdh_vf_item *vf_item = NULL;
    DPP_PF_INFO_T pf_info = {0};
    uint16_t new_vlan_tci = 0;
    uint16_t old_vlan_tci = 0;
    uint16_t old_vid = 0;
    MAC_VPORT_INFO *unicast_mac_arry   = NULL;
    uint32_t current_unicast_num   = 0;
    uint16_t current_vport         = 0;
    struct  zxdh_vf_item *cur_vf_item  = NULL;
    struct pci_dev *pdev = NULL;
    uint16_t num_vfs               = 0;

    pdev = en_dev->ops->get_pdev(en_dev->parent);
    num_vfs = pci_num_vf(pdev);
    if (num_vfs == 0) {
        LOG_ERR_DEV(en_dev->parent, "vf is disable, vf number:%d\n", num_vfs);
        return -ENODEV;
    }

    vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_idx);
    old_vid = vf_item->vlan;
    old_vlan_tci = ZXDH_VLAN_TCI_GEN(vf_item->vlan, vf_item->qos);
    new_vlan_tci = ZXDH_VLAN_TCI_GEN(vid, qos);
    if (new_vlan_tci == 0)
    {
        vlan_proto = 0;
    }

    /* 参数vlan_proto是网络字节序， 转为为主机字节序保存在vf_item中*/
    if (new_vlan_tci == old_vlan_tci && vf_item->vlan_proto == htons(vlan_proto))
    {
        return 0;
    }

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    unicast_mac_arry = (MAC_VPORT_INFO *)kzalloc(sizeof(MAC_VPORT_INFO)*UNICAST_MAX_NUM, GFP_KERNEL);
    if (unicast_mac_arry == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc unicast_mac_arry failed \n");
        return -ENOMEM;
    }

    mutex_lock(&vf_item->lock);
    retval = dpp_unicast_mac_dump(&pf_info, unicast_mac_arry, &current_unicast_num);
    if (retval != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_dump failed, ret:%d\n", retval);
        retval = -1;
        goto out_free;
    }

    for (i=0; i<current_unicast_num; i++)
    {
        if (vf_item->vport == unicast_mac_arry[i].vport)
        {
            retval = dpp_unicast_mac_search(&pf_info, unicast_mac_arry[i].addr, htons(vlan_proto), vid, &current_vport);
            if ((retval == 0) && (vf_item->vport != current_vport))
            {
                LOG_ERR_DEV(en_dev->parent, "modify vlan failed, [Mac]+[vlan] Already exists: current_vport=0x%04x\n", current_vport);
                LOG_ERR_DEV(en_dev->parent, "new_vlan_id=%d; qos=%u; vlan_proto=0x%x\n", vid, qos, htons(vlan_proto));
                LOG_ERR_DEV(en_dev->parent, "mac = %x %x %x %x %x %x\n", unicast_mac_arry[i].addr[0],unicast_mac_arry[i].addr[1],\
                                                        unicast_mac_arry[i].addr[2],unicast_mac_arry[i].addr[3],\
                                                        unicast_mac_arry[i].addr[4],unicast_mac_arry[i].addr[5]);
                retval = -EEXIST;
                goto out_free;
            }
            else if ((retval != 0) && (retval != DPP_HASH_RC_SRH_FAIL))
            {
                LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_search failed, ret:%d\n", retval);
                retval = -1;
                goto out_free;
            }
        }
    }

    if (vf_item->pf_set_mac)
    {
        retval = dpp_unicast_mac_search(&pf_info, vf_item->mac, htons(vlan_proto), vid, &current_vport);
        if ((retval == 0) && (current_vport != vf_item->vport)) {
            LOG_ERR_DEV(en_dev->parent, "modify vlan failed, [Itm Mac]+[vlan] Already exists np: current_vport=0x%04x\n", current_vport);
            LOG_ERR_DEV(en_dev->parent, "new_vlan_id=%d; qos=%u; vlan_proto=0x%x\n", vid, qos, htons(vlan_proto));
            LOG_ERR_DEV(en_dev->parent, "mac = %x %x %x %x %x %x\n", vf_item->mac[0],vf_item->mac[1],vf_item->mac[2],\
                                                vf_item->mac[3],vf_item->mac[4],vf_item->mac[5]);
            retval = -EEXIST;
            goto out_free;
        }
        else if((retval != 0) && (retval != DPP_HASH_RC_SRH_FAIL))
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_search failed, ret:%d\n", retval);
            retval = -1;
            goto out_free;
        }

        for (i = 0; i < num_vfs; i++) {
            if (i == vf_idx)
                continue;
            cur_vf_item = en_dev->ops->get_vf_item(en_dev->parent, i);
            if (IS_ERR_OR_NULL(cur_vf_item)) {
                LOG_ERR_DEV(en_dev->parent, "Failed to get vf_item, vf_idx:%d\n", i);
                retval = -EEXIST;
                goto out_free;
            }

            if (ether_addr_equal(cur_vf_item->mac, vf_item->mac) && \
                ((ZXDH_VLAN_TCI_GEN(cur_vf_item->vlan, cur_vf_item->qos) == new_vlan_tci) && (cur_vf_item->vlan_proto == htons(vlan_proto)))) {
                LOG_ERR_DEV(en_dev->parent, "modify vlan failed, [Itm Mac]+[vlan] Already exists: current_vport=0x%04x\n", current_vport);
                LOG_ERR_DEV(en_dev->parent, "new_vlan_id=%d; qos=%u; vlan_proto=0x%x\n", vid, qos, htons(vlan_proto));
                LOG_ERR_DEV(en_dev->parent, "mac = %x %x %x %x %x %x\n", vf_item->mac[0],vf_item->mac[1],vf_item->mac[2],\
                                                    vf_item->mac[3],vf_item->mac[4],vf_item->mac[5]);
                retval = -EEXIST;
                goto out_free;
            }
        }
    }

    pf_info.vport = vf_item->vport;

    if (vid)
    {
        if (en_dev->ops->get_vf_is_probe(en_dev->parent, vf_idx))
        {
            retval = zxdh_handle_mac_operations(&pf_info, unicast_mac_arry, current_unicast_num, vf_item, old_vid, vlan_proto, 0);
            if (retval != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "del handle mac operations failed: %d\n", retval);
                retval = -1;
                goto out_free;
            }

            retval = zxdh_handle_mac_operations(&pf_info, unicast_mac_arry, current_unicast_num, vf_item, vid, vlan_proto, 1);
            if (retval != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "add handle mac operations failed: %d\n", retval);
                retval = -1;
                goto out_free;
            }
        }

        retval = zxdh_enable_sriov_vlan_tbl(&pf_info, new_vlan_tci, htons(vlan_proto));
        if (retval != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_enable_sriov_vlan_tbl, retval: %d\n", retval);
            retval = -1;
            goto out_free;
        }
    }
    else
    {
        if (en_dev->ops->get_vf_is_probe(en_dev->parent, vf_idx))
        {
            retval = zxdh_handle_mac_operations(&pf_info, unicast_mac_arry, current_unicast_num, vf_item, old_vid, vlan_proto, 0);
            if (retval != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "dpp del all unicast mac failed: %d\n", retval);
                retval = -1;
                goto out_free;
            }

            retval = zxdh_handle_mac_operations(&pf_info, unicast_mac_arry, current_unicast_num, vf_item, 0, 0, 1);
            if (retval != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "dpp add all unicast mac failed: %d\n", retval);
                retval = -1;
                goto out_free;
            }
        }

        retval = zxdh_disable_sriov_vlan_tbl(&pf_info);
        if (retval != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_disable_sriov_vlan_tbl failed: %d\n", retval);
            retval = -1;
            goto out_free;
        }
    }

    /* 更新pf本地的vf vlan信息，用于ip link show显示， 和vf初始化获取和重配*/
    vf_item->vlan = vid;
    vf_item->qos = qos;
    vf_item->vlan_proto = htons(vlan_proto);

out_free:
    mutex_unlock(&vf_item->lock);
    kfree(unicast_mac_arry);
    return retval;
}

int zxdh_en_ndo_set_vf_mac(struct net_device *netdev, int vf_id, u8 *mac)
{
    struct  zxdh_en_priv *en_priv  = netdev_priv(netdev);
    struct  zxdh_en_device *en_dev = &en_priv->edev;
    struct  zxdh_vf_item *vf_item  = NULL;
    struct  zxdh_vf_item *cur_vf_item  = NULL;
    int32_t retval                 = 0;
    uint8_t i                      = 0;
    uint16_t sriov_vlan_tpid       = 0;
    uint16_t sriov_vlan_id         = 0;
    uint16_t sriov_vlan_tci        = 0;
    uint16_t current_vport         = 0;
    uint16_t num_vfs               = 0;
    struct pci_dev *pdev = NULL;
    DPP_PF_INFO_T pf_info = {0};

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    LOG_DEBUG_DEV(en_dev->parent, "[SET_VF_MAC]--setting MAC %pM on VF %d\n", mac, vf_id);
    pdev = en_dev->ops->get_pdev(en_dev->parent);
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    num_vfs = pci_num_vf(pdev);
    if (num_vfs == 0)
    {
        LOG_ERR_DEV(en_dev->parent, "vf is disable, vf number:%d\n", num_vfs);
        return -ENODEV;
    }

    vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_id);
    if (IS_ERR_OR_NULL(vf_item))
    {
        return -ENODEV;
    }

    if (is_multicast_ether_addr(mac))
    {
        LOG_ERR_DEV(en_dev->parent, "Invalid Ethernet address %pM for VF %d\n", mac, vf_id);
        return -EINVAL;
    }

    if (is_zero_ether_addr(mac))
    {
        vf_item->pf_set_mac = false;
        ether_addr_copy(vf_item->mac, mac);
        return 0;
    }

    mutex_lock(&vf_item->lock);
    sriov_vlan_tpid = vf_item->vlan_proto;
    sriov_vlan_id = vf_item->vlan;
    sriov_vlan_tci = ZXDH_VLAN_TCI_GEN(vf_item->vlan, vf_item->qos);

    retval = dpp_unicast_mac_search(&pf_info, mac, sriov_vlan_tpid, sriov_vlan_id, &current_vport);
    if ((retval == 0) && (current_vport == vf_item->vport))
    {
        mutex_unlock(&vf_item->lock);
        return 0;
    }
    else if ((retval == 0) && (current_vport != vf_item->vport))
    {
        LOG_ERR_DEV(en_dev->parent, "Mac Already exists\n");
        mutex_unlock(&vf_item->lock);
        return -EEXIST;
    }
    else if((retval != 0) && (retval != DPP_HASH_RC_SRH_FAIL))
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_unicast_mac_search failed, ret:%d\n", retval);
        mutex_unlock(&vf_item->lock);
        return -1;
    }

    for (i = 0; i < num_vfs; i++)
    {
        if (i == vf_id)
            continue;
        cur_vf_item = en_dev->ops->get_vf_item(en_dev->parent, i);
        if (IS_ERR_OR_NULL(cur_vf_item))
        {
            LOG_ERR_DEV(en_dev->parent, "Failed to get vf_item, vf_id:%d\n", i);
            mutex_unlock(&vf_item->lock);
            return -ENODEV;
        }

        if (ether_addr_equal(cur_vf_item->mac, mac) && \
           ((ZXDH_VLAN_TCI_GEN(cur_vf_item->vlan, cur_vf_item->qos) == sriov_vlan_tci) && \
           (cur_vf_item->vlan_proto == sriov_vlan_tpid)))
        {
            LOG_INFO_DEV(en_dev->parent, "%s Mac already exists vf %d\n", __func__, i);
            mutex_unlock(&vf_item->lock);
            return -EEXIST;
        }
    }

    if (!en_dev->ops->get_vf_is_probe(en_dev->parent, vf_id))
    {
        goto set_flag;
    }

    en_dev->ops->set_vf_mac(en_dev->parent, mac, vf_id);

set_flag:
    vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_id);
    vf_item->pf_set_mac = true;
    ether_addr_copy(vf_item->mac, mac);
    mutex_unlock(&vf_item->lock);
    return 0;
}

/* Started by AICoder, pid:e0afcte8b0f536314b9d0bcfc0e2ed2a5211e78b */
int32_t zxdh_get_vlan_info(void *in_para, void *out_para)
{
    struct net_device *netdev = NULL;
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    struct zxdh_rdma_vlan_event_info *info = NULL;

    netdev = (struct net_device *)in_para;
    en_priv= netdev_priv(netdev);
    en_dev= &en_priv->edev;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    info = (struct zxdh_rdma_vlan_event_info *)out_para;
    info->vlan_id = en_dev->vlan_dev.vlan_id;
    info->qos = en_dev->vlan_dev.qos;
    info->protocol = en_dev->vlan_dev.protocol;

    return 0;
}
/* Ended by AICoder, pid:e0afcte8b0f536314b9d0bcfc0e2ed2a5211e78b */

#ifdef IFLA_VF_VLAN_INFO_MAX
int zxdh_en_ndo_set_vf_port_vlan(struct net_device *netdev, int vf_id,
                                               u16 vlan_id, u8 qos, __be16 vlan_proto)
#else
int zxdh_en_ndo_set_vf_port_vlan(struct net_device *netdev, int vf_id, u16 vlan_id, u8 qos)
#endif /* IFLA_VF_VLAN_INFO_MAX */
{
    int num_vfs = 0;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct pci_dev *pdev = NULL;
    struct dh_core_dev *dh_dev = NULL;
    int ret = 0;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA) {
        LOG_ERR_DEV(en_dev->parent, "[SET+VF_VLAN]--NDO set VF vlan - RDMA dev unsupport set vf vlan.\n");
        return -EINVAL;
    }

    /* Comparing with the mellnox network card, it only supports the configuration of cvlan*/
    if (vlan_proto != htons(ETH_P_8021Q) && vlan_proto != htons(ETH_P_8021AD))
    {
        return -EPROTONOSUPPORT;
    }
    dh_dev = en_dev->parent;
    pdev = en_dev->ops->get_pdev(dh_dev);
    num_vfs = pci_num_vf(pdev);
    if (vf_id >= num_vfs || vlan_id > MAX_VLAN_ID || qos > MAX_QOS_ID)
    {
        LOG_ERR_DEV(en_dev->parent, "[SET+VF_VLAN]--NDO set VF vlan - invalid VF idx: %d\n", vf_id);
        return -EINVAL;
    }
    ret = zxdh_pf_set_vf_port_vlan(en_dev, vf_id, vlan_id, qos, vlan_proto);
    if (ret) {
        LOG_ERR_DEV(en_dev->parent, "set vf %d port failed\n", vf_id);
        return ret;
    }
    ret = zxdh_pf_set_vf_vlan_info(en_dev, vf_id, vlan_id, qos, vlan_proto);
    if (ret) {
        LOG_ERR_DEV(en_dev->parent, "set vf %d port vlan info failed\n", vf_id);
        return ret;
    }
    return ret;
}

int zxdh_en_ndo_set_vf_bw(struct net_device *netdev, int vf_id, int min_tx_rate, int max_tx_rate)
{
    return 0;
}

int zxdh_en_ndo_get_vf_config(struct net_device *netdev, int vf_idx, struct ifla_vf_info *ivi)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_vf_item *vf_item = NULL;
    uint8_t link_up = 0;

    vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_idx);
    if (IS_ERR_OR_NULL(vf_item))
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to get vf_item, vf_idx:%d\n", vf_idx);
        return PTR_ERR(vf_item);
    }

    ivi->vf = vf_idx;

    ether_addr_copy(ivi->mac, vf_item->mac);

#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
    ivi->max_tx_rate = vf_item->max_tx_rate;
    ivi->min_tx_rate = vf_item->min_tx_rate;
#else
    ivi->tx_rate = vf_item->max_tx_rate;
#endif

    ivi->vlan = vf_item->vlan;
    ivi->qos = vf_item->qos;
    ivi->vlan_proto = htons(vf_item->vlan_proto);

#ifdef HAVE_NDO_SET_VF_LINK_STATE
    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA) {
        link_up = en_dev->ops->get_vf_link_info(en_dev->parent, vf_idx);
        if (link_up)
            ivi->linkstate = IFLA_VF_LINK_STATE_ENABLE;
        else
            ivi->linkstate = IFLA_VF_LINK_STATE_DISABLE;
    }
    else
    {
        if (vf_item->link_forced == false)
        {
            ivi->linkstate = IFLA_VF_LINK_STATE_AUTO;
        }
        else if (vf_item->link_up == true)
        {
            ivi->linkstate = IFLA_VF_LINK_STATE_ENABLE;
        }
        else
        {
            ivi->linkstate = IFLA_VF_LINK_STATE_DISABLE;
        }
    }
#endif

#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
    ivi->spoofchk = vf_item->spoofchk;
#endif

#ifdef HAVE_NDO_SET_VF_TRUST
    ivi->trusted = vf_item->trusted;
#endif

    return 0;
}

int zxdh_en_ndo_set_vf_spoofchk(struct net_device *netdev, int vf_idx, bool enable)
{
    int ret = 0;
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_vf_item *vf_item = NULL;
    DPP_PF_INFO_T pf_info = {0};

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA) {
        LOG_ERR_DEV(en_dev->parent, "RDMA dev can not open spoofchk, vf_idx:%d\n", vf_idx);
        return -1;
    }

    vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_idx);
    if (IS_ERR_OR_NULL(vf_item))
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to get vf_item, vf_idx:%d\n", vf_idx);
        return PTR_ERR(vf_item);
    }

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = vf_item->vport;
    vf_item->spoofchk = enable;
    LOG_DEBUG_DEV(en_dev->parent, "vf %d spoof check is %s\n", vf_idx, vf_item->spoofchk? "on" : "off");
    if (vf_item->is_probed)
    {
        ret = dpp_vport_attr_set(&pf_info, SRIOV_VPORT_SPOOFCHK_EN_OFF, enable);
        if (0 != ret)
        {
            LOG_ERR_DEV(en_dev->parent, "[SET_VF_SPOOFCHK]--Failed to set vf %d spookchk %s\n", vf_idx, enable ? "on" : "off");
            return -1;
        }
    }
    return ret;
}

#ifdef HAVE_NDO_SET_VF_TRUST

int zxdh_en_ndo_set_vf_trust(struct net_device *netdev, int vf_idx, bool setting)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_vf_item *vf_item = NULL;
    DPP_PF_INFO_T pf_info = {0};

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_idx);
    if (IS_ERR_OR_NULL(vf_item))
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to get vf_item, vf_idx:%d\n", vf_idx);
        return PTR_ERR(vf_item);
    }

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = vf_item->vport;
    vf_item->trusted = setting;
    LOG_DEBUG_DEV(en_dev->parent, "VF %u is now %strusted\n", vf_idx, setting ? "" : "un");
    if (vf_item->is_probed && !vf_item->trusted)
    {
        LOG_DEBUG_DEV(en_dev->parent, "vport[0x%x] promisc and allmulti off\n", vf_item->vport);
        dpp_vport_uc_promisc_set(&pf_info, false);
        dpp_vport_promisc_en_set(&pf_info, false);
        dpp_vport_mc_promisc_set(&pf_info, false);
    } else if (vf_item->is_probed && vf_item->trusted) {
        LOG_DEBUG_DEV(en_dev->parent, "vport[0x%x] promisc and allmulti recovery, promisc=%d, mc_promisc=%d\n",
            vf_item->vport, vf_item->promisc, vf_item->mc_promisc);
        dpp_vport_uc_promisc_set(&pf_info, vf_item->promisc);
        dpp_vport_promisc_en_set(&pf_info, vf_item->promisc);
        dpp_vport_mc_promisc_set(&pf_info, vf_item->promisc ? true:vf_item->mc_promisc);
    }

    return 0;
}
#endif

int zxdh_en_ndo_set_tx_maxrate(struct net_device *netdev, int qid, uint32_t max_rate)
{
    int rtn = 0;
    zxdh_plcr_rate_limit_paras rate_limit_paras;
    struct zxdh_en_priv   *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev  = &en_priv->edev;
    struct dh_core_dev    *dh_dev  = en_dev->parent;
    struct zxdh_pf_device *pf_dev  = dh_core_priv(dh_dev->parent);

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    PLCR_FUNC_DBG_ENTER();

    /*1. 入参检测：队列号不能超过vf下实际的队列数*/
    if (qid >= en_dev->eth_config.num_txq)
    {
        LOG_ERR_DEV(dh_dev, "zxdh_en_ndo_set_tx_maxrate : invalid parameter qid=%d\n", qid);
        return -EINVAL;
    }
#if 0
    if (!en_dev->link_up)
    {
        LOG_ERR("[EN SET TX MAXRATE]--PF is not link up.\n");
        return -EINVAL;
    }
    link_speed = en_dev->link_speed;
#endif

    rate_limit_paras.req_type  = E_RATE_LIMIT_REQ_QUEUE_BYTE;
    rate_limit_paras.direction = E_RATE_LIMIT_TX;
    rate_limit_paras.mode      = E_RATE_LIMIT_BYTE ;
    rate_limit_paras.max_rate  = max_rate;
    rate_limit_paras.min_rate  = 0;
    rate_limit_paras.queue_id  = qid;
    rate_limit_paras.vf_idx    = PLCR_INVALID_PARAM;
    rate_limit_paras.vfid      = PLCR_INVALID_PARAM;
    rate_limit_paras.group_id  = PLCR_INVALID_PARAM;

    rtn = zxdh_plcr_unified_set_rate_limit(pf_dev, &rate_limit_paras);
    PLCR_COMM_ASSERT(rtn);

    PLCR_LOG_INFO_DEV(dh_dev, "The maxrate of tx-%d has been set to %dMbit/s\n", qid, max_rate);

    //私有配置命令
    //引入vqm vf限速，提高小流量限速精度;
    //条件1，4G以内满足配置能满足vqm限速周期全局共享,
    //条件2，每次配置新值需要将vqm vf限速清空再做配置
    rtn = zxdh_vqm_vf_set_rate_limit(pf_dev, qid, 0);
    PLCR_COMM_ASSERT(rtn);
    if (max_rate < 4000)
    {
        rtn = zxdh_vqm_vf_set_rate_limit(pf_dev, qid, max_rate);
        PLCR_COMM_ASSERT(rtn);

        PLCR_LOG_INFO_DEV(dh_dev, "The Rate of VF item:%d has been set to: Max Tx Rate: %dMbit/s in vqm\n",
                        qid, max_rate);
    }
    return rtn;
}

/**-------------------------------------------------------------------------------------------------------------------@n
 * 功能详述:
 *     - zxdh_en_ndo_set_vf_rate函数属于接口函数, 其功能是:
 *     - 设置vf端口发送方向，最大速率和最小保证速率
 *     - 该接口会挂接到内核的钩子上，函数声明是固定的
 *
 * 基于plcr的端口限速背景：
 *     - 1.一级flowid与vqm的2K个（接收和发送）队列是一一映射的
 *     - 2.二级flow id与vf num的映射关系
 *         端口限速，需要将vf下的发送队列（即一级flow id）映射到二级flowid
 *         二级flow id的资源是4K，dpu限制vf数量是1K，即二级flow id数量 > vf数量
 *         所以规定固定的映射关系：二级flow id前1K <---> 与1K个vf（发送）一一对应
 *         下面的链接整理了pf下vf转换成全局vf（0-1023）的原理
 *     - 3.vf限速的设置
 *         项目对vf提出了最小保证带宽的需求；
 *         二级CAR的限速模板使用：双速率，三色算法，色敏模式
 *     - 4.创建vf的其它考虑
 *         参考mlx的做法，vf创建之后，默认关联到vf组0（注意：>>>>>>>>先交付vf端口限速的需求，这一步可以暂时不实现<<<<<<<<）；
 *         vf创建之后，用户设置限速才会调用到这里，用户不设置限速，vf（二级flow id）就不用关联限速模板
 *
 * 参数概述:
 *     - netdev      : 网络设备结构体指针
 *     - vf_id       ：pf内vf的编号（从0开始）
 *     - min_tx_rate : 最小保证速率
 *     - max_tx_rate : 最大速率
 *     - 返回值类型是INT32, 含义是: 错误码，正确时为S_OK
 *
 * 引用(类变量,外部变量,接口函数):
 *     - 无
 *
 * 注意：该函数挂接到pf的钩子上，只在pf下执行
 *--------------------------------------------------------------------------------------------------------------------*/
int zxdh_en_ndo_set_vf_rate(struct net_device *netdev, int vf_id, int min_tx_rate, int max_tx_rate)
{
    int rtn;
    zxdh_plcr_rate_limit_paras rate_limit_paras;

    struct zxdh_en_priv   *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev  = &en_priv->edev;
    struct dh_core_dev    *dh_dev  = en_dev->parent;
    struct zxdh_pf_device *pf_dev  = dh_core_priv(dh_dev->parent);
    struct zxdh_vf_item *vf_item = NULL;

    ZXDH_DEVICE_STATE_CHECK_RTN(en_dev);
    PLCR_FUNC_DBG_ENTER();

    rate_limit_paras.req_type  = E_RATE_LIMIT_REQ_VF_BYTE;
    rate_limit_paras.direction = E_RATE_LIMIT_TX;
    rate_limit_paras.mode      = E_RATE_LIMIT_BYTE ;
    rate_limit_paras.max_rate  = max_tx_rate;
    rate_limit_paras.min_rate  = min_tx_rate;
    rate_limit_paras.queue_id  = PLCR_INVALID_PARAM;
    rate_limit_paras.vf_idx    = vf_id;
    rate_limit_paras.vfid      = PLCR_INVALID_PARAM;
    rate_limit_paras.group_id  = PLCR_INVALID_PARAM;

    vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_id);
    if (IS_ERR_OR_NULL(vf_item))
    {
        LOG_ERR_DEV(dh_dev, "Failed to get vf_item, vf_idx:%d\n", vf_id);
        return PTR_ERR(vf_item);
    }
    vf_item->min_tx_rate = min_tx_rate;
    vf_item->max_tx_rate = max_tx_rate;

    if (!en_dev->ops->get_vf_is_probe(en_dev->parent, vf_id))
    {
        LOG_INFO_DEV(dh_dev, "zxdh_en_ndo_set_vf_rate, vf %d is not probed\n", vf_id);
        return 0;
    }

    rtn = zxdh_plcr_unified_set_rate_limit(pf_dev, &rate_limit_paras);
        PLCR_COMM_ASSERT(rtn);

    PLCR_LOG_INFO_DEV(dh_dev, "The Rate of VF%d has been set to: Min Tx Rate: %dMbit/s, Max Tx Rate: %dMbit/s\n",
                        vf_id, min_tx_rate, max_tx_rate);

    // vf_item = en_dev->ops->get_vf_item(en_dev->parent, vf_id);
    // if (IS_ERR_OR_NULL(vf_item))
    // {
    //     LOG_ERR("Failed to get vf_item, vf_idx:%d\n", vf_id);
    //     return PTR_ERR(vf_item);
    // }
    // vf_item->min_tx_rate = min_tx_rate;
    // vf_item->max_tx_rate = max_tx_rate;

    //引入vqm vf限速，提高小流量限速精度;
    //条件1，4G以内满足配置能满足vqm限速周期全局共享,
    //条件2，每次配置新值需要将vqm vf限速清空再做配置
    rtn = zxdh_vqm_vf_set_rate_limit(pf_dev, vf_item->vport, 0);
    PLCR_COMM_ASSERT(rtn);
    if (max_tx_rate < 4000)
    {
        rtn = zxdh_vqm_vf_set_rate_limit(pf_dev, vf_item->vport, max_tx_rate);
        PLCR_COMM_ASSERT(rtn);

        PLCR_LOG_INFO_DEV(dh_dev, "The Rate of VF item:%d has been set to: Max Tx Rate: %dMbit/s in nic\n",
                        vf_item->vport, max_tx_rate);
    }

    return rtn;
}

const struct net_device_ops zxdh_netdev_ops = {
    .ndo_open = zxdh_en_open,
    .ndo_stop = zxdh_en_close,
    .ndo_start_xmit = zxdh_en_xmit,

#if defined(HAVE_NDO_GET_STATS64) || defined(HAVE_VOID_NDO_GET_STATS64)
    .ndo_get_stats64 = zxdh_en_get_netdev_stats_struct,
#else
    .ndo_get_stats = zxdh_en_get_netdev_stats_struct,
#endif
    .ndo_set_rx_mode = zxdh_en_set_rx_mode,
    .ndo_validate_addr = eth_validate_addr,
    .ndo_set_mac_address = zxdh_en_set_mac,

#ifdef HAVE_RHEL7_EXTENDED_MIN_MAX_MTU
    .extended.ndo_change_mtu = zxdh_en_change_mtu,
#else
    .ndo_change_mtu = zxdh_en_change_mtu,
#endif /* HAVE_RHEL7_EXTENDED_MIN_MAX_MTU */

#if defined(ZXDH_ADAPT_REDHAT_9_2) || defined(USE_PRIV_IOCTL) || defined(ZXDH_ADAPT_REDHAT_9_1)
    .ndo_eth_ioctl = zxdh_en_ioctl,
    .ndo_siocdevprivate = zxdh_en_private_ioctl,
#else
    .ndo_do_ioctl = zxdh_en_ioctl,
#endif

#ifdef ZXDH_CONFIG_SPECIAL_SQ_EN
    .ndo_select_queue = zxdh_en_select_queue,
#endif
#ifdef ZXDH_PLCR_OPEN
#ifdef CGS_V5_693
    .extended.ndo_set_tx_maxrate = zxdh_en_ndo_set_tx_maxrate,
#else
    .ndo_set_tx_maxrate = zxdh_en_ndo_set_tx_maxrate,
#endif
#endif
    .ndo_tx_timeout = zxdh_en_tx_timeout,

#ifdef HAVE_VLAN_RX_REGISTER
    .ndo_vlan_rx_register = zxdh_en_vlan_rx_register,
#endif
    .ndo_vlan_rx_add_vid = zxdh_en_vlan_rx_add_vid,
    .ndo_vlan_rx_kill_vid = zxdh_en_vlan_rx_kill_vid,
#ifndef CGS_V5_693
    .ndo_bpf = zxdh_en_xdp,
    .ndo_xdp_xmit = zxdh_en_xdp_xmit,
#endif
#ifdef CONFIG_NET_POLL_CONTROLLER
    .ndo_poll_controller = zxdh_en_netpoll,
#endif

#ifdef HAVE_SETUP_TC
#ifdef HAVE_RHEL7_NETDEV_OPS_EXT_NDO_SETUP_TC
    .extended.ndo_setup_tc_rh = __zxdh_en_setup_tc,
#else
#ifdef NETIF_F_HW_TC
    .ndo_setup_tc = __zxdh_en_setup_tc,
#else
    .ndo_setup_tc = zxdh_en_setup_tc,
#endif /* NETIF_F_HW_TC */
#endif /* HAVE_RHEL7_NETDEV_OPS_EXT_NDO_SETUP_TC */
#endif /* HAVE_SETUP_TC */

#if defined(HAVE_RHEL7_NET_DEVICE_OPS_EXT)
    .ndo_size = sizeof(const struct net_device_ops),
#endif

#ifdef IFLA_VF_MAX
    .ndo_set_vf_mac = zxdh_en_ndo_set_vf_mac,
#ifdef HAVE_RHEL7_NETDEV_OPS_EXT_NDO_SET_VF_VLAN
    .extended.ndo_set_vf_vlan = zxdh_en_ndo_set_vf_port_vlan,
#else
    .ndo_set_vf_vlan = zxdh_en_ndo_set_vf_port_vlan,
#endif
#ifdef HAVE_NDO_SET_VF_MIN_MAX_TX_RATE
#ifdef ZXDH_PLCR_OPEN
    .ndo_set_vf_rate = zxdh_en_ndo_set_vf_rate,
#else
    .ndo_set_vf_rate = zxdh_en_ndo_set_vf_bw,
#endif
#else
    .ndo_set_vf_rate = zxdh_en_ndo_set_vf_bw,
#endif
    .ndo_get_vf_config = zxdh_en_ndo_get_vf_config,
#ifdef HAVE_VF_SPOOFCHK_CONFIGURE
    .ndo_set_vf_spoofchk = zxdh_en_ndo_set_vf_spoofchk,
#endif
#ifdef HAVE_NDO_SET_VF_TRUST
#ifdef HAVE_RHEL7_NET_DEVICE_OPS_EXT
    .extended.ndo_set_vf_trust = zxdh_en_ndo_set_vf_trust,
#else
    .ndo_set_vf_trust = zxdh_en_ndo_set_vf_trust,
#endif /* HAVE_RHEL7_NET_DEVICE_OPS_EXT */
#endif /* HAVE_NDO_SET_VF_TRUST */
#endif /* IFLA_VF_MAX */

#ifdef HAVE_UDP_ENC_RX_OFFLOAD
#ifdef HAVE_VXLAN_RX_OFFLOAD
#if IS_ENABLED(CONFIG_VXLAN)
#ifndef CGS_V5_693
    .ndo_add_vxlan_port = zxdh_en_add_vxlan_port,
    .ndo_del_vxlan_port = zxdh_en_del_vxlan_port,
#endif
#endif
#endif /* HAVE_VXLAN_RX_OFFLOAD */

#ifdef HAVE_GENEVE_RX_OFFLOAD
#if IS_ENABLED(CONFIG_GENEVE)
#ifndef CGS_V5_693
    .ndo_add_geneve_port = zxdh_en_add_geneve_port,
    .ndo_del_geneve_port = zxdh_en_del_geneve_port,
#endif
#endif
#endif /* HAVE_GENEVE_RX_OFFLOAD */
#endif /* HAVE_UDP_ENC_RX_OFFLOAD */

    .ndo_set_features = zxdh_en_set_features,

#ifdef HAVE_FDB_OPS
    .ndo_fdb_add = zxdh_en_ndo_fdb_add,
    .ndo_fdb_del = zxdh_en_ndo_fdb_del,
#ifdef HAVE_NDO_FEATURES_CHECK
    .ndo_features_check = zxdh_en_features_check,
#endif /* HAVE_NDO_FEATURES_CHECK */
#ifdef HAVE_BRIDGE_ATTRIBS
    .ndo_bridge_getlink = zxdh_en_ndo_bridge_getlink,
    .ndo_bridge_setlink = zxdh_en_ndo_bridge_setlink,
#endif /* HAVE_BRIDGE_ATTRIBS */
#endif /* HAVE_FDB_OPS */

#ifdef HAVE_RHEL6_NET_DEVICE_OPS_EXT
};

/* RHEL6 keeps these operations in a separate structure */
static const struct net_device_ops_ext zxdh_netdev_ops_ext =
{
    .size = sizeof(struct net_device_ops_ext),
#endif /* HAVE_RHEL6_NET_DEVICE_OPS_EXT */

#ifdef HAVE_NDO_SET_FEATURES
    .ndo_set_features = zxdh_en_set_features,
#endif /* HAVE_NDO_SET_FEATURES */

#ifdef HAVE_NDO_SET_VF_LINK_STATE
    .ndo_set_vf_link_state = zxdh_en_ndo_set_vf_link_state,
#endif
};

static void priv_flags_init(struct zxdh_en_priv *priv)
{
    priv->edev.pflags = 0;

    priv->edev.pflags |= BIT(ZXDH_PFLAG_ENABLE_LLDP); /* LLDP默认为开 */
    priv->edev.pflags |= BIT(ZXDH_PFLAG_HARDWARE_BOND_PRIMARY);
    priv->edev.pflags |= BIT(ZXDH_PFLAG_PCIE_AER_CPL_TIMEOUT);
}

static int32_t get_max_num_qs(struct zxdh_en_container *en_con)
{
    return en_con->ops->get_qpairs(en_con->parent);
}

int32_t zxdh_priv_init(struct zxdh_en_priv *priv, struct net_device *netdev)
{
    struct zxdh_en_device *en_dev = &priv->edev;

    mutex_init(&priv->lock);
    priv_flags_init(priv);
    en_dev->msglevel =  NETIF_MSG_LINK;

    /* 优先级4，只支持MAGIC WAKE */
    en_dev->wol_support = WAKE_MAGIC;
    en_dev->wolopts = WAKE_MAGIC;

    return 0 ;
}

struct net_device *zxdh_create_netdev(struct zxdh_en_container *en_con, uint16_t max_vq_pairs)
{
    struct net_device *netdev = NULL;
    struct zxdh_en_priv *en_priv = NULL;
    struct dh_core_dev *dh_dev = en_con->parent;

    netdev = alloc_etherdev_mqs(sizeof(struct zxdh_en_priv), max_vq_pairs, max_vq_pairs);
    if (unlikely(netdev == NULL))
    {
        LOG_ERR_DEV(dh_dev, "alloc_etherdev_mqs() failed\n");
        return NULL;
    }

    en_priv = netdev_priv(netdev);

    en_priv->edev.parent = dh_dev;
    en_priv->edev.ops = en_con->ops;
    en_priv->edev.netdev = netdev;

    zxdh_priv_init(en_priv, netdev);

    netif_carrier_off(netdev);
    netif_tx_disable(netdev);
#ifdef CGS_V5_693
    /* 3.10 内核中 devlink_net 可能返回 NULL，直接使用 init_net */
    dev_net_set(netdev, &init_net);
#else
    dev_net_set(netdev, dh_core_net(dh_dev));
#endif

#ifdef CGS_V5_693
    /* 3.10 内核要求 watchdog_timeo 必须设置为非零值 */
    netdev->watchdog_timeo = 5 * HZ;
#endif

    return netdev;
}

void zxdh_netdev_features_init(struct net_device *netdev)
{
    netdev->features |= NETIF_F_RXCSUM |
                        NETIF_F_HW_CSUM |
                        NETIF_F_TSO |
                        NETIF_F_SG |
                        NETIF_F_GSO |
                        // NETIF_F_LRO |
                        NETIF_F_TSO6 |
                        NETIF_F_GRO |
                        NETIF_F_HW_VLAN_STAG_FILTER |
                        NETIF_F_GSO_UDP_TUNNEL |
                        NETIF_F_GSO_UDP_TUNNEL_CSUM |
                        NETIF_F_RXHASH |
                        NETIF_F_GSO_IPXIP4 |
                        NETIF_F_GSO_GRE |
                        NETIF_F_GSO_GRE_CSUM;

    netdev->hw_features |= NETIF_F_RXCSUM |
                        NETIF_F_HW_CSUM |
                        NETIF_F_TSO |
                        NETIF_F_SG |
                        NETIF_F_GSO |
                        NETIF_F_LRO |
                        NETIF_F_TSO6 |
                        NETIF_F_GRO |
                        NETIF_F_HW_VLAN_STAG_FILTER |
                        NETIF_F_HW_VLAN_CTAG_FILTER |
                        NETIF_F_GSO_UDP_TUNNEL |
                        NETIF_F_GSO_UDP_TUNNEL_CSUM |
                        NETIF_F_HW_VLAN_CTAG_RX     |
                        NETIF_F_HW_VLAN_CTAG_TX     |
                        NETIF_F_HW_VLAN_STAG_RX     |
                        NETIF_F_HW_VLAN_STAG_TX     |
#ifdef CGS_V5_693
                        /* 3.10 内核 TC offload 框架不完整，不启用 HW TC */
#else
                        NETIF_F_HW_TC  |
#endif
                        NETIF_F_RXHASH |
                        NETIF_F_NTUPLE |
                        NETIF_F_GSO_IPXIP4 |
                        NETIF_F_GSO_GRE |
                        NETIF_F_GSO_GRE_CSUM;

    netdev->hw_enc_features |= NETIF_F_RXCSUM  |
                               NETIF_F_HW_CSUM |
                               NETIF_F_GSO_UDP_TUNNEL_CSUM |
                               NETIF_F_GSO_UDP_TUNNEL      |
                               NETIF_F_TSO                 |
                               NETIF_F_TSO6 |
                               NETIF_F_GSO_IPXIP4 |
                               NETIF_F_GSO_GRE |
                               NETIF_F_GSO_GRE_CSUM;

    netdev->vlan_features = NETIF_F_RXCSUM |
                            NETIF_F_HW_CSUM |
                            NETIF_F_GRO |
                            NETIF_F_TSO |
                            NETIF_F_SG |
                            NETIF_F_TSO6 |
                            NETIF_F_GSO_UDP_TUNNEL |
                            NETIF_F_GSO_UDP_TUNNEL_CSUM |
                            NETIF_F_LRO |
                            NETIF_F_RXHASH|
                            NETIF_F_GSO_IPXIP4 |
                            NETIF_F_GSO_GRE |
                            NETIF_F_GSO_GRE_CSUM;
    return;
}

extern const struct xfrmdev_ops zxdh_xfrmdev_ops;

static void zxdh_build_nic_netdev(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct dh_core_dev *dh_dev = en_priv->edev.parent;

    SET_NETDEV_DEV(netdev, &dh_dev->parent->pdev->dev);

    netdev->netdev_ops = &zxdh_netdev_ops;

#ifdef ZXDH_SEC
    /*内核 sec相关*/
    netdev->features |=NETIF_F_HW_ESP;
    netdev->xfrmdev_ops = &zxdh_xfrmdev_ops;
#endif

#ifdef HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT
    zxdh_en_set_ethtool_ops_ext(netdev);
#else
    zxdh_en_set_ethtool_ops(netdev);
#endif /* HAVE_RHEL6_ETHTOOL_OPS_EXT_STRUCT */

    zxdh_netdev_features_init(netdev);

    if (en_priv->edev.ops->get_dev_type(dh_dev) == ZXDH_DEV_ROCE_RDMA) {
        netdev->hw_features = netdev->hw_features & ~ (NETIF_F_HW_VLAN_CTAG_RX|NETIF_F_HW_VLAN_CTAG_TX|NETIF_F_NTUPLE);
    }
}

int32_t zxdh_en_bond_get_mac(struct net_device *netdev, uint8_t pannel_id, uint8_t *mac)
{
    int32_t ret = 0;
    union zxdh_msg *msg = NULL;
    struct zxdh_en_priv *en_priv  = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -1;
    }

    msg->payload.hdr_to_agt.op_code = AGENT_FLASH_MAC_READ;
    msg->payload.flash_read_msg.index = pannel_id;

    ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_FLASH, msg, msg, &para);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "en_dev->ops->msg_send_cmd failed: %d\n", ret);
        kfree(msg);
        return ret;
    }

    ether_addr_copy(mac, msg->reps.flash_mac_read_msg.mac);
    kfree(msg);
    return ret;
}

int32_t zxdh_mac_addr_init(struct net_device *netdev)
{
    uint8_t mac[6] = {0};
    uint8_t pannel_id = 0;
    int32_t ret = 0;
    struct zxdh_en_priv *en_priv  = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        pannel_id = en_dev->pannel_id;
        ret = zxdh_en_bond_get_mac(netdev, pannel_id, mac);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_en_bond_mac_get failed: %d\n", ret);
        }
    }
    else
    {
        en_dev->ops->get_mac(en_dev->parent, mac);
    }

    if (!is_valid_ether_addr(mac))
    {
        get_random_bytes(mac, 6);
        mac[0] &= 0xfe;
        LOG_INFO_DEV(en_dev->parent, "set random mac %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    }
    LOG_DEBUG_DEV(en_dev->parent, "set mac %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    zxdh_netdev_addr_set(netdev, mac);

    return ret;
}

int32_t zxdh_status_init(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (en_dev->ops->if_init(en_dev->parent))
    {
        zxdh_vp_reset(netdev);
    }

    /* Disable VQ/configuration callbacks. */
    zxdh_vp_disable_cbs(netdev);

    zxdh_add_status(netdev, ZXDH_CONFIG_S_ACKNOWLEDGE);

    zxdh_add_status(netdev, ZXDH_CONFIG_S_DRIVER);

    /* fix features, not set features*/
    zxdh_pf_features_init(netdev);

    might_sleep();
    zxdh_add_status(netdev, ZXDH_CONFIG_S_FEATURES_OK);
    if (!zxdh_has_status(netdev, ZXDH_CONFIG_S_FEATURES_OK))
    {
        LOG_ERR_DEV(en_dev->parent, "device refuses features ok\n");
        return -ENODEV;
    }

    return 0;
}

void zxdh_device_ready(struct net_device *netdev)
{
    zxdh_vp_enable_cbs(netdev);

    zxdh_add_status(netdev, ZXDH_CONFIG_S_DRIVER_OK);
}

void zxdh_link_state_notify_kernel(struct net_device *netdev)
{
    struct zxdh_en_priv   *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev  = &en_priv->edev;
    uint8_t link_up = 0;

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA)
    {
        en_dev->ops->get_link_info_from_vqm(en_dev->parent, &link_up);
        en_dev->link_up = link_up;
    }

    if(en_dev->link_up)
    {
        netif_carrier_off(netdev);
        udelay(10);
        netif_carrier_on(netdev);
    }
    else
    {
        netif_carrier_on(netdev);
        udelay(10);
        netif_carrier_off(netdev);
    }
}

int32_t aux_get_bond_attrs(struct zxdh_en_device *en_dev, struct zxdh_lag_attrs *attr)
{
    *attr = (struct zxdh_lag_attrs)
    {
        .pannel_id = en_dev->pannel_id,
        .vport = en_dev->vport,
        .slot_id = en_dev->slot_id,
        .qid[0] = en_dev->phy_index[0],
        .qid[1] = en_dev->phy_index[1],
        .pcie_id = en_dev->pcie_id,
        .phy_port = en_dev->phy_port,
    };

    LOG_DEBUG_DEV(en_dev->parent, "bond pf: pannel %hu, vport 0x%hx, phy_qid[0] %u, phy_qid[1] %u, pcie id 0x%x\n",
            attr->pannel_id, attr->vport, attr->qid[0], attr->qid[1], attr->pcie_id);

    return 0;
}

void aux_set_netdev_name(struct net_device *netdev, uint16_t pannel_id)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct dh_core_dev *dh_dev = en_dev->parent;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dh_dev->parent);

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        netdev->dev_port = pannel_id + 1;
    }
    else if ((pf_dev->board_type == DH_STDA) || (pf_dev->board_type == DH_STD_E312S) \
            || (pf_dev->board_type == DH_STD_E312S_D))
    {
        if(!en_dev->is_multi_ep)
            return;
        /* 将vf_index插入到bit0-bit7，将panel_id插入到bit8 */
        netdev->dev_id = ((en_dev->pcie_id & 0xFF) | ((en_dev->panel_id & 0x01) << 8)) & ~(1 << 9);
        /* 将is_pf插入到bit9, 0(VF)/1(PF) */
        netdev->dev_id |= ((en_dev->pcie_id & (1 << 11)) >> 2);
        LOG_DEBUG_DEV(dh_dev, "%s board_type: %d,netdev->dev_id: %#x\n", __func__, pf_dev->board_type, netdev->dev_id);
    }
}

int32_t ptp_set_pf_uplink_vfid(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    en_dev->vf_1588_call_np_num = PTP_PORT_VFID_SET;
    LOG_DEBUG_DEV(en_dev->parent, "%s vport: 0x%x, IS_PF: %d VFID %u", __func__, en_dev->vport, IS_PF(en_dev->vport), VQM_VFID(en_dev->vport));
    if (IS_PF(en_dev->vport))
    {
        ret = dpp_ptp_port_vfid_set(&pf_info, VQM_VFID(en_dev->vport));
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_ptp_port_vfid_set failed!!!\n");
            return -1;
        }
    }
    return 0;
}

int32_t ptp_set_pf_tc_enable(struct zxdh_en_device *en_dev, uint32_t tc_enable)
{
    int32_t ret = 0;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    en_dev->ptp_tc_enable_opt = tc_enable;
    LOG_DEBUG_DEV(en_dev->parent, "ptp_tc_enable_opt = %u\n", en_dev->ptp_tc_enable_opt);

    en_dev->vf_1588_call_np_num = PTP_TC_ENABLE_SET;

    if (IS_PF(en_dev->vport))
    {
        ret = dpp_ptp_tc_enable_set(&pf_info, en_dev->ptp_tc_enable_opt);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "dpp_ptp_tc_enable_set failed!!!\n");
            return -1;
        }
    }
    return 0;
}


int32_t zxdh_en_mtu_init(struct net_device *netdev)
{
#ifdef CGS_V5_693
    /* 3.10 内核不支持 min_mtu/max_mtu 成员，跳过赋值 */
#else
    netdev->min_mtu = ETH_MIN_MTU;
    netdev->max_mtu = ZXDH_MAX_MTU;
#endif

    return zxdh_en_config_mtu_to_np(netdev, ZXDH_DEFAULT_MTU);
}

void zxdh_cap_pkt_init(struct zxdh_en_device *en_dev)
{
    en_dev->pkt_save_file.log_file = NULL;
    en_dev->pkt_save_file.enable_pkt_num_mode = 0;
    en_dev->pkt_save_file.pkt_file_size = 0;
    en_dev->pkt_save_file.pkt_set_count = 0;
    en_dev->pkt_save_file.is_stop = 0;
    en_dev->pkt_save_file.pkt_rbuf_idx = 0;
    en_dev->pkt_save_file.pkt_ubuf_idx = 0;
    en_dev->pkt_save_file.pkt_cur_num = 0;
    memset(en_dev->pkt_save_file.file_path, 0, sizeof(en_dev->pkt_save_file.file_path));
    en_dev->pkt_save_file.file_pos = 0;
    en_dev->pkt_dev_flag = 0;
    en_dev->pkt_dev_speed = ZXDH_PKT_INIT_SPEED;
    en_dev->pkt_file_num = 0;
    en_dev->pkt_cap_switch = 0;
    en_dev->pkt_save_file_flag = 0;
    en_dev->pkt_addr_marked = 0;
}

/* Started by AICoder, pid:93575f5d4cs5818140a70aa4c06dfc4f5bd055e2 */
int32_t zxdh_hash_id_init(struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_UPF)
    {
        en_dev->hash_search_idx = 2;
    }
    else if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_NE0)
    {
        en_dev->hash_search_idx = 0;
    }
    else if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_NE1)
    {
        en_dev->hash_search_idx = 1;
    }
    else if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_SRIOV)
    {
        en_dev->hash_search_idx = 2;
    }
    else if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA)
    {
        en_dev->hash_search_idx = 3;
    }
    else if (!en_dev->ops->is_bond(en_dev->parent))
    {
        ret = zxdh_hash_id_get(en_dev);
        if (ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_hash_id_get failed: %d\n", ret);
            return -1;
        }
    }

    return 0;
}
/* Ended by AICoder, pid:93575f5d4cs5818140a70aa4c06dfc4f5bd055e2 */

uint32_t pcie_id2vfid_for_pf(uint16_t pcie_id) //仅支持PF
{
    uint8_t ep_id = (pcie_id >> 12) & 0x7;
    uint8_t pf_id = (pcie_id >> 8) & 0x7;

    return (ep_id * 8 + pf_id + 1152);
}

void zxdh_cfg_vqm_vf_fc_kbps(struct zxdh_en_device *en_dev, uint32_t pf_fc_val)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_pf_device *pf_dev = dh_core_priv(en_dev->parent->parent);
    uint32_t vqm_vfid = 0xffff;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return;
    }
    vqm_vfid = pcie_id2vfid_for_pf(pf_dev->pcie_id);
    if (vqm_vfid >= 0xffff)
    {
        LOG_ERR_DEV(en_dev->parent, "vfid(%u) is invalid!\n", vqm_vfid);
        kfree(msg);
        return;
    }
    msg->vqm_msg.vqm_vfid = (uint16_t)vqm_vfid;
    msg->vqm_msg.opcode = OPCODE_SET;
    msg->vqm_msg.cmd = VQM_VF_FC_CMD;
    msg->vqm_msg.vqm_vf_fc.pps = 0;
    msg->vqm_msg.vqm_vf_fc.kbps = pf_fc_val;
    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_CFG_VQM, msg, msg, &para);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "send vf fc msg to riscv failed!\n");
    }
    else
    {
        if (msg->vqm_reps.check_result != 0xaa)
        {
            LOG_ERR_DEV(en_dev->parent, "cfg vfid(%u) rate %ukbps failed!\n", vqm_vfid, pf_fc_val);
        }
    }
    kfree(msg);
}

void zxdh_set_pf_fc(struct zxdh_en_device *en_dev)
{
    uint32_t pf_fc_val = 0;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF) // 非PF则跳过
    {
        return;
    }

    if (en_dev->ops->is_pf_rate_enable(en_dev->parent, &pf_fc_val))
    {
        LOG_INFO_DEV(en_dev->parent, "pf rate: %uMbps\n", pf_fc_val);
        if (pf_fc_val != 0)
        {
            zxdh_cfg_vqm_vf_fc_kbps(en_dev, pf_fc_val * 1000);
        }
    }
    else
    {
        LOG_INFO_DEV(en_dev->parent, "pf rate: disable\n");
    }
}

void zxdh_del_pf_fc(struct zxdh_en_device *en_dev)
{
    uint32_t pf_fc_val = 0;

    if (en_dev->quick_remove)
        return;

    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF) // 非PF则跳过
    {
        return;
    }
    if (en_dev->ops->is_pf_rate_enable(en_dev->parent, &pf_fc_val))
    {
        if (pf_fc_val != 0)
        {
            zxdh_cfg_vqm_vf_fc_kbps(en_dev, 0);
        }
    }
}

static void enable_1588_init(struct zxdh_en_device *en_dev)
{
    union zxdh_msg *msg = NULL;
    int32_t ret = 0;
    struct zxdh_bar_extra_para para = {0};
    DPP_PF_INFO_T dpp_pf_info = {
        .slot = en_dev->slot_id,
        .vport = en_dev->vport,
    };

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    en_dev->enable_1588 = false;
    if (en_dev->ops->get_coredev_type(en_dev->parent) != DH_COREDEV_PF)
    {
        msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
        if (msg == NULL)
        {
            LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
            return;
        }

        msg->payload.vf_1588_enable.proc_cmd = ZXDH_VF_1588_ENABLE_SET;
        msg->payload.hdr.op_code = ZXDH_VF_1588_ENABLE;
        msg->payload.hdr.vport = en_dev->vport;
        msg->payload.hdr.pcie_id = en_dev->pcie_id;
        msg->payload.vf_1588_enable.enable_1588_vf = (uint32_t)false;
        ret = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
        if(ret != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_send_command_to_pf failed: %d\n", ret);
            kfree(msg);
            return;
        }

        kfree(msg);
        return;
    }

    ret = dpp_vport_attr_set(&dpp_pf_info, SRIOV_VPORT_1588_EN, (uint32_t)false);
    if (ret != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dpp_vport_attr_set SRIOV_VPORT_1588_EN failed, ret:%d\n", ret);
        return;
    }
    return;
}

/* Must be called with intf_mutex held */
struct zxdh_en_device *dh_get_next_phys_dev(struct zxdh_en_device *en_dev)
{
	struct zxdh_en_device *res = NULL;
	struct zxdh_en_device *tmp_dev;
	uint16_t slot_id = en_dev->slot_id;
    uint16_t tmp_slot_id = 0;

	list_for_each_entry(tmp_dev, &zxdh_en_dev_list, dev_node) {
		if (tmp_dev->ops->get_coredev_type(tmp_dev->parent) != DH_COREDEV_PF)
			continue;
        tmp_slot_id = tmp_dev->slot_id;
        LOG_DEBUG("cur dev %s slot_id %d, tmp dev %s slot_id %d\n", pci_name(en_dev->ops->get_pdev(en_dev->parent)), \
        slot_id, pci_name(tmp_dev->ops->get_pdev(tmp_dev->parent)), tmp_slot_id);
		if ((en_dev != tmp_dev) && (tmp_slot_id == slot_id)) {
			res = tmp_dev;
			break;
		}
	}

	return res;
}

void zxdh_update_rdma_hwbond_master(void)
{
    struct net_device *netdev;
    struct net_device *uplink_upper;
    struct net_device *primary_netdev = NULL;
    struct zxdh_lag_dev *lag = NULL;
	struct zxdh_en_device *tmp_dev;

	list_for_each_entry(tmp_dev, &zxdh_en_dev_list, dev_node) 
    {
        netdev = tmp_dev->netdev;
        if (!netdev)
        {
            continue;
        }

        /* if netdev not hwbond */
        if (!zxdh_netdev_is_hwbond(netdev))
        {
            continue;
        }
        rcu_read_lock();
        uplink_upper = netdev_master_upper_dev_get_rcu(netdev);
        rcu_read_unlock();
        if (uplink_upper && netif_is_lag_master(uplink_upper))
        {
            // 所在的ldev是否激活且为hardware-bond?
            lag = tmp_dev->ldev;
            if (!lag) {
                LOG_INFO("%s has no lag_dev bound, skip", netdev->name);
                continue;
            }
            if (lag->is_active && lag->state == LAG_DEV_ACTIVE\
            && lag->tracker.bond_type == HARDWARE_BOND\
            && lag->upper_netdev && lag->upper_netdev == uplink_upper)
            {
                primary_netdev = lag->tracker.slaves[lag->primary_pf_idx].slave_info.netdev;
                if (!primary_netdev)
                {
                    LOG_ERR("primary_netdev is NULL for lag[%d]", lag->idx);
                    continue;
                }
                LOG_INFO("update %s with %s to rdma\n", primary_netdev->name, uplink_upper->name);
                /* set rdma dev bind netdev and port speed */
                zxdh_set_rdma_hwbond_master(primary_netdev, uplink_upper, true);
                zxdh_bond_cofig_rdma_speed(lag);
                continue;
            }
            LOG_INFO("%s-lag_dev[%d] not active or not hardware-bond\n", tmp_dev->netdev->name, lag->idx);
        }
    }

    return;
}

static int32_t zxdh_en_dev_probe(struct zxdh_auxiliary_device *adev, const struct zxdh_auxiliary_device_id *id)
{
    struct zxdh_en_container *en_container = container_of(adev, struct zxdh_en_container, adev);
    struct net_device *netdev = NULL;
    struct zxdh_en_priv *en_priv = NULL;
    struct zxdh_en_device *en_dev = NULL;
    struct zxdh_lag_attrs lag_attrs;
    int32_t err = 0;
    int32_t vqs_channel_num = 0;
    uint16_t max_vq_pairs = 0;
    uint8_t link_up = 0;
    uint8_t carrier_status = 0;
    uint64_t mcode_feature = 0;
    DPP_PF_INFO_T pf_info = {0};
    int value_to_write = vf_broadcast ? 1 : 0;
#ifdef PTP_DRIVER_INTERFACE_EN
    uint32_t phcidx = 0xff;
#endif /* PTP_DRIVER_INTERFACE_EN */

    LOG_INFO_DEV(en_container->parent, "aux level driver probe %s start\n", pci_name(en_container->parent->pdev));

    max_vq_pairs = get_max_num_qs(en_container);
    netdev = zxdh_create_netdev(en_container, max_vq_pairs);
    if (unlikely(netdev == NULL))
    {
        LOG_ERR("zxdh_create_netdev is null\n");
        err = -ENOMEM;
        goto err_create_netdev;
    }

    zxdh_build_nic_netdev(netdev);

    dev_set_drvdata(&adev->dev, netdev_priv(netdev));

    en_priv = netdev_priv(netdev);
    en_dev = &en_priv->edev;
    en_dev->dmadev = en_dev->ops->get_dma_dev(en_dev->parent);
    en_dev->channels_num = en_dev->ops->get_channels_num(en_dev->parent);
    en_dev->is_special_bond = en_dev->ops->is_special_bond(en_dev->parent);
    en_dev->ops->set_rdma_netdev(en_dev->parent, netdev);
    en_dev->curr_unicast_num = 1;
    en_dev->curr_multicast_num = 0;
    en_dev->init_comp_flag = AUX_INIT_INCOMPLETED;
    en_dev->delay_statistics_enable = 0;
    en_dev->phy_port = INVALID_PHY_PORT;
    en_dev->link_down_on_close = false;
    en_dev->time_sync_done = false;
    en_dev->ldev_manager = NULL;
    en_dev->ldev = NULL;

    en_dev->max_vq_pairs = max_vq_pairs;
#ifdef CONFIG_INET
    en_dev->local_lb_enable = false;
#endif
    en_dev->board_type = en_dev->ops->get_board_type(en_dev->parent);

    en_dev->is_lowlatency = en_dev->ops->is_lowlatency(en_dev->parent);
    mcode_feature = en_dev->ops->get_mcode_feature(en_dev->parent);
    en_dev->is_outer_l4_rxcsum_offload = (mcode_feature & TUNNEL_RX_OUTER_L4_CKSUM_FLAG) ? true : false;

    if (en_dev->is_lowlatency)
    {
        netdev->features |= NETIF_F_LRO;
#ifndef CGS_V5_693
        init_net.ipv4.sysctl_tcp_timestamps = 0;
#endif
    }

    vqs_channel_num = en_dev->ops->create_vqs_channels(en_dev->parent, en_dev);
    if (vqs_channel_num < 0)
    {
        LOG_ERR_DEV(en_dev->parent, "create_vqs_channels failed, vqs_channel_num: %d\n", vqs_channel_num);
        err = vqs_channel_num;
        goto err_create_vqs_channels;
    }

    err = dh_aux_eq_table_init(en_priv);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to alloc IRQs: %d\n", err);
        goto err_eq_table_init;
    }

    err = dh_aux_events_init(en_priv);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dh_aux_events_init failed: %d\n", err);
        goto err_events_init;
    }

    err = dh_aux_eq_table_create(en_priv);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "Failed to alloc EQs: %d\n", err);
        goto err_eq_table_create;
    }

    err = zxdh_status_init(netdev);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_status_init failed: %d\n", err);
        goto err_status_init;
    }

    en_dev->ep_bdf = en_dev->ops->get_epbdf(en_dev->parent);
    en_dev->vport = en_dev->ops->get_vport(en_dev->parent);
    en_dev->pcie_id = en_dev->ops->get_pcie_id(en_dev->parent);
    en_dev->ro_flag = en_dev->ops->get_ro_info_from_fwshrd(en_dev->parent);
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        en_dev->slot_id = en_dev->ops->get_slot_id(en_dev->parent);
    }
    LOG_DEBUG_DEV(en_dev->parent, "ep_bdf: 0x%x, vport: 0x%x, pcie_id: %d, slot_id: %d, ro_flag: %d. is_bond %d\n", en_dev->ep_bdf, \
            en_dev->vport, en_dev->pcie_id, en_dev->slot_id, en_dev->ro_flag, en_dev->ops->is_bond(en_dev->parent));
    if (!en_dev->ops->is_bond(en_dev->parent))
    {
        en_dev->is_hwbond = en_dev->ops->is_hwbond(en_dev->parent, en_dev->is_hwbond, FALSE);
        en_dev->is_rdma_aux_plug = en_dev->ops->is_rdma_aux_plug(en_dev->parent, en_dev->is_rdma_aux_plug, FALSE);
        en_dev->is_primary_port = en_dev->ops->is_primary_port(en_dev->parent, en_dev->is_primary_port, FALSE);
        en_dev->is_multi_ep = en_dev->ops->is_multi_ep(en_dev->parent);
        ZXDH_SET_PFLAG(en_dev->pflags, ZXDH_PFLAG_HARDWARE_BOND, en_dev->is_hwbond ? 1 : 0);
    }
    en_dev->eth_config.rx_queue_size = ZXDH_PF_DEFAULT_DESC_NUM;
    en_dev->eth_config.tx_queue_size = ZXDH_PF_DEFAULT_DESC_NUM;
    en_dev->eth_config.num_rxq = en_dev->max_vq_pairs;
    en_dev->eth_config.num_txq = en_dev->max_vq_pairs;

    // PF92.5 特殊限速
    zxdh_set_pf_fc(en_dev);

    err = zxdh_tc_nic_init(en_priv);
    if (err)
    {
        LOG_ERR_DEV(en_dev->parent, "[%s] tc init failed: %d\n", netdev->name, err);
        goto err_tc_init;
    }
    err = zxdh_create_vqs(en_dev);
    if (err != 0)
    {
        HEAL_ERR_DEV(en_dev->parent, "%s zxdh_create_vqs failed: %d\n", netdev->name, err);
        goto err_vqs_create;
    }

    err = zxdh_vqs_init(netdev);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_vqs_init failed: %d\n", err);
        goto err_vqs_init;
    }

    if (en_dev->ops->is_drs_sec_enable(en_dev->parent))
    {
        err = zxdh_sec_vqs_init(netdev);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_sec_vqs_init failed\n");
            goto err_sec_vqs_init;
        }
    }

    err = zxdh_hash_id_init(en_dev);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_hash_id_init failed: %d\n", err);
        goto err_do_vqs_free;
    }

    if (zxdh_en_is_panel_port(en_dev) && !en_dev->ops->is_bond(en_dev->parent))
    {
        en_dev->panel_id = en_dev->ops->get_panel_id(en_dev->parent);
    }

    en_dev->eth_config.hash_func = ZXDH_FUNC_TOP;
    en_dev->eth_config.hash_mode = zxdh_get_default_hash_mode(en_dev);
    en_dev->eth_config.curr_combined = en_dev->curr_queue_pairs;

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        zxdh_cap_pkt_init(en_dev);
        if (en_dev->ops->is_bond(en_dev->parent))
        {
            err = zxdh_aux_alloc_pannel(en_dev);
            if (err != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "zxdh_aux_alloc_pannel failed: %d\n", err);
                goto err_do_vqs_free;
            }
        }
        else if (zxdh_en_is_panel_port(en_dev))
        {
            err = zxdh_phyport_get(en_dev);
            if (err != 0)
            {
                LOG_ERR_DEV(en_dev->parent, "zxdh_phyport_get failed: %d\n", err);
                goto err_do_vqs_free;
            }
        }

        err = zxdh_mac_addr_init(netdev);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_mac_addr_init failed: %d\n", err);
            goto err_mac_addr_init;
        }

        en_dev->wolopts = WAKE_MAGIC;
        err = zxdh_pf_port_init(en_dev, true);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_pf_port_init failed: %d\n", err);
            goto err_mac_addr_init;
        }
    }
    else
    {
        err = zxdh_vf_dpp_port_init(en_dev);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_dpp_port_init failed: %d\n", err);
            goto err_mac_addr_init;
        }
    }
    enable_1588_init(en_dev);

    if (!en_dev->ops->is_bond(en_dev->parent) && (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF) && (zxdh_en_is_panel_port(en_dev)))
    {
        mutex_lock(&dh_intf_mutex);
        list_add_tail(&en_dev->dev_node, &zxdh_en_dev_list);
        mutex_unlock(&dh_intf_mutex);
        en_dev->spec_sbdf = en_dev->ops->get_spec_sbdf(en_dev->parent);
        err = zxdh_lag_manager_add(en_dev);
        if (err != 0)
        {
            LOG_ERR("zxdh_lag_manager_add failed: %d\n", err);
            goto err_do_vport_free;
        }
        err = zxdh_lag_bond_lacp_dpp_init(en_dev);
        if (err != 0)
        {
            LOG_ERR("zxdh_lag_bond_lacp_dpp_init failed: %d\n", err);
            goto err_do_lag_manager_remove;
        }
    }

    if (!en_dev->ops->is_bond(en_dev->parent))
    {
        netdev->priv_flags &= ~IFF_RXFH_CONFIGURED;
        err = zxdh_num_channels_changed(en_dev, en_dev->eth_config.num_rxq);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_num_channels_changed failed: %d\n", err);
            goto err_do_lag_manager_remove;
        }

        err = zxdh_en_sync_features(en_dev, netdev->features);
        ZXDH_CHECK_RET_GOTO_ERR(err, err_do_rxfh_free, "zxdh_en_sync_features failed: %d\n", err);
        en_dev->features = netdev->features;
    }

    zxdh_device_ready(netdev);

    en_dev->ops->get_vendor(en_dev->parent, en_dev->vendor);
    if (zte_strlen_s(en_dev->vendor) == 0) {
        err = zxdh_en_firmware_version_get(en_dev, en_dev->fw_version);
        if (err != 0) {
            LOG_ERR_DEV(en_dev->parent, "zxdh_en_firmware_version_get failed: %d\n", err);
        }
    } else {
        en_dev->ops->get_fw_version(en_dev->parent, en_dev->fw_version);
        zxdh_format_firmware_version(en_dev->vendor, en_dev->fw_version);
    }

    err = zxdh_en_mtu_init(netdev);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_en_mtu_init failed: %d\n", err);
        goto err_do_rxfh_free;
    }

    en_dev->hw_stats.q_stats = kmalloc_array(en_dev->max_vq_pairs, sizeof(struct zxdh_en_queue_stats), GFP_KERNEL);
    if (unlikely(en_dev->hw_stats.q_stats == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "hw_stats.q_stats kmalloc failed\n");
        goto err_do_rxfh_free;
    }
    memset(en_dev->hw_stats.q_stats, 0, en_dev->max_vq_pairs * sizeof(struct zxdh_en_queue_stats));

    memset(&en_dev->pre_stats, 0, sizeof(struct zxdh_en_vport_stats));
    memset(&en_dev->last_stats, 0, sizeof(struct zxdh_en_vport_stats));
    memset(&en_dev->high_count, 0, sizeof(struct zxdh_en_vport_stats));
    memset(&en_dev->extended_stats, 0, sizeof(struct zxdh_en_vport_stats));
    spin_lock_init(&en_dev->vport_stats_lock);
    en_dev->last_tx_vport_ssvpc_packets = 0;

    err = zxdh_en_vport_pre_stats_get(en_dev);
    if(err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "get vport pre stats failed, %d\n", err);
    }

    en_dev->hw_stats.idma_stats = kmalloc_array(8, sizeof(struct zxdh_en_idma_stats), GFP_KERNEL);
    if (unlikely(en_dev->hw_stats.idma_stats == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "hw_stats.idma_stats kmalloc failed\n");
        goto err_kmalloc_idma_stats;
    }
    zte_memset_s(en_dev->hw_stats.idma_stats, 0, 8 * sizeof(struct zxdh_en_idma_stats));

    en_dev->hw_stats.tm_odma_stats = kmalloc_array(8, sizeof(struct zxdh_en_tm_odma_stats), GFP_KERNEL);
    if (unlikely(en_dev->hw_stats.tm_odma_stats == NULL))
    {
        LOG_ERR_DEV(en_dev->parent, "hw_stats.tm_odma_stats kmalloc failed\n");
        goto err_kmalloc_tm_odma_stats;
    }
    zte_memset_s(en_dev->hw_stats.tm_odma_stats, 0, 8 * sizeof(struct zxdh_en_tm_odma_stats));

    aux_set_netdev_name(netdev, en_dev->pannel_id);
#ifdef CGS_V5_693
    /* 3.10 内核要求 real_num_tx_queues 和 real_num_rx_queues 必须非零 */
    netdev->real_num_tx_queues = netdev->num_tx_queues;
    netdev->real_num_rx_queues = netdev->num_rx_queues;
    /* 3.10 内核中 qdisc 可能未正确初始化，需要手动设置为 noop_qdisc */
    if (!netdev->qdisc)
    {
        LOG_INFO_DEV(en_dev->parent, "netdev->qdisc is NULL, setting to &noop_qdisc\n");
        netdev->qdisc = &noop_qdisc;
    }

    /* 3.10 内核还要求每个 tx queue 的 qdisc 字段也必须初始化 */
    {
        unsigned int i;
        LOG_INFO_DEV(en_dev->parent, "Initializing %d tx queues\n", netdev->num_tx_queues);
        for (i = 0; i < netdev->num_tx_queues; i++)
        {
            struct netdev_queue *txq = netdev_get_tx_queue(netdev, i);
            if (!txq->qdisc)
            {
                txq->qdisc = &noop_qdisc;
            }
        }
        LOG_INFO_DEV(en_dev->parent, "Tx queues initialized\n");
    }
#endif
    err = register_netdev(netdev);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "register_netdev failed, %d\n", err);
        goto err_register_netdev;
    }
#ifdef CGS_V5_693
    LOG_INFO_DEV(en_dev->parent, "register_netdev success\n");
#endif

    zxdh_link_state_notify_kernel(netdev);

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        aux_get_bond_attrs(en_dev, &lag_attrs);
        err = zxdh_ldev_add_netdev(en_container->parent, en_dev->pannel_id, netdev, &lag_attrs);
        if (err != 0)
        {
            goto err_ldev_add_netdev;
        }
    }

#ifdef ZXDH_PLCR_OPEN
    err = zxdh_plcr_init(en_priv);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "zxdh_plcr_init failed, %d\n", err);
        //TODO:goto?
    }
#endif
    err = dh_aux_vlan_netdev_notifier_init(en_dev);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent,"dh_aux_vlan_netdev_notifier_init failed: %d\n", err);
        goto err_vlan_netdev_notifier_init;
    }

    err = dh_aux_vxlan_netdev_notifier_init(en_priv);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dh_aux_vxlan_netdev_notifier_init failed: %d\n", err);
        goto err_vxlan_netdev_notifier_init;
    }

    err = dh_aux_ipv6_notifier_init(en_priv);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "dh_aux_ipv6_notifier_init failed: %d\n", err);
        goto err_ipv6_notifier_init;
    }

    if ((en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF))
    {
        /* clear mcode gate,successfully build the scheduling tree, and then open it again */
        if (zxdh_en_is_panel_port(en_dev))
            zxdh_dcbnl_set_tm_pport_mcode_gate_close(netdev);
#ifdef ZXDH_DCBNL_OPEN
        err = zxdh_dcbnl_initialize(netdev);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_dcbnl_initialize failed: %d\n", err);
            //TODO:goto?
        }
        if (zxdh_en_is_panel_port(en_dev))
        {
            if (en_dev->is_lowlatency)
            {
                err = zxdh_dcbnl_set_np_flow_monitor_flag(netdev, 0);
                if (err != 0)
                {
                    LOG_ERR_DEV(en_dev->parent, "zxdh_dcbnl_set_np_flow_monitor: %d\n", err);
                }
            }
        }
#endif
    }

    en_dev->ops->set_bond_num(en_dev->parent, true);

    en_dev->init_comp_flag = AUX_INIT_COMPLETED;
    zxdh_dev_promisc_sync(en_dev);

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        en_dev->autoneg_enable = AUTONEG_ENABLE;
        err = zxdh_en_phyport_init(en_dev);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_en_phyport_init failed: %d\n", err);
            goto err_phyport_init;
        }
    }

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
#ifdef PTP_DRIVER_INTERFACE_EN
        if(zxdh_get_ptp_clock_index(en_dev, &phcidx))
        {
            LOG_ERR_DEV(en_dev->parent, "%s: aux dev get phc fail\n", netdev->name);
        }
        en_dev->clock_no = phcidx;
#else
        en_dev->clock_no = 0; /* 3.10 内核未启用 PTP 时设置为 0 */
#endif /* PTP_DRIVER_INTERFACE_EN */
        if (zxdh_en_is_panel_port(en_dev) && !en_dev->ops->is_bond(en_dev->parent))
        {
            if(ptp_set_pf_uplink_vfid(en_dev))
            {
                LOG_ERR_DEV(en_dev->parent, "%s: set ptp l2 vfid fail\n", netdev->name);
            }

            if(ptp_set_pf_tc_enable(en_dev, 0))
            {
                LOG_ERR_DEV(en_dev->parent, "%s: set tc enable fail\n", netdev->name);
            }
        }

        pf_info.vport = en_dev->vport;
        pf_info.slot = en_dev->slot_id;
        dpp_pktrx_mcode_glb_cfg_write(&pf_info,
                                    DPP_VF_BROADCAST_START_BIT,
                                    DPP_VF_BROADCAST_END_BIT, value_to_write);
    }

    en_dev->ops->set_init_comp_flag(en_dev->parent, 1);
    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        carrier_status = netif_carrier_ok(en_dev->netdev) ? 1 : 0;
        en_dev->ops->get_link_info_from_vqm(en_dev->parent, &link_up);
        link_up = link_up == 0 ? 0 : 1;
        LOG_DEBUG_DEV(en_dev->parent, "VF device: %s phy_link_state %d vs kernel_link_state %d\n",
                    pci_name(en_dev->ops->get_pdev(en_dev->parent)), link_up, carrier_status);
        if(link_up != carrier_status)
        {
            LOG_DEBUG_DEV(en_dev->parent, "VF device: %s phy_link_state %d not equal to kernel_link_state %d\n",
                        pci_name(en_dev->ops->get_pdev(en_dev->parent)), link_up, carrier_status);
            dh_eq_async_link_info_int_process(en_priv);
        }
    }
    card_num++;

    dh_ip_mac_init(en_priv);
    if (en_dev->ops->is_fw_feature_support(en_dev->parent, FW_FEATURE_PRIO_STATS) &&
        zxdh_en_is_panel_port(en_dev) && (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF))
    {
        pf_info.vport = en_dev->vport;
        pf_info.slot = en_dev->slot_id;

        err = dpp_pktrx_mcode_glb_cfg_write(&pf_info, ZXDH_NP_PRIO_STAT_ENABLE_BIT, ZXDH_NP_PRIO_STAT_ENABLE_BIT, 1);
        if (err != 0) {
            LOG_ERR_DEV(en_dev->parent, "pktrx_mcode_glb_cfg switch prio stat state on failed.\n");
            return -1;
        }
        ZXDH_SET_PFLAG(en_dev->pflags, ZXDH_PFLAG_RRIO_STAT_SWITCH, 1);
    }

    LOG_DEBUG_DEV(en_dev->parent, "ENTER zxdh_en_stats_init\n");
    zxdh_en_stats_init(en_dev);
    LOG_INFO_DEV(en_dev->parent, "aux level driver probe %s completed\n", pci_name(en_container->parent->pdev));
#ifdef HAVE_ETHTOOL_GET_MODULE_EEPROM_BY_PAGE
    zxdh_en_eeprom_init(en_dev);
#endif
    return 0;

err_phyport_init:
    en_dev->ops->set_bond_num(en_dev->parent, false);
#ifdef ZXDH_DCBNL_OPEN
    zxdh_dcbnl_ets_uninit(netdev);
#endif
    dh_inet6_addr_change_notifier_unregister(&(en_dev->ipv6_notifier));
err_ipv6_notifier_init:
    dh_vxlan_netdev_change_notifier_unregister(&(en_dev->vxlan_notifier));
err_vxlan_netdev_notifier_init:
    dh_aux_vlan_netdev_notifier_uninit(en_dev);
err_vlan_netdev_notifier_init:
#ifdef ZXDH_PLCR_OPEN
    zxdh_plcr_uninit(en_priv);
#endif
    if (en_dev->ops->is_bond(en_dev->parent))
    {
        aux_get_bond_attrs(en_dev, &lag_attrs);
        zxdh_ldev_remove_netdev(en_dev->parent, netdev, &lag_attrs);
    }
err_ldev_add_netdev:
    unregister_netdev(netdev);
err_register_netdev:
    kfree(en_dev->hw_stats.tm_odma_stats);
err_kmalloc_tm_odma_stats:
    kfree(en_dev->hw_stats.idma_stats);
err_kmalloc_idma_stats:
    kfree(en_dev->hw_stats.q_stats);
err_do_rxfh_free:
    if (!en_dev->ops->is_bond(en_dev->parent))
    {
        zxdh_rxfh_del(en_dev);
    }
err_do_lag_manager_remove:
    if (!en_dev->ops->is_bond(en_dev->parent) && (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF) && (zxdh_en_is_panel_port(en_dev)))
    {
        zxdh_lag_manager_remove(en_dev);
        mutex_lock(&dh_intf_mutex);
        list_del(&en_dev->dev_node);
        mutex_unlock(&dh_intf_mutex);
    }
err_do_vport_free:
    zxdh_vport_uninit(netdev, true);
err_mac_addr_init:
    if (en_dev->ops->is_bond(en_dev->parent))
    {
        en_dev->ops->release_port(en_dev->parent, en_dev->pannel_id);
    }
err_do_vqs_free:
    if (en_dev->ops->is_drs_sec_enable(en_dev->parent))
    {
        zxdh_sec_vqs_uninit(netdev, ZXDH_SEC_QUEUES_NUM(en_dev));
    }
err_sec_vqs_init:
    zxdh_vqs_uninit(netdev);
err_vqs_init:
    zxdh_add_status(netdev, ZXDH_CONFIG_S_FAILED);
    zxdh_destroy_vqs(en_dev);
err_vqs_create:
    zxdh_tc_nic_cleanup(en_priv);
err_tc_init:
    zxdh_del_pf_fc(en_dev);
err_status_init:
    dh_aux_eq_table_destroy(en_priv);
err_eq_table_create:
    dh_aux_events_uninit(en_priv);
err_events_init:
    dh_aux_eq_table_cleanup(en_priv);
err_eq_table_init:
    en_dev->ops->destroy_vqs_channels(en_dev->parent);
err_create_vqs_channels:
    en_dev->ops->set_rdma_netdev(en_dev->parent, NULL);
    free_netdev(netdev);
err_create_netdev:
    return -EPERM;
}

static int32_t eth_pflags_config_recover(struct net_device *netdev)
{
    struct zxdh_en_priv *en_priv = netdev_priv(netdev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t i = 0;
    int32_t err = 0;

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        err = zxdh_pflags_update(netdev, ZXDH_PFLAG_RSSKEY_IPID, en_dev->pflags & BIT(ZXDH_PFLAG_RSSKEY_IPID));
        if (err) {
            HEAL_ERR_DEV(en_dev->parent, "%s zxdh_pflags_update[%d] failed: %d\n", netdev->name, ZXDH_PFLAG_RSSKEY_IPID, err);
            return err;
        }
        return 0;
    }

    err = zxdh_dual_tor_label_get(en_dev);
    if (err == 1)
        en_dev->pflags |= BIT(ZXDH_PFLAG_DUAL_TOR_CTRL);
    else if (err == 0)
        en_dev->pflags &= ~BIT(ZXDH_PFLAG_DUAL_TOR_CTRL);

    for (i = 0; i < ZXDH_NUM_PFLAGS; ++i) {
        if (i == ZXDH_PFLAG_ETS_SWITCH ||
            i == ZXDH_PFLAG_PCIE_AER_CPL_TIMEOUT ||
            i == ZXDH_PFLAG_PCIE_HP_IRQ_CTRL ||
            i == ZXDH_PFLAG_RDMA_ETS_SWITCH ||
            i == ZXDH_PFLAG_RRIO_STAT_SWITCH)
            continue;
        err = zxdh_pflags_update(netdev, i, en_dev->pflags & BIT(i));
        if (err) {
            HEAL_ERR_DEV(en_dev->parent, "%s zxdh_pflags_update[%d] failed: %d\n", netdev->name, i, err);
            continue;
        }
    }

    return 0;
}

static void zxdh_init_stats(struct zxdh_en_device *en_dev)
{
    int32_t err = 0;
    int32_t i = 0;

    zte_memset_s(&en_dev->hw_stats.netdev_stats, 0, sizeof(struct zxdh_en_netdev_stats));
    zte_memset_s(&en_dev->hw_stats.vport_stats, 0, sizeof(struct zxdh_en_vport_stats));
    zte_memset_s(&en_dev->hw_stats.phy_stats, 0, sizeof(struct zxdh_en_phy_stats));
    zte_memset_s(&en_dev->hw_stats.udp_stats, 0, sizeof(struct zxdh_en_udp_phy_stats));
    zte_memset_s(en_dev->hw_stats.q_stats, 0, en_dev->max_vq_pairs * sizeof(struct zxdh_en_queue_stats));
    zte_memset_s(en_dev->hw_stats.idma_stats, 0, 8 * sizeof(struct zxdh_en_idma_stats));
    zte_memset_s(en_dev->hw_stats.tm_odma_stats, 0, 8 * sizeof(struct zxdh_en_tm_odma_stats));
    zte_memset_s(&en_dev->pre_stats, 0, sizeof(struct zxdh_en_vport_stats));
    zte_memset_s(&en_dev->last_stats, 0, sizeof(struct zxdh_en_vport_stats));
    zte_memset_s(&en_dev->high_count, 0, sizeof(struct zxdh_en_vport_stats));
    zte_memset_s(&en_dev->extended_stats, 0, sizeof(struct zxdh_en_vport_stats));
    spin_lock_init(&en_dev->vport_stats_lock);
    en_dev->last_tx_vport_ssvpc_packets = 0;

    err = zxdh_en_vport_pre_stats_get(en_dev);
    if(err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "get vport pre stats failed, %d\n", err);
    }

    for (i = 0; i < en_dev->max_queue_pairs; i++)
    {
        zte_memset_s(&en_dev->rq[i].stats, 0, sizeof(struct virtnet_rq_stats));
        zte_memset_s(&en_dev->sq[i].stats, 0, sizeof(struct virtnet_sq_stats));
    }
    return;
}

int32_t zxdh_cfg_reload(struct zxdh_en_device *en_dev)
{
    int32_t err = 0;

    if (en_dev->ops->is_bond(en_dev->parent))
        return 0;

    err = zxdh_dev_promisc_sync(en_dev);
    ZXDH_CHECK_RET_RETURN(err, "zxdh_dev_promisc_sync failed: %d\n", err);

    err = zxdh_en_sync_features(en_dev, en_dev->features);
    ZXDH_CHECK_RET_RETURN(err, "zxdh_en_sync_features failed: %d\n", err);

    return 0;
}

static uint32_t dh_pf_ets_init(struct zxdh_en_priv *en_priv)
{
    uint32_t err = 0;
    int32_t i = 0;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;
    LOG_DEBUG_DEV(en_priv->edev.parent, "dh pf ets init begin\n");

    err = zxdh_dcbnl_init_ets_scheduling_tree(en_priv, false);
    if (err)
    {
        LOG_ERR_DEV(en_priv->edev.parent, "dh pf ets init: init_ets_scheduling_tree failed \n");
        return err;
    }

    for (i = 0; i < ZXDH_DCBNL_MAX_PRIORITY; i++)
    {
        err = dpp_tm_pport_up_map_table_set(&pf_info, en_dev->phy_port, i, i);  //初始时，配置一一对应
        if (err)
        {
            LOG_ERR_DEV(en_dev->parent, "dcbnl_init_ets: dpp_tm_pport_up_map_table_set failed, vport: %d, phy_port: %d, err:%d\n",
                    en_dev->vport, en_dev->phy_port, err);
            return err;
        }
    }
    LOG_DEBUG_DEV(en_dev->parent, " vport:%d,phy_port:%d prio2tc ok \n", en_dev->vport, en_dev->phy_port);

    for (i = 0; i < ZXDH_DCBNL_MAX_DSCP; i++)
    {
        err = dpp_tm_pport_dscp_map_table_set(&pf_info, en_dev->phy_port, i, i>>3);
        if (err)
        {
            LOG_ERR_DEV(en_dev->parent, "dcbnl_init_ets: dscp_map_table_set failed, vport: %d, phy_port: %d, err:%d\n",
                    en_dev->vport, en_dev->phy_port, err);
            return err;
        }
    }
    LOG_DEBUG_DEV(en_dev->parent, "vport:%d,phy_port:%d,dscp2prio ok \n", en_dev->vport, en_dev->phy_port);

    zxdh_dcbnl_set_ets_trust(en_priv, en_dev->trust);

    zxdh_dcbnl_printk_ets_tree(en_priv);

    LOG_DEBUG_DEV(en_priv->edev.parent, "dh pf ets init init ok ");

    return err;
}


static void dh_pf_ets_free(struct zxdh_en_priv *en_priv)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct zxdh_dcbnl_ets_flow_node *flow_node = NULL;
    struct zxdh_dcbnl_ets_se_node *se_node = NULL;
    struct zxdh_dcbnl_ets_node_list_head *ets_node_list_head = NULL;
    uint32_t level = 0;

    LOG_DEBUG("dh pf ets free begin\n");

    if (en_priv == NULL)
    {
        LOG_ERR("en_priv not initialized!\n");
        return;
    }

    if (en_dev == NULL && !en_dev->init_comp_flag)
    {
        LOG_ERR("en_device not initialized!\n");
        return;
    }

    ets_node_list_head = &en_dev->dcb_para.ets_node_list_head[ZXDH_DCBNL_ETS_TREE_FLOW_LEVEL];

    while (NULL != ets_node_list_head->flow_next)
    {
        flow_node = ets_node_list_head->flow_next;
        ets_node_list_head->flow_next = flow_node->flow_next;
        kfree(flow_node);
        ets_node_list_head->node_num -= 1;
    }

    for (level = 1; level < ZXDH_DCBNL_ETS_TREE_ROOT_LEVEL + 1; level++)
    {
        ets_node_list_head = &en_dev->dcb_para.ets_node_list_head[level];
        while (NULL != ets_node_list_head->se_next)
        {
            se_node = ets_node_list_head->se_next;
            ets_node_list_head->se_next = se_node->se_next;
            kfree(se_node);
            ets_node_list_head->node_num -= 1;
        }
    }

    LOG_DEBUG("dh pf ets free begin\n");
}

int32_t zxdh_aux_load(struct zxdh_en_priv *en_priv)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct net_device *netdev = en_dev->netdev;
    struct zxdh_lag_attrs lag_attrs;
    int32_t err = 0;
    int32_t vqs_channel_num = 0;
    DPP_PF_INFO_T pf_info = {0};
    int value_to_write = vf_broadcast ? 1 : 0;

    LOG_INFO_DEV(en_dev->parent, "aux level driver load %s start\n", netdev->name);

    mutex_lock(&en_dev->parent->lock);
    if (en_dev->parent->driver_process == ZXDH_REMOVE)
        goto unlock;

    vqs_channel_num = en_dev->ops->create_vqs_channels(en_dev->parent, en_dev);
    if (vqs_channel_num < 0)
    {
        HEAL_ERR_DEV(en_dev->parent, "%s create_vqs_channels failed, vqs_channel_num: %d\n", netdev->name, vqs_channel_num);
        en_dev->parent->driver_process = ZXDH_LOAD_FAIL;
        goto unlock;
    }
    en_dev->ro_flag = en_dev->ops->get_ro_info_from_fwshrd(en_dev->parent);
    LOG_DEBUG_DEV(en_dev->parent, "netdev %s ro flag:%x.\n", netdev->name, en_dev->ro_flag);
    err = dh_aux_eq_table_create(en_priv);
    if (err != 0)
    {
        HEAL_ERR_DEV(en_dev->parent, "%s Failed to alloc EQs: %d\n", netdev->name, err);
        goto err_eq_table_create;
    }

    err = zxdh_status_init(netdev);
    if (err != 0)
    {
        HEAL_ERR_DEV(en_dev->parent, "%s zxdh_status_init failed: %d\n", netdev->name, err);
        goto err_status_init;
    }

    // PF92.5特殊限速
    zxdh_set_pf_fc(en_dev);
    err = zxdh_vqs_init(netdev);
    if (err != 0)
    {
        HEAL_ERR_DEV(en_dev->parent, "%s zxdh_vqs_init failed: %d\n", netdev->name, err);
        goto err_vqs_init;
    }

    if (en_dev->ops->is_drs_sec_enable(en_dev->parent))
    {
        err = zxdh_sec_vqs_init(netdev);
        if (err != 0)
        {
            HEAL_ERR_DEV(en_dev->parent, "%s zxdh_sec_vqs_init failed\n", netdev->name);
            goto err_sec_vqs_init;
        }
    }

    zxdh_device_ready(netdev);

    err = zxdh_port_init(netdev);
    if (err != 0)
    {
        HEAL_ERR_DEV(en_dev->parent, "%s zxdh_port_init failed: %d\n", netdev->name, err);
        goto err_port_init;
    }

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        aux_get_bond_attrs(en_dev, &lag_attrs);
        err = zxdh_ldev_add_netdev(en_dev->parent, en_dev->pannel_id, netdev, &lag_attrs);
        if (err != 0)
        {
            HEAL_ERR_DEV(en_dev->parent, "%s zxdh_ldev_add_netdev failed: %d\n", netdev->name, err);
            goto err_ldev_add_netdev;
        }
    }

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        if (en_dev->phy_port <= ZXDH_PHY_PORT_MAX)
        {
            dh_pf_ets_free(en_priv);
            err = dh_pf_ets_init(en_priv);
            if (err != 0)
            {
                HEAL_ERR_DEV(en_dev->parent, "dh_pf_ets_init failed\n");
                goto ets_init_err;
            }
        }
    }

    en_dev->init_comp_flag = AUX_INIT_COMPLETED;
    zxdh_init_stats(en_dev);
    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        dh_eq_async_link_info_int_process(en_priv);
    }
    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        err = zxdh_recover_hwbond_in_reload(en_dev->netdev);
        if (err != 0)
        {
            HEAL_ERR_DEV(en_dev->parent, "zxdh_recover_hwbond_in_reload failed: %d\n", err);
            goto err_recover_hwbond;
        }
        err = zxdh_en_phyport_init(en_dev);
        if (err != 0)
        {
            HEAL_ERR_DEV(en_dev->parent, "%s zxdh_en_phyport_init failed: %d\n", netdev->name, err);
            goto err_recover_hwbond;
        }

        pf_info.slot = en_dev->slot_id;
        pf_info.vport = en_dev->vport;
        dpp_pktrx_mcode_glb_cfg_write(&pf_info,
                                    DPP_VF_BROADCAST_START_BIT,
                                    DPP_VF_BROADCAST_END_BIT, value_to_write);
    }
    en_dev->fast_unload = false;
    en_dev->ops->set_bond_num(en_dev->parent, true);
    en_dev->ops->set_init_comp_flag(en_dev->parent, 1);

    en_dev->curr_queue_pairs = en_dev->eth_config.curr_combined;
    eth_pflags_config_recover(en_dev->netdev);

    en_dev->device_state = ZXDH_DEVICE_STATE_UP;
    mutex_unlock(&en_dev->parent->lock);
    if (netif_running(netdev))
        en_open(netdev, false);
    else
        en_phyport_close(en_dev);
    netif_tx_wake_all_queues(netdev);

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        mod_timer(&en_dev->service_riscv_timer, jiffies);
    }
    LOG_INFO("%s aux level load success\n", netdev->name);
    return 0;

err_recover_hwbond:
    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        zxdh_dcbnl_ets_uninit(netdev);
    }
ets_init_err:
    if (en_dev->ops->is_bond(en_dev->parent))
    {
        aux_get_bond_attrs(en_dev, &lag_attrs);
        zxdh_ldev_remove_netdev(en_dev->parent, netdev, &lag_attrs);
    }
err_ldev_add_netdev:
    zxdh_vport_uninit(netdev, false);
err_port_init:
    if (en_dev->ops->is_drs_sec_enable(en_dev->parent))
    {
        zxdh_sec_vqs_uninit(netdev, ZXDH_SEC_QUEUES_NUM(en_dev));
    }
err_sec_vqs_init:
    zxdh_vqs_uninit(netdev);
err_vqs_init:
    zxdh_del_pf_fc(en_dev);
    zxdh_vp_reset(netdev);
err_status_init:
    dh_aux_eq_table_destroy(en_priv);
err_eq_table_create:
    en_dev->ops->destroy_vqs_channels(en_dev->parent);
    en_dev->parent->driver_process = ZXDH_LOAD_FAIL;
unlock:
    mutex_unlock(&en_dev->parent->lock);
    return -EPERM;
}

static void del_cfg_shell_script(struct work_struct *work)
{
    static const char command[] = "/etc/zxdh_cfg/smart_nic_cfg_proc.sh";
    char *argv[] = {(char *)command, "d", NULL};
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
        LOG_ERR("Failed to execute smart_nic_cfg_del.sh(err:%d)\n", ret);
    }
    else
    {
        LOG_INFO("smart_nic_cfg_del.sh executed successfully,ret:%d\n", ret);
    }
}

void zxdh_aux_unload(struct zxdh_en_priv *en_priv)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct net_device *netdev = en_dev->netdev;
    struct zxdh_lag_attrs lag_attrs;

    zxdh_napi_close(en_priv);

    mutex_lock(&en_dev->parent->lock);
    if ((en_dev->parent->driver_process == ZXDH_REMOVE) || (en_dev->parent->driver_process == ZXDH_LOAD_FAIL)) {
        mutex_unlock(&en_dev->parent->lock);
        return;
    }
    en_dev->device_state = ZXDH_DEVICE_STATE_INTERNAL_ERROR;
    en_dev->parent->driver_process = ZXDH_UNLOAD;
    en_dev->init_comp_flag = AUX_INIT_INCOMPLETED;
    en_dev->ops->set_init_comp_flag(en_dev->parent, 0);
    en_dev->ops->set_bond_num(en_dev->parent, false);
    en_dev->fast_unload = true;

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        aux_get_bond_attrs(en_dev, &lag_attrs);
        zxdh_ldev_remove_netdev(en_dev->parent, netdev, &lag_attrs);
    }

    if (en_dev->ops->is_drs_sec_enable(en_dev->parent))
    {
        zxdh_sec_vqs_uninit(netdev, ZXDH_SEC_QUEUES_NUM(en_dev));
    }

    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        zxdh_cap_pkt_uninit(en_dev, false);
    }

    synchronize_net();  // 等待所有正在处理的网络操作完成
    zxdh_vqs_uninit(netdev);

    en_dev->ops->destroy_vqs_channels(en_dev->parent);
    mutex_unlock(&en_dev->parent->lock);
}

#if defined(ZXDH_ADAPT_REDHAT_9_2) || defined(AUX_BUS_TYPE_REMOVE_RETURN_VOID)
void zxdh_en_dev_remove(struct zxdh_auxiliary_device *adev)
#else
static int32_t zxdh_en_dev_remove(struct zxdh_auxiliary_device *adev)
#endif
{
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)dev_get_drvdata(&adev->dev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct net_device *netdev = en_dev->netdev;
    struct pci_dev *pdev = en_dev->parent->pdev;
    struct zxdh_lag_attrs lag_attrs;
    DPP_PF_INFO_T pf_info = {0};

    LOG_INFO_DEV(en_dev->parent, "aux level driver remove %s start\n", pci_name(pdev));
    if (!en_dev->ops->get_rp_link_status(en_dev->parent)) {
        en_dev->quick_remove = true;
        LOG_INFO_DEV(en_dev->parent, "aux level driver quick_remove %s start\n", netdev->name);
    }

    mutex_lock(&en_dev->parent->lock);
    en_dev->ops->set_init_comp_flag(en_dev->parent, 0);
    en_dev->parent->driver_process = ZXDH_REMOVE;
    en_dev->init_comp_flag = AUX_INIT_INCOMPLETED;
    mutex_unlock(&en_dev->parent->lock);

    en_dev->ops->set_bond_num(en_dev->parent, false);
    zxdh_en_stats_uninit(en_dev);
#ifdef HAVE_ETHTOOL_GET_MODULE_EEPROM_BY_PAGE
    zxdh_en_eeprom_uninit(en_dev);
#endif
    if (en_dev->ops->is_fw_feature_support(en_dev->parent, FW_FEATURE_PRIO_STATS) &&
        zxdh_en_is_panel_port(en_dev) && (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF))
    {
        pf_info.vport = en_dev->vport;
        pf_info.slot = en_dev->slot_id;

        dpp_pktrx_mcode_glb_cfg_write(&pf_info, ZXDH_NP_PRIO_STAT_ENABLE_BIT, ZXDH_NP_PRIO_STAT_ENABLE_BIT, 0);
    }
#ifdef ZXDH_DCBNL_OPEN
    zxdh_dcbnl_ets_uninit(netdev);
#endif
    dh_aux_vlan_netdev_notifier_uninit(en_dev);
    dh_vxlan_netdev_change_notifier_unregister(&(en_dev->vxlan_notifier));
    dh_inet6_addr_change_notifier_unregister(&(en_dev->ipv6_notifier));

#ifdef ZXDH_PLCR_OPEN
    zxdh_plcr_uninit(en_priv);
#endif

    if (en_dev->ops->is_bond(en_dev->parent))
    {
        aux_get_bond_attrs(en_dev, &lag_attrs);
        zxdh_ldev_remove_netdev(en_dev->parent, netdev, &lag_attrs);
    }
    if (!en_dev->ops->is_bond(en_dev->parent) && (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF) && (zxdh_en_is_panel_port(en_dev)))
    {
            zxdh_lag_manager_remove(en_dev);
            mutex_lock(&dh_intf_mutex);
            list_del(&en_dev->dev_node);
            mutex_unlock(&dh_intf_mutex);
    }

    unregister_netdev(netdev);
    kfree(en_dev->hw_stats.tm_odma_stats);
    kfree(en_dev->hw_stats.idma_stats);
    kfree(en_dev->hw_stats.q_stats);

    if (!en_dev->fast_unload) {
        if (!en_dev->ops->is_bond(en_dev->parent))
            zxdh_rxfh_del(en_dev);

        zxdh_vport_uninit(netdev, true);
        if (en_dev->ops->is_drs_sec_enable(en_dev->parent))
        {
            zxdh_sec_vqs_uninit(netdev, ZXDH_SEC_QUEUES_NUM(en_dev));
        }
        zxdh_vqs_uninit(netdev);
        zxdh_del_pf_fc(en_dev);
        zxdh_add_status(netdev, ZXDH_CONFIG_S_FAILED);
        dh_aux_eq_table_destroy(en_priv);
    }
    zxdh_destroy_vqs(en_dev);

    dh_aux_events_uninit(en_priv);
    dh_aux_eq_table_cleanup(en_priv);
    if (!en_dev->fast_unload)
        en_dev->ops->destroy_vqs_channels(en_dev->parent);
    if (en_dev->ops->is_bond(en_dev->parent))
        en_dev->ops->release_port(en_dev->parent, en_dev->pannel_id);
    en_dev->ops->set_rdma_netdev(en_dev->parent, NULL);

    zxdh_tc_nic_cleanup(en_priv);

    mutex_destroy(&en_priv->lock);
    LOG_INFO("start free_netdev %s\n", netdev_name(netdev));
    free_netdev(netdev);
    LOG_INFO("free_netdev finish\n");

    card_num--;
    if (card_num == 0)
    {
        INIT_WORK(&work_cfg_del, del_cfg_shell_script);
        queue_work(system_wq, &work_cfg_del);
    }

    LOG_INFO("aux level driver remove %s completed\n", pci_name(pdev));

#if defined(ZXDH_ADAPT_REDHAT_9_2) || defined(AUX_BUS_TYPE_REMOVE_RETURN_VOID)
    return;
#else
    return 0;
#endif
}

/* Started by AICoder, pid:k59c8wece3ve22d14b550a71e04692169b623a29 */
static void zxdh_en_dev_shutdown(struct zxdh_auxiliary_device *adev)
{
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)dev_get_drvdata(&adev->dev);
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct net_device *netdev = en_dev->netdev;
    struct pci_dev *pdev = en_dev->parent->pdev;

    LOG_INFO_DEV(en_dev->parent, "aux level driver shutdown %s start\n", netdev->name);
    zxdh_en_dev_remove(adev);
    LOG_INFO("aux level driver shutdown %s completed\n", pci_name(pdev));
};
/* Ended by AICoder, pid:k59c8wece3ve22d14b550a71e04692169b623a29 */

static const struct zxdh_auxiliary_device_id zxdh_en_dev_id_table[] = {
    { .name = ZXDH_PF_NAME "." ZXDH_EN_DEV_ID_NAME, },
    { },
};

MODULE_DEVICE_TABLE(zxdh_auxiliary, zxdh_en_dev_id_table);

static struct zxdh_auxiliary_driver zxdh_en_driver = {
    .name     = ZXDH_EN_DEV_ID_NAME,
    .probe    = zxdh_en_dev_probe,
    .remove   = zxdh_en_dev_remove,
    .shutdown = zxdh_en_dev_shutdown,
    .id_table = zxdh_en_dev_id_table,
};

int32_t zxdh_en_driver_register(void)
{
    int32_t err = 0;

    if ((max_pairs == 0) || (max_pairs >= ZXDH_MAX_PAIRS_NUM))
    {
        LOG_INFO("max_pairs %u parameter is a invalid value, use the default value %u\n", max_pairs, ZXDH_MQ_PAIRS_NUM);
        max_pairs = ZXDH_MQ_PAIRS_NUM;
    }

    zxdh_rdma_device_lock_init();
    zxdh_lag_lock_init();
    err = zxdh_auxiliary_driver_register(&zxdh_en_driver);
    if (err != 0)
    {
        LOG_ERR("zxdh_auxiliary_driver_register failed: %d\n", err);
        goto err_aux_register;
    }

    err = dh_aux_msg_recv_func_register();
    if (err != 0)
    {
        LOG_ERR("dh_aux_msg_recv_func_register failed: %d\n", err);
        goto err_msg_recv_register;
    }

    err = zxdh_tools_netlink_register();
    if (err != 0)
    {
        LOG_ERR("zxdh_tools_msg_family register error failed: %d\n", err);
        goto err_netlink_register;
    }

    LOG_INFO("all driver insmod completed\n");

    return 0;

err_netlink_register:
    dh_aux_msg_recv_func_unregister();
err_msg_recv_register:
    zxdh_auxiliary_driver_unregister(&zxdh_en_driver);
err_aux_register:
#ifdef CONFIG_ZXDH_HARDWARE_BOND
    zxdh_lag_lock_deinit();
#endif
    zxdh_rdma_device_lock_deinit();
    return err;
}

void zxdh_en_driver_unregister(void)
{
    LOG_INFO("driver rmmod start\n");
    zxdh_tools_netlink_unregister();
    dh_aux_msg_recv_func_unregister();
    zxdh_auxiliary_driver_unregister(&zxdh_en_driver);
#ifdef CONFIG_ZXDH_HARDWARE_BOND
    zxdh_lag_lock_deinit();
#endif
    zxdh_rdma_device_lock_deinit();
}

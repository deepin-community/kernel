#include <linux/netdevice.h>
#include <net/tc_act/tc_csum.h>
#include <net/pkt_cls.h>
#include "en_tc.h"
#include "../en_aux.h"
#include "../slib.h"
#include "../en_aux/en_aux_cmd.h"
#include "linux/dinghai/dh_cmd.h"
#include "../msg_common.h"
#include "../en_pf/msg_func.h"
#include "../en_ethtool/ethtool.h"

static void parse_l2_param(ZXDH_FD_CFG_T *p_fd_cfg,struct zxdh_ifc_lyr2_4_param *header)
{
    uint32_t i = 0;

    if (!is_zero_ether_addr(header->key.dmac)) 
    {
        zte_memcpy_s(p_fd_cfg->key.dmac, header->key.dmac, ETH_ALEN);
        for(i=0;i<ETH_ALEN;i++)
        {
            p_fd_cfg->mask.dmac[i] = ~header->mask.dmac[i];
        }
    }

    if (!is_zero_ether_addr(header->key.smac)) 
    {
        zte_memcpy_s(p_fd_cfg->key.smac, header->key.smac, ETH_ALEN);
        for(i=0;i<ETH_ALEN;i++)
        {
            p_fd_cfg->mask.smac[i] = ~header->mask.smac[i];
        }
    }
        
    if(header->key.vlan_id)
    {
        p_fd_cfg->key.cvlan_pri = header->key.vlan_prio;
        p_fd_cfg->mask.cvlan_pri = ETHTOOL_TRUE_MASK;
        p_fd_cfg->key.cvlanid = header->key.vlan_id;
        p_fd_cfg->mask.cvlanid = ETHTOOL_TRUE_MASK;
    }

    return;
}

static void parse_l3_param(ZXDH_FD_CFG_T *p_fd_cfg,struct zxdh_ifc_lyr2_4_param *header)
{
    uint32_t i = 0;

    if(header->key.ethertype == ETH_P_IP)
    {
        zte_memcpy_s((uint8_t *)p_fd_cfg->key.sip + 12, header->key.src_ip, ETHTOOL_IP4_LEN);
        for(i=0;i<ETHTOOL_IP4_LEN;i++)
        {
            p_fd_cfg->mask.sip[12+i] = ~header->mask.src_ip[i];
        }
        zte_memcpy_s((uint8_t *)p_fd_cfg->key.dip + 12 , header->key.dst_ip, ETHTOOL_IP4_LEN);
        for(i=0;i<ETHTOOL_IP4_LEN;i++)
        {
            p_fd_cfg->mask.dip[12+i] = ~header->mask.dst_ip[i];
        }
    }
    else if(header->key.ethertype == ETH_P_IPV6)
    {
        zte_memcpy_s(p_fd_cfg->key.sip, header->key.src_ip, ETHTOOL_IP6_LEN);
        for(i=0;i<ETHTOOL_IP6_LEN;i++)
        {
            p_fd_cfg->mask.sip[i] = ~header->mask.src_ip[i];
        }
        zte_memcpy_s(p_fd_cfg->key.dip, header->key.dst_ip, ETHTOOL_IP6_LEN);
        for(i=0;i<ETHTOOL_IP6_LEN;i++)
        {
            p_fd_cfg->mask.dip[i] = ~header->mask.dst_ip[i];
        }
    }

     p_fd_cfg->key.fragment = header->key.frag;
     p_fd_cfg->mask.fragment = ~header->mask.frag;

     p_fd_cfg->key.tos = (header->key.ip_dscp<<2) | header->key.ip_ecn;
     p_fd_cfg->mask.tos = ~((header->mask.ip_dscp<<2) | header->mask.ip_ecn);

    return;
}

static void parse_l4_param(ZXDH_FD_CFG_T *p_fd_cfg,struct zxdh_ifc_lyr2_4_param *header)
{
    if(header->key.ip_protocol==IPPROTO_UDP)
    {
        p_fd_cfg->key.proto = IPPROTO_UDP;
        p_fd_cfg->mask.proto = ETHTOOL_TRUE_MASK;
        p_fd_cfg->key.sport = header->key.udp_sport;
        p_fd_cfg->mask.sport = ~header->mask.udp_sport;
        p_fd_cfg->key.dport = header->key.udp_dport;
        p_fd_cfg->mask.dport = ~header->mask.udp_dport;
    }
    else if(header->key.ip_protocol==IPPROTO_TCP)
    {
        p_fd_cfg->key.proto = IPPROTO_TCP;
        p_fd_cfg->mask.proto = ETHTOOL_TRUE_MASK;
        p_fd_cfg->key.sport = header->key.tcp_sport;
        p_fd_cfg->mask.sport = ~header->mask.tcp_sport;
        p_fd_cfg->key.dport = header->key.tcp_dport;
        p_fd_cfg->mask.dport = ~header->mask.tcp_dport;
    }
    
    return;
}

int32_t zxdh_tc_flow_table_add(struct zxdh_nic_flow_attr *flow_attr, ZXDH_FD_CFG_T *p_fd_cfg,
                            DPP_PF_INFO_T *pf_info)
{
    struct zxdh_flow_spec *spec = &flow_attr->parse_attr->spec;
    uint32_t match_criteria_enable = spec->match_criteria_enable;
    struct zxdh_ifc_lyr2_4_param *header = NULL;

    p_fd_cfg->key.vqm_vfid = VQM_VFID(pf_info->vport);
    p_fd_cfg->mask.vqm_vfid = ETHTOOL_TRUE_MASK;

    if(match_criteria_enable & ZXDH_MATCH_OUTER_HEADERS)
    {
        header = &spec->outer_header;
    }
    else if(match_criteria_enable & ZXDH_MATCH_INNER_HEADERS)
    {
        header = &spec->inner_header;
    }
    else
    {
        LOG_ERR("match_criteria_enable error\n");
        return -EINVAL;
    }

    /* 根据不同流类型，设置掩码和键值 */
    switch(flow_attr->match_level)
    {
        case ZXDH_MATCH_L2:
        {
            parse_l2_param(p_fd_cfg,header);
            break;
        }
        case ZXDH_MATCH_L3:
        {
            parse_l2_param(p_fd_cfg,header);
            parse_l3_param(p_fd_cfg,header);
            break;
        }
        case ZXDH_MATCH_L4:
        {
            parse_l2_param(p_fd_cfg,header);
            parse_l3_param(p_fd_cfg,header);
            parse_l4_param(p_fd_cfg,header);
            break;
        }
        default:
            break;
    }

    p_fd_cfg->key.ethtype = header->key.ethertype;
    p_fd_cfg->mask.ethtype = ~header->mask.ethertype;

    return 0;
}

int32_t zxdh_tc_flow_action_add(struct zxdh_en_device *en_dev, struct zxdh_nic_flow_attr *flow_attr, ZXDH_FD_CFG_T *p_fd_cfg)
{
    struct net_device *out_dev = NULL;
    struct zxdh_en_device *out_en_dev = NULL;
    uint32_t valid_actions_mask = ZXDH_FLOW_CONTEXT_ACTION_DROP | 
                                   ZXDH_FLOW_CONTEXT_ACTION_COUNT | 
                                    ZXDH_FLOW_CONTEXT_ACTION_FWD_DEST;
    
    if ((flow_attr->action & ~valid_actions_mask) != 0) {
        LOG_ERR_DEV(en_dev->parent, "Unsupported action: 0x%x\n", flow_attr->action);
        return -EOPNOTSUPP;
    }

    if (flow_attr->action & ZXDH_FLOW_CONTEXT_ACTION_DROP) 
    {
        p_fd_cfg->as_rlt.action_index |= ACTION_TYPE_DROP;
    }

    if(flow_attr->action & ZXDH_FLOW_CONTEXT_ACTION_COUNT)
    {
        p_fd_cfg->as_rlt.action_index |= ACTION_TYPE_COUNT;
    }

    if(flow_attr->action & ZXDH_FLOW_CONTEXT_ACTION_FWD_DEST)
    {
        out_dev = __dev_get_by_index(dev_net(en_dev->netdev),flow_attr->parse_attr->mirred_ifindex[0]);
        out_en_dev = netdev_priv(out_dev);
        p_fd_cfg->as_rlt.action_index2 |= ACTION_TYPE_SPEC_PORT;
        p_fd_cfg->as_rlt.spec_port_vfid = VQM_VFID(out_en_dev->vport);
    }

    return 0;
}

static int32_t zxdh_tc_pf_add_fd(struct zxdh_en_device *en_dev, struct zxdh_nic_flow_attr *flow_attr, ZXDH_FD_CFG_T *p_fd_cfg)
{
    uint32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
    uint32_t handle = 0;
    DPP_STAT_VALUE_U stat_value = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* 申请新的index */
    err = dpp_fd_acl_index_request(&pf_info, &handle);
    if (err) 
    {
        LOG_ERR_DEV(en_dev->parent, "failed to request index!\n");
        return -ENOSPC;
    }

    p_fd_cfg->as_rlt.count_id = handle;
    flow_attr->counter->id = handle;

    err = dpp_tbl_fd_cfg_add(&pf_info, ZXDH_SDT_FD_CFG_TABLE, handle, p_fd_cfg);
    if(err)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to add fd in np!\n"); 
        goto error;
    }

    err = dpp_stat_item_cnt_get(&pf_info, DPP_STAT_ITEM_FD_FLOW_STAT, handle, STAT_RD_CLR_MODE_CLR, &stat_value);
    if(err)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to clear stat!\n");
        goto error;
    }
    return 0;

error:
    err = dpp_fd_acl_index_release(&pf_info, handle);
    if(err!=0)
    {
        LOG_ERR_DEV(en_dev->parent, "acl index release fail!\n");
        return -ENOSPC; 
    }
    return -ENOSPC;
}

static int32_t zxdh_tc_vf_add_fd(struct zxdh_en_device *en_dev, struct zxdh_nic_flow_attr *flow_attr, ZXDH_FD_CFG_T *p_fd_cfg)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !", sizeof(union zxdh_msg));
        return -ENOMEM;
    }
    msg->payload.hdr.op_code = ZXDH_TC_FD_ADD;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;
    zte_memcpy_s(&msg->payload.tc_vf_fd_cfg_msg.fd_cfg, p_fd_cfg, sizeof(*p_fd_cfg));

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    flow_attr->counter->id = msg->reps.fd_cfg_resp.index;
    kfree(msg);
    if (err != 0) {
        LOG_ERR_DEV(en_dev->parent, "send_command_to_pf_np failed: %d\n",err);
    }

    return err;
}

int32_t zxdh_tc_flow_replace(struct zxdh_en_priv *priv,
                             struct zxdh_tc_flow *flow)
{
    uint32_t err = 0;
    struct zxdh_en_device *en_dev = &priv->edev;
    ZXDH_FD_CFG_T fd_cfg = {0};
    DPP_PF_INFO_T pf_info = {0};

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    /* 填写fd表 */
    zte_memset_s(&fd_cfg.key, 0x0, sizeof(ZXDH_FD_CFG_KEY));
    zte_memset_s(&fd_cfg.mask, 0xff, sizeof(ZXDH_FD_CFG_MASK));

    err = zxdh_tc_flow_table_add(flow->nic_attr, &fd_cfg, &pf_info);
    if (err != 0) {
        return err;
    }

    err = zxdh_tc_flow_action_add(en_dev, flow->nic_attr, &fd_cfg);
    if (err != 0) {
        return err;
    }

    /* 配置到np(PF) */
    flow->nic_attr->counter->id = DEFAULT_ADD_INDEX;
    if (en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF) 
    {
        err = zxdh_tc_pf_add_fd(en_dev, flow->nic_attr, &fd_cfg);
    }
    else  /*VF发送消息给PF进行配置*/
    {
        err = zxdh_tc_vf_add_fd(en_dev, flow->nic_attr, &fd_cfg);
    }

    if (err != 0) {
        LOG_ERR_DEV(en_dev->parent, "failed to add fd in np!\n");
        return -EPERM;
    }

    return 0;
}

int32_t zxdh_tc_flow_remove(struct zxdh_en_priv *priv, uint32_t location)
{
    int32_t err = 0;
    struct zxdh_en_device *en_dev = &priv->edev;
    DPP_PF_INFO_T pf_info = {0};
    DPP_STAT_VALUE_U stat_value = {0};
    
    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (location >=  ETHTOOL_FD_MAX_NUM)
        return -EINVAL;
    
    /* 清除vf的fd表 */
    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        err = zxdh_vf_del_fd(en_dev, location);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "zxdh_vf_del_fd failed!\n");
            return -EPERM;
        }
        return 0;
    }
    
    /* 清除pf的fd表 */
    err = dpp_tbl_fd_cfg_del(&pf_info, ZXDH_SDT_FD_CFG_TABLE, location);
    if (err != 0) {
        LOG_ERR_DEV(en_dev->parent, "del fd cfg failed!\n");
        return -EPERM;
    }
    
    /* 释放index */
    err = dpp_fd_acl_index_release(&pf_info, location);
    if (err)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to release index!!!\n");
        return -EPERM;
    }
    
    err = dpp_stat_item_cnt_get(&pf_info, DPP_STAT_ITEM_FD_FLOW_STAT, location, STAT_RD_CLR_MODE_CLR, &stat_value);
    if(err)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to clear stat!\n");
        return -EPERM;
    }
    
    return 0;
}

static int32_t zxdh_tc_vf_fd_stat_get(struct zxdh_en_device *en_dev, 
                                      uint32_t stat_item,
                                      uint32_t count_id, 
                                      uint32_t rd_clr,
                                      DPP_STAT_VALUE_U *stat_value)
{
    union zxdh_msg *msg = NULL;
    int32_t err = 0;
    struct zxdh_bar_extra_para para = {0};

    para.is_sync = true;
    para.retrycnt = BAR_MSG_RETRY_CNT_MAX;

    msg = kzalloc(sizeof(union zxdh_msg), GFP_KERNEL);
    if (msg == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "kzalloc(%lu, GFP_KERNEL) failed !",sizeof(union zxdh_msg));
        return -1;
    }
    msg->payload.hdr.op_code = ZXDH_VF_PPU_STAT;
    msg->payload.hdr.vport = en_dev->vport;
    msg->payload.hdr.pcie_id = en_dev->pcie_id;

    msg->payload.vf_ppu_stat_msg.index = count_id;
    msg->payload.vf_ppu_stat_msg.rd_clr = rd_clr;
    msg->payload.vf_ppu_stat_msg.stat_item = stat_item;

    err = en_dev->ops->msg_send_cmd(en_dev->parent, MODULE_VF_BAR_MSG_TO_PF, msg, msg, &para);
    if (err != 0) {
        LOG_ERR_DEV(en_dev->parent, "send_command_to_pf_np failed: %d\n", err);
        kfree(msg);
        return err;
    }

    stat_value->stat_cnt_128.bytes = msg->reps.vf_ppu_stat_rsp.byte_cnt;
    stat_value->stat_cnt_128.pkts = msg->reps.vf_ppu_stat_rsp.pkt_cnt;

    kfree(msg);
    return err;
}

int32_t zxdh_tc_flow_stat(struct zxdh_en_priv *priv, uint32_t count_id, uint64_t *pkts, uint64_t *bytes)
{
    int32_t err = 0;
    DPP_PF_INFO_T pf_info = {0};
    DPP_STAT_VALUE_U stat_value = {0};
    struct zxdh_en_device *en_dev = &priv->edev;

    pf_info.slot = en_dev->slot_id;
    pf_info.vport = en_dev->vport;

    if (count_id >=  ETHTOOL_FD_MAX_NUM)
        return -EINVAL;

    
    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        err = zxdh_tc_vf_fd_stat_get(en_dev,DPP_STAT_ITEM_FD_FLOW_STAT,count_id, STAT_RD_CLR_MODE_UNCLR, &stat_value);
    }
    else
    {
        err = dpp_stat_item_cnt_get(&pf_info, DPP_STAT_ITEM_FD_FLOW_STAT, count_id, STAT_RD_CLR_MODE_UNCLR, &stat_value);
    }

    if(err)
        return -EPERM;
    
    *bytes = stat_value.stat_cnt_128.bytes;
    *pkts = stat_value.stat_cnt_128.pkts;
    
    return 0;
}

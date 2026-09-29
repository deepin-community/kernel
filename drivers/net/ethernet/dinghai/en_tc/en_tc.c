#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/rhashtable.h>
#include <net/tc_act/tc_csum.h>
#include <net/pkt_cls.h>
#include "en_tc.h"
#include "../en_aux.h"
#include "../slib.h"
#include "../en_sf/en_sf_eq.h"

static const struct rhashtable_params tc_ht_params = {
    .head_offset = offsetof(struct zxdh_tc_flow, node),
    .key_offset = offsetof(struct zxdh_tc_flow, cookie),
    .key_len = sizeof(((struct zxdh_tc_flow *)0)->cookie),
    .automatic_shrinking = true,
};

#ifdef HAVE_TC_FLOW_OFFLOAD
static int pedit_header_offsets[] = {
    [FLOW_ACT_MANGLE_HDR_TYPE_ETH] = offsetof(struct pedit_headers, eth),
    [FLOW_ACT_MANGLE_HDR_TYPE_IP4] = offsetof(struct pedit_headers, ip4),
    [FLOW_ACT_MANGLE_HDR_TYPE_IP6] = offsetof(struct pedit_headers, ip6),
    [FLOW_ACT_MANGLE_HDR_TYPE_TCP] = offsetof(struct pedit_headers, tcp),
    [FLOW_ACT_MANGLE_HDR_TYPE_UDP] = offsetof(struct pedit_headers, udp),
};

#define pedit_header(_ph, _htype) ((void *)(_ph) + pedit_header_offsets[_htype])

static int set_pedit_val(uint8_t hdr_type, uint32_t mask, uint32_t val, uint32_t offset,
                         struct pedit_headers_action *hdrs)
{
    uint32_t *curr_pmask, *curr_pval;

    curr_pmask = (uint32_t *)(pedit_header(&hdrs->masks, hdr_type) + offset);
    curr_pval  = (uint32_t *)(pedit_header(&hdrs->vals, hdr_type) + offset);

    if (*curr_pmask & mask)  /* disallow acting twice on the same location */
        goto out_err;

    *curr_pmask |= mask;
    *curr_pval  |= (val & mask);

    return 0;

out_err:
    return -EOPNOTSUPP;
}

static struct zxdh_tc_flow *zxdh_flow_get(struct zxdh_tc_flow *flow)
{
    if (!flow || !refcount_inc_not_zero(&flow->refcnt))
        return ERR_PTR(-EINVAL);
    return flow;
}
#endif

static struct rhashtable *get_tc_ht(struct zxdh_en_priv *priv, unsigned long flags)
{
    return &priv->edev.fs.tc.ht;
}

int32_t zxdh_tc_num_filters(struct zxdh_en_priv *priv, unsigned long flags)
{
    struct rhashtable *tc_ht = get_tc_ht(priv, flags);

    if (IS_ERR_OR_NULL(tc_ht))
        return 0;

    return atomic_read(&tc_ht->nelems);
}

#ifdef HAVE_TC_FLOW_OFFLOAD
static void zxdh_tc_del_nic_flow(struct zxdh_en_priv *priv,
                                 struct zxdh_tc_flow *flow)
{
    struct zxdh_nic_flow_attr *attr = flow->nic_attr;
    int32_t err = 0;
    uint32_t index = 0;
    
    if(!IS_ERR_OR_NULL(attr->counter))
    {
        index = attr->counter->id;
        LOG_INFO_DEV(priv->edev.parent, "zxdh_tc_del_nic_flow, rule_index = %d index=0x%x\n",
                    attr->rule_index, index);
        err = zxdh_tc_flow_remove(priv, index);
        if(err)
        {
            LOG_ERR_DEV(priv->edev.parent, "zxdh_tc_flow_remove failed, rule_index = %d\n", attr->rule_index);
        }

        kfree(attr->counter);
    }
    
    kvfree(attr->parse_attr);
    
    return;
}

static void zxdh_tc_del_flow(struct zxdh_en_priv *priv,
                             struct zxdh_tc_flow *flow)
{
    zxdh_tc_del_nic_flow(priv, flow);
}

void zxdh_flow_put_lock(struct zxdh_en_priv *priv,
                        struct zxdh_tc_flow *flow, bool lock)
{
    if (refcount_dec_and_test(&flow->refcnt)) 
    {
        if (flow->dep_lock && lock)
            spin_lock(flow->dep_lock);
        if (!list_empty(&flow->nft_node))
            list_del_init(&flow->nft_node);
        if (flow->dep_lock && lock)
            spin_unlock(flow->dep_lock);
        zxdh_tc_del_flow(priv, flow);
        kfree_rcu(flow, rcu_head);
    }
}

void zxdh_flow_put(struct zxdh_en_priv *priv,
                   struct zxdh_tc_flow *flow)
{
    zxdh_flow_put_lock(priv, flow, true);
}

bool zxdh_is_eswitch_flow(struct zxdh_tc_flow *flow)
{
    return flow_flag_test(flow, ESWITCH);
}

bool zxdh_is_simple_flow(struct zxdh_tc_flow *flow)
{
    return flow_flag_test(flow, SIMPLE);
}

bool zxdh_is_offloaded_flow(struct zxdh_tc_flow *flow)
{
    return flow_flag_test(flow, OFFLOADED);
}

int zxdh_get_flow_namespace(struct zxdh_tc_flow *flow)
{
    return zxdh_is_eswitch_flow(flow) ?
        ZXDH_FLOW_NAMESPACE_FDB : ZXDH_FLOW_NAMESPACE_KERNEL;
}

static void get_flags(int flags, unsigned long *flow_flags)
{
    unsigned long __flow_flags = 0;

    if (flags & ZXDH_TC_FLAG(INGRESS))
        __flow_flags |= BIT(ZXDH_TC_FLOW_FLAG_INGRESS);
    if (flags & ZXDH_TC_FLAG(EGRESS))
        __flow_flags |= BIT(ZXDH_TC_FLOW_FLAG_EGRESS);
    if (flags & ZXDH_TC_FLAG(ESW_OFFLOAD))
        __flow_flags |= BIT(ZXDH_TC_FLOW_FLAG_ESWITCH);
    if (flags & ZXDH_TC_FLAG(NIC_OFFLOAD))
        __flow_flags |= BIT(ZXDH_TC_FLOW_FLAG_NIC);

    *flow_flags = __flow_flags;
}

static void zxdh_init_flow(struct zxdh_en_priv *priv, int attr_size,
                           uint64_t cookie, unsigned long flow_flags,
                           struct zxdh_tc_flow_parse_attr *parse_attr,
                           struct zxdh_tc_flow *flow)
{
    int out_index = 0;

    flow->flags = flow_flags;
    flow->cookie = cookie;
    flow->priv = priv;

    for (out_index = 0; out_index < ZXDH_MAX_FLOW_FWD_VPORTS; out_index++)
        INIT_LIST_HEAD(&flow->encaps[out_index].list);
    INIT_LIST_HEAD(&flow->mod_hdr);
    INIT_LIST_HEAD(&flow->hairpin);
    refcount_set(&flow->refcnt, 1);
    init_completion(&flow->init_done);
    INIT_LIST_HEAD(&flow->miniflow_list);
    INIT_LIST_HEAD(&flow->nft_node);

    flow->nic_attr->parse_attr = parse_attr;
}

static bool skip_key_basic(struct net_device *filter_dev,
                           struct flow_cls_offload *f)
{
    /* When doing mpls over udp decap, the user needs to provide
     * MPLS_UC as the protocol in order to be able to match on mpls
     * label fields.  However, the actual ethertype is IP so we want to
     * avoid matching on this, otherwise we'll fail the match.
     */
    //if (netif_is_bareudp(filter_dev) && f->common.chain_index == 0)
        //return true;

    return false;
}

static int parse_tunnel_attr(struct zxdh_flow_spec *spec,
                             struct flow_rule *rule,
                             struct net_device *filter_dev,
                             uint8_t *match_level)
{
    struct flow_match_ports enc_ports;
    struct flow_match_enc_keyid enc_keyid;
    struct flow_match_control enc_control;
    struct flow_match_ipv4_addrs ipv4_match;
    struct flow_match_ipv6_addrs ipv6_match;
    struct flow_match_ip ip_match;

    *match_level = ZXDH_MATCH_L4;
    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_ENC_PORTS))
    {
        flow_rule_match_enc_ports(rule, &enc_ports);
        if (memchr_inv(&enc_ports.mask->dst, 0xff,sizeof(enc_ports.mask->dst)))
        {
            LOG_ERR("UDP tunnel decap filter must match enc_dst_port fully\n");
            return -EOPNOTSUPP;
        }
        spec->outer_header.key.ip_protocol = IPPROTO_UDP;
        spec->outer_header.mask.ip_protocol = 0xffff;
        spec->outer_header.key.udp_dport = ntohs(enc_ports.key->dst);
        spec->outer_header.mask.udp_dport = ntohs(enc_ports.mask->dst);
        spec->outer_header.key.udp_sport = ntohs(enc_ports.key->src);
        spec->outer_header.mask.udp_sport = ntohs(enc_ports.mask->src);
    }

    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_ENC_KEYID))
    {
        flow_rule_match_enc_keyid(rule, &enc_keyid);
        if (enc_keyid.mask->keyid)
        {
            spec->misc_parameter.key.vxlan_vni = be32_to_cpu(enc_keyid.key->keyid);
            spec->misc_parameter.mask.vxlan_vni = be32_to_cpu(enc_keyid.mask->keyid);
        }
    }

    flow_rule_match_enc_control(rule, &enc_control);
    if (enc_control.key->addr_type == FLOW_DISSECTOR_KEY_IPV4_ADDRS)
    {
        flow_rule_match_ipv4_addrs(rule, &ipv4_match);
        zte_memcpy_s(spec->outer_header.key.dst_ip,&(ipv4_match.key->dst),sizeof(ipv4_match.key->dst));
        zte_memcpy_s(spec->outer_header.mask.dst_ip,&(ipv4_match.mask->dst),sizeof(ipv4_match.mask->dst));
        zte_memcpy_s(spec->outer_header.key.src_ip,&(ipv4_match.key->src),sizeof(ipv4_match.key->src));
        zte_memcpy_s(spec->outer_header.mask.src_ip,&(ipv4_match.mask->src),sizeof(ipv4_match.mask->src));
        spec->outer_header.key.ethertype = ETH_P_IP;
        spec->outer_header.mask.ethertype = 0xffff;
    }

    if (enc_control.key->addr_type == FLOW_DISSECTOR_KEY_IPV6_ADDRS)
    {
        flow_rule_match_ipv6_addrs(rule, &ipv6_match);
        zte_memcpy_s(spec->outer_header.key.dst_ip,&(ipv6_match.key->dst),sizeof(ipv6_match.key->dst));
        zte_memcpy_s(spec->outer_header.mask.dst_ip,&(ipv6_match.mask->dst),sizeof(ipv6_match.mask->dst));
        zte_memcpy_s(spec->outer_header.key.src_ip,&(ipv6_match.key->src),sizeof(ipv6_match.key->src));
        zte_memcpy_s(spec->outer_header.mask.src_ip,&(ipv6_match.mask->src),sizeof(ipv6_match.mask->src));
        spec->outer_header.key.ethertype = ETH_P_IPV6;
        spec->outer_header.mask.ethertype = 0xffff;
    }

    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_ENC_IP))
    {
        flow_rule_match_enc_ip(rule, &ip_match);
        spec->outer_header.key.ip_ecn = (ip_match.key->tos) & 0x3;
        spec->outer_header.mask.ip_ecn = (ip_match.mask->tos) & 0x3;

        spec->outer_header.key.ip_dscp = (ip_match.key->tos) >> 2;
        spec->outer_header.mask.ip_dscp = (ip_match.mask->tos) >> 2;

        spec->outer_header.key.ttl_hoplimit = ip_match.key->ttl;
        spec->outer_header.mask.ttl_hoplimit = ip_match.mask->ttl;
    }

    /* Enforce DMAC when offloading incoming tunneled flows.
     * Flow counters require a match on the DMAC.
    */
    ether_addr_copy(spec->outer_header.key.dmac, filter_dev->dev_addr);
    zte_memset_s(spec->outer_header.mask.dmac,0xff,ETH_ALEN);

    /* let software handle IP fragments */
    spec->outer_header.key.frag = 0;
    spec->outer_header.mask.frag = 0xff;

    return 0;
}

static int parse_cls_flower_tunnel(struct zxdh_flow_spec *spec,
                                  struct flow_rule *rule,
                                  struct net_device *filter_dev,
                                  uint8_t *outer_match_level)
{
    struct flow_match_control match = {0};

    flow_rule_match_enc_control(rule, &match);
    switch (match.key->addr_type)
    {
        case FLOW_DISSECTOR_KEY_IPV4_ADDRS:
        case FLOW_DISSECTOR_KEY_IPV6_ADDRS:
        {
            if (parse_tunnel_attr(spec, rule, filter_dev, outer_match_level))
                return -EOPNOTSUPP;
            break;
        }
        default:
            return -EOPNOTSUPP;
    }
    return 0;
}

static void parse_cls_flower_vlan(struct zxdh_ifc_lyr2_4_param *header,
                                  struct flow_rule *rule,
                                  struct net_device *filter_dev,
                                  uint8_t *match_level)
{
    struct flow_dissector_key_vlan filter_dev_mask = {0};
    struct flow_dissector_key_vlan filter_dev_key = {0};
    struct flow_match_vlan match = {0};

    if (is_vlan_dev(filter_dev))
    {
        match.key = &filter_dev_key;
        match.key->vlan_id = vlan_dev_vlan_id(filter_dev);
        match.key->vlan_tpid = vlan_dev_vlan_proto(filter_dev);
        match.key->vlan_priority = 0;
        match.mask = &filter_dev_mask;
        zte_memset_s(match.mask, 0xff, sizeof(*match.mask));
        match.mask->vlan_priority = 0;
    }
    else
    {
        flow_rule_match_vlan(rule, &match);
    }

    if (match.mask->vlan_id ||
         match.mask->vlan_priority ||
         match.mask->vlan_tpid)
    {
        if (match.key->vlan_tpid == htons(ETH_P_8021AD))
        {
            header->key.svlan_tag = 1;
            header->mask.svlan_tag = 1;
        }
        else
        {
            header->key.cvlan_tag = 1;
            header->mask.cvlan_tag = 1;
        }

        header->key.vlan_id = match.key->vlan_id;
        header->mask.vlan_id = match.mask->vlan_id;
        header->key.vlan_prio = match.key->vlan_priority;
        header->mask.vlan_prio = match.mask->vlan_priority;

        *match_level = ZXDH_MATCH_L2;
    }

    return;
}

static void parse_cls_flower_cvlan(struct zxdh_flow_spec *spec,
                                  struct flow_rule *rule,
                                  uint8_t *match_level)
{
    struct flow_match_vlan match = {0};

    flow_rule_match_cvlan(rule, &match);
    if (match.mask->vlan_id ||
        match.mask->vlan_priority ||
        match.mask->vlan_tpid)
    {
        if (match.key->vlan_tpid == htons(ETH_P_8021AD))
        {
            spec->misc_parameter.key.outer_second_svlan_tag = 1;
            spec->misc_parameter.mask.outer_second_svlan_tag = 1;
        }
        else
        {
            spec->misc_parameter.key.outer_second_cvlan_tag = 1;
            spec->misc_parameter.mask.outer_second_cvlan_tag = 1;
        }

        spec->misc_parameter.key.outer_second_vid = match.key->vlan_id;
        spec->misc_parameter.mask.outer_second_vid = match.mask->vlan_id;
        spec->misc_parameter.key.outer_second_prio = match.key->vlan_priority;
        spec->misc_parameter.mask.outer_second_prio = match.mask->vlan_priority;

        *match_level = ZXDH_MATCH_L2;
        spec->match_criteria_enable |= ZXDH_MATCH_MISC_PARAMETERS;
    }
}

static void parse_cls_flower_eth_addrs(struct zxdh_ifc_lyr2_4_param *header,
                                  struct flow_rule *rule,
                                  uint8_t *match_level)
{
    struct flow_match_eth_addrs match = {0};

    flow_rule_match_eth_addrs(rule, &match);

    ether_addr_copy(header->key.dmac,match.key->dst);
    ether_addr_copy(header->mask.dmac,match.mask->dst);

    ether_addr_copy(header->key.smac,match.key->src);
    ether_addr_copy(header->mask.smac,match.mask->src);

    if (!is_zero_ether_addr(match.mask->src) || !is_zero_ether_addr(match.mask->dst))
    {
        *match_level = ZXDH_MATCH_L2;
    }
}

static int parse_cls_flower_control(struct zxdh_ifc_lyr2_4_param *header,
                                  struct flow_rule *rule,
                                  uint8_t *match_level,
                                  uint16_t *addr_type)
{
    struct flow_match_control match = {0};

    flow_rule_match_control(rule, &match);
    *addr_type = match.key->addr_type;

    /* the HW doesn't support frag first/later */
    if (match.mask->flags & FLOW_DIS_FIRST_FRAG)
    {
        LOG_ERR("Match on frag first/later is not supported");
        return -EOPNOTSUPP;
    }

    if (match.mask->flags & FLOW_DIS_IS_FRAGMENT)
    {
        header->key.frag = match.key->flags & FLOW_DIS_IS_FRAGMENT;
        header->mask.frag = 1;

        /* the HW doesn't need L3 inline to match on frag=no */
        if (!(match.key->flags & FLOW_DIS_IS_FRAGMENT))
        {
            *match_level = ZXDH_MATCH_L2;
        }
        /* ***  L2 attributes parsing up to here *** */
        else
        {
            *match_level = ZXDH_MATCH_L3;
        }
    }

    return 0;
}

static void parse_cls_flower_basic(struct zxdh_ifc_lyr2_4_param *header,
                                  struct flow_rule *rule,
                                  uint8_t *ip_proto,
                                  uint8_t *match_level,
                                  uint16_t addr_type)
{
    struct flow_match_basic match = {0};
    struct flow_match_ipv4_addrs ipv4_match = {0};
    struct flow_match_ipv6_addrs ipv6_match = {0};

    flow_rule_match_basic(rule, &match);
    *ip_proto = match.key->ip_proto;

    header->key.ip_protocol = match.key->ip_proto;
    header->mask.ip_protocol = match.mask->ip_proto;

    if (match.mask->ip_proto)
    {
        *match_level = ZXDH_MATCH_L3;
    }

    if (addr_type == FLOW_DISSECTOR_KEY_IPV4_ADDRS)
    {
        flow_rule_match_ipv4_addrs(rule, &ipv4_match);
        zte_memcpy_s(header->key.dst_ip,&(ipv4_match.key->dst),sizeof(ipv4_match.key->dst));
        zte_memcpy_s(header->mask.dst_ip,&(ipv4_match.mask->dst),sizeof(ipv4_match.mask->dst));
        zte_memcpy_s(header->key.src_ip,&(ipv4_match.key->src),sizeof(ipv4_match.key->src));
        zte_memcpy_s(header->mask.src_ip,&(ipv4_match.mask->src),sizeof(ipv4_match.mask->src));
        header->key.ethertype = ETH_P_IP;
        header->mask.ethertype = 0xffff;
        if (ipv4_match.mask->src || ipv4_match.mask->dst)
        {
            *match_level = ZXDH_MATCH_L3;
        }
    }

    if (addr_type == FLOW_DISSECTOR_KEY_IPV6_ADDRS)
    {
        flow_rule_match_ipv6_addrs(rule, &ipv6_match);
        zte_memcpy_s(header->key.dst_ip,&(ipv6_match.key->dst),sizeof(ipv6_match.key->dst));
        zte_memcpy_s(header->mask.dst_ip,&(ipv6_match.mask->dst),sizeof(ipv6_match.mask->dst));
        zte_memcpy_s(header->key.src_ip,&(ipv6_match.key->src),sizeof(ipv6_match.key->src));
        zte_memcpy_s(header->mask.src_ip,&(ipv6_match.mask->src),sizeof(ipv6_match.mask->src));
        header->key.ethertype = ETH_P_IPV6;
        header->mask.ethertype = 0xffff;
        if (ipv6_addr_type(&ipv6_match.mask->src) != IPV6_ADDR_ANY ||
            ipv6_addr_type(&ipv6_match.mask->dst) != IPV6_ADDR_ANY)
        {
            *match_level = ZXDH_MATCH_L3;
        }
    }

    return;
}

static void parse_cls_flower_ip(struct zxdh_ifc_lyr2_4_param *header,
                                  struct flow_rule *rule,
                                  uint8_t *match_level)
{
    struct flow_match_ip match;

    flow_rule_match_ip(rule, &match);

    header->key.ip_ecn = (match.key->tos) & 0x3;
    header->mask.ip_ecn = (match.mask->tos) & 0x3;

    header->key.ip_dscp = (match.key->tos) >> 2;
    header->mask.ip_dscp = (match.mask->tos) >> 2;

    header->key.ttl_hoplimit = match.key->ttl;
    header->mask.ttl_hoplimit = match.mask->ttl;

    if (match.mask->tos || match.mask->ttl)
    {
        *match_level = ZXDH_MATCH_L3;
    }

    return;

}

static int parse_cls_flower_ports(struct zxdh_ifc_lyr2_4_param *header,
                                  struct flow_rule *rule,
                                  uint8_t ip_proto,
                                  uint8_t *match_level)
{
    struct flow_match_ports match = {0};

    flow_rule_match_ports(rule, &match);
    switch (ip_proto)
    {
        case IPPROTO_TCP:
        {
            header->key.tcp_sport = ntohs(match.key->src);
            header->mask.tcp_sport = ntohs(match.mask->src);
            header->key.tcp_dport = ntohs(match.key->dst);
            header->mask.tcp_dport = ntohs(match.mask->dst);
            break;
        }
        case IPPROTO_UDP:
        {
            header->key.udp_sport = ntohs(match.key->src);
            header->mask.udp_sport = ntohs(match.mask->src);
            header->key.udp_dport = ntohs(match.key->dst);
            header->mask.udp_dport = ntohs(match.mask->dst);
            break;
        }
        default:
        {
            LOG_ERR("Only UDP and TCP transport are supported\n");
            return -EINVAL;
        }
    }

    if (match.mask->src || match.mask->dst)
    {
        *match_level = ZXDH_MATCH_L4;
    }

    return 0;
}

static void parse_cls_flower_tcp(struct zxdh_ifc_lyr2_4_param *header,
                                  struct flow_rule *rule,
                                  uint8_t *match_level)
{
    struct flow_match_tcp match;

    flow_rule_match_tcp(rule, &match);

    header->key.tcp_flags = ntohs(match.key->flags);
    header->mask.tcp_flags = ntohs(match.mask->flags);

    if (match.mask->flags)
    {
        *match_level = ZXDH_MATCH_L4;
    }

	return;
}

static bool flow_user_keys_valid(uint32_t used_keys)
{
    if (used_keys &
         ~(BIT(FLOW_DISSECTOR_KEY_CONTROL) |
           BIT(FLOW_DISSECTOR_KEY_BASIC) |
           BIT(FLOW_DISSECTOR_KEY_ETH_ADDRS) |
           BIT(FLOW_DISSECTOR_KEY_VLAN) |
           BIT(FLOW_DISSECTOR_KEY_CVLAN) |
           BIT(FLOW_DISSECTOR_KEY_IPV4_ADDRS) |
           BIT(FLOW_DISSECTOR_KEY_IPV6_ADDRS) |
           BIT(FLOW_DISSECTOR_KEY_PORTS) |
           BIT(FLOW_DISSECTOR_KEY_ENC_KEYID) |
           BIT(FLOW_DISSECTOR_KEY_ENC_IPV4_ADDRS) |
           BIT(FLOW_DISSECTOR_KEY_ENC_IPV6_ADDRS) |
           BIT(FLOW_DISSECTOR_KEY_ENC_PORTS)      |
           BIT(FLOW_DISSECTOR_KEY_ENC_CONTROL) |
           BIT(FLOW_DISSECTOR_KEY_TCP) |
           BIT(FLOW_DISSECTOR_KEY_IP)  |
           BIT(FLOW_DISSECTOR_KEY_ENC_IP) |
           BIT(FLOW_DISSECTOR_KEY_ENC_OPTS)))
    {
        return false;
    }

    return true;
}

static void *get_match_headers_value(uint32_t flags,
                           struct zxdh_flow_spec *spec)
{
    return (flags & ZXDH_FLOW_CONTEXT_ACTION_DECAP) ?
           (void *)(&spec->inner_header.key) : (void *)(&spec->outer_header.key);
}

static int __parse_cls_flower(struct zxdh_flow_spec *spec,
                              struct flow_cls_offload *f,
                              struct net_device *filter_dev,
                              uint8_t *inner_match_level,
                              uint8_t *outer_match_level,
                              bool *is_tunnel_flow)
{
    struct flow_rule *rule = flow_cls_offload_flow_rule(f);
    struct flow_dissector *dissector = rule->match.dissector;
    struct zxdh_ifc_lyr2_4_param *header = &spec->outer_header;
    struct flow_match_basic basic_match = {0};
    uint8_t *match_level = NULL;
    uint16_t addr_type = 0;
    uint8_t ip_proto = 0;
    int err = 0;

    match_level = outer_match_level;
    LOG_DEBUG("Supported key used: 0x%x\n",(uint32_t)dissector->used_keys);

    if (!flow_user_keys_valid((uint32_t)dissector->used_keys))
    {
        LOG_ERR("Unsupported key used: 0x%x\n",(uint32_t)dissector->used_keys);
        return -EOPNOTSUPP;
    }

    if ((flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_ENC_IPV4_ADDRS) ||
         flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_ENC_KEYID) ||
         flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_ENC_PORTS) ||
         flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_ENC_OPTS)) &&
         flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_ENC_CONTROL))
    {
        err = parse_cls_flower_tunnel(spec, rule, filter_dev, outer_match_level);
        if(!err)
        {
            LOG_ERR("parse_cls_flower_tunnel failed\n");
            return err;
        }

        /* At this point, header pointers should point to the inner
         * headers, outer header were already set by parse_tunnel_attr
         */
        match_level = inner_match_level;
        header = &spec->inner_header;
        *is_tunnel_flow = true;
    }

    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_BASIC) && (!skip_key_basic(filter_dev, f)))
    {
        flow_rule_match_basic(rule, &basic_match);
        header->key.ethertype = ntohs(basic_match.key->n_proto);
        header->mask.ethertype = ntohs(basic_match.mask->n_proto);
        if (basic_match.mask->n_proto)
            *match_level = ZXDH_MATCH_L2;
    }

    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_VLAN) || is_vlan_dev(filter_dev))
    {
        parse_cls_flower_vlan(header,rule,filter_dev,match_level);
    }
    else if (*match_level != ZXDH_MATCH_NONE)
    {
        header->mask.cvlan_tag = 0xff;
        *match_level = ZXDH_MATCH_L2;
    }

    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_CVLAN))
    {
        parse_cls_flower_cvlan(spec,rule,match_level);
    }

    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_ETH_ADDRS))
    {
        parse_cls_flower_eth_addrs(header,rule,match_level);
    }

    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_CONTROL))
    {
        err = parse_cls_flower_control(header,rule,match_level,&addr_type);
        if (err)
        {
            return err;
        }
    }

    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_BASIC))
    {
        parse_cls_flower_basic(header,rule,&ip_proto,match_level,addr_type);
    }

    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_IP))
    {
        parse_cls_flower_ip(header,rule,match_level);
    }

    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_PORTS))
    {
        err = parse_cls_flower_ports(header,rule,ip_proto,match_level);
        if (err)
        {
            return err;
        }
    }

    if (flow_rule_match_key(rule, FLOW_DISSECTOR_KEY_TCP))
    {
        parse_cls_flower_tcp(header,rule,match_level);
    }

    return 0;
}

static void cls_flower_prt(struct zxdh_flow_spec *spec, uint8_t non_tunnel_match_level)
{
    LOG_DEBUG("match_criteria_enable=0x%x match_level=%u\n",spec->match_criteria_enable,non_tunnel_match_level);
    LOG_DEBUG("key.dmac=0x%02x-%02x-%02x-%02x-%02x-%02x\n",spec->outer_header.key.dmac[0],spec->outer_header.key.dmac[1],
                                            spec->outer_header.key.dmac[2],spec->outer_header.key.dmac[3],
                                            spec->outer_header.key.dmac[4],spec->outer_header.key.dmac[5]);
    LOG_DEBUG("mask.dmac=0x%02x-%02x-%02x-%02x-%02x-%02x\n",spec->outer_header.mask.dmac[0],spec->outer_header.mask.dmac[1],
                                            spec->outer_header.mask.dmac[2],spec->outer_header.mask.dmac[3],
                                            spec->outer_header.mask.dmac[4],spec->outer_header.mask.dmac[5]);
    LOG_DEBUG("key.smac=0x%02x-%02x-%02x-%02x-%02x-%02x\n",spec->outer_header.key.smac[0],spec->outer_header.key.smac[1],
                                            spec->outer_header.key.smac[2],spec->outer_header.key.smac[3],
                                            spec->outer_header.key.smac[4],spec->outer_header.key.smac[5]);
    LOG_DEBUG("mask.smac=0x%02x-%02x-%02x-%02x-%02x-%02x\n",spec->outer_header.mask.smac[0],spec->outer_header.mask.smac[1],
                                            spec->outer_header.mask.smac[2],spec->outer_header.mask.smac[3],
                                            spec->outer_header.mask.smac[4],spec->outer_header.mask.smac[5]);
    LOG_DEBUG("key.ethertype=0x%x mask.ethertype=0x%x\n",spec->outer_header.key.ethertype,spec->outer_header.mask.ethertype);
    LOG_DEBUG("key.vlan_id=0x%x mask.vlan_id=0x%x\n",spec->outer_header.key.vlan_id,spec->outer_header.mask.vlan_id);
    LOG_DEBUG("key.vlan_prio=0x%x mask.vlan_prio=0x%x\n",spec->outer_header.key.vlan_prio,spec->outer_header.mask.vlan_prio);
    LOG_DEBUG("key.ip_protocol=0x%x mask.ip_protocol=0x%x\n",spec->outer_header.key.ip_protocol,spec->outer_header.mask.ip_protocol);
    LOG_DEBUG("key.ip_dscp=0x%x mask.ip_dscp=0x%x\n",spec->outer_header.key.ip_dscp,spec->outer_header.mask.ip_dscp);
    LOG_DEBUG("key.ip_ecn=0x%x mask.ip_ecn=0x%x\n",spec->outer_header.key.ip_ecn,spec->outer_header.mask.ip_ecn);
    LOG_DEBUG("key.cvlan_tag=0x%x mask.cvlan_tag=0x%x\n",spec->outer_header.key.cvlan_tag,spec->outer_header.mask.cvlan_tag);
    LOG_DEBUG("key.svlan_tag=0x%x mask.svlan_tag=0x%x\n",spec->outer_header.key.svlan_tag,spec->outer_header.mask.svlan_tag);
    LOG_DEBUG("key.frag=0x%x mask.frag=0x%x\n",spec->outer_header.key.frag,spec->outer_header.mask.frag);
    LOG_DEBUG("key.ip_version=0x%x mask.ip_version=0x%x\n",spec->outer_header.key.ip_version,spec->outer_header.mask.ip_version);
    LOG_DEBUG("key.tcp_flags=0x%x mask.tcp_flags=0x%x\n",spec->outer_header.key.tcp_flags,spec->outer_header.mask.tcp_flags);
    LOG_DEBUG("key.tcp_dport=0x%x mask.tcp_dport=0x%x\n",spec->outer_header.key.tcp_dport,spec->outer_header.mask.tcp_dport);
    LOG_DEBUG("key.tcp_sport=0x%x mask.tcp_sport=0x%x\n",spec->outer_header.key.tcp_sport,spec->outer_header.mask.tcp_sport);
    LOG_DEBUG("key.ipv4_ihl=0x%x mask.ipv4_ihl=0x%x\n",spec->outer_header.key.ipv4_ihl,spec->outer_header.mask.ipv4_ihl);
    LOG_DEBUG("key.ttl_hoplimit=0x%x mask.ttl_hoplimit=0x%x\n",spec->outer_header.key.ttl_hoplimit,spec->outer_header.mask.ttl_hoplimit);
    LOG_DEBUG("key.udp_dport=0x%x mask.udp_dport=0x%x\n",spec->outer_header.key.udp_dport,spec->outer_header.mask.udp_dport);
    LOG_DEBUG("key.udp_sport=0x%x mask.udp_sport=0x%x\n",spec->outer_header.key.udp_sport,spec->outer_header.mask.udp_sport);
    LOG_DEBUG("key.dst_ip=0x%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
                spec->outer_header.key.dst_ip[0],spec->outer_header.key.dst_ip[1],spec->outer_header.key.dst_ip[2],spec->outer_header.key.dst_ip[3],
                spec->outer_header.key.dst_ip[4],spec->outer_header.key.dst_ip[5],spec->outer_header.key.dst_ip[6],spec->outer_header.key.dst_ip[7],
                spec->outer_header.key.dst_ip[8],spec->outer_header.key.dst_ip[9],spec->outer_header.key.dst_ip[10],spec->outer_header.key.dst_ip[11],
                spec->outer_header.key.dst_ip[12],spec->outer_header.key.dst_ip[13],spec->outer_header.key.dst_ip[14],spec->outer_header.key.dst_ip[15]);
    LOG_DEBUG("mask.dst_ip=0x%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
                spec->outer_header.mask.dst_ip[0],spec->outer_header.mask.dst_ip[1],spec->outer_header.mask.dst_ip[2],spec->outer_header.mask.dst_ip[3],
                spec->outer_header.mask.dst_ip[4],spec->outer_header.mask.dst_ip[5],spec->outer_header.mask.dst_ip[6],spec->outer_header.mask.dst_ip[7],
                spec->outer_header.mask.dst_ip[8],spec->outer_header.mask.dst_ip[9],spec->outer_header.mask.dst_ip[10],spec->outer_header.mask.dst_ip[11],
                spec->outer_header.mask.dst_ip[12],spec->outer_header.mask.dst_ip[13],spec->outer_header.mask.dst_ip[14],spec->outer_header.mask.dst_ip[15]);
    LOG_DEBUG("key.src_ip=0x%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
                spec->outer_header.key.src_ip[0],spec->outer_header.key.src_ip[1],spec->outer_header.key.src_ip[2],spec->outer_header.key.src_ip[3],
                spec->outer_header.key.src_ip[4],spec->outer_header.key.src_ip[5],spec->outer_header.key.src_ip[6],spec->outer_header.key.src_ip[7],
                spec->outer_header.key.src_ip[8],spec->outer_header.key.src_ip[9],spec->outer_header.key.src_ip[10],spec->outer_header.key.src_ip[11],
                spec->outer_header.key.src_ip[12],spec->outer_header.key.src_ip[13],spec->outer_header.key.src_ip[14],spec->outer_header.key.src_ip[15]);
    LOG_DEBUG("mask.src_ip=0x%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n",
                spec->outer_header.mask.src_ip[0],spec->outer_header.mask.src_ip[1],spec->outer_header.mask.src_ip[2],spec->outer_header.mask.src_ip[3],
                spec->outer_header.mask.src_ip[4],spec->outer_header.mask.src_ip[5],spec->outer_header.mask.src_ip[6],spec->outer_header.mask.src_ip[7],
                spec->outer_header.mask.src_ip[8],spec->outer_header.mask.src_ip[9],spec->outer_header.mask.src_ip[10],spec->outer_header.mask.src_ip[11],
                spec->outer_header.mask.src_ip[12],spec->outer_header.mask.src_ip[13],spec->outer_header.mask.src_ip[14],spec->outer_header.mask.src_ip[15]);

    return;
}

static int parse_cls_flower(struct zxdh_en_priv *priv,
                            struct zxdh_tc_flow *flow,
                            struct zxdh_flow_spec *spec,
                            struct flow_cls_offload *f,
                            struct net_device *filter_dev)
{
    uint8_t inner_match_level = ZXDH_MATCH_NONE;
    uint8_t outer_match_level = ZXDH_MATCH_NONE;
    uint8_t non_tunnel_match_level = ZXDH_MATCH_NONE;
    bool is_tunnel_flow = false;
    int err = 0;

    err = __parse_cls_flower(spec, f, filter_dev,
                             &inner_match_level, &outer_match_level,
                             &is_tunnel_flow);
    non_tunnel_match_level = (inner_match_level == ZXDH_MATCH_NONE) ?
                              outer_match_level : inner_match_level;

    flow->nic_attr->match_level = non_tunnel_match_level;
    flow->nic_attr->rule_index = f->common.prio;

    cls_flower_prt(spec,non_tunnel_match_level);

    return err;
}

int parse_tc_pedit_action(struct zxdh_en_priv *priv,
                        const struct flow_action_entry *act, int namespace,
                        struct zxdh_tc_flow_parse_attr *parse_attr,
                        struct pedit_headers_action *hdrs,
                        struct netlink_ext_ack *extack)
{
    uint8_t cmd = (act->id == FLOW_ACTION_MANGLE) ? 0 : 1;
    uint32_t mask = act->mangle.mask;
    uint32_t val = act->mangle.val;
    uint32_t offset = act->mangle.offset;
    uint8_t htype = act->mangle.htype;
    int err = -EOPNOTSUPP;

    if (htype == FLOW_ACT_MANGLE_UNSPEC)
    {
        LOG_ERR("legacy pedit isn't offloaded");
        goto out_err;
    }

    err = set_pedit_val(htype, ~mask, val, offset, &hdrs[cmd]);
    if (err)
        goto out_err;

    hdrs[cmd].pedits++;

    return 0;
out_err:
    return err;
}
#if 0
static int add_vlan_rewrite_action(struct zxdh_en_priv *priv, int namespace,
                                   const struct flow_action_entry *act,
                                   struct zxdh_tc_flow_parse_attr *parse_attr,
                                   struct pedit_headers_action *hdrs,
                                   uint32_t *action)
{
    uint16_t mask16 = VLAN_VID_MASK;
    uint16_t val16 = act->vlan.vid & VLAN_VID_MASK;
    const struct flow_action_entry pedit_act = {
        .id = FLOW_ACTION_MANGLE,
        .mangle.htype = FLOW_ACT_MANGLE_HDR_TYPE_ETH,
        .mangle.offset = offsetof(struct vlan_ethhdr, h_vlan_TCI),
        .mangle.mask = ~(uint32_t)be16_to_cpu(*(__be16 *)&mask16),
        .mangle.val = (uint32_t)be16_to_cpu(*(__be16 *)&val16),
    };
    uint8_t match_prio_mask = 0;
    uint8_t match_prio_val = 0;
    int err = 0;

    if (!(parse_attr->spec.outer_header.key.cvlan_tag &&
        parse_attr->spec.outer_header.mask.cvlan_tag)) {
        LOG_ERR("VLAN rewrite action must have VLAN protocol match");
        return -EOPNOTSUPP;
    }

    match_prio_mask = parse_attr->spec.outer_header.mask.vlan_prio;
    match_prio_val = parse_attr->spec.outer_header.key.vlan_prio;

    if (act->vlan.prio != (match_prio_val & match_prio_mask)) {
        LOG_ERR("Changing VLAN prio is not supported");
        return -EOPNOTSUPP;
    }
    
    err = parse_tc_pedit_action(priv, &pedit_act, namespace, parse_attr,
             hdrs, NULL);

    *action |= ZXDH_FLOW_CONTEXT_ACTION_MOD_HDR;

    return err;
}

static int add_vlan_prio_tag_rewrite_action(struct zxdh_en_priv *priv,
                                            struct zxdh_tc_flow_parse_attr *parse_attr,
                                            struct pedit_headers_action *hdrs,
                                            uint32_t *action, struct netlink_ext_ack *extack)
{
    struct zxdh_match_lyr_2_4_param *key_param =
        (struct zxdh_match_lyr_2_4_param *)get_match_headers_value(*action,&parse_attr->spec);
    const struct flow_action_entry prio_tag_act = {
        .vlan.vid = 0,
        .vlan.prio = key_param->vlan_prio,
    };

    return add_vlan_rewrite_action(priv, ZXDH_FLOW_NAMESPACE_FDB,
               &prio_tag_act, parse_attr, hdrs, action);
}

static bool csum_offload_supported(struct zxdh_en_priv *priv,
                                uint32_t action,
                                uint32_t update_flags,
                                struct netlink_ext_ack *extack)
{
    uint32_t prot_flags = TCA_CSUM_UPDATE_FLAG_IPV4HDR |
	                      TCA_CSUM_UPDATE_FLAG_TCP |
                          TCA_CSUM_UPDATE_FLAG_UDP;

    /*  The HW recalcs checksums only if re-writing headers */
    if (!(action & ZXDH_FLOW_CONTEXT_ACTION_MOD_HDR)) {
        LOG_ERR("TC csum action is only offloaded with pedit");
        return false;
    }

    if (update_flags & ~prot_flags) {
        LOG_ERR("can't offload TC csum action for some header/s");
        return false;
    }

    return true;
}
#endif

static bool is_action_keys_supported(const struct flow_action_entry *act)
{
    uint32_t mask = ~act->mangle.mask;
    uint32_t offset = act->mangle.offset;
    uint8_t htype = act->mangle.htype;

    /* For IPv4 & IPv6 header check 4 byte word,
     * to determine that modified fields
     * are NOT ttl & hop_limit only.
     */
    if (htype == FLOW_ACT_MANGLE_HDR_TYPE_IP4)
    {
        struct ip_ttl_word *ttl_word = (struct ip_ttl_word *)&mask;

        if (offset != offsetof(struct iphdr, ttl) ||
            ttl_word->protocol ||
            ttl_word->check)
        {
            return true;
        }
    }
    else if (htype == FLOW_ACT_MANGLE_HDR_TYPE_IP6)
    {
        struct ipv6_hoplimit_word *hoplimit_word =
            (struct ipv6_hoplimit_word *)&mask;

        if (offset != offsetof(struct ipv6hdr, payload_len) ||
            hoplimit_word->payload_len ||
            hoplimit_word->nexthdr)
        {
            return true;
        }
    }

    return false;
}

static bool modify_header_match_supported(struct zxdh_flow_spec *spec,
                                        struct flow_action *flow_action,
                                        uint32_t actions,
                                        struct netlink_ext_ack *extack)
{
    const struct flow_action_entry *act = NULL;
    bool modify_ip_header = false;
    struct zxdh_match_lyr_2_4_param *headers_v;
    uint16_t ethertype;
    uint8_t ip_proto;
    int i;

    headers_v = (struct zxdh_match_lyr_2_4_param *)get_match_headers_value(actions, spec);
    ethertype = headers_v->ethertype;

    /* for non-IP we only re-write MACs, so we're okay */
    if (ethertype != ETH_P_IP && ethertype != ETH_P_IPV6)
        goto out_ok;

    modify_ip_header = false;
    flow_action_for_each(i, act, flow_action)
    {
        if (act->id != FLOW_ACTION_MANGLE &&
            act->id != FLOW_ACTION_ADD)
            continue;

        if (is_action_keys_supported(act))
        {
            modify_ip_header = true;
            break;
        }
    }

    ip_proto = headers_v->ip_protocol;
    if (modify_ip_header && ip_proto != IPPROTO_TCP &&
        ip_proto != IPPROTO_UDP && ip_proto != IPPROTO_ICMP)
    {
        LOG_ERR("can't offload re-write of ip proto %d\n", ip_proto);
        return false;
    }

out_ok:
    return true;
}

static bool actions_match_supported(struct zxdh_en_priv *priv,
                                    struct flow_action *flow_action,
                                    struct zxdh_tc_flow_parse_attr *parse_attr,
                                    struct zxdh_tc_flow *flow,
                                    struct netlink_ext_ack *extack)
{
    uint32_t actions = flow->nic_attr->action;

    if (actions & ZXDH_FLOW_CONTEXT_ACTION_MOD_HDR)
    {
        return modify_header_match_supported(&parse_attr->spec,
                                             flow_action, actions,
                                             extack);
    }

    return true;
}

static bool same_hw_devs(struct zxdh_en_priv *priv, struct zxdh_en_priv *peer_priv)
{
    struct zxdh_en_device *en_dev = &priv->edev;
    struct zxdh_en_device *peer_en_dev = &peer_priv->edev;
    uint32_t fslot_id = 0;
    uint32_t pslot_id = 0;

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        fslot_id = zxdh_en_sf_get_slot_id(en_dev->parent);
    }
    else
    {
        fslot_id = en_dev->slot_id;
    }

    if(peer_en_dev->ops->get_coredev_type(peer_en_dev->parent) == DH_COREDEV_VF)
    {
        pslot_id = zxdh_en_sf_get_slot_id(peer_en_dev->parent);
    }
    else
    {
        pslot_id = peer_en_dev->slot_id;
    }
    
    LOG_DEBUG_DEV(en_dev->parent, "fslot_id=%d, pslot_id=%d\n", fslot_id, pslot_id);
    return (fslot_id == pslot_id);
}
#if 0
static int parse_tc_vlan_action(struct zxdh_en_priv *priv,
                                const struct flow_action_entry *act,
                                struct zxdh_nic_flow_attr *attr,
                                uint32_t *action,
                                struct zxdh_tc_flow_parse_attr *parse_attr,
                                struct netlink_ext_ack *extack)
{
    uint8_t vlan_idx = attr->total_vlan;

    if (vlan_idx >= ZXDH_FS_VLAN_DEPTH)
        return -EOPNOTSUPP;

    switch (act->id)
    {
        case FLOW_ACTION_VLAN_POP:
        {
            if (vlan_idx)
            {
                *action |= ZXDH_FLOW_CONTEXT_ACTION_VLAN_POP_2;
            }
            else
            {
                *action |= ZXDH_FLOW_CONTEXT_ACTION_VLAN_POP;
            }
            break;
        }
        case FLOW_ACTION_VLAN_PUSH:
        {
            attr->vlan_vid[vlan_idx] = act->vlan.vid;
            attr->vlan_prio[vlan_idx] = act->vlan.prio;
            attr->vlan_proto[vlan_idx] = act->vlan.proto;
            if (!attr->vlan_proto[vlan_idx])
                attr->vlan_proto[vlan_idx] = htons(ETH_P_8021Q);

            if (vlan_idx) {
                *action |= ZXDH_FLOW_CONTEXT_ACTION_VLAN_PUSH_2;
            }
            else
            {
                *action |= ZXDH_FLOW_CONTEXT_ACTION_VLAN_PUSH;
            }
            break;
        }
        default:
            return -EINVAL;
    }

    attr->total_vlan = vlan_idx + 1;

    return 0;
}
#endif
static int parse_tc_nic_actions(struct zxdh_en_priv *priv,
                                struct flow_action *flow_action,
                                struct zxdh_tc_flow_parse_attr *parse_attr,
                                struct zxdh_tc_flow *flow,
                                struct netlink_ext_ack *extack)
{
    struct zxdh_nic_flow_attr *attr = flow->nic_attr;
    const struct flow_action_entry *act = NULL;
    uint32_t action = 0;
    int i = 0;

    if (!flow_action_has_entries(flow_action))
        return -EINVAL;

    attr->flow_tag = ZXDH_FS_DEFAULT_FLOW_TAG;
    
    flow_action_for_each(i, act, flow_action)
    {
        switch (act->id)
        {
            case FLOW_ACTION_DROP:
            {
                action |= ZXDH_FLOW_CONTEXT_ACTION_DROP |
                          ZXDH_FLOW_CONTEXT_ACTION_COUNT;
                break;
            }
            case FLOW_ACTION_REDIRECT:
            {
                struct net_device *peer_dev = act->dev;

                if (priv->edev.netdev->netdev_ops == peer_dev->netdev_ops &&
                    same_hw_devs(priv, netdev_priv(peer_dev))) 
                {
                    parse_attr->mirred_ifindex[0] = peer_dev->ifindex;
                    flow_flag_set(flow, HAIRPIN);
                    action |= ZXDH_FLOW_CONTEXT_ACTION_FWD_DEST |
                               ZXDH_FLOW_CONTEXT_ACTION_COUNT;
                }
                else
                {
                    LOG_ERR_DEV(priv->edev.parent, "device is not on same HW, can't offload");
                    return -EINVAL;
                }
                 break;
            }
            default:
            {
                LOG_ERR_DEV(priv->edev.parent, "The offload action is not supported");
                return -EOPNOTSUPP;
            }
        }
    }

    attr->action = action;
    if (!actions_match_supported(priv, flow_action, parse_attr, flow, extack))
        return -EOPNOTSUPP;

    return 0;
}

static struct zxdh_fc *zxdh_fc_create(void)
{
    struct zxdh_fc *counter;

    counter = kzalloc(sizeof(*counter), GFP_KERNEL);
    if (!counter)
        return ERR_PTR(-ENOMEM);

    return counter;
}

static int zxdh_tc_add_nic_flow(struct zxdh_en_priv *priv,
                                struct zxdh_tc_flow_parse_attr *parse_attr,
                                struct zxdh_tc_flow *flow,
                                struct netlink_ext_ack *extack)
{
    struct zxdh_flow_context *flow_context = &parse_attr->spec.flow_context;
    struct zxdh_nic_flow_attr *attr = flow->nic_attr;
    struct zxdh_fc *counter = NULL;
    int32_t err             = 0;

    flow_context->flags |= FLOW_CONTEXT_HAS_TAG;
    flow_context->flow_tag = attr->flow_tag;

    if (attr->action & ZXDH_FLOW_CONTEXT_ACTION_COUNT) {
        counter = zxdh_fc_create();
        if (IS_ERR(counter))
            return PTR_ERR(counter);
        attr->counter = counter;
    }

    if (attr->match_level != ZXDH_MATCH_NONE)
        parse_attr->spec.match_criteria_enable |= ZXDH_MATCH_OUTER_HEADERS;

    mutex_lock(&priv->edev.fs.tc.t_lock);
    err = zxdh_tc_flow_replace(priv, flow);
    mutex_unlock(&priv->edev.fs.tc.t_lock);

    return err;
}

static int zxdh_add_nic_flow(struct zxdh_en_priv *priv,
                             struct flow_cls_offload *f,
                             unsigned long flow_flags,
                             struct net_device *filter_dev,
                             struct zxdh_tc_flow **__flow)
{
    struct flow_rule *rule = flow_cls_offload_flow_rule(f);
    struct netlink_ext_ack *extack = f->common.extack;
    struct zxdh_tc_flow_parse_attr *parse_attr = NULL;
    struct zxdh_tc_flow *flow = NULL;
    int attr_size = 0;
    int err = 0;

    if (!tc_cls_can_offload_and_chain0(priv->edev.netdev, &f->common))
        return -EOPNOTSUPP;

    flow_flags |= BIT(ZXDH_TC_FLOW_FLAG_NIC) | BIT(ZXDH_TC_FLOW_FLAG_SIMPLE);
    attr_size  = sizeof(struct zxdh_nic_flow_attr);
    flow = kzalloc(sizeof(*flow) + attr_size, GFP_KERNEL);
    if (!flow) {
        return -ENOMEM;
    }

    parse_attr = kvzalloc(sizeof(*parse_attr), GFP_KERNEL);
    if (!parse_attr) {
        kfree(flow);
        return -ENOMEM;
    }

    zxdh_init_flow(priv, attr_size, f->cookie, flow_flags, parse_attr, flow);
    parse_attr->filter_dev = filter_dev;

    err = parse_cls_flower(priv, flow, &parse_attr->spec, f, filter_dev);
    if (err)
        goto err_free;

    err = parse_tc_nic_actions(priv, &rule->action, parse_attr, flow, extack);
    if (err)
        goto err_free;

    err = zxdh_tc_add_nic_flow(priv, parse_attr, flow, extack);
    if (err)
        goto err_free;

    flow_flag_set(flow, OFFLOADED);
    *__flow = flow;

    return 0;

err_free:
    zxdh_tc_del_flow(priv, flow);
    kfree(flow);
    return err;

}

static int zxdh_tc_add_flow(struct zxdh_en_priv *priv,
                            struct flow_cls_offload *f,
                            unsigned long flags,
                            struct net_device *filter_dev,
                            struct zxdh_tc_flow **flow)
{
    unsigned long flow_flags;

    get_flags(flags, &flow_flags);

    if (!tc_can_offload_extack(priv->edev.netdev, f->common.extack))
        return -EOPNOTSUPP;

    return zxdh_add_nic_flow(priv, f, flow_flags, filter_dev, flow);
}

int zxdh_configure_flower(struct net_device *dev, struct zxdh_en_priv *priv,
                         struct flow_cls_offload *f, unsigned long flags)
{
    struct rhashtable *tc_ht = get_tc_ht(priv, flags);
    struct zxdh_tc_flow *flow =NULL;
    int err = 0;

    if (IS_ERR_OR_NULL(tc_ht))
        return -EOPNOTSUPP;

    //LOG_INFO("zxdh_configure_flower tc_ht=%p head_offset=%d key_offset=%d key_len=%d cookie=0x%lx classid=0x%x prio=0x%x\n",
        //tc_ht,tc_ht_params.head_offset,tc_ht_params.key_offset,tc_ht_params.key_len,f->cookie,f->classid,f->common.prio);
    flow = rhashtable_lookup_fast(tc_ht, &f->cookie, tc_ht_params);
    if (flow) {
        /* Same flow rule offloaded to non-uplink representor sharing
         * tc block, just return 0.
         */
        LOG_INFO_DEV(priv->edev.parent, "flow cookie %lx already exists, ignoring\n",f->cookie);
        err = -EEXIST;
        goto out;
    }

    err = zxdh_tc_add_flow(priv, f, flags, dev, &flow);
    if (err)
        goto out;

    err = rhashtable_lookup_insert_fast(tc_ht, &flow->node, tc_ht_params);
    if (err)
        goto err_free;

    return 0;

err_free:
    zxdh_flow_put(priv, flow);
out:
    return err;
}

static bool same_flow_direction(struct zxdh_tc_flow *flow, int flags)
{
    bool dir_ingress = !!(flags & ZXDH_TC_FLAG(INGRESS));
    bool dir_egress = !!(flags & ZXDH_TC_FLAG(EGRESS));

    return flow_flag_test(flow, INGRESS) == dir_ingress &&
            flow_flag_test(flow, EGRESS) == dir_egress;
}

static void zxdh_flow_defered_put(struct rcu_head *head)
{
    struct zxdh_tc_flow *flow = container_of(head, struct zxdh_tc_flow, rcu);

    zxdh_flow_put(flow->priv, flow);
}

void zxdh_fc_query_cached(struct zxdh_fc *counter,
                          uint64_t *bytes, uint64_t *packets, uint64_t *lastuse)
{
    *bytes = counter->cache.bytes - counter->lastbytes;
    *packets = counter->cache.packets - counter->lastpackets;
    *lastuse = counter->cache.lastuse;

    counter->lastbytes = counter->cache.bytes;
    counter->lastpackets = counter->cache.packets;
}

int zxdh_delete_flower(struct net_device *dev, struct zxdh_en_priv *priv,
                       struct flow_cls_offload *f, unsigned long flags)
{
    struct rhashtable *tc_ht = get_tc_ht(priv, flags);
    struct zxdh_tc_flow *flow;
    int err = 0;

    if (IS_ERR_OR_NULL(tc_ht))
        return -EOPNOTSUPP;

    //LOG_INFO("zxdh_delete_flower tc_ht=%p head_offset=%d key_offset=%d key_len=%d cookie=0x%lx classid=0x%x prio=0x%x\n",
             //tc_ht,tc_ht_params.head_offset,tc_ht_params.key_offset,tc_ht_params.key_len,f->cookie,f->classid,f->common.prio);

    rcu_read_lock();

    flow = rhashtable_lookup_fast(tc_ht, &f->cookie, tc_ht_params);
    if (!flow || !same_flow_direction(flow, flags)) {
        err = -EINVAL;
        goto errout;
    }

    /* Only delete the flow if it doesn't have ZXDH_TC_FLOW_DELETED flag
     * set.
     */
    if (flow_flag_test_and_set(flow, DELETED)) {
        err = -EINVAL;
        goto errout;
    }

    rhashtable_remove_fast(tc_ht, &flow->node, tc_ht_params);
    rcu_read_unlock();

    /* Protect __miniflow_merge() */
    if (!zxdh_is_simple_flow(flow)) {
        call_rcu(&flow->rcu, zxdh_flow_defered_put);
        return 0;
    }

    zxdh_flow_put(priv, flow);
    return 0;

errout:
    rcu_read_unlock();
    return err;
}

int zxdh_stats_flower(struct net_device *dev, struct zxdh_en_priv *priv,
    struct flow_cls_offload *f, unsigned long flags)
{
    struct rhashtable *tc_ht = get_tc_ht(priv, flags);
    struct zxdh_tc_flow *flow = NULL;
    struct zxdh_fc *counter = NULL;
    uint64_t lastuse = 0;
    uint64_t packets = 0;
    uint64_t bytes = 0;
    uint32_t counter_id = 0;
    int err = 0;

    if (IS_ERR_OR_NULL(tc_ht))
        return -EOPNOTSUPP;

    if(f==NULL) {
        return -EINVAL;
    }
    rcu_read_lock();
    flow = zxdh_flow_get((struct zxdh_tc_flow *)rhashtable_lookup(tc_ht, &f->cookie,
                         tc_ht_params));
    rcu_read_unlock();
    if (IS_ERR(flow))
        return PTR_ERR(flow);

    if (!same_flow_direction(flow, flags)) {
        err = -EINVAL;
        goto errout;
    }

    counter = flow->nic_attr->counter;
    if(!counter) {
        err = -EINVAL;
        goto errout;
    }
    counter_id = counter->id;

    err = zxdh_tc_flow_stat(priv, counter_id, &packets, &bytes);
    if (err)
        goto errout;

    counter->cache.bytes = bytes;
    counter->cache.packets = packets;
    counter->cache.lastuse = jiffies;

    zxdh_fc_query_cached(counter, &bytes, &packets, &lastuse);
#ifdef FLOW_UPDATE_STAT_SIX_PARAMS
    flow_stats_update(&f->stats, bytes, packets, 0, lastuse, FLOW_ACTION_HW_STATS_IMMEDIATE);
#elif defined (FLOW_UPDATE_STAT_FIVE_PARAMS)
    flow_stats_update(&f->stats, bytes, packets, lastuse, FLOW_ACTION_HW_STATS_IMMEDIATE);
#else
    flow_stats_update(&f->stats, bytes, packets, lastuse);
#endif
errout:
    zxdh_flow_put(priv, flow);
    return err;
}

static int zxdh_setup_tc_cls_flower(struct zxdh_en_priv *priv,
                                    struct flow_cls_offload *cls_flower,
                                    unsigned long flags)
{
    LOG_DEBUG_DEV(priv->edev.parent, "enter zxdh_setup_tc_cls_flower, cls_flower->command = %u\n",cls_flower->command);
    switch (cls_flower->command)
    {
        case FLOW_CLS_REPLACE:
            return zxdh_configure_flower(priv->edev.netdev, priv, cls_flower, flags);          
        case FLOW_CLS_DESTROY:
            return zxdh_delete_flower(priv->edev.netdev, priv, cls_flower, flags);
        case FLOW_CLS_STATS:
            return zxdh_stats_flower(priv->edev.netdev, priv, cls_flower, flags);
        default:
            return -EOPNOTSUPP;
    }
}

int zxdh_setup_tc_block_cb(enum tc_setup_type type, void *type_data,
                           void *cb_priv)
{
    unsigned long flags = ZXDH_TC_FLAG(INGRESS);
    struct zxdh_en_priv *en_priv = cb_priv;
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (!en_dev->netdev || !netif_device_present(en_dev->netdev))
        return -EOPNOTSUPP;

    flags |= ZXDH_TC_FLAG(NIC_OFFLOAD);
    LOG_DEBUG_DEV(en_dev->parent, "zxdh_setup_tc_block_cb type=%u flags=0x%lx\n",type,flags);
    switch (type)
    {
        case TC_SETUP_CLSFLOWER:
            return zxdh_setup_tc_cls_flower(en_priv, type_data, flags);
        default:
            return -EOPNOTSUPP;
    }

    return 0;
}
#endif
int32_t zxdh_tc_flow_recover(struct zxdh_en_priv *priv)
{
    int32_t err = 0;
    struct rhashtable_iter iter;
    struct zxdh_tc_flow *flow = NULL;
    struct rhashtable *tc_ht = get_tc_ht(priv,0);

    if (IS_ERR_OR_NULL(tc_ht))
        return 0;

    rcu_read_lock();
    rhashtable_walk_enter(tc_ht, &iter);
    rhashtable_walk_start(&iter);
    while (true)
    {
        flow = (struct zxdh_tc_flow *)rhashtable_walk_next(&iter);
        if (flow == NULL) {
            break;
        }

        if (IS_ERR(flow)) {
            if (PTR_ERR(flow) == -EAGAIN) {
                /* 如果是 -EAGAIN 错误，继续遍历 */
                continue;
            } else {
                /* 其他错误，停止遍历 */
                LOG_DEBUG_DEV(priv->edev.parent, "Error during rhashtable walk: %ld\n", PTR_ERR(flow));
                break;
            }
        }

        err = zxdh_tc_flow_replace(priv,flow);
        if (err != 0) {
            LOG_ERR_DEV(priv->edev.parent, "failed to recover fd cfg in np!\n");
            rcu_read_unlock();

            return -EPERM;
        }
    }

    rhashtable_walk_stop(&iter);
    rhashtable_walk_exit(&iter);
    rcu_read_unlock();

    return 0;
}

int zxdh_tc_nic_init(struct zxdh_en_priv *priv)
{
    struct zxdh_tc_table *tc = &priv->edev.fs.tc;
    int err = 0;

    mutex_init(&tc->t_lock);
    mutex_init(&tc->mod_hdr.lock);
    hash_init(tc->mod_hdr.hlist);
    mutex_init(&tc->hairpin_tbl_lock);
    hash_init(tc->hairpin_tbl);

    err = rhashtable_init(&tc->ht, &tc_ht_params);
    if (err) {
        mutex_destroy(&tc->mod_hdr.lock);
        mutex_destroy(&tc->hairpin_tbl_lock);
        mutex_destroy(&tc->t_lock);
        return err;
    }

    tc->netdevice_nb.notifier_call = NULL;

    return err;
}

void zxdh_tc_nic_cleanup(struct zxdh_en_priv *priv)
{
    struct zxdh_tc_table *tc = &priv->edev.fs.tc;

    if (tc->netdevice_nb.notifier_call)
    {
#ifdef CGS_V5_693
        unregister_netdevice_notifier_rh(&tc->netdevice_nb);
#else
        unregister_netdevice_notifier(&tc->netdevice_nb);
#endif
    }

    mutex_destroy(&tc->mod_hdr.lock);
    mutex_destroy(&tc->hairpin_tbl_lock);

    rhashtable_destroy(&tc->ht);

    mutex_destroy(&tc->t_lock);
}




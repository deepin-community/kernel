#ifndef __ZXDH_EN_TC_H__
#define __ZXDH_EN_TC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>
#include <linux/types.h>
#include <linux/stddef.h>
#ifdef HAVE_TC_FLOW_OFFLOAD
#ifndef CGS_V5_693
#include <net/flow_offload.h>
#endif
#endif
#include "fs_core.h"
#include "en_aux.h"
#include "../en_np/flow/api/include/dpp_tbl_fd_cfg.h"

enum {
    ZXDH_TC_FLAG_INGRESS_BIT,
    ZXDH_TC_FLAG_EGRESS_BIT,
    ZXDH_TC_FLAG_NIC_OFFLOAD_BIT,
    ZXDH_TC_FLAG_ESW_OFFLOAD_BIT,
    ZXDH_TC_FLAG_FT_OFFLOAD_BIT,
    ZXDH_TC_FLAG_LAST_EXPORTED_BIT = ZXDH_TC_FLAG_FT_OFFLOAD_BIT,
};

#define ZXDH_TC_FLOW_BASE (ZXDH_TC_FLAG_LAST_EXPORTED_BIT + 1)
#define ZXDH_TC_FLAG(flag) BIT(ZXDH_TC_FLAG_##flag##_BIT)

#define ZXDH_TC_FLOW_ID_MASK (0x0000FFFF)

#define ZXDH_TC_MAX_SPLITS 1

#define ZXDH_TC_TABLE_NUM_GROUPS           (4)
#define ZXDH_TC_TABLE_MAX_GROUP_SIZE       BIT(16)
#define ZXDH_INVALID_INDEX                 (0xFFFFFFFF)

enum {
    ZXDH_TC_FLOW_FLAG_INGRESS     = ZXDH_TC_FLAG_INGRESS_BIT,
    ZXDH_TC_FLOW_FLAG_EGRESS      = ZXDH_TC_FLAG_EGRESS_BIT,
    ZXDH_TC_FLOW_FLAG_ESWITCH     = ZXDH_TC_FLAG_ESW_OFFLOAD_BIT,
    ZXDH_TC_FLOW_FLAG_NIC         = ZXDH_TC_FLAG_NIC_OFFLOAD_BIT,
    ZXDH_TC_FLOW_FLAG_OFFLOADED   = ZXDH_TC_FLOW_BASE,
    ZXDH_TC_FLOW_FLAG_HAIRPIN     = ZXDH_TC_FLOW_BASE + 1,
    ZXDH_TC_FLOW_FLAG_HAIRPIN_RSS = ZXDH_TC_FLOW_BASE + 2,
    ZXDH_TC_FLOW_FLAG_SLOW        = ZXDH_TC_FLOW_BASE + 3,
    ZXDH_TC_FLOW_FLAG_DUP         = ZXDH_TC_FLOW_BASE + 4,
    ZXDH_TC_FLOW_FLAG_NOT_READY   = ZXDH_TC_FLOW_BASE + 5,
    ZXDH_TC_FLOW_FLAG_DELETED     = ZXDH_TC_FLOW_BASE + 6,
    ZXDH_TC_FLOW_FLAG_SIMPLE      = ZXDH_TC_FLOW_BASE + 7,
    ZXDH_TC_FLOW_FLAG_CT          = ZXDH_TC_FLOW_BASE + 8,
    ZXDH_TC_FLOW_FLAG_CT_ORIG     = ZXDH_TC_FLOW_BASE + 9,
};

enum {
    ZXDH_ATTR_FLAG_VLAN_HANDLED  = BIT(0),
    ZXDH_ATTR_FLAG_SLOW_PATH     = BIT(1),
    ZXDH_ATTR_FLAG_NO_IN_PORT    = BIT(2),
    ZXDH_ATTR_FLAG_SRC_REWRITE   = BIT(3),
    ZXDH_ATTR_FLAG_SAMPLE        = BIT(4),
    ZXDH_ATTR_FLAG_ACCEPT        = BIT(5),
    ZXDH_ATTR_FLAG_CT            = BIT(6),
    ZXDH_ATTR_FLAG_TERMINATING   = BIT(7),
    ZXDH_ATTR_FLAG_MTU           = BIT(8),
};

enum {
    ZXDH_TC_PRIO = 0,
    ZXDH_NIC_PRIO
};

struct pedit_headers {
    struct ethhdr  eth;
    struct vlan_hdr vlan;
    struct iphdr   ip4;
    struct ipv6hdr ip6;
    struct tcphdr  tcp;
    struct udphdr  udp;
};

struct pedit_headers_action {
    struct pedit_headers  vals;
    struct pedit_headers  masks;
    uint32_t              pedits;
};

struct zxdh_ifc_set_action_in_bits {
    uint8_t         action_type;
    uint16_t        field;
    uint8_t         reserved_at_10;
    uint8_t         offset;
    uint8_t         reserved_at_18;
    uint8_t         length;

    uint32_t         data;
};

struct zxdh_ifc_add_action_in_bits {
    uint8_t         action_type;
    uint16_t        field;
    uint16_t        reserved_at_10;

    uint32_t        data;
};

struct zxdh_ifc_copy_action_in_bits {
    uint8_t         action_type;
    uint16_t        src_field;
    uint8_t         reserved_at_10;
    uint8_t         src_offset;
    uint8_t         reserved_at_18;
    uint8_t         length;

    uint8_t         reserved_at_20;
    uint16_t        dst_field;
    uint8_t         reserved_at_30;
    uint8_t         dst_offset;
    uint8_t         reserved_at_38;
};

union zxdh_ifc_set_add_copy_action_in_auto_bits {
    struct zxdh_ifc_set_action_in_bits  set_action_in;
    struct zxdh_ifc_add_action_in_bits  add_action_in;
    struct zxdh_ifc_copy_action_in_bits copy_action_in;
    uint64_t         reserved_at_0;
};

enum {
    ZXDH_ACTION_TYPE_SET   = 0x1,
    ZXDH_ACTION_TYPE_ADD   = 0x2,
    ZXDH_ACTION_TYPE_COPY  = 0x3,
};

struct ip_ttl_word {
    __u8    ttl;
    __u8    protocol;
    __sum16 check;
};

struct ipv6_hoplimit_word {
    __be16  payload_len;
    __u8    nexthdr;
    __u8    hop_limit;
};

struct zxdh_match_lyr_2_4_param {
    uint8_t dmac[ETH_ALEN];
    uint8_t smac[ETH_ALEN];
    uint16_t ethertype;
    uint16_t vlan_id;
    uint16_t vlan_prio;

    uint16_t ip_protocol;
    uint8_t  ip_dscp;
    uint8_t  ip_ecn;
    uint8_t  cvlan_tag;
    uint8_t  svlan_tag;
    uint8_t  frag;
    uint8_t  ip_version;
    uint16_t tcp_flags;

    uint16_t tcp_dport;
    uint16_t tcp_sport;

    uint8_t   ipv4_ihl;
    uint8_t   ttl_hoplimit;

    uint16_t udp_dport;
    uint16_t udp_sport;

    uint8_t dst_ip[16];
    uint8_t src_ip[16];
};

struct zxdh_match_misc_param {
    uint8_t outer_second_cvlan_tag;
    uint8_t inner_second_cvlan_tag;
    uint8_t outer_second_svlan_tag;
    uint8_t inner_second_svlan_tag;

    uint16_t outer_second_vid;
    uint8_t outer_second_prio;
    uint8_t outer_second_cfi;

    uint16_t inner_second_vid;
    uint8_t inner_second_prio;
    uint8_t inner_second_cfi;

    uint32_t vxlan_vni;

    uint8_t icmp_type;
    uint8_t icmp_code;
    uint8_t icmpv6_type;
    uint8_t icmpv6_code;
};

struct zxdh_ifc_lyr2_4_param {
    struct zxdh_match_lyr_2_4_param key;
    struct zxdh_match_lyr_2_4_param mask;
};

struct zxdh_ifc_misc_param {
    struct zxdh_match_misc_param key;
    struct zxdh_match_misc_param mask;
};

struct zxdh_flow_spec {
    uint32_t match_criteria_enable;
    struct zxdh_ifc_lyr2_4_param outer_header;
    struct zxdh_ifc_lyr2_4_param inner_header;
    struct zxdh_ifc_misc_param misc_parameter;
    uint32_t  handle;
    struct zxdh_flow_context flow_context;
};

struct zxdh_tc_flow_parse_attr {
    const struct ip_tunnel_info *tun_info[ZXDH_MAX_FLOW_FWD_VPORTS];
    struct ip_tunnel_info tun_info2[ZXDH_MAX_FLOW_FWD_VPORTS];
    struct net_device *filter_dev;
    struct zxdh_flow_spec spec;
    int num_mod_hdr_actions;
    int max_mod_hdr_actions;
    void *mod_hdr_actions;
    int mirred_ifindex[ZXDH_MAX_FLOW_FWD_VPORTS];
};

struct zxdh_nic_flow_attr {
    uint32_t rule_index;
    uint32_t action;
    uint32_t flow_tag;
    //struct zxdh_modify_hdr *modify_hdr;
    uint32_t hairpin_tirn;
    uint32_t split_count;
    uint32_t out_count;
    uint8_t match_level;
    uint8_t total_vlan;
    __be16  vlan_proto[ZXDH_FS_VLAN_DEPTH];
    uint16_t vlan_vid[ZXDH_FS_VLAN_DEPTH];
    uint8_t vlan_prio[ZXDH_FS_VLAN_DEPTH];
    //struct zxdh_flow_table    *hairpin_ft;
    struct zxdh_fc   *counter;
    struct zxdh_tc_flow_parse_attr *parse_attr;
};

struct encap_flow_item {
    //struct zxdh_encap_entry *e; /* attached encap instance */
    struct list_head list;
    int index;
};

struct zxdh_tc_flow {
    struct rhash_head    node;
    struct zxdh_en_priv *priv;
    uint64_t            cookie;
    unsigned long       flags;
    struct zxdh_flow_handle *rule[ZXDH_TC_MAX_SPLITS + 1];
    /* Flow can be associated with multiple encap IDs.
     * The number of encaps is bounded by the number of supported
     * destinations.
     */
    struct encap_flow_item encaps[ZXDH_MAX_FLOW_FWD_VPORTS];
    struct zxdh_tc_flow    *peer_flow;
    //struct zxdh_mod_hdr_entry *mh; /* attached mod header instance */
    struct list_head    mod_hdr; /* flows sharing the same mod hdr ID */
    //struct zxdh_hairpin_entry *hpe; /* attached hairpin instance */
    struct list_head    hairpin; /* flows sharing the same hairpin */
    struct list_head    peer;    /* flows with peer flow */
    struct list_head    unready; /* flows not ready to be offloaded (e.g due to missing route) */
    int                 tmp_efi_index;
    struct list_head    tmp_list; /* temporary flow list used by neigh update */
    struct net_device   *added_dev;
    refcount_t          refcnt;
    struct rcu_head     rcu_head;
    struct completion   init_done;

    uint64_t            version;
    //struct zxdh_miniflow   *miniflow;
    struct zxdh_fc          *dummy_counter;
    struct list_head        miniflow_list;
    struct rcu_head         rcu;
    struct list_head        nft_node;
    spinlock_t              *dep_lock;
    struct zxdh_nic_flow_attr nic_attr[0];

    //union {
        //struct zxdh_esw_flow_attr esw_attr[0];
        //struct zxdh_nic_flow_attr nic_attr[0];
    //};
};

static inline void __flow_flag_set(struct zxdh_tc_flow *flow, unsigned long flag)
{
    /* Complete all memory stores before setting bit. */
    smp_mb__before_atomic();
    set_bit(flag, (unsigned long *)&flow->flags);
}

#define flow_flag_set(flow, flag) __flow_flag_set(flow, ZXDH_TC_FLOW_FLAG_##flag)

static inline bool __flow_flag_test_and_set(struct zxdh_tc_flow *flow,unsigned long flag)
{
    /* test_and_set_bit() provides all necessary barriers */
    return test_and_set_bit(flag, (unsigned long *)&flow->flags);
}

#define flow_flag_test_and_set(flow, flag) __flow_flag_test_and_set(flow,ZXDH_TC_FLOW_FLAG_##flag)

static inline void __flow_flag_clear(struct zxdh_tc_flow *flow, unsigned long flag)
{
    /* Complete all memory stores before clearing bit. */
    smp_mb__before_atomic();
    clear_bit(flag, (unsigned long *)&flow->flags);
}

#define flow_flag_clear(flow, flag) __flow_flag_clear(flow, ZXDH_TC_FLOW_FLAG_##flag)

static inline bool __flow_flag_test(struct zxdh_tc_flow *flow, unsigned long flag)
{
    bool ret = test_bit(flag, (unsigned long *)&flow->flags);

    /* Read fields of flow structure only after checking flags. */
    smp_mb__after_atomic();
    return ret;
}

#define flow_flag_test(flow, flag) __flow_flag_test(flow, ZXDH_TC_FLOW_FLAG_##flag)
#ifdef HAVE_TC_FLOW_OFFLOAD
int parse_tc_pedit_action(struct zxdh_en_priv *priv,
                          const struct flow_action_entry *act, int namespace,
                          struct zxdh_tc_flow_parse_attr *parse_attr,
                          struct pedit_headers_action *hdrs,
                          struct netlink_ext_ack *extack);
int zxdh_setup_tc_block_cb(enum tc_setup_type type, void *type_data,void *cb_priv);
#endif
int zxdh_tc_nic_init(struct zxdh_en_priv *priv);
void zxdh_tc_nic_cleanup(struct zxdh_en_priv *priv);
int32_t zxdh_tc_flow_recover(struct zxdh_en_priv *priv);
int32_t zxdh_tc_num_filters(struct zxdh_en_priv *priv, unsigned long flags);
int32_t zxdh_tc_flow_replace(struct zxdh_en_priv *priv, struct zxdh_tc_flow *flow);
int32_t zxdh_tc_flow_remove(struct zxdh_en_priv *priv, uint32_t location);
int32_t zxdh_tc_flow_stat(struct zxdh_en_priv *priv, uint32_t count_id, uint64_t *pkts, uint64_t *bytes);

int32_t zxdh_tc_flow_table_add(struct zxdh_nic_flow_attr *flow_attr, ZXDH_FD_CFG_T *p_fd_cfg, DPP_PF_INFO_T *pf_info);
int32_t zxdh_tc_flow_action_add(struct zxdh_en_device *en_dev, struct zxdh_nic_flow_attr *flow_attr, ZXDH_FD_CFG_T *p_fd_cfg);

#ifdef __cplusplus
}
#endif

#endif
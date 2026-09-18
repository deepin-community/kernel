#ifndef __ZXDH_FS_CORE_H__
#define __ZXDH_FS_CORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>
#include <linux/types.h>
#ifdef CGS_V5_693
#include <linux/rhashtable.h>
#endif

#define ZXDH_MAX_FLOW_FWD_VPORTS       (32)
#define ZXDH_FS_DEFAULT_FLOW_TAG       (0)
#define MINIFLOW_MAX_FLOWS             (12)

enum fs_node_type {
    FS_TYPE_NAMESPACE,
    FS_TYPE_PRIO,
    FS_TYPE_PRIO_CHAINS,
    FS_TYPE_FLOW_TABLE,
    FS_TYPE_FLOW_GROUP,
    FS_TYPE_FLOW_ENTRY,
    FS_TYPE_FLOW_DEST
};

enum fs_flow_table_type {
    FS_FT_NIC_RX          = 0x0,
    FS_FT_NIC_TX          = 0x1,
    FS_FT_ESW_EGRESS_ACL  = 0x2,
    FS_FT_ESW_INGRESS_ACL = 0x3,
    FS_FT_FDB             = 0X4,
    FS_FT_SNIFFER_RX      = 0X5,
    FS_FT_SNIFFER_TX      = 0X6,
    FS_FT_RDMA_RX         = 0X7,
    FS_FT_RDMA_TX         = 0X8,
    FS_FT_PORT_SEL        = 0X9,
    FS_FT_MAX_TYPE        = FS_FT_PORT_SEL,
};

enum fs_flow_table_op_mod {
    FS_FT_OP_MOD_NORMAL,
    FS_FT_OP_MOD_LAG_DEMUX,
};

enum zxdh_flow_table_miss_action {
    ZXDH_FLOW_TABLE_MISS_ACTION_DEF,
    ZXDH_FLOW_TABLE_MISS_ACTION_FWD,
    ZXDH_FLOW_TABLE_MISS_ACTION_SWITCH_DOMAIN,
};

enum zxdh_flow_destination_type {
    ZXDH_FLOW_DESTINATION_TYPE_NONE,
    ZXDH_FLOW_DESTINATION_TYPE_VPORT,
    ZXDH_FLOW_DESTINATION_TYPE_FLOW_TABLE,
    ZXDH_FLOW_DESTINATION_TYPE_TIR,
    ZXDH_FLOW_DESTINATION_TYPE_FLOW_SAMPLER,
    ZXDH_FLOW_DESTINATION_TYPE_UPLINK,
    ZXDH_FLOW_DESTINATION_TYPE_PORT,
    ZXDH_FLOW_DESTINATION_TYPE_COUNTER,
    ZXDH_FLOW_DESTINATION_TYPE_FLOW_TABLE_NUM,
    ZXDH_FLOW_DESTINATION_TYPE_RANGE,
    ZXDH_FLOW_DESTINATION_TYPE_TABLE_TYPE,
};

enum zxdh_flow_namespace_type {
    ZXDH_FLOW_NAMESPACE_BYPASS,
    ZXDH_FLOW_NAMESPACE_KERNEL_RX_MACSEC,
    ZXDH_FLOW_NAMESPACE_LAG,
    ZXDH_FLOW_NAMESPACE_OFFLOADS,
    ZXDH_FLOW_NAMESPACE_ETHTOOL,
    ZXDH_FLOW_NAMESPACE_KERNEL,
    ZXDH_FLOW_NAMESPACE_LEFTOVERS,
    ZXDH_FLOW_NAMESPACE_ANCHOR,
    ZXDH_FLOW_NAMESPACE_FDB_BYPASS,
    ZXDH_FLOW_NAMESPACE_FDB,
    ZXDH_FLOW_NAMESPACE_ESW_EGRESS,
    ZXDH_FLOW_NAMESPACE_ESW_INGRESS,
    ZXDH_FLOW_NAMESPACE_SNIFFER_RX,
    ZXDH_FLOW_NAMESPACE_SNIFFER_TX,
    ZXDH_FLOW_NAMESPACE_EGRESS,
    ZXDH_FLOW_NAMESPACE_EGRESS_IPSEC,
    ZXDH_FLOW_NAMESPACE_EGRESS_KERNEL = ZXDH_FLOW_NAMESPACE_EGRESS_IPSEC,
    ZXDH_FLOW_NAMESPACE_EGRESS_MACSEC,
    ZXDH_FLOW_NAMESPACE_RDMA_RX,
    ZXDH_FLOW_NAMESPACE_RDMA_RX_KERNEL,
    ZXDH_FLOW_NAMESPACE_RDMA_TX,
    ZXDH_FLOW_NAMESPACE_PORT_SEL,
    ZXDH_FLOW_NAMESPACE_RDMA_RX_COUNTERS,
    ZXDH_FLOW_NAMESPACE_RDMA_TX_COUNTERS,
    ZXDH_FLOW_NAMESPACE_RDMA_RX_IPSEC,
    ZXDH_FLOW_NAMESPACE_RDMA_TX_IPSEC,
};

enum zxdh_flow_dest_range_field {
    ZXDH_FLOW_DEST_RANGE_FIELD_PKT_LEN = 0,
};

enum fs_packet_reformat_owner {
    FS_PACKET_REFORMAT_NONE,
    FS_PACKET_REFORMAT_SW,
    FS_PACKET_REFORMAT_FW,
};

enum {
    FLOW_CONTEXT_HAS_TAG = BIT(0),
};

enum fs_fte_status {
    FS_FTE_STATUS_EXISTING = 1UL << 0,
};

struct fs_node {
    struct list_head    list;
    struct list_head    children;
    enum fs_node_type   type;
    struct fs_node      *parent;
    struct fs_node      *root;
    /* lock the node for writing and traversing */
    struct rw_semaphore lock;
    refcount_t          refcount;
    bool                active;
    void                (*del_hw_func)(struct fs_node *);
    void                (*del_sw_func)(struct fs_node *);
    atomic_t            version;
};

struct zxdh_flow_context {
    uint32_t flags;
    uint32_t flow_tag;
    uint32_t flow_source;
};

struct fs_fte {
    struct fs_node              node;
    //struct zxdh_fs_dr_rule    fs_dr_rule;
    //u32     val[ZXDH_ST_SZ_DW_MATCH_PARAM];
    uint32_t                    dests_size;
    uint32_t                    index;
    struct zxdh_flow_context    flow_context;
    //struct zxdh_flow_act      action;
    enum fs_fte_status          status;
    struct zxdh_fc              *counter;
    struct rhash_head           hash;
    //struct fs_debugfs_fte     debugfs;
    int32_t                     modify_mask;
    uint32_t                    handle;
};

enum zxdh_inline_modes {
    ZXDH_INLINE_MODE_NONE,
    ZXDH_INLINE_MODE_L2,
    ZXDH_INLINE_MODE_IP,
    ZXDH_INLINE_MODE_TCP_UDP,
};

enum zxdh_flow_match_level {
    ZXDH_MATCH_NONE = ZXDH_INLINE_MODE_NONE,
    ZXDH_MATCH_L2   = ZXDH_INLINE_MODE_L2,
    ZXDH_MATCH_L3   = ZXDH_INLINE_MODE_IP,
    ZXDH_MATCH_L4   = ZXDH_INLINE_MODE_TCP_UDP,
};

enum {
    ZXDH_MATCH_OUTER_HEADERS     = 1 << 0,
    ZXDH_MATCH_MISC_PARAMETERS   = 1 << 1,
    ZXDH_MATCH_INNER_HEADERS     = 1 << 2,
    ZXDH_MATCH_MISC_PARAMETERS_2 = 1 << 3,
    ZXDH_MATCH_MISC_PARAMETERS_3 = 1 << 4,
    ZXDH_MATCH_MISC_PARAMETERS_4 = 1 << 5,
    ZXDH_MATCH_MISC_PARAMETERS_5 = 1 << 6,
};

enum {
    ZXDH_FLOW_CONTEXT_ACTION_ALLOW     = 0x1,
    ZXDH_FLOW_CONTEXT_ACTION_DROP      = 0x2,
    ZXDH_FLOW_CONTEXT_ACTION_FWD_DEST  = 0x4,
    ZXDH_FLOW_CONTEXT_ACTION_COUNT     = 0x8,
    ZXDH_FLOW_CONTEXT_ACTION_PACKET_REFORMAT = 0x10,
    ZXDH_FLOW_CONTEXT_ACTION_DECAP     = 0x20,
    ZXDH_FLOW_CONTEXT_ACTION_MOD_HDR   = 0x40,
    ZXDH_FLOW_CONTEXT_ACTION_VLAN_POP  = 0x80,
    ZXDH_FLOW_CONTEXT_ACTION_VLAN_PUSH = 0x100,
    ZXDH_FLOW_CONTEXT_ACTION_VLAN_POP_2  = 0x400,
    ZXDH_FLOW_CONTEXT_ACTION_VLAN_PUSH_2 = 0x800,
    /* BIG proplem !!!! needs PRM change !!!! */
    ZXDH_FLOW_CONTEXT_ACTION_IPSEC_DECRYPT = 0x1000,
    ZXDH_FLOW_CONTEXT_ACTION_IPSEC_ENCRYPT = 0x2000,
    /**
     * XXX: following actions are not supported in hw
     * and being removed from flow before offloading it.
     */
    ZXDH_FLOW_CONTEXT_ACTION_CT        = 0x4000,
    ZXDH_FLOW_CONTEXT_ACTION_GOTO      = 0x8000,
};

enum {
    ZXDH_FLOW_DEST_VPORT_VHCA_ID      = BIT(0),
    ZXDH_FLOW_DEST_VPORT_REFORMAT_ID  = BIT(1),
};

/* Type of children is fs_prio */
struct zxdh_flow_namespace {
    /* parent == NULL => root ns */
    struct	fs_node			node;
    enum zxdh_flow_table_miss_action def_miss_action;
};

struct zxdh_flow_table_attr {
    int32_t prio;
    int32_t max_fte;
    uint32_t level;
    uint32_t flags;
};

struct zxdh_flow_table {
    struct fs_node      node;
    //struct zxdh_fs_dr_table   fs_dr_table;
    uint32_t            id;
    uint16_t            vport;
    uint32_t            max_fte;
    uint32_t            level;
    enum fs_flow_table_type     type;
    enum fs_flow_table_op_mod   op_mod;
    struct {
        bool            active;
        uint32_t        required_groups;
        uint32_t        group_size;
        uint32_t        num_groups;
        uint32_t        max_fte;
    } autogroup;
    /* Protect fwd_rules */
    struct mutex        lock;
    /* FWD rules that point on this flow table */
    struct list_head    fwd_rules;
    uint32_t            flags;
    struct rhltable     fgs_hash;
    enum zxdh_flow_table_miss_action def_miss_action;
    //struct zxdh_flow_namespace *ns;
};

struct zxdh_pkt_reformat {
    enum zxdh_flow_namespace_type ns_type;
    int reformat_type; /* from zxdh_ifc */
    enum fs_packet_reformat_owner owner;
    union {
       //struct zxdh_fs_dr_action action;
       uint32_t id;
    };
};

struct zxdh_flow_destination {
    enum zxdh_flow_destination_type type;
    union {
        uint32_t                tir_num;
        uint32_t                ft_num;
        struct zxdh_flow_table  *ft;
        uint32_t                counter_id;
        struct {
            uint16_t      num;
            uint16_t      vhca_id;
            struct zxdh_pkt_reformat *pkt_reformat;
            uint8_t       flags;
        } vport;
    };
};

struct zxdh_flow_rule {
    struct fs_node                  node;
    struct zxdh_flow_table          *ft;
    struct zxdh_flow_destination    dest_attr;
    /* next_ft should be accessed under chain_lock and only of
     * destination type is FWD_NEXT_fT.
     */
    struct list_head                next_ft;
    uint32_t                        sw_action;
};

struct zxdh_flow_handle {
    int num_rules;
    struct zxdh_flow_rule *rule[];
};

struct zxdh_fc_cache {
    uint64_t packets;
    uint64_t bytes;
    uint64_t lastuse;
};

struct zxdh_fc {
    struct list_head list;
    struct llist_node addlist;
    struct llist_node dellist;

    /* last{packets,bytes} members are used when calculating the delta since
     * last reading
     */
    uint64_t lastpackets;
    uint64_t lastbytes;

    struct zxdh_fc_bulk *bulk;
    uint32_t id;
    bool aging;
    bool dummy;

    atomic_t nr_dummies;
    struct zxdh_fc *dummies[MINIFLOW_MAX_FLOWS];

    struct zxdh_fc_cache cache ____cacheline_aligned_in_smp;
};

struct zxdh_fc_bulk {
    struct list_head pool_list;
    uint32_t base_id;
    int32_t bulk_len;
    unsigned long *bitmask;
    struct zxdh_fc fcs[];
};

/* bits[1-8] are reserved for number of actions, so we use bits[9-32] for flags values */
enum zxdh_modify_header_flags {
    ZXDH_MODIFY_HEADER_FLAG_FW_CREATED = BIT(9),
};

struct zxdh_modify_hdr {
    enum zxdh_flow_namespace_type ns_type;
    bool sw_owned;
    union {
        //struct zxdh_fs_dr_action action;
        uint32_t id;
    };
    enum zxdh_modify_header_flags flags;
};

struct mod_hdr_key {
    int num_actions;
    void *actions;
};

struct zxdh_mod_hdr_handle {
    /* a node of a hash table which keeps all the mod_hdr entries */
    struct hlist_node mod_hdr_hlist;

    struct mod_hdr_key key;

    struct zxdh_modify_hdr *modify_hdr;

    refcount_t refcnt;
    struct completion res_ready;
    int compl_result;
};

struct zxdh_ct_attr {
    uint16_t zone;
    uint16_t ct_action;
    struct nf_flowtable *nf_ft;
    uint32_t ct_labels_id;
    uint32_t act_miss_mapping;
    uint64_t act_miss_cookie;
    bool offloaded;
    //struct zxdh_ct_ft *ft;
};

struct zxdh_mpls_info {
    uint32_t             label;
    uint8_t              tc;
    uint8_t              bos;
    uint8_t              ttl;
};

struct zxdh_tc_mod_hdr_acts {
    int32_t num_actions;
    int32_t max_actions;
    bool is_static;
    void *actions;
};

struct zxdh_fs_vlan {
    uint16_t ethtype;
    uint16_t vid;
    uint8_t  prio;
};

#define ZXDH_FS_VLAN_DEPTH  (2)

enum {
    FLOW_ACT_NO_APPEND = BIT(0),
};

struct zxdh_flow_act {
    uint32_t action;
    struct zxdh_modify_hdr  *modify_hdr;
    struct zxdh_pkt_reformat *pkt_reformat;
    uint32_t ipsec_obj_id;
    uintptr_t esp_id;
    uint32_t flags;
    struct zxdh_fs_vlan vlan[ZXDH_FS_VLAN_DEPTH];
    struct ib_counters *counters;
};

#ifdef __cplusplus
}
#endif

#endif
#ifndef __ZXDH_MSG_COMMON_H__
#define __ZXDH_MSG_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "en_np/table/include/dpp_tbl_api.h"
#include "en_np/flow/api/include/dpp_tbl_fd_cfg.h"
#include "en_aux/queue.h"
#include "en_pf.h"

#define ZXDH_VLAN_TCI_GEN(vid, qos)    ((vid) | ((qos) << 13))
#define FW_VERSION_LEN 32
#define VENDOR_SIZE 16
#define ZXDH_REPS_BEYOND_MAC   0xfe /* 转发域mac超过上限 */
#define ZXDH_REPS_EXIST_MAC    0xfd /* 存在重复的单播mac */
#define MAX_QUE_CNT 60
#define ZXDH_REPS_MAX_SIZE_BEFORE57  1032 //zxdh_msg_op_code=57之前的最大sizeof(union zxdh_msg)
#define BAR_MSG_RETRY_CNT_MAX        10

#define ZXDH_BAR_DUALTOR_LABEL_ON    (0xaaaaaaaa)
#define ZXDH_DUALTOR_LABEL_OFFSET    (0x5000 + 1920)

#define DEFAULT_ADD_INDEX  0XFFFFFFFF /* 默认申请index */
typedef enum
{
    ZXDH_NULL = 0,

    ZXDH_VF_PORT_INIT = 1,
    ZXDH_VF_PORT_UNINIT = 2,
    ZXDH_MAC_ADD = 3,
    ZXDH_MAC_DEL = 4,
    ZXDH_MAC_GET =  5,

    ZXDH_RSS_EN_SET = 7,
    ZXDH_RXFH_SET = 8,
    ZXDH_RXFH_GET = 9,
    ZXDH_RXFH_DEL = 10,
    ZXDH_THASH_KEY_SET = 11,
    ZXDH_THASH_KEY_GET = 12,
    ZXDH_HASH_FUNC_SET = 13,
    ZXDH_HASH_FUNC_GET = 14,
    ZXDH_RX_FLOW_HASH_SET = 15,
    ZXDH_RX_FLOW_HASH_GET = 16,

    ZXDH_VLAN_FILTER_SET = 17,
    ZXDH_VLAN_FILTER_ADD = 18,
    ZXDH_VLAN_FILTER_DEL = 19,
    ZXDH_VLAN_OFFLOAD_SET = 21,

    ZXDH_PORT_ATTRS_GET = 22,
    ZXDH_SET_TPID = 23,
    ZXDH_VXLAN_OFFLOAD_ADD = 24,
    ZXDH_PORT_ATTRS_SET = 25,
    ZXDH_PROMISC_SET = 26,

    /*sriov msg type*/
    ZXDH_SRIOV_RESET = 27,

    ZXDH_SET_VF_LINK_STATE = 28,
    ZXDH_PF_SET_VF_VLAN = 29,
    ZXDH_SET_VF_RESET = 30,
    ZXDH_GET_NP_STATS = 31,

    ZXDH_VF_RATE_LIMIT_SET = 32,
    ZXDH_PLCR_UNINIT = 33,
    ZXDH_MAP_PLCR_FLOWID = 34,
    ZXDH_PLCR_FLOW_INIT = 35,
    ZXDH_PLCR_CAR_PROFILE_ID_ADD = 36,
    ZXDH_PLCR_CAR_PROFILE_ID_DELETE = 37,
    ZXDH_PLCR_CAR_PROFILE_CFG_SET = 38,
    ZXDH_PLCR_CAR_PROFILE_CFG_GET = 39,
    ZXDH_PLCR_CAR_QUEUE_CFG_SET = 40,
    ZXDH_PORT_METER_STAT_CLR = 41,
    ZXDH_PORT_METER_STAT_GET = 42,
    ZXDH_PF_GET_VF_QUEUE_INFO = 43,
    ZXDH_PLCR_GET_MODE        = 44,
    ZXDH_PLCR_SET_MODE        = 45,
    ZXDH_FLOW_HW_ADD          = 46,
    ZXDH_FLOW_HW_DEL          = 47,
    ZXDH_FLOW_HW_GET          = 48,
    ZXDH_FLOW_HW_FLUSH        = 49,


    ZXDH_VF_1588_CALL_NP = 50,
    ZXDH_VF_SLOT_ID_GET = 51,

    ZXDH_IPV6_MAC_ADD = 52,
    ZXDH_IPV6_MAC_DEL = 53,
    ZXDH_MAC_DUMP = 54,
    ZXDH_MC_CMPAT_VERINFO = 55,
    ZXDH_GET_K_CMPAT_VERINFO = 56,
    ZXDH_GET_SW_STATS = 57,
    ZXDH_LACP_MAC_ADD = 58,
    ZXDH_LACP_MAC_DEL = 59,
    ZXDH_VXLAN_OFFLOAD_DEL = 60,
    ZXDH_VF_PORT_RELOAD = 61,
    ZXDH_VF_1588_ENABLE = 62,
    ZXDH_VF_GET_UDP_STATS = 63,
    ZXDH_FD_ADD = 64,
    ZXDH_FD_GET = 65,
    ZXDH_FD_DEL = 66,
    ZXDH_FD_EN_SET = 67,
    ZXDH_PF_UPDATE_VF_RO_FLAG = 68,
    ZXDH_VF_RX_NUM_SET = 69,
    ZXDH_VF_RSSKEY_IPID_ENABLE = 70,
    ZXDH_VF_QUEUE_INFO_SHOW = 71,
    ZXDH_VF_PPU_STAT = 72,
    ZXDH_TC_FD_ADD = 73,
    ZXDH_MSG_TYPE_CNT_MAX, /* should be at last */
} __attribute__((packed)) zxdh_msg_op_code;

enum dh_flow_type {
    FLOW_TYPE_FLOW = 0,
    FLOW_TYPE_FD_TCAM,
    FLOW_TYPE_FD_SW,
};

enum {
    FD_ACTION_VXLAN_ENCAP = 0,
    FD_ACTION_VXLAN_DECAP = 1,
    FD_ACTION_RSS_BIT   = 2,
    FD_ACTION_COUNT_BIT = 3,
    FD_ACTION_DROP_BIT  = 4,
    FD_ACTION_MARK_BIT  = 5,
    FD_ACTION_QUEUE_BIT = 6,
};

enum vqm_msg_opcode
{
    OPCODE_GET = 0,
    OPCODE_SET = 1,
};

enum vqm_msg_cmd
{
    START_QID_QPAIR = 0x4,
    VQM_VF_FC_CMD = 0x9,
    MSIX_MODE_CMD = 0x10,
    OVS_VQM_CTRL_RESET_QIDS = 0x11,
    COALESCE_USECS_CMD = 0xf,
    VQM_MSIX_ADAPTIVE_CFG_CMD = 0x12,
    COALESCE_PARAMS_CMD = 0x14,
};

struct vqm_wr_used_t
{
    uint16_t rx_used_ring_t;  /* rx超时写已用环时间查询结果或配置结果，单位us */
    uint16_t tx_used_ring_t;  /* tx超时写已用环时间查询结果或配置结果，单位us */
}__attribute__ ((packed));

struct vqm_msix_mode
{
    uint32_t intr_adaptataion_flag;
}__attribute__ ((packed));

struct vqm_phy_qid
{
#define ZXDH_VNET_ZTE  (0x6)
    uint8_t version;
    uint8_t qnum;
#define MAX_QNUM    ((ZXDH_QUEUE_PAIRS_MAX + 1) * 2)
    uint32_t qid[MAX_QNUM];
}__attribute__ ((packed));

struct vqm_flow_cfg
{
    uint32_t pps;
    uint32_t kbps;
}__attribute__ ((packed));

struct vqm_queue_get
{
    uint16_t start_qid;
    uint16_t qpair;
}__attribute__ ((packed));

struct vqm_coalesce_data
{
    uint16_t rx_coalesce_usecs;    /* rx聚合时间，单位us */
    uint16_t tx_coalesce_usecs;    /* tx聚合时间，单位us */
    uint16_t rx_coalesced_frames;  /* rx聚合包数 */
    uint16_t tx_coalesced_frames;  /* tx聚合包数 */
}__attribute__ ((packed));

typedef struct {
    uint16_t vqm_vfid; /* 设备号，填写任意值 */
    uint16_t opcode;   /* 查询 0， 配置 1 */
    uint16_t cmd;     /* cmd - 0x10 */

    union
    {
        struct vqm_wr_used_t wr_used_t;
        struct vqm_msix_mode msix_mode_sel;
        struct vqm_phy_qid qid_reset_msg;
        struct vqm_flow_cfg vqm_vf_fc;
    }__attribute__ ((packed));
}__attribute__ ((packed)) host_to_vqm_msg;

typedef struct {
    uint32_t check_result;  /* 执行结果， 如果成功为0xaa, 其他失败*/
    union
    {
        struct vqm_wr_used_t wr_used_t;
        struct vqm_msix_mode msix_mode_sel;
        struct vqm_flow_cfg vqm_vf_fc;
        struct vqm_queue_get queue;
        struct vqm_coalesce_data coalesce_data;
    }__attribute__ ((packed));
}__attribute__ ((packed)) vqm_rsp_host_data;

struct fd_flow_key {
    uint8_t mac_dst[ZXDH_MAC_NUM];
    uint8_t mac_src[ZXDH_MAC_NUM];
    uint16_t ether_type;
    union {
        struct {
            uint16_t cvlan_pri:4;
            uint16_t cvlan_vlanid:12; /* vlanid 0xfff is valid */
        };
        uint16_t vlan_tci;
    };

    uint8_t src_ip[16];
    uint8_t dst_ip[16];
    uint8_t rsv0;
    union {
        uint8_t tos;
        uint8_t tc;
    };
    uint8_t nw_proto;
    uint8_t frag_flag;
    uint16_t tp_src;
    uint16_t tp_dst;

    uint8_t rsv1;
    uint8_t vni[3];

    uint16_t vfid;
    uint8_t rsvs[18];
} __attribute__((packed));

struct fd_flow_result {
    uint16_t qid;
    uint8_t rsv0;

    uint8_t action_idx:7;
    uint8_t hit_flag:1;

    uint32_t mark_fd_id;
    uint32_t countid:20;
    uint32_t sriov_tunnel_encap1_index:12;

    uint16_t sriov_tunnel_encap0_index:12;
    uint16_t rsv1:4;
    uint8_t rss_hash_factor;
    uint8_t rss_hash_alg;
} __attribute__((packed));

struct fd_flow_entry {
    struct fd_flow_key key;
    struct fd_flow_key key_mask;
    struct fd_flow_result result;
} __attribute__((packed));

struct zxdh_flow_info {
    enum dh_flow_type flowtype;
    uint16_t hw_idx;
    uint16_t rsv;
    union {
        struct fd_flow_entry fd_flow;
    };
} __attribute__((packed));

struct zxdh_flow {
    uint8_t direct;
    uint8_t group;
    uint8_t pri;
    uint8_t hash_search_idx;
    struct zxdh_flow_info flowentry;
} __attribute__((packed));

typedef struct
{
    zxdh_msg_op_code op_code;
    uint16_t vport;
    uint16_t vf_id;
    uint16_t pcie_id;
} __attribute__((packed)) zxdh_msg_head_to_pf;

typedef struct
{
    zxdh_msg_op_code op_code;
    uint16_t dst_pcie_id;
} __attribute__((packed)) zxdh_msg_head_to_vf;

typedef struct
{
    bool link_up;
    bool is_upf;
    uint16_t base_qid;
    uint8_t mac_addr[ZXDH_MAC_NUM];
    uint32_t speed;
    uint32_t autoneg_enable;
    uint32_t sup_link_modes;
    uint32_t adv_link_modes;
    uint8_t hash_search_idx;
    uint8_t duplex;
    uint8_t phy_port;
    uint8_t rss_enable;
    uint16_t vlan_id;
    uint16_t tpid;
    uint8_t vlan_qos;
    uint8_t addr_assign_type;
} __attribute__((packed)) zxdh_vf_init_msg;

typedef struct
{
    bool is_upf;
    bool uc_promisc;
    bool mc_promisc;
    uint8_t hash_search_idx;
    uint16_t base_qid;
    uint8_t vlan_qos;
    uint8_t hash_func;
    uint32_t hash_mode;
    uint32_t queue_map[ZXDH_INDIR_RQT_SIZE];
    uint8_t link_up;
    uint8_t speed;
    uint8_t duplex;
#define VLAN_BITMAP_BYTE_SIZE  (512)
    uint8_t vlan_trunk_bitmap[VLAN_BITMAP_BYTE_SIZE];
} __attribute__((packed)) zxdh_vf_reload_msg;

typedef struct
{
    uint32_t vfid;
    uint32_t call_np_interface_num;
    uint32_t ptp_tc_enable_opt;
} __attribute__((packed)) zxdh_vf_1588_call_np;

typedef struct
{
    uint8_t rss_enable;
} __attribute__((packed)) zxdh_rss_enable_msg;

typedef struct
{
    uint8_t fd_enable;
} __attribute__((packed)) zxdh_fd_enable_msg;

typedef struct {
    struct ethtool_rx_flow_spec fs;
    uint32_t index;
}__attribute__((packed)) zxdh_vf_fd_cfg_msg;

typedef struct {
    ZXDH_FD_CFG_T fd_cfg;
    uint32_t index;
}__attribute__((packed)) zxdh_tc_vf_fd_cfg_msg;

typedef struct {
    uint32_t  index;
    uint16_t  stat_item;
    uint8_t   rd_clr;
}__attribute__((packed)) zxdh_vf_ppu_stat_msg;

typedef struct
{
    uint16_t rx_num;
} __attribute__((packed)) zxdh_vf_rx_num_msg;

typedef struct
{
    bool enable;
#define  VLAN_STRIP_MSG_TYPE  0
#define  QINQ_STRIP_MSG_TYPE  1
    uint8_t flag;
} __attribute__((packed)) zxdh_strip_enable_msg;

typedef struct
{
    uint16_t vf_idx;
    uint16_t vlan_id;
    uint8_t qos;
    uint8_t rsv;
    uint16_t protocol;
} __attribute__((packed)) zxdh_set_vf_vlan_msg;

typedef struct
{
    uint16_t tpid;
} __attribute__((packed)) zxdh_qinq_tpid_cfg_msg;

typedef struct
{
    uint32_t queue_map[ZXDH_INDIR_RQT_SIZE];
} __attribute__((packed)) zxdh_rxfh_set_msg;

typedef struct
{
    uint8_t key_map[ZXDH_NET_HASH_KEY_SIZE];
} __attribute__((packed)) zxdh_thash_key_set_msg;

typedef struct
{
    uint16_t slot_id;
} __attribute__((packed)) zxdh_slot_id_msg;

typedef struct
{
    uint8_t func;
} __attribute__((packed)) zxdh_hfunc_set_msg;

typedef struct
{
    uint32_t hash_mode;
} __attribute__((packed)) zxdh_rx_flow_hash_set_msg;

typedef struct
{
    bool     mac_flag;
    uint8_t  filter_flag; /* 0xaa表示过滤，0Xff表示其他*/
    uint8_t  mac_addr[ZXDH_MAC_NUM];
} __attribute__((packed)) zxdh_mac_addr_msg;

typedef struct
{
    uint8_t  mac_addr[ZXDH_MAC_NUM];
} __attribute__((packed)) zxdh_ipv6_mac_addr_msg;

typedef struct
{
    uint32_t mode;
    uint32_t value;
    uint8_t allmulti_follow;
} __attribute__((packed)) zxdh_port_attr_set_msg;

#define ZXDH_PROMISC_MODE 1
#define ZXDH_ALLMULTI_MODE 2
typedef struct
{
    uint8_t mode;
    uint8_t value;
    uint8_t mc_follow;
} __attribute__((packed)) zxdh_promisc_set_msg;

typedef struct
{
    uint8_t rsv2;
    uint16_t read_bytes;
    uint8_t value;
}__attribute__((packed)) common_recv_msg;

typedef struct
{
    uint8_t rsv2;
    uint16_t read_bytes;
    uint16_t queue_nums;
    uint16_t phy_qidx[256];
}__attribute__((packed)) common_vq_msg;


#define MAX_ACCESS_NUM 500
typedef struct 
{
    uint32_t queue_idx;
    uint32_t desc_start;
    uint32_t desc_num;
    uint32_t vf_idx;
} __attribute__((packed)) zxdh_vf_queue_info_msg;

typedef enum
{
    AGENT_MAC_STATS_GET = 10,
    AGENT_MAC_STATS_CLEAR,
    AGENT_MAC_PHYPORT_INIT,
    AGENT_MAC_AUTONEG_SET,
    AGENT_MAC_LINK_INFO_GET,
    AGENT_MAC_LED_BLINK,
    AGENT_MAC_FEC_MODE_SET,
    AGENT_MAC_FEC_MODE_GET,
    AGENT_MAC_FC_MODE_SET,
    AGENT_MAC_FC_MODE_GET,
    AGENT_MAC_MODULE_EEPROM_READ,
    AGENT_VQM_DEVICE_STATS_GET,
    AGENT_VQM_STATS_CLEAR,
    AGENT_FLASH_FIR_VERSION_GET = 23,
    AGENT_DEV_STATUS_NOTIFY,
    AGENT_DEBUG_LLDP_ENABLE_SET,
    AGENT_DEBUG_LLDP_ENABLE_GET,
    AGENT_SSHD_START,
    AGENT_SSHD_STOP,
    AGENT_FLASH_MAC_READ,
    AGENT_FLASH_MAC_WRITE,
    AGENT_FLASH_MAC_ERASE,
    AGENT_MAC_RECOVERY_CLK_SET,
    AGENT_MAC_SYNCE_CLK_STATS_GET,
    AGENT_MAC_PORT_TSTAMP_ENABLE_SET,
    AGENT_MAC_PORT_TSTAMP_ENABLE_GET,
    AGENT_MAC_PORT_TSTAMP_MODE_SET,
    AGENT_MAC_PORT_TSTAMP_MODE_GET,
    AGENT_MAC_PORT_DELAY_VALUE_GET,
    AGENT_MAC_PORT_DELAY_VALUE_CLR,
    AGENT_SLOT_INFO_SEND = 40,
    AGENT_OS_TYPE_GET = 41,
    AGENT_DTP_STATS_GET,
    AGENT_SPM_PORT_ENABLE_SET,
    AGENT_MAC_MSG_NUM_MAX, /* should be at last */
} __attribute__((packed)) agent_msg_op_code;

typedef struct
{
    agent_msg_op_code op_code;
    uint8_t port_id;
    uint8_t phyport;
    uint8_t is_upf;
    uint16_t vf_id;
    uint16_t pcie_id;
} __attribute__((packed)) agent_msg_hdr;

typedef struct
{
    uint8_t autoneg;
    uint8_t link_state;
    uint8_t blink_enable;
    uint8_t duplex;
    uint32_t speed_modes;
    uint32_t speed;
} __attribute__((packed)) agent_mac_autoneg_msg;

typedef struct
{
    uint64_t rx_total;
    uint64_t tx_total;
    uint64_t rx_total_bytes;
    uint64_t tx_total_bytes;
    uint64_t rx_error;
    uint64_t tx_error;
    uint64_t rx_drop;
    uint64_t tx_drop;
    uint64_t rx_good_bytes;
    uint64_t tx_good_bytes;
    uint64_t rx_unicast;
    uint64_t tx_unicast;
    uint64_t rx_multicast;
    uint64_t tx_multicast;
    uint64_t rx_broadcast;
    uint64_t tx_broadcast;
    uint64_t rx_under64_drop;
    uint64_t rx_undersize;
    uint64_t rx_size_64;
    uint64_t rx_size_65_127;
    uint64_t rx_size_128_255;
    uint64_t rx_size_256_511;
    uint64_t rx_size_512_1023;
    uint64_t rx_size_1024_1518;
    uint64_t rx_size_1519_mru;
    uint64_t rx_oversize;
    uint64_t tx_undersize;
    uint64_t tx_size_64;
    uint64_t tx_size_65_127;
    uint64_t tx_size_128_255;
    uint64_t tx_size_256_511;
    uint64_t tx_size_512_1023;
    uint64_t tx_size_1024_1518;
    uint64_t tx_size_1519_mtu;
    uint64_t tx_oversize;
    uint64_t rx_pause;
    uint64_t tx_pause;
    uint64_t rx_fcs_error;
    uint64_t tx_fcs_error;
    uint64_t rx_mac_control;
    uint64_t tx_mac_control;
    uint64_t rx_fragment;
    uint64_t tx_fragment;
    uint64_t rx_jabber;
    uint64_t tx_jabber;
    uint64_t rx_vlan;
    uint64_t tx_vlan;
    uint64_t rx_eee;
    uint64_t tx_eee;
} __attribute__((packed)) agent_stats;

typedef struct
{
    uint64_t np_rx_vport_unicast_packets;
    uint64_t np_tx_vport_unicast_packets;
    uint64_t np_rx_vport_unicast_bytes;
    uint64_t np_tx_vport_unicast_bytes;
    uint64_t np_rx_vport_multicast_packets;
    uint64_t np_tx_vport_multicast_packets;
    uint64_t np_rx_vport_multicast_bytes;
    uint64_t np_tx_vport_multicast_bytes;
    uint64_t np_rx_vport_broadcast_packets;
    uint64_t np_tx_vport_broadcast_packets;
    uint64_t np_rx_vport_broadcast_bytes;
    uint64_t np_tx_vport_broadcast_bytes;
    uint64_t np_rx_vport_mtu_drop_packets;
    uint64_t np_tx_vport_mtu_drop_packets;
    uint64_t np_rx_vport_mtu_drop_bytes;
    uint64_t np_tx_vport_mtu_drop_bytes;
    uint64_t np_rx_vport_plcr_drop_packets;
    uint64_t np_tx_vport_plcr_drop_packets;
    uint64_t np_rx_vport_plcr_drop_bytes;
    uint64_t np_tx_vport_plcr_drop_bytes;
    uint64_t np_tx_vport_ssvpc_packets; // switch security violation packet count, only for PF.
    uint64_t rx_vport_idma_drop_packets; // port to np drop (idma point not enough).
    uint64_t np_rx_vport_fdir_hits_packets;
    uint64_t np_rx_vport_fdir_hits_bytes;
    uint64_t np_rx_vport_fdir_drop_packets;
    uint64_t np_rx_vport_fdir_drop_bytes;
}__attribute__((packed)) np_stats;

typedef struct
{
    uint8_t fec_cfg;
    uint8_t fec_cap;
    uint8_t fec_link;
} __attribute__((packed)) agent_mac_fec_mode_msg;

typedef struct
{
    uint8_t fc_mode;
} __attribute__((packed)) agent_mac_fc_mode_msg;

typedef struct
{
    uint16_t index;
} __attribute__((packed)) agent_flash_read_msg;

typedef struct
{
    uint8_t i2c_addr;
    uint8_t bank;
    uint8_t page;
    uint8_t offset;
    uint8_t length;
    uint8_t data[128];
} __attribute__((packed)) agent_mac_module_eeprom_msg;

typedef struct
{
    bool is_link_force_set;
    bool link_forced;
    bool link_up;
    uint32_t speed;
    uint32_t autoneg_enable;
    uint32_t supported_speed_modes;
    uint32_t advertising_speed_modes;
    uint8_t  duplex;
} __attribute__((packed)) zxdh_link_state_msg;

typedef struct
{
    bool enable;
} __attribute__((packed)) zxdh_vlan_filter_set_msg;

typedef struct
{
    uint16_t vlan_id;
} __attribute__((packed)) zxdh_rx_vid_add_msg;

typedef struct
{
    uint16_t vlan_id;
} __attribute__((packed)) zxdh_rx_vid_del_msg;

typedef struct
{
    uint8_t type;
    uint8_t field;
    uint16_t pcie_id;
    uint16_t write_bytes;
    uint16_t rsv;
} __attribute__((packed)) zxdh_common_tbl_hdr;

typedef struct
{
    uint8_t tmmng_type;
    uint8_t dir;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} __attribute__((packed)) zxdh_cfg_time_msg;

typedef struct
{
    uint16_t pcie_id;
    uint16_t write_bytes;
} __attribute__((packed)) zxdh_common_time_hdr;

typedef struct
{
    uint8_t clk_speed;
    uint8_t clk_stats;
} __attribute__((packed)) zxdh_synce_clk_msg;

typedef struct
{
    uint32_t tx_enable;
    uint32_t rx_enable;
    uint32_t tx_mode;
    uint32_t rx_mode;
} __attribute__((packed)) zxdh_mac_tstamp_msg;

typedef struct
{
    uint64_t min_delay;
    uint64_t max_delay;
} __attribute__((packed)) zxdh_delay_statistics_val;

typedef struct
{
    uint16_t pcieid;            // 发送者自己的pcie id
    uint16_t extern_pps_vector; // 外部pps中断向量
    uint16_t local_pps_vector;  // local pps中断向量
    uint16_t pps_intr_support;  // riscv侧返回的是否支持pps中断
} __attribute__((packed)) zxdh_bar_msg_pps;

typedef struct
{
    uint8_t dev_id;       //用dbg module的id, 为0
    uint8_t type;         //区分PXE下和正常的np配表流程, 0: PXE 7: 配np
    uint8_t operate_mode; //区分PXE开始和结束, 0: 开始 1: 结束
    uint8_t pfNum;
    uint32_t portNum[10];
    uint32_t evid[10];
    uint32_t qid[10];
} __attribute__((packed)) zxdh_cfg_np_msg;

#define MAX_HDR_LEN 8

typedef struct
{
    char ifname[IFNAMSIZ];
    uint8_t mac[ETH_ALEN];
    uint16_t pannel_id;
    uint16_t ctl;
    uint16_t rsv;
} __attribute__((packed)) zxdh_pf_cfg_mac_msg;

#define MAX_VF_NUM 256
typedef struct
{
    uint16_t num;
    uint16_t func_no[MAX_VF_NUM];
} __attribute__((packed)) agent_pcie_msix_msg;

typedef struct
{
    bool lldp_enable;
} __attribute__((packed)) zxdh_lldp_enable_msg;

typedef struct
{
    uint32_t  flowid;
    uint32_t  car_type;
    uint32_t  is_packet;
    uint32_t  max_rate;
    uint32_t  min_rate;
} __attribute__((packed)) zxdh_rate_limit_set_msg;

/*vf send message to pf to map flow id between CARS*/
typedef struct
{
    uint32_t car_type;
    uint32_t flowid;
    uint32_t map_flowid;
    uint32_t sp;
} __attribute__((packed)) zxdh_plcr_flowid_map_msg;

typedef struct
{
    uint32_t car_type;
    uint32_t flowid;
    uint32_t profile_id;
} __attribute__((packed)) zxdh_plcr_flow_init_msg;

typedef struct
{
    uint32_t vir_queue_start;
    uint32_t vir_queue_num;
} __attribute__((packed)) zxdh_plcr_pf_get_vf_queue_info_msg;

typedef struct
{
    uint16_t vport;
    uint16_t mode;
} __attribute__((packed)) zxdh_plcr_work_mode_msg;


/*用户态QOS申请限速模板*/
typedef struct
{
    uint8_t car_type;
} __attribute__((packed)) zxdh_vf_plcr_profile_id_add_msg;

/*用户态QOS删除限速模板*/
typedef struct
{
    uint8_t  car_type;
    uint8_t  rsvd;
    uint16_t profile_id;
} __attribute__((packed)) zxdh_vf_plcr_profile_id_delete_msg;

/*用户态QOS配置限速模板*/
typedef struct
{
    uint8_t  car_type;
    uint8_t  pkt_mode;
    uint16_t profile_id;
    union    zxdh_plcr_profile_cfg profile_cfg;
} __attribute__((packed)) zxdh_vf_plcr_profile_cfg_set_msg;

/*用户态QOS获取限速模板*/
typedef struct
{
    uint8_t  car_type;
    uint8_t  pkt_mode;
    uint16_t profile_id;
} __attribute__((packed)) zxdh_vf_plcr_profile_cfg_get_msg;

/*用户态QOS绑定flow和profile*/
typedef struct
{
    uint8_t  car_type;
    uint8_t  drop_flag;
    uint8_t  plcr_en;
    uint8_t  rsvd;
    uint16_t flow_id;
    uint16_t profile_id;
} __attribute__((packed)) zxdh_vf_plcr_queue_cfg_set_msg;

/*用户态QOS获取plcr丢包统计*/
typedef struct
{
    uint8_t direction; //取值为1：获取tx方向的统计计数；取值为0：获取rx方向的统计计数
    uint8_t is_clr;    //取值为1：读取之后，计数器清零，取值为0：读取之后，计数器不清零
} __attribute__((packed)) zxdh_vf_plcr_port_meter_stat_get_msg;

typedef struct
{
    uint16_t port;
} __attribute__((packed)) zxdh_vf_vxlan_port_msg;

typedef struct
{
    uint32_t clear_mode;
    bool is_init_get;
    bool fd_enable;
} __attribute__((packed)) zxdh_np_stats_get_msg;

struct zxdh_port_msg
{
    uint16_t pcie_id;
    uint8_t  rsv[2];
} __attribute__((packed));

struct port_message_recv
{
    uint8_t hdr[4];
    uint8_t port_num; //bond组中端口个数
    uint8_t bond_num; //bond组个数
    uint8_t bond_idx;
    uint8_t no_bondpf_panel; //不属于bond pf的面板个数
    struct zxdh_pannle_port data[16];
} __attribute__((packed));

typedef struct
{
    uint32_t enable;
} __attribute__((packed)) zxdh_spm_port_enable;

struct sriov_tunnel_encap_ip_dip
{
    uint8_t ip_addr[16];
} __attribute__((packed));

struct sriov_tunnel_encap0 {
    uint8_t tos;
    uint8_t rsv2[2];
    uint8_t rsv1: 6;
    uint8_t eth_type: 1;
    uint8_t hit_flag: 1;
    uint16_t dst_mac1;
    uint16_t tp_dst;
    uint32_t dst_mac2;

    uint32_t encap_ttl:8;
    uint32_t vni:24;
    struct sriov_tunnel_encap_ip_dip dip;
};

struct sriov_tunnel_encap_ip_sip
{
    uint8_t ip_addr[16];
} __attribute__((packed));

struct sriov_tunnel_encap1 {
    uint32_t rsv1: 31;
    uint32_t hit_flag: 1;

    uint16_t src_mac1;
    uint16_t vlan_tci;
    uint32_t src_mac2;
    uint32_t rsv;
    struct sriov_tunnel_encap_ip_dip sip;
};

typedef struct {
    struct zxdh_flow dh_flow;
    struct sriov_tunnel_encap0 encap0;
    struct sriov_tunnel_encap1 encap1;
} __attribute__((packed)) zxdh_flow_op_msg;

typedef struct {
    uint32_t dev_id;
    uint32_t index;
} __attribute__((packed)) zxdh_mcode_feature_msg;

typedef struct {
    uint32_t vfid;
} __attribute__((packed)) zxdh_k_cmpat_msg;

typedef struct{
    uint16_t sum_check;
    uint8_t opcode;
    uint8_t rsv;
}__attribute__((packed)) health_msg_hdr;

typedef struct {
    uint16_t pcie_id;
    uint16_t vector;
}__attribute__((packed)) pf_status_msg;

 typedef struct  {
    uint8_t act;
}__attribute__((packed)) health_config_msg;

typedef struct {
    uint8_t  err_stat_flag:1; /*1: 取异常统计，0：取所有统计*/
    uint8_t  que_stat_flag:1; /*1:取各队列统计  0: 取端口统计*/
    uint8_t  rd_clr:1;       /* 读清标记，0：不读清  1：读清*/
    uint8_t  rsv0:5;
    uint16_t vf_idx;       /* 指定vf */
    uint16_t start_index;  /* 指定获取的起始队列号0~223 */
    uint16_t queue_num;    /* 每次获取的最大队列数，最大值32个*/
}__attribute__((packed)) zxdh_get_sw_stats;


typedef enum
{
    ZXDH_VF_1588_ENABLE_SET,
    ZXDH_VF_1588_ENABLE_GET,
    ZXDH_VF_1588_CMD_CNT_MAX, /* should be at last */
}__attribute__((packed)) zxdh_1588_vf_op_code;

typedef struct {
    uint32_t proc_cmd;
    uint32_t enable_1588_vf;
}__attribute__((packed)) zxdh_vf_1588_enable;

typedef struct {
    uint32_t enable_1588_vf_rsp;
}__attribute__((packed)) zxdh_vf_1588_enable_rsp;

typedef struct {
    uint32_t vfid;
    uint16_t pcie_id;
    uint16_t rsv;
    uint64_t bits;
}__attribute__((packed)) zxdh_vqmb_hdr;

typedef struct {
    uint16_t port_enable;
    uint16_t version;
}__attribute__((packed)) zxdh_vqmb_port_ctrl_msg;

typedef struct {
    zxdh_vqmb_hdr vqmb_hdr;
    zxdh_vqmb_port_ctrl_msg vqmb_port_ctrl_msg;
}__attribute__((packed)) vqmb_to_host_msg;

typedef enum
{
    ZXDH_VF_RSSKEY_IPID_SET,
    ZXDH_VF_RSSKEY_IPID_GET,
    ZXDH_VF_RSSKEY_IPID_CMD_CNT_MAX, /* should be at last */
}__attribute__((packed)) zxdh_rsskey_ipid_vf_opcode;

typedef struct {
    uint32_t proc_cmd;
    uint32_t enable_vf_rsskey_ipid;
}__attribute__((packed)) zxdh_vf_rsskey_ipid;

typedef struct {
    uint32_t enable_rsskey_ipid_rsp;
}__attribute__((packed)) zxdh_vf_rsskey_ipid_rsp;

typedef struct
{
    union
    {
        uint8_t len[MAX_HDR_LEN];
        zxdh_msg_head_to_pf hdr;
        zxdh_msg_head_to_vf hdr_vf;
        agent_msg_hdr hdr_to_agt;
        zxdh_common_tbl_hdr hdr_to_cmn;
        zxdh_common_time_hdr hdr_time_to_cmn;
        health_msg_hdr health_hdr;
    }; /* should be no more than MAX_HDR_LEN */

    union
    {
        zxdh_vf_queue_info_msg vf_queue_info_msg;
        zxdh_vf_reload_msg vf_reload_msg;
        zxdh_rss_enable_msg rss_enable_msg;
        zxdh_rxfh_set_msg rxfh_set_msg;
        zxdh_thash_key_set_msg thash_key_set_msg;
        zxdh_hfunc_set_msg hfunc_set_msg;
        zxdh_rx_flow_hash_set_msg rx_flow_hash_set_msg;
        zxdh_mac_addr_msg mac_addr_set_msg;
        zxdh_port_attr_set_msg port_attr_set_msg;
        zxdh_promisc_set_msg promisc_set_msg;
        zxdh_link_state_msg link_state_msg;
        zxdh_vlan_filter_set_msg vlan_filter_set_msg;
        zxdh_rx_vid_add_msg rx_vid_add_msg;
        zxdh_rx_vid_del_msg rx_vid_del_msg;
        agent_mac_autoneg_msg mac_set_msg;
        agent_mac_fec_mode_msg mac_fec_mode_msg;
        agent_mac_fc_mode_msg mac_fc_mode_msg;
        agent_mac_module_eeprom_msg module_eeprom_msg;
        agent_flash_read_msg flash_read_msg;
        zxdh_vf_init_msg vf_init_msg;
        zxdh_strip_enable_msg vlan_strip_msg;
        zxdh_set_vf_vlan_msg vf_vlan_msg;
        zxdh_qinq_tpid_cfg_msg tpid_cfg_msg;
        zxdh_pf_cfg_mac_msg mac_cfg_msg;
        zxdh_cfg_time_msg time_cfg_msg;
        agent_pcie_msix_msg pcie_msix_msg;
        zxdh_lldp_enable_msg lldp_msg;
        zxdh_rate_limit_set_msg rate_limit_set_msg;
        zxdh_plcr_flowid_map_msg plcr_flowid_map_msg;
        zxdh_plcr_flow_init_msg plcr_flow_init_msg;
        zxdh_plcr_pf_get_vf_queue_info_msg plcr_pf_get_vf_queue_info_msg;
        zxdh_plcr_work_mode_msg plcr_work_mode_msg;

        /*用户态QOS申请限速模板*/
        zxdh_vf_plcr_profile_id_add_msg vf_plcr_profile_id_add_msg;
        /*用户态QOS删除限速模板*/
        zxdh_vf_plcr_profile_id_delete_msg vf_plcr_profile_id_delete_msg;
        /*用户态QOS配置限速模板*/
        zxdh_vf_plcr_profile_cfg_set_msg vf_plcr_profile_cfg_set_msg;
        /*用户态QOS获取限速模板*/
        zxdh_vf_plcr_profile_cfg_get_msg vf_plcr_profile_cfg_get_msg;
        /*用户态QOS绑定flow和profile*/
        zxdh_vf_plcr_queue_cfg_set_msg  vf_plcr_queue_cfg_set_msg;
        /*用户态QOS获取plcr丢包统计*/
        zxdh_vf_plcr_port_meter_stat_get_msg vf_plcr_port_meter_stat_get_msg;
        zxdh_vf_vxlan_port_msg vf_vxlan_port;
        zxdh_np_stats_get_msg np_stats_get_msg;

        zxdh_vf_1588_call_np vf_1588_call_np;
        zxdh_synce_clk_msg synce_clk_recovery_port;
        zxdh_mac_tstamp_msg mac_tstamp_msg;
        zxdh_bar_msg_pps msg_pps;
        uint16_t cmn_tbl_msg[257];
        zxdh_spm_port_enable spm_port_enable_set;
        zxdh_flow_op_msg flow_msg;
        zxdh_mcode_feature_msg mcode_feature_msg;
        zxdh_k_cmpat_msg kernel_cmpat_msg;
        health_config_msg health_config_msg;
        pf_status_msg pf_status_msg;
        zxdh_get_sw_stats vf_sw_stats;
        zxdh_vf_1588_enable vf_1588_enable;
        zxdh_vf_fd_cfg_msg vf_fd_cfg_msg;
        zxdh_fd_enable_msg vf_fd_enable_msg;
        zxdh_vf_rx_num_msg vf_rx_num_msg;
        zxdh_vf_rsskey_ipid vf_rsskey_ipid;
        zxdh_tc_vf_fd_cfg_msg  tc_vf_fd_cfg_msg;
        zxdh_vf_ppu_stat_msg  vf_ppu_stat_msg;

    };
} zxdh_msg_info;

typedef enum
{
    ZXDH_REPS_FAIL,
    ZXDH_REPS_SUCC = 0xaa,
    ZXDH_INVALID_OP_CODE = 0xee,
} __attribute__((packed)) zxdh_reps_flag;

typedef enum
{
    GET_STAT_SUCCESS = 0,
    GET_STAT_FAILED = 1,
    VF_ERR = 2,
    ACTION_IS_NOT_SUPPORTED = 3,
} __attribute__((packed)) zxdh_get_sw_stats_flag;

typedef struct
{
    uint8_t lldp_status;
} __attribute__((packed)) agent_debug_lldp_msg;

typedef struct
{
    uint8_t firmware_version[FW_VERSION_LEN];
} __attribute__((packed)) agent_flash_msg;

typedef struct
{
    uint8_t mac[ETH_ALEN];
} __attribute__((packed)) agent_flash_mac_read_msg;

typedef struct
{
    uint32_t phy_queue_num;
    uint16_t phy_rxq[16];
    uint16_t phy_txq[16];
} __attribute__((packed)) zxdh_plcr_pf_get_vf_queue_info_rsp;

typedef struct
{
    int32_t  err_code;
} __attribute__((packed)) zxdh_rate_limit_set_rsp;

typedef struct
{
    uint8_t mode;
} __attribute__((packed)) zxdh_plcr_work_mode_rsp;

/*用户态QOS申请限速模板，返回模板profile_id*/
typedef struct
{
    uint16_t profile_id;
} __attribute__((packed)) zxdh_vf_plcr_profile_id_add_rsp;

/*用户态QOS获取限速模板，返回模板参数*/
typedef struct
{
    union zxdh_plcr_profile_cfg profile_cfg;
} __attribute__((packed)) zxdh_vf_plcr_profile_cfg_get_rsp;

/*用户态QOS获取丢包统计*/
typedef struct
{
    uint64_t drop_pkB_cnt;  //丢数据包：包数统计
    uint64_t drop_pk_cnt;   //丢数据包：字节数计数
} __attribute__((packed)) zxdh_vf_plcr_port_meter_stat_get_rsp;

typedef struct
{
    ZXDH_SRIOV_VPORT_T port_attr_entry;
} __attribute__((packed)) zxdh_port_attr_get_msg;

typedef struct
{
    uint8_t mac_err_flag;
}__attribute__((packed)) zxdh_port_mac_set_rsp;

typedef struct {
    uint32_t index;
} __attribute__((packed)) zxdh_fd_cfg_reply;

typedef struct
{
    uint64_t pkt_cnt;    //包数统计
    uint64_t byte_cnt;   //字节数计数
} __attribute__((packed)) zxdh_vf_ppu_stat_get_rsp;
struct rte_flow_query_count
{
    uint32_t reset:1;
    uint32_t hits_set:1;
    uint32_t bytes_set:1;
    uint32_t reserved:29;
    uint64_t hits;
    uint64_t bytes;
};

struct err_reason
{
    uint8_t err_type;
    uint8_t rsv[3];
    char reason[512];
} __attribute__((packed));

typedef struct
{
    struct zxdh_flow dh_flow;
    union {
        struct rte_flow_query_count count;
        struct err_reason error;
    };
} __attribute__((packed)) zxdh_flow_op_rsp;

typedef struct {
    uint64_t len;
    uint64_t feature;
} __attribute__((packed)) zxdh_mcode_feature_rsp;

typedef struct {
    uint64_t  k_msg_idmax;
} __attribute__((packed)) zxdh_k_cmpat_rsp;

typedef struct {
    uint64_t truncated_err;
    uint64_t offload_cfg_err;
    uint64_t invalid_hdr_len_err;
    uint64_t no_segs_err;
} __attribute__((packed)) err_stats;

typedef struct {
    err_stats rx_stats;
    err_stats tx_stats;
} __attribute__((packed)) sw_stats;

typedef struct {
    uint8_t queue_state;  /*0:rx 1:tx*/
    err_stats stats;
} __attribute__((packed)) que_err_stats;

typedef struct {
    union {
        que_err_stats que_stats[MAX_QUE_CNT];
        sw_stats port_stats;
    };
} __attribute__((packed)) zxdh_sw_stats_reply;

typedef struct
{
    uint64_t rx_arn_phy;
    uint64_t tx_psn_phy;
    uint64_t rx_psn_phy;
    uint64_t tx_psn_ack_phy;
    uint64_t rx_psn_ack_phy;
}__attribute__((packed)) udp_phy_stats;

typedef struct
{
    zxdh_reps_flag flag;
    union
    {
        zxdh_vf_reload_msg vf_reload_msg;
        zxdh_thash_key_set_msg thash_key_set_msg;
        zxdh_rx_flow_hash_set_msg rx_flow_hash_set_msg;
        zxdh_link_state_msg link_state_msg;
        agent_mac_autoneg_msg mac_set_msg;
        agent_stats stats_msg;
        np_stats np_stats_msg;
        udp_phy_stats udp_phy_stats_msg;
        agent_mac_fec_mode_msg mac_fec_mode_msg;
        agent_mac_fc_mode_msg mac_fc_mode_msg;
        agent_mac_module_eeprom_msg module_eeprom_msg;
        common_recv_msg cmn_recv_msg;
        common_vq_msg cmn_vq_msg;
        zxdh_port_mac_set_rsp vf_mac_set_msg;
        zxdh_mac_addr_msg vf_mac_addr_get_msg;
        zxdh_vf_init_msg vf_init_msg;
        agent_flash_msg flash_msg;
        agent_flash_mac_read_msg flash_mac_read_msg;
        agent_debug_lldp_msg debug_lldp_msg;
        zxdh_plcr_pf_get_vf_queue_info_rsp plcr_pf_get_vf_queue_info_rsp;
        zxdh_rate_limit_set_rsp rate_limit_set_rsp;
        zxdh_plcr_work_mode_rsp plcr_work_mode_rsp;

        /*用户态QOS申请限速模板返回profile_id*/
        zxdh_vf_plcr_profile_id_add_rsp vf_plcr_profile_id_add_rsp;
        /*用户态QOS获取限速模板*/
        zxdh_vf_plcr_profile_cfg_get_rsp vf_plcr_profile_cfg_get_rsp;
        /*用户态QOS获取丢包统计*/
        zxdh_vf_plcr_port_meter_stat_get_rsp vf_plcr_port_meter_stat_get_rsp;
        zxdh_synce_clk_msg synce_clk_recovery_port;
        zxdh_mac_tstamp_msg mac_tstamp_msg;
        zxdh_delay_statistics_val delay_statistics_val;
        zxdh_port_attr_get_msg port_attr_get_msg;
        zxdh_rxfh_set_msg rxfh_get_msg;
        zxdh_bar_msg_pps msg_pps;
        zxdh_slot_id_msg slot_info;
        zxdh_flow_op_rsp flow_rsp;
        zxdh_mcode_feature_rsp mcode_feature_rsp;
        zxdh_k_cmpat_rsp kernel_cmpat_rsp;
        zxdh_sw_stats_reply vf_sw_stats_rsp;
        zxdh_vf_1588_enable_rsp vf_1588_enable_rsp;
        zxdh_fd_cfg_reply fd_cfg_resp;
        zxdh_vf_rsskey_ipid_rsp vf_rsskey_ipid_rsp;
        zxdh_vf_ppu_stat_get_rsp vf_ppu_stat_rsp;
    };
} zxdh_reps_info;

union zxdh_msg
{
    zxdh_msg_info payload;
    zxdh_reps_info reps;
    vqm_rsp_host_data vqm_reps;
    host_to_vqm_msg vqm_msg;
};

static inline uint16_t sum_func(void *data, uint16_t len)
{
    uint64_t result = 0;
    int idx = 0;
    uint16_t ret = 0;

    if (data == NULL)
    {
        return 0;
    }

    for (idx = 0; idx < len; idx++)
    {
        result += *((uint8_t *)data + idx);
    }

    ret = (uint16_t)result;
    return ret;
}

#ifdef __cplusplus
}
#endif

#endif

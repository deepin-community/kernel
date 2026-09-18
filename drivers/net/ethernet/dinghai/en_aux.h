#ifndef __ZXDH_EN_AUX_H__
#define __ZXDH_EN_AUX_H__

#include "msg_common.h"
#include "zxdh_tools/zxdh_tools_ioctl.h"
#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/dh_cmd.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/en_aux.h>
#include <linux/dinghai/eq.h>
#ifndef CGS_V5_693
#include <linux/compiler_types.h>
#include <linux/rhashtable.h>
#endif
#include <linux/types.h>
#include "./en_aux/queue.h"
#include "./en_aux/en_aux_cmd.h"
#include "./en_pf.h"
#include "./en_aux/dcbnl/en_dcbnl.h"
#include "./en_np/driver/include/dpp_drv_hash.h"
#include "./en_pf/msg_func.h"
#include "./en_ethtool/ethtool.h"

#define MAX_VLAN_ID            (4095)
#define MAX_QOS_ID             (7)
#define VLAN_BITMAP_LENGTH     (MAX_VLAN_ID + 1)
#define VLAN_BITMAP_BYTE_SIZE  (512)
#define BIT_NUM_PER_BYTE       (8)

#define PF_AC_MASK     0x800
#define FILTER_MAC     0xAA
#define UNFILTER_MAC   0xFF

#define AUX_INIT_INCOMPLETED  0
#define AUX_INIT_COMPLETED    1

#define IS_DELAY_STATISTICS_PKT      0
#define IS_NOT_DELAY_STATICTICS_PKT  1

#define ADD_IP6MAC 1
#define DEL_IP6MAC 2

#define WAKE_MAGIC		(1 << 5)

#define TUNNEL_RX_OUTER_L4_CKSUM_FLAG (1ULL << 62)

#define UINT64_MAX    (0xFFFFFFFFFFFFFFFF)

#define CALC_DELTA_64(curr, prev) ({ \
    typeof(curr) _curr = (curr); \
    typeof(prev) _prev = (prev); \
    (_curr >= _prev) ? (_curr - _prev) : (_curr + (UINT64_MAX - _prev) + 1); \
})

/* IPv6 MAC work data structure */
struct zxdh_ip6mac_work_data {
    uint32_t addr6[4];  /* IPv6 address */
    uint8_t ip6mac[ETH_ALEN];  /* MAC address */
};

/* IPv6 MAC work item - each work item has its own data */
struct zxdh_ip6mac_work_item {
    struct work_struct work;
    struct zxdh_en_device *en_dev;
    struct zxdh_ip6mac_work_data data;
};

struct zxdh_vlan_to_mac
{
    struct list_head list;
    uint8_t addr[ETH_ALEN];
    uint16_t vlan_id;
    struct rcu_head rcu_head; /* 实现安全的并发访问 */
};

struct zxdh_vlan_to_mac_list
{
    struct list_head list;
    int32_t count;
    spinlock_t lock;  // 保护锁
};

extern const uint8_t BOND_MCAST_ADDR[ETH_ALEN];

#define ZXDH_AUX_INIT_COMP_CHECK(en_dev) \
    do {                                                   \
        if (en_dev->init_comp_flag != AUX_INIT_COMPLETED)  \
        {                                                  \
            return;                          \
        }                                                  \
    } while (0)

typedef int (*zxdh_feature_handler)(struct zxdh_en_device *en_dev, bool enable);

extern uint32_t max_pairs;

struct zxdh_rdma_if;
struct zxdh_en_if;
struct zxdh_sec_if;

struct zxdh_en_container {
    struct zxdh_auxiliary_device adev;
    struct zxdh_rdma_dev_info *rdma_infos;
    struct zxdh_rdma_if *rdma_ops;
    struct zxdh_en_if *ops;
    struct dh_core_dev *parent;
    int32_t aux_id;
    struct zxdh_sec_if *sec_ops;
    void *auxiliary_ops[17]; //max support 20 auxiliary devices
};

struct zxdh_en_queue_stats
{
    uint64_t q_rx_pkts;
    uint64_t q_tx_pkts;
    uint64_t q_rx_bytes;
    uint64_t q_tx_bytes;
    uint64_t q_tx_stopped;
    uint64_t q_tx_wake;
    uint64_t q_tx_dropped;
};

struct zxdh_en_netdev_stats
{
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t tx_queue_wake;
    uint64_t tx_queue_stopped;
    uint64_t tx_queue_dropped;
    uint64_t rx_removed_vlan_packets;
    uint64_t tx_added_vlan_packets;
    uint64_t rx_csum_unnecessary;
    uint64_t rx_csum_none;
    uint64_t rx_csum_unnecessary_tunnel;
};

struct zxdh_en_vport_vqm_stats
{
    uint64_t rx_vport_packets;
    uint64_t tx_vport_packets;
    uint64_t rx_vport_bytes;
    uint64_t tx_vport_bytes;
    uint64_t rx_vport_dropped;
};

struct zxdh_en_vport_dtp_stats
{
    uint64_t rx_lro_packets;
    uint64_t rx_udp_csum_fail_packets;
    uint64_t tx_udp_csum_fail_packets;
    uint64_t rx_tcp_csum_fail_packets;
    uint64_t tx_tcp_csum_fail_packets;
    uint64_t rx_ipv4_csum_fail_packets;
    uint64_t tx_ipv4_csum_fail_packets;
};

struct zxdh_en_vport_stats
{
    struct zxdh_en_vport_vqm_stats vqm_stats;
    struct zxdh_en_vport_np_stats np_stats;
    struct zxdh_en_vport_dtp_stats dtp_stats;
};

struct zxdh_en_phy_stats
{
    uint64_t rx_packets_phy;
    uint64_t tx_packets_phy;
    uint64_t rx_bytes_phy;
    uint64_t tx_bytes_phy;
    uint64_t rx_error_phy;
    uint64_t tx_error_phy;
    uint64_t rx_drop_phy;
    uint64_t tx_drop_phy;
    uint64_t rx_good_bytes_phy;
    uint64_t tx_good_bytes_phy;
    uint64_t rx_unicast_phy;
    uint64_t tx_unicast_phy;
    uint64_t rx_multicast_phy;
    uint64_t tx_multicast_phy;
    uint64_t rx_broadcast_phy;
    uint64_t tx_broadcast_phy;
    uint64_t rx_under64_drop;
    uint64_t rx_undersize_phy;
    uint64_t rx_size_64_phy;
    uint64_t rx_size_65_127;
    uint64_t rx_size_128_255;
    uint64_t rx_size_256_511;
    uint64_t rx_size_512_1023;
    uint64_t rx_size_1024_1518;
    uint64_t rx_size_1519_mru;
    uint64_t rx_oversize_phy;
    uint64_t tx_undersize_phy;
    uint64_t tx_size_64_phy;
    uint64_t tx_size_65_127;
    uint64_t tx_size_128_255;
    uint64_t tx_size_256_511;
    uint64_t tx_size_512_1023;
    uint64_t tx_size_1024_1518;
    uint64_t tx_size_1519_mtu;
    uint64_t tx_oversize_phy;
    uint64_t rx_pause_phy;
    uint64_t tx_pause_phy;
    uint64_t rx_crc_errors;
    uint64_t tx_crc_errors;
    uint64_t rx_mac_control_phy;
    uint64_t tx_mac_control_phy;
    uint64_t rx_fragment_phy;
    uint64_t tx_fragment_phy;
    uint64_t rx_jabber_phy;
    uint64_t tx_jabber_phy;
    uint64_t rx_vlan_phy;
    uint64_t tx_vlan_phy;
    uint64_t rx_eee_phy;
    uint64_t tx_eee_phy;
}__attribute__((packed));

struct zxdh_en_udp_phy_stats
{
    uint64_t rx_arn_phy;
    uint64_t tx_psn_phy;
    uint64_t rx_psn_phy;
    uint64_t tx_psn_ack_phy;
    uint64_t rx_psn_ack_phy;
}__attribute__((packed));

struct zxdh_en_spm_stats
{
    uint64_t rx_total;
    uint64_t rx_pause;
    uint64_t rx_unicast;
    uint64_t rx_multicast;
    uint64_t rx_broadcast;
    uint64_t rx_vlan;
    uint64_t rx_size_64;
    uint64_t rx_size_65_127;
    uint64_t rx_size_128_255;
    uint64_t rx_size_256_511;
    uint64_t rx_size_512_1023;
    uint64_t rx_size_1024_1518;
    uint64_t rx_size_1519_mru;
    uint64_t rx_undersize;
    uint64_t rx_oversize;
    uint64_t rx_fragment;
    uint64_t rx_jabber;
    uint64_t rx_control;
    uint64_t rx_eee;

    uint64_t tx_total;
    uint64_t tx_pause;
    uint64_t tx_unicast;
    uint64_t tx_multicast;
    uint64_t tx_broadcast;
    uint64_t tx_vlan;
    uint64_t tx_size_64;
    uint64_t tx_size_65_127;
    uint64_t tx_size_128_255;
    uint64_t tx_size_256_511;
    uint64_t tx_size_512_1023;
    uint64_t tx_size_1024_1518;
    uint64_t tx_size_1519_mtu;
    uint64_t tx_undersize;
    uint64_t tx_oversize;
    uint64_t tx_fragment;
    uint64_t tx_jabber;
    uint64_t tx_control;
    uint64_t tx_eee;

    uint64_t rx_error;
    uint64_t rx_fcs_error;
    uint64_t rx_drop;

    uint64_t tx_error;
    uint64_t tx_fcs_error;
    uint64_t tx_drop;
} __attribute__((packed));

typedef struct
{
    uint64_t rx_total;
    uint64_t rx_pause;
    uint64_t rx_unicast;
    uint64_t rx_multicast;
    uint64_t rx_broadcast;
    uint64_t rx_vlan;
    uint64_t rx_size_64;
    uint64_t rx_size_65_127;
    uint64_t rx_size_128_255;
    uint64_t rx_size_256_511;
    uint64_t rx_size_512_1023;
    uint64_t rx_size_1024_1518;
    uint64_t rx_size_1519_mru;
    uint64_t rx_undersize;
    uint64_t rx_oversize;
    uint64_t rx_fragment;
    uint64_t rx_jabber;
    uint64_t rx_control;
    uint64_t rx_eee;
    uint64_t rx_error;
    uint64_t rx_fcs_error;
    uint64_t rx_drop;
    uint64_t rx_total_bytes;
    uint64_t rx_good_bytes;

    uint64_t tx_total;
    uint64_t tx_pause;
    uint64_t tx_unicast;
    uint64_t tx_multicast;
    uint64_t tx_broadcast;
    uint64_t tx_vlan;
    uint64_t tx_size_64;
    uint64_t tx_size_65_127;
    uint64_t tx_size_128_255;
    uint64_t tx_size_256_511;
    uint64_t tx_size_512_1023;
    uint64_t tx_size_1024_1518;
    uint64_t tx_size_1519_mtu;
    uint64_t tx_undersize;
    uint64_t tx_oversize;
    uint64_t tx_fragment;
    uint64_t tx_jabber;
    uint64_t tx_control;
    uint64_t tx_eee;
    uint64_t tx_error;
    uint64_t tx_fcs_error;
    uint64_t tx_drop;
    uint64_t tx_total_bytes;
    uint64_t tx_good_bytes;

} T_SPM_MAC_STATS;

struct zxdh_en_spm_bytes
{
    uint64_t rx_total_bytes;
    uint64_t rx_good_bytes;

    uint64_t tx_total_bytes;
    uint64_t tx_good_bytes;
} __attribute__((packed));

struct zxdh_en_idma_stats
{
    uint64_t rx_pkts;
    uint64_t rx_bytes;
} __attribute__((packed));

struct zxdh_en_tm_odma_stats
{
    uint64_t tx_pkts;
    uint64_t tx_bytes;
} __attribute__((packed));

struct zxdh_en_hw_stats
{
    struct zxdh_en_netdev_stats netdev_stats;
    struct zxdh_en_vport_stats vport_stats;
    struct zxdh_en_phy_stats phy_stats;
    struct zxdh_en_udp_phy_stats udp_stats;
    struct zxdh_en_queue_stats *q_stats;
    struct zxdh_en_idma_stats *idma_stats;
    struct zxdh_en_tm_odma_stats *tm_odma_stats;
};

struct zxdh_vlan_dev
{
    uint8_t qos;
    uint8_t rsv;
    uint16_t protocol;
    uint16_t vlan_id;
};

/* drs sec */
typedef struct
{
   uint64_t  SecVAddr;  /*每个设备的sec私有内存的虚拟基地址*/
   uint64_t  SecPAddr;  /*每个设备的sec私有内存的物理基地址*/
   uint32_t  SecMemSize; /*每个设备的sec私有内存的大小*/
}zxdh_sec_pri;

struct zxdh_sec_info
{
    dma_addr_t ring_dma_addr;
    dma_addr_t driver_event_dma_addr;
    dma_addr_t device_event_dma_addr;
    struct vring_packed_desc *desc;
    struct vring_packed_desc_event *driver;
    struct vring_packed_desc_event *device;
    size_t ring_size_in_bytes;
    size_t event_size_in_bytes;

    uint16_t desc_num;
    uint8_t queue_pairs;
    uint32_t phy_index;
    uint64_t notify_phy_addr;

    uint64_t bar0_phy_addr;
    uint64_t bar0_vir_addr;
    uint64_t bar0_size;
    uint16_t pcie_id;
    struct pci_dev *pdev;
};

struct zxdh_ethtool_table
{
    struct ethtool_rx_flow_spec rfs;
    uint32_t loc;
    uint32_t index;
    bool is_used;
};

struct mod_hdr_tbl {
    struct mutex lock; /* protects hlist */
    DECLARE_HASHTABLE(hlist, 8);
};

struct zxdh_tc_table {
    /* protects flow table */
    struct mutex                t_lock;
    struct zxdh_flow_table      *t;

    struct rhashtable           ht;

    struct mod_hdr_tbl mod_hdr;
    struct mutex hairpin_tbl_lock; /* protects hairpin_tbl */
    DECLARE_HASHTABLE(hairpin_tbl, 8);

    struct notifier_block     netdevice_nb;
};

struct zxdh_flow_steering
{
    struct zxdh_ethtool_table ethtool_fs[ETHTOOL_FD_MAX_NUM];
    uint32_t tot_num_rules;
    struct zxdh_tc_table           tc;
};
struct en_device_config {
    uint16_t rx_queue_size;
    uint16_t tx_queue_size;
    uint16_t num_rxq;
    uint16_t num_txq;
    uint16_t curr_combined;
    uint32_t hash_mode;
    uint8_t hash_func;
    uint8_t dev_addr[6];
    uint32_t queue_map[ZXDH_INDIR_RQT_SIZE];
    uint8_t vlan_trunk_bitmap[VLAN_BITMAP_BYTE_SIZE];
    struct recover_mac pf_recover_mac;
};

struct zxdh_pkt_file_info
{
    uint8_t *pkt_addr_array;
    uint32_t pkt_buf_len;
};

struct zxdh_pkt_save_file
{
    struct file *log_file;
    uint8_t enable_pkt_num_mode;
    uint32_t pkt_file_size;
    uint32_t pkt_set_count;
    uint32_t is_stop;
    uint32_t pkt_rbuf_idx;
    uint32_t pkt_ubuf_idx;
    uint32_t pkt_cur_num;
    char file_path[150];
    loff_t file_pos;
    size_t total_written_bytes;
};

enum zxdh_module_region_id {
    ZXDH_MOD_LOW_OFF0_LEN1,      /* A0h @ off 0, len 1   — identifier byte */
    ZXDH_MOD_LOW_OFF0_LEN128,    /* A0h @ off 0, len 128 */
    ZXDH_MOD_LOW_OFF128,         /* A0h @ off 128, len 128, page 0 bank 0 */
    ZXDH_MOD_LOW_PG1,            /* A0h page1 @ off 128, len 128 */
    ZXDH_MOD_LOW_PG2,
    ZXDH_MOD_LOW_PG3,
    ZXDH_MOD_LOW_PG17_BANK0,
    ZXDH_MOD_HIGH_OFF0_LEN128,   /* A2h @ off 0, len 128 */
    ZXDH_MOD_REGION_MAX,
};

struct zxdh_module_region {
    uint8_t  *data;
    uint8_t  len;
    int32_t  read_bytes;
};

struct zxdh_netlink_module_info
{
    struct mutex lock;
    struct zxdh_module_region    regions[ZXDH_MOD_REGION_MAX];
};

enum zxdh_allmulti_conf
{
    DO_NOT_CONFIGED = 0,
    OPENED_ALLMULTI = 1,
    CLOSED_ALLMULTI = 2,
};

struct zxdh_en_device {
    struct dh_core_dev *parent;
    struct net_device *netdev;
    struct device *dmadev;
    struct zxdh_en_if *ops;
    struct zxdh_en_hw_stats hw_stats;
    struct zxdh_en_vport_stats pre_stats;
    struct zxdh_en_vport_stats last_stats;
    struct zxdh_en_vport_stats high_count;
    struct zxdh_en_vport_stats extended_stats;
    spinlock_t vport_stats_lock;
    struct zxdh_vlan_dev vlan_dev;

    uint32_t device_id;
    uint32_t vendor_id;

    uint64_t driver_feature;
    uint64_t device_feature;
    uint64_t guest_feature;

    struct list_head vqs_list;
    spinlock_t vqs_list_lock;
    uint32_t indir_rqt[ZXDH_INDIR_RQT_SIZE];

    int32_t channels_num;

    /* a list of queues so we can dispatch IRQs */
    spinlock_t lock;
    struct list_head virtqueues;
    /* array of all queues for house-keeping */
    struct zxdh_pci_vq_info **vqs ____cacheline_aligned_in_smp;

    struct send_queue *sq;
    struct receive_queue *rq;
    uint32_t status;

    /* Max # of queue pairs supported by the device */
    uint16_t curr_queue_pairs;
    uint16_t max_queue_pairs;   /* max_vq_pairs + msg_qpairs */
    uint16_t max_vq_pairs;
#ifndef CGS_V5_693
    uint16_t xdp_queue_pairs;
    bool xdp_enabled;
    bool mergeable_rx_bufs;
#endif
    uint16_t old_queue_pairs; /* for selq flow_map attrbuite group */

    enum zxdh_device_state device_state;
    /* Host can handle any s/g split between our header and packet data */
    bool any_header_sg;
    /* Packet custom queue header size */
    uint8_t hdr_len;
    uint8_t hdr_1588_len;
    uint8_t hdr_len_rx_split;
    /* Work struct for refilling if we run low on memory. */
    struct delayed_work refill;

    /* CPU hotplug instances for online & dead */
    struct hlist_node node;
    struct hlist_node node_dead;
    bool fast_unload;
    bool vqmb_port_ctl;

    bool dtp_drs_offload;

    uint32_t phy_index[ZXDH_MAX_QUEUES_NUM];
    uint32_t logic_index_split[ZXDH_MAX_QUEUES_NUM];

    uint8_t link_check_bit;
    uint8_t pannel_id;
    uint8_t rsv[2];

    uint16_t ep_bdf;
    uint64_t spec_sbdf;    /* special bdf， 用于rdma持久化配置文件路径创建 */
    uint16_t pcie_id;
    /* vfunc_active */
    uint16_t slot_id;
    uint16_t vport;
    uint8_t phy_port;
    uint8_t panel_id;
    uint8_t hash_search_idx;

    uint32_t link_speed;
    bool link_up;
    uint8_t duplex;

    uint32_t speed;
    uint32_t curr_speed_modes;
    uint32_t autoneg_enable;
    uint32_t supported_speed_modes;
    uint32_t advertising_speed_modes;

    bool promisc_enabled;
    bool allmulti_enabled;
    uint32_t pflags;
    uint8_t clock_no;
    uint32_t msglevel;
    uint32_t wol_support;
    uint32_t wolopts;
    uint32_t vf_1588_call_np_num;
    uint32_t ptp_tc_enable_opt;
    uint32_t delay_statistics_enable;

    uint8_t vendor[VENDOR_SIZE];
    uint8_t fw_version[FW_VERSION_LEN];
    struct zxdh_flow_steering fs ____cacheline_aligned_in_smp;

    struct work_struct vf_link_info_update_work;
    struct work_struct link_info_irq_update_vf_work;
    struct work_struct link_info_irq_process_work;
    struct work_struct link_info_irq_update_np_work;
    struct work_struct rx_mode_set_work;
    struct work_struct plug_adev_work;
    struct work_struct unplug_adev_work;
    struct work_struct smart_nic_copy_work;
    struct work_struct vf_ro_info_update_work;

    uint8_t curr_unicast_num;
    uint8_t curr_multicast_num;
    struct work_struct pf_notify_vf_link_state_work;
    struct work_struct pf2vf_msg_proc_work;
    struct work_struct service_task;
    struct work_struct service_riscv_task;
    struct timer_list service_timer;
    struct timer_list service_riscv_timer;
    struct timer_list service_stat_timer;
    struct timer_list service_eeprom_timer;
    struct workqueue_struct *stats_wq;
    struct delayed_work stats_work;
    struct workqueue_struct *eeprom_wq;
    struct work_struct eeprom_work;
    struct work_struct riscv2aux_msg_proc_work;
    struct work_struct capture_save_file_work;
    /* QoS DCB */
    struct zxdh_dcbnl_para dcb_para;
    struct zxdh_dcbnl_ets_switch_info ets_info;
    bool rdma_ets_flag;
    uint32_t trust;

    /* SEC */
    zxdh_sec_pri drs_sec_pri;
    struct zxdh_sec_info *sec_info;
    uint32_t sec_phy_index[256];
    resource_size_t notify_phy_addr;

#ifdef ZXDH_CONFIG_SPECIAL_SQ_EN
    DECLARE_HASHTABLE(flow_map_hash, ilog2(ZXDH_MAX_PAIRS_NUM));
#endif
    /* initialization completion flag */
    uint8_t init_comp_flag;

    struct notifier_block ipv6_notifier;
    struct notifier_block vxlan_notifier;
    struct notifier_block vlan_notifier;

    /* just for hardware bond */
    bool is_hwbond;
    bool is_primary_port;
    bool is_rdma_aux_plug;
    uint64_t last_tx_vport_ssvpc_packets;
    /* link-down-on-close */
    bool link_down_on_close;
    bool enable_1588;
#ifdef CONFIG_INET
    bool local_lb_enable;
#endif
    unsigned long state;
    uint8_t pkt_dev_flag;
    uint8_t pkt_cap_switch;
    uint8_t pkt_save_file_flag;
    uint8_t pkt_file_num;
    uint8_t pkt_addr_marked;
    uint32_t pkt_dev_speed;
    struct zxdh_pkt_file_info *pkt_file_info;
    struct zxdh_pkt_save_file pkt_save_file;
    struct workqueue_struct *pkt_wq;

    netdev_features_t features;
    struct en_device_config eth_config;
    struct sockaddr last_np_mac_addr;

    uint32_t board_type;
    bool is_multi_ep;
    bool ro_flag;
    bool quick_remove;
    bool is_special_bond;
    bool time_sync_done;

    bool is_lowlatency;
    bool is_outer_l4_rxcsum_offload;
    bool packed_status;
    struct zxdh_vlan_to_mac_list vuc;
    struct list_head dev_node;
    struct zxdh_lag_manager *ldev_manager;
    struct sockaddr last_mac_addr;  //标卡硬bond使用
    uint16_t bond_link_info; //标卡硬bond使用：共16bit，bit0~15对应phyport 0~15的link状态
    uint8_t ldev_idx;
    struct zxdh_lag_dev *ldev;
    uint32_t                  roce_cycle_speed;
    struct zxdh_netlink_module_info netlink_module_info;
    enum zxdh_allmulti_conf   user_allmulti_cfg;
};

struct zxdh_en_priv {
    struct zxdh_en_device edev;
    struct mutex lock;
    struct dh_eq_table eq_table;
    struct dh_events *events;
#ifdef HAVE_ETHTOOL_COALESCE_PARAMS_SUPPORT
    struct ethtool_ops ethtool_ops_adjusted;
#endif
};

typedef struct {
    uint8_t mac_addr[ETH_ALEN];
} MacAddress;

typedef struct {
    uint32_t mac_num;
    uint32_t target_vf;
    union
    {
        uint32_t unicast_add_count;
        uint32_t unicast_del_count;
    };
    union
    {
        uint32_t multicast_add_count;
        uint32_t multicast_del_count;
    };
    MacAddress unicast_mac_array[128]; /* 用于存储多个单播MAC地址的链表 */
    MacAddress multicast_mac_array[32];  /* 用于存储多个组播MAC地址的链表 */
} mac_config_info;

typedef struct {
    uint32_t src_vf;   /* 被转移的vf */
    uint32_t dst_vf;   /* 转移的vf*/
} mac_transfer_info ;

struct dhtool_set_vf_mac_msg
{
    enum { MAC_ADD, MAC_DEL, MAC_TRANSFER } action;  /* 配置mac动作 */
    union
    {
        mac_transfer_info mac_transfer;
        mac_config_info mac_config;
    };
};

typedef enum
{
    MAC_CONFIG_SUCCESS = 0,              /* 配置mac成功 */
    MAC_CONFIG_FAILED = 1,               /* 配置mac失败*/
    MAC_ALREADY_EXISTS_IN_OTHER_VF = 2,  /* mac已经存在 */
    UNICAST_MAC_NUM_BEYOND_MAXNUM = 3,   /* 单播mac数量超过上限*/
    MULTICAST_MAC_NUM_BEYOND_MAXNUM = 4, /* 组播mac数量超过上限*/
    UNICAST_MAC_NOT_EXISTS = 5,          /* 单播mac不存在 */
    MULTICAST_MAC_NOT_EXISTS = 6,        /* 组播mac不存在 */
    UNICAST_MAC_TRANSFER_FAILED = 7,     /* 单播迁移异常  */
    MULTICAST_MAC_TRANSFER_FAILED = 8,   /* 组播迁移异常  */
    VF_ERROR = 9,                        /* vf异常 */
}VF_MAC_SET_RET;

#ifndef CGS_V5_693
struct padded_zxdh_net_hdr {
    struct zxdh_net_hdr_tx hdr;
    /*
     * hdr is in a separate sg buffer, and data sg buffer shares same page
     * with this header sg. This padding makes next sg 16 byte aligned
     * after the header.
     */
    char padding[4];
};
#endif

#define DEV_UNICAST_MAX_NUM     VF_MAX_UNICAST_MAC      /* 每个PF/VF存储的单播mac转发表上限 */
#define DEV_MULTICAST_MAX_NUM   VF_MAX_MULTICAST_MAC    /* 每个PF/VF存储的组播mac转发表上限 */
#define UNICAST_MAX_NUM         (16 * 257)
#define MULTICAST_MAX_NUM       (4 * 257)

#define EXTRACT_BUS(bdf) ((bdf >> 8) & 0xff)    /* 从BDF号中提取bus */
#define EXTRACT_DEVICE(bdf) ((bdf >> 3) & 0x1f)  /* 从BDF号中提取device */
#define DEVICE_RANGE  31 /* 每个bus下面可以挂载的vf设备数量 */

typedef struct mac_queue {
    uint8_t addr[DEV_MULTICAST_MAX_NUM][ETH_ALEN];
    uint8_t count;
} mac_queue;

int32_t dh_aux_eq_table_init(struct zxdh_en_priv *en_priv);
void dh_aux_eq_table_cleanup(struct zxdh_en_priv *en_priv);
int32_t zxdh_ip6mac_add(struct zxdh_en_device *en_dev, const uint32_t *addr6, const uint8_t *ip6mac);
int32_t zxdh_ip6mac_del(struct zxdh_en_device *en_dev, const uint32_t *addr6, const uint8_t *ip6mac);
int32_t zxdh_ip6mac_del_safe(struct zxdh_en_device *en_dev, const uint32_t *addr6, const uint8_t *ip6mac);
void zxdh_ip6mac_del_work_handler(struct work_struct *work);
int32_t zxdh_ip6mac_add_safe(struct zxdh_en_device *en_dev, const uint32_t *addr6, const uint8_t *ip6mac);
void zxdh_ip6mac_add_work_handler(struct work_struct *work);
int32_t zxdh_ip4mac_add(struct zxdh_en_device *en_dev, const uint8_t *ip4mac, uint8_t action);
int32_t zxdh_ip4mac_del(struct zxdh_en_device *en_dev, const uint8_t *ip4mac, uint8_t action);
int32_t zxdh_vlan_mac_add(struct zxdh_en_device *en_dev, uint16_t vlan_id, const uint8_t *vlan_mac);
int32_t zxdh_vlan_mac_del(struct zxdh_en_device *en_dev, uint16_t vlan_id, const uint8_t *vlan_mac);
#ifdef CGS_V5_693
u16 zxdh_en_select_queue(struct net_device *dev, struct sk_buff *skb, void *accel_priv, select_queue_fallback_t fallback);
#else
#ifdef CTYUNOS_4_19_5_10
u16 zxdh_en_select_queue(struct net_device *netdev, struct sk_buff *skb, struct net_device *sb_dev);
#else
#ifdef HAVE_NDO_SELECT_QUEUE_FALLBACK_REMOVED
uint16_t zxdh_en_select_queue(struct net_device *netdev, struct sk_buff *skb, struct net_device *sb_dev);
#else
uint16_t zxdh_en_select_queue(struct net_device *netdev, struct sk_buff *skb, struct net_device *sb_dev, select_queue_fallback_t fallback);
#endif
#endif
#endif
void zxdh_flow_map_cleanup(struct zxdh_en_priv *en_priv);
int32_t zxdh_flow_map_init(struct zxdh_en_priv *en_priv);
int32_t zxdh_flow_map_update_sysfs(struct net_device *netdev, int32_t pre_txq_num);
void zxdh_netdev_addr_set(struct net_device *dev, const u8 *addr);
extern void zxdh_netdev_features_over_dtp(struct net_device *netdev);
int32_t set_feature_rxhash(struct zxdh_en_device *en_dev, bool enable);
int32_t set_feature_ntuple(struct zxdh_en_device *en_dev, bool enable);
int32_t set_feature_tc(struct zxdh_en_device *en_dev, bool enable);
int32_t zxdh_pf_add_vf_unicast_mac(struct zxdh_en_device *en_dev, struct dhtool_set_vf_mac_msg *msg);
int32_t zxdh_pf_del_vf_unicast_mac(struct zxdh_en_device *en_dev, struct dhtool_set_vf_mac_msg *msg);
int32_t zxdh_pf_add_vf_multicast_mac(struct zxdh_en_device *en_dev, struct dhtool_set_vf_mac_msg *msg);
int32_t zxdh_pf_del_vf_multicast_mac(struct zxdh_en_device *en_dev, struct dhtool_set_vf_mac_msg *msg);
int32_t zxdh_pf_transfer_vf_mac(struct zxdh_en_device *en_dev, uint32_t src_vf, uint32_t dst_vf);
bool zxdh_old_vlan_mac_get(struct zxdh_en_device *en_dev, uint8_t *old_vlan_mac, uint16_t vlan_id);
int32_t zxdh_pflags_update(struct net_device *netdev, uint8_t flag, bool enable);
int32_t zxdh_port_enable(struct zxdh_en_device *en_dev, bool enable);
int32_t zxdh_en_sync_features(struct zxdh_en_device *en_dev, netdev_features_t want_features);
int32_t zxdh_en_config_mtu_to_np(struct net_device *netdev, int32_t mtu_value);
int32_t zxdh_vlan_trunk_recover(DPP_PF_INFO_T *pf_info, uint8_t *vlan_trunk_bitmap);
void zxdh_napi_close(struct zxdh_en_priv *en_priv);
void cleanup_rx_queues(struct zxdh_en_device *en_dev, int32_t max_index);
struct zxdh_en_device *dh_get_next_phys_dev(struct zxdh_en_device *en_dev);
void zxdh_update_rdma_hwbond_master(void);
int32_t zxdh_get_vlan_info(void *in_para, void *out_para);

#define ZXDH_DEVICE_STATE_CHECK_RTN(en_dev) \
do { \
    if (en_dev->device_state == ZXDH_DEVICE_STATE_INTERNAL_ERROR) \
        return -ENXIO; \
} while(0)

struct zxdh_rdma_if {
    void *(*get_rdma_netdev)(struct dh_core_dev *dh_dev);
};

struct zxdh_sec_if{
    void *(*get_sec_info)(struct dh_core_dev *dh_dev);
};

struct zxdh_en_if {
    uint16_t (*get_channels_num)(struct dh_core_dev *dh_dev);
    int32_t (*create_vqs_channels)(struct dh_core_dev *dh_dev, void *data);
    void (*destroy_vqs_channels)(struct dh_core_dev *dh_dev);
    void (*switch_vqs_channel)(struct dh_core_dev *dh_dev, int32_t channel, int32_t op);
    int32_t (*vqs_channel_bind_handler)(struct dh_core_dev *dh_dev, int32_t vqs_channel_num, struct dh_vq_handler *handler);
    void (*vqs_channel_unbind_handler)(struct dh_core_dev *dh_dev, int32_t vqs_channel_num);
    int32_t (*vq_bind_channel)(struct dh_core_dev *dh_dev, uint16_t channel_num, uint16_t phy_index, uint16_t index, uint16_t vq_idx);
    void (*vq_unbind_channel)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index);
    int32_t (*vqs_bind_eqs)(struct dh_core_dev *dh_dev, int32_t vqs_channel_num, struct list_head *vq_node);
    void (*vqs_unbind_eqs)(struct dh_core_dev *dh_dev, int32_t vqs_channel_num);
    void __iomem * (*vp_modern_map_vq_notify)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, resource_size_t *pa);
    void (*vp_modern_unmap_vq_notify)(struct dh_core_dev *dh_dev, void *priv);
    int32_t (*get_vq_lock)(struct dh_core_dev *dh_dev);
    int32_t (*find_valid_vqs)(struct dh_core_dev *dh_dev, uint16_t vqs_cnt, uint32_t vq_index[]);
    int32_t (*write_vqs_bit)(struct dh_core_dev *dh_dev, uint16_t vqs_cnt, uint32_t vq_index[]);
    int32_t (*write_queue_tlb)(struct dh_core_dev *dh_dev, uint16_t vqs_cnt, uint32_t vq_index[], bool need_msgq);
    uint16_t (*get_fw_patch)(struct dh_core_dev *dh_dev);
    int32_t (*release_vq_lock)(struct dh_core_dev *dh_dev);
    void (*activate_phy_vq)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, int32_t queue_size,
                            uint64_t desc_addr, uint64_t driver_addr, uint64_t device_addr);
    void (*de_activate_phy_vq)(struct dh_core_dev *dh_dev, uint32_t phy_index);
    void (*set_status)(struct dh_core_dev *dh_dev, uint8_t status);
    uint8_t (*get_status)(struct dh_core_dev *dh_dev);
    uint8_t (*get_cfg_gen)(struct dh_core_dev *dh_dev);
    bool (*get_rp_link_status)(struct dh_core_dev *dh_dev);
    void (*set_vf_mac)(struct dh_core_dev *dh_dev, uint8_t *mac, int32_t vf_id);
    void (*get_vf_mac)(struct dh_core_dev *dh_dev, uint8_t *mac, int32_t vf_id);
    void (*set_mac)(struct dh_core_dev *dh_dev, uint8_t *mac);
    void (*get_mac)(struct dh_core_dev *dh_dev, uint8_t *mac);
    uint64_t (*get_features)(struct dh_core_dev *dh_dev);
    void (*set_features)(struct dh_core_dev *dh_dev, uint64_t features);
    uint16_t (*get_queue_num)(struct dh_core_dev *dh_dev);
    uint16_t (*get_queue_size)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index);
    void (*set_queue_size)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, uint16_t size);
    void (*set_queue_enable)(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, bool enable);
    uint16_t (*get_epbdf)(struct dh_core_dev *dh_dev);
    uint64_t (*get_spec_sbdf)(struct dh_core_dev *dh_dev);
    bool (*is_multi_ep)(struct dh_core_dev *dh_dev);
    uint16_t (*get_vport)(struct dh_core_dev *dh_dev);
    uint16_t (*get_pcie_id)(struct dh_core_dev *dh_dev);
    uint16_t (*get_slot_id)(struct dh_core_dev *dh_dev);
    bool (*is_bond)(struct dh_core_dev *dh_dev);
    bool (*is_upf)(struct dh_core_dev *dh_dev);
    enum dh_coredev_type (*get_coredev_type)(struct dh_core_dev *dh_dev);
    struct pci_dev * (*get_pdev)(struct dh_core_dev *dh_dev);
    uint64_t (*get_bar_virt_addr)(struct dh_core_dev *dh_dev, uint8_t bar_num);
    uint64_t (*get_bar_phy_addr)(struct dh_core_dev *dh_dev, uint8_t bar_num);
    uint64_t (*get_bar_size)(struct dh_core_dev *dh_dev, uint8_t bar_num);
    int32_t (*msg_send_cmd)(struct dh_core_dev *dh_dev, uint16_t module_id, void *msg, void *ack, \
                            struct zxdh_bar_extra_para *para);
    int32_t (*async_eq_enable)(struct dh_core_dev *dh_dev, struct dh_eq_async *eq, const char *name, bool attach);
    void (*aux_nh_attach)(struct dh_core_dev *dh_dev, struct dh_nb *nb, bool attach);
    struct zxdh_vf_item *(*get_vf_item)(struct dh_core_dev *dh_dev, uint16_t vf_idx);
    void (*set_pf_link_up) (struct dh_core_dev *dh_dev, bool link_up);
    bool (*get_pf_link_up) (struct dh_core_dev *dh_dev);
    void (*update_pf_link_info)(struct dh_core_dev *dh_dev, struct link_info_struct *link_info_val);
    int32_t (*get_pf_drv_msg)(struct dh_core_dev *dh_dev, uint8_t *drv_version, uint8_t *drv_version_len);
    void (*set_vepa) (struct dh_core_dev *dh_dev, bool setting);
    bool (*get_vepa) (struct dh_core_dev *dh_dev);
    void (*set_bond_num)(struct dh_core_dev *dh_dev, bool add);
    bool (*if_init)(struct dh_core_dev *dh_dev);
    int32_t (*request_port)(struct dh_core_dev *dh_dev, void *data);
    int32_t (*release_port)(struct dh_core_dev *dh_dev, uint32_t port_id);
    void (*get_link_info_from_vqm)(struct dh_core_dev *dh_dev, uint8_t *link_up);
    bool (*set_vf_link_info)(struct dh_core_dev *dh_dev, uint16_t vf_idx, uint8_t link_up);
    uint8_t (*get_vf_link_info)(struct dh_core_dev *dh_dev, uint16_t vf_idx);
    bool (*get_vf_is_probe)(struct dh_core_dev *dh_dev, uint16_t vf_idx);
    void (*set_pf_phy_port)(struct dh_core_dev *dh_dev, uint8_t phy_port);
    void (*set_rdma_netdev)(struct dh_core_dev *dh_dev, void *data);
    uint8_t (*get_pf_phy_port)(struct dh_core_dev *dh_dev);
    void (*set_init_comp_flag)(struct dh_core_dev *dh_dev, uint8_t flag);
    struct zxdh_ipv6_mac_tbl * (*get_ip6mac_tbl)(struct dh_core_dev *dh_dev);
    struct device *(*get_dma_dev)(struct dh_core_dev *dh_dev);
    void (*unplug_adev)(struct dh_core_dev *dh_dev, enum AUX_DEVICE_TYPE adev_type);
    int32_t (*plug_adev)(struct dh_core_dev *dh_dev, enum AUX_DEVICE_TYPE adev_type);
    bool (*is_nic)(struct dh_core_dev *dh_dev);
    bool (*is_special_bond)(struct dh_core_dev *dh_dev);
    bool (*is_support_bond_config)(struct dh_core_dev *dh_dev);
    uint8_t (*get_qpairs)(struct dh_core_dev *dh_dev);
    int32_t (*eth_config_recover)(struct net_device *netdev);
    void (*eth_config_show)(struct net_device *netdev);
    int32_t (*events_call_chain)(struct dh_core_dev *dh_dev, unsigned long type, void *data);
    int32_t (*get_cpl_timeout_if_mask)(struct dh_core_dev *dh_dev);
    int32_t (*set_cpl_timeout_mask)(struct dh_core_dev *dh_dev, uint32_t mask);
    int32_t (*get_hp_irq_ctrl_status)(struct dh_core_dev *dh_dev);
    int32_t (*set_hp_irq_ctrl_status)(struct dh_core_dev *dh_dev, uint32_t status);
    uint32_t (*get_dev_type)(struct dh_core_dev *dh_dev);
    bool (*is_panel_port)(struct dh_core_dev *dh_dev);
    uint8_t (*get_panel_id)(struct dh_core_dev *dh_dev);
    bool (*if_suport_np_ext_stats)(struct dh_core_dev *dh_dev);
    struct zxdh_np_ext_stats* (*get_np_ext_stats)(struct dh_core_dev *dh_dev, uint8_t panel_id);
    void (*set_sec_info)(struct dh_core_dev *dh_dev, void *data);
    bool (*is_drs_sec_enable) (struct dh_core_dev *dh_dev);
    bool (*is_fw_feature_support) (struct dh_core_dev *dh_dev, uint32_t feature);
    bool (*is_pf_rate_enable)(struct dh_core_dev *dh_dev, uint32_t *pf_fc_val);
    uint16_t (*get_ovs_pf_vfid) (struct dh_core_dev *dh_dev);
    uint8_t (*get_board_type)(struct dh_core_dev *dh_dev);
    bool (*is_hwbond) (struct dh_core_dev *dh_dev, bool is_hwbond, bool update_pf);
    bool (*is_rdma_aux_plug) (struct dh_core_dev *dh_dev, bool is_rdma_aux_plug, bool update_pf);
    bool (*is_primary_port) (struct dh_core_dev *dh_dev, bool is_primary_port, bool update_pf);
    void (*optim_hardware_bond_time) (struct dh_core_dev *dh_dev, bool enable);
    void (*update_mp_enable) (struct dh_core_dev *dh_dev, bool enable);
    void (*get_psn_feature_info) (struct dh_core_dev *dh_dev, uint8_t *psn_version, bool *dual_tor, bool *quad_tor);
    int32_t (*update_hb_file_val) (struct dh_core_dev *dh_dev, uint64_t spec_sbdf, const char *file_name, bool flag);
    bool (*is_rdma_enable) (struct dh_core_dev *dh_dev);
    bool (*get_ro_info_from_fwshrd)(struct dh_core_dev *dh_dev);
    uint32_t (*get_bond_config_from_fwshrd)(struct dh_core_dev *dh_dev);
    bool (*update_active_bond_config_to_fwshrd)(struct dh_core_dev *dh_dev, uint32_t active_bond_info);
    bool (*is_lowlatency)(struct dh_core_dev *dh_dev);
    void (*set_bond_link_info)(struct dh_core_dev *dh_dev, uint16_t bond_link_info);
    void (*set_bond_slave_flag)(struct dh_core_dev *dh_dev, bool is_bond_slave);
    uint64_t (*get_mcode_feature)(struct dh_core_dev *dh_dev);
    uint8_t (*get_bond_port_type)(struct dh_core_dev *dh_dev);
    uint8_t (*get_no_bondpf_panel_num)(struct dh_core_dev *dh_dev);
    void (*set_rdma_speed)(struct dh_core_dev *dh_dev, uint32_t speed);
    void *(*get_dev_cap)(struct dh_core_dev *dh_dev);
    uint16_t (*get_pre_bond_qidx)(struct dh_core_dev *dh_dev);
    void (*set_pre_bond_qidx)(struct dh_core_dev *dh_dev, uint16_t pre_bond_qidx);
    void (*get_fw_version)(struct dh_core_dev *dh_dev, uint8_t *fw_version);
    void (*get_vendor)(struct dh_core_dev *dh_dev, uint8_t *vendor);
    bool (*get_split_packed)(struct dh_core_dev *dh_dev);
};

#ifdef __cplusplus
}
#endif

#endif

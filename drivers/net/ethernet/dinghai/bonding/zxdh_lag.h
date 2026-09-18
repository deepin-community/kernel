#ifndef _ZXDH_HARDWARE_BOND_H_
#define _ZXDH_HARDWARE_BOND_H_
#include <linux/dinghai/lag.h>
#include "../en_aux.h"

struct zxdh_bond_group;

#define LAG_ENABLE_DATA_IDX 0  //对应端口配置表bit[127:96]
#define LAG_ID_DATA_GAP 3  //端口配置表中lag_id占用3个bit位
#define LAG_ID_DATA_IDX 1  //对应端口配置表bit[95:64]
#define RDMA_START_PORT_IN_MCODE 16 //微码中分配给rdma的端口范围为16-23
#define RDMA_END_PORT_IN_MCODE 23
#define PORT_NUM_IN_MCODE (10)

#define ZXDH_SPECIAL_LGA_ID 0

#define MAX_MAC_ENTRIES 10
#define DH_MAX_PORTS 10
#define DH_MAX_LAG 2

struct mac_entry 
{
    uint8_t mac[ETH_ALEN];  // MAC地址
    int refcount;      // 引用计数
};

struct mac_table 
{
    struct mac_entry entries[MAX_MAC_ENTRIES]; // 固定大小数组
    int count;         // 当前有效条目数
};

struct upper_info_struct
{
    struct net_device *upper_dev;
    struct netdev_lag_upper_info lag_upper_info;
};

struct event_ctx;

struct dh_dev_func {
	struct zxdh_en_device *en_dev;
	struct net_device    *netdev; /* this PF's netdev */
    bool is_active;
};

struct slave_state {
    struct dh_dev_func slave_info;
    struct netdev_lag_lower_state_info lower_state;
    bool slave_link_change;
    bool slave_mac_change;
};

enum zxdh_bond_type 
{
    SPECIAL_BOND,   // I511类型的bond（旧的bond_pf）
    HARDWARE_BOND,  // 标卡的硬bond
    SOFTWARE_BOND,   // 软bond
};

struct zxdh_lag_tracker {
    enum zxdh_netdev_lag_tx_type tx_type;
    enum zxdh_netdev_lag_hash hash_type;
	struct slave_state slaves[DH_MAX_PORTS];
    enum zxdh_bond_type bond_type;
    enum zxdh_bond_type target_bond_type;
    bool is_bonded;
};

// 全局上下文
struct event_ctx {
    struct delayed_work bond_work;
    spinlock_t lock;             // 保护队列的锁
    struct list_head event_list; // 事件链表头（FIFO队列）
    uint32_t idx;
};

/* LAG data of a ConnectX card.
 * It serves both its phys functions.
 */
struct zxdh_lag_dev {
    uint8_t                   idx; // 在lag_mgr数组中的索引
    struct zxdh_lag_tracker   tracker;
	struct workqueue_struct   *wq;
	struct notifier_block     nb;
    struct event_ctx ctx;
    uint16_t cur_slaves;     // 当前的slave配置
    uint16_t expect_slaves;  // 期望的slave配置，比如 0000 0000 0000 0011 表示需要用到panel_0和panel_1
    uint16_t primary_pf_idx; // primary_pf的panel_id
    struct mutex mlock;
    struct net_device *upper_netdev;
    char upper_name[IFNAMSIZ];
    enum {
        LAG_DEV_IDLE,    // 空闲，可关联新bond
        LAG_DEV_ACTIVE,  // 已关联bond，正在使用
        LAG_DEV_INVALID  // 无效，不可用
    } state;
    bool is_active;
    uint32_t bond_speed; // 当前bond的总带宽
    int32_t group_ida; // 下表到np中的lag_id
};

struct zxdh_lag_manager {
	struct dh_dev_func        pf[DH_MAX_PORTS];
	struct zxdh_lag_dev       ldev[DH_MAX_LAG];
    uint32_t cur_bond_config;
    struct mutex mlock;
};

// 每个事件对应一个独立的节点
struct event_node {
    struct list_head list;       // 内核链表节点
    unsigned long event;    // 事件类型（如NETDEV_XXX)
    struct zxdh_lag_tracker tracker;
    uint32_t idx;
    int16_t event_slave_id;  //触发本次bond事件的slave的panel_id
    struct net_device *event_netdev; //触发本次bond事件的slave的netdev
    bool linking;  // 加入或离开bond事件
    bool from_recover;
};

struct zxdh_bond_group
{
    char name[IFNAMSIZ];

    int32_t group_ida;

    uint8_t lag_tx_type;        /* enum zxdh_netdev_lag_tx_type */
    uint8_t hash_policy;
    uint8_t num_slaves;

    bool configured;

    struct list_head node;
};

static inline bool zxdh_netdev_is_hwbond(const struct net_device *netdev)
{
    return (&((struct zxdh_en_priv *)netdev_priv(netdev))->edev)->is_hwbond;
}

static inline bool zxdh_netdev_is_special_bond(const struct net_device *netdev)
{
    return (&((struct zxdh_en_priv *)netdev_priv(netdev))->edev)->is_special_bond;
}

/* Check if any standby port has VF enabled (common function) */
bool zxdh_lag_has_any_standby_vf_enabled(struct zxdh_lag_tracker *tracker, uint8_t primary_panel_id);

void zxdh_lag_lock_init(void);
void zxdh_lag_lock_deinit(void);

void zxdh_update_hw_bond_panel_state(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct zxdh_en_device *en_dev, bool use_en_dev_linkup);
void zxdh_bond_set_dpp_member_port(struct zxdh_en_device *en_dev, bool enable, struct event_node *node, int32_t group_ida);
void zxdh_lag_vf_enable_to_sw_bond(struct zxdh_pf_device *pf_dev);
void zxdh_lag_vf_disable_to_hw_bond(struct zxdh_pf_device *pf_dev);

/* Structure to hold bond slave statistics */
struct zxdh_bond_slave_stats {
    uint16_t same_ldev_slaves;   /* DH card slave count */
    uint16_t hardbond_slaves;    /* Hardware-bond enabled slave count */
    uint16_t total_slaves;       /* Total slave count in kernel bond */
    uint16_t specbond_slaves;    /* Special-bond slave count */
    uint16_t cur_slaves;         /* Current slave configuration (bitmask) */
};

/* Function to count slave statistics from bond device */
void zxdh_lag_count_bond_slaves(struct zxdh_lag_dev *ldev,
                                 struct net_device *upper_dev,
                                 struct zxdh_bond_slave_stats *stats);
int zxdh_lag_manager_add(struct zxdh_en_device *dev);
void zxdh_lag_manager_remove(struct zxdh_en_device *en_dev);
void zxdh_hardware_bond_primary_update(struct net_device *netdev);
int32_t zxdh_bond_cofig_rdma_speed(struct zxdh_lag_dev *cur_lag);
int32_t zxdh_lag_bond_lacp_dpp_init(struct zxdh_en_device *en_dev);
int32_t zxdh_lag_change_to_software_bond(struct zxdh_lag_dev *curr_lag, struct zxdh_lag_tracker *tracker, struct zxdh_en_device *en_dev);
int32_t zxdh_lag_change_to_hardware_bond(struct zxdh_lag_dev *curr_lag, struct zxdh_lag_tracker *tracker, struct zxdh_en_device *en_dev);
int32_t zxdh_lag_init_special_bond_slave_dpp(struct zxdh_lag_tracker *tracker, struct zxdh_lag_dev *cur_lag, struct slave_state *cur_slave);
void zxdh_lag_init_special_bond_dpp(struct zxdh_lag_tracker *tracker, struct slave_state *cur_slave, int32_t group_ida);
#endif  /* END _ZXDH_HARDWARE_BOND_H_ */
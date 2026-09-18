#ifndef _ZXDH_DUAL_TOR_H
#define _ZXDH_DUAL_TOR_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/rhashtable.h>
#include <linux/timer.h>
#include <linux/mutex.h>
#include <net/sock.h>
#include <net/genetlink.h>
#include <linux/dinghai/log.h>
#include "slib.h"

#define MODULE_PSN    "MODULE_PSN"

/* Started by AICoder, pid:6801as7a4cga37e145ad0979a0ddd30e83d8eefa */
#define PSN_LOG_ERR(fmt, arg...)    DH_LOG_ERR(MODULE_PSN, fmt, ##arg);
#define PSN_LOG_INFO(fmt, arg...)   DH_LOG_INFO(MODULE_PSN, fmt, ##arg);
#define PSN_LOG_DEBUG(fmt, arg...)  DH_LOG_DEBUG(MODULE_PSN, fmt, ##arg);

#define PSN_LOG_ERR_DEV(dev, fmt, arg...)    DH_LOG_ERR_DEV(MODULE_PSN, dev, fmt, ##arg);
#define PSN_LOG_INFO_DEV(dev, fmt, arg...)   DH_LOG_INFO_DEV(MODULE_PSN, dev, fmt, ##arg);
#define PSN_LOG_DEBUG_DEV(dev, fmt, arg...)  DH_LOG_DEBUG_DEV(MODULE_PSN, dev, fmt, ##arg);
/* Ended by AICoder, pid:6801as7a4cga37e145ad0979a0ddd30e83d8eefa */

/**************通用log宏定义****************/
#define    BOND_STATE_DOWN           (0)
#define    BOND_STATE_UP             (1)

#define    IP_ADDR_STR_MAX_LENGTH    (48)
#define    IFNAME_MAX_SIZE           (32)
#define    KERNEL_MSGQ_MAX_LEN       (256)
#define    HEARTBEAT_THRESHOLD       (10)

#define    SLAVE_TWO                 (2)
#define    SLAVE_MUTIL               (4)

#define    PF_FID_GET(fid) ((fid) = ((fid) | 0x800) & 0xffffff00)

#define    BOND_SLAVE_MAX_NUMS       (10)

struct ip_info
{
    uint8_t ip[IP_ADDR_STR_MAX_LENGTH];
    uint8_t src_ip[IP_ADDR_STR_MAX_LENGTH];
};

struct ip_rhash_tbl_value_st
{
    uint8_t src_mac[6];
    uint8_t src_mac_rsv[2];
    uint8_t dst_mac[6];
    uint8_t dst_mac_rsv[2];
    uint32_t type; //建链设备类型
};

enum ip_event
{
    PATH_STATE_CHANGED,
    ACK_RECV,
    ACK_TIMEOUT,
    IP_DELETE,
    IP_ADD,
};

enum ip_state {
    STATE_NEW,
    STATE_NOTIFY,
    STATE_DONE,
    STATE_DELETING,
    STATE_DELETED,
    STATE_ERROR,
};

enum slave_enable {
    SLAVE_DISABLE = 0,
    SLAVE_ENABLE = 1,
};
struct remote_ip_hash_item
{
    struct ip_info ip_pair; /*ip 条目的键*/
    int rsv;   // 关联的值
    struct rhash_head node;
    enum ip_state state;
    enum ip_state wanted_state;
    struct ip_rhash_tbl_value_st mac_pair;
    struct timer_list timeout_timer; //定时器用于处理超时
};

struct slave_dev
{
    uint8_t link_state;
    uint8_t np_port;
    uint8_t is_update;
    uint8_t is_enable;
    uint32_t fid;
};

struct bond_ports_info
{
    uint8_t slaves_num;
    uint8_t rsv1;
    uint16_t rsv2;
    struct slave_dev slave_devs[BOND_SLAVE_MAX_NUMS];
};
struct bond_dev_info
{
    struct bond_ports_info ports_desc;
    struct rhashtable remote_ip_hash_list;    // 存储 IP 和值的哈希表
    struct rhashtable_params ip_hash_params;  // 哈希表参数
    struct mutex remote_ip_lock;
};

/* 自定义hash表表项*/
struct bond_dev_hash_item
{
    uint8_t ifname[IFNAME_MAX_SIZE];
    struct rhash_head node;
    struct bond_dev_info dev_info;
};
struct bond_devs_rht_st
{
    struct rhashtable bond_devs_rht;
    struct rhashtable_params bond_devs_rht_params;
    struct mutex devs_lock;
};

/**********用户态内核态消息通信模块**********/
#define VALID_REPS_FLAG_V2      (0xaa)
#define VALID_REPS_FLAG_V4      (0x55)

enum {
    NETLINK_CMD_GET_BOND_STATE        = 1,  /* 获取bond状态*/
    NETLINK_CMD_UPDATE_BOND_STATE     = 2,  /* 模拟更新bond状态*/
    NETLINK_CMD_USER_RECV_MSG         = 6,  /* 用户态接收消息cmd*/
    NETLINK_CMD_APP_EXIT              = 7,  /* 用户态app退出通知消息*/
    NETLINK_CMD_CREATE_NEW_BOND       = 8,  /* 模拟创建新bond设备信息*/
    NETLINK_CMD_ADD_REMOTE_IP         = 9,  /* 模拟rdma socket建链消息*/
    NETLINK_CMD_DEL_REMOTE_IP         = 10, /* 模拟rdma socket删链消息*/
    NETLINK_CMD_REPLY_ACK             = 11, /* 用户态回复ack消息*/
    NETLINK_CMD_INIT_MSG              = 12, /* 用户态app初始化消息*/
    NETLINK_CMD_LISTENING_NOTICE      = 15, /* 内核创建bond通知监听消息*/
    NETLINK_CMD_SRC_IP_NOTICE         = 16, /* 源ip过滤消息*/
    NETLINK_CMD_MAX,
};

enum {
    RECV_CMD_RSV0        = 0,
    RECV_CMD_RDMA_ADD_IP = 1,
    RECV_CMD_RDMA_DEL_IP = 2,
    RECV_CMD_UPDATE_BOND = 3,
    RECV_CMD_CREATE_BOND = 4,
    RECV_CMD_DELETE_BOND = 5,
    RECV_CMD_STOP_ALL_LISTEN = 6,
    RECV_CMD_RSV7,
    RECV_CMD_RSV8,
    RECV_CMD_RSV9,
    RECV_CMD_RSV10,
    RECV_CMD_RSV11,
    RECV_CMD_RSV12,
    RECV_CMD_RSV13,
    RECV_CMD_RSV14,
    RECV_CMD_LISTENDING_NOTICE = 15,
    RECV_CMD_MAX,
};
typedef struct __attribute__((packed)) normal_msg_entity_v2
{
    uint8_t bond1_state : 1;
    uint8_t bond2_state : 1;
    uint8_t slave1_is_update : 1;
    uint8_t slave2_is_update : 1;
    uint8_t rsv4_bit : 4;
    uint8_t slave1_port : 4;
    uint8_t slave2_port : 4;
/* 单次psn报文需要依赖的信息*/
    char ifname[IFNAME_MAX_SIZE];
    uint32_t slave1_fid;
    uint32_t slave2_fid;
    struct ip_info ip_pair;
    struct ip_rhash_tbl_value_st mac_pair;
}msg_entity_t_v2;

struct bond_slave_stat
{
    uint8_t link_state : 1;
    uint8_t is_update : 1;
    uint8_t rscv2_bit : 2;
    uint8_t np_port : 4;
    uint8_t is_enable;
    uint16_t rsv;
    uint32_t fid;
};/* 8字节*/

/**********************多个bond模式消息*************************/
struct psn_bond_event_msg
{
    char ifname[IFNAME_MAX_SIZE];
    uint8_t slave_nums;      /* 成员口数量*/
    uint8_t rscv1;
    uint16_t rscv2;
    struct bond_slave_stat slave_stats[BOND_SLAVE_MAX_NUMS];
};

struct psn_rdma_event_msg
{
    struct ip_info ip_pair;
    struct ip_rhash_tbl_value_st mac_pair;
};
typedef struct __attribute__((packed)) normal_msg_entity_v4
{
    uint8_t slot_id;
    uint8_t rsv1;
    uint16_t rsv2;
    union
    {
        struct psn_bond_event_msg bond_event;
        struct psn_rdma_event_msg rdma_event;
    };
}msg_entity_t_v4;

typedef struct __attribute__((packed)) normal_msg_entity_header
{
    uint8_t recv_state;      /* 消息状态 */
    uint8_t cmd;             /* 消息cmd */
} msg_entity_header_t;

/* 内核态和用户态通信结构体*/
typedef struct __attribute__((packed)) normal_msg_entity
{
    msg_entity_header_t header;
    union{
        msg_entity_t_v2 v2;
        msg_entity_t_v4 v4;
    };
} msg_entity_t;

_Static_assert(sizeof(msg_entity_t_v2) == (sizeof(msg_entity_t) - sizeof(msg_entity_header_t)),
               "msg_entity_t size must equal msg_entity_t_v2");


int bond_dev_create_remove_event(char *ifname, struct bond_ports_info *bond_info, uint8_t mode);
int bond_dev_update_event(char *ifname, struct bond_ports_info *bond_info);

int psn_init(void);
void psn_exit(void);



#define MAXLEN                         (2048)
#define NLA_DATA(na)                   ((void *)((char *)(na) + NLA_HDRLEN))
#define MPF_GENRIC_NTLINK_NAME         "bar_msg_family"
/* 属性类型*/
enum
{
    BAR_A_UNSPEC,
    BAR_A_MSG,
    __BAR_A_MAX,
};
#define BAR_A_MAX (__BAR_A_MAX - 1)

/* 操作码*/
enum
{
    BAR_C_UNSPEC,
    BAR_C_ECHO,
    __BAR_C_ECHO,
};
#define BAR_C_MAX (__BAR_C_MAX - 1)

#endif  /* END _ZXDH_HARDWARE_BOND_H_ */
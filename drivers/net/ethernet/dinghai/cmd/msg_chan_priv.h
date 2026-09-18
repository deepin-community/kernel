#ifndef _ZXDH_MSG_CHAN_PRIV_H_
#define _ZXDH_MSG_CHAN_PRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/pci.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/dinghai/kcompat.h>
#include <linux/dinghai/log.h>
/* <bar通道宏定义>*/
#define BAR_KFREE_PTR(ptr)   { \
    if (ptr != NULL)           \
    {                          \
        kfree(ptr);            \
    }                          \
    ptr = NULL;                \
}

#define BAR_LOG_ERR(fmt, arg...)    DH_LOG_ERR(MODULE_CMD, fmt, ##arg);
#define BAR_LOG_INFO(fmt, arg...)   DH_LOG_INFO(MODULE_CMD, fmt, ##arg);
#define BAR_LOG_DEBUG(fmt, arg...)  DH_LOG_DEBUG(MODULE_CMD, fmt, ##arg);
#define BAR_LOG_WARN(fmt, arg...)   printk(KERN_WARNING "[%s][%d] "fmt"", MODULE_CMD, __LINE__, ##arg);

#define BAR_LOG_ERR_DEV(dev, fmt, arg...)    DH_LOG_ERR_DEV(MODULE_CMD, dev, fmt, ##arg);
#define BAR_LOG_INFO_DEV(dev, fmt, arg...)   DH_LOG_INFO_DEV(MODULE_CMD, dev, fmt, ##arg);
#define BAR_LOG_DEBUG_DEV(dev, fmt, arg...)  DH_LOG_DEBUG_DEV(MODULE_CMD, dev, fmt, ##arg);
#define BAR_LOG_WARN_DEV(dev, fmt, arg...)   DH_LOG_WARNING_DEV(MODULE_CMD, dev, fmt, ##arg);

#define HOST_OR_ZX                   0

#define MAX_MSG_BUFF_NUM             0xffff

#define BAR_ALIGN_WORD_MASK          0xffffffc
#define BAR_MSG_ADDR_CHAN_INTERVAL   (1024*2)

/* 消息类型*/
#define BAR_CHAN_MSG_SYNC            0
#define BAR_CHAN_MSG_ASYNC           1
#define BAR_CHAN_MSG_NO_EMEC         0
#define BAR_CHAN_MSG_EMEC            1
#define BAR_CHAN_MSG_NO_ACK          0
#define BAR_CHAN_MSG_ACK             1

/* payload, valid和内容的偏移*/
#define BAR_MSG_PLAYLOAD_OFFSET      (sizeof(struct bar_msg_header))
#define BAR_MSG_LEN_OFFSET           2
#define BAR_MSG_VALID_OFFSET         0

/* valid字段的掩码*/
#define BAR_MSG_VALID_MASK           1

/* reps_buff的偏移*/
#define REPS_HEADER_VALID_OFFSET     0
#define REPS_HEADER_LEN_OFFSET       1
#define REPS_HEADER_PAYLOAD_OFFSET   4

#define REPS_HEADER_REPLYED          0xff

/* 通道状态*/
#define BAR_MSG_CHAN_USABLE          0
#define BAR_MSG_CHAN_USED            1

/* 超时时间 = 100 us *30000次轮询 = 3s*/
#define BAR_MSG_POLLING_SPAN_US      100
#define BAR_MSG_TIMEOUT_TH           30000

/* vf,pf,mpf总数*/
#define BAR_DRIVER_TOTAL_NUM        (BAR_MPF_NUM + BAR_PF_NUM + BAR_VF_NUM)

/* bar的通道偏移*/
#define BAR_INDEX_TO_RISC            0
#define BAR_MPF_NUM                  1

/* 定时器周期宏*/
#define BAR_MSGID_FREE_THRESHOLD     (jiffies + msecs_to_jiffies(2000))

/* 管理pf信息*/
#define BAR_MSG_OFFSET               (0x2000)
#define MPF_VENDOR_ID                (0x16c3)
#define MPF_DEVICE_ID                (0x8045)

enum {
    TYPE_SEND_NP = 0x0,
    TYPE_SEND_DRS = 0x01,
    TYPE_SEND_DTP = 0x10,
    TYPE_END,
};

/**************************************************************************
 * common.ko会工作在5中场景，不同场景每个mpf/pf/vf可以看到的bar不一样
 * 1、DPU场景下的host中：SCENE_HOST_IN_DPU
 * 2、DPU场景下的ZF中：  SCENE_ZF_IN_DPU
 * 3、智能网卡带ddr:     SCENE_NIC_WITH_DDR
 * 4、智能网卡不带ddr:   SCENE_NIC_NO_DDR
 * 5、普卡：            SCENE_STD_NIC
**************************************************************************/
#define SCENE_TEST

#ifdef SCENE_HOST_IN_DPU
#define BAR_PF_NUM                   31
#define BAR_VF_NUM                   1024
#define BAR_INDEX_PF_TO_VF           1
#define BAR_INDEX_MPF_TO_MPF         1
#define BAR_INDEX_MPF_TO_PFVF        0xff
#define BAR_INDEX_PFVF_TO_MPF        0xff
#endif

#ifdef  SCENE_ZF_IN_DPU
#define BAR_PF_NUM                   7
#define BAR_VF_NUM                   128
#define BAR_INDEX_PF_TO_VF           0xff
#define BAR_INDEX_MPF_TO_MPF         1
#define BAR_INDEX_MPF_TO_PFVF        0xff
#define BAR_INDEX_PFVF_TO_MPF        0xff
#endif

#ifdef  SCENE_NIC_WITH_DDR
#define BAR_PF_NUM                   31
#define BAR_VF_NUM                   1024
#define BAR_INDEX_PF_TO_VF           1
#define BAR_INDEX_MPF_TO_MPF         0xff
#define BAR_INDEX_MPF_TO_PFVF        0xff
#define BAR_INDEX_PFVF_TO_MPF        0xff
#endif

#ifdef  SCENE_NIC_NO_DDR
#define BAR_PF_NUM                   31
#define BAR_VF_NUM                   1024
#define BAR_INDEX_PF_TO_VF           1
#define BAR_INDEX_MPF_TO_MPF         0xff
#define BAR_INDEX_MPF_TO_PFVF        1
#define BAR_INDEX_PFVF_TO_MPF        2
#endif

#ifdef  SCENE_STD_NIC
#define BAR_PF_NUM                   7
#define BAR_VF_NUM                   256
#define BAR_INDEX_PF_TO_VF           1
#define BAR_INDEX_MPF_TO_MPF         0xff
#define BAR_INDEX_MPF_TO_PFVF        1
#define BAR_INDEX_PFVF_TO_MPF        2
#endif

#ifdef  SCENE_TEST
#define BAR_PF_NUM                   7
#define BAR_VF_NUM                   256
#define BAR_INDEX_PF_TO_VF           0
#define BAR_INDEX_MPF_TO_MPF         0xff
#define BAR_INDEX_MPF_TO_PFVF        0
#define BAR_INDEX_PFVF_TO_MPF        0
#endif

/* 左边通道还是右边通道*/
#define BAR_SUBCHAN_INDEX_SEND      0
#define BAR_SUBCHAN_INDEX_RECV      1

/* 消息源索引*/
#define BAR_MSG_SRC_NUM             3
#define BAR_MSG_SRC_MPF             0
#define BAR_MSG_SRC_PF              1
#define BAR_MSG_SRC_VF              2
#define BAR_MSG_SRC_ERR             0xff

/* 消息目的索引*/
#define BAR_MSG_DST_NUM             3
#define BAR_MSG_DST_RISC            0
#define BAR_MSG_DST_MPF             2
#define BAR_MSG_DST_PFVF            1
#define BAR_MSG_DST_ERR             0xff

/* msg_id项标志位状态*/
#define REPS_INFO_FLAG_USABLE       0
#define REPS_INFO_FLAG_USED         1

#define BAR_MSG_PAYLOAD_MAX_LEN     (BAR_MSG_ADDR_CHAN_INTERVAL - sizeof(struct bar_msg_header))

#define BAR_MSG_POL_MASK                (0x10)
#define BAR_MSG_POL_OFFSET              (4)

enum {
    CHECK_STATE_OK = 0,
    CHECK_STATE_EVENT_EXCEED = 1,
    CHECK_STATE_EVENT_NOT_EXIST = 2,
    CHECK_STATE_EVENT_ERR_RET = 4,
    CHECK_STATE_EVENT_ERR_REPS_LEN = 5,
};

struct zxdh_pcie_bar_msg_internal
{
    uint32_t id;        /**< the msg id that passing through */
    uint64_t virt_addr; /**< pcie bar mapping virtual addr */
};

/* bar通道消息头 */
struct bar_msg_header
{
    uint8_t valid: 1;               /* 消息通道状态 */
    uint8_t sync:  1;               /* 同步消息or异步消息*/
    uint8_t emec:  1;               /* 消息是否紧急 */
    uint8_t ack:  1;                /* 是否是回复消息*/
    uint8_t poll:  1;
    uint8_t usr:  1;
    uint8_t check;
    uint16_t event_id;              /* 请求的消息处理函数标识 */
    uint16_t len;                   /* 消息长度 */
    uint16_t msg_id;                /* 消息id*/
    uint16_t src_pcieid;
    uint16_t dst_pcieid;                   /* 用于pf给vf发消息*/
};

/* 根据消息的msgid查询回复缓存的地址和长度*/
struct msgid_reps_info
{
    void *reps_buffer;                   /* reps的地址*/
    uint16_t id;                         /* msg_id*/
    uint16_t buffer_len;                 /* buffer的最大长度*/
    uint16_t flag;                       /* 该条目是否被分配，已经非配和未被分配*/
    struct timer_list id_timer;          /* 该id对应的定时器*/
};

struct msix_msg
{
    uint16_t pcie_id;
    uint16_t vector_risc;
    uint16_t vector_pfvf;
    uint16_t vector_mpf;
} __attribute__((packed));

struct offset_get_msg
{
    uint16_t pcie_id;
    uint16_t type;
};

struct bar_offset_reps
{
    uint16_t check;
    uint16_t rsv;
    uint32_t offset;
    uint32_t length;
}__attribute__((packed));

struct bar_recv_msg
{
    uint8_t replied;
    uint16_t reps_len;
    uint8_t rsv1;
    union
    {
        struct bar_offset_reps offset_reps;
        uint8_t data[BAR_MSG_PAYLOAD_MAX_LEN - 4];
    };
}__attribute__((packed));

struct msgid_ring
{
    uint16_t msg_id;
    spinlock_t lock;
    struct msgid_reps_info reps_info_tbl[MAX_MSG_BUFF_NUM];
};

/* 异步消息相关实体*/
struct async_msg_entity
{
    struct task_struct *async_proc;        /* 异步队列消息线程*/
    struct mutex async_qlock;              /* 易怒队列入队锁*/
    struct bar_async_node *noemq_head;     /* 非紧急队列头*/
    struct bar_async_node *noemq_tail;     /* 非紧急队列尾部*/
    struct bar_async_node *emq_head;       /* 紧急队列头*/
    struct bar_async_node *emq_tail;       /* 紧急队列尾部*/
};

/* 异步消息队列节点*/
struct bar_async_node
{
    uint32_t msg_id;
    void *payload_addr;     /**< 消息净荷起始地址，有用户创建并填充 */
    uint64_t payload_len;   /**< 消息净荷长度. */
    uint64_t subchan_addr;  /**< 消息发送到哪个2K, 由virt_addr, src, dst共同决定，计算交给common来做>**/
    uint32_t event_id;     /**< 消息发送模块，描述消息哪个模块发送 */
    uint16_t src_pcieid;
    uint16_t dst_pcieid;           /**< 消息目的的bdf号，适用于PF与VF公用4K的时候用>**/
    uint16_t emec;          /**< 消息紧急类型，异步消息可以分为紧急消息和非紧急消息>**/
    uint16_t ack;
    uint8_t src;
    uint8_t dst;
    struct bar_async_node *next;
};

struct vqm_qid_reset_msg
{
    uint32_t qid;
}__attribute__((packed));
typedef struct
{
    uint16_t vqm_vfid; /* 设备号 */
    uint16_t opcode; /* get:0, set:1 */
#define VQM_QUEUE_RSET (14)
    uint16_t cmd; /* 控制命令类 mac-1, 多队列-4, feature-5 */
    union
    {
        uint8_t value[8]; /* 如果是set, 附带数据, 目前已知的有mac地址 */
        struct vqm_qid_reset_msg q_reset_msg;
    }__attribute__ ((packed));
}__attribute__ ((packed)) OVS_TO_VQM_MSG;

typedef struct
{
    uint32_t reps_hdr;
#define VQM_REPS_SUCCESS  (0xaa)
    uint32_t check_result;
    union
    {
        uint8_t rsv[40];
    }__attribute__ ((packed));
}__attribute__ ((packed)) VQM_RSP_OVS_DATA;

#define VCQ_NOTIFY_EVENT_ID  (36)

uint8_t bar_msg_col_index_trans(uint8_t dst);
uint8_t bar_msg_row_index_trans(uint8_t src);

#ifdef __cplusplus
}
#endif

#endif /* _ZXDH_MSG_CHAN_PRIV_H_  */

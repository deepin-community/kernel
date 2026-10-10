#ifndef _ZXDH_MSG_CHAN_PUB_H_
#define _ZXDH_MSG_CHAN_PUB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/netdevice.h>
#include <linux/workqueue.h>


struct zxdh_bar_extra_para
{
    bool is_sync; /* 是否同步消息 */
    uint16_t retrycnt; /* bar消息重传次数 */
}__attribute__ ((packed));


#define INVALID_NUM 0xff
#define ZXDH_MPF_PCIEID       0x800
#define ZXDH_NET_ACK_OK       0
#define RISCV_MAC_OK          0xaa
#define BAR_MSG_REPS_OK       0xff
#define COMMON_TBL_OK         0xaa
#define RISCV_DEBUG_OK        0xaa
#define PCIEID_PF_ID_MASK    (0x0700)
#define PCIEID_PF_ID_OFFSET     (8)

#define FIND_PF_PCIE_ID(value)                    ((value & 0xff00) | BIT(11))
#define FIND_VF_PCIE_ID(pf_pcie_id, vf_id)        ((pf_pcie_id & (~BIT(11))) | (vf_id))
#define FIND_PF_ID(pf_pcie_id)                   ((pf_pcie_id & PCIEID_PF_ID_MASK) >> PCIEID_PF_ID_OFFSET)
#define GET_FUNC_NO(pf_no, vf_idx)                ((pf_no & 0xF) | ((vf_idx & 0xFF) << 8))

#define PFVF_FLAG_OFFSET 11 // 用于判断pcie_id的第11位为0或1

typedef enum
{
    MODULE_DBG = 0,    /* 中断测试*/
    MODULE_TBL,        /* 资源表交互*/
    MODULE_MSIX,       /* Msix配置*/
    MODULE_SDA,        /* Sda消息*/
    MODULE_RDMA,       /* Rdma调试*/
    MODULE_DEMO,       /* 通路测试*/
    MODULE_SMMU,       /* Smmu调试*/
    MODULE_MAC,        /* MAC相关*/
    MODULE_VDPA,       /* vdpa热迁移*/
    MODULE_VQM,        /* vqm消息*/
    MODULE_MSGQ,
    MODULE_VPORT_GET,  /* 获取vport接口*/
    MODULE_BDF_GET,    /* 获取bdf接口*/
    MODULE_RISC_READY, /* risc ready信号*/
    MODULE_REVERSE,    /* 字节流取反*/
    MODULE_NVME,       /* NVME调试*/
    MODULE_NPSDK,      /* Np配表*/
    MODULE_TOD,        /* UART通信*/
    MODULE_VF_BAR_MSG_TO_PF,  /* VF发送给PF的消息 */
    MODULE_PF_BAR_MSG_TO_VF,  /* PF发送给VF的消息 */
    MODULE_DEBUG = 20,      /* 调用debug接口 */
    MODULE_PPS = 23,        /*pps中断相关消息*/
    MODULE_VIRTIO = 25,
    MODULE_FLASH = 32, /* 读取flash信息 */
    MODULE_OFFSET_GET = 33,
    MODULE_CFG_MAC = 34,
    MODULE_CFG_VQM = 36,
    MODULE_PHYPORT_QUERY = 37,  /* BOND 获取phyport*/
    MODULE_DHTOOL = 39,    /*dhtool，riscv发送给AUX层的消息，将其转发给user*/
    MODULE_RESET_MSG = 40, /* host复位消息的ID号 */
    MODULE_PF_TIMER_TO_RISC_MSG = 41, /* server rsicv time */
    MODULE_LOGIN_CTRL = 43, /* 控制riscv上sshd守护进程开启和关闭*/
    MODULE_PCIE_RES_QUERY = 52, /* pcie资源查询*/
    MODULE_DTP = 53,
    MODULE_DPU_PCIE_PF_INFO = 54, /* dpu卡zf_mpf bar消息*/
    MODULE_HEALTH = 55, /* 自愈相关 */
    MODULE_VQMB = 59, /* riscv->host */
    MSG_MODULE_NUM = 60,
} MSG_MODULE_ID;


/* 消息端*/
typedef enum BAR_DRIVER_TYPE
{
    MSG_CHAN_END_MPF = 0,
    MSG_CHAN_END_PF,
    MSG_CHAN_END_VF,
    MSG_CHAN_END_RISC,
    MSG_CHAN_END_ERR,
} BAR_DRIVER_TYPE;

#define BDF_ECAM(bus, devid, func)   ((bus & 0xff) << 8) | (func & 0x07) | ((devid & 0x1f) << 3)
#define SBDF_ECAM(domain, bus, devid, func)   ((domain & 0xffff) << 16) |   \
                ((bus & 0xff) << 8) | (func & 0x07) | ((devid & 0x1f) << 3)

/* bar通道错误码*/
typedef enum BAR_MSG_RTN
{
    BAR_MSG_OK = 0,
    BAR_MSG_ERR_NULL,              /* 空指针*/
    BAR_MSG_ERR_TYPE,              /* 消息类型异常 */
    BAR_MSG_ERR_MODULE ,           /* 模块ID异常 */
    BAR_MSG_ERR_BODY_NULL,         /* 消息体异常 */
    BAR_MSG_ERR_LEN,               /* 消息长度异常 */
    BAR_MSG_ERR_TIME_OUT,          /* 消息发送超长 */
    BAR_MSG_ERR_NOT_READY,         /* 消息发送条件异常，BUF不可以用*/
    BAR_MEG_ERR_NULL_FUNC,         /* 空的接收处理函数指针*/
    BAR_MSG_ERR_REPEAT_REGISTER,   /* 模块重复注册*/
    BAR_MSG_ERR_UNGISTER,          /* 重复解注册*/
    BAR_MSG_ERR_NULL_PARA,         /* 发送接口参数界结构体指针为空*/
    BAR_MSG_ERR_REPSBUFF_LEN,      /* reps_buff的长度太短*/
    BAR_MSG_ERR_MODULE_NOEXIST,    /*查找不到该模块对应的消息处理函数*/
    BAR_MSG_ERR_VIRTADDR_NULL,     /*发送接口传入参数中的虚拟地址为空*/
    BAR_MSG_ERR_REPLY,             /**< seq_id匹配失败>**/
    BAR_MSG_ERR_MSGID,             /**< seq_id申请失败>**/
    BAR_MSG_ERR_MPF_NOT_SCANED,    /**< MPF通道不可用>**/
    BAR_MSG_ERR_USR_RET_ERR,       /**< 处理函数返回值错误>**/
    BAR_MSG_ERR_ERR_PCIEID,        /**< pcieID错误>**/
    BAR_MSG_ERR_LOCK_FAILED,	   /**获取硬件锁失败**/
    BAR_MSG_ERR_BAR_ABNORMAL,	   /**bar全ff**/
    BAR_MSG_ERR_NOT_MATCH,
} BAR_MSG_RTN;

enum pciebar_layout_type
{
    URI_VQM         = 0,
    URI_SPINLOCK    = 1,
    URI_FWCAP       = 2,
    URI_FWSHR       = 3,
    URI_DRS_SEC     = 4,
    URI_RSV         = 5,
    URI_CTRLCH      = 6,
    URI_1588        = 7,
    URI_QBV         = 8,
    URI_MACPCS      = 9,
    URI_RDMA        = 10,
/* DEBUG PF */
    URI_MNP         = 11,
    URI_MSPM        = 12,
    URI_MVQM        = 13,
    URI_MDPI        = 14,
    URI_NP          = 15,
/* END DEBUG PF */
    URI_MAX,
};

typedef enum
{
    BAR_MSG_MSIX_FROM_VF = 0,
    BAR_MSG_MSIX_FROM_MPF,
    BAR_MSG_MSIX_FROM_RISCV,
    BAR_MSG_MSIX_NUM_MAX
} bar_msg_msix_irq_type;

/* msix消息参数结构体*/
struct msix_para
{
    uint16_t vector_risc;
    uint16_t vector_pfvf;
    uint16_t vector_mpf;
    uint16_t driver_type;
    uint16_t pcie_id;
    struct pci_dev *pdev;
    uint64_t virt_addr;
};

struct bar_offset_params
{
    uint64_t virt_addr;
    uint16_t pcie_id;
    uint16_t type;
};
struct bar_offset_res
{
    uint32_t bar_offset;
    uint32_t bar_length;
};

struct zxdh_pci_bar_msg
{
    uint64_t virt_addr;               /**< 4k空间地址, 若src为MPF该参数不生效>**/
    void *payload_addr;               /**< 消息净荷地址>**/
    uint16_t payload_len;             /**< 消息净荷长度>**/
    uint16_t emec;                    /**< 消息紧急类型>**/
    uint16_t src;                     /**< 消息发送源，参考BAR_DRIVER_TYPE>**/
    uint16_t dst;                     /**< 消息接收者，参考BAR_DRIVER_TYPE>**/
    uint32_t event_id;                /**< 事件id>**/
    uint16_t src_pcieid;              /**< 源  pcie_id>**/
    uint16_t dst_pcieid;              /**< 目的pcie_id>**/
};

struct link_info_struct
{
    uint32_t speed;
    uint32_t autoneg_enable;
    uint32_t supported_speed_modes;
    uint32_t advertising_speed_modes;
    uint8_t  duplex;
};

struct zxdh_msg_recviver_mem
{
    void *recv_buffer;                /**< 消息接收缓存>**/
    uint16_t buffer_len;              /**< 消息缓存长度>**/
};

/**
 * zxdh_bar_chan_msg_recv_callback - 消息处理函数
 * @pay_load: 消息内容
 * @len: 消息长度
 * @reps_buffer: 回复消息
 * @reps_len: 回复消息长度
 * @dev: 私有数据
 */
typedef int (*zxdh_bar_chan_msg_recv_callback)(void *pay_load, uint16_t len, void *reps_buffer, uint16_t *reps_len, void *dev);

/**
 * zxdh_bar_chan_sync_msg_send - 通过PCIE BAR空间发送同步消息
 * @in: 消息发送信息
 * @result: 消息结果反馈
 * @return: 0 成功，其他失败
 */
int zxdh_bar_chan_sync_msg_send(struct zxdh_pci_bar_msg *in, struct zxdh_msg_recviver_mem *result);

/**
 * zxdh_bar_send_without_reps_hdr - 回复不包括四字节头
 * @in: 消息发送信息
 * @result: 消息结果反馈
 * @return: 0 成功，其他失败
 */
int zxdh_bar_send_without_reps_hdr(struct zxdh_pci_bar_msg *in, struct zxdh_msg_recviver_mem *result);

/**
 * zxdh_bar_chan_msg_recv_register - PCIE BAR空间消息方式，注册消息接收回调
 * @event_id: 注册模块id
 * @callback: 模块实现的接收处理函数指针
 * @return: 0 成功，其他失败
 * 在驱动初始化时调用
 */
int zxdh_bar_chan_msg_recv_register(uint8_t event_id, zxdh_bar_chan_msg_recv_callback callback);

/**
 * zxdh_bar_chan_msg_recv_unregister - PCIE BAR空间消息方式，解注册消息接收回调
 * @event_id: 内核PCIE设备地址
 * @return:0 成功，其他失败
 * 在驱动卸载时需要调用
 */
int zxdh_bar_chan_msg_recv_unregister(uint8_t event_id);

/**
 * zxdh_bar_callback_register_state - 查看某一个消息处理函数状态
 * @event_id: 事件id
 * @return:0 表示已经注册，其他表示未注册
 * 在驱动卸载时需要调用
 */
int zxdh_bar_callback_register_state(uint16_t event_id);

/**
 * zxdh_bar_enable_chan - 驱动使能通道函数
 * @_msix_para: msix中断配置信息
 * @pcie_id: 查询到的pcie_id
 * @vport: 查询到的vport
 * @return: 0 成功，其他失败
 */
int zxdh_bar_enable_chan(struct msix_para *_msix_para, uint16_t *vport);

/**
 * zxdh_get_bar_offset - 获取指定模块的偏移值
 * @bar_offset_params:  输入参数
 * @bar_offset_res: 模块偏移和长度
 */
int zxdh_get_bar_offset(struct bar_offset_params *paras, struct bar_offset_res *res);

int32_t zxdh_send_command(uint64_t vaddr, uint16_t pcie_id, uint16_t module_id, \
                            void *msg, void *ack, uint16_t ack_len, bool is_sync_msg);

int zxdh_bar_msg_chan_init(void);
int zxdh_bar_msg_chan_remove(void);

/**
 * zxdh_bar_reset_valid - 重置risc发来消息的valid
 * @subchan_addr: 4k首地址
 * @return: 0 成功，其他失败
 */
void zxdh_bar_reset_valid(uint64_t subchan_addr);

/**
 * zxdh_get_event_id - 获取risc发来消息的event_id
 * @subchan_addr: 4k首地址
 * @return: event_id
 */
uint16_t zxdh_get_event_id(uint64_t subchan_addr, uint8_t src_type, uint8_t dst_type);

int zxdh_bar_irq_recv(uint8_t src, uint8_t dst, uint64_t virt_addr, void *dev);
int32_t call_msg_recv_func_tbl(uint16_t event_id, void *pay_load, uint16_t len, void *reps_buffer, uint16_t *reps_len, void *dev);

/**
 * bar_chan_pf_init_spinlock - 自定义网络驱动清理硬件锁
 * @pcie_id: 设备pcie_id
 * @bar_base_addr: 设备bar0虚拟基地址
 */
int bar_chan_pf_init_spinlock(uint16_t pcie_id, uint64_t bar_base_addr);


typedef int (*zxdh_usr_msg_cache_callback)(uint16_t event_id, void *msg, uint16_t msg_len);
/**
 * zxdh_usr_msg_cache_func_register - 消息缓存函数注册接口
 * zxdh-zf-mpf调用， 当接收接口接收到用户态消息时， 调用钩子进行消息缓存
 * @msg_cache_func: 消息缓存函数， 参考格式msg_cache_func
 */
void zxdh_usr_msg_cache_func_register(zxdh_usr_msg_cache_callback func);

int32_t zxdh_vqm_queue_cfg(uint64_t virt_addr, uint16_t pcie_id, uint32_t phy_queue_idx);

#ifdef __cplusplus
}
#endif

#endif /* _ZXDH_MSG_CHAN_PUB_H_ */

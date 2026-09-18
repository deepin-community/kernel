#ifndef ZXDH_TOOLS_IOCTL_H_
#define ZXDH_TOOLS_IOCTL_H_

#include <linux/dinghai/log.h>
#include "../en_aux.h"
#include "../en_np/table/include/dpp_tbl_pkt_cap.h"
#include "../en_np/flow/api/include/dpp_tbl_roce_match.h"
#include "../en_np/table/include/dpp_tbl_api.h"
#include "../en_np/sdk/include/api/dpp_apt_se_api.h"
#include "../en_np/sdk/include/dev/module/table/se/dpp_dtb_table.h"
#include "../en_aux/en_aux_cmd.h"

#define SAFE_KFREE(ptr) \
if ((ptr) != NULL) { \
    kfree(ptr); \
    (ptr) = NULL; \
}

#define MAX_COS 8

/* dhtool与内核驱动有新的兼容问题时维护这些值 */
#define       DHTOOL_COMPAT_ITM         (0)
#define       DHTOOL_COMPAT_MAJOR       (0)
#define       DHTOOL_COMPAT_DRIV_MINOR  (0)
#define       DHTOOL_COMPAT_TOOL_MINOR  (0)
#define       DHTOOL_COMPAT_PATCH       (0)


#define       DH_SWITCH_DEVICE_ID    0x8036
#define       DH_SWITCH_VENDOR_ID    0x1cf2

#define       MAX_DHTOOL_PID_NUMS    15
#define       DHTOOL_ERROR           0x1a
#define       max_entry_num          68
#define       key_entry_num          2
#define       normal_tcam_index      60
#define       key_tcam_index         68
#define       ZXDH_PKT_FLAG          0xbb
#define       PKT_PAYLIAD_VALUE      255
#define       ZXDH_PKT_INIT_SPEED    10000
#define       ZXDH_PKT_HEDER_LENGTH  20
#define       ZXDH_PKT_SPLIT_HEDER_LENGTH  28
#define       ZXDH_PKT_PD_HDR_LEN          16
#define       ZXDH_PCAP_MAGIC        0xa1b2c3d4
#define       ZXDH_PCAP_VER_MAJOR    2
#define       ZXDH_PCAP_VER_MINOR    4
#define       ZXDH_PCAP_THISZONE     0
#define       ZXDH_PCAP_SIGFIGS      0
#define       ZXDH_PCAP_SNAPLEN      65535
#define       ZXDH_PCAP_LINKTYPE     1
#define       ZXDH_ROCE_CYCLE_INIT_SPEED    1000000

/* Started by AICoder, pid:kd6be6da67t21ef1435f09dca0f04f1b6b946b44 */
#define DBDF_ECAM(domain, bus, devid, func)   ((domain & 0xffff) << 16) | ((bus & 0xff) << 8) | ((devid & 0x1f) << 3) | (func & 0x07)
/* Ended by AICoder, pid:kd6be6da67t21ef1435f09dca0f04f1b6b946b44 */

typedef enum
{
    MSG_MARK_INFO = 0,
    MSG_SEND_TO_RISCV,
    MSG_DEVICE_INFO_GET,
    MSG_SET_VF_STATUS,
    MSG_DEVICE_PHYPORT_GET,
    MSG_PKT_CAPTURE = 5,
    MSG_GET_DRV_VERSION = 6,
    MSG_SET_VF_MAC = 7,
    MSG_GET_SW_STAT = 8,
    MSG_SDT_TABLE = 9,
    MSG_PORT_CONFIG = 10,
    MSG_QOS = 11,
    MSG_CSIG_TAG = 12,
    MSG_ROCE_CYCLE_STAT = 14,
    SUBCMD_NUM
}MSG_SUBCMD;

struct zxdh_port_cfg_msg
{
    uint32_t op_code;
    uint32_t port_id;
    uint32_t index;
    uint32_t start_bit_no;
    uint32_t end_bit_no;
    uint32_t port_cfg_data;
}__attribute__((packed));

typedef enum dhtool_port_cfg_cmd_index
{
    DHTOOL_PORT_CFG_CMD_SET = 0,
    DHTOOL_PORT_CFG_CMD_GET = 1,
    
}DHTOOL_PORT_CFG_CMD_INDEX;

typedef struct zxdh_port_cfg_response_info
{
    uint32_t status;
    uint32_t port_cfg_data;
}ZXDH_PORT_CFG_RESPONSE_INFO;
struct zxdh_pkt_capture_msg
{
    uint32_t op_code;
    uint16_t payload_len;
    uint8_t  payload[0];
}__attribute__((packed));

typedef enum dhtool_pkt_capture_main_cmd_index
{
    DHTOOL_PKT_CAPTURE_CMD_ENABLE = 0,
    DHTOOL_PKT_CAPTURE_CMD_DISABLE,
    DHTOOL_PKT_CAPTURE_CMD_DISABLE_ALL,
    DHTOOL_PKT_CAPTURE_CMD_RULE_INSERT,
    DHTOOL_PKT_CAPTURE_CMD_RULE_DELETE,
    DHTOOL_PKT_CAPTURE_CMD_SHOW,
    DHTOOL_PKT_CAPTURE_CMD_SAVE_TO_FILE,
    DHTOOL_PKT_CAPTURE_CMD_SET_SPEED,
    DHTOOL_PKT_CAPTURE_CMD_ERROR
}DHTOOL_PKT_CAPTURE_MAIN_CMD_INDEX;

enum QosPortMode
{
    QOS_PANEL,
    QOS_INTERNAL
};

enum QosOpcode
{
    QOS_OP_CODE_SHOW,
    QOS_OP_CODE_CONFIG
};

struct qos_command_msg_t
{
    uint32_t op_code;
    uint32_t mode;
    uint32_t buffer_size[MAX_COS];
    uint32_t threshold_size[MAX_COS];
};

struct qos_response_t
{
    uint32_t panel_buffer_size[MAX_COS];
    uint32_t panel_threshold_size[MAX_COS];
    uint32_t internal_buffer_size[MAX_COS];
    uint32_t internal_threshold_size[MAX_COS];
};

typedef enum
{
    MSG_RECV_NOT_FOUND = 0,
    MSG_RECV_OK = 1,
    MSG_RECV_FAILED = 2,
    MSG_RECV_PKT_CAP_PF_LOCK = 3,
    MSG_RECV_PKT_FILE_PATH_ERR = 4,
    MSG_RECV_PKT_FILE_EXIST_ERR = 5,
    MSG_RECV_PKT_FILE_IN_PROGRESS_ERR = 6,
}DHTOOL_RESPONSE;

typedef enum
{
    MSG_RECV_ROCE_NOT_FOUND = 0,
    MSG_RECV_ROCE_OK = 1,
    MSG_RECV_ROCE_FAILED = 2,
    MSG_RECV_ROCE_STAT_NOT_ENABLE = 3,
    MSG_RECV_ROCE_STAT_FLOW_FULL = 4,
}ZXDH_ROCE_CYCLE_STAT_RESPONSE;

typedef enum
{
    NO_SWITCH = 0,
    SWITCH = 1,
}SWITCH_FLAG;

struct zxdh_tools_msg
{
    uint32_t subcmd;            //provider care
    uint32_t event_pid;         //provider care
    void*    tools_reps;        //provider care
    uint32_t event_id;          //caller care
    uint16_t dst;               //caller care
    uint16_t dst_pcieid;        //caller care
    void*    msg_reps;          //caller care
    uint16_t msg_reps_len;      //caller care
    uint16_t sync_or_async;     //caller care
    uint16_t payload_len;       //caller care
    uint16_t reserved;           //caller care
    uint8_t  payload[0];
}__attribute__((packed));

struct zxdh_tools_reps
{
    uint32_t status; /* must be */
    int32_t bar_or_vq_chan_ret;
    uint32_t data[15];
};

struct zxdh_tools_ioctl_subcmd_info
{
    MSG_SUBCMD subcmd;
    int32_t (*subcmd_callback)(struct net_device *netdev, struct ifreq *ifr);
};

struct dhtool_dev_pcieid_get
{
   uint32_t dev_pcieid;
};
struct dhtool_dev_info
{
    uint32_t domain_no;
    uint32_t bus_no;
    uint32_t device_no;
    uint32_t func_no;
};

struct dhtool_dev_info_get_reps
{
    SWITCH_FLAG switch_or_noswitch;
    struct dhtool_dev_info dev_info;
    struct dhtool_dev_info rp_info;
    union
    {
        struct dhtool_dev_info swusp_info;
    };
};

struct dhtool_eventpid_devbdf_array
{
    bool     is_valid;
    uint16_t dev_pcieid;
    uint32_t dev_bdf;
    uint32_t event_pid;
};

typedef enum
{
    ALL_VF ,
    VF3_MAX
}VF_SET_MODE;

typedef enum
{
    VF_STATUS_AUTO ,
    VF_STATUS_ENABLE ,
    VF_STATUS_DISABLE
}VF_SET_STATUS;

struct dhtool_set_vf_status_msg {
    VF_SET_MODE mode;              // 存储 'all_vf' 或 'vf3_max'
    VF_SET_STATUS vf_status;   // 存储 'auto', 'enable', 'disable'
};

struct dhtool_dev_phyport_get
{
    uint8_t phyport;
    uint8_t rsv[15];
};

struct pkt_deve_name
{
    char dev_name[IFNAMSIZ];
};

typedef struct zxdh_pkt_cap_cmd_save_to_file
{
    char file_path[150];
    uint32_t file_size;
    uint32_t pkt_count;
    uint32_t is_stop;
}ZXDH_PKT_CAP_CMD_SAVE_TO_FILE;

typedef struct zxdh_pkt_cap_rule_rule_insert
{
    uint32_t rule_index;
    ZXDH_PKT_CAP_POINT cap_point;
    ZXDH_PKT_CAP_MODE  cap_mode;
    ZXDH_PKT_CAP_KEY pkt_cap_key;
    ZXDH_PKT_CAP_NORMAL_CONFIG rule_config;
    char dev_name[IFNAMSIZ];
}ZXDH_PKT_CAP_RULE_INSERT;

typedef struct zxdh_pkt_cap_rule_rule_delete
{
    uint32_t rule_index;
    uint32_t is_mode_all;
    uint32_t is_all;
    ZXDH_PKT_CAP_POINT cap_point;
    ZXDH_PKT_CAP_MODE  cap_mode;
}ZXDH_PKT_CAP_RULE_DELETE;

typedef struct zxdh_pkt_cap_cmd_show
{
    uint32_t speed;
    uint32_t entry_num;
    uint32_t file_size;
    uint32_t pkt_count;
    uint32_t is_save_to_file;
    ZXDH_PKT_CAP_ENABLE_STATUS enable_status;
    ZXDH_PKT_CAP_RULE_INSERT entry_array[68];
    char file_path[150];
}ZXDH_PKT_CAP_CMD_SHOW;

typedef uint32_t (*callback_t)(struct net_device *netdev, struct zxdh_tools_msg *tool_msg, struct zxdh_pkt_capture_msg *pkt_msg, DPP_PF_INFO_T *pf_info);

typedef struct
{
    uint32_t op_code;
    callback_t callback;
} zxdh_pkt_capture_callback_entry_t;

/***********************************************************/
/** ROCE Cycle Stat 命令枚举
************************************************************/
typedef enum zxdh_roce_cycle_stat_main_cmd_index
{
    ZXDH_ROCE_CYCLE_STAT_CMD_ENABLE = 0,      /*无填充*/
    ZXDH_ROCE_CYCLE_STAT_CMD_DISABLE,      /*无填充*/
    ZXDH_ROCE_CYCLE_STAT_CMD_FLOW_INSERT,      /*req填充ZXDH_ROCE_CYCLE_STAT_FLOW_INSERT，resp无填充*/
    ZXDH_ROCE_CYCLE_STAT_CMD_FLOW_DELETE,      /*req填充ZXDH_ROCE_CYCLE_STAT_FLOW_DELETE，resp无填充*/
    ZXDH_ROCE_CYCLE_STAT_CMD_GET_FLOW_NUM,      /*req无填充，resp填充 uint16_t 表项个数*/
    ZXDH_ROCE_CYCLE_STAT_CMD_SHOW,      /*req无填充，resp填充ZXDH_ROCE_CYCLE_STAT_FLOW_SHOW*/
    ZXDH_ROCE_CYCLE_STAT_CMD_SET_SPEED,     /*req填充 uint32_t 限速值，resp无填充*/
}ZXDH_ROCE_CYCLE_STAT_MAIN_CMD_INDEX;

/***********************************************************/
/** ROCE Cycle Stat 消息结构
************************************************************/
struct zxdh_roce_cycle_stat_msg
{
    uint32_t op_code;
    uint16_t payload_len;
    uint8_t  payload[0];
}__attribute__((packed));

/***********************************************************/
/** ROCE Cycle Stat Flow Insert 结构
* @param   key[36]        匹配的流 key (如 SIP, DIP, upcall_port 等)
* @param   result[4]      统计结果配置
************************************************************/
typedef struct zxdh_roce_cycle_stat_flow_insert
{
    uint8_t key[36];
    uint8_t result[4];
}ZXDH_ROCE_CYCLE_STAT_FLOW_INSERT;

/***********************************************************/
/** ROCE Cycle Stat Flow Delete 结构
* @param   is_all         是否删除所有规则
* @param   key[36]        要删除的流 key
************************************************************/
typedef struct zxdh_roce_cycle_stat_flow_delete
{
    uint32_t is_all;
    uint8_t key[36];
}ZXDH_ROCE_CYCLE_STAT_FLOW_DELETE;

/***********************************************************/
/** ROCE Cycle Stat Flow Show 结构
* @param   entry_num      表项个数
* @param   enable_status  使能状态
* @param   entry_array[0] 表项数组
************************************************************/
typedef struct zxdh_roce_cycle_stat_flow_show
{
    uint32_t speed;
    uint32_t entry_num;
    uint8_t enable_status;
    ZXDH_ROCE_CYCLE_STAT_FLOW_INSERT entry_array[0];
}ZXDH_ROCE_CYCLE_STAT_FLOW_SHOW;

/***********************************************************/
/** ROCE Cycle Stat 回调类型定义
************************************************************/
typedef uint32_t (*roce_cycle_stat_callback_t)(struct net_device *netdev,
    struct zxdh_tools_msg *tool_msg, struct zxdh_roce_cycle_stat_msg *pkt_msg, DPP_PF_INFO_T *pf_info);

typedef struct
{
    uint32_t op_code;
    roce_cycle_stat_callback_t callback;
} zxdh_roce_cycle_stat_callback_entry_t;

typedef uint32_t (*port_callback_t)(struct zxdh_tools_msg *tool_msg, struct zxdh_port_cfg_msg *pkt_msg, DPP_PF_INFO_T *pf_info);
typedef struct
{
    uint32_t op_code;
    port_callback_t callback;
} zxdh_port_cfg_callback_entry_t;

typedef uint32_t (*qos_callback_t)(struct net_device *netdev, struct zxdh_tools_msg *tool_msg, struct qos_command_msg_t *qos_msg, DPP_PF_INFO_T *pf_info);

typedef struct
{
    uint32_t op_code;
    qos_callback_t callback;
} zxdh_dhtool_qos_callback_entry_t;

struct dhtool_compat_reg
{
    uint8_t version_compat_item;
    uint8_t major;
    uint8_t tool_minor;
    uint8_t drv_minor;
    uint16_t patch;
    uint8_t rsv[2];
}__attribute__((packed));

typedef enum
{
    zxdh_cap_disable ,
    zxdh_cap_enable
}zxdh_cap_status;

struct pcap_file_header {
    uint32_t magic;
    uint16_t version_major;
    uint16_t version_minor;
    int32_t thiszone;
    uint32_t sigfigs;
    uint32_t snaplen;
    uint32_t linktype;
};

struct pcap_pkthdr {
    uint32_t ts_sec;
    uint32_t ts_usec;
    uint32_t caplen;
    uint32_t len;
};

int32_t zxdh_tools_ioctl_dispatcher(struct net_device *netdev, struct ifreq *ifr);
ssize_t pkt_packet_to_file(struct zxdh_en_device *en_dev, const char *data, size_t len);
uint8_t pkt_packet_process(struct zxdh_en_device *en_dev, void *buf, uint32_t len, uint8_t pkt_flag);
uint8_t pkt_skb_packet_process(struct zxdh_en_device *en_dev, struct sk_buff *skb, uint8_t pkt_flag);
void capture_save_file_work_handler(struct work_struct *work);
void close_log_file(struct file *filp);
#define DHTOOLS_LOG_ERR(fmt, arg...) DH_LOG_ERR(MODULE_DHTOOLS, fmt, ##arg);
#define DHTOOLS_LOG_INFO(fmt, arg...) DH_LOG_INFO(MODULE_DHTOOLS, fmt, ##arg);
#define DHTOOLS_LOG_DEBUG(fmt, arg...) DH_LOG_DEBUG(MODULE_DHTOOLS, fmt, ##arg);

#define DHTOOLS_LOG_ERR_DEV(dev, fmt, arg...) DH_LOG_ERR_DEV(MODULE_DHTOOLS, dev, fmt, ##arg);
#define DHTOOLS_LOG_INFO_DEV(dev, fmt, arg...) DH_LOG_INFO_DEV(MODULE_DHTOOLS, dev, fmt, ##arg);
#define DHTOOLS_LOG_DEBUG_DEV(dev, fmt, arg...) DH_LOG_DEBUG_DEV(MODULE_DHTOOLS, dev, fmt, ##arg);

#endif

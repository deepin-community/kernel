/*****************************************************************************
(C) 2023 ZTE. 版权所有.

文件名      : en_1588_pkt_proc_func.h
内容摘要    : 不同数据类型包的处理接口实现
版本        : 1.0
*****************************************************************************/

#ifndef _EN_1588_PKT_PROC_FUNC_H_
#define _EN_1588_PKT_PROC_FUNC_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#include "../en_aux.h"
#include "queue.h"

#define PTP_REG_INFO_NUM                32
#define MAX_PTP_REG_INFO_NUM            64

/* MAC FIFO相关定义 */
#define MSGTYPE_OFFSET                  20
#define SRCPORTID_OFFSET                16

/* PTP报文时间戳处理函数返回值 */
#define PTP_RET_SUCCESS                 0
#define PTP_RET_TIME_ERR                (-1)

/* CF字段ns位和s位长度 */
#define CF_DECIMAL_NS_SIZE              2
#define CF_NS_SIZE                      6
#define CF_SIZE                         8

/* PTP时间戳长度 */
#define PTP_TS_5G_LEN                   10
#define PTP_TS_TSN_LEN                  10
#define PTP_REQRECE_TS_LEN              10

/* PTP报文后缀相关字段偏移和长度 */
#define PTPHDR_FREQUENCY_OFFSET         54
#define PTPHDR_TSI_OFFSET               86
#define PTPHDR_TSI_TLV_OFFSET           76
#define PTPHDR_TSI_TLV_OFFSET_TWO       44
#define PTPHDR_TSI_TLV_LEN              20
#define ORIGINTIMESTAMP_LEN             10
#define FOLLOWUP_TLV_LEN                32
#define TSITLV_LEN                      20
#define SRCPORTID_LEN                   10

/* 80bit时间戳ns位和s位长度 */
#define S_SIZE                          6           /* 高48位 */
#define NS_SIZE                         4           /* 低32位 */
#define S_HOLD                          1000000000L /* 进位阈值，即低32位达到1e9 */

/* pi头中cpu_tx字段，高29位为ns位，低3位为小数ns位 */
#define CPU_TX_DECIMAL_NS               3
#define CPU_TX_NS                       29

typedef struct
{
    uint8_t data[S_SIZE + NS_SIZE];
} Bits80_t;

struct time_stamps
{
    uint64_t s;
    uint32_t ns;
};

struct SkbSharedHwtstamps_t
{
    struct time_stamps ts_5g_t;
    struct time_stamps ts_tsn_t;
};

struct ptpHdr_t
{
    uint8_t     majorType;              /* 高4位为majorSdoId，低4位为msgType */
    uint8_t     versionPTP;
    uint16_t    msglen;
    uint8_t     domainNumber;
    uint8_t     minorSdoId;
    uint16_t    flagField;
    uint8_t     correctionField[CF_SIZE];     /* 高48位为ns位，低16字节为小数ns位 */
    uint32_t    msgTypeSpecific;        /* 大端 */
    uint8_t     srcPortIdentity[SRCPORTID_LEN];
    uint16_t    sequenceId;
    uint8_t     controlField;
    uint8_t     logMsgInterval;
} __attribute__((packed));


struct ptp_reg_info
{
    uint32_t cfVal[2]; //High寄存器是ns位，Low寄存器的高16bit是ns位，低16bit是ns小数位
    uint32_t matchInfo; //保存的内容是[bit23:0]:  {MessageType[23:20], sourcePortIdentity[19:16], sequenceId[15:0]}
};

struct ptp_buff
{
    uint32_t cfCount;
    struct ptp_reg_info ptpRegInfo[PTP_REG_INFO_NUM];
};

struct ptp_update_buff
{
    uint32_t cfCount;
    struct ptp_reg_info ptpRegInfo[MAX_PTP_REG_INFO_NUM];
};

/* PTP报文类型枚举 */
enum
{
    /* event message types */
    PTP_MSG_TYPE_SYNC = 0,
    PTP_MSG_TYPE_DELAY_REQ,
    PTP_MSG_TYPE_PDELAY_REQ,
    PTP_MSG_TYPE_PDELAY_RESP,

    /* general message types */
    PTP_MSG_TYPE_FOLLOW_UP = 8,
    PTP_MSG_TYPE_DELAY_RESP,
    PTP_MSG_TYPE_PDELAY_RESP_FOLLOW_UP,
    PTP_MSG_TYPE_ANNOUNCE,
    PTP_MSG_TYPE_SIGNALING,
    PTP_MSG_TYPE_MANAGEMENT
};

uint64_t htonll(uint64_t u64_host);

int32_t pkt_proc_type_sync(struct sk_buff *skb,                         \
                           struct zxdh_1588_pd_tx *hdr,               \
                           uint8_t *ptpHdr,                             \
                           struct time_stamps *t5g,                     \
                           struct time_stamps *tsn,                     \
                           uint32_t *thw,                               \
                           struct zxdh_en_device *en_dev);

int32_t pkt_proc_type_delay_req(struct sk_buff *skb,                    \
                                struct zxdh_1588_pd_tx *hdr,          \
                                uint8_t *ptpHdr,                        \
                                struct time_stamps *t5g,                \
                                struct time_stamps *tsn,                \
                                uint32_t *thw,                          \
                                struct zxdh_en_device *en_dev);

int32_t pkt_proc_type_pdelay_req(struct sk_buff *skb,                   \
                                struct zxdh_1588_pd_tx *hdr,          \
                                uint8_t *ptpHdr,                        \
                                struct time_stamps *t5g,                \
                                struct time_stamps *tsn,                \
                                uint32_t *thw,                          \
                                struct zxdh_en_device *en_dev);

int32_t pkt_proc_type_pdelay_resp(struct sk_buff *skb,                  \
                                struct zxdh_1588_pd_tx *hdr,          \
                                uint8_t *ptpHdr,                        \
                                struct time_stamps *t5g,                \
                                struct time_stamps *tsn,                \
                                uint32_t *thw,                          \
                                struct zxdh_en_device *en_dev);

int32_t pkt_proc_type_follow_up(struct sk_buff *skb,                    \
                                struct zxdh_1588_pd_tx *hdr,          \
                                uint8_t *ptpHdr,                        \
                                struct time_stamps *t5g,                \
                                struct time_stamps *tsn,                \
                                uint32_t *thw,                          \
                                struct zxdh_en_device *en_dev);

int32_t pkt_proc_type_delay_resp(struct sk_buff *skb,                   \
                                struct zxdh_1588_pd_tx *hdr,          \
                                uint8_t *ptpHdr,                        \
                                struct time_stamps *t5g,                \
                                struct time_stamps *tsn,                \
                                uint32_t *thw,                          \
                                struct zxdh_en_device *en_dev);

int32_t pkt_proc_type_pdelay_resp_follow_up(struct sk_buff *skb,        \
                                            struct zxdh_1588_pd_tx *hdr,   \
                                            uint8_t *ptpHdr,            \
                                            struct time_stamps *t5g,    \
                                            struct time_stamps *tsn,    \
                                            uint32_t *thw,              \
                                            struct zxdh_en_device *en_dev);

int32_t pkt_proc_type_announce(struct sk_buff *skb,                     \
                            struct zxdh_1588_pd_tx *hdr,              \
                            uint8_t *ptpHdr,                            \
                            struct time_stamps *t5g,                    \
                            struct time_stamps *tsn,                    \
                            uint32_t *thw,                              \
                            struct zxdh_en_device *en_dev);

int32_t pkt_proc_type_signaling(struct sk_buff *skb,                    \
                                struct zxdh_1588_pd_tx *hdr,          \
                                uint8_t *ptpHdr,                        \
                                struct time_stamps *t5g,                \
                                struct time_stamps *tsn,                \
                                uint32_t *thw,                          \
                                struct zxdh_en_device *en_dev);

int32_t pkt_proc_type_management(struct sk_buff *skb,                   \
                                struct zxdh_1588_pd_tx *hdr,          \
                                uint8_t *ptpHdr,                        \
                                struct time_stamps *t5g,                \
                                struct time_stamps *tsn,                \
                                uint32_t *thw,                          \
                                struct zxdh_en_device *en_dev);

int32_t pkt_rcv_type_event(struct zxdh_1588_pd_rx *hdr,           \
                          uint8_t *ptpHdr,                              \
                          struct time_stamps *t5g,                      \
                          struct time_stamps *tsn,                      \
                          uint32_t *thw,                                \
                          struct skb_shared_info *ptSkbSharedInfo,
                          struct zxdh_en_device *en_dev);

int32_t pkt_rcv_type_delay_resp(struct zxdh_1588_pd_rx *hdr,      \
                            uint8_t *ptpHdr,                            \
                            struct time_stamps *t5g,                    \
                            struct time_stamps *tsn,                    \
                            uint32_t *thw,                              \
                            struct skb_shared_info *ptSkbSharedInfo,
                            struct zxdh_en_device *en_dev);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EN_1588_PKT_PROC_FUNC_H_ */

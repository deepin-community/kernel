/*****************************************************************************
(C) 2023 ZTE. 版权所有.

文件名      : en_1588_pkt_proc.c
内容摘要    : 提供PTP数据包处理相关接口
版本        : 1.0
*****************************************************************************/

#include "en_1588_pkt_proc.h"
#include "en_aux_ioctl.h"
#include "queue.h"
#include "../en_ethtool/ethtool.h"
#include "../slib.h"

#define PTP_MESSAGE_HRD_LEN         34
#define IPV6_HDR_LEN                40
#define IPV6_PROT_OFFSET            6
#define UDP_DEST_PORT_OFFSET        2
#define VLAN_TPID                   0x8100
#define UDP_PRORT_EVENT_1588        319
#define UDP_PRORT_GENERAL_1588      320

/* pi头中pkt_type字段值 */
#define PTP_EVENT_TYPE_NOSECURITY   2
#define PTP_EVENT_TYPE_SECURITY     3
#define PTP_GENERAL_TYPE            0
#define PTP_TYPE_OFFSET             4
/* 下行层四1588微码是否需要查询ipsec表 */
#define PTP_L4_NEED_QUERY_IPSEC_TABLE   1
#define PTP_TYPE_L4_SECURITY_OFFSET     3

/* L3报文类型 */
#define ETH_TYPE_PTP        0x88f7
#define ETH_TYPE_IPV4       0x0800
#define ETH_TYPE_IPV6       0x86dd

/* L4报文类型 */
#define ETH_TYPE_UDP        0x11
#define ETH_TYPE_TCP        0x06

#define UDP_HDR_LEN         0x08
#define TCP_HDR_LEN         0x14

/* 报文中关键字段的长度 */
#define ETHER_TYPE_LEN      2
#define ETHER_MAC_LEN       6
#define L2_PKT_HDR_LEN      ((2 * ETHER_MAC_LEN) + ETHER_TYPE_LEN)

#define IP_PROT_OFFSET      9 /* IP头中protocol字段的偏移 */

#define PTP_MSG_ERROR_TYPE  0xff
#define PTPHDR_CF_OFFSET    8

#define VLAN_LEN            4
#ifdef PTP_DRIVER_INTERFACE_EN
extern int get_hw_timestamp(struct zxdh_en_device *en_dev, u32 *hwts);
#endif
/* PTP报文类型和处理函数对应关系结构体 */
typedef struct
{
    uint8_t type;
    int32_t (*proc_func)(struct sk_buff *skb,               \
                         struct zxdh_1588_pd_tx *hdr,          \
                         uint8_t *ptpHdr,                   \
                         struct time_stamps *t5g,           \
                         struct time_stamps *tsn,           \
                         uint32_t *thw,                     \
                         struct zxdh_en_device *en_dev);
} MsgProc_t;

typedef struct
{
    uint8_t type;
    int32_t (*proc_func)(struct zxdh_1588_pd_rx *hdr, \
                         uint8_t *ptpHdr,                   \
                         struct time_stamps *t5g,           \
                         struct time_stamps *tsn,           \
                         uint32_t *thw,                     \
                         struct skb_shared_info *ptSkbSharedInfo,
                         struct zxdh_en_device *en_dev);
} MsgRcv_t;

/* PTP报文类型和处理函数对应关系表 */
MsgProc_t g_MsgProcTable[] =
{
    {PTP_MSG_TYPE_SYNC,                  pkt_proc_type_sync                 },
    {PTP_MSG_TYPE_DELAY_REQ,             pkt_proc_type_delay_req            },
    {PTP_MSG_TYPE_PDELAY_REQ,            pkt_proc_type_pdelay_req           },
    {PTP_MSG_TYPE_PDELAY_RESP,           pkt_proc_type_pdelay_resp          },

    {PTP_MSG_TYPE_FOLLOW_UP,             pkt_proc_type_follow_up            },
    {PTP_MSG_TYPE_DELAY_RESP,            pkt_proc_type_delay_resp           },
    {PTP_MSG_TYPE_PDELAY_RESP_FOLLOW_UP, pkt_proc_type_pdelay_resp_follow_up},
    {PTP_MSG_TYPE_ANNOUNCE,              pkt_proc_type_announce             },
    {PTP_MSG_TYPE_SIGNALING,             pkt_proc_type_signaling            },
    {PTP_MSG_TYPE_MANAGEMENT,            pkt_proc_type_management           },

    {PTP_MSG_ERROR_TYPE,                 NULL                               }
};

MsgRcv_t g_MsgRcvTable[] =
{
    {PTP_MSG_TYPE_SYNC,                  pkt_rcv_type_event                 },
    {PTP_MSG_TYPE_DELAY_REQ,             pkt_rcv_type_event                 },
    {PTP_MSG_TYPE_PDELAY_REQ,            pkt_rcv_type_event                 },
    {PTP_MSG_TYPE_PDELAY_RESP,           pkt_rcv_type_event                 },

    {PTP_MSG_TYPE_DELAY_RESP,            pkt_rcv_type_delay_resp            },

    {PTP_MSG_ERROR_TYPE,                 NULL                               }
};

/* 判断是否为事件报文 */
bool is_event_message(const uint8_t msg_type)
{
    if (msg_type <= PTP_MSG_TYPE_PDELAY_RESP)
    {
        return true;
    }
    return false;
}

/* 判断是否为普通报文 */
bool is_general_message(const uint8_t msg_type)
{
    if ((PTP_MSG_TYPE_FOLLOW_UP <= msg_type) && (msg_type <= PTP_MSG_TYPE_MANAGEMENT))
    {
        return true;
    }
    return false;
}

// 处理扩展首部通用逻辑，返回是否成功处理该扩展首部
int process_extension_header(const uint8_t **ptr, uint16_t *remaining_len, uint8_t header_type)
{
    uint16_t ext_len = 0;
    if (*remaining_len == 0)
    {
        DEBUG_1588("Remaining length is 0, can't process extension header.\n");
        return 0;  // 剩余长度为0，无法处理任何扩展首部了
    }

    // 获取扩展首部长度字段（不同扩展首部长度计算方式类似，此处统一处理基础逻辑）
    ext_len = (uint16_t)((*(*ptr)) + 1) * 8;
    DEBUG_1588("Extension header type: %d, declared length: %hu bytes\n", header_type, ext_len);
    if (ext_len > (*remaining_len + 1))
    {
        DEBUG_1588("Extension header length exceeds remaining length, can't process completely.\n");
        return 0;  // 扩展首部声明的长度超过剩余可处理长度，无法完整处理
    }

    // 移动指针并更新剩余长度
    *ptr = *ptr + (ext_len - 1);
    *remaining_len = *remaining_len - (ext_len - 1);
    DEBUG_1588("Successfully processed extension header, new pointer position and remaining length updated.\n");
    return 1;  // 成功处理该扩展首部
}

int is_fragmented_ipv6(struct ipv6hdr *ipv6h)
{
    uint8_t next_header_type = 0;
    const uint8_t *ptr = NULL;
    uint16_t remaining_len = 0;
    uint16_t payload_len = ntohs(ipv6h->payload_len);

    next_header_type = ipv6h->nexthdr;
    if (next_header_type == 44)
    {
        DEBUG_1588("Encountered fragmentation-related next header in IPv6 header, packet is fragmented.\n");
        return 1;  // 如果IPv6报文头的下一报头就是表示分片相关的类型，直接返回1表示是分片
    }

    ptr = (const uint8_t *)ipv6h + sizeof(struct ipv6hdr);
    remaining_len = payload_len;

    while (remaining_len > 0)
    {
        next_header_type = *ptr;
        ptr++;
        remaining_len--;
        DEBUG_1588("Starting to process extension headers, initial remaining length: %hu bytes\n", remaining_len);
        DEBUG_1588("Processing extension header of type: %d\n", next_header_type);

        switch (next_header_type) {
            case 0:  // 逐跳选项首部
            case 60:  // 目的选项首部
            case 43:  // 路由首部
                if (!process_extension_header(&ptr, &remaining_len, next_header_type)) {
                    return 1;  // 若无法完整处理扩展首部，直接返回1表示是分片，1588处理流程不处理
                }
                break;
            case 44:
                DEBUG_1588("Encountered fragmentation-related extension header, packet is fragmented.\n");
                return 1;  // 遇到特定表示分片的扩展首部类型，返回1表示是分片
            default:
                return 0;  // 其他未知扩展首部类型，暂认为不是分片
        }
    }

    return 0;  // 遍历完所有扩展首部后没发现分片相关标识，认为不是分片
}

// 函数用于判断是否是分片报文，返回1表示是分片报文，0表示不是
bool is_fragmented_ipv4(struct iphdr *ipv4h) 
{
    uint16_t mf_flag = 0;
    uint16_t flags_fragment_offset = 0;

    flags_fragment_offset = ntohs(ipv4h->frag_off);

    DEBUG_1588("frag_off:%hu\n", flags_fragment_offset);
    // 获取标志位中的"更多分片（MF）"位，通过与运算提取出对应位的值
    mf_flag = (flags_fragment_offset >> 13) & 0x01;
    DEBUG_1588("mf_flag:%hu\n", mf_flag);
    if (mf_flag != 0)
    {
        return true;
    }

    // MF位为0，再看分片偏移量是否为0，若不为0也是分片（最后一个分片)
    flags_fragment_offset = flags_fragment_offset & 0x1FFF;
    DEBUG_1588("flags_fragment_offset:%hu\n", flags_fragment_offset);
    if (flags_fragment_offset != 0)
    {
        return true;
    }
    return false;
}

/* p得到PTP报文头位置 */
int32_t get_hdr_point(uint8_t *pData, uint8_t *piTs0ffset, uint8_t **ptpHdr)
{
    uint16_t    udp_dest_port_ptp   = 0;
    uint16_t    offset              = 0;
    uint16_t    temp_len            = 0;
    uint16_t    eth_type_lay3       = ntohs(*((uint16_t*)(pData + (2 * ETHER_MAC_LEN))));   /* get Eth Type */
    uint8_t     eth_type_lay4       = 0;
    uint8_t     eth_type_lay4_ipv6  = 0;
    uint16_t    eth_type_vlan_lay3  = ntohs(*((uint16_t*)(pData + (2 * ETHER_MAC_LEN) + VLAN_LEN)));
    struct iphdr *ipv4h = NULL;
    struct ipv6hdr *ipv6h = NULL;

    /* 计算PTP头的偏移 */
    offset = L2_PKT_HDR_LEN;

    if ((VLAN_TPID == eth_type_lay3) && (VLAN_TPID != eth_type_vlan_lay3)) /* 单vlan偏移 */
    {
        offset += VLAN_LEN;
    }
    else if ((VLAN_TPID == eth_type_lay3) && (VLAN_TPID == eth_type_vlan_lay3)) /* 双vlan偏移 */
    {
        offset += (VLAN_LEN * 2);
    }

    eth_type_lay3 = ntohs(*((uint16_t*)(pData + offset - ETHER_TYPE_LEN)));
    eth_type_lay4 = *(pData + offset + IP_PROT_OFFSET);

    eth_type_lay4_ipv6  = *(pData + offset + IPV6_PROT_OFFSET);

    if ((ETH_TYPE_PTP != eth_type_lay3) && (ETH_TYPE_IPV4 != eth_type_lay3) && (ETH_TYPE_IPV6 != eth_type_lay3))
    {
        DEBUG_1588("unknown L3 eth type: %d\n", eth_type_lay3);
        return IS_NOT_PTP_MSG;
    }

    if (ETH_TYPE_IPV4 == eth_type_lay3)
    {
        /* 判断ipv4报文是否分片 */
        ipv4h = (struct iphdr *)(pData + offset);
        if (is_fragmented_ipv4(ipv4h))
        {
            DEBUG_1588("is fragmented ipv4!!\n");
            return IS_NOT_PTP_MSG;
        }

        /* IP首部第一字节: 版本(4b)+首部长度（4b），这里取低4位，长度是以4字节为单位 */
        temp_len = *(pData + offset);
        temp_len = (temp_len & 0x0f) * 4;
        offset += temp_len;

        /* L4类型PTP只有UDP */
        if (ETH_TYPE_UDP == eth_type_lay4)
        {
            udp_dest_port_ptp = ntohs(*(uint16_t *)(pData + offset + UDP_DEST_PORT_OFFSET));
            if ((udp_dest_port_ptp != UDP_PRORT_EVENT_1588) && (udp_dest_port_ptp != UDP_PRORT_GENERAL_1588))
            {
                DEBUG_1588("UDP destination port(%hd) is not 319 or 320!!\n", udp_dest_port_ptp);
                return IS_NOT_PTP_MSG;
            }
            temp_len = UDP_HDR_LEN;
            offset += temp_len;
        }
        else
        {
            DEBUG_1588("eth_type_lay4 = %hhu, is not UDP!!!!!\n", eth_type_lay4);
            return IS_NOT_PTP_MSG;
        }
    }
    else if(ETH_TYPE_IPV6 == eth_type_lay3)
    {
        /* 判断ipv6报文是否分片 */
        ipv6h = (struct ipv6hdr *)(pData + offset);
        if (is_fragmented_ipv6(ipv6h))
        {
            DEBUG_1588("is fragmented ipv6!!\n");
            return IS_NOT_PTP_MSG;
        }

        temp_len = IPV6_HDR_LEN;
        offset += temp_len;

        /* L4类型PTP只有UDP */
        if (ETH_TYPE_UDP == eth_type_lay4_ipv6)
        {
            udp_dest_port_ptp = ntohs(*(uint16_t *)(pData + offset + UDP_DEST_PORT_OFFSET));
            if ((udp_dest_port_ptp != UDP_PRORT_EVENT_1588) && (udp_dest_port_ptp != UDP_PRORT_GENERAL_1588))
            {
                DEBUG_1588("UDP destination port(%hd) is not 319 or 320!!\n", udp_dest_port_ptp);
                return IS_NOT_PTP_MSG;
            }
            temp_len = UDP_HDR_LEN;
            offset += temp_len;
        }
        else
        {
            DEBUG_1588("eth_type_lay4_ipv6 = %hhu, is not UDP!!!!!!\n",eth_type_lay4_ipv6);
            return IS_NOT_PTP_MSG;
        }
    }

    *ptpHdr = pData + offset;

    /* 赋值pd头的ts_offset字段 */
    *piTs0ffset = offset;

    return PTP_SUCCESS;
}

/* 从PTP报文头中解析出报文类型 */
uint8_t get_msgtype_from_hrd(uint8_t *hrd, const uint8_t len)
{
    uint8_t msg_type = PTP_MSG_ERROR_TYPE;

    CHECK_UNEQUAL_ERR(len, PTP_MESSAGE_HRD_LEN, -EFAULT, "error len %d!", len);

    msg_type = hrd[0] & 0x0f;
    if (is_event_message(msg_type) || is_general_message(msg_type))
    {
        return msg_type;
    }

    DEBUG_1588("error message type %d", msg_type);
    return PTP_MSG_ERROR_TYPE;
}

/* 调用PTP模块驱动接口，读取3个时间戳：两个80bit（T1，T2），一个32bit（T3） */
#ifdef    PTP_DRIVER_INTERFACE_EN
extern int get_pkt_timestamp(int32_t clock_no, struct zxdh_en_device *en_dev, struct time_stamps *ts, u32 *hwts);
#endif /* PTP_DRIVER_INTERFACE_EN */

int32_t get_tstamps_from_ptp(int32_t clock_no, struct time_stamps *t5g, struct time_stamps *tsn, uint32_t *thw, struct zxdh_en_device *en_dev)
{ 
    uint32_t hwts                       = 0;
    struct time_stamps ts[2];

#ifdef  PTP_DRIVER_INTERFACE_EN
    int32_t ret                         = 0;
#endif /* PTP_DRIVER_INTERFACE_EN */

    /* 3.10 内核未启用 PTP 时初始化为 0，避免编译器报错 */
    zte_memset_s(ts, 0, sizeof(ts));

#ifdef  PTP_DRIVER_INTERFACE_EN
    ret = get_pkt_timestamp(clock_no, en_dev, ts, &hwts);//todo
    if (unlikely(ret != 0))
    {
        LOG_ERR_DEV(en_dev->parent, "netdev %s get tsn clock %d failed!, ret = %d", en_dev->netdev->name,clock_no,ret);
        return -1;
    }
#endif /* PTP_DRIVER_INTERFACE_EN */

    LOG_DEBUG_DEV(en_dev->parent, "===GET-PTP===: hwts=%u", hwts);
    LOG_DEBUG_DEV(en_dev->parent, "===GET-PTP===: ts[0].s=%llu, ts[0].ns=%u", ts[0].s, ts[0].ns);
    LOG_DEBUG_DEV(en_dev->parent, "===GET-PTP===: ts[1].s=%llu, ts[1].ns=%u", ts[1].s, ts[1].ns);

    *t5g = ts[1];
    *tsn = ts[1];
    *thw = hwts;

    return 0;
}

/* 发送流程中的报文时间戳处理 */
int32_t pkt_1588_proc_xmit(struct sk_buff *skb,  struct zxdh_1588_pd_tx *hdr, int32_t clock_no,
                        struct zxdh_en_device *en_dev, uint8_t *ptpHdr)
{
    struct time_stamps  ts_5g;              /* 5G时间戳，有效值80bit */
    struct time_stamps  ts_tsn;             /* TSN时间戳，有效值80bit */
    uint32_t            ts_thw      = 0;    /* 硬件当前时间戳，有效值32bit */
    uint8_t             ret         = 0;
    uint8_t             i           = 0;
    uint8_t             cnt         = 0;
    uint8_t             msg_type    = 0xff;
    struct ptpHdr_t     *ptPtpHdr   = NULL;

    memset(&ts_5g, 0, sizeof(struct time_stamps));
    memset(&ts_tsn, 0, sizeof(struct time_stamps));

    CHECK_EQUAL_ERR(skb, NULL, -EADDRNOTAVAIL, "skb is NULL!\n");
    CHECK_EQUAL_ERR(hdr, NULL, -EADDRNOTAVAIL, "hdr is NULL!\n");

    ptPtpHdr = (struct ptpHdr_t *)ptpHdr;

    /* 解析PTP报文类型 */
    msg_type = get_msgtype_from_hrd(ptpHdr, PTP_MESSAGE_HRD_LEN);
    if (PTP_MSG_ERROR_TYPE == msg_type)
    {
        DEBUG_1588_DEV(en_dev->parent, "unknow PTP msg type!\n");
        return -EFAULT;
    }

    LOG_DEBUG_DEV(en_dev->parent, "pkt_1588_proc_xmit msg_type %d\n",msg_type);
    /* 如果是事件报文，提取时间戳 */
    if (is_event_message(msg_type))
    {
        ret = get_tstamps_from_ptp(clock_no, &ts_5g, &ts_tsn, &ts_thw, en_dev);
        CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "get tstamps from ptp failed!\n");

        /* ptp_type[2]的低bit2-4表示pkt_type,加密事件报文类型为2，非加密事件报文为3,   */
        hdr->ptp_type[2] = (hdr->ptp_type[2] & 0x8F) + (PTP_EVENT_TYPE_NOSECURITY << PTP_TYPE_OFFSET);
        if (0 != ((ptPtpHdr->flagField) & 0x0080))
        {
            hdr->ptp_type[2] = (hdr->ptp_type[2] & 0x8F) + (PTP_EVENT_TYPE_SECURITY << PTP_TYPE_OFFSET);
        }
    }
    else
    {
        /* 普通报文类型为0 */
        hdr->ptp_type[2] = (hdr->ptp_type[2] & 0x8F) + (PTP_GENERAL_TYPE << PTP_TYPE_OFFSET);
    }
    /* 层四1588报文,下行微码处理时，是否需要查ipsec表, 加密报文需要 */
    if (0 != ((ptPtpHdr->flagField) & 0x0080))
    {
        hdr->ptp_type[2] = (hdr->ptp_type[2] & 0xF7) + (PTP_L4_NEED_QUERY_IPSEC_TABLE << PTP_TYPE_L4_SECURITY_OFFSET);
    }
    /* 层二发送方向的出端口需要这里指示 */
    hdr->port = en_dev->phy_port;

    /* 根据不同报文类型做不同处理 */
    cnt = sizeof(g_MsgProcTable) / sizeof(MsgProc_t);
    for (i = 0; i < cnt; i++)
    {
        if (g_MsgProcTable[i].type == msg_type)
        {
            if (likely(g_MsgProcTable[i].proc_func != NULL))
            {
                ret = g_MsgProcTable[i].proc_func(skb, hdr, ptpHdr, &ts_5g, &ts_tsn, &ts_thw, en_dev);
            }
        }
    }

    return ret;
}

/* 接收流程中的报文时间戳处理 */
int32_t pkt_1588_proc_rcv(struct sk_buff *skb, struct zxdh_1588_pd_rx *hdr, int32_t clock_no, struct zxdh_en_device *en_dev)
{
    struct time_stamps      ts_5g;  /* 5G时间戳，有效值80bit */
    struct time_stamps      ts_tsn; /* TSN时间戳，有效值80bit */
    uint32_t    ts_thw      = 0;    /* 硬件当前时间戳，有效值32bit */
    uint8_t     *pData      = NULL;
    uint8_t     *ptpHdr     = NULL;
    int32_t     ret         = 0;
    uint8_t     i           = 0;
    uint8_t     cnt         = 0;
    uint8_t     msg_type    = 0xff;
    uint8_t     piTsOffset  = 0;

    memset(&ts_5g, 0, sizeof(struct time_stamps));
    memset(&ts_tsn, 0, sizeof(struct time_stamps));

    CHECK_EQUAL_ERR(skb, NULL, -EADDRNOTAVAIL, "skb is NULL!\n");
    CHECK_EQUAL_ERR(hdr, NULL, -EADDRNOTAVAIL, "hdr is NULL!\n");

    pData = skb->data;

    /* 获得ptp报文头指针&赋值pi头ts_offset字段 */
    ret = get_hdr_point(pData, &piTsOffset, &ptpHdr);
    CHECK_EQUAL_ERR(ptpHdr, NULL, -EADDRNOTAVAIL, "get ptp hdr failed!\n");
    if (ret != 0)
    {
        DEBUG_1588_DEV(en_dev->parent, "is not ptp msg or get hdr err!!\n");
        return -EFAULT;
    }

    /* 解析PTP报文类型 */
    msg_type = get_msgtype_from_hrd(ptpHdr, PTP_MESSAGE_HRD_LEN);
    if (PTP_MSG_ERROR_TYPE == msg_type)
    {
        DEBUG_1588_DEV(en_dev->parent, "unknow PTP msg type!\n");
        return -EFAULT;
    }

    /* 如果是事件报文，提取时间戳 */
    if (is_event_message(msg_type))
    {
        ret = get_tstamps_from_ptp(clock_no, &ts_5g, &ts_tsn, &ts_thw, en_dev);
        CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "get tstamps from ptp failed!\n");
    }

    /* 根据不同报文类型做不同处理 */
    cnt = sizeof(g_MsgRcvTable) / sizeof(MsgRcv_t);
    for (i = 0; i < cnt; i++)
    {
        if (g_MsgRcvTable[i].type == msg_type)
        {
            if (likely(g_MsgRcvTable[i].proc_func != NULL))
            {
                ret = g_MsgRcvTable[i].proc_func(hdr, ptpHdr, &ts_5g, &ts_tsn, &ts_thw, skb_shinfo(skb), en_dev);
            }
        }
    }

    return ret;
}

int32_t is_delay_statistics_pkt(uint8_t *pData)
{
    uint16_t    udp_dest_port   = 0;
    uint16_t    offset              = 0;
    uint16_t    temp_len            = 0;
    uint16_t    eth_type_lay3       = ntohs(*((uint16_t*)(pData + (2 * ETHER_MAC_LEN))));   /* get Eth Type */
    uint8_t     eth_type_lay4       = 0;
    uint8_t     eth_type_lay4_ipv6  = 0;
    uint16_t    eth_type_vlan_lay3  = ntohs(*((uint16_t*)(pData + (2 * ETHER_MAC_LEN) + VLAN_LEN)));

    /* 计算PTP头的偏移 */
    offset = L2_PKT_HDR_LEN;

    if ((VLAN_TPID == eth_type_lay3) && (VLAN_TPID != eth_type_vlan_lay3)) /* 单vlan偏移 */
    {
        offset += VLAN_LEN;
    }
    else if ((VLAN_TPID == eth_type_lay3) && (VLAN_TPID == eth_type_vlan_lay3)) /* 双vlan偏移 */
    {
        offset += (VLAN_LEN * 2);
    }

    eth_type_lay3 = ntohs(*((uint16_t*)(pData + offset - ETHER_TYPE_LEN)));
    eth_type_lay4 = *(pData + offset + IP_PROT_OFFSET);

    eth_type_lay4_ipv6  = *(pData + offset + IPV6_PROT_OFFSET);

    if (ETH_TYPE_IPV4 != eth_type_lay3)
    {
        // LOG_ERR("unknown L4 eth type: %d\n", eth_type_lay3);
        return IS_NOT_STATISTICS_PKT;
    }

    if (ETH_TYPE_IPV4 == eth_type_lay3)
    {
        /* IP首部第一字节: 版本(4b)+首部长度（4b），这里取低4位，长度是以4字节为单位 */
        temp_len = *(pData + offset);
        temp_len = (temp_len & 0x0f) * 4;
        offset += temp_len;

        /* L4类型PTP只有UDP */
        if (ETH_TYPE_UDP == eth_type_lay4)
        {
            udp_dest_port = ntohs(*(uint16_t *)(pData + offset + UDP_DEST_PORT_OFFSET));
            if (udp_dest_port != 49184)
            {
                // LOG_ERR("UDP destination port(%hd) is not 49184!!\n", udp_dest_port);
                return IS_NOT_STATISTICS_PKT;
            }
        }
        else
        {
            // LOG_ERR("eth_type_lay4 = %c, is not UDP!!!!!\n", eth_type_lay4);
            return IS_NOT_STATISTICS_PKT;
        }
    }

    return PTP_SUCCESS;
}

/* delay统计报文发送流程中的时间戳处理 */
int32_t pkt_delay_statistics_proc(struct sk_buff *skb, struct pd_net_hdr_tx *pd_hdr, struct zxdh_en_device *en_dev)
{
    uint8_t *pData = NULL;
    uint8_t ret = 0;
    uint32_t ts_thw = 0;

    CHECK_EQUAL_ERR(skb, NULL, -EADDRNOTAVAIL, "skb is NULL!\n");
    CHECK_EQUAL_ERR(pd_hdr, NULL, -EADDRNOTAVAIL, "hdr is NULL!\n");

    pData = skb->data;

    /* 检查是否是delay统计报文: udp端口号：49184 */
    if(IS_NOT_STATISTICS_PKT == is_delay_statistics_pkt(pData))
    {
        return DELAY_STATISTICS_FAILED;
    }
    /* 时延统计使能 */
    pd_hdr->ol_flag |= htons(DELAY_STATISTICS_INSERT_EN_BIT);

#ifdef PTP_DRIVER_INTERFACE_EN
    ret = get_hw_timestamp(en_dev, &ts_thw);
    CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "get_hw_timestamp failed!\n");
#endif
    /*hw的时间戳，写到 PD头的5~8字节:高29位为ns位，低3bits位为小数ns位 */
    *(uint32_t*)(&(pd_hdr->tag_idx)) = htonl(ts_thw << CPU_TX_DECIMAL_NS); /* 大端对齐 */

    return ret;
}

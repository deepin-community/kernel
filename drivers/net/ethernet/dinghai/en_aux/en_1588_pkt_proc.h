/*****************************************************************************
(C) 2023 ZTE. 版权所有.

文件名      : en_1588_pkt_proc.h
内容摘要    : 提供PTP数据包处理相关接口
版本        : 1.0
*****************************************************************************/

#ifndef _EN_1588_PKT_PROC_H_
#define _EN_1588_PKT_PROC_H_

#ifdef __cplusplus
    extern "C" {
#endif /* __cplusplus */

#include "en_1588_pkt_proc_func.h"

#define PTP_SUCCESS 0
#define PTP_FAILED (-1)
#define IS_NOT_PTP_MSG 1
#define IS_NOT_STATISTICS_PKT   1
#define DELAY_STATISTICS_FAILED (-1)

int32_t pkt_1588_proc_xmit(struct sk_buff *skb, struct zxdh_1588_pd_tx *hdr, int32_t clock_no,
                        struct zxdh_en_device *en_dev, uint8_t *ptpHdr);
int32_t pkt_1588_proc_rcv(struct sk_buff *skb, struct zxdh_1588_pd_rx *hdr, int32_t clock_no, struct zxdh_en_device *en_dev);
int32_t pi_1588_net_hdr_add(struct sk_buff *skb, struct zxdh_net_hdr_tx *hdr, int32_t clock_no, struct zxdh_en_device *en_dev);
int32_t pkt_delay_statistics_proc(struct sk_buff *skb, struct pd_net_hdr_tx *pd_hdr, struct zxdh_en_device *en_dev);
int32_t get_hdr_point(uint8_t *pData, uint8_t *piTs0ffset, uint8_t **ptpHdr);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _EN_1588_PKT_PROC_H_ */

/*****************************************************************************
(C) 2023 ZTE. 版权所有.

文件名      : en_1588_pkt_proc_func.c
内容摘要    : 不同数据类型包的处理接口实现
版本        : 1.0
*****************************************************************************/

#include "en_1588_pkt_proc_func.h"
#include "en_aux_cmd.h"
#include "en_aux_ioctl.h"

struct ptp_update_buff tGlobalPtpBuff = {0};

uint64_t htonll(uint64_t u64_host)
{
    uint64_t u64_net = 0;
    uint32_t u32_host_h = 0;
    uint32_t u32_host_l = 0;

    u32_host_l = u64_host & 0xffffffff;
    u32_host_h = (u64_host >> 32) & 0xffffffff;

    u64_net = htonl(u32_host_l);
    u64_net = ( u64_net << 32 ) | htonl(u32_host_h);

    return u64_net;
}

/**
* @brief 计算两时间戳subtraction和minuend之差，并将差值赋值给*ptMinusRet
* @param minuend 高48bit为s位，低32bit为ns位
*/
int32_t bits_80_minus(struct time_stamps subtraction, Bits80_t minuend, struct time_stamps *ptMinusRet)
{
    uint64_t    minusHigh48_s   = 0;
    uint32_t    minusLow32_ns   = 0;

    /* 取出80bits被减数的ns位值和s位值 */
    memcpy((uint8_t *)(&minusHigh48_s), &minuend, S_SIZE);
    memcpy(&minusLow32_ns, (uint8_t *)(&minuend) + S_SIZE, NS_SIZE);

    /* minuend大端 */
    minusHigh48_s = htonll(minusHigh48_s) >> 16;
    minusLow32_ns = htonl(minusLow32_ns);

    /* 如果减数值小于被减数值 */
    if ((subtraction.s < minusHigh48_s) || ((subtraction.s == minusHigh48_s) && (subtraction.ns < minusLow32_ns)))
    {
        LOG_ERR("The difference between the two times is negative！！");
        return PTP_RET_TIME_ERR;
    }

    if (subtraction.ns > minusLow32_ns)
    {
        ptMinusRet->ns = subtraction.ns - minusLow32_ns;    /* 赋值ns位 */
        ptMinusRet->s  = subtraction.s - minusHigh48_s;     /* 赋值s位 */
    }
    else
    {
        ptMinusRet->ns = S_HOLD - (minusLow32_ns - subtraction.ns); /* 赋值ns位 */
        ptMinusRet->s  = subtraction.s - minusHigh48_s - 1;         /* 赋值s位 */
    }

    return PTP_RET_SUCCESS;
}

int32_t pkt_proc_type_sync(struct sk_buff *skb,             \
                         struct zxdh_1588_pd_tx *hdr,     \
                         uint8_t *ptpHdr,                   \
                         struct time_stamps *t5g,           \
                         struct time_stamps *tsn,           \
                         uint32_t *thw,                     \
                         struct zxdh_en_device *en_dev)
{
    struct SkbSharedHwtstamps_t         tShhwtstamps;
    struct time_stamps                  tMinusRet;
    struct skb_shared_hwtstamps         tHwtstamps5g;
    struct skb_shared_hwtstamps         tHwtstampsTsn;
    Bits80_t                            tTsi;
    struct ptpHdr_t * ptPtpHdr          = NULL;
    uint8_t         *pOriginTimeStamp   = NULL;
    uint8_t         majorSdoId          = 0;
    uint32_t        t5gNsBig            = 0;
    uint64_t        t5gSBig             = 0;
    uint32_t        tsnNsBig            = 0;
    uint64_t        tsnSBig             = 0;
    uint32_t        frequency           = 0;
    uint64_t        cfAddedVal          = 0;
    uint8_t         *tsiTlv             = NULL;
    uint32_t        cpuTx_ns            = 0;
    uint32_t        cpuTx_frac_ns       = 0;
    uint64_t        cfNs                = 0;


    ptPtpHdr            = (struct ptpHdr_t *)ptpHdr;
    majorSdoId          = ((ptPtpHdr->majorType) & 0xf0) >> 4;
    pOriginTimeStamp    = ptpHdr + sizeof(struct ptpHdr_t);
    t5gSBig             = (htonll(t5g->s)) >> 16;
    t5gNsBig            = htonl(t5g->ns);
    tsnSBig             = (htonll(tsn->s)) >> 16;
    tsnNsBig            = htonl(tsn->ns);

    memset(&tShhwtstamps, 0, sizeof(struct SkbSharedHwtstamps_t));
    memset(&tHwtstamps5g, 0, sizeof(struct skb_shared_hwtstamps));
    memset(&tHwtstampsTsn, 0, sizeof(struct skb_shared_hwtstamps));
    memset(&tMinusRet, 0, sizeof(struct time_stamps));
    memset(&tTsi, 0, sizeof(Bits80_t));

    /* 解析PTP Header的majorSdoId字段，如果是0，表示PTP消息由1588使用 */
    if (0 == majorSdoId)
    {
        /* 解析Flag字段低一字节bit1，如果为0，则是一步法,为1不做处理 */
        if (0 == ((ptPtpHdr->flagField) & 0x0002))
        {
            memcpy(pOriginTimeStamp, &t5gSBig, S_SIZE);
            memcpy(pOriginTimeStamp + S_SIZE, &t5gNsBig, NS_SIZE);
        }
    }
    else if (1 == majorSdoId) /* 如果是1，表示PTP消息由802.1AS协议使用 */
    {
        /* 解析Flag字段低一字节bit1，如果为0，则是一步法 */
        if (0 == ((ptPtpHdr->flagField) & 0x0002))
        {
            /* tsn时间戳放到Sync报文的originTimestamp字段 */
            memcpy(pOriginTimeStamp, &tsnSBig, S_SIZE);
            memcpy(pOriginTimeStamp + S_SIZE, &tsnNsBig, NS_SIZE);

            if (0 != ((ptPtpHdr->flagField) & 0x8000)) /* 解析Flag字段高一字节bit7，如果为1，做如下处理，为0不做处理*/
            {
                frequency = *(uint32_t *)(ptpHdr + PTPHDR_FREQUENCY_OFFSET);
                frequency = htonl(frequency);/* 频率比 */

                memcpy(&tTsi, ptpHdr + PTPHDR_TSI_OFFSET, sizeof(Bits80_t));

                bits_80_minus(*t5g, tTsi, &tMinusRet);

                /* （*t5g-*tsi）*频率比 计算结果叠加到CF字段ns位，(不会出现CF字段ns位值溢出情况) */
                cfAddedVal  = (tMinusRet.s * S_HOLD + tMinusRet.ns) * frequency;
                memcpy(&cfNs, ptPtpHdr->correctionField, CF_NS_SIZE);
                cfNs        = htonll(cfNs) >> 16;
                cfNs        += cfAddedVal;
                cfNs        = htonll(cfNs) >> 16;
                memcpy(&(ptPtpHdr->correctionField[0]), &cfNs, CF_NS_SIZE);

                /* flagField字段的高1字节bit7清0 */
                ptPtpHdr->flagField = (ptPtpHdr->flagField) & 0x7f;

                /* 清除20 byte tTsi TLV为0 */
                tsiTlv = ptpHdr + PTPHDR_TSI_TLV_OFFSET;
                memset(tsiTlv, 0, PTPHDR_TSI_TLV_LEN);

                /* 把Header中的messageLength值减去20 */
                ptPtpHdr->msglen = htons(ptPtpHdr->msglen);
                ptPtpHdr->msglen -= PTPHDR_TSI_TLV_LEN;
                ptPtpHdr->msglen = htons(ptPtpHdr->msglen);
            }
        }
        else /* 如果为1，则是两步法 */
        {
            /* 解析Flag字段高一字节bit7，如果为1，做如下处理，为0不做处理 */
            if (0 != ((ptPtpHdr->flagField) & 0x8000))
            {
                memcpy(&tTsi, ptpHdr + PTPHDR_TSI_OFFSET, sizeof(Bits80_t));

                bits_80_minus(*t5g, tTsi, &tMinusRet);

                /* （*t5g-*tsi）*频率比 计算结果叠加到CF字段ns位，(不会出现CF字段ns位值溢出情况) */
                frequency = htonl(ptPtpHdr-> msgTypeSpecific);
                cfAddedVal  = (tMinusRet.s * S_HOLD + tMinusRet.ns) * frequency;
                memcpy(&cfNs, ptPtpHdr->correctionField, CF_NS_SIZE);
                cfNs        = htonll(cfNs) >> 16;
                cfNs        += cfAddedVal;
                cfNs        = htonll(cfNs) >> 16;
                memcpy(&(ptPtpHdr->correctionField[0]), &cfNs, CF_NS_SIZE);

                /* 将messagetypespecific清0 */
                memset(&(ptPtpHdr->msgTypeSpecific), 0, sizeof(uint32_t));

                /* flagField字段的高1字节bit7清0 */
                ptPtpHdr->flagField = (ptPtpHdr->flagField) & 0x7f;

                /* 清除20 byte tTsi TLV为0 */
                tsiTlv = ptpHdr + PTPHDR_TSI_TLV_OFFSET_TWO;
                memset(tsiTlv, 0, PTPHDR_TSI_TLV_LEN);

                /* 把Header中的messageLength值减去20 */
                ptPtpHdr->msglen = htons(ptPtpHdr->msglen);
                ptPtpHdr->msglen -= PTPHDR_TSI_TLV_LEN;
                ptPtpHdr->msglen = htons(ptPtpHdr->msglen);
            }
        }
    }

    /*PTPM的32bit的时间戳，写到 PI头的cpu_tx字段:高29位为ns位，低3bits位为小数ns位 */
    cpuTx_frac_ns   = (hdr->cpu_tx) & 0x07;
    cpuTx_ns        = *thw << CPU_TX_DECIMAL_NS;
    hdr->cpu_tx     = htonl(cpuTx_ns + cpuTx_frac_ns); /* 大端对齐 */

    /* 两个80bit时间戳（T1，T2）放到socket的ERR_QUEUE中 */
    tShhwtstamps.ts_5g_t    = *t5g;
    tShhwtstamps.ts_tsn_t   = *tsn;
#ifndef CGS_V5_693
    tHwtstamps5g.hwtstamp   = tShhwtstamps.ts_5g_t.ns + tShhwtstamps.ts_5g_t.s * S_HOLD;
    tHwtstampsTsn.hwtstamp  = tShhwtstamps.ts_tsn_t.ns + tShhwtstamps.ts_tsn_t.s * S_HOLD;
#else
    tHwtstamps5g.hwtstamp   = ktime_set(tShhwtstamps.ts_5g_t.s, tShhwtstamps.ts_5g_t.ns);
    tHwtstampsTsn.hwtstamp  = ktime_set(tShhwtstamps.ts_tsn_t.s, tShhwtstamps.ts_tsn_t.ns);
#endif
    skb_tstamp_tx(skb, &tHwtstamps5g);
#ifdef CGEL_TSTAMP_2_PATCH_EN
    skb_tstamp_tx_2(skb, &tHwtstampsTsn);
#endif /* CGEL_TSTAMP_2_PATCH_EN */

    return PTP_RET_SUCCESS;
}

int32_t delay_and_pdelay_req_proc(struct sk_buff *skb,      \
                                struct zxdh_1588_pd_tx *hdr,   \
                                struct time_stamps *t5g,    \
                                struct time_stamps *tsn,    \
                                uint32_t *thw)
{
    struct SkbSharedHwtstamps_t tShhwtstamps;
    struct skb_shared_hwtstamps tHwtstamps5g;
    struct skb_shared_hwtstamps tHwtstampsTsn;
    uint32_t    cpuTx_ns        = 0;
    uint32_t    cpuTx_frac_ns   = 0;

    memset(&tShhwtstamps, 0, sizeof(struct SkbSharedHwtstamps_t));
    memset(&tHwtstamps5g, 0, sizeof(struct skb_shared_hwtstamps));
    memset(&tHwtstampsTsn, 0, sizeof(struct skb_shared_hwtstamps));
    /*PTPM的32bit的时间戳，写到 PI头的cpu_tx字段:高29位为ns位，低3bits位为小数ns位 */
    cpuTx_frac_ns   = (hdr->cpu_tx) & 0x07;
    cpuTx_ns        = *thw << CPU_TX_DECIMAL_NS;
    hdr->cpu_tx     = htonl(cpuTx_ns + cpuTx_frac_ns);

    tShhwtstamps.ts_5g_t  = *t5g;
    tShhwtstamps.ts_tsn_t = *tsn;

    /* 2个80bit放到socket error queue中 */
#ifndef CGS_V5_693
    tHwtstamps5g.hwtstamp   = tShhwtstamps.ts_5g_t.ns + tShhwtstamps.ts_5g_t.s * S_HOLD;
    tHwtstampsTsn.hwtstamp  = tShhwtstamps.ts_tsn_t.ns + tShhwtstamps.ts_tsn_t.s * S_HOLD;
#else
    tHwtstamps5g.hwtstamp   = ktime_set(tShhwtstamps.ts_5g_t.s, tShhwtstamps.ts_5g_t.ns);
    tHwtstampsTsn.hwtstamp  = ktime_set(tShhwtstamps.ts_tsn_t.s, tShhwtstamps.ts_tsn_t.ns);
#endif
    skb_tstamp_tx(skb, &tHwtstamps5g);
#ifdef CGEL_TSTAMP_2_PATCH_EN
    skb_tstamp_tx_2(skb, &tHwtstampsTsn);
#endif /* CGEL_TSTAMP_2_PATCH_EN */
    return PTP_RET_SUCCESS;
}

int32_t pkt_proc_type_delay_req(struct sk_buff *skb,               \
                                struct zxdh_1588_pd_tx *hdr,     \
                                uint8_t *ptpHdr,                   \
                                struct time_stamps *t5g,           \
                                struct time_stamps *tsn,           \
                                uint32_t *thw,                     \
                                struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    ret = delay_and_pdelay_req_proc(skb, hdr, t5g, tsn, thw);
    return ret;
}

int32_t pkt_proc_type_pdelay_req(struct sk_buff *skb,              \
                                struct zxdh_1588_pd_tx *hdr,     \
                                uint8_t *ptpHdr,                   \
                                struct time_stamps *t5g,           \
                                struct time_stamps *tsn,           \
                                uint32_t *thw,                     \
                                struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    ret = delay_and_pdelay_req_proc(skb, hdr, t5g, tsn, thw);
    return ret;
}

int32_t pkt_proc_type_pdelay_resp(struct sk_buff *skb,             \
                                struct zxdh_1588_pd_tx *hdr,     \
                                uint8_t *ptpHdr,                   \
                                struct time_stamps *t5g,           \
                                struct time_stamps *tsn,           \
                                uint32_t *thw,                     \
                                struct zxdh_en_device *en_dev)
{
    Bits80_t                        tReqReceTs;
    struct SkbSharedHwtstamps_t     tShhwtstamps;
    struct skb_shared_hwtstamps     tHwtstamps5g;
    struct skb_shared_hwtstamps     tHwtstampsTsn;
    struct time_stamps              tMinusRet;
    struct ptpHdr_t *ptPtpHdr       = NULL;
    uint64_t        MinusVal        = 0;
    uint32_t        cpuTx_ns        = 0;
    uint32_t        cpuTx_frac_ns   = 0;
    uint64_t        cfNs            = 0;

    memset(&tReqReceTs, 0, sizeof(Bits80_t));
    memset(&tShhwtstamps, 0, sizeof(struct SkbSharedHwtstamps_t));
    memset(&tMinusRet, 0, sizeof(struct time_stamps));
    memset(&tHwtstamps5g, 0, sizeof(struct skb_shared_hwtstamps));
    memset(&tHwtstampsTsn, 0, sizeof(struct skb_shared_hwtstamps));
    ptPtpHdr        = (struct ptpHdr_t *)ptpHdr;

    /*PTPM的32bit的时间戳，写到 PI头的cpu_tx字段:高29位为ns位，低3bits位为小数ns位 */
    cpuTx_frac_ns   = (hdr->cpu_tx) & 0x07;
    cpuTx_ns        = *thw << CPU_TX_DECIMAL_NS;
    hdr->cpu_tx     = htonl(cpuTx_ns + cpuTx_frac_ns);

    /* 解析Header中flagField的低1字节的bit1，如果是0（一步法） */
    if (0 == (ptPtpHdr->flagField & 0x0002))
    {
        /* 提取requestRecieptTimestamp */
        tReqReceTs = *(Bits80_t *)(ptpHdr + sizeof(struct ptpHdr_t)); /* 记为T2 */

        /* 将*tsn-T2的差值加到CorrectionField字段的高48bit ns位上 */
        bits_80_minus(*tsn, tReqReceTs, &tMinusRet);
        MinusVal = tMinusRet.ns + tMinusRet.s * S_HOLD;
        memcpy(&cfNs, ptPtpHdr->correctionField, CF_NS_SIZE);
        cfNs        = htonll(cfNs) >> 16;
        cfNs        += MinusVal;
        cfNs        = htonll(cfNs) >> 16;
        memcpy(&(ptPtpHdr->correctionField[0]), &cfNs, CF_NS_SIZE);
    }

    tShhwtstamps.ts_5g_t  = *t5g;
    tShhwtstamps.ts_tsn_t = *tsn;

    /* 2个80bit放到socket error queue中 */
#ifndef CGS_V5_693
    tHwtstamps5g.hwtstamp   = tShhwtstamps.ts_5g_t.ns + tShhwtstamps.ts_5g_t.s * S_HOLD;
    tHwtstampsTsn.hwtstamp  = tShhwtstamps.ts_tsn_t.ns + tShhwtstamps.ts_tsn_t.s * S_HOLD;
#else
    tHwtstamps5g.hwtstamp   = ktime_set(tShhwtstamps.ts_5g_t.s, tShhwtstamps.ts_5g_t.ns);
    tHwtstampsTsn.hwtstamp  = ktime_set(tShhwtstamps.ts_tsn_t.s, tShhwtstamps.ts_tsn_t.ns);
#endif
    skb_tstamp_tx(skb, &tHwtstamps5g);
#ifdef  CGEL_TSTAMP_2_PATCH_EN
    skb_tstamp_tx_2(skb, &tHwtstampsTsn);
#endif /* CGEL_TSTAMP_2_PATCH_EN */
    return PTP_RET_SUCCESS;
}

/* 接收方向的事件报文的时间戳处理函数：在1588驱动中对接收方向的事件报文的时间戳处理是一致的 */
int32_t pkt_rcv_type_event(struct zxdh_1588_pd_rx *hdr,           \
                            uint8_t *ptpHdr,                            \
                            struct time_stamps *t5g,                    \
                            struct time_stamps *tsn,                    \
                            uint32_t *thw,                              \
                            struct skb_shared_info *ptSkbSharedInfo,    \
                            struct zxdh_en_device *en_dev)
{
    struct SkbSharedHwtstamps_t tShhwtstamps;
    uint32_t    tsRx            = 0;
    uint32_t    tsRx_ns         = 0;
    uint32_t    tsRx_frac_ns    = 0;
    int32_t     MinusRetThwCpu  = 0;
    uint64_t    temp            = 0x20000000;
    uint32_t    i               = 0;

    memset(&tShhwtstamps, 0, sizeof(struct SkbSharedHwtstamps_t));
    /* cpu_tx高29bits ns位，低3bits小数ns位 */
    tsRx = htonl(hdr->rx_ts);

    tsRx_frac_ns = tsRx & 0x07;
    tsRx_ns = tsRx >> 3;
    // LOG_DEBUG("hdr->rx_ts = %d, tsRx = %d, tsRx_ns = %d\n", hdr->rx_ts, tsRx, tsRx_ns);

    if (tsRx_frac_ns > 4)
    {
        tsRx_ns += 1;
    }
    // LOG_DEBUG("thw = %d, tsRx_ns = %d\n", *thw, tsRx_ns);
    MinusRetThwCpu = (*thw & 0x1fffffff) - tsRx_ns;

    if(MinusRetThwCpu < 0)
    {
        MinusRetThwCpu += temp;
    }

    LOG_DEBUG_DEV(en_dev->parent, "MinusRetThwCpu = %d\n", MinusRetThwCpu);

    tShhwtstamps.ts_5g_t  = *t5g;
    tShhwtstamps.ts_tsn_t = *tsn;

    /* 更新两个80bits时间戳 */
    if (tShhwtstamps.ts_5g_t.ns > MinusRetThwCpu)
    {
        tShhwtstamps.ts_5g_t.ns -= MinusRetThwCpu;
    }
    else
    {
        for (i = 1; i < tShhwtstamps.ts_5g_t.s + 1; i++)
        {
            temp = i * S_HOLD + tShhwtstamps.ts_5g_t.ns;

            if (temp > MinusRetThwCpu)
            {
                tShhwtstamps.ts_5g_t.ns = temp - MinusRetThwCpu;
                tShhwtstamps.ts_5g_t.s -= i;
                break;
            }
        }
        if (temp < MinusRetThwCpu)
        {
            LOG_ERR_DEV(en_dev->parent, "ts_5g_t < MinusRetThwCpu!!!\n");
        }
    }

    if (tShhwtstamps.ts_tsn_t.ns > MinusRetThwCpu)
    {
        tShhwtstamps.ts_tsn_t.ns -= MinusRetThwCpu;
    }
    else
    {
        for (i = 1; i < tShhwtstamps.ts_tsn_t.s + 1; i++)
        {
            temp = i * S_HOLD + tShhwtstamps.ts_tsn_t.ns;
            if(temp > MinusRetThwCpu)
            {
                tShhwtstamps.ts_tsn_t.ns = temp - MinusRetThwCpu;
                tShhwtstamps.ts_tsn_t.s -= i;
                break;
            }
        }
        if (temp < MinusRetThwCpu)
        {
            LOG_ERR_DEV(en_dev->parent, "ts_tsn_t < MinusRetThwCpu!!!\n");
        }
    }

    LOG_DEBUG_DEV(en_dev->parent, "enter in pkt_rcv_type_event!!!!\n");
    LOG_DEBUG_DEV(en_dev->parent, "tShhwtstamps.ts_5g_t.s = %llu, tShhwtstamps.ts_5g_t.ns = %d\n",
                tShhwtstamps.ts_5g_t.s, tShhwtstamps.ts_5g_t.ns);
    LOG_DEBUG_DEV(en_dev->parent, "tShhwtstamps.ts_tsn_t.s = %llu, tShhwtstamps.ts_tsn_t.ns = %d\n",
                tShhwtstamps.ts_tsn_t.s, tShhwtstamps.ts_tsn_t.ns);

    /* 2个80bit放到socket cmsg中。连同报文返回给应用 */
    ptSkbSharedInfo->hwtstamps.hwtstamp = ktime_set(tShhwtstamps.ts_5g_t.s, tShhwtstamps.ts_5g_t.ns);
#ifdef CGEL_TSTAMP_2_PATCH_EN
    ptSkbSharedInfo->hwtstamps2.hwtstamp = ktime_set(tShhwtstamps.ts_tsn_t.s, tShhwtstamps.ts_tsn_t.ns);
#endif /* CGEL_TSTAMP_2_PATCH_EN */
    return PTP_RET_SUCCESS;
}

/**
* @fn read_ts_match_info
* @brief 查询时间戳匹配信息，查询到匹配信息后更新cf字段和本地时间戳信息
* @param msgType ptp事件报文类型
* @return 返回值为0表示查询时间戳匹配信息成功
*/
int32_t read_ts_match_info(uint32_t msgType, uint8_t *ptpHdr)
{
    uint32_t            mssageType      = 0;
    int32_t             cfNum           = 0;
    uint32_t            srcPortIdFifo   = 0;
    uint32_t            sequeIdFifo     = 0;
    struct ptpHdr_t     *ptPtpHdr       = NULL;
    uint32_t            matchInfo       = 0;
    uint8_t             srcPortId       = 0;
    uint64_t            cfVal           = 0;

    ptPtpHdr = (struct ptpHdr_t *)ptpHdr;

    CHECK_EQUAL_ERR(ptPtpHdr, NULL, -EADDRNOTAVAIL, "tPtpBuff is NULL\n");

    srcPortId = *(uint8_t *)(ptPtpHdr->srcPortIdentity + SRCPORTID_LEN - 1); /* 只取srcPortIdentity最后一字节值 */

    for (cfNum = 0; cfNum < tGlobalPtpBuff.cfCount; cfNum++)
    {
        matchInfo = tGlobalPtpBuff.ptpRegInfo[cfNum].matchInfo;
        mssageType = (matchInfo >> MSGTYPE_OFFSET) & 0xf;
        srcPortIdFifo = (matchInfo >> SRCPORTID_OFFSET) & 0xf;
        sequeIdFifo = htons(matchInfo & 0xffff);

        if((mssageType == msgType) &&              \
           (srcPortIdFifo == (srcPortId & 0xf)) && \
           (sequeIdFifo == ptPtpHdr->sequenceId))
        {
            LOG_DEBUG("read the match info successfully!!!\n");
            LOG_DEBUG("mssageType: %u, srcPortIdFifo: %u, sequeIdFifo: %u\n", mssageType, srcPortIdFifo, sequeIdFifo);
            memcpy(&cfVal, &(tGlobalPtpBuff.ptpRegInfo[cfNum].cfVal[0]), CF_SIZE);
            cfVal = htonll(cfVal);
            memcpy(&(ptPtpHdr->correctionField[0]), &cfVal, CF_SIZE);

            /* 将匹配到的信息从本地buff去除 */
            tGlobalPtpBuff.cfCount--;
            if (cfNum == MAX_PTP_REG_INFO_NUM - 1)
            {
                memset(&(tGlobalPtpBuff.ptpRegInfo[cfNum]), 0, sizeof(struct ptp_reg_info));
                return 0;
            }
            memcpy(&(tGlobalPtpBuff.ptpRegInfo[cfNum]), &(tGlobalPtpBuff.ptpRegInfo[cfNum + 1]), \
                    (MAX_PTP_REG_INFO_NUM - cfNum - 1) * sizeof(struct ptp_reg_info));
            memset(&(tGlobalPtpBuff.ptpRegInfo[MAX_PTP_REG_INFO_NUM - 1]), 0, sizeof(struct ptp_reg_info));

            return 0;
        }
    }

    return -1;
}

#ifdef  PTP_DRIVER_INTERFACE_EN
extern int32_t get_event_ts_info(struct zxdh_en_device *en_dev, struct ptp_buff* p_tsInfo, int32_t mac_number);
#endif /* PTP_DRIVER_INTERFACE_EN */

/**
* @fn general_encrypt_msg_proc
* @brief 使用两步法，获取、存储和处理不同的ptp加密事件报文的时间戳信息
* @param msgType ptp事件报文类型
*/
int32_t general_encrypt_msg_proc(uint32_t msgType, uint8_t *ptpHdr, struct zxdh_en_device *en_dev)
{
    int32_t             num             = 0;
    int32_t             macNum          = 0;
    int32_t             ret             = 0;
    struct ptpHdr_t     *ptPtpHdr       = NULL;
    struct ptp_buff     tempBuff;

    memset(&tempBuff, 0, sizeof(struct ptp_buff));
    ptPtpHdr = (struct ptpHdr_t *)ptpHdr;

    /* 判断报文是否是加密报文 */
    if (!(0x0080 == ((ptPtpHdr->flagField) & 0x0080)))
    {
        return ret;
    }

    macNum = zxdh_pf_macpcs_num_get(en_dev);
    if (macNum < 0)
    {
        LOG_ERR_DEV(en_dev->parent, "get mac num %d err, its value should is 0-2!\n", macNum);
        return -1;
    }

    // LOG_INFO("ptp buff:\n ");
    // print_data((uint8_t *)&tGlobalPtpBuff, sizeof(struct ptp_update_buff));

    /* 1、从本地buff查询和处理时间戳匹配信息，并更新本地buff */
    ret = read_ts_match_info(msgType, ptpHdr);

    /* 2、从本地没匹配到信息，则读取FIFO中信息，将读取到的信息更新到本地，重新匹配 */
    if (ret != 0)
    {
        // LOG_INFO("cannot read the matchInfo from the BUFF!---------------");

    #ifdef  PTP_DRIVER_INTERFACE_EN
        ret = get_event_ts_info(en_dev, &tempBuff, macNum);
        CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "read FIFO form ptpDriver failed!!!");
    #endif /* PTP_DRIVER_INTERFACE_EN */

        /* 2.1 将读取到到的FIFO信息，添加到本地全局buff */
        if (tempBuff.cfCount > 0)
        {
            if (tempBuff.cfCount + tGlobalPtpBuff.cfCount < MAX_PTP_REG_INFO_NUM)
            {
                memcpy(&(tGlobalPtpBuff.ptpRegInfo[tGlobalPtpBuff.cfCount]),
                    tempBuff.ptpRegInfo, sizeof(struct ptp_reg_info) * tempBuff.cfCount);

                tGlobalPtpBuff.cfCount += tempBuff.cfCount;
                // LOG_INFO("tGlobalPtpBuff.cfCount: %u\n", tGlobalPtpBuff.cfCount);
            }
            else /* 当超过64组时间戳信息时 */
            {
                num = tempBuff.cfCount + tGlobalPtpBuff.cfCount - MAX_PTP_REG_INFO_NUM;

                /* 丢弃掉最先存在本地的信息(此信息更大的概率匹配不上) */
                memcpy(&(tGlobalPtpBuff.ptpRegInfo[0]), &(tGlobalPtpBuff.ptpRegInfo[num]),
                        sizeof(struct ptp_reg_info) * (MAX_PTP_REG_INFO_NUM - num));
                tGlobalPtpBuff.cfCount -= num;

                /* 添加新的信息到本地 */
                memcpy(&(tGlobalPtpBuff.ptpRegInfo[tGlobalPtpBuff.cfCount]),
                    tempBuff.ptpRegInfo, sizeof(struct ptp_reg_info) * tempBuff.cfCount);
                tGlobalPtpBuff.cfCount = MAX_PTP_REG_INFO_NUM;
            }

            /* 2.2 在更新后的本地全局buff查询和处理匹配信息，并更新本地buff*/
            ret = read_ts_match_info(msgType, ptpHdr);
            CHECK_UNEQUAL_ERR(ret, 0, -EFAULT, "cannot read the matchInfo from the local BUFF!");
        }
    }

    // LOG_INFO("ptp buff:\n ");
    // print_data((uint8_t *)&tGlobalPtpBuff, sizeof(struct ptp_update_buff));

    return ret;
}


int32_t pkt_proc_type_follow_up(struct sk_buff *skb,               \
                                struct zxdh_1588_pd_tx *hdr,     \
                                uint8_t *ptpHdr,                   \
                                struct time_stamps *t5g,           \
                                struct time_stamps *tsn,           \
                                uint32_t *thw,                     \
                                struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    ret = general_encrypt_msg_proc(PTP_MSG_TYPE_SYNC, ptpHdr, en_dev);

    return ret;
}

int32_t pkt_proc_type_delay_resp(struct sk_buff *skb,              \
                                struct zxdh_1588_pd_tx *hdr,     \
                                uint8_t *ptpHdr,                   \
                                struct time_stamps *t5g,           \
                                struct time_stamps *tsn,           \
                                uint32_t *thw,                     \
                                struct zxdh_en_device *en_dev)
{
    return 0;
}

int32_t pkt_rcv_type_delay_resp(struct zxdh_1588_pd_rx *hdr,      \
                            uint8_t *ptpHdr,                            \
                            struct time_stamps *t5g,                    \
                            struct time_stamps *tsn,                    \
                            uint32_t *thw,                              \
                            struct skb_shared_info *ptSkbSharedInfo,    \
                            struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    ret = general_encrypt_msg_proc(PTP_MSG_TYPE_DELAY_REQ, ptpHdr, en_dev);

    return ret;
}

int32_t pkt_proc_type_pdelay_resp_follow_up(struct sk_buff *skb,               \
                                            struct zxdh_1588_pd_tx *hdr,     \
                                            uint8_t *ptpHdr,                   \
                                            struct time_stamps *t5g,           \
                                            struct time_stamps *tsn,           \
                                            uint32_t *thw,                     \
                                            struct zxdh_en_device *en_dev)
{
    int32_t ret = 0;
    ret = general_encrypt_msg_proc(PTP_MSG_TYPE_PDELAY_RESP, ptpHdr, en_dev);

    return ret;
}

int32_t pkt_proc_type_announce(struct sk_buff *skb,            \
                            struct zxdh_1588_pd_tx *hdr,     \
                            uint8_t *ptpHdr,                   \
                            struct time_stamps *t5g,           \
                            struct time_stamps *tsn,           \
                            uint32_t *thw,                     \
                            struct zxdh_en_device *en_dev)
{
    /* 驱动不做处理 */
    return 0;
}

int32_t pkt_proc_type_signaling(struct sk_buff *skb,               \
                                struct zxdh_1588_pd_tx *hdr,     \
                                uint8_t *ptpHdr,                   \
                                struct time_stamps *t5g,           \
                                struct time_stamps *tsn,           \
                                uint32_t *thw,                     \
                                struct zxdh_en_device *en_dev)
{
    /* 驱动不做处理 */
    return 0;
}

int32_t pkt_proc_type_management(struct sk_buff *skb,              \
                                struct zxdh_1588_pd_tx *hdr,          \
                                uint8_t *ptpHdr,                   \
                                struct time_stamps *t5g,           \
                                struct time_stamps *tsn,           \
                                uint32_t *thw,                     \
                                struct zxdh_en_device *en_dev)
{
    /* 驱动不做处理 */
    return 0;
}

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 3snic Technologies Co., Ltd */

#ifndef SSS_NIC_RX_H
#define SSS_NIC_RX_H

#include <linux/types.h>
#include <linux/device.h>
#include <linux/mm_types.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/u64_stats_sync.h>

#include "sss_nic_io.h"
#include "sss_nic_dev_define.h"

#define SSSNIC_HEADER_LEN_TO_BYTE(header)	((header) >> 2)

#define SSSNIC_RQ_CQE_STATUS_CSUM_ERR_SHIFT 0
#define SSSNIC_RQ_CQE_STATUS_NUM_LRO_SHIFT 16
#define SSSNIC_RQ_CQE_STATUS_LRO_PUSH_SHIFT 25
#define SSSNIC_RQ_CQE_STATUS_LRO_ENTER_SHIFT 26
#define SSSNIC_RQ_CQE_STATUS_LRO_INTR_SHIFT 27

#define SSSNIC_RQ_CQE_STATUS_BP_EN_SHIFT 30
#define SSSNIC_RQ_CQE_STATUS_RXDONE_SHIFT 31
#define SSSNIC_RQ_CQE_STATUS_DECRY_PKT_SHIFT 29
#define SSSNIC_RQ_CQE_STATUS_FLUSH_SHIFT 28

#define SSSNIC_RQ_CQE_STATUS_CSUM_ERR_MASK 0xFFFFU
#define SSSNIC_RQ_CQE_STATUS_NUM_LRO_MASK 0xFFU
#define SSSNIC_RQ_CQE_STATUS_LRO_PUSH_MASK 0X1U
#define SSSNIC_RQ_CQE_STATUS_LRO_ENTER_MASK 0X1U
#define SSSNIC_RQ_CQE_STATUS_LRO_INTR_MASK 0X1U
#define SSSNIC_RQ_CQE_STATUS_BP_EN_MASK 0X1U
#define SSSNIC_RQ_CQE_STATUS_RXDONE_MASK 0x1U
#define SSSNIC_RQ_CQE_STATUS_FLUSH_MASK 0x1U
#define SSSNIC_RQ_CQE_STATUS_DECRY_PKT_MASK 0x1U

#define SSSNIC_RQ_CQE_STATUS_GET(val, member) \
	(((val) >> SSSNIC_RQ_CQE_STATUS_##member##_SHIFT) & \
	 SSSNIC_RQ_CQE_STATUS_##member##_MASK)

#define SSSNIC_GET_RQ_CQE_STATUS(rq_desc, id) \
	sss_hw_cpu32((rq_desc)->rx_desc_group[id].cqe->state)

#define SSSNIC_GET_RX_DONE(status) SSSNIC_RQ_CQE_STATUS_GET(status, RXDONE)

bool sss_nic_rx_alloc_dma_page(struct sss_nic_dev *nic_dev,
			       struct sss_nic_rx_desc *rx_desc);
u32 sss_nic_fill_bd_sge(struct sss_nic_rq_desc *rq_desc);
void sss_nic_get_rq_stats(struct sss_nic_rq_desc *rq_desc,
			  struct sss_nic_rq_stats *stats);
int sss_nic_rx_poll(struct sss_nic_rq_desc *rq_desc, int budget);

#endif

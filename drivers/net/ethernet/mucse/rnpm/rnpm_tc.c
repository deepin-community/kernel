// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2022 - 2025 Mucse Corporation. */

#include <linux/netdevice.h>
#include <net/tc_act/tc_gact.h>
#include <net/tc_act/tc_mirred.h>
#include <net/pkt_cls.h>
#include "rnpm_tc_u32_parse.h"
#include "rnpm_tc.h"

static void __maybe_unused rnpm_setup_txr_prio(void __iomem *ioaddr,
					       struct rnpm_ring *tx_ring,
					       int prio)
{
	u16 dma_ring_idx = tx_ring->rnpm_queue_idx;

	rnpm_wr_reg(ioaddr + RNPM_DMA_REG_TX_ARB_DEF_LVL(dma_ring_idx),
		    prio);
}

int rnpm_setup_tx_maxrate(void __iomem *ioaddr, struct rnpm_ring *tx_ring,
			  u64 max_rate, int samples_1sec)
{
	u16 dma_ring_idx = tx_ring->rnpm_queue_idx;

	/* set hardware samping internal 1S */
	rnpm_wr_reg(ioaddr + RNPM_DMA_REG_TX_FLOW_CTRL_TM(dma_ring_idx),
		    samples_1sec);
	rnpm_wr_reg(ioaddr + RNPM_DMA_REG_TX_FLOW_CTRL_TH(dma_ring_idx),
		    max_rate);
	return 0;
}

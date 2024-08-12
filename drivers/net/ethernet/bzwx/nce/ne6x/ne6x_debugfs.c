// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023, Chengdu BeiZhongWangXin Technology Co., Ltd. */

#include <linux/fs.h>
#include <linux/debugfs.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/rtc.h>

#include "ne6x.h"
#include "ne6x_debugfs.h"
#include "ne6x_portmap.h"
#include "ne6x_reg.h"
#include "ne6x_dev.h"
#include "ne6x_txrx.h"
#include "ne6x_arfs.h"

#define NE6X_CQ_TO_OFF_TX(__desc, __idx) \
	(((__desc)->payload.data[3 * (__idx) + 1] << 0) | \
	 ((__desc)->payload.data[3 * (__idx) + 2] << 8))
#define NE6X_CQ_TO_STS_TX(__desc, __idx) ((__desc)->payload.data[3 * (__idx)])

#define NE6X_CQ_TO_LEN_RX(__desc, __idx) \
	(((__desc)->payload.data[5 * (__idx) + 1] << 0) | \
	 ((__desc)->payload.data[5 * (__idx) + 2] << 8))
#define NE6X_CQ_TO_STS_RX(__desc, __idx) ((__desc)->payload.data[5 * (__idx)])
#define NE6X_CQ_TO_OFF_RX(__desc, __idx) \
	(((__desc)->payload.data[5 * (__idx) + 3] << 0) | \
	 ((__desc)->payload.data[5 * (__idx) + 4] << 8))

#define PARA_KEY_STRING             " "
#define ARRAY_P_MAX_COUNT           140
#define HASH_KEY_SIZE               64
#define HASH_DATA_SIZE              64
#define TABLE_WIDHT_BIT_512         512
#define TABLE_WIDHT_BIT_128         128
#define TABLE_WIDHT_BIT_64          64
#define TABLE_WIDHT_BIT_16          16
#define TABLE_WIDHT_BIT_256         256
#define TABLE_WIDHT_BIT_32	    32

#define FRU_CHECK_6ASCII(x)	    (((x) >> 6) == 0x2)
#define ASCII628_BASE               32
#define FRU_6BIT_8BITLENGTH(x)      (((x) * 4) / 3)

static int table_size[] = {
	TABLE_WIDHT_BIT_512,
	TABLE_WIDHT_BIT_64,
	TABLE_WIDHT_BIT_16,
	TABLE_WIDHT_BIT_64,
	TABLE_WIDHT_BIT_256,
	TABLE_WIDHT_BIT_64,
	TABLE_WIDHT_BIT_64,
	TABLE_WIDHT_BIT_32
};

static const struct ne6x_debug_info ne6x_device_info[] = {
	{0xE220, "N5E025P2-PAUA", "25G"},  {0xE22C, "N5E025P2-NAUA", "25G"},
	{0xE221, "N5S025P2-PAUA", "25G"},  {0xE22D, "N5S025P2-NAUA", "25G"},
	{0xEA20, "N6E100P2-PAUA", "100G"}, {0xEA2C, "N6E100P2-NAUA", "100G"},
	{0xEA21, "N6S100P2-PAUA", "100G"}, {0xEA2D, "N6S100P2-NAUA", "100G"},
	{0xD221, "N6S025P2-PDUA", "25G"},  {0xDA21, "N6S100P2-PDUA", "100G"},
	{0x1220, "N5E025P2-PAGA", "25G"},  {0x122C, "N5E025P2-NAGA", "25G"},
	{0x1221, "N5S025P2-PAGA", "25G"},  {0x122D, "N5S025P2-NAGA", "25G"},
	{0x1A20, "N6E100P2-PAGA", "100G"}, {0x1A2C, "N6E100P2-NAGA", "100G"},
	{0x1A21, "N6S100P2-PAGA", "100G"}, {0x1A2D, "N6S100P2-NAGA", "100G"},
	{0x0221, "N6S100P2-NAGA", "100G"}, {0x0A21, "N6S100P2-PDGA", "100G"} };

char *my_strtok(char *p_in_string, char *p_in_delimit, char **pp_out_ret)
{
	static char *p_tmp;
	char *p_strstr = NULL;
	char *ret = NULL;
	int for_index;

	if (!pp_out_ret)
		return NULL;

	*pp_out_ret = NULL;
	if (!p_in_delimit)
		return p_in_string;

	if (p_in_string)
		p_tmp = p_in_string;

	if (!p_tmp)
		return NULL;

	ret = p_tmp;
	p_strstr = strstr(p_tmp, p_in_delimit);
	if (p_strstr) {
		p_tmp = p_strstr + strlen(p_in_delimit);
		for (for_index = 0; for_index < strlen(p_in_delimit); for_index++)
			*(p_strstr + for_index) = '\0';
	} else {
		p_tmp = NULL;
	}

	*pp_out_ret = p_tmp;

	return ret;
}

int my_isdigit(char in_char)
{
	if ((in_char >= '0') && (in_char <= '9'))
		return 1;
	else
		return 0;
}

int my_atoi(char *p_in_string)
{
	int flag = 1;
	int ret = 0;

	while (my_isdigit(p_in_string[0]) == 0)
		p_in_string++;

	if (*(p_in_string - 1) == '-')
		flag = -1;

	while (my_isdigit(p_in_string[0]) != 0) {
		ret *= 10;
		ret += p_in_string[0] - '0';
		if (ret > INT_MAX || ret < INT_MIN)
			return 0;

		p_in_string++;
	}

	if (ret != 0)
		return (flag * ret);
	else
		return 0;
}

static struct dentry *ne6x_dbg_root;
u8 *ne6x_dbg_get_fru_product_part(u8 *buffer, enum fru_product_part part, u8 *len);

void ne6x_dbg_show_queue(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_ring *ring;
	struct ne6x_adapter *adpt;
	u64 head, tail, oft;
	int queue_num = 0;
	int i, j;

	for (i = 0; i < pf->num_alloc_adpt; i++) {
		adpt = pf->adpt[i];

		if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state))
			dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", i);

		dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
		for (j = 0; j < adpt->num_queue; j++) {
			ring = adpt->rx_rings[j];
			queue_num = adpt->base_queue + j;
			if (queue_num < NE6X_PF_VP0_NUM) {
				head = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_RQ_HD_POINTER));
				tail = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_RQ_TAIL_POINTER));
				oft = rd64(&pf->hw, NE6X_VPINT_DYN_CTLN(queue_num, NE6X_RQ_OFST));
			} else {
				head = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_RQ_HD_POINTER));
				tail = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_RQ_TAIL_POINTER));
				oft = rd64_bar4(&pf->hw,
						NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								    NE6X_RQ_OFST));
			}
			dev_info(&pf->pdev->dev, "----RX: Netdev[%d] Queue[%d]: H[0x%04llx], T[0x%04llx], RQ[0x%04llx], idle:%04d, alloc:%04d, use:%04d, clean:%04d\n",
				 i, j, head, tail, oft, NE6X_DESC_UNUSED(ring), ring->next_to_alloc,
				 ring->next_to_use, ring->next_to_clean);
		}

		dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
		for (j = 0; j < adpt->num_queue; j++) {
			ring = adpt->tx_rings[j];
			queue_num = adpt->base_queue + j;
			if (queue_num < NE6X_PF_VP0_NUM) {
				head = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_SQ_HD_POINTER));
				tail = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_SQ_TAIL_POINTER));
				oft = rd64(&pf->hw, NE6X_VPINT_DYN_CTLN(queue_num, NE6X_SQ_OFST));
			} else {
				head = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_SQ_HD_POINTER));
				tail = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_SQ_TAIL_POINTER));
				oft = rd64_bar4(&pf->hw,
						NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								    NE6X_SQ_OFST));
			}
			dev_info(&pf->pdev->dev, "----TX: Netdev[%d] Queue[%d]: H[0x%04llx], T[0x%04llx], SQ[0x%04llx], idle:%04d, use:%04d, clean:%04d\n",
				 i, j, head, tail, oft, NE6X_DESC_UNUSED(ring), ring->next_to_use,
				 ring->next_to_clean);
		}

		dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
		for (j = 0; j < adpt->num_queue; j++) {
			ring = adpt->cq_rings[j];
			queue_num = adpt->base_queue + j;
			if (queue_num < NE6X_PF_VP0_NUM) {
				head = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_CQ_HD_POINTER));
				tail = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_CQ_TAIL_POINTER));
			} else {
				head = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_CQ_HD_POINTER));
				tail = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_CQ_TAIL_POINTER));
			}
			dev_info(&pf->pdev->dev, "----CQ: Netdev[%d] Queue[%d]: H[0x%04llx], T[0x%04llx], idle:%04d, use:%04d, clean:%04d\n",
				 i, j, head, tail, NE6X_DESC_UNUSED(ring), ring->next_to_use,
				 ring->next_to_clean);
		}

		dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
	}
}

void ne6x_dbg_show_ring(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	int i, j, k, l;
	union ne6x_rx_desc *rx_desc;
	struct ne6x_tx_desc *tx_desc;
	struct ne6x_cq_desc *cq_desc;
	struct ne6x_ring *ring;
	struct ne6x_adapter *adpt;

	for (i = 0; i < pf->num_alloc_adpt; i++) {
		adpt = pf->adpt[i];
		if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
			dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", i);
			continue;
		}

		for (j = 0; j < adpt->num_queue; j++) {
			ring = adpt->rx_rings[j];
			for (k = 0; k < ring->count; k++) {
				rx_desc = NE6X_RX_DESC(ring, k);
				if (!rx_desc->wb.u.val)
					/* this descriptor is empty，skip */
					continue;

				dev_info(&pf->pdev->dev, "**** rx_desc[%d], vp[%d], mml[%d], sml[%d], bsa[0x%llx], bma[0x%llx], flag[0x%x], vp[%d], pkt_len[%d]\n",
					 k, rx_desc->w.vp, rx_desc->w.mop_mem_len,
					 rx_desc->w.sop_mem_len, rx_desc->w.buffer_sop_addr,
					 rx_desc->w.buffer_mop_addr, rx_desc->wb.u.val,
					 rx_desc->wb.vp, rx_desc->wb.pkt_len);
			}
		}

		for (j = 0; j < adpt->num_queue; j++) {
			ring = adpt->tx_rings[j];
			for (k = 0; k < ring->count; k++) {
				tx_desc = NE6X_TX_DESC(ring, k);
				if (!tx_desc->buffer_mop_addr)
					/* this descriptor is empty，skip */
					continue;

				dev_info(&pf->pdev->dev, "**** tx_desc[%d], flag[0x%x], vp[%d], et[%d], ch[%d], tt[%d],sopv[%d],eopv[%d],tso[%d],l3chk[%d],l3oft[%d],l4chk[%d],l4oft[%d],pld[%d],mop[%d],sop[%d],mss[%d],mopa[%lld],sopa[%lld]\n",
					 k, tx_desc->u.val, tx_desc->vp, tx_desc->event_trigger,
					 tx_desc->chain, tx_desc->transmit_type, tx_desc->sop_valid,
					 tx_desc->eop_valid, tx_desc->tso, tx_desc->l3_csum,
					 tx_desc->l3_ofst, tx_desc->l4_csum, tx_desc->l4_ofst,
					 tx_desc->pld_ofst, tx_desc->mop_cnt, tx_desc->sop_cnt,
					 tx_desc->mss, tx_desc->buffer_mop_addr,
					 tx_desc->buffer_sop_addr);
			}
		}

		for (j = 0; j < adpt->num_queue; j++) {
			ring = adpt->cq_rings[j];
			for (k = 0; k < ring->count; k++) {
				cq_desc = NE6X_CQ_DESC(ring, k);
				if (!cq_desc->num)
					/* this descriptor is empty，skip */
					continue;

				dev_info(&pf->pdev->dev,
					 "**** cq_desc[%d], vp[%d], ctype[%d], num[%d]\n", k,
					 ring->reg_idx, cq_desc->ctype, cq_desc->num);
				for (l = 0; l < cq_desc->num; l++) {
					if (cq_desc->ctype == 0)
						dev_info(&pf->pdev->dev,
							 "******[TX] %d:%d val:0x%x\n", l,
							 NE6X_CQ_TO_OFF_TX(cq_desc, l),
							 NE6X_CQ_TO_STS_TX(cq_desc, l));
					else
						dev_info(&pf->pdev->dev,
							 "******[RX] %d:%d val:0x%x len:0x%x\n", l,
							 NE6X_CQ_TO_OFF_RX(cq_desc, l),
							 NE6X_CQ_TO_STS_RX(cq_desc, l),
							 NE6X_CQ_TO_LEN_RX(cq_desc, l));
				}
			}
		}
	}
}

void ne6x_dbg_show_txtail(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	int i, j;
	struct ne6x_adapter *adpt;
	struct ne6x_ring *ring;

	for (i = 0; i < pf->num_alloc_adpt; i++) {
		adpt = pf->adpt[i];
		if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
			dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", i);
			continue;
		}

		dev_info(&pf->pdev->dev, "+----------------------------------------------------------------+\n");
		for (j = 0; j < adpt->num_queue; j++) {
			ring = adpt->tx_rings[j];
			dev_info(&pf->pdev->dev,
				 "+ Netdev[%d] TX queue[%d] processed %llx packets\n", i, j,
				 readq(ring->tail + j));
		}
		dev_info(&pf->pdev->dev, "+----------------------------------------------------------------+\n");
	}
}

void ne6x_dbg_show_txq(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_ring *ring;
	struct ne6x_adapter *adpt;
	int i, j;

	for (i = 0; i < pf->num_alloc_adpt; i++) {
		adpt = pf->adpt[i];

		if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
			dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", i);
			continue;
		}

		dev_info(&pf->pdev->dev, "+----------------------------------------------------------------+\n");
		for (j = 0; j < adpt->num_queue; j++) {
			ring = adpt->tx_rings[j];
			dev_info(&pf->pdev->dev,
				 "+ Netdev[%d] TX queue[%d] processed %lld packets\n", i, j,
				 ring->stats.packets);
		}
		dev_info(&pf->pdev->dev, "+----------------------------------------------------------------+\n");
	}
}

void ne6x_dbg_show_rxq(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_ring *ring;
	struct ne6x_adapter *adpt;
	int i, j;

	for (i = 0; i < pf->num_alloc_adpt; i++) {
		adpt = pf->adpt[i];

		if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
			dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", i);
			continue;
		}

		dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
		for (j = 0; j < adpt->num_queue; j++) {
			ring = adpt->rx_rings[j];
			dev_info(&pf->pdev->dev,
				 "+ Netdev[%d] RX queue[%d] processed %lld packets\n", i, j,
				 ring->stats.packets);
		}
		dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
	}
}

void ne6x_dbg_show_cq(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_ring *ring;
	struct ne6x_adapter *adpt;
	int i, j;

	for (i = 0; i < pf->num_alloc_adpt; i++) {
		adpt = pf->adpt[i];

		if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
			dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", i);
			continue;
		}

		dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
		for (j = 0; j < adpt->num_queue; j++) {
			ring = adpt->cq_rings[j];
			dev_info(&pf->pdev->dev,
				 "+ Netdev[%d] CQ queue[%d] processed %lld packets\n", i, j,
				 ring->stats.packets);
		}
		dev_info(&pf->pdev->dev, "--------------------------------------------------------------------------------------------");
	}
}

void ne6x_dbg_clean_queue(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_ring *tx_ring;
	struct ne6x_ring *rx_ring;
	struct ne6x_ring *cq_ring;
	struct ne6x_adapter *adpt;
	int i, j;

	for (i = 0; i < pf->num_alloc_adpt; i++) {
		adpt = pf->adpt[i];

		if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
			dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", i);
			continue;
		}

		for (j = 0; j < adpt->num_queue; j++) {
			tx_ring = adpt->tx_rings[j];
			rx_ring = adpt->rx_rings[j];
			cq_ring = adpt->cq_rings[j];

			memset(&tx_ring->stats, 0, sizeof(struct ne6x_q_stats));
			memset(&tx_ring->tx_stats, 0, sizeof(struct ne6x_txq_stats));

			memset(&rx_ring->stats, 0, sizeof(struct ne6x_q_stats));
			memset(&rx_ring->rx_stats, 0, sizeof(struct ne6x_rxq_stats));

			memset(&cq_ring->stats, 0, sizeof(struct ne6x_q_stats));
			memset(&cq_ring->cq_stats, 0, sizeof(struct ne6x_cq_stats));
		}
		dev_info(&pf->pdev->dev, "---------------------------adpt[%d] all ring cleaned---------------------------------------",
			 i);
	}
}

void ne6x_dbg_show_txring(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_ring *tx_ring;
	struct ne6x_adapter *adpt;
	u64 head, tail, oft;
	int queue_num = 0;
	int i, j;

	dev_info(&pf->pdev->dev, "\n");
	dev_info(&pf->pdev->dev, "+----------------------------tx begin------------------------------+\n");
	for (i = 0; i < pf->num_alloc_adpt; i++) {
		adpt = pf->adpt[i];

		if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
			dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", i);
			continue;
		}

		for (j = 0; j < adpt->num_queue; j++) {
			tx_ring = adpt->tx_rings[j];
			queue_num = adpt->base_queue + j;
			if (queue_num < NE6X_PF_VP0_NUM) {
				head = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_SQ_HD_POINTER));
				tail = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_SQ_TAIL_POINTER));
				oft = rd64(&pf->hw, NE6X_VPINT_DYN_CTLN(queue_num, NE6X_SQ_OFST));
			} else {
				head = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_SQ_HD_POINTER));
				tail = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_SQ_TAIL_POINTER));
				oft = rd64_bar4(&pf->hw,
						NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								    NE6X_SQ_OFST));
			}
			dev_info(&pf->pdev->dev, "---- Netdev[%d] Queue[%02d]: H[0x%04llx], T[0x%04llx], SQ[0x%04llx], idle:%04d, use:%04d, clean:%04d, busy:%lld\n",
				 i, j, head, tail, oft, NE6X_DESC_UNUSED(tx_ring),
				 tx_ring->next_to_use, tx_ring->next_to_clean,
				 tx_ring->tx_stats.tx_busy);
		}
	}
	dev_info(&pf->pdev->dev, "+----------------------------tx end--------------------------------+\n");
	dev_info(&pf->pdev->dev, "\n");
}

void ne6x_dbg_show_rxring(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_ring *rx_ring;
	struct ne6x_adapter *adpt;
	u64 head, tail, oft;
	int queue_num = 0;
	int i, j;

	dev_info(&pf->pdev->dev, "\n");
	dev_info(&pf->pdev->dev, "+----------------------------rx begin------------------------------+\n");
	for (i = 0; i < pf->num_alloc_adpt; i++) {
		adpt = pf->adpt[i];

		if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
			dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", i);
			continue;
		}

		for (j = 0; j < adpt->num_queue; j++) {
			rx_ring = adpt->rx_rings[j];
			queue_num = adpt->base_queue + j;
			if (queue_num < NE6X_PF_VP0_NUM) {
				head = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_RQ_HD_POINTER));
				tail = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_RQ_TAIL_POINTER));
				oft = rd64(&pf->hw, NE6X_VPINT_DYN_CTLN(queue_num, NE6X_RQ_OFST));
			} else {
				head = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_RQ_HD_POINTER));
				tail = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_RQ_TAIL_POINTER));
				oft = rd64_bar4(&pf->hw,
						NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								    NE6X_RQ_OFST));
			}
			dev_info(&pf->pdev->dev, "---- Netdev[%d] Queue[%02d]: H[0x%04llx], T[0x%04llx], RQ[0x%04llx], alloc:%04d, use:%04d, clean:%04d, cq_expect:%04d\n",
				 i, j, head, tail, oft, rx_ring->next_to_alloc,
				 rx_ring->next_to_use, rx_ring->next_to_clean,
				 rx_ring->cq_last_expect);
		}
	}
	dev_info(&pf->pdev->dev, "+----------------------------rx end--------------------------------+\n");
	dev_info(&pf->pdev->dev, "\n");
}

void ne6x_dbg_show_cqring(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_ring *cq_ring;
	struct ne6x_adapter *adpt;
	int queue_num = 0;
	u64 head, tail;
	int i, j;

	dev_info(&pf->pdev->dev, "\n");
	dev_info(&pf->pdev->dev, "+----------------------------cq begin------------------------------+\n");
	for (i = 0; i < pf->num_alloc_adpt; i++) {
		adpt = pf->adpt[i];

		if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
			dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", i);
			continue;
		}

		for (j = 0; j < adpt->num_queue; j++) {
			cq_ring = adpt->cq_rings[j];
			queue_num = adpt->base_queue + j;
			if (queue_num < NE6X_PF_VP0_NUM) {
				head = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_CQ_HD_POINTER));
				tail = rd64(&pf->hw,
					    NE6X_VPINT_DYN_CTLN(queue_num, NE6X_CQ_TAIL_POINTER));
			} else {
				head = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_RQ_HD_POINTER));
				tail = rd64_bar4(&pf->hw,
						 NE6X_PFINT_DYN_CTLN(queue_num - NE6X_PF_VP0_NUM,
								     NE6X_RQ_TAIL_POINTER));
			}
			dev_info(&pf->pdev->dev, "---- Netdev[%d] Queue[%02d]: H[0x%04llx], T[0x%04llx], idle:%04d, use:%04d, clean:%04d\n",
				 i, j, head, tail, NE6X_DESC_UNUSED(cq_ring), cq_ring->next_to_use,
				 cq_ring->next_to_clean);
		}
	}
	dev_info(&pf->pdev->dev, "+----------------------------cq end--------------------------------+\n");
	dev_info(&pf->pdev->dev, "\n");
}

void ne6x_dbg_show_txdesc_states(int adpt_num, int queue_num, struct ne6x_pf *pf)
{
	struct ne6x_tx_desc *tx_desc = NULL;
	struct ne6x_ring *tx_ring = NULL;
	struct ne6x_adapter *adpt = NULL;
	int i;

	if (adpt_num > pf->num_alloc_adpt) {
		dev_warn(&pf->pdev->dev, "<adpt_num> error\n");
		return;
	}
	adpt = pf->adpt[adpt_num];

	if (queue_num > adpt->num_queue) {
		dev_warn(&pf->pdev->dev, "<queue_num> error\n");
		return;
	}

	if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
		dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", adpt_num);
		return;
	}

	tx_ring = adpt->tx_rings[queue_num];

	dev_info(&pf->pdev->dev, "\n");
	dev_info(&pf->pdev->dev, "+-----------------------------------Netdev[%d] - Queue[%d] - tx_desc begin-----------------------------------------+\n",
		 adpt_num, queue_num);
	for (i = 0; i < tx_ring->count; i++) {
		tx_desc = NE6X_TX_DESC(tx_ring, i);
		if (!tx_desc->buffer_mop_addr && i != 0)
			/* this descriptor is empty，skip */
			continue;

		dev_info(&pf->pdev->dev, "tx_desc[%d]\n", i);
		dev_info(&pf->pdev->dev, "struct ne6x_tx_desc\n"
		       "{\n"
		       "    u8 flags         : 8;	[0x%x]\n"
		       "    u8 vp            : 7;	[%d]\n"
		       "    u8 event_trigger : 1;	[%d]\n"
		       "    u8 chain         : 1;	[%d]\n"
		       "    u8 transmit_type : 2;	[%d]\n"
		       "    u8 sop_valid     : 1;	[%d]\n"
		       "    u8 eop_valid     : 1;	[%d]\n"
		       "    u8 tso           : 1;	[%d]\n"
		       "    u8 l3_csum       : 1;	[%d]\n"
		       "    u8 l3_ofst       : 7;	[%d]\n"
		       "    u8 l4_csum       : 1;	[%d]\n"
		       "    u8 l4_ofst       : 7;	[%d]\n"
		       "    u8 pld_ofst;			[%d]\n"
		       "    __le64 mop_cnt : 24;		[%d]\n"
		       "    __le64 sop_cnt : 16;		[%d]\n"
		       "    __le64 mss     : 16;		[%d]\n"
		       "    __le64 buffer_mop_addr; [%lld]\n"
		       "    __le64 buffer_sop_addr;	[%lld]\n"
		       "};\n",
		       tx_desc->u.val, tx_desc->vp, tx_desc->event_trigger, tx_desc->chain,
		       tx_desc->transmit_type, tx_desc->sop_valid, tx_desc->eop_valid, tx_desc->tso,
		       tx_desc->l3_csum, tx_desc->l3_ofst, tx_desc->l4_csum, tx_desc->l4_ofst,
		       tx_desc->pld_ofst, tx_desc->mop_cnt, tx_desc->sop_cnt, tx_desc->mss,
		       tx_desc->buffer_mop_addr, tx_desc->buffer_sop_addr);
	}
	dev_info(&pf->pdev->dev, "+------------------------------------------------Netdev[%d] - Queue[%d] - tx_desc end--------------------------------------------------+\n",
		 adpt_num, queue_num);
	dev_info(&pf->pdev->dev, "\n");
}

void ne6x_dbg_show_rxdesc_states(int adpt_num, int queue_num, struct ne6x_pf *pf)
{
	union ne6x_rx_desc *rx_desc = NULL;
	struct ne6x_ring *rx_ring = NULL;
	struct ne6x_adapter *adpt = NULL;
	int i;

	if (adpt_num > pf->num_alloc_adpt) {
		dev_warn(&pf->pdev->dev, "<adapter_num> error\n");
		return;
	}
	adpt = pf->adpt[adpt_num];

	if (queue_num > adpt->num_queue) {
		dev_warn(&pf->pdev->dev, "<queue_num> error\n");
		return;
	}

	if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
		dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", adpt_num);
		return;
	}
	rx_ring = adpt->rx_rings[queue_num];

	dev_info(&pf->pdev->dev, "\n");
	dev_info(&pf->pdev->dev, "+-------------------------------------------------Netdev[%d] - Queue[%2d] - rx_desc begin-------------------------------------------------+\n",
		 adpt_num, queue_num);
	for (i = 0; i < rx_ring->count; i++) {
		rx_desc = NE6X_RX_DESC(rx_ring, i);

		if (!rx_desc->wb.u.val)
			/* this descriptor is empty，skip */
			continue;

		dev_info(&pf->pdev->dev, "**** Netdev[%d], Queue[%02d], rx_desc[%d], vp[%d], mml[%d], sml[%d], bsa[0x%llx], bma[0x%llx], flag[0x%x], vp[%d], p[0x%02x%02x%02x%02x%02x%02x%02x%02x], pkt_len[%d]\n",
			 adpt_num, queue_num, i, rx_desc->w.vp, rx_desc->w.mop_mem_len,
			 rx_desc->w.sop_mem_len, rx_desc->w.buffer_sop_addr,
			 rx_desc->w.buffer_mop_addr, rx_desc->wb.u.val, rx_desc->wb.vp,
			 rx_desc->wb.pd[0], rx_desc->wb.pd[1], rx_desc->wb.pd[2], rx_desc->wb.pd[3],
			 rx_desc->wb.pd[4], rx_desc->wb.pd[5], rx_desc->wb.pd[6], rx_desc->wb.pd[7],
			 rx_desc->wb.pkt_len);
	}
	dev_info(&pf->pdev->dev, "+-------------------------------------------------Netdev[%d] - Queue[%d] - rx_desc end----------------------------------------------------+\n",
		 adpt_num, queue_num);
	dev_info(&pf->pdev->dev, "\n");
}

void ne6x_dbg_show_cqdesc_states(int adpt_num, int queue_num, struct ne6x_pf *pf)
{
	struct ne6x_cq_desc *cq_desc = NULL;
	struct ne6x_ring *cq_ring = NULL;
	struct ne6x_adapter *adpt = NULL;
	int i, j;

	if (adpt_num > pf->num_alloc_adpt) {
		dev_warn(&pf->pdev->dev, "<adpt_num> error\n");
		return;
	}
	adpt = pf->adpt[adpt_num];

	if (queue_num > adpt->num_queue) {
		dev_warn(&pf->pdev->dev, "<queue_num> error\n");
		return;
	}

	if (test_bit(NE6X_ADPT_DOWN, adpt->comm.state)) {
		dev_warn(&pf->pdev->dev, "**-- Netdev[%d] is link down --**\n", adpt_num);
		return;
	}
	cq_ring = adpt->cq_rings[queue_num];

	dev_info(&pf->pdev->dev, "\n");
	dev_info(&pf->pdev->dev, "+--------------------------------------------------Netdev[%d] - Queue[%d] - cq_desc begin------------------------------------------------+\n",
		 adpt_num, queue_num);
	for (i = 0; i < cq_ring->count; i++) {
		cq_desc = NE6X_CQ_DESC(cq_ring, i);

		if (!cq_desc->num)
			/* this descriptor is empty，skip */
			continue;

		dev_info(&pf->pdev->dev, "**** Netdev[%d], Queue[%02d], cq_desc[%d], vp[%d], ctype[%s], num[%d]\n",
			 adpt_num, queue_num, i, cq_ring->reg_idx,
			 cq_desc->ctype == 0 ? "tx" : "rx",
			 cq_desc->num);
		for (j = 0; j < cq_desc->num; j++) {
			if (cq_desc->ctype == 0)
				dev_info(&pf->pdev->dev, "******TX%d[%d]: val:0x%x\n", j,
					 NE6X_CQ_TO_OFF_TX(cq_desc, j),
					 NE6X_CQ_TO_STS_TX(cq_desc, j));
			else
				dev_info(&pf->pdev->dev, "******RX%d[%d]: val:0x%x len:%d\n", j,
					 NE6X_CQ_TO_OFF_RX(cq_desc, j),
					 NE6X_CQ_TO_STS_RX(cq_desc, j),
					 NE6X_CQ_TO_LEN_RX(cq_desc, j));
		}
	}
	dev_info(&pf->pdev->dev, "+--------------------------------------------------Netdev[%d] - Queue[%d] - cq_desc end--------------------------------------------------+\n",
		 adpt_num, queue_num);
	dev_info(&pf->pdev->dev, "\n");
}

#ifdef CONFIG_RFS_ACCEL
void ne6x_dbg_show_arfs_cnt(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u8 idx = 0;
	struct ne6x_adapter *pf_adpt;
	struct ne6x_arfs_active_fltr_cntrs *fltr_cntrs = NULL;

	ne6x_for_each_pf(pf, idx) {
		pf_adpt = pf->adpt[idx];
		fltr_cntrs = pf_adpt->arfs_fltr_cntrs;
		dev_info(&pf->pdev->dev, "+---------------------------+\n");
		dev_info(&pf->pdev->dev, "pf_num:%d totle_num:%d\n\t\t\t tcp_v4_num:%d\n\t\t\t udp_v4_num:%d\n\t\t\t tcp_v6_num:%d\n\t\t\t udp_v6_num:%d\n",
			 idx, (atomic_read(&fltr_cntrs->active_tcpv4_cnt) +
			 atomic_read(&fltr_cntrs->active_udpv4_cnt) +
			 atomic_read(&fltr_cntrs->active_tcpv6_cnt) +
			 atomic_read(&fltr_cntrs->active_udpv6_cnt)),
			 atomic_read(&fltr_cntrs->active_tcpv4_cnt),
			 atomic_read(&fltr_cntrs->active_udpv4_cnt),
			 atomic_read(&fltr_cntrs->active_tcpv6_cnt),
			 atomic_read(&fltr_cntrs->active_udpv6_cnt));
		dev_info(&pf->pdev->dev, "+---------------------------+\n");
	}
}
#endif

extern u32 ne6x_dev_crc32(const u8 *buf, u32 size);

void ne6x_dbg_apb_read(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u64 offset;
	u32 value;
	u32 addr;
	int cnt;

	cnt = sscanf(&cmd_buf[0], "%i", &addr);
	if (cnt != 1) {
		dev_warn(&pf->pdev->dev, "apb_read <offset>\n");
		return;
	}

	offset = addr;
	value = ne6x_reg_apb_read(pf, offset);
	dev_info(&pf->pdev->dev, "offset = 0x%08X 0x%08X\n", addr, value);
}

void ne6x_dbg_apb_write(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u64 offset;
	u32 value;
	u32 addr;
	int cnt;

	cnt = sscanf(&cmd_buf[0], "%i %i", &addr, &value);
	if (cnt != 2) {
		dev_warn(&pf->pdev->dev, "apb_write <offset> <value>\n");
		return;
	}

	offset = addr;
	ne6x_reg_apb_write(pf, offset, value);
	dev_info(&pf->pdev->dev, "apb_write: 0x%llx = 0x%x\n", offset, value);
}

void ne6x_dbg_mem_read(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	int index = 0, cnt;
	u32 *reg_data;
	u64 offset;
	u32 addr;
	u32 size;

	cnt = sscanf(&cmd_buf[0], "%i %i", &addr, &size);
	if (cnt != 2) {
		dev_warn(&pf->pdev->dev, "mem_read <offset> <size>\n");
		return;
	}

	reg_data = kzalloc((size + 4) * 4, GFP_KERNEL);
	offset = addr;
	for (index = 0x00; index < size; index++)
		reg_data[index] = ne6x_reg_apb_read(pf, offset + index * 4);

	for (index = 0x00; index < size / 4; index++)
		dev_info(&pf->pdev->dev, "%lx: %08X %08X %08X %08X\n",
			 (unsigned int long)(offset + index * 16), reg_data[4 * index],
			 reg_data[4 * index + 1], reg_data[4 * index + 2], reg_data[4 * index + 3]);

	if ((size % 4) == 1)
		dev_info(&pf->pdev->dev, "%lx: %08X\n", (unsigned int long)(offset + index * 16),
			 reg_data[4 * index]);
	else if ((size % 4) == 2)
		dev_info(&pf->pdev->dev, "%lx: %08X %08X\n",
			 (unsigned int long)(offset + index * 16), reg_data[4 * index],
			 reg_data[4 * index + 1]);
	else if ((size % 4) == 3)
		dev_info(&pf->pdev->dev, "%lx: %08X %08X %08X\n",
			 (unsigned int long)(offset + index * 16), reg_data[4 * index],
			 reg_data[4 * index + 1], reg_data[4 * index + 2]);

	kfree((void *)reg_data);
}

void ne6x_dbg_mem_write(struct ne6x_pf *pf, char *cmd_buf, int count) {}

void ne6x_dbg_templ_help(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	dev_info(&pf->pdev->dev, "HW_FEATURES		= 0\n");
	dev_info(&pf->pdev->dev, "HW_FLAGS		= 1\n");
	dev_info(&pf->pdev->dev, "RSS_TABLE_SIZE	= 2\n");
	dev_info(&pf->pdev->dev, "RSS_TABLE_ENTRY_WIDTH = 3\n");
	dev_info(&pf->pdev->dev, "RSS_HASH_KEY_BLOCK_SIZE = 4\n");
	dev_info(&pf->pdev->dev, "PORT2PI_0		= 5\n");
	dev_info(&pf->pdev->dev, "PI2PORT_0		= 25\n");
	dev_info(&pf->pdev->dev, "VLAN_TYPE		= 33\n");
	dev_info(&pf->pdev->dev, "PI0_BROADCAST_LEAF = 37\n");
	dev_info(&pf->pdev->dev, "PORT_OLFLAGS_0	= 53\n");
	dev_info(&pf->pdev->dev, "PORT_2_COS_0		= 121\n");
	dev_info(&pf->pdev->dev, "VPORT0_LINK_STATUS	= 155\n");
	dev_info(&pf->pdev->dev, "TSO_CKSUM_DISABLE	= 156\n");
	dev_info(&pf->pdev->dev, "PORT0_MTU		= 157\n");
	dev_info(&pf->pdev->dev, "PORT0_QINQ		= 161\n");
	dev_info(&pf->pdev->dev, "CQ_SIZE		= 229\n");
}

void ne6x_dbg_templ_read(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u32 vport;
	u32 value;
	u32 type;
	int cnt;

	cnt = sscanf(&cmd_buf[0], "%i %i", &vport, &type);
	if (cnt != 2) {
		dev_warn(&pf->pdev->dev, "temp_read <vport> <type>\n");
		return;
	}

	ne6x_reg_get_user_data(pf, vport + type, &value);
	dev_info(&pf->pdev->dev, "temp_read  0x%04X value 0x%08X\n", type, value);
}

void ne6x_dbg_templ_write(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u32 vport;
	u32 value;
	u32 type;
	int cnt;

	cnt = sscanf(&cmd_buf[0], "%i %i %i", &vport, &type, &value);
	if (cnt != 3) {
		dev_warn(&pf->pdev->dev, "temp_write <vport> <type> <value>\n");
		return;
	}

	ne6x_reg_set_user_data(pf, vport + type, value);
	dev_info(&pf->pdev->dev, "temp_write: 0x%04x = 0x%x\n", type, value);
}

void ne6x_dbg_soc_read(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u32 value;
	u32 addr;
	int cnt;

	cnt = sscanf(&cmd_buf[0], "%i", &addr);
	if (cnt != 1) {
		dev_warn(&pf->pdev->dev, "soc_read <offset>\n");
		return;
	}

	ne6x_reg_indirect_read(pf, addr, &value);
	dev_info(&pf->pdev->dev, "offset = 0x%08X 0x%08X\n", addr, value);
}

void ne6x_dbg_soc_write(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u32 value;
	u32 addr;
	int cnt;

	cnt = sscanf(&cmd_buf[0], "%i %i", &addr, &value);
	if (cnt != 2) {
		dev_warn(&pf->pdev->dev, "soc_write <offset> <value>\n");
		return;
	}

	ne6x_reg_indirect_write(pf, addr, value);
	dev_info(&pf->pdev->dev, "soc_write: 0x%08X = 0x%08X\n", addr, value);
}

void ne6x_dbg_tab_read(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	int array_index = 0, ret, index;
	struct ne6x_debug_table *table_info;
	u8 *p_str_array[10] = {0};
	u8 *p_in_string = NULL;
	char *p_tmp_ret = NULL;

	table_info = kzalloc(sizeof(*table_info), GFP_KERNEL);
	memset(table_info, 0, sizeof(*table_info));

	p_in_string = &cmd_buf[0];
	while ((p_str_array[array_index] = my_strtok(p_in_string, PARA_KEY_STRING, &p_tmp_ret)) !=
	       NULL) {
		p_in_string = p_str_array[array_index] + strlen(p_str_array[array_index]) + 1;
		array_index++;
		if (array_index >= 10)
			break;

		if (!p_tmp_ret)
			break;
	}

	if (array_index < 2) {
		dev_warn(&pf->pdev->dev, "tab_read <table> <index>\n");
		kfree(table_info);
		return;
	}

	/* table */
	if (!strncmp(p_str_array[0], "0x", 2))
		table_info->table = strtoul(p_str_array[0], 0, 16);
	else
		table_info->table = my_atoi(p_str_array[0]);

	/* index */
	if (!strncmp(p_str_array[1], "0x", 2))
		table_info->index = strtoul(p_str_array[1], 0, 16);
	else
		table_info->index = my_atoi(p_str_array[1]);

	table_info->size = table_size[table_info->table];
	ret = ne6x_reg_table_read(pf, table_info->table, table_info->index,
				  (u32 *)&table_info->data[0], table_info->size);
	dev_info(&pf->pdev->dev, "%s: %s\n", __func__, (ret == 0) ? "success" : "timeout!");

	for (index = 0x00; index < (table_info->size >> 2) / 4; index++)
		dev_info(&pf->pdev->dev, "%08X: %08X %08X %08X %08X\n", index * 16,
			 table_info->data[4 * index], table_info->data[4 * index + 1],
			 table_info->data[4 * index + 2], table_info->data[4 * index + 3]);

	if (((table_info->size >> 2) % 4) == 1)
		dev_info(&pf->pdev->dev, "%08X: %08X\n", index * 16, table_info->data[4 * index]);
	else if (((table_info->size >> 2) % 4) == 2)
		dev_info(&pf->pdev->dev, "%08X: %08X %08X\n", index * 16,
			 table_info->data[4 * index], table_info->data[4 * index + 1]);
	else if (((table_info->size >> 2) % 4) == 3)
		dev_info(&pf->pdev->dev, "%08X: %08X %08X %08X\n", index * 16,
			 table_info->data[4 * index], table_info->data[4 * index + 1],
			 table_info->data[4 * index + 2]);

	kfree(table_info);
}

void ne6x_dbg_set_mac_to_eeprom(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_dev_eeprom_info *psdk_spd_info = &pf->sdk_spd_info;
	u8 mac_addr[6];
	int port = 0;
	int ret;
	int cnt;

	if (strncmp(cmd_buf, "P0", 2) == 0) {
		port = 0;
	} else if (strncmp(cmd_buf, "P1", 2) == 0) {
		port = 1;
	} else {
		dev_warn(&pf->pdev->dev, "set_port_mac P0/P1 macaddr\n");
		dev_warn(&pf->pdev->dev, "example-- set_port_mac P0 94:f5:21:00:00:01\n");
		return;
	}

	cnt = sscanf(&cmd_buf[2], "%hhX:%hhX:%hhX:%hhX:%hhX:%hhX", &mac_addr[0], &mac_addr[1],
		     &mac_addr[2], &mac_addr[3], &mac_addr[4], &mac_addr[5]);
	if (cnt != 6) {
		dev_warn(&pf->pdev->dev, "set_port_mac P0/P1 macaddr\n");
		dev_warn(&pf->pdev->dev, "example-- set_port_mac P0 94:f5:24:00:00:01\n");
		return;
	}

	if (port == 0)
		memcpy(&psdk_spd_info->port_0_mac, &mac_addr, 6);
	else if (port == 1)
		memcpy(&psdk_spd_info->port_1_mac, &mac_addr, 6);
	else if (port == 2)
		memcpy(&psdk_spd_info->port_2_mac, &mac_addr, 6);
	else if (port == 3)
		memcpy(&psdk_spd_info->port_3_mac, &mac_addr, 6);

	psdk_spd_info->spd_verify_value =
		cpu_to_be32(ne6x_dev_crc32((const u8 *)psdk_spd_info,
					   sizeof(*psdk_spd_info) - 4));
	ret = ne6x_dev_write_eeprom(pf->adpt[0], 0x0, (u8 *)psdk_spd_info,
				    sizeof(*psdk_spd_info));
	dev_info(&pf->pdev->dev, "%s: %s\n", __func__,
		 (ret == 0) ? "set mac success!" : "set mac fail!");
}

void ne6x_dbg_get_mac(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_dev_eeprom_info *psdk_spd_info = &pf->sdk_spd_info;
	u8 mac_addr[6];
	int port = 0;

	if (strncmp(cmd_buf, "P0", 2) == 0) {
		port = 0;
	} else if (strncmp(cmd_buf, "P1", 2) == 0) {
		port = 1;
	} else {
		dev_warn(&pf->pdev->dev, "get_port_mac P0/P1\n");
		dev_warn(&pf->pdev->dev, "example-- get_port_mac P0\n");
		return;
	}

	if (port == 0)
		memcpy(&mac_addr, &psdk_spd_info->port_0_mac, 6);
	else if (port == 1)
		memcpy(&mac_addr, &psdk_spd_info->port_1_mac, 6);
	else if (port == 2)
		memcpy(&mac_addr, &psdk_spd_info->port_2_mac, 6);
	else if (port == 3)
		memcpy(&mac_addr, &psdk_spd_info->port_3_mac, 6);
	else
		return;

	dev_info(&pf->pdev->dev, "port %d: mac = %02x:%02x:%02x:%02x:%02x:%02x\n", port,
		 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}

void ne6x_dbg_set_dev_type_to_eeprom(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_dev_eeprom_info *psdk_spd_info = &pf->sdk_spd_info;
	u8 *p_str_array[10] = {0};
	int array_index = 0, ret;
	u8 *p_in_string = NULL;
	char *p_tmp_ret = NULL;
	u16 dev_type = 0;

	p_in_string = &cmd_buf[0];
	while ((p_str_array[array_index] = my_strtok(p_in_string, PARA_KEY_STRING, &p_tmp_ret)) !=
	       NULL) {
		p_in_string = p_str_array[array_index] + strlen(p_str_array[array_index]) + 1;
		array_index++;
		if (array_index >= 10)
			break;

		if (!p_tmp_ret)
			break;
	}

	if (array_index < 1) {
		dev_warn(&pf->pdev->dev, "set_dev_type <devtype> (0xA003:2*25,0xA004:4*25)\n");
		return;
	}

	if (!strncmp(p_str_array[0], "0x", 2)) {
		dev_type = strtoul(p_str_array[0], 0, 16);
	} else {
		dev_warn(&pf->pdev->dev, "set_dev_type <devtype> (0xA003:2*25,0xA004:4*25)\n");
		return;
	}

	if (dev_type != NE6000AI_2S_X16H_25G_N5 && dev_type != NE6000AI_2S_X16H_25G_N6) {
		dev_warn(&pf->pdev->dev, "set_dev_type <devtype> (0xA003:2*25,0xA004:4*25)\n");
		return;
	}

	psdk_spd_info->product_mode = cpu_to_be16(dev_type);
	psdk_spd_info->is_pcie_exist = 0x1;

	if (dev_type == NE6000AI_2S_X16H_25G_N5) {
		psdk_spd_info->number_of_physical_controllers = 2;
		psdk_spd_info->logic_port_to_phyical = cpu_to_be32(0x00000800);
	} else if (dev_type == NE6000AI_2S_X16H_25G_N6) {
		psdk_spd_info->number_of_physical_controllers = 2;
		psdk_spd_info->logic_port_to_phyical = cpu_to_be32(0x00000100);
	} else {
		return;
	}

	psdk_spd_info->spd_verify_value =
		cpu_to_be32(ne6x_dev_crc32((const u8 *)psdk_spd_info,
					   sizeof(struct ne6x_dev_eeprom_info) - 4));
	ret = ne6x_dev_write_eeprom(pf->adpt[0], 0x0, (u8 *)psdk_spd_info,
				    sizeof(struct ne6x_dev_eeprom_info));
	dev_info(&pf->pdev->dev, "%s: %s\n", __func__,
		 (ret == 0) ? "write eeprom mac success!" : "write eeprom mac fail!");
}

void ne6x_dbg_tab_write(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_debug_table *table_info;
	int array_index = 0, ret, index;
	u8 *p_str_array[100] = {0};
	u8 *p_in_string = NULL;
	char *p_tmp_ret = NULL;

	table_info = kzalloc(sizeof(*table_info), GFP_KERNEL);
	memset(table_info, 0, sizeof(*table_info));

	p_in_string = &cmd_buf[0];
	while ((p_str_array[array_index] = my_strtok(p_in_string, PARA_KEY_STRING, &p_tmp_ret)) !=
	       NULL) {
		p_in_string = p_str_array[array_index] + strlen(p_str_array[array_index]) + 1;
		array_index++;
		if (array_index >= 100)
			break;

		if (!p_tmp_ret)
			break;
	}

	if (array_index < 8) {
		dev_info(&pf->pdev->dev, "tab_write <table> <index> <param1> <param2> <param3> <param4> <param5> <param6> ...\n");
		kfree(table_info);
		return;
	}

	/* table */
	if (!strncmp(p_str_array[0], "0x", 2))
		table_info->table = strtoul(p_str_array[0], 0, 16);
	else
		table_info->table = my_atoi(p_str_array[0]);

	/* index */
	if (!strncmp(p_str_array[1], "0x", 2))
		table_info->index = strtoul(p_str_array[1], 0, 16);
	else
		table_info->index = my_atoi(p_str_array[1]);

	/* data */
	table_info->size = 0;
	for (index = 0; index < (array_index - 2); index++) {
		if (!strncmp(p_str_array[index + 2], "0x", 2))
			table_info->data[index] = strtoul(p_str_array[index + 2], 0, 16);
		else
			table_info->data[index] = my_atoi(p_str_array[index + 2]);

		table_info->size++;
	}

	table_info->size = table_size[table_info->table];

	ret = ne6x_reg_table_write(pf, table_info->table, table_info->index,
				   (u32 *)&table_info->data[0], table_info->size);
	kfree(table_info);
	dev_info(&pf->pdev->dev, "%s: %s\n", __func__, (ret == 0) ? "success!" : "timeout!");
}

void ne6x_dbg_tab_insert(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u8 *p_str_array[ARRAY_P_MAX_COUNT] = {0};
	struct ne6x_debug_table *table_info;
	int array_index = 0, ret, index;
	u32 table_id = 0xffffffff;
	u32 temp_table[130];
	u8 *p_in_string = NULL;
	char *p_tmp_ret = NULL;

	table_info = kzalloc(sizeof(*table_info), GFP_KERNEL);
	memset(table_info, 0, sizeof(*table_info));

	p_in_string = &cmd_buf[0];
	while ((p_str_array[array_index] = my_strtok(p_in_string, PARA_KEY_STRING, &p_tmp_ret)) !=
	       NULL) {
		p_in_string = p_str_array[array_index] + strlen(p_str_array[array_index]) + 1;
		array_index++;
		if (array_index >= ARRAY_P_MAX_COUNT)
			break;

		if (!p_tmp_ret)
			break;
	}

	/* 1 + 16 + 1+++ */
	if (array_index < 24) {
		dev_warn(&pf->pdev->dev, "tab_insert <table> <hash_key[64]> <hash_data[64]>\n");
		kfree(table_info);
		return;
	}

	/* table */
	if (!strncmp(p_str_array[0], "0x", 2))
		table_info->table = strtoul(p_str_array[0], 0, 16);
	else
		table_info->table = my_atoi(p_str_array[0]);

	/* data */
	table_info->size = 0;
	for (index = 0; index < (array_index - 1); index++) {
		if (!strncmp(p_str_array[index + 1], "0x", 2))
			table_info->data[index] = strtoul(p_str_array[index + 1], 0, 16);
		else
			table_info->data[index] = my_atoi(p_str_array[index + 1]);

		table_info->size++;
	}

	table_info->size = 64;

	ret = ne6x_reg_table_search(pf, (enum ne6x_reg_table)table_info->table,
				    (u32 *)&table_info->data[0], table_info->size, temp_table,
				   table_info->size);
	if (ret == -ENOENT) {
		table_info->size = 64 + table_size[table_info->table];
		ret = ne6x_reg_table_insert(pf, (enum ne6x_reg_table)table_info->table,
					    (u32 *)&table_info->data[0], table_info->size,
					    &table_id);
	} else {
		dev_info(&pf->pdev->dev, "0x%x 0x%x 0x%x 0x%x table exist\n", table_info->data[0],
			 table_info->data[1], table_info->data[2], table_info->data[3]);
		return;
	}
	if (ret == 0)
		dev_info(&pf->pdev->dev, "insert rule_id = 0x%x\n", table_id);

	dev_info(&pf->pdev->dev, "%s: %s\n", __func__, (ret == 0) ? "success!" :
		 ((ret != -ETIMEDOUT) ? "fail!" : "timeout!"));
}

void ne6x_dbg_tab_delete(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	int array_index = 0, ret, index;
	struct ne6x_debug_table *table_info;
	u8 *p_str_array[100] = {0};
	u8 *p_in_string = NULL;
	char *p_tmp_ret = NULL;

	table_info = kzalloc(sizeof(*table_info), GFP_KERNEL);
	memset(table_info, 0, sizeof(*table_info));

	p_in_string = &cmd_buf[0];
	while ((p_str_array[array_index] = my_strtok(p_in_string, PARA_KEY_STRING, &p_tmp_ret)) !=
	       NULL) {
		p_in_string = p_str_array[array_index] + strlen(p_str_array[array_index]) + 1;
		array_index++;
		if (array_index >= 100)
			break;

		if (!p_tmp_ret)
			break;
	}

	if (array_index < 9) {
		dev_warn(&pf->pdev->dev, "tab_delete <table> <hash_key>\n");
		kfree(table_info);
		return;
	}

	/* table */
	if (!strncmp(p_str_array[0], "0x", 2))
		table_info->table = strtoul(p_str_array[0], 0, 16);
	else
		table_info->table = my_atoi(p_str_array[0]);

	/* data */
	table_info->size = 0;
	for (index = 0; index < (array_index - 1); index++) {
		if (!strncmp(p_str_array[index + 1], "0x", 2))
			table_info->data[index] = strtoul(p_str_array[index + 1], 0, 16);
		else
			table_info->data[index] = my_atoi(p_str_array[index + 1]);

		table_info->size++;
	}

	table_info->size = 64;

	ret = ne6x_reg_table_delete(pf, (enum ne6x_reg_table)table_info->table,
				    (u32 *)&table_info->data[0], table_info->size);
	kfree(table_info);
	dev_info(&pf->pdev->dev, "%s: %s\n", __func__, (ret == 0) ? "success!" : "timeout!");
}

void ne6x_dbg_tab_search(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_debug_table *table_info;
	int array_index = 0, ret, index;
	u8 *p_str_array[100] = {0};
	u8 *p_in_string = NULL;
	char *p_tmp_ret = NULL;

	table_info = kzalloc(sizeof(*table_info), GFP_KERNEL);
	memset(table_info, 0, sizeof(*table_info));

	p_in_string = &cmd_buf[0];
	while ((p_str_array[array_index] = my_strtok(p_in_string, PARA_KEY_STRING, &p_tmp_ret)) !=
	       NULL) {
		p_in_string = p_str_array[array_index] + strlen(p_str_array[array_index]) + 1;
		array_index++;
		if (array_index >= 100)
			break;

		if (!p_tmp_ret)
			break;
	}

	dev_info(&pf->pdev->dev, "array_index = %d\n", array_index);
	if (array_index < 9) {
		dev_warn(&pf->pdev->dev, "tab_delete <table> <hash_key>\n");
		kfree(table_info);
		return;
	}

	if (!strncmp(p_str_array[0], "0x", 2))
		table_info->table = strtoul(p_str_array[0], 0, 16);
	else
		table_info->table = my_atoi(p_str_array[0]);

	table_info->size = 0;
	for (index = 0; index < (array_index - 1); index++) {
		if (!strncmp(p_str_array[index + 1], "0x", 2))
			table_info->data[index] = strtoul(p_str_array[index + 1], 0, 16);
		else
			table_info->data[index] = my_atoi(p_str_array[index + 1]);

		table_info->size++;
	}

	table_info->size = 64;
	ret = ne6x_reg_table_search(pf, (enum ne6x_reg_table)table_info->table,
				    (u32 *)&table_info->data[0], table_info->size,
				    (u32 *)&table_info->data[0], table_info->size);
	dev_info(&pf->pdev->dev, "%s: %s\n", __func__,
		 (ret == 0) ? "success!" : ((ret == -ENOENT) ? "not fount!" : "timeout!"));
	if (ret)
		return;

	for (index = 0x00; index < (table_info->size >> 2) / 4; index++)
		dev_info(&pf->pdev->dev, "%08X: %08X %08X %08X %08X\n", index * 16,
			 table_info->data[4 * index], table_info->data[4 * index + 1],
			 table_info->data[4 * index + 2], table_info->data[4 * index + 3]);

	if (((table_info->size >> 2) % 4) == 1)
		dev_info(&pf->pdev->dev, "%08X: %08X\n", index * 16, table_info->data[4 * index]);
	else if (((table_info->size >> 2) % 4) == 2)
		dev_info(&pf->pdev->dev, "%08X: %08X %08X\n", index * 16,
			 table_info->data[4 * index], table_info->data[4 * index + 1]);
	else if (((table_info->size >> 2) % 4) == 3)
		dev_info(&pf->pdev->dev, "%08X: %08X %08X %08X\n", index * 16,
			 table_info->data[4 * index], table_info->data[4 * index + 1],
			 table_info->data[4 * index + 2]);

	kfree(table_info);
}

void ne6x_dbg_get_fru_info(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct file *fp = NULL;
	u8 *buffer;
	int para_count;
	u32 size;

	para_count = sscanf(&cmd_buf[0], "%i", &size);
	if (para_count != 1) {
		dev_warn(&pf->pdev->dev, "fru_read <size>\n");
		return;
	}

	if (size > 512) {
		dev_warn(&pf->pdev->dev, "size must less than 512\n.");
		return;
	}

	buffer = kzalloc((size + 4), GFP_KERNEL);
	ne6x_dev_get_fru(pf, (u32 *)buffer, size);

	fp = filp_open("/opt/share/fru.bin", O_RDWR | O_CREAT, 0644);
	if (!fp) {
		dev_err(&pf->pdev->dev, "can't open /opt/share/fru.bin.\n");
		return;
	}

	kernel_write(fp, (char *)buffer, size, &fp->f_pos);
	filp_close(fp, NULL);
}

u32 getparam(char *cmd_buf, u32 *param, int max_cnt)
{
	int ret, i, j, tmp, tmp1, tmp2, flag = 0;
	u32 count = 0, cnt = 0, cnt_t = 0;
	char *p = &cmd_buf[0];
	char *char_offset;
	u32 *offset;

	offset = kzalloc((max_cnt + 1) * sizeof(u32), GFP_ATOMIC);
	char_offset = kzalloc((max_cnt + 1) * sizeof(char), GFP_ATOMIC);
	/* count the number */
	for (i = 0; i < strlen(cmd_buf); i++) {
		if (cmd_buf[i] == ',' || cmd_buf[i] == '-') {
			count++;
			if (cmd_buf[i] == ',') {
				offset[count] = i + 1;
				char_offset[count] = ',';
			} else if (cmd_buf[i] == '-') {
				offset[count] = i + 1;
				char_offset[count] = '-';
			}
		}
		if (cmd_buf[i] == ' ')
			break;

		if (count >= max_cnt)
			break;
	}

	for (i = 0; i <= count; i++) {
		ret = sscanf(p, "%i", &param[i + cnt_t]);
		if (ret == 1) {
			cnt++;
			if (char_offset[cnt] == '-') {
				flag++;
				p = &cmd_buf[offset[cnt]];
				ret = sscanf(p, "%i", &param[i + cnt_t + 1]);
				tmp1 = param[i + cnt_t];
				tmp2 = param[i + cnt_t + 1];
				if (ret == 1) {
					tmp = i + cnt_t;
					for (j = 0; j <= tmp2 - tmp1; j++)
						param[tmp + j] = tmp1 + j;
				}
				cnt_t += tmp2 - tmp1;

				cnt++;
			}
			p = &cmd_buf[offset[cnt]];
		}
	}

	kfree(offset);

	return cnt + cnt_t - 2 * flag;
}

void GetNextArg(char **Args, char *buffer)
{
	while (**Args == ' ' || **Args == '\t')
		(*Args)++;

	while (**Args != ' ' && **Args != '\t' && **Args != '\0')
		*buffer++ = *(*Args)++;

	*buffer = '\0';
}

void ne6x_dbg_show_pcie_drop_counter(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	union ne6x_eth_recv_cnt eth_recv_cnt;
	u64 __iomem *reg;

	reg = (void __iomem *)pf->hw.hw_addr4 + NE6X_PFINT_DYN_CTLN(7, NE6X_ETH_RECV_CNT);
	eth_recv_cnt.val = readq(reg);
	dev_info(&pf->pdev->dev, "pcie drop cnt = %d\n", eth_recv_cnt.reg.csr_eth_pkt_drop_cnt
		 + eth_recv_cnt.reg.csr_eth_rdq_drop_cnt);
}

void ne6x_dbg_clr_table(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u32 table_id = 0, cnt;

	cnt = sscanf(&cmd_buf[0], "%i", &table_id);
	if (table_id == 6)
		ne6x_reg_clear_table(pf, table_id);
}

void ne6x_dbg_set_hw_flag_eeprom(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	struct ne6x_dev_eeprom_info *psdk_spd_info = &pf->sdk_spd_info;
	int flag = 0;
	int ret;
	int cnt;

	cnt = sscanf(&cmd_buf[0], "%i", &flag);
	if (cnt != 1) {
		dev_warn(&pf->pdev->dev, "\n0:none;1,ram white list;2,ddr white list\n");
		return;
	}

	psdk_spd_info->hw_flag = cpu_to_be32(flag);
	psdk_spd_info->spd_verify_value =
		cpu_to_be32(ne6x_dev_crc32((const u8 *)psdk_spd_info,
					   sizeof(struct ne6x_dev_eeprom_info) - 4));
	ret = ne6x_dev_write_eeprom(pf->adpt[0], 0x0, (u8 *)psdk_spd_info,
				    sizeof(struct ne6x_dev_eeprom_info));
	dev_info(&pf->pdev->dev, "%s: %s\n", __func__, (ret == 0) ? "set hw_flag success!"
		: "set hw_flag fail!");
}

void ne6x_dbg_erase_norflash(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u32 offset;
	u32 length;
	int cnt;

	cnt = sscanf(&cmd_buf[0], "%i %i", &offset, &length);
	if (cnt != 2) {
		dev_warn(&pf->pdev->dev, "norflash_erase <offset> <length:4 bytes aligned>\n");
		return;
	}

	if (!ne6x_reg_erase_norflash(pf, offset, length))
		return;

	dev_err(&pf->pdev->dev, "norflash_erase fail.\n");
}

void ne6x_dbg_write_norflash(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u8 *ptemp_data = NULL;
	u32 offset = 0;
	u32 length = 0;
	u32 temp_data = 0;
	u8 *ptemp = NULL;
	int i = 0;

	ptemp_data = kzalloc(1024, GFP_ATOMIC);

	while ((ptemp = strsep(&cmd_buf, " "))) {
		if (!strncmp(ptemp, "0x", 2))
			temp_data = strtoul(ptemp, 0, 16);
		else
			temp_data = my_atoi(ptemp);

		if (i == 0)
			offset = temp_data;
		else if (i == 1)
			length = temp_data;
		else
			ptemp_data[i - 2] = (u8)temp_data;

		i++;
		if (i == 1026)
			break;
	}

	if (length > 1024 || i < 2) {
		dev_warn(&pf->pdev->dev, "norflash_write <offset> <length> <data> (byte split by space max 256)\n");
		goto pdata_memfree;
	}

	if (!ne6x_reg_write_norflash(pf, offset, length, (u32 *)ptemp_data))
		dev_info(&pf->pdev->dev, "write norflash success.\n");
	else
		dev_err(&pf->pdev->dev, "write norflash fail.\n");

pdata_memfree:
	kfree(ptemp_data);
}

void ne6x_dbg_read_norflash(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u32 offset = 0;
	u32 length = 0;
	u32 buffer_len;
	char *pdata = NULL;
	int cnt;

	cnt = sscanf(&cmd_buf[0], "%i %i", &offset, &length);
	if (cnt != 2) {
		dev_warn(&pf->pdev->dev, "norflash_read <offset> <length:4 bytes aligned>\n");
		return;
	}

	buffer_len = length;
	if (length % 4)
		buffer_len = (length / 4 + 1) * 4;

	pdata = kzalloc(buffer_len, GFP_ATOMIC);
	if (!ne6x_reg_read_norflash(pf, offset, buffer_len, (u32 *)pdata))
		print_hex_dump_bytes("", DUMP_PREFIX_OFFSET, pdata, length);
	else
		dev_err(&pf->pdev->dev, "read_norflash fail.\n");

	kfree(pdata);
}

void ne6x_dbg_meter_write(struct ne6x_pf *pf, char *cmd_buf, int count)
{
	u8 *p_str_array[ARRAY_P_MAX_COUNT] = {0};
	u32 cir, type_num, type_flag = 0;
	u32 cir_maxnum = 0xfffff;
	u32 cbs_maxnum = 0xffffff;
	struct meter_table vf_bw;
	char *p_tmp_ret;
	int index, ret = 0;
	int array_index = 0;
	u8 *p_in_string = NULL;
	u32 data[3] = {0};
	u32 type = 0;

	p_in_string = &cmd_buf[0];
	p_tmp_ret = NULL;

	while ((p_str_array[array_index] = my_strtok(p_in_string, PARA_KEY_STRING, &p_tmp_ret)) !=
	       NULL) {
		p_in_string = p_str_array[array_index] + strlen(p_str_array[array_index]) + 1;
		array_index++;
		if (array_index >= ARRAY_P_MAX_COUNT)
			break;
		if (!p_tmp_ret)
			break;
	}
	if (array_index != 3) {
		dev_warn(&pf->pdev->dev, "Incorrect input, please re-enter\n");
		return;
	}

	for (index = 0; index < array_index; index++) {
		if (!strncmp(p_str_array[index], "0x", 2))
			data[index] = strtoul(p_str_array[index], 0, 16);
		else
			data[index] = my_atoi(p_str_array[index]);
	}

	type_num = data[0];
	switch (type_num) {
	case 0:
		type_flag |= NE6X_F_ACK_FLOOD;
		break;
	case 1:
		type_flag |= NE6X_F_PUSH_ACK_FLOOD;
		break;
	case 2:
		type_flag |= NE6X_F_SYN_ACK_FLOOD;
		break;
	case 3:
		type_flag |= NE6X_F_FIN_FLOOD;
		break;
	case 4:
		type_flag |= NE6X_F_RST_FLOOD;
		break;
	case 5:
		type_flag |= NE6X_F_PUSH_SYN_ACK_FLOOD;
		break;
	case 6:
		type_flag |= NE6X_F_UDP_FLOOD;
		break;
	case 7:
		type_flag |= NE6X_F_ICMP_FLOOD;
		break;
	case 8:
		type_flag |= NE6X_F_FRAGMENT_FLOOD;
		break;
	default:
		dev_err(&pf->pdev->dev, "err_input,please enter one of'0-8'\n");
		return;
	}

	if (data[1] == 1) {
		ne6x_reg_get_user_data(pf, NP_USER_DATA_DDOS_FLAG, &type);
		type |= type_flag;
		ne6x_reg_set_user_data(pf, NP_USER_DATA_DDOS_FLAG, type);
	} else if (data[1] == 0) {
		ne6x_reg_get_user_data(pf, NP_USER_DATA_DDOS_FLAG, &type);
		type &= ~type_flag;
		ne6x_reg_set_user_data(pf, NP_USER_DATA_DDOS_FLAG, type);
	} else {
		dev_err(&pf->pdev->dev, "Input error, please enter '0' or '1'\n");
		return;
	}

	cir = data[2] * 1000 + 1023;
	cir = min((cir / 1024), cir_maxnum);
	vf_bw.cir = cir;
	vf_bw.pir = min((cir + cir / 10), cir_maxnum);

	vf_bw.cbs = min((vf_bw.cir * 10000), cbs_maxnum);
	vf_bw.pbs = min((vf_bw.pir * 10000), cbs_maxnum);
	ret = ne6x_reg_config_meter(pf, NE6X_METER1_TABLE |
				    NE6X_METER_SUBSET(NE6X_METER_SUBSET0) | type_num,
				    (u32 *)&vf_bw, sizeof(vf_bw));

	dev_info(&pf->pdev->dev, "%s: %s\n", __func__,
		 (ret == 0) ? "write meter success!" : "write meter fail!");
}

const struct ne6x_dbg_cmd_wr deg_cmd_wr[] = {
	{"queue",               ne6x_dbg_show_queue},
	{"ring",                ne6x_dbg_show_ring},
	{"txq",                 ne6x_dbg_show_txq},
	{"rxq",                 ne6x_dbg_show_rxq},
	{"cq",                  ne6x_dbg_show_cq},
	{"clean",               ne6x_dbg_clean_queue},
	{"txtail",              ne6x_dbg_show_txtail},
	{"txr",                 ne6x_dbg_show_txring},
	{"rxr",                 ne6x_dbg_show_rxring},
	{"cqr",                 ne6x_dbg_show_cqring},
#ifdef CONFIG_RFS_ACCEL
	{"arfs",                ne6x_dbg_show_arfs_cnt},
#endif
	{"apb_read",            ne6x_dbg_apb_read},
	{"apb_write",           ne6x_dbg_apb_write},
	{"mem_read",            ne6x_dbg_mem_read},
	{"mem_write",           ne6x_dbg_mem_write},
	{"soc_read",            ne6x_dbg_soc_read},
	{"soc_write",           ne6x_dbg_soc_write},
	{"templ_help",          ne6x_dbg_templ_help},
	{"templ_read",          ne6x_dbg_templ_read},
	{"templ_write",         ne6x_dbg_templ_write},
	{"tab_read",            ne6x_dbg_tab_read},
	{"tab_write",           ne6x_dbg_tab_write},
	{"tab_insert",          ne6x_dbg_tab_insert},
	{"tab_delete",          ne6x_dbg_tab_delete},
	{"tab_search",          ne6x_dbg_tab_search},
	{"set_port_mac",        ne6x_dbg_set_mac_to_eeprom},
	{"get_port_mac",        ne6x_dbg_get_mac},
	{"fru_read",            ne6x_dbg_get_fru_info},
	{"pcie_dropcnt",        ne6x_dbg_show_pcie_drop_counter},
	{"clear_table",         ne6x_dbg_clr_table},
	{"set_hw_flag",         ne6x_dbg_set_hw_flag_eeprom},
	{"norflash_erase",      ne6x_dbg_erase_norflash},
	{"norflash_write",      ne6x_dbg_write_norflash},
	{"norflash_read",       ne6x_dbg_read_norflash},
	{"meter_write",         ne6x_dbg_meter_write},
};

/**
 * ne6x_dbg_command_read - read for command datum
 * @filp: the opened file
 * @buffer: where to write the data for the user to read
 * @count: the size of the user's buffer
 * @ppos: file position offset
 **/
static ssize_t ne6x_dbg_command_read(struct file *filp, char __user *buffer, size_t count,
				     loff_t *ppos)
{
	return 0;
}

static ssize_t ne6x_dbg_info_pnsn_read(struct file *filp, char __user *buffer,
				       size_t count, loff_t *ppos)
{
	u8 *pru_name = NULL, *pru_pn = NULL, *pru_sn = NULL;
	char name_pre[INFO_COL] = {0};
	char name_aft[INFO_COL] = {0};
	struct ne6x_pf *pf = NULL;
	u32 buf_size = 500;
	char *name = NULL;
	ssize_t len = 0;
	u8 *buffer_data;
	u8 length = 0;
	u16 device_id;
	int erro = 0;
	int dex = 0;
	int i = 0;

	if (*ppos > 0 || count < PAGE_SIZE)
		return 0;

	name = kzalloc(PAGE_SIZE, GFP_KERNEL);
	if (!name)
		return -ENOMEM;

	buffer_data = kzalloc(buf_size, GFP_KERNEL);
	if (!buffer_data) {
		kfree(name);
		return -ENOMEM;
	}

	pf = filp->private_data;
	ne6x_dev_get_fru(pf, (u32 *)buffer_data, buf_size);

	pru_name = ne6x_dbg_get_fru_product_part(buffer_data, PRODUCT_NAME, &length);
	if (!pru_name) {
		dev_err(&pf->pdev->dev, "get pru_name info erro");
		device_id = pf->hw.subsystem_device_id;
		if (!device_id) {
			dev_err(&pf->pdev->dev, "subsystem_device_id is NULL!");
			erro = 1;
			goto get_buffer_end;
		}

		sprintf(name_pre, "Product Name: BeiZhongWangXin");
		sprintf(name_aft, "Ethernet Adapter");

		for (i = 0; i < ARRAY_SIZE(ne6x_device_info); i++) {
			if (device_id == ne6x_device_info[i].system_id)
				dex = i;
		}

		if (dex != -1) {
			len = sprintf(name, "%s %s %s %s\n", name_pre,
				      ne6x_device_info[dex].system_name,
				      ne6x_device_info[dex].system_speed, name_aft);
		} else {
			dev_warn(&pf->pdev->dev, "subsystem_device_id not match");
			erro = 1;
			goto get_buffer_end;
		}

	} else {
		len = sprintf(name, "Product Name: %s\n", pru_name);
	}

	pru_pn = ne6x_dbg_get_fru_product_part(buffer_data, PRODUCT_PART_NUMBER, &length);
	if (pru_pn)
		len = sprintf(name, "%s[PN] Part number: %s\n", name, pru_pn);

	pru_sn = ne6x_dbg_get_fru_product_part(buffer_data, PRODUCT_SERIAL_NUMBER, &length);
	if (pru_sn)
		len = sprintf(name, "%s[SN] Serial number: %s\n", name, pru_sn);

	if (copy_to_user(buffer, name, len)) {
		erro = 2;
		goto get_buffer_end;
	}

	if (!len) {
		erro = 1;
		goto get_buffer_end;
	}

	*ppos = len;
	goto get_buffer_end;

get_buffer_end:
	kfree(pru_pn);
	kfree(pru_sn);
	kfree(pru_name);
	kfree(name);
	kfree(buffer_data);

	if (erro == 1)
		return 0;
	else if (erro == 2)
		return -EFAULT;

	return len;
}

static bool ne6x_dbg_fru_checksum(const u8 *data, u32 len)
{
	u8 gl = 0;
	u32 i;

	for (i = 0; i < len - 1; i++)
		gl += data[i];

	gl = ~gl + 1;
	return gl == data[len - 1];
}

static int ne6x_dbg_fru_get_offset(u8 *buffer, enum fru_type type, u8 *offset)
{
	u8 hd[8] = {0};
	int i;

	for (i = 0; i < 8; i++)
		hd[i] = buffer[i];

	if (!(hd[0] & 0x1))
		return -2;

	if (!ne6x_dbg_fru_checksum(hd, 8))
		return -3;

	if (type < INTER_USE_AREA || type > MUILT_AREA)
		return -4;

	*offset = hd[type + 1];

	return 0;
}

static u8 *ne6x_dbg_fru_6ascii28(const u8 *data, u8 *len)
{
	u8 len_bit_6, len_bit_8;
	int i, i6, byte;
	u8 *buf = NULL;

	len_bit_6 = data[0] & 0x3F;
	len_bit_8 = FRU_6BIT_8BITLENGTH(len_bit_6);
	buf = kzalloc(len_bit_8 + 1, GFP_ATOMIC);

	if (!buf) {
		*len = 0;
		return NULL;
	}

	for (i = 0, i6 = 1; i6 <= len_bit_6 && i < len_bit_8 && data[i6]; i++) {
		byte = (i - 1) % 4;

		switch (byte) {
		case 0:
			buf[i] = data[i6] & 0x3F;
			break;
		case 1:
			buf[i] = (data[i6] >> 6) | (data[1 + i6] << 2);
			i6++;
			break;
		case 2:
			buf[i] = (data[i6] >> 4) | (data[1 + i6] << 4);
			i6++;
			break;
		case 3:
			buf[i] = data[i6++] >> 2;
			break;
		}

		buf[i] &= 0x3F;
		buf[i] += ASCII628_BASE;
	}

	*len = len_bit_8;

	return buf;
}

u8 *ne6x_dbg_get_fru_product_part(u8 *buffer, enum fru_product_part part, u8 *len)
{
	u8 hd[2] = {0};
	u8 *pt = NULL;
	u8 ofst = 0;
	u32 i = 0;

	if (!buffer)
		return NULL;

	if (ne6x_dbg_fru_get_offset(buffer, PRODUCT_AREA, &ofst) != 0 || ofst == 0) {
		*len = 0;
		return NULL;
	}

	ofst *= 8;
	hd[0] = buffer[ofst];
	hd[1] = buffer[ofst + 1];
	if (!(hd[0] & 0x1) || hd[1] == 0)
		return NULL;

	if (!ne6x_dbg_fru_checksum(&buffer[ofst], hd[1] * 8))
		return NULL;

	ofst += 3;

	for (i = 0; i < part; i++)
		ofst += 1 + (buffer[ofst] & 0x3f);

	if (FRU_CHECK_6ASCII(buffer[ofst])) {
		pt = ne6x_dbg_fru_6ascii28(&buffer[ofst], len);
	} else {
		*len = (buffer[ofst] & 0x3f);
		pt = kzalloc(*len, GFP_ATOMIC);
		if (!pt)
			return NULL;

		memcpy(pt, &buffer[ofst + 1], *len);
	}

	return pt;
}

void ne6x_dbg_update_adpt_speed(struct ne6x_adapter *adpt, u32 speed, u32 lane_mode) {}

/**
 * ne6x_dbg_command_write - write into command datum
 * @filp: the opened file
 * @buffer: where to find the user's data
 * @count: the length of the user's data
 * @ppos: file position offset
 **/
static ssize_t ne6x_dbg_command_write(struct file *filp, const char __user *buffer,
				      size_t count, loff_t *ppos)
{
	struct ne6x_pf *pf = filp->private_data;
	char *cmd_buf, *cmd_buf_tmp;
	struct ne6x_ring *tx_ring;
	int bytes_not_copied;
	struct ne6x_adapter *adpt;
	int i, cnt = 0;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;

	/* don't cross maximal possible value */
	if (count >= NE6X_DEBUG_CHAR_LEN)
		return -ENOSPC;

	cmd_buf = kzalloc(count + 1, GFP_KERNEL);
	if (!cmd_buf)
		return count;

	bytes_not_copied = copy_from_user(cmd_buf, buffer, count);
	if (bytes_not_copied) {
		kfree(cmd_buf);
		return -EFAULT;
	}
	cmd_buf[count] = '\0';

	cmd_buf_tmp = strchr(cmd_buf, '\n');
	if (cmd_buf_tmp) {
		*cmd_buf_tmp = '\0';
		count = cmd_buf_tmp - cmd_buf + 1;
	}

	if (strncmp(cmd_buf, "updtail", 7) == 0) {
		int idx, vp, tail;

		cnt = sscanf(&cmd_buf[7], "%d %d %d", &idx, &vp, &tail);
		if (cnt != 3) {
			dev_warn(&pf->pdev->dev, "updtail <vp> <tail>\n");
			goto command_write_done;
		}
		adpt = pf->adpt[idx ? 1 : 0];
		tx_ring = adpt->tx_rings[vp & 0xF];
		ne6x_tail_update(tx_ring, tail);
		dev_info(&pf->pdev->dev, "write: adpt = %d vp = 0x%x  tail_ptr = %d\n", idx ? 1 : 0,
			 vp, tail);
	} else if (strncmp(cmd_buf, "memrd", 5) == 0) {
		u32 base_addr;
		u32 offset_addr = 0;
		u64 value;
		int index, vp;

		cnt = sscanf(&cmd_buf[5], "%d", &vp);
		if (cnt != 1) {
			dev_warn(&pf->pdev->dev, "memrd <vp_num>\n");
			goto command_write_done;
		}

		offset_addr = 0x0;
		for (index = 0; index < 0x20; index++) {
			base_addr = 0x140 + vp;
			value = ne6x_reg_pci_read(pf, base_addr, offset_addr);
			dev_info(&pf->pdev->dev, "read: 0x%x 0x%02x = 0x%llx\n", base_addr,
				 offset_addr, value);
			offset_addr++;
		}

		if (base_addr == 0x13F) {
			offset_addr = 0x21;
			for (index = 0x21; index < 0x24; index++) {
				base_addr = 0x140 + vp;
				value = ne6x_reg_pci_read(pf, base_addr, offset_addr);
				dev_info(&pf->pdev->dev, "read: 0x%x 0x%02x = 0x%llx\n", base_addr,
					 offset_addr, value);
				offset_addr++;
			}

			offset_addr = 0x39;
			for (index = 0x39; index < 0x4E; index++) {
				base_addr = 0x140 + vp;
				value = ne6x_reg_pci_read(pf, base_addr, offset_addr);
				dev_info(&pf->pdev->dev, "read: 0x%x 0x%02x = 0x%llx\n", base_addr,
					 offset_addr, value);
				offset_addr++;
			}

			offset_addr = 0x80;
			for (index = 0x80; index < 0x95; index++) {
				base_addr = 0x140 + vp;
				value = ne6x_reg_pci_read(pf, base_addr, offset_addr);
				dev_info(&pf->pdev->dev, "read: 0x%x 0x%02x = 0x%llx\n", base_addr,
					 offset_addr, value);
				offset_addr++;
			}

			offset_addr = 0xA3;
			for (index = 0xA3; index < 0xA5; index++) {
				base_addr = 0x140 + vp;
				value = ne6x_reg_pci_read(pf, base_addr, offset_addr);
				dev_info(&pf->pdev->dev, "read: 0x%x 0x%02x = 0x%llx\n", base_addr,
					 offset_addr, value);
				offset_addr++;
			}
		}
	} else if (strncmp(cmd_buf, "read", 4) == 0) {
		u32 base_addr;
		u32 offset_addr;
		u64 value;

		cnt = sscanf(&cmd_buf[4], "%i %i", &base_addr, &offset_addr);
		if (cnt != 2) {
			dev_warn(&pf->pdev->dev, "read <reg_base> <reg_offset>\n");
			goto command_write_done;
		}

		value = ne6x_reg_pci_read(pf, base_addr, offset_addr);
		dev_info(&pf->pdev->dev, "read: 0x%x 0x%x = 0x%llx\n", base_addr, offset_addr,
			 value);
	} else if (strncmp(cmd_buf, "write", 5) == 0) {
		u32 base_addr;
		u32 offset_addr;
		u64 value;

		cnt = sscanf(&cmd_buf[5], "%i %i %lli ", &base_addr, &offset_addr, &value);
		if (cnt != 3) {
			dev_warn(&pf->pdev->dev, "write <reg_base> <reg_offset> <value>\n");
			goto command_write_done;
		}

		ne6x_reg_pci_write(pf, base_addr, offset_addr, value);
		value = ne6x_reg_pci_read(pf, base_addr, offset_addr);
		dev_info(&pf->pdev->dev, "write: 0x%x 0x%x = 0x%llx\n", base_addr, offset_addr,
			 value);
	} else if (strncmp(cmd_buf, "wr", 2) == 0) {
		u32 offset;
		u32 value;

		cnt = sscanf(&cmd_buf[2], "%i %i", &offset, &value);
		if (cnt != 2) {
			dev_warn(&pf->pdev->dev, "rr <offset> <value>\n");
			goto command_write_done;
		}
		ne6x_reg_indirect_write(pf, offset, value);
		dev_info(&pf->pdev->dev, "wr: 0x%x = 0x%x\n", offset, value);
	} else if (strncmp(cmd_buf, "rr", 2) == 0) {
		u32 offset;
		u32 value;

		cnt = sscanf(&cmd_buf[2], "%i", &offset);
		if (cnt != 1) {
			dev_warn(&pf->pdev->dev, "read <reg_base> <reg_offset>\n");
			goto command_write_done;
		}

		value = ne6x_reg_indirect_read(pf, offset, &value);
		dev_info(&pf->pdev->dev, "rr: 0x%x = 0x%x\n", offset, value);
	} else if (strncmp(cmd_buf, "txd", 3) == 0) {
		u32 adpt_num;
		u32 quenue_num;

		cnt = sscanf(&cmd_buf[3], "%i %i", &adpt_num, &quenue_num);
		if (cnt != 2) {
			dev_warn(&pf->pdev->dev, "txd <adpt_num> <quenue_num>\n");
			goto command_write_done;
		}

		ne6x_dbg_show_txdesc_states(adpt_num, quenue_num, pf);
	} else if (strncmp(cmd_buf, "rxd", 3) == 0) {
		u32 adpt_num;
		u32 quenue_num;

		cnt = sscanf(&cmd_buf[3], "%i %i", &adpt_num, &quenue_num);
		if (cnt != 2) {
			dev_warn(&pf->pdev->dev, "rxd <adpt_num> <quenue_num>\n");
			goto command_write_done;
		}

		ne6x_dbg_show_rxdesc_states(adpt_num, quenue_num, pf);
	} else if (strncmp(cmd_buf, "cqd", 3) == 0) {
		u32 adpt_num;
		u32 quenue_num;

		cnt = sscanf(&cmd_buf[3], "%i %i", &adpt_num, &quenue_num);
		if (cnt != 2) {
			dev_warn(&pf->pdev->dev, "cqd <adpt_num> <quenue_num>\n");
			goto command_write_done;
		}

		ne6x_dbg_show_cqdesc_states(adpt_num, quenue_num, pf);
	} else {
		for (i = 0; i < count; i++) {
			if (cmd_buf[i] == ' ') {
				cmd_buf[i] = '\0';
				cnt = i;
				break;
			}
			if (cmd_buf[i] == '\0') {
				cnt = i;
				break;
			}
		}

		for (i = 0; i < ARRAY_SIZE(deg_cmd_wr); i++) {
			if (strncmp(cmd_buf, deg_cmd_wr[i].command, cnt) == 0) {
				deg_cmd_wr[i].command_proc(pf, &cmd_buf[cnt + 1], count - cnt - 1);
				goto command_write_done;
			}
		}

		dev_info(&pf->pdev->dev, "unknown command '%s'\n", cmd_buf);
	}

command_write_done:
	kfree(cmd_buf);
	cmd_buf = NULL;
	return count;
}

static const struct file_operations ne6x_dbg_command_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ne6x_dbg_command_read,
	.write = ne6x_dbg_command_write,
};

const struct ne6x_dbg_cmd_wr deg_netdev_ops_cmd_wr[] = {};

/**
 * ne6x_dbg_netdev_ops_read - read for netdev_ops datum
 * @filp: the opened file
 * @buffer: where to write the data for the user to read
 * @count: the size of the user's buffer
 * @ppos: file position offset
 **/
static const struct file_operations ne6x_dbg_info_pnsn_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ne6x_dbg_info_pnsn_read,
};

static const struct file_operations ne6x_dbg_info_tps_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ne6x_proc_tps_read,
};

static ssize_t ne6x_dbg_netdev_ops_read(struct file *filp, char __user *buffer,
					size_t count, loff_t *ppos)
{
	return 0;
}

/**
 * ne6x_dbg_netdev_ops_write - write into netdev_ops datum
 * @filp: the opened file
 * @buffer: where to find the user's data
 * @count: the length of the user's data
 * @ppos: file position offset
 **/
static ssize_t ne6x_dbg_netdev_ops_write(struct file *filp,
					 const char __user *buffer,
					 size_t count, loff_t *ppos)
{
	struct ne6x_pf *pf = filp->private_data;
	char *cmd_buf, *cmd_buf_tmp;
	int bytes_not_copied;
	int i;

	/* don't allow partial writes */
	if (*ppos != 0)
		return 0;

	/* don't cross maximal possible value */
	if (count >= NE6X_DEBUG_CHAR_LEN)
		return -ENOSPC;

	cmd_buf = kzalloc(count + 1, GFP_KERNEL);
	if (!cmd_buf)
		return count;

	bytes_not_copied = copy_from_user(cmd_buf, buffer, count);
	if (bytes_not_copied) {
		kfree(cmd_buf);
		return -EFAULT;
	}
	cmd_buf[count] = '\0';

	cmd_buf_tmp = strchr(cmd_buf, '\n');
	if (cmd_buf_tmp) {
		*cmd_buf_tmp = '\0';
		count = cmd_buf_tmp - cmd_buf + 1;
	}

	for (i = 0; i < ARRAY_SIZE(deg_netdev_ops_cmd_wr); i++) {
		if (strncmp(cmd_buf, deg_netdev_ops_cmd_wr[i].command, count) == 0) {
			deg_netdev_ops_cmd_wr[i].command_proc(pf,
				&cmd_buf[sizeof(deg_netdev_ops_cmd_wr[i].command) + 1],
				count - 1 - sizeof(deg_netdev_ops_cmd_wr[i].command));
			goto command_write_done;
		}
	}
	dev_info(&pf->pdev->dev, "unknown command '%s'\n", cmd_buf);

command_write_done:
	kfree(cmd_buf);
	cmd_buf = NULL;
	return count;
}

static const struct file_operations ne6x_dbg_netdev_ops_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ne6x_dbg_netdev_ops_read,
	.write = ne6x_dbg_netdev_ops_write,
};

/**
 * ne6x_dbg_pf_init - setup the debugfs directory for the PF
 * @pf: the PF that is starting up
 **/
void ne6x_dbg_pf_init(struct ne6x_pf *pf)
{
	const struct device *dev = &pf->pdev->dev;
	const char *name = pci_name(pf->pdev);
	struct dentry *pfile;

	pf->ne6x_dbg_pf = debugfs_create_dir(name, ne6x_dbg_root);
	if (!pf->ne6x_dbg_pf)
		return;

	pf->ne6x_dbg_info_pf = debugfs_create_dir("info", pf->ne6x_dbg_pf);
	if (!pf->ne6x_dbg_info_pf)
		return;

	pfile = debugfs_create_file("command", 0600, pf->ne6x_dbg_pf, pf, &ne6x_dbg_command_fops);
	if (!pfile)
		goto create_failed;

	pfile = debugfs_create_file("netdev_ops", 0600, pf->ne6x_dbg_pf, pf,
				    &ne6x_dbg_netdev_ops_fops);
	if (!pfile)
		goto create_failed;

	pfile = debugfs_create_file("product_info", 0600, pf->ne6x_dbg_info_pf, pf,
				    &ne6x_dbg_info_pnsn_fops);
	if (!pfile)
		goto create_failed;

	pfile = debugfs_create_file("power_info", 0600, pf->ne6x_dbg_info_pf, pf,
				    &ne6x_dbg_info_tps_fops);
	if (!pfile)
		goto create_failed;

	return;

create_failed:
	dev_err(dev, "debugfs dir/file for %s failed\n", name);
	debugfs_remove_recursive(pf->ne6x_dbg_info_pf);
	debugfs_remove_recursive(pf->ne6x_dbg_pf);
}

/**
 * ne6x_dbg_pf_exit - clear out the PF's debugfs entries
 * @pf: the PF that is stopping
 **/
void ne6x_dbg_pf_exit(struct ne6x_pf *pf)
{
	debugfs_remove_recursive(pf->ne6x_dbg_info_pf);
	pf->ne6x_dbg_info_pf = NULL;

	debugfs_remove_recursive(pf->ne6x_dbg_pf);
	pf->ne6x_dbg_pf = NULL;
}

/**
 * ne6x_dbg_init - start up debugfs for the driver
 **/
void ne6x_dbg_init(void)
{
	ne6x_dbg_root = debugfs_create_dir(ne6x_driver_name, NULL);
	if (!ne6x_dbg_root)
		pr_info("init of debugfs failed\n");
}

/**
 * ne6x_dbg_exit - clean out the driver's debugfs entries
 **/
void ne6x_dbg_exit(void)
{
	debugfs_remove_recursive(ne6x_dbg_root);
	ne6x_dbg_root = NULL;
}

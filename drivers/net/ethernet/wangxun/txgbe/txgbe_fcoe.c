/*
 * WangXun 10 Gigabit PCI Express Linux driver
 * Copyright (c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 * based on ixgbe_fcoe.c, Copyright(c) 1999 - 2017 Intel Corporation.
 * Contact Information:
 * Linux NICS <linux.nics@intel.com>
 * e1000-devel Mailing List <e1000-devel@lists.sourceforge.net>
 * Intel Corporation, 5200 N.E. Elam Young Parkway, Hillsboro, OR 97124-6497
 */

#include "txgbe.h"

#if IS_ENABLED(CONFIG_FCOE)
#if IS_ENABLED(CONFIG_DCB)
#include "txgbe_dcb.h"
#endif /* CONFIG_DCB */
#include <linux/if_ether.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/fc/fc_fs.h>
#include <scsi/fc/fc_fcoe.h>
#include <scsi/libfc.h>
#include <scsi/libfcoe.h>

/**
 * txgbe_fcoe_clear_ddp - clear the given ddp context
 * @ddp - ptr to the txgbe_fcoe_ddp
 *
 * Returns : none
 *
 */
static inline void txgbe_fcoe_clear_ddp(struct txgbe_fcoe_ddp *ddp)
{
	ddp->len = 0;
	ddp->err = 1;
	ddp->udl = NULL;
	ddp->udp = 0UL;
	ddp->sgl = NULL;
	ddp->sgc = 0;
}

/**
 * txgbe_fcoe_ddp_put - free the ddp context for a given xid
 * @netdev: the corresponding net_device
 * @xid: the xid that corresponding ddp will be freed
 *
 * This is the implementation of net_device_ops.ndo_fcoe_ddp_done
 * and it is expected to be called by ULD, i.e., FCP layer of libfc
 * to release the corresponding ddp context when the I/O is done.
 *
 * Returns : data length already ddp-ed in bytes
 */
int txgbe_fcoe_ddp_put(struct net_device *netdev, u16 xid)
{
	int len = 0;
	struct txgbe_fcoe *fcoe;
	struct txgbe_adapter *adapter;
	struct txgbe_hw *hw;
	struct txgbe_fcoe_ddp *ddp;
	u32 fcbuff;

	if (!netdev)
		goto out_ddp_put;

	if (xid > netdev->fcoe_ddp_xid)
		goto out_ddp_put;

	adapter = netdev_priv(netdev);
	hw = &adapter->hw;
	fcoe = &adapter->fcoe;
	ddp = &fcoe->ddp[xid];
	if (!ddp->udl)
		goto out_ddp_put;

	len = ddp->len;
	/* if there an error, force to invalidate ddp context */
	if (ddp->err) {

		/* other hardware requires DDP FCoE lock */
		spin_lock_bh(&fcoe->lock);

		wr32(hw, TXGBE_PSR_FC_FLT_CTXT, 0);
		wr32(hw, TXGBE_PSR_FC_FLT_RW,
				(xid | TXGBE_PSR_FC_FLT_RW_WE));
		wr32(hw, TXGBE_RDM_FCBUF, 0);
		wr32(hw, TXGBE_RDM_FCRW,
				(xid | TXGBE_RDM_FCRW_WE));

		/* read FCBUFF to check context invalidated */
		wr32(hw, TXGBE_RDM_FCRW,
				(xid | TXGBE_RDM_FCRW_RE));
		fcbuff = rd32(hw, TXGBE_RDM_FCBUF);

		spin_unlock_bh(&fcoe->lock);

		/* guaranteed to be invalidated after 100us */
		if (fcbuff & TXGBE_RDM_FCBUF_VALID)
			udelay(100);
	}
	if (ddp->sgl)
		dma_unmap_sg(pci_dev_to_dev(adapter->pdev), ddp->sgl, ddp->sgc,
			     DMA_FROM_DEVICE);
	if (ddp->pool) {
		dma_pool_free(ddp->pool, ddp->udl, ddp->udp);
		ddp->pool = NULL;
	}

	txgbe_fcoe_clear_ddp(ddp);

out_ddp_put:
	return len;
}

/**
 * txgbe_fcoe_ddp_setup - called to set up ddp context
 * @netdev: the corresponding net_device
 * @xid: the exchange id requesting ddp
 * @sgl: the scatter-gather list for this request
 * @sgc: the number of scatter-gather items
 *
 * Returns : 1 for success and 0 for no ddp
 */
static int txgbe_fcoe_ddp_setup(struct net_device *netdev, u16 xid,
				struct scatterlist *sgl, unsigned int sgc,
				int target_mode)
{
	struct txgbe_adapter *adapter;
	struct txgbe_hw *hw;
	struct txgbe_fcoe *fcoe;
	struct txgbe_fcoe_ddp *ddp;
	struct txgbe_fcoe_ddp_pool *ddp_pool;
	struct scatterlist *sg;
	unsigned int i, j, dmacount;
	unsigned int len;
	static const unsigned int bufflen = TXGBE_FCBUFF_MIN;
	unsigned int firstoff = 0;
	unsigned int lastsize;
	unsigned int thisoff = 0;
	unsigned int thislen = 0;
	u32 fcbuff, fcdmarw, fcfltrw, fcfltctxt;
	dma_addr_t addr = 0;

	if (!netdev || !sgl || !sgc)
		return 0;

	adapter = netdev_priv(netdev);
	if (xid > netdev->fcoe_ddp_xid) {
		e_warn(drv, "xid=0x%x out-of-range\n", xid);
		return 0;
	}

	/* no DDP if we are already down or resetting */
	if (test_bit(__TXGBE_DOWN, &adapter->state) ||
	    test_bit(__TXGBE_RESETTING, &adapter->state))
		return 0;

	fcoe = &adapter->fcoe;
	ddp = &fcoe->ddp[xid];
	if (ddp->sgl) {
		e_err(drv, "xid 0x%x w/ non-null sgl=%p nents=%d\n",
			xid, ddp->sgl, ddp->sgc);
		return 0;
	}
	txgbe_fcoe_clear_ddp(ddp);


	if (!fcoe->ddp_pool) {
		e_warn(drv, "No ddp_pool resources allocated\n");
		return 0;
	}

	ddp_pool = per_cpu_ptr(fcoe->ddp_pool, get_cpu());
	if (!ddp_pool->pool) {
		e_warn(drv, "xid=0x%x no ddp pool for fcoe\n", xid);
		goto out_noddp;
	}

	/* setup dma from scsi command sgl */
	dmacount = dma_map_sg(pci_dev_to_dev(adapter->pdev), sgl, sgc,
					     DMA_FROM_DEVICE);
	if (dmacount == 0) {
		e_err(drv, "xid 0x%x DMA map error\n", xid);
		goto out_noddp;
	}

	/* alloc the udl from per cpu ddp pool */
	ddp->udl = dma_pool_alloc(ddp_pool->pool, GFP_ATOMIC, &ddp->udp);
	if (!ddp->udl) {
		e_err(drv, "failed allocated ddp context\n");
		goto out_noddp_unmap;
	}
	ddp->pool = ddp_pool->pool;
	ddp->sgl = sgl;
	ddp->sgc = sgc;

	j = 0;
	for_each_sg(sgl, sg, dmacount, i) {
		addr = sg_dma_address(sg);
		len = sg_dma_len(sg);
		while (len) {
			/* max number of buffers allowed in one DDP context */
			if (j >= TXGBE_BUFFCNT_MAX) {
				ddp_pool->noddp++;
				goto out_noddp_free;
			}

			/* get the offset of length of current buffer */
			thisoff = addr & ((dma_addr_t)bufflen - 1);
			thislen = min((bufflen - thisoff), len);
			/*
			 * all but the 1st buffer (j == 0)
			 * must be aligned on bufflen
			 */
			if ((j != 0) && (thisoff))
				goto out_noddp_free;
			/*
			 * all but the last buffer
			 * ((i == (dmacount - 1)) && (thislen == len))
			 * must end at bufflen
			 */
			if (((i != (dmacount - 1)) || (thislen != len))
			    && ((thislen + thisoff) != bufflen))
				goto out_noddp_free;

			ddp->udl[j] = (u64)(addr - thisoff);
			/* only the first buffer may have none-zero offset */
			if (j == 0)
				firstoff = thisoff;
			len -= thislen;
			addr += thislen;
			j++;
		}
	}
	/* only the last buffer may have non-full bufflen */
	lastsize = thisoff + thislen;

	/*
	 * lastsize can not be bufflen.
	 * If it is then adding another buffer with lastsize = 1.
	 * Since lastsize is 1 there will be no HW access to this buffer.
	 */
	if (lastsize == bufflen) {
		if (j >= TXGBE_BUFFCNT_MAX) {
			ddp_pool->noddp_ext_buff++;
			goto out_noddp_free;
		}

		ddp->udl[j] = (u64)(fcoe->extra_ddp_buffer_dma);
		j++;
		lastsize = 1;
	}
	put_cpu();

	fcbuff = TXGBE_RDM_FCBUF_SIZE(TXGBE_FCBUFF_4KB) |
			TXGBE_RDM_FCBUF_COUNT(j) |
			TXGBE_RDM_FCBUF_OFFSET(firstoff) |
			TXGBE_RDM_FCBUF_VALID;

	/* Set WRCONTX bit to allow DDP for target */
	fcfltctxt = TXGBE_PSR_FC_FLT_CTXT_VALID;
	if (!target_mode)
		fcfltctxt |= TXGBE_PSR_FC_FLT_CTXT_WR;

	fcdmarw = xid | TXGBE_RDM_FCRW_WE |
		  TXGBE_RDM_FCRW_LASTSIZE(lastsize);

	fcfltrw = xid;
	fcfltrw |= TXGBE_PSR_FC_FLT_RW_WE;

	/* program DMA context */
	hw = &adapter->hw;

	/* turn on last frame indication for target mode as FCP_RSPtarget is
	 * supposed to send FCP_RSP when it is done. */
	if (target_mode && !test_bit(__TXGBE_FCOE_TARGET, &fcoe->mode)) {
		set_bit(__TXGBE_FCOE_TARGET, &fcoe->mode);
		wr32m(hw, TXGBE_PSR_FC_CTL,
			TXGBE_PSR_FC_CTL_LASTSEQH, TXGBE_PSR_FC_CTL_LASTSEQH);
	}

	/* other devices require DDP lock with direct DDP context access */
	spin_lock_bh(&fcoe->lock);

	wr32(hw, TXGBE_RDM_FCPTRL, ddp->udp & DMA_BIT_MASK(32));
	wr32(hw, TXGBE_RDM_FCPTRH, (u64)ddp->udp >> 32);
	wr32(hw, TXGBE_RDM_FCBUF, fcbuff);
	wr32(hw, TXGBE_RDM_FCRW, fcdmarw);
	/* program filter context */
	wr32(hw, TXGBE_PSR_FC_PARAM, 0);
	wr32(hw, TXGBE_PSR_FC_FLT_CTXT, fcfltctxt);
	wr32(hw, TXGBE_PSR_FC_FLT_RW, fcfltrw);

	spin_unlock_bh(&fcoe->lock);


	return 1;

out_noddp_free:
	dma_pool_free(ddp->pool, ddp->udl, ddp->udp);
	txgbe_fcoe_clear_ddp(ddp);

out_noddp_unmap:
	dma_unmap_sg(pci_dev_to_dev(adapter->pdev), sgl, sgc, DMA_FROM_DEVICE);
out_noddp:
	put_cpu();
	return 0;
}

/**
 * txgbe_fcoe_ddp_get - called to set up ddp context in initiator mode
 * @netdev: the corresponding net_device
 * @xid: the exchange id requesting ddp
 * @sgl: the scatter-gather list for this request
 * @sgc: the number of scatter-gather items
 *
 * This is the implementation of net_device_ops.ndo_fcoe_ddp_setup
 * and is expected to be called from ULD, e.g., FCP layer of libfc
 * to set up ddp for the corresponding xid of the given sglist for
 * the corresponding I/O.
 *
 * Returns : 1 for success and 0 for no ddp
 */
int txgbe_fcoe_ddp_get(struct net_device *netdev, u16 xid,
		       struct scatterlist *sgl, unsigned int sgc)
{
	return txgbe_fcoe_ddp_setup(netdev, xid, sgl, sgc, 0);
}

#ifdef HAVE_NETDEV_OPS_FCOE_DDP_TARGET
/**
 * txgbe_fcoe_ddp_target - called to set up ddp context in target mode
 * @netdev: the corresponding net_device
 * @xid: the exchange id requesting ddp
 * @sgl: the scatter-gather list for this request
 * @sgc: the number of scatter-gather items
 *
 * This is the implementation of net_device_ops.ndo_fcoe_ddp_target
 * and is expected to be called from ULD, e.g., FCP layer of libfc
 * to set up ddp for the corresponding xid of the given sglist for
 * the corresponding I/O. The DDP in target mode is a write I/O request
 * from the initiator.
 *
 * Returns : 1 for success and 0 for no ddp
 */
int txgbe_fcoe_ddp_target(struct net_device *netdev, u16 xid,
			  struct scatterlist *sgl, unsigned int sgc)
{
	return txgbe_fcoe_ddp_setup(netdev, xid, sgl, sgc, 1);
}

#endif /* HAVE_NETDEV_OPS_FCOE_DDP_TARGET */
/**
 * txgbe_fcoe_ddp - check ddp status and mark it done
 * @adapter: txgbe adapter
 * @rx_desc: advanced rx descriptor
 * @skb: the skb holding the received data
 *
 * This checks ddp status.
 *
 * Returns : < 0 indicates an error or not a FCoE ddp, 0 indicates
 * not passing the skb to ULD, > 0 indicates is the length of data
 * being ddped.
 */
int txgbe_fcoe_ddp(struct txgbe_adapter *adapter,
		   union txgbe_rx_desc *rx_desc,
		   struct sk_buff *skb)
{
	struct txgbe_fcoe *fcoe = &adapter->fcoe;
	struct txgbe_fcoe_ddp *ddp;
	struct fc_frame_header *fh;
	int rc = -EINVAL, ddp_max;
	__le32 fcerr = txgbe_test_staterr(rx_desc, TXGBE_RXD_ERR_FCERR);
	__le32 ddp_err;
	u32 fctl;
	u16 xid;

	if (fcerr == cpu_to_le32(TXGBE_FCERR_BADCRC))
		skb->ip_summed = CHECKSUM_NONE;
	else
		skb->ip_summed = CHECKSUM_UNNECESSARY;

	/* verify header contains at least the FCOE header */
	BUG_ON(skb_headlen(skb) < FCOE_HEADER_LEN);

	fh = (struct fc_frame_header *)(skb->data + sizeof(struct fcoe_hdr));

	if (skb->protocol == htons(ETH_P_8021Q))
		fh = (struct fc_frame_header *)((char *)fh + VLAN_HLEN);

	fctl = ntoh24(fh->fh_f_ctl);
	if (fctl & FC_FC_EX_CTX)
		xid =  ntohs(fh->fh_ox_id);
	else
		xid =  ntohs(fh->fh_rx_id);

	ddp_max = TXGBE_FCOE_DDP_MAX;

	if (xid >= ddp_max)
		goto ddp_out;

	ddp = &fcoe->ddp[xid];
	if (!ddp->udl)
		goto ddp_out;

	ddp_err = txgbe_test_staterr(rx_desc, TXGBE_RXD_ERR_FCEOFE |
					      TXGBE_RXD_ERR_FCERR);
	if (ddp_err)
		goto ddp_out;

	switch (txgbe_test_staterr(rx_desc, TXGBE_RXD_STAT_FCSTAT)) {
	/* return 0 to bypass going to ULD for DDPed data */
	case __constant_cpu_to_le32(TXGBE_RXD_STAT_FCSTAT_DDP):
		/* update length of DDPed data */
		ddp->len = le32_to_cpu(rx_desc->wb.lower.hi_dword.rss);
		rc = 0;
		break;
	/* unmap the sg list when FCPRSP is received */
	case __constant_cpu_to_le32(TXGBE_RXD_STAT_FCSTAT_FCPRSP):
		dma_unmap_sg(pci_dev_to_dev(adapter->pdev), ddp->sgl,
			     ddp->sgc, DMA_FROM_DEVICE);
		ddp->err = ddp_err;
		ddp->sgl = NULL;
		ddp->sgc = 0;
		/* fall through */
	/* if DDP length is present pass it through to ULD */
	case __constant_cpu_to_le32(TXGBE_RXD_STAT_FCSTAT_NODDP):
		/* update length of DDPed data */
		ddp->len = le32_to_cpu(rx_desc->wb.lower.hi_dword.rss);
		if (ddp->len)
			rc = ddp->len;
		break;
	/* no match will return as an error */
	case __constant_cpu_to_le32(TXGBE_RXD_STAT_FCSTAT_NOMTCH):
	default:
		break;
	}

	/* In target mode, check the last data frame of the sequence.
	 * For DDP in target mode, data is already DDPed but the header
	 * indication of the last data frame ould allow is to tell if we
	 * got all the data and the ULP can send FCP_RSP back, as this is
	 * not a full fcoe frame, we fill the trailer here so it won't be
	 * dropped by the ULP stack.
	 */
	if ((fh->fh_r_ctl == FC_RCTL_DD_SOL_DATA) &&
	    (fctl & FC_FC_END_SEQ)) {
		struct fcoe_crc_eof *crc;
		skb_linearize(skb);
		crc = (struct fcoe_crc_eof *)skb_put(skb, sizeof(*crc));
		crc->fcoe_eof = FC_EOF_T;
	}
ddp_out:
	return rc;
}

/**
 * txgbe_fso - txgbe FCoE Sequence Offload (FSO)
 * @tx_ring: tx desc ring
 * @first: first tx_buffer structure containing skb, tx_flags, and protocol
 * @hdr_len: hdr_len to be returned
 *
 * This sets up large send offload for FCoE
 *
 * Returns : 0 indicates success, < 0 for error
 */
int txgbe_fso(struct txgbe_ring *tx_ring,
	      struct txgbe_tx_buffer *first,
	      u8 *hdr_len)
{
	struct sk_buff *skb = first->skb;
	struct fc_frame_header *fh;
	u32 vlan_macip_lens;
	u32 fcoe_sof_eof = 0;
	u32 mss_l4len_idx;
	u8 sof, eof;

#ifdef NETIF_F_FSO
	if (skb_is_gso(skb) && skb_shinfo(skb)->gso_type != SKB_GSO_FCOE) {
		dev_err(tx_ring->dev, "Wrong gso type %d:expecting "
			"SKB_GSO_FCOE\n", skb_shinfo(skb)->gso_type);
		return -EINVAL;
	}

#endif
	/* resets the header to point fcoe/fc */
	skb_set_network_header(skb, skb->mac_len);
	skb_set_transport_header(skb, skb->mac_len +
				 sizeof(struct fcoe_hdr));

	/* sets up SOF and ORIS */
	sof = ((struct fcoe_hdr *)skb_network_header(skb))->fcoe_sof;
	switch (sof) {
	case FC_SOF_I2:
		fcoe_sof_eof = TXGBE_TXD_FCOEF_ORIS;
		break;
	case FC_SOF_I3:
		fcoe_sof_eof = TXGBE_TXD_FCOEF_SOF |
			       TXGBE_TXD_FCOEF_ORIS;
		break;
	case FC_SOF_N2:
		break;
	case FC_SOF_N3:
		fcoe_sof_eof = TXGBE_TXD_FCOEF_SOF;
		break;
	default:
		dev_warn(tx_ring->dev, "unknown sof = 0x%x\n", sof);
		return -EINVAL;
	}

	/* the first byte of the last dword is EOF */
	skb_copy_bits(skb, skb->len - 4, &eof, 1);
	/* sets up EOF and ORIE */
	switch (eof) {
	case FC_EOF_N:
		fcoe_sof_eof |= TXGBE_TXD_FCOEF_EOF_N;
		break;
	case FC_EOF_T:
		/* lso needs ORIE */
		if (skb_is_gso(skb))
			fcoe_sof_eof |= TXGBE_TXD_FCOEF_EOF_N |
					TXGBE_TXD_FCOEF_ORIE;
		else
			fcoe_sof_eof |= TXGBE_TXD_FCOEF_EOF_T;
		break;
	case FC_EOF_NI:
		fcoe_sof_eof |= TXGBE_TXD_FCOEF_EOF_NI;
		break;
	case FC_EOF_A:
		fcoe_sof_eof |= TXGBE_TXD_FCOEF_EOF_A;
		break;
	default:
		dev_warn(tx_ring->dev, "unknown eof = 0x%x\n", eof);
		return -EINVAL;
	}

	/* sets up PARINC indicating data offset */
	fh = (struct fc_frame_header *)skb_transport_header(skb);
	if (fh->fh_f_ctl[2] & FC_FC_REL_OFF)
		fcoe_sof_eof |= TXGBE_TXD_FCOEF_PARINC;

	/* include trailer in headlen as it is replicated per frame */
	*hdr_len = sizeof(struct fcoe_crc_eof);

	/* hdr_len includes fc_hdr if FCoE LSO is enabled */
	if (skb_is_gso(skb)) {
		*hdr_len += skb_transport_offset(skb) +
			    sizeof(struct fc_frame_header);
		/* update gso_segs and bytecount */
		first->gso_segs = DIV_ROUND_UP(skb->len - *hdr_len,
					       skb_shinfo(skb)->gso_size);
		first->bytecount += (first->gso_segs - 1) * *hdr_len;
		first->tx_flags |= TXGBE_TX_FLAGS_TSO;
	}

	/* set flag indicating FCOE to txgbe_tx_map call */
	first->tx_flags |= TXGBE_TX_FLAGS_FCOE | TXGBE_TX_FLAGS_CC;

	/* mss_l4len_id: use 0 for FSO as TSO, no need for L4LEN */
	mss_l4len_idx = skb_shinfo(skb)->gso_size << TXGBE_TXD_MSS_SHIFT;

	/* vlan_macip_lens: HEADLEN, MACLEN, VLAN tag */
	vlan_macip_lens = skb_transport_offset(skb) +
			  sizeof(struct fc_frame_header);
	vlan_macip_lens |= (skb_transport_offset(skb) - 4)
			   << TXGBE_TXD_MACLEN_SHIFT;
	vlan_macip_lens |= first->tx_flags & TXGBE_TX_FLAGS_VLAN_MASK;

	/* write context desc */
	txgbe_tx_ctxtdesc(tx_ring, vlan_macip_lens, fcoe_sof_eof,
			  TXGBE_TXD_TUCMD_FCOE, mss_l4len_idx);

	return 0;
}

static void txgbe_fcoe_dma_pool_free(struct txgbe_fcoe *fcoe, unsigned int cpu)
{
	struct txgbe_fcoe_ddp_pool *ddp_pool;

	ddp_pool = per_cpu_ptr(fcoe->ddp_pool, cpu);
	if (ddp_pool->pool)
		dma_pool_destroy(ddp_pool->pool);
	ddp_pool->pool = NULL;
}

static int txgbe_fcoe_dma_pool_alloc(struct txgbe_fcoe *fcoe,
				     struct device *dev,
				     unsigned int cpu)
{
	struct txgbe_fcoe_ddp_pool *ddp_pool;
	struct dma_pool *pool;
	char pool_name[32];

	snprintf(pool_name, 32, "txgbe_fcoe_ddp_%d", cpu);

	pool = dma_pool_create(pool_name, dev, TXGBE_FCPTR_MAX,
			       TXGBE_FCPTR_ALIGN, PAGE_SIZE);
	if (!pool)
		return -ENOMEM;

	ddp_pool = per_cpu_ptr(fcoe->ddp_pool, cpu);
	ddp_pool->pool = pool;
	ddp_pool->noddp = 0;
	ddp_pool->noddp_ext_buff = 0;

	return 0;
}

/**
 * txgbe_configure_fcoe - configures registers for fcoe at start
 * @adapter: ptr to txgbe adapter
 *
 * This sets up FCoE related registers
 *
 * Returns : none
 */
void txgbe_configure_fcoe(struct txgbe_adapter *adapter)
{
	struct txgbe_ring_feature *fcoe = &adapter->ring_feature[RING_F_FCOE];
	struct txgbe_hw *hw = &adapter->hw;
	int i, fcoe_i;
	u32 fcoe_q;
	u32 etqf;
	int fcreta_size;

	/* Minimal funcionality for FCoE requires at least CRC offloads */
	if (!(adapter->netdev->features & NETIF_F_FCOE_CRC))
		return;

	/* Enable L2 EtherType filter for FCoE, needed for FCoE CRC and DDP */
	etqf = ETH_P_FCOE | TXGBE_PSR_ETYPE_SWC_FCOE |
	       TXGBE_PSR_ETYPE_SWC_FILTER_EN;
	if (adapter->flags & TXGBE_FLAG_SRIOV_ENABLED) {
		etqf |= TXGBE_PSR_ETYPE_SWC_POOL_ENABLE;
		etqf |= VMDQ_P(0) << TXGBE_PSR_ETYPE_SWC_POOL_SHIFT;
	}
	wr32(hw,
			TXGBE_PSR_ETYPE_SWC(TXGBE_PSR_ETYPE_SWC_FILTER_FCOE),
			etqf);
	wr32(hw,
			TXGBE_RDB_ETYPE_CLS(TXGBE_PSR_ETYPE_SWC_FILTER_FCOE),
			0);

	/* leave remaining registers unconfigued if FCoE is disabled */
	if (!(adapter->flags & TXGBE_FLAG_FCOE_ENABLED))
		return;

	/* Use one or more Rx queues for FCoE by redirection table */
	fcreta_size = TXGBE_RDB_FCRE_TBL_SIZE;

	for (i = 0; i < fcreta_size; i++) {
		fcoe_i =
		    TXGBE_RDB_FCRE_TBL_RING(fcoe->offset + (i % fcoe->indices));
		fcoe_q = adapter->rx_ring[fcoe_i]->reg_idx;
		wr32(hw, TXGBE_RDB_FCRE_TBL(i), fcoe_q);
	}
	wr32(hw, TXGBE_RDB_FCRE_CTL, TXGBE_RDB_FCRE_CTL_ENA);

	/* Enable L2 EtherType filter for FIP */
	etqf = ETH_P_FIP | TXGBE_PSR_ETYPE_SWC_FILTER_EN;
	if (adapter->flags & TXGBE_FLAG_SRIOV_ENABLED) {
		etqf |= TXGBE_PSR_ETYPE_SWC_POOL_ENABLE;
		etqf |= VMDQ_P(0) << TXGBE_PSR_ETYPE_SWC_POOL_SHIFT;
	}
	wr32(hw, TXGBE_PSR_ETYPE_SWC(TXGBE_PSR_ETYPE_SWC_FILTER_FIP),
			etqf);

	/* Send FIP frames to the first FCoE queue */
	fcoe_q = adapter->rx_ring[fcoe->offset]->reg_idx;
	wr32(hw, TXGBE_RDB_ETYPE_CLS(TXGBE_PSR_ETYPE_SWC_FILTER_FIP),
			TXGBE_RDB_ETYPE_CLS_QUEUE_EN |
			(fcoe_q << TXGBE_RDB_ETYPE_CLS_RX_QUEUE_SHIFT));

	/* Configure FCoE Rx control */
	wr32(hw, TXGBE_PSR_FC_CTL,
			TXGBE_PSR_FC_CTL_FCCRCBO |
			TXGBE_PSR_FC_CTL_FCOEVER(FC_FCOE_VER) |
			TXGBE_PSR_FC_CTL_ALLH);
}

/**
 * txgbe_free_fcoe_ddp_resources - release all fcoe ddp context resources
 * @adapter : txgbe adapter
 *
 * Cleans up outstanding ddp context resources
 *
 * Returns : none
 */
void txgbe_free_fcoe_ddp_resources(struct txgbe_adapter *adapter)
{
	struct txgbe_fcoe *fcoe = &adapter->fcoe;
	int cpu, i, ddp_max;

	/* do nothing if no DDP pools were allocated */
	if (!fcoe->ddp_pool)
		return;

	ddp_max = TXGBE_FCOE_DDP_MAX;

	for (i = 0; i < ddp_max; i++)
		txgbe_fcoe_ddp_put(adapter->netdev, i);

	for_each_possible_cpu(cpu)
		txgbe_fcoe_dma_pool_free(fcoe, cpu);

	dma_unmap_single(pci_dev_to_dev(adapter->pdev),
			 fcoe->extra_ddp_buffer_dma,
			 TXGBE_FCBUFF_MIN,
			 DMA_FROM_DEVICE);
	kfree(fcoe->extra_ddp_buffer);

	fcoe->extra_ddp_buffer = NULL;
	fcoe->extra_ddp_buffer_dma = 0;
}

/**
 * txgbe_setup_fcoe_ddp_resources - setup all fcoe ddp context resources
 * @adapter: txgbe adapter
 *
 * Sets up ddp context resouces
 *
 * Returns : 0 indicates success or -EINVAL on failure
 */
int txgbe_setup_fcoe_ddp_resources(struct txgbe_adapter *adapter)
{
	struct txgbe_fcoe *fcoe = &adapter->fcoe;
	struct device *dev = pci_dev_to_dev(adapter->pdev);
	void *buffer;
	dma_addr_t dma;
	unsigned int cpu;

	/* do nothing if no DDP pools were allocated */
	if (!fcoe->ddp_pool)
		return 0;

	/* Extra buffer to be shared by all DDPs for HW work around */
	buffer = kmalloc(TXGBE_FCBUFF_MIN, GFP_ATOMIC);
	if (!buffer) {
		e_err(drv, "failed to allocate extra DDP buffer\n");
		return -ENOMEM;
	}

	dma = dma_map_single(dev, buffer, TXGBE_FCBUFF_MIN, DMA_FROM_DEVICE);
	if (dma_mapping_error(dev, dma)) {
		e_err(drv, "failed to map extra DDP buffer\n");
		kfree(buffer);
		return -ENOMEM;
	}

	fcoe->extra_ddp_buffer = buffer;
	fcoe->extra_ddp_buffer_dma = dma;

	/* allocate pci pool for each cpu */
	for_each_possible_cpu(cpu) {
		int err = txgbe_fcoe_dma_pool_alloc(fcoe, dev, cpu);
		if (!err)
			continue;

		e_err(drv, "failed to alloc DDP pool on cpu:%d\n", cpu);
		txgbe_free_fcoe_ddp_resources(adapter);
		return -ENOMEM;
	}

	return 0;
}

#ifndef HAVE_NETDEV_OPS_FCOE_ENABLE
int txgbe_fcoe_ddp_enable(struct txgbe_adapter *adapter)
#else
static int txgbe_fcoe_ddp_enable(struct txgbe_adapter *adapter)
#endif
{
	struct txgbe_fcoe *fcoe = &adapter->fcoe;

	if (!(adapter->flags & TXGBE_FLAG_FCOE_CAPABLE))
		return -EINVAL;

	fcoe->ddp_pool = alloc_percpu(struct txgbe_fcoe_ddp_pool);

	if (!fcoe->ddp_pool) {
		e_err(drv, "failed to allocate percpu DDP resources\n");
		return -ENOMEM;
	}

	adapter->netdev->fcoe_ddp_xid = TXGBE_FCOE_DDP_MAX - 1;

	return 0;
}

#ifndef HAVE_NETDEV_OPS_FCOE_ENABLE
void txgbe_fcoe_ddp_disable(struct txgbe_adapter *adapter)
#else
static void txgbe_fcoe_ddp_disable(struct txgbe_adapter *adapter)
#endif
{
	struct txgbe_fcoe *fcoe = &adapter->fcoe;

	adapter->netdev->fcoe_ddp_xid = 0;

	if (!fcoe->ddp_pool)
		return;

	free_percpu(fcoe->ddp_pool);
	fcoe->ddp_pool = NULL;
}

#ifdef HAVE_NETDEV_OPS_FCOE_ENABLE
/**
 * txgbe_fcoe_enable - turn on FCoE offload feature
 * @netdev: the corresponding netdev
 *
 * Turns on FCoE offload feature in sapphire.
 *
 * Returns : 0 indicates success or -EINVAL on failure
 */
int txgbe_fcoe_enable(struct net_device *netdev)
{
	struct txgbe_adapter *adapter = netdev_priv(netdev);
	struct txgbe_fcoe *fcoe = &adapter->fcoe;

	atomic_inc(&fcoe->refcnt);

	if (!(adapter->flags & TXGBE_FLAG_FCOE_CAPABLE))
		return -EINVAL;

	if (adapter->flags & TXGBE_FLAG_FCOE_ENABLED)
		return -EINVAL;

	e_info(drv, "Enabling FCoE offload features.\n");

	if (adapter->flags & TXGBE_FLAG_SRIOV_ENABLED)
		e_warn(probe, "Enabling FCoE on PF will disable legacy VFs\n");

	if (netif_running(netdev))
		netdev->netdev_ops->ndo_stop(netdev);

	/* Allocate per CPU memory to track DDP pools */
	txgbe_fcoe_ddp_enable(adapter);

	/* enable FCoE and notify stack */
	adapter->flags |= TXGBE_FLAG_FCOE_ENABLED;
	netdev->features |= NETIF_F_FCOE_MTU;
	netdev_features_change(netdev);

	/* release existing queues and reallocate them */
	txgbe_clear_interrupt_scheme(adapter);
	txgbe_init_interrupt_scheme(adapter);

	if (netif_running(netdev))
		netdev->netdev_ops->ndo_open(netdev);

	return 0;
}

/**
 * txgbe_fcoe_disable - turn off FCoE offload feature
 * @netdev: the corresponding netdev
 *
 * Turns off FCoE offload feature in sapphire.
 *
 * Returns : 0 indicates success or -EINVAL on failure
 */
int txgbe_fcoe_disable(struct net_device *netdev)
{
	struct txgbe_adapter *adapter = netdev_priv(netdev);

	if (!atomic_dec_and_test(&adapter->fcoe.refcnt))
		return -EINVAL;

	if (!(adapter->flags & TXGBE_FLAG_FCOE_ENABLED))
		return -EINVAL;

	e_info(drv, "Disabling FCoE offload features.\n");
	if (netif_running(netdev))
		netdev->netdev_ops->ndo_stop(netdev);

	/* Free per CPU memory to track DDP pools */
	txgbe_fcoe_ddp_disable(adapter);

	/* disable FCoE and notify stack */
	adapter->flags &= ~TXGBE_FLAG_FCOE_ENABLED;
	netdev->features &= ~NETIF_F_FCOE_MTU;

	netdev_features_change(netdev);

	/* release existing queues and reallocate them */
	txgbe_clear_interrupt_scheme(adapter);
	txgbe_init_interrupt_scheme(adapter);

	if (netif_running(netdev))
		netdev->netdev_ops->ndo_open(netdev);

	return 0;
}
#endif /* HAVE_NETDEV_OPS_FCOE_ENABLE */

#if IS_ENABLED(CONFIG_DCB)
#ifdef HAVE_DCBNL_OPS_GETAPP
/**
 * txgbe_fcoe_getapp - retrieves current user priority bitmap for FCoE
 * @netdev: the corresponding net_device
 *
 * Finds out the corresponding user priority bitmap from the current
 * traffic class that FCoE belongs to. Returns 0 as the invalid user
 * priority bitmap to indicate an error.
 *
 * Returns : 802.1p user priority bitmap for FCoE
 */
u8 txgbe_fcoe_getapp(struct net_device *netdev)
{
	struct txgbe_adapter *adapter = netdev_priv(netdev);
	return 1 << adapter->fcoe.up;
}
#endif /* HAVE_DCBNL_OPS_GETAPP */
#endif /* CONFIG_DCB */
#ifdef HAVE_NETDEV_OPS_FCOE_GETWWN
/**
 * txgbe_fcoe_get_wwn - get world wide name for the node or the port
 * @netdev : txgbe adapter
 * @wwn : the world wide name
 * @type: the type of world wide name
 *
 * Returns the node or port world wide name if both the prefix and the san
 * mac address are valid, then the wwn is formed based on the NAA-2 for
 * IEEE Extended name identifier (ref. to T10 FC-LS Spec., Sec. 15.3).
 *
 * Returns : 0 on success
 */
int txgbe_fcoe_get_wwn(struct net_device *netdev, u64 *wwn, int type)
{
	int rc = -EINVAL;
	u16 prefix = 0xffff;
	struct txgbe_adapter *adapter = netdev_priv(netdev);
	struct txgbe_mac_info *mac = &adapter->hw.mac;

	switch (type) {
	case NETDEV_FCOE_WWNN:
		prefix = mac->wwnn_prefix;
		break;
	case NETDEV_FCOE_WWPN:
		prefix = mac->wwpn_prefix;
		break;
	default:
		break;
	}

	if ((prefix != 0xffff) &&
	    is_valid_ether_addr(mac->san_addr)) {
		*wwn = ((u64) prefix << 48) |
		       ((u64) mac->san_addr[0] << 40) |
		       ((u64) mac->san_addr[1] << 32) |
		       ((u64) mac->san_addr[2] << 24) |
		       ((u64) mac->san_addr[3] << 16) |
		       ((u64) mac->san_addr[4] << 8)  |
		       ((u64) mac->san_addr[5]);
		rc = 0;
	}
	return rc;
}

#endif /* HAVE_NETDEV_OPS_FCOE_GETWWN */
/**
 * txgbe_fcoe_get_tc - get the current TC that fcoe is mapped to
 * @adapter - pointer to the device adapter structure
 *
 * Return : TC that FCoE is mapped to
 */
u8 txgbe_fcoe_get_tc(struct txgbe_adapter *adapter)
{
	return netdev_get_prio_tc_map(adapter->netdev, adapter->fcoe.up);
}
#endif /* CONFIG_FCOE */

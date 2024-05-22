/*++

Copyright (c) 2021 Motorcomm Corporation.
Confidential and Proprietary. All rights reserved.

This is Motorcomm Corporation NIC driver relevant files. Please don't copy, modify,
distribute without commercial permission.

--*/


#include "fuxi-gmac.h"
#include "fuxi-gmac-reg.h"

#if defined(UEFI)
#include "nic_sw.h"
#elif defined(_WIN32) || defined(_WIN64)
#include "fuxi-mp.h"
#endif

static void fxgmac_unmap_desc_data(struct fxgmac_pdata* pdata,
	struct fxgmac_desc_data* desc_data)
{
#ifndef LINUX
    pdata = pdata;
    desc_data = desc_data;
#else
    if (desc_data->skb_dma) {
    	if (desc_data->mapped_as_page) {
    		dma_unmap_page(pdata->dev, desc_data->skb_dma,
    			desc_data->skb_dma_len, DMA_TO_DEVICE);
    	}
    	else {
    		dma_unmap_single(pdata->dev, desc_data->skb_dma,
    			desc_data->skb_dma_len, DMA_TO_DEVICE);
    	}
    	desc_data->skb_dma = 0;
    	desc_data->skb_dma_len = 0;
    }

    if (desc_data->rx.buf.dma_base) {
        dma_unmap_single(pdata->dev, desc_data->rx.buf.dma_base,
                    pdata->rx_buf_size, DMA_FROM_DEVICE);
        desc_data->rx.buf.dma_base = 0;
    }

    if (desc_data->skb) {
        dev_kfree_skb_any(desc_data->skb);
        desc_data->skb = NULL;
    }

#if 0
    if (desc_data->rx.hdr.pa.pages)
    	put_page(desc_data->rx.hdr.pa.pages);

    if (desc_data->rx.hdr.pa_unmap.pages) {
    	dma_unmap_page(pdata->dev, desc_data->rx.hdr.pa_unmap.pages_dma,
    		desc_data->rx.hdr.pa_unmap.pages_len,
    		DMA_FROM_DEVICE);
    	put_page(desc_data->rx.hdr.pa_unmap.pages);
    }

    if (desc_data->rx.buf.pa.pages)
    	put_page(desc_data->rx.buf.pa.pages);

    if (desc_data->rx.buf.pa_unmap.pages) {
    	dma_unmap_page(pdata->dev, desc_data->rx.buf.pa_unmap.pages_dma,
    		desc_data->rx.buf.pa_unmap.pages_len,
    		DMA_FROM_DEVICE);
    	put_page(desc_data->rx.buf.pa_unmap.pages);
    }
#endif
    memset(&desc_data->tx, 0, sizeof(desc_data->tx));
    memset(&desc_data->rx, 0, sizeof(desc_data->rx));

    desc_data->mapped_as_page = 0;

    if (desc_data->state_saved) {
    	desc_data->state_saved = 0;
    	desc_data->state.skb = NULL;
    	desc_data->state.len = 0;
    	desc_data->state.error = 0;
    }
#endif
}

static void fxgmac_free_ring(struct fxgmac_pdata* pdata,
	struct fxgmac_ring* ring)
{
#ifndef LINUX
    pdata = pdata;

    if (!ring)
    	return;
#else
    struct fxgmac_desc_data* desc_data;
    unsigned int i;

    if (!ring)
    	return;

    if (ring->desc_data_head) {
    	for (i = 0; i < ring->dma_desc_count; i++) {
    		desc_data = FXGMAC_GET_DESC_DATA(ring, i);
    		fxgmac_unmap_desc_data(pdata, desc_data);
    	}

    	kfree(ring->desc_data_head);
    	ring->desc_data_head = NULL;
    }

#if 0
    if (ring->rx_hdr_pa.pages) {
    	dma_unmap_page(pdata->dev, ring->rx_hdr_pa.pages_dma,
    		ring->rx_hdr_pa.pages_len, DMA_FROM_DEVICE);
    	put_page(ring->rx_hdr_pa.pages);

    	ring->rx_hdr_pa.pages = NULL;
    	ring->rx_hdr_pa.pages_len = 0;
    	ring->rx_hdr_pa.pages_offset = 0;
    	ring->rx_hdr_pa.pages_dma = 0;
    }

    if (ring->rx_buf_pa.pages) {
    	dma_unmap_page(pdata->dev, ring->rx_buf_pa.pages_dma,
    		ring->rx_buf_pa.pages_len, DMA_FROM_DEVICE);
    	put_page(ring->rx_buf_pa.pages);

    	ring->rx_buf_pa.pages = NULL;
    	ring->rx_buf_pa.pages_len = 0;
    	ring->rx_buf_pa.pages_offset = 0;
    	ring->rx_buf_pa.pages_dma = 0;
    }
#endif

    if (ring->dma_desc_head) {
    	dma_free_coherent(pdata->dev,
    		(sizeof(struct fxgmac_dma_desc) *
    			ring->dma_desc_count),
    		ring->dma_desc_head,
    		ring->dma_desc_head_addr);
    	ring->dma_desc_head = NULL;
    }
#endif
}

static int fxgmac_init_ring(struct fxgmac_pdata* pdata,
	struct fxgmac_ring* ring,
	unsigned int dma_desc_count)
{
    if (!ring)
        return 0;

#ifndef LINUX
    pdata = pdata;
    dma_desc_count = dma_desc_count;
    ring->dma_desc_count = 0;

    return 0;
#else
    /* Descriptors */
    ring->dma_desc_count = dma_desc_count;
    ring->dma_desc_head = dma_alloc_coherent(pdata->dev,
    	(sizeof(struct fxgmac_dma_desc) *
    		dma_desc_count),
    	&ring->dma_desc_head_addr,
    	GFP_KERNEL);
    if (!ring->dma_desc_head)
    	return -ENOMEM;

    /* Array of descriptor data */
    ring->desc_data_head = kcalloc(dma_desc_count,
    	sizeof(struct fxgmac_desc_data),
    	GFP_KERNEL);
    if (!ring->desc_data_head)
    	return -ENOMEM;

    netif_dbg(pdata, drv, pdata->netdev,
    	"dma_desc_head=%p, dma_desc_head_addr=%pad, desc_data_head=%p\n",
    	ring->dma_desc_head,
    	&ring->dma_desc_head_addr,
    	ring->desc_data_head);

    return 0;
#endif
}

static void fxgmac_free_rings(struct fxgmac_pdata* pdata)
{
    struct fxgmac_channel* channel;
    unsigned int i;

    if (!pdata->channel_head)
    	return;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
    	fxgmac_free_ring(pdata, channel->tx_ring);
    	fxgmac_free_ring(pdata, channel->rx_ring);
    }
}

static int fxgmac_alloc_rings(struct fxgmac_pdata* pdata)
{
    struct fxgmac_channel* channel;
    unsigned int i;
    int ret;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
            netif_dbg(pdata, drv, pdata->netdev, "%s - Tx ring:\n",
            channel->name);

    		if (i < pdata->tx_ring_count)
    		{
                ret = fxgmac_init_ring(pdata, channel->tx_ring,
                    pdata->tx_desc_count);

                if (ret) {
                    netdev_alert(pdata->netdev, "error initializing Tx ring");
                    goto err_init_ring;
                }
    		}

        netif_dbg(pdata, drv, pdata->netdev, "%s - Rx ring:\n", channel->name);

    	ret = fxgmac_init_ring(pdata, channel->rx_ring,
    		pdata->rx_desc_count);
    	if (ret) {
    		netdev_alert(pdata->netdev,
    			"error initializing Rx ring\n");
    		goto err_init_ring;
    	}
    	if(netif_msg_drv(pdata)) DPRINTK("fxgmac_alloc_ring..ch=%u,tx_desc_cnt=%u,rx_desc_cnt=%u\n",i,pdata->tx_desc_count,pdata->rx_desc_count);
    }
    if(netif_msg_drv(pdata)) DPRINTK("alloc_rings callout ok\n");

    return 0;

err_init_ring:
    fxgmac_free_rings(pdata);

    DPRINTK("alloc_rings callout err,%d\n",ret);
    return ret;
}

#ifdef LINUX
static void fxgmac_free_channels(struct fxgmac_pdata *pdata)
{
    if (!pdata->channel_head)
    	return;
    if(netif_msg_drv(pdata)) DPRINTK("free_channels,tx_ring=%p\n", pdata->channel_head->tx_ring);
    kfree(pdata->channel_head->tx_ring);
    pdata->channel_head->tx_ring = NULL;

    if(netif_msg_drv(pdata)) DPRINTK("free_channels,rx_ring=%p\n", pdata->channel_head->rx_ring);
    kfree(pdata->channel_head->rx_ring);
    pdata->channel_head->rx_ring = NULL;

    if(netif_msg_drv(pdata)) DPRINTK("free_channels,channel=%p\n", pdata->channel_head);
    kfree(pdata->channel_head);

    pdata->channel_head = NULL;
    //comment out below line for that, channel_count is initialized only once in hw_init()
    //pdata->channel_count = 0;
}
#else
static void fxgmac_free_channels(struct fxgmac_pdata* pdata)
{
    if (!pdata->channel_head)
    	return;

    //kfree(pdata->channel_head->tx_ring);
    pdata->channel_head->tx_ring = NULL;

    //kfree(pdata->channel_head->rx_ring);
    pdata->channel_head->rx_ring = NULL;

    //kfree(pdata->channel_head);

    pdata->channel_head = NULL;
    pdata->channel_count = 0;
}
#endif

#if defined(UEFI)
static int fxgmac_alloc_channels(struct fxgmac_pdata* pdata)
{
    struct fxgmac_channel* channel_head, * channel;
    struct fxgmac_ring* tx_ring, * rx_ring;
    int ret = -ENOMEM;
    unsigned int i;
    EFI_STATUS  Status;
    UINT64              vir_addr;
    UINT64              cache_sz = CACHE_ALIGN_SZ;
    PADAPTER    adpt = (PADAPTER)pdata->pAdapter;
    //
    // Allocate memory for transmit and receive resources.
    //
    DEBUGPRINT(INIT, "[%a,%a,%d]:\n",__FILE__, __func__,__LINE__);
    Status = adpt->Io_Function->AllocateBuffer(
        adpt->Io_Function,
        AllocateAnyPages,
        EfiRuntimeServicesData,
        UNDI_MEM_PAGES(CHANNEL_MEMORY_NEEDED),
        (VOID**)&(adpt->MemoryChannelPtr),
        0
    );

    if (EFI_ERROR(Status)) {
        DEBUGPRINT(INIT, ("Error  PCI IO AllocateBuffer returns \n"));
	goto err;
    }

    DEBUGPRINT(INIT, "[%a,%a,%d]:status=%x,MemoryChannelPtr=%llx\n",__FILE__, __func__,__LINE__,Status,adpt->MemoryChannelPtr);
    vir_addr = (adpt->MemoryChannelPtr + cache_sz) & (~(cache_sz - 1));
#ifdef UEFI_64
    channel_head = (struct fxgmac_channel*)vir_addr;
#else
    channel_head = (struct fxgmac_channel*)(UINT32)vir_addr;
#endif

    DEBUGPRINT(INIT, "[%a,%a,%d]:Channel_head=%llx,vir_addr=%llx\n",__FILE__, __func__,__LINE__,channel_head,vir_addr);
    netif_dbg(pdata, drv, pdata->netdev,
        "channel_head=%p\n", channel_head);

    vir_addr = (vir_addr + 4 * sizeof(struct fxgmac_channel) + cache_sz) & (~(cache_sz - 1));
#ifdef UEFI_64
    tx_ring = (struct fxgmac_ring*)vir_addr;
#else
    tx_ring = (struct fxgmac_ring*)(UINT32)vir_addr;
#endif


    vir_addr = (vir_addr + 4 * sizeof(struct fxgmac_ring) + cache_sz) & (~(cache_sz - 1));
#ifdef UEFI_64
    rx_ring = (struct fxgmac_ring*)vir_addr;
#else
    rx_ring = (struct fxgmac_ring*)(UINT32)vir_addr;
#endif

    DEBUGPRINT(INIT, "[%a,%a,%d]:Channel_head=%llx,*channel_head=%llx,vir_addr=%llx,tx_ring=%llx,rx_ring=%llx,channelcount=%llx,pdata=%llx\n",__FILE__, __func__,__LINE__,channel_head,*channel_head,vir_addr,tx_ring,rx_ring,pdata->channel_count,&channel_head->pdata);
    for (i = 0, channel = channel_head; i < pdata->channel_count;
        i++, channel++) {
        //snprintf(channel->name, sizeof(channel->name), "channel-%u", i);
        //RtlStringCchPrintfA(channel->name, sizeof(channel->name), "channel-%u", i);
              //netif_dbg(pdata, drv, pdata->netdev,"channel-%u\n", i);

    	DEBUGPRINT(INIT, "[%a,%a,%d]:channel=%llx,&channel->pdata=%llx\n",__FILE__, __func__,__LINE__,channel,&channel->pdata);
        channel->pdata = pdata;
    	DEBUGPRINT(INIT, "[%a,%d]:channel->pdata:%llx,pdata:%llx\n",__func__,__LINE__,channel->pdata,pdata);
        channel->queue_index = i;
        channel->dma_regs = pdata->mac_regs + DMA_CH_BASE +
            (DMA_CH_INC * i);

        if (pdata->per_channel_irq) {
            /* Get the per DMA interrupt */
            ret = pdata->channel_irq[i];
            if (ret < 0) {
                netdev_err(pdata->netdev,
                    "get_irq %u failed\n",
                    i + 1);
                goto err;
            }
            channel->dma_irq = ret;
        }

        if (i < pdata->tx_ring_count)
            channel->tx_ring = tx_ring++;

        if (i < pdata->rx_ring_count)
            channel->rx_ring = rx_ring++;
        netif_dbg(pdata, drv, pdata->netdev,
            ""STR_FORMAT": dma_regs=%p, tx_ring=%p, rx_ring=%p\n",
            channel->name, channel->dma_regs,
            channel->tx_ring, channel->rx_ring);

    DEBUGPRINT(INIT, "[%a,%d]:Channel_dma_regs=%llx,channel txring=%llx,channel rx_ring=%llx,i=%llx,channel_count=%llx\n", __func__,__LINE__,channel->dma_regs,channel->tx_ring,channel->rx_ring,i,pdata->channel_count);
    }

    pdata->channel_head = channel_head;

    return 0;

    err:
    return ret;
}
#elif defined(LINUX)
static int fxgmac_alloc_channels(struct fxgmac_pdata *pdata)
{
    struct fxgmac_channel *channel_head, *channel;
    struct fxgmac_ring *tx_ring, *rx_ring;
    int ret = -ENOMEM;
    unsigned int i;

#ifdef CONFIG_PCI_MSI
    u32 msix = FXGMAC_GET_REG_BITS(pdata->expansion.int_flags,
                                    FXGMAC_FLAG_MSIX_POS,
                                    FXGMAC_FLAG_MSIX_LEN);
#endif

    channel_head = kcalloc(pdata->channel_count,
    		       sizeof(struct fxgmac_channel), GFP_KERNEL);
    if(netif_msg_drv(pdata)) DPRINTK("alloc_channels,channel_head=%p,size=%d*%ld\n", channel_head, pdata->channel_count,sizeof(struct fxgmac_channel));

    if (!channel_head)
    	return ret;

    netif_dbg(pdata, drv, pdata->netdev,
    	  "channel_head=%p\n", channel_head);

    tx_ring = kcalloc(pdata->tx_ring_count, sizeof(struct fxgmac_ring),
    		  GFP_KERNEL);
    if (!tx_ring)
    	goto err_tx_ring;

    if(netif_msg_drv(pdata)) DPRINTK("alloc_channels,tx_ring=%p,size=%d*%ld\n", tx_ring, pdata->tx_ring_count,sizeof(struct fxgmac_ring));
    rx_ring = kcalloc(pdata->rx_ring_count, sizeof(struct fxgmac_ring),
    		  GFP_KERNEL);
    if (!rx_ring)
    	goto err_rx_ring;

    if(netif_msg_drv(pdata)) DPRINTK("alloc_channels,rx_ring=%p,size=%d*%ld\n", rx_ring, pdata->rx_ring_count,sizeof(struct fxgmac_ring));
    //DPRINTK("fxgmac_alloc_channels ch_num=%d,rxring=%d,txring=%d\n",pdata->channel_count, pdata->rx_ring_count,pdata->tx_ring_count);

    for (i = 0, channel = channel_head; i < pdata->channel_count;
    	i++, channel++) {
    	snprintf(channel->name, sizeof(channel->name), "channel-%u", i);
    	channel->pdata = pdata;
    	channel->queue_index = i;
    	channel->dma_regs = pdata->mac_regs + DMA_CH_BASE +
    			    (DMA_CH_INC * i);

    	if (pdata->per_channel_irq) {
    		/* Get the per DMA interrupt */
#ifdef CONFIG_PCI_MSI
    		//20210526 for MSIx
    		if(msix) {
    			pdata->channel_irq[i] = pdata->expansion.msix_entries[i].vector;
    			if (FXGMAC_IS_CHANNEL_WITH_TX_IRQ(i)) {
    				pdata->channel_irq[FXGMAC_MAX_DMA_CHANNELS] = pdata->expansion.msix_entries[FXGMAC_MAX_DMA_CHANNELS].vector;

    				if (pdata->channel_irq[FXGMAC_MAX_DMA_CHANNELS] < 0) {
    					netdev_err(pdata->netdev,
    						   "get_irq %u for tx failed\n",
    						   i + 1);
    					goto err_irq;
    				}

    				channel->expansion.dma_irq_tx = pdata->channel_irq[FXGMAC_MAX_DMA_CHANNELS];
    				DPRINTK("fxgmac_alloc_channels, for MSIx, channel %d dma_irq_tx=%u\n", i, channel->expansion.dma_irq_tx);
    			}
    		}
#endif
    		ret = pdata->channel_irq[i];
    		if (ret < 0) {
    			netdev_err(pdata->netdev,
    				   "get_irq %u failed\n",
    				   i + 1);
    			goto err_irq;
    		}
    		channel->dma_irq = ret;
    		DPRINTK("fxgmac_alloc_channels, for MSIx, channel %d dma_irq=%u\n", i, channel->dma_irq);
    	}

    	if (i < pdata->tx_ring_count)
    		channel->tx_ring = tx_ring++;

    	if (i < pdata->rx_ring_count)
    		channel->rx_ring = rx_ring++;

    	netif_dbg(pdata, drv, pdata->netdev,
    		  "%s: dma_regs=%p, tx_ring=%p, rx_ring=%p\n",
    		  channel->name, channel->dma_regs,
    		  channel->tx_ring, channel->rx_ring);
    }

    pdata->channel_head = channel_head;

    if(netif_msg_drv(pdata)) DPRINTK("alloc_channels callout ok\n");
    return 0;

err_irq:
    kfree(rx_ring);

err_rx_ring:
    kfree(tx_ring);

err_tx_ring:
    kfree(channel_head);

    DPRINTK("fxgmac alloc_channels callout err,%d\n",ret);
    return ret;
}
#elif defined(_WIN32) || defined(_WIN64)
static int fxgmac_alloc_channels(struct fxgmac_pdata* pdata)
{
    struct fxgmac_channel* channel_head, * channel;
    struct fxgmac_ring* tx_ring, * rx_ring;
    PMP_ADAPTER pAdapter = (PMP_ADAPTER)pdata->pAdapter;
    int ret = -ENOMEM;
    unsigned int i;

    //channel_head = kcalloc(pdata->channel_count,
    //	sizeof(struct fxgmac_channel), GFP_KERNEL);
    //if (!channel_head)
    //	return ret;

    channel_head = &pAdapter->MpChannelRingResources.fx_channel[0];

    netif_dbg(pdata, drv, pdata->netdev,
        "channel_head=%p\n", channel_head);

    //tx_ring = kcalloc(pdata->tx_ring_count, sizeof(struct fxgmac_ring),
    //	GFP_KERNEL);
    //if (!tx_ring)
    //	goto err_tx_ring;
    tx_ring = &pAdapter->MpChannelRingResources.fx_tx_ring[0];

    //rx_ring = kcalloc(pdata->rx_ring_count, sizeof(struct fxgmac_ring),
    //	GFP_KERNEL);
    //if (!rx_ring)
    //	goto err_rx_ring;
    rx_ring = &pAdapter->MpChannelRingResources.fx_rx_ring[0];

    for (i = 0, channel = channel_head; i < pdata->channel_count;
        i++, channel++) {
        //snprintf(channel->name, sizeof(channel->name), "channel-%u", i);
        RtlStringCchPrintfA(channel->name, sizeof(channel->name), "channel-%u", i);

        channel->pdata = pdata;
        channel->queue_index = i;
        channel->dma_regs = pdata->mac_regs + DMA_CH_BASE +
            (DMA_CH_INC * i);

        if (pdata->per_channel_irq) {
            /* Get the per DMA interrupt */
            ret = pdata->channel_irq[i];
            if (ret < 0) {
                netdev_err(pdata->netdev,
                    "get_irq %u failed\n",
                    i + 1);
                goto err_irq;
            }
            channel->dma_irq = ret;
        }

        if (i < pdata->tx_ring_count)
            channel->tx_ring = tx_ring++;

        if (i < pdata->rx_ring_count)
            channel->rx_ring = rx_ring++;

        netif_dbg(pdata, drv, pdata->netdev,
            "%s: dma_regs=%p, tx_ring=%p, rx_ring=%p\n",
            channel->name, channel->dma_regs,
            channel->tx_ring, channel->rx_ring);
    }

    pdata->channel_head = channel_head;

    return 0;

err_irq:
    //kfree(rx_ring);

//err_rx_ring:
    //kfree(tx_ring);

//err_tx_ring:
    //kfree(channel_head);

    return ret;

}
#elif defined(UBOOT)
static int fxgmac_alloc_channels(struct fxgmac_pdata* pdata)
{
     struct fxgmac_channel * channel;
     unsigned int i;

     for (i = 0; i < pdata->channel_count; i++)
     {
         //snprintf(channel->name, sizeof(channel->name), "channel-%u", i);
         //RtlStringCchPrintfA(channel->name, sizeof(channel->name), "channel-%u"     , i);
               //netif_dbg(pdata, drv, pdata->netdev,"channel-%u\n", i);
         channel = pdata->channel_head;

         channel->pdata = pdata;
         channel->queue_index = i;
         channel->dma_regs = pdata->mac_regs + DMA_CH_BASE +
             (DMA_CH_INC * i);

         /* set tx/rx channel*/
#if 1
         pdata->expansion.tx_channel = pdata->channel_head;

         pdata->expansion.rx_channel = pdata->channel_head;
#else
         if(i == 0)
             pdata->tx_channel = &pdata->channel_head[i];
         else
             pdata->rx_channel = &pdata->channel_head[i];
#endif
    }
    return 0;
}
#else
static struct fxgmac_channel fx_channel[4];
static struct fxgmac_ring fx_tx_ring[4];
static struct fxgmac_ring fx_rx_ring[4];

static int fxgmac_alloc_channels(struct fxgmac_pdata* pdata)
{
    struct fxgmac_channel* channel_head, * channel;
    struct fxgmac_ring* tx_ring, * rx_ring;
    int ret = -ENOMEM;
    unsigned int i;

    //channel_head = kcalloc(pdata->channel_count,
    //	sizeof(struct fxgmac_channel), GFP_KERNEL);
    //if (!channel_head)
    //	return ret;
    channel_head = &fx_channel[0];

    netif_dbg(pdata, drv, pdata->netdev,
    	"channel_head=%p\n", channel_head);

    //tx_ring = kcalloc(pdata->tx_ring_count, sizeof(struct fxgmac_ring),
    //	GFP_KERNEL);
    //if (!tx_ring)
    //	goto err_tx_ring;
    tx_ring = &fx_tx_ring[0];

    //rx_ring = kcalloc(pdata->rx_ring_count, sizeof(struct fxgmac_ring),
    //	GFP_KERNEL);
    //if (!rx_ring)
    //	goto err_rx_ring;
    rx_ring = &fx_rx_ring[0];

    for (i = 0, channel = channel_head; i < pdata->channel_count;
    	i++, channel++) {
    	//snprintf(channel->name, sizeof(channel->name), "channel-%u", i);
    	//RtlStringCchPrintfA(channel->name, sizeof(channel->name), "channel-%u", i);

    	channel->pdata = pdata;
    	channel->queue_index = i;
    	channel->dma_regs = pdata->mac_regs + DMA_CH_BASE +
    		(DMA_CH_INC * i);

    	if (pdata->per_channel_irq) {
    		/* Get the per DMA interrupt */
    		ret = pdata->channel_irq[i];
    		if (ret < 0) {
    			netdev_err(pdata->netdev,
    				"get_irq %u failed\n",
    				i + 1);
    			goto err_irq;
    		}
    		channel->dma_irq = ret;
    	}

    	if (i < pdata->tx_ring_count)
    		channel->tx_ring = tx_ring++;

    	if (i < pdata->rx_ring_count)
    		channel->rx_ring = rx_ring++;

    	netif_dbg(pdata, drv, pdata->netdev,
    		"%s: dma_regs=%p, tx_ring=%p, rx_ring=%p\n",
    		channel->name, channel->dma_regs,
    		channel->tx_ring, channel->rx_ring);
    }

    pdata->channel_head = channel_head;

    return 0;

err_irq:
    //kfree(rx_ring);

    //err_rx_ring:
    //kfree(tx_ring);

    //err_tx_ring:
    //kfree(channel_head);

    return ret;
}
#endif

static void fxgmac_free_channels_and_rings(struct fxgmac_pdata* pdata)
{
    fxgmac_free_rings(pdata);

    fxgmac_free_channels(pdata);
}

static int fxgmac_alloc_channels_and_rings(struct fxgmac_pdata* pdata)
{
    int ret;

    ret = fxgmac_alloc_channels(pdata);
    if (ret)
    	goto err_alloc;

    ret = fxgmac_alloc_rings(pdata);
    if (ret)
    	goto err_alloc;

    return 0;

err_alloc:
    fxgmac_free_channels_and_rings(pdata);

    return ret;
}

#if 0
static int fxgmac_alloc_pages(struct fxgmac_pdata *pdata,
			      struct fxgmac_page_alloc *pa,
			      gfp_t gfp, int order)
{
    struct page *pages = NULL;
    dma_addr_t pages_dma;

    /* Try to obtain pages, decreasing order if necessary */
    gfp |= __GFP_COMP | __GFP_NOWARN;
    while (order >= 0) {
    	pages = alloc_pages(gfp, order);
    	if (pages)
    		break;

    	order--;
    }
    if (!pages)
    	return -ENOMEM;

    /* Map the pages */
    pages_dma = dma_map_page(pdata->dev, pages, 0,
    			 PAGE_SIZE << order, DMA_FROM_DEVICE);
    if (dma_mapping_error(pdata->dev, pages_dma)) {
    	put_page(pages);
    	return -ENOMEM;
    }

    pa->pages = pages;
    pa->pages_len = PAGE_SIZE << order;
    pa->pages_offset = 0;
    pa->pages_dma = pages_dma;

    return 0;
}
#endif
#if !(defined(UEFI) || defined(LINUX) || defined(UBOOT))
static void fxgmac_set_buffer_data(struct fxgmac_buffer_data* bd,
	struct fxgmac_page_alloc* pa,
	unsigned int len)
{
#ifndef LINUX
    bd = bd;
    pa = pa;
    len = len;
#else
    get_page(pa->pages);
    bd->pa = *pa;

    bd->dma_base = pa->pages_dma;
    bd->dma_off = pa->pages_offset;
    bd->dma_len = len;

    pa->pages_offset += len;
    if ((pa->pages_offset + len) > pa->pages_len) {
    	/* This data descriptor is responsible for unmapping page(s) */
    	bd->pa_unmap = *pa;

    	/* Get a new allocation next time */
    	pa->pages = NULL;
    	pa->pages_len = 0;
    	pa->pages_offset = 0;
    	pa->pages_dma = 0;
    }
#endif
}
#endif

static int fxgmac_map_rx_buffer(struct fxgmac_pdata* pdata,
	struct fxgmac_ring* ring,
	struct fxgmac_desc_data* desc_data)
{
#ifndef LINUX
    pdata = pdata;
    ring = ring;
    desc_data = desc_data;
#else
#if 0
    int order, ret;

    if (!ring->rx_hdr_pa.pages) {
    	ret = fxgmac_alloc_pages(pdata, &ring->rx_hdr_pa,
    		GFP_ATOMIC, 0);
    	if (ret)
    		return ret;
    }

    if (!ring->rx_buf_pa.pages) {
    	order = max_t(int, PAGE_ALLOC_COSTLY_ORDER - 1, 0);
    	ret = fxgmac_alloc_pages(pdata, &ring->rx_buf_pa,
    		GFP_ATOMIC, order);
    	if (ret)
    		return ret;
    }

    /* Set up the header page info */
    fxgmac_set_buffer_data(&desc_data->rx.hdr, &ring->rx_hdr_pa,
    	FXGMAC_SKB_ALLOC_SIZE);

    /* Set up the buffer page info */
    fxgmac_set_buffer_data(&desc_data->rx.buf, &ring->rx_buf_pa,
    	pdata->rx_buf_size);
#endif
    struct sk_buff *skb;
    skb = __netdev_alloc_skb_ip_align(pdata->netdev, pdata->rx_buf_size, GFP_ATOMIC);
    if (!skb) {
        netdev_err(pdata->netdev,
                       "%s: Rx init fails; skb is NULL\n", __func__);
        return -ENOMEM;
    }

    desc_data->skb = skb;
    desc_data->rx.buf.dma_base = dma_map_single(pdata->dev, skb->data, pdata->rx_buf_size, DMA_FROM_DEVICE);
	if (dma_mapping_error(pdata->dev, desc_data->rx.buf.dma_base)) {
        netdev_err(pdata->netdev, "%s: DMA mapping error\n", __func__);
        dev_kfree_skb_any(skb);
        return -EINVAL;
    }

#endif
    return 0;
}

#ifdef UBOOT
static void yt6801_desc_reset(struct fxgmac_dma_desc *desc_data)
{
    /* Reset the Tx descriptor
     *   Set buffer 1 (lo) address to zero
     *   Set buffer 1 (hi) address to zero
     *   Reset all other control bits (IC, TTSE, B2L & B1L)
     *   Reset all other control bits (OWN, CTXT, FD, LD, CPC, CIC, etc)
     */
    desc_data->desc0 = 0;
    desc_data->desc1 = 0;
    desc_data->desc2 = 0;
    desc_data->desc3 = 0;

    /* Make sure ownership is written to the descriptor */
    //dma_wmb();
}

static void fxgmac_hw_tx_desc_init(struct fxgmac_channel *channel)
{
    struct fxgmac_dma_desc *desc_data;
    struct fxgmac_pdata * pdata = channel->pdata;
    unsigned int i;

    /* Initialize all descriptors */
    for (i = 0; i < NIC_DEF_TBDS; i++) {
        desc_data = pdata->expansion.tx_desc_list + i;

        /* Initialize Tx descriptor */
        yt6801_desc_reset(desc_data);
    }

    ///* Update the total number of Tx descriptors */
    writereg(pdata->pAdapter, NIC_DEF_TBDS - 1, FXGMAC_DMA_REG(channel, DMA_CH_TDRLR));

#if 0
    DbgPrintF(MP_TRACE, "tx_desc_list:%p\n", pdata->tx_desc_list);
    DbgPrintF(MP_TRACE, "bus_to_phys:%llx\n", bus_to_phys(pdata->pdev,
	(pci_addr_t)(unsigned long)pdata->tx_desc_list));//adpt->TbdPhyAddr
    DbgPrintF(MP_TRACE, "lower_32_bits:%x\n", lower_32_bits(bus_to_phys(pdata->pdev,
	(pci_addr_t)(unsigned long)pdata->tx_desc_list)));//adpt->TbdPhyAddr
    DbgPrintF(MP_TRACE, "dma tdlr lo:%p\n", FXGMAC_DMA_REG(channel, DMA_CH_TDLR_LO));
#endif
    /* Update the starting address of descriptor ring */
    writereg(pdata->pAdapter, upper_32_bits(cpu_to_le64(bus_to_phys(pdata->pdev,
	(pci_addr_t)(unsigned long)pdata->expansion.tx_desc_list))),//adpt->TbdPhyAddr
        FXGMAC_DMA_REG(channel, DMA_CH_TDLR_HI));
    writereg(pdata->pAdapter, lower_32_bits(cpu_to_le64(bus_to_phys(pdata->pdev,
	(pci_addr_t)(unsigned long)pdata->expansion.tx_desc_list))),//adpt->TbdPhyAddr
        FXGMAC_DMA_REG(channel, DMA_CH_TDLR_LO));
#if 0
    DbgPrintF(MP_TRACE, "Read tx starting high address:%x\n",
		    readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_TDLR_HI)));
    DbgPrintF(MP_TRACE, "Read tx starting low address:%x\n",
		    readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_TDLR_LO)));
#endif
}

static void fxgmac_tx_desc_init(struct fxgmac_pdata* pdata)
{
    fxgmac_hw_tx_desc_init(pdata->expansion.tx_channel);
}

static void fxgmac_hw_rx_desc_init(struct fxgmac_channel *channel)
{
    struct fxgmac_pdata *pdata = channel->pdata;
    struct fxgmac_dma_desc *desc_data;
    unsigned int i;
    uint64_t   HwRbdPa;

    /* Initialize all descriptors */
    for (i = 0; i < NIC_DEF_RECV_BUFFERS; i++) {
        desc_data = pdata->expansion.rx_desc_list + i;

        /* Initialize Rx descriptor */
        yt6801_desc_reset(desc_data);
        desc_data->desc0 = lower_32_bits(bus_to_phys(pdata->pdev, (pci_addr_t)(unsigned long)(pdata->expansion.rx_buffer)));
        desc_data->desc1 = upper_32_bits(bus_to_phys(pdata->pdev, (pci_addr_t)(unsigned long)(pdata->expansion.rx_buffer)));
	    desc_data->desc3 = FXGMAC_SET_REG_BITS_LE(
                desc_data->desc3,
                RX_NORMAL_DESC3_BUF2V_POS,
                RX_NORMAL_DESC3_BUF2V_LEN,
                1);
    	desc_data->desc3 = FXGMAC_SET_REG_BITS_LE(
                desc_data->desc3,
                RX_NORMAL_DESC3_BUF1V_POS,
                RX_NORMAL_DESC3_BUF1V_LEN,
                1);
        desc_data->desc3 = FXGMAC_SET_REG_BITS_LE(
		desc_data->desc3,
                RX_NORMAL_DESC3_OWN_POS,
                RX_NORMAL_DESC3_OWN_LEN,
		1);
    }

    /* Update the total number of Rx descriptors */
    writereg(pdata->pAdapter, NIC_DEF_RECV_BUFFERS - 1, FXGMAC_DMA_REG(channel, DMA_CH_RDRLR));
#if 0
    DbgPrintF(MP_TRACE, "rx_desc_list:%p\n", pdata->rx_desc_list);
    DbgPrintF(MP_TRACE, "bus_to_phys:%llx\n", bus_to_phys(pdata->pdev,
	(pci_addr_t)(unsigned long)pdata->rx_desc_list));//adpt->TbdPhyAddr
    DbgPrintF(MP_TRACE, "lower_32_bits:%x\n", lower_32_bits(bus_to_phys(pdata->pdev,
	(pci_addr_t)(unsigned long)pdata->rx_desc_list)));//adpt->TbdPhyAddr
    DbgPrintF(MP_TRACE, "dma rdlr lo:%p\n", FXGMAC_DMA_REG(channel, DMA_CH_RDLR_LO));
#endif
    /* Update the starting address of descriptor ring */
    writereg(pdata->pAdapter, upper_32_bits(cpu_to_le64(bus_to_phys(pdata->pdev,
	(pci_addr_t)(unsigned long)pdata->expansion.rx_desc_list))),
        FXGMAC_DMA_REG(channel, DMA_CH_RDLR_HI));
    writereg(pdata->pAdapter, lower_32_bits(cpu_to_le64(bus_to_phys(pdata->pdev,
	(pci_addr_t)(unsigned long)pdata->expansion.rx_desc_list))),
        FXGMAC_DMA_REG(channel, DMA_CH_RDLR_LO));
#if 0
    DbgPrintF(MP_TRACE, "Read rx starting high address:%x\n",
		    readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_RDLR_HI)));
    DbgPrintF(MP_TRACE, "Read rx starting low address:%x\n",
		    readreg(pdata->pAdapter, FXGMAC_DMA_REG(channel, DMA_CH_RDLR_LO)));
#endif

    HwRbdPa = (uint64_t)pdata->expansion.rx_desc_list + (NIC_DEF_RECV_BUFFERS) * sizeof(struct fxgmac_dma_desc);
    /* Update the Rx Descriptor Tail Pointer */
    writereg(pdata->pAdapter, lower_32_bits((unsigned long)HwRbdPa), FXGMAC_DMA_REG(channel, DMA_CH_RDTR_LO));

}


static void fxgmac_rx_desc_init(struct fxgmac_pdata* pdata)
{
    fxgmac_hw_rx_desc_init(pdata->expansion.rx_channel);
}

#else
static void fxgmac_tx_desc_init(struct fxgmac_pdata* pdata)
{
#ifndef LINUX
    struct fxgmac_hw_ops* hw_ops = &pdata->hw_ops;
    struct fxgmac_channel* channel;
    channel = pdata->channel_head;
    hw_ops->tx_desc_init(channel);
#else
    struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
    struct fxgmac_desc_data *desc_data;
    struct fxgmac_dma_desc *dma_desc;
    struct fxgmac_channel *channel;
    struct fxgmac_ring *ring;
    dma_addr_t dma_desc_addr;
    unsigned int i, j;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
    	ring = channel->tx_ring;
    	if (!ring)
    		break;

    	/* reset the tx timer status. 20220104 */
    	channel->tx_timer_active = 0;

    	dma_desc = ring->dma_desc_head;
    	dma_desc_addr = ring->dma_desc_head_addr;

    	for (j = 0; j < ring->dma_desc_count; j++) {
    		desc_data = FXGMAC_GET_DESC_DATA(ring, j);

    		desc_data->dma_desc = dma_desc;
    		desc_data->dma_desc_addr = dma_desc_addr;

    		dma_desc++;
    		dma_desc_addr += sizeof(struct fxgmac_dma_desc);
    	}

    	ring->cur = 0;
    	ring->dirty = 0;
    	memset(&ring->tx, 0, sizeof(ring->tx));

    	hw_ops->tx_desc_init(channel);
    }
#endif
}

static void fxgmac_rx_desc_init(struct fxgmac_pdata* pdata)
{
#ifndef LINUX
    struct fxgmac_hw_ops* hw_ops = &pdata->hw_ops;
    struct fxgmac_channel* channel;
    int Qid = 0;

    channel = pdata->channel_head;
    for (Qid = 0; Qid < RSS_Q_COUNT; Qid++)
    {
        hw_ops->rx_desc_init(channel + Qid);
    }
#else
    struct fxgmac_hw_ops *hw_ops = &pdata->hw_ops;
    struct fxgmac_desc_data *desc_data;
    struct fxgmac_dma_desc *dma_desc;
    struct fxgmac_channel *channel;
    struct fxgmac_ring *ring;
    dma_addr_t dma_desc_addr;
    unsigned int i, j;

    channel = pdata->channel_head;
    for (i = 0; i < pdata->channel_count; i++, channel++) {
    	ring = channel->rx_ring;
    	if (!ring)
    		break;

    	dma_desc = ring->dma_desc_head;
    	dma_desc_addr = ring->dma_desc_head_addr;

    	for (j = 0; j < ring->dma_desc_count; j++) {
    		desc_data = FXGMAC_GET_DESC_DATA(ring, j);

    		desc_data->dma_desc = dma_desc;
    		desc_data->dma_desc_addr = dma_desc_addr;

    		if (fxgmac_map_rx_buffer(pdata, ring, desc_data))
    			break;

    		dma_desc++;
    		dma_desc_addr += sizeof(struct fxgmac_dma_desc);
    	}

    	ring->cur = 0;
    	ring->dirty = 0;

    	hw_ops->rx_desc_init(channel);
    }
#endif
}
#endif
#ifdef LINUX
static int fxgmac_map_tx_skb(struct fxgmac_channel *channel,
			     struct sk_buff *skb)
{
    struct fxgmac_pdata *pdata = channel->pdata;
    struct fxgmac_ring *ring = channel->tx_ring;
    unsigned int start_index, cur_index;
    struct fxgmac_desc_data *desc_data;
    unsigned int offset, datalen, len;
    struct fxgmac_pkt_info *pkt_info;
#if ( LINUX_VERSION_CODE >= KERNEL_VERSION(5,0,0) )
    skb_frag_t *frag;
#else
    struct skb_frag_struct *frag;
#endif
    unsigned int tso, vlan;
    dma_addr_t skb_dma;
    unsigned int i;
#if 0
    void* addr;
    struct skb_shared_info *info = skb_shinfo(skb);
#endif

    offset = 0;
    start_index = ring->cur;
    cur_index = ring->cur;

    pkt_info = &ring->pkt_info;
    pkt_info->desc_count = 0;
    pkt_info->length = 0;

    tso = FXGMAC_GET_REG_BITS(pkt_info->attributes,
    			  TX_PACKET_ATTRIBUTES_TSO_ENABLE_POS,
    			  TX_PACKET_ATTRIBUTES_TSO_ENABLE_LEN);
    vlan = FXGMAC_GET_REG_BITS(pkt_info->attributes,
    			   TX_PACKET_ATTRIBUTES_VLAN_CTAG_POS,
    			   TX_PACKET_ATTRIBUTES_VLAN_CTAG_LEN);

    /* Save space for a context descriptor if needed */
    if ((tso && (pkt_info->mss != ring->tx.cur_mss)) ||
        (vlan && (pkt_info->vlan_ctag != ring->tx.cur_vlan_ctag)))
    {
        cur_index = FXGMAC_GET_ENTRY(cur_index, ring->dma_desc_count);
    }
    desc_data = FXGMAC_GET_DESC_DATA(ring, cur_index);

    if (tso) {
    	/* Map the TSO header */
    	skb_dma = dma_map_single(pdata->dev, skb->data,
    				 pkt_info->header_len, DMA_TO_DEVICE);
    	if (dma_mapping_error(pdata->dev, skb_dma)) {
    		netdev_alert(pdata->netdev, "dma_map_single failed\n");
    		goto err_out;
    	}
    	desc_data->skb_dma = skb_dma;
    	desc_data->skb_dma_len = pkt_info->header_len;
    	netif_dbg(pdata, tx_queued, pdata->netdev,
    		  "skb header: index=%u, dma=%pad, len=%u\n",
    		  cur_index, &skb_dma, pkt_info->header_len);

    	offset = pkt_info->header_len;

    	pkt_info->length += pkt_info->header_len;

    	cur_index = FXGMAC_GET_ENTRY(cur_index, ring->dma_desc_count);
    	desc_data = FXGMAC_GET_DESC_DATA(ring, cur_index);
    }

    /* Map the (remainder of the) packet */
    for (datalen = skb_headlen(skb) - offset; datalen; ) {
    	len = min_t(unsigned int, datalen, FXGMAC_TX_MAX_BUF_SIZE);

    	skb_dma = dma_map_single(pdata->dev, skb->data + offset, len,
    				 DMA_TO_DEVICE);
    	if (dma_mapping_error(pdata->dev, skb_dma)) {
    		netdev_alert(pdata->netdev, "dma_map_single failed\n");
    		goto err_out;
    	}
    	desc_data->skb_dma = skb_dma;
    	desc_data->skb_dma_len = len;
    	netif_dbg(pdata, tx_queued, pdata->netdev,
    		  "skb data: index=%u, dma=%pad, len=%u\n",
    		  cur_index, &skb_dma, len);

    	datalen -= len;
    	offset += len;

    	pkt_info->length += len;

    	cur_index = FXGMAC_GET_ENTRY(cur_index, ring->dma_desc_count);
    	desc_data = FXGMAC_GET_DESC_DATA(ring, cur_index);
    }

    for (i = 0; i < skb_shinfo(skb)->nr_frags; i++) {
    	netif_dbg(pdata, tx_queued, pdata->netdev,
    		  "mapping frag %u\n", i);
#if 0
        frag = info->frags + i;
        len = skb_frag_size(frag);
        addr = skb_frag_address(frag);
#else
        frag = &skb_shinfo(skb)->frags[i];
#endif
        offset = 0;

    	for (datalen = skb_frag_size(frag); datalen; ) {
    		len = min_t(unsigned int, datalen,
    			    FXGMAC_TX_MAX_BUF_SIZE);

#if 1
            skb_dma = skb_frag_dma_map(pdata->dev, frag, offset,
                            len, DMA_TO_DEVICE);
#else
            skb_dma = dma_map_single(pdata->dev, addr + offset, len, DMA_TO_DEVICE);
#endif
    		if (dma_mapping_error(pdata->dev, skb_dma)) {
    			netdev_alert(pdata->netdev,
    				     "skb_frag_dma_map failed\n");
    			goto err_out;
    		}
    		desc_data->skb_dma = skb_dma;
    		desc_data->skb_dma_len = len;
#if 1
    		desc_data->mapped_as_page = 1;
#endif
    		netif_dbg(pdata, tx_queued, pdata->netdev,
    			  "skb frag: index=%u, dma=%pad, len=%u\n",
    			  cur_index, &skb_dma, len);

    		datalen -= len;
    		offset += len;

    		pkt_info->length += len;

    		cur_index = FXGMAC_GET_ENTRY(cur_index, ring->dma_desc_count);
    		desc_data = FXGMAC_GET_DESC_DATA(ring, cur_index);
    	}
    }

    /* Save the skb address in the last entry. We always have some data
     * that has been mapped so desc_data is always advanced past the last
     * piece of mapped data - use the entry pointed to by cur_index - 1.
     */
    desc_data = FXGMAC_GET_DESC_DATA(ring, (cur_index - 1) & (ring->dma_desc_count - 1));
    desc_data->skb = skb;

    /* Save the number of descriptor entries used */
    if (start_index <= cur_index)
    	pkt_info->desc_count = cur_index - start_index;
    else
    	pkt_info->desc_count = ring->dma_desc_count - start_index + cur_index;

    return pkt_info->desc_count;

err_out:
    while (start_index < cur_index) {
    	desc_data = FXGMAC_GET_DESC_DATA(ring, start_index);
    	start_index = FXGMAC_GET_ENTRY(start_index, ring->dma_desc_count);
    	fxgmac_unmap_desc_data(pdata, desc_data);
    }

    return 0;
}
#endif

void fxgmac_init_desc_ops(struct fxgmac_desc_ops* desc_ops)
{
#ifdef UBOOT
    desc_ops->alloc_channles_and_rings = fxgmac_alloc_channels;
#else
    desc_ops->alloc_channles_and_rings = fxgmac_alloc_channels_and_rings;
#endif
    desc_ops->free_channels_and_rings = fxgmac_free_channels_and_rings;
#ifndef LINUX
    desc_ops->map_tx_skb = NULL;
#else
    desc_ops->map_tx_skb = fxgmac_map_tx_skb;
#endif
    desc_ops->map_rx_buffer = fxgmac_map_rx_buffer;
    desc_ops->unmap_desc_data = fxgmac_unmap_desc_data;
    desc_ops->tx_desc_init = fxgmac_tx_desc_init;
    desc_ops->rx_desc_init = fxgmac_rx_desc_init;
}

#include "pcie-zte-zf-hdma.h"

#if ZF_MPF_HDMA_MODE_ENABLE
static struct task_struct *callback_thread[PCIE_DPU_EP_NUMS];
#endif

static int __attribute__((unused)) cfg_phy_write(u32 value, u64 PhyAddr)
{
    void __iomem *virt_addr = NULL;
    u64 tmp_addr = 0;
    u64 offset = 0;
    u64 size = 0;

    offset = PhyAddr % 0x1000;
    if (PhyAddr < offset)
    {
        DH_LOG_ERR(MODULE_MPF, "data overflow! PhyAddr=0x%llx, offset=0x%llx\n", PhyAddr, offset);
        return PCIBIOS_BAD_REGISTER_NUMBER;
    }
    else
    {
        tmp_addr = PhyAddr - offset;
    }

    if (offset <= (0x1000 - 4))
    {
        size = 0x1000;
    }
    else
    {
        size = 2 * 0x1000;
    }

    virt_addr = ioremap(tmp_addr, size);
    if (NULL == virt_addr)
    {
        DH_LOG_ERR(MODULE_MPF, "cfg_write ioremap failed!\n");
        return PCIBIOS_BAD_REGISTER_NUMBER;
    }

    writel(value, (virt_addr + offset));

    iounmap(virt_addr);
    return 0;
}

static inline struct zf_hdma_chan *to_zf_hdma_chan(struct dma_chan *chan)
{
    return container_of(chan, struct zf_hdma_chan, zxdh_vc.chan);
}

static u64 zte_pcie_dma_atu_addr_remapping(u64 addr_input)
{
    u64 addr_output = 0;
    // DMA and ATU地址重映射，将地址的bit12~18左移4位，设置bit15 = 0
    addr_output = (((addr_input & (0x7F << 12)) << 4) | (addr_input & 0xFFF)) & (~(1UL << 15));

    return addr_output;
}

static inline void read_ch(struct zf_hdma_chan *zf_chan, u32 is_read, int offset, u32 *val)
{
    u64 register_offet = 0;

    void __iomem *addr = zf_chan->base_addr;

    register_offet = zte_pcie_dma_atu_addr_remapping(((u64)zf_chan->id * (u64)ZF_HDMA_PER_CHANNEL_SIZE)) + ((u64)is_read * (u64)ZF_HDMA_RDCH_OFFSET);
    *val = readl(addr + register_offet + offset);
}

static inline void write_ch(struct zf_hdma_chan *zf_chan, u32 is_read, int offset, u32 val)
{
    u64 register_off = 0;
    void __iomem *addr = zf_chan->base_addr;

    register_off = zte_pcie_dma_atu_addr_remapping(((u64)zf_chan->id * (u64)ZF_HDMA_PER_CHANNEL_SIZE)) + ((u64)is_read * (u64)ZF_HDMA_RDCH_OFFSET);
    writel(val, addr + register_off + offset);
}

static inline void rmw_ch(struct zf_hdma_chan *zf_chan, u32 is_read, int offset, u32 val, u32 mask)
{
    u32 reg_val = 0;

    read_ch(zf_chan, is_read, offset, &reg_val);
    reg_val &= (~mask);
    reg_val |= (val & mask);
    write_ch(zf_chan, is_read, offset, reg_val);
}

static int zf_hdma_alloc_chan_resources(struct dma_chan *chan)
{
    u32 is_read = 0;
    // struct zf_hdma_chan *zf_chan = to_zf_hdma_chan(chan);

    if (chan->device->directions == BIT(DMA_MEM_TO_DEV))
    {
        is_read = HDMA_RD;
    }
    else if (chan->device->directions == BIT(DMA_DEV_TO_MEM))
    {
        is_read = HDMA_WR;
    }
    else
    {
        DH_LOG_ERR(MODULE_MPF, "err direct\n");
        return -EINVAL;
    }

    /* 开启dma通道中断 */
    // rmw_ch(zf_chan, is_read, HDMA_INT_SETUP_OFF, 0x0 << HDMA_INT_MASK_BIT, HDMA_INT_MASK << HDMA_INT_MASK_BIT);
    // rmw_ch(zf_chan, is_read, HDMA_INT_SETUP_OFF, 0x1 << HDMA_LSIE_BIT, HDMA_LSIE_MASK << HDMA_LSIE_BIT);

    return 0;
}

static void zf_hdma_free_chan_resources(struct dma_chan *chan)
{
    u32 is_read = 0;
    // struct zf_hdma_chan *zf_chan = to_zf_hdma_chan(chan);

    if (chan->device->directions == BIT(DMA_MEM_TO_DEV))
    {
        is_read = HDMA_RD;
    }
    else if (chan->device->directions == BIT(DMA_DEV_TO_MEM))
    {
        is_read = HDMA_WR;
    }
    else
    {
        DH_LOG_ERR(MODULE_MPF, "err direct\n");
        return;
    }

    /* 释放dma通道中断 */
    // rmw_ch(zf_chan, is_read, HDMA_INT_SETUP_OFF, 0x7 << HDMA_INT_MASK_BIT, HDMA_INT_MASK << HDMA_INT_MASK_BIT);
    // rmw_ch(zf_chan, is_read, HDMA_INT_SETUP_OFF, 0x0 << HDMA_LSIE_BIT, HDMA_LSIE_MASK << HDMA_LSIE_BIT);
}

static int zf_hdma_device_config(struct dma_chan *chan, struct dma_slave_config *config)
{
    // struct zf_hdma_chan *zf_chan = to_zf_hdma_chan(chan);
    // u32 is_read = HDMA_WR;

    return 0;
}

static struct dma_async_tx_descriptor *zf_hdma_prep_dma_memcpy(struct dma_chan *chan, dma_addr_t dest, dma_addr_t src, size_t len, unsigned long flags)
{
    u32 is_read = HDMA_WR;
    struct zf_hdma_sqe *sqe = NULL, *tem_sqe = NULL;
    struct zf_hdma_tx *tx = NULL, *tem_tx = NULL;
    struct dma_async_tx_descriptor *tx_desc = NULL;
    struct zf_hdma_chan *zf_chan = to_zf_hdma_chan(chan);
    int tx_num = 0;

    DH_LOG_DEBUG(MODULE_MPF, "enter\n");
    if (chan->device->directions == BIT(DMA_MEM_TO_DEV))
    {
        is_read = HDMA_RD;
        dest |= ZF_PREFIX_ADDR;
    }
    else if (chan->device->directions == BIT(DMA_DEV_TO_MEM))
    {
        is_read = HDMA_WR;
        src |= ZF_PREFIX_ADDR;
    }
    else
    {
        DH_LOG_ERR(MODULE_MPF, "err direct\n");
        return NULL;
    }

    tem_sqe = zf_chan->sqe_list;
    sqe = devm_kzalloc(&zf_chan->ep_pdev->dev, sizeof(struct zf_hdma_sqe), GFP_KERNEL);
    if (!sqe)
    {
        DH_LOG_ERR(MODULE_MPF, "err alloc sqe\n");
        return NULL;
    }
    sqe->length = len;
    sqe->src_addr = src;
    sqe->dst_addr = dest;

    while (NULL != tem_sqe->next)
    {
        tem_sqe = tem_sqe->next;
    }
    tem_sqe->next = sqe;

    tx_desc = zxdh_vchan_tx_prep(&zf_chan->zxdh_vc, &zf_chan->zxdh_vd, flags);
    tem_tx = zf_chan->tx_list;
    tx = devm_kzalloc(&zf_chan->ep_pdev->dev, sizeof(struct zf_hdma_tx), GFP_KERNEL);
    if (!tx)
    {
        DH_LOG_ERR(MODULE_MPF, "err alloc tx\n");
        devm_kfree(&zf_chan->ep_pdev->dev, sqe);
        return NULL;
    }
    tx->tx_desc = tx_desc;

    while (NULL != tem_tx->next)
    {
        tem_tx = tem_tx->next;
        tx_num++;
    }
    tx->tx_id = tx_num;
    tem_tx->next = tx;

    return tx_desc;
}

static int zf_hdma_terminate_all(struct dma_chan *chan)
{
    u32 is_read = HDMA_WR;
    struct zf_hdma_chan *zf_chan = to_zf_hdma_chan(chan);

    if (chan->device->directions == BIT(DMA_MEM_TO_DEV))
    {
        is_read = HDMA_RD;
    }

    write_ch(zf_chan, is_read, HDMA_DOORBELL_OFF, HDMA_DOORBELL_STOP);

    return 0;
}

static enum dma_status zf_hdma_tx_status(struct dma_chan *chan, dma_cookie_t cookie, struct dma_tx_state *txstate)
{
    return 0;
}

static struct dma_async_tx_descriptor *zf_hdma_device_prep_slave_sg(struct dma_chan *chan, struct scatterlist *sgl, unsigned int len, enum dma_transfer_direction direction, unsigned long flags, void *context)
{
    struct zf_hdma_chan *zf_chan = to_zf_hdma_chan(chan);
    u32 is_read = HDMA_WR;

    if (chan->device->directions == BIT(DMA_MEM_TO_DEV))
    {
        is_read = HDMA_RD;
    }
    else if (chan->device->directions == BIT(DMA_DEV_TO_MEM))
    {
        is_read = HDMA_WR;
    }
    else
    {
        DH_LOG_ERR(MODULE_MPF, "err direct\n");
        return NULL;
    }

    return zxdh_vchan_tx_prep(&zf_chan->zxdh_vc, &zf_chan->zxdh_vd, flags);
}

static struct dma_async_tx_descriptor *zf_hdma_device_prep_dma_cyclic(struct dma_chan *chan, dma_addr_t paddr, size_t len, size_t count, enum dma_transfer_direction direction, unsigned long flags)
{
    struct zf_hdma_chan *zf_chan = to_zf_hdma_chan(chan);

    return zxdh_vchan_tx_prep(&zf_chan->zxdh_vc, &zf_chan->zxdh_vd, flags);
}

static void free_used_sqe(struct zf_hdma_chan *zf_chan)
{
    struct zf_hdma_sqe *temp = NULL;

    temp = zf_chan->sqe_list->next;
    zf_chan->sqe_list->next = zf_chan->sqe_list->next->next;

    devm_kfree(&zf_chan->ep_pdev->dev, temp);
}

static void zf_hdma_register_set(struct zf_hdma_chan *zf_chan, u32 is_read, struct zf_hdma_sqe *sqe)
{
    /* DMA Engine enable */
    write_ch(zf_chan, is_read, HDMA_EN_OFF, HDMA_EN);

    /* DMA transfer Size */
    write_ch(zf_chan, is_read, HDMA_XFERSIZE_OFF, (u32)sqe->length);

    /* DMA SAR & DAR */
    write_ch(zf_chan, is_read, HDMA_SAR_LOW_OFF, (sqe->src_addr & 0xffffffff));
    write_ch(zf_chan, is_read, HDMA_SAR_HIGH_OFF, ((sqe->src_addr >> 32) & 0xffffffff));
    write_ch(zf_chan, is_read, HDMA_DAR_LOW_OFF, (sqe->dst_addr & 0xffffffff));
    write_ch(zf_chan, is_read, HDMA_DAR_HIGH_OFF, ((sqe->dst_addr >> 32) & 0xffffffff));

    /* func_no */
    write_ch(zf_chan, is_read, HDMA_FUNC_NUM_OFF, ((zf_chan->func_no & PCIE_DPU_EP_GET_PF_NO) | (zf_chan->vfunc_no << HDMA_FUNC_NUM_OFF_VF) | (!isPF(zf_chan->func_no) << HDMA_FUNC_NUM_OFF_VF_ENABLE)));

    /* DMA Doorbell */
    write_ch(zf_chan, is_read, HDMA_DOORBELL_OFF, HDMA_DOORBELL_START);
    return;
}

static void zf_hdma_issue_pending(struct dma_chan *chan)
{
    // u32 is_read = HDMA_WR;
    unsigned long flags = 0;
    struct zf_hdma_chan *zf_chan = NULL;
    struct zf_hdma_tx *tx_temp = NULL;

    zf_chan = to_zf_hdma_chan(chan);

    tx_temp = zf_chan->tx_list->next;
    while (tx_temp->next != NULL)
    {
        tx_temp = tx_temp->next;
    }

    tx_temp->callback = tx_temp->tx_desc->callback;
    tx_temp->callback_param = tx_temp->tx_desc->callback_param;

    spin_lock_irqsave(&zf_chan->zxdh_vc.lock, flags);
    zxdh_vchan_issue_pending(&zf_chan->zxdh_vc);
    spin_unlock_irqrestore(&zf_chan->zxdh_vc.lock, flags);

    return;
}

static void free_used_tx(struct zf_hdma_chan *zf_chan)
{
    struct zf_hdma_tx *temp = NULL;

    temp = zf_chan->tx_list->next;
    zf_chan->tx_list->next = zf_chan->tx_list->next->next;

    devm_kfree(&zf_chan->ep_pdev->dev, temp);
}

int zf_hdma_wr_handler(void *data)
{
    u32 chan_status, chan_int_status;
    struct pcie_dpu_ep *dpu_dev = data;
    struct dma_chan *chan = NULL;
    struct zf_hdma_chan *zf_chan = NULL;
    struct zf_hdma_tx *zf_tx_desc = NULL;

    list_for_each_entry(chan, &dpu_dev->wr_dd->channels, device_node)
    {
        zf_chan = to_zf_hdma_chan(chan);
        if (!zf_chan->tx_list->next || !zf_chan->tx_list->next->tx_desc || !zf_chan->tx_list->next->callback)
        {
            DH_LOG_INFO(MODULE_MPF, "wr_zf_chan%d is not used!\n", zf_chan->id);
            continue;
        }
        else
        {
            zf_tx_desc = zf_chan->tx_list->next;
        }
        read_ch(zf_chan, HDMA_WR, HDMA_STATUS_OFF, &chan_status);
        read_ch(zf_chan, HDMA_WR, HDMA_INT_STATUS_OFF, &chan_int_status);

        // if (((chan_status & HDMA_STATUS_OFF_STATUS) & HDMA_STATUS_STOPPED) && (chan_int_status & HDMA_STOP_INT_STATUS))
        // {
        spin_lock(&zf_chan->zxdh_vc.lock);
        zf_tx_desc->callback(zf_tx_desc->callback_param);
        free_used_tx(zf_chan);
        zf_chan->is_busy = HDMA_CHAN_IDLE;
        spin_unlock(&zf_chan->zxdh_vc.lock);
        break;
        // }
    }

    return 0;
}

int zf_hdma_rd_handler(void *data)
{
    u32 chan_status = 0, chan_int_status = 0;
    struct pcie_dpu_ep *dpu_dev = data;
    struct dma_chan *chan = NULL;
    struct zf_hdma_chan *zf_chan = NULL;
    struct zf_hdma_tx *zf_tx_desc = NULL;

    list_for_each_entry(chan, &dpu_dev->rd_dd->channels, device_node)
    {
        zf_chan = to_zf_hdma_chan(chan);
        if (!zf_chan->tx_list->next || !zf_chan->tx_list->next->tx_desc || !zf_chan->tx_list->next->callback)
        {
            DH_LOG_INFO(MODULE_MPF, "rd_zf_chan%d is not used!\n", zf_chan->id);
            continue;
        }
        else
        {
            zf_tx_desc = zf_chan->tx_list->next;
        }
        read_ch(zf_chan, HDMA_RD, HDMA_STATUS_OFF, &chan_status);
        read_ch(zf_chan, HDMA_RD, HDMA_INT_STATUS_OFF, &chan_int_status);

        // if (((chan_status & HDMA_STATUS_OFF_STATUS) & HDMA_STATUS_STOPPED) && (chan_int_status & HDMA_STOP_INT_STATUS))
        // {
        spin_lock(&zf_chan->zxdh_vc.lock);
        zf_tx_desc->callback(zf_tx_desc->callback_param);
        free_used_tx(zf_chan);
        zf_chan->is_busy = HDMA_CHAN_IDLE;
        spin_unlock(&zf_chan->zxdh_vc.lock);
        break;
        // }
    }

    return 0;
}

static void zf_hdma_desc_free(struct zxdh_virt_dma_desc *zxdh_vd)
{
    dma_descriptor_unmap(&zxdh_vd->tx);
}

static bool zf_dma_filter_fn(struct dma_chan *chan, void *node)
{
    unsigned long dev_node = (unsigned long)dev_to_node(&chan->dev->device);
    return (dev_node == (unsigned long)node);
}

struct dma_chan *zte_get_chan_for_dma(struct pci_epc *epc, u32 is_read)
{
    int node = 0;
    dma_cap_mask_t dma_mask;
    struct dma_chan *chan = NULL;
    struct pcie_dpu_ep *ep = NULL;

    if (IS_ERR_OR_NULL(epc))
    {
        DH_LOG_ERR(MODULE_MPF, "not found epc\n");
        return NULL;
    }

    ep = epc_get_drvdata(epc);
    if (!ep)
    {
        DH_LOG_ERR(MODULE_MPF, " not found ep\n");
        return NULL;
    }

    if (is_read)
    {
        node = dev_to_node(ep->rd_dd->dev);
    }
    else
    {
        node = dev_to_node(ep->wr_dd->dev);
    }

    dma_cap_zero(dma_mask);
    dma_cap_set(DMA_MEMCPY, dma_mask);
    chan = dma_request_channel(dma_mask, zf_dma_filter_fn, (void *)(unsigned long)node);

    return chan;
}
EXPORT_SYMBOL_GPL(zte_get_chan_for_dma);

void zte_zf_pcie_set_pfvf_no(struct dma_chan *chan, u8 func_no, u8 vfunc_no)
{
    struct zf_hdma_chan *zf_chan = to_zf_hdma_chan(chan);

    DH_LOG_INFO(MODULE_MPF, "%s func_no = 0x%x, vfunc_no = 0x%x\n", __func__, func_no, vfunc_no);

    zf_chan->func_no = func_no;
    zf_chan->vfunc_no = vfunc_no;
}
EXPORT_SYMBOL_GPL(zte_zf_pcie_set_pfvf_no);

int zf_pcie_get_hdma_chan(struct pci_epc *epc, u8 func_no, u8 vfunc_no,
                          struct dma_chan **rchan, struct dma_chan **wchan)
{
    struct dma_chan *rch, *wch;

    wch = zte_get_chan_for_dma(epc, HDMA_WR);
    if (IS_ERR_OR_NULL(wch))
    {
        DH_LOG_ERR(MODULE_MPF, "failed to get write chan\n");
        return -EFAULT;
    }

    rch = zte_get_chan_for_dma(epc, HDMA_RD);
    if (IS_ERR_OR_NULL(rch))
    {
        DH_LOG_ERR(MODULE_MPF, "failed to get read chan\n");
        dma_release_channel(rch);
        return -EFAULT;
    }

    zte_zf_pcie_set_pfvf_no(rch, func_no, vfunc_no);
    zte_zf_pcie_set_pfvf_no(wch, func_no, vfunc_no);

    *rchan = rch;
    *wchan = wch;
    return 0;
}

__attribute__((unused)) static int zf_hdma_virtual_channels_init(struct dma_device *dma_dev, struct pci_dev *pdev, void __iomem *addr)
{
    struct zf_hdma_chan *zf_chan = NULL;
    u32 i = 0;

    INIT_LIST_HEAD(&dma_dev->channels);

    /*
     * Register as many memcpy as we have physical channels,
     * we won't always be able to use all but the code will have
     * to cope with that situation.
     */
    for (i = 0; i < (u32)ZF_HDMA_CHAN_NUM; i++)
    {
        zf_chan = devm_kzalloc(&pdev->dev, sizeof(*zf_chan), GFP_KERNEL);
        if (!zf_chan)
        {
            // TODO free
            return -ENOMEM;
        }
        zf_chan->sqe_list = devm_kzalloc(&pdev->dev, sizeof(struct zf_hdma_sqe), GFP_KERNEL);
        if (!zf_chan->sqe_list)
        {
            // TODO free
            return -ENOMEM;
        }
        zf_chan->tx_list = devm_kzalloc(&pdev->dev, sizeof(struct zf_hdma_tx), GFP_KERNEL);
        if (!zf_chan->tx_list)
        {
            // TODO free
            return -ENOMEM;
        }
        zf_chan->tx_list->tx_id = 0;

        zf_chan->id = i + ZF_HDMA_CHAN_FIRST_IDX;
        zf_chan->ep_pdev = pdev;
        zf_chan->base_addr = addr + ZF_HDMA_ADDR_OFFSET;
        zf_chan->is_busy = HDMA_CHAN_IDLE;

        zf_chan->name = kasprintf(GFP_KERNEL, "chan%d", i);
        if (!zf_chan->name)
            return -ENOMEM;

        zf_chan->zxdh_vc.desc_free = zf_hdma_desc_free;

        zxdh_vchan_init(&zf_chan->zxdh_vc, dma_dev);
    }

    return i;
}

__attribute__((unused)) void zf_hdma_device_init(struct device *dev, struct dma_device *dd, u32 is_read)
{
    dd->device_alloc_chan_resources = zf_hdma_alloc_chan_resources; // 3,4,8,9 unneccessary
    dd->device_free_chan_resources = zf_hdma_free_chan_resources;
    dd->device_config = zf_hdma_device_config;
    dd->device_tx_status = zf_hdma_tx_status;
    dd->device_issue_pending = zf_hdma_issue_pending; // push pending transactions to hardware
    dd->device_prep_dma_memcpy = zf_hdma_prep_dma_memcpy;
    dd->device_terminate_all = zf_hdma_terminate_all;
    dd->device_prep_slave_sg = zf_hdma_device_prep_slave_sg;     // prepares a slave dma operation
    dd->device_prep_dma_cyclic = zf_hdma_device_prep_dma_cyclic; // prepare a cyclic dma operation suitable for audio. The function takes a buffer of size buf_len. The callback function will be called after period_len bytes have been transferred.

    dd->chancnt = ZF_HDMA_CHAN_NUM;
    dd->privatecnt = 0;
    dd->copy_align = ZF_HDMA_ALIGN_SIZE;
    dd->src_addr_widths = ZF_HDMA_DMA_BUSWIDTHS;
    dd->dst_addr_widths = ZF_HDMA_DMA_BUSWIDTHS;
    dd->residue_granularity = DMA_RESIDUE_GRANULARITY_SEGMENT;
    dd->dev = dev;

    if (is_read)
    {
        dd->directions = BIT(DMA_MEM_TO_DEV);
    }
    else
    {
        dd->directions = BIT(DMA_DEV_TO_MEM);
    }

    dma_cap_zero(dd->cap_mask);
    dma_cap_set(DMA_MEMCPY, dd->cap_mask);
}

__attribute__((unused)) int callback_thread_function(void *data)
{
    u32 chan_status = 0;
    struct pcie_dpu_ep *dpu_dev = data;
    struct dma_chan *chan = NULL;
    struct zf_hdma_chan *zf_chan = NULL;
    struct zf_hdma_tx *zf_tx_desc = NULL;

    while (!kthread_should_stop())
    {
        list_for_each_entry(chan, &dpu_dev->rd_dd->channels, device_node)
        {
            zf_chan = to_zf_hdma_chan(chan);
            if (!zf_chan->tx_list->next || !zf_chan->tx_list->next->tx_desc || !zf_chan->tx_list->next->callback)
            {
                continue;
            }
            else
            {
                if ((zf_chan->sqe_list->next != NULL) && (zf_chan->is_busy == HDMA_CHAN_IDLE))
                {
                    zf_hdma_register_set(zf_chan, HDMA_RD, zf_chan->sqe_list->next);
                    zf_chan->is_busy = HDMA_CHAN_USED;
                    // DH_LOG_DEBUG(MODULE_MPF, "zf_chan->id:%d is_pending\n", zf_chan->id);
                    free_used_sqe(zf_chan);
                }

                zf_tx_desc = zf_chan->tx_list->next;
                // DH_LOG_DEBUG(MODULE_MPF, "rd_zf_chan%d is used!\n", zf_chan->id);

                read_ch(zf_chan, HDMA_RD, HDMA_STATUS_OFF, &chan_status);

                if ((chan_status & HDMA_STATUS_OFF_STATUS) == HDMA_STATUS_STOPPED && (zf_chan->is_busy == HDMA_CHAN_USED))
                {
                    spin_lock(&zf_chan->zxdh_vc.lock);
                    zf_tx_desc->callback(zf_tx_desc->callback_param);
                    free_used_tx(zf_chan);
                    spin_unlock(&zf_chan->zxdh_vc.lock);
                    zf_chan->is_busy = HDMA_CHAN_IDLE;
                }
            }
        }

        list_for_each_entry(chan, &dpu_dev->wr_dd->channels, device_node)
        {
            zf_chan = to_zf_hdma_chan(chan);
            if (!zf_chan->tx_list->next || !zf_chan->tx_list->next->tx_desc || !zf_chan->tx_list->next->callback)
            {
                continue;
            }
            else
            {
                if ((zf_chan->sqe_list->next != NULL) && (zf_chan->is_busy == HDMA_CHAN_IDLE))
                {
                    zf_hdma_register_set(zf_chan, HDMA_WR, zf_chan->sqe_list->next);
                    zf_chan->is_busy = HDMA_CHAN_USED;
                    // DH_LOG_DEBUG(MODULE_MPF, "zf_chan->id:%d is_pending\n", zf_chan->id);
                    free_used_sqe(zf_chan);
                }

                zf_tx_desc = zf_chan->tx_list->next;
                // DH_LOG_DEBUG(MODULE_MPF, "wr_zf_chan%d is used!\n", zf_chan->id);
                read_ch(zf_chan, HDMA_WR, HDMA_STATUS_OFF, &chan_status);

                if ((chan_status & HDMA_STATUS_OFF_STATUS) == HDMA_STATUS_STOPPED && (zf_chan->is_busy == HDMA_CHAN_USED))
                {
                    spin_lock(&zf_chan->zxdh_vc.lock);
                    zf_tx_desc->callback(zf_tx_desc->callback_param);
                    free_used_tx(zf_chan);
                    spin_unlock(&zf_chan->zxdh_vc.lock);
                    zf_chan->is_busy = HDMA_CHAN_IDLE;
                }
            }
        }
        msleep(1);
    }

    DH_LOG_INFO(MODULE_MPF, "Kernel thread is stopping\n");
    return 0;
}

int pcie_zf_dma_init(struct pcie_dpu_ep *dpu_dev, struct pci_dev *pdev)
{
#if ZF_MPF_HDMA_MODE_ENABLE
    int ret = 0;
    int node = 0;
    struct device *dev_wr = NULL, *dev_rd = NULL;
    struct dma_device *wr_dd = NULL, *rd_dd = NULL;

    if (IS_ERR_OR_NULL(dpu_dev))
    {
        DH_LOG_ERR(MODULE_MPF, "err input\n");
        return -ENOENT;
    }

    dev_wr = &(dpu_dev->zf_pdev->dev);
    dev_rd = &(dpu_dev->zf_pdev_dma->dev);

    node = dev_to_node(dev_wr);

    wr_dd = kzalloc_node(sizeof(struct dma_device), GFP_KERNEL, node);
    if (!wr_dd)
    {
        DH_LOG_ERR(MODULE_MPF, "Error kzalloc node\n");
        return -ENODEV;
    }

    rd_dd = kzalloc_node(sizeof(struct dma_device), GFP_KERNEL, node);
    if (!rd_dd)
    {
        DH_LOG_ERR(MODULE_MPF, "Error kzalloc node\n");
        kfree(wr_dd);
        return -ENODEV;
    }

    zf_hdma_device_init(dev_wr, wr_dd, HDMA_WR);
    zf_hdma_device_init(dev_rd, rd_dd, HDMA_RD);

    zf_hdma_virtual_channels_init(wr_dd, pdev, dpu_dev->dbi_base);
    zf_hdma_virtual_channels_init(rd_dd, pdev, dpu_dev->dbi_base);

    ret = dma_async_device_register(wr_dd);
    ret |= dma_async_device_register(rd_dd);
    if (ret)
    {
        DH_LOG_ERR(MODULE_MPF, "dma_async_device_register failed\n");
        goto free_dma_device;
    }

    dpu_dev->wr_dd = wr_dd;
    dpu_dev->rd_dd = rd_dd;

    if (dpu_dev->ep_id >= 0 && dpu_dev->ep_id < PCIE_DPU_EP_NUMS)
    {
        callback_thread[dpu_dev->ep_id] = kthread_run(callback_thread_function, dpu_dev, "my_thread");
        if (callback_thread[dpu_dev->ep_id])
        {
            DH_LOG_INFO(MODULE_MPF, "Thread created successfully\n");
        }
        else
        {
            DH_LOG_ERR(MODULE_MPF, "Thread creation failed\n");
        }
    }

    return 0;

free_dma_device:
    kfree(wr_dd);
    kfree(rd_dd);
    return ret;
#endif
    return 0;
}

void pcie_zf_dma_free(struct pcie_dpu_ep *dpu_dev, struct pci_dev *pdev)
{
#if ZF_MPF_HDMA_MODE_ENABLE
    if (dpu_dev == NULL || pdev == NULL)
    {
        DH_LOG_ERR(MODULE_MPF, "dpu_dev or pdev is NULL\n");
        return;
    }

    if (dpu_dev->ep_id >= 0 && dpu_dev->ep_id < PCIE_DPU_EP_NUMS)
    {
        if (callback_thread[dpu_dev->ep_id])
        {
            kthread_stop(callback_thread[dpu_dev->ep_id]);
            DH_LOG_INFO(MODULE_MPF, "Thread created successfully\n");
        }
    }

    if (dpu_dev->wr_dd)
    {
        dma_async_device_unregister(dpu_dev->wr_dd);
        kfree(dpu_dev->wr_dd);
    }
    if (dpu_dev->rd_dd)
    {
        dma_async_device_unregister(dpu_dev->rd_dd);
        kfree(dpu_dev->rd_dd);
    }
#endif
}

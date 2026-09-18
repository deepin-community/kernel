#include <linux/acpi.h>
#include <linux/acpi_dma.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/idr.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/mempool.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/numa.h>
#include <linux/of_dma.h>
#include <linux/rculist.h>
#include <linux/rcupdate.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>

#include "gdma.h"
#include "zf_mpf.h"
#include "epc/pcie-zte-zf-epc.h"

/*
 * User define:
 * ep_id-bit[15:12] vfunc_num-bit[11:4] func_num-bit[3:1] vfunc_active-bit0
 * host ep_id:5~8   zf ep_id:9
 */
#define ZF_GDMA_ZF_USER                         (0x9000)  /* ep4 pf0 */
#define ZF_GDMA_PF_NUM_SHIFT                    (1)
#define ZF_GDMA_VF_NUM_SHIFT                    (4)
#define ZF_GDMA_EP_ID_SHIFT                     (12)
#define ZF_GDMA_VF_EN                           (1)
#define ZF_GDMA_VF_MASK                         (1UL << 7)

/* Register offset */
#define ZF_GDMA_BASE_OFFSET                     (0x100000)
#define ZF_GDMA_CHAN_SHIFT                      (0x80)
#define ZF_GDMA_EXT_ADDR_OFFSET                 (0x218)
#define ZF_GDMA_SAR_LOW_OFFSET                  (0x200)
#define ZF_GDMA_DAR_LOW_OFFSET                  (0x204)
#define ZF_GDMA_SAR_HIGH_OFFSET                 (0x234)
#define ZF_GDMA_DAR_HIGH_OFFSET                 (0x238)
#define ZF_GDMA_XFERSIZE_OFFSET                 (0x208)
#define ZF_GDMA_CONTROL_OFFSET                  (0x230)
#define ZF_GDMA_TC_STATUS_OFFSET                (0x0)
#define ZF_GDMA_STATUS_CLEAN_OFFSET             (0x80)
#define ZF_GDMA_LINKADDR_LOW_OFFSET             (0x21c)
#define ZF_GDMA_LINKADDR_HIGH_OFFSET            (0x220)
#define ZF_GDMA_CHAN_CONTINUE_OFFSET            (0x224)
#define ZF_GDMA_TC_CNT_OFFSET                   (0x23c)
#define ZF_GDMA_LLI_USER_OFFSET                 (0x228)
#define ZF_GDMA_PULSE_WIDTH_OFFSET              (0x1ec)

/* Control register */
#define ZF_GDMA_CHAN_ENABLE                     (1UL)
#define ZF_GDMA_SOFT_CHAN                       (1UL << 1)
#define ZF_GDMA_TC_INTR_ENABLE                  (1UL << 4)
#define ZF_GDMA_ERR_INTR_ENABLE                 (1UL << 5)
#define ZF_GDMA_SBS_SHIFT                       (6)           /* src burst size */
#define ZF_GDMA_SBL_SHIFT                       (9)           /* src burst length */
#define ZF_GDMA_DBS_SHIFT                       (13)          /* dest burst size */
#define ZF_GDMA_BURST_SIZE_MIN                  (0x1)         /* 1 byte */
#define ZF_GDMA_BURST_SIZE_MEDIUM               (0x4)         /* 4 word */
#define ZF_GDMA_BURST_SIZE_MAX                  (0x6)         /* 16 word */
#define ZF_GDMA_DEFAULT_BURST_LEN               (0xf)         /* 16 beats */
#define ZF_GDMA_TC_CNT_ENABLE                   (1UL << 27)
#define ZF_GDMA_CHAN_FORCE_CLOSE                (1UL << 31)

/* TC count & Error interrupt status register */
#define ZF_GDMA_SRC_LLI_ERR                     (1UL << 16)
#define ZF_GDMA_SRC_DATA_ERR                    (1UL << 17)
#define ZF_GDMA_DST_ADDR_ERR                    (1UL << 18)
#define ZF_GDMA_ERR_STATUS                      (1UL << 19)
#define ZF_GDMA_ERR_RPT_ENABLE                  (1UL << 20)
#define ZF_GDMA_TC_CNT_CLEAN                    (1)

#define ZF_GDMA_ALIGN_SIZE                      (1)
#define ZF_GDMA_DMA_BUSWIDTHS                   (BIT(DMA_SLAVE_BUSWIDTH_4_BYTES))
#define ZF_GDMA_BUFF_SIZE_MAX                   (0xfffff)

#define LOW16_MASK                              (0xffff)
#define LOW32_MASK                              (0xffffffff)

extern int is_pcie_ep_link(int ep_id);

void gchan_irq_tasklet_process(unsigned long data);
static void zf_gdma_enqueue_buff(struct zf_gdma_chan *gchan);

static struct zf_gdma_chan *to_zf_gdma_chan(struct dma_chan *chan)
{
    return container_of(chan, struct zf_gdma_chan, vc.chan);
}

static struct zf_gdma_desc *to_zf_gdma_desc(struct zxdh_virt_dma_desc *vdesc)
{
    return container_of(vdesc, struct zf_gdma_desc, vd);
}

static inline uint32_t zf_gdma_read_reg(struct zf_gdma_chan *gchan, uint16_t chan_id, uint32_t offset)
{
    uint64_t addr = chan_id * ZF_GDMA_CHAN_SHIFT + offset;

    return *(volatile uint32_t *)(gchan->gdev->base_addr + addr);
}

static inline void zf_gdma_write_reg(struct zf_gdma_chan *gchan, uint16_t chan_id, uint32_t offset, uint32_t val)
{
    uint64_t addr = chan_id * ZF_GDMA_CHAN_SHIFT + offset;

    *(volatile uint32_t *)(gchan->gdev->base_addr + addr) = val;
}

static inline void zf_gdma_user_get(struct zf_rbp_info *rbp_info, uint32_t *user)
{
    uint32_t pf_id = rbp_info->pfid;

    //host addr
    if (rbp_info->host)
    {
        if ((pf_id & ZF_GDMA_VF_MASK) != 0)
        {
            pf_id &= ~ZF_GDMA_VF_MASK;
            *user = (ZF_GDMA_VF_EN | (rbp_info->vfid << ZF_GDMA_VF_NUM_SHIFT));
        }
        *user |= ((rbp_info->epid << ZF_GDMA_EP_ID_SHIFT) | (pf_id << ZF_GDMA_PF_NUM_SHIFT));
    }
    else
    {
        *user = ZF_GDMA_ZF_USER;
    }
}

/* 从user中解析出逻辑上的EP_ID: src_epid和dst_epid */
static inline uint8_t zf_gdma_get_epid_from_user(uint8_t *src_epid, uint8_t *dst_epid, uint32_t user)
{
    uint8_t src_ep_id = 0;
    uint8_t dst_ep_id = 0;
    uint8_t host_ep = 0;

    src_ep_id = (user & 0x0000F000) >> ZF_GDMA_EP_ID_SHIFT;
    dst_ep_id = ((user >> 16)  & 0x0000F000) >> ZF_GDMA_EP_ID_SHIFT;

    //硬件上EP编号5~9，所以逻辑上的EP对应硬件EP编号减去5.
    if ((src_ep_id < 5) || (dst_ep_id < 5))
    {
        src_ep_id = 9;
        dst_ep_id = 9;
    }

    *src_epid = src_ep_id - 5;
    *dst_epid = dst_ep_id - 5;

    host_ep = (*src_epid == 4)?(*dst_epid):(*src_epid);
    return host_ep;
}

static inline void zf_gdma_cfg_get(uint32_t *val, uint8_t tc_enable)
{
    *val = (ZF_GDMA_CHAN_ENABLE | ZF_GDMA_SOFT_CHAN |
            ZF_GDMA_TC_INTR_ENABLE | ZF_GDMA_ERR_INTR_ENABLE |
            (ZF_GDMA_DEFAULT_BURST_LEN << ZF_GDMA_SBL_SHIFT) |
            (ZF_GDMA_BURST_SIZE_MAX << ZF_GDMA_SBS_SHIFT) |
            (ZF_GDMA_BURST_SIZE_MAX << ZF_GDMA_DBS_SHIFT));

    if (tc_enable != 0)
    {
        *val |= ZF_GDMA_TC_CNT_ENABLE;
    }
}

static void zf_gdma_desc_free(struct zxdh_virt_dma_desc *vd)
{
    if (vd != NULL)
    {
        kfree(to_zf_gdma_desc(vd));
    }
}

static int32_t zf_gdma_alloc_chan_resources(struct dma_chan *chan)
{
    return 0;
}

static void zf_gdma_free_chan_resources(struct dma_chan *chan)
{

}

static int32_t zf_gdma_device_config(struct dma_chan *chan, struct dma_slave_config *config)
{
    return 0;
}

static struct dma_async_tx_descriptor *zf_gdma_prep_dma_memcpy(struct dma_chan *chan,
                                                               dma_addr_t dst,
                                                               dma_addr_t src,
                                                               size_t len,
                                                               unsigned long flags)
{
    struct zf_gdma_chan *gchan = NULL;
    struct zf_gdma_desc *desc = NULL;
    struct zf_dma_addr_rbp *srbp = NULL;
    struct zf_dma_addr_rbp *drbp = NULL;
    uint32_t src_user = 0;
    uint32_t dst_user = 0;
    unsigned long status = 0;

    if ((chan == NULL) || ((void *)dst == NULL) || ((void *)src == NULL))
    {
        printk(KERN_ERR "%s:param is invalid\n", __func__);
        return ERR_PTR(-EINVAL);
    }

    gchan = to_zf_gdma_chan(chan);
    srbp = (struct zf_dma_addr_rbp *)src;
    drbp = (struct zf_dma_addr_rbp *)dst;

    desc = kzalloc(sizeof(struct zf_gdma_desc), GFP_KERNEL);
    if (desc == NULL)
    {
        printk(KERN_ERR "%s: Failed to alloc gdma desc\n", __func__);
        return ERR_PTR(-ENOMEM);
    }

    zf_gdma_user_get(&srbp->rbp_info, &src_user);
    zf_gdma_user_get(&drbp->rbp_info, &dst_user);

    desc->user = ((src_user & LOW16_MASK) | (dst_user << 16));
    desc->src  = (uint64_t)(srbp->addr);
    desc->dst  = (uint64_t)(drbp->addr);
    desc->len  = (uint64_t)len;
    desc->chan = gchan;
    spin_lock_irqsave(&gchan->chan_lock, status);
    list_add_tail(&desc->node, &gchan->desc_list);
    spin_unlock_irqrestore(&gchan->chan_lock, status);

    return zxdh_vchan_tx_prep(&gchan->vc, &desc->vd, flags);
}

static void zf_gdma_issue_pending(struct dma_chan *chan)
{
    struct zf_gdma_chan *gchan = NULL;
    unsigned long flags = 0;
    bool pending = false;

    if (chan == NULL)
        return;

    gchan = to_zf_gdma_chan(chan);

    spin_lock_irqsave(&gchan->vc.lock, flags);
    if (zxdh_vchan_issue_pending(&gchan->vc))
    {
        pending = true;
    }
    spin_unlock_irqrestore(&gchan->vc.lock, flags);

    if (pending)
    {
        zf_gdma_enqueue_buff(gchan);
    }

    return;
}

static enum dma_status zf_gdma_tx_status(struct dma_chan *chan, dma_cookie_t cookie, struct dma_tx_state *state)
{
     return 0;
}

static int32_t zf_gdma_terminate_all(struct dma_chan *chan)
{
    return 0;
}

void zf_gdma_dev_init(struct device *dev, struct dma_device *dd)
{
    dd->device_alloc_chan_resources = zf_gdma_alloc_chan_resources;
    dd->device_free_chan_resources = zf_gdma_free_chan_resources;
    dd->device_config = zf_gdma_device_config;
    dd->device_prep_dma_memcpy = zf_gdma_prep_dma_memcpy;
    dd->device_issue_pending = zf_gdma_issue_pending;
    dd->device_tx_status = zf_gdma_tx_status;
    dd->device_terminate_all = zf_gdma_terminate_all;

    dd->chancnt = ZF_GDMA_CHAN_NUM;
    dd->privatecnt = 0;
    dd->copy_align = ZF_GDMA_ALIGN_SIZE;
    dd->src_addr_widths = ZF_GDMA_DMA_BUSWIDTHS;
    dd->dst_addr_widths = ZF_GDMA_DMA_BUSWIDTHS;
    dd->residue_granularity = DMA_RESIDUE_GRANULARITY_SEGMENT;
    dd->dev = dev;

    dma_cap_zero(dd->cap_mask);
    dma_cap_set(DMA_RBP, dd->cap_mask);
}

static int32_t zf_gdma_virt_chan_init(struct zf_gdma_dev *gdev)
{
    struct zf_gdma_chan *gchan = NULL;
    uint32_t val = 0;
    uint16_t i = 0;

    INIT_LIST_HEAD(&gdev->dd->channels);
    for (i = 0; i < ZF_GDMA_CHAN_NUM; i++)
    {
        gchan = &gdev->chan[i];
        gchan->status = GDMA_CHAN_IDLE;
        gchan->chan_id = ZF_GDMA_CHAN_BASE + i;
        gchan->gdev = gdev;
        gchan->vc.desc_free = zf_gdma_desc_free;
        tasklet_init(&gchan->task, gchan_irq_tasklet_process, (unsigned long)gchan);

        zxdh_vchan_init(&gchan->vc, gdev->dd);
        spin_lock_init(&gchan->chan_lock);
        INIT_LIST_HEAD(&gchan->desc_list);

        /* reset gdma channel */
        val = ZF_GDMA_CHAN_FORCE_CLOSE;
        zf_gdma_write_reg(gchan, gchan->chan_id, ZF_GDMA_CONTROL_OFFSET, val);

        val = ZF_GDMA_ERR_RPT_ENABLE | ZF_GDMA_ERR_STATUS | ZF_GDMA_TC_CNT_CLEAN;
        zf_gdma_write_reg(gchan, gchan->chan_id, ZF_GDMA_TC_CNT_OFFSET, val);
    }

    /* Configure interrupt pulse width to 8 cycle */
    zf_gdma_write_reg(gchan, 0, ZF_GDMA_PULSE_WIDTH_OFFSET, 7);

    return 0;
}

static int32_t zf_gdma_xmit_done(struct zf_gdma_chan *gchan)
{
    struct zxdh_virt_dma_desc *vdesc = NULL;
    uint32_t widx = gchan->chan_id / 32;
    uint32_t bidx = gchan->chan_id % 32;
    uint32_t val = 0;
    unsigned long flags = 0;

    val = zf_gdma_read_reg(gchan, 0, ZF_GDMA_TC_STATUS_OFFSET + (widx * sizeof(uint32_t)));
    if ((val & (1UL << bidx)) == 0)
    {
        printk(KERN_ERR "%s:chan%d tc status error\n", __func__, gchan->chan_id);
        if (gchan->desc != NULL)
            printk(KERN_ERR "src:%llx dst:%llx\n", gchan->desc->src, gchan->desc->dst);
        spin_lock(&gchan->chan_lock);
        gchan->status = GDMA_CHAN_ERR;
        spin_unlock(&gchan->chan_lock);
        return -1;
    }

    if (gchan->desc != NULL)
        pr_debug("ged: ch:%u src:%llx dst:%llx\n", gchan->chan_id, gchan->desc->src, gchan->desc->dst);

    zf_gdma_write_reg(gchan, 0, ZF_GDMA_STATUS_CLEAN_OFFSET + (widx * sizeof(uint32_t)), 1 << bidx);

    spin_lock(&gchan->chan_lock);
    if (gchan->desc == NULL)
    {
        printk(KERN_ERR "%s:chan%d descriptor missing\n", __func__, gchan->chan_id);
        gchan->status = GDMA_CHAN_ERR;
        spin_unlock(&gchan->chan_lock);
        return -1;
    }

    if (gchan->desc->len == 0)
    {
        list_del(&gchan->desc->node);
        vdesc = &gchan->desc->vd;
        gchan->desc = NULL;

        spin_lock_irqsave(&gchan->vc.lock, flags);
        list_del(&vdesc->node);
        zxdh_vchan_cookie_complete(vdesc);
        spin_unlock_irqrestore(&gchan->vc.lock, flags);
    }
    gchan->status = GDMA_CHAN_IDLE;
    spin_unlock(&gchan->chan_lock);

    return 0;
}

static void zf_gdma_enqueue_buff(struct zf_gdma_chan *gchan)
{
    struct zf_gdma_desc *desc = NULL;
    struct zxdh_virt_dma_desc *vdesc = NULL;
    uint32_t val = 0;
    uint64_t cur_len = 0;
    unsigned long flags = 0;

    spin_lock_irqsave(&gchan->chan_lock, flags);
    if (gchan->status != GDMA_CHAN_IDLE)
    {
        goto out;
    }
    spin_lock(&gchan->vc.lock);
    vdesc = zxdh_vchan_next_desc(&gchan->vc);
    spin_unlock(&gchan->vc.lock);
    if (vdesc == NULL)
    {
        goto out;
    }
    gchan->status = GDMA_CHAN_BUSY;
    spin_unlock_irqrestore(&gchan->chan_lock, flags);

    desc = to_zf_gdma_desc(vdesc);
    gchan->desc = desc;
    cur_len = (desc->len > ZF_GDMA_BUFF_SIZE_MAX) ? ZF_GDMA_BUFF_SIZE_MAX : desc->len;
    pr_debug("gst: ch:%u src:%llx dst:%llx len:%llu\n", gchan->chan_id, desc->src, desc->dst, cur_len);

    zf_gdma_write_reg(gchan, gchan->chan_id, ZF_GDMA_SAR_LOW_OFFSET, desc->src & LOW32_MASK);
    zf_gdma_write_reg(gchan, gchan->chan_id, ZF_GDMA_SAR_HIGH_OFFSET, (desc->src >> 32) & LOW32_MASK);

    zf_gdma_write_reg(gchan, gchan->chan_id, ZF_GDMA_DAR_LOW_OFFSET, desc->dst & LOW32_MASK);
    zf_gdma_write_reg(gchan, gchan->chan_id, ZF_GDMA_DAR_HIGH_OFFSET, (desc->dst >> 32) & LOW32_MASK);

    zf_gdma_write_reg(gchan, gchan->chan_id, ZF_GDMA_XFERSIZE_OFFSET, cur_len);

    zf_gdma_write_reg(gchan, gchan->chan_id, ZF_GDMA_EXT_ADDR_OFFSET, desc->user);

    desc->src += cur_len;
    desc->dst += cur_len;
    desc->len -= cur_len;
    smp_mb();

    zf_gdma_cfg_get(&val, 1);
    zf_gdma_write_reg(gchan, gchan->chan_id, ZF_GDMA_CONTROL_OFFSET, val);

    return;

out:
    spin_unlock_irqrestore(&gchan->chan_lock, flags);
}

static void zf_gdma_free_channels(struct zf_gdma_dev *gdev)
{
    struct zf_gdma_chan *gchan = NULL;
    struct zf_gdma_desc *desc = NULL;
    struct zf_gdma_desc *tmp = NULL;
    uint16_t i = 0;
    unsigned long flags = 0;

    for (i = 0; i < ZF_GDMA_CHAN_NUM; i++)
    {
        gchan = &gdev->chan[i];
        tasklet_kill(&gchan->task);

        spin_lock_irqsave(&gchan->chan_lock, flags);
        spin_lock(&gchan->vc.lock);
        list_for_each_entry_safe(desc, tmp, &gchan->desc_list, node)
        {
            list_del(&desc->vd.node);
            list_del(&desc->node);
            kfree(desc);
        }
        spin_unlock(&gchan->vc.lock);
        spin_unlock_irqrestore(&gchan->chan_lock, flags);
    }
}

int32_t dh_zf_mpf_gdma_init(struct dh_core_dev *dh_dev)
{
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dh_dev);
    struct zf_gdma_dev *gdev = NULL;
    struct device *dev = NULL;
    int32_t node = 0;
    int32_t ret = 0;

    if (dh_dev->pdev == NULL)
    {
        printk(KERN_ERR "%s: pdev is invalid\n", __func__);
        return -ENODEV;
    }

    if (mpf_dev->pci_ioremap_addr == 0)
    {
        printk(KERN_ERR "%s: pci_ioremap_addr is invalid\n", __func__);
        return -ENAVAIL;
    }

    gdev = kzalloc(sizeof(struct zf_gdma_dev), GFP_KERNEL);
    if (gdev == NULL)
    {
        printk(KERN_ERR "%s: Failed to alloc gdev\n", __FUNCTION__);
        return -ENOMEM;
    }
    mpf_dev->gdev = gdev;

    if (dh_dev->zf_ep->vsock_vaddr != NULL)
    {
        gdev->base_addr = (uint64_t)(dh_dev->zf_ep->vsock_vaddr) + ZF_GDMA_BASE_OFFSET;
    }
    else
    {
        printk(KERN_ERR "%s: MPF VF BAR0 is not mapped\n", __FUNCTION__);
        ret = -ENOMEM;
        goto free_gdev;
    }

    gdev->pdev = dh_dev->pdev;
    dev = &(dh_dev->pdev->dev);
    node = dev_to_node(dev);
    gdev->dd = kzalloc_node(sizeof(struct dma_device), GFP_KERNEL, node);
    if (gdev->dd == NULL)
    {
        printk(KERN_ERR "%s: Failed to alloc dma_device\n", __FUNCTION__);
        ret = -ENOMEM;
        goto free_gdev;
    }

    zf_gdma_dev_init(dev, gdev->dd);
    zf_gdma_virt_chan_init(gdev);

    ret = dma_async_device_register(gdev->dd);
    if (ret != 0)
    {
        printk(KERN_ERR "%s: Failed to register gdma device\n", __func__);
        goto err_out;
    }

    return 0;

err_out:
    zf_gdma_free_channels(gdev);
    kfree(gdev->dd);
free_gdev:
    kfree(gdev);
    return ret;
}

void dh_zf_mpf_gdma_uninit(struct dh_core_dev *dh_dev)
{
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dh_dev);
    struct zf_gdma_dev *gdev = NULL;

    if (mpf_dev->gdev == NULL)
    {
        printk(KERN_ERR "%s:gdev is invalid\n", __func__);
        return;
    }
    gdev = mpf_dev->gdev;

    if (gdev->dd != NULL)
    {
        dma_async_device_unregister(gdev->dd);
        kfree(gdev->dd);
    }
    zf_gdma_free_channels(gdev);
    kfree(gdev);
}

int32_t zf_gdma_err_irq_handle(struct notifier_block *nb, unsigned long action, void *data)
{
    pr_debug("%s is called\n", __func__);

    return 0;
}

int32_t zf_gdma_chan_irq_handle(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async *eq = container_of(nb, struct dh_eq_async, irq_nb);
    struct zf_gdma_chan *gchan = NULL;

    if (eq->priv == NULL)
    {
        printk(KERN_ERR "%s:eq->priv is NULL\n", __func__);
        return -1;
    }
    gchan = (struct zf_gdma_chan *)eq->priv;
    tasklet_hi_schedule(&gchan->task);

    return 0;
}

void gchan_irq_tasklet_process(unsigned long data)
{
    struct zf_gdma_chan *gchan = NULL;

    if (unlikely(data == 0))
    {
        printk(KERN_ERR "%s:param is invalid\n", __func__);
        return;
    }

    gchan = (struct zf_gdma_chan *)data;
    if (zf_gdma_xmit_done(gchan) != 0)
        return;

    zf_gdma_enqueue_buff(gchan);
}
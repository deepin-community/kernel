#include <linux/device.h>
#include <linux/dmaengine.h>
#include <linux/module.h>
#include <linux/spinlock.h>

#include "virt-dma.h"

static struct zxdh_virt_dma_desc *zxdh_to_virt_desc(struct dma_async_tx_descriptor *tx)
{
    return container_of(tx, struct zxdh_virt_dma_desc, tx);
}

dma_cookie_t zxdh_vchan_tx_submit(struct dma_async_tx_descriptor *tx)
{
    struct zxdh_virt_dma_chan *zxdh_vc = zxdh_to_virt_chan(tx->chan);
    struct zxdh_virt_dma_desc *zxdh_vd = zxdh_to_virt_desc(tx);
    unsigned long flags;
    dma_cookie_t cookie;

    spin_lock_irqsave(&zxdh_vc->lock, flags);
    cookie = zxdh_dma_cookie_assign(tx);

    list_move_tail(&zxdh_vd->node, &zxdh_vc->desc_submitted);
    spin_unlock_irqrestore(&zxdh_vc->lock, flags);

    DH_LOG_DEBUG(MODULE_MPF, "%s vchan %p: txd %p[%x]: submitted\n", __func__,
                 zxdh_vc, zxdh_vd, cookie);

    return cookie;
}
EXPORT_SYMBOL_GPL(zxdh_vchan_tx_submit);

/**
 * zxdh_vchan_tx_desc_free - free a reusable descriptor
 * @tx: the transfer
 *
 * This function frees a previously allocated reusable descriptor. The only
 * other way is to clear the DMA_CTRL_REUSE flag and submit one last time the
 * transfer.
 *
 * Returns 0 upon success
 */
int zxdh_vchan_tx_desc_free(struct dma_async_tx_descriptor *tx)
{
    struct zxdh_virt_dma_chan *zxdh_vc = zxdh_to_virt_chan(tx->chan);
    struct zxdh_virt_dma_desc *zxdh_vd = zxdh_to_virt_desc(tx);
    unsigned long flags;

    spin_lock_irqsave(&zxdh_vc->lock, flags);
    list_del(&zxdh_vd->node);
    spin_unlock_irqrestore(&zxdh_vc->lock, flags);

    DH_LOG_DEBUG(MODULE_MPF, "%s vchan %p: txd %p[%x]: freeing\n", __func__,
                 zxdh_vc, zxdh_vd, zxdh_vd->tx.cookie);

    zxdh_vc->desc_free(zxdh_vd);
    return 0;
}
EXPORT_SYMBOL_GPL(zxdh_vchan_tx_desc_free);

struct zxdh_virt_dma_desc *
zxdh_vchan_find_desc(struct zxdh_virt_dma_chan *zxdh_vc, dma_cookie_t cookie)
{
    struct zxdh_virt_dma_desc *zxdh_vd;

    list_for_each_entry(zxdh_vd, &zxdh_vc->desc_issued,
                        node) if (zxdh_vd->tx.cookie == cookie) return zxdh_vd;

    return NULL;
}
EXPORT_SYMBOL_GPL(zxdh_vchan_find_desc);

/*
 * This tasklet handles the completion of a DMA descriptor by
 * calling its callback and freeing it.
 */
static void zxdh_vchan_complete(struct tasklet_struct *t)
{
    struct zxdh_virt_dma_chan *zxdh_vc = from_tasklet(zxdh_vc, t, task);
    struct zxdh_virt_dma_desc *zxdh_vd, *_vd;
    struct zxdh_dmaengine_desc_callback cb;
    LIST_HEAD(head);

    spin_lock_irq(&zxdh_vc->lock);
    list_splice_tail_init(&zxdh_vc->desc_completed, &head);
    zxdh_vd = zxdh_vc->cyclic;
    if (zxdh_vd)
    {
        zxdh_vc->cyclic = NULL;
        zxdh_dmaengine_desc_get_callback(&zxdh_vd->tx, &cb);
    }
    else
    {
        memset(&cb, 0, sizeof(cb));
    }
    spin_unlock_irq(&zxdh_vc->lock);

    zxdh_dmaengine_desc_callback_invoke(&cb, &zxdh_vd->tx_result);

    list_for_each_entry_safe(zxdh_vd, _vd, &head, node)
    {
        zxdh_dmaengine_desc_get_callback(&zxdh_vd->tx, &cb);

        list_del(&zxdh_vd->node);
        zxdh_dmaengine_desc_callback_invoke(&cb, &zxdh_vd->tx_result);
        zxdh_vchan_vdesc_fini(zxdh_vd);
    }
}

void zxdh_vchan_dma_desc_free_list(struct zxdh_virt_dma_chan *zxdh_vc,
                                   struct list_head *head)
{
    struct zxdh_virt_dma_desc *zxdh_vd, *_vd;

    list_for_each_entry_safe(zxdh_vd, _vd, head, node)
    {
        list_del(&zxdh_vd->node);
        zxdh_vchan_vdesc_fini(zxdh_vd);
    }
}
EXPORT_SYMBOL_GPL(zxdh_vchan_dma_desc_free_list);

void zxdh_vchan_init(struct zxdh_virt_dma_chan *zxdh_vc,
                     struct dma_device *dmadev)
{
    zxdh_dma_cookie_init(&zxdh_vc->chan);

    spin_lock_init(&zxdh_vc->lock);
    INIT_LIST_HEAD(&zxdh_vc->desc_allocated);
    INIT_LIST_HEAD(&zxdh_vc->desc_submitted);
    INIT_LIST_HEAD(&zxdh_vc->desc_issued);
    INIT_LIST_HEAD(&zxdh_vc->desc_completed);
    INIT_LIST_HEAD(&zxdh_vc->desc_terminated);

    tasklet_setup(&zxdh_vc->task, zxdh_vchan_complete);

    zxdh_vc->chan.device = dmadev;
    list_add_tail(&zxdh_vc->chan.device_node, &dmadev->channels);
}
EXPORT_SYMBOL_GPL(zxdh_vchan_init);

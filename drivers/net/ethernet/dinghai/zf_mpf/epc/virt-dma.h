#ifndef ZXDH_VIRT_DMA_H
#define ZXDH_VIRT_DMA_H

#include <linux/dmaengine.h>
#include <linux/interrupt.h>
#include <linux/dinghai/log.h>
#include "dmaengine.h"

struct zxdh_virt_dma_desc {
	struct dma_async_tx_descriptor tx;
	struct dmaengine_result tx_result;
	/* protected by zxdh_vc.lock */
	struct list_head node;
};

struct zxdh_virt_dma_chan {
	struct dma_chan	chan;
	struct tasklet_struct task;
	void (*desc_free)(struct zxdh_virt_dma_desc *);

	spinlock_t lock;

	/* protected by zxdh_vc.lock */
	struct list_head desc_allocated;
	struct list_head desc_submitted;
	struct list_head desc_issued;
	struct list_head desc_completed;
	struct list_head desc_terminated;

	struct zxdh_virt_dma_desc *cyclic;
};

static inline struct zxdh_virt_dma_chan *zxdh_to_virt_chan(struct dma_chan *chan)
{
	return container_of(chan, struct zxdh_virt_dma_chan, chan);
}

void zxdh_vchan_dma_desc_free_list(struct zxdh_virt_dma_chan *zxdh_vc, struct list_head *head);
void zxdh_vchan_init(struct zxdh_virt_dma_chan *zxdh_vc, struct dma_device *dmadev);
struct zxdh_virt_dma_desc *zxdh_vchan_find_desc(struct zxdh_virt_dma_chan *, dma_cookie_t);
extern dma_cookie_t zxdh_vchan_tx_submit(struct dma_async_tx_descriptor *);
extern int zxdh_vchan_tx_desc_free(struct dma_async_tx_descriptor *);

/**
 * zxdh_vchan_tx_prep - prepare a descriptor
 * @zxdh_vc: virtual channel allocating this descriptor
 * @zxdh_vd: virtual descriptor to prepare
 * @tx_flags: flags argument passed in to prepare function
 */
static inline struct dma_async_tx_descriptor *zxdh_vchan_tx_prep(struct zxdh_virt_dma_chan *zxdh_vc,
	struct zxdh_virt_dma_desc *zxdh_vd, unsigned long tx_flags)
{
	unsigned long flags;

	dma_async_tx_descriptor_init(&zxdh_vd->tx, &zxdh_vc->chan);
	zxdh_vd->tx.flags = tx_flags;
	zxdh_vd->tx.tx_submit = zxdh_vchan_tx_submit;
	zxdh_vd->tx.desc_free = zxdh_vchan_tx_desc_free;

	zxdh_vd->tx_result.result = DMA_TRANS_NOERROR;
	zxdh_vd->tx_result.residue = 0;

	spin_lock_irqsave(&zxdh_vc->lock, flags);
	list_add_tail(&zxdh_vd->node, &zxdh_vc->desc_allocated);
	spin_unlock_irqrestore(&zxdh_vc->lock, flags);

	return &zxdh_vd->tx;
}

/**
 * zxdh_vchan_issue_pending - move submitted descriptors to issued list
 * @zxdh_vc: virtual channel to update
 *
 * zxdh_vc.lock must be held by caller
 */
static inline bool zxdh_vchan_issue_pending(struct zxdh_virt_dma_chan *zxdh_vc)
{
	list_splice_tail_init(&zxdh_vc->desc_submitted, &zxdh_vc->desc_issued);
	return !list_empty(&zxdh_vc->desc_issued);
}

/**
 * zxdh_vchan_cookie_complete - report completion of a descriptor
 * @zxdh_vd: virtual descriptor to update
 *
 * zxdh_vc.lock must be held by caller
 */
static inline void zxdh_vchan_cookie_complete(struct zxdh_virt_dma_desc *zxdh_vd)
{
	struct zxdh_virt_dma_chan *zxdh_vc = zxdh_to_virt_chan(zxdh_vd->tx.chan);
	dma_cookie_t cookie;

	cookie = zxdh_vd->tx.cookie;
	zxdh_dma_cookie_complete(&zxdh_vd->tx);
	dev_vdbg(zxdh_vc->chan.device->dev, "txd %p[%x]: marked complete\n",
		 zxdh_vd, cookie);
	list_add_tail(&zxdh_vd->node, &zxdh_vc->desc_completed);

	tasklet_schedule(&zxdh_vc->task);
}

/**
 * zxdh_vchan_vdesc_fini - Free or reuse a descriptor
 * @zxdh_vd: virtual descriptor to free/reuse
 */
static inline void zxdh_vchan_vdesc_fini(struct zxdh_virt_dma_desc *zxdh_vd)
{
	struct zxdh_virt_dma_chan *zxdh_vc = zxdh_to_virt_chan(zxdh_vd->tx.chan);

	if (dmaengine_desc_test_reuse(&zxdh_vd->tx)) {
		unsigned long flags;

		spin_lock_irqsave(&zxdh_vc->lock, flags);
		list_add(&zxdh_vd->node, &zxdh_vc->desc_allocated);
		spin_unlock_irqrestore(&zxdh_vc->lock, flags);
	} else {
		zxdh_vc->desc_free(zxdh_vd);
	}
}

/**
 * zxdh_vchan_cyclic_callback - report the completion of a period
 * @zxdh_vd: virtual descriptor
 */
static inline void zxdh_vchan_cyclic_callback(struct zxdh_virt_dma_desc *zxdh_vd)
{
	struct zxdh_virt_dma_chan *zxdh_vc = zxdh_to_virt_chan(zxdh_vd->tx.chan);

	zxdh_vc->cyclic = zxdh_vd;
	tasklet_schedule(&zxdh_vc->task);
}

/**
 * zxdh_vchan_terminate_vdesc - Disable pending cyclic callback
 * @zxdh_vd: virtual descriptor to be terminated
 *
 * zxdh_vc.lock must be held by caller
 */
static inline void zxdh_vchan_terminate_vdesc(struct zxdh_virt_dma_desc *zxdh_vd)
{
	struct zxdh_virt_dma_chan *zxdh_vc = zxdh_to_virt_chan(zxdh_vd->tx.chan);

	list_add_tail(&zxdh_vd->node, &zxdh_vc->desc_terminated);

	if (zxdh_vc->cyclic == zxdh_vd)
		zxdh_vc->cyclic = NULL;
}

/**
 * zxdh_vchan_next_desc - peek at the next descriptor to be processed
 * @zxdh_vc: virtual channel to obtain descriptor from
 *
 * zxdh_vc.lock must be held by caller
 */
static inline struct zxdh_virt_dma_desc *zxdh_vchan_next_desc(struct zxdh_virt_dma_chan *zxdh_vc)
{
	return list_first_entry_or_null(&zxdh_vc->desc_issued,
					struct zxdh_virt_dma_desc, node);
}

/**
 * zxdh_vchan_get_all_descriptors - obtain all submitted and issued descriptors
 * @zxdh_vc: virtual channel to get descriptors from
 * @head: list of descriptors found
 *
 * zxdh_vc.lock must be held by caller
 *
 * Removes all submitted and issued descriptors from internal lists, and
 * provides a list of all descriptors found
 */
static inline void zxdh_vchan_get_all_descriptors(struct zxdh_virt_dma_chan *zxdh_vc,
	struct list_head *head)
{
	list_splice_tail_init(&zxdh_vc->desc_allocated, head);
	list_splice_tail_init(&zxdh_vc->desc_submitted, head);
	list_splice_tail_init(&zxdh_vc->desc_issued, head);
	list_splice_tail_init(&zxdh_vc->desc_completed, head);
	list_splice_tail_init(&zxdh_vc->desc_terminated, head);
}

static inline void zxdh_vchan_free_chan_resources(struct zxdh_virt_dma_chan *zxdh_vc)
{
	struct zxdh_virt_dma_desc *zxdh_vd;
	unsigned long flags;
	LIST_HEAD(head);

	spin_lock_irqsave(&zxdh_vc->lock, flags);
	zxdh_vchan_get_all_descriptors(zxdh_vc, &head);
	list_for_each_entry(zxdh_vd, &head, node)
		dmaengine_desc_clear_reuse(&zxdh_vd->tx);
	spin_unlock_irqrestore(&zxdh_vc->lock, flags);

	zxdh_vchan_dma_desc_free_list(zxdh_vc, &head);
}

/**
 * zxdh_vchan_synchronize() - synchronize callback execution to the current context
 * @zxdh_vc: virtual channel to synchronize
 *
 * Makes sure that all scheduled or active callbacks have finished running. For
 * proper operation the caller has to ensure that no new callbacks are scheduled
 * after the invocation of this function started.
 * Free up the terminated cyclic descriptor to prevent memory leakage.
 */
static inline void zxdh_vchan_synchronize(struct zxdh_virt_dma_chan *zxdh_vc)
{
	LIST_HEAD(head);
	unsigned long flags;

	tasklet_kill(&zxdh_vc->task);

	spin_lock_irqsave(&zxdh_vc->lock, flags);

	list_splice_tail_init(&zxdh_vc->desc_terminated, &head);

	spin_unlock_irqrestore(&zxdh_vc->lock, flags);

	zxdh_vchan_dma_desc_free_list(zxdh_vc, &head);
}

#endif

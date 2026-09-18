#ifndef __GDMA_H
#define __GDMA_H

#include <linux/bitfield.h>
#include <linux/dinghai/driver.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/dmaengine.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of_dma.h>
#include <linux/spinlock.h>

#include "./epc/virt-dma.h"

#define ZF_GDMA_CHAN_NUM                        (4)
#define ZF_GDMA_CHAN_BASE                       (58)

enum zf_gdma_chan_status
{
    GDMA_CHAN_IDLE = 0,
    GDMA_CHAN_BUSY,
    GDMA_CHAN_ERR
};

struct zf_gdma_chan
{
    enum zf_gdma_chan_status status;
    uint16_t chan_id;

    struct list_head desc_list;
    spinlock_t chan_lock;

    struct zf_gdma_dev *gdev;
    struct zxdh_virt_dma_chan vc;
    struct zf_gdma_desc *desc;
    struct tasklet_struct task;
};

struct zf_gdma_desc
{
    uint64_t src;                   /* src addr */
    uint64_t dst;
    uint64_t len;
    uint32_t user;
    struct zxdh_virt_dma_desc vd;
    struct list_head node;
    struct zf_gdma_chan *chan;
};

struct zf_gdma_dev
{
    uint64_t base_addr;
    struct dma_device *dd;
    struct pci_dev *pdev;
    struct zf_gdma_chan chan[ZF_GDMA_CHAN_NUM];
};

int32_t dh_zf_mpf_gdma_init(struct dh_core_dev *dh_dev);
void dh_zf_mpf_gdma_uninit(struct dh_core_dev *dh_dev);
int32_t zf_gdma_err_irq_handle(struct notifier_block *nb, unsigned long action, void *data);
int32_t zf_gdma_chan_irq_handle(struct notifier_block *nb, unsigned long action, void *data);

#endif
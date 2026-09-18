#include <linux/pci.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/helper.h>
#include "en_pf_irq.h"
#include "en_pf_eq.h"
#include "en_pf.h"

#define ZXDH_PF_ASYNC_IRQ_MIN_COMP 0
#define ZXDH_PF_ASYNC_IRQ_MAX_COMP 7

#define ZXDH_PF_RDMA_IRQ_MIN 0
#define ZXDH_PF_RDMA_IRQ_MAX 5

#define ZXDH_PF_COMP_IRQ_MIN_COMP 0
#define ZXDH_PF_COMP_IRQ_MAX_COMP 1

#define ZXDH_PF_VQ_IRQ_MIN 0
#define ZXDH_PF_VQ_IRQ_MAX 17


struct dh_irq_range {
    int32_t start;
    int32_t size;
};

static int32_t irq_pools_init(struct dh_core_dev *dev, int vq_n, int pf_async_vec)
{
    struct dh_irq_table *table = &dev->irq_table;
    struct dh_pf_irq_table *pf_irq_table = (struct dh_pf_irq_table *)table->priv;
    int32_t err = 0;
    struct dh_irq_range irq_range;

    if(vq_n > 0)
    {
        irq_range.start = ZXDH_VQS_IRQ_START_IDX;
        if (dev->coredev_type == DH_COREDEV_VF)
        {
            irq_range.start = ZXDH_VF_VQS_IRQ_START_IDX;
        }
        irq_range.size = vq_n;

        pf_irq_table->pf_vq_pool = irq_pool_alloc(dev, irq_range.start,
                                                  irq_range.size, "zxdh_pf_vq",
                                                  ZXDH_PF_VQ_IRQ_MIN,
                                                  ZXDH_PF_VQ_IRQ_MAX);
        if (IS_ERR_OR_NULL(pf_irq_table->pf_vq_pool))
        {
            LOG_ERR_DEV(dev, "pf_irq_table->pf_vq_pool irq_pool_alloc failed\n");
            return PTR_ERR(pf_irq_table->pf_vq_pool);
        }

        pf_irq_table->pf_vq_pool->irqs_per_cpu = kcalloc(nr_cpu_ids, sizeof(u16), GFP_KERNEL);
        if (unlikely(pf_irq_table->pf_vq_pool->irqs_per_cpu == NULL))
        {
            LOG_ERR_DEV(dev, "pf_irq_table->pf_vq_pool->irqs_per_cpu kcalloc failed\n");
            err = -ENOMEM;
            goto err_irqs_per_cpu;
        }
    }

    if (pf_async_vec > 0)
    {
        irq_range.start = 0;
        irq_range.size = pf_async_vec;
        pf_irq_table->pf_async_pool = irq_pool_alloc(dev, irq_range.start, irq_range.size, "zxdh_pf_async",
                                                     ZXDH_PF_ASYNC_IRQ_MIN_COMP,
                                                     ZXDH_PF_ASYNC_IRQ_MAX_COMP);
        if (IS_ERR_OR_NULL(pf_irq_table->pf_async_pool))
        {
            LOG_ERR_DEV(dev, "pf_irq_table->pf_async_pool irq_pool_alloc failed\n");
            err = PTR_ERR(pf_irq_table->pf_async_pool);
            goto err_irq_async_pool;
        }
    }

    irq_range.start = ZXDH_RDMA_IRQ_START_IDX;
    if (dev->coredev_type == DH_COREDEV_VF)
    {
        irq_range.start = ZXDH_VF_RDMA_IRQ_START_IDX;
    }
    irq_range.size = ZXDH_RDMA_CHANNELS_NUM;
    pf_irq_table->pf_rdma_pool = irq_pool_alloc(dev, irq_range.start, irq_range.size, "zxdh_pf_rdma",
                                                ZXDH_PF_RDMA_IRQ_MIN,
                                                ZXDH_PF_RDMA_IRQ_MAX);
    if (IS_ERR_OR_NULL(pf_irq_table->pf_rdma_pool))
    {
        LOG_ERR_DEV(dev, "pf_irq_table->pf_rdma_pool irq_pool_alloc failed\n");
        err = PTR_ERR(pf_irq_table->pf_rdma_pool);
        goto err_irq_rdma_pool;
    }

    return 0;

err_irq_rdma_pool:
    if (pf_async_vec > 0)
    {
        irq_pool_free(pf_irq_table->pf_async_pool);
    }
err_irq_async_pool:
err_irqs_per_cpu:
    if(vq_n > 0)
    {
        irq_pool_free(pf_irq_table->pf_vq_pool);
    }
    return err;
}

static void irq_pools_destroy(struct dh_irq_table *table)
{
    struct dh_pf_irq_table* pf_irq_table = NULL;

    pf_irq_table = (struct dh_pf_irq_table*)table->priv;
    pf_irq_table->pf_vq_pool ? irq_pool_free(pf_irq_table->pf_vq_pool) : 0;
    pf_irq_table->pf_async_pool ? irq_pool_free(pf_irq_table->pf_async_pool) : 0;
    pf_irq_table->pf_rdma_pool ? irq_pool_free(pf_irq_table->pf_rdma_pool) : 0;
}

static int32_t zxdh_get_total_vec(struct dh_core_dev *dev)
{
    if (dev->coredev_type == DH_COREDEV_VF)
    {
        return ZXDH_VF_VQS_CHANNELS_NUM + ZXDH_VF_ASYNC_CHANNELS_NUM + ZXDH_RDMA_CHANNELS_NUM;
    }
    return ZXDH_VQS_CHANNELS_NUM + ZXDH_ASYNC_CHANNELS_NUM + ZXDH_RDMA_CHANNELS_NUM;
}

int32_t dh_pf_irq_table_create(struct dh_core_dev *dev)
{
    int32_t total_vec = 0;
    int32_t err = 0;

    total_vec = zxdh_get_total_vec(dev);

    total_vec = pci_alloc_irq_vectors(dev->pdev, total_vec, total_vec, PCI_IRQ_MSIX);
    if (total_vec < 0)
    {
        LOG_ERR_DEV(dev, "pci_alloc_irq_vectors failed: %d\n", total_vec);
        return total_vec;
    }

    if (dev->coredev_type == DH_COREDEV_VF)
    {
        err = irq_pools_init(dev, ZXDH_VF_VQS_CHANNELS_NUM, ZXDH_VF_ASYNC_CHANNELS_NUM);
    }
    else
    {
        err = irq_pools_init(dev, ZXDH_VQS_CHANNELS_NUM, ZXDH_ASYNC_CHANNELS_NUM);
    }
    if (err != 0)
    {
        LOG_ERR_DEV(dev, "irq_pools_init failed: %d\n", err);
        pci_free_irq_vectors(dev->pdev);
    }

    return err;
}

void dh_pf_irq_table_destroy(struct dh_core_dev *dev)
{
    struct dh_irq_table *table = &dev->irq_table;
    struct zxdh_pf_device *pf_dev = dh_core_priv(dev);
    uint32_t i = 0;

    /* There are cases where IRQs still will be in used when we reaching
     * to here. Hence, making sure all the irqs are released.
     */
    irq_pools_destroy(table);

    mutex_lock(&pf_dev->irq_lock);
    for(i = 0; i < ZXDH_RDMA_CHANNELS_NUM; ++i) {
        if (pf_dev->rdma_irqs[i].is_request) {
            pf_dev->rdma_irqs[i].is_request = false;
            zxdh_irq_free(pf_dev->rdma_irqs[i].irq, pf_dev->rdma_irqs[i].data);
            LOG_DEBUG_DEV(dev, "irq[%u] is free\n", pf_dev->rdma_irqs[i].irq);
        }
    }
    mutex_unlock(&pf_dev->irq_lock);

    pci_free_irq_vectors(dev->pdev);
}

struct dh_irq *dh_pf_async_irq_request(struct dh_core_dev *dev)
{
    struct dh_irq_table *table = &dev->irq_table;
    struct dh_pf_irq_table* pf_irq_table;

    pf_irq_table = (struct dh_pf_irq_table*)table->priv;

    return pf_irq_table->pf_async_pool ? zxdh_get_irq_of_pool(dev, pf_irq_table->pf_async_pool) : NULL;
}

/* irq_table API */
int32_t dh_pf_irq_table_init(struct dh_core_dev *dev)
{
    struct dh_irq_table *irq_table;
    struct dh_pf_irq_table* pf_irq_table = NULL;

    irq_table = &dev->irq_table;

    pf_irq_table = kvzalloc(sizeof(*pf_irq_table), GFP_KERNEL);
    if (unlikely(pf_irq_table == NULL))
    {
        LOG_ERR_DEV(dev, "pf_irq_table kvzalloc failed\n");
        return -ENOMEM;
    }

    irq_table->priv = pf_irq_table;

    return 0;
}
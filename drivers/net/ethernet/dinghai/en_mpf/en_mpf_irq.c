#include <linux/pci.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/helper.h>
#include "en_mpf_irq.h"

#define ZXDH_MPF_ASYNC_IRQ_MIN_COMP 0
#define ZXDH_MPF_ASYNC_IRQ_MAX_COMP 1

#define ZXDH_MPF_COMP_IRQ_MIN_COMP 0
#define ZXDH_MPF_COMP_IRQ_MAX_COMP 1

#ifndef CONFIG_DINGHAI_ZF_MPF
#define ZXDH_MPF_ASYNC_IRQ_NUM 2
#else
#define ZXDH_MPF_ASYNC_IRQ_NUM 6
#endif

struct dh_mpf_irq_table {
    struct dh_irq_pool *mpf_comp_pool;
    struct dh_irq_pool *mpf_async_pool;
};

struct dh_irq_range {
    int32_t start;
    int32_t size;
};

static struct dh_irq_range zxdh_get_mpf_range(struct dh_core_dev *dev)
{
    struct dh_irq_range tmp = {
        .start = 0,
        .size = ZXDH_MPF_ASYNC_IRQ_NUM
    };

    return tmp;
}
static struct dh_irq_range zxdh_get_comp_mpf_range(struct dh_core_dev *dev)
{
    struct dh_irq_range tmp = {
        .start = ZXDH_MPF_ASYNC_IRQ_NUM + 1,
        .size = ZXDH_MPF_ASYNC_IRQ_NUM + 1
    };

    return tmp;
}

static int32_t irq_pools_init(struct dh_core_dev *dev)
{
    struct dh_irq_table *table              = &dev->irq_table;
    int32_t err                             = 0;
    struct dh_irq_range irq_range;
    struct dh_mpf_irq_table * mpf_irq_table = table->priv;

    /* init mpf_pool */
    irq_range = zxdh_get_mpf_range(dev);

    mpf_irq_table->mpf_async_pool = irq_pool_alloc(dev, irq_range.start, irq_range.size, "zxdh_mpf_msg",
                                                   ZXDH_MPF_ASYNC_IRQ_MIN_COMP,
                                                   ZXDH_MPF_ASYNC_IRQ_MAX_COMP);
    if (IS_ERR_OR_NULL(mpf_irq_table->mpf_async_pool))
    {
        return PTR_ERR(mpf_irq_table->mpf_async_pool);
    }

    /* init sf_comp_pool */
    irq_range = zxdh_get_comp_mpf_range(dev);

    mpf_irq_table->mpf_comp_pool = irq_pool_alloc(dev, irq_range.start,
                                                  irq_range.size, "zxdh_mpf_comp",
                                                  ZXDH_MPF_COMP_IRQ_MIN_COMP,
                                                  ZXDH_MPF_COMP_IRQ_MAX_COMP);
    if (IS_ERR_OR_NULL(mpf_irq_table->mpf_comp_pool))
    {
        err = PTR_ERR(mpf_irq_table->mpf_comp_pool);
        goto err_mpf_comp;
    }

    mpf_irq_table->mpf_comp_pool->irqs_per_cpu = kcalloc(nr_cpu_ids, sizeof(u16), GFP_KERNEL);
    if (unlikely(mpf_irq_table->mpf_comp_pool->irqs_per_cpu == NULL))
    {
        err = -ENOMEM;
        goto err_irqs_per_cpu;
    }

    return 0;

err_irqs_per_cpu:
    irq_pool_free(mpf_irq_table->mpf_comp_pool);
err_mpf_comp:
    irq_pool_free(mpf_irq_table->mpf_async_pool);
    return err;
}

static void irq_pools_destroy(struct dh_irq_table *table)
{
    struct dh_mpf_irq_table *mpf_irq_table = (struct dh_mpf_irq_table *)table->priv;

    irq_pool_free(mpf_irq_table->mpf_comp_pool);
    irq_pool_free(mpf_irq_table->mpf_async_pool);
}

/*todo*/
static int32_t zxdh_get_total_vec(struct dh_core_dev *dev)
{
    return ZXDH_MPF_ASYNC_IRQ_NUM;
}

int32_t dh_mpf_irq_table_create(struct dh_core_dev *dev)
{
    int32_t total_vec = 0;
    int32_t err       = 0;

    total_vec = zxdh_get_total_vec(dev);

    total_vec = pci_alloc_irq_vectors(dev->pdev, total_vec, total_vec, PCI_IRQ_MSIX);
    if (total_vec < 0)
    {
        dh_err(dev, "pci_alloc_irq_vectors failed: %d\n", total_vec);
        return total_vec;
    }

    err = irq_pools_init(dev);
    if (err != 0)
    {
        pci_free_irq_vectors(dev->pdev);
    }

    return err;
}

void dh_mpf_irq_table_destroy(struct dh_core_dev *dev)
{
    struct dh_irq_table *table = &dev->irq_table;

    /* There are cases where IRQs still will be in used when we reaching
     * to here. Hence, making sure all the irqs are released.
     */
    irq_pools_destroy(table);
    pci_free_irq_vectors(dev->pdev);
}

struct dh_irq *dh_mpf_async_irq_request(struct dh_core_dev *dev)
{
    struct dh_irq_table *table             = &dev->irq_table;
    struct dh_mpf_irq_table *mpf_irq_table = (struct dh_mpf_irq_table *)table->priv;

    struct dh_irq *irq = zxdh_get_irq_of_pool(dev, mpf_irq_table->mpf_async_pool);
    if (IS_ERR_OR_NULL(irq))
        dh_err(dev, "irq=0x%llx\r\n", (unsigned long long)irq);
    dh_dbg(dev, "end\r\n");
    return irq;
}

/* irq_table API */
int32_t dh_mpf_irq_table_init(struct dh_core_dev *dev)
{
    struct dh_irq_table *irq_table;
    struct dh_mpf_irq_table * mpf_irq_table = NULL;

    irq_table = &dev->irq_table;

    mpf_irq_table = kvzalloc(sizeof(*mpf_irq_table), GFP_KERNEL);
    if (unlikely(mpf_irq_table == NULL))
    {
        return -ENOMEM;
    }

    irq_table->priv = mpf_irq_table;

    return 0;
}

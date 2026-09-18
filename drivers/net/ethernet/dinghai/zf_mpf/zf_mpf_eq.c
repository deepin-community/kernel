#include <linux/dinghai/eq.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/dinghai/helper.h>
#include <linux/dinghai/log.h>
#include "zf_mpf_irq.h"
#include "zf_mpf_eq.h"
#include "zf_mpf.h"
#include "gdma.h"

struct dh_mpf_eq_table {
    struct dh_irq      **comp_irqs;
    struct dh_irq      *async_risc_irq;
    struct dh_irq      *async_pf_irq;
    struct dh_irq      *async_irq1;
    struct dh_irq      *async_irq2;
    struct dh_irq      *async_irq3;
    struct dh_irq      *async_irq4;
    struct dh_irq      **gdma_irqs;
    struct dh_eq_async async_risc_eq;
    struct dh_eq_async async_pf_eq;
    struct dh_eq_async async_eq1;
    struct dh_eq_async async_eq2;
    struct dh_eq_async async_eq3;
    struct dh_eq_async async_eq4;
    struct dh_eq_async gdma_eq[ZXDH_MPF_GDMA_IRQ_NUM];
};

static int32_t create_async_eqs(struct dh_core_dev *dev);

#ifdef CONFIG_ZF_GDMA
void cleanup_gdma_eq(struct dh_core_dev *dev, struct dh_mpf_eq_table *table_priv, uint16_t num);
#endif

static int32_t __attribute__((unused)) create_eq_map(struct dh_eq_param *param)
{
    int32_t err = 0;

    /* inform device*/
    return err;
}

int32_t dh_mpf_eq_table_init(struct dh_core_dev *dev)
{
    struct dh_mpf_eq_table *table_priv = NULL;

    table_priv = kvzalloc(sizeof(*table_priv), GFP_KERNEL);
    if (unlikely(table_priv == NULL))
    {
        return -ENOMEM;
    }

    dh_eq_table_init(dev, table_priv);

    return 0;
}

/*todo*/
int32_t dh_eq_get_comp_eqs(struct dh_core_dev *dev)
{
    return 0;
}

static int32_t create_comp_eqs(struct dh_core_dev *dev)
{
    if (IS_ERR_OR_NULL(dev))
    {
        DH_LOG_ERR(MODULE_MPF, "error dev\n");
        return PTR_ERR(dev);
    }

    return 0;
}

static int32_t destroy_async_eq(struct dh_core_dev *dev)
{
    struct dh_eq_table *eq_table = &dev->eq_table;

    mutex_lock(&eq_table->lock);
    /*unmap inform device*/
    mutex_unlock(&eq_table->lock);

    return 0;
}

static void cleanup_async_eq(struct dh_core_dev *dev,
                             struct dh_eq_async *eq, const char *name)
{
    dh_eq_disable(dev, &eq->core, &eq->irq_nb);
}

static void destroy_async_eqs(struct dh_core_dev *dev)
{
    struct dh_eq_table *table          = &dev->eq_table;
    struct dh_mpf_eq_table *table_priv = table->priv;

    cleanup_async_eq(dev, &table_priv->async_risc_eq, "riscv");
    cleanup_async_eq(dev, &table_priv->async_pf_eq, "pf");
    cleanup_async_eq(dev, &table_priv->async_eq1, "eq1");
    cleanup_async_eq(dev, &table_priv->async_eq2, "eq2");
    cleanup_async_eq(dev, &table_priv->async_eq3, "eq3");
    cleanup_async_eq(dev, &table_priv->async_eq4, "eq4");
    destroy_async_eq(dev);
    dh_irqs_release_vectors(&table_priv->async_risc_irq, 1);
    dh_irqs_release_vectors(&table_priv->async_pf_irq, 1);
    dh_irqs_release_vectors(&table_priv->async_irq1, 1);
    dh_irqs_release_vectors(&table_priv->async_irq2, 1);
    dh_irqs_release_vectors(&table_priv->async_irq3, 1);
    dh_irqs_release_vectors(&table_priv->async_irq4, 1);
}

void destroy_comp_eqs(struct dh_core_dev *dev)
{

}

#ifdef CONFIG_ZF_GDMA
void destroy_gdma_eqs(struct dh_core_dev *dev)
{
    struct dh_eq_table *eq_table = &dev->eq_table;
    struct dh_mpf_eq_table *mpf_eq_table = eq_table->priv;
    struct dh_irq_table *irq_table = &dev->irq_table;
    struct dh_mpf_irq_table *mpf_irq_table = irq_table->priv;

    cleanup_gdma_eq(dev, mpf_eq_table, ZXDH_MPF_GDMA_IRQ_NUM);

    dh_irq_affinity_irqs_release(mpf_irq_table->mpf_gdma_pool, mpf_eq_table->gdma_irqs, ZXDH_MPF_GDMA_IRQ_NUM);
    dh_irqs_release_vectors(mpf_eq_table->gdma_irqs, ZXDH_MPF_GDMA_IRQ_NUM);
}
#endif

void dh_mpf_eq_table_destroy(struct dh_core_dev *dev)
{
    destroy_comp_eqs(dev);
    destroy_async_eqs(dev);
}

int32_t dh_mpf_eq_table_create(struct dh_core_dev *dev)
{
    int32_t err = 0;

    err = create_async_eqs(dev);
    if (err != 0)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to create async EQs\n");
        goto err_async_eqs;
    }

    err = create_comp_eqs(dev);
    if (err != 0)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to create completion EQs\n");
        goto err_comp_eqs;
    }

    return 0;

err_comp_eqs:
    destroy_async_eqs(dev);
err_async_eqs:
    return err;
}

/*create eventq*/
static int32_t create_async_eq(struct dh_core_dev *dev, struct dh_irq *risc, struct dh_irq *pf)
{
    struct dh_eq_table *eq_table = &dev->eq_table;
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dev);
    struct msix_para in = {0};
    int32_t err = 0;

    in.vector_risc = risc->index;
    in.vector_pfvf = pf->index;
    in.vector_mpf = 0xff;
    in.driver_type = MSG_CHAN_END_PF;//TODO
    in.pdev = dev->pdev;
    in.virt_addr = mpf_dev->pci_ioremap_addr + ZXDH_BAR1_CHAN_OFFSET;
    in.pcie_id = mpf_dev->pcie_id;
    DH_LOG_INFO_DEV(MODULE_MPF, dev, "%s pcie_id = 0x%x\n", __func__, mpf_dev->pcie_id);

    mutex_lock(&eq_table->lock);

    err = zxdh_bar_enable_chan(&in, &mpf_dev->vport);

    mutex_unlock(&eq_table->lock);

    return err;
}

static int32_t dh_eq_async_riscv_int(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async *eq_riscv_async = container_of(nb, struct dh_eq_async, irq_nb);
    struct dh_core_dev *dev = (struct dh_core_dev *)eq_riscv_async->priv;
    struct dh_eq_table *eq_table = &dev->eq_table;

    atomic_notifier_call_chain(&eq_table->nh[DH_EVENT_TYPE_NOTIFY_RISC_TO_MPF], DH_EVENT_TYPE_NOTIFY_RISC_TO_MPF, NULL);

    return 0;
}

static int32_t dh_eq_async_mpf_int(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async *eq_riscv_async = container_of(nb, struct dh_eq_async, irq_nb);
    struct dh_core_dev *dev = (struct dh_core_dev *)eq_riscv_async->priv;
    struct dh_eq_table *eq_table = &dev->eq_table;

    atomic_notifier_call_chain(&eq_table->nh[DH_EVENT_TYPE_NOTIFY_PF_TO_MPF], DH_EVENT_TYPE_NOTIFY_PF_TO_MPF, NULL);

    return 0;
}

static int32_t dh_eq_async_int1(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async *eq_riscv_async = container_of(nb, struct dh_eq_async, irq_nb);
    struct dh_core_dev *dev = (struct dh_core_dev *)eq_riscv_async->priv;
    struct dh_eq_table *eq_table = &dev->eq_table;

    atomic_notifier_call_chain(&eq_table->nh[DH_EVENT_TYPE_NOTIFY_1], DH_EVENT_TYPE_NOTIFY_1, dev);

    return 0;
}

static int32_t dh_eq_async_int2(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async *eq_riscv_async = container_of(nb, struct dh_eq_async, irq_nb);
    struct dh_core_dev *dev = (struct dh_core_dev *)eq_riscv_async->priv;
    struct dh_eq_table *eq_table = &dev->eq_table;

    atomic_notifier_call_chain(&eq_table->nh[DH_EVENT_TYPE_NOTIFY_2], DH_EVENT_TYPE_NOTIFY_2, dev);

    return 0;
}

static int32_t dh_eq_async_int3(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async *eq_riscv_async = container_of(nb, struct dh_eq_async, irq_nb);
    struct dh_core_dev *dev = (struct dh_core_dev *)eq_riscv_async->priv;
    struct dh_eq_table *eq_table = &dev->eq_table;

    atomic_notifier_call_chain(&eq_table->nh[DH_EVENT_TYPE_NOTIFY_3], DH_EVENT_TYPE_NOTIFY_3, dev);

    return 0;
}

static int32_t dh_eq_async_int4(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async *eq_riscv_async = container_of(nb, struct dh_eq_async, irq_nb);
    struct dh_core_dev *dev = (struct dh_core_dev *)eq_riscv_async->priv;
    struct dh_eq_table *eq_table = &dev->eq_table;

    atomic_notifier_call_chain(&eq_table->nh[DH_EVENT_TYPE_NOTIFY_4], DH_EVENT_TYPE_NOTIFY_4, dev);

    return 0;
}

static int32_t create_async_eqs(struct dh_core_dev *dev)
{
    struct dh_eq_table *table          = &dev->eq_table;
    struct dh_mpf_eq_table *table_priv = table->priv;
    struct dh_eq_param param           = {};
    int32_t err                        = 0;

    DH_LOG_DEBUG_DEV(MODULE_MPF, dev, "start\r\n");
    table_priv->async_risc_irq = dh_mpf_async_irq_request(dev);
    if (IS_ERR_OR_NULL(table_priv->async_risc_irq))
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to get async_risc_irq\n");
        return PTR_ERR(table_priv->async_risc_irq);
    }

    table_priv->async_pf_irq = dh_mpf_async_irq_request(dev);
    if (IS_ERR_OR_NULL(table_priv->async_pf_irq))
    {
        err = PTR_ERR(table_priv->async_pf_irq);
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to get async_pf_irq\n");
        goto err_irq_request;
    }

    table_priv->async_irq1 = dh_mpf_async_irq_request(dev);
    if (IS_ERR_OR_NULL(table_priv->async_irq1))
    {
        err = PTR_ERR(table_priv->async_irq1);
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to get async_irq1\n");
        goto err_irq_request1;
    }

    table_priv->async_irq2 = dh_mpf_async_irq_request(dev);
    if (IS_ERR_OR_NULL(table_priv->async_irq2))
    {
        err = PTR_ERR(table_priv->async_irq2);
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to get async_irq2\n");
        goto err_irq_request2;
    }

    table_priv->async_irq3 = dh_mpf_async_irq_request(dev);
    if (IS_ERR_OR_NULL(table_priv->async_irq3))
    {
        err = PTR_ERR(table_priv->async_irq3);
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to get async_irq3\n");
        goto err_irq_request3;
    }

    table_priv->async_irq4 = dh_mpf_async_irq_request(dev);
    if (IS_ERR_OR_NULL(table_priv->async_irq4))
    {
        err = PTR_ERR(table_priv->async_irq4);
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to get async_irq4\n");
        goto err_irq_request4;
    }

    err = create_async_eq(dev, table_priv->async_risc_irq, table_priv->async_pf_irq);
    if (err != 0)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to create async_eq\n");
        goto err_create_async_eq;
    }

    param = (struct dh_eq_param) {
        .irq = table_priv->async_risc_irq,
        .nent = 10,
        .event_type = DH_EVENT_QUEUE_TYPE_RISCV /* used for inform dpu */
    };
    err = setup_async_eq(dev, &table_priv->async_risc_eq, &param, dh_eq_async_riscv_int, "riscv", dev);
    if (err != 0)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to setup async_risc_eq\n");
        goto err_setup_async_risc_eq;
    }

    param.irq = table_priv->async_pf_irq,
    err = setup_async_eq(dev, &table_priv->async_pf_eq, &param, dh_eq_async_mpf_int, "pf", dev);
    if (err != 0)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to setup async_pf_eq\n");
        goto err_setup_async_pf_eq;
    }

    param.irq = table_priv->async_irq1,
    err = setup_async_eq(dev, &table_priv->async_eq1, &param, dh_eq_async_int1, "irq1", dev);
    if (err != 0)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to setup async_eq1\n");
        goto err_setup_async_eq1;
    }

    param.irq = table_priv->async_irq2,
    err = setup_async_eq(dev, &table_priv->async_eq2, &param, dh_eq_async_int2, "irq2", dev);
    if (err != 0)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to setup async_eq1\n");
        goto err_setup_async_eq2;
    }

    param.irq = table_priv->async_irq3,
    err = setup_async_eq(dev, &table_priv->async_eq3, &param, dh_eq_async_int3, "irq3", dev);
    if (err != 0)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to setup async_eq3\n");
        goto err_setup_async_eq3;
    }

    param.irq = table_priv->async_irq4,
    err = setup_async_eq(dev, &table_priv->async_eq4, &param, dh_eq_async_int4, "irq4", dev);
    if (err != 0)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to setup async_eq4\n");
        goto err_setup_async_eq4;
    }

    return 0;

err_setup_async_eq4:
    cleanup_async_eq(dev, &table_priv->async_eq3, "irq3");
err_setup_async_eq3:
    cleanup_async_eq(dev, &table_priv->async_eq2, "irq2");
err_setup_async_eq2:
    cleanup_async_eq(dev, &table_priv->async_eq1, "irq1");
err_setup_async_eq1:
    cleanup_async_eq(dev, &table_priv->async_pf_eq, "pf");
err_setup_async_pf_eq:
    cleanup_async_eq(dev, &table_priv->async_risc_eq, "riscv");
err_setup_async_risc_eq:
    destroy_async_eq(dev);
err_create_async_eq:
    dh_irqs_release_vectors(&table_priv->async_irq4, 1);
err_irq_request4:
    dh_irqs_release_vectors(&table_priv->async_irq3, 1);
err_irq_request3:
    dh_irqs_release_vectors(&table_priv->async_irq2, 1);
err_irq_request2:
    dh_irqs_release_vectors(&table_priv->async_irq1, 1);
err_irq_request1:
    dh_irqs_release_vectors(&table_priv->async_pf_irq, 1);
err_irq_request:
    dh_irqs_release_vectors(&table_priv->async_risc_irq, 1);
    return err;
}

#ifdef CONFIG_ZF_GDMA
void cleanup_gdma_eq(struct dh_core_dev *dev, struct dh_mpf_eq_table *table_priv, uint16_t num)
{
    uint16_t i = 0;

    for (i = 0; i < num; i++)
    {
        cleanup_async_eq(dev, &table_priv->gdma_eq[i], NULL);
    }
}

int32_t setup_gdma_eq(struct dh_core_dev *dev, struct dh_mpf_eq_table *table_priv, uint16_t gdma_irq_num)
{
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dev);
    struct zf_gdma_dev *gdev = mpf_dev->gdev;
    struct dh_eq_param param = {};
    uint16_t i = 0;
    int32_t ret = 0;
    void *priv = NULL;
    notifier_fn_t callback;

    if (gdma_irq_num > ZXDH_MPF_GDMA_IRQ_NUM)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "gdma_irq_num %d is invalid\n", gdma_irq_num);
        return -1;
    }

    for (i = 0; i < gdma_irq_num; i++)
    {
        if (i < ZF_GDMA_CHAN_NUM)
        {
            callback = zf_gdma_chan_irq_handle;
            priv = &gdev->chan[i];
        }
        else
        {
            callback = zf_gdma_err_irq_handle;
            priv = dev;
        }

        param.irq = table_priv->gdma_irqs[i];
        ret = setup_async_eq(dev, &table_priv->gdma_eq[i], &param, callback, "gdma", priv);
        if (ret != 0)
        {
            DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to setup gdma %d eq\n", i);
            cleanup_gdma_eq(dev, table_priv, i);
            return ret;
        }
    }

    return 0;
}

int32_t create_gdma_eqs(struct dh_core_dev *dev)
{
    struct dh_eq_table *eq_table = &dev->eq_table;
    struct dh_mpf_eq_table *mpf_eq_table = eq_table->priv;
    struct dh_irq_table *irq_table = &dev->irq_table;
    struct dh_mpf_irq_table *mpf_irq_table = irq_table->priv;
    int32_t ret = 0;
    int numa = dev_to_node(dev->device);

    mpf_eq_table->gdma_irqs = kcalloc(ZXDH_MPF_GDMA_IRQ_NUM, sizeof(struct dh_irq *), GFP_KERNEL);
    if (unlikely(mpf_eq_table->gdma_irqs == NULL))
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to alloc mpf_eq_table->gdma_irqs\n");
        return -ENOMEM;
    }

    ret = dh_irq_affinity_irqs_request_auto(mpf_irq_table->mpf_gdma_pool, mpf_eq_table->gdma_irqs, ZXDH_MPF_GDMA_IRQ_NUM, 2, numa);
    if (ret < 0)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "Failed to get gdma irq\n");
        goto err_gdma_irq;
    }

    ret = setup_gdma_eq(dev, mpf_eq_table, ZXDH_MPF_GDMA_IRQ_NUM);
    if (ret != 0)
    {
        goto err_gdma_eq;
    }

    return 0;

err_gdma_eq:
    dh_irq_affinity_irqs_release(mpf_irq_table->mpf_gdma_pool, mpf_eq_table->gdma_irqs, ZXDH_MPF_GDMA_IRQ_NUM);
    dh_irqs_release_vectors(mpf_eq_table->gdma_irqs, ZXDH_MPF_GDMA_IRQ_NUM);
err_gdma_irq:
    kfree(mpf_eq_table->gdma_irqs);
    return ret;
}
#endif
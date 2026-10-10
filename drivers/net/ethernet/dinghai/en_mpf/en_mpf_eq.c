#include <linux/dinghai/eq.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/dinghai/helper.h>
#include "en_mpf_irq.h"
#include "en_mpf_eq.h"
#include "../en_mpf.h"

struct dh_mpf_eq_table {
    struct dh_irq      **comp_irqs;
    struct dh_irq      *async_risc_irq;
    struct dh_irq      *async_pf_irq;
    struct dh_eq_async async_risc_eq;
    struct dh_eq_async async_pf_eq;
};

static int32_t create_async_eqs(struct dh_core_dev *dev);

static int32_t __attribute__((unused)) create_eq_map(struct dh_eq_param *param)
{
    int32_t err = 0;

    /* inform device*/
    return err;
}

int32_t dh_mpf_eq_table_init(struct dh_core_dev *dev)
{
    struct dh_eq_table *eq_table;
    struct dh_mpf_eq_table *table_priv = NULL;
    int32_t err                        = 0;

    eq_table = &dev->eq_table;

    table_priv = kvzalloc(sizeof(*table_priv), GFP_KERNEL);  
    if (unlikely(table_priv == NULL))
    {
        err = -ENOMEM;
        goto err_table_priv;
    }

    dh_eq_table_init(dev, table_priv);

    return 0;

err_table_priv:
    kvfree(eq_table);
    return err;
}

/*todo*/
int32_t dh_eq_get_comp_eqs(struct dh_core_dev *dev)
{
    return 0;
}

static int32_t create_comp_eqs(struct dh_core_dev *dev)
{
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
    destroy_async_eq(dev);
    dh_irqs_release_vectors(&table_priv->async_risc_irq, 1);
    dh_irqs_release_vectors(&table_priv->async_pf_irq, 1);
}

void destroy_comp_eqs(struct dh_core_dev *dev)
{

}

void dh_mpf_eq_table_destroy(struct dh_core_dev *dev)
{
    destroy_comp_eqs(dev);
    destroy_async_eqs(dev);
}

int32_t dh_mpf_eq_table_create(struct dh_core_dev *dev)
{
    int32_t err                  = 0;

    err = create_async_eqs(dev);
    if (err != 0)
    {
        dh_err(dev, "Failed to create async EQs\n");
        goto err_async_eqs;
    }

    err = create_comp_eqs(dev);
    if (err != 0)
    {
        dh_err(dev, "Failed to create completion EQs\n");
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

static int32_t create_async_eqs(struct dh_core_dev *dev)
{
    struct dh_eq_table *table          = &dev->eq_table;
    struct dh_mpf_eq_table *table_priv = table->priv;
    struct dh_eq_param param           = {};
    int32_t err                        = 0;

    dh_dbg(dev, "start\r\n");
    table_priv->async_risc_irq = dh_mpf_async_irq_request(dev);
    if (IS_ERR_OR_NULL(table_priv->async_risc_irq))
    {
        dh_err(dev, "Failed to get async_risc_irq\n");
        return PTR_ERR(table_priv->async_risc_irq);
    }

    table_priv->async_pf_irq = dh_mpf_async_irq_request(dev);
    if (IS_ERR_OR_NULL(table_priv->async_pf_irq))
    {
        err = PTR_ERR(table_priv->async_pf_irq);
        dh_err(dev, "Failed to get async_pf_irq\n");
        goto err_irq_request;
    }

    err = create_async_eq(dev, table_priv->async_risc_irq, table_priv->async_pf_irq);
    if (err != 0)
    {
        dh_err(dev, "Failed to create async_eq\n");
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
        dh_err(dev, "Failed to setup async_risc_eq\n");
        goto err_setup_async_eq;
    }

    param.irq = table_priv->async_pf_irq,
    err = setup_async_eq(dev, &table_priv->async_pf_eq, &param, dh_eq_async_mpf_int, "pf", dev);
    if (err != 0)
    {
        dh_err(dev, "Failed to setup async_pf_eq\n");
        goto cleanup_async_eq;
    }

    return 0;

cleanup_async_eq:
    cleanup_async_eq(dev, &table_priv->async_risc_eq, "riscv");
err_setup_async_eq:
    destroy_async_eq(dev);
err_create_async_eq:
    dh_irqs_release_vectors(&table_priv->async_pf_irq, 1);
err_irq_request:
    dh_irqs_release_vectors(&table_priv->async_risc_irq, 1);
    return err;
}
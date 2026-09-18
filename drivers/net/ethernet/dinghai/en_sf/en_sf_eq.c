#include <linux/dinghai/eq.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/helper.h>
#include <linux/list.h>
#include <linux/dinghai/en_sf.h>
#include <linux/dinghai/queue.h>
#include "en_sf_irq.h"
#include "en_sf_eq.h"
#include "../en_sf.h"


static int32_t create_async_eqs(struct dh_core_dev *dev)
{
    return 0;
}

int32_t dh_en_sf_eq_table_create(struct dh_core_dev *dev, struct zxdh_en_sf_if *ops)
{
    int32_t err;

    err = create_async_eqs(dev);

    return err;
}

void dh_sf_eq_table_destroy(struct dh_core_dev *dev)
{
    return;
}

void zxdh_set_queue_size(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, uint16_t size)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    en_sf_dev->sf_ops->en_sf_set_queue_size(dh_dev->parent, phy_index, index, size);
}

void zxdh_queue_address(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index,
                        uint64_t desc_addr, uint64_t driver_addr, uint64_t device_addr)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    en_sf_dev->sf_ops->en_sf_set_queue_address(dh_dev->parent, phy_index, index, desc_addr, driver_addr, device_addr);
}

void zxdh_en_sf_activate_phy_vq(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index,
                                int32_t queue_size, uint64_t desc_addr, uint64_t avail_addr, uint64_t used_addr)
{
    zxdh_set_queue_size(dh_dev, phy_index, index, queue_size);
    zxdh_queue_address(dh_dev, phy_index, index, desc_addr, avail_addr, used_addr);
}

int32_t dh_en_sf_eq_table_init(struct dh_core_dev *dev)
{
    struct dh_eq_table *eq_table = &dev->eq_table;
    struct dh_en_sf_eq_table *table_priv = NULL;
    int32_t err = 0;

    table_priv = kvzalloc(sizeof(*table_priv), GFP_KERNEL);
    if (unlikely(table_priv == NULL))
    {
        LOG_ERR_DEV(dev, "dh_en_sf_eq_table kvzalloc failed\n");
        err = -ENOMEM;
        goto err_table_priv;
    }
    dh_eq_table_init(dev, table_priv);

    return 0;

err_table_priv:
    kvfree(eq_table);
    return err;
}

static void vqs_irqs_release(struct dh_core_dev *dh_dev)
{
    struct dh_eq_table *table = &dh_dev->eq_table;
    struct dh_en_sf_eq_table *sf_eq_table = table->priv;
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);
    int32_t vqs_channel_num = 0;

    vqs_channel_num = zxdh_en_sf_get_vqs_channels_num(dh_dev);

    en_sf_dev->sf_ops->en_sf_affinity_irqs_release(dh_dev->parent, sf_eq_table->vq_irqs, vqs_channel_num);

    dh_irqs_release_vectors(sf_eq_table->vq_irqs, sf_eq_table->vq_irq_num);
}

static void clean_vqs_eqs(struct dh_core_dev *dh_dev)
{
    struct dh_eq_table *table = &dh_dev->eq_table;
    struct dh_en_sf_eq_table *sf_eq_table = table->priv;
    struct dh_eq_vqs *eq;
    struct dh_eq_vqs *n;

    list_for_each_entry_safe(eq, n, &sf_eq_table->vqs_eqs_list, list)
    {
        list_del(&eq->list);
        kfree(eq);
    }
}

static void destroy_vqs_eqs(struct dh_core_dev *dh_dev, int32_t vqs_channel_num)
{
    struct dh_eq_table *table = &dh_dev->eq_table;
    struct dh_en_sf_eq_table *sf_eq_table = table->priv;
    struct dh_eq_vqs *eq;
    struct dh_eq_vqs *n;
    int32_t i = 0;

    list_for_each_entry_safe(eq, n, &sf_eq_table->vqs_eqs_list, list)
    {
        if (i <= vqs_channel_num)
        {
            dh_eq_disable(dh_dev, &eq->vq_s.core, &eq->vq_s.irq_nb);
        }
        i++;
    }
}

uint16_t zxdh_en_sf_get_vqs_channels_num(struct dh_core_dev *dh_dev)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);
    uint16_t channels_num = 0;
    uint8_t queue_pairs_num = 0;

    channels_num = en_sf_dev->sf_ops->en_sf_get_channels_num(dh_dev->parent);
    queue_pairs_num = en_sf_dev->sf_ops->en_sf_get_queue_pairs(dh_dev->parent);
    if (channels_num > queue_pairs_num)
    {
        channels_num = queue_pairs_num;
    }
    return channels_num;
}

static int32_t create_map_eq(struct dh_core_dev *dev, struct dh_eq *eq, struct dh_eq_param *param)
{
    eq->irq = param->irq;

    return 0;
}

void zxdh_en_sf_destroy_vqs_channels(struct dh_core_dev *dh_dev)
{
    struct dh_eq_table *table = &dh_dev->eq_table;
    struct dh_en_sf_eq_table *sf_eq_table = table->priv;

    clean_vqs_eqs(dh_dev);
    vqs_irqs_release(dh_dev);
    kfree(sf_eq_table->vq_irqs);
}

void zxdh_en_sf_switch_vqs_channel(struct dh_core_dev *dh_dev, int32_t channel, int32_t op)
{
    struct dh_eq_table *table = &dh_dev->eq_table;
    struct dh_en_sf_eq_table *sf_eq_table = table->priv;
    struct dh_irq *irq = sf_eq_table->vq_irqs[channel];
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    en_sf_dev->sf_ops->en_sf_switch_irq(dh_dev->parent, irq->irqn, op);
}

int32_t zxdh_en_sf_create_vqs_channels(struct dh_core_dev *dh_dev, void *data)
{
    struct dh_eq_table *table = &dh_dev->eq_table;
    struct dh_en_sf_eq_table *sf_eq_table = table->priv;
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);
    int32_t vqs_channel_num = 0;
    int32_t i = 0;
    struct dh_eq_vqs *eq_vqs = NULL;
    int32_t err = 0;

    vqs_channel_num = zxdh_en_sf_get_vqs_channels_num(dh_dev);

    sf_eq_table->vq_irqs = kcalloc(vqs_channel_num, sizeof(*sf_eq_table->vq_irqs), GFP_KERNEL);
    if (unlikely(sf_eq_table->vq_irqs == NULL))
    {
        LOG_ERR_DEV(dh_dev, "sf_eq_table->vq_irqs kcalloc null\n");
        return -ENOMEM;
    }

    vqs_channel_num = en_sf_dev->sf_ops->en_sf_vq_irqs_request(dh_dev->parent, sf_eq_table->vq_irqs, vqs_channel_num, data);
    if (vqs_channel_num < 0)
    {
        LOG_ERR_DEV(dh_dev, "en_sf_vq_irqs_request failed: %d\n", vqs_channel_num);
        kfree(sf_eq_table->vq_irqs);
        return vqs_channel_num;
    }

    sf_eq_table->vq_irq_num = vqs_channel_num;

    INIT_LIST_HEAD(&sf_eq_table->vqs_eqs_list);

    for (i = 0; i < vqs_channel_num; i++)
    {
        eq_vqs  = kzalloc(sizeof(struct dh_eq_vqs), GFP_KERNEL);
        if (unlikely(eq_vqs == NULL))
        {
            LOG_ERR_DEV(dh_dev, "eq_vqs %d kzalloc null\n", i);
            err = -ENOMEM;
            goto clean;
        }

        INIT_LIST_HEAD(&eq_vqs->vqs);

        list_add_tail(&eq_vqs->list, &sf_eq_table->vqs_eqs_list);
    }

    return vqs_channel_num;

clean:
    zxdh_en_sf_destroy_vqs_channels(dh_dev);
    return err;
}

void zxdh_en_sf_vqs_unbind_eqs(struct dh_core_dev *dh_dev, int32_t vqs_channel_num)
{
    /*struct dh_eq_table *table = &dh_dev->eq_table;
    struct dh_en_sf_eq_table *sf_eq_table = table->priv;
    struct dh_eq_vqs *eq;
    struct dh_eq_vqs *n;
    int32_t i = 0;

    list_for_each_entry_safe(eq, n, &sf_eq_table->vqs_eqs_list, list)
    {
        if (i++ <= vqs_channel_num)
        {
            list_del(&eq->vqs);
        }
    }*/

    return;
}

int32_t zxdh_en_sf_vqs_bind_eqs(struct dh_core_dev *dh_dev, int32_t vqs_channel_num, struct list_head *vq_node)
{
    struct dh_eq_table *table = &dh_dev->eq_table;
    struct dh_en_sf_eq_table *sf_eq_table = table->priv;
    struct dh_eq_vqs *eq;
    struct dh_eq_vqs *n;
    int32_t i = 0;

    list_for_each_entry_safe(eq, n, &sf_eq_table->vqs_eqs_list, list)
    {
        if (i++ == vqs_channel_num)
        {
            list_add_tail(vq_node, &eq->vqs);
            return 0;
        }
    }

    return -ENOENT;
}

void __iomem *zxdh_en_sf_map_vq_notify(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, resource_size_t *pa)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);
    void __iomem *notify_addr = NULL;

    notify_addr = en_sf_dev->sf_ops->en_sf_map_vq_notify(dh_dev->parent, phy_index, index, pa);

    return notify_addr;
}

void zxdh_en_sf_unmap_vq_notify(struct dh_core_dev *dh_dev, void *priv)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    en_sf_dev->sf_ops->en_sf_unmap_vq_notify(dh_dev->parent, priv);
}

void zxdh_en_sf_set_queue_enable(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, bool enable)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    en_sf_dev->sf_ops->en_sf_set_queue_enable(dh_dev->parent, phy_index, index, enable);
}

uint16_t zxdh_en_sf_get_queue_vector(struct dh_core_dev *dh_dev, uint16_t channel, uint16_t phy_index, uint16_t index, uint16_t vq_idx)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);
    struct dh_eq_table *table = &dh_dev->eq_table;
    struct dh_en_sf_eq_table *sf_eq_table = table->priv;
    int32_t msix_vec = ZXDH_MSI_NO_VECTOR;

    msix_vec = en_sf_dev->sf_ops->en_sf_get_queue_vector(dh_dev->parent, channel, &sf_eq_table->vqs_eqs_list, phy_index, index, vq_idx);

    return msix_vec;
}

void zxdh_en_sf_vq_unbind_channel(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    en_sf_dev->sf_ops->en_sf_release_queue_vector(dh_dev->parent, phy_index, index);
}

int32_t zxdh_en_sf_vq_bind_channel(struct dh_core_dev *dh_dev, uint16_t channel_num, uint16_t phy_index, uint16_t index, uint16_t vq_idx)
{
    int32_t msix_vec = ZXDH_MSI_NO_VECTOR;

    msix_vec = zxdh_en_sf_get_queue_vector(dh_dev, channel_num, phy_index, index, vq_idx);

    if (msix_vec == ZXDH_MSI_NO_VECTOR)
    {
        return -EBUSY;
    }

    return msix_vec;
}

void zxdh_en_sf_vqs_channel_unbind_handler(struct dh_core_dev *dh_dev, int32_t vqs_channel_num)
{
    destroy_vqs_eqs(dh_dev, vqs_channel_num);
    return;
}

int32_t zxdh_en_sf_vqs_channel_bind_handler(struct dh_core_dev *dh_dev, int32_t vqs_channel_num, struct dh_vq_handler *handler)
{
    struct dh_eq_table *table = &dh_dev->eq_table;
    struct dh_en_sf_eq_table *sf_eq_table = table->priv;
    int32_t i = 0;
    struct dh_eq_vqs *eq_vqs;
    struct dh_eq_vqs *n;
    int32_t err = 0;

    list_for_each_entry_safe(eq_vqs, n, &sf_eq_table->vqs_eqs_list, list)
    {
        if (i == vqs_channel_num)
        {
            struct dh_eq_param param = {};

            eq_vqs->vq_s.irq_nb.notifier_call = handler->callback;
            eq_vqs->vq_s.para = handler->para;
            param = (struct dh_eq_param) {
                .irq = sf_eq_table->vq_irqs[i],
                .nent = 0,
            };
            create_map_eq(dh_dev, &eq_vqs->vq_s.core, &param);

            err = dh_eq_enable(dh_dev, &eq_vqs->vq_s.core, &eq_vqs->vq_s.irq_nb);
            if(err != 0)
            {
                LOG_ERR_DEV(dh_dev, "dh_eq_enable failed: %d\n", err);
                goto clean_eq;
            }
            return 0;
        }
        i++;
    }

clean_eq:
    destroy_vqs_eqs(dh_dev, vqs_channel_num);
    return err;
}

uint16_t zxdh_en_sf_get_epbdf(struct dh_core_dev *dh_dev)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    return en_sf_dev->sf_ops->en_sf_get_epbdf(dh_dev->parent);
}

uint64_t zxdh_en_sf_get_spec_sbdf(struct dh_core_dev *dh_dev)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    return en_sf_dev->sf_ops->en_sf_get_spec_sbdf(dh_dev->parent);
}

uint16_t zxdh_en_sf_get_vport(struct dh_core_dev *dh_dev)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    return en_sf_dev->sf_ops->en_sf_get_vport(dh_dev->parent);
}

enum dh_coredev_type zxdh_en_sf_get_coredev_type(struct dh_core_dev *dh_dev)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    return en_sf_dev->sf_ops->en_sf_get_coredev_type(dh_dev->parent);
}

uint16_t zxdh_en_sf_get_pcie_id(struct dh_core_dev *dh_dev)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    return en_sf_dev->sf_ops->en_sf_get_pcie_id(dh_dev->parent);
}

uint16_t zxdh_en_sf_get_slot_id(struct dh_core_dev *dh_dev)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    return en_sf_dev->sf_ops->en_sf_get_slot_id(dh_dev->parent);
}

struct zxdh_vf_item *zxdh_en_sf_get_vf_item(struct dh_core_dev *dh_dev, uint16_t vf_idx)
{
    struct zxdh_en_sf_device *en_sf_dev = dh_core_priv(dh_dev);

    return en_sf_dev->sf_ops->en_sf_get_vf_item(dh_dev->parent, vf_idx);
}

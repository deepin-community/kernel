#include <linux/list.h>
#include <linux/dinghai/eq.h>
#include <linux/dinghai/dh_cmd.h>
#include "en_aux_eq.h"
#include "../en_ethtool/ethtool.h"

int32_t dh_bond_pf_link_info_get(struct zxdh_en_priv *en_priv)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t link_up = 0;
    uint8_t link_info = 0;
    uint8_t bit_value = 0;

    if (en_dev == NULL)
    {
        LOG_ERR("null ptr\n");
        return -1;
    }

    if (en_dev->init_comp_flag != AUX_INIT_COMPLETED)
    {
        return 0;
    }

    if(!en_dev->ops->is_bond(en_dev->parent))
    {
        LOG_DEBUG_DEV(en_dev->parent, "isn't bond_pf exit\n");
        return 0;
    }

    //读取state寄存器中第en_dev->link_check_bit位的值
    en_dev->ops->get_link_info_from_vqm(en_dev->parent, &link_info);
    if (link_info == 0xff)
    {
        LOG_DEBUG_DEV(en_dev->parent, "%s link_info is 0xff, exit\n", en_dev->netdev->name);
        return 0;
    }
    bit_value = (link_info >> en_dev->link_check_bit) & 0x01;
    LOG_DEBUG_DEV(en_dev->parent, "%s read 0x%x: link_check_bit[%d]-bit_value[%d]\n",
                en_dev->netdev->name, link_info, en_dev->link_check_bit, bit_value);
    link_up |= bit_value;

    if (en_dev->link_up == link_up)
    {
        LOG_DEBUG_DEV(en_dev->parent, "%s link info is no changed, current link is %d\n",
                en_dev->netdev->name, en_dev->link_up);
        return 0;
    }

    en_dev->link_up = link_up;
    queue_work(en_priv->events->wq, &en_dev->link_info_irq_update_np_work);

    if(link_up == 0)
    {
        netif_carrier_off(en_dev->netdev);
        en_dev->speed = SPEED_UNKNOWN;
        en_dev->duplex = DUPLEX_UNKNOWN;
        en_dev->ops->set_rdma_speed(en_dev->parent, en_dev->speed);
        LOG_INFO_DEV(en_dev->parent, "%s %s is link down\n", __func__, en_dev->netdev->name);
    }
    else
    {
        LOG_INFO_DEV(en_dev->parent, "%s %s is link up\n", __func__, en_dev->netdev->name);
        queue_work(en_priv->events->wq, &en_dev->link_info_irq_process_work);
    }

    return 0;
}

static int32_t dh_eq_async_link_info_int_bond_pf(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async *eq_link_info_async = container_of(nb, struct dh_eq_async, irq_nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)eq_link_info_async->priv;

    return dh_bond_pf_link_info_get(en_priv);
}

int32_t dh_eq_async_link_info_int_process(struct zxdh_en_priv *en_priv)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;
    uint8_t link_up = 0;
    uint8_t link_info = 0;

    //判断是否为bond_pf
    if (en_dev == NULL)
    {
        LOG_ERR("null ptr\n");
        return -1;
    }

    if (en_dev->init_comp_flag != AUX_INIT_COMPLETED)
    {
        return 0;
    }

    if(en_dev->ops->is_bond(en_dev->parent))
    {
        LOG_DEBUG_DEV(en_dev->parent, "is bond_pf, exit\n");
        return 0;
    }

    if ((en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_NE0) ||
        (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_NE1))
    {
        return 0;
    }

    en_dev->ops->get_link_info_from_vqm(en_dev->parent, &link_info);
    if (link_info == 0xff)
    {
        LOG_DEBUG_DEV(en_dev->parent, "%s link_info is 0xff, exit\n", en_dev->netdev->name);
        return 0;
    }

    if(en_dev->ops->is_upf(en_dev->parent)) //是upf设备
    {
        //高四位phoport, 低四位link_up信息
        link_up = link_info & 0x0F;
        LOG_INFO_DEV(en_dev->parent, "[upf dev netdev %s] read [0x%x]: link_up[%d]\n", en_dev->netdev->name, link_info, link_up);
    }
    else
    {
        link_up = link_info;
    }

    en_dev->link_up = link_up;
    queue_work(en_priv->events->wq, &en_dev->link_info_irq_update_np_work);
    if(link_up == 0)
    {
        en_dev->ops->set_pf_link_up(en_dev->parent, FALSE);
        netif_carrier_off(en_dev->netdev);
        en_dev->speed = SPEED_UNKNOWN;
        en_dev->duplex = DUPLEX_UNKNOWN;
        en_dev->ops->set_rdma_speed(en_dev->parent, en_dev->speed);
        LOG_INFO_DEV(en_dev->parent, "%s %s is link down\n", __func__, en_dev->netdev->name);
    }
    else
    {
        en_dev->ops->set_pf_link_up(en_dev->parent, TRUE);
        LOG_INFO_DEV(en_dev->parent, "%s %s is link up\n", __func__, en_dev->netdev->name);
        if(en_dev->ops->is_upf(en_dev->parent))
        {
            netif_carrier_on(en_dev->netdev);
            en_dev->speed = SPEED_200000;
            en_dev->duplex = DUPLEX_FULL;
            en_dev->ops->set_rdma_speed(en_dev->parent, en_dev->speed);
        }
        else
        {
            queue_work(en_priv->events->wq, &en_dev->link_info_irq_process_work);
            if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA) {
                netif_carrier_on(en_dev->netdev);
                en_dev->duplex = DUPLEX_FULL;
                en_dev->speed = SPEED_200000;
                en_dev->ops->set_rdma_speed(en_dev->parent, en_dev->speed);
            }
        }
    }

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        queue_work(en_priv->events->wq, &en_dev->link_info_irq_update_vf_work);
    }

    if (en_dev->ops->get_dev_type(en_dev->parent) == ZXDH_DEV_ROCE_RDMA) {
        uint8_t mac[6] = {0};
        en_dev->ops->get_mac(en_dev->parent, mac);
        LOG_DEBUG_DEV(en_dev->parent, "rdma dev set mac %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
                    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        zxdh_netdev_addr_set(en_dev->netdev, mac);
    }
    return 0;
}

static int32_t dh_eq_async_link_info_int(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async *eq_link_info_async = container_of(nb, struct dh_eq_async, irq_nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)eq_link_info_async->priv;
    return dh_eq_async_link_info_int_process(en_priv);
}

static int32_t dh_eq_async_ro_info_update(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async *eq_link_info_async = container_of(nb, struct dh_eq_async, irq_nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)eq_link_info_async->priv;
    struct zxdh_en_device *en_dev = &en_priv->edev;

    if (en_dev == NULL)
    {
        LOG_ERR("null ptr\n");
        return -1;
    }

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_VF)
    {
        return 0;
    }

    if (en_dev->init_comp_flag != AUX_INIT_COMPLETED)
    {
        return 0;
    }

    if(en_dev->ops->is_bond(en_dev->parent))
    {
        LOG_DEBUG_DEV(en_dev->parent, "is bond_pf exit\n");
        return 0;
    }

    // 1、前往fwshrd读取RO状态, 更新延时标志位
    en_dev->ro_flag = en_dev->ops->get_ro_info_from_fwshrd(en_dev->parent);
    LOG_DEBUG_DEV(en_dev->parent, "netdev %s update ro_flag %d\n", en_dev->netdev->name, en_dev->ro_flag);

    queue_work(en_priv->events->wq, &en_dev->vf_ro_info_update_work);

    return 0;
}

static int32_t dh_eq_async_riscv_int(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async * eq_riscv_async = container_of(nb, struct dh_eq_async, irq_nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)eq_riscv_async->priv;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct dh_events *events = en_priv->events;
    struct dh_event_nb *event_nb = NULL;
    uint64_t virt_addr = 0;
    int32_t event_type = 0;
    uint16_t event_idx = 0;
    uint16_t i = 0;

    virt_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0) + ZXDH_BAR_MSG_OFFSET;
    event_idx = zxdh_get_event_id(virt_addr, MSG_CHAN_END_RISC, MSG_CHAN_END_PF);
    event_type = dh_eq_event_type_get(event_idx);

    if(events == NULL)
    {
        LOG_ERR_DEV(en_dev->parent, "riscv_irq trigger, events is null\n");
        return 0;
    }

    for (i = 0; i < events->evt_num; i++)
    {
        event_nb = &events->notifiers[i];

        if (event_type == event_nb->nb.event_type)
        {
            LOG_DEBUG_DEV(en_dev->parent, "en_aux event_id[%d] is called\n", event_type);
            en_dev->ops->events_call_chain(en_dev->parent, event_type, NULL);
            return NOTIFY_STOP_MASK;
        }
    }

    return 0;
}

static int32_t dh_eq_async_pf_int(struct notifier_block *nb, unsigned long action, void *data)
{
    struct dh_eq_async * eq_pf_async = container_of(nb, struct dh_eq_async, irq_nb);
    struct zxdh_en_priv *en_priv = (struct zxdh_en_priv *)eq_pf_async->priv;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    struct dh_events *events = en_priv->events;
    struct dh_event_nb *event_nb = NULL;
    uint64_t virt_addr = 0;
    int32_t event_type = 0;
    uint16_t event_idx = 0;
    uint16_t i = 0;

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        return 0;
    }

    virt_addr = en_dev->ops->get_bar_virt_addr(en_dev->parent, 0) + ZXDH_BAR_MSG_OFFSET + ZXDH_BAR_PFVF_MSG_OFFSET;
    event_idx = zxdh_get_event_id(virt_addr, MSG_CHAN_END_PF, MSG_CHAN_END_VF);
    event_type = dh_eq_event_type_get(event_idx);
    //LOG_INFO("------------- event_idx: %d, event_type: %d------------\n", event_idx, event_type);

    for (i = 0; i < events->evt_num; i++)
    {
        event_nb = &events->notifiers[i];

        if (event_type == event_nb->nb.event_type)
        {
            LOG_DEBUG_DEV(en_dev->parent, "en_aux async pf irq_handler called\n");
            en_dev->ops->events_call_chain(en_dev->parent, event_type, NULL);
            return NOTIFY_STOP_MASK;
        }
    }

    return 0;
}

struct dh_aux_async_eq_table
{
    char name[64];
    notifier_fn_t async_int;
};

static struct dh_aux_async_eq_table dh_aux_pf_async_irq_tbl[] =
{
    {"riscv", dh_eq_async_riscv_int},
    {"pf", dh_eq_async_pf_int},
    {"link_info", dh_eq_async_link_info_int},
    {"link_info", dh_eq_async_link_info_int_bond_pf},
    {"ro_info", dh_eq_async_ro_info_update},
};

static struct dh_aux_async_eq_table dh_aux_vf_async_irq_tbl[] =
{
    {"riscv", dh_eq_async_riscv_int},
    {"pf", dh_eq_async_pf_int},
    {"link_info", dh_eq_async_link_info_int},
};

static int32_t dh_aux_setup_async_eq(struct zxdh_en_priv *en_priv, \
                            struct dh_eq_async *eq, const char *name, \
                            notifier_fn_t call)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t err = 0;

    spin_lock_init(&eq->lock);//unused
    eq->priv = en_priv;
    eq->irq_nb.notifier_call = call;
    err = en_dev->ops->async_eq_enable(en_dev->parent, eq, name, true);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to enable %s EQ %d\n", name, err);
    }

    return err;
}

static void cleanup_async_eq(struct zxdh_en_priv *en_priv, struct dh_eq_async *eq, const char *name)
{
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t err = 0;

    err = en_dev->ops->async_eq_enable(en_dev->parent, eq, name, false);
    if (err != 0)
    {
        LOG_ERR_DEV(en_dev->parent, "failed to disable %s EQ %d\n", name, err);
    }
}

static void destroy_async_eqs(struct zxdh_en_priv *en_priv)
{
    struct dh_eq_table *table = &en_priv->eq_table;
    struct dh_aux_eq_table *table_priv = table->priv;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    int32_t i = 0;
    const struct dh_aux_async_eq_table *irq_tbl;
    int32_t tbl_size = 0;

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        irq_tbl = dh_aux_pf_async_irq_tbl;
        tbl_size = ARRAY_SIZE(dh_aux_pf_async_irq_tbl);
    }
    else
    {
        irq_tbl = dh_aux_vf_async_irq_tbl;
        tbl_size = ARRAY_SIZE(dh_aux_vf_async_irq_tbl);
    }

    for (i = 0; i < tbl_size; ++i)
    {
        cleanup_async_eq(en_priv, &table_priv->async_eq_tbl[i], irq_tbl[i].name);
    }
}

void dh_aux_eq_table_destroy(struct zxdh_en_priv *en_priv)
{
    destroy_async_eqs(en_priv);
}

void dh_aux_eq_table_cleanup(struct zxdh_en_priv *en_priv)
{
    kvfree(en_priv->eq_table.priv);
}

int32_t dh_aux_eq_table_init(struct zxdh_en_priv *en_priv)
{
    struct dh_eq_table *eq_table;
    struct dh_aux_eq_table *table_priv = NULL;
    int32_t err = 0;
    uint32_t i = 0;

    eq_table = &en_priv->eq_table;

    table_priv = kvzalloc(sizeof(*table_priv), GFP_KERNEL);
    if (unlikely(table_priv == NULL))
    {
        LOG_ERR_DEV(en_priv->edev.parent, "dh_aux_eq_table kvzalloc failed\n");
        err = -ENOMEM;
        goto err_table_priv;
    }

    eq_table->priv = table_priv;

    mutex_init(&eq_table->lock);
    for (i = 0; i < DH_EVENT_TYPE_MAX; i++)
    {
        ATOMIC_INIT_NOTIFIER_HEAD(&eq_table->nh[i]);
    }

    eq_table->irq_table = NULL;

    return 0;

err_table_priv:
    return err;
}

static int32_t create_async_eqs(struct zxdh_en_priv *en_priv)
{
    struct dh_eq_table *eq_table = &en_priv->eq_table;
    struct dh_aux_eq_table *table_priv = eq_table->priv;
    struct zxdh_en_device *en_dev = &en_priv->edev;
    const struct dh_aux_async_eq_table *irq_tbl;
    int32_t tbl_size = 0;
    int32_t err = 0;
    int32_t i = 0;
    int32_t j = 0;

    if(en_dev->ops->get_coredev_type(en_dev->parent) == DH_COREDEV_PF)
    {
        irq_tbl = dh_aux_pf_async_irq_tbl;
        tbl_size = ARRAY_SIZE(dh_aux_pf_async_irq_tbl);
    }
    else
    {
        irq_tbl = dh_aux_vf_async_irq_tbl;
        tbl_size = ARRAY_SIZE(dh_aux_vf_async_irq_tbl);
    }

    for (i = 0; i < tbl_size; ++i)
    {
        err = dh_aux_setup_async_eq(en_priv, &table_priv->async_eq_tbl[i], \
                irq_tbl[i].name, irq_tbl[i].async_int);
        if (err != 0)
        {
            LOG_ERR_DEV(en_dev->parent, "Failed to setup aux_async_eq_tbl[%d]\n", i);
            goto err_setup_async_eq;
        }
    }

    return err;

err_setup_async_eq:
    for (j = 0; j < i; ++j)
    {
        cleanup_async_eq(en_priv, &table_priv->async_eq_tbl[j], irq_tbl[j].name);
    }
    return err;
}

int32_t dh_aux_eq_table_create(struct zxdh_en_priv *en_priv)
{
    int32_t err = 0;

    err = create_async_eqs(en_priv);
    if (err != 0)
    {
        LOG_ERR_DEV(en_priv->edev.parent, "Failed to create async EQs\n");
    }

    return err;
}

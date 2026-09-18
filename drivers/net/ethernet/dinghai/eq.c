#include <linux/dinghai/eq.h>
#include <linux/dinghai/driver.h>
#include <linux/dinghai/device.h>
#include <linux/dinghai/dinghai_irq.h>
#include <linux/dinghai/helper.h>
#include <linux/notifier.h>
#include <linux/dinghai/pci_irq.h>
#include <linux/dinghai/dh_cmd.h>
#include <net/addrconf.h>


int32_t dh_eq_enable(struct dh_core_dev *dev, struct dh_eq *eq,
                     struct notifier_block *nb)
{
    return dh_irq_attach_nb(eq->irq, nb);
}

int32_t setup_async_eq(struct dh_core_dev *dev, struct dh_eq_async *eq, \
                       struct dh_eq_param *param, notifier_fn_t dh_eq_async_int, \
                       const char *name, void *priv)
{
    struct dh_eq *eq_core = NULL;
    int32_t err = 0;

    eq->irq_nb.notifier_call = dh_eq_async_int;
    eq->priv = priv;
    spin_lock_init(&eq->lock);//unused

    eq_core = &eq->core;
    eq_core->irq = param->irq;

    err = dh_eq_enable(dev, &eq->core, &eq->irq_nb);
    if (err != 0)
    {
        LOG_WARN_DEV(dev, "failed to enable %s EQ %d\n", name, err);
    }

    return err;
}

void dh_eq_disable(struct dh_core_dev *dev, struct dh_eq *eq,
                   struct notifier_block *nb)
{
    dh_irq_detach_nb(eq->irq, nb);
}

void dh_eq_table_cleanup(struct dh_core_dev *dev)
{
    kvfree(dev->eq_table.priv);
}

int32_t dh_inet6_addr_change_notifier_register(struct notifier_block *inet6_addr_change_notifier)
{
    return register_inet6addr_notifier(inet6_addr_change_notifier);
}

int32_t dh_vxlan_netdev_change_notifier_register(struct notifier_block *vxlan_netdev_change_notifier)
{
#ifdef CGS_V5_693
    return register_netdevice_notifier_rh(vxlan_netdev_change_notifier);
#else
    return register_netdevice_notifier(vxlan_netdev_change_notifier);
#endif
}

int32_t dh_vlan_netdev_change_notifier_register(struct notifier_block *vlan_netdev_change_notifier)
{
#ifdef CGS_V5_693
    return register_netdevice_notifier_rh(vlan_netdev_change_notifier);
#else
    return register_netdevice_notifier(vlan_netdev_change_notifier);
#endif
}

int32_t dh_inet6_addr_change_notifier_unregister(struct notifier_block *inet6_addr_change_notifier)
{
    return unregister_inet6addr_notifier(inet6_addr_change_notifier);
}

int32_t dh_vxlan_netdev_change_notifier_unregister(struct notifier_block *vxlan_netdev_change_notifier)
{
#ifdef CGS_V5_693
    return unregister_netdevice_notifier_rh(vxlan_netdev_change_notifier);
#else
    return unregister_netdevice_notifier(vxlan_netdev_change_notifier);
#endif
}

int32_t dh_vlan_netdev_change_notifier_unregister(struct notifier_block *vlan_netdev_change_notifier)
{
#ifdef CGS_V5_693
    return unregister_netdevice_notifier_rh(vlan_netdev_change_notifier);
#else
    return unregister_netdevice_notifier(vlan_netdev_change_notifier);
#endif
}

int32_t dh_eq_notifier_register(struct dh_eq_table *eqt, struct dh_nb *nb)
{
    return atomic_notifier_chain_register(&eqt->nh[nb->event_type], &nb->nb);
}

int32_t dh_eq_notifier_unregister(struct dh_eq_table *eqt, struct dh_nb *nb)
{
    return atomic_notifier_chain_unregister(&eqt->nh[nb->event_type], &nb->nb);
}

void dh_eq_table_init(struct dh_core_dev *dev, void *table_priv)
{
    struct dh_eq_table *eq_table = &dev->eq_table;
    int32_t i;

    eq_table->priv = table_priv;

    mutex_init(&eq_table->lock);
    for (i = 0; i < DH_EVENT_TYPE_MAX; i++)
    {
        ATOMIC_INIT_NOTIFIER_HEAD(&eq_table->nh[i]);
    }

    eq_table->irq_table = &dev->irq_table;
}

static uint16_t event_type_map[MSG_MODULE_NUM] = {
    [MODULE_VF_BAR_MSG_TO_PF] = DH_EVENT_TYPE_NOTIFY_VF_TO_PF,
    [MODULE_RISC_READY] = DH_EVENT_TYPE_RISCV_READY,
    [MODULE_PF_BAR_MSG_TO_VF] = DH_EVENT_TYPE_NOTIFY_PF_TO_VF,
    [MODULE_VIRTIO] = DH_EVENT_TYPE_NOTIFY_ANY,
    [MODULE_DHTOOL] = DH_EVENT_TYPE_NOTIFY_RISCV_TO_AUX,
    [MODULE_VQMB] = DH_EVENT_TYPE_NOTIFY_RISCV_TO_AUX,
    [MODULE_RESET_MSG] = DH_EVENT_TYPE_NOTIFY_ANY,
    [MODULE_DEMO] = DH_EVENT_TYPE_NOTIFY_ANY,
};

uint16_t dh_eq_event_type_get(uint16_t event_id)
{
    if (event_id >= MSG_MODULE_NUM) {
        LOG_ERR("event_id %d is out of range\n", event_id);
        return 0;
    }

    return event_type_map[event_id];
}

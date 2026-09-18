#ifndef __DINGHAI_EQ_H__
#define __DINGHAI_EQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/types.h>
#include <linux/notifier.h>
#include <linux/dinghai/device.h>

struct dh_irq;

#define INVALID_EVENT_TYPE 0

enum DH_EVENT_TYPE {
    DH_EVENT_TYPE_RISCV_READY = 13,
    DH_EVENT_TYPE_NOTIFY_ANY = 14,
    DH_EVENT_TYPE_NOTIFY_RISC_TO_MPF = 15,
    DH_EVENT_TYPE_NOTIFY_PF_TO_MPF = 16,
    DH_EVENT_TYPE_NOTIFY_VF_TO_PF = 18,
    DH_EVENT_TYPE_NOTIFY_PF_TO_VF = 19,
    DH_EVENT_TYPE_NOTIFY_1 = 20,
    DH_EVENT_TYPE_NOTIFY_2 = 21,
    DH_EVENT_TYPE_NOTIFY_3 = 22,
    DH_EVENT_TYPE_NOTIFY_4 = 23,
    DH_EVENT_TYPE_NOTIFY_RISCV_TO_AUX = 24,
    DH_EVENT_TYPE_NOTIFY_RISC_EXT_PPS = 25,
    DH_EVENT_TYPE_NOTIFY_RISC_LOCAL_PPS = 26,
    DH_EVENT_TYPE_AUX_UNLOAD = 27,
    DH_EVENT_TYPE_AUX_LOAD = 28,
    DH_EVENT_TYPE_AUX_EVENT = 29,
    DH_EVENT_TYPE_AUX_STATE = 30,
    DH_EVENT_TYPE_RDMA_LOAD = 31,
    DH_EVENT_TYPE_AUX_INFO = 32,
    DH_EVENT_TYPE_RDMA_EVENT_NOTIFY = 33,
    DH_EVENT_TYPE_MAX = 100,
};

/* eq core */
struct dh_eq {
    struct dh_core_dev *dev;
    __be32 __iomem *doorbell;
    uint32_t cons_index;
    struct dh_irq *irq; /* interrupt core */
};

/* asynchronous interrupt */
struct dh_eq_async {
    struct dh_eq core;
    struct notifier_block irq_nb; /* interrupt: notification chain related to the event queue*/
    void *priv;
    spinlock_t lock;              /* To avoid irq EQ handle races with resiliency flows */
};

struct dh_eq_param {
    int32_t nent;           /* queue depth */
    enum dh_event_queue_type event_type;
    struct dh_irq *irq; /* interrupt associated with the event queue*/
};

/* event type in the event queue */
struct dh_nb {
    struct notifier_block nb;
    int32_t event_type;
};

struct dh_irq_table;

struct dh_eq_table {
    struct atomic_notifier_head nh[DH_EVENT_TYPE_MAX];
    struct mutex lock;
    struct dh_irq_table *irq_table;
    void *priv;
};

struct dh_eq_vq {
    struct dh_eq          core;
    struct notifier_block irq_nb;
    void *para;
};

struct dh_eq_vqs {
    struct dh_eq_vq       vq_s;
    struct list_head      vqs;
    struct list_head      list;
};

#define DH_NB_INIT(name, handler, event) do {     \
    (name)->nb.notifier_call = handler;           \
    (name)->event_type = DH_EVENT_TYPE_##event;   \
} while (0)

void dh_eq_table_cleanup(struct dh_core_dev *dev);
void dh_eq_table_init(struct dh_core_dev *dev, void *table_priv);
int32_t setup_async_eq(struct dh_core_dev *dev, struct dh_eq_async *eq,
                       struct dh_eq_param *param, notifier_fn_t dh_eq_async_int, const char *name, void *priv);
int32_t dh_inet6_addr_change_notifier_register(struct notifier_block *inet6_addr_change_notifier);
int32_t dh_inet6_addr_change_notifier_unregister(struct notifier_block *inet6_addr_change_notifier);
int32_t dh_vxlan_netdev_change_notifier_register(struct notifier_block *vxlan_netdev_change_notifier);
int32_t dh_vxlan_netdev_change_notifier_unregister(struct notifier_block *vxlan_netdev_change_notifier);
int32_t dh_vlan_netdev_change_notifier_register(struct notifier_block *vlan_netdev_change_notifier);
int32_t dh_vlan_netdev_change_notifier_unregister(struct notifier_block *vlan_netdev_change_notifier);
int32_t dh_eq_notifier_register(struct dh_eq_table *eqt, struct dh_nb *nb);
void dh_eq_disable(struct dh_core_dev *dev, struct dh_eq *eq, struct notifier_block *nb);
int32_t dh_eq_enable(struct dh_core_dev *dev, struct dh_eq *eq, struct notifier_block *nb);
int32_t dh_eq_notifier_unregister(struct dh_eq_table *eqt, struct dh_nb *nb);
uint16_t dh_eq_event_type_get(uint16_t event_id);

typedef int32_t (*zxdh_callchain_cbk_t)(struct notifier_block *nb, unsigned long action, void *data);

struct dh_vq_handler {
    zxdh_callchain_cbk_t callback;
    void *para;
};


#ifdef __cplusplus
}
#endif

#endif

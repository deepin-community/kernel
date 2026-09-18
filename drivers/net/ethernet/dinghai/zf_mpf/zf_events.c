#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/dinghai/driver.h>
#include <linux/notifier.h>
#include <linux/dinghai/events.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/module.h>
#include "zf_events.h"

static int32_t zf_riscv_notifier(struct notifier_block *nb, unsigned long type, void *data);
static int32_t zf_pf_notifier(struct notifier_block *nb, unsigned long type, void *data);
static int32_t irq1_notifier(struct notifier_block *nb, unsigned long type, void *data);
static int32_t fuc_hotplug_failed_notifier(struct notifier_block *nb, unsigned long type, void *data);
static int32_t fuc_hotplug_finish_notifier(struct notifier_block *nb, unsigned long type, void *data);
static int32_t irq4_notifier(struct notifier_block *nb, unsigned long type, void *data);
int get_fuc_hp_ret(void);
int finish_flag = 0;

static struct dh_nb zf_mpf_events[] = {
    {.nb.notifier_call = zf_riscv_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_RISC_TO_MPF},
    {.nb.notifier_call = zf_pf_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_PF_TO_MPF},
    {.nb.notifier_call = irq1_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_1},
    {.nb.notifier_call = fuc_hotplug_failed_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_2},
    {.nb.notifier_call = fuc_hotplug_finish_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_3},
    {.nb.notifier_call = irq4_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_4}
};

static int32_t zf_riscv_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct dh_core_dev *dh_dev = (struct dh_core_dev *)event_nb->ctx;
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dh_dev);

    DH_LOG_INFO_DEV(MODULE_MPF, dh_dev, "%s is called, type=%ld\n", __func__, type);
    zxdh_events_work_enqueue(dh_dev, &mpf_dev->dh_np_sdk_from_risc);

    return NOTIFY_OK;
}

static int32_t zf_pf_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct dh_core_dev *dh_dev = (struct dh_core_dev *)event_nb->ctx;
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dh_dev);

    DH_LOG_INFO_DEV(MODULE_MPF, dh_dev, "%s is called, type=%ld\n", __func__, type);
    zxdh_events_work_enqueue(dh_dev, &mpf_dev->dh_np_sdk_from_pf);

    return NOTIFY_OK;
}

static int32_t irq1_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct dh_core_dev *dh_dev = (struct dh_core_dev *)event_nb->ctx;
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dh_dev);

    DH_LOG_INFO_DEV(MODULE_MPF, dh_dev, "%s is called, ep_bdf=0x%x, pcie_id=%d\n", __func__, mpf_dev->ep_bdf, mpf_dev->pcie_id);

    return zf_hdma_wr_handler((void *)dh_dev->zf_ep->dpu_ep_array[0]);
}

static int32_t fuc_hotplug_failed_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    finish_flag = FUC_HP_RET_FAILED;
    DH_LOG_ERR(MODULE_MPF, "hotplug failed\n");
    return NOTIFY_OK;
}
static int32_t fuc_hotplug_finish_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    finish_flag = FUC_HP_RET_FINISH;
    DH_LOG_INFO(MODULE_MPF, "hotplug success\n");
    return NOTIFY_OK;
}

static int32_t irq4_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct dh_core_dev *dh_dev = (struct dh_core_dev *)event_nb->ctx;
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dh_dev);

    DH_LOG_INFO_DEV(MODULE_MPF, dh_dev, "%s is called, ep_bdf=0x%x, pcie_id=%d\n", __func__, mpf_dev->ep_bdf, mpf_dev->pcie_id);

    return NOTIFY_OK;
}

static void zf_np_sdk_handler_from_risc(struct work_struct *p_work)
{
    struct dh_en_mpf_dev *mpf_dev = container_of(p_work, struct dh_en_mpf_dev, dh_np_sdk_from_risc);

    DH_LOG_INFO(MODULE_MPF, "%s is called\n", __func__);
    zxdh_bar_irq_recv(MSG_CHAN_END_RISC, MSG_CHAN_END_MPF, mpf_dev->pci_ioremap_addr + ZXDH_BAR1_CHAN_OFFSET, NULL);
    return;
}

static void zf_np_sdk_handler_from_pf(struct work_struct *p_work)
{
    struct dh_en_mpf_dev *mpf_dev = container_of(p_work, struct dh_en_mpf_dev, dh_np_sdk_from_pf);

    DH_LOG_ERR(MODULE_MPF, "is called\n");
    zxdh_bar_irq_recv(MSG_CHAN_END_PF, MSG_CHAN_END_MPF, mpf_dev->pci_ioremap_addr + ZXDH_BAR2_CHAN_OFFSET, NULL);
    return;
}

void zxdh_zf_events_start(struct dh_core_dev *dev)
{
    struct dh_events *events = dev->events;
    int32_t i = 0;
    int32_t err = 0;

    for (i = 0; i < ARRAY_SIZE(zf_mpf_events); i++)
    {
        events->notifiers[i].nb = zf_mpf_events[i];
        events->notifiers[i].ctx = dev;
        err = dh_eq_notifier_register(&dev->eq_table, &events->notifiers[i].nb);
        if (err != 0)
        {
            DH_LOG_ERR_DEV(MODULE_MPF, dev, "i: %d, err: %d.\n", i, err);
        }
    }
}

void dh_zf_events_stop(struct dh_core_dev *dev)
{
    struct dh_events *events = dev->events;
    int32_t i = 0;

    for (i = ARRAY_SIZE(zf_mpf_events) - 1; i >= 0 ; i--)
    {
        dh_eq_notifier_unregister(&dev->eq_table, &events->notifiers[i].nb);
    }

    zxdh_events_cleanup(dev);
}

int32_t dh_zf_mpf_events_init(struct dh_core_dev *dev)
{
    struct dh_events *events = NULL;
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dev);
    int32_t ret = 0;

    events = kzalloc((sizeof(*events) + ARRAY_SIZE(zf_mpf_events) * sizeof(struct dh_event_nb)), GFP_KERNEL);
    if (unlikely(events == NULL))
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "events kzalloc failed: %p\n", events);
        ret = -ENOMEM;
        goto err_events_kzalloc;
    }

    events->evt_num = ARRAY_SIZE(zf_mpf_events);
    events->dev = dev;
    dev->events = events;
    events->wq = create_singlethread_workqueue("dh_zf_mpf_events");
    if (!events->wq)
    {
        DH_LOG_ERR_DEV(MODULE_MPF, dev, "events->wq create_singlethread_workqueue failed: %p\n", events->wq);
        ret = -ENOMEM;
        goto err_create_wq;
    }

    INIT_WORK(&mpf_dev->dh_np_sdk_from_risc, zf_np_sdk_handler_from_risc);
    INIT_WORK(&mpf_dev->dh_np_sdk_from_pf, zf_np_sdk_handler_from_pf);

    zxdh_zf_events_start(dev);

    return 0;

err_create_wq:
    kfree(events);
err_events_kzalloc:
    return ret;
}

void dh_zf_mpf_events_uninit(struct dh_core_dev *dev)
{
    return dh_zf_events_stop(dev);
}

void reset_fuc_hp_ret(void)
{
    finish_flag = 0;
    return;
}

int get_fuc_hp_ret(void)
{
    return finish_flag;
}
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/dinghai/driver.h>
#include <linux/notifier.h>
#include <linux/dinghai/events.h>
#include <linux/dinghai/dh_cmd.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include "en_mpf_events.h"
#include "../en_mpf.h"

static int32_t riscv_notifier(struct notifier_block *nb, unsigned long type, void *data);
static int32_t pf_notifier(struct notifier_block *nb, unsigned long type, void *data);

static struct dh_nb mpf_events[] = {
    {.nb.notifier_call = riscv_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_RISC_TO_MPF},
    {.nb.notifier_call = pf_notifier, .event_type = DH_EVENT_TYPE_NOTIFY_PF_TO_MPF}
};

static int32_t riscv_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct dh_core_dev *dh_dev = (struct dh_core_dev *)event_nb->ctx;
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dh_dev);

    zxdh_events_work_enqueue(dh_dev, &mpf_dev->dh_np_sdk_from_risc);

    return NOTIFY_OK;
}

static int32_t pf_notifier(struct notifier_block *nb, unsigned long type, void *data)
{
    struct dh_event_nb *event_nb = dh_nb_cof(nb, struct dh_event_nb, nb);
    struct dh_core_dev *dh_dev = (struct dh_core_dev *)event_nb->ctx;
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dh_dev);

    zxdh_events_work_enqueue(dh_dev, &mpf_dev->dh_np_sdk_from_pf);

    return NOTIFY_OK;
}

void np_sdk_handler_from_risc(struct work_struct *p_work)
{
    struct dh_en_mpf_dev *mpf_dev = container_of(p_work, struct dh_en_mpf_dev, dh_np_sdk_from_risc);

    LOG_INFO("%s is called\n", __func__);
    zxdh_bar_irq_recv(MSG_CHAN_END_RISC, MSG_CHAN_END_MPF, mpf_dev->pci_ioremap_addr + ZXDH_BAR1_CHAN_OFFSET, NULL);
    return;
}

void np_sdk_handler_from_pf(struct work_struct *p_work)
{
    struct dh_en_mpf_dev *mpf_dev = container_of(p_work, struct dh_en_mpf_dev, dh_np_sdk_from_pf);

    LOG_INFO("%s is called\n", __func__);
    zxdh_bar_irq_recv(MSG_CHAN_END_PF, MSG_CHAN_END_MPF, mpf_dev->pci_ioremap_addr + ZXDH_BAR2_CHAN_OFFSET, NULL);
    return;
}

void zxdh_events_start(struct dh_core_dev *dev)
{
    struct dh_events *events = dev->events;
    int32_t i;
    int32_t err;

    for (i = 0; i < ARRAY_SIZE(mpf_events); i++)
    {
        events->notifiers[i].nb = mpf_events[i];
        events->notifiers[i].ctx = dev;
        err = dh_eq_notifier_register(&dev->eq_table, &events->notifiers[i].nb);
        if (err != 0)
        {
            LOG_ERR_DEV(dev, "i: %d, err: %d.\n", i, err);
        }
    }
}

int32_t dh_mpf_events_init(struct dh_core_dev *dev)
{
    struct dh_events *events = NULL;
    struct dh_en_mpf_dev *mpf_dev = dh_core_priv(dev);
    int32_t ret = 0;

    events = kzalloc((sizeof(*events) + ARRAY_SIZE(mpf_events) * sizeof(struct dh_event_nb)), GFP_KERNEL);
    if (unlikely(events == NULL))
    {
        LOG_ERR_DEV(dev, "events kzalloc failed: %p\n", events);
        ret = -ENOMEM;
        goto err_events_kzalloc;
    }

    events->evt_num = ARRAY_SIZE(mpf_events);
    events->dev = dev;
    dev->events = events;
    events->wq = create_singlethread_workqueue("dh_mpf_events");
    if (!events->wq)
    {
        LOG_ERR_DEV(dev, "events->wq create_singlethread_workqueue failed: %p\n", events->wq);
        ret = -ENOMEM;
        goto err_create_wq;
    }

    INIT_WORK(&mpf_dev->dh_np_sdk_from_risc, np_sdk_handler_from_risc);
    INIT_WORK(&mpf_dev->dh_np_sdk_from_pf, np_sdk_handler_from_pf);

    zxdh_events_start(dev);

    return 0;

err_create_wq:
    kfree(events);
err_events_kzalloc:
    return ret;
}

void dh_events_stop(struct dh_core_dev *dev)
{
    struct dh_events *events = dev->events;
    int32_t i = 0;

    for (i = ARRAY_SIZE(mpf_events) - 1; i >= 0 ; i--)
    {
        dh_eq_notifier_unregister(&dev->eq_table, &events->notifiers[i].nb);
    }

    zxdh_events_cleanup(dev);
}

void dh_mpf_events_uninit(struct dh_core_dev *dev)
{
    return dh_events_stop(dev);
}
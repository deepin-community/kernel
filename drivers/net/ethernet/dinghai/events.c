#include <linux/dinghai/events.h>

void zxdh_events_work_enqueue(struct dh_core_dev *dev, struct work_struct *work)
{
    queue_work(dev->events->wq, work);
}

void zxdh_events_cleanup(struct dh_core_dev *dev)
{
    destroy_workqueue(dev->events->wq);
    kfree(dev->events);
}
 

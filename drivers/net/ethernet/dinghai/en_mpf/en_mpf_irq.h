#ifndef __EN_MPF_IRQ_H__
#define __EN_MPF_IRQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/dinghai_irq.h>
#include <linux/dinghai/driver.h>

struct dh_irq *dh_mpf_async_irq_request(struct dh_core_dev *dev);
void dh_mpf_irq_table_destroy(struct dh_core_dev *dev);
int32_t dh_mpf_irq_table_create(struct dh_core_dev *dev);
int32_t dh_mpf_irq_table_init(struct dh_core_dev *dev);

#ifdef __cplusplus
}
#endif

#endif
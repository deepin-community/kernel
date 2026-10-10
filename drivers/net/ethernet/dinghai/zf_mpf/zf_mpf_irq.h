#ifndef __ZF_MPF_IRQ_H__
#define __ZF_MPF_IRQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/dinghai_irq.h>
#include <linux/dinghai/driver.h>

#include "gdma.h"

#define ZXDH_MPF_ASYNC_IRQ_NUM 6
#define ZXDH_MPF_ASYNC_IRQ_MIN_COMP 0
#define ZXDH_MPF_ASYNC_IRQ_MAX_COMP 1

#define ZXDH_MPF_COMP_IRQ_NUM 1
#define ZXDH_MPF_COMP_IRQ_MIN_COMP 0
#define ZXDH_MPF_COMP_IRQ_MAX_COMP 1

#define ZXDH_MPF_GDMA_IRQ_NUM (ZF_GDMA_CHAN_NUM + 1)
#define ZXDH_MPF_GDMA_MSIX_VEC_BASE 10
#define ZXDH_MPF_GDMA_IRQ_MIN 0
#define ZXDH_MPF_GDMA_IRQ_MAX 1

/* async irq:<0-5> comp irq:<6> gdma irq:<10-14> */
#define ZXDH_ZF_MPF_IRQ_NUM_TOTAL 16

struct dh_mpf_irq_table {
    struct dh_irq_pool *mpf_comp_pool;
    struct dh_irq_pool *mpf_async_pool;
    struct dh_irq_pool *mpf_gdma_pool;
};

struct dh_irq_range {
    int32_t start;
    int32_t size;
};

struct dh_irq *dh_mpf_async_irq_request(struct dh_core_dev *dev);
void dh_mpf_irq_table_destroy(struct dh_core_dev *dev);
int32_t dh_mpf_irq_table_create(struct dh_core_dev *dev);
int32_t dh_mpf_irq_table_init(struct dh_core_dev *dev);

#ifdef __cplusplus
}
#endif

#endif
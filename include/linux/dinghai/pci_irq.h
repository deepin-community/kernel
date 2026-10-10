#ifndef DINGHAI_PCI_IRQ_H
#define DINGHAI_PCI_IRQ_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/xarray.h>
#include <linux/types.h>

#define DH_MAX_IRQ_NAME 100
#define DH_FW_RESERVED_EQS 16
/* max irq_index is 2047, so four chars */
#define DH_MAX_IRQ_IDX_CHARS (4)
#define DH_EQ_REFS_PER_IRQ (2)

#define ZXDH_VQS_CHANNELS_NUM (64)
#define ZXDH_BOND_VQS_CHANNELS_NUM (1)
#define ZXDH_ASYNC_CHANNELS_NUM 8
#define ZXDH_RDMA_CHANNELS_NUM 6
#define ZXDH_RDMA_IRQ_START_IDX ZXDH_ASYNC_CHANNELS_NUM
#define ZXDH_VQS_IRQ_START_IDX (ZXDH_ASYNC_CHANNELS_NUM + ZXDH_RDMA_CHANNELS_NUM)

#define ZXDH_VF_VQS_CHANNELS_NUM 8
#define ZXDH_VF_ASYNC_CHANNELS_NUM 6
#define ZXDH_VF_RDMA_IRQ_START_IDX ZXDH_VF_ASYNC_CHANNELS_NUM
#define ZXDH_VF_VQS_IRQ_START_IDX (ZXDH_RDMA_CHANNELS_NUM + ZXDH_VF_ASYNC_CHANNELS_NUM)

struct dh_irq;

struct dh_irq_pool {
    char name[DH_MAX_IRQ_NAME];
    struct xa_limit xa_num_irqs;
    struct mutex lock; /* sync IRQs creations */
    struct xarray irqs;
    uint32_t max_threshold;
    uint32_t min_threshold;
    uint16_t *irqs_per_cpu;
    struct dh_core_dev *dev;
    void *data;
};

static inline bool dh_irq_pool_is_sf_pool(struct dh_irq_pool *pool)
{
    return !strncmp("dh_mpf_sf", pool->name, strlen("dh_mpf_sf"));
}

struct dh_irq *zxdh_get_irq_of_pool(struct dh_core_dev *dev, struct dh_irq_pool *pool);

struct dh_irq *dh_irq_alloc(struct dh_irq_pool *pool, int32_t i,
                            const struct cpumask *affinity);

int32_t dh_irq_read_locked(struct dh_irq *irq);
int32_t dh_irq_get_locked(struct dh_irq *irq);
int32_t dh_irq_put(struct dh_irq *irq);

void irq_pool_free(struct dh_irq_pool *pool);
struct dh_irq_pool *
irq_pool_alloc(struct dh_core_dev *dev, int32_t start, int32_t size, char *name,
               uint32_t min_threshold, uint32_t max_threshold);
struct dh_irq {
    struct atomic_notifier_head nh; /* notification chain invoked when an interruption is triggered. the invocation environment is the atomic context */
    cpumask_var_t mask;             /* interrupt affinity */
    char name[DH_MAX_IRQ_NAME];     /* interrupt name */
    struct dh_irq_pool *pool;       /* interrupt pool */
    int32_t refcount;
    uint32_t index;                 /* interrupt vec index */
    int32_t irqn;                   /* interrupt number */
};

#ifdef __cplusplus
}
#endif

#endif
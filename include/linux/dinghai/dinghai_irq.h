#ifndef __DINGHAI_IRQ_H__
#define __DINGHAI_IRQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>
#include <linux/dinghai/pci_irq.h>

#define DH_COMP_EQS_PER_SF 8

struct dh_irq;

int32_t dh_irq_table_init(struct dh_core_dev *dev);
void dh_irq_table_cleanup(struct dh_core_dev *dev);
int32_t dh_irq_table_create(struct dh_core_dev *dev);
void dh_irq_table_destroy(struct dh_core_dev *dev);
void dh_irqs_release_vectors(struct dh_irq **irqs, int32_t nirqs);
int32_t dh_irq_attach_nb(struct dh_irq *irq, struct notifier_block *nb);
int32_t dh_irq_detach_nb(struct dh_irq *irq, struct notifier_block *nb);
struct cpumask *dh_irq_get_affinity_mask(struct dh_irq *irq);
int32_t dh_irq_get_index(struct dh_irq *irq);

struct dh_irq_pool;

int32_t dh_irq_affinity_irqs_request_auto(struct dh_irq_pool *pool, struct dh_irq **irqs, int32_t num_irqs,
                                                                       int32_t num_irqs_per_cpu, int numa);
struct dh_irq *dh_irq_affinity_request(struct dh_irq_pool *pool, const struct cpumask *req_mask);
void dh_irq_affinity_irqs_release(struct dh_irq_pool *pool, struct dh_irq **irqs, int32_t num_irqs);

#ifdef __cplusplus
}
#endif

#endif

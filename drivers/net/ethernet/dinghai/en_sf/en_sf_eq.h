#ifndef __EN_SF_EQ_H__
#define __EN_SF_EQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>
#include <linux/notifier.h>
#include <linux/types.h>
#include <linux/dinghai/pci_irq.h>
#include <linux/dinghai/eq.h>
#include <linux/dinghai/en_sf.h>

void dh_en_sf_eq_table_destroy(struct dh_core_dev *dev);
int32_t dh_en_sf_eq_table_init(struct dh_core_dev *dev);
int32_t dh_en_sf_eq_table_create(struct dh_core_dev *dev, struct zxdh_en_sf_if *ops);
void dh_sf_eq_table_destroy(struct dh_core_dev *dev);

uint16_t zxdh_en_sf_get_vqs_channels_num(struct dh_core_dev *dh_dev);
int32_t zxdh_en_sf_create_vqs_channels(struct dh_core_dev *dh_dev, void *data);
void zxdh_en_sf_destroy_vqs_channels(struct dh_core_dev *dh_dev);
void zxdh_en_sf_switch_vqs_channel(struct dh_core_dev *dh_dev, int32_t channel, int32_t op);
int32_t zxdh_en_sf_vqs_channel_bind_handler(struct dh_core_dev *dh_dev, int32_t vqs_channel_num, struct dh_vq_handler *handler);
void zxdh_en_sf_vqs_channel_unbind_handler(struct dh_core_dev *dh_dev, int32_t vqs_channel_num);
int32_t zxdh_en_sf_vq_bind_channel(struct dh_core_dev *dh_dev, uint16_t channel_num, uint16_t phy_index, uint16_t index, uint16_t vq_idx);
void zxdh_en_sf_vq_unbind_channel(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index);
int32_t zxdh_en_sf_vqs_bind_eqs(struct dh_core_dev *dh_dev, int32_t vqs_channel_num, struct list_head *vq_node);
void zxdh_en_sf_vqs_unbind_eqs(struct dh_core_dev *dh_dev, int32_t vqs_channel_num);
void __iomem *zxdh_en_sf_map_vq_notify(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, resource_size_t *pa);
void zxdh_en_sf_unmap_vq_notify(struct dh_core_dev *dh_dev, void *priv);
void zxdh_en_sf_activate_phy_vq(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index,
                                int32_t queue_size, uint64_t desc_addr, uint64_t avail_addr, uint64_t used_addr);
void zxdh_en_sf_set_queue_enable(struct dh_core_dev *dh_dev, uint16_t phy_index, uint16_t index, bool enable);

uint16_t zxdh_en_sf_get_epbdf(struct dh_core_dev *dh_dev);
uint64_t zxdh_en_sf_get_spec_sbdf(struct dh_core_dev *dh_dev);
uint16_t zxdh_en_sf_get_vport(struct dh_core_dev *dh_dev);
uint16_t zxdh_en_sf_get_pcie_id(struct dh_core_dev *dh_dev);
uint16_t zxdh_en_sf_get_slot_id(struct dh_core_dev *dh_dev);
enum dh_coredev_type zxdh_en_sf_get_coredev_type(struct dh_core_dev *dh_dev);
void zxdh_en_sf_dpp_np_init(struct dh_core_dev *dh_dev, uint32_t vport);
struct pci_dev *zxdh_en_sf_get_pdev(struct dh_core_dev *dh_dev);
uint64_t zxdh_en_sf_get_bar_virt_addr(struct dh_core_dev *dh_dev, uint8_t bar_num);
int32_t zxdh_en_sf_do_cmd_exec(struct dh_core_dev *dh_dev, uint32_t dst, uint32_t id, uint32_t len, void *payload, void *ack);
struct zxdh_vf_item *zxdh_en_sf_get_vf_item(struct dh_core_dev *dh_dev, uint16_t vf_idx);

struct dh_en_sf_eq_table {
    struct dh_irq **vq_irqs;
    struct dh_irq *async_irq;
    struct dh_eq_async async_eq;
    int32_t vq_irq_num;
    struct list_head vqs_eqs_list;
};

#ifdef __cplusplus
}
#endif

#endif

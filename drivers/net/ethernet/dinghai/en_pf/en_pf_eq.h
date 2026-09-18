#ifndef __EN_PF_EQ_H__
#define __EN_PF_EQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>
#include <linux/notifier.h>
#include <linux/types.h>
#include <linux/dinghai/pci_irq.h>
#include <linux/dinghai/eq.h>

#define ZXDH_PF_INVALID_MSIX_VEC    0xffff
#define ZXDH_MAC_FLAG_BAR_OFFSET    0xFB000
#define ZXDH_MAC_VERSION_OFFSET     0x5
#define ZXDH_MAC_VERSION            0xfa
#define ZXDH_MAC_HEADER_BAR_OFFSET  0x8
#define ZXDH_MAC_HEADER_OLD_OFFSET  0x30
#define MAX_EP                   5
#define ZXDH_EP_FLAG_SIZE        2048
#define ZXDH_PF_FLAG_SIZE        256
#define ZXDH_VF_NUM              256

int32_t dh_pf_eq_table_create(struct dh_core_dev *dev, bool boot);
void dh_pf_eq_table_destroy(struct dh_core_dev *dev, bool boot);
int32_t dh_pf_eq_table_init(struct dh_core_dev *dev);
uint16_t zxdh_pf_get_vqs_channels_num(struct dh_core_dev *dh_dev);

void zxdh_pf_switch_irq(struct dh_core_dev *dh_dev, int32_t i, int32_t op);
int32_t zxdh_pf_vq_irqs_request(struct dh_core_dev *dh_dev, struct dh_irq **vq_irqs, int32_t vq_channels, void *data);
void zxdh_pf_affinity_irqs_release(struct dh_core_dev *dh_dev, struct dh_irq **vq_irqs, int32_t num_irqs);
void zxdh_enable_irq(struct dh_core_dev *dh_dev, int32_t irq_index);

int32_t zxdh_pf_async_eq_enable(struct dh_core_dev *dev, struct dh_eq_async *eq, const char *name, bool attach);
void zxdh_pf_set_pf_link_up(struct dh_core_dev *dh_dev, bool link_up);
bool zxdh_pf_get_pf_link_up(struct dh_core_dev *dh_dev);
bool zxdh_pf_set_vf_link_info(struct dh_core_dev *dh_dev, uint16_t vf_idx, uint8_t link_up);
uint8_t zxdh_pf_get_vf_link_info(struct dh_core_dev *dh_dev, uint16_t vf_idx);
bool zxdh_pf_get_vf_is_probe(struct dh_core_dev *dh_dev, uint16_t vf_idx);
void zxdh_pf_get_link_info_from_vqm(struct dh_core_dev *dh_dev, uint8_t *link_up);
void zxdh_pf_set_pf_phy_port(struct dh_core_dev *dh_dev, uint8_t phy_port);
uint8_t zxdh_pf_get_pf_phy_port(struct dh_core_dev *dh_dev);
int32_t zxdh_pf_call_aux_events(struct dh_core_dev *dev, int32_t event_type);
int32_t zxdh_pf_call_aux_events_with_data(struct dh_core_dev *dev, int32_t event_type, void* data);

struct dh_pf_eq_table {
    struct dh_irq      **vq_irqs;
    int32_t vq_irq_num;
    struct list_head    vqs_eqs_list;
    struct dh_irq       *async_irq_tbl[ZXDH_ASYNC_CHANNELS_NUM];
    struct dh_eq_async  async_eq_tbl[ZXDH_ASYNC_CHANNELS_NUM];
};

struct dh_pf_async_irq_table
{
    char name[64];
    notifier_fn_t async_int;
};

#ifdef __cplusplus
}
#endif

#endif

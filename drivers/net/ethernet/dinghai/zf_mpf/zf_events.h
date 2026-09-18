#ifndef __ZXDH_ZF_MPF_EVENTS_H__
#define __ZXDH_ZF_MPF_EVENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>
#include "epc/pcie-zte-zf-epc.h"
#include "fuc_hotplug/fuc_hotplug_commom.h"

int32_t dh_zf_mpf_events_init(struct dh_core_dev *dev);
void zxdh_zf_events_start(struct dh_core_dev *dev);
int32_t dh_zf_mpf_events_init(struct dh_core_dev *dev);
void dh_zf_events_stop(struct dh_core_dev *dev);
void dh_zf_mpf_events_uninit(struct dh_core_dev *dev);
int zf_hdma_wr_handler(void *data);
int zf_hdma_rd_handler(void *data);

#ifdef __cplusplus
}
#endif

#endif
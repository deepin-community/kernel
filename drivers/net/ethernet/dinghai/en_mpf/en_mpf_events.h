#ifndef __EN_MPF_EVENTS_H__
#define __EN_MPF_EVENTS_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>

int32_t dh_mpf_events_init(struct dh_core_dev *dev);
void dh_mpf_events_uninit(struct dh_core_dev *dev);
void zxdh_events_start(struct dh_core_dev *dev);

#ifdef __cplusplus
}
#endif

#endif
#ifndef __ZF_MPF_EQ_H__
#define __ZF_MPF_EQ_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/dinghai/driver.h>

int32_t dh_mpf_eq_table_init(struct dh_core_dev *dev);

int32_t dh_mpf_eq_table_create(struct dh_core_dev *dev);
void dh_mpf_eq_table_destroy(struct dh_core_dev *dev);
#ifdef CONFIG_ZF_GDMA
int32_t create_gdma_eqs(struct dh_core_dev *dev);
void destroy_gdma_eqs(struct dh_core_dev *dev);
#endif

#ifdef __cplusplus
}
#endif

#endif
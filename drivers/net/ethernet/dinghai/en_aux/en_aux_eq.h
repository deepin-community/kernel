#ifndef __EN_AUX_EQ_H__
#define __EN_AUX_EQ_H__

#include "../en_aux.h"
#ifdef __cplusplus
extern "C" {
#endif

#define ZXDH_AUX_ASYNC_EQ_NUM 5
struct dh_aux_eq_table {
    struct dh_eq_async async_eq_tbl[ZXDH_AUX_ASYNC_EQ_NUM];
};

int32_t dh_aux_eq_table_init(struct zxdh_en_priv *en_priv);
int32_t dh_aux_eq_table_create(struct zxdh_en_priv *en_priv);
void dh_aux_eq_table_destroy(struct zxdh_en_priv *en_priv);
void dh_aux_eq_table_cleanup(struct zxdh_en_priv *en_priv);
int32_t dh_eq_async_link_info_int_process(struct zxdh_en_priv *en_priv);
int32_t dh_bond_pf_link_info_get(struct zxdh_en_priv *en_priv);
#ifdef __cplusplus
}
#endif

#endif
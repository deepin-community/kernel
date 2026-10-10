#ifndef __ZXDH_EN_DCBNL_API_H__
#define __ZXDH_EN_DCBNL_API_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/device.h>
#include <linux/dinghai/kcompat.h>
#if defined(CHECK_DCB_ENABLED) && !defined(CONFIG_DCB)
#include <net/dcbnl.h>
#endif
struct zxdh_en_priv;

uint32_t zxdh_dcbnl_init_port_speed(struct zxdh_en_priv *en_priv);
uint32_t zxdh_dcbnl_init_ets_scheduling_tree(struct zxdh_en_priv *en_priv, bool dcb_init);
uint32_t zxdh_dcbnl_printk_ets_tree(struct zxdh_en_priv *en_priv);
uint32_t zxdh_dcbnl_pfc_init(struct zxdh_en_priv *en_priv);

uint32_t zxdh_dcbnl_free_flow_resources(struct zxdh_en_priv *en_priv);
uint32_t zxdh_dcbnl_free_se_resources(struct zxdh_en_priv *en_priv);

uint32_t zxdh_dcbnl_set_tc_scheduling(struct zxdh_en_priv *en_priv, uint8_t *tc_type, uint8_t *tc_tx_bw);
uint32_t zxdh_dcbnl_set_ets_up_tc_map(struct zxdh_en_priv *en_priv, uint8_t *prio_tc);
uint32_t zxdh_dcbnl_set_tc_maxrate(struct zxdh_en_priv *en_priv, uint32_t *maxrate);
uint32_t zxdh_dcbnl_set_ets_trust(struct zxdh_en_priv *en_priv, uint32_t trust);
uint32_t zxdh_dcbnl_set_dscp2prio(struct zxdh_en_priv *en_priv, uint16_t dscp, uint8_t prio);

uint32_t zxdh_dcbnl_set_tm_gate(struct zxdh_en_priv *en_priv, uint32_t mode);
uint32_t zxdh_dcbnl_set_flow_td_th(struct zxdh_en_priv *en_priv, uint32_t* tc_td_th);
uint32_t zxdh_dcbnl_set_single_td_th(struct zxdh_en_priv *en_priv, uint32_t tc, uint32_t tc_td_th);
uint32_t zxdh_dcbnl_get_flow_td_th(struct zxdh_en_priv *en_priv, uint32_t* tc_td_th);
uint32_t zxdh_dcbnl_clear_flow_td_th(struct zxdh_en_priv *en_priv);
uint32_t zxdh_dcbnl_enable_debug(struct zxdh_en_priv *en_priv);
uint32_t zxdh_dcbnl_disable_debug(struct zxdh_en_priv *en_priv);
uint32_t zxdh_dcbnl_set_flow_monitor_gate(struct zxdh_en_priv *en_priv, uint32_t flag);

#ifdef __cplusplus
}
#endif

#endif

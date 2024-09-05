#ifndef __SXE_DCB_H__
#define __SXE_DCB_H__

#ifdef SXE_DCB_CONFIGURE
#include <linux/dcbnl.h>
#include "sxe_hw.h"

extern const struct dcbnl_rtnl_ops sxe_dcbnl_ops;
struct sxe_adapter;
#endif 

#define SXE_MAX_PACKET_BUFFERS   8
#define MAX_USER_PRIORITY        8
#define MAX_BW_GROUP             8
#define BW_PERCENT               100

enum {
	DCB_PATH_TX   =  0,
	DCB_PATH_RX   =  1,
	DCB_PATH_NUM  =  DCB_PATH_RX + 1,
};

#define DCB_ERR_CONFIG           1

#define DCB_ERR_BW_GROUP        -3
#define DCB_ERR_TC_BW           -4
#define DCB_ERR_LS_GS           -5
#define DCB_ERR_LS_BW_NONZERO   -6
#define DCB_ERR_LS_BWG_NONZERO  -7
#define DCB_ERR_TC_BW_ZERO      -8

#define DCB_NOT_IMPLEMENTED      0x7FFFFFFF

#define SXE_DCB_MMW_SIZE_DEFAULT  0x04

#define SXE_PERCENT_100			 100

#define SXE_DCB_PG_SUPPORT        0x00000001
#define SXE_DCB_PFC_SUPPORT       0x00000002
#define SXE_DCB_BCN_SUPPORT       0x00000004
#define SXE_DCB_UP2TC_SUPPORT     0x00000008
#define SXE_DCB_GSP_SUPPORT       0x00000010
#define SXE_DCB_8_TC_SUPPORT      0x80

#define DCB_CREDIT_QUANTUM	64   
#define MAX_CREDIT_REFILL       511  
#define DCB_MAX_TSO_SIZE        (32*1024) 
#define MINIMUM_CREDIT_FOR_TSO  (DCB_MAX_TSO_SIZE/64 + 1) 
#define MAX_CREDIT              4095 

struct sxe_tc_bw_alloc {
	u8 bwg_id;		  
	u8 bwg_percent;		  
	u8 link_percent;	  
	u8 up_to_tc_bitmap;	  
	u16 data_credits_refill;  
	u16 data_credits_max;	  
	enum sxe_strict_prio_type prio_type; 
};

enum sxe_dcb_pfc_type {
	pfc_disabled = 0,
	pfc_enabled_full,
	pfc_enabled_tx,
	pfc_enabled_rx
};

struct sxe_tc_config {
	struct sxe_tc_bw_alloc channel[DCB_PATH_NUM]; 
	enum sxe_dcb_pfc_type  pfc_type; 

	u16 desc_credits_max; 
};

struct sxe_dcb_num_tcs {
	u8 pg_tcs;    
	u8 pfc_tcs;
};

struct sxe_dcb_cee_config {
	struct sxe_dcb_num_tcs num_tcs;
	struct sxe_tc_config tc_config[MAX_TRAFFIC_CLASS];
	u8     bwg_link_percent[DCB_PATH_NUM][MAX_BW_GROUP];
	bool   pfc_mode_enable;
};

#ifdef SXE_DCB_CONFIGURE

void sxe_dcb_init(struct sxe_adapter *adapter);
void sxe_dcb_configure(struct sxe_adapter *adapter);
u8 sxe_dcb_cee_get_tc_from_up(struct sxe_dcb_cee_config *cfg,
						u8 direction, u8 up);
s32 sxe_dcb_cee_tc_credits_calculate(struct sxe_hw *hw,
				   struct sxe_dcb_cee_config *dcb_config,
				   u32 max_frame, u8 direction);

void sxe_dcb_cee_refill_parse(struct sxe_dcb_cee_config *cfg,
					u8 direction, u16 *refill);
void sxe_dcb_cee_max_credits_parse(struct sxe_dcb_cee_config *cfg,
						u16 *max_credits);
void sxe_dcb_cee_bwgid_parse(struct sxe_dcb_cee_config *cfg,
						u8 direction, u8 *bwgid);
void sxe_dcb_cee_prio_parse(struct sxe_dcb_cee_config *cfg,
						u8 direction, u8 *ptype);
void sxe_dcb_cee_up2tc_map_parse(struct sxe_dcb_cee_config *cfg,
						u8 direction, u8 *map);
void sxe_dcb_cee_pfc_parse(struct sxe_dcb_cee_config *cfg, u8 *pfc_en);
void sxe_dcb_hw_pfc_configure(struct sxe_hw *hw, u8 pfc_en, u8 *prio_tc);
void sxe_dcb_hw_ets_configure(struct sxe_hw *hw,
			    u16 *refill, u16 *max, u8 *bwg_id,
			    u8 *prio_type, u8 *prio_tc);
s32 sxe_dcb_hw_ieee_ets_configure(struct sxe_hw *hw,
					struct ieee_ets *ets, u32 max_frame);
void sxe_dcb_pfc_configure(struct sxe_adapter *adapter);

void sxe_dcb_exit(struct sxe_adapter* adapter);

s32 sxe_dcb_tc_validate(struct sxe_adapter *adapter, u8 tc);

s32 sxe_dcb_tc_setup(struct sxe_adapter *adapter, u8 tc);

#endif 

void sxe_rx_drop_mode_set(struct sxe_adapter *adapter);
#endif

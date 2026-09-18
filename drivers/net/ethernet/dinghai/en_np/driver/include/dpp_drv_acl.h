/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_drv_acl.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 :
* 完成日期 : 2014/01/27
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#ifndef DPP_DRV_ACL_H
#define DPP_DRV_ACL_H

#include "zxic_common.h"
#include "dpp_apt_se_api.h"
#include "dpp_apt_se.h"

typedef struct zxdh_ipsec_enc_key
{
    ZXIC_UINT8  dip[16];
    ZXIC_UINT8  sip[16];
    ZXIC_UINT32 rsv2;
    ZXIC_UINT32 rsv1;
}ZXDH_IPSEC_ENC_KEY;

typedef struct zxdh_ipsec_enc_mask
{
    ZXIC_UINT8  dip[16];
    ZXIC_UINT8  sip[16];
    ZXIC_UINT32 rsv2;
    ZXIC_UINT32 rsv1;
}ZXDH_IPSEC_ENC_MASK;

typedef struct zxdh_ipsec_enc_entry
{
    ZXIC_UINT32 sa_id;
    ZXIC_UINT32 rsv;
    ZXIC_UINT32 hit_flag;
}ZXDH_IPSEC_ENC_ENTRY;

typedef struct zxdh_ipsec_enc_t
{
    ZXIC_UINT32 index;
    ZXDH_IPSEC_ENC_KEY key;
    ZXDH_IPSEC_ENC_MASK mask;
    ZXDH_IPSEC_ENC_ENTRY entry;
} ZXDH_IPSEC_ENC_T;

typedef struct zxdh_pkt_cap_key
{
    ZXIC_UINT32 rsv;
    ZXIC_UINT32 qp;
    ZXIC_UINT16 vhca_id;
    ZXIC_UINT16 vqm_vfid;
    ZXIC_UINT16 ethtype;
    ZXIC_UINT16 sport;
    ZXIC_UINT16 dport;
    ZXIC_UINT16 key_word_off;
    ZXIC_UINT8 protocol;
    ZXIC_UINT8 key_word_len;
    ZXIC_UINT8 capture_pkt_flag;
    ZXIC_UINT8 panel_id;
    ZXIC_UINT8 sip[16];
    ZXIC_UINT8 dip[16];
    ZXIC_UINT8 dmac[6];
    ZXIC_UINT8 smac[6];
    ZXIC_UINT8 key_word[15];
}ZXDH_PKT_CAP_KEY;

typedef struct zxdh_pkt_cap_mask
{
    ZXIC_UINT32 rsv_mask;
    ZXIC_UINT32 qp_mask;
    ZXIC_UINT16 vhca_id_mask;
    ZXIC_UINT16 vqm_vfid_mask;
    ZXIC_UINT16 ethtype_mask;
    ZXIC_UINT16 sport_mask;
    ZXIC_UINT16 dport_mask;
    ZXIC_UINT16 key_word_off_mask;
    ZXIC_UINT8 protocol_mask;
    ZXIC_UINT8 key_word_len_mask;
    ZXIC_UINT8 capture_pkt_flag_mask;
    ZXIC_UINT8 panel_id_mask;
    ZXIC_UINT8 sip_mask[16];
    ZXIC_UINT8 dip_mask[16];
    ZXIC_UINT8 dmac_mask[6];
    ZXIC_UINT8 smac_mask[6];
    ZXIC_UINT8 key_word_mask[15];
}ZXDH_PKT_CAP_MASK;

typedef struct zxdh_pkt_cap_entry
{
    ZXIC_UINT32 vqm_vfid;
    ZXIC_UINT32 index;
    ZXIC_UINT32 value_flag;
    ZXIC_UINT32 hit_flag;
}ZXDH_PKT_CAP_ENTRY;

typedef struct zxdh_pkt_cap_t
{
    ZXIC_UINT32 index;
    ZXDH_PKT_CAP_KEY key;
    ZXDH_PKT_CAP_MASK mask;
    ZXDH_PKT_CAP_ENTRY entry;
} ZXDH_PKT_CAP_T;

ZXIC_UINT32 dpp_apt_set_ipsec_enc_data(ZXIC_VOID *pData,DPP_ACL_ENTRY_EX_T *aclEntry);
ZXIC_UINT32 dpp_apt_get_ipsec_enc_data(ZXIC_VOID *pData,DPP_ACL_ENTRY_EX_T *aclEntry);
ZXIC_UINT32 dpp_apt_set_pkt_cap_data(ZXIC_VOID *pData, DPP_ACL_ENTRY_EX_T *aclEntry);
ZXIC_UINT32 dpp_apt_get_pkt_cap_data(ZXIC_VOID *pData, DPP_ACL_ENTRY_EX_T *aclEntry);

SE_APT_ACL_CONVERT_T *se_acl_callback_get(ZXIC_UINT32 sdt_no);

#endif

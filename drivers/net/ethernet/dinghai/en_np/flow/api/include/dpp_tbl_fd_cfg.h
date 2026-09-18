#ifndef DPP_TBL_FD_CFG_H
#define DPP_TBL_FD_CFG_H

#include "zxic_common.h"

typedef struct zxdh_fd_cfg_key
{
    ZXIC_UINT8 dmac[6];
    ZXIC_UINT8 smac[6];
    ZXIC_UINT32 ethtype;
    ZXIC_UINT16 cvlan_pri;
    ZXIC_UINT16 cvlanid;
    ZXIC_UINT8 sip[16];
    ZXIC_UINT8 dip[16];
    ZXIC_UINT8 rsv1;
    ZXIC_UINT8 tos;
    ZXIC_UINT8 proto;
    ZXIC_UINT8 fragment;
    ZXIC_UINT16 sport;
    ZXIC_UINT16 dport;
    ZXIC_UINT32 rsv2;
    ZXIC_UINT32 vxlan_vni;
    ZXIC_UINT16 vqm_vfid;
    ZXIC_UINT16 rsv3;
} ZXDH_FD_CFG_KEY;

typedef ZXDH_FD_CFG_KEY ZXDH_FD_CFG_MASK;

typedef struct zxdh_fd_cfg_as_rlt
{
    ZXIC_UINT8 hit_flag;
    ZXIC_UINT8 action_index;
    ZXIC_UINT16 action_index2;
    ZXIC_UINT32 v_qid;
    ZXIC_UINT32 uplink_fd_id;
    ZXIC_UINT32 spec_port_vfid;
    ZXIC_UINT32 count_id;
    ZXIC_UINT16 hash_alg;
    ZXIC_UINT16 rss_hash_factor;
    ZXIC_UINT16 rsv3;
    ZXIC_UINT16 encap0_index;
} ZXDH_FD_CFG_AS_RLT;

typedef struct zxdh_fd_cfg_t
{
    ZXDH_FD_CFG_KEY key;
    ZXDH_FD_CFG_MASK mask;
    ZXDH_FD_CFG_AS_RLT as_rlt;
} ZXDH_FD_CFG_T;

ZXIC_UINT32 dpp_tbl_fd_cfg_add(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, ZXDH_FD_CFG_T *p_fd_cfg);
ZXIC_UINT32 dpp_tbl_fd_cfg_del(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle);
ZXIC_UINT32 dpp_tbl_fd_cfg_get(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, ZXDH_FD_CFG_T *p_fd_cfg);
ZXIC_UINT32 dpp_tbl_fd_cfg_search(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXIC_UINT32 handle, ZXDH_FD_CFG_T *p_fd_cfg);

#endif

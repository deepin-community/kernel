#ifndef DPP_TBL_ROCE_MATCH_H
#define DPP_TBL_ROCE_MATCH_H

#include "zxic_common.h"

typedef struct zxdh_roce_match_key
{
    ZXIC_UINT8 sip[16];
    ZXIC_UINT8 dip[16];
    ZXIC_UINT32 rsv;
} ZXDH_ROCE_MATCH_KEY;

typedef struct zxdh_roce_match_entry
{
    ZXIC_UINT32 hit_flag;
    ZXIC_UINT32 rsv0;
    ZXIC_UINT32 vqm_vfid;
} ZXDH_ROCE_MATCH_ENTRY;

typedef struct zxdh_roce_match_t
{
    ZXDH_ROCE_MATCH_KEY key;
    ZXDH_ROCE_MATCH_ENTRY entry;
} ZXDH_ROCE_MATCH_T;

ZXIC_UINT32 dpp_tbl_roce_match_add(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_ROCE_MATCH_T *p_Data);
ZXIC_UINT32 dpp_tbl_roce_match_del(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_ROCE_MATCH_T *p_Data);
ZXIC_UINT32 dpp_tbl_roce_match_search(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_ROCE_MATCH_T *p_Data);

#endif

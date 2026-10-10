#ifndef DPP_TBL_PMTU_INFO_H
#define DPP_TBL_PMTU_INFO_H

#include "zxic_common.h"

typedef struct zxdh_pmtu_info_key
{
    ZXIC_UINT32 rsv0;
    ZXIC_UINT32 rsv1;
    ZXIC_UINT32 qp;
} ZXDH_PMTU_INFO_KEY;

typedef struct zxdh_pmtu_info_entry
{
    ZXIC_UINT32 hit_flag;
    ZXIC_UINT32 rsv0;
    ZXIC_UINT32 pmtu;
} ZXDH_PMTU_INFO_ENTRY;

typedef struct zxdh_pmtu_info_t
{
    ZXDH_PMTU_INFO_KEY key;
    ZXDH_PMTU_INFO_ENTRY entry;
} ZXDH_PMTU_INFO_T;

ZXIC_UINT32 dpp_tbl_pmtu_info_add(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_PMTU_INFO_T *p_Data);
ZXIC_UINT32 dpp_tbl_pmtu_info_del(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_PMTU_INFO_T *p_Data);
ZXIC_UINT32 dpp_tbl_pmtu_info_search(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_PMTU_INFO_T *p_Data);

#endif

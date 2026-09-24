#ifndef DPP_TBL_QP_INFO_H
#define DPP_TBL_QP_INFO_H

#include "zxic_common.h"

typedef struct zxdh_qp_info_key
{
    ZXIC_UINT32 rsv0;
    ZXIC_UINT32 rsv1;
    ZXIC_UINT32 qp;
} ZXDH_QP_INFO_KEY;

typedef struct zxdh_qp_info_entry
{
    ZXIC_UINT32 hit_flag;
    ZXIC_UINT32 rsv0;
    ZXIC_UINT32 psn;
    ZXIC_UINT32 length;
    ZXIC_UINT32 rsv1;
    ZXIC_UINT32 va_h;
    ZXIC_UINT32 va_l;
} ZXDH_QP_INFO_ENTRY;

typedef struct zxdh_qp_info_t
{
    ZXDH_QP_INFO_KEY key;
    ZXDH_QP_INFO_ENTRY entry;
} ZXDH_QP_INFO_T;

ZXIC_UINT32 dpp_tbl_qp_info_add(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_QP_INFO_T *p_Data);
ZXIC_UINT32 dpp_tbl_qp_info_del(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_QP_INFO_T *p_Data);
ZXIC_UINT32 dpp_tbl_qp_info_search(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_QP_INFO_T *p_Data);

#endif

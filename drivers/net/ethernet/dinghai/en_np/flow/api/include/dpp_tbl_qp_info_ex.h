#ifndef DPP_TBL_QP_INFO_EX_H
#define DPP_TBL_QP_INFO_EX_H

#include "dpp_tbl_qp_info.h"

ZXIC_UINT32 dpp_tbl_qp_info_add_ex(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_QP_INFO_T *p_Data);
ZXIC_UINT32 dpp_tbl_qp_info_del_ex(DPP_PF_INFO_T * pf_info, ZXIC_UINT32 sdt_no, ZXDH_QP_INFO_T *p_Data);

#endif

/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_pktrx_cfg.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : zzh
* 完成日期 : 2015/02/06
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#ifndef _DPP_PKTRX_CFG_H_
#define _DPP_PKTRX_CFG_H_

#include "dpp_pktrx_api.h"
#include "dpp_reg.h"

ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_0(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_0);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_1(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_1);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_2(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_2);
ZXIC_UINT32 dpp_pktrx_mcode_glb_cfg_get_3(DPP_DEV_T *dev, ZXIC_UINT32 *p_glb_cfg_data_3);

#endif
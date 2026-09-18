/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_drv_init.h
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

#ifndef DPP_DRV_INIT_H
#define DPP_DRV_INIT_H

#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_dev.h"

DPP_STATUS dpp_flow_init(DPP_DEV_T *dev);
DPP_STATUS dpp_flow_uninit(DPP_DEV_T *dev);
ZXIC_VOID dpp_flow_init_status_init(ZXIC_VOID);
DPP_STATUS dpp_flow_data_all_flush(DPP_DEV_T *dev, ZXIC_UINT32 queue_id);
DPP_STATUS dpp_bar_msg_num_init(DPP_DEV_T *dev);
#endif


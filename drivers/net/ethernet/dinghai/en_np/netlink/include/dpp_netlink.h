/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_netlink.h
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

#ifndef DPP_NETLINK_H
#define DPP_NETLINK_H

#include "zxic_common.h"
#include "dpp_type_api.h"

ZXIC_SINT32 dpp_netlink_init(ZXIC_VOID);
ZXIC_VOID dpp_netlink_exit(ZXIC_VOID);
DPP_STATUS dpp_netlink_regist_msg_proc_fun(ZXIC_UINT32 id, ZXIC_VOID *ptr);

#endif

/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_pci.h
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : 石金锋
* 完成日期 : 2014/02/10
* DEPARTMENT: ASIC_FPGA_R&D_Dept
* MANUAL_PERCENT: 100%

* 修改记录1: 代码规范性修改
* 修改日期:  2014/02/10
* 版 本 号:
* 修 改 人:  丁金凤
* 修改内容:
***************************************************************/

#ifndef _DPP_PCI_H_
#define _DPP_PCI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dpp_dev.h"
#include "dpp_module.h"

ZXIC_UINT32 dpp_pci_write32(DPP_DEV_T *dev, ZXIC_ADDR_T abs_addr, ZXIC_UINT32 *p_data);
ZXIC_UINT32 dpp_pci_read32(DPP_DEV_T *dev, ZXIC_ADDR_T  abs_addr, ZXIC_UINT32 *p_data);

#ifdef __cplusplus
}
#endif

#endif

/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_pci.c
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
#include "zxic_common.h"
#include "dpp_type_api.h"
#include "dpp_pci.h"
#include "dpp_dev.h"

/***********************************************************/
/**
* @param   abs_addr
* @param   p_data
*
* @return
* @remark  无
* @see
************************************************************/
ZXIC_UINT32 dpp_pci_write32(DPP_DEV_T *dev, ZXIC_ADDR_T abs_addr, ZXIC_UINT32 *p_data)
{
    /* ZXIC_UINT32 rtn  = 0; */
    ZXIC_UINT32 data = 0;
    ZXIC_UINT64 addr = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_data);

    data = *p_data;

    if (zxic_comm_is_big_endian())
    {
        data = ZXIC_COMM_CONVERT32(data);
    }

    addr = abs_addr + SYS_VF_NP_BASE_OFFSET;
    *((ZXIC_VOL ZXIC_UINT32 *)addr) = data;

    return DPP_OK;
}

/***********************************************************/
/**
* @param   abs_addr
* @param   p_data
*
* @return
* @remark  无
* @see
************************************************************/
ZXIC_UINT32 dpp_pci_read32(DPP_DEV_T *dev, ZXIC_ADDR_T abs_addr, ZXIC_UINT32 *p_data)
{
    ZXIC_UINT32 data = 0;
    ZXIC_UINT64 addr = 0;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_POINT(p_data);

    addr = abs_addr + SYS_VF_NP_BASE_OFFSET;
    data = *((ZXIC_VOL ZXIC_UINT32 *)addr);

    if (zxic_comm_is_big_endian())
    {
        data = ZXIC_COMM_CONVERT32(data);
    }
    *p_data = data;

    if (0xdadedade == *p_data)
    {
        ZXIC_COMM_TRACE_DEBUG("PCIE time out err happening at addr[0x%llx]\n", abs_addr);
    }

    return DPP_OK;
}

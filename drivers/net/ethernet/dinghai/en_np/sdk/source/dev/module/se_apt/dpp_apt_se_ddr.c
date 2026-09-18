/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : dpp_apt_se_ddr.c
* 文件标识 :
* 内容摘要 :
* 其它说明 :
* 当前版本 :
* 作    者 : 陈勤00181032
* 完成日期 : 2023/07/26
* MANUAL_PERCENT: 100%

* 修改记录1:
* 修改日期:
* 版 本 号:
* 修 改 人:
* 修改内容:
***************************************************************/

#include "dpp_apt_se.h"
#include "dpp_dev.h"
#include "dpp_sdt.h"

/***********************************************************/
/** DDR表资源初始化
* @param   dev_id  设备号 
* @param   tbl_num  需初始化的DDR表个数
* @param   pDdrTbl  ddr资源信息，包括SDT配置信息，直接表读取位宽和结构体码流转换回调函数
* @return  
* @remark  无
* @see     
* @author  chenqin00181032      @date  2023/07/26
************************************************************/
DPP_STATUS dpp_apt_ddr_res_init(DPP_DEV_T *dev,ZXIC_UINT32 tbl_num,DPP_APT_DDR_TABLE_T *pDdrTbl)
{
    DPP_STATUS rc = DPP_OK;
    ZXIC_UINT8 index = 0;
    DPP_APT_DDR_TABLE_T *pTempDdrTbl = NULL;

    ZXIC_COMM_CHECK_POINT(dev);
    ZXIC_COMM_CHECK_INDEX(DEV_ID(dev), 0, DPP_DEV_CHANNEL_MAX - 1);
    ZXIC_COMM_CHECK_INDEX_UPPER(tbl_num, DPP_DEV_SDT_ID_MAX);
    ZXIC_COMM_CHECK_POINT(pDdrTbl);

    for(index = 0;index < tbl_num;index++)
    {
        pTempDdrTbl = pDdrTbl + index;
        rc = dpp_sdt_tbl_write(dev,
                        pTempDdrTbl->sdtNo,
                        pTempDdrTbl->eDdrSdt.table_type,
                        &(pTempDdrTbl->eDdrSdt),
                        SDT_OPER_ADD);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_sdt_tbl_write");
        
        rc = dpp_apt_set_callback(dev,
                        pTempDdrTbl->sdtNo,
                        pTempDdrTbl->eDdrSdt.table_type,
                        (ZXIC_VOID *)pTempDdrTbl);
        ZXIC_COMM_CHECK_DEV_RC(DEV_ID(dev), rc, "dpp_apt_set_callback");
    }

    return DPP_OK;
}
